# Technical decisions

Every significant design decision in AstraForge, with the reasoning and — where
applicable — the measurement that backs it. Numbering continues from
`context.md` → Decisions (1–11); the items below are the gameplay and
performance-phase decisions (12+).

---

## Decision 12 — Gameplay architecture: a GameSession orchestrator, headless-first

The arena game (player, enemy FSM, pooled projectiles, score, death/restart)
is a `GameSession` — a World + ordered SystemManager + projectile pool +
scripted `PlayerInput` — built headless-first. The SDL2 app is a thin shell:
it maps SDL input to `PlayerInput` and syncs `Renderable` components onto
gameplay entities. The session itself never touches SDL or OpenGL.

Why: the whole game — damage, FSM transitions, restart, score, determinism,
allocation behavior — is then testable in CI without a GPU (15+ test cases in
`tests/game/`). The render-free core also keeps the dependency story intact:
SDL2 remains the only external library.

## Decision 13 — Projectiles: fixed-capacity pool of parked entities

Projectiles are pooled: the pool pre-creates `capacity` bare entities; Spawn
adds Transform+Projectile, Despawn removes them and parks the entity. The
pool never calls DestroyEntity/CreateEntity in steady state, and `Spawn`
returns `Entity::Null()` when exhausted — callers drop the shot.

Why: spawn/despawn churn is the hottest structural path in the game.
Measured: 1000 spawn/despawn cycles allocate **0 bytes** (verified by the
allocation probe), and pool reuse recycles the same handles. The generation
check on `Entity` handles (Phase 2) would make even destroy-based pooling
safe — but parking is simpler and measurably free.

## Decision 14 — Combat narrowphase is O(P·E), not grid-based

`CombatSystem` tests every projectile against every enemy with a
Sphere↔Sphere test. No broadphase.

Why (measured): projectile counts are bounded by the pool (64) and enemies
by the spawner cap (6 in gameplay; thousands in the stress benchmark). The
measured combat cost is **<0.01 ms/frame even at 20k enemies** (see
`docs/performance.md` §1) — the grid broadphase is reserved for the body
population where it pays (collision, 82% of stress-frame cost). Revisit only
if gameplay moves to bullet-hell densities.

## Decision 15 — Static colliders (walls) live outside the grid

Arena walls are static AABB colliders. `CollisionSystem` splits the gather
step: dynamic colliders go through the uniform grid; static ones are stored
separately and tested directly against each dynamic collider (O(n·m), m=4
walls).

Why: walls span the whole arena, so their extents would inflate the grid's
query radius to arena size and degenerate the broadphase to O(n²). A direct
static pass keeps grid inflation tight (largest *dynamic* extent only) and
makes the wall response trivially one-sided (full push on the dynamic body,
zero on the static). Verified by dedicated tests, including the
sphere-center-inside-wall case where the narrowphase normal points OUT of the
nearest face.

## Decision 16 — Death/restart as session policy, not a system

When the player's health reaches zero, `GameSession` counts down
`restartDelay`, then resets: enemies destroyed, projectiles parked, player
respawned at center, score zeroed, spawner re-seeded. It is not a System.

Why: restart spans the whole world state (every gameplay system's data),
which is session policy, not per-frame simulation. Keeping it out of the
system loop means no system has a hidden world-reset side effect, and the
restart path is exercised directly by tests (`test_game_session.cpp`).

## Decision 17 — Determinism as a first-class property

The game has exactly one RNG (the spawner's LCG, seeded at construction and
re-seeded on restart). Everything else — scripted input, system order,
pooled-entity reuse, contact resolution order — is deterministic by
construction. The twin-session test runs two sessions with identical input
for 420 steps and requires bit-identical player positions, scores, and
entity counts.

Why: reproducibility is what makes the benchmark numbers and the headless CI
tests meaningful. It also makes the game logic auditable frame-by-frame in an
interview.

## Decision 18 — Concurrency: no job system (measured)

**Question:** does AstraForge need a job system / multithreaded systems?

**Data (sandbox, 2 vCPU, -O3; see docs/performance.md):**

- Full gameplay frame @ 20,000 enemies: **3.6 ms** — 4.6× inside the 60 Hz
  budget, and target gameplay runs ≤ dozens of entities (µs range).
- The dominant cost (collision, 82%) is the grid rebuild + queries, which is
  memory-bandwidth-bound and has a strict rebuild→query dependency within a
  frame.

**Analysis:** the workloads are either too small to amortize thread
scheduling (µs-scale systems), or sequential by design (fixed-order systems
with structural dependencies: spawner → physics → collision → combat →
score). The realistic parallel split (per-system task parallelism) would
yield at best ~1.8× on 2 cores for the collision system alone — and 3.6 ms
already meets budget 4.6× over.

**Decision:** no job system. Single-threaded, ordered systems meet the frame
budget with headroom at 20× the game's entity count. Synchronization
complexity, cache-line ping-pong on shared sparse sets, and loss of
bit-determinism (a core testable property, Decision 17) are real costs with
no measurable payoff at this scale.

**Revisit trigger (written down so the decision stays falsifiable):** if
laptop measurements (Phase 11) show a sustained frame > 8 ms at target
entity counts, or if gameplay grows beyond ~50k entities, prototype a
two-worker split (AI/physics || collision) with per-system chunking, and
re-evaluate with the same benchmark harness.

## Decision 19 — Dev UI: console stats + profiler CSV, no ImGui (deferred)

The windowed app prints FPS/draw-call/culling/score/entity stats per second;
the profiler dumps per-scope and frame-time CSVs. An ImGui panel was
deferred.

Why: an ImGui panel cannot be runtime-verified in the development sandbox
(no GPU/display), and it would add unverifiable UI code plus an extra
vendored dependency (though MIT-licensed). The console stats + CSV deliver
the same diagnostic value with everything verifiable. Revisit after the
Windows run (Phase 11) if the demo video needs an on-screen panel.

## Decision 20 — Arena density in the stress benchmark

`af_bench_gameplay` scales the arena with the injected population (constant
~0.11 enemies/m²). A fixed 40×40 arena with 5k entities becomes an overlap
soup that measures narrowphase saturation, not system scaling.

Why: the benchmark's job is to answer "how does the engine scale with
population", not "how pathological can we make the arena". The fixed-arena
regime is still available by passing a small count; the scaling table uses
constant density, disclosed in `docs/performance.md`.

---

## Reference: decisions 1–11 (implementation phase)

Summarized in `context.md` → Decisions: own math library (no GLM);
sparse-set ECS; uniform-grid broadphase with center-anchored cells + inflated
queries; fixed-timestep loop; own test framework; headless simulation mode;
SDL2-only external dependencies; concurrency deferred pending measurements
(now resolved by Decision 18); forward renderer; own GL loader; grid
membership redesign (multi-cell corruption bug).
