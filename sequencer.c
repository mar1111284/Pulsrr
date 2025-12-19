#include "utils.h"
#include "sequencer.h"

/* sequencer.c */
int left_bar_x  = -1;
int right_bar_x = -1;
int sequences_overlay_width = 0;

GtkWidget *loop_end_box = NULL;
GtkWidget *loop_start_box = NULL;

void on_sequencer_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data) {
    GtkWidget *loop_end_bar = GTK_WIDGET(user_data);

    int width = allocation->width;
    gtk_widget_set_margin_start(loop_end_bar, width - 4); // 4 = bar width
}

void on_overlay_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data) {
    sequences_overlay_width = allocation->width;

    // Initialize bars if needed
    if (left_bar_x == -1)  left_bar_x  = 0;
    if (right_bar_x == -1) right_bar_x = sequences_overlay_width - LOOP_BAR_WIDTH;

    // Clamp to prevent overlap / overflow
    if (left_bar_x > right_bar_x - LOOP_BAR_WIDTH)
        left_bar_x = right_bar_x - LOOP_BAR_WIDTH;

    if (right_bar_x > sequences_overlay_width - LOOP_BAR_WIDTH)
        right_bar_x = sequences_overlay_width - LOOP_BAR_WIDTH;

    gtk_widget_set_margin_start(loop_start_box, left_bar_x);
    gtk_widget_set_margin_start(loop_end_box, right_bar_x);

    g_print("Overlay width: %d px, left_bar_x: %d, right_bar_x: %d\n",
            sequences_overlay_width, left_bar_x, right_bar_x);
}


gboolean on_bar_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    SelectedBar bar = (SelectedBar)(intptr_t)user_data;
    selected_bar = bar;

    // Remove class from both bars first
    GtkStyleContext *ctx_start = gtk_widget_get_style_context(loop_start_box);
    GtkStyleContext *ctx_end   = gtk_widget_get_style_context(loop_end_box);
    gtk_style_context_remove_class(ctx_start, "bar-selected");
    gtk_style_context_remove_class(ctx_end, "bar-selected");

    // Add class to selected bar
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), "bar-selected");

    g_print("Selected bar: %s\n", bar == BAR_START ? "start" : "end");
    return TRUE;
}


GtkWidget* create_sequence_widget_css(const char *sequence_folder) {
    // Build path to first frame
    gchar *frame_path = g_build_filename(sequence_folder, "frame_00001.png", NULL);
    g_print("CSS widget frame: %s\n", frame_path);

    if (!g_file_test(frame_path, G_FILE_TEST_IS_REGULAR)) {
        g_free(frame_path);
        return gtk_label_new("Sequence"); // fallback
    }

    // Create EventBox
    GtkWidget *event_box = gtk_event_box_new();
    gtk_widget_set_hexpand(event_box, TRUE);
    gtk_widget_set_vexpand(event_box, TRUE);

    // Give it a CSS name
    gtk_widget_set_name(event_box, "sequence-preview-css");

    // Apply CSS to set background image
    GtkCssProvider *provider = gtk_css_provider_new();
    gchar *css = g_strdup_printf(
        "#sequence-preview-css {"
        "background-image: url('%s');"
        "background-repeat: repeat;"
        "background-size: contain;"
        "}", frame_path);

    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    g_free(css);

    GtkStyleContext *context = gtk_widget_get_style_context(event_box);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);

    g_free(frame_path);
    return event_box;
}

// Returns the width of each sequence widget in pixels
int get_sequence_widget_width(GtkWidget *sequences_box, int num_sequences) {
    int box_width = gtk_widget_get_allocated_width(sequences_box);
    if (num_sequences > 0)
        return box_width / num_sequences;
    else
        return 50; // fallback
}

// Returns the height of each sequence widget in pixels
int get_sequence_widget_height(GtkWidget *sequences_box) {
    return gtk_widget_get_allocated_height(sequences_box);
}

