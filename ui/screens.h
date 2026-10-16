#ifndef SCREENS_H
#define SCREENS_H

#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void screens_render(AppState *app, uint64_t now_utc_sec, uint64_t now_utc_ms, uint64_t now_ticks_ms);

#ifdef __cplusplus
}
#endif

#endif /* SCREENS_H */
