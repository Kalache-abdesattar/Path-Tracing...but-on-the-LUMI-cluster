#ifndef BMP_HH
#define BMP_HH
#include <cstdint>

/* Writes a 3-channel 8-bit BMP image file.
 *
 * Parameters:
 *     name:       output path of the file
 *     w:          width of the image
 *     h:          height of the image
 *     stride:     distance (in bytes) between pixels in `color_data`
 *     pitch:      distance (in bytes) between rows in `color_data`
 *     color_data: pointer to pitch*h bytes of image data, in blue-green-red order.
 */
void write_bmp(
    const char* name,
    uint32_t w,
    uint32_t h,
    uint32_t stride,
    uint32_t pitch,
    const uint8_t* color_data
);

#endif
