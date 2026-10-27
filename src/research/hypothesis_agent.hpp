#pragma once
#include "research/discovery_agent.hpp"
#include "research/hypothesis_program.hpp"
#include <span>
namespace reality {
enum class HypothesisMode : unsigned { FlatActive, SynthesisRandom, SynthesisActive };
struct HypothesisLimits {
    std::uint64_t primitives = 10000000, reads = 300000000;
};
struct HypothesisCost {
    std::uint64_t primitives = 0, reads = 0, generated = 0;
};
struct HypothesisChoice {
    unsigned input = 0;
    bool active = false;
    std::array<std::uint64_t, 65> worst{};
    std::array<std::uint64_t, 129> partition{};
};
class HypothesisAgent final {
  public:
    HypothesisAgent(EntityId actor, HypothesisMode mode, HypothesisLimits limits = {});
    [[nodiscard]] bool observe(const DiscoveryEvidence& evidence);
    [[nodiscard]] HypothesisChoice choose_input(std::span<const unsigned> permutation);
    [[nodiscard]] std::optional<DiscoveryPrediction> predict(const DiscoveryChain& chain,
                                                             unsigned input);
    [[nodiscard]] std::optional<DiscoveryChain> choose_chain(unsigned input, unsigned target);
    void freeze() noexcept;
    [[nodiscard]] bool frozen() const noexcept;
    [[nodiscard]] bool exhausted() const noexcept;
    [[nodiscard]] bool synthesized() const noexcept;
    [[nodiscard]] std::uint64_t contradiction_tick() const noexcept;
    [[nodiscard]] const HypothesisCost& cost() const noexcept;
    [[nodiscard]] const std::vector<HypothesisModel>& models() const noexcept;
    [[nodiscard]] const std::vector<DiscoveryEvidence>& evidence() const noexcept;
    [[nodiscard]] std::string state() const;

  private:
    EntityId actor_;
    HypothesisMode mode_;
    HypothesisLimits limits_;
    HypothesisCost cost_;
    bool synthesized_ = false, exhausted_ = false, frozen_ = false;
    std::uint64_t contradiction_tick_ = 0;
    std::vector<HypothesisModel> models_;
    std::vector<DiscoveryEvidence> evidence_;
    void build(std::vector<HypothesisProgram> programs, bool filter);
    [[nodiscard]] bool charge(std::uint64_t count, bool primitive = false);
    [[nodiscard]] std::optional<unsigned> output(const HypothesisModel& model,
                                                 const DiscoveryChain& chain, unsigned input);
};
} // namespace reality
