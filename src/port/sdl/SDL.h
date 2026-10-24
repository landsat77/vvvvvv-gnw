/*
 * The small SDL 2 of the Game & Watch port of VVVVVV.
 *
 * Only what VVVVVV 2.4 uses, with SDL 2's names, values and semantics. The
 * implementation is src/port/vv_sdl.cpp: a software renderer drawing
 * straight into the console's RGB565 screen (see there for how the render
 * target textures of the game are handled), images used in place from the
 * asset pack, key events made from the buttons, time from the platform.
 * No windows, mice, controllers, clipboard or text input: stubs.
 */
#ifndef VV_SDL_H
#define VV_SDL_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- SDL_stdinc.h -------------------------------------------------- */
typedef uint8_t  Uint8;
typedef int8_t   Sint8;
typedef uint16_t Uint16;
typedef int16_t  Sint16;
typedef uint32_t Uint32;
typedef int32_t  Sint32;
typedef uint64_t Uint64;
typedef int64_t  Sint64;

typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

#define SDL_INLINE inline
#define _SDL_HAS_BUILTIN(x) __has_builtin(x)
#define SDL_FORCE_INLINE static inline
#define SDL_NORETURN __attribute__((noreturn))
#define SDL_PRINTF_FORMAT_STRING
#define SDL_PRINTF_VARARG_FUNC(fmtargnumber) __attribute__((format(printf, fmtargnumber, fmtargnumber + 1)))
#define SDL_SCANF_FORMAT_STRING
#define SDL_SCANF_VARARG_FUNC(fmtargnumber)
#define SDL_FALLTHROUGH __attribute__((fallthrough))
#define SDL_UNUSED __attribute__((unused))
#define SDLCALL
#define DECLSPEC

#define SDL_arraysize(array) (sizeof(array) / sizeof(array[0]))
#define SDL_TABLESIZE(table) SDL_arraysize(table)
#define SDL_min(x, y) (((x) < (y)) ? (x) : (y))
#define SDL_max(x, y) (((x) > (y)) ? (x) : (y))
#define SDL_clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define SDL_zero(x) SDL_memset(&(x), 0, sizeof((x)))
#define SDL_zerop(x) SDL_memset((x), 0, sizeof(*(x)))
#define SDL_zeroa(x) SDL_memset((x), 0, sizeof((x)))
#define SDL_stack_alloc(type, count) (type *)SDL_malloc(sizeof(type) * (count))
#define SDL_stack_free(data) SDL_free(data)
#define SDL_COMPILE_TIME_ASSERT(name, x) typedef int SDL_compile_time_assert_##name[(x) * 2 - 1]
#define SDL_FOURCC(A, B, C, D) ((Uint32)(Uint8)(A) | ((Uint32)(Uint8)(B) << 8) | ((Uint32)(Uint8)(C) << 16) | ((Uint32)(Uint8)(D) << 24))

/* the game heap (vv_alloc.c) */
void *SDL_malloc(size_t size);
void *SDL_calloc(size_t nmemb, size_t size);
void *SDL_realloc(void *mem, size_t size);
void SDL_free(void *mem);

#define SDL_memset memset
#define SDL_memcpy memcpy
#define SDL_memmove memmove
#define SDL_memcmp memcmp
#define SDL_strlen strlen
#define SDL_strcmp strcmp
#define SDL_strncmp strncmp
#define SDL_strchr strchr
#define SDL_strrchr strrchr
#define SDL_strstr strstr
#define SDL_strtol strtol
#define SDL_strtoul strtoul
#define SDL_strtoll strtoll
#define SDL_strtod strtod
#define SDL_atoi atoi
#define SDL_atof atof
#define SDL_snprintf snprintf
#define SDL_vsnprintf vsnprintf
#define SDL_sscanf sscanf
#define SDL_abs abs
#define SDL_sin sin
#define SDL_cos cos
#define SDL_sinf sinf
#define SDL_cosf cosf
#define SDL_sqrt sqrt
#define SDL_sqrtf sqrtf
#define SDL_floor floor
#define SDL_floorf floorf
#define SDL_ceil ceil
#define SDL_ceilf ceilf
#define SDL_fabs fabs
#define SDL_fabsf fabsf
#define SDL_fmod fmod
#define SDL_fmodf fmodf
#define SDL_exp exp
#define SDL_log log
#define SDL_pow pow
#define SDL_scalbn scalbn
#define SDL_round round
#define SDL_lround lround

