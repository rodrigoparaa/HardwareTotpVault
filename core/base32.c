#include "base32.h"
#include <string.h>

static const char B32_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

static int b32_char_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '2' && c <= '7') return c - '2' + 26;
    return -1;
}

int base32_decode(const char *encoded, uint8_t *out, size_t out_max, size_t *out_len) {
    if (!encoded || !out) return -1;

    uint32_t buffer = 0;
    int bits_left = 0;
    size_t count = 0;

    for (size_t i = 0; encoded[i] != '\0'; i++) {
        char c = encoded[i];
        if (c == ' ' || c == '-' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }
        if (c == '=') {
            break; /* Padding starts, end of useful data */
        }

        int val = b32_char_val(c);
        if (val < 0) {
            return -1; /* Invalid Base32 character */
        }

        buffer = (buffer << 5) | (uint32_t)val;
        bits_left += 5;

        if (bits_left >= 8) {
            bits_left -= 8;
            if (count >= out_max) {
                return -1; /* Overflow */
            }
            out[count++] = (uint8_t)((buffer >> bits_left) & 0xFF);
        }
    }

    if (out_len) {
        *out_len = count;
    }
    return 0;
}

int base32_encode(const uint8_t *in, size_t in_len, char *out, size_t out_max) {
    if (!out || out_max == 0) return -1;
    if (!in && in_len > 0) return -1;

    size_t out_idx = 0;
    uint32_t buffer = 0;
    int bits_left = 0;

    for (size_t i = 0; i < in_len; i++) {
        buffer = (buffer << 8) | in[i];
        bits_left += 8;

        while (bits_left >= 5) {
            bits_left -= 5;
            if (out_idx + 1 >= out_max) return -1;
            out[out_idx++] = B32_ALPHABET[(buffer >> bits_left) & 0x1F];
        }
    }

    if (bits_left > 0) {
        buffer <<= (5 - bits_left);
        if (out_idx + 1 >= out_max) return -1;
        out[out_idx++] = B32_ALPHABET[buffer & 0x1F];
    }

    /* Add '=' padding to multiple of 8 if room */
    while ((out_idx % 8) != 0) {
        if (out_idx + 1 >= out_max) return -1;
        out[out_idx++] = '=';
    }

    if (out_idx >= out_max) return -1;
    out[out_idx] = '\0';
    return 0;
}
