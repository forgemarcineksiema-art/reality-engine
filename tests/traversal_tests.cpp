#include "lab/traversal_session.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace reality;
namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
TraversalScene flat() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {0, 0, 0}, 0, 10, 10, .3, 0, 0, true}};
    scene.spawns = {{{1, {-3, 0}}, {}, false}};
    return scene;
}
void course() {
    const auto started = std::chrono::steady_clock::now();
    TraversalSession session;
    session.validate();
    session.advance(5);
    session.validate();
    const auto bytes = session.serialize();
    TraversalSession clone;
    clone.restore(bytes);
    require(clone.serialize() == bytes, "course snapshot changed on load");
    std::cout << session.inspect() << "course_5_ticks_ms="
              << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                           started)
                     .count()
              << '\n';
}
void walk() {
    TraversalSession session{flat()};
    session.set_goal(0, {1, {3, 0}});
    for (unsigned tick = 0; tick < 220; ++tick) {
        session.advance(1);
        session.validate();
        if (tick % 47 == 0) {
            TraversalSession clone{flat()};
            clone.restore(session.serialize());
            clone.advance(1);
            auto next = session;
            next.advance(1);
            require(clone.serialize() == next.serialize(),
                    "in-flight velocity continuation diverged");
        }
    }
    const auto& actor = session.actors()[0];
    if (actor.state != TraversalState::Arrived ||
        length(actor.feet - SpatialPoint{3, 0, 0}) > .016) {
        std::cerr << session.inspect();
        throw std::runtime_error("accelerated walk failed to arrive");
    }
}
void ramp() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {0, 1, 0}, .5, 3, 2, .3, .4, 0, true}};
    scene.spawns = {{{1, {-2, 0}}, {}, false}};
    TraversalSession session{scene};
    session.set_goal(0, {1, {2, 0}});
    for (unsigned tick = 0; tick < 220; ++tick) {
        const auto before = session.actors()[0].feet;
        session.advance(1);
        session.validate();
        require(length(session.actors()[0].feet - before) <= 3 * .05 + 1e-6,
                "slope exceeds physical speed limit");
    }
    if (session.actors()[0].state != TraversalState::Arrived) {
        std::cerr << session.inspect();
        throw std::runtime_error("slope travel failed");
    }
}
void step() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {2, .2, 0}, 0, 2, 2, .3, 0, 0, true}};
    scene.spawns = {{{1, {-1, 0}}, {}, false}};
    scene.connections = {{{1, {1.7, 0}}, {2, {-1.7, 0}}}};
    TraversalSession session{scene};
    session.set_goal(0, {2, {1, 0}});
    for (unsigned tick = 0; tick < 300; ++tick) {
        session.advance(1);
        session.validate();
    }
    if (session.actors()[0].state != TraversalState::Arrived ||
        session.actors()[0].support.surface != 2) {
        std::cerr << session.inspect();
        throw std::runtime_error("physical step did not complete");
    }
}
void rider() {
    auto scene = flat();
    scene.spawns = {{{2, {.5, 0}}, {}, false}};
    scene.geometry.bodies.push_back({2, {0, 1, 0}, 0, 1, 1, .25, 0, 0, true});
    scene.platforms = {{{2, {0, 2, 0}, std::numbers::pi / 2, 80, 10, 0}, 0, false}};
    TraversalSession session{scene};
    for (unsigned tick = 0; tick < 220; ++tick) {
        session.advance(1);
        session.validate();
        const auto& actor = session.actors()[0];
        const auto& body = session.geometry().body(2);
        require(std::abs(body.local(actor.feet).x - .5) < 1e-8, "rider local anchor drifted");
        const auto shown = session.presentation(.5);
        const auto geometry = session.presentation_geometry(.5);
        require(std::abs(shown[0].feet.y - geometry.body(2).origin.y) < 1e-9,
                "interpolated rider leaves lift surface");
        if (tick == 34) {
            TraversalSession clone{scene};
            clone.restore(session.serialize());
            clone.advance(1);
            auto next = session;
            next.advance(1);
            require(clone.serialize() == next.serialize(), "moving-platform continuation diverged");
        }
    }
}
void autonomy() {
    auto scene = flat();
    scene.spawns[0].autonomous = true;
    scene.spawns[0].hunger = 700;
    scene.stations = {{{1, {0, 0}}, TraversalTask::Eat, 1}};
    TraversalSession session{scene};
    for (unsigned tick = 0; tick < 160; ++tick) {
        session.advance(1);
        session.validate();
    }
    if (session.stations()[0].completed != 1 ||
        session.world().resource(session.stations()[0].lot)->record.quantity.value != 0 ||
        session.actors()[0].hunger >= 250) {
        std::cerr << session.inspect();
        throw std::runtime_error("autonomy did not physically reach, consume and eat");
    }
    scene.stations[0].initial_stock = 0;
    TraversalSession hungry{scene};
    hungry.advance(160);
    hungry.validate();
    require(hungry.stations()[0].completed == 0 && hungry.actors()[0].hunger >= 700,
            "missing food created fake meal");
}
TraversalScene needs_fixture(bool unreachable) {
    auto scene = flat();
    scene.spawns[0].autonomous = true;
    scene.spawns[0].hunger = 700;
    scene.spawns[0].energy = 100;
    if (unreachable) {
        scene.geometry.bodies.push_back({2, {30, 0, 0}, 0, 3, 3, .3, 0, 0, true});
    }
    scene.stations = {{{unreachable ? 2U : 1U, {0, 0}}, TraversalTask::Eat, 0},
                      {{1, {-6, 0}}, TraversalTask::Rest, 1}};
    return scene;
}
void need_fallback(bool unreachable) {
    const auto scene = needs_fixture(unreachable);
    TraversalSession session{scene};
    session.advance(1);
    if (!unreachable) {
        require(session.actors()[0].task == TraversalTask::Eat,
                "chooser used remote stock knowledge instead of visiting food");
    }
    bool denied_locally = false, resumed_action = false;
    for (unsigned tick = 0; tick < 360; ++tick) {
        session.advance(1);
        session.validate();
        const auto actor = session.actors()[0];
        if (!unreachable && actor.station_retry[0] > session.world().clock().tick_count() &&
            !denied_locally) {
            require(length(actor.feet - SpatialPoint{0, 0, 0}) < 1.1,
                    "food denial was learned away from the station");
            TraversalSession loaded{scene};
            loaded.restore(session.serialize());
            require(loaded.serialize() == session.serialize(), "denial/cooldown lost on load");
            session = std::move(loaded);
            denied_locally = true;
        }
        if (actor.task == TraversalTask::Rest && actor.action.value != 0 && !resumed_action) {
            TraversalSession loaded{scene};
            loaded.restore(session.serialize());
            auto uninterrupted = session;
            loaded.advance(70);
            uninterrupted.advance(70);
            require(loaded.serialize() == uninterrupted.serialize(),
                    "rest reservation or effect diverged after load");
            require(loaded.stations()[1].completed == 1, "rest action did not finish after load");
            resumed_action = true;
        }
    }
    require(unreachable || denied_locally, "empty station was never observed locally");
    if (session.stations()[1].completed == 0) {
        std::cerr << session.inspect();
        throw std::runtime_error("unavailable food starved the necessary rest task");
    }
    require(resumed_action && session.actors()[0].energy > 300 &&
                session.actors()[0].hunger >= 700 && session.stations()[0].completed == 0 &&
                session.world().resource(session.stations()[0].lot)->record.quantity.value == 0,
            "fallback faked food, rest or need recovery");
}
void last_meal() {
    auto scene = flat();
    scene.spawns = {{{1, {-3, -1}}, {}, true, 700}, {{1, {-3, 1}}, {}, true, 700}};
    scene.stations = {{{1, {0, 0}}, TraversalTask::Eat, 1}};
    TraversalSession session{scene};
    bool saved_action = false;
    for (unsigned tick = 0; tick < 220; ++tick) {
        session.advance(1);
        session.validate();
        const auto running =
            session.actors()[0].action.value != 0 || session.actors()[1].action.value != 0;
        if (running && !saved_action) {
            require(session.world().resource(session.stations()[0].lot)->record.quantity.value ==
                            0 &&
                        session.stations()[0].completed == 0,
                    "meal did not consume the portion at action start");
            TraversalSession loaded{scene};
            loaded.restore(session.serialize());
            auto next = session;
            loaded.advance(100);
            next.advance(100);
            require(loaded.serialize() == next.serialize(), "last meal reservation diverged");
            require(loaded.stations()[0].completed == 1, "last portion was not consumed once");
            saved_action = true;
        }
    }
    unsigned fed = 0;
    for (const auto& actor : session.actors())
        fed += actor.hunger < 300 ? 1U : 0U;
    if (!saved_action || fed != 1 || session.stations()[0].completed != 1) {
        std::cerr << "saved_action=" << saved_action << " fed=" << fed << '\n' << session.inspect();
    }
    unsigned consumed = 0;
    for (const auto& event : session.world().events()) {
        if (event.kind == EventKind::Consumed)
            consumed += static_cast<unsigned>(event.quantity.value);
    }
    require(saved_action && fed == 1 && consumed == 1 && session.stations()[0].completed == 1 &&
                session.world().resource(session.stations()[0].lot)->record.quantity.value == 0,
            "two actors duplicated or failed to consume the last portion");
}
void reusable_rest() {
    auto scene = flat();
    scene.spawns = {{{1, {-3, -1}}, {}, true, 0, 100}, {{1, {-3, 1}}, {}, true, 0, 100}};
    scene.stations = {{{1, {0, 0}}, TraversalTask::Rest, 1}};
    TraversalSession session{scene};
    bool saved_busy = false;
    for (unsigned tick = 0; tick < 235; ++tick) {
        session.advance(1);
        session.validate();
        const auto resource = *session.world().resource(session.stations()[0].lot);
        if (resource.reserved.value == 1 && !saved_busy) {
            TraversalSession loaded{scene};
            loaded.restore(session.serialize());
            auto next = session;
            loaded.advance(200);
            next.advance(200);
            require(loaded.serialize() == next.serialize(), "occupied rest continuation diverged");
            require(loaded.stations()[0].completed == 2, "rest place was not reusable after load");
            saved_busy = true;
        }
        require(resource.record.quantity.value == 1, "rest consumed the reusable place");
    }
    require(saved_busy && session.stations()[0].completed == 2 &&
                session.actors()[0].energy > 300 && session.actors()[1].energy > 300,
            "two actors failed to share one rest place sequentially");
    for (const auto& event : session.world().events()) {
        require(event.kind != EventKind::Consumed, "rest capacity produced material consumption");
    }
}
void occupied_goal() {
    auto scene = flat();
    scene.spawns.push_back({{1, {3, 0}}, {}, false});
    TraversalSession session{scene};
    session.set_goal(0, {1, {3, 0}});
    session.advance(180);
    session.validate();
    require(session.actors()[0].state != TraversalState::Arrived,
            "occupied exact goal declared reached");
    session.approach(0, {1, {3, 0}}, 1);
    session.advance(180);
    session.validate();
    if (session.actors()[0].state != TraversalState::Arrived ||
        length(session.actors()[0].feet - session.actors()[1].feet) > 1.02) {
        std::cerr << session.inspect();
        throw std::runtime_error("explicit approach failed");
    }
}
TraversalScene ferry() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-3, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {5, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {3, {0, 0, 0}, 0, 1, 1, .25, 0, 0, true}};
    scene.spawns = {{{1, {0, 0}}, {}, false}};
    scene.platforms = {{{3, {2, 0, 0}, 0, 60, 10, 0}, 0, false}};
    scene.connections = {{{1, {1.8, 0}}, {2, {-1.8, 0}}, SurfaceLegKind::Transport, 0, 1, 4}};
    scene.services = {{1, 3, {{{1, {1.8, 0}}, {2, {-1.8, 0}}}}, {{{-.8, 0}, {.8, 0}}}, {}, {}}};
    return scene;
}
void transport() {
    TraversalSession session{ferry()};
    session.set_goal(0, {2, {0, 0}});
    bool boarded = false, rode = false, exited = false, loaded = false;
    for (unsigned tick = 0; tick < 650; ++tick) {
        const auto old = session.actors()[0].feet;
        session.advance(1);
        session.validate();
        const auto& actor = session.actors()[0];
        boarded = boarded || actor.transfer == TransferStage::Boarding;
        rode = rode || actor.transfer == TransferStage::Riding;
        exited = exited || actor.transfer == TransferStage::Exiting;
        require(length(actor.feet - old) < .25, "transport teleported passenger");
        if (actor.transfer == TransferStage::Riding && !loaded) {
            TraversalSession clone{ferry()};
            clone.restore(session.serialize());
            clone.advance(20);
            auto next = session;
            next.advance(20);
            require(clone.serialize() == next.serialize(),
                    "transport phase/owner continuation diverged");
            loaded = true;
        }
    }
    if (!boarded || !rode || !exited || session.actors()[0].state != TraversalState::Arrived ||
        session.actors()[0].support.surface != 2) {
        std::cerr << session.inspect();
        throw std::runtime_error("autonomous physical boarding/ride/exit failed");
    }
}
TraversalScene step_fixture() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {2, .2, 0}, 0, 2, 2, .3, 0, 0, true}};
    scene.spawns = {{{1, {-1, 0}}, {}, false}};
    scene.connections = {{{1, {1.7, 0}}, {2, {-1.7, 0}}}};
    return scene;
}
bool on_committed_step(const TraversalSession& session) {
    const auto& actor = session.actors()[0];
    return actor.leg < actor.route.legs.size() &&
           actor.route.legs[actor.leg].kind == SurfaceLegKind::Step && actor.step_point > 0;
}
void commit_step(TraversalSession& session, SurfaceLocation goal) {
    session.set_goal(0, goal);
    for (unsigned tick = 0; tick < 200 && !on_committed_step(session); ++tick) {
        session.advance(1);
        session.validate();
    }
    require(on_committed_step(session), "fixture never committed to a step");
}
void require_step_goal(const TraversalSession& session, const char* message) {
    const auto& actor = session.actors()[0];
    if (actor.state == TraversalState::Arrived && actor.support.surface == 2 &&
        length(actor.feet - SpatialPoint{3, .2, 0}) <= .016) {
        return;
    }
    std::cerr << session.inspect();
    throw std::runtime_error(message);
}
EntityId passage_owner(const TraversalSession& session) {
    const auto bytes = session.serialize();
    const auto begin = bytes.find("PASSAGES ");
    require(begin != std::string::npos, "missing passages in snapshot");
    std::istringstream input{bytes.substr(begin)};
    std::string tag;
    std::size_t count = 0;
    std::uint32_t id = 0, surface = 0;
    EntityId owner;
    require(static_cast<bool>(input >> tag >> count >> id >> surface >> owner.value) && count == 1,
            "expected one serialized passage");
    return owner;
}
void command_roundtrip() {
    const SurfaceLocation goal{2, {1, 0}}, other{1, {0, 0}};
    for (unsigned command = 0; command < 3; ++command) {
        TraversalSession original{step_fixture()};
        commit_step(original, goal);
        if (command == 0)
            original.set_goal(0, other);
        else if (command == 1)
            original.stop(0);
        else
            original.approach(0, other, .5);
        TraversalSession loaded{step_fixture()};
        loaded.restore(original.serialize());
        const auto before = loaded.actors()[0];
        loaded.set_goal(0, goal);
        const auto& after = loaded.actors()[0];
        require(!after.pending_goal && after.pending_reach == 0 && !after.stop_after_transfer &&
                    after.requested_goal == goal && after.requested_reach == 0,
                "latest exact goal did not clear deferred intent after load");
        require(after.plans == before.plans && after.leg == before.leg &&
                    after.step_point == before.step_point && after.feet == before.feet &&
                    after.motion.speed == before.motion.speed,
                "repeated goal restarted the committed motion");
        original.restore(loaded.serialize());
        for (unsigned tick = 0; tick < 300; ++tick) {
            loaded.advance(1);
            original.advance(1);
            loaded.validate();
            original.validate();
            require(loaded.serialize() == original.serialize(), "command continuation diverged");
        }
        require_step_goal(loaded, "latest goal failed after roundtrip");
    }
    // The same replacement must preserve a physically committed platform ride.
    TraversalSession ride{ferry()};
    const SurfaceLocation bank{2, {1, 0}};
    ride.set_goal(0, bank);
    for (unsigned tick = 0; tick < 400 && ride.actors()[0].transfer != TransferStage::Riding;
         ++tick)
        ride.advance(1);
    require(ride.actors()[0].transfer == TransferStage::Riding, "fixture never boarded ferry");
    ride.stop(0);
    TraversalSession loaded{ferry()};
    loaded.restore(ride.serialize());
    loaded.set_goal(0, bank);
    require(!loaded.actors()[0].stop_after_transfer &&
                loaded.actors()[0].transfer == TransferStage::Riding,
            "latest goal did not cancel deferred stop during ride");
    ride.restore(loaded.serialize());
    for (unsigned tick = 0; tick < 650; ++tick) {
        ride.advance(1);
        loaded.advance(1);
        ride.validate();
        loaded.validate();
        require(ride.serialize() == loaded.serialize(), "ride replacement continuation diverged");
    }
    require(loaded.actors()[0].state == TraversalState::Arrived &&
                length(loaded.actors()[0].feet - SpatialPoint{6, 0, 0}) < .016,
            "replacement goal did not finish ferry journey");
}
// Latest command wins: reissuing the goal already under execution must cancel a
// goal that was queued while the actor is committed to a step.
void supersede_pending_goal() {
    TraversalSession session{step_fixture()};
    const SurfaceLocation goal{2, {1, 0}}, other{1, {0, 0}};
    commit_step(session, goal);
    session.set_goal(0, other);
    require(session.actors()[0].pending_goal == other, "committed actor dropped the queued goal");
    session.set_goal(0, goal);
    const auto queued = session.actors()[0].pending_goal;
    require(!queued || *queued == goal, "reissued goal left the stale pending goal queued");
    for (unsigned tick = 0; tick < 300 && session.actors()[0].state != TraversalState::Arrived;
         ++tick) {
        session.advance(1);
        session.validate();
    }
    require_step_goal(session, "latest goal did not supersede the pending goal");
}
// Same deferral path for a committed stop: the later goal must clear it.
void supersede_pending_stop() {
    TraversalSession session{step_fixture()};
    const SurfaceLocation goal{2, {1, 0}};
    commit_step(session, goal);
    session.stop(0);
    require(session.actors()[0].stop_after_transfer, "committed stop was not queued");
    session.set_goal(0, goal);
    require(!session.actors()[0].stop_after_transfer, "reissued goal left the pending stop queued");
    for (unsigned tick = 0; tick < 300 && session.actors()[0].state != TraversalState::Arrived;
         ++tick) {
        session.advance(1);
        session.validate();
    }
    require_step_goal(session, "reissued goal did not supersede the pending stop");
}
// The source plate overlaps the gated plate up to x=.2. The explicit entry ends
// beyond that overlap. This permits manual support handoff before the local
// steering horizon reaches plate 2's edge; touching-only plates block departure
// first and cannot establish whether ownership is released after departure.
TraversalScene passage_gate() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-3.4, 0, 0}, 0, 3.6, 2, .3, 0, 0, true},
                             {2, {0, 0, 0}, 0, 1, 2, .3, 0, 0, true}};
    scene.spawns = {{{1, {-.6, 0}}, {}, false}, {{1, {-.6, 1}}, {}, false}};
    scene.connections = {{{1, {3.55, 0}}, {2, {.35, 0}}, SurfaceLegKind::Step, 1}};
    scene.passages = {{1, 2, {}, {}}};
    return scene;
}
void release_passage_ownership() {
    TraversalSession session{passage_gate()};
    const SurfaceLocation owner_goal{2, {.6, 1}}, waiter_goal{2, {.6, 0}};
    const auto owner_arrived = [&] {
        return session.actors()[0].state == TraversalState::Arrived &&
               session.actors()[0].support.surface == 2;
    };
    session.set_goal(0, owner_goal);
    for (unsigned tick = 0; tick < 300 && !owner_arrived(); ++tick) {
        session.advance(1);
        session.validate();
    }
    require(owner_arrived(), "fixture: routed actor never reached the gated plate");
    // The waiter is queued behind the owner and must not cross while the owner stays.
    session.set_goal(1, waiter_goal);
    for (unsigned tick = 0; tick < 300 && session.actors()[1].state != TraversalState::Waiting;
         ++tick) {
        session.advance(1);
        session.validate();
    }
    require(session.actors()[1].state == TraversalState::Waiting &&
                session.actors()[1].support.surface != 2,
            "fixture did not hold the passage: waiter was not queued behind the owner");
    // Manual departure: no goal and no routed progress run, so ownership can only
    // be released by the movement path itself.
    session.set_input(0, {-1, 0});
    bool held_after_support_change = false;
    std::string occupied_checkpoint;
    for (unsigned tick = 0; tick < 80 && session.actors()[0].feet.x >= -1.5; ++tick) {
        session.advance(1);
        session.validate();
        const auto& owner = session.actors()[0];
        if (owner.support.surface == 1 && owner.feet.x > -1.2) {
            require(passage_owner(session) == owner.id,
                    "support handoff released the passage while the capsule remained inside");
            held_after_support_change = true;
            if (occupied_checkpoint.empty())
                occupied_checkpoint = session.serialize();
        }
    }
    session.clear_input();
    require(held_after_support_change, "fixture never exercised overlapping support ownership");
    require(passage_owner(session) != session.actors()[0].id,
            "manual departure retained passage ownership");
    TraversalSession continuation{passage_gate()};
    continuation.restore(session.serialize());
    continuation.advance(340);
    continuation.validate();
    require(continuation.actors()[1].state == TraversalState::Arrived,
            "passage waiter starved after load");
    continuation.restore(occupied_checkpoint);
    require(passage_owner(continuation) == continuation.actors()[0].id,
            "load lost physically occupied passage ownership");
    // Keyboard state is transient; deliberately resume the same manual command.
    continuation.set_input(0, {-1, 0});
    continuation.advance(80);
    continuation.clear_input();
    continuation.advance(340);
    continuation.validate();
    require(continuation.actors()[1].state == TraversalState::Arrived,
            "restored passage ownership was never released on manual departure");
    require(session.actors()[0].support.surface == 1 && session.actors()[0].feet.x < -1.5,
            "fixture: manual departure did not leave the gated plate");
    std::cout << "manual_departure support=" << session.actors()[0].support.surface
              << " feet_x=" << session.actors()[0].feet.x << '\n';
    for (unsigned tick = 0; tick < 40 && session.actors()[1].state == TraversalState::Waiting;
         ++tick) {
        session.advance(1);
        session.validate();
    }
    if (session.actors()[1].state == TraversalState::Waiting) {
        const auto bytes = session.serialize();
        const auto begin = bytes.find("PASSAGES ");
        const auto end = bytes.find("STATIONS ", begin);
        std::cerr << session.inspect() << bytes.substr(begin, end - begin);
    }
    require(session.actors()[1].state != TraversalState::Waiting,
            "waiter still queued after the owner left the passage");
    for (unsigned tick = 0; tick < 300 && session.actors()[1].state != TraversalState::Arrived;
         ++tick) {
        session.advance(1);
        session.validate();
    }
    const auto& waiter = session.actors()[1];
    if (waiter.state != TraversalState::Arrived || waiter.support.surface != 2 ||
        length(waiter.feet - SpatialPoint{.6, 0, 0}) > .016) {
        std::cerr << session.inspect();
        throw std::runtime_error("passage ownership was not released: waiter starved");
    }
}
void manual_touching_departure() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-4, 0, 0}, 0, 3, 2, .3, 0, 0, true},
                             {2, {0, 0, 0}, 0, 1, 2, .3, 0, 0, true}};
    scene.spawns = {{{2, {0, 0}}, {}, false}};
    TraversalSession session{scene};
    session.set_input(0, {-1, 0});
    for (unsigned tick = 0; tick < 80; ++tick) {
        session.advance(1);
        session.validate();
    }
    const auto& actor = session.actors()[0];
    if (actor.support.surface != 1 || actor.feet.x >= -1.5) {
        std::cerr << session.inspect();
        throw std::runtime_error("manual movement cannot cross touching coplanar supports");
    }
}
void support_coverage() {
    SpatialGeometry geometry;
    geometry.bodies = {{1, {-1, 0, 0}, 0, 1, 2, .3, 0, 0, true},
                       {2, {1, 0, 0}, 0, 1, 2, .3, 0, 0, true}};
    const auto clear = [&] { return continuous_surface_feet(geometry, 1, {-1, 0, 0}, {1, 0}, {}); };
    require(clear().has_value(), "touching supports rejected");
    geometry.bodies[1].origin.x += .01;
    require(!clear(), "unsupported interior gap accepted despite supported endpoint");
    geometry.bodies[1].origin.x -= .01;
    geometry.bodies[1].origin.y = .1;
    require(!clear(), "height change bypassed explicit step traversal");
    geometry.bodies[1].origin.y = 0;
    geometry.bodies[1].walkable = false;
    require(!clear(), "nonwalkable support accepted");
    geometry.bodies[1].walkable = true;
    // Rotate the complete fixture, retaining a genuine coplanar join.
    for (auto& body : geometry.bodies) {
        const auto x = body.origin.x;
        body.origin.x = x * std::cos(.6);
        body.origin.z = x * std::sin(.6);
        body.yaw = .6;
    }
    require(continuous_surface_feet(geometry, 1, geometry.bodies[0].origin,
                                    {geometry.bodies[1].origin.x, geometry.bodies[1].origin.z}, {})
                .has_value(),
            "rotated coplanar join rejected");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 3 && std::string_view{argv[1]} == "--case", "expected --case NAME");
        const std::string_view name{argv[2]};
        if (name == "course") {
            course();
        } else if (name == "walk") {
            walk();
        } else if (name == "ramp") {
            ramp();
        } else if (name == "step") {
            step();
        } else if (name == "rider") {
            rider();
        } else if (name == "autonomy") {
            autonomy();
        } else if (name == "occupied_goal") {
            occupied_goal();
        } else if (name == "need_fallback_empty") {
            need_fallback(false);
        } else if (name == "need_fallback_unreachable") {
            need_fallback(true);
        } else if (name == "last_meal") {
            last_meal();
        } else if (name == "reusable_rest") {
            reusable_rest();
        } else if (name == "transport") {
            transport();
        } else if (name == "supersede_pending_goal") {
            supersede_pending_goal();
        } else if (name == "supersede_pending_stop") {
            supersede_pending_stop();
        } else if (name == "release_passage_ownership") {
            release_passage_ownership();
        } else if (name == "manual_touching_departure") {
            manual_touching_departure();
        } else if (name == "command_roundtrip") {
            command_roundtrip();
        } else if (name == "support_coverage") {
            support_coverage();
        } else {
            throw std::invalid_argument("unknown test case");
        }
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
