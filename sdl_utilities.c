#include "sdl.h"
#include "sdl_utilities.h"
#include <glib.h>
#include <SDL_ttf.h>

void sdl_set_loading(void)  { render_state = RENDER_STATE_LOADING; }
void sdl_set_ready(void)    { render_state = RENDER_STATE_READY; }
void sdl_set_no_frames(void) { render_state = RENDER_STATE_NO_FRAMES; }
void sdl_set_playing(int p) { is_playing = p ? 1 : 0; }
int  sdl_is_playing(void)   { return is_playing; }
RenderState get_render_state(void) { return render_state; }

void set_layer_speed(int layer_index, double speed) {
    if (layer_index < 0 || layer_index > 3) return;
    if (speed <= 0.0) speed = 0.01;
    layer_speed[layer_index] = speed;
    g_print("[SDL] Layer %d speed set to %.2f\n", layer_index + 1, speed);
}

void set_layer_modified(guint8 layer_index) {
    if (layer_index < 0 || layer_index > 3) return;
    layer_state[layer_index] = LAYER_MODIFIED;
    g_print("[GTK] Layer %d set to MODIFIED\n", layer_index + 1 );
}

double get_layer_speed(int layer_index) {
    if (layer_index < 0 || layer_index > 3) return 1.0;
    return layer_speed[layer_index];
}

void set_gray(guint8 layer_index, int gray) {
    if (layer_index >= 4) return;
    layer_gray[layer_index] = gray ? 1 : 0;
    g_print("Layer %d grayscale: %s\n", layer_index + 1, gray ? "ON" : "OFF");
}

int is_layer_gray(int layer_number) {
    if (layer_number < 1 || layer_number > 4) return 0;
    return layer_gray[layer_number - 1];
}

SDL_Renderer* sdl_get_renderer() {
    return renderer;
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

void draw_centered_text(const char *text) {
    static TTF_Font *font = NULL;

    if (!font) {
        font = TTF_OpenFont("DS-TERM.TTF", 24);
        if (!font) {
            printf("TTF_OpenFont error: %s\n", TTF_GetError());
            return;
        }
    }

    SDL_Color green = { 0x33, 0xFF, 0x33, 255 };

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

    SDL_Rect dst = {
        (win_w - tex_w) / 2,
        (win_h - tex_h) / 2,
        tex_w,
        tex_h
    };

    SDL_RenderCopy(renderer, texture, NULL, &dst);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

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

void sdl_restart(GtkWidget *widget) {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (sdl_win) {
        SDL_DestroyWindow(sdl_win);
        sdl_win = NULL;
    }

    sdl_embed_in_gtk(widget);
}

