#include "graphics/lab_scene.hpp"
#include "lab/traversal_session.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reality;
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
std::size_t actor_index(const TraversalSession& s, EntityId id) {
    const auto it = std::find_if(s.actors().begin(), s.actors().end(),
                                 [&](const auto& a) { return a.id == id; });
    require(it != s.actors().end(), "role not found by persistent ID");
    return static_cast<std::size_t>(it - s.actors().begin());
}
void balance(const TraversalSession& s) {
    std::uint64_t total = 0;
    for (const auto& slot : s.world().entities().slots())
        if (slot.occupied && slot.kind == EntityKind::Resource) {
            const auto r = s.world().resource(slot.id);
            if (r && r->record.material == MaterialTypeId{1})
                total += r->record.quantity.value;
        }
    for (const auto& command : s.world().commands())
        if (command.result.status == RequestStatus::Accepted) {
            const auto a = s.world().action(command.result.action);
            if (a && s.world().resource(a->request.lot)->record.material == MaterialTypeId{1})
                total += a->consumed.value;
        }
    require(total == 4, "material created, destroyed or lost during story");
}
void step(TraversalSession& s) {
    s.advance(1);
    s.validate();
    balance(s);
}
void finish(TraversalSession& s) {
    for (unsigned i = 0; i < 600 && s.story()->stage == StoryStage::Accepted; ++i)
        step(s);
    require(s.story()->stage == StoryStage::Completed,
            "story did not follow real delivery and meal");
    const auto& st = *s.story();
    const auto& courier = s.actors()[actor_index(s, st.executor)];
    require(courier.delivery && courier.delivery->delivered == 4,
            "text completed without four delivered units");
    const auto meal = s.world().action(st.meal);
    require(meal && meal->request.actor == st.recipient && meal->request.lot == st.destination &&
                meal->state == ActionState::Completed,
            "story did not bind recipient's real meal");
}
void normal() {
    TraversalSession s{make_story_course()};
    require(s.story() && s.story()->stage == StoryStage::Dormant, "story decided without request");
    const auto id = s.story()->executor;
    require(!s.actors()[actor_index(s, id)].delivery, "order created before acceptance");
    require(s.begin_story(actor_index(s, id)) == StoryReply::Accepted,
            "story not accepted after physical contact");
    const auto commands = s.world().commands().size();
    const auto entities = s.world().entities().next_id();
    require(s.begin_story(actor_index(s, id)) == StoryReply::Accepted &&
                s.world().commands().size() == commands &&
                s.world().entities().next_id() == entities,
            "repeated acceptance created work or cargo");
    require(!s.actors()[actor_index(s, id)].autonomous, "acceptance stole manual control");
    s.set_autonomous(actor_index(s, id), true);
    finish(s);
    const auto bytes = s.serialize();
    require(s.begin_story(actor_index(s, id)) == StoryReply::Ended && s.serialize() == bytes,
            "completed scene restarted");
}
void removed_actor() {
    TraversalSession s{make_story_course()};
    const auto roles = *s.story();
    require(s.remove_actor(roles.speaker), "real speaker removal refused");
    require(!s.world().entities().contains(roles.speaker, EntityKind::Actor) &&
                s.actors().size() == 3,
            "speaker only hidden instead of removed");
    require(s.actors()[actor_index(s, roles.recipient)].id == roles.recipient,
            "role rebound to shifted index");
    require(s.begin_story(actor_index(s, roles.executor)) == StoryReply::Ended &&
                s.story()->stage == StoryStage::MissingSpeaker,
            "missing participant demanded impossible conversation");
    for (unsigned i = 0; i < 20; ++i)
        step(s);
    require(s.story()->stage == StoryStage::MissingSpeaker &&
                !s.world().entities().contains(roles.speaker, EntityKind::Actor),
            "story resurrected missing participant");
}
void removed_site() {
    TraversalSession s{make_story_course()};
    const auto site = s.story()->site;
    const auto version = s.geometry().version;
    require(s.remove_static_body(site), "unused meeting structure removal refused");
    require(std::none_of(s.geometry().bodies.begin(), s.geometry().bodies.end(),
                         [&](const auto& b) { return b.id == site; }) &&
                s.geometry().version > version,
            "site only hidden instead of removed");
    require(s.begin_story(0) == StoryReply::Ended && s.story()->stage == StoryStage::MissingSite,
            "scene restored missing place");
    s.validate();
    balance(s);
}
void move_supply(TraversalSession& s) {
    const auto mover = s.actors().back().id;
    const auto i = actor_index(s, mover);
    s.assign_delivery(i, 0, 1, 4);
    require(s.interact_cargo(i, 0) == CargoStatus::Transferred,
            "disturbance did not physically move supply");
    s.cancel_delivery(i);
    require(s.world().resource(s.actors()[i].cargo)->record.quantity.value == 4,
            "moved supply is not in actual bag");
}
void moved_supply() {
    TraversalSession s{make_story_course()};
    move_supply(s);
    require(s.begin_story(0) == StoryReply::Ended && s.story()->stage == StoryStage::MissingSupply,
            "empty locally observed source did not change story");
    require(!s.actors()[0].delivery, "refusal invented a delivery");
    s.validate();
    balance(s);
}
void contact() {
    TraversalSession s{make_story_course()};
    require(s.remove_actor(s.story()->speaker), "setup removal failed");
    s.set_goal(0, {1, {4, 0}});
    for (unsigned n = 0; n < 100; ++n)
        step(s);
    const auto before = s.serialize();
    require(s.begin_story(0) == StoryReply::OutOfReach && s.story()->stage == StoryStage::Dormant &&
                s.serialize() == before,
            "remote request disclosed missing role");
}
void wall() {
    auto scene = make_story_course();
    scene.geometry.bodies.push_back({3, {3, 1, .7}, 0, .5, .05, 1, 0, 0, false});
    TraversalSession s{scene};
    const auto before = s.serialize();
    require(!s.can_exchange_knowledge(0, 1), "fixture wall does not block physical contact");
    require(s.begin_story(0) == StoryReply::OutOfReach && s.serialize() == before,
            "conversation crossed wall");
}
void busy() {
    TraversalSession queued{make_story_course()};
    const auto original_feet = queued.actors()[0].feet;
    queued.queue_input(0, {1, 0});
    require(queued.begin_story(0) == StoryReply::Busy &&
                queued.story()->stage == StoryStage::Dormant && !queued.actors()[0].delivery,
            "acceptance swallowed queued movement");
    for (unsigned n = 0; n < 5; ++n)
        step(queued);
    require(length(queued.actors()[0].feet - original_feet) > 1e-8,
            "busy refusal discarded queued movement");
    TraversalSession loaded{make_story_course()};
    require(loaded.interact_cargo(0, 0) == CargoStatus::Transferred, "load fixture failed");
    const auto before = loaded.serialize();
    require(loaded.begin_story(0) == StoryReply::Busy && loaded.serialize() == before,
            "acceptance took over loaded bag");
}
void cancel() {
    TraversalSession s{make_story_course()};
    require(s.begin_story(0) == StoryReply::Accepted, "acceptance failed");
    s.set_autonomous(0, true);
    for (unsigned n = 0;
         n < 100 && s.world().resource(s.actors()[0].cargo)->record.quantity.value == 0; ++n)
        step(s);
    const auto bag = s.actors()[0].cargo;
    require(s.world().resource(bag)->record.quantity.value == 4, "cancel test had no real cargo");
    s.cancel_delivery(0);
    s.validate();
    balance(s);
    require(s.story()->stage == StoryStage::Failed &&
                s.world().resource(bag)->record.quantity.value == 4,
            "cancel fabricated completion or lost bag");
}
void deadline() {
    TraversalSession s{make_story_course()};
    require(s.begin_story(0) == StoryReply::Accepted, "acceptance failed");
    for (unsigned n = 0; n < 599; ++n)
        step(s);
    require(s.story()->stage == StoryStage::Accepted, "story deadline shortened");
    step(s);
    require(s.story()->stage == StoryStage::Failed && s.story()->ended_at == 600 &&
                s.actors()[0].delivery->phase == DeliveryPhase::Cancelled,
            "story waited beyond declared horizon");
}
void sticky() {
    TraversalSession s{make_story_course()};
    move_supply(s);
    require(s.begin_story(0) == StoryReply::Ended, "refusal missing");
    require(s.interact_cargo(s.actors().size() - 1, 0) == CargoStatus::Transferred,
            "real supply return failed");
    require(s.world().resource(s.story()->source)->record.quantity.value == 4,
            "supply did not return");
    s.validate();
    const auto bytes = s.serialize();
    TraversalSession c;
    c.restore(bytes);
    require(c.story() && c.story()->stage == StoryStage::MissingSupply && c.serialize() == bytes,
            "restore re-evaluated historical refusal against returned stock");
}
void combinations() {
    for (unsigned mask = 1; mask < 8; ++mask) {
        std::cout << "mask=" << mask << "\n";
        TraversalSession s{make_story_course()};
        const auto st = *s.story();
        if (mask & 1)
            require(s.remove_actor(st.speaker), "combination actor removal failed");
        if (mask & 2)
            require(s.remove_static_body(st.site), "combination place removal failed");
        if (mask & 4)
            move_supply(s);
        require(s.begin_story(0) == StoryReply::Ended, "combination accepted impossible scene");
        require(s.story()->stage == ((mask & 1)   ? StoryStage::MissingSpeaker
                                     : (mask & 2) ? StoryStage::MissingSite
                                                  : StoryStage::MissingSupply),
                "refusal priority changed");
        s.validate();
        balance(s);
    }
}
void unsafe_removal() {
    TraversalSession s{make_story_course()};
    auto bytes = s.serialize();
    require(!s.remove_actor(s.story()->executor) && !s.remove_static_body(1) &&
                s.serialize() == bytes,
            "dependent support or first actor removed");
    s.queue_input(0, {1, 0});
    bytes = s.serialize();
    require(!s.remove_static_body(s.story()->site) && !s.remove_actor(s.story()->speaker) &&
                s.serialize() == bytes,
            "queued operation was interrupted by world edit");
    TraversalSession late{make_story_course()};
    step(late);
    bytes = late.serialize();
    require(!late.remove_actor(late.story()->speaker) &&
                !late.remove_static_body(late.story()->site) && late.serialize() == bytes,
            "world preparation edited running session");
}
void persistence() {
    TraversalSession s{make_story_course()};
    require(s.begin_story(0) == StoryReply::Accepted, "acceptance failed");
    s.set_autonomous(0, true);
    for (unsigned n = 0;
         n < 100 && s.world().resource(s.actors()[0].cargo)->record.quantity.value == 0; ++n)
        step(s);
    const auto bytes = s.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 10\n"), "story codec missing optional format 10");
    TraversalSession c;
    c.restore(bytes);
    require(c.story() && c.serialize() == bytes, "in-flight story not restored");
    finish(s);
    finish(c);
    require(s.serialize() == c.serialize(), "continued world diverged");
}
std::string rechecksum(std::string text) {
    text.resize(text.rfind("CHECKSUM "));
    return text + "CHECKSUM " + std::to_string(snapshot_checksum(text)) + "\n";
}
void corruption() {
    TraversalSession s{make_story_course()};
    require(s.begin_story(0) == StoryReply::Accepted, "acceptance failed");
    const auto bytes = s.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 10\n"), "story codec missing optional format 10");
    const auto reject = [&](std::string bad) {
        bool failed = false;
        try {
            s.restore(rechecksum(bad));
        } catch (const std::exception&) {
            failed = true;
        }
        require(failed && s.serialize() == bytes,
                "corruption committed partial or fictional story");
    };
    auto bad = bytes;
    const auto p = bad.find("STORY_STATE ");
    const auto end = bad.find('\n', p);
    bad.replace(p, end - p,
                "STORY_STATE 2 0 0 " + std::to_string(s.story()->begin_event) + " " +
                    std::to_string(s.world().events().back().sequence) + " " +
                    std::to_string(s.story()->order.value) + " " +
                    std::to_string(s.story()->cargo.value) + " 0 " +
                    std::to_string(s.story()->order.value + 1));
    reject(bad);
    bad = bytes;
    bad.replace(p, 12, "STORY_STATE 99");
    reject(bad);
    bad = bytes;
    const auto bindings = bad.find("STORY ");
    const auto be = bad.find('\n', bindings);
    bad.replace(bindings, be - bindings, "STORY 1 2 3 5 6 999 1 0 0 4");
    reject(bad);
    TraversalSession gone{make_story_course()};
    require(gone.remove_static_body(gone.story()->site), "site corruption fixture failed");
    require(gone.begin_story(0) == StoryReply::Ended, "site corruption ending failed");
    const auto gone_bytes = gone.serialize();
    auto forged = gone_bytes;
    const auto gp = forged.find("STORY ");
    forged.replace(gp, forged.find('\n', gp) - gp, "STORY 1 2 3 5 6 999 1 0 0 4");
    bool rejected = false;
    try {
        gone.restore(rechecksum(forged));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && gone.serialize() == gone_bytes,
            "missing-site ID lacks original binding context");
    bad = bytes;
    const auto o = bad.find("STORY_CONTACT ");
    bad.replace(o, bad.find('\n', o) - o, "STORY_CONTACT 1 0 0 0 1 0 0 0 0 0");
    reject(bad);
}
void legacy() {
    TraversalSession distant{make_distant_course()};
    const auto bytes = distant.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 9\n"), "fixture is not legacy 9");
    TraversalSession s{make_story_course()};
    s.restore(bytes);
    require(!s.story() && s.serialize() == bytes, "foreign old world kept story or changed bytes");
    auto scene = make_story_course();
    scene.stations.push_back({{1, {-2, 0}}, TraversalTask::Maintain, 4});
    scene.stations.back().initial_material = {2};
    scene.practice = PracticeSeed{};
    scene.practice->station = 4;
    scene.practice->researchers = {0};
    TraversalSession practice{scene};
    const auto state = practice.serialize();
    s.restore(state);
    require(s.story() && s.has_practice() && s.serialize() == state,
            "format10 lost optional practice state");
}

