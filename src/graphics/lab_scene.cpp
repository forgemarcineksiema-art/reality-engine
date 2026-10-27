#include "graphics/lab_scene.hpp"
#include "lab/motion_session.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace reality {
namespace {

float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 scale(Vec3 a, float factor) { return {a.x * factor, a.y * factor, a.z * factor}; }

void box(std::vector<SceneTriangle>& scene, const LabCamera& camera, float aspect, Vec3 center,
         Vec3 extent, Vec3 color, int object = -1) {
    const std::array<Vec3, 8> points{{{-1, -1, -1},
                                      {1, -1, -1},
                                      {1, 1, -1},
                                      {-1, 1, -1},
                                      {-1, -1, 1},
                                      {1, -1, 1},
                                      {1, 1, 1},
                                      {-1, 1, 1}}};
    constexpr std::array<std::array<int, 4>, 6> faces{
        {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3}, {1, 2, 6, 5}, {3, 7, 6, 2}, {0, 1, 5, 4}}};
    constexpr std::array<float, 6> light{0.65F, 0.82F, 0.72F, 0.9F, 1.0F, 0.5F};
    for (std::size_t face = 0; face < faces.size(); ++face) {
        for (const auto& indices : {std::array<int, 3>{0, 1, 2}, std::array<int, 3>{0, 2, 3}}) {
            SceneTriangle triangle;
            triangle.object = object;
            for (std::size_t index = 0; index < 3; ++index) {
                auto point = points[static_cast<std::size_t>(faces[face][indices[index]])];
                point = add(center, {point.x * extent.x, point.y * extent.y, point.z * extent.z});
                triangle.vertices[index] = {camera.project(point, aspect),
                                            scale(color, light[face])};
            }
            scene.push_back(triangle);
        }
    }
}

void finite(float value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("non-finite camera input");
    }
}

} // namespace

void LabCamera::orbit(float radians) {
    finite(radians);
    yaw_ = std::remainder(yaw_ + radians, 6.2831853F);
}
void LabCamera::tilt(float radians) {
    finite(radians);
    pitch_ = std::clamp(pitch_ + radians, 0.25F, 1.25F);
}
void LabCamera::zoom(float amount) {
    finite(amount);
    height_ = std::clamp(height_ + amount, 4.5F, 16.0F);
}

Vec3 LabCamera::project(Vec3 point, float aspect) const {
    if (!(aspect > 0) || !std::isfinite(aspect)) {
        throw std::invalid_argument("invalid viewport aspect");
    }
    const Vec3 right{std::cos(yaw_), 0, -std::sin(yaw_)};
    const Vec3 up{-std::sin(yaw_) * std::sin(pitch_), std::cos(pitch_),
                  -std::cos(yaw_) * std::sin(pitch_)};
    const Vec3 forward{-std::sin(yaw_) * std::cos(pitch_), -std::sin(pitch_),
                       -std::cos(yaw_) * std::cos(pitch_)};
    point.y -= 0.7F;
    return {dot(point, right) / (height_ * aspect), dot(point, up) / height_,
            (dot(point, forward) + 30.0F) / 60.0F};
}

GroundPoint LabCamera::ground_direction(float right, float forward) const {
    finite(right);
    finite(forward);
    return {right * std::cos(yaw_) - forward * std::sin(yaw_),
            -right * std::sin(yaw_) - forward * std::cos(yaw_)};
}
CameraRay LabCamera::ray(float x, float y, float aspect) const {
    finite(x);
    finite(y);
    static_cast<void>(project({}, aspect));
    const SpatialPoint right{std::cos(yaw_), 0, -std::sin(yaw_)};
    const SpatialPoint up{-std::sin(yaw_) * std::sin(pitch_), std::cos(pitch_),
                          -std::cos(yaw_) * std::sin(pitch_)};
    const SpatialPoint forward{-std::sin(yaw_) * std::cos(pitch_), -std::sin(pitch_),
                               -std::cos(yaw_) * std::cos(pitch_)};
    return {right * (x * height_ * aspect) + up * (y * height_) + forward * (-30) +
                SpatialPoint{0, .7, 0},
            forward};
}
GroundPoint LabCamera::ground_point(float x, float y, float aspect) const {
    finite(x);
    finite(y);
    static_cast<void>(project({}, aspect));
    const auto right = x * height_ * aspect;
    const auto forward = (y * height_ + .7F * std::cos(pitch_)) / std::sin(pitch_);
    return ground_direction(right, forward);
}

