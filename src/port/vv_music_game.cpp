/*
 * Music.cpp of the port (replaces the FAudio / stb_vorbis one).
 *
 * The songs (music/<name>.mus) and the sound effects (sounds/<name>.wav)
 * are GWMU samples in the pack (tools/vv_pack.py, vv_render.c), at the
 * output rate (or half of it for the songs: --music-rate 16000), mono. They
 * are decoded in place from the flash while the platform asks for audio
 * (vv_core_audio_render): one song, 8 sound channels (like VVVVVV: a new
 * sound takes a free channel, or is not played).
 *
 * The musicclass logic (fades, queued songs, areas) is the one of
 * Music.cpp; only the track layer underneath changed.
 */
#include "Music.h"

#include <SDL.h>
#include <string.h>

#include "Game.h"
#include "Graphics.h"
#include "Map.h"
#include "Script.h"
#include "UtilityClass.h"
#include "Vlogging.h"

#include "vv_platform.h"
#include "vv_music.h"
#include "vv_ogg.h"

#define VVV_MAX_VOLUME 128
#define VVV_MAX_CHANNELS 8

/* ------------------------------------------------------------------ */
/* the track layer                                                    */
/* ------------------------------------------------------------------ */
static const char* const song_names[] = {
    "0levelcomplete", "1pushingonwards", "2positiveforce", "3potentialforanything",
    "4passionforexploring", "5intermission", "6presentingvvvvvv", "7gamecomplete",
    "8predestinedfate", "9positiveforcereversed", "10popularpotpourri", "11pipedream",
    "12pressurecooker", "13pacedenergy", "14piercingthesky", "predestinedfatefinallevel",
};
#define NUM_SONGS ((int) SDL_arraysize(song_names))

static const char* const sound_names[] = {
    "jump", "jump2", "hurt", "souleyeminijingle", "coin", "save", "crumble", "vanish",
    "blip", "preteleport", "teleport", "crew1", "crew2", "crew3", "crew4", "crew5",
    "crew6", "terminal", "gamesaved", "crashing", "blip2", "countdown", "go", "crash",
    "combine", "newrecord", "trophy", "rescue",
};
#define NUM_SOUNDS ((int) SDL_arraysize(sound_names))

struct Sample
{
    const uint8_t* data;
    uint32_t size;
    bool ogg;                   /* the original Ogg Vorbis file (else GWMU) */
};
static Sample songs[NUM_SONGS];
static Sample sounds[NUM_SOUNDS];

/* the song playing (MusicTrack of Music.cpp) */
static vv_mus_t* song;          /* NULL: halted (GWMU songs) */
static vv_ogg_t* song_ogg;      /* the same for an Ogg Vorbis song */
static bool song_loops;
static bool song_paused = true;
static uint32_t song_left;      /* frames before the end of a song that does not loop */
static int song_volume;         /* 0 .. VVV_MAX_VOLUME */

/* the sound channels (SoundTrack) */
struct Channel
{
    vv_mus_t* mus;
    uint32_t left;              /* frames to play */
};
static Channel channels[VVV_MAX_CHANNELS];
static bool sounds_paused;
static int sound_volume = VVV_MAX_VOLUME;

/* options of the platform's menu */
static bool opt_music = true, opt_sound = true;

static void song_halt(void)
{
    if (song != NULL || song_ogg != NULL)
    {
        vv_mus_close(song);
        song = NULL;
        vv_ogg_close(song_ogg);
        song_ogg = NULL;
        song_paused = true;
    }
}

static bool song_play(int t, bool loop)
{
    song_halt();
    if (t < 0 || t >= NUM_SONGS || songs[t].data == NULL)
    {
        return false;
    }
    if (songs[t].ogg)
    {
        song_ogg = vv_ogg_open(songs[t].data, songs[t].size);
        if (song_ogg == NULL)
        {
            vlog_error("Cannot play %s (out of memory?)", song_names[t]);
            return false;
        }
        song_left = 0;
    }
    else
    {
        song = vv_mus_open(songs[t].data, (int) songs[t].size);
        if (song == NULL)
        {
            return false;
        }
        song_left = vv_mus_length(song, VV_SAMPLE_RATE);
    }
    song_loops = loop;
    song_paused = false;
    return true;
}

