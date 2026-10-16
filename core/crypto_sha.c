#include "crypto_sha.h"
#include <string.h>

#define ROTL32(x, n) (((uint32_t)(x) << (n)) | ((uint32_t)(x) >> (32 - (n))))
#define ROTR32(x, n) (((uint32_t)(x) >> (n)) | ((uint32_t)(x) << (32 - (n))))

/* =========================================================================
 * SHA-1 Implementation (RFC 3174)
 * ========================================================================= */

void sha1_init(sha1_ctx_t *ctx) {
    if (!ctx) return;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count = 0;
}

static void sha1_transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t w[80];
    int t;

    for (t = 0; t < 16; t++) {
        w[t] = ((uint32_t)buffer[t * 4] << 24) |
               ((uint32_t)buffer[t * 4 + 1] << 16) |
               ((uint32_t)buffer[t * 4 + 2] << 8) |
               ((uint32_t)buffer[t * 4 + 3]);
    }

    for (t = 16; t < 80; t++) {
        w[t] = ROTL32(w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16], 1);
    }

    for (t = 0; t < 20; t++) {
        uint32_t f = (b & c) | ((~b) & d);
        uint32_t k = 0x5A827999;
        uint32_t temp = ROTL32(a, 5) + f + e + k + w[t];
        e = d;
        d = c;
        c = ROTL32(b, 30);
        b = a;
        a = temp;
    }

    for (t = 20; t < 40; t++) {
        uint32_t f = b ^ c ^ d;
        uint32_t k = 0x6ED9EBA1;
        uint32_t temp = ROTL32(a, 5) + f + e + k + w[t];
        e = d;
        d = c;
        c = ROTL32(b, 30);
        b = a;
        a = temp;
    }

    for (t = 40; t < 60; t++) {
        uint32_t f = (b & c) | (b & d) | (c & d);
        uint32_t k = 0x8F1BBCDC;
        uint32_t temp = ROTL32(a, 5) + f + e + k + w[t];
        e = d;
        d = c;
        c = ROTL32(b, 30);
        b = a;
        a = temp;
    }

    for (t = 60; t < 80; t++) {
        uint32_t f = b ^ c ^ d;
        uint32_t k = 0xCA62C1D6;
        uint32_t temp = ROTL32(a, 5) + f + e + k + w[t];
        e = d;
        d = c;
        c = ROTL32(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void sha1_update(sha1_ctx_t *ctx, const void *data, size_t len) {
    if (!ctx || !data || len == 0) return;
    const uint8_t *input = (const uint8_t *)data;
    size_t buffer_idx = (size_t)(ctx->count % SHA1_BLOCK_SIZE);
    ctx->count += len;

    size_t part_len = SHA1_BLOCK_SIZE - buffer_idx;
    size_t i = 0;

    if (len >= part_len) {
        memcpy(&ctx->buffer[buffer_idx], input, part_len);
        sha1_transform(ctx->state, ctx->buffer);
        for (i = part_len; i + SHA1_BLOCK_SIZE <= len; i += SHA1_BLOCK_SIZE) {
            sha1_transform(ctx->state, &input[i]);
        }
        buffer_idx = 0;
    }

    if (i < len) {
        memcpy(&ctx->buffer[buffer_idx], &input[i], len - i);
    }
}

void sha1_final(sha1_ctx_t *ctx, uint8_t digest[SHA1_DIGEST_SIZE]) {
    if (!ctx || !digest) return;
    uint8_t final_count[8];
    uint64_t total_bits = ctx->count * 8;

    for (int i = 0; i < 8; i++) {
        final_count[i] = (uint8_t)((total_bits >> ((7 - i) * 8)) & 0xFF);
    }

    uint8_t pad = 0x80;
    sha1_update(ctx, &pad, 1);

    uint8_t zero = 0x00;
    while ((ctx->count % SHA1_BLOCK_SIZE) != 56) {
        sha1_update(ctx, &zero, 1);
    }

    sha1_update(ctx, final_count, 8);

    for (int i = 0; i < 5; i++) {
        digest[i * 4]     = (uint8_t)((ctx->state[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i] & 0xFF);
    }
}

void sha1(const void *data, size_t len, uint8_t digest[SHA1_DIGEST_SIZE]) {
    sha1_ctx_t ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, digest);
}

/* =========================================================================
 * SHA-256 Implementation (FIPS 180-4)
 * ========================================================================= */

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROTR32(x, 2) ^ ROTR32(x, 13) ^ ROTR32(x, 22))
#define EP1(x)       (ROTR32(x, 6) ^ ROTR32(x, 11) ^ ROTR32(x, 25))
#define SIG0(x)      (ROTR32(x, 7) ^ ROTR32(x, 18) ^ ((x) >> 3))
#define SIG1(x)      (ROTR32(x, 17) ^ ROTR32(x, 19) ^ ((x) >> 10))

void sha256_init(sha256_ctx_t *ctx) {
    if (!ctx) return;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

static void sha256_transform(uint32_t state[8], const uint8_t buffer[64]) {
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];
    uint32_t w[64];
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)buffer[i * 4] << 24) |
               ((uint32_t)buffer[i * 4 + 1] << 16) |
               ((uint32_t)buffer[i * 4 + 2] << 8) |
               ((uint32_t)buffer[i * 4 + 3]);
    }

    for (i = 16; i < 64; i++) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }

    for (i = 0; i < 64; i++) {
        uint32_t t1 = h + EP1(e) + CH(e, f, g) + K256[i] + w[i];
        uint32_t t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void sha256_update(sha256_ctx_t *ctx, const void *data, size_t len) {
    if (!ctx || !data || len == 0) return;
    const uint8_t *input = (const uint8_t *)data;
    size_t buffer_idx = (size_t)(ctx->count % SHA256_BLOCK_SIZE);
    ctx->count += len;

    size_t part_len = SHA256_BLOCK_SIZE - buffer_idx;
    size_t i = 0;

    if (len >= part_len) {
        memcpy(&ctx->buffer[buffer_idx], input, part_len);
        sha256_transform(ctx->state, ctx->buffer);
        for (i = part_len; i + SHA256_BLOCK_SIZE <= len; i += SHA256_BLOCK_SIZE) {
            sha256_transform(ctx->state, &input[i]);
        }
        buffer_idx = 0;
    }

    if (i < len) {
        memcpy(&ctx->buffer[buffer_idx], &input[i], len - i);
    }
}

void sha256_final(sha256_ctx_t *ctx, uint8_t digest[SHA256_DIGEST_SIZE]) {
    if (!ctx || !digest) return;
    uint8_t final_count[8];
    uint64_t total_bits = ctx->count * 8;

    for (int i = 0; i < 8; i++) {
        final_count[i] = (uint8_t)((total_bits >> ((7 - i) * 8)) & 0xFF);
    }

    uint8_t pad = 0x80;
    sha256_update(ctx, &pad, 1);

    uint8_t zero = 0x00;
    while ((ctx->count % SHA256_BLOCK_SIZE) != 56) {
        sha256_update(ctx, &zero, 1);
    }

    sha256_update(ctx, final_count, 8);

    for (int i = 0; i < 8; i++) {
        digest[i * 4]     = (uint8_t)((ctx->state[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i] & 0xFF);
    }
}

void sha256(const void *data, size_t len, uint8_t digest[SHA256_DIGEST_SIZE]) {
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, digest);
}
