/*
 * The small SDL 2 of the VVVVVV port (src/port/sdl/SDL.h).
 *
 * Video. VVVVVV 2.4 draws with SDL_Renderer: images are textures, and it
 * composes its frames in ten 320x240 (or 336x336) render target textures.
 * There is no room for those on the console (150 KB each in RGB565), so:
 *  - the renderer draws straight into the frame being built
 *    (vv_plat_draw_buffer(): the LCD back buffer, RGB565), in software;
 *  - the target textures the game composes the frame with (gameTexture,
 *    gameplayTexture, menuTexture) are "screen" textures: drawing to them
 *    draws to the frame, and copying one to another does nothing (they
 *    are the same pixels). The others are "sinks" (drawing to them does
 *    nothing): the code that filled them under GNW (Graphics.cpp) draws its
 *    tiles straight into the frame every frame instead (room tiles, tower
 *    and warp backgrounds), and the few effects that moved the whole frame
 *    (screen shake, flip mode, menus sliding over the game) are done on the
 *    frame by vv_sdl_screen_*();
 *  - images come from the asset pack ("GWIM": 8 bit with a palette of RGBA
 *    colours, read in place from the flash): a draw maps the palette once
 *    through the colour / alpha modulation of the texture (cached), then
 *    copies or blends the pixels (nearest scaling, flips; no rotation).
 *
 * Events: key presses made from the buttons of the console (vv_core.cpp
 * maps them), SDL_PollEvent / SDL_Delay give the platform its service.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "SDL.h"
#include "vv_platform.h"
#include "vv_sdl.h"

/* ------------------------------------------------------------------ */
/* stdinc                                                             */
/* ------------------------------------------------------------------ */
void *SDL_malloc(size_t size) { return vv_malloc(size ? size : 1); }
void *SDL_calloc(size_t n, size_t size) { return vv_calloc(n ? n : 1, size ? size : 1); }
void *SDL_realloc(void *mem, size_t size) { return vv_realloc(mem, size ? size : 1); }
void SDL_free(void *mem) { vv_free(mem); }

int SDL_tolower(int x) { return (x >= 'A' && x <= 'Z') ? x + 32 : x; }
int SDL_toupper(int x) { return (x >= 'a' && x <= 'z') ? x - 32 : x; }
int SDL_isdigit(int x) { return x >= '0' && x <= '9'; }
int SDL_isxdigit(int x) { return SDL_isdigit(x) || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'); }
int SDL_isspace(int x) { return x == ' ' || (x >= '\t' && x <= '\r'); }
int SDL_isalpha(int x) { return (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z'); }

int SDL_strncasecmp(const char *a, const char *b, size_t len)
{
    while (len--) {
        int ca = SDL_tolower((unsigned char)*a++), cb = SDL_tolower((unsigned char)*b++);
        if (ca != cb)
            return ca - cb;
        if (!ca)
            break;
    }
    return 0;
}

int SDL_strcasecmp(const char *a, const char *b)
{
    return SDL_strncasecmp(a, b, (size_t)-1);
}

char *SDL_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = (char *)SDL_malloc(n);
    if (d)
        memcpy(d, s, n);
    return d;
}

size_t SDL_strlcpy(char *dst, const char *src, size_t maxlen)
{
    size_t n = strlen(src);
    if (maxlen) {
        size_t c = n < maxlen - 1 ? n : maxlen - 1;
        memcpy(dst, src, c);
        dst[c] = '\0';
    }
    return n;
}

size_t SDL_strlcat(char *dst, const char *src, size_t maxlen)
{
    size_t d = strnlen(dst, maxlen);
    if (d == maxlen)
        return d + strlen(src);
    return d + SDL_strlcpy(dst + d, src, maxlen - d);
}

char *SDL_getenv(const char *name) { (void)name; return NULL; }

void SDL_qsort(void *base, size_t nmemb, size_t size, int (*compare)(const void *, const void *))
{
    qsort(base, nmemb, size, compare);
}

/* ------------------------------------------------------------------ */
/* errors, log, hints, init                                           */
/* ------------------------------------------------------------------ */
static char error_text[96];

const char *SDL_GetError(void) { return error_text; }
int SDL_SetError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(error_text, sizeof(error_text), fmt, ap);
    va_end(ap);
    return -1;
}
void SDL_ClearError(void) { error_text[0] = '\0'; }

static SDL_LogPriority log_priority = SDL_LOG_PRIORITY_INFO;
void SDL_LogSetPriority(int category, SDL_LogPriority priority) { (void)category; log_priority = priority; }
void SDL_LogMessageV(int category, SDL_LogPriority priority, const char *fmt, va_list ap)
{
    (void)category;
    if (priority < log_priority)
        return;
    char line[160];
    vsnprintf(line, sizeof(line), fmt, ap);
    printf("vv: %s\n", line);
}
void SDL_Log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SDL_LogMessageV(0, SDL_LOG_PRIORITY_INFO, fmt, ap);
    va_end(ap);
}

