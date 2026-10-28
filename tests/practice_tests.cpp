#include "lab/traversal_session.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#if __has_include("lab/traversal_practice.hpp")
#include "lab/traversal_practice.hpp"
using namespace reality;
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
TraversalScene fixture(unsigned stock = 4, bool remote = false) {
    TraversalScene s;
    s.geometry.bodies = {{1, {}, 0, 10, 6, .3, 0, 0, true}};
    s.stations = {{{1, {0, 0}}, TraversalTask::Maintain, stock, {}, 500, 0, {2}}};
    s.spawns = {{{1, {remote ? -5. : -.8, 0}}, {}, false}, {{1, {.8, 0}}, {}, false}};
    s.practice = PracticeSeed{};
    s.practice->station = 0;
    s.practice->researchers = {0, 1};
    return s;
}
void calibration() {
    TraversalSession gated{fixture()};
    require(!gated.begin_maintenance(0, 0), "manual maintenance bypassed calibration");
    require(gated.stations()[0].completed == 0 && !gated.actors()[0].action.value,
            "fictional work");
    TraversalSession s{fixture()};
    const auto lot = s.stations()[0].lot;
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "probe refused");
    s.advance(2);
    require(s.world().resource(lot)->record.quantity.value == 4,
            "failed calibration consumed a part");
    require(s.practice_probe(0, {0, 0}, 16) == DiscoveryOutcome::Accepted,
            "composed probe refused");
    s.advance(2);
    require(s.begin_maintenance(0, 0), "physical calibration did not authorize work");
    require(s.world().resource(lot)->record.quantity.value == 3, "material not consumed by work");
    s.advance(80);
    require(s.stations()[0].completed == 1, "maintenance not completed");
    require(!s.begin_maintenance(0, 0), "calibration reused for a second work");
}
void energy() {
    TraversalSession s{fixture()};
    auto before = s.practice_view(0).energy;
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "energy probe");
    s.advance(1);
    require(s.practice_view(0).energy == before, "energy spent before completed pulse");
    s.advance(1);
    require(s.practice_view(0).energy == before - 16, "lost physical energy");
    require(s.practice_probe(0, {0}, 129) == DiscoveryOutcome::Invalid, "range accepted");
}
void concurrent() {
    TraversalSession s{fixture()};
    require(s.can_interact_at(1, s.stations()[0].location),
            "second fixture operator is out of reach");
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "first operator");
    require(s.practice_probe(1, {0}, 32) == DiscoveryOutcome::Busy, "two simultaneous operators");
    s.advance(2);
    require(s.practice_probe(1, {0}, 32) == DiscoveryOutcome::Accepted, "device never released");
}
void protected_probe() {
    TraversalSession s{fixture()};
    const auto energy = s.practice_view(0).energy;
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "protected begin");
    s.set_goal(0, {1, {-4, 0}});
    s.advance(2);
    require(s.practice_view(0).energy == energy - 16, "deferred command lost the pulse");
    s.advance(40);
    require(s.actors()[0].feet.x < -2, "deferred goal was lost");
}
void queued_input() {
    TraversalSession s{fixture()};
    s.queue_input(0, {1, 0});
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Busy,
            "apparatus accepted an operator with queued movement");
    s.advance(2);
    require(s.practice_view(0).probes == 0, "refused moving operator acquired a measurement");
}
void refusals(bool remote) {
    TraversalSession s{fixture(remote ? 4 : 0, remote)};
    if (remote)
        require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::OutOfReach, "remote observation");
    else {
        require(s.practice_probe(0, {0, 0}, 16) == DiscoveryOutcome::Accepted, "calibration");
        s.advance(2);
        require(!s.begin_maintenance(0, 0) && s.stations()[0].completed == 0, "work without part");
    }
}
} // namespace
#endif
int main(int argc, char** argv) {
    try {
#if __has_include("lab/traversal_practice.hpp")
        std::string name = argc == 2 ? argv[1] : "";
        if (name == "calibration")
            calibration();
        else if (name == "energy")
            energy();
        else if (name == "concurrent")
            concurrent();
        else if (name == "no_parts")
            refusals(false);
        else if (name == "out_of_reach")
            refusals(true);
        else if (name == "protected")
            protected_probe();
        else if (name == "queued_input")
            queued_input();
        else
            throw std::runtime_error("unknown practice case");
        std::cout << "PASS " << name << '\n';
#else
        static_cast<void>(argv);
        if (argc > 0)
            throw std::runtime_error("practice domain integration is missing");
#endif
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
