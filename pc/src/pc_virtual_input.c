#include "pc_virtual_input.h"

PCVirtualPadState g_pc_virtual_pad_state = {0};

void pc_pad_set_virtual_state(u16 buttons, s8 stick_x, s8 stick_y, s8 cstick_x, s8 cstick_y, u8 trigger_left,
                              u8 trigger_right) {
    g_pc_virtual_pad_state.buttons = buttons;
    g_pc_virtual_pad_state.stick_x = stick_x;
    g_pc_virtual_pad_state.stick_y = stick_y;
    g_pc_virtual_pad_state.cstick_x = cstick_x;
    g_pc_virtual_pad_state.cstick_y = cstick_y;
    g_pc_virtual_pad_state.trigger_left = trigger_left;
    g_pc_virtual_pad_state.trigger_right = trigger_right;
}

void pc_pad_clear_virtual_state(void) {
    pc_pad_set_virtual_state(0, 0, 0, 0, 0, 0, 0);
}
