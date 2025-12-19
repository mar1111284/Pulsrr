#include "modal_load.h"

static inline gboolean export_done_cb(gpointer data) {
    ExportJob *job = (ExportJob *)data;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(job->progress_bar), 1.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(job->progress_bar), "Export completed");
    set_layer_modified(job->layer_number);
    sdl_set_playing(1);
    //g_timeout_add_seconds(2, close_modal_cb, global_modal_layer);
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
	GtkWidget *selected_file_label = gtk_label_new("Drag an MP4 file here");
	gtk_widget_set_name(selected_file_label, "selected-file-label");
	gtk_box_pack_start(GTK_BOX(black_box), selected_file_label, FALSE, FALSE, 5);

	// Enable drag-and-drop on black_box
	GtkTargetEntry targets[] = {
	{ "text/uri-list", 0, 0 }
	};
	gtk_drag_dest_set(GTK_WIDGET(black_box),GTK_DEST_DEFAULT_ALL,targets,G_N_ELEMENTS(targets),GDK_ACTION_COPY);

	// Button row (Cancel + Export)
	GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(button_row, "button-row");
	gtk_widget_set_size_request(button_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), button_row, FALSE, FALSE, 0);

	// Cancel button
	GtkWidget *btn_cancel = gtk_button_new_with_label("BACK");
	gtk_widget_set_name(btn_cancel, "modal-cancel-button");
	gtk_widget_set_size_request(btn_cancel, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_cancel, TRUE, TRUE, 0);

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
	
	// Export progress bar
	GtkWidget *progress_bar = gtk_progress_bar_new();
	gtk_widget_set_name(progress_bar, "export-progress");
	gtk_widget_set_size_request(progress_bar, 360, 20);

	// Initial state
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "0%");
	gtk_box_pack_end(GTK_BOX(black_box), progress_bar, FALSE, FALSE, 10);
	
	// Kind of context we send
	ExportContext *ctx = g_malloc(sizeof(ExportContext));
	ctx->file_label = selected_file_label;
	ctx->fps_spin = GTK_SPIN_BUTTON(fps_spin);
	ctx->scale_combo = GTK_COMBO_BOX_TEXT(scale_combo);
	ctx->layer_index = layer_index;
	ctx->progress_bar = progress_bar;

	// Signals
	g_signal_connect(black_box, "drag-data-received",G_CALLBACK(on_drag_data_received), selected_file_label);
	g_signal_connect(btn_cancel, "clicked", G_CALLBACK(on_modal_cancel_clicked), global_modal_layer);
	g_signal_connect(btn_export, "clicked", G_CALLBACK(on_export_clicked), ctx);

	gtk_container_add(GTK_CONTAINER(global_modal_layer), black_box);
	g_free(ctx);
	gtk_widget_show_all(global_modal_layer);
}


void on_export_clicked(GtkButton *button, gpointer user_data) {
    ExportContext *ctx = (ExportContext *)user_data;

    const gchar *filename = gtk_label_get_text(GTK_LABEL(ctx->file_label));
    int fps = gtk_spin_button_get_value_as_int(ctx->fps_spin);
    const gchar *scale_text = gtk_combo_box_text_get_active_text(ctx->scale_combo);
    guint8 layer_index = ctx->layer_index;
    GtkWidget *progress_bar = ctx->progress_bar;

    // Determine scale width
    int scale_width = 854; // default 480p
    if (g_strcmp0(scale_text, "1080p") == 0) scale_width = 1920;
    else if (g_strcmp0(scale_text, "720p") == 0) scale_width = 1280;
    else if (g_strcmp0(scale_text, "480p") == 0) scale_width = 854;
    else if (g_strcmp0(scale_text, "360p") == 0) scale_width = 640;

	// Folder name for exported frames
	char folder[64];
	g_snprintf(folder, sizeof(folder), "Frames_%d", layer_index);

	// Create heap-allocated export context
	ExportContext *ctx = g_malloc(sizeof(ExportContext));
	ctx->filename = g_strdup(filename);
	ctx->fps = fps;
	ctx->scale_width = scale_width;
	ctx->folder = g_strdup(folder);
	ctx->progress_bar = progress_bar;
	ctx->layer_index = layer_index;

    // Start thread
    g_thread_new("export-thread", export_thread_func, job);
}

gpointer export_thread_func(gpointer data) {
    ExportJob *job = (ExportJob *)data;

    char line[256];
    double fraction = 0.0;

    // Clean folder
    char clean_cmd[600];
    snprintf(clean_cmd, sizeof(clean_cmd), "mkdir -p \"%s\" && rm -f \"%s\"/*", job->folder, job->folder);
    system(clean_cmd);

    // Run ffmpeg
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "ffmpeg -i \"%s\" -vf scale=%d:-1,fps=%d \"%s/frame_%%05d.png\" -progress - -nostats",
             job->filename, job->scale_width, job->fps, job->folder);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) return NULL;

    while (fgets(line, sizeof(line), pipe)) {
        if (g_str_has_prefix(line, "frame=")) {
            fraction += 0.01;
            if (fraction > 1.0) fraction = 1.0;

            char text[16];
            snprintf(text, sizeof(text), "%d%%", (int)(fraction*100));

            ProgressUpdate *upd = g_malloc(sizeof(ProgressUpdate));
            upd->progress_bar = job->progress_bar;
            upd->fraction = fraction;
            upd->text = g_strdup(text);

            g_idle_add(update_progress_cb, upd);
        }
    }

    pclose(pipe);

    // Final update
    ProgressUpdate *upd = g_malloc(sizeof(ProgressUpdate));
    upd->progress_bar = job->progress_bar;
    upd->fraction = 1.0;
    upd->text = g_strdup("100%");
    g_idle_add(update_progress_cb, upd);

    // ===== DEBUG: print layer number =====
    g_print("[EXPORT_THREAD] Layer number in thread: %d\n", job->layer_number);

    // Notify UI
    g_idle_add(update_preview_idle, GINT_TO_POINTER(job->layer_number));

    g_idle_add(export_done_cb, job);
    return NULL;
}
void on_modal_cancel_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *modal_layer = GTK_WIDGET(user_data);

    gtk_widget_hide(modal_layer);

    // Optional: remove previous modal content
    GList *children = gtk_container_get_children(GTK_CONTAINER(modal_layer));
    for (GList *l = children; l != NULL; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    
    // Render play
    sdl_set_playing(1);
    
    g_list_free(children);
}

gboolean update_progress_cb(gpointer data) {
    ProgressUpdate *upd = (ProgressUpdate *)data;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(upd->progress_bar), upd->fraction);
    if (upd->text)
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(upd->progress_bar), upd->text);

    g_free(upd->text);
    g_free(upd);

    return G_SOURCE_REMOVE; // run once
}
