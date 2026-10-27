#include "research/hypothesis_session.hpp"
#include "research/discovery_random.hpp"
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
namespace reality {
namespace {
std::array<HypothesisTable, 3> law_tables(const HypothesisLaw& law) {
    return {law.table(), hypothesis_table({{HypothesisKind::Fraction, 8}}),
            hypothesis_table({{HypothesisKind::Cap, 48}})};
}
void chain_out(std::ostream& out, const DiscoveryChain& chain) {
    out << ' ' << chain.size();
    for (const auto symbol : chain)
        out << ' ' << static_cast<unsigned>(symbol);
}
} // namespace
HypothesisTable HypothesisLaw::table() const {
    if (!square_divisor)
        return hypothesis_table(program);
    if (!program.empty() || (square_divisor != 128 && square_divisor != 160 &&
                             square_divisor != 192 && square_divisor != 256))
        throw std::invalid_argument("hypothesis square control");
    HypothesisTable result{};
    for (unsigned e = 0; e <= 128; ++e)
        result[e] = static_cast<std::uint16_t>(e * e / square_divisor);
    return result;
}
HypothesisSession::HypothesisSession(HypothesisLaw law, HypothesisMode mode, std::uint32_t seed,
                                     TraversalScene scene, HypothesisLimits limits)
    : law_(std::move(law)), mode_(mode), seed_(seed), limits_(limits), traversal_(std::move(scene)),
      apparatus_(law_tables(law_)), agent_(traversal_.actors().at(0).id, mode, limits) {
    if (traversal_.actors().size() != 2)
        throw std::invalid_argument("hypothesis requires two actors");
    initial_ = traversal_.serialize();
    permutation_.resize(64);
    std::iota(permutation_.begin(), permutation_.end(), 1U);
    for (std::size_t n = 64; n > 1; --n) {
        const auto index = discovery_draw(seed_, 41, random_counter_) % n;
        std::swap(permutation_[n - 1], permutation_[static_cast<std::size_t>(index)]);
    }
}
bool HypothesisSession::at_bench() const {
    const auto& actor = traversal_.actors()[0];
    return traversal_.can_interact_at(0, {1, {-4, 0}}) && actor.transfer == TransferStage::None &&
           !actor.action.value &&
           (actor.state == TraversalState::Idle || actor.state == TraversalState::Arrived);
}
void HypothesisSession::check_log(bool advancing, std::uint64_t ticks) const {
    unsigned reserved = pending_ ? 2 - pending_->stage : 0;
    if (advancing)
        reserved = ticks >= reserved ? 0 : reserved - static_cast<unsigned>(ticks);
    if (events_.size() >= record_limit - reserved)
        throw std::invalid_argument("hypothesis event limit");
}
DiscoveryOutcome HypothesisSession::begin_probe(DiscoveryChain chain, unsigned input,
                                                bool learning) {
    if (!discovery_chain_valid(chain) || !input || input > 128)
        return DiscoveryOutcome::Invalid;
    if (pending_)
        return DiscoveryOutcome::Busy;
    if (events_.size() > record_limit - 3)
        return DiscoveryOutcome::Invalid;
    check_log();
    if (learning && agent_.frozen())
        return DiscoveryOutcome::Frozen;
    if (learning == archive_.has_value() || (!learning && !agent_.frozen()))
        return DiscoveryOutcome::Invalid;
    if (!at_bench())
        return DiscoveryOutcome::OutOfReach;
    if (learning &&
        (chain != DiscoveryChain{0} || input > 64 || attempts_ >= 24 || spent_ + input > 1536 ||
         (learning_start_ && tick() - *learning_start_ + 2 > 1000)))
        return DiscoveryOutcome::Invalid;
    if (learning)
        for (const auto& e : agent_.evidence())
            if (e.measurement.input == input)
                return DiscoveryOutcome::Invalid;
    if (apparatus_.devices()[0].source < input)
        return DiscoveryOutcome::NoEnergy;
    if (next_command_ > UINT64_MAX - 2 || tick() > UINT64_MAX - 2)
        return DiscoveryOutcome::Invalid;
    const auto status = apparatus_.configure(next_command_, traversal_.actors()[0].id, 0, chain);
    if (status != DiscoveryOutcome::Accepted)
        return status;
    pending_ = Pending{0, input, next_command_ + 1, learning};
    next_command_ += 2;
    if (learning) {
        ++attempts_;
        spent_ += input;
        if (!learning_start_)
            learning_start_ = tick();
    }
    std::ostringstream event;
    event << "b " << learning << ' ' << input;
    chain_out(event, chain);
    events_.push_back(event.str());
    return status;
}
void HypothesisSession::advance(std::uint64_t ticks) {
    check_log(true, ticks);
    if (ticks > 10000 || ticks > UINT64_MAX - tick())
        throw std::invalid_argument("hypothesis advance bound");
    for (std::uint64_t n = 0; n < ticks; ++n) {
        traversal_.advance(1);
        apparatus_.advance(tick());
        if (pending_) {
            if (!at_bench())
                throw std::logic_error("hypothesis observer left bench");
            if (pending_->stage == 0) {
                if (apparatus_.pulse(pending_->command, traversal_.actors()[0].id, 0,
                                     pending_->input) != DiscoveryOutcome::Accepted)
                    throw std::logic_error("hypothesis reserved pulse refused");
                pending_->stage = 1;
            } else {
                const auto& m = apparatus_.measurements().back();
                if (m.command != pending_->command)
                    throw std::logic_error("hypothesis observation mismatch");
                if (pending_->learning && !agent_.observe({m, m.source_actor, m.observed_tick}))
                    throw std::logic_error("hypothesis observation refused");
                pending_.reset();
            }
        }
    }
    events_.push_back("a " + std::to_string(ticks));
}
void HypothesisSession::set_goal(SurfaceLocation goal) {
    check_log();
    if (pending_)
        throw std::invalid_argument("hypothesis active probe cannot move");
    traversal_.set_goal(0, goal);
    std::ostringstream event;
    event << std::setprecision(17) << "g " << goal.surface << ' ' << goal.local.x_m << ' '
          << goal.local.z_m;
    events_.push_back(event.str());
}
HypothesisChoice HypothesisSession::choose_input() {
    check_log();
    if (pending_ || agent_.frozen() || archive_ || attempts_ >= 24)
        throw std::invalid_argument("hypothesis input phase");
    HypothesisChoice choice;
    if (attempts_ < 3)
        choice.input = std::array{16U, 32U, 64U}[attempts_];
    else
        choice = agent_.choose_input(permutation_);
    std::ostringstream event;
    event << "i " << choice.input << ' ' << choice.active;
    for (const auto value : choice.worst)
        event << ' ' << value;
    for (const auto value : choice.partition)
        event << ' ' << value;
    events_.push_back(event.str());
    return choice;
}
std::optional<DiscoveryPrediction> HypothesisSession::predict(const DiscoveryChain& chain,
                                                              unsigned input) {
    check_log();
    const auto result = agent_.predict(chain, input);
    std::ostringstream event;
    event << "p " << input;
    chain_out(event, chain);
    event << ' ' << result.has_value();
    if (result)
        event << ' ' << result->median << ' ' << result->minimum << ' ' << result->maximum << ' '
              << result->combinations;
    events_.push_back(event.str());
    return result;
}
std::optional<DiscoveryChain> HypothesisSession::choose_chain(unsigned input, unsigned target) {
    check_log();
    if (!agent_.frozen() || pending_)
        throw std::invalid_argument("hypothesis planning before freeze");
    const auto chain = agent_.choose_chain(input, target);
    std::ostringstream event;
    event << "c " << input << ' ' << target << ' ' << chain.has_value();
    if (chain)
        chain_out(event, *chain);
    events_.push_back(event.str());
    return chain;
}
void HypothesisSession::freeze() {
    check_log();
    if (pending_)
        throw std::invalid_argument("hypothesis freeze during impulse");
    agent_.freeze();
    events_.push_back("f");
}
void HypothesisSession::enter_evaluation() {
    check_log();
    if (!agent_.frozen() || pending_ || archive_)
        throw std::invalid_argument("hypothesis evaluation phase");
    archive_ = apparatus_;
    apparatus_ = apparatus_.fresh();
    apparatus_.advance(tick());
    events_.push_back("e");
}
HypothesisSession HypothesisSession::evaluation_copy() const {
    auto result = *this;
    result.enter_evaluation();
    return result;
}
const HypothesisAgent& HypothesisSession::agent() const noexcept { return agent_; }
const DiscoveryWorld& HypothesisSession::apparatus() const noexcept { return apparatus_; }
const TraversalSession& HypothesisSession::traversal() const noexcept { return traversal_; }
std::uint64_t HypothesisSession::tick() const noexcept {
    return traversal_.world().clock().tick_count();
}
unsigned HypothesisSession::attempts() const noexcept { return attempts_; }
bool HypothesisSession::pending() const noexcept { return pending_.has_value(); }
} // namespace reality
