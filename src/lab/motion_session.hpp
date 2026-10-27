#pragma once

#include "lab/lab_session.hpp"
#include "navigation/grid_navigation.hpp"

namespace reality {

enum class TravelState {
    Idle,
    Following,
    Arrived,
    NoPath,
    InvalidEndpoint,
    BudgetExceeded,
    Waiting
};
[[nodiscard]] std::string_view describe(TravelState state) noexcept;
struct MobileActor {
    EntityId id;
    GroundPoint position;
    BodySize size;
    std::optional<GroundPoint> goal;
    GroundRoute route;
    std::size_t next_point = 0;
    TravelState state = TravelState::Idle;
    std::uint64_t plans = 0;
};

// Non-owning reference: LabSession outlives this object and owns the single domain clock.
class MotionSession final {
  public:
    explicit MotionSession(LabSession& laboratory);
    [[nodiscard]] const WorkWorld& world() const noexcept;
    [[nodiscard]] const KinematicGeometry& geometry() const noexcept;
    [[nodiscard]] const std::array<MobileActor, 2>& actors() const noexcept;
    [[nodiscard]] std::array<GroundPoint, 2> presentation_positions(double fraction) const;
    void synchronize_presentation();
    [[nodiscard]] bool gate_closed(std::size_t gate) const;
    [[nodiscard]] std::optional<EntityId> interaction_target(std::size_t actor) const;
    [[nodiscard]] bool set_gate(std::size_t gate, bool closed);
    void set_goal(std::size_t actor, GroundPoint goal);
    void set_input(std::size_t actor, GroundPoint direction);
    void queue_input(std::size_t actor, GroundPoint direction);
    void clear_input() noexcept;
    [[nodiscard]] bool accept_route(std::size_t actor, GroundRoute route);
    void advance(std::uint64_t ticks);
    [[nodiscard]] std::string serialize() const;
    void restore(std::string_view bytes);
    void save(const std::filesystem::path& path) const;
    void load(const std::filesystem::path& path);
    [[nodiscard]] std::string inspect() const;

  private:
    LabSession& laboratory_;
    KinematicGeometry geometry_;
    std::array<bool, 2> gates_{};
    std::array<MobileActor, 2> actors_;
    std::array<GroundPoint, 2> inputs_{};
    std::array<GroundPoint, 2> queued_inputs_{};
    std::array<std::vector<MotionSample>, 2> presentation_;
};

} // namespace reality
