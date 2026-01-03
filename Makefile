# Compiler
CC = gcc

# Directories
SRC_DIR = src
SDL_DIR = $(SRC_DIR)/sdl
MODALS_DIR = $(SRC_DIR)/modals
COMP_DIR = $(SRC_DIR)/components
UTILS_DIR = $(SRC_DIR)/utils

BUILD_DIR = build
TARGET = pulsrr

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SDL_DIR)/sdl.c \
       $(UTILS_DIR)/utils.c \
       $(UTILS_DIR)/accessor.c \
       $(COMP_DIR)/component_layer.c \
       $(COMP_DIR)/component_sequencer.c \
       $(COMP_DIR)/component_screen.c \
       $(COMP_DIR)/component_aphorism.c \
       $(MODALS_DIR)/modal_add_sequence.c \
       $(MODALS_DIR)/modal_download.c \
       $(MODALS_DIR)/modal_fx.c \
       $(MODALS_DIR)/modal_load_video.c

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# pkg-config dependencies
PKG_DEPS = gtk+-x11-3.0 sdl2 SDL2_image SDL2_ttf
PKG_CFLAGS = $(shell pkg-config --cflags $(PKG_DEPS))
PKG_LIBS   = $(shell pkg-config --libs $(PKG_DEPS))

# Extra libraries (pthread and math are sometimes needed explicitly)
EXTRA_LIBS = -lpthread -lm

# === Build Modes ===

# Default debug build
CFLAGS  = -g -O0 -Wall -Wextra $(PKG_CFLAGS)
LDFLAGS = $(PKG_LIBS) $(EXTRA_LIBS)

# Profile build — best for perf
profile: CFLAGS = -g -O2 -fno-omit-frame-pointer -DNDEBUG $(PKG_CFLAGS)
profile: LDFLAGS = $(PKG_LIBS) $(EXTRA_LIBS)
profile: clean $(TARGET)
	$(info === Built in PROFILE mode — ready for perf ===)

# Release build — maximum performance
release: CFLAGS = -O3 -DNDEBUG -march=native -flto $(PKG_CFLAGS)
release: LDFLAGS = $(PKG_LIBS) $(EXTRA_LIBS) -flto
release: clean $(TARGET)
	$(info === Built in RELEASE mode ===)

# Valgrind-friendly debug
valgrind: CFLAGS = -g -O0 -Wall -Wextra $(PKG_CFLAGS)
valgrind: LDFLAGS = $(PKG_LIBS) $(EXTRA_LIBS)
valgrind: clean $(TARGET)
	$(info === Built for Valgrind (full debug symbols) ===)

# === Rules ===

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile with automatic dependency generation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Include dependency files
-include $(OBJS:.o=.d)

# Convenience targets
run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean profile release valgrind run
