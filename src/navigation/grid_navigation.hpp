#pragma once

#include "physics/kinematic.hpp"

#include <cstddef>

namespace reality {

struct NavigationGrid {
    double cell_m = 1;
    unsigned columns = 20;
    unsigned rows = 14;
    void validate(const KinematicGeometry& geometry) const;
};
enum class RouteStatus { Found, NoPath, InvalidEndpoint, BudgetExceeded };
struct GroundRoute {
    RouteStatus status = RouteStatus::NoPath;
    std::uint64_t geometry_version = 0;
    BodySize size;
    GroundPoint start;
    GroundPoint goal;
    std::vector<GroundPoint> points;
    std::size_t expanded = 0;
};
[[nodiscard]] GroundRoute find_route(const KinematicGeometry& geometry, NavigationGrid grid,
                                     GroundPoint start, GroundPoint goal, BodySize size,
                                     std::size_t expansion_budget = 4096);
// Motion routes use the same geometry, with a direct segment or a bounded shortcut pass.
// find_route remains the shortest unit-cost grid route for reference and consumers.
[[nodiscard]] GroundRoute find_motion_route(const KinematicGeometry& geometry, NavigationGrid grid,
                                            GroundPoint start, GroundPoint goal, BodySize size,
                                            std::size_t expansion_budget = 4096,
                                            std::size_t shortcut_budget = 4096);
[[nodiscard]] bool route_matches(const GroundRoute& route, const KinematicGeometry& geometry,
                                 GroundPoint start, BodySize size) noexcept;

} // namespace reality
