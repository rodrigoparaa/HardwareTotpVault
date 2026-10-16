#ifndef OTP_H
#define OTP_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OTP_ALGO_SHA1   = 0,
    OTP_ALGO_SHA256 = 1
} otp_algo_t;

typedef enum {
    OTP_TYPE_TOTP = 0,
    OTP_TYPE_HOTP = 1
} otp_type_t;

/**
 * Computes an RFC 4226 HOTP numerical value.
 *
 * @param secret Raw binary secret.
 * @param secret_len Length of the secret in bytes.
 * @param counter 64-bit moving factor / event counter.
 * @param digits Number of decimal digits (6 or 8).
 * @param algo Hash algorithm (OTP_ALGO_SHA1 or OTP_ALGO_SHA256).
 * @return Decimal OTP code (e.g., 287082).
 */
uint32_t hotp_generate(const uint8_t *secret, size_t secret_len,
                      uint64_t counter, int digits, otp_algo_t algo);

/**
 * Computes an RFC 6238 TOTP numerical value for a given timestamp.
 *
 * @param secret Raw binary secret.
 * @param secret_len Length of the secret in bytes.
 * @param timestamp_sec Unix timestamp in UTC (seconds since 1970-01-01).
 * @param period_sec Time step in seconds (normally 30).
 * @param digits Number of decimal digits (6 or 8).
 * @param algo Hash algorithm (OTP_ALGO_SHA1 or OTP_ALGO_SHA256).
 * @return Decimal OTP code.
 */
uint32_t totp_generate(const uint8_t *secret, size_t secret_len,
                      uint64_t timestamp_sec, uint32_t period_sec,
                      int digits, otp_algo_t algo);

/**
 * Computes remaining seconds in current TOTP time window (0..period_sec).
 */
uint32_t totp_remaining_seconds(uint64_t timestamp_sec, uint32_t period_sec);

/**
 * Computes a smooth 0.0 to 1.0 progress ratio for 60 FPS animated bars.
 * 1.0 = just started fresh 30s window, 0.0 = expired / rotating.
 */
float totp_progress_ratio(uint64_t timestamp_ms, uint32_t period_sec);

/**
 * Formats a numerical OTP code into a display string.
 *
 * @param code Numerical value.
 * @param digits 6 or 8.
 * @param out Output buffer (at least 12 bytes).
 * @param out_max Capacity of output buffer.
 * @param with_space If 1, formats as "123 456" or "1234 5678". If 0, "123456" / "12345678".
 */
void otp_format_code(uint32_t code, int digits, char *out, size_t out_max, int with_space);

#ifdef __cplusplus
}
#endif

#endif /* OTP_H */
