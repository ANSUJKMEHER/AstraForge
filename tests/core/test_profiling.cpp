#include "af/core/Profiling.h"

#include <cstdio>
#include <string_view>
#include <thread>

#include "framework/AllocationProbe.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

// Sleeps ~2 ms so scope samples are measurable (wall-clock granularity in
// sandboxes is coarse; sleeps are more reliable than busy-spins).
void BusyWait() { std::this_thread::sleep_for(std::chrono::milliseconds(2)); }

}  // namespace

AF_TEST("profiler records named scope samples and call counts") {
    Profiler& prof = Profiler::Instance();
    prof.Reset();
    prof.BeginFrame();

    {
        ScopeTimer a("scope_a");
        BusyWait();
    }
    {
        ScopeTimer a("scope_a");
        BusyWait();
    }
    {
        ScopeTimer b("scope_b");
        BusyWait();
    }
    prof.EndFrame();

    const auto& scopes = prof.LastFrameScopes();
    AF_CHECK_EQ(scopes.size(), 2u);
    bool foundA = false, foundB = false;
    for (const auto& s : scopes) {
        if (s.name == std::string_view{"scope_a"}) {
            foundA = true;
            AF_CHECK_EQ(s.calls, 2u);
            AF_CHECK(s.ms > 0.0);  // two ~2 ms samples
        } else if (s.name == std::string_view{"scope_b"}) {
            foundB = true;
            AF_CHECK_EQ(s.calls, 1u);
            AF_CHECK(s.ms > 0.0);
        }
    }
    AF_CHECK(foundA);
    AF_CHECK(foundB);
}

AF_TEST("profiler frame ring buffer accumulates frame times") {
    Profiler& prof = Profiler::Instance();
    prof.Reset();

    for (int i = 0; i < 5; ++i) {
        prof.BeginFrame();
        {
            ScopeTimer t("work");
            BusyWait();
        }  // timer must stop before EndFrame commits the frame
        prof.EndFrame();
    }

    const auto& times = prof.FrameTimes();
    double sum = 0.0;
    for (int i = 0; i < 5; ++i) sum += times[static_cast<std::size_t>(i)];
    // Loose bound: sleep granularity varies; frames must simply be non-zero.
    AF_CHECK(sum > 0.0);
    const double avg = prof.AverageFrameMs();
    AF_CHECK(avg > 0.0 && avg < 50.0);
}

AF_TEST("profiler begin_frame zeroes per-frame accumulators") {
    Profiler& prof = Profiler::Instance();
    prof.Reset();

    prof.BeginFrame();
    {
        ScopeTimer t("work");
        BusyWait();
    }
    prof.EndFrame();

    prof.BeginFrame();  // no timers this frame
    prof.EndFrame();

    for (const auto& s : prof.LastFrameScopes()) {
        AF_CHECK_EQ(s.calls, 0u);
        AF_CHECK_NEAR(s.ms, 0.0, 1e-9);
    }
}

AF_TEST("profiler steady state allocates zero bytes") {
    Profiler& prof = Profiler::Instance();
    prof.Reset();

    // Warm up: register scopes + first ring writes.
    for (int i = 0; i < 10; ++i) {
        prof.BeginFrame();
        {
            ScopeTimer a("steady_a");
            ScopeTimer b("steady_b");
        }
        prof.EndFrame();
    }

    const long long before = af::test::AllocationCount();
    for (int i = 0; i < 500; ++i) {
        prof.BeginFrame();
        {
            ScopeTimer a("steady_a");
            ScopeTimer b("steady_b");
        }
        prof.EndFrame();
    }
    const long long after = af::test::AllocationCount();
    AF_CHECK_EQ(after, before);
}

AF_TEST("profiler dumps a CSV with scope and frame rows") {
    Profiler& prof = Profiler::Instance();
    prof.Reset();
    for (int i = 0; i < 3; ++i) {
        prof.BeginFrame();
        ScopeTimer t("csv_scope");
        prof.EndFrame();
    }

    const char* path = "profiler_test_out.csv";
    AF_CHECK(prof.DumpCSV(path));

    std::FILE* f = std::fopen(path, "r");
    AF_CHECK(f != nullptr);
    if (f != nullptr) {
        char buf[256];
        std::size_t lines = 0;
        bool sawScope = false;
        bool sawFrame = false;
        while (std::fgets(buf, sizeof(buf), f) != nullptr) {
            ++lines;
            if (buf[0] == 'c') sawScope = true;   // csv_scope row
            if (buf[0] >= '0' && buf[0] <= '9') sawFrame = true;  // frame rows
        }
        std::fclose(f);
        std::remove(path);
        AF_CHECK(lines >= 7);  // header + blank + scope row + header + 3 frames
        AF_CHECK(sawScope);
        AF_CHECK(sawFrame);
    }
}
