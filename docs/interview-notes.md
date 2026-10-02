# Interview notes

A self-study sheet for explaining AstraForge in a Software Engineer Intern
interview (EA Slingshot Studios). Every claim here is backed by code + tests
in the repository; the point is that each answer can be made concrete on a
whiteboard.

## 1. What is this project?

A lightweight C++20 3D game engine + playable arena prototype, built to
demonstrate engineering depth: math, ECS, collision, physics, gameplay
systems, profiling — all written from scratch (only dependency: SDL2), all
measurable and tested (154 test cases / 4233 checks, zero failures, in both
normal and ASan/UBSan builds).

## 2. The "elevator" walkthrough

- **Math**: own Vec/Mat/Quat library instead of GLM (Decision 1) — the test
  suite caught a transposed Mat4·Mat4 on the first build. Column-major,
  right-handed, +Y up, cameras down −Z (`docs/math-conventions.md`).
- **ECS**: entity = (id, generation) handle; sparse-set storage; filtered
  views; ordered systems. Zero steady-state allocations (verified by
  overriding global `operator new`); 100k entities update in 0.56 ms/step.
- **Collision**: uniform XZ-grid broadphase (center-anchored + inflated
  queries) → Sphere/AABB narrowphase → positional separation. 6.6–10.8×
  faster than naive, contacts cross-checked equal.
- **Game loop**: fixed 60 Hz timestep + accumulator (deterministic, testable),
  render rate free.
- **Gameplay**: player controller, enemy FSM, pooled projectiles, score,
  death/restart — headless-testable, bit-deterministic (twin-session test).

## 3. Likely questions and the shape of the answer

**"Why sparse sets and not archetypes?"**
Sparse sets give O(1) add/remove with swap-remove, dense cache-friendly
iteration, and simplicity. Archetypes (EnTT-style) pay off at huge entity
counts with many component combinations — measured, this project doesn't
need it (Decision 2). Views iterate the smallest requested set.

**"Explain your broadphase."**
Uniform grid on the XZ plane (gameplay is planar, arena bounded). Each entity
anchors in ONE cell (its bounds center); queries inflate by the largest
extent of the frame. Superset property: if A and B overlap, B's center is
within A's inflated query range. The first version inserted entities into
every covered cell and corrupted cell lists — the superset property test
caught it (Decision 11). Statics (walls) are excluded from the grid to keep
inflation tight (Decision 15).

**"What's the fixed-timestep loop and why?"**
Deterministic simulation, decoupled from render rate: accumulator counts
elapsed wall time, runs N fixed 60 Hz steps, clamps at 250 ms (spiral-of-
death guard). Same input → bit-identical state (tested). Interpolation is a
documented future item; at 60 Hz + vsync it's small.

**"How do you know you don't leak memory / allocate per frame?"**
The test binary overrides global `operator new` and counts bytes. Steady-
state claims (physics, pool, full session) are all verified as exactly zero
allocations after warm-up.

**"Why pool projectiles?"**
Projectile spawn/despawn is the hottest structural path. Pool = pre-created
parked entities; Spawn/Despawn = component add/remove. 1000 cycles allocate
0 bytes (measured). Pool exhaustion drops shots — bounded frame cost
(Decision 13).

**"What's the enemy FSM?"**
Idle → Chase (detect range) → Attack (attack range, cooldown damage) → Hurt
(timer) → Chase; health ≤ 0 → Dead → destroyed + scored. State lives in the
`Enemy` component; the AI system advances it at fixed dt.

**"Why no job system?"**
Measured: full gameplay frame at 20k enemies is 3.6 ms on a 2-core sandbox
(4.6× inside the 60 Hz budget); collision (the only heavy system) is a
sequential rebuild→query dependency; parallelizing would cost determinism
and add sync complexity for no measured gain. The decision includes a
written revisit trigger (Decision 18) — that's the "measured no" interview
answer.

**"How is determinism guaranteed?"**
One LCG (spawner), seeded at construction; scripted input; fixed system
order; no hash-order-dependent iteration anywhere. Twin-session test: two
sessions, identical input, 420 steps → bit-identical positions/scores.

**"What did testing actually catch?"**
Transposed matrix multiply (Phase 1), missing sized-delete in the allocation
hook (Phase 2), grid cell-list corruption from multi-cell insertion
(Phase 3), BMP pad byte in the decode fixture (Phase 5). The suite is the
evidence the process works.

**"Draw the frame."**
See `docs/architecture.md` → data flow (8 systems in documented order) +
render path. Numbers: per-system breakdown table in
`docs/performance.md` §1.

## 4. Numbers to have ready

- 154 test cases / 4233 checks / 0 failures (normal + ASan/UBSan), GCC 12.2.
- ECS: 0.0056 ms/step @ 1k → 0.5635 ms/step @ 100k entities.
- Broadphase: 6.6–10.8× vs naive; 99.984% candidate reduction.
- Gameplay: 0.148 ms @ 1k → 3.64 ms @ 20k enemies (sandbox container).
- Zero steady-state allocations: physics, pool, full session (verified).

Hardware disclosure matters: sandbox = 2 vCPU container, -O3; laptop numbers
come from the Windows run (Phase 11) — say so if asked.

## 5. Honesty checklist (interview non-negotiables)

- The SDL2/OpenGL app is **compile-checked but not yet run** in the sandbox
  (no GPU/display). First runtime check: Windows + CI. Say this unprompted
  when discussing the app.
- All benchmark numbers carry their hardware label.
- "I don't know" + a plan to find out beats hand-waving.
