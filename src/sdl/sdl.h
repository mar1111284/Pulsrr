#ifndef SDL_H
#define SDL_H

#include <gtk/gtk.h>
#include <SDL2/SDL.h>
#include "../utils/utils.h"

// Render & Layer States
typedef enum {
    LIVE_MODE,
    PLAYBACK_MODE
} ScreenMode;

typedef enum {
    RENDER_STATE_LOADING,
    RENDER_STATE_PLAY,
    RENDER_STATE_PAUSE,
    RENDER_STATE_IDLE,
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
    double accumulated_delta;
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
    double speed;
    double accumulated_delta;
} Sequence;

extern SDL g_sdl;

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

// Render Live
void sdl_render_live_mode(int advance_frames);

// Sequence
void free_sequence(Sequence *seq);
Sequence* update_sequence_texture();
void sdl_render_playback_mode(int advance_frames);

// Utils
bool sdl_has_live_texture(void);
bool sdl_has_sequence_texture(void);


#endif // SDL_H

