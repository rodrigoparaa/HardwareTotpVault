#ifndef CRYPTO_CHACHA20_H
#define CRYPTO_CHACHA20_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHACHA20_KEY_SIZE   32
#define CHACHA20_NONCE_SIZE 12
#define CHACHA20_BLOCK_SIZE 64

typedef struct {
    uint32_t state[16];
    uint8_t  keystream[CHACHA20_BLOCK_SIZE];
    size_t   keystream_pos;
} chacha20_ctx_t;

void chacha20_init(chacha20_ctx_t *ctx,
                   const uint8_t key[CHACHA20_KEY_SIZE],
                   const uint8_t nonce[CHACHA20_NONCE_SIZE],
                   uint32_t initial_counter);

void chacha20_crypt(chacha20_ctx_t *ctx,
                    const uint8_t *in,
                    uint8_t *out,
                    size_t len);

void chacha20_xor(const uint8_t key[CHACHA20_KEY_SIZE],
                  const uint8_t nonce[CHACHA20_NONCE_SIZE],
                  uint32_t initial_counter,
                  const uint8_t *in,
                  uint8_t *out,
                  size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_CHACHA20_H */
