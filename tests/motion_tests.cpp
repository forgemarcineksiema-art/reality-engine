#include "lab/motion_session.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <stdexcept>

namespace {
using namespace reality;
void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string{message});
    }
}
template <typename F> void rejects(F operation) {
    try {
        operation();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error("expected rejection");
}
void near(GroundPoint actual, GroundPoint expected) {
    require(distance(actual, expected) < 0.000002, "position outside two micrometre tolerance");
}
KinematicGeometry room() { return {{{-10, -10}, {10, 10}}, {}, 1}; }
void sweep() {
    auto geometry = room();
    geometry.obstacles.push_back({{{0, -8}, {.01, 8}}, 0, 3, 1});
    geometry.validate();
    const auto hit = move_kinematic(geometry, {-8, 0}, {.5, .5, 2}, {16, 0});
    near(hit.position, {-.5, 0});
    require(hit.status == MoveStatus::Contact, "thin wall tunnelled");
    const auto slide = move_kinematic(geometry, {-2, -2}, {.5, .5, 2}, {4, 4});
    near(slide.position, {-.5, 2});
    require(can_occupy(geometry, slide.position, {.5, .5, 2}), "slide penetrated wall");
    near(move_kinematic(geometry, {-.5, 0}, {.5, .5, 2}, {-1, 0}).position, {-1.5, 0});
    near(move_kinematic(geometry, {-.5, 0}, {.5, .5, 2}, {0, 1}).position, {-.5, 1});
    require(clear_segment(geometry, {-2, 0}, {-.5, 0}, {.5, .5, 2}),
            "exact endpoint contact was treated as penetration");
    const auto overlap = move_kinematic(geometry, {0, 0}, {.5, .5, 2}, {-1, 0});
    require(overlap.status == MoveStatus::InvalidStart && overlap.position == GroundPoint{0, 0},
            "overlap teleported");
    near(move_kinematic(geometry, {-2, 0}, {.5, .5, 2}, {}).position, {-2, 0});
}
void corners() {
    auto geometry = room();
    geometry.obstacles = {{{{0, -10}, {1, 10}}, 0, 3, 1}, {{{-10, 0}, {10, 1}}, 0, 3, 2}};
    const auto result = move_kinematic(geometry, {-2, -2}, {.5, .5, 2}, {5, 5});
    near(result.position, {-.5, -.5});
    require(result.contacts == 1, "simultaneous corner contacts not combined");
    const auto multiple = move_kinematic(geometry, {-3, -2}, {.5, .5, 2}, {4, 4});
    near(multiple.position, {-.5, -.5});
    require(multiple.contacts == 2, "sequential wall contacts lost");
    geometry.obstacles.clear();
    near(move_kinematic(geometry, {0, 0}, {.5, .5, 2}, {100, 20}).position, {9.5, 9.5});
    require(move_kinematic(geometry, {11, 0}, {}, {0, 0}).status == MoveStatus::InvalidStart,
            "outside start accepted");
}
void size_and_height() {
    auto geometry = room();
    geometry.obstacles = {{{{-1, -10}, {1, -.4}}, 0, 3, 1}, {{{-1, .4}, {1, 10}}, 0, 3, 2}};
    require(clear_segment(geometry, {-4, 0}, {4, 0}, {.3, .3, 1}), "small actor blocked in gap");
    require(!clear_segment(geometry, {-4, 0}, {4, 0}, {.5, .5, 1}), "wide actor passed narrow gap");
    geometry.obstacles = {{{{-1, -1}, {1, 1}}, 1.4, 2, 1}};
    require(clear_segment(geometry, {-4, 0}, {4, 0}, {.3, .3, 1.2}),
            "short actor blocked below beam");
    require(!clear_segment(geometry, {-4, 0}, {4, 0}, {.3, .3, 1.8}), "tall actor passed beam");
}
void invalid_data() {
    auto geometry = room();
    rejects([&] {
        geometry.version = 0;
        geometry.validate();
    });
    geometry = room();
    rejects([&] {
        geometry.obstacles = {{{{1, 1}, {0, 0}}, 0, 1, 1}};
        geometry.validate();
    });
    geometry = room();
    rejects([&] {
        static_cast<void>(
            move_kinematic(geometry, {0, 0}, {}, {std::numeric_limits<double>::quiet_NaN(), 0}));
    });
    rejects([&] { static_cast<void>(can_occupy(geometry, {0, 0}, {0, .2, 1})); });
    rejects([&] { static_cast<void>(find_route(geometry, {1, 65, 20}, {0, 0}, {3, 0}, {})); });
}
void route_oracle() {
    // Independent unit-cell BFS oracle. Obstacles fully fill cells; expected passability is an
    // integer mask.
    for (unsigned seed = 1; seed <= 24; ++seed) {
        KinematicGeometry geometry{{{0, 0}, {8, 8}}, {}, 1};
        std::array<bool, 64> blocked{};
        auto random = seed;
        for (unsigned i = 1; i < 63; ++i) {
            random = random * 1664525U + 1013904223U;
            if ((random >> 28) < 4) {
                blocked[i] = true;
                const double x = i % 8, z = i / 8;
                geometry.obstacles.push_back({{{x, z}, {x + 1, z + 1}}, 0, 2, i});
            }
        }
        std::array<int, 64> lengths;
        lengths.fill(-1);
        lengths[0] = 0;
        std::queue<unsigned> queue;
        queue.push(0);
        while (!queue.empty()) {
            const auto node = queue.front();
            queue.pop();
            const auto visit = [&](unsigned next) {
                if (!blocked[next] && lengths[next] < 0) {
                    lengths[next] = lengths[node] + 1;
                    queue.push(next);
                }
            };
            if (node % 8 > 0) {
                visit(node - 1);
            }
            if (node % 8 < 7) {
                visit(node + 1);
            }
            if (node >= 8) {
                visit(node - 8);
            }
            if (node < 56) {
                visit(node + 8);
            }
        }
        const auto route = find_route(geometry, {1, 8, 8}, {.5, .5}, {7.5, 7.5}, {.2, .2, 1});
        require((route.status == RouteStatus::Found) == (lengths[63] >= 0),
                "A* reachability disagrees with BFS, 8x8 seed=" + std::to_string(seed));
        if (lengths[63] >= 0) {
            require(route.points.size() == static_cast<std::size_t>(lengths[63] + 1),
                    "A* route is not shortest, 8x8 seed=" + std::to_string(seed));
            auto prior = route.start;
            for (const auto next : route.points) {
                require(clear_segment(geometry, prior, next, route.size),
                        "route crosses collision");
                prior = next;
            }
        }
    }
}
void route_failures() {
    auto geometry = room();
    NavigationGrid grid{1, 20, 20};
    const auto already_there = find_route(geometry, grid, {-5, 0}, {-5, 0}, {}, 0);
    require(already_there.status == RouteStatus::Found &&
                already_there.points == std::vector<GroundPoint>{{-5, 0}} &&
                already_there.expanded == 0,
            "zero-distance route invented a trip");
    require(find_route(geometry, grid, {-5, 0}, {5, 0}, {}, 0).status ==
                RouteStatus::BudgetExceeded,
            "budget exhaustion hidden");
    require(find_route(geometry, grid, {-11, 0}, {5, 0}, {}).status == RouteStatus::InvalidEndpoint,
            "invalid endpoint accepted");
    geometry.obstacles = {{{{0, -10}, {.1, 10}}, 0, 3, 1}};
    require(find_route(geometry, grid, {-5, 0}, {5, 0}, {}).status == RouteStatus::NoPath,
            "sealed wall has a route");
    geometry.obstacles = {{{{-.2, -10}, {.2, -.4}}, 0, 3, 1}, {{{-.2, .4}, {.2, 10}}, 0, 3, 2}};
    const NavigationGrid fine{.5, 40, 40};
    require(find_route(geometry, fine, {-5, .25}, {5, .25}, {.1, .1, 1}).status ==
                RouteStatus::Found,
            "narrow actor cannot route through gap");
    require(find_route(geometry, fine, {-5, .25}, {5, .25}, {.5, .5, 1}).status ==
                RouteStatus::NoPath,
            "wide actor route passes gap");
    geometry.obstacles = {{{{-.2, -10}, {.2, 10}}, 1.4, 2, 1}};
    require(find_route(geometry, grid, {-5, 0}, {5, 0}, {.2, .2, 1.2}).status == RouteStatus::Found,
            "short actor route ignores valid clearance");
    require(find_route(geometry, grid, {-5, 0}, {5, 0}, {.2, .2, 1.8}).status ==
                RouteStatus::NoPath,
            "tall actor route crosses beam");
}
void route_stamps() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(0, {7, 0});
    auto route = find_route(motion.geometry(), {}, motion.actors()[0].position, {7, 0},
                            motion.actors()[0].size);
    require(motion.accept_route(0, route), "matching route rejected");
    auto wrong = route;
    wrong.size.height_m = 1;
    require(!motion.accept_route(0, wrong), "wrong shape route accepted");
    wrong = route;
    wrong.start.x_m += 1;
    require(!motion.accept_route(0, wrong), "wrong start route accepted");
    wrong = route;
    wrong.expanded = 4097;
    require(!motion.accept_route(0, wrong), "unbounded route metadata accepted");
    wrong = route;
    wrong.points = {{0, -4}, {7, 0}};
    require(!motion.accept_route(0, wrong), "unvalidated path accepted");
    require(motion.set_gate(0, true), "gate change failed");
    require(!motion.accept_route(0, route), "stale route accepted");
}
void detour_and_wait() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(0, {7, 0});
    motion.advance(10);
    const auto plans = motion.actors()[0].plans;
    require(motion.set_gate(0, true), "main gate close failed");
    motion.advance(1);
    require(motion.actors()[0].plans == plans + 1,
            "geometry change did not replan before movement");
    bool detoured = false;
    for (int i = 0; i < 300 && motion.actors()[0].state != TravelState::Arrived; ++i) {
        motion.advance(1);
        detoured = detoured || motion.actors()[0].position.z_m > 5;
        require(can_occupy(motion.geometry(), motion.actors()[0].position, motion.actors()[0].size),
                "follower penetrated geometry");
    }
    require(detoured && motion.actors()[0].state == TravelState::Arrived,
            "actor did not arrive via detour");
    near(motion.actors()[0].position, {7, 0});
    require(motion.set_gate(1, true), "detour gate close failed after arrival");
    motion.advance(1);
    require(motion.actors()[0].position == GroundPoint{7, 0} &&
                motion.actors()[0].state == TravelState::Arrived,
            "geometry change moved an already arrived actor");
    LabSession other_lab;
    MotionSession sealed{other_lab};
    require(sealed.set_gate(0, true) && sealed.set_gate(1, true), "seal failed");
    sealed.set_goal(0, {7, 0});
    const auto before = sealed.actors()[0].position;
    sealed.advance(50);
    require(sealed.actors()[0].state == TravelState::NoPath &&
                sealed.actors()[0].position == before,
            "no path moved actor");
    require(sealed.set_gate(1, false), "reopen failed");
    sealed.advance(300);
    require(sealed.actors()[0].state == TravelState::Arrived, "retained goal did not resume");
}
void gate_occupancy() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_input(0, {1, 0});
    motion.advance(46);
    motion.clear_input();
    require(std::abs(motion.actors()[0].position.x_m) < .2, "actor is not in gate");
    const auto before = motion.serialize();
    require(!motion.set_gate(0, true), "gate closed on actor");
    require(before == motion.serialize(), "rejected gate changed state");
}
void input_and_collisions() {
    LabSession lab;
    MotionSession motion{lab};
    motion.queue_input(0, {1, 0});
    motion.set_input(0, {});
    motion.advance(0);
    near(motion.actors()[0].position, {-7, 0});
    motion.advance(1);
    near(motion.actors()[0].position, {-6.85, 0});
    motion.advance(5);
    near(motion.actors()[0].position, {-6.85, 0});
    // Reset through persistence; transient key pulses must not survive a load.
    LabSession fresh;
    MotionSession initial{fresh};
    motion.restore(initial.serialize());
    motion.set_input(0, {1, 1});
    motion.advance(1);
    require(std::abs(distance(motion.actors()[0].position, {-7, 0}) - .15) < 1e-9,
            "diagonal input changes speed");
    motion.clear_input();
    const auto before = motion.actors()[0].position;
    motion.advance(5);
    require(before == motion.actors()[0].position, "cleared input continued moving");
    LabSession second;
    MotionSession collision{second};
    collision.set_input(1, {0, 1});
    collision.advance(50);
    require(!motion.interaction_target(0), "distant actor was interactable");
    require(collision.actors()[1].position.z_m < -.59 && collision.actors()[1].position.z_m > -.61,
            "mobile bodies passed through each other");
    require(collision.interaction_target(1) == collision.actors()[0].id,
            "nearby actor interaction failed");
    collision.set_goal(0, {7, 0});
    collision.set_input(0, {1, 0});
    collision.advance(1);
    require(!collision.actors()[0].goal, "manual movement did not cancel route");
}
void persistence() {
    LabSession first_lab, second_lab;
    MotionSession first{first_lab}, second{second_lab};
    require(first_lab.reserve(0).status == RequestStatus::Accepted &&
                first_lab.begin(0) == Transition::Changed,
            "work setup failed");
    first.set_goal(0, {7, 0});
    first.advance(10);
    require(first.set_gate(0, true), "gate failed");
    const auto stale = first.serialize();
    second.restore(stale);
    require(stale == second.serialize(), "stale route not restored exactly");
    first.advance(40);
    second.advance(40);
    require(first.serialize() == second.serialize(), "running continuation diverged");
    const auto fresh = first.serialize();
    second.restore(fresh);
    first.advance(250);
    second.advance(250);
    require(first.serialize() == second.serialize() &&
                first.actors()[0].state == TravelState::Arrived &&
                first_lab.latest_action(0)->state == ActionState::Completed,
            "movement and work continuation diverged");
    const auto protected_state = second.serialize();
    auto corrupt = fresh;
    corrupt[0] = 'X';
    rejects([&] { second.restore(corrupt); });
    rejects([&] { second.restore(serialize_world(first_lab.world())); });
    // Valid checksum, invalid gate semantics: the parser must reject more than accidental byte
    // damage.
    corrupt = stale;
    const auto nested_end = corrupt.find("\n2 1 0\n");
    require(nested_end != std::string::npos, "fixture gate line missing");
    corrupt.replace(nested_end + 1, 5, "2 9 0");
    const auto marker = corrupt.rfind("CHECKSUM ");
    corrupt = corrupt.substr(0, marker) + "CHECKSUM " +
              std::to_string(snapshot_checksum(corrupt.substr(0, marker))) + "\n";
    rejects([&] { second.restore(corrupt); });
    require(second.serialize() == protected_state, "failed load partly replaced world");
    const auto resign = [](std::string bytes) {
        const auto end = bytes.rfind("CHECKSUM ");
        const auto body = bytes.substr(0, end);
        return body + "CHECKSUM " + std::to_string(snapshot_checksum(body)) + "\n";
    };
    corrupt = stale;
    corrupt.replace(0, std::string{"GENESIS_MOTION 1"}.size(), "GENESIS_MOTION 2");
    rejects([&] { second.restore(resign(corrupt)); });
    corrupt = stale;
    const auto actor_line = nested_end + 7;
    auto prefix_end = actor_line;
    for (int field = 0; field < 3; ++field) {
        prefix_end = corrupt.find(' ', prefix_end) + 1;
    }
    corrupt.replace(actor_line, prefix_end - actor_line, "1 0 0 ");
    rejects([&] { second.restore(resign(corrupt)); });
    require(second.serialize() == protected_state, "semantic or version failure replaced world");
}
void partition() {
    LabSession a, b;
    MotionSession first{a}, second{b};
    first.set_goal(0, {7, 0});
    second.set_goal(0, {7, 0});
    SimulationPacer p, q;
    p.set_running(true);
    q.set_running(true);
    for (int i = 0; i < 500; ++i) {
        p.update(first, std::chrono::milliseconds{10});
    }
    for (int i = 0; i < 5; ++i) {
        q.update(second, std::chrono::seconds{1});
    }
    require(first.serialize() == second.serialize(), "frame partition changes movement");
    const auto prior = first.serialize();
    rejects([&] { first.advance(std::numeric_limits<std::uint64_t>::max()); });
    require(prior == first.serialize(), "overflow partially moved world");
}

