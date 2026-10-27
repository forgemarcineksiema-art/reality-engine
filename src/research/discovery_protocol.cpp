#include "research/discovery_protocol.hpp"
#include "research/discovery_random.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <Psapi.h>
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
struct PhaseCost {
    std::uint64_t work = 0;
    double cpu = 0;
    std::uint64_t ticks = 0, energy = 0;
};
std::uint64_t total_work(const DiscoverySession& s) {
    return s.agent(0).work_units() + s.agent(1).work_units();
}
void write_phase_cost(std::ostream& out, const PhaseCost& cost) {
    out << "{\"work_units\":" << cost.work << ",\"cpu_ms\":" << cost.cpu
        << ",\"ticks\":" << cost.ticks << ",\"spent_eu\":" << cost.energy << '}';
}
struct Evaluation {
    DiscoverySession session;
    unsigned hits = 0;
    PhaseCost cost;
};
Evaluation evaluate(const DiscoverySession& checkpoint, unsigned actor,
                    const std::vector<DiscoveryQuery>& queries, bool predictions,
                    bool random_control = false) {
    auto s = checkpoint.evaluation_copy();
    s.freeze(actor);
    const auto start = s.tick();
    const auto cpu_start = cpu_ms();
    const auto work_start = s.agent(actor).work_units();
    unsigned hits = 0;
    for (const auto& query : queries) {
        auto chain = query.recipe;
        if (!predictions) {
            if (random_control)
                chain = s.random_chain(actor);
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
    const PhaseCost cost{s.agent(actor).work_units() - work_start, cpu_ms() - cpu_start,
                         s.tick() - start, 8192U - s.apparatus().devices()[actor].source};
    return {std::move(s), hits, cost};
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
    const auto prior_cpu_start = cpu_ms();
    for (const auto& query : data.predictions)
        preexperience.push_back(prior.predict(query.recipe, query.input).median);
    const PhaseCost prior_cost{prior.work_units(), cpu_ms() - prior_cpu_start, 0, 0};
    const auto teacher_cpu_start = cpu_ms();
    teacher.train(0);
    const PhaseCost teacher_cost{total_work(teacher), cpu_ms() - teacher_cpu_start, teacher.tick(),
                                 8192U - teacher.apparatus().devices()[0].source};
    teacher.save(directory / "teacher.save");
    auto d = evaluate(teacher, 0, data.predictions, true);
    unsigned before_hits = 0;
    for (std::size_t i = 0; i < data.predictions.size(); ++i)
        before_hits += preexperience[i] == d.session.apparatus().measurements()[i].output;
    auto g = evaluate(teacher, 0, data.tasks_a, false);
    auto naive = make_session(data);
    auto random_g = evaluate(naive, 0, data.tasks_a, false, true);
    auto random_teacher = make_session(data);
    const auto random_teacher_cpu_start = cpu_ms();
    random_teacher.train(0, true);
    const PhaseCost random_teacher_cost{total_work(random_teacher),
                                        cpu_ms() - random_teacher_cpu_start, random_teacher.tick(),
                                        8192U - random_teacher.apparatus().devices()[0].source};
    auto random_d = evaluate(random_teacher, 0, data.predictions, true);
    auto random_trained_g = evaluate(random_teacher, 0, data.tasks_a, false);
    auto with = teacher;
    auto without = teacher;
    const auto contact_start = teacher.tick();
    const auto with_contact_cpu_start = cpu_ms();
    check(with.meet_and_share(true) == DiscoveryOutcome::Accepted, "contact failed");
    const PhaseCost with_contact_cost{total_work(with) - total_work(teacher),
                                      cpu_ms() - with_contact_cpu_start,
                                      with.tick() - contact_start, 0};
    const auto without_contact_cpu_start = cpu_ms();
    check(without.meet_and_share(false) == DiscoveryOutcome::Accepted, "control contact failed");
    const PhaseCost without_contact_cost{total_work(without) - total_work(teacher),
                                         cpu_ms() - without_contact_cpu_start,
                                         without.tick() - contact_start, 0};
    const auto contact_ticks = with.tick() - contact_start;
    const auto control_ticks = without.tick() - contact_start;
    check(contact_ticks == control_ticks && without.agent(1).evidence().empty(),
          "contact control leaked data or changed schedule");
    const auto packet_size = with.agent(1).evidence().size();
    const auto packet = with.agent(1).evidence();
    const auto b_with_start = cpu_ms();
    const auto b_with_work = total_work(with);
    const auto b_with_tick = with.tick();
    with.train(1);
    const PhaseCost b_with_cost{total_work(with) - b_with_work, cpu_ms() - b_with_start,
                                with.tick() - b_with_tick,
                                8192U - with.apparatus().devices()[1].source};
    const auto b_without_start = cpu_ms();
    const auto b_without_work = total_work(without);
    const auto b_without_tick = without.tick();
    without.train(1);
    const PhaseCost b_without_cost{total_work(without) - b_without_work, cpu_ms() - b_without_start,
                                   without.tick() - b_without_tick,
                                   8192U - without.apparatus().devices()[1].source};
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
    out << ",\"phase_costs\":{";
    const std::array<const char*, 14> phase_names{"D_prior",
                                                  "train_A",
                                                  "D",
                                                  "G",
                                                  "G_random",
                                                  "train_A_random",
                                                  "random_D",
                                                  "random_G",
                                                  "contact_with",
                                                  "contact_without",
                                                  "train_B_with",
                                                  "train_B_without",
                                                  "S_with",
                                                  "S_without"};
    const std::array<PhaseCost, 14> phase_costs{
        prior_cost,        teacher_cost,         d.cost,        g.cost,
        random_g.cost,     random_teacher_cost,  random_d.cost, random_trained_g.cost,
        with_contact_cost, without_contact_cost, b_with_cost,   b_without_cost,
        s_with.cost,       s_without.cost};
    for (std::size_t i = 0; i < phase_names.size(); ++i) {
        if (i)
            out << ',';
        out << '"' << phase_names[i] << "\":";
        write_phase_cost(out, phase_costs[i]);
    }
    out << "},\"pair_work_with\":" << total_work(s_with.session)
        << ",\"pair_work_without\":" << total_work(s_without.session) << ",\"pair_cpu_with_ms\":"
        << teacher_cost.cpu + with_contact_cost.cpu + b_with_cost.cpu + s_with.cost.cpu
        << ",\"pair_cpu_without_ms\":"
        << teacher_cost.cpu + without_contact_cost.cpu + b_without_cost.cpu + s_without.cost.cpu
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
    DiscoverySession negative{{DiscoveryRule{DiscoveryFamily::Outside, 0},
                               DiscoveryRule{DiscoveryFamily::Loss, 4},
                               DiscoveryRule{DiscoveryFamily::Cap, 20}}};
    std::vector<DiscoveryEnergy> outputs;
    for (const auto input : {16U, 32U, 64U}) {
        const auto m = negative.probe(0, {0}, input);
        check(m.has_value(), "negative physical control refused");
        outputs.push_back(m->output);
    }
    check(outputs == std::vector<DiscoveryEnergy>{0, 32, 64} && negative.agent(0).inadequate(),
          "negative physical control did not reject model");
    negative.save(directory / "negative-control.save");
    const auto& ledger = negative.apparatus().devices()[0];
    std::ofstream negative_out(directory / "negative-control.json", std::ios::binary);
    negative_out << "{\"outputs\":[0,32,64],\"model_inadequate\":true,\"source\":" << ledger.source
                 << ",\"output\":" << ledger.output << ",\"loss\":" << ledger.loss
                 << ",\"trials\":";
    write_trials(negative_out, negative);
    negative_out << "}\n";
    negative_out.close();
    check(static_cast<bool>(negative_out), "negative control write failed");
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
