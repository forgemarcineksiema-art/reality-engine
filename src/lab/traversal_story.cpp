#include "lab/traversal_session.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
namespace reality {
namespace {
void check(bool v, const char* message) {
    if (!v)
        throw std::invalid_argument(message);
}
std::optional<std::size_t> person(const TraversalSession& s, EntityId id) {
    for (std::size_t i = 0; i < s.actors().size(); ++i)
        if (s.actors()[i].id == id)
            return i;
    return std::nullopt;
}
std::optional<std::size_t> station(const TraversalSession& s, EntityId id) {
    for (std::size_t i = 0; i < s.stations().size(); ++i)
        if (s.stations()[i].lot == id)
            return i;
    return std::nullopt;
}
bool body(const SpatialGeometry& g, std::uint32_t id) {
    return std::any_of(g.bodies.begin(), g.bodies.end(), [&](const auto& b) { return b.id == id; });
}
std::uint64_t cursor(const WorkWorld& w) {
    return w.events().empty() ? 0 : w.events().back().sequence;
}
std::uint64_t available_at(const WorkWorld& w, EntityId lot, std::uint64_t boundary) {
    std::uint64_t quantity = 0;
    bool created = false;
    std::map<EntityId, std::uint64_t> holds;
    for (const auto& e : w.events()) {
        if (e.sequence > boundary)
            break;
        if (e.subject == lot) {
            if (e.kind == EventKind::ResourceCreated) {
                quantity = e.quantity.value;
                created = true;
            } else if (e.kind == EventKind::TransferIn) {
                check(e.quantity.value <= UINT64_MAX - quantity, "story quantity overflow");
                quantity += e.quantity.value;
            } else if (e.kind == EventKind::TransferOut || e.kind == EventKind::ResourceRemoved) {
                check(e.quantity.value <= quantity, "story quantity underflow");
                quantity -= e.quantity.value;
            }
        }
        if (e.kind == EventKind::Reserved || e.kind == EventKind::Consumed ||
            e.kind == EventKind::Cancelled || e.kind == EventKind::Completed) {
            const auto a = w.action(e.subject);
            if (!a || a->request.lot != lot)
                continue;
            if (e.kind == EventKind::Reserved)
                holds[e.subject] = a->request.required.value;
            if (e.kind == EventKind::Consumed) {
                check(e.quantity.value <= quantity && e.quantity.value <= holds[e.subject],
                      "story consumption underflow");
                quantity -= e.quantity.value;
                holds[e.subject] -= e.quantity.value;
            }
            if (e.kind == EventKind::Cancelled || e.kind == EventKind::Completed)
                holds.erase(e.subject);
        }
    }
    check(created, "story source did not yet exist");
    for (const auto& [id, held] : holds) {
        static_cast<void>(id);
        check(held <= quantity, "story held quantity underflow");
        quantity -= held;
    }
    return quantity;
}
std::uint64_t delivery_event(const WorkWorld& w, const StoryState& st, std::uint64_t boundary) {
    std::uint64_t delivered = 0, carried = 0;
    for (const auto& t : w.transfers()) {
        const auto& r = t.request;
        if (t.result.status != TransferStatus::Accepted || t.event_sequence > boundary ||
            t.event_sequence <= st.begin_event || r.command.value < st.order.value ||
            (st.order_end.value && r.command.value >= st.order_end.value) ||
            (r.source != st.cargo && r.destination != st.cargo))
            continue;
        check(r.actor == st.executor, "story cargo transferred by another actor");
        if (r.destination == st.cargo) {
            check(r.source == st.source && r.quantity.value <= st.quantity - carried,
                  "story cargo did not come from its source");
            carried += r.quantity.value;
            continue;
        }
        check(r.quantity.value <= carried &&
                  (r.destination == st.source || r.destination == st.destination),
              "story transfer lacks bound pickup");
        carried -= r.quantity.value;
        if (r.destination == st.destination) {
            check(r.quantity.value <= st.quantity - delivered, "story delivery exceeds goal");
            delivered += r.quantity.value;
            if (delivered == st.quantity)
                return t.event_sequence;
        }
    }
    return 0;
}
EntityId meal_after(const WorkWorld& w, const StoryState& st, std::uint64_t boundary) {
    const auto delivered = delivery_event(w, st, boundary);
    if (!delivered)
        return {};
    std::map<EntityId, std::uint64_t> starts;
    for (const auto& e : w.events()) {
        if (e.sequence > boundary)
            break;
        if (e.sequence <= delivered)
            continue;
        if (e.kind == EventKind::Started)
            starts[e.subject] = e.sequence;
        if (e.kind != EventKind::Completed || !starts.contains(e.subject))
            continue;
        const auto a = w.action(e.subject);
        if (a && a->state == ActionState::Completed && a->request.actor == st.recipient &&
            a->request.lot == st.destination && a->consumed.value > 0)
            return a->id;
    }
    return {};
}
} // namespace
const std::optional<StoryState>& TraversalSession::story() const noexcept { return story_; }
void TraversalSession::init_story() {
    if (!scene_.story)
        return;
    const auto& seed = *scene_.story;
    StoryState st;
    st.executor = actors_.at(seed.executor).id;
    st.speaker = actors_.at(seed.speaker).id;
    st.recipient = actors_.at(seed.recipient).id;
    st.source = scene_.stations.at(seed.source).lot;
    st.destination = scene_.stations.at(seed.destination).lot;
    st.site = seed.site;
    st.meeting = seed.meeting;
    st.site_context = scene_.geometry.body(st.site);
    story_ = st;
}
StoryReply TraversalSession::begin_story(std::size_t index) {
    if (!story_)
        return StoryReply::Unavailable;
    const auto& actor = actors_.at(index);
    if (actor.id != story_->executor)
        return StoryReply::Unavailable;
    if (story_->stage != StoryStage::Dormant)
        return story_->stage == StoryStage::Accepted ? StoryReply::Accepted : StoryReply::Ended;
    // This local gate precedes all structural role checks and source reads.
    if (actor.support.surface != story_->meeting.surface ||
        !interaction_accessible(index, story_->meeting, actor.feet))
        return StoryReply::OutOfReach;
    if (protected_operation(actor) || actor.task != TraversalTask::None ||
        actor.motion.speed > .01 || actor.goal || actor.pending_goal ||
        actor.transfer != TransferStage::None || inputs_[index] != GroundPoint{} ||
        queued_inputs_[index] != GroundPoint{} ||
        (actor.cargo.value && world_.resource(actor.cargo)->record.quantity.value))
        return StoryReply::Busy;
    auto candidate = *this;
    auto& st = *candidate.story_;
    st.started_at = world_.clock().tick_count();
    check(st.started_at <= UINT64_MAX - 600, "story horizon overflow");
    st.begin_event = cursor(world_);
    st.request_feet = actor.feet;
    st.request_surface = actor.support.surface;
    auto end = [&](StoryStage stage) {
        st.stage = stage;
        st.ended_at = st.started_at;
        st.end_event = st.begin_event;
    };
    const auto speaker = person(candidate, st.speaker);
    if (!speaker)
        end(StoryStage::MissingSpeaker);
    else if (!body(geometry_, st.site))
        end(StoryStage::MissingSite);
    else {
        const auto source = station(candidate, st.source),
                   destination = station(candidate, st.destination);
        check(source && destination, "story station unavailable");
        if (!candidate.knowledge_contact(index, *speaker) ||
            !candidate.interaction_accessible(index, candidate.scene_.stations[*source].location,
                                              actor.feet))
            return StoryReply::OutOfReach;
        candidate.observe_station(index, *source);
        st.observed = true;
        st.available = candidate.actors_[index].beliefs[*source]->available;
        st.speaker_feet = candidate.actors_[*speaker].feet;
        st.speaker_surface = candidate.actors_[*speaker].support.surface;
        if (st.available < st.quantity)
            end(StoryStage::MissingSupply);
        else {
            st.stage = StoryStage::Accepted;
            check(candidate.next_command_.value < UINT64_MAX, "story command overflow");
            st.order = candidate.next_command_;
            candidate.assign_delivery(index, static_cast<std::uint32_t>(*source),
                                      static_cast<std::uint32_t>(*destination), st.quantity);
            st.cargo = candidate.actors_[index].cargo;
            ++candidate.next_command_.value; // Reserve a unique assignment marker.
        }
    }
    candidate.validate();
    *this = std::move(candidate);
    return story_->stage == StoryStage::Accepted ? StoryReply::Accepted : StoryReply::Ended;
}
void TraversalSession::update_story() {
    if (!story_ || story_->stage != StoryStage::Accepted)
        return;
    auto& st = *story_;
    const auto now = world_.clock().tick_count(), last = cursor(world_);
    const auto meal = meal_after(world_, st, last);
    if (meal.value) {
        const auto delivered = delivery_event(world_, st, last);
        for (const auto& transfer : world_.transfers())
            if (transfer.event_sequence == delivered)
                st.order_end = {transfer.request.command.value + 1};
        st.stage = StoryStage::Completed;
        st.meal = meal;
        st.ended_at = now;
        st.end_event = last;
        return;
    }
    const auto index = person(*this, st.executor);
    const auto& job = index ? actors_[*index].delivery : std::optional<DeliveryState>{};
    if (!job || job->started_command != st.order || job->phase == DeliveryPhase::Cancelled ||
        now - st.started_at >= 600) {
        st.stage = StoryStage::Failed;
        st.ended_at = now;
        st.end_event = last;
        if (index) {
            cancel_delivery(*index);
            if (actors_[*index].delivery)
                st.order_end = actors_[*index].delivery->ended_command;
        }
    }
}
bool TraversalSession::remove_actor(EntityId id) {
    if (world_.clock().tick_count() != 0 || actors_.size() <= 1 || practice_ ||
        !property_rules_.empty() || !incidents_.empty() || !scene_.services.empty() ||
        !scene_.passages.empty() || !scene_.distant_connections.empty() ||
        !scene_.initial_deliveries.empty() ||
        (story_ && (story_->stage != StoryStage::Dormant || id != story_->speaker)))
        return false;
    const auto index = person(*this, id);
    if (!index || *index == 0)
        return false;
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        const auto& a = actors_[i];
        if (!a.local() || protected_operation(a) || a.action.value || a.goal || a.pending_goal ||
            a.motion.speed > .01 || inputs_[i] != GroundPoint{} ||
            queued_inputs_[i] != GroundPoint{})
            return false;
        if (a.id == id && (a.cargo.value || a.delivery || !a.testimonies.empty()))
            return false;
        for (const auto& b : a.beliefs)
            if (b && (b->source_actor == id || b->via_actor == id))
                return false;
        for (const auto& b : a.passage_beliefs)
            if (b && (b->source_actor == id || b->via_actor == id))
                return false;
    }
    auto c = *this;
    check(c.world_.remove_actor(id), "actor removal failed");
    const auto offset = static_cast<std::ptrdiff_t>(*index);
    const auto count = c.actors_.size();
    c.actors_.erase(c.actors_.begin() + offset);
    c.inputs_.erase(c.inputs_.begin() + offset);
    c.queued_inputs_.erase(c.queued_inputs_.begin() + offset);
    if (c.scene_.spawns.size() == count)
        c.scene_.spawns.erase(c.scene_.spawns.begin() + offset);
    c.synchronize_presentation();
    c.validate();
    *this = std::move(c);
    return true;
}
bool TraversalSession::remove_static_body(std::uint32_t id) {
    if (world_.clock().tick_count() != 0 || !body(scene_.geometry, id) || practice_ ||
        (story_ && (story_->stage != StoryStage::Dormant || id == story_->meeting.surface)))
        return false;
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        const auto& a = actors_[i];
        if (a.support.surface == id || a.goal || a.pending_goal || a.action.value ||
            protected_operation(a) || a.motion.speed > .01 || inputs_[i] != GroundPoint{} ||
            queued_inputs_[i] != GroundPoint{})
            return false;
    }
    for (const auto& s : scene_.stations)
        if (s.location.surface == id)
            return false;
    for (const auto& c : scene_.connections)
        if (c.from.surface == id || c.to.surface == id)
            return false;
    for (const auto& p : scene_.platforms)
        if (p.definition.body == id || p.definition.foundation == id)
            return false;
    for (const auto& p : scene_.passages)
        if (p.surface == id)
            return false;
    for (const auto& t : scene_.services)
        if (t.platform == id || t.banks[0].surface == id || t.banks[1].surface == id)
            return false;
    for (const auto& c : scene_.distant_connections)
        if (c.terminals[0].surface == id || c.terminals[1].surface == id)
            return false;
    auto c = *this;
    check(c.scene_.geometry.version < UINT64_MAX, "geometry version overflow");
    std::erase_if(c.scene_.geometry.bodies, [&](const auto& b) { return b.id == id; });
    ++c.scene_.geometry.version;
    c.rebuild_geometry();
    c.synchronize_presentation();
    c.validate();
    *this = std::move(c);
    return true;
}
void TraversalSession::validate_story() const {
    if (!story_)
        return;
    const auto& st = *story_;
    const auto executor = person(*this, st.executor), recipient = person(*this, st.recipient);
    const auto source = station(*this, st.source), destination = station(*this, st.destination);
    const auto known_speaker =
        std::any_of(world_.events().begin(), world_.events().end(), [&](const auto& event) {
            return event.subject == st.speaker && event.kind == EventKind::ActorCreated;
        });
    check(executor && recipient && known_speaker && source && destination &&
              st.executor != st.speaker && st.executor != st.recipient &&
              st.speaker != st.recipient && st.source != st.destination && st.quantity == 4 &&
              st.site != 0 && st.site != st.meeting.surface &&
              static_cast<unsigned>(st.stage) <= static_cast<unsigned>(StoryStage::Failed),
          "invalid story bindings");
    const auto from = world_.resource(st.source), to = world_.resource(st.destination);
    check(from && to && from->record.active && to->record.active &&
              from->record.kind == ResourceKind::Material &&
              to->record.kind == ResourceKind::Material &&
              from->record.material == MaterialTypeId{1} &&
              scene_.stations[*source].kind == TraversalTask::Eat &&
              scene_.stations[*destination].kind == TraversalTask::Eat &&
              from->record.material == to->record.material,
          "invalid story material");
    for (const auto& platform : scene_.platforms)
        check(platform.definition.body != st.site &&
                  platform.definition.body != st.meeting.surface &&
                  platform.definition.body != scene_.stations[*source].location.surface &&
                  platform.definition.body != scene_.stations[*destination].location.surface,
              "story requires static role locations");
    st.site_context.validate();
    check(st.site_context.id == st.site, "story site identity lost its binding context");
    if (body(geometry_, st.site)) {
        const auto& actual = geometry_.body(st.site);
        const auto& original = st.site_context;
        check(actual.origin == original.origin && actual.yaw == original.yaw &&
                  actual.half_x == original.half_x && actual.half_z == original.half_z &&
                  actual.depth == original.depth && actual.slope_x == original.slope_x &&
                  actual.slope_z == original.slope_z && actual.walkable == original.walkable,
              "story site context differs from geometry");
    }
    const auto& actor = actors_[*executor];
    const auto anchor = surface_feet(geometry_, st.meeting, TraversalLimits{actor.size});
    check(anchor.has_value(), "invalid story meeting anchor");
    if (st.stage == StoryStage::Dormant) {
        check(st.started_at == 0 && st.ended_at == 0 && st.begin_event == 0 && st.end_event == 0 &&
                  !st.order.value && !st.order_end.value && !st.cargo.value && !st.meal.value &&
                  st.request_surface == 0 && st.speaker_surface == 0 &&
                  st.request_feet == SpatialPoint{} && st.speaker_feet == SpatialPoint{} &&
                  !st.observed && st.available == 0,
              "dormant story has execution");
        return;
    }
    const auto now = world_.clock().tick_count(), last = cursor(world_);
    check(st.started_at <= now && st.started_at <= UINT64_MAX - 600 && st.begin_event > 0 &&
              st.begin_event <= last && st.request_surface == st.meeting.surface &&
              length(st.request_feet - *anchor) <= 1.1 &&
              spatial_segment_clear(geometry_, st.request_feet, *anchor, actor.size),
          "invalid story start");
    check(world_.events()[static_cast<std::size_t>(st.begin_event - 1)].tick <= st.started_at,
          "story start observes future domain events");
    if (st.stage == StoryStage::Accepted)
        check(st.ended_at == 0 && st.end_event == 0 && !st.order_end.value && !st.meal.value &&
                  now - st.started_at < 600,
              "invalid active story horizon");
    else
        check(st.ended_at >= st.started_at && st.ended_at <= now &&
                  st.end_event >= st.begin_event && st.end_event <= last,
              "invalid story ending");
    if (st.stage != StoryStage::Accepted)
        check(world_.events()[static_cast<std::size_t>(st.end_event - 1)].tick <= st.ended_at,
              "story ending observes future domain events");
    if (st.stage == StoryStage::MissingSpeaker || st.stage == StoryStage::MissingSite) {
        check(!st.observed && st.available == 0 && !st.order.value && !st.order_end.value &&
                  !st.cargo.value && !st.meal.value && st.speaker_surface == 0 &&
                  st.speaker_feet == SpatialPoint{} && st.ended_at == st.started_at &&
                  st.end_event == st.begin_event,
              "structural refusal has execution");
        check(st.stage == StoryStage::MissingSpeaker
                  ? !person(*this, st.speaker)
                  : person(*this, st.speaker).has_value() && !body(geometry_, st.site),
              "story refusal contradicts world");
        return;
    }
    check(st.observed && st.speaker_surface == st.request_surface && body(geometry_, st.site) &&
              person(*this, st.speaker) && length(st.request_feet - st.speaker_feet) <= 2 &&
              spatial_segment_clear(geometry_, st.request_feet, st.speaker_feet, actor.size) &&
              spatial_segment_clear(geometry_, st.speaker_feet, st.request_feet,
                                    actors_[*person(*this, st.speaker)].size),
          "invalid story contact");
    const auto point =
        surface_feet(geometry_, scene_.stations[*source].location, TraversalLimits{actor.size});
    check(point && length(*point - st.request_feet) <= 1.1 &&
              spatial_segment_clear(geometry_, st.request_feet, *point, actor.size) &&
              st.available == available_at(world_, st.source, st.begin_event),
          "invalid historical supply observation");
    if (st.stage == StoryStage::MissingSupply) {
        check(st.available < 4 && !st.order.value && !st.order_end.value && !st.cargo.value &&
                  !st.meal.value && st.ended_at == st.started_at && st.end_event == st.begin_event,
              "invalid supply refusal");
        return;
    }
    const auto bag = world_.resource(st.cargo);
    check(st.available >= 4 && st.order.value > 0 && st.order.value < next_command_.value && bag &&
              bag->record.active && bag->record.owner == st.executor &&
              bag->record.material == from->record.material,
          "invalid story order");
    for (const auto& command : world_.commands())
        check(command.request.command != st.order, "story marker reused by action command");
    for (const auto& transfer : world_.transfers())
        check(transfer.request.command != st.order, "story marker reused by transfer command");
    if (st.stage != StoryStage::Accepted)
        check(st.order_end.value > st.order.value && st.order_end.value <= next_command_.value,
              "invalid story order interval");
    const auto& job = actor.delivery;
    check(job && job->started_command == st.order && job->source == *source &&
              job->destination == *destination && job->quantity == st.quantity &&
              actor.cargo == st.cargo,
          "story lost its original delivery record");
    if (st.stage == StoryStage::Failed)
        check(st.ended_at - st.started_at <= 600 && job->ended_command == st.order_end &&
                  (job->phase == DeliveryPhase::Cancelled ||
                   (job->phase == DeliveryPhase::Completed && st.ended_at - st.started_at == 600)),
              "failed story lacks ended delivery");
    else
        check(job->phase != DeliveryPhase::Cancelled,
              "live or completed story has cancelled order");
    if (st.stage == StoryStage::Completed)
        check(job->delivered == st.quantity && st.ended_at - st.started_at <= 600,
              "story success lacks timely original delivery");
    const auto boundary = st.stage == StoryStage::Accepted ? last : st.end_event;
    const auto meal = meal_after(world_, st, boundary);
    if (st.stage == StoryStage::Completed) {
        const auto delivered = delivery_event(world_, st, boundary);
        for (const auto& transfer : world_.transfers())
            if (transfer.event_sequence == delivered)
                check(st.order_end.value == transfer.request.command.value + 1,
                      "story success has wrong order boundary");
        const auto action = world_.action(st.meal);
        check(meal.value && meal == st.meal && action && action->ended_at <= st.ended_at,
              "story success without delivery and meal");
    } else
        check(!meal.value && !st.meal.value, "unfinished story has finished evidence");
}
} // namespace reality
