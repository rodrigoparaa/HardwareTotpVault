#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "crypto_sha.h"
#include "crypto_hmac.h"
#include "crypto_chacha20.h"
#include "base32.h"
#include "otp.h"
#include "vault.h"
#include "input_logic.h"
#include "app_state.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
    } else { \
        printf("  [FAIL] Line %d: %s\n", __LINE__, msg); \
    } \
} while(0)

static void bytes_to_hex(const uint8_t *in, size_t len, char *out) {
    for (size_t i = 0; i < len; i++) {
        sprintf(out + (i * 2), "%02x", in[i]);
    }
    out[len * 2] = '\0';
}

/* =========================================================================
 * 1. SHA-1 Tests (RFC 3174)
 * ========================================================================= */
static void test_sha1(void) {
    printf("[*] Testing SHA-1 (RFC 3174)...\n");
    uint8_t digest[SHA1_DIGEST_SIZE];
    char hex[SHA1_DIGEST_SIZE * 2 + 1];

    /* Test 1: "abc" */
    sha1("abc", 3, digest);
    bytes_to_hex(digest, SHA1_DIGEST_SIZE, hex);
    TEST_ASSERT(strcmp(hex, "a9993e364706816aba3e25717850c26c9cd0d89d") == 0,
                "SHA-1 'abc' mismatch");

    /* Test 2: multi-block */
    const char *msg2 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    sha1(msg2, strlen(msg2), digest);
    bytes_to_hex(digest, SHA1_DIGEST_SIZE, hex);
    TEST_ASSERT(strcmp(hex, "84983e441c3bd26ebaae4aa1f95129e5e54670f1") == 0,
                "SHA-1 multi-block mismatch");
}

/* =========================================================================
 * 2. SHA-256 Tests (FIPS 180-4)
 * ========================================================================= */