int SDL_tolower(int x);
int SDL_toupper(int x);
int SDL_isdigit(int x);
int SDL_isxdigit(int x);
int SDL_isspace(int x);
int SDL_isalpha(int x);
int SDL_strcasecmp(const char *a, const char *b);
int SDL_strncasecmp(const char *a, const char *b, size_t len);
char *SDL_strdup(const char *s);
size_t SDL_strlcpy(char *dst, const char *src, size_t maxlen);
size_t SDL_strlcat(char *dst, const char *src, size_t maxlen);
char *SDL_getenv(const char *name);
void SDL_qsort(void *base, size_t nmemb, size_t size, int (*compare)(const void *, const void *));

/* ---- SDL_assert.h / SDL_error.h / SDL_log.h ----------------------- */
#define SDL_assert(condition) ((void)0)
#define SDL_assert_release(condition) ((void)0)
#define SDL_assert_paranoid(condition) ((void)0)
#define SDL_assert_always(condition) ((void)0)
const char *SDL_GetError(void);
int SDL_SetError(const char *fmt, ...);
void SDL_ClearError(void);

enum { SDL_LOG_CATEGORY_APPLICATION = 0 };
typedef enum {
    SDL_LOG_PRIORITY_VERBOSE = 1,
    SDL_LOG_PRIORITY_DEBUG,
    SDL_LOG_PRIORITY_INFO,
    SDL_LOG_PRIORITY_WARN,
    SDL_LOG_PRIORITY_ERROR,
    SDL_LOG_PRIORITY_CRITICAL,
    SDL_NUM_LOG_PRIORITIES
} SDL_LogPriority;
void SDL_LogMessageV(int category, SDL_LogPriority priority, const char *fmt, va_list ap);
void SDL_LogSetPriority(int category, SDL_LogPriority priority);
void SDL_Log(const char *fmt, ...);

/* ---- SDL.h / SDL_hints.h / SDL_messagebox.h ---------------------- */
#define SDL_INIT_TIMER          0x00000001u
#define SDL_INIT_AUDIO          0x00000010u
#define SDL_INIT_VIDEO          0x00000020u
#define SDL_INIT_JOYSTICK       0x00000200u
#define SDL_INIT_HAPTIC         0x00001000u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS         0x00004000u
int SDL_Init(Uint32 flags);
int SDL_InitSubSystem(Uint32 flags);
void SDL_Quit(void);

typedef enum { SDL_HINT_DEFAULT, SDL_HINT_NORMAL, SDL_HINT_OVERRIDE } SDL_HintPriority;
#define SDL_HINT_RENDER_DRIVER "SDL_RENDER_DRIVER"
#define SDL_HINT_IME_SHOW_UI "SDL_IME_SHOW_UI"
#define SDL_HINT_IME_SUPPORT_EXTENDED_TEXT "SDL_IME_SUPPORT_EXTENDED_TEXT"
#define SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS "SDL_GAMECONTROLLER_USE_BUTTON_LABELS"
#define SDL_HINT_RENDER_VSYNC "SDL_RENDER_VSYNC"
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
#define SDL_HINT_VIDEO_HIGHDPI_DISABLED "SDL_VIDEO_HIGHDPI_DISABLED"
SDL_bool SDL_SetHintWithPriority(const char *name, const char *value, SDL_HintPriority priority);
SDL_bool SDL_SetHint(const char *name, const char *value);
SDL_bool SDL_GetHintBoolean(const char *name, SDL_bool default_value);
const char *SDL_GetHint(const char *name);

#define SDL_MESSAGEBOX_ERROR       0x00000010
#define SDL_MESSAGEBOX_WARNING     0x00000020
#define SDL_MESSAGEBOX_INFORMATION 0x00000040

/* ---- SDL_rect.h / SDL_pixels.h ------------------------------------ */
typedef struct SDL_Point { int x, y; } SDL_Point;
typedef struct SDL_FPoint { float x, y; } SDL_FPoint;
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
typedef struct SDL_FRect { float x, y, w, h; } SDL_FRect;

