#include "research/discovery_protocol.hpp"
#include "research/discovery_random.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Psapi.h>
#include <Windows.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace reality {
namespace {
void check(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
std::uint64_t number(std::istream& in, std::uint64_t max) {
    std::string token;
    check(static_cast<bool>(in >> token), "missing case integer");
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    check(parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size() && value <= max,
          "invalid case integer");
    return value;
}
unsigned small(std::istream& in, unsigned max) { return static_cast<unsigned>(number(in, max)); }
double cpu_ms() {
    FILETIME created{}, exited{}, kernel{}, user{};
    check(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user) != 0,
          "CPU time measurement unavailable");
    const auto value = [](FILETIME time) {
        return (static_cast<std::uint64_t>(time.dwHighDateTime) << 32) | time.dwLowDateTime;
    };
    return static_cast<double>(value(kernel) + value(user)) / 10000.;
}
std::uint64_t peak_memory() {
    PROCESS_MEMORY_COUNTERS memory{};
    check(GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)) != 0,
          "process memory measurement unavailable");
    return memory.PeakWorkingSetSize;
}
DiscoverySession make_session(const DiscoveryCase& data) {
    return DiscoverySession{data.rules, make_discovery_scene(), data.seed};
}
struct Evaluation {
    DiscoverySession session;
    unsigned hits = 0;
};
Evaluation evaluate(const DiscoverySession& checkpoint, unsigned actor,
                    const std::vector<DiscoveryQuery>& queries, bool predictions,
                    std::optional<std::uint32_t> random_seed = {}) {
    auto s = checkpoint.evaluation_copy();
    s.freeze(actor);
    const auto start = s.tick();
    std::uint32_t counter = 0;
    unsigned hits = 0;
    for (const auto& query : queries) {
        auto chain = query.recipe;
        if (!predictions) {
            if (random_seed)
                chain = discovery_chains()[discovery_draw(*random_seed, 6, counter) % 39];
            else
                chain = s.agent(actor).choose_chain(query.input, query.target);
        }
        const auto m = s.probe(actor, chain, query.input, false);
        check(m.has_value(), "evaluation impulse refused");
        check(s.tick() - start <= 2000, "evaluation world-time budget exceeded");
        if (predictions)
            hits +=
                s.traces().back().prediction && s.traces().back().prediction->median == m->output;
        else
            hits += m->output == query.target;
    }
    s.validate();
    return {std::move(s), hits};
}
void write_chain(std::ostream& out, const DiscoveryChain& chain) {
    out << '[';
    bool first = true;
    for (const auto symbol : chain) {
        if (!first)
            out << ',';
        out << static_cast<unsigned>(symbol);
        first = false;
    }
    out << ']';
}
void write_trials(std::ostream& out, const DiscoverySession& s,
                  const std::vector<DiscoveryQuery>* queries = nullptr) {
    out << '[';
    std::size_t index = 0;
    for (const auto& m : s.apparatus().measurements()) {
        const auto trace = std::find_if(s.traces().begin(), s.traces().end(),
                                        [&](const auto& t) { return t.command == m.command; });
        check(trace != s.traces().end(), "measurement has no pre-intervention trace");
        if (index)
            out << ',';
        out << "{\"command\":" << m.command << ",\"source_actor\":" << m.source_actor.value
            << ",\"input\":" << m.input << ",\"output\":" << m.output
            << ",\"observed_tick\":" << m.observed_tick
            << ",\"started_tick\":" << trace->started_tick
            << ",\"learning\":" << (trace->learning ? "true" : "false") << ",\"chain\":";
        write_chain(out, m.chain);
        out << ",\"prediction\":";
        if (trace->prediction)
            out << trace->prediction->median << ",\"minimum\":" << trace->prediction->minimum
                << ",\"maximum\":" << trace->prediction->maximum;
        else
            out << "null";
        if (queries)
            out << ",\"target\":" << queries->at(index).target;
        out << '}';
        ++index;
    }
    if (queries)
        check(index == queries->size(), "incomplete trial evidence");
    out << ']';
}
void write_cost(std::ostream& out, const DiscoverySession& s, unsigned actor) {
    unsigned energy = 0;
    for (const auto& e : s.agent(actor).evidence())
        if (e.measurement.source_actor == s.agent(actor).actor())
            energy += e.measurement.input;
    out << "{\"probes\":" << s.training_attempts(actor) << ",\"spent_eu\":" << energy
        << ",\"work_units\":" << s.agent(actor).work_units() << ",\"tick\":" << s.tick() << '}';
}
void write_energy(std::ostream& out, const DiscoverySession& s) {
    out << '[';
    for (unsigned i = 0; i < 2; ++i) {
        const auto& d = s.apparatus().devices()[i];
        if (i)
            out << ',';
        out << "{\"source\":" << d.source << ",\"output\":" << d.output << ",\"loss\":" << d.loss
            << '}';
    }
    out << ']';
}
void finish_fixture(DiscoverySession& s) {
    s.advance(1);
    s.train(0);
    check(s.meet_and_share(true) == DiscoveryOutcome::Accepted, "fixture contact failed");
    s.train(1);
    auto evaluation = s.evaluation_copy();
    check(evaluation.probe(1, {2, 2}, 80, false).has_value(),
          "fixture post-teaching action failed");
    s = std::move(evaluation);
}
} // namespace
DiscoveryCase read_discovery_case(const std::filesystem::path& path) {
    check(std::filesystem::file_size(path) <= 1024 * 1024, "case file too large");
    std::ifstream in(path);
    check(static_cast<bool>(in), "cannot open case file");
    std::string magic;
    check(static_cast<bool>(in >> magic) && magic == "DISCOVERY_CASE" && small(in, 1) == 1,
          "invalid case header");
    DiscoveryCase result;
    result.seed = static_cast<std::uint32_t>(number(in, UINT32_MAX));
    for (auto& rule : result.rules) {
        rule.family = static_cast<DiscoveryFamily>(small(in, 2));
        rule.parameter = small(in, 48);
        static_cast<void>(discovery_apply(rule, 16));
    }
    for (unsigned section = 0; section < 3; ++section) {
        const std::array names{"predictions", "tasks_a", "tasks_b"};
        check(static_cast<bool>(in >> magic) && magic == names[section], "wrong case section");
        const auto count = small(in, 24);
        check(count == (section == 0 ? 24U : 20U), "wrong query count");
        auto& queries =
            section == 0 ? result.predictions : (section == 1 ? result.tasks_a : result.tasks_b);
        for (unsigned i = 0; i < count; ++i) {
            DiscoveryQuery query;
            query.input = small(in, 112);
            query.target = small(in, query.input);
            const auto length = small(in, 3);
            check((query.input == 48 || query.input == 80 || query.input == 112) && length >= 2,
                  "query is not held out");
            for (unsigned j = 0; j < length; ++j)
                query.recipe.push_back(static_cast<std::uint8_t>(small(in, 2)));
            auto output = query.input;
            for (const auto symbol : query.recipe)
                output = discovery_apply(result.rules[symbol], output);
            check(output == query.target, "manifest target disagrees with physical rules");
            queries.push_back(std::move(query));
        }
    }
    in >> std::ws;
    check(in.eof(), "trailing case data");
    return result;
}
void run_discovery_case(const DiscoveryCase& data, const std::filesystem::path& directory) {
    const auto wall_start = std::chrono::steady_clock::now();
    const auto cpu_start = cpu_ms();
    auto teacher = make_session(data);
    auto prior = teacher.agent(0);
    std::vector<DiscoveryEnergy> preexperience;
    for (const auto& query : data.predictions)
        preexperience.push_back(prior.predict(query.recipe, query.input).median);
    teacher.train(0);
    teacher.save(directory / "teacher.save");
    auto d = evaluate(teacher, 0, data.predictions, true);
    unsigned before_hits = 0;
    for (std::size_t i = 0; i < data.predictions.size(); ++i)
        before_hits += preexperience[i] == d.session.apparatus().measurements()[i].output;
    auto g = evaluate(teacher, 0, data.tasks_a, false);
    auto naive = make_session(data);
    auto random_g = evaluate(naive, 0, data.tasks_a, false, data.seed);
    auto random_teacher = make_session(data);
    random_teacher.train(0, true);
    auto random_d = evaluate(random_teacher, 0, data.predictions, true);
    auto random_trained_g = evaluate(random_teacher, 0, data.tasks_a, false);
    auto with = teacher;
    auto without = teacher;
    const auto contact_start = teacher.tick();
    check(with.meet_and_share(true) == DiscoveryOutcome::Accepted &&
              without.meet_and_share(false) == DiscoveryOutcome::Accepted,
          "paired contact failed");
    const auto contact_ticks = with.tick() - contact_start;
    const auto control_ticks = without.tick() - contact_start;
    check(contact_ticks == control_ticks && without.agent(1).evidence().empty(),
          "contact control leaked data or changed schedule");
    const auto packet_size = with.agent(1).evidence().size();
    const auto packet = with.agent(1).evidence();
    with.train(1);
    without.train(1);
    with.save(directory / "student-with.save");
    without.save(directory / "student-without.save");
    auto s_with = evaluate(with, 1, data.tasks_b, false);
    auto s_without = evaluate(without, 1, data.tasks_b, false);
    d.session.save(directory / "predictions.save");
    g.session.save(directory / "generalization.save");
    s_with.session.save(directory / "student-with-final.save");
    s_without.session.save(directory / "student-without-final.save");
    std::ofstream out(directory / "result.json", std::ios::binary);
    check(static_cast<bool>(out), "cannot write result");
    out << std::setprecision(17) << "{\"seed\":" << data.seed
        << ",\"integrity\":true,\"d_before\":" << before_hits / 24.
        << ",\"d_after\":" << d.hits / 24. << ",\"g\":" << g.hits / 20.
        << ",\"g_random\":" << random_g.hits / 20. << ",\"s_with\":" << s_with.hits / 20.
        << ",\"s_without\":" << s_without.hits / 20. << ",\"random_d\":" << random_d.hits / 24.
        << ",\"random_g\":" << random_trained_g.hits / 20. << ",\"teacher\":";
    write_cost(out, teacher, 0);
    out << ",\"student_with\":";
    write_cost(out, with, 1);
    out << ",\"student_without\":";
    write_cost(out, without, 1);
    out << ",\"random_teacher\":";
    write_cost(out, random_teacher, 0);
    out << ",\"pair_work_with\":" << with.agent(0).work_units() + with.agent(1).work_units()
        << ",\"pair_work_without\":"
        << without.agent(0).work_units() + without.agent(1).work_units()
        << ",\"contact\":{\"with_ticks\":" << contact_ticks
        << ",\"without_ticks\":" << control_ticks << ",\"packet_records\":" << packet_size
        << ",\"path\":[";
    bool first_move = true;
    for (const auto& move : with.movements()) {
        if (!first_move)
            out << ',';
        first_move = false;
        out << "{\"tick\":" << move.tick << ",\"actor\":" << move.actor
            << ",\"surface\":" << move.surface << ",\"x\":" << move.feet.x
            << ",\"y\":" << move.feet.y << ",\"z\":" << move.feet.z << '}';
    }
    std::ostringstream packet_json;
    packet_json << '[';
    for (std::size_t i = 0; i < packet.size(); ++i) {
        if (i)
            packet_json << ',';
        const auto& e = packet[i];
        const auto& m = e.measurement;
        packet_json << "{\"id\":" << m.id << ",\"source_actor\":" << m.source_actor.value
                    << ",\"via_actor\":" << e.via_actor.value
                    << ",\"observed_tick\":" << m.observed_tick
                    << ",\"learned_tick\":" << e.learned_tick << ",\"input\":" << m.input
                    << ",\"output\":" << m.output << ",\"chain\":";
        write_chain(packet_json, m.chain);
        packet_json << '}';
    }
    packet_json << ']';
    out << "],\"packet\":" << packet_json.str()
        << ",\"packet_json_bytes\":" << packet_json.str().size()
        << "},\"preexperience_predictions\":[";
    for (std::size_t i = 0; i < preexperience.size(); ++i) {
        if (i)
            out << ',';
        out << preexperience[i];
    }
    const auto overlap =
        std::count_if(data.tasks_b.begin(), data.tasks_b.end(), [&](const auto& b) {
            return std::any_of(data.tasks_a.begin(), data.tasks_a.end(), [&](const auto& a) {
                return a.input == b.input && a.target == b.target;
            });
        });
    out << "],\"overlapping_tasks_b\":" << overlap << ",\"trials\":{\"Training_A\":";
    write_trials(out, teacher);
    const std::array<const char*, 7> names{"D",        "G",      "G_random", "random_D",
                                           "random_G", "S_with", "S_without"};
    const std::array<const DiscoverySession*, 7> sessions{&d.session,
                                                          &g.session,
                                                          &random_g.session,
                                                          &random_d.session,
                                                          &random_trained_g.session,
                                                          &s_with.session,
                                                          &s_without.session};
    const std::array<const std::vector<DiscoveryQuery>*, 7> queries{
        &data.predictions, &data.tasks_a, &data.tasks_a, &data.predictions,
        &data.tasks_a,     &data.tasks_b, &data.tasks_b};
    for (std::size_t i = 0; i < names.size(); ++i) {
        out << ",\"" << names[i] << "\":";
        write_trials(out, *sessions[i], queries[i]);
    }
    out << "},\"energy\":{";
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i)
            out << ',';
        out << '"' << names[i] << "\":";
        write_energy(out, *sessions[i]);
    }
    out << "},\"cpu_ms\":" << cpu_ms() - cpu_start
        << ",\"peak_working_set_bytes\":" << peak_memory() << ",\"wall_ms\":"
        << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - wall_start)
               .count()
        << "}\n";
    out.close();
    check(static_cast<bool>(out), "result write failed");
}
void discovery_fixture(const std::filesystem::path& directory) {
    DiscoverySession s{{DiscoveryRule{DiscoveryFamily::Loss, 4},
                        DiscoveryRule{DiscoveryFamily::Cap, 20},
                        DiscoveryRule{DiscoveryFamily::Fraction, 8}}};
    check(s.begin_probe(0, {0}, 64) == DiscoveryOutcome::Accepted, "fixture probe refused");
    s.advance(1);
    s.save(directory / "checkpoint.save");
    finish_fixture(s);
    s.save(directory / "expected.save");
}
void continue_discovery_fixture(const std::filesystem::path& input,
                                const std::filesystem::path& directory) {
    DiscoverySession s{{DiscoveryRule{DiscoveryFamily::Loss, 4},
                        DiscoveryRule{DiscoveryFamily::Cap, 20},
                        DiscoveryRule{DiscoveryFamily::Fraction, 8}}};
    s.load(input);
    finish_fixture(s);
    s.save(directory / "final.save");
}
} // namespace reality
