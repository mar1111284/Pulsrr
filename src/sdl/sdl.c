/* SDL engine */
#include "sdl.h"
#include "../utils/accessor.h"

/* System & libraries */
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <gdk/gdkx.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

// Global SDL state
SDL g_sdl = {
    .window = NULL,
    .renderer = NULL,
    .surface = NULL,
    .initialized = FALSE,
    .draw_source_id = 0,
    .screen_mode = LIVE_MODE
};


void sdl_clear_layer(guint8 layer_index)
{
    if (layer_index >= MAX_LAYERS) {
        add_main_log("[WARN] sdl_clear_layer: invalid layer index");
        return;
    }

    Layer *layer = g_sdl.layers[layer_index];
    if (!layer) {
        // Already empty — nothing to do
        return;
    }

    add_main_log(g_strdup_printf("[SDL] Clearing layer %u...", layer_index + 1));

    // 1. Destroy all textures
    if (layer->textures) {
        for (int i = 0; i < layer->frame_count; i++) {
            if (layer->textures[i]) {
                SDL_DestroyTexture(layer->textures[i]);
                layer->textures[i] = NULL;
            }
        }
        free(layer->textures);
        layer->textures = NULL;
    }

    if (layer->textures_gray) {
        for (int i = 0; i < layer->frame_count; i++) {
            if (layer->textures_gray[i]) {
                SDL_DestroyTexture(layer->textures_gray[i]);
                layer->textures_gray[i] = NULL;
            }
        }
        free(layer->textures_gray);
        layer->textures_gray = NULL;
    }

    // 2. Free surfaces (if you store them)
    if (layer->frames) {
        for (int i = 0; i < layer->frame_count; i++) {
            if (layer->frames[i]) {
                SDL_FreeSurface(layer->frames[i]);
                layer->frames[i] = NULL;
            }
        }
        free(layer->frames);
        layer->frames = NULL;
    }

    if (layer->frames_gray) {
        for (int i = 0; i < layer->frame_count; i++) {
            if (layer->frames_gray[i]) {
                SDL_FreeSurface(layer->frames_gray[i]);
                layer->frames_gray[i] = NULL;
            }
        }
        free(layer->frames_gray);
        layer->frames_gray = NULL;
    }

    // 3. Free folder path
    free(layer->frame_folder);
    layer->frame_folder = NULL;

    // 4. Reset all fields
    layer->frame_count = 0;
    layer->current_frame = 0;
    layer->last_tick = 0;
    layer->speed = 1.0;
    layer->fps = 25;  // or your default
    layer->alpha = 255;
    layer->grayscale = 0;
    layer->blend_mode = SDL_BLENDMODE_BLEND;
    layer->width = 0;
    layer->height = 0;
    layer->accumulated_delta = 0.0;
    layer->state = LAYER_EMPTY;

    add_main_log(g_strdup_printf("[SDL] Layer %u fully cleared from memory", layer_index + 1));
}

void free_sequence(Sequence *seq) {
    if (!seq) return;

    if (seq->textures) {
        for (int i = 0; i < seq->frame_count; i++)
            if (seq->textures[i]) SDL_DestroyTexture(seq->textures[i]);
        g_free(seq->textures);
    }

    if (seq->frames) {
        for (int i = 0; i < seq->frame_count; i++)
            if (seq->frames[i]) SDL_FreeSurface(seq->frames[i]);
        g_free(seq->frames);
    }
	seq->accumulated_delta = 0.0;
    g_free(seq->root_folder);
    g_free(seq);
}

