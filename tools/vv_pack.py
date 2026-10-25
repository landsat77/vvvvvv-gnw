#!/usr/bin/env python3
"""Build the game data pack of the VVVVVV homebrew.

    python3 vv_pack.py VVVVVV.vvv /path/to/data.zip [--music CHOICE]

CHOICE: the music in the pack (asked when the option is not given):
    ogg    the original songs, not converted (Ogg Vorbis 44.1 kHz, decoded by
           the console while they play): best sound, pack of ~66 MB
    32000  converted to IMA ADPCM mono 32 kHz (default): ~44 MB
    22050  converted to IMA ADPCM mono 22 kHz: ~31 MB
    11025  converted to IMA ADPCM mono 11 kHz: ~17 MB
    none   no music: ~3.5 MB

data.zip is the data file of VVVVVV 2.x (the full game: Steam, GOG,
itch.io... next to the game's executable; on macOS inside VVVVVV.app,
Contents/Resources). An unzipped data.zip folder works too.

The console reads the pack in place from its external flash, so the files
are converted for it on this computer (by vv_render: vv_render/ next to this
script, or tools/bin of the repository):
  - the pictures (graphics/*.png) to 8 bit images with a palette;
  - the 16 songs of vvvvvvmusic.vvv (Ogg Vorbis, stereo, 44.1 kHz) to mono
    samples in IMA ADPCM (the console has one speaker), unless --music ogg;
  - the sound effects (sounds/*.wav) to mono samples at the console's
    output rate (44.1 kHz with the original songs, 32 kHz, 22 kHz with the
    22 and 11 kHz songs);
and the pack gets the files of this release: vv_levels.bin (the rooms and
scripts of the game, taken out of its code), font.png / font.fontmeta (the
font of VVVVVV 2.4).

The player levels of data.zip (levels/) are not used. Pack format:
src/port/vv_pack.c. Only Python 3 is needed (no extra module).
"""
import argparse
import os
import platform
import shutil
import struct
import subprocess
import sys
import tempfile
import zipfile

MAGIC = b"GWVV01\0\0"
ENTRY_NAME_LEN = 40
# --music: (rate of the songs (None: the original Ogg Vorbis files), output
# rate of the console = rate of the sound effects, menu text)
MUSIC_CHOICES = {
    "ogg": (None, 44100, "original Ogg Vorbis songs, not converted (best sound, ~66 MB)"),
    "32000": (32000, 32000, "IMA ADPCM mono 32 kHz (~44 MB)"),
    "22050": (22050, 22050, "IMA ADPCM mono 22 kHz (~31 MB)"),
    "11025": (11025, 22050, "IMA ADPCM mono 11 kHz (~17 MB)"),
    "none": (None, 32000, "no music (~3.5 MB)"),
}
MUSIC_ORDER = ["ogg", "32000", "22050", "11025", "none"]
DEFAULT_MUSIC = "32000"
# the external flash of the console must hold the pack (and the firmware)
FLASH_WARNING_BYTES = 55 * 1000 * 1000


def ask_music():
    """The music choice, asked on the terminal."""
    print("Music of the pack:")
    for i, key in enumerate(MUSIC_ORDER, 1):
        print("  %d. %s%s" % (i, MUSIC_CHOICES[key][2], "  [default]" if key == DEFAULT_MUSIC else ""))
    while True:
        try:
            answer = input("Choice (1-%d, Enter = %d): " % (len(MUSIC_ORDER), MUSIC_ORDER.index(DEFAULT_MUSIC) + 1)).strip()
        except EOFError:
            return DEFAULT_MUSIC
        if not answer:
            return DEFAULT_MUSIC
        if answer.isdigit() and 1 <= int(answer) <= len(MUSIC_ORDER):
            return MUSIC_ORDER[int(answer) - 1]
        if answer in MUSIC_CHOICES:
            return answer

# the songs, in the order of vvvvvvmusic.vvv (Music.h: Music_PATHCOMPLETE...)
SONGS = [
    "0levelcomplete", "1pushingonwards", "2positiveforce", "3potentialforanything",
    "4passionforexploring", "5intermission", "6presentingvvvvvv", "7gamecomplete",
    "8predestinedfate", "9positiveforcereversed", "10popularpotpourri", "11pipedream",
    "12pressurecooker", "13pacedenergy", "14piercingthesky", "predestinedfatefinallevel",
]
REQUIRED = ["graphics/tiles.png", "graphics/sprites.png", "sounds/jump.wav", "vvvvvvmusic.vvv"]


