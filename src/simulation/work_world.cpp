#include "simulation/work_world.hpp"
#include "core/bounded_records.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace reality {
namespace {

template <typename Records> auto locate(Records& records, EntityId id) {
    return std::find_if(records.begin(), records.end(),
                        [id](const auto& record) { return record.id == id; });
}

template <typename T> void prepare(std::vector<T>& records, std::size_t extra) {
    prepare_records(records, extra);
}

bool active(const ActionRecord& action) {
    return action.state == ActionState::Reserved || action.state == ActionState::Running;
}

struct EventAudit {
    unsigned phase = 0;
    std::uint64_t consumed = 0;
    bool upfront_seen = false;
    bool remaining_seen = false;
};

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

WorkWorld::WorkWorld(std::chrono::nanoseconds step) : clock_(step) {}

void WorkWorld::emit(EventKind kind, EntityId subject, Quantity quantity, std::uint64_t tick) {
    events_.push_back(WorldEvent{events_.size() + 1, tick, kind, subject, quantity});
}

EntityId WorkWorld::create_actor() {
    prepare(actors_, 1);
    prepare(events_, 1);
    const auto id = entities_.create(EntityKind::Actor);
    actors_.push_back({id, true});
    emit(EventKind::ActorCreated, id, {}, clock_.tick_count());
    return id;
}

EntityId WorkWorld::create_resource(EntityId owner, Quantity quantity, ResourceKind kind,
                                    MaterialTypeId material) {
    if (kind != ResourceKind::Material && kind != ResourceKind::Capacity) {
        throw std::invalid_argument("invalid resource kind");
    }
    if (!entities_.contains(owner, EntityKind::Actor)) {
        throw std::invalid_argument("resource owner is not a live actor");
    }
    if (kind == ResourceKind::Capacity && material.value != 0)
        throw std::invalid_argument("capacity cannot have a material type");
    prepare(resources_, 1);
    prepare(events_, 1);
    const auto id = entities_.create(EntityKind::Resource);
    resources_.push_back({id, owner, quantity, quantity, {}, true, kind, material});
    emit(EventKind::ResourceCreated, id, quantity, clock_.tick_count());
    return id;
}

RequestResult WorkWorld::reserve(const WorkRequest& request) {
    if (request.command.value == 0) {
        throw std::invalid_argument("command id must be nonzero");
    }
    if (std::any_of(transfers_.begin(), transfers_.end(),
                    [&](const auto& r) { return r.request.command == request.command; }))
        throw std::invalid_argument("command id already used by transfer");
    const auto previous = std::find_if(commands_.begin(), commands_.end(), [&](const auto& record) {
        return record.request.command == request.command;
    });
    if (previous != commands_.end()) {
        if (previous->request != request) {
            throw std::invalid_argument("command id reused with different payload");
        }
        return previous->result;
    }
    RequestResult result{RequestStatus::Accepted, {}};
    if (request.required.value == 0 || request.duration_ticks == 0 ||
        request.consume_on_start > request.required) {
        result.status = RequestStatus::InvalidRequest;
    } else if (!entities_.contains(request.actor, EntityKind::Actor)) {
        result.status = RequestStatus::ActorUnavailable;
    } else if (!entities_.contains(request.lot, EntityKind::Resource)) {
        result.status = RequestStatus::ResourceUnavailable;
    } else if (resource(request.lot)->record.kind == ResourceKind::Capacity &&
               request.consume_on_start.value != 0) {
        result.status = RequestStatus::InvalidRequest;
    } else if (resource(request.lot)->available < request.required) {
        result.status = RequestStatus::InsufficientResource;
    }
    if (commands_.size() + transfers_.size() >= record_limit)
        throw std::length_error("prototype command limit reached");
    prepare(commands_, 1);
    if (result.status == RequestStatus::Accepted) {
        prepare(actions_, 1);
        prepare(events_, 1);
        result.action = entities_.create(EntityKind::Action);
        actions_.push_back(ActionRecord{result.action, request, {}});
        emit(EventKind::Reserved, result.action, request.required, clock_.tick_count());
    }
    commands_.push_back({request, result});
    return result;
}

Transition WorkWorld::begin(EntityId id) {
    const auto action = locate(actions_, id);
    if (action == actions_.end()) {
        return Transition::Missing;
    }
    if (action->state == ActionState::Running || action->state == ActionState::Completed) {
        return Transition::Unchanged;
    }
    if (action->state != ActionState::Reserved) {
        return Transition::InvalidState;
    }
    const auto maximum_tick = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max() /
                                                         clock_.step().count());
    if (action->request.duration_ticks > maximum_tick - clock_.tick_count()) {
        return Transition::TimeOverflow;
    }
    prepare(events_, action->request.consume_on_start.value == 0 ? 1 : 2);
    auto lot = locate(resources_, action->request.lot);
    lot->quantity.value -= action->request.consume_on_start.value;
    action->consumed = action->request.consume_on_start;
    action->started_at = clock_.tick_count();
    action->due_at = action->started_at + action->request.duration_ticks;
    action->state = ActionState::Running;
    emit(EventKind::Started, id, {}, clock_.tick_count());
    if (action->consumed.value != 0) {
        emit(EventKind::Consumed, id, action->consumed, clock_.tick_count());
    }
    return Transition::Changed;
}

