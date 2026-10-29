#pragma once
#include "navigation/surface_navigation.hpp"
#include "simulation/work_world.hpp"
namespace reality {
enum class StoryStage {
    Dormant,
    Accepted,
    Completed,
    MissingSpeaker,
    MissingSite,
    MissingSupply,
    Failed
};
enum class StoryReply { Accepted, Ended, OutOfReach, Busy, Unavailable };
struct StorySeed {
    std::size_t executor = 0, speaker = 1, recipient = 2;
    std::uint32_t source = 0, destination = 1, site = 2;
    SurfaceLocation meeting{1, {0, 0}};
};
struct StoryState {
    EntityId executor, speaker, recipient, source, destination;
    std::uint32_t site = 0;
    // Immutable role context; never inserted back into geometry or rendering.
    SpatialPrism site_context;
    SurfaceLocation meeting;
    std::uint64_t quantity = 4;
    StoryStage stage = StoryStage::Dormant;
    std::uint64_t started_at = 0, ended_at = 0, begin_event = 0, end_event = 0;
    CommandId order, order_end;
    EntityId cargo, meal;
    SpatialPoint request_feet, speaker_feet;
    std::uint32_t request_surface = 0, speaker_surface = 0;
    bool observed = false;
    std::uint64_t available = 0;
};
} // namespace reality
