#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vault.h"
#include "otp.h"
#include "base32.h"

static void print_usage(const char *prog) {
    printf("===============================================================\n");
    printf("  04_HardwareTotp_Vault CLI Tool (macOS / Host)                \n");
    printf("===============================================================\n");
    printf("Usage:\n");
    printf("  %s init-demo <output.enc>      Generate a demo encrypted vault\n", prog);
    printf("  %s inspect <vault.enc>         Inspect & test live TOTP codes\n", prog);
    printf("  %s pack <plain.txt> <out.enc>  Pack text config into encrypted vault\n", prog);
    printf("  %s unpack <vault.enc> <out.txt> Decrypt and export to text file\n", prog);
    printf("===============================================================\n");
}

static vault_button_code_t parse_btn_token(const char *tok) {
    if (strcasecmp(tok, "U") == 0 || strcasecmp(tok, "UP") == 0) return BTN_UP;
    if (strcasecmp(tok, "D") == 0 || strcasecmp(tok, "DOWN") == 0) return BTN_DOWN;
    if (strcasecmp(tok, "L_LEFT") == 0 || strcasecmp(tok, "LEFT") == 0) return BTN_LEFT;
    if (strcasecmp(tok, "R_RIGHT") == 0 || strcasecmp(tok, "RIGHT") == 0) return BTN_RIGHT;
    if (strcasecmp(tok, "L") == 0) return BTN_L;
    if (strcasecmp(tok, "R") == 0) return BTN_R;
    if (strcasecmp(tok, "X") == 0 || strcasecmp(tok, "CROSS") == 0) return BTN_CROSS;
    if (strcasecmp(tok, "O") == 0 || strcasecmp(tok, "CIRCLE") == 0) return BTN_CIRCLE;
    if (strcasecmp(tok, "SQUARE") == 0 || strcasecmp(tok, "SQ") == 0 || strcasecmp(tok, "[]") == 0) return BTN_SQUARE;
    if (strcasecmp(tok, "TRIANGLE") == 0 || strcasecmp(tok, "TRI") == 0 || strcasecmp(tok, "/\\") == 0) return BTN_TRIANGLE;
    return BTN_NONE;
}

static int parse_pin_string(const char *str, uint8_t *out_pin, uint8_t expected_len) {
    char buf[128];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    uint8_t count = 0;
    char *token = strtok(buf, ", -/\r\n");
    while (token && count < expected_len) {
        vault_button_code_t btn = parse_btn_token(token);
        if (btn == BTN_NONE) {
            printf("[!] Unknown button token: '%s'\n", token);
            return -1;
        }
        out_pin[count++] = (uint8_t)btn;
        token = strtok(NULL, ", -/\r\n");
    }

    if (count != expected_len) {
        printf("[!] PIN count mismatch: got %u, expected %u\n", count, expected_len);
        return -1;
    }
    return 0;
}

