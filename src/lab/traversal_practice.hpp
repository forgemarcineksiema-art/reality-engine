#pragma once
#include "research/contrast_agent.hpp"
#include <optional>
namespace reality {
class TraversalSession;
struct PracticeSeed {
    std::uint32_t device = 1, version = 1;
    std::size_t station = 2;
    HypothesisProgram law{
        {HypothesisKind::Cap, 24}, {HypothesisKind::Loss, 4}, {HypothesisKind::Cap, 16}};
    unsigned input = 16, target = 8;
    std::vector<std::size_t> researchers{3, 5};
};
enum class PracticeStage {
    Idle,
    Configuring,
    Pulsing,
    Working,
    Sharing,
    Waiting,
    NoMethod,
    NoEnergy
};
struct PracticeView {
    bool enabled = false, local = false, method_known = false;
    PracticeStage stage = PracticeStage::Idle;
    unsigned energy = 0, probes = 0, records = 0;
    std::optional<DiscoveryMeasurement> last;
    std::optional<DiscoveryChain> method;
    std::string message;
};
class PracticeState final {
  public:
    PracticeState(PracticeSeed seed, const TraversalSession& session);

  private:
    friend class TraversalSession;
    struct Mind {
        EntityId actor;
        ContrastAgent agent;
        unsigned probes = 0, spent = 0;
        std::optional<DiscoveryChain> method;
        explicit Mind(EntityId id) : actor(id), agent(id, true) {}
    };
    struct Pending {
        std::size_t actor;
        unsigned input, stage;
        std::uint64_t command;
        bool learning;
    };
    struct Permit {
        EntityId actor;
        std::uint64_t measurement, tick;
    };
    struct Working {
        std::size_t actor;
        EntityId action;
    };
    PracticeSeed seed_;
    DiscoveryWorld apparatus_;
    std::vector<Mind> minds_;
    std::vector<EntityId> researchers_;
    std::optional<Pending> pending_;
    std::optional<Permit> permit_;
    std::optional<Working> working_;
    std::uint64_t command_ = 1;
};
} // namespace reality