static bool song_is_paused(void)
{
    return song_paused || (song == NULL && song_ogg == NULL);
}

static void sound_play(int t)
{
    if (sounds[t].data == NULL)
    {
        return;
    }
    for (int i = 0; i < VVV_MAX_CHANNELS; i++)
    {
        if (channels[i].mus == NULL)
        {
            channels[i].mus = vv_mus_open(sounds[t].data, (int) sounds[t].size);
            channels[i].left = vv_mus_length(channels[i].mus, VV_SAMPLE_RATE);
            return;
        }
    }
}

static void sound_stop(Channel* c)
{
    vv_mus_close(c->mus);
    c->mus = NULL;
}

/* the rate of the pack's sound effects (vv_render: GWMU, rate at 8) */
int vv_core_audio_rate(void)
{
    static int rate;
    if (rate == 0)
    {
        const uint8_t* data;
        uint32_t size;
        rate = 32000;
        if (vv_plat_find_file("sounds/jump.wav", &data, &size) && size >= 12 && memcmp(data, "GWMU", 4) == 0)
        {
            const int r = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
            if (r >= 8000 && r <= 48000)
            {
                rate = r;
            }
        }
    }
    return rate;
}

void vv_core_set_sound(bool music_on, bool sound_on)
{
    opt_music = music_on;
    opt_sound = sound_on;
}

/* mixes `src` (mono, `n` frames) scaled by vol / (VVV_MAX_VOLUME * 256)
 * into acc */
static void add_scaled(int32_t* acc, const int16_t* src, int n, int vol)
{
    for (int i = 0; i < n; i++)
    {
        acc[i] += src[i] * vol;
    }
}

