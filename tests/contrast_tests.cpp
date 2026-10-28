#include <iostream>
#include <stdexcept>
#include <string>
#if __has_include("research/contrast_session.hpp")
#include "research/contrast_session.hpp"
#include <algorithm>
#include <numeric>
using namespace reality;
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
HypothesisLaw example() { return {{{HypothesisKind::Loss, 48}, {HypothesisKind::Fraction, 12}}}; }
void train(ContrastSession& s) {
    for (unsigned i = 0; i < 8; ++i) {
        const auto c = s.choose(0);
        require(s.begin_probe(0, {0}, c.input, true) == DiscoveryOutcome::Accepted, "training");
        s.advance(2);
    }
    s.freeze(0);
}
void agent() {
    ContrastAgent a{{1}, true};
    require(a.models().size() == 2370, "all competitors must exist before contradiction");
    std::vector<unsigned> order(128);
    std::iota(order.begin(), order.end(), 1U);
    for (unsigned n = 1; n <= 8; ++n) {
        const auto c = a.choose(order);
        require(c.input >= 1 && c.input <= 128, "extended apparatus");
        if (c.active) {
            for (const auto x : order) {
                std::array<unsigned, 129> buckets{};
                for (const auto& model : a.models())
                    ++buckets[model.table[x]];
                require(c.worst <= *std::max_element(buckets.begin(), buckets.end()),
                        "not minimax");
            }
        }
        const auto y = example().table()[c.input];
        require(a.observe({{n, n, 0, {1}, n * 2, {0}, c.input, y}, {1}, n * 2}), "learn");
    }
    require(a.models().size() == 1 && a.models()[0].table == example().table(), "failed identify");
    const auto p = a.predict({0}, 96);
    require(p && p->median == 36 && p->minimum == 36 && p->maximum == 36, "wrong projection");
    ContrastAgent exhausted{{1}, true, {1, 1}};
    require(exhausted.exhausted() && !exhausted.predict({0}, 96), "partial search looks certain");
}
void physical() {
    ContrastSession s{example(), true, 1};
    require(s.share(true) == DiscoveryOutcome::OutOfReach, "remote communication");
    require(s.begin_probe(1, {0}, 96, true) == DiscoveryOutcome::Accepted, "second bench");
    const auto before = s.apparatus().devices()[1].source;
    s.advance(1);
    require(s.apparatus().devices()[1].source == before, "premature energy spend");
    s.advance(1);
    require(s.apparatus().devices()[1].source == before - 96, "physical energy absent");
    auto saved = s.serialize();
    s.restore(saved);
    require(s.serialize() == saved, "roundtrip");
    saved.back() = saved.back() == 'X' ? 'Y' : 'X';
    const auto unchanged = s.serialize();
    bool rejected = false;
    try {
        s.restore(saved);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == unchanged, "nontransactional corrupt restore");
}
void work() {
    ContrastSession s{example(), true, 1};
    train(s);
    const auto parts = s.parts(0);
    require(s.begin_work(0, {0}, 96, 35) == DiscoveryOutcome::Accepted, "bad work impulse");
    s.advance(2);
    require(!s.work_started() && s.parts(0) == parts && s.completed(0) == 0,
            "wrong calibration worked");
    require(s.begin_work(0, {0}, 96, 36) == DiscoveryOutcome::Accepted, "good work impulse");
    s.advance(2);
    require(s.work_started() && s.parts(0) == parts - 1 && s.completed(0) == 0,
            "missing real action");
    const auto checkpoint = s.serialize();
    s.advance(80);
    require(s.completed(0) == 1 && s.parts(0) == parts - 1, "missing physical completion");
    const auto expected = s.serialize();
    s.restore(checkpoint);
    s.advance(80);
    require(s.serialize() == expected, "pending work replays differently");
    auto control = s;
    require(s.meet_and_share(true) == DiscoveryOutcome::Accepted, "meeting");
    require(control.meet_and_share(false) == DiscoveryOutcome::Accepted, "control meeting");
    require(s.tick() == control.tick(), "unequal contact time");
    require(s.agent(1).evidence().size() == 8 && control.agent(1).evidence().empty(), "bad packet");
    for (const auto& e : s.agent(1).evidence())
        require(e.measurement.source_actor == s.traversal().actors()[0].id &&
                    e.via_actor == e.measurement.source_actor &&
                    e.learned_tick > e.measurement.observed_tick,
                "lost provenance");
    for (unsigned i = 0; i < 2; ++i) {
        const auto c = s.choose(1);
        require(s.begin_probe(1, {0}, c.input, true) == DiscoveryOutcome::Accepted, "B probe");
        s.advance(2);
    }
    s.freeze(1);
    const auto chain = s.choose_chain(1, 96, 36);
    require(chain.has_value(), "B learned no method");
    require(s.begin_work(1, *chain, 96, 36) == DiscoveryOutcome::Accepted, "B calibration");
    s.advance(82);
    require(s.completed(1) == 1 && s.parts(1) == 3, "B did not take over work");
}
void pending() {
    for (unsigned stage = 0; stage < 2; ++stage) {
        ContrastSession s{example(), true, 1};
        require(s.begin_probe(0, {0}, 96, true) == DiscoveryOutcome::Accepted, "pending begin");
        if (stage)
            s.advance(1);
        const auto checkpoint = s.serialize();
        s.advance(2 - stage);
        const auto expected = s.serialize();
        s.restore(checkpoint);
        s.advance(2 - stage);
        require(s.serialize() == expected, "pending probe continuation");
    }
}
void depleted() {
    ContrastSession s{example(), true, 1};
    s.freeze(0);
    for (unsigned i = 0; i < 64; ++i) {
        require(s.begin_probe(0, {0}, 128, false) == DiscoveryOutcome::Accepted, "deplete pulse");
        s.advance(2);
    }
    const auto before = s.serialize();
    require(s.begin_probe(0, {0}, 128, false) == DiscoveryOutcome::NoEnergy,
            "accepted an impulse without energy");
    require(s.serialize() == before, "energy refusal mutated state");
}
void boundary() {
    ContrastSession late{example(), true, 1};
    late.advance(9999);
    require(late.begin_probe(0, {0}, 96, true) == DiscoveryOutcome::Invalid,
            "accepted impulse beyond session horizon");
    ContrastSession s{example(), true, 1};
    s.freeze(0);
    for (unsigned i = 0; i < 4092; ++i)
        static_cast<void>(s.predict(0, {0}, 96));
    require(s.begin_work(0, {0}, 96, 36) == DiscoveryOutcome::Accepted, "boundary work");
    s.advance(2);
    bool refused = false;
    try {
        s.advance(1);
    } catch (const std::exception&) {
        refused = true;
    }
    require(refused, "consumed reserved work-completion journal slot");
    s.advance(80);
    require(s.completed(0) == 1, "journal stranded a worker");
}
} // namespace
#endif
int main(int argc, char** argv) {
    try {
#if __has_include("research/contrast_session.hpp")
        const std::string name = argc == 2 ? argv[1] : "";
        if (name == "agent")
            agent();
        else if (name == "physical")
            physical();
        else if (name == "work")
            work();
        else if (name == "pending")
            pending();
        else if (name == "depleted")
            depleted();
        else if (name == "boundary")
            boundary();
        else
            throw std::runtime_error("unknown contrast case");
        std::cout << "PASS " << name << '\n';
#else
        static_cast<void>(argc);
        static_cast<void>(argv);
        throw std::runtime_error("missing competing-model agent and physical task session");
#endif
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
