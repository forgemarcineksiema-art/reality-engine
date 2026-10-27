#include "graphics/lab_scene.hpp"
#include "lab/lab_session.hpp"
#include "lab/motion_session.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace reality;
using namespace std::chrono_literals;
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
template <typename Operation> void rejects(Operation operation) {
    try {
        operation();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error("expected failure did not occur");
}

void consumer_cycle() {
    LabSession session;
    require(session.reserve(0).status == RequestStatus::Accepted, "first actor reservation failed");
    require(session.reserve(1).status == RequestStatus::InsufficientResource,
            "second actor double-spent stock");
    require(session.begin(0) == Transition::Changed, "begin failed");
    session.advance(40);
    auto view = session.world().resource(session.lot()).value();
    require(view.record.quantity.value == 2 && view.reserved.value == 2 &&
                view.available.value == 0,
            "partial balance failed");
    require(session.cancel(0) == Transition::Changed, "running cancellation failed");
    view = session.world().resource(session.lot()).value();
    require(view.record.quantity.value == 2 && view.reserved.value == 0,
            "cancel refunded spent stock");
    require(session.reserve(1).status == RequestStatus::InsufficientResource,
            "remaining stock satisfied impossible request");
    session.world().validate();
    rejects([&] { return session.reserve(2); });
}

void frame_partition() {
    LabSession small, large;
    require(small.reserve(0).status == RequestStatus::Accepted &&
                large.reserve(0).status == RequestStatus::Accepted,
            "setup reserve failed");
    require(small.begin(0) == Transition::Changed && large.begin(0) == Transition::Changed,
            "setup begin failed");
    SimulationPacer first, second;
    first.update(small, 10s);
    require(small.world().clock().tick_count() == 0, "paused clock advanced");
    first.set_running(true);
    second.set_running(true);
    for (int frame = 0; frame < 500; ++frame) {
        first.update(small, 10ms);
    }
    for (int frame = 0; frame < 5; ++frame) {
        second.update(large, 1s);
    }
    require(serialize_world(small.world()) == serialize_world(large.world()),
            "frame partition changed domain outcome");
    require(small.latest_action(0)->state == ActionState::Completed &&
                small.world().clock().tick_count() == 100,
            "pacer did not finish at expected deadline");
}

void backlog_and_pause() {
    LabSession session;
    SimulationPacer pacer;
    pacer.set_running(true);
    pacer.update(session, 5025ms);
    require(session.world().clock().tick_count() == 20 && pacer.pending() == 4025ms,
            "update budget dropped elapsed time");
    pacer.set_running(false);
    pacer.update(session, 100s);
    require(pacer.pending() == 4025ms && session.world().clock().tick_count() == 20,
            "pause lost debt or accrued time");
    pacer.set_running(true);
    for (int frame = 0; frame < 4; ++frame) {
        pacer.update(session, 0ns);
    }
    require(session.world().clock().tick_count() == 100 && pacer.pending() == 25ms,
            "backlog failed to drain exactly");
    rejects([&] { pacer.update(session, -1ns); });
    const auto before = serialize_world(session.world());
    rejects([&] {
        pacer.update(session, std::chrono::nanoseconds{std::numeric_limits<std::int64_t>::max()});
    });
    require(serialize_world(session.world()) == before && pacer.pending() == 25ms,
            "rejected interval mutated pacing or world");
    // Test-only consumer injects a failure between committed ticks, not a domain success mock.
    struct FailingSession {
        LabSession lab;
        const WorkWorld& world() const { return lab.world(); }
        void advance(std::uint64_t ticks) {
            if (lab.world().clock().tick_count() == 1) {
                throw std::runtime_error("injected tick failure");
            }
            lab.advance(ticks);
        }
    } failing;
    SimulationPacer partial;
    partial.set_running(true);
    rejects([&] { partial.update(failing, 150ms); });
    require(failing.world().clock().tick_count() == 1 && partial.pending() == 100ms,
            "partially committed update lost or duplicated time");
}

void picking_and_camera() {
    LabCamera camera;
    const auto scene = build_lab_scene({}, camera, 1.5F);
    for (int object = 0; object < 3; ++object) {
        const Vec3 center = object == 0   ? Vec3{-3, 1, 0}
                            : object == 1 ? Vec3{3, 1, 0}
                                          : Vec3{0, 0.5F, 3};
        const auto point = camera.project(center, 1.5F);
        require(pick_scene(scene, point.x, point.y) == object, "visible object picking mismatch");
    }
    require(!pick_scene(scene, -0.99F, 0.99F), "empty background selected an object");
    const auto before = camera.project({3, 1, 0}, 1.5F);
    camera.orbit(0.5F);
    camera.tilt(0.2F);
    camera.zoom(-1);
    const auto after = camera.project({3, 1, 0}, 1.5F);
    require(std::abs(before.x - after.x) > 0.01F, "camera controls did not change projection");
    const auto rotated = build_lab_scene({}, camera, 1.5F);
    require(pick_scene(rotated, after.x, after.y) == 1, "picking used stale camera projection");
    rejects([&] { return camera.project({}, 0); });
    rejects([&] { camera.orbit(std::numeric_limits<float>::infinity()); });
    for (int rotation = 0; rotation < 6; ++rotation) {
        camera.orbit(.7F);
        camera.tilt(.05F);
        const auto projected = camera.project({2.5F, 0, -3.5F}, 1.5F);
        const auto ground = camera.ground_point(projected.x, projected.y, 1.5F);
        require(distance(ground, {2.5, -3.5}) < .00001,
                "ground picking disagrees with camera projection");
    }
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(0, {7, 0});
    const auto motion_scene = build_motion_scene(motion, 0, camera, 1.5F);
    require(motion_scene.size() * 3 <= 16384, "motion scene exceeds renderer capacity");
    const auto actor = camera.project({-7, .9F, 0}, 1.5F);
    require(pick_scene(motion_scene, actor.x, actor.y) == 0, "motion actor not pickable");
    for (const auto& triangle : rotated) {
        for (const auto& vertex : triangle.vertices) {
            require(std::isfinite(vertex.position.x) && vertex.position.z > 0 &&
                        vertex.position.z < 1,
                    "invalid generated geometry");
        }
    }
}

void load_protection() {
    const auto folder =
        std::filesystem::temp_directory_path() /
        ("genesis-lab-test-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    require(std::filesystem::create_directory(folder), "test directory already exists");
    struct Cleanup {
        std::filesystem::path folder;
        ~Cleanup() {
            std::error_code ignored;
            std::filesystem::remove(folder / "world.save", ignored);
            std::filesystem::remove(folder, ignored);
        }
    } cleanup{folder};
    const auto path = folder / "world.save";
    LabSession session;
    require(session.reserve(0).status == RequestStatus::Accepted, "reserve failed");
    require(session.begin(0) == Transition::Changed, "begin failed");
    session.advance(40);
    session.save(path);
    const auto before = serialize_world(session.world());
    session.advance(60);
    session.load(path);
    require(serialize_world(session.world()) == before, "running action changed during load");
    require(session.reserve(1).status == RequestStatus::InsufficientResource &&
                session.world().commands().back().request.command.value == 2,
            "loaded command counter reused id");
    const auto protected_world = serialize_world(session.world());
    WorkWorld wrong_shape;
    save_world(wrong_shape, path);
    rejects([&] { session.load(path); });
    require(serialize_world(session.world()) == protected_world,
            "foreign lab shape replaced world");
    {
        std::ofstream damaged{path};
        damaged << "invalid";
    }
    rejects([&] { session.load(path); });
    require(serialize_world(session.world()) == protected_world, "corrupt load replaced world");
}

} // namespace

int main() {
    try {
        consumer_cycle();
        std::cout << "PASS consumer_cycle\n";
        frame_partition();
        std::cout << "PASS frame_partition\n";
        backlog_and_pause();
        std::cout << "PASS backlog_and_pause\n";
        picking_and_camera();
        std::cout << "PASS picking_and_camera\n";
        load_protection();
        std::cout << "PASS load_protection\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
