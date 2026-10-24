#ifndef TOWERBG_H
#define TOWERBG_H

#include <SDL.h>

struct TowerBG
{
    SDL_Texture* texture;
    bool tdrawback;
    int bypos;
    int bscroll;
    int colstate;
    int scrolldir;
    int r;
    int g;
    int b;
#ifdef GNW
    /* what the texture would hold (Graphics.cpp draws it every frame):
     * the tower background from this position, in these colours */
    int gnw_pos;
    int gnw_colstate;
#endif
};

#endif /* TOWERBG_H */
