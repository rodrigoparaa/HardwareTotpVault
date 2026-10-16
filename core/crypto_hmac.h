#ifndef CRYPTO_HMAC_H
#define CRYPTO_HMAC_H

#include <stdint.h>
#include <stddef.h>
#include "crypto_sha.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- HMAC-SHA1 & HMAC-SHA256 (RFC 2104) --- */
void hmac_sha1(const void *key, size_t key_len,
               const void *data, size_t data_len,
               uint8_t out[SHA1_DIGEST_SIZE]);

void hmac_sha256(const void *key, size_t key_len,
                 const void *data, size_t data_len,
                 uint8_t out[SHA256_DIGEST_SIZE]);

/* --- PBKDF2 with HMAC-SHA256 (RFC 2898 / RFC 8018) --- */
void pbkdf2_hmac_sha256(const void *password, size_t pass_len,
                        const void *salt, size_t salt_len,
                        uint32_t iterations,
                        uint8_t *out_key, size_t out_key_len);

/* --- Constant-Time Comparison & Secure Wipe --- */
int crypto_constant_time_eq(const void *a, const void *b, size_t len);
void crypto_secure_zero(void *ptr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_HMAC_H */
