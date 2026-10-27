#pragma once

#include <cstddef>
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
} // namespace reality
