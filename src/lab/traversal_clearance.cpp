#include "lab/traversal_session.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace reality {
namespace {
bool in_step(const TraversalActor& actor) {
    return actor.transfer == TransferStage::None && actor.leg < actor.route.legs.size() &&
           actor.route.legs[actor.leg].kind == SurfaceLegKind::Step && actor.step_point > 0;
}
std::vector<SpatialPoint> remaining_step(const SpatialGeometry& geometry,
                                         const TraversalActor& actor) {
    if (!in_step(actor))
        return {};
    const auto& leg = actor.route.legs[actor.leg];
    const auto from = surface_feet(geometry, leg.from, TraversalLimits{actor.size});
    const auto to = surface_feet(geometry, leg.to, TraversalLimits{actor.size});
    if (!from || !to)
        return {};
    const auto points = step_points(*from, *to);
    std::vector<SpatialPoint> result{actor.feet};
    result.insert(result.end(), points.begin() + static_cast<std::ptrdiff_t>(actor.step_point),
                  points.end());
    return result;
}
bool obstructs(std::span<const SpatialPoint> path, CapsuleSize traveller, SpatialPoint feet,
               CapsuleSize size) {
    for (std::size_t i = 1; i < path.size(); ++i)
        if (!capsule_paths_clear(path[i - 1], path[i], traveller, feet, feet, size))
            return true;
    return false;
}
void check(bool value, const char* message) {
    if (!value)
        throw std::invalid_argument(message);
}
} // namespace
const char* clearance_label(ClearanceOutcome outcome) {
    switch (outcome) {
    case ClearanceOutcome::None:
        return "none";
    case ClearanceOutcome::Accepted:
        return "accepted";
    case ClearanceOutcome::Controlled:
        return "controlled";
    case ClearanceOutcome::Busy:
        return "busy";
    case ClearanceOutcome::NoSpace:
        return "no_space";
    case ClearanceOutcome::Completed:
        return "completed";
    case ClearanceOutcome::Expired:
        return "expired";
    case ClearanceOutcome::Cancelled:
        return "cancelled";
    }
    throw std::invalid_argument("invalid clearance outcome");
}
ClearanceOutcome TraversalSession::request_clearance(std::size_t index) {
    auto& requester = actors_.at(index);
    const auto now = world_.clock().tick_count();
    if (!in_step(requester))
        return ClearanceOutcome::None;
    if (now < requester.clearance.retry_at)
        return requester.clearance.outcome;
    if (requester.clearance.outcome == ClearanceOutcome::Accepted) {
        requester.clearance.retry_at = now + 40;
        return ClearanceOutcome::Accepted;
    }
    const auto path = remaining_step(geometry_, requester);
    if (path.empty())
        return ClearanceOutcome::None;
    std::optional<std::size_t> recipient;
    double closest = 3.;
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        const auto& other = actors_[i];
        const auto distance = length(other.feet - requester.feet);
        if (i != index && distance < closest &&
            obstructs(path, requester.size, other.feet, other.size)) {
            recipient = i;
            closest = distance;
        }
    }
    if (!recipient)
        return ClearanceOutcome::None;
    auto& other = actors_[*recipient];
    const auto reply = [&](ClearanceOutcome outcome) {
        requester.clearance = {other.id, outcome, now, now + 40};
        return outcome;
    };
    if (!other.autonomous)
        return reply(ClearanceOutcome::Controlled);
    if (other.yielding || other.action.value != 0 || practice_busy(*recipient) || in_step(other) ||
        other.transfer != TransferStage::None || other.motion.speed > .05 || other.pending_goal ||
        other.stop_after_transfer ||
        std::any_of(scene_.platforms.begin(), scene_.platforms.end(),
                    [&](const auto& p) { return p.definition.body == other.support.surface; }) ||
        std::any_of(scene_.passages.begin(), scene_.passages.end(),
                    [&](const auto& p) { return p.owner == other.id; }))
        return reply(ClearanceOutcome::Busy);
    std::vector<SurfaceBlocker> blockers;
    for (const auto& person : actors_)
        if (person.id != other.id && length(person.feet - other.feet) <= 3)
            blockers.push_back({person.feet, person.size});
    const auto& body = geometry_.body(other.support.surface);
    const auto local = body.local(other.feet);
    const SurfaceLocation start{body.id, {local.x, local.z}};
    unsigned searches = 0;
    for (const auto radius : {.8, 1.2, 1.6}) {
        for (unsigned direction = 0; direction < 8; ++direction) {
            const auto angle = 2 * std::numbers::pi * direction / 8;
            const SurfaceLocation candidate{
                body.id, {local.x + radius * std::cos(angle), local.z + radius * std::sin(angle)}};
            const auto feet = surface_feet(geometry_, candidate, TraversalLimits{other.size});
            if (!feet || obstructs(path, requester.size, *feet, other.size))
                continue;
            bool free = true;
            for (const auto& person : actors_) {
                if (person.id == other.id)
                    continue;
                if (length(person.feet - other.feet) <= 3 &&
                    !capsule_paths_clear(*feet, *feet, other.size, person.feet, person.feet,
                                         person.size))
                    free = false;
                if (person.yielding && person.goal) {
                    const auto reserved =
                        surface_feet(geometry_, *person.goal, TraversalLimits{person.size});
                    if (reserved && !capsule_paths_clear(*feet, *feet, other.size, *reserved,
                                                         *reserved, person.size))
                        free = false;
                }
            }
            if (!free)
                continue;
            if (searches == 4)
                return reply(ClearanceOutcome::NoSpace);
            ++searches;
            auto route = find_surface_route(geometry_, {}, start, candidate,
                                            TraversalLimits{other.size}, 512, 32768, blockers);
            ++other.plans;
            if (route.status != RouteStatus::Found || route.cost_m > 4 ||
                std::any_of(route.legs.begin(), route.legs.end(), [&](const auto& leg) {
                    return leg.kind != SurfaceLegKind::Walk || leg.from.surface != body.id ||
                           leg.to.surface != body.id;
                }))
                continue;
            const auto& leg = requester.route.legs[requester.leg];
            other.yielding =
                ClearanceMove{requester.id, now, now + 200, leg.from, leg.to, other.goal};
            release_navigation(*recipient);
            other.goal = candidate;
            other.route = std::move(route);
            other.leg = 0;
            other.step_point = 0;
            other.blocked_ticks = 0;
            other.state =
                other.route.legs.empty() ? TraversalState::Arrived : TraversalState::Moving;
            return reply(ClearanceOutcome::Accepted);
        }
    }
    return reply(ClearanceOutcome::NoSpace);
}
void TraversalSession::cancel_yield(std::size_t index) {
    finish_yield(index, ClearanceOutcome::Cancelled);
}
void TraversalSession::finish_yield(std::size_t index, ClearanceOutcome outcome) {
    auto& other = actors_.at(index);
    if (!other.yielding)
        return;
    const auto move = *other.yielding;
    const auto sender = std::find_if(actors_.begin(), actors_.end(),
                                     [&](const auto& a) { return a.id == move.requester; });
    if (sender != actors_.end() && sender->clearance.peer == other.id &&
        sender->clearance.at == move.requested_at)
        sender->clearance.outcome = outcome;
    other.yielding.reset();
    other.goal = move.original_goal;
    other.route = {};
    other.leg = 0;
    other.step_point = 0;
    other.state = other.goal ? TraversalState::NoPath : TraversalState::Idle;
    other.retry_at = world_.clock().tick_count();
    other.blocked_ticks = 0;
}
void TraversalSession::update_clearance() {
    const auto now = world_.clock().tick_count();
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        auto& other = actors_[i];
        if (!other.yielding)
            continue;
        const auto move = *other.yielding;
        const auto sender = std::find_if(actors_.begin(), actors_.end(),
                                         [&](const auto& a) { return a.id == move.requester; });
        check(sender != actors_.end(), "missing clearance requester");
        const auto active = in_step(*sender) && sender->route.legs[sender->leg].from == move.from &&
                            sender->route.legs[sender->leg].to == move.to;
        if (active && now < move.expires_at)
            continue;
        finish_yield(i, active ? ClearanceOutcome::Expired : ClearanceOutcome::Completed);
    }
    for (std::size_t i = 0; i < actors_.size(); ++i)
        if (in_step(actors_[i]) && actors_[i].blocked_ticks >= 100 &&
            now >= actors_[i].clearance.retry_at)
            static_cast<void>(request_clearance(i));
}
void TraversalSession::validate_clearance() const {
    const auto now = world_.clock().tick_count();
    const auto exists = [&](EntityId id) {
        return std::any_of(actors_.begin(), actors_.end(),
                           [&](const auto& a) { return a.id == id; });
    };
    for (const auto& actor : actors_) {
        const auto& reply = actor.clearance;
        check(static_cast<unsigned>(reply.outcome) <=
                  static_cast<unsigned>(ClearanceOutcome::Cancelled),
              "invalid clearance response");
        if (reply.outcome == ClearanceOutcome::None)
            check(reply.peer.value == 0 && reply.at == 0 && reply.retry_at == 0,
                  "invalid empty clearance");
        else
            check(reply.peer != actor.id && exists(reply.peer) && reply.at <= now &&
                      reply.retry_at >= reply.at && reply.retry_at <= now + 40,
                  "invalid clearance reference or clock");
        if (reply.outcome == ClearanceOutcome::Accepted)
            check(std::any_of(actors_.begin(), actors_.end(),
                              [&](const auto& recipient) {
                                  return recipient.id == reply.peer && recipient.yielding &&
                                         recipient.yielding->requester == actor.id &&
                                         recipient.yielding->requested_at == reply.at;
                              }),
                  "accepted clearance has no matching maneuver");
        if (!actor.yielding)
            continue;
        const auto& move = *actor.yielding;
        check(actor.autonomous && actor.action.value == 0 &&
                  actor.transfer == TransferStage::None && !in_step(actor) && actor.goal &&
                  move.requester != actor.id && exists(move.requester) &&
                  move.requested_at <= now && move.expires_at > move.requested_at &&
                  move.expires_at - move.requested_at == 200 && move.expires_at >= now &&
                  move.expires_at <= now + 200 && actor.route.status == RouteStatus::Found,
              "invalid clearance maneuver");
        const auto sender = std::find_if(actors_.begin(), actors_.end(),
                                         [&](const auto& a) { return a.id == move.requester; });
        check(sender != actors_.end() && sender->clearance.peer == actor.id &&
                  sender->clearance.outcome == ClearanceOutcome::Accepted &&
                  sender->clearance.at == move.requested_at,
              "clearance maneuver does not match its agreement");
        const auto same_step = in_step(*sender) &&
                               sender->route.legs[sender->leg].from == move.from &&
                               sender->route.legs[sender->leg].to == move.to;
        check(same_step, "clearance maneuver does not match its committed step");
        static_cast<void>(geometry_.body(move.from.surface));
        static_cast<void>(geometry_.body(move.to.surface));
        check(std::any_of(scene_.connections.begin(), scene_.connections.end(),
                          [&](const auto& c) {
                              return c.kind == SurfaceLegKind::Step &&
                                     ((c.from == move.from && c.to == move.to) ||
                                      (c.bidirectional && c.from == move.to && c.to == move.from));
                          }),
              "fabricated clearance step");
        if (move.original_goal) {
            static_cast<void>(geometry_.body(move.original_goal->surface));
            reality::validate(move.original_goal->local);
        }
        for (const auto& leg : actor.route.legs)
            check(leg.kind == SurfaceLegKind::Walk && leg.from.surface == actor.support.surface &&
                      leg.to.surface == actor.support.surface,
                  "clearance maneuver leaves its support");
        const auto point = surface_feet(geometry_, *actor.goal, TraversalLimits{actor.size});
        check(point.has_value(), "unsupported clearance destination");
        for (const auto& recipient : actors_) {
            if (recipient.id.value <= actor.id.value || !recipient.yielding || !recipient.goal)
                continue;
            const auto reserved =
                surface_feet(geometry_, *recipient.goal, TraversalLimits{recipient.size});
            check(reserved && capsule_paths_clear(*point, *point, actor.size, *reserved, *reserved,
                                                  recipient.size),
                  "clearance destinations overlap");
        }
    }
}
} // namespace reality
