# context.md — AstraForge persistent project context

The source of truth for what this project is, how it is built, and what state
it is in. Read this first when resuming work. `phases.md` holds the plan and
per-phase status. Never mark something done here that the repository does not
actually contain.

---

## Project

- **Objective:** a lightweight 3D game engine in modern C++, used to build a
  small playable arena prototype. The point is engineering depth, not graphics:
  measurable, tested, explainable subsystems.
- **Current scope:** engine core (math, ECS, collision, physics, resources,
  profiling) + a complete arena game (player, enemy FSM, pooled projectiles,
  score, death/restart) + an SDL2/OpenGL windowed app.
- **Target role:** Software Engineer Intern application for EA Slingshot
  Studios, Hyderabad. The project must demonstrate C++, data structures,
  game development, 3D math, performance work, testing, and documentation —
  and be explainable by the developer in an interview, with or without AI help.
- **Hardware target:** normal student laptop (HP Pavilion, Ryzen 5, integrated
  GPU). No heavy assets, no huge dependencies.

## Architecture

Subsystems are modules of `af_core` + `af_game` (pure C++, headless-testable)
plus the application layer (SDL2/OpenGL). Decided architecture:

```
            Application (SDL2 window + input)
                       |
                   Game Loop  (fixed timestep + accumulator)
                       |
      +----------------+----------------+
      |                |                |
   ECS World      Physics        Resource Cache
   (sparse sets)  (kinematics)   (mesh/shader/texture)
      |                |                |
   Gameplay        Collision        Renderer
   (player/enemy/  (broadphase      (GL 3.3 forward,
    projectile)    uniform grid →   frustum culling)
                   narrowphase      console HUD stats
                   AABB/sphere)
```

| Subsystem | Responsibility |
|---|---|
| `af::math` | Vec2/3/4, Mat4, Quat, Transform, camera matrices (own implementation — see Decisions) |
| ECS | Entity handles (id + generation), sparse-set component storage, ordered systems, filtered views |
| Collision | AABB/sphere primitives, uniform-grid broadphase (XZ plane), narrowphase tests, static colliders (walls), response |
| Physics | Velocity/gravity integration at fixed dt, ground response, jump |
| Resources | Procedural primitives, texture loading, shader wrapper, cache with lifetime management |
| Renderer | GL 3.3 forward renderer, camera, one PBR-free Phong-ish material model, draw call accounting |
| Profiling | RAII scope timers, per-frame counters, frame-time ring buffer, CSV dump |
| Gameplay | Player controller, enemy FSM, projectile pool, combat, score, spawner, death/restart (GameSession) |

Data flow and full walkthrough: `docs/architecture.md`.

## Technology

- **Language:** C++20.
- **Build:** CMake ≥ 3.16. Verified with CMake 3.30.5+ (sandbox) and GCC 12.2.
  Primary dev machine (Windows): MSVC 2022 or MinGW — both supported by the
  CMake setup; SDL2 is the only external library.
- **Dependencies:** SDL2 (window/input/GL context) + OpenGL 3.3 (system driver).
  Everything else is written from scratch or vendored: stb_image (textures).
  Dear ImGui was considered for the dev UI and deferred (Decision 19).
- **Testing:** own minimal header-only framework (`tests/framework/af_test.hpp`).

## Current State

- **Phases 0–13 complete** (see `phases.md`): math → ECS → collision →
  physics → resources → rendering → app layer → gameplay → performance
  engineering → concurrency decision → CI → docs → portfolio audit.
- **Test suite: 154 test cases / 4233 checks / 0 failures** (verified
  2026-10-02, GCC 12.2, Linux sandbox, normal + ASan/UBSan builds).
- ctest runs `core_tests`, `headless_smoke`, `game_smoke`,
  `collision_bench_smoke`, `gameplay_bench_smoke`.
- **The SDL2/OpenGL app has never run** — the sandbox has no GPU/display.
  The app layer (including the Phase 8 arena game integration) is
  compile-checked against real SDL2 2.30.9 headers. First runtime
  verification: the developer's Windows machine, then GitHub Actions
  (authored, UNVERIFIED until first push). See Risks.
- Engine version: 0.2.0.

## Decisions

1. **Write our own math library instead of GLM.** GLM is header-only and
   battle-tested, but implementing Vec/Mat/Quat ourselves is exactly the 3D
   math an interviewer probes, it removes a dependency, and it is fully
   testable. Cost: we own correctness — mitigated by a test suite that
   caught a transposed matrix multiply in the first build.
2. **Sparse-set ECS, not archetypes** *(implemented Phase 2)*. Simple, O(1)
   add/remove, swap-remove deletion, cache-friendly iteration. Verified:
   stale-handle rejection via id+generation; swap-remove correctness; view
   filtering; **zero allocations in steady state** (measured by overriding
   global `operator new`); bit-identical twin-world determinism.
3. **Uniform grid broadphase on the XZ plane.** Gameplay is mostly planar
   movement in a bounded arena; a uniform grid gives O(1) neighbor queries,
   trivially explains itself, and is easy to benchmark against naive O(n²).
