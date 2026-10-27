#include "core/entity_registry.hpp"

#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace reality {

EntityId EntityRegistry::create(EntityKind kind) {
    if (kind < EntityKind::Actor || kind > EntityKind::Action) {
        throw std::invalid_argument("invalid entity kind");
    }
    if (next_id_ == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("entity id space exhausted");
    }
    const EntityId id{next_id_};
    for (auto& slot : slots_) {
        if (!slot.occupied && slot.generation < std::numeric_limits<std::uint64_t>::max()) {
            slot.id = id;
            slot.kind = kind;
            slot.occupied = true;
            ++next_id_;
            return id;
        }
    }
    if (slots_.size() == record_limit) {
        throw std::length_error("entity slot limit reached");
    }
    slots_.push_back(EntitySlot{id, kind});
    ++next_id_;
    return id;
}

bool EntityRegistry::destroy(EntityId id) noexcept {
    for (auto& slot : slots_) {
        if (slot.occupied && slot.id == id) {
            slot.occupied = false;
            if (slot.generation != std::numeric_limits<std::uint64_t>::max()) {
                ++slot.generation;
            }
            return true;
        }
    }
    return false;
}

std::optional<EntityHandle> EntityRegistry::handle(EntityId id) const noexcept {
    for (std::size_t index = 0; index < slots_.size(); ++index) {
        if (slots_[index].occupied && slots_[index].id == id) {
            return EntityHandle{static_cast<std::uint32_t>(index), slots_[index].generation};
        }
    }
    return std::nullopt;
}

std::optional<EntityId> EntityRegistry::resolve(EntityHandle handle) const noexcept {
    if (handle.slot >= slots_.size()) {
        return std::nullopt;
    }
    const auto& slot = slots_[handle.slot];
    return slot.occupied && slot.generation == handle.generation ? std::optional<EntityId>{slot.id}
                                                                 : std::nullopt;
}

bool EntityRegistry::contains(EntityId id, EntityKind kind) const noexcept {
    const auto found = handle(id);
    return found && slots_[found->slot].kind == kind;
}

std::uint64_t EntityRegistry::next_id() const noexcept { return next_id_; }
const std::vector<EntitySlot>& EntityRegistry::slots() const noexcept { return slots_; }

void EntityRegistry::validate() const {
    if (next_id_ == 0 || slots_.size() > record_limit) {
        throw std::runtime_error("invalid entity registry bounds");
    }
    std::set<EntityId> ids;
    for (const auto& slot : slots_) {
        if (slot.id.value == 0 || slot.id.value >= next_id_ || slot.generation == 0 ||
            slot.kind < EntityKind::Actor || slot.kind > EntityKind::Action ||
            !ids.insert(slot.id).second) {
            throw std::runtime_error("invalid entity slot");
        }
    }
}

EntityRegistry EntityRegistry::restore(std::uint64_t next_id, std::vector<EntitySlot> slots) {
    EntityRegistry result;
    result.next_id_ = next_id;
    result.slots_ = std::move(slots);
    result.validate();
    return result;
}

} // namespace reality
