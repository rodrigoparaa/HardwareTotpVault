#ifndef PSP_RTC_H
#define PSP_RTC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the RTC subsystem.
 */
void psp_rtc_init(void);

/**
 * Returns the current Unix Epoch in seconds (UTC).
 * Incorporates the manual drift offset.
 */
uint64_t psp_rtc_get_utc_sec(int16_t drift_sec);

/**
 * Returns the current Unix Epoch in milliseconds (UTC).
 * Incorporates the manual drift offset.
 */
uint64_t psp_rtc_get_utc_ms(int16_t drift_sec);

/**
 * Returns raw system uptime in milliseconds (from sceRtc or clock)
 * for frame timing and delta calculations.
 */
uint64_t psp_rtc_get_ticks_ms(void);

/**
 * Formats a Unix timestamp into a readable time string (e.g. "14:02:19").
 */
void psp_rtc_format_time(uint64_t epoch_sec, char *out, size_t out_max, int is_24h);

#ifdef __cplusplus
}
#endif

#endif /* PSP_RTC_H */
