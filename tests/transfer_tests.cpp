#include "persistence/world_snapshot.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <new>
#include <sstream>
namespace {
std::ptrdiff_t fail_after = -1;
}
void* operator new(std::size_t size) {
    if (fail_after == 0) {
        fail_after = -1;
        throw std::bad_alloc{};
    }
    if (fail_after > 0)
        --fail_after;
    if (auto* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc{};
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
#include <stdexcept>
using namespace reality;
namespace {
void require(bool c, const char* m) {
    if (!c)
        throw std::runtime_error(m);
}
void reserved() {
    WorkWorld w;
    const auto a = w.create_actor();
    const auto x = w.create_resource(a, {10}, ResourceKind::Material, {1});
    const auto y = w.create_resource(a, {0}, ResourceKind::Material, {1});
    const auto action = w.reserve({{1}, a, x, {8}, {4}, 2});
    require(action.status == RequestStatus::Accepted, "reservation setup failed");
    require(w.transfer({{2}, a, x, y, {3}}).status == TransferStatus::InsufficientResource,
            "transfer stole reserved material");
    require(w.transfer({{3}, a, x, y, {2}}).status == TransferStatus::Accepted,
            "free remainder rejected");
    require(w.begin(action.action) == Transition::Changed, "reserved action could not start");
    require(w.transfer({{4}, a, x, y, {1}}).status == TransferStatus::InsufficientResource,
            "running reservation stolen");
    w.advance(2);
    w.validate();
    require(w.resource(x)->record.quantity.value == 0 && w.resource(y)->record.quantity.value == 2,
            "reserved consumption broke transfer balance");
}
void replay() {
    WorkWorld w;
    const auto a = w.create_actor();
    const auto x = w.create_resource(a, {10}, ResourceKind::Material, {1});
    const auto y = w.create_resource(a, {0}, ResourceKind::Material, {1});
    const TransferRequest denied{{1}, a, x, y, {11}};
    require(w.transfer(denied).status == TransferStatus::InsufficientResource,
            "fixture request not denied");
    const auto z = w.create_resource(a, {20}, ResourceKind::Material, {1});
    require(w.transfer({{2}, a, z, x, {20}}).status == TransferStatus::Accepted, "funding failed");
    const auto before = serialize_world(w);
    auto loaded_denial = deserialize_world(before);
    require(loaded_denial.transfer(denied).status == TransferStatus::InsufficientResource &&
                serialize_world(loaded_denial) == before,
            "loaded denied replay was re-evaluated");
    require(w.transfer(denied).status == TransferStatus::InsufficientResource &&
                serialize_world(w) == before,
            "denial replay was re-evaluated against later supply");
    bool rejected = false;
    try {
        static_cast<void>(w.transfer({{1}, a, x, y, {1}}));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && serialize_world(w) == before, "changed transfer payload was accepted");
    rejected = false;
    try {
        static_cast<void>(w.reserve({{2}, a, x, {1}, {1}, 1}));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && serialize_world(w) == before, "reserve reused transfer command id");
    static_cast<void>(w.reserve({{3}, a, x, {1}, {1}, 1}));
    const auto after = serialize_world(w);
    rejected = false;
    try {
        static_cast<void>(w.transfer({{3}, a, x, y, {1}}));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && serialize_world(w) == after, "transfer reused reservation command id");
    w.validate();
}
void refusals() {
    WorkWorld w;
    const auto a = w.create_actor();
    const auto x = w.create_resource(a, {10}, ResourceKind::Material, {1});
    const auto y = w.create_resource(a, {0}, ResourceKind::Material, {1});
    const auto wrong = w.create_resource(a, {0}, ResourceKind::Material, {2});
    const auto cap = w.create_resource(a, {4}, ResourceKind::Capacity);
    const auto unknown = w.create_resource(a, {4});
    const std::vector<std::pair<TransferRequest, TransferStatus>> cases{
        {{{1}, a, x, y, {0}}, TransferStatus::InvalidRequest},
        {{{2}, a, x, x, {1}}, TransferStatus::InvalidRequest},
        {{{3}, {999}, x, y, {1}}, TransferStatus::ActorUnavailable},
        {{{4}, a, x, {999}, {1}}, TransferStatus::ResourceUnavailable},
        {{{5}, a, x, wrong, {1}}, TransferStatus::IncompatibleMaterial},
        {{{6}, a, x, cap, {1}}, TransferStatus::IncompatibleMaterial},
        {{{7}, a, unknown, y, {1}}, TransferStatus::IncompatibleMaterial}};
    const auto events = w.events().size();
    for (const auto& [request, status] : cases) {
        require(w.transfer(request).status == status, "wrong transfer refusal");
        require(w.resource(x)->record.quantity.value == 10 &&
                    w.resource(y)->record.quantity.value == 0 && w.events().size() == events,
                "refused transfer partly mutated material");
    }
    w.validate();
}
void large_cycle() {
    WorkWorld w;
    const auto a = w.create_actor();
    const auto max = std::numeric_limits<std::uint64_t>::max();
    const auto x = w.create_resource(a, {max}, ResourceKind::Material, {1});
    const auto y = w.create_resource(a, {0}, ResourceKind::Material, {1});
    const auto z = w.create_resource(a, {1}, ResourceKind::Material, {1});
    require(w.transfer({{1}, a, z, x, {1}}).status == TransferStatus::Overflow,
            "target overflow accepted");
    for (std::uint64_t i = 0; i < 100; ++i) {
        require(w.transfer({{2 + i * 2}, a, x, y, {max}}).status == TransferStatus::Accepted,
                "large outgoing rejected");
        require(w.transfer({{3 + i * 2}, a, y, x, {max}}).status == TransferStatus::Accepted,
                "large return rejected");
    }
    w.validate();
    const auto bytes = serialize_world(w);
    require(serialize_world(deserialize_world(bytes)) == bytes &&
                w.resource(x)->record.quantity.value == max,
            "valid repeated flow overflowed the audit or snapshot");
}
void removed() {
    WorkWorld w;
    const auto a = w.create_actor();
    const auto x = w.create_resource(a, {10}, ResourceKind::Material, {1});
    const auto y = w.create_resource(a, {0}, ResourceKind::Material, {1});
    const TransferRequest request{{1}, a, x, y, {8}};
    require(w.transfer(request).status == TransferStatus::Accepted, "fixture transfer failed");
    require(w.remove_resource(y) && w.resource(y)->record.lost.value == 8,
            "incoming material lost without accounting");
    require(w.remove_actor(a), "actor removal failed");
    w.validate();
    const auto bytes = serialize_world(w);
    require(w.transfer(request).status == TransferStatus::Accepted && serialize_world(w) == bytes,
            "replay after removal changed history");
    require(serialize_world(deserialize_world(bytes)) == bytes,
            "loss after inflow cannot be restored");
}
void basic() {
    WorkWorld w;
    auto a = w.create_actor();
    auto x = w.create_resource(a, {10}, ResourceKind::Material, MaterialTypeId{1});
    auto y = w.create_resource(a, {0}, ResourceKind::Material, MaterialTypeId{1});
    const TransferRequest r{{1}, a, x, y, {3}};
    require(w.transfer(r).status == TransferStatus::Accepted, "transfer rejected");
    require(w.resource(x)->record.quantity.value == 7 && w.resource(y)->record.quantity.value == 3,
            "transfer did not conserve material");
    require(w.transfer(r).status == TransferStatus::Accepted &&
                w.resource(y)->record.quantity.value == 3,
            "replay duplicated material");
    auto q = w.reserve({{2}, a, y, {2}, {1}, 2});
    require(q.status == RequestStatus::Accepted, "received material unusable");
    require(w.begin(q.action) == Transition::Changed, "could not consume received lot");
    w.advance(2);
    w.validate();
    require(w.resource(y)->record.quantity.value == 1, "consumption after transfer wrong");
    require(serialize_world(deserialize_world(serialize_world(w))) == serialize_world(w),
            "transfer snapshot changed");
}
void boundaries() {
    WorkWorld w;
    const auto a = w.create_actor();
    const auto x = w.create_resource(a, {10}, ResourceKind::Material, {1});
    const auto y = w.create_resource(a, {0}, ResourceKind::Material, {1});
    const TransferRequest first{{1}, a, x, y, {1}};
    require(w.transfer(first).status == TransferStatus::Accepted, "limit fixture transfer failed");
    for (std::uint64_t i = 2; i <= record_limit; ++i) {
        if (i % 2 == 0)
            static_cast<void>(w.reserve({{i}, a, x, {0}, {0}, 1}));
        else
            static_cast<void>(w.transfer({{i}, a, x, y, {0}}));
    }
    const auto before = serialize_world(w);
    bool rejected = false;
    try {
        static_cast<void>(w.transfer({{record_limit + 1}, a, x, y, {1}}));
    } catch (const std::length_error&) {
        rejected = true;
    }
    require(rejected && serialize_world(w) == before,
            "full command journal partly accepted transfer");
    require(w.transfer(first).status == TransferStatus::Accepted && serialize_world(w) == before,
            "full journal rejected accepted replay");
    require(w.transfer({{3}, a, x, y, {0}}).status == TransferStatus::InvalidRequest &&
                serialize_world(w) == before,
            "full journal rejected denied replay");
    WorkWorld e;
    const auto owner = e.create_actor();
    const auto source = e.create_resource(owner, {10}, ResourceKind::Material, {1});
    const auto destination = e.create_resource(owner, {0}, ResourceKind::Material, {1});
    while (e.events().size() < record_limit - 1)
        static_cast<void>(e.create_actor());
    const auto state = serialize_world(e);
    rejected = false;
    try {
        static_cast<void>(e.transfer({{1}, owner, source, destination, {1}}));
    } catch (const std::length_error&) {
        rejected = true;
    }
    require(rejected && serialize_world(e) == state,
            "single remaining event slot allowed half a transfer");
}
void allocation() {
    for (const auto fault : {0, 1}) {
        WorkWorld w;
        const auto a = w.create_actor();
        const auto x = w.create_resource(a, {10}, ResourceKind::Material, {1});
        const auto y = w.create_resource(a, {0}, ResourceKind::Material, {1});
        for (unsigned i = 0; i < 4; ++i)
            static_cast<void>(w.create_actor());
        const auto before = serialize_world(w);
        bool rejected = false;
        fail_after = fault;
        try {
            static_cast<void>(w.transfer({{1}, a, x, y, {1}}));
        } catch (const std::bad_alloc&) {
            rejected = true;
        }
        fail_after = -1;
        require(rejected && serialize_world(w) == before,
                "allocation failure partly moved material");
        require(w.transfer({{1}, a, x, y, {1}}).status == TransferStatus::Accepted,
                "failed allocation poisoned command retry");
        w.validate();
    }
}
std::string checksum(std::string body) {
    body.resize(body.rfind("CHECKSUM "));
    std::ostringstream out;
    out << body << "CHECKSUM " << std::hex << std::setw(16) << std::setfill('0')
        << snapshot_checksum(body) << '\n';
    return out.str();
}
std::string field(std::string bytes, std::string_view section, unsigned row, unsigned column,
                  std::string value) {
    auto first = bytes.find('\n', bytes.find(section)) + 1;
    for (unsigned i = 0; i < row; ++i)
        first = bytes.find('\n', first) + 1;
    const auto last = bytes.find('\n', first);
    std::istringstream in{bytes.substr(first, last - first)};
    std::ostringstream out;
    std::string word;
    unsigned n = 0;
    while (in >> word) {
        if (n)
            out << ' ';
        out << (n++ == column ? value : word);
    }
    require(n > column, "snapshot field missing");
    bytes.replace(first, last - first, out.str());
    return checksum(bytes);
}
std::string reorder(std::string bytes, std::initializer_list<unsigned> order,
                    std::uint64_t first_transfer) {
    const auto first = bytes.find('\n', bytes.find("EVENTS ")) + 1,
               last = bytes.find("END\n", first);
    std::istringstream in{bytes.substr(first, last - first)};
    std::vector<std::string> rows;
    std::string row;
    while (std::getline(in, row))
        rows.push_back(row);
    std::string changed;
    unsigned sequence = 1;
    for (auto i : order) {
        auto line = rows.at(i);
        line.replace(0, line.find(' '), std::to_string(sequence++));
        changed += line + '\n';
    }
    bytes.replace(first, last - first, changed);
    return field(bytes, "TRANSFERS ", 0, 7, std::to_string(first_transfer));
}
void corruption() {
    WorkWorld w;
    const auto a = w.create_actor();
    const auto x = w.create_resource(a, {10}, ResourceKind::Material, {1});
    const auto y = w.create_resource(a, {0}, ResourceKind::Material, {1});
    static_cast<void>(w.transfer({{1}, a, x, y, {3}}));
    const auto good = serialize_world(w);
    const auto reject = [&](const std::string& bytes) {
        bool denied = false;
        try {
            static_cast<void>(deserialize_world(bytes));
        } catch (const std::exception&) {
            denied = true;
        }
        require(denied, "semantic transfer corruption accepted");
    };
    for (const auto& bad : std::vector<std::string>{
             field(good, "TRANSFERS ", 0, 4, "4"), field(good, "TRANSFERS ", 0, 7, "999"),
             field(good, "EVENTS ", 3, 4, "4"), field(good, "RESOURCES ", 1, 7, "2"),
             field(field(good, "RESOURCES ", 0, 3, "8"), "RESOURCES ", 1, 3, "2"),
             reorder(good, {0, 3, 4, 1, 2}, 2)})
        reject(bad);
    static_cast<void>(w.reserve({{2}, a, x, {1}, {0}, 1}));
    reject(field(serialize_world(w), "TRANSFERS ", 0, 0, "2"));
    WorkWorld h;
    const auto actor = h.create_actor();
    const auto source = h.create_resource(actor, {10}, ResourceKind::Material, {1});
    const auto target = h.create_resource(actor, {0}, ResourceKind::Material, {1});
    auto action = h.reserve({{1}, actor, source, {8}, {0}, 2});
    static_cast<void>(h.transfer({{2}, actor, source, target, {2}}));
    static_cast<void>(h.cancel(action.action));
    auto bad = field(serialize_world(h), "TRANSFERS ", 0, 4, "3");
    bad = field(bad, "EVENTS ", 4, 4, "3");
    bad = field(bad, "EVENTS ", 5, 4, "3");
    bad = field(bad, "RESOURCES ", 0, 3, "7");
    bad = field(bad, "RESOURCES ", 1, 3, "3");
    reject(bad);
    WorkWorld removed = deserialize_world(good);
    static_cast<void>(removed.remove_resource(y));
    // The accepted pair must not be moved behind resource removal.
    auto raw = serialize_world(removed);
    reject(reorder(raw, {0, 1, 2, 5, 3, 4}, 5));
    auto restored = deserialize_world(good);
    const auto before = serialize_world(restored);
    require(restored.transfer({{1}, a, x, y, {3}}).status == TransferStatus::Accepted &&
                serialize_world(restored) == before,
            "loaded accepted replay changed material");
}
void legacy() {
    for (const auto name :
         {"work-world-v1-before-transfer.save", "work-world-v2-before-transfer.save"}) {
        const auto bytes =
            read_snapshot_bytes(std::filesystem::path{REALITY_TRANSFER_FIXTURES} / name);
        require(serialize_world(deserialize_world(bytes)) == bytes,
                "historical world bytes changed");
    }
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected transfer test name");
        const std::string_view name{argv[1]};
        if (name == "boundaries")
            boundaries();
        else if (name == "allocation")
            allocation();
        else if (name == "corruption")
            corruption();
        else if (name == "legacy")
            legacy();
        else if (name == "basic")
            basic();
        else if (name == "reserved")
            reserved();
        else if (name == "replay")
            replay();
        else if (name == "refusals")
            refusals();
        else if (name == "large_cycle")
            large_cycle();
        else if (name == "removed")
            removed();
        else
            throw std::invalid_argument("unknown transfer test");
        std::cout << "PASS transfer " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
