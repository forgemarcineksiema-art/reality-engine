#include "research/discovery_protocol.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
int wmain(int argc, wchar_t* argv[]) {
    try {
        if (argc < 4)
            throw std::invalid_argument(
                "use --case PATH --output NEW_DIR; --fixture; --continue PATH");
        const std::wstring_view mode{argv[1]};
        const bool fixture = mode == L"--fixture";
        const auto output_arg = fixture ? 2 : 3;
        if (argc != output_arg + 2 || std::wstring_view{argv[output_arg]} != L"--output")
            throw std::invalid_argument("--output NEW_DIR required");
        const std::filesystem::path directory{argv[output_arg + 1]};
        if (!std::filesystem::create_directory(directory))
            throw std::invalid_argument("output directory must be new");
        if (fixture)
            reality::discovery_fixture(directory);
        else if (mode == L"--case")
            reality::run_discovery_case(reality::read_discovery_case(argv[2]), directory);
        else if (mode == L"--continue")
            reality::continue_discovery_fixture(argv[2], directory);
        else
            throw std::invalid_argument("unknown discovery mode");
        std::cout << "PASS discovery runner integrity\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
