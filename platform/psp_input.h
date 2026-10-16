#ifndef PSP_INPUT_H
#define PSP_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t buttons_down;
    uint32_t buttons_pressed;
    uint32_t buttons_released;
    int8_t   analog_x;
    int8_t   analog_y;
} psp_input_frame_t;

void psp_input_init(void);
void psp_input_poll(psp_input_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* PSP_INPUT_H */
