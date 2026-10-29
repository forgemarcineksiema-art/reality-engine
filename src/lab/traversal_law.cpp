#include "lab/traversal_session.hpp"
#include <algorithm>
#include <stdexcept>
namespace reality {
const std::vector<PropertyIncident>& TraversalSession::incidents() const noexcept {
    return incidents_;
}
bool TraversalSession::legal_service_allowed(std::size_t actor, std::size_t station) const {
    const auto offender = actors_.at(actor).id;
    for (const auto& rule : property_rules_) {
        if (rule.station != station)
            continue;
        const auto custodian = std::find_if(actors_.begin(), actors_.end(),
                                            [&](const auto& a) { return a.id == rule.custodian; });
        for (const auto& report : custodian->testimonies) {
            if (report.resolved || report.offender != offender || report.station != station)
                continue;
            if (std::none_of(
                    custodian->testimonies.begin(), custodian->testimonies.end(),
                    [&](const auto& b) { return b.incident == report.incident && b.resolved; }))
                return false;
        }
    }
    return true;
}
bool TraversalSession::knowledge_contact(std::size_t from, std::size_t to) const {
    const auto& a = actors_.at(from);
    const auto& b = actors_.at(to);
    return a.local() && b.local() && from != to && length(a.feet - b.feet) <= 2 &&
           a.support.surface == b.support.surface &&
           spatial_segment_clear(geometry_, a.feet, b.feet, a.size) &&
           spatial_segment_clear(geometry_, b.feet, a.feet, b.size);
}
bool TraversalSession::authorized_pickup(std::size_t index, std::size_t station) const {
    const auto& a = actors_.at(index);
    for (const auto& rule : property_rules_) {
        if (rule.station != station)
            continue;
        return a.id == rule.custodian ||
               std::find(rule.residents.begin(), rule.residents.end(), a.id) !=
                   rule.residents.end() ||
               (a.id == rule.courier && a.delivery &&
                a.delivery->phase < DeliveryPhase::Completed && a.delivery->source == station);
    }
    return true;
}
void TraversalSession::observe_violation(const PropertyIncident& incident, bool resolved) {
    const auto offender = std::find_if(actors_.begin(), actors_.end(),
                                       [&](const auto& a) { return a.id == incident.offender; });
    const auto index = static_cast<std::size_t>(offender - actors_.begin());
    const auto now = world_.clock().tick_count();
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        if (!knowledge_contact(index, i))
            continue;
        auto& a = actors_[i];
        // Seeing a deposit cannot identify a previously unknown theft.
        if (resolved &&
            std::none_of(a.testimonies.begin(), a.testimonies.end(), [&](const auto& b) {
                return b.incident == incident.command && !b.resolved;
            }))
            continue;
        if (a.testimonies.size() >= 512)
            throw std::length_error("testimony limit");
        a.testimonies.push_back({incident.command, incident.offender, incident.lot,
                                 incident.station, incident.quantity, resolved, now, now, a.id,
                                 a.id});
        a.share_until = now + 40;
        a.share_retry_at = now;
    }
}
void TraversalSession::record_law_transfer(std::size_t index, std::size_t station, bool loading,
                                           std::uint64_t quantity, CommandId command) {
    const auto& a = actors_[index];
    if (loading) {
        if (authorized_pickup(index, station))
            return;
        if (incidents_.size() >= 256)
            throw std::length_error("property incident limit");
        incidents_.push_back({command,
                              a.id,
                              scene_.stations[station].lot,
                              a.cargo,
                              static_cast<std::uint32_t>(station),
                              quantity,
                              world_.clock().tick_count(),
                              0,
                              {}});
        observe_violation(incidents_.back(), false);
        return;
    }
    for (auto& incident : incidents_) {
        if (incident.offender != a.id || incident.station != station || incident.cargo != a.cargo)
            continue;
        const auto paid = std::min(quantity, incident.quantity - incident.returned);
        quantity -= paid;
        incident.returned += paid;
        if (paid != 0 && incident.returned == incident.quantity) {
            incident.resolved_command = command;
            observe_violation(incident, true);
        }
    }
}
void TraversalSession::validate_law() const {
    if (property_rules_.empty() && incidents_.empty() &&
        std::all_of(actors_.begin(), actors_.end(),
                    [](const auto& a) { return a.testimonies.empty(); }))
        return;
    const auto check = [](bool c, const char* m) {
        if (!c)
            throw std::invalid_argument(m);
    };
    const auto person = [&](EntityId id) -> const TraversalActor* {
        const auto a =
            std::find_if(actors_.begin(), actors_.end(), [&](const auto& v) { return v.id == id; });
        return a == actors_.end() ? nullptr : &*a;
    };
    const auto transfer = [&](CommandId id) -> const TransferRecord* {
        const auto t = std::find_if(world_.transfers().begin(), world_.transfers().end(),
                                    [&](const auto& v) { return v.request.command == id; });
        return t == world_.transfers().end() ? nullptr : &*t;
    };
    check(property_rules_.size() <= 16 && incidents_.size() <= 256, "property collection limit");
    std::vector<std::uint32_t> stations;
    for (const auto& rule : property_rules_) {
        check(rule.station < scene_.stations.size() && person(rule.custodian) &&
                  (rule.courier.value == 0 || person(rule.courier)) &&
                  rule.residents.size() <= actors_.size() &&
                  std::find(stations.begin(), stations.end(), rule.station) == stations.end(),
              "invalid property rule");
        const auto lot = world_.resource(scene_.stations[rule.station].lot);
        check(lot && lot->record.owner == rule.custodian, "invalid protected station resource");
        stations.push_back(rule.station);
        std::vector<EntityId> seen;
        for (const auto id : rule.residents) {
            check(person(id) && std::find(seen.begin(), seen.end(), id) == seen.end(),
                  "invalid authorized resident");
            seen.push_back(id);
        }
    }
    std::uint64_t previous = 0;
    std::vector<std::uint64_t> incident_sequences;
    for (const auto& incident : incidents_) {
        const auto* flow = transfer(incident.command);
        const auto* offender = person(incident.offender);
        check(flow && offender && incident.station < scene_.stations.size() &&
                  std::any_of(
                      property_rules_.begin(), property_rules_.end(),
                      [&](const auto& r) { return r.station == incident.station; }) &&
                  flow->result.status == TransferStatus::Accepted &&
                  flow->request.actor == incident.offender &&
                  flow->request.source == incident.lot &&
                  flow->request.destination == incident.cargo &&
                  scene_.stations[incident.station].lot == incident.lot &&
                  offender->cargo == incident.cargo &&
                  flow->request.quantity.value == incident.quantity && incident.quantity > 0 &&
                  flow->tick == incident.tick && flow->event_sequence > previous &&
                  incident.returned <= incident.quantity,
              "incident is not backed by a physical transfer");
        previous = flow->event_sequence;
        incident_sequences.push_back(previous);
    }
    // Reconstruct restitution in chronological order; one deposit pays at most its quantity.
    std::vector<std::uint64_t> paid(incidents_.size());
    std::vector<CommandId> closed(incidents_.size());
    std::vector<const TransferRecord*> flows;
    for (const auto& t : world_.transfers())
        if (t.result.status == TransferStatus::Accepted)
            flows.push_back(&t);
    std::sort(flows.begin(), flows.end(),
              [](const auto* a, const auto* b) { return a->event_sequence < b->event_sequence; });
    for (const auto* flow : flows) {
        auto budget = flow->request.quantity.value;
        for (std::size_t i = 0; i < incidents_.size(); ++i) {
            const auto& v = incidents_[i];
            if (flow->event_sequence <= incident_sequences[i] ||
                flow->request.actor != v.offender || flow->request.source != v.cargo ||
                flow->request.destination != v.lot)
                continue;
            const auto amount = std::min(budget, v.quantity - paid[i]);
            budget -= amount;
            paid[i] += amount;
            if (amount && paid[i] == v.quantity)
                closed[i] = flow->request.command;
        }
    }
    for (std::size_t i = 0; i < incidents_.size(); ++i)
        check(incidents_[i].returned == paid[i] && incidents_[i].resolved_command == closed[i],
              "unbacked incident settlement");
    for (const auto& a : actors_) {
        check(a.testimonies.size() <= 512, "testimony collection limit");
        std::vector<std::pair<CommandId, bool>> seen;
        for (const auto& b : a.testimonies) {
            const auto incident =
                std::find_if(incidents_.begin(), incidents_.end(),
                             [&](const auto& v) { return v.command == b.incident; });
            check(incident != incidents_.end(), "testimony references unknown incident");
            const auto* source = person(b.source_actor);
            const auto* via = person(b.via_actor);
            const auto* event =
                transfer(b.resolved ? incident->resolved_command : incident->command);
            const auto key = std::pair{b.incident, b.resolved};
            check(source && via && source->id != incident->offender && event &&
                      b.offender == incident->offender && b.lot == incident->lot &&
                      b.station == incident->station && b.quantity == incident->quantity &&
                      b.observed_tick == event->tick && b.observed_tick <= b.learned_tick &&
                      b.learned_tick <= world_.clock().tick_count() &&
                      std::find(seen.begin(), seen.end(), key) == seen.end(),
                  "invalid testimony fact or provenance");
            seen.push_back(key);
            if (b.resolved)
                check(std::any_of(a.testimonies.begin(), a.testimonies.end(),
                                  [&](const auto& r) {
                                      const auto deadline =
                                          b.source_actor == a.id ? b.observed_tick : b.learned_tick;
                                      return r.incident == b.incident && !r.resolved &&
                                             r.learned_tick <= deadline;
                                  }),
                      "settlement testimony lacks prior incident knowledge");
            const auto matching = [&](const auto& r) {
                return r.incident == b.incident && r.resolved == b.resolved &&
                       r.source_actor == b.source_actor && r.observed_tick == b.observed_tick;
            };
            const auto root =
                std::find_if(source->testimonies.begin(), source->testimonies.end(), matching);
            check(root != source->testimonies.end() && root->via_actor == source->id &&
                      root->learned_tick == root->observed_tick,
                  "missing original eyewitness");
            if (b.source_actor == a.id || b.via_actor == a.id)
                check(b.source_actor == a.id && b.via_actor == a.id &&
                          b.learned_tick == b.observed_tick,
                      "invalid personal eyewitness");
            else {
                const auto relay =
                    std::find_if(via->testimonies.begin(), via->testimonies.end(), matching);
                check(relay != via->testimonies.end() && relay->learned_tick <= b.learned_tick,
                      "missing physical report source");
            }
        }
    }
}
} // namespace reality
