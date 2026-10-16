#include "psp_input.h"
#include <string.h>

#if defined(PSP) || defined(__PSP__)
#include <pspctrl.h>
#endif

static uint32_t g_last_buttons = 0;

void psp_input_init(void) {
#if defined(PSP) || defined(__PSP__)
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
#endif
    g_last_buttons = 0;
}

void psp_input_poll(psp_input_frame_t *frame) {
    if (!frame) return;

#if defined(PSP) || defined(__PSP__)
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);

    frame->buttons_down     = pad.Buttons;
    frame->buttons_pressed  = pad.Buttons & ~g_last_buttons;
    frame->buttons_released = g_last_buttons & ~pad.Buttons;

    /* Center is 128 (0x80) */
    frame->analog_x = (int8_t)((int)pad.Lx - 128);
    frame->analog_y = (int8_t)((int)pad.Ly - 128);

    g_last_buttons = pad.Buttons;
#else
    memset(frame, 0, sizeof(psp_input_frame_t));
#endif
}
