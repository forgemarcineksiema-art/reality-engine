#pragma once
#include "lab/traversal_practice.hpp"

#include "navigation/surface_navigation.hpp"
#include "persistence/world_snapshot.hpp"
#include "physics/locomotion.hpp"
#include "physics/platform_motion.hpp"

namespace reality {
enum class TraversalTask { None, Eat, Rest, Maintain, Deliver };
enum class DeliveryPhase { ToPickup, ToDestination, Returning, Completed, Cancelled };
enum class CargoStatus {
    Transferred,
    OutOfReach,
    Busy,
    Empty,
    Incompatible,
    Unavailable,
    Denied,
    Returned,
    Blocked
};
struct DeliveryState {
    std::uint32_t source = 0, destination = 0;
    std::uint64_t quantity = 0, delivered = 0;
    DeliveryPhase phase = DeliveryPhase::ToPickup;
    CommandId started_command, ended_command;
};
enum class TraversalState { Idle, Moving, Arrived, Waiting, NoPath, InvalidGoal, BudgetExceeded };
enum class TransferStage { None, Waiting, Boarding, Riding, Exiting };
struct TraversalStation {
    SurfaceLocation location;
    TraversalTask kind = TraversalTask::Eat;
    std::uint64_t initial_stock = 50;
    EntityId lot;
    std::uint64_t wear = 500;
    std::uint64_t completed = 0;
    MaterialTypeId initial_material;
};
struct TransportService {
    std::uint32_t id = 0;
    std::uint32_t platform = 0;
    std::array<SurfaceLocation, 2> banks;
    std::array<GroundPoint, 2> deck_ports;
    EntityId owner;
    std::vector<EntityId> queue;
};
struct PlatformState {
    PlatformDefinition definition;
    std::uint64_t phase = 0;
    bool blocked = false;
};
struct PassageState {
    std::uint32_t id = 0;
    std::uint32_t surface = 0;
    EntityId owner;
    std::vector<EntityId> queue;
    bool open = true;
    std::optional<std::uint32_t> maintenance;
};
enum class PassageOutcome {
    Changed,
    Unchanged,
    Controlled,
    Busy,
    Occupied,
    OutOfReach,
    Worn,
    Unavailable
};
struct SuspendedTraversalTask {
    TraversalTask task = TraversalTask::None;
    std::uint32_t station = 0;
};
enum class ClearanceOutcome {
    None,
    Accepted,
    Controlled,
    Busy,
    NoSpace,
    Completed,
    Expired,
    Cancelled
};
[[nodiscard]] const char* clearance_label(ClearanceOutcome outcome);
struct ClearanceReply {
    EntityId peer;
    ClearanceOutcome outcome = ClearanceOutcome::None;
    std::uint64_t at = 0;
    std::uint64_t retry_at = 0;
};
struct ClearanceMove {
    EntityId requester;
    std::uint64_t requested_at = 0;
    std::uint64_t expires_at = 0;
    SurfaceLocation from;
    SurfaceLocation to;
    std::optional<SurfaceLocation> original_goal;
};
// Historical free quantity and wear, never a live view of WorkWorld.
struct StationBelief {
    std::uint64_t available = 0, wear = 0;
    std::uint64_t observed_tick = 0, learned_tick = 0;
    EntityId source_actor, via_actor;
};
struct PassageBelief {
    bool open = true;
    std::uint64_t observed_tick = 0, learned_tick = 0;
    EntityId source_actor, via_actor;
};
enum class KnowledgeOutcome { Shared, NoNews, OutOfReach, Blocked };
struct PropertyRule {
    std::uint32_t station = 0;
    EntityId custodian;
    std::vector<EntityId> residents;
    EntityId courier;
};
struct PropertyIncident {
    CommandId command;
    EntityId offender, lot, cargo;
    std::uint32_t station = 0;
    std::uint64_t quantity = 0, tick = 0, returned = 0;
    CommandId resolved_command;
};
struct ViolationTestimony {
    CommandId incident;
    EntityId offender, lot;
    std::uint32_t station = 0;
    std::uint64_t quantity = 0;
    bool resolved = false;
    std::uint64_t observed_tick = 0, learned_tick = 0;
    EntityId source_actor, via_actor;
};
struct TraversalActor {
    EntityId id;
    SurfaceLocation support;
    SpatialPoint feet;
    LocomotionState motion;
    CapsuleSize size;
    std::optional<SurfaceLocation> goal;
    std::optional<SurfaceLocation> requested_goal;
    double requested_reach = 0;
    std::optional<SurfaceLocation> pending_goal;
    double pending_reach = 0;
    bool stop_after_transfer = false;
    SurfaceRoute route;
    std::size_t leg = 0;
    std::size_t step_point = 0;
    TraversalState state = TraversalState::Idle;
    bool autonomous = true;
    TraversalTask task = TraversalTask::None;
    std::uint32_t station = 0;
    std::uint32_t known_stations = 0;
    std::uint64_t hunger = 0;
    std::uint64_t energy = 1000;
    std::uint64_t retry_at = 0;
    std::uint64_t blocked_ticks = 0;
    std::uint64_t plans = 0;
    std::array<std::uint64_t, 16> station_retry{};
    EntityId action;
    TransferStage transfer = TransferStage::None;
    std::size_t transfer_point = 0;
    int boarding_side = 0;
    std::vector<SuspendedTraversalTask> suspended;
    std::uint64_t need_retry_at = 0;
    std::uint64_t finish_grace_until = 0;
    ClearanceReply clearance;
    std::optional<ClearanceMove> yielding;
    EntityId cargo;
    std::optional<DeliveryState> delivery;
    std::array<std::optional<StationBelief>, 16> beliefs{};
    std::uint64_t share_until = 0, share_retry_at = 0;
    std::array<std::optional<PassageBelief>, 16> passage_beliefs{};
    std::uint32_t planned_closed_passages = 0;
    std::vector<ViolationTestimony> testimonies;
};
struct TraversalScene {
    SpatialGeometry geometry;
    std::vector<SurfaceConnection> connections;
    std::vector<PlatformState> platforms;
    std::vector<TransportService> services;
    std::vector<PassageState> passages;
    std::vector<TraversalStation> stations;
    struct Spawn {
        SurfaceLocation location;
        CapsuleSize size;
        bool autonomous = false;
        std::uint64_t hunger = 0;
        std::uint64_t energy = 1000;
        std::optional<std::uint32_t> known_stations;
    };
    std::vector<Spawn> spawns;
    struct DeliverySeed {
        std::size_t actor = 0;
        std::uint32_t source = 0, destination = 0;
        std::uint64_t quantity = 0;
    };
    std::vector<DeliverySeed> initial_deliveries;
    struct PropertySeed {
        std::uint32_t station = 0;
        std::size_t custodian = 0;
        std::vector<std::size_t> residents;
        std::optional<std::size_t> courier;
    };
    std::vector<PropertySeed> property_rules;
    std::optional<PracticeSeed> practice;
};
[[nodiscard]] TraversalScene make_traversal_course();
[[nodiscard]] TraversalScene make_delivery_course();
[[nodiscard]] TraversalScene make_crossing_course();
[[nodiscard]] TraversalScene make_law_course();
[[nodiscard]] TraversalScene make_community_course();
[[nodiscard]] TraversalScene make_learning_course();
struct TraversalPose {
    SpatialPoint feet;
    double heading = 0;
};
enum class TraversalGuidanceKind { None, Walk, Parking, Precision, Riding, OccupiedGoal, Blocked };
struct TraversalNavigationView {
    TraversalGuidanceKind kind = TraversalGuidanceKind::None;
    std::optional<SpatialPoint> target;
    std::uint32_t resource = 0;
    bool transport = false;
    EntityId blocker;
    std::size_t queue_position = 0;
    std::uint64_t progress = 0;
    std::uint64_t stall_window = 100;
    bool externally_blocked = false;
};

class TraversalSession final {
  public:
    [[nodiscard]] bool has_practice() const noexcept;
    [[nodiscard]] DiscoveryOutcome share_practice(std::size_t sender, std::size_t recipient);
    [[nodiscard]] PracticeView practice_view(std::size_t actor) const;
    [[nodiscard]] DiscoveryOutcome practice_probe(std::size_t actor, DiscoveryChain chain,
                                                  unsigned input);
    explicit TraversalSession(TraversalScene scene = make_traversal_course());
    [[nodiscard]] const WorkWorld& world() const noexcept;
    [[nodiscard]] const SpatialGeometry& geometry() const noexcept;
    [[nodiscard]] const std::vector<TraversalActor>& actors() const noexcept;
    [[nodiscard]] const std::vector<PlatformState>& platforms() const noexcept;
    [[nodiscard]] const std::vector<TraversalStation>& stations() const noexcept;
    [[nodiscard]] const std::vector<PassageState>& passages() const noexcept;
    [[nodiscard]] PassageOutcome interact_passage(std::size_t actor, std::uint32_t passage,
                                                  bool open);
    [[nodiscard]] bool observe_passage(std::size_t actor, std::uint32_t passage);
    [[nodiscard]] bool begin_maintenance(std::size_t actor, std::size_t station);
    [[nodiscard]] TraversalNavigationView navigation_view(std::size_t actor) const;
    [[nodiscard]] std::vector<TraversalPose> presentation(double fraction) const;
    [[nodiscard]] SpatialGeometry presentation_geometry(double fraction) const;
    void synchronize_presentation();
    void set_goal(std::size_t actor, SurfaceLocation location);
    void approach(std::size_t actor, SurfaceLocation location, double reach = 1);
    void set_autonomous(std::size_t actor, bool enabled);
    void set_input(std::size_t actor, GroundPoint direction);
    void queue_input(std::size_t actor, GroundPoint direction);
    void clear_input() noexcept;
    void stop(std::size_t actor);
    [[nodiscard]] ClearanceOutcome request_clearance(std::size_t actor);
    void assign_delivery(std::size_t actor, std::uint32_t source, std::uint32_t destination,
                         std::uint64_t quantity);
    void cancel_delivery(std::size_t actor);
    [[nodiscard]] CargoStatus interact_cargo(std::size_t actor, std::size_t station);
    [[nodiscard]] KnowledgeOutcome share_knowledge(std::size_t sender, std::size_t recipient);
    [[nodiscard]] bool can_interact_at(std::size_t actor, SurfaceLocation location) const;
    [[nodiscard]] bool can_exchange_knowledge(std::size_t sender, std::size_t recipient) const;
    [[nodiscard]] const std::vector<PropertyIncident>& incidents() const noexcept;
    [[nodiscard]] bool legal_service_allowed(std::size_t actor, std::size_t station) const;
    void advance(std::uint64_t ticks);
    [[nodiscard]] std::string serialize() const;
    void restore(std::string_view bytes);
    void save(const std::filesystem::path& path) const;
    void load(const std::filesystem::path& path);
    [[nodiscard]] std::string inspect() const;
    void validate() const;

