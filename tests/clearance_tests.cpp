#include "lab/traversal_session.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace reality;
namespace {
void require(bool c, const char* m) {
    if (!c)
        throw std::runtime_error(m);
}
void step(TraversalSession& s, unsigned n) {
    for (unsigned t = 0; t < n; ++t) {
        s.advance(1);
        s.validate();
    }
}
TraversalScene blockade(bool willing = true) {
    TraversalScene s;
    s.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                         {2, {2, .2, 0}, 0, 2, 2, .3, 0, 0, true}};
    s.connections = {{{1, {1.7, 0}}, {2, {-1.7, 0}}}};
    s.spawns = {{{1, {1, 0}}, {}, false}, {{2, {-1.3, 0}}, {}, willing}};
    return s;
}
void step_blockade() {
    TraversalSession s{blockade()};
    s.set_goal(0, {2, {1, 0}});
    step(s, 300);
    if (s.actors()[0].state != TraversalState::Arrived)
        std::cerr << s.inspect();
    require(s.actors()[0].state == TraversalState::Arrived,
            "blocked Step did not finish through voluntary local clearance");
}
void retained() {
    TraversalSession s;
    s.load(REALITY_CLEARANCE_FIXTURE);
    bool completed = false;
    for (unsigned tick = 0; tick < 600 && !completed; ++tick) {
        step(s, 1);
        completed = s.actors()[5].support.surface == 4;
    }
    if (!completed)
        std::cerr << s.inspect();
    require(completed, "retained blocked Step never physically reached its landing");
}
void controlled() {
    TraversalSession s{blockade(false)};
    const auto feet = s.actors()[1].feet;
    s.set_goal(0, {2, {1, 0}});
    step(s, 200);
    require(length(s.actors()[1].feet - feet) < 1e-9 && !s.actors()[1].goal,
            "clearance commandeered a manually controlled actor");
    require(s.actors()[0].clearance.outcome == ClearanceOutcome::Controlled,
            "controlled recipient did not give an explicit refusal");
}
void await_yield(TraversalSession& s) {
    s.set_goal(0, {2, {1, 0}});
    for (unsigned t = 0; t < 180 && !s.actors()[1].yielding; ++t)
        step(s, 1);
    require(s.actors()[1].yielding.has_value(), "fixture did not enter a clearance maneuver");
}
void persistence() {
    TraversalSession s{blockade()};
    await_yield(s);
    const auto bytes = s.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 3\n"), "clearance state used an older format");
    const auto at = s.actors()[0].clearance.at;
    const auto plans = s.actors()[1].plans;
    require(s.request_clearance(0) == ClearanceOutcome::Accepted && s.serialize() == bytes &&
                s.actors()[1].plans == plans && s.actors()[0].clearance.at == at,
            "duplicate request changed the accepted maneuver");
    auto clone = s;
    clone.restore(bytes);
    step(s, 250);
    step(clone, 250);
    require(s.serialize() == clone.serialize() && s.actors()[0].state == TraversalState::Arrived,
            "clearance save did not preserve actual passage and deterministic continuation");
    require(!s.actors()[1].yielding &&
                s.actors()[0].clearance.outcome == ClearanceOutcome::Completed,
            "successful passage did not close the maneuver");
}
void no_space() {
    auto scene = blockade();
    scene.geometry.bodies[1].origin.x = .65;
    scene.geometry.bodies[1].half_x = .45;
    scene.geometry.bodies[1].half_z = .3;
    scene.connections[0].to = {2, {-.35, 0}};
    scene.spawns[1].location = {2, {.05, 0}};
    TraversalSession s{scene};
    const auto feet = s.actors()[1].feet;
    s.set_goal(0, {2, {0, 0}});
    step(s, 180);
    require(s.actors()[0].clearance.outcome == ClearanceOutcome::NoSpace &&
                !s.actors()[1].yielding && length(s.actors()[1].feet - feet) < 1e-9,
            "recipient without a supported clearance spot did not refuse");
}
void busy() {
    auto scene = blockade();
    scene.stations = {{{2, {-1.3, 0}}, TraversalTask::Maintain, 10}};
    TraversalSession s{scene};
    s.set_goal(0, {2, {1, 0}});
    for (unsigned t = 0; t < 70 && s.actors()[0].step_point == 0; ++t)
        step(s, 1);
    require(s.actors()[0].step_point > 0 && s.actors()[1].action.value != 0,
            "fixture missed a committed Step and running action");
    const auto action = s.actors()[1].action;
    require(s.request_clearance(0) == ClearanceOutcome::Busy &&
                s.world().action(action)->state == ActionState::Running && !s.actors()[1].yielding,
            "clearance cancelled a material-consuming action");
}
void manual_override() {
    TraversalSession s{blockade()};
    await_yield(s);
    s.set_goal(1, {2, {0, -1}});
    require(!s.actors()[1].yielding && !s.actors()[1].autonomous &&
                s.actors()[0].clearance.outcome == ClearanceOutcome::Cancelled,
            "manual command did not cancel voluntary clearance");
    step(s, 150);
    require(s.actors()[1].requested_goal == SurfaceLocation{2, {0, -1}},
            "expired clearance overwrote a later manual command");
}
void same_intent_override() {
    TraversalSession s;
    s.load(REALITY_CLEARANCE_FIXTURE);
    for (unsigned t = 0; t < 100; ++t) {
        step(s, 1);
        for (std::size_t i = 0; i < s.actors().size(); ++i) {
            const auto a = s.actors()[i];
            if (!a.yielding || !a.requested_goal || a.requested_reach <= 0)
                continue;
            const auto center = *a.requested_goal;
            const auto reach = a.requested_reach;
            s.approach(i, center, reach);
            s.validate();
            require(!s.actors()[i].yielding && !s.actors()[i].autonomous,
                    "same-intent override did not take control");
            if (s.actors()[i].goal) {
                const auto endpoint =
                    surface_feet(s.geometry(), *s.actors()[i].goal, TraversalLimits{a.size});
                const auto target = surface_feet(s.geometry(), center, TraversalLimits{a.size});
                require(endpoint && target && length(*endpoint - *target) <= reach + 1e-8,
                        "same-intent takeover retained the temporary clearance endpoint");
            }
            return;
        }
    }
    throw std::runtime_error("retained fixture never offered a same-intent takeover");
}
std::string changed(std::string bytes, std::string_view prefix, std::size_t field,
                    const std::string& value) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto start = bytes.find(prefix);
    require(start != std::string::npos, "mutation record missing");
    const auto first = start + 1, last = bytes.find('\n', first);
    std::istringstream in{bytes.substr(first, last - first)};
    std::ostringstream out;
    std::string word;
    std::size_t n = 0;
    while (in >> word) {
        if (n)
            out << ' ';
        out << (n++ == field ? value : word);
    }
    require(n > field, "mutation field missing");
    bytes.replace(first, last - first, out.str());
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n';
}
void invalid_restore() {
    TraversalSession s{blockade()};
    await_yield(s);
    const auto bytes = s.serialize();
    const auto at = std::to_string(s.actors()[0].clearance.at + 1);
    const std::vector<std::string> bad{
        changed(bytes, "\nCLEARANCE ", 1, "999"), changed(bytes, "\nCLEARANCE ", 1, "1"),
        changed(bytes, "\nCLEARANCE ", 2, "2"),   changed(bytes, "\nCLEARANCE ", 3, at),
        changed(bytes, "\nYIELD 1", 2, "999"),    changed(bytes, "\nYIELD 1", 4, "0")};
    for (const auto& value : bad) {
        bool rejected = false;
        try {
            s.restore(value);
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected && s.serialize() == bytes,
                "unpaired or corrupt clearance replaced the current world");
    }
}
void stale_step_restore() {
    TraversalSession s{blockade()};
    await_yield(s);
    const auto before = s.serialize();
    const auto last = s.actors()[0].route.legs.size() - 1;
    require(s.actors()[0].route.legs[last].kind == SurfaceLegKind::Walk,
            "fixture lacks final walk");
    auto bytes = changed(before, "\nACTOR ", 2, "2");
    bytes = changed(bytes, "\nACTOR ", 3, "1");
    bytes = changed(bytes, "\nACTOR ", 4, "0");
    bytes = changed(bytes, "\nACTOR ", 5, "3");
    bytes = changed(bytes, "\nACTOR ", 6, "0.2");
    bytes = changed(bytes, "\nACTOR ", 7, "0");
    bytes = changed(bytes, "\nACTOR ", 8, "0");
    bytes = changed(bytes, "\nACTOR ", 9, "0");
    bytes = changed(bytes, "\nACTOR ", 14, "1");
    bytes = changed(bytes, "\nACTOR ", 28, std::to_string(last));
    bytes = changed(bytes, "\nACTOR ", 29, "0");
    bool rejected = false;
    try {
        s.restore(bytes);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before,
            "stale completed step retained an active paired agreement");
}
void reserved_targets() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-2, 0, 1.55}, 0, 2, 5, .3, 0, 0, true},
                             {2, {.7, .2, 1.55}, 0, .05, 1.55, .3, 0, 0, true}};
    scene.connections = {{{1, {1.55, -1.65}}, {2, {0, -1.25}}},
                         {{1, {1.55, 1.65}}, {2, {0, 1.25}}}};
    scene.spawns = {{{1, {1.3, -1.65}}, {.65, 1.8}, false},
                    {{2, {0, -1.55}}, {.5, 1.8}, true},
                    {{1, {1.3, 1.65}}, {.65, 1.8}, false},
                    {{2, {0, 1.55}}, {.5, 1.8}, true}};
    TraversalSession s{scene};
    s.set_goal(0, {2, {0, -1.55}});
    s.set_goal(2, {2, {0, 1.55}});
    for (unsigned t = 0; t < 100 && !(s.actors()[0].step_point > 0 && s.actors()[2].step_point > 0);
         ++t)
        step(s, 1);
    if (!(s.actors()[0].step_point > 0 && s.actors()[2].step_point > 0))
        std::cerr << s.inspect();
    require(s.actors()[0].step_point > 0 && s.actors()[2].step_point > 0,
            "parallel Step fixture did not start");
    const auto first = s.request_clearance(0);
    require(first == ClearanceOutcome::Accepted && s.actors()[1].yielding,
            "first parallel request did not reserve a maneuver");
    const auto second = s.request_clearance(2);
    if (second == ClearanceOutcome::Accepted) {
        require(s.actors()[3].yielding.has_value(), "second agreement lacks a maneuver");
        const auto a =
            surface_feet(s.geometry(), *s.actors()[1].goal, TraversalLimits{s.actors()[1].size});
        const auto b =
            surface_feet(s.geometry(), *s.actors()[3].goal, TraversalLimits{s.actors()[3].size});
        require(a && b &&
                    capsule_paths_clear(*a, *a, s.actors()[1].size, *b, *b, s.actors()[3].size),
                "parallel requests reserved overlapping clearance destinations");
    } else
        require(second == ClearanceOutcome::NoSpace, "unexpected parallel refusal");
}
void expiry() {
    TraversalSession s{blockade(false)};
    s.set_goal(0, {2, {1, 0}});
    step(s, 220);
    s.set_autonomous(1, true);
    for (unsigned t = 0; t < 100 && !s.actors()[1].yielding; ++t)
        step(s, 1);
    require(s.actors()[1].yielding.has_value(), "expiry fixture did not receive consent");
    const auto now = s.world().clock().tick_count();
    require(now >= 200, "expiry fixture lacks clock history");
    // Restore a coherent maneuver at the last tick of its fixed 200-tick lifetime.
    auto bytes = changed(s.serialize(), "\nCLEARANCE ", 3, std::to_string(now - 199));
    bytes = changed(bytes, "\nYIELD 1", 3, std::to_string(now - 199));
    bytes = changed(bytes, "\nYIELD 1", 4, std::to_string(now + 1));
    s.restore(bytes);
    step(s, 2);
    require(!s.actors()[1].yielding &&
                s.actors()[0].clearance.outcome == ClearanceOutcome::Expired &&
                s.actors()[0].step_point > 0,
            "expired agreement renewed itself or fabricated step completion");
}
void intent_preserved() {
    TraversalSession s;
    s.load(REALITY_CLEARANCE_FIXTURE);
    for (unsigned t = 0; t < 100; ++t) {
        const auto before = s.actors();
        step(s, 1);
        for (std::size_t i = 0; i < s.actors().size(); ++i) {
            const auto& a = s.actors()[i];
            if (!a.yielding)
                continue;
            require(a.yielding->original_goal == before[i].goal &&
                        a.requested_goal == before[i].requested_goal &&
                        a.requested_reach == before[i].requested_reach &&
                        a.task == before[i].task && a.station == before[i].station &&
                        a.suspended.size() == before[i].suspended.size(),
                    "consent discarded the recipient's prior work or intention");
            for (unsigned n = 0; n < 200 && s.actors()[i].yielding; ++n) {
                require(s.actors()[i].action.value == 0,
                        "yielding NPC worked remotely while making room");
                step(s, 1);
            }
            require(!s.actors()[i].yielding &&
                        s.actors()[i].requested_goal == before[i].requested_goal,
                    "completed maneuver failed to return to its own intention");
            return;
        }
    }
    throw std::runtime_error("intent fixture never accepted clearance");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string_view{argv[1]} == "--capture-retained") {
            require(!std::filesystem::exists(argv[2]), "capture path already exists");
            TraversalSession s;
            s.load(REALITY_CLEARANCE_FIXTURE);
            for (unsigned t = 0; t < 150; ++t) {
                step(s, 1);
                if (std::any_of(s.actors().begin(), s.actors().end(),
                                [](const auto& a) { return a.yielding.has_value(); })) {
                    s.save(argv[2]);
                    std::cout << s.inspect();
                    return 0;
                }
            }
            throw std::runtime_error("no retained clearance to capture");
        }
        require(argc == 2, "expected clearance test");
        const std::string_view n{argv[1]};
        if (n == "stale_step_restore")
            stale_step_restore();
        else if (n == "expiry")
            expiry();
        else if (n == "intent_preserved")
            intent_preserved();
        else if (n == "invalid_restore")
            invalid_restore();
        else if (n == "reserved_targets")
            reserved_targets();
        else if (n == "persistence")
            persistence();
        else if (n == "no_space")
            no_space();
        else if (n == "busy")
            busy();
        else if (n == "manual_override")
            manual_override();
        else if (n == "same_intent_override")
            same_intent_override();
        else if (n == "step_blockade")
            step_blockade();
        else if (n == "retained")
            retained();
        else if (n == "controlled")
            controlled();
        else
            throw std::invalid_argument("unknown clearance test");
        std::cout << "PASS " << n << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
