#pragma once

#include "simulation/work_world.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace reality {

inline constexpr std::size_t snapshot_byte_limit = 4 * 1024 * 1024;

[[nodiscard]] std::uint64_t snapshot_checksum(std::string_view bytes) noexcept;
void write_snapshot_atomic(std::string_view bytes, const std::filesystem::path& destination);
[[nodiscard]] std::string read_snapshot_bytes(const std::filesystem::path& source);

[[nodiscard]] std::string serialize_world(const WorkWorld& world);
[[nodiscard]] WorkWorld deserialize_world(std::string_view bytes);
void save_world(const WorkWorld& world, const std::filesystem::path& destination);
[[nodiscard]] WorkWorld load_world(const std::filesystem::path& source);

} // namespace reality
