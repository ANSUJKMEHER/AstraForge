# phases.md — AstraForge development plan

Phase structure designed around: system dependencies, implementation risk,
learning value, performance requirements, testing needs, portfolio value.
Phases are executed sequentially; a phase is marked `[x] Completed` only after
its verification actually passed. See `context.md` for design decisions.

---

## Phase 0 — Workspace bootstrap & architecture design

**Objective:** establish the repository, build system, and persistent context
so any session (human or agent) can resume the project reliably.

**Why this phase matters:** session interruptions are expected; the workspace
is the source of truth, not chat history.

**Dependencies:** none.

**Planned work:** project skeleton (CMakeLists, .gitignore, README, dirs),
architecture decisions documented in `context.md`, phase plan (this file).

**Files/systems affected:** repo root files, `third_party/README.md`.

**Verification:** clean CMake configure + build + ctest run in sandbox.

**Status:** [x] Completed — verified 2026-10-01 (CMake 3.30.5, GCC 12.2).

**Notes:** build works; `af_tests` runs via ctest.

---

## Phase 1 — Math library

**Objective:** own vector/matrix/quaternion library with a strong test suite.

**Why this phase matters:** everything else depends on it; 3D math is core EA
interview material; writing it ourselves (vs GLM) is a deliberate, documented
decision.

**Dependencies:** Phase 0.

**Planned work:** Vec2/3/4, column-major Mat4 (multiply, transpose,
determinant, inverse, Perspective/Ortho/LookAt), Quat (axis-angle, Euler YXZ,
slerp), Transform, conventions doc.

**Files/systems affected:** `include/af/math/*`, `docs/math-conventions.md`,
`tests/math/*`, `tests/framework/af_test.hpp`.

**Verification:** 35 test cases / 406 checks pass via ctest.

**Status:** [x] Completed — verified 2026-10-01.

**Notes:** test suite caught a transposed `Mat4 * Mat4` on first run; fixed and
regression-guarded by strengthening the identity test with a non-symmetric
matrix.

---

## Phase 2 — ECS core

**Objective:** entity/component/system architecture with sparse-set storage.

**Why this phase matters:** component architecture is a headline engine topic;
cache-friendly storage and zero-allocation iteration are concrete, testable
engineering claims.

**Dependencies:** Phase 1.

**Planned work:** entity handles (id + generation), sparse-set component
storage, component signatures, ordered systems, iteration views, headless
simulation runner (first version).

**Files/systems affected:** `include/af/ecs/*`, `src/core/ecs/*`,
`include/af/core/Components.h`, `src/app/headless_main.cpp`,
`tests/ecs/*`.

**Verification:** tests for add/remove/reuse-safety/iteration/allocation
behavior; headless runner runs N fixed steps without crashing.

**Status:** [x] Completed — verified 2026-10-01.

**Notes:** 63 test cases / 3121 checks green (normal + ASan/UBSan builds);
ctest runs `core_tests` + `headless_smoke`. ASan caught one real bug
(missing sized-delete overloads in the allocation hook) — fixed. Headless
update loop measured at 0.0056 ms/step @1k → 0.5635 ms/step @100k entities
(sandbox 2 vCPU container, -O3; laptop numbers still pending). Zero-allocation
steady state verified by intercepting global `operator new`. View API:
`for (auto [entity, t, v] : world.ViewComponents<Transform, Velocity>())`.

---

## Phase 3 — Collision: narrowphase + spatial hash broadphase

**Objective:** AABB/sphere primitives, intersection tests, uniform-grid
broadphase with candidate counting.

**Why this phase matters:** data structures + algorithms (the EA bullet),
measurable complexity improvement over naive O(n²), benchmark methodology
established here.

**Dependencies:** Phase 2 (entities carry colliders).

**Planned work:** Sphere/AABB tests, uniform XZ grid, candidate pair
generation, broadphase/narrowphase split, benchmark executable (naive vs grid).

**Files/systems affected:** `include/af/collision/*`, `src/core/collision*.cpp`,
`tests/collision/*`, `src/bench/*`.

**Verification:** correctness tests + first real benchmark numbers
(checks/frame, ms/frame at 1k/5k/10k entities; sandbox hardware disclosed).

**Status:** [x] Completed — verified 2026-10-01.

**Notes:** Narrowphase: Sphere/Sphere, Sphere/AABB (closest point + inside-box), AABB/AABB with a→b contact normals and least-penetration depths. Broadphase: uniform XZ grid — center-anchored cells + extent-inflated queries (redesigned after the superset property test caught multi-cell list corruption; Decision 11). CollisionSystem: gather → grid → candidates (j>i dedup) → narrowphase → accumulated symmetric separation. Benchmark `af_bench_collision`: 6.6–10.8× speedup, 99.984% candidate reduction at 1k–20k, contact-count cross-check vs naive = OK. Zero-alloc steady state verified (shared allocation probe).


