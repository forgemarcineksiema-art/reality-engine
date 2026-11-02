#define main original_two_methods_main
#include "tests/two_methods_tests.cpp"
#undef main
int main(int argc, char** argv) {
    try {
        require(argc == 3, "checkpoint and output required");
        const auto start = std::chrono::steady_clock::now();
        const auto timing = [&](const char* name) {
            std::cout << name << "=" << std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count() << std::endl;
        };
        TraversalSession session;
        timing("constructed");
        session.load(argv[1]);
        timing("loaded");
        static_cast<void>(finish(session, false));
        timing("finished");
        session.save(argv[2]);
        timing("saved");
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
