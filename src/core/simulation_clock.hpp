#pragma once

#include <chrono>
#include <cstdint>

namespace reality {

class SimulationClock final {
  public:
    explicit SimulationClock(std::chrono::nanoseconds step);

    void advance(std::uint64_t steps = 1);

    [[nodiscard]] std::uint64_t tick_count() const noexcept;
    [[nodiscard]] std::chrono::nanoseconds step() const noexcept;
    [[nodiscard]] std::chrono::nanoseconds elapsed() const noexcept;

  private:
    std::chrono::nanoseconds step_;
    std::uint64_t ticks_ = 0;
};

} // namespace reality
