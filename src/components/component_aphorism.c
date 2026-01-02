#include "../utils/utils.h"
#include "component_aphorism.h"
#include <glib.h>

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

GtkWidget *create_aphorism_widget(void) {
    char *aphorism = NULL;
	const AppPaths *paths = get_app_paths();
	char *aph_path = g_build_filename(paths->media_dir, "aphorisms.txt", NULL);

	AphorismErrorCode err = get_random_aphorism(aph_path, &aphorism);

	g_free(aph_path);

    /* Container */
    GtkWidget *aphorism_box = gtk_event_box_new();
    gtk_widget_set_hexpand(aphorism_box, FALSE);
    gtk_widget_set_vexpand(aphorism_box, FALSE);
    gtk_widget_set_halign(aphorism_box, GTK_ALIGN_CENTER);

    gtk_widget_set_size_request(aphorism_box, 200, -1);
    gtk_widget_set_name(aphorism_box, "aphorism-box");

    /* Label */
    GtkWidget *aphorism_label;
    if (err == APHORISM_OK && aphorism) {
        aphorism_label = gtk_label_new(aphorism);
        g_free(aphorism);
    } else {
        g_warning("Could not get aphorism (error code: %d)", err);
        aphorism_label = gtk_label_new("Loop it till it hurts.");
    }

    gtk_label_set_xalign(GTK_LABEL(aphorism_label), 0.5);
    gtk_label_set_justify(GTK_LABEL(aphorism_label), GTK_JUSTIFY_CENTER);
    gtk_label_set_line_wrap(GTK_LABEL(aphorism_label), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(aphorism_label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_width_chars(GTK_LABEL(aphorism_label), 30);
    gtk_label_set_max_width_chars(GTK_LABEL(aphorism_label), 30);

    gtk_container_add(GTK_CONTAINER(aphorism_box), aphorism_label);

    return aphorism_box;
}