static inline SDL_bool SDL_PointInRect(const SDL_Point *p, const SDL_Rect *r)
{
    return (p->x >= r->x && p->x < r->x + r->w && p->y >= r->y && p->y < r->y + r->h) ? SDL_TRUE : SDL_FALSE;
}
static inline SDL_bool SDL_RectEmpty(const SDL_Rect *r)
{
    return (!r || r->w <= 0 || r->h <= 0) ? SDL_TRUE : SDL_FALSE;
}
SDL_bool SDL_HasIntersection(const SDL_Rect *A, const SDL_Rect *B);
SDL_bool SDL_IntersectRect(const SDL_Rect *A, const SDL_Rect *B, SDL_Rect *result);

typedef struct SDL_Color { Uint8 r, g, b, a; } SDL_Color;
#define SDL_Colour SDL_Color

typedef struct SDL_Palette {
    int ncolors;
    SDL_Color *colors;
    Uint32 version;
    int refcount;
} SDL_Palette;

/* Formats: only what VVVVVV names. The port's images are 8 bit with a
 * palette (SDL_PIXELFORMAT_INDEX8), its screen RGB565. */
#define SDL_PIXELFORMAT_UNKNOWN  0u
#define SDL_PIXELFORMAT_INDEX8   0x13000801u
#define SDL_PIXELFORMAT_RGB565   0x15151002u
#define SDL_PIXELFORMAT_RGB24    0x17101803u
#define SDL_PIXELFORMAT_RGB888   0x16161804u
#define SDL_PIXELFORMAT_ARGB8888 0x16362004u
#define SDL_PIXELFORMAT_RGBA8888 0x16462004u
#define SDL_PIXELFORMAT_ABGR8888 0x16762004u

typedef struct SDL_PixelFormat {
    Uint32 format;
    SDL_Palette *palette;
    Uint8 BitsPerPixel;
    Uint8 BytesPerPixel;
    Uint8 padding[2];
    Uint32 Rmask, Gmask, Bmask, Amask;
    Uint8 Rloss, Gloss, Bloss, Aloss;
    Uint8 Rshift, Gshift, Bshift, Ashift;
    int refcount;
    struct SDL_PixelFormat *next;
} SDL_PixelFormat;

Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b);
Uint32 SDL_MapRGBA(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void SDL_GetRGB(Uint32 pixel, const SDL_PixelFormat *format, Uint8 *r, Uint8 *g, Uint8 *b);
void SDL_GetRGBA(Uint32 pixel, const SDL_PixelFormat *format, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a);

/* ---- SDL_blendmode.h / SDL_surface.h ------------------------------ */
typedef enum {
    SDL_BLENDMODE_NONE = 0x00000000,
    SDL_BLENDMODE_BLEND = 0x00000001,
    SDL_BLENDMODE_ADD = 0x00000002,
    SDL_BLENDMODE_MOD = 0x00000004,
    SDL_BLENDMODE_MUL = 0x00000008,
    SDL_BLENDMODE_INVALID = 0x7FFFFFFF
} SDL_BlendMode;

#define SDL_SWSURFACE 0
#define SDL_PREALLOC  0x00000001
typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h;
    int pitch;
    void *pixels;
    void *userdata;
    int locked;
    void *list_blitmap;
    SDL_Rect clip_rect;
    void *map;
    int refcount;
} SDL_Surface;

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
SDL_Surface *SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format);
SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 format);
void SDL_FreeSurface(SDL_Surface *surface);
int SDL_LockSurface(SDL_Surface *surface);
void SDL_UnlockSurface(SDL_Surface *surface);
int SDL_SetSurfaceBlendMode(SDL_Surface *surface, SDL_BlendMode blendMode);
int SDL_GetSurfaceBlendMode(SDL_Surface *surface, SDL_BlendMode *blendMode);
int SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
int SDL_BlitScaled(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
SDL_Surface *SDL_ConvertSurfaceFormat(SDL_Surface *src, Uint32 pixel_format, Uint32 flags);
int SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color);

