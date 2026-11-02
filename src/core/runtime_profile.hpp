#pragma once

#include <array>
#include <chrono>
#include <cstdint>

namespace reality {
enum class ProfileOperation : unsigned { Navigation, Learning, Save };
struct OperationCost {
    double milliseconds = 0;
    std::uint64_t calls = 0;
};
struct RuntimeProfile {
    std::array<OperationCost, 3> costs{};
};
// Observer binding is thread-local and scoped. Domain code never reads its values.
class ProfileBinding final {
  public:
    explicit ProfileBinding(RuntimeProfile* profile) noexcept;
    ~ProfileBinding();
    ProfileBinding(const ProfileBinding&) = delete;
    ProfileBinding& operator=(const ProfileBinding&) = delete;

  private:
    RuntimeProfile* previous_;
    std::array<unsigned, 3> previous_depth_;
};
class ProfileScope final {
  public:
    explicit ProfileScope(ProfileOperation operation) noexcept;
    ~ProfileScope();
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

  private:
    RuntimeProfile* target_;
    unsigned operation_;
    bool outer_ = false;
    std::chrono::steady_clock::time_point start_{};
};
} // namespace reality