void WorkWorld::cancel_active(ActionRecord& action, CancelReason reason) {
    action.state = ActionState::Cancelled;
    action.reason = reason;
    action.ended_at = clock_.tick_count();
    emit(EventKind::Cancelled, action.id,
         Quantity{action.request.required.value - action.consumed.value}, clock_.tick_count());
}

Transition WorkWorld::cancel(EntityId id) {
    const auto action = locate(actions_, id);
    if (action == actions_.end()) {
        return Transition::Missing;
    }
    if (action->state == ActionState::Cancelled) {
        return Transition::Unchanged;
    }
    if (!active(*action)) {
        return Transition::InvalidState;
    }
    prepare(events_, 1);
    cancel_active(*action, CancelReason::Requested);
    return Transition::Changed;
}

bool WorkWorld::remove_actor(EntityId id) {
    auto actor = locate(actors_, id);
    if (actor == actors_.end() || !actor->alive) {
        return false;
    }
    const auto affected = std::count_if(actions_.begin(), actions_.end(), [id](const auto& action) {
        return active(action) && action.request.actor == id;
    });
    prepare(events_, static_cast<std::size_t>(affected) + 1);
    for (auto& action : actions_) {
        if (active(action) && action.request.actor == id) {
            cancel_active(action, CancelReason::ActorRemoved);
        }
    }
    actor->alive = false;
    entities_.destroy(id);
    emit(EventKind::ActorRemoved, id, {}, clock_.tick_count());
    return true;
}

bool WorkWorld::remove_resource(EntityId id) {
    auto lot = locate(resources_, id);
    if (lot == resources_.end() || !lot->active) {
        return false;
    }
    const auto affected = std::count_if(actions_.begin(), actions_.end(), [id](const auto& action) {
        return active(action) && action.request.lot == id;
    });
    prepare(events_, static_cast<std::size_t>(affected) + 1);
    for (auto& action : actions_) {
        if (active(action) && action.request.lot == id) {
            cancel_active(action, CancelReason::ResourceRemoved);
        }
    }
    lot->lost = lot->quantity;
    lot->quantity = {};
    lot->active = false;
    entities_.destroy(id);
    emit(EventKind::ResourceRemoved, id, lot->lost, clock_.tick_count());
    return true;
}