class Source:
    """data.zip, or the folder it was unzipped to."""

    def __init__(self, path):
        self.zip = None
        self.root = None
        if os.path.isdir(path):
            self.root = path
            if not os.path.exists(os.path.join(path, "graphics")):
                for sub in ("data", "VVVVVV"):
                    if os.path.isdir(os.path.join(path, sub, "graphics")):
                        self.root = os.path.join(path, sub)
        elif zipfile.is_zipfile(path):
            self.zip = zipfile.ZipFile(path)
        else:
            sys.exit("error: %s is not data.zip or a folder" % path)

    def names(self):
        if self.zip:
            return [n for n in self.zip.namelist() if not n.endswith("/")]
        out = []
        for d, _, files in os.walk(self.root):
            for f in files:
                out.append(os.path.relpath(os.path.join(d, f), self.root).replace(os.sep, "/"))
        return out

    def read(self, name):
        if self.zip:
            return self.zip.read(name)
        with open(os.path.join(self.root, name), "rb") as fh:
            return fh.read()


def split_music_blob(blob):
    """vvvvvvmusic.vvv (BinaryBlob.cpp): 128 headers {char name[48]; int
    start (unused); int size; int valid (1 byte + padding)}, then the
    files one after the other."""
    count, head = 128, 60
    if len(blob) < count * head:
        sys.exit("error: vvvvvvmusic.vvv is too short")
    pos = count * head
    songs = {}
    for i in range(count):
        name = blob[i * head:i * head + 48].split(b"\0")[0].decode("latin-1")
        size, valid = struct.unpack_from("<iB", blob, i * head + 52)
        if not name or not valid or size <= 0:
            continue
        songs[name.rsplit("/", 1)[-1]] = blob[pos:pos + size]
        pos += size
    return songs


def write_pack(out_path, files, verbose=True):
    header_size = 16 + len(files) * (ENTRY_NAME_LEN + 8)
    offset = (header_size + 3) & ~3
    entries = []
    for name, blob in files:
        entries.append((name, offset, len(blob)))
        offset = (offset + len(blob) + 3) & ~3
    total = offset
    out = bytearray(total)
    out[0:8] = MAGIC
    struct.pack_into("<II", out, 8, len(files), total)
    for i, (name, off, size) in enumerate(entries):
        nb = name.encode("ascii")
        assert len(nb) < ENTRY_NAME_LEN, name
        struct.pack_into("<%dsII" % ENTRY_NAME_LEN, out, 16 + i * (ENTRY_NAME_LEN + 8), nb, off, size)
    for (name, blob), (_, off, size) in zip(files, entries):
        out[off:off + size] = blob
    with open(out_path, "wb") as fh:
        fh.write(out)
    if verbose:
        print("wrote %s: %d files, %.1f MB" % (out_path, len(files), total / 1e6))
    return total


def here():
    return os.path.dirname(os.path.abspath(__file__))


def default_renderer():
    """vv_render-<os>-<arch> in vv_render/ next to this script (release zip)
    or in tools/bin/ (tools/build_renderers.sh)."""
    system = {"Linux": "linux", "Windows": "windows", "Darwin": "macos"}.get(platform.system())
    machine = platform.machine().lower()
    arch = "arm64" if machine in ("arm64", "aarch64") else "x86_64"
    candidates = []
    if system:
        names = ["vv_render-%s-%s%s" % (system, arch, ".exe" if system == "windows" else "")]
        if system == "windows" and arch == "arm64":  # x86_64 emulation
            names.append("vv_render-windows-x86_64.exe")
        for n in names:
            candidates += [os.path.join(here(), "vv_render", n), os.path.join(here(), "bin", n),
                           os.path.join(here(), n)]
    for p in candidates:
        if os.path.isfile(p):
            prepare_executable(p)
            return p
    return None


