#include "lab/traversal_diagnostics.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace reality {
namespace {
NavigationActivity activity(const TraversalActor& actor) {
    if (actor.transfer != TransferStage::None)
        return NavigationActivity::Transfer;
    switch (actor.state) {
    case TraversalState::Idle:
        return NavigationActivity::Inactive;
    case TraversalState::Arrived:
        return NavigationActivity::Arrived;
    case TraversalState::Waiting:
        return NavigationActivity::Waiting;
    case TraversalState::NoPath:
        return NavigationActivity::NoPath;
    case TraversalState::InvalidGoal:
        return NavigationActivity::InvalidGoal;
    case TraversalState::BudgetExceeded:
        return NavigationActivity::BudgetExceeded;
    case TraversalState::Moving:
        if (actor.goal && actor.leg < actor.route.legs.size() &&
            actor.route.legs[actor.leg].kind == SurfaceLegKind::Walk)
            return NavigationActivity::Travelling;
        return NavigationActivity::Transfer;
    }
    throw std::invalid_argument("invalid navigation state for observation");
}
const char* describe(NavigationActivity activity) {
    switch (activity) {
    case NavigationActivity::Inactive:
        return "inactive";
    case NavigationActivity::Travelling:
        return "travelling";
    case NavigationActivity::Waiting:
        return "waiting";
    case NavigationActivity::Transfer:
        return "transfer";
    case NavigationActivity::NoPath:
        return "no_path";
    case NavigationActivity::Arrived:
        return "arrived";
    case NavigationActivity::BudgetExceeded:
        return "budget_exceeded";
    case NavigationActivity::InvalidGoal:
        return "invalid_goal";
    case NavigationActivity::Parking:
        return "parking";
    case NavigationActivity::Precision:
        return "precision";
    case NavigationActivity::Riding:
        return "riding";
    case NavigationActivity::SuspectedStall:
        return "suspected_stall";
    }
    throw std::invalid_argument("invalid navigation observation");
}
} // namespace
void NavigationProgressWatch::observe(NavigationSample sample) {
    if (sample.actor.value == 0 || !std::isfinite(sample.distance_m) || sample.distance_m < 0 ||
        static_cast<unsigned>(sample.activity) >=
            static_cast<unsigned>(NavigationActivity::SuspectedStall) ||
        sample.stall_window < 100 || sample.stall_window > 10020 ||
        (sample.activity == NavigationActivity::Travelling && (!sample.goal || !sample.waypoint)))
        throw std::invalid_argument("invalid navigation sample");
    if (sample.position)
        reality::validate(*sample.position);
    const auto continuous = previous_ && previous_->actor == sample.actor &&
                            previous_->goal == sample.goal && sample.tick >= previous_->tick &&
                            sample.tick - previous_->tick <= 40;
    new_alarm_ = false;
    bool progressed = false;
    if (!continuous) {
        best_distance_m_ = sample.distance_m;
        last_progress_ = sample.tick;
        stagnant_ticks_ = 0;
        origin_ = sample.position;
        alarm_episode_ = false;
    } else {
        if (sample.position && origin_ &&
            (sample.support != previous_->support || length(*sample.position - *origin_) >= .5))
            progressed = true;
        const auto same_target =
            previous_->waypoint == sample.waypoint && previous_->activity == sample.activity &&
            previous_->resource == sample.resource && previous_->transport == sample.transport;
        if (!same_target)
            best_distance_m_ = sample.distance_m;
        else if (sample.distance_m <= best_distance_m_ - .1 + 1e-9) {
            best_distance_m_ = sample.distance_m;
            progressed = true;
        }
        if ((sample.activity == NavigationActivity::Waiting ||
             sample.activity == NavigationActivity::Riding ||
             sample.activity == NavigationActivity::Precision) &&
            (sample.progress != previous_->progress || sample.blocker != previous_->blocker ||
             sample.queue_position != previous_->queue_position))
            progressed = true;
        if (progressed) {
            last_progress_ = sample.tick;
            stagnant_ticks_ = 0;
            origin_ = sample.position;
            alarm_episode_ = false;
        }
    }
    activity_ = sample.activity;
    const auto movement = sample.activity == NavigationActivity::Travelling ||
                          sample.activity == NavigationActivity::Parking ||
                          sample.activity == NavigationActivity::Precision ||
                          sample.activity == NavigationActivity::Riding;
    if (continuous && movement && !sample.externally_blocked && !progressed)
        stagnant_ticks_ =
            std::min(std::uint64_t{10020}, stagnant_ticks_ + sample.tick - previous_->tick);
    if (movement && !sample.externally_blocked && stagnant_ticks_ >= sample.stall_window) {
        activity_ = NavigationActivity::SuspectedStall;
        new_alarm_ = !alarm_episode_;
        alarm_episode_ = true;
    }
    if (!sample.goal || sample.activity == NavigationActivity::Arrived) {
        stagnant_ticks_ = 0;
        alarm_episode_ = false;
    }
    previous_ = sample;
}
void NavigationProgressWatch::reset() noexcept { *this = {}; }
NavigationActivity NavigationProgressWatch::activity() const noexcept { return activity_; }
bool NavigationProgressWatch::new_alarm() const noexcept { return new_alarm_; }
EntityId NavigationProgressWatch::actor() const noexcept {
    return previous_ ? previous_->actor : EntityId{};
}
std::string NavigationProgressWatch::report() const {
    if (!previous_)
        return {};
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(4) << "navigation_actor=" << previous_->actor.value
        << " observed_tick=" << previous_->tick << " activity=" << describe(activity_)
        << " last_progress_tick=" << last_progress_ << " distance_m=" << previous_->distance_m
        << " stagnant_ticks=" << stagnant_ticks_ << " resource=" << previous_->resource
        << " transport=" << previous_->transport << " blocker=" << previous_->blocker.value
        << " queue_position=" << previous_->queue_position
        << " resource_progress=" << previous_->progress
        << " externally_blocked=" << previous_->externally_blocked << '\n';
    return out.str();
}
void TraversalDiagnostics::observe(const TraversalSession& session) {
    std::vector<NavigationSample> samples;
    for (std::size_t i = 0; i < session.actors().size(); ++i) {
        const auto& actor = session.actors()[i];
        const auto view = session.navigation_view(i);
        NavigationSample sample{
            actor.id, session.world().clock().tick_count(), activity(actor), actor.goal, {}, 0};
        sample.position = session.geometry().body(actor.support.surface).local(actor.feet);
        sample.support = actor.support.surface;
        sample.resource = view.resource;
        sample.blocker = view.blocker;
        sample.queue_position = view.queue_position;
        sample.progress = view.progress;
        sample.stall_window = view.stall_window;
        sample.transport = view.transport;
        sample.externally_blocked = view.externally_blocked;
        if (view.target) {
            sample.distance_m = length(*view.target - actor.feet);
            const auto local = session.geometry().body(actor.support.surface).local(*view.target);
            sample.waypoint = SurfaceLocation{actor.support.surface, {local.x, local.z}};
        }
        switch (view.kind) {
        case TraversalGuidanceKind::None:
            break;
        case TraversalGuidanceKind::Walk:
            sample.activity = NavigationActivity::Travelling;
            break;
        case TraversalGuidanceKind::Parking:
            sample.activity = sample.distance_m > .015 || actor.motion.speed > .03
                                  ? NavigationActivity::Parking
                                  : NavigationActivity::Waiting;
            break;
        case TraversalGuidanceKind::Precision:
            sample.activity = NavigationActivity::Precision;
            break;
        case TraversalGuidanceKind::Riding:
            sample.activity = NavigationActivity::Riding;
            break;
        case TraversalGuidanceKind::OccupiedGoal:
        case TraversalGuidanceKind::Blocked:
            sample.activity = NavigationActivity::Waiting;
            break;
        }
        samples.push_back(sample);
    }
    observe(samples);
}
void TraversalDiagnostics::observe(std::span<const NavigationSample> samples) {
    if (samples.size() > 32)
        throw std::invalid_argument("too many navigation observations");
    std::vector<NavigationProgressWatch> next;
    std::vector<EntityId> alerts;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        for (std::size_t j = 0; j < i; ++j)
            if (samples[j].actor == samples[i].actor)
                throw std::invalid_argument("duplicate observed actor");
        const auto prior = std::find_if(watches_.begin(), watches_.end(), [&](const auto& watch) {
            return watch.actor() == samples[i].actor;
        });
        next.push_back(prior != watches_.end() ? *prior : NavigationProgressWatch{});
        next[i].observe(samples[i]);
        if (next[i].new_alarm())
            alerts.push_back(samples[i].actor);
    }
    watches_.swap(next);
    new_stalls_.swap(alerts);
}
void TraversalDiagnostics::reset() noexcept {
    watches_.clear();
    new_stalls_.clear();
}
const std::vector<EntityId>& TraversalDiagnostics::new_stalls() const noexcept {
    return new_stalls_;
}
bool TraversalDiagnostics::suspected_stall() const noexcept {
    return std::any_of(watches_.begin(), watches_.end(), [](const auto& watch) {
        return watch.activity() == NavigationActivity::SuspectedStall;
    });
}
std::string TraversalDiagnostics::report() const {
    std::string result = "navigation_observations=" + std::to_string(watches_.size()) + '\n';
    for (const auto& watch : watches_)
        result += watch.report();
    return result;
}
void TraversalDiagnostics::capture(const TraversalSession& session,
                                   const std::filesystem::path& directory,
                                   std::string_view reason) const {
    if (reason.empty() || reason.size() > 200 || reason.find_first_of("\r\n") != reason.npos)
        throw std::invalid_argument("invalid diagnostic reason");
    const auto world = session.serialize();
    auto observed = *this;
    observed.observe(session);
    if (!std::filesystem::create_directory(directory))
        throw std::runtime_error("diagnostic directory already exists");
    write_snapshot_atomic(world, directory / "world.save");
    const auto text = "GENESIS_TRAVERSAL_DIAGNOSTIC 1\nreason=" + std::string{reason} +
                      "\nworld_checksum=" + std::to_string(snapshot_checksum(world)) + '\n' +
                      observed.report() + session.inspect() + "complete=1\n";
    write_snapshot_atomic(text, directory / "diagnostic.txt");
}
} // namespace reality
