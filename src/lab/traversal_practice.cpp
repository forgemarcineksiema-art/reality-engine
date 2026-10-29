#include "lab/traversal_session.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
namespace reality {
namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::invalid_argument(message);
}
} // namespace
void PracticeState::infer(Mind& mind, std::uint64_t tick) {
    auto& agent = mind.edit();
    const auto method = agent.choose_chain(seed_.input, seed_.target);
    if (method) {
        const auto prediction = agent.predict(*method, seed_.input);
        if (prediction && prediction->minimum == seed_.target &&
            prediction->maximum == seed_.target) {
            mind.method = method;
            agent.freeze();
        }
    }
    mind.journal.push_back("q " + std::to_string(tick));
}
unsigned PracticeState::choose(Mind& mind, std::uint64_t tick) {
    std::array<unsigned, 128> order;
    std::iota(order.begin(), order.end(), 1U);
    const auto choice = mind.edit().choose(order);
    mind.journal.push_back("c " + std::to_string(tick));
    return choice.input;
}
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
    v.station = p.seed_.station;
    v.required_input = p.seed_.input;
    v.required_output = p.seed_.target;
    v.reads = mind.agent->cost().reads;
    v.primitives = mind.agent->cost().primitives;
    v.history = mind.agent->evidence();
    v.local = interaction_accessible(index, scene_.stations[p.seed_.station].location, a.feet);
    v.energy = v.local ? p.apparatus_.devices()[0].source : 0;
    v.probes = mind.probes;
    v.records = static_cast<unsigned>(mind.agent->evidence().size());
    v.method = mind.method;
    v.method_known = v.method.has_value();
    if (!mind.agent->evidence().empty())
        v.last = mind.agent->evidence().back().measurement;
    for (auto it = p.apparatus_.measurements().rbegin(); it != p.apparatus_.measurements().rend();
         ++it)
        if (it->source_actor == a.id) {
            v.last = *it;
            break;
        }
    if (p.pending_ && p.pending_->actor == index)
        v.stage = p.pending_->stage == 0 ? PracticeStage::Configuring : PracticeStage::Pulsing;
    else if (p.working_ && p.working_->actor == index)
        v.stage = PracticeStage::Working;
    else if (p.sharing_ && (p.sharing_->from == index || p.sharing_->to == index))
        v.stage = PracticeStage::Sharing;
    else if (a.autonomous && a.task == TraversalTask::Maintain && practice_at_bench(index) &&
             std::find(p.queue_.begin(), p.queue_.end(), index) != p.queue_.end())
        v.stage = PracticeStage::Waiting;
    if (v.local && v.energy < p.seed_.input && v.stage == PracticeStage::Idle)
        v.stage = PracticeStage::NoEnergy;
    if (!mind.method &&
        (mind.probes >= 8 || mind.agent->exhausted() || mind.agent->models().empty()) &&
        v.stage == PracticeStage::Idle)
        v.stage = PracticeStage::NoMethod;
    v.message = v.stage == PracticeStage::Working       ? "Wykonuje konserwacje"
                : v.stage == PracticeStage::Configuring ? "Przygotowuje probe"
                : v.stage == PracticeStage::Pulsing     ? "Sprawdza ustawienie"
                : v.stage == PracticeStage::Sharing     ? "Przekazuje pomiary"
                : v.stage == PracticeStage::Waiting     ? "Czeka na urzadzenie"
                : v.stage == PracticeStage::NoMethod    ? "Nie znalazl pewnej metody"
                : v.stage == PracticeStage::NoEnergy    ? "Brak energii urzadzenia"
                                                        : "";
    return v;
}
DiscoveryOutcome TraversalSession::practice_probe(std::size_t index, DiscoveryChain chain,
                                                  unsigned input) {
    if (!practice_ || index >= actors_.size() || !discovery_chain_valid(chain) || input < 1 ||
        input > 128 || std::find(chain.begin(), chain.end(), 0) == chain.end())
        return DiscoveryOutcome::Invalid;
    auto& p = *practice_;
    if (p.pending_ || p.working_ ||
        (p.sharing_ && (p.sharing_->from == index || p.sharing_->to == index)))
        return DiscoveryOutcome::Busy;
    if (inputs_.at(index) != GroundPoint{} || queued_inputs_.at(index) != GroundPoint{})
        return DiscoveryOutcome::Busy;
    if (!practice_at_bench(index))
        return DiscoveryOutcome::OutOfReach;
    auto& mind = p.minds_[index];
    const auto learning = chain == DiscoveryChain{0} && !mind.agent->frozen();
    if (learning && (mind.probes >= 8 || mind.spent + input > 1024 || mind.agent->frozen()))
        return DiscoveryOutcome::Frozen;
    if (learning && mind.agent->evidence().size() >= 24)
        return DiscoveryOutcome::Frozen;
    if (p.apparatus_.devices()[0].source < input)
        return DiscoveryOutcome::NoEnergy;
    if (p.pulses_.size() >= 2048 || mind.journal.size() >= 120)
        return DiscoveryOutcome::Frozen;
    auto clock = world_.clock();
    try {
        clock.advance(2);
    } catch (const std::overflow_error&) {
        return DiscoveryOutcome::Invalid;
    }
    const auto result = p.apparatus_.configure(p.command_, actors_[index].id, 0, chain);
    if (result != DiscoveryOutcome::Accepted)
        return result;
    ++p.command_;
    p.pulses_.push_back(
        {actors_[index].id, input, std::move(chain), p.apparatus_.tick(), learning});
    p.pending_ = PracticeState::Pending{index, input, 0, p.command_++, learning};
    p.permit_.reset();
    if (learning) {
        ++mind.probes;
        mind.spent += input;
    }
    return DiscoveryOutcome::Accepted;
}
void TraversalSession::request_practice_work(std::size_t index) {
    if (!practice_ || !practice_at_bench(index))
        return;
    auto& p = *practice_;
    auto& mind = p.minds_[index];
    if (std::find(p.researchers_.begin(), p.researchers_.end(), mind.actor) ==
            p.researchers_.end() ||
        !legal_service_allowed(index, p.seed_.station))
        return;
    // This inspection is legal here: the actor is physically at the station.
    const auto resource = world_.resource(scene_.stations[p.seed_.station].lot);
    if (!resource || resource->available.value == 0)
        return;
    std::erase_if(p.queue_, [&](auto i) {
        return !actors_[i].autonomous || actors_[i].task != TraversalTask::Maintain ||
               !practice_at_bench(i);
    });
    if (std::find(p.queue_.begin(), p.queue_.end(), index) == p.queue_.end())
        p.queue_.push_back(index);
    if (p.queue_.front() != index || p.pending_ || p.working_ || p.sharing_)
        return;
    if (p.permit_) {
        auto it = std::find_if(actors_.begin(), actors_.end(),
                               [&](const auto& a) { return a.id == p.permit_->actor; });
        if (it != actors_.end() && it->autonomous && it->task == TraversalTask::Maintain)
            return;
    }
    if (!mind.method && !mind.agent->evidence().empty() &&
        (mind.journal.empty() || !mind.journal.back().starts_with("q ")))
        p.infer(mind, world_.clock().tick_count());
    DiscoveryOutcome result = DiscoveryOutcome::Frozen;
    if (mind.method)
        result = practice_probe(index, *mind.method, p.seed_.input);
    else if (mind.probes < 8 && !mind.agent->exhausted() && !mind.agent->models().empty() &&
             mind.journal.size() < 118) {
        const auto input = p.choose(mind, world_.clock().tick_count());
        if (input)
            result = practice_probe(index, {0}, input);
    }
    if (result == DiscoveryOutcome::Accepted || result == DiscoveryOutcome::Frozen ||
        result == DiscoveryOutcome::NoEnergy)
        p.queue_.erase(p.queue_.begin());
}
DiscoveryOutcome TraversalSession::share_practice(std::size_t from, std::size_t to) {
    if (!practice_ || from >= actors_.size() || to >= actors_.size() || from == to)
        return DiscoveryOutcome::Invalid;
    auto& p = *practice_;
    if (p.sharing_ || practice_busy(from) || practice_busy(to))
        return DiscoveryOutcome::Busy;
    if (!knowledge_contact(from, to))
        return DiscoveryOutcome::OutOfReach;
    // The first production scope uses an unmoving bank, not historical moving-platform frames.
    if (std::any_of(scene_.platforms.begin(), scene_.platforms.end(), [&](const auto& b) {
            return b.definition.body == actors_[from].support.surface;
        }))
        return DiscoveryOutcome::OutOfReach;
    auto& target = p.minds_[to];
    if (target.agent->frozen() || target.journal.size() >= 112 || p.contacts_.size() >= 256)
        return DiscoveryOutcome::Frozen;
    const auto tick = world_.clock().tick_count();
    PracticeState::Contact contact{from,
                                   to,
                                   tick,
                                   actors_[from].support.surface,
                                   {actors_[from].feet.x, actors_[from].feet.y,
                                    actors_[from].feet.z, actors_[to].feet.x, actors_[to].feet.y,
                                    actors_[to].feet.z},
                                   {},
                                   {}};
    for (const auto& e : p.minds_[from].agent->evidence())
        if (e.measurement.source_actor == actors_[from].id &&
            std::none_of(target.agent->evidence().begin(), target.agent->evidence().end(),
                         [&](const auto& old) { return old.measurement.id == e.measurement.id; }))
            contact.records.push_back(e.measurement.id);
    if (contact.records.empty())
        return DiscoveryOutcome::Invalid;
    if (contact.records.size() + target.agent->evidence().size() > 24 ||
        contact.records.size() + target.journal.size() > 120)
        return DiscoveryOutcome::Frozen;
    p.sharing_ = std::move(contact);
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
    practice_->uses_.push_back({practice_->permit_->measurement, action});
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
    if (p.sharing_ && tick > p.sharing_->tick) {
        auto contact = *p.sharing_;
        p.sharing_.reset();
        if (actors_[contact.from].local() && actors_[contact.to].local() &&
            actors_[contact.from].support.surface == contact.surface &&
            actors_[contact.to].support.surface == contact.surface &&
            knowledge_contact(contact.from, contact.to) && !practice_busy(contact.from) &&
            !practice_busy(contact.to)) {
            auto& mind = p.minds_[contact.to];
            contact.end = {actors_[contact.from].feet.x, actors_[contact.from].feet.y,
                           actors_[contact.from].feet.z, actors_[contact.to].feet.x,
                           actors_[contact.to].feet.y,   actors_[contact.to].feet.z};
            const auto id = p.contacts_.size();
            for (auto record : contact.records) {
                const auto& ms = p.apparatus_.measurements();
                const auto m = std::find_if(ms.begin(), ms.end(),
                                            [&](const auto& v) { return v.id == record; });
                check(m != ms.end() && mind.edit().observe({*m, m->source_actor, tick}, true),
                      "practice teaching evidence missing");
                mind.journal.push_back("s " + std::to_string(record) + " " + std::to_string(id));
            }
            p.contacts_.push_back(std::move(contact));
        }
    }
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
            if (request.learning) {
                check(p.minds_[request.actor].edit().observe({m, m.source_actor, m.observed_tick}),
                      "duplicate practice observation");
                p.minds_[request.actor].journal.push_back("o " + std::to_string(m.id));
            }
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
    if (!p.pending_ && !p.working_ && !p.sharing_)
        for (std::size_t from = 0; from < actors_.size() && !p.sharing_; ++from)
            if (actors_[from].autonomous && p.minds_[from].method &&
                std::any_of(p.uses_.begin(), p.uses_.end(), [&](const auto& u) {
                    const auto a = world_.action(u.action);
                    return a && a->request.actor == actors_[from].id &&
                           a->state == ActionState::Completed;
                }))
                for (std::size_t to = 0; to < actors_.size() && !p.sharing_; ++to)
                    if (actors_[to].autonomous &&
                        std::find(p.researchers_.begin(), p.researchers_.end(), actors_[to].id) !=
                            p.researchers_.end())
                        static_cast<void>(share_practice(from, to));
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
    if (p.pending_)
        check(p.pending_->actor < actors_.size() && practice_at_bench(p.pending_->actor),
              "pending practice operator left the apparatus");
    for (std::size_t i = 0; i < actors_.size(); ++i)
        check(p.minds_[i].actor == actors_[i].id && p.minds_[i].probes <= 8 &&
                  p.minds_[i].spent <= 1024,
              "practice actor budget or identity mismatch");
}
} // namespace reality
