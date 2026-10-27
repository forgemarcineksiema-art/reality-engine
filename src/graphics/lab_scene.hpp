#pragma once

#include "navigation/surface_navigation.hpp"
#include "physics/spatial_geometry.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace reality {
inline constexpr unsigned laboratory_vertex_capacity = 16384;

class MotionSession;
class TraversalSession;
struct CameraRay {
    SpatialPoint origin;
    SpatialPoint direction;
};

struct Vec3 {
    float x = 0;
    float y = 0;
    float z = 0;
};
struct SceneVertex {
    Vec3 position;
    Vec3 color;
};
struct SceneTriangle {
    std::array<SceneVertex, 3> vertices;
    int object = -1;
};
struct LabSceneView {
    int selected = 0;
    std::uint64_t quantity = 3;
    std::uint64_t reserved = 0;
    std::array<bool, 2> running{};
};

class LabCamera final {
  public:
    void orbit(float radians);
    void tilt(float radians);
    void zoom(float amount);
    [[nodiscard]] Vec3 project(Vec3 point, float aspect) const;
    [[nodiscard]] GroundPoint ground_point(float normalized_x, float normalized_y,
                                           float aspect) const;
    [[nodiscard]] GroundPoint ground_direction(float right, float forward) const;
    [[nodiscard]] CameraRay ray(float normalized_x, float normalized_y, float aspect) const;

  private:
    float yaw_ = 0.55F;
    float pitch_ = 0.65F;
    float height_ = 7.5F;
};

[[nodiscard]] std::vector<SceneTriangle> build_lab_scene(const LabSceneView& view,
                                                         const LabCamera& camera, float aspect);
[[nodiscard]] std::optional<int> pick_scene(const std::vector<SceneTriangle>& scene,
                                            float normalized_x, float normalized_y);
[[nodiscard]] std::vector<SceneTriangle> build_motion_scene(const MotionSession& session,
                                                            int selected, const LabCamera& camera,
                                                            float aspect,
                                                            double frame_fraction = 1);
[[nodiscard]] std::vector<SceneTriangle>
build_traversal_scene(const TraversalSession& session, int selected, const LabCamera& camera,
                      float aspect, double frame_fraction = 1);
[[nodiscard]] std::optional<SurfaceLocation> pick_traversal_surface(const TraversalSession& session,
                                                                    const LabCamera& camera,
                                                                    float x, float y, float aspect,
                                                                    double fraction, bool approach);

} // namespace reality
