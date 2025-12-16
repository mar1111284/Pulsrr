#ifndef SDL_H
#define SDL_H

#include <gtk/gtk.h>
#include <SDL2/SDL.h>

SDL_Renderer* sdl_get_renderer(void);
int  sdl_embed_in_gtk(GtkWidget *widget);
void sdl_draw_frame(void);
void sdl_free_textures(void);
void sdl_restart(GtkWidget *widget);
// Set the alpha transparency for a given layer (1–4)
void set_transparency(int layer_number, int alpha);
void set_gray(int layer_number, int gray);
int is_layer_gray(int layer_number);
void sdl_update_textures();


/* --- playback control --- */
void sdl_set_playing(int playing);
int  sdl_is_playing(void);

#endif