static int cmd_init_demo(const char *out_path) {
    printf("[*] Initializing demo vault: %s\n", out_path);

    VaultPlaintext vault;
    vault_init(&vault);
    vault.time_drift_sec = 0;
    vault.auto_lock_minutes = 5;
    vault.default_theme = 0; /* Cyberpunk */

    /* 1. Account: GitHub (TOTP SHA-1, 6 digits) */
    TotpAccount *acc0 = &vault.otp_accounts[vault.otp_count++];
    strncpy(acc0->issuer, "GitHub", sizeof(acc0->issuer));
    strncpy(acc0->label, "developer@example.com", sizeof(acc0->label));
    base32_decode("JBSWY3DPEHPK3PXP", acc0->secret, sizeof(acc0->secret), (size_t *)&acc0->secret_len);
    acc0->type = OTP_TYPE_TOTP;
    acc0->algo = OTP_ALGO_SHA1;
    acc0->digits = 6;
    acc0->period = 30;

    /* 2. Account: Cloudflare (TOTP SHA-256, 8 digits) */
    TotpAccount *acc1 = &vault.otp_accounts[vault.otp_count++];
    strncpy(acc1->issuer, "Cloudflare", sizeof(acc1->issuer));
    strncpy(acc1->label, "infra-admin", sizeof(acc1->label));
    base32_decode("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ", acc1->secret, sizeof(acc1->secret), (size_t *)&acc1->secret_len);
    acc1->type = OTP_TYPE_TOTP;
    acc1->algo = OTP_ALGO_SHA256;
    acc1->digits = 8;
    acc1->period = 30;

    /* 3. Account: AWS (TOTP SHA-1, 6 digits) */
    TotpAccount *acc2 = &vault.otp_accounts[vault.otp_count++];
    strncpy(acc2->issuer, "AWS", sizeof(acc2->issuer));
    strncpy(acc2->label, "root-account", sizeof(acc2->label));
    base32_decode("MZXW6YTBOI======", acc2->secret, sizeof(acc2->secret), (size_t *)&acc2->secret_len);
    acc2->type = OTP_TYPE_TOTP;
    acc2->algo = OTP_ALGO_SHA1;
    acc2->digits = 6;
    acc2->period = 30;

    /* 4. Account: ProtonMail (HOTP Counter 0) */
    TotpAccount *acc3 = &vault.otp_accounts[vault.otp_count++];
    strncpy(acc3->issuer, "ProtonMail", sizeof(acc3->issuer));
    strncpy(acc3->label, "backup-token", sizeof(acc3->label));
    base32_decode("GEZDGNBVGY3TQOJQ", acc3->secret, sizeof(acc3->secret), (size_t *)&acc3->secret_len);
    acc3->type = OTP_TYPE_HOTP;
    acc3->algo = OTP_ALGO_SHA1;
    acc3->digits = 6;
    acc3->period = 30;
    acc3->counter = 0;

    /* Add Secrets */
    VaultSecret *sec0 = &vault.secrets[vault.secret_count++];
    sec0->category = SEC_CAT_SSH_KEY;
    strncpy(sec0->title, "SSH Ed25519 Prod", sizeof(sec0->title));
    strncpy(sec0->subtitle, "SHA256:4x8k... / root@srv", sizeof(sec0->subtitle));
    strncpy(sec0->value, "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIExampleFingerprintKey1234567890", sizeof(sec0->value));

    VaultSecret *sec1 = &vault.secrets[vault.secret_count++];
    sec1->category = SEC_CAT_SEED_BIP39;
    strncpy(sec1->title, "Trezor Seed 12w", sizeof(sec1->title));
    strncpy(sec1->subtitle, "Cold Storage 01", sizeof(sec1->subtitle));
    strncpy(sec1->value, "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about", sizeof(sec1->value));

    VaultSecret *sec2 = &vault.secrets[vault.secret_count++];
    sec2->category = SEC_CAT_PASSWORD;
    strncpy(sec2->title, "Master Database", sizeof(sec2->title));
    strncpy(sec2->subtitle, "Emergency Root Pass", sizeof(sec2->subtitle));
    strncpy(sec2->value, "kP9#mX$2vL@8qZ!5wR&4tY*7uJ", sizeof(sec2->value));

    /* Default PIN: [L, R, L, R] (4 buttons - instant dock unlocking!) */
    uint8_t pin[4] = { BTN_L, BTN_R, BTN_L, BTN_R };
    size_t file_size = vault_calc_file_size();
    uint8_t *enc_buf = (uint8_t *)malloc(file_size);
    if (!enc_buf) return -1;

    size_t written = 0;
    int r = vault_encrypt(&vault, pin, 4, 10000, NULL, NULL, enc_buf, file_size, &written);
    if (r != 0) {
        printf("[!] vault_encrypt failed: %d\n", r);
        free(enc_buf);
        return -1;
    }

    FILE *f = fopen(out_path, "wb");
    if (!f) {
        perror("fopen");
        free(enc_buf);
        return -1;
    }
    fwrite(enc_buf, 1, written, f);
    fclose(f);
    free(enc_buf);

    printf("[+] Demo vault written successfully (%zu bytes)!\n", written);
    printf("    Default PIN: [L, R, L, R] (4 triggers - Dock slider closed compatible)\n");
    printf("    Included %u OTP accounts and %u Secrets.\n", vault.otp_count, vault.secret_count);
    return 0;
}

