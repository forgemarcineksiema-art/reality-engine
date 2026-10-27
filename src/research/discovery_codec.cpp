#include "research/discovery_session.hpp"
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
namespace reality {
namespace {
void check(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
std::uint64_t number(std::istream& in, std::uint64_t limit = UINT64_MAX) {
    std::string token;
    check(static_cast<bool>(in >> token), "missing discovery integer");
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    check(parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size() && value <= limit,
          "invalid discovery integer");
    return value;
}
unsigned small(std::istream& in, unsigned limit) {
    return static_cast<unsigned>(number(in, limit));
}
std::string text(std::istream& in) {
    std::string value;
    check(static_cast<bool>(in >> std::quoted(value)), "missing discovery section");
    return value;
}
void chain_out(std::ostream& out, const DiscoveryChain& chain) {
    out << ' ' << chain.size();
    for (const auto symbol : chain)
        out << ' ' << static_cast<unsigned>(symbol);
}
DiscoveryChain chain_in(std::istream& in) {
    const auto length = small(in, 3);
    check(length > 0, "empty discovery chain");
    DiscoveryChain chain;
    for (unsigned i = 0; i < length; ++i)
        chain.push_back(static_cast<std::uint8_t>(small(in, 2)));
    return chain;
}
} // namespace
std::string DiscoverySession::serialize() const {
    validate();
    std::ostringstream out;
    out << seed_ << ' ' << next_command_ << ' ' << static_cast<unsigned>(phase_) << '\n';
    out << std::quoted(traversal_.serialize()) << '\n'
        << std::quoted(apparatus_.serialize()) << '\n';
    out << archives_.size() << '\n';
    for (const auto& archive : archives_)
        out << std::quoted(archive.serialize()) << '\n';
    for (unsigned actor = 0; actor < 2; ++actor)
        out << std::quoted(agents_[actor].serialize()) << ' ' << attempts_[actor] << ' '
            << spent_[actor] << ' ' << learning_start_[actor].has_value() << ' '
            << learning_start_[actor].value_or(0) << ' ' << random_counters_[actor] << '\n';
    out << pending_.has_value();
    if (pending_)
        out << ' ' << pending_->actor << ' ' << pending_->stage << ' ' << pending_->command << ' '
            << pending_->input << ' ' << pending_->learning;
    out << '\n' << traces_.size() << '\n';
    for (const auto& trace : traces_) {
        out << trace.actor << ' ' << trace.command << ' ' << trace.started_tick << ' '
            << trace.input << ' ' << trace.learning;
        chain_out(out, trace.chain);
        out << ' ' << trace.prediction.has_value();
        if (trace.prediction) {
            const auto& p = *trace.prediction;
            out << ' ' << p.median << ' ' << p.minimum << ' ' << p.maximum << ' ' << p.combinations;
        }
        out << '\n';
    }
    out << exchanges_.size() << '\n';
    for (const auto& e : exchanges_) {
        out << e.sender << ' ' << e.recipient << ' ' << e.tick << ' ' << e.records.size();
        for (const auto id : e.records)
            out << ' ' << id;
        out << ' ' << std::quoted(e.contact_snapshot) << '\n';
    }
    const auto payload = out.str();
    return "GENESIS_DISCOVERY 1\n" + payload + "CHECKSUM " +
           std::to_string(snapshot_checksum(payload)) + "\n";
}
DiscoverySession DiscoverySession::deserialize(std::string_view bytes) {
    constexpr std::string_view header = "GENESIS_DISCOVERY 1\n";
    check(bytes.size() <= snapshot_byte_limit && bytes.starts_with(header),
          "invalid discovery snapshot header/size");
    const auto marker = bytes.rfind("\nCHECKSUM ");
    check(marker != std::string_view::npos && marker >= header.size(),
          "missing discovery checksum");
    const auto payload = bytes.substr(header.size(), marker + 1 - header.size());
    std::istringstream checksum{std::string(bytes.substr(marker + 10))};
    check(number(checksum) == snapshot_checksum(payload), "discovery checksum mismatch");
    std::istringstream in{std::string(payload)};
    DiscoverySession result{{DiscoveryRule{DiscoveryFamily::Loss, 1},
                             DiscoveryRule{DiscoveryFamily::Loss, 1},
                             DiscoveryRule{DiscoveryFamily::Loss, 1}}};
    result.seed_ = static_cast<std::uint32_t>(number(in, UINT32_MAX));
    result.next_command_ = number(in);
    result.phase_ = static_cast<Phase>(small(in, 2));
    result.traversal_.restore(text(in));
    result.apparatus_ = DiscoveryWorld::deserialize(text(in));
    const auto archive_count = small(in, 8);
    for (unsigned i = 0; i < archive_count; ++i)
        result.archives_.push_back(DiscoveryWorld::deserialize(text(in)));
    for (unsigned actor = 0; actor < 2; ++actor) {
        result.agents_[actor] = DiscoveryAgent::deserialize(text(in));
        result.attempts_[actor] = small(in, actor == 0 ? 24U : 2U);
        result.spent_[actor] = small(in, actor == 0 ? 1536U : 128U);
        const auto started = small(in, 1);
        const auto tick = number(in);
        check(started || tick == 0, "unset learning start contains time");
        if (started)
            result.learning_start_[actor] = tick;
        result.random_counters_[actor] = small(in, 1U << 24);
    }
    if (small(in, 1)) {
        Pending p;
        p.actor = small(in, 1);
        p.stage = small(in, 1);
        p.command = number(in);
        p.input = small(in, 8192);
        p.learning = small(in, 1) != 0;
        result.pending_ = p;
    }
    const auto trace_count = small(in, 256);
    for (unsigned i = 0; i < trace_count; ++i) {
        DiscoveryTrace trace;
        trace.actor = small(in, 1);
        trace.command = number(in);
        trace.started_tick = number(in);
        trace.input = small(in, 8192);
        trace.learning = small(in, 1) != 0;
        trace.chain = chain_in(in);
        if (small(in, 1)) {
            DiscoveryPrediction p;
            p.median = small(in, trace.input);
            p.minimum = small(in, trace.input);
            p.maximum = small(in, trace.input);
            p.combinations = number(in, 421875);
            check(p.minimum <= p.median && p.median <= p.maximum && p.combinations > 0,
                  "invalid prediction range");
            trace.prediction = p;
        }
        result.traces_.push_back(std::move(trace));
    }
    const auto exchange_count = small(in, 8);
    for (unsigned i = 0; i < exchange_count; ++i) {
        Exchange e;
        e.sender = small(in, 1);
        e.recipient = small(in, 1);
        e.tick = number(in);
        const auto count = small(in, 6);
        for (unsigned j = 0; j < count; ++j)
            e.records.push_back(number(in));
        e.contact_snapshot = text(in);
        auto contact = result.traversal_;
        contact.restore(e.contact_snapshot);
        check(e.sender != e.recipient && e.tick > 0 && e.tick <= result.tick() &&
                  contact.world().clock().tick_count() == e.tick - 1 &&
                  contact.actors().size() == 2 &&
                  contact.actors()[e.sender].id == result.agents_[e.sender].actor() &&
                  contact.actors()[e.recipient].id == result.agents_[e.recipient].actor() &&
                  contact.can_exchange_knowledge(e.sender, e.recipient),
              "exchange has no physical contact");
        for (const auto id : e.records) {
            const auto* m = result.measurement(id);
            check(m && m->source_actor == result.agents_[e.sender].actor() &&
                      std::any_of(result.agents_[e.sender].evidence().begin(),
                                  result.agents_[e.sender].evidence().end(),
                                  [&](const auto& record) {
                                      return record.measurement == *m &&
                                             record.learned_tick < e.tick;
                                  }),
                  "packet contains no original discovery evidence");
        }
        result.exchanges_.push_back(std::move(e));
    }
    in >> std::ws;
    check(in.eof(), "trailing discovery payload");
    std::array<unsigned, 2> attempts{};
    std::array<DiscoveryEnergy, 2> spent{};
    for (const auto& trace : result.traces_) {
        const auto* m = result.measurement(trace.command);
        const auto pending = result.pending_ && result.pending_->command == trace.command;
        check(m || pending, "trace without impulse");
        if (m)
            check(m->chain == trace.chain && m->input == trace.input &&
                      m->source_actor == result.agents_[trace.actor].actor() &&
                      m->observed_tick == trace.started_tick + 2,
                  "trace inconsistent with actual impulse");
        if (trace.learning) {
            ++attempts[trace.actor];
            spent[trace.actor] += trace.input;
            check(trace.chain.size() == 1 &&
                      (trace.input == 16 || trace.input == 32 || trace.input == 64),
                  "non-training observation in discovery ledger");
        }
    }
    check(attempts == result.attempts_ && spent == result.spent_,
          "training budget ledger mismatch");
    for (const auto& agent : result.agents_)
        for (const auto& e : agent.evidence())
            check(std::any_of(result.traces_.begin(), result.traces_.end(),
                              [&](const auto& trace) {
                                  return trace.learning && trace.command == e.measurement.command;
                              }),
                  "evaluation leaked into discovery evidence");
    result.validate();
    check(result.serialize() == bytes, "noncanonical discovery snapshot");
    return result;
}
void DiscoverySession::restore(std::string_view bytes) {
    auto candidate = deserialize(bytes);
    *this = std::move(candidate);
}
void DiscoverySession::save(const std::filesystem::path& path) const {
    write_snapshot_atomic(serialize(), path);
}
void DiscoverySession::load(const std::filesystem::path& path) {
    restore(read_snapshot_bytes(path));
}
} // namespace reality
