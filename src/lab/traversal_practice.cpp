#include "lab/traversal_session.hpp"
#include <algorithm>
#include <stdexcept>
namespace reality {
namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::invalid_argument(message);
}
} // namespace
PracticeState::PracticeState(PracticeSeed seed, const TraversalSession& session)
    : seed_(std::move(seed)),
      apparatus_(std::array{hypothesis_table(seed_.law),
                            hypothesis_table({{HypothesisKind::Fraction, 8}}),
                            hypothesis_table({{HypothesisKind::Cap, 48}})}) {
    check(seed_.device && seed_.version && seed_.station < session.stations().size() &&
              session.stations()[seed_.station].kind == TraversalTask::Maintain &&
              seed_.input >= 1 && seed_.input <= 128 && seed_.target > 0 &&
              seed_.target <= seed_.input,
          "invalid practice seed");
    for (const auto& actor : session.actors())
        minds_.emplace_back(actor.id);
    for (auto index : seed_.researchers) {
        check(index < minds_.size(), "practice researcher unavailable");
        const auto id = minds_[index].actor;
        check(std::find(researchers_.begin(), researchers_.end(), id) == researchers_.end(),
              "duplicate researcher");
        researchers_.push_back(id);
    }
    apparatus_.advance(session.world().clock().tick_count());
}
bool TraversalSession::has_practice() const noexcept { return practice_.has_value(); }
bool TraversalSession::practice_busy(std::size_t index) const {
    return practice_ && ((practice_->pending_ && practice_->pending_->actor == index) ||
                         (practice_->working_ && practice_->working_->actor == index));
}
bool TraversalSession::practice_at_bench(std::size_t index) const {
    const auto& a = actors_.at(index);
    return practice_ && !a.action.value && !a.yielding && a.transfer == TransferStage::None &&
           a.motion.speed <= .03 &&
           (a.state == TraversalState::Idle || a.state == TraversalState::Arrived) &&
           interaction_accessible(index, scene_.stations.at(practice_->seed_.station).location,
                                  a.feet);
}
PracticeView TraversalSession::practice_view(std::size_t index) const {
    const auto& a = actors_.at(index);
    PracticeView v;
    if (!practice_)
        return v;
    v.enabled = true;
    const auto& p = *practice_;
    const auto& mind = p.minds_.at(index);
    v.local = interaction_accessible(index, scene_.stations[p.seed_.station].location, a.feet);
    v.energy = v.local ? p.apparatus_.devices()[0].source : 0;
    v.probes = mind.probes;
    v.records = static_cast<unsigned>(mind.agent.evidence().size());
    v.method = mind.method;
    v.method_known = v.method.has_value();
    if (!mind.agent.evidence().empty())
        v.last = mind.agent.evidence().back().measurement;
    if (p.pending_ && p.pending_->actor == index)
        v.stage = p.pending_->stage == 0 ? PracticeStage::Configuring : PracticeStage::Pulsing;
    else if (p.working_ && p.working_->actor == index)
        v.stage = PracticeStage::Working;
    v.message = v.stage == PracticeStage::Working       ? "Wykonuje konserwacje"
                : v.stage == PracticeStage::Configuring ? "Przygotowuje probe"
                : v.stage == PracticeStage::Pulsing     ? "Sprawdza ustawienie"
                                                        : "";
    return v;
}
DiscoveryOutcome TraversalSession::practice_probe(std::size_t index, DiscoveryChain chain,
                                                  unsigned input) {
    if (!practice_ || index >= actors_.size() || !discovery_chain_valid(chain) || input < 1 ||
        input > 128 || std::find(chain.begin(), chain.end(), 0) == chain.end())
        return DiscoveryOutcome::Invalid;
    auto& p = *practice_;
    if (p.pending_ || p.working_)
        return DiscoveryOutcome::Busy;
    if (inputs_.at(index) != GroundPoint{} || queued_inputs_.at(index) != GroundPoint{})
        return DiscoveryOutcome::Busy;
    if (!practice_at_bench(index))
        return DiscoveryOutcome::OutOfReach;
    auto& mind = p.minds_[index];
    const auto learning = chain == DiscoveryChain{0};
    if (learning && (mind.probes >= 8 || mind.spent + input > 1024 || mind.agent.frozen()))
        return DiscoveryOutcome::Frozen;
    if (p.apparatus_.devices()[0].source < input)
        return DiscoveryOutcome::NoEnergy;
    const auto result = p.apparatus_.configure(p.command_, actors_[index].id, 0, std::move(chain));
    if (result != DiscoveryOutcome::Accepted)
        return result;
    ++p.command_;
    p.pending_ = PracticeState::Pending{index, input, 0, p.command_++, learning};
    p.permit_.reset();
    if (learning) {
        ++mind.probes;
        mind.spent += input;
    }
    return DiscoveryOutcome::Accepted;
}
bool TraversalSession::practice_allows_work(std::size_t index, std::size_t station) const {
    if (!practice_ || practice_->seed_.station != station)
        return true;
    const auto& p = *practice_;
    return !p.pending_ && !p.working_ && p.permit_ && p.permit_->actor == actors_[index].id &&
           p.permit_->tick <= world_.clock().tick_count();
}
void TraversalSession::practice_work_started(std::size_t index, EntityId action) {
    if (!practice_ || actors_[index].station != practice_->seed_.station)
        return;
    check(practice_allows_work(index, actors_[index].station), "maintenance calibration missing");
    practice_->permit_.reset();
    practice_->working_ = PracticeState::Working{index, action};
}
void TraversalSession::advance_practice() {
    if (!practice_)
        return;
    auto& p = *practice_;
    const auto tick = world_.clock().tick_count();
    std::optional<std::size_t> finished;
    p.apparatus_.advance(tick);
    if (p.pending_) {
        auto& request = *p.pending_;
        check(practice_at_bench(request.actor), "practice observer moved during impulse");
        if (request.stage == 0) {
            check(p.apparatus_.pulse(request.command, actors_[request.actor].id, 0,
                                     request.input) == DiscoveryOutcome::Accepted,
                  "configured practice pulse failed");
            request.stage = 1;
        } else {
            const auto& m = p.apparatus_.measurements().back();
            check(m.command == request.command, "practice impulse result missing");
            if (request.learning)
                check(p.minds_[request.actor].agent.observe({m, m.source_actor, m.observed_tick}),
                      "duplicate practice observation");
            if (m.input == p.seed_.input && m.output == p.seed_.target)
                p.permit_ = PracticeState::Permit{m.source_actor, m.id, m.observed_tick};
            finished = request.actor;
            p.pending_.reset();
        }
    }
    if (p.working_) {
        const auto action = world_.action(p.working_->action);
        if (!action || action->state == ActionState::Completed ||
            action->state == ActionState::Cancelled) {
            finished = p.working_->actor;
            p.working_.reset();
        }
    }
    if (finished && !practice_busy(*finished)) {
        auto& a = actors_[*finished];
        if (!a.autonomous && (a.pending_goal || a.stop_after_transfer)) {
            const auto goal = a.pending_goal;
            const auto reach = a.pending_reach;
            a.pending_goal.reset();
            a.pending_reach = 0;
            a.stop_after_transfer = false;
            if (goal) {
                if (reach > 0)
                    approach(*finished, *goal, reach);
                else
                    set_goal(*finished, *goal);
            } else
                stop(*finished);
        }
    }
}
void TraversalSession::validate_practice() const {
    if (!practice_)
        return;
    const auto& p = *practice_;
    check(p.minds_.size() == actors_.size() && p.seed_.station < scene_.stations.size() &&
              p.apparatus_.tick() == world_.clock().tick_count(),
          "practice world binding mismatch");
    p.apparatus_.validate();
    for (std::size_t i = 0; i < actors_.size(); ++i)
        check(p.minds_[i].actor == actors_[i].id && p.minds_[i].probes <= 8 &&
                  p.minds_[i].spent <= 1024,
              "practice actor budget or identity mismatch");
}
} // namespace reality
