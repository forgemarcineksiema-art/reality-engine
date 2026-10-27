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
void step(TraversalSession& session, unsigned ticks) {
    for (unsigned i = 0; i < ticks; ++i) {
        session.advance(1);
        session.validate();
    }
}
TraversalScene needs_scene(std::uint64_t energy = 1000) {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {}, 0, 120, 5, .3, 0, 0, true}};
    scene.spawns = {{{1, {-20, 0}}, {}, true, 0, energy}};
    scene.stations = {{{1, {100, 0}}, TraversalTask::Maintain, 80},
                      {{1, {0, 1}}, TraversalTask::Eat, 80},
                      {{1, {0, -1}}, TraversalTask::Rest, 1}};
    return scene;
}
void interruption() {
    TraversalSession session{needs_scene()};
    step(session, 1);
    require(session.actors()[0].task == TraversalTask::Maintain,
            "fixture did not start maintenance");
    step(session, 779);
    if (session.actors()[0].task != TraversalTask::Eat)
        std::cerr << session.inspect();
    require(session.actors()[0].task == TraversalTask::Eat && session.stations()[0].completed == 0,
            "critical hunger did not interrupt travelling to nonurgent maintenance");
    bool resumed = false;
    auto previous_task = session.actors()[0].task;
    for (unsigned t = 0; t < 4000 && session.stations()[0].completed == 0; ++t) {
        step(session, 1);
        if (session.actors()[0].task != previous_task) {
            std::cerr << "task_change tick=" << session.world().clock().tick_count()
                      << " task=" << static_cast<unsigned>(session.actors()[0].task)
                      << " x=" << session.actors()[0].feet.x
                      << " hunger=" << session.actors()[0].hunger
                      << " energy=" << session.actors()[0].energy << '\n';
            previous_task = session.actors()[0].task;
        }
        resumed = resumed || (session.stations()[1].completed > 0 &&
                              session.actors()[0].task == TraversalTask::Maintain);
    }
    if (!(session.stations()[1].completed > 0 && resumed && session.stations()[0].completed > 0))
        std::cerr << session.inspect();
    require(session.stations()[1].completed > 0 && resumed && session.stations()[0].completed > 0,
            "interrupted maintenance failed to resume and complete after actual need satisfaction");
}
void exhaustion() {
    TraversalSession session{needs_scene(305)};
    step(session, 1);
    require(session.actors()[0].task == TraversalTask::Maintain,
            "fixture did not start maintenance");
    bool changed = false;
    for (unsigned t = 0; t < 350 && !changed; ++t) {
        step(session, 1);
        changed = session.actors()[0].task == TraversalTask::Rest;
    }
    require(changed && session.stations()[0].completed == 0,
            "critical exhaustion did not interrupt nonurgent maintenance");
}
std::string changed_actor(std::string bytes, std::size_t field, const std::string& value) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto actors = bytes.find("\nACTORS ", bytes.find("\nTUNING "));
    const auto first = bytes.find('\n', actors + 1) + 1, last = bytes.find('\n', first);
    std::istringstream in{bytes.substr(first, last - first)};
    std::ostringstream out;
    std::string token;
    std::size_t n = 0;
    while (in >> token) {
        if (n)
            out << ' ';
        out << (n++ == field ? value : token);
    }
    require(n > field, "missing actor mutation field");
    bytes.replace(first, last - first, out.str());
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n';
}
TraversalScene step_scene() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                             {2, {20, .2, 0}, 0, 20, 3, .3, 0, 0, true}};
    scene.connections = {{{1, {1.7, 0}}, {2, {-19.7, 0}}}};
    scene.spawns = {{{1, {-1, 0}}, {}, true}};
    scene.stations = {{{2, {10, 0}}, TraversalTask::Maintain, 80},
                      {{1, {-1, 1}}, TraversalTask::Eat, 80}};
    return scene;
}
bool committed_step(const TraversalActor& actor) {
    return actor.leg < actor.route.legs.size() &&
           actor.route.legs[actor.leg].kind == SurfaceLegKind::Step && actor.step_point > 0;
}
void safe_step() {
    TraversalSession session{step_scene()};
    for (unsigned t = 0; t < 200 && !committed_step(session.actors()[0]); ++t)
        step(session, 1);
    require(committed_step(session.actors()[0]), "fixture never started a physical step");
    session.restore(changed_actor(session.serialize(), 19, "749"));
    bool finished = false;
    for (unsigned t = 0; t < 100; ++t) {
        const auto unsafe = committed_step(session.actors()[0]);
        step(session, 1);
        if (unsafe)
            require(session.actors()[0].task == TraversalTask::Maintain,
                    "urgent need cancelled a committed physical step");
        if (!committed_step(session.actors()[0])) {
            finished = true;
            break;
        }
    }
    require(finished, "physical step did not finish");
    step(session, 2);
    require(session.actors()[0].task == TraversalTask::Eat,
            "urgent need lost after safe step completion");
}
TraversalScene transport_scene() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-3, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                             {2, {20, 0, 0}, 0, 17, 3, .3, 0, 0, true},
                             {3, {}, 0, 1, 1, .25, 0, 0, true}};
    scene.platforms = {{{3, {2, 0, 0}, 0, 60, 10, 0}, 0, false}};
    scene.connections = {{{1, {1.8, 0}}, {2, {-16.8, 0}}, SurfaceLegKind::Transport, 0, 1, 4}};
    scene.services = {{1, 3, {{{1, {1.8, 0}}, {2, {-16.8, 0}}}}, {{{-.8, 0}, {.8, 0}}}, {}, {}}};
    scene.spawns = {{{1, {0, 0}}, {}, true}};
    scene.stations = {{{2, {10, 0}}, TraversalTask::Maintain, 80},
                      {{1, {0, 1}}, TraversalTask::Eat, 80}};
    return scene;
}
void safe_transport() {
    for (const auto phase :
         {TransferStage::Boarding, TransferStage::Riding, TransferStage::Exiting}) {
        TraversalSession session{transport_scene()};
        for (unsigned t = 0; t < 500 && session.actors()[0].transfer != phase; ++t)
            step(session, 1);
        require(session.actors()[0].transfer == phase, "fixture did not reach transport phase");
        session.restore(changed_actor(session.serialize(), 19, "749"));
        bool finished = false;
        for (unsigned t = 0; t < 500; ++t) {
            const auto unsafe = session.actors()[0].transfer != TransferStage::None &&
                                session.actors()[0].transfer != TransferStage::Waiting;
            step(session, 1);
            if (unsafe)
                require(session.actors()[0].task == TraversalTask::Maintain,
                        "urgent need cancelled boarding, riding or exiting");
            if (session.actors()[0].transfer == TransferStage::None) {
                finished = true;
                break;
            }
        }
        require(finished, "transport failed to reach a safe bank");
        step(session, 2);
        require(session.actors()[0].task == TraversalTask::Eat,
                "urgent need did not take over on safe bank");
        if (phase == TransferStage::Riding) {
            for (unsigned t = 0; t < 500 && session.actors()[0].transfer != TransferStage::Riding;
                 ++t)
                step(session, 1);
            require(session.actors()[0].transfer == TransferStage::Riding &&
                        !session.actors()[0].suspended.empty(),
                    "fixture did not carry deferred work through return transport");
            auto clone = session;
            clone.restore(session.serialize());
            step(session, 100);
            step(clone, 100);
            require(session.serialize() == clone.serialize(),
                    "deferred task changed after restoring a committed ride");
        }
    }
}
void running_action() {
    auto scene = needs_scene();
    scene.stations[0].location = scene.spawns[0].location;
    TraversalSession session{scene};
    step(session, 2);
    const auto action = session.actors()[0].action;
    require(action.value != 0 && session.world().action(action)->state == ActionState::Running,
            "fixture did not begin a material-consuming action");
    session.restore(changed_actor(session.serialize(), 19, "749"));
    step(session, 10);
    require(session.actors()[0].action == action &&
                session.world().action(action)->state == ActionState::Running,
            "critical need cancelled a running action");
    step(session, 80);
    require(session.world().action(action)->state == ActionState::Completed &&
                session.stations()[0].completed == 1 &&
                session.world().resource(session.stations()[0].lot)->record.quantity.value == 79,
            "protected action lost cost or duplicated effect");
}
void manual_override() {
    TraversalSession session{needs_scene()};
    step(session, 780);
    require(!session.actors()[0].suspended.empty(), "fixture lacks suspended work");
    const auto original = session.serialize();
    session.set_autonomous(0, true);
    require(session.serialize() == original, "idempotent autonomy discarded pending work");
    session.set_goal(0, {1, {-30, 2}});
    require(session.actors()[0].suspended.empty() && session.actors()[0].need_retry_at == 0 &&
                session.actors()[0].finish_grace_until == 0,
            "player command retained autonomous obligations");
    step(session, 100);
    require(!session.actors()[0].autonomous && session.actors()[0].task == TraversalTask::None &&
                session.actors()[0].requested_goal == SurfaceLocation{1, {-30, 2}},
            "critical need replaced the player's exact goal");
}
void unavailable_need() {
    auto scene = needs_scene();
    scene.geometry.bodies.push_back({2, {0, 0, 20}, 0, 3, 3, .3, 0, 0, true});
    scene.stations[1].location = {2, {0, 0}};
    TraversalSession session{scene};
    step(session, 750);
    const auto plans = session.actors()[0].plans;
    step(session, 25);
    require(session.actors()[0].task == TraversalTask::Maintain && session.actors()[0].goal &&
                session.actors()[0].suspended.empty(),
            "unavailable need destroyed existing work");
    require(session.actors()[0].plans <= plans + 4, "unavailable need replanned every tick");
    require(session.world().resource(session.stations()[1].lot)->record.quantity.value == 80,
            "chooser consumed unreachable remote food");
}
std::string corrupt_autonomy(std::string bytes, std::string_view row) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto at = bytes.find("AUTONOMY ");
    require(at != std::string::npos, "missing extended autonomy record");
    bytes.replace(at, bytes.find('\n', at) - at, row);
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n';
}
void persistence() {
    TraversalSession session{needs_scene()};
    step(session, 780);
    const auto saved = session.serialize();
    require(saved.starts_with("GENESIS_TRAVERSAL 2\n"), "suspended intent saved as legacy data");
    auto loaded = session;
    loaded.restore(saved);
    step(session, 200);
    step(loaded, 200);
    require(session.serialize() == loaded.serialize(), "suspended intent continuation differs");
    const auto before = loaded.serialize();
    for (const auto row :
         {"AUTONOMY 0 0 3", "AUTONOMY 0 0 1 1 0", "AUTONOMY 0 0 1 3 16", "AUTONOMY 0 0 2 3 0 3 0",
          "AUTONOMY 18446744073709551615 0 1 3 0", "AUTONOMY 0 18446744073709551615 1 3 0"}) {
        bool rejected = false;
        try {
            loaded.restore(corrupt_autonomy(saved, row));
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected && loaded.serialize() == before,
                "invalid suspended record replaced live world");
    }
    bool grace = false;
    for (unsigned t = 0; t < 2500 && !grace; ++t) {
        step(session, 1);
        grace = session.actors()[0].finish_grace_until > session.world().clock().tick_count();
    }
    require(grace, "fixture never exercised bounded finish grace");
    loaded.restore(session.serialize());
    step(session, 200);
    step(loaded, 200);
    require(session.serialize() == loaded.serialize(), "finish grace continuation differs");
}
void safe_queue() {
    auto scene = transport_scene();
    scene.platforms[0].phase = 35;
    scene.spawns[0].location = {1, {1, 0}};
    TraversalSession session{scene};
    for (unsigned t = 0; t < 100 && session.actors()[0].transfer != TransferStage::Waiting; ++t)
        step(session, 1);
    require(session.actors()[0].transfer == TransferStage::Waiting &&
                session.navigation_view(0).transport,
            "fixture never joined a transport queue");
    session.restore(changed_actor(session.serialize(), 19, "749"));
    step(session, 1);
    require(session.actors()[0].task == TraversalTask::Eat &&
                session.actors()[0].transfer == TransferStage::None &&
                !session.navigation_view(0).transport && session.actors()[0].suspended.size() == 1,
            "critical need retained obsolete transport queue or lost deferred work");
    step(session, 20);
}
void finish_expiry() {
    auto scene = needs_scene();
    scene.stations[0].location = {1, {-15, 0}};
    TraversalSession session{scene};
    step(session, 1);
    require(session.actors()[0].task == TraversalTask::Maintain,
            "fixture lacks near maintenance intent");
    auto bytes = changed_actor(session.serialize(), 19, "749");
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto at = bytes.find("TUNING ");
    const auto end = bytes.find('\n', at);
    auto row = bytes.substr(at, end - at);
    const auto speed = row.find(" 3 ");
    require(speed != std::string::npos, "fixture lacks default speed");
    row.replace(speed, 3, " 0.05 ");
    bytes.replace(at, end - at, row);
    session.restore(bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n');
    const auto started = session.world().clock().tick_count();
    step(session, 1);
    const auto deadline = session.actors()[0].finish_grace_until;
    require(deadline == started + 120, "near finish did not receive bounded grace");
    step(session, 119);
    require(session.actors()[0].task == TraversalTask::Maintain &&
                session.actors()[0].finish_grace_until == deadline,
            "finish grace was renewed or ignored before expiry");
    step(session, 2);
    require(session.actors()[0].task == TraversalTask::Eat && session.stations()[0].completed == 0,
            "unfinished approach renewed grace instead of serving critical need");
}
void legacy_fixture() {
    const auto bytes = read_snapshot_bytes(REALITY_NEEDS_FIXTURE);
    require(bytes.starts_with("GENESIS_TRAVERSAL 1\n"), "fixture is not historical version 1");
    TraversalSession session;
    session.restore(bytes);
    require(session.serialize() == bytes, "historical version 1 bytes changed on round trip");
    for (const auto& actor : session.actors())
        require(actor.suspended.empty() && actor.need_retry_at == 0 &&
                    actor.finish_grace_until == 0,
                "legacy save invented deferred obligations");
    auto clone = session;
    clone.restore(bytes);
    step(session, 120);
    step(clone, 120);
    require(session.serialize() == clone.serialize(), "legacy continuation is not deterministic");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected needs case");
        const std::string_view name{argv[1]};
        if (name == "safe_queue")
            safe_queue();
        else if (name == "finish_expiry")
            finish_expiry();
        else if (name == "legacy_fixture")
            legacy_fixture();
        else if (name == "interruption")
            interruption();
        else if (name == "exhaustion")
            exhaustion();
        else if (name == "safe_step")
            safe_step();
        else if (name == "safe_transport")
            safe_transport();
        else if (name == "running_action")
            running_action();
        else if (name == "manual_override")
            manual_override();
        else if (name == "unavailable_need")
            unavailable_need();
        else if (name == "persistence")
            persistence();
        else
            throw std::invalid_argument("unknown needs case");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
