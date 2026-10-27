#include "lab/traversal_session.hpp"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#ifndef REALITY_DELIVERY_CHECKPOINT
#define REALITY_DELIVERY_CHECKPOINT "delivery-actual-checkpoint.save"
#endif
using namespace reality;
namespace {
void require(bool c, const char* m) {
    if (!c)
        throw std::runtime_error(m);
}
// The 608-tick actual-delivery scenario is split at a saved checkpoint. Each
// process stays below the host's sustained-power knee while the total number of
// simulated ticks and assertions remains unchanged.
constexpr unsigned delivery_checkpoint_tick = 300;
void delivery_outbound() {
    TraversalSession s{make_delivery_course()};
    require(s.world().resource(s.stations()[1].lot)->record.quantity.value == 0,
            "recipient is not initially empty");
    bool loaded = false;
    for (unsigned t = 0; t < delivery_checkpoint_tick; ++t) {
        s.advance(1);
        s.validate();
        const auto& a = s.actors()[1];
        if (a.cargo.value != 0 && s.world().resource(a.cargo)->record.quantity.value > 0)
            loaded = true;
    }
    require(loaded, "courier did not physically load before the delivery checkpoint");
    s.save(REALITY_DELIVERY_CHECKPOINT);
}
void delivery_completion() {
    TraversalSession s{make_delivery_course()};
    s.load(REALITY_DELIVERY_CHECKPOINT);
    std::error_code ignored;
    std::filesystem::remove(REALITY_DELIVERY_CHECKPOINT, ignored);
    bool delivered = false;
    for (unsigned t = delivery_checkpoint_tick; t < 900 && !delivered; ++t) {
        s.advance(1);
        s.validate();
        const auto& a = s.actors()[1];
        delivered = a.delivery && a.delivery->delivered > 0;
    }
    if (!delivered)
        std::cerr << s.inspect();
    require(delivered, "courier did not physically deliver actual material after the checkpoint");
}
// Manual profiling case, intentionally outside CTest's per-process time limit.
void delivery_single() {
    TraversalSession s{make_delivery_course()};
    require(s.world().resource(s.stations()[1].lot)->record.quantity.value == 0,
            "recipient is not initially empty");
    bool loaded = false, delivered = false;
    for (unsigned t = 0; t < 900 && !delivered; ++t) {
        s.advance(1);
        s.validate();
        const auto& a = s.actors()[1];
        if (a.cargo.value != 0 && s.world().resource(a.cargo)->record.quantity.value > 0)
            loaded = true;
        delivered = a.delivery && a.delivery->delivered > 0;
    }
    if (!delivered)
        std::cerr << s.inspect();
    require(loaded && delivered, "courier did not physically load and deliver actual material");
}
void step(TraversalSession& s, unsigned ticks) {
    for (unsigned i = 0; i < ticks; ++i) {
        s.advance(1);
        s.validate();
    }
}
TraversalScene small(std::uint64_t quantity = 4) {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {}, 0, 30, 4, .3, 0, 0, true}};
    scene.spawns = {{{1, {-3, 0}}, {}, true}, {{1, {3, 0}}, {}, false}};
    scene.stations = {{{1, {-3, 0}}, TraversalTask::Eat, 20},
                      {{1, {3, 0}}, TraversalTask::Eat, 0},
                      {{1, {-3, 2}}, TraversalTask::Rest, 1}};
    scene.stations[0].initial_material = {1};
    scene.stations[1].initial_material = {1};
    scene.initial_deliveries = {{0, 0, 1, quantity}};
    return scene;
}
void load(TraversalSession& s) {
    for (unsigned i = 0;
         i < 100 && s.world().resource(s.actors()[0].cargo)->record.quantity.value == 0; ++i)
        step(s, 1);
    require(s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4,
            "fixture failed to load cargo");
}
void balance(const TraversalSession& s) {
    auto total = s.world().resource(s.stations()[0].lot)->record.quantity.value +
                 s.world().resource(s.stations()[1].lot)->record.quantity.value +
                 s.world().resource(s.actors()[0].cargo)->record.quantity.value;
    for (const auto& record : s.world().commands())
        if (record.result.status == RequestStatus::Accepted) {
            const auto action = s.world().action(record.result.action);
            if (action->request.lot == s.stations()[0].lot ||
                action->request.lot == s.stations()[1].lot)
                total += action->consumed.value;
        }
    require(total == 20, "source, cargo, destination and real consumption do not balance");
}
TraversalScene returning_scene(double destination_x) {
    auto scene = small();
    scene.stations[1].location.local.x_m = destination_x;
    scene.spawns[1].location.local.x_m = destination_x;
    return scene;
}
TraversalScene detour_scene() {
    auto scene = small();
    scene.geometry.bodies.push_back({2, {0, 1, 0}, 0, .2, 2, 2, 0, 0, false});
    scene.stations[0].location = {1, {-6, 0}};
    scene.spawns[0].location = {1, {-6, 0}};
    scene.stations[1].location = {1, {6, 0}};
    scene.spawns[1].location = {1, {6, 0}};
    return scene;
}
void completed() {
    TraversalSession s{small(8)};
    for (unsigned i = 0; i < 1000 && s.actors()[0].delivery->phase != DeliveryPhase::Completed;
         ++i) {
        step(s, 1);
        balance(s);
    }
    require(s.actors()[0].delivery->phase == DeliveryPhase::Completed &&
                s.actors()[0].delivery->delivered == 8,
            "batched delivery did not finish its return trip");
    const auto source =
        surface_feet(s.geometry(), s.stations()[0].location, TraversalLimits{s.actors()[0].size});
    require(source && length(s.actors()[0].feet - *source) <= 1.1,
            "completed courier never returned to source");
}
std::string field(std::string bytes, std::string_view prefix, std::size_t n, std::string value) {
    bytes.resize(bytes.rfind("CHECKSUM "));
    const auto start = bytes.find(prefix, bytes.find("\nTUNING ")) + 1,
               last = bytes.find('\n', start);
    require(start != 0, "missing outer record");
    std::istringstream in{bytes.substr(start, last - start)};
    std::ostringstream out;
    std::string word;
    std::size_t i = 0;
    while (in >> word) {
        if (i)
            out << ' ';
        out << (i++ == n ? value : word);
    }
    require(i > n, "missing outer field");
    bytes.replace(start, last - start, out.str());
    return bytes + "CHECKSUM " + std::to_string(snapshot_checksum(bytes)) + '\n';
}
void local_and_cancel() {
    TraversalSession s{small()};
    load(s);
    const auto before = s.serialize();
    require(s.interact_cargo(0, 1) == CargoStatus::OutOfReach && s.serialize() == before,
            "remote cargo deposit accepted");
    const auto cargo = s.actors()[0].cargo;
    // Manual control keeps the cancelled material in the bag; autonomous return
    // is a separate contract verified by return_cancelled.
    s.set_autonomous(0, false);
    s.cancel_delivery(0);
    step(s, 100);
    require(s.actors()[0].delivery->phase == DeliveryPhase::Cancelled &&
                s.world().resource(cargo)->record.quantity.value == 4 &&
                s.world().resource(s.stations()[1].lot)->record.quantity.value == 0,
            "cancellation lost, returned or delivered cargo remotely");
    balance(s);
}
void persistence() {
    TraversalSession s{small(8)};
    load(s);
    const auto bytes = s.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 5\n"),
            "observed cargo saved without knowledge format");
    auto clone = s;
    clone.restore(bytes);
    step(s, 450);
    step(clone, 450);
    require(s.serialize() == clone.serialize(), "loaded cargo delivery diverged");
    require(s.actors()[0].delivery->delivered == 8, "restored courier never completed delivery");
    balance(s);
}
void needs() {
    auto scene = small();
    scene.stations[1].location.local.x_m = 15;
    scene.spawns[1].location.local.x_m = 15;
    TraversalSession s{scene};
    load(s);
    step(s, 1);
    require(s.actors()[0].task == TraversalTask::Deliver, "fixture is not actively delivering");
    s.restore(field(s.serialize(), "\nACTOR ", 19, "749"));
    step(s, 1);
    require(s.actors()[0].task == TraversalTask::Eat &&
                s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4,
            "critical need did not safely interrupt delivery with cargo intact");
    for (unsigned i = 0; i < 700 && s.actors()[0].delivery->delivered == 0; ++i)
        step(s, 1);
    require(s.actors()[0].delivery->delivered == 4, "delivery failed to resume after eating");
    balance(s);
}
void manual() {
    TraversalSession s{small()};
    load(s);
    s.set_autonomous(0, false);
    s.approach(0, s.stations()[1].location, .9);
    step(s, 200);
    require(s.interact_cargo(0, 1) == CargoStatus::Transferred,
            "manual actor could not deposit its physical cargo");
    require(s.actors()[0].delivery->delivered == 4 &&
                s.world().resource(s.actors()[0].cargo)->record.quantity.value == 0,
            "manual deposit did not use the same domain transfer");
    s.approach(0, s.stations()[0].location, .9);
    step(s, 200);
    require(s.interact_cargo(0, 0) == CargoStatus::Returned &&
                s.actors()[0].delivery->phase == DeliveryPhase::Completed,
            "manual courier cannot acknowledge its physical return");
    balance(s);
}
void no_recycling() {
    TraversalSession s{small(8)};
    load(s);
    s.set_autonomous(0, false);
    s.approach(0, s.stations()[1].location, .9);
    step(s, 200);
    require(s.interact_cargo(0, 1) == CargoStatus::Transferred, "fixture did not deposit");
    const auto before = s.serialize();
    require(s.interact_cargo(0, 1) == CargoStatus::Denied && s.serialize() == before,
            "courier can recycle destination stock as newly sourced delivery");
}
void wrong_station() {
    auto scene = small();
    scene.stations.push_back({{1, {-3, 0}}, TraversalTask::Eat, 10});
    scene.stations.back().initial_material = {1};
    TraversalSession s{scene};
    const auto before = serialize_world(s.world());
    require(s.interact_cargo(0, 3) == CargoStatus::Denied && serialize_world(s.world()) == before &&
                s.actors()[0].cargo.value != 0 &&
                s.world().resource(s.actors()[0].cargo)->record.quantity.value == 0 &&
                s.actors()[0].delivery->phase == DeliveryPhase::ToPickup &&
                s.actors()[0].beliefs[3] && s.actors()[0].beliefs[3]->available == 10 &&
                s.actors()[0].beliefs[3]->source_actor == s.actors()[0].id,
            "active order loaded from an unassigned origin");
    load(s);
    require(s.interact_cargo(0, 3) == CargoStatus::Denied,
            "active order dumped cargo at another station");
    s.cancel_delivery(0);
    require(s.interact_cargo(0, 3) == CargoStatus::Transferred,
            "cancelled order imprisoned real inventory");
}
void corruption() {
    TraversalSession s{small()};
    load(s);
    const auto before = s.serialize();
    for (const auto& bad : std::vector<std::string>{
             field(before, "\nCARGO ", 1, "0"),
             field(before, "\nCARGO ", 1, std::to_string(s.stations()[1].lot.value)),
             field(before, "\nDELIVERY 1", 3, "0"), field(before, "\nDELIVERY 1", 5, "1"),
             field(before, "\nDELIVERY 1", 6, "3"), field(before, "\nDELIVERY 1", 7, "999")}) {
        bool rejected = false;
        try {
            s.restore(bad);
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected && s.serialize() == before, "corrupt delivery replaced current world");
    }
}
void meals() {
    auto scene = small(8);
    scene.spawns[1].autonomous = true;
    scene.spawns[1].hunger = 300;
    TraversalSession s{scene};
    // This resident knows the local recipient's food station, not the remote depot.
    s.restore(field(s.serialize(), "\nACTOR 2 ", 18, "2"));
    bool delivered = false;
    for (unsigned i = 0; i < 500 && s.stations()[1].completed == 0; ++i) {
        step(s, 1);
        balance(s);
        if (!delivered) {
            require(s.stations()[1].completed == 0,
                    "recipient ate from a delivery that did not arrive");
            delivered = s.actors()[0].delivery->delivered > 0;
        }
    }
    require(delivered && s.stations()[1].completed > 0,
            "actual delivery did not enable a real local meal");
}
TraversalScene crossing() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-3, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {2, {3, 0, 0}, 0, 2, 2, .3, 0, 0, true},
                             {3, {}, 0, 1.2, .35, .3, 0, 0, true}};
    scene.connections = {{{1, {1.8, 0}}, {3, {-1, 0}}}, {{3, {1, 0}}, {2, {-1.8, 0}}}};
    scene.spawns = {{{1, {-1, 0}}, {}, true}, {{2, {1, 0}}, {}, false}, {{3, {0, 0}}, {}, false}};
    scene.stations = {{{1, {-1, 0}}, TraversalTask::Eat, 20},
                      {{2, {1, 0}}, TraversalTask::Eat, 0},
                      {{1, {-1, 1}}, TraversalTask::Rest, 1}};
    scene.stations[0].initial_material = {1};
    scene.stations[1].initial_material = {1};
    scene.initial_deliveries = {{0, 0, 1, 4}};
    return scene;
}
void blocked() {
    TraversalSession s{crossing()};
    load(s);
    const auto feet = s.actors()[2].feet;
    step(s, 300);
    require(s.actors()[0].delivery->delivered == 0 &&
                s.world().resource(s.stations()[1].lot)->record.quantity.value == 0 &&
                s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4 &&
                length(s.actors()[2].feet - feet) < 1e-9,
            "blocked route delivered remotely, lost cargo or forced the blocker");
    s.set_goal(2, {2, {-1, 0}});
    for (unsigned i = 0; i < 600 && s.actors()[0].delivery->delivered == 0; ++i)
        step(s, 1);
    require(s.actors()[0].delivery->delivered == 4,
            "delivery did not recover after real obstruction departure");
    balance(s);
}
void cancel_step() {
    TraversalSession s{crossing()};
    load(s);
    for (unsigned i = 0; i < 150 && s.actors()[0].step_point == 0; ++i)
        step(s, 1);
    require(s.actors()[0].step_point > 0, "cancellation fixture missed committed Step");
    // Manual control completes the committed Step without an autonomous return.
    s.set_autonomous(0, false);
    s.cancel_delivery(0);
    s.validate();
    const auto bytes = s.serialize();
    auto clone = s;
    clone.restore(bytes);
    step(s, 80);
    step(clone, 80);
    require(s.serialize() == clone.serialize() &&
                s.actors()[0].delivery->phase == DeliveryPhase::Cancelled &&
                s.world().resource(s.actors()[0].cargo)->record.quantity.value == 4 &&
                s.actors()[0].delivery->delivered == 0,
            "committed cancellation lost cargo or changed after load");
}
void return_cancelled() {
    auto scene = returning_scene(15);
    // A nearer compatible station must not replace the cancelled order's source.
    scene.stations.push_back({{1, {0, 0}}, TraversalTask::Eat, 10});
    scene.stations.back().initial_material = {1};
    TraversalSession s{scene};
    load(s);
    for (unsigned i = 0;
         i < 120 && !(s.actors()[0].delivery->phase == DeliveryPhase::ToDestination &&
                      s.actors()[0].feet.x > -.4);
         ++i)
        step(s, 1);
    require(s.actors()[0].delivery &&
                s.actors()[0].delivery->phase == DeliveryPhase::ToDestination &&
                s.actors()[0].feet.x > -.4,
            "return fixture did not start the outbound leg");
    s.cancel_delivery(0);
    const auto cargo = s.actors()[0].cargo;
    require(cargo.value != 0 && s.world().resource(cargo)->record.quantity.value == 4,
            "cancellation lost the physical batch");
    step(s, 2);
    require(s.world().resource(cargo)->record.quantity.value == 4 &&
                s.world().resource(s.stations()[3].lot)->record.quantity.value == 10,
            "autonomous return preferred a nearer station over the cancelled source");
    // Manual takeover stops the return but cannot move the physical batch.
    s.set_autonomous(0, false);
    step(s, 20);
    require(s.world().resource(cargo)->record.quantity.value == 4 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 16 &&
                s.world().resource(s.stations()[3].lot)->record.quantity.value == 10,
            "manual takeover teleported or deposited the carried batch");
    s.set_autonomous(0, true);
    for (unsigned i = 0; i < 400 && s.world().resource(cargo)->record.quantity.value > 0; ++i)
        step(s, 1);
    require(s.world().resource(cargo)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 20 &&
                s.world().resource(s.stations()[1].lot)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[3].lot)->record.quantity.value == 10 &&
                s.actors()[0].delivery->phase == DeliveryPhase::Cancelled,
            "autonomous return did not deposit at the cancelled source");
    s.assign_delivery(0, 0, 1, 4);
    require(s.actors()[0].delivery->phase == DeliveryPhase::ToPickup,
            "new order was not accepted after the bag was emptied");
    for (unsigned i = 0; i < 60 && s.world().resource(cargo)->record.quantity.value == 0; ++i)
        step(s, 1);
    require(s.world().resource(cargo)->record.quantity.value == 4,
            "new order did not physically load after the return");
    for (unsigned i = 0; i < 400 && s.actors()[0].delivery->delivered == 0; ++i)
        step(s, 1);
    require(s.actors()[0].delivery->delivered == 4, "new order never reached the recipient");
    balance(s);
}
void return_interaction() {
    TraversalSession s{small()};
    load(s);
    s.cancel_delivery(0);
    for (unsigned i = 0; i < 40 && !(s.actors()[0].task == TraversalTask::Deliver &&
                                     s.actors()[0].state == TraversalState::Arrived &&
                                     s.actors()[0].motion.speed <= .03);
         ++i)
        step(s, 1);
    require(s.actors()[0].task == TraversalTask::Deliver &&
                s.actors()[0].state == TraversalState::Arrived,
            "interaction fixture did not reach a pending return");
    const auto cargo = s.actors()[0].cargo;
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred,
            "direct interaction did not deposit the returning batch");
    require(s.actors()[0].task == TraversalTask::None &&
                s.world().resource(cargo)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 20,
            "direct interaction left an invalid return task or lost material");
    s.validate();
    static_cast<void>(s.serialize());
    balance(s);
}
void return_fallback() {
    auto scene = returning_scene(15);
    scene.stations.push_back({{1, {0, 0}}, TraversalTask::Eat, 10});
    scene.stations.back().initial_material = {1};
    TraversalSession s{scene};
    load(s);
    for (unsigned i = 0;
         i < 120 && !(s.actors()[0].delivery->phase == DeliveryPhase::ToDestination &&
                      s.actors()[0].feet.x > -.4);
         ++i)
        step(s, 1);
    require(s.actors()[0].delivery && s.actors()[0].delivery->phase == DeliveryPhase::ToDestination,
            "fallback fixture did not start the outbound leg");
    s.cancel_delivery(0);
    // Forget the cancelled source; the nearest known compatible station wins.
    // A consistent fixture must forget its observation as well as its location.
    auto forgotten = s.serialize();
    forgotten.resize(forgotten.rfind("CHECKSUM "));
    const auto belief = forgotten.find("\nBELIEF 0 ") + 1;
    require(belief != 0, "fallback fixture did not observe its source");
    forgotten.erase(belief, forgotten.find('\n', belief) - belief + 1);
    forgotten += "CHECKSUM " + std::to_string(snapshot_checksum(forgotten)) + '\n';
    forgotten = field(forgotten, "\nKNOWLEDGE ", 3, "0");
    s.restore(field(forgotten, "\nACTOR ", 18, "14"));
    const auto cargo = s.actors()[0].cargo;
    for (unsigned i = 0; i < 400 && s.world().resource(cargo)->record.quantity.value > 0; ++i)
        step(s, 1);
    require(s.world().resource(cargo)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 16 &&
                s.world().resource(s.stations()[1].lot)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[3].lot)->record.quantity.value == 14 &&
                s.actors()[0].delivery->phase == DeliveryPhase::Cancelled,
            "return did not fall back to the nearest known compatible station");
    require(s.world().resource(s.stations()[0].lot)->record.quantity.value +
                    s.world().resource(s.stations()[1].lot)->record.quantity.value +
                    s.world().resource(s.stations()[3].lot)->record.quantity.value ==
                30,
            "fallback return changed the material total");
}
void return_hunger() {
    TraversalSession s{returning_scene(15)};
    load(s);
    for (unsigned i = 0;
         i < 120 && !(s.actors()[0].delivery->phase == DeliveryPhase::ToDestination &&
                      s.actors()[0].feet.x > -1.);
         ++i)
        step(s, 1);
    require(s.actors()[0].delivery && s.actors()[0].delivery->phase == DeliveryPhase::ToDestination,
            "hunger fixture did not start the outbound leg");
    s.cancel_delivery(0);
    for (unsigned i = 0; i < 20 && s.actors()[0].task != TraversalTask::Deliver; ++i)
        step(s, 1);
    const auto cargo = s.actors()[0].cargo;
    require(s.actors()[0].task == TraversalTask::Deliver &&
                s.world().resource(cargo)->record.quantity.value == 4,
            "fixture is not autonomously returning cargo");
    s.restore(field(s.serialize(), "\nACTOR ", 19, "749"));
    step(s, 1);
    require(s.actors()[0].task == TraversalTask::Eat &&
                s.world().resource(cargo)->record.quantity.value == 4 &&
                s.actors()[0].delivery->phase == DeliveryPhase::Cancelled,
            "critical hunger did not interrupt the cargo return");
    for (unsigned i = 0; i < 400 && s.actors()[0].hunger != 0; ++i) {
        step(s, 1);
        require(s.world().resource(cargo)->record.quantity.value == 4,
                "eating lost the returned batch");
    }
    require(s.actors()[0].hunger == 0, "critical meal did not finish");
    for (unsigned i = 0; i < 400 && s.world().resource(cargo)->record.quantity.value > 0; ++i)
        step(s, 1);
    require(s.world().resource(cargo)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 19 &&
                s.stations()[0].completed == 1 &&
                s.actors()[0].delivery->phase == DeliveryPhase::Cancelled,
            "cargo return did not resume after the critical meal");
    balance(s);
}
void return_detour() {
    TraversalSession s{detour_scene()};
    load(s);
    for (unsigned i = 0;
         i < 400 && !(s.actors()[0].delivery->phase == DeliveryPhase::ToDestination &&
                      s.actors()[0].feet.x > .5);
         ++i)
        step(s, 1);
    require(s.actors()[0].delivery &&
                s.actors()[0].delivery->phase == DeliveryPhase::ToDestination &&
                s.actors()[0].feet.x > .5,
            "detour fixture did not pass the wall");
    s.cancel_delivery(0);
    for (unsigned i = 0; i < 20 && s.actors()[0].task != TraversalTask::Deliver; ++i)
        step(s, 1);
    const auto cargo = s.actors()[0].cargo;
    require(s.actors()[0].task == TraversalTask::Deliver &&
                s.world().resource(cargo)->record.quantity.value == 4 &&
                s.actors()[0].goal.has_value(),
            "detour return was not planned");
    const auto bytes = s.serialize();
    auto clone = s;
    clone.restore(bytes);
    step(s, 20);
    step(clone, 20);
    require(s.serialize() == clone.serialize() &&
                s.world().resource(cargo)->record.quantity.value == 4,
            "cargo return diverged after a mid-return snapshot");
    for (unsigned i = 0; i < 600 && s.world().resource(cargo)->record.quantity.value > 0; ++i)
        step(s, 1);
    require(s.world().resource(cargo)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 20 &&
                s.world().resource(s.stations()[1].lot)->record.quantity.value == 0 &&
                s.actors()[0].delivery->phase == DeliveryPhase::Cancelled,
            "detour return did not deposit at the source");
    balance(s);
}
void return_step() {
    TraversalSession s{crossing()};
    load(s);
    for (unsigned i = 0; i < 150 && s.actors()[0].step_point == 0; ++i)
        step(s, 1);
    require(s.actors()[0].step_point > 0, "returned cancellation missed committed Step");
    s.cancel_delivery(0);
    const auto cargo = s.actors()[0].cargo;
    const auto bytes = s.serialize();
    auto clone = s;
    clone.restore(bytes);
    step(s, 10);
    step(clone, 10);
    require(s.serialize() == clone.serialize() &&
                s.world().resource(cargo)->record.quantity.value == 4,
            "autonomous return after a committed Step diverged after load");
    for (unsigned i = 0; i < 500 && s.world().resource(cargo)->record.quantity.value > 0; ++i)
        step(s, 1);
    require(s.world().resource(cargo)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 20 &&
                s.actors()[0].delivery->phase == DeliveryPhase::Cancelled,
            "autonomous return did not finish after a committed Step");
    balance(s);
}
void return_manual() {
    auto scene = small();
    scene.initial_deliveries.clear();
    scene.spawns[0].autonomous = false;
    scene.stations[1].initial_material = {2};
    TraversalSession s{scene};
    require(s.interact_cargo(0, 0) == CargoStatus::Transferred,
            "manual loading through the shared interaction failed");
    const auto cargo = s.actors()[0].cargo;
    require(s.world().resource(cargo)->record.quantity.value == 4,
            "manual loading did not create a physical batch");
    const auto source =
        surface_feet(s.geometry(), s.stations()[0].location, TraversalLimits{s.actors()[0].size});
    require(source.has_value(), "manual return source is unusable");
    s.approach(0, {1, {5, 0}}, .9);
    for (unsigned i = 0; i < 200 && s.actors()[0].state != TraversalState::Arrived; ++i)
        step(s, 1);
    require(s.actors()[0].state == TraversalState::Arrived && !s.actors()[0].autonomous,
            "manual fixture did not walk away from the source");
    s.set_autonomous(0, true);
    for (unsigned i = 0; i < 500 && s.world().resource(cargo)->record.quantity.value > 0; ++i)
        step(s, 1);
    require(s.world().resource(cargo)->record.quantity.value == 0 &&
                s.world().resource(s.stations()[0].lot)->record.quantity.value == 20 &&
                s.world().resource(s.stations()[1].lot)->record.quantity.value == 0 &&
                s.actors()[0].autonomous && length(s.actors()[0].feet - *source) <= 1.1,
            "autonomy did not physically return the manually loaded batch");
    balance(s);
}
void boat() {
    auto scene = make_traversal_course();
    for (auto& spawn : scene.spawns)
        spawn.autonomous = false;
    scene.spawns[1].autonomous = true;
    scene.spawns[1].hunger = 0;
    scene.stations[0].initial_material = {1};
    scene.stations[0].initial_stock = 20;
    scene.stations.push_back({{6, {0, 0}}, TraversalTask::Eat, 0});
    scene.stations.back().initial_material = {1};
    scene.initial_deliveries = {{1, 0, 4, 4}};
    TraversalSession s{scene};
    bool riding = false;
    for (unsigned i = 0; i < 1200 && s.actors()[1].delivery->delivered == 0; ++i) {
        step(s, 1);
        if (!riding && s.actors()[1].transfer == TransferStage::Riding) {
            require(s.world().resource(s.actors()[1].cargo)->record.quantity.value == 4,
                    "platform trip lost cargo");
            const auto bytes = s.serialize();
            auto clone = s;
            clone.restore(bytes);
            step(s, 30);
            step(clone, 30);
            require(s.serialize() == clone.serialize(),
                    "cargo diverged after restoring the actual platform ride");
            riding = true;
        }
    }
    if (!riding || s.actors()[1].delivery->delivered != 4)
        std::cerr << s.inspect();
    require(riding && s.actors()[1].delivery->delivered == 4,
            "cargo did not physically ride the platform and arrive");
}
void aliased_lots() {
    TraversalSession s{small()};
    const auto before = s.serialize();
    const auto header = before.find("\nSTATIONS ");
    auto first = before.find('\n', header + 1) + 1;
    first = before.find('\n', first) + 1;
    const auto last = before.find('\n', first);
    const auto row = "\n" + before.substr(first, last - first) + "\n";
    const auto bad = field(before, row, 5, std::to_string(s.stations()[0].lot.value));
    bool rejected = false;
    try {
        s.restore(bad);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before,
            "delivery recipient aliases source lot and can fake a receipt");
}
void station_kind() {
    TraversalSession s{small()};
    const auto before = s.serialize();
    const auto header = before.find("\nSTATIONS ");
    auto line = before.find('\n', header + 1) + 1;
    line = before.find('\n', line) + 1;
    line = before.find('\n', line) + 1;
    const auto last = before.find('\n', line);
    const auto row = "\n" + before.substr(line, last - line) + "\n";
    const auto bad = field(before, row, 3, "1");
    bool rejected = false;
    try {
        s.restore(bad);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before,
            "capacity-backed rest station was accepted as a food station");
}
TraversalScene walled(double source_x) {
    auto scene = small();
    scene.spawns[0].location = {1, {-.4, 0}};
    scene.stations[0].location = {1, {source_x, 0}};
    scene.geometry.bodies.push_back({2, {0, 2, 0}, 0, .05, 1, 2, 0, 0, false});
    return scene;
}
void wall() {
    TraversalSession s{walled(.4)};
    const auto before = s.serialize();
    require(s.interact_cargo(0, 0) != CargoStatus::Transferred && s.serialize() == before,
            "physical cargo transferred through a full-height solid wall");
}
void wall_approach() {
    TraversalSession s{walled(.35)};
    bool loaded = false;
    for (unsigned i = 0; i < 250 && !loaded; ++i) {
        step(s, 1);
        loaded = s.world().resource(s.actors()[0].cargo)->record.quantity.value > 0;
    }
    require(loaded && s.actors()[0].feet.x > 0,
            "autonomous pickup used the wrong side of an interaction wall");
}
void legacy_wall_need() {
    auto scene = walled(.35);
    scene.initial_deliveries.clear();
    scene.spawns[0].location = {1, {-.9, 0}};
    scene.spawns[0].autonomous = false;
    TraversalSession s{scene};
    s.approach(0, scene.stations[0].location, .9);
    for (unsigned i = 0; i < 150 && s.actors()[0].state != TraversalState::Arrived; ++i)
        step(s, 1);
    require(s.actors()[0].state == TraversalState::Arrived && !s.actors()[0].route.legs.empty(),
            "fixture did not retain an arrived legacy approach");
    auto bytes = field(s.serialize(), "\nACTOR ", 15, "1");
    bytes = field(bytes, "\nACTOR ", 16, "1");
    bytes = field(bytes, "\nACTOR ", 17, "0");
    bytes = field(bytes, "\nACTOR ", 19, "300");
    s.restore(bytes);
    for (unsigned i = 0; i < 20; ++i) {
        s.advance(1);
        s.validate();
    }
    require(s.world().resource(s.stations()[0].lot)->record.quantity.value == 20,
            "legacy approach consumed material through a wall");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected delivery test");
        const std::string_view name{argv[1]};
        if (name == "actual_outbound")
            delivery_outbound();
        else if (name == "actual_completion")
            delivery_completion();
        else if (name == "actual_single")
            delivery_single();
        else if (name == "station_kind")
            station_kind();
        else if (name == "legacy_wall_need")
            legacy_wall_need();
        else if (name == "aliased_lots")
            aliased_lots();
        else if (name == "wall")
            wall();
        else if (name == "wall_approach")
            wall_approach();
        else if (name == "meals")
            meals();
        else if (name == "blocked")
            blocked();
        else if (name == "cancel_step")
            cancel_step();
        else if (name == "return_cancelled")
            return_cancelled();
        else if (name == "return_interaction")
            return_interaction();
        else if (name == "return_fallback")
            return_fallback();
        else if (name == "return_hunger")
            return_hunger();
        else if (name == "return_detour")
            return_detour();
        else if (name == "return_step")
            return_step();
        else if (name == "return_manual")
            return_manual();
        else if (name == "boat")
            boat();
        else if (name == "completed")
            completed();
        else if (name == "local_and_cancel")
            local_and_cancel();
        else if (name == "persistence")
            persistence();
        else if (name == "needs")
            needs();
        else if (name == "manual")
            manual();
        else if (name == "no_recycling")
            no_recycling();
        else if (name == "wrong_station")
            wrong_station();
        else if (name == "corruption")
            corruption();
        else
            throw std::invalid_argument("unknown delivery test");
        std::cout << "PASS delivery " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
