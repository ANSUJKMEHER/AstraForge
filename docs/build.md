# Build Instructions

AstraForge builds with **CMake ≥ 3.16** and a **C++20 compiler**. There is
exactly **one external dependency: SDL2** (the engine core and all tests build
with zero dependencies; the windowed app needs SDL2 + OpenGL 3.3 from your GPU
driver).

## Windows (primary dev machine)

### Option A — vcpkg (recommended, works with MSVC 2022)

```powershell
# one-time
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install sdl2:x64-windows

# configure (adjust the toolchain path to your machine)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure

# run the engine
build\Release\astraforge_arena.exe
```

### Option B — prebuilt SDL2 binaries (works with MSVC or MinGW)

1. Download `SDL2-devel-2.x.y-VC.zip` (MSVC) or `-mingw` from
   https://github.com/libsdl-org/SDL/releases — matching your toolchain.
2. Extract and pass the prefix to CMake:

```powershell
cmake -B build -DCMAKE_PREFIX_PATH=C:/SDL2-2.30.9
```

OpenGL comes from your GPU driver — no extra download.

## Linux (CI / secondary)

```sh
sudo apt install libsdl2-dev   # Debian/Ubuntu
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build options

| Option | Default | Purpose |
|---|---|---|
| `ASTRAFORGE_BUILD_TESTS` | ON | unit tests + ctest registration |
| `ASTRAFORGE_BUILD_APP` | ON | SDL2/OpenGL app (skipped with a warning if SDL2 missing) |
| `ASTRAFORGE_BUILD_HEADLESS` | ON | headless sim runner + headless gameplay runner |
| `ASTRAFORGE_BUILD_BENCHMARKS` | ON | benchmark binaries |
| `ASTRAFORGE_SANITIZE` | OFF | ASan+UBSan build (GCC/Clang only) |

## Binaries produced

| Binary | Purpose |
|---|---|
| `astraforge_arena` | the windowed arena game (needs SDL2) |
| `af_tests` | full unit-test suite (via ctest: `core_tests`) |
| `af_headless` | headless physics smoke runner (`af_headless [entities] [steps]`) |
| `af_headless_game` | headless full-gameplay runner, scripted input (`af_headless_game [steps]`) |
| `af_bench_collision` | naive vs grid broadphase benchmark |
| `af_bench_gameplay` | gameplay stress benchmark (`af_bench_gameplay [enemies] [frames]`) |

## Game controls (windowed app)

| Input | Action |
|---|---|
| Mouse | look around (relative mode) |
| W A S D | move (camera-relative) |
| Space | jump |
| Left mouse button | shoot (aim = camera forward) |
| R | restart the session |
| Esc | quit |

## Troubleshooting

- **"SDL2 not found — astraforge_arena not built"** → SDL2 dev files are not
  on the CMake search path; use vcpkg (Option A) or `CMAKE_PREFIX_PATH`
  (Option B).
- **"renderer init failed (OpenGL 3.3 required)"** → the driver/context could
  not provide GL 3.3 core. Update GPU drivers; any laptop GPU from the last
  decade supports 3.3.
- **"missing GL entry point gl…"** → the context exposes fewer entry points
  than the loader expects; this should never happen on a 3.3 core context.
- **MSVC warning D9025** → you used GCC-style `-W` flags; remove any manually
  added flags (the CMake project handles warnings per compiler).

## What is verified where (honest status)

- Engine core + all tests + benchmarks: verified on GCC 12.2 (Linux CI
  sandbox) in normal and sanitizer builds.
- The SDL2/OpenGL app: **compile-checked against real SDL2 2.30.9 headers in
  the sandbox, but NOT yet run** — the sandbox has no GPU/display. First
  runtime verification happens on a Windows machine and in GitHub Actions
  (Phase 11). Do not claim the app "runs" until it has run somewhere.
