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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

guint get_duration_in_seconds(const gchar *file_path)
{
    if (!file_path || !*file_path)
        return 0;

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "ffprobe -v error "
        "-show_entries format=duration "
        "-of default=noprint_wrappers=1:nokey=1 "
        "\"%s\"",
        file_path
    );

    FILE *pipe = popen(cmd, "r");
    if (!pipe)
        return 0;

    char buf[128] = {0};
    if (!fgets(buf, sizeof(buf), pipe)) {
        pclose(pipe);
        return 0;
    }

    pclose(pipe);

    // ffprobe returns seconds as float (e.g. 205.4321)
    double seconds = g_ascii_strtod(buf, NULL);
    if (seconds < 0)
        seconds = 0;

    return (guint)(seconds + 0.5); // round to nearest second
}


int get_width_from_resolution(const gchar *res) {
    int width = 0;
    if (res) sscanf(res, "%dx", &width);
    return width > 0 ? width : 854; // default to 480p width
}


// Get resolution in "WIDTHxHEIGHT" format
gchar* get_resolution(const gchar *file_path) {
    gchar cmd[512];
    snprintf(cmd, sizeof(cmd),
             "ffprobe -v error -select_streams v:0 -show_entries stream=width,height "
             "-of csv=p=0 \"%s\"", file_path);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return g_strdup("--x--");

    char buffer[64];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        // Remove newline
        buffer[strcspn(buffer, "\n")] = 0;
        pclose(pipe);
        return g_strdup(buffer);
    }

    pclose(pipe);
    return g_strdup("--x--");
}

// Get FPS as string
gchar* get_fps(const gchar *file_path) {
    gchar cmd[512];
    snprintf(cmd, sizeof(cmd),
             "ffprobe -v 0 -of csv=p=0 -select_streams v:0 -show_entries stream=r_frame_rate \"%s\"",
             file_path);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return g_strdup("--");

    char buffer[32];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        buffer[strcspn(buffer, "\n")] = 0;

        // Evaluate FPS fraction if needed (e.g., "30000/1001")
        int num, den;
        if (sscanf(buffer, "%d/%d", &num, &den) == 2 && den != 0) {
            int fps = num / den;
            pclose(pipe);
            return g_strdup_printf("%d", fps);
        }

        pclose(pipe);
        return g_strdup(buffer);
    }

    pclose(pipe);
    return g_strdup("--");
}

// Get duration as HH:MM:SS
gchar* get_duration(const gchar *file_path) {
    gchar cmd[512];
    snprintf(cmd, sizeof(cmd),
             "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"%s\"",
             file_path);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return g_strdup("--:--");

    char buffer[32];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        double seconds = atof(buffer);
        int h = (int)(seconds / 3600);
        int m = ((int)seconds % 3600) / 60;
        int s = (int)seconds % 60;
        pclose(pipe);
        return g_strdup_printf("%02d:%02d:%02d", h, m, s);
    }

    pclose(pipe);
    return g_strdup("--:--");
}

// Get file size as string
gchar* get_filesize(const gchar *file_path) {
    GStatBuf st;
    if (g_stat(file_path, &st) == 0) {
        double size = st.st_size;
        const char *units[] = {"B", "KB", "MB", "GB"};
        int unit = 0;
        while (size > 1024 && unit < 3) {
            size /= 1024;
            unit++;
        }
        return g_strdup_printf("%.1f %s", size, units[unit]);
    }
    return g_strdup("--");
}


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





