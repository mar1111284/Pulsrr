#include "sdl.h"
#include "sdl_utilities.h"
#include <glib.h>
#include <SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "modal_add_sequence.h"

// State management
void sdl_set_loading(void)     { render_state = RENDER_STATE_LOADING; }
void sdl_set_ready(void)       { render_state = RENDER_STATE_READY; }
void sdl_set_no_frames(void)   { render_state = RENDER_STATE_NO_FRAMES; }
void sdl_set_playing(int p)    { is_playing = p ? 1 : 0; }
int  sdl_is_playing(void)      { return is_playing; }
RenderState get_render_state(void) { return render_state; }

// Layer control
void set_layer_speed(int layer_index, double speed) {
    if (layer_index < 0 || layer_index > 3) return;
    if (speed <= 0.0) speed = 0.01;
    layer_speed[layer_index] = speed;
}

double get_layer_speed(int layer_index) {
    if (layer_index < 0 || layer_index > 3) return 1.0;
    return layer_speed[layer_index];
}

void set_layer_modified(guint8 layer_index) {
    if (layer_index < 0 || layer_index > 3) return;
    layer_state[layer_index] = LAYER_MODIFIED;
}

void set_gray(guint8 layer_index, int gray) {
    if (layer_index >= 4) return;
    layer_gray[layer_index] = gray ? 1 : 0;
}

int is_layer_gray(int layer_index) {
    if (layer_index < 0 || layer_index >= MAX_LAYERS) return 0;
    return layer_gray[layer_index];
}

void set_transparency(guint8 layer_index, int alpha) {
    if (layer_index >= 4) return;
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    layer_alpha[layer_index] = (Uint8)alpha;
}

int get_transparency(guint8 layer_index) {
    if (layer_index >= 4) return 255;
    return (int)layer_alpha[layer_index];
}

// SDL access
SDL_Renderer* sdl_get_renderer() { return renderer; }

// Grayscale
void apply_grayscale(SDL_Surface *surface) {
    if (!surface) return;
    SDL_LockSurface(surface);
    Uint8 *pixels = (Uint8 *)surface->pixels;
    int pitch = surface->pitch;
    for (int y = 0; y < surface->h; y++) {
        Uint8 *row = pixels + y * pitch;
        for (int x = 0; x < surface->w; x++) {
            Uint8 *p = row + x * surface->format->BytesPerPixel;
            Uint8 r, g, b;
            SDL_GetRGB(*(Uint32 *)p, surface->format, &r, &g, &b);
            Uint8 gray = (Uint8)((r + g + b) / 3);
            *(Uint32 *)p = SDL_MapRGB(surface->format, gray, gray, gray);
        }
    }
    SDL_UnlockSurface(surface);
}

// Text rendering
void draw_centered_text(const char *text) {
    static TTF_Font *font = NULL;
    if (!font) {
        font = TTF_OpenFont("DS-TERM.TTF", 24);
        if (!font) return;
    }

    SDL_Color green = {0x33, 0xFF, 0x33, 255};
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, green);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    int tex_w = 0, tex_h = 0;
    SDL_QueryTexture(texture, NULL, NULL, &tex_w, &tex_h);

    int win_w = 0, win_h = 0;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    SDL_Rect dst = {(win_w - tex_w) / 2, (win_h - tex_h) / 2, tex_w, tex_h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Rendering screens
void render_loading_screen(void) {
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderClear(renderer);
    draw_centered_text("LOADING FRAMES…");
    SDL_RenderPresent(renderer);
}

void render_no_frames_screen(void) {
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderClear(renderer);
    draw_centered_text("NO FRAMES LOADED");
    SDL_RenderPresent(renderer);
}

// Restart SDL
void sdl_restart(GtkWidget *widget) {
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = NULL; }
    if (sdl_win) { SDL_DestroyWindow(sdl_win); sdl_win = NULL; }
    sdl_embed_in_gtk(widget);
}

