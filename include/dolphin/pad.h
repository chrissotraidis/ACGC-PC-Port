#ifndef _DOLPHIN_PAD_H_
#define _DOLPHIN_PAD_H_

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAD_SPEC_0 0
#define PAD_SPEC_1 1
#define PAD_SPEC_2 2
#define PAD_SPEC_3 3
#define PAD_SPEC_4 4
#define PAD_SPEC_5 5

#define PAD_MOTOR_STOP 0
#define PAD_MOTOR_RUMBLE 1
#define PAD_MOTOR_STOP_HARD 2

#define PAD_CHAN0_BIT 0x80000000

#ifdef AURORA
#define PAD_CHAN0 0
#endif

#define PAD_MAX_CONTROLLERS     4

#define PAD_BUTTON_LEFT  (1 << 0)   // 0x0001
#define PAD_BUTTON_RIGHT (1 << 1)   // 0x0002
#define PAD_BUTTON_DOWN  (1 << 2)   // 0x0004
#define PAD_BUTTON_UP    (1 << 3)   // 0x0008
#define PAD_TRIGGER_Z    (1 << 4)   // 0x0010
#define PAD_TRIGGER_R    (1 << 5)   // 0x0020
#define PAD_TRIGGER_L    (1 << 6)   // 0x0040
#define PAD_BUTTON_A     (1 << 8)   // 0x0100
#define PAD_BUTTON_B     (1 << 9)   // 0x0200
#define PAD_BUTTON_X     (1 << 10)  // 0x0400
#define PAD_BUTTON_Y     (1 << 11)  // 0x0800
#define PAD_BUTTON_MENU  (1 << 12)  // 0x1000
#define PAD_BUTTON_START (1 << 12)  // 0x1000

#ifdef AURORA
#define PAD_BUTTON_COUNT 12

#define PAD_AXIS_LEFT_X_POS 0
#define PAD_AXIS_LEFT_X_NEG 1
#define PAD_AXIS_LEFT_Y_POS 2
#define PAD_AXIS_LEFT_Y_NEG 3
#define PAD_AXIS_RIGHT_X_POS 4
#define PAD_AXIS_RIGHT_X_NEG 5
#define PAD_AXIS_RIGHT_Y_POS 6
#define PAD_AXIS_RIGHT_Y_NEG 7
#define PAD_AXIS_TRIGGER_L 8
#define PAD_AXIS_TRIGGER_R 9
#define PAD_AXIS_COUNT 10
#endif

#define PAD_ERR_NONE 0
#define PAD_ERR_NO_CONTROLLER -1
#define PAD_ERR_NOT_READY -2
#define PAD_ERR_TRANSFER -3

#define RES_WIRELESS_LITE 0x40000

typedef void (*PADSamplingCallback)(void);

typedef struct PADStatus
{
    /*0x00*/ u16 button;
    /*0x02*/ s8 stickX;
    /*0x03*/ s8 stickY;
    /*0x04*/ s8 substickX;
    /*0x05*/ s8 substickY;
    /*0x06*/ u8 triggerLeft;
    /*0x07*/ u8 triggerRight;
    /*0x08*/ u8 analogA;
    /*0x09*/ u8 analogB;
    /*0x0A*/ s8 err;
#ifdef AURORA
    /* Aurora reports additional host buttons after the SDK-compatible data.
     * Keep the caller's array stride identical to Aurora's PADStatus ABI. */
    u32 extButton;
#endif
} PADStatus;

#ifdef AURORA
typedef u16 PADButton;
typedef u16 PADAxis;

typedef struct PADKeyButtonBinding {
    s32 scancode;
    PADButton padButton;
} PADKeyButtonBinding;

typedef struct PADKeyAxisBinding {
    s32 scancode;
    PADAxis padAxis;
    s16 influence;
} PADKeyAxisBinding;
#endif

// PAD.c
BOOL PADReset(u32 mask);
BOOL PADRecalibrate(u32 mask);
BOOL PADInit();
u32 PADRead(PADStatus* status);
void PADSetSamplingRate(u32 msec);
void __PADTestSamplingRate(u32 tvmode);
void PADControlAllMotors(const u32 *commandArray);
void PADControlMotor(s32 chan, u32 command);
void PADSetSpec(u32 spec);
u32 PADGetSpec();
BOOL PADGetType(s32 chan, u32* type);
BOOL PADSync(void);
void PADSetAnalogMode(u32 mode);
PADSamplingCallback PADSetSamplingCallback(PADSamplingCallback callback);

#ifdef AURORA
BOOL PADSetKeyButtonBindings(u32 port, PADKeyButtonBinding bindings[PAD_BUTTON_COUNT]);
PADKeyButtonBinding* PADGetKeyButtonBindings(u32 port, u32* buttonCount);
BOOL PADSetKeyAxisBindings(u32 port, PADKeyAxisBinding bindings[PAD_AXIS_COUNT]);
void PADSetKeyboardActive(u32 port, BOOL active);
void PADSetVirtualStatus(u32 port, const PADStatus* status);
void PADClearVirtualStatus(u32 port);
#endif

// Padclamp.c
void PADClamp(PADStatus * status);

#ifdef __cplusplus
}
#endif

#endif
