#ifndef MODAL_LOAD_H
#define MODAL_LOAD_H

#include <gtk/gtk.h>

#include "utils.h"
#include "sdl.h"
#include "layer.h"

// --------------------
// UI data structures
// --------------------

typedef struct {
    GtkLabel        *filename;
    GtkLabel        *resolution;
    GtkLabel        *fps;
    GtkLabel        *duration;
    GtkLabel        *filesize;
    GtkLabel        *estimated_frames_nb;
    GtkLabel        *estimated_size;
    GtkWidget       *fps_spin;
    GtkComboBoxText *scale_combo;
    GtkWidget *progress_bar;
} VideoInfoLabels;

typedef struct {
    GtkWidget      *file_label;
    GtkSpinButton  *fps_spin;
    GtkComboBoxText *scale_combo;
    GtkWidget      *progress_bar;
    guint8          layer_index;
} ExportUIContext;

// --------------------
// Thread / progress
// --------------------

typedef struct {
    GtkWidget *progress_bar;
    double     fraction;
    char      *text;
} ProgressUpdate;

typedef struct {
    gchar     *file_path;
    guint8     fps;
    int        resolution;
    guint8     layer_index;
    GtkWidget *progress_bar;
    gchar     *folder;
} ExportContext;

// --------------------
// Callbacks / API
// --------------------

void on_load_button_clicked(GtkButton *button, gpointer user_data);
void on_export_clicked(GtkButton *button, gpointer user_data);

void on_drag_data_received(GtkWidget *widget,
                           GdkDragContext *context,
                           gint x, gint y,
                           GtkSelectionData *data,
                           guint info, guint time,
                           gpointer user_data);

void on_fps_spin_changed(GtkSpinButton *spin, gpointer user_data);
void on_scale_combo_changed(GtkComboBox *combo, gpointer user_data);
void update_export_estimation(VideoInfoLabels *labels);
void on_fps_spin_changed(GtkSpinButton *spin, gpointer user_data);
void on_scale_combo_changed(GtkComboBox *combo, gpointer user_data);
gboolean debounce_update_estimation(gpointer user_data);

gpointer export_thread_func(gpointer data);
gboolean update_progress_cb(gpointer data);

#endif // MODAL_LOAD_H

