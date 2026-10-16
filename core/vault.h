#ifndef VAULT_H
#define VAULT_H

#include <stdint.h>
#include <stddef.h>
#include "otp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VAULT_MAGIC       "PVLT"
#define VAULT_VERSION     1
#define VAULT_SALT_SIZE   16
#define VAULT_NONCE_SIZE  12
#define VAULT_TAG_SIZE    32
#define VAULT_MAX_ACCOUNTS 32
#define VAULT_MAX_SECRETS  32

#define VAULT_FLAG_LR_ONLY (1 << 0)

/* Physical button codes for PIN sequences */
typedef enum {
    BTN_NONE     = 0x00,
    BTN_UP       = 0x01,
    BTN_DOWN     = 0x02,
    BTN_LEFT     = 0x03,
    BTN_RIGHT    = 0x04,
    BTN_L        = 0x05,
    BTN_R        = 0x06,
    BTN_CROSS    = 0x07,
    BTN_CIRCLE   = 0x08,
    BTN_SQUARE   = 0x09,
    BTN_TRIANGLE = 0x0A
} vault_button_code_t;

/* Secret categories for critical vault entries */
typedef enum {
    SEC_CAT_PASSWORD   = 0,
    SEC_CAT_SSH_KEY    = 1,
    SEC_CAT_GPG        = 2,
    SEC_CAT_SEED_BIP39 = 3,
    SEC_CAT_RECOVERY   = 4
} secret_category_t;

/* --- 2FA OTP Account Definition --- */
typedef struct {
    char        issuer[28];
    char        label[32];
    uint8_t     secret[64];
    uint8_t     secret_len;
    uint8_t     type;       /* OTP_TYPE_TOTP or OTP_TYPE_HOTP */
    uint8_t     algo;       /* OTP_ALGO_SHA1 or OTP_ALGO_SHA256 */
    uint8_t     digits;     /* 6 or 8 */
    uint32_t    period;     /* Typically 30s */
    uint64_t    counter;    /* For HOTP */
} TotpAccount;

/* --- Critical Vault Secret Definition --- */
typedef struct {
    uint8_t     category;   /* secret_category_t */
    char        title[36];
    char        subtitle[40];
    char        value[256];
} VaultSecret;

/* --- Plaintext Vault in RAM (Aligned for DMA/eDRAM) --- */
typedef struct __attribute__((aligned(16))) {
    int16_t     time_drift_sec;     /* Manual RTC drift offset in seconds */
    uint8_t     auto_lock_minutes;  /* 0=Disabled, 2, 5, 10 minutes */
    uint8_t     default_theme;      /* 0=Cyberpunk, 1=Stealth Red, 2=OLED Gold */
    uint8_t     reserved[12];

    uint8_t     otp_count;
    TotpAccount otp_accounts[VAULT_MAX_ACCOUNTS];

    uint8_t     secret_count;
    VaultSecret secrets[VAULT_MAX_SECRETS];
} VaultPlaintext;

/* --- Binary File Header --- */
typedef struct __attribute__((packed)) {
    char        magic[4];       /* "PVLT" */
    uint8_t     version;        /* 1 */
    uint8_t     pin_length;     /* 2, 4, or 8 */
    uint16_t    flags;          /* VAULT_FLAG_* */
    uint32_t    kdf_iters;      /* Default 10000 */
    uint8_t     salt[VAULT_SALT_SIZE];
    uint8_t     nonce[VAULT_NONCE_SIZE];
    uint32_t    payload_len;    /* sizeof(VaultPlaintext) */
} VaultHeader;

/* Initialize an empty vault */
void vault_init(VaultPlaintext *vault);

/* Zero out all memory occupied by the decrypted vault */
void vault_secure_wipe(VaultPlaintext *vault);

/* Parse and validate the unencrypted header from raw file bytes */
int vault_parse_header(const uint8_t *file_data, size_t file_len, VaultHeader *out_hdr);

/**
 * Encrypts a VaultPlaintext structure into a binary vault buffer.
 *
 * @param vault Pointer to plaintext vault in RAM.
 * @param pin Array of pin_len button codes.
 * @param pin_len 2, 4, or 8.
 * @param kdf_iters PBKDF2 iterations (default 10000).
 * @param salt Optional 16-byte salt (if NULL, generated from random/entropy).
 * @param nonce Optional 12-byte nonce (if NULL, generated from random/entropy).
 * @param out_buf Output buffer for encrypted file (must be at least vault_calc_file_size()).
 * @param out_buf_max Capacity of output buffer.
 * @param out_file_size Pointer to receive the exact file size.
 * @return 0 on success, negative on error.
 */
int vault_encrypt(const VaultPlaintext *vault,
                  const uint8_t *pin, uint8_t pin_len,
                  uint32_t kdf_iters,
                  const uint8_t *salt, const uint8_t *nonce,
                  uint8_t *out_buf, size_t out_buf_max, size_t *out_file_size);

/**
 * Verifies HMAC tag and decrypts a binary vault into RAM.
 *
 * @param file_data Raw encrypted file bytes.
 * @param file_len Total size of file_data.
 * @param pin Input PIN sequence.
 * @param pin_len Length of input PIN.
 * @param out_vault Pointer to VaultPlaintext in RAM to receive decrypted data.
 * @return 0 on success, -1 on bad PIN/HMAC failure, -2 on format/corrupted data.
 */
int vault_decrypt(const uint8_t *file_data, size_t file_len,
                  const uint8_t *pin, uint8_t pin_len,
                  VaultPlaintext *out_vault);

/* Computes total serialized file size */
size_t vault_calc_file_size(void);

/* Helper to convert button code to short string */
const char *vault_btn_name(vault_button_code_t btn);

#ifdef __cplusplus
}
#endif

#endif /* VAULT_H */
