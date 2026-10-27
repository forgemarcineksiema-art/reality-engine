#pragma once

#include "persistence/world_snapshot.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace reality {

class LabSession final {
  public:
    LabSession();
    [[nodiscard]] const WorkWorld& world() const noexcept;
    [[nodiscard]] const std::array<EntityId, 2>& actors() const noexcept;
    [[nodiscard]] EntityId lot() const noexcept;
    [[nodiscard]] std::optional<ActionRecord> latest_action(std::size_t actor) const;
    [[nodiscard]] RequestResult reserve(std::size_t actor);
    [[nodiscard]] Transition begin(std::size_t actor);
    [[nodiscard]] Transition cancel(std::size_t actor);
    void advance(std::uint64_t ticks);
    void save(const std::filesystem::path& path) const;
    void load(const std::filesystem::path& path);
    void restore(WorkWorld candidate);

  private:
    WorkWorld world_;
    std::array<EntityId, 2> actors_{};
    EntityId lot_;
};

class SimulationPacer final {
  public:
    void set_running(bool running) noexcept;
    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] std::chrono::nanoseconds pending() const noexcept;
    template <typename Session> void update(Session& session, std::chrono::nanoseconds elapsed) {
        if (elapsed.count() < 0) {
            throw std::invalid_argument("negative real-time interval");
        }
        if (!running_) {
            return;
        }
        if (elapsed.count() > std::numeric_limits<std::int64_t>::max() - pending_.count()) {
            throw std::overflow_error("real-time backlog overflow");
        }
        pending_ += elapsed;
        const auto step = session.world().clock().step();
        const auto ticks = std::min(std::uint64_t{20}, static_cast<std::uint64_t>(pending_ / step));
        for (std::uint64_t tick = 0; tick < ticks; ++tick) {
            session.advance(1);
            pending_ -= step;
        }
    }
    void reset() noexcept;

  private:
    bool running_ = false;
    std::chrono::nanoseconds pending_{};
};

} // namespace reality
