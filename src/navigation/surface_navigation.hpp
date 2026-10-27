#pragma once

#include "navigation/grid_navigation.hpp"
#include "physics/spatial_geometry.hpp"

namespace reality {

inline constexpr double support_height_tolerance_m = 3e-5;

struct SurfaceLocation {
    std::uint32_t surface = 0;
    GroundPoint local;
    bool operator==(const SurfaceLocation&) const = default;
};
enum class SurfaceLegKind { Walk, Step, Transport };
struct SurfaceConnection {
    SurfaceLocation from;
    SurfaceLocation to;
    SurfaceLegKind kind = SurfaceLegKind::Step;
    std::uint32_t passage = 0;
    std::uint32_t transport = 0;
    double extra_cost_m = 0;
    bool bidirectional = true;
};
struct TraversalLimits {
    CapsuleSize size;
    double maximum_slope = .6108652381980153; // 35 degrees, radians.
    double maximum_step = .25;
    void validate() const;
    [[nodiscard]] bool allows_step(SpatialPoint from, SpatialPoint to) const;
};
struct SurfaceLeg {
    SurfaceLocation from;
    SurfaceLocation to;
    SurfaceLegKind kind = SurfaceLegKind::Walk;
    std::uint32_t passage = 0;
    std::uint32_t transport = 0;
};
struct SurfaceRoute {
    RouteStatus status = RouteStatus::NoPath;
    std::uint64_t geometry_version = 0;
    std::vector<SurfaceLeg> legs;
    double cost_m = 0;
    std::size_t expanded = 0;
    std::size_t visibility_queries = 0;
};
struct SurfaceBlocker {
    SpatialPoint feet;
    CapsuleSize size;
};
[[nodiscard]] std::optional<SpatialPoint>
surface_feet(const SpatialGeometry& geometry, SurfaceLocation location, TraversalLimits limits);
// Requires continuous coplanar support over the entire segment, including slab joins.
[[nodiscard]] std::optional<SpatialPoint> continuous_surface_feet(const SpatialGeometry& geometry,
                                                                  std::uint32_t reference_surface,
                                                                  SpatialPoint from, GroundPoint to,
                                                                  TraversalLimits limits);
// Step traversal is an explicit up/across/down path. It does not teleport between floors.
inline constexpr double step_clearance_m = 4 * contact_margin_m;
[[nodiscard]] std::vector<SpatialPoint> step_points(SpatialPoint from, SpatialPoint to);
[[nodiscard]] SurfaceRoute
find_surface_route(const SpatialGeometry& geometry, std::span<const SurfaceConnection> connections,
                   SurfaceLocation start, SurfaceLocation goal, TraversalLimits limits,
                   std::size_t expansion_budget = 512, std::size_t visibility_budget = 32768,
                   std::span<const SurfaceBlocker> blockers = {});

} // namespace reality
