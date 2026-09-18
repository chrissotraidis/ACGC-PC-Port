#include "pc_virtual_input.h"

#ifdef AURORA
#include <dolphin/pad.h>
#endif

PCVirtualPadState g_pc_virtual_pad_state = {0};

#ifdef AURORA
__attribute__((weak)) int bellpad_copy_normalized_pad_state(u16* buttons, s8* stick_x, s8* stick_y,
                                                             s8* cstick_x, s8* cstick_y,
                                                             u8* trigger_left, u8* trigger_right) {
    (void)buttons;
    (void)stick_x;
    (void)stick_y;
    (void)cstick_x;
    (void)cstick_y;
    (void)trigger_left;
    (void)trigger_right;
    return FALSE;
}
#endif

void pc_pad_set_virtual_state(u16 buttons, s8 stick_x, s8 stick_y, s8 cstick_x, s8 cstick_y, u8 trigger_left,
                              u8 trigger_right) {
    g_pc_virtual_pad_state.buttons = buttons;
    g_pc_virtual_pad_state.stick_x = stick_x;
    g_pc_virtual_pad_state.stick_y = stick_y;
    g_pc_virtual_pad_state.cstick_x = cstick_x;
    g_pc_virtual_pad_state.cstick_y = cstick_y;
    g_pc_virtual_pad_state.trigger_left = trigger_left;
    g_pc_virtual_pad_state.trigger_right = trigger_right;
#ifdef AURORA
    PADStatus status = {0};
    status.button = buttons;
    status.stickX = stick_x;
    status.stickY = stick_y;
    status.substickX = cstick_x;
    status.substickY = cstick_y;
    status.triggerLeft = trigger_left;
    status.triggerRight = trigger_right;
    status.err = PAD_ERR_NONE;
    PADSetVirtualStatus(PAD_CHAN0, &status);
#endif
}

void pc_pad_clear_virtual_state(void) {
    pc_pad_set_virtual_state(0, 0, 0, 0, 0, 0, 0);
#ifdef AURORA
    PADClearVirtualStatus(PAD_CHAN0);
#endif
}

#ifdef AURORA
void pc_pad_poll_normalized_state(void) {
    PCVirtualPadState state;
    if (bellpad_copy_normalized_pad_state(&state.buttons, &state.stick_x, &state.stick_y,
                                          &state.cstick_x, &state.cstick_y,
                                          &state.trigger_left, &state.trigger_right)) {
        pc_pad_set_virtual_state(state.buttons, state.stick_x, state.stick_y,
                                 state.cstick_x, state.cstick_y,
                                 state.trigger_left, state.trigger_right);
    }
}
#endif
