#include "research/discovery_world.hpp"
#include "research/hypothesis_agent.hpp"
#include "research/hypothesis_program.hpp"
#include "research/hypothesis_session.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
using namespace reality;
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
HypothesisProgram example() { return {{HypothesisKind::Fraction, 8}, {HypothesisKind::Loss, 8}}; }
void program_case() {
    const HypothesisProgram equivalent{{HypothesisKind::Loss, 16}, {HypothesisKind::Fraction, 8}};
    require(hypothesis_apply(example(), 96) == 40, "novel expression prediction");
    require(hypothesis_table(example()) == hypothesis_table(equivalent), "semantic equivalence");
    const auto grammar = hypothesis_grammar();
    require(grammar.size() == 20440, "grammar must contain all syntax");
    std::set<HypothesisTable> tables;
    for (const auto& p : grammar) {
        const auto table = hypothesis_table(p);
        for (unsigned e = 0; e <= 128; ++e)
            require(table[e] <= e, "active law violates passivity");
        tables.insert(table);
    }
    const auto flat = hypothesis_flat_models();
    for (const auto& p : flat)
        require(p.table != hypothesis_table(example()), "example already flat");
    for (const unsigned d : {128U, 160U, 192U, 256U}) {
        HypothesisTable square{};
        for (unsigned e = 0; e <= 128; ++e)
            square[e] = static_cast<std::uint16_t>(e * e / d);
        require(!tables.contains(square), "outside control inside grammar");
        for (const auto& p : flat)
            require(p.table != square, "outside control inside F");
    }
}
void apparatus_case() {
    std::array<HypothesisTable, 3> tables{hypothesis_table(example()),
                                          hypothesis_table({{HypothesisKind::Fraction, 8}}),
                                          hypothesis_table({{HypothesisKind::Cap, 48}})};
    DiscoveryWorld world{tables};
    require(world.configure(1, {1}, 0, {0, 1}) == DiscoveryOutcome::Accepted, "configure");
    world.advance(1);
    require(world.pulse(2, {1}, 0, 96) == DiscoveryOutcome::Accepted, "pulse");
    require(world.devices()[0].source == 8192 && world.measurements().empty(), "premature spend");
    const auto saved = world.serialize();
    auto loaded = DiscoveryWorld::deserialize(saved);
    loaded.advance(2);
    world.advance(2);
    require(world.serialize() == loaded.serialize(), "pending world continuation");
    require(world.measurements().back().output == 20, "physical composed result");
    require(world.devices()[0].source == 8096 && world.devices()[0].loss == 76, "physical ledger");
    const auto done = world.serialize();
    require(world.pulse(2, {1}, 0, 96) == DiscoveryOutcome::Accepted && world.serialize() == done,
            "idempotency");
    require(world.pulse(3, {1}, 0, 129) == DiscoveryOutcome::Invalid, "table input range");
    tables[0][1] = 2;
    bool rejected = false;
    try {
        DiscoveryWorld invalid{tables};
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "nonpassive table accepted");
}
DiscoveryEvidence evidence(unsigned id, unsigned input, unsigned output) {
    return {{id, id, 0, {1}, id * 2, {0}, input, output}, {1}, id * 2};
}
void learning_case() {
    HypothesisAgent agent{{1}, HypothesisMode::SynthesisActive};
    HypothesisAgent flat{{1}, HypothesisMode::FlatActive};
    for (unsigned n = 1; n <= 3; ++n) {
        const auto input = std::array{16U, 32U, 64U}[n - 1];
        const auto e = evidence(n, input, hypothesis_apply(example(), input));
        require(agent.observe(e) && flat.observe(e), "observation refused");
    }
    require(agent.synthesized() && agent.contradiction_tick() != 0 && !agent.models().empty(),
            "no novel synthesis");
    require(flat.models().empty() && !flat.synthesized(), "flat invented a model");
    bool present = false;
    for (const auto& model : agent.models())
        present |= model.table == hypothesis_table(example());
    require(present, "true model eliminated");
    std::vector<unsigned> order;
    for (unsigned e = 1; e <= 64; ++e)
        order.push_back(e);
    const auto choice = agent.choose_input(order);
    require(choice.input != 16 && choice.input != 32 && choice.input != 64,
            "repeated intervention");
    require(choice.active && choice.worst[choice.input] > 0, "missing active decision");
    for (unsigned e = 1; e <= 64; ++e)
        if (choice.worst[e])
            require(choice.worst[choice.input] <= choice.worst[e], "not minimax");
    const auto prediction = agent.predict({0}, 96);
    require(prediction && prediction->minimum <= 40 && prediction->maximum >= 40,
            "lost true prediction");
    const auto state = agent.state();
    require(!agent.observe(evidence(1, 16, 0)) && agent.state() == state,
            "duplicate changed knowledge");
    agent.freeze();
    require(!agent.observe(evidence(4, 1, 0)), "frozen learned");
    require(hypothesis_chains().size() == 25, "configuration space changed");
}
void limits_case() {
    HypothesisLimits limits;
    limits.reads = 1;
    HypothesisAgent agent{{1}, HypothesisMode::SynthesisActive, limits};
    require(!agent.predict({0}, 96) && agent.exhausted(), "read limit not permanent");
    const auto count = agent.cost().reads;
    require(!agent.predict({0}, 96) && agent.cost().reads == count, "budget reset");
    require(agent.observe(evidence(1, 16, 0)), "exhaustion dropped physical observation");
    std::vector<unsigned> order{7, 16, 32, 64};
    require(agent.choose_input(order).input == 7, "exhaustion must use permutation");
    agent.freeze();
    require(!agent.choose_chain(96, 20), "exhausted planning accepted");
    HypothesisAgent random{{1}, HypothesisMode::SynthesisRandom};
    require(random.choose_input(order).input == 7 && random.cost().reads == 0,
            "random uses active selector");
    limits.reads = 2;
    HypothesisAgent filtering{{1}, HypothesisMode::SynthesisActive, limits};
    require(filtering.observe(evidence(1, 1, 0)) && filtering.exhausted(), "filter budget");
    for (const auto& m : filtering.models())
        require(m.table == hypothesis_table(m.program), "partial filter damaged program");
    limits.reads = hypothesis_flat_models().size() + 1;
    HypothesisAgent during{{1}, HypothesisMode::SynthesisActive, limits};
    require(during.choose_input(order).input == 7 && during.exhausted(),
            "partial active choice escaped exhausted selector");
}
void session_case() {
    HypothesisSession session{HypothesisLaw{example()}, HypothesisMode::SynthesisActive, 123};
    require(session.begin_probe({0}, 96, true) == DiscoveryOutcome::Invalid, "training range");
    require(session.begin_probe({0}, 16, false) == DiscoveryOutcome::Invalid,
            "evaluation before freeze");
    require(session.begin_probe({0}, 16, true) == DiscoveryOutcome::Accepted, "begin");
    require(session.agent().evidence().empty() && session.apparatus().devices()[0].source == 8192,
            "early evidence");
    session.advance(1);
    require(session.agent().evidence().empty(), "pulse observed before completion");
    session.advance(1);
    require(session.agent().evidence().size() == 1 && session.tick() == 2, "physical observation");
    require(session.begin_probe({0}, 16, true) == DiscoveryOutcome::Invalid, "repeated input");
    auto distant = make_discovery_scene();
    distant.spawns[0].location.local = {-7, 0};
    HypothesisSession far{HypothesisLaw{example()}, HypothesisMode::SynthesisActive, 123, distant};
    require(far.begin_probe({0}, 16, true) == DiscoveryOutcome::OutOfReach, "remote observation");
    session.freeze();
    require(session.begin_probe({0}, 32, true) == DiscoveryOutcome::Frozen, "learned after freeze");
    auto evaluation = session.evaluation_copy();
    const auto knowledge = evaluation.agent().evidence().size();
    require(evaluation.begin_probe({0, 1}, 96, false) == DiscoveryOutcome::Accepted,
            "physical evaluation");
    evaluation.advance(2);
    require(evaluation.apparatus().measurements().back().output == 20 &&
                evaluation.agent().evidence().size() == knowledge,
            "evaluation learned");
}
void continuation_case() {
    HypothesisSession session{HypothesisLaw{example()}, HypothesisMode::SynthesisActive, 124};
    for (const auto input : {16U, 32U}) {
        require(session.begin_probe({0}, input, true) == DiscoveryOutcome::Accepted, "probe");
        session.advance(2);
    }
    for (unsigned stage = 0; stage < 3; ++stage) {
        const auto snapshot = session.serialize();
        HypothesisSession loaded{HypothesisLaw{example()}, HypothesisMode::FlatActive, 1};
        loaded.restore(snapshot);
        require(loaded.serialize() == snapshot, "canonical session restore");
        auto uninterrupted = session;
        while (loaded.pending()) {
            loaded.advance(1);
            uninterrupted.advance(1);
        }
        require(loaded.choose_input().input == uninterrupted.choose_input().input,
                "RNG/choice continuation");
        require(loaded.serialize() == uninterrupted.serialize(), "budget continuation");
        if (stage == 0)
            require(session.begin_probe({0}, 64, true) == DiscoveryOutcome::Accepted,
                    "third configure");
        else
            session.advance(1);
    }
    const auto snapshot = session.serialize();
    const auto split = snapshot.find('\n');
    auto body = snapshot.substr(split + 1);
    const auto state_start = body.find("DWORLD");
    require(state_start != std::string::npos, "snapshot physical state absent");
    body[state_start + 9] = body[state_start + 9] == '1' ? '2' : '1';
    const auto corrupt =
        "GENESIS_HYPOTHESIS 1 " + std::to_string(hypothesis_checksum(body)) + "\n" + body;
    bool rejected = false;
    try {
        session.restore(corrupt);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && session.serialize() == snapshot, "semantic corruption changed session");
}
void capacity_case() {
    HypothesisSession full{HypothesisLaw{example()}, HypothesisMode::FlatActive, 4};
    for (std::size_t n = 0; n < record_limit - 2; ++n)
        full.advance(0);
    const auto before = full.serialize();
    require(full.begin_probe({0}, 16, true) == DiscoveryOutcome::Invalid &&
                full.serialize() == before,
            "accepted probe without completion capacity");
    HypothesisSession session{HypothesisLaw{example()}, HypothesisMode::FlatActive, 4};
    for (std::size_t n = 0; n < record_limit - 3; ++n)
        session.advance(0);
    require(session.begin_probe({0}, 16, true) == DiscoveryOutcome::Accepted,
            "reserved probe refused");
    for (unsigned phase = 0; phase < 2; ++phase) {
        const auto pending = session.serialize();
        bool refused = false;
        try {
            (void)session.predict({0}, 96);
        } catch (const std::exception&) {
            refused = true;
        }
        require(refused && session.serialize() == pending, "prediction stole completion capacity");
        auto restored = session;
        restored.restore(pending);
        restored.advance(1);
        session.advance(1);
        require(restored.serialize() == session.serialize(), "near-full pending continuation");
    }
    require(!session.pending() && session.agent().evidence().size() == 1 && session.tick() == 2,
            "reserved probe did not finish at full log");
    auto restored = session;
    restored.restore(session.serialize());
    require(restored.serialize() == session.serialize(), "full-log canonical restore");
}
void interrupted_case() {
    const auto count = hypothesis_flat_models().size();
    HypothesisLimits limits;
    limits.reads = count + 1;
    HypothesisSession session{HypothesisLaw{example()}, HypothesisMode::SynthesisActive, 5,
                              make_discovery_scene(), limits};
    const auto first = session.predict({0}, 96);
    require(first.has_value() && !session.agent().exhausted(), "first declaration failed");
    require(!session.predict({0}, 96) && session.agent().exhausted(),
            "mid-list exhaustion missing");
    const auto saved = session.serialize();
    require(saved.find("p 96 1 0 1 ") != std::string::npos,
            "successful earlier declaration erased");
    auto restored = session;
    restored.restore(saved);
    require(restored.serialize() == saved, "interrupted declarations not persisted");
    for (unsigned n = 0; n < 24; ++n) {
        const auto choice = restored.choose_input();
        require(restored.begin_probe({0}, choice.input, true) == DiscoveryOutcome::Accepted,
                "exhausted physical trial refused");
        restored.advance(2);
        require(!restored.predict({0}, 96), "inference restarted after exhaustion");
    }
    require(restored.agent().evidence().size() == 24 && restored.attempts() == 24 &&
                restored.agent().cost().reads == count + 1,
            "exhaustion dropped trials or changed budget");
    restored.freeze();
    auto evaluation = restored.evaluation_copy();
    require(evaluation.agent().exhausted() && !evaluation.predict({0}, 96) &&
                evaluation.agent().cost().reads == count + 1,
            "evaluation reset budget");
    limits.reads = 35 * count + 1;
    HypothesisSession planner{HypothesisLaw{example()}, HypothesisMode::SynthesisActive, 5,
                              make_discovery_scene(), limits};
    planner.freeze();
    planner = planner.evaluation_copy();
    require(planner.choose_chain(96, 20).has_value(), "first configuration failed");
    const auto checkpoint = planner.serialize();
    auto resumed = planner;
    resumed.restore(checkpoint);
    require(!planner.choose_chain(96, 20) && planner.agent().exhausted() &&
                !resumed.choose_chain(96, 20) && resumed.serialize() == planner.serialize(),
            "interrupted configuration search not preserved");
    require(planner.serialize().find("c 96 20 1 ") != std::string::npos,
            "earlier configuration erased");
}
} // namespace
int main(int argc, char* argv[]) {
    try {
        require(argc == 2, "case required");
        const std::string name{argv[1]};
        if (name == "program")
            program_case();
        else if (name == "session")
            session_case();
        else if (name == "capacity")
            capacity_case();
        else if (name == "interrupted")
            interrupted_case();
        else if (name == "continuation")
            continuation_case();
        else if (name == "learning")
            learning_case();
        else if (name == "limits")
            limits_case();
        else if (name == "apparatus")
            apparatus_case();
        else
            throw std::invalid_argument("unknown case");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
