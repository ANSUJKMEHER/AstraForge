# stb

| File | Version | License | Upstream |
|---|---|---|---|
| stb_image.h | 2.30 | MIT / public domain (dual) | https://github.com/nothings/stb |

Vendored (single-header, no build step). `STB_IMAGE_IMPLEMENTATION` is defined
in exactly one translation unit: `src/core/resources/image_loader.cpp`.

Only `stb_image` is vendored — image DECODING. No encoder is used anywhere in
the engine.
