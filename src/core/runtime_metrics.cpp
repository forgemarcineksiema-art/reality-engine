#include "core/runtime_metrics.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace reality {
namespace {
TimingSummary summarize(std::vector<double> values) {
    if (values.empty())
        return {};
    std::sort(values.begin(), values.end());
    const auto percentile = [&](double p) {
        return values[static_cast<std::size_t>(std::ceil(p * static_cast<double>(values.size()))) -
                      1];
    };
    double mean = 0;
    std::size_t count = 0;
    for (const auto value : values)
        mean += (value - mean) / static_cast<double>(++count);
    return {values.size(),
            values.front(),
            percentile(.5),
            percentile(.95),
            percentile(.99),
            values.back(),
            mean};
}
} // namespace
void RuntimeMetrics::record(double frame_ms, double simulation_ms, double backlog_ms) {
    for (const auto value : {frame_ms, simulation_ms, backlog_ms})
        if (!std::isfinite(value) || value < 0)
            throw std::invalid_argument("invalid runtime timing sample");
    if (frames_.size() == 1000000)
        throw std::length_error("runtime timing sample budget exhausted");
    const auto needed = frames_.size() + 1;
    if (frames_.capacity() < needed || simulation_.capacity() < needed ||
        backlog_.capacity() < needed) {
        const auto capacity =
            std::min(std::size_t{1000000}, std::max(std::size_t{1024}, needed * 2));
        frames_.reserve(capacity);
        simulation_.reserve(capacity);
        backlog_.reserve(capacity);
    }
    frames_.push_back(frame_ms);
    simulation_.push_back(simulation_ms);
    backlog_.push_back(backlog_ms);
}
TimingSummary RuntimeMetrics::frames() const { return summarize(frames_); }
std::string RuntimeMetrics::report() const {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(4);
    const auto append = [&](const char* name, const std::vector<double>& samples) {
        const auto s = summarize(samples);
        out << name << "_samples=" << s.count << '\n'
            << name << "_min_ms=" << s.minimum_ms << '\n'
            << name << "_p50_ms=" << s.p50_ms << '\n'
            << name << "_p95_ms=" << s.p95_ms << '\n'
            << name << "_p99_ms=" << s.p99_ms << '\n'
            << name << "_max_ms=" << s.maximum_ms << '\n'
            << name << "_mean_ms=" << s.mean_ms << '\n';
    };
    append("frame", frames_);
    append("simulation", simulation_);
    append("backlog", backlog_);
    return out.str();
}
} // namespace reality
