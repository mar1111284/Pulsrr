#include "modal_download.h"
#define SEQUENCES_DIR "./sequences"

typedef struct {
    AddSequenceUI *ui;
    char *msg;
} LogJob;

// Append log message to the modal
static void log_message(AddSequenceUI *ui, const char *msg) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(ui->log_buffer, &end);
    gtk_text_buffer_insert(ui->log_buffer, &end, msg, -1);
    gtk_text_buffer_insert(ui->log_buffer, &end, "\n", -1);
}

static gboolean log_message_idle(gpointer data) {
    LogJob *job = data;
    log_message(job->ui, job->msg);
    g_free(job->msg);
    g_free(job);
    return G_SOURCE_REMOVE;
}


int get_total_sequences() {
    int count = 0;
    DIR *d = opendir(SEQUENCES_DIR);
    if (!d) return 0;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strncmp(entry->d_name, "sequence_", 9) == 0) {
            count++;
        }
    }
    closedir(d);
    return count;
}

// Update progress bar on UI thread
static gboolean ui_update_progress(gpointer data) {
    ProgressUpdate *u = data;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(u->progress_bar), u->fraction);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(u->progress_bar), u->text);

    g_free(u->text);
    g_free(u);
    return G_SOURCE_REMOVE;
}

// Run FFmpeg in a thread (temp video generation only)
// Run FFmpeg in a thread
static gpointer download_worker(gpointer data)
{
    DownloadJob *job = data;
    int total_sequences = g_list_length(job->sequence_paths);

    // Map scale combo to width:height
    int width = 1280, height = 720;
    const char *scale = job->scale;
    if (strcmp(scale, "1080p") == 0) { width = 1920; height = 1080; }
    else if (strcmp(scale, "720p") == 0) { width = 1280; height = 720; }
    else if (strcmp(scale, "480p") == 0) { width = 854; height = 480; }
    else if (strcmp(scale, "360p") == 0) { width = 640; height = 360; }

    GList *temp_files = NULL;

    for (int i = 0; i < total_sequences; i++) {
        char *seq_path = g_list_nth_data(job->sequence_paths, i);

        // Count frames
        DIR *d = opendir(seq_path);
        int total_frames = 0;
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL) {
                if (entry->d_type == DT_REG &&
                    strstr(entry->d_name, "frame_") != NULL) {
                    total_frames++;
                }
            }
            closedir(d);
        }

        if (total_frames == 0) {
            // Update progress and log warning
            char msg[128];
            snprintf(msg, sizeof(msg), "[WARNING] Sequence %d missing or empty, skipping...", i + 1);

            ProgressUpdate *update = g_new0(ProgressUpdate, 1);
            update->progress_bar = job->ui->progress_bar;
            update->fraction = (double)i / total_sequences;
            update->text = g_strdup(msg);
            g_idle_add(ui_update_progress, update);

            LogJob *lj = g_new0(LogJob, 1);
            lj->ui = job->ui;
            lj->msg = g_strdup(msg);
            g_idle_add(log_message_idle, lj);

            continue;
        }

        // Build temporary output path
        char temp_output[256];
        snprintf(temp_output, sizeof(temp_output), "./sequences/temp_seq_%d.mp4", i + 1);

        // FFmpeg command for this sequence
        char ff_cmd[1024];
        snprintf(ff_cmd, sizeof(ff_cmd),
                 "ffmpeg -y -framerate %d -i \"%s/frame_%%05d.png\" -vf scale=%d:%d \"%s\"",
                 job->fps, seq_path, width, height, temp_output);

        // Log start
        char start_msg[128];
        snprintf(start_msg, sizeof(start_msg), "[INFO] Encoding sequence %d ...", i + 1);
        LogJob *lj_start = g_new0(LogJob, 1);
        lj_start->ui = job->ui;
        lj_start->msg = g_strdup(start_msg);
        g_idle_add(log_message_idle, lj_start);

        // Run FFmpeg
        int ret = system(ff_cmd);
        if (ret != 0) {
            char err_msg[128];
            snprintf(err_msg, sizeof(err_msg), "[ERROR] Failed to encode sequence %d, skipping...", i + 1);

            ProgressUpdate *update = g_new0(ProgressUpdate, 1);
            update->progress_bar = job->ui->progress_bar;
            update->fraction = (double)(i + 1) / total_sequences;
            update->text = g_strdup(err_msg);
            g_idle_add(ui_update_progress, update);

            LogJob *lj_err = g_new0(LogJob, 1);
            lj_err->ui = job->ui;
            lj_err->msg = g_strdup(err_msg);
            g_idle_add(log_message_idle, lj_err);

            continue;
        }

        temp_files = g_list_append(temp_files, g_strdup(temp_output));

        // Log finished
        char finished_msg[128];
        snprintf(finished_msg, sizeof(finished_msg), "[INFO] Finished sequence %d -> %s", i + 1, temp_output);

        ProgressUpdate *update = g_new0(ProgressUpdate, 1);
        update->progress_bar = job->ui->progress_bar;
        update->fraction = (double)(i + 1) / total_sequences;
        update->text = g_strdup(finished_msg);
        g_idle_add(ui_update_progress, update);

        LogJob *lj_fin = g_new0(LogJob, 1);
        lj_fin->ui = job->ui;
        lj_fin->msg = g_strdup(finished_msg);
        g_idle_add(log_message_idle, lj_fin);
    }

    // Concatenate videos if more than one
    if (temp_files) {
        if (g_list_length(temp_files) == 1) {
            // Only one video, rename to output
            char *single = g_list_nth_data(temp_files, 0);
            rename(single, "./sequences/output.mp4");
        } else {
            // Build FFmpeg command: ffmpeg -y -i t1 -i t2 ... -filter_complex "[0:v][1:v]...concat=n=X:v=1:a=0 [v]" -map [v] output.mp4
            char concat_cmd[4096] = "ffmpeg -y ";
            char filter[1024] = "";
            int idx = 0;
            for (GList *l = temp_files; l != NULL; l = l->next, idx++) {
                strcat(concat_cmd, "-i \"");
                strcat(concat_cmd, (char*)l->data);
                strcat(concat_cmd, "\" ");
                
                char tmp[32];
                snprintf(tmp, sizeof(tmp), "[%d:v]", idx);
                strcat(filter, tmp);
            }
            char tmp_filter[128];
            snprintf(tmp_filter, sizeof(tmp_filter), "concat=n=%d:v=1:a=0 [v]", idx);
            strcat(filter, tmp_filter);

            strcat(concat_cmd, "-filter_complex \"");
            strcat(concat_cmd, filter);
            strcat(concat_cmd, "\" -map [v] ./sequences/output.mp4");

            int ret = system(concat_cmd);
            if (ret != 0) {
                LogJob *lj_err = g_new0(LogJob, 1);
                lj_err->ui = job->ui;
                lj_err->msg = g_strdup("[ERROR] Failed to concatenate sequences!");
                g_idle_add(log_message_idle, lj_err);
            }
        }

        // Cleanup temp files
        for (GList *l = temp_files; l != NULL; l = l->next)
            remove((char*)l->data);
        g_list_free_full(temp_files, g_free);
    }

    // Final progress/log
    char *final_msg = "[INFO] Download complete!";
    ProgressUpdate *final_update = g_new0(ProgressUpdate, 1);
    final_update->progress_bar = job->ui->progress_bar;
    final_update->fraction = 1.0;
    final_update->text = g_strdup(final_msg);
    g_idle_add(ui_update_progress, final_update);

    LogJob *lj_final = g_new0(LogJob, 1);
    lj_final->ui = job->ui;
    lj_final->msg = g_strdup(final_msg);
    g_idle_add(log_message_idle, lj_final);

    g_list_free_full(job->sequence_paths, g_free);
    g_free(job);

    return NULL;
}





