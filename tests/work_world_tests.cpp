#include "persistence/world_snapshot.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace reality;
using namespace std::chrono_literals;

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string{message});
    }
}

template <typename Exception = std::exception, typename Operation>
void require_throws(Operation operation) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error("expected exception was not thrown");
}

struct Fixture {
    WorkWorld world;
    EntityId first = world.create_actor();
    EntityId second = world.create_actor();
    EntityId lot = world.create_resource(first, Quantity{10});

    WorkRequest request(std::uint64_t command = 1, std::uint64_t required = 6,
                        std::uint64_t upfront = 2, std::uint64_t duration = 5) const {
        return {CommandId{command}, first, lot, Quantity{required}, Quantity{upfront}, duration};
    }

    EntityId reserve() {
        const auto result = world.reserve(request());
        require(result.status == RequestStatus::Accepted, "fixture reservation rejected");
        return result.action;
    }
};

void balance(const WorkWorld& world, EntityId lot, std::uint64_t physical, std::uint64_t reserved,
             std::uint64_t available) {
    const auto view = world.resource(lot);
    require(view.has_value(), "missing resource view");
    require(view->record.quantity.value == physical && view->reserved.value == reserved &&
                view->available.value == available,
            "resource balance differs from expected outcome");
    world.validate();
}

void stale_handle() {
    EntityRegistry registry;
    const auto old = registry.create(EntityKind::Actor);
    const auto handle = registry.handle(old).value();
    require(registry.resolve(handle) == old, "fresh handle does not resolve");
    require(registry.destroy(old), "live entity not destroyed");
    require(!registry.destroy(old), "duplicate deletion succeeded");
    const auto next = registry.create(EntityKind::Resource);
    const auto next_handle = registry.handle(next).value();
    require(next != old && next_handle.slot == handle.slot &&
                next_handle.generation != handle.generation,
            "slot reuse aliased identity");
    require(!registry.resolve(handle) && !registry.handle(old), "stale handle resolved");
    require(!registry.resolve(EntityHandle{4000, 1}), "out-of-range handle resolved");
    auto restored = EntityRegistry::restore(registry.next_id(), registry.slots());
    require(!restored.resolve(handle) && restored.resolve(next_handle) == next,
            "restoring registry lost handle generations");
}

void registry_exhaustion() {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    auto registry = EntityRegistry::restore(2, {{EntityId{1}, EntityKind::Actor, maximum, true}});
    const auto old = registry.handle(EntityId{1}).value();
    require(registry.destroy(EntityId{1}), "max-generation entity not destroyed");
    const auto fresh = registry.create(EntityKind::Actor);
    require(registry.handle(fresh)->slot != old.slot && !registry.resolve(old),
            "exhausted generation slot was reused");
    auto exhausted = EntityRegistry::restore(maximum, {});
    require_throws<std::overflow_error>([&] { return exhausted.create(EntityKind::Actor); });
    require(exhausted.next_id() == maximum && exhausted.slots().empty(),
            "id exhaustion mutated registry");
}

void registry_validation() {
    require_throws([] { return EntityRegistry::restore(0, {}); });
    require_throws(
        [] { return EntityRegistry::restore(2, {{EntityId{1}, EntityKind::Actor, 0, true}}); });
    require_throws(
        [] { return EntityRegistry::restore(1, {{EntityId{1}, EntityKind::Actor, 1, true}}); });
    require_throws([] {
        return EntityRegistry::restore(2, {{EntityId{1}, EntityKind::Actor, 1, true},
                                           {EntityId{1}, EntityKind::Actor, 1, false}});
    });
    EntityRegistry registry;
    require_throws<std::invalid_argument>(
        [&] { return registry.create(static_cast<EntityKind>(99)); });
    require(registry.next_id() == 1, "invalid kind allocated an id");
}

