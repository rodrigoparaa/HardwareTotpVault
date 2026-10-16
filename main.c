#include <stdio.h>
#include <string.h>

#if defined(PSP) || defined(__PSP__)
#include <pspkernel.h>
#include <pspdisplay.h>
#endif

#include "core/app_state.h"
#include "core/vault.h"
#include "platform/psp_rtc.h"
#include "platform/psp_input.h"
#include "platform/psp_power.h"
#include "platform/psp_fs.h"
#include "ui/graphics.h"
#include "ui/screens.h"

#if defined(PSP) || defined(__PSP__)
/* PSP Go Safety Directives: User Mode, 512KB Stack, 20MB Heap */
PSP_MODULE_INFO("TotpVault", 0, 1, 1);
PSP_MAIN_THREAD_STACK_SIZE_KB(512);
PSP_HEAP_SIZE_KB(20 * 1024);
#endif

/* Static BSS Allocation (16-byte aligned, zero stack risk) */
static AppState g_app __attribute__((aligned(16)));
static uint8_t  g_file_buf[32768] __attribute__((aligned(16)));
static size_t   g_file_len = 0;

static void on_exit_cleanup(void) {
    vault_secure_wipe(&g_app.vault);
}

int main(void) {
    /* 1. Hardware Initialization & Safety Callbacks */
    psp_power_init(on_exit_cleanup);
    psp_rtc_init();
    psp_input_init();
    gfx_init();

    app_init(&g_app);

    /* 2. Load vault.enc from internal eMMC ef0:/ or ms0:/ */
    if (psp_fs_load_vault(g_file_buf, sizeof(g_file_buf), &g_file_len) == 0) {
        vault_parse_header(g_file_buf, g_file_len, &g_app.file_header);
        pin_input_init(&g_app.pin_input, g_app.file_header.pin_length);
    }

    psp_input_frame_t input_frame;
    int running = 1;

    /* 3. Main 60 FPS Event & Rendering Loop */
    while (running) {
        /* Anti-sleep pulse for desk dock use */
        psp_power_tick();

        /* Sample Hardware Inputs */
        psp_input_poll(&input_frame);

        uint64_t now_ticks_ms = psp_rtc_get_ticks_ms();
        uint64_t now_utc_sec  = psp_rtc_get_utc_sec(g_app.vault.time_drift_sec);
        uint64_t now_utc_ms   = psp_rtc_get_utc_ms(g_app.vault.time_drift_sec);

        /* Update State Machine */
        app_update(&g_app,
                   input_frame.buttons_down,
                   input_frame.buttons_pressed,
                   input_frame.buttons_released,
                   now_ticks_ms);

        /* Auto-trigger unlock when PIN sequence completes */
        if (g_app.screen == SCREEN_LOCKED && g_app.pin_input.trigger_submit) {
            app_try_unlock(&g_app, g_file_buf, g_file_len, now_ticks_ms);
        }

        /* 4. Render 60 FPS Frame */
        gfx_start_frame();
        screens_render(&g_app, now_utc_sec, now_utc_ms, now_ticks_ms);
        gfx_end_frame();
    }

    on_exit_cleanup();
    gfx_finish();

#if defined(PSP) || defined(__PSP__)
    sceKernelExitGame();
#endif
    return 0;
}
