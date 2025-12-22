#ifndef LAYER_H
#define LAYER_H

#include <gtk/gtk.h>

typedef struct {
    guint8 layer_index;
    MainUI *ui;
} LayerCallbackData;

// Global preview boxes array
extern GtkWidget *layer_preview_boxes[MAX_LAYERS];

// Layer component creation
GtkWidget* create_layer_component(guint8 layer_index);

// Load / FX / Menu callbacks
void on_load_button_clicked(GtkButton *button, gpointer user_data);
void on_fx_button_clicked(GtkButton *button, gpointer user_data);
void on_fx_apply_clicked(GtkButton *button, gpointer user_data);
gboolean on_layer_menu_label_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

// Modal / Export callbacks
void on_modal_cancel_clicked(GtkButton *button, gpointer user_data);
void on_export_clicked(GtkButton *button, gpointer user_data);

// Layer preview updates
void set_preview_thumbnail(guint8 layer_index);

// Threading / progress updates
gpointer export_thread_func(gpointer data);
gboolean update_progress_cb(gpointer data);

#endif // LAYER_H

