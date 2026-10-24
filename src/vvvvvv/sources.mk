# VVVVVV 2.4.2 sources built for the Game & Watch (and the harness).
#
# VV_FILES: the game files built as they are (with -DGNW).
# VV_GEN_FILES: the files whose level / script tables go to the pack:
#   built from the copies tools/gen_levels.py writes to build/gen.
# Not built: Music.cpp, FileSystemUtils.cpp (replaced by src/port/
# vv_music_game.cpp, vv_fs.cpp), InterimVersion.in.c (template),
# LocalizationMaint.cpp, the network / Steam / GOG files, lodepng.

VV_FILES = \
	BinaryBlob.cpp BlockV.cpp ButtonGlyphs.cpp CWrappers.cpp CustomLevels.cpp \
	Editor.cpp Ent.cpp Entity.cpp Font.cpp FontBidi.cpp Game.cpp Graphics.cpp \
	GraphicsResources.cpp GraphicsUtil.cpp IMERender.cpp Input.cpp KeyPoll.cpp \
	LevelDebugger.cpp Localization.cpp LocalizationStorage.cpp Logic.cpp Map.cpp \
	Render.cpp RenderFixed.cpp RoomnameTranslator.cpp Screen.cpp Script.cpp \
	Textbox.cpp UtilityClass.cpp XMLUtils.cpp main.cpp preloader.cpp

VV_C_FILES = \
	DeferCallbacks.c GlitchrunnerMode.c Network.c Textbook.c UTF8.c VFormat.c \
	Vlogging.c Xoshiro.c

VV_GEN_FILES = \
	Otherlevel.cpp Spacestation2.cpp Labclass.cpp WarpClass.cpp Finalclass.cpp \
	Tower.cpp Scripts.cpp TerminalScripts.cpp

VV_THIRD_PARTY = tinyxml2/tinyxml2.cpp c-hashmap/map.c

# the port, built with the game (C++ / C)
VV_PORT_CXX = vv_stubs.cpp vv_core.cpp vv_state.cpp vv_fs.cpp vv_music_game.cpp vv_levels.cpp vv_cxx.cpp
VV_PORT_C = vv_sdl.c vv_alloc.c vv_pack.c vv_music.c
