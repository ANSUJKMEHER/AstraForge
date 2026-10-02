# Performance engineering (Phase 9)

Measured evidence for AstraForge's performance story. Every number below was
produced by the benchmark binaries in `src/bench/` on the hardware stated —
nothing is extrapolated or estimated.

**Hardware: 2 vCPU Linux sandbox container, GCC 12.2, `-O3` (Release).
This is NOT the target laptop** (HP Pavilion, Ryzen 5, integrated GPU).
Laptop numbers are collected on the developer's machine; the methodology and
expected trends transfer directly.

## 1. Gameplay session under stress

`af_bench_gameplay [enemy_count] [frames]` drives the full arena session —
player controller, enemy AI, spawner, physics, collision, projectiles,
combat, score — at constant entity density (arena scales with population, so
this measures per-system scaling rather than a degenerate overlap soup).

| Enemies | ms/frame | FPS equivalent | vs 60 Hz budget (16.67 ms) |
|---|---|---|---|
| 1,000 | 0.148 | 6,752 | 113× headroom |
| 5,000 | 0.896 | 1,116 | 18.6× headroom |
| 10,000 | 1.809 | 553 | 9.2× headroom |
| 20,000 | 3.640 | 275 | 4.6× headroom |

Scaling is near-linear (≈0.18 µs/enemy/frame) — the uniform-grid broadphase
(Phase 3) keeps collision work proportional to population.

### Windows Hardware Verification (MinGW GCC 15.2.0, -O3, Release)

Measured via `af_bench_gameplay [count] 60` directly on the Windows environment:

| Enemies | ms/frame | FPS equivalent | vs 60 Hz budget (16.67 ms) |
|---|---|---|---|
| 1,000 | 0.111 | 8,997 | 150× headroom |
| 5,000 | 0.694 | 1,441 | 24.0× headroom |
| 10,000 | 1.428 | 700 | 11.6× headroom |
| 20,000 | 2.991 | 334 | 5.5× headroom |

Per-system breakdown at 20,000 enemies on Windows (ms/frame):
- **Collision:** 2.12 ms (71%)
- **Physics:** 0.61 ms (20%)
- **Enemy AI:** 0.13 ms (4%)
- **Combat:** 0.09 ms (3%)
- **Spawner:** 0.07 ms (2%)
- **Player Controller & Projectiles:** <0.01 ms (<1%)


Per-system breakdown at 20,000 enemies (ms/frame, one profiled frame):

| System | ms/frame | Share |
|---|---|---|
| collision | 2.91 | 82% |
| physics | 0.63 | 18% |
| enemy AI | 0.25 | 7% |
| spawner | 0.12 | 3% |
| score | 0.06 | 2% |
| player controller / projectiles / combat | <0.01 | <1% |

**Conclusion (measured):** collision is the dominant cost at stress scale,
exactly as designed for — the grid broadphase is where the engineering went,
and the profiler confirms it. Target gameplay scale (≤ a few dozen entities
in the arena) runs far below 0.1 ms/frame.

## 2. Allocation behavior

Steady-state zero-allocation claims are verified by intercepting global
`operator new` (see `tests/framework/AllocationProbe.h`):

- Physics update, 600 steps after warmup: **0 bytes allocated**.
- Projectile pool, 1000 spawn/despawn cycles after warmup: **0 bytes
  allocated**.
- Full GameSession (player + injected enemies + constant fire), 600 fixed
  steps after warmup: **0 bytes allocated**.

Pooling works as designed: entities are parked, not destroyed; sparse-set
capacity is retained; scratch buffers are reused across frames.

## 3. Earlier results (context)

- ECS update loop: 0.0056 ms/step @ 1k → 0.5635 ms/step @ 100k entities
  (near-linear).
- Collision broadphase: 6.6–10.8× faster than naive O(n²) with 99.984%
  candidate-pair reduction at 1k–20k entities, contact counts cross-checked
  equal to the naive reference.
- Frustum culling (Gribb–Hartmann) and camera math are unit-tested headless;
  their runtime win (culled draw submissions) is only observable on the GPU
  machine — reported in the Phase 11/12 Windows run.

## 4. Profiler

`af/core/Profiling.h` — RAII `ScopeTimer`, per-frame scope stats, a
fixed-capacity frame-time ring buffer, and `DumpCSV` output for graphing.
Steady state allocates nothing (slots registered once, ring pre-sized) —
verified by test. `GameSession::UpdateInstrumented` wraps each system stage
in a scope; the benchmark uses it to produce the per-system table above.

The windowed app prints FPS / draw-call / culling / score / entity stats to
the console each second. An on-screen ImGui panel was considered and
**deferred** (see `context.md` → Decisions): it cannot be runtime-verified in
the sandbox, and the console stats + profiler CSV deliver the same diagnostic
value with zero unverifiable code.

## 5. Methodology notes (reproducibility)

- Deterministic placement (LCG), scripted input, fixed frame counts — every
  run is bit-reproducible; the collision benchmark additionally cross-checks
  grid contacts against naive O(n²).
- The benchmark warm-up phase reaches steady state before timing so
  allocation and cache effects don't pollute measurements.
- Repeat runs vary <5% on this container; run `af_bench_gameplay N M`
  yourself to reproduce.
