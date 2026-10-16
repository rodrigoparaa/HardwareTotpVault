#include "font_7seg.h"
#include <string.h>

static uint8_t get_segment_mask(char c) {
    switch (c) {
        case '0': return 0x3F; /* a,b,c,d,e,f */
        case '1': return 0x06; /* b,c */
        case '2': return 0x5B; /* a,b,d,e,g */
        case '3': return 0x4F; /* a,b,c,d,g */
        case '4': return 0x66; /* b,c,f,g */
        case '5': return 0x6D; /* a,c,d,f,g */
        case '6': return 0x7D; /* a,c,d,e,f,g */
        case '7': return 0x07; /* a,b,c */
        case '8': return 0x7F; /* a,b,c,d,e,f,g */
        case '9': return 0x6F; /* a,b,c,d,f,g */
        case '-': return 0x40; /* g */
        default:  return 0x00;
    }
}

void font_7seg_draw_digit(int x, int y, int w, int h, int thickness,
                          char c, uint32_t active_color, uint32_t ghost_color) {
    if (w <= 0 || h <= 0 || thickness <= 0) return;

    uint8_t mask = get_segment_mask(c);

    int seg_h = (h - (3 * thickness)) / 2;
    if (seg_h <= 0) seg_h = 1;
    int seg_w = w - (2 * thickness);
    if (seg_w <= 0) seg_w = 1;

    /* Segment a: top */
    uint32_t col_a = (mask & 0x01) ? active_color : ghost_color;
    if (col_a) gfx_draw_rect_fill(x + thickness, y, seg_w, thickness, col_a);

    /* Segment b: top-right */
    uint32_t col_b = (mask & 0x02) ? active_color : ghost_color;
    if (col_b) gfx_draw_rect_fill(x + w - thickness, y + thickness, thickness, seg_h, col_b);

    /* Segment c: bottom-right */
    uint32_t col_c = (mask & 0x04) ? active_color : ghost_color;
    if (col_c) gfx_draw_rect_fill(x + w - thickness, y + (2 * thickness) + seg_h, thickness, seg_h, col_c);

    /* Segment d: bottom */
    uint32_t col_d = (mask & 0x08) ? active_color : ghost_color;
    if (col_d) gfx_draw_rect_fill(x + thickness, y + h - thickness, seg_w, thickness, col_d);

    /* Segment e: bottom-left */
    uint32_t col_e = (mask & 0x10) ? active_color : ghost_color;
    if (col_e) gfx_draw_rect_fill(x, y + (2 * thickness) + seg_h, thickness, seg_h, col_e);

    /* Segment f: top-left */
    uint32_t col_f = (mask & 0x20) ? active_color : ghost_color;
    if (col_f) gfx_draw_rect_fill(x, y + thickness, thickness, seg_h, col_f);

    /* Segment g: middle */
    uint32_t col_g = (mask & 0x40) ? active_color : ghost_color;
    if (col_g) gfx_draw_rect_fill(x + thickness, y + thickness + seg_h, seg_w, thickness, col_g);
}

void font_7seg_draw_otp(int center_x, int y, const char *code_str,
                        int digits, int h,
                        uint32_t active_color, uint32_t ghost_color) {
    if (!code_str) return;

    int thickness = h / 8;
    if (thickness < 2) thickness = 2;

    int digit_w;
    int digit_gap;
    int group_gap;

    if (digits == 8) {
        digit_w = (h * 48) / 100;    /* Aspect ratio ~0.48 */
        digit_gap = thickness * 2;
        group_gap = thickness * 5;
    } else {
        /* 6 digits */
        digit_w = (h * 54) / 100;    /* Aspect ratio ~0.54 */
        digit_gap = thickness * 2;
        group_gap = thickness * 6;
    }

    /* Calculate total rendering width */
    int total_w = 0;
    size_t len = strlen(code_str);
    for (size_t i = 0; i < len; i++) {
        if (code_str[i] == ' ') {
            total_w += group_gap;
        } else {
            total_w += digit_w;
            if (i + 1 < len && code_str[i + 1] != ' ') {
                total_w += digit_gap;
            }
        }
    }

    int cur_x = center_x - (total_w / 2);

    for (size_t i = 0; i < len; i++) {
        char c = code_str[i];
        if (c == ' ') {
            cur_x += group_gap;
        } else {
            font_7seg_draw_digit(cur_x, y, digit_w, h, thickness, c, active_color, ghost_color);
            cur_x += digit_w;
            if (i + 1 < len && code_str[i + 1] != ' ') {
                cur_x += digit_gap;
            }
        }
    }
}
