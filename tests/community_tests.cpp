#include "lab/traversal_session.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reality;
namespace {
void require(bool c, const char* m) {
    if (!c)
        throw std::runtime_error(m);
}
void balance(const TraversalSession& s) {
    std::uint64_t food = 0, parts = 0, eaten = 0, used = 0;
    for (const auto& station : s.stations()) {
        const auto r = *s.world().resource(station.lot);
        if (r.record.material.value == 1)
            food += r.record.quantity.value;
        if (r.record.material.value == 2)
            parts += r.record.quantity.value;
        if (station.kind == TraversalTask::Rest)
            require(r.record.quantity.value == 2, "rest capacity was consumed");
    }
    for (const auto& a : s.actors())
        if (a.cargo.value) {
            const auto r = *s.world().resource(a.cargo);
            if (r.record.material.value == 1)
                food += r.record.quantity.value;
            if (r.record.material.value == 2)
                parts += r.record.quantity.value;
        }
    for (const auto& command : s.world().commands())
        if (command.result.status == RequestStatus::Accepted) {
            const auto a = *s.world().action(command.result.action);
            const auto r = *s.world().resource(a.request.lot);
            if (r.record.material.value == 1)
                eaten += a.consumed.value;
            if (r.record.material.value == 2)
                used += a.consumed.value;
        }
    require(food + eaten == 20 && parts + used == 24, "community material balance failed");
}
void step(TraversalSession& s) {
    s.advance(1);
    s.validate();
    balance(s);
}
void shape() {
    TraversalSession s{make_community_course()};
    require(s.actors().size() == 8 && s.stations().size() == 5,
            "community scene population/stations missing");
    require(s.world().resource(s.stations()[2].lot)->record.owner == s.actors()[4].id,
            "parts are not owned by custodian");
    require(s.actors()[1].delivery && s.actors()[1].delivery->quantity == 8,
            "courier has no actual order");
    balance(s);
}
void observe(bool disturbed) {
    TraversalSession s{make_community_course()};
    bool closed_seen = false;
    for (unsigned t = 0; t < 600; ++t) {
        if (disturbed && t == 12) {
            require(s.actors()[1].cargo.value &&
                        s.world().resource(s.actors()[1].cargo)->record.quantity.value > 0,
                    "disturbance precedes real loading");
            require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
                    "local disturbance failed");
            require(!s.actors()[1].passage_beliefs[0], "closure leaked remote truth");
        }
        step(s);
        const auto& b = s.actors()[1].passage_beliefs[0];
        closed_seen = closed_seen || (b && !b->open);
    }
    require(s.actors()[1].delivery->delivered >= 4 && s.stations()[1].completed > 0,
            "community failed to deliver and feed right bank");
    require(s.stations()[2].completed > 0 &&
                s.stations()[3].completed + s.stations()[4].completed > 0,
            "community did not work and rest autonomously");
    require(!disturbed || closed_seen, "courier never locally discovered obstruction");
    require(s.incidents().empty(), "authorized workers committed invented crime");
    std::cout << "mode=" << (disturbed ? "disturbed" : "calm")
              << " ticks=600 delivered=" << s.actors()[1].delivery->delivered
              << " meals_left=" << s.stations()[0].completed
              << " meals_right=" << s.stations()[1].completed
              << " rests=" << s.stations()[3].completed + s.stations()[4].completed
              << " repairs=" << s.stations()[2].completed << " closed_seen=" << closed_seen << '\n';
}
void law() {
    TraversalSession s{make_community_course()};
    require(s.interact_cargo(0, 2) == CargoStatus::Transferred, "physical theft failed");
    require(!s.actors()[3].testimonies.empty() && s.actors()[4].testimonies.empty() &&
                s.legal_service_allowed(0, 2),
            "witness/unknown custodian distinction failed");
    step(s);
    require(!s.actors()[4].testimonies.empty() && !s.legal_service_allowed(0, 2) &&
                !s.begin_maintenance(0, 2),
            "local automatic report did not refuse service");
    require(s.actors()[4].testimonies[0].source_actor == s.actors()[3].id,
            "custodian lost original witness");
    require(s.interact_cargo(0, 2) == CargoStatus::Transferred, "physical restitution failed");
    step(s);
    require(s.incidents()[0].returned == 4 && s.legal_service_allowed(0, 2),
            "return knowledge did not settle reaction");
    require(s.begin_maintenance(0, 2), "settled player cannot help repair");
    for (unsigned t = 0; t < 100; ++t)
        step(s);
    require(s.stations()[2].completed > 0, "physical work did not complete");
}
void continuation() {
    TraversalSession s{make_community_course()};
    for (unsigned t = 0; t < 12; ++t)
        step(s);
    require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
            "checkpoint closure failed");
    require(s.interact_cargo(0, 2) == CargoStatus::Transferred, "checkpoint theft failed");
    step(s);
    const auto bytes = s.serialize();
    require(bytes.starts_with("GENESIS_TRAVERSAL 7\n"), "integrated snapshot lost v7");
    TraversalSession copy;
    copy.restore(bytes);
    for (unsigned t = 0; t < 160; ++t) {
        step(s);
        step(copy);
        require(s.serialize() == copy.serialize(), "integrated continuation diverged");
    }
    require(s.incidents().size() == 1 && !s.legal_service_allowed(0, 2),
            "continuation lost pending incident");
    auto corrupt = s.serialize();
    corrupt[corrupt.find("INCIDENT ") + 9] = 'x';
    const auto before = s.serialize();
    bool rejected = false;
    try {
        s.restore(corrupt);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && s.serialize() == before, "failed load replaced integrated state");
}
// Reference controller: common physics/cargo, a fixed bridge itinerary, no route learning.
struct FixedCourier {
    unsigned stage = 0;
    bool issued = false;
    void update(TraversalSession& s) {
        static const SurfaceLocation points[] = {
            {1, {-1, 0}}, {3, {0, 0}}, {2, {1, 0}}, {3, {0, 0}}};
        if (s.actors()[1].delivery->phase == DeliveryPhase::Completed)
            return;
        const auto& a = s.actors()[1];
        if (!issued) {
            s.set_goal(1, points[stage]);
            issued = true;
        }
        if (a.state != TraversalState::Arrived || a.motion.speed > .01)
            return;
        if (stage == 0 || stage == 2) {
            const auto result = s.interact_cargo(1, stage == 0 ? 0 : 1);
            if (result != CargoStatus::Transferred)
                return;
        }
        stage = (stage + 1) % 4;
        issued = false;
    }
};
void reference(bool fixed, bool disturbed) {
    TraversalSession s{make_community_course()};
    FixedCourier routine;
    if (fixed)
        s.set_autonomous(1, false);
    std::uint64_t first = 0;
    bool ferry = false;
    for (unsigned t = 0; t < 240; ++t) {
        if (t == 12 && disturbed)
            require(s.interact_passage(0, 1, false) == PassageOutcome::Changed,
                    "comparison disturbance failed");
        if (fixed)
            routine.update(s);
        step(s);
        ferry = ferry || s.actors()[1].transfer == TransferStage::Riding;
        if (!first && s.actors()[1].delivery->delivered)
            first = s.world().clock().tick_count();
    }
    if (!disturbed && first == 0)
        std::cout << "fixed_stage=" << routine.stage << " feet=" << s.actors()[1].feet.x << ','
                  << s.actors()[1].feet.z << " state=" << static_cast<unsigned>(s.actors()[1].state)
                  << '\n';
    if (!disturbed)
        require(first > 0, "reference/adaptive courier failed calm delivery");
    std::cout << "controller=" << (fixed ? "fixed_bridge" : "adaptive")
              << " disturbed=" << disturbed << " ticks=240 first_delivery=" << first
              << " delivered=" << s.actors()[1].delivery->delivered
              << " meals_right=" << s.stations()[1].completed << " ferry=" << ferry
              << " state=" << static_cast<unsigned>(s.actors()[1].state)
              << " cargo=" << s.world().resource(s.actors()[1].cargo)->record.quantity.value
              << '\n';
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "case required");
        const std::string n = argv[1];
        if (n == "scene")
            shape();
        else if (n == "calm")
            observe(false);
        else if (n == "disturbed")
            observe(true);
        else if (n == "law")
            law();
        else if (n == "continuation")
            continuation();
        else if (n == "adaptive_calm")
            reference(false, false);
        else if (n == "adaptive_disturbed")
            reference(false, true);
        else if (n == "fixed_calm")
            reference(true, false);
        else if (n == "fixed_disturbed")
            reference(true, true);
        else
            throw std::runtime_error("unknown case");
        std::cout << "PASS " << n << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