std::vector<SceneTriangle> build_motion_scene(const MotionSession& session, int selected,
                                              const LabCamera& camera, float aspect,
                                              double frame_fraction) {
    std::vector<SceneTriangle> scene;
    const auto poses = session.presentation_positions(frame_fraction);
    scene.reserve(1000);
    box(scene, camera, aspect, {0, -.1F, 0}, {10, .1F, 7}, {.14F, .21F, .24F});
    for (int line = -9; line <= 9; ++line) {
        box(scene, camera, aspect, {static_cast<float>(line), .005F, 0}, {.008F, .005F, 7},
            {.22F, .29F, .31F});
    }
    for (int line = -6; line <= 6; ++line) {
        box(scene, camera, aspect, {0, .005F, static_cast<float>(line)}, {10, .005F, .008F},
            {.22F, .29F, .31F});
    }
    for (const auto& obstacle : session.geometry().obstacles) {
        const auto& r = obstacle.footprint;
        box(scene, camera, aspect,
            {static_cast<float>((r.minimum.x_m + r.maximum.x_m) / 2),
             static_cast<float>((obstacle.bottom_m + obstacle.top_m) / 2),
             static_cast<float>((r.minimum.z_m + r.maximum.z_m) / 2)},
            {static_cast<float>((r.maximum.x_m - r.minimum.x_m) / 2),
             static_cast<float>((obstacle.top_m - obstacle.bottom_m) / 2),
             static_cast<float>((r.maximum.z_m - r.minimum.z_m) / 2)},
            obstacle.key >= 6 ? Vec3{.8F, .25F, .15F} : Vec3{.43F, .45F, .48F});
    }
    for (std::size_t index = 0; index < session.actors().size(); ++index) {
        const auto& actor = session.actors()[index];
        const auto pose = poses[index];
        const Vec3 color = index == 0 ? Vec3{.22F, .63F, .98F} : Vec3{.98F, .66F, .2F};
        box(scene, camera, aspect,
            {static_cast<float>(pose.x_m), static_cast<float>(actor.size.height_m / 2),
             static_cast<float>(pose.z_m)},
            {static_cast<float>(actor.size.half_x_m), static_cast<float>(actor.size.height_m / 2),
             static_cast<float>(actor.size.half_z_m)},
            color, static_cast<int>(index));
        if (selected == static_cast<int>(index)) {
            box(scene, camera, aspect,
                {static_cast<float>(pose.x_m), static_cast<float>(actor.size.height_m + .12),
                 static_cast<float>(pose.z_m)},
                {.4F, .04F, .4F}, {.75F, 1, .8F}, static_cast<int>(index));
        }
        if (actor.goal) {
            box(scene, camera, aspect,
                {static_cast<float>(actor.goal->x_m), .03F, static_cast<float>(actor.goal->z_m)},
                {.25F, .02F, .25F}, color);
        }
        // Bound visualization independently of imported route length; inspector retains full count.
        const auto shown = std::min(actor.route.points.size(), actor.next_point + 128);
        for (std::size_t p = actor.next_point; p < shown; ++p) {
            const auto waypoint = actor.route.points[p];
            const auto route_color = actor.route.geometry_version == session.geometry().version
                                         ? color
                                         : Vec3{.8F, .3F, .3F};
            box(scene, camera, aspect,
                {static_cast<float>(waypoint.x_m), .025F, static_cast<float>(waypoint.z_m)},
                {.07F, .015F, .07F}, route_color);
        }
    }
    return scene;
}