int SDL_Init(Uint32 flags) { (void)flags; return 0; }
int SDL_InitSubSystem(Uint32 flags) { (void)flags; return 0; }
void SDL_Quit(void) {}
SDL_bool SDL_SetHintWithPriority(const char *n, const char *v, SDL_HintPriority p) { (void)n; (void)v; (void)p; return SDL_TRUE; }
SDL_bool SDL_SetHint(const char *n, const char *v) { (void)n; (void)v; return SDL_TRUE; }
SDL_bool SDL_GetHintBoolean(const char *n, SDL_bool d) { (void)n; return d; }
const char *SDL_GetHint(const char *n) { (void)n; return NULL; }
const char *SDL_GetPlatform(void) { return "Game & Watch"; }
void SDL_GetVersion(SDL_version *ver) { SDL_VERSION(ver); }
int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window)
{
    (void)flags; (void)window;
    printf("vv: %s: %s\n", title, message);
    return 0;
}

/* ------------------------------------------------------------------ */
/* rects, pixels                                                      */
/* ------------------------------------------------------------------ */
SDL_bool SDL_IntersectRect(const SDL_Rect *A, const SDL_Rect *B, SDL_Rect *r)
{
    int x0 = SDL_max(A->x, B->x), y0 = SDL_max(A->y, B->y);
    int x1 = SDL_min(A->x + A->w, B->x + B->w), y1 = SDL_min(A->y + A->h, B->y + B->h);
    r->x = x0;
    r->y = y0;
    r->w = x1 - x0;
    r->h = y1 - y0;
    if (r->w <= 0 || r->h <= 0) {
        r->w = r->h = 0;
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

SDL_bool SDL_HasIntersection(const SDL_Rect *A, const SDL_Rect *B)
{
    SDL_Rect r;
    if (SDL_RectEmpty(A) || SDL_RectEmpty(B))
        return SDL_FALSE;
    return SDL_IntersectRect(A, B, &r);
}

static SDL_PixelFormat format_argb8888 = {
    SDL_PIXELFORMAT_ARGB8888, NULL, 32, 4, {0, 0},
    0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0xFF000000u,
    0, 0, 0, 0, 16, 8, 0, 24, 1, NULL
};

Uint32 SDL_MapRGBA(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (f && f->BytesPerPixel == 1 && f->palette) {
        /* nearest colour of the palette */
        int best = 0, bestd = 1 << 30;
        for (int i = 0; i < f->palette->ncolors; i++) {
            const SDL_Color *c = &f->palette->colors[i];
            int d = (c->r - r) * (c->r - r) + (c->g - g) * (c->g - g) + (c->b - b) * (c->b - b) + (c->a - a) * (c->a - a);
            if (d < bestd) {
                bestd = d;
                best = i;
            }
        }
        return (Uint32)best;
    }
    return ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b)
{
    return SDL_MapRGBA(f, r, g, b, 255);
}

void SDL_GetRGBA(Uint32 pixel, const SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a)
{
    if (f && f->BytesPerPixel == 1 && f->palette) {
        SDL_Color c = { 0, 0, 0, 0 };
        if ((int)pixel < f->palette->ncolors)
            c = f->palette->colors[pixel];
        *r = c.r;
        *g = c.g;
        *b = c.b;
        *a = c.a;
        return;
    }
    *r = (Uint8)(pixel >> 16);
    *g = (Uint8)(pixel >> 8);
    *b = (Uint8)pixel;
    *a = (Uint8)(pixel >> 24);
}

void SDL_GetRGB(Uint32 pixel, const SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b)
{
    Uint8 a;
    SDL_GetRGBA(pixel, f, r, g, b, &a);
}

/* ------------------------------------------------------------------ */
/* images of the pack ("GWIM", vv_render.c)                           */
/* ------------------------------------------------------------------ */
typedef struct {
    int w, h, ncolors;
    const uint8_t *palette; /* ncolors x RGBA */
    const uint8_t *pixels;  /* h x w indexes */
} gwim_t;

static bool gwim_parse(const uint8_t *data, uint32_t size, gwim_t *im)
{
    if (!data || size < 12 || memcmp(data, "GWIM", 4) != 0)
        return false;
    im->w = data[4] | (data[5] << 8);
    im->h = data[6] | (data[7] << 8);
    im->ncolors = data[8] | (data[9] << 8);
    if (im->ncolors < 1 || im->ncolors > 256 || 12u + im->ncolors * 4u + (uint32_t)im->w * im->h > size)
        return false;
    im->palette = data + 12;
    im->pixels = data + 12 + im->ncolors * 4;
    return true;
}

/* An 8 bit surface over a pack image (read only, no copy) */
SDL_Surface *vv_sdl_image_surface(const uint8_t *data, uint32_t size)
{
    gwim_t im;
    if (!gwim_parse(data, size, &im))
        return NULL;
    struct {
        SDL_Surface s;
        SDL_PixelFormat f;
        SDL_Palette p;
    } *m = SDL_calloc(1, sizeof(*m));
    SDL_Color *colors = SDL_malloc(sizeof(SDL_Color) * im.ncolors);
    if (!m || !colors) {
        SDL_free(m);
        SDL_free(colors);
        return NULL;
    }
    for (int i = 0; i < im.ncolors; i++) {
        colors[i].r = im.palette[i * 4 + 0];
        colors[i].g = im.palette[i * 4 + 1];
        colors[i].b = im.palette[i * 4 + 2];
        colors[i].a = im.palette[i * 4 + 3];
    }
    m->p.ncolors = im.ncolors;
    m->p.colors = colors;
    m->p.refcount = 1;
    m->f.format = SDL_PIXELFORMAT_INDEX8;
    m->f.palette = &m->p;
    m->f.BitsPerPixel = 8;
    m->f.BytesPerPixel = 1;
    m->f.refcount = 1;
    m->s.flags = SDL_PREALLOC;
    m->s.format = &m->f;
    m->s.w = im.w;
    m->s.h = im.h;
    m->s.pitch = im.w;
    m->s.pixels = (void *)im.pixels;
    m->s.clip_rect.w = im.w;
    m->s.clip_rect.h = im.h;
    m->s.refcount = 1;
    m->s.userdata = colors;
    return &m->s;
}

/* ------------------------------------------------------------------ */
/* surfaces (ARGB8888 in the heap, or the 8 bit views above)          */
/* ------------------------------------------------------------------ */
SDL_Surface *SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format)
{
    (void)flags; (void)depth; (void)format;
    if (width <= 0 || height <= 0)
        return NULL;
    SDL_Surface *s = SDL_calloc(1, sizeof(SDL_Surface));
    if (!s)
        return NULL;
    s->pixels = SDL_calloc((size_t)width * height, 4);
    if (!s->pixels) {
        SDL_free(s);
        SDL_SetError("out of memory");
        return NULL;
    }
    s->format = &format_argb8888;
    s->w = width;
    s->h = height;
    s->pitch = width * 4;
    s->clip_rect.w = width;
    s->clip_rect.h = height;
    s->refcount = 1;
    return s;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    (void)Rmask; (void)Gmask; (void)Bmask; (void)Amask;
    return SDL_CreateRGBSurfaceWithFormat(flags, width, height, depth, SDL_PIXELFORMAT_ARGB8888);
}

SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 format)
{
    (void)pixels; (void)width; (void)height; (void)depth; (void)pitch; (void)format;
    SDL_SetError("SDL_CreateRGBSurfaceWithFormatFrom: not on the console");
    return NULL;
}

