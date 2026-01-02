#include "utils.h"
#include "sdl.h"
#include "sdl_utilities.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>

// Global MainUI instance
MainUI g_main_ui = {0};

// Helper: count files in a folder
int count_files_in_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return 0;

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) count++;
    }
    closedir(dir);
    return count;
}

MainUI* main_ui_get(void) {
    return &g_main_ui;
}

// Thread-safe logging
static gboolean add_log_idle(gpointer data) {
    LogIdleData *ld = (LogIdleData *)data;
    MainUI *ui = main_ui_get();
    if (ui && ui->log_buffer) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(ui->log_buffer, &end);
        gtk_text_buffer_insert(ui->log_buffer, &end, ld->msg, -1);
        gtk_text_buffer_insert(ui->log_buffer, &end, "\n", -1);

        GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(ui->log_view));
        gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
    }
    g_free(ld->msg);
    g_free(ld);
    return G_SOURCE_REMOVE;
}

void add_main_log(const char *message) {
    if (!message) return;
    LogIdleData *ld = g_new0(LogIdleData, 1);
    ld->msg = g_strdup(message);
    g_idle_add(add_log_idle, ld);
}

// File & folder utilities
void ensure_dir(const char *path) {
    if (mkdir(path, 0755) == -1 && errno != EEXIST) {
        add_main_log(g_strdup_printf("[ERROR] Failed to create folder: %s", path));
    }
}

int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;

    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }

    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);

    fclose(in);
    fclose(out);
    return 0;
}

void copy_directory(const char *src, const char *dst) {
    ensure_dir(dst);
    DIR *dir = opendir(src);
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (ent->d_type != DT_REG || ent->d_name[0] == '.') continue;

        gchar *src_path = g_build_filename(src, ent->d_name, NULL);
        gchar *dst_path = g_build_filename(dst, ent->d_name, NULL);

        copy_file(src_path, dst_path);

        g_free(src_path);
        g_free(dst_path);
    }
    closedir(dir);
}

void cleanup_frames_folders(void) {
    for (int i = 1; i <= MAX_LAYERS; i++) {
        gchar *path = g_strdup_printf("Frames_%d", i);
        if (!g_file_test(path, G_FILE_TEST_IS_DIR)) { g_free(path); continue; }

        if (g_remove(path) != 0) {
            gchar *cmd = g_strdup_printf("rm -rf \"%s\"", path);
            if (system(cmd) != 0) add_main_log(g_strdup_printf("[ERROR] Failed to remove folder: %s", path));
            g_free(cmd);
        }
        g_free(path);
    }
}

int count_frames(const char *folder) {
    DIR *d = opendir(folder);
    if (!d) return 0;

    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(d)) != NULL) if (strstr(entry->d_name, ".png")) count++;
    closedir(d);
    return count;
}

// FFmpeg utilities
int encode_sequence_with_ffmpeg(int sequence_number, int fps, int width, int height) {
    gchar *seq_dir = g_strdup_printf("sequences/sequence_%d", sequence_number);
    gchar *frames_dir = g_build_filename(seq_dir, "mixed_frames", NULL);
    gchar *output_mp4 = g_strdup_printf("%s/sequence_%d.mp4", seq_dir, sequence_number);

    if (!g_file_test(frames_dir, G_FILE_TEST_IS_DIR)) {
        add_main_log(g_strdup_printf("[FFMPEG] mixed_frames folder not found: %s", frames_dir));
        goto fail;
    }

    gchar *cmd = g_strdup_printf(
        "ffmpeg -y -framerate %d -i \"%s/frame_%%05d.png\" -s %dx%d -pix_fmt yuv420p -c:v libx264 \"%s\"",
        fps, frames_dir, width, height, output_mp4
    );

    int ret = system(cmd);
    if (ret != 0) {
        add_main_log(g_strdup_printf("[FFMPEG] Encoding failed (code=%d)", ret));
        g_free(cmd);
        goto fail;
    }

    add_main_log("[FFMPEG] Video generated successfully.");

    g_free(cmd);
    g_free(seq_dir);
    g_free(frames_dir);
    g_free(output_mp4);
    return 0;

fail:
    g_free(seq_dir);
    g_free(frames_dir);
    g_free(output_mp4);
    return -1;
}

