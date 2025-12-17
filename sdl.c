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

#include <pthread.h>

void* load_textures_thread(void* arg) {
    load_all_textures();  // the same function
    return NULL;
}

void start_load_textures_async() {
    pthread_t tid;
    pthread_create(&tid, NULL, load_textures_thread, NULL);
    pthread_detach(tid);  // optional, we won't join
}

static SDL_Window   *sdl_win = NULL;
static SDL_Renderer *renderer = NULL;

double layer_speed[4] = {1, 1.0, 1.0, 1.0}; // default 1x
Uint32 last_update[4] = {0, 0, 0, 0};          // for fractional speeds

RenderState get_render_state(void) { return render_state; }
LayerState layer_state[4] = {LAYER_EMPTY, LAYER_EMPTY, LAYER_EMPTY, LAYER_EMPTY};

// Global state variables defined once
RenderState render_state = RENDER_STATE_NO_FRAMES;
int is_playing = 0;

/* -----------------------------
 * SDL Layer Speed Accessors
 * ----------------------------- */

// Setter
void set_layer_speed(int layer_index, double speed) {
    if (layer_index < 0 || layer_index > 3) return;
    if (speed <= 0.0) speed = 0.01; // prevent divide by zero
    layer_speed[layer_index] = speed;
    g_print("[SDL] Layer %d speed set to %.2f\n", layer_index + 1, speed);
}

// Getter
double get_layer_speed(int layer_index) {
    if (layer_index < 0 || layer_index > 3) return 1.0;
    return layer_speed[layer_index];
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

void set_layer_modified(int layer_number) {
    int layer_index = layer_number - 1;  // convert 1-indexed to 0-indexed
    if (layer_index < 0 || layer_index > 3) return;
    layer_state[layer_index] = LAYER_MODIFIED;
    g_print("[GTK] Layer %d set to MODIFIED\n", layer_number);
}

static void draw_centered_text(const char *text) {
    static TTF_Font *font = NULL;

    if (!font) {
        font = TTF_OpenFont("DS-TERM.TTF", 24);  // pick a simple font
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

    draw_centered_text("LOADING FRAMES…");

    SDL_RenderPresent(renderer);
}


static void render_no_frames_screen(void) {
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderClear(renderer);

    draw_centered_text("NO FRAMES LOADED");

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

static void free_layer(SDL_Texture **tex, SDL_Texture **tex_gray, int total) {
    if (!tex) return;

    for (int i = 0; i < total; i++) {
        if (tex[i]) SDL_DestroyTexture(tex[i]);
        if (tex_gray && tex_gray[i]) SDL_DestroyTexture(tex_gray[i]);
    }

    free(tex);
    free(tex_gray);
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
	SDL_Surface *surf = IMG_Load(filename);
	if (!surf) {
	g_print("⚠️ Failed to load %s\n", filename);
	return NULL;
	}
	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND); // allow transparency
	SDL_FreeSurface(surf);
	return tex;
}

// Load SDL_Texture from PNG and return a grayscale version
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
	
	Uint32 now = SDL_GetTicks();
	Uint32 base_delay = 1000 / 25; // 25 FPS default
	
	/*
	frame_1++;
	if (frame_1 > total_frames_1) frame_1 = 1;
	frame_2++;
	if (frame_2 > total_frames_2) frame_2 = 1;
	frame_3++;
	if (frame_3 > total_frames_3) frame_3 = 1;
	frame_4++;
	if (frame_4 > total_frames_4) frame_4 = 1;
	*/
	

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



// Repeat similarly for frames 2, 3, 4

}


void load_all_textures() {
    g_print("[LOAD_TEXTURES] start\n");
    
    g_print("[LOAD_TEXTURES] layer states: %d %d %d %d\n",layer_state[0], layer_state[1], layer_state[2], layer_state[3]);


    sdl_set_loading();
    g_print("[LOAD_TEXTURES] set loading state\n");
    
	// If all layers empty, mark render state accordingly
	if (layer_state[0] == LAYER_EMPTY &&
	    layer_state[1] == LAYER_EMPTY &&
	    layer_state[2] == LAYER_EMPTY &&
	    layer_state[3] == LAYER_EMPTY) {
	    render_state = RENDER_STATE_NO_FRAMES;
	    g_print("[LOAD_TEXTURES] all layers empty, render state set to NO_FRAMES\n");
	    return;
	} else {
	    render_state = RENDER_STATE_LOADING;
	}

    // Reset frame counters
    frame_1 = frame_2 = frame_3 = frame_4 = 1;
    g_print("[LOAD_TEXTURES] frame counters reset\n");

    // --- Video 1 ---
    if (layer_state[0] != LAYER_UP_TO_DATE && layer_state[0] != LAYER_EMPTY) {
        free_layer(textures_1, textures_1_gray, total_frames_1); // free only if modified
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
        free_layer(textures_2, textures_2_gray, total_frames_2); // free only if modified
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
        free_layer(textures_3, textures_3_gray, total_frames_3); // free only if modified
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
        free_layer(textures_4, textures_4_gray, total_frames_4); // free only if modified
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
	
	TTF_Init();


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
	    start_load_textures_async();
	}

	printf("Frames in folder 1: %d\n", total_frames_1);
	printf("Frames in folder 2: %d\n", total_frames_2);
	printf("Frames in folder 3: %d\n", total_frames_3);
	printf("Frames in folder 4: %d\n", total_frames_4);
	
	// Refresh ~60 FPS
	g_timeout_add(16, (GSourceFunc)sdl_draw_frame, NULL);

	g_print("✅ SDL embedded and frame loop started!\n");
	return 1;
}

