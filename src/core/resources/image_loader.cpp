// stb_image implementation TU (the only place STB_IMAGE_IMPLEMENTATION is
// defined; see third_party/stb/README.md).

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_GIF
#define STBI_NO_PSD
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "stb_image.h"

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "af/resources/Image.h"

namespace af {

Image LoadImage(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data =
        stbi_load(path.c_str(), &width, &height, &channels, 0);
    Image image;
    if (data == nullptr) return image;  // invalid; stbi_failure_reason has detail
    image.width = width;
    image.height = height;
    image.channels = channels;
    const std::size_t byteCount =
        static_cast<std::size_t>(width) * height * channels;
    image.pixels.assign(data, data + byteCount);
    stbi_image_free(data);
    return image;
}

}  // namespace af