/* ---- SDL_video.h -------------------------------------------------- */
typedef struct SDL_Window SDL_Window;
typedef enum {
    SDL_WINDOW_FULLSCREEN = 0x00000001,
    SDL_WINDOW_OPENGL = 0x00000002,
    SDL_WINDOW_SHOWN = 0x00000004,
    SDL_WINDOW_HIDDEN = 0x00000008,
    SDL_WINDOW_BORDERLESS = 0x00000010,
    SDL_WINDOW_RESIZABLE = 0x00000020,
    SDL_WINDOW_MINIMIZED = 0x00000040,
    SDL_WINDOW_MAXIMIZED = 0x00000080,
    SDL_WINDOW_INPUT_FOCUS = 0x00000200,
    SDL_WINDOW_MOUSE_FOCUS = 0x00000400,
    SDL_WINDOW_FULLSCREEN_DESKTOP = (SDL_WINDOW_FULLSCREEN | 0x00001000),
    SDL_WINDOW_ALLOW_HIGHDPI = 0x00002000
} SDL_WindowFlags;
#define SDL_WINDOWPOS_UNDEFINED 0x1FFF0000u
#define SDL_WINDOWPOS_CENTERED  0x2FFF0000u
#define SDL_WINDOWPOS_CENTERED_DISPLAY(X) (SDL_WINDOWPOS_CENTERED | (X))
typedef struct { Uint32 format; int w, h, refresh_rate; void *driverdata; } SDL_DisplayMode;

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags);
void SDL_DestroyWindow(SDL_Window *window);
Uint32 SDL_GetWindowFlags(SDL_Window *window);
SDL_Window *SDL_GetWindowFromID(Uint32 id);
int SDL_SetWindowFullscreen(SDL_Window *window, Uint32 flags);
void SDL_SetWindowSize(SDL_Window *window, int w, int h);
void SDL_GetWindowSize(SDL_Window *window, int *w, int *h);
void SDL_SetWindowPosition(SDL_Window *window, int x, int y);
void SDL_SetWindowMinimumSize(SDL_Window *window, int min_w, int min_h);
void SDL_SetWindowTitle(SDL_Window *window, const char *title);
void SDL_SetWindowIcon(SDL_Window *window, SDL_Surface *icon);
void SDL_ShowWindow(SDL_Window *window);
void SDL_MinimizeWindow(SDL_Window *window);
int SDL_GetWindowDisplayIndex(SDL_Window *window);
int SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode *mode);
int SDL_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode *mode);
const char *SDL_GetCurrentVideoDriver(void);
void SDL_EnableScreenSaver(void);
void SDL_DisableScreenSaver(void);
int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window);

/* ---- SDL_render.h ------------------------------------------------- */
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef enum {
    SDL_RENDERER_SOFTWARE = 0x00000001,
    SDL_RENDERER_ACCELERATED = 0x00000002,
    SDL_RENDERER_PRESENTVSYNC = 0x00000004,
    SDL_RENDERER_TARGETTEXTURE = 0x00000008
} SDL_RendererFlags;
typedef struct SDL_RendererInfo {
    const char *name;
    Uint32 flags;
    Uint32 num_texture_formats;
    Uint32 texture_formats[16];
    int max_texture_width;
    int max_texture_height;
} SDL_RendererInfo;
typedef enum { SDL_ScaleModeNearest, SDL_ScaleModeLinear, SDL_ScaleModeBest } SDL_ScaleMode;
typedef enum {
    SDL_TEXTUREACCESS_STATIC,
    SDL_TEXTUREACCESS_STREAMING,
    SDL_TEXTUREACCESS_TARGET
} SDL_TextureAccess;
typedef enum {
    SDL_FLIP_NONE = 0x00000000,
    SDL_FLIP_HORIZONTAL = 0x00000001,
    SDL_FLIP_VERTICAL = 0x00000002
} SDL_RendererFlip;

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags);
void SDL_DestroyRenderer(SDL_Renderer *renderer);
int SDL_GetRendererInfo(SDL_Renderer *renderer, SDL_RendererInfo *info);
int SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h);
int SDL_RenderSetIntegerScale(SDL_Renderer *renderer, SDL_bool enable);
int SDL_RenderSetVSync(SDL_Renderer *renderer, int vsync);
int SDL_GetRendererOutputSize(SDL_Renderer *renderer, int *w, int *h);
SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h);
SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *renderer, SDL_Surface *surface);
void SDL_DestroyTexture(SDL_Texture *texture);
int SDL_QueryTexture(SDL_Texture *texture, Uint32 *format, int *access, int *w, int *h);
int SDL_SetTextureColorMod(SDL_Texture *texture, Uint8 r, Uint8 g, Uint8 b);
int SDL_GetTextureColorMod(SDL_Texture *texture, Uint8 *r, Uint8 *g, Uint8 *b);
int SDL_SetTextureAlphaMod(SDL_Texture *texture, Uint8 alpha);
int SDL_GetTextureAlphaMod(SDL_Texture *texture, Uint8 *alpha);
int SDL_SetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode blendMode);
int SDL_GetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode *blendMode);
int SDL_SetTextureScaleMode(SDL_Texture *texture, SDL_ScaleMode scaleMode);
int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch);
int SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture);
SDL_Texture *SDL_GetRenderTarget(SDL_Renderer *renderer);
int SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
int SDL_GetRenderDrawColor(SDL_Renderer *renderer, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a);
int SDL_SetRenderDrawBlendMode(SDL_Renderer *renderer, SDL_BlendMode blendMode);
int SDL_GetRenderDrawBlendMode(SDL_Renderer *renderer, SDL_BlendMode *blendMode);
int SDL_RenderClear(SDL_Renderer *renderer);
int SDL_RenderDrawPoint(SDL_Renderer *renderer, int x, int y);
int SDL_RenderDrawPoints(SDL_Renderer *renderer, const SDL_Point *points, int count);
int SDL_RenderDrawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2);
int SDL_RenderDrawRect(SDL_Renderer *renderer, const SDL_Rect *rect);
int SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_Rect *rect);
int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect);
int SDL_RenderCopyEx(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect,
                     const double angle, const SDL_Point *center, const SDL_RendererFlip flip);