void WorkWorld::advance(std::uint64_t ticks) {
    auto next_clock = clock_;
    next_clock.advance(ticks);
    std::vector<std::size_t> due;
    for (std::size_t index = 0; index < actions_.size(); ++index) {
        if (actions_[index].state == ActionState::Running &&
            actions_[index].due_at <= next_clock.tick_count()) {
            due.push_back(index);
        }
    }
    std::sort(due.begin(), due.end(), [&](auto left, auto right) {
        return std::tie(actions_[left].due_at, actions_[left].id) <
               std::tie(actions_[right].due_at, actions_[right].id);
    });
    std::size_t event_count = due.size();
    for (const auto index : due) {
        const auto& action = actions_[index];
        event_count += locate(resources_, action.request.lot)->kind == ResourceKind::Material &&
                               action.consumed < action.request.required
                           ? 1
                           : 0;
    }
    prepare(events_, event_count);
    clock_ = next_clock;
    for (const auto index : due) {
        auto& action = actions_[index];
        auto lot = locate(resources_, action.request.lot);
        const Quantity remaining{lot->kind == ResourceKind::Material
                                     ? action.request.required.value - action.consumed.value
                                     : 0};
        lot->quantity.value -= remaining.value;
        action.consumed.value += remaining.value;
        action.state = ActionState::Completed;
        action.ended_at = action.due_at;
        if (remaining.value != 0) {
            emit(EventKind::Consumed, action.id, remaining, action.due_at);
        }
        emit(EventKind::Completed, action.id, {}, action.due_at);
    }
}

std::optional<ResourceView> WorkWorld::resource(EntityId id) const {
    const auto lot = locate(resources_, id);
    if (lot == resources_.end()) {
        return std::nullopt;
    }
    std::uint64_t reserved = 0;
    for (const auto& action : actions_) {
        if (active(action) && action.request.lot == id) {
            const auto held = action.request.required.value - action.consumed.value;
            check(held <= lot->quantity.value && reserved <= lot->quantity.value - held,
                  "reservations exceed physical quantity");
            reserved += held;
        }
    }
    return ResourceView{*lot, Quantity{reserved}, Quantity{lot->quantity.value - reserved}};
}

std::optional<ActionRecord> WorkWorld::action(EntityId id) const {
    const auto found = locate(actions_, id);
    return found == actions_.end() ? std::nullopt : std::optional<ActionRecord>{*found};
}

const EntityRegistry& WorkWorld::entities() const noexcept { return entities_; }
const SimulationClock& WorkWorld::clock() const noexcept { return clock_; }
const std::vector<WorldEvent>& WorkWorld::events() const noexcept { return events_; }
const std::vector<CommandRecord>& WorkWorld::commands() const noexcept { return commands_; }

std::string_view describe(ActionState state) {
    switch (state) {
    case ActionState::Reserved:
        return "reserved";
    case ActionState::Running:
        return "running";
    case ActionState::Completed:
        return "completed";
    case ActionState::Cancelled:
        return "cancelled";
    }
    throw std::logic_error("invalid action state");
}

std::string_view describe(RequestStatus status) {
    switch (status) {
    case RequestStatus::Accepted:
        return "accepted";
    case RequestStatus::InvalidRequest:
        return "invalid_request";
    case RequestStatus::ActorUnavailable:
        return "actor_unavailable";
    case RequestStatus::ResourceUnavailable:
        return "resource_unavailable";
    case RequestStatus::InsufficientResource:
        return "insufficient_resource";
    }
    throw std::logic_error("invalid request status");
}

std::string_view describe(CancelReason reason) {
    switch (reason) {
    case CancelReason::None:
        return "none";
    case CancelReason::Requested:
        return "requested";
    case CancelReason::ActorRemoved:
        return "actor_removed";
    case CancelReason::ResourceRemoved:
        return "resource_removed";
    }
    throw std::logic_error("invalid cancellation reason");
}

std::string_view describe(EventKind kind) {
    switch (kind) {
    case EventKind::ActorCreated:
        return "actor_created";
    case EventKind::ActorRemoved:
        return "actor_removed";
    case EventKind::ResourceCreated:
        return "resource_created";
    case EventKind::ResourceRemoved:
        return "resource_removed";
    case EventKind::Reserved:
        return "reserved";
    case EventKind::Started:
        return "started";
    case EventKind::Consumed:
        return "consumed";
    case EventKind::Completed:
        return "completed";
    case EventKind::Cancelled:
        return "cancelled";
    case EventKind::TransferOut:
        return "transfer_out";
    case EventKind::TransferIn:
        return "transfer_in";
    }
    throw std::logic_error("invalid event kind");
}

