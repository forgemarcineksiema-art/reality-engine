#include "lab/traversal_session.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
namespace reality {
namespace {
void check(bool c, const char* m) {
    if (!c)
        throw std::invalid_argument(m);
}
SpatialGeometry barrier(const SpatialGeometry& geometry, const PassageState& p, CapsuleSize size) {
    auto body = geometry.body(p.surface);
    const auto height = size.height + 1;
    body.origin.y += height;
    body.depth += height;
    body.walkable = false;
    SpatialGeometry result;
    result.bodies.push_back(body);
    return result;
}
} // namespace
const std::vector<PassageState>& TraversalSession::passages() const noexcept {
    return scene_.passages;
}
bool TraversalSession::protected_operation(const TraversalActor& a) const {
    if (practice_)
        for (std::size_t i = 0; i < actors_.size(); ++i)
            if (actors_[i].id == a.id && practice_busy(i))
                return true;
    return a.action.value != 0 || a.yielding || a.transfer == TransferStage::Boarding ||
           a.transfer == TransferStage::Riding || a.transfer == TransferStage::Exiting ||
           (a.leg < a.route.legs.size() && a.route.legs[a.leg].kind == SurfaceLegKind::Step &&
            a.step_point > 0);
}
std::uint32_t TraversalSession::closed_passage_mask(const TraversalActor& actor) const {
    std::uint32_t mask = 0;
    const auto now = world_.clock().tick_count();
    for (std::size_t i = 0; i < scene_.passages.size(); ++i) {
        const auto& b = actor.passage_beliefs[i];
        if (b && !b->open && now - b->observed_tick < 100)
            mask |= std::uint32_t{1} << i;
    }
    return mask;
}
std::vector<SurfaceConnection>
TraversalSession::known_connections(const TraversalActor& actor) const {
    const auto mask = closed_passage_mask(actor);
    std::vector<SurfaceConnection> result;
    for (const auto& c : scene_.connections) {
        bool closed = false;
        for (std::size_t i = 0; i < scene_.passages.size(); ++i)
            if ((mask & (std::uint32_t{1} << i)) != 0 &&
                (c.passage == scene_.passages[i].id ||
                 c.from.surface == scene_.passages[i].surface ||
                 c.to.surface == scene_.passages[i].surface))
                closed = true;
        if (!closed)
            result.push_back(c);
    }
    return result;
}
bool TraversalSession::passage_motion_clear(SpatialPoint from, SpatialPoint to,
                                            CapsuleSize size) const {
    for (const auto& p : scene_.passages)
        if (!p.open && !spatial_segment_clear(barrier(geometry_, p, size), from, to, size))
            return false;
    return true;
}
bool TraversalSession::passage_busy(const PassageState& p) const {
    for (const auto& a : actors_) {
        const auto blocked = barrier(geometry_, p, a.size);
        if (a.support.surface == p.surface || !spatial_occupiable(blocked, a.feet, a.size))
            return true;
        if (a.leg < a.route.legs.size()) {
            const auto& l = a.route.legs[a.leg];
            if (l.kind == SurfaceLegKind::Step && a.step_point > 0 &&
                (l.passage == p.id || l.from.surface == p.surface || l.to.surface == p.surface))
                return true;
        }
        if (a.yielding) {
            const auto& m = *a.yielding;
            if (m.from.surface == p.surface || m.to.surface == p.surface ||
                !spatial_segment_clear(
                    blocked, a.feet,
                    geometry_.body(m.to.surface).world({m.to.local.x_m, 0, m.to.local.z_m}),
                    a.size))
                return true;
            const auto requester = std::find_if(actors_.begin(), actors_.end(),
                                                [&](const auto& r) { return r.id == m.requester; });
            if (requester != actors_.end() && requester->leg < requester->route.legs.size() &&
                requester->route.legs[requester->leg].passage == p.id)
                return true;
        }
    }
    return false;
}
bool TraversalSession::passage_contact(std::size_t index, const PassageState& p) const {
    const auto& a = actors_.at(index);
    for (const auto& c : scene_.connections) {
        // Geometry owns the bank association; a historical Step may have no
        // FIFO label. It must still expose its physical observation channel.
        if (c.passage != p.id && c.from.surface != p.surface && c.to.surface != p.surface)
            continue;
        for (const auto bank : {c.from, c.to}) {
            if (bank.surface != a.support.surface)
                continue;
            const auto feet = surface_feet(geometry_, bank, TraversalLimits{a.size});
            if (feet && length(*feet - a.feet) <= 1.5 &&
                spatial_segment_clear(geometry_, a.feet, *feet, a.size))
                return true;
        }
    }
    return false;
}
bool TraversalSession::observe_passage(std::size_t index, std::uint32_t id) {
    auto& actor = actors_.at(index);
    const auto it = std::find_if(scene_.passages.begin(), scene_.passages.end(),
                                 [&](const auto& p) { return p.id == id; });
    if (it == scene_.passages.end() || !passage_contact(index, *it))
        return false;
    const auto i = static_cast<std::size_t>(it - scene_.passages.begin());
    const auto now = world_.clock().tick_count();
    auto& b = actor.passage_beliefs[i];
    const auto changed = !b || b->open != it->open;
    b = PassageBelief{it->open, now, now, actor.id, actor.id};
    if (changed) {
        actor.share_until = now + 40;
        actor.share_retry_at = now;
    }
    return true;
}
void TraversalSession::change_passage(std::size_t index, bool open) {
    auto& p = scene_.passages.at(index);
    if (p.open == open)
        return;
    check(scene_.geometry.version < std::numeric_limits<std::uint64_t>::max(),
          "passage version exhausted");
    check(open || !passage_busy(p), "unsafe internal passage closure");
    p.open = open;
    p.queue.clear();
    p.owner = {};
    ++scene_.geometry.version;
    geometry_.version = scene_.geometry.version;
}
PassageOutcome TraversalSession::interact_passage(std::size_t index, std::uint32_t id, bool open) {
    const auto& a = actors_.at(index);
    const auto it = std::find_if(scene_.passages.begin(), scene_.passages.end(),
                                 [&](const auto& p) { return p.id == id; });
    if (it == scene_.passages.end())
        return PassageOutcome::Unavailable;
    if (a.autonomous)
        return PassageOutcome::Controlled;
    if (protected_operation(a) || a.motion.speed > .03)
        return PassageOutcome::Busy;
    if (!passage_contact(index, *it))
        return PassageOutcome::OutOfReach;
    if (!open && passage_busy(*it))
        return PassageOutcome::Occupied;
    if (open && it->maintenance && scene_.stations[*it->maintenance].wear == 1000)
        return PassageOutcome::Worn;
    const auto changed = it->open != open;
    change_passage(static_cast<std::size_t>(it - scene_.passages.begin()), open);
    static_cast<void>(observe_passage(index, id));
    return changed ? PassageOutcome::Changed : PassageOutcome::Unchanged;
}
void TraversalSession::update_passages() {
    for (std::size_t p = 0; p < scene_.passages.size(); ++p) {
        const auto& passage = scene_.passages[p];
        if (passage.open && passage.maintenance &&
            scene_.stations[*passage.maintenance].wear == 1000 && !passage_busy(passage))
            change_passage(p, false);
    }
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        for (std::size_t p = 0; p < scene_.passages.size(); ++p)
            static_cast<void>(observe_passage(i, scene_.passages[p].id));
        auto& a = actors_[i];
        const auto mask = closed_passage_mask(a);
        bool affected = false, reopened = false;
        for (std::size_t p = 0; p < scene_.passages.size(); ++p) {
            const auto bit = std::uint32_t{1} << p;
            if ((mask & bit) != 0 && (a.planned_closed_passages & bit) == 0)
                for (std::size_t leg = a.leg; leg < a.route.legs.size(); ++leg) {
                    const auto& l = a.route.legs[leg];
                    affected = affected || l.passage == scene_.passages[p].id ||
                               l.from.surface == scene_.passages[p].surface ||
                               l.to.surface == scene_.passages[p].surface;
                }
            reopened = reopened || ((a.planned_closed_passages & bit) != 0 && (mask & bit) == 0 &&
                                    a.passage_beliefs[p] && a.passage_beliefs[p]->open);
        }
        // Expiry permits a new scout, but must not cancel a viable long detour
        // every 100 ticks. A fresh opening report can shorten an active route.
        if (a.goal && !protected_operation(a) && a.planned_closed_passages != mask &&
            (affected || reopened || a.route.status != RouteStatus::Found)) {
            release_navigation(i);
            plan(i, true);
        }
    }
}
void TraversalSession::repair_passages(std::size_t actor, std::size_t station) {
    for (std::size_t i = 0; i < scene_.passages.size(); ++i) {
        const auto& p = scene_.passages[i];
        if (p.maintenance && *p.maintenance == station && scene_.stations[station].wear < 1000) {
            change_passage(i, true);
            static_cast<void>(observe_passage(actor, p.id));
        }
    }
}
bool TraversalSession::begin_maintenance(std::size_t index, std::size_t station) {
    auto& a = actors_.at(index);
    if (station >= scene_.stations.size() ||
        scene_.stations[station].kind != TraversalTask::Maintain || a.autonomous ||
        protected_operation(a) || a.transfer != TransferStage::None || a.motion.speed > .03 ||
        !interaction_accessible(index, scene_.stations[station].location, a.feet))
        return false;
    auto candidate = *this;
    candidate.stop(index);
    candidate.actors_[index].task = TraversalTask::Maintain;
    candidate.actors_[index].station = static_cast<std::uint32_t>(station);
    const auto started = candidate.start_station_action(index);
    *this = std::move(candidate);
    return started;
}
void TraversalSession::validate_passages() const {
    check(scene_.passages.size() <= 16, "too many passage states");
    std::set<std::uint32_t> ids, surfaces;
    for (const auto& p : scene_.passages) {
        check(p.id != 0 && ids.insert(p.id).second && surfaces.insert(p.surface).second,
              "duplicate passage identity or surface");
        const auto& body = scene_.geometry.body(p.surface);
        check(body.walkable &&
                  std::none_of(scene_.platforms.begin(), scene_.platforms.end(),
                               [&](const auto& m) { return m.definition.body == p.surface; }),
              "passage requires static walkable support");
        check(std::any_of(scene_.connections.begin(), scene_.connections.end(),
                          [&](const auto& c) {
                              return c.passage == p.id &&
                                     (c.from.surface == p.surface || c.to.surface == p.surface);
                          }),
              "passage has no physical connection");
        if (p.maintenance) {
            check(*p.maintenance < scene_.stations.size(), "invalid passage maintenance index");
            const auto& s = scene_.stations[*p.maintenance];
            const auto lot = world_.resource(s.lot);
            check(s.kind == TraversalTask::Maintain && lot &&
                      lot->record.kind == ResourceKind::Material &&
                      lot->record.material == MaterialTypeId{2},
                  "passage requires maintenance material2");
            bool local = false;
            const auto station = geometry_.body(s.location.surface)
                                     .world({s.location.local.x_m, 0, s.location.local.z_m});
            for (const auto& c : scene_.connections)
                if (c.passage == p.id || c.from.surface == p.surface || c.to.surface == p.surface)
                    for (const auto bank : {c.from, c.to})
                        if (bank.surface == s.location.surface && bank.surface != p.surface) {
                            const auto point = geometry_.body(bank.surface)
                                                   .world({bank.local.x_m, 0, bank.local.z_m});
                            local = local || length(point - station) <= 1.5;
                        }
            check(local, "maintenance station is not at a passage bank");
        }
        check(p.open || (p.owner.value == 0 && p.queue.empty() && !passage_busy(p)),
              "closed passage is occupied or allocated");
    }
    for (const auto& c : scene_.connections)
        if (c.passage != 0) {
            const auto p = std::find_if(scene_.passages.begin(), scene_.passages.end(),
                                        [&](const auto& value) { return value.id == c.passage; });
            check(p != scene_.passages.end() && c.kind == SurfaceLegKind::Step &&
                      (c.from.surface == p->surface || c.to.surface == p->surface),
                  "connection references invalid passage");
        }
    const auto allowed =
        scene_.passages.empty() ? 0 : ((std::uint32_t{1} << scene_.passages.size()) - 1);
    const auto now = world_.clock().tick_count();
    const auto valid_actor = [&](EntityId id) {
        return id.value != 0 && std::any_of(actors_.begin(), actors_.end(),
                                            [&](const auto& a) { return a.id == id; });
    };
    for (const auto& a : actors_) {
        check((a.planned_closed_passages & ~allowed) == 0, "invalid planned passage mask");
        for (const auto& leg : a.route.legs) {
            if (leg.kind == SurfaceLegKind::Walk)
                continue;
            for (std::size_t p = 0; p < scene_.passages.size(); ++p)
                if ((a.planned_closed_passages & (std::uint32_t{1} << p)) != 0)
                    check(leg.passage != scene_.passages[p].id &&
                              leg.from.surface != scene_.passages[p].surface &&
                              leg.to.surface != scene_.passages[p].surface,
                          "route contradicts its passage planning mask");
        }
        for (std::size_t i = 0; i < a.passage_beliefs.size(); ++i) {
            const auto& b = a.passage_beliefs[i];
            if (!b)
                continue;
            check(i < scene_.passages.size() && b->observed_tick <= b->learned_tick &&
                      b->learned_tick <= now && valid_actor(b->source_actor) &&
                      valid_actor(b->via_actor),
                  "invalid passage belief");
            if (b->source_actor == a.id || b->via_actor == a.id)
                check(b->source_actor == a.id && b->via_actor == a.id &&
                          b->observed_tick == b->learned_tick,
                      "invalid passage witness");
        }
    }
}
} // namespace reality