void reservation_contention() {
    Fixture f;
    const auto first = f.world.reserve(f.request(1, 10, 0));
    auto competitor = f.request(2, 1, 0);
    competitor.actor = f.second;
    const auto denied = f.world.reserve(competitor);
    require(first.status == RequestStatus::Accepted &&
                denied.status == RequestStatus::InsufficientResource && denied.action.value == 0,
            "two actors reserved the same stock");
    balance(f.world, f.lot, 10, 10, 0);
    require(f.world.cancel(first.action) == Transition::Changed, "reservation did not cancel");
    require(f.world.reserve(competitor) == denied, "denied command replay changed outcome");
    competitor.command = CommandId{3};
    require(f.world.reserve(competitor).status == RequestStatus::Accepted,
            "fresh command could not reserve released stock");
    balance(f.world, f.lot, 10, 1, 9);
}

void command_retries() {
    Fixture f;
    const auto request = f.request();
    const auto first = f.world.reserve(request);
    const auto before = serialize_world(f.world);
    require(f.world.reserve(request) == first, "command replay returned a new action");
    auto conflict = request;
    conflict.required = Quantity{5};
    require_throws<std::invalid_argument>([&] { return f.world.reserve(conflict); });
    require(serialize_world(f.world) == before, "replay or conflict mutated world");
    require(f.world.begin(first.action) == Transition::Changed, "begin failed");
    f.world.advance(5);
    const auto completed = serialize_world(f.world);
    require(f.world.reserve(request) == first && serialize_world(f.world) == completed,
            "completed command repeated its effects");
    balance(f.world, f.lot, 4, 0, 4);
}

void cancel_before_start() {
    Fixture f;
    const auto id = f.reserve();
    require(f.world.cancel(id) == Transition::Changed, "cancel failed");
    const auto before = serialize_world(f.world);
    require(f.world.cancel(id) == Transition::Unchanged &&
                f.world.begin(id) == Transition::InvalidState,
            "cancelled transition incorrect");
    require(serialize_world(f.world) == before, "repeated cancellation emitted extra effects");
    f.world.advance(10);
    balance(f.world, f.lot, 10, 0, 10);
}

void partial_cancellation() {
    Fixture f;
    const auto id = f.reserve();
    require(f.world.begin(id) == Transition::Changed, "begin failed");
    balance(f.world, f.lot, 8, 4, 4);
    f.world.advance(2);
    require(f.world.cancel(id) == Transition::Changed, "running action did not cancel");
    balance(f.world, f.lot, 8, 0, 8);
    f.world.advance(20);
    balance(f.world, f.lot, 8, 0, 8);
    require(f.world.action(id)->consumed.value == 2 && f.world.action(id)->ended_at == 2,
            "cancellation refunded consumed stock or changed its time");
}

void completion_once() {
    Fixture f;
    const auto id = f.reserve();
    f.world.advance(3);
    require(f.world.begin(id) == Transition::Changed, "begin failed");
    const auto started = serialize_world(f.world);
    require(f.world.begin(id) == Transition::Unchanged && serialize_world(f.world) == started,
            "repeated begin consumed stock");
    f.world.advance(4);
    require(f.world.action(id)->state == ActionState::Running, "work completed before deadline");
    f.world.advance(1);
    require(f.world.action(id)->state == ActionState::Completed &&
                f.world.action(id)->ended_at == 8,
            "work did not complete at deadline");
    balance(f.world, f.lot, 4, 0, 4);
    const auto event_count = f.world.events().size();
    require(f.world.begin(id) == Transition::Unchanged &&
                f.world.cancel(id) == Transition::InvalidState,
            "completed action restarted or cancelled");
    f.world.advance(100);
    require(f.world.events().size() == event_count, "completion emitted twice");
    balance(f.world, f.lot, 4, 0, 4);

    Fixture all_upfront;
    const auto full = all_upfront.world.reserve(all_upfront.request(1, 10, 10, 1)).action;
    require(all_upfront.world.begin(full) == Transition::Changed, "begin failed");
    balance(all_upfront.world, all_upfront.lot, 0, 0, 0);
    all_upfront.world.advance(1);
    balance(all_upfront.world, all_upfront.lot, 0, 0, 0);
}

