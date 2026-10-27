#include "navigation/surface_navigation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <queue>
#include <stdexcept>
#include <tuple>

namespace reality {
void TraversalLimits::validate() const {
    size.validate();
    if (!std::isfinite(maximum_slope) || maximum_slope < 0 ||
        maximum_slope >= std::numbers::pi / 2 || !std::isfinite(maximum_step) || maximum_step < 0 ||
        maximum_step > 1) {
        throw std::invalid_argument("invalid traversal limits");
    }
}
std::optional<SpatialPoint> surface_feet(const SpatialGeometry& geometry, SurfaceLocation location,
                                         TraversalLimits limits) {
    reality::validate(location.local);
    const auto& body = geometry.body(location.surface);
    const auto point = body.world({location.local.x_m, 0, location.local.z_m});
    const auto height = body.support({point.x, point.z}, limits.size.radius, limits.maximum_slope);
    if (!height) {
        return std::nullopt;
    }
    SpatialPoint feet{point.x, *height, point.z};
    if (!spatial_occupiable(geometry, feet, limits.size)) {
        return std::nullopt;
    }
    return feet;
}
std::optional<SpatialPoint> continuous_surface_feet(const SpatialGeometry& geometry,
                                                    std::uint32_t reference_surface,
                                                    SpatialPoint from, GroundPoint to,
                                                    TraversalLimits limits) {
    limits.validate();
    reality::validate(from);
    reality::validate(to);
    const auto& reference = geometry.body(reference_surface);
    // Metre tolerance for numerical plane matching, never a permitted gameplay step.
    constexpr double tolerance = 1e-8;
    const auto height = [&](const SpatialPrism& body, SpatialPoint point) {
        const auto local = body.local(point);
        return body.origin.y + body.slope_x * local.x + body.slope_z * local.z +
               limits.size.radius *
                   (std::sqrt(1 + body.slope_x * body.slope_x + body.slope_z * body.slope_z) - 1);
    };
    const SpatialPoint end{to.x_m, height(reference, {to.x_m, 0, to.z_m}), to.z_m};
    const auto from_height = height(reference, from);
    if (std::abs(from_height - from.y) >= support_height_tolerance_m) {
        return std::nullopt;
    }
    // Geometry validation caps the body count at 64; the fixed buffer keeps the
    // hot steering path allocation-free without changing the ordering of intervals.
    std::array<std::pair<double, double>, 64> intervals;
    if (geometry.bodies.size() > intervals.size())
        throw std::invalid_argument("surface geometry exceeds interval capacity");
    std::size_t interval_count = 0;
    const auto reference_normal = reference.normal();
    for (const auto& body : geometry.bodies) {
        if (!body.walkable ||
            std::atan(std::hypot(body.slope_x, body.slope_z)) > limits.maximum_slope) {
            continue;
        }
        // A yawed prism stays inside half_x + half_z (a conservative circumradius);
        // a segment outside that bound cannot contribute an interval.
        const auto horizontal = body.half_x + body.half_z;
        if (std::min(from.x, end.x) - limits.size.radius > body.origin.x + horizontal ||
            std::max(from.x, end.x) + limits.size.radius < body.origin.x - horizontal ||
            std::min(from.z, end.z) - limits.size.radius > body.origin.z + horizontal ||
            std::max(from.z, end.z) + limits.size.radius < body.origin.z - horizontal) {
            continue;
        }
        const auto scale = std::sqrt(1 + body.slope_x * body.slope_x + body.slope_z * body.slope_z);
        const auto height_offset = limits.size.radius * (scale - 1);
        const auto body_height = [&](SpatialPoint point) {
            const auto local = body.local(point);
            return body.origin.y + body.slope_x * local.x + body.slope_z * local.z + height_offset;
        };
        if (length(body.normal() - reference_normal) > tolerance ||
            std::abs(body_height(from) - from_height) > tolerance ||
            std::abs(body_height(end) - end.y) > tolerance) {
            continue;
        }
        const auto a = body.local(from), b = body.local(end);
        const auto ox = limits.size.radius * body.slope_x / scale;
        const auto oz = limits.size.radius * body.slope_z / scale;
        double first = 0, last = 1;
        const auto clip = [&](double start, double delta, double low, double high) {
            if (std::abs(delta) < 1e-12) {
                return start >= low - tolerance && start <= high + tolerance;
            }
            auto enter = (low - start) / delta, leave = (high - start) / delta;
            if (enter > leave) {
                std::swap(enter, leave);
            }
            first = std::max(first, enter);
            last = std::min(last, leave);
            return first <= last;
        };
        if (clip(a.x, b.x - a.x, -body.half_x - ox, body.half_x - ox) &&
            clip(a.z, b.z - a.z, -body.half_z - oz, body.half_z - oz)) {
            intervals[interval_count++] = {first, last};
        }
    }
    std::sort(intervals.begin(), intervals.begin() + static_cast<std::ptrdiff_t>(interval_count));
    const auto fraction_tolerance = tolerance / std::max(1.0, length(end - from));
    double covered = 0;
    for (std::size_t i = 0; i < interval_count; ++i) {
        const auto [first, last] = intervals[i];
        if (first > covered + fraction_tolerance) {
            return std::nullopt;
        }
        covered = std::max(covered, last);
        if (covered >= 1 - fraction_tolerance) {
            return end;
        }
    }
    return std::nullopt;
}
bool TraversalLimits::allows_step(SpatialPoint from, SpatialPoint to) const {
    const auto delta = to - from;
    return std::abs(delta.y) <= maximum_step + 1e-9 &&
           std::hypot(delta.x, delta.z) <= 2 * size.radius + .05;
}
std::vector<SpatialPoint> step_points(SpatialPoint a, SpatialPoint b) {
    const auto high = std::max(a.y, b.y) + (std::abs(a.y - b.y) > 1e-9 ? step_clearance_m : 0);
    std::vector<SpatialPoint> points{a};
    const auto add = [&](SpatialPoint p) {
        if (length(p - points.back()) > 1e-9) {
            points.push_back(p);
        }
    };
    add({a.x, high, a.z});
    add({b.x, high, b.z});
    add(b);
    return points;
}
SurfaceRoute find_surface_route(const SpatialGeometry& geometry,
                                std::span<const SurfaceConnection> connections,
                                SurfaceLocation start, SurfaceLocation goal, TraversalLimits limits,
                                std::size_t expansion_budget, std::size_t visibility_budget,
                                std::span<const SurfaceBlocker> blockers) {
    geometry.validate();
    limits.validate();
    if (blockers.size() > 32)
        throw std::invalid_argument("too many navigation blockers");
    for (const auto& blocker : blockers) {
        reality::validate(blocker.feet);
        blocker.size.validate();
    }
    if (connections.size() > 256) {
        throw std::invalid_argument("too many surface connections");
    }
    SurfaceRoute result;
    result.geometry_version = geometry.version;
    const auto first = surface_feet(geometry, start, limits),
               last = surface_feet(geometry, goal, limits);
    if (!first || !last) {
        result.status = RouteStatus::InvalidEndpoint;
        return result;
    }
    if (start == goal) {
        result.status = RouteStatus::Found;
        return result;
    }
    struct Node {
        SurfaceLocation location;
        SpatialPoint feet;
    };
    struct Link {
        std::size_t a, b;
        double cost;
        SurfaceLeg leg;
    };
    std::vector<Node> nodes{{start, *first}, {goal, *last}};
    bool node_budget = false;
    const auto add = [&](SurfaceLocation location) -> std::optional<std::size_t> {
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].location == location) {
                return i;
            }
        }
        const auto point = surface_feet(geometry, location, limits);
        if (!point) {
            return std::nullopt;
        }
        if (nodes.size() == 512) {
            node_budget = true;
            return std::nullopt;
        }
        nodes.push_back({location, *point});
        return nodes.size() - 1;
    };
    bool exhausted = false;
    const auto clear = [&](SpatialPoint a, SpatialPoint b) {
        if (result.visibility_queries == visibility_budget) {
            exhausted = true;
            return false;
        }
        ++result.visibility_queries;
        if (!spatial_segment_clear(geometry, a, b, limits.size))
            return false;
        for (const auto& blocker : blockers) {
            if (!capsule_paths_clear(a, b, limits.size, blocker.feet, blocker.feet, blocker.size))
                return false;
        }
        return true;
    };
    std::vector<Link> links;
    for (const auto& connection : connections) {
        if ((connection.kind != SurfaceLegKind::Step &&
             connection.kind != SurfaceLegKind::Transport) ||
            !std::isfinite(connection.extra_cost_m) || connection.extra_cost_m < 0 ||
            (connection.kind == SurfaceLegKind::Transport ? connection.transport == 0
                                                          : connection.transport != 0)) {
            throw std::invalid_argument("invalid surface connection");
        }
        const auto a = add(connection.from), b = add(connection.to);
        if (!a || !b) {
            continue;
        }
        const auto delta = nodes[*b].feet - nodes[*a].feet;
        double cost = length(delta) + connection.extra_cost_m;
        if (connection.kind == SurfaceLegKind::Step) {
            if (!limits.allows_step(nodes[*a].feet, nodes[*b].feet)) {
                continue;
            }
            const auto points = step_points(nodes[*a].feet, nodes[*b].feet);
            bool legal = true;
            cost = connection.extra_cost_m;
            for (std::size_t p = 1; p < points.size(); ++p) {
                cost += length(points[p] - points[p - 1]);
                legal = clear(points[p - 1], points[p]) && legal;
            }
            if (!legal) {
                continue;
            }
        }
        const SurfaceLeg leg{connection.from, connection.to, connection.kind, connection.passage,
                             connection.transport};
        links.push_back({*a, *b, cost, leg});
        if (connection.bidirectional) {
            auto reversed = leg;
            std::swap(reversed.from, reversed.to);
            links.push_back({*b, *a, cost, reversed});
        }
    }
    constexpr double corner_margin = .0001;
    // Obstacle corners are configuration-space candidates, not raster samples.
    // Expanded rectangles conservatively enclose the capsule's rounded offset.
    for (const auto& surface : geometry.bodies) {
        if (!surface.walkable ||
            std::atan(std::hypot(surface.slope_x, surface.slope_z)) > limits.maximum_slope) {
            continue;
        }
        for (const auto& obstacle : geometry.bodies) {
            if (surface.id == obstacle.id) {
                continue;
            }
            for (const auto x : {-1.0, 1.0}) {
                for (const auto z : {-1.0, 1.0}) {
                    const auto p = obstacle.world(
                        {x * (obstacle.half_x + limits.size.radius + corner_margin), 0,
                         z * (obstacle.half_z + limits.size.radius + corner_margin)});
                    const auto local = surface.local(p);
                    static_cast<void>(add({surface.id, {local.x, local.z}}));
                }
            }
        }
        for (const auto& blocker : blockers) {
            const auto radius = limits.size.radius + blocker.size.radius + corner_margin;
            for (const auto x : {-1., 1.})
                for (const auto z : {-1., 1.}) {
                    const auto local =
                        surface.local(blocker.feet + SpatialPoint{x * radius, 0, z * radius});
                    static_cast<void>(add({surface.id, {local.x, local.z}}));
                }
        }
    }
    if (exhausted || node_budget) {
        result.status = RouteStatus::BudgetExceeded;
        return result;
    }
    const auto infinity = std::numeric_limits<double>::infinity();
    const auto absent = std::numeric_limits<std::size_t>::max();
    std::vector<double> costs(nodes.size(), infinity);
    std::vector<std::size_t> parents(nodes.size(), absent);
    std::vector<SurfaceLeg> parent_legs(nodes.size());
    std::vector<bool> closed(nodes.size(), false);
    using Entry = std::tuple<double, std::size_t, double>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
    costs[0] = 0;
    open.emplace(length(*last - *first), 0, 0);
    while (!open.empty()) {
        const auto [estimate, node, cost] = open.top();
        open.pop();
        static_cast<void>(estimate);
        if (closed[node] || cost != costs[node]) {
            continue;
        }
        if (result.expanded == expansion_budget) {
            result.status = RouteStatus::BudgetExceeded;
            return result;
        }
        ++result.expanded;
        closed[node] = true;
        if (node == 1) {
            std::vector<SurfaceLeg> reversed;
            for (auto cursor = node; parents[cursor] != absent; cursor = parents[cursor]) {
                reversed.push_back(parent_legs[cursor]);
            }
            result.legs.assign(reversed.rbegin(), reversed.rend());
            result.cost_m = cost;
            result.status = RouteStatus::Found;
            return result;
        }
        const auto visit = [&](std::size_t next, double edge_cost, SurfaceLeg leg) {
            const auto next_cost = cost + edge_cost;
            if (!closed[next] && next_cost < costs[next]) {
                costs[next] = next_cost;
                parents[next] = node;
                parent_legs[next] = leg;
                open.emplace(next_cost + length(nodes[next].feet - *last), next, next_cost);
            }
        };
        for (const auto& link : links) {
            if (link.a == node) {
                visit(link.b, link.cost, link.leg);
            }
        }
        for (std::size_t next = 0; next < nodes.size(); ++next) {
            if (next == node || closed[next] ||
                nodes[node].location.surface != nodes[next].location.surface) {
                continue;
            }
            const auto edge_cost = length(nodes[next].feet - nodes[node].feet);
            if (cost + edge_cost >= costs[next]) {
                continue;
            }
            if (clear(nodes[node].feet, nodes[next].feet)) {
                visit(next, edge_cost, {nodes[node].location, nodes[next].location});
            }
            if (exhausted) {
                result.status = RouteStatus::BudgetExceeded;
                return result;
            }
        }
    }
    result.status = RouteStatus::NoPath;
    return result;
}
} // namespace reality
