#include <gtk/gtk.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "sdl.h"
#include "utils.h"
#include "layer.h"
#include "aphorism.h"
#include "sequencer.h"
#include "screen_panel.h"
#include "sdl_utilities.h"
#include "modal_add_sequence.h"

// Global
SelectedBar selected_bar = BAR_NONE;
GtkWidget *global_modal_layer = NULL;

// Callbacks
static void on_update_render_clicked(GtkButton *button, gpointer user_data)
{
	(void)button;
	(void)user_data;
	start_load_textures_async();
}

static void on_app_destroy(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    sdl_set_playing(0);
    cleanup_frames_folders();
    gtk_main_quit();
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    (void)widget;
    (void)user_data;

    const int step = 5;

    switch (event->keyval) {
    
	case GDK_KEY_Escape:
		gtk_widget_destroy(widget);
		return TRUE;
		
    case GDK_KEY_Left:
        if (selected_bar == BAR_START) {
            left_bar_x = MAX(0, left_bar_x - step);
            gtk_widget_set_margin_start(loop_start_box, left_bar_x);
        } else if (selected_bar == BAR_END) {
            right_bar_x = MAX(0, right_bar_x - step);
            gtk_widget_set_margin_start(loop_end_box, right_bar_x);
        }
        break;

    case GDK_KEY_Right:
        if (selected_bar == BAR_START) {
            left_bar_x = MIN(right_bar_x - LOOP_BAR_WIDTH, left_bar_x + step);
            gtk_widget_set_margin_start(loop_start_box, left_bar_x);
        } else if (selected_bar == BAR_END) {
            right_bar_x = MIN(sequences_overlay_width - LOOP_BAR_WIDTH, right_bar_x + step);
            gtk_widget_set_margin_start(loop_end_box, right_bar_x);
        }
        break;

    default:
        break;
    }

    gtk_widget_queue_draw(loop_start_box);
    gtk_widget_queue_draw(loop_end_box);

    return TRUE;
}

