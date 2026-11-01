#include "core/runtime_metrics.hpp"
#include "lab/traversal_diagnostics.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>

using namespace reality;
namespace {
void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
NavigationSample travelling(std::uint64_t tick, double distance = 3) {
    NavigationSample sample{EntityId{1},
                            tick,
                            NavigationActivity::Travelling,
                            SurfaceLocation{1, {3, 0}},
                            SurfaceLocation{1, {3, 0}},
                            distance};
    sample.position = SpatialPoint{};
    sample.support = 1;
    return sample;
}
void progress() {
    NavigationProgressWatch watch;
    for (std::uint64_t t = 0; t <= 300; ++t)
        watch.observe(travelling(t, 10 - static_cast<double>(t) * .02));
    require(watch.activity() == NavigationActivity::Travelling,
            "slow continuous progress reported as stalled");
    NavigationProgressWatch oscillating;
    for (std::uint64_t t = 0; t < 100; ++t) {
        oscillating.observe(travelling(t, 3 + (t % 2 == 0 ? .02 : -.02)));
        require(oscillating.activity() == NavigationActivity::Travelling,
                "stall reported before the observation window elapsed");
    }
    oscillating.observe(travelling(100, 3));
    require(oscillating.activity() == NavigationActivity::SuspectedStall,
            "small oscillations hide lack of meaningful progress");
    oscillating.observe(travelling(101, 2.8));
    require(oscillating.activity() == NavigationActivity::Travelling,
            "watch failed to recover after actual progress");
}
void waiting() {
    for (const auto mode : {NavigationActivity::Inactive, NavigationActivity::Waiting,
                            NavigationActivity::Transfer, NavigationActivity::NoPath,
                            NavigationActivity::Arrived, NavigationActivity::BudgetExceeded}) {
        NavigationProgressWatch watch;
        auto sample = travelling(0);
        sample.activity = mode;
        for (std::uint64_t t = 0; t <= 300; ++t) {
            sample.tick = t;
            watch.observe(sample);
        }
        require(watch.activity() == mode,
                "legal waiting or failure classified as locomotion stall");
    }
    auto scene = make_traversal_course();
    scene.spawns = {{{1, {0, -2.8}}, {}, false}, {{3, {0, 0}}, {}, false}};
    TraversalSession session{scene};
    session.set_goal(0, {2, {0, 0}});
    TraversalDiagnostics diagnostics;
    for (unsigned t = 0; t < 200; ++t) {
        session.advance(1);
        session.validate();
        diagnostics.observe(session);
    }
    require(!diagnostics.suspected_stall(), "real passage queue raises false locomotion alarm");
}
void reset() {
    NavigationProgressWatch watch;
    watch.observe(travelling(0));
    watch.observe(travelling(100)); // A whole unseen window is not evidence of a stall.
    require(watch.activity() == NavigationActivity::Travelling, "unobserved gap became a stall");
    for (std::uint64_t t = 101; t <= 200; ++t)
        watch.observe(travelling(t));
    require(watch.activity() == NavigationActivity::SuspectedStall,
            "continuous stalled window missed");
    auto changed = travelling(201);
    changed.waypoint = SurfaceLocation{1, {0, 3}};
    changed.position = SpatialPoint{1, 0, 0};
    watch.observe(changed);
    require(watch.activity() == NavigationActivity::Travelling, "next route leg inherited alarm");
    watch.observe(travelling(0));
    require(watch.activity() == NavigationActivity::Travelling, "restored clock inherited alarm");
    watch.reset();
    require(watch.activity() == NavigationActivity::Inactive,
            "explicit reset retained observation");
    bool rejected = false;
    try {
        watch.observe(travelling(1, std::numeric_limits<double>::quiet_NaN()));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "invalid progress sample accepted");
}
void snapshot() {
    TraversalSession session;
    TraversalDiagnostics diagnostics;
    diagnostics.observe(session);
    session.advance(2);
    const auto bytes = session.serialize();
    static_cast<void>(diagnostics.report());
    require(session.serialize() == bytes, "observation mutated the world");
    const auto root = std::filesystem::temp_directory_path() /
                      ("genesis-diagnostic-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root);
    try {
        const auto directory = root / L"próba-桥";
        diagnostics.capture(session, directory, "test");
        TraversalSession loaded;
        loaded.load(directory / "world.save");
        require(loaded.serialize() == bytes, "diagnostic snapshot changed the saved world");
        require(read_snapshot_bytes(directory / "diagnostic.txt").find("test") != std::string::npos,
                "snapshot lacks its reason");
        require(read_snapshot_bytes(directory / "diagnostic.txt").find("observed_tick=2") !=
                    std::string::npos,
                "diagnostic snapshot reports an observation older than its world");
        bool rejected = false;
        try {
            diagnostics.capture(session, directory, "overwrite");
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected && read_snapshot_bytes(directory / "world.save") == bytes,
                "duplicate capture damaged prior evidence");
        require(session.serialize() == bytes, "failed capture changed live world");
        {
            std::ofstream blocked{root / "file"};
            blocked << "preserve";
        }
        rejected = false;
        try {
            diagnostics.capture(session, root / "file" / "case", "blocked");
        } catch (const std::exception&) {
            rejected = true;
        }
        require(rejected && read_snapshot_bytes(root / "file") == "preserve" &&
                    session.serialize() == bytes,
                "blocked diagnostic path damaged data or world");
    } catch (...) {
        std::filesystem::remove_all(root);
        throw;
    }
    std::filesystem::remove_all(root);
}
void timings() {
    RuntimeMetrics metrics;
    for (const auto frame : {1., 2., 3., 4., 100.})
        metrics.record(frame, .5, 0);
    const auto summary = metrics.frames();
    require(summary.count == 5 && summary.minimum_ms == 1 && summary.p50_ms == 3 &&
                summary.p95_ms == 100 && summary.p99_ms == 100 && summary.maximum_ms == 100 &&
                std::abs(summary.mean_ms - 22) < 1e-9,
            "timing distribution hides an actual long frame");
    bool rejected = false;
    try {
        metrics.record(1, -1, 0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && metrics.frames().count == 5, "invalid metric partially changed sample set");
    require(RuntimeMetrics{}.frames().count == 0, "empty timing window fabricated samples");
    RuntimeMetrics large;
    for (unsigned i = 0; i < 4; ++i)
        large.record(1e308, 0, 0);
    require(std::isfinite(large.frames().mean_ms), "finite timing samples overflow their mean");
}
void trace_format() {
    struct Comma final : std::numpunct<char> {
        char do_decimal_point() const override { return ','; }
    };
    const auto original = std::locale();
    std::locale::global(std::locale{original, new Comma});
    try {
        MeasurementTrace trace;
        trace.record({1.25, 40, MeasurementPhase::Warmup, 1.25, .5, 0, {}, {}});
        trace.record({2.5, 41, MeasurementPhase::Measurement, 1.25, .25, 2, 12345, 67890});
        trace.record({4, 2, MeasurementPhase::Measurement, 1.5, .5, 0, {}, {}});
        require(trace.size() == 3 &&
                    trace.csv() ==
                        "elapsed_ms,tick,phase,frame_ms,simulation_ms,backlog_ms,working_set_bytes,"
                        "private_bytes\n"
                        "1.250000,40,warmup,1.250000,0.500000,0.000000,,\n"
                        "2.500000,41,measurement,1.250000,0.250000,2.000000,12345,67890\n"
                        "4.000000,2,measurement,1.500000,0.500000,0.000000,,\n",
                "CSV lost phase, precision, missing memory or restored tick under locale");
    } catch (...) {
        std::locale::global(original);
        throw;
    }
    std::locale::global(original);
}
void trace_invalid() {
    MeasurementTrace trace;
    trace.record({10, 10, MeasurementPhase::Measurement, 1, .5, 0, {}, {}});
    const auto before = trace.csv();
    const MeasurementSample valid{11, 2, MeasurementPhase::Measurement, 1, .5, 0, {}, {}};
    const auto reject = [&](MeasurementSample sample) {
        bool rejected = false;
        try {
            trace.record(sample);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected && trace.size() == 1 && trace.csv() == before,
                "invalid trace sample changed prior evidence");
    };
    for (const auto bad :
         {-1., std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        for (unsigned field = 0; field < 4; ++field) {
            auto sample = valid;
            switch (field) {
            case 0:
                sample.elapsed_ms = bad;
                break;
            case 1:
                sample.frame_ms = bad;
                break;
            case 2:
                sample.simulation_ms = bad;
                break;
            case 3:
                sample.backlog_ms = bad;
                break;
            }
            reject(sample);
        }
    }
    auto sample = valid;
    sample.elapsed_ms = 9;
    reject(sample);
    sample = valid;
    sample.phase = MeasurementPhase::Warmup;
    reject(sample);
    sample.phase = static_cast<MeasurementPhase>(99);
    reject(sample);
    sample = valid;
    sample.working_set_bytes = 100;
    reject(sample);
    sample = valid;
    sample.private_bytes = 100;
    reject(sample);
    trace.record(valid);
    require(trace.size() == 2, "restored world tick was mistaken for wall-clock reversal");
}
void trace_capacity() {
    MeasurementTrace trace;
    const MeasurementSample sample{1, 1, MeasurementPhase::Measurement, 1, 0, 0, {}, {}};
    for (std::size_t i = 0; i < 1000000; ++i)
        trace.record(sample);
    bool rejected = false;
    try {
        trace.record(sample);
    } catch (const std::length_error&) {
        rejected = true;
    }
    require(rejected && trace.size() == 1000000,
            "trace silently dropped or accepted samples beyond its explicit budget");
}
void trace_observer() {
    TraversalSession observed, reference;
    MeasurementTrace trace;
    for (unsigned i = 0; i < 40; ++i) {
        observed.advance(1);
        reference.advance(1);
        trace.record({static_cast<double>(i),
                      observed.world().clock().tick_count(),
                      i < 10 ? MeasurementPhase::Warmup : MeasurementPhase::Measurement,
                      1,
                      .5,
                      0,
                      {},
                      {}});
        static_cast<void>(trace.csv());
        require(observed.serialize() == reference.serialize(),
                "measurement trace changed world or persistence semantics");
    }
}
void invalid_sample() {
    NavigationProgressWatch watch;
    watch.observe(travelling(0));
    const auto before = watch.report();
    auto sample = travelling(20);
    sample.goal.reset();
    bool rejected = false;
    try {
        watch.observe(sample);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && watch.report() == before,
            "rejected navigation intent partially changed progress checkpoint");
    sample = travelling(20);
    sample.activity = static_cast<NavigationActivity>(100);
    rejected = false;
    try {
        watch.observe(sample);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && watch.report() == before, "unknown activity changed progress state");
}
void plan_churn() {
    NavigationProgressWatch watch;
    for (std::uint64_t tick = 0; tick <= 220; ++tick) {
        auto sample = travelling(tick);
        sample.waypoint = SurfaceLocation{1, {3, static_cast<double>((tick / 10) % 2)}};
        if (tick % 2 != 0)
            sample.activity = NavigationActivity::Waiting;
        watch.observe(sample);
    }
    require(watch.activity() == NavigationActivity::SuspectedStall,
            "replanning and transient Waiting hide a stationary intent forever");
}
void independent_alarms() {
    TraversalDiagnostics diagnostics;
    for (std::uint64_t tick = 0; tick <= 210; ++tick) {
        auto first = travelling(tick);
        auto second = travelling(tick, tick <= 100 ? 10 - static_cast<double>(tick) * .02 : 8);
        second.actor = EntityId{2};
        std::array samples{first, second};
        if (tick >= 50)
            std::swap(samples[0], samples[1]);
        diagnostics.observe(samples);
        const auto& alerts = diagnostics.new_stalls();
        if (tick == 100)
            require(alerts.size() == 1 && alerts.front() == EntityId{1},
                    "reordering actors hides the first independent alarm");
        else if (tick == 200)
            require(alerts.size() == 1 && alerts.front() == EntityId{2},
                    "another active alarm hides a newly stalled actor");
        else
            require(alerts.empty(), "same unresolved alarm emitted duplicate captures");
    }
}
void service_progress() {
    for (const auto mode :
         {NavigationActivity::Parking, NavigationActivity::Precision, NavigationActivity::Riding}) {
        NavigationProgressWatch stopped, moving, blocked;
        for (std::uint64_t tick = 0; tick <= 220; ++tick) {
            auto sample = travelling(tick);
            sample.activity = mode;
            sample.resource = 7;
            stopped.observe(sample);
            sample.externally_blocked = true;
            blocked.observe(sample);
            sample.externally_blocked = false;
            if (mode == NavigationActivity::Riding)
                sample.progress = tick / 20;
            else {
                sample.position = SpatialPoint{static_cast<double>(tick) * .02, 0, 0};
                sample.distance_m = 3;
            }
            moving.observe(sample);
        }
        require(stopped.activity() == NavigationActivity::SuspectedStall,
                "stopped parking, precision movement or service was not monitored");
        require(moving.activity() == mode,
                "real service or support-relative progress raised alarm");
        require(blocked.activity() == mode,
                "declared physical obstruction reported as unexplained stall");
    }
}
void batch_transaction() {
    TraversalDiagnostics diagnostics;
    for (std::uint64_t tick = 0; tick <= 100; ++tick) {
        auto first = travelling(tick);
        auto other = travelling(tick);
        other.actor = EntityId{2};
        if (tick < 50)
            diagnostics.observe(std::array{first, other});
        else
            diagnostics.observe(std::array{first});
    }
    require(diagnostics.new_stalls() == std::vector{EntityId{1}},
            "removing another actor restarted this actor's observation");
    const auto before = diagnostics.report();
    auto invalid = travelling(101);
    invalid.actor = EntityId{0};
    bool rejected = false;
    try {
        diagnostics.observe(std::array{travelling(101), invalid});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected && diagnostics.report() == before &&
                diagnostics.new_stalls() == std::vector{EntityId{1}},
            "failed observation batch partially changed state or alarms");
    auto second = travelling(101);
    second.actor = EntityId{3};
    diagnostics.observe(std::array{second, travelling(101)});
    require(diagnostics.new_stalls().empty(), "adding an actor duplicated an existing alarm");
}
void live_view() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-3, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                             {2, {5, 0, 0}, 0, 2, 3, .3, 0, 0, true},
                             {3, {}, 0, 1, 1, .25, 0, 0, true}};
    scene.platforms = {{{3, {2, 0, 0}, 0, 60, 10, 0}, 0, false}};
    scene.connections = {{{1, {1.8, 0}}, {2, {-1.8, 0}}, SurfaceLegKind::Transport, 0, 1, 4}};
    scene.services = {{1, 3, {{{1, {1.8, 0}}, {2, {-1.8, 0}}}}, {{{-.8, 0}, {.8, 0}}}, {}, {}}};
    scene.spawns = {{{1, {0, 0}}, {}, false}};
    TraversalSession session{scene};
    session.set_goal(0, {2, {0, 0}});
    TraversalDiagnostics diagnostics;
    bool parking = false, precision = false, riding = false;
    for (unsigned tick = 0; tick < 600; ++tick) {
        session.advance(1);
        session.validate();
        const auto bytes = session.serialize();
        for (std::size_t i = 0; i < session.actors().size(); ++i) {
            const auto view = session.navigation_view(i);
            parking = parking || view.kind == TraversalGuidanceKind::Parking;
            precision = precision || view.kind == TraversalGuidanceKind::Precision;
            riding = riding || view.kind == TraversalGuidanceKind::Riding;
        }
        diagnostics.observe(session);
        require(bytes == session.serialize(), "live guidance observation mutated world or queues");
    }
    require(parking && precision && riding, "live fixture missed parking, precision or riding");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected a diagnostics test name");
        const std::string_view name{argv[1]};
        if (name == "service_progress")
            service_progress();
        else if (name == "batch_transaction")
            batch_transaction();
        else if (name == "live_view")
            live_view();
        else if (name == "independent_alarms")
            independent_alarms();
        else if (name == "plan_churn")
            plan_churn();
        else if (name == "invalid_sample")
            invalid_sample();
        else if (name == "progress")
            progress();
        else if (name == "waiting")
            waiting();
        else if (name == "reset")
            reset();
        else if (name == "snapshot")
            snapshot();
        else if (name == "timings")
            timings();
        else if (name == "trace_format")
            trace_format();
        else if (name == "trace_invalid")
            trace_invalid();
        else if (name == "trace_capacity")
            trace_capacity();
        else if (name == "trace_observer")
            trace_observer();
        else
            throw std::invalid_argument("unknown diagnostics case");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
