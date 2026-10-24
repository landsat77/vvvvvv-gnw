/* Pre-rendered music pieces (vv_music.c) */
#ifndef VV_MUSIC_H
#define VV_MUSIC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vv_mus vv_mus_t;

/* A "GWMU" piece in memory (read in place), NULL if invalid. */
vv_mus_t *vv_mus_open(const unsigned char *data, int len);
void vv_mus_close(vv_mus_t *f);
/* the piece plays once (it does not loop), its length at `out_rate` */
bool vv_mus_ends(const vv_mus_t *f);
uint32_t vv_mus_length(const vv_mus_t *f, int out_rate);
int vv_mus_rewind(vv_mus_t *f);
/* Adds the next `frames` frames at `out_rate` to `out` (interleaved,
 * `channels` channels) when `audible`. The piece loops forever (from its
 * loop start): returns `frames`. */
int vv_mus_mix(vv_mus_t *f, short *out, int frames, int channels, int audible, int out_rate);

#ifdef __cplusplus
}
#endif

#endif
