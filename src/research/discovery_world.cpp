#include "research/discovery_world.hpp"
#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
namespace reality {
namespace {
void check(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
bool valid_rule(DiscoveryRule rule) {
    switch (rule.family) {
    case DiscoveryFamily::Fraction:
        return rule.parameter >= 3 && rule.parameter <= 12;
    case DiscoveryFamily::Loss:
        return rule.parameter >= 1 && rule.parameter <= 24;
    case DiscoveryFamily::Cap:
        return rule.parameter >= 8 && rule.parameter <= 48;
    case DiscoveryFamily::Outside:
        return rule.parameter == 0;
    }
    return false;
}
} // namespace
DiscoveryEnergy discovery_apply(DiscoveryRule rule, DiscoveryEnergy input) {
    check(valid_rule(rule) && input <= 8192, "invalid discovery rule/input");
    switch (rule.family) {
    case DiscoveryFamily::Fraction:
        return input * rule.parameter / 16;
    case DiscoveryFamily::Loss:
        return input > rule.parameter ? input - rule.parameter : 0;
    case DiscoveryFamily::Cap:
        return std::min(input, rule.parameter);
    case DiscoveryFamily::Outside:
        return input > 16 ? std::min(input, 2 * input - 32) : 0;
    }
    throw std::invalid_argument("unknown discovery family");
}
bool discovery_chain_valid(const DiscoveryChain& chain) noexcept {
    return !chain.empty() && chain.size() <= 3 &&
           std::all_of(chain.begin(), chain.end(), [](auto symbol) { return symbol < 3; });
}
DiscoveryWorld::DiscoveryWorld(std::array<DiscoveryRule, 3> rules) : rules_(rules) {
    for (const auto rule : rules_)
        check(valid_rule(rule), "invalid apparatus rule");
}
std::optional<DiscoveryOutcome> DiscoveryWorld::replay(const Command& request) const {
    for (const auto& previous : commands_)
        if (previous.id == request.id)
            return previous.actor == request.actor && previous.apparatus == request.apparatus &&
                           previous.pulse == request.pulse && previous.chain == request.chain &&
                           previous.input == request.input
                       ? DiscoveryOutcome::Accepted
                       : DiscoveryOutcome::Conflict;
    return {};
}
DiscoveryOutcome DiscoveryWorld::configure(std::uint64_t command, EntityId actor,
                                           unsigned apparatus, DiscoveryChain chain) {
    const Command request{command, tick_, actor, apparatus, false, chain, 0};
    if (const auto previous = replay(request))
        return *previous;
    if (!command || !actor.value || apparatus >= 2 || !discovery_chain_valid(chain) ||
        commands_.size() >= record_limit || tick_ == std::numeric_limits<std::uint64_t>::max())
        return DiscoveryOutcome::Invalid;
    auto& device = devices_[apparatus];
    if (device.pending)
        return DiscoveryOutcome::Busy;
    auto next = device;
    next.chain = std::move(chain);
    next.ready_at = tick_ + 1;
    commands_.push_back(request);
    device = std::move(next);
    return DiscoveryOutcome::Accepted;
}
DiscoveryOutcome DiscoveryWorld::pulse(std::uint64_t command, EntityId actor, unsigned apparatus,
                                       DiscoveryEnergy input) {
    const Command request{command, tick_, actor, apparatus, true, {}, input};
    if (const auto previous = replay(request))
        return *previous;
    if (!command || !actor.value || apparatus >= 2 || input == 0 || input > 8192 ||
        commands_.size() >= record_limit || tick_ == std::numeric_limits<std::uint64_t>::max())
        return DiscoveryOutcome::Invalid;
    auto& device = devices_[apparatus];
    if (device.pending || device.chain.empty() || tick_ < device.ready_at)
        return DiscoveryOutcome::Busy;
    if (device.source < input)
        return DiscoveryOutcome::NoEnergy;
    commands_.push_back(request);
    device.pending = commands_.size() - 1;
    return DiscoveryOutcome::Accepted;
}
void DiscoveryWorld::advance(std::uint64_t tick) {
    check(tick >= tick_, "apparatus clock moved backwards");
    std::vector<DiscoveryMeasurement> completed;
    for (unsigned bench = 0; bench < 2; ++bench) {
        const auto& device = devices_[bench];
        if (!device.pending)
            continue;
        const auto& command = commands_.at(*device.pending);
        if (tick <= command.tick)
            continue;
        auto output = command.input;
        for (const auto symbol : device.chain)
            output = discovery_apply(rules_[symbol], output);
        completed.push_back({0, command.id, bench, command.actor, command.tick + 1, device.chain,
                             command.input, output});
    }
    std::sort(completed.begin(), completed.end(), [](const auto& a, const auto& b) {
        return a.observed_tick != b.observed_tick ? a.observed_tick < b.observed_tick
                                                  : a.command < b.command;
    });
    measurements_.reserve(measurements_.size() + completed.size());
    for (auto& measurement : completed) {
        measurement.id = measurement.command;
        auto& device = devices_[measurement.apparatus];
        device.source -= measurement.input;
        device.output += measurement.output;
        device.loss += measurement.input - measurement.output;
        device.pending.reset();
        measurements_.push_back(std::move(measurement));
    }
    tick_ = tick;
    validate();
}
const std::array<DiscoveryDevice, 2>& DiscoveryWorld::devices() const noexcept { return devices_; }
const std::vector<DiscoveryMeasurement>& DiscoveryWorld::measurements() const noexcept {
    return measurements_;
}
std::uint64_t DiscoveryWorld::tick() const noexcept { return tick_; }
DiscoveryWorld DiscoveryWorld::fresh() const { return DiscoveryWorld{rules_}; }
bool DiscoveryWorld::same_rules(const DiscoveryWorld& other) const noexcept {
    return rules_ == other.rules_;
}
void DiscoveryWorld::validate() const {
    check(commands_.size() <= record_limit && measurements_.size() <= commands_.size(),
          "apparatus record limit");
    for (unsigned bench = 0; bench < 2; ++bench) {
        const auto& d = devices_[bench];
        check(static_cast<std::uint64_t>(d.source) + d.output + d.loss == 8192,
              "apparatus energy balance");
        if (d.pending) {
            const auto& c = commands_.at(*d.pending);
            check(c.pulse && c.apparatus == bench && c.input <= d.source && c.tick == tick_,
                  "invalid pending pulse");
        }
    }
}
std::string DiscoveryWorld::serialize() const {
    validate();
    std::ostringstream out;
    out << "DWORLD 1 " << tick_;
    for (const auto rule : rules_)
        out << ' ' << static_cast<unsigned>(rule.family) << ' ' << rule.parameter;
    out << ' ' << commands_.size() << '\n';
    for (const auto& c : commands_) {
        out << c.id << ' ' << c.tick << ' ' << c.actor.value << ' ' << c.apparatus << ' ' << c.pulse
            << ' ' << c.input << ' ' << c.chain.size();
        for (const auto symbol : c.chain)
            out << ' ' << static_cast<unsigned>(symbol);
        out << '\n';
    }
    return out.str();
}
DiscoveryWorld DiscoveryWorld::deserialize(std::string_view bytes) {
    check(bytes.size() < 4 * 1024 * 1024, "apparatus snapshot too large");
    std::istringstream in{std::string(bytes)};
    std::string magic;
    unsigned version = 0;
    std::uint64_t tick = 0;
    check(static_cast<bool>(in >> magic >> version >> tick) && magic == "DWORLD" && version == 1,
          "invalid apparatus header");
    std::array<DiscoveryRule, 3> rules;
    for (auto& rule : rules) {
        unsigned family = 0;
        check(static_cast<bool>(in >> family >> rule.parameter) && family <= 3,
              "invalid rule snapshot");
        rule.family = static_cast<DiscoveryFamily>(family);
    }
    DiscoveryWorld result{rules};
    std::size_t count = 0;
    check(static_cast<bool>(in >> count) && count <= record_limit, "apparatus command count");
    for (std::size_t n = 0; n < count; ++n) {
        Command c;
        unsigned pulse = 0, length = 0;
        check(static_cast<bool>(in >> c.id >> c.tick >> c.actor.value >> c.apparatus >> pulse >>
                                c.input >> length) &&
                  pulse <= 1 && length <= 3 && c.tick <= tick,
              "invalid apparatus command");
        c.pulse = pulse != 0;
        for (unsigned i = 0; i < length; ++i) {
            unsigned symbol = 0;
            check(static_cast<bool>(in >> symbol) && symbol < 3, "invalid snapshot symbol");
            c.chain.push_back(static_cast<std::uint8_t>(symbol));
        }
        check(!result.replay(c), "duplicate snapshot command");
        result.advance(c.tick);
        const auto status = c.pulse ? result.pulse(c.id, c.actor, c.apparatus, c.input)
                                    : result.configure(c.id, c.actor, c.apparatus, c.chain);
        check(status == DiscoveryOutcome::Accepted && (c.pulse ? c.chain.empty() : c.input == 0),
              "impossible apparatus history");
    }
    result.advance(tick);
    in >> std::ws;
    check(in.eof() && result.serialize() == bytes, "noncanonical apparatus data");
    return result;
}
} // namespace reality
