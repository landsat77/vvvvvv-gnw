# VVVVVV for Game & Watch Retro-Go SD

Terry Cavanagh's **VVVVVV** (2.4.2) running on the Nintendo Game & Watch
(Mario / Zelda) as a **Retro-Go SD homebrew** (`/homebrews/VVVVVV.bin`,
firmware v2.0.0-rc1 or later).

This is a **modified version** of VVVVVV, made from the
[VVVVVV source code](https://github.com/TerryCavanagh/VVVVVV) (tag 2.4.2,
`desktop_version/src`) for the console's 1 MB of RAM and 280 MHz Cortex-M7.
VVVVVV is the copyright of Terry Cavanagh, and this port is based on the
VVVVVV source code. It is not an official release, and it is not
supported by its authors.

**The game data is not included.** You need the full game, VVVVVV 2.x:
its `data.zip` (Steam, GOG, itch.io, the Humble version...). VVVVVV is a
commercial game: if you enjoy it, buy it at
[thelettervsixtim.es](https://thelettervsixtim.es).

## Installation

See [INSTALL.md](INSTALL.md) (English and French). In short:

1. Build the data pack from `data.zip` (Python 3 only):

   ```
   python3 vv_pack.py VVVVVV.vvv /path/to/data.zip
   ```

   On Windows: `py vv_pack.py VVVVVV.vvv "C:\Program Files (x86)\Steam\steamapps\common\VVVVVV\data.zip"`

2. Copy both files to the SD card, side by side, with the same name:

   ```
   /homebrews/VVVVVV.bin
   /homebrews/VVVVVV.vvv
   ```

3. Start "VVVVVV" from the Homebrew tab. The first start copies the pack
   to the external flash of the console (a few minutes: 44 MB).

### What vv_pack.py does

The console reads the pack in place from its external flash, so the game's
files are converted for it on your computer by `vv_render` (shipped for
Windows, macOS and Linux in `vv_render/` next to the script):

- the pictures to 8 bit images with a palette;
- the 16 songs (Ogg Vorbis, stereo, 44.1 kHz: 62 MB) and the sound effects
  to mono samples (the console has one speaker), the songs in IMA ADPCM.

The pack also gets `vv_levels.bin`, the rooms and the scripts of the game
(taken out of its source code: on the console the code runs from RAM), and
the font of VVVVVV 2.4.

| Option | Pack size | Music |
|---|---|---|
| *(default)* | ~44 MB | IMA ADPCM, 32 kHz |
| `--music-rate 16000` | ~24 MB | IMA ADPCM, 16 kHz |
| `--no-music` | ~3.5 MB | none |

## Controls

| Game & Watch | In the game | In the menus |
|---|---|---|
| D-pad | Move | Move |
| A | Flip, action (talk, advance the text) | Select |
| B | Flip | Back |
| TIME (SELECT on Zelda units) | Map, teleporters, terminals (Enter) | — |
| GAME (START on Zelda units) | Pause menu (Esc) | Back |
| PAUSE/SET | Retro-Go menu | Retro-Go menu |

The Retro-Go menu adds savestates and, in *Options*: music, sound effects,
"Swap A/B".

**Savestates** save the game where the player stands (the game's own saves
go back to the last checkpoint). They are refused during a cutscene. The
game's own saves work too (teleporters, the "save" of the map menu).

## What is different

- English only. The game's options for the screen, the game pads and the
  languages are greyed out; so is "levels": no player levels and no level
  editor on the console (`data.zip`'s `levels/` folder is not used).
- 30 frames per second, the game's speed (no interpolated frames in
  between).
- The menus slide over a copy of the game in 256 colours.

## Building

Needs `arm-none-eabi-gcc` (hard-float, v10+, with its `libstdc++_nano`),
GNU Make, Python 3 (+ Pillow for the cover):

```
make            # → VVVVVV.bin and build/homebrew/vv_levels.bin
make docker     # same, inside the Retro-Go SD builder image
```

`vv_render` (tools/render) is cross-built with zig:

```
pip install ziglang
tools/build_renderers.sh    # → tools/bin/vv_render-<os>-<arch>
```

### Testing without hardware

`harness/` builds the same game and port code for Linux, headless, with a
virtual clock, scripted buttons, PPM screenshots and WAV output:

```
make -C harness
python3 tools/vv_pack.py VVVVVV.vvv data.zip --renderer tools/bin/vv_render-linux-x86_64 \
    --levels harness/build/vv_levels.bin --font tools/assets
harness/build/vv_host VVVVVV.vvv -t 30000 -e 1000 -o /tmp/out -w /tmp/out/a.wav
harness/build/vv_host VVVVVV.vvv -s "3000:A:1,3100:A:0,6000:A:1,6100:A:0" -S 8000 -o /tmp/out
```

`-q file -Q ms` writes a savestate, `-j file -J ms` loads one, `-L file`
loads one at start. `VV_HOST_HEAP_KB` sets the heap. `make -C harness
SAN=1` builds it with AddressSanitizer and UBSan. The pack is mapped
read-only, like the flash.

## Layout

| Path | Role |
|------|------|
| `src/vvvvvv/` | VVVVVV 2.4.2 (`desktop_version/src` @ 149f5fb). Game & Watch changes are under `#ifdef GNW`. |
| `tools/gen_levels.py` | Takes the room / script tables out of 8 source files at build time: `vv_levels.bin` + the sources without them. |
| `src/port/sdl/`, `vv_sdl.c` | The small SDL 2: a software renderer drawing straight into the LCD, images in place from the pack, keys from the buttons. |
| `src/port/vv_music_game.cpp`, `vv_music.c` | Music.cpp of the port: songs and sound effects from the pack, mixed at 32 kHz. |
| `src/port/vv_fs.cpp` | FileSystemUtils.cpp of the port: assets in the pack, saves on the SD card. |
| `src/port/vv_core.cpp`, `vv_state.cpp` | The game run for the platform, buttons → keys, savestates. |
| `src/port/vv_main.c` | Retro-Go side: entry point, display, audio, buttons, pause menu. |
| `tools/vv_pack.py`, `tools/render/` | The `.vvv` pack and its converter (`vv_render`: stb_image, stb_vorbis). |
| `harness/` | Headless test harness. |
| `sdk/` | Retro-Go SD core / homebrew SDK (from retro-go-sd-templates). |

See `CLAUDE.md` for the internals.

## License

- The VVVVVV source code (`src/vvvvvv/`, modified) and `tools/assets/`
  (the font of the VVVVVV repository) are under the **VVVVVV Source Code
  License v1.0** ([LICENSE.md](LICENSE.md)): free, non-commercial use only;
  this is an altered version and is marked as such. VVVVVV's graphics and
  music are proprietary and are not in this repository nor in its
  releases: the pack is made on your computer from your `data.zip`.
- The port (`src/port/`, `tools/`, `harness/`) is under the same terms.
- `sdk/` is MIT (`sdk/LICENSE.MIT`), tinyxml2 is zlib, c-hashmap is
  BSD-3-Clause (their licenses are next to them), stb_image / stb_vorbis
  are public domain.
