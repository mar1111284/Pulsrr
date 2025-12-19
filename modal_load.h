#ifndef MODAL_LOAD_H
#define MODAL_LOAD_H

#include <gtk/gtk.h>
#include "utils.h"
#include "sdl.h"         // for sdl_set_playing()
#include "layer.h"       // for ProgressUpdate, ExportJob, update_preview_idle(), set_layer_modified()

typedef struct {
    GtkWidget *progress_bar;
    double fraction;
    char *text;
} ProgressUpdate;

typedef struct {
    GtkWidget *file_label;
    GtkSpinButton *fps_spin;
    GtkComboBoxText *scale_combo;
    guint8 layer_index;
    GtkWidget *progress_bar;
} ExportContext;

void on_load_button_clicked(GtkButton *button, gpointer user_data);
void on_export_clicked(GtkButton *button, gpointer user_data);
gpointer export_thread_func(gpointer data);
void on_modal_cancel_clicked(GtkButton *button, gpointer user_data);
gboolean update_progress_cb(gpointer data);

#endif