std::string WorkWorld::inspect() const {
    std::ostringstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out << "tick=" << clock_.tick_count() << " elapsed_ns=" << clock_.elapsed().count() << '\n';
    for (const auto& actor : actors_) {
        out << "actor=" << actor.id.value << " alive=" << actor.alive << '\n';
    }
    for (const auto& lot : resources_) {
        const auto view = resource(lot.id).value();
        out << "lot=" << lot.id.value << " owner=" << lot.owner.value
            << " quantity=" << lot.quantity.value << " reserved=" << view.reserved.value
            << " available=" << view.available.value << " active=" << lot.active;
        if (lot.kind == ResourceKind::Capacity)
            out << " kind=capacity";
        if (lot.material.value != 0)
            out << " material=" << lot.material.value;
        out << '\n';
    }
    for (const auto& action : actions_) {
        out << "action=" << action.id.value << " actor=" << action.request.actor.value
            << " lot=" << action.request.lot.value << " state=" << describe(action.state)
            << " consumed=" << action.consumed.value << " due=" << action.due_at
            << " reason=" << describe(action.reason) << '\n';
    }
    for (const auto& command : commands_) {
        out << "command=" << command.request.command.value
            << " result=" << describe(command.result.status)
            << " action=" << command.result.action.value << '\n';
    }
    for (const auto& event : events_) {
        out << "event=" << event.sequence << " tick=" << event.tick
            << " kind=" << describe(event.kind) << " subject=" << event.subject.value
            << " quantity=" << event.quantity.value << '\n';
    }
    for (const auto& record : transfers_)
        out << "transfer_command=" << record.request.command.value
            << " from=" << record.request.source.value << " to=" << record.request.destination.value
            << " quantity=" << record.request.quantity.value
            << " result=" << static_cast<unsigned>(record.result.status) << '\n';
    return out.str();
}

