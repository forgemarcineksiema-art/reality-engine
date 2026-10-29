#include "graphics/lab_scene.hpp"
#include "lab/traversal_session.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reality;
namespace {
void require(bool v, const char* m) {
    if (!v)
        throw std::runtime_error(m);
}
void balance(const TraversalSession& s) {
    std::uint64_t total = 0;
    for (const auto& slot : s.world().entities().slots())
        if (slot.occupied && slot.kind == EntityKind::Resource) {
            const auto r = s.world().resource(slot.id);
            if (r && r->record.material == MaterialTypeId{1})
                total += r->record.quantity.value;
        }
    for (const auto& c : s.world().commands())
        if (c.result.status == RequestStatus::Accepted) {
            const auto a = s.world().action(c.result.action);
            if (a && s.world().resource(a->request.lot)->record.material == MaterialTypeId{1})
                total += a->consumed.value;
        }
    require(total == 12, "chronological material balance changed");
}
void step(TraversalSession& s) {
    s.advance(1);
    s.validate();
    balance(s);
}
void reach_far(TraversalSession& s) {
    for (unsigned n = 0; n < 180 && s.actors()[0].local(); ++n) {
        const auto energy = s.actors()[0].energy;
        step(s);
        if (!s.actors()[0].local())
            require(s.actors()[0].energy + s.actors()[0].journey->elapsed == energy,
                    "automatic departure did not charge its first travel tick exactly once");
    }
    require(!s.actors()[0].local(), "courier never handed off to distant ownership");
}
void finish(TraversalSession& s) {
    for (unsigned n = 0;
         n < 900 && (s.actors()[0].delivery->delivered != 8 || !s.stations()[1].completed); ++n)
        step(s);
    require(s.actors()[0].delivery->delivered == 8 && s.stations()[1].completed > 0,
            "real distant trips did not deliver eight units and feed the recipient");
}
TraversalScene blocked_scene() {
    auto scene = make_distant_course();
    scene.spawns[2].location = scene.distant_connections[0].terminals[1];
    scene.spawns[2].autonomous = false;
    return scene;
}
void handoff() {
    TraversalSession s{make_distant_course()};
    const auto id = s.actors()[0].id;
    reach_far(s);
    const auto cargo = s.actors()[0].cargo;
    const auto stock = s.world().resource(cargo)->record.quantity;
    const auto before = s.actors()[0];
    require(stock.value == 4, "departed without the physical cargo batch");
    require(!s.can_interact_at(0, s.stations()[0].location) && !s.can_exchange_knowledge(0, 1),
            "distant actor still has contact");
    require(s.interact_cargo(0, 0) == CargoStatus::OutOfReach,
            "distant actor transferred cargo locally");
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::OutOfReach,
            "distant actor shared locally");
    require(!s.presentation(.5)[0].visible, "distant actor still has a rendered capsule");
    const auto mesh = build_traversal_scene(s, 0, LabCamera{}, 1, .5);
    require(std::none_of(mesh.begin(), mesh.end(),
                         [](const auto& triangle) { return triangle.object == 0; }) &&
                std::any_of(mesh.begin(), mesh.end(),
                            [](const auto& triangle) { return triangle.object == 1; }) &&
                std::any_of(mesh.begin(), mesh.end(),
                            [](const auto& triangle) { return triangle.object == 2; }),
            "hidden distant mesh retained picking geometry or changed local object IDs");
    s.set_goal(0, {1, {0, 0}});
    s.set_input(0, {1, 0});
    for (unsigned n = 0; n < 20; ++n)
        step(s);
    require(s.actors()[0].feet == before.feet &&
                s.actors()[0].journey->elapsed == before.journey->elapsed + 20,
            "local controller also moved distant actor or travel time changed");
    require(s.actors()[0].hunger == before.hunger + 20 &&
                s.actors()[0].energy == before.energy - 20,
            "shared clock did not progress needs exactly once");
    require(s.actors()[0].beliefs[0]->observed_tick == before.beliefs[0]->observed_tick,
            "distant actor renewed a local observation");
    require(s.actors()[0].id == id && s.actors()[0].cargo == cargo &&
                s.world().resource(cargo)->record.quantity == stock,
            "handoff duplicated identity or cargo");
    finish(s);
    require(s.actors()[0].id == id && s.actors()[0].cargo == cargo,
            "multiple handoffs replaced identity or cargo");
    std::cout << s.inspect();
}
void ghost() {
    TraversalSession s{make_distant_course()};
    reach_far(s);
    const auto departure = s.distant_connections()[0].terminals[0];
    s.set_goal(1, departure);
    for (unsigned n = 0; n < 65; ++n)
        step(s);
    const auto target = surface_feet(s.geometry(), departure, TraversalLimits{s.actors()[1].size});
    require(target && length(s.actors()[1].feet - *target) < .02,
            "distant ghost still blocks local movement");
    require(!s.actors()[0].local(), "ghost trial exceeded distant travel duration");
}
void blocked() {
    TraversalSession s{blocked_scene()};
    reach_far(s);
    const auto cargo = s.actors()[0].cargo;
    for (unsigned n = 0; n < 150; ++n)
        step(s);
    require(!s.actors()[0].local() && s.actors()[0].journey->elapsed == 120 &&
                s.world().resource(cargo)->record.quantity.value == 4 &&
                s.actors()[0].delivery->delivered == 0,
            "occupied terminal restarted travel, overlapped or delivered remotely");
    const auto energy = s.actors()[0].energy, hunger = s.actors()[0].hunger;
    for (unsigned n = 0; n < 15; ++n)
        step(s);
    require(s.actors()[0].energy == energy && s.actors()[0].hunger == hunger + 15,
            "waiting repeated travel effort or stopped the needs clock");
    const auto bytes = s.serialize();
    TraversalSession restored;
    restored.restore(bytes);
    require(restored.serialize() == bytes, "blocked return changed through restore");
    s.set_goal(2, {2, {1, 2}});
    for (unsigned n = 0; n < 80 && !s.actors()[0].local(); ++n)
        step(s);
    require(s.actors()[0].local() && s.actors()[0].support.surface == 2,
            "released terminal never accepted physical return");
    for (unsigned n = 0; n < 60; ++n)
        step(s);
    s.set_autonomous(2, true);
    finish(s);
}
void stale_goal() {
    TraversalSession s{make_distant_course()};
    reach_far(s);
    s.set_goal(1, s.distant_connections()[0].terminals[1]);
    require(s.actors()[1].state == TraversalState::NoPath, "stale goal fixture found local route");
    for (unsigned n = 0; n < 120; ++n)
        step(s);
    require(s.actors()[0].local() && s.actors()[0].support.surface == 2,
            "empty return terminal was reserved by another actor's stale goal");
}
void closed() {
    TraversalSession s{make_distant_course()};
    reach_far(s);
    require(s.set_distant_open(1, false), "connection missing");
    for (unsigned n = 0; n < 120; ++n)
        step(s);
    require(s.actors()[0].local() && s.actors()[0].support.surface == 2,
            "closure stranded an already committed journey");
    s.stop(0);
    s.set_goal(0, s.distant_connections()[0].terminals[1]);
    for (unsigned n = 0; n < 100 && (s.actors()[0].state != TraversalState::Arrived ||
                                     s.actors()[0].motion.speed > .01);
         ++n)
        step(s);
    const auto bytes = s.serialize();
    require(s.depart(0, 1, 1) == DistantOutcome::Closed && s.serialize() == bytes,
            "closed departure mutated physical state");
}
void needs() {
    auto scene = make_distant_course();
    scene.spawns[0].hunger = 740;
    scene.spawns[0].energy = 105;
    // Explicit command starts from a safe terminal with a loaded real bag.
    scene.spawns[0].location = scene.distant_connections[0].terminals[0];
    scene.spawns[0].autonomous = false;
    scene.stations[0].location = scene.spawns[0].location;
    TraversalSession s{scene};
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "load failed");
    require(s.depart(0, 1, 0) == DistantOutcome::Departed, "safe departure refused");
    s.set_autonomous(0, true);
    for (unsigned n = 0; n < 119; ++n)
        step(s);
    require(!s.actors()[0].local() && s.actors()[0].energy == 0 && s.actors()[0].hunger == 859,
            "critical needs cancelled travel or received free food/energy");
    step(s);
    require(s.actors()[0].local() && s.actors()[0].finish_grace_until == 0,
            "reentry refreshed needs protection");
    step(s);
    require(s.actors()[0].task == TraversalTask::Eat || s.actors()[0].task == TraversalTask::Rest,
            "safe terminal did not restore urgent needs planning");
}
std::string rechecksum(std::string s) {
    s.resize(s.rfind("CHECKSUM "));
    return s + "CHECKSUM " + std::to_string(snapshot_checksum(s)) + "\n";
}
void corruption() {
    TraversalSession s{make_distant_course()};
    reach_far(s);
    const auto bytes = s.serialize();
    const auto reject = [&](std::string bad) {
        bool failed = false;
        try {
            s.restore(rechecksum(bad));
        } catch (const std::exception&) {
            failed = true;
        }
        require(failed && s.serialize() == bytes, "semantic corruption committed partial state");
    };
    const auto row = bytes.find("JOURNEY " + std::to_string(s.actors()[0].id.value));
    const auto end = bytes.find('\n', row);
    for (const auto& record : {"JOURNEY 1 1 999 0 0 0", "JOURNEY 1 1 1 2 0 0",
                               "JOURNEY 1 1 1 0 99999 0", "JOURNEY 1 1 1 0 0 999"}) {
        auto bad = bytes;
        bad.replace(row, end - row, record);
        reject(bad);
    }
    for (const auto& token : {"OPTIONAL_PRACTICE 2", "OPTIONAL_PRACTICE 1"}) {
        auto bad = bytes;
        const auto pos = bad.find("OPTIONAL_PRACTICE 0");
        bad.replace(pos, 19, token);
        reject(bad);
    }
    auto bad = bytes;
    const auto pos = bad.find("DISTANT_CONNECTION 1");
    const auto e = bad.find('\n', pos);
    bad.replace(pos, e - pos, "DISTANT_CONNECTION 1 1 2 0 2 -2 0 1800 1 1");
    reject(bad);
    auto legacy_scene = make_distant_course();
    legacy_scene.distant_connections.clear();
    legacy_scene.initial_deliveries.clear();
    legacy_scene.spawns.resize(1);
    legacy_scene.stations[0].kind = TraversalTask::Maintain;
    legacy_scene.stations[0].initial_material = {2};
    legacy_scene.practice = PracticeSeed{};
    legacy_scene.practice->station = 0;
    legacy_scene.practice->researchers = {0};
    TraversalSession old{legacy_scene};
    const auto oldbytes = old.serialize();
    s.restore(oldbytes);
    require(s.serialize() == oldbytes && s.distant_connections().empty(),
            "old v8 world retained distant state or changed bytes");
}
void protected_phases() {
    auto intent_scene = make_distant_course();
    intent_scene.initial_deliveries.clear();
    intent_scene.spawns[0].location = intent_scene.distant_connections[0].terminals[0];
    intent_scene.spawns[0].hunger = 500;
    intent_scene.stations[0].location = {1, {1, 0}};
    TraversalSession intent{intent_scene};
    intent.advance(1);
    intent.validate();
    require(intent.actors()[0].task == TraversalTask::Eat && !intent.actors()[0].action.value &&
                intent.actors()[0].motion.speed <= .01,
            "planned need fixture did not reach safe slow planning phase");
    const auto intent_bytes = intent.serialize();
    const auto intent_result = intent.depart(0, 1, 0);
    auto queued_scene = make_distant_course();
    queued_scene.initial_deliveries.clear();
    queued_scene.spawns[0].location = queued_scene.distant_connections[0].terminals[0];
    queued_scene.spawns[0].autonomous = false;
    TraversalSession queued{queued_scene};
    queued.queue_input(0, {1, 0});
    const auto queued_bytes = queued.serialize();
    const auto queued_result = queued.depart(0, 1, 0);
    std::cout << "planned_depart=" << static_cast<unsigned>(intent_result)
              << " queued_depart=" << static_cast<unsigned>(queued_result) << '\n';
    require(intent_result == DistantOutcome::Busy && intent.serialize() == intent_bytes,
            "departure accepted a pending non-delivery need or discarded its plan");
    require(queued_result == DistantOutcome::Busy && queued.serialize() == queued_bytes,
            "departure accepted queued local movement or erased its command");

    auto scene = make_distant_course();
    scene.spawns[0].location = scene.distant_connections[0].terminals[0];
    scene.spawns[0].autonomous = false;
    TraversalSession s{scene};
    s.set_input(0, {1, 0});
    for (unsigned n = 0; n < 5; ++n)
        step(s);
    const auto before = s.serialize();
    require(s.depart(0, 1, 0) == DistantOutcome::Busy && s.serialize() == before,
            "moving handoff changed state");
    auto course = make_traversal_course();
    course.distant_connections = {{1, {{{1, {-2, 0}}, {2, {2, 0}}}}, 18, 120, true}};
    TraversalSession t{course};
    t.set_goal(0, {5, {0, 0}});
    bool found = false;
    for (unsigned n = 0; n < 180 && !found; ++n) {
        t.advance(1);
        const auto& a = t.actors()[0];
        found = a.step_point > 0;
    }
    require(found, "protected test never reached Step");
    const auto snap = t.serialize();
    require(t.depart(0, 1, 0) == DistantOutcome::Busy && t.serialize() == snap,
            "handoff interrupted committed Step");
    TraversalScene ferry;
    ferry.geometry.bodies = {{1, {-3, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {5, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {3, {0, 0, 0}, 0, 1, 1, .25, 0, 0, true}};
    ferry.spawns = {{{1, {0, 0}}, {}, false}};
    ferry.platforms = {{{3, {2, 0, 0}, 0, 60, 10, 0}, 0, false}};
    ferry.connections = {{{1, {1.8, 0}}, {2, {-1.8, 0}}, SurfaceLegKind::Transport, 0, 1, 4}};
    ferry.services = {{1, 3, {{{1, {1.8, 0}}, {2, {-1.8, 0}}}}, {{{-.8, 0}, {.8, 0}}}, {}, {}}};
    ferry.distant_connections = {{1, {{{1, {0, 0}}, {2, {0, 0}}}}, 18, 120, true}};
    TraversalSession ride{ferry};
    ride.set_goal(0, {2, {0, 0}});
    bool boarded = false, rode = false, exited = false;
    for (unsigned n = 0; n < 400 && !exited; ++n) {
        ride.advance(1);
        const auto phase = ride.actors()[0].transfer;
        if (phase == TransferStage::Boarding || phase == TransferStage::Riding ||
            phase == TransferStage::Exiting) {
            boarded = boarded || phase == TransferStage::Boarding;
            rode = rode || phase == TransferStage::Riding;
            exited = exited || phase == TransferStage::Exiting;
            const auto rs = ride.serialize();
            require(ride.depart(0, 1, 0) == DistantOutcome::Busy && ride.serialize() == rs,
                    "handoff interrupted reserved ferry");
        }
    }
    require(boarded && rode && exited, "protected test never traversed all ferry phases");
}
void release_blocker(TraversalSession& s) {
    s.set_goal(2, {2, {1, 2}});
    for (unsigned n = 0; n < 80; ++n)
        step(s);
    s.set_autonomous(2, true);
}
void isolation() {
    auto scene = make_distant_course();
    scene.initial_deliveries.clear();
    scene.spawns[0].location = scene.distant_connections[0].terminals[0];
    scene.spawns[0].autonomous = false;
    scene.stations[0].location = scene.spawns[0].location;
    scene.spawns[1].location = {1, {1, 0}};
    scene.spawns[1].autonomous = false;
    scene.property_rules = {{0, 2, {0}, {}}};
    TraversalSession law{scene};
    require(law.depart(0, 1, 0) == DistantOutcome::Departed, "witness depart refused");
    require(law.interact_cargo(1, 0) == CargoStatus::Transferred && law.incidents().size() == 1 &&
                law.actors()[0].testimonies.empty(),
            "distant actor witnessed a local theft");
    require(!law.can_exchange_knowledge(0, 1), "distant historical anchor exchanged testimony");
    auto practice = make_distant_course();
    practice.initial_deliveries.clear();
    practice.spawns.resize(1);
    practice.spawns[0].location = practice.distant_connections[0].terminals[0];
    practice.spawns[0].autonomous = false;
    practice.stations[0].location = practice.spawns[0].location;
    practice.stations[0].kind = TraversalTask::Maintain;
    practice.stations[0].initial_material = {2};
    practice.practice = PracticeSeed{};
    practice.practice->station = 0;
    practice.practice->researchers = {0};
    TraversalSession p{practice};
    require(p.practice_probe(0, {0, 0}, 16) == DiscoveryOutcome::Accepted,
            "practice fixture refused");
    const auto pulse = p.serialize();
    require(p.depart(0, 1, 0) == DistantOutcome::Busy && p.serialize() == pulse,
            "handoff interrupted apparatus pulse");
    p.advance(2);
    require(p.begin_maintenance(0, 0), "calibrated work never began");
    const auto work = p.serialize();
    require(p.depart(0, 1, 0) == DistantOutcome::Busy && p.serialize() == work,
            "handoff interrupted work");
    TraversalSession restored;
    restored.restore(work);
    require(restored.serialize() == work, "v9 optional practice state changed");
}
void manual() {
    auto scene = make_distant_course();
    scene.spawns[0].location = scene.distant_connections[0].terminals[0];
    scene.stations[0].location = scene.spawns[0].location;
    scene.spawns[0].autonomous = false;
    TraversalSession s{scene};
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "manual real load refused");
    require(s.depart(0, 1, 0) == DistantOutcome::Departed, "manual safe departure refused");
    for (unsigned n = 0; n < 45; ++n)
        step(s);
    require(s.actors()[0].energy == 955 && s.actors()[0].journey->elapsed == 45,
            "manual travel energy/time differ");
    TraversalSession restored;
    restored.restore(s.serialize());
    s.cancel_delivery(0);
    restored.cancel_delivery(0);
    for (unsigned n = 0; n < 75; ++n) {
        step(s);
        step(restored);
    }
    require(s.serialize() == restored.serialize() && s.actors()[0].local() &&
                !s.actors()[0].autonomous && s.actors()[0].energy == 880 &&
                s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4,
            "manual return lost identity, paid twice, teleported cargo or changed through save");
}
TraversalScene cancellation_scene() {
    auto scene = make_distant_course();
    scene.spawns[0].known_stations = 1;
    return scene;
}
void finish_cancelled(TraversalSession& s) {
    const auto id = s.actors()[0].id, cargo = s.actors()[0].cargo;
    for (unsigned n = 0; n < 800 && s.world().resource(cargo)->record.quantity.value > 0; ++n)
        step(s);
    require(
        s.world().resource(cargo)->record.quantity.value == 0 && s.actors()[0].local() &&
            s.actors()[0].support.surface == 1 && s.actors()[0].id == id &&
            s.actors()[0].cargo == cargo &&
            s.actors()[0].delivery->phase == DeliveryPhase::Cancelled &&
            s.actors()[0].delivery->delivered == 0,
        "cancelled distant cargo did not physically return to its only known compatible station");
    unsigned flows = 0;
    CommandId previous;
    for (const auto& transfer : s.world().transfers())
        if (transfer.result.status == TransferStatus::Accepted &&
            (transfer.request.source == cargo || transfer.request.destination == cargo)) {
            require(transfer.request.actor == id && transfer.request.command != previous,
                    "cancelled transfer reused a command or actor");
            previous = transfer.request.command;
            ++flows;
            require(transfer.request.quantity.value == 4 &&
                        (transfer.request.source == s.stations()[0].lot ||
                         transfer.request.destination == s.stations()[0].lot),
                    "cancelled batch was sent to an unknown or fictional station");
        }
    require(flows == 2, "cancelled batch loaded or deposited more than once");
}
void cancelled_return() {
    TraversalSession s{cancellation_scene()};
    reach_far(s);
    s.cancel_delivery(0);
    require(!s.actors()[0].local() && s.actors()[0].autonomous,
            "cancellation teleported courier or silently disabled autonomy");
    finish_cancelled(s);
}
void checkpoint_cancelled(const std::filesystem::path& save,
                          const std::filesystem::path& expected) {
    TraversalSession s{cancellation_scene()};
    reach_far(s);
    s.cancel_delivery(0);
    for (unsigned n = 0; n < 37; ++n)
        step(s);
    s.save(save);
    finish_cancelled(s);
    s.save(expected);
}
void checkpoint(bool waiting, const std::filesystem::path& save,
                const std::filesystem::path& expected) {
    TraversalSession s{waiting ? blocked_scene() : make_distant_course()};
    reach_far(s);
    for (unsigned n = 0; n < (waiting ? 150u : 37u); ++n)
        step(s);
    s.save(save);
    if (waiting)
        release_blocker(s);
    finish(s);
    s.save(expected);
}
} // namespace
int main(int argc, char** argv) {
    try {
        const std::string name = argc > 1 ? argv[1] : "handoff";
        if (name == "stale_goal")
            stale_goal();
        else if (name == "handoff")
            handoff();
        else if (name == "ghost")
            ghost();
        else if (name == "blocked")
            blocked();
        else if (name == "closed")
            closed();
        else if (name == "needs")
            needs();
        else if (name == "corruption")
            corruption();
        else if (name == "protected")
            protected_phases();
        else if (name == "isolation")
            isolation();
        else if (name == "manual")
            manual();
        else if (name == "cancelled_return")
            cancelled_return();
        else if (name == "checkpoint" && argc == 5 && std::string(argv[2]) == "cancelled")
            checkpoint_cancelled(argv[3], argv[4]);
        else if (name == "checkpoint" && argc == 5)
            checkpoint(std::string(argv[2]) == "waiting", argv[3], argv[4]);
        else if (name == "continue" && argc == 5) {
            TraversalSession s;
            s.load(argv[3]);
            if (std::string(argv[2]) == "cancelled")
                finish_cancelled(s);
            else {
                if (std::string(argv[2]) == "waiting")
                    release_blocker(s);
                finish(s);
            }
            s.save(argv[4]);
        } else
            throw std::runtime_error("unknown case");
        std::cout << "PASS " << name << '\n';
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
