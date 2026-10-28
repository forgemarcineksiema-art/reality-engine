#include "research/contrast_session.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace reality;
namespace {
template <class T> void array(std::ostream& o, const T& a) {
    o << '[';
    bool comma = false;
    for (auto x : a) {
        if (comma)
            o << ',';
        comma = true;
        o << static_cast<std::uint64_t>(x);
    }
    o << ']';
}
void measurement(std::ostream& o, const DiscoveryMeasurement& m) {
    o << "{\"id\":" << m.id << ",\"source\":" << m.source_actor.value
      << ",\"tick\":" << m.observed_tick << ",\"input\":" << m.input << ",\"output\":" << m.output
      << '}';
}
void trial(ContrastSession& s, std::ostream& o, unsigned actor) {
    auto c = s.choose(actor);
    o << "{\"input\":" << c.input << ",\"active\":" << c.active << ",\"before\":" << c.before
      << ",\"worst\":" << c.worst << ",\"partition\":";
    array(o, c.partition);
    if (s.begin_probe(actor, {0}, c.input, true) != DiscoveryOutcome::Accepted)
        throw std::runtime_error("probe refused");
    s.advance(2);
    o << ",\"measurement\":";
    measurement(o, s.apparatus().measurements().back());
    o << ",\"models\":" << s.agent(actor).models().size() << '}';
}
bool goal(ContrastSession& s, std::ostream& o, unsigned actor, unsigned x, unsigned y, bool work) {
    auto c = s.choose_chain(actor, x, y);
    o << "{\"chain\":";
    bool success = false;
    if (c) {
        array(o, *c);
        auto status = work ? s.begin_work(actor, *c, x, y) : s.begin_probe(actor, *c, x, false);
        if (status != DiscoveryOutcome::Accepted)
            throw std::runtime_error("goal refused");
        s.advance(2);
        o << ",\"measurement\":";
        measurement(o, s.apparatus().measurements().back());
        success = s.apparatus().measurements().back().output == y;
        if (work) {
            s.advance(80);
            success = s.completed(actor) == 1;
        }
    } else
        o << "null,\"measurement\":null";
    o << ",\"success\":" << success << ",\"parts\":" << s.parts(actor)
      << ",\"completed\":" << s.completed(actor) << '}';
    return success;
}
void variant(std::ostream& o, HypothesisLaw law, bool active, unsigned seed,
             const std::array<std::pair<unsigned, unsigned>, 20>& goals) {
    auto start = std::chrono::steady_clock::now();
    ContrastSession s{law, active, seed};
    o << "{\"steps\":[";
    for (unsigned n = 0; n < 8; ++n) {
        if (n)
            o << ',';
        o << "{\"trial\":";
        trial(s, o, 0);
        o << ",\"predictions\":[";
        for (unsigned x = 1; x <= 128; ++x) {
            if (x > 1)
                o << ',';
            auto p = s.predict(0, {0}, x);
            if (p)
                o << '[' << p->median << ',' << p->minimum << ',' << p->maximum << ','
                  << p->combinations << ']';
            else
                o << "null";
        }
        o << "]}";
    }
    s.freeze(0);
    o << "],\"goals\":[";
    for (unsigned n = 0; n < 20; ++n) {
        if (n)
            o << ',';
        (void)goal(s, o, 0, goals[n].first, goals[n].second, false);
    }
    o << "],\"teacher\":";
    (void)goal(s, o, 0, goals[0].first, goals[0].second, true);
    o << ",\"social\":[";
    for (unsigned message = 0; message < 2; ++message) {
        if (message)
            o << ',';
        auto b = s;
        const auto start_tick = b.tick();
        if (b.meet_and_share(message != 0) != DiscoveryOutcome::Accepted)
            throw std::runtime_error("contact refused");
        o << "{\"contact_ticks\":" << b.tick() - start_tick
          << ",\"contact_tick\":" << b.contact_tick() << ",\"received\":[";
        bool comma = false;
        for (const auto& e : b.agent(1).evidence()) {
            if (comma)
                o << ',';
            comma = true;
            o << "{\"measurement\":";
            measurement(o, e.measurement);
            o << ",\"via\":" << e.via_actor.value << ",\"learned\":" << e.learned_tick << '}';
        }
        o << "],\"trials\":[";
        for (unsigned n = 0; n < 2; ++n) {
            if (n)
                o << ',';
            trial(b, o, 1);
        }
        b.freeze(1);
        o << "],\"work\":";
        (void)goal(b, o, 1, goals[0].first, goals[0].second, true);
        o << ",\"reads\":" << b.agent(1).cost().reads
          << ",\"primitives\":" << b.agent(1).cost().primitives
          << ",\"energy\":" << 8192 - b.apparatus().devices()[1].source << '}';
    }
    const auto& cost = s.agent(0).cost();
    o << "],\"reads\":" << cost.reads << ",\"primitives\":" << cost.primitives
      << ",\"exhausted\":" << s.agent(0).exhausted()
      << ",\"energy\":" << 8192 - s.apparatus().devices()[0].source << ",\"wall_ms\":"
      << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
      << '}';
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--fixture") {
            const std::filesystem::path directory{argv[2]};
            if (!std::filesystem::create_directory(directory))
                throw std::runtime_error("fixture exists");
            HypothesisLaw law{{{HypothesisKind::Loss, 48}, {HypothesisKind::Fraction, 12}}};
            ContrastSession s{law, true, 1};
            if (s.begin_probe(0, {0}, 96, true) != DiscoveryOutcome::Accepted)
                throw std::runtime_error("fixture begin");
            write_snapshot_atomic(s.serialize(), directory / "configure.save");
            auto a = s;
            a.advance(2);
            write_snapshot_atomic(a.serialize(), directory / "configure.expected");
            s.advance(1);
            write_snapshot_atomic(s.serialize(), directory / "pulse.save");
            s.advance(1);
            write_snapshot_atomic(s.serialize(), directory / "pulse.expected");
            s.freeze(0);
            if (s.begin_work(0, {0}, 96, 36) != DiscoveryOutcome::Accepted)
                throw std::runtime_error("fixture work");
            s.advance(2);
            write_snapshot_atomic(s.serialize(), directory / "work.save");
            s.advance(80);
            write_snapshot_atomic(s.serialize(), directory / "work.expected");
            if (s.meet_and_share(true) != DiscoveryOutcome::Accepted)
                throw std::runtime_error("fixture contact");
            write_snapshot_atomic(s.serialize(), directory / "contact.save");
            s.advance(1);
            write_snapshot_atomic(s.serialize(), directory / "contact.expected");
            return 0;
        }
        if (argc == 5 && std::string(argv[1]) == "--resume") {
            ContrastSession s{{}, true, 0};
            s.restore(read_snapshot_bytes(argv[2]));
            const std::string ticks = argv[3];
            if (ticks != "1" && ticks != "2" && ticks != "80")
                throw std::runtime_error("fixture ticks");
            s.advance(static_cast<unsigned>(std::stoul(ticks)));
            if (std::filesystem::exists(argv[4]))
                throw std::runtime_error("resume output exists");
            write_snapshot_atomic(s.serialize(), argv[4]);
            return 0;
        }
        if (argc != 3)
            throw std::runtime_error("usage: reality_contrast CASE OUTPUT");
        std::ifstream in(argv[1]);
        unsigned seed = 0, d = 0, n = 0;
        in >> seed >> d >> n;
        if (!in || n > 3)
            throw std::runtime_error("case header");
        HypothesisLaw law;
        law.square_divisor = d;
        for (unsigned i = 0; i < n; ++i) {
            unsigned k = 0, p = 0;
            in >> k >> p;
            law.program.push_back({static_cast<HypothesisKind>(k), p});
        }
        std::array<std::pair<unsigned, unsigned>, 20> goals{};
        for (auto& g : goals) {
            in >> g.first >> g.second;
            if (!in || !g.first || g.first > 128 || !g.second || g.second > g.first)
                throw std::runtime_error("case goal");
        }
        in >> std::ws;
        if (!in.eof())
            throw std::runtime_error("case trailing data");
        if (std::filesystem::exists(argv[2]))
            throw std::runtime_error("output exists");
        std::ofstream o(argv[2]);
        o << "{\"R\":";
        variant(o, law, false, seed, goals);
        o << ",\"A\":";
        variant(o, law, true, seed, goals);
        o << "}\n";
        if (!o)
            throw std::runtime_error("output failed");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
