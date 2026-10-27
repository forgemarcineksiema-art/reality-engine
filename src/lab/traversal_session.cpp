#include "lab/traversal_session.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numbers>
#include <set>
#include <sstream>
#include <stdexcept>

namespace reality {
namespace {
constexpr double step_seconds = .05;
constexpr unsigned subdivisions = 5;
constexpr double arrival = .015;
void check(bool condition, const char* message) {
    if (!condition) {
        throw std::invalid_argument(message);
    }
}
TraversalLimits limits(const TraversalActor& actor) {
    TraversalLimits limits;
    limits.size = actor.size;
    return limits;
}
SurfaceLocation location(const SpatialGeometry& geometry, const TraversalActor& actor) {
    const auto local = geometry.body(actor.support.surface).local(actor.feet);
    return {actor.support.surface, {local.x, local.z}};
}
PlatformState* platform(TraversalScene& scene, std::uint32_t body) {
    const auto p = std::find_if(scene.platforms.begin(), scene.platforms.end(),
                                [&](const auto& p) { return p.definition.body == body; });
    return p == scene.platforms.end() ? nullptr : &*p;
}
TransportService& service(TraversalScene& scene, std::uint32_t id) {
    const auto p = std::find_if(scene.services.begin(), scene.services.end(),
                                [&](const auto& s) { return s.id == id; });
    if (p == scene.services.end()) {
        throw std::invalid_argument("unknown transport service");
    }
    return *p;
}
SpatialPoint feet_at(const SpatialGeometry& geometry, SurfaceLocation point,
                     const TraversalActor& actor) {
    const auto feet = surface_feet(geometry, point, limits(actor));
    if (!feet) {
        throw std::invalid_argument("surface target no longer usable");
    }
    return *feet;
}
void append(std::vector<SpatialSample>& history, double time, SpatialPoint feet) {
    if (history.size() >= 256) {
        throw std::overflow_error("traversal presentation history exhausted");
    }
    if (history.empty() || time > history.back().fraction) {
        history.push_back({time, feet});
    } else if (time == history.back().fraction) {
        history.back().feet = feet;
    }
}
SpatialPoint sample(const std::vector<SpatialSample>& history, double fraction) {
    for (std::size_t p = 1; p < history.size(); ++p) {
        if (fraction <= history[p].fraction) {
            const auto t = (fraction - history[p - 1].fraction) /
                           (history[p].fraction - history[p - 1].fraction);
            return history[p - 1].feet + (history[p].feet - history[p - 1].feet) * t;
        }
    }
    return history.back().feet;
}
std::vector<SpatialPoint> transfer_points(const SpatialGeometry& geometry,
                                          const TransportService& s, const TraversalActor& actor) {
    const auto side =
        actor.transfer == TransferStage::Boarding ? actor.boarding_side : 1 - actor.boarding_side;
    const auto bank = feet_at(geometry, s.banks.at(static_cast<std::size_t>(side)), actor);
    const auto port =
        feet_at(geometry, {s.platform, s.deck_ports.at(static_cast<std::size_t>(side))}, actor);
    const auto center = feet_at(geometry, {s.platform, {0, 0}}, actor);
    if (actor.transfer == TransferStage::Boarding) {
        auto points = step_points(bank, port);
        points.push_back(center);
        return points;
    }
    std::vector<SpatialPoint> points{port};
    const auto step = step_points(port, bank);
    points.insert(points.end(), step.begin() + 1, step.end());
    return points;
}
bool boarding_clear(const SpatialGeometry& geometry, const TransportService& s,
                    const TraversalActor& actor, std::span<const TraversalActor> people) {
    const auto side = static_cast<std::size_t>(actor.boarding_side);
    const auto bank = feet_at(geometry, s.banks[side], actor);
    const auto port = surface_feet(geometry, {s.platform, s.deck_ports[side]}, limits(actor));
    const auto center = surface_feet(geometry, {s.platform, {0, 0}}, limits(actor));
    if (!port || !center || !limits(actor).allows_step(bank, *port))
        return false;
    auto path = step_points(bank, *port);
    path.push_back(*center);
    for (std::size_t p = 1; p < path.size(); ++p) {
        if (!spatial_segment_clear(geometry, path[p - 1], path[p], actor.size))
            return false;
        for (const auto& other : people)
            if (other.id != actor.id && !capsule_paths_clear(path[p - 1], path[p], actor.size,
                                                             other.feet, other.feet, other.size))
                return false;
    }
    return true;
}
bool parking_free(SpatialPoint feet, const TraversalActor& actor,
                  std::span<const TraversalActor> people) {
    return std::all_of(people.begin(), people.end(), [&](const auto& other) {
        return other.id == actor.id ||
               capsule_paths_clear(feet, feet, actor.size, other.feet, other.feet, other.size);
    });
}
SpatialPoint queue_point(const SpatialGeometry& geometry, const TransportService& s,
                         const TraversalActor& actor, std::span<const TraversalActor> people) {
    const auto bank =
        feet_at(geometry, s.banks.at(static_cast<std::size_t>(actor.boarding_side)), actor);
    const auto& surface =
        geometry.body(s.banks.at(static_cast<std::size_t>(actor.boarding_side)).surface);
    const auto found = std::find(s.queue.begin(), s.queue.end(), actor.id);
    const auto position = static_cast<double>(std::distance(s.queue.begin(), found));
    auto direction = bank - geometry.body(s.platform).origin;
    direction.y = 0;
    direction = unit(direction);
    for (const auto lateral : {.85, -.85, 1.3, -1.3}) {
        const auto point = bank + direction * (.9 + .75 * position) +
                           SpatialPoint{-direction.z, 0, direction.x} * lateral;
        const auto local = surface.local(point);
        const auto supported =
            surface_feet(geometry, {surface.id, {local.x, local.z}}, limits(actor));
        if (supported && parking_free(*supported, actor, people))
            return *supported;
    }
    return actor.feet;
}
SpatialPoint passage_queue_point(const SpatialGeometry& geometry, const PassageState& passage,
                                 SurfaceLocation entry, const TraversalActor& actor,
                                 std::span<const TraversalActor> people) {
    const auto bank = feet_at(geometry, entry, actor);
    auto direction = bank - geometry.body(passage.surface).origin;
    direction.y = 0;
    direction = unit(direction);
    const auto found = std::find(passage.queue.begin(), passage.queue.end(), actor.id);
    const auto pos = static_cast<double>(std::distance(passage.queue.begin(), found));
    const auto& surface = geometry.body(entry.surface);
    for (const auto lateral : {.85, -.85, 1.3, -1.3}) {
        const auto point = bank + direction * (1. + .75 * pos) +
                           SpatialPoint{-direction.z, 0, direction.x} * lateral;
        const auto local = surface.local(point);
        const auto supported =
            surface_feet(geometry, {entry.surface, {local.x, local.z}}, limits(actor));
        if (supported && parking_free(*supported, actor, people))
            return *supported;
    }
    return actor.feet;
}
bool step_ready(const SpatialGeometry& geometry, const SurfaceLeg& leg,
                const TraversalActor& actor) {
    const auto from = surface_feet(geometry, leg.from, limits(actor)),
               to = surface_feet(geometry, leg.to, limits(actor));
    if (!from || !to || !limits(actor).allows_step(*from, *to)) {
        return false;
    }
    const auto path = step_points(*from, *to);
    for (std::size_t p = 1; p < path.size(); ++p) {
        if (!spatial_segment_clear(geometry, path[p - 1], path[p], actor.size)) {
            return false;
        }
    }
    return true;
}
} // namespace
TraversalSession::TraversalSession(TraversalScene scene)
    : scene_(std::move(scene)), geometry_(scene_.geometry) {
    scene_.geometry.validate();
    check(!scene_.spawns.empty() && scene_.spawns.size() <= 32 && scene_.stations.size() <= 16,
          "invalid traversal population");
    for (const auto& moving : scene_.platforms) {
        moving.definition.validate(scene_.geometry);
    }
    rebuild_geometry();
    for (std::size_t i = 0; i < scene_.spawns.size(); ++i) {
        TraversalActor actor;
        actor.id = world_.create_actor();
        const auto& seed = scene_.spawns[i];
        actor.support = seed.location;
        actor.size = seed.size;
        actor.feet = feet_at(geometry_, actor.support, actor);
        actor.known_stations = seed.known_stations.value_or(
            scene_.stations.empty() ? 0 : ((std::uint32_t{1} << scene_.stations.size()) - 1));
        actor.hunger = seed.hunger;
        actor.energy = seed.energy;
        actor.autonomous = seed.autonomous;
        actors_.push_back(actor);
    }
    for (std::size_t index = 0; index < scene_.stations.size(); ++index) {
        auto& station = scene_.stations[index];
        auto owner = actors_.front().id;
        for (const auto& rule : scene_.property_rules)
            if (rule.station == index)
                owner = actors_.at(rule.custodian).id;
        station.lot = world_.create_resource(
            owner, Quantity{station.initial_stock},
            station.kind == TraversalTask::Rest ? ResourceKind::Capacity : ResourceKind::Material,
            station.initial_material);
    }
    for (const auto& job : scene_.initial_deliveries)
        assign_delivery(job.actor, job.source, job.destination, job.quantity);
    for (const auto& seed : scene_.property_rules) {
        PropertyRule rule;
        rule.station = seed.station;
        rule.custodian = actors_.at(seed.custodian).id;
        for (const auto resident : seed.residents)
            rule.residents.push_back(actors_.at(resident).id);
        if (seed.courier)
            rule.courier = actors_.at(*seed.courier).id;
        property_rules_.push_back(std::move(rule));
    }
    inputs_.resize(actors_.size());
    queued_inputs_.resize(actors_.size());
    synchronize_presentation();
    validate();
}
const WorkWorld& TraversalSession::world() const noexcept { return world_; }
const SpatialGeometry& TraversalSession::geometry() const noexcept { return geometry_; }
const std::vector<TraversalActor>& TraversalSession::actors() const noexcept { return actors_; }
const std::vector<PlatformState>& TraversalSession::platforms() const noexcept {
    return scene_.platforms;
}
const std::vector<TraversalStation>& TraversalSession::stations() const noexcept {
    return scene_.stations;
}
void TraversalSession::rebuild_geometry() {
    geometry_ = scene_.geometry;
    for (const auto& moving : scene_.platforms) {
        const auto pose =
            platform_pose(scene_.geometry, moving.definition, static_cast<double>(moving.phase));
        for (auto& body : geometry_.bodies) {
            if (body.id == pose.id) {
                body = pose;
                break;
            }
        }
    }
}
void TraversalSession::synchronize_presentation() {
    history_.clear();
    previous_headings_.clear();
    previous_supports_.clear();
    previous_phases_.clear();
    for (const auto& actor : actors_) {
        history_.push_back({{0, actor.feet}, {1, actor.feet}});
        previous_headings_.push_back(actor.motion.heading);
        previous_supports_.push_back(actor.support.surface);
    }
    for (const auto& p : scene_.platforms) {
        previous_phases_.push_back(p.phase);
    }
    previous_geometry_ = geometry_;
}
SpatialGeometry TraversalSession::presentation_geometry(double fraction) const {
    check(std::isfinite(fraction) && fraction >= 0 && fraction <= 1,
          "invalid presentation fraction");
    auto geometry = scene_.geometry;
    for (std::size_t i = 0; i < scene_.platforms.size(); ++i) {
        const auto& p = scene_.platforms[i];
        auto current = p.phase;
        if (current < previous_phases_[i]) {
            current += p.definition.period();
        }
        auto phase = static_cast<double>(previous_phases_[i]) +
                     static_cast<double>(current - previous_phases_[i]) * fraction;
        phase = std::fmod(phase, static_cast<double>(p.definition.period()));
        const auto pose = platform_pose(scene_.geometry, p.definition, phase);
        for (auto& body : geometry.bodies) {
            if (body.id == pose.id) {
                body = pose;
                break;
            }
        }
    }
    return geometry;
}
std::vector<TraversalPose> TraversalSession::presentation(double fraction) const {
    const auto geometry = presentation_geometry(fraction);
    std::vector<TraversalPose> poses;
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        const auto& actor = actors_[i];
        auto feet = sample(history_[i], fraction);
        if (previous_supports_[i] == actor.support.surface) {
            const auto moving =
                std::find_if(scene_.platforms.begin(), scene_.platforms.end(), [&](const auto& p) {
                    return p.definition.body == actor.support.surface;
                });
            if (moving != scene_.platforms.end()) {
                // Interpolate the rider in the supporting body's frame, not through
                // the deck's curved/vertical motion in world space.
                std::size_t p = 1;
                while (p + 1 < history_[i].size() && fraction > history_[i][p].fraction) {
                    ++p;
                }
                const auto before_geometry = presentation_geometry(history_[i][p - 1].fraction);
                const auto after_geometry = presentation_geometry(history_[i][p].fraction);
                const auto a =
                    before_geometry.body(actor.support.surface).local(history_[i][p - 1].feet);
                const auto b =
                    after_geometry.body(actor.support.surface).local(history_[i][p].feet);
                const auto phase = (fraction - history_[i][p - 1].fraction) /
                                   (history_[i][p].fraction - history_[i][p - 1].fraction);
                const auto local = a + (b - a) * phase;
                const auto& deck = geometry.body(actor.support.surface);
                const auto point = deck.world({local.x, 0, local.z});
                const auto height = deck.support({point.x, point.z}, actor.size.radius,
                                                 limits(actor).maximum_slope);
                if (height) {
                    feet = deck.world(local);
                }
            }
        }
        poses.push_back(
            {feet, previous_headings_[i] +
                       angle_delta(previous_headings_[i], actor.motion.heading) * fraction});
    }
    return poses;
}
std::vector<SurfaceBlocker> TraversalSession::stationary_neighbours(std::size_t index) const {
    std::vector<SurfaceBlocker> result;
    for (std::size_t other = 0; other < actors_.size(); ++other) {
        if (other != index && actors_[other].motion.speed < .05 &&
            length(actors_[other].feet - actors_[index].feet) < 3.)
            result.push_back({actors_[other].feet, actors_[other].size});
    }
    return result;
}
void TraversalSession::plan(std::size_t index, bool avoid_neighbours) {
    auto& actor = actors_.at(index);
    if (!actor.goal) {
        return;
    }
    const auto blockers =
        avoid_neighbours ? stationary_neighbours(index) : std::vector<SurfaceBlocker>{};
    actor.planned_closed_passages = closed_passage_mask(actor);
    const auto connections = known_connections(actor);
    actor.route = find_surface_route(geometry_, connections, location(geometry_, actor),
                                     *actor.goal, limits(actor), 512, 32768, blockers);
    actor.leg = 0;
    actor.step_point = 0;
    ++actor.plans;
    switch (actor.route.status) {
    case RouteStatus::Found:
        actor.state = actor.route.legs.empty() ? TraversalState::Arrived : TraversalState::Moving;
        break;
    case RouteStatus::NoPath:
        actor.state = TraversalState::NoPath;
        break;
    case RouteStatus::InvalidEndpoint:
        actor.state = TraversalState::InvalidGoal;
        break;
    case RouteStatus::BudgetExceeded:
        actor.state = TraversalState::BudgetExceeded;
        break;
    }
    actor.retry_at = world_.clock().tick_count() + 40;
}
namespace {
bool committed_step(const TraversalActor& actor) {
    return actor.leg < actor.route.legs.size() &&
           actor.route.legs[actor.leg].kind == SurfaceLegKind::Step && actor.step_point > 0;
}
void cancel_waiting(TraversalScene& scene, TraversalActor& actor) {
    if (actor.transfer != TransferStage::Waiting) {
        return;
    }
    for (auto& s : scene.services) {
        s.queue.erase(std::remove(s.queue.begin(), s.queue.end(), actor.id), s.queue.end());
    }
    actor.transfer = TransferStage::None;
}
void clear_deferred(TraversalActor& actor) {
    actor.pending_goal.reset();
    actor.pending_reach = 0;
    actor.stop_after_transfer = false;
}
bool occupies_passage(const SpatialGeometry& geometry, std::uint32_t surface,
                      const TraversalActor& actor) {
    const auto& body = geometry.body(surface);
    const auto local = body.local(actor.feet);
    const auto dx = std::max(0.0, std::abs(local.x) - body.half_x);
    const auto dz = std::max(0.0, std::abs(local.z) - body.half_z);
    const auto height =
        body.origin.y + body.slope_x * local.x + body.slope_z * local.z +
        actor.size.radius *
            (std::sqrt(1 + body.slope_x * body.slope_x + body.slope_z * body.slope_z) - 1);
    return std::hypot(dx, dz) <= actor.size.radius + contact_margin_m &&
           std::abs(actor.feet.y - height) <= limits(actor).maximum_step + step_clearance_m;
}
void release_departed(TraversalScene& scene, const SpatialGeometry& geometry,
                      const TraversalActor& actor, bool preserve_entry) {
    for (auto& p : scene.passages) {
        bool entering = false;
        if (preserve_entry && actor.goal && actor.leg < actor.route.legs.size()) {
            const auto& leg = actor.route.legs[actor.leg];
            entering = leg.passage == p.id && leg.to.surface == p.surface;
            if (leg.kind == SurfaceLegKind::Walk && actor.leg + 1 < actor.route.legs.size()) {
                const auto& next = actor.route.legs[actor.leg + 1];
                entering = next.passage == p.id && next.to.surface == p.surface;
            }
        }
        if (p.owner == actor.id && !entering && !occupies_passage(geometry, p.surface, actor)) {
            p.owner = {};
            p.queue.erase(std::remove(p.queue.begin(), p.queue.end(), actor.id), p.queue.end());
        }
    }
}
void release_uncommitted(TraversalScene& scene, const SpatialGeometry& geometry,
                         const TraversalActor& actor) {
    release_departed(scene, geometry, actor, false);
    for (auto& s : scene.services) {
        if (s.owner != actor.id) {
            s.queue.erase(std::remove(s.queue.begin(), s.queue.end(), actor.id), s.queue.end());
        }
    }
    for (auto& p : scene.passages) {
        if (p.owner != actor.id) {
            p.queue.erase(std::remove(p.queue.begin(), p.queue.end(), actor.id), p.queue.end());
        }
    }
}
} // namespace
void TraversalSession::release_navigation(std::size_t index) {
    auto& actor = actors_.at(index);
    cancel_waiting(scene_, actor);
    release_uncommitted(scene_, geometry_, actor);
}
TraversalNavigationView TraversalSession::navigation_view(std::size_t index) const {
    const auto& actor = actors_.at(index);
    TraversalNavigationView result;
    if (!actor.goal || actor.route.status != RouteStatus::Found ||
        actor.leg >= actor.route.legs.size())
        return result;
    const auto& leg = actor.route.legs[actor.leg];
    result.kind = TraversalGuidanceKind::Walk;
    result.target = surface_feet(geometry_, leg.to, limits(actor));
    const auto* transport_leg = leg.kind == SurfaceLegKind::Transport ? &leg : nullptr;
    if (leg.kind == SurfaceLegKind::Walk && actor.leg + 1 < actor.route.legs.size() &&
        actor.route.legs[actor.leg + 1].kind == SurfaceLegKind::Transport)
        transport_leg = &actor.route.legs[actor.leg + 1];
    if (transport_leg) {
        const auto found =
            std::find_if(scene_.services.begin(), scene_.services.end(),
                         [&](const auto& s) { return s.id == transport_leg->transport; });
        check(found != scene_.services.end(), "missing observed transport");
        const auto& s = *found;
        const auto moving =
            std::find_if(scene_.platforms.begin(), scene_.platforms.end(),
                         [&](const auto& p) { return p.definition.body == s.platform; });
        check(moving != scene_.platforms.end(), "missing observed platform");
        result.resource = s.id;
        result.transport = true;
        result.blocker = s.owner == actor.id ? EntityId{} : s.owner;
        result.progress = moving->phase;
        result.stall_window =
            std::max(std::uint64_t{100}, std::uint64_t{moving->definition.dwell_ticks} + 20);
        result.externally_blocked = moving->blocked;
        const auto queued = std::find(s.queue.begin(), s.queue.end(), actor.id);
        if (queued != s.queue.end())
            result.queue_position = static_cast<std::size_t>(queued - s.queue.begin());
        if (actor.transfer == TransferStage::Boarding || actor.transfer == TransferStage::Exiting) {
            result.kind = TraversalGuidanceKind::Precision;
            result.target = transfer_points(geometry_, s, actor).at(actor.transfer_point);
            result.progress =
                static_cast<std::uint64_t>(actor.transfer) * 16 + actor.transfer_point;
            result.externally_blocked = false;
            return result;
        }
        if (actor.transfer == TransferStage::Riding) {
            result.kind = TraversalGuidanceKind::Riding;
            result.target = feet_at(geometry_, {s.platform, {0, 0}}, actor);
            return result;
        }
        if (queued != s.queue.end() &&
            (actor.transfer == TransferStage::Waiting || s.owner != actor.id)) {
            result.kind = TraversalGuidanceKind::Parking;
            result.target = queue_point(geometry_, s, actor, actors_);
            if (s.owner.value == 0 && s.queue.front() == actor.id &&
                platform_dock(moving->definition, moving->phase) == actor.boarding_side &&
                boarding_clear(geometry_, s, actor, actors_))
                result.target = feet_at(
                    geometry_, s.banks[static_cast<std::size_t>(actor.boarding_side)], actor);
            return result;
        }
        const auto target = result.target;
        result = {};
        result.kind = TraversalGuidanceKind::Walk;
        result.target = target;
    }
    const auto* entry = &leg;
    if (leg.kind == SurfaceLegKind::Walk && actor.leg + 1 < actor.route.legs.size()) {
        const auto& next = actor.route.legs[actor.leg + 1];
        if (next.passage != 0 && length(feet_at(geometry_, next.from, actor) - actor.feet) < 2.)
            entry = &next;
    }
    if (entry->passage != 0) {
        const auto passage = std::find_if(scene_.passages.begin(), scene_.passages.end(),
                                          [&](const auto& p) { return p.id == entry->passage; });
        check(passage != scene_.passages.end(), "missing observed passage");
        const auto queued = std::find(passage->queue.begin(), passage->queue.end(), actor.id);
        if (queued != passage->queue.end() && passage->owner != actor.id &&
            entry->to.surface == passage->surface && entry->from.surface != passage->surface) {
            result.kind = TraversalGuidanceKind::Parking;
            result.target = passage_queue_point(geometry_, *passage, entry->from, actor, actors_);
            result.resource = passage->id;
            result.blocker = passage->owner;
            result.queue_position = static_cast<std::size_t>(queued - passage->queue.begin());
            return result;
        }
    }
    if (leg.kind == SurfaceLegKind::Step) {
        if (actor.step_point == 0 && !step_ready(geometry_, leg, actor)) {
            result.kind = TraversalGuidanceKind::Blocked;
            result.externally_blocked = true;
            result.target = actor.feet;
            return result;
        }
        result.kind = TraversalGuidanceKind::Precision;
        result.target =
            step_points(feet_at(geometry_, leg.from, actor), feet_at(geometry_, leg.to, actor))
                .at(actor.step_point);
        result.progress = actor.leg * 16 + actor.step_point;
    }
    if (leg.kind == SurfaceLegKind::Walk && actor.leg + 1 == actor.route.legs.size() &&
        actor.requested_reach == 0 && result.target && length(*result.target - actor.feet) < 1.25) {
        for (const auto& other : actors_)
            if (other.id != actor.id &&
                !capsule_paths_clear(*result.target, *result.target, actor.size, other.feet,
                                     other.feet, other.size)) {
                result.kind = TraversalGuidanceKind::OccupiedGoal;
                result.blocker = other.id;
                break;
            }
    }
    return result;
}
void TraversalSession::set_goal(std::size_t index, SurfaceLocation goal) {
    auto& actor = actors_.at(index);
    check(surface_feet(geometry_, goal, limits(actor)).has_value(),
          "goal is not physically usable");
    cancel_yield(index);
    inputs_.at(index) = {};
    queued_inputs_.at(index) = {};
    actor.autonomous = false;
    actor.suspended.clear();
    actor.need_retry_at = 0;
    actor.finish_grace_until = 0;
    if (actor.goal == goal && actor.route.geometry_version == geometry_.version &&
        actor.state != TraversalState::NoPath) {
        actor.task = TraversalTask::None;
        clear_deferred(actor);
        actor.requested_goal = goal;
        actor.requested_reach = 0;
        return;
    }
    cancel_waiting(scene_, actor);
    if (actor.transfer != TransferStage::None ||
        (actor.leg < actor.route.legs.size() &&
         actor.route.legs[actor.leg].kind == SurfaceLegKind::Step && actor.step_point > 0)) {
        actor.pending_goal = goal;
        actor.pending_reach = 0;
        actor.requested_goal = goal;
        actor.requested_reach = 0;
        actor.stop_after_transfer = false;
        actor.autonomous = false;
        return;
    }
    clear_deferred(actor);
    release_uncommitted(scene_, geometry_, actor);
    if (actor.action.value != 0) {
        static_cast<void>(world_.cancel(actor.action));
        actor.action = {};
    }
    actor.requested_goal = goal;
    actor.requested_reach = 0;
    actor.goal = goal;
    actor.autonomous = false;
    actor.task = TraversalTask::None;
    inputs_.at(index) = {};
    plan(index);
}
void TraversalSession::stop(std::size_t index) {
    auto& actor = actors_.at(index);
    cancel_yield(index);
    actor.suspended.clear();
    actor.need_retry_at = 0;
    actor.finish_grace_until = 0;
    inputs_.at(index) = {};
    queued_inputs_.at(index) = {};
    actor.autonomous = false;
    cancel_waiting(scene_, actor);
    if (actor.transfer != TransferStage::None ||
        (actor.leg < actor.route.legs.size() &&
         actor.route.legs[actor.leg].kind == SurfaceLegKind::Step && actor.step_point > 0)) {
        actor.pending_goal.reset();
        actor.pending_reach = 0;
        actor.stop_after_transfer = true;
        return;
    }
    clear_deferred(actor);
    release_uncommitted(scene_, geometry_, actor);
    if (actor.action.value != 0) {
        static_cast<void>(world_.cancel(actor.action));
        actor.action = {};
    }
    actor.goal.reset();
    actor.route = {};
    actor.leg = 0;
    actor.step_point = 0;
    actor.state = TraversalState::Idle;
    actor.task = TraversalTask::None;
    inputs_.at(index) = {};
    actor.requested_goal.reset();
    actor.requested_reach = 0;
}
void TraversalSession::set_autonomous(std::size_t index, bool enabled) {
    if (actors_.at(index).autonomous == enabled) {
        if (enabled)
            actors_[index].stop_after_transfer = false;
        return;
    }
    stop(index);
    actors_[index].autonomous = enabled;
    if (enabled)
        actors_[index].stop_after_transfer = false;
    actors_[index].retry_at = world_.clock().tick_count();
}
void TraversalSession::set_input(std::size_t index, GroundPoint direction) {
    reality::validate(direction);
    const auto norm = std::hypot(direction.x_m, direction.z_m);
    if (norm > 1) {
        direction.x_m /= norm;
        direction.z_m /= norm;
    }
    auto& actor = actors_.at(index);
    if (actor.transfer != TransferStage::None && actor.transfer != TransferStage::Waiting) {
        return;
    }
    if (norm > 0) {
        stop(index);
        actor.autonomous = false;
    }
    inputs_.at(index) = direction;
}
void TraversalSession::clear_input() noexcept {
    for (auto& input : inputs_) {
        input = {};
    }
    for (auto& input : queued_inputs_) {
        input = {};
    }
}
void TraversalSession::queue_input(std::size_t index, GroundPoint direction) {
    if (actors_.at(index).transfer != TransferStage::None &&
        actors_[index].transfer != TransferStage::Waiting) {
        return;
    }
    set_input(index, direction);
    queued_inputs_.at(index) = inputs_[index];
    inputs_[index] = {};
}
bool TraversalSession::approach_point_free(std::size_t index, SpatialPoint feet) const {
    const auto& actor = actors_.at(index);
    for (std::size_t other = 0; other < actors_.size(); ++other) {
        if (other == index)
            continue;
        const auto& person = actors_[other];
        if (!capsule_paths_clear(feet, feet, actor.size, person.feet, person.feet, person.size))
            return false;
        if (person.goal) {
            const auto target = surface_feet(geometry_, *person.goal, limits(person));
            if (target &&
                !capsule_paths_clear(feet, feet, actor.size, *target, *target, person.size))
                return false;
        }
    }
    return true;
}
bool TraversalSession::interaction_accessible(std::size_t index, SurfaceLocation destination,
                                              SpatialPoint feet) const {
    const auto& actor = actors_.at(index);
    const auto target = surface_feet(geometry_, destination, limits(actor));
    return target && length(*target - feet) <= 1.1 &&
           spatial_segment_clear(geometry_, feet, *target, actor.size);
}
bool TraversalSession::choose_approach(std::size_t index, SurfaceLocation destination, double reach,
                                       bool physical_access) {
    auto& actor = actors_.at(index);
    const auto& surface = geometry_.body(destination.surface);
    std::vector<SurfaceLocation> candidates{destination};
    for (unsigned direction = 0; direction < 12; ++direction) {
        const auto angle = 2 * std::numbers::pi * static_cast<double>(direction) / 12;
        candidates.push_back({destination.surface,
                              {destination.local.x_m + reach * .8 * std::cos(angle),
                               destination.local.z_m + reach * .8 * std::sin(angle)}});
    }
    std::stable_sort(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
        const auto pa = surface.world({a.local.x_m, 0, a.local.z_m});
        const auto pb = surface.world({b.local.x_m, 0, b.local.z_m});
        return length(pa - actor.feet) < length(pb - actor.feet);
    });
    unsigned searches = 0;
    for (const auto candidate : candidates) {
        const auto feet = surface_feet(geometry_, candidate, limits(actor));
        if (!feet) {
            continue;
        }
        if ((physical_access && !interaction_accessible(index, destination, *feet)) ||
            !approach_point_free(index, *feet)) {
            continue;
        }
        if (searches++ == 4) {
            break;
        }
        const auto blockers = stationary_neighbours(index);
        const auto connections = known_connections(actor);
        auto route = find_surface_route(geometry_, connections, location(geometry_, actor),
                                        candidate, limits(actor), 512, 32768, blockers);
        ++actor.plans;
        if (route.status != RouteStatus::Found) {
            continue;
        }
        actor.requested_goal = destination;
        actor.requested_reach = reach;
        actor.goal = candidate;
        actor.route = std::move(route);
        actor.planned_closed_passages = closed_passage_mask(actor);
        actor.leg = 0;
        actor.step_point = 0;
        actor.state = actor.route.legs.empty() ? TraversalState::Arrived : TraversalState::Moving;
        return true;
    }
    return false;
}
void TraversalSession::retry_approach(std::size_t index) {
    auto& actor = actors_.at(index);
    const auto physical = actor.task != TraversalTask::None;
    if (choose_approach(index, *actor.requested_goal, actor.requested_reach, physical))
        return;
    const auto old =
        actor.goal ? surface_feet(geometry_, *actor.goal, limits(actor)) : std::nullopt;
    if (physical && (!old || !interaction_accessible(index, *actor.requested_goal, *old))) {
        actor.route = {};
        actor.leg = 0;
        actor.step_point = 0;
        actor.state = TraversalState::NoPath;
    } else if (actor.goal) {
        plan(index, true);
    }
}
void TraversalSession::approach(std::size_t index, SurfaceLocation destination, double reach) {
    check(std::isfinite(reach) && reach > 0 && reach <= 2, "invalid approach reach");
    auto& actor = actors_.at(index);
    reality::validate(destination.local);
    const auto& body = geometry_.body(destination.surface);
    const auto anchor = body.world({destination.local.x_m, 0, destination.local.z_m});
    check(body.support({anchor.x, anchor.z}, 0, limits(actor).maximum_slope).has_value(),
          "invalid approach anchor");
    cancel_yield(index);
    inputs_[index] = {};
    queued_inputs_[index] = {};
    actor.autonomous = false;
    actor.suspended.clear();
    actor.need_retry_at = 0;
    actor.finish_grace_until = 0;
    if (actor.pending_goal == destination && actor.pending_reach == reach &&
        (actor.transfer != TransferStage::None || committed_step(actor))) {
        actor.stop_after_transfer = false;
        return;
    }
    const auto prior_goal =
        actor.goal ? surface_feet(geometry_, *actor.goal, limits(actor)) : std::nullopt;
    if (actor.requested_goal == destination && actor.requested_reach == reach && actor.goal &&
        actor.route.status == RouteStatus::Found &&
        actor.route.geometry_version == geometry_.version && !actor.pending_goal && prior_goal &&
        approach_point_free(index, *prior_goal)) {
        clear_deferred(actor);
        actor.task = TraversalTask::None;
        return;
    }
    cancel_waiting(scene_, actor);
    if (actor.transfer != TransferStage::None || committed_step(actor)) {
        actor.pending_goal = destination;
        actor.pending_reach = reach;
        actor.requested_goal = destination;
        actor.requested_reach = reach;
        actor.stop_after_transfer = false;
        actor.autonomous = false;
        return;
    }
    stop(index);
    actors_[index].autonomous = false;
    if (!choose_approach(index, destination, reach)) {
        actors_[index].requested_goal = destination;
        actors_[index].requested_reach = reach;
        actors_[index].state = TraversalState::NoPath;
        actors_[index].retry_at = world_.clock().tick_count() + 40;
    }
}
SpatialPoint TraversalSession::target(TraversalActor& actor) {
    if (actor.leg >= actor.route.legs.size()) {
        return actor.feet;
    }
    const auto& leg = actor.route.legs[actor.leg];
    if (leg.kind == SurfaceLegKind::Walk && actor.leg + 1 < actor.route.legs.size()) {
        const auto& next = actor.route.legs[actor.leg + 1];
        if (next.kind == SurfaceLegKind::Transport) {
            auto& s = service(scene_, next.transport);
            const auto queued =
                std::find(s.queue.begin(), s.queue.end(), actor.id) != s.queue.end();
            const auto bank = feet_at(geometry_, next.from, actor);
            if (queued || length(bank - actor.feet) < 2.) {
                actor.boarding_side = s.banks[0] == next.from ? 0 : 1;
                if (!queued)
                    s.queue.push_back(actor.id);
                if (s.owner.value != 0 || s.queue.front() != actor.id) {
                    actor.state = TraversalState::Waiting;
                    return queue_point(geometry_, s, actor, actors_);
                }
            }
        }
    }
    if (leg.kind == SurfaceLegKind::Walk && actor.leg + 1 == actor.route.legs.size() &&
        actor.requested_reach == 0) {
        const auto goal = feet_at(geometry_, leg.to, actor);
        if (length(goal - actor.feet) < 1.25) {
            for (const auto& other : actors_) {
                if (other.id != actor.id && !capsule_paths_clear(goal, goal, actor.size, other.feet,
                                                                 other.feet, other.size)) {
                    actor.state = TraversalState::Waiting;
                    return actor.feet;
                }
            }
        }
    }
    const auto* entry = &leg;
    if (leg.kind == SurfaceLegKind::Walk && actor.leg + 1 < actor.route.legs.size()) {
        const auto& next = actor.route.legs[actor.leg + 1];
        if (next.passage != 0 && length(feet_at(geometry_, next.from, actor) - actor.feet) < 2.)
            entry = &next;
    }
    if (entry->passage != 0) {
        const auto found = std::find_if(scene_.passages.begin(), scene_.passages.end(),
                                        [&](const auto& p) { return p.id == entry->passage; });
        check(found != scene_.passages.end(), "missing passage state");
        if (!found->open) {
            const auto observed =
                observe_passage(static_cast<std::size_t>(&actor - actors_.data()), found->id);
            if (!observed && leg.kind == SurfaceLegKind::Walk)
                return feet_at(geometry_, leg.to, actor);
            actor.state = TraversalState::Waiting;
            return actor.feet;
        }
        if (entry->to.surface == found->surface && entry->from.surface != found->surface) {
            if (std::find(found->queue.begin(), found->queue.end(), actor.id) ==
                found->queue.end()) {
                found->queue.push_back(actor.id);
            }
            if (found->owner.value == 0) {
                for (const auto& other : actors_) {
                    if (occupies_passage(geometry_, found->surface, other)) {
                        found->owner = other.id;
                        if (std::find(found->queue.begin(), found->queue.end(), other.id) ==
                            found->queue.end())
                            found->queue.insert(found->queue.begin(), other.id);
                        break;
                    }
                }
            }
            if (found->owner.value == 0 && !found->queue.empty() &&
                found->queue.front() == actor.id) {
                found->owner = actor.id;
            }
            if (found->owner != actor.id) {
                actor.state = TraversalState::Waiting;
                return passage_queue_point(geometry_, *found, entry->from, actor, actors_);
            }
        }
    }
    if (leg.kind == SurfaceLegKind::Transport) {
        auto& s = service(scene_, leg.transport);
        auto* moving = platform(scene_, s.platform);
        check(moving != nullptr, "missing service platform");
        if (actor.transfer == TransferStage::None) {
            actor.boarding_side = s.banks[0] == leg.from ? 0 : 1;
            actor.transfer = TransferStage::Waiting;
            if (std::find(s.queue.begin(), s.queue.end(), actor.id) == s.queue.end()) {
                s.queue.push_back(actor.id);
            }
        }
        if (actor.transfer == TransferStage::Waiting) {
            const auto waiting = queue_point(geometry_, s, actor, actors_);
            if (s.owner.value == 0 && !s.queue.empty() && s.queue.front() == actor.id &&
                platform_dock(moving->definition, moving->phase) == actor.boarding_side) {
                const auto bank = feet_at(
                    geometry_, s.banks[static_cast<std::size_t>(actor.boarding_side)], actor);
                const auto free = boarding_clear(geometry_, s, actor, actors_);
                if (free) {
                    if (length(bank - actor.feet) > arrival || actor.motion.speed >= .03) {
                        actor.state = TraversalState::Waiting;
                        return bank;
                    }
                    s.owner = actor.id;
                    actor.transfer = TransferStage::Boarding;
                    actor.transfer_point = 0;
                } else {
                    actor.state = TraversalState::Waiting;
                    return waiting;
                }
            } else {
                actor.state = TraversalState::Waiting;
                return waiting;
            }
        }
        if (actor.transfer == TransferStage::Riding) {
            if (platform_dock(moving->definition, moving->phase) == 1 - actor.boarding_side) {
                actor.transfer = TransferStage::Exiting;
                actor.transfer_point = 0;
            } else {
                actor.state = TraversalState::Waiting;
                return feet_at(geometry_, {s.platform, {0, 0}}, actor);
            }
        }
        actor.state = TraversalState::Moving;
        return transfer_points(geometry_, s, actor).at(actor.transfer_point);
    }
    actor.state = TraversalState::Moving;
    if (leg.kind == SurfaceLegKind::Step) {
        if (actor.step_point == 0 && !step_ready(geometry_, leg, actor)) {
            actor.state = TraversalState::Waiting;
            return actor.feet;
        }
        const auto points =
            step_points(feet_at(geometry_, leg.from, actor), feet_at(geometry_, leg.to, actor));
        return points.at(actor.step_point);
    }
    return feet_at(geometry_, leg.to, actor);
}
void TraversalSession::progress(TraversalActor& actor) {
    for (unsigned skip = 0; skip < 4 && actor.leg < actor.route.legs.size(); ++skip) {
        const auto desired = target(actor);
        const auto exact = actor.route.legs[actor.leg].kind != SurfaceLegKind::Walk;
        if (actor.state == TraversalState::Waiting ||
            length(desired - actor.feet) > (exact ? 1e-6 : arrival) ||
            actor.motion.speed > (exact ? .001 : .03)) {
            return;
        }
        const auto leg = actor.route.legs[actor.leg];
        if (leg.kind == SurfaceLegKind::Transport) {
            auto& s = service(scene_, leg.transport);
            const auto points = transfer_points(geometry_, s, actor);
            if (actor.transfer == TransferStage::Boarding &&
                actor.transfer_point + 2 == points.size()) {
                const auto local = geometry_.body(s.platform).local(actor.feet);
                actor.support = {s.platform, {local.x, local.z}};
            }
            if (++actor.transfer_point < points.size()) {
                return;
            }
            actor.transfer_point = 0;
            if (actor.transfer == TransferStage::Boarding) {
                actor.transfer = TransferStage::Riding;
                actor.state = TraversalState::Waiting;
                return;
            }
            actor.transfer = TransferStage::None;
            s.owner = {};
            s.queue.erase(std::remove(s.queue.begin(), s.queue.end(), actor.id), s.queue.end());
        } else if (leg.kind == SurfaceLegKind::Step) {
            const auto points =
                step_points(feet_at(geometry_, leg.from, actor), feet_at(geometry_, leg.to, actor));
            if (++actor.step_point < points.size()) {
                return;
            }
        }
        const auto local = geometry_.body(leg.to.surface).local(actor.feet);
        actor.support = {leg.to.surface, {local.x, local.z}};
        ++actor.leg;
        actor.step_point = 0;
        release_departed(scene_, geometry_, actor, true);
        if (actor.pending_goal || actor.stop_after_transfer) {
            const auto pending = actor.pending_goal;
            const auto pending_reach = actor.pending_reach;
            clear_deferred(actor);
            actor.task = TraversalTask::None;
            if (pending) {
                actor.goal = pending;
                actor.requested_goal = pending;
                actor.requested_reach = 0;
            } else {
                actor.goal.reset();
                actor.route = {};
                actor.leg = 0;
                actor.state = TraversalState::Idle;
                return;
            }
            const auto index = static_cast<std::size_t>(&actor - actors_.data());
            if (pending_reach > 0) {
                actor.goal.reset();
                actor.route = {};
                actor.leg = 0;
                actor.requested_reach = pending_reach;
                if (!choose_approach(index, *pending, pending_reach)) {
                    actor.state = TraversalState::NoPath;
                    actor.retry_at = world_.clock().tick_count() + 40;
                }
            } else {
                plan(index);
            }
            return;
        }
    }
    if (actor.goal && actor.leg == actor.route.legs.size()) {
        actor.state = TraversalState::Arrived;
    }
}
void TraversalSession::move(std::size_t index, double dt, const SpatialGeometry& end_geometry) {
    auto& actor = actors_[index];
    const auto old_feet = actor.feet;
    auto carried = old_feet;
    std::uint32_t carrier = 0;
    double padding = 0;
    if (platform(scene_, actor.support.surface) && !committed_step(actor) &&
        actor.transfer != TransferStage::Boarding && actor.transfer != TransferStage::Exiting) {
        const auto& old = substep_geometry_.body(actor.support.surface);
        const auto& now = geometry_.body(actor.support.surface);
        if (old.support({old_feet.x, old_feet.z}, actor.size.radius, limits(actor).maximum_slope)) {
            carried = platform_anchor(old, now, old_feet, actor.size, limits(actor).maximum_slope);
            const auto angle = angle_delta(old.yaw, now.yaw);
            actor.motion.heading =
                std::remainder(actor.motion.heading + angle, 2 * std::numbers::pi);
            const auto anchor = old.local(old_feet);
            padding = std::hypot(anchor.x, anchor.z) * (1 - std::cos(angle * .5));
            carrier = actor.support.surface;
        }
    }
    padding = std::max(padding, substep_padding_[index]);
    actor.feet = carried;
    const auto input = inputs_[index];
    const auto committed =
        actor.transfer != TransferStage::None ||
        (actor.leg < actor.route.legs.size() &&
         actor.route.legs[actor.leg].kind == SurfaceLegKind::Step && actor.step_point > 0);
    const auto manual = !committed && std::hypot(input.x_m, input.z_m) > 0;
    auto desired = carried;
    bool stepping = false;
    if (manual) {
        desired = carried + SpatialPoint{input.x_m, 0, input.z_m} * 3;
    } else if (actor.goal && actor.route.status == RouteStatus::Found &&
               actor.leg < actor.route.legs.size()) {
        desired = target(actor);
        const auto kind = actor.route.legs[actor.leg].kind;
        stepping = (kind == SurfaceLegKind::Step && actor.state != TraversalState::Waiting) ||
                   actor.transfer == TransferStage::Boarding ||
                   actor.transfer == TransferStage::Exiting;
    }
    auto direction = desired - carried;
    const auto distance = length(direction);
    const auto wants_movement = distance > (stepping ? 1e-6 : arrival);
    auto heading = wants_movement && std::hypot(direction.x, direction.z) > 1e-8
                       ? std::atan2(direction.x, direction.z)
                       : actor.motion.heading;
    auto wanted_speed = wants_movement ? tuning_.maximum_speed : 0;
    if (actor.energy < 150) {
        wanted_speed *= .65;
    }
    // A short-horizon velocity search is only steering advice. Continuous pair
    // collision checks below remain authoritative for actual movement.
    const auto clear_surface_segment = [&](SpatialPoint point) {
        const auto supported = continuous_surface_feet(geometry_, actor.support.surface, carried,
                                                       {point.x, point.z}, limits(actor));
        if (!supported)
            return false;
        // Reject a candidate blocked by a person before the swept surface search.
        for (std::size_t other = 0; other < actors_.size(); ++other) {
            if (other != index &&
                !capsule_paths_clear(carried, *supported, actor.size, actors_[other].feet,
                                     actors_[other].feet, actors_[other].size))
                return false;
        }
        return spatial_segment_clear(geometry_, carried, *supported, actor.size);
    };
    bool steering_blocked = false;
    if (wants_movement && !stepping && !manual) {
        auto best = std::numeric_limits<double>::infinity();
        double chosen = heading;
        for (const auto offset : {0.0, .35, -.35, .7, -.7, 1.15, -1.15, 1.55, -1.55, 1.9, -1.9}) {
            // Scores are nonnegative. A clear straight path at full horizon is optimal.
            if (best == 0)
                break;
            for (const auto lookahead : {std::max(.7, actor.motion.speed * 1.1), .35}) {
                if (best == 0)
                    break;
                const auto angle = heading + offset;
                const auto horizon = std::min(distance, lookahead);
                auto future = carried + SpatialPoint{std::sin(angle), 0, std::cos(angle)} * horizon;
                const auto supported = continuous_surface_feet(
                    geometry_, actor.support.surface, carried, {future.x, future.z}, limits(actor));
                if (!supported) {
                    continue;
                }
                future = *supported;
                if (!spatial_segment_clear(geometry_, carried, future, actor.size)) {
                    continue;
                }
                bool free = true;
                for (std::size_t other = 0; other < actors_.size(); ++other) {
                    if (other == index) {
                        continue;
                    }
                    const auto& person = actors_[other];
                    auto projected = person.feet + SpatialPoint{std::sin(person.motion.heading), 0,
                                                                std::cos(person.motion.heading)} *
                                                       (person.motion.speed * .8);
                    projected.y = person.feet.y;
                    if (!capsule_paths_clear(carried, future, actor.size, person.feet, projected,
                                             person.size)) {
                        free = false;
                        break;
                    }
                }
                if (!free) {
                    continue;
                }
                const auto score =
                    std::abs(offset) + (offset < 0 ? .025 : 0) + (lookahead == .35 ? .1 : 0);
                if (score < best) {
                    best = score;
                    chosen = angle;
                }
            }
        }
        if (!std::isfinite(best)) {
            wanted_speed = 0;
            steering_blocked = true;
        } else {
            heading = chosen;
        }
    }
    auto stop_distance =
        manual ? std::optional<double>{}
               : std::optional<double>{std::max(0.0, distance - (stepping ? 0.0 : arrival * .25))};
    if (manual) {
        const auto forward = SpatialPoint{std::sin(heading), 0, std::cos(heading)};
        double low = 0, high = std::max(1., braking_distance(actor.motion.speed,
                                                             actor.motion.acceleration, tuning_) +
                                                .2);
        if (!clear_surface_segment(carried + forward * high)) {
            for (unsigned i = 0; i < 24; ++i) {
                const auto mid = (low + high) * .5;
                if (clear_surface_segment(carried + forward * mid))
                    low = mid;
                else
                    high = mid;
            }
            stop_distance = std::max(0., low - 2 * contact_margin_m);
        }
    }
    if (!stepping && wants_movement && std::abs(angle_delta(actor.motion.heading, heading)) > .2 &&
        !clear_surface_segment(
            carried +
            SpatialPoint{std::sin(actor.motion.heading), 0, std::cos(actor.motion.heading)} * .05))
        wanted_speed = 0;
    if (!manual && !stepping && distance < .5) {
        wanted_speed = std::min(wanted_speed, distance * 3);
        if (std::abs(angle_delta(actor.motion.heading, heading)) > .35) {
            wanted_speed = 0;
        }
    }
    const auto motion =
        advance_locomotion(actor.motion, {heading, wanted_speed, stop_distance}, tuning_, dt);
    SpatialPoint own_delta;
    if (stepping) {
        own_delta = unit(direction) * motion.distance;
    } else {
        const auto mid =
            actor.motion.heading + angle_delta(actor.motion.heading, motion.state.heading) * .5;
        const auto normal = geometry_.body(actor.support.surface).normal();
        const auto x = std::sin(mid), z = std::cos(mid);
        const auto rise = -(normal.x * x + normal.z * z) / normal.y;
        own_delta = unit(SpatialPoint{x, rise, z}) * motion.distance;
    }
    auto destination = carried + own_delta;
    SurfaceLocation support = actor.support;
    if (!stepping) {
        const auto supported =
            continuous_surface_feet(geometry_, actor.support.surface, carried,
                                    {destination.x, destination.z}, limits(actor));
        std::optional<double> selected_height;
        std::uint32_t selected = 0;
        for (const auto& body : geometry_.bodies) {
            const auto height = body.support({destination.x, destination.z}, actor.size.radius,
                                             limits(actor).maximum_slope);
            if (!supported || !height || std::abs(*height - supported->y) > 1e-8) {
                continue;
            }
            if (!selected_height || *height > *selected_height) {
                selected_height = *height;
                selected = body.id;
            }
        }
        if (selected_height) {
            destination.y = *selected_height;
            const auto local = geometry_.body(selected).local(destination);
            support = {selected, {local.x, local.z}};
        } else {
            destination = carried;
        }
    }
    const auto unsupported =
        !stepping && motion.distance > 1e-6 && length(destination - carried) < motion.distance * .1;
    auto inflated = actor.size;
    inflated.radius += padding;
    inflated.height += 2 * padding;
    const auto query_start = old_feet;
    const auto query_end = destination;
    auto swept = sweep_capsule(geometry_, query_start, inflated, query_end - query_start, carrier);
    auto accepted = swept.feet;
    bool legal = !unsupported && swept.status != MoveStatus::InvalidStart &&
                 swept.status != MoveStatus::ContactBudget;
    if (carrier != 0 && length(accepted - destination) > 2 * contact_margin_m)
        legal = false;
    if (!stepping && legal) {
        const auto supported = continuous_surface_feet(geometry_, actor.support.surface, carried,
                                                       {accepted.x, accepted.z}, limits(actor));
        bool found = false;
        if (supported && std::abs(supported->y - accepted.y) < support_height_tolerance_m) {
            const auto& declared = geometry_.body(actor.support.surface);
            const auto declared_height = declared.support(
                {accepted.x, accepted.z}, actor.size.radius, limits(actor).maximum_slope);
            if (declared_height &&
                std::abs(*declared_height - accepted.y) < support_height_tolerance_m) {
                const auto local = declared.local(accepted);
                support = {declared.id, {local.x, local.z}};
                found = true;
            }
            for (const auto& body : geometry_.bodies) {
                if (found)
                    break;
                const auto height = body.support({accepted.x, accepted.z}, actor.size.radius,
                                                 limits(actor).maximum_slope);
                if (height && std::abs(*height - accepted.y) < support_height_tolerance_m) {
                    const auto local = body.local(accepted);
                    support = {body.id, {local.x, local.z}};
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            legal = false;
    }
    for (const auto& moving : scene_.platforms) {
        if (moving.definition.body == carrier) {
            continue;
        }
        const auto& from = substep_geometry_.body(moving.definition.body);
        auto to = geometry_.body(moving.definition.body);
        to.origin = to.origin - (accepted - old_feet);
        if (!prism_motion_clear(from, to, old_feet, actor.size)) {
            legal = false;
            break;
        }
    }
    for (std::size_t other = 0; legal && other < actors_.size(); ++other) {
        if (other == index) {
            continue;
        }
        const auto other_start = substep_starts_[other];
        const auto other_end = processed_[other] ? actors_[other].feet : substep_carries_[other];
        auto other_size = actors_[other].size;
        const auto other_padding = substep_padding_[other];
        other_size.radius += other_padding;
        other_size.height += 2 * other_padding;
        if (!capsule_paths_clear(query_start, accepted, inflated, other_start, other_end,
                                 other_size)) {
            legal = false;
        }
    }
    if (legal && carrier != 0) {
        // Own movement changes the local anchor after the tick's carry was checked.
        // Accept it only if the remaining carry from this new anchor is also safe.
        const auto& from = geometry_.body(carrier);
        const auto& to = end_geometry.body(carrier);
        const auto future =
            platform_anchor(from, to, accepted, actor.size, limits(actor).maximum_slope);
        const auto anchor = from.local(accepted);
        const auto pad =
            std::hypot(anchor.x, anchor.z) * (1 - std::cos(angle_delta(from.yaw, to.yaw) * .5));
        auto size = actor.size;
        size.radius += pad;
        size.height += 2 * pad;
        const auto sweep = sweep_capsule(geometry_, accepted, size, future - accepted, carrier);
        if (sweep.status == MoveStatus::InvalidStart || sweep.status == MoveStatus::ContactBudget ||
            length(sweep.feet - future) > 2 * contact_margin_m)
            legal = false;
        for (std::size_t other = 0; legal && other < actors_.size(); ++other) {
            if (other == index)
                continue;
            const auto& person = actors_[other];
            const auto current = processed_[other] ? person.feet : substep_carries_[other];
            auto next = current;
            if (platform(scene_, person.support.surface) && !committed_step(person) &&
                person.transfer != TransferStage::Boarding &&
                person.transfer != TransferStage::Exiting)
                next = platform_anchor(geometry_.body(person.support.surface),
                                       end_geometry.body(person.support.surface), current,
                                       person.size, limits(person).maximum_slope);
            if (!capsule_paths_clear(accepted, future, size, current, next, person.size))
                legal = false;
        }
    }
    legal = legal && passage_motion_clear(old_feet, accepted, inflated);
    actor.feet = legal ? accepted : carried;
    if (legal)
        actor.support = support;
    actor.motion = motion.state;
    if (!legal || length(actor.feet - destination) > 2 * contact_margin_m) {
        actor.motion.speed = 0;
        actor.motion.acceleration = 0;
        ++actor.blocked_ticks;
        if (actor.goal) {
            actor.state = TraversalState::Waiting;
        }
    } else {
        actor.support = support;
        if (length(actor.feet - old_feet) > 1e-5) {
            actor.blocked_ticks = 0;
        }
    }
    if (actor.goal && wants_movement &&
        (steering_blocked || (length(actor.feet - carried) <= 1e-5 && actor.motion.speed < .01))) {
        ++actor.blocked_ticks;
        actor.state = TraversalState::Waiting;
    }
    if (!stepping) {
        const auto local = geometry_.body(actor.support.surface).local(actor.feet);
        actor.support.local = {local.x, local.z};
    }
    processed_[index] = true;
    release_departed(scene_, geometry_, actor, true);
    append(history_[index], static_cast<double>(substep_ + 1) / subdivisions, actor.feet);
    if (actor.goal && actor.route.status == RouteStatus::Found) {
        progress(actor);
    }
    if (actor.goal && actor.leg < actor.route.legs.size() && wants_movement &&
        (steering_blocked || (length(actor.feet - carried) <= 1e-5 && actor.motion.speed < .01))) {
        actor.state = TraversalState::Waiting;
    }
}
void TraversalSession::tick() {
    const auto held = inputs_;
    for (std::size_t i = 0; i < inputs_.size(); ++i) {
        if (inputs_[i] == GroundPoint{}) {
            inputs_[i] = queued_inputs_[i];
        }
    }
    synchronize_presentation();
    for (auto& h : history_) {
        h.resize(1);
    }
    const auto now = world_.clock().tick_count();
    update_clearance();
    update_passages();
    update_knowledge();
    update_passages();
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        auto& actor = actors_[i];
        actor.hunger = std::min(std::uint64_t{1000}, actor.hunger + 1);
        if (actor.motion.speed > .1 && actor.energy > 0) {
            --actor.energy;
        }
        decide(i);
        if (!actor.yielding && actor.goal && actor.blocked_ticks >= 25 && now >= actor.retry_at &&
            actor.transfer == TransferStage::None && !committed_step(actor)) {
            if (actor.requested_goal && actor.requested_reach > 0) {
                // A failed alternative must not leave the obsolete route executing.
                // Revalidate its endpoint against the same observed neighbours.
                retry_approach(i);
                actor.blocked_ticks = 0;
            } else {
                plan(i, true);
                actor.blocked_ticks = 0;
            }
            actor.retry_at = now + 40;
        }
        const auto pending_approach = !actor.autonomous && actor.state == TraversalState::NoPath &&
                                      actor.requested_goal && actor.requested_reach > 0;
        if (!actor.yielding && (actor.goal || pending_approach) &&
            actor.route.status != RouteStatus::Found && now >= actor.retry_at) {
            // Retry against the same locally observed obstruction model. Dropping
            // neighbours here revives a known blocked route every two seconds.
            if (actor.requested_goal && actor.requested_reach > 0) {
                retry_approach(i);
            } else {
                plan(i, true);
            }
            actor.retry_at = now + 40;
        }
        if (actor.goal && actor.route.status == RouteStatus::Found) {
            progress(actor);
        }
    }
    std::vector<std::uint64_t> next_phases;
    for (auto& moving : scene_.platforms) {
        moving.blocked = false;
        auto next = (moving.phase + 1) % moving.definition.period();
        for (const auto& actor : actors_) {
            for (std::size_t ahead = actor.leg;
                 ahead < std::min(actor.route.legs.size(), actor.leg + 2); ++ahead) {
                const auto& leg = actor.route.legs[ahead];
                if (leg.kind != SurfaceLegKind::Step ||
                    (leg.from.surface != moving.definition.body &&
                     leg.to.surface != moving.definition.body)) {
                    continue;
                }
                const auto from_point = surface_feet(geometry_, leg.from, limits(actor));
                if ((ahead == actor.leg && actor.step_point > 0) ||
                    (from_point && length(*from_point - actor.feet) < .5 &&
                     step_ready(geometry_, leg, actor))) {
                    next = moving.phase;
                }
            }
        }
        for (const auto& s : scene_.services) {
            if (s.platform != moving.definition.body) {
                continue;
            }
            if (s.owner.value == 0) {
                if (!s.queue.empty()) {
                    const auto person =
                        std::find_if(actors_.begin(), actors_.end(),
                                     [&](const auto& a) { return a.id == s.queue.front(); });
                    if (person != actors_.end() &&
                        (person->transfer == TransferStage::Waiting ||
                         (person->leg + 1 < person->route.legs.size() &&
                          person->route.legs[person->leg].kind == SurfaceLegKind::Walk &&
                          person->route.legs[person->leg + 1].transport == s.id)) &&
                        platform_dock(moving.definition, moving.phase) == person->boarding_side &&
                        length(feet_at(geometry_,
                                       s.banks[static_cast<std::size_t>(person->boarding_side)],
                                       *person) -
                               person->feet) < 2.5)
                        next = moving.phase;
                }
                continue;
            }
            const auto person = std::find_if(actors_.begin(), actors_.end(),
                                             [&](const auto& a) { return a.id == s.owner; });
            check(person != actors_.end(), "missing transport passenger");
            if (person->transfer == TransferStage::Boarding ||
                person->transfer == TransferStage::Exiting) {
                next = moving.phase;
            }
        }
        const auto& from = geometry_.body(moving.definition.body);
        const auto to =
            platform_pose(scene_.geometry, moving.definition, static_cast<double>(next));
        for (const auto& body : geometry_.bodies) {
            if (body.id == from.id || body.id == moving.definition.foundation) {
                continue;
            }
            if (!prism_motion_clear(from, to, body)) {
                moving.blocked = true;
                break;
            }
        }
        for (const auto& actor : actors_) {
            if (moving.blocked) {
                break;
            }
            if (actor.support.surface == from.id &&
                from.support({actor.feet.x, actor.feet.z}, actor.size.radius,
                             limits(actor).maximum_slope)) {
                const auto destination =
                    platform_anchor(from, to, actor.feet, actor.size, limits(actor).maximum_slope);
                const auto local = from.local(actor.feet);
                const auto angle = std::abs(angle_delta(from.yaw, to.yaw));
                const auto pad = std::hypot(local.x, local.z) * (1 - std::cos(angle * .5));
                auto size = actor.size;
                size.radius += pad;
                size.height += 2 * pad;
                const auto carried =
                    sweep_capsule(geometry_, actor.feet, size, destination - actor.feet, from.id);
                if (length(carried.feet - destination) > 2 * contact_margin_m) {
                    moving.blocked = true;
                }
                for (const auto& other : actors_) {
                    if (other.id == actor.id) {
                        continue;
                    }
                    auto other_end = other.feet;
                    if (other.support.surface == from.id &&
                        from.support({other.feet.x, other.feet.z}, other.size.radius,
                                     limits(other).maximum_slope)) {
                        other_end = platform_anchor(from, to, other.feet, other.size,
                                                    limits(other).maximum_slope);
                    }
                    if (!capsule_paths_clear(actor.feet, destination, size, other.feet, other_end,
                                             other.size)) {
                        moving.blocked = true;
                        break;
                    }
                }
            } else if (!prism_motion_clear(from, to, actor.feet, actor.size)) {
                moving.blocked = true;
            }
        }
        if (moving.blocked) {
            next = moving.phase;
        }
        next_phases.push_back(next);
    }
    auto end_geometry = scene_.geometry;
    for (std::size_t p = 0; p < scene_.platforms.size(); ++p) {
        const auto pose = platform_pose(scene_.geometry, scene_.platforms[p].definition,
                                        static_cast<double>(next_phases[p]));
        for (auto& body : end_geometry.bodies)
            if (body.id == pose.id) {
                body = pose;
                break;
            }
    }
    for (substep_ = 0; substep_ < subdivisions; ++substep_) {
        substep_geometry_ = geometry_;
        substep_starts_.clear();
        for (const auto& a : actors_) {
            substep_starts_.push_back(a.feet);
        }
        processed_.assign(actors_.size(), false);
        const auto fraction = static_cast<double>(substep_ + 1) / subdivisions;
        geometry_ = scene_.geometry;
        for (std::size_t p = 0; p < scene_.platforms.size(); ++p) {
            const auto& moving = scene_.platforms[p];
            auto end = next_phases[p];
            if (end < moving.phase) {
                end += moving.definition.period();
            }
            auto phase = static_cast<double>(moving.phase) +
                         static_cast<double>(end - moving.phase) * fraction;
            phase = std::fmod(phase, static_cast<double>(moving.definition.period()));
            const auto pose = platform_pose(scene_.geometry, moving.definition, phase);
            for (auto& body : geometry_.bodies) {
                if (body.id == pose.id) {
                    body = pose;
                    break;
                }
            }
        }
        substep_carries_ = substep_starts_;
        substep_padding_.assign(actors_.size(), 0);
        for (std::size_t i = 0; i < actors_.size(); ++i) {
            const auto& actor = actors_[i];
            if (!platform(scene_, actor.support.surface) || committed_step(actor) ||
                actor.transfer == TransferStage::Boarding ||
                actor.transfer == TransferStage::Exiting) {
                continue;
            }
            const auto& before = substep_geometry_.body(actor.support.surface);
            const auto& after = geometry_.body(actor.support.surface);
            if (!before.support({actor.feet.x, actor.feet.z}, actor.size.radius,
                                limits(actor).maximum_slope)) {
                continue;
            }
            substep_carries_[i] =
                platform_anchor(before, after, actor.feet, actor.size, limits(actor).maximum_slope);
            const auto local = before.local(actor.feet);
            const auto angle = std::abs(angle_delta(before.yaw, after.yaw));
            substep_padding_[i] = (std::hypot(local.x, local.z) +
                                   tuning_.maximum_speed * step_seconds / subdivisions) *
                                      (1 - std::cos(angle * .5)) +
                                  tuning_.maximum_speed * step_seconds / subdivisions * angle * .5;
        }
        // Rotating the first mover avoids permanent ID priority. Unprocessed actors
        // retain their old trajectories; processed actors expose actual endpoints.
        for (std::size_t n = 0; n < actors_.size(); ++n) {
            move((n + static_cast<std::size_t>(now % actors_.size())) % actors_.size(),
                 step_seconds / subdivisions, end_geometry);
        }
    }
    for (std::size_t p = 0; p < scene_.platforms.size(); ++p) {
        scene_.platforms[p].phase = next_phases[p];
    }
    for (auto& station : scene_.stations) {
        if (station.kind == TraversalTask::Maintain) {
            station.wear = std::min(std::uint64_t{1000}, station.wear + 1);
        }
    }
    world_.advance(1);
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        if (actors_[i].action.value != 0) {
            decide(i);
        }
    }
    // Close agreements at the same authoritative boundary as their physical Step.
    update_clearance();
    update_passages();
    inputs_ = held;
    for (auto& input : queued_inputs_) {
        input = {};
    }
}
void TraversalSession::validate() const {
    world_.validate();
    scene_.geometry.validate();
    geometry_.validate();
    validate_clearance();
    validate_delivery();
    validate_knowledge();
    validate_law();
    validate_passages();
    tuning_.validate();
    check(!actors_.empty() && actors_.size() <= 32 && scene_.stations.size() <= 16 &&
              scene_.platforms.size() <= 16 && scene_.services.size() <= 16 &&
              scene_.passages.size() <= 16 && scene_.connections.size() <= 256,
          "invalid traversal collection size");
    check(next_command_.value != 0, "invalid next command");
    std::uint64_t largest = 0;
    for (const auto& command : world_.commands()) {
        largest = std::max(largest, command.request.command.value);
    }
    for (const auto& transfer : world_.transfers())
        largest = std::max(largest, transfer.request.command.value);
    check(next_command_.value > largest, "next command reuses an old identity");
    const auto known =
        scene_.stations.empty() ? 0 : ((std::uint32_t{1} << scene_.stations.size()) - 1);
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        const auto& actor = actors_[i];
        check(world_.entities().contains(actor.id, EntityKind::Actor),
              "traversal actor identity missing");
        check(actor.suspended.size() <= 2 &&
                  (actor.autonomous || (actor.suspended.empty() && actor.need_retry_at == 0 &&
                                        actor.finish_grace_until == 0)),
              "invalid suspended autonomous intent");
        check(actor.need_retry_at <= world_.clock().tick_count() + 40,
              "need retry exceeds observation horizon");
        check(actor.finish_grace_until <= world_.clock().tick_count() + 120,
              "finish grace exceeds its horizon");
        TraversalTask previous_paused = TraversalTask::None;
        for (const auto& paused : actor.suspended) {
            check((paused.task == TraversalTask::Maintain || paused.task == TraversalTask::Rest) &&
                      paused.station < scene_.stations.size() &&
                      scene_.stations[paused.station].kind == paused.task &&
                      (actor.known_stations & (std::uint32_t{1} << paused.station)) != 0 &&
                      paused.task != actor.task &&
                      (previous_paused == TraversalTask::None ||
                       (previous_paused == TraversalTask::Maintain &&
                        paused.task == TraversalTask::Rest)),
                  "invalid suspended task reference or priority");
            previous_paused = paused.task;
        }
        reality::validate(actor.motion, tuning_);
        actor.size.validate();
        check(spatial_occupiable(geometry_, actor.feet, actor.size),
              "traversal actor penetrates geometry");
        check(actor.hunger <= 1000 && actor.energy <= 1000 &&
                  (actor.known_stations & ~known) == 0 && actor.requested_reach >= 0 &&
                  actor.requested_reach <= 2 && std::isfinite(actor.requested_reach),
              "invalid actor needs or knowledge");
        for (std::size_t other = i + 1; other < actors_.size(); ++other) {
            check(actor.id != actors_[other].id, "duplicate traversal actor identity");
            check(capsule_paths_clear(actor.feet, actor.feet, actor.size, actors_[other].feet,
                                      actors_[other].feet, actors_[other].size),
                  "traversal actors overlap");
        }
        check(actor.route.legs.size() <= 512 && actor.leg <= actor.route.legs.size() &&
                  actor.step_point <= 8 && actor.transfer_point <= 8 && actor.boarding_side >= 0 &&
                  actor.boarding_side <= 1 &&
                  static_cast<unsigned>(actor.state) <=
                      static_cast<unsigned>(TraversalState::BudgetExceeded) &&
                  static_cast<unsigned>(actor.task) <=
                      static_cast<unsigned>(TraversalTask::Deliver) &&
                  static_cast<unsigned>(actor.transfer) <=
                      static_cast<unsigned>(TransferStage::Exiting),
              "invalid traversal state");
        static_cast<void>(geometry_.body(actor.support.surface));
        if (actor.task != TraversalTask::None && actor.task != TraversalTask::Deliver) {
            check(actor.station < scene_.stations.size() &&
                      scene_.stations[actor.station].kind == actor.task,
                  "invalid task station");
        }
        if (actor.action.value != 0) {
            const auto action = world_.action(actor.action);
            check(action && action->request.actor == actor.id &&
                      actor.task != TraversalTask::None && actor.task != TraversalTask::Deliver &&
                      action->request.lot == scene_.stations.at(actor.station).lot &&
                      (action->state == ActionState::Running ||
                       action->state == ActionState::Reserved),
                  "invalid active traversal action");
        }
        const auto is_step = actor.leg < actor.route.legs.size() &&
                             actor.route.legs[actor.leg].kind == SurfaceLegKind::Step &&
                             actor.step_point > 0;
        const auto is_transfer =
            actor.transfer == TransferStage::Boarding || actor.transfer == TransferStage::Exiting;
        check(is_step || actor.step_point == 0, "step cursor without a step");
        if (is_step) {
            const auto& leg = actor.route.legs[actor.leg];
            check(actor.step_point < step_points(feet_at(geometry_, leg.from, actor),
                                                 feet_at(geometry_, leg.to, actor))
                                         .size(),
                  "step cursor outside physical profile");
        }
        check(is_transfer || actor.transfer_point == 0, "transfer cursor outside boarding/exit");
        if (is_transfer) {
            check(actor.leg < actor.route.legs.size(), "transfer cursor without a route leg");
            const auto found =
                std::find_if(scene_.services.begin(), scene_.services.end(), [&](const auto& s) {
                    return s.id == actor.route.legs[actor.leg].transport;
                });
            check(found != scene_.services.end() &&
                      actor.transfer_point < transfer_points(geometry_, *found, actor).size(),
                  "transfer cursor outside physical profile");
        }
        if (!is_step && !is_transfer) {
            const auto& body = geometry_.body(actor.support.surface);
            const auto height = body.support({actor.feet.x, actor.feet.z}, actor.size.radius,
                                             limits(actor).maximum_slope);
            check(height && std::abs(*height - actor.feet.y) < support_height_tolerance_m,
                  "actor lost declared support");
        }
        for (const auto& leg : actor.route.legs) {
            static_cast<void>(scene_.geometry.body(leg.from.surface));
            static_cast<void>(scene_.geometry.body(leg.to.surface));
            check(static_cast<unsigned>(leg.kind) <=
                      static_cast<unsigned>(SurfaceLegKind::Transport),
                  "invalid route leg kind");
            if (leg.kind == SurfaceLegKind::Walk)
                check(leg.from.surface == leg.to.surface && leg.passage == 0 && leg.transport == 0,
                      "invalid walk leg");
            if (leg.kind == SurfaceLegKind::Step) {
                check(std::any_of(scene_.connections.begin(), scene_.connections.end(),
                                  [&](const auto& c) {
                                      return c.kind == leg.kind && c.passage == leg.passage &&
                                             c.transport == leg.transport &&
                                             ((c.from == leg.from && c.to == leg.to) ||
                                              (c.bidirectional && c.from == leg.to &&
                                               c.to == leg.from));
                                  }),
                      "route contains fabricated step");
            }
            if (leg.kind == SurfaceLegKind::Transport) {
                const auto s = std::find_if(scene_.services.begin(), scene_.services.end(),
                                            [&](const auto& s) { return s.id == leg.transport; });
                check(s != scene_.services.end() &&
                          ((s->banks[0] == leg.from && s->banks[1] == leg.to) ||
                           (s->banks[1] == leg.from && s->banks[0] == leg.to)),
                      "route contains fabricated transport");
            }
        }
        if (actor.transfer != TransferStage::None) {
            check(actor.leg < actor.route.legs.size() &&
                      actor.route.legs[actor.leg].kind == SurfaceLegKind::Transport,
                  "transfer has no transport leg");
        }
        check(actor.goal || (actor.route.legs.empty() && actor.transfer == TransferStage::None),
              "route or transfer without a goal");
        check(actor.goal ||
                  (actor.state != TraversalState::Moving && actor.state != TraversalState::Arrived),
              "moving or arrived state without a goal");
        for (std::size_t n = 1; n < actor.route.legs.size(); ++n)
            check(actor.route.legs[n - 1].to == actor.route.legs[n].from, "discontinuous route");
        if (actor.goal && actor.route.status == RouteStatus::Found && !actor.route.legs.empty())
            check(actor.route.legs.back().to == *actor.goal, "route endpoint differs from goal");
        for (const auto& s : scene_.services) {
            const auto using_service = actor.transfer != TransferStage::None &&
                                       actor.leg < actor.route.legs.size() &&
                                       actor.route.legs[actor.leg].transport == s.id;
            const auto owns = using_service && (actor.transfer == TransferStage::Boarding ||
                                                actor.transfer == TransferStage::Riding ||
                                                actor.transfer == TransferStage::Exiting);
            check((s.owner == actor.id) == owns, "transport owner and passenger phase disagree");
            if (using_service)
                check(std::find(s.queue.begin(), s.queue.end(), actor.id) != s.queue.end(),
                      "passenger absent from service queue");
        }
    }
    for (const auto& moving : scene_.platforms) {
        moving.definition.validate(scene_.geometry);
        check(moving.phase < moving.definition.period(), "invalid platform phase");
    }
    const auto valid_queue = [&](const auto& queue, EntityId owner) {
        check(queue.size() <= actors_.size(), "queue too large");
        bool has_owner = owner.value == 0;
        for (std::size_t q = 0; q < queue.size(); ++q) {
            check(world_.entities().contains(queue[q], EntityKind::Actor), "queue actor missing");
            for (std::size_t other = q + 1; other < queue.size(); ++other) {
                check(queue[q] != queue[other], "duplicate queue identity");
            }
            if (queue[q] == owner) {
                has_owner = true;
            }
        }
        check(has_owner, "queue owner missing");
    };
    for (const auto& s : scene_.services) {
        check(s.id != 0 && std::find_if(scene_.platforms.begin(), scene_.platforms.end(),
                                        [&](const auto& p) {
                                            return p.definition.body == s.platform;
                                        }) != scene_.platforms.end(),
              "invalid transport service");
        valid_queue(s.queue, s.owner);
    }
    for (const auto& p : scene_.passages) {
        check(p.id != 0, "invalid passage");
        static_cast<void>(scene_.geometry.body(p.surface));
        valid_queue(p.queue, p.owner);
    }
    std::set<EntityId> station_lots;
    for (const auto& station : scene_.stations) {
        const auto lot = world_.resource(station.lot);
        check(station.kind != TraversalTask::None &&
                  static_cast<unsigned>(station.kind) <=
                      static_cast<unsigned>(TraversalTask::Maintain) &&
                  station.wear <= 1000 && lot.has_value() &&
                  station_lots.insert(station.lot).second,
              "invalid traversal station");
        check(station.kind == TraversalTask::Rest ? (lot->record.kind == ResourceKind::Capacity ||
                                                     (lot->record.kind == ResourceKind::Material &&
                                                      lot->record.material.value == 0))
                                                  : lot->record.kind == ResourceKind::Material,
              "station resource kind mismatch");
        static_cast<void>(scene_.geometry.body(station.location.surface));
        std::uint64_t completed = 0;
        for (const auto& slot : world_.entities().slots()) {
            if (slot.occupied && slot.kind == EntityKind::Action) {
                const auto action = world_.action(slot.id);
                if (action && action->request.lot == station.lot &&
                    action->state == ActionState::Completed) {
                    ++completed;
                }
            }
        }
        check(completed == station.completed,
              "station completion is not backed by completed domain actions");
    }
}
std::string TraversalSession::inspect() const {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(3);
    out << "tick=" << world_.clock().tick_count() << " actors=" << actors_.size() << "\n";
    for (const auto& actor : actors_) {
        out << "actor=" << actor.id.value << " surface=" << actor.support.surface
            << " feet=" << actor.feet.x << ',' << actor.feet.y << ',' << actor.feet.z
            << " speed=" << actor.motion.speed << " accel=" << actor.motion.acceleration
            << " yaw=" << actor.motion.heading << " state=" << static_cast<unsigned>(actor.state)
            << " auto=" << actor.autonomous << " task=" << static_cast<unsigned>(actor.task)
            << " hunger=" << actor.hunger << " energy=" << actor.energy << " plans=" << actor.plans
            << " leg=" << actor.leg << '/' << actor.route.legs.size()
            << " transfer=" << static_cast<unsigned>(actor.transfer)
            << " action=" << actor.action.value << " blocked=" << actor.blocked_ticks
            << " step_point=" << actor.step_point << " need_retry_at=" << actor.need_retry_at
            << " finish_grace_until=" << actor.finish_grace_until << '\n';
        for (const auto& paused : actor.suspended)
            out << "  suspended_task=" << static_cast<unsigned>(paused.task)
                << " station=" << paused.station << '\n';
        if (actor.clearance.outcome != ClearanceOutcome::None)
            out << "  clearance_to=" << actor.clearance.peer.value
                << " response=" << clearance_label(actor.clearance.outcome)
                << " at=" << actor.clearance.at << " retry_at=" << actor.clearance.retry_at << '\n';
        if (actor.yielding)
            out << "  yielding_for=" << actor.yielding->requester.value
                << " expires_at=" << actor.yielding->expires_at << '\n';
        if (actor.cargo.value != 0)
            out << "  cargo_lot=" << actor.cargo.value
                << " cargo_quantity=" << world_.resource(actor.cargo)->record.quantity.value
                << " cargo_capacity=4\n";
        if (cargo_returning(actor))
            out << "  returning_to=" << actor.station << '\n';
        if (actor.delivery)
            out << "  delivery_phase=" << static_cast<unsigned>(actor.delivery->phase)
                << " source=" << actor.delivery->source
                << " destination=" << actor.delivery->destination
                << " delivered=" << actor.delivery->delivered << '/' << actor.delivery->quantity
                << '\n';
        for (std::size_t station = 0; station < scene_.stations.size(); ++station) {
            if ((actor.known_stations & (std::uint32_t{1} << station)) == 0)
                continue;
            const auto& belief = actor.beliefs[station];
            if (!belief) {
                out << "  belief_unknown_station=" << station << '\n';
                continue;
            }
            out << "  belief_station=" << station << " available=" << belief->available
                << " wear=" << belief->wear << " source_actor=" << belief->source_actor.value
                << " via_actor=" << belief->via_actor.value
                << " observed_tick=" << belief->observed_tick
                << " learned_tick=" << belief->learned_tick << '\n';
        }
        for (std::size_t p = 0; p < scene_.passages.size(); ++p) {
            const auto& b = actor.passage_beliefs[p];
            if (b)
                out << "  passage_belief=" << scene_.passages[p].id << " open=" << b->open
                    << " source_actor=" << b->source_actor.value
                    << " via_actor=" << b->via_actor.value << " observed_tick=" << b->observed_tick
                    << " learned_tick=" << b->learned_tick << '\n';
        }
        for (const auto& b : actor.testimonies)
            out << "  testimony_incident=" << b.incident.value << " offender=" << b.offender.value
                << " station=" << b.station << " lot=" << b.lot.value << " quantity=" << b.quantity
                << " resolved=" << b.resolved << " observed_tick=" << b.observed_tick
                << " learned_tick=" << b.learned_tick << " source_actor=" << b.source_actor.value
                << " via_actor=" << b.via_actor.value << '\n';
        if (actor.goal) {
            out << "  goal_surface=" << actor.goal->surface << " local=" << actor.goal->local.x_m
                << ',' << actor.goal->local.z_m << " reach=" << actor.requested_reach << '\n';
            constexpr std::array route_names{"found", "no_path", "invalid_endpoint",
                                             "budget_exceeded"};
            out << "  route=" << route_names.at(static_cast<std::size_t>(actor.route.status))
                << " retry_at=" << actor.retry_at << '\n';
            if (actor.state == TraversalState::Waiting) {
                for (const auto& p : scene_.passages) {
                    if (p.owner != actor.id &&
                        std::find(p.queue.begin(), p.queue.end(), actor.id) != p.queue.end())
                        out << "  waiting_for_passage=" << p.id << " owner=" << p.owner.value
                            << '\n';
                }
                for (const auto& s : scene_.services) {
                    if (s.owner != actor.id &&
                        std::find(s.queue.begin(), s.queue.end(), actor.id) != s.queue.end())
                        out << "  waiting_for_transport=" << s.id << " owner=" << s.owner.value
                            << '\n';
                }
            }
        }
    }
    for (const auto& moving : scene_.platforms) {
        out << "platform=" << moving.definition.body << " phase=" << moving.phase
            << " dock=" << platform_dock(moving.definition, moving.phase)
            << " blocked=" << moving.blocked << '\n';
    }
    for (const auto& station : scene_.stations) {
        const auto resource = *world_.resource(station.lot);
        out << "station=" << station.lot.value << " kind=" << static_cast<unsigned>(station.kind);
        if (resource.record.kind == ResourceKind::Capacity) {
            out << " capacity=" << resource.record.quantity.value
                << " occupied=" << resource.reserved.value
                << " available=" << resource.available.value;
        } else {
            out << " stock=" << resource.record.quantity.value;
        }
        out << " completed=" << station.completed << " wear=" << station.wear << '\n';
    }
    for (const auto& p : scene_.passages) {
        out << "passage=" << p.id << " surface=" << p.surface << " owner=" << p.owner.value
            << " queue=" << p.queue.size() << " open=" << p.open;
        if (p.maintenance)
            out << " maintenance_station=" << *p.maintenance;
        out << '\n';
    }
    for (const auto& s : scene_.services) {
        out << "service=" << s.id << " owner=" << s.owner.value << " queue=" << s.queue.size()
            << '\n';
    }
    for (const auto& v : incidents_)
        out << "incident_truth=" << v.command.value << " offender=" << v.offender.value
            << " station=" << v.station << " lot=" << v.lot.value << " cargo=" << v.cargo.value
            << " quantity=" << v.quantity << " returned=" << v.returned
            << " resolved_command=" << v.resolved_command.value << '\n';
    for (const auto& r : property_rules_) {
        out << "property_station=" << r.station << " custodian=" << r.custodian.value << '\n';
        for (std::size_t i = 0; i < actors_.size(); ++i)
            out << "  legal_service_actor=" << actors_[i].id.value
                << " allowed=" << legal_service_allowed(i, r.station) << '\n';
    }
    return out.str();
}
void TraversalSession::advance(std::uint64_t ticks) {
    check(ticks <= 1000000, "traversal batch too large");
    auto clock = world_.clock();
    clock.advance(ticks);
    for (std::uint64_t tick = 0; tick < ticks; ++tick) {
        auto candidate = *this;
        candidate.tick();
        *this = std::move(candidate);
    }
}
void TraversalSession::save(const std::filesystem::path& path) const {
    write_snapshot_atomic(serialize(), path);
}
void TraversalSession::load(const std::filesystem::path& path) {
    restore(read_snapshot_bytes(path));
}
} // namespace reality