void participant_removal() {
    Fixture f;
    const auto id = f.reserve();
    const auto old_handle = f.world.entities().handle(f.first).value();
    require(f.world.begin(id) == Transition::Changed, "begin failed");
    require(f.world.remove_actor(f.first) && !f.world.remove_actor(f.first),
            "actor deletion not idempotent");
    require(f.world.action(id)->reason == CancelReason::ActorRemoved,
            "actor deletion did not cancel work");
    const auto replacement = f.world.create_actor();
    require(replacement != f.first && !f.world.entities().resolve(old_handle),
            "actor slot aliased old identity");
    balance(f.world, f.lot, 8, 0, 8);
    auto request = f.request(2, 4, 1);
    request.actor = f.second;
    const auto next = f.world.reserve(request).action;
    require(f.world.begin(next) == Transition::Changed, "begin failed");
    require(f.world.remove_resource(f.lot) && !f.world.remove_resource(f.lot),
            "lot deletion not idempotent");
    require(f.world.action(next)->reason == CancelReason::ResourceRemoved &&
                f.world.resource(f.lot)->record.lost.value == 7,
            "lot deletion lost cancellation or loss accounting");
    f.world.advance(50);
    balance(f.world, f.lot, 0, 0, 0);
}

void invalid_requests() {
    Fixture f;
    const auto before = serialize_world(f.world);
    auto request = f.request(0);
    require_throws<std::invalid_argument>([&] { return f.world.reserve(request); });
    require_throws<std::invalid_argument>(
        [&] { return f.world.create_resource(f.lot, Quantity{1}); });
    require(serialize_world(f.world) == before, "invalid command identity or owner mutated world");
    require(f.world.reserve(f.request(1, 0, 0)).status == RequestStatus::InvalidRequest,
            "zero requirement accepted");
    require(f.world.reserve(f.request(2, 1, 2)).status == RequestStatus::InvalidRequest,
            "excess upfront accepted");
    require(f.world.reserve(f.request(3, 1, 0, 0)).status == RequestStatus::InvalidRequest,
            "zero duration accepted");
    request = f.request(4);
    request.actor = f.lot;
    require(f.world.reserve(request).status == RequestStatus::ActorUnavailable,
            "lot treated as actor");
    request = f.request(5);
    request.lot = f.first;
    require(f.world.reserve(request).status == RequestStatus::ResourceUnavailable,
            "actor treated as lot");
    require(f.world.begin(EntityId{999}) == Transition::Missing &&
                f.world.cancel(f.first) == Transition::Missing,
            "unknown action transition accepted");
    balance(f.world, f.lot, 10, 0, 10);
    WorkRequest empty;
    require_throws<std::invalid_argument>([&] { return f.world.reserve(empty); });
    require(!f.world.entities().resolve(EntityHandle{}), "default handle resolved");
}

void time_overflow() {
    WorkWorld world{std::chrono::nanoseconds{std::numeric_limits<std::int64_t>::max()}};
    const auto actor = world.create_actor();
    const auto lot = world.create_resource(actor, Quantity{2});
    const auto id = world.reserve({CommandId{1}, actor, lot, Quantity{2}, Quantity{1}, 1}).action;
    world.advance(1);
    const auto before = serialize_world(world);
    require(world.begin(id) == Transition::TimeOverflow && serialize_world(world) == before,
            "deadline overflow consumed or changed reservation");
    require_throws<std::overflow_error>([&] { world.advance(1); });
    require(serialize_world(world) == before, "clock overflow changed world");
    balance(world, lot, 2, 2, 0);

    WorkWorld bounded{1ns};
    const auto person = bounded.create_actor();
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    const auto stock = bounded.create_resource(person, Quantity{maximum});
    const auto work =
        bounded.reserve({CommandId{1}, person, stock, Quantity{maximum}, Quantity{maximum - 1}, 1});
    require(bounded.begin(work.action) == Transition::Changed, "maximum quantity begin failed");
    balance(bounded, stock, 1, 1, 0);
    bounded.advance(1);
    balance(bounded, stock, 0, 0, 0);
}

