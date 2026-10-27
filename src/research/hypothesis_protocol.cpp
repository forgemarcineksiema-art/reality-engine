#include "research/hypothesis_protocol.hpp"
#include "research/hypothesis_session.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <Psapi.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <fstream>
#include <set>
#include <stdexcept>
namespace reality {
namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
unsigned number(std::istream& in, unsigned maximum = 128) {
    std::string token;
    check(static_cast<bool>(in >> token), "missing hypothesis case integer");
    unsigned value = 0;
    const auto p = std::from_chars(token.data(), token.data() + token.size(), value);
    check(p.ec == std::errc{} && p.ptr == token.data() + token.size() && value <= maximum,
          "invalid hypothesis case integer");
    return value;
}
DiscoveryChain chain_in(std::istream& in) {
    DiscoveryChain chain;
    const auto size = number(in, 3);
    check(size > 0, "empty case chain");
    for (unsigned n = 0; n < size; ++n)
        chain.push_back(static_cast<std::uint8_t>(number(in, 2)));
    check(std::find(chain.begin(), chain.end(), 0) != chain.end(), "case bypasses unknown element");
    return chain;
}
struct Forecast {
    unsigned input;
    DiscoveryChain chain;
};
struct Goal {
    unsigned input, target;
};
struct Case {
    std::uint32_t seed;
    HypothesisLaw law;
    std::array<Forecast, 24> forecasts;
    std::array<Goal, 20> goals;
    std::vector<Goal> zeros;
    std::array<unsigned, 3> counters;
};
bool evaluation_input(unsigned input) {
    return input == 80 || input == 96 || input == 112 || input == 128;
}
Case read_case(const std::filesystem::path& path) {
    check(std::filesystem::file_size(path) < 65536, "hypothesis case size");
    std::ifstream in{path};
    std::string magic;
    in >> magic;
    check(magic == "HCASE" && number(in, 1) == 1, "hypothesis case header");
    Case c{};
    c.seed = number(in, UINT32_MAX);
    c.law.square_divisor = number(in, 256);
    const auto size = number(in, 3);
    for (unsigned n = 0; n < size; ++n)
        c.law.program.push_back({static_cast<HypothesisKind>(number(in, 3)), number(in, 64)});
    (void)c.law.table();
    std::set<std::pair<unsigned, DiscoveryChain>> distinct;
    for (unsigned n = 0; n < 24; ++n) {
        auto& forecast = c.forecasts[n];
        forecast.input = number(in);
        forecast.chain = chain_in(in);
        check(n < 8 ? (forecast.input == 72 + 8 * n && forecast.chain == DiscoveryChain{0})
                    : (evaluation_input(forecast.input) && forecast.chain.size() > 1),
              "hypothesis forecast plan");
        check(distinct.insert({forecast.input, forecast.chain}).second, "duplicate forecast");
    }
    for (auto& goal : c.goals) {
        goal = {number(in), number(in)};
        check(evaluation_input(goal.input) && goal.target > 0 && goal.target <= goal.input,
              "hypothesis goal plan");
    }
    const auto zeros = number(in, 4);
    std::set<unsigned> zero_inputs;
    for (unsigned n = 0; n < zeros; ++n) {
        const Goal g{number(in), number(in, 0)};
        check(evaluation_input(g.input) && zero_inputs.insert(g.input).second,
              "hypothesis zero plan");
        c.zeros.push_back(g);
    }
    for (auto& counter : c.counters)
        counter = number(in, (1U << 24) - 1);
    in >> std::ws;
    check(in.eof(), "trailing hypothesis case");
    return c;
}
template <class Range> void numbers(std::ostream& out, const Range& values) {
    out << '[';
    bool comma = false;
    for (const auto value : values) {
        if (comma)
            out << ',';
        comma = true;
        out << static_cast<std::uint64_t>(value);
    }
    out << ']';
}
void measurement(std::ostream& out, const DiscoveryMeasurement& m) {
    out << "{\"id\":" << m.id << ",\"command\":" << m.command << ",\"tick\":" << m.observed_tick
        << ",\"actor\":" << m.source_actor.value << ",\"apparatus\":" << m.apparatus
        << ",\"input\":" << m.input << ",\"output\":" << m.output << ",\"chain\":";
    numbers(out, m.chain);
    out << '}';
}
void state(std::ostream& out, const HypothesisAgent& agent) {
    const auto& cost = agent.cost();
    out << "{\"models\":" << agent.models().size() << ",\"synthesized\":" << agent.synthesized()
        << ",\"contradiction_tick\":" << agent.contradiction_tick()
        << ",\"exhausted\":" << agent.exhausted() << ",\"primitives\":" << cost.primitives
        << ",\"reads\":" << cost.reads << ",\"generated\":" << cost.generated << '}';
}
void ledger(std::ostream& out, const DiscoveryWorld& world) {
    const auto& d = world.devices()[0];
    out << "{\"source\":" << d.source << ",\"output\":" << d.output << ",\"loss\":" << d.loss
        << '}';
}
double cpu_ms() {
    FILETIME created{}, exited{}, kernel{}, user{};
    check(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user) != 0,
          "CPU measurement unavailable");
    const auto value = [](FILETIME t) {
        return (static_cast<std::uint64_t>(t.dwHighDateTime) << 32) | t.dwLowDateTime;
    };
    return static_cast<double>(value(kernel) + value(user)) / 10000.;
}
void goals(std::ostream& out, HypothesisSession& session, std::span<const Goal> tasks) {
    out << '[';
    bool comma = false;
    for (const auto goal : tasks) {
        if (comma)
            out << ',';
        comma = true;
        const auto chain = session.choose_chain(goal.input, goal.target);
        out << "{\"input\":" << goal.input << ",\"target\":" << goal.target << ",\"chain\":";
        if (chain)
            numbers(out, *chain);
        else
            out << "null";
        out << ",\"measurement\":";
        if (chain) {
            check(session.begin_probe(*chain, goal.input, false) == DiscoveryOutcome::Accepted,
                  "goal physical impulse refused");
            session.advance(2);
            measurement(out, session.apparatus().measurements().back());
        } else
            out << "null";
        out << '}';
    }
    out << ']';
}
void variant(std::ostream& out, const Case& c, HypothesisMode mode,
             const std::filesystem::path& directory, const char* name) {
    const auto started = std::chrono::steady_clock::now();
    const auto cpu_start = cpu_ms();
    HypothesisSession session{c.law, mode, c.seed};
    out << "{\"steps\":[";
    for (unsigned n = 0; n < 24; ++n) {
        if (n)
            out << ',';
        const auto choice = session.choose_input();
        out << "{\"choice\":{\"input\":" << choice.input << ",\"active\":" << choice.active
            << ",\"worst\":";
        numbers(out, choice.worst);
        out << ",\"partition\":";
        numbers(out, choice.partition);
        out << "},\"measurement\":";
        check(session.begin_probe({0}, choice.input, true) == DiscoveryOutcome::Accepted,
              "training physical impulse refused");
        session.advance(2);
        measurement(out, session.apparatus().measurements().back());
        out << ",\"predictions\":[";
        if (n >= 2)
            for (unsigned i = 0; i < 24; ++i) {
                if (i)
                    out << ',';
                const auto prediction = session.predict(c.forecasts[i].chain, c.forecasts[i].input);
                if (prediction)
                    out << '[' << prediction->median << ',' << prediction->minimum << ','
                        << prediction->maximum << ',' << prediction->combinations << ']';
                else
                    out << "null";
            }
        out << "],\"state\":";
        state(out, session.agent());
        out << '}';
    }
    session.freeze();
    session.save(directory / (std::string(name) + "-learning.state"));
    out << "],\"learning_ledger\":";
    ledger(out, session.apparatus());
    auto evaluation = session.evaluation_copy();
    out << ",\"goals\":";
    goals(out, evaluation, c.goals);
    out << ",\"zero_goals\":";
    goals(out, evaluation, c.zeros);
    check(evaluation.agent().evidence() == session.agent().evidence(),
          "evaluation learned from outcomes");
    evaluation.save(directory / (std::string(name) + "-evaluation.state"));
    out << ",\"evaluation_ledger\":";
    ledger(out, evaluation.apparatus());
    out << ",\"final_state\":";
    state(out, evaluation.agent());
    PROCESS_MEMORY_COUNTERS memory{};
    memory.cb = sizeof(memory);
    check(GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)) != 0,
          "memory measurement unavailable");
    const auto elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started)
            .count();
    out << ",\"final_tick\":" << evaluation.tick() << ",\"wall_ms\":" << elapsed
        << ",\"cpu_ms\":" << cpu_ms() - cpu_start
        << ",\"peak_process_bytes\":" << memory.PeakWorkingSetSize << '}';
}
} // namespace
void run_hypothesis_case(const std::filesystem::path& input,
                         const std::filesystem::path& directory) {
    const auto c = read_case(input);
    std::ofstream out{directory / "raw.json", std::ios::binary};
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out << std::boolalpha;
    out << "{\"seed\":" << c.seed << ",\"law_table\":";
    numbers(out, c.law.table());
    out << ",\"variants\":{\"FA\":";
    variant(out, c, HypothesisMode::FlatActive, directory, "FA");
    out << ",\"SR\":";
    variant(out, c, HypothesisMode::SynthesisRandom, directory, "SR");
    out << ",\"SA\":";
    variant(out, c, HypothesisMode::SynthesisActive, directory, "SA");
    out << "}}\n";
}
} // namespace reality
