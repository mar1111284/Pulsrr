#include "sdl.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>
#include <gdk/gdkx.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include "utils.h"
#include <pthread.h>
#include "sdl_utilities.h"

// GLobals
static int frame_1 = 1;
static int frame_2 = 1;
static int frame_3 = 1;
static int frame_4 = 1;

static int total_frames_1 = 0;  
static int total_frames_2 = 0;
static int total_frames_3 = 0;  
static int total_frames_4 = 0;

static char frame_1_pattern[512] = "Frames_1/frame_%05d.png";
static char frame_2_pattern[512] = "Frames_2/frame_%05d.png";
static char frame_3_pattern[512] = "Frames_3/frame_%05d.png";
static char frame_4_pattern[512] = "Frames_4/frame_%05d.png";

static SDL_Texture **textures_1 = NULL;
static SDL_Texture **textures_2 = NULL;
static SDL_Texture **textures_3 = NULL;
static SDL_Texture **textures_4 = NULL;

static SDL_Texture **textures_1_gray = NULL;
static SDL_Texture **textures_2_gray = NULL;
static SDL_Texture **textures_3_gray = NULL;
static SDL_Texture **textures_4_gray = NULL;

Uint8 layer_alpha[4] = {255, 128, 128, 128};
Uint8 layer_gray[4] = {0, 0, 0, 0};
double layer_speed[4] = {1, 1.0, 1.0, 1.0};
Uint32 last_update[4] = {0, 0, 0, 0};

SDL_Window   *sdl_win = NULL;
SDL_Renderer *renderer = NULL;

LayerState layer_state[4] = {LAYER_EMPTY, LAYER_EMPTY, LAYER_EMPTY, LAYER_EMPTY};
int is_playing = 0;
RenderState render_state = RENDER_STATE_NO_FRAMES;

void* load_textures_thread(void* arg) {
    load_all_textures();
    return NULL;
}

void start_load_textures_async() {
    pthread_t tid;
    pthread_create(&tid, NULL, load_textures_thread, NULL);
    pthread_detach(tid);
}

static void handle_sdl_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_p) {
                is_playing = !is_playing;
                g_print("▶️ Play = %d\n", is_playing);
            }
        }
    }
}

static void free_layer(SDL_Texture **tex, SDL_Texture **tex_gray, int total) {
    if (!tex) return;

    for (int i = 0; i < total; i++) {
        if (tex[i]) SDL_DestroyTexture(tex[i]);
        if (tex_gray && tex_gray[i]) SDL_DestroyTexture(tex_gray[i]);
    }

    free(tex);
    free(tex_gray);
}

static SDL_Texture* load_frame_texture(const char *filename) {
	SDL_Surface *surf = IMG_Load(filename);
	if (!surf) {
	g_print("⚠️ Failed to load %s\n", filename);
	return NULL;
	}
	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	SDL_FreeSurface(surf);
	return tex;
}

static SDL_Texture* load_frame_texture_gray(const char *filename) {
	SDL_Surface *surf = IMG_Load(filename);
	if (!surf) {
	g_print("⚠️ Failed to load %s\n", filename);
	return NULL;
	}

	// Apply grayscale on a copy
	SDL_Surface *surf_gray = SDL_ConvertSurface(surf, surf->format, 0);
	apply_grayscale(surf_gray);

	// Create texture from grayscale surface
	SDL_Texture *tex_gray = SDL_CreateTextureFromSurface(renderer, surf_gray);
	SDL_SetTextureBlendMode(tex_gray, SDL_BLENDMODE_BLEND);

	SDL_FreeSurface(surf_gray);
	SDL_FreeSurface(surf);

	return tex_gray;
}

