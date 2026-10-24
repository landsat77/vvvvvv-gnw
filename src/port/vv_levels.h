/*
 * The level and script tables of the game, taken out of its code by
 * tools/gen_levels.py into vv_levels.bin (in the pack): vv_levels.cpp
 * looks them up in place.
 */
#ifndef VV_LEVELS_H
#define VV_LEVELS_H

#include <vector>
#include <string>

/* table `id`: the shorts of a room / tower (read only, in the flash) */
const short* vv_level_table(int id);

/* script table `id`: its lines added to `commands` (filllines()) */
void vv_fill_script(std::vector<std::string>& commands, int id);

/* the pack's vv_levels.bin matches this binary */
bool vv_levels_check(const char** error);

#endif
