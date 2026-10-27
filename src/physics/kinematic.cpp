#include "physics/kinematic.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace reality {
namespace {

constexpr double coordinate_limit_m = 1000000;
constexpr double direction_epsilon = 1e-12;

void check(bool valid, const char* message) {
    if (!valid) {
        throw std::invalid_argument(message);
    }
}
void validate_rect(GroundRect rectangle) {
    validate(rectangle.minimum);
    validate(rectangle.maximum);
    check(rectangle.minimum.x_m < rectangle.maximum.x_m &&
              rectangle.minimum.z_m < rectangle.maximum.z_m,
          "invalid kinematic rectangle");
}
bool blocks_height(const StaticObstacle& obstacle, BodySize size) {
    return obstacle.bottom_m < size.height_m && obstacle.top_m > 0;
}
GroundRect expanded(const StaticObstacle& obstacle, BodySize size) {
    return {{obstacle.footprint.minimum.x_m - size.half_x_m,
             obstacle.footprint.minimum.z_m - size.half_z_m},
            {obstacle.footprint.maximum.x_m + size.half_x_m,
             obstacle.footprint.maximum.z_m + size.half_z_m}};
}
bool inside_open(GroundRect box, GroundPoint point) {
    return point.x_m > box.minimum.x_m && point.x_m < box.maximum.x_m &&
           point.z_m > box.minimum.z_m && point.z_m < box.maximum.z_m;
}
struct Hit {
    double fraction = 1;
    GroundPoint normal;
    bool hit = false;
    GroundPoint plane;
};

Hit sweep_box(GroundPoint start, GroundPoint delta, GroundRect box) {
    double enter = -std::numeric_limits<double>::infinity();
    double leave = std::numeric_limits<double>::infinity();
    GroundPoint normal;
    const auto slab = [&](double origin, double direction, double minimum, double maximum,
                          GroundPoint negative_normal) {
        if (std::abs(direction) < direction_epsilon) {
            return origin > minimum && origin < maximum;
        }
        const auto first = (minimum - origin) / direction;
        const auto second = (maximum - origin) / direction;
        const auto near = std::min(first, second);
        const auto far = std::max(first, second);
        auto near_normal = negative_normal;
        if (direction < 0) {
            near_normal.x_m = -near_normal.x_m;
            near_normal.z_m = -near_normal.z_m;
        }
        if (near > enter + direction_epsilon) {
            enter = near;
            normal = near_normal;
        } else if (std::abs(near - enter) <= direction_epsilon) {
            normal.x_m += near_normal.x_m;
            normal.z_m += near_normal.z_m;
        }
        leave = std::min(leave, far);
        return enter < leave;
    };
    if (!slab(start.x_m, delta.x_m, box.minimum.x_m, box.maximum.x_m, {-1, 0}) ||
        !slab(start.z_m, delta.z_m, box.minimum.z_m, box.maximum.z_m, {0, -1}) || enter < 0 ||
        leave <= 0) {
        return {};
    }
    const GroundPoint end{start.x_m + delta.x_m, start.z_m + delta.z_m};
    if (enter > 1 && !inside_open(box, end)) {
        return {};
    }
    Hit hit{std::min(1.0, enter), normal, true};
    if (normal.x_m != 0) {
        hit.plane.x_m = normal.x_m < 0 ? box.minimum.x_m : box.maximum.x_m;
    }
    if (normal.z_m != 0) {
        hit.plane.z_m = normal.z_m < 0 ? box.minimum.z_m : box.maximum.z_m;
    }
    return hit;
}

void combine(Hit& earliest, const Hit& candidate) {
    if (!candidate.hit) {
        return;
    }
    if (!earliest.hit || candidate.fraction < earliest.fraction - direction_epsilon) {
        earliest = candidate;
    } else if (std::abs(candidate.fraction - earliest.fraction) <= direction_epsilon) {
        earliest.fraction = std::min(earliest.fraction, candidate.fraction);
        if (candidate.normal.x_m != 0) {
            earliest.normal.x_m = candidate.normal.x_m;
            earliest.plane.x_m = candidate.plane.x_m;
        }
        if (candidate.normal.z_m != 0) {
            earliest.normal.z_m = candidate.normal.z_m;
            earliest.plane.z_m = candidate.plane.z_m;
        }
    }
}

} // namespace

void validate(GroundPoint point) {
    check(std::isfinite(point.x_m) && std::isfinite(point.z_m) &&
              std::abs(point.x_m) <= coordinate_limit_m &&
              std::abs(point.z_m) <= coordinate_limit_m,
          "invalid ground coordinate; supported range is one million metres");
}
void validate(BodySize size) {
    check(std::isfinite(size.half_x_m) && std::isfinite(size.half_z_m) &&
              std::isfinite(size.height_m) && size.half_x_m > 0 && size.half_z_m > 0 &&
              size.height_m > 0 && size.half_x_m <= 1000 && size.half_z_m <= 1000 &&
              size.height_m <= 1000,
          "invalid kinematic body size");
}
double distance(GroundPoint a, GroundPoint b) { return std::hypot(a.x_m - b.x_m, a.z_m - b.z_m); }

void KinematicGeometry::validate() const {
    validate_rect(bounds);
    check(version != 0 && obstacles.size() <= 4096, "invalid geometry bounds or version");
    std::set<std::uint32_t> keys;
    for (const auto& obstacle : obstacles) {
        validate_rect(obstacle.footprint);
        check(std::isfinite(obstacle.bottom_m) && std::isfinite(obstacle.top_m) &&
                  obstacle.bottom_m >= 0 && obstacle.top_m > obstacle.bottom_m &&
                  obstacle.top_m <= 1000 && obstacle.key != 0 && keys.insert(obstacle.key).second,
              "invalid or duplicate obstacle");
    }
}