def prepare_executable(path):
    """Make a renderer from a zip file runnable (exec bit, macOS quarantine)."""
    if os.name != "nt" and not os.access(path, os.X_OK):
        try:
            os.chmod(path, os.stat(path).st_mode | 0o111)
        except OSError:
            pass
    if platform.system() == "Darwin":
        subprocess.run(["xattr", "-d", "com.apple.quarantine", path],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def find_release_file(name, explicit=None):
    """A file of the release next to this script (vv_levels.bin, font.png...):
    in the script's folder, its assets/ folder, or the build folders of the
    repository."""
    if explicit:
        return explicit
    for p in (os.path.join(here(), name), os.path.join(here(), "assets", name),
              os.path.join(here(), "..", "build", name), os.path.join(here(), "..", "build", "homebrew", name)):
        if os.path.isfile(p):
            return p
    return None


class Converter:
    def __init__(self, renderer, tmp, quiet):
        self.renderer = renderer
        self.tmp = tmp
        self.quiet = quiet
        self.n = 0

    def run(self, kind, data, ext, *extra):
        self.n += 1
        src = os.path.join(self.tmp, "in%d%s" % (self.n, ext))
        dst = os.path.join(self.tmp, "out%d" % self.n)
        with open(src, "wb") as fh:
            fh.write(data)
        r = subprocess.run([self.renderer, kind, src, dst] + [str(x) for x in extra],
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        if r.returncode != 0:
            sys.stderr.write(r.stdout[-4000:])
            sys.exit("error: vv_render %s failed (%s)" % (kind, self.renderer))
        with open(dst, "rb") as fh:
            out = fh.read()
        os.remove(src)
        os.remove(dst)
        return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("output", help="pack to write (same name as the .bin, extension .vvv)")
    ap.add_argument("data", help="data.zip of VVVVVV 2.x (or the folder it was unzipped to)")
    ap.add_argument("--music", choices=MUSIC_ORDER,
                    help="music of the pack: ogg (original files, ~66 MB), 32000 (default, ~44 MB), "
                         "22050 (~31 MB), 11025 (~17 MB) or none (~3.5 MB); asked when not given")
    ap.add_argument("--no-music", action="store_true", help="same as --music none")
    ap.add_argument("--renderer", help="vv_render program (vv_render/ next to this script by default)")
    ap.add_argument("--levels", help="vv_levels.bin of the release (next to this script by default)")
    ap.add_argument("--font", help="folder with font.png and font.fontmeta (next to this script by default)")
    ap.add_argument("-q", "--quiet", action="store_true")
    args = ap.parse_args()

    music = "none" if args.no_music else args.music
    if music is None:
        music = ask_music() if sys.stdin.isatty() else DEFAULT_MUSIC
    song_rate, out_rate, music_text = MUSIC_CHOICES[music]
    if not args.quiet:
        print("music: %s" % music_text)

    src = Source(args.data)
    names = src.names()
    lower = {n.lower(): n for n in names}
    missing = [n for n in REQUIRED if n not in lower]
    if missing:
        sys.exit("error: missing game data: %s (is this the data.zip of VVVVVV 2.x?)" % ", ".join(missing))

    renderer = args.renderer or default_renderer()
    if not renderer or not os.path.isfile(renderer):
        sys.exit("error: vv_render not found (vv_render/ next to this script, or --renderer)")
    levels = find_release_file("vv_levels.bin", args.levels)
    if not levels:
        sys.exit("error: vv_levels.bin not found (it comes with this script)")
    font_dir = args.font
    if not font_dir:
        f = find_release_file("font.png")
        font_dir = os.path.dirname(f) if f else None

    files = []
    tmp = tempfile.mkdtemp(prefix="vv_pack_")
    try:
        conv = Converter(renderer, tmp, args.quiet)
        say = (lambda *a: None) if args.quiet else print

        # pictures (the font of the release replaces the one of data.zip:
        # VVVVVV 2.4 has more characters in it)
        font_png = None
        if font_dir and os.path.isfile(os.path.join(font_dir, "font.png")):
            with open(os.path.join(font_dir, "font.png"), "rb") as fh:
                font_png = fh.read()
            with open(os.path.join(font_dir, "font.fontmeta"), "rb") as fh:
                files.append(("graphics/font.fontmeta", fh.read()))
        n_img = 0
        for low, real in sorted(lower.items()):
            if low.startswith("graphics/") and low.endswith(".png"):
                data = font_png if (low == "graphics/font.png" and font_png) else src.read(real)
                files.append((low, conv.run("image", data, ".png")))
                n_img += 1
        say("pictures: %d" % n_img)

        # sound effects
        size = 0
        for low, real in sorted(lower.items()):
            if low.startswith("sounds/") and low.endswith(".wav"):
                out = conv.run("sound", src.read(real), ".wav", out_rate)
                files.append((low, out))
                size += len(out)
        say("sounds: %d, %d Hz, %.1f MB" % (sum(1 for n, _ in files if n.startswith("sounds/")), out_rate, size / 1e6))

        # music
        if music != "none":
            songs = split_music_blob(src.read(lower["vvvvvvmusic.vvv"]))
            size = 0
            for i, name in enumerate(SONGS):
                ogg = songs.get(name + ".ogg")
                if ogg is None:
                    sys.exit("error: vvvvvvmusic.vvv has no %s.ogg" % name)
                if song_rate is None:
                    # as it is: the console decodes it (vv_ogg.c), at its
                    # rate (the output rate: 44.1 kHz)
                    files.append(("music/%s.ogg" % name, ogg))
                    size += len(ogg)
                    continue
                out = conv.run("music", ogg, ".ogg", song_rate, "adpcm")
                files.append(("music/%s.mus" % name, out))
                size += len(out)
                say("music %2d/%d: %s" % (i + 1, len(SONGS), name))
            if song_rate is None:
                say("music: %d songs, Ogg Vorbis (not converted), %.1f MB" % (len(SONGS), size / 1e6))
            else:
                say("music: %d songs, IMA ADPCM %d Hz, %.1f MB" % (len(SONGS), song_rate, size / 1e6))

        with open(levels, "rb") as fh:
            files.append(("vv_levels.bin", fh.read()))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    total = write_pack(args.output, sorted(files), verbose=not args.quiet)
    if total > FLASH_WARNING_BYTES:
        print("warning: the pack is %.1f MB: it may not fit in the external flash of the console "
              "(--music 32000, 22050 or 11025 make it smaller)" % (total / 1e6))


if __name__ == "__main__":
    main()
