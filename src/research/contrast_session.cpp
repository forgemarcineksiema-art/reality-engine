#include "research/contrast_session.hpp"
#include "research/discovery_random.hpp"
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
namespace reality {
namespace {
void check(bool c, const char* message) {
    if (!c)
        throw std::invalid_argument(message);
}
SurfaceLocation bench(unsigned actor) { return {1, {actor == 0 ? -4. : 4., 0}}; }
TraversalScene scene() {
    auto result = make_discovery_scene();
    for (unsigned i = 0; i < 2; ++i)
        result.stations.push_back({bench(i), TraversalTask::Maintain, 4, {}, 500, 0, {2}});
    return result;
}
std::string chain_text(const DiscoveryChain& chain) {
    std::ostringstream out;
    out << chain.size();
    for (auto symbol : chain)
        out << ' ' << static_cast<unsigned>(symbol);
    return out.str();
}
std::uint64_t num(std::istream& in, std::uint64_t maximum = UINT64_MAX) {
    std::string token;
    check(static_cast<bool>(in >> token), "missing contrast integer");
    std::uint64_t n = 0;
    const auto p = std::from_chars(token.data(), token.data() + token.size(), n);
    check(p.ec == std::errc{} && p.ptr == token.data() + token.size() && n <= maximum,
          "contrast integer range");
    return n;
}
std::string quoted(std::istream& in) {
    std::string s;
    check(static_cast<bool>(in >> std::quoted(s)), "missing contrast section");
    return s;
}
DiscoveryChain read_chain(std::istream& in) {
    DiscoveryChain c;
    const auto n = num(in, 3);
    check(n != 0, "empty contrast chain");
    for (unsigned i = 0; i < n; ++i)
        c.push_back(static_cast<std::uint8_t>(num(in, 2)));
    return c;
}
} // namespace
ContrastSession::ContrastSession(HypothesisLaw law, bool active, std::uint32_t seed,
                                 HypothesisLimits limits)
    : law_(std::move(law)), active_(active), seed_(seed), limits_(limits), traversal_(scene()),
      apparatus_(std::array{law_.table(), hypothesis_table({{HypothesisKind::Fraction, 8}}),
                            hypothesis_table({{HypothesisKind::Cap, 48}})}),
      agents_{ContrastAgent{traversal_.actors()[0].id, active, limits},
              ContrastAgent{traversal_.actors()[1].id, true, limits}} {
    permutation_.resize(128);
    std::iota(permutation_.begin(), permutation_.end(), 1U);
    for (unsigned n = 128; n > 1; --n) {
        const auto j = discovery_draw(seed_, 61, random_counter_) % n;
        std::swap(permutation_[n - 1], permutation_[j]);
    }
}
std::uint64_t ContrastSession::tick() const noexcept {
    return traversal_.world().clock().tick_count();
}
void ContrastSession::room(unsigned entries) const {
    check(events_.size() + entries <= 4096, "contrast journal full");
}
void ContrastSession::ready() const {
    check(!pending_, "contrast operation during impulse");
    const bool working = std::any_of(traversal_.actors().begin(), traversal_.actors().end(),
                                     [](const auto& a) { return a.action.value != 0; });
    room(working ? 2U : 1U);
}
bool ContrastSession::at_bench(unsigned actor) const {
    const auto& a = traversal_.actors().at(actor);
    return !a.action.value && a.transfer == TransferStage::None && a.motion.speed <= .03 &&
           (a.state == TraversalState::Idle || a.state == TraversalState::Arrived) &&
           traversal_.can_interact_at(actor, bench(actor));
}
ContrastChoice ContrastSession::choose(unsigned actor) {
    ready();
    auto result = agents_.at(actor).choose(permutation_);
    events_.push_back("c " + std::to_string(actor));
    return result;
}
std::optional<DiscoveryPrediction>
ContrastSession::predict(unsigned actor, const DiscoveryChain& chain, unsigned input) {
    ready();
    auto result = agents_.at(actor).predict(chain, input);
    events_.push_back("p " + std::to_string(actor) + " " + std::to_string(input) + " " +
                      chain_text(chain));
    return result;
}
std::optional<DiscoveryChain> ContrastSession::choose_chain(unsigned actor, unsigned input,
                                                            unsigned target) {
    ready();
    auto result = agents_.at(actor).choose_chain(input, target);
    events_.push_back("q " + std::to_string(actor) + " " + std::to_string(input) + " " +
                      std::to_string(target));
    return result;
}
DiscoveryOutcome ContrastSession::begin_probe(unsigned actor, DiscoveryChain chain, unsigned input,
                                              bool learning) {
    return begin(actor, std::move(chain), input, learning, 0);
}
DiscoveryOutcome ContrastSession::begin_work(unsigned actor, DiscoveryChain chain, unsigned input,
                                             unsigned target) {
    if (!target || target > input)
        return DiscoveryOutcome::Invalid;
    return begin(actor, std::move(chain), input, false, target);
}
DiscoveryOutcome ContrastSession::begin(unsigned actor, DiscoveryChain chain, unsigned input,
                                        bool learning, unsigned target) {
    if (actor > 1 || !discovery_chain_valid(chain) || !input || input > 128 ||
        std::find(chain.begin(), chain.end(), 0) == chain.end())
        return DiscoveryOutcome::Invalid;
    if (pending_)
        return DiscoveryOutcome::Busy;
    if (!at_bench(actor))
        return DiscoveryOutcome::OutOfReach;
    if (learning && agents_[actor].frozen())
        return DiscoveryOutcome::Frozen;
    if (!learning && !agents_[actor].frozen())
        return DiscoveryOutcome::Invalid;
    if (tick() + (target ? 82U : 2U) > 10000)
        return DiscoveryOutcome::Invalid;
    if (learning) {
        if (chain != DiscoveryChain{0} || attempts_[actor] >= (actor == 0 ? 8U : 2U) ||
            spent_[actor] + input > (actor == 0 ? 1024U : 256U) ||
            (start_[actor] && tick() + 2 - *start_[actor] > 1000))
            return DiscoveryOutcome::Invalid;
        for (const auto& e : agents_[actor].evidence())
            if (e.measurement.input == input)
                return DiscoveryOutcome::Invalid;
    }
    if (apparatus_.devices()[actor].source < input)
        return DiscoveryOutcome::NoEnergy;
    room(3); // begin, a split configure completion, and the remaining pulse completion
    const auto result = apparatus_.configure(command_, traversal_.actors()[actor].id, actor, chain);
    if (result != DiscoveryOutcome::Accepted)
        return result;
    ++command_;
    pending_ = Pending{actor, input, 0, target, command_++, learning};
    if (learning) {
        if (!start_[actor])
            start_[actor] = tick();
        ++attempts_[actor];
        spent_[actor] += input;
    }
    work_started_ = false;
    events_.push_back("b " + std::to_string(actor) + " " + std::to_string(input) + " " +
                      std::to_string(learning) + " " + std::to_string(target) + " " +
                      chain_text(chain));
    return DiscoveryOutcome::Accepted;
}
void ContrastSession::advance(std::uint64_t ticks) {
    check(ticks > 0 && ticks <= 10000 && tick() + ticks <= 10000, "contrast advance limit");
    bool remains = pending_ && ticks < 2 - pending_->stage;
    if (pending_ && pending_->target && ticks < 82 - pending_->stage)
        remains = true;
    for (const auto& a : traversal_.actors())
        if (a.action.value && traversal_.world().action(a.action)->due_at > tick() + ticks)
            remains = true;
    room(remains ? 2U : 1U);
    for (std::uint64_t i = 0; i < ticks; ++i) {
        traversal_.advance(1);
        apparatus_.advance(tick());
        if (pending_ && pending_->stage == 0) {
            check(at_bench(pending_->actor), "lost observer during configuration");
            check(apparatus_.pulse(pending_->command, traversal_.actors()[pending_->actor].id,
                                   pending_->actor, pending_->input) == DiscoveryOutcome::Accepted,
                  "configured contrast pulse failed");
            pending_->stage = 1;
        } else if (pending_) {
            const auto p = *pending_;
            check(at_bench(p.actor), "lost observer during impulse");
            const auto& m = apparatus_.measurements().back();
            check(m.command == p.command, "contrast measurement identity");
            if (p.learning)
                check(agents_[p.actor].observe({m, m.source_actor, m.observed_tick}),
                      "duplicate contrast observation");
            if (p.target && m.output == p.target)
                work_started_ = traversal_.begin_maintenance(p.actor, p.actor);
            pending_.reset();
        }
        apparatus_.validate();
        traversal_.validate();
    }
    events_.push_back("a " + std::to_string(ticks));
}
void ContrastSession::freeze(unsigned actor) {
    ready();
    agents_.at(actor).freeze();
    events_.push_back("f " + std::to_string(actor));
}
void ContrastSession::set_goal(unsigned actor, SurfaceLocation goal) {
    ready();
    check(!traversal_.actors().at(actor).action.value, "cannot move worker");
    traversal_.set_goal(actor, goal);
    std::ostringstream out;
    out << std::setprecision(17) << "g " << actor << ' ' << goal.surface << ' ' << goal.local.x_m
        << ' ' << goal.local.z_m;
    events_.push_back(out.str());
}
void ContrastSession::stop(unsigned actor) {
    ready();
    check(!traversal_.actors().at(actor).action.value, "cannot stop worker");
    traversal_.stop(actor);
    events_.push_back("t " + std::to_string(actor));
}
DiscoveryOutcome ContrastSession::share(bool message) {
    if (pending_)
        return DiscoveryOutcome::Busy;
    if (!traversal_.can_exchange_knowledge(0, 1))
        return DiscoveryOutcome::OutOfReach;
    if (agents_[1].frozen())
        return DiscoveryOutcome::Frozen;
    ready();
    room(2);
    auto recipient = agents_[1];
    if (message)
        for (auto e : agents_[0].evidence()) {
            const auto& measurements = apparatus_.measurements();
            check(std::find(measurements.begin(), measurements.end(), e.measurement) !=
                          measurements.end() &&
                      e.measurement.source_actor == traversal_.actors()[0].id,
                  "invented contrast packet");
            e.via_actor = traversal_.actors()[0].id;
            e.learned_tick = tick() + 1;
            static_cast<void>(recipient.observe(e, true));
        }
    advance(1);
    events_.pop_back(); // share is one atomic journal operation, including its time
    contact_tick_ = tick();
    agents_[1] = std::move(recipient);
    events_.push_back("s " + std::to_string(message));
    return DiscoveryOutcome::Accepted;
}
DiscoveryOutcome ContrastSession::meet_and_share(bool message) {
    ready();
    const auto began = tick();
    set_goal(0, {1, {2.5, 0}});
    while (!traversal_.can_exchange_knowledge(0, 1) && tick() - began < 1000)
        advance(1);
    if (!traversal_.can_exchange_knowledge(0, 1))
        return DiscoveryOutcome::OutOfReach;
    stop(0);
    const auto result = share(message);
    set_goal(0, bench(0));
    while (!at_bench(0) && tick() - began < 1000)
        advance(1);
    check(at_bench(0), "teacher failed to return from contact");
    stop(0);
    return result;
}
std::uint64_t ContrastSession::parts(unsigned actor) const {
    return traversal_.world().resource(traversal_.stations().at(actor).lot)->record.quantity.value;
}
std::uint64_t ContrastSession::completed(unsigned actor) const {
    return traversal_.stations().at(actor).completed;
}
std::string ContrastSession::canonical() const {
    std::ostringstream out;
    out << command_ << ' ' << random_counter_ << ' ' << work_started_ << ' ' << contact_tick_
        << '\n';
    for (unsigned i = 0; i < 2; ++i)
        out << attempts_[i] << ' ' << spent_[i] << ' ' << start_[i].has_value() << ' '
            << start_[i].value_or(0) << '\n';
    out << pending_.has_value();
    if (pending_)
        out << ' ' << pending_->actor << ' ' << pending_->input << ' ' << pending_->stage << ' '
            << pending_->target << ' ' << pending_->command << ' ' << pending_->learning;
    out << '\n'
        << std::quoted(traversal_.serialize()) << '\n'
        << std::quoted(apparatus_.serialize()) << '\n';
    for (const auto& a : agents_)
        out << std::quoted(a.state()) << '\n';
    for (auto x : permutation_)
        out << x << ' ';
    return out.str();
}
std::string ContrastSession::serialize() const {
    std::ostringstream out;
    out << active_ << ' ' << seed_ << ' ' << limits_.primitives << ' ' << limits_.reads << ' '
        << law_.square_divisor << ' ' << law_.program.size();
    for (auto op : law_.program)
        out << ' ' << static_cast<unsigned>(op.kind) << ' ' << op.parameter;
    out << '\n' << events_.size() << '\n';
    for (const auto& e : events_)
        out << std::quoted(e) << '\n';
    out << std::quoted(canonical()) << '\n';
    const auto body = out.str();
    check(body.size() + 64 < snapshot_byte_limit, "contrast snapshot too large");
    return "GENESIS_CONTRAST 1 " + std::to_string(snapshot_checksum(body)) + "\n" + body;
}
ContrastSession ContrastSession::deserialize(std::string_view bytes) {
    check(bytes.size() < snapshot_byte_limit, "contrast snapshot too large");
    const auto newline = bytes.find('\n');
    check(newline != std::string_view::npos, "missing contrast header");
    std::istringstream header{std::string(bytes.substr(0, newline))};
    std::string magic;
    header >> magic;
    check(magic == "GENESIS_CONTRAST" && num(header, 1) == 1, "contrast version");
    const auto hash = num(header);
    header >> std::ws;
    const auto body = bytes.substr(newline + 1);
    check(header.eof() && hash == snapshot_checksum(body), "contrast checksum");
    std::istringstream in{std::string(body)};
    const auto active = num(in, 1) != 0;
    const auto seed = static_cast<std::uint32_t>(num(in, UINT32_MAX));
    const HypothesisLimits limits{num(in, 10000000), num(in, 300000000)};
    HypothesisLaw law;
    law.square_divisor = static_cast<unsigned>(num(in, 256));
    const auto size = num(in, 3);
    for (unsigned i = 0; i < size; ++i)
        law.program.push_back(
            {static_cast<HypothesisKind>(num(in, 3)), static_cast<unsigned>(num(in, 64))});
    ContrastSession result{law, active, seed, limits};
    const auto count = num(in, 4096);
    for (unsigned i = 0; i < count; ++i) {
        const auto e = quoted(in);
        std::istringstream args{e};
        char op = 0;
        args >> op;
        if (op == 'a')
            result.advance(num(args, 10000));
        else if (op == 's')
            check(result.share(num(args, 1) != 0) == DiscoveryOutcome::Accepted,
                  "invalid contrast share");
        else {
            const auto actor = static_cast<unsigned>(num(args, 1));
            switch (op) {
            case 'c':
                static_cast<void>(result.choose(actor));
                break;
            case 'f':
                result.freeze(actor);
                break;
            case 't':
                result.stop(actor);
                break;
            case 'p': {
                const auto x = static_cast<unsigned>(num(args, 128));
                static_cast<void>(result.predict(actor, read_chain(args), x));
                break;
            }
            case 'q': {
                const auto x = static_cast<unsigned>(num(args, 128));
                const auto y = static_cast<unsigned>(num(args, 128));
                static_cast<void>(result.choose_chain(actor, x, y));
                break;
            }
            case 'b': {
                const auto x = static_cast<unsigned>(num(args, 128));
                const auto learning = num(args, 1) != 0;
                const auto target = static_cast<unsigned>(num(args, 128));
                check(!learning || target == 0, "learning work conflict");
                check(result.begin(actor, read_chain(args), x, learning, target) ==
                          DiscoveryOutcome::Accepted,
                      "invalid contrast pulse");
                break;
            }
            case 'g': {
                SurfaceLocation goal;
                goal.surface = static_cast<std::uint32_t>(num(args, UINT32_MAX));
                check(static_cast<bool>(args >> goal.local.x_m >> goal.local.z_m), "contrast goal");
                result.set_goal(actor, goal);
                break;
            }
            default:
                throw std::invalid_argument("contrast event kind");
            }
        }
        args >> std::ws;
        check(args.eof() && result.events_.back() == e, "contrast event mismatch");
    }
    const auto state = quoted(in);
    in >> std::ws;
    check(in.eof() && result.canonical() == state && result.serialize() == bytes,
          "contrast semantic mismatch");
    return result;
}
void ContrastSession::restore(std::string_view bytes) {
    auto candidate = deserialize(bytes);
    *this = std::move(candidate);
}
} // namespace reality
