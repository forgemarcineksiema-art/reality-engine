#pragma once
#include "research/discovery_session.hpp"
#include "research/hypothesis_agent.hpp"
namespace reality {
struct HypothesisLaw {
    HypothesisProgram program;
    unsigned square_divisor = 0;
    [[nodiscard]] HypothesisTable table() const;
};
[[nodiscard]] inline std::uint64_t hypothesis_checksum(std::string_view bytes) noexcept {
    return snapshot_checksum(bytes);
}
class HypothesisSession final {
  public:
    HypothesisSession(HypothesisLaw law, HypothesisMode mode, std::uint32_t seed,
                      TraversalScene scene = make_discovery_scene(), HypothesisLimits limits = {});
    [[nodiscard]] DiscoveryOutcome begin_probe(DiscoveryChain chain, unsigned input,
                                               bool learning = true);
    void advance(std::uint64_t ticks);
    void set_goal(SurfaceLocation goal);
    [[nodiscard]] HypothesisChoice choose_input();
    [[nodiscard]] std::optional<DiscoveryPrediction> predict(const DiscoveryChain& chain,
                                                             unsigned input);
    [[nodiscard]] std::optional<DiscoveryChain> choose_chain(unsigned input, unsigned target);
    void freeze();
    [[nodiscard]] HypothesisSession evaluation_copy() const;
    [[nodiscard]] const HypothesisAgent& agent() const noexcept;
    [[nodiscard]] const DiscoveryWorld& apparatus() const noexcept;
    [[nodiscard]] const TraversalSession& traversal() const noexcept;
    [[nodiscard]] std::uint64_t tick() const noexcept;
    [[nodiscard]] unsigned attempts() const noexcept;
    [[nodiscard]] bool pending() const noexcept;
    [[nodiscard]] std::string serialize() const;
    void restore(std::string_view bytes);
    void save(const std::filesystem::path& path) const;
    void load(const std::filesystem::path& path);

  private:
    struct Pending {
        unsigned stage, input;
        std::uint64_t command;
        bool learning;
    };
    HypothesisLaw law_;
    HypothesisMode mode_;
    std::uint32_t seed_, random_counter_ = 0;
    HypothesisLimits limits_;
    TraversalSession traversal_;
    DiscoveryWorld apparatus_;
    HypothesisAgent agent_;
    std::vector<unsigned> permutation_;
    std::vector<std::string> events_;
    std::string initial_;
    std::optional<DiscoveryWorld> archive_;
    std::optional<Pending> pending_;
    unsigned attempts_ = 0, spent_ = 0;
    std::uint64_t next_command_ = 1;
    std::optional<std::uint64_t> learning_start_;
    [[nodiscard]] bool at_bench() const;
    void check_log(bool advancing = false, std::uint64_t ticks = 0) const;
    void enter_evaluation();
    [[nodiscard]] std::string canonical_state() const;
    [[nodiscard]] static HypothesisSession deserialize(std::string_view bytes);
};
} // namespace reality
