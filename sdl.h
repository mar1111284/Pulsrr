#ifndef SDL_H
#define SDL_H

#include <gtk/gtk.h>
#include <SDL2/SDL.h>

// Render state enum
typedef enum {
    RENDER_STATE_NO_FRAMES,
    RENDER_STATE_LOADING,
    RENDER_STATE_READY
} RenderState;

// Declare globals as extern
extern RenderState render_state;
extern int is_playing;

// Static inline setters/getters
static inline void sdl_set_loading(void)  { render_state = RENDER_STATE_LOADING; }
static inline void sdl_set_ready(void)    { render_state = RENDER_STATE_READY; }
static inline void sdl_set_playing(int p) { is_playing = p ? 1 : 0; }
static inline int  sdl_is_playing(void)   { return is_playing; }

void sdl_load_textures(void);

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

