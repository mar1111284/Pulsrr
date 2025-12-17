#include "sdl.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <gdk/gdkx.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>

static SDL_Window   *sdl_win = NULL;
static SDL_Renderer *renderer = NULL;

// Global state variables defined once
RenderState render_state = RENDER_STATE_NO_FRAMES;
int is_playing = 0;

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


// Count frames in folder
static int count_frames(const char *folder) {
    DIR *d = opendir(folder);
    if (!d) return 0;
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(d)) != NULL) {
        if (strstr(entry->d_name, ".png")) count++;
    }
    closedir(d);
    return count;
}

// Frame tracking (what frame we start)
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

// Preloaded textures for videos
static SDL_Texture **textures_1 = NULL;
static SDL_Texture **textures_2 = NULL;
static SDL_Texture **textures_3 = NULL;
static SDL_Texture **textures_4 = NULL;

// Preloaded textures grayscale for videos
static SDL_Texture **textures_1_gray = NULL;
static SDL_Texture **textures_2_gray = NULL;
static SDL_Texture **textures_3_gray = NULL;
static SDL_Texture **textures_4_gray = NULL;

static void render_loading_screen(void) {
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderClear(renderer);

    // Text will be added next step
    SDL_RenderPresent(renderer);
}

// Transparency for each layer
static Uint8 layer_alpha[4] = {255, 128, 128, 128};
static Uint8 layer_gray[4] = {0, 0, 0, 0};

void set_gray(int layer_number, int gray) {
    if (layer_number < 1 || layer_number > 4) return;
    layer_gray[layer_number - 1] = gray ? 1 : 0;
    g_print("Layer %d grayscale: %s\n", layer_number, gray ? "ON" : "OFF");
}

// Return whether a layer is currently in grayscale
int is_layer_gray(int layer_number) {
    if (layer_number < 1 || layer_number > 4) return 0;
    return layer_gray[layer_number - 1];
}

// Getter for renderer
SDL_Renderer* sdl_get_renderer() {
    return renderer;
}

// Apply grayscale directly on SDL_Surface
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

// modify transarency of a layer
void set_transparency(int layer_number, int alpha) {
    if (layer_number < 1 || layer_number > 4) return;
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;

    layer_alpha[layer_number - 1] = (Uint8)alpha;
    g_print("Layer %d transparency set to %d\n", layer_number, alpha);
}

// Load SDL_Texture from PNG with alpha enabled
static SDL_Texture* load_frame_texture(const char *filename) {
	sdl_set_loading(); // loading begins
	SDL_Surface *surf = IMG_Load(filename);
	if (!surf) {
	g_print("⚠️ Failed to load %s\n", filename);
	return NULL;
	}
	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND); // allow transparency
	SDL_FreeSurface(surf);
	sdl_set_ready(); // all frames loaded
	return tex;
}

// Load SDL_Texture from PNG and return a grayscale version
static SDL_Texture* load_frame_texture_gray(const char *filename) {
	sdl_set_loading(); // loading begins
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
	sdl_set_ready(); // all frames loaded

	return tex_gray;
}



// Preload all textures for video 1
/*
static void preload_textures_1() {
    if (total_frames_1 <= 0) return;

    textures_1 = malloc(sizeof(SDL_Texture*) * total_frames_1);
    for (int i = 0; i < total_frames_1; i++) {
        char filename[512];
        snprintf(filename, sizeof(filename), frame_1_pattern, i + 1); // 1-based filenames
        textures_1[i] = load_frame_texture(filename);
        if (!textures_1[i]) {
            g_print("⚠️ Failed to preload frame %d\n", i + 1);
        }
    }
}
*/

static void render_no_frames_screen(void) {
    // Slightly lighter than #0d0d0d
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderClear(renderer);

    // Placeholder: text comes later
    SDL_RenderPresent(renderer);
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

	// 👇 handle keyboard
	handle_sdl_events();

	// --- Clear once ---
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

	// ⏸️ Pause stops frame advancement
	if (!is_playing) return;

	// --- Increment frames independently ---
	frame_1++;
	if (frame_1 > total_frames_1) frame_1 = 1;
	frame_2++;
	if (frame_2 > total_frames_2) frame_2 = 1;
	frame_3++;
	if (frame_3 > total_frames_3) frame_3 = 1;
	frame_4++;
	if (frame_4 > total_frames_4) frame_4 = 1;
}