static void test_sha256(void) {
    printf("[*] Testing SHA-256 (FIPS 180-4)...\n");
    uint8_t digest[SHA256_DIGEST_SIZE];
    char hex[SHA256_DIGEST_SIZE * 2 + 1];

    /* Test 1: "abc" */
    sha256("abc", 3, digest);
    bytes_to_hex(digest, SHA256_DIGEST_SIZE, hex);
    TEST_ASSERT(strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0,
                "SHA-256 'abc' mismatch");

    /* Test 2: multi-block */
    const char *msg2 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    sha256(msg2, strlen(msg2), digest);
    bytes_to_hex(digest, SHA256_DIGEST_SIZE, hex);
    TEST_ASSERT(strcmp(hex, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1") == 0,
                "SHA-256 multi-block mismatch");
}

/* =========================================================================
 * 3. HMAC Tests (RFC 2202 & RFC 4231)
 * ========================================================================= */
static void test_hmac(void) {
    printf("[*] Testing HMAC-SHA1 & HMAC-SHA256 (RFC 2202 / 4231)...\n");
    uint8_t mac1[SHA1_DIGEST_SIZE];
    uint8_t mac256[SHA256_DIGEST_SIZE];
    char hex[SHA256_DIGEST_SIZE * 2 + 1];

    /* HMAC-SHA1: Key "Jefe", Data "what do ya want for nothing?" */
    hmac_sha1("Jefe", 4, "what do ya want for nothing?", 28, mac1);
    bytes_to_hex(mac1, SHA1_DIGEST_SIZE, hex);
    TEST_ASSERT(strcmp(hex, "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79") == 0,
                "HMAC-SHA1 'Jefe' mismatch");

    /* HMAC-SHA256: Key "Jefe", Data "what do ya want for nothing?" */
    hmac_sha256("Jefe", 4, "what do ya want for nothing?", 28, mac256);
    bytes_to_hex(mac256, SHA256_DIGEST_SIZE, hex);
    TEST_ASSERT(strcmp(hex, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843") == 0,
                "HMAC-SHA256 'Jefe' mismatch");

    /* Constant time comparison check */
    TEST_ASSERT(crypto_constant_time_eq(mac256, mac256, SHA256_DIGEST_SIZE) == 1,
                "crypto_constant_time_eq self-compare should be 1");
    mac256[0] ^= 0x01;
    TEST_ASSERT(crypto_constant_time_eq(mac256, mac1, 20) == 0,
                "crypto_constant_time_eq diff should be 0");
}

/* =========================================================================
 * 4. PBKDF2-HMAC-SHA256 Tests (RFC 6070)
 * ========================================================================= */
static void test_pbkdf2(void) {
    printf("[*] Testing PBKDF2-HMAC-SHA256 (RFC 6070)...\n");
    uint8_t out[32];
    char hex[65];

    /* Test 1: c=1 */
    pbkdf2_hmac_sha256("password", 8, "salt", 4, 1, out, 32);
    bytes_to_hex(out, 32, hex);
    TEST_ASSERT(strcmp(hex, "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b") == 0,
                "PBKDF2 c=1 mismatch");

    /* Test 2: c=2 */
    pbkdf2_hmac_sha256("password", 8, "salt", 4, 2, out, 32);
    bytes_to_hex(out, 32, hex);
    TEST_ASSERT(strcmp(hex, "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43") == 0,
                "PBKDF2 c=2 mismatch");

    /* Test 3: c=4096 */
    pbkdf2_hmac_sha256("password", 8, "salt", 4, 4096, out, 32);
    bytes_to_hex(out, 32, hex);
    TEST_ASSERT(strcmp(hex, "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a") == 0,
                "PBKDF2 c=4096 mismatch");
}

/* =========================================================================
 * 5. ChaCha20 Tests (RFC 8439 Sec 2.4.2)
 * ========================================================================= */
static void test_chacha20(void) {
    printf("[*] Testing ChaCha20 (RFC 8439 Sec 2.4.2)...\n");
    uint8_t key[32];
    for (int i = 0; i < 32; i++) key[i] = (uint8_t)i;

    uint8_t nonce[12] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00
    };
    uint32_t counter = 1;

    const char *plaintext = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
    size_t pt_len = strlen(plaintext);
    uint8_t *ciphertext = (uint8_t *)malloc(pt_len);
    uint8_t *decrypted = (uint8_t *)malloc(pt_len);

    chacha20_xor(key, nonce, counter, (const uint8_t *)plaintext, ciphertext, pt_len);

    /* Verify first 32 bytes against RFC 8439 Section 2.4.2 */
    const char *expected_first_32_hex = "6e2e359a2568f98041ba0728dd0d6981e97e7aec1d4360c20a27afccfd9fae0b";
    char actual_hex[65];
    bytes_to_hex(ciphertext, 32, actual_hex);
    TEST_ASSERT(strcmp(actual_hex, expected_first_32_hex) == 0,
                "ChaCha20 RFC 8439 vector mismatch");

    /* Test roundtrip decrypt */
    chacha20_xor(key, nonce, counter, ciphertext, decrypted, pt_len);
    TEST_ASSERT(memcmp(plaintext, decrypted, pt_len) == 0,
                "ChaCha20 roundtrip decryption failed");

    free(ciphertext);
    free(decrypted);
}

/* =========================================================================
 * 6. Base32 Tests (RFC 4648)
 * ========================================================================= */
static void test_base32(void) {
    printf("[*] Testing Base32 (RFC 4648)...\n");
    struct {
        const char *plain;
        const char *encoded;
    } cases[] = {
        {"", ""},
        {"f", "MY======"},
        {"fo", "MZXQ===="},
        {"foo", "MZXW6==="},
        {"foob", "MZXW6YQ="},
        {"fooba", "MZXW6YTB"},
        {"foobar", "MZXW6YTBOI======"}
    };

    char enc_buf[64];
    uint8_t dec_buf[64];
    size_t dec_len = 0;

    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        /* Encode test */
        int r1 = base32_encode((const uint8_t *)cases[i].plain, strlen(cases[i].plain), enc_buf, sizeof(enc_buf));
        TEST_ASSERT(r1 == 0 && strcmp(enc_buf, cases[i].encoded) == 0, "Base32 encode failed");

        /* Decode test */
        int r2 = base32_decode(cases[i].encoded, dec_buf, sizeof(dec_buf), &dec_len);
        TEST_ASSERT(r2 == 0 && dec_len == strlen(cases[i].plain) &&
                    memcmp(dec_buf, cases[i].plain, dec_len) == 0, "Base32 decode failed");
    }

    /* Test stripping of spaces, hyphens, and lowercase */
    int r3 = base32_decode("mzxw-6ytb oi====", dec_buf, sizeof(dec_buf), &dec_len);
    TEST_ASSERT(r3 == 0 && dec_len == 6 && memcmp(dec_buf, "foobar", 6) == 0,
                "Base32 loose formatting decode failed");
}

