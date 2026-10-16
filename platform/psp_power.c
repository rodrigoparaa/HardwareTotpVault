#include "psp_power.h"

#if defined(PSP) || defined(__PSP__)
#include <pspkernel.h>
#include <psppower.h>

static psp_exit_callback_t g_user_exit_cb = NULL;

static int exit_callback_handler(int arg1, int arg2, void *common) {
    (void)arg1; (void)arg2; (void)common;
    if (g_user_exit_cb) {
        g_user_exit_cb();
    }
    sceKernelExitGame();
    return 0;
}

static int callback_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback_handler, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}
#endif

void psp_power_init(psp_exit_callback_t exit_cb) {
#if defined(PSP) || defined(__PSP__)
    g_user_exit_cb = exit_cb;
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    psp_power_set_max_clock();
#else
    (void)exit_cb;
#endif
}

void psp_power_tick(void) {
#if defined(PSP) || defined(__PSP__)
    scePowerTick(PSP_POWER_TICK_ALL);
#endif
}

void psp_power_set_max_clock(void) {
#if defined(PSP) || defined(__PSP__)
    scePowerSetClockFrequency(333, 333, 166);
#endif
}

int psp_power_get_battery_percent(void) {
#if defined(PSP) || defined(__PSP__)
    return scePowerGetBatteryLifePercent();
#else
    return 95; /* Mock for host */
#endif
}

int psp_power_is_charging(void) {
#if defined(PSP) || defined(__PSP__)
    return scePowerIsBatteryCharging();
#else
    return 0;
#endif
}

int psp_power_is_online(void) {
#if defined(PSP) || defined(__PSP__)
    return scePowerIsPowerOnline();
#else
    return 1;
#endif
}
