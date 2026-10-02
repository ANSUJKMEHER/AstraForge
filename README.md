<div align="center">

# ⚡ AstraForge

### *A High-Performance C++20 3D Game Engine & Interactive Dev Studio*

[![CI](https://img.shields.io/github/actions/workflow/status/ANSUJKMEHER/AstraForge/ci.yml?branch=main&style=for-the-badge&logo=githubactions&logoColor=white&label=CI)](https://github.com/ANSUJKMEHER/AstraForge/actions)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![OpenGL 3.3](https://img.shields.io/badge/OpenGL-3.3%20Core-5586A4?style=for-the-badge&logo=opengl&logoColor=white)](https://www.khronos.org/opengl/)
[![SDL2](https://img.shields.io/badge/SDL2-2.30.9-red?style=for-the-badge&logo=sdl&logoColor=white)](https://www.libsdl.org/)
[![ImGui](https://img.shields.io/badge/Dear%20ImGui-1.91-orange?style=for-the-badge)](https://github.com/ocornut/imgui)
[![Tests Passing](https://img.shields.io/badge/Unit%20Tests-154%2F154%20PASS-brightgreen?style=for-the-badge)](tests/)
[![Zero Allocations](https://img.shields.io/badge/Steady--State-0%20Allocations-gold?style=for-the-badge)](docs/performance.md)
[![License](https://img.shields.io/badge/License-MIT-blueviolet?style=for-the-badge)](LICENSE)

<br/>

**AstraForge** is a lightweight, high-performance 3D game engine, interactive development studio, and arena game built from the ground up in modern **C++20**. Designed with rigorous systems-engineering discipline: custom 3D math, sparse-set ECS, uniform-grid spatial collision broadphase, zero steady-state heap allocations, procedural audio synthesis, and an integrated real-time development environment.

</div>

---

## 🌟 Highlights

<table>
  <tr>
    <td width="50%">
      <h3>🚀 Pure C++20 ECS Architecture</h3>
      <ul>
        <li>Sparse-set component storage with contiguous, cache-dense iteration.</li>
        <li>Generational entity handles guaranteeing 100% reuse safety.</li>
        <li>Strict <b>zero per-frame dynamic heap allocations</b> in steady state.</li>
      </ul>
    </td>
    <td width="50%">
      <h3>🎯 Broadphase Collision System</h3>
      <ul>
        <li>Uniform 2D spatial hash grid broadphase with inflated AABB queries.</li>
        <li><b>99.98% candidate-pair reduction</b> over naive $O(n^2)$.</li>
        <li>Static boundary and obstacle collision with exact contact resolution.</li>
      </ul>
    </td>
  </tr>
  <tr>
    <td width="50%">
      <h3>🛠️ In-Game Dev Studio (Dear ImGui)</h3>
      <ul>
        <li><b>World Hierarchy & Inspector:</b> Click, inspect, and edit entity transforms, health, and AI live.</li>
        <li><b>Object Spawner & Palette:</b> Spawn defense turrets, barricades, enemy variants, and items in real time.</li>
        <li><b>Scene Manager:</b> Save and load custom arena layouts to <code>arena_scene.txt</code>.</li>
      </ul>
    </td>
    <td width="50%">
      <h3>🔊 Procedural Retro Audio Synthesizer</h3>
      <ul>
        <li>Built-in real-time audio synthesizer via SDL2 audio callbacks.</li>
        <li><b>Zero external audio assets or .wav files required</b> — laser sweeps, crunchy impact hits, noise explosions, and jump chirps synthesized mathematically.</li>
      </ul>
    </td>
  </tr>
  <tr>
    <td width="50%">
      <h3>✨ 3D Particle System & Visuals</h3>
      <ul>
        <li>Dynamic bouncing spark impacts on projectile hits.</li>
        <li>Fiery debris explosions on enemy elimination.</li>
        <li><b>Soft contact drop shadows</b> under entities for realistic physical grounding.</li>
        <li>Stylized Half-Lambert soft shading, distance fog, and glowing emissive grid.</li>
      </ul>
    </td>
    <td width="50%">
      <h3>🌱 Tower Defense / PvZ Mechanics</h3>
      <ul>
        <li>Stationary autonomous <b>Defense Turrets (Plants)</b> that lock onto enemies and fire.</li>
        <li>Enemy FSM state machine: <code>Idle → Chase → Attack → Hurt → Dead</code>.</li>
        <li>Customizable barricades and high-health blocker walls.</li>
      </ul>
    </td>
  </tr>
</table>

---

## 🏗️ System Architecture

```
                      +------------------------------------------+
                      |         Application (SDL2 Window)        |
                      +------------------------------------------+
                                           |
                                    PumpEvents / Loop
                                           |
             +-----------------------------+-----------------------------+
             |                                                           |
             v                                                           v
  [ Fixed 60 Hz Simulation ]                                  [ Render & Presentation ]
             |                                                           |
             v                                                           v
+--------------------------+                                +--------------------------+
|      GameSession         |                                |        Renderer          |
|--------------------------|                                |--------------------------|
| 1. PlayerController      |                                | - Frustum Culling        |
| 2. EnemyAISystem (FSM)   |                                | - Drop Shadows Pass      |
| 3. SpawnerSystem         |                                | - Forward Phong / Shaders|
| 4. PhysicsSystem         |                                | - 3D Particle System     |
| 5. CollisionSystem (Grid)|                                +--------------------------+
| 6. TurretDefenseSystem   |                                             |
| 7. Combat & Score        |                                             v
+--------------------------+                                +--------------------------+
             |                                              |   Dear ImGui DevStudio   |
             v                                              |--------------------------|
+--------------------------+                                | - Hierarchy Inspector    |
|   ECS World (Sparse-Set) |                                | - Object Spawner Palette |
| - Zero Steady-State Alloc|                                | - Real-time Tuner        |
+--------------------------+                                +--------------------------+
```

---

## ⚡ Performance Benchmarks

Measured using the reproducible stress benchmark `af_bench_gameplay` across scaled populations at constant entity density:

### 💻 Windows Hardware Results (MinGW GCC 15.2.0, `-O3`)

| Entity Population | Frame Time (ms) | Equivalent FPS | vs 60 Hz Budget (16.67 ms) |
|---|---|---|---|
| **1,000 Entities** | **0.111 ms** | **8,997 FPS** | 150× Headroom |
| **5,000 Entities** | **0.694 ms** | **1,441 FPS** | 24.0× Headroom |
| **10,000 Entities** | **1.428 ms** | **700 FPS** | 11.6× Headroom |
| **20,000 Entities** | **2.991 ms** | **334 FPS** | **5.5× Headroom** |

#### Per-System Timing Breakdown at 20,000 Entities:
* **Spatial Collision Grid:** `2.12 ms` (71%)
* **Kinematic Physics:** `0.61 ms` (20%)
* **Enemy AI State Machine:** `0.13 ms` (4%)
* **Combat Resolution:** `0.09 ms` (3%)
* **Entity Spawner & Bookkeeping:** `0.07 ms` (2%)
* **Player Controller & Projectiles:** `<0.01 ms` (<1%)

---

## 🎮 Controls & Shortcuts

| Input | Mode | Action |
|---|---|---|
| **`TAB`** or **`~`** | Any | **Toggle Dev Studio (Cursor Free) / Play Mode (Mouse Look)** |
| **`Esc`** | Any | Safely release cursor and open Dev Studio (Never quits) |
| **`F12`** | Any | **Capture instant high-res screenshot** (saves as `.bmp` with shutter sound) |
| **`W, A, S, D`** | Play Mode | Move player (camera-relative) |
| **`Mouse Move`** | Play Mode | 3D Orbit look-around and crosshair aiming |
| **`Left Mouse Click`** | Play Mode | Fire laser projectile |
| **`Spacebar`** | Play Mode | Jump |
| **`Right Mouse Button`** | Dev Mode | Hold and drag to orbit 3D camera while clicking menus |
| **`Mouse Wheel`** | Any | Zoom camera in and out |
| **`F5`** / **`F6`** | Any | Pause/Resume simulation / Step single simulation tick |
| **`R`** | Any | Restart the arena session |

---

## 🚀 Quick Start Guide

### Prerequisites
* **CMake ≥ 3.16**
* **C++20 compatible compiler** (MinGW-w64 GCC ≥ 12, Clang ≥ 15, or MSVC 2022)
* **SDL2 library** (Pre-configured in `third_party/` or installed via package manager)

### Build on Windows (MinGW + Ninja)
```powershell
# Clone the repository
git clone https://github.com/ANSUJKMEHER/AstraForge.git
cd AstraForge

# Configure and compile
cmake -B build -G Ninja
cmake --build build

# Launch the game with Dev Studio
.\build\astraforge_arena.exe
```

### Build on Linux (Ubuntu / Debian)
```bash
sudo apt-get update && sudo apt-get install -y cmake ninja-build libsdl2-dev libgl1-mesa-dev

cmake -B build -G Ninja
cmake --build build

# Run unit tests
ctest --test-dir build --output-on-failure

# Launch the game
./build/astraforge_arena
```

---

## 🧪 Testing & Verification

AstraForge includes a self-contained, zero-dependency unit test framework with **154 test cases and 4,233 checks (100% passing)**:

```bash
ctest --test-dir build --output-on-failure
```

```text
    Start 1: core_tests
1/5 Test #1: core_tests .......................   Passed    0.21 sec
    Start 2: headless_smoke
2/5 Test #2: headless_smoke ...................   Passed    0.02 sec
    Start 3: game_smoke
3/5 Test #3: game_smoke .......................   Passed    0.02 sec
    Start 4: collision_bench_smoke
4/5 Test #4: collision_bench_smoke ............   Passed    0.03 sec
    Start 5: gameplay_bench_smoke
5/5 Test #5: gameplay_bench_smoke .............   Passed    0.03 sec

100% tests passed, 0 tests failed out of 5
```

---

## 📁 Repository Structure

```text
AstraForge/
├── .github/workflows/       # Multi-platform CI (Linux, Sanitizers, Windows)
├── docs/                    # Technical documentation & engineering decisions
│   ├── architecture.md      # Data-flow and subsystem interactions
│   ├── technical-decisions.md# 20 measurement-backed engineering decisions
│   ├── performance.md       # Benchmarking methodology and hardware tables
│   ├── interview-notes.md   # Systems explainability & whiteboard walkthroughs
│   └── quality-gate.md      # Definition of Done and portfolio audit
├── include/af/              # Public headers
│   ├── app/                 # Application, DevUI, Audio, Particles, Screenshot
│   ├── core/                # Camera, Frustum, Profiler, Version, Assert
│   ├── ecs/                 # Sparse-Set, Entity, World, Systems, Views
│   ├── game/                # Gameplay systems, Components, Turrets, Pooling
│   ├── math/                # Vec2/3/4, column-major Mat4, Quat, Transform
│   ├── physics/             # Semi-implicit Euler, Gravity, Kinematics
│   ├── collision/           # Spatial hash grid, AABB/Sphere narrowphase
│   └── render/              # Renderer, GL Loader, Shaders, Mesh
├── src/                     # C++ implementation files
├── tests/                   # 154 unit and determinism test cases
├── third_party/             # Vendored ImGui and STB image
└── CMakeLists.txt           # Modern CMake configuration
```

---

## 📄 License

This project is licensed under the [MIT License](LICENSE). Built for game engineering portfolio and educational demonstration.
