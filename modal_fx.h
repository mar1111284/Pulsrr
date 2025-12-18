#ifndef MODAL_FX_H
#define MODAL_FX_H

#include <gtk/gtk.h>
#include "utils.h"
#include "layer.h"  // for set_transparency, set_gray, set_layer_speed, is_layer_gray
#include "sdl.h"    // for sdl_set_playing

typedef struct {
    int layer_number;  	
    GtkSpinButton *speed_spin;
    GtkSpinButton *alpha_spin;
    GtkSpinButton *threshold_spin;
    GtkSpinButton *contrast_spin;
    GtkToggleButton *gray_check;
    GtkToggleButton *invert_check;
} LayerFxWidgets;

// Functions
void on_fx_button_clicked(GtkButton *button, gpointer user_data);
void on_fx_apply_clicked(GtkButton *button, gpointer user_data);

// Extern modal layer (defined elsewhere)
extern GtkWidget *global_modal_layer;

#endif // MODAL_FX_H