void vv_core_audio_render(int16_t* out, int frames)
{
    enum { CHUNK = 256 };
    int32_t acc[CHUNK];
    int16_t tmp[CHUNK];
    while (frames > 0)
    {
        int n = frames < CHUNK ? frames : CHUNK;
        memset(acc, 0, sizeof(acc));

        if (song_ogg != NULL && !song_paused)
        {
            int m = 0;
            int rewinds = 0;
            while (m < n)
            {
                const int got = vv_ogg_read(song_ogg, tmp + m, n - m);
                if (got > 0)
                {
                    m += got;
                    continue;
                }
                /* the end: again from the start (once per chunk: an
                 * empty song stops), or silence for a song that ends */
                if (!song_loops || rewinds++ > 0)
                {
                    break;
                }
                vv_ogg_rewind(song_ogg);
            }
            if (m > 0 && opt_music && !game.muted && !game.musicmuted)
            {
                add_scaled(acc, tmp, m, song_volume * music.user_music_volume / USER_VOLUME_MAX * 2);
            }
        }
        else if (song != NULL && !song_paused)
        {
            int m = n;
            if (!song_loops && (uint32_t) m > song_left)
            {
                m = (int) song_left;
            }
            if (m > 0)
            {
                memset(tmp, 0, m * sizeof(int16_t));
                vv_mus_mix(song, tmp, m, 1, 1, VV_SAMPLE_RATE);
                if (opt_music && !game.muted && !game.musicmuted)
                {
                    add_scaled(acc, tmp, m, song_volume * music.user_music_volume / USER_VOLUME_MAX * 2);
                }
                if (!song_loops)
                {
                    song_left -= m;
                }
            }
            /* a song that ends stays "playing" (silent) like a voice whose
             * buffers have run out */
        }

        if (!sounds_paused)
        {
            for (int i = 0; i < VVV_MAX_CHANNELS; i++)
            {
                Channel* c = &channels[i];
                if (c->mus == NULL)
                {
                    continue;
                }
                int m = (uint32_t) n > c->left ? (int) c->left : n;
                memset(tmp, 0, m * sizeof(int16_t));
                vv_mus_mix(c->mus, tmp, m, 1, 1, VV_SAMPLE_RATE);
                if (opt_sound && !game.muted)
                {
                    add_scaled(acc, tmp, m, sound_volume * music.user_sound_volume / USER_VOLUME_MAX * 2);
                }
                c->left -= m;
                if (c->left == 0)
                {
                    sound_stop(c);
                }
            }
        }

        for (int i = 0; i < n; i++)
        {
            /* volumes: 128 * 2 = unity */
            int32_t v = acc[i] >> 8;
            out[i] = (int16_t) (v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        }
        out += n;
        frames -= n;
    }
}

/* ------------------------------------------------------------------ */
/* musicclass (Music.cpp)                                             */
/* ------------------------------------------------------------------ */
musicclass::musicclass(void)
{
    safeToProcessMusic= false;
    m_doFadeInVol = false;
    m_doFadeOutVol = false;
    musicVolume = 0;

    user_music_volume = USER_VOLUME_MAX;
    user_sound_volume = USER_VOLUME_MAX;

    currentsong = -1;
    haltedsong = -1;
    nicechange = -1;
    nicefade = false;
    quick_fade = true;

    usingmmmmmm = false;
}

void musicclass::init(void)
{
    char name[48];
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        SDL_snprintf(name, sizeof(name), "sounds/%s.wav", sound_names[i]);
        if (!vv_plat_find_file(name, &sounds[i].data, &sounds[i].size))
        {
            vlog_error("Sound not found: %s", name);
        }
    }
    num_pppppp_tracks = 0;
    for (int i = 0; i < NUM_SONGS; i++)
    {
        SDL_snprintf(name, sizeof(name), "music/%s.mus", song_names[i]);
        songs[i].ogg = false;
        if (!vv_plat_find_file(name, &songs[i].data, &songs[i].size))
        {
            /* vv_pack.py --music ogg: the original files */
            SDL_snprintf(name, sizeof(name), "music/%s.ogg", song_names[i]);
            songs[i].ogg = vv_plat_find_file(name, &songs[i].data, &songs[i].size);
        }
        if (songs[i].data != NULL)
        {
            num_pppppp_tracks = i + 1;
        }
    }
    if (num_pppppp_tracks == 0)
    {
        /* a pack made with --no-music: the game still counts its songs */
        vlog_info("No music in the pack");
        num_pppppp_tracks = NUM_SONGS;
    }
    num_mmmmmm_tracks = 0;
    mmmmmm = false;
    usingmmmmmm = false;
}

void musicclass::destroy(void)
{
    song_halt();
    for (int i = 0; i < VVV_MAX_CHANNELS; i++)
    {
        if (channels[i].mus != NULL)
        {
            sound_stop(&channels[i]);
        }
    }
}

void musicclass::play(int t)
{
    if (num_pppppp_tracks > 0)
    {
        t %= num_pppppp_tracks;
    }

    safeToProcessMusic = true;

    if (currentsong == t && !m_doFadeOutVol)
    {
        return;
    }

    currentsong = t;
    haltedsong = -1;

    if (t == -1)
    {
        return;
    }

    if (t < 0 || t >= num_pppppp_tracks)
    {
        vlog_error("play() out-of-bounds!");
        currentsong = -1;
        return;
    }

    if (currentsong == Music_PATHCOMPLETE || currentsong == Music_PLENARY)
    {
        // No fade in or repeat
        if (song_play(t, false))
        {
            m_doFadeInVol = false;
            m_doFadeOutVol = false;
            musicVolume = VVV_MAX_VOLUME;
            song_volume = VVV_MAX_VOLUME;
        }
    }
    else
    {
        if (m_doFadeOutVol)
        {
            // We're already fading out
            nicechange = t;
            nicefade = true;
            currentsong = -1;

            if (quick_fade)
            {
                fadeMusicVolumeOut(500); // fade out quicker
            }
            else
            {
                quick_fade = true;
            }
        }
        else if (song_play(t, true))
        {
            m_doFadeInVol = false;
            m_doFadeOutVol = false;
            fadeMusicVolumeIn(3000);
        }
    }
}