void waiting_continuation() {
    LabSession a, b;
    MotionSession first{a}, second{b};
    require(first.set_gate(0, true) && first.set_gate(1, true), "sealing failed");
    first.set_goal(0, {7, 0});
    first.advance(10);
    second.restore(first.serialize());
    first.advance(5);
    second.advance(5);
    require(first.serialize() == second.serialize() &&
                first.actors()[0].state == TravelState::NoPath,
            "no-path wait did not survive load");
    require(first.set_gate(0, false), "opening failed");
    first.set_goal(0, {-7, 0});
    first.set_goal(1, {-7, 0});
    first.advance(60);
    require(first.actors()[1].state == TravelState::Waiting,
            "local collision did not enter waiting state");
    second.restore(first.serialize());
    const auto stopped = first.actors()[1].position;
    first.advance(5);
    second.advance(5);
    require(first.serialize() == second.serialize() && first.actors()[1].position == stopped,
            "local collision wait changed after load");
    first.set_input(0, {1, 0});
    second.set_input(0, {1, 0});
    first.advance(40);
    second.advance(40);
    require(first.serialize() == second.serialize() &&
                first.actors()[1].state == TravelState::Arrived,
            "local follower did not resume when obstacle moved");
}

double route_length(const GroundRoute& route) {
    auto previous = route.start;
    double length = 0;
    for (const auto next : route.points) {
        length += distance(previous, next);
        previous = next;
    }
    return length;
}
void direct_goal() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(0, {7, 0});
    const auto& route = motion.actors()[0].route;
    std::cout << "open_route_points=" << route.points.size() << " length_m=" << route_length(route)
              << '\n';
    require(route.points == std::vector<GroundPoint>{{7, 0}},
            "open route still visits cell centres");
    require(std::abs(route_length(route) - 14) < 1e-9,
            "open route is not the direct 14 metre segment");
    motion.advance(1);
    near(motion.actors()[0].position, {-6.85, 0});
}
void retarget() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(0, {7, 0});
    motion.advance(7);
    const auto before = motion.actors()[0].position;
    motion.set_goal(0, {7, -1});
    const auto& route = motion.actors()[0].route;
    require(motion.actors()[0].position == before && route.start == before,
            "new goal changed position or used an old start");
    const auto snapshot = motion.serialize();
    const auto plans = motion.actors()[0].plans;
    motion.set_goal(0, {7, -1});
    require(motion.serialize() == snapshot && motion.actors()[0].plans == plans,
            "identical goal resets travel progress");
    const auto length = distance(before, {7, -1});
    motion.advance(1);
    near(motion.actors()[0].position, {before.x_m + (7 - before.x_m) * .15 / length,
                                       before.z_m + (-1 - before.z_m) * .15 / length});
}
void replan_forward() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(0, {7, 0});
    motion.advance(7);
    const auto before = motion.actors()[0].position;
    const auto plans = motion.actors()[0].plans;
    require(motion.set_gate(1, true), "detour gate toggle failed");
    motion.advance(1);
    require(motion.actors()[0].plans == plans + 1, "geometry did not trigger a new plan");
    require(motion.actors()[0].position.x_m > before.x_m,
            "replan moved backwards to a cell centre");
    near(motion.actors()[0].position, {before.x_m + .15, before.z_m});
}
void arrival_stability() {
    LabSession lab;
    MotionSession motion{lab};
    motion.set_goal(0, {-3, -1});
    motion.advance(100);
    require(motion.actors()[0].state == TravelState::Arrived, "open goal did not finish");
    near(motion.actors()[0].position, {-3, -1});
    const auto stopped = motion.actors()[0].position;
    const auto points = motion.actors()[0].next_point;
    const auto plans = motion.actors()[0].plans;
    motion.advance(100);
    require(motion.actors()[0].position == stopped && motion.actors()[0].next_point == points &&
                motion.actors()[0].plans == plans,
            "arrived actor kept walking or planning");
    motion.set_goal(0, {0, -4});
    const auto blocked = motion.actors()[0].position;
    motion.advance(100);
    require(motion.actors()[0].state == TravelState::InvalidEndpoint &&
                motion.actors()[0].position == blocked,
            "unreachable endpoint was silently reached or caused drift");
}

