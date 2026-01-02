CC = gcc

# Debug flags
CFLAGS = -g -O0 -Wall `pkg-config --cflags gtk+-x11-3.0 sdl2 SDL2_image SDL2_ttf`
LIBS   = `pkg-config --libs gtk+-x11-3.0 sdl2 SDL2_image SDL2_ttf`

all: pulsrr

pulsrr: main.c sdl.c sdl_utilities.c modal_add_sequence.c modal_add_sequence.h modal_download.c modal_download.h sdl_utilities.h utils.c layer.c modal_load.c modal_fx.c screen_panel.c screen_panel.h aphorism.c sequencer.c sequencer.h aphorism.h modal_fx.h sdl.h utils.h layer.h modal_load.h
	$(CC) $(CFLAGS) main.c sdl.c utils.c layer.c sdl_utilities.c modal_load.c modal_add_sequence.c modal_download.c screen_panel.c sequencer.c modal_fx.c aphorism.c -o pulsrr $(LIBS)

clean:
	rm -f pulsrr

