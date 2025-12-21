#include "modal_load.h"

guint estimation_timeout_id = 0;

gboolean debounce_update_estimation(gpointer user_data) {
    VideoInfoLabels *labels = (VideoInfoLabels *)user_data;
    update_export_estimation(labels);
    estimation_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

void update_export_estimation(VideoInfoLabels *labels)
{
    const gchar *filename = gtk_label_get_text(labels->filename);
    if (!filename || !*filename)
        return;

    guint fps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(labels->fps_spin));
    guint dur_seconds = get_duration_in_seconds(filename);
    guint est_frames = fps * dur_seconds;

    gchar buf[32];
    g_snprintf(buf, sizeof(buf), "%u", est_frames);
    gtk_label_set_text(labels->estimated_frames_nb, buf);

    const gchar *scale_text = gtk_combo_box_text_get_active_text(labels->scale_combo); // Parse scale to width

    int export_width = 854; // default 480p
    if (g_strcmp0(scale_text, "1080p") == 0) export_width = 1920;
    else if (g_strcmp0(scale_text, "720p") == 0) export_width = 1280;
    else if (g_strcmp0(scale_text, "360p") == 0) export_width = 640;

    // Original resolution
    gchar *res_str = get_resolution(filename);
    int orig_w = 0, orig_h = 0;

    if (res_str && sscanf(res_str, "%d x %d", &orig_w, &orig_h) == 2) {
        // ok
    } else {
        orig_w = export_width;
        orig_h = (int)(export_width * 9.0 / 16.0);
    }
    g_free(res_str);

    // Preserve aspect ratio
    int export_h = (int)((double)export_width * orig_h / orig_w);

    // Size estimate
    gdouble bytes_per_frame = export_width * export_h * 0.7;
    gdouble total_mb =
        (bytes_per_frame * est_frames) / (1024.0 * 1024.0);

    gchar size_buf[32];
    g_snprintf(size_buf, sizeof(size_buf), "%.2f MB", total_mb);
    gtk_label_set_text(labels->estimated_size, size_buf);
}

static gboolean idle_set_preview_thumbnail(gpointer user_data) {
    guint8 layer = GPOINTER_TO_UINT(user_data);
    set_preview_thumbnail(layer);
    return G_SOURCE_REMOVE;
}

static inline gboolean export_done_cb(gpointer data) {
    ExportContext *ctx = (ExportContext *)data;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ctx->progress_bar), 1.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ctx->progress_bar), "Export completed");
    set_layer_modified(ctx->layer_index);
    sdl_set_playing(1);
    return G_SOURCE_REMOVE;
}

