#ifndef SDL_H
#define SDL_H

#include <gtk/gtk.h>
#include <SDL2/SDL.h>

SDL_Renderer* sdl_get_renderer(void);
int  sdl_embed_in_gtk(GtkWidget *widget);
void sdl_draw_frame(void);
void sdl_free_textures(void);
void sdl_restart(GtkWidget *widget);

/* --- playback control --- */
void sdl_set_playing(int playing);
int  sdl_is_playing(void);

#endif

