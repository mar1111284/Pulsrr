# Compiler
CC = gcc

# Debug flags
CFLAGS = -g -O0 -Wall `pkg-config --cflags gtk+-x11-3.0 sdl2 SDL2_image SDL2_ttf`
LIBS   = `pkg-config --libs gtk+-x11-3.0 sdl2 SDL2_image SDL2_ttf`

# Source directories
SRC_DIR = src
SDL_DIR = $(SRC_DIR)/sdl
MODALS_DIR = $(SRC_DIR)/modals
COMP_DIR = $(SRC_DIR)/components
UTILS_DIR = $(SRC_DIR)/utils

# Source files
SRCS = $(SRC_DIR)/main.c \
       $(SDL_DIR)/sdl.c \
       $(UTILS_DIR)/utils.c \
       $(COMP_DIR)/component_layer.c \
       $(COMP_DIR)/component_sequencer.c \
       $(COMP_DIR)/component_screen.c \
       $(COMP_DIR)/component_aphorism.c \
       $(MODALS_DIR)/modal_add_sequence.c \
       $(MODALS_DIR)/modal_download.c \
       $(MODALS_DIR)/modal_fx.c \
       $(MODALS_DIR)/modal_load_video.c \
       $(COMP_DIR)/component_layer.c \

# Object files (map .c to .o in build/)
BUILD_DIR = build
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Executable
TARGET = pulsrr

# Default target
all: $(BUILD_DIR) $(TARGET)

# Build executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean

