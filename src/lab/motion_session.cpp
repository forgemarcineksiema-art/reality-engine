#include "lab/motion_session.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace reality {
namespace {

KinematicGeometry course(std::array<bool, 2> gates, std::uint64_t version) {
    KinematicGeometry result{{{-10, -7}, {10, 7}},
                             {{{{-.25, -7}, {.25, -1.5}}, 0, 2.4, 1},
                              {{{-.25, 1.5}, {.25, 4.5}}, 0, 2.4, 2},
                              {{{-5, 2}, {-3, 4}}, 0, 2, 3},
                              {{{3, -5}, {5, -3}}, 0, 2, 4},
                              {{{3, 2}, {5, 2.4}}, 1.4, 2.1, 5}},
                             version};
    if (gates[0]) {
        result.obstacles.push_back({{{-.25, -1.5}, {.25, 1.5}}, 0, 2.4, 6});
    }
    if (gates[1]) {
        result.obstacles.push_back({{{-.25, 4.5}, {.25, 7}}, 0, 2.4, 7});
    }
    result.validate();
    return result;
}
StaticObstacle body_obstacle(GroundPoint position, BodySize size) {
    return {{{position.x_m - size.half_x_m, position.z_m - size.half_z_m},
             {position.x_m + size.half_x_m, position.z_m + size.half_z_m}},
            0,
            size.height_m,
            100};
}
StaticObstacle actor_obstacle(const MobileActor& actor) {
    return body_obstacle(actor.position, actor.size);
}
void check(bool valid, const char* message) {
    if (!valid) {
        throw std::runtime_error(message);
    }
}
void plan(MobileActor& actor, const KinematicGeometry& geometry) {
    check(actor.plans != std::numeric_limits<std::uint64_t>::max(), "route plan counter overflow");
    actor.route = find_motion_route(geometry, {}, actor.position, *actor.goal, actor.size);
    actor.next_point = 0;
    ++actor.plans;
    switch (actor.route.status) {
    case RouteStatus::Found:
        actor.state = TravelState::Following;
        break;
    case RouteStatus::NoPath:
        actor.state = TravelState::NoPath;
        break;
    case RouteStatus::InvalidEndpoint:
        actor.state = TravelState::InvalidEndpoint;
        break;
    case RouteStatus::BudgetExceeded:
        actor.state = TravelState::BudgetExceeded;
        break;
    }
}
using Histories = std::array<std::vector<MotionSample>, 2>;
Histories stationary_history(const std::array<MobileActor, 2>& actors) {
    return {{{{0, actors[0].position}, {1, actors[0].position}},
             {{0, actors[1].position}, {1, actors[1].position}}}};
}
void append_history(std::vector<MotionSample>& history, double fraction, GroundPoint position) {
    fraction = std::clamp(fraction, 0.0, 1.0);
    check(history.size() < 24584, "motion presentation history limit exceeded");
    if (fraction <= history.back().fraction) {
        check(distance(history.back().position, position) < 1e-8, "nonmonotonic motion history");
        history.back().position = position;
    } else {
        history.push_back({fraction, position});
    }
}
GroundPoint interpolate(MotionSample a, MotionSample b, double fraction) {
    const auto part = (fraction - a.fraction) / (b.fraction - a.fraction);
    return {a.position.x_m + (b.position.x_m - a.position.x_m) * part,
            a.position.z_m + (b.position.z_m - a.position.z_m) * part};
}
void follow(MobileActor& actor, const KinematicGeometry& geometry, const StaticObstacle& other,
            double metres, std::vector<MotionSample>& history) {
    const auto budget = metres;
    while (metres > 1e-10 && actor.next_point < actor.route.points.size()) {
        const auto target = actor.route.points[actor.next_point];
        const auto length = distance(actor.position, target);
        if (length < 1e-9) {
            ++actor.next_point;
            continue;
        }
        const auto amount = std::min(metres, length);
        const auto offset = (budget - metres) / budget;
        const auto result = move_kinematic(geometry, actor.position, actor.size,
                                           {(target.x_m - actor.position.x_m) * amount / length,
                                            (target.z_m - actor.position.z_m) * amount / length},
                                           std::span{&other, 1}, false);
        check(result.status != MoveStatus::InvalidStart, "invalid mobile actor start");
        for (std::size_t sample = 1; sample < result.sample_count; ++sample) {
            append_history(history, offset + result.samples[sample].fraction * amount / budget,
                           result.samples[sample].position);
        }
        actor.position = result.position;
        if (result.status != MoveStatus::Clear) {
            actor.state = TravelState::Waiting;
            return;
        }
        metres -= amount;
        if (amount == length) {
            ++actor.next_point;
        }
    }
    if (actor.next_point == actor.route.points.size()) {
        actor.state = TravelState::Arrived;
    } else {
        actor.state = TravelState::Following;
    }
}
void avoid_blocker(MobileActor& actor, const KinematicGeometry& geometry,
                   const StaticObstacle& other) {
    auto local = geometry;
    local.obstacles.push_back(other);
    local.validate();
    if (!can_occupy(local, *actor.goal, actor.size)) {
        return;
    }
    auto route = find_motion_route(local, {}, actor.position, *actor.goal, actor.size);
    if (route.status != RouteStatus::Found) {
        return;
    }
    check(actor.plans != std::numeric_limits<std::uint64_t>::max(), "route plan counter overflow");
    actor.route = std::move(route);
    actor.next_point = 0;
    actor.state = TravelState::Following;
    ++actor.plans;
}
std::uint64_t integer(std::istream& input) {
    std::string token;
    check(static_cast<bool>(input >> token), "truncated motion snapshot");
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    check(parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size(),
          "invalid motion integer");
    return value;
}
double real(std::istream& input) {
    double value = 0;
    check(static_cast<bool>(input >> value) && std::isfinite(value), "invalid motion coordinate");
    return value;
}
GroundPoint point(std::istream& input) {
    GroundPoint value{real(input), real(input)};
    validate(value);
    return value;
}
void write_point(std::ostream& out, GroundPoint p) { out << p.x_m << ' ' << p.z_m << ' '; }
void write_size(std::ostream& out, BodySize s) {
    out << s.half_x_m << ' ' << s.half_z_m << ' ' << s.height_m << ' ';
}
BodySize read_size(std::istream& input) {
    BodySize s{real(input), real(input), real(input)};
    validate(s);
    return s;
}

} // namespace

