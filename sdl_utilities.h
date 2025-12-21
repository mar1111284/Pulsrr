#ifndef SDL_UTILITIES_H
#define SDL_UTILITIES_H

#include "sdl.h"
#include <SDL2/SDL.h>

void sdl_set_loading(void);
void sdl_set_ready(void);
void sdl_set_playing(int p);
int  sdl_is_playing(void);
RenderState get_render_state(void);

// Getters & Setters
void set_layer_speed(int layer_index, double speed);
double get_layer_speed(int layer_index);
void set_gray(guint8 layer_index, int gray);
int is_layer_gray(int layer_number);
SDL_Renderer* sdl_get_renderer();
void set_transparency(guint8 layer_index, int alpha);
int get_transparency(guint8 layer_index);
void apply_grayscale(SDL_Surface *surface);
void set_layer_modified(guint8 layer_index);
void draw_centered_text(const char *text);
void render_loading_screen(void);
void render_no_frames_screen(void);
void sdl_set_no_frames(void);
void sdl_restart(GtkWidget *widget);

#endif

