#pragma once
// Decoded image in tightly packed RGBA/component order, ready for GL upload
// (the GL texture wrapper lands with the renderer; decoding itself is pure
// C++, fully testable headless).

#include <cstdint>
#include <string>
#include <vector>

namespace af {

struct Image {
    int width = 0;
    int height = 0;
    int channels = 0;  // 1..4 as decoded (stb_image)
    std::vector<uint8_t> pixels;

    bool IsValid() const { return width > 0 && height > 0 && channels > 0; }
};

// Decodes PNG/JPEG/BMP/... via stb_image. Returns an invalid Image on failure
// (missing file, corrupt data); never throws.
Image LoadImage(const std::string& path);

}  // namespace af