void musicclass::resume(void)
{
    if (currentsong == -1)
    {
        currentsong = haltedsong;
        haltedsong = -1;
    }
    if (song != NULL)
    {
        song_paused = false;
    }
}

void musicclass::resumefade(const int fadein_ms)
{
    resume();
    fadeMusicVolumeIn(fadein_ms);
}

void musicclass::fadein(void)
{
    resumefade(3000); // 3000 ms fadein
}

void musicclass::pause(void)
{
    if (song != NULL)
    {
        song_paused = true;
    }
}

void musicclass::haltdasmusik(void)
{
    haltdasmusik(false);
}

void musicclass::haltdasmusik(const bool from_fade)
{
    /* Just pauses music. This is intended. */
    pause();
    haltedsong = currentsong;
    currentsong = -1;
    m_doFadeInVol = false;
    m_doFadeOutVol = false;
    if (!from_fade)
    {
        nicefade = false;
        nicechange = -1;
    }
}

void musicclass::silencedasmusik(void)
{
    musicVolume = 0;
    m_doFadeInVol = false;
    m_doFadeOutVol = false;
}

struct FadeState
{
    int start_volume;
    int end_volume;
    int duration_ms;
    int step_ms;
};

static struct FadeState fade;

enum FadeCode
{
    Fade_continue,
    Fade_finished
};

static enum FadeCode processmusicfade(struct FadeState* state, int* volume)
{
    int range;
    int new_volume;

    if (state->duration_ms == 0 /* Fast path. */
    || state->start_volume == state->end_volume /* Fast path. */
    || state->step_ms >= state->duration_ms /* We're finished. */)
    {
        *volume = state->end_volume;
        state->step_ms = 0;
        return Fade_finished;
    }

    range = state->end_volume - state->start_volume;
    new_volume = range * state->step_ms / state->duration_ms;
    new_volume += state->start_volume;

    *volume = new_volume;

    state->step_ms += game.get_timestep();

    return Fade_continue;
}

void musicclass::fadeMusicVolumeIn(int ms)
{
    if (halted())
    {
        return;
    }

    m_doFadeInVol = true;
    m_doFadeOutVol = false;

    /* Ensure it starts at 0 */
    musicVolume = 0;

    /* Fix 1-frame glitch */
    song_volume = 0;

    fade.step_ms = 0;
    fade.duration_ms = ms;
    fade.start_volume = 0;
    fade.end_volume = VVV_MAX_VOLUME;
}

void musicclass::fadeMusicVolumeOut(const int fadeout_ms)
{
    if (halted())
    {
        return;
    }

    m_doFadeInVol = false;
    m_doFadeOutVol = true;

    fade.step_ms = 0;
    /* Duration is proportional to current volume. */
    fade.duration_ms = fadeout_ms * musicVolume / VVV_MAX_VOLUME;
    fade.start_volume = musicVolume;
    fade.end_volume = 0;
}

void musicclass::fadeout(const bool quick_fade_ /*= true*/)
{
    fadeMusicVolumeOut(quick_fade_ ? 500 : 2000);
    quick_fade = quick_fade_;
}

void musicclass::processmusicfadein(void)
{
    enum FadeCode fade_code = processmusicfade(&fade, &musicVolume);
    if (fade_code == Fade_finished)
    {
        m_doFadeInVol = false;
    }
}

void musicclass::processmusicfadeout(void)
{
    enum FadeCode fade_code = processmusicfade(&fade, &musicVolume);
    if (fade_code == Fade_finished)
    {
        musicVolume = 0;
        m_doFadeOutVol = false;
        haltdasmusik(true);
    }
}

void musicclass::processmusic(void)
{
    if(!safeToProcessMusic)
    {
        return;
    }

    if(m_doFadeInVol)
    {
        processmusicfadein();
    }

    if (m_doFadeOutVol)
    {
        processmusicfadeout();
    }

    /* This needs to come after processing fades */
    if (nicefade && halted())
    {
        play(nicechange);
        nicechange = -1;
        nicefade = false;
    }
}


