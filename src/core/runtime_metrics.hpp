#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace reality {
struct TimingSummary {
    std::size_t count = 0;
    double minimum_ms = 0;
    double p50_ms = 0;
    double p95_ms = 0;
    double p99_ms = 0;
    double maximum_ms = 0;
    double mean_ms = 0;
};
class RuntimeMetrics final {
  public:
    void record(double frame_ms, double simulation_ms, double backlog_ms);
    [[nodiscard]] TimingSummary frames() const;
    [[nodiscard]] std::string report() const;

  private:
    std::vector<double> frames_, simulation_, backlog_;
};
enum class MeasurementPhase { Warmup, Measurement };
struct MeasurementSample {
    double elapsed_ms = 0;
    std::uint64_t tick = 0;
    MeasurementPhase phase = MeasurementPhase::Warmup;
    double frame_ms = 0, simulation_ms = 0, backlog_ms = 0;
    std::optional<std::uint64_t> working_set_bytes, private_bytes;
};
// Process-local observer: elapsed time orders samples even after a world restore.
class MeasurementTrace final {
  public:
    void record(MeasurementSample sample);
    [[nodiscard]] std::string csv() const;
    [[nodiscard]] std::size_t size() const noexcept { return samples_.size(); }

  private:
    std::vector<MeasurementSample> samples_;
};
} // namespace reality
