#include "lab/lab_session.hpp"
#include "lab/traversal_diagnostics.hpp"
#include "lab/traversal_session.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace reality;
namespace {
std::string edited_row(std::string bytes, std::string_view section, std::size_t field,
                       std::string value);
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
TraversalScene floor() {
    TraversalScene s;
    s.geometry.bodies = {{1, {}, 0, 8, 5, .3, 0, 0, true}};
    s.spawns = {{{1, {-3, 0}}, {}, false}};
    return s;
}
void image_check(const TraversalSession& s) {
    std::vector<TraversalPose> previous;
    for (const auto t : {0., .25, .5, .75, 1.}) {
        const auto poses = s.presentation(t);
        const auto geometry = s.presentation_geometry(t);
        for (std::size_t i = 0; i < poses.size(); ++i) {
            // These fixtures bound walking plus carry below 6 m/s: 75 mm per quarter tick.
            if (!previous.empty())
                require(length(poses[i].feet - previous[i].feet) <= .075001,
                        "presentation jumps between quarter-tick samples");
            require(spatial_occupiable(geometry, poses[i].feet, s.actors()[i].size),
                    "presentation penetrates geometry");
            for (std::size_t j = i + 1; j < poses.size(); ++j)
                require(capsule_paths_clear(poses[i].feet, poses[i].feet, s.actors()[i].size,
                                            poses[j].feet, poses[j].feet, s.actors()[j].size),
                        "presentation capsules overlap");
        }
        previous = poses;
    }
}
void step(TraversalSession& s, unsigned ticks, bool image = true) {
    for (unsigned t = 0; t < ticks; ++t) {
        try {
            s.advance(1);
            s.validate();
            if (image)
                image_check(s);
        } catch (...) {
            std::cerr << s.inspect();
            throw;
        }
    }
}
void arrived(const TraversalSession& s) {
    for (const auto& a : s.actors()) {
        if (a.state != TraversalState::Arrived) {
            std::cerr << s.inspect();
            throw std::runtime_error("actor failed to finish route");
        }
    }
}
TraversalScene bridge() {
    auto s = floor();
    s.geometry.bodies = {{1, {-6, 0, 0}, 0, 4, 5, .3, 0, 0, true},
                         {2, {6, 0, 0}, 0, 4, 5, .3, 0, 0, true},
                         {3, {0, 0, 0}, 0, 2, .65, .3, 0, 0, true}};
    s.connections = {{{1, {3.8, 0}}, {3, {-1.8, 0}}, SurfaceLegKind::Step, 1},
                     {{3, {1.8, 0}}, {2, {-3.8, 0}}, SurfaceLegKind::Step, 1}};
    s.passages = {{1, 3, {}, {}}};
    s.spawns = {{{1, {0, -1}}, {}, false},
                {{2, {0, -1}}, {}, false},
                {{1, {0, 1}}, {}, false},
                {{2, {0, 1}}, {}, false}};
    return s;
}
void narrow_bridge_opposite() {
    TraversalSession s{bridge()};
    for (std::size_t i = 0; i < 4; ++i)
        s.set_goal(i, {i % 2 == 0 ? 2U : 1U, {0, i < 2 ? -1. : 1.}});
    EntityId previous_owner;
    std::vector<EntityId> previous_queue;
    bool saved_queue = false;
    for (unsigned t = 0; t < 700; ++t) {
        step(s, 1);
        const auto bytes = s.serialize();
        const auto at = bytes.find("PASSAGES ");
        std::istringstream in{bytes.substr(at)};
        std::string tag;
        unsigned count = 0, id = 0, surface = 0;
        std::size_t queued = 0;
        EntityId owner;
        require(static_cast<bool>(in >> tag >> count >> id >> surface >> owner.value >> queued),
                "passage snapshot missing");
        std::vector<EntityId> queue(queued);
        for (auto& actor : queue)
            in >> actor.value;
        if (owner.value != 0 && owner != previous_owner && !previous_queue.empty()) {
            const auto expected = std::find_if(previous_queue.begin(), previous_queue.end(),
                                               [&](auto actor) { return actor != previous_owner; });
            if (expected != previous_queue.end())
                require(owner == *expected, "bridge queue violated FIFO");
        }
        if (owner.value != 0 && queued > 1 && !saved_queue) {
            TraversalSession loaded{bridge()};
            loaded.restore(bytes);
            auto next = s;
            loaded.advance(3);
            next.advance(3);
            require(loaded.serialize() == next.serialize(),
                    "owned bridge queue continuation differs");
            saved_queue = true;
        }
        previous_owner = owner;
        previous_queue = queue;
    }
    arrived(s);
    require(saved_queue, "bridge fixture never exercised a saved waiting queue");
    require(s.inspect().find("passage=") != std::string::npos, "missing passage diagnostics");
    auto scene = bridge();
    scene.spawns = {{{3, {0, .3}}, {}, false}, {{2, {0, 0}}, {}, false}};
    TraversalSession occupied{scene};
    occupied.set_goal(1, {1, {0, 0}});
    step(occupied, 120);
    require(occupied.actors()[1].support.surface == 2,
            "unowned but occupied bridge admitted an entrant");
    occupied.set_input(0, {1, 0});
    step(occupied, 60);
    occupied.clear_input();
    step(occupied, 250);
    require(occupied.actors()[1].state == TraversalState::Arrived,
            "manual bridge occupant never released entry");
}
TraversalScene ferry() {
    TraversalScene s;
    s.geometry.bodies = {{1, {-3, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                         {2, {5, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                         {3, {0, 0, 0}, 0, 1, 1, .25, 0, 0, true}};
    s.platforms = {{{3, {2, 0, 0}, 0, 60, 10, 0}, 0, false}};
    s.connections = {{{1, {1.8, 0}}, {2, {-1.8, 0}}, SurfaceLegKind::Transport, 0, 1, 4}};
    s.services = {{1, 3, {{{1, {1.8, 0}}, {2, {-1.8, 0}}}}, {{{-.8, 0}, {.8, 0}}}, {}, {}}};
    s.spawns = {{{1, {0, 0}}, {}, false}, {{2, {-.5, -1}}, {}, false}, {{2, {-.5, 1}}, {}, false}};
    return s;
}
void ferry_exchange() {
    const auto scene = ferry();
    TraversalSession s{scene};
    s.set_goal(0, {2, {1, 0}});
    s.set_goal(1, {1, {-1, -1}});
    s.set_goal(2, {1, {-1, 1}});
    bool seen[5]{};
    for (unsigned t = 0; t < 1000; ++t) {
        step(s, 1);
        const auto phase = static_cast<unsigned>(s.actors()[0].transfer);
        if (phase >= 2 && !seen[phase]) {
            const auto bytes = s.serialize();
            TraversalSession loaded{scene};
            loaded.restore(bytes);
            auto next = s;
            loaded.advance(2);
            next.advance(2);
            require(loaded.serialize() == next.serialize(), "transfer phase continuation differs");
            require(s.serialize() == bytes, "presentation or continuation probe mutated session");
            seen[phase] = true;
        }
    }
    require(seen[2] && seen[3] && seen[4], "missing boarding/ride/exit phase");
    arrived(s);
}
void rapid_retarget() {
    TraversalSession s{floor()};
    s.approach(0, {1, {3, 0}}, .9);
    step(s, 5);
    const auto before = s.actors()[0];
    s.approach(0, {1, {3, 0}}, .9);
    require(s.actors()[0].plans == before.plans && s.actors()[0].leg == before.leg,
            "repeated approach restarted navigation");
    for (unsigned t = 0; t < 120; ++t) {
        if (t % 4 == 0)
            s.stop(0);
        else if (t % 4 == 1)
            s.approach(0, {1, {3, 1}}, .9);
        else
            s.set_goal(0, {1, {t % 2 ? -3. : 3., t % 3 ? -2. : 2.}});
        step(s, 2);
        if (t % 13 == 0) {
            const auto bytes = s.serialize();
            s.restore(bytes);
        }
    }
    s.set_goal(0, {1, {3, 0}});
    step(s, 250);
    arrived(s);
}
void obstacle_braking() {
    auto scene = floor();
    scene.geometry.bodies[0].half_z = 2;
    scene.spawns[0].location.local = {0, 0};
    TraversalSession s{scene};
    s.set_input(0, {0, 1});
    step(s, 100);
    require(std::abs(s.actors()[0].feet.x) < .01, "manual input steered sideways near edge");
    require(s.actors()[0].feet.z <= 2 && s.actors()[0].motion.speed < .01,
            "manual movement failed to brake at edge");
    s.clear_input();
    s.set_goal(0, {1, {3, 1.8}});
    step(s, 250);
    arrived(s);
    s.set_input(0, {-1, 0});
    step(s, 30);
    s.stop(0);
    step(s, 60);
    require(s.actors()[0].motion.speed == 0, "stop did not finish braking");
}
void slope_maneuvers() {
    auto scene = floor();
    scene.geometry.bodies = {{1, {0, 1, 0}, .5, 3, 2, .3, .4, 0, true}};
    scene.spawns[0].location.local = {-2, 0};
    TraversalSession s{scene};
    for (const auto x : {2., -2.}) {
        s.set_goal(0, {1, {x, 0}});
        for (unsigned t = 0; t < 180; ++t) {
            const auto old = s.actors()[0].feet;
            step(s, 1);
            require(length(s.actors()[0].feet - old) <= .150001, "slope speed exceeds 3m/s");
            if (t == 25) {
                const auto bytes = s.serialize();
                s.restore(bytes);
            }
        }
        arrived(s);
    }
    scene.geometry.bodies = {{1, {-3, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {0, .4, 0}, 0, 1, 2, .3, .4, 0, true},
                             {3, {3, .8, 0}, 0, 2, 2, .3, 0, 0, true}};
    scene.connections = {{{1, {1.8, 0}}, {2, {-.8, 0}}}, {{2, {.8, 0}}, {3, {-1.8, 0}}}};
    scene.spawns[0].location = {1, {-1, 0}};
    TraversalSession chain{scene};
    chain.set_goal(0, {3, {1, 0}});
    step(chain, 240);
    arrived(chain);
    chain.set_goal(0, {1, {-1, 0}});
    step(chain, 240);
    arrived(chain);
}
void rotating_support() {
    const auto course = make_traversal_course();
    for (const auto radius : {.3, .25}) {
        TraversalScene scene;
        scene.geometry.bodies = {course.geometry.body(1), course.geometry.body(8)};
        scene.platforms = {course.platforms[1]};
        scene.connections = {course.connections[4]};
        scene.spawns = {{{1, {-1.8, -4}}, {radius, 1.8}, false}};
        TraversalSession s{scene};
        s.set_goal(0, {8, {.4, 0}});
        for (unsigned t = 0; t < 220; ++t) {
            const auto& a = s.actors()[0];
            const auto committed = a.leg < a.route.legs.size() &&
                                   a.route.legs[a.leg].kind == SurfaceLegKind::Step &&
                                   a.step_point > 0;
            const auto phase = s.platforms()[0].phase;
            step(s, 1);
            if (committed)
                require(s.platforms()[0].phase == phase, "platform moved during committed step");
        }
        arrived(s);
        require(s.actors()[0].support.surface == 8, "did not enter rotating deck");
        s.set_goal(0, {1, {-2.5, -4}});
        step(s, 350);
        arrived(s);
    }
    auto scene = floor();
    scene.geometry.bodies.push_back({2, {0, .01, 0}, 0, 1, 1, .3, 0, 0, true});
    scene.platforms = {{{2, {}, std::numbers::pi / 2, 80, 10, 1}, 0, false}};
    scene.spawns = {{{2, {1, 1}}, {}, false}, {{1, {1.45, 0}}, {}, false}};
    TraversalSession blocked{scene};
    bool observed = false;
    for (unsigned t = 0; t < 140; ++t) {
        step(blocked, 1);
        observed = observed || blocked.platforms()[0].blocked;
    }
    require(observed, "rotation ignored a neighbouring capsule");
    const auto held = blocked.platforms()[0].phase;
    blocked.set_goal(1, {1, {3, 0}});
    step(blocked, 100);
    require(blocked.platforms()[0].phase != held, "rotation did not resume after blocker left");
    scene = floor();
    scene.geometry.bodies.push_back({2, {}, 0, 1, 1, .3, 0, 0, true});
    scene.platforms = {{{2, {}, .7, 80, 10, 1}, 0, false}};
    scene.spawns = {{{2, {.5, 0}}, {}, false}};
    TraversalSession coplanar{scene};
    step(coplanar, 100);
    require(coplanar.actors()[0].support.surface == 2 && coplanar.platforms()[0].phase > 10,
            "coplanar foundation stole support or blocked rotation");
    require(std::abs(coplanar.geometry().body(2).local(coplanar.actors()[0].feet).x - .5) < 1e-8,
            "coplanar passenger lost local anchor");
}
void moving_passengers() {
    auto scene = floor();
    scene.geometry.bodies = {{1, {}, 0, 8, 8, .3, 0, 0, true},
                             {2, {0, 1, 0}, 0, 2, 2, .3, 0, 0, true}};
    scene.platforms = {{{2, {0, 2, 0}, .7, 80, 10, 1}, 0, false}};
    scene.spawns = {{{2, {-1, -.7}}, {}, false}, {{2, {1, .7}}, {.25, 1.2}, false}};
    TraversalSession s{scene};
    s.set_goal(0, {2, {1, -.7}});
    s.set_goal(1, {2, {-1, .7}});
    step(s, 160);
    arrived(s);
    const auto bytes = s.serialize();
    image_check(s);
    require(s.serialize() == bytes, "presentation mutated moving passengers");
    // Keep contact with the edge while the deck translates and rotates.
    s.set_input(0, {1, 0});
    step(s, 70);
    s.clear_input();
    for (const auto& a : s.actors()) {
        require(a.support.surface == 2, "walking passenger left its deck without a step");
        const auto height = s.geometry().body(2).support({a.feet.x, a.feet.z}, a.size.radius, .61);
        require(height && std::abs(*height - a.feet.y) < support_height_tolerance_m,
                "contact sank passenger into moving deck");
    }
}
void moving_contact() {
    auto scene = floor();
    scene.geometry.bodies = {{1, {}, 0, 8, 8, .3, 0, 0, true},
                             {2, {0, 1, 0}, 0, 2, 2, .3, 0, 0, true},
                             {3, {1.3, 4, 0}, 0, .5, 2, 2.5, 0, 0, false}};
    scene.platforms = {{{2, {0, 1, 0}, .2, 80, 10, 1}, 40, false}};
    scene.spawns = {{{2, {.4, -.7}}, {}, false}, {{2, {-1, .7}}, {}, false}};
    TraversalSession s{scene};
    auto bytes = edited_row(s.serialize(), "ACTORS 2\n", 8, "3");
    bytes = edited_row(bytes, "ACTORS 2\n", 10, "1.5707963267948966");
    s.restore(bytes);
    s.set_input(0, {1, 0});
    step(s, 12);
    require(s.actors()[0].support.surface == 2 && s.actors()[0].motion.speed < .1,
            "collision failed to retain moving support and brake");
}
void command_isolation() {
    auto scene = floor();
    scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                             {2, {2, .2, 0}, 0, 2, 3, .3, 0, 0, true}};
    scene.connections = {{{1, {1.7, 0}}, {2, {-1.7, 0}}}};
    scene.spawns = {{{1, {-1, 0}}, {}, false}, {{1, {-1, 2}}, {}, false}};
    TraversalSession s{scene};
    s.set_goal(0, {2, {1, 0}});
    for (unsigned t = 0; t < 180 && s.actors()[0].step_point == 0; ++t)
        step(s, 1);
    require(s.actors()[0].step_point > 0, "fixture did not enter a step");
    s.set_input(1, {0, -1});
    auto control = s;
    s.approach(0, {2, {1, 0}}, .9);
    step(s, 1);
    step(control, 1);
    require(s.actors()[1].feet == control.actors()[1].feet,
            "approach cleared another actor's manual input");
    s.approach(0, {2, {.5, 1}}, .9);
    const auto deferred = s.actors()[0].pending_goal;
    require(deferred.has_value(), "fixture lost its committed step");
    s.approach(0, {2, {.5, 1}}, .9);
    require(s.actors()[0].pending_goal == deferred && s.actors()[0].pending_reach == .9,
            "repeated deferred approach lost its destination");
    s.clear_input();
    s.set_autonomous(0, true);
    require(!s.actors()[0].stop_after_transfer, "enabling autonomy queued a stop");
    step(s, 160);
    require(s.actors()[0].autonomous && !s.actors()[0].goal,
            "autonomy failed to retire the completed manual route");
}
void support_contract() {
    SpatialPrism from{1, {}, 0, 2, 2, .3, 0, 0, true};
    for (unsigned i = 0; i < 64; ++i) {
        auto to = from;
        to.yaw = .17 + static_cast<double>(i) * .07;
        to.origin = {12, 2, -4};
        const auto feet = platform_anchor(from, to, {2 + 1e-9, 0, 2}, {}, .61);
        require(std::abs(feet.y - 2) < 1e-9, "corner anchor lost support height");
    }
    auto current = from;
    SpatialPoint corner{2 + 1e-9, 0, 2};
    for (unsigned i = 0; i < 200; ++i) {
        auto next = current;
        next.yaw += .013;
        next.origin = {12, 2, -4};
        corner = platform_anchor(current, next, corner, {}, .61);
        require(next.support({corner.x, corner.z}, .3, .61).has_value(),
                "repeated corner carry lost declared support");
        current = next;
    }
    for (const auto gap : {.01, .1}) {
        auto scene = floor();
        scene.geometry.bodies = {{1, {0, 0, 0}, 0, 1, 3, .3, 0, 0, true},
                                 {2, {2 + gap, 0, 0}, 0, 1, 3, .3, 0, 0, true}};
        scene.spawns = {{{1, {.995, 0}}, {}, false}};
        TraversalSession s{scene};
        auto bytes = edited_row(s.serialize(), "ACTORS 1\n", 8, "3");
        bytes = edited_row(bytes, "ACTORS 1\n", 10, "1.5707963267948966");
        s.restore(bytes);
        s.set_input(0, {1, 0});
        step(s, 100);
        require(s.actors()[0].support.surface == 1 && s.actors()[0].feet.x <= 1 + 1e-8,
                "manual inertia crossed an unsupported gap");
    }
    auto contact_scene = floor();
    contact_scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                                     {2, {2, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                                     {3, {.41, 2, 0}, 0, .1, 2, 2, 0, 0, false}};
    contact_scene.spawns = {{{1, {1.995, 0}}, {}, false}};
    TraversalSession contact{contact_scene};
    auto start = edited_row(contact.serialize(), "ACTORS 1\n", 8, "3");
    start = edited_row(start, "ACTORS 1\n", 10, "1.5707963267948966");
    contact.restore(start);
    contact.set_input(0, {1, 0});
    step(contact, 5);
    require(contact.actors()[0].support.surface == 2, "partial contact retained the old support");
    auto scene = floor();
    // A low roof separates the two ends: only the short capsule may pass.
    scene.geometry.bodies.push_back({2, {0, 1.55, 0}, 0, .5, 5, .1, 0, 0, false});
    for (const auto height : {1.2, 1.8}) {
        scene.spawns[0].size = {.25, height};
        TraversalSession s{scene};
        s.set_goal(0, {1, {3, 0}});
        step(s, 200);
        require((s.actors()[0].state == TraversalState::Arrived) == (height < 1.5),
                "navigation ignored unequal capsule heights under roof");
    }
    // A standing blocker in a corridor with room to pass must not stall a route.
    scene = floor();
    scene.geometry.bodies[0].half_z = 1.3;
    scene.spawns.push_back({{1, {0, 0}}, {}, false});
    TraversalSession s{scene};
    s.set_goal(0, {1, {3, 0}});
    step(s, 300);
    require(s.actors()[0].state == TraversalState::Arrived,
            "failed to route around stationary neighbour");
}
void crowd_passing() {
    auto scene = floor();
    scene.spawns = {{{1, {-3, -1}}, {}, false},
                    {{1, {3, -1}}, {}, false},
                    {{1, {-3, 1}}, {.25, 1.2}, false},
                    {{1, {3, 1}}, {}, false}};
    TraversalSession s{scene};
    for (std::size_t i = 0; i < 4; ++i)
        s.set_goal(i, {1, {i % 2 ? -3. : 3., i < 2 ? -1. : 1.}});
    step(s, 350);
    arrived(s);
    scene.spawns = {{{1, {-.31, 0}}, {}, false}, {{1, {.31, 0}}, {}, false}};
    TraversalSession close{scene};
    close.set_goal(0, {1, {3, 0}});
    close.set_goal(1, {1, {-3, 0}});
    step(close, 250);
    arrived(close);
}
void collinear_rest() {
    auto scene = floor();
    scene.spawns = {{{1, {-4, 0}}, {}, true, 0, 100}, {{1, {-3, 0}}, {}, true, 0, 100}};
    scene.stations = {{{1, {}}, TraversalTask::Rest, 1}};
    TraversalSession rest{scene};
    // Two walks, two 60-tick rests and the 100-tick retry after contention.
    step(rest, 360);
    if (rest.stations()[0].completed != 2)
        std::cerr << rest.inspect();
    require(rest.stations()[0].completed == 2, "collinear actors failed to share rest station");
}
void occupied_goal_recovery() {
    auto scene = floor();
    scene.spawns.push_back({{1, {3, 0}}, {}, false});
    TraversalSession s{scene};
    s.set_goal(0, {1, {3, 0}});
    step(s, 180);
    require(s.actors()[0].state == TraversalState::Waiting,
            "occupied goal not reported as Waiting");
    s.set_goal(1, {1, {3, 3}});
    step(s, 250);
    arrived(s);
    collinear_rest();
}
std::string amended(std::string bytes, std::string_view from, std::string_view to) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto at = bytes.find(from);
    require(at != std::string::npos, "mutation target missing");
    bytes.replace(at, from.size(), to);
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + "\n";
}
std::string edited_row(std::string bytes, std::string_view section, std::size_t field,
                       std::string value) {
    // WorkWorld has its own ACTORS section. Mutations here target the outer session.
    const auto framing = bytes.find("\nTUNING ");
    require(framing != std::string::npos, "outer session framing missing");
    const auto section_at = bytes.find(section, framing);
    require(section_at != std::string::npos, "section missing");
    const auto begin = bytes.find('\n', section_at) + 1, end = bytes.find('\n', begin);
    const auto row = bytes.substr(begin, end - begin);
    std::istringstream in{row};
    std::ostringstream out;
    std::string word;
    std::size_t n = 0;
    while (in >> word) {
        if (n)
            out << ' ';
        out << (n++ == field ? value : word);
    }
    require(n > field, "row field missing");
    return amended(bytes, row + "\n", out.str() + "\n");
}
void snapshot_corruption() {
    TraversalSession s{floor()};
    s.set_goal(0, {1, {3, 0}});
    const auto bytes = s.serialize();
    for (const auto& bad : {amended(bytes, "GENESIS_TRAVERSAL 1", "GENESIS_TRAVERSAL 2"),
                            amended(bytes, "GOALS 1 1 3 0", "GOALS 1 1 2 0"),
                            amended(bytes, "GOALS 1 1 3 0", "GOALS 0"), bytes + "junk"}) {
        bool rejected = false;
        try {
            s.restore(bad);
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected, "semantically corrupt snapshot accepted");
        require(s.serialize() == bytes, "failed restore mutated live session");
    }
    TraversalSession ride{ferry()};
    ride.set_goal(0, {2, {1, 0}});
    for (unsigned t = 0; t < 300 && ride.actors()[0].transfer != TransferStage::Riding; ++t)
        step(ride, 1);
    require(ride.actors()[0].transfer == TransferStage::Riding, "fixture failed to ride");
    const auto saved = ride.serialize();
    const auto bad_owner = edited_row(saved, "SERVICES 1\n", 12, "0");
    bool rejected = false;
    try {
        ride.restore(bad_owner);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && ride.serialize() == saved,
            "inconsistent service owner accepted or mutated world");
    const auto bad_cursor = edited_row(saved, "ACTORS 3\n", 26, "7");
    rejected = false;
    try {
        ride.restore(bad_cursor);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && ride.serialize() == saved,
            "inconsistent transfer cursor accepted or mutated world");
    TraversalSession chain{bridge()};
    chain.set_goal(0, {2, {0, -1}});
    const auto linked = chain.serialize();
    const auto bad_route = edited_row(linked, "ROUTE ", 3, "2");
    rejected = false;
    try {
        chain.restore(bad_route);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && chain.serialize() == linked,
            "discontinuous route accepted or mutated world");
    auto overlap = edited_row(linked, "ACTORS 4\n", 4, "1");
    overlap = edited_row(overlap, "ACTORS 4\n", 7, "1");
    rejected = false;
    try {
        chain.restore(overlap);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && chain.serialize() == linked,
            "overlapping actors accepted or mutated world");
    auto corrupt = linked;
    corrupt[0] = 'X';
    rejected = false;
    try {
        chain.restore(corrupt);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && chain.serialize() == linked, "checksum failure mutated world");
}
void long_course(unsigned ticks = 400, bool need_rest = false) {
    auto scene = make_traversal_course();
    if (need_rest)
        scene.spawns[1].energy = 100;
    TraversalSession s{scene};
    step(s, ticks, false);
    const auto bytes = s.serialize();
    TraversalSession loaded;
    loaded.restore(bytes);
    step(loaded, 5, false);
    step(s, 5, false);
    require(s.serialize() == loaded.serialize(), "full course continuation diverged");
    unsigned completed = 0;
    for (const auto& station : s.stations())
        completed += static_cast<unsigned>(station.completed);
    std::cout << "course_checksum=" << snapshot_checksum(bytes) << '\n';
    for (const auto& station : s.stations())
        std::cout << "course_task=" << static_cast<unsigned>(station.kind)
                  << " completed=" << station.completed << '\n';
    require(completed > 0, "full course made no domain progress");
    if (need_rest) {
        unsigned meals = 0, rests = 0, maintenance = 0;
        for (const auto& station : s.stations()) {
            if (station.kind == TraversalTask::Eat)
                meals += static_cast<unsigned>(station.completed);
            if (station.kind == TraversalTask::Rest)
                rests += static_cast<unsigned>(station.completed);
            if (station.kind == TraversalTask::Maintain)
                maintenance += static_cast<unsigned>(station.completed);
        }
        require(meals > 0 && rests > 0 && maintenance > 0,
                "needs course did not complete every task kind");
    }
    std::cout << s.inspect();
    const auto services = bytes.find("SERVICES ");
    std::cout << bytes.substr(services, bytes.find("PASSAGES ", services) - services);
}
void partition() {
    for (const bool course : {false, true}) {
        const auto scene = course ? make_traversal_course() : floor();
        TraversalSession a{scene}, b{scene};
        if (!course) {
            a.set_goal(0, {1, {3, 0}});
            b.set_goal(0, {1, {3, 0}});
        }
        SimulationPacer p, q;
        p.set_running(true);
        q.set_running(true);
        for (unsigned t = 0; t < 500; ++t)
            p.update(a, std::chrono::milliseconds{10});
        for (unsigned t = 0; t < 5; ++t)
            q.update(b, std::chrono::seconds{1});
        require(a.serialize() == b.serialize(), "time partition changed traversal");
        const auto bytes = a.serialize();
        bool rejected = false;
        try {
            a.advance(std::numeric_limits<std::uint64_t>::max());
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected && a.serialize() == bytes, "time overflow changed traversal");
    }
}
void wall_neighbour_recovery() {
    for (const auto z : {-2.4, -1.9, -3.0}) {
        auto scene = make_traversal_course();
        scene.spawns = {{{1, {0, z}}, {}, false}, {{1, {.62, z}}, {}, false}};
        // A nearby stationary neighbour and the rotated wall leave a route to the bridge.
        TraversalSession s{scene};
        s.set_goal(0, {2, {2, 1.5}});
        for (unsigned t = 0; t < 600 && s.actors()[0].state != TraversalState::Arrived; ++t)
            step(s, 1);
        require(s.actors()[0].state == TraversalState::Arrived,
                "neighbour beside wall prevents reaching the opposite bank");
        require(s.actors()[1].feet == TraversalSession{scene}.actors()[1].feet,
                "navigation moved an uncommanded stationary actor");
    }
}
void wall_neighbour_matrix(std::size_t row) {
    const std::array starts{GroundPoint{-1, -1.5}, GroundPoint{-.8, -1.8}, GroundPoint{-1.6, -.8},
                            GroundPoint{.8, -1.8}, GroundPoint{1.6, .8},   GroundPoint{-.8, 1.8}};
    const auto start = starts.at(row);
    unsigned exercised = 0;
    for (unsigned angle = 0; angle < 8; ++angle) {
        auto scene = floor();
        scene.geometry.bodies.push_back({2, {0, 2, -.2}, .55, .25, 1.2, 2, 0, 0, false});
        const auto radians = static_cast<double>(angle) * std::numbers::pi / 4;
        const GroundPoint neighbour{start.x_m + .62 * std::sin(radians),
                                    start.z_m + .62 * std::cos(radians)};
        if (!surface_feet(scene.geometry, {1, start}, {}) ||
            !surface_feet(scene.geometry, {1, neighbour}, {}))
            continue; // Placements inside the wall are not valid initial scenes.
        ++exercised;
        scene.spawns = {{{1, start}, {}, false}, {{1, neighbour}, {}, false}};
        TraversalSession s{scene};
        s.set_goal(0, {1, {4, 0}});
        for (unsigned t = 0; t < 400 && s.actors()[0].state != TraversalState::Arrived; ++t)
            step(s, 1);
        if (s.actors()[0].state != TraversalState::Arrived) {
            std::cerr << "wall matrix start=" << start.x_m << ',' << start.z_m
                      << " neighbour_angle=" << angle << '\n'
                      << s.inspect();
            throw std::runtime_error("open wall-neighbour detour stalled");
        }
    }
    require(exercised > 0, "wall matrix did not exercise a valid placement");
    std::cout << "wall matrix row=" << row << " valid_placements=" << exercised << '\n';
}
void blocked_route_settling() {
    auto scene = floor();
    scene.geometry.bodies = {{1, {}, 0, 8, .4, .3, 0, 0, true}};
    scene.spawns = {{{1, {-2, 0}}, {}, false}, {{1, {0, 0}}, {}, false}};
    TraversalSession s{scene};
    s.set_goal(0, {1, {3, 0}});
    step(s, 200);
    const auto feet = s.actors()[0].feet;
    const auto heading = s.actors()[0].motion.heading;
    const auto plans = s.actors()[0].plans;
    for (unsigned t = 0; t < 80; ++t) {
        step(s, 1);
        if (length(s.actors()[0].feet - feet) >= .01 ||
            std::abs(angle_delta(heading, s.actors()[0].motion.heading)) >= .05)
            std::cerr << "settling baseline=" << feet.x << ',' << feet.z << " heading=" << heading
                      << '\n'
                      << s.inspect();
        require(length(s.actors()[0].feet - feet) < .01,
                "fully blocked route keeps shuffling instead of settling");
        require(std::abs(angle_delta(heading, s.actors()[0].motion.heading)) < .05,
                "fully blocked route keeps changing facing instead of settling");
    }
    require(s.actors()[0].plans <= plans + 2,
            "blocked route replans more than once per two seconds");
    require(s.actors()[0].goal == SurfaceLocation{1, {3, 0}} &&
                s.inspect().find("route=no_path") != std::string::npos,
            "blocked route lost the exact goal or missing no-path diagnostics");
    const auto saved = s.serialize();
    auto loaded = s;
    loaded.restore(saved);
    step(s, 20);
    step(loaded, 20);
    require(s.serialize() == loaded.serialize(), "blocked recovery continuation differs");
    // Remove the obstacle through ordinary actor movement, not by editing physics state.
    s.set_goal(1, {1, {5, 0}});
    step(s, 400);
    std::cout << s.inspect();
    require(s.actors()[0].state == TraversalState::Arrived,
            "route did not recover when stationary blocker departed");
}
void occupied_queue_parking(const std::filesystem::path& capture = {}) {
    auto scene = bridge();
    scene.spawns = {
        {{1, {-1, 0}}, {}, false}, {{3, {0, .3}}, {}, false}, {{1, {2.05, -.85}}, {}, false}};
    TraversalSession s{scene};
    s.set_goal(0, {2, {0, 0}});
    step(s, 300);
    const auto feet = s.actors()[0].feet;
    const auto heading = s.actors()[0].motion.heading;
    for (unsigned t = 0; t < 60; ++t) {
        step(s, 1);
        if (length(s.actors()[0].feet - feet) >= .01 ||
            std::abs(angle_delta(heading, s.actors()[0].motion.heading)) >= .05)
            std::cerr << "occupied parking baseline=" << feet.x << ',' << feet.z << '\n'
                      << s.inspect();
        require(length(s.actors()[0].feet - feet) < .01 &&
                    std::abs(angle_delta(heading, s.actors()[0].motion.heading)) < .05,
                "occupied queue parking causes endless movement or rotation");
    }
    require(s.inspect().find("waiting_for_passage=1 owner=2") != std::string::npos,
            "queued actor lacks passage-owner diagnostics");
    if (!capture.empty())
        s.save(capture);
    auto loaded = s;
    loaded.restore(s.serialize());
    step(s, 20);
    step(loaded, 20);
    require(s.serialize() == loaded.serialize(), "occupied parking continuation differs");
    s.set_goal(1, {2, {0, 2}});
    step(s, 500);
    require(s.actors()[0].state == TraversalState::Arrived,
            "parked actor failed to cross after bridge was freed");
}
void approach_recovery(bool open) {
    for (const auto slope : {0., .15}) {
        TraversalScene scene;
        scene.geometry.bodies = {{1, {}, .3, 8, open ? 1.4 : .4, .3, slope, 0, true}};
        scene.spawns = {{{1, {-2, 0}}, {}, false}, {{1, {1, 0}}, {}, false}};
        TraversalSession s{scene};
        s.set_goal(1, {1, {0, 0}});
        step(s, 10);
        require(s.actors()[1].motion.speed > .05, "fixture blocker is not initially moving");
        s.approach(0, {1, {3, 0}}, .5);
        require(s.actors()[0].route.status == RouteStatus::Found, "fixture lacks initial approach");
        step(s, 200);
        const auto blocker = s.actors()[1].feet;
        if (open) {
            require(s.actors()[0].state == TraversalState::Arrived,
                    "available approach detour was abandoned");
        } else {
            require(s.actors()[0].state == TraversalState::NoPath &&
                        s.actors()[0].route.status == RouteStatus::NoPath,
                    "failed approach recovery retained an obsolete Found route");
            const auto feet = s.actors()[0].feet;
            const auto heading = s.actors()[0].motion.heading;
            const auto plans = s.actors()[0].plans;
            step(s, 80);
            require(length(s.actors()[0].feet - feet) < .01 &&
                        std::abs(angle_delta(heading, s.actors()[0].motion.heading)) < .05,
                    "blocked approach shuffles or spins");
            // Four candidate searches plus one endpoint revalidation, once per 40 ticks.
            require(s.actors()[0].plans <= plans + 10,
                    "blocked approach exceeds five searches per retry window");
            require(s.actors()[0].requested_goal == SurfaceLocation{1, {3, 0}} &&
                        s.actors()[0].requested_reach == .5 &&
                        length(s.actors()[1].feet - blocker) < .001,
                    "recovery changed the requested goal or pushed an uncommanded blocker");
            auto loaded = s;
            loaded.restore(s.serialize());
            step(s, 40);
            step(loaded, 40);
            require(s.serialize() == loaded.serialize(), "blocked approach continuation differs");
            s.set_goal(1, {1, {6, 0}});
            step(s, 300);
            require(s.actors()[0].state == TraversalState::Arrived,
                    "approach failed to resume after the obstruction left");
        }
    }
}
void retained_ramp_recovery() {
    TraversalSession s;
    s.load(REALITY_STALL_FIXTURE);
    // Control: the obstructing actor refuses clearance; stale-route recovery must
    // remain stable independently of the separate cooperative-passage test.
    s.set_autonomous(1, false);
    const auto requested = s.actors()[6].requested_goal;
    step(s, 400);
    require(s.actors()[6].state == TraversalState::NoPath &&
                s.actors()[6].route.status == RouteStatus::NoPath &&
                s.actors()[6].requested_goal == requested,
            "retained actor7 did not settle with its blocked intention preserved");
    const auto feet = s.actors()[6].feet;
    const auto heading = s.actors()[6].motion.heading;
    auto loaded = s;
    loaded.restore(s.serialize());
    for (unsigned tick = 0; tick < 80; ++tick) {
        step(s, 1);
        step(loaded, 1);
        require(length(s.actors()[6].feet - feet) < .01 &&
                    std::abs(angle_delta(heading, s.actors()[6].motion.heading)) < .05,
                "retained actor7 continues shuffling or spinning in the blocked tail");
    }
    require(s.serialize() == loaded.serialize(), "retained recovery continuation differs");
}
void approach_alternative_retry() {
    auto scene = floor();
    scene.spawns = {{{1, {-3, 0}}, {}, false}, {{1, {0, 0}}, {.9, 1.8}, false}};
    TraversalSession s{scene};
    s.approach(0, {1, {0, 0}}, .5);
    require(s.actors()[0].state == TraversalState::NoPath,
            "fixture approach was not fully occupied");
    s.set_goal(1, {1, {-1.1, 0}});
    step(s, 400);
    const auto center =
        surface_feet(s.geometry(), {1, {0, 0}}, TraversalLimits{s.actors()[0].size});
    require(center && !capsule_paths_clear(*center, *center, s.actors()[0].size, s.actors()[1].feet,
                                           s.actors()[1].feet, s.actors()[1].size),
            "fixture freed the original center instead of an alternative");
    require(s.actors()[0].state == TraversalState::Arrived &&
                s.actors()[0].requested_goal == SurfaceLocation{1, {0, 0}} &&
                length(s.actors()[0].feet - *center) <= .5,
            "NoPath approach did not reconsider a newly available in-reach endpoint");
}
void approach_existing_retry() {
    auto scene = floor();
    scene.geometry.bodies[0].half_z = 1.4;
    scene.spawns = {{{1, {-3, 0}}, {}, false},
                    {{1, {1, -.95}}, {.6, 1.8}, false},
                    {{1, {1, 0}}, {}, false},
                    {{1, {1, .95}}, {.6, 1.8}, false}};
    TraversalSession s{scene};
    for (std::size_t i = 1; i < 4; ++i)
        s.set_goal(i, {1, {0, scene.spawns[i].location.local.z_m}});
    step(s, 10);
    s.approach(0, {1, {3, 0}}, .9);
    require(s.actors()[0].route.status == RouteStatus::Found,
            "fixture lacks initial selected endpoint");
    step(s, 200);
    const auto endpoint = s.actors()[0].goal;
    if (!endpoint || s.actors()[0].state != TraversalState::NoPath)
        std::cerr << s.inspect();
    require(endpoint && s.actors()[0].state == TraversalState::NoPath,
            "fixture did not preserve an obstructed endpoint");
    s.set_goal(2, *endpoint);
    step(s, 400);
    const auto feet = surface_feet(s.geometry(), *endpoint, TraversalLimits{s.actors()[0].size});
    require(feet && !capsule_paths_clear(*feet, *feet, s.actors()[0].size, s.actors()[2].feet,
                                         s.actors()[2].feet, s.actors()[2].size),
            "fixture did not keep the original selected endpoint occupied");
    require(s.actors()[0].state == TraversalState::Arrived && s.actors()[0].goal != endpoint &&
                s.actors()[0].requested_goal == SurfaceLocation{1, {3, 0}},
            "existing NoPath endpoint suppressed another reachable approach");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if ((argc == 3 || (argc == 5 && std::string_view{argv[3]} == "--diagnostics")) &&
            std::string_view{argv[1]} == "--resume-course") {
            const auto output =
                argc == 5 ? std::filesystem::path{argv[4]} : std::filesystem::path{};
            if (argc == 5 && !std::filesystem::create_directory(output))
                throw std::invalid_argument("observation output already exists");
            unsigned captures = 0;
            TraversalSession session;
            session.load(argv[2]);
            std::cout << "resume_at=" << session.world().clock().tick_count() << '\n';
            TraversalDiagnostics diagnostics;
            diagnostics.observe(session);
            for (unsigned tick = 0; tick < 1000; ++tick) {
                session.advance(1);
                session.validate();
                diagnostics.observe(session);
                for (const auto actor : diagnostics.new_stalls()) {
                    std::cout << "stall_actor=" << actor.value
                              << " tick=" << session.world().clock().tick_count() << '\n';
                    if (argc == 5 && captures < 8) {
                        diagnostics.capture(
                            session,
                            output / (std::to_string(session.world().clock().tick_count()) + "-" +
                                      std::to_string(actor.value)),
                            "retained_world_observation");
                        ++captures;
                    }
                }
            }
            std::cout << "resumed_ticks=1000\n" << session.inspect() << diagnostics.report();
            return 0;
        }
        if (argc == 3 && std::string_view{argv[1]} == "--capture-queue-wait") {
            occupied_queue_parking(argv[2]);
            return 0;
        }
        if (argc == 3 && (std::string_view{argv[1]} == "--observe-course" ||
                          std::string_view{argv[1]} == "--observe-needs-course")) {
            std::size_t used = 0;
            const auto ticks = std::stoul(argv[2], &used);
            require(used == std::string_view{argv[2]}.size() && ticks >= 400 && ticks <= 2000,
                    "course observation must be 400..2000 ticks");
            long_course(static_cast<unsigned>(ticks),
                        std::string_view{argv[1]} == "--observe-needs-course");
            return 0;
        }
        require(argc == 3 && std::string_view{argv[1]} == "--case", "expected --case NAME");
        const std::string_view name{argv[2]};
        if (name == "approach_existing_retry")
            approach_existing_retry();
        else if (name == "approach_alternative_retry")
            approach_alternative_retry();
        else if (name == "approach_blocked_recovery")
            approach_recovery(false);
        else if (name == "approach_recovery_detour")
            approach_recovery(true);
        else if (name == "retained_ramp_recovery")
            retained_ramp_recovery();
        else if (name == "narrow_bridge_opposite")
            narrow_bridge_opposite();
        else if (name == "ferry_exchange")
            ferry_exchange();
        else if (name == "rapid_retarget")
            rapid_retarget();
        else if (name == "obstacle_braking")
            obstacle_braking();
        else if (name == "slope_maneuvers")
            slope_maneuvers();
        else if (name == "rotating_support")
            rotating_support();
        else if (name == "crowd_passing")
            crowd_passing();
        else if (name == "occupied_goal_recovery")
            occupied_goal_recovery();
        else if (name == "snapshot_corruption")
            snapshot_corruption();
        else if (name == "long_course")
            long_course();
        else if (name == "partition")
            partition();
        else if (name == "collinear_rest")
            collinear_rest();
        else if (name == "moving_passengers")
            moving_passengers();
        else if (name == "command_isolation")
            command_isolation();
        else if (name == "support_contract")
            support_contract();
        else if (name == "wall_neighbour_recovery")
            wall_neighbour_recovery();
        else if (name.starts_with("wall_neighbour_matrix_") && name.size() == 23 &&
                 name.back() >= '0' && name.back() <= '5')
            wall_neighbour_matrix(static_cast<std::size_t>(name.back() - '0'));
        else if (name == "blocked_route_settling")
            blocked_route_settling();
        else if (name == "occupied_queue_parking")
            occupied_queue_parking();
        else if (name == "moving_contact")
            moving_contact();
        else
            throw std::invalid_argument("unknown audit case");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
