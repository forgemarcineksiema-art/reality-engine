#include "research/discovery_agent.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
namespace reality {
namespace {
void check(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
std::vector<DiscoveryRule> prior() {
    std::vector<DiscoveryRule> rules;
    for (unsigned q = 3; q <= 12; ++q)
        rules.push_back({DiscoveryFamily::Fraction, q});
    for (unsigned d = 1; d <= 24; ++d)
        rules.push_back({DiscoveryFamily::Loss, d});
    for (unsigned c = 8; c <= 48; ++c)
        rules.push_back({DiscoveryFamily::Cap, c});
    return rules;
}
using Models = std::array<std::vector<DiscoveryRule>, 3>;
std::uint64_t combinations(const Models& models) {
    return models[0].size() * models[1].size() * models[2].size();
}
Models filtered(Models models, const DiscoveryMeasurement& m, std::uint64_t& work) {
    auto& rules = models.at(m.chain.front());
    std::erase_if(rules, [&](auto rule) {
        ++work;
        return discovery_apply(rule, m.input) != m.output;
    });
    return models;
}
} // namespace
const std::vector<DiscoveryChain>& discovery_chains() {
    static const auto result = [] {
        std::vector<DiscoveryChain> chains;
        for (unsigned length = 1; length <= 3; ++length) {
            const unsigned count = length == 1 ? 3 : (length == 2 ? 9 : 27);
            for (unsigned index = 0; index < count; ++index) {
                DiscoveryChain chain(length);
                auto remaining = index;
                for (unsigned j = length; j > 0; --j) {
                    chain[j - 1] = static_cast<std::uint8_t>(remaining % 3);
                    remaining /= 3;
                }
                chains.push_back(std::move(chain));
            }
        }
        return chains;
    }();
    return result;
}
DiscoveryAgent::DiscoveryAgent(EntityId actor) : actor_(actor) {
    check(actor.value != 0, "discovery agent has no identity");
    for (auto& models : models_)
        models = prior();
}
EntityId DiscoveryAgent::actor() const noexcept { return actor_; }
bool DiscoveryAgent::observe(const DiscoveryEvidence& evidence) {
    check(!frozen_, "frozen discovery model");
    const auto& m = evidence.measurement;
    check(m.id && m.command && m.source_actor.value && evidence.via_actor.value &&
              m.apparatus < 2 && m.observed_tick > 0 && evidence.learned_tick >= m.observed_tick &&
              m.chain.size() == 1 && discovery_chain_valid(m.chain) && m.input > 0 &&
              m.input <= 8192 && m.output <= m.input,
          "invalid measurement evidence");
    for (const auto& known : evidence_)
        if (known.measurement.id == m.id) {
            check(known.measurement == m, "conflicting measurement identity");
            return false;
        }
    check(evidence_.size() < 128, "discovery evidence limit");
    auto work = work_units_;
    auto next = filtered(models_, m, work);
    evidence_.push_back(evidence);
    models_ = std::move(next);
    work_units_ = work;
    return true;
}
std::array<std::size_t, 3> DiscoveryAgent::counts() const noexcept {
    return {models_[0].size(), models_[1].size(), models_[2].size()};
}
const std::vector<DiscoveryEvidence>& DiscoveryAgent::evidence() const noexcept {
    return evidence_;
}
bool DiscoveryAgent::inadequate() const noexcept { return combinations(models_) == 0; }
std::optional<DiscoveryProbe> DiscoveryAgent::choose_probe() const {
    if (frozen_ || inadequate())
        return {};
    const auto current = combinations(models_);
    auto best = current;
    std::optional<DiscoveryProbe> choice;
    // Iteration order also implements the specified energy/symbol tie break.
    for (const auto input : {16U, 32U, 64U})
        for (unsigned symbol = 0; symbol < 3; ++symbol) {
            std::array<std::uint64_t, 65> groups{};
            for (const auto rule : models_[symbol]) {
                ++groups[discovery_apply(rule, input)];
                ++work_units_;
            }
            const auto others = current / models_[symbol].size();
            const auto worst = *std::max_element(groups.begin(), groups.end()) * others;
            if (worst < best) {
                best = worst;
                choice = DiscoveryProbe{symbol, input};
            }
        }
    return choice;
}
std::vector<std::uint64_t> DiscoveryAgent::histogram(const DiscoveryChain& chain,
                                                     DiscoveryEnergy input) const {
    check(discovery_chain_valid(chain) && input <= 8192 && !inadequate(),
          "model_inadequate or invalid prediction");
    std::vector<std::uint64_t> result(input + 1);
    for (const auto r0 : models_[0])
        for (const auto r1 : models_[1])
            for (const auto r2 : models_[2]) {
                const std::array rules{r0, r1, r2};
                auto output = input;
                for (const auto symbol : chain) {
                    output = discovery_apply(rules[symbol], output);
                    ++work_units_;
                }
                ++result[output];
            }
    return result;
}
DiscoveryPrediction DiscoveryAgent::predict(const DiscoveryChain& chain,
                                            DiscoveryEnergy input) const {
    const auto h = histogram(chain, input);
    DiscoveryPrediction result;
    result.combinations = combinations(models_);
    result.minimum = input;
    std::uint64_t seen = 0;
    bool median = false;
    for (DiscoveryEnergy value = 0; value <= input; ++value) {
        if (h[value]) {
            result.minimum = std::min(result.minimum, value);
            result.maximum = value;
        }
        seen += h[value];
        if (!median && seen > (result.combinations - 1) / 2) {
            result.median = value;
            median = true;
        }
    }
    return result;
}
DiscoveryChain DiscoveryAgent::choose_chain(DiscoveryEnergy input, DiscoveryEnergy target) const {
    check(target <= input && !inadequate(), "invalid discovery goal/model");
    std::uint64_t best_hits = 0;
    DiscoveryEnergy best_spread = std::numeric_limits<DiscoveryEnergy>::max();
    DiscoveryChain best;
    for (const auto& chain : discovery_chains()) {
        const auto h = histogram(chain, input);
        DiscoveryEnergy lo = input, hi = 0;
        for (DiscoveryEnergy i = 0; i <= input; ++i)
            if (h[i]) {
                lo = std::min(lo, i);
                hi = i;
            }
        if (best.empty() || h[target] > best_hits ||
            (h[target] == best_hits && hi - lo < best_spread)) {
            best = chain;
            best_hits = h[target];
            best_spread = hi - lo;
        }
    }
    return best;
}
std::vector<DiscoveryEvidence> DiscoveryAgent::teaching_packet() const {
    std::vector<DiscoveryEvidence> own;
    for (const auto& e : evidence_)
        if (e.measurement.source_actor == actor_)
            own.push_back(e);
    std::sort(own.begin(), own.end(),
              [](const auto& a, const auto& b) { return a.measurement.id < b.measurement.id; });
    Models empty{prior(), prior(), prior()};
    auto best_count = combinations(empty);
    std::vector<DiscoveryEvidence> best, current;
    const std::function<void(std::size_t, const Models&)> visit = [&](std::size_t begin,
                                                                      const Models& models) {
        const auto count = combinations(models);
        // DFS visits equally-sized subsets lexicographically by measurement id.
        if (count < best_count || (count == best_count && current.size() < best.size())) {
            best_count = count;
            best = current;
        }
        if (current.size() == 6)
            return;
        for (std::size_t i = begin; i < own.size(); ++i) {
            current.push_back(own[i]);
            const auto next = filtered(models, own[i].measurement, work_units_);
            visit(i + 1, next);
            current.pop_back();
        }
    };
    visit(0, empty);
    return best;
}
void DiscoveryAgent::freeze() noexcept { frozen_ = true; }
bool DiscoveryAgent::frozen() const noexcept { return frozen_; }
std::uint64_t DiscoveryAgent::work_units() const noexcept { return work_units_; }
std::string DiscoveryAgent::serialize() const {
    std::ostringstream out;
    out << "DAGENT 1 " << actor_.value << ' ' << frozen_ << ' ' << work_units_ << ' '
        << evidence_.size() << '\n';
    for (const auto& e : evidence_) {
        const auto& m = e.measurement;
        out << m.id << ' ' << m.command << ' ' << m.apparatus << ' ' << m.source_actor.value << ' '
            << m.observed_tick << ' ' << static_cast<unsigned>(m.chain.front()) << ' ' << m.input
            << ' ' << m.output << ' ' << e.via_actor.value << ' ' << e.learned_tick << '\n';
    }
    return out.str();
}
DiscoveryAgent DiscoveryAgent::deserialize(std::string_view bytes) {
    check(bytes.size() < 128 * 1024, "agent snapshot too large");
    std::istringstream in{std::string(bytes)};
    std::string magic;
    unsigned version = 0, frozen = 0;
    EntityId actor;
    std::uint64_t work = 0;
    std::size_t count = 0;
    check(static_cast<bool>(in >> magic >> version >> actor.value >> frozen >> work >> count) &&
              magic == "DAGENT" && version == 1 && frozen <= 1 && count <= 128,
          "invalid agent header");
    DiscoveryAgent result{actor};
    for (std::size_t i = 0; i < count; ++i) {
        DiscoveryEvidence e;
        auto& m = e.measurement;
        unsigned symbol = 0;
        check(static_cast<bool>(in >> m.id >> m.command >> m.apparatus >> m.source_actor.value >>
                                m.observed_tick >> symbol >> m.input >> m.output >>
                                e.via_actor.value >> e.learned_tick) &&
                  symbol < 3,
              "invalid agent evidence");
        m.chain = {static_cast<std::uint8_t>(symbol)};
        check(result.observe(e), "duplicate agent evidence");
    }
    check(work >= result.work_units_, "agent work counter regressed");
    result.work_units_ = work;
    result.frozen_ = frozen != 0;
    in >> std::ws;
    check(in.eof() && result.serialize() == bytes, "noncanonical agent snapshot");
    return result;
}
} // namespace reality
