#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

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




