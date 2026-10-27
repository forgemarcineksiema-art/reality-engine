#include "lab/traversal_session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace reality;
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
void step(TraversalSession& s, unsigned n = 1) {
    for (unsigned i = 0; i < n; ++i) {
        s.advance(1);
        s.validate();
    }
}
TraversalScene small() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {}, 0, 20, 5, .3, 0, 0, true}};
    scene.stations = {{{1, {0, 0}}, TraversalTask::Eat, 1}, {{1, {12, 0}}, TraversalTask::Eat, 10}};
    for (auto& station : scene.stations)
        station.initial_material = {1};
    scene.spawns = {{{1, {0, -.8}}, {}, false}, {{1, {-7, 0}}, {}, true, 300}};
    return scene;
}
std::string actor_inspect(const TraversalSession& s, std::size_t index) {
    auto text = s.inspect();
    const auto start = text.find("\nactor=" + std::to_string(s.actors()[index].id.value) + " ");
    require(start != std::string::npos, "missing inspector actor");
    auto end = text.find("\nactor=", start + 1);
    if (end == std::string::npos)
        end = text.find("\nstation=", start + 1);
    return text.substr(start, end - start);
}
void no_global() {
    TraversalSession s{small()};
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "fixture did not empty station");
    step(s);
    require(s.actors()[1].task == TraversalTask::Eat && s.actors()[1].station == 0,
            "actor peeked at remote empty stock");
    require(actor_inspect(s, 1).find("belief_station=0") == std::string::npos,
            "unobserved actor already knows stock");
    for (unsigned t = 0; t < 150 && s.actors()[1].station_retry[0] == 0; ++t)
        step(s);
    require(s.actors()[1].station_retry[0] > s.world().clock().tick_count(),
            "actor did not physically reach and receive a refusal");
    require(actor_inspect(s, 1).find("belief_station=0 available=0") != std::string::npos,
            "arrival did not record a local empty observation");
}
void witness() {
    auto scene = small();
    scene.stations[0].initial_stock = 0;
    scene.spawns = {{{1, {0, 0}}, {}, true, 300}, {{1, {-1.8, 0}}, {}, true, 300}};
    TraversalSession s{scene};
    step(s, 5);
    require(s.actors()[1].task == TraversalTask::Eat && s.actors()[1].station == 1,
            "recipient still walks to the empty station despite nearby witness");
    require(actor_inspect(s, 1).find("source_actor=" + std::to_string(s.actors()[0].id.value)) !=
                std::string::npos,
            "recipient lost the original witness");
}
// Fixture editor changes only the known-location mask, not world stock or a result.
std::string known_only(std::string bytes, std::size_t actor, unsigned mask) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    auto first = bytes.find("\nACTOR ");
    for (std::size_t i = 0; i < actor; ++i)
        first = bytes.find("\nACTOR ", first + 1);
    ++first;
    const auto end = bytes.find('\n', first);
    std::istringstream in{bytes.substr(first, end - first)};
    std::ostringstream out;
    std::string token;
    unsigned field = 0;
    while (in >> token) {
        if (field)
            out << ' ';
        out << (field++ == 18 ? std::to_string(mask) : token);
    }
    bytes.replace(first, end - first, out.str());
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n';
}
void delivery() {
    auto scene = small();
    scene.stations[0].initial_stock = 4;
    scene.stations[0].location.local = {-5, 0};
    scene.stations[1].initial_stock = 0;
    scene.stations[1].location.local = {0, 0};
    scene.spawns = {{{1, {-5, 0}}, {}, false}, {{1, {1.5, 0}}, {}, true, 300}};
    TraversalSession s{scene};
    s.restore(known_only(s.serialize(), 1, 2));
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "courier did not load");
    for (unsigned t = 0; t < 60 && s.actors()[1].station_retry[1] == 0; ++t)
        step(s);
    const auto old_retry = s.actors()[1].station_retry[1];
    require(old_retry > s.world().clock().tick_count(), "resident never learned empty stock");
    s.set_goal(0, {1, {-.6, 0}});
    for (unsigned t = 0; t < 100 && (s.actors()[0].state != TraversalState::Arrived ||
                                     s.actors()[0].motion.speed > .01);
         ++t)
        step(s);
    require(s.interact_cargo(0, 1) == CargoStatus::Transferred, "courier did not deposit");
    s.set_autonomous(0, true);
    step(s, 3);
    require(s.actors()[1].action.value != 0 || s.actors()[1].goal.has_value(),
            "delivery information did not wake resident before old retry");
    require(s.world().clock().tick_count() < old_retry, "fixture missed old retry deadline");
    for (unsigned t = 0; t < 80 && s.stations()[1].completed == 0; ++t)
        step(s);
    require(s.stations()[1].completed > 0 && s.actors()[1].hunger < 100,
            "delivery knowledge did not lead to an actual meal");
}
void persistence() {
    TraversalSession s{small()};
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "fixture did not observe");
    const auto bytes = s.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 5\n"), "knowledge was not versioned");
    TraversalSession restored;
    restored.restore(bytes);
    require(restored.serialize() == bytes, "knowledge did not round trip");
    for (unsigned t = 0; t < 100; ++t) {
        step(s);
        step(restored);
        require(s.serialize() == restored.serialize(), "continuation differs after knowledge save");
    }
}
void go(TraversalSession& s, std::size_t actor, SurfaceLocation goal) {
    s.set_goal(actor, goal);
    for (unsigned t = 0; t < 150 && (s.actors()[actor].state != TraversalState::Arrived ||
                                     s.actors()[actor].motion.speed > .01);
         ++t)
        step(s);
    require(s.actors()[actor].state == TraversalState::Arrived &&
                s.actors()[actor].motion.speed <= .01,
            "fixture did not physically arrive");
}
void stale() {
    auto scene = small();
    scene.stations[0].initial_stock = 5;
    scene.spawns = {
        {{1, {0, -.8}}, {}, false}, {{1, {-1.5, -.8}}, {}, false, 300}, {{1, {0, .8}}, {}, false}};
    TraversalSession s{scene};
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "witness did not load four");
    require(s.actors()[0].beliefs[0]->available == 1, "witness did not see last free unit");
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared, "initial report not delivered");
    const auto observed = s.actors()[1].beliefs[0]->observed_tick;
    go(s, 1, {1, {-6, -.8}});
    require(s.interact_cargo(2, 0) == CargoStatus::Transferred,
            "competitor did not take last unit");
    require(s.actors()[1].beliefs[0]->available == 1,
            "remote consumption magically changed recipient belief");
    s.set_autonomous(1, true);
    step(s);
    require(s.actors()[1].task == TraversalTask::Eat && s.actors()[1].station == 0,
            "stale positive belief was secretly checked against world stock");
    const auto commands = s.world().commands().size();
    for (unsigned t = 0; t < 150 && s.actors()[1].station_retry[0] == 0; ++t)
        step(s);
    const auto& belief = s.actors()[1].beliefs[0];
    require(belief && belief->available == 0 && belief->source_actor == s.actors()[1].id &&
                belief->via_actor == s.actors()[1].id && belief->observed_tick > observed &&
                belief->learned_tick == belief->observed_tick,
            "physical refusal did not replace stale report with eyewitness observation");
    require(s.world().commands().size() > commands &&
                s.world().commands().back().result.status != RequestStatus::Accepted,
            "correction did not include a real WorkWorld refusal");
}
void contact() {
    for (unsigned mode = 0; mode < 3; ++mode) {
        auto scene = small();
        scene.spawns[1] = {{1, {-1.5, -.8}}, {}, false};
        if (mode == 0)
            scene.geometry.bodies.push_back({2, {-.75, 1, 0}, 0, .05, 4, 2, 0, 0, false});
        if (mode == 1)
            scene.spawns[1].location.local.x_m = -7;
        if (mode == 2) {
            scene.geometry.bodies.push_back({2, {-1.5, .15, -.8}, 0, .4, .4, .1, 0, 0, true});
            scene.spawns[1].location = {2, {0, 0}};
        }
        TraversalSession s{scene};
        require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "fixture did not observe");
        const auto before = s.serialize();
        require(s.share_knowledge(0, 1) ==
                    (mode == 1 ? KnowledgeOutcome::OutOfReach : KnowledgeOutcome::Blocked),
                "knowledge crossed a wall, distance or disconnected support");
        require(s.serialize() == before, "rejected message mutated state");
    }
}
std::string record_field(std::string bytes, std::string_view record, unsigned field,
                         const std::string& value) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto first = bytes.find(record);
    require(first != std::string::npos, "missing mutation record");
    const auto end = bytes.find('\n', first);
    std::istringstream in{bytes.substr(first, end - first)};
    std::ostringstream out;
    std::string token;
    unsigned n = 0;
    while (in >> token) {
        if (n)
            out << ' ';
        out << (n++ == field ? value : token);
    }
    require(n > field, "missing mutation field");
    bytes.replace(first, end - first, out.str());
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n';
}
void corruption() {
    TraversalSession s{small()};
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "fixture did not observe");
    const auto bytes = s.serialize();
    const std::pair<unsigned, std::string> invalid[] = {
        {1, "16"},
        {1, "2"},
        {2, "-1"},
        {3, "1001"},
        {4, "1"},
        {5, "1"},
        {6, "0"},
        {6, "999999"},
        {6, std::to_string(s.stations()[0].lot.value)},
        {7, "0"},
        {7, std::to_string(s.actors()[1].id.value)}};
    for (const auto& [field, value] : invalid) {
        bool rejected = false;
        try {
            s.restore(record_field(bytes, "BELIEF ", field, value));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && s.serialize() == bytes,
                "corrupt belief with valid checksum was not rejected transactionally");
    }
    for (const auto& [field, value] :
         std::vector<std::pair<unsigned, std::string>>{{1, "41"}, {2, "11"}, {3, "17"}}) {
        bool rejected = false;
        try {
            s.restore(record_field(bytes, "KNOWLEDGE ", field, value));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && s.serialize() == bytes, "invalid sharing metadata was accepted");
    }
}
void relay() {
    auto scene = small();
    scene.stations[0].initial_stock = 0;
    scene.spawns = {
        {{1, {0, 0}}, {}, false}, {{1, {-1.5, 0}}, {}, false}, {{1, {-3, 0}}, {}, false}};
    TraversalSession s{scene};
    require(s.interact_cargo(0, 0) == CargoStatus::Empty, "witness did not see shortage");
    s.restore(known_only(s.serialize(), 1, 0));
    step(s, 3);
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared, "first report not received");
    require((s.actors()[1].known_stations & 1) != 0, "report did not teach unknown location");
    step(s, 4);
    require(s.share_knowledge(1, 2) == KnowledgeOutcome::Shared, "relay not received");
    const auto& b = *s.actors()[2].beliefs[0];
    require(b.observed_tick == 0 && b.learned_tick == 7 && b.source_actor == s.actors()[0].id &&
                b.via_actor == s.actors()[1].id && s.actors()[2].station_retry[0] == 100,
            "relay changed witness, observation time or uncertainty horizon");
    const auto before = s.serialize();
    require(s.share_knowledge(1, 2) == KnowledgeOutcome::NoNews && s.serialize() == before,
            "equal-tick message refreshed or replaced knowledge");
    step(s, 5);
    require(s.interact_cargo(0, 0) == CargoStatus::Empty, "new observation failed");
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared, "newer report ignored");
    require(s.share_knowledge(2, 1) == KnowledgeOutcome::NoNews,
            "older report overwrote a newer observation");
}
void scout() {
    auto scene = small();
    scene.stations.resize(1);
    scene.stations[0].initial_stock = 0;
    scene.spawns = {{{1, {0, 0}}, {}, true, 300}};
    TraversalSession s{scene};
    step(s, 3);
    const auto observed = s.actors()[0].beliefs[0]->observed_tick;
    const auto commands = s.world().commands().size();
    step(s, 90);
    require(s.world().commands().size() == commands, "empty station was retried before horizon");
    for (unsigned t = 0; t < 80 && s.world().commands().size() == commands; ++t)
        step(s);
    require(s.world().commands().size() > commands &&
                s.actors()[0].beliefs[0]->observed_tick >= observed + 100 &&
                s.actors()[0].beliefs[0]->source_actor == s.actors()[0].id,
            "no-message uncertainty horizon never led to real local scouting");
}
void safe_action() {
    auto scene = small();
    scene.stations = {{{1, {0, 0}}, TraversalTask::Rest, 1}};
    scene.spawns = {{{1, {0, -.8}}, {}, false}, {{1, {0, .8}}, {}, true, 0, 200}};
    TraversalSession s{scene};
    for (unsigned t = 0; t < 50 && s.actors()[1].action.value == 0; ++t)
        step(s);
    const auto action = s.actors()[1].action;
    require(action.value != 0, "fixture did not start rest");
    require(s.interact_cargo(0, 0) == CargoStatus::Incompatible,
            "witness did not observe occupied capacity");
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared &&
                s.actors()[1].beliefs[0]->available == 0,
            "capacity report not received");
    step(s);
    require(s.actors()[1].action == action && s.actors()[1].task == TraversalTask::Rest,
            "empty report cancelled an action in progress");
    step(s, 65);
    require(s.stations()[0].completed == 1 && s.actors()[1].energy == 1000 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 1 &&
                s.actors()[1].beliefs[0]->available == 1 &&
                s.actors()[1].beliefs[0]->source_actor == s.actors()[1].id,
            "protected rest failed to release capacity and observe completion exactly once");
}
void bounded_auto() {
    auto scene = small();
    scene.stations.resize(1);
    scene.stations[0].initial_stock = 0;
    scene.spawns = {
        {{1, {0, 0}}, {}, false}, {{1, {-3, 0}}, {}, false}, {{1, {-4.5, 0}}, {}, true}};
    TraversalSession s{scene};
    require(s.interact_cargo(0, 0) == CargoStatus::Empty, "fixture did not observe empty");
    s.set_autonomous(0, true);
    step(s, 45);
    go(s, 1, {1, {-1.5, 0}});
    step(s, 2);
    require(!s.actors()[1].beliefs[0], "automatic sharing continued past its 40-tick window");
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared, "explicit old report was denied");
    go(s, 1, {1, {-3, 0}});
    s.set_autonomous(1, true);
    // Recipient 2 stays out of range of witness 0. A received fact must not start
    // another automatic broadcast window in the intermediary.
    step(s, 5);
    require(!s.actors()[2].beliefs[0] && s.actors()[1].share_until == 0,
            "received report triggered telepathic automatic relaying");
}
void autonomous_delivery() {
    auto scene = small();
    scene.stations[0].location.local = {-5, 0};
    scene.stations[0].initial_stock = 4;
    scene.stations[1].location.local = {0, 0};
    scene.stations[1].initial_stock = 0;
    scene.spawns = {{{1, {-5, 0}}, {}, true}, {{1, {1.5, 0}}, {}, true, 300}};
    scene.initial_deliveries = {{0, 0, 1, 4}};
    TraversalSession s{scene};
    s.restore(known_only(s.serialize(), 1, 2));
    for (unsigned t = 0; t < 60 && s.actors()[1].station_retry[1] == 0; ++t)
        step(s);
    const auto old_retry = s.actors()[1].station_retry[1];
    bool heard_delivery = false;
    auto clone = s;
    clone.restore(s.serialize());
    for (unsigned t = 0; t < 150 && s.stations()[1].completed == 0; ++t) {
        step(s);
        step(clone);
        require(s.serialize() == clone.serialize(),
                "autonomous information flow diverged after save");
        const auto& b = s.actors()[1].beliefs[1];
        if (b && b->source_actor == s.actors()[0].id && b->available > 0) {
            heard_delivery = true;
            require(s.world().clock().tick_count() < old_retry,
                    "autonomous delivery report arrived after old retry");
        }
    }
    require(heard_delivery && s.stations()[1].completed == 1 &&
                s.actors()[0].delivery->delivered == 4 && s.actors()[1].hunger < 100,
            "unattended delivery/report did not lead to real consumption");
}
void legacy4() {
    auto scene = small();
    scene.spawns[0].autonomous = true;
    scene.initial_deliveries = {{0, 0, 1, 1}};
    TraversalSession s{scene};
    const auto bytes = s.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 4\n"), "unobserved cargo changed legacy encoding");
    TraversalSession clone;
    clone.restore(bytes);
    require(clone.serialize() == bytes && !clone.actors()[0].beliefs[0],
            "legacy v4 read fabricated station knowledge");
    step(s, 100);
    step(clone, 100);
    require(s.serialize() == clone.serialize(),
            "legacy v4 continuation diverged with observations");
}
void safe_clearance() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {2, .2, 0}, 0, 2, 2, .3, 0, 0, true}};
    scene.connections = {{{1, {1.7, 0}}, {2, {-1.7, 0}}}};
    scene.spawns = {
        {{1, {1, 0}}, {}, false}, {{2, {-1.3, 0}}, {}, true}, {{2, {-1.3, 1}}, {}, false}};
    scene.stations = {{{2, {-1.3, 1}}, TraversalTask::Maintain, 0}};
    scene.stations[0].initial_material = {1};
    TraversalSession s{scene};
    s.restore(known_only(s.serialize(), 1, 0));
    s.set_goal(0, {2, {1, 0}});
    for (unsigned t = 0; t < 200 && !s.actors()[1].yielding; ++t)
        step(s);
    require(s.actors()[1].yielding.has_value(), "fixture did not start clearance");
    require(s.interact_cargo(2, 0) == CargoStatus::Empty &&
                s.share_knowledge(2, 1) == KnowledgeOutcome::Shared,
            "clearance recipient did not receive local report");
    const auto requested = s.actors()[1].yielding->requested_at;
    step(s);
    require(s.actors()[1].yielding && s.actors()[1].yielding->requested_at == requested,
            "knowledge interrupted voluntary clearance maneuver");
    step(s, 150);
    require(s.actors()[0].state == TraversalState::Arrived && !s.actors()[1].yielding &&
                s.actors()[0].clearance.outcome == ClearanceOutcome::Completed,
            "report prevented physical completion of agreed clearance");
}
bool in_step(const TraversalActor& a) {
    return a.leg < a.route.legs.size() && a.route.legs[a.leg].kind == SurfaceLegKind::Step &&
           a.step_point > 0;
}
void safe_motion(int phase, bool aged = false) {
    TraversalScene scene;
    if (phase < 0) {
        scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                                 {2, {20, .2, 0}, 0, 20, 3, .3, 0, 0, true}};
        scene.connections = {{{1, {1.7, 0}}, {2, {-19.7, 0}}}};
        scene.spawns = {{{1, {1.2, 1}}, {}, false}, {{1, {.5, 0}}, {}, true, 300}};
    } else {
        scene.geometry.bodies = {{1, {-3, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                                 {2, {20, 0, 0}, 0, 17, 3, .3, 0, 0, true},
                                 {3, {}, 0, 1, 1, .25, 0, 0, true}};
        scene.platforms = {{{3, {2, 0, 0}, 0, 20, 10, 0}, 0, false}};
        scene.connections = {{{1, {1.8, 0}}, {2, {-16.8, 0}}, SurfaceLegKind::Transport, 0, 1, 4}};
        scene.services = {
            {1, 3, {{{1, {1.8, 0}}, {2, {-16.8, 0}}}}, {{{-.8, 0}, {.8, 0}}}, {}, {}}};
        scene.spawns = {{{1, {1.2, 1}}, {}, false}, {{1, {0, 0}}, {}, true, 300}};
        if (phase != static_cast<int>(TransferStage::Boarding) || !aged)
            scene.spawns[0].location = {3, {0, .65}};
    }
    scene.stations = {{scene.spawns[0].location, TraversalTask::Maintain, 0},
                      {{2, {10, 0}}, TraversalTask::Eat, 5}};
    scene.stations[0].initial_material = {1};
    TraversalSession s{scene};
    s.restore(known_only(s.serialize(), 1, 2));
    const auto protected_phase = [&] {
        return phase < 0 ? in_step(s.actors()[1])
                         : s.actors()[1].transfer == static_cast<TransferStage>(phase) &&
                               (phase != static_cast<int>(TransferStage::Boarding) || aged ||
                                s.actors()[1].support.surface == 3);
    };
    for (unsigned t = 0; t < 350 && !protected_phase(); ++t)
        step(s);
    require(protected_phase(), "fixture never reached protected motion");
    require(s.interact_cargo(0, 0) == CargoStatus::Empty, "fixture source is not accessible");
    // Only this safety fixture seeds a historical belief about the destination.
    // Real acquisition, stock truth and contact are tested independently above.
    // No position, route, resource or protected-operation state is edited.
    s.restore(record_field(s.serialize(), "BELIEF ", 1, "1"));
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared,
            "protected actor did not physically receive report");
    const auto observed = s.actors()[1].beliefs[1]->observed_tick;
    const auto from = s.actors()[1].feet;
    bool progressed = false;
    for (unsigned t = 0; t < 150; ++t) {
        const auto unsafe = phase < 0 ? in_step(s.actors()[1])
                                      : s.actors()[1].transfer == TransferStage::Boarding ||
                                            s.actors()[1].transfer == TransferStage::Riding ||
                                            s.actors()[1].transfer == TransferStage::Exiting;
        step(s);
        if (unsafe)
            require(s.actors()[1].task == TraversalTask::Eat && s.actors()[1].station == 1,
                    "received empty report cancelled protected motion");
        progressed = progressed || length(s.actors()[1].feet - from) > .1;
        const auto finished =
            phase < 0 ? !in_step(s.actors()[1]) : s.actors()[1].transfer == TransferStage::None;
        if (finished) {
            step(s, 2);
            if (aged) {
                require(progressed && s.world().clock().tick_count() >= observed + 100 &&
                            s.actors()[1].task == TraversalTask::Eat,
                        "expired report suppressed scouting after long protected transport");
                return;
            }
            require(progressed && s.actors()[1].task == TraversalTask::None &&
                        s.actors()[1].action.value == 0 &&
                        s.world().clock().tick_count() < observed + 100,
                    "empty report was lost after protected motion completed");
            return;
        }
    }
    throw std::runtime_error("protected operation never physically finished");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected test case");
        const std::string name = argv[1];
        if (name == "no_global")
            no_global();
        else if (name == "witness")
            witness();
        else if (name == "delivery")
            delivery();
        else if (name == "persistence")
            persistence();
        else if (name == "stale")
            stale();
        else if (name == "contact")
            contact();
        else if (name == "corruption")
            corruption();
        else if (name == "relay")
            relay();
        else if (name == "scout")
            scout();
        else if (name == "safe_action")
            safe_action();
        else if (name == "bounded_auto")
            bounded_auto();
        else if (name == "autonomous_delivery")
            autonomous_delivery();
        else if (name == "safe_step")
            safe_motion(-1);
        else if (name == "safe_boarding")
            safe_motion(static_cast<int>(TransferStage::Boarding));
        else if (name == "safe_riding")
            safe_motion(static_cast<int>(TransferStage::Riding));
        else if (name == "safe_exiting")
            safe_motion(static_cast<int>(TransferStage::Exiting));
        else if (name == "safe_boarding_expired")
            safe_motion(static_cast<int>(TransferStage::Boarding), true);
        else if (name == "legacy4")
            legacy4();
        else if (name == "safe_clearance")
            safe_clearance();
        else
            throw std::invalid_argument("unknown test case");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