// Main
int main(int argc, char *argv[])
{
    srand((unsigned int)time(NULL));
    gtk_init(&argc, &argv);

    // Window
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "Pulsrr Sequencer");
    gtk_window_set_default_size(GTK_WINDOW(win),MINIMAL_WINDOW_WIDTH,MINIMAL_WINDOW_HEIGHT);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
    g_signal_connect(win, "destroy", G_CALLBACK(on_app_destroy), NULL);
    
    // Set window icon
	GError *icon_err = NULL;
	if (!gtk_window_set_icon_from_file(GTK_WINDOW(win), "icon.png", &icon_err)) {
		g_printerr("Failed to load icon: %s\n", icon_err->message);
		g_clear_error(&icon_err);
	}

    // Overlay
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(win), overlay);

    // Main container
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(main_hbox, "main-container");
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), main_hbox);

    // Modal layer
    GtkWidget *modal_layer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(modal_layer, "modal-layer");
    gtk_widget_set_halign(modal_layer, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(modal_layer, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(modal_layer, TRUE);
    gtk_widget_set_vexpand(modal_layer, TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), modal_layer);
    gtk_widget_hide(modal_layer);
    global_modal_layer = modal_layer;

    // Left container
    GtkWidget *left_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_name(left_container, "left-container");
    gtk_widget_set_size_request(left_container, LEFT_CONTAINER_WIDTH, -1);
    gtk_widget_set_hexpand(left_container, FALSE);
    gtk_widget_set_vexpand(left_container, TRUE);
    gtk_box_pack_start(GTK_BOX(main_hbox), left_container, FALSE, FALSE, 0);

    // Logo
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale("logo3.png", LOGO_WIDTH, -1, TRUE, NULL);
    if (pixbuf) {
        GtkWidget *logo = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
        gtk_widget_set_halign(logo, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(left_container), logo, FALSE, FALSE, 0);
    }

    // Aphorism
    gtk_box_pack_start(GTK_BOX(left_container),create_aphorism_widget(),FALSE, FALSE, 0);

    // Buttons
    GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_name(button_row, "button-row");
    gtk_widget_set_size_request(button_row,LEFT_CONTAINER_WIDTH,BUTTON_TOP_CONTROL_HEIGHT);
    gtk_box_pack_start(GTK_BOX(left_container), button_row, FALSE, FALSE, 0);

    GtkWidget *btn_create = gtk_button_new_with_label("ADD SEQ.");
    GtkWidget *btn_update = gtk_button_new_with_label("UPDATE");

    gtk_widget_set_name(btn_create, "btn-add-seq");
    gtk_widget_set_name(btn_update, "btn-update-render");

    gtk_widget_set_size_request(btn_create,BUTTON_TOP_CONTROL_WIDTH,BUTTON_TOP_CONTROL_HEIGHT);
    gtk_widget_set_size_request(btn_update,BUTTON_TOP_CONTROL_WIDTH,BUTTON_TOP_CONTROL_HEIGHT);
    
    gtk_box_pack_start(GTK_BOX(button_row), btn_create, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), btn_update, TRUE, TRUE, 0);

	// Log area (for future use)
	GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_name(log_scroll, "main-log-scroll");
	gtk_widget_set_size_request(log_scroll, LEFT_CONTAINER_WIDTH, 60);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scroll),
		                           GTK_POLICY_AUTOMATIC,
		                           GTK_POLICY_AUTOMATIC);

	// Text view
	GtkWidget *log_view = gtk_text_view_new();
	gtk_widget_set_name(log_view, "main-log-view");
	gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(log_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(log_view), GTK_WRAP_WORD_CHAR);
	GtkTextBuffer *log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
	gtk_text_buffer_set_text(log_buffer, "[LOG] App Start.\n", -1);
	gtk_container_add(GTK_CONTAINER(log_scroll), log_view);

    // Layers
	gtk_box_pack_start(GTK_BOX(left_container), create_layer_component(0), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_container), create_layer_component(1), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_container), create_layer_component(2), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_container), create_layer_component(3), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(left_container), log_scroll, FALSE, FALSE, 0);

    // Version
    GtkWidget *version_label = gtk_label_new("Version 0.0.1 by Rekav");
    gtk_widget_set_name(version_label, "version-label");
    gtk_widget_set_halign(version_label, GTK_ALIGN_CENTER);
    gtk_box_pack_end(GTK_BOX(left_container), version_label, FALSE, FALSE, 0);

    // Right container
    GtkWidget *right_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(right_container, "right-container");
    gtk_widget_set_hexpand(right_container, TRUE);
    gtk_widget_set_vexpand(right_container, TRUE);
    gtk_box_pack_start(GTK_BOX(main_hbox), right_container, TRUE, TRUE, 0);

    GtkWidget *render_panel = NULL;
    GtkWidget *render_ui = create_render_screen(&render_panel);
    gtk_box_pack_start(GTK_BOX(right_container), render_ui, TRUE, TRUE, 0);

    g_timeout_add(33, sdl_refresh_loop, render_panel);

    gtk_box_pack_start(GTK_BOX(right_container),create_sequencer_component(),FALSE, FALSE, 0);
    
    // Populate for later access
	g_main_ui.log_view = GTK_TEXT_VIEW(log_view);
	g_main_ui.log_buffer = log_buffer;
	g_main_ui.main_container = main_hbox;

    // Signals
    g_signal_connect(btn_update, "clicked",G_CALLBACK(on_update_render_clicked), render_panel);
    g_signal_connect(btn_create, "clicked",G_CALLBACK(on_add_button_clicked),main_hbox);
    

    gtk_widget_add_events(win, GDK_KEY_PRESS_MASK);
    g_signal_connect(win, "key-press-event",G_CALLBACK(on_key_press), NULL);

    // CSS
    GtkCssProvider *css = gtk_css_provider_new();
    GError *gerr = NULL;

    if (!gtk_css_provider_load_from_path(css, "style.css", &gerr)) {
        g_printerr("Failed to load style.css: %s\n", gerr->message);
        g_clear_error(&gerr);
    } else {
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
    }

    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}

