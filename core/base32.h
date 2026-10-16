#ifndef BASE32_H
#define BASE32_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decodes an RFC 4648 Base32 string into raw bytes.
 * Handles case-insensitivity and strips spaces/hyphens/padding '='.
 *
 * @param encoded Null-terminated Base32 string.
 * @param out Output buffer for decoded bytes.
 * @param out_max Maximum capacity of out buffer.
 * @param out_len Pointer to receive the actual number of decoded bytes.
 * @return 0 on success, -1 on invalid character or buffer overflow.
 */
int base32_decode(const char *encoded, uint8_t *out, size_t out_max, size_t *out_len);

/**
 * Encodes raw bytes into a null-terminated RFC 4648 Base32 string with padding.
 *
 * @param in Input bytes.
 * @param in_len Number of input bytes.
 * @param out Output buffer for Base32 string.
 * @param out_max Maximum capacity of output buffer (including null terminator).
 * @return 0 on success, -1 if output buffer is too small.
 */
int base32_encode(const uint8_t *in, size_t in_len, char *out, size_t out_max);

#ifdef __cplusplus
}
#endif

#endif /* BASE32_H */
