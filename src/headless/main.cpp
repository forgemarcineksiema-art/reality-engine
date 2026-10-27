#include "core/simulation_clock.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace {

std::uint64_t parse_unsigned(std::string_view text) {
    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        throw std::invalid_argument("invalid unsigned integer");
    }
    return value;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        std::uint64_t steps = 10;
        std::uint64_t step_ms = 50;
        bool saw_steps = false;
        bool saw_step_ms = false;

        if (argc == 2 && std::string_view{argv[1]} == "--help") {
            std::cout << "Usage: reality_headless [--steps N] [--step-ms N]\n";
            return 0;
        }
        for (int index = 1; index < argc; ++index) {
            const std::string_view option{argv[index]};
            if (index + 1 >= argc) {
                throw std::invalid_argument("expected option value");
            }
            const auto value = parse_unsigned(argv[++index]);
            if (option == "--steps" && !saw_steps) {
                steps = value;
                saw_steps = true;
            } else if (option == "--step-ms" && !saw_step_ms) {
                step_ms = value;
                saw_step_ms = true;
            } else {
                throw std::invalid_argument("unknown or duplicate option");
            }
        }

        if (step_ms == 0) {
            throw std::invalid_argument("step-ms must be positive");
        }
        constexpr auto nanoseconds_per_millisecond = std::uint64_t{1'000'000};
        const auto maximum_step_ms =
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) /
            nanoseconds_per_millisecond;
        if (step_ms > maximum_step_ms) {
            throw std::overflow_error("step-ms exceeds nanosecond range");
        }
        const auto step = std::chrono::nanoseconds{
            static_cast<std::int64_t>(step_ms * nanoseconds_per_millisecond)};
        reality::SimulationClock clock{step};
        clock.advance(steps);
        std::cout << "ticks=" << clock.tick_count() << " step_ns=" << clock.step().count()
                  << " elapsed_ns=" << clock.elapsed().count() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