void on_load_button_clicked(GtkButton *button, gpointer user_data) {

	sdl_set_playing(0);

	guint8 layer_index = GPOINTER_TO_INT(user_data);

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

	// Drag and drop label
	GtkWidget *file_path = gtk_label_new("Drag an MP4 file here");
	gtk_widget_set_name(file_path, "selected-file-label");
	gtk_label_set_ellipsize(GTK_LABEL(file_path), PANGO_ELLIPSIZE_END);
	gtk_label_set_max_width_chars(GTK_LABEL(file_path), 40);
	gtk_label_set_line_wrap(GTK_LABEL(file_path), TRUE);
	gtk_widget_set_size_request(file_path, 360, -1);
	gtk_box_pack_start(GTK_BOX(black_box), file_path, FALSE, FALSE, 5);

	// Enable drag-and-drop on black_box
	GtkTargetEntry targets[] = {
	{ "text/uri-list", 0, 0 }
	};
	gtk_drag_dest_set(GTK_WIDGET(black_box),GTK_DEST_DEFAULT_ALL,targets,G_N_ELEMENTS(targets),GDK_ACTION_COPY);
	
	// Video Info container
	GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,30); 
	    
	// Left column & right column
	GtkWidget *col1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // spacing between rows
	gtk_box_pack_start(GTK_BOX(info_box), col1, TRUE, TRUE, 0);
	GtkWidget *col2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_pack_start(GTK_BOX(info_box), col2, TRUE, TRUE, 0);

	// Row 1: Resolution
	GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkLabel *label_resolution = GTK_LABEL(gtk_label_new("Resolution:"));
	GtkLabel *value_resolution = GTK_LABEL(gtk_label_new("--"));
	gtk_widget_set_halign(GTK_WIDGET(label_resolution), GTK_ALIGN_START);
	gtk_widget_set_halign(GTK_WIDGET(value_resolution), GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(hbox2), GTK_WIDGET(label_resolution), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox2), GTK_WIDGET(value_resolution), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(col1), hbox2, FALSE, FALSE, 0);

	// Row 2: FPS
	GtkWidget *hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkLabel *label_fps = GTK_LABEL(gtk_label_new("FPS:"));
	GtkLabel *value_fps = GTK_LABEL(gtk_label_new("--"));
	gtk_widget_set_halign(GTK_WIDGET(label_fps), GTK_ALIGN_START);
	gtk_widget_set_halign(GTK_WIDGET(value_fps), GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(hbox3), GTK_WIDGET(label_fps), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox3), GTK_WIDGET(value_fps), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(col1), hbox3, FALSE, FALSE, 0);

	// Right column Row 1: Duration
	GtkWidget *hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkLabel *label_duration = GTK_LABEL(gtk_label_new("Duration:"));
	GtkLabel *value_duration = GTK_LABEL(gtk_label_new("--:--"));
	gtk_widget_set_halign(GTK_WIDGET(label_duration), GTK_ALIGN_START);
	gtk_widget_set_halign(GTK_WIDGET(value_duration), GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(hbox4), GTK_WIDGET(label_duration), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox4), GTK_WIDGET(value_duration), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(col2), hbox4, FALSE, FALSE, 0);

	// Right column Row 2: File size
	GtkWidget *hbox5 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkLabel *label_size = GTK_LABEL(gtk_label_new("File size:"));
	GtkLabel *value_size = GTK_LABEL(gtk_label_new("--"));
	gtk_widget_set_halign(GTK_WIDGET(label_size), GTK_ALIGN_START);
	gtk_widget_set_halign(GTK_WIDGET(value_size), GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(hbox5), GTK_WIDGET(label_size), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox5), GTK_WIDGET(value_size), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(col2), hbox5, FALSE, FALSE, 0);

	// Pack info_box under the drag-and-drop label
	gtk_box_pack_start(GTK_BOX(black_box), info_box, FALSE, FALSE, 10);

	// Container for export estimates (under video info)
	GtkWidget *estimates_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

	GtkWidget *header_label = gtk_label_new("Estimation:");
	gtk_widget_set_halign(header_label, GTK_ALIGN_START);
	gtk_widget_set_name(header_label, "estimation-header");
	gtk_box_pack_start(GTK_BOX(estimates_box), header_label, FALSE, FALSE, 0);

	// Row: Expected frames + Estimated size (same row)
	GtkWidget *hbox_estimates = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);

	// Left: Expected frames
	GtkWidget *frames_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkLabel *label_estimated_frames = GTK_LABEL(gtk_label_new("Expected frames:"));
	GtkLabel *value_estimated_frames = GTK_LABEL(gtk_label_new("--"));
	gtk_widget_set_halign(GTK_WIDGET(label_estimated_frames), GTK_ALIGN_START);
	gtk_widget_set_halign(GTK_WIDGET(value_estimated_frames), GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(frames_box), GTK_WIDGET(label_estimated_frames), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(frames_box), GTK_WIDGET(value_estimated_frames), TRUE, TRUE, 0);

	// Right: Estimated size
	GtkWidget *size_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkLabel *label_estimated_size = GTK_LABEL(gtk_label_new("Estimated size:"));
	GtkLabel *value_estimated_size = GTK_LABEL(gtk_label_new("--"));
	gtk_widget_set_halign(GTK_WIDGET(label_estimated_size), GTK_ALIGN_START);
	gtk_widget_set_halign(GTK_WIDGET(value_estimated_size), GTK_ALIGN_END);
	gtk_box_pack_start(GTK_BOX(size_box), GTK_WIDGET(label_estimated_size), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(size_box), GTK_WIDGET(value_estimated_size), TRUE, TRUE, 0);

	// Pack left and right into the same row
	gtk_box_pack_start(GTK_BOX(hbox_estimates), frames_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_estimates), size_box, TRUE, TRUE, 0);

	// Pack the whole row into estimates_box
	gtk_box_pack_start(GTK_BOX(estimates_box), hbox_estimates, FALSE, FALSE, 0);

	// Pack estimates_box under the info_box
	gtk_box_pack_start(GTK_BOX(black_box), estimates_box, FALSE, FALSE, 10);

	// Button row (Back + Export)
	GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(button_row, "button-row");
	gtk_widget_set_size_request(button_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), button_row, FALSE, FALSE, 0);

	// Back button
	GtkWidget *btn_back = gtk_button_new_with_label("BACK");
	gtk_widget_set_name(btn_back, "modal-cancel-button");
	gtk_widget_set_size_request(btn_back, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_back, TRUE, TRUE, 0);

	// Export button
	GtkWidget *btn_export = gtk_button_new_with_label("EXPORT");
	gtk_widget_set_name(btn_export, "modal-export-button");
	gtk_widget_set_size_request(btn_export, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_export, TRUE, TRUE, 0);

	// Controls row: FPS + Scale
	GtkWidget *controls_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(controls_row, "controls-row");
	gtk_widget_set_size_request(controls_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), controls_row, FALSE, FALSE, 5);

	// FPS spin button
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
	
	// Export progress bar
	GtkWidget *progress_bar = gtk_progress_bar_new();
	gtk_widget_set_name(progress_bar, "export-progress");
	gtk_widget_set_size_request(progress_bar, 360, 20);
	
	// Initial state of progress bar
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "0%");
	gtk_box_pack_end(GTK_BOX(black_box), progress_bar, FALSE, FALSE, 10);
	
	// Allocate and assign the labels to the struct
	VideoInfoLabels *video_info = g_malloc(sizeof(VideoInfoLabels));
	video_info->filename = GTK_LABEL(file_path);
	video_info->resolution = GTK_LABEL(value_resolution);
	video_info->fps        = GTK_LABEL(value_fps);
	video_info->duration   = GTK_LABEL(value_duration);
	video_info->filesize   = GTK_LABEL(value_size);
	video_info->estimated_frames_nb = GTK_LABEL(value_estimated_frames);
	video_info->estimated_size      = GTK_LABEL(value_estimated_size);
	video_info->fps_spin = fps_spin;
	video_info->scale_combo = GTK_COMBO_BOX_TEXT(scale_combo);
	video_info->progress_bar = progress_bar;

	// Connect drag-and-drop signal on the black box
	g_signal_connect(black_box,"drag-data-received",G_CALLBACK(on_drag_data_received),video_info);

	// We send the ui context to the export function
	ExportUIContext *ui = g_malloc(sizeof *ui);
	ui->file_label = file_path;
	ui->fps_spin = GTK_SPIN_BUTTON(fps_spin);
	ui->scale_combo = GTK_COMBO_BOX_TEXT(scale_combo);
	ui->progress_bar = progress_bar;
	ui->layer_index = layer_index;

	// Signals
	g_signal_connect(fps_spin, "value-changed",G_CALLBACK(on_fps_spin_changed),video_info);
    g_signal_connect(scale_combo,"changed",G_CALLBACK(on_scale_combo_changed),video_info);

	g_signal_connect(btn_back, "clicked", G_CALLBACK(on_modal_back_clicked), global_modal_layer);
	g_signal_connect(btn_export, "clicked", G_CALLBACK(on_export_clicked), ui);

	gtk_container_add(GTK_CONTAINER(global_modal_layer), black_box);
	gtk_widget_show_all(global_modal_layer);
}

