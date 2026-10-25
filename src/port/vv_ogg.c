/*
 * The original Ogg Vorbis songs (vv_pack.py --music ogg), decoded while
 * they play with stb_vorbis (public domain, tools/render/stb_vorbis.c),
 * stereo averaged to mono. The file is read in place from the pack.
 *
 * Memory: stb_vorbis builds the codebooks of the stream when it is opened:
 * ~125 KB for VVVVVV's 224 kbps songs with the options below (the lookup
 * tables computed on the fly, short fast Huffman tables). It works in one
 * arena of VV_OGG_ARENA bytes taken from the game heap at the first song
 * and kept (no fragmentation of the heap by the song changes; no alloca on
 * the 24 KB stack either: stb_vorbis takes its temporary memory from the
 * arena too).
 */
#include <stdint.h>
#include <string.h>

#include "vv_ogg.h"

#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_DIVIDES_IN_CODEBOOK
#define STB_VORBIS_FAST_HUFFMAN_LENGTH 8
#define STB_VORBIS_FAST_HUFFMAN_SHORT
#include "../../tools/render/stb_vorbis.c"
#undef L
#undef C
#undef R

#define VV_OGG_ARENA (128 * 1024)
static char *arena;

struct vv_ogg {
    stb_vorbis *v;
    int channels;
    int rate;
};

vv_ogg_t *vv_ogg_open(const uint8_t *data, uint32_t size)
{
    int err = 0;
    if (!arena) {
        arena = malloc(VV_OGG_ARENA);
        if (!arena)
            return NULL;
    }
    stb_vorbis_alloc alloc = { arena, VV_OGG_ARENA };
    stb_vorbis *v = stb_vorbis_open_memory(data, (int)size, &err, &alloc);
    if (!v)
        return NULL;
    vv_ogg_t *f = malloc(sizeof(*f));
    if (!f) {
        stb_vorbis_close(v);
        return NULL;
    }
    stb_vorbis_info info = stb_vorbis_get_info(v);
    f->v = v;
    f->channels = info.channels;
    f->rate = (int)info.sample_rate;
    return f;
}

void vv_ogg_close(vv_ogg_t *f)
{
    if (!f)
        return;
    stb_vorbis_close(f->v);
    free(f);
}

int vv_ogg_rate(const vv_ogg_t *f)
{
    return f ? f->rate : 0;
}

int vv_ogg_read(vv_ogg_t *f, int16_t *out, int frames)
{
    if (!f)
        return 0;
    if (f->channels == 1)
        return stb_vorbis_get_samples_short_interleaved(f->v, 1, out, frames);
    /* stereo: decoded in two halves of `out`, averaged */
    int done = 0;
    while (done < frames) {
        int16_t st[2 * 128];
        int want = frames - done < 128 ? frames - done : 128;
        int n = stb_vorbis_get_samples_short_interleaved(f->v, 2, st, want * 2);
        if (n <= 0)
            break;
        for (int i = 0; i < n; i++)
            out[done + i] = (int16_t)((st[2 * i] + st[2 * i + 1]) >> 1);
        done += n;
    }
    return done;
}

void vv_ogg_rewind(vv_ogg_t *f)
{
    if (f)
        stb_vorbis_seek_start(f->v);
}
