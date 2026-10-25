# Changelog

This file follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and
[Semantic Versioning](https://semver.org/spec/v2.0.0.html). Release tags must
match a section heading exactly (for example `v1.0.0`): CI uses the matching
section as the GitHub Release notes.

## [Unreleased]

## [v1.0.0] - 2026-10-25

### Added

- VVVVVV 2.4.2 (the full game, from its `data.zip`) as a Retro-Go SD GWHB
  homebrew, built on the VVVVVV source code and the retro-go-sd-templates
  SDK (firmware ABI v2, Retro-Go SD v2.0.0-rc1 or later), on a small SDL 2
  written for the console (software renderer straight into the LCD).
- Game data from a `.vvv` pack next to the `.bin`, used in place from the
  external flash (`vv_pack.py`, with `vv_render` for Windows, macOS and
  Linux): 8 bit pictures, the songs in IMA ADPCM mono 32 kHz (or 16 kHz),
  the rooms and scripts of the game (`vv_levels.bin`). About 44 MB.
- Savestates at the player's position (not during cutscenes), resume from
  the launcher.
- Pause menu options: music, sound effects, swap A/B.
