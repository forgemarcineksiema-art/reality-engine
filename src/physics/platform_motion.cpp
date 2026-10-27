#include "physics/platform_motion.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace reality {
void PlatformDefinition::validate(const SpatialGeometry& base) const {
    reality::validate(translation);
    const auto& initial = base.body(body);
    if (!initial.walkable || !std::isfinite(rotation) ||
        std::abs(rotation) > 2 * std::numbers::pi || travel_ticks < 20 || travel_ticks > 10000 ||
        dwell_ticks < 10 || dwell_ticks > 10000 || foundation == body) {
        throw std::invalid_argument("invalid moving platform definition");
    }
    if (foundation != 0) {
        static_cast<void>(base.body(foundation));
    }
    auto end = initial;
    end.origin = end.origin + translation;
    end.yaw += rotation;
    end.validate();
}
std::uint64_t PlatformDefinition::period() const noexcept {
    return 2 * (std::uint64_t{travel_ticks} + dwell_ticks);
}
SpatialPrism platform_pose(const SpatialGeometry& base, PlatformDefinition definition,
                           double phase) {
    if (!std::isfinite(phase) || phase < 0 || phase >= static_cast<double>(definition.period())) {
        throw std::invalid_argument("invalid platform phase");
    }
    const auto dwell = static_cast<double>(definition.dwell_ticks),
               travel = static_cast<double>(definition.travel_ticks);
    double progress = 0;
    if (phase >= dwell && phase < dwell + travel) {
        progress = (phase - dwell) / travel;
    } else if (phase >= dwell + travel && phase < 2 * dwell + travel) {
        progress = 1;
    } else if (phase >= 2 * dwell + travel) {
        progress = 1 - (phase - 2 * dwell - travel) / travel;
    }
    progress = progress * progress * progress * (10 + progress * (-15 + 6 * progress));
    auto pose = base.body(definition.body);
    pose.origin = pose.origin + definition.translation * progress;
    pose.yaw += definition.rotation * progress;
    return pose;
}
int platform_dock(PlatformDefinition definition, std::uint64_t phase) {
    if (phase >= definition.period()) {
        throw std::invalid_argument("invalid platform phase");
    }
    if (phase < definition.dwell_ticks) {
        return 0;
    }
    if (phase >= definition.dwell_ticks + definition.travel_ticks &&
        phase < 2 * std::uint64_t{definition.dwell_ticks} + definition.travel_ticks) {
        return 1;
    }
    return -1;
}
SpatialPoint platform_anchor(const SpatialPrism& from, const SpatialPrism& to, SpatialPoint feet,
                             CapsuleSize size, double maximum_slope) {
    auto local = from.local(feet);
    if (!to.support_local({local.x, local.z}, size.radius, maximum_slope)) {
        throw std::invalid_argument("passenger lost platform support");
    }
    // Canonicalize only the already accepted numerical border, not an unsupported point.
    // This prevents world/local roundoff from accumulating outside the top face.
    const auto scale = std::sqrt(1 + to.slope_x * to.slope_x + to.slope_z * to.slope_z);
    const auto ox = size.radius * to.slope_x / scale, oz = size.radius * to.slope_z / scale;
    local.x = std::clamp(local.x, -to.half_x - ox, to.half_x - ox);
    local.z = std::clamp(local.z, -to.half_z - oz, to.half_z - oz);
    const auto destination = to.world({local.x, 0, local.z});
    const auto y = to.support_local({local.x, local.z}, size.radius, maximum_slope);
    return {destination.x, *y, destination.z};
}
} // namespace reality