void snapshot_continuation() {
    Fixture f;
    const auto id = f.reserve();
    auto reserved = deserialize_world(serialize_world(f.world));
    require(reserved.begin(id) == Transition::Changed, "reserved action did not survive load");
    reserved.advance(2);
    const auto running = serialize_world(reserved);
    auto loaded = deserialize_world(running);
    require(serialize_world(loaded) == running, "snapshot round trip changed running state");
    loaded.advance(3);
    balance(loaded, f.lot, 4, 0, 4);
    require(loaded.action(id)->state == ActionState::Completed, "loaded action did not complete");
    const auto before = serialize_world(loaded);
    require(loaded.reserve(f.request()).action == id && serialize_world(loaded) == before,
            "command ledger did not survive load");
    loaded.advance(1);
    balance(loaded, f.lot, 4, 0, 4);
}

void snapshot_history() {
    Fixture f;
    const auto id = f.reserve();
    const auto handle = f.world.entities().handle(f.first).value();
    require(f.world.begin(id) == Transition::Changed, "begin failed");
    f.world.remove_actor(f.first);
    f.world.remove_resource(f.lot);
    const auto replacement = f.world.create_actor();
    const auto bytes = serialize_world(f.world);
    auto loaded = deserialize_world(bytes);
    require(serialize_world(loaded) == bytes && !loaded.entities().resolve(handle),
            "deleted identity or history changed after load");
    require(loaded.create_actor().value > replacement.value, "load reused historical id");
    require(loaded.reserve(f.request()).action == id, "cancelled command ledger lost");
    balance(loaded, f.lot, 0, 0, 0);
}

// Altered, checksummed fixtures exercise semantic validation beyond accidental corruption.
std::string amended(std::string bytes, std::string_view from, std::string_view to) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto offset = bytes.find(from);
    require(offset != std::string::npos, "snapshot mutation target absent");
    bytes.replace(offset, from.size(), to);
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char byte : bytes) {
        hash = (hash ^ static_cast<unsigned char>(byte)) * 0x100000001b3ULL;
    }
    std::ostringstream out;
    out << bytes << "CHECKSUM " << std::hex << hash << '\n';
    return out.str();
}

