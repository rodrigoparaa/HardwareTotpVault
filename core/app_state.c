#include "app_state.h"
#include <string.h>

void app_init(AppState *app) {
    if (!app) return;
    memset(app, 0, sizeof(AppState));
    vault_init(&app->vault);
    pin_input_init(&app->pin_input, 4);
    dock_gesture_init(&app->dock_gesture);
    app->screen = SCREEN_LOCKED;
    app->theme_idx = 0;
    app->is_unlocked = 0;
}

void app_lock(AppState *app) {
    if (!app) return;
    vault_secure_wipe(&app->vault);
    pin_input_reset(&app->pin_input);
    dock_gesture_init(&app->dock_gesture);
    app->screen = SCREEN_LOCKED;
    app->is_unlocked = 0;
    app->reveal_secret = 0;
    app->preview_next_window = 0;
}

int app_try_unlock(AppState *app, const uint8_t *file_data, size_t file_len, uint64_t now_ms) {
    if (!app || !file_data) return -1;

    int r = vault_decrypt(file_data, file_len,
                          app->pin_input.buffer, app->pin_input.count,
                          &app->vault);
    if (r == 0) {
        app->is_unlocked = 1;
        app->screen = SCREEN_TOTP_DOCK;
        app->theme_idx = app->vault.default_theme;
        app->pin_input.failed_attempts = 0;
        pin_input_reset(&app->pin_input);
        app->last_activity_ms = now_ms;
        return 0;
    } else {
        app->pin_input.failed_attempts++;
        uint64_t penalty = (app->pin_input.failed_attempts >= 4) ? 5000 : 1500;
        app->pin_input.cooldown_until_ms = now_ms + penalty;
        pin_input_reset(&app->pin_input);
        return -1;
    }
}

void app_update(AppState *app,
                uint32_t buttons_down,
                uint32_t buttons_pressed,
                uint32_t buttons_released,
                uint64_t now_ms) {
    if (!app) return;

    /* Update activity timestamp if any button state changed */
    if (buttons_pressed || buttons_released) {
        app->last_activity_ms = now_ms;
    }

    /* 1. Auto-Lock Watchdog */
    if (app->is_unlocked && app->vault.auto_lock_minutes > 0) {
        uint64_t timeout_ms = (uint64_t)app->vault.auto_lock_minutes * 60ULL * 1000ULL;
        if (now_ms - app->last_activity_ms >= timeout_ms) {
            app_lock(app);
            return;
        }
    }

    /* 2. Global Panic Lock Shortcut (L + R held in any screen, or START) */
    if (app->is_unlocked) {
        if (buttons_pressed & BTN_MASK_START) {
            app_lock(app);
            return;
        }
    }

    /* 3. Screen-Specific State Machine */
    switch (app->screen) {
        case SCREEN_LOCKED: {
            pin_input_update(&app->pin_input, buttons_down, buttons_pressed, now_ms);
            break;
        }

        case SCREEN_TOTP_DOCK: {
            dock_event_t evt = dock_gesture_update(&app->dock_gesture,
                                                   buttons_down, buttons_pressed, buttons_released,
                                                   now_ms);
            switch (evt) {
                case DOCK_EVT_NEXT_ACCOUNT:
                    if (app->vault.otp_count > 0) {
                        app->current_account_idx = (app->current_account_idx + 1) % app->vault.otp_count;
                    }
                    break;
                case DOCK_EVT_PREV_ACCOUNT:
                    if (app->vault.otp_count > 0) {
                        app->current_account_idx = (app->current_account_idx - 1 + app->vault.otp_count) % app->vault.otp_count;
                    }
                    break;
                case DOCK_EVT_PREVIEW_NEXT_START:
                    app->preview_next_window = 1;
                    break;
                case DOCK_EVT_PREVIEW_NEXT_STOP:
                    app->preview_next_window = 0;
                    break;
                case DOCK_EVT_CYCLE_THEME:
                    app->theme_idx = (app->theme_idx + 1) % 3;
                    break;
                case DOCK_EVT_PANIC_LOCK:
                    app_lock(app);
                    break;
                case DOCK_EVT_OPEN_VAULT:
                    app->screen = SCREEN_VAULT_LIST;
                    break;
                case DOCK_EVT_OPEN_SETTINGS:
                    app->screen = SCREEN_SETTINGS;
                    break;
                default:
                    break;
            }
            break;
        }

        case SCREEN_VAULT_LIST: {
            if (buttons_pressed & (BTN_MASK_DOWN | BTN_MASK_RIGHT)) {
                if (app->vault.secret_count > 0) {
                    app->current_secret_idx = (app->current_secret_idx + 1) % app->vault.secret_count;
                }
            } else if (buttons_pressed & (BTN_MASK_UP | BTN_MASK_LEFT)) {
                if (app->vault.secret_count > 0) {
                    app->current_secret_idx = (app->current_secret_idx - 1 + app->vault.secret_count) % app->vault.secret_count;
                }
            } else if (buttons_pressed & BTN_MASK_CROSS) {
                if (app->vault.secret_count > 0) {
                    app->screen = SCREEN_VAULT_DETAIL;
                }
            } else if (buttons_pressed & (BTN_MASK_CIRCLE | BTN_MASK_TRIANGLE)) {
                app->screen = SCREEN_TOTP_DOCK;
            }
            break;
        }

        case SCREEN_VAULT_DETAIL: {
            /* Hold-to-Reveal: secret is exposed in clear only while holding Cross or R */
            app->reveal_secret = (buttons_down & (BTN_MASK_CROSS | BTN_MASK_R)) ? 1 : 0;

            if (buttons_pressed & BTN_MASK_CIRCLE) {
                app->reveal_secret = 0;
                app->screen = SCREEN_VAULT_LIST;
            }
            break;
        }

        case SCREEN_SETTINGS: {
            if (buttons_pressed & BTN_MASK_CIRCLE) {
                app->screen = SCREEN_TOTP_DOCK;
            }
            break;
        }
    }
}