std::vector<SceneTriangle> build_lab_scene(const LabSceneView& view, const LabCamera& camera,
                                           float aspect) {
    std::vector<SceneTriangle> scene;
    scene.reserve(600);
    box(scene, camera, aspect, {0, -0.2F, 0}, {6, 0.2F, 6}, {0.16F, 0.22F, 0.28F});
    for (int line = -5; line <= 5; ++line) {
        const auto offset = static_cast<float>(line);
        box(scene, camera, aspect, {offset, 0.01F, 0}, {0.008F, 0.006F, 5.5F},
            {0.25F, 0.33F, 0.39F});
        box(scene, camera, aspect, {0, 0.01F, offset}, {5.5F, 0.006F, 0.008F},
            {0.25F, 0.33F, 0.39F});
    }
    for (int actor = 0; actor < 2; ++actor) {
        const float x = actor == 0 ? -3.0F : 3.0F;
        const Vec3 color = actor == 0 ? Vec3{0.25F, 0.65F, 0.95F} : Vec3{0.95F, 0.65F, 0.25F};
        box(scene, camera, aspect, {x, 0.1F, 0}, {1, 0.1F, 1},
            view.selected == actor ? Vec3{0.7F, 0.95F, 0.8F} : Vec3{0.3F, 0.4F, 0.48F}, actor);
        box(scene, camera, aspect, {x, 0.9F, 0}, {0.5F, 0.7F, 0.35F}, color, actor);
        box(scene, camera, aspect, {x, 1.95F, 0}, {0.36F, 0.35F, 0.35F}, color, actor);
        if (view.running[static_cast<std::size_t>(actor)]) {
            box(scene, camera, aspect, {x, 2.6F, 0}, {0.45F, 0.08F, 0.1F}, {0.4F, 1.0F, 0.55F},
                actor);
        }
    }
    box(scene, camera, aspect, {0, 0.15F, 3}, {1.3F, 0.15F, 0.75F},
        view.selected == 2 ? Vec3{0.7F, 0.95F, 0.8F} : Vec3{0.3F, 0.4F, 0.48F}, 2);
    const auto shown = std::min(view.quantity, std::uint64_t{3});
    for (std::uint64_t index = 0; index < shown; ++index) {
        const Vec3 color =
            index < view.reserved ? Vec3{0.85F, 0.55F, 0.25F} : Vec3{0.35F, 0.85F, 0.65F};
        box(scene, camera, aspect, {-0.8F + static_cast<float>(index) * 0.8F, 0.65F, 3},
            {0.3F, 0.35F, 0.35F}, color, 2);
    }
    return scene;
}

std::optional<int> pick_scene(const std::vector<SceneTriangle>& scene, float x, float y) {
    std::optional<int> result;
    float nearest = std::numeric_limits<float>::max();
    for (const auto& triangle : scene) {
        const auto a = triangle.vertices[0].position;
        const auto b = triangle.vertices[1].position;
        const auto c = triangle.vertices[2].position;
        const auto denominator = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
        if (std::abs(denominator) < 0.0000001F) {
            continue;
        }
        const auto first = ((b.y - c.y) * (x - c.x) + (c.x - b.x) * (y - c.y)) / denominator;
        const auto second = ((c.y - a.y) * (x - c.x) + (a.x - c.x) * (y - c.y)) / denominator;
        const auto third = 1.0F - first - second;
        const auto depth = first * a.z + second * b.z + third * c.z;
        if (first >= 0 && second >= 0 && third >= 0 && depth >= 0 && depth <= 1 &&
            depth < nearest) {
            nearest = depth;
            result = triangle.object >= 0 ? std::optional<int>{triangle.object} : std::nullopt;
        }
    }
    return result;
}

} // namespace reality
