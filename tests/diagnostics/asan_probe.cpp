// Deliberately invalid memory access is isolated to this diagnostic fixture.
#include <iostream>
#include <memory>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Expected clean or heap-overflow\n";
        return 2;
    }
    const std::string_view mode{argv[1]};
    if (mode != "clean" && mode != "heap-overflow") {
        std::cerr << "Unknown diagnostic mode\n";
        return 2;
    }
    auto storage = std::make_unique<int[]>(4);
    volatile int* memory = storage.get();
    const int index = mode == "clean" ? 0 : 4;
    memory[index] = 42;
    if (mode == "clean") {
        std::cout << "CLEAN FIXTURE\n";
        return memory[0] == 42 ? 0 : 1;
    }
    std::cerr << "ERROR: sanitizer did not stop the invalid access\n";
    return 0;
}
