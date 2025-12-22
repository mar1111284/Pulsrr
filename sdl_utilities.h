#ifndef SDL_UTILITIES_H
#define SDL_UTILITIES_H

#include "utils.h"
#include "sdl.h"
#include "modal_add_sequence.h"
#include <SDL2/SDL.h>

// Layer specifications
typedef struct {
    double speed;
    int gray;
    int alpha;
    int total_frames;
    int current_frame;
    int output_width;
    int output_height;
    int duration;
    int fps;
    char *frame_folder;
    SDL_Surface **frames;
    SDL_Surface **frames_gray;
    int blend_mode;
} LayerSpecs;

// Render state
void sdl_set_loading(void);
void sdl_set_ready(void);
void sdl_set_playing(int p);
int  sdl_is_playing(void);
RenderState get_render_state(void);

// Layer control
void set_layer_speed(int layer_index, double speed);
double get_layer_speed(int layer_index);
void set_gray(guint8 layer_index, int gray);
int is_layer_gray(int layer_number);
SDL_Renderer* sdl_get_renderer();
void set_transparency(guint8 layer_index, int alpha);
int get_transparency(guint8 layer_index);
void set_layer_modified(guint8 layer_index);

// SDL & rendering
void apply_grayscale(SDL_Surface *surface);
void draw_centered_text(const char *text);
void render_loading_screen(void);
void render_no_frames_screen(void);
void sdl_set_no_frames(void);
void sdl_restart(GtkWidget *widget);

// Video creation
void create_video_from_sequence(int duration, int width, int height, int sequence_number, AddSequenceUI *ui);

#endif

