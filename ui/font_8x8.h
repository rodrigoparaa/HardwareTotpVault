#ifndef FONT_8X8_H
#define FONT_8X8_H

#include <stdint.h>
#include "graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

void font_draw_char(int x, int y, uint8_t ch, int scale, uint32_t color);

int font_string_width(const char *str, int scale);

void font_draw_string(int x, int y, const char *str, int scale, uint32_t color);

void font_draw_string_centered(int y, const char *str, int scale, uint32_t color);

void font_draw_string_truncated(int x, int y, const char *str, int max_w, int scale, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif /* FONT_8X8_H */
