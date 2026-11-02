#include "core/runtime_profile.hpp"

namespace reality {
namespace {
thread_local RuntimeProfile* active = nullptr;
// A binding starts a separate stack; nested binding must restore its parent's depths.
thread_local std::array<unsigned, 3> depth{};
} // namespace
ProfileBinding::ProfileBinding(RuntimeProfile* profile) noexcept
    : previous_(active), previous_depth_(depth) {
    active = profile;
    depth = {};
}
ProfileBinding::~ProfileBinding() {
    active = previous_;
    depth = previous_depth_;
}
ProfileScope::ProfileScope(ProfileOperation operation) noexcept
    : target_(active), operation_(static_cast<unsigned>(operation)) {
    if (target_) {
        outer_ = depth[operation_]++ == 0;
        if (outer_)
            start_ = std::chrono::steady_clock::now();
    }
}
ProfileScope::~ProfileScope() {
    if (!target_)
        return;
    --depth[operation_];
    if (outer_) {
        auto& cost = target_->costs[operation_];
        cost.milliseconds +=
            std::chrono::duration<double, std::milli>{std::chrono::steady_clock::now() - start_}
                .count();
        ++cost.calls;
    }
}
} // namespace reality