void SDL_FreeSurface(SDL_Surface *s)
{
    if (!s)
        return;
    if (s->flags & SDL_PREALLOC) {
        SDL_free(s->userdata); /* palette colours of an image view */
        SDL_free(s);
        return;
    }
    SDL_free(s->pixels);
    SDL_free(s);
}

int SDL_LockSurface(SDL_Surface *s) { (void)s; return 0; }
void SDL_UnlockSurface(SDL_Surface *s) { (void)s; }
int SDL_SetSurfaceBlendMode(SDL_Surface *s, SDL_BlendMode m) { (void)s; (void)m; return 0; }
int SDL_GetSurfaceBlendMode(SDL_Surface *s, SDL_BlendMode *m) { (void)s; if (m) *m = SDL_BLENDMODE_BLEND; return 0; }

static Uint32 surface_get(const SDL_Surface *s, int x, int y)
{
    const uint8_t *row = (const uint8_t *)s->pixels + y * s->pitch;
    if (s->format->BytesPerPixel == 1) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(row[x], s->format, &r, &g, &b, &a);
        return ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
    }
    return ((const Uint32 *)row)[x];
}

int SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    /* plain copy (VVVVVV copies parts of images into new surfaces) */
    if (!src || !dst || dst->format->BytesPerPixel != 4)
        return -1;
    SDL_Rect sr = srcrect ? *srcrect : (SDL_Rect){ 0, 0, src->w, src->h };
    int dx = dstrect ? dstrect->x : 0, dy = dstrect ? dstrect->y : 0;
    for (int y = 0; y < sr.h; y++) {
        int sy = sr.y + y, ty = dy + y;
        if (sy < 0 || sy >= src->h || ty < 0 || ty >= dst->h)
            continue;
        for (int x = 0; x < sr.w; x++) {
            int sx = sr.x + x, tx = dx + x;
            if (sx < 0 || sx >= src->w || tx < 0 || tx >= dst->w)
                continue;
            ((Uint32 *)((uint8_t *)dst->pixels + ty * dst->pitch))[tx] = surface_get(src, sx, sy);
        }
    }
    return 0;
}

int SDL_BlitScaled(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    return SDL_BlitSurface(src, srcrect, dst, dstrect);
}

SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *src, Uint32 pixel_format, Uint32 flags)
{
    (void)pixel_format; (void)flags;
    if (!src)
        return NULL;
    SDL_Surface *d = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (d)
        SDL_BlitSurface(src, NULL, d, NULL);
    return d;
}

int SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color)
{
    if (!dst || dst->format->BytesPerPixel != 4)
        return -1;
    SDL_Rect r = rect ? *rect : (SDL_Rect){ 0, 0, dst->w, dst->h };
    for (int y = SDL_max(r.y, 0); y < SDL_min(r.y + r.h, dst->h); y++)
        for (int x = SDL_max(r.x, 0); x < SDL_min(r.x + r.w, dst->w); x++)
            ((Uint32 *)((uint8_t *)dst->pixels + y * dst->pitch))[x] = color;
    return 0;
}

/* ------------------------------------------------------------------ */
/* window                                                             */
/* ------------------------------------------------------------------ */
struct SDL_Window { int dummy; };
static SDL_Window the_window;

