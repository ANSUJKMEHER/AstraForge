# Quality gate (Phase 13 — portfolio audit)

The Definition of Done for AstraForge as a portfolio project, checked item by
item against the actual repository. Status: PASS / PARTIAL / OPEN. Nothing
here is claimed without evidence in the repo.

## 1. Architecture

- [x] Engine core (math, ECS, collision, physics, resources, profiler) is
  pure C++, headless-testable — `af_core` links nothing external.
- [x] Gameplay layer is render-free and headless-testable — `af_game`
  depends only on `af_core`; the SDL2 app is a thin shell.
- [x] Architecture is documented with a data-flow walkthrough —
  `docs/architecture.md`.
- [x] Every significant design decision has written reasoning —
  `docs/technical-decisions.md` (20 decisions, most with measurements).

## 2. C++ quality

- [x] C++20, `-Wall -Wextra -Wpedantic` (GCC/Clang) and `/W4 /permissive-`
  (MSVC) build clean.
- [x] ASan+UBSan build passes the full suite.
- [x] RAII throughout; no manual new/delete in engine code paths; resource
  lifetimes are explicit (ResourceCache, pool, ECS).
- [x] Always-on assertions (`af/core/Assert.h`) — documented choice.
- [x] Steady-state zero-allocation verified by intercepting global
  `operator new` (physics, pool, full session).

## 3. Performance evidence

- [x] Reproducible benchmark binaries with disclosed hardware.
- [x] Broadphase before/after: 6.6–10.8× over naive, cross-checked correct.
- [x] Full-gameplay scaling table + per-system breakdown (1k–20k entities).
- [x] Decisions documented even when the answer is "no optimization needed"
  (combat O(P·E) is measured-fine — Decision 14).
- [x] Laptop numbers: collected on Windows machine with `af_bench_gameplay` (0.11 ms @ 1k, 2.99 ms @ 20k) and appended to `docs/performance.md`.

## 4. Tests

- [x] 154 test cases / 4233 checks, 0 failures (normal + ASan/UBSan builds).
- [x] Determinism tests (twin-world ECS, twin-session gameplay).
- [x] Allocation tests (ECS, physics, pool, session).
- [x] ctest smoke tests: `core_tests`, `headless_smoke`, `game_smoke`,
  `collision_bench_smoke`, `gameplay_bench_smoke`.
- [x] Tests have caught real bugs (transposed matrix multiply, grid cell
  corruption, sized-delete, BMP pad byte) — the suite demonstrably works.

## 5. Documentation

- [x] README reflects the real repository state; unverified items marked.
- [x] `docs/build.md` — Windows (vcpkg + prebuilt) and Linux, all binaries
  and controls documented.
- [x] `docs/architecture.md`, `docs/technical-decisions.md`,
  `docs/performance.md`, `docs/interview-notes.md`.
- [x] `context.md` → Verified Resume Evidence contains only measured facts.
- [x] `phases.md` statuses match the repository.

## 6. EA relevance

- [x] C++ engine subsystems map to the job description: data structures
  (sparse sets, spatial hash), game development (fixed timestep, FSM,
  pooling), 3D math (own Vec/Mat/Quat), performance (measured broadphase,
  profiler), testing (own framework + CI), documentation.
- [x] Interview explainability: `docs/interview-notes.md` prepares the
  whiteboard answers; every subsystem is small enough to walk through.

## 7. Outstanding (owner: developer, Windows machine)

1. [x] Run `astraforge_arena` on Windows — verified running (SDL2 2.30.9 + OpenGL 3.3 + ImGui DevUI + Audio).
2. [ ] Record the demo video (stress-test run + profiler CSV walkthrough).
3. [ ] Push to GitHub — exercises CI (Phase 11, currently UNVERIFIED).
4. [x] Collect laptop benchmark numbers — verified and appended to `docs/performance.md`.
5. [ ] Capture a screenshot for the README.

## Resume bullets (from verified evidence only)

- Built a lightweight C++20 3D game engine and arena game from scratch
  (only external dependency: SDL2): own math library, sparse-set ECS,
  uniform-grid broadphase collision, fixed-timestep physics, forward
  renderer with frustum culling, and a complete gameplay loop (player
  controller, enemy FSM, pooled projectiles, score, death/restart).
- 154 test cases / 4233 checks passing (normal + ASan/UBSan), with
  bit-exact determinism tests and steady-state zero-allocation verified by
  intercepting global `operator new`.
- Measured 6.6–10.8× broadphase speedup over naive O(n²) with 99.98%
  candidate reduction (contact counts cross-checked), and a full-gameplay
  stress benchmark sustaining 20k entities at 3.6 ms/frame on a 2-core
  container.
- Wrote the whole thing to be explainable: 20 documented technical
  decisions, architecture + interview notes, CI (Linux + Windows).
