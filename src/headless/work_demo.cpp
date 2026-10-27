#include "persistence/world_snapshot.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

int wmain(int argc, wchar_t* argv[]) {
    try {
        if (argc != 3) {
            throw std::invalid_argument(
                "Usage: reality_work_demo --create|--resume|--inspect FILE");
        }
        const std::wstring_view mode{argv[1]};
        if (mode == L"--create") {
            reality::WorkWorld world;
            const auto first = world.create_actor();
            const auto second = world.create_actor();
            const auto lot = world.create_resource(first, reality::Quantity{3});
            const reality::WorkRequest request{
                reality::CommandId{1}, first, lot, reality::Quantity{3}, reality::Quantity{1}, 5};
            const auto result = world.reserve(request);
            const auto denied =
                world.reserve({reality::CommandId{2}, second, lot, reality::Quantity{1}, {}, 2});
            if (result.status != reality::RequestStatus::Accepted ||
                denied.status != reality::RequestStatus::InsufficientResource ||
                world.begin(result.action) != reality::Transition::Changed) {
                throw std::logic_error("demo setup failed");
            }
            world.advance(2);
            reality::save_world(world, argv[2]);
            std::cout << world.inspect();
        } else if (mode == L"--resume") {
            auto world = reality::load_world(argv[2]);
            world.advance(3);
            reality::save_world(world, argv[2]);
            std::cout << world.inspect();
        } else if (mode == L"--inspect") {
            const auto world = reality::load_world(argv[2]);
            std::cout << world.inspect();
        } else {
            throw std::invalid_argument("unknown demo mode");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
