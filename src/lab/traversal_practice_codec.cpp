#include "lab/traversal_session.hpp"
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
namespace reality {
namespace {
void check(bool v) {
    if (!v)
        throw std::invalid_argument("invalid practice snapshot");
}
std::uint64_t number(std::istream& in, std::uint64_t max = UINT64_MAX) {
    std::string s;
    check(static_cast<bool>(in >> s));
    std::uint64_t n = 0;
    const auto p = std::from_chars(s.data(), s.data() + s.size(), n);
    check(p.ec == std::errc{} && p.ptr == s.data() + s.size() && n <= max);
    return n;
}
void tag(std::istream& in, const char* expected) {
    std::string s;
    check(static_cast<bool>(in >> s) && s == expected);
}
std::string quoted(std::istream& in) {
    std::string s;
    check(static_cast<bool>(in >> std::quoted(s)));
    return s;
}
void chain(std::ostream& out, const DiscoveryChain& c) {
    out << c.size();
    for (auto x : c)
        out << ' ' << unsigned(x);
}
DiscoveryChain chain(std::istream& in) {
    DiscoveryChain c;
    auto n = number(in, 3);
    check(n > 0);
    while (n--)
        c.push_back(static_cast<std::uint8_t>(number(in, 2)));
    return c;
}
} // namespace
std::string PracticeState::serialize() const {
    std::ostringstream out;
    out << "PSTATE 1\nSEED " << seed_.device << ' ' << seed_.version << ' ' << seed_.station << ' '
        << seed_.input << ' ' << seed_.target << ' ' << seed_.law.size();
    for (const auto& op : seed_.law)
        out << ' ' << static_cast<unsigned>(op.kind) << ' ' << op.parameter;
    out << "\nRESEARCHERS " << researchers_.size();
    for (auto id : researchers_)
        out << ' ' << id.value;
    out << "\nAPPARATUS " << std::quoted(apparatus_.serialize());
    out << "\nPULSES " << pulses_.size();
    for (const auto& p : pulses_) {
        out << "\nPULSE " << p.actor.value << ' ' << p.input << ' ' << p.tick << ' ' << p.learning
            << ' ';
        chain(out, p.chain);
    }
    out << "\nPENDING " << pending_.has_value();
    if (pending_)
        out << ' ' << pending_->actor << ' ' << pending_->input << ' ' << pending_->stage << ' '
            << pending_->command << ' ' << pending_->learning;
    out << "\nPERMIT " << permit_.has_value();
    if (permit_)
        out << ' ' << permit_->actor.value << ' ' << permit_->measurement << ' ' << permit_->tick;
    out << "\nWORKING " << working_.has_value();
    if (working_)
        out << ' ' << working_->actor << ' ' << working_->action.value;
    out << "\nUSES " << uses_.size();
    for (const auto& u : uses_)
        out << "\nUSE " << u.measurement << ' ' << u.action.value;
    out << "\nMINDS " << minds_.size();
    for (const auto& m : minds_) {
        out << "\nMIND " << m.actor.value << ' ' << m.probes << ' ' << m.spent << ' '
            << m.method.has_value();
        if (m.method) {
            out << ' ';
            chain(out, *m.method);
        }
        out << ' ' << m.journal.size();
        for (const auto& e : m.journal)
            out << "\nEVENT " << std::quoted(e);
        out << "\nSTATE " << std::quoted(m.agent->state());
    }
    out << "\nCOMMAND " << command_ << '\n';
    return out.str();
}
PracticeState PracticeState::deserialize(std::string_view bytes, const TraversalSession& session) {
    std::istringstream in{std::string(bytes)};
    tag(in, "PSTATE");
    check(number(in, 1) == 1);
    tag(in, "SEED");
    PracticeSeed seed;
    seed.device = static_cast<std::uint32_t>(number(in, UINT32_MAX));
    seed.version = static_cast<std::uint32_t>(number(in, UINT32_MAX));
    seed.station = number(in, 15);
    seed.input = static_cast<unsigned>(number(in, 128));
    seed.target = static_cast<unsigned>(number(in, 128));
    seed.law.clear();
    auto count = number(in, 3);
    check(count > 0);
    while (count--)
        seed.law.push_back(
            {static_cast<HypothesisKind>(number(in, 3)), static_cast<unsigned>(number(in, 128))});
    tag(in, "RESEARCHERS");
    seed.researchers.clear();
    count = number(in, 32);
    while (count--) {
        const EntityId id{number(in)};
        const auto& a = session.actors();
        auto it = std::find_if(a.begin(), a.end(), [&](const auto& v) { return v.id == id; });
        check(it != a.end());
        seed.researchers.push_back(static_cast<std::size_t>(it - a.begin()));
    }
    PracticeState p{seed, session};
    tag(in, "APPARATUS");
    const auto apparatus = quoted(in);
    auto replay = p.apparatus_.fresh();
    const auto now = session.world().clock().tick_count();
    tag(in, "PULSES");
    count = number(in, 2048);
    while (count--) {
        tag(in, "PULSE");
        Pulse pulse;
        pulse.actor = {number(in)};
        pulse.input = static_cast<unsigned>(number(in, 128));
        pulse.tick = number(in);
        pulse.learning = number(in, 1) != 0;
        pulse.chain = chain(in);
        check(pulse.input > 0 && pulse.tick <= now && pulse.tick >= replay.tick());
        check(std::any_of(p.minds_.begin(), p.minds_.end(),
                          [&](const auto& m) { return m.actor == pulse.actor; }));
        check(!pulse.learning || pulse.chain == DiscoveryChain{0});
        replay.advance(pulse.tick);
        check(replay.configure(p.command_++, pulse.actor, 0, pulse.chain) ==
              DiscoveryOutcome::Accepted);
        if (pulse.tick < now) {
            replay.advance(pulse.tick + 1);
            check(replay.pulse(p.command_, pulse.actor, 0, pulse.input) ==
                  DiscoveryOutcome::Accepted);
        }
        ++p.command_;
        p.pulses_.push_back(std::move(pulse));
    }
    replay.advance(now);
    check(replay.serialize() == apparatus);
    p.apparatus_ = std::move(replay);
    tag(in, "PENDING");
    if (number(in, 1))
        p.pending_ = Pending{static_cast<std::size_t>(number(in, 31)),
                             static_cast<unsigned>(number(in, 128)),
                             static_cast<unsigned>(number(in, 1)), number(in), number(in, 1) != 0};
    const auto complete = p.apparatus_.measurements().size();
    check(complete + (p.pending_ ? 1 : 0) == p.pulses_.size());
    if (p.pending_) {
        const auto& r = *p.pending_;
        const auto& last = p.pulses_.back();
        check(r.actor < p.minds_.size() && p.minds_[r.actor].actor == last.actor &&
              r.input == last.input && r.learning == last.learning && r.command == p.command_ - 1 &&
              now == last.tick + r.stage);
    }
    tag(in, "PERMIT");
    if (number(in, 1))
        p.permit_ = Permit{{number(in)}, number(in), number(in)};
    tag(in, "WORKING");
    if (number(in, 1))
        p.working_ = Working{static_cast<std::size_t>(number(in, 31)), {number(in)}};
    tag(in, "USES");
    count = number(in, 2048);
    std::set<std::uint64_t> used, actions;
    const auto& measurements = p.apparatus_.measurements();
    auto measurement = [&](std::uint64_t id) -> const DiscoveryMeasurement& {
        auto it = std::find_if(measurements.begin(), measurements.end(),
                               [&](const auto& m) { return m.id == id; });
        check(it != measurements.end());
        return *it;
    };
    while (count--) {
        tag(in, "USE");
        Use use{number(in), {number(in)}};
        check(used.insert(use.measurement).second && actions.insert(use.action.value).second);
        const auto& m = measurement(use.measurement);
        auto action = session.world().action(use.action);
        check(action && action->request.actor == m.source_actor &&
              action->request.lot == session.stations()[seed.station].lot &&
              action->request.duration_ticks == 80 && action->request.consume_on_start.value == 1 &&
              action->consumed.value == 1 && action->started_at >= m.observed_tick &&
              m.input == seed.input && m.output == seed.target);
        p.uses_.push_back(use);
    }
    for (const auto& c : session.world().commands())
        if (c.result.status == RequestStatus::Accepted &&
            c.request.lot == session.stations()[seed.station].lot)
            check(actions.contains(c.result.action.value));
    if (p.permit_) {
        const auto& r = *p.permit_;
        const auto& m = measurement(r.measurement);
        check(!p.pending_ && !p.working_ && !measurements.empty() && m == measurements.back() &&
              m.source_actor == r.actor && m.observed_tick == r.tick && m.input == seed.input &&
              m.output == seed.target && !used.contains(m.id));
    }
    if (p.working_) {
        const auto& w = *p.working_;
        auto a = session.world().action(w.action);
        check(!p.pending_ && w.actor < p.minds_.size() && a && a->state == ActionState::Running &&
              a->request.actor == p.minds_[w.actor].actor && actions.contains(w.action.value) &&
              session.actors()[w.actor].action == w.action);
    }
    tag(in, "MINDS");
    check(number(in, 32) == p.minds_.size());
    for (auto& m : p.minds_) {
        tag(in, "MIND");
        check(number(in) == m.actor.value);
        m.probes = static_cast<unsigned>(number(in, 8));
        m.spent = static_cast<unsigned>(number(in, 1024));
        if (number(in, 1))
            m.method = chain(in);
        count = number(in, 128);
        std::set<std::uint64_t> observations;
        while (count--) {
            tag(in, "EVENT");
            auto event = quoted(in);
            std::istringstream e{event};
            std::string op;
            e >> op;
            check(op == "o");
            const auto id = number(e);
            const auto& obs = measurement(id);
            const auto index = static_cast<std::size_t>(id / 2 - 1);
            check(index < p.pulses_.size() && p.pulses_[index].learning &&
                  obs.source_actor == m.actor && observations.insert(id).second);
            check(m.edit().observe({obs, obs.source_actor, obs.observed_tick}));
            std::string extra;
            check(!(e >> extra));
            m.journal.push_back(std::move(event));
        }
        unsigned probes = 0, spent = 0;
        std::size_t completed = 0;
        for (std::size_t i = 0; i < p.pulses_.size(); ++i)
            if (p.pulses_[i].actor == m.actor && p.pulses_[i].learning) {
                ++probes;
                spent += p.pulses_[i].input;
                if (i < complete)
                    ++completed;
            }
        check(probes == m.probes && spent == m.spent && completed == observations.size() &&
              !m.method);
        tag(in, "STATE");
        check(quoted(in) == m.agent->state());
    }
    tag(in, "COMMAND");
    check(number(in) == p.command_);
    std::string extra;
    check(!(in >> extra));
    return p;
}
} // namespace reality
