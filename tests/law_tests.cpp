#include "lab/traversal_session.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace reality;
namespace {
void require(bool c, const char* m) {
    if (!c)
        throw std::runtime_error(m);
}
TraversalScene scene(bool witness = true) {
    TraversalScene s;
    s.geometry.bodies = {{1, {}, 0, 20, 8, .3, 0, 0, true}};
    s.stations = {{{1, {0, 0}}, TraversalTask::Maintain, 12},
                  {{1, {12, 0}}, TraversalTask::Maintain, 0}};
    for (auto& station : s.stations)
        station.initial_material = {2};
    s.spawns = {{{1, {0, -.8}}, {}, false},
                {{1, {witness ? 1.2 : 7, -.8}}, {}, false},
                {{1, {-8, 0}}, {}, false},
                {{1, {3, 3}}, {}, false}};
    s.property_rules = {{0, 2, {3}, 3}};
    return s;
}
void step(TraversalSession& s, unsigned n = 1) {
    for (unsigned i = 0; i < n; ++i) {
        s.advance(1);
        s.validate();
    }
}
void go(TraversalSession& s, std::size_t a, SurfaceLocation to) {
    s.set_goal(a, to);
    for (unsigned i = 0; i < 240; ++i) {
        step(s);
        if (s.actors()[a].state == TraversalState::Arrived && s.actors()[a].motion.speed <= .01)
            return;
    }
    throw std::runtime_error("physical arrival failed");
}
void take(TraversalSession& s) {
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "physical theft was blocked");
    require(s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 8,
            "transfer balance failed");
    s.validate();
}
void unseen() {
    TraversalSession s{scene(false)};
    take(s);
    require(s.incidents().size() == 1, "unseen violation was not recorded as physical fact");
    for (const auto& a : s.actors())
        require(a.testimonies.empty(), "unseen offender magically known");
    require(s.legal_service_allowed(0, 0), "uninformed institution reacted");
    require(s.begin_maintenance(0, 0), "uninformed institution refused actual service");
}
void isolated() {
    TraversalSession s{scene()};
    take(s);
    require(s.actors()[1].testimonies.size() == 1, "eyewitness has no testimony");
    require(s.actors()[2].testimonies.empty() && s.legal_service_allowed(0, 0),
            "custodian knows remotely");
    require(s.share_knowledge(1, 2) == KnowledgeOutcome::OutOfReach, "remote report accepted");
    step(s, 45);
    require(s.actors()[2].testimonies.empty(), "distant custodian acquired knowledge");
    require(s.begin_maintenance(0, 0), "unknown crime denied real service");
}
void relay() {
    TraversalSession s{scene()};
    step(s, 3);
    take(s);
    const auto original = s.actors()[1].testimonies.at(0);
    go(s, 1, {1, {-6.5, 0}});
    require(s.share_knowledge(1, 2) == KnowledgeOutcome::Shared, "contact did not transmit report");
    const auto& b = s.actors()[2].testimonies.at(0);
    require(b.source_actor == s.actors()[1].id && b.via_actor == s.actors()[1].id &&
                b.observed_tick == original.observed_tick && b.learned_tick > b.observed_tick,
            "crime provenance changed in transit");
    require(!s.legal_service_allowed(0, 0) && !s.begin_maintenance(0, 0),
            "known crime did not deny actual service");
    require(s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4,
            "reaction teleported cargo");
}
void restitution() {
    auto f = scene();
    f.spawns[2].location = {1, {-1.2, -.8}};
    TraversalSession s{f};
    take(s);
    require(!s.legal_service_allowed(0, 0), "custodian eyewitness did not react");
    TraversalSession copy;
    copy.restore(s.serialize());
    require(s.serialize().starts_with("GENESIS_TRAVERSAL 7\n"), "law not versioned");
    for (unsigned i = 0; i < 20; ++i) {
        step(s);
        step(copy);
        require(s.serialize() == copy.serialize(), "continuation differs");
    }
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "physical return failed");
    require(s.incidents()[0].returned == 4 && s.legal_service_allowed(0, 0),
            "witnessed return did not settle");
    require(s.world().resource(s.stations()[0].lot)->record.quantity.value == 12,
            "returned material missing");
    copy.restore(s.serialize());
    require(copy.serialize() == s.serialize(), "settlement lost in save");
    require(copy.begin_maintenance(0, 0), "service not restored after settlement");
}
void geometry() {
    for (unsigned variant = 0; variant < 2; ++variant) {
        auto f = scene();
        if (variant == 0)
            f.geometry.bodies.push_back({2, {.6, 2, -.8}, 0, .05, 1, 2, 0, 0, false});
        else {
            f.geometry.bodies.push_back({2, {1.2, 1.5, -.8}, 0, .5, .5, .3, 0, 0, true});
            f.spawns[1].location = {2, {0, 0}};
        }
        TraversalSession s{f};
        require(s.share_knowledge(0, 1) == KnowledgeOutcome::Blocked,
                "fixture does not block contact");
        take(s);
        require(s.actors()[1].testimonies.empty(), "occluded/different-support witness saw crime");
        require(s.legal_service_allowed(0, 0), "invisible crime triggered institution");
        go(s, 2, {1, {-1.2, -.8}});
        require(s.actors()[2].testimonies.empty(), "late arrival identified unseen offender");
    }
}
void repeated_return() {
    TraversalSession s{scene()};
    for (unsigned i = 0; i < 171; ++i) {
        require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "repeated pickup failed");
        require(s.interact_cargo(0, 0) == CargoStatus::Transferred,
                "return blocked below actual testimony limit");
    }
    require(s.actors()[1].testimonies.size() == 342 && s.incidents().size() == 171,
            "repeated flow evidence lost");
    s.validate();
}
void owner() {
    TraversalSession s{scene()};
    require(s.world().resource(s.stations()[0].lot)->record.owner == s.actors()[2].id,
            "protected lot owner is not its custodian");
}
void authorized() {
    for (unsigned variant = 0; variant < 3; ++variant) {
        auto f = scene();
        if (variant == 1)
            f.property_rules[0].residents.clear();
        TraversalSession s{f};
        if (variant == 1)
            s.assign_delivery(3, 0, 1, 4);
        go(s, 3, {1, {0, .5}});
        require(s.interact_cargo(3, 0) == CargoStatus::Transferred, "authorized pickup failed");
        require(s.incidents().empty(), "authorized pickup became theft");
        s.validate();
        if (variant == 1) {
            s.cancel_delivery(3);
            require(s.interact_cargo(3, 0) == CargoStatus::Transferred, "courier return failed");
            require(s.interact_cargo(3, 0) == CargoStatus::Transferred,
                    "unassigned courier physically blocked");
            require(s.incidents().size() == 1, "unassigned courier remained authorized");
        }
    }
}
void remote_return() {
    TraversalSession s{scene()};
    take(s);
    go(s, 1, {1, {-6.5, 0}});
    require(s.share_knowledge(1, 2) == KnowledgeOutcome::Shared, "crime report failed");
    require(!s.legal_service_allowed(0, 0), "crime did not deny service");
    go(s, 1, {1, {1.2, -.8}});
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "return failed");
    require(s.incidents()[0].returned == 4 && !s.legal_service_allowed(0, 0),
            "remote closure leaked to custodian");
    go(s, 1, {1, {-6.5, 0}});
    require(s.share_knowledge(1, 2) == KnowledgeOutcome::Shared && s.legal_service_allowed(0, 0),
            "witnessed closure did not propagate");
    require(s.actors()[2].testimonies.size() == 2, "crime/closure provenance was merged");
}
void bounded_auto() {
    auto f = scene();
    f.spawns[1].autonomous = true;
    f.spawns[2].location = {1, {2.8, -.8}};
    f.spawns[1].known_stations = 0;
    TraversalSession s{f};
    take(s);
    require(s.actors()[2].testimonies.empty(), "custodian already saw crime");
    step(s);
    require(!s.legal_service_allowed(0, 0), "eyewitness automatic contact did not react");
    require(s.actors()[2].share_until == 0, "received crime opened relay automation");
    step(s, 45);
    const auto source = s.actors()[2].testimonies[0].source_actor;
    require(source == s.actors()[1].id && s.actors()[2].testimonies[0].observed_tick == 0,
            "automatic relay refreshed observation");
}
void multiple() {
    TraversalSession s{scene()};
    take(s);
    go(s, 0, {1, {12, -.8}});
    require(s.interact_cargo(0, 1) == CargoStatus::Transferred, "physical dump failed");
    require(s.incidents()[0].returned == 0, "wrong station paid debt");
    go(s, 0, {1, {0, -.8}});
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "second theft blocked");
    require(s.incidents().size() == 2, "second transfer lost identity");
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "return failed");
    require(s.incidents()[0].returned == 4 && s.incidents()[1].returned == 0,
            "one return paid twice");
    s.validate();
    TraversalSession copy;
    copy.restore(s.serialize());
    require(copy.serialize() == s.serialize(), "FIFO state lost");
}
void unseen_return() {
    TraversalSession s{scene(false)};
    take(s);
    go(s, 1, {1, {1.2, -.8}});
    require(s.actors()[1].testimonies.empty(), "late witness learned unseen theft");
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "physical return failed");
    require(s.actors()[1].testimonies.empty(), "return invented knowledge of unseen crime");
    require(s.legal_service_allowed(0, 0), "unseen return incriminated offender");
}

