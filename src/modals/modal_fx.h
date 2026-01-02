#ifndef MODAL_FX_H
#define MODAL_FX_H

#include <gtk/gtk.h>
#include "../utils/utils.h"
#include "../components/component_layer.h"
#include "../sdl/sdl.h"

typedef struct {
    guint8 layer_index;  	
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