/* =========================================================================
 * 7. HOTP Tests (RFC 4226)
 * ========================================================================= */
static void test_hotp(void) {
    printf("[*] Testing HOTP (RFC 4226)...\n");
    const char *secret_ascii = "12345678901234567890";
    uint32_t expected_codes[10] = {
        755224, 287082, 359152, 969429, 338314,
        254676, 287922, 162583, 399871, 520489
    };

    for (uint64_t counter = 0; counter < 10; counter++) {
        uint32_t code = hotp_generate((const uint8_t *)secret_ascii, 20, counter, 6, OTP_ALGO_SHA1);
        TEST_ASSERT(code == expected_codes[counter], "HOTP RFC 4226 counter mismatch");
    }
}

/* =========================================================================
 * 8. TOTP Tests (RFC 6238)
 * ========================================================================= */
static void test_totp(void) {
    printf("[*] Testing TOTP (RFC 6238)...\n");
    const char *sha1_secret = "12345678901234567890"; // 20 bytes
    const char *sha256_secret = "12345678901234567890123456789012"; // 32 bytes

    struct {
        uint64_t time;
        uint32_t sha1_8;
        uint32_t sha256_8;
    } rfc_cases[] = {
        { 59ULL,         94287082, 46119246 },
        { 1111111109ULL, 7081804,  68084774 },
        { 1111111111ULL, 14050471, 67062674 },
        { 1234567890ULL, 89005924, 91819424 },
        { 2000000000ULL, 69279037, 90698825 }
    };

    for (size_t i = 0; i < sizeof(rfc_cases)/sizeof(rfc_cases[0]); i++) {
        uint32_t c1 = totp_generate((const uint8_t *)sha1_secret, 20,
                                    rfc_cases[i].time, 30, 8, OTP_ALGO_SHA1);
        TEST_ASSERT(c1 == rfc_cases[i].sha1_8, "TOTP SHA-1 8-digit RFC 6238 mismatch");

        uint32_t c2 = totp_generate((const uint8_t *)sha256_secret, 32,
                                    rfc_cases[i].time, 30, 8, OTP_ALGO_SHA256);
        TEST_ASSERT(c2 == rfc_cases[i].sha256_8, "TOTP SHA-256 8-digit RFC 6238 mismatch");
    }

    /* Test formatting helper */
    char fmt[16];
    otp_format_code(287082, 6, fmt, sizeof(fmt), 1);
    TEST_ASSERT(strcmp(fmt, "287 082") == 0, "otp_format_code 6-digit with space failed");

    otp_format_code(7081804, 8, fmt, sizeof(fmt), 1);
    TEST_ASSERT(strcmp(fmt, "0708 1804") == 0, "otp_format_code 8-digit with space failed");
}

/* =========================================================================
 * 9. Vault Container Tests (vault.c)
 * ========================================================================= */
