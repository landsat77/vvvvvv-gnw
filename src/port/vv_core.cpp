/*
 * Runs VVVVVV for the platform (vv_platform.h): its main() (main.cpp,
 * vv_game_main under GNW), the buttons turned into the keys of the PC
 * version, and the copy of the game kept behind the sliding menus.
 */
#include <string.h>
#include <stdlib.h>

#include <SDL.h>

#include "Game.h"
#include "Graphics.h"
#include "Enums.h"

#include "vv_core.h"
#include "vv_platform.h"
#include "vv_sdl.h"
#include "vv_levels.h"

int vv_game_main(int argc, char* argv[]);

/* ------------------------------------------------------------------ */
/* buttons -> keys                                                    */
/* ------------------------------------------------------------------ */
static uint32_t buttons;

extern "C" void vv_core_set_buttons(uint32_t b)
{
    buttons = b;
}

/* In the game: A = Z (flip, action), B = V (flip too), GAME = Esc (the
 * pause menu), TIME = Enter (the map, terminals...). In the menus: A = Z
 * (select), B = Esc (back). The D-pad is the arrow keys. */
extern "C" void vv_core_update_keys(void)
{
    SDL_Keycode keys[8];
    int n = 0;
    const bool in_game = game.gamestate == GAMEMODE;
    if (buttons & VV_BTN_LEFT)  keys[n++] = SDLK_LEFT;
    if (buttons & VV_BTN_RIGHT) keys[n++] = SDLK_RIGHT;
    if (buttons & VV_BTN_UP)    keys[n++] = SDLK_UP;
    if (buttons & VV_BTN_DOWN)  keys[n++] = SDLK_DOWN;
    if (buttons & VV_BTN_A)     keys[n++] = SDLK_z;
    if (buttons & VV_BTN_B)     keys[n++] = in_game ? SDLK_v : SDLK_ESCAPE;
    if (buttons & VV_BTN_GAME)  keys[n++] = SDLK_ESCAPE;
    if (buttons & VV_BTN_TIME)  keys[n++] = SDLK_RETURN;
    vv_sdl_set_keys(keys, n);
}

/* ------------------------------------------------------------------ */
/* the game behind the menus                                          */
/* ------------------------------------------------------------------ */
/* The map / pause menu slides over the last frame of the game
 * (Graphics::menuoffrender, gameplayTexture). The frame shown when the
 * menu opens is kept here (RGB332, 75 KB) while the menu is open. */
static uint8_t* snapshot;

extern "C" void vv_core_game_frame(void)
{
    if (snapshot != NULL)
    {
        SDL_free(snapshot);
        snapshot = NULL;
    }
}

static void take_snapshot(void)
{
    const uint16_t* shown = vv_plat_shown_buffer();
    if (shown == NULL)
    {
        return;
    }
    snapshot = (uint8_t*) SDL_malloc(VV_WIDTH * VV_HEIGHT);
    if (snapshot == NULL)
    {
        return;
    }
    for (int y = 0; y < VV_HEIGHT; y++)
    {
        /* the frame shown is upside down in flip mode */
        const uint16_t* src = shown + (graphics.flipmode ? VV_HEIGHT - 1 - y : y) * VV_WIDTH;
        uint8_t* dst = snapshot + y * VV_WIDTH;
        for (int x = 0; x < VV_WIDTH; x++)
        {
            const uint16_t c = src[x];
            dst[x] = (uint8_t) (((c >> 13) << 5) | (((c >> 8) & 7) << 2) | ((c >> 3) & 3));
        }
    }
}

static void draw_snapshot(int lines)
{
    uint16_t* fb = vv_plat_draw_buffer();
    if (snapshot == NULL)
    {
        memset(fb, 0, (size_t) lines * VV_WIDTH * 2);
        return;
    }
    static uint16_t rgb[256];
    static bool rgb_ready;
    if (!rgb_ready)
    {
        for (int i = 0; i < 256; i++)
        {
            const int r = (i >> 5) & 7, g = (i >> 2) & 7, b = i & 3;
            rgb[i] = (uint16_t) (((r * 31 / 7) << 11) | ((g * 63 / 7) << 5) | (b * 31 / 3));
        }
        rgb_ready = true;
    }
    for (int i = 0; i < lines * VV_WIDTH; i++)
    {
        fb[i] = rgb[snapshot[i]];
    }
}

extern "C" void vv_core_menu_begin(void)
{
    if (snapshot == NULL)
    {
        take_snapshot();
    }
    if (!graphics.resumegamemode && graphics.menuoffset <= 0 && graphics.oldmenuoffset <= 0)
    {
        return; /* the menu covers the screen */
    }
    const int offset = (int) graphics.lerp(graphics.oldmenuoffset, graphics.menuoffset);
    /* the game above the menu, then the menu (cleared again: it is
     * opaque) moved down */
    draw_snapshot(SDL_clamp(offset, 0, VV_HEIGHT));
    vv_sdl_set_offset(0, offset);
    graphics.clear();
}

/* ------------------------------------------------------------------ */
/* run                                                                */
/* ------------------------------------------------------------------ */
extern "C" void vv_core_run(const vv_core_options_t* opts)
{
    const char* error;
    if (!vv_levels_check(&error))
    {
        vv_plat_fatal(error);
    }
    vv_core_set_sound(opts->music, opts->sound);
    static char arg0[] = "VVVVVV";
    static char* argv[] = { arg0, NULL };
    vv_game_main(1, argv);
    vv_plat_quit();
}