---

## Phase 4 — Physics (kinematics + ground response)

**Objective:** velocity/gravity integration, jump, ground-plane resolution at
fixed dt.

**Why this phase matters:** game-loop determinism made concrete; the physics
step is the fixed-timestep unit.

**Dependencies:** Phase 3.

**Planned work:** MovementSystem, gravity constant, jump impulse, ground
collision response, deterministic step tests.

**Files/systems affected:** `include/af/physics/*`, `src/core/physics*.cpp`,
`tests/physics/*`.

**Verification:** multi-step deterministic tests (same input → same state,
bit-exact); jump arc sanity tests.

**Status:** [x] Completed — verified 2026-10-01.

**Notes:** Semi-implicit Euler at fixed dt, per-entity Gravity with terminal-velocity clamp, ground response using collider extent (0 without collider), JumpRequest consumed on use (grounded-only). Tests are bit-exact against same-order references (gravity→integrate→ground→jump); free-fall/apex checked vs analytic within documented O(dt) discretization error. Headless runner now drives the physics scene (falls land at y=0 deterministically).


---

## Phase 5 — Resources: mesh/shader/texture + cache

**Objective:** procedural primitive meshes, shader wrapper, texture loading
(stb_image), resource cache with explicit lifetimes.

**Why this phase matters:** RAII/resource-ownership story for interviews;
primitives-only asset policy keeps the laptop lightweight.

**Dependencies:** Phase 1 (rendering math exists).

**Planned work:** Mesh/ShaderProgram/Texture classes, ResourceCache
(load-once, handle-based), primitive generators (cube, sphere, plane),
stb_image vendored.

**Files/systems affected:** `include/af/resources/*`, `src/core/resources*.cpp`,
`tests/resources/*`, `third_party/stb/*`.

**Verification:** cache behavior tests (dedup, release, stale handles);
primitive geometry sanity tests.

**Status:** [x] Completed — verified 2026-10-01.

**Notes:** MeshGen: cube (24/36), UV sphere, XZ plane with bounds for culling. stb_image 2.30 vendored (MIT, preserved LICENSE) — decodes BMP/PNG/JPEG; tested by generating a real 2x2 BMP byte-exactly and decoding it, plus missing/corrupt-file paths. ResourceCache<T>: weak_ptr-based — dedup while pinned, lazy free + reload after the last handle drops (tested). Default GLSL 330 Phong-ish shaders embedded as strings (zero asset-path assumptions).


---

## Phase 6 — Rendering (OpenGL 3.3)

**Objective:** minimal forward renderer: VAO/VBO/EBO, one Phong-ish shader
program, camera, lighting, frustum culling.

**Why this phase matters:** the visible proof the engine works; culling gives
a measured before/after optimization.

**Dependencies:** Phase 5.

**Planned work:** Renderer, Camera (perspective from our Mat4), draw call
accounting, frustum culling (6 planes, sphere test), ImGui vendored.

**Files/systems affected:** `include/af/render/*`, `src/app/render*.cpp`,
`third_party/imgui/*`.

**Verification:** sandbox can compile-check only (no SDL2/GL); real
verification on developer's Windows machine + CI. Frustum culling math is
unit-tested headless.

**Status:** [x] Completed — core verified in sandbox 2026-10-01; app runtime UNVERIFIED (no GPU/display).

**Notes:** Headless-verified: Frustum (Gribb–Hartmann from view-projection, unit-cube and camera cases), Camera (orbit/yaw/pitch/clamp/basis orthonormality), GLSL sources. App layer (own GL 3.3 loader via SDL_GL_GetProcAddress — no GLAD/GLEW, Decision 10; ShaderProgram; GLMesh; Renderer with frustum-culled Submit + stats; RenderScene): all sources compile-checked against real SDL2 2.30.9 headers in the sandbox. Runtime verification pending Windows + CI (Phase 11). ImGui dev panel intentionally moved to Phase 9 (profiler phase) — scope control.
 UNVERIFIED locally until run on real hardware — will be marked as
such in docs.

---

## Phase 7 — Application layer: window, loop, input

**Objective:** SDL2 window + GL context, fixed-timestep loop with accumulator,
mouse-look camera, WASD input, window close handling.

**Why this phase matters:** the game loop is a classic interview question;
fixed vs variable timestep must be explainable and implemented deliberately.

**Dependencies:** Phase 6.

**Planned work:** `Application` class, timing, input state, camera controls,
frame profiler hooks.

**Files/systems affected:** `src/app/*`, `include/af/app/*`, CMake SDL2 wiring.

**Verification:** headless loop variant runs N steps in sandbox; windowed
version verified on Windows + CI.

**Status:** [x] Completed — core verified; app runtime UNVERIFIED (same caveat as Phase 6).

