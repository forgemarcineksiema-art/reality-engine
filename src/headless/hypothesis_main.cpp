#include "research/hypothesis_protocol.hpp"
#include "research/hypothesis_session.hpp"
#include <iostream>
#include <stdexcept>
namespace {
using namespace reality;
void suffix(HypothesisSession& session) {
    while (session.pending())
        session.advance(1);
    while (session.attempts() < 5) {
        const auto input = session.choose_input().input;
        if (session.begin_probe({0}, input, true) != DiscoveryOutcome::Accepted)
            throw std::logic_error("fixture impulse refused");
        session.advance(2);
    }
    session.freeze();
    (void)session.predict({0, 1}, 96);
    session = session.evaluation_copy();
    const auto chain = session.choose_chain(96, 20);
    if (chain) {
        if (session.begin_probe(*chain, 96, false) != DiscoveryOutcome::Accepted)
            throw std::logic_error("fixture evaluation refused");
        session.advance(2);
    }
}
void fixture(const std::filesystem::path& directory) {
    for (const auto mode : {HypothesisMode::SynthesisRandom, HypothesisMode::SynthesisActive}) {
        HypothesisSession session{
            HypothesisLaw{{{HypothesisKind::Fraction, 8}, {HypothesisKind::Loss, 8}}}, mode, 124};
        for (const auto input : {16U, 32U}) {
            if (session.begin_probe({0}, input, true) != DiscoveryOutcome::Accepted)
                throw std::logic_error("fixture initialization");
            session.advance(2);
        }
        for (unsigned stage = 0; stage < 4; ++stage) {
            const auto name =
                std::to_string(static_cast<unsigned>(mode)) + "-" + std::to_string(stage);
            session.save(directory / (name + ".state"));
            auto uninterrupted = session;
            suffix(uninterrupted);
            uninterrupted.save(directory / (name + ".expected"));
            if (stage == 0) {
                if (session.begin_probe({0}, 64, true) != DiscoveryOutcome::Accepted)
                    throw std::logic_error("fixture third probe");
            } else if (stage < 3)
                session.advance(1);
        }
    }
}
} // namespace
int wmain(int argc, wchar_t* argv[]) {
    try {
        if (argc < 4)
            throw std::invalid_argument(
                "--fixture --output NEW_DIR; --continue PATH --output NEW_DIR");
        const std::wstring_view mode{argv[1]};
        const bool make_fixture = mode == L"--fixture";
        const auto output_arg = make_fixture ? 2 : 3;
        if (argc != output_arg + 2 || std::wstring_view{argv[output_arg]} != L"--output")
            throw std::invalid_argument("output required");
        const std::filesystem::path directory{argv[output_arg + 1]};
        if (!std::filesystem::create_directory(directory))
            throw std::invalid_argument("output directory must be new");
        if (make_fixture)
            fixture(directory);
        else if (mode == L"--continue") {
            HypothesisSession session{HypothesisLaw{}, HypothesisMode::FlatActive, 0};
            session.load(argv[2]);
            suffix(session);
            session.save(directory / "final.state");
        } else if (mode == L"--case")
            run_hypothesis_case(argv[2], directory);
        else
            throw std::invalid_argument("unknown hypothesis mode");
        std::cout << "PASS hypothesis runner integrity\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
