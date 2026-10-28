#include "lab/traversal_session.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reality;
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
void balance(const TraversalSession& s) {
    std::uint64_t food = 0, parts = 0;
    const auto count = [&](EntityId lot) {
        const auto resource = *s.world().resource(lot);
        if (resource.record.material == MaterialTypeId{1})
            food += resource.record.quantity.value;
        if (resource.record.material == MaterialTypeId{2})
            parts += resource.record.quantity.value;
    };
    for (const auto& station : s.stations())
        count(station.lot);
    for (const auto& actor : s.actors())
        if (actor.cargo.value)
            count(actor.cargo);
    for (const auto& c : s.world().commands())
        if (c.result.status == RequestStatus::Accepted) {
            const auto a = *s.world().action(c.result.action);
            const auto r = *s.world().resource(a.request.lot);
            if (r.record.material == MaterialTypeId{1})
                food += a.consumed.value;
            if (r.record.material == MaterialTypeId{2})
                parts += a.consumed.value;
        }
    require(food == 20 && parts == 24, "material balance changed during the journey");
}
void step(TraversalSession& s) {
    s.advance(1);
    s.validate();
    balance(s);
}
bool delivered(const TraversalSession& s) {
    return s.actors()[1].delivery->delivered == 8 && s.stations()[1].completed > 0;
}
struct Trace {
    bool bridge = false, ferry = false;
    std::uint64_t first = 0;
    std::optional<unsigned> local_energy;
    void observe(const TraversalSession& s) {
        bridge = bridge || s.actors()[1].support.surface == s.passages()[0].surface;
        ferry = ferry || s.actors()[1].transfer == TransferStage::Riding;
        if (!first && s.actors()[1].delivery->delivered)
            first = s.world().clock().tick_count();
        const auto v = s.practice_view(3);
        if (v.local)
            local_energy = v.energy;
    }
};
Trace finish(TraversalSession& s, bool repair) {
    Trace trace;
    while (s.world().clock().tick_count() < 1200 && !delivered(s)) {
        if (repair && s.passages()[0].open && !s.actors()[1].autonomous)
            s.set_autonomous(1, true);
        step(s);
        trace.observe(s);
    }
    if (!delivered(s))
        std::cerr << s.inspect();
    require(delivered(s), "chosen method did not deliver eight units and feed the recipient");
    return trace;
}
void choice() {
    TraversalSession s{make_two_methods_course()};
    require(s.has_practice() && s.actors().size() == 8, "shared community is missing");
    require(!s.passages()[0].open && s.stations()[2].wear == 1000,
            "the initial obstruction does not require work");
    require(!s.actors()[1].passage_beliefs[0], "remote courier knows the initial obstruction");
    require(s.interact_passage(0, 1, true) == PassageOutcome::Worn,
            "player opened a worn bridge without work");
    require(!s.begin_maintenance(0, 2), "repair bypassed the physical calibration");
    for (unsigned i = 0; i < 80; ++i)
        step(s);
    require(!s.passages()[0].open && s.stations()[2].completed == 0 &&
                s.actors()[1].delivery->delivered == 0 && s.practice_view(3).probes == 0,
            "the community decided for the player before either method was selected");
}
void journey(bool repair) {
    TraversalSession s{make_two_methods_course()};
    require(s.practice_view(3).local, "energy observer is not physically at the apparatus");
    const auto energy = s.practice_view(3).energy;
    s.set_autonomous(repair ? 3 : 1, true);
    const auto trace = finish(s, repair);
    if (repair)
        require(trace.bridge && !trace.ferry && s.stations()[2].completed > 0 &&
                    s.practice_view(3).method_known && s.practice_view(3).probes > 0 &&
                    trace.local_energy && *trace.local_energy < energy,
                "repair did not connect learned knowledge, paid work and physical bridge travel");
    else
        require(trace.ferry && !trace.bridge && !s.passages()[0].open &&
                    s.stations()[2].completed == 0 &&
                    s.world().resource(s.stations()[2].lot)->record.quantity.value == 24 &&
                    trace.local_energy && *trace.local_energy == energy,
                "detour invented a repair or did not physically ride the ferry");
    std::cout << "method=" << (repair ? "repair" : "detour") << " first=" << trace.first
              << " finish=" << s.world().clock().tick_count()
              << " delivered=8 meals=" << s.stations()[1].completed
              << " repairs=" << s.stations()[2].completed
              << " parts=" << s.world().resource(s.stations()[2].lot)->record.quantity.value
              << " last_local_energy=" << *trace.local_energy << " bridge=" << trace.bridge
              << " ferry=" << trace.ferry << '\n';
}
void checkpoint(bool repair, const std::filesystem::path& path,
                const std::filesystem::path& expected) {
    const auto start = std::chrono::steady_clock::now();
    const auto timing = [&](const char* stage) {
        std::cout << stage << "="
                  << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
                  << '\n';
    };
    TraversalSession s{make_two_methods_course()};
    timing("constructed");
    s.set_autonomous(repair ? 3 : 1, true);
    bool found = false;
    for (unsigned i = 0; i < 240 && !found; ++i) {
        step(s);
        found = repair ? s.actors()[3].action.value != 0
                       : s.actors()[1].transfer == TransferStage::Riding;
    }
    require(found, "checkpoint did not reach real work or a loaded ferry");
    timing("reached");
    if (repair)
        require(s.actors()[3].task == TraversalTask::Maintain && s.actors()[3].station == 2 &&
                    s.world().action(s.actors()[3].action)->request.lot == s.stations()[2].lot,
                "repair checkpoint is not actual maintenance at the bridge");
    else
        require(s.world().resource(s.actors()[1].cargo)->record.quantity.value > 0,
                "ferry checkpoint has no cargo");
    s.save(path);
    timing("saved");
    static_cast<void>(finish(s, repair));
    timing("finished");
    s.save(expected);
    timing("expected_saved");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc >= 2, "case required");
        const std::string name = argv[1];
        if (argc == 5 && name == "checkpoint")
            checkpoint(std::string(argv[2]) == "repair", argv[3], argv[4]);
        else if (argc == 5 && name == "continue") {
            TraversalSession s;
            s.load(argv[3]);
            static_cast<void>(finish(s, std::string(argv[2]) == "repair"));
            s.save(argv[4]);
        } else if (name == "choice")
            choice();
        else if (name == "repair" || name == "detour")
            journey(name == "repair");
        else
            throw std::runtime_error("unknown case");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
