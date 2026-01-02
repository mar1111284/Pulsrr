#include "../utils/utils.h"
#include "component_sequencer.h"
#include "../modals/modal_download.h"

// Globals - TO REFACT
int left_bar_x  = -1;
int right_bar_x = -1;
int sequences_overlay_width = 0;

GtkWidget *loop_end_box = NULL;
GtkWidget *loop_start_box = NULL;
GtkWidget *sequences_box = NULL;
SelectedBar selected_bar = BAR_NONE;

static gboolean update_sequencer_ui(gpointer user_data);

static void on_sequence_play_clicked(GtkButton *button, gpointer user_data)
{
    // Only play if frames exist or not loading
    if (sdl_get_render_state() != RENDER_STATE_IDLE &&
        sdl_get_render_state() != RENDER_STATE_LOADING) {
        sdl_set_render_state(RENDER_STATE_PLAY);
    }
}

static void on_sequence_pause_clicked(GtkButton *button, gpointer user_data)
{
	// Only pause if frames exist or not loading
    if (sdl_get_render_state() != RENDER_STATE_IDLE &&
        sdl_get_render_state() != RENDER_STATE_LOADING) {
        sdl_set_render_state(RENDER_STATE_PAUSE);
    }
}


static void on_sequence_restart_clicked(GtkButton *button, gpointer user_data)
{
    g_print("[UI] sequence-restart clicked\n");
}

static void on_sequence_loop_clicked(GtkButton *button, gpointer user_data)
{
    g_print("[UI] sequence-loop clicked\n");
}

void update_sequencer(void) {
    g_idle_add(update_sequencer_ui, NULL);
}

static void clear_container(GtkWidget *container) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(container));
    for (GList *l = children; l != NULL; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);
}

static gboolean update_sequencer_ui(gpointer user_data) {
    if (!sequences_box)
        return G_SOURCE_REMOVE;

    clear_container(sequences_box);

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

    return G_SOURCE_REMOVE; // run once
}


// Overlay size allocate callback
void on_overlay_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data) {
    sequences_overlay_width = allocation->width;

    if (left_bar_x == -1)  left_bar_x  = 0;
    if (right_bar_x == -1) right_bar_x = sequences_overlay_width - LOOP_BAR_WIDTH;

    if (left_bar_x > right_bar_x - LOOP_BAR_WIDTH)
        left_bar_x = right_bar_x - LOOP_BAR_WIDTH;

    if (right_bar_x > sequences_overlay_width - LOOP_BAR_WIDTH)
        right_bar_x = sequences_overlay_width - LOOP_BAR_WIDTH;

    gtk_widget_set_margin_start(loop_start_box, left_bar_x);
    gtk_widget_set_margin_start(loop_end_box, right_bar_x);

}

// Bar click callback
gboolean on_bar_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    SelectedBar bar = (SelectedBar)(intptr_t)user_data;
    selected_bar = bar;

    GtkStyleContext *ctx_start = gtk_widget_get_style_context(loop_start_box);
    GtkStyleContext *ctx_end   = gtk_widget_get_style_context(loop_end_box);
    gtk_style_context_remove_class(ctx_start, "bar-selected");
    gtk_style_context_remove_class(ctx_end, "bar-selected");

    gtk_style_context_add_class(gtk_widget_get_style_context(widget), "bar-selected");

    return TRUE;
}