Sequence* update_sequence_texture() {
    Sequence *seq = g_malloc0(sizeof(Sequence));
    seq->current_frame = 0;
    seq->last_tick = SDL_GetTicks();
    seq->frame_count = 0;
    seq->frames = NULL;
    seq->textures = NULL;
    seq->speed = 1.0;              
    seq->accumulated_delta = 0.0;
    seq->last_tick = SDL_GetTicks();

    gchar *sequences_dir = "sequences";
    seq->root_folder = g_strdup(sequences_dir);
    GPtrArray *all_frames = g_ptr_array_new();

    DIR *dir = opendir(sequences_dir);
    if (!dir) {
        g_printerr("[PLAYBACK] Cannot open sequences folder\n");
        return seq;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) continue;
        if (g_str_has_prefix(entry->d_name, "sequence_")) {

            gchar *mixed_path = g_build_filename(sequences_dir, entry->d_name, "mixed_frames", NULL);
            if (!g_file_test(mixed_path, G_FILE_TEST_IS_DIR)) {
                g_printerr("[PLAYBACK] No mixed_frames in %s\n", entry->d_name);
                g_free(mixed_path);
                continue;
            }

            DIR *frames_dir = opendir(mixed_path);
            if (!frames_dir) {
                g_free(mixed_path);
                continue;
            }

            struct dirent *frame_entry;
            while ((frame_entry = readdir(frames_dir)) != NULL) {
                if (frame_entry->d_type != DT_REG) continue;
                if (!g_str_has_suffix(frame_entry->d_name, ".png")) continue;

                gchar *frame_file = g_build_filename(mixed_path, frame_entry->d_name, NULL);
                SDL_Surface *surf = IMG_Load(frame_file);
                if (!surf) {
                    g_printerr("[PLAYBACK] Failed to load frame: %s\n", frame_file);
                    g_free(frame_file);
                    continue;
                }

                g_ptr_array_add(all_frames, surf);
                g_free(frame_file);
            }
            closedir(frames_dir);
            g_free(mixed_path);
        }
    }
    closedir(dir);

    seq->frame_count = all_frames->len;
    if (seq->frame_count > 0) {
        seq->frames = g_malloc0(sizeof(SDL_Surface*) * seq->frame_count);
        seq->textures = g_malloc0(sizeof(SDL_Texture*) * seq->frame_count);

        for (int i = 0; i < seq->frame_count; i++) {
            SDL_Surface *surf = g_ptr_array_index(all_frames, i);
            seq->frames[i] = surf;
            seq->textures[i] = SDL_CreateTextureFromSurface(g_sdl.renderer, surf);
        }
    }

    g_ptr_array_free(all_frames, TRUE);
    g_sdl.sequence = seq;
    return seq;
}

// sdl.c — implementation
void sdl_clear_all_sequences(void)
{
    Sequence *seq = g_sdl.sequence;
    if (!seq) {
        add_main_log("[SDL] No sequences to clear");
        return;
    }

    add_main_log("[SDL] Clearing all sequences from memory...");

    // 1. Free textures
    if (seq->textures) {
        for (int i = 0; i < seq->frame_count; i++) {
            if (seq->textures[i]) {
                SDL_DestroyTexture(seq->textures[i]);
                seq->textures[i] = NULL;
            }
        }
        free(seq->textures);
        seq->textures = NULL;
    }

    // 2. Free surfaces
    if (seq->frames) {
        for (int i = 0; i < seq->frame_count; i++) {
            if (seq->frames[i]) {
                SDL_FreeSurface(seq->frames[i]);
                seq->frames[i] = NULL;
            }
        }
        free(seq->frames);
        seq->frames = NULL;
    }

    // 3. Free folder path
    free(seq->root_folder);
    seq->root_folder = NULL;

    // 4. Reset all fields
    seq->frame_count = 0;
    seq->current_frame = 0;
    seq->last_tick = 0;
    seq->fps = 25;
    seq->speed = 1.0;
    seq->accumulated_delta = 0.0;

    add_main_log("[SDL] All sequences cleared from memory");
}

void sdl_render_playback_mode(int advance_frames) {

	if (!sdl_has_sequence_texture()) {
            draw_centered_text("PLAYBACK MODE (No sequence loaded)");
        }
        
    if (!g_sdl.sequence || g_sdl.sequence->frame_count == 0) {
        return;
    }

    Sequence *seq = g_sdl.sequence;
    Uint32 now = SDL_GetTicks();
    double frame_duration = 1000.0 / MASTER_FPS;

    if (seq->current_frame >= seq->frame_count || seq->current_frame < 0) {
        seq->current_frame = 0;
    }

    SDL_Texture *tex = seq->textures[seq->current_frame];
    if (!tex) {
        g_printerr("[PLAYBACK] Texture %d is NULL\n", seq->current_frame);
        return;
    }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(g_sdl.renderer, tex, NULL, NULL);

    if (advance_frames) {
        Uint32 elapsed = now - seq->last_tick;
        double delta = (double)elapsed / frame_duration * seq->speed;

        seq->accumulated_delta += delta;

        while (seq->accumulated_delta >= 1.0) {
            seq->current_frame = (seq->current_frame + 1) % seq->frame_count;
            seq->accumulated_delta -= 1.0;
        }

        seq->last_tick = now;
    }
	
    //g_print("[PLAYBACK] frame=%d/%d speed=%.2f accum=%.2f\n",seq->current_frame, seq->frame_count, seq->speed, seq->accumulated_delta);
}