void support_contact() {
    auto scene = make_story_course();
    scene.geometry.bodies.push_back({3, {3, 0, 0}, 0, 1, 1, .3, 0, 0, true});
    scene.story->meeting = {3, {0, 0}};
    TraversalSession s{scene};
    const auto before = s.serialize();
    require(s.begin_story(0) == StoryReply::OutOfReach && s.serialize() == before,
            "meeting accepted on a different support");
}
void source_consumption() {
    auto scene = make_story_course();
    scene.spawns[1].autonomous = true;
    scene.spawns[1].hunger = 700;
    scene.spawns[1].location = {1, {1, .8}};
    TraversalSession s{scene};
    for (unsigned n = 0; n < 60 && !s.actors()[1].action.value; ++n)
        step(s);
    require(s.actors()[1].action.value &&
                s.world().resource(s.story()->source)->record.quantity.value < 4,
            "fixture never consumed actual source material");
    require(s.begin_story(0) == StoryReply::Ended && s.story()->stage == StoryStage::MissingSupply,
            "consumed material counted as free supply");
    const auto bytes = s.serialize();
    TraversalSession restored;
    restored.restore(bytes);
    for (unsigned n = 0; n < 100; ++n) {
        step(s);
        step(restored);
    }
    require(s.serialize() == restored.serialize() && s.story()->stage == StoryStage::MissingSupply,
            "later work changed historical free quantity proof");
}
void deadline_step() {
    auto scene = make_story_course();
    scene.spawns[2].autonomous = false;
    scene.geometry.bodies.push_back({3, {6, .25, 0}, 0, 1, 1, .25, 0, 0, true});
    scene.stations[1].location = {3, {.4, 0}};
    scene.spawns[2].location = {3, {.3, .7}};
    scene.connections.push_back({{1, {1.7, 0}}, {3, {-.7, 0}}, SurfaceLegKind::Step});
    const auto start_walk = [](TraversalSession& s) {
        require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "step fixture load failed");
        s.set_goal(0, {3, {.4, 0}});
    };
    TraversalSession reference{scene};
    require(reference.begin_story(0) == StoryReply::Accepted, "step fixture acceptance failed");
    start_walk(reference);
    unsigned ticks = 0;
    while (ticks < 100 && !reference.actors()[0].step_point) {
        step(reference);
        ++ticks;
    }
    require(reference.actors()[0].step_point > 0 && ticks < 100, "fixture never reached real Step");
    TraversalSession s{scene};
    require(s.begin_story(0) == StoryReply::Accepted, "step acceptance failed");
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "deadline real cargo missing");
    for (unsigned n = 0; n < 600 - ticks; ++n)
        step(s);
    s.set_goal(0, {3, {.4, 0}});
    for (unsigned n = 0; n < ticks; ++n)
        step(s);
    require(s.story()->stage == StoryStage::Failed && s.actors()[0].step_point > 0 &&
                s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4,
            "deadline interrupted Step or teleported cargo");
    for (unsigned n = 0; n < 100 && s.actors()[0].support.surface != 3; ++n)
        step(s);
    require(s.actors()[0].support.surface == 3, "failed story stopped committed physical Step");
}

