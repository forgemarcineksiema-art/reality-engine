#include "research/discovery_world.hpp"
#include "research/hypothesis_program.hpp"
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
} // namespace
int main(int argc, char* argv[]) {
    try {
        require(argc == 2, "case required");
        const std::string name{argv[1]};
        if (name == "program")
            program_case();
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