4. **Fixed-timestep game loop (60 Hz simulation) + accumulator.** Deterministic
   physics/gameplay (testable, reproducible benchmarks), decoupled from render
   rate. Interpolation to render rate is a stretch goal.
5. **Own test framework, not Catch2/doctest.** ~100 lines, zero build
   complexity on Windows, easily explained.
6. **Headless simulation mode.** The full gameplay sim runs without a window
   (fixed steps, scripted input). This is how gameplay/ECS/performance get
   tested in CI and in this sandbox, where no GPU/display exists.
7. **Only SDL2 as an external dependency.** stb_image vendored (small,
   permissively licensed). Keeps Windows setup to one library.
8. **No job system** *(resolved with measurements — see Decision 18)*.
9. **Forward renderer, not deferred.** One simple Phong-ish lighting model,
   straightforward to explain end to end.
10. **Own GL loader, not GLAD/GLEW.** `af/render/GL.h` declares the GL 3.3
    entry points we use as function pointers, resolved once via
    `SDL_GL_GetProcAddress`. Missing entry points fail loudly with names.
11. **Grid membership: center-anchored cells + inflated queries.** The first
    grid version inserted entities into every cell their bounds covered;
    with a single per-entity next-pointer that corrupts cell lists (found by
    the superset property test in Phase 3). The redesign anchors each entity
    in the single cell containing its bounds center and inflates queries by
    the frame's largest extent — correct by construction, faster (1 insertion
    per entity), still zero-allocation. The benchmark's cross-check (grid
    contacts == naive contacts) now guards the property continuously.

Decisions 12–20 (gameplay architecture, pooling, combat narrowphase, static
colliders, restart policy, determinism, concurrency, dev UI, benchmark
methodology) live in `docs/technical-decisions.md`.

## Bugs

- None known: math, ECS, collision, physics, resources, gameplay (Phases
  1–8) are fully green including under ASan+UBSan.
- Real bugs found and fixed during development (the process works):
  - **Transposed `Mat4*Mat4`** (Phase 1) — caught by an associativity test;
    regression-guarded by a non-symmetric identity test.
  - **Missing sized-delete overloads** in the allocation probe (Phase 2) —
    caught by ASan.
  - **Grid cell-list corruption from multi-cell insertion** (Phase 3) —
    caught by the superset property test; fixed by the center-anchor redesign
    (Decision 11).
  - **Extra BMP pad byte in the decode test fixture** (Phase 5) — caught by
    comparing decoded pixels against expectations; the decoder itself was
    correct.
- Template — fill in as bugs are found:
  - Symptom / root cause / workaround / fixed-in-commit.

## Performance

Measurements so far (**hardware: 2 vCPU Linux sandbox container, GCC 12.2,
-O3 — NOT the target laptop**; laptop numbers are collected on the Windows
machine, see `docs/quality-gate.md`). Full methodology + per-system
breakdowns: `docs/performance.md`.

ECS update loop (`p += v·dt` over Transform+Velocity, 600 steps):

| Entities | ms/step |
|---|---|
| 1,000 | 0.0056 |
| 5,000 | 0.0532 |
| 10,000 | 0.0557 |
| 20,000 | 0.1147 |
| 50,000 | 0.2857 |
| 100,000 | 0.5635 |

Collision broadphase: naive O(n²) vs uniform grid (spheres r=0.75 in a
40×40 arena, 100 frames; `af_bench_collision`):

| Entities | Speedup | Candidate reduction | Cross-check |
|---|---|---|---|
| 1,000 | 6.64× | 99.984% | OK (contacts match) |
| 5,000 | 9.99× | 99.984% | OK |
| 10,000 | 10.82× | 99.984% | OK |
| 20,000 | 10.45× | 99.984% | OK |

Full gameplay session under stress (`af_bench_gameplay`, constant entity
density — arena scales with population):

| Enemies | ms/frame | FPS equiv | vs 60 Hz budget |
|---|---|---|---|
| 1,000 | 0.148 | 6,752 | 113× headroom |
| 5,000 | 0.896 | 1,116 | 18.6× headroom |
| 10,000 | 1.809 | 553 | 9.2× headroom |
| 20,000 | 3.640 | 275 | 4.6× headroom |

At 20k enemies the per-frame split is collision 2.91 ms (82%), physics
0.63 ms (18%), enemy AI 0.25 ms, everything else < 0.1 ms. Determinism:
repeated headless runs are bit-identical.

## Testing

- Framework: `tests/framework/af_test.hpp` (own, header-only).
- Run: `cmake -B build && cmake --build build && ctest --test-dir build`.
- Current status: **154 test cases, 4233 checks, 0 failures** (verified
  2026-10-02, GCC 12.2, Linux sandbox, normal + ASan/UBSan builds).
- ctest also runs `headless_smoke` (1000 entities × 120 physics steps),
  `game_smoke` (600-step scripted gameplay), `collision_bench_smoke` and
  `gameplay_bench_smoke` (benchmark cross-checks).