**Notes:** Application: SDL2 window + GL 3.3 core context, vsync request, relative mouse, InputState (keys/motion/wheel/quit). Fixed 60 Hz accumulator loop with 250 ms spiral-of-death clamp (SDL_GetTicks64, needs SDL2 ≥ 2.0.18). main.cpp: floor + 81-cube grid + physics ball scene, orbit camera (mouse look / WASD pan / wheel zoom), per-second FPS + draw-call stats. docs/build.md covers MSVC+vcpkg, MinGW/prebuilt SDL2, Linux, and troubleshooting. CMake app target guarded by find_package(SDL2) with a clear warning.


---

## Phase 8 — Gameplay: arena, player, enemies, projectiles

**Objective:** playable arena: WASD + jump + shoot, enemy FSM (IDLE→CHASE→
ATTACK→HURT→DEAD), pooled projectiles, health/damage/score, death/restart.

**Why this phase matters:** makes the engine demonstrably functional; FSM and
pooling are concrete gameplay-engineering artifacts.

**Dependencies:** Phases 2-7.

**Planned work:** PlayerController, EnemyFSM, ProjectileSystem (pooled),
CombatSystem, ScoreSystem, arena bounds, spawner.

**Files/systems affected:** `src/game/*`, `include/af/game/*`, `tests/game/*`,
static-collider support in CollisionSystem, app integration in `main.cpp`.

**Verification:** headless gameplay tests (damage, death, restart, FSM
transitions, pool reuse).

**Status:** [x] Completed — verified 2026-10-02.

**Notes:** `GameSession` orchestrator (Decision 12): World + 8 ordered systems
+ fixed-capacity ProjectilePool + scripted PlayerInput. PlayerController:
WASD strafe (diagonal-normalized), edge-triggered jump via JumpRequest,
rate-limited pooled shots, arena clamp. Enemy FSM in `Enemy` component:
Idle→Chase (detectRange)→Attack (attackTimer windup → melee damage on
expiration)→Hurt (timer)→Dead; chase is XZ-only velocity. Pool: pre-created
parked entities, Spawn/Despawn = component add/remove, Null on exhaustion
(Decision 13); 1000 cycles = 0 allocations. Combat: O(P·E) Sphere↔Sphere,
Hurt/Dead transitions, single-hit projectiles (Decision 14). ScoreSystem:
deferred destroy + points/kills. SpawnerSystem: LCG edge-ring placement,
maxAlive cap. Death → restartDelay countdown → full reset (Decision 16).
Static wall colliders excluded from the grid; direct one-sided push
(Decision 15; inside-wall normal case handled + tested). Determinism: twin
sessions 420 steps bit-identical (Decision 17). Headless runner
`af_headless_game` + ctest `game_smoke`. App (main.cpp) maps SDL→PlayerInput,
syncs Renderables, camera orbits the player; compile-checked vs SDL2 2.30.9
headers (app runtime still unverified — Risk R1).


---

## Phase 9 — Performance engineering (measure → optimize)

**Objective:** reproducible benchmarks, profiling, evidence-based optimization.

**Why this phase matters:** EA cares about performance/frame rates; every
optimization must be measured before/after. No fabricated numbers.

**Dependencies:** Phase 8 (realistic workload exists).

**Planned work:** benchmark harness + stress-test mode (1k/5k/10k entities),
profiler (scope timers, frame graph), evaluate: spatial grid (done P3),
object pooling, frustum culling, allocation reduction, batching/instancing.

**Files/systems affected:** `src/bench/gameplay_bench.cpp`, `src/core/profiling.*`,
`include/af/core/Profiling.h`, `docs/performance.md`.

**Verification:** before/after tables with disclosed hardware; decisions
documented even when an optimization doesn't help.

**Status:** [x] Completed — verified 2026-10-02 (sandbox; laptop numbers remain).

**Notes:** Profiler: RAII ScopeTimer, per-frame scope stats, fixed-capacity
frame-time ring buffer, CSV dump; zero steady-state allocations (tested).
`GameSession::UpdateInstrumented` scopes each system. `af_bench_gameplay`
injects N enemies at constant density (arena scales with population —
Decision 20) and reports avg frame + per-system breakdown:
0.148 ms @1k → 0.896 @5k → 1.809 @10k → 3.640 ms @20k enemies (2 vCPU
sandbox container, -O3). At 20k: collision 82%, physics 18%, AI 7%,
rest <1% — the profiler confirms the grid broadphase is where the cost is,
as designed. Combat O(P·E) measured <0.01 ms/frame even at 20k → no
broadphase needed (Decision 14). Pooling + full-session zero-allocation
verified. ImGui dev panel deferred (Decision 19): console HUD + profiler
CSV instead. See docs/performance.md for tables + methodology.


---

