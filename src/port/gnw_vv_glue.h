/*
 * Forced include (-include) of the C files of the game and of c-hashmap
 * (and of Font.cpp): their malloc & co use the game heap (vv_alloc.c),
 * like SDL_malloc and operator new (vv_cxx.cpp).
 */
#ifndef GNW_VV_GLUE_H
#define GNW_VV_GLUE_H

#ifndef GNW
#define GNW 1
#endif

#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#endif
#include "vv_platform.h"

#define malloc(n)     vv_malloc(n)
#define calloc(n, s)  vv_calloc(n, s)
#define realloc(p, n) vv_realloc(p, n)
#define free(p)       vv_free(p)

#endif
