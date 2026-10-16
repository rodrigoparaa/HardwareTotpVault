#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include <stddef.h>
#include "vault.h"
#include "input_logic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCREEN_LOCKED = 0,
    SCREEN_TOTP_DOCK,
    SCREEN_VAULT_LIST,
    SCREEN_VAULT_DETAIL,
    SCREEN_SETTINGS
} app_screen_t;

typedef struct {
    app_screen_t        screen;
    VaultPlaintext      vault;
    VaultHeader         file_header;
    PinInputState       pin_input;
    DockGestureState    dock_gesture;

    uint8_t             current_account_idx;
    uint8_t             current_secret_idx;
    int                 reveal_secret;
    int                 preview_next_window;
    uint8_t             theme_idx;

    uint64_t            last_activity_ms;
    int                 is_unlocked;
    int                 is_dirty;
    int                 status_message_timer_ms;
    char                status_message[48];
} AppState;

/* Initialize main application state */
void app_init(AppState *app);

/* Lock vault, securely wipe plaintext memory in RAM, reset to SCREEN_LOCKED */
void app_lock(AppState *app);

/**
 * Attempts to unlock using the current PIN in pin_input.
 *
 * @param app Application state.
 * @param file_data Raw encrypted vault bytes from disk.
 * @param file_len Size of file_data.
 * @param now_ms Current timestamp.
 * @return 0 on success, negative on error/wrong PIN.
 */
int app_try_unlock(AppState *app, const uint8_t *file_data, size_t file_len, uint64_t now_ms);

/**
 * Main frame update logic. Handles inputs, auto-lock timeouts, and navigation.
 *
 * @param app Application state.
 * @param buttons_down Currently pressed buttons bitmask.
 * @param buttons_pressed Edge-triggered newly pressed buttons bitmask.
 * @param buttons_released Edge-triggered newly released buttons bitmask.
 * @param now_ms Current timestamp in milliseconds.
 */
void app_update(AppState *app,
                uint32_t buttons_down,
                uint32_t buttons_pressed,
                uint32_t buttons_released,
                uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
