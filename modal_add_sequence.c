#include "modal_add_sequence.h"
#include "utils.h"
#include "sdl_utilities.h"

void set_progress_add_sequence(AddSequenceUI *ui, double fraction, const char *text)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ui->progress_bar), fraction);

    if (text) {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ui->progress_bar), text);
    }

    while (gtk_events_pending())
        gtk_main_iteration();
}

void on_add_button_clicked(GtkButton *button, gpointer user_data) {
	
	sdl_set_playing(0);

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
	GtkWidget *title_label = gtk_label_new("ADD SEQUENCE");
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
	gtk_text_buffer_set_text(log_buffer, "[INFO] Ready to generate sequence…\n", -1);

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
	GtkWidget *btn_add_sequence = gtk_button_new_with_label("EXPORT");
	gtk_widget_set_name(btn_add_sequence, "modal-export-button");
	gtk_widget_set_size_request(btn_add_sequence, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_add_sequence, TRUE, TRUE, 0);

	// Controls row: Duration + Scale
	GtkWidget *controls_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(controls_row, "controls-row");
	gtk_widget_set_size_request(controls_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), controls_row, FALSE, FALSE, 5);

	// Duration spin button
	GtkWidget *duration_spin = gtk_spin_button_new_with_range(1, 60, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(duration_spin), 10);
	gtk_widget_set_name(controls_row, "spin-button");
	gtk_widget_set_size_request(duration_spin, 150, 42);
	gtk_box_pack_start(GTK_BOX(controls_row), duration_spin, TRUE, TRUE, 0);

	// Scale combo box
	GtkWidget *scale_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "1080p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "720p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "480p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "360p");
	gtk_combo_box_set_active(GTK_COMBO_BOX(scale_combo), 2);
	gtk_widget_set_size_request(scale_combo, 150, 42);
	gtk_box_pack_start(GTK_BOX(controls_row), scale_combo, TRUE, TRUE, 0);
	
	// Labels row: "Duration" and "Quality"
	GtkWidget *labels_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_size_request(labels_row, 400, 20);

	// FPS & Quality label
	GtkWidget *duration_label = gtk_label_new("Duration");
	gtk_widget_set_halign(duration_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(labels_row), duration_label, TRUE, TRUE, 0);
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
	ui->duration_spin = GTK_SPIN_BUTTON(duration_spin);
	ui->scale_combo = GTK_COMBO_BOX_TEXT(scale_combo);
	ui->log_view = log_view;
	ui->log_buffer = log_buffer;
	ui->progress_bar = progress_bar;
	ui->root_container = GTK_WIDGET(user_data);
	ui->parent_container = black_box;

	g_signal_connect(btn_add_sequence, "clicked", G_CALLBACK(on_add_sequence_clicked), ui);
	g_signal_connect(btn_back, "clicked", G_CALLBACK(on_modal_back_clicked), global_modal_layer);

	gtk_container_add(GTK_CONTAINER(global_modal_layer), black_box);
	gtk_widget_show_all(global_modal_layer);
}

void add_log(AddSequenceUI *ui, const char *message) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(ui->log_buffer, &end);
    gtk_text_buffer_insert(ui->log_buffer, &end, message, -1);
    gtk_text_buffer_insert(ui->log_buffer, &end, "\n", -1);
    GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(ui->log_view));
    gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
    while (gtk_events_pending()) gtk_main_iteration();
}

void create_fx_file(const char *path, AddSequenceUI *ui)
{
	set_progress_add_sequence(ui, 0.01, "Creating fx.txt...");
    add_log(ui, "Creating fx.txt...");

    FILE *f = fopen(path, "w");
    if (!f) {
        add_log(ui, "Failed to create fx.txt!");
        return;
    }

    for (int layer = 0; layer < MAX_LAYERS; layer++) {
        double speed = get_layer_speed(layer);
        int gray = is_layer_gray(layer);
        int alpha = get_transparency(layer);

        fprintf(f, "layer=%d\n", layer);
        fprintf(f, "speed=%f\n", speed);
        fprintf(f, "gray=%d\n", gray);
        fprintf(f, "alpha=%d\n\n", alpha);

        gchar log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "Layer %d: speed=%f gray=%d alpha=%d", layer, speed, gray, alpha);
        //add_log(ui, log_msg); /*KEEP FOR DEBUG*/
    }
    fclose(f);
    add_log(ui, "fx.txt created successfully.");
}


void on_add_sequence_clicked(GtkButton *button, gpointer user_data)
{
    AddSequenceUI *ui = (AddSequenceUI *)user_data;
	gtk_widget_set_sensitive(ui->root_container, FALSE);
	gtk_widget_set_sensitive(ui->parent_container, FALSE);
    gint duration = gtk_spin_button_get_value_as_int(ui->duration_spin);
    const gchar *scale = gtk_combo_box_text_get_active_text(ui->scale_combo);
    ensure_dir("sequences");
    int seq = get_next_sequence_index();
    gchar *seq_dir = g_strdup_printf("sequences/sequence_%d", seq);
    ensure_dir(seq_dir);
	
	set_progress_add_sequence(ui, 0.02, "Copy frame folders...");
    /* Copy frame folders */
    gchar *dst = NULL;

    dst = g_build_filename(seq_dir, "Frames_1", NULL);
    copy_directory("Frames_1", dst);
    g_free(dst);

    dst = g_build_filename(seq_dir, "Frames_2", NULL);
    copy_directory("Frames_2", dst);
    g_free(dst);

    dst = g_build_filename(seq_dir, "Frames_3", NULL);
    copy_directory("Frames_3", dst);
    g_free(dst);

    dst = g_build_filename(seq_dir, "Frames_4", NULL);
    copy_directory("Frames_4", dst);
    g_free(dst);

	// Create fx.txt
	gchar *fx_path = g_build_filename(seq_dir, "fx.txt", NULL);
	create_fx_file(fx_path, ui);
	g_free(fx_path);
    
    //we gonna figure out the output width and height here
    int output_width = 1280, output_height = 720;
    if (strcmp(scale, "1080p") == 0) { output_width = 1920; output_height = 1080; }
    else if (strcmp(scale, "720p") == 0) { output_width = 1280; output_height = 720; }
    else if (strcmp(scale, "480p") == 0) { output_width = 854; output_height = 480; }
    else if (strcmp(scale, "360p") == 0) { output_width = 640; output_height = 360; }
    
    add_log(ui, "[INFO] Add Sequence process start...");
    create_video_from_sequence(duration, output_width,output_height,seq, ui);
    add_log(ui, "[INFO] Add Sequence process completed.");
    set_progress_add_sequence(ui, 1, "Completed.");

    g_free(seq_dir);
    gtk_widget_set_sensitive(ui->root_container, TRUE);
    gtk_widget_set_sensitive(ui->parent_container, TRUE);
}

