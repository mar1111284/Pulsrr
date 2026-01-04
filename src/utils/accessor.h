#ifndef ACCESSOR_H
#define ACCESSOR_H

#include "../sdl/sdl.h"
#include <glib.h>
#include <stdint.h>
#include <SDL2/SDL.h>


// Get current screen mode
ScreenMode sdl_get_screen_mode(void);
void sdl_set_screen_mode(ScreenMode mode);

// --- Playback ---
void sdl_set_playback_speed(double speed);

// --- Render accessors ---
RenderState sdl_get_render_state(void);
void sdl_set_render_state(RenderState state);

// --- Layer accessors ---
int sdl_get_alpha(uint8_t layer_index);
void sdl_set_layer_alpha(int layer_index, Uint8 alpha);

gboolean sdl_is_layer_gray(uint8_t layer_index);
void sdl_set_layer_grayscale(int layer_index, int grayscale);

double sdl_get_layer_speed(uint8_t layer_index);
void sdl_set_layer_speed(int layer_index, double speed);

LayerState sdl_get_layer_state(uint8_t layer_index);
void sdl_set_layer_modified(int layer_index);

void sdl_set_layer_state(guint8 layer_index, LayerState new_state);

#endif // ACCESSOR_H

