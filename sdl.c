#include "sdl.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>
#include <gdk/gdkx.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include "utils.h"
#include "sdl_utilities.h"

// Globals
static int frame[4] = {1, 1, 1, 1};
static int total_frames[4] = {0, 0, 0, 0};
static char frame_pattern[4][512] = {
    "Frames_1/frame_%05d.png",
    "Frames_2/frame_%05d.png",
    "Frames_3/frame_%05d.png",
    "Frames_4/frame_%05d.png"
};

static SDL_Texture **textures[4] = {NULL, NULL, NULL, NULL};
static SDL_Texture **textures_gray[4] = {NULL, NULL, NULL, NULL};

Uint8 layer_alpha[4] = {255, 128, 128, 128};
Uint8 layer_gray[4] = {0, 0, 0, 0};
double layer_speed[4] = {1, 1.0, 1.0, 1.0};
Uint32 last_update[4] = {0, 0, 0, 0};
static int sdl_initialized = 0; 

SDL_Window   *sdl_win = NULL;
SDL_Renderer *renderer = NULL;
LayerState layer_state[4] = {LAYER_EMPTY, LAYER_EMPTY, LAYER_EMPTY, LAYER_EMPTY};
int is_playing = 0;
RenderState render_state = RENDER_STATE_NO_FRAMES;

// Helpers
int sdl_is_initialized(void) {
    return sdl_initialized;
}

static SDL_Texture* load_texture(const char *filename, int gray) {
    SDL_Surface *surf = IMG_Load(filename);
    if (!surf) return NULL;

    if (gray) {
        SDL_Surface *surf_gray = SDL_ConvertSurface(surf, surf->format, 0);
        apply_grayscale(surf_gray);
        SDL_Texture *tex_gray = SDL_CreateTextureFromSurface(renderer, surf_gray);
        SDL_SetTextureBlendMode(tex_gray, SDL_BLENDMODE_BLEND);
        SDL_FreeSurface(surf_gray);
        SDL_FreeSurface(surf);
        return tex_gray;
    } else {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_FreeSurface(surf);
        return tex;
    }
}

static void free_layer_textures(SDL_Texture **tex, SDL_Texture **tex_gray, int total) {
    if (!tex) return;
    for (int i = 0; i < total; i++) {
        if (tex[i]) SDL_DestroyTexture(tex[i]);
        if (tex_gray && tex_gray[i]) SDL_DestroyTexture(tex_gray[i]);
    }
    free(tex);
    free(tex_gray);
}

// Texture Loading
static void* load_textures_thread(void* arg) {
    if (count_frames("Frames_1") == 0 &&
        count_frames("Frames_2") == 0 &&
        count_frames("Frames_3") == 0 &&
        count_frames("Frames_4") == 0) {
        sdl_set_no_frames();
        return NULL;
    }

    load_all_textures();
    return NULL;
}


void start_load_textures_async() {
    pthread_t tid;
    pthread_create(&tid, NULL, load_textures_thread, NULL);
    pthread_detach(tid);
}

void load_all_textures() {
    sdl_set_loading();

    for (int layer = 0; layer < 4; layer++) {
        if (layer_state[layer] == LAYER_UP_TO_DATE || layer_state[layer] == LAYER_EMPTY) continue;

        free_layer_textures(textures[layer], textures_gray[layer], total_frames[layer]);
        total_frames[layer] = count_frames(g_strdup_printf("Frames_%d", layer + 1));
        textures[layer] = malloc(total_frames[layer] * sizeof(SDL_Texture*));
        textures_gray[layer] = malloc(total_frames[layer] * sizeof(SDL_Texture*));

        for (int i = 0; i < total_frames[layer]; i++) {
            char filename[512];
            snprintf(filename, sizeof(filename), frame_pattern[layer], i + 1);
            textures[layer][i] = load_texture(filename, 0);
            textures_gray[layer][i] = load_texture(filename, 1);

            if (!textures[layer][i]) {
                add_main_log(g_strdup_printf("[ERROR] Failed frame %d (%s)", i + 1, filename));
                textures[layer][i] = NULL;
            }
        }
        layer_state[layer] = LAYER_UP_TO_DATE;
    }

    sdl_set_ready();
    add_main_log("[INFO] All textures loaded.");
}

// SDL Drawing
void sdl_draw_frame() {
    if (!renderer) return;

    switch (render_state) {
        case RENDER_STATE_NO_FRAMES: render_no_frames_screen(); return;
        case RENDER_STATE_LOADING:   render_loading_screen(); return;
        default: break;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int layer = 0; layer < 4; layer++) {
        if (total_frames[layer] == 0) continue;
        SDL_Texture *tex = layer_gray[layer] ? textures_gray[layer][frame[layer] - 1] : textures[layer][frame[layer] - 1];
        if (tex) {
            SDL_SetTextureAlphaMod(tex, layer_alpha[layer]);
            SDL_RenderCopy(renderer, tex, NULL, NULL);
        }
    }

    SDL_RenderPresent(renderer);
    if (!is_playing) return;

    Uint32 now = SDL_GetTicks();
    Uint32 base_delay = 1000 / 30;

    for (int layer = 0; layer < 4; layer++) {
        if (layer_speed[layer] >= 1.0) {
            frame[layer] += (int)layer_speed[layer];
            if (frame[layer] > total_frames[layer]) frame[layer] = 1;
        } else if (now - last_update[layer] >= (Uint32)(base_delay / layer_speed[layer])) {
            frame[layer]++;
            if (frame[layer] > total_frames[layer]) frame[layer] = 1;
            last_update[layer] = now;
        }
    }
}

// SDL Embed
int sdl_embed_in_gtk(GtkWidget *widget) {
    sdl_set_playing(!sdl_is_playing());
    TTF_Init();

    for (int i = 0; i < 4; i++) textures[i] = textures_gray[i] = NULL;

    GdkWindow *gdk = gtk_widget_get_window(widget);
    Window xid = GDK_WINDOW_XID(gdk);

    SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "0");
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        add_main_log(g_strdup_printf("[SDL] Init failed: %s", SDL_GetError()));
        return 0;
    }

    sdl_win = SDL_CreateWindowFrom((void *)(uintptr_t)xid);
    if (!sdl_win) {
        add_main_log(g_strdup_printf("[SDL] CreateWindowFrom failed: %s", SDL_GetError()));
        return 0;
    }

    renderer = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        add_main_log(g_strdup_printf("[SDL] CreateRenderer failed: %s", SDL_GetError()));
        return 0;
    }

    for (int i = 0; i < 4; i++)
        total_frames[i] = count_frames(g_strdup_printf("Frames_%d", i + 1));

    int all_empty = 1;
    for (int i = 0; i < 4; i++) if (total_frames[i] > 0) all_empty = 0;

	if (all_empty)
		sdl_set_no_frames();
	else {
		sdl_set_loading();
		start_load_textures_async();
	}

    g_timeout_add(16, (GSourceFunc)sdl_draw_frame, NULL); // ~60 FPS
    add_main_log("[INFO] SDL doing well.");
    return 1;
}

