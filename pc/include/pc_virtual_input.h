#ifndef PC_VIRTUAL_INPUT_H
#define PC_VIRTUAL_INPUT_H

#include "pc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u16 buttons;
    s8 stick_x;
    s8 stick_y;
    s8 cstick_x;
    s8 cstick_y;
    u8 trigger_left;
    u8 trigger_right;
} PCVirtualPadState;

extern PCVirtualPadState g_pc_virtual_pad_state;

void pc_pad_set_virtual_state(u16 buttons, s8 stick_x, s8 stick_y, s8 cstick_x, s8 cstick_y, u8 trigger_left,
                              u8 trigger_right);
void pc_pad_clear_virtual_state(void);
#ifdef AURORA
void pc_pad_poll_normalized_state(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