int SDL_RenderReadPixels(SDL_Renderer *renderer, const SDL_Rect *rect, Uint32 format, void *pixels, int pitch);
void SDL_RenderPresent(SDL_Renderer *renderer);

/* ---- SDL_timer.h -------------------------------------------------- */
Uint32 SDL_GetTicks(void);
Uint64 SDL_GetTicks64(void);
Uint64 SDL_GetPerformanceCounter(void);
Uint64 SDL_GetPerformanceFrequency(void);
void SDL_Delay(Uint32 ms);

/* ---- SDL_keycode.h ------------------------------------------------ */
typedef Sint32 SDL_Keycode;
typedef int SDL_Scancode;
#define SDLK_SCANCODE_MASK (1 << 30)
#define SDL_SCANCODE_TO_KEYCODE(X) (X | SDLK_SCANCODE_MASK)
typedef enum {
    SDLK_UNKNOWN = 0,
    SDLK_RETURN = '\r',
    SDLK_ESCAPE = '\x1B',
    SDLK_BACKSPACE = '\b',
    SDLK_TAB = '\t',
    SDLK_SPACE = ' ',
    SDLK_PERIOD = '.',
    SDLK_COMMA = ',',
    SDLK_0 = '0', SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9,
    SDLK_a = 'a', SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f, SDLK_g, SDLK_h, SDLK_i, SDLK_j,
    SDLK_k, SDLK_l, SDLK_m, SDLK_n, SDLK_o, SDLK_p, SDLK_q, SDLK_r, SDLK_s, SDLK_t,
    SDLK_u, SDLK_v, SDLK_w, SDLK_x, SDLK_y, SDLK_z,
    SDLK_DELETE = '\x7F',
    SDLK_F1 = SDL_SCANCODE_TO_KEYCODE(58),
    SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12,
    SDLK_RIGHT = SDL_SCANCODE_TO_KEYCODE(79),
    SDLK_LEFT = SDL_SCANCODE_TO_KEYCODE(80),
    SDLK_DOWN = SDL_SCANCODE_TO_KEYCODE(81),
    SDLK_UP = SDL_SCANCODE_TO_KEYCODE(82),
    SDLK_KP_ENTER = SDL_SCANCODE_TO_KEYCODE(88),
    SDLK_LCTRL = SDL_SCANCODE_TO_KEYCODE(224),
    SDLK_LSHIFT = SDL_SCANCODE_TO_KEYCODE(225),
    SDLK_LALT = SDL_SCANCODE_TO_KEYCODE(226),
    SDLK_LGUI = SDL_SCANCODE_TO_KEYCODE(227),
    SDLK_RCTRL = SDL_SCANCODE_TO_KEYCODE(228),
    SDLK_RSHIFT = SDL_SCANCODE_TO_KEYCODE(229),
    SDLK_RALT = SDL_SCANCODE_TO_KEYCODE(230),
    SDLK_RGUI = SDL_SCANCODE_TO_KEYCODE(231)
} SDL_KeyCode;
typedef enum {
    KMOD_NONE = 0x0000,
    KMOD_LSHIFT = 0x0001, KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL = 0x0040, KMOD_RCTRL = 0x0080,
    KMOD_LALT = 0x0100, KMOD_RALT = 0x0200,
    KMOD_LGUI = 0x0400, KMOD_RGUI = 0x0800,
    KMOD_CTRL = KMOD_LCTRL | KMOD_RCTRL,
    KMOD_SHIFT = KMOD_LSHIFT | KMOD_RSHIFT,
    KMOD_ALT = KMOD_LALT | KMOD_RALT,
    KMOD_GUI = KMOD_LGUI | KMOD_RGUI
} SDL_Keymod;
typedef struct SDL_Keysym {
    SDL_Scancode scancode;
    SDL_Keycode sym;
    Uint16 mod;
    Uint32 unused;
} SDL_Keysym;
SDL_Keymod SDL_GetModState(void);
const char *SDL_GetKeyName(SDL_Keycode key);

