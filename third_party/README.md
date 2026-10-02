# third_party

Vendored dependencies live here.

**Policy:** prefer writing code ourselves when it is core learning material
(math, ECS, collision, physics — the subsystems an interviewer will ask
about). Vendor only small, stable, permissively licensed libraries that are
pure infrastructure.

| Directory | Library              | License | Status         |
|-----------|----------------------|---------|----------------|
| stb/      | stb_image 2.30       | MIT / public domain (dual) | vendored (Phase 5) |
| imgui/    | Dear ImGui (pinned)  | MIT     | **deferred** — see below |

`stb_image` is vendored (BMP/PNG/JPEG decode; tested with a generated BMP in
`tests/resources/test_image.cpp`).

Dear ImGui (planned for the dev UI) was **deferred with a written decision**
(`docs/technical-decisions.md` → Decision 19): the panel cannot be
runtime-verified in the sandbox (no GPU/display), and the console stats +
profiler CSV deliver the same diagnostic value. Revisit after the Windows
runtime verification (Phase 11).

When vendoring: preserve the upstream `LICENSE` file(s) in each directory and
record the pinned version + source commit in that directory's README.
