#ifndef FONT_7SEG_H
#define FONT_7SEG_H

#include <stdint.h>
#include "graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Draws a single 7-segment character.
 *
 * @param x Top-left X coordinate.
 * @param y Top-left Y coordinate.
 * @param w Width of the digit.
 * @param h Height of the digit.
 * @param thickness Stroke thickness in pixels (typically h / 8).
 * @param c Character ('0'..'9', '-', ' ').
 * @param active_color RGBA color for lit segments.
 * @param ghost_color RGBA color for unlit background segments (or 0 for transparent).
 */
void font_7seg_draw_digit(int x, int y, int w, int h, int thickness,
                          char c, uint32_t active_color, uint32_t ghost_color);

/**
 * Draws a formatted OTP code (e.g. "813 325" or "1234 5678") centered horizontally.
 *
 * @param center_x Horizontal center point (typically 240 for PSP).
 * @param y Top Y coordinate.
 * @param code_str Formatted string containing digits and spaces.
 * @param digits 6 or 8.
 * @param h Height of digits.
 * @param active_color Lit segment color.
 * @param ghost_color Unlit background segment color.
 */
void font_7seg_draw_otp(int center_x, int y, const char *code_str,
                        int digits, int h,
                        uint32_t active_color, uint32_t ghost_color);

#ifdef __cplusplus
}
#endif

#endif /* FONT_7SEG_H */
