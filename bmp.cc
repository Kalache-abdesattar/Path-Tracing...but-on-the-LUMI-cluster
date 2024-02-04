#include "bmp.hh"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <memory>

void write_bmp(
    const char* name,
    uint32_t w,
    uint32_t h,
    uint32_t stride,
    uint32_t pitch,
    const uint8_t* color_data
){
    uint32_t out_pitch = (w * 3 + 3) / 4 * 4;
    uint32_t file_size = 54 + out_pitch * h;
    std::unique_ptr<uint8_t[]> data(new uint8_t[file_size]);
    uint8_t* header = data.get();

    // BMP Header
    memcpy(header+0x00, "BM", 2);
    memcpy(header+0x02, &file_size, sizeof(file_size));
    uint32_t size = 54;
    memcpy(header+0x0A, &size, sizeof(size));

    // DIB Header
    size = 40;
    memcpy(header+0x0E, &size, sizeof(size));
    memcpy(header+0x12, &w, sizeof(w));
    memcpy(header+0x16, &h, sizeof(h));
    uint16_t planes = 1;
    uint16_t bpp = 24;
    memcpy(header+0x1A, &planes, sizeof(planes));
    memcpy(header+0x1C, &bpp, sizeof(bpp));
    uint32_t format = 0;
    memcpy(header+0x1E, &format, sizeof(format));
    size = out_pitch * h;
    memcpy(header+0x22, &size, sizeof(size));
    uint32_t ppm = 2835;
    memcpy(header+0x26, &ppm, sizeof(ppm));
    memcpy(header+0x2A, &ppm, sizeof(ppm));
    size = 0;
    memcpy(header+0x2E, &size, sizeof(size));
    memcpy(header+0x32, &size, sizeof(size));

    uint8_t* pixels = data.get() + 54;

    // Data
    for(uint32_t y = 0; y < h; ++y)
    for(uint32_t x = 0; x < w; ++x)
    for(uint32_t c = 0; c < 3; ++c)
        pixels[y * out_pitch + x * 3 + c] = color_data[(h-1-y) * pitch + x * stride + c];

    FILE* out = fopen(name, "w");
    if(!out)
    {
        fprintf(stderr, "Failed to write %s\n", name);
        exit(1);
    }

    fwrite(data.get(), 1, file_size, out);
    fclose(out);
}
