#include "crypto_chacha20.h"
#include "crypto_hmac.h"
#include <string.h>

#define ROTL32(x, n) (((uint32_t)(x) << (n)) | ((uint32_t)(x) >> (32 - (n))))

#define QR(s, a, b, c, d) do { \
    s[a] += s[b]; s[d] ^= s[a]; s[d] = ROTL32(s[d], 16); \
    s[c] += s[d]; s[b] ^= s[c]; s[b] = ROTL32(s[b], 12); \
    s[a] += s[b]; s[d] ^= s[a]; s[d] = ROTL32(s[d], 8);  \
    s[c] += s[d]; s[b] ^= s[c]; s[b] = ROTL32(s[b], 7);  \
} while(0)

static inline uint32_t load32_le(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static inline void store32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

void chacha20_init(chacha20_ctx_t *ctx,
                   const uint8_t key[CHACHA20_KEY_SIZE],
                   const uint8_t nonce[CHACHA20_NONCE_SIZE],
                   uint32_t initial_counter) {
    if (!ctx || !key || !nonce) return;

    /* Constants: "expand 32-byte k" */
    ctx->state[0] = 0x61707865;
    ctx->state[1] = 0x3320646e;
    ctx->state[2] = 0x79622d32;
    ctx->state[3] = 0x6b206574;

    /* 256-bit Key */
    ctx->state[4] = load32_le(key + 0);
    ctx->state[5] = load32_le(key + 4);
    ctx->state[6] = load32_le(key + 8);
    ctx->state[7] = load32_le(key + 12);
    ctx->state[8] = load32_le(key + 16);
    ctx->state[9] = load32_le(key + 20);
    ctx->state[10] = load32_le(key + 24);
    ctx->state[11] = load32_le(key + 28);

    /* Counter & Nonce */
    ctx->state[12] = initial_counter;
    ctx->state[13] = load32_le(nonce + 0);
    ctx->state[14] = load32_le(nonce + 4);
    ctx->state[15] = load32_le(nonce + 8);

    ctx->keystream_pos = CHACHA20_BLOCK_SIZE; /* Force block generation on first read */
}

static void chacha20_block(chacha20_ctx_t *ctx, uint8_t output[CHACHA20_BLOCK_SIZE]) {
    uint32_t s[16];
    memcpy(s, ctx->state, sizeof(s));

    for (int i = 0; i < 10; i++) {
        /* Column rounds */
        QR(s, 0, 4, 8, 12);
        QR(s, 1, 5, 9, 13);
        QR(s, 2, 6, 10, 14);
        QR(s, 3, 7, 11, 15);

        /* Diagonal rounds */
        QR(s, 0, 5, 10, 15);
        QR(s, 1, 6, 11, 12);
        QR(s, 2, 7, 8, 13);
        QR(s, 3, 4, 9, 14);
    }

    for (int i = 0; i < 16; i++) {
        store32_le(output + (i * 4), s[i] + ctx->state[i]);
    }

    /* Increment block counter */
    ctx->state[12]++;
}

void chacha20_crypt(chacha20_ctx_t *ctx,
                    const uint8_t *in,
                    uint8_t *out,
                    size_t len) {
    if (!ctx || !out || len == 0) return;

    for (size_t i = 0; i < len; i++) {
        if (ctx->keystream_pos >= CHACHA20_BLOCK_SIZE) {
            chacha20_block(ctx, ctx->keystream);
            ctx->keystream_pos = 0;
        }
        uint8_t in_byte = in ? in[i] : 0x00;
        out[i] = in_byte ^ ctx->keystream[ctx->keystream_pos++];
    }
}

void chacha20_xor(const uint8_t key[CHACHA20_KEY_SIZE],
                  const uint8_t nonce[CHACHA20_NONCE_SIZE],
                  uint32_t initial_counter,
                  const uint8_t *in,
                  uint8_t *out,
                  size_t len) {
    chacha20_ctx_t ctx;
    chacha20_init(&ctx, key, nonce, initial_counter);
    chacha20_crypt(&ctx, in, out, len);
    crypto_secure_zero(&ctx, sizeof(ctx));
}
