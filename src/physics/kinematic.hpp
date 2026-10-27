#pragma once

#include <array>

#include <cstdint>
#include <span>
#include <vector>

namespace reality {

struct GroundPoint {
    double x_m = 0;
    double z_m = 0;
    bool operator==(const GroundPoint&) const = default;
};
struct GroundRect {
    GroundPoint minimum;
    GroundPoint maximum;
};
struct BodySize {
    double half_x_m = 0.35;
    double half_z_m = 0.35;
    double height_m = 1.8;
    bool operator==(const BodySize&) const = default;
};
struct StaticObstacle {
    GroundRect footprint;
    double bottom_m = 0;
    double top_m = 1;
    std::uint32_t key = 0;
};
struct KinematicGeometry {
    GroundRect bounds;
    std::vector<StaticObstacle> obstacles;
    std::uint64_t version = 1;
    void validate() const;
};

inline constexpr double contact_margin_m = 0.000001;
void validate(GroundPoint point);
void validate(BodySize size);
[[nodiscard]] double distance(GroundPoint a, GroundPoint b);
// Geometry must pass validate() at its owning boundary before collision queries.
// Additional obstacles are validated bodies supplied by the caller, not imported scene data.
[[nodiscard]] bool can_occupy(const KinematicGeometry& geometry, GroundPoint point, BodySize size,
                              std::span<const StaticObstacle> additional = {});
enum class MoveStatus { Clear, Contact, InvalidStart, ContactBudget };
struct MotionSample {
    double fraction = 0;
    GroundPoint position;
};
struct MoveResult {
    GroundPoint position;
    MoveStatus status = MoveStatus::Clear;
    unsigned contacts = 0;
    std::array<MotionSample, 6> samples{};
    std::size_t sample_count = 0;
};
[[nodiscard]] MoveResult move_kinematic(const KinematicGeometry& geometry, GroundPoint start,
                                        BodySize size, GroundPoint displacement,
                                        std::span<const StaticObstacle> additional = {},
                                        bool slide = true);
[[nodiscard]] bool clear_segment(const KinematicGeometry& geometry, GroundPoint start,
                                 GroundPoint end, BodySize size);

} // namespace reality
