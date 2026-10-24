/* Additions of the port to its small SDL (vv_sdl.c) */
#ifndef VV_SDL_H_PORT
#define VV_SDL_H_PORT

#include <stdint.h>
#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

/* colour of an image texture (GraphicsResources.cpp TextureLoadType) */
enum { VV_SDL_LOAD_COLOR = 0, VV_SDL_LOAD_WHITE = 1, VV_SDL_LOAD_GRAYSCALE = 2 };

/* A texture / an 8 bit surface over a "GWIM" image of the pack (used in
 * place: the data must stay valid) */
SDL_Texture *vv_sdl_image_texture(const uint8_t *data, uint32_t size, int load);
SDL_Surface *vv_sdl_image_surface(const uint8_t *data, uint32_t size);

/* This render target texture is the frame (see vv_sdl.c) */
void vv_sdl_texture_is_screen(SDL_Texture *t);

/* Draws go `dx`, `dy` further, clipped to the moved screen (until the
 * next present) */
void vv_sdl_set_offset(int dx, int dy);
/* The frame being drawn, flipped upside down / moved (black comes in) */
void vv_sdl_screen_flip_vertical(void);
void vv_sdl_screen_shift(int dx, int dy);

/* The keys held, made from the buttons (vv_core.cpp); events follow */
void vv_sdl_set_keys(const SDL_Keycode *keys, int n);
/* vv_core.cpp: called by SDL_PollEvent after the platform's service */
void vv_core_update_keys(void);

#ifdef __cplusplus
}
#endif

#endif
