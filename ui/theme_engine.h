#ifndef THEME_ENGINE_H
#define THEME_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    uint32_t bg;
    uint32_t card_bg;
    uint32_t text_primary;
    uint32_t text_secondary;
    uint32_t accent;
    uint32_t ghost_7seg;
    uint32_t warning;
    uint32_t critical;
} theme_palette_t;

const theme_palette_t *theme_get(uint8_t index);
const char *theme_get_name(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* THEME_ENGINE_H */
