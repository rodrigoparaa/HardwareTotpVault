#include "psp_fs.h"
#include <stdio.h>
#include <string.h>

static char g_active_vault_path[128] = "vault.enc";

static const char *CANDIDATE_PATHS[] = {
    "ef0:/PSP/GAME/TotpVault/vault.enc",
    "ms0:/PSP/GAME/TotpVault/vault.enc",
    "vault.enc"
};

int psp_fs_load_vault(uint8_t *buffer, size_t max_size, size_t *out_size) {
    if (!buffer || max_size == 0) return -1;

    for (size_t i = 0; i < sizeof(CANDIDATE_PATHS)/sizeof(CANDIDATE_PATHS[0]); i++) {
        FILE *f = fopen(CANDIDATE_PATHS[i], "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long file_len = ftell(f);
            fseek(f, 0, SEEK_SET);

            if (file_len > 0 && (size_t)file_len <= max_size) {
                size_t read_bytes = fread(buffer, 1, file_len, f);
                fclose(f);
                if (read_bytes == (size_t)file_len) {
                    strncpy(g_active_vault_path, CANDIDATE_PATHS[i], sizeof(g_active_vault_path) - 1);
                    if (out_size) *out_size = read_bytes;
                    return 0;
                }
            } else {
                fclose(f);
            }
        }
    }

    return -1; /* File not found */
}

int psp_fs_save_vault(const uint8_t *data, size_t size) {
    if (!data || size == 0) return -1;

    char tmp_path[140];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", g_active_vault_path);

    /* 1. Write to temporary file */
    FILE *f = fopen(tmp_path, "wb");
    if (!f) return -2;

    size_t written = fwrite(data, 1, size, f);
    fflush(f);
    fclose(f);

    if (written != size) {
        remove(tmp_path);
        return -3;
    }

    /* 2. Atomic replacement: remove old, rename tmp */
    remove(g_active_vault_path);
    if (rename(tmp_path, g_active_vault_path) != 0) {
        return -4;
    }

    return 0;
}

const char *psp_fs_get_vault_path(void) {
    return g_active_vault_path;
}