/* ---- SDL_joystick.h / SDL_gamecontroller.h ------------------------ */
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_GameController SDL_GameController;
typedef Sint32 SDL_JoystickID;
typedef enum {
    SDL_CONTROLLER_TYPE_UNKNOWN = 0,
    SDL_CONTROLLER_TYPE_XBOX360,
    SDL_CONTROLLER_TYPE_XBOXONE,
    SDL_CONTROLLER_TYPE_PS3,
    SDL_CONTROLLER_TYPE_PS4,
    SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO,
    SDL_CONTROLLER_TYPE_VIRTUAL,
    SDL_CONTROLLER_TYPE_PS5
} SDL_GameControllerType;
typedef enum {
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_MISC1,
    SDL_CONTROLLER_BUTTON_PADDLE1,
    SDL_CONTROLLER_BUTTON_PADDLE2,
    SDL_CONTROLLER_BUTTON_PADDLE3,
    SDL_CONTROLLER_BUTTON_PADDLE4,
    SDL_CONTROLLER_BUTTON_TOUCHPAD,
    SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;
typedef enum {
    SDL_CONTROLLER_AXIS_INVALID = -1,
    SDL_CONTROLLER_AXIS_LEFTX,
    SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    SDL_CONTROLLER_AXIS_MAX
} SDL_GameControllerAxis;
SDL_GameController *SDL_GameControllerOpen(int joystick_index);
void SDL_GameControllerClose(SDL_GameController *gamecontroller);
const char *SDL_GameControllerName(SDL_GameController *gamecontroller);
SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *gamecontroller);
SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick *joystick);
Uint16 SDL_GameControllerGetVendor(SDL_GameController *gamecontroller);
Uint16 SDL_GameControllerGetProduct(SDL_GameController *gamecontroller);
SDL_GameControllerType SDL_GameControllerGetType(SDL_GameController *gamecontroller);
int SDL_GameControllerAddMappingsFromFile(const char *file);
int SDL_GameControllerRumble(SDL_GameController *gamecontroller, Uint16 low, Uint16 high, Uint32 duration_ms);

/* ---- SDL_mouse.h / SDL_keyboard.h / SDL_clipboard.h --------------- */
#define SDL_QUERY   -1
#define SDL_IGNORE   0
#define SDL_DISABLE  0
#define SDL_ENABLE   1
#define SDL_BUTTON_LEFT   1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT  3
int SDL_ShowCursor(int toggle);
void SDL_StartTextInput(void);
void SDL_StopTextInput(void);
SDL_bool SDL_IsTextInputActive(void);
void SDL_SetTextInputRect(const SDL_Rect *rect);
char *SDL_GetClipboardText(void);
int SDL_SetClipboardText(const char *text);

