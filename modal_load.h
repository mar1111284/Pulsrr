#ifndef MODAL_LOAD_H
#define MODAL_LOAD_H

#include <gtk/gtk.h>
#include "utils.h"
#include "sdl.h"         // for sdl_set_playing()
#include "layer.h"       // for ProgressUpdate, ExportJob, update_preview_idle(), set_layer_modified()

typedef struct {
    GtkLabel *filename;
    GtkLabel *resolution;
    GtkLabel *fps;
    GtkLabel *duration;
    GtkLabel *filesize;
    GtkLabel *estimated_frames_nb;
    GtkLabel *estimated_size;
    GtkWidget *fps_spin;
    GtkComboBoxText *scale_combo;
} VideoInfoLabels;

typedef struct {
    GtkWidget *progress_bar;
    double fraction;
    char *text;
} ProgressUpdate;

typedef struct {
    GtkWidget *file_label;
    GtkSpinButton *fps_spin;
    GtkComboBoxText *scale_combo;
    GtkWidget *progress_bar;
    guint8 layer_index;
} ExportUIContext;

typedef struct {
    gchar *file_path;
    guint8 fps;
    int resolution;
    guint8 layer_index;
    GtkWidget *progress_bar;
    gchar *folder;  
} ExportContext;

void on_drag_data_received(GtkWidget *widget,GdkDragContext *context,gint x, gint y,GtkSelectionData *data,guint info, guint time,gpointer user_data);
void on_load_button_clicked(GtkButton *button, gpointer user_data);
void on_export_clicked(GtkButton *button, gpointer user_data);
gpointer export_thread_func(gpointer data);
void on_modal_cancel_clicked(GtkButton *button, gpointer user_data);
gboolean update_progress_cb(gpointer data);
void on_fps_spin_changed(GtkSpinButton *spin, gpointer user_data);

#endif