- Coverage: math (406 checks), ECS (handles/sparse sets/world/views/systems/
  determinism/zero-allocation), collision (narrowphase geometry, grid
  superset property, system behavior, static walls), physics (bit-exact
  integration references, ground/jump/terminal velocity, zero-allocation),
  resources (mesh geometry, real BMP decode, cache semantics, shader
  sources), frustum/camera (plane extraction, containment, orbit/clamp),
  profiler (scope accounting, ring buffer, CSV, zero-allocation), gameplay
  (projectile pool reuse/exhaustion/zero-allocation, enemy FSM transitions,
  melee cadence, combat hits/misses/kills, player controller movement/jump/
  shoot cooldown/clamp, session spawn/score/death/restart/twin-session
  determinism/steady-state zero-allocation).
- Sanitizer support: `-DASTRAFORGE_SANITIZE=ON` (GCC/Clang).

## Repository

```
AstraForge/
├── context.md            ← this file (persistent project context)
├── phases.md             ← plan + per-phase status
├── README.md
├── CMakeLists.txt
├── .gitignore
├── .github/workflows/    ← ci.yml (authored; UNVERIFIED until first push)
├── include/af/           ← public headers: math/ core/ ecs/ collision/
│                             physics/ resources/ render/ app/ game/
├── src/core/             ← engine core translation units (pure C++)
├── src/game/             ← gameplay systems + session + headless game runner
├── src/app/              ← SDL2/OpenGL application layer
├── src/bench/            ← benchmark binaries (collision, gameplay)
├── tests/                ← unit tests + framework/af_test.hpp + allocation probe
├── docs/                 ← build, performance, technical-decisions,
│                             architecture, interview-notes, quality-gate
├── assets/               ← placeholder for runtime assets (unused: procedural)
└── third_party/          ← vendored deps: stb/ (done); imgui/ deferred
```

Build commands:

```
cmake -B build
cmake --build build --config Release   # (--config only for MSVC generators)
ctest --test-dir build --output-on-failure
```

## Future Work

Runtime verification on real hardware (see `docs/quality-gate.md` →
Outstanding): run the app on Windows, capture screenshots + demo video,
push to GitHub to exercise CI, collect laptop benchmark numbers. No new
features are planned — the project is feature-complete per `phases.md`.

## Verified Resume Evidence

Only facts verified by the actual repository. **Nothing here is fabricated.**

- C++20 engine codebase, CMake ≥ 3.16 build. *(verified — builds clean)*
- Own 3D math library (vectors, column-major Mat4, quaternions, perspective/
  orthographic/lookAt). *(verified 2026-10-02, GCC 12.2 sandbox)*
- Sparse-set ECS: id+generation handles, O(1) swap-remove, filtered views;
  steady-state zero-allocation verified by intercepting global
  `operator new`; headless 100k-entity update at **0.56 ms/step** on the
  2 vCPU sandbox container (laptop numbers pending).
- **154 test cases / 4233 assertions passing** total (normal + ASan/UBSan
  builds, ctest). *(verified 2026-10-02)*
- Uniform-grid broadphase: **6.6–10.8× faster** than naive O(n²) with
  **99.98% candidate-pair reduction** at 1k–20k entities, contact counts
  cross-checked equal to the naive reference (sandbox container, laptop
  numbers pending). *(verified 2026-10-02)*
- Complete arena gameplay: player controller, enemy FSM, fixed-capacity
  projectile pool (1000 cycles, 0 bytes allocated), score, death/restart;
  bit-exact twin-session determinism. *(verified)*
- Fixed-timestep physics (semi-implicit Euler, gravity/ground/jump) with
  bit-exact determinism tests. *(verified)*
- Full-gameplay stress benchmark: **20k entities at 3.6 ms/frame** (275 fps
  equivalent) on the 2 vCPU sandbox; per-system profiler breakdown included.
  *(verified)*
- Measured concurrency decision ("no job system, here's why") with a written
  revisit trigger. *(verified — docs/technical-decisions.md → Decision 18)*
- CI authored for Linux (GCC + sanitizers) and Windows (MSVC + vcpkg).
  *(authored; UNVERIFIED until first push to GitHub)*
- TODO (fill in after the Windows run): laptop entity benchmark, FPS,
  frame time, culling reduction, CI status, repo URL, screenshots.

## Risks

- **R1 — sandbox has no SDL2/OpenGL/display.** The engine core, gameplay,
  tests, and benchmarks are fully verified here; the SDL2/OpenGL app layer
  is **compile-checked against real SDL2 2.30.9 headers but has never run**
  (no GPU/display in the sandbox). First runtime verification: the
  developer's Windows machine, then GitHub Actions (authored, UNVERIFIED).
  Never claim the app "runs" until it has.
- **R2 — Windows toolchain variation.** MSVC vs MinGW, SDL2 install paths.
  Mitigation: standard `find_package(SDL2)`; `docs/build.md` covers both
  vcpkg and manual installs.
- **R3 — scope creep.** Every feature gates on: does it materially improve
  engineering value, EA relevance, or portfolio quality? (ImGui dev panel
  deferred on this gate — Decision 19.)
- **R4 — session interruption.** Mitigation: this file + `phases.md` + a
  continuously buildable repository. An agent resuming work starts by reading
  these two files and verifying against the source tree.
