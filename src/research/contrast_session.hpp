#pragma once
#include "research/contrast_agent.hpp"
#include "research/hypothesis_session.hpp"
namespace reality {
class ContrastSession final {
  public:
    ContrastSession(HypothesisLaw law, bool active, std::uint32_t seed,
                    HypothesisLimits limits = {});
    [[nodiscard]] ContrastChoice choose(unsigned actor);
    [[nodiscard]] std::optional<DiscoveryPrediction>
    predict(unsigned actor, const DiscoveryChain& chain, unsigned input);
    [[nodiscard]] std::optional<DiscoveryChain> choose_chain(unsigned actor, unsigned input,
                                                             unsigned target);
    [[nodiscard]] DiscoveryOutcome begin_probe(unsigned actor, DiscoveryChain chain, unsigned input,
                                               bool learning);
    [[nodiscard]] DiscoveryOutcome begin_work(unsigned actor, DiscoveryChain chain, unsigned input,
                                              unsigned target);
    void advance(std::uint64_t ticks);
    void freeze(unsigned actor);
    void set_goal(unsigned actor, SurfaceLocation goal);
    [[nodiscard]] DiscoveryOutcome share(bool message);
    [[nodiscard]] DiscoveryOutcome meet_and_share(bool message);
    [[nodiscard]] const ContrastAgent& agent(unsigned actor) const { return agents_.at(actor); }
    [[nodiscard]] const auto& apparatus() const noexcept { return apparatus_; }
    [[nodiscard]] const auto& traversal() const noexcept { return traversal_; }
    [[nodiscard]] std::uint64_t tick() const noexcept;
    [[nodiscard]] std::uint64_t parts(unsigned actor) const;
    [[nodiscard]] std::uint64_t completed(unsigned actor) const;
    [[nodiscard]] bool work_started() const noexcept { return work_started_; }
    [[nodiscard]] bool pending() const noexcept { return pending_.has_value(); }
    [[nodiscard]] std::uint64_t contact_tick() const noexcept { return contact_tick_; }
    [[nodiscard]] const auto& contact_positions() const noexcept { return contact_positions_; }
    [[nodiscard]] const auto& contact_surfaces() const noexcept { return contact_surfaces_; }
    [[nodiscard]] std::string serialize() const;
    void restore(std::string_view bytes);

  private:
    struct Pending {
        unsigned actor, input, stage, target;
        std::uint64_t command;
        bool learning;
    };
    HypothesisLaw law_;
    bool active_;
    std::uint32_t seed_, random_counter_ = 0;
    HypothesisLimits limits_;
    TraversalSession traversal_;
    DiscoveryWorld apparatus_;
    std::array<ContrastAgent, 2> agents_;
    std::vector<unsigned> permutation_;
    std::vector<std::string> events_;
    std::optional<Pending> pending_;
    std::array<unsigned, 2> attempts_{}, spent_{};
    std::array<std::optional<std::uint64_t>, 2> start_{};
    std::uint64_t command_ = 1;
    bool work_started_ = false;
    std::uint64_t contact_tick_ = 0;
    std::array<double, 6> contact_positions_{};
    std::array<std::uint32_t, 2> contact_surfaces_{};
    [[nodiscard]] bool at_bench(unsigned actor) const;
    void room(unsigned entries = 1) const;
    void ready() const;
    void stop(unsigned actor);
    [[nodiscard]] DiscoveryOutcome begin(unsigned actor, DiscoveryChain chain, unsigned input,
                                         bool learning, unsigned target);
    [[nodiscard]] std::string canonical() const;
    [[nodiscard]] static ContrastSession deserialize(std::string_view bytes);
};
} // namespace reality
