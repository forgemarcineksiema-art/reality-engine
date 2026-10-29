#include "lab/traversal_session.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
namespace reality {
namespace {
void check(bool c, const char* message) {
    if (!c)
        throw std::invalid_argument(message);
}
const DistantConnection* connection(const TraversalScene& s, std::uint32_t id) {
    const auto it = std::find_if(s.distant_connections.begin(), s.distant_connections.end(),
                                 [&](const auto& c) { return c.id == id; });
    return it == s.distant_connections.end() ? nullptr : &*it;
}
} // namespace
const std::vector<DistantConnection>& TraversalSession::distant_connections() const noexcept {
    return scene_.distant_connections;
}
bool TraversalSession::set_distant_open(std::uint32_t id, bool open) {
    for (auto& c : scene_.distant_connections)
        if (c.id == id) {
            c.open = open;
            return true;
        }
    return false;
}
DistantOutcome TraversalSession::depart(std::size_t index, std::uint32_t id, unsigned side) {
    auto& a = actors_.at(index);
    const auto* c = connection(scene_, id);
    if (!c || side > 1)
        return DistantOutcome::Unavailable;
    if (!a.local() || (a.task != TraversalTask::None && a.task != TraversalTask::Deliver) ||
        inputs_[index] != GroundPoint{} || queued_inputs_[index] != GroundPoint{} ||
        (practice_ && practice_->sharing_ &&
         (practice_->sharing_->from == index || practice_->sharing_->to == index)) ||
        protected_operation(a) || a.transfer != TransferStage::None || a.motion.speed > .01 ||
        a.pending_goal || a.stop_after_transfer ||
        a.clearance.outcome == ClearanceOutcome::Accepted)
        return DistantOutcome::Busy;
    const auto feet = surface_feet(geometry_, c->terminals[side], TraversalLimits{a.size});
    if (!feet || a.support.surface != c->terminals[side].surface || length(*feet - a.feet) > .015)
        return DistantOutcome::OutOfReach;
    if (!surface_feet(geometry_, c->terminals[1 - side], TraversalLimits{a.size}))
        return DistantOutcome::Unavailable;
    if (!c->open)
        return DistantOutcome::Closed;
    release_navigation(index);
    a.goal.reset();
    a.requested_goal.reset();
    a.requested_reach = 0;
    a.route = {};
    a.leg = 0;
    a.step_point = 0;
    a.motion.speed = 0;
    a.motion.acceleration = 0;
    a.state = TraversalState::Idle;
    a.blocked_ticks = 0;
    a.finish_grace_until = 0;
    inputs_[index] = {};
    queued_inputs_[index] = {};
    a.journey = DistantJourney{id, side, world_.clock().tick_count(), 0};
    return DistantOutcome::Departed;
}
void TraversalSession::advance_distant() {
    const auto now = world_.clock().tick_count();
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        auto& a = actors_[i];
        if (a.local())
            continue;
        auto& j = *a.journey;
        const auto& c = *connection(scene_, j.connection);
        const auto elapsed = std::min(c.travel_ticks, now - j.departed_at);
        const auto effort = elapsed - j.elapsed;
        a.energy = effort >= a.energy ? 0 : a.energy - effort;
        j.elapsed = elapsed;
        if (j.elapsed < c.travel_ticks)
            continue;
        const auto terminal = c.terminals[1 - j.side];
        const auto feet = surface_feet(geometry_, terminal, TraversalLimits{a.size});
        if (!feet || !passage_motion_clear(*feet, *feet, a.size) ||
            !spatial_occupiable(geometry_, *feet, a.size))
            continue;
        const auto occupied = std::any_of(actors_.begin(), actors_.end(), [&](const auto& other) {
            return other.local() && other.id != a.id &&
                   !capsule_paths_clear(*feet, *feet, a.size, other.feet, other.feet, other.size);
        });
        if (occupied)
            continue;
        a.support = terminal;
        a.feet = *feet;
        a.journey.reset();
        a.retry_at = now;
        a.need_retry_at = a.autonomous ? now : 0;
        a.finish_grace_until = 0;
        a.task = TraversalTask::None;
        history_[i] = {{0, *feet}, {1, *feet}};
        previous_supports_[i] = terminal.surface;
    }
}
bool TraversalSession::plan_distant_delivery(std::size_t index, SurfaceLocation destination) {
    auto& a = actors_[index];
    for (const auto& c : scene_.distant_connections)
        for (unsigned side = 0; side < 2; ++side) {
            const auto terminal = c.terminals[side];
            if (a.support.surface != terminal.surface ||
                destination.surface != c.terminals[1 - side].surface)
                continue;
            const auto feet = surface_feet(geometry_, terminal, TraversalLimits{a.size});
            if (feet && length(*feet - a.feet) <= .015) {
                if (a.motion.speed > .01)
                    return true;
                const auto outcome = depart(index, c.id, side);
                if (outcome != DistantOutcome::Departed) {
                    a.task = TraversalTask::None;
                    a.retry_at = world_.clock().tick_count() + 40;
                    a.goal.reset();
                    a.requested_goal.reset();
                    a.requested_reach = 0;
                    a.route = {};
                    a.leg = 0;
                    a.state = TraversalState::Idle;
                }
            } else {
                a.goal = terminal;
                a.requested_goal = terminal;
                a.requested_reach = 0;
                a.task = TraversalTask::Deliver;
                a.finish_grace_until = 0;
                plan(index, true);
            }
            return true;
        }
    return false;
}
void TraversalSession::validate_distant() const {
    check(scene_.distant_connections.size() <= 16, "too many distant connections");
    std::set<std::uint32_t> ids;
    for (const auto& c : scene_.distant_connections) {
        check(c.id != 0 && ids.insert(c.id).second && c.travel_ticks > 0 &&
                  c.travel_ticks <= 1200 && std::isfinite(c.distance_m) && c.distance_m > 0 &&
                  c.distance_m <= static_cast<double>(c.travel_ticks) * .15 + 1e-9,
              "invalid distant distance or travel horizon");
        std::array<SpatialPoint, 2> points;
        for (unsigned side = 0; side < 2; ++side) {
            const auto terminal = c.terminals[side];
            check(
                std::none_of(scene_.platforms.begin(), scene_.platforms.end(),
                             [&](const auto& p) { return p.definition.body == terminal.surface; }),
                "distant terminal is moving");
            const auto feet = surface_feet(geometry_, terminal, TraversalLimits{});
            check(feet.has_value(), "invalid distant terminal");
            points[side] = *feet;
        }
        check(c.terminals[0].surface != c.terminals[1].surface &&
                  c.distance_m + 1e-9 >= length(points[1] - points[0]),
              "invalid distant geometry");
    }
    const auto now = world_.clock().tick_count();
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        const auto& a = actors_[i];
        if (a.local())
            continue;
        const auto& j = *a.journey;
        const auto* c = connection(scene_, j.connection);
        check(c && j.side <= 1, "unknown distant itinerary");
        check(j.departed_at <= now && j.elapsed == std::min(c->travel_ticks, now - j.departed_at),
              "invalid distant progress");
        const auto feet = surface_feet(geometry_, c->terminals[j.side], TraversalLimits{a.size});
        const auto anchor = surface_feet(geometry_, a.support, TraversalLimits{a.size});
        check(anchor && length(*anchor - a.feet) < 1e-6, "invalid distant historical anchor");
        check(!practice_ || !practice_->sharing_ ||
                  (practice_->sharing_->from != i && practice_->sharing_->to != i),
              "distant practice conversation in flight");
        check(feet && a.support.surface == c->terminals[j.side].surface &&
                  length(*feet - a.feet) <= .015 && !protected_operation(a) &&
                  a.transfer == TransferStage::None && !a.goal && !a.requested_goal &&
                  !a.pending_goal && a.route.legs.empty() && a.leg == 0 && a.step_point == 0 &&
                  a.motion.speed == 0 && a.motion.acceleration == 0 && !a.stop_after_transfer &&
                  a.finish_grace_until == 0 && a.requested_reach == 0 && a.pending_reach == 0 &&
                  a.state == TraversalState::Idle && a.blocked_ticks == 0 &&
                  a.clearance.outcome != ClearanceOutcome::Accepted &&
                  (a.task == TraversalTask::None || a.task == TraversalTask::Deliver),
              "distant actor has local execution");
        for (const auto& p : scene_.passages)
            check(p.owner != a.id &&
                      std::find(p.queue.begin(), p.queue.end(), a.id) == p.queue.end(),
                  "distant actor owns passage");
        for (const auto& p : scene_.services)
            check(p.owner != a.id &&
                      std::find(p.queue.begin(), p.queue.end(), a.id) == p.queue.end(),
                  "distant actor owns ferry");
    }
}
} // namespace reality
