#pragma once

#include "lab/traversal_session.hpp"

namespace reality {
enum class NavigationActivity {
    Inactive,
    Travelling,
    Waiting,
    Transfer,
    NoPath,
    Arrived,
    BudgetExceeded,
    InvalidGoal,
    Parking,
    Precision,
    Riding,
    SuspectedStall
};
struct NavigationSample {
    EntityId actor;
    std::uint64_t tick = 0;
    NavigationActivity activity = NavigationActivity::Inactive;
    std::optional<SurfaceLocation> goal, waypoint;
    double distance_m = 0;
    std::optional<SpatialPoint> position;
    std::uint32_t support = 0;
    std::uint32_t resource = 0;
    EntityId blocker;
    std::size_t queue_position = 0;
    std::uint64_t progress = 0;
    std::uint64_t stall_window = 100;
    bool transport = false;
    bool externally_blocked = false;
};
class NavigationProgressWatch final {
  public:
    void observe(NavigationSample sample);
    void reset() noexcept;
    [[nodiscard]] NavigationActivity activity() const noexcept;
    [[nodiscard]] std::string report() const;
    [[nodiscard]] bool new_alarm() const noexcept;
    [[nodiscard]] EntityId actor() const noexcept;

  private:
    std::optional<NavigationSample> previous_;
    NavigationActivity activity_ = NavigationActivity::Inactive;
    double best_distance_m_ = 0;
    std::uint64_t last_progress_ = 0;
    std::uint64_t stagnant_ticks_ = 0;
    std::optional<SpatialPoint> origin_;
    bool alarm_episode_ = false, new_alarm_ = false;
};
class TraversalDiagnostics final {
  public:
    void observe(const TraversalSession& session);
    void observe(std::span<const NavigationSample> samples);
    void reset() noexcept;
    [[nodiscard]] bool suspected_stall() const noexcept;
    [[nodiscard]] const std::vector<EntityId>& new_stalls() const noexcept;
    [[nodiscard]] std::string report() const;
    void capture(const TraversalSession& session, const std::filesystem::path& directory,
                 std::string_view reason) const;

  private:
    std::vector<NavigationProgressWatch> watches_;
    std::vector<EntityId> new_stalls_;
};
} // namespace reality
