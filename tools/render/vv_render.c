/*
 * vv_render: converts the files of VVVVVV's data.zip to the formats the
 * Game & Watch port reads in place from its pack (run by tools/vv_pack.py,
 * one file per call).
 *
 *   vv_render image <in.png> <out>
 *       PNG -> "GWIM": 8 bit image with a palette of RGBA colours (every
 *       picture of the game has at most 256 different RGBA values; the
 *       colour of the transparent pixels is kept, the game's collision test
 *       reads it). Layout (little endian):
 *         char magic[4] "GWIM", u16 width, u16 height, u16 colours, u16 0,
 *         u8 palette[colours][4] (r, g, b, a), u8 pixels[height][width]
 *
 *   vv_render music <in.ogg> <out> [rate] [adpcm|pcm]
 *   vv_render sound <in.wav> <out> [rate]
 *       Ogg Vorbis (the songs of vvvvvvmusic.vvv) or WAV (the sound
 *       effects) -> "GWMU" (src/port/vv_music.c), mono (the console has one
 *       speaker), resampled to `rate` (32000 by default: the port's output
 *       rate). Songs loop from their start (VVVVVV's tracks have no loop
 *       points), IMA ADPCM by default; sounds play once, 16 bit PCM.
 *
 * Deterministic: the same output on every computer (no FMA contraction:
 * built with -ffp-contract=off).
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO_FAILS
#include "stb_image.h"

#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"
#undef L
#undef C
#undef R

static void die(const char *msg, const char *arg)
{
    fprintf(stderr, "vv_render: %s%s%s\n", msg, arg ? ": " : "", arg ? arg : "");
    exit(1);
}

static uint8_t *read_file(const char *path, long *size)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        die("cannot open", path);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *p = malloc(n > 0 ? n : 1);
    if (!p || fread(p, 1, n, f) != (size_t)n)
        die("cannot read", path);
    fclose(f);
    *size = n;
    return p;
}

/* ---- output buffer ---- */
typedef struct { uint8_t *p; size_t n, cap; } buf_t;

static void put(buf_t *b, uint8_t v)
{
    if (b->n == b->cap) {
        b->cap = b->cap ? b->cap * 2 : 65536;
        b->p = realloc(b->p, b->cap);
        if (!b->p)
            die("out of memory", NULL);
    }
    b->p[b->n++] = v;
}
static void put16(buf_t *b, uint32_t x) { put(b, x & 0xFF); put(b, (x >> 8) & 0xFF); }
static void put32(buf_t *b, uint32_t x) { put16(b, x & 0xFFFF); put16(b, x >> 16); }

static void write_file(const char *path, const buf_t *b)
{
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(b->p, 1, b->n, f) != b->n || fclose(f) != 0)
        die("cannot write", path);
}

/* ---- images ---- */
static int do_image(const char *in, const char *out)
{
    long size;
    uint8_t *file = read_file(in, &size);
    int w, h, comp;
    uint8_t *px = stbi_load_from_memory(file, (int)size, &w, &h, &comp, 4);
    if (!px)
        die("not a PNG image", in);
    if (w > 65535 || h > 65535)
        die("image too big", in);
    uint32_t pal[256];
    int npal = 0;
    uint8_t *idx = malloc((size_t)w * h);
    for (long i = 0; i < (long)w * h; i++) {
        const uint8_t *p = px + i * 4;
        uint32_t c = p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
        int k;
        for (k = 0; k < npal; k++)
            if (pal[k] == c)
                break;
        if (k == npal) {
            if (npal == 256)
                die("more than 256 colours", in);
            pal[npal++] = c;
        }
        idx[i] = (uint8_t)k;
    }
    buf_t b = { 0 };
    put(&b, 'G'); put(&b, 'W'); put(&b, 'I'); put(&b, 'M');
    put16(&b, w);
    put16(&b, h);
    put16(&b, npal);
    put16(&b, 0);
    for (int k = 0; k < npal; k++)
        put32(&b, pal[k]);
    for (long i = 0; i < (long)w * h; i++)
        put(&b, idx[i]);
    write_file(out, &b);
    printf("%s: %dx%d, %d colours\n", in, w, h, npal);
    return 0;
}

