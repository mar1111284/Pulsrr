#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <glib/gstdio.h>

/*
static gboolean close_modal_cb(gpointer data) {
    GtkWidget *modal = GTK_WIDGET(data);
    gtk_widget_hide(modal);
    return G_SOURCE_REMOVE; // run once
}
*/

// Count frames in folder
int count_frames(const char *folder) {
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

void log_error(const char *msg, GError *err) {
    if (err) {
        g_printerr("%s: %s\n", msg, err->message);
        g_clear_error(&err);
    } else {
        g_printerr("%s\n", msg);
    }
}

// Returns the number of sequence folders inside "sequences/"
int get_number_of_sequences() {
    const gchar *sequences_path = "sequences";
    int count = 0;

    GDir *dir = g_dir_open(sequences_path, 0, NULL);
    if (!dir) return 0;

    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        gchar *path = g_build_filename(sequences_path, name, NULL);
        if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
            count++;
        }
        g_free(path);
    }

    g_dir_close(dir);
    return count;
}

// Drag-and-drop callback
void on_drag_data_received(GtkWidget *widget,
                                  GdkDragContext *context,
                                  gint x, gint y,
                                  GtkSelectionData *data,
                                  guint info, guint time,
                                  gpointer user_data)
{
    GtkWidget *label = GTK_WIDGET(user_data);

    if (gtk_selection_data_get_length(data) >= 0) {
        gchar **uris = gtk_selection_data_get_uris(data);
        for (int i = 0; uris[i] != NULL; i++) {
            gchar *filename = g_filename_from_uri(uris[i], NULL, NULL);
            if (filename) {
                if (is_mp4_file(filename)) {
                    gtk_label_set_text(GTK_LABEL(label), filename);
                    g_warning("Dropped MP4 file: %s", filename);
                } else {
                    g_warning("Ignored non-MP4 file: %s", filename);
                }
                g_free(filename);
            }
        }
        g_strfreev(uris);
    }

    gtk_drag_finish(context, TRUE, FALSE, time);
}

void cleanup_frames_folders(void)
{
    for (int i = 1; i <= MAX_LAYERS; i++) {
        char path[256];
        snprintf(path, sizeof(path), "Frames_%d", i);

        if (!g_file_test(path, G_FILE_TEST_IS_DIR))
            continue;

        /* Try GLib removal first */
        if (g_remove(path) != 0) {
            /* Directory not empty → recursive fallback */
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", path);

            if (system(cmd) != 0) {
                g_warning("Failed to remove folder: %s", path);
            }
        }
    }
}





