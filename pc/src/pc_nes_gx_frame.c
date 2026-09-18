#include "pc_nes_gx_frame.h"

static uint16_t pc_nes_fixnes_to_rgb565(uint16_t pixel) {
    return (uint16_t)(((pixel & 0x001fu) << 11) |
                      (pixel & 0x07e0u) |
                      ((pixel & 0xf800u) >> 11));
}

int pc_nes_convert_frame_to_gx(const uint16_t* source, size_t source_pixels,
                               uint8_t* destination, size_t destination_bytes) {
    if (source == NULL || destination == NULL ||
        source_pixels < (size_t)PC_NES_FRAME_WIDTH * PC_NES_FRAME_HEIGHT ||
        destination_bytes < PC_NES_GX_TEXTURE_BYTES) {
        return 0;
    }

    size_t output = 0;
    for (int tile_y = 0; tile_y < PC_NES_VISIBLE_HEIGHT; tile_y += 4) {
        for (int tile_x = 0; tile_x < PC_NES_FRAME_WIDTH; tile_x += 4) {
            for (int y = 0; y < 4; ++y) {
                const size_t source_row =
                    (size_t)(PC_NES_VISIBLE_TOP + tile_y + y) * PC_NES_FRAME_WIDTH;
                for (int x = 0; x < 4; ++x) {
                    const uint16_t pixel = pc_nes_fixnes_to_rgb565(
                        source[source_row + (size_t)tile_x + x]);
                    destination[output++] = (uint8_t)(pixel >> 8);
                    destination[output++] = (uint8_t)pixel;
                }
            }
        }
    }
    return 1;
}
