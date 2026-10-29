#include "graphics/lab_scene.hpp"

#include "lab/traversal_session.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace reality {
namespace {
SpatialPoint cross3(SpatialPoint a, SpatialPoint b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
void triangle(std::vector<SceneTriangle>& scene, const LabCamera& camera, float aspect,
              SpatialPoint a, SpatialPoint b, SpatialPoint c, Vec3 color, int object = -1) {
    SceneTriangle triangle;
    triangle.object = object;
    const std::array<SpatialPoint, 3> points{a, b, c};
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto p = points[i];
        triangle.vertices[i] = {camera.project({static_cast<float>(p.x), static_cast<float>(p.y),
                                                static_cast<float>(p.z)},
                                               aspect),
                                color};
    }
    scene.push_back(triangle);
}
void capsule(std::vector<SceneTriangle>& scene, const LabCamera& camera, float aspect,
             TraversalPose pose, CapsuleSize size, Vec3 color, int object, bool selected) {
    constexpr unsigned sides = 10;
    constexpr std::array<double, 6> angles{-std::numbers::pi / 2, -std::numbers::pi / 4, 0, 0,
                                           std::numbers::pi / 4,  std::numbers::pi / 2};
    std::array<std::array<SpatialPoint, sides>, 6> rings;
    for (std::size_t ring = 0; ring < rings.size(); ++ring) {
        const auto y = ring < 3 ? size.radius : size.height - size.radius;
        for (unsigned side = 0; side < sides; ++side) {
            const auto phi = 2 * std::numbers::pi * side / sides;
            const auto radius = size.radius * std::cos(angles[ring]);
            rings[ring][side] = pose.feet + SpatialPoint{radius * std::cos(phi),
                                                         y + size.radius * std::sin(angles[ring]),
                                                         radius * std::sin(phi)};
        }
    }
    for (std::size_t ring = 1; ring < rings.size(); ++ring) {
        for (unsigned side = 0; side < sides; ++side) {
            const auto next = (side + 1) % sides;
            const auto light =
                static_cast<float>(.72 + .25 * std::cos(2 * std::numbers::pi * side / sides));
            const Vec3 shaded{color.x * light, color.y * light, color.z * light};
            triangle(scene, camera, aspect, rings[ring - 1][side], rings[ring][side],
                     rings[ring][next], shaded, object);
            triangle(scene, camera, aspect, rings[ring - 1][side], rings[ring][next],
                     rings[ring - 1][next], shaded, object);
        }
    }
    const SpatialPoint forward{std::sin(pose.heading), 0, std::cos(pose.heading)};
    const SpatialPoint right{forward.z, 0, -forward.x};
    const auto origin = pose.feet + SpatialPoint{0, size.height + .06, 0};
    triangle(scene, camera, aspect, origin + forward * .55, origin - forward * .12 + right * .2,
             origin - forward * .12 - right * .2,
             selected ? Vec3{.75F, 1, .85F} : Vec3{.7F, .75F, .8F}, object);
}
void diamond(std::vector<SceneTriangle>& scene, const LabCamera& camera, float aspect,
             SpatialPoint point, double radius, Vec3 color) {
    const auto a = point + SpatialPoint{-radius, 0, 0}, b = point + SpatialPoint{0, 0, radius};
    const auto c = point + SpatialPoint{radius, 0, 0}, d = point + SpatialPoint{0, 0, -radius};
    triangle(scene, camera, aspect, a, b, c, color);
    triangle(scene, camera, aspect, a, c, d, color);
}
std::optional<double> ray_triangle(CameraRay ray, const SpatialTriangle& triangle) {
    const auto a = triangle.points[0], edge1 = triangle.points[1] - a,
               edge2 = triangle.points[2] - a;
    const auto p = cross3(ray.direction, edge2);
    const auto determinant = dot(edge1, p);
    if (std::abs(determinant) < 1e-12) {
        return std::nullopt;
    }
    const auto t = ray.origin - a;
    const auto u = dot(t, p) / determinant;
    const auto q = cross3(t, edge1);
    const auto v = dot(ray.direction, q) / determinant;
    const auto distance = dot(edge2, q) / determinant;
    if (u < 0 || v < 0 || u + v > 1 || distance < 0 || distance > 60) {
        return std::nullopt;
    }
    return distance;
}
} // namespace
std::vector<SceneTriangle> build_traversal_scene(const TraversalSession& session, int selected,
                                                 const LabCamera& camera, float aspect,
                                                 double fraction) {
    std::vector<SceneTriangle> scene;
    scene.reserve(2000);
    const auto geometry = session.presentation_geometry(fraction);
    for (const auto& body : geometry.bodies) {
        const auto moving =
            std::any_of(session.platforms().begin(), session.platforms().end(),
                        [&](const auto& p) { return p.definition.body == body.id; });
        const auto closed =
            std::any_of(session.passages().begin(), session.passages().end(),
                        [&](const auto& p) { return p.surface == body.id && !p.open; });
        const auto color = closed                                     ? Vec3{.75F, .15F, .12F}
                           : moving                                   ? Vec3{.8F, .53F, .23F}
                           : !body.walkable                           ? Vec3{.49F, .48F, .52F}
                           : (body.slope_x != 0 || body.slope_z != 0) ? Vec3{.55F, .48F, .3F}
                           : body.origin.y > .1                       ? Vec3{.27F, .43F, .46F}
                                                                      : Vec3{.16F, .24F, .27F};
        const auto mesh = body.triangles();
        for (std::size_t i = 0; i < mesh.size(); ++i) {
            const auto light = i < 2 ? 1.0F : i < 4 ? .4F : .72F;
            const auto& t = mesh[i];
            triangle(scene, camera, aspect, t.points[0], t.points[1], t.points[2],
                     {color.x * light, color.y * light, color.z * light});
        }
    }
    for (const auto& c : session.distant_connections())
        for (const auto terminal : c.terminals) {
            const auto point = surface_feet(session.geometry(), terminal, TraversalLimits{});
            if (point)
                diamond(scene, camera, aspect, *point + SpatialPoint{0, .04, 0}, .3,
                        {.1F, .8F, .85F});
        }
    const auto poses = session.presentation(fraction);
    for (std::size_t i = 0; i < poses.size(); ++i) {
        const auto chosen = selected == static_cast<int>(i);
        const auto color = chosen ? Vec3{.24F, .7F, 1} : Vec3{.95F, .55F, .2F};
        if (!poses[i].visible)
            continue;
        capsule(scene, camera, aspect, poses[i], session.actors()[i].size, color,
                static_cast<int>(i), chosen);
        const auto& actor = session.actors()[i];
        if (actor.cargo.value != 0 &&
            session.world().resource(actor.cargo)->record.quantity.value > 0)
            diamond(scene, camera, aspect,
                    poses[i].feet + SpatialPoint{0, actor.size.height + .35, 0}, .18,
                    {1, .9F, .25F});
    }
    for (const auto& station : session.stations()) {
        const auto& body = geometry.body(station.location.surface);
        const auto p = station.location.local;
        auto world = body.world({p.x_m, body.slope_x * p.x_m + body.slope_z * p.z_m, p.z_m});
        world.y += .015;
        diamond(scene, camera, aspect, world, .35,
                station.kind == TraversalTask::Eat    ? Vec3{.35F, .95F, .5F}
                : station.kind == TraversalTask::Rest ? Vec3{.75F, .5F, 1}
                                                      : Vec3{1, .7F, .25F});
    }
    if (session.has_practice()) {
        const auto view = session.practice_view(0);
        const auto& station = session.stations().at(view.station);
        const auto& body = geometry.body(station.location.surface);
        const auto p = station.location.local;
        const auto center =
            body.world({p.x_m, body.slope_x * p.x_m + body.slope_z * p.z_m, p.z_m}) +
            SpatialPoint{0, .5, 0};
        diamond(scene, camera, aspect, center, .3, {.15F, 1, .95F});
        diamond(scene, camera, aspect, center + SpatialPoint{-.32, .25, 0}, .12, {.2F, .7F, 1});
        diamond(scene, camera, aspect, center + SpatialPoint{.32, .25, 0}, .12, {.2F, .7F, 1});
    }
    if (selected >= 0 && static_cast<std::size_t>(selected) < session.actors().size()) {
        const auto& actor = session.actors()[static_cast<std::size_t>(selected)];
        const auto end = std::min(actor.route.legs.size(), actor.leg + 64);
        for (std::size_t leg = actor.leg; leg < end; ++leg) {
            TraversalLimits limits;
            limits.size = actor.size;
            const auto p = surface_feet(geometry, actor.route.legs[leg].to, limits);
            if (p) {
                diamond(scene, camera, aspect, *p + SpatialPoint{0, .025, 0}, .08,
                        actor.route.legs[leg].kind == SurfaceLegKind::Transport
                            ? Vec3{.8F, .5F, 1}
                            : Vec3{.3F, .8F, 1});
            }
        }
        if (actor.goal) {
            TraversalLimits limits;
            limits.size = actor.size;
            const auto p = surface_feet(geometry, *actor.goal, limits);
            if (p) {
                diamond(scene, camera, aspect, *p + SpatialPoint{0, .03, 0}, .2, {.45F, .85F, 1});
            }
        }
    }
    return scene;
}
std::optional<SurfaceLocation> pick_traversal_surface(const TraversalSession& session,
                                                      const LabCamera& camera, float x, float y,
                                                      float aspect, double fraction,
                                                      bool approach) {
    const auto ray = camera.ray(x, y, aspect);
    const auto geometry = session.presentation_geometry(fraction);
    double nearest = std::numeric_limits<double>::infinity();
    const SpatialPrism* hit = nullptr;
    std::size_t face = 0;
    for (const auto& body : geometry.bodies) {
        const auto mesh = body.triangles();
        for (std::size_t i = 0; i < mesh.size(); ++i) {
            const auto distance = ray_triangle(ray, mesh[i]);
            if (distance && *distance < nearest) {
                nearest = *distance;
                hit = &body;
                face = i;
            }
        }
    }
    if (!hit) {
        return std::nullopt;
    }
    const auto point = ray.origin + ray.direction * nearest;
    if (hit->walkable && face < 2) {
        const auto local = hit->local(point);
        return SurfaceLocation{hit->id, {local.x, local.z}};
    }
    if (!approach) {
        return std::nullopt;
    }
    std::optional<SurfaceLocation> result;
    double highest = -std::numeric_limits<double>::infinity();
    for (const auto& body : geometry.bodies) {
        const auto height = body.support({point.x, point.z}, 0, 1.4);
        if (height && *height <= point.y + 1e-6 && *height > highest) {
            const auto local = body.local(point);
            result = SurfaceLocation{body.id, {local.x, local.z}};
            highest = *height;
        }
    }
    return result;
}
} // namespace reality
