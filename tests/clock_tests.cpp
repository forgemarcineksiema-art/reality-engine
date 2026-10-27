#include "core/simulation_clock.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace std::chrono_literals;

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string{message});
    }
}

template <typename Exception, typename Operation> void require_throws(Operation operation) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error("expected exception was not thrown");
}

void initial_state() {
    const reality::SimulationClock clock{7ms};
    require(clock.tick_count() == 0, "new clock has nonzero tick count");
    require(clock.elapsed() == 0ns, "new clock has nonzero elapsed time");
    require(clock.step() == 7ms, "step changed during construction");
}

void reject_zero_step() {
    require_throws<std::invalid_argument>([] { reality::SimulationClock clock{0ns}; });
}

void reject_negative_step() {
    require_throws<std::invalid_argument>([] { reality::SimulationClock clock{-1ns}; });
}

void exact_elapsed_time() {
    reality::SimulationClock clock{7ms};
    clock.advance(3);
    require(clock.tick_count() == 3, "incorrect tick count after batch");
    require(clock.elapsed() == 21ms, "incorrect elapsed time after batch");
    clock.advance();
    require(clock.tick_count() == 4, "single advance did not add one tick");
    require(clock.elapsed() == 28ms, "incorrect elapsed time after single tick");
}

void batch_equivalence() {
    reality::SimulationClock individual{7ms};
    reality::SimulationClock batch{7ms};
    for (int index = 0; index < 100; ++index) {
        individual.advance();
    }
    batch.advance(100);
    require(individual.tick_count() == batch.tick_count(), "batch changed tick count");
    require(individual.elapsed() == batch.elapsed(), "batch changed elapsed time");
    require(batch.elapsed() == 700ms, "elapsed time differs from independent expectation");
}

void zero_batch() {
    reality::SimulationClock clock{7ms};
    clock.advance(2);
    clock.advance(0);
    require(clock.tick_count() == 2 && clock.elapsed() == 14ms, "zero batch mutated state");
}

void overflow_preserves_state() {
    const auto maximum = std::chrono::nanoseconds{std::numeric_limits<std::int64_t>::max()};
    reality::SimulationClock clock{maximum};
    clock.advance();
    require_throws<std::overflow_error>([&clock] { clock.advance(); });
    require(clock.tick_count() == 1, "overflow mutated tick count");
    require(clock.elapsed() == maximum, "overflow mutated elapsed time");
}

void reject_large_batch() {
    reality::SimulationClock clock{1ns};
    require_throws<std::overflow_error>(
        [&clock] { clock.advance(std::numeric_limits<std::uint64_t>::max()); });
    require(clock.tick_count() == 0 && clock.elapsed() == 0ns, "rejected batch mutated clock");
    clock.advance();
    require(clock.elapsed() == 1ns, "clock is unusable after rejected batch");
}

void intentional_failure() { require(false, "INTENTIONAL TEST FAILURE"); }

struct TestCase {
    std::string_view name;
    void (*run)();
};

constexpr std::array cases{
    TestCase{"initial_state", initial_state},
    TestCase{"reject_zero_step", reject_zero_step},
    TestCase{"reject_negative_step", reject_negative_step},
    TestCase{"exact_elapsed_time", exact_elapsed_time},
    TestCase{"batch_equivalence", batch_equivalence},
    TestCase{"zero_batch", zero_batch},
    TestCase{"overflow_preserves_state", overflow_preserves_state},
    TestCase{"reject_large_batch", reject_large_batch},
};

bool run_case(const TestCase& test) {
    try {
        test.run();
        std::cout << "PASS " << test.name << '\n';
        return true;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
    } catch (...) {
        std::cerr << "FAIL " << test.name << ": unknown exception\n";
    }
    return false;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view{argv[1]} == "--self-test-failure") {
        return run_case(TestCase{"intentional_failure", intentional_failure}) ? 0 : 1;
    }
    if (argc == 2 && std::string_view{argv[1]} == "--list") {
        for (const auto& test : cases) {
            std::cout << test.name << '\n';
        }
        return 0;
    }
    if (argc == 3 && std::string_view{argv[1]} == "--case") {
        for (const auto& test : cases) {
            if (test.name == argv[2]) {
                return run_case(test) ? 0 : 1;
            }
        }
        std::cerr << "ERROR: unknown test case\n";
        return 2;
    }
    if (argc != 1) {
        std::cerr << "Usage: reality_tests [--list | --case NAME | --self-test-failure]\n";
        return 2;
    }
    int failed = 0;
    for (const auto& test : cases) {
        if (!run_case(test)) {
            ++failed;
        }
    }
    return failed == 0 ? 0 : 1;
}
