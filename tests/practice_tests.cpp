#include "lab/traversal_session.hpp"
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#if __has_include("lab/traversal_practice.hpp")
#include "lab/traversal_practice.hpp"
using namespace reality;
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
TraversalScene fixture(unsigned stock = 4, bool remote = false) {
    TraversalScene s;
    s.geometry.bodies = {{1, {}, 0, 10, 6, .3, 0, 0, true}};
    s.stations = {{{1, {0, 0}}, TraversalTask::Maintain, stock, {}, 500, 0, {2}}};
    s.spawns = {{{1, {remote ? -5. : -.8, 0}}, {}, false}, {{1, {.8, 0}}, {}, false}};
    s.practice = PracticeSeed{};
    s.practice->station = 0;
    s.practice->researchers = {0, 1};
    return s;
}
void calibration() {
    TraversalSession gated{fixture()};
    require(!gated.begin_maintenance(0, 0), "manual maintenance bypassed calibration");
    require(gated.stations()[0].completed == 0 && !gated.actors()[0].action.value,
            "fictional work");
    TraversalSession s{fixture()};
    const auto lot = s.stations()[0].lot;
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "probe refused");
    s.advance(2);
    require(s.world().resource(lot)->record.quantity.value == 4,
            "failed calibration consumed a part");
    require(s.practice_probe(0, {0, 0}, 16) == DiscoveryOutcome::Accepted,
            "composed probe refused");
    s.advance(2);
    require(s.begin_maintenance(0, 0), "physical calibration did not authorize work");
    require(s.world().resource(lot)->record.quantity.value == 3, "material not consumed by work");
    s.advance(80);
    require(s.stations()[0].completed == 1, "maintenance not completed");
    require(!s.begin_maintenance(0, 0), "calibration reused for a second work");
}
void energy() {
    TraversalSession s{fixture()};
    auto before = s.practice_view(0).energy;
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "energy probe");
    s.advance(1);
    require(s.practice_view(0).energy == before, "energy spent before completed pulse");
    s.advance(1);
    require(s.practice_view(0).energy == before - 16, "lost physical energy");
    require(s.practice_probe(0, {0}, 129) == DiscoveryOutcome::Invalid, "range accepted");
}
void concurrent() {
    TraversalSession s{fixture()};
    require(s.can_interact_at(1, s.stations()[0].location),
            "second fixture operator is out of reach");
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "first operator");
    require(s.practice_probe(1, {0}, 32) == DiscoveryOutcome::Busy, "two simultaneous operators");
    s.advance(2);
    require(s.practice_probe(1, {0}, 32) == DiscoveryOutcome::Accepted, "device never released");
}
void protected_probe() {
    TraversalSession s{fixture()};
    const auto energy = s.practice_view(0).energy;
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "protected begin");
    s.set_goal(0, {1, {-4, 0}});
    s.advance(2);
    require(s.practice_view(0).energy == energy - 16, "deferred command lost the pulse");
    s.advance(40);
    require(s.actors()[0].feet.x < -2, "deferred goal was lost");
}
void queued_input() {
    TraversalSession s{fixture()};
    s.queue_input(0, {1, 0});
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Busy,
            "apparatus accepted an operator with queued movement");
    s.advance(2);
    require(s.practice_view(0).probes == 0, "refused moving operator acquired a measurement");
}
void persistence() {
    for (unsigned stage = 0; stage < 2; ++stage) {
        TraversalSession s{fixture()};
        require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted,
                "pending snapshot begin");
        if (stage)
            s.advance(1);
        const auto saved = s.serialize();
        require(saved.starts_with("GENESIS_TRAVERSAL 8\n"), "new world lacks version8");
        s.advance(2 - stage);
        const auto expected = s.serialize();
        TraversalSession loaded;
        loaded.restore(saved);
        require(loaded.serialize() == saved, "pending restore differs before advance");
        loaded.advance(2 - stage);
        require(loaded.serialize() == expected, "pending snapshot continuation differs");
    }
    TraversalSession s{fixture()};
    require(s.practice_probe(0, {0, 0}, 16) == DiscoveryOutcome::Accepted,
            "work snapshot calibration");
    s.advance(2);
    require(s.begin_maintenance(0, 0), "work snapshot begin");
    s.advance(17);
    const auto saved = s.serialize();
    s.advance(63);
    const auto expected = s.serialize();
    TraversalSession loaded;
    loaded.restore(saved);
    loaded.advance(63);
    require(loaded.serialize() == expected && loaded.stations()[0].completed == 1,
            "work snapshot consumed twice");
}
std::string checksum(std::string text) {
    const auto marker = text.rfind("CHECKSUM ");
    require(marker != std::string::npos, "snapshot checksum marker");
    text.resize(marker);
    return text + "CHECKSUM " + std::to_string(snapshot_checksum(text)) + "\n";
}
std::string alter_practice(const std::string& before,
                           const std::function<void(std::string&)>& change) {
    const auto at = before.find("PRACTICE ");
    require(at != std::string::npos, "practice section absent");
    std::istringstream in{before.substr(at + 9)};
    std::string payload;
    in >> std::quoted(payload);
    change(payload);
    std::ostringstream out;
    out << before.substr(0, at) << "PRACTICE " << std::quoted(payload) << "\nCHECKSUM 0\n";
    return checksum(out.str());
}
void forged_work() {
    TraversalSession s{fixture()};
    require(s.practice_probe(0, {0, 0}, 16) == DiscoveryOutcome::Accepted, "proof fixture");
    s.advance(2);
    require(s.begin_maintenance(0, 0), "proof work");
    s.advance(17);
    const auto before = s.serialize();
    auto bad = alter_practice(before, [](auto& p) {
        const auto at = p.find("WORKING 1 ");
        require(at != std::string::npos, "running owner absent");
        p.replace(at, p.find('\n', at) - at, "WORKING 0");
    });
    bool rejected = false;
    try {
        s.restore(bad);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before, "running calibrated work lost its required owner");
}
void missing_permit() {
    TraversalSession s{fixture()};
    require(s.practice_probe(0, {0, 0}, 16) == DiscoveryOutcome::Accepted, "permit fixture");
    s.advance(2);
    const auto before = s.serialize();
    const auto bad = alter_practice(before, [](auto& p) {
        auto at = p.find("PERMIT 1 ");
        require(at != std::string::npos, "proof absent");
        p.replace(at, p.find('\n', at) - at, "PERMIT 0");
    });
    bool rejected = false;
    try {
        s.restore(bad);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before,
            "unused successful calibration disappeared on restore");
}
void evidence_capacity() {
    auto scene = fixture();
    scene.spawns = {{{1, {-.8, 0}}, {}, false},
                    {{1, {0, -.8}}, {}, false},
                    {{1, {.8, 0}}, {}, false},
                    {{1, {0, .8}}, {}, false}};
    TraversalSession s{scene};
    for (std::size_t from = 0; from < 3; ++from) {
        for (unsigned input = 1; input <= 8; ++input) {
            require(s.practice_probe(from, {0}, input) == DiscoveryOutcome::Accepted,
                    "donor probe");
            s.advance(2);
        }
        require(s.share_practice(from, 3) == DiscoveryOutcome::Accepted, "capacity contact");
        s.advance(1);
    }
    require(s.practice_view(3).records == 24 && s.practice_view(3).probes == 0,
            "capacity precondition");
    const auto before = s.serialize();
    require(s.practice_probe(3, {0}, 16) == DiscoveryOutcome::Frozen,
            "accepted an observation with no storage");
    require(s.serialize() == before, "refusal configured or spent energy");
}
void frozen_pending() {
    auto scene = fixture();
    scene.practice->law = {{HypothesisKind::Fraction, 8}};
    scene.spawns[0].autonomous = true;
    scene.spawns[0].known_stations = 1;
    TraversalSession s{scene};
    for (unsigned i = 0; i < 180 && !(s.practice_view(0).method_known &&
                                      s.practice_view(0).stage == PracticeStage::Pulsing);
         ++i)
        s.advance(1);
    require(s.practice_view(0).method_known && s.practice_view(0).stage == PracticeStage::Pulsing,
            "frozen pending fixture");
    const auto before = s.serialize();
    const auto bad = alter_practice(before, [](auto& p) {
        auto at = p.rfind("PULSE ");
        auto end = p.find('\n', at);
        auto line = p.substr(at, end - at);
        std::istringstream in{line};
        std::string tag;
        std::uint64_t actor, tick;
        unsigned input, learning, n, module;
        in >> tag >> actor >> input >> tick >> learning >> n >> module;
        require(learning == 0 && n == 1 && module == 0, "pending classification fixture");
        std::ostringstream out;
        out << "PULSE " << actor << ' ' << input << ' ' << tick << " 1 1 0";
        p.replace(at, end - at, out.str());
        at = p.find("PENDING 1 ");
        end = p.find('\n', at);
        require(p[end - 1] == '0', "pending purpose fixture");
        p[end - 1] = '1';
        at = p.find("MIND ");
        std::istringstream mind{p.substr(at)};
        unsigned probes, spent;
        mind >> tag >> actor >> probes >> spent;
        std::ostringstream old, new_value;
        old << "MIND " << actor << ' ' << probes << ' ' << spent;
        new_value << "MIND " << actor << ' ' << probes + 1 << ' ' << spent + input;
        p.replace(at, old.str().size(), new_value.str());
    });
    bool rejected = false;
    try {
        s.restore(bad);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before, "accepted learning pulse after mind freeze");
}
void contact_support() {
    auto scene = fixture();
    scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {2, 0, 0}, 0, 2, 2, .3, 0, 0, true}};
    scene.stations[0].location = {1, {1, 0}};
    scene.spawns = {{{1, {1, -.5}}, {}, false}, {{1, {1, .5}}, {}, false}};
    scene.connections = {{{1, {1.7, -.5}}, {2, {-1.7, -.5}}}, {{1, {1.7, .5}}, {2, {-1.7, .5}}}};
    TraversalSession s{scene};
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "support donor probe");
    s.advance(2);
    s.set_goal(0, {2, {1, -.5}});
    s.set_goal(1, {2, {1, .5}});
    for (unsigned i = 0; i < 100; ++i) {
        if (s.actors()[0].support.surface == 1 && s.actors()[1].support.surface == 1 &&
            s.can_exchange_knowledge(0, 1)) {
            auto crossing = s;
            require(crossing.share_practice(0, 1) == DiscoveryOutcome::Accepted,
                    "boundary contact refused before move");
            crossing.advance(1);
            if (crossing.actors()[0].support.surface == 2 &&
                crossing.actors()[1].support.surface == 2) {
                require(crossing.practice_view(1).records == 0,
                        "contact completed on a different support");
                const auto saved = crossing.serialize();
                crossing.restore(saved);
                require(crossing.serialize() == saved,
                        "boundary contact produced unloadable state");
                return;
            }
        }
        s.advance(1);
    }
    throw std::runtime_error("fixture never crossed both supports in one step");
}
void corruption() {
    for (bool consumed : {false, true}) {
        TraversalSession s{fixture()};
        require(s.practice_probe(0, consumed ? DiscoveryChain{0, 0} : DiscoveryChain{0}, 16) ==
                    DiscoveryOutcome::Accepted,
                "corrupt fixture");
        s.advance(2);
        if (consumed) {
            require(s.begin_maintenance(0, 0), "consumed fixture");
            s.advance(80);
        }
        const auto before = s.serialize();
        auto bad = before;
        auto where = bad.find("PERMIT 0");
        require(where != std::string::npos, "permit record missing");
        bad.replace(where, 8, "PERMIT 1 1 2 2");
        bool rejected = false;
        try {
            s.restore(checksum(bad));
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected && s.serialize() == before,
                "forged or reused calibration accepted/nontransactional");
        const auto section = before.find("PRACTICE ");
        require(section != std::string::npos, "missing practice section");
        std::istringstream input{before.substr(section + 9)};
        std::string payload;
        input >> std::quoted(payload);
        const auto v = s.practice_view(0);
        const std::vector<std::pair<std::string, std::string>> changes = {
            {"PULSE 1 16 0 ", "PULSE 2 16 0 "},
            {"PULSE 1 16 0 ", "PULSE 1 32 0 "},
            {"MIND 1 ", "MIND 2 "},
            {"STATE \"1 1 ", "STATE \"1 0 "},
            {std::to_string(v.primitives) + " " + std::to_string(v.reads) + " ",
             std::to_string(v.primitives + 1) + " " + std::to_string(v.reads) + " "}};
        for (const auto& [from, to] : changes) {
            auto changed = payload;
            auto at = changed.find(from);
            require(at != std::string::npos, "corruption target absent");
            changed.replace(at, from.size(), to);
            std::ostringstream encoded;
            encoded << before.substr(0, section) << "PRACTICE " << std::quoted(changed)
                    << "\nCHECKSUM 0\n";
            rejected = false;
            try {
                s.restore(checksum(encoded.str()));
            } catch (const std::exception&) {
                rejected = true;
            }
            require(rejected && s.serialize() == before,
                    "semantic corruption accepted or partially committed");
        }
    }
}
void legacy() {
    TraversalSession old{make_community_course()};
    old.advance(10);
    const auto old_bytes = old.serialize();
    TraversalSession s{fixture()};
    const auto new_bytes = s.serialize();
    s.restore(old_bytes);
    require(!s.has_practice() && s.serialize() == old_bytes, "legacy load invented practice state");
    s.restore(new_bytes);
    require(s.has_practice() && s.serialize() == new_bytes,
            "device did not return after foreign load");
}
void horizon() {
    TraversalSession s{fixture()};
    s.advance(10001);
    const auto saved = s.serialize();
    s.advance(1);
    auto expected = s.serialize();
    s.restore(saved);
    s.advance(1);
    require(s.serialize() == expected, "research horizon leaked into live world");
}
void autonomy() {
    auto scene = fixture();
    scene.spawns[0].autonomous = true;
    scene.spawns[0].known_stations = 1;
    TraversalSession s{scene};
    s.advance(180);
    require(s.stations()[0].completed > 0 && s.practice_view(0).method_known,
            "resident did not discover a useful work method");
    require(s.practice_view(0).probes > 0, "work method was given without experience");
    const auto saved = s.serialize();
    s.advance(20);
    const auto expected = s.serialize();
    s.restore(saved);
    s.advance(20);
    require(s.serialize() == expected, "learned mind continuation differs");
}
void teaching() {
    TraversalSession s{fixture()};
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted, "teacher probe");
    s.advance(2);
    require(s.practice_probe(0, {0}, 64) == DiscoveryOutcome::Accepted, "teacher probe2");
    s.advance(2);
    require(s.share_practice(0, 1) == DiscoveryOutcome::Accepted, "physical teaching refused");
    require(s.practice_view(1).records == 0, "teaching completed before contact duration");
    auto saved = s.serialize();
    s.advance(1);
    auto expected = s.serialize();
    require(s.practice_view(1).records == 2 && s.practice_view(1).probes == 0,
            "student received no actual evidence");
    s.restore(saved);
    s.advance(1);
    require(s.serialize() == expected, "teaching restore differs");
    s.set_autonomous(1, true);
    s.advance(180);
    require(s.practice_view(1).method_known && s.stations()[0].completed > 0,
            "student could not use evidence");
}
void alternate() {
    auto scene = fixture();
    scene.practice->law = {{HypothesisKind::Fraction, 8}};
    scene.spawns[0].autonomous = true;
    scene.spawns[0].known_stations = 1;
    TraversalSession s{scene};
    s.advance(240);
    require(s.practice_view(0).method == DiscoveryChain{0} && s.stations()[0].completed >= 2,
            "known single-module method cannot be reused with new pulses");
}
void blocked_contact() {
    auto scene = fixture();
    scene.stations[0].location.local = {-1.5, 0};
    scene.geometry.bodies.push_back({2, {0, 2, -.1}, 0, .1, .7, 2, 0, 0, false});
    TraversalSession s{scene};
    require(!s.can_exchange_knowledge(0, 1), "fixture wall does not block contact");
    require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::Accepted,
            "blocked-contact own probe");
    s.advance(2);
    require(s.share_practice(0, 1) == DiscoveryOutcome::OutOfReach &&
                s.practice_view(1).records == 0,
            "evidence crossed a wall");
}
void safe_queue() {
    auto scene = fixture(6);
    for (auto& a : scene.spawns) {
        a.autonomous = true;
        a.known_stations = 1;
    }
    TraversalSession s{scene};
    s.advance(240);
    std::array<bool, 2> worked{};
    for (const auto& c : s.world().commands())
        if (c.result.status == RequestStatus::Accepted)
            for (std::size_t i = 0; i < 2; ++i)
                if (c.request.actor == s.actors()[i].id)
                    worked[i] = true;
    require(worked[0] && worked[1], "one operator monopolized work");
    s.validate();
    auto saved = s.serialize();
    s.restore(saved);
    require(s.serialize() == saved, "queue/contact snapshot changed");
}
void needs() {
    auto scene = fixture();
    scene.spawns[0].autonomous = true;
    scene.spawns[0].hunger = 800;
    TraversalSession s{scene};
    s.advance(100);
    require(s.practice_view(0).probes == 0 && s.stations()[0].completed == 0,
            "research overrode urgent hunger");
}
void manual_override() {
    auto scene = fixture();
    scene.spawns[0].autonomous = true;
    TraversalSession s{scene};
    for (unsigned i = 0; i < 40 && s.practice_view(0).stage != PracticeStage::Pulsing; ++i)
        s.advance(1);
    require(s.practice_view(0).stage == PracticeStage::Pulsing, "automatic probe never started");
    s.set_autonomous(0, false);
    s.set_goal(0, {1, {-4, 0}});
    static_cast<void>(s.share_knowledge(1, 0));
    s.advance(2);
    require(s.practice_view(0).records > 0,
            "manual override or message erased the physical result");
    s.advance(60);
    require(s.actors()[0].feet.x < -2, "manual order did not resume after pulse");
}
void community() {
    TraversalSession s{make_learning_course()};
    s.advance(600);
    s.validate();
    require(s.actors().size() == 8 && s.stations()[2].completed > 0,
            "learning was not integrated into the community");
    require(s.practice_view(3).method_known || s.practice_view(5).method_known,
            "community had no learned method");
    const auto saved = s.serialize();
    s.restore(saved);
    require(s.serialize() == saved, "community learning snapshot differs");
}
void refusals(bool remote) {
    TraversalSession s{fixture(remote ? 4 : 0, remote)};
    if (remote)
        require(s.practice_probe(0, {0}, 16) == DiscoveryOutcome::OutOfReach, "remote observation");
    else {
        require(s.practice_probe(0, {0, 0}, 16) == DiscoveryOutcome::Accepted, "calibration");
        s.advance(2);
        require(!s.begin_maintenance(0, 0) && s.stations()[0].completed == 0, "work without part");
    }
}
} // namespace
#endif
int main(int argc, char** argv) {
    try {
#if __has_include("lab/traversal_practice.hpp")
        if (argc == 6 && std::string(argv[1]) == "checkpoint") {
            const auto stage = std::stoul(argv[2]);
            TraversalSession s{fixture()};
            require(s.practice_probe(0, stage == 3 ? DiscoveryChain{0, 0} : DiscoveryChain{0},
                                     16) == DiscoveryOutcome::Accepted,
                    "checkpoint probe");
            s.advance(std::min(stage, 2UL));
            if (stage == 3) {
                require(s.begin_maintenance(0, 0), "checkpoint work");
                s.advance(17);
            }
            s.save(argv[3]);
            s.advance(std::stoull(argv[5]));
            s.save(argv[4]);
            return 0;
        }
        if (argc == 5 && std::string(argv[1]) == "continue") {
            TraversalSession s;
            s.load(argv[2]);
            s.advance(std::stoull(argv[4]));
            s.save(argv[3]);
            return 0;
        }
        std::string name = argc == 2 ? argv[1] : "";
        if (name == "calibration")
            calibration();
        else if (name == "energy")
            energy();
        else if (name == "concurrent")
            concurrent();
        else if (name == "no_parts")
            refusals(false);
        else if (name == "out_of_reach")
            refusals(true);
        else if (name == "protected")
            protected_probe();
        else if (name == "queued_input")
            queued_input();
        else if (name == "persistence")
            persistence();
        else if (name == "corruption")
            corruption();
        else if (name == "legacy")
            legacy();
        else if (name == "horizon")
            horizon();
        else if (name == "autonomy")
            autonomy();
        else if (name == "teaching")
            teaching();
        else if (name == "alternate")
            alternate();
        else if (name == "blocked_contact")
            blocked_contact();
        else if (name == "safe_queue")
            safe_queue();
        else if (name == "needs")
            needs();
        else if (name == "manual_override")
            manual_override();
        else if (name == "forged_work")
            forged_work();
        else if (name == "missing_permit")
            missing_permit();
        else if (name == "evidence_capacity")
            evidence_capacity();
        else if (name == "frozen_pending")
            frozen_pending();
        else if (name == "contact_support")
            contact_support();
        else if (name == "community")
            community();
        else
            throw std::runtime_error("unknown practice case");
        std::cout << "PASS " << name << '\n';
#else
        static_cast<void>(argv);
        if (argc > 0)
            throw std::runtime_error("practice domain integration is missing");
#endif
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
