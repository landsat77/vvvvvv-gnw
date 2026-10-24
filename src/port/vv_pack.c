/*
 * Asset pack (".vvv" file) reader for the VVVVVV homebrew.
 *
 * A pack is produced by tools/vv_pack.py from the data.zip of VVVVVV 2.4. It
 * is a plain container of the original files (images converted to 8 bit, the music
 * and sounds converted, the level tables of the code):
 *
 *   0x00  char  magic[8]    "GWVV01\0\0"
 *   0x08  u32   file_count
 *   0x0C  u32   total_size  (size of the whole pack, for validation)
 *   0x10  entry entries[file_count]
 *           char name[40]   NUL terminated, lower case ("graphics/tiles.png", ...);
 *                           looked up without regard to case
 *           u32  offset     from the start of the pack, 4 byte aligned
 *           u32  size
 *   ...   file data
 *
 * The game reads the files from the memory-mapped flash (vv_file.c).
 */
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "vv_pack.h"

typedef struct {
    char name[40];
    uint32_t offset;
    uint32_t size;
} __attribute__((packed)) pack_entry_t;

static const uint8_t *pack_data;
static uint32_t pack_size;
static uint32_t pack_count;

static int name_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == *b;
}

int vv_pack_open(const uint8_t *data, uint32_t size)
{
    pack_data = NULL;
    pack_size = 0;
    pack_count = 0;
    if (data == NULL || size < 16 || memcmp(data, VV_PACK_MAGIC, 8) != 0)
        return VV_PACK_ERR_MAGIC;
    uint32_t count, total;
    memcpy(&count, data + 8, 4);
    memcpy(&total, data + 12, 4);
    if (total != size || count == 0 || 16 + (uint64_t)count * sizeof(pack_entry_t) > size)
        return VV_PACK_ERR_CORRUPT;
    const pack_entry_t *e = (const pack_entry_t *)(data + 16);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t off, sz;
        memcpy(&off, &e[i].offset, 4);
        memcpy(&sz, &e[i].size, 4);
        if ((uint64_t)off + sz > size || e[i].name[sizeof(e[i].name) - 1] != 0)
            return VV_PACK_ERR_CORRUPT;
    }
    pack_data = data;
    pack_size = size;
    pack_count = count;
    return VV_PACK_OK;
}

bool vv_pack_find(const char *name, const uint8_t **data, uint32_t *size)
{
    if (pack_data == NULL || name == NULL)
        return false;
    const pack_entry_t *e = (const pack_entry_t *)(pack_data + 16);
    for (uint32_t i = 0; i < pack_count; i++) {
        if (name_equal(e[i].name, name)) {
            uint32_t off, sz;
            memcpy(&off, &e[i].offset, 4);
            memcpy(&sz, &e[i].size, 4);
            if (data)
                *data = pack_data + off;
            if (size)
                *size = sz;
            return true;
        }
    }
    return false;
}

uint32_t vv_pack_count(void)
{
    return pack_count;
}

const char *vv_pack_name(uint32_t index)
{
    if (index >= pack_count)
        return NULL;
    return ((const pack_entry_t *)(pack_data + 16))[index].name;
}
