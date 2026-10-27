#pragma once
#include "research/discovery_session.hpp"
namespace reality {
struct DiscoveryQuery {
    DiscoveryEnergy input = 0, target = 0;
    DiscoveryChain recipe;
};
struct DiscoveryCase {
    std::uint32_t seed = 0;
    std::array<DiscoveryRule, 3> rules;
    std::vector<DiscoveryQuery> predictions, tasks_a, tasks_b;
};
[[nodiscard]] DiscoveryCase read_discovery_case(const std::filesystem::path& path);
void run_discovery_case(const DiscoveryCase& data, const std::filesystem::path& directory);
void discovery_fixture(const std::filesystem::path& directory);
void continue_discovery_fixture(const std::filesystem::path& input,
                                const std::filesystem::path& directory);
} // namespace reality
