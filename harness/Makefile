# VVVVVV — headless Linux harness
#
#   make            → build/vv_host (and build/vv_levels.bin)
#   make SAN=1      → with AddressSanitizer + UBSan
#
#   ./build/vv_host VVVVVV.vvv -t 20000 -e 1000 -o /tmp/vv

BUILD_DIR = build
TARGET = $(BUILD_DIR)/vv_host
GEN = $(BUILD_DIR)/gen

VV = ../src/vvvvvv
PORT = ../src/port
TP = ../src/third_party

include $(VV)/sources.mk

SAN ?= 0
OPT ?= -O2 -g
ifeq ($(SAN),1)
OPT += -fsanitize=address,undefined -fno-omit-frame-pointer -DVV_HOST_SYSTEM_MALLOC
LDFLAGS += -fsanitize=address,undefined
endif

INCLUDES = -I$(PORT) -I$(PORT)/sdl -I$(VV) -I$(TP) -I$(TP)/tinyxml2 -I$(GEN)
WARN = -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function \
	-Wno-missing-braces -Wno-unused-parameter -Wno-class-memaccess -Wno-format-truncation
DEFS = -DGNW -DNDEBUG
CFLAGS = $(OPT) -MMD -std=gnu11 $(DEFS) $(WARN) $(INCLUDES)
CXXFLAGS = $(OPT) -MMD -std=gnu++17 -fno-exceptions -fno-rtti $(DEFS) $(WARN) $(INCLUDES)
GLUE = -include $(PORT)/gnw_vv_glue.h

VV_OBJECTS = $(addprefix $(BUILD_DIR)/vv_,$(VV_FILES:.cpp=.o))
VV_C_OBJECTS = $(addprefix $(BUILD_DIR)/vvc_,$(VV_C_FILES:.c=.o))
GEN_SOURCES = $(addprefix $(GEN)/,$(VV_GEN_FILES))
GEN_OBJECTS = $(addprefix $(BUILD_DIR)/gen_,$(VV_GEN_FILES:.cpp=.o))
TP_OBJECTS = $(BUILD_DIR)/tp_tinyxml2.o $(BUILD_DIR)/tp_map.o
PORT_OBJECTS = $(addprefix $(BUILD_DIR)/p_,$(VV_PORT_CXX:.cpp=.o) $(VV_PORT_C:.c=.o)) $(BUILD_DIR)/p_vv_host.o

all: $(TARGET)

$(BUILD_DIR) $(GEN):
	mkdir -p $@

# the level / script tables -> vv_levels.bin, the sources without them
$(GEN)/vv_levels_gen.h $(GEN_SOURCES) $(BUILD_DIR)/vv_levels.bin: ../tools/gen_levels.py $(addprefix $(VV)/,$(VV_GEN_FILES)) | $(GEN)
	python3 ../tools/gen_levels.py --out-dir $(GEN) --blob $(BUILD_DIR)/vv_levels.bin \
		--header $(GEN)/vv_levels_gen.h $(addprefix $(VV)/,$(VV_GEN_FILES))

$(VV_OBJECTS) $(GEN_OBJECTS) $(PORT_OBJECTS) $(VV_C_OBJECTS): $(GEN)/vv_levels_gen.h

$(BUILD_DIR)/vv_Font.o: $(VV)/Font.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) $(GLUE) $< -o $@

$(BUILD_DIR)/vv_%.o: $(VV)/%.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/vvc_%.o: $(VV)/%.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(GLUE) $< -o $@

$(BUILD_DIR)/gen_%.o: $(GEN)/%.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(VV) $< -o $@

$(BUILD_DIR)/tp_tinyxml2.o: $(TP)/tinyxml2/tinyxml2.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/tp_map.o: $(TP)/c-hashmap/map.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(GLUE) $< -o $@

$(BUILD_DIR)/p_%.o: $(PORT)/%.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/p_%.o: $(PORT)/%.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

# stb_vorbis allocates from the game heap, like on the console
$(BUILD_DIR)/p_vv_ogg.o: $(PORT)/vv_ogg.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(GLUE) -w $< -o $@

$(BUILD_DIR)/p_vv_host.o: vv_host.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(VV_OBJECTS) $(VV_C_OBJECTS) $(GEN_OBJECTS) $(TP_OBJECTS) $(PORT_OBJECTS)
	$(CXX) $^ $(LDFLAGS) -lm -o $@

-include $(wildcard $(BUILD_DIR)/*.d)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
