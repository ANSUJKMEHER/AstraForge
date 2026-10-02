#pragma once
// Component type registry: each component type gets a small id on first use,
// and a bit in a 64-bit signature mask.

#include <cstdint>

namespace af {

using ComponentId = uint8_t;
using ComponentMask = uint64_t;

inline constexpr std::size_t MaxComponentTypes = 64;

inline ComponentId& NextComponentIdCounter() {
    static ComponentId counter = 0;
    return counter;
}

// Inline function template: every translation unit in the binary agrees on the
// id for a given T (the function-local static is merged at link time).
// Ids are process-internal; nothing is serialized. Requires < 64 distinct
// component types (documented limit; the engine uses ~15).
template <typename T>
ComponentId ComponentIdOf() {
    static const ComponentId id = NextComponentIdCounter()++;
    return id;
}

template <typename T>
ComponentMask ComponentMaskOf() {
    return ComponentMask{1} << ComponentIdOf<T>();
}

}  // namespace af