gboolean sdl_finalize_texture_update(gpointer data)
{
    (void)data;

    for (int i = 0; i < 4; i++) {
        Layer *ly = g_sdl.layers[i];
        if (!ly) continue;

        if (ly->state != LAYER_MODIFIED)
            continue;

        /* --- clear old textures --- */
        if (ly->textures) {
            for (int f = 0; f < ly->frame_count; f++) {
                if (ly->textures[f]) SDL_DestroyTexture(ly->textures[f]);
                if (ly->textures_gray && ly->textures_gray[f])
                    SDL_DestroyTexture(ly->textures_gray[f]);
            }
            g_free(ly->textures);
            g_free(ly->textures_gray);
        }

        ly->textures = g_malloc0(sizeof(SDL_Texture*) * ly->frame_count);
        ly->textures_gray = g_malloc0(sizeof(SDL_Texture*) * ly->frame_count);

        /* --- build textures --- */
        for (int f = 0; f < ly->frame_count; f++) {
            if (!ly->frames[f])
                continue;

            ly->textures[f] =
                SDL_CreateTextureFromSurface(g_sdl.renderer, ly->frames[f]);

            ly->textures_gray[f] =
                SDL_CreateTextureFromSurface(g_sdl.renderer, ly->frames_gray[f]);

            if (!ly->textures[f] || !ly->textures_gray[f]) {
                g_printerr("[ERROR] Layer %d Frame %d texture creation failed\n", i, f + 1);
            }
        }

        /* --- free surfaces --- */
        for (int f = 0; f < ly->frame_count; f++) {
            if (ly->frames[f]) SDL_FreeSurface(ly->frames[f]);
            if (ly->frames_gray[f]) SDL_FreeSurface(ly->frames_gray[f]);
        }

        g_free(ly->frames);
        g_free(ly->frames_gray);

        ly->frames = NULL;
        ly->frames_gray = NULL;

        ly->current_frame = 0;
        ly->last_tick = SDL_GetTicks();
        ly->state = LAYER_UP_TO_DATE;

    }

    sdl_set_render_state(RENDER_STATE_PLAY);

    return G_SOURCE_REMOVE;
}

void* texture_update_thread(void *arg)
{
    (void)arg;
	sdl_set_render_state(RENDER_STATE_LOADING);

    for (int i = 0; i < 4; i++) {
        Layer *ly = g_sdl.layers[i];
        if (!ly) continue;
		
		// CLear surface
        if (ly->frames) {
            for (int f = 0; f < ly->frame_count; f++) {
                if (ly->frames[f]) SDL_FreeSurface(ly->frames[f]);
                if (ly->frames_gray && ly->frames_gray[f])
                    SDL_FreeSurface(ly->frames_gray[f]);
            }
            g_free(ly->frames);
            g_free(ly->frames_gray);
        }

        ly->frames = NULL;
        ly->frames_gray = NULL;
        ly->frame_count = 0;

		// count frames
		if (!ly->frame_folder) {
			ly->state = LAYER_EMPTY;
			continue;
		}

		int count = count_frames(ly->frame_folder);
		if (count <= 0) {
			ly->state = LAYER_EMPTY;
			continue;
		}


        ly->frames = g_malloc0(sizeof(SDL_Surface*) * count);
        ly->frames_gray = g_malloc0(sizeof(SDL_Surface*) * count);
        ly->frame_count = count;

        // Load surfaces
        for (int f = 0; f < count; f++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/frame_%05d.png", ly->frame_folder, f + 1);

            SDL_Surface *src = IMG_Load(path);
            if (!src) {
                g_printerr("[ERROR] Layer %d Frame %d load failed (%s)\n", i, f + 1, path);
                continue;
            }

            ly->frames[f] = src;
            ly->frames_gray[f] = create_grayscale_surface(src);
        }
    }

    g_idle_add(sdl_finalize_texture_update, NULL);

    return NULL;
}

// Update text async
void update_textures_async(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, texture_update_thread, NULL);
    pthread_detach(tid);
}

// Init layer
void init_layers() {
    for (int i = 0; i < 4; i++) {
        if (!g_sdl.layers[i]) {
            g_sdl.layers[i] = g_malloc0(sizeof(Layer));
        }

        Layer *ly = g_sdl.layers[i];
        ly->state = LAYER_EMPTY;
        ly->frame_count = 0;
        ly->textures = NULL;
        ly->textures_gray = NULL;
        ly->grayscale = 0;
        ly->speed = 1;

        // Initial alpha default
        ly->alpha = (i == 0) ? 255 : 128;

        ly->current_frame = 0;
        ly->last_tick = SDL_GetTicks();
        ly->fps = 30;
    }
}


