#include "af/core/Profiling.h"

#include <chrono>
#include <cstdio>
#include <cstring>

namespace af {

double NowMs() {
    using Clock = std::chrono::steady_clock;
    return std::chrono::duration<double, std::milli>(Clock::now().time_since_epoch())
        .count();
}

Profiler& Profiler::Instance() {
    static Profiler profiler;
    return profiler;
}

void Profiler::Reset() {
    scopes_.clear();
    ring_.assign(RingCapacity, 0.0);
    ringNext_ = 0;
    ringCount_ = 0;
}

void Profiler::BeginFrame() {
    for (ScopeStat& s : scopes_) {
        s.ms = 0.0;
        s.calls = 0;
    }
}

void Profiler::EndFrame() {
    // Commit frame total: sum of all scope samples this frame.
    double total = 0.0;
    for (const ScopeStat& s : scopes_) total += s.ms;
    ring_[ringNext_] = total;
    ringNext_ = (ringNext_ + 1) % RingCapacity;
    if (ringCount_ < RingCapacity) ++ringCount_;
}

double Profiler::AverageFrameMs() const {
    if (ringCount_ == 0) return 0.0;
    double sum = 0.0;
    for (std::size_t i = 0; i < ringCount_; ++i) sum += ring_[i];
    return sum / static_cast<double>(ringCount_);
}

void Profiler::AddSample(const char* name, double ms) {
    for (ScopeStat& s : scopes_) {
        if (std::strcmp(s.name, name) == 0) {
            s.ms += ms;
            ++s.calls;
            return;
        }
    }
    ScopeStat slot;
    slot.name = name;
    slot.ms = ms;
    slot.calls = 1;
    scopes_.push_back(slot);
}

bool Profiler::DumpCSV(const char* path) const {
    std::FILE* f = std::fopen(path, "w");
    if (f == nullptr) return false;

    std::fprintf(f, "scope,ms_last_frame,calls_last_frame\n");
    for (const ScopeStat& s : scopes_) {
        std::fprintf(f, "%s,%.6f,%zu\n", s.name, s.ms, s.calls);
    }
    std::fprintf(f, "\nframe_index,frame_ms\n");
    for (std::size_t i = 0; i < ringCount_; ++i) {
        std::fprintf(f, "%zu,%.6f\n", i, ring_[i]);
    }
    std::fclose(f);
    return true;
}

ScopeTimer::ScopeTimer(const char* name) : name_(name), startMs_(NowMs()) {}

ScopeTimer::~ScopeTimer() {
    Profiler::Instance().AddSample(name_, NowMs() - startMs_);
}

}  // namespace af
