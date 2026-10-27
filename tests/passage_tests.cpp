#include "lab/traversal_session.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace reality;
namespace {
void require(bool c, const char* m) {
    if (!c)
        throw std::runtime_error(m);
}
void step(TraversalSession& s, unsigned n = 1) {
    for (unsigned i = 0; i < n; ++i) {
        s.advance(1);
        s.validate();
    }
}
bool bridge(const TraversalActor& a) {
    return std::any_of(a.route.legs.begin(), a.route.legs.end(),
                       [](const auto& l) { return l.passage == 1; });
}
bool ferry(const TraversalActor& a) {
    return std::any_of(a.route.legs.begin(), a.route.legs.end(),
                       [](const auto& l) { return l.transport == 1; });
}
bool in_step(const TraversalActor& a) {
    return a.leg < a.route.legs.size() && a.route.legs[a.leg].kind == SurfaceLegKind::Step &&
           a.step_point > 0;
}
TraversalScene course() {
    auto scene = make_crossing_course();
    scene.spawns[3].autonomous = false;
    return scene;
}
void loaded(TraversalSession& s) {
    for (unsigned t = 0;
         t < 100 && s.world().resource(s.actors()[1].cargo)->record.quantity.value == 0; ++t)
        step(s);
    require(s.world().resource(s.actors()[1].cargo)->record.quantity.value > 0,
            "courier did not physically load");
    step(s, 2);
    require(bridge(s.actors()[1]), "default route did not choose faster bridge");
}
void discovery() {
    TraversalSession s{course()};
    loaded(s);
    const auto quantity = s.world().resource(s.actors()[1].cargo)->record.quantity.value;
    const auto version = s.geometry().version;
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed, "local closure rejected");
    require(s.geometry().version == version + 1 && !s.actors()[1].passage_beliefs[0],
            "remote closure leaked knowledge");
    step(s);
    require(bridge(s.actors()[1]) && !s.actors()[1].passage_beliefs[0],
            "global version telepathically replaced courier route");
    bool saw = false, rode = false;
    for (unsigned t = 0; t < 750 && s.stations()[1].completed == 0; ++t) {
        step(s);
        const auto& a = s.actors()[1];
        if (a.passage_beliefs[0] && !a.passage_beliefs[0]->open) {
            saw = true;
            require(a.passage_beliefs[0]->source_actor == a.id,
                    "courier did not discover closure locally");
        }
        rode = rode || a.transfer == TransferStage::Riding;
        require(a.support.surface != s.passages()[0].surface,
                "courier physically entered closed bridge");
    }
    if (!(saw && rode && s.stations()[1].completed > 0))
        std::cerr << s.inspect();
    require(saw && rode && s.actors()[1].delivery->delivered >= quantity &&
                s.stations()[1].completed > 0,
            "discovery did not restore real food delivery and meal by ferry");
}
void witness() {
    auto scene = course();
    scene.spawns[1].location = {1, {1, -3}};
    scene.initial_deliveries.clear();
    scene.spawns[1].autonomous = false;
    TraversalSession s{scene};
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed, "witness closure failed");
    step(s);
    require(s.observe_passage(0, 1), "witness cannot see local closure");
    const auto time = s.actors()[0].passage_beliefs[0]->observed_tick;
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared,
            "closure report did not reach nearby courier");
    const auto& b = *s.actors()[1].passage_beliefs[0];
    require(!b.open && b.source_actor == s.actors()[0].id && b.via_actor == s.actors()[0].id &&
                b.observed_tick == time,
            "closure report lost its original witness or time");
    s.set_goal(1, {2, {1, 0}});
    require(ferry(s.actors()[1]) && !bridge(s.actors()[1]),
            "informed courier did not immediately choose detour");
}
void repair() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[1].location = {1, {1, -3}};
    scene.spawns[1].autonomous = false;
    scene.passages[0].open = false;
    scene.stations[2].wear = 1000;
    TraversalSession s{scene};
    require(s.observe_passage(0, 1), "closed bridge not visible to repairer");
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared, "initial negative report failed");
    const auto stock = s.world().resource(s.stations()[2].lot)->record.quantity.value;
    require(s.begin_maintenance(0, 2), "local material maintenance did not start");
    require(s.world().resource(s.stations()[2].lot)->record.quantity.value == stock - 1 &&
                !s.passages()[0].open,
            "repair opened before real work or did not consume material");
    step(s, 79);
    require(!s.passages()[0].open, "unfinished repair opened passage");
    step(s, 2);
    require(s.passages()[0].open && s.stations()[2].wear < 1000 && s.stations()[2].completed == 1,
            "completed material maintenance did not restore passage");
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared &&
                s.actors()[1].passage_beliefs[0]->open,
            "repair information did not replace older closure");
    s.assign_delivery(1, 0, 1, 4);
    s.set_goal(1, s.stations()[0].location);
    for (unsigned t = 0; t < 120 && (s.actors()[1].state != TraversalState::Arrived ||
                                     s.actors()[1].motion.speed > .01);
         ++t)
        step(s);
    require(s.interact_cargo(1, 0) == CargoStatus::Transferred &&
                s.world().resource(s.actors()[1].cargo)->record.quantity.value == 4,
            "repaired-route courier did not physically load food");
    s.set_goal(1, s.stations()[1].location);
    require(bridge(s.actors()[1]) && !ferry(s.actors()[1]), "repair did not restore shorter route");
    bool crossed = false;
    for (unsigned t = 0; t < 300 && s.actors()[1].state != TraversalState::Arrived; ++t) {
        step(s);
        crossed = crossed || s.actors()[1].support.surface == 3;
    }
    require(crossed && s.actors()[1].state == TraversalState::Arrived,
            "repaired bridge was not physically traversed");
    require(s.interact_cargo(1, 1) == CargoStatus::Transferred &&
                s.actors()[1].delivery->delivered == 4 &&
                s.world().resource(s.stations()[1].lot)->record.quantity.value == 4,
            "restored shorter route did not deliver actual food");
}
void occupied() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[1] = {{3, {0, 0}}, {}, false};
    TraversalSession s{scene};
    const auto before = s.serialize();
    require(s.interact_passage(0, 1, false) == PassageOutcome::Occupied && s.serialize() == before,
            "closure on bridge occupant changed world");
}
void step_refusal() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[1] = {{1, {1, -2}}, {}, false};
    scene.spawns[3].location = {1, {-2, -3}};
    TraversalSession s{scene};
    s.set_goal(1, {2, {1, -2}});
    for (unsigned t = 0; t < 100 && !in_step(s.actors()[1]); ++t)
        step(s);
    require(in_step(s.actors()[1]), "fixture did not start bridge Step");
    const auto before = s.serialize();
    require(s.interact_passage(0, 1, false) == PassageOutcome::Occupied && s.serialize() == before,
            "closure interrupted a committed Step");
}
void local_manual() {
    TraversalSession s{course()};
    const auto before = s.serialize();
    require(s.interact_passage(1, 1, false) == PassageOutcome::Controlled &&
                s.serialize() == before,
            "player operation commandeered autonomous actor");
    s.set_autonomous(1, false);
    const auto far = s.serialize();
    require(s.interact_passage(1, 1, false) == PassageOutcome::OutOfReach && s.serialize() == far,
            "remote player operation closed passage");
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed, "fixture closure failed");
    s.set_goal(0, {3, {0, 0}});
    step(s, 150);
    require(s.actors()[0].support.surface != 3, "manual route entered closed passage");
}
void community() {
    for (bool disturbed : {false, true}) {
        TraversalSession s{make_crossing_course()};
        if (disturbed)
            require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
                    "disturbance failed");
        for (unsigned t = 0; t < 550; ++t)
            step(s);
        require(s.stations()[2].completed > 0 && s.stations()[1].completed > 0 &&
                    s.actors()[1].delivery->delivered > 0,
                "community did not physically maintain, deliver and eat");
    }
}
void persistence() {
    TraversalSession s{course()};
    loaded(s);
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed, "fixture closure failed");
    step(s, 110);
    const auto bytes = s.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 6\n"), "passage state lacks version6");
    TraversalSession copy;
    copy.restore(bytes);
    require(copy.serialize() == bytes, "passage save did not roundtrip");
    for (unsigned t = 0; t < 200; ++t) {
        step(s);
        step(copy);
        require(s.serialize() == copy.serialize(), "passage continuation diverged");
    }
}
std::string corrupt(std::string bytes, std::string_view tag, unsigned field,
                    const std::string& value) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto first = bytes.find(tag);
    require(first != std::string::npos, "missing corruption fixture record");
    const auto end = bytes.find('\n', first);
    std::istringstream in{bytes.substr(first, end - first)};
    std::ostringstream out;
    std::string word;
    unsigned n = 0;
    while (in >> word) {
        if (n)
            out << ' ';
        out << (n++ == field ? value : word);
    }
    bytes.replace(first, end - first, out.str());
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n';
}
void corruption() {
    TraversalSession s{course()};
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed, "fixture closure failed");
    const auto bytes = s.serialize();
    for (const auto& bad :
         {corrupt(bytes, "PASSAGE_STATE ", 1, "999"), corrupt(bytes, "PASSAGE_STATE ", 2, "2"),
          corrupt(bytes, "PASSAGE_STATE ", 4, "0"), corrupt(bytes, "PASSAGE_BELIEF ", 2, "2"),
          corrupt(bytes, "PASSAGE_BELIEF ", 3, "999"), corrupt(bytes, "PASSAGE_BELIEF ", 5, "0"),
          corrupt(bytes, "PASSAGE_BELIEF ", 6, "999"),
          corrupt(bytes, "PASSAGE_MEMORY ", 1, "65536"), corrupt(bytes, "PASSAGE_MEMORY ", 2, "17"),
          corrupt(bytes, "PASSAGE_STATE ", 4, "15"), corrupt(bytes, "PASSAGE_BELIEF ", 1, "999"),
          corrupt(bytes, "PASSAGE_BELIEF ", 4, "999"),
          corrupt(bytes, "PASSAGE_BELIEF ", 5, std::to_string(s.stations()[2].lot.value))}) {
        bool rejected = false;
        try {
            s.restore(bad);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && s.serialize() == bytes, "corrupt passage snapshot replaced world");
    }
    const auto reject = [&](const std::string& bad) {
        bool rejected = false;
        try {
            s.restore(bad);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && s.serialize() == bytes, "corrupt passage structure replaced world");
    };
    auto duplicate = corrupt(bytes, "PASSAGE_MEMORY ", 2, "2");
    duplicate.resize(duplicate.rfind("CHECKSUM "));
    const auto first = duplicate.find("PASSAGE_BELIEF "), end = duplicate.find('\n', first) + 1;
    duplicate.insert(end, duplicate.substr(first, end - first));
    reject(duplicate + "CHECKSUM " + std::to_string(snapshot_checksum(duplicate)) + "\n");
    auto queued = bytes.substr(0, bytes.rfind("CHECKSUM "));
    const auto row = queued.find('\n', queued.find("PASSAGES ")) + 1,
               row_end = queued.find('\n', row);
    queued.replace(row, row_end - row, "1 3 1 1 1");
    reject(queued + "CHECKSUM " + std::to_string(snapshot_checksum(queued)) + "\n");
    auto occupied_scene = course();
    occupied_scene.initial_deliveries.clear();
    occupied_scene.spawns[1] = {{3, {0, 0}}, {}, false};
    TraversalSession occupied_world{occupied_scene};
    reject(corrupt(occupied_world.serialize(), "PASSAGE_STATE ", 2, "0"));
    auto wrong_link = course();
    wrong_link.initial_deliveries.clear();
    wrong_link.stations[2].initial_material = {1};
    bool invalid = false;
    try {
        TraversalSession bad{wrong_link};
    } catch (const std::invalid_argument&) {
        invalid = true;
    }
    require(invalid, "passage accepted wrong maintenance material");
    TraversalSession routed{course()};
    loaded(routed);
    reject(corrupt(routed.serialize(), "PASSAGE_MEMORY 0 0", 1, "1"));
}
void observation_ring() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[1] = {{1, {1, -2}}, {}, false};
    scene.spawns[3].location = {1, {-2, -3}};
    TraversalSession s{scene};
    s.set_goal(1, {2, {1, -2}});
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
            "ring fixture did not close");
    require(!s.actors()[1].passage_beliefs[0], "ring actor already knew closure");
    step(s, 15);
    require(s.actors()[1].passage_beliefs[0] && !s.actors()[1].passage_beliefs[0]->open &&
                ferry(s.actors()[1]),
            "unaware actor froze outside observation range instead of approaching and discovering "
            "closure");
}
void wear_and_queue() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.stations[2].wear = 999;
    TraversalSession s{scene};
    step(s, 2);
    require(!s.passages()[0].open && s.stations()[2].wear == 1000,
            "wear threshold did not close bridge");
    const auto before = s.serialize();
    require(s.interact_passage(0, 1, true) == PassageOutcome::Worn && s.serialize() == before,
            "manual opening bypassed real maintenance");
    scene.spawns[1] = {{3, {0, 0}}, {}, false};
    TraversalSession occupied_world{scene};
    step(occupied_world, 2);
    require(occupied_world.passages()[0].open && occupied_world.stations()[2].wear == 1000,
            "automatic wear closure displaced occupant");
    TraversalSession copy;
    copy.restore(occupied_world.serialize());
    copy.set_goal(1, {2, {1, -2}});
    for (unsigned t = 0; t < 250 && copy.passages()[0].open; ++t)
        step(copy);
    require(!copy.passages()[0].open,
            "deferred wear closure never retried after physical departure");
}
void manual_barrier() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[3].location = {1, {-2, -3}};
    scene.spawns[0].location = {1, {2, -2}};
    TraversalSession s{scene};
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
            "manual barrier closure failed");
    s.set_input(0, {1, 0});
    step(s, 100);
    s.clear_input();
    require(s.actors()[0].support.surface == 1 && s.actors()[0].feet.x < -2.29,
            "WASD physically bypassed closed bridge");
}
void no_path_scout() {
    auto scene = course();
    scene.initial_deliveries.clear();
    std::erase_if(scene.connections, [](const auto& c) { return c.transport != 0; });
    scene.services.clear();
    scene.platforms.clear();
    scene.geometry.bodies.pop_back();
    scene.spawns[1] = {{1, {1, -2}}, {}, false};
    scene.spawns[3].location = {1, {-2, -3}};
    TraversalSession s{scene};
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed, "no-path closure failed");
    s.set_goal(1, {2, {1, -2}});
    step(s, 30);
    require(s.actors()[1].state == TraversalState::NoPath && s.actors()[1].goal,
            "closed unique route did not retain goal with NoPath");
    const auto at = s.actors()[1].passage_beliefs[0]->observed_tick;
    s.set_goal(1, {1, {-.5, -2}});
    for (unsigned t = 0; t < 100 && s.actors()[1].state != TraversalState::Arrived; ++t)
        step(s);
    step(s, 110);
    s.set_goal(1, {2, {1, -2}});
    require(s.world().clock().tick_count() >= at + 100 && bridge(s.actors()[1]),
            "expired negative report never allowed new scouting plan");
}
void seeded_report(TraversalSession& s, std::size_t actor, std::uint32_t passage, bool open) {
    // Safety-only historical memory fixture; physical acquisition is tested above.
    auto bytes = s.serialize();
    bytes.resize(bytes.rfind("CHECKSUM "));
    auto start = bytes.find("\nACTOR ");
    for (std::size_t i = 0; i < actor; ++i)
        start = bytes.find("\nACTOR ", start + 1);
    const auto first = bytes.find("PASSAGE_MEMORY ", start), end = bytes.find("ROUTE ", first);
    const auto now = s.world().clock().tick_count(), id = s.actors()[actor].id.value;
    const auto line = "PASSAGE_MEMORY 0 1\nPASSAGE_BELIEF " + std::to_string(passage) + " " +
                      std::to_string(open) + " " + std::to_string(now) + " " + std::to_string(now) +
                      " " + std::to_string(id) + " " + std::to_string(id) + "\n";
    bytes.replace(first, end - first, line);
    s.restore(bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + "\n");
}
void protected_ride(TransferStage phase = TransferStage::Riding) {
    auto scene = course();
    scene.spawns[0].location = {4, {0, .65}};
    scene.platforms[0].definition.travel_ticks = 20;
    for (auto& c : scene.connections)
        if (c.passage)
            c.extra_cost_m = 100;
    TraversalSession s{scene};
    for (unsigned t = 0;
         t < 350 && !(s.actors()[1].transfer == phase && s.actors()[1].support.surface == 4); ++t)
        step(s);
    require(s.actors()[1].transfer == phase,
            "fixture did not physically reach protected ferry phase");
    seeded_report(s, 0, 1, false);
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared, "onboard report not received");
    const auto feet = s.actors()[1].feet;
    step(s);
    require(s.actors()[1].transfer != TransferStage::None &&
                s.actors()[1].task == TraversalTask::Deliver &&
                s.actors()[1].planned_closed_passages == 0 &&
                length(s.actors()[1].feet - feet) < .2,
            "closure report interrupted or teleported protected rider");
    for (unsigned t = 0; t < 200 && s.actors()[1].transfer != TransferStage::None; ++t)
        step(s);
    step(s, 2);
    require(s.actors()[1].support.surface == 2 && s.actors()[1].passage_beliefs[0] &&
                !s.actors()[1].passage_beliefs[0]->open &&
                s.actors()[1].passage_beliefs[0]->source_actor == s.actors()[0].id,
            "rider did not physically exit with received knowledge intact");
}
void protected_step() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-2, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {2, .2, 0}, 0, 2, 2, .3, 0, 0, true},
                             {3, {6, .2, 0}, 0, 2, 2, .3, 0, 0, true}};
    scene.connections = {{{1, {1.7, 0}}, {2, {-1.7, 0}}, SurfaceLegKind::Step, 1},
                         {{2, {1.7, 0}}, {3, {-1.7, 0}}, SurfaceLegKind::Step, 2}};
    scene.passages = {{1, 2, {}, {}}, {2, 3, {}, {}}};
    scene.spawns = {{{1, {1.2, 1}}, {}, false}, {{1, {.5, 0}}, {}, false}};
    TraversalSession s{scene};
    s.set_goal(1, {3, {1, 0}});
    for (unsigned t = 0; t < 100 && !in_step(s.actors()[1]); ++t)
        step(s);
    require(in_step(s.actors()[1]), "fixture did not commit Step");
    require(s.observe_passage(0, 1), "fixture source not at bank");
    seeded_report(s, 0, 2, false);
    require(s.share_knowledge(0, 1) == KnowledgeOutcome::Shared,
            "step recipient did not hear report");
    const auto feet = s.actors()[1].feet;
    step(s);
    require(in_step(s.actors()[1]) && s.actors()[1].planned_closed_passages == 0 &&
                length(s.actors()[1].feet - feet) < .2,
            "report interrupted or teleported started Step");
    for (unsigned t = 0; t < 80 && in_step(s.actors()[1]); ++t)
        step(s);
    step(s, 2);
    require(s.actors()[1].support.surface == 2 && s.actors()[1].state == TraversalState::NoPath,
            "future closure was lost after safe Step completion");
}
void long_detour() {
    auto scene = course();
    scene.geometry.bodies[0].half_z = 20;
    scene.geometry.bodies[1].half_z = 20;
    scene.geometry.bodies[3].origin.z = 18;
    scene.services[0].banks[0].local.z_m = 18;
    scene.services[0].banks[1].local.z_m = 18;
    scene.connections[2].from.local.z_m = 18;
    scene.connections[2].to.local.z_m = 18;
    TraversalSession s{scene};
    loaded(s);
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
            "long detour closure failed");
    for (unsigned t = 0; t < 700 && s.actors()[1].delivery->delivered == 0; ++t)
        step(s);
    require(s.actors()[1].delivery->delivered > 0,
            "uncertainty expiry repeatedly cancelled a viable long detour");
}
void opening_report() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[1] = {{1, {1, -3}}, {}, false};
    scene.spawns[3].location = {1, {-2, -3}};
    TraversalSession s{scene};
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed &&
                s.share_knowledge(0, 1) == KnowledgeOutcome::Shared,
            "negative opening fixture report failed");
    s.set_goal(1, {2, {1, -2}});
    require(ferry(s.actors()[1]), "closed belief did not select ferry");
    step(s);
    require(s.interact_passage(0, 1, true) == PassageOutcome::Changed &&
                s.share_knowledge(0, 1) == KnowledgeOutcome::Shared,
            "new opening report failed");
    step(s);
    require(bridge(s.actors()[1]) && !ferry(s.actors()[1]) &&
                s.actors()[1].passage_beliefs[0]->source_actor == s.actors()[0].id,
            "fresh opening did not shorten an active detour before reaching bank");
}
void queue_cancel() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[1] = {{1, {1, -2}}, {}, false};
    scene.spawns[3].location = {1, {-2, -3}};
    TraversalSession s{scene};
    s.set_goal(1, {2, {1, -2}});
    step(s);
    require(!s.passages()[0].queue.empty() && !in_step(s.actors()[1]),
            "fixture missed uncommitted passage queue");
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed &&
                s.passages()[0].queue.empty() && s.passages()[0].owner.value == 0,
            "safe closure did not release uncommitted allocation");
    s.validate();
}
void repair_resume() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.passages[0].open = false;
    scene.stations[2].wear = 1000;
    TraversalSession s{scene};
    require(s.begin_maintenance(0, 2), "repair checkpoint action failed");
    step(s, 35);
    TraversalSession copy;
    copy.restore(s.serialize());
    for (unsigned t = 0; t < 50; ++t) {
        step(s);
        step(copy);
        require(s.serialize() == copy.serialize(), "repair continuation diverged");
    }
    require(s.passages()[0].open && s.stations()[2].completed == 1,
            "restored repair did not open exactly once");
}
void unlabelled_bank() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.connections[1].passage = 0;
    scene.spawns[1] = {{2, {-1, -2}}, {}, false};
    TraversalSession s{scene};
    s.set_goal(1, {1, {-1, -2}});
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
            "unlabelled fixture did not close");
    step(s, 30);
    require(s.actors()[1].passage_beliefs[0] && !s.actors()[1].passage_beliefs[0]->open &&
                ferry(s.actors()[1]),
            "bank without queue label lost physical observation channel");
}
void action_report() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[1].autonomous = false;
    scene.geometry.bodies.push_back({5, {10, 0, 0}, 0, 2, 2, .3, 0, 0, true});
    scene.connections.push_back({{2, {2.8, 0}}, {5, {-1.8, 0}}, SurfaceLegKind::Step, 2});
    scene.passages.push_back({2, 5, {}, {}, false, {}});
    TraversalSession s{scene};
    const auto quantity = s.world().resource(s.stations()[2].lot)->record.quantity.value;
    require(s.begin_maintenance(0, 2), "protected action did not start");
    step(s, 10);
    const auto action = s.actors()[0].action;
    seeded_report(s, 3, 2, false);
    require(s.share_knowledge(3, 0) == KnowledgeOutcome::Shared,
            "working actor did not receive report");
    step(s);
    require(s.actors()[0].action == action &&
                s.world().action(action)->state == ActionState::Running,
            "passage report interrupted material action");
    step(s, 69);
    require(s.world().action(action)->state == ActionState::Completed &&
                s.stations()[2].completed == 1 &&
                s.world().resource(s.stations()[2].lot)->record.quantity.value == quantity - 1 &&
                s.actors()[0].passage_beliefs[1],
            "protected action lost its single cost, completion or report");
}
void clearance_refusal() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.spawns[1] = {{1, {1, -2}}, {}, false};
    scene.spawns[2] = {{2, {-2.3, -2}}, {}, true};
    scene.spawns[2].known_stations = 0;
    scene.spawns[3].location = {1, {-2, -3}};
    TraversalSession s{scene};
    s.set_goal(1, {2, {1, -2}});
    for (unsigned t = 0; t < 250 && !s.actors()[2].yielding; ++t)
        step(s);
    require(s.actors()[2].yielding.has_value(), "fixture did not enter real clearance");
    const auto bytes = s.serialize();
    require(s.interact_passage(0, 1, false) == PassageOutcome::Occupied && s.serialize() == bytes,
            "closure mutated passage during active clearance");
    step(s, 180);
    require(s.actors()[1].state == TraversalState::Arrived && !s.actors()[2].yielding &&
                s.interact_passage(0, 1, false) == PassageOutcome::Changed,
            "safe closure did not become possible after physical clearance completion");
}
void delivery_cost() {
    std::uint64_t arrival[2]{};
    for (unsigned closed = 0; closed < 2; ++closed) {
        TraversalSession s{course()};
        loaded(s);
        if (closed)
            require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
                    "cost disturbance failed");
        bool used = false;
        for (unsigned t = 0; t < 650 && s.actors()[1].delivery->delivered == 0; ++t) {
            step(s);
            used = used || (closed ? s.actors()[1].transfer == TransferStage::Riding
                                   : s.actors()[1].support.surface == 3);
        }
        require(used && s.actors()[1].delivery->delivered > 0,
                "comparison did not use real distinct routes");
        arrival[closed] = s.world().clock().tick_count();
    }
    std::cout << "open_delivery_tick=" << arrival[0] << " closed_delivery_tick=" << arrival[1]
              << '\n';
    require(arrival[1] > arrival[0], "closed bridge did not actually delay delivery");
}
void open_observation() {
    auto scene = course();
    scene.initial_deliveries.clear();
    scene.passages[0].maintenance.reset();
    TraversalSession s{scene};
    require(!s.actors()[0].passage_beliefs[0], "fixture already observed open passage");
    step(s);
    require(s.actors()[0].passage_beliefs[0] && s.actors()[0].passage_beliefs[0]->open &&
                s.actors()[0].passage_beliefs[0]->source_actor == s.actors()[0].id,
            "nearby actor did not record open passage without a maintenance link");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected case");
        const std::string n = argv[1];
        if (n == "discovery")
            discovery();
        else if (n == "witness")
            witness();
        else if (n == "repair")
            repair();
        else if (n == "occupied")
            occupied();
        else if (n == "step_refusal")
            step_refusal();
        else if (n == "local_manual")
            local_manual();
        else if (n == "community")
            community();
        else if (n == "persistence")
            persistence();
        else if (n == "corruption")
            corruption();
        else if (n == "observation_ring")
            observation_ring();
        else if (n == "wear_and_queue")
            wear_and_queue();
        else if (n == "manual_barrier")
            manual_barrier();
        else if (n == "no_path_scout")
            no_path_scout();
        else if (n == "protected_ride")
            protected_ride();
        else if (n == "protected_step")
            protected_step();
        else if (n == "long_detour")
            long_detour();
        else if (n == "protected_boarding")
            protected_ride(TransferStage::Boarding);
        else if (n == "protected_exiting")
            protected_ride(TransferStage::Exiting);
        else if (n == "opening_report")
            opening_report();
        else if (n == "queue_cancel")
            queue_cancel();
        else if (n == "repair_resume")
            repair_resume();
        else if (n == "unlabelled_bank")
            unlabelled_bank();
        else if (n == "action_report")
            action_report();
        else if (n == "clearance_refusal")
            clearance_refusal();
        else if (n == "delivery_cost")
            delivery_cost();
        else if (n == "open_observation")
            open_observation();
        else
            throw std::invalid_argument("unknown case");
        std::cout << "PASS " << n << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