void snapshot_rejection() {
    Fixture f;
    const auto bytes = serialize_world(f.world);
    require_throws([&] { return deserialize_world(bytes.substr(0, bytes.size() / 2)); });
    auto corrupt = bytes;
    corrupt[0] = 'X';
    require_throws([&] { return deserialize_world(corrupt); });
    require_throws([&] {
        return deserialize_world(amended(bytes, "GENESIS_WORK_WORLD 1", "GENESIS_WORK_WORLD 3"));
    });
    require_throws([&] { return deserialize_world(amended(bytes, "ACTORS 2", "ACTORS 999999")); });
    require_throws(
        [&] { return deserialize_world(amended(bytes, "CLOCK 50000000 0", "CLOCK -1 0")); });
    require_throws([&] { return deserialize_world(bytes + "extra"); });
    require_throws([] { return deserialize_world(std::string(snapshot_byte_limit + 1, 'x')); });
    require(serialize_world(f.world) == bytes, "failed load changed existing world");
}
void capacity_lifecycle() {
    WorkWorld world;
    const auto first = world.create_actor(), second = world.create_actor();
    const auto slot = world.create_resource(first, Quantity{1}, ResourceKind::Capacity);
    const WorkRequest request{CommandId{1}, first, slot, Quantity{1}, Quantity{0}, 5};
    const auto accepted = world.reserve(request);
    require(accepted.status == RequestStatus::Accepted, "capacity request rejected");
    balance(world, slot, 1, 1, 0);
    require(world.reserve({CommandId{2}, second, slot, Quantity{1}, Quantity{0}, 5}).status ==
                RequestStatus::InsufficientResource,
            "occupied capacity double booked");
    require(world.begin(accepted.action) == Transition::Changed, "capacity action did not start");
    balance(world, slot, 1, 1, 0);
    auto loaded = deserialize_world(serialize_world(world));
    loaded.advance(5);
    loaded.validate();
    balance(loaded, slot, 1, 0, 1);
    require(loaded.action(accepted.action)->consumed.value == 0, "completion consumed capacity");
    const auto before_retry = serialize_world(loaded);
    require(loaded.reserve(request) == accepted && serialize_world(loaded) == before_retry,
            "replayed capacity command created a second action");
    const auto next = loaded.reserve({CommandId{3}, second, slot, Quantity{1}, Quantity{0}, 5});
    require(next.status == RequestStatus::Accepted, "completed capacity not reusable");
    require(loaded.cancel(next.action) == Transition::Changed, "reserved capacity not cancelled");
    balance(loaded, slot, 1, 0, 1);
    const auto running = loaded.reserve({CommandId{4}, second, slot, Quantity{1}, Quantity{0}, 5});
    require(loaded.begin(running.action) == Transition::Changed, "second use failed");
    require(loaded.cancel(running.action) == Transition::Changed, "running capacity not cancelled");
    balance(loaded, slot, 1, 0, 1);
    require(loaded.reserve({CommandId{5}, first, slot, Quantity{1}, Quantity{1}, 5}).status ==
                RequestStatus::InvalidRequest,
            "capacity accepted upfront consumption");
    for (const auto& event : loaded.events())
        require(event.kind != EventKind::Consumed, "capacity emitted consumption");
    const auto stable = serialize_world(loaded);
    require_throws(
        [&] { return loaded.create_resource(first, Quantity{1}, static_cast<ResourceKind>(99)); });
    require(serialize_world(loaded) == stable, "invalid kind mutated world");
}
void capacity_removal() {
    for (const bool remove_place : {false, true}) {
        for (const bool begin : {false, true}) {
            WorkWorld world;
            const auto owner = world.create_actor(), user = world.create_actor();
            const auto slot = world.create_resource(owner, Quantity{1}, ResourceKind::Capacity);
            const auto action =
                world.reserve({CommandId{1}, user, slot, Quantity{1}, Quantity{0}, 5});
            if (begin)
                require(world.begin(action.action) == Transition::Changed, "begin failed");
            require(remove_place ? world.remove_resource(slot) : world.remove_actor(user),
                    "participant removal failed");
            require(
                world.action(action.action)->state == ActionState::Cancelled &&
                    world.action(action.action)->reason ==
                        (remove_place ? CancelReason::ResourceRemoved : CancelReason::ActorRemoved),
                "removal did not cancel capacity with its reason");
            balance(world, slot, remove_place ? 0 : 1, 0, remove_place ? 0 : 1);
            const auto bytes = serialize_world(world);
            require(serialize_world(deserialize_world(bytes)) == bytes,
                    "removed capacity history changed");
        }
    }
}
void capacity_snapshot() {
    WorkWorld world;
    const auto first = world.create_actor();
    const auto second = world.create_actor();
    const auto slot = world.create_resource(first, Quantity{1}, ResourceKind::Capacity);
    const auto action = world.reserve({CommandId{1}, second, slot, Quantity{1}, Quantity{0}, 5});
    require(world.begin(action.action) == Transition::Changed, "snapshot fixture begin failed");
    const auto material = world.create_resource(first, Quantity{7});
    const auto bytes = serialize_world(world);
    require(bytes.starts_with("GENESIS_WORK_WORLD 2\n"), "capacity silently used version 1");
    auto loaded = deserialize_world(bytes);
    require(serialize_world(loaded) == bytes &&
                loaded.resource(material)->record.kind == ResourceKind::Material &&
                loaded.resource(slot)->record.kind == ResourceKind::Capacity,
            "mixed resource kinds lost on load");
    require_throws(
        [&] { return deserialize_world(amended(bytes, "3 1 1 1 0 1 1\n", "3 1 1 1 0 1 99\n")); });
    require_throws([&] {
        return deserialize_world(
            amended(bytes, "4 1 2 3 1 0 5 0 1 0 5 0 0", "4 1 2 3 1 0 5 1 1 0 5 0 0"));
    });
    require(serialize_world(world) == bytes, "failed read mutated source world");
    Fixture legacy;
    const auto v1 = serialize_world(legacy.world);
    require(v1.starts_with("GENESIS_WORK_WORLD 1\n") &&
                serialize_world(deserialize_world(v1)) == v1,
            "material-only version 1 changed");
}

