#ifndef PC_NES_GX_FRAME_H
#define PC_NES_GX_FRAME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PC_NES_FRAME_WIDTH = 256,
    PC_NES_FRAME_HEIGHT = 240,
    PC_NES_VISIBLE_TOP = 8,
    PC_NES_VISIBLE_HEIGHT = 224,
    PC_NES_GX_TEXTURE_BYTES = PC_NES_FRAME_WIDTH * PC_NES_VISIBLE_HEIGHT * 2,
};

/* Converts fixNES's linear, red-in-low-bits RGB565 framebuffer into the
 * GameCube's 4x4-tiled, big-endian RGB565 texture layout. */
int pc_nes_convert_frame_to_gx(const uint16_t* source, size_t source_pixels,
                               uint8_t* destination, size_t destination_bytes);

#ifdef __cplusplus
}
#endif

#endif