/* ---- SDL_events.h ------------------------------------------------- */
typedef enum {
    SDL_FIRSTEVENT = 0,
    SDL_QUIT = 0x100,
    SDL_WINDOWEVENT = 0x200,
    SDL_KEYDOWN = 0x300,
    SDL_KEYUP,
    SDL_TEXTEDITING,
    SDL_TEXTINPUT,
    SDL_KEYMAPCHANGED,
    SDL_TEXTEDITING_EXT,
    SDL_MOUSEMOTION = 0x400,
    SDL_MOUSEBUTTONDOWN,
    SDL_MOUSEBUTTONUP,
    SDL_MOUSEWHEEL,
    SDL_CONTROLLERAXISMOTION = 0x650,
    SDL_CONTROLLERBUTTONDOWN,
    SDL_CONTROLLERBUTTONUP,
    SDL_CONTROLLERDEVICEADDED,
    SDL_CONTROLLERDEVICEREMOVED,
    SDL_LASTEVENT = 0xFFFF
} SDL_EventType;
typedef enum {
    SDL_WINDOWEVENT_NONE,
    SDL_WINDOWEVENT_SHOWN,
    SDL_WINDOWEVENT_HIDDEN,
    SDL_WINDOWEVENT_EXPOSED,
    SDL_WINDOWEVENT_MOVED,
    SDL_WINDOWEVENT_RESIZED,
    SDL_WINDOWEVENT_SIZE_CHANGED,
    SDL_WINDOWEVENT_MINIMIZED,
    SDL_WINDOWEVENT_MAXIMIZED,
    SDL_WINDOWEVENT_RESTORED,
    SDL_WINDOWEVENT_ENTER,
    SDL_WINDOWEVENT_LEAVE,
    SDL_WINDOWEVENT_FOCUS_GAINED,
    SDL_WINDOWEVENT_FOCUS_LOST,
    SDL_WINDOWEVENT_CLOSE
} SDL_WindowEventID;
#define SDL_TEXTEDITINGEVENT_TEXT_SIZE 32
#define SDL_TEXTINPUTEVENT_TEXT_SIZE 32
typedef struct { Uint32 type, timestamp, windowID; Uint8 state, repeat, padding2, padding3; SDL_Keysym keysym; } SDL_KeyboardEvent;
typedef struct { Uint32 type, timestamp, windowID; Uint8 event, padding1, padding2, padding3; Sint32 data1, data2; } SDL_WindowEvent;
typedef struct { Uint32 type, timestamp, windowID; char text[SDL_TEXTINPUTEVENT_TEXT_SIZE]; } SDL_TextInputEvent;
typedef struct { Uint32 type, timestamp, windowID; char text[SDL_TEXTEDITINGEVENT_TEXT_SIZE]; Sint32 start, length; } SDL_TextEditingEvent;
typedef struct { Uint32 type, timestamp, windowID; char *text; Sint32 start, length; } SDL_TextEditingExtEvent;
typedef struct { Uint32 type, timestamp, windowID, which, state; Sint32 x, y, xrel, yrel; } SDL_MouseMotionEvent;
typedef struct { Uint32 type, timestamp, windowID, which; Uint8 button, state, clicks, padding1; Sint32 x, y; } SDL_MouseButtonEvent;
typedef struct { Uint32 type, timestamp; SDL_JoystickID which; Uint8 axis, padding1, padding2, padding3; Sint16 value; Uint16 padding4; } SDL_ControllerAxisEvent;
typedef struct { Uint32 type, timestamp; SDL_JoystickID which; Uint8 button, state, padding1, padding2; } SDL_ControllerButtonEvent;
typedef struct { Uint32 type, timestamp; Sint32 which; } SDL_ControllerDeviceEvent;
typedef union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_WindowEvent window;
    SDL_TextInputEvent text;
    SDL_TextEditingEvent edit;
    SDL_TextEditingExtEvent editExt;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_ControllerAxisEvent caxis;
    SDL_ControllerButtonEvent cbutton;
    SDL_ControllerDeviceEvent cdevice;
    Uint8 padding[56];
} SDL_Event;
int SDL_PollEvent(SDL_Event *event);
void SDL_PumpEvents(void);

/* ---- SDL_rwops.h (unused by the port's file layer) ---------------- */
typedef struct SDL_RWops SDL_RWops;

/* ---- SDL_cpuinfo / SDL_version / SDL_platform --------------------- */
const char *SDL_GetPlatform(void);
typedef struct SDL_version { Uint8 major, minor, patch; } SDL_version;
#define SDL_MAJOR_VERSION 2
#define SDL_MINOR_VERSION 30
#define SDL_PATCHLEVEL 0
#define SDL_VERSION(x) do { (x)->major = 2; (x)->minor = 30; (x)->patch = 0; } while (0)
#define SDL_VERSION_ATLEAST(X, Y, Z) ((2 * 1000 + 30) * 1000 >= ((X) * 1000 + (Y)) * 1000 + (Z))
void SDL_GetVersion(SDL_version *ver);

#define SDL_BYTEORDER 1234
#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321
#define SDL_SwapLE16(X) (X)
#define SDL_SwapLE32(X) (X)

#ifdef __cplusplus
}
#endif

#endif