void multi_relay() {
    TraversalSession s{scene()};
    take(s);
    go(s, 1, {1, {3, 1.5}});
    require(s.share_knowledge(1, 3) == KnowledgeOutcome::Shared, "first relay failed");
    go(s, 3, {1, {-6.5, 0}});
    require(s.share_knowledge(3, 2) == KnowledgeOutcome::Shared, "second relay failed");
    const auto& b = s.actors()[2].testimonies.at(0);
    require(b.source_actor == s.actors()[1].id && b.via_actor == s.actors()[3].id &&
                b.observed_tick == 0,
            "multi-relay lost eyewitness or time");
    TraversalSession copy;
    copy.restore(s.serialize());
    for (unsigned i = 0; i < 20; ++i) {
        step(s);
        step(copy);
        require(s.serialize() == copy.serialize(), "relayed continuation differs");
    }
    const auto bytes = s.serialize();
    require(s.share_knowledge(1, 2) == KnowledgeOutcome::OutOfReach && s.serialize() == bytes,
            "remote relay mutated memory");
    require(!s.legal_service_allowed(0, 0), "relayed report did not react");
}
void safe_action() {
    TraversalSession s{scene()};
    take(s);
    go(s, 1, {1, {-6.5, 0}});
    require(s.begin_maintenance(0, 0), "uninformed custodian refused service");
    const auto action = s.actors()[0].action;
    require(s.world().action(action)->state == ActionState::Running, "fixture missed running work");
    require(s.share_knowledge(1, 2) == KnowledgeOutcome::Shared && !s.legal_service_allowed(0, 0),
            "report did not reach custodian");
    require(s.world().action(action)->state == ActionState::Running,
            "report interrupted started work");
    TraversalSession copy;
    copy.restore(s.serialize());
    step(s, 85);
    step(copy, 85);
    require(s.serialize() == copy.serialize() &&
                s.world().action(action)->state == ActionState::Completed &&
                s.stations()[0].completed == 1,
            "protected work failed to finish once");
    const auto stock = s.world().resource(s.stations()[0].lot)->record.quantity;
    require(!s.begin_maintenance(0, 0) &&
                s.world().resource(s.stations()[0].lot)->record.quantity == stock,
            "new service ignored reaction");
}
void limits() {
    TraversalSession s{scene(false)};
    for (unsigned i = 0; i < 256; ++i) {
        require(s.interact_cargo(0, 0) == CargoStatus::Transferred,
                "pickup before incident limit failed");
        require(s.interact_cargo(0, 0) == CargoStatus::Transferred,
                "return at incident limit failed");
    }
    const auto bytes = s.serialize();
    bool rejected = false;
    try {
        static_cast<void>(s.interact_cargo(0, 0));
    } catch (const std::length_error&) {
        rejected = true;
    }
    require(rejected && s.serialize() == bytes, "incident limit left partial transfer");
    go(s, 3, {1, {0, .5}});
    require(s.interact_cargo(3, 0) == CargoStatus::Transferred && s.incidents().size() == 256,
            "incident limit blocked authorized resident");
}

