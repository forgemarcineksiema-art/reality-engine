#include "research/hypothesis_agent.hpp"
#include <algorithm>
#include <set>
#include <sstream>
#include <stdexcept>
namespace reality {
HypothesisAgent::HypothesisAgent(EntityId actor, HypothesisMode mode, HypothesisLimits limits)
    : actor_(actor), mode_(mode), limits_(limits) {
    if (!actor.value || static_cast<unsigned>(mode) > 2 || limits.primitives > 10000000 ||
        limits.reads > 300000000)
        throw std::invalid_argument("hypothesis agent parameters");
    build(hypothesis_flat_programs(), false);
}
bool HypothesisAgent::charge(std::uint64_t count, bool primitive) {
    auto& spent = primitive ? cost_.primitives : cost_.reads;
    const auto limit = primitive ? limits_.primitives : limits_.reads;
    if (exhausted_)
        return false;
    if (count > limit - spent) {
        exhausted_ = true;
        return false;
    }
    spent += count;
    return true;
}
void HypothesisAgent::build(std::vector<HypothesisProgram> programs, bool filter) {
    std::sort(programs.begin(), programs.end(), [](const auto& a, const auto& b) {
        return a.size() != b.size() ? a.size() < b.size() : a < b;
    });
    std::set<HypothesisTable> seen;
    models_.clear();
    for (auto& program : programs) {
        if (!charge(program.size() * 129, true))
            return;
        auto table = hypothesis_table(program);
        if (!seen.insert(table).second)
            continue;
        bool matches = true;
        if (filter)
            for (const auto& e : evidence_) {
                if (!charge(1))
                    return;
                if (table[e.measurement.input] != e.measurement.output) {
                    matches = false;
                    break;
                }
            }
        if (matches)
            models_.push_back({std::move(program), table});
    }
}
bool HypothesisAgent::observe(const DiscoveryEvidence& evidence) {
    if (frozen_)
        return false;
    const auto& m = evidence.measurement;
    if (!m.id || m.id != m.command || m.source_actor != actor_ || evidence.via_actor != actor_ ||
        m.input < 1 || m.input > 64 || m.output > m.input || m.chain != DiscoveryChain{0} ||
        !m.observed_tick || evidence.learned_tick != m.observed_tick || m.apparatus != 0)
        throw std::invalid_argument("hypothesis observation source/range");
    for (const auto& previous : evidence_) {
        if (previous.measurement.id == m.id) {
            if (previous != evidence)
                throw std::invalid_argument("conflicting hypothesis evidence");
            return false;
        }
        if (previous.measurement.input == m.input ||
            previous.measurement.observed_tick >= m.observed_tick)
            throw std::invalid_argument("hypothesis repeated or nonchronological input");
    }
    if (evidence_.size() >= 24)
        throw std::invalid_argument("hypothesis observation limit");
    evidence_.push_back(evidence);
    if (exhausted_)
        return true;
    std::vector<HypothesisModel> remaining;
    for (auto& model : models_) {
        if (!charge(1))
            return true;
        if (model.table[m.input] == m.output)
            remaining.push_back(model);
    }
    models_ = std::move(remaining);
    if (models_.empty() && !contradiction_tick_) {
        contradiction_tick_ = m.observed_tick;
        if (mode_ != HypothesisMode::FlatActive) {
            synthesized_ = true;
            auto programs = hypothesis_grammar();
            cost_.generated = programs.size();
            auto flat = hypothesis_flat_programs();
            programs.insert(programs.end(), flat.begin(), flat.end());
            build(std::move(programs), true);
        }
    }
    return true;
}
HypothesisChoice HypothesisAgent::choose_input(std::span<const unsigned> permutation) {
    HypothesisChoice choice;
    std::array<bool, 65> used{};
    for (const auto& e : evidence_)
        used[e.measurement.input] = true;
    for (const auto e : permutation) {
        if (e < 1 || e > 64)
            throw std::invalid_argument("hypothesis permutation input");
        if (!used[e] && !choice.input)
            choice.input = e;
    }
    if (!choice.input || exhausted_ || models_.size() < 2 ||
        mode_ == HypothesisMode::SynthesisRandom)
        return choice;
    const auto fallback = choice.input;
    auto best = models_.size();
    for (unsigned e = 1; e <= 64; ++e) {
        if (used[e])
            continue;
        std::array<std::uint64_t, 129> groups{};
        for (const auto& model : models_) {
            if (!charge(1))
                return HypothesisChoice{fallback, false, {}, {}};
            ++groups[model.table[e]];
        }
        const auto worst = *std::max_element(groups.begin(), groups.end());
        choice.worst[e] = worst;
        if (worst < best) {
            best = static_cast<std::size_t>(worst);
            choice.input = e;
            choice.partition = groups;
            choice.active = true;
        }
    }
    return choice;
}
std::optional<unsigned> HypothesisAgent::output(const HypothesisModel& model,
                                                const DiscoveryChain& chain, unsigned input) {
    for (const auto symbol : chain) {
        if (symbol == 0) {
            if (!charge(1))
                return {};
            input = model.table[input];
        } else if (symbol == 1)
            input /= 2;
        else
            input = std::min(input, 48U);
    }
    return input;
}
std::optional<DiscoveryPrediction> HypothesisAgent::predict(const DiscoveryChain& chain,
                                                            unsigned input) {
    if (!discovery_chain_valid(chain) || input > 128)
        throw std::invalid_argument("hypothesis prediction input");
    if (exhausted_ || models_.empty())
        return {};
    std::vector<unsigned> values;
    for (const auto& model : models_) {
        const auto value = output(model, chain, input);
        if (!value)
            return {};
        values.push_back(*value);
    }
    std::sort(values.begin(), values.end());
    return DiscoveryPrediction{values[(values.size() - 1) / 2], values.front(), values.back(),
                               values.size()};
}
std::optional<DiscoveryChain> HypothesisAgent::choose_chain(unsigned input, unsigned target) {
    if (input > 128 || target > 128)
        throw std::invalid_argument("hypothesis target input");
    if (exhausted_ || models_.empty())
        return {};
    std::optional<DiscoveryChain> best;
    std::uint64_t best_hits = 0;
    unsigned best_spread = 129;
    for (const auto& chain : hypothesis_chains()) {
        std::uint64_t hits = 0;
        unsigned minimum = 129, maximum = 0;
        for (const auto& model : models_) {
            const auto value = output(model, chain, input);
            if (!value)
                return {};
            hits += *value == target;
            minimum = std::min(minimum, *value);
            maximum = std::max(maximum, *value);
        }
        const auto spread = maximum - minimum;
        if (!best || hits > best_hits ||
            (hits == best_hits &&
             (spread < best_spread ||
              (spread == best_spread && (chain.size() < best->size() ||
                                         (chain.size() == best->size() && chain < *best)))))) {
            best = chain;
            best_hits = hits;
            best_spread = spread;
        }
    }
    return best;
}
void HypothesisAgent::freeze() noexcept { frozen_ = true; }
bool HypothesisAgent::frozen() const noexcept { return frozen_; }
bool HypothesisAgent::exhausted() const noexcept { return exhausted_; }
bool HypothesisAgent::synthesized() const noexcept { return synthesized_; }
std::uint64_t HypothesisAgent::contradiction_tick() const noexcept { return contradiction_tick_; }
const HypothesisCost& HypothesisAgent::cost() const noexcept { return cost_; }
const std::vector<HypothesisModel>& HypothesisAgent::models() const noexcept { return models_; }
const std::vector<DiscoveryEvidence>& HypothesisAgent::evidence() const noexcept {
    return evidence_;
}
std::string HypothesisAgent::state() const {
    std::ostringstream out;
    out << actor_.value << ' ' << static_cast<unsigned>(mode_) << ' ' << limits_.primitives << ' '
        << limits_.reads << ' ' << cost_.primitives << ' ' << cost_.reads << ' ' << cost_.generated
        << ' ' << synthesized_ << ' ' << exhausted_ << ' ' << frozen_ << ' ' << contradiction_tick_
        << ' ' << models_.size() << '\n';
    for (const auto& model : models_) {
        out << model.program.size();
        for (const auto op : model.program)
            out << ' ' << static_cast<unsigned>(op.kind) << ' ' << op.parameter;
        out << '\n';
    }
    for (const auto& e : evidence_)
        out << e.measurement.id << ' ' << e.measurement.observed_tick << ' ' << e.measurement.input
            << ' ' << e.measurement.output << '\n';
    return out.str();
}
} // namespace reality
