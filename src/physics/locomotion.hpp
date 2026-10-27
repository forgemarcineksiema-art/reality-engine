#pragma once

#include <optional>

namespace reality {
struct LocomotionTuning {
    double maximum_speed = 3;
    double acceleration = 4;
    double braking = 6;
    double jerk = 30;
    double turn_speed = 4;
    double turn_acceleration = 12;
    double lateral_acceleration = 6;
    void validate() const;
};
struct LocomotionState {
    double speed = 0;
    double acceleration = 0;
    double heading = 0;
    double turn_speed = 0;
    bool operator==(const LocomotionState&) const = default;
};
struct LocomotionCommand {
    double heading = 0;
    double speed = 0;
    std::optional<double> stop_distance;
};
struct LocomotionStep {
    LocomotionState state;
    double distance = 0;
};
void validate(LocomotionState state, LocomotionTuning tuning);
[[nodiscard]] double braking_distance(double speed, double acceleration, LocomotionTuning tuning);
// Limits apply to forward speed/acceleration/jerk and yaw rate/acceleration.
// Collision impulses are handled by the caller and are not smooth braking commands.
[[nodiscard]] LocomotionStep advance_locomotion(LocomotionState state, LocomotionCommand command,
                                                LocomotionTuning tuning, double seconds);
} // namespace reality
