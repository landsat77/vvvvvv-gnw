/* The original Ogg Vorbis songs of VVVVVV, decoded on the console
 * (vv_ogg.c: stb_vorbis), mixed down to mono. */
#ifndef VV_OGG_H
#define VV_OGG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vv_ogg vv_ogg_t;

/* An Ogg Vorbis file in memory (read in place), NULL if invalid or out of
 * memory (~130 KB of heap while it plays). */
vv_ogg_t *vv_ogg_open(const uint8_t *data, uint32_t size);
void vv_ogg_close(vv_ogg_t *f);
int vv_ogg_rate(const vv_ogg_t *f);
/* Decodes up to `frames` mono samples into `out`: the number decoded, 0 at
 * the end of the song. */
int vv_ogg_read(vv_ogg_t *f, int16_t *out, int frames);
void vv_ogg_rewind(vv_ogg_t *f);

#ifdef __cplusplus
}
#endif

#endif