void on_download_button_clicked(GtkButton *button, gpointer user_data) {
	
	// Only pause if frames exist or not loading
    if (sdl_get_render_state() != RENDER_STATE_NO_FRAMES &&
        sdl_get_render_state() != RENDER_STATE_LOADING) {
        sdl_set_render_state(RENDER_STATE_PAUSE);
    }

	// Clear previous content if any
	GList *children = gtk_container_get_children(GTK_CONTAINER(global_modal_layer));
	for (GList *iter = children; iter != NULL; iter = iter->next) {
	gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	// Modal black box
	GtkWidget *black_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_size_request(black_box,400 ,400);
	gtk_widget_set_name(black_box, "black-modal");
	gtk_widget_set_halign(black_box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(black_box, GTK_ALIGN_CENTER);
	
	// Header label
	GtkWidget *title_label = gtk_label_new("DOWNLOAD");
	gtk_widget_set_name(title_label, "modal-title");
	gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top(title_label, 20);
	gtk_widget_set_margin_bottom(title_label, 20);
	gtk_box_pack_start(GTK_BOX(black_box), title_label, FALSE, FALSE, 0);
	
	// Log area
	GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_name(log_scroll, "sequence-log-scroll");
	gtk_widget_set_size_request(log_scroll, 380, 150);
	gtk_widget_set_margin_start(log_scroll, 20);
	gtk_widget_set_margin_end(log_scroll, 20);
	gtk_widget_set_margin_bottom(log_scroll, 20);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scroll),
		                           GTK_POLICY_AUTOMATIC,
		                           GTK_POLICY_AUTOMATIC);

	GtkWidget *log_view = gtk_text_view_new();
	gtk_widget_set_name(log_view, "sequence-log");
	gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(log_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(log_view), GTK_WRAP_WORD_CHAR);

	GtkTextBuffer *log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
	gtk_text_buffer_set_text(log_buffer, "[INFO] Ready to download…\n", -1);

	gtk_container_add(GTK_CONTAINER(log_scroll), log_view);
	gtk_box_pack_start(GTK_BOX(black_box), log_scroll, TRUE, TRUE, 0);

	// Button row (Back + Add)
	GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(button_row, "button-row");
	gtk_widget_set_size_request(button_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), button_row, FALSE, FALSE, 0);

	// Back button
	GtkWidget *btn_back = gtk_button_new_with_label("BACK");
	gtk_widget_set_name(btn_back, "modal-cancel-button");
	gtk_widget_set_size_request(btn_back, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_back, TRUE, TRUE, 0);

	// Add button
	GtkWidget *btn_dl_sequence = gtk_button_new_with_label("Download");
	gtk_widget_set_name(btn_dl_sequence, "modal-download-button");
	gtk_widget_set_size_request(btn_dl_sequence, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_dl_sequence, TRUE, TRUE, 0);

	// Controls row: FPS + Scale
	GtkWidget *controls_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(controls_row, "controls-row");
	gtk_widget_set_size_request(controls_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), controls_row, FALSE, FALSE, 5);

	GtkWidget *fps_spin = gtk_spin_button_new_with_range(1, 60, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(fps_spin), 25);
	gtk_widget_set_name(controls_row, "spin-button");
	gtk_widget_set_size_request(fps_spin, 150, 42);
	gtk_box_pack_start(GTK_BOX(controls_row), fps_spin, TRUE, TRUE, 0);

	// Scale combo box
	GtkWidget *scale_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "1080p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "720p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "480p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "360p");
	gtk_combo_box_set_active(GTK_COMBO_BOX(scale_combo), 2);
	gtk_widget_set_size_request(scale_combo, 150, 42);
	gtk_box_pack_start(GTK_BOX(controls_row), scale_combo, TRUE, TRUE, 0);
	
	// Labels row: "FPS" and "Quality"
	GtkWidget *labels_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_size_request(labels_row, 400, 20);

	// FPS & Quality label
	GtkWidget *fps_label = gtk_label_new("FPS");
	gtk_widget_set_halign(fps_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(labels_row), fps_label, TRUE, TRUE, 0);
	GtkWidget *quality_label = gtk_label_new("Quality");
	gtk_widget_set_halign(quality_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(labels_row), quality_label, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(black_box), labels_row, FALSE, FALSE, 0);
	
	// Adding progress bar
	GtkWidget *progress_bar = gtk_progress_bar_new();
	gtk_widget_set_name(progress_bar, "export-progress");
	gtk_widget_set_size_request(progress_bar, 360, 20);
	
	// Initial state of progress bar
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "0%");
	gtk_box_pack_end(GTK_BOX(black_box), progress_bar, FALSE, FALSE, 10);
	
	// Populate SequcneUI
	AddSequenceUI *ui = g_new0(AddSequenceUI, 1);
	ui->fps_spin = GTK_SPIN_BUTTON(fps_spin);
	ui->scale_combo = GTK_COMBO_BOX_TEXT(scale_combo);
	ui->log_view = log_view;
	ui->log_buffer = log_buffer;
	ui->progress_bar = progress_bar;
	ui->root_container = GTK_WIDGET(user_data);
	ui->parent_container = black_box;

	g_signal_connect(btn_dl_sequence, "clicked", G_CALLBACK(on_dl_sequence_clicked), ui);
	g_signal_connect(btn_back, "clicked", G_CALLBACK(on_modal_dl_back_clicked), global_modal_layer);

	gtk_container_add(GTK_CONTAINER(global_modal_layer), black_box);
	gtk_widget_show_all(global_modal_layer);
}

