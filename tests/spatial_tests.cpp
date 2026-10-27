#include "navigation/surface_navigation.hpp"
#include "physics/locomotion.hpp"
#include "physics/spatial_geometry.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string_view>

using namespace reality;
namespace {
constexpr double pi = std::numbers::pi;
void require(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}
void rejects(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error("invalid spatial input accepted");
}
SpatialPrism floor() { return {1, {0, 0, 0}, 0, 10, 10, .3, 0, 0, true}; }
void ramp() {
    SpatialPrism ramp{2, {0, 1, 0}, .7, 3, 2, .3, .4, 0, true};
    ramp.validate();
    const CapsuleSize size;
    for (int step = -20; step <= 20; ++step) {
        const auto p = ramp.world({step * .1, 0, 0});
        const auto support = ramp.support({p.x, p.z}, size.radius, .7);
        require(support.has_value(), "legal ramp refused");
        const auto expected = 1 + .4 * step * .1 + size.radius * (std::sqrt(1.16) - 1);
        require(std::abs(*support - expected) < 1e-12, "incorrect spherical slope support");
        require(std::abs(capsule_contact(ramp, {p.x, *support, p.z}, size).gap) < 1e-12,
                "support penetrates slope");
    }
    require(!ramp.support({0, 0}, size.radius, .2), "over-steep surface accepted");
    const auto a = ramp.world({-2, 0, 0}), b = ramp.world({2, 0, 0});
    const SpatialPoint start{a.x, *ramp.support({a.x, a.z}, size.radius, .7), a.z};
    const SpatialPoint end{b.x, *ramp.support({b.x, b.z}, size.radius, .7), b.z};
    SpatialGeometry geometry{{ramp}, 1};
    geometry.validate();
    const auto moved = sweep_capsule(geometry, start, size, end - start);
    if (!spatial_segment_clear(geometry, start, end, size)) {
        std::cerr << "slope status=" << static_cast<int>(moved.status)
                  << " error=" << length(moved.feet - end) << " normal=" << moved.normal.x << ','
                  << moved.normal.y << ',' << moved.normal.z << '\n';
        throw std::runtime_error("continuous legal slope path blocked");
    }
}
void rotated_sweep() {
    SpatialGeometry geometry{{floor(), {2, {0, 2, 0}, pi / 4, .05, 2, 2, 0, 0, false}}, 1};
    geometry.validate();
    const auto moved = sweep_capsule(geometry, {-5, 0, -5}, {}, {10, 0, 10});
    require(moved.status == MoveStatus::Contact, "rotated thin wall tunneled");
    const auto local = geometry.body(2).local(moved.feet);
    require(std::abs(local.x + .35) < 2e-6 && std::abs(local.z) < 1e-8,
            "wrong rotated contact plane");
    require(spatial_occupiable(geometry, moved.feet, {}), "sweep ends in rotated wall");
    require(moved.normal.x < -.7 && moved.normal.z < -.7, "wrong rotated wall normal");
    const auto away = sweep_capsule(geometry, moved.feet, {}, {-1, 0, -1});
    require(away.status == MoveStatus::Clear, "legal departure blocked");
}
void levels() {
    SpatialGeometry geometry{{floor(), {2, {0, 1.5, 0}, .2, 2, 2, .25, 0, 0, true}}, 1};
    geometry.validate();
    require(spatial_occupiable(geometry, {0, 1.5, 0}, {}), "upper floor overlaps itself");
    require(!spatial_occupiable(geometry, {0, 0, 0}, {}), "tall actor fits through low ceiling");
    require(spatial_occupiable(geometry, {0, 0, 0}, {.3, 1}),
            "short actor cannot pass beneath deck");
    require(geometry.body(1).support({0, 0}, .3, .7) == 0, "lower support missing");
    require(geometry.body(2).support({0, 0}, .3, .7) == 1.5, "upper support missing");
    require(!spatial_occupiable(geometry, {0, -.15, 0}, {.01, .02}),
            "capsule wholly inside solid accepted");
}
void platform_volume() {
    SpatialPrism a{2, {-2, 1.5, 0}, 0, 1, 1, .25, 0, 0, true};
    auto b = a;
    b.origin.x = 2;
    require(!prism_motion_clear(a, b, {0, 0, 0}, {}), "moving deck tunnels through non-rider");
    require(prism_motion_clear(a, b, {0, 1.5, 0}, {}),
            "flat deck falsely crushes capsule above top");
    require(prism_motion_clear(a, b, {0, 0, 5}, {}), "separated actor blocks deck");
    a.origin = {0, 1.5, 0};
    a.half_x = 2;
    a.half_z = .1;
    b = a;
    b.yaw = pi / 2;
    require(!prism_motion_clear(a, b, {1.2, 0, 1.2}, {}), "rotating beam sweeps through actor");
    b.half_x = 3;
    rejects([&] { static_cast<void>(prism_motion_clear(a, b, {0, 0, 0}, {})); });
}
void solid_sat() {
    SpatialPrism a{1, {0, 2, 0}, 0, 1, .2, 2, 0, 0, false};
    auto b = a;
    b.id = 2;
    b.origin.z = .5;
    require(!prisms_overlap(a, b), "separated slabs overlap");
    b.origin.z = .3;
    require(prisms_overlap(a, b), "overlapping slabs separated");
    for (int step = 0; step < 100; ++step) {
        const auto yaw = step * .061;
        a.yaw = yaw;
        b.yaw = yaw;
        b.origin.x = -.3 * std::sin(yaw);
        b.origin.z = .3 * std::cos(yaw);
        require(prisms_overlap(a, b), "SAT not invariant under common rotation");
    }
}
void invalid_data() {
    SpatialGeometry geometry{{floor()}, 1};
    geometry.validate();
    auto bad = floor();
    bad.slope_x = std::numeric_limits<double>::quiet_NaN();
    rejects([&] { bad.validate(); });
    rejects([&] { CapsuleSize{.5, .9}.validate(); });
    rejects([&] {
        static_cast<void>(
            sweep_capsule(geometry, {}, {}, {std::numeric_limits<double>::infinity(), 0, 0}));
    });
    geometry.bodies.push_back(floor());
    rejects([&] { geometry.validate(); });
}
void surface_narrow() {
    SpatialGeometry geometry{{floor(),
                              {2, {0, 2, 3.31}, 0, 1, 3, 2, 0, 0, false},
                              {3, {0, 2, -3.31}, 0, 1, 3, 2, 0, 0, false}},
                             1};
    geometry.validate();
    const auto route = find_surface_route(geometry, {}, {1, {-3, 0}}, {1, {3, 0}}, {});
    require(route.status == RouteStatus::Found && route.legs.size() == 1,
            "off-grid narrow passage missed");
    require(std::abs(route.cost_m - 6) < 1e-12, "direct metric cost incorrect");
    auto limits = TraversalLimits{};
    limits.size.radius = .32;
    const auto large = find_surface_route(geometry, {}, {1, {-3, 0}}, {1, {3, 0}}, limits);
    require(large.status == RouteStatus::Found && large.cost_m > 6.1,
            "oversized capsule uses narrow gap");
}
void sphere_sweep() {
    SpatialGeometry geometry{{floor(), {2, {0, 2, 0}, .3, .001, 2, 2, 0, 0, false}}, 1};
    const auto sphere = sweep_capsule(geometry, {-5, 0, 0}, {.3, .6}, {10, 0, 0});
    require(sphere.status == MoveStatus::Contact && sphere.feet.x < 0, "zero-axis sphere tunneled");
    const auto short_axis = sweep_capsule(geometry, {-5, 0, 0}, {.3, .6000001}, {10, 0, 0});
    require(short_axis.status == MoveStatus::Contact && short_axis.feet.x < 0,
            "short capsule axis tunneled");
}
void locomotion_envelopes() {
    const LocomotionTuning tuning;
    LocomotionState state;
    for (unsigned tick = 0; tick < 240; ++tick) {
        const auto prior = state;
        const auto step =
            advance_locomotion(state, {0, tick < 120 ? 3.0 : 0.0, std::nullopt}, tuning, .05);
        state = step.state;
        require(state.speed >= 0 && state.speed <= 3, "speed limit violated");
        require(state.acceleration >= -6 - 1e-8 && state.acceleration <= 4 + 1e-8,
                "acceleration limit violated");
        require(std::abs(state.acceleration - prior.acceleration) <= 30 * .05 + 1e-7,
                "jerk limit violated");
        require(step.distance >= 0, "braking reversed position");
        if (tick == 119) {
            require(state.speed > 2.99, "never accelerated to cruise");
        }
    }
    require(state.speed < 1e-8 && std::abs(state.acceleration) < 1e-8, "braking did not settle");
    const auto rest = advance_locomotion(state, {0, 0, std::nullopt}, tuning, .05);
    require(rest.distance < 1e-10, "stationary character drifted");
}
void locomotion_arrival() {
    LocomotionState state;
    double traveled = 0;
    for (unsigned tick = 0; tick < 300; ++tick) {
        const auto step = advance_locomotion(state, {0, 3, std::max(0.0, 7.3 - traveled)}, {}, .05);
        state = step.state;
        traveled += step.distance;
        if (traveled > 7.3 + 1e-6) {
            std::cerr << "overshoot tick=" << tick << " traveled=" << traveled
                      << " speed=" << state.speed << " acceleration=" << state.acceleration << '\n';
            throw std::runtime_error("jerk-aware arrival overshot goal");
        }
    }
    if (std::abs(traveled - 7.3) > 1e-4 || state.speed > 1e-4) {
        std::cerr << "arrival traveled=" << traveled << " speed=" << state.speed
                  << " acceleration=" << state.acceleration << '\n';
        throw std::runtime_error("jerk-aware arrival did not settle at goal");
    }
}
void locomotion_turn() {
    LocomotionState state;
    for (unsigned tick = 0; tick < 160; ++tick) {
        const auto previous = state;
        const auto step =
            advance_locomotion(state, {tick < 80 ? pi / 2 : -pi / 2, 3, std::nullopt}, {}, .05);
        state = step.state;
        require(std::abs(state.turn_speed) <= 4 + 1e-10, "turn rate exceeded");
        require(std::abs(state.turn_speed - previous.turn_speed) <= 12 * .05 + 1e-10,
                "angular acceleration exceeded");
        require(std::abs(angle_delta(previous.heading, state.heading)) <= 4 * .05 + 1e-10,
                "heading snapped");
        if (tick == 79) {
            require(std::abs(angle_delta(state.heading, pi / 2)) < .005,
                    "first orientation failed");
        }
    }
    require(std::abs(angle_delta(state.heading, -pi / 2)) < .005, "retargeted orientation failed");
    rejects([&] { static_cast<void>(advance_locomotion(state, {0, 3, std::nullopt}, {}, 0)); });
}
void pair_paths() {
    require(!capsule_paths_clear({-2, 0, 0}, {2, 0, 0}, {}, {2, 0, 0}, {-2, 0, 0}, {}),
            "head-on crossing tunneled");
    require(capsule_paths_clear({-2, 0, 0}, {2, 0, 0}, {}, {0, 3, 0}, {0, 3, 0}, {}),
            "different floors falsely collide");
    require(!capsule_paths_clear({-2, 0, 0}, {2, 0, 0}, {.3, .6}, {0, 0, 0}, {0, 0, 0}, {.3, .6}),
            "spherical pair tunneled through degenerate Minkowski axis");
    require(capsule_paths_clear({-2, 0, 0}, {2, 0, 0}, {}, {0, 0, 2}, {0, 0, 3}, {}),
            "separated paths collide");
}
void unequal_capsules() {
    // Independent oracle: a capsule covers every point within its radius of the
    // vertical axis segment [feet.y + radius, feet.y + height - radius] at
    // (feet.x, feet.z). Two capsules stay clear exactly while the distance between
    // those two axis segments is at least the sum of the radii.
    const CapsuleSize tall{.3, 1.8};   // axis y in [feet.y + .30, feet.y + 1.50]
    const CapsuleSize broad{.5, 1.2};  // axis y in [feet.y + .50, feet.y + .70]
    const CapsuleSize low{.2, 1.5};    // axis y in [feet.y + .20, feet.y + 1.30]
    const CapsuleSize squat{.35, .75}; // axis y in [feet.y + .35, feet.y + .40]
    // Axis gap .90 (2.40 - 1.50) against radii sum .80: clear.
    require(capsule_paths_clear({0, 0, 0}, {0, 0, 0}, tall, {-2, 1.9, 0}, {2, 1.9, 0}, broad),
            "unequal capsules collided across a .90 m clear axis gap");
    // Same pair lowered .40: axis gap .50 < .80, so the paths must collide.
    require(!capsule_paths_clear({0, 0, 0}, {0, 0, 0}, tall, {-2, 1.5, 0}, {2, 1.5, 0}, broad),
            "unequal capsules passed through a .50 m axis gap");
    require(capsule_paths_clear({-2, 1.9, 0}, {2, 1.9, 0}, broad, {0, 0, 0}, {0, 0, 0}, tall) &&
                !capsule_paths_clear({-2, 1.5, 0}, {2, 1.5, 0}, broad, {0, 0, 0}, {0, 0, 0}, tall),
            "unequal capsule pair is not symmetric in argument order");
    // Tall capsule axis ends at 1.50: short capsule axis at 1.95 gives a
    // .45 gap against .65 sum, at 2.35 a .85 gap.
    require(!capsule_paths_clear({0, 0, 0}, {0, 0, 0}, tall, {0, 1.6, 0}, {0, 1.6, 0}, squat),
            "short capsule tunnelled through the tall capsule's upper axis");
    require(capsule_paths_clear({0, 0, 0}, {0, 0, 0}, tall, {0, 2.0, 0}, {0, 2.0, 0}, squat),
            "unequal capsules collided across a .85 m clear axis gap");
    // Overlapping axis spans, so only the horizontal distance counts: radii sum .55.
    require(!capsule_paths_clear({0, 0, 0}, {0, 0, 0}, low, {.3, .1, -2}, {.3, .1, 2}, squat),
            "unequal radii tunneled at a .30 m horizontal offset");
    require(capsule_paths_clear({0, 0, 0}, {0, 0, 0}, low, {.8, .1, -2}, {.8, .1, 2}, squat),
            "unequal radii collided at a .80 m horizontal offset");
}
void surface_metric() {
    SpatialGeometry geometry{{floor(), {2, {0, 2, 0}, 0, .5, 1, 2, 0, 0, false}}, 1};
    const auto route = find_surface_route(geometry, {}, {1, {-3, 0}}, {1, {3, 0}}, {});
    const auto expected = 2 * std::hypot(3 - .8001, 1.3001) + 2 * .8001;
    require(route.status == RouteStatus::Found, "visibility detour missing");
    require(std::abs(route.cost_m - expected) < 1e-8,
            "not shortest route in analytic corner graph");
    for (const auto& leg : route.legs) {
        require(spatial_segment_clear(geometry, *surface_feet(geometry, leg.from, {}),
                                      *surface_feet(geometry, leg.to, {}), {}),
                "route cuts obstacle");
    }
}
void surface_step() {
    SpatialGeometry geometry{
        {{1, {-2, 0, 0}, 0, 2, 2, .3, 0, 0, true}, {2, {2, .2, 0}, 0, 2, 2, .3, 0, 0, true}}, 1};
    const SurfaceLocation start{1, {-1, 0}}, goal{2, {1, 0}};
    require(find_surface_route(geometry, {}, start, goal, {}).status == RouteStatus::NoPath,
            "floors linked without connection");
    const std::array<SurfaceConnection, 1> connections{{{{1, {1.7, 0}}, {2, {-1.7, 0}}}}};
    const auto route = find_surface_route(geometry, connections, start, goal, {});
    require(route.status == RouteStatus::Found, "legal physical step refused");
    require(std::abs(route.cost_m - (6.2 + 2 * step_clearance_m)) < 1e-8,
            "step omitted vertical travel or toe clearance");
    auto limits = TraversalLimits{};
    limits.maximum_step = .1;
    require(find_surface_route(geometry, connections, start, goal, limits).status ==
                RouteStatus::NoPath,
            "step limit ignored");
}
void surface_budgets() {
    SpatialGeometry geometry{{floor()}, 1};
    require(find_surface_route(geometry, {}, {1, {-3, 0}}, {1, {3, 0}}, {}, 0).status ==
                RouteStatus::BudgetExceeded,
            "expansion budget ignored");
    require(find_surface_route(geometry, {}, {1, {-3, 0}}, {1, {3, 0}}, {}, 512, 0).status ==
                RouteStatus::BudgetExceeded,
            "visibility budget ignored");
    geometry.bodies.push_back({2, {0, 3, 0}, 0, 1, 1, .3, 0, 0, true});
    require(find_surface_route(geometry, {}, {1, {-3, 0}}, {2, {0, 0}}, {}).status ==
                RouteStatus::NoPath,
            "wrong floor reached without ascent");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 3 || std::string_view{argv[1]} != "--case") {
            throw std::invalid_argument("expected --case NAME");
        }
        const std::string_view name{argv[2]};
        if (name == "ramp") {
            ramp();
        } else if (name == "rotated_sweep") {
            rotated_sweep();
        } else if (name == "levels") {
            levels();
        } else if (name == "platform_volume") {
            platform_volume();
        } else if (name == "solid_sat") {
            solid_sat();
        } else if (name == "invalid_data") {
            invalid_data();
        } else if (name == "surface_narrow") {
            surface_narrow();
        } else if (name == "sphere_sweep") {
            sphere_sweep();
        } else if (name == "locomotion_envelopes") {
            locomotion_envelopes();
        } else if (name == "locomotion_arrival") {
            locomotion_arrival();
        } else if (name == "locomotion_turn") {
            locomotion_turn();
        } else if (name == "pair_paths") {
            pair_paths();
        } else if (name == "unequal_capsules") {
            unequal_capsules();
        } else if (name == "surface_metric") {
            surface_metric();
        } else if (name == "surface_step") {
            surface_step();
        } else if (name == "surface_budgets") {
            surface_budgets();
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
