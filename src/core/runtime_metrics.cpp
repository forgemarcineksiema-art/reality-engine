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
void MeasurementTrace::record(MeasurementSample sample) {
    for (const auto value :
         {sample.elapsed_ms, sample.frame_ms, sample.simulation_ms, sample.backlog_ms})
        if (!std::isfinite(value) || value < 0)
            throw std::invalid_argument("invalid measurement trace timing");
    if ((sample.phase != MeasurementPhase::Warmup &&
         sample.phase != MeasurementPhase::Measurement) ||
        sample.working_set_bytes.has_value() != sample.private_bytes.has_value())
        throw std::invalid_argument("invalid measurement trace phase or memory pair");
    if (!samples_.empty() && (sample.elapsed_ms < samples_.back().elapsed_ms ||
                              (samples_.back().phase == MeasurementPhase::Measurement &&
                               sample.phase == MeasurementPhase::Warmup)))
        throw std::invalid_argument("measurement trace moved backwards");
    if (samples_.size() == 1000000)
        throw std::length_error("measurement trace sample budget exhausted");
    samples_.push_back(sample);
}
std::string MeasurementTrace::csv() const {
    std::ostringstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out.imbue(std::locale::classic());
    out << "elapsed_ms,tick,phase,frame_ms,simulation_ms,backlog_ms,working_set_bytes,private_"
           "bytes\n"
        << std::fixed << std::setprecision(6);
    for (const auto& sample : samples_) {
        out << sample.elapsed_ms << ',' << sample.tick << ','
            << (sample.phase == MeasurementPhase::Warmup ? "warmup" : "measurement") << ','
            << sample.frame_ms << ',' << sample.simulation_ms << ',' << sample.backlog_ms << ',';
        if (sample.working_set_bytes)
            out << *sample.working_set_bytes;
        out << ',';
        if (sample.private_bytes)
            out << *sample.private_bytes;
        out << '\n';
    }
    return out.str();
}
} // namespace reality