// Draw frames with independent looping
void sdl_draw_frame() {
	if (!renderer) return;
	
	if (render_state == RENDER_STATE_NO_FRAMES) {
		render_no_frames_screen();
		return;
	}
	
	if (render_state == RENDER_STATE_LOADING) {
		render_loading_screen();
		return;
	}

	handle_sdl_events();

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// --- Video 1 (bottom layer) ---
	if (total_frames_1 > 0) {
	    SDL_Texture *tex = layer_gray[0] ? textures_1_gray[frame_1 - 1] : textures_1[frame_1 - 1];
	    if (tex) {
		SDL_SetTextureAlphaMod(tex, layer_alpha[0]);
		SDL_RenderCopy(renderer, tex, NULL, NULL);
	    }
	}

	// --- Video 2 ---
	if (total_frames_2 > 0) {
	    SDL_Texture *tex = layer_gray[1] ? textures_2_gray[frame_2 - 1] : textures_2[frame_2 - 1];
	    if (tex) {
		SDL_SetTextureAlphaMod(tex, layer_alpha[1]);
		SDL_RenderCopy(renderer, tex, NULL, NULL);
	    }
	}

	// --- Video 3 ---
	if (total_frames_3 > 0) {
	    SDL_Texture *tex = layer_gray[2] ? textures_3_gray[frame_3 - 1] : textures_3[frame_3 - 1];
	    if (tex) {
		SDL_SetTextureAlphaMod(tex, layer_alpha[2]);
		SDL_RenderCopy(renderer, tex, NULL, NULL);
	    }
	}

	// --- Video 4 ---
	if (total_frames_4 > 0) {
	    SDL_Texture *tex = layer_gray[3] ? textures_4_gray[frame_4 - 1] : textures_4[frame_4 - 1];
	    if (tex) {
		SDL_SetTextureAlphaMod(tex, layer_alpha[3]);
		SDL_RenderCopy(renderer, tex, NULL, NULL);
	    }
	}


	SDL_RenderPresent(renderer);

	if (!is_playing) return;

	Uint32 now = SDL_GetTicks();
	Uint32 base_delay = 1000 / 25; // 25 FPS default

	// --- Layer 1 ---
	if (layer_speed[0] >= 1.0) {
	    frame_1 += (int)layer_speed[0];
	    if (frame_1 > total_frames_1) frame_1 = 1;
	} else if (now - last_update[0] >= (Uint32)(base_delay / layer_speed[0])) {
	    frame_1++;
	    if (frame_1 > total_frames_1) frame_1 = 1;
	    last_update[0] = now;
	}

	// --- Layer 2 ---
	if (layer_speed[1] >= 1.0) {
	    frame_2 += (int)layer_speed[1];
	    if (frame_2 > total_frames_2) frame_2 = 1;
	} else if (now - last_update[1] >= (Uint32)(base_delay / layer_speed[1])) {
	    frame_2++;
	    if (frame_2 > total_frames_2) frame_2 = 1;
	    last_update[1] = now;
	}

	// --- Layer 3 ---
	if (layer_speed[2] >= 1.0) {
	    frame_3 += (int)layer_speed[2];
	    if (frame_3 > total_frames_3) frame_3 = 1;
	} else if (now - last_update[2] >= (Uint32)(base_delay / layer_speed[2])) {
	    frame_3++;
	    if (frame_3 > total_frames_3) frame_3 = 1;
	    last_update[2] = now;
	}

	// --- Layer 4 ---
	if (layer_speed[3] >= 1.0) {
	    frame_4 += (int)layer_speed[3];
	    if (frame_4 > total_frames_4) frame_4 = 1;
	} else if (now - last_update[3] >= (Uint32)(base_delay / layer_speed[3])) {
	    frame_4++;
	    if (frame_4 > total_frames_4) frame_4 = 1;
	    last_update[3] = now;
	}

}