GtkWidget * create_sequencer_component(void){
	// =====================================================
	// Sequencer panel container
	// =====================================================
	GtkWidget *sequencer_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_name(sequencer_container, "sequencer-container");
	gtk_widget_set_size_request(sequencer_container, -1, 150);
	gtk_widget_set_vexpand(sequencer_container, FALSE);
	gtk_widget_set_hexpand(sequencer_container, TRUE);

	// =====================================================
	// Left column: controls (fixed width)
	// =====================================================
	GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_widget_set_name(controls_box, "timeline-controls");
	gtk_widget_set_size_request(controls_box, 100, -1);
	gtk_widget_set_vexpand(controls_box, TRUE);

	// -----------------------------------------------------
	// Row 1: Playback controls
	// -----------------------------------------------------
	GtkWidget *row_playback = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_halign(row_playback, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(row_playback, GTK_ALIGN_CENTER);

	GtkWidget *sequence_play = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
	GtkWidget *sequence_pause = gtk_button_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_BUTTON);
	GtkWidget *sequence_restart = gtk_button_new_from_icon_name("media-seek-backward", GTK_ICON_SIZE_BUTTON);
	GtkWidget *sequence_loop = gtk_button_new_from_icon_name("media-playlist-repeat", GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_name(sequence_play, "sequence-play");
        gtk_widget_set_name(sequence_pause, "sequence-pause");
        gtk_widget_set_name(sequence_restart, "sequence-restart");
        gtk_widget_set_name(sequence_loop, "sequence-loop");
        
	gtk_box_pack_start(GTK_BOX(row_playback), sequence_play, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(row_playback), sequence_pause, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(row_playback), sequence_restart, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(row_playback), sequence_loop, FALSE, FALSE, 2);
	
	// -----------------------------------------------------
	// Row 2: Export / destination
	// -----------------------------------------------------
	GtkWidget *row_export = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_name(row_export, "export-row");
	gtk_widget_set_size_request(row_export, 150, 36);
	
	GtkWidget *btn_export = gtk_button_new_with_label("Dwld. MP4");
	GtkWidget *btn_open_export = gtk_button_new_with_label("Opn. Folder");
	gtk_widget_set_name(btn_export, "btn-export");
	gtk_widget_set_name(btn_open_export, "btn-open-export");
        gtk_widget_set_size_request(btn_export, 100, 36);
	gtk_widget_set_size_request(btn_open_export, 100, 36);
	gtk_box_pack_start(GTK_BOX(row_export), btn_export, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(row_export), btn_open_export, TRUE, TRUE, 2);
	
	// -----------------------------------------------------
	// Row 3: Sequence info (hard-coded, 1 spec per row)
	// -----------------------------------------------------
	GtkWidget *row_info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_set_name(row_info, "sequence-info");

	/* ---- Seq. duration ---- */
	GtkWidget *row_duration = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *lbl_duration_title = gtk_label_new("Seq. duration:");
	GtkWidget *lbl_duration_value = gtk_label_new("8.0 s");

	gtk_label_set_xalign(GTK_LABEL(lbl_duration_title), 0.0);
	gtk_label_set_xalign(GTK_LABEL(lbl_duration_value), 1.0);

	gtk_style_context_add_class(
	    gtk_widget_get_style_context(lbl_duration_title),
	    "sequence-info-title"
	);
	gtk_style_context_add_class(
	    gtk_widget_get_style_context(lbl_duration_value),
	    "sequence-info-value"
	);

	gtk_box_pack_start(GTK_BOX(row_duration), lbl_duration_title, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(row_duration), lbl_duration_value, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row_info), row_duration, FALSE, FALSE, 1);

	/* ---- Est. size ---- */
	GtkWidget *row_size = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *lbl_size_title = gtk_label_new("Est. size:");
	GtkWidget *lbl_size_value = gtk_label_new("~42 MB");

	gtk_label_set_xalign(GTK_LABEL(lbl_size_title), 0.0);
	gtk_label_set_xalign(GTK_LABEL(lbl_size_value), 1.0);

	gtk_style_context_add_class(
	    gtk_widget_get_style_context(lbl_size_title),
	    "sequence-info-title"
	);
	gtk_style_context_add_class(
	    gtk_widget_get_style_context(lbl_size_value),
	    "sequence-info-value"
	);

	gtk_box_pack_start(GTK_BOX(row_size), lbl_size_title, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(row_size), lbl_size_value, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row_info), row_size, FALSE, FALSE, 1);

	/* ---- Nb. frames ---- */
	GtkWidget *row_frames = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *lbl_frames_title = gtk_label_new("Nb. frames:");
	GtkWidget *lbl_frames_value = gtk_label_new("240");

	gtk_label_set_xalign(GTK_LABEL(lbl_frames_title), 0.0);
	gtk_label_set_xalign(GTK_LABEL(lbl_frames_value), 1.0);

	gtk_style_context_add_class(
	    gtk_widget_get_style_context(lbl_frames_title),
	    "sequence-info-title"
	);
	gtk_style_context_add_class(
	    gtk_widget_get_style_context(lbl_frames_value),
	    "sequence-info-value"
	);

	gtk_box_pack_start(GTK_BOX(row_frames), lbl_frames_title, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(row_frames), lbl_frames_value, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row_info), row_frames, FALSE, FALSE, 1);

	/* ---- Resolution ---- */
	GtkWidget *row_resolution = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *lbl_resolution_title = gtk_label_new("Resolution:");
	GtkWidget *lbl_resolution_value = gtk_label_new("1920×1080");

	gtk_label_set_xalign(GTK_LABEL(lbl_resolution_title), 0.0);
	gtk_label_set_xalign(GTK_LABEL(lbl_resolution_value), 1.0);

	gtk_style_context_add_class(
	    gtk_widget_get_style_context(lbl_resolution_title),
	    "sequence-info-title"
	);
	gtk_style_context_add_class(
	    gtk_widget_get_style_context(lbl_resolution_value),
	    "sequence-info-value"
	);

	gtk_box_pack_start(GTK_BOX(row_resolution), lbl_resolution_title, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(row_resolution), lbl_resolution_value, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(row_info), row_resolution, FALSE, FALSE, 1);


	// -----------------------------------------------------
	// Pack rows into controls box
	// -----------------------------------------------------
	gtk_box_pack_start(GTK_BOX(controls_box), row_playback, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(controls_box), row_export, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(controls_box), row_info, FALSE, FALSE, 10);
	


	// =====================================================
	// Sequencer view container RIGHT COLUMN
	// =====================================================
	GtkWidget *sequencer_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_widget_set_name(sequencer_view, "sequencer-view");
	gtk_widget_set_vexpand(sequencer_view, TRUE);
	gtk_widget_set_hexpand(sequencer_view, TRUE);
	
	// -----------------------------------------------------
	// Music control row (38px)
	// -----------------------------------------------------
	GtkWidget *music_ctrl_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_name(music_ctrl_row, "music-control-row");
	gtk_widget_set_size_request(music_ctrl_row, -1, 38);
	gtk_widget_set_hexpand(music_ctrl_row, TRUE);
	gtk_widget_set_valign(music_ctrl_row, GTK_ALIGN_CENTER);

	// Label
	GtkWidget *music_label = gtk_label_new("Music ctrl.");
	gtk_widget_set_name(music_label, "music-control-label");
	gtk_widget_set_valign(music_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(music_ctrl_row), music_label, FALSE, FALSE, 6);

	// Buttons
	GtkWidget *btn_music_load = gtk_button_new_from_icon_name("document-open-symbolic", GTK_ICON_SIZE_BUTTON);
	//GtkWidget *btn_music_play = gtk_button_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
	GtkWidget *btn_music_mute = gtk_button_new_from_icon_name("audio-volume-muted-symbolic", GTK_ICON_SIZE_BUTTON);

	gtk_widget_set_name(btn_music_load, "music-btn");
	//gtk_widget_set_name(btn_music_play, "music-btn");
	gtk_widget_set_name(btn_music_mute, "music-btn");

	gtk_box_pack_start(GTK_BOX(music_ctrl_row), btn_music_load, FALSE, FALSE, 0);
	//gtk_box_pack_start(GTK_BOX(music_ctrl_row), btn_music_play, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(music_ctrl_row), btn_music_mute, FALSE, FALSE, 0);

	// Spacer
	GtkWidget *ctrl_spacer = gtk_label_new(NULL);
	gtk_widget_set_hexpand(ctrl_spacer, TRUE);
	gtk_box_pack_start(GTK_BOX(music_ctrl_row), ctrl_spacer, TRUE, TRUE, 0);

	// Volume
	GtkWidget *vol_label = gtk_label_new("Vol:");
	gtk_widget_set_name(vol_label, "music-volume-label");
	gtk_widget_set_valign(vol_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(music_ctrl_row), vol_label, FALSE, FALSE, 4);

	GtkWidget *vol_spin = gtk_spin_button_new_with_range(0, 100, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(vol_spin), 100);
	gtk_widget_set_size_request(vol_spin, 60, -1);
	gtk_widget_set_name(vol_spin, "music-volume-spin");
	gtk_box_pack_start(GTK_BOX(music_ctrl_row), vol_spin, FALSE, FALSE, 0);

	// Pack between header and sequences overlay
	gtk_box_pack_start(GTK_BOX(sequencer_view), music_ctrl_row, FALSE, FALSE, 0);	

	// -----------------------------------------------------
	// Sequence preview container (horizontal row of frames + loop bars)
	// -----------------------------------------------------
	
	GtkWidget *sequences_overlay = gtk_overlay_new();
	gtk_widget_set_hexpand(sequences_overlay, TRUE);
	gtk_widget_set_vexpand(sequences_overlay, TRUE);
	gtk_box_pack_start(GTK_BOX(sequencer_view), sequences_overlay, TRUE, TRUE, 0);


	// Sequences horizontal box
	GtkWidget *sequences_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_widget_set_name(sequences_box, "frames-box");
	gtk_widget_set_hexpand(sequences_box, TRUE);
	gtk_widget_set_vexpand(sequences_box, TRUE);
	gtk_container_add(GTK_CONTAINER(sequences_overlay), sequences_box);

	// -----------------------------------------------------
	// Display sequence preview
	// -----------------------------------------------------
	int num_sequences = get_number_of_sequences();

	if (num_sequences == 0) {
	    GtkWidget *empty_label = gtk_label_new("(No sequence added)");
	    gtk_widget_set_name(empty_label, "no-sequence-label");

	    gtk_widget_set_hexpand(empty_label, TRUE);
	    gtk_widget_set_vexpand(empty_label, TRUE);
	    gtk_widget_set_halign(empty_label, GTK_ALIGN_CENTER);
	    gtk_widget_set_valign(empty_label, GTK_ALIGN_CENTER);

	    gtk_box_pack_start(GTK_BOX(sequences_box), empty_label, TRUE, TRUE, 0);
	} else {
	    for (int i = 0; i < num_sequences; i++) {
		gchar *sequence_path = g_strdup_printf("sequences/sequence_%d", i + 1);
		GtkWidget *sequence_widget = create_sequence_widget_css(sequence_path);
		gtk_box_pack_start(GTK_BOX(sequences_box), sequence_widget, TRUE, TRUE, 2);
		g_free(sequence_path);
	    }
	}

	gtk_widget_show_all(sequences_box);


	// Loop start bar
	loop_start_box = gtk_event_box_new();
	gtk_widget_set_name(loop_start_box, "loop-start-bar");
	gtk_widget_set_size_request(loop_start_box, LOOP_BAR_WIDTH, -1);
	gtk_widget_set_halign(loop_start_box, GTK_ALIGN_START);
	gtk_widget_set_valign(loop_start_box, GTK_ALIGN_FILL);
	gtk_widget_set_vexpand(loop_start_box, TRUE);
	gtk_widget_set_hexpand(loop_start_box, FALSE); // width fixed
	gtk_overlay_add_overlay(GTK_OVERLAY(sequences_overlay), loop_start_box);

	// Set initial position via margin
	gtk_widget_set_margin_start(loop_start_box, 0); // start at left edge


	loop_end_box = gtk_event_box_new();
	gtk_widget_set_name(loop_end_box, "loop-end-bar");
	gtk_widget_set_size_request(loop_end_box, LOOP_BAR_WIDTH, -1);
	gtk_widget_set_halign(loop_end_box, GTK_ALIGN_START); // align left so margin works
	gtk_widget_set_valign(loop_end_box, GTK_ALIGN_FILL);   // fill vertically
	gtk_widget_set_vexpand(loop_end_box, TRUE);
	gtk_widget_set_hexpand(loop_end_box, FALSE);
	gtk_overlay_add_overlay(GTK_OVERLAY(sequences_overlay), loop_end_box);
	
	g_signal_connect(sequences_overlay, "size-allocate",
                 G_CALLBACK(on_overlay_size_allocate), NULL);
                 
        gtk_widget_add_events(loop_start_box, GDK_BUTTON_PRESS_MASK);
	gtk_widget_add_events(loop_end_box,   GDK_BUTTON_PRESS_MASK);

	g_signal_connect(loop_start_box, "button-press-event",
		         G_CALLBACK(on_bar_clicked), (gpointer)BAR_START);

	g_signal_connect(loop_end_box, "button-press-event",
		         G_CALLBACK(on_bar_clicked), (gpointer)BAR_END);




	// -----------------------------------------------------
	// Timeline labels (bottom)
	// -----------------------------------------------------
	GtkWidget *timeline_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(timeline_box, "timeline-box");
	gtk_box_pack_start(GTK_BOX(sequencer_view), timeline_box, FALSE, FALSE, 0);
	
	// Fixed height, flexible width
	gtk_widget_set_size_request(timeline_box, -1, 36);
	gtk_widget_set_hexpand(timeline_box, TRUE);

	// ---- Left label (timeline start) ----
	GtkWidget *timeline_start_label = gtk_label_new("00:00");
	gtk_widget_set_name(timeline_start_label, "timeline-start-label");
	gtk_box_pack_start(GTK_BOX(timeline_box), timeline_start_label, FALSE, FALSE, 8);

	// ---- Left spacer ----
	GtkWidget *spacer_left = gtk_label_new(NULL);
	gtk_widget_set_hexpand(spacer_left, TRUE);
	gtk_box_pack_start(GTK_BOX(timeline_box), spacer_left, TRUE, TRUE, 0);

	// -----------------------------------------------------
	// Middle info group (Start / Stop / Loop)
	// -----------------------------------------------------
	GtkWidget *timeline_info = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
	gtk_widget_set_name(timeline_info, "timeline-info");

	// Start
	gtk_box_pack_start(GTK_BOX(timeline_info),
	    gtk_label_new("Start:"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(timeline_info),
	    gtk_label_new("--:--"), FALSE, FALSE, 0);

	// Stop
	gtk_box_pack_start(GTK_BOX(timeline_info),
	    gtk_label_new("Stop:"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(timeline_info),
	    gtk_label_new("--:--"), FALSE, FALSE, 0);

	// Loop duration
	gtk_box_pack_start(GTK_BOX(timeline_info),
	    gtk_label_new("Dr. Loop:"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(timeline_info),
	    gtk_label_new("--:--"), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(timeline_box), timeline_info, FALSE, FALSE, 0);

	// ---- Right spacer ----
	GtkWidget *spacer_right = gtk_label_new(NULL);
	gtk_widget_set_hexpand(spacer_right, TRUE);
	gtk_box_pack_start(GTK_BOX(timeline_box), spacer_right, TRUE, TRUE, 0);

	// ---- Right label (timeline end) ----
	GtkWidget *timeline_end_label = gtk_label_new("--:--");
	gtk_widget_set_name(timeline_end_label, "timeline-end-label");
	gtk_box_pack_start(GTK_BOX(timeline_box), timeline_end_label, FALSE, FALSE, 8);
	
	gtk_box_pack_start(GTK_BOX(sequencer_container), controls_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(sequencer_container), sequencer_view, TRUE, TRUE, 0);
	
	return sequencer_container;
}
