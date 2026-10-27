#include "lab/lab_session.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace reality {

LabSession::LabSession() {
    actors_[0] = world_.create_actor();
    actors_[1] = world_.create_actor();
    lot_ = world_.create_resource(actors_[0], Quantity{3});
}

const WorkWorld& LabSession::world() const noexcept { return world_; }
const std::array<EntityId, 2>& LabSession::actors() const noexcept { return actors_; }
EntityId LabSession::lot() const noexcept { return lot_; }

std::optional<ActionRecord> LabSession::latest_action(std::size_t actor) const {
    const auto person = actors_.at(actor);
    std::optional<ActionRecord> latest;
    for (const auto& slot : world_.entities().slots()) {
        if (slot.occupied && slot.kind == EntityKind::Action) {
            const auto action = world_.action(slot.id).value();
            if (action.request.actor == person && (!latest || action.id > latest->id)) {
                latest = action;
            }
        }
    }
    return latest;
}

RequestResult LabSession::reserve(std::size_t actor) {
    const auto person = actors_.at(actor);
    std::uint64_t maximum = 0;
    for (const auto& command : world_.commands()) {
        maximum = std::max(maximum, command.request.command.value);
    }
    if (maximum == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("command id space exhausted");
    }
    return world_.reserve({CommandId{maximum + 1}, person, lot_, Quantity{3}, Quantity{1}, 100});
}

Transition LabSession::begin(std::size_t actor) {
    const auto action = latest_action(actor);
    return action ? world_.begin(action->id) : Transition::Missing;
}

Transition LabSession::cancel(std::size_t actor) {
    const auto action = latest_action(actor);
    return action ? world_.cancel(action->id) : Transition::Missing;
}

void LabSession::advance(std::uint64_t ticks) { world_.advance(ticks); }
void LabSession::save(const std::filesystem::path& path) const { save_world(world_, path); }

void LabSession::load(const std::filesystem::path& path) { restore(load_world(path)); }

void LabSession::restore(WorkWorld candidate) {
    std::vector<EntityId> actors;
    std::vector<EntityId> lots;
    for (const auto& slot : candidate.entities().slots()) {
        if (slot.occupied && slot.kind == EntityKind::Actor) {
            actors.push_back(slot.id);
        }
        if (slot.occupied && slot.kind == EntityKind::Resource) {
            lots.push_back(slot.id);
        }
    }
    if (actors.size() != 2 || lots.size() != 1) {
        throw std::invalid_argument(
            "laboratory needs exactly two live actors and one live resource lot");
    }
    std::sort(actors.begin(), actors.end());
    world_ = std::move(candidate);
    actors_ = {actors[0], actors[1]};
    lot_ = lots[0];
}

void SimulationPacer::set_running(bool running) noexcept { running_ = running; }
bool SimulationPacer::running() const noexcept { return running_; }
std::chrono::nanoseconds SimulationPacer::pending() const noexcept { return pending_; }
void SimulationPacer::reset() noexcept {
    running_ = false;
    pending_ = {};
}

} // namespace reality