void load_all_textures() {

    sdl_set_loading();
    
	if (layer_state[0] == LAYER_EMPTY &&
	    layer_state[1] == LAYER_EMPTY &&
	    layer_state[2] == LAYER_EMPTY &&
	    layer_state[3] == LAYER_EMPTY) {
		void sdl_set_no_frames(void);
	    return;
	} else {
	    sdl_set_loading();
	}

    // Reset frame counters
    frame_1 = frame_2 = frame_3 = frame_4 = 1;

    // --- Video 1 ---
    if (layer_state[0] != LAYER_UP_TO_DATE && layer_state[0] != LAYER_EMPTY) {
        free_layer(textures_1, textures_1_gray, total_frames_1);
        total_frames_1 = count_frames("Frames_1"); 
    	textures_1 = malloc(total_frames_1 * sizeof(SDL_Texture*));
    	textures_1_gray = malloc(total_frames_1 * sizeof(SDL_Texture*));
        for (int i = 0; i < total_frames_1; i++) {
            char filename[512];
            snprintf(filename, sizeof(filename), frame_1_pattern, i + 1);
            textures_1[i] = load_frame_texture(filename);
            textures_1_gray[i] = load_frame_texture_gray(filename);
            if (!textures_1[i]) {
                g_print("⚠️ Failed to load frame %d of video 1 (%s)\n", i + 1, filename);
                textures_1[i] = NULL;
            } else {
                g_print("[LOAD_TEXTURES] video1 frame %d loaded\n", i + 1);
            }
        }
        layer_state[0] = LAYER_UP_TO_DATE;
    }

    // --- Video 2 ---
    if (layer_state[1] != LAYER_UP_TO_DATE && layer_state[1] != LAYER_EMPTY) {
        free_layer(textures_2, textures_2_gray, total_frames_2);
        total_frames_2 = count_frames("Frames_2");
	textures_2 = malloc(total_frames_2 * sizeof(SDL_Texture*));
	textures_2_gray = malloc(total_frames_2 * sizeof(SDL_Texture*));
        for (int i = 0; i < total_frames_2; i++) {
            char filename[512];
            snprintf(filename, sizeof(filename), frame_2_pattern, i + 1);
            textures_2[i] = load_frame_texture(filename);
            textures_2_gray[i] = load_frame_texture_gray(filename);
            if (!textures_2[i]) {
                g_print("⚠️ Failed to load frame %d of video 2 (%s)\n", i + 1, filename);
                textures_2[i] = NULL;
            } else {
                g_print("[LOAD_TEXTURES] video2 frame %d loaded\n", i + 1);
            }
        }
        layer_state[1] = LAYER_UP_TO_DATE;
    }

    // --- Video 3 ---
    if (layer_state[2] != LAYER_UP_TO_DATE && layer_state[2] != LAYER_EMPTY) {
        free_layer(textures_3, textures_3_gray, total_frames_3);
        total_frames_3 = count_frames("Frames_3");
	textures_3 = malloc(total_frames_3 * sizeof(SDL_Texture*));
	textures_3_gray = malloc(total_frames_3 * sizeof(SDL_Texture*));
        for (int i = 0; i < total_frames_3; i++) {
            char filename[512];
            snprintf(filename, sizeof(filename), frame_3_pattern, i + 1);
            textures_3[i] = load_frame_texture(filename);
            textures_3_gray[i] = load_frame_texture_gray(filename);
            if (!textures_3[i]) {
                g_print("⚠️ Failed to load frame %d of video 3 (%s)\n", i + 1, filename);
                textures_3[i] = NULL;
            } else {
                g_print("[LOAD_TEXTURES] video3 frame %d loaded\n", i + 1);
            }
        }
        layer_state[2] = LAYER_UP_TO_DATE;
    }

    // --- Video 4 ---
    if (layer_state[3] != LAYER_UP_TO_DATE && layer_state[3] != LAYER_EMPTY) {
        free_layer(textures_4, textures_4_gray, total_frames_4);
        total_frames_4 = count_frames("Frames_4");
	textures_4 = malloc(total_frames_4 * sizeof(SDL_Texture*));
	textures_4_gray = malloc(total_frames_4 * sizeof(SDL_Texture*));
        for (int i = 0; i < total_frames_4; i++) {
            char filename[512];
            snprintf(filename, sizeof(filename), frame_4_pattern, i + 1);
            textures_4[i] = load_frame_texture(filename);
            textures_4_gray[i] = load_frame_texture_gray(filename);
            if (!textures_4[i]) {
                g_print("⚠️ Failed to load frame %d of video 4 (%s)\n", i + 1, filename);
                textures_4[i] = NULL;
            } else {
                g_print("[LOAD_TEXTURES] video4 frame %d loaded\n", i + 1);
            }
        }
        layer_state[3] = LAYER_UP_TO_DATE;
    }

    sdl_set_ready();
    g_print("[LOAD_TEXTURES] done, state set ready\n");
}

// Embed SDL into GTK DrawingArea
int sdl_embed_in_gtk(GtkWidget *widget) {

	sdl_set_playing(!sdl_is_playing());

	TTF_Init();

    textures_1 = textures_1_gray = NULL;
    textures_2 = textures_2_gray = NULL;
    textures_3 = textures_3_gray = NULL;
    textures_4 = textures_4_gray = NULL;

	GdkWindow *gdk = gtk_widget_get_window(widget);
	if (!gdk || !GDK_IS_X11_WINDOW(gdk)) {
	g_printerr("❌ No X11 window\n");
	return 0;
	}

	Window xid = GDK_WINDOW_XID(gdk);
	g_print("XID = 0x%lx\n", (unsigned long)xid);

	SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "0");
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
	g_printerr("SDL_Init failed: %s\n", SDL_GetError());
	return 0;
	}

	sdl_win = SDL_CreateWindowFrom((void *)(uintptr_t)xid);
	if (!sdl_win) {
	g_printerr("SDL_CreateWindowFrom failed: %s\n", SDL_GetError());
	return 0;
	}

	renderer = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer) {
	g_printerr("SDL_CreateRenderer failed: %s\n", SDL_GetError());
	return 0;
	}

	total_frames_1 = count_frames("Frames_1");  
	total_frames_2 = count_frames("Frames_2");
	total_frames_3 = count_frames("Frames_3");  
	total_frames_4 = count_frames("Frames_4");
	
	if (total_frames_1 == 0 &&
	    total_frames_2 == 0 &&
	    total_frames_3 == 0 &&
	    total_frames_4 == 0) {

	    void sdl_set_no_frames(void);
	} else {
		sdl_set_loading();
	    start_load_textures_async();
	}

	printf("Frames in folder 1: %d\n", total_frames_1);
	printf("Frames in folder 2: %d\n", total_frames_2);
	printf("Frames in folder 3: %d\n", total_frames_3);
	printf("Frames in folder 4: %d\n", total_frames_4);
	
	g_timeout_add(16, (GSourceFunc)sdl_draw_frame, NULL); // Refresh ~60 FPS

	g_print("✅ SDL embedded and frame loop started!\n");
	return 1;
}

