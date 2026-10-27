#include "research/hypothesis_session.hpp"
#include <charconv>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace reality {
namespace {
std::uint64_t number(std::istream& in, std::uint64_t max = UINT64_MAX) {
    std::string token;
    if (!(in >> token))
        throw std::invalid_argument("missing hypothesis number");
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size() || value > max)
        throw std::invalid_argument("invalid hypothesis number");
    return value;
}
std::string quoted(std::istream& in) {
    std::string text;
    if (!(in >> std::quoted(text)))
        throw std::invalid_argument("missing hypothesis section");
    return text;
}
DiscoveryChain chain_in(std::istream& in) {
    DiscoveryChain chain;
    const auto n = number(in, 3);
    if (!n)
        throw std::invalid_argument("empty hypothesis chain");
    for (unsigned i = 0; i < n; ++i)
        chain.push_back(static_cast<std::uint8_t>(number(in, 2)));
    return chain;
}
} // namespace
std::string HypothesisSession::canonical_state() const {
    std::ostringstream out;
    out << next_command_ << ' ' << random_counter_ << ' ' << attempts_ << ' ' << spent_ << ' '
        << learning_start_.has_value() << ' ' << learning_start_.value_or(0) << '\n';
    out << pending_.has_value();
    if (pending_)
        out << ' ' << pending_->stage << ' ' << pending_->input << ' ' << pending_->command << ' '
            << pending_->learning;
    out << '\n'
        << std::quoted(traversal_.serialize()) << '\n'
        << std::quoted(apparatus_.serialize()) << '\n'
        << archive_.has_value() << '\n';
    if (archive_)
        out << std::quoted(archive_->serialize()) << '\n';
    out << std::quoted(agent_.state()) << '\n';
    for (const auto input : permutation_)
        out << input << ' ';
    out << '\n';
    return out.str();
}
std::string HypothesisSession::serialize() const {
    std::ostringstream out;
    out << seed_ << ' ' << static_cast<unsigned>(mode_) << ' ' << limits_.primitives << ' '
        << limits_.reads << ' ' << law_.square_divisor << ' ' << law_.program.size();
    for (const auto op : law_.program)
        out << ' ' << static_cast<unsigned>(op.kind) << ' ' << op.parameter;
    out << '\n' << std::quoted(initial_) << '\n' << events_.size() << '\n';
    for (const auto& event : events_)
        out << std::quoted(event) << '\n';
    out << std::quoted(canonical_state()) << '\n';
    const auto body = out.str();
    if (body.size() + 64 >= snapshot_byte_limit)
        throw std::invalid_argument("hypothesis snapshot limit");
    return "GENESIS_HYPOTHESIS 1 " + std::to_string(hypothesis_checksum(body)) + "\n" + body;
}
HypothesisSession HypothesisSession::deserialize(std::string_view bytes) {
    if (bytes.size() >= snapshot_byte_limit)
        throw std::invalid_argument("hypothesis snapshot limit");
    const auto newline = bytes.find('\n');
    if (newline == std::string_view::npos)
        throw std::invalid_argument("hypothesis header missing");
    std::istringstream header{std::string(bytes.substr(0, newline))};
    std::string magic;
    header >> magic;
    if (magic != "GENESIS_HYPOTHESIS" || number(header, 1) != 1)
        throw std::invalid_argument("hypothesis version");
    const auto checksum = number(header);
    header >> std::ws;
    const auto body = bytes.substr(newline + 1);
    if (!header.eof() || checksum != hypothesis_checksum(body))
        throw std::invalid_argument("hypothesis checksum");
    std::istringstream in{std::string(body)};
    const auto seed = static_cast<std::uint32_t>(number(in, UINT32_MAX));
    const auto mode = static_cast<HypothesisMode>(number(in, 2));
    HypothesisLimits limits{number(in, 10000000), number(in, 300000000)};
    HypothesisLaw law;
    law.square_divisor = static_cast<unsigned>(number(in, 256));
    const auto n = number(in, 3);
    for (unsigned i = 0; i < n; ++i)
        law.program.push_back(
            {static_cast<HypothesisKind>(number(in, 3)), static_cast<unsigned>(number(in, 64))});
    HypothesisSession result{law, mode, seed, make_discovery_scene(), limits};
    result.initial_ = quoted(in);
    result.traversal_.restore(result.initial_);
    if (result.traversal_.actors().size() != 2 || result.tick() != 0)
        throw std::invalid_argument("hypothesis initial scene");
    result.agent_ = HypothesisAgent{result.traversal_.actors()[0].id, mode, limits};
    const auto count = number(in, record_limit);
    for (unsigned i = 0; i < count; ++i) {
        const auto event = quoted(in);
        std::istringstream args{event};
        char op = 0;
        args >> op;
        switch (op) {
        case 'b': {
            const bool learning = number(args, 1) != 0;
            const auto input = static_cast<unsigned>(number(args, 128));
            if (result.begin_probe(chain_in(args), input, learning) != DiscoveryOutcome::Accepted)
                throw std::invalid_argument("impossible hypothesis impulse");
            break;
        }
        case 'a':
            result.advance(number(args, 10000));
            break;
        case 'i':
            (void)result.choose_input();
            break;
        case 'p': {
            const auto input = static_cast<unsigned>(number(args, 128));
            (void)result.predict(chain_in(args), input);
            break;
        }
        case 'c': {
            const auto input = static_cast<unsigned>(number(args, 128));
            const auto target = static_cast<unsigned>(number(args, 128));
            (void)result.choose_chain(input, target);
            break;
        }
        case 'f':
            result.freeze();
            break;
        case 'e':
            result.enter_evaluation();
            break;
        case 'g': {
            SurfaceLocation goal;
            goal.surface = static_cast<std::uint32_t>(number(args, UINT32_MAX));
            if (!(args >> goal.local.x_m >> goal.local.z_m))
                throw std::invalid_argument("hypothesis goal");
            result.set_goal(goal);
            break;
        }
        default:
            throw std::invalid_argument("hypothesis event kind");
        }
        if (result.events_.back() != event)
            throw std::invalid_argument("noncanonical hypothesis event");
    }
    const auto state = quoted(in);
    in >> std::ws;
    if (!in.eof() || result.canonical_state() != state || result.serialize() != bytes)
        throw std::invalid_argument("hypothesis semantic mismatch");
    return result;
}
void HypothesisSession::restore(std::string_view bytes) {
    auto candidate = deserialize(bytes);
    *this = std::move(candidate);
}
void HypothesisSession::save(const std::filesystem::path& path) const {
    write_snapshot_atomic(serialize(), path);
}
void HypothesisSession::load(const std::filesystem::path& path) {
    restore(read_snapshot_bytes(path));
}
} // namespace reality