void order_binding() {
    auto scene = make_story_course();
    scene.spawns[2].autonomous = false;
    TraversalSession s{scene};
    require(s.begin_story(0) == StoryReply::Accepted, "order fixture acceptance failed");
    s.set_autonomous(0, true);
    for (unsigned n = 0; n < 400 && s.actors()[0].delivery->phase != DeliveryPhase::Completed; ++n)
        step(s);
    require(s.story()->stage == StoryStage::Accepted &&
                s.actors()[0].delivery->phase == DeliveryPhase::Completed,
            "fixture did not finish delivery before recipient meal");
    const auto before = s.serialize();
    bool refused = false;
    try {
        s.assign_delivery(0, 0, 1, 1);
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    require(refused && s.serialize() == before,
            "new order replaced accepted story before its meal");
}
void cancelled_corruption() {
    TraversalSession s{make_story_course()};
    require(s.begin_story(0) == StoryReply::Accepted, "cancel corruption acceptance failed");
    s.cancel_delivery(0);
    const auto before = s.serialize();
    auto bad = before;
    const auto at = bad.find("STORY_STATE ");
    bad.replace(at, bad.find('\n', at) - at,
                "STORY_STATE 1 0 0 " + std::to_string(s.story()->begin_event) + " 0 " +
                    std::to_string(s.story()->order.value) + " " +
                    std::to_string(s.story()->cargo.value) + " 0 0");
    bool rejected = false;
    try {
        s.restore(rechecksum(bad));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before, "cancelled delivery restored as accepted story");
}

void resource_binding() {
    for (unsigned kind = 0; kind < 2; ++kind) {
        auto scene = make_story_course();
        if (kind == 0) {
            scene.stations[0].initial_material = {2};
            scene.stations[1].initial_material = {2};
        } else
            scene.stations[1].kind = TraversalTask::Maintain;
        bool refused = false;
        try {
            TraversalSession invalid{scene};
        } catch (const std::invalid_argument&) {
            refused = true;
        }
        require(refused, "supply story allowed non-food or non-meal destination");
    }
}
void observation_time() {
    auto scene = make_story_course();
    scene.spawns[1].autonomous = true;
    scene.spawns[1].hunger = 700;
    scene.spawns[1].location = {1, {1, .8}};
    TraversalSession s{scene};
    for (unsigned n = 0; n < 60 && !s.actors()[1].action.value; ++n)
        step(s);
    require(s.world().clock().tick_count() > 0 && s.begin_story(0) == StoryReply::Ended &&
                s.story()->stage == StoryStage::MissingSupply,
            "chronology fixture failed");
    const auto before = s.serialize();
    auto bad = before;
    const auto at = bad.find("STORY_STATE ");
    bad.replace(at, bad.find('\n', at) - at,
                "STORY_STATE 5 0 0 " + std::to_string(s.story()->begin_event) + " " +
                    std::to_string(s.story()->end_event) + " 0 0 0 0");
    bool refused = false;
    try {
        s.restore(rechecksum(bad));
    } catch (const std::exception&) {
        refused = true;
    }
    require(refused && s.serialize() == before,
            "future consumption justified an earlier source observation");
}

void static_bindings() {
    for (const auto id : {1U, 2U}) {
        auto scene = make_story_course();
        if (id == 2)
            scene.geometry.bodies[1].walkable = true;
        scene.platforms.push_back({{id, {1, 0, 0}, 0, 60, 10, 0}, 0, false});
        scene.platforms[0].definition.validate(scene.geometry);
        bool refused = false;
        try {
            TraversalSession invalid{scene};
        } catch (const std::invalid_argument&) {
            refused = true;
        }
        require(refused, "static story binding accepted a moving support or site");
    }
}

void false_failure() {
    TraversalSession s{make_story_course()};
    require(s.begin_story(0) == StoryReply::Accepted, "failure corruption fixture failed");
    const auto before = s.serialize();
    auto bad = before;
    const auto at = bad.find("STORY_STATE ");
    bad.replace(at, bad.find('\n', at) - at,
                "STORY_STATE 6 0 0 " + std::to_string(s.story()->begin_event) + " " +
                    std::to_string(s.world().events().back().sequence) + " " +
                    std::to_string(s.story()->order.value) + " " +
                    std::to_string(s.story()->cargo.value) + " 0 " +
                    std::to_string(s.story()->order.value + 1));
    bool rejected = false;
    try {
        s.restore(rechecksum(bad));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before,
            "failed story accepted while original delivery remains active");
}
void terminal_order() {
    TraversalSession s{make_story_course()};
    require(s.begin_story(0) == StoryReply::Accepted, "terminal order fixture failed");
    s.cancel_delivery(0);
    require(s.story()->stage == StoryStage::Failed, "original order did not fail");
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "later manual pickup failed");
    s.set_goal(0, s.stations()[1].location);
    for (unsigned n = 0; n < 100; ++n)
        step(s);
    require(s.interact_cargo(0, 1) == CargoStatus::Transferred, "later manual drop failed");
    s.set_goal(0, {1, {4, 0}}); // Leave the recipient physical room to approach.
    for (unsigned n = 0; n < 300 && !s.stations()[1].completed; ++n)
        step(s);
    if (!s.stations()[1].completed)
        std::cerr << s.inspect();
    require(s.stations()[1].completed && s.story()->stage == StoryStage::Failed,
            "later real meal changed failed story");
    EntityId meal;
    for (const auto& c : s.world().commands())
        if (c.result.status == RequestStatus::Accepted) {
            const auto a = s.world().action(c.result.action);
            if (a && a->request.actor == s.story()->recipient &&
                a->request.lot == s.story()->destination && a->state == ActionState::Completed)
                meal = a->id;
        }
    require(meal.value != 0, "fixture did not feed actual recipient");
    const auto before = s.serialize();
    s.restore(before);
    require(s.serialize() == before, "valid failed story rejected after manual transport");
    auto bad = before;
    const auto at = bad.find("STORY_STATE ");
    std::uint64_t forged_end = 0;
    for (const auto& t : s.world().transfers())
        if (t.request.source == s.story()->cargo && t.request.destination == s.story()->destination)
            forged_end = t.request.command.value + 1;
    require(forged_end > s.story()->order_end.value,
            "forged interval did not include later transport");
    bad.replace(at, bad.find('\n', at) - at,
                "STORY_STATE 2 0 " + std::to_string(s.world().clock().tick_count()) + " " +
                    std::to_string(s.story()->begin_event) + " " +
                    std::to_string(s.world().events().back().sequence) + " " +
                    std::to_string(s.story()->order.value) + " " +
                    std::to_string(s.story()->cargo.value) + " " + std::to_string(meal.value) +
                    " " + std::to_string(forged_end));
    bool rejected = false;
    try {
        s.restore(rechecksum(bad));
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before,
            "expanded interval completed a previously cancelled story");
}
void manual() {
    TraversalSession s{make_story_course()};
    require(s.begin_story(0) == StoryReply::Accepted, "manual story admission failed");
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred, "manual pickup failed");
    s.set_goal(0, s.stations()[1].location);
    for (unsigned n = 0; n < 100; ++n)
        step(s);
    require(s.interact_cargo(0, 1) == CargoStatus::Transferred, "manual drop failed");
    s.set_goal(0, {1, {4, 0}});
    for (unsigned n = 0; n < 300 && s.story()->stage == StoryStage::Accepted; ++n)
        step(s);
    require(s.story()->stage == StoryStage::Completed && !s.actors()[0].autonomous &&
                s.actors()[0].delivery->phase == DeliveryPhase::Returning,
            "manual story required autonomous return");
    const auto bytes = s.serialize();
    s.restore(bytes);
    require(s.serialize() == bytes, "manual completion did not roundtrip");
    const auto physical = serialize_world(s.world());
    s.cancel_delivery(0);
    s.validate();
    require(s.story()->stage == StoryStage::Completed &&
                s.actors()[0].delivery->phase == DeliveryPhase::Completed &&
                serialize_world(s.world()) == physical,
            "cancelling completed return changed success or material");
    const auto closed = s.serialize();
    s.restore(closed);
    require(s.serialize() == closed, "closed successful return did not roundtrip");
}
void terminal_rebind() {
    for (bool success : {false, true}) {
        TraversalSession s{make_story_course()};
        require(s.begin_story(0) == StoryReply::Accepted, "binding fixture failed");
        if (success) {
            s.set_autonomous(0, true);
            finish(s);
            for (unsigned n = 0;
                 n < 200 && s.actors()[0].delivery->phase != DeliveryPhase::Completed; ++n)
                step(s);
            require(s.actors()[0].delivery->phase == DeliveryPhase::Completed,
                    "completed order fixture still busy");
        } else
            s.cancel_delivery(0);
        const auto before = s.serialize();
        bool rejected = false;
        try {
            s.assign_delivery(0, 0, 1, 1);
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected && s.serialize() == before, "terminal story delivery record was replaced");
    }
}

