#pragma once
#include <cstdint>
#include <stdexcept>
namespace reality {
inline std::uint64_t discovery_draw(std::uint32_t seed, std::uint8_t tag, std::uint32_t& counter) {
    if (counter >= (1U << 24))
        throw std::overflow_error("discovery random domain exhausted");
    const auto key = (static_cast<std::uint64_t>(seed) << 32) |
                     (static_cast<std::uint64_t>(tag) << 24) | counter;
    ++counter;
    auto z = key + 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
} // namespace reality