std::string edit(std::string bytes, const std::string& tag, unsigned field,
                 const std::string& value, unsigned occurrence = 0) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    auto found = bytes.find("\n" + tag + " ");
    for (unsigned i = 0; i < occurrence; ++i)
        found = bytes.find("\n" + tag + " ", found + 1);
    const auto begin = found + 1;
    require(begin != 0, "missing corruption fixture");
    const auto end = bytes.find('\n', begin);
    std::istringstream in{bytes.substr(begin, end - begin)};
    std::ostringstream out;
    std::string token;
    unsigned i = 0;
    while (in >> token) {
        if (i)
            out << ' ';
        out << (i++ == field ? value : token);
    }
    bytes.replace(begin, end - begin, out.str());
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n';
}
void corruption() {
    TraversalSession s{scene()};
    take(s);
    const auto original = s.serialize();
    const auto reject = [&](const std::string& bytes) {
        bool rejected = false;
        try {
            s.restore(bytes);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && s.serialize() == original, "corrupt law data replaced current world");
    };
    for (const auto field : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u})
        reject(edit(original, "INCIDENT", field, "999"));
    for (const auto field : {1u, 2u, 3u, 4u, 5u, 7u, 8u, 9u, 10u})
        reject(edit(original, "TESTIMONY", field, "999"));
    reject(edit(original, "TESTIMONY", 6, "1"));
    reject(edit(original, "TESTIMONY", 9, std::to_string(s.actors()[0].id.value)));
    reject(edit(original, "TESTIMONY", 10, std::to_string(s.actors()[2].id.value)));
    reject(edit(original, "PROPERTY", 2, "999"));
    reject(edit(original, "PROPERTY", 3, "999"));
    reject(edit(original, "PROPERTY", 1, "15"));
    reject(edit(original, "INCIDENTS", 1, "257"));
    reject(edit(original, "TESTIMONIES", 1, "513"));
    auto duplicate = original.substr(0, original.rfind("CHECKSUM "));
    const auto start = duplicate.find("TESTIMONY ");
    const auto end = duplicate.find('\n', start);
    duplicate.insert(end + 1, duplicate.substr(start, end - start + 1));
    const auto count = duplicate.rfind("TESTIMONIES 1", start);
    duplicate.replace(count, 13, "TESTIMONIES 2");
    reject(duplicate + "CHECKSUM " + std::to_string(snapshot_checksum(duplicate)) + '\n');
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "settlement fixture failed");
    const auto settled = s.serialize();
    bool rejected = false;
    try {
        s.restore(edit(settled, "INCIDENT", 8, "0"));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && s.serialize() == settled, "unbacked settlement accepted");
}
void empty_corruption() {
    TraversalSession s{scene()};
    const auto original = s.serialize();
    auto body = original.substr(0, original.rfind("CHECKSUM "));
    const auto first = body.find('\n', body.find("STATIONS ")) + 1;
    const auto end = body.find('\n', first);
    std::istringstream input{body.substr(first, end - first)};
    std::ostringstream output;
    std::string token;
    unsigned field = 0;
    while (input >> token) {
        if (field)
            output << ' ';
        output << (field++ == 5 ? "999" : token);
    }
    body.replace(first, end - first, output.str());
    bool rejected = false;
    try {
        s.restore(body + "CHECKSUM " + std::to_string(snapshot_checksum(body)) + '\n');
    } catch (const std::invalid_argument& e) {
        rejected = std::string{e.what()} == "invalid protected station resource";
    }
    require(rejected && s.serialize() == original,
            "missing protected lot did not get explicit transactional rejection");
}
void settlement_chronology() {
    TraversalSession s{scene()};
    take(s);
    go(s, 3, {1, {0, .8}});
    require(s.share_knowledge(1, 3) == KnowledgeOutcome::Shared,
            "future settlement witness did not learn crime");
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "settlement fixture failed");
    step(s, 5);
    const auto original = s.serialize();
    // Actor 1 has crime+settlement; actor 3's relayed crime is the third testimony.
    const auto corrupt =
        edit(original, "TESTIMONY", 8, std::to_string(s.world().clock().tick_count()), 2);
    bool rejected = false;
    try {
        s.restore(corrupt);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && s.serialize() == original,
            "settlement witness learned crime after observing its return");
}

