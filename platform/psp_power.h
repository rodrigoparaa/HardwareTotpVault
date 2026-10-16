#ifndef PSP_POWER_H
#define PSP_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*psp_exit_callback_t)(void);

void psp_power_init(psp_exit_callback_t exit_cb);
void psp_power_tick(void);
void psp_power_set_max_clock(void);

int psp_power_get_battery_percent(void);
int psp_power_is_charging(void);
int psp_power_is_online(void);

#ifdef __cplusplus
}
#endif

#endif /* PSP_POWER_H */
