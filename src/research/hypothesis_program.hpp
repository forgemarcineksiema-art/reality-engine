#pragma once
#include "research/discovery_world.hpp"
#include <compare>
namespace reality {
enum class HypothesisKind : unsigned { Loss, Cap, Fraction, Gate };
struct HypothesisOp {
    HypothesisKind kind;
    unsigned parameter;
    auto operator<=>(const HypothesisOp&) const = default;
};
using HypothesisProgram = std::vector<HypothesisOp>;
using HypothesisTable = DiscoveryTable;
struct HypothesisModel {
    HypothesisProgram program;
    HypothesisTable table;
};
[[nodiscard]] unsigned hypothesis_apply(const HypothesisProgram& program, unsigned input);
[[nodiscard]] HypothesisTable hypothesis_table(const HypothesisProgram& program);
[[nodiscard]] const std::vector<HypothesisOp>& hypothesis_primitives();
[[nodiscard]] std::vector<HypothesisProgram> hypothesis_grammar();
[[nodiscard]] std::vector<HypothesisModel> hypothesis_flat_models();
[[nodiscard]] std::vector<HypothesisProgram> hypothesis_flat_programs();
[[nodiscard]] std::vector<HypothesisModel>
hypothesis_canonical(std::vector<HypothesisProgram> programs);
[[nodiscard]] const std::vector<DiscoveryChain>& hypothesis_chains();
} // namespace reality
