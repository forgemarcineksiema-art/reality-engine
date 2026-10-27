#include "lab/traversal_session.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
namespace reality {
namespace {
constexpr std::uint64_t cargo_limit = 4;
bool active(const DeliveryState& job) { return job.phase < DeliveryPhase::Completed; }
bool committed(const TraversalActor& a) {
    return a.transfer != TransferStage::None ||
           (a.leg < a.route.legs.size() && a.route.legs[a.leg].kind == SurfaceLegKind::Step &&
            a.step_point > 0);
}
void check(bool c, const char* m) {
    if (!c)
        throw std::invalid_argument(m);
}
} // namespace
void TraversalSession::assign_delivery(std::size_t index, std::uint32_t source,
                                       std::uint32_t destination, std::uint64_t quantity) {
    auto& actor = actors_.at(index);
    check(source < scene_.stations.size() && destination < scene_.stations.size() &&
              source != destination && quantity > 0 && quantity <= 1024,
          "invalid delivery order");
    check(!actor.yielding && !committed(actor) && actor.action.value == 0 &&
              (!actor.delivery || !active(*actor.delivery)),
          "delivery actor is busy");
    const auto from = world_.resource(scene_.stations[source].lot),
               to = world_.resource(scene_.stations[destination].lot);
    check(from && to && scene_.stations[source].lot != scene_.stations[destination].lot &&
              from->record.kind == ResourceKind::Material &&
              to->record.kind == ResourceKind::Material && from->record.material.value != 0 &&
              from->record.material == to->record.material,
          "incompatible delivery material");
    if (actor.cargo.value != 0) {
        const auto bag = world_.resource(actor.cargo);
        check(bag && bag->record.quantity.value == 0 &&
                  bag->record.material == from->record.material,
              "delivery requires an empty compatible cargo lot");
    } else
        actor.cargo =
            world_.create_resource(actor.id, {0}, ResourceKind::Material, from->record.material);
    actor.delivery =
        DeliveryState{source, destination, quantity, 0, DeliveryPhase::ToPickup, next_command_, {}};
}
void TraversalSession::cancel_delivery(std::size_t index) {
    auto& actor = actors_.at(index);
    if (!actor.delivery || !active(*actor.delivery))
        return;
    actor.delivery->phase = DeliveryPhase::Cancelled;
    actor.delivery->ended_command = next_command_;
    if (actor.task == TraversalTask::Deliver) {
        const auto autonomous = actor.autonomous;
        stop(index);
        actor.autonomous = autonomous;
        actor.task = TraversalTask::None;
    }
}
bool TraversalSession::cargo_returning(const TraversalActor& actor) const noexcept {
    return actor.task == TraversalTask::Deliver &&
           (!actor.delivery || actor.delivery->phase >= DeliveryPhase::Completed);
}
std::optional<std::size_t>
TraversalSession::choose_return_station(const TraversalActor& actor) const {
    if (actor.cargo.value == 0)
        return std::nullopt;
    const auto bag = world_.resource(actor.cargo);
    if (!bag || !bag->record.active || bag->record.kind != ResourceKind::Material ||
        bag->record.material.value == 0 || bag->record.quantity.value == 0)
        return std::nullopt;
    const auto now = world_.clock().tick_count();
    const auto compatible = [&](std::size_t station) {
        if (station >= scene_.stations.size() ||
            (actor.known_stations & (std::uint32_t{1} << station)) == 0 ||
            now < actor.station_retry[station])
            return false;
        const auto lot = world_.resource(scene_.stations[station].lot);
        return lot && lot->record.active && lot->record.kind == ResourceKind::Material &&
               lot->record.material.value != 0 && lot->record.material == bag->record.material;
    };
    if (actor.delivery && actor.delivery->phase == DeliveryPhase::Cancelled &&
        compatible(actor.delivery->source)) {
        return actor.delivery->source;
    }
    std::optional<std::size_t> nearest;
    auto best = std::numeric_limits<double>::infinity();
    for (std::size_t station = 0; station < scene_.stations.size(); ++station) {
        if (!compatible(station))
            continue;
        const auto& location = scene_.stations[station].location;
        const auto point =
            geometry_.body(location.surface).world({location.local.x_m, 0, location.local.z_m});
        const auto distance = length(point - actor.feet);
        if (!nearest || distance < best) {
            nearest = station;
            best = distance;
        }
    }
    return nearest;
}
CargoStatus TraversalSession::interact_cargo(std::size_t index, std::size_t station_index) {
    auto& actor = actors_.at(index);
    if (station_index >= scene_.stations.size())
        return CargoStatus::Unavailable;
    if (actor.yielding || committed(actor) || actor.action.value != 0 || actor.motion.speed > .03)
        return CargoStatus::Busy;
    const auto& station = scene_.stations[station_index];
    const auto point = surface_feet(geometry_, station.location, TraversalLimits{actor.size});
    if (!point || length(*point - actor.feet) > 1.1)
        return CargoStatus::OutOfReach;
    if (!interaction_accessible(index, station.location, actor.feet))
        return CargoStatus::Blocked;
    observe_station(index, station_index);
    const auto lot = world_.resource(station.lot);
    if (!lot || !lot->record.active)
        return CargoStatus::Unavailable;
    if (lot->record.kind != ResourceKind::Material || lot->record.material.value == 0)
        return CargoStatus::Incompatible;
    auto bag =
        actor.cargo.value != 0 ? world_.resource(actor.cargo) : std::optional<ResourceView>{};
    if (actor.cargo.value != 0 && !bag)
        return CargoStatus::Unavailable;
    if (bag && bag->record.material != lot->record.material)
        return CargoStatus::Incompatible;
    const auto loading = !bag || bag->record.quantity.value == 0;
    if (actor.delivery && active(*actor.delivery)) {
        auto& job = *actor.delivery;
        if ((loading && station_index != job.source) ||
            (!loading && station_index != job.source && station_index != job.destination))
            return CargoStatus::Denied;
        if (loading && job.phase == DeliveryPhase::Returning && station_index == job.source) {
            job.phase =
                job.delivered == job.quantity ? DeliveryPhase::Completed : DeliveryPhase::ToPickup;
            if (job.phase == DeliveryPhase::Completed)
                job.ended_command = next_command_;
            return CargoStatus::Returned;
        }
    }
    auto quantity =
        loading ? std::min(cargo_limit, lot->available.value) : bag->record.quantity.value;
    if (loading && actor.delivery && active(*actor.delivery))
        quantity = std::min(quantity, actor.delivery->quantity - actor.delivery->delivered);
    if (quantity == 0)
        return CargoStatus::Empty;
    check(next_command_.value < std::numeric_limits<std::uint64_t>::max(),
          "command space exhausted");
    if (!bag)
        actor.cargo =
            world_.create_resource(actor.id, {0}, ResourceKind::Material, lot->record.material);
    const auto result = world_.transfer({next_command_,
                                         actor.id,
                                         loading ? station.lot : actor.cargo,
                                         loading ? actor.cargo : station.lot,
                                         {quantity}});
    ++next_command_.value;
    if (result.status != TransferStatus::Accepted)
        return CargoStatus::Denied;
    observe_station(index, station_index);
    if (!loading) {
        actor.share_until = world_.clock().tick_count() + 40;
        actor.share_retry_at = world_.clock().tick_count();
    }
    if (actor.delivery && active(*actor.delivery)) {
        auto& job = *actor.delivery;
        if (loading)
            job.phase = DeliveryPhase::ToDestination;
        else if (station_index == job.destination) {
            check(quantity <= job.quantity - job.delivered, "delivery exceeded requested quantity");
            job.delivered += quantity;
            job.phase = DeliveryPhase::Returning;
        } else
            job.phase = DeliveryPhase::ToPickup;
    }
    if (!loading && cargo_returning(actor)) {
        // A direct interaction while a return is pending must not leave a return
        // task with an empty bag.
        const auto remaining = world_.resource(actor.cargo);
        if (remaining && remaining->record.quantity.value == 0) {
            release_navigation(index);
            actor.goal.reset();
            actor.requested_goal.reset();
            actor.requested_reach = 0;
            actor.route = {};
            actor.leg = 0;
            actor.step_point = 0;
            actor.task = TraversalTask::None;
            actor.state = TraversalState::Idle;
            actor.retry_at = world_.clock().tick_count() + 1;
        }
    }
    return CargoStatus::Transferred;
}
bool TraversalSession::plan_delivery(std::size_t index) {
    auto& actor = actors_[index];
    const auto now = world_.clock().tick_count();
    if (actor.delivery && active(*actor.delivery)) {
        auto& job = *actor.delivery;
        const auto station =
            job.phase == DeliveryPhase::ToDestination ? job.destination : job.source;
        if (choose_approach(index, scene_.stations[station].location, .9, true)) {
            actor.task = TraversalTask::Deliver;
            actor.finish_grace_until = 0;
        } else {
            actor.goal.reset();
            actor.route = {};
            actor.leg = 0;
            actor.step_point = 0;
            actor.task = TraversalTask::None;
            actor.state = TraversalState::NoPath;
            actor.retry_at = now + 40;
        }
        return true;
    }
    // Without an active order a carried batch is still real property: return it
    // to a known compatible station instead of blocking the bag forever.
    const auto station = choose_return_station(actor);
    if (!station)
        return false;
    if (choose_approach(index, scene_.stations[*station].location, .9, true)) {
        actor.task = TraversalTask::Deliver;
        actor.station = static_cast<std::uint32_t>(*station);
        actor.finish_grace_until = 0;
        return true;
    }
    actor.station_retry[*station] = now + 100;
    actor.goal.reset();
    actor.route = {};
    actor.leg = 0;
    actor.step_point = 0;
    actor.task = TraversalTask::None;
    actor.state = TraversalState::NoPath;
    actor.retry_at = now + 40;
    return true;
}
void TraversalSession::arrive_delivery(std::size_t index) {
    auto& actor = actors_[index];
    const auto returning = cargo_returning(actor);
    check((actor.delivery && active(*actor.delivery)) || returning, "missing delivery at arrival");
    if (actor.motion.speed > .01)
        return;
    std::size_t station = actor.station;
    if (actor.delivery && active(*actor.delivery)) {
        const auto& job = *actor.delivery;
        station = job.phase == DeliveryPhase::ToDestination ? job.destination : job.source;
    }
    check(station < scene_.stations.size(), "cargo return arrived without a station");
    const auto point =
        surface_feet(geometry_, scene_.stations[station].location, TraversalLimits{actor.size});
    auto success = false;
    if (point && length(*point - actor.feet) <= 1.1) {
        const auto result = interact_cargo(index, station);
        success = result == CargoStatus::Transferred || result == CargoStatus::Returned;
    }
    release_navigation(index);
    actor.goal.reset();
    actor.requested_goal.reset();
    actor.requested_reach = 0;
    actor.route = {};
    actor.leg = 0;
    actor.step_point = 0;
    actor.task = TraversalTask::None;
    actor.state = success ? TraversalState::Idle : TraversalState::NoPath;
    actor.retry_at = world_.clock().tick_count() + (success ? 1 : 40);
    if (!success && returning)
        actor.station_retry[station] = world_.clock().tick_count() + 100;
}
void TraversalSession::validate_delivery() const {
    std::set<EntityId> bags;
    for (const auto& actor : actors_) {
        std::optional<ResourceView> bag;
        if (actor.cargo.value != 0) {
            bag = world_.resource(actor.cargo);
            check(bag && bag->record.active && bag->record.owner == actor.id &&
                      bag->record.kind == ResourceKind::Material &&
                      bag->record.material.value != 0 && bag->record.initial.value == 0 &&
                      bag->record.quantity.value <= cargo_limit && bags.insert(actor.cargo).second,
                  "invalid physical cargo lot");
            for (const auto& station : scene_.stations)
                check(station.lot != actor.cargo, "station reused as cargo");
        }
        const auto returning = cargo_returning(actor);
        check(actor.task != TraversalTask::Deliver || (actor.delivery && active(*actor.delivery)) ||
                  (returning && bag && bag->record.quantity.value > 0),
              "delivery task has no active order or carried batch");
        if (returning) {
            check(actor.station < scene_.stations.size() &&
                      (actor.known_stations & (std::uint32_t{1} << actor.station)) != 0,
                  "cargo return station is not known");
            const auto lot = world_.resource(scene_.stations[actor.station].lot);
            check(lot && lot->record.active && lot->record.kind == ResourceKind::Material &&
                      lot->record.material.value != 0 &&
                      lot->record.material == bag->record.material,
                  "cargo return station is incompatible");
        }
        if (!actor.delivery)
            continue;
        const auto& job = *actor.delivery;
        check(bag && job.source < scene_.stations.size() &&
                  job.destination < scene_.stations.size() && job.source != job.destination &&
                  job.quantity > 0 && job.quantity <= 1024 && job.delivered <= job.quantity &&
                  job.phase <= DeliveryPhase::Cancelled && job.started_command.value > 0 &&
                  job.started_command.value <= next_command_.value,
              "invalid delivery record");
        check(active(job) ? job.ended_command.value == 0
                          : job.ended_command.value >= job.started_command.value &&
                                job.ended_command.value <= next_command_.value,
              "invalid delivery command interval");
        const auto from = world_.resource(scene_.stations[job.source].lot);
        const auto to = world_.resource(scene_.stations[job.destination].lot);
        check(from && to && from->record.kind == ResourceKind::Material &&
                  to->record.kind == ResourceKind::Material &&
                  from->record.material == bag->record.material &&
                  to->record.material == bag->record.material,
              "delivery types disagree");
        std::uint64_t delivered = 0, in_transit = 0;
        std::vector<const TransferRecord*> flows;
        for (const auto& record : world_.transfers()) {
            const auto& r = record.request;
            if (record.result.status == TransferStatus::Accepted &&
                r.command.value >= job.started_command.value &&
                (job.ended_command.value == 0 || r.command.value < job.ended_command.value) &&
                (r.source == actor.cargo || r.destination == actor.cargo))
                flows.push_back(&record);
        }
        std::sort(flows.begin(), flows.end(), [](const auto* a, const auto* b) {
            return a->event_sequence < b->event_sequence;
        });
        for (const auto* record : flows) {
            const auto& r = record->request;
            check(r.actor == actor.id, "cargo transferred outside its delivery custodian");
            if (r.destination == actor.cargo) {
                check(r.source == from->record.id && r.quantity.value <= cargo_limit - in_transit &&
                          r.quantity.value <= job.quantity - delivered - in_transit,
                      "delivery loaded from invalid origin");
                in_transit += r.quantity.value;
            } else {
                check(r.quantity.value <= in_transit,
                      "delivery spent material that was not loaded");
                check(r.destination == from->record.id || r.destination == to->record.id,
                      "delivery dumped outside its order");
                in_transit -= r.quantity.value;
                if (r.destination == to->record.id) {
                    check(r.quantity.value <= job.quantity - delivered, "delivery exceeds order");
                    delivered += r.quantity.value;
                }
            }
        }
        check(delivered == job.delivered, "delivery progress is not backed by actual transfers");
        if (job.phase == DeliveryPhase::Completed)
            check(job.delivered == job.quantity && in_transit == 0,
                  "incomplete delivery marked completed");
        if (active(job)) {
            check(in_transit == bag->record.quantity.value,
                  "delivery cargo disagrees with its transfer history");
            if (job.phase == DeliveryPhase::ToDestination)
                check(bag->record.quantity.value > 0 &&
                          bag->record.quantity.value <= job.quantity - job.delivered,
                      "invalid carried delivery batch");
            else
                check(bag->record.quantity.value == 0,
                      "pickup or return carries an unexpected batch");
        }
    }
}
} // namespace reality
