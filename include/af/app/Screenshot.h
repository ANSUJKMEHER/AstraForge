#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <ctime>

#include "af/render/GL.h"

namespace af {

inline bool CaptureScreenshot(int width, int height, std::string& outFilename) {
    if (width <= 0 || height <= 0) return false;

    // Generate filename with timestamp
    std::time_t now = std::time(nullptr);
    char nameBuf[64];
    std::strftime(nameBuf, sizeof(nameBuf), "screenshot_%Y%m%d_%H%M%S.bmp", std::localtime(&now));
    outFilename = nameBuf;

    // Read pixels from current framebuffer
    std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 3));
    gl::ReadPixels(0, 0, width, height, gl::Rgb, gl::UnsignedByte, pixels.data());

    // Row size in BMP must be padded to multiple of 4 bytes
    const int rowPadding = (4 - (width * 3) % 4) % 4;
    const int stride = width * 3 + rowPadding;
    const uint32_t imageSize = static_cast<uint32_t>(stride * height);
    const uint32_t fileSize = 14 + 40 + imageSize;

    FILE* f = std::fopen(outFilename.c_str(), "wb");
    if (!f) return false;

    // BMP Header (14 bytes)
    uint8_t bmpHeader[14] = {
        'B', 'M',
        static_cast<uint8_t>(fileSize), static_cast<uint8_t>(fileSize >> 8),
        static_cast<uint8_t>(fileSize >> 16), static_cast<uint8_t>(fileSize >> 24),
        0, 0, 0, 0,
        54, 0, 0, 0  // offset to pixel data
    };
    std::fwrite(bmpHeader, 1, 14, f);

    // DIB Header (BITMAPINFOHEADER - 40 bytes)
    uint8_t dibHeader[40] = {};
    dibHeader[0] = 40;  // header size
    // Width
    dibHeader[4] = static_cast<uint8_t>(width);
    dibHeader[5] = static_cast<uint8_t>(width >> 8);
    dibHeader[6] = static_cast<uint8_t>(width >> 16);
    dibHeader[7] = static_cast<uint8_t>(width >> 24);
    // Height (positive = bottom-to-top, exactly matching OpenGL!)
    dibHeader[8] = static_cast<uint8_t>(height);
    dibHeader[9] = static_cast<uint8_t>(height >> 8);
    dibHeader[10] = static_cast<uint8_t>(height >> 16);
    dibHeader[11] = static_cast<uint8_t>(height >> 24);
    dibHeader[12] = 1;   // color planes
    dibHeader[14] = 24;  // bits per pixel
    dibHeader[20] = static_cast<uint8_t>(imageSize);
    dibHeader[21] = static_cast<uint8_t>(imageSize >> 8);
    dibHeader[22] = static_cast<uint8_t>(imageSize >> 16);
    dibHeader[23] = static_cast<uint8_t>(imageSize >> 24);
    std::fwrite(dibHeader, 1, 40, f);

    // Write pixels: BMP expects BGR order
    std::vector<uint8_t> rowBuffer(stride, 0);
    for (int y = 0; y < height; ++y) {
        const uint8_t* srcRow = &pixels[static_cast<size_t>(y * width * 3)];
        for (int x = 0; x < width; ++x) {
            rowBuffer[static_cast<size_t>(x * 3 + 0)] = srcRow[x * 3 + 2];  // Blue
            rowBuffer[static_cast<size_t>(x * 3 + 1)] = srcRow[x * 3 + 1];  // Green
            rowBuffer[static_cast<size_t>(x * 3 + 2)] = srcRow[x * 3 + 0];  // Red
        }
        std::fwrite(rowBuffer.data(), 1, static_cast<size_t>(stride), f);
    }

    std::fclose(f);
    return true;
}

}  // namespace af
