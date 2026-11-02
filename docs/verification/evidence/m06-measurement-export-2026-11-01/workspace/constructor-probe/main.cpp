#include "research/contrast_agent.hpp"
#include "persistence/world_snapshot.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <string_view>
using namespace reality;
struct FullHash {
    std::size_t operator()(const HypothesisTable& table) const noexcept {
        return std::hash<std::string_view>{}({reinterpret_cast<const char*>(table.data()), table.size()*sizeof(table[0])});
    }
};
struct SampleHash {
    std::size_t operator()(const HypothesisTable& table) const noexcept {
        std::uint64_t h=14695981039346656037ULL;
        for(std::size_t i=0;i<table.size();i+=8) { h^=table[i]; h*=1099511628211ULL; }
        return static_cast<std::size_t>(h);
    }
};
template<class Hash> std::vector<HypothesisModel> candidate_models() {
    auto programs=hypothesis_grammar();
    const auto grammar_size=programs.size();
    auto flat=hypothesis_flat_programs();
    const auto less=[](const auto& a,const auto& b){return a.size()!=b.size()?a.size()<b.size():a<b;};
    std::sort(flat.begin(),flat.end(),less);
    programs.insert(programs.end(),flat.begin(),flat.end());
    std::inplace_merge(programs.begin(),programs.begin()+static_cast<std::ptrdiff_t>(grammar_size),programs.end(),less);
    std::unordered_set<HypothesisTable,Hash> seen;
    std::vector<HypothesisModel> models;
    for(auto& p:programs) {
        auto table=hypothesis_table(p);
        if(seen.insert(table).second) models.push_back({std::move(p),table});
    }
    return models;
}
template<class F> void measure(const char* name, F operation) {
    const auto start = std::chrono::steady_clock::now();
    operation();
    std::cout << name << "_seconds="
              << std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count() << '\n';
}
int main() {
    measure("grammar", [] { std::cout << "grammar_count=" << hypothesis_grammar().size() << '\n'; });
    measure("flat", [] { std::cout << "flat_count=" << hypothesis_flat_programs().size() << '\n'; });
    for(unsigned i=1;i<=3;++i) {
        measure("agent", [i] {
            ContrastAgent agent{EntityId{i}, true};
            std::cout << "models=" << agent.models().size() << " primitives=" << agent.cost().primitives
                      << " state_hash=" << snapshot_checksum(agent.state()) << '\n';
        });
    }
    for(const auto budget : {0ULL, 128ULL, 129ULL, 1000ULL, 100000ULL, 10000000ULL}) {
        ContrastAgent agent{EntityId{10}, true, HypothesisLimits{budget, 300000000}};
        std::cout << "budget=" << budget << " state_hash=" << snapshot_checksum(agent.state())
                  << " models=" << agent.models().size() << " generated=" << agent.cost().generated
                  << " primitives=" << agent.cost().primitives << " exhausted=" << agent.exhausted() << '\n';
    }
    ContrastAgent reference{EntityId{10},true};
    const auto compare=[&](const auto& models){
        if(models.size()!=reference.models().size()) throw std::runtime_error("candidate count");
        for(std::size_t i=0;i<models.size();++i)
            if(models[i].program!=reference.models()[i].program || models[i].table!=reference.models()[i].table)
                throw std::runtime_error("candidate order/table");
    };
    for(unsigned repeat=0;repeat<3;++repeat) {
        measure("current",[]{ContrastAgent a{EntityId{10},true}; std::cout<<a.models().size()<<'\n';});
        measure("full_hash",[&]{compare(candidate_models<FullHash>());});
        measure("sample_hash",[&]{compare(candidate_models<SampleHash>());});
    }
}
