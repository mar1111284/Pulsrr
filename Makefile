CC = gcc

# Debug flags: -g = symbols, -O0 = no optimization, -Wall = warnings
CFLAGS = -g -O0 -Wall `pkg-config --cflags gtk+-3.0 sdl2 SDL2_image SDL2_ttf`
LIBS   = `pkg-config --libs gtk+-3.0 sdl2 SDL2_image SDL2_ttf`

all: pulsrr

pulsrr: main.c sdl.c utils.c sdl.h utils.h
	$(CC) $(CFLAGS) main.c sdl.c utils.c -o pulsrr $(LIBS)

clean:
	rm -f pulsrr

