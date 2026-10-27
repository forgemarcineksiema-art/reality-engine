#include "lab/traversal_session.hpp"

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
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::invalid_argument(message);
    }
}
std::uint64_t integer(std::istream& input,
                      std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max()) {
    std::string token;
    require(static_cast<bool>(input >> token), "missing traversal integer");
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    require(parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size() &&
                value <= maximum,
            "invalid traversal integer");
    return value;
}
double real(std::istream& input, double maximum = 1000000) {
    double value = 0;
    require(static_cast<bool>(input >> value) && std::isfinite(value) && std::abs(value) <= maximum,
            "invalid traversal real");
    return value;
}
void tag(std::istream& input, const char* expected) {
    std::string found;
    require(static_cast<bool>(input >> found) && found == expected, "invalid traversal record");
}
void point(std::ostream& out, SpatialPoint p) { out << ' ' << p.x << ' ' << p.y << ' ' << p.z; }
SpatialPoint point(std::istream& in) { return {real(in), real(in), real(in)}; }
void location(std::ostream& out, SurfaceLocation p) {
    out << ' ' << p.surface << ' ' << p.local.x_m << ' ' << p.local.z_m;
}
SurfaceLocation location(std::istream& in) {
    return {static_cast<std::uint32_t>(integer(in, UINT32_MAX)), {real(in), real(in)}};
}
void optional_location(std::ostream& out, std::optional<SurfaceLocation> p) {
    out << ' ' << p.has_value();
    if (p) {
        location(out, *p);
    }
}
std::optional<SurfaceLocation> optional_location(std::istream& in) {
    if (integer(in, 1) == 0) {
        return std::nullopt;
    }
    return location(in);
}
void queue(std::ostream& out, const std::vector<EntityId>& queue) {
    out << ' ' << queue.size();
    for (const auto id : queue) {
        out << ' ' << id.value;
    }
}
std::vector<EntityId> queue(std::istream& in) {
    std::vector<EntityId> result;
    const auto count = integer(in, 32);
    for (std::uint64_t i = 0; i < count; ++i) {
        result.push_back(EntityId{integer(in)});
    }
    return result;
}
} // namespace
std::string TraversalSession::serialize() const {
    validate();
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17);
    const auto work = serialize_world(world_);
    const auto passage_state =
        std::any_of(scene_.passages.begin(), scene_.passages.end(),
                    [](const auto& p) { return !p.open || p.maintenance.has_value(); }) ||
        std::any_of(actors_.begin(), actors_.end(), [](const auto& a) {
            return a.planned_closed_passages != 0 ||
                   std::any_of(a.passage_beliefs.begin(), a.passage_beliefs.end(),
                               [](const auto& b) { return b.has_value(); });
        });
    const auto knowledgeable =
        passage_state || std::any_of(actors_.begin(), actors_.end(), [](const auto& actor) {
            return actor.share_until != 0 || actor.share_retry_at != 0 ||
                   std::any_of(actor.beliefs.begin(), actor.beliefs.end(),
                               [](const auto& belief) { return belief.has_value(); });
        });
    const auto inventory =
        knowledgeable || std::any_of(actors_.begin(), actors_.end(), [](const auto& actor) {
            return actor.cargo.value != 0 || actor.delivery.has_value();
        });
    const auto cooperative =
        inventory || std::any_of(actors_.begin(), actors_.end(), [](const auto& actor) {
            return actor.yielding || actor.clearance.outcome != ClearanceOutcome::None;
        });
    const auto extended =
        cooperative || std::any_of(actors_.begin(), actors_.end(), [](const auto& actor) {
            return !actor.suspended.empty() || actor.need_retry_at != 0 ||
                   actor.finish_grace_until != 0;
        });
    out << "GENESIS_TRAVERSAL "
        << (passage_state   ? 6
            : knowledgeable ? 5
            : inventory     ? 4
            : cooperative   ? 3
            : extended      ? 2
                            : 1)
        << "\nWORLD " << work.size() << '\n'
        << work;
    out << "TUNING " << next_command_.value << ' ' << tuning_.maximum_speed << ' '
        << tuning_.acceleration << ' ' << tuning_.braking << ' ' << tuning_.jerk << ' '
        << tuning_.turn_speed << ' ' << tuning_.turn_acceleration << ' '
        << tuning_.lateral_acceleration << '\n';
    out << "BASE " << scene_.geometry.version << ' ' << scene_.geometry.bodies.size() << '\n';
    for (const auto& b : scene_.geometry.bodies) {
        out << b.id;
        point(out, b.origin);
        out << ' ' << b.yaw << ' ' << b.half_x << ' ' << b.half_z << ' ' << b.depth << ' '
            << b.slope_x << ' ' << b.slope_z << ' ' << b.walkable << '\n';
    }
    out << "CONNECTIONS " << scene_.connections.size() << '\n';
    for (const auto& c : scene_.connections) {
        location(out, c.from);
        location(out, c.to);
        out << ' ' << static_cast<unsigned>(c.kind) << ' ' << c.passage << ' ' << c.transport << ' '
            << c.extra_cost_m << ' ' << c.bidirectional << '\n';
    }
    out << "PLATFORMS " << scene_.platforms.size() << '\n';
    for (const auto& p : scene_.platforms) {
        const auto& d = p.definition;
        out << d.body;
        point(out, d.translation);
        out << ' ' << d.rotation << ' ' << d.travel_ticks << ' ' << d.dwell_ticks << ' '
            << d.foundation << ' ' << p.phase << ' ' << p.blocked << '\n';
    }
    out << "SERVICES " << scene_.services.size() << '\n';
    for (const auto& s : scene_.services) {
        out << s.id << ' ' << s.platform;
        location(out, s.banks[0]);
        location(out, s.banks[1]);
        for (const auto p : s.deck_ports) {
            out << ' ' << p.x_m << ' ' << p.z_m;
        }
        out << ' ' << s.owner.value;
        queue(out, s.queue);
        out << '\n';
    }
    out << "PASSAGES " << scene_.passages.size() << '\n';
    for (const auto& p : scene_.passages) {
        out << p.id << ' ' << p.surface << ' ' << p.owner.value;
        queue(out, p.queue);
        out << '\n';
        if (passage_state) {
            out << "PASSAGE_STATE " << p.id << ' ' << p.open << ' ' << p.maintenance.has_value();
            if (p.maintenance)
                out << ' ' << *p.maintenance;
            out << '\n';
        }
    }
    out << "STATIONS " << scene_.stations.size() << '\n';
    for (const auto& s : scene_.stations) {
        location(out, s.location);
        out << ' ' << static_cast<unsigned>(s.kind) << ' ' << s.initial_stock << ' ' << s.lot.value
            << ' ' << s.wear << ' ' << s.completed << '\n';
    }
    out << "ACTORS " << actors_.size() << '\n';
    for (const auto& a : actors_) {
        out << "ACTOR " << a.id.value;
        location(out, a.support);
        point(out, a.feet);
        out << ' ' << a.motion.speed << ' ' << a.motion.acceleration << ' ' << a.motion.heading
            << ' ' << a.motion.turn_speed << ' ' << a.size.radius << ' ' << a.size.height << ' '
            << static_cast<unsigned>(a.state) << ' ' << a.autonomous << ' '
            << static_cast<unsigned>(a.task) << ' ' << a.station << ' ' << a.known_stations << ' '
            << a.hunger << ' ' << a.energy << ' ' << a.retry_at << ' ' << a.blocked_ticks << ' '
            << a.plans << ' ' << a.action.value << ' ' << static_cast<unsigned>(a.transfer) << ' '
            << a.transfer_point << ' ' << a.boarding_side << ' ' << a.leg << ' ' << a.step_point
            << '\n';
        out << "GOALS";
        optional_location(out, a.goal);
        optional_location(out, a.requested_goal);
        out << ' ' << a.requested_reach;
        optional_location(out, a.pending_goal);
        out << ' ' << a.pending_reach << ' ' << a.stop_after_transfer << '\n';
        out << "MEMORY";
        for (const auto retry : a.station_retry) {
            out << ' ' << retry;
        }
        out << '\n';
        if (extended) {
            out << "AUTONOMY " << a.need_retry_at << ' ' << a.finish_grace_until << ' '
                << a.suspended.size();
            for (const auto& paused : a.suspended)
                out << ' ' << static_cast<unsigned>(paused.task) << ' ' << paused.station;
            out << '\n';
        }
        if (cooperative) {
            out << "CLEARANCE " << a.clearance.peer.value << ' '
                << static_cast<unsigned>(a.clearance.outcome) << ' ' << a.clearance.at << ' '
                << a.clearance.retry_at << '\n';
            out << "YIELD " << a.yielding.has_value();
            if (a.yielding) {
                const auto& move = *a.yielding;
                out << ' ' << move.requester.value << ' ' << move.requested_at << ' '
                    << move.expires_at;
                location(out, move.from);
                location(out, move.to);
                optional_location(out, move.original_goal);
            }
            out << '\n';
        }
        if (inventory) {
            out << "CARGO " << a.cargo.value << "\nDELIVERY " << a.delivery.has_value();
            if (a.delivery) {
                const auto& job = *a.delivery;
                out << ' ' << job.source << ' ' << job.destination << ' ' << job.quantity << ' '
                    << job.delivered << ' ' << static_cast<unsigned>(job.phase) << ' '
                    << job.started_command.value << ' ' << job.ended_command.value;
            }
            out << '\n';
        }
        if (knowledgeable) {
            const auto count = std::count_if(a.beliefs.begin(), a.beliefs.end(),
                                             [](const auto& b) { return b.has_value(); });
            out << "KNOWLEDGE " << a.share_until << ' ' << a.share_retry_at << ' ' << count << '\n';
            for (std::size_t station = 0; station < a.beliefs.size(); ++station) {
                if (!a.beliefs[station])
                    continue;
                const auto& b = *a.beliefs[station];
                out << "BELIEF " << station << ' ' << b.available << ' ' << b.wear << ' '
                    << b.observed_tick << ' ' << b.learned_tick << ' ' << b.source_actor.value
                    << ' ' << b.via_actor.value << '\n';
            }
        }
        if (passage_state) {
            const auto count = std::count_if(a.passage_beliefs.begin(), a.passage_beliefs.end(),
                                             [](const auto& b) { return b.has_value(); });
            out << "PASSAGE_MEMORY " << a.planned_closed_passages << ' ' << count << '\n';
            for (std::size_t i = 0; i < a.passage_beliefs.size(); ++i)
                if (a.passage_beliefs[i]) {
                    const auto& b = *a.passage_beliefs[i];
                    out << "PASSAGE_BELIEF " << scene_.passages[i].id << ' ' << b.open << ' '
                        << b.observed_tick << ' ' << b.learned_tick << ' ' << b.source_actor.value
                        << ' ' << b.via_actor.value << '\n';
                }
        }
        out << "ROUTE " << static_cast<unsigned>(a.route.status) << ' ' << a.route.geometry_version
            << ' ' << a.route.cost_m << ' ' << a.route.expanded << ' ' << a.route.visibility_queries
            << ' ' << a.route.legs.size() << '\n';
        for (const auto& leg : a.route.legs) {
            location(out, leg.from);
            location(out, leg.to);
            out << ' ' << static_cast<unsigned>(leg.kind) << ' ' << leg.passage << ' '
                << leg.transport << '\n';
        }
    }
    const auto body = out.str();
    require(body.size() + 64 < snapshot_byte_limit, "traversal snapshot exceeds limit");
    return body + "CHECKSUM " + std::to_string(snapshot_checksum(body)) + "\n";
}
void TraversalSession::restore(std::string_view bytes) {
    require(bytes.size() <= snapshot_byte_limit, "traversal snapshot exceeds limit");
    const auto checksum = bytes.rfind("CHECKSUM ");
    require(checksum != std::string_view::npos, "traversal checksum missing");
    std::istringstream tail{std::string{bytes.substr(checksum)}};
    tag(tail, "CHECKSUM");
    require(integer(tail) == snapshot_checksum(bytes.substr(0, checksum)),
            "traversal checksum mismatch");
    std::string extra;
    require(!(tail >> extra), "trailing traversal checksum data");
    std::istringstream input{std::string{bytes.substr(0, checksum)}};
    input.imbue(std::locale::classic());
    tag(input, "GENESIS_TRAVERSAL");
    const auto version = integer(input, 6);
    require(version >= 1 && version <= 6, "unsupported traversal version");
    tag(input, "WORLD");
    const auto work_size = integer(input, snapshot_byte_limit);
    require(input.get() == '\n', "invalid traversal world framing");
    std::string work(static_cast<std::size_t>(work_size), '\0');
    input.read(work.data(), static_cast<std::streamsize>(work.size()));
    require(static_cast<std::size_t>(input.gcount()) == work.size(), "truncated traversal world");
    auto candidate = *this;
    candidate.world_ = deserialize_world(work);
    candidate.scene_ = {};
    candidate.actors_.clear();
    tag(input, "TUNING");
    candidate.next_command_ = {integer(input)};
    candidate.tuning_ = {real(input), real(input), real(input), real(input),
                         real(input), real(input), real(input)};
    tag(input, "BASE");
    candidate.scene_.geometry.version = integer(input);
    const auto body_count = integer(input, 64);
    for (std::uint64_t i = 0; i < body_count; ++i) {
        SpatialPrism b;
        b.id = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        b.origin = point(input);
        b.yaw = real(input);
        b.half_x = real(input);
        b.half_z = real(input);
        b.depth = real(input);
        b.slope_x = real(input);
        b.slope_z = real(input);
        b.walkable = integer(input, 1) != 0;
        candidate.scene_.geometry.bodies.push_back(b);
    }
    tag(input, "CONNECTIONS");
    const auto connection_count = integer(input, 256);
    for (std::uint64_t i = 0; i < connection_count; ++i) {
        SurfaceConnection c;
        c.from = location(input);
        c.to = location(input);
        c.kind = static_cast<SurfaceLegKind>(integer(input, 2));
        c.passage = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        c.transport = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        c.extra_cost_m = real(input, 1000000000);
        c.bidirectional = integer(input, 1) != 0;
        candidate.scene_.connections.push_back(c);
    }
    tag(input, "PLATFORMS");
    const auto platform_count = integer(input, 16);
    for (std::uint64_t i = 0; i < platform_count; ++i) {
        PlatformState p;
        p.definition.body = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        p.definition.translation = point(input);
        p.definition.rotation = real(input);
        p.definition.travel_ticks = static_cast<std::uint32_t>(integer(input, 10000));
        p.definition.dwell_ticks = static_cast<std::uint32_t>(integer(input, 10000));
        p.definition.foundation = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        p.phase = integer(input);
        p.blocked = integer(input, 1) != 0;
        candidate.scene_.platforms.push_back(p);
    }
    tag(input, "SERVICES");
    const auto service_count = integer(input, 16);
    for (std::uint64_t i = 0; i < service_count; ++i) {
        TransportService s;
        s.id = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        s.platform = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        s.banks = {location(input), location(input)};
        for (auto& p : s.deck_ports) {
            p = {real(input), real(input)};
        }
        s.owner = {integer(input)};
        s.queue = queue(input);
        candidate.scene_.services.push_back(std::move(s));
    }
    tag(input, "PASSAGES");
    const auto passage_count = integer(input, 16);
    for (std::uint64_t i = 0; i < passage_count; ++i) {
        PassageState p;
        p.id = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        p.surface = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        p.owner = {integer(input)};
        p.queue = queue(input);
        if (version >= 6) {
            tag(input, "PASSAGE_STATE");
            require(integer(input, UINT32_MAX) == p.id, "passage state identity mismatch");
            p.open = integer(input, 1) != 0;
            if (integer(input, 1) != 0)
                p.maintenance = static_cast<std::uint32_t>(integer(input, 15));
        }
        candidate.scene_.passages.push_back(std::move(p));
    }
    tag(input, "STATIONS");
    const auto station_count = integer(input, 16);
    for (std::uint64_t i = 0; i < station_count; ++i) {
        TraversalStation s;
        s.location = location(input);
        s.kind = static_cast<TraversalTask>(integer(input, 3));
        s.initial_stock = integer(input);
        s.lot = {integer(input)};
        s.wear = integer(input, 1000);
        s.completed = integer(input);
        candidate.scene_.stations.push_back(s);
    }
    tag(input, "ACTORS");
    const auto actor_count = integer(input, 32);
    for (std::uint64_t i = 0; i < actor_count; ++i) {
        tag(input, "ACTOR");
        TraversalActor a;
        a.id = {integer(input)};
        a.support = location(input);
        a.feet = point(input);
        a.motion = {real(input), real(input), real(input), real(input)};
        a.size = {real(input), real(input)};
        a.state = static_cast<TraversalState>(integer(input, 6));
        a.autonomous = integer(input, 1) != 0;
        a.task = static_cast<TraversalTask>(integer(input, version >= 4 ? 4 : 3));
        a.station = static_cast<std::uint32_t>(integer(input, 16));
        a.known_stations = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
        a.hunger = integer(input, 1000);
        a.energy = integer(input, 1000);
        a.retry_at = integer(input);
        a.blocked_ticks = integer(input);
        a.plans = integer(input);
        a.action = {integer(input)};
        a.transfer = static_cast<TransferStage>(integer(input, 4));
        a.transfer_point = static_cast<std::size_t>(integer(input, 8));
        a.boarding_side = static_cast<int>(integer(input, 1));
        a.leg = static_cast<std::size_t>(integer(input, 512));
        a.step_point = static_cast<std::size_t>(integer(input, 8));
        tag(input, "GOALS");
        a.goal = optional_location(input);
        a.requested_goal = optional_location(input);
        a.requested_reach = real(input, 2);
        a.pending_goal = optional_location(input);
        a.pending_reach = real(input, 2);
        a.stop_after_transfer = integer(input, 1) != 0;
        tag(input, "MEMORY");
        for (auto& retry : a.station_retry) {
            retry = integer(input);
        }
        if (version >= 2) {
            tag(input, "AUTONOMY");
            a.need_retry_at = integer(input);
            a.finish_grace_until = integer(input);
            const auto paused_count = integer(input, 2);
            for (std::uint64_t n = 0; n < paused_count; ++n)
                a.suspended.push_back({static_cast<TraversalTask>(integer(input, 3)),
                                       static_cast<std::uint32_t>(integer(input, 16))});
        }
        if (version >= 3) {
            tag(input, "CLEARANCE");
            a.clearance.peer = {integer(input)};
            a.clearance.outcome = static_cast<ClearanceOutcome>(integer(input, 7));
            a.clearance.at = integer(input);
            a.clearance.retry_at = integer(input);
            tag(input, "YIELD");
            if (integer(input, 1) != 0) {
                ClearanceMove move;
                move.requester = {integer(input)};
                move.requested_at = integer(input);
                move.expires_at = integer(input);
                move.from = location(input);
                move.to = location(input);
                move.original_goal = optional_location(input);
                a.yielding = move;
            }
        }
        if (version >= 4) {
            tag(input, "CARGO");
            a.cargo = {integer(input)};
            tag(input, "DELIVERY");
            if (integer(input, 1) != 0) {
                DeliveryState job;
                job.source = static_cast<std::uint32_t>(integer(input, 16));
                job.destination = static_cast<std::uint32_t>(integer(input, 16));
                job.quantity = integer(input, 1024);
                job.delivered = integer(input, 1024);
                job.phase = static_cast<DeliveryPhase>(integer(input, 4));
                job.started_command = {integer(input)};
                job.ended_command = {integer(input)};
                a.delivery = job;
            }
        }
        if (version >= 5) {
            tag(input, "KNOWLEDGE");
            a.share_until = integer(input);
            a.share_retry_at = integer(input);
            const auto count = integer(input, 16);
            for (std::uint64_t n = 0; n < count; ++n) {
                tag(input, "BELIEF");
                const auto station = static_cast<std::size_t>(integer(input, 15));
                require(!a.beliefs[station], "duplicate station belief");
                StationBelief belief;
                belief.available = integer(input);
                belief.wear = integer(input, 1000);
                belief.observed_tick = integer(input);
                belief.learned_tick = integer(input);
                belief.source_actor = {integer(input)};
                belief.via_actor = {integer(input)};
                a.beliefs[station] = belief;
            }
        }
        if (version >= 6) {
            tag(input, "PASSAGE_MEMORY");
            a.planned_closed_passages = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
            const auto count = integer(input, 16);
            for (std::uint64_t n = 0; n < count; ++n) {
                tag(input, "PASSAGE_BELIEF");
                const auto id = integer(input, UINT32_MAX);
                const auto p =
                    std::find_if(candidate.scene_.passages.begin(), candidate.scene_.passages.end(),
                                 [&](const auto& value) { return value.id == id; });
                require(p != candidate.scene_.passages.end(), "unknown passage belief identity");
                const auto slot = static_cast<std::size_t>(p - candidate.scene_.passages.begin());
                require(!a.passage_beliefs[slot], "duplicate passage belief");
                PassageBelief b;
                b.open = integer(input, 1) != 0;
                b.observed_tick = integer(input);
                b.learned_tick = integer(input);
                b.source_actor = {integer(input)};
                b.via_actor = {integer(input)};
                a.passage_beliefs[slot] = b;
            }
        }
        tag(input, "ROUTE");
        a.route.status = static_cast<RouteStatus>(integer(input, 3));
        a.route.geometry_version = integer(input);
        a.route.cost_m = real(input, 1000000000);
        a.route.expanded = static_cast<std::size_t>(integer(input, 512));
        a.route.visibility_queries = static_cast<std::size_t>(integer(input, 32768));
        const auto leg_count = integer(input, 512);
        for (std::uint64_t leg = 0; leg < leg_count; ++leg) {
            SurfaceLeg l;
            l.from = location(input);
            l.to = location(input);
            l.kind = static_cast<SurfaceLegKind>(integer(input, 2));
            l.passage = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
            l.transport = static_cast<std::uint32_t>(integer(input, UINT32_MAX));
            a.route.legs.push_back(l);
        }
        candidate.actors_.push_back(std::move(a));
    }
    require(!(input >> extra), "trailing traversal data");
    candidate.rebuild_geometry();
    candidate.inputs_.assign(candidate.actors_.size(), {});
    candidate.queued_inputs_.assign(candidate.actors_.size(), {});
    candidate.validate();
    candidate.synchronize_presentation();
    *this = std::move(candidate);
}
} // namespace reality
