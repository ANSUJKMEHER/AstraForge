# Architecture

How AstraForge is put together. Companion reading: `context.md` (decisions,
current state), `docs/technical-decisions.md` (decisions 12+), `phases.md`
(plan + status).

## Overview

A lightweight C++20 3D game engine + arena prototype, organized as **engine
core** (pure C++, headless-testable) under a thin **application layer**
(SDL2 + OpenGL 3.3).

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
                   narrowphase
                   AABB/sphere)
```

## Libraries (CMake targets)

| Target | Contents | Dependencies |
|---|---|---|
| `af_core` | math, ECS, collision, physics, resources, frustum/camera, profiler | none (own code + vendored stb_image) |
| `af_game` | gameplay components, systems, projectile pool, GameSession | `af_core` |
| `af_tests` | unit tests (own framework `af_test.hpp`) | `af_game` |
| `af_headless` / `af_headless_game` | headless sim runners (CI smoke tests) | `af_core` / `af_game` |
| `af_bench_collision` / `af_bench_gameplay` | benchmark binaries | `af_core` / `af_game` |
| `astraforge_arena` | SDL2/OpenGL windowed app | `af_game` + SDL2 |

The game does NOT depend on SDL2 or OpenGL. The windowed app is a shell:
input mapping + `Renderable` sync + camera + draw loop. This is what makes
the whole game testable in CI without a GPU (Decision 12).

## Data flow — one fixed simulation step (60 Hz)

`GameSession::Update(dt)` runs eight systems in a fixed, documented order:

```
PlayerController ──► EnemyAI ──► Spawner ──► Physics ──► Collision
                                                                  │
Score ◄── Combat ◄── Projectiles ◄───────────────────────────────┘
```

1. **PlayerController** — `PlayerInput` → WASD velocity, edge-triggered
   jump (`JumpRequest`), rate-limited pooled projectile spawn. Clamps the
   player to the arena.
2. **EnemyAI** — per-enemy FSM (Idle → Chase → Attack → Hurt → Dead):
   chase velocity, melee damage on the attack timer, hurt recovery.
3. **Spawner** — LCG-placed enemies on the arena edge ring, capped at
   `maxAlive`. Bit-reproducible.
4. **Physics** — semi-implicit Euler at fixed dt: gravity (terminal-velocity
   clamped) → integrate → ground response → jump consumption.
5. **Collision** — uniform XZ grid broadphase (dynamic) + direct static-wall
   pass → narrowphase (Sphere/AABB with contact normals) → positional
   separation (symmetric for dynamic pairs, full-push for walls).
6. **Projectiles** — constant-velocity motion, lifetime + arena expiry,
   returned to the pool.
7. **Combat** — O(P·E) Sphere↔Sphere hits: damage, Hurt/Dead transitions,
   projectile consumption.
8. **Score** — destroys Dead enemies, awards points, counts kills.

After the systems run, the session checks the player's health: death starts
the restart countdown, then resets the world (Decision 16).

The **render path** (variable rate) is separate: `RenderScene` walks
Transform+Renderable, frustum-culls against the camera, and submits draws;
stats (submitted/culled/draw calls) feed the console HUD.

## Key mechanisms

- **Sparse-set ECS** (`af/ecs/`): entity = (id, generation) handle; dense
  component arrays + sparse id→index map; O(1) add/remove/swap-remove;
  filtered views iterate the smallest requested set. Steady-state zero
  allocations, verified by intercepting global `operator new`.
- **Spatial hash broadphase** (`af/collision/`): center-anchored cells +
  extent-inflated queries — correct by construction (superset property),
  6.6–10.8× over naive O(n²), contact counts cross-checked.
- **Fixed timestep + accumulator** (`af/app/Application.h`): deterministic
  60 Hz sim, clamped accumulator (spiral-of-death guard), render rate free.
- **Resource cache** (`af/resources/`): weak_ptr-based lazy free/reload,
  procedural primitives, embedded GLSL 330 shaders, vendored stb_image.
- **Profiler** (`af/core/Profiling.h`): RAII scope timers, frame-time ring
  buffer, CSV dump; zero steady-state allocations.

## Filesystem map

```
AstraForge/
├── include/af/
│   ├── math/        Vec2/3/4, Mat4, Quat, Transform
│   ├── ecs/         Entity, SparseSet, World, System, View
│   ├── collision/   Shapes, CollisionTests, SpatialHashGrid, CollisionSystem
│   ├── physics/     PhysicsComponents, PhysicsSystem
│   ├── resources/   Mesh, Image, ResourceCache, Shaders
│   ├── render/      GL (own loader), Renderer, RenderSystem
│   ├── app/         Application (SDL2 window/input/loop)
│   ├── core/        Components, Camera, Frustum, Profiling, Assert, Version
│   └── game/        GameplayComponents, *System, ProjectilePool, GameSession
├── src/core/        engine core translation units
├── src/game/        gameplay translation units + headless game runner
├── src/app/         SDL2/OpenGL application layer
├── src/bench/       af_bench_collision, af_bench_gameplay
├── tests/           af_test.hpp framework + per-subsystem suites
├── docs/            build, performance, technical-decisions, architecture
└── third_party/     vendored stb_image (MIT)
```
