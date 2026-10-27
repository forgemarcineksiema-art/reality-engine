#include "physics/locomotion.hpp"

#include "physics/spatial_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace reality {
namespace {
void check(bool condition, const char* message) {
    if (!condition) {
        throw std::invalid_argument(message);
    }
}
double integral(double v, double a, double jerk, double t) {
    return v * t + .5 * a * t * t + jerk * t * t * t / 6;
}
double stopping(double v, double a, const LocomotionTuning& tuning) {
    const auto peak = std::sqrt(std::max(0.0, tuning.jerk * v + a * a * .5));
    const auto brake = std::min(peak, tuning.braking);
    const auto down = std::max(0.0, (a + brake) / tuning.jerk);
    const auto first = integral(v, a, -tuning.jerk, down);
    const auto v1 = std::max(0.0, v + a * down - .5 * tuning.jerk * down * down);
    const auto hold =
        brake > 1e-12 ? std::max(0.0, (v1 - brake * brake / (2 * tuning.jerk)) / brake) : 0;
    const auto middle = v1 * hold - .5 * brake * hold * hold;
    const auto v2 = std::max(0.0, v1 - brake * hold);
    const auto up = brake / tuning.jerk;
    return std::max(0.0, first + middle + integral(v2, -brake, tuning.jerk, up));
}
LocomotionStep brake_step(LocomotionState state, const LocomotionTuning& tuning, double dt) {
    const auto peak = std::min(
        tuning.braking, std::sqrt(std::max(0.0, tuning.jerk * state.speed +
                                                    state.acceleration * state.acceleration * .5)));
    const auto down = std::max(0.0, (state.acceleration + peak) / tuning.jerk);
    const auto first_v = state.speed + state.acceleration * down - .5 * tuning.jerk * down * down;
    const auto hold =
        peak > 1e-12 ? std::max(0.0, (first_v - peak * peak / (2 * tuning.jerk)) / peak) : 0;
    const auto up = peak / tuning.jerk;
    double distance = 0, remaining = dt;
    const auto phase = [&](double duration, double jerk) {
        const auto t = std::min(remaining, duration);
        distance += integral(state.speed, state.acceleration, jerk, t);
        state.speed += state.acceleration * t + .5 * jerk * t * t;
        state.acceleration += jerk * t;
        remaining -= t;
    };
    phase(down, -tuning.jerk);
    phase(hold, 0);
    phase(up, tuning.jerk);
    if (remaining > 1e-12 || dt >= down + hold + up) {
        state.speed = 0;
        state.acceleration = 0;
    }
    state.speed = std::max(0.0, state.speed);
    return {state, std::max(0.0, distance)};
}
LocomotionStep linear_step(LocomotionState state, LocomotionCommand command,
                           const LocomotionTuning& tuning, double dt, unsigned events) {
    const auto v = state.speed, a = state.acceleration, j = tuning.jerk;
    if (command.speed == 0) {
        return brake_step(state, tuning, dt);
    }
    // The final release of acceleration may finish inside this fixed tick.
    // Integrate that event and the remaining hold/control time separately.
    if (events < 4 && std::abs(a) > 1e-10) {
        const auto boundary =
            a < 0 ? v - a * a / (2 * j) : tuning.maximum_speed - v - a * a / (2 * j);
        const auto duration = std::abs(a) / j;
        if (std::abs(boundary) < 1e-8 && duration <= dt) {
            const auto applied_jerk = a < 0 ? j : -j;
            const auto distance = integral(v, a, applied_jerk, duration);
            state.speed = a < 0 ? 0 : tuning.maximum_speed;
            state.acceleration = 0;
            if (command.stop_distance) {
                *command.stop_distance = std::max(0.0, *command.stop_distance - distance);
            }
            if (dt - duration > 1e-10) {
                auto remaining = linear_step(state, command, tuning, dt - duration, events + 1);
                remaining.distance += distance;
                return remaining;
            }
            return {state, distance};
        }
    }
    auto low = std::max(-tuning.braking, a - j * dt),
         high = std::min(tuning.acceleration, a + j * dt);
    const auto velocity = [&](double next_a) { return v + (a + next_a) * dt * .5; };
    const auto minimum = [&](double next_a) {
        const auto negative = std::min(0.0, next_a);
        return velocity(next_a) - negative * negative / (2 * j);
    };
    const auto maximum = [&](double next_a) {
        const auto positive = std::max(0.0, next_a);
        return velocity(next_a) + positive * positive / (2 * j);
    };
    if (minimum(low) < 0) {
        auto l = low, h = high;
        for (unsigned i = 0; i < 48; ++i) {
            const auto m = (l + h) * .5;
            if (minimum(m) < 0) {
                l = m;
            } else {
                h = m;
            }
        }
        low = h;
    }
    if (maximum(high) > tuning.maximum_speed) {
        auto l = low, h = high;
        for (unsigned i = 0; i < 48; ++i) {
            const auto m = (l + h) * .5;
            if (maximum(m) > tuning.maximum_speed) {
                h = m;
            } else {
                l = m;
            }
        }
        high = l;
    }
    check(low <= high + 1e-8, "locomotion state has no admissible jerk transition");
    high = std::max(low, high);
    const auto distance = [&](double next_a) { return integral(v, a, (next_a - a) / dt, dt); };
    if (command.stop_distance) {
        const auto can_stop = [&](double next_a) {
            return distance(next_a) + stopping(std::max(0.0, velocity(next_a)), next_a, tuning) <=
                   *command.stop_distance + 1e-9;
        };
        if (!can_stop(low)) {
            // An optimal brake may switch jerk within this tick. A single constant
            // jerk for the whole interval would lose that reachable stopping point.
            return brake_step(state, tuning, dt);
        } else if (!can_stop(high)) {
            auto l = low, h = high;
            for (unsigned i = 0; i < 48; ++i) {
                const auto m = (l + h) * .5;
                if (can_stop(m)) {
                    l = m;
                } else {
                    h = m;
                }
            }
            high = l;
        }
    }
    const auto wanted = (command.speed - v) / .2;
    const auto next_a = std::clamp(wanted, low, high);
    const auto next_v = velocity(next_a);
    check(next_v >= -1e-8 && next_v <= tuning.maximum_speed + 1e-8,
          "locomotion speed envelope violated");
    state.speed = std::clamp(next_v, 0.0, tuning.maximum_speed);
    state.acceleration = next_a;
    return {state, std::max(0.0, distance(next_a))};
}
} // namespace
void LocomotionTuning::validate() const {
    for (const auto value : {maximum_speed, acceleration, braking, jerk, turn_speed,
                             turn_acceleration, lateral_acceleration}) {
        check(std::isfinite(value) && value > 0 && value <= 1000, "invalid locomotion tuning");
    }
}
double braking_distance(double speed, double acceleration, LocomotionTuning tuning) {
    tuning.validate();
    check(std::isfinite(speed) && speed >= 0 && speed <= tuning.maximum_speed &&
              std::isfinite(acceleration) && acceleration >= -tuning.braking &&
              acceleration <= tuning.acceleration,
          "invalid braking state");
    return stopping(speed, acceleration, tuning);
}
void validate(LocomotionState state, LocomotionTuning tuning) {
    tuning.validate();
    check(std::isfinite(state.speed) && state.speed >= 0 && state.speed <= tuning.maximum_speed &&
              std::isfinite(state.acceleration) && state.acceleration >= -tuning.braking &&
              state.acceleration <= tuning.acceleration && std::isfinite(state.heading) &&
              std::abs(state.heading) <= 1000000 && std::isfinite(state.turn_speed) &&
              std::abs(state.turn_speed) <= tuning.turn_speed,
          "invalid locomotion state");
    const auto negative = std::min(0.0, state.acceleration),
               positive = std::max(0.0, state.acceleration);
    check(state.speed - negative * negative / (2 * tuning.jerk) >= -1e-8 &&
              state.speed + positive * positive / (2 * tuning.jerk) <= tuning.maximum_speed + 1e-8,
          "infeasible locomotion speed envelope");
}
LocomotionStep advance_locomotion(LocomotionState state, LocomotionCommand command,
                                  LocomotionTuning tuning, double seconds) {
    validate(state, tuning);
    check(std::isfinite(seconds) && seconds > 0 && seconds <= .1, "invalid locomotion interval");
    check(std::isfinite(command.heading) && std::abs(command.heading) <= 1000000 &&
              std::isfinite(command.speed) && command.speed >= 0 &&
              command.speed <= tuning.maximum_speed &&
              (!command.stop_distance ||
               (std::isfinite(*command.stop_distance) && *command.stop_distance >= 0 &&
                *command.stop_distance <= 2000000)),
          "invalid locomotion command");
    const auto angle = angle_delta(state.heading, command.heading);
    const auto curve_limit =
        std::min(tuning.turn_speed, tuning.lateral_acceleration / std::max(.1, state.speed));
    const auto wanted_turn = std::clamp(angle * 8, -curve_limit, curve_limit);
    const auto turn = std::clamp(wanted_turn, state.turn_speed - tuning.turn_acceleration * seconds,
                                 state.turn_speed + tuning.turn_acceleration * seconds);
    const auto heading = std::remainder(state.heading + (state.turn_speed + turn) * seconds * .5,
                                        2 * std::numbers::pi);
    command.speed *= std::max(0.0, std::cos(angle));
    auto result = linear_step(state, command, tuning, seconds, 0);
    result.state.turn_speed = turn;
    result.state.heading = heading;
    return result;
}
} // namespace reality