SDL_Window *SDL_CreateWindow(const char *t, int x, int y, int w, int h, Uint32 f) { (void)t; (void)x; (void)y; (void)w; (void)h; (void)f; return &the_window; }
void SDL_DestroyWindow(SDL_Window *w) { (void)w; }
Uint32 SDL_GetWindowFlags(SDL_Window *w) { (void)w; return SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS; }
SDL_Window *SDL_GetWindowFromID(Uint32 id) { (void)id; return &the_window; }
int SDL_SetWindowFullscreen(SDL_Window *w, Uint32 f) { (void)w; (void)f; return 0; }
void SDL_SetWindowSize(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }
void SDL_GetWindowSize(SDL_Window *w, int *x, int *y) { (void)w; if (x) *x = VV_WIDTH; if (y) *y = VV_HEIGHT; }
void SDL_SetWindowPosition(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }
void SDL_SetWindowMinimumSize(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }
void SDL_SetWindowTitle(SDL_Window *w, const char *t) { (void)w; (void)t; }
void SDL_SetWindowIcon(SDL_Window *w, SDL_Surface *i) { (void)w; (void)i; }
void SDL_ShowWindow(SDL_Window *w) { (void)w; }
void SDL_MinimizeWindow(SDL_Window *w) { (void)w; }
int SDL_GetWindowDisplayIndex(SDL_Window *w) { (void)w; return 0; }
int SDL_GetCurrentDisplayMode(int i, SDL_DisplayMode *m) { (void)i; memset(m, 0, sizeof(*m)); m->w = VV_WIDTH; m->h = VV_HEIGHT; m->refresh_rate = 60; return 0; }
int SDL_GetDesktopDisplayMode(int i, SDL_DisplayMode *m) { return SDL_GetCurrentDisplayMode(i, m); }
const char *SDL_GetCurrentVideoDriver(void) { return "gnw"; }
void SDL_EnableScreenSaver(void) {}
void SDL_DisableScreenSaver(void) {}

/* ------------------------------------------------------------------ */
/* renderer                                                           */
/* ------------------------------------------------------------------ */
enum { TEX_IMAGE, TEX_SCREEN, TEX_SINK };

struct SDL_Texture {
    int kind;
    int w, h;
    gwim_t im;
    int load;              /* VV_SDL_LOAD_* (colour of the image) */
    Uint8 r, g, b, a;      /* modulation */
    SDL_BlendMode blend;
    /* the palette through the modulation, for the last (r, g, b, a) used */
    bool cache_valid;
    Uint8 cr, cg, cb, ca;
    uint16_t pal565[256];
    uint8_t pala[256];
};

struct SDL_Renderer {
    SDL_Texture *target;
    Uint8 r, g, b, a;
    SDL_BlendMode blend;
};
static SDL_Renderer the_renderer;

/* translation of the drawing (menus sliding over the game) */
static int draw_dx, draw_dy;

static uint16_t *screen(void)
{
    return vv_plat_draw_buffer();
}

/* where the draws go: true = the frame, false = nowhere */
static bool target_is_screen(void)
{
    return the_renderer.target == NULL || the_renderer.target->kind == TEX_SCREEN;
}

static inline uint16_t rgb565(int r, int g, int b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/* d = d * (255 - a) + c * a, per channel (565) */
static inline uint16_t blend565(uint16_t d, uint16_t c, unsigned a)
{
    unsigned ia = 255 - a;
    unsigned dr = d >> 11, dg = (d >> 5) & 63, db = d & 31;
    unsigned cr = c >> 11, cg = (c >> 5) & 63, cb = c & 31;
    unsigned r = (cr * a + dr * ia + 127) / 255;
    unsigned g = (cg * a + dg * ia + 127) / 255;
    unsigned b = (cb * a + db * ia + 127) / 255;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

SDL_Renderer *SDL_CreateRenderer(SDL_Window *w, int i, Uint32 f)
{
    (void)w; (void)i; (void)f;
    memset(&the_renderer, 0, sizeof(the_renderer));
    the_renderer.a = 255;
    return &the_renderer;
}
void SDL_DestroyRenderer(SDL_Renderer *r) { (void)r; }
int SDL_GetRendererInfo(SDL_Renderer *r, SDL_RendererInfo *info)
{
    (void)r;
    memset(info, 0, sizeof(*info));
    info->name = "gnw";
    info->flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;
    info->max_texture_width = info->max_texture_height = 4096;
    return 0;
}
int SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h) { (void)r; (void)w; (void)h; return 0; }
int SDL_RenderSetIntegerScale(SDL_Renderer *r, SDL_bool e) { (void)r; (void)e; return 0; }
int SDL_RenderSetVSync(SDL_Renderer *r, int v) { (void)r; (void)v; return 0; }
int SDL_GetRendererOutputSize(SDL_Renderer *r, int *w, int *h) { (void)r; if (w) *w = VV_WIDTH; if (h) *h = VV_HEIGHT; return 0; }

static SDL_Texture *texture_new(int kind, int w, int h)
{
    SDL_Texture *t = SDL_calloc(1, sizeof(SDL_Texture));
    if (!t)
        return NULL;
    t->kind = kind;
    t->w = w;
    t->h = h;
    t->r = t->g = t->b = t->a = 255;
    t->blend = kind == TEX_IMAGE ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE;
    return t;
}

SDL_Texture *SDL_CreateTexture(SDL_Renderer *r, Uint32 format, int access, int w, int h)
{
    (void)r; (void)format; (void)access;
    /* target textures: sinks until vv_sdl_texture_is_screen() */
    SDL_Texture *t = texture_new(TEX_SINK, w, h);
    if (!t)
        SDL_SetError("out of memory");
    return t;
}

SDL_Texture *vv_sdl_image_texture(const uint8_t *data, uint32_t size, int load)
{
    gwim_t im;
    if (!gwim_parse(data, size, &im)) {
        SDL_SetError("not a GWIM image");
        return NULL;
    }
    SDL_Texture *t = texture_new(TEX_IMAGE, im.w, im.h);
    if (!t)
        return NULL;
    t->im = im;
    t->load = load;
    return t;
}

SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *r, SDL_Surface *s)
{
    (void)r; (void)s;
    SDL_SetError("SDL_CreateTextureFromSurface: not on the console");
    return NULL;
}

