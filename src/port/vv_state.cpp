/*
 * Savestates of the port: the game's own quick save (qsave.vvv, XML),
 * written to the savestate file of retro-go instead, with the position
 * the player is at (not the last checkpoint) when it is safe.
 *
 * Loading is deferred: the file is read (vv_core_load_state), then the
 * main loop is restarted (vv_core_poll_restart -> longjmp to main.cpp)
 * and starts the game like "Continue" does (Start_MAINGAME_QUICKSAVE),
 * with the file standing for saves/qsave.vvv (vv_fs.cpp).
 */
#include <stdio.h>
#include <string.h>

#include <tinyxml2.h>

#include "Entity.h"
#include "Enums.h"
#include "Game.h"
#include "Map.h"
#include "Script.h"
#include "Vlogging.h"

#include "vv_core.h"
#include "vv_platform.h"

void vv_fs_override(const char* name, const char* data, size_t size);

jmp_buf vv_core_restart_point;
static bool loop_ready;
static char* pending;        /* XML of the state to load */
static size_t pending_size;
static bool pending_jump;

extern "C" bool vv_core_can_save(void)
{
    /* not in a cutscene (its script is not saved: loading would give a
     * room without the rest of the cutscene) */
    return loop_ready
        && (game.gamestate == GAMEMODE || game.gamestate == MAPMODE)
        && !map.custommode && !game.inspecial()
        && !script.running && game.hascontrol
        && obj.getplayer() > -1;
}

extern "C" bool vv_core_save_state(const char* path)
{
    if (!vv_core_can_save())
    {
        return false;
    }
    /* the player where he is, when nothing is going on (else the last
     * checkpoint, like the game's own save) */
    const int savex = game.savex, savey = game.savey, saverx = game.saverx, savery = game.savery;
    const int savegc = game.savegc, savedir = game.savedir;
    const int p = obj.getplayer();
    if (game.gamestate == GAMEMODE && game.deathseq == -1 && !map.towermode
        && !game.teleport && !game.completestop)
    {
        const entclass& pl = obj.entities[p];
        game.savex = pl.xp;
        game.savey = pl.yp;
        game.saverx = game.roomx;
        game.savery = game.roomy;
        game.savegc = game.gravitycontrol;
        game.savedir = pl.dir;
    }

    tinyxml2::XMLDocument doc;
    game.writemaingamesave(doc);

    game.savex = savex;
    game.savey = savey;
    game.saverx = saverx;
    game.savery = savery;
    game.savegc = savegc;
    game.savedir = savedir;

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    FILE* f = fopen(path, "wb");
    if (f == NULL)
    {
        return false;
    }
    const size_t len = printer.CStrSize() - 1;
    bool ok = fwrite(printer.CStr(), 1, len, f) == len;
    ok = fclose(f) == 0 && ok;
    vlog_info("savestate %s: %s", path, ok ? "saved" : "failed");
    return ok;
}

extern "C" bool vv_core_load_state(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (f == NULL)
    {
        return false;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 256 * 1024)
    {
        fclose(f);
        return false;
    }
    char* buf = (char*) SDL_malloc((size_t) n + 1);
    if (buf == NULL || fread(buf, 1, (size_t) n, f) != (size_t) n || memcmp(buf, "<?xml", 5) != 0)
    {
        SDL_free(buf);
        fclose(f);
        return false;
    }
    fclose(f);
    buf[n] = '\0';
    SDL_free(pending);
    pending = buf;
    pending_size = (size_t) n;
    pending_jump = true;
    return true;
}

extern "C" void vv_core_poll_restart(void)
{
    if (pending_jump && loop_ready)
    {
        pending_jump = false;
        longjmp(vv_core_restart_point, 1);
    }
}

extern "C" void vv_core_loop_ready(void)
{
    loop_ready = true;
    /* a state given by the launcher */
    vv_core_poll_restart();
}

extern "C" void vv_core_apply_pending_load(void)
{
    if (pending == NULL)
    {
        return;
    }
    vlog_info("savestate: loading");
    vv_fs_override("saves/qsave.vvv", pending, pending_size);
    game.gamestate = TITLEMODE;
    script.startgamemode(Start_MAINGAME_QUICKSAVE);
    vv_fs_override(NULL, NULL, 0);
    SDL_free(pending);
    pending = NULL;
}
