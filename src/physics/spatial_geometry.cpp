#include "physics/spatial_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <set>
#include <stdexcept>

namespace reality {
namespace {
constexpr double epsilon = 1e-9;
void check(bool condition, const char* message) {
    if (!condition) {
        throw std::invalid_argument(message);
    }
}
SpatialPoint cross(SpatialPoint a, SpatialPoint b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
SpatialPoint closest_triangle(SpatialPoint p, const SpatialTriangle& triangle) {
    const auto a = triangle.points[0], b = triangle.points[1], c = triangle.points[2];
    const auto ab = b - a, ac = c - a, ap = p - a;
    const double d1 = dot(ab, ap), d2 = dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) {
        return a;
    }
    const auto bp = p - b;
    const double d3 = dot(ab, bp), d4 = dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) {
        return b;
    }
    const auto vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        return a + ab * (d1 / (d1 - d3));
    }
    const auto cp = p - c;
    const double d5 = dot(ab, cp), d6 = dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) {
        return c;
    }
    const auto vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        return a + ac * (d2 / (d2 - d6));
    }
    const auto va = d3 * d6 - d5 * d4;
    if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0) {
        return b + (c - b) * ((d4 - d3) / (d4 - d3 + d5 - d6));
    }
    const auto inverse = 1 / (va + vb + vc);
    return a + ab * (vb * inverse) + ac * (vc * inverse);
}
struct ClosestPair {
    SpatialPoint axis;
    SpatialPoint triangle;
    double squared = std::numeric_limits<double>::infinity();
    void add(SpatialPoint a, SpatialPoint b) {
        const auto squared_distance = dot(a - b, a - b);
        if (squared_distance < squared) {
            axis = a;
            triangle = b;
            squared = squared_distance;
        }
    }
};
void segment_pair(SpatialPoint a, SpatialPoint b, SpatialPoint c, SpatialPoint d,
                  ClosestPair& result) {
    const auto u = b - a, v = d - c, w = a - c;
    const double aa = dot(u, u), bb = dot(u, v), cc = dot(v, v);
    const double dd = dot(u, w), ee = dot(v, w), denominator = aa * cc - bb * bb;
    constexpr double degenerate_squared = 1e-28;
    if (cc <= degenerate_squared) {
        const auto s = aa > degenerate_squared ? std::clamp(-dd / aa, 0.0, 1.0) : 0;
        result.add(a + u * s, c);
        return;
    }
    double s = aa > degenerate_squared && denominator > 0
                   ? std::clamp((bb * ee - cc * dd) / denominator, 0.0, 1.0)
                   : 0;
    double t = cc > degenerate_squared ? (bb * s + ee) / cc : 0;
    if (t < 0) {
        t = 0;
        s = aa > degenerate_squared ? std::clamp(-dd / aa, 0.0, 1.0) : 0;
    } else if (t > 1) {
        t = 1;
        s = aa > degenerate_squared ? std::clamp((bb - dd) / aa, 0.0, 1.0) : 0;
    }
    result.add(a + u * s, c + v * t);
}
ClosestPair triangle_pair(SpatialPoint a, SpatialPoint b, const SpatialTriangle& triangle) {
    ClosestPair result;
    const auto edge1 = triangle.points[1] - triangle.points[0];
    const auto edge2 = triangle.points[2] - triangle.points[0];
    const auto delta = b - a;
    const auto p = cross(delta, edge2);
    const auto determinant = dot(edge1, p);
    if (std::abs(determinant) > 1e-15) {
        const auto t = a - triangle.points[0];
        const auto u = dot(t, p) / determinant;
        const auto q = cross(t, edge1);
        const auto v = dot(delta, q) / determinant;
        const auto fraction = dot(edge2, q) / determinant;
        if (u >= 0 && v >= 0 && u + v <= 1 && fraction >= 0 && fraction <= 1) {
            const auto hit = a + delta * fraction;
            result.add(hit, hit);
            return result;
        }
    }
    result.add(a, closest_triangle(a, triangle));
    result.add(b, closest_triangle(b, triangle));
    for (std::size_t edge = 0; edge < 3; ++edge) {
        segment_pair(a, b, triangle.points[edge], triangle.points[(edge + 1) % 3], result);
    }
    return result;
}
bool axis_inside(const SpatialPrism& body, SpatialPoint feet, CapsuleSize size) {
    const auto p = body.local(feet + SpatialPoint{0, size.radius, 0});
    if (std::abs(p.x) >= body.half_x || std::abs(p.z) >= body.half_z) {
        return false;
    }
    const auto top = body.slope_x * p.x + body.slope_z * p.z;
    return p.y < top && p.y + size.height - 2 * size.radius > top - body.depth;
}
struct SweepHit {
    double time = 1;
    SpatialPoint normal;
    bool hit = false;
    bool budget = false;
};
bool separating_face(const SpatialPrism& body, SpatialPoint feet, CapsuleSize size,
                     SpatialPoint delta) {
    if (body.slope_x == 0 && body.slope_z == 0 &&
        (std::min(feet.y, feet.y + delta.y) >= body.origin.y - epsilon ||
         std::max(feet.y + size.height, feet.y + delta.y + size.height) <=
             body.origin.y - body.depth + epsilon)) {
        return true;
    }
    // One rotation for both local points and the top normal avoids three
    // independent cosine/sine evaluations on the hot sweep path.
    const auto cosine = std::cos(body.yaw), sine = std::sin(body.yaw);
    const auto local = [&](SpatialPoint p) {
        p = p - body.origin;
        return SpatialPoint{p.x * cosine + p.z * sine, p.y, -p.x * sine + p.z * cosine};
    };
    const auto a = local(feet), b = local(feet + delta);
    if (std::min(a.x, b.x) >= body.half_x + size.radius - epsilon ||
        std::max(a.x, b.x) <= -body.half_x - size.radius + epsilon ||
        std::min(a.z, b.z) >= body.half_z + size.radius - epsilon ||
        std::max(a.z, b.z) <= -body.half_z - size.radius + epsilon) {
        return true;
    }
    const auto rotated = body.origin + SpatialPoint{-body.slope_x * cosine + body.slope_z * sine, 1,
                                                    -body.slope_x * sine - body.slope_z * cosine};
    const auto n = unit(rotated - body.origin);
    const auto top = dot(feet + SpatialPoint{0, size.radius, 0} - body.origin, n) - size.radius;
    if (std::min(top, top + dot(delta, n)) >= -epsilon) {
        return true;
    }
    const auto bottom = -dot(feet + SpatialPoint{0, size.height - size.radius, 0} -
                                 (body.origin - SpatialPoint{0, body.depth, 0}),
                             n) -
                        size.radius;
    return std::min(bottom, bottom - dot(delta, n)) >= -epsilon;
}
bool swept_axis_separated(SpatialPoint feet, CapsuleSize size, SpatialPoint delta,
                          const SpatialPrism& body) {
    if (separating_face(body, feet, size, delta)) {
        return true;
    }
    if (body.slope_x == 0 && body.slope_z == 0 && delta.y == 0) {
        const auto p = body.local(feet), end = body.local(feet + delta);
        const auto vertical = p.y + size.radius > 0 ? p.y + size.radius
                              : p.y + size.height - size.radius < -body.depth
                                  ? p.y + size.height - size.radius + body.depth
                                  : 0;
        const auto threshold =
            (size.radius - epsilon) * (size.radius - epsilon) - vertical * vertical;
        if (threshold <= 0) {
            return true;
        }
        const SpatialPoint a{p.x, 0, p.z}, b{end.x, 0, end.z};
        const auto nearest = [&](SpatialPoint q) {
            return SpatialPoint{std::clamp(q.x, -body.half_x, body.half_x), 0,
                                std::clamp(q.z, -body.half_z, body.half_z)};
        };
        ClosestPair pair;
        pair.add(a, nearest(a));
        pair.add(b, nearest(b));
        const std::array<SpatialPoint, 4> corners{{{-body.half_x, 0, -body.half_z},
                                                   {body.half_x, 0, -body.half_z},
                                                   {body.half_x, 0, body.half_z},
                                                   {-body.half_x, 0, body.half_z}}};
        for (std::size_t edge = 0; edge < 4; ++edge) {
            segment_pair(a, b, corners[edge], corners[(edge + 1) % 4], pair);
        }
        return pair.squared >= threshold;
    }
    const auto low = feet + SpatialPoint{0, size.radius, 0};
    const auto high = feet + SpatialPoint{0, size.height - size.radius, 0};
    const auto mesh = body.triangles();
    // A common separating face certifies the entire translation, including legal
    // tangential travel above a deck or along a slope. It also rejects distant bodies.
    for (std::size_t face = 0; face < mesh.size(); face += 2) {
        const auto& t = mesh[face];
        const auto normal = unit(cross(t.points[1] - t.points[0], t.points[2] - t.points[0]));
        const auto plane = dot(t.points[0], normal);
        const auto start = std::min(dot(low, normal), dot(high, normal)) - size.radius;
        if (std::min(start, start + dot(delta, normal)) >= plane - epsilon) {
            return true;
        }
    }
    // The swept capsule axis is a parallelogram. Minimum distance from that convex
    // patch to the prism distinguishes a grazing corner from actual penetration;
    // conservative advancement alone can converge to a tangent and falsely stop.
    const auto threshold = (size.radius - epsilon) * (size.radius - epsilon);
    if (length(high - low) < 1e-14) {
        for (const auto& triangle : mesh) {
            if (triangle_pair(low, low + delta, triangle).squared < threshold) {
                return false;
            }
        }
        return true;
    }
    if (length(cross(high - low, delta)) < 1e-14) {
        const SpatialPoint a{low.x, std::min(low.y, low.y + delta.y), low.z};
        const SpatialPoint b{high.x, std::max(high.y, high.y + delta.y), high.z};
        for (const auto& triangle : mesh) {
            if (triangle_pair(a, b, triangle).squared < threshold) {
                return false;
            }
        }
        return true;
    }
    const std::array<SpatialTriangle, 2> patch{
        {{{low, low + delta, high + delta}}, {{low, high + delta, high}}}};
    for (const auto& triangle : mesh) {
        for (const auto& swept : patch) {
            for (std::size_t edge = 0; edge < 3; ++edge) {
                if (triangle_pair(swept.points[edge], swept.points[(edge + 1) % 3], triangle)
                            .squared < threshold ||
                    triangle_pair(triangle.points[edge], triangle.points[(edge + 1) % 3], swept)
                            .squared < threshold) {
                    return false;
                }
            }
        }
    }
    return true;
}
SweepHit prism_sweep(SpatialPoint feet, CapsuleSize size, SpatialPoint delta,
                     const SpatialPrism& body) {
    if (swept_axis_separated(feet, size, delta, body)) {
        return {};
    }
    double time = 0;
    SpatialPoint normal;
    for (unsigned iteration = 0; iteration < 48; ++iteration) {
        const auto p = feet + delta * time;
        const auto contact = capsule_contact(body, p, size);
        normal = contact.normal;
        const auto closing = -dot(delta, normal);
        if (closing <= 1e-12) {
            return {};
        }
        const auto gap = contact.gap;
        if (gap <= 1e-8) {
            return {time, normal, true, false};
        }
        const auto advance = gap / closing;
        if (time + advance > 1 + 1e-10) {
            return {};
        }
        time = std::min(1.0, time + advance);
    }
    return {time, normal, true, true};
}
} // namespace

