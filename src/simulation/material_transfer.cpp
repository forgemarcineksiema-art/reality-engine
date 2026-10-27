#include "core/bounded_records.hpp"
#include "simulation/work_world.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace reality {
namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
template <typename T> auto locate(const T& records, EntityId id) {
    return std::find_if(records.begin(), records.end(), [&](const auto& r) { return r.id == id; });
}
} // namespace
const std::vector<TransferRecord>& WorkWorld::transfers() const noexcept { return transfers_; }
TransferResult WorkWorld::transfer(const TransferRequest& request) {
    if (request.command.value == 0)
        throw std::invalid_argument("command id must be nonzero");
    const auto previous = std::find_if(transfers_.begin(), transfers_.end(), [&](const auto& r) {
        return r.request.command == request.command;
    });
    if (previous != transfers_.end()) {
        if (previous->request != request)
            throw std::invalid_argument("command id reused with different transfer");
        return previous->result;
    }
    if (std::any_of(commands_.begin(), commands_.end(),
                    [&](const auto& r) { return r.request.command == request.command; }))
        throw std::invalid_argument("command id already used by reservation");
    TransferResult result{TransferStatus::Accepted};
    const auto source = resource(request.source), target = resource(request.destination);
    if (request.quantity.value == 0 || request.source == request.destination)
        result.status = TransferStatus::InvalidRequest;
    else if (!entities_.contains(request.actor, EntityKind::Actor))
        result.status = TransferStatus::ActorUnavailable;
    else if (!source || !target || !source->record.active || !target->record.active)
        result.status = TransferStatus::ResourceUnavailable;
    else if (source->record.kind != ResourceKind::Material ||
             target->record.kind != ResourceKind::Material || source->record.material.value == 0 ||
             source->record.material != target->record.material)
        result.status = TransferStatus::IncompatibleMaterial;
    else if (source->available < request.quantity)
        result.status = TransferStatus::InsufficientResource;
    else if (request.quantity.value >
             std::numeric_limits<std::uint64_t>::max() - target->record.quantity.value)
        result.status = TransferStatus::Overflow;
    if (commands_.size() + transfers_.size() >= record_limit)
        throw std::length_error("prototype command limit reached");
    prepare_records(transfers_, 1);
    if (result.status == TransferStatus::Accepted)
        prepare_records(events_, 2);
    const auto sequence = result.status == TransferStatus::Accepted ? events_.size() + 1 : 0;
    if (result.status == TransferStatus::Accepted) {
        auto from = std::find_if(resources_.begin(), resources_.end(),
                                 [&](const auto& r) { return r.id == request.source; });
        auto to = std::find_if(resources_.begin(), resources_.end(),
                               [&](const auto& r) { return r.id == request.destination; });
        from->quantity.value -= request.quantity.value;
        to->quantity.value += request.quantity.value;
        emit(EventKind::TransferOut, request.source, request.quantity, clock_.tick_count());
        emit(EventKind::TransferIn, request.destination, request.quantity, clock_.tick_count());
    }
    transfers_.push_back({request, result, clock_.tick_count(), sequence});
    return result;
}
void WorkWorld::validate_material_flow() const {
    check(transfers_.size() <= record_limit && commands_.size() + transfers_.size() <= record_limit,
          "transfer command limit exceeded");
    std::set<CommandId> ids;
    for (const auto& command : commands_)
        ids.insert(command.request.command);
    std::map<std::uint64_t, const TransferRecord*> pairs;
    for (const auto& record : transfers_) {
        const auto& r = record.request;
        check(r.command.value != 0 && ids.insert(r.command).second &&
                  record.result.status <= TransferStatus::Overflow &&
                  record.tick <= clock_.tick_count(),
              "invalid transfer command record");
        if (record.result.status != TransferStatus::Accepted) {
            check(record.event_sequence == 0, "denied transfer owns events");
            continue;
        }
        check(r.quantity.value > 0 && r.source != r.destination && record.event_sequence > 0 &&
                  record.event_sequence < events_.size(),
              "invalid accepted transfer");
        const auto from = locate(resources_, r.source), to = locate(resources_, r.destination);
        check(from != resources_.end() && to != resources_.end() &&
                  from->kind == ResourceKind::Material && to->kind == ResourceKind::Material &&
                  from->material.value != 0 && from->material == to->material,
              "transfer material mismatch");
        check(pairs.emplace(record.event_sequence, &record).second,
              "duplicate transfer event pair");
        const auto& out = events_[static_cast<std::size_t>(record.event_sequence - 1)];
        const auto& in = events_[static_cast<std::size_t>(record.event_sequence)];
        check(out.kind == EventKind::TransferOut && in.kind == EventKind::TransferIn &&
                  out.subject == r.source && in.subject == r.destination &&
                  out.quantity == r.quantity && in.quantity == r.quantity &&
                  out.tick == record.tick && in.tick == record.tick,
              "transfer journal pair mismatch");
    }
    struct Lot {
        std::uint64_t amount = 0, held = 0;
        bool active = false, created = false;
    };
    std::map<EntityId, Lot> lots;
    std::map<EntityId, bool> actors;
    std::map<EntityId, std::uint64_t> holds;
    std::size_t flows = 0;
    for (const auto& event : events_) {
        if (event.kind == EventKind::ActorCreated) {
            actors[event.subject] = true;
            continue;
        }
        if (event.kind == EventKind::ActorRemoved) {
            actors[event.subject] = false;
            continue;
        }
        if (event.kind == EventKind::ResourceCreated) {
            auto& lot = lots[event.subject];
            check(!lot.created, "material journal duplicate creation");
            lot = {event.quantity.value, 0, true, true};
            continue;
        }
        if (event.kind == EventKind::ResourceRemoved) {
            auto& lot = lots[event.subject];
            check(lot.active && lot.held == 0 && event.quantity.value == lot.amount,
                  "material loss journal mismatch");
            lot.amount = 0;
            lot.active = false;
            continue;
        }
        if (event.kind == EventKind::TransferOut || event.kind == EventKind::TransferIn) {
            const auto sequence =
                event.kind == EventKind::TransferOut ? event.sequence : event.sequence - 1;
            const auto record = pairs.find(sequence);
            check(record != pairs.end(), "orphan transfer event");
            const auto& r = record->second->request;
            auto& from = lots[r.source];
            auto& to = lots[r.destination];
            check(actors[r.actor] && from.active && to.active,
                  "transfer outside participant lifetime");
            if (event.kind == EventKind::TransferOut) {
                check(from.held <= from.amount && r.quantity.value <= from.amount - from.held,
                      "historical transfer spent reserved or missing material");
                from.amount -= r.quantity.value;
            } else {
                check(r.quantity.value <= std::numeric_limits<std::uint64_t>::max() - to.amount,
                      "historical transfer overflow");
                to.amount += r.quantity.value;
                ++flows;
            }
            continue;
        }
        const auto action = locate(actions_, event.subject);
        check(action != actions_.end(), "material journal unknown action");
        auto& lot = lots[action->request.lot];
        auto& held = holds[action->id];
        if (event.kind == EventKind::Reserved) {
            check(lot.active && lot.held <= lot.amount &&
                      event.quantity.value <= lot.amount - lot.held,
                  "historical overreservation");
            held = event.quantity.value;
            lot.held += held;
        } else if (event.kind == EventKind::Consumed) {
            check(lot.active && event.quantity.value <= lot.amount &&
                      event.quantity.value <= held && event.quantity.value <= lot.held,
                  "historical consumption exceeds material");
            lot.amount -= event.quantity.value;
            lot.held -= event.quantity.value;
            held -= event.quantity.value;
        } else if (event.kind == EventKind::Completed || event.kind == EventKind::Cancelled) {
            check(held <= lot.held, "historical reservation release mismatch");
            lot.held -= held;
            held = 0;
        }
    }
    check(flows == pairs.size(), "transfer without journal effects");
    for (const auto& record : resources_) {
        const auto lot = lots.find(record.id);
        check(lot != lots.end() && lot->second.amount == record.quantity.value &&
                  lot->second.active == record.active &&
                  lot->second.held == resource(record.id)->reserved.value,
              "chronological material balance mismatch");
    }
}
} // namespace reality
