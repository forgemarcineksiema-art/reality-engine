#include "research/hypothesis_program.hpp"
#include "research/discovery_agent.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace reality {
unsigned hypothesis_apply(const HypothesisProgram& program, unsigned input) {
    if (input > 128 || program.size() > 3)
        throw std::invalid_argument("hypothesis expression range");
    auto value = input;
    for (const auto& op : program) {
        if (op.parameter == 0 || op.parameter > 64)
            throw std::invalid_argument("hypothesis constant range");
        switch (op.kind) {
        case HypothesisKind::Loss:
            value = value > op.parameter ? value - op.parameter : 0;
            break;
        case HypothesisKind::Cap:
            value = std::min(value, op.parameter);
            break;
        case HypothesisKind::Fraction:
            if (op.parameter > 16)
                throw std::invalid_argument("hypothesis fraction range");
            value = value * op.parameter / 16;
            break;
        case HypothesisKind::Gate:
            value = value >= op.parameter ? value : 0;
            break;
        default:
            throw std::invalid_argument("hypothesis operator");
        }
    }
    return value;
}
HypothesisTable hypothesis_table(const HypothesisProgram& program) {
    HypothesisTable table{};
    for (unsigned e = 0; e <= 128; ++e)
        table[e] = static_cast<std::uint16_t>(hypothesis_apply(program, e));
    return table;
}
const std::vector<HypothesisOp>& hypothesis_primitives() {
    static const auto ops = [] {
        std::vector<HypothesisOp> result;
        for (const auto kind : {HypothesisKind::Loss, HypothesisKind::Cap, HypothesisKind::Fraction,
                                HypothesisKind::Gate}) {
            if (kind == HypothesisKind::Fraction) {
                for (const auto q : {4U, 8U, 12U})
                    result.push_back({kind, q});
            } else {
                for (const auto c : {4U, 8U, 12U, 16U, 24U, 32U, 48U, 64U})
                    result.push_back({kind, c});
            }
        }
        return result;
    }();
    return ops;
}
std::vector<HypothesisProgram> hypothesis_grammar() {
    std::vector<HypothesisProgram> result(1);
    std::size_t start = 0, end = 1;
    for (unsigned depth = 0; depth < 3; ++depth) {
        for (auto i = start; i < end; ++i)
            for (const auto op : hypothesis_primitives()) {
                auto expression = result[i];
                expression.push_back(op);
                result.push_back(std::move(expression));
            }
        start = end;
        end = result.size();
    }
    return result;
}
std::vector<HypothesisModel> hypothesis_canonical(std::vector<HypothesisProgram> programs) {
    std::sort(programs.begin(), programs.end(), [](const auto& a, const auto& b) {
        return a.size() != b.size() ? a.size() < b.size() : a < b;
    });
    std::set<HypothesisTable> seen;
    std::vector<HypothesisModel> models;
    for (auto& expression : programs) {
        auto table = hypothesis_table(expression);
        if (seen.insert(table).second)
            models.push_back({std::move(expression), table});
    }
    return models;
}
std::vector<HypothesisModel> hypothesis_flat_models() {
    std::vector<HypothesisProgram> programs(1);
    for (const auto op : hypothesis_primitives())
        programs.push_back({op});
    for (unsigned q = 3; q <= 12; ++q)
        programs.push_back({{HypothesisKind::Fraction, q}});
    for (unsigned d = 1; d <= 24; ++d)
        programs.push_back({{HypothesisKind::Loss, d}});
    for (unsigned c = 8; c <= 48; ++c)
        programs.push_back({{HypothesisKind::Cap, c}});
    return hypothesis_canonical(std::move(programs));
}
const std::vector<DiscoveryChain>& hypothesis_chains() {
    static const auto chains = [] {
        std::vector<DiscoveryChain> result;
        for (const auto& chain : discovery_chains())
            if (std::find(chain.begin(), chain.end(), 0) != chain.end())
                result.push_back(chain);
        return result;
    }();
    return chains;
}
} // namespace reality
