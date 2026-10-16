#include "otp.h"
#include "crypto_hmac.h"
#include <stdio.h>
#include <string.h>

static const uint32_t DIGIT_MODULOS[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
};

uint32_t hotp_generate(const uint8_t *secret, size_t secret_len,
                      uint64_t counter, int digits, otp_algo_t algo) {
    if (!secret || secret_len == 0) return 0;

    /* Big-endian 8-byte serialization of counter */
    uint8_t counter_be[8];
    for (int i = 0; i < 8; i++) {
        counter_be[7 - i] = (uint8_t)((counter >> (i * 8)) & 0xFF);
    }

    uint8_t mac[SHA256_DIGEST_SIZE];
    int mac_len = 0;

    if (algo == OTP_ALGO_SHA256) {
        hmac_sha256(secret, secret_len, counter_be, 8, mac);
        mac_len = SHA256_DIGEST_SIZE;
    } else {
        hmac_sha1(secret, secret_len, counter_be, 8, mac);
        mac_len = SHA1_DIGEST_SIZE;
    }

    /* Dynamic Truncation (RFC 4226 Section 5.4) */
    int offset = mac[mac_len - 1] & 0x0F;
    uint32_t bin_code = (((uint32_t)(mac[offset] & 0x7F)) << 24) |
                        (((uint32_t)mac[offset + 1]) << 16) |
                        (((uint32_t)mac[offset + 2]) << 8) |
                        ((uint32_t)mac[offset + 3]);

    uint32_t modulo = (digits >= 1 && digits <= 9) ? DIGIT_MODULOS[digits] : 1000000;
    uint32_t otp = bin_code % modulo;

    crypto_secure_zero(mac, sizeof(mac));
    return otp;
}

uint32_t totp_generate(const uint8_t *secret, size_t secret_len,
                      uint64_t timestamp_sec, uint32_t period_sec,
                      int digits, otp_algo_t algo) {
    if (period_sec == 0) period_sec = 30;
    uint64_t counter = timestamp_sec / period_sec;
    return hotp_generate(secret, secret_len, counter, digits, algo);
}

uint32_t totp_remaining_seconds(uint64_t timestamp_sec, uint32_t period_sec) {
    if (period_sec == 0) period_sec = 30;
    uint32_t elapsed = (uint32_t)(timestamp_sec % period_sec);
    return period_sec - elapsed;
}

float totp_progress_ratio(uint64_t timestamp_ms, uint32_t period_sec) {
    if (period_sec == 0) period_sec = 30;
    uint64_t period_ms = (uint64_t)period_sec * 1000ULL;
    uint64_t elapsed_ms = timestamp_ms % period_ms;
    return 1.0f - ((float)elapsed_ms / (float)period_ms);
}

void otp_format_code(uint32_t code, int digits, char *out, size_t out_max, int with_space) {
    if (!out || out_max == 0) return;

    char raw[16];
    if (digits == 8) {
        snprintf(raw, sizeof(raw), "%08u", (unsigned int)code);
        if (with_space && out_max >= 10) {
            /* "1234 5678" */
            snprintf(out, out_max, "%.4s %.4s", raw, raw + 4);
            return;
        }
    } else {
        /* Default 6 digits */
        snprintf(raw, sizeof(raw), "%06u", (unsigned int)code);
        if (with_space && out_max >= 8) {
            /* "123 456" */
            snprintf(out, out_max, "%.3s %.3s", raw, raw + 3);
            return;
        }
    }

    snprintf(out, out_max, "%s", raw);
}
