#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <glib/gstdio.h>

gboolean is_frames_file_empty(int layer_number)
{
    char folder_path[512];
    snprintf(folder_path, sizeof(folder_path), "Frames_%d", layer_number);

    DIR *dir = opendir(folder_path);
    if (!dir) {
        // Folder does not exist
        return TRUE;
    }

    struct dirent *entry;
    gboolean has_frame = FALSE;

    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Check if filename starts with "frame_" and ends with ".png"
        if (g_str_has_prefix(entry->d_name, "frame_") &&
            g_str_has_suffix(entry->d_name, ".png")) {
            has_frame = TRUE;
            break;
        }
    }

    closedir(dir);

    return !has_frame; // TRUE if empty
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

AphorismErrorCode get_random_aphorism(const char *filename, char **out_str) {
    if (!filename || !out_str) return APHORISM_MEMORY_ERROR;
    *out_str = NULL;

    FILE *file = fopen(filename, "r");
    if (!file) return APHORISM_FILE_NOT_FOUND;

    char buffer[256];
    int lines_count = 0;
    while (fgets(buffer, sizeof(buffer), file)) lines_count++;
    if (lines_count == 0) { fclose(file); return APHORISM_EMPTY_FILE; }

    int pick = rand() % lines_count;
    rewind(file);
    int current_line = 0;
    while (fgets(buffer, sizeof(buffer), file)) if (current_line++ == pick) break;
    fclose(file);

    if (!buffer[0]) return APHORISM_READ_FAILED;

    buffer[strcspn(buffer, "\r\n")] = 0;
    *out_str = g_strdup(buffer);
    if (!*out_str) return APHORISM_MEMORY_ERROR;

    return APHORISM_OK;
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