// Video creation
void create_video_from_sequence(int duration, int width, int height, int sequence_number, AddSequenceUI *ui) {
    LayerSpecs layers[MAX_LAYERS] = {0};
    char folder_path[PATH_MAX];

    // Loading layers
    set_progress_add_sequence(ui, 0.30, "Loading layers...");
    for (int i = 0; i < MAX_LAYERS; i++) {
        layers[i].speed = get_layer_speed(i);
        layers[i].gray = is_layer_gray(i);
        layers[i].alpha = get_transparency(i);
        layers[i].current_frame = 0;
        layers[i].output_width = width;
        layers[i].output_height = height;
        layers[i].duration = duration;
        layers[i].fps = 25;

        snprintf(folder_path, sizeof(folder_path),
                 "sequences/sequence_%d/Frames_%d",
                 sequence_number, i + 1);

        layers[i].frame_folder = g_strdup(folder_path);
        layers[i].total_frames = count_frames(layers[i].frame_folder);
    }

    // Load frames
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (layers[i].total_frames == 0) {
            layers[i].frames = layers[i].frames_gray = NULL;
            add_log(ui, g_strdup_printf("[LOAD] Layer %d has no frames, skipped", i + 1));
            continue;
        }

        layers[i].frames = malloc(sizeof(SDL_Surface*) * layers[i].total_frames);
        layers[i].frames_gray = malloc(sizeof(SDL_Surface*) * layers[i].total_frames);

        for (int f = 0; f < layers[i].total_frames; f++) {
            char filename[PATH_MAX];
            snprintf(filename, sizeof(filename),
                     "%s/frame_%05d.png",
                     layers[i].frame_folder, f + 1);

            SDL_Surface *surf = IMG_Load(filename);
            if (!surf) { layers[i].frames[f] = layers[i].frames_gray[f] = NULL; continue; }

            layers[i].frames[f] = surf;

            SDL_Surface *surf_gray = SDL_ConvertSurface(surf, surf->format, 0);
            apply_grayscale(surf_gray);
            layers[i].frames_gray[f] = surf_gray;
        }

        add_log(ui, g_strdup_printf("[LOAD] Layer %d completed (%d frames)", i + 1, layers[i].total_frames));
    }

    // Mixing frames
    char mixed_dir[PATH_MAX];
    snprintf(mixed_dir, sizeof(mixed_dir),
             "sequences/sequence_%d/mixed_frames",
             sequence_number);
    ensure_dir(mixed_dir);

    int total_output_frames = duration * layers[0].fps;
    set_progress_add_sequence(ui, 0.50, "Mixing frames...");
    int last_reported = -1;

    for (int f = 0; f < total_output_frames; f++) {
        SDL_Surface *mixed = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        SDL_FillRect(mixed, NULL, SDL_MapRGBA(mixed->format, 0, 0, 0, 255));

        for (int l = 0; l < MAX_LAYERS; l++) {
            if (layers[l].total_frames == 0) continue;

            int frame_idx;
            if (layers[l].speed >= 1.0)
                frame_idx = (int)(f * layers[l].speed) % layers[l].total_frames;
            else {
                int repeat = (int)(1.0 / layers[l].speed + 0.5);
                frame_idx = (f / repeat) % layers[l].total_frames;
            }

            SDL_Surface *src = layers[l].gray ? layers[l].frames_gray[frame_idx] : layers[l].frames[frame_idx];
            if (!src) continue;

            SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_BLEND);
            SDL_SetSurfaceAlphaMod(src, layers[l].alpha);

            SDL_Rect dest = {0, 0, width, height};
            SDL_BlitScaled(src, NULL, mixed, &dest);
        }

        gchar *frame_name = g_strdup_printf("frame_%05d.png", f + 1);
        gchar *filename = g_build_filename(mixed_dir, frame_name, NULL);
        IMG_SavePNG(mixed, filename);
        g_free(frame_name);
        g_free(filename);
        SDL_FreeSurface(mixed);

        int percent = (f * 100) / total_output_frames;
        if (percent >= 50 && last_reported < 50) last_reported = 50;
    }

    // Encoding
    set_progress_add_sequence(ui, 0.85, "Encoding video...");
    encode_sequence_with_ffmpeg(sequence_number, 30, width, height);
    set_progress_add_sequence(ui, 0.95, "Encoding completed");

    // Cleanup
    for (int i = 0; i < MAX_LAYERS; i++) {
        for (int f = 0; f < layers[i].total_frames; f++) {
            if (layers[i].frames[f]) SDL_FreeSurface(layers[i].frames[f]);
            if (layers[i].frames_gray[f]) SDL_FreeSurface(layers[i].frames_gray[f]);
        }
        free(layers[i].frames);
        free(layers[i].frames_gray);
        g_free(layers[i].frame_folder);
    }
}

