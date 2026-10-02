#pragma once
// AF_ASSERT — always-on assertion (even in Release builds).
//
// Deliberate choice for a learning/portfolio engine: the dominant cost here is
// correctness and explainability, not the last 0.1% of speed. The guarded
// conditions are cheap (pointer/index checks), and a clear crash beats silent
// undefined behavior. Standard <cassert> (NDEBUG-sensitive) behavior was
// rejected precisely because it silently disables safety in the build types
// we ship by default (RelWithDebInfo defines NDEBUG).

#include <cstdio>
#include <cstdlib>

#define AF_ASSERT(cond, message)                                        \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::fprintf(stderr,                                        \
                         "AstraForge ASSERT failed: %s\n  at %s:%d\n  %s\n", \
                         #cond, __FILE__, __LINE__, message);           \
            std::abort();                                               \
        }                                                               \
    } while (0)
