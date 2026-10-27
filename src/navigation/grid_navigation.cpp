#include "navigation/grid_navigation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace reality {

void NavigationGrid::validate(const KinematicGeometry& geometry) const {
    const auto width = columns * cell_m, depth = rows * cell_m;
    const auto tolerance = [&](double minimum, double maximum, double expected) {
        return std::max(expected * 1e-12, 8 * std::numeric_limits<double>::epsilon() *
                                              std::max(std::abs(minimum), std::abs(maximum)));
    };
    if (!std::isfinite(cell_m) || cell_m < .001 || !std::isfinite(width) || !std::isfinite(depth) ||
        columns == 0 || rows == 0 || columns > 64 || rows > 64 ||
        std::abs(width - (geometry.bounds.maximum.x_m - geometry.bounds.minimum.x_m)) >
            tolerance(geometry.bounds.minimum.x_m, geometry.bounds.maximum.x_m, width) ||
        std::abs(depth - (geometry.bounds.maximum.z_m - geometry.bounds.minimum.z_m)) >
            tolerance(geometry.bounds.minimum.z_m, geometry.bounds.maximum.z_m, depth)) {
        throw std::invalid_argument(
            "navigation grid must cover geometry bounds, at most 64 by 64 cells");
    }
}

GroundRoute find_route(const KinematicGeometry& geometry, NavigationGrid grid, GroundPoint start,
                       GroundPoint goal, BodySize size, std::size_t expansion_budget) {
    geometry.validate();
    grid.validate(geometry);
    validate(start);
    validate(goal);
    validate(size);
    GroundRoute result{RouteStatus::InvalidEndpoint, geometry.version, size, start, goal, {}, 0};
    if (!can_occupy(geometry, start, size) || !can_occupy(geometry, goal, size)) {
        return result;
    }
    if (start == goal) {
        result.status = RouteStatus::Found;
        result.points.push_back(goal);
        return result;
    }
    const auto cell = [&](GroundPoint point) {
        const auto x =
            static_cast<unsigned>((point.x_m - geometry.bounds.minimum.x_m) / grid.cell_m);
        const auto z =
            static_cast<unsigned>((point.z_m - geometry.bounds.minimum.z_m) / grid.cell_m);
        return std::min(z, grid.rows - 1) * grid.columns + std::min(x, grid.columns - 1);
    };
    const auto center = [&](unsigned index) -> GroundPoint {
        return {geometry.bounds.minimum.x_m + (index % grid.columns + 0.5) * grid.cell_m,
                geometry.bounds.minimum.z_m + (index / grid.columns + 0.5) * grid.cell_m};
    };
    const auto count = grid.rows * grid.columns;
    constexpr auto absent = std::numeric_limits<unsigned>::max();
    const auto infinity = std::numeric_limits<double>::infinity();
    const auto connections = [&](GroundPoint endpoint) {
        std::vector<std::pair<unsigned, double>> links;
        const auto own = cell(endpoint);
        const auto add = [&](unsigned node) {
            if (clear_segment(geometry, endpoint, center(node), size)) {
                links.emplace_back(node, distance(endpoint, center(node)) / grid.cell_m);
            }
        };
        add(own);
        if (!links.empty()) {
            return links;
        }
        const auto x = static_cast<int>(own % grid.columns),
                   z = static_cast<int>(own / grid.columns);
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dx = -1; dx <= 1; ++dx) {
                const auto nx = x + dx, nz = z + dz;
                if (nx >= 0 && nz >= 0 && nx < static_cast<int>(grid.columns) &&
                    nz < static_cast<int>(grid.rows)) {
                    const auto node =
                        static_cast<unsigned>(nz) * grid.columns + static_cast<unsigned>(nx);
                    if (node != own) {
                        add(node);
                    }
                }
            }
        }
        return links;
    };
    const auto sources = connections(start), targets = connections(goal);
    if (sources.empty() || targets.empty()) {
        result.status = RouteStatus::NoPath;
        return result;
    }
    const auto heuristic = [&](unsigned node) {
        double best = infinity;
        for (const auto& [target, attachment] : targets) {
            const auto manhattan = std::abs(static_cast<int>(node % grid.columns) -
                                            static_cast<int>(target % grid.columns)) +
                                   std::abs(static_cast<int>(node / grid.columns) -
                                            static_cast<int>(target / grid.columns));
            best = std::min(best, static_cast<double>(manhattan) + attachment);
        }
        return best;
    };
    std::vector<double> costs(count, infinity), finish_cost(count, infinity);
    std::vector<unsigned> parents(count, absent);
    for (const auto& [node, cost] : targets) {
        finish_cost[node] = cost;
    }
    std::vector<bool> closed(count, false);
    // f, node index, g: stable node index resolves ties without depending on allocation order.
    using Entry = std::tuple<double, unsigned, double>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
    for (const auto& [node, cost] : sources) {
        costs[node] = cost;
        open.emplace(cost + heuristic(node), node, cost);
    }
    unsigned best_goal = absent;
    double best_cost = infinity;
    const auto finish = [&]() {
        std::vector<GroundPoint> reversed;
        for (auto cursor = best_goal; cursor != absent; cursor = parents[cursor]) {
            reversed.push_back(center(cursor));
        }
        result.points.assign(reversed.rbegin(), reversed.rend());
        if (distance(result.points.back(), goal) > 1e-9) {
            result.points.push_back(goal);
        }
        result.status = RouteStatus::Found;
        return result;
    };
    while (!open.empty()) {
        const auto [estimate, node, cost] = open.top();
        open.pop();
        if (best_goal != absent && estimate >= best_cost) {
            return finish();
        }
        if (closed[node] || costs[node] != cost) {
            continue;
        }
        if (result.expanded == expansion_budget) {
            result.status = RouteStatus::BudgetExceeded;
            return result;
        }
        closed[node] = true;
        ++result.expanded;
        if (cost + finish_cost[node] < best_cost) {
            best_cost = cost + finish_cost[node];
            best_goal = node;
        }
        const auto visit = [&](unsigned next) {
            if (closed[next] || cost + 1 >= costs[next] ||
                !clear_segment(geometry, center(node), center(next), size)) {
                return;
            }
            costs[next] = cost + 1;
            parents[next] = node;
            open.emplace(cost + 1 + heuristic(next), next, cost + 1);
        };
        if (node % grid.columns > 0) {
            visit(node - 1);
        }
        if (node % grid.columns + 1 < grid.columns) {
            visit(node + 1);
        }
        if (node >= grid.columns) {
            visit(node - grid.columns);
        }
        if (node + grid.columns < count) {
            visit(node + grid.columns);
        }
    }
    if (best_goal != absent) {
        return finish();
    }
    result.status = RouteStatus::NoPath;
    return result;
}

