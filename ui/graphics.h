#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272
#define BUFFER_STRIDE 512

#define COLOR_RGBA(r, g, b, a) \
    ((uint32_t)(r) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))

#define COLOR_RGB(r, g, b) COLOR_RGBA(r, g, b, 255)

void gfx_init(void);
void gfx_start_frame(void);
void gfx_end_frame(void);
void gfx_finish(void);

void gfx_clear(uint32_t color);
void gfx_draw_rect_fill(int x, int y, int w, int h, uint32_t color);
void gfx_draw_rect_outline(int x, int y, int w, int h, uint32_t color);
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif /* GRAPHICS_H */
