#include "core/simulation_clock.hpp"

#include <limits>
#include <stdexcept>

namespace reality {

SimulationClock::SimulationClock(std::chrono::nanoseconds step) : step_(step) {
    if (step_.count() <= 0) {
        throw std::invalid_argument("simulation step must be positive");
    }
}

void SimulationClock::advance(std::uint64_t steps) {
    const auto maximum_ticks =
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max() / step_.count());
    if (steps > maximum_ticks - ticks_) {
        throw std::overflow_error("simulation time overflow");
    }
    ticks_ += steps;
}

std::uint64_t SimulationClock::tick_count() const noexcept { return ticks_; }

std::chrono::nanoseconds SimulationClock::step() const noexcept { return step_; }

std::chrono::nanoseconds SimulationClock::elapsed() const noexcept {
    return std::chrono::nanoseconds{static_cast<std::int64_t>(ticks_) * step_.count()};
}

} // namespace reality
