#ifndef PSP_FS_H
#define PSP_FS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Searches for and reads vault.enc into buffer.
 * Priority:
 *   1. ef0:/PSP/GAME/TotpVault/vault.enc
 *   2. ms0:/PSP/GAME/TotpVault/vault.enc
 *   3. ./vault.enc
 *
 * @param buffer Output buffer for file contents.
 * @param max_size Capacity of buffer.
 * @param out_size Pointer to receive the read size.
 * @return 0 on success, negative on error / file not found.
 */
int psp_fs_load_vault(uint8_t *buffer, size_t max_size, size_t *out_size);

/**
 * Atomically writes vault data to vault.enc (via temporary file vault.tmp).
 *
 * @param data Buffer of encrypted vault data.
 * @param size Exact size to write.
 * @return 0 on success, negative on error.
 */
int psp_fs_save_vault(const uint8_t *data, size_t size);

/**
 * Returns the path of the last opened or target vault file.
 */
const char *psp_fs_get_vault_path(void);

#ifdef __cplusplus
}
#endif

#endif /* PSP_FS_H */