static int cmd_inspect(const char *in_path) {
    FILE *f = fopen(in_path, "rb");
    if (!f) {
        perror("fopen");
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = (uint8_t *)malloc(size);
    if (!data) { fclose(f); return -1; }
    fread(data, 1, size, f);
    fclose(f);

    VaultHeader hdr;
    if (vault_parse_header(data, size, &hdr) != 0) {
        printf("[!] Invalid vault header or unsupported file.\n");
        free(data);
        return -1;
    }

    printf("===============================================================\n");
    printf("  Vault Header Inspection: %s\n", in_path);
    printf("===============================================================\n");
    printf("  Magic:         %.4s\n", hdr.magic);
    printf("  Version:       %u\n", hdr.version);
    printf("  PIN Length:    %u buttons\n", hdr.pin_length);
    printf("  Slider Dock:   %s\n", (hdr.flags & VAULT_FLAG_LR_ONLY) ? "YES (L/R only)" : "NO (all buttons)");
    printf("  KDF Iterations:%u (PBKDF2-HMAC-SHA256)\n", hdr.kdf_iters);
    printf("  Payload Size:  %u bytes\n", hdr.payload_len);
    printf("===============================================================\n");

    printf("Enter PIN sequence (e.g. L,R,L,R or UP,DOWN,X,O): ");
    char pin_str[64];
    if (!fgets(pin_str, sizeof(pin_str), stdin)) {
        free(data);
        return -1;
    }

    uint8_t pin[8];
    if (parse_pin_string(pin_str, pin, hdr.pin_length) != 0) {
        free(data);
        return -1;
    }

    VaultPlaintext vault;
    int r = vault_decrypt(data, size, pin, hdr.pin_length, &vault);
    if (r != 0) {
        printf("[!] Decryption failed! Incorrect PIN or file corrupted.\n");
        free(data);
        return -1;
    }
    free(data);

    printf("[+] Vault Decrypted Successfully in RAM!\n");
    printf("  Time Drift:        %+d seconds\n", vault.time_drift_sec);
    printf("  Auto-Lock Timeout: %u minutes\n", vault.auto_lock_minutes);
    printf("  Default Theme:     %u\n\n", vault.default_theme);

    uint64_t now_utc = (uint64_t)time(NULL) + vault.time_drift_sec;
    printf("--- 2FA OTP ACCOUNTS (%u) [Current UTC: %llu] ---\n", vault.otp_count, now_utc);
    for (int i = 0; i < vault.otp_count; i++) {
        TotpAccount *acc = &vault.otp_accounts[i];
        uint32_t code = 0;
        uint32_t rem = 0;
        if (acc->type == OTP_TYPE_TOTP) {
            code = totp_generate(acc->secret, acc->secret_len, now_utc, acc->period, acc->digits, (otp_algo_t)acc->algo);
            rem = totp_remaining_seconds(now_utc, acc->period);
        } else {
            code = hotp_generate(acc->secret, acc->secret_len, acc->counter, acc->digits, (otp_algo_t)acc->algo);
        }

        char fmt[16];
        otp_format_code(code, acc->digits, fmt, sizeof(fmt), 1);
        printf("[%02d] %-12s (%-15s) -> CODE: %s  [%2us remaining] (%s-%s)\n",
               i + 1, acc->issuer, acc->label, fmt, rem,
               (acc->type == OTP_TYPE_TOTP) ? "TOTP" : "HOTP",
               (acc->algo == OTP_ALGO_SHA256) ? "SHA256" : "SHA1");
    }

    printf("\n--- SECRETS COFFER (%u) ---\n", vault.secret_count);
    for (int i = 0; i < vault.secret_count; i++) {
        VaultSecret *sec = &vault.secrets[i];
        printf("[%02d] [%u] %-18s (%-20s): %s\n",
               i + 1, sec->category, sec->title, sec->subtitle, sec->value);
    }
    printf("===============================================================\n");

    vault_secure_wipe(&vault);
    return 0;
}

static char *trim_whitespace(char *str) {
    if (!str) return str;
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    if (*str == '\0') return str;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return str;
}

static void parse_otpauth_uri(const char *uri, TotpAccount *acc) {
    /* Format: otpauth://totp/Label?secret=... */
    const char *q = strchr(uri, '?');
    if (!q) return;

    /* Check type */
    if (strstr(uri, "otpauth://hotp/")) {
        acc->type = OTP_TYPE_HOTP;
    } else {
        acc->type = OTP_TYPE_TOTP;
    }

    char query[256];
    strncpy(query, q + 1, sizeof(query) - 1);
    query[sizeof(query) - 1] = '\0';

    char *pair = strtok(query, "&");
    while (pair) {
        char *eq = strchr(pair, '=');
        if (eq) {
            *eq = '\0';
            const char *key = pair;
            const char *val = eq + 1;
            if (strcasecmp(key, "secret") == 0) {
                base32_decode(val, acc->secret, sizeof(acc->secret), (size_t *)&acc->secret_len);
            } else if (strcasecmp(key, "issuer") == 0) {
                strncpy(acc->issuer, val, sizeof(acc->issuer) - 1);
            } else if (strcasecmp(key, "algorithm") == 0) {
                if (strcasecmp(val, "SHA256") == 0) acc->algo = OTP_ALGO_SHA256;
                else acc->algo = OTP_ALGO_SHA1;
            } else if (strcasecmp(key, "digits") == 0) {
                acc->digits = (uint8_t)atoi(val);
            } else if (strcasecmp(key, "period") == 0) {
                acc->period = (uint32_t)atoi(val);
            } else if (strcasecmp(key, "counter") == 0) {
                acc->counter = (uint64_t)strtoull(val, NULL, 10);
            }
        }
        pair = strtok(NULL, "&");
    }
}

static int cmd_pack(const char *in_path, const char *out_path) {
    printf("[*] Packing plain config '%s' into '%s'...\n", in_path, out_path);

    FILE *f = fopen(in_path, "r");
    if (!f) {
        perror("fopen");
        return -1;
    }

    VaultPlaintext vault;
    vault_init(&vault);

    uint8_t cfg_pin_len = 4;
    char cfg_pin_str[64] = "L, R, L, R";

    int current_section = 0; /* 1: config, 2: otp, 3: secret */
    TotpAccount *cur_acc = NULL;
    VaultSecret *cur_sec = NULL;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == ';' || *p == '\0' || *p == '\r' || *p == '\n') {
            continue;
        }

        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end) {
                *end = '\0';
                char *sec_name = trim_whitespace(p + 1);
                if (strcasecmp(sec_name, "config") == 0) {
                    current_section = 1;
                } else if (strcasecmp(sec_name, "otp") == 0) {
                    current_section = 2;
                    if (vault.otp_count < VAULT_MAX_ACCOUNTS) {
                        cur_acc = &vault.otp_accounts[vault.otp_count++];
                        cur_acc->type = OTP_TYPE_TOTP;
                        cur_acc->algo = OTP_ALGO_SHA1;
                        cur_acc->digits = 6;
                        cur_acc->period = 30;
                    }
                } else if (strcasecmp(sec_name, "secret") == 0) {
                    current_section = 3;
                    if (vault.secret_count < VAULT_MAX_SECRETS) {
                        cur_sec = &vault.secrets[vault.secret_count++];
                        cur_sec->category = SEC_CAT_PASSWORD;
                    }
                }
            }
            continue;
        }

        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim_whitespace(p);
        char *val = trim_whitespace(eq + 1);

        if (current_section == 1) {
            /* [config] */
            if (strcasecmp(key, "time_drift_sec") == 0) {
                vault.time_drift_sec = (int16_t)atoi(val);
            } else if (strcasecmp(key, "auto_lock_minutes") == 0) {
                vault.auto_lock_minutes = (uint8_t)atoi(val);
            } else if (strcasecmp(key, "default_theme") == 0) {
                vault.default_theme = (uint8_t)atoi(val);
            } else if (strcasecmp(key, "pin_length") == 0) {
                cfg_pin_len = (uint8_t)atoi(val);
            } else if (strcasecmp(key, "pin_sequence") == 0) {
                strncpy(cfg_pin_str, val, sizeof(cfg_pin_str) - 1);
            }
        } else if (current_section == 2 && cur_acc) {
            /* [otp] */
            if (strcasecmp(key, "issuer") == 0) {
                strncpy(cur_acc->issuer, val, sizeof(cur_acc->issuer) - 1);
            } else if (strcasecmp(key, "label") == 0) {
                strncpy(cur_acc->label, val, sizeof(cur_acc->label) - 1);
            } else if (strcasecmp(key, "secret") == 0) {
                base32_decode(val, cur_acc->secret, sizeof(cur_acc->secret), (size_t *)&cur_acc->secret_len);
            } else if (strcasecmp(key, "uri") == 0) {
                parse_otpauth_uri(val, cur_acc);
            } else if (strcasecmp(key, "type") == 0) {
                if (strcasecmp(val, "HOTP") == 0) cur_acc->type = OTP_TYPE_HOTP;
                else cur_acc->type = OTP_TYPE_TOTP;
            } else if (strcasecmp(key, "algo") == 0) {
                if (strcasecmp(val, "SHA256") == 0) cur_acc->algo = OTP_ALGO_SHA256;
                else cur_acc->algo = OTP_ALGO_SHA1;
            } else if (strcasecmp(key, "digits") == 0) {
                cur_acc->digits = (uint8_t)atoi(val);
            } else if (strcasecmp(key, "period") == 0) {
                cur_acc->period = (uint32_t)atoi(val);
            } else if (strcasecmp(key, "counter") == 0) {
                cur_acc->counter = (uint64_t)strtoull(val, NULL, 10);
            }
        } else if (current_section == 3 && cur_sec) {
            /* [secret] */
            if (strcasecmp(key, "category") == 0) {
                if (strcasecmp(val, "SSH") == 0) cur_sec->category = SEC_CAT_SSH_KEY;
                else if (strcasecmp(val, "GPG") == 0) cur_sec->category = SEC_CAT_GPG;
                else if (strcasecmp(val, "SEED") == 0) cur_sec->category = SEC_CAT_SEED_BIP39;
                else if (strcasecmp(val, "RECOVERY") == 0) cur_sec->category = SEC_CAT_RECOVERY;
                else cur_sec->category = SEC_CAT_PASSWORD;
            } else if (strcasecmp(key, "title") == 0) {
                strncpy(cur_sec->title, val, sizeof(cur_sec->title) - 1);
            } else if (strcasecmp(key, "subtitle") == 0) {
                strncpy(cur_sec->subtitle, val, sizeof(cur_sec->subtitle) - 1);
            } else if (strcasecmp(key, "value") == 0) {
                strncpy(cur_sec->value, val, sizeof(cur_sec->value) - 1);
            }
        }
    }
    fclose(f);

    if (cfg_pin_len != 2 && cfg_pin_len != 4 && cfg_pin_len != 8) {
        printf("[!] Invalid pin_length in config (%u). Defaulting to 4.\n", cfg_pin_len);
        cfg_pin_len = 4;
    }

    uint8_t pin[8];
    if (parse_pin_string(cfg_pin_str, pin, cfg_pin_len) != 0) {
        printf("[!] Error in pin_sequence: '%s'\n", cfg_pin_str);
        return -1;
    }

    size_t file_size = vault_calc_file_size();
    uint8_t *enc_buf = (uint8_t *)malloc(file_size);
    if (!enc_buf) return -1;

    size_t written = 0;
    int r = vault_encrypt(&vault, pin, cfg_pin_len, 10000, NULL, NULL, enc_buf, file_size, &written);
    if (r != 0) {
        printf("[!] vault_encrypt failed: %d\n", r);
        free(enc_buf);
        return -1;
    }

    FILE *out_f = fopen(out_path, "wb");
    if (!out_f) {
        perror("fopen out_path");
        free(enc_buf);
        return -1;
    }
    fwrite(enc_buf, 1, written, out_f);
    fclose(out_f);
    free(enc_buf);

    printf("[+] Successfully packed and encrypted %s (%zu bytes)!\n", out_path, written);
    printf("    PIN configured: %s (%u buttons)\n", cfg_pin_str, cfg_pin_len);
    printf("    Packed %u OTP accounts and %u Secrets.\n", vault.otp_count, vault.secret_count);
    return 0;
}

