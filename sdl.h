#ifndef SDL_H
#define SDL_H

#include <gtk/gtk.h>
#include <SDL2/SDL.h>

// Render & Layer States
typedef enum {
    RENDER_STATE_NO_FRAMES,
    RENDER_STATE_LOADING,
    RENDER_STATE_READY
} RenderState;

typedef enum {
    LAYER_EMPTY,
    LAYER_UP_TO_DATE,
    LAYER_MODIFIED
} LayerState;

// Global Variables
extern SDL_Renderer *renderer;
extern SDL_Window   *sdl_win;

extern double layer_speed[4];
extern Uint8  layer_alpha[4];
extern Uint8  layer_gray[4];
extern LayerState layer_state[4];

extern int is_playing;
extern RenderState render_state;

// Render State Accessors
RenderState get_render_state(void);
int sdl_is_initialized(void);

// SDL / GTK Integration
SDL_Renderer* sdl_get_renderer(void);
int  sdl_embed_in_gtk(GtkWidget *widget);
void sdl_draw_frame(void);
void sdl_restart(GtkWidget *widget);

// Texture Management
void start_load_textures_async(void);
void load_all_textures(void);
void sdl_update_textures(void);
void sdl_free_textures(void);

// Layer Controls
void set_transparency(guint8 layer_index, int alpha);
int  get_transparency(guint8 layer_index);

void set_gray(guint8 layer_index, int gray);
int  is_layer_gray(int layer_number);

#endif // SDL_H