void legacy_sections() {
    TraversalSession old{make_traversal_course()};
    const auto canonical = old.serialize();
    require(canonical.starts_with("GENESIS_TRAVERSAL 1\n"),
            "fixture is not historical base layout");
    for (unsigned version : {2u, 3u, 5u}) {
        std::istringstream input{canonical.substr(0, canonical.rfind("CHECKSUM "))};
        std::ostringstream output;
        std::string line;
        while (std::getline(input, line)) {
            if (line == "GENESIS_TRAVERSAL 1")
                line = "GENESIS_TRAVERSAL " + std::to_string(version);
            output << line << '\n';
            if (line.starts_with("MEMORY ")) {
                output << "AUTONOMY 0 0 0\n";
                if (version >= 3)
                    output << "CLEARANCE 0 0 0 0\nYIELD 0\n";
                if (version >= 4)
                    output << "CARGO 0\nDELIVERY 0\n";
                if (version >= 5)
                    output << "KNOWLEDGE 0 0 0\n";
            }
        }
        const auto body = output.str();
        TraversalSession s{scene()};
        take(s);
        s.restore(body + "CHECKSUM " + std::to_string(snapshot_checksum(body)) + '\n');
        require(s.serialize() == canonical && s.incidents().empty(),
                "old format retained law state");
    }
}