void shortcut_geometry() {
    auto geometry = room();
    geometry.obstacles = {{{{0, -3}, {1, 3}}, 0, 2.5, 1}};
    const NavigationGrid grid{1, 20, 20};
    const BodySize size{.3, .3, 1.8};
    const auto raw = find_route(geometry, grid, {-5, 0}, {5, 0}, size);
    const auto shortened = find_motion_route(geometry, grid, {-5, 0}, {5, 0}, size);
    require(find_motion_route(geometry, grid, {-5, 0}, {5, 0}, size, 0).status ==
                RouteStatus::BudgetExceeded,
            "motion search hid its exhausted A* budget");
    require(raw.status == RouteStatus::Found && shortened.status == RouteStatus::Found,
            "wall route missing");
    require(shortened.points.size() < raw.points.size() &&
                route_length(shortened) < route_length(raw),
            "shortcut did not remove redundant corners");
    auto previous = shortened.start;
    for (const auto next : shortened.points) {
        require(clear_segment(geometry, previous, next, size), "shortcut cuts a corner");
        if ((previous.x_m < .5 && next.x_m >= .5) || (next.x_m < .5 && previous.x_m >= .5)) {
            const auto fraction = (.5 - previous.x_m) / (next.x_m - previous.x_m);
            const auto z = previous.z_m + (next.z_m - previous.z_m) * fraction;
            require(std::abs(z) >= 3.3,
                    "independent wall-crossing check caught a cut through the expanded wall");
        }
        previous = next;
    }
    for (const std::size_t budget : {std::size_t{0}, std::size_t{1}, std::size_t{3}}) {
        const auto limited = find_motion_route(geometry, grid, {-5, 0}, {5, 0}, size, 4096, budget);
        require(limited.status == RouteStatus::Found &&
                    limited.points.back() == GroundPoint{5, 0} &&
                    route_length(limited) <= route_length(raw) + 1e-9,
                "shortcut budget discarded or lengthened a valid route");
        previous = limited.start;
        for (const auto next : limited.points) {
            require(clear_segment(geometry, previous, next, size),
                    "budget fallback lost a checked edge");
            previous = next;
        }
        if (budget == 0) {
            require(limited.points == raw.points, "zero shortcut budget changed raw route");
        }
    }
    geometry.obstacles = {{{{-.01, -10}, {.01, -.4}}, 0, 2, 1}, {{{-.01, .4}, {.01, 10}}, 0, 2, 2}};
    require(find_motion_route(geometry, grid, {-5, 0}, {5, 0}, {.2, .2, 1}).points ==
                std::vector<GroundPoint>{{5, 0}},
            "small body cannot take clear gap");
    require(find_motion_route(geometry, grid, {-5, 0}, {5, 0}, {.5, .5, 1}).status ==
                RouteStatus::NoPath,
            "shortcut sent large body through narrow gap");
    geometry.obstacles = {{{{-.01, -10}, {.01, 10}}, 1.4, 2, 1}};
    require(find_motion_route(geometry, grid, {-5, 0}, {5, 0}, {.2, .2, 1.2}).points ==
                std::vector<GroundPoint>{{5, 0}},
            "short body cannot pass under beam");
    require(find_motion_route(geometry, grid, {-5, 0}, {5, 0}, {.2, .2, 1.8}).status ==
                RouteStatus::NoPath,
            "shortcut ignored body height");
}
void retarget_continuation() {
    LabSession a, b;
    MotionSession first{a}, second{b};
    first.set_goal(0, {7, 0});
    first.advance(7);
    first.set_goal(0, {6, -1});
    first.advance(11);
    second.restore(first.serialize());
    first.advance(200);
    second.advance(200);
    require(first.serialize() == second.serialize() &&
                first.actors()[0].state == TravelState::Arrived,
            "new goal continuation diverged");
    near(first.actors()[0].position, {6, -1});
    LabSession c, d;
    MotionSession blocked{c}, restored{d};
    blocked.set_goal(1, {-7, 0});
    blocked.advance(60);
    require(blocked.actors()[1].state == TravelState::Waiting, "blocked follower did not wait");
    const auto position = blocked.actors()[1].position;
    restored.restore(blocked.serialize());
    blocked.advance(100);
    restored.advance(100);
    require(blocked.actors()[1].position == position && blocked.serialize() == restored.serialize(),
            "blocked follower jittered or diverged after load");
    blocked.set_goal(1, {-4, -2});
    restored.set_goal(1, {-4, -2});
    blocked.advance(100);
    restored.advance(100);
    require(blocked.serialize() == restored.serialize() &&
                blocked.actors()[1].state == TravelState::Arrived,
            "new goal could not release waiting follower");
}
} // namespace
int main(int argc, char** argv) {
    try {
        const std::map<std::string, std::function<void()>> cases{
            {"sweep", sweep},
            {"corners", corners},
            {"size_height", size_and_height},
            {"invalid_data", invalid_data},
            {"route_oracle", route_oracle},
            {"route_failures", route_failures},
            {"route_stamps", route_stamps},
            {"detour_wait", detour_and_wait},
            {"gate_occupancy", gate_occupancy},
            {"input_collision", input_and_collisions},
            {"persistence", persistence},
            {"partition", partition},
            {"waiting_continuation", waiting_continuation},
            {"direct_goal", direct_goal},
            {"retarget", retarget},
            {"replan_forward", replan_forward},
            {"arrival_stability", arrival_stability},
            {"shortcut_geometry", shortcut_geometry},
            {"retarget_continuation", retarget_continuation}};
        if (argc != 3 || std::string{argv[1]} != "--case" || !cases.contains(argv[2])) {
            throw std::invalid_argument("unknown test case");
        }
        cases.at(argv[2])();
        std::cout << "PASS " << argv[2] << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
