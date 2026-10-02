#pragma once
// Profiler — RAII scope timers + frame-time ring buffer (Phase 9).
//
// Headless-friendly: accumulates per-scope milliseconds each frame, keeps a
// fixed-capacity ring buffer of frame times, and can dump a CSV the app's
// dev panel (or any tool) can graph. No platform dependencies.
//
// Steady state allocates nothing: scope slots are registered on first use
// (bounded by the distinct timer names in the code), and the ring buffer is
// fixed-size. Verification: tests/core/test_profiling.cpp.
//
// Usage:
//   Profiler& prof = Profiler::Instance();
//   prof.Reset();                       // per-run
//   for (frames) {
//       prof.BeginFrame();
//       { ScopeTimer t("physics"); ... }
//       prof.EndFrame();
//   }
//   prof.DumpCSV("frame.csv");
//
// ScopeTimer resolves its slot by name (linear scan — scope counts are
// single-digit). Timers are disabled entirely when the profiler is off, so
// gameplay code can instrument liberally without steady-state cost.

#include <cstddef>
#include <vector>

namespace af {

class ScopeTimer;

class Profiler {
public:
    static Profiler& Instance();

    void Reset();  // clears slots + ring buffer (allocates: one-time setup)

    void BeginFrame();  // zeroes per-frame accumulators
    void EndFrame();    // commits this frame's total into the ring buffer

    struct ScopeStat {
        const char* name = nullptr;
        double ms = 0.0;
        std::size_t calls = 0;
    };
    const std::vector<ScopeStat>& LastFrameScopes() const { return scopes_; }
    const std::vector<double>& FrameTimes() const { return ring_; }
    double AverageFrameMs() const;

    // Writes "scope,ms,calls" rows + frame-time column for the last run.
    // Returns false on I/O failure.
    bool DumpCSV(const char* path) const;

private:
    friend class ScopeTimer;
    Profiler() = default;

    // Adds a sample to the named scope (creates the slot on first use).
    void AddSample(const char* name, double ms);

    std::vector<ScopeStat> scopes_;
    std::vector<double> ring_;  // fixed capacity, circular
    std::size_t ringNext_ = 0;
    std::size_t ringCount_ = 0;
    static constexpr std::size_t RingCapacity = 1024;
};

class ScopeTimer {
public:
    explicit ScopeTimer(const char* name);
    ~ScopeTimer();
    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;

private:
    const char* name_;
    double startMs_ = 0.0;
};

// Monotonic milliseconds since an arbitrary epoch (steady clock).
double NowMs();

}  // namespace af