// Embed SDL into GTK DrawingArea
int sdl_embed_in_gtk(GtkWidget *widget) {
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

	// calculate total frames
	total_frames_1 = count_frames("Frames_1");  
	total_frames_2 = count_frames("Frames_2");
	total_frames_3 = count_frames("Frames_3");  
	total_frames_4 = count_frames("Frames_4");
	
	if (total_frames_1 == 0 &&
	    total_frames_2 == 0 &&
	    total_frames_3 == 0 &&
	    total_frames_4 == 0) {

	    render_state = RENDER_STATE_NO_FRAMES;
	} else {
	    render_state = RENDER_STATE_LOADING;
	}

		
	printf("Frames in folder 1: %d\n", total_frames_1);
	printf("Frames in folder 2: %d\n", total_frames_2);
	printf("Frames in folder 3: %d\n", total_frames_3);
	printf("Frames in folder 4: %d\n", total_frames_4);
	
	// TEXTURE PRELOADING MEMORY ALLOCATION
	// Allocate memory for normal + grayscale textures
	textures_1 = malloc(total_frames_1 * sizeof(SDL_Texture*));
	textures_1_gray = malloc(total_frames_1 * sizeof(SDL_Texture*));

	textures_2 = malloc(total_frames_2 * sizeof(SDL_Texture*));
	textures_2_gray = malloc(total_frames_2 * sizeof(SDL_Texture*));

	textures_3 = malloc(total_frames_3 * sizeof(SDL_Texture*));
	textures_3_gray = malloc(total_frames_3 * sizeof(SDL_Texture*));

	textures_4 = malloc(total_frames_4 * sizeof(SDL_Texture*));
	textures_4_gray = malloc(total_frames_4 * sizeof(SDL_Texture*));
	
	// Preload textures for video 1
	for (int i = 0; i < total_frames_1; i++) {
	    char filename[512];
	    snprintf(filename, sizeof(filename), frame_1_pattern, i + 1); // keep +1 for frame filenames
	    textures_1[i] = load_frame_texture(filename);
	    textures_1_gray[i] = load_frame_texture_gray(filename);
	    if (!textures_1[i]) {
		g_print("⚠️ Failed to load frame %d\n", i + 1);
		textures_1[i] = NULL; // avoid garbage pointer
	    }
	}
	
	// Preload textures for video 2
	for (int i = 0; i < total_frames_2; i++) {
	    char filename[512];
	    snprintf(filename, sizeof(filename), frame_2_pattern, i + 1); // filenames start at 1
	    textures_2[i] = load_frame_texture(filename);
	    textures_2_gray[i] = load_frame_texture_gray(filename);
	    if (!textures_2[i]) {
		g_print("⚠️ Failed to load frame %d of video 2\n", i + 1);
		textures_2[i] = NULL;
	    }
	}
	
	// Preload textures for video 3
	for (int i = 0; i < total_frames_3; i++) {
	    char filename[512];
	    snprintf(filename, sizeof(filename), frame_3_pattern, i + 1); // filenames start at 1
	    textures_3[i] = load_frame_texture(filename);
	    textures_3_gray[i] = load_frame_texture_gray(filename);
	    if (!textures_3[i]) {
		g_print("⚠️ Failed to load frame %d of video 3\n", i + 1);
		textures_3[i] = NULL;
	    }
	}
	
	// Preload textures for video 4
	for (int i = 0; i < total_frames_4; i++) {
	    char filename[512];
	    snprintf(filename, sizeof(filename), frame_4_pattern, i + 1); // filenames start at 1
	    textures_4[i] = load_frame_texture(filename);
	    textures_4_gray[i] = load_frame_texture_gray(filename);
	    if (!textures_4[i]) {
		g_print("⚠️ Failed to load frame %d of video 4\n", i + 1);
		textures_4[i] = NULL;
	    }
	}

	

	// Refresh ~60 FPS
	g_timeout_add(16, (GSourceFunc)sdl_draw_frame, NULL);

	g_print("✅ SDL embedded and frame loop started!\n");
	return 1;
}

