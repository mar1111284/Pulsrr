#ifndef SDL_H
#define SDL_H

#include <gtk/gtk.h>
#include <SDL2/SDL.h>

// Render & Layer States
typedef enum {
    LIVE_MODE,
    PLAYBACK_MODE
} ScreenMode;

typedef enum {
    RENDER_STATE_NO_FRAMES,
    RENDER_STATE_LOADING,
    RENDER_STATE_PLAY,
    RENDER_STATE_PAUSE,
} RenderState;

typedef enum {
    LAYER_EMPTY,
    LAYER_UP_TO_DATE,
    LAYER_MODIFIED
} LayerState;

// Core runtime
typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Surface  *surface;
    gboolean      initialized;
    RenderState   render_state;
    ScreenMode screen_mode;
    guint         draw_source_id;
    pthread_mutex_t mutex;
    struct Layer    *layers[4];
    struct Sequence *sequence;
} SDL;


// Layer specifications
typedef struct Layer {
    char *frame_folder;
    SDL_Surface **frames;
    SDL_Surface **frames_gray;
    SDL_Texture **textures;
    SDL_Texture **textures_gray;
    int     current_frame;
    int     frame_count;
    Uint32  last_tick;
    double  speed;
    int     fps;
    Uint8   alpha;
    int     grayscale;
    int     blend_mode;
    int     width;
    int     height;
    LayerState state;
} Layer;

// Sequence specifications
typedef struct Sequence {
    char *root_folder;
    SDL_Surface **frames;
    int           frame_count;
    SDL_Texture **textures;
    int     current_frame;
    Uint32  last_tick;
    int     fps;
} Sequence;

// SDL Core
int sdl_init(GtkWidget *widget);
int sdl_embed_in_gtk(GtkWidget *widget);

// Text rendering
void draw_centered_text(const char *text);

// Update textures
SDL_Surface* create_grayscale_surface(SDL_Surface *src);

// Gray surface 
SDL_Surface* create_grayscale_surface(SDL_Surface *src);
void* texture_update_thread(void *arg);
gboolean sdl_finalize_texture_update(gpointer data);
void update_textures_async(void);

// Init layers 
void init_layers();

// Setters
void sdl_set_render_state(RenderState state);
void sdl_set_layer_modified(int layer_index);
void sdl_set_layer_alpha(int layer_index, Uint8 alpha);
void sdl_set_layer_grayscale(int layer_index, int grayscale);
void sdl_set_layer_speed(int layer_index, double speed);

// Getters general 
RenderState sdl_get_render_state(void);
LayerState sdl_get_layer_state(guint8 layer_index);

// Getters FX
double sdl_get_layer_speed(guint8 layer_index);
int sdl_get_alpha(guint8 layer_index);
gboolean sdl_is_layer_gray(guint8 layer_index);

// Render Live
void sdl_render_live_mode(int advance_frames);

// Sequence
void free_sequence(Sequence *seq);
Sequence* update_sequence_texture();
void sdl_render_playback_mode(int advance_frames);

#endif // SDL_H