static void test_vault(void) {
    printf("[*] Testing Vault Container (vault.c)...\n");

    VaultPlaintext original;
    vault_init(&original);
    original.time_drift_sec = -5;
    original.auto_lock_minutes = 2;
    original.default_theme = 1;

    /* Add an OTP account */
    original.otp_count = 1;
    strncpy(original.otp_accounts[0].issuer, "GitHub", sizeof(original.otp_accounts[0].issuer));
    strncpy(original.otp_accounts[0].label, "developer@example.com", sizeof(original.otp_accounts[0].label));
    memcpy(original.otp_accounts[0].secret, "12345678901234567890", 20);
    original.otp_accounts[0].secret_len = 20;
    original.otp_accounts[0].type = OTP_TYPE_TOTP;
    original.otp_accounts[0].algo = OTP_ALGO_SHA1;
    original.otp_accounts[0].digits = 6;
    original.otp_accounts[0].period = 30;

    /* Add a Secret */
    original.secret_count = 1;
    original.secrets[0].category = SEC_CAT_SSH_KEY;
    strncpy(original.secrets[0].title, "SSH Ed25519", sizeof(original.secrets[0].title));
    strncpy(original.secrets[0].subtitle, "main server", sizeof(original.secrets[0].subtitle));
    strncpy(original.secrets[0].value, "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5...", sizeof(original.secrets[0].value));

    /* Buffer for encrypted file */
    size_t file_size = vault_calc_file_size();
    uint8_t *enc_file = (uint8_t *)malloc(file_size);
    assert(enc_file != NULL);

    /* Test 1: Encrypt with 4-button PIN [L, R, L, R] */
    uint8_t pin4[4] = { BTN_L, BTN_R, BTN_L, BTN_R };
    size_t written_size = 0;
    int r1 = vault_encrypt(&original, pin4, 4, 1000, NULL, NULL, enc_file, file_size, &written_size);
    TEST_ASSERT(r1 == 0 && written_size == file_size, "vault_encrypt failed");

    /* Test 2: Check Header */
    VaultHeader hdr;
    int r2 = vault_parse_header(enc_file, file_size, &hdr);
    TEST_ASSERT(r2 == 0, "vault_parse_header failed");
    TEST_ASSERT(hdr.pin_length == 4, "hdr.pin_length should be 4");
    TEST_ASSERT((hdr.flags & VAULT_FLAG_LR_ONLY) != 0, "VAULT_FLAG_LR_ONLY should be set");

    /* Test 3: Decrypt with WRONG PIN [L, R, L, L] -> MUST fail */
    VaultPlaintext decrypted;
    uint8_t wrong_pin4[4] = { BTN_L, BTN_R, BTN_L, BTN_L };
    int r3 = vault_decrypt(enc_file, file_size, wrong_pin4, 4, &decrypted);
    TEST_ASSERT(r3 != 0, "vault_decrypt with wrong PIN should fail");

    /* Test 4: Decrypt with CORRECT PIN -> MUST succeed */
    int r4 = vault_decrypt(enc_file, file_size, pin4, 4, &decrypted);
    TEST_ASSERT(r4 == 0, "vault_decrypt with correct PIN failed");
    TEST_ASSERT(decrypted.time_drift_sec == -5, "Decrypted drift mismatch");
    TEST_ASSERT(decrypted.auto_lock_minutes == 2, "Decrypted auto_lock mismatch");
    TEST_ASSERT(decrypted.otp_count == 1, "Decrypted otp_count mismatch");
    TEST_ASSERT(strcmp(decrypted.otp_accounts[0].issuer, "GitHub") == 0, "Decrypted issuer mismatch");
    TEST_ASSERT(strcmp(decrypted.secrets[0].title, "SSH Ed25519") == 0, "Decrypted secret title mismatch");

    /* Test 5: Secure Wipe */
    vault_secure_wipe(&decrypted);
    TEST_ASSERT(decrypted.otp_count == 0 && decrypted.secret_count == 0 && decrypted.time_drift_sec == 0,
                "vault_secure_wipe should zero all fields");

    /* Test 6: 2-button PIN [UP, DOWN] */
    uint8_t pin2[2] = { BTN_UP, BTN_DOWN };
    int r5 = vault_encrypt(&original, pin2, 2, 500, NULL, NULL, enc_file, file_size, &written_size);
    TEST_ASSERT(r5 == 0, "vault_encrypt 2-button PIN failed");
    r5 = vault_decrypt(enc_file, file_size, pin2, 2, &decrypted);
    TEST_ASSERT(r5 == 0, "vault_decrypt 2-button PIN failed");

    /* Test 7: 8-button PIN */
    uint8_t pin8[8] = { BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_CROSS, BTN_CIRCLE, BTN_SQUARE, BTN_TRIANGLE };
    int r6 = vault_encrypt(&original, pin8, 8, 500, NULL, NULL, enc_file, file_size, &written_size);
    TEST_ASSERT(r6 == 0, "vault_encrypt 8-button PIN failed");
    r6 = vault_decrypt(enc_file, file_size, pin8, 8, &decrypted);
    TEST_ASSERT(r6 == 0, "vault_decrypt 8-button PIN failed");

    free(enc_file);
}

/* =========================================================================
 * 10. Input Logic & Gesture Tests (input_logic.c)
 * ========================================================================= */