void snapshot_semantics() {
    Fixture f;
    const auto id = f.reserve();
    require(f.world.begin(id) == Transition::Changed, "begin failed");
    const auto bytes = serialize_world(f.world);
    require_throws(
        [&] { return deserialize_world(amended(bytes, "3 1 10 8 0 1", "3 1 10 9 0 1")); });
    require_throws([&] {
        return deserialize_world(amended(bytes, "ACTORS 2\n1 1\n2 1", "ACTORS 2\n1 1\n1 1"));
    });
    require_throws([&] {
        return deserialize_world(
            amended(bytes, "4 1 1 3 6 2 5 2 1 0 5 0 0", "4 1 1 3 6 2 5 2 1 0 4 0 0"));
    });
    require_throws([&] {
        return deserialize_world(
            amended(bytes, "4 1 1 3 6 2 5 2 1 0 5 0 0", "4 1 1 3 6 2 5 2 99 0 5 0 0"));
    });
    require_throws(
        [&] { return deserialize_world(amended(bytes, "2 1 1 1\n3 2", "2 1 1 0\n3 2")); });
    auto unreachable = amended(bytes, "4 1 1 3 6 2 5 2 1 0 5 0 0",
                               "4 1 1 3 6 2 18446744073709551615 2 1 0 18446744073709551615 0 0");
    unreachable = amended(unreachable, "COMMANDS 1\n1 1 3 6 2 5 0 4",
                          "COMMANDS 1\n1 1 3 6 2 18446744073709551615 0 4");
    require_throws([&] { return deserialize_world(unreachable); });
}

void snapshot_journal() {
    Fixture f;
    const auto id = f.reserve();
    require(f.world.begin(id) == Transition::Changed, "begin failed");
    const auto bytes = serialize_world(f.world);
    require_throws([&] { return deserialize_world(amended(bytes, "1 0 0 1 0", "1 0 0 3 0")); });
    require_throws([&] { return deserialize_world(amended(bytes, "4 0 4 4 6", "4 0 7 4 6")); });
    require_throws([&] { return deserialize_world(amended(bytes, "5 0 5 4 0", "5 0 5 4 1")); });
    require_throws([&] { return deserialize_world(amended(bytes, "6 0 6 4 2", "6 0 6 4 1")); });
    f.world.advance(5);
    const auto completed = serialize_world(f.world);
    require_throws([&] { return deserialize_world(amended(completed, "8 5 7 4 0", "8 5 5 4 0")); });
}

class TestDirectory final {
  public:
    TestDirectory() {
        for (unsigned attempt = 0; attempt < 100; ++attempt) {
            path = std::filesystem::temp_directory_path() /
                   ("genesis-work-test-" + std::to_string(GetCurrentProcessId()) + "-" +
                    std::to_string(attempt));
            if (std::filesystem::create_directory(path)) {
                return;
            }
        }
        throw std::runtime_error("test temporary directory names exhausted");
    }
    ~TestDirectory() {
        std::error_code ignored;
        for (const auto& entry : std::filesystem::directory_iterator(path, ignored)) {
            std::filesystem::remove(entry.path(), ignored);
        }
        std::filesystem::remove(path, ignored);
    }
    TestDirectory(const TestDirectory&) = delete;
    TestDirectory& operator=(const TestDirectory&) = delete;
    std::filesystem::path path;
};

struct CloseFile {
    void operator()(void* handle) const noexcept { CloseHandle(handle); }
};

