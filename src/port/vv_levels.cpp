/*
 * The level and script tables of the game, in the pack (vv_levels.bin,
 * written by tools/gen_levels.py from the game's sources at build time;
 * format there). Read in place.
 */
#include <string.h>
#include <stdint.h>

#include "vv_levels.h"
#include "vv_levels_gen.h"
#include "vv_platform.h"

static const uint8_t* blob;
static uint32_t blob_size;
static bool blob_ok;

static uint32_t rd32(const uint8_t* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t) p[3] << 24);
}

bool vv_levels_check(const char** error)
{
    if (!vv_plat_find_file("vv_levels.bin", &blob, &blob_size))
    {
        *error = "no vv_levels.bin in the pack";
        return false;
    }
    if (blob_size < 16 + 8 * VV_LEVELS_COUNT || memcmp(blob, "GWVL01\0\0", 8) != 0
        || rd32(blob + 8) != VV_LEVELS_COUNT)
    {
        *error = "vv_levels.bin of the pack is invalid";
        return false;
    }
    if (rd32(blob + 12) != VV_LEVELS_HASH)
    {
        *error = "the pack was made for another version (vv_levels.bin)";
        return false;
    }
    for (uint32_t i = 0; i < VV_LEVELS_COUNT; i++)
    {
        uint32_t off = rd32(blob + 16 + i * 8), size = rd32(blob + 20 + i * 8);
        if ((uint64_t) off + size > blob_size || (off & 3) != 0)
        {
            *error = "vv_levels.bin of the pack is invalid";
            return false;
        }
    }
    blob_ok = true;
    return true;
}

static const uint8_t* table(int id, uint32_t* size)
{
    if (!blob_ok || id < 0 || id >= VV_LEVELS_COUNT)
    {
        vv_plat_fatal("level table missing (vv_levels.bin)");
    }
    *size = rd32(blob + 20 + id * 8);
    return blob + rd32(blob + 16 + id * 8);
}

const short* vv_level_table(int id)
{
    uint32_t size;
    return (const short*) table(id, &size);
}

void vv_fill_script(std::vector<std::string>& commands, int id)
{
    uint32_t size;
    const uint8_t* p = table(id, &size);
    const uint8_t* end = p + size;
    uint32_t n = rd32(p);
    p += 4;
    commands.reserve(commands.size() + n);
    for (uint32_t i = 0; i < n && p < end; i++)
    {
        size_t len = strnlen((const char*) p, end - p);
        commands.push_back(std::string((const char*) p, len));
        p += len + 1;
    }
}