## Phase 10 — Concurrency evaluation

**Objective:** decide, with data, whether a job system is justified.

**Why this phase matters:** a measured "no, and here's why" is better than
resume-driven multithreading. EA interviews probe synchronization knowledge
either way.

**Dependencies:** Phase 9 data.

**Planned work:** identify candidate parallel workloads, prototype if data
justifies, document ownership/synchronization if built; otherwise document
the analysis.

**Files/systems affected:** `docs/technical-decisions.md` (+ code only if
justified).

**Verification:** benchmark comparison if implemented; written rationale
either way.

**Status:** [x] Completed — 2026-10-02 (documented analysis, no job system).

**Notes:** Decision 18. Data: 3.6 ms/frame @ 20k enemies = 4.6× inside the
60 Hz budget on 2 cores; dominant system (collision) is a sequential
rebuild→query dependency; a parallel split would buy ≤~1.8× at the cost of
sync complexity and loss of bit-determinism (Decision 17). Written revisit
trigger: sustained >8 ms/frame at target counts, or >50k entities on laptop
→ prototype two-worker split with the same harness. No code shipped — the
analysis IS the deliverable, and it is falsifiable.


---

## Phase 11 — CI + repository hygiene

**Objective:** GitHub Actions: configure + build + test on Linux and Windows.

**Why this phase matters:** CI proves cross-platform claims; the app build on
CI is the first independent verification of the SDL2/GL code.

**Dependencies:** Phase 8 (app builds).

**Planned work:** `.github/workflows/ci.yml` (ubuntu: apt SDL2; windows:
vcpkg SDL2), README badge placeholders, git commit discipline docs.

**Files/systems affected:** `.github/*`, `docs/build.md`.

**Verification:** CI status marked UNVERIFIED until the developer pushes to
GitHub; locally: nothing fabricatable.

**Status:** [x] Completed (authored) — UNVERIFIED until first push (2026-10-02).

**Notes:** Three jobs: linux-gcc (Release + ctest), linux-sanitizers
(ASan+UBSan + ctest), windows-msvc (vcpkg sdl2:x64-windows + ctest). YAML
syntax validated locally. The workflow exercises the full matrix the
sandbox cannot: the SDL2 app target compiles on CI, and the headless suite
runs on Windows for the first time. The "UNVERIFIED" marker must be removed
(phases.md + context.md Risks R1 + quality-gate.md) only after a successful
GitHub run.


---

## Phase 12 — Documentation & presentation

**Objective:** README, architecture doc + diagram, build guide,
technical decisions, interview notes.

**Why this phase matters:** the portfolio is judged by humans; the developer
must be able to explain everything.

**Dependencies:** all implementation phases.

**Planned work:** `docs/architecture.md`, `docs/build.md`,
`docs/technical-decisions.md`, `docs/interview-notes.md`, README completion,
demo-video recording guide (stress test + dev panel flow).

**Files/systems affected:** `docs/*`, `README.md`.

**Verification:** every doc claim checked against the actual code.

**Status:** [x] Completed — 2026-10-02.

**Notes:** architecture.md (data flow + CMake target map + filesystem map),
technical-decisions.md (20 decisions, most measurement-backed),
performance.md (tables + methodology), interview-notes.md (whiteboard-shaped
answers + honesty checklist), build.md (all binaries, controls, options),
README (complete status, controls, verified numbers). Demo-video guide
deferred to the Windows run (documented in docs/quality-gate.md →
Outstanding) — the recording itself is impossible without the app running.


---

## Phase 13 — Portfolio audit (quality gate)

**Objective:** final audit against the Definition of Done: architecture,
C++ quality, performance evidence, tests, docs, EA relevance; verified resume
bullets.

**Why this phase matters:** the difference between a repo and a portfolio.

**Dependencies:** Phase 12.

**Planned work:** run the full quality-gate checklist, fill
`context.md` → Verified Resume Evidence with only measured facts, produce
resume bullets from that evidence.

**Files/systems affected:** `context.md`, `README.md` (final), everything.

**Verification:** build + all tests + benchmarks re-run on the developer's
laptop; docs diff-checked against source.

**Status:** [x] Completed (sandbox scope) — 2026-10-02; laptop items remain OPEN.

**Notes:** docs/quality-gate.md: checklist across architecture, C++ quality,
performance evidence, tests, docs, EA relevance — every sandbox-verifiable
item PASSes; 5 items are OPEN and owned by the developer on the Windows
machine (run app, demo video, push to GitHub, laptop benchmarks, README
screenshot). Resume bullets in docs/quality-gate.md use only measured facts.
context.md → Verified Resume Evidence updated with the 2026-10-02 test
counts and benchmark numbers. Final sandbox state: 154 test cases / 4233
checks / 0 failures, normal + ASan/UBSan builds, all 5 ctest suites green.
