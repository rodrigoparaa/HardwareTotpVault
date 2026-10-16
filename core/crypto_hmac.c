#include "crypto_hmac.h"
#include <string.h>

void hmac_sha1(const void *key, size_t key_len,
               const void *data, size_t data_len,
               uint8_t out[SHA1_DIGEST_SIZE]) {
    uint8_t k_pad[SHA1_BLOCK_SIZE];
    uint8_t tk[SHA1_DIGEST_SIZE];

    if (key_len > SHA1_BLOCK_SIZE) {
        sha1(key, key_len, tk);
        key = tk;
        key_len = SHA1_DIGEST_SIZE;
    }

    memset(k_pad, 0, sizeof(k_pad));
    memcpy(k_pad, key, key_len);

    uint8_t ipad[SHA1_BLOCK_SIZE];
    uint8_t opad[SHA1_BLOCK_SIZE];
    for (int i = 0; i < SHA1_BLOCK_SIZE; i++) {
        ipad[i] = k_pad[i] ^ 0x36;
        opad[i] = k_pad[i] ^ 0x5C;
    }

    sha1_ctx_t ctx;
    uint8_t inner_hash[SHA1_DIGEST_SIZE];

    sha1_init(&ctx);
    sha1_update(&ctx, ipad, SHA1_BLOCK_SIZE);
    sha1_update(&ctx, data, data_len);
    sha1_final(&ctx, inner_hash);

    sha1_init(&ctx);
    sha1_update(&ctx, opad, SHA1_BLOCK_SIZE);
    sha1_update(&ctx, inner_hash, SHA1_DIGEST_SIZE);
    sha1_final(&ctx, out);

    crypto_secure_zero(k_pad, sizeof(k_pad));
    crypto_secure_zero(tk, sizeof(tk));
    crypto_secure_zero(ipad, sizeof(ipad));
    crypto_secure_zero(opad, sizeof(opad));
    crypto_secure_zero(inner_hash, sizeof(inner_hash));
}

void hmac_sha256(const void *key, size_t key_len,
                 const void *data, size_t data_len,
                 uint8_t out[SHA256_DIGEST_SIZE]) {
    uint8_t k_pad[SHA256_BLOCK_SIZE];
    uint8_t tk[SHA256_DIGEST_SIZE];

    if (key_len > SHA256_BLOCK_SIZE) {
        sha256(key, key_len, tk);
        key = tk;
        key_len = SHA256_DIGEST_SIZE;
    }

    memset(k_pad, 0, sizeof(k_pad));
    memcpy(k_pad, key, key_len);

    uint8_t ipad[SHA256_BLOCK_SIZE];
    uint8_t opad[SHA256_BLOCK_SIZE];
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        ipad[i] = k_pad[i] ^ 0x36;
        opad[i] = k_pad[i] ^ 0x5C;
    }

    sha256_ctx_t ctx;
    uint8_t inner_hash[SHA256_DIGEST_SIZE];

    sha256_init(&ctx);
    sha256_update(&ctx, ipad, SHA256_BLOCK_SIZE);
    sha256_update(&ctx, data, data_len);
    sha256_final(&ctx, inner_hash);

    sha256_init(&ctx);
    sha256_update(&ctx, opad, SHA256_BLOCK_SIZE);
    sha256_update(&ctx, inner_hash, SHA256_DIGEST_SIZE);
    sha256_final(&ctx, out);

    crypto_secure_zero(k_pad, sizeof(k_pad));
    crypto_secure_zero(tk, sizeof(tk));
    crypto_secure_zero(ipad, sizeof(ipad));
    crypto_secure_zero(opad, sizeof(opad));
    crypto_secure_zero(inner_hash, sizeof(inner_hash));
}

void pbkdf2_hmac_sha256(const void *password, size_t pass_len,
                        const void *salt, size_t salt_len,
                        uint32_t iterations,
                        uint8_t *out_key, size_t out_key_len) {
    if (!password || !salt || iterations == 0 || !out_key || out_key_len == 0) return;

    uint32_t block_count = (uint32_t)((out_key_len + SHA256_DIGEST_SIZE - 1) / SHA256_DIGEST_SIZE);
    uint8_t u_prev[SHA256_DIGEST_SIZE];
    uint8_t u_curr[SHA256_DIGEST_SIZE];
    uint8_t t_block[SHA256_DIGEST_SIZE];

    /* Buffer for salt || be32(block_idx), avoiding heap allocation */
    uint8_t salt_buf[128];
    if (salt_len + 4 > sizeof(salt_buf)) return;
    memcpy(salt_buf, salt, salt_len);

    size_t out_pos = 0;

    for (uint32_t block = 1; block <= block_count; block++) {
        salt_buf[salt_len]     = (uint8_t)((block >> 24) & 0xFF);
        salt_buf[salt_len + 1] = (uint8_t)((block >> 16) & 0xFF);
        salt_buf[salt_len + 2] = (uint8_t)((block >> 8) & 0xFF);
        salt_buf[salt_len + 3] = (uint8_t)(block & 0xFF);

        /* U_1 = PRF(P, S || INT_32_BE(block)) */
        hmac_sha256(password, pass_len, salt_buf, salt_len + 4, u_prev);
        memcpy(t_block, u_prev, SHA256_DIGEST_SIZE);

        for (uint32_t iter = 2; iter <= iterations; iter++) {
            hmac_sha256(password, pass_len, u_prev, SHA256_DIGEST_SIZE, u_curr);
            for (int k = 0; k < SHA256_DIGEST_SIZE; k++) {
                t_block[k] ^= u_curr[k];
            }
            memcpy(u_prev, u_curr, SHA256_DIGEST_SIZE);
        }

        size_t copy_len = out_key_len - out_pos;
        if (copy_len > SHA256_DIGEST_SIZE) {
            copy_len = SHA256_DIGEST_SIZE;
        }
        memcpy(&out_key[out_pos], t_block, copy_len);
        out_pos += copy_len;
    }

    crypto_secure_zero(salt_buf, sizeof(salt_buf));
    crypto_secure_zero(u_prev, sizeof(u_prev));
    crypto_secure_zero(u_curr, sizeof(u_curr));
    crypto_secure_zero(t_block, sizeof(t_block));
}

int crypto_constant_time_eq(const void *a, const void *b, size_t len) {
    const uint8_t *p1 = (const uint8_t *)a;
    const uint8_t *p2 = (const uint8_t *)b;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= (p1[i] ^ p2[i]);
    }
    return diff == 0;
}

void crypto_secure_zero(void *ptr, size_t len) {
    if (!ptr || len == 0) return;
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--) {
        *p++ = 0x00;
    }
}