SpatialPoint operator+(SpatialPoint a, SpatialPoint b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
SpatialPoint operator-(SpatialPoint a, SpatialPoint b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
SpatialPoint operator*(SpatialPoint a, double scale) {
    return {a.x * scale, a.y * scale, a.z * scale};
}
double dot(SpatialPoint a, SpatialPoint b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
double length(SpatialPoint a) { return std::sqrt(dot(a, a)); }
SpatialPoint unit(SpatialPoint a) {
    const auto n = length(a);
    return n > 1e-15 ? a * (1 / n) : SpatialPoint{};
}
double angle_delta(double from, double to) {
    return std::remainder(to - from, 2 * std::numbers::pi);
}
void validate(SpatialPoint point) {
    check(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z) &&
              std::abs(point.x) <= 1000000 && std::abs(point.y) <= 1000000 &&
              std::abs(point.z) <= 1000000,
          "invalid spatial coordinate");
}
void CapsuleSize::validate() const {
    check(std::isfinite(radius) && std::isfinite(height) && radius >= .001 &&
              height >= 2 * radius && height <= 1000,
          "invalid capsule size");
}
SpatialPoint SpatialPrism::world(SpatialPoint p) const {
    return origin + SpatialPoint{p.x * std::cos(yaw) - p.z * std::sin(yaw), p.y,
                                 p.x * std::sin(yaw) + p.z * std::cos(yaw)};
}
SpatialPoint SpatialPrism::local(SpatialPoint p) const {
    p = p - origin;
    return {p.x * std::cos(yaw) + p.z * std::sin(yaw), p.y,
            -p.x * std::sin(yaw) + p.z * std::cos(yaw)};
}
std::array<SpatialPoint, 8> SpatialPrism::vertices() const {
    std::array<SpatialPoint, 8> vertices;
    constexpr std::array<GroundPoint, 4> corners{{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}};
    for (std::size_t i = 0; i < 4; ++i) {
        const auto x = corners[i].x_m * half_x, z = corners[i].z_m * half_z;
        vertices[i] = world({x, slope_x * x + slope_z * z, z});
        vertices[i + 4] = vertices[i] - SpatialPoint{0, depth, 0};
    }
    return vertices;
}
std::array<SpatialTriangle, 12> SpatialPrism::triangles() const {
    const auto v = vertices();
    constexpr std::array<std::array<unsigned, 3>, 12> indices{{{0, 2, 1},
                                                               {0, 3, 2},
                                                               {4, 5, 6},
                                                               {4, 6, 7},
                                                               {0, 1, 5},
                                                               {0, 5, 4},
                                                               {1, 2, 6},
                                                               {1, 6, 5},
                                                               {2, 3, 7},
                                                               {2, 7, 6},
                                                               {3, 0, 4},
                                                               {3, 4, 7}}};
    std::array<SpatialTriangle, 12> result;
    for (std::size_t i = 0; i < indices.size(); ++i) {
        for (std::size_t j = 0; j < 3; ++j) {
            result[i].points[j] = v[indices[i][j]];
        }
    }
    return result;
}
SpatialPoint SpatialPrism::normal() const { return unit(world({-slope_x, 1, -slope_z}) - origin); }
std::optional<double> SpatialPrism::support(GroundPoint center, double radius,
                                            double maximum_slope) const {
    const auto p = local({center.x_m, origin.y, center.z_m});
    return support_local({p.x, p.z}, radius, maximum_slope);
}
std::optional<double> SpatialPrism::support_local(GroundPoint center, double radius,
                                                  double maximum_slope) const {
    const auto gradient = std::hypot(slope_x, slope_z);
    if (!walkable || std::atan(gradient) > maximum_slope) {
        return std::nullopt;
    }
    const auto scale = std::sqrt(1 + gradient * gradient);
    const auto contact_x = center.x_m + radius * slope_x / scale;
    const auto contact_z = center.z_m + radius * slope_z / scale;
    if (std::abs(contact_x) > half_x + epsilon || std::abs(contact_z) > half_z + epsilon) {
        return std::nullopt;
    }
    return origin.y + slope_x * center.x_m + slope_z * center.z_m + radius * (scale - 1);
}
void SpatialPrism::validate() const {
    reality::validate(origin);
    check(id != 0 && std::isfinite(yaw) && std::abs(yaw) <= 1000000 && std::isfinite(half_x) &&
              std::isfinite(half_z) && std::isfinite(depth) && half_x >= .001 && half_z >= .001 &&
              depth >= .001 && half_x <= 1000 && half_z <= 1000 && depth <= 1000 &&
              std::isfinite(slope_x) && std::isfinite(slope_z) && std::abs(slope_x) <= 8 &&
              std::abs(slope_z) <= 8,
          "invalid spatial prism");
    for (const auto v : vertices()) {
        reality::validate(v);
    }
}
void SpatialGeometry::validate() const {
    check(version != 0 && !bodies.empty() && bodies.size() <= 64, "invalid spatial geometry");
    std::set<std::uint32_t> ids;
    for (const auto& body : bodies) {
        body.validate();
        check(ids.insert(body.id).second, "duplicate spatial body");
    }
}
const SpatialPrism& SpatialGeometry::body(std::uint32_t id) const {
    const auto found =
        std::find_if(bodies.begin(), bodies.end(), [&](const auto& b) { return b.id == id; });
    if (found == bodies.end()) {
        throw std::invalid_argument("unknown spatial body");
    }
    return *found;
}
CapsuleContact capsule_contact(const SpatialPrism& body, SpatialPoint feet, CapsuleSize size) {
    if (body.slope_x == 0 && body.slope_z == 0) {
        const auto p = body.local(feet);
        const auto y = p.y + size.radius > 0 ? p.y + size.radius
                       : p.y + size.height - size.radius < -body.depth
                           ? p.y + size.height - size.radius + body.depth
                           : 0;
        const SpatialPoint delta{p.x - std::clamp(p.x, -body.half_x, body.half_x), y,
                                 p.z - std::clamp(p.z, -body.half_z, body.half_z)};
        return {length(delta) - size.radius, unit(body.world(delta) - body.origin)};
    }
    ClosestPair best;
    for (const auto& triangle : body.triangles()) {
        const auto pair =
            triangle_pair(feet + SpatialPoint{0, size.radius, 0},
                          feet + SpatialPoint{0, size.height - size.radius, 0}, triangle);
        if (pair.squared < best.squared) {
            best = pair;
        }
    }
    const auto distance = std::sqrt(best.squared);
    return {axis_inside(body, feet, size) ? -size.radius - distance : distance - size.radius,
            unit(best.axis - best.triangle)};
}
bool prism_bounds_disjoint(SpatialPoint from, SpatialPoint to, CapsuleSize size,
                           const SpatialPrism& body) {
    // Capsule axis bounds: horizontal radius plus the vertical feet..feet+height span.
    const auto min_x = std::min(from.x, to.x) - size.radius;
    const auto max_x = std::max(from.x, to.x) + size.radius;
    const auto min_z = std::min(from.z, to.z) - size.radius;
    const auto max_z = std::max(from.z, to.z) + size.radius;
    const auto min_y = std::min(from.y, to.y);
    const auto max_y = std::max(from.y, to.y) + size.height;
    // A yawed prism stays inside half_x + half_z (a conservative circumradius);
    // slopes only extend the vertical bound. Disjoint bounds certify no contact.
    const auto horizontal = body.half_x + body.half_z;
    const auto vertical =
        std::abs(body.slope_x) * body.half_x + std::abs(body.slope_z) * body.half_z;
    return max_x < body.origin.x - horizontal || min_x > body.origin.x + horizontal ||
           max_z < body.origin.z - horizontal || min_z > body.origin.z + horizontal ||
           max_y < body.origin.y - body.depth - vertical || min_y > body.origin.y + vertical;
}
bool spatial_occupiable(const SpatialGeometry& geometry, SpatialPoint feet, CapsuleSize size,
                        std::uint32_t ignored_body) {
    validate(feet);
    size.validate();
    for (const auto& body : geometry.bodies) {
        if (body.id != ignored_body && prism_bounds_disjoint(feet, feet, size, body))
            continue;
        if (body.id != ignored_body && !separating_face(body, feet, size, {}) &&
            capsule_contact(body, feet, size).gap < -epsilon) {
            return false;
        }
    }
    return true;
}
SpatialMove sweep_capsule(const SpatialGeometry& geometry, SpatialPoint feet, CapsuleSize size,
                          SpatialPoint delta, std::uint32_t ignored_body, bool slide) {
    validate(feet + delta);
    SpatialMove result{feet};
    result.samples[0] = {0, feet};
    result.count = 1;
    const auto append = [&](double time) {
        if (time > result.samples[result.count - 1].fraction) {
            result.samples[result.count++] = {time, result.feet};
        }
    };
    const auto finish = [&]() {
        append(1);
        return result;
    };
    if (!spatial_occupiable(geometry, feet, size, ignored_body)) {
        result.status = MoveStatus::InvalidStart;
        return finish();
    }
    double remaining_time = 1;
    for (unsigned contact = 0; contact < 4; ++contact) {
        if (length(delta) < 1e-12) {
            return finish();
        }
        SweepHit earliest;
        for (const auto& body : geometry.bodies) {
            if (body.id == ignored_body ||
                prism_bounds_disjoint(result.feet, result.feet + delta, size, body)) {
                continue;
            }
            const auto hit = prism_sweep(result.feet, size, delta, body);
            if (hit.hit && (!earliest.hit || hit.time < earliest.time)) {
                earliest = hit;
            }
        }
        if (!earliest.hit) {
            result.feet = result.feet + delta;
            return finish();
        }
        const auto fraction = std::max(0.0, earliest.time - contact_margin_m / length(delta));
        result.feet = result.feet + delta * fraction;
        remaining_time *= 1 - fraction;
        append(1 - remaining_time);
        result.normal = earliest.normal;
        result.status = earliest.budget ? MoveStatus::ContactBudget : MoveStatus::Contact;
        if (!slide || earliest.budget) {
            return finish();
        }
        delta = delta * (1 - fraction);
        delta = delta - earliest.normal * std::min(0.0, dot(delta, earliest.normal));
    }
    result.status = MoveStatus::ContactBudget;
    return finish();
}
bool spatial_segment_clear(const SpatialGeometry& geometry, SpatialPoint a, SpatialPoint b,
                           CapsuleSize size) {
    // sweep_capsule rejects an invalid start with InvalidStart; checking the end
    // keeps the same accept/reject result without a second start scan.
    if (!spatial_occupiable(geometry, b, size)) {
        return false;
    }
    const auto moved = sweep_capsule(geometry, a, size, b - a);
    return length(moved.feet - b) <= 2 * contact_margin_m &&
           moved.status != MoveStatus::ContactBudget && moved.status != MoveStatus::InvalidStart;
}
bool prism_motion_clear(const SpatialPrism& from, const SpatialPrism& to, SpatialPoint feet,
                        CapsuleSize size) {
    from.validate();
    to.validate();
    validate(feet);
    size.validate();
    check(from.id == to.id && from.half_x == to.half_x && from.half_z == to.half_z &&
              from.depth == to.depth && from.slope_x == to.slope_x && from.slope_z == to.slope_z &&
              from.walkable == to.walkable,
          "kinematic motion cannot change prism shape");
    const auto horizontal = from.half_x + from.half_z;
    const auto vertical =
        std::abs(from.slope_x) * from.half_x + std::abs(from.slope_z) * from.half_z;
    const auto min_x = std::min(from.origin.x, to.origin.x) - horizontal;
    const auto max_x = std::max(from.origin.x, to.origin.x) + horizontal;
    const auto min_z = std::min(from.origin.z, to.origin.z) - horizontal;
    const auto max_z = std::max(from.origin.z, to.origin.z) + horizontal;
    const auto min_y = std::min(from.origin.y, to.origin.y) - from.depth - vertical;
    const auto max_y = std::max(from.origin.y, to.origin.y) + vertical;
    if (feet.x + size.radius < min_x || feet.x - size.radius > max_x ||
        feet.z + size.radius < min_z || feet.z - size.radius > max_z ||
        feet.y + size.height < min_y || feet.y > max_y) {
        return true;
    }
    auto middle = from;
    middle.origin = (from.origin + to.origin) * .5;
    const auto angle = angle_delta(from.yaw, to.yaw);
    if (std::abs(angle) < 1e-14) {
        const SpatialGeometry geometry{{from}, 1};
        return spatial_segment_clear(geometry, feet, feet - (to.origin - from.origin), size);
    }
    middle.yaw += angle * .5;
    // Horizontal movement of a flat deck cannot penetrate a capsule entirely above it.
    if (from.slope_x == 0 && from.slope_z == 0 && from.origin.y == to.origin.y &&
        (feet.y >= from.origin.y - epsilon ||
         feet.y + size.height <= from.origin.y - from.depth + epsilon)) {
        return true;
    }
    const auto bound = length(to.origin - from.origin) * .5 +
                       std::hypot(from.half_x, from.half_z) * std::abs(angle) * .5;
    return capsule_contact(middle, feet, size).gap >= bound + contact_margin_m;
}
bool prisms_overlap(const SpatialPrism& a, const SpatialPrism& b) {
    const auto av = a.vertices(), bv = b.vertices();
    const std::array<SpatialPoint, 3> ae{av[1] - av[0], av[3] - av[0], av[4] - av[0]};
    const std::array<SpatialPoint, 3> be{bv[1] - bv[0], bv[3] - bv[0], bv[4] - bv[0]};
    const auto separated = [&](SpatialPoint axis) {
        if (length(axis) < 1e-12) {
            return false;
        }
        axis = unit(axis);
        double amin = dot(av[0], axis), amax = amin, bmin = dot(bv[0], axis), bmax = bmin;
        for (const auto v : av) {
            amin = std::min(amin, dot(v, axis));
            amax = std::max(amax, dot(v, axis));
        }
        for (const auto v : bv) {
            bmin = std::min(bmin, dot(v, axis));
            bmax = std::max(bmax, dot(v, axis));
        }
        return amax <= bmin + epsilon || bmax <= amin + epsilon;
    };
    for (std::size_t i = 0; i < 3; ++i) {
        if (separated(cross(ae[i], ae[(i + 1) % 3])) || separated(cross(be[i], be[(i + 1) % 3]))) {
            return false;
        }
        for (std::size_t j = 0; j < 3; ++j) {
            if (separated(cross(ae[i], be[j]))) {
                return false;
            }
        }
    }
    return true;
}
bool capsule_paths_clear(SpatialPoint a_start, SpatialPoint a_end, CapsuleSize a,
                         SpatialPoint b_start, SpatialPoint b_end, CapsuleSize b) {
    validate(a_start);
    validate(a_end);
    validate(b_start);
    validate(b_end);
    a.validate();
    b.validate();
    // The exact test measures horizontal-axis distance against the sum of radii.
    // Disjoint inflated bounds therefore certify clearance without changing the test.
    if (std::min(a_start.x, a_end.x) - a.radius > std::max(b_start.x, b_end.x) + b.radius ||
        std::max(a_start.x, a_end.x) + a.radius < std::min(b_start.x, b_end.x) - b.radius ||
        std::min(a_start.z, a_end.z) - a.radius > std::max(b_start.z, b_end.z) + b.radius ||
        std::max(a_start.z, a_end.z) + a.radius < std::min(b_start.z, b_end.z) - b.radius) {
        return true;
    }
    const auto radius = a.radius + b.radius;
    ClosestPair pair;
    segment_pair(a_start - b_start, a_end - b_end, {0, radius - a.height, 0},
                 {0, b.height - radius, 0}, pair);
    return pair.squared >= (radius - epsilon) * (radius - epsilon);
}
bool prism_motion_clear(const SpatialPrism& from, const SpatialPrism& to,
                        const SpatialPrism& obstacle) {
    const auto angle = angle_delta(from.yaw, to.yaw);
    const auto translation = to.origin - from.origin;
    const auto vertices = from.vertices(), static_vertices = obstacle.vertices();
    std::vector<SpatialPoint> axes;
    for (const auto* shape : {&from, &to, &obstacle}) {
        const auto triangles = shape->triangles();
        for (std::size_t face = 0; face < triangles.size(); face += 2) {
            const auto& t = triangles[face];
            axes.push_back(unit(cross(t.points[1] - t.points[0], t.points[2] - t.points[0])));
        }
    }
    const std::array<SpatialPoint, 3> moving_edges{
        vertices[1] - vertices[0], vertices[3] - vertices[0], vertices[4] - vertices[0]};
    const std::array<SpatialPoint, 3> fixed_edges{static_vertices[1] - static_vertices[0],
                                                  static_vertices[3] - static_vertices[0],
                                                  static_vertices[4] - static_vertices[0]};
    for (const auto a : moving_edges) {
        for (const auto b : fixed_edges) {
            if (length(cross(a, b)) > 1e-12) {
                axes.push_back(unit(cross(a, b)));
            }
        }
    }
    for (const auto axis : axes) {
        double low = std::numeric_limits<double>::infinity(), high = -low;
        const auto linear = dot(translation, axis);
        for (const auto vertex : vertices) {
            const auto p = from.local(vertex);
            const auto cosine = axis.x * p.x + axis.z * p.z, sine = -axis.x * p.z + axis.z * p.x;
            const auto theta = std::remainder(from.yaw, 2 * std::numbers::pi);
            const auto projection = [&](double time) {
                return dot(from.origin, axis) + axis.y * p.y + linear * time +
                       cosine * std::cos(theta + angle * time) +
                       sine * std::sin(theta + angle * time);
            };
            const auto add = [&](double time) {
                const auto value = projection(time);
                low = std::min(low, value);
                high = std::max(high, value);
            };
            add(0);
            add(1);
            const auto amplitude = std::hypot(cosine, sine);
            if (std::abs(angle) > 1e-14 && amplitude > 1e-14) {
                const auto ratio = -linear / (angle * amplitude);
                if (std::abs(ratio) <= 1) {
                    const auto phase = std::atan2(cosine, sine), root = std::acos(ratio);
                    for (const auto sign : {-1.0, 1.0}) {
                        for (int cycle = -3; cycle <= 3; ++cycle) {
                            const auto solution =
                                sign * root - phase + 2 * std::numbers::pi * cycle;
                            const auto time = (solution - theta) / angle;
                            if (time > 0 && time < 1) {
                                add(time);
                            }
                        }
                    }
                }
            }
        }
        auto fixed_low = dot(static_vertices[0], axis), fixed_high = fixed_low;
        for (const auto v : static_vertices) {
            fixed_low = std::min(fixed_low, dot(v, axis));
            fixed_high = std::max(fixed_high, dot(v, axis));
        }
        if (high <= fixed_low + epsilon || fixed_high <= low + epsilon) {
            return true;
        }
    }
    return false;
}

} // namespace reality