// Create a CSS-styled sequence widget
GtkWidget* create_sequence_widget_css(const char *sequence_folder) {
    gchar *frame_path = g_build_filename(sequence_folder, "mixed_frames/frame_00001.png", NULL);

    if (!g_file_test(frame_path, G_FILE_TEST_IS_REGULAR)) {
        g_free(frame_path);
        return gtk_label_new("Sequence");
    }

    GtkWidget *event_box = gtk_event_box_new();
    gtk_widget_set_hexpand(event_box, TRUE);
    gtk_widget_set_vexpand(event_box, TRUE);
    gtk_widget_set_name(event_box, "sequence-preview-css");

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

// Get sequence widget width
int get_sequence_widget_width(GtkWidget *sequences_box, int num_sequences) {
    int box_width = gtk_widget_get_allocated_width(sequences_box);
    if (num_sequences > 0)
        return box_width / num_sequences;
    return 50;
}

// Get sequence widget height
int get_sequence_widget_height(GtkWidget *sequences_box) {
    return gtk_widget_get_allocated_height(sequences_box);
}

// Create full sequencer component
GtkWidget *create_sequencer_component(void) {
    // Sequencer container
    GtkWidget *sequencer_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_name(sequencer_container, "sequencer-container");
    gtk_widget_set_size_request(sequencer_container, -1, 150);
    gtk_widget_set_vexpand(sequencer_container, FALSE);
    gtk_widget_set_hexpand(sequencer_container, TRUE);

    // Controls column
    GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_name(controls_box, "timeline-controls");
    gtk_widget_set_size_request(controls_box, 100, -1);
    gtk_widget_set_vexpand(controls_box, TRUE);

    // Row: Playback controls
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

    // Row: Export / destination
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

    // Row: Sequence info
    GtkWidget *row_info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_name(row_info, "sequence-info");

    // Sequence info items
    struct { const char *title; const char *value; } info_items[] = {
        {"Seq. duration:", "8.0 s"},
        {"Est. size:", "~42 MB"},
        {"Nb. frames:", "240"},
        {"Resolution:", "1920×1080"}
    };

    for (int i = 0; i < 4; i++) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *lbl_title = gtk_label_new(info_items[i].title);
        GtkWidget *lbl_value = gtk_label_new(info_items[i].value);
        gtk_label_set_xalign(GTK_LABEL(lbl_title), 0.0);
        gtk_label_set_xalign(GTK_LABEL(lbl_value), 1.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_title), "sequence-info-title");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_value), "sequence-info-value");
        gtk_box_pack_start(GTK_BOX(row), lbl_title, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(row), lbl_value, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row_info), row, FALSE, FALSE, 1);
    }

    // Pack control rows
    gtk_box_pack_start(GTK_BOX(controls_box), row_playback, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(controls_box), row_export, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(controls_box), row_info, FALSE, FALSE, 10);

    // Sequencer view
    GtkWidget *sequencer_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_name(sequencer_view, "sequencer-view");
    gtk_widget_set_vexpand(sequencer_view, TRUE);
    gtk_widget_set_hexpand(sequencer_view, TRUE);

    // Music control row
    GtkWidget *music_ctrl_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_name(music_ctrl_row, "music-control-row");
    gtk_widget_set_size_request(music_ctrl_row, -1, 38);
    gtk_widget_set_hexpand(music_ctrl_row, TRUE);
    gtk_widget_set_valign(music_ctrl_row, GTK_ALIGN_CENTER);

    GtkWidget *music_label = gtk_label_new("Music ctrl.");
    gtk_widget_set_name(music_label, "music-control-label");
    gtk_widget_set_valign(music_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(music_ctrl_row), music_label, FALSE, FALSE, 6);

    GtkWidget *btn_music_load = gtk_button_new_from_icon_name("document-open-symbolic", GTK_ICON_SIZE_BUTTON);
    GtkWidget *btn_music_mute = gtk_button_new_from_icon_name("audio-volume-muted-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_name(btn_music_load, "music-btn");
    gtk_widget_set_name(btn_music_mute, "music-btn");
    gtk_box_pack_start(GTK_BOX(music_ctrl_row), btn_music_load, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(music_ctrl_row), btn_music_mute, FALSE, FALSE, 0);

    GtkWidget *ctrl_spacer = gtk_label_new(NULL);
    gtk_widget_set_hexpand(ctrl_spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(music_ctrl_row), ctrl_spacer, TRUE, TRUE, 0);

    GtkWidget *vol_label = gtk_label_new("Vol:");
    gtk_widget_set_name(vol_label, "music-volume-label");
    gtk_widget_set_valign(vol_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(music_ctrl_row), vol_label, FALSE, FALSE, 4);

    GtkWidget *vol_spin = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(vol_spin), 100);
    gtk_widget_set_size_request(vol_spin, 60, -1);
    gtk_widget_set_name(vol_spin, "music-volume-spin");
    gtk_box_pack_start(GTK_BOX(music_ctrl_row), vol_spin, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(sequencer_view), music_ctrl_row, FALSE, FALSE, 0);

    // Sequence preview overlay
    GtkWidget *sequences_overlay = gtk_overlay_new();
    gtk_widget_set_hexpand(sequences_overlay, TRUE);
    gtk_widget_set_vexpand(sequences_overlay, TRUE);
    gtk_box_pack_start(GTK_BOX(sequencer_view), sequences_overlay, TRUE, TRUE, 0);

    sequences_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_name(sequences_box, "frames-box");
    gtk_widget_set_hexpand(sequences_box, TRUE);
    gtk_widget_set_vexpand(sequences_box, TRUE);
    gtk_container_add(GTK_CONTAINER(sequences_overlay), sequences_box);

    update_sequencer();

    // Loop bars
    loop_start_box = gtk_event_box_new();
    gtk_widget_set_name(loop_start_box, "loop-start-bar");
    gtk_widget_set_size_request(loop_start_box, LOOP_BAR_WIDTH, -1);
    gtk_widget_set_halign(loop_start_box, GTK_ALIGN_START);
    gtk_widget_set_valign(loop_start_box, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand(loop_start_box, TRUE);
    gtk_widget_set_hexpand(loop_start_box, FALSE);
    gtk_overlay_add_overlay(GTK_OVERLAY(sequences_overlay), loop_start_box);
    gtk_widget_set_margin_start(loop_start_box, 0);

    loop_end_box = gtk_event_box_new();
    gtk_widget_set_name(loop_end_box, "loop-end-bar");
    gtk_widget_set_size_request(loop_end_box, LOOP_BAR_WIDTH, -1);
    gtk_widget_set_halign(loop_end_box, GTK_ALIGN_START);
    gtk_widget_set_valign(loop_end_box, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand(loop_end_box, TRUE);
    gtk_widget_set_hexpand(loop_end_box, FALSE);
    gtk_overlay_add_overlay(GTK_OVERLAY(sequences_overlay), loop_end_box);

    g_signal_connect(sequences_overlay, "size-allocate",G_CALLBACK(on_overlay_size_allocate), NULL);

    gtk_widget_add_events(loop_start_box, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(loop_end_box, GDK_BUTTON_PRESS_MASK);

    g_signal_connect(loop_start_box, "button-press-event",G_CALLBACK(on_bar_clicked), (gpointer)BAR_START);
    g_signal_connect(loop_end_box, "button-press-event",G_CALLBACK(on_bar_clicked), (gpointer)BAR_END);

    // Timeline labels
    GtkWidget *timeline_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(timeline_box, "timeline-box");
    gtk_box_pack_start(GTK_BOX(sequencer_view), timeline_box, FALSE, FALSE, 0);
    gtk_widget_set_size_request(timeline_box, -1, 36);
    gtk_widget_set_hexpand(timeline_box, TRUE);

    GtkWidget *timeline_start_label = gtk_label_new("00:00");
    gtk_widget_set_name(timeline_start_label, "timeline-start-label");
    gtk_box_pack_start(GTK_BOX(timeline_box), timeline_start_label, FALSE, FALSE, 8);

    GtkWidget *spacer_left = gtk_label_new(NULL);
    gtk_widget_set_hexpand(spacer_left, TRUE);
    gtk_box_pack_start(GTK_BOX(timeline_box), spacer_left, TRUE, TRUE, 0);

    GtkWidget *timeline_info = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_name(timeline_info, "timeline-info");

    gtk_box_pack_start(GTK_BOX(timeline_info), gtk_label_new("Start:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timeline_info), gtk_label_new("--:--"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timeline_info), gtk_label_new("Stop:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timeline_info), gtk_label_new("--:--"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timeline_info), gtk_label_new("Dr. Loop:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timeline_info), gtk_label_new("--:--"), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(timeline_box), timeline_info, FALSE, FALSE, 0);

    GtkWidget *spacer_right = gtk_label_new(NULL);
    gtk_widget_set_hexpand(spacer_right, TRUE);
    gtk_box_pack_start(GTK_BOX(timeline_box), spacer_right, TRUE, TRUE, 0);

    GtkWidget *timeline_end_label = gtk_label_new("--:--");
    gtk_widget_set_name(timeline_end_label, "timeline-end-label");
    gtk_box_pack_start(GTK_BOX(timeline_box), timeline_end_label, FALSE, FALSE, 8);

    gtk_box_pack_start(GTK_BOX(sequencer_container), controls_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sequencer_container), sequencer_view, TRUE, TRUE, 0);
    
    // Button signals
    g_signal_connect(sequence_play,"clicked",G_CALLBACK(on_sequence_play_clicked), NULL);
	g_signal_connect(sequence_pause,"clicked",G_CALLBACK(on_sequence_pause_clicked), NULL);
	g_signal_connect(sequence_restart,"clicked",G_CALLBACK(on_sequence_restart_clicked), NULL);
	g_signal_connect(sequence_loop,"clicked",G_CALLBACK(on_sequence_loop_clicked), NULL);
	g_signal_connect(btn_export, "clicked", G_CALLBACK(on_download_button_clicked), sequencer_container);


    return sequencer_container;
}