void vv_sdl_texture_is_screen(SDL_Texture *t)
{
    if (t)
        t->kind = TEX_SCREEN;
}

void SDL_DestroyTexture(SDL_Texture *t)
{
    if (the_renderer.target == t)
        the_renderer.target = NULL;
    SDL_free(t);
}

int SDL_QueryTexture(SDL_Texture *t, Uint32 *format, int *access, int *w, int *h)
{
    if (!t)
        return SDL_SetError("no texture");
    if (format)
        *format = SDL_PIXELFORMAT_ARGB8888;
    if (access)
        *access = t->kind == TEX_IMAGE ? SDL_TEXTUREACCESS_STATIC : SDL_TEXTUREACCESS_TARGET;
    if (w)
        *w = t->w;
    if (h)
        *h = t->h;
    return 0;
}

int SDL_SetTextureColorMod(SDL_Texture *t, Uint8 r, Uint8 g, Uint8 b)
{
    if (!t)
        return SDL_SetError("no texture");
    t->r = r;
    t->g = g;
    t->b = b;
    return 0;
}
int SDL_GetTextureColorMod(SDL_Texture *t, Uint8 *r, Uint8 *g, Uint8 *b)
{
    if (!t)
        return SDL_SetError("no texture");
    *r = t->r;
    *g = t->g;
    *b = t->b;
    return 0;
}
int SDL_SetTextureAlphaMod(SDL_Texture *t, Uint8 a)
{
    if (!t)
        return SDL_SetError("no texture");
    t->a = a;
    return 0;
}
int SDL_GetTextureAlphaMod(SDL_Texture *t, Uint8 *a)
{
    if (!t)
        return SDL_SetError("no texture");
    *a = t->a;
    return 0;
}
int SDL_SetTextureBlendMode(SDL_Texture *t, SDL_BlendMode m)
{
    if (!t)
        return SDL_SetError("no texture");
    t->blend = m;
    return 0;
}
int SDL_GetTextureBlendMode(SDL_Texture *t, SDL_BlendMode *m)
{
    if (!t)
        return SDL_SetError("no texture");
    *m = t->blend;
    return 0;
}
int SDL_SetTextureScaleMode(SDL_Texture *t, SDL_ScaleMode m) { (void)t; (void)m; return 0; }
int SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *r, const void *p, int pitch) { (void)t; (void)r; (void)p; (void)pitch; return 0; }

int SDL_SetRenderTarget(SDL_Renderer *r, SDL_Texture *t)
{
    (void)r;
    the_renderer.target = t;
    return 0;
}
SDL_Texture *SDL_GetRenderTarget(SDL_Renderer *r) { (void)r; return the_renderer.target; }

int SDL_SetRenderDrawColor(SDL_Renderer *r, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    (void)r;
    the_renderer.r = cr;
    the_renderer.g = cg;
    the_renderer.b = cb;
    the_renderer.a = ca;
    return 0;
}
int SDL_GetRenderDrawColor(SDL_Renderer *r, Uint8 *cr, Uint8 *cg, Uint8 *cb, Uint8 *ca)
{
    (void)r;
    if (cr) *cr = the_renderer.r;
    if (cg) *cg = the_renderer.g;
    if (cb) *cb = the_renderer.b;
    if (ca) *ca = the_renderer.a;
    return 0;
}
int SDL_SetRenderDrawBlendMode(SDL_Renderer *r, SDL_BlendMode m) { (void)r; the_renderer.blend = m; return 0; }
int SDL_GetRenderDrawBlendMode(SDL_Renderer *r, SDL_BlendMode *m) { (void)r; *m = the_renderer.blend; return 0; }

/* the area of the frame a draw may touch: the screen, moved by the
 * translation (what slides off the screen is not drawn) */
static void clip_bounds(int *x0, int *y0, int *x1, int *y1)
{
    *x0 = SDL_max(0, draw_dx);
    *y0 = SDL_max(0, draw_dy);
    *x1 = SDL_min(VV_WIDTH, VV_WIDTH + draw_dx);
    *y1 = SDL_min(VV_HEIGHT, VV_HEIGHT + draw_dy);
}

static void fill(int x, int y, int w, int h, bool clear)
{
    if (!target_is_screen())
        return;
    x += draw_dx;
    y += draw_dy;
    int cx0, cy0, cx1, cy1;
    clip_bounds(&cx0, &cy0, &cx1, &cy1);
    int x0 = SDL_max(x, cx0), y0 = SDL_max(y, cy0);
    int x1 = SDL_min(x + w, cx1), y1 = SDL_min(y + h, cy1);
    if (x0 >= x1 || y0 >= y1)
        return;
    uint16_t c = rgb565(the_renderer.r, the_renderer.g, the_renderer.b);
    unsigned a = the_renderer.a;
    uint16_t *fb = screen();
    if (clear || the_renderer.blend == SDL_BLENDMODE_NONE || a == 255) {
        uint32_t c2 = c | ((uint32_t)c << 16);
        for (int yy = y0; yy < y1; yy++) {
            uint16_t *p = fb + yy * VV_WIDTH + x0;
            int n = x1 - x0;
            if (((uintptr_t)p & 2) && n) {
                *p++ = c;
                n--;
            }
            uint32_t *q = (uint32_t *)p;
            for (; n >= 2; n -= 2)
                *q++ = c2;
            if (n)
                *(uint16_t *)q = c;
        }
    } else if (a) {
        for (int yy = y0; yy < y1; yy++) {
            uint16_t *p = fb + yy * VV_WIDTH;
            for (int xx = x0; xx < x1; xx++)
                p[xx] = blend565(p[xx], c, a);
        }
    }
}

