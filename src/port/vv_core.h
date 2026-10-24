/*
 * Hooks of the port called from the game's code (under #ifdef GNW),
 * implemented in vv_core.cpp / vv_state.cpp.
 */
#ifndef VV_CORE_H
#define VV_CORE_H

#include <setjmp.h>
#include "vv_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* main.cpp: the main loop is entered (savestates may load from now on) /
 * a savestate load jumps back here, the game state is set by
 * vv_core_apply_pending_load() */
extern jmp_buf vv_core_restart_point;
void vv_core_loop_ready(void);
void vv_core_apply_pending_load(void);

/* Render.cpp: a frame of the game is drawn (gamerender) / a menu that
 * slides over the frozen game is drawn (maprender, teleporterrender) */
void vv_core_game_frame(void);
void vv_core_menu_begin(void);

#ifdef __cplusplus
}
#endif

#endif