void save_replacement() {
    TestDirectory directory;
    const auto path = directory.path / "world.save";
    Fixture f;
    const auto id = f.reserve();
    save_world(f.world, path);
    const auto good = serialize_world(load_world(path));
    require(f.world.begin(id) == Transition::Changed, "begin failed");
    {
        const auto raw = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(raw != INVALID_HANDLE_VALUE,
                "cannot lock existing snapshot for replacement failure test");
        const std::unique_ptr<void, CloseFile> lock{raw};
        require_throws<std::system_error>([&] { save_world(f.world, path); });
    }
    require(serialize_world(load_world(path)) == good,
            "failed replacement destroyed last good save");
    std::size_t files = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory.path)) {
        require(entry.path() == path, "failed save leaked a temporary file");
        ++files;
    }
    require(files == 1, "last valid snapshot disappeared");
    save_world(f.world, path);
    require(serialize_world(load_world(path)) == serialize_world(f.world),
            "successful replacement lost state");
    require_throws([&] { save_world(f.world, directory.path / "missing" / "world.save"); });
    require_throws([&] { return load_world(directory.path / "absent.save"); });
    const auto before = serialize_world(f.world);
    {
        std::ofstream output{path, std::ios::binary | std::ios::trunc};
        output << "damaged";
    }
    require_throws([&] { f.world = load_world(path); });
    require(serialize_world(f.world) == before, "failed load replaced existing world");
}

void capacity_limits() {
    EntityRegistry registry;
    for (std::size_t index = 0; index < record_limit; ++index) {
        require(registry.create(EntityKind::Actor).value == index + 1, "id sequence skipped");
    }
    const auto next = registry.next_id();
    require_throws<std::length_error>([&] { return registry.create(EntityKind::Actor); });
    require(registry.next_id() == next, "registry limit spent an id");

    Fixture f;
    for (std::size_t index = 0; index < record_limit; ++index) {
        require(f.world.reserve(f.request(index + 1, 0, 0)).status == RequestStatus::InvalidRequest,
                "invalid command was accepted");
    }
    const auto before = serialize_world(f.world);
    require_throws<std::length_error>([&] { return f.world.reserve(f.request(record_limit + 1)); });
    require(serialize_world(f.world) == before,
            "command capacity failure left partial reservation");
    require(f.world.reserve(f.request(1, 0, 0)).status == RequestStatus::InvalidRequest,
            "full ledger could not replay an existing command");
}

void event_capacity() {
    Fixture f;
    const auto id = f.reserve();
    require(f.world.begin(id) == Transition::Changed, "begin failed");
    while (f.world.events().size() < record_limit - 1) {
        const auto empty_lot = f.world.create_resource(f.first, Quantity{0});
        require(empty_lot.value != 0, "empty resource not created");
    }
    const auto before = serialize_world(f.world);
    require_throws<std::length_error>([&] { f.world.advance(5); });
    require(serialize_world(f.world) == before, "journal capacity failure partly completed action");
    balance(f.world, f.lot, 8, 4, 4);
    require(f.world.cancel(id) == Transition::Changed,
            "last event slot could not hold cancellation");
    balance(f.world, f.lot, 8, 0, 8);
    const auto full = serialize_world(f.world);
    require_throws<std::length_error>([&] { return f.world.create_actor(); });
    require(serialize_world(f.world) == full, "full journal allowed partial actor creation");
}

void batch_ordering() {
    Fixture f;
    const auto later = f.world.reserve(f.request(1, 3, 1, 7)).action;
    const auto earlier = f.world.reserve(f.request(2, 3, 0, 3)).action;
    const auto tied = f.world.reserve(f.request(3, 3, 0, 3)).action;
    require(f.world.begin(later) == Transition::Changed, "begin failed");
    require(f.world.begin(tied) == Transition::Changed, "begin failed");
    require(f.world.begin(earlier) == Transition::Changed, "begin failed");
    auto single = deserialize_world(serialize_world(f.world));
    f.world.advance(10);
    for (int tick = 0; tick < 10; ++tick) {
        single.advance(1);
    }
    require(serialize_world(single) == serialize_world(f.world),
            "batch advance changed event order or outcome");
    std::vector<EntityId> completed;
    for (const auto& event : f.world.events()) {
        if (event.kind == EventKind::Completed) {
            completed.push_back(event.subject);
        }
    }
    require(completed == std::vector<EntityId>{earlier, tied, later},
            "deadline/identity tie order incorrect");
    balance(f.world, f.lot, 1, 0, 1);
}

