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

static int is_playing = 1;

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

void sdl_set_playing(int playing) {
    is_playing = playing ? 1 : 0;
    g_print("SDL playback: %s\n", is_playing ? "PLAY" : "PAUSE");
}

int sdl_is_playing(void) {
    return is_playing;
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

// Getter for renderer
SDL_Renderer* sdl_get_renderer() {
    return renderer;
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

// Preload all textures for video 1
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

// Draw frames with independent looping
void sdl_draw_frame() {
    if (!renderer) return;
    
    // 👇 handle keyboard
    handle_sdl_events();

    // --- Clear once ---
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // --- Video 1 (bottom layer) ---
    if (total_frames_1 > 0 && textures_1[frame_1 - 1]) {
        SDL_SetTextureAlphaMod(textures_1[frame_1 - 1], 255); // opaque
        SDL_RenderCopy(renderer, textures_1[frame_1 - 1], NULL, NULL);
    }

    // --- Video 2 (top layer) ---
    if (total_frames_2 > 0 && textures_2[frame_2 - 1]) {
        SDL_SetTextureAlphaMod(textures_2[frame_2 - 1], 128); // semi-transparent
        SDL_RenderCopy(renderer, textures_2[frame_2 - 1], NULL, NULL);
    }
    
    // --- Video 3 (top layer) ---
    if (total_frames_3 > 0 && textures_3[frame_3 - 1]) {
        SDL_SetTextureAlphaMod(textures_3[frame_3 - 1], 128); // opaque
        SDL_RenderCopy(renderer, textures_3[frame_3 - 1], NULL, NULL);
    }

    // --- Video 4 (top layer) ---
    if (total_frames_4 > 0 && textures_4[frame_4 - 1]) {
        SDL_SetTextureAlphaMod(textures_4[frame_4 - 1], 128); // semi-transparent
        SDL_RenderCopy(renderer, textures_4[frame_4 - 1], NULL, NULL);
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
	
	printf("Frames in folder 1: %d\n", total_frames_1);
	printf("Frames in folder 2: %d\n", total_frames_2);
	printf("Frames in folder 3: %d\n", total_frames_3);
	printf("Frames in folder 4: %d\n", total_frames_4);
	
	// Preload textures for video 1
	// Allocate textures array
	textures_1 = malloc(total_frames_1 * sizeof(SDL_Texture*));
	for (int i = 0; i < total_frames_1; i++) {
	    char filename[512];
	    snprintf(filename, sizeof(filename), frame_1_pattern, i + 1); // keep +1 for frame filenames
	    textures_1[i] = load_frame_texture(filename);
	    if (!textures_1[i]) {
		g_print("⚠️ Failed to load frame %d\n", i + 1);
		textures_1[i] = NULL; // avoid garbage pointer
	    }
	}
	
	// Preload textures for video 2
	textures_2 = malloc(total_frames_2 * sizeof(SDL_Texture*));
	for (int i = 0; i < total_frames_2; i++) {
	    char filename[512];
	    snprintf(filename, sizeof(filename), frame_2_pattern, i + 1); // filenames start at 1
	    textures_2[i] = load_frame_texture(filename);
	    if (!textures_2[i]) {
		g_print("⚠️ Failed to load frame %d of video 2\n", i + 1);
		textures_2[i] = NULL;
	    }
	}
	
	// Preload textures for video 3
	textures_3 = malloc(total_frames_3 * sizeof(SDL_Texture*));
	for (int i = 0; i < total_frames_3; i++) {
	    char filename[512];
	    snprintf(filename, sizeof(filename), frame_3_pattern, i + 1); // filenames start at 1
	    textures_3[i] = load_frame_texture(filename);
	    if (!textures_3[i]) {
		g_print("⚠️ Failed to load frame %d of video 3\n", i + 1);
		textures_3[i] = NULL;
	    }
	}
	
	// Preload textures for video 4
	textures_4 = malloc(total_frames_4 * sizeof(SDL_Texture*));
	for (int i = 0; i < total_frames_4; i++) {
	    char filename[512];
	    snprintf(filename, sizeof(filename), frame_4_pattern, i + 1); // filenames start at 1
	    textures_4[i] = load_frame_texture(filename);
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

