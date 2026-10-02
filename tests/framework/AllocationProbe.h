#pragma once
// Shared allocation probe: a global operator new counter for the test binary.
// The overrides themselves live in allocation_probe.cpp (exactly one TU).
// Tests measure deltas: AllocationCount() before/after a region.

namespace af::test {

long long AllocationCount();

}  // namespace af::test