static void test_input_logic(void) {
    printf("[*] Testing Input Logic & Gestures (input_logic.c)...\n");

    /* Test PIN Input Processor */
    PinInputState pin;
    pin_input_init(&pin, 4);
    TEST_ASSERT(pin.count == 0 && pin.max_len == 4, "pin_input_init failed");

    /* Press UP */
    pin_input_update(&pin, BTN_MASK_UP, BTN_MASK_UP, 100);
    TEST_ASSERT(pin.count == 1 && pin.buffer[0] == BTN_UP, "pin_input press UP failed");

    /* Press DOWN */
    pin_input_update(&pin, BTN_MASK_DOWN, BTN_MASK_DOWN, 200);
    TEST_ASSERT(pin.count == 2 && pin.buffer[1] == BTN_DOWN, "pin_input press DOWN failed");

    /* Backspace with CIRCLE */
    pin_input_update(&pin, BTN_MASK_CIRCLE, BTN_MASK_CIRCLE, 300);
    TEST_ASSERT(pin.count == 1 && pin.buffer[0] == BTN_UP, "pin_input backspace failed");

    /* Complete 4 buttons */
    pin_input_update(&pin, BTN_MASK_LEFT, BTN_MASK_LEFT, 400);
    pin_input_update(&pin, BTN_MASK_RIGHT, BTN_MASK_RIGHT, 500);
    pin_input_update(&pin, BTN_MASK_CROSS, BTN_MASK_CROSS, 600);
    TEST_ASSERT(pin.count == 4 && pin.trigger_submit == 1, "pin_input 4-button auto-submit failed");

    /* Test Reset via L+R chord */
    pin_input_update(&pin, BTN_MASK_L | BTN_MASK_R, 0, 700);
    TEST_ASSERT(pin.count == 0 && pin.trigger_submit == 0, "pin_input reset via L+R failed");

    /* Test Dock Gestures */
    DockGestureState dock;
    dock_gesture_init(&dock);

    /* Short press R: Next account */
    dock_gesture_update(&dock, BTN_MASK_R, BTN_MASK_R, 0, 1000);
    dock_event_t evt1 = dock_gesture_update(&dock, 0, 0, BTN_MASK_R, 1200); /* 200ms < 600ms */
    TEST_ASSERT(evt1 == DOCK_EVT_NEXT_ACCOUNT, "Short press R should trigger DOCK_EVT_NEXT_ACCOUNT");

    /* Short press L: Prev account */
    dock_gesture_update(&dock, BTN_MASK_L, BTN_MASK_L, 0, 1500);
    dock_event_t evt2 = dock_gesture_update(&dock, 0, 0, BTN_MASK_L, 1650);
    TEST_ASSERT(evt2 == DOCK_EVT_PREV_ACCOUNT, "Short press L should trigger DOCK_EVT_PREV_ACCOUNT");

    /* Long press R: Preview next window */
    dock_gesture_update(&dock, BTN_MASK_R, BTN_MASK_R, 0, 2000);
    dock_event_t evt3 = dock_gesture_update(&dock, BTN_MASK_R, 0, 0, 3300); /* 1300ms >= 1200ms */
    TEST_ASSERT(evt3 == DOCK_EVT_PREVIEW_NEXT_START, "Long press R should trigger DOCK_EVT_PREVIEW_NEXT_START");
    dock_event_t evt3_stop = dock_gesture_update(&dock, 0, 0, BTN_MASK_R, 3500);
    TEST_ASSERT(evt3_stop == DOCK_EVT_PREVIEW_NEXT_STOP, "Release R should trigger DOCK_EVT_PREVIEW_NEXT_STOP");

    /* Chord L+R >= 1000ms: Panic Lock */
    dock_gesture_update(&dock, BTN_MASK_L | BTN_MASK_R, BTN_MASK_L | BTN_MASK_R, 0, 4000);
    dock_event_t evt4 = dock_gesture_update(&dock, BTN_MASK_L | BTN_MASK_R, 0, 0, 5100);
    TEST_ASSERT(evt4 == DOCK_EVT_PANIC_LOCK, "L+R chord >= 1000ms should trigger DOCK_EVT_PANIC_LOCK");
}

/* =========================================================================
 * 11. App State Machine Tests (app_state.c)
 * ========================================================================= */
