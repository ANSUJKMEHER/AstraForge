// Image decoding test: writes a real 2x2 24-bit BMP to a temp path and
// decodes it with stb_image via af::LoadImage. The BMP bytes are constructed
// per the BITMAPFILEHEADER/BITMAPINFOHEADER layout — no external asset files
// in the repository.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "af/resources/Image.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

void WriteLe16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>(v >> 8));
}

void WriteLe32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

// 2x2, 24-bit, bottom-up rows, rows padded to 4 bytes.
std::vector<uint8_t> MakeBmp2x2() {
    const int width = 2;
    const int height = 2;
    const int rowSize = (width * 3 + 3) & ~3;  // 8
    const int dataSize = rowSize * height;     // 16
    const int fileSize = 14 + 40 + dataSize;   // 70

    std::vector<uint8_t> b;
    b.push_back('B');
    b.push_back('M');
    WriteLe32(b, static_cast<uint32_t>(fileSize));
    WriteLe32(b, 0);  // reserved
    WriteLe32(b, 54); // data offset
    WriteLe32(b, 40); // info header size
    WriteLe32(b, static_cast<uint32_t>(width));
    WriteLe32(b, static_cast<uint32_t>(height));
    WriteLe16(b, 1);   // planes
    WriteLe16(b, 24);  // bpp
    WriteLe32(b, 0);   // compression = BI_RGB
    WriteLe32(b, static_cast<uint32_t>(dataSize));
    WriteLe32(b, 2835);  // ppm X
    WriteLe32(b, 2835);  // ppm Y
    WriteLe32(b, 0);     // colors used
    WriteLe32(b, 0);     // colors important

    // Bottom row first, BGR order: bottom-left red, bottom-right green.
    b.push_back(0x00); b.push_back(0x00); b.push_back(0xFF);  // red
    b.push_back(0x00); b.push_back(0xFF); b.push_back(0x00);  // green
    b.push_back(0x00); b.push_back(0x00);                     // pad to 8
    // Top row: blue, white.
    b.push_back(0xFF); b.push_back(0x00); b.push_back(0x00);  // blue
    b.push_back(0xFF); b.push_back(0xFF); b.push_back(0xFF);  // white
    b.push_back(0x00); b.push_back(0x00);                     // pad to 8
    return b;
}

std::string TempPath() {
    static int counter = 0;
    const auto dir = std::filesystem::temp_directory_path() /
                     ("af_test_image_" + std::to_string(counter++) + ".bmp");
    return dir.string();
}

}  // namespace

AF_TEST("bmp decode returns correct dimensions and pixels") {
    const std::vector<uint8_t> bytes = MakeBmp2x2();
    const std::string path = TempPath();
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }

    const Image img = LoadImage(path);
    std::remove(path.c_str());

    AF_CHECK(img.IsValid());
    AF_CHECK_EQ(img.width, 2);
    AF_CHECK_EQ(img.height, 2);
    AF_CHECK_EQ(img.channels, 3);
    AF_CHECK_EQ(img.pixels.size(), 12u);

    // The 2x2 image must contain exactly these four RGB colors (stb converts
    // BMP's BGR order to RGB; orientation may flip, so compare as a set).
    const uint8_t expected[4][3] = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255},
    };
    int matched = 0;
    for (const auto& want : expected) {
        for (std::size_t i = 0; i + 2 < img.pixels.size(); i += 3) {
            if (img.pixels[i] == want[0] && img.pixels[i + 1] == want[1] &&
                img.pixels[i + 2] == want[2]) {
                ++matched;
                break;
            }
        }
    }
    AF_CHECK_EQ(matched, 4);
}

AF_TEST("missing file produces an invalid image without throwing") {
    const auto missing =
        (std::filesystem::temp_directory_path() / "af_definitely_missing_12345.png")
            .string();
    const Image img = LoadImage(missing);
    AF_CHECK(!img.IsValid());
    AF_CHECK_EQ(img.width, 0);
    AF_CHECK_EQ(img.pixels.size(), 0u);
}

AF_TEST("corrupt data produces an invalid image without throwing") {
    const std::string path = TempPath();
    {
        std::ofstream out(path, std::ios::binary);
        out << "this is not an image at all";
    }
    const Image img = LoadImage(path);
    std::remove(path.c_str());
    AF_CHECK(!img.IsValid());
}
