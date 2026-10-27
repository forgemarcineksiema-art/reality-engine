#include "research/discovery_session.hpp"
#include "research/discovery_random.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
namespace reality {
namespace {
void check(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
SurfaceLocation bench(unsigned actor) { return {1, {actor == 0 ? -4. : 4., 0}}; }
} // namespace
TraversalScene make_discovery_scene() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {}, 0, 8, 4, .3, 0, 0, true}};
    scene.spawns = {{{1, {-4, 0}}, {}, false}, {{1, {4, 0}}, {}, false}};
    return scene;
}
DiscoverySession::DiscoverySession(std::array<DiscoveryRule, 3> rules, TraversalScene scene,
                                   std::uint32_t seed)
    : traversal_(std::move(scene)), apparatus_(rules),
      agents_{DiscoveryAgent{traversal_.actors().at(0).id},
              DiscoveryAgent{traversal_.actors().at(1).id}},
      seed_(seed) {
    check(traversal_.actors().size() == 2, "discovery requires exactly two actors");
    validate();
}
const TraversalSession& DiscoverySession::traversal() const noexcept { return traversal_; }
const DiscoveryWorld& DiscoverySession::apparatus() const noexcept { return apparatus_; }
const DiscoveryAgent& DiscoverySession::agent(unsigned actor) const { return agents_.at(actor); }
const std::vector<DiscoveryTrace>& DiscoverySession::traces() const noexcept { return traces_; }
const std::vector<DiscoveryMovement>& DiscoverySession::movements() const noexcept {
    return movements_;
}
std::uint64_t DiscoverySession::tick() const noexcept {
    return traversal_.world().clock().tick_count();
}
unsigned DiscoverySession::training_attempts(unsigned actor) const { return attempts_.at(actor); }
bool DiscoverySession::at_bench(unsigned actor) const {
    const auto& person = traversal_.actors().at(actor);
    return traversal_.can_interact_at(actor, bench(actor)) &&
           person.transfer == TransferStage::None && !person.action.value &&
           (person.state == TraversalState::Idle || person.state == TraversalState::Arrived);
}
void DiscoverySession::set_goal(unsigned actor, SurfaceLocation location) {
    check(actor < 2 && (!pending_ || pending_->actor != actor), "active probe cannot move");
    traversal_.set_goal(actor, location);
}
void DiscoverySession::advance(std::uint64_t ticks) {
    check(ticks <= 10000, "discovery advance limit");
    for (std::uint64_t i = 0; i < ticks; ++i) {
        const std::array before{traversal_.actors()[0].feet, traversal_.actors()[1].feet};
        traversal_.advance(1);
        for (unsigned actor = 0; actor < 2; ++actor)
            if (length(traversal_.actors()[actor].feet - before[actor]) > 1e-12) {
                check(movements_.size() < 4096, "discovery diagnostic path limit");
                movements_.push_back({tick(), actor, traversal_.actors()[actor].support.surface,
                                      traversal_.actors()[actor].feet});
            }
        apparatus_.advance(tick());
        if (pending_ && pending_->stage == 0) {
            auto& pending = *pending_;
            check(at_bench(pending.actor), "actor left configured apparatus");
            check(apparatus_.pulse(pending.command, agents_[pending.actor].actor(), pending.actor,
                                   pending.input) == DiscoveryOutcome::Accepted,
                  "configured impulse failed");
            pending.stage = 1;
        } else if (pending_) {
            const auto pending = *pending_;
            const auto* result = measurement(pending.command);
            check(result && at_bench(pending.actor), "completed impulse lost observer");
            if (pending.learning)
                check(agents_[pending.actor].observe(
                          {*result, result->source_actor, result->observed_tick}),
                      "impulse delivered duplicate observation");
            pending_.reset();
        }
        validate();
    }
}
DiscoveryOutcome DiscoverySession::begin_probe(unsigned actor, DiscoveryChain chain,
                                               DiscoveryEnergy input, bool learning) {
    if (actor >= 2 || !discovery_chain_valid(chain) || input == 0 || input > 8192)
        return DiscoveryOutcome::Invalid;
    if (pending_)
        return DiscoveryOutcome::Busy;
    if (!at_bench(actor))
        return DiscoveryOutcome::OutOfReach;
    auto& mind = agents_[actor];
    if (learning && mind.frozen())
        return DiscoveryOutcome::Frozen;
    if (!learning && !mind.frozen())
        return DiscoveryOutcome::Invalid;
    if (learning && (chain.size() != 1 || (input != 16 && input != 32 && input != 64)))
        return DiscoveryOutcome::Invalid;
    const auto limit = actor == 0 ? 24U : 2U;
    const auto energy_limit = actor == 0 ? 1536U : 128U;
    if (learning && (attempts_[actor] >= limit || spent_[actor] + input > energy_limit ||
                     (learning_start_[actor] && tick() - *learning_start_[actor] + 2 > 1000)))
        return DiscoveryOutcome::Invalid;
    if (apparatus_.devices()[actor].source < input)
        return DiscoveryOutcome::NoEnergy;
    if (next_command_ > std::numeric_limits<std::uint64_t>::max() - 2 || traces_.size() >= 256)
        return DiscoveryOutcome::Invalid;
    DiscoveryTrace trace{actor, next_command_ + 1, tick(), chain, input, {}, learning};
    if (!mind.inadequate())
        trace.prediction = mind.predict(chain, input);
    traces_.reserve(traces_.size() + 1);
    const auto configured =
        apparatus_.configure(next_command_, mind.actor(), actor, std::move(chain));
    if (configured != DiscoveryOutcome::Accepted)
        return configured;
    pending_ = Pending{actor, 0, next_command_ + 1, input, learning};
    traces_.push_back(std::move(trace));
    next_command_ += 2;
    if (learning) {
        if (!learning_start_[actor])
            learning_start_[actor] = tick();
        ++attempts_[actor];
        spent_[actor] += input;
    }
    return DiscoveryOutcome::Accepted;
}
std::optional<DiscoveryMeasurement> DiscoverySession::probe(unsigned actor, DiscoveryChain chain,
                                                            DiscoveryEnergy input, bool learning) {
    if (begin_probe(actor, std::move(chain), input, learning) != DiscoveryOutcome::Accepted)
        return {};
    const auto command = pending_->command;
    advance(2);
    return *measurement(command);
}
void DiscoverySession::train(unsigned actor, bool random) {
    check(actor < 2 && !pending_, "invalid training phase");
    phase_ = Phase::Learning;
    if (!learning_start_[actor])
        learning_start_[actor] = tick();
    if (!at_bench(actor)) {
        set_goal(actor, bench(actor));
        while (!at_bench(actor) && tick() - *learning_start_[actor] < 998)
            advance(1);
    }
    while (!agents_[actor].frozen() && !agents_[actor].inadequate()) {
        std::optional<DiscoveryProbe> choice;
        if (random) {
            const auto index = discovery_draw(seed_, 5, random_counters_[actor]) % 9;
            choice = DiscoveryProbe{static_cast<unsigned>(index % 3),
                                    std::array{16U, 32U, 64U}[index / 3]};
        } else
            choice = agents_[actor].choose_probe();
        if (!choice || !probe(actor, {static_cast<std::uint8_t>(choice->symbol)}, choice->input))
            break;
    }
    agents_[actor].freeze();
}
void DiscoverySession::freeze(unsigned actor) {
    check(actor < 2 && (!pending_ || pending_->actor != actor), "freeze during observation");
    agents_.at(actor).freeze();
}
DiscoveryOutcome DiscoverySession::share(unsigned sender, unsigned recipient, bool message) {
    if (sender >= 2 || recipient >= 2 || sender == recipient)
        return DiscoveryOutcome::Invalid;
    if (pending_)
        return DiscoveryOutcome::Busy;
    if (!traversal_.can_exchange_knowledge(sender, recipient))
        return DiscoveryOutcome::OutOfReach;
    if (agents_[recipient].frozen())
        return DiscoveryOutcome::Frozen;
    if (exchanges_.size() >= 8)
        return DiscoveryOutcome::Invalid;
    const auto packet =
        message ? agents_[sender].teaching_packet() : std::vector<DiscoveryEvidence>{};
    auto recipient_mind = agents_[recipient];
    Exchange exchange{sender, recipient, tick() + 1, {}, traversal_.serialize()};
    for (auto e : packet) {
        const auto* actual = measurement(e.measurement.id);
        check(actual && *actual == e.measurement && actual->source_actor == agents_[sender].actor(),
              "packet contains invented/foreign data");
        e.via_actor = agents_[sender].actor();
        e.learned_tick = exchange.tick;
        static_cast<void>(recipient_mind.observe(e));
        exchange.records.push_back(e.measurement.id);
    }
    exchanges_.reserve(exchanges_.size() + 1);
    advance(1);
    agents_[recipient] = std::move(recipient_mind);
    exchanges_.push_back(std::move(exchange));
    phase_ = Phase::Contact;
    validate();
    return DiscoveryOutcome::Accepted;
}
DiscoveryOutcome DiscoverySession::meet_and_share(bool message) {
    const auto start = tick();
    if (!traversal_.can_exchange_knowledge(0, 1)) {
        const auto& person = traversal_.actors()[1];
        auto destination = person.support;
        destination.local.x_m -= 1.5;
        set_goal(0, destination);
        while (!traversal_.can_exchange_knowledge(0, 1) && tick() - start < 1000)
            advance(1);
        if (!traversal_.can_exchange_knowledge(0, 1))
            return DiscoveryOutcome::OutOfReach;
        traversal_.stop(0);
    }
    return share(0, 1, message);
}
DiscoverySession DiscoverySession::evaluation_copy() const {
    check(!pending_ && archives_.size() < 8, "cannot branch inflight/evaluation archive limit");
    auto copy = *this;
    copy.archives_.push_back(copy.apparatus_);
    copy.apparatus_ = apparatus_.fresh();
    copy.apparatus_.advance(copy.tick());
    copy.phase_ = Phase::Evaluation;
    copy.validate();
    return copy;
}
const DiscoveryMeasurement* DiscoverySession::measurement(std::uint64_t id) const {
    for (const auto& m : apparatus_.measurements())
        if (m.id == id)
            return &m;
    for (const auto& archive : archives_)
        for (const auto& m : archive.measurements())
            if (m.id == id)
                return &m;
    return nullptr;
}
void DiscoverySession::validate() const {
    traversal_.validate();
    apparatus_.validate();
    check(apparatus_.tick() == tick() && traversal_.actors().size() == 2 && archives_.size() <= 8,
          "discovery clock/population/archive mismatch");
    std::set<std::uint64_t> identifiers;
    const auto check_world = [&](const DiscoveryWorld& world) {
        check(world.same_rules(apparatus_) && world.tick() <= tick(),
              "inconsistent archived world");
        for (const auto& m : world.measurements())
            check(
                identifiers.insert(m.id).second && m.command < next_command_ &&
                    (m.source_actor == agents_[0].actor() || m.source_actor == agents_[1].actor()),
                "measurement identity/actor mismatch");
    };
    for (const auto& archive : archives_)
        check_world(archive);
    check_world(apparatus_);
    check(next_command_ == traces_.size() * 2 + 1 && traces_.size() <= 256 &&
              exchanges_.size() <= 8,
          "discovery session limits");
    for (unsigned actor = 0; actor < 2; ++actor) {
        check(agents_[actor].actor() == traversal_.actors()[actor].id &&
                  !traversal_.actors()[actor].autonomous &&
                  attempts_[actor] <= (actor == 0 ? 24U : 2U) &&
                  spent_[actor] <= (actor == 0 ? 1536U : 128U) &&
                  (!learning_start_[actor] || *learning_start_[actor] <= tick()) &&
                  random_counters_[actor] <= (1U << 24),
              "invalid research actor/budget");
        for (const auto& e : agents_[actor].evidence()) {
            const auto* actual = measurement(e.measurement.id);
            check(actual && *actual == e.measurement && e.learned_tick <= tick(),
                  "evidence has no actual measurement");
            if (actual->source_actor == agents_[actor].actor())
                check(e.via_actor == actual->source_actor &&
                          e.learned_tick == actual->observed_tick,
                      "invalid personal observation");
            else
                check(std::any_of(exchanges_.begin(), exchanges_.end(),
                                  [&](const auto& exchange) {
                                      return exchange.recipient == actor &&
                                             exchange.tick == e.learned_tick &&
                                             agents_[exchange.sender].actor() == e.via_actor &&
                                             std::find(exchange.records.begin(),
                                                       exchange.records.end(),
                                                       actual->id) != exchange.records.end();
                                  }),
                      "evidence received without exchange");
        }
    }
    if (pending_) {
        check(pending_->actor < 2 && pending_->stage <= 1 &&
                  pending_->command + 1 == next_command_ && !traces_.empty() &&
                  traces_.back().command == pending_->command,
              "invalid session pending impulse");
        const auto& p = *pending_;
        const auto& trace = traces_.back();
        const auto& device = apparatus_.devices()[p.actor];
        check(trace.actor == p.actor && trace.input == p.input && trace.learning == p.learning &&
                  trace.started_tick <= UINT64_MAX - 2 && tick() == trace.started_tick + p.stage &&
                  device.chain == trace.chain && device.ready_at == trace.started_tick + 1 &&
                  (p.learning ? !agents_[p.actor].frozen() : agents_[p.actor].frozen()),
              "pending impulse differs from configured trace/observer");
        const auto active = apparatus_.pending_pulse(p.actor);
        if (p.stage == 0)
            check(!active, "configuration stage already has a pulse");
        else
            check(active && active->command == p.command && active->input == p.input &&
                      active->actor == agents_[p.actor].actor() && active->started_tick == tick(),
                  "pending impulse differs from apparatus command");
    }
}
} // namespace reality
