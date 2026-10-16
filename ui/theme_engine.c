#include "theme_engine.h"
#include "graphics.h"

static const theme_palette_t PALETTES[3] = {
    {
        .name           = "CYBERPUNK",
        .bg             = COLOR_RGB(13, 17, 23),      /* #0D1117 */
        .card_bg        = COLOR_RGB(22, 27, 34),      /* #161B22 */
        .text_primary   = COLOR_RGB(201, 209, 217),  /* #C9D1D9 */
        .text_secondary = COLOR_RGB(139, 148, 158),  /* #8B949E */
        .accent         = COLOR_RGB(88, 166, 255),    /* #58A6FF Cyan */
        .ghost_7seg     = COLOR_RGB(28, 35, 45),
        .warning        = COLOR_RGB(210, 153, 34),    /* #D29922 Amber */
        .critical       = COLOR_RGB(248, 81, 73)      /* #F85149 Red */
    },
    {
        .name           = "STEALTH RED",
        .bg             = COLOR_RGB(5, 5, 5),         /* #050505 */
        .card_bg        = COLOR_RGB(25, 10, 10),      /* #190A0A */
        .text_primary   = COLOR_RGB(240, 240, 240),  /* #F0F0F0 */
        .text_secondary = COLOR_RGB(150, 90, 90),
        .accent         = COLOR_RGB(255, 51, 51),     /* #FF3333 Crimson */
        .ghost_7seg     = COLOR_RGB(45, 12, 12),
        .warning        = COLOR_RGB(255, 160, 0),
        .critical       = COLOR_RGB(255, 20, 20)
    },
    {
        .name           = "OLED GOLD",
        .bg             = COLOR_RGB(0, 0, 0),         /* #000000 */
        .card_bg        = COLOR_RGB(18, 16, 8),       /* #121008 */
        .text_primary   = COLOR_RGB(230, 237, 243),  /* #E6EDF3 */
        .text_secondary = COLOR_RGB(140, 130, 96),
        .accent         = COLOR_RGB(242, 204, 96),    /* #F2CC60 Gold */
        .ghost_7seg     = COLOR_RGB(38, 32, 16),
        .warning        = COLOR_RGB(229, 160, 32),
        .critical       = COLOR_RGB(255, 77, 77)
    }
};

const theme_palette_t *theme_get(uint8_t index) {
    if (index >= 3) index = 0;
    return &PALETTES[index];
}

const char *theme_get_name(uint8_t index) {
    if (index >= 3) index = 0;
    return PALETTES[index].name;
}