void on_fps_spin_changed(GtkSpinButton *spin, gpointer user_data) {
    VideoInfoLabels *labels = (VideoInfoLabels *)user_data;

    if (estimation_timeout_id != 0)
        g_source_remove(estimation_timeout_id);

    estimation_timeout_id = g_timeout_add(400, debounce_update_estimation, labels);
}

void on_scale_combo_changed(GtkComboBox *combo, gpointer user_data) {
    VideoInfoLabels *labels = (VideoInfoLabels *)user_data;

    if (estimation_timeout_id != 0)
        g_source_remove(estimation_timeout_id);

    estimation_timeout_id = g_timeout_add(400, debounce_update_estimation, labels);
}

// Drag-and-drop callback
void on_drag_data_received(GtkWidget *widget,
                           GdkDragContext *context,
                           gint x, gint y,
                           GtkSelectionData *data,
                           guint info, guint time,
                           gpointer user_data)
{
    VideoInfoLabels *labels = (VideoInfoLabels *)user_data;
	
    if (gtk_selection_data_get_length(data) >= 0) {
        gchar **uris = gtk_selection_data_get_uris(data);
        for (int i = 0; uris[i] != NULL; i++) {
            gchar *filename = g_filename_from_uri(uris[i], NULL, NULL);
            if (filename) {
                if (is_mp4_file(filename)) {
                	
                	// Reset progress bar
                	if (labels->progress_bar) {
						gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(labels->progress_bar), 0.0);
						gtk_progress_bar_set_text(GTK_PROGRESS_BAR(labels->progress_bar), "0%");
					}
                    
                    // Update infos
                    gtk_label_set_text(labels->filename, filename);
            
                    gchar *res = get_resolution(filename);
                    gtk_label_set_text(labels->resolution, res);
                    g_free(res);

                    gchar *fps = get_fps(filename);
                    gtk_label_set_text(labels->fps, fps);
                    g_free(fps);

                    gchar *dur = get_duration(filename);
                    gtk_label_set_text(labels->duration, dur);
                    g_free(dur);

                    gchar *size = get_filesize(filename);
                    gtk_label_set_text(labels->filesize, size);
                    g_free(size);

                    update_export_estimation((VideoInfoLabels *)user_data);

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

void on_export_clicked(GtkButton *button, gpointer user_data) {
    ExportUIContext *ui = (ExportUIContext *)user_data;

    // Read values from UI widgets
    const gchar *label_text = gtk_label_get_text(GTK_LABEL(ui->file_label));
    guint8 fps = gtk_spin_button_get_value_as_int(ui->fps_spin);
    const gchar *scale_text = gtk_combo_box_text_get_active_text(ui->scale_combo);
    guint8 layer_index = ui->layer_index;
    GtkWidget *progress_bar = ui->progress_bar;

    // Determine resolution
    int resolution = 854; // default 480p
    if (g_strcmp0(scale_text, "1080p") == 0) resolution = 1920;
    else if (g_strcmp0(scale_text, "720p") == 0) resolution = 1280;
    else if (g_strcmp0(scale_text, "480p") == 0) resolution = 854;
    else if (g_strcmp0(scale_text, "360p") == 0) resolution = 640;

    // Folder name for exported frames
    gchar folder[64];
    g_snprintf(folder, sizeof(folder), "Frames_%d", layer_index + 1);

    // Allocated thread context
    ExportContext *th_ctx = g_malloc0(sizeof(ExportContext));
    th_ctx->file_path = g_strdup(label_text);
    th_ctx->fps = fps;
    th_ctx->resolution = resolution;
    th_ctx->folder = g_strdup(folder);
    th_ctx->progress_bar = progress_bar;
    th_ctx->layer_index = layer_index;

    // Start export thread
    g_thread_new("export-thread", export_thread_func, th_ctx);
}

gpointer export_thread_func(gpointer data) {
    ExportContext *ctx = (ExportContext *)data;

    // Build absolute folder path
    gchar *folder_abs = g_build_filename(g_get_current_dir(), ctx->folder, NULL);

    // Ensure folder exists
    if (g_mkdir_with_parents(folder_abs, 0755) != 0) {
        g_warning("Failed to create folder: %s", folder_abs);
        g_free(folder_abs);
        return NULL;
    }

    // Clean existing files in folder
    GError *err = NULL;
    GDir *dir = g_dir_open(folder_abs, 0, &err);
    if (dir) {
        const gchar *name;
        while ((name = g_dir_read_name(dir))) {
            gchar *path = g_build_filename(folder_abs, name, NULL);
            unlink(path);
            g_free(path);
        }
        g_dir_close(dir);
    } else if (err) {
        g_warning("Failed to open folder for cleaning: %s", err->message);
        g_error_free(err);
    }

    // Total duration in milliseconds
    guint64 total_ms = get_duration_in_seconds(ctx->file_path) * 1000;

    // FFmpeg command
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "ffmpeg -i \"%s\" -vf scale=%d:-1,fps=%d \"%s/frame_%%05d.png\" -progress pipe:1 -nostats -loglevel info",
             ctx->file_path,
             ctx->resolution,
             ctx->fps,
             folder_abs
    );
    
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        g_warning("Failed to run FFmpeg!");
        g_free(folder_abs);
        return NULL;
    }

    char line[256];
    while (fgets(line, sizeof(line), pipe)) {
        line[strcspn(line, "\r\n")] = 0; // trim newline

        if (g_str_has_prefix(line, "out_time_ms=")) {
            guint64 out_ms = 0;
            sscanf(line + strlen("out_time_ms="), "%lu", &out_ms);
            double fraction = (double)out_ms / (double)(total_ms * 1000);
            if (fraction > 1.0) fraction = 1.0;

            char text[16];
            snprintf(text, sizeof(text), "%d%%", (int)(fraction * 100));

            ProgressUpdate *upd = g_malloc(sizeof(ProgressUpdate));
            upd->progress_bar = ctx->progress_bar;
            upd->fraction = fraction;
            upd->text = g_strdup(text);
            g_idle_add(update_progress_cb, upd);
        }
    }

    pclose(pipe);
    g_free(folder_abs);

    // Final progress update
    ProgressUpdate *upd = g_malloc(sizeof(ProgressUpdate));
    upd->progress_bar = ctx->progress_bar;
    upd->fraction = 1.0;
    upd->text = g_strdup("100%");
    g_idle_add(update_progress_cb, upd);

    // Update thumbnail and mark done
    g_idle_add(idle_set_preview_thumbnail, GUINT_TO_POINTER(ctx->layer_index));
    g_idle_add(export_done_cb, ctx);

    return NULL;
}



gboolean update_progress_cb(gpointer data) {
    ProgressUpdate *upd = (ProgressUpdate *)data;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(upd->progress_bar), upd->fraction);
    if (upd->text)
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(upd->progress_bar), upd->text);

    g_free(upd->text);
    g_free(upd);

    return G_SOURCE_REMOVE;
}
