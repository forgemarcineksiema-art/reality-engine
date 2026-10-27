#pragma once
#include "core/entity_registry.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace reality {
using DiscoveryEnergy = std::uint32_t;
using DiscoveryChain = std::vector<std::uint8_t>;
using DiscoveryTable = std::array<std::uint16_t, 129>;
enum class DiscoveryFamily : unsigned { Fraction, Loss, Cap, Outside };
struct DiscoveryRule {
    DiscoveryFamily family = DiscoveryFamily::Fraction;
    unsigned parameter = 3;
    bool operator==(const DiscoveryRule&) const = default;
};
[[nodiscard]] DiscoveryEnergy discovery_apply(DiscoveryRule rule, DiscoveryEnergy input);
[[nodiscard]] bool discovery_chain_valid(const DiscoveryChain& chain) noexcept;
struct DiscoveryMeasurement {
    std::uint64_t id = 0, command = 0;
    unsigned apparatus = 0;
    EntityId source_actor;
    std::uint64_t observed_tick = 0;
    DiscoveryChain chain;
    DiscoveryEnergy input = 0, output = 0;
    bool operator==(const DiscoveryMeasurement&) const = default;
};
enum class DiscoveryOutcome { Accepted, Busy, Invalid, NoEnergy, Conflict, OutOfReach, Frozen };
struct DiscoveryDevice {
    DiscoveryChain chain;
    std::uint64_t ready_at = 0;
    DiscoveryEnergy source = 8192, output = 0, loss = 0;
    std::optional<std::size_t> pending;
};
struct DiscoveryPulseView {
    std::uint64_t command = 0, started_tick = 0;
    EntityId actor;
    DiscoveryEnergy input = 0;
};
class DiscoveryWorld final {
  public:
    explicit DiscoveryWorld(std::array<DiscoveryRule, 3> rules);
    explicit DiscoveryWorld(std::array<DiscoveryTable, 3> tables);
    [[nodiscard]] DiscoveryOutcome configure(std::uint64_t command, EntityId actor,
                                             unsigned apparatus, DiscoveryChain chain);
    [[nodiscard]] DiscoveryOutcome pulse(std::uint64_t command, EntityId actor, unsigned apparatus,
                                         DiscoveryEnergy input);
    void advance(std::uint64_t tick);
    [[nodiscard]] const std::array<DiscoveryDevice, 2>& devices() const noexcept;
    [[nodiscard]] const std::vector<DiscoveryMeasurement>& measurements() const noexcept;
    [[nodiscard]] std::uint64_t tick() const noexcept;
    [[nodiscard]] DiscoveryWorld fresh() const;
    [[nodiscard]] bool same_rules(const DiscoveryWorld& other) const noexcept;
    [[nodiscard]] std::optional<DiscoveryPulseView> pending_pulse(unsigned apparatus) const;
    [[nodiscard]] std::string serialize() const;
    [[nodiscard]] static DiscoveryWorld deserialize(std::string_view bytes);
    void validate() const;

  private:
    struct Command {
        std::uint64_t id = 0, tick = 0;
        EntityId actor;
        unsigned apparatus = 0;
        bool pulse = false;
        DiscoveryChain chain;
        DiscoveryEnergy input = 0;
    };
    std::array<DiscoveryRule, 3> rules_{};
    std::optional<std::array<DiscoveryTable, 3>> tables_;
    std::array<DiscoveryDevice, 2> devices_;
    std::vector<Command> commands_;
    std::vector<DiscoveryMeasurement> measurements_;
    std::uint64_t tick_ = 0;
    [[nodiscard]] std::optional<DiscoveryOutcome> replay(const Command& request) const;
};
} // namespace reality
