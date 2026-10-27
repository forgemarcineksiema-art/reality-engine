#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace reality {

inline constexpr std::size_t record_limit = 4096;

struct EntityId {
    std::uint64_t value = 0;
    auto operator<=>(const EntityId&) const = default;
};

enum class EntityKind : std::uint64_t { Actor = 1, Resource = 2, Action = 3 };

struct EntityHandle {
    std::uint32_t slot = 0;
    std::uint64_t generation = 0;
    auto operator<=>(const EntityHandle&) const = default;
};

struct EntitySlot {
    EntityId id;
    EntityKind kind = EntityKind::Actor;
    std::uint64_t generation = 1;
    bool occupied = true;
};

class EntityRegistry final {
  public:
    [[nodiscard]] EntityId create(EntityKind kind);
    bool destroy(EntityId id) noexcept;
    [[nodiscard]] std::optional<EntityHandle> handle(EntityId id) const noexcept;
    [[nodiscard]] std::optional<EntityId> resolve(EntityHandle handle) const noexcept;
    [[nodiscard]] bool contains(EntityId id, EntityKind kind) const noexcept;
    [[nodiscard]] std::uint64_t next_id() const noexcept;
    [[nodiscard]] const std::vector<EntitySlot>& slots() const noexcept;
    void validate() const;
    [[nodiscard]] static EntityRegistry restore(std::uint64_t next_id,
                                                std::vector<EntitySlot> slots);

  private:
    std::uint64_t next_id_ = 1;
    std::vector<EntitySlot> slots_;
};

} // namespace reality
