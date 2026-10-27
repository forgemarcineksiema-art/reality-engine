#pragma once
#include "lab/traversal_session.hpp"
#include "research/discovery_agent.hpp"
namespace reality {
[[nodiscard]] TraversalScene make_discovery_scene();
struct DiscoveryTrace {
    unsigned actor = 0;
    std::uint64_t command = 0, started_tick = 0;
    DiscoveryChain chain;
    DiscoveryEnergy input = 0;
    std::optional<DiscoveryPrediction> prediction;
    bool learning = true;
};
struct DiscoveryMovement {
    std::uint64_t tick = 0;
    unsigned actor = 0;
    std::uint32_t surface = 0;
    SpatialPoint feet;
};
class DiscoverySession final {
  public:
    explicit DiscoverySession(std::array<DiscoveryRule, 3> rules,
                              TraversalScene scene = make_discovery_scene(),
                              std::uint32_t seed = 0);
    [[nodiscard]] const TraversalSession& traversal() const noexcept;
    [[nodiscard]] const DiscoveryWorld& apparatus() const noexcept;
    [[nodiscard]] const DiscoveryAgent& agent(unsigned actor) const;
    [[nodiscard]] const std::vector<DiscoveryTrace>& traces() const noexcept;
    [[nodiscard]] const std::vector<DiscoveryMovement>& movements() const noexcept;
    [[nodiscard]] std::uint64_t tick() const noexcept;
    [[nodiscard]] unsigned training_attempts(unsigned actor) const;
    void set_goal(unsigned actor, SurfaceLocation location);
    void advance(std::uint64_t ticks);
    [[nodiscard]] DiscoveryOutcome begin_probe(unsigned actor, DiscoveryChain chain,
                                               DiscoveryEnergy input, bool learning = true);
    [[nodiscard]] std::optional<DiscoveryMeasurement>
    probe(unsigned actor, DiscoveryChain chain, DiscoveryEnergy input, bool learning = true);
    void train(unsigned actor, bool random = false);
    void freeze(unsigned actor);
    [[nodiscard]] DiscoveryOutcome share(unsigned sender, unsigned recipient, bool message);
    [[nodiscard]] DiscoveryOutcome meet_and_share(bool message);
    [[nodiscard]] DiscoverySession evaluation_copy() const;
    [[nodiscard]] std::string serialize() const;
    void restore(std::string_view bytes);
    void save(const std::filesystem::path& path) const;
    void load(const std::filesystem::path& path);
    void validate() const;

  private:
    enum class Phase : unsigned { Learning, Contact, Evaluation };
    struct Pending {
        unsigned actor = 0, stage = 0;
        std::uint64_t command = 0;
        DiscoveryEnergy input = 0;
        bool learning = true;
    };
    struct Exchange {
        unsigned sender = 0, recipient = 0;
        std::uint64_t tick = 0;
        std::vector<std::uint64_t> records;
        std::string contact_snapshot;
    };
    TraversalSession traversal_;
    DiscoveryWorld apparatus_;
    std::array<DiscoveryAgent, 2> agents_;
    std::vector<DiscoveryWorld> archives_;
    std::array<unsigned, 2> attempts_{};
    std::array<DiscoveryEnergy, 2> spent_{};
    std::array<std::optional<std::uint64_t>, 2> learning_start_{};
    std::array<std::uint32_t, 2> random_counters_{};
    std::uint32_t seed_ = 0;
    std::uint64_t next_command_ = 1;
    Phase phase_ = Phase::Learning;
    std::optional<Pending> pending_;
    std::vector<DiscoveryTrace> traces_;
    std::vector<Exchange> exchanges_;
    // Diagnostic path only: never used by the world/controller and not serialized.
    std::vector<DiscoveryMovement> movements_;
    [[nodiscard]] bool at_bench(unsigned actor) const;
    [[nodiscard]] const DiscoveryMeasurement* measurement(std::uint64_t id) const;
    [[nodiscard]] static DiscoverySession deserialize(std::string_view bytes);
};
} // namespace reality