void legacy() {
    for (auto f : {make_traversal_course(), make_delivery_course(), make_crossing_course()}) {
        TraversalSession old{f};
        const auto bytes = old.serialize();
        TraversalSession law{scene()};
        take(law);
        law.restore(bytes);
        require(law.incidents().empty() && law.serialize() == bytes,
                "legacy load retained new law state");
        step(law);
        step(old);
        require(law.serialize() == old.serialize(), "legacy continuation changed");
    }
}

} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "case required");
        const std::string name = argv[1];
        if (name == "unseen")
            unseen();
        else if (name == "isolated")
            isolated();
        else if (name == "relay")
            relay();
        else if (name == "restitution")
            restitution();
        else if (name == "empty_corruption")
            empty_corruption();
        else if (name == "settlement_chronology")
            settlement_chronology();
        else if (name == "unseen_return")
            unseen_return();
        else if (name == "multi_relay")
            multi_relay();
        else if (name == "safe_action")
            safe_action();
        else if (name == "limits")
            limits();
        else if (name == "legacy_sections")
            legacy_sections();
        else if (name == "repeated_return")
            repeated_return();
        else if (name == "owner")
            owner();
        else if (name == "geometry")
            geometry();
        else if (name == "authorized")
            authorized();
        else if (name == "remote_return")
            remote_return();
        else if (name == "bounded_auto")
            bounded_auto();
        else if (name == "multiple")
            multiple();
        else if (name == "corruption")
            corruption();
        else if (name == "legacy")
            legacy();
        else
            throw std::runtime_error("unknown case");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