int SDL_RenderClear(SDL_Renderer *r)
{
    (void)r;
    fill(-draw_dx, -draw_dy, VV_WIDTH, VV_HEIGHT, true);
    return 0;
}

int SDL_RenderFillRect(SDL_Renderer *r, const SDL_Rect *rect)
{
    (void)r;
    if (rect)
        fill(rect->x, rect->y, rect->w, rect->h, false);
    else
        fill(-draw_dx, -draw_dy, VV_WIDTH, VV_HEIGHT, false);
    return 0;
}

static void plot(int x, int y)
{
    x += draw_dx;
    y += draw_dy;
    int cx0, cy0, cx1, cy1;
    clip_bounds(&cx0, &cy0, &cx1, &cy1);
    if (x < cx0 || y < cy0 || x >= cx1 || y >= cy1)
        return;
    uint16_t c = rgb565(the_renderer.r, the_renderer.g, the_renderer.b);
    uint16_t *p = screen() + y * VV_WIDTH + x;
    if (the_renderer.blend == SDL_BLENDMODE_NONE || the_renderer.a == 255)
        *p = c;
    else if (the_renderer.a)
        *p = blend565(*p, c, the_renderer.a);
}

int SDL_RenderDrawPoint(SDL_Renderer *r, int x, int y)
{
    (void)r;
    if (target_is_screen())
        plot(x, y);
    return 0;
}

int SDL_RenderDrawPoints(SDL_Renderer *r, const SDL_Point *points, int count)
{
    (void)r;
    if (target_is_screen())
        for (int i = 0; i < count; i++)
            plot(points[i].x, points[i].y);
    return 0;
}

int SDL_RenderDrawLine(SDL_Renderer *r, int x1, int y1, int x2, int y2)
{
    (void)r;
    if (!target_is_screen())
        return 0;
    if (y1 == y2) {
        fill(SDL_min(x1, x2), y1, SDL_abs(x2 - x1) + 1, 1, false);
        return 0;
    }
    if (x1 == x2) {
        fill(x1, SDL_min(y1, y2), 1, SDL_abs(y2 - y1) + 1, false);
        return 0;
    }
    /* Bresenham */
    int dx = SDL_abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -SDL_abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        plot(x1, y1);
        if (x1 == x2 && y1 == y2)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
    return 0;
}

int SDL_RenderDrawRect(SDL_Renderer *r, const SDL_Rect *rect)
{
    SDL_Rect full = { -draw_dx, -draw_dy, VV_WIDTH, VV_HEIGHT };
    if (!rect)
        rect = &full;
    if (rect->w <= 0 || rect->h <= 0)
        return 0;
    (void)r;
    fill(rect->x, rect->y, rect->w, 1, false);
    if (rect->h > 1)
        fill(rect->x, rect->y + rect->h - 1, rect->w, 1, false);
    if (rect->h > 2) {
        fill(rect->x, rect->y + 1, 1, rect->h - 2, false);
        if (rect->w > 1)
            fill(rect->x + rect->w - 1, rect->y + 1, 1, rect->h - 2, false);
    }
    return 0;
}

/* the palette of an image through the modulation of its texture */
static void texture_palette(SDL_Texture *t)
{
    if (t->cache_valid && t->cr == t->r && t->cg == t->g && t->cb == t->b && t->ca == t->a)
        return;
    const uint8_t *p = t->im.palette;
    for (int i = 0; i < t->im.ncolors; i++, p += 4) {
        unsigned r = p[0], g = p[1], b = p[2];
        if (t->load == VV_SDL_LOAD_WHITE) {
            r = g = b = 255;
        } else if (t->load == VV_SDL_LOAD_GRAYSCALE) {
            /* GraphicsResources.cpp: each channel truncated, then rounded */
            unsigned gr = (unsigned)(uint8_t)(r * 0.299) + (unsigned)(uint8_t)(g * 0.587) + (unsigned)(uint8_t)(b * 0.114);
            r = g = b = gr > 255 ? 255 : gr;
        }
        r = r * t->r / 255;
        g = g * t->g / 255;
        b = b * t->b / 255;
        t->pal565[i] = rgb565(r, g, b);
        t->pala[i] = (uint8_t)(p[3] * t->a / 255);
    }
    t->cr = t->r;
    t->cg = t->g;
    t->cb = t->b;
    t->ca = t->a;
    t->cache_valid = true;
}