void musicclass::niceplay(int t)
{
    /* important: do nothing if the correct song is playing! */
    if (currentsong != t)
    {
        if (currentsong != -1)
        {
            fadeout(false);
        }
        nicefade = true;
    }
    nicechange = t;
}

static const int areamap[] = {
    4, 3, 3, 3, 3, 3, 3, 3, 4,-2, 4, 4, 4,12,12,12,12,12,12,12,
    4, 3, 3, 3, 3, 3, 3, 4, 4,-2, 4, 4, 4, 4,12,12,12,12,12,12,
    4, 4, 4, 4, 3, 4, 4, 4, 4,-2, 4, 4, 4, 4,12,12,12,12,12,12,
    4, 4, 4, 4, 3, 4, 4, 4, 4,-2, 4, 4, 1, 1, 1, 1,12,12,12,12,
    4, 4, 3, 3, 3, 4, 4, 4, 4,-2,-2,-2, 1, 1, 1, 1, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4,-2, 1, 1, 1, 1, 1, 1,11,11,-1, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4,-2, 1, 1, 1, 1, 1, 1, 1,11,11,11,
    4, 4, 4, 4, 4, 4, 4, 4, 4,-2, 1, 1, 1, 1, 1, 1, 1, 1, 1,11,
    4, 4, 4, 4, 4, 4, 4, 4, 4,-2, 4, 4, 4, 1, 1, 1, 1, 1, 1, 3,
    4, 4, 4, 4, 4, 4, 4, 4,-2,-2, 4, 4, 4, 1, 1, 1, 1, 1, 1, 4,
    4, 4,-1,-1,-1, 4, 4, 4, 4,-2, 4, 4, 4, 1, 1, 1, 1, 1, 1, 4,
    4, 4,-1,-1,-1, 4, 4, 4, 4,-2, 4, 1, 1, 1, 1, 1, 1, 1, 1, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4,-2, 4, 1, 1, 1, 1, 1, 1, 4, 1, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4,-2, 4, 1, 1, 1, 1, 1, 1, 4, 1, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4,-2, 4,-1,-3, 4, 4, 4, 4, 4, 1, 4,
    4, 4, 4, 4, 4, 3, 3, 3, 4,-2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 3, 3, 3, 3, 3, 3, 4,-2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 3, 3, 3, 3, 3, 3, 3, 4,-2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 4, 4, 3, 4,-2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 4, 4, 3, 4,-2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

SDL_COMPILE_TIME_ASSERT(areamap, SDL_arraysize(areamap) == 20 * 20);

void musicclass::changemusicarea(int x, int y)
{
    int room;
    int track;

    if (script.running)
    {
        return;
    }

    room = musicroom(x, y);

    if (!INBOUNDS_ARR(room, areamap))
    {
        SDL_assert(0 && "Music map index out-of-bounds!");
        return;
    }

    track = areamap[room];

    switch (track)
    {
    case -1:
        /* Don't change music. */
        return;
    case -2:
        /* Special case: Tower music, changes with Flip Mode. */
        if (graphics.setflipmode)
        {
            track = 9; /* ecroF evitisoP */
        }
        else
        {
            track = 2; /* Positive Force */
        }
        break;
    case -3:
        /* Special case: start of Space Station 2. */
        if (game.intimetrial)
        {
            track = 1; /* Pushing Onwards */
        }
        else
        {
            track = 4; /* Passion for Exploring */
        }
        break;
    }

    niceplay(track);
}

void musicclass::playef(int t)
{
    if (t < 0 || t >= NUM_SOUNDS)
    {
        return;
    }
    sound_play(t);
}

void musicclass::pauseef(void)
{
    sounds_paused = true;
}

void musicclass::resumeef(void)
{
    sounds_paused = false;
}

bool musicclass::halted(void)
{
    return song_is_paused();
}

void musicclass::updatemutestate(void)
{
    /* the mute options are applied while mixing (vv_core_audio_render);
     * musicVolume follows the fades */
    song_volume = musicVolume;
}
