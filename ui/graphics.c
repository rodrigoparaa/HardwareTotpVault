#include "graphics.h"
#include <string.h>

#if defined(PSP) || defined(__PSP__)
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>

static unsigned int __attribute__((aligned(16))) g_dlist[262144];
static void *g_fbp0 = (void *)0;
static void *g_fbp1 = (void *)0x88000;

typedef struct {
    uint32_t color;
    int16_t  x, y, z;
} Vertex2D;

void gfx_init(void) {
    sceGuInit();
    sceGuStart(GU_DIRECT, g_dlist);

    sceGuDrawBuffer(GU_PSM_8888, g_fbp0, BUFFER_STRIDE);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, g_fbp1, BUFFER_STRIDE);
    sceGuDepthBuffer((void *)0x110000, BUFFER_STRIDE);

    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);

    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void gfx_start_frame(void) {
    sceGuStart(GU_DIRECT, g_dlist);
}

void gfx_end_frame(void) {
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

void gfx_finish(void) {
    sceGuTerm();
}

void gfx_clear(uint32_t color) {
    sceGuClearColor(color);
    sceGuClear(GU_COLOR_BUFFER_BIT);
}

void gfx_draw_rect_fill(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    Vertex2D *v = (Vertex2D *)sceGuGetMemory(2 * sizeof(Vertex2D));
    if (!v) return;

    v[0].color = color;
    v[0].x = (int16_t)x;
    v[0].y = (int16_t)y;
    v[0].z = 0;

    v[1].color = color;
    v[1].x = (int16_t)(x + w);
    v[1].y = (int16_t)(y + h);
    v[1].z = 0;

    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, v);
}

void gfx_draw_rect_outline(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    gfx_draw_rect_fill(x, y, w, 1, color);             /* Top */
    gfx_draw_rect_fill(x, y + h - 1, w, 1, color);     /* Bottom */
    gfx_draw_rect_fill(x, y, 1, h, color);             /* Left */
    gfx_draw_rect_fill(x + w - 1, y, 1, h, color);     /* Right */
}

void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    Vertex2D *v = (Vertex2D *)sceGuGetMemory(2 * sizeof(Vertex2D));
    if (!v) return;

    v[0].color = color;
    v[0].x = (int16_t)x0;
    v[0].y = (int16_t)y0;
    v[0].z = 0;

    v[1].color = color;
    v[1].x = (int16_t)x1;
    v[1].y = (int16_t)y1;
    v[1].z = 0;

    sceGuDrawArray(GU_LINES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, v);
}

#else
/* Host stub */
void gfx_init(void) {}
void gfx_start_frame(void) {}
void gfx_end_frame(void) {}
void gfx_finish(void) {}
void gfx_clear(uint32_t color) { (void)color; }
void gfx_draw_rect_fill(int x, int y, int w, int h, uint32_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}
void gfx_draw_rect_outline(int x, int y, int w, int h, uint32_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
}
#endif
