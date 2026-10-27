#include "lab/traversal_session.hpp"

#include <algorithm>
#include <stdexcept>

namespace reality {
namespace {
bool unavailable(const StationBelief& belief, TraversalTask kind) {
    return belief.available == 0 || (kind == TraversalTask::Maintain && belief.wear == 0);
}
bool protected_motion(const TraversalActor& actor) {
    return actor.transfer == TransferStage::Boarding || actor.transfer == TransferStage::Riding ||
           actor.transfer == TransferStage::Exiting ||
           (actor.leg < actor.route.legs.size() && actor.step_point > 0 &&
            actor.route.legs[actor.leg].kind == SurfaceLegKind::Step);
}
void check(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
} // namespace
bool TraversalSession::belief_unavailable(const TraversalActor& actor, std::size_t station) const {
    const auto& belief = actor.beliefs.at(station);
    const auto now = world_.clock().tick_count();
    return belief && unavailable(*belief, scene_.stations.at(station).kind) &&
           now - belief->observed_tick < 100;
}
void TraversalSession::observe_station(std::size_t index, std::size_t station_index) {
    auto& actor = actors_.at(index);
    const auto& station = scene_.stations.at(station_index);
    // Every caller still needs this gate, including completion and cargo refusals.
    if (!interaction_accessible(index, station.location, actor.feet))
        return;
    const auto lot = world_.resource(station.lot);
    check(lot && lot->record.active, "observed station has no active resource");
    const auto now = world_.clock().tick_count();
    actor.known_stations |= std::uint32_t{1} << station_index;
    actor.beliefs[station_index] =
        StationBelief{lot->available.value, station.wear, now, now, actor.id, actor.id};
    if (lot->available.value == 0) {
        actor.share_until = now + 40;
        actor.share_retry_at = now;
    }
}
KnowledgeOutcome TraversalSession::share_knowledge(std::size_t sender, std::size_t recipient) {
    const auto& from = actors_.at(sender);
    auto& to = actors_.at(recipient);
    if (sender == recipient || length(from.feet - to.feet) > 2)
        return KnowledgeOutcome::OutOfReach;
    if (!knowledge_contact(sender, recipient))
        return KnowledgeOutcome::Blocked;
    const auto now = world_.clock().tick_count();
    // Allocate before mutating any kind of knowledge.
    std::size_t new_reports = 0;
    const auto has_report = [&](const auto& b) {
        return std::any_of(to.testimonies.begin(), to.testimonies.end(), [&](const auto& c) {
            return c.incident == b.incident && c.resolved == b.resolved;
        });
    };
    for (const auto& b : from.testimonies)
        if (!has_report(b))
            ++new_reports;
    if (to.testimonies.size() + new_reports > 512)
        throw std::length_error("testimony limit");
    to.testimonies.reserve(to.testimonies.size() + new_reports);
    auto changed = false;
    for (const auto& b : from.testimonies) {
        if (has_report(b))
            continue;
        auto report = b;
        report.learned_tick = now;
        report.via_actor = from.id;
        to.testimonies.push_back(report);
        changed = true;
    }
    for (std::size_t station = 0; station < scene_.stations.size(); ++station) {
        const auto& incoming = from.beliefs[station];
        auto& current = to.beliefs[station];
        if (!incoming || (current && incoming->observed_tick <= current->observed_tick))
            continue;
        const auto was_unavailable =
            !current || unavailable(*current, scene_.stations[station].kind);
        current = *incoming;
        current->learned_tick = now;
        current->via_actor = from.id;
        to.known_stations |= std::uint32_t{1} << station;
        if (unavailable(*current, scene_.stations[station].kind)) {
            // A relay never makes an old report young again.
            if (now - current->observed_tick < 100)
                to.station_retry[station] =
                    std::max(to.station_retry[station], current->observed_tick + 100);
        } else if (was_unavailable) {
            to.station_retry[station] = now;
            if (to.autonomous) {
                to.retry_at = now;
                to.need_retry_at = now;
            }
        }
        changed = true;
    }
    for (std::size_t i = 0; i < scene_.passages.size(); ++i) {
        const auto& incoming = from.passage_beliefs[i];
        auto& current = to.passage_beliefs[i];
        if (!incoming || (current && incoming->observed_tick <= current->observed_tick))
            continue;
        current = *incoming;
        current->learned_tick = now;
        current->via_actor = from.id;
        to.retry_at = now;
        changed = true;
    }
    return changed ? KnowledgeOutcome::Shared : KnowledgeOutcome::NoNews;
}
void TraversalSession::update_knowledge() {
    const auto now = world_.clock().tick_count();
    for (std::size_t sender = 0; sender < actors_.size(); ++sender) {
        auto& actor = actors_[sender];
        if (!actor.autonomous || actor.share_until == 0 || now >= actor.share_until ||
            now < actor.share_retry_at)
            continue;
        actor.share_retry_at = now + 10;
        for (std::size_t recipient = 0; recipient < actors_.size(); ++recipient)
            if (sender != recipient)
                static_cast<void>(share_knowledge(sender, recipient));
    }
}
void TraversalSession::reconsider_belief(std::size_t index) {
    auto& actor = actors_[index];
    if (!actor.autonomous || actor.yielding || actor.action.value != 0 || protected_motion(actor) ||
        actor.task == TraversalTask::None || actor.task == TraversalTask::Deliver ||
        !belief_unavailable(actor, actor.station))
        return;
    // Only a received report changes an in-flight plan. Own reservation/consumption
    // can legitimately leave zero available while the current action completes.
    if (actor.beliefs[actor.station]->via_actor == actor.id)
        return;
    release_navigation(index);
    actor.goal.reset();
    actor.requested_goal.reset();
    actor.requested_reach = 0;
    actor.pending_goal.reset();
    actor.pending_reach = 0;
    actor.route = {};
    actor.leg = 0;
    actor.step_point = 0;
    actor.task = TraversalTask::None;
    actor.state = TraversalState::Idle;
    actor.retry_at = world_.clock().tick_count();
    actor.finish_grace_until = 0;
}
void TraversalSession::validate_knowledge() const {
    const auto now = world_.clock().tick_count();
    const auto actor_exists = [&](EntityId id) {
        return id.value != 0 && std::any_of(actors_.begin(), actors_.end(),
                                            [&](const auto& a) { return a.id == id; });
    };
    for (const auto& actor : actors_) {
        check(actor.share_until <= now + 40 && actor.share_retry_at <= now + 10 &&
                  (actor.share_until != 0 || actor.share_retry_at == 0),
              "invalid knowledge sharing horizon");
        for (std::size_t station = 0; station < actor.beliefs.size(); ++station) {
            const auto& belief = actor.beliefs[station];
            if (!belief)
                continue;
            check(station < scene_.stations.size() &&
                      (actor.known_stations & (std::uint32_t{1} << station)) != 0 &&
                      belief->wear <= 1000 && belief->observed_tick <= belief->learned_tick &&
                      belief->learned_tick <= now && actor_exists(belief->source_actor) &&
                      actor_exists(belief->via_actor),
                  "invalid station belief reference or observation");
            if (belief->via_actor == actor.id || belief->source_actor == actor.id)
                check(belief->source_actor == actor.id && belief->via_actor == actor.id &&
                          belief->observed_tick == belief->learned_tick,
                      "invalid eyewitness provenance");
        }
    }
}
} // namespace reality
