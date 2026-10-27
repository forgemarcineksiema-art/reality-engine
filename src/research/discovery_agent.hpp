#pragma once
#include "research/discovery_world.hpp"
namespace reality {
struct DiscoveryEvidence {
    DiscoveryMeasurement measurement;
    EntityId via_actor;
    std::uint64_t learned_tick = 0;
    bool operator==(const DiscoveryEvidence&) const = default;
};
struct DiscoveryProbe {
    unsigned symbol = 0;
    DiscoveryEnergy input = 16;
    bool operator==(const DiscoveryProbe&) const = default;
};
struct DiscoveryPrediction {
    DiscoveryEnergy median = 0, minimum = 0, maximum = 0;
    std::uint64_t combinations = 0;
};
class DiscoveryAgent final {
  public:
    explicit DiscoveryAgent(EntityId actor);
    [[nodiscard]] EntityId actor() const noexcept;
    [[nodiscard]] bool observe(const DiscoveryEvidence& evidence);
    [[nodiscard]] std::array<std::size_t, 3> counts() const noexcept;
    [[nodiscard]] const std::vector<DiscoveryEvidence>& evidence() const noexcept;
    [[nodiscard]] std::optional<DiscoveryProbe> choose_probe() const;
    [[nodiscard]] DiscoveryPrediction predict(const DiscoveryChain& chain,
                                              DiscoveryEnergy input) const;
    [[nodiscard]] DiscoveryChain choose_chain(DiscoveryEnergy input, DiscoveryEnergy target) const;
    [[nodiscard]] std::vector<DiscoveryEvidence> teaching_packet() const;
    void freeze() noexcept;
    [[nodiscard]] bool frozen() const noexcept;
    [[nodiscard]] bool inadequate() const noexcept;
    [[nodiscard]] std::uint64_t work_units() const noexcept;
    [[nodiscard]] std::string serialize() const;
    [[nodiscard]] static DiscoveryAgent deserialize(std::string_view bytes);

  private:
    using Models = std::array<std::vector<DiscoveryRule>, 3>;
    EntityId actor_;
    Models models_;
    std::vector<DiscoveryEvidence> evidence_;
    bool frozen_ = false;
    mutable std::uint64_t work_units_ = 0;
    [[nodiscard]] std::vector<std::uint64_t> histogram(const DiscoveryChain& chain,
                                                       DiscoveryEnergy input) const;
};
[[nodiscard]] const std::vector<DiscoveryChain>& discovery_chains();
} // namespace reality
