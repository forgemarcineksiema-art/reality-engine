#pragma once
#include "research/hypothesis_agent.hpp"
#include <span>
namespace reality {
struct ContrastChoice {
    unsigned input = 0;
    bool active = false;
    std::uint64_t before = 0, worst = 0;
    std::array<std::uint64_t, 129> partition{};
};
class ContrastAgent final {
  public:
    ContrastAgent(EntityId actor, bool active, HypothesisLimits limits = {});
    [[nodiscard]] bool observe(const DiscoveryEvidence& evidence, bool shared = false);
    [[nodiscard]] ContrastChoice choose(std::span<const unsigned> permutation);
    [[nodiscard]] std::optional<DiscoveryPrediction> predict(const DiscoveryChain& chain,
                                                             unsigned input);
    [[nodiscard]] std::optional<DiscoveryChain> choose_chain(unsigned input, unsigned target);
    void freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool frozen() const noexcept { return frozen_; }
    [[nodiscard]] bool exhausted() const noexcept { return exhausted_; }
    [[nodiscard]] const auto& models() const noexcept { return models_; }
    [[nodiscard]] const auto& evidence() const noexcept { return evidence_; }
    [[nodiscard]] const auto& cost() const noexcept { return cost_; }
    [[nodiscard]] std::string state() const;

  private:
    EntityId actor_;
    bool active_, frozen_ = false, exhausted_ = false;
    HypothesisLimits limits_;
    HypothesisCost cost_;
    std::vector<HypothesisModel> models_;
    std::vector<DiscoveryEvidence> evidence_;
    [[nodiscard]] bool charge(std::uint64_t count, bool primitive = false);
    [[nodiscard]] std::optional<unsigned> output(const HypothesisModel& model,
                                                 const DiscoveryChain& chain, unsigned input);
};
} // namespace reality