static int copy_image(SDL_Texture *t, const SDL_Rect *srcrect, const SDL_Rect *dstrect, SDL_RendererFlip flip)
{
    if (!target_is_screen())
        return 0;
    SDL_Rect s = srcrect ? *srcrect : (SDL_Rect){ 0, 0, t->w, t->h };
    SDL_Rect d = dstrect ? *dstrect : (SDL_Rect){ 0, 0, VV_WIDTH, VV_HEIGHT };
    /* the source inside the image */
    if (s.x < 0) { if (d.w == s.w) { d.x -= s.x; d.w += s.x; } s.w += s.x; s.x = 0; }
    if (s.y < 0) { if (d.h == s.h) { d.y -= s.y; d.h += s.y; } s.h += s.y; s.y = 0; }
    if (s.x + s.w > t->w) { int c = s.x + s.w - t->w; if (d.w == s.w) d.w -= c; s.w -= c; }
    if (s.y + s.h > t->h) { int c = s.y + s.h - t->h; if (d.h == s.h) d.h -= c; s.h -= c; }
    if (s.w <= 0 || s.h <= 0 || d.w <= 0 || d.h <= 0)
        return 0;
    d.x += draw_dx;
    d.y += draw_dy;
    int cx0, cy0, cx1, cy1;
    clip_bounds(&cx0, &cy0, &cx1, &cy1);
    int x0 = SDL_max(d.x, cx0), y0 = SDL_max(d.y, cy0);
    int x1 = SDL_min(d.x + d.w, cx1), y1 = SDL_min(d.y + d.h, cy1);
    if (x0 >= x1 || y0 >= y1)
        return 0;

    texture_palette(t);
    const uint16_t *pal = t->pal565;
    const uint8_t *pa = t->pala;
    const bool opaque_copy = t->blend == SDL_BLENDMODE_NONE;
    uint16_t *fb = screen();
    const int iw = t->im.w;
    const uint8_t *pix = t->im.pixels;
    const bool fh = (flip & SDL_FLIP_HORIZONTAL) != 0, fv = (flip & SDL_FLIP_VERTICAL) != 0;
    const bool unscaled = s.w == d.w && s.h == d.h;

    for (int y = y0; y < y1; y++) {
        int ry = y - d.y; /* row of the destination rect */
        int sy = unscaled ? ry : (int)((int64_t)ry * s.h / d.h);
        if (fv)
            sy = s.h - 1 - sy;
        const uint8_t *row = pix + (s.y + sy) * iw + s.x;
        uint16_t *out = fb + y * VV_WIDTH;
        if (unscaled && !fh) {
            const uint8_t *src = row + (x0 - d.x);
            for (int x = x0; x < x1; x++) {
                unsigned i = *src++;
                unsigned a = pa[i];
                if (opaque_copy || a == 255)
                    out[x] = pal[i];
                else if (a)
                    out[x] = blend565(out[x], pal[i], a);
            }
        } else {
            for (int x = x0; x < x1; x++) {
                int rx = x - d.x;
                int sx = unscaled ? rx : (int)((int64_t)rx * s.w / d.w);
                if (fh)
                    sx = s.w - 1 - sx;
                unsigned i = row[sx];
                unsigned a = pa[i];
                if (opaque_copy || a == 255)
                    out[x] = pal[i];
                else if (a)
                    out[x] = blend565(out[x], pal[i], a);
            }
        }
    }
    return 0;
}

int SDL_RenderCopyEx(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *srcrect, const SDL_Rect *dstrect,
                     const double angle, const SDL_Point *center, const SDL_RendererFlip flip)
{
    (void)r; (void)angle; (void)center;
    if (!t)
        return SDL_SetError("no texture");
    if (t->kind == TEX_IMAGE)
        return copy_image(t, srcrect, dstrect, flip);
    /* a target texture onto another: they are the frame itself (or
     * nothing): see the top of this file */
    return 0;
}

int SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *srcrect, const SDL_Rect *dstrect)
{
    return SDL_RenderCopyEx(r, t, srcrect, dstrect, 0, NULL, SDL_FLIP_NONE);
}

int SDL_RenderReadPixels(SDL_Renderer *r, const SDL_Rect *rect, Uint32 format, void *pixels, int pitch)
{
    (void)r; (void)rect; (void)format; (void)pixels; (void)pitch;
    return SDL_SetError("SDL_RenderReadPixels: not on the console");
}

void SDL_RenderPresent(SDL_Renderer *r)
{
    (void)r;
    draw_dx = draw_dy = 0;
    vv_plat_present();
}

/* ---- effects on the whole frame (Graphics.cpp under GNW) ---- */
void vv_sdl_set_offset(int dx, int dy)
{
    draw_dx = dx;
    draw_dy = dy;
}

void vv_sdl_screen_flip_vertical(void)
{
    uint16_t *fb = screen();
    uint16_t line[VV_WIDTH];
    for (int y = 0; y < VV_HEIGHT / 2; y++) {
        uint16_t *a = fb + y * VV_WIDTH, *b = fb + (VV_HEIGHT - 1 - y) * VV_WIDTH;
        memcpy(line, a, sizeof(line));
        memcpy(a, b, sizeof(line));
        memcpy(b, line, sizeof(line));
    }
}

