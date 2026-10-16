#include "input_logic.h"
#include <string.h>

void pin_input_init(PinInputState *state, uint8_t max_len) {
    if (!state) return;
    memset(state, 0, sizeof(PinInputState));
    state->max_len = (max_len == 2 || max_len == 4 || max_len == 8) ? max_len : 4;
}

void pin_input_reset(PinInputState *state) {
    if (!state) return;
    state->count = 0;
    state->trigger_submit = 0;
    memset(state->buffer, 0, sizeof(state->buffer));
}

vault_button_code_t button_mask_to_code(uint32_t mask) {
    if (mask & BTN_MASK_UP)       return BTN_UP;
    if (mask & BTN_MASK_DOWN)     return BTN_DOWN;
    if (mask & BTN_MASK_LEFT)     return BTN_LEFT;
    if (mask & BTN_MASK_RIGHT)    return BTN_RIGHT;
    if (mask & BTN_MASK_L)        return BTN_L;
    if (mask & BTN_MASK_R)        return BTN_R;
    if (mask & BTN_MASK_CROSS)    return BTN_CROSS;
    if (mask & BTN_MASK_CIRCLE)   return BTN_CIRCLE;
    if (mask & BTN_MASK_SQUARE)   return BTN_SQUARE;
    if (mask & BTN_MASK_TRIANGLE) return BTN_TRIANGLE;
    return BTN_NONE;
}

int pin_input_update(PinInputState *state,
                     uint32_t buttons_down,
                     uint32_t buttons_pressed,
                     uint64_t now_ms) {
    if (!state) return 0;

    /* Respect active anti-tamper penalty cooldown */
    if (now_ms < state->cooldown_until_ms) {
        return 0;
    }

    /* Reset sequence shortcut (L + R chord in dock or SELECT) */
    if (((buttons_down & (BTN_MASK_L | BTN_MASK_R)) == (BTN_MASK_L | BTN_MASK_R)) ||
        (buttons_pressed & BTN_MASK_SELECT)) {
        pin_input_reset(state);
        return 1;
    }

    /* Backspace (Circle button) */
    if (buttons_pressed & BTN_MASK_CIRCLE) {
        if (state->count > 0) {
            state->count--;
            state->buffer[state->count] = 0;
            return 1;
        }
        return 0;
    }

    /* Process single button presses */
    if (buttons_pressed) {
        vault_button_code_t code = button_mask_to_code(buttons_pressed);
        if (code != BTN_NONE && state->count < state->max_len) {
            state->buffer[state->count++] = (uint8_t)code;
            if (state->count == state->max_len) {
                state->trigger_submit = 1;
            }
            return 1;
        }
    }

    return 0;
}

void dock_gesture_init(DockGestureState *state) {
    if (!state) return;
    memset(state, 0, sizeof(DockGestureState));
}

dock_event_t dock_gesture_update(DockGestureState *state,
                                 uint32_t buttons_down,
                                 uint32_t buttons_pressed,
                                 uint32_t buttons_released,
                                 uint64_t now_ms) {
    if (!state) return DOCK_EVT_NONE;

    /* 1. Instant Panic Lock from START or L+R chord */
    if (buttons_pressed & BTN_MASK_START) {
        return DOCK_EVT_PANIC_LOCK;
    }

    int both_triggers_down = ((buttons_down & (BTN_MASK_L | BTN_MASK_R)) == (BTN_MASK_L | BTN_MASK_R));

    if (both_triggers_down) {
        if (state->chord_press_start_ms == 0) {
            state->chord_press_start_ms = now_ms;
        } else if ((now_ms - state->chord_press_start_ms) >= 1000 && !state->chord_fired) {
            state->chord_fired = 1;
            state->latch_until_ms = now_ms + 400; /* Release latch lockout */
            return DOCK_EVT_PANIC_LOCK;
        }
    } else {
        state->chord_press_start_ms = 0;
    }

    /* Reset chord fired flag once both are released */
    if (!(buttons_down & (BTN_MASK_L | BTN_MASK_R))) {
        state->chord_fired = 0;
    }

    /* 2. Slider Open Direct Shortcuts */
    if (buttons_pressed & BTN_MASK_TRIANGLE) {
        return DOCK_EVT_OPEN_VAULT;
    }
    if (buttons_pressed & BTN_MASK_SELECT) {
        return DOCK_EVT_OPEN_SETTINGS;
    }

    /* 3. Right Trigger (R) Gesture Tracking */
    if (buttons_pressed & BTN_MASK_R) {
        state->r_press_start_ms = now_ms;
        state->r_long_fired = 0;
    }

    if (buttons_down & BTN_MASK_R) {
        if (!both_triggers_down && !state->r_long_fired && (now_ms - state->r_press_start_ms) >= 1200) {
            state->r_long_fired = 1;
            state->preview_active = 1;
            return DOCK_EVT_PREVIEW_NEXT_START;
        }
    }

    if (buttons_released & BTN_MASK_R) {
        if (state->preview_active) {
            state->preview_active = 0;
            return DOCK_EVT_PREVIEW_NEXT_STOP;
        }
        if (!state->r_long_fired && !state->chord_fired && (now_ms >= state->latch_until_ms)) {
            if ((now_ms - state->r_press_start_ms) < 600) {
                return DOCK_EVT_NEXT_ACCOUNT;
            }
        }
    }

    /* 4. Left Trigger (L) Gesture Tracking */
    if (buttons_pressed & BTN_MASK_L) {
        state->l_press_start_ms = now_ms;
        state->l_long_fired = 0;
    }

    if (buttons_down & BTN_MASK_L) {
        if (!both_triggers_down && !state->l_long_fired && (now_ms - state->l_press_start_ms) >= 1200) {
            state->l_long_fired = 1;
            return DOCK_EVT_CYCLE_THEME;
        }
    }

    if (buttons_released & BTN_MASK_L) {
        if (!state->l_long_fired && !state->chord_fired && (now_ms >= state->latch_until_ms)) {
            if ((now_ms - state->l_press_start_ms) < 600) {
                return DOCK_EVT_PREV_ACCOUNT;
            }
        }
    }

    return DOCK_EVT_NONE;
}
