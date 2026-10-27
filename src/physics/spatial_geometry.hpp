#pragma once

#include "physics/kinematic.hpp"

#include <optional>

namespace reality {

struct SpatialPoint {
    double x = 0;
    double y = 0;
    double z = 0;
    bool operator==(const SpatialPoint&) const = default;
};
[[nodiscard]] SpatialPoint operator+(SpatialPoint a, SpatialPoint b);
[[nodiscard]] SpatialPoint operator-(SpatialPoint a, SpatialPoint b);
[[nodiscard]] SpatialPoint operator*(SpatialPoint a, double scale);
[[nodiscard]] double dot(SpatialPoint a, SpatialPoint b);
[[nodiscard]] double length(SpatialPoint a);
[[nodiscard]] SpatialPoint unit(SpatialPoint a);
[[nodiscard]] double angle_delta(double from, double to);
void validate(SpatialPoint point);

struct SpatialTriangle {
    std::array<SpatialPoint, 3> points;
};
struct CapsuleSize {
    double radius = .3;
    double height = 1.8;
    bool operator==(const CapsuleSize&) const = default;
    void validate() const;
};

// A convex slab: the upper plane is y=slope_x*x+slope_z*z in its local frame;
// the lower plane is depth metres below it. Frame origin lies on the upper plane.
// Boxes, ramps, floors and moving decks share this geometry and its triangle mesh.
struct SpatialPrism {
    std::uint32_t id = 0;
    SpatialPoint origin;
    double yaw = 0;
    double half_x = 1;
    double half_z = 1;
    double depth = 1;
    double slope_x = 0;
    double slope_z = 0;
    bool walkable = false;
    void validate() const;
    [[nodiscard]] SpatialPoint world(SpatialPoint local) const;
    [[nodiscard]] SpatialPoint local(SpatialPoint world_point) const;
    [[nodiscard]] std::array<SpatialPoint, 8> vertices() const;
    [[nodiscard]] std::array<SpatialTriangle, 12> triangles() const;
    [[nodiscard]] SpatialPoint normal() const;
    // Returns capsule feet height only when its actual spherical contact lies on this top face.
    [[nodiscard]] std::optional<double> support(GroundPoint center, double radius,
                                                double maximum_slope) const;
    [[nodiscard]] std::optional<double> support_local(GroundPoint center, double radius,
                                                      double maximum_slope) const;
};
struct SpatialGeometry {
    std::vector<SpatialPrism> bodies;
    std::uint64_t version = 1;
    void validate() const;
    [[nodiscard]] const SpatialPrism& body(std::uint32_t id) const;
};
struct CapsuleContact {
    double gap = 0;
    SpatialPoint normal;
};
[[nodiscard]] CapsuleContact capsule_contact(const SpatialPrism& body, SpatialPoint feet,
                                             CapsuleSize size);
[[nodiscard]] bool spatial_occupiable(const SpatialGeometry& geometry, SpatialPoint feet,
                                      CapsuleSize size, std::uint32_t ignored_body = 0);
struct SpatialSample {
    double fraction = 0;
    SpatialPoint feet;
};
struct SpatialMove {
    SpatialPoint feet;
    MoveStatus status = MoveStatus::Clear;
    SpatialPoint normal;
    std::array<SpatialSample, 6> samples{};
    std::size_t count = 0;
};
[[nodiscard]] SpatialMove sweep_capsule(const SpatialGeometry& geometry, SpatialPoint feet,
                                        CapsuleSize size, SpatialPoint delta,
                                        std::uint32_t ignored_body = 0, bool slide = false);
[[nodiscard]] bool spatial_segment_clear(const SpatialGeometry& geometry, SpatialPoint start,
                                         SpatialPoint end, CapsuleSize size);
// Conservative swept-volume check for a kinematic prism. A false result may request
// waiting; a true result certifies separation through linear translation and yaw rotation.
[[nodiscard]] bool prism_motion_clear(const SpatialPrism& from, const SpatialPrism& to,
                                      SpatialPoint feet, CapsuleSize size);
[[nodiscard]] bool prisms_overlap(const SpatialPrism& a, const SpatialPrism& b);
[[nodiscard]] bool prism_motion_clear(const SpatialPrism& from, const SpatialPrism& to,
                                      const SpatialPrism& obstacle);
[[nodiscard]] bool capsule_paths_clear(SpatialPoint a_start, SpatialPoint a_end, CapsuleSize a,
                                       SpatialPoint b_start, SpatialPoint b_end, CapsuleSize b);

} // namespace reality