static int cmd_unpack(const char *in_path, const char *out_path) {
    FILE *f = fopen(in_path, "rb");
    if (!f) {
        perror("fopen");
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = (uint8_t *)malloc(size);
    if (!data) { fclose(f); return -1; }
    fread(data, 1, size, f);
    fclose(f);

    VaultHeader hdr;
    if (vault_parse_header(data, size, &hdr) != 0) {
        printf("[!] Invalid vault header.\n");
        free(data);
        return -1;
    }

    printf("Enter PIN sequence (%u buttons, e.g. L,R,L,R): ", hdr.pin_length);
    char pin_str[64];
    if (!fgets(pin_str, sizeof(pin_str), stdin)) {
        free(data);
        return -1;
    }

    uint8_t pin[8];
    if (parse_pin_string(pin_str, pin, hdr.pin_length) != 0) {
        free(data);
        return -1;
    }

    VaultPlaintext vault;
    int r = vault_decrypt(data, size, pin, hdr.pin_length, &vault);
    free(data);

    if (r != 0) {
        printf("[!] Decryption failed! Incorrect PIN.\n");
        return -1;
    }

    FILE *out = fopen(out_path, "w");
    if (!out) {
        perror("fopen out_path");
        vault_secure_wipe(&vault);
        return -1;
    }

    fprintf(out, "# Exported from %s\n\n", in_path);
    fprintf(out, "[config]\ntime_drift_sec = %d\nauto_lock_minutes = %u\ndefault_theme = %u\npin_length = %u\n\n",
            vault.time_drift_sec, vault.auto_lock_minutes, vault.default_theme, hdr.pin_length);

    for (int i = 0; i < vault.otp_count; i++) {
        TotpAccount *acc = &vault.otp_accounts[i];
        char b32_buf[128];
        base32_encode(acc->secret, acc->secret_len, b32_buf, sizeof(b32_buf));
        fprintf(out, "[otp]\nissuer = %s\nlabel = %s\nsecret = %s\ntype = %s\nalgo = %s\ndigits = %u\nperiod = %u\n",
                acc->issuer, acc->label, b32_buf,
                (acc->type == OTP_TYPE_TOTP) ? "TOTP" : "HOTP",
                (acc->algo == OTP_ALGO_SHA256) ? "SHA256" : "SHA1",
                acc->digits, acc->period);
        if (acc->type == OTP_TYPE_HOTP) {
            fprintf(out, "counter = %llu\n", (unsigned long long)acc->counter);
        }
        fprintf(out, "\n");
    }

    for (int i = 0; i < vault.secret_count; i++) {
        VaultSecret *sec = &vault.secrets[i];
        const char *cat_name = "PASSWORD";
        switch (sec->category) {
            case SEC_CAT_SSH_KEY:    cat_name = "SSH"; break;
            case SEC_CAT_GPG:        cat_name = "GPG"; break;
            case SEC_CAT_SEED_BIP39: cat_name = "SEED"; break;
            case SEC_CAT_RECOVERY:   cat_name = "RECOVERY"; break;
            default:                 cat_name = "PASSWORD"; break;
        }
        fprintf(out, "[secret]\ncategory = %s\ntitle = %s\nsubtitle = %s\nvalue = %s\n\n",
                cat_name, sec->title, sec->subtitle, sec->value);
    }

    fclose(out);
    vault_secure_wipe(&vault);
    printf("[+] Successfully unpacked decrypted data to '%s'!\n", out_path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "init-demo") == 0) {
        if (argc < 3) {
            printf("Error: Missing output path.\n");
            return 1;
        }
        return cmd_init_demo(argv[2]);
    } else if (strcmp(argv[1], "inspect") == 0) {
        if (argc < 3) {
            printf("Error: Missing vault file path.\n");
            return 1;
        }
        return cmd_inspect(argv[2]);
    } else if (strcmp(argv[1], "pack") == 0) {
        if (argc < 4) {
            printf("Error: Usage: %s pack <plain.txt> <out_vault.enc>\n", argv[0]);
            return 1;
        }
        return cmd_pack(argv[2], argv[3]);
    } else if (strcmp(argv[1], "unpack") == 0) {
        if (argc < 4) {
            printf("Error: Usage: %s unpack <in_vault.enc> <out_plain.txt>\n", argv[0]);
            return 1;
        }
        return cmd_unpack(argv[2], argv[3]);
    } else {
        print_usage(argv[0]);
        return 1;
    }
}

