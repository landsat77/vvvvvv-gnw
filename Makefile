# VVVVVV for Game & Watch Retro-Go SD — GWHB homebrew.
#
#   make                  — build + pack → VVVVVV.bin (and build/homebrew/vv_levels.bin)
#   make docker           — same build inside the Retro-Go SD builder image
#   make harness          — headless Linux test harness (harness/vv_host.c)
#
# SD card (Retro-Go SD v2.0.0 or later):
#   /homebrews/VVVVVV.bin (this homebrew)
#   /homebrews/VVVVVV.vvv (game data: tools/vv_pack.py, same name)
#
# Verbose compiler lines: make V=

#######################################
# Project identity
#######################################
PROJECT_KIND := homebrew

CORE_NAME  := vv
CORE_ENTRY := app_main

HB_NAME    := VVVVVV
PACKED_BIN := VVVVVV.bin

VV_SRC  := src/vvvvvv
VV_PORT := src/port
VV_TP   := src/third_party
BUILD_DIR ?= build/$(PROJECT_KIND)
VV_GEN  := $(BUILD_DIR)/gen
LEVELS_BIN := $(BUILD_DIR)/vv_levels.bin

include $(VV_SRC)/sources.mk

# The game (C++), its level / script files without their tables
# (tools/gen_levels.py), the port's C++ side
VV_GEN_SOURCES := $(addprefix $(VV_GEN)/,$(VV_GEN_FILES))
CORE_CXX_SOURCES := $(addprefix $(VV_SRC)/,$(VV_FILES)) $(VV_GEN_SOURCES) \
	$(VV_TP)/tinyxml2/tinyxml2.cpp $(addprefix $(VV_PORT)/,$(VV_PORT_CXX))

# The game's C files, c-hashmap, the port's C side, the retro-go side
CORE_C_SOURCES := $(addprefix $(VV_SRC)/,$(VV_C_FILES)) $(VV_TP)/c-hashmap/map.c \
	$(addprefix $(VV_PORT)/,$(VV_PORT_C)) \
	$(VV_PORT)/vv_libc.c \
	$(VV_PORT)/vv_main.c

CORE_C_INCLUDES := -I$(VV_PORT) -I$(VV_PORT)/sdl -I$(VV_SRC) -I$(VV_TP) -I$(VV_TP)/tinyxml2 -I$(VV_GEN)

CORE_C_DEFS := -DPROJECT_KIND_HOMEBREW=1 -DVV_DEVICE=1 -DGNW -DNDEBUG

OPT := -Os

# libstdc++ (std::string, std::vector, std::map), its calls to the C
# library renamed like ours (below)
CORE_LDLIBS := -L$(BUILD_DIR) -lgnwstdcxx -lgcc

GNW_CORE_SDK ?= sdk

include $(GNW_CORE_SDK)/Makefile

# The SDK's C++ runtime has a bump allocator without free for operator new:
# vv_cxx.cpp replaces it (built, not linked).
CXX_OBJECTS := $(filter-out $(BUILD_DIR)/gw_core_cxx_support.o,$(CXX_OBJECTS))

#######################################
# VVVVVV specific flags
#######################################
VV_WARN := -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function \
	-Wno-unused-parameter -Wno-missing-braces -Wno-format-truncation -Wno-sign-compare \
	-Wno-class-memaccess
CXXFLAGS += -std=gnu++17 $(VV_WARN)
CFLAGS += $(VV_WARN)
GLUE := -include $(VV_PORT)/gnw_vv_glue.h
GLUED_OBJECTS := $(addprefix $(BUILD_DIR)/,$(VV_C_FILES:.c=.o) map.o Font.o)
$(GLUED_OBJECTS): CFLAGS += $(GLUE)
$(GLUED_OBJECTS): CXXFLAGS += $(GLUE)
$(C_OBJECTS) $(CXX_OBJECTS): Makefile $(VV_GEN)/vv_levels_gen.h

# the level / script tables -> vv_levels.bin (goes in the pack), the
# sources without them
$(VV_GEN)/vv_levels_gen.h $(VV_GEN_SOURCES) $(LEVELS_BIN): tools/gen_levels.py $(addprefix $(VV_SRC)/,$(VV_GEN_FILES))
	$(V)mkdir -p $(VV_GEN)
	$(V)python3 tools/gen_levels.py --out-dir $(VV_GEN) --blob $(LEVELS_BIN) \
		--header $(VV_GEN)/vv_levels_gen.h $(addprefix $(VV_SRC)/,$(VV_GEN_FILES))