static void test_app_state(void) {
    printf("[*] Testing App State Machine (app_state.c)...\n");

    AppState app;
    app_init(&app);
    TEST_ASSERT(app.screen == SCREEN_LOCKED, "Initial screen should be SCREEN_LOCKED");

    /* Create sample vault in memory */
    VaultPlaintext mock_vault;
    vault_init(&mock_vault);
    mock_vault.auto_lock_minutes = 5;
    mock_vault.otp_count = 2;
    strncpy(mock_vault.otp_accounts[0].issuer, "Acc0", 28);
    strncpy(mock_vault.otp_accounts[1].issuer, "Acc1", 28);
    mock_vault.secret_count = 1;
    strncpy(mock_vault.secrets[0].title, "Sec0", 36);

    size_t file_size = vault_calc_file_size();
    uint8_t *enc_buf = (uint8_t *)malloc(file_size);
    uint8_t pin[4] = { BTN_L, BTN_R, BTN_L, BTN_R };
    size_t written = 0;
    vault_encrypt(&mock_vault, pin, 4, 500, NULL, NULL, enc_buf, file_size, &written);

    /* Unlock with wrong PIN */
    app.pin_input.buffer[0] = BTN_UP;
    app.pin_input.count = 1;
    int r1 = app_try_unlock(&app, enc_buf, file_size, 1000);
    TEST_ASSERT(r1 != 0 && app.screen == SCREEN_LOCKED, "Wrong PIN should remain locked");
    TEST_ASSERT(app.pin_input.cooldown_until_ms > 1000, "Cooldown penalty should be applied");

    /* Unlock with correct PIN */
    memcpy(app.pin_input.buffer, pin, 4);
    app.pin_input.count = 4;
    int r2 = app_try_unlock(&app, enc_buf, file_size, 3000);
    TEST_ASSERT(r2 == 0 && app.screen == SCREEN_TOTP_DOCK, "Correct PIN should unlock to SCREEN_TOTP_DOCK");
    TEST_ASSERT(app.is_unlocked == 1, "is_unlocked should be 1");

    /* Switch accounts with R */
    app_update(&app, BTN_MASK_R, BTN_MASK_R, 0, 3100);
    app_update(&app, 0, 0, BTN_MASK_R, 3200);
    TEST_ASSERT(app.current_account_idx == 1, "Short R tap should advance to account 1");

    /* Open Vault list with Triangle */
    app_update(&app, BTN_MASK_TRIANGLE, BTN_MASK_TRIANGLE, 0, 3300);
    TEST_ASSERT(app.screen == SCREEN_VAULT_LIST, "Triangle should switch to SCREEN_VAULT_LIST");

    /* Open Secret Detail with Cross */
    app_update(&app, BTN_MASK_CROSS, BTN_MASK_CROSS, 0, 3400);
    TEST_ASSERT(app.screen == SCREEN_VAULT_DETAIL, "Cross should open SCREEN_VAULT_DETAIL");

    /* Hold Cross to Reveal */
    app_update(&app, BTN_MASK_CROSS, 0, 0, 3500);
    TEST_ASSERT(app.reveal_secret == 1, "Holding Cross should set reveal_secret = 1");
    app_update(&app, 0, 0, BTN_MASK_CROSS, 3600);
    TEST_ASSERT(app.reveal_secret == 0, "Releasing Cross should set reveal_secret = 0");

    /* Panic Lock via START */
    app_update(&app, BTN_MASK_START, BTN_MASK_START, 0, 3700);
    TEST_ASSERT(app.screen == SCREEN_LOCKED && app.is_unlocked == 0, "START should trigger Panic Lock");
    TEST_ASSERT(app.vault.otp_count == 0, "RAM should be wiped on panic lock");

    free(enc_buf);
}

int main(void) {
    printf("====================================================\n");
    printf("  04_HardwareTotp_Vault - Cryptographic Verification \n");
    printf("====================================================\n");

    test_sha1();
    test_sha256();
    test_hmac();
    test_pbkdf2();
    test_chacha20();
    test_base32();
    test_hotp();
    test_totp();
    test_vault();
    test_input_logic();
    test_app_state();

    printf("====================================================\n");
    printf("  Summary: %d / %d tests passed successfully.\n", g_tests_passed, g_tests_run);
    printf("====================================================\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