  private:
    std::optional<PracticeState> practice_;
    [[nodiscard]] bool practice_busy(std::size_t actor) const;
    [[nodiscard]] bool practice_at_bench(std::size_t actor) const;
    [[nodiscard]] bool practice_allows_work(std::size_t actor, std::size_t station) const;
    void practice_work_started(std::size_t actor, EntityId action);
    void advance_practice();
    void validate_practice() const;
    void request_practice_work(std::size_t actor);
    WorkWorld world_;
    std::vector<PropertyRule> property_rules_;
    std::vector<PropertyIncident> incidents_;
    TraversalScene scene_;
    SpatialGeometry geometry_;
    std::vector<TraversalActor> actors_;
    std::vector<GroundPoint> inputs_;
    std::vector<GroundPoint> queued_inputs_;
    std::vector<std::vector<SpatialSample>> history_;
    std::vector<double> previous_headings_;
    SpatialGeometry previous_geometry_;
    std::vector<std::uint64_t> previous_phases_;
    std::vector<std::uint32_t> previous_supports_;
    std::vector<SpatialPoint> substep_starts_;
    std::vector<SpatialPoint> substep_carries_;
    std::vector<double> substep_padding_;
    std::vector<bool> processed_;
    SpatialGeometry substep_geometry_;
    unsigned substep_ = 0;
    CommandId next_command_{1};
    LocomotionTuning tuning_;
    void tick();
    void rebuild_geometry();
    [[nodiscard]] std::vector<SurfaceBlocker> stationary_neighbours(std::size_t actor) const;
    void plan(std::size_t actor, bool avoid_neighbours = false);
    void decide(std::size_t actor);
    [[nodiscard]] bool select_task(std::size_t actor, TraversalTask kind,
                                   std::optional<std::size_t> preferred = {});
    [[nodiscard]] bool interrupt_for_need(std::size_t actor);
    void release_navigation(std::size_t actor);
    void update_clearance();
    void cancel_yield(std::size_t actor);
    void finish_yield(std::size_t actor, ClearanceOutcome outcome);
    void validate_clearance() const;
    [[nodiscard]] bool cargo_returning(const TraversalActor& actor) const noexcept;
    [[nodiscard]] std::optional<std::size_t>
    choose_return_station(const TraversalActor& actor) const;
    [[nodiscard]] bool plan_delivery(std::size_t actor);
    void arrive_delivery(std::size_t actor);
    void validate_delivery() const;
    void observe_station(std::size_t actor, std::size_t station);
    void update_knowledge();
    void validate_knowledge() const;
    [[nodiscard]] bool knowledge_contact(std::size_t from, std::size_t to) const;
    [[nodiscard]] bool authorized_pickup(std::size_t actor, std::size_t station) const;
    [[nodiscard]] CargoStatus interact_cargo_impl(std::size_t actor, std::size_t station);
    void record_law_transfer(std::size_t actor, std::size_t station, bool loading,
                             std::uint64_t quantity, CommandId command);
    void observe_violation(const PropertyIncident& incident, bool resolved);
    void validate_law() const;
    [[nodiscard]] bool protected_operation(const TraversalActor& actor) const;
    [[nodiscard]] std::uint32_t closed_passage_mask(const TraversalActor& actor) const;
    [[nodiscard]] std::vector<SurfaceConnection>
    known_connections(const TraversalActor& actor) const;
    [[nodiscard]] bool passage_busy(const PassageState& passage) const;
    [[nodiscard]] bool passage_contact(std::size_t actor, const PassageState& passage) const;
    [[nodiscard]] bool passage_motion_clear(SpatialPoint from, SpatialPoint to,
                                            CapsuleSize size) const;
    void update_passages();
    void repair_passages(std::size_t actor, std::size_t station);
    void change_passage(std::size_t passage, bool open);
    void validate_passages() const;
    [[nodiscard]] bool start_station_action(std::size_t actor);
    [[nodiscard]] bool belief_unavailable(const TraversalActor& actor, std::size_t station) const;
    void reconsider_belief(std::size_t actor);
    [[nodiscard]] bool approach_point_free(std::size_t actor, SpatialPoint feet) const;
    [[nodiscard]] bool interaction_accessible(std::size_t actor, SurfaceLocation location,
                                              SpatialPoint feet) const;
    [[nodiscard]] bool choose_approach(std::size_t actor, SurfaceLocation location, double reach,
                                       bool physical_access = false);
    void retry_approach(std::size_t actor);
    [[nodiscard]] SpatialPoint target(TraversalActor& actor);
    void progress(TraversalActor& actor);
    void move(std::size_t actor, double dt, const SpatialGeometry& end_geometry);
};
} // namespace reality
