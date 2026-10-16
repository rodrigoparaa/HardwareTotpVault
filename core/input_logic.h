#ifndef INPUT_LOGIC_H
#define INPUT_LOGIC_H

#include <stdint.h>
#include <stddef.h>
#include "vault.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Standard PSP button bitmasks (portable definition for host tests and target) */
#define BTN_MASK_SELECT   0x000001
#define BTN_MASK_START    0x000008
#define BTN_MASK_UP       0x000010
#define BTN_MASK_RIGHT    0x000020
#define BTN_MASK_DOWN     0x000040
#define BTN_MASK_LEFT     0x000080
#define BTN_MASK_L        0x000100
#define BTN_MASK_R        0x000200
#define BTN_MASK_TRIANGLE 0x001000
#define BTN_MASK_CIRCLE   0x002000
#define BTN_MASK_CROSS    0x004000
#define BTN_MASK_SQUARE   0x008000

/* Gesture event types in TOTP dock mode */
typedef enum {
    DOCK_EVT_NONE = 0,
    DOCK_EVT_NEXT_ACCOUNT,
    DOCK_EVT_PREV_ACCOUNT,
    DOCK_EVT_PREVIEW_NEXT_START,
    DOCK_EVT_PREVIEW_NEXT_STOP,
    DOCK_EVT_CYCLE_THEME,
    DOCK_EVT_PANIC_LOCK,
    DOCK_EVT_OPEN_VAULT,
    DOCK_EVT_OPEN_SETTINGS,
    DOCK_EVT_HOTP_INCREMENT
} dock_event_t;

/* Gesture event types in Vault browser/detail mode */
typedef enum {
    VAULT_EVT_NONE = 0,
    VAULT_EVT_UP,
    VAULT_EVT_DOWN,
    VAULT_EVT_SELECT,
    VAULT_EVT_BACK,
    VAULT_EVT_REVEAL_ON,
    VAULT_EVT_REVEAL_OFF,
    VAULT_EVT_PANIC_LOCK
} vault_event_t;

/* PIN input state */
typedef struct {
    uint8_t buffer[8];
    uint8_t count;
    uint8_t max_len;
    uint8_t failed_attempts;
    uint64_t cooldown_until_ms;
    uint8_t trigger_submit;
} PinInputState;

/* Dock gesture tracker state */
typedef struct {
    uint64_t l_press_start_ms;
    uint64_t r_press_start_ms;
    uint64_t chord_press_start_ms;
    uint64_t latch_until_ms;
    int      l_long_fired;
    int      r_long_fired;
    int      chord_fired;
    int      preview_active;
} DockGestureState;

void pin_input_init(PinInputState *state, uint8_t max_len);
void pin_input_reset(PinInputState *state);

/**
 * Feeds a raw button bitmask update into the PIN input processor.
 * Automatically appends button code, handles backspace/reset and cooldown penalties.
 *
 * @param state PIN input state.
 * @param buttons_down Bitmask of buttons currently held.
 * @param buttons_pressed Bitmask of buttons pressed down on this frame (edge-triggered).
 * @param now_ms Current timestamp in milliseconds.
 * @return 1 if a button was accepted or submitted, 0 if ignored/cooldown.
 */
int pin_input_update(PinInputState *state,
                     uint32_t buttons_down,
                     uint32_t buttons_pressed,
                     uint64_t now_ms);

void dock_gesture_init(DockGestureState *state);

/**
 * Evaluates physical gestures for Dock mode (Slider closed L/R gestures & slider open shortcuts).
 *
 * @param state Dock gesture state.
 * @param buttons_down Currently held buttons.
 * @param buttons_pressed Newly pressed buttons on this frame.
 * @param buttons_released Released buttons on this frame.
 * @param now_ms Current timestamp in milliseconds.
 * @return Detected dock_event_t event.
 */
dock_event_t dock_gesture_update(DockGestureState *state,
                                 uint32_t buttons_down,
                                 uint32_t buttons_pressed,
                                 uint32_t buttons_released,
                                 uint64_t now_ms);

/**
 * Maps a single button mask bit to its canonical vault_button_code_t.
 */
vault_button_code_t button_mask_to_code(uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_LOGIC_H */
