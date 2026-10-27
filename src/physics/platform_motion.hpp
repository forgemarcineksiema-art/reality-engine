#pragma once

#include "physics/spatial_geometry.hpp"

namespace reality {
struct PlatformDefinition {
    std::uint32_t body = 0;
    SpatialPoint translation;
    double rotation = 0;
    std::uint32_t travel_ticks = 120;
    std::uint32_t dwell_ticks = 80;
    // A machinery insert may be embedded in its own foundation. This explicit
    // structural pair is excluded from body/body tests, never from actor collision.
    std::uint32_t foundation = 0;
    void validate(const SpatialGeometry& base) const;
    [[nodiscard]] std::uint64_t period() const noexcept;
};
[[nodiscard]] SpatialPrism platform_pose(const SpatialGeometry& base, PlatformDefinition definition,
                                         double phase);
[[nodiscard]] int platform_dock(PlatformDefinition definition, std::uint64_t phase);
[[nodiscard]] SpatialPoint platform_anchor(const SpatialPrism& from, const SpatialPrism& to,
                                           SpatialPoint feet, CapsuleSize size,
                                           double maximum_slope);
} // namespace reality
