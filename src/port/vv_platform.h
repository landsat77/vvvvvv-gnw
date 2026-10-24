/*
 * Interface between VVVVVV (src/vvvvvv + the mini SDL of src/port/sdl) and
 * the platform it runs on:
 *  - src/port/vv_main.c : the Game & Watch homebrew (retro-go)
 *  - harness/vv_host.c  : a headless Linux harness used to test the port
 */
#ifndef VV_PLATFORM_H
#define VV_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The game's screen (and the console's) */
#define VV_WIDTH  320
#define VV_HEIGHT 240

/* Output sample rate of the port (mono, signed 16 bit). The music and the
 * sound effects of the pack are stored at this rate (vv_pack.py). */
#define VV_SAMPLE_RATE 32000

/* Buttons of the console, as seen by the core (vv_core_set_buttons()) */
enum {
    VV_BTN_LEFT   = 1 << 0,
    VV_BTN_RIGHT  = 1 << 1,
    VV_BTN_UP     = 1 << 2,
    VV_BTN_DOWN   = 1 << 3,
    VV_BTN_A      = 1 << 4,
    VV_BTN_B      = 1 << 5,
    VV_BTN_GAME   = 1 << 6,   /* START on Zelda units */
    VV_BTN_TIME   = 1 << 7,   /* SELECT on Zelda units */
};

/* ---- implemented by the platform ---- */

/* Monotonic time in milliseconds (game time: not while the pause menu of
 * the platform is open). */
uint32_t vv_plat_time_ms(void);

/* Called while the game waits (in_delay = 1, SDL_Delay) and when it reads
 * its events (in_delay = 0, SDL_PollEvent). The platform feeds its audio
 * output with vv_core_audio_render(), reads its buttons and gives them to
 * vv_core_set_buttons(). */
void vv_plat_service(int in_delay);

/* The screen the game draws to: VV_WIDTH x VV_HEIGHT RGB565 pixels, line
 * after line (no padding). It changes after vv_plat_present(). */
uint16_t *vv_plat_draw_buffer(void);

/* The frame drawn in vv_plat_draw_buffer() is complete: show it. */
void vv_plat_present(void);

/* The frame shown now (for a copy: the menus slide over it). NULL if
 * the platform cannot give it. */
const uint16_t *vv_plat_shown_buffer(void);

/* Look up a file of the asset pack (names as in the game: "graphics/
 * tiles.png"...). The memory is read-only and stays valid. */
bool vv_plat_find_file(const char *name, const uint8_t **data, uint32_t *size);

/* Name of the file `index` of the pack (NULL past the last one). */
const char *vv_plat_pack_name(uint32_t index);

/* Directory (without trailing '/') where the game may write its files
 * (settings.vvv, unlock.vvv, saves). */
const char *vv_plat_save_dir(void);

/* Unrecoverable error. Never returns. */
void vv_plat_fatal(const char *msg) __attribute__((noreturn));

/* The game asked to quit. Never returns. */
void vv_plat_quit(void) __attribute__((noreturn));

/* ---- implemented by the core (vv_core.cpp, vv_sdl.cpp, vv_music.cpp) ---- */

typedef struct {
    bool music;
    bool sound;
} vv_core_options_t;

/* Run the game. Never returns. */
void vv_core_run(const vv_core_options_t *opts) __attribute__((noreturn));

/* Buttons currently held (VV_BTN_*). */
void vv_core_set_buttons(uint32_t buttons);

/* Savestates (vv_state.cpp): the game's quick save (the last
 * checkpoint), in its own format. */
bool vv_core_can_save(void);
bool vv_core_save_state(const char *path);
bool vv_core_load_state(const char *path);
/* After a service of the platform (outside its menu): a load waiting
 * restarts the game's loop (longjmp). */
void vv_core_poll_restart(void);

/* Options of the pause menu */
void vv_core_set_sound(bool music, bool sound);

/* Render `frames` mono samples (sound effects and music). */
void vv_core_audio_render(int16_t *out, int frames);

/* ---- implemented by vv_alloc.c ---- */
void   vv_heap_init(void *base, size_t size);
void  *vv_malloc(size_t size);
void  *vv_calloc(size_t n, size_t size);
void  *vv_realloc(void *ptr, size_t size);
void   vv_free(void *ptr);
size_t vv_heap_used(void);
size_t vv_heap_peak(void);
size_t vv_heap_size(void);
void   vv_heap_ignore_range(const void *lo, const void *hi);

#ifdef __cplusplus
}
#endif

#endif