void prepare(TraversalSession& s, const std::string& variant) {
    if (variant == "speaker")
        require(s.remove_actor(s.story()->speaker), "checkpoint removal failed");
    else if (variant == "site")
        require(s.remove_static_body(s.story()->site), "checkpoint site removal failed");
    else if (variant == "supply" || variant == "returned")
        move_supply(s);
    else
        require(variant == "travel" || variant == "complete" || variant == "failed",
                "unknown checkpoint variant");
}
void continue_case(TraversalSession& s, const std::string& variant) {
    if (variant == "travel")
        finish(s);
    else if (variant == "speaker" || variant == "site" || variant == "supply") {
        require(s.story()->stage == StoryStage::Dormant && s.begin_story(0) == StoryReply::Ended,
                "pre-start mutation lost on continuation");
        for (unsigned n = 0; n < 20; ++n)
            step(s);
    } else {
        const auto stage = s.story()->stage;
        for (unsigned n = 0; n < 20; ++n)
            step(s);
        require(s.story()->stage == stage, "terminal story changed after continuation");
    }
}
void checkpoint(const std::string& variant, const std::filesystem::path& path,
                const std::filesystem::path& expected) {
    TraversalSession s{make_story_course()};
    prepare(s, variant);
    if (variant == "travel" || variant == "complete") {
        require(s.begin_story(0) == StoryReply::Accepted, "checkpoint acceptance failed");
        s.set_autonomous(0, true);
        if (variant == "complete")
            finish(s);
        else {
            for (unsigned n = 0;
                 n < 100 && (s.world().resource(s.actors()[0].cargo)->record.quantity.value != 4 ||
                             s.actors()[0].motion.speed <= .1);
                 ++n)
                step(s);
            require(s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4 &&
                        s.actors()[0].motion.speed > .1,
                    "checkpoint is not real loaded motion");
        }
    } else if (variant == "returned") {
        require(s.begin_story(0) == StoryReply::Ended &&
                    s.interact_cargo(s.actors().size() - 1, 0) == CargoStatus::Transferred,
                "returned-refusal fixture failed");
    } else if (variant == "failed") {
        require(s.begin_story(0) == StoryReply::Accepted, "deadline fixture failed");
        for (unsigned n = 0; n < 600; ++n)
            step(s);
    }
    s.save(path);
    continue_case(s, variant);
    s.save(expected);
}

} // namespace
int main(int argc, char** argv) {
    try {
        const std::string name = argc > 1 ? argv[1] : "normal";
        if (name == "checkpoint" && argc == 5) {
            checkpoint(argv[2], argv[3], argv[4]);
            std::cout << "PASS checkpoint\n";
            return 0;
        }
        if (name == "continue" && argc == 5) {
            TraversalSession resumed;
            resumed.load(argv[3]);
            continue_case(resumed, argv[2]);
            resumed.save(argv[4]);
            std::cout << "PASS continue\n";
            return 0;
        }
        if (name == "normal")
            normal();
        else if (name == "removed_actor")
            removed_actor();
        else if (name == "removed_site")
            removed_site();
        else if (name == "moved_supply")
            moved_supply();
        else if (name == "false_failure")
            false_failure();
        else if (name == "manual")
            manual();
        else if (name == "terminal_order")
            terminal_order();
        else if (name == "terminal_rebind")
            terminal_rebind();
        else if (name == "static_bindings")
            static_bindings();
        else if (name == "resource_binding")
            resource_binding();
        else if (name == "observation_time")
            observation_time();
        else if (name == "order_binding")
            order_binding();
        else if (name == "cancelled_corruption")
            cancelled_corruption();
        else if (name == "support_contact")
            support_contact();
        else if (name == "source_consumption")
            source_consumption();
        else if (name == "deadline_step")
            deadline_step();
        else if (name == "contact")
            contact();
        else if (name == "wall")
            wall();
        else if (name == "busy")
            busy();
        else if (name == "cancel")
            cancel();
        else if (name == "deadline")
            deadline();
        else if (name == "sticky")
            sticky();
        else if (name == "combinations")
            combinations();
        else if (name == "unsafe_removal")
            unsafe_removal();
        else if (name == "persistence")
            persistence();
        else if (name == "corruption")
            corruption();
        else if (name == "legacy")
            legacy();
        else
            throw std::runtime_error("unknown story case");
        std::cout << "PASS " << name << '\n';
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
