#pragma once

#include "core/entity_registry.hpp"
#include "core/simulation_clock.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace reality {

struct CommandId {
    std::uint64_t value = 0;
    auto operator<=>(const CommandId&) const = default;
};
struct Quantity {
    std::uint64_t value = 0;
    auto operator<=>(const Quantity&) const = default;
};
struct MaterialTypeId {
    std::uint64_t value = 0;
    auto operator<=>(const MaterialTypeId&) const = default;
};
struct TransferRequest {
    CommandId command;
    EntityId actor;
    EntityId source;
    EntityId destination;
    Quantity quantity;
    bool operator==(const TransferRequest&) const = default;
};
enum class TransferStatus : std::uint64_t {
    Accepted,
    InvalidRequest,
    ActorUnavailable,
    ResourceUnavailable,
    InsufficientResource,
    IncompatibleMaterial,
    Overflow
};
struct TransferResult {
    TransferStatus status = TransferStatus::InvalidRequest;
    bool operator==(const TransferResult&) const = default;
};
struct TransferRecord {
    TransferRequest request;
    TransferResult result;
    std::uint64_t tick = 0;
    std::uint64_t event_sequence = 0;
};
struct WorkRequest {
    CommandId command;
    EntityId actor;
    EntityId lot;
    Quantity required;
    Quantity consume_on_start;
    std::uint64_t duration_ticks = 0;
    bool operator==(const WorkRequest&) const = default;
};
enum class RequestStatus : std::uint64_t {
    Accepted,
    InvalidRequest,
    ActorUnavailable,
    ResourceUnavailable,
    InsufficientResource
};
struct RequestResult {
    RequestStatus status = RequestStatus::InvalidRequest;
    EntityId action;
    bool operator==(const RequestResult&) const = default;
};
enum class ActionState : std::uint64_t { Reserved, Running, Completed, Cancelled };
enum class CancelReason : std::uint64_t { None, Requested, ActorRemoved, ResourceRemoved };
enum class Transition { Changed, Unchanged, Missing, InvalidState, TimeOverflow };
struct ActorRecord {
    EntityId id;
    bool alive = false;
};
enum class ResourceKind : std::uint64_t { Material, Capacity };
struct ResourceRecord {
    EntityId id;
    EntityId owner;
    Quantity initial;
    Quantity quantity;
    Quantity lost;
    bool active = false;
    ResourceKind kind = ResourceKind::Material;
    MaterialTypeId material;
};
struct ActionRecord {
    EntityId id;
    WorkRequest request;
    Quantity consumed;
    ActionState state = ActionState::Reserved;
    std::uint64_t started_at = 0;
    std::uint64_t due_at = 0;
    std::uint64_t ended_at = 0;
    CancelReason reason = CancelReason::None;
};
struct CommandRecord {
    WorkRequest request;
    RequestResult result;
};
enum class EventKind : std::uint64_t {
    ActorCreated,
    ActorRemoved,
    ResourceCreated,
    ResourceRemoved,
    Reserved,
    Started,
    Consumed,
    Completed,
    Cancelled,
    TransferOut,
    TransferIn
};
struct WorldEvent {
    std::uint64_t sequence = 0;
    std::uint64_t tick = 0;
    EventKind kind = EventKind::ActorCreated;
    EntityId subject;
    Quantity quantity;
};
struct ResourceView {
    ResourceRecord record;
    Quantity reserved;
    Quantity available;
};

class WorkWorld final {
  public:
    explicit WorkWorld(std::chrono::nanoseconds step = std::chrono::milliseconds{50});
    [[nodiscard]] EntityId create_actor();
    [[nodiscard]] EntityId create_resource(EntityId owner, Quantity quantity,
                                           ResourceKind kind = ResourceKind::Material,
                                           MaterialTypeId material = {});
    [[nodiscard]] TransferResult transfer(const TransferRequest& request);
    bool remove_actor(EntityId actor);
    bool remove_resource(EntityId lot);
    [[nodiscard]] RequestResult reserve(const WorkRequest& request);
    [[nodiscard]] Transition begin(EntityId action);
    [[nodiscard]] Transition cancel(EntityId action);
    void advance(std::uint64_t ticks);

    [[nodiscard]] std::optional<ResourceView> resource(EntityId id) const;
    [[nodiscard]] std::optional<ActionRecord> action(EntityId id) const;
    [[nodiscard]] const EntityRegistry& entities() const noexcept;
    [[nodiscard]] const SimulationClock& clock() const noexcept;
    [[nodiscard]] const std::vector<WorldEvent>& events() const noexcept;
    [[nodiscard]] const std::vector<CommandRecord>& commands() const noexcept;
    [[nodiscard]] const std::vector<TransferRecord>& transfers() const noexcept;
    [[nodiscard]] std::string inspect() const;
    void validate() const;

  private:
    EntityRegistry entities_;
    SimulationClock clock_;
    std::vector<ActorRecord> actors_;
    std::vector<ResourceRecord> resources_;
    std::vector<ActionRecord> actions_;
    std::vector<CommandRecord> commands_;
    std::vector<WorldEvent> events_;
    std::vector<TransferRecord> transfers_;
    void validate_material_flow() const;

    void emit(EventKind kind, EntityId subject, Quantity quantity, std::uint64_t tick);
    void cancel_active(ActionRecord& action, CancelReason reason);
    friend std::string serialize_world(const WorkWorld& world);
    friend WorkWorld deserialize_world(std::string_view bytes);
};

[[nodiscard]] std::string_view describe(ActionState state);
[[nodiscard]] std::string_view describe(RequestStatus status);
[[nodiscard]] std::string_view describe(CancelReason reason);
[[nodiscard]] std::string_view describe(EventKind kind);

} // namespace reality