std::string_view describe(TravelState state) noexcept {
    switch (state) {
    case TravelState::Idle:
        return "idle";
    case TravelState::Following:
        return "following";
    case TravelState::Arrived:
        return "arrived";
    case TravelState::NoPath:
        return "no_path";
    case TravelState::InvalidEndpoint:
        return "invalid_endpoint";
    case TravelState::BudgetExceeded:
        return "budget_exceeded";
    case TravelState::Waiting:
        return "waiting";
    }
    return "invalid";
}
MotionSession::MotionSession(LabSession& laboratory)
    : laboratory_(laboratory), geometry_(course({}, 1)),
      actors_{
          {{laboratory.actors()[0], {-7, 0}, {.35, .35, 1.8}, {}, {}, 0, TravelState::Idle, 0},
           {laboratory.actors()[1], {-7, -4}, {.25, .25, 1.2}, {}, {}, 0, TravelState::Idle, 0}}} {
    check(world().clock().step() == std::chrono::milliseconds{50},
          "motion laboratory requires a 50 ms step");
    synchronize_presentation();
}
const WorkWorld& MotionSession::world() const noexcept { return laboratory_.world(); }
const KinematicGeometry& MotionSession::geometry() const noexcept { return geometry_; }
const std::array<MobileActor, 2>& MotionSession::actors() const noexcept { return actors_; }
void MotionSession::synchronize_presentation() { presentation_ = stationary_history(actors_); }
std::array<GroundPoint, 2> MotionSession::presentation_positions(double fraction) const {
    check(std::isfinite(fraction) && fraction >= 0 && fraction <= 1,
          "invalid presentation fraction");
    if (fraction == 1) {
        return {actors_[0].position, actors_[1].position};
    }
    std::array<GroundPoint, 2> poses;
    const auto& first = presentation_[0];
    poses[0] = first.front().position;
    for (std::size_t sample = 1; sample < first.size(); ++sample) {
        if (first[sample].fraction >= fraction) {
            poses[0] = interpolate(first[sample - 1], first[sample], fraction);
            break;
        }
    }
    // The second body's visual path is clipped against the displayed first body, not only
    // its later authoritative pose. This preserves separation during sequential integration.
    const auto other = body_obstacle(poses[0], actors_[0].size);
    const auto& second = presentation_[1];
    poses[1] = second.front().position;
    for (std::size_t sample = 1; sample < second.size(); ++sample) {
        if (second[sample - 1].fraction >= fraction) {
            break;
        }
        const auto target = second[sample].fraction <= fraction
                                ? second[sample].position
                                : interpolate(second[sample - 1], second[sample], fraction);
        const auto move = move_kinematic(geometry_, poses[1], actors_[1].size,
                                         {target.x_m - poses[1].x_m, target.z_m - poses[1].z_m},
                                         std::span{&other, 1}, false);
        check(move.status != MoveStatus::InvalidStart, "invalid interpolated body start");
        poses[1] = move.position;
        if (move.status != MoveStatus::Clear) {
            break;
        }
    }
    return poses;
}
bool MotionSession::gate_closed(std::size_t gate) const { return gates_.at(gate); }
std::optional<EntityId> MotionSession::interaction_target(std::size_t actor) const {
    const auto& source = actors_.at(actor);
    const auto& other = actors_[1 - actor];
    if (distance(source.position, other.position) <= 1.5 &&
        clear_segment(geometry_, source.position, other.position, source.size)) {
        return other.id;
    }
    return std::nullopt;
}
bool MotionSession::set_gate(std::size_t gate, bool closed) {
    if (gates_.at(gate) == closed) {
        return true;
    }
    check(geometry_.version != std::numeric_limits<std::uint64_t>::max(),
          "geometry version overflow");
    auto gates = gates_;
    gates.at(gate) = closed;
    auto candidate = course(gates, geometry_.version + 1);
    for (const auto& actor : actors_) {
        if (!can_occupy(candidate, actor.position, actor.size)) {
            return false;
        }
    }
    auto history = stationary_history(actors_);
    geometry_ = std::move(candidate);
    gates_ = gates;
    presentation_ = std::move(history);
    return true;
}
void MotionSession::set_goal(std::size_t actor, GroundPoint goal) {
    validate(goal);
    const auto& current = actors_.at(actor);
    const bool obsolete_endpoint =
        current.state == TravelState::InvalidEndpoint && can_occupy(geometry_, goal, current.size);
    if (current.goal == goal && current.route.geometry_version == geometry_.version &&
        !obsolete_endpoint) {
        inputs_.at(actor) = {};
        queued_inputs_.at(actor) = {};
        return;
    }
    auto candidate = actors_.at(actor);
    candidate.goal = goal;
    plan(candidate, geometry_);
    actors_.at(actor) = std::move(candidate);
    inputs_.at(actor) = {};
    queued_inputs_.at(actor) = {};
}
void MotionSession::set_input(std::size_t actor, GroundPoint direction) {
    validate(direction);
    const auto length = std::hypot(direction.x_m, direction.z_m);
    if (length > 1) {
        direction.x_m /= length;
        direction.z_m /= length;
    }
    inputs_.at(actor) = direction;
}
void MotionSession::queue_input(std::size_t actor, GroundPoint direction) {
    validate(direction);
    const auto length = std::hypot(direction.x_m, direction.z_m);
    if (length > 1) {
        direction.x_m /= length;
        direction.z_m /= length;
    }
    queued_inputs_.at(actor) = direction;
}
void MotionSession::clear_input() noexcept {
    inputs_ = {};
    queued_inputs_ = {};
}
bool MotionSession::accept_route(std::size_t actor, GroundRoute route) {
    auto& mobile = actors_.at(actor);
    if (!route_matches(route, geometry_, mobile.position, mobile.size) || route.expanded > 4096 ||
        route.points.size() > 4097 || !mobile.goal || route.goal != *mobile.goal ||
        distance(route.points.back(), route.goal) > 1e-9) {
        return false;
    }
    auto previous = mobile.position;
    for (const auto next : route.points) {
        if (!clear_segment(geometry_, previous, next, mobile.size)) {
            return false;
        }
        previous = next;
    }
    mobile.route = std::move(route);
    mobile.next_point = 0;
    mobile.state = TravelState::Following;
    return true;
}
void MotionSession::advance(std::uint64_t ticks) {
    auto preflight = world().clock();
    preflight.advance(ticks);
    const double seconds = std::chrono::duration<double>(world().clock().step()).count();
    for (std::uint64_t tick = 0; tick < ticks; ++tick) {
        auto candidates = actors_;
        Histories history;
        for (std::size_t index = 0; index < candidates.size(); ++index) {
            auto& actor = candidates[index];
            history[index].push_back({0, actor.position});
            const auto other = actor_obstacle(candidates[1 - index]);
            const auto held = inputs_[index];
            const auto input = held.x_m != 0 || held.z_m != 0 ? held : queued_inputs_[index];
            if (input.x_m != 0 || input.z_m != 0) {
                actor.goal.reset();
                actor.route = {};
                actor.next_point = 0;
                actor.state = TravelState::Idle;
                const auto result = move_kinematic(
                    geometry_, actor.position, actor.size,
                    {input.x_m * 3 * seconds, input.z_m * 3 * seconds}, std::span{&other, 1});
                check(result.status != MoveStatus::InvalidStart, "invalid direct movement start");
                actor.position = result.position;
                history[index].assign(result.samples.begin(),
                                      result.samples.begin() +
                                          static_cast<std::ptrdiff_t>(result.sample_count));
            } else if (actor.goal) {
                if (actor.route.geometry_version != geometry_.version ||
                    (actor.state == TravelState::InvalidEndpoint &&
                     can_occupy(geometry_, *actor.goal, actor.size))) {
                    plan(actor, geometry_);
                }
                if (actor.state == TravelState::Following || actor.state == TravelState::Waiting) {
                    const auto& neighbour = candidates[1 - index];
                    const bool yielding =
                        neighbour.state == TravelState::Waiting && actor.id < neighbour.id;
                    if (actor.state == TravelState::Waiting && !yielding &&
                        world().clock().tick_count() % 10 == (actor.id.value % 2) * 5) {
                        avoid_blocker(actor, geometry_, other);
                    }
                    follow(actor, geometry_, other, 3 * seconds, history[index]);
                }
            }
            append_history(history[index], 1, actor.position);
        }
        laboratory_.advance(1);
        static_assert(std::is_nothrow_move_assignable_v<decltype(actors_)>);
        actors_ = std::move(candidates);
        presentation_ = std::move(history);
        queued_inputs_ = {};
    }
}
std::string MotionSession::serialize() const {
    const auto domain = serialize_world(world());
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17);
    out << "GENESIS_MOTION 1\n" << domain.size() << '\n' << domain;
    out << geometry_.version << ' ' << gates_[0] << ' ' << gates_[1] << '\n';
    for (const auto& actor : actors_) {
        out << actor.id.value << ' ';
        write_point(out, actor.position);
        write_size(out, actor.size);
        out << actor.goal.has_value() << ' ';
        write_point(out, actor.goal.value_or(GroundPoint{}));
        out << static_cast<unsigned>(actor.state) << ' ' << actor.plans << ' ' << actor.next_point
            << ' ';
        out << static_cast<unsigned>(actor.route.status) << ' ' << actor.route.geometry_version
            << ' ';
        write_size(out, actor.route.size);
        write_point(out, actor.route.start);
        write_point(out, actor.route.goal);
        out << actor.route.expanded << ' ' << actor.route.points.size() << '\n';
        for (const auto p : actor.route.points) {
            write_point(out, p);
        }
        out << '\n';
    }
    const auto body = out.str();
    out << "CHECKSUM " << snapshot_checksum(body) << '\n';
    auto bytes = out.str();
    check(bytes.size() <= snapshot_byte_limit, "motion snapshot byte limit exceeded");
    return bytes;
}
void MotionSession::restore(std::string_view bytes) {
    check(bytes.size() <= snapshot_byte_limit, "motion snapshot byte limit exceeded");
    const auto marker = bytes.rfind("CHECKSUM ");
    check(marker != std::string_view::npos, "missing motion checksum");
    std::istringstream trailer{std::string{bytes.substr(marker + 9)}};
    check(integer(trailer) == snapshot_checksum(bytes.substr(0, marker)),
          "motion checksum mismatch");
    std::string extra;
    check(!(trailer >> extra), "trailing motion checksum data");
    std::istringstream input{std::string{bytes.substr(0, marker)}};
    input.imbue(std::locale::classic());
    std::string header;
    std::getline(input, header);
    check(header == "GENESIS_MOTION 1", "unsupported motion snapshot version");
    const auto length = integer(input);
    check(length <= snapshot_byte_limit && input.get() == '\n', "invalid nested snapshot length");
    std::string nested(static_cast<std::size_t>(length), '\0');
    input.read(nested.data(), static_cast<std::streamsize>(length));
    check(static_cast<std::uint64_t>(input.gcount()) == length, "truncated nested snapshot");
    LabSession lab;
    lab.restore(deserialize_world(nested));
    check(lab.world().clock().step() == std::chrono::milliseconds{50},
          "motion laboratory requires a 50 ms step");
    const auto version = integer(input);
    const auto main = integer(input);
    const auto detour = integer(input);
    check(main <= 1 && detour <= 1, "invalid gate flag");
    const std::array<bool, 2> gates{main != 0, detour != 0};
    auto geometry = course(gates, version);
    auto candidates = actors_;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        auto& actor = candidates[index];
        actor.id = EntityId{integer(input)};
        check(actor.id == lab.actors()[index], "mobile actor identity mismatch");
        actor.position = point(input);
        actor.size = read_size(input);
        const auto has_goal = integer(input);
        check(has_goal <= 1, "invalid goal flag");
        const auto goal = point(input);
        actor.goal = has_goal ? std::optional{goal} : std::nullopt;
        const auto state = integer(input);
        check(state <= static_cast<unsigned>(TravelState::Waiting), "invalid travel state");
        actor.state = static_cast<TravelState>(state);
        actor.plans = integer(input);
        const auto cursor = integer(input);
        const auto status = integer(input);
        check(status <= static_cast<unsigned>(RouteStatus::BudgetExceeded), "invalid route status");
        actor.route.status = static_cast<RouteStatus>(status);
        actor.route.geometry_version = integer(input);
        actor.route.size = read_size(input);
        actor.route.start = point(input);
        actor.route.goal = point(input);
        const auto expanded = integer(input);
        const auto count = integer(input);
        check(expanded <= 4096 && count <= 4097 && cursor <= count, "invalid route bounds");
        actor.route.expanded = static_cast<std::size_t>(expanded);
        actor.next_point = static_cast<std::size_t>(cursor);
        actor.route.points.clear();
        for (std::uint64_t p = 0; p < count; ++p) {
            actor.route.points.push_back(point(input));
        }
        check(can_occupy(geometry, actor.position, actor.size), "mobile actor overlaps geometry");
        if (!actor.goal) {
            check(actor.state == TravelState::Idle && count == 0 && cursor == 0 &&
                      actor.route.geometry_version == 0,
                  "idle mobile actor contains a route");
        } else {
            check(actor.state != TravelState::Idle && actor.route.geometry_version > 0 &&
                      actor.route.geometry_version <= version && actor.route.size == actor.size &&
                      actor.route.goal == *actor.goal && actor.plans > 0,
                  "inconsistent route metadata");
            const auto found = actor.route.status == RouteStatus::Found;
            const auto travelling =
                actor.state == TravelState::Following || actor.state == TravelState::Waiting;
            check(found ? (count > 0 && distance(actor.route.points.back(), *actor.goal) < 1e-9 &&
                           ((travelling && cursor < count) ||
                            (actor.state == TravelState::Arrived && cursor == count &&
                             distance(actor.position, *actor.goal) < 1e-9)))
                        : (count == 0 && cursor == 0 &&
                           ((actor.state == TravelState::NoPath &&
                             actor.route.status == RouteStatus::NoPath) ||
                            (actor.state == TravelState::InvalidEndpoint &&
                             actor.route.status == RouteStatus::InvalidEndpoint) ||
                            (actor.state == TravelState::BudgetExceeded &&
                             actor.route.status == RouteStatus::BudgetExceeded))),
                  "inconsistent route state");
            if (found && actor.route.geometry_version == version) {
                auto previous = actor.position;
                for (std::size_t p = actor.next_point; p < actor.route.points.size(); ++p) {
                    check(clear_segment(geometry, previous, actor.route.points[p], actor.size),
                          "route crosses geometry");
                    previous = actor.route.points[p];
                }
            }
        }
    }
    const auto other = actor_obstacle(candidates[1]);
    check(can_occupy(geometry, candidates[0].position, candidates[0].size, std::span{&other, 1}),
          "mobile actors overlap");
    check(!(input >> extra), "trailing motion data");
    auto history = stationary_history(candidates);
    laboratory_.restore(lab.world());
    geometry_ = std::move(geometry);
    gates_ = gates;
    actors_ = std::move(candidates);
    presentation_ = std::move(history);
    clear_input();
}
void MotionSession::save(const std::filesystem::path& path) const {
    write_snapshot_atomic(serialize(), path);
}
void MotionSession::load(const std::filesystem::path& path) { restore(read_snapshot_bytes(path)); }
std::string MotionSession::inspect() const {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(2);
    out << "geometry=" << geometry_.version << " main=" << gates_[0] << " detour=" << gates_[1]
        << '\n';
    for (const auto& actor : actors_) {
        out << "actor=" << actor.id.value << " x=" << actor.position.x_m
            << " z=" << actor.position.z_m << ' ' << describe(actor.state)
            << " waypoint=" << actor.next_point << '/' << actor.route.points.size()
            << " plans=" << actor.plans << '\n';
    }
    return out.str();
}

} // namespace reality
