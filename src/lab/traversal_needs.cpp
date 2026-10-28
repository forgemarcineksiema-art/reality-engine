#include "lab/traversal_session.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace reality {
namespace {
void check(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
bool committed(const TraversalActor& actor) {
    return actor.transfer == TransferStage::Boarding || actor.transfer == TransferStage::Riding ||
           actor.transfer == TransferStage::Exiting ||
           (actor.leg < actor.route.legs.size() &&
            actor.route.legs[actor.leg].kind == SurfaceLegKind::Step && actor.step_point > 0);
}
} // namespace
bool TraversalSession::select_task(std::size_t index, TraversalTask wanted,
                                   std::optional<std::size_t> preferred) {
    auto& actor = actors_[index];
    const auto now = world_.clock().tick_count();
    std::vector<std::size_t> known;
    for (std::size_t station = 0; station < scene_.stations.size(); ++station)
        if ((actor.known_stations & (std::uint32_t{1} << station)) != 0 &&
            scene_.stations[station].kind == wanted && now >= actor.station_retry[station] &&
            !belief_unavailable(actor, station))
            known.push_back(station);
    std::stable_sort(known.begin(), known.end(), [&](auto a, auto b) {
        if (preferred && (a == *preferred || b == *preferred))
            return a == *preferred && b != *preferred;
        const auto& sa = scene_.stations[a].location;
        const auto& sb = scene_.stations[b].location;
        return length(geometry_.body(sa.surface).world({sa.local.x_m, 0, sa.local.z_m}) -
                      actor.feet) <
               length(geometry_.body(sb.surface).world({sb.local.x_m, 0, sb.local.z_m}) -
                      actor.feet);
    });
    for (const auto station : known) {
        if (choose_approach(index, scene_.stations[station].location, .9, true)) {
            actor.task = wanted;
            actor.station = static_cast<std::uint32_t>(station);
            actor.finish_grace_until = 0;
            std::erase_if(actor.suspended, [&](const auto& task) { return task.task == wanted; });
            return true;
        }
    }
    return false;
}
bool TraversalSession::interrupt_for_need(std::size_t index) {
    auto& actor = actors_[index];
    const auto now = world_.clock().tick_count();
    if (!actor.autonomous || actor.action.value != 0 || practice_busy(index) || committed(actor) ||
        actor.task == TraversalTask::None || now < actor.need_retry_at)
        return false;
    actor.need_retry_at = 0;
    const auto urgent_hunger = actor.hunger >= 750 && (actor.task == TraversalTask::Maintain ||
                                                       actor.task == TraversalTask::Rest ||
                                                       actor.task == TraversalTask::Deliver);
    const auto urgent_rest = actor.energy <= 100 && (actor.task == TraversalTask::Maintain ||
                                                     actor.task == TraversalTask::Deliver);
    if (!urgent_hunger && !urgent_rest)
        return false;
    const auto may_finish =
        actor.task != TraversalTask::Deliver ||
        (actor.delivery && actor.delivery->phase == DeliveryPhase::ToDestination);
    if (may_finish && actor.finish_grace_until == 0 && actor.goal &&
        actor.route.status == RouteStatus::Found &&
        (actor.state == TraversalState::Arrived ||
         (actor.state == TraversalState::Moving && actor.leg + 1 == actor.route.legs.size() &&
          actor.route.legs[actor.leg].kind == SurfaceLegKind::Walk))) {
        const auto goal = surface_feet(geometry_, *actor.goal, TraversalLimits{actor.size});
        if (goal && length(*goal - actor.feet) <= 6)
            actor.finish_grace_until = now + 120;
    }
    if (now < actor.finish_grace_until)
        return false;
    for (const auto wanted : {TraversalTask::Eat, TraversalTask::Rest}) {
        const auto urgent = wanted == TraversalTask::Eat
                                ? actor.hunger >= 750 && (actor.task == TraversalTask::Maintain ||
                                                          actor.task == TraversalTask::Rest ||
                                                          actor.task == TraversalTask::Deliver)
                                : actor.energy <= 100 && (actor.task == TraversalTask::Maintain ||
                                                          actor.task == TraversalTask::Deliver);
        if (!urgent)
            continue;
        actor.need_retry_at = now + 40;
        const SuspendedTraversalTask previous{actor.task, actor.station};
        if (!select_task(index, wanted))
            continue;
        if (previous.task != TraversalTask::Deliver) {
            check(actor.suspended.size() < 2, "suspended task budget exhausted");
            actor.suspended.push_back(previous);
        }
        release_navigation(index);
        actor.pending_goal.reset();
        actor.pending_reach = 0;
        actor.stop_after_transfer = false;
        actor.blocked_ticks = 0;
        return true;
    }
    return false;
}
void TraversalSession::decide(std::size_t index) {
    auto& actor = actors_[index];
    const auto now = world_.clock().tick_count();
    if (actor.yielding)
        return;
    if (actor.action.value != 0) {
        const auto action = world_.action(actor.action);
        check(action.has_value(), "missing autonomous action");
        if (action->state == ActionState::Completed || action->state == ActionState::Cancelled) {
            if (action->state == ActionState::Completed) {
                auto& station = scene_.stations.at(actor.station);
                ++station.completed;
                if (actor.task == TraversalTask::Eat) {
                    actor.hunger = 0;
                }
                if (actor.task == TraversalTask::Rest) {
                    actor.energy = 1000;
                }
                if (actor.task == TraversalTask::Maintain) {
                    station.wear = station.wear > 250 ? station.wear - 250 : 0;
                    repair_passages(index, actor.station);
                }
            }
            observe_station(index, actor.station);
            actor.action = {};
            actor.task = TraversalTask::None;
            actor.state = TraversalState::Idle;
            actor.retry_at = now + 10;
            actor.goal.reset();
            actor.route = {};
            actor.leg = 0;
            actor.need_retry_at = 0;
            actor.finish_grace_until = 0;
        }
        return;
    }
    if (!actor.autonomous || practice_busy(index) || committed(actor)) {
        return;
    }
    reconsider_belief(index);
    if (interrupt_for_need(index))
        return;
    if (actor.transfer != TransferStage::None)
        return;
    if (actor.goal && actor.state == TraversalState::Arrived &&
        actor.task == TraversalTask::Deliver) {
        arrive_delivery(index);
        return;
    }
    if (actor.goal && actor.state == TraversalState::Arrived && actor.task == TraversalTask::None) {
        // A manual route retained to finish a committed step/ride is not a station task.
        release_navigation(index);
        actor.goal.reset();
        actor.requested_goal.reset();
        actor.requested_reach = 0;
        actor.route = {};
        actor.leg = 0;
        actor.state = TraversalState::Idle;
        actor.retry_at = now;
    }
    if (actor.goal && actor.state == TraversalState::Arrived) {
        if (actor.motion.speed > .01) {
            return;
        }
        auto& station = scene_.stations.at(actor.station);
        const auto point = surface_feet(geometry_, station.location, TraversalLimits{actor.size});
        if (!point || !interaction_accessible(index, station.location, actor.feet)) {
            release_navigation(index);
            actor.goal.reset();
            actor.requested_goal.reset();
            actor.requested_reach = 0;
            actor.route = {};
            actor.leg = 0;
            actor.step_point = 0;
            actor.task = TraversalTask::None;
            actor.state = TraversalState::NoPath;
            actor.retry_at = now + 40;
            return;
        }
        // Availability and wear are inspected only on arrival, not read remotely by the chooser.
        observe_station(index, actor.station);
        if (station.kind == TraversalTask::Maintain && station.wear == 0) {
            actor.station_retry[actor.station] = now + 200;
            actor.goal.reset();
            actor.state = TraversalState::Idle;
            actor.task = TraversalTask::None;
            return;
        }
        static_cast<void>(start_station_action(index));
        return;
    }
    if (actor.goal &&
        (actor.state == TraversalState::Moving || actor.state == TraversalState::Waiting)) {
        return;
    }
    if (now < actor.retry_at) {
        return;
    }
    if (actor.energy > 300)
        std::erase_if(actor.suspended,
                      [](const auto& task) { return task.task == TraversalTask::Rest; });
    if (actor.hunger < 250 && actor.energy > 300 && plan_delivery(index))
        return;
    for (const auto wanted : {TraversalTask::Eat, TraversalTask::Rest, TraversalTask::Maintain}) {
        if ((wanted == TraversalTask::Eat && actor.hunger < 250) ||
            (wanted == TraversalTask::Rest && actor.energy > 300) ||
            (wanted == TraversalTask::Maintain && (actor.hunger >= 250 || actor.energy <= 300)))
            continue;
        std::optional<std::size_t> preferred;
        for (const auto& paused : actor.suspended)
            if (paused.task == wanted)
                preferred = paused.station;
        if (select_task(index, wanted, preferred))
            return;
    }
    actor.goal.reset();
    actor.route = {};
    actor.leg = 0;
    actor.step_point = 0;
    actor.task = TraversalTask::None;
    actor.state = TraversalState::NoPath;
    actor.retry_at = now + 40;
}
bool TraversalSession::start_station_action(std::size_t index) {
    auto& actor = actors_[index];
    auto& station = scene_.stations.at(actor.station);
    const auto now = world_.clock().tick_count();
    if (!practice_allows_work(index, actor.station)) {
        if (actor.autonomous)
            request_practice_work(index);
        return false;
    }
    check(interaction_accessible(index, station.location, actor.feet),
          "station action requires physical access");
    check(next_command_.value < std::numeric_limits<std::uint64_t>::max(),
          "command space exhausted");
    if (!legal_service_allowed(index, actor.station)) {
        actor.station_retry[actor.station] = now + 100;
        actor.goal.reset();
        actor.route = {};
        actor.leg = 0;
        actor.task = TraversalTask::None;
        actor.state = TraversalState::Waiting;
        actor.retry_at = now + 10;
        return false;
    }
    const auto duration = station.kind == TraversalTask::Eat    ? 40ULL
                          : station.kind == TraversalTask::Rest ? 60ULL
                                                                : 80ULL;
    const auto request =
        world_.reserve({next_command_, actor.id, station.lot, Quantity{1},
                        Quantity{station.kind == TraversalTask::Rest ? 0ULL : 1ULL}, duration});
    ++next_command_.value;
    if (request.status == RequestStatus::Accepted) {
        actor.action = request.action;
        check(world_.begin(actor.action) == Transition::Changed,
              "autonomous action failed to begin");
        if (station.kind == TraversalTask::Maintain)
            practice_work_started(index, actor.action);
        actor.goal.reset();
        actor.route = {};
        actor.leg = 0;
        actor.state = TraversalState::Idle;
    } else {
        actor.station_retry[actor.station] = now + 100;
        actor.goal.reset();
        actor.route = {};
        actor.leg = 0;
        actor.task = TraversalTask::None;
        actor.state = TraversalState::Waiting;
        actor.retry_at = now + 10;
    }
    observe_station(index, actor.station);
    return request.status == RequestStatus::Accepted;
}
} // namespace reality
