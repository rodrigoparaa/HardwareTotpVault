#include "psp_rtc.h"
#include <stdio.h>
#include <time.h>

#if defined(PSP) || defined(__PSP__)
#include <psprtc.h>
#include <psputility.h>
#else
#include <sys/time.h>
#endif

void psp_rtc_init(void) {
    /* No special initialization required for sceRtc */
}

uint64_t psp_rtc_get_ticks_ms(void) {
#if defined(PSP) || defined(__PSP__)
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    return (uint64_t)(tick / 1000ULL);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000ULL) + ((uint64_t)tv.tv_usec / 1000ULL);
#endif
}

uint64_t psp_rtc_get_utc_sec(int16_t drift_sec) {
#if defined(PSP) || defined(__PSP__)
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    /* Microseconds between 0001-01-01 and 1970-01-01 (Unix Epoch) */
    int64_t epoch_sec = (int64_t)((tick - 62135596800000000ULL) / 1000000ULL);
    return (uint64_t)(epoch_sec + drift_sec);
#else
    return (uint64_t)(time(NULL) + drift_sec);
#endif
}

uint64_t psp_rtc_get_utc_ms(int16_t drift_sec) {
#if defined(PSP) || defined(__PSP__)
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    int64_t epoch_ms = (int64_t)((tick - 62135596800000000ULL) / 1000ULL);
    return (uint64_t)(epoch_ms + ((int64_t)drift_sec * 1000LL));
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t epoch_ms = ((int64_t)tv.tv_sec * 1000LL) + ((int64_t)tv.tv_usec / 1000LL);
    return (uint64_t)(epoch_ms + ((int64_t)drift_sec * 1000LL));
#endif
}

void psp_rtc_format_time(uint64_t epoch_sec, char *out, size_t out_max, int is_24h) {
    if (!out || out_max == 0) return;

    time_t raw = (time_t)epoch_sec;
    struct tm *t = gmtime(&raw);
    if (!t) {
        snprintf(out, out_max, "--:--:--");
        return;
    }

    if (is_24h) {
        snprintf(out, out_max, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    } else {
        int hour = t->tm_hour % 12;
        if (hour == 0) hour = 12;
        const char *ampm = (t->tm_hour >= 12) ? "PM" : "AM";
        snprintf(out, out_max, "%02d:%02d:%02d %s", hour, t->tm_min, t->tm_sec, ampm);
    }
}