# libstdc++ of the toolchain with its C library calls renamed like the
# objects of the core (memcpy -> core_memcpy...: the firmware's)
LIBSTDCXX := $(shell $(CXX) $(MCU) -print-file-name=libstdc++_nano.a)
$(BUILD_DIR)/libgnwstdcxx.a: $(LIBSTDCXX) $(REDEFINE_SYMS_FILE) | $(BUILD_DIR)
	$(V)$(ECHO) "[ LIB ] libstdc++ (renamed C library calls)"
	$(V)$(CP) --redefine-syms=$(REDEFINE_SYMS_FILE) $(LIBSTDCXX) $@
$(TARGET_ELF): $(BUILD_DIR)/libgnwstdcxx.a

#######################################
# Packed header version
#######################################
CORE_VERSION ?= $(shell git describe --tags --dirty 2>/dev/null || echo NOTAG)

#######################################
# Pack (GWHB)
#######################################
PACK_HOMEBREW := $(GNW_CORE_SDK)/tools/pack_homebrew.py
MAKE_COVER    := scripts/make_cover.py
COVER_SRC     := src/assets/cover.png
COVER_JPG     := $(BUILD_DIR)/cover.jpg
COVER_ARG     := $(if $(wildcard $(COVER_SRC)),--cover $(COVER_JPG),)

.PHONY: pack cover
cover: $(COVER_JPG)

# The cover art (src/assets/cover.png). Must stay <= 186x100 and <= 10 KiB
# (firmware cover cache).
$(COVER_JPG): $(MAKE_COVER) $(COVER_SRC) | $(BUILD_DIR)
	$(V)$(ECHO) "[ COVER ] $(COVER_JPG)"
	$(V)python3 $(MAKE_COVER) --in $(COVER_SRC) --out $(COVER_JPG)

pack: $(TARGET_BIN) $(if $(wildcard $(COVER_SRC)),$(COVER_JPG),) $(LEVELS_BIN)
	$(V)$(ECHO) [ PACK GWHB ] $(PACKED_BIN) version=$(CORE_VERSION)
	$(V)python3 $(PACK_HOMEBREW) \
		--elf $(TARGET_ELF) --bin $(TARGET_BIN) \
		--name "$(HB_NAME)" --version "$(CORE_VERSION)" \
		$(COVER_ARG) \
		--out $(PACKED_BIN)

all: pack

clean::
	$(V)rm -f $(PACKED_BIN) $(COVER_JPG)

#######################################
# Harness
#######################################
.PHONY: harness
harness:
	$(MAKE) -C harness

# Read-only helpers for CI / scripts (make print-PACKED_BIN, etc.).
.PHONY: print-PROJECT_KIND print-PACKED_BIN print-CORE_NAME print-DOCKER_IMAGE \
	print-TARGET_ELF print-TARGET_MAP print-CORE_VERSION
print-PROJECT_KIND:
	@echo $(PROJECT_KIND)
print-PACKED_BIN:
	@echo $(PACKED_BIN)
print-CORE_NAME:
	@echo $(CORE_NAME)
print-DOCKER_IMAGE:
	@echo $(DOCKER_IMAGE)
print-TARGET_ELF:
	@echo $(TARGET_ELF)
print-TARGET_MAP:
	@echo $(BUILD_DIR)/$(CORE_NAME)_core.map
print-CORE_VERSION:
	@echo $(CORE_VERSION)
#######################################
# Docker (same image as the firmware repo)
#######################################
.PHONY: docker docker_pull docker_shell

RELEASE_VERSION ?= v1.5
DOCKER_REPOSITORY ?= sylverb/retro-go-sd-builder
DOCKER_IMAGE ?= $(DOCKER_REPOSITORY):$(RELEASE_VERSION)

DOCKER_TTY_FLAG := $(shell if [ -t 0 ]; then echo -it; else echo; fi)
DOCKER_USER := $(shell id -u):$(shell id -g)
DOCKER_RUN := docker run --rm $(DOCKER_TTY_FLAG) \
	--user $(DOCKER_USER) \
	-v "$(CURDIR):/opt/workdir" \
	-w /opt/workdir \
	$(DOCKER_IMAGE)

docker:
	$(V)$(ECHO) "[ DOCKER ]" $(DOCKER_IMAGE)
	$(V)$(DOCKER_RUN) make --no-print-directory -j$$(nproc)

docker_pull:
	$(V)docker pull $(DOCKER_IMAGE)

docker_shell:
	$(DOCKER_RUN) bash