// Modal utilities
void on_modal_back_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *modal_layer = GTK_WIDGET(user_data);
    gtk_widget_hide(modal_layer);

    GList *children = gtk_container_get_children(GTK_CONTAINER(modal_layer));
    for (GList *l = children; l != NULL; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    // Only play if frames exist or not loading
    if (sdl_get_render_state() != RENDER_STATE_NO_FRAMES &&
        sdl_get_render_state() != RENDER_STATE_LOADING) {
        sdl_set_render_state(RENDER_STATE_PLAY);
    }
}

// File info utilities
guint get_duration_in_seconds(const gchar *file_path) {
    if (!file_path || !*file_path) return 0;

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "ffprobe -v error -show_entries format=duration "
        "-of default=noprint_wrappers=1:nokey=1 \"%s\"", file_path
    );

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return 0;

    char buf[128] = {0};
    if (!fgets(buf, sizeof(buf), pipe)) { pclose(pipe); return 0; }
    pclose(pipe);

    double seconds = g_ascii_strtod(buf, NULL);
    return (guint)(seconds + 0.5);
}

gchar* get_resolution(const gchar *file_path) {
    gchar cmd[512];
    snprintf(cmd, sizeof(cmd),
        "ffprobe -v error -select_streams v:0 -show_entries stream=width,height "
        "-of csv=p=0 \"%s\"", file_path
    );

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return g_strdup("--x--");

    char buffer[64];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        buffer[strcspn(buffer, "\n")] = 0;
        pclose(pipe);
        return g_strdup(buffer);
    }

    pclose(pipe);
    return g_strdup("--x--");
}

gchar* get_fps(const gchar *file_path) {
    gchar cmd[512];
    snprintf(cmd, sizeof(cmd),
        "ffprobe -v 0 -of csv=p=0 -select_streams v:0 -show_entries stream=r_frame_rate \"%s\"", file_path
    );

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return g_strdup("--");

    char buffer[32];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        buffer[strcspn(buffer, "\n")] = 0;

        int num, den;
        if (sscanf(buffer, "%d/%d", &num, &den) == 2 && den != 0) {
            pclose(pipe);
            return g_strdup_printf("%d", num / den);
        }

        pclose(pipe);
        return g_strdup(buffer);
    }

    pclose(pipe);
    return g_strdup("--");
}

gchar* get_duration(const gchar *file_path) {
    gchar cmd[512];
    snprintf(cmd, sizeof(cmd),
        "ffprobe -v error -show_entries format=duration "
        "-of default=noprint_wrappers=1:nokey=1 \"%s\"", file_path
    );

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

gchar* get_filesize(const gchar *file_path) {
    GStatBuf st;
    if (g_stat(file_path, &st) == 0) {
        double size = st.st_size;
        const char *units[] = {"B", "KB", "MB", "GB"};
        int unit = 0;
        while (size > 1024 && unit < 3) { size /= 1024; unit++; }
        return g_strdup_printf("%.1f %s", size, units[unit]);
    }
    return g_strdup("--");
}

int get_width_from_resolution(const gchar *res) {
    int width = 0;
    if (res) sscanf(res, "%dx", &width);
    return width > 0 ? width : 854;
}

// Sequence utilities
int get_next_sequence_index(void) {
    int idx = 1;
    while (1) {
        gchar *path = g_strdup_printf("sequences/sequence_%d", idx);
        if (access(path, F_OK) != 0) { g_free(path); return idx; }
        g_free(path);
        idx++;
    }
}

int get_number_of_sequences() {
    const gchar *sequences_path = "sequences";
    int count = 0;
    GDir *dir = g_dir_open(sequences_path, 0, NULL);
    if (!dir) return 0;

    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        gchar *path = g_build_filename(sequences_path, name, NULL);
        if (g_file_test(path, G_FILE_TEST_IS_DIR)) count++;
        g_free(path);
    }

    g_dir_close(dir);
    return count;
}

