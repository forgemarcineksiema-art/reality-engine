#include "graphics/lab_scene.hpp"
#include "lab/motion_session.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
using namespace reality;
void require(bool valid, const char* message) {
    if (!valid) {
        throw std::runtime_error(message);
    }
}
void wall_retarget() {
    LabSession lab;
    MotionSession motion{lab};
    require(motion.set_gate(0, true), "close gate failed");
    motion.set_input(0, {1, 0});
    motion.advance(60);
    motion.clear_input();
    const auto start = motion.actors()[0].position;
    require(can_occupy(motion.geometry(), start, motion.actors()[0].size),
            "contact position is invalid");
    motion.set_goal(0, {7, 0});
    std::cout << "contact_x=" << start.x_m
              << " route_status=" << static_cast<unsigned>(motion.actors()[0].route.status) << '\n';
    require(motion.actors()[0].route.status == RouteStatus::Found,
            "valid contact position cannot attach to navigation grid");
    motion.advance(300);
    require(motion.actors()[0].state == TravelState::Arrived &&
                distance(motion.actors()[0].position, {7, 0}) < 1e-8,
            "retargeted contact actor did not use detour");
}
void wall_goal() {
    KinematicGeometry geometry{{{-10, -10}, {10, 10}}, {{{{-.25, -3}, {.25, 3}}, 0, 3, 1}}, 1};
    const auto route = find_motion_route(geometry, {1, 20, 20}, {5, 0}, {-.61, 0}, {.35, .35, 1.8});
    require(route.status == RouteStatus::Found,
            "valid endpoint beside wall rejected because its cell centre is blocked");
}
void stationary_blocker() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(1, {-7, 3});
    motion.advance(300);
    require(motion.actors()[1].state == TravelState::Arrived, "open floor route setup failed");
    require(distance(motion.actors()[1].position, {-7, 3}) < 1e-8,
            "did not walk around stationary actor A");
}
void extreme_span() {
    KinematicGeometry geometry{{{-1000000, -10}, {1000000, 10}}, {}, 1};
    geometry.validate();
    require(clear_segment(geometry, {-900000, 0}, {900000, 0}, {}),
            "valid world span rejected as an invalid displacement");
}
template <typename F> void rejects(F operation) {
    try {
        operation();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error("invalid input was accepted");
}
void interpolation() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_input(0, {1, 0});
    motion.advance(1);
    motion.clear_input();
    const auto snapshot = motion.serialize();
    for (int frame = 0; frame <= 30; ++frame) {
        const auto fraction = static_cast<double>(frame) / 30;
        const auto pose = motion.presentation_positions(fraction)[0];
        require(distance(pose, {-7 + .15 * fraction, 0}) < 1e-9,
                "render position is not interpolated within a fixed step");
    }
    require(motion.serialize() == snapshot, "presentation changed authoritative world");
    rejects([&] { static_cast<void>(motion.presentation_positions(-.1)); });
    rejects([&] {
        static_cast<void>(motion.presentation_positions(std::numeric_limits<double>::quiet_NaN()));
    });
    motion.synchronize_presentation();
    require(motion.presentation_positions(0)[0] == motion.actors()[0].position,
            "pause resumed stale history");
    motion.restore(snapshot);
    require(motion.presentation_positions(.5)[0] == motion.actors()[0].position,
            "load interpolated from unrelated history");
}
void corner_image() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(0, {-5.30, 1.65});
    motion.advance(100);
    const auto start = motion.actors()[0].position;
    const GroundPoint goal{-5.35, 1.75};
    motion.set_goal(0, goal);
    GroundRoute route{RouteStatus::Found,
                      motion.geometry().version,
                      motion.actors()[0].size,
                      start,
                      goal,
                      {{-5.35, 1.65}, goal},
                      0};
    require(motion.accept_route(0, route), "valid corner route fixture rejected");
    motion.advance(1);
    const auto end = motion.actors()[0].position;
    const GroundPoint chord{(start.x_m + end.x_m) / 2, (start.z_m + end.z_m) / 2};
    require(!can_occupy(motion.geometry(), chord, motion.actors()[0].size),
            "fixture no longer distinguishes chord from actual corner");
    for (int frame = 0; frame <= 30; ++frame) {
        require(can_occupy(motion.geometry(),
                           motion.presentation_positions(static_cast<double>(frame) / 30)[0],
                           motion.actors()[0].size),
                "interpolated image cuts through expanded corner");
    }
}
void rapid_targets() {
    LabSession a, b;
    MotionSession first{a}, second{b};
    require(first.set_gate(0, true), "close failed");
    first.set_input(0, {1, 0});
    first.advance(60);
    first.clear_input();
    second.restore(first.serialize());
    const std::array<GroundPoint, 5> goals{{{7, 0}, {-7, -5}, {-1, 5}, {8, 6}, {-7, 0}}};
    for (unsigned operation = 0; operation < 200; ++operation) {
        const auto goal = goals[operation % goals.size()];
        first.set_goal(0, goal);
        second.set_goal(0, goal);
        const auto position = first.actors()[0].position;
        first.set_goal(0, goal);
        require(first.actors()[0].position == position, "rapid retarget teleported actor");
        first.advance(1);
        second.advance(1);
        require(first.serialize() == second.serialize(),
                "rapid retargets diverged from save continuation");
        require(can_occupy(first.geometry(), first.actors()[0].position, first.actors()[0].size),
                "rapid target entered wall");
        if (operation % 17 == 0) {
            second.restore(first.serialize());
        }
    }
    first.set_goal(0, {7, 0});
    first.advance(400);
    require(first.actors()[0].state == TravelState::Arrived, "last rapid target left actor stuck");
}
void passing_case(double destination) {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(1, {7, 0});
    motion.advance(300);
    require(motion.actors()[1].state == TravelState::Arrived, "passing setup failed");
    motion.set_goal(0, {destination, 0});
    motion.set_goal(1, {-destination, 0});
    LabSession copy_lab;
    MotionSession copy{copy_lab};
    copy.restore(motion.serialize());
    for (int tick = 0; tick < 400; ++tick) {
        motion.advance(1);
        copy.advance(1);
        require(motion.serialize() == copy.serialize(), "passing diverged after save continuation");
        if (tick % 23 == 0) {
            copy.restore(motion.serialize());
        }
        for (const double fraction : {0.0, .25, .5, .75, 1.0}) {
            const auto poses = motion.presentation_positions(fraction);
            const auto a = motion.actors()[0].size, b = motion.actors()[1].size;
            require(std::abs(poses[0].x_m - poses[1].x_m) >= a.half_x_m + b.half_x_m - 1e-9 ||
                        std::abs(poses[0].z_m - poses[1].z_m) >= a.half_z_m + b.half_z_m - 1e-9,
                    "displayed bodies overlap while passing");
            for (std::size_t actor = 0; actor < 2; ++actor) {
                require(can_occupy(motion.geometry(), poses[actor], motion.actors()[actor].size),
                        "passing image entered static geometry");
            }
        }
    }
    require(motion.actors()[0].state == TravelState::Arrived &&
                motion.actors()[1].state == TravelState::Arrived,
            "head-on agents failed to pass on open floor");
}
void passing() { passing_case(8); }
void goal_swap() { passing_case(7); }
bool independently_free(const KinematicGeometry& geometry, GroundPoint p, BodySize s) {
    if (p.x_m - s.half_x_m < geometry.bounds.minimum.x_m - 1e-9 ||
        p.x_m + s.half_x_m > geometry.bounds.maximum.x_m + 1e-9 ||
        p.z_m - s.half_z_m < geometry.bounds.minimum.z_m - 1e-9 ||
        p.z_m + s.half_z_m > geometry.bounds.maximum.z_m + 1e-9) {
        return false;
    }
    for (const auto& obstacle : geometry.obstacles) {
        if (obstacle.bottom_m >= s.height_m) {
            continue;
        }
        const auto& r = obstacle.footprint;
        if (std::max(p.x_m - s.half_x_m, r.minimum.x_m) <
                std::min(p.x_m + s.half_x_m, r.maximum.x_m) - 1e-9 &&
            std::max(p.z_m - s.half_z_m, r.minimum.z_m) <
                std::min(p.z_m + s.half_z_m, r.maximum.z_m) - 1e-9) {
            return false;
        }
    }
    return true;
}
void collision_sequences() {
    KinematicGeometry geometry{
        {{-10, -10}, {10, 10}}, {{{{-2, -4}, {0, 3}}, 0, 3, 1}, {{{2, -1}, {4, 5}}, 0, 3, 2}}, 1};
    geometry.validate();
    const BodySize size{.3, .4, 1.8};
    GroundPoint position{-7, 0};
    std::uint32_t random = 0x4d303341;
    const auto number = [&]() {
        random = random * 1664525U + 1013904223U;
        return (static_cast<double>(random >> 8) / 16777216 - .5) * 40;
    };
    std::cout << "seed=0x4d303341 operations=1000 samples_per_segment=17\n";
    for (int operation = 0; operation < 1000; ++operation) {
        const GroundPoint delta{number(), number()};
        const auto result = move_kinematic(geometry, position, size, delta);
        require(result.status != MoveStatus::InvalidStart && result.sample_count >= 2 &&
                    result.sample_count <= 6,
                "sweep trace count invalid");
        require(result.samples[0].fraction == 0 &&
                    result.samples[result.sample_count - 1].fraction == 1,
                "sweep trace does not cover full step");
        double travelled = 0;
        for (std::size_t sample = 1; sample < result.sample_count; ++sample) {
            const auto from = result.samples[sample - 1], to = result.samples[sample];
            require(to.fraction > from.fraction, "sweep time moved backwards");
            travelled += distance(from.position, to.position);
            for (int frame = 0; frame <= 16; ++frame) {
                const auto fraction = static_cast<double>(frame) / 16;
                const GroundPoint p{
                    from.position.x_m + (to.position.x_m - from.position.x_m) * fraction,
                    from.position.z_m + (to.position.z_m - from.position.z_m) * fraction};
                if (!independently_free(geometry, p, size)) {
                    throw std::runtime_error("seed 0x4d303341 penetration at operation " +
                                             std::to_string(operation));
                }
            }
        }
        require(travelled <= std::hypot(delta.x_m, delta.z_m) + 1e-8,
                "collision created movement distance");
        position = result.position;
    }
}
void input_limits() {
    KinematicGeometry tiny{{{0, 0}, {1e-10, 1e-10}}, {}, 1};
    rejects([&] { NavigationGrid{1e-300, 20, 20}.validate(tiny); });
    KinematicGeometry translated{{{999999.98, 0}, {1000000, .02}}, {}, 1};
    translated.validate();
    NavigationGrid{.001, 20, 20}.validate(translated);
    rejects(
        [&] { NavigationGrid{std::numeric_limits<double>::max(), 64, 64}.validate(translated); });
    LabSession lab;
    MotionSession motion{lab};
    const auto prior = motion.serialize();
    rejects([&] { motion.set_goal(0, {std::numeric_limits<double>::infinity(), 0}); });
    require(motion.serialize() == prior, "invalid goal partly changed world");
}
void legacy_endpoint() {
    LabSession lab;
    MotionSession motion{lab};
    require(motion.set_gate(0, true), "close failed");
    motion.set_goal(0, {0, 0});
    auto bytes = motion.serialize();
    const auto geometry_line = bytes.find("\n2 1 0\n");
    require(geometry_line != std::string::npos, "legacy fixture geometry missing");
    const auto begin = geometry_line + 7, end = bytes.find('\n', begin);
    std::istringstream input{bytes.substr(begin, end - begin)};
    std::vector<std::string> fields;
    std::string token;
    while (input >> token) {
        fields.push_back(token);
    }
    require(fields.size() == 23, "legacy fixture schema changed");
    // Synthetic version-1 record of the former centre-attachment failure, explicitly not
    // evidence generated by the old executable. The goal and contact position are legal.
    fields[1] = "-0.600001";
    fields[7] = "7";
    fields[17] = "-0.600001";
    fields[19] = "7";
    std::string line;
    for (const auto& field : fields) {
        line += field + ' ';
    }
    bytes.replace(begin, end - begin, line);
    const auto checksum = bytes.rfind("CHECKSUM ");
    const auto body = bytes.substr(0, checksum);
    bytes = body + "CHECKSUM " + std::to_string(snapshot_checksum(body)) + "\n";
    motion.restore(bytes);
    motion.set_goal(0, {7, 0});
    require(motion.actors()[0].state == TravelState::Following,
            "identical goal preserved obsolete invalid endpoint");
    motion.restore(bytes);
    motion.advance(300);
    require(motion.actors()[0].state == TravelState::Arrived,
            "loaded obsolete endpoint did not recover on continuation");
}
void contact_rounding() {
    KinematicGeometry geometry{{{-10, -10}, {10, 10}}, {{{{-.25, -3}, {.25, 3}}, 0, 3, 1}}, 1};
    geometry.validate();
    const BodySize size{.35, .35, 1.8};
    std::uint32_t random = 0x636f6e74;
    const double target = geometry.obstacles[0].footprint.minimum.x_m - size.half_x_m;
    for (int operation = 0; operation < 1000; ++operation) {
        random = random * 1664525U + 1013904223U;
        const GroundPoint start{-9 + static_cast<double>(random >> 8) / 16777216 * 8, -1};
        require(clear_segment(geometry, start, {target, .7}, size),
                "legal endpoint contact is not a clear segment");
        const auto result = move_kinematic(geometry, start, size, {target - start.x_m, 1.7});
        require(distance(result.position, {target, .7}) < 1e-9,
                "endpoint contact changed tangential movement");
        if (!can_occupy(geometry, result.position, size)) {
            throw std::runtime_error("rounding penetrated contact, seed 0x636f6e74 operation " +
                                     std::to_string(operation));
        }
        require(move_kinematic(geometry, result.position, size, {-1, 0}).status !=
                    MoveStatus::InvalidStart,
                "computed contact poisoned the next movement");
    }
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string{argv[1]} == "--contact-fixture") {
            LabSession lab;
            MotionSession motion{lab};
            require(motion.set_gate(0, true), "fixture gate failed");
            motion.set_input(0, {1, 0});
            motion.advance(60);
            motion.clear_input();
            motion.save(std::filesystem::path{argv[2]});
            std::cout << motion.inspect();
            return 0;
        }
        const std::map<std::string, std::function<void()>> cases{
            {"wall_retarget", wall_retarget},
            {"wall_goal", wall_goal},
            {"stationary_blocker", stationary_blocker},
            {"extreme_span", extreme_span},
            {"interpolation", interpolation},
            {"corner_image", corner_image},
            {"rapid_targets", rapid_targets},
            {"passing", passing},
            {"goal_swap", goal_swap},
            {"collision_sequences", collision_sequences},
            {"input_limits", input_limits},
            {"legacy_endpoint", legacy_endpoint},
            {"contact_rounding", contact_rounding}};
        if (argc != 3 || std::string{argv[1]} != "--case" || !cases.contains(argv[2])) {
            throw std::invalid_argument("unknown adversarial case");
        }
        cases.at(argv[2])();
        std::cout << "PASS " << argv[2] << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