bool route_matches(const GroundRoute& route, const KinematicGeometry& geometry, GroundPoint start,
                   BodySize size) noexcept {
    return route.status == RouteStatus::Found && route.geometry_version == geometry.version &&
           route.start == start && route.size == size && !route.points.empty();
}

GroundRoute find_motion_route(const KinematicGeometry& geometry, NavigationGrid grid,
                              GroundPoint start, GroundPoint goal, BodySize size,
                              std::size_t expansion_budget, std::size_t shortcut_budget) {
    geometry.validate();
    grid.validate(geometry);
    validate(start);
    validate(goal);
    validate(size);
    if (clear_segment(geometry, start, goal, size)) {
        return {RouteStatus::Found, geometry.version, size, start, goal, {goal}, 0};
    }
    auto route = find_route(geometry, grid, start, goal, size, expansion_budget);
    if (route.status != RouteStatus::Found) {
        return route;
    }
    auto points = std::move(route.points);
    route.points.clear();
    auto anchor = start;
    std::size_t first = 0, checks = 0;
    while (first < points.size()) {
        std::optional<std::size_t> chosen;
        for (auto end = points.size(); end > first; --end) {
            if (checks == shortcut_budget) {
                route.points.insert(route.points.end(),
                                    points.begin() + static_cast<std::ptrdiff_t>(first),
                                    points.end());
                return route;
            }
            ++checks;
            if (clear_segment(geometry, anchor, points[end - 1], size)) {
                chosen = end - 1;
                break;
            }
        }
        if (!chosen) {
            throw std::logic_error("grid route lost a validated edge during shortcutting");
        }
        anchor = points[*chosen];
        route.points.push_back(anchor);
        first = *chosen + 1;
    }
    return route;
}

} // namespace reality