bool can_occupy(const KinematicGeometry& geometry, GroundPoint point, BodySize size,
                std::span<const StaticObstacle> additional) {
    validate(point);
    validate(size);
    if (point.x_m < geometry.bounds.minimum.x_m + size.half_x_m ||
        point.x_m > geometry.bounds.maximum.x_m - size.half_x_m ||
        point.z_m < geometry.bounds.minimum.z_m + size.half_z_m ||
        point.z_m > geometry.bounds.maximum.z_m - size.half_z_m) {
        return false;
    }
    for (const auto obstacles : {std::span<const StaticObstacle>{geometry.obstacles}, additional}) {
        for (const auto& obstacle : obstacles) {
            if (blocks_height(obstacle, size) && inside_open(expanded(obstacle, size), point)) {
                return false;
            }
        }
    }
    return true;
}

MoveResult move_kinematic(const KinematicGeometry& geometry, GroundPoint start, BodySize size,
                          GroundPoint displacement, std::span<const StaticObstacle> additional,
                          bool slide) {
    check(std::isfinite(displacement.x_m) && std::isfinite(displacement.z_m) &&
              std::abs(displacement.x_m) <= 2 * coordinate_limit_m &&
              std::abs(displacement.z_m) <= 2 * coordinate_limit_m,
          "invalid ground displacement");
    if (!can_occupy(geometry, start, size, additional)) {
        return {start, MoveStatus::InvalidStart, 0};
    }
    MoveResult result{start};
    result.samples[0] = {0, start};
    result.sample_count = 1;
    double time_left = 1;
    const auto append = [&](double fraction) {
        if (fraction <= result.samples[result.sample_count - 1].fraction) {
            return;
        }
        result.samples[result.sample_count++] = {fraction, result.position};
    };
    const auto finish = [&]() {
        append(1);
        return result;
    };
    auto remaining = displacement;
    for (unsigned iteration = 0; iteration < 4; ++iteration) {
        const auto length = std::hypot(remaining.x_m, remaining.z_m);
        if (length < direction_epsilon) {
            return finish();
        }
        Hit hit;
        const auto boundary = [&](double origin, double delta, double minimum, double maximum,
                                  GroundPoint normal) {
            const auto add = [&](double fraction, GroundPoint direction, double plane) {
                Hit candidate{fraction, direction, true};
                if (direction.x_m != 0) {
                    candidate.plane.x_m = plane;
                } else {
                    candidate.plane.z_m = plane;
                }
                combine(hit, candidate);
            };
            if (delta > 0 && origin + delta > maximum) {
                add((maximum - origin) / delta, normal, maximum);
            }
            if (delta < 0 && origin + delta < minimum) {
                add((minimum - origin) / delta, {-normal.x_m, -normal.z_m}, minimum);
            }
        };
        boundary(result.position.x_m, remaining.x_m, geometry.bounds.minimum.x_m + size.half_x_m,
                 geometry.bounds.maximum.x_m - size.half_x_m, {-1, 0});
        boundary(result.position.z_m, remaining.z_m, geometry.bounds.minimum.z_m + size.half_z_m,
                 geometry.bounds.maximum.z_m - size.half_z_m, {0, -1});
        for (const auto obstacles :
             {std::span<const StaticObstacle>{geometry.obstacles}, additional}) {
            for (const auto& obstacle : obstacles) {
                if (blocks_height(obstacle, size)) {
                    combine(hit, sweep_box(result.position, remaining, expanded(obstacle, size)));
                }
            }
        }
        const auto fraction = hit.hit && hit.fraction < 1
                                  ? std::max(0.0, hit.fraction - contact_margin_m / length)
                                  : 1.0;
        result.position.x_m += remaining.x_m * fraction;
        result.position.z_m += remaining.z_m * fraction;
        // An endpoint contact is legal. Use the exact contact planes instead of a rounded
        // start+delta that could place the next step a few ulps inside the obstacle.
        if (hit.hit && hit.fraction == 1) {
            if (hit.normal.x_m != 0) {
                result.position.x_m = hit.plane.x_m;
            }
            if (hit.normal.z_m != 0) {
                result.position.z_m = hit.plane.z_m;
            }
            return finish();
        }
        time_left *= 1 - fraction;
        append(1 - time_left);
        if (!hit.hit) {
            return finish();
        }
        ++result.contacts;
        result.status = MoveStatus::Contact;
        if (!slide) {
            return finish();
        }
        remaining.x_m *= 1 - fraction;
        remaining.z_m *= 1 - fraction;
        if (hit.normal.x_m != 0) {
            remaining.x_m = 0;
        }
        if (hit.normal.z_m != 0) {
            remaining.z_m = 0;
        }
    }
    if (std::hypot(remaining.x_m, remaining.z_m) > direction_epsilon) {
        result.status = MoveStatus::ContactBudget;
    }
    return finish();
}

bool clear_segment(const KinematicGeometry& geometry, GroundPoint start, GroundPoint end,
                   BodySize size) {
    if (!can_occupy(geometry, start, size) || !can_occupy(geometry, end, size)) {
        return false;
    }
    const auto result =
        move_kinematic(geometry, start, size, {end.x_m - start.x_m, end.z_m - start.z_m});
    return result.status == MoveStatus::Clear && distance(result.position, end) <= 1e-9;
}

} // namespace reality