void draw_centered_text(const char *text)
{
    static TTF_Font *font = NULL;
    const AppPaths *paths = get_app_paths();

    if (!font) {
        char *font_path = g_build_filename(paths->media_dir, "DS-TERM.TTF", NULL);
        if (!font_path) {
            g_printerr("[SDL] Failed to build font path\n");
            return;
        }

        font = TTF_OpenFont(font_path, 24);
        g_free(font_path);  // ← FREE IT HERE

        if (!font) {
            g_printerr("[SDL] Failed to load font: %s\n", TTF_GetError());
            return;
        }
    }

    if (!text || !*text) return;  // Safety

    SDL_Color green = {0x33, 0xFF, 0x33, 255};
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, green);
    if (!surf) {
        g_printerr("[SDL] TTF_RenderUTF8_Blended failed: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(g_sdl.renderer, surf);
    SDL_FreeSurface(surf);

    if (!tex) {
        g_printerr("[SDL] SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
        return;
    }

    int tex_w, tex_h;
    SDL_QueryTexture(tex, NULL, NULL, &tex_w, &tex_h);

    int win_w, win_h;
    SDL_GetRendererOutputSize(g_sdl.renderer, &win_w, &win_h);

    SDL_Rect dst = {
        .x = (win_w - tex_w) / 2,
        .y = (win_h - tex_h) / 2,
        .w = tex_w,
        .h = tex_h
    };

    SDL_RenderCopy(g_sdl.renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);  // Good — you already do this
}
// SDL init
int sdl_init(GtkWidget *widget)
{
    if (g_sdl.initialized)
        return 1;

    if (!gtk_widget_get_realized(widget)) {
        g_printerr("[SDL] Widget not realized\n");
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        g_printerr("[SDL] SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }

    if (TTF_Init() != 0) {
        g_printerr("[SDL] TTF_Init failed: %s\n", TTF_GetError());
        return 0;
    }

    GdkWindow *gdk_window = gtk_widget_get_window(widget);
    Window xid = GDK_WINDOW_XID(gdk_window);

    g_sdl.window = SDL_CreateWindowFrom((void *)(uintptr_t)xid);
    if (!g_sdl.window) {
        g_printerr("[SDL] CreateWindowFrom failed: %s\n", SDL_GetError());
        return 0;
    }

    g_sdl.renderer = SDL_CreateRenderer(
        g_sdl.window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!g_sdl.renderer) {
        g_printerr("[SDL] CreateRenderer failed: %s\n", SDL_GetError());
        return 0;
    }

    SDL_SetRenderDrawBlendMode(g_sdl.renderer, SDL_BLENDMODE_BLEND);

    g_sdl.initialized = TRUE;
    //g_print("[SDL] Initialized (renderer mode)\n");

    return 1;
}

// Apply grayscale
SDL_Surface* create_grayscale_surface(SDL_Surface *src) {
    if (!src) return NULL;

    SDL_Surface *gray = SDL_ConvertSurface(src, src->format, 0);
    if (!gray) return NULL;

    SDL_LockSurface(gray);
    Uint8 *pixels = (Uint8 *)gray->pixels;
    int pitch = gray->pitch;

    for (int y = 0; y < gray->h; y++) {
        Uint8 *row = pixels + y * pitch;
        for (int x = 0; x < gray->w; x++) {
            Uint8 *p = row + x * gray->format->BytesPerPixel;
            Uint8 r, g, b;
            SDL_GetRGB(*(Uint32 *)p, gray->format, &r, &g, &b);
            Uint8 v = (r + g + b) / 3;
            *(Uint32 *)p = SDL_MapRGB(gray->format, v, v, v);
        }
    }

    SDL_UnlockSurface(gray);
    return gray;
}

// Render draw live
void sdl_render_live_mode(int advance_frames) {
    static int error_logged[4] = {0, 0, 0, 0};
    Uint32 now = SDL_GetTicks();
    double frame_duration = 1000.0 / MASTER_FPS;  // Time per frame in ms
    
    if (!sdl_has_live_texture()) {
            draw_centered_text("LIVE MODE (No frames loaded)");
        }

    for (int i = 0; i < 4; i++) {
        Layer *ly = g_sdl.layers[i];
        if (!ly || ly->state != LAYER_UP_TO_DATE || ly->frame_count <= 0 || !ly->textures) continue;

        if (ly->current_frame >= ly->frame_count || ly->current_frame < 0) ly->current_frame = 0;

        SDL_Texture *tex = ly->grayscale ? ly->textures_gray[ly->current_frame] : ly->textures[ly->current_frame];
        if (!tex) {
            if (!error_logged[i]) {
                g_printerr("[LIVE] Layer %d texture[%d] is NULL\n", i, ly->current_frame);
                error_logged[i] = 1;
            }
            continue;
        }
        error_logged[i] = 0;

        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(tex, ly->alpha);
        SDL_RenderCopy(g_sdl.renderer, tex, NULL, NULL);

        if (advance_frames) {
            Uint32 elapsed = now - ly->last_tick;
            double delta = (double)elapsed / frame_duration * ly->speed;
            ly->accumulated_delta += delta;

            while (ly->accumulated_delta >= 1.0) {
                ly->current_frame = (ly->current_frame + 1) % ly->frame_count;
                ly->accumulated_delta -= 1.0;
            }

            ly->last_tick = now;
        }

        //g_print("[LIVE] Layer %d: frame=%d/%d alpha=%d grayscale=%d speed=%.2f accum=%.2f\n",i, ly->current_frame, ly->frame_count, ly->alpha, ly->grayscale, ly->speed, ly->accumulated_delta);
    }
}

// Returns true if at least one live layer has frames loaded TO BE REPLACED
bool sdl_has_live_texture(void)
{
    for (int i = 0; i < MAX_LAYERS; i++) {
        Layer *layer = g_sdl.layers[i];
        if (!layer) continue;

        // Only count layers that have textures loaded
        if (layer->textures && layer->frame_count > 0) {
            return true;
        }
    }
    return false;
}

// Returns true if at least one sequence has been created TO BE REPLACED
bool sdl_has_sequence_texture(void)
{
    Sequence *seq = g_sdl.sequence;
    if (!seq) return false;

    if (seq->textures && seq->frame_count > 0) {
        return true;
    }

    return false;
}

// Main draw loop
gboolean sdl_draw_tick(gpointer data)
{
    (void)data;

    if (!g_sdl.initialized || !g_sdl.renderer)
        return TRUE;

    // Clear background once
    SDL_SetRenderDrawColor(g_sdl.renderer, 18, 18, 18, 255);
    SDL_RenderClear(g_sdl.renderer);

	if (g_sdl.screen_mode == LIVE_MODE) {
        
		switch (g_sdl.render_state) {
		
			case RENDER_STATE_IDLE:
				break;
		
		    case RENDER_STATE_LOADING:
		        draw_centered_text("LOADING FRAMES...");
		        break;

		    case RENDER_STATE_PLAY:
		        sdl_render_live_mode(1);
		        break;

		    case RENDER_STATE_PAUSE:
		        sdl_render_live_mode(0);
		        break;
		}
		
		if (!sdl_has_live_texture() && g_sdl.render_state == RENDER_STATE_IDLE) {
            draw_centered_text("LIVE MODE (No frames loaded)");
        }

	} else { // PLAYBACK_MODE
        
		switch (g_sdl.render_state) {
		
			case RENDER_STATE_IDLE:
				break;

		    case RENDER_STATE_LOADING:
		        draw_centered_text("LOADING SEQUENCE FRAMES...");
		        break;

		    case RENDER_STATE_PLAY:
		        sdl_render_playback_mode(1);
		        break;

		    case RENDER_STATE_PAUSE:
		        sdl_render_playback_mode(0);
		        break;
		}
		
		if (!sdl_has_sequence_texture()&& g_sdl.render_state == RENDER_STATE_IDLE) {
            draw_centered_text("PLAYBACK MODE (No sequence loaded)");
        }

	}

    // Present once per tick
    SDL_RenderPresent(g_sdl.renderer);

    return TRUE;
}

// Embed SDL in GTK
int sdl_embed_in_gtk(GtkWidget *widget)
{
    //g_print("[SDL] embed called\n");
	sdl_set_render_state(RENDER_STATE_IDLE);
    if (!sdl_init(widget))
        return 0;

    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);

    if (w <= 0 || h <= 0)
        return 0;

    SDL_SetWindowSize(g_sdl.window, w, h);
    SDL_RenderSetLogicalSize(g_sdl.renderer, w, h);
    
    init_layers();

    if (g_sdl.draw_source_id == 0) {
        g_sdl.draw_source_id = g_timeout_add(16, sdl_draw_tick, NULL);
    }

    return 1;
}