// Called when user clicks "Download"
void on_dl_sequence_clicked(GtkButton *button, gpointer user_data) {
    AddSequenceUI *ui = user_data;

    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui->fps_spin), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui->scale_combo), FALSE);

    const char *scale = gtk_combo_box_text_get_active_text(ui->scale_combo);
    int fps = gtk_spin_button_get_value_as_int(ui->fps_spin);

    // Build list of sequence frame folders
    GList *seq_list = NULL;
    int total_sequences = get_total_sequences();
    for (int i = 0; i < total_sequences; i++) {
        char path[256];
        snprintf(path, sizeof(path), "sequences/sequence_%d/mixed_frames", i + 1);
        seq_list = g_list_append(seq_list, g_strdup(path));
    }

    DownloadJob *job = g_new0(DownloadJob, 1);
    job->ui = ui;
    job->fps = fps;
    job->scale = scale;
    job->sequence_paths = seq_list;

    g_thread_new("download-worker", download_worker, job);
}


// Placeholder for the modal "Back" button
void on_modal_dl_back_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *modal_layer = GTK_WIDGET(user_data);
    g_print("[INFO] Modal back clicked\n");
    gtk_widget_hide(modal_layer);

    // Optional: destroy children if needed
    GList *children = gtk_container_get_children(GTK_CONTAINER(modal_layer));
    for (GList *l = children; l != NULL; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);
}

