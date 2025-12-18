#ifndef LAYER_H
#define LAYER_H

#include <gtk/gtk.h>

// --------------------
// Constants
// --------------------
/*
#define MAX_LAYERS 16
#define LAYER_COMPONENT_WIDTH 300
#define LAYER_COMPONENT_HEIGHT 120
*/
// --------------------
// Global structures
// --------------------


// --------------------
// Global preview boxes array
// --------------------
extern GtkWidget *layer_preview_boxes[MAX_LAYERS];
// --------------------
// Layer component creation
// --------------------
GtkWidget* create_layer_component(const char *label_text, int layer_number);

// --------------------
// Load / FX / Menu callbacks
// --------------------
void on_load_button_clicked(GtkButton *button, gpointer user_data);
void on_fx_button_clicked(GtkButton *button, gpointer user_data);
void on_fx_apply_clicked(GtkButton *button, gpointer user_data);
gboolean on_layer_menu_label_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

// --------------------
// Modal / Export callbacks
// --------------------
void on_modal_cancel_clicked(GtkButton *button, gpointer user_data);
void on_export_clicked(GtkButton *button, gpointer user_data);

// --------------------
// Layer preview updates
// --------------------
void update_layer_preview(int layer_number);
gboolean update_preview_idle(gpointer data);

// --------------------
// Threading / progress updates
// --------------------
gpointer export_thread_func(gpointer data);
gboolean update_progress_cb(gpointer data);

// --------------------
// Utility functions
// --------------------


#endif // LAYER_H