void operation_sequence() {
    // Seed 0x4d3031. Failures include the operation index for reproduction.
    Fixture f;
    std::uint64_t seed = 0x4d3031;
    std::uint64_t expected_quantity = 10;
    for (std::uint64_t index = 0; index < 100; ++index) {
        try {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            if (expected_quantity == 0) {
                f.lot = f.world.create_resource(f.first, Quantity{10});
                expected_quantity = 10;
            }
            const auto required = std::min(expected_quantity, (seed % 3) + 1);
            const auto upfront = (seed >> 4) % (required + 1);
            const auto request = f.request(index + 1, required, upfront, 3);
            const auto result = f.world.reserve(request);
            require(result.status == RequestStatus::Accepted, "sequence reservation denied");
            require(f.world.begin(result.action) == Transition::Changed, "begin failed");
            expected_quantity -= upfront;
            f.world.advance(1);
            if ((seed & 1) == 0) {
                require(f.world.cancel(result.action) == Transition::Changed, "cancel failed");
            } else {
                f.world.advance(2);
                expected_quantity -= required - upfront;
            }
            balance(f.world, f.lot, expected_quantity, 0, expected_quantity);
            const auto before = serialize_world(f.world);
            f.world = deserialize_world(before);
            require(f.world.reserve(request) == result && serialize_world(f.world) == before,
                    "sequence replay after load changed effects");
        } catch (const std::exception& error) {
            throw std::runtime_error("seed=0x4d3031 operation=" + std::to_string(index) + ": " +
                                     error.what());
        }
    }
}

struct TestCase {
    std::string_view name;
    void (*run)();
};
constexpr std::array cases{
    TestCase{"stale_handle", stale_handle},
    TestCase{"registry_exhaustion", registry_exhaustion},
    TestCase{"registry_validation", registry_validation},
    TestCase{"reservation_contention", reservation_contention},
    TestCase{"command_retries", command_retries},
    TestCase{"cancel_before_start", cancel_before_start},
    TestCase{"partial_cancellation", partial_cancellation},
    TestCase{"completion_once", completion_once},
    TestCase{"participant_removal", participant_removal},
    TestCase{"invalid_requests", invalid_requests},
    TestCase{"time_overflow", time_overflow},
    TestCase{"snapshot_continuation", snapshot_continuation},
    TestCase{"snapshot_history", snapshot_history},
    TestCase{"snapshot_rejection", snapshot_rejection},
    TestCase{"snapshot_semantics", snapshot_semantics},
    TestCase{"snapshot_journal", snapshot_journal},
    TestCase{"capacity_lifecycle", capacity_lifecycle},
    TestCase{"capacity_removal", capacity_removal},
    TestCase{"capacity_snapshot", capacity_snapshot},
    TestCase{"save_replacement", save_replacement},
    TestCase{"record_limit", capacity_limits},
    TestCase{"event_capacity", event_capacity},
    TestCase{"batch_ordering", batch_ordering},
    TestCase{"operation_sequence", operation_sequence},
};

bool run_case(const TestCase& test) {
    try {
        test.run();
        std::cout << "PASS " << test.name << '\n';
        return true;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
    } catch (...) {
        std::cerr << "FAIL " << test.name << ": unknown exception\n";
    }
    return false;
}

void intentional_failure() { require(false, "INTENTIONAL WORK TEST FAILURE"); }

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view{argv[1]} == "--self-test-failure") {
        return run_case(TestCase{"intentional_failure", intentional_failure}) ? 0 : 1;
    }
    if (argc == 3 && std::string_view{argv[1]} == "--case") {
        for (const auto& test : cases) {
            if (test.name == argv[2]) {
                return run_case(test) ? 0 : 1;
            }
        }
        std::cerr << "ERROR: unknown test case\n";
        return 2;
    }
    if (argc != 1) {
        std::cerr << "Usage: reality_work_tests [--case NAME]\n";
        return 2;
    }
    int failures = 0;
    for (const auto& test : cases) {
        failures += run_case(test) ? 0 : 1;
    }
    return failures == 0 ? 0 : 1;
}
