#include "vault.h"
#include "crypto_hmac.h"
#include "crypto_chacha20.h"
#include <string.h>

void vault_init(VaultPlaintext *vault) {
    if (!vault) return;
    crypto_secure_zero(vault, sizeof(VaultPlaintext));
    vault->time_drift_sec = 0;
    vault->auto_lock_minutes = 5;
    vault->default_theme = 0;
    vault->otp_count = 0;
    vault->secret_count = 0;
}

void vault_secure_wipe(VaultPlaintext *vault) {
    if (!vault) return;
    crypto_secure_zero(vault, sizeof(VaultPlaintext));
}

size_t vault_calc_file_size(void) {
    return sizeof(VaultHeader) + sizeof(VaultPlaintext) + VAULT_TAG_SIZE;
}

int vault_parse_header(const uint8_t *file_data, size_t file_len, VaultHeader *out_hdr) {
    if (!file_data || file_len < sizeof(VaultHeader) + VAULT_TAG_SIZE || !out_hdr) {
        return -1;
    }

    const VaultHeader *hdr = (const VaultHeader *)file_data;
    if (memcmp(hdr->magic, VAULT_MAGIC, 4) != 0) {
        return -2; /* Invalid magic */
    }
    if (hdr->version != VAULT_VERSION) {
        return -3; /* Unsupported version */
    }
    if (hdr->pin_length != 2 && hdr->pin_length != 4 && hdr->pin_length != 8) {
        return -4; /* Invalid PIN length */
    }
    if (hdr->payload_len != sizeof(VaultPlaintext)) {
        return -5; /* Payload size mismatch */
    }

    memcpy(out_hdr, hdr, sizeof(VaultHeader));
    return 0;
}

int vault_encrypt(const VaultPlaintext *vault,
                  const uint8_t *pin, uint8_t pin_len,
                  uint32_t kdf_iters,
                  const uint8_t *salt, const uint8_t *nonce,
                  uint8_t *out_buf, size_t out_buf_max, size_t *out_file_size) {
    if (!vault || !pin || (pin_len != 2 && pin_len != 4 && pin_len != 8) || !out_buf) {
        return -1;
    }

    size_t total_size = vault_calc_file_size();
    if (out_buf_max < total_size) {
        return -2;
    }

    VaultHeader *hdr = (VaultHeader *)out_buf;
    memcpy(hdr->magic, VAULT_MAGIC, 4);
    hdr->version = VAULT_VERSION;
    hdr->pin_length = pin_len;
    hdr->flags = 0;

    /* Check if PIN consists purely of L and R triggers */
    int lr_only = 1;
    for (int i = 0; i < pin_len; i++) {
        if (pin[i] != BTN_L && pin[i] != BTN_R) {
            lr_only = 0;
            break;
        }
    }
    if (lr_only) {
        hdr->flags |= VAULT_FLAG_LR_ONLY;
    }

    hdr->kdf_iters = (kdf_iters > 0) ? kdf_iters : 10000;

    if (salt) {
        memcpy(hdr->salt, salt, VAULT_SALT_SIZE);
    } else {
        /* Pseudo-random default fallback for tests */
        for (int i = 0; i < VAULT_SALT_SIZE; i++) {
            hdr->salt[i] = (uint8_t)(0xA5 ^ (i * 17));
        }
    }

    if (nonce) {
        memcpy(hdr->nonce, nonce, VAULT_NONCE_SIZE);
    } else {
        for (int i = 0; i < VAULT_NONCE_SIZE; i++) {
            hdr->nonce[i] = (uint8_t)(0x5A ^ (i * 23));
        }
    }

    hdr->payload_len = (uint32_t)sizeof(VaultPlaintext);

    /* 1. Derive 64-byte master key: [0..31] K_enc, [32..63] K_mac */
    uint8_t master_key[64];
    pbkdf2_hmac_sha256(pin, pin_len, hdr->salt, VAULT_SALT_SIZE,
                       hdr->kdf_iters, master_key, sizeof(master_key));

    const uint8_t *k_enc = master_key;
    const uint8_t *k_mac = master_key + 32;

    /* 2. Encrypt plaintext payload directly into output buffer */
    uint8_t *ciphertext_dest = out_buf + sizeof(VaultHeader);
    chacha20_xor(k_enc, hdr->nonce, 1,
                 (const uint8_t *)vault, ciphertext_dest, sizeof(VaultPlaintext));

    /* 3. Compute HMAC-SHA256 over Header || Ciphertext (Encrypt-then-MAC) */
    uint8_t *tag_dest = ciphertext_dest + sizeof(VaultPlaintext);
    hmac_sha256(k_mac, 32, out_buf, sizeof(VaultHeader) + sizeof(VaultPlaintext), tag_dest);

    /* 4. Secure wipe master key from stack */
    crypto_secure_zero(master_key, sizeof(master_key));

    if (out_file_size) {
        *out_file_size = total_size;
    }
    return 0;
}

int vault_decrypt(const uint8_t *file_data, size_t file_len,
                  const uint8_t *pin, uint8_t pin_len,
                  VaultPlaintext *out_vault) {
    if (!file_data || !pin || !out_vault) {
        return -1;
    }

    VaultHeader hdr;
    int r = vault_parse_header(file_data, file_len, &hdr);
    if (r != 0) {
        return r; /* Format error */
    }

    if (pin_len != hdr.pin_length) {
        return -1; /* PIN length mismatch */
    }

    /* 1. Derive master key from input PIN and file salt */
    uint8_t master_key[64];
    pbkdf2_hmac_sha256(pin, pin_len, hdr.salt, VAULT_SALT_SIZE,
                       hdr.kdf_iters, master_key, sizeof(master_key));

    const uint8_t *k_enc = master_key;
    const uint8_t *k_mac = master_key + 32;

    /* 2. Verify-then-Decrypt: Verify HMAC tag */
    size_t authenticated_len = sizeof(VaultHeader) + hdr.payload_len;
    const uint8_t *file_tag = file_data + authenticated_len;

    uint8_t computed_tag[VAULT_TAG_SIZE];
    hmac_sha256(k_mac, 32, file_data, authenticated_len, computed_tag);

    if (!crypto_constant_time_eq(computed_tag, file_tag, VAULT_TAG_SIZE)) {
        crypto_secure_zero(master_key, sizeof(master_key));
        crypto_secure_zero(computed_tag, sizeof(computed_tag));
        return -1; /* Authentication failed (incorrect PIN) */
    }

    /* 3. Decrypt payload into RAM */
    const uint8_t *ciphertext = file_data + sizeof(VaultHeader);
    chacha20_xor(k_enc, hdr.nonce, 1,
                 ciphertext, (uint8_t *)out_vault, hdr.payload_len);

    /* 4. Secure wipe sensitive keys and tags */
    crypto_secure_zero(master_key, sizeof(master_key));
    crypto_secure_zero(computed_tag, sizeof(computed_tag));

    return 0;
}

const char *vault_btn_name(vault_button_code_t btn) {
    switch (btn) {
        case BTN_UP:       return "UP";
        case BTN_DOWN:     return "DOWN";
        case BTN_LEFT:     return "LEFT";
        case BTN_RIGHT:    return "RIGHT";
        case BTN_L:        return "L";
        case BTN_R:        return "R";
        case BTN_CROSS:    return "X";
        case BTN_CIRCLE:   return "O";
        case BTN_SQUARE:   return "[]";
        case BTN_TRIANGLE: return "/\\";
        default:           return "?";
    }
}
