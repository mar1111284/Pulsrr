#include "accessor.h"
#include "../sdl/sdl.h"
#include "utils.h"

// Playback
void sdl_set_playback_speed(double speed) {
    if (speed < 0.01) speed = 0.01;  // prevent total freeze
    if (g_sdl.sequence) {
        g_sdl.sequence->speed = speed;
        g_print("[PLAYBACK] Speed set to %.2f\n", speed);
    }
}

// Render state
RenderState sdl_get_render_state(void) {
    return g_sdl.render_state;
}

void sdl_set_render_state(RenderState state) {
    g_sdl.render_state = state;
}

// Layer accessors
static inline Layer* get_layer_safe(int layer_index) {
    if (layer_index < 0 || layer_index >= MAX_LAYERS)
        return NULL;
    return g_sdl.layers[layer_index];
}

// Alpha
int sdl_get_alpha(uint8_t layer_index) {
    Layer *ly = get_layer_safe(layer_index);
    if (!ly) return 255;
    return ly->alpha;
}

void sdl_set_layer_alpha(int layer_index, Uint8 alpha) {
    Layer *ly = get_layer_safe(layer_index);
    if (!ly) return;

    ly->alpha = alpha;
    g_print("[SDL] Layer %d alpha set to %d\n", layer_index, alpha);
}

// Grayscale
gboolean sdl_is_layer_gray(uint8_t layer_index) {
    Layer *ly = get_layer_safe(layer_index);
    if (!ly) return FALSE;
    return ly->grayscale ? TRUE : FALSE;
}

void sdl_set_layer_grayscale(int layer_index, int grayscale) {
    Layer *ly = get_layer_safe(layer_index);
    if (!ly) return;

    ly->grayscale = grayscale ? 1 : 0;
}

// Speed
double sdl_get_layer_speed(uint8_t layer_index) {
    Layer *ly = get_layer_safe(layer_index);
    if (!ly) return 1.0;
    return ly->speed;
}

void sdl_set_layer_speed(int layer_index, double speed) {
    Layer *ly = get_layer_safe(layer_index);
    if (!ly) return;

    if (speed <= 0.0) speed = 1.0; // prevent zero or negative speed
    ly->speed = speed;
    g_print("[SDL] Layer %d speed set to %.2f\n", layer_index, speed);
}

// Layer state
LayerState sdl_get_layer_state(uint8_t layer_index) {
    Layer *ly = get_layer_safe(layer_index);
    if (!ly) return LAYER_EMPTY;
    return ly->state;
}

// Mark layer as modified
void sdl_set_layer_modified(int layer_index) {
    if (layer_index < 0 || layer_index >= MAX_LAYERS)
        return;

    Layer *layer = g_sdl.layers[layer_index];
    if (!layer) {
        layer = calloc(1, sizeof(Layer));
        g_sdl.layers[layer_index] = layer;
    }

    layer->state = LAYER_MODIFIED;

    // Set the folder where frames are exported
    char folder[64];
    snprintf(folder, sizeof(folder), "Frames_%d", layer_index + 1);
    free(layer->frame_folder);
    layer->frame_folder = strdup(folder);

    g_print("[SDL] Layer %d marked as MODIFIED, folder: %s\n", layer_index, folder);
}

