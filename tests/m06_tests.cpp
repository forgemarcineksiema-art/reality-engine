#include "lab/traversal_session.hpp"
#include "persistence/world_snapshot.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace reality;
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
TraversalSession prepared() {
    auto scene = make_learning_course();
    for (auto& station : scene.stations)
        if (station.kind != TraversalTask::Rest)
            station.initial_stock = 10000;
    return TraversalSession(std::move(scene));
}
void continuation(const std::filesystem::path& source, const std::filesystem::path& output,
                  unsigned steps = 200) {
    require(!std::filesystem::exists(output), "M06 output already exists");
    const auto original = read_snapshot_bytes(source);
    TraversalSession uninterrupted, restored;
    uninterrupted.restore(original);
    restored.restore(original);
    require(uninterrupted.serialize() == original, "loaded world changed its canonical bytes");
    for (unsigned tick = 0; tick < steps; ++tick) {
        try {
            uninterrupted.advance(1);
            uninterrupted.validate();
        } catch (...) {
            std::cout << "FAILED_FORECAST tick=" << tick + 1 << '\n' << uninterrupted.inspect();
            TraversalSession before;
            before.restore(original);
            before.advance(tick);
            auto failure = output;
            failure += L".before-failure.save";
            before.save(failure);
            throw;
        }
        if (steps > 200 && tick % 1200 == 1199)
            std::cout << "forecast_tick=" << tick + 1
                      << " events=" << uninterrupted.world().events().size() << std::endl;
        if (tick < steps / 2)
            restored.advance(1);
    }
    restored.save(output);
    TraversalSession next_process;
    next_process.load(output);
    for (unsigned tick = 0; tick < steps - steps / 2; ++tick) {
        next_process.advance(1);
        next_process.validate();
    }
    require(next_process.serialize() == uninterrupted.serialize(),
            "M06 checkpoint changed continuation");
    require(read_snapshot_bytes(source) == original, "M06 continuation overwrote evidence");
    auto final = output;
    final += L".expected.save";
    require(!std::filesystem::exists(final), "M06 expected output already exists");
    uninterrupted.save(final);
    std::cout << uninterrupted.inspect();
    std::cout << "M06 CONTINUATION PASS ticks=" << steps << " checkpoint=" << steps / 2 << "\n";
}
void completed_target() {
    auto session = prepared();
    session.advance(914);
    session.validate();
    session.advance(1);
    session.validate();
    for (const auto& actor : session.actors())
        require(actor.goal || actor.route.legs.empty(), "finished target retained its route");
    TraversalSession restored;
    restored.restore(session.serialize());
    require(restored.serialize() == session.serialize(), "completed target cannot be restored");
    std::cout << "M06 COMPLETED TARGET PASS tick=915\n";
}
void foundation() {
    auto session = prepared();
    require(session.actors().size() == 8, "M06 fixture changed population");
    for (const auto& station : session.stations()) {
        const auto resource = session.world().resource(station.lot);
        require(resource.has_value(), "M06 fixture lost resource");
        require(resource->record.quantity.value ==
                    (station.kind == TraversalTask::Rest ? 2U : 10000U),
                "M06 fixture changed rest capacity or material supply");
    }
    const auto before = session.serialize();
    TraversalSession restored;
    restored.restore(before);
    session.advance(40);
    restored.advance(40);
    require(session.serialize() == restored.serialize(), "M06 initial world is not repeatable");
    session.validate();
    std::cout << "M06 FOUNDATION PASS\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            foundation();
        } else if (argc == 2 && std::string_view(argv[1]) == "--completed-target") {
            completed_target();
        } else if (argc == 3 && std::string_view(argv[1]) == "--prepare") {
            require(!std::filesystem::exists(argv[2]), "M06 initial output already exists");
            prepared().save(argv[2]);
            std::cout << "M06 PREPARED stock=10000 per material station\n";
        } else if (argc == 4 && std::string_view(argv[1]) == "--forecast") {
            continuation(argv[2], argv[3], 38400);
        } else if (argc == 4 && std::string_view(argv[1]) == "--continue") {
            continuation(argv[2], argv[3]);
        } else {
            throw std::invalid_argument("M06 requires --prepare PATH or --continue SOURCE OUTPUT");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