void WorkWorld::validate() const {
    entities_.validate();
    for (const auto size :
         {actors_.size(), resources_.size(), actions_.size(), commands_.size(), events_.size()}) {
        check(size <= record_limit, "world record limit exceeded");
    }
    std::set<EntityId> ids;
    const auto register_id = [&](EntityId id) {
        check(id.value != 0 && id.value < entities_.next_id() && ids.insert(id).second,
              "invalid or duplicate world id");
    };
    std::size_t expected_live = actions_.size();
    for (const auto& actor : actors_) {
        register_id(actor.id);
        check(entities_.contains(actor.id, EntityKind::Actor) == actor.alive,
              "actor registry mismatch");
        expected_live += actor.alive ? 1 : 0;
    }
    for (const auto& lot : resources_) {
        register_id(lot.id);
        check(lot.kind == ResourceKind::Material || lot.kind == ResourceKind::Capacity,
              "invalid resource kind");
        check(locate(actors_, lot.owner) != actors_.end(), "unknown resource owner");
        check(entities_.contains(lot.id, EntityKind::Resource) == lot.active,
              "resource registry mismatch");
        check((!transfers_.empty() || lot.lost <= lot.initial) &&
                  (lot.active ? lot.lost.value == 0 : lot.quantity.value == 0),
              "invalid resource loss");
        expected_live += lot.active ? 1 : 0;
        check(lot.kind != ResourceKind::Capacity || lot.material.value == 0, "typed capacity");
    }
    const auto now = clock_.tick_count();
    for (const auto& action : actions_) {
        register_id(action.id);
        check(entities_.contains(action.id, EntityKind::Action), "action registry mismatch");
        check(locate(actors_, action.request.actor) != actors_.end() &&
                  locate(resources_, action.request.lot) != resources_.end(),
              "unknown action participant");
        check(action.request.command.value != 0 && action.request.required.value != 0 &&
                  action.request.duration_ticks != 0 &&
                  action.request.consume_on_start <= action.request.required &&
                  action.consumed <= action.request.required,
              "invalid action quantities");
        const auto capacity =
            locate(resources_, action.request.lot)->kind == ResourceKind::Capacity;
        check(!capacity ||
                  (action.request.consume_on_start.value == 0 && action.consumed.value == 0),
              "capacity cannot be consumed");
        check(action.state <= ActionState::Cancelled &&
                  action.reason <= CancelReason::ResourceRemoved,
              "invalid action enum");
        if (active(action)) {
            check(entities_.contains(action.request.actor, EntityKind::Actor) &&
                      entities_.contains(action.request.lot, EntityKind::Resource),
                  "active action has missing participant");
        }
        if (action.state == ActionState::Reserved) {
            check(action.consumed.value == 0 && action.started_at == 0 && action.due_at == 0 &&
                      action.ended_at == 0 && action.reason == CancelReason::None,
                  "invalid reserved phase");
        } else {
            const bool began = action.due_at != 0;
            if (began) {
                const auto maximum_tick = static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max() / clock_.step().count());
                check(action.started_at <= now && action.due_at <= maximum_tick &&
                          action.due_at > action.started_at &&
                          action.due_at - action.started_at == action.request.duration_ticks,
                      "invalid action deadline");
            } else {
                check(action.started_at == 0 && action.consumed.value == 0,
                      "invalid unstarted action");
            }
            if (action.state == ActionState::Running) {
                check(began && action.due_at > now && action.ended_at == 0 &&
                          action.consumed == action.request.consume_on_start &&
                          action.reason == CancelReason::None,
                      "invalid running phase");
            } else if (action.state == ActionState::Completed) {
                check(began && action.ended_at == action.due_at && action.ended_at <= now &&
                          action.consumed == (capacity ? Quantity{} : action.request.required) &&
                          action.reason == CancelReason::None,
                      "invalid completed phase");
            } else {
                check(action.reason != CancelReason::None && action.ended_at <= now &&
                          (!began || (action.ended_at >= action.started_at &&
                                      action.ended_at < action.due_at &&
                                      action.consumed == action.request.consume_on_start)),
                      "invalid cancelled phase");
            }
        }
    }
    const auto actual_live = std::count_if(entities_.slots().begin(), entities_.slots().end(),
                                           [](const auto& slot) { return slot.occupied; });
    check(static_cast<std::size_t>(actual_live) == expected_live, "unowned registry entry");
    for (const auto& slot : entities_.slots()) {
        check(ids.contains(slot.id), "registry slot has no historical record");
        const bool correct_kind =
            (slot.kind == EntityKind::Actor && locate(actors_, slot.id) != actors_.end()) ||
            (slot.kind == EntityKind::Resource &&
             locate(resources_, slot.id) != resources_.end()) ||
            (slot.kind == EntityKind::Action && locate(actions_, slot.id) != actions_.end());
        check(correct_kind, "historical registry kind mismatch");
    }
    for (const auto& lot : resources_) {
        if (!transfers_.empty()) {
            static_cast<void>(resource(lot.id));
            continue;
        }
        auto remainder = lot.initial.value - lot.lost.value;
        for (const auto& action : actions_) {
            if (action.request.lot == lot.id) {
                check(action.consumed.value <= remainder, "consumption exceeds original quantity");
                remainder -= action.consumed.value;
            }
        }
        check(lot.quantity.value == remainder, "resource balance mismatch");
        static_cast<void>(resource(lot.id));
    }
    std::set<CommandId> command_ids;
    std::set<EntityId> accepted_actions;
    for (const auto& command : commands_) {
        check(command.request.command.value != 0 &&
                  command_ids.insert(command.request.command).second &&
                  command.result.status <= RequestStatus::InsufficientResource,
              "invalid command record");
        if (command.result.status == RequestStatus::Accepted) {
            const auto action = locate(actions_, command.result.action);
            check(action != actions_.end() && action->request == command.request &&
                      accepted_actions.insert(action->id).second,
                  "command action mismatch");
        } else {
            check(command.result.action.value == 0, "denied command owns action");
        }
    }
    check(accepted_actions.size() == actions_.size(), "action without accepted command");
    std::uint64_t previous_tick = 0;
    std::map<EntityId, EventAudit> audit;
    for (std::size_t index = 0; index < events_.size(); ++index) {
        const auto& event = events_[index];
        check(event.sequence == index + 1 && event.tick >= previous_tick && event.tick <= now &&
                  event.kind <= EventKind::TransferIn && ids.contains(event.subject),
              "invalid world event");
        previous_tick = event.tick;
        auto& entry = audit[event.subject];
        if (event.kind == EventKind::ActorCreated || event.kind == EventKind::ActorRemoved) {
            const auto actor = locate(actors_, event.subject);
            check(actor != actors_.end() && event.quantity.value == 0, "invalid actor event");
            if (event.kind == EventKind::ActorCreated) {
                check(entry.phase == 0, "duplicate actor creation");
                entry.phase = 1;
            } else {
                check(entry.phase == 1 && !actor->alive, "invalid actor removal");
                entry.phase = 2;
            }
        } else if (event.kind == EventKind::ResourceCreated ||
                   event.kind == EventKind::ResourceRemoved) {
            const auto lot = locate(resources_, event.subject);
            check(lot != resources_.end(), "invalid resource event");
            if (event.kind == EventKind::ResourceCreated) {
                check(entry.phase == 0 && audit[lot->owner].phase == 1 &&
                          event.quantity == lot->initial,
                      "invalid resource creation");
                entry.phase = 1;
            } else {
                check(entry.phase == 1 && !lot->active && event.quantity == lot->lost,
                      "invalid resource removal");
                entry.phase = 2;
            }
        } else if (event.kind == EventKind::TransferOut || event.kind == EventKind::TransferIn) {
            check(locate(resources_, event.subject) != resources_.end() && entry.phase == 1 &&
                      event.quantity.value > 0,
                  "invalid material flow event");
        } else {
            const auto action = locate(actions_, event.subject);
            check(action != actions_.end(), "invalid action event");
            switch (event.kind) {
            case EventKind::Reserved:
                check(entry.phase == 0 && event.quantity == action->request.required &&
                          audit[action->request.actor].phase == 1 &&
                          audit[action->request.lot].phase == 1,
                      "invalid reservation event");
                entry.phase = 1;
                break;
            case EventKind::Started:
                check(entry.phase == 1 && action->due_at != 0 && event.tick == action->started_at &&
                          event.quantity.value == 0 && audit[action->request.actor].phase == 1 &&
                          audit[action->request.lot].phase == 1,
                      "invalid start event");
                entry.phase = 2;
                break;
            case EventKind::Consumed: {
                check(entry.phase == 2 && event.quantity.value != 0 &&
                          event.quantity.value <= action->consumed.value &&
                          entry.consumed <= action->consumed.value - event.quantity.value,
                      "invalid consumption event");
                if (event.tick == action->started_at) {
                    check(!entry.upfront_seen && event.quantity == action->request.consume_on_start,
                          "invalid upfront consumption");
                    entry.upfront_seen = true;
                } else {
                    check(!entry.remaining_seen && action->state == ActionState::Completed &&
                              event.tick == action->due_at &&
                              event.quantity.value == action->request.required.value -
                                                          action->request.consume_on_start.value,
                          "invalid completion consumption");
                    entry.remaining_seen = true;
                }
                entry.consumed += event.quantity.value;
                break;
            }
            case EventKind::Completed:
                check(entry.phase == 2 && action->state == ActionState::Completed &&
                          event.tick == action->due_at && event.quantity.value == 0,
                      "invalid completion event");
                entry.phase = 3;
                break;
            case EventKind::Cancelled:
                check((entry.phase == 1 || entry.phase == 2) &&
                          action->state == ActionState::Cancelled &&
                          event.tick == action->ended_at &&
                          event.quantity.value ==
                              action->request.required.value - action->consumed.value,
                      "invalid cancellation event");
                entry.phase = 3;
                break;
            default:
                throw std::runtime_error("unexpected action event kind");
            }
        }
    }
    for (const auto& actor : actors_) {
        check(audit[actor.id].phase == (actor.alive ? 1U : 2U), "actor journal mismatch");
    }
    for (const auto& lot : resources_) {
        check(audit[lot.id].phase == (lot.active ? 1U : 2U), "resource journal mismatch");
    }
    for (const auto& action : actions_) {
        const auto& entry = audit[action.id];
        const auto expected_phase = action.state == ActionState::Reserved  ? 1U
                                    : action.state == ActionState::Running ? 2U
                                                                           : 3U;
        check(entry.phase == expected_phase && entry.consumed == action.consumed.value,
              "action journal mismatch");
    }
    validate_material_flow();
}

} // namespace reality