void vv_sdl_screen_shift(int dx, int dy)
{
    uint16_t *fb = screen();
    if (dx <= -VV_WIDTH || dx >= VV_WIDTH || dy <= -VV_HEIGHT || dy >= VV_HEIGHT) {
        memset(fb, 0, VV_WIDTH * VV_HEIGHT * 2);
        return;
    }
    /* rows: from the end when moving down */
    if (dy > 0) {
        for (int y = VV_HEIGHT - 1; y >= 0; y--) {
            uint16_t *d = fb + y * VV_WIDTH;
            if (y - dy >= 0)
                memmove(d, fb + (y - dy) * VV_WIDTH, VV_WIDTH * 2);
            else
                memset(d, 0, VV_WIDTH * 2);
        }
    } else if (dy < 0) {
        for (int y = 0; y < VV_HEIGHT; y++) {
            uint16_t *d = fb + y * VV_WIDTH;
            if (y - dy < VV_HEIGHT)
                memmove(d, fb + (y - dy) * VV_WIDTH, VV_WIDTH * 2);
            else
                memset(d, 0, VV_WIDTH * 2);
        }
    }
    if (dx) {
        for (int y = 0; y < VV_HEIGHT; y++) {
            uint16_t *row = fb + y * VV_WIDTH;
            if (dx > 0) {
                memmove(row + dx, row, (VV_WIDTH - dx) * 2);
                memset(row, 0, dx * 2);
            } else {
                memmove(row, row - dx, (VV_WIDTH + dx) * 2);
                memset(row + VV_WIDTH + dx, 0, -dx * 2);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* time                                                               */
/* ------------------------------------------------------------------ */
Uint32 SDL_GetTicks(void) { return vv_plat_time_ms(); }
Uint64 SDL_GetTicks64(void) { return vv_plat_time_ms(); }
Uint64 SDL_GetPerformanceCounter(void) { return vv_plat_time_ms(); }
Uint64 SDL_GetPerformanceFrequency(void) { return 1000; }

void SDL_Delay(Uint32 ms)
{
    uint32_t t0 = vv_plat_time_ms();
    do {
        vv_plat_service(1);
    } while (vv_plat_time_ms() - t0 < ms);
}

/* ------------------------------------------------------------------ */
/* events: keys from the buttons                                      */
/* ------------------------------------------------------------------ */
#define MAX_KEYS 8
static SDL_Keycode keys_wanted[MAX_KEYS]; /* vv_sdl_set_keys() */
static int nkeys_wanted;
static SDL_Keycode keys_down[MAX_KEYS];   /* reported to the game */
static int nkeys_down;

void vv_sdl_set_keys(const SDL_Keycode *keys, int n)
{
    if (n > MAX_KEYS)
        n = MAX_KEYS;
    memcpy(keys_wanted, keys, n * sizeof(SDL_Keycode));
    nkeys_wanted = n;
}

static bool has_key(const SDL_Keycode *list, int n, SDL_Keycode k)
{
    for (int i = 0; i < n; i++)
        if (list[i] == k)
            return true;
    return false;
}

int SDL_PollEvent(SDL_Event *ev)
{
    static bool serviced;
    if (!serviced) {
        /* once per series of polls (the game polls until there is no
         * event left) */
        vv_plat_service(0);
        vv_core_update_keys();
        serviced = true;
    }
    /* releases first */
    for (int i = 0; i < nkeys_down; i++) {
        if (!has_key(keys_wanted, nkeys_wanted, keys_down[i])) {
            memset(ev, 0, sizeof(*ev));
            ev->type = SDL_KEYUP;
            ev->key.keysym.sym = keys_down[i];
            keys_down[i] = keys_down[--nkeys_down];
            return 1;
        }
    }
    for (int i = 0; i < nkeys_wanted; i++) {
        if (!has_key(keys_down, nkeys_down, keys_wanted[i]) && nkeys_down < MAX_KEYS) {
            memset(ev, 0, sizeof(*ev));
            ev->type = SDL_KEYDOWN;
            ev->key.state = 1;
            ev->key.keysym.sym = keys_wanted[i];
            keys_down[nkeys_down++] = keys_wanted[i];
            return 1;
        }
    }
    serviced = false;
    return 0;
}

void SDL_PumpEvents(void) {}
SDL_Keymod SDL_GetModState(void) { return KMOD_NONE; }
const char *SDL_GetKeyName(SDL_Keycode key) { (void)key; return ""; }

/* no mouse, no text input, no clipboard, no controllers */
int SDL_ShowCursor(int toggle) { (void)toggle; return SDL_DISABLE; }
static bool text_input;
void SDL_StartTextInput(void) { text_input = true; }
void SDL_StopTextInput(void) { text_input = false; }
SDL_bool SDL_IsTextInputActive(void) { return text_input ? SDL_TRUE : SDL_FALSE; }
void SDL_SetTextInputRect(const SDL_Rect *rect) { (void)rect; }
char *SDL_GetClipboardText(void) { return SDL_strdup(""); }
int SDL_SetClipboardText(const char *text) { (void)text; return -1; }
SDL_GameController *SDL_GameControllerOpen(int i) { (void)i; return NULL; }
void SDL_GameControllerClose(SDL_GameController *c) { (void)c; }
const char *SDL_GameControllerName(SDL_GameController *c) { (void)c; return ""; }
SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *c) { (void)c; return NULL; }
SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick *j) { (void)j; return -1; }
Uint16 SDL_GameControllerGetVendor(SDL_GameController *c) { (void)c; return 0; }
Uint16 SDL_GameControllerGetProduct(SDL_GameController *c) { (void)c; return 0; }
SDL_GameControllerType SDL_GameControllerGetType(SDL_GameController *c) { (void)c; return SDL_CONTROLLER_TYPE_UNKNOWN; }
int SDL_GameControllerAddMappingsFromFile(const char *f) { (void)f; return 0; }
int SDL_GameControllerRumble(SDL_GameController *c, Uint16 l, Uint16 h, Uint32 d) { (void)c; (void)l; (void)h; (void)d; return -1; }