/* ---- IMA ADPCM (vv_music.c) ---- */
static const int16_t ima_step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};
static const int8_t ima_index[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
#define ADPCM_BLOCK 1024

static void adpcm_encode(const int16_t *s, long n, buf_t *out)
{
    for (long b = 0; b < n; b += ADPCM_BLOCK) {
        long len = n - b < ADPCM_BLOCK ? n - b : ADPCM_BLOCK;
        const int16_t *blk = s + b;
        int pred = blk[0];
        int idx = 0;
        if (len > 1) {
            int d = blk[1] - blk[0];
            if (d < 0) d = -d;
            while (idx < 88 && ima_step[idx] < d) idx++;
        }
        put(out, (uint8_t)(pred & 0xFF));
        put(out, (uint8_t)((pred >> 8) & 0xFF));
        put(out, (uint8_t)idx);
        put(out, 0);
        uint8_t byte = 0;
        for (long i = 0; i < len; i++) {
            int step = ima_step[idx];
            int diff = blk[i] - pred;
            int code = 0;
            if (diff < 0) { code = 8; diff = -diff; }
            int dq = step >> 3;
            if (diff >= step) { code |= 4; diff -= step; dq += step; }
            step >>= 1;
            if (diff >= step) { code |= 2; diff -= step; dq += step; }
            step >>= 1;
            if (diff >= step) { code |= 1; dq += step; }
            pred = (code & 8) ? pred - dq : pred + dq;
            if (pred > 32767) pred = 32767;
            if (pred < -32768) pred = -32768;
            idx += ima_index[code & 7];
            if (idx < 0) idx = 0;
            if (idx > 88) idx = 88;
            if (i & 1)
                put(out, (uint8_t)(byte | (code << 4)));
            else
                byte = (uint8_t)code;
        }
        if (len & 1)
            put(out, byte);
    }
}

/* ---- resampling: windowed sinc (Blackman), cut-off at 0.45 x the lower
 * rate, 32 zero crossings ---- */
#define RS_ZEROS 32

static long gcd(long a, long b)
{
    while (b) {
        long t = a % b;
        a = b;
        b = t;
    }
    return a;
}

/* polyphase: output sample j sits at input position j * M / L; the filter
 * of each of the L phases is computed once */
static float *resample(const float *in, long n, int in_rate, int out_rate, long *out_n)
{
    if (in_rate == out_rate) {
        float *o = malloc((n > 0 ? n : 1) * sizeof(float));
        memcpy(o, in, n * sizeof(float));
        *out_n = n;
        return o;
    }
    const double pi = 3.14159265358979323846;
    long g = gcd(in_rate, out_rate);
    long L = out_rate / g, M = in_rate / g;
    double ratio = (double)out_rate / in_rate;
    double fc = 0.45 * (ratio < 1 ? ratio : 1.0); /* cut-off, cycles per input sample */
    double half = RS_ZEROS / (2 * fc);            /* half width, input samples */
    int taps = (int)(2 * half) + 2;
    long *k0 = malloc(L * sizeof(long));
    int *cnt = malloc(L * sizeof(int));
    double *coef = malloc((size_t)L * taps * sizeof(double));
    for (long ph = 0; ph < L; ph++) {
        double frac = (double)ph / L;
        long a = (long)ceil(-frac - half), b = (long)floor(-frac + half);
        k0[ph] = a;
        cnt[ph] = (int)(b - a + 1);
        double wsum = 0;
        for (long k = a; k <= b; k++) {
            double x = k - frac;
            double s = x == 0 ? 2 * fc : sin(2 * pi * fc * x) / (pi * x);
            double u = (x + half) / (2 * half);
            double w = 0.42 - 0.5 * cos(2 * pi * u) + 0.08 * cos(4 * pi * u);
            coef[ph * taps + (k - a)] = s * w;
            wsum += s * w;
        }
        for (int k = 0; k < cnt[ph]; k++)
            coef[ph * taps + k] /= wsum;
    }
    long m = (long)((double)n * out_rate / in_rate);
    float *o = malloc((m > 0 ? m : 1) * sizeof(float));
    for (long j = 0; j < m; j++) {
        long base = (long)((int64_t)j * M / L);
        long ph = (long)((int64_t)j * M % L);
        const double *c = coef + ph * taps;
        long i = base + k0[ph];
        double acc = 0;
        for (int k = 0; k < cnt[ph]; k++, i++)
            if (i >= 0 && i < n)
                acc += c[k] * in[i];
        o[j] = (float)acc;
    }
    free(k0);
    free(cnt);
    free(coef);
    *out_n = m;
    return o;
}

static int16_t *to_pcm16(const float *s, long n, double gain)
{
    int16_t *o = malloc((n > 0 ? n : 1) * sizeof(int16_t));
    for (long i = 0; i < n; i++) {
        long v = lround(s[i] * gain * 32767.0);
        o[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
    }
    return o;
}

static void write_mus(const char *out, const int16_t *s, long n, int rate, int adpcm, int ends)
{
    buf_t b = { 0 };
    put(&b, 'G'); put(&b, 'W'); put(&b, 'M'); put(&b, 'U');
    put16(&b, 2);
    put16(&b, (adpcm ? 1 : 0) | (ends ? 0x8000 : 0));
    put32(&b, rate);
    put32(&b, (uint32_t)n);
    put32(&b, 0); /* loop start */
    if (adpcm) {
        adpcm_encode(s, n, &b);
    } else {
        for (long i = 0; i < n; i++)
            put16(&b, (uint16_t)s[i]);
    }
    while (b.n & 3)
        put(&b, 0);
    write_file(out, &b);
}

static int do_music(const char *in, const char *out, int rate, int adpcm)
{
    long size;
    uint8_t *file = read_file(in, &size);
    int channels, in_rate;
    short *dec;
    int frames = stb_vorbis_decode_memory(file, (int)size, &channels, &in_rate, &dec);
    if (frames <= 0)
        die("not an Ogg Vorbis file", in);
    float *mono = malloc((size_t)frames * sizeof(float));
    for (long i = 0; i < frames; i++) {
        double acc = 0;
        for (int c = 0; c < channels; c++)
            acc += dec[i * channels + c];
        mono[i] = (float)(acc / channels / 32768.0);
    }
    long n;
    float *rs = resample(mono, frames, in_rate, rate, &n);
    int16_t *pcm = to_pcm16(rs, n, 1.0);
    write_mus(out, pcm, n, rate, adpcm, 0);
    printf("%s: %d Hz %d ch, %.1f s -> %d Hz mono %s\n", in, in_rate, channels,
           (double)frames / in_rate, rate, adpcm ? "IMA ADPCM" : "PCM");
    return 0;
}

static uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static int do_sound(const char *in, const char *out, int rate)
{
    long size;
    uint8_t *f = read_file(in, &size);
    if (size < 12 || memcmp(f, "RIFF", 4) || memcmp(f + 8, "WAVE", 4))
        die("not a WAV file", in);
    int channels = 0, in_rate = 0, bits = 0;
    const uint8_t *data = NULL;
    uint32_t data_len = 0;
    long p = 12;
    while (p + 8 <= size) {
        uint32_t len = rd32(f + p + 4);
        if (!memcmp(f + p, "fmt ", 4) && len >= 16) {
            if (rd16(f + p + 8) != 1)
                die("not PCM", in);
            channels = rd16(f + p + 10);
            in_rate = (int)rd32(f + p + 12);
            bits = rd16(f + p + 22);
        } else if (!memcmp(f + p, "data", 4)) {
            data = f + p + 8;
            data_len = len;
            if (p + 8 + (long)data_len > size)
                data_len = (uint32_t)(size - p - 8);
        }
        p += 8 + len + (len & 1);
    }
    if (!data || channels < 1 || in_rate <= 0 || (bits != 8 && bits != 16))
        die("unsupported WAV file", in);
    long frames = data_len / (channels * bits / 8);
    float *mono = malloc((frames > 0 ? frames : 1) * sizeof(float));
    for (long i = 0; i < frames; i++) {
        double acc = 0;
        for (int c = 0; c < channels; c++) {
            if (bits == 16)
                acc += (int16_t)rd16(data + (i * channels + c) * 2) / 32768.0;
            else
                acc += (data[i * channels + c] - 128) / 128.0;
        }
        mono[i] = (float)(acc / channels);
    }
    long n;
    float *rs = resample(mono, frames, in_rate, rate, &n);
    int16_t *pcm = to_pcm16(rs, n, 1.0);
    write_mus(out, pcm, n, rate, 0, 1);
    printf("%s: %d Hz %d ch %d bit, %.2f s -> %d Hz mono PCM\n", in, in_rate, channels, bits,
           (double)frames / in_rate, rate);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc >= 4 && !strcmp(argv[1], "image"))
        return do_image(argv[2], argv[3]);
    if (argc >= 4 && !strcmp(argv[1], "music"))
        return do_music(argv[2], argv[3], argc > 4 ? atoi(argv[4]) : 32000,
                        argc > 5 ? strcmp(argv[5], "pcm") != 0 : 1);
    if (argc >= 4 && !strcmp(argv[1], "sound"))
        return do_sound(argv[2], argv[3], argc > 4 ? atoi(argv[4]) : 32000);
    fprintf(stderr,
            "usage: vv_render image <in.png> <out>\n"
            "       vv_render music <in.ogg> <out> [rate] [adpcm|pcm]\n"
            "       vv_render sound <in.wav> <out> [rate]\n");
    return 2;
}
