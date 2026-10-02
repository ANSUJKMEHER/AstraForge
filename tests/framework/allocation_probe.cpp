// Global operator new interception for the test binary. Overriding these
// functions in a single TU replaces allocation for the WHOLE binary; tests
// compare AllocationCount() deltas around measured regions. All deallocation
// overloads (unsized, sized, aligned) must free consistently with the
// allocation side — ASan caught a missing sized-delete here during Phase 2.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace {

std::atomic<long long> gAllocations{0};

void* AlignedAlloc(std::size_t size, std::size_t align) {
    void* raw = std::malloc(size + align + sizeof(void*));
    if (!raw) throw std::bad_alloc();
    std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(raw) + sizeof(void*);
    const std::uintptr_t aligned = (addr + align - 1) & ~(align - 1);
    reinterpret_cast<void**>(aligned)[-1] = raw;
    return reinterpret_cast<void*>(aligned);
}

void AlignedFree(void* p) {
    if (p) std::free(reinterpret_cast<void**>(p)[-1]);
}

void* Tracked(std::size_t size) {
    gAllocations.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(size)) return p;
    throw std::bad_alloc();
}

}  // namespace

void* operator new(std::size_t size) { return Tracked(size); }
void* operator new[](std::size_t size) { return Tracked(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

void* operator new(std::size_t size, std::align_val_t align) {
    gAllocations.fetch_add(1, std::memory_order_relaxed);
    return AlignedAlloc(size, static_cast<std::size_t>(align));
}
void* operator new[](std::size_t size, std::align_val_t align) {
    gAllocations.fetch_add(1, std::memory_order_relaxed);
    return AlignedAlloc(size, static_cast<std::size_t>(align));
}
void operator delete(void* p, std::align_val_t) noexcept { AlignedFree(p); }
void operator delete[](void* p, std::align_val_t) noexcept { AlignedFree(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { AlignedFree(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { AlignedFree(p); }

namespace af::test {

long long AllocationCount() { return gAllocations.load(std::memory_order_relaxed); }

}  // namespace af::test
