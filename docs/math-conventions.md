# Math Conventions

AstraForge uses a right-handed, OpenGL-compatible convention throughout. This
document is the single source of truth for every coordinate/matrix question.
Interviewers ask about this — know it.

## Coordinate system

```
        +Y (up)
        |
        |
        +------ +X (right)
       /
      /
     +Z (toward the camera)
```

- **Right-handed**: `X × Y = Z`.
- Cameras look down **-Z** (their forward vector is -Z in view space).
- The world's "up" is +Y.

## Matrix storage

- `Mat4` is **column-major**: element `(row, col)` lives at `v[col * 4 + row]`.
- `m(0, 3), m(1, 3), m(2, 3)` is the translation column.
- `Data()` can be passed directly to `glUniformMatrix4fv(..., GL_FALSE, ...)`.

## Multiplication conventions

- Matrices transform **column vectors**: `M * v`.
- `(a * b) * v == a * (b * v)` — the rightmost matrix applies first.
- `Transform::ToMat4()` composes `T * R * S`: scale first, then rotate, then
  translate (so scaling happens in local space, translation in world space).
- `Translate(m, t)` is shorthand for `m * T(t)` (post-multiply).

## The space pipeline

```
World Space        — object positions, gameplay coordinates
     ↓  view matrix (LookAt)
View Space         — camera at origin, looking down -Z
     ↓  projection matrix (Perspective / Ortho)
Clip Space         — homogeneous (x, y, z, w); vertices outside
                     [-w, w] in each axis are clipped
     ↓  perspective divide (x/w, y/w, z/w) — done by the GPU
NDC                — x, y, z ∈ [-1, 1] (OpenGL convention, not Vulkan's [0,1])
     ↓  viewport transform
Screen Space       — pixels
```

| Stage | Purpose |
|---|---|
| World → View | Put the camera at the origin; make "forward" be -Z so depth is simple |
| View → Clip | Apply perspective; encode depth in w for the divide |
| Clip → NDC | Normalize the visible volume to a unit cube |
| NDC → Screen | Map the unit cube to the actual window pixels |

## Depth convention

`Perspective`/`Ortho` map view-space z from `-zNear` (→ NDC -1) to `-zFar`
(→ NDC +1). The default OpenGL depth test is `GL_LESS`, so smaller NDC depth
wins: nearer fragments have smaller z, exactly as expected.

## Rotations

- Quaternions are stored `(x, y, z, w)`, w = real part.
- `Rotate(q, v)` applies the rotation; `q * p` composes with `p` applied first.
- `FromEuler(yaw, pitch, roll)` uses intrinsic **YXZ** order:
  `qY(yaw) * qX(pitch) * qZ(roll)` — yaw about Y, pitch about X, roll about Z.
- `Slerp` interpolates along the shorter arc (it flips the sign when
  `dot(q1, q2) < 0`).
- `-q` and `q` represent the same rotation (double cover).

## Known limitations

- `LookAt` is degenerate when `center - eye` is parallel to `up`. The gameplay
  camera clamps pitch away from ±90° so this never happens in practice.
- `Inverse` produces garbage for singular matrices; callers are responsible
  for using it only on invertible matrices (rigid transforms always are).
