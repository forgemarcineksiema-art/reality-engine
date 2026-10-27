#include "research/discovery_agent.hpp"
#include "research/discovery_session.hpp"
#include "research/discovery_world.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reality;
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
DiscoveryWorld apparatus() {
    return DiscoveryWorld{{DiscoveryRule{DiscoveryFamily::Loss, 4},
                           DiscoveryRule{DiscoveryFamily::Cap, 20},
                           DiscoveryRule{DiscoveryFamily::Fraction, 8}}};
}
void world_case() {
    auto w = apparatus();
    require(w.configure(1, {1}, 0, {0, 1}) == DiscoveryOutcome::Accepted, "configure refused");
    w.advance(1);
    require(w.pulse(2, {1}, 0, 64) == DiscoveryOutcome::Accepted, "pulse refused");
    require(w.measurements().empty() && w.devices()[0].source == 8192,
            "pulse spent source or observed before completion");
    w.advance(2);
    require(w.measurements().size() == 1 && w.measurements()[0].output == 20,
            "ordered loss/cap result wrong");
    require(w.devices()[0].source == 8128 && w.devices()[0].output == 20 &&
                w.devices()[0].loss == 44,
            "energy balance wrong");
    require(w.configure(3, {1}, 0, {1, 0}) == DiscoveryOutcome::Accepted, "reconfigure refused");
    w.advance(3);
    require(w.pulse(4, {1}, 0, 64) == DiscoveryOutcome::Accepted, "second pulse refused");
    w.advance(4);
    require(w.measurements().back().output == 16, "chain order ignored");
    require(discovery_apply({DiscoveryFamily::Fraction, 3}, 17) == 3,
            "fraction must use integer floor");
    require(discovery_apply({DiscoveryFamily::Loss, 24}, 16) == 0, "loss underflow");
    w.validate();
}
void replay_case() {
    auto w = apparatus();
    require(w.configure(1, {1}, 0, {2}) == DiscoveryOutcome::Accepted, "configure");
    w.advance(1);
    require(w.pulse(2, {1}, 0, 32) == DiscoveryOutcome::Accepted, "pulse");
    const auto pending = w.serialize();
    require(w.pulse(2, {1}, 0, 32) == DiscoveryOutcome::Accepted && w.serialize() == pending,
            "pending replay mutated apparatus");
    require(w.pulse(2, {1}, 0, 64) == DiscoveryOutcome::Conflict && w.serialize() == pending,
            "conflicting command accepted");
    w.advance(2);
    const auto finished = w.serialize();
    require(w.pulse(2, {1}, 0, 32) == DiscoveryOutcome::Accepted && w.serialize() == finished &&
                w.measurements().size() == 1,
            "finished replay spent energy twice");
}
void refusal_case() {
    auto w = apparatus();
    const auto start = w.serialize();
    require(w.configure(1, {1}, 0, {3}) == DiscoveryOutcome::Invalid && w.serialize() == start,
            "invalid symbol changed state");
    require(w.pulse(1, {1}, 0, 32) == DiscoveryOutcome::Busy && w.serialize() == start,
            "unconfigured device fired");
    require(w.configure(1, {1}, 0, {0}) == DiscoveryOutcome::Accepted, "configure");
    const auto configured = w.serialize();
    require(w.pulse(2, {1}, 0, 32) == DiscoveryOutcome::Busy && w.serialize() == configured,
            "configuration took no time");
    w.advance(1);
    require(w.pulse(2, {1}, 0, 8192) == DiscoveryOutcome::Accepted, "full source pulse");
    const auto busy = w.serialize();
    require(w.configure(3, {1}, 0, {1}) == DiscoveryOutcome::Busy && w.serialize() == busy,
            "active pulse reconfigured");
    w.advance(2);
    const auto empty = w.serialize();
    require(w.pulse(3, {1}, 0, 1) == DiscoveryOutcome::NoEnergy && w.serialize() == empty,
            "empty source refilled or mutated");
}
void world_restore_case() {
    auto original = apparatus();
    require(original.configure(1, {2}, 1, {2, 2}) == DiscoveryOutcome::Accepted, "configure");
    original.advance(1);
    require(original.pulse(2, {2}, 1, 64) == DiscoveryOutcome::Accepted, "pulse");
    auto copy = DiscoveryWorld::deserialize(original.serialize());
    original.advance(2);
    copy.advance(2);
    require(original.serialize() == copy.serialize() && copy.measurements().back().output == 16,
            "inflight continuation diverged");
    bool rejected = false;
    try {
        static_cast<void>(DiscoveryWorld::deserialize(copy.serialize() + "corruption"));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "trailing corruption accepted");
}
DiscoveryEvidence evidence(std::uint64_t id, unsigned symbol, unsigned input, unsigned output) {
    return {
        {id, id + 100, 0, {1}, id, {static_cast<std::uint8_t>(symbol)}, input, output}, {1}, id};
}
void teach(DiscoveryAgent& agent) {
    require(agent.observe(evidence(1, 0, 16, 12)), "loss observation");
    require(agent.observe(evidence(2, 0, 64, 60)), "loss observation 2");
    require(agent.observe(evidence(3, 1, 16, 16)), "cap observation");
    require(agent.observe(evidence(4, 1, 64, 20)), "cap observation 2");
    require(agent.observe(evidence(5, 2, 16, 8)), "fraction observation");
    require(agent.observe(evidence(6, 2, 64, 32)), "fraction observation 2");
}
void learning_case() {
    DiscoveryAgent a{{1}};
    require(a.counts() == std::array<std::size_t, 3>{75, 75, 75}, "agent seeded with truth");
    teach(a);
    require(a.counts() == std::array<std::size_t, 3>{1, 1, 1}, "agent failed identification");
    require(a.predict({0}, 112).median == 108, "new input not predicted");
    const auto before = a.serialize();
    require(!a.observe(evidence(1, 0, 16, 12)) && a.serialize() == before,
            "duplicate was new evidence");
    auto copy = DiscoveryAgent::deserialize(before);
    require(copy.serialize() == before, "agent continuation lost evidence");
}
void minimax_case() {
    DiscoveryAgent a{{1}};
    const auto first = a.choose_probe();
    require(first && first->input == 64 && first->symbol == 0, "minimax first choice");
    require(a.observe(evidence(1, 0, 16, 12)), "observation");
    require(a.observe(evidence(2, 0, 64, 60)), "observation");
    const auto next = a.choose_probe();
    require(next && next->symbol != 0, "minimax repeatedly probes identified symbol");
    teach(a = DiscoveryAgent{{1}});
    require(!a.choose_probe(), "agent keeps probing after identification");
}
void composition_case() {
    DiscoveryAgent a{{1}};
    teach(a);
    require(a.predict({2, 2}, 80).median == 20, "repeated symbol resampled its rule");
    require(a.predict({1, 0}, 80).median == 16 && a.predict({0, 1}, 80).median == 20,
            "composition lost order");
    const auto selected = a.choose_chain(80, 16);
    require(a.predict(selected, 80).median == 16, "chosen configuration misses target");
    DiscoveryAgent b{{2}};
    const auto packet = a.teaching_packet();
    require(packet.size() <= 6 && !packet.empty(), "invalid teaching packet");
    for (auto record : packet) {
        record.via_actor = {1};
        record.learned_tick = 20;
        static_cast<void>(b.observe(record));
    }
    require(b.counts() == std::array<std::size_t, 3>{1, 1, 1} &&
                b.predict({2, 2}, 80).median == 20 && b.teaching_packet().empty(),
            "student failed inference or relayed received records as own discovery");
}
void frozen_case() {
    DiscoveryAgent a{{1}};
    a.freeze();
    const auto before = a.serialize();
    bool rejected = false;
    try {
        static_cast<void>(a.observe(evidence(1, 0, 16, 12)));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && a.serialize() == before, "frozen evaluation learned from outcome");
    require(DiscoveryAgent::deserialize(before).frozen(), "freeze lost on restore");
}
void outside_case() {
    DiscoveryAgent a{{1}};
    require(a.observe(evidence(1, 0, 16, 0)), "outside first observation");
    require(a.observe(evidence(2, 0, 32, 32)), "outside second observation");
    require(a.inadequate() && !a.choose_probe(), "inadequate model silently repaired");
    bool rejected = false;
    try {
        static_cast<void>(a.predict({0}, 64));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "inadequate model gave an invented prediction");
}
DiscoverySession session() {
    return DiscoverySession{{DiscoveryRule{DiscoveryFamily::Loss, 4},
                             DiscoveryRule{DiscoveryFamily::Cap, 20},
                             DiscoveryRule{DiscoveryFamily::Fraction, 8}}};
}
void access_case() {
    auto s = session();
    const auto before = s.serialize();
    require(s.begin_probe(0, {0}, 64) == DiscoveryOutcome::Accepted, "local probe refused");
    require(s.agent(0).evidence().empty(), "measurement delivered before impulse");
    s.advance(1);
    require(s.agent(0).evidence().empty(), "configuration already taught agent");
    s.advance(1);
    require(s.agent(0).evidence().size() == 1 && s.agent(0).evidence()[0].measurement.output == 60,
            "local observation missing");
    s.set_goal(0, {1, {0, 0}});
    for (unsigned t = 0; t < 150; ++t)
        s.advance(1);
    const auto remote = s.serialize();
    require(s.begin_probe(0, {0}, 64) == DiscoveryOutcome::OutOfReach && s.serialize() == remote,
            "remote probe learned global truth");
    require(before != remote, "physical session never advanced");
}
void contact_case() {
    auto s = session();
    s.train(0);
    require(s.agent(0).counts() == std::array<std::size_t, 3>{1, 1, 1}, "teacher did not learn");
    require(s.agent(1).evidence().empty(), "student observed remote apparatus");
    const auto before = s.serialize();
    require(s.share(0, 1, true) == DiscoveryOutcome::OutOfReach && s.serialize() == before,
            "remote teaching accepted");
    const auto start = s.tick();
    require(s.meet_and_share(true) == DiscoveryOutcome::Accepted && s.tick() > start + 1,
            "teaching skipped actual travel");
    require(s.agent(1).counts() == std::array<std::size_t, 3>{1, 1, 1},
            "student model copied badly");
    for (const auto& e : s.agent(1).evidence())
        require(e.measurement.source_actor == s.agent(0).actor() &&
                    e.via_actor == s.agent(0).actor() &&
                    e.learned_tick > e.measurement.observed_tick,
                "teaching source or age rewritten");
    const auto evaluation = s.evaluation_copy();
    require(evaluation.apparatus().devices()[0].source == 8192 &&
                evaluation.agent(1).evidence() == s.agent(1).evidence(),
            "independent evaluation failed to retain knowledge/reset fixture");
}
void isolation_case() {
    auto a = session();
    DiscoverySession b{{DiscoveryRule{DiscoveryFamily::Cap, 40},
                        DiscoveryRule{DiscoveryFamily::Fraction, 4},
                        DiscoveryRule{DiscoveryFamily::Loss, 12}}};
    require(a.agent(0).choose_probe() == b.agent(0).choose_probe(),
            "unobserved truth changed decision");
    a.train(0);
    const auto checkpoint = a.serialize();
    auto no_message = session();
    no_message.restore(checkpoint);
    require(no_message.meet_and_share(false) == DiscoveryOutcome::Accepted &&
                no_message.agent(1).evidence().empty(),
            "empty control transmitted knowledge");
    require(a.agent(1).evidence().empty(), "checkpoint branches share knowledge state");
}
void continuation_case() {
    auto s = session();
    require(s.begin_probe(0, {0}, 64) == DiscoveryOutcome::Accepted, "begin");
    s.advance(1);
    auto copy = session();
    copy.restore(s.serialize());
    for (unsigned n = 0; n < 4; ++n) {
        s.advance(1);
        copy.advance(1);
        require(s.serialize() == copy.serialize(), "inflight session diverged");
    }
    s.train(0);
    require(s.meet_and_share(true) == DiscoveryOutcome::Accepted, "share");
    auto eval = s.evaluation_copy();
    eval.freeze(1);
    copy.restore(eval.serialize());
    require(eval.probe(1, {2, 2}, 80, false) && copy.probe(1, {2, 2}, 80, false) &&
                eval.serialize() == copy.serialize(),
            "frozen/received knowledge continuation diverged");
    auto random = session();
    for (unsigned i = 0; i < 5; ++i)
        static_cast<void>(random.random_chain(0));
    copy.restore(random.serialize());
    for (unsigned i = 0; i < 8; ++i)
        require(random.random_chain(0) == copy.random_chain(0) &&
                    random.random_chain(1) == copy.random_chain(1),
                "random control stream lost across snapshot");
}
void corruption_case() {
    auto s = session();
    s.train(0);
    const auto before = s.serialize();
    auto corrupt = before;
    corrupt[corrupt.size() / 2] ^= 1;
    bool rejected = false;
    try {
        s.restore(corrupt);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before, "corrupt save replaced live session");
    const auto alter_pending = [](const std::string& snapshot, const std::string& from,
                                  const std::string& to) {
        const auto header = snapshot.find('\n') + 1;
        const auto marker = snapshot.rfind("\nCHECKSUM ");
        auto payload = snapshot.substr(header, marker + 1 - header);
        const auto position = payload.find(from);
        require(position != std::string::npos, "pending fixture not found");
        payload.replace(position, from.size(), to);
        return snapshot.substr(0, header) + payload + "CHECKSUM " +
               std::to_string(snapshot_checksum(payload)) + "\n";
    };
    auto inflight = session();
    require(inflight.begin_probe(0, {0}, 64) == DiscoveryOutcome::Accepted, "pending fixture");
    const auto configured = inflight.serialize();
    rejected = false;
    try {
        inflight.restore(alter_pending(configured, "\n1 0 0 2 64 1\n", "\n1 0 0 2 32 1\n"));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && inflight.serialize() == configured,
            "semantic pending input corruption accepted with valid checksum");
    inflight.advance(1);
    const auto pulsing = inflight.serialize();
    rejected = false;
    try {
        inflight.restore(alter_pending(pulsing, "\n1 0 1 2 64 1\n", "\n1 0 1 2 64 0\n"));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && inflight.serialize() == pulsing,
            "semantic observation suppression accepted with valid checksum");
}
void blocked_share_case() {
    auto scene = make_discovery_scene();
    scene.spawns[0].location = {1, {-.8, 0}};
    scene.spawns[1].location = {1, {.8, 0}};
    scene.geometry.bodies.push_back({2, {0, 2, 0}, 0, .1, 2, 2, 0, 0, false});
    auto s = DiscoverySession{{DiscoveryRule{DiscoveryFamily::Loss, 4},
                               DiscoveryRule{DiscoveryFamily::Cap, 20},
                               DiscoveryRule{DiscoveryFamily::Fraction, 8}},
                              scene};
    const auto before = s.serialize();
    require(s.share(0, 1, true) == DiscoveryOutcome::OutOfReach && s.serialize() == before,
            "teaching crossed wall");
    scene.geometry.bodies.pop_back();
    scene.geometry.bodies.push_back({2, {0, 0, 0}, 0, 8, 4, .3, 0, 0, true});
    scene.spawns[1].location.surface = 2;
    DiscoverySession other{{DiscoveryRule{DiscoveryFamily::Loss, 4},
                            DiscoveryRule{DiscoveryFamily::Cap, 20},
                            DiscoveryRule{DiscoveryFamily::Fraction, 8}},
                           scene};
    require(other.share(0, 1, true) == DiscoveryOutcome::OutOfReach,
            "teaching crossed support boundary");
}
void pending_observation_case() {
    auto s = session();
    require(s.begin_probe(0, {0}, 64) == DiscoveryOutcome::Accepted, "pending observation fixture");
    s.advance(1);
    const auto before = s.serialize();
    const auto header = before.find('\n') + 1;
    const auto marker = before.rfind("\nCHECKSUM ");
    auto payload = before.substr(header, marker + 1 - header);
    const auto location = payload.find("\n1 0 1 2 64 1\n");
    require(location != std::string::npos, "pending observation fixture line");
    payload.replace(location, std::string("\n1 0 1 2 64 1\n").size(), "\n1 0 1 2 64 0\n");
    const auto modified = before.substr(0, header) + payload + "CHECKSUM " +
                          std::to_string(snapshot_checksum(payload)) + "\n";
    bool rejected = false;
    try {
        s.restore(modified);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before, "pending learning flag corruption accepted");
}
void budgets_case() {
    auto s = session();
    s.train(1);
    require(s.training_attempts(1) <= 2, "student exceeded probe budget");
    const auto before = s.serialize();
    require(s.begin_probe(1, {0}, 64) != DiscoveryOutcome::Accepted && s.serialize() == before,
            "exhausted/frozen student was granted extra training");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "case required");
        const std::string name = argv[1];
        if (name == "world")
            world_case();
        else if (name == "replay")
            replay_case();
        else if (name == "refusals")
            refusal_case();
        else if (name == "world_restore")
            world_restore_case();
        else if (name == "learning")
            learning_case();
        else if (name == "minimax")
            minimax_case();
        else if (name == "composition")
            composition_case();
        else if (name == "frozen")
            frozen_case();
        else if (name == "outside")
            outside_case();
        else if (name == "access")
            access_case();
        else if (name == "contact")
            contact_case();
        else if (name == "isolation")
            isolation_case();
        else if (name == "continuation")
            continuation_case();
        else if (name == "corruption")
            corruption_case();
        else if (name == "blocked_share")
            blocked_share_case();
        else if (name == "budgets")
            budgets_case();
        else if (name == "pending_observation")
            pending_observation_case();
        else
            throw std::runtime_error("unknown case");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
