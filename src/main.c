#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <locale.h>
#include <X11/Xlib.h>

/* SDL engine */
#include "sdl/sdl.h"

/* Components */
#include "components/component_sequencer.h"
#include "components/component_layer.h"
#include "components/component_screen.h"
#include "components/component_aphorism.h"

/* Utilities */
#include "utils/utils.h"

/* Modals */
#include "modals/modal_add_sequence.h"

static void on_update_render_clicked(GtkButton *button, gpointer user_data);
static void on_app_destroy(GtkWidget *widget, gpointer data);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

// Main entry point
int main(int argc, char *argv[]) {

    gtk_init(&argc, &argv);
 	init_app_paths(argv[0]);
    AppContext ctx = {0};
    const AppPaths *paths = get_app_paths();

    // Create main window
    ctx.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!ctx.window) {
        fprintf(stderr, "Failed to create window\n");
        return EXIT_FAILURE;
    }
    gtk_window_set_title(GTK_WINDOW(ctx.window), "Pulsrr Sequencer");
    gtk_window_set_default_size(GTK_WINDOW(ctx.window), MINIMAL_WINDOW_WIDTH, MINIMAL_WINDOW_HEIGHT);
    gtk_window_set_resizable(GTK_WINDOW(ctx.window), FALSE);  // Consider TRUE for usability
    g_signal_connect(ctx.window, "destroy", G_CALLBACK(on_app_destroy), NULL);

    // Set icon with error handling
    GError *error = NULL;
    char *icon_path = g_build_filename(paths->media_dir,"icon.png",NULL);
    if (!gtk_window_set_icon_from_file(GTK_WINDOW(ctx.window), icon_path, &error)) {
        add_main_log("[ERROR] Failed to load icon");
        g_clear_error(&error);
    }

    // Overlay for modals
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(ctx.window), overlay);

    // Main horizontal box
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(main_hbox, "main-container");
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), main_hbox);

    // Modal layer (hidden initially)
    ctx.modal_layer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(ctx.modal_layer, "modal-layer");
    gtk_widget_set_halign(ctx.modal_layer, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(ctx.modal_layer, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(ctx.modal_layer, TRUE);
    gtk_widget_set_vexpand(ctx.modal_layer, TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), ctx.modal_layer);
    gtk_widget_hide(ctx.modal_layer);

    // Left vertical container
    GtkWidget *left_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_DEFAULT);
    gtk_widget_set_name(left_container, "left-container");
    gtk_widget_set_size_request(left_container, LEFT_CONTAINER_WIDTH, -1);
    gtk_box_pack_start(GTK_BOX(main_hbox), left_container, FALSE, FALSE, 0);

    // Logo
	char *logo_path = g_build_filename(paths->media_dir,"logo.png",NULL);
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(logo_path, LOGO_WIDTH, -1, TRUE, &error);
    if (pixbuf) {
        GtkWidget *logo = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
        gtk_widget_set_halign(logo, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(left_container), logo, FALSE, FALSE, 0);
    } else {
        add_main_log("[ERROR] Failed to load logo");
        g_clear_error(&error);
    }

    // Aphorism widget
    gtk_box_pack_start(GTK_BOX(left_container), create_aphorism_widget(), FALSE, FALSE, 0);

    // Top buttons row
    GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, BUTTON_SPACING);
    gtk_widget_set_name(button_row, "button-row");
    gtk_widget_set_size_request(button_row, LEFT_CONTAINER_WIDTH, BUTTON_TOP_CONTROL_HEIGHT);
    gtk_box_pack_start(GTK_BOX(left_container), button_row, FALSE, FALSE, 0);

    GtkWidget *btn_add_seq = gtk_button_new_with_label("ADD SEQ.");
    GtkWidget *btn_update = gtk_button_new_with_label("UPDATE");
    gtk_widget_set_name(btn_add_seq, "btn-add-seq");
    gtk_widget_set_name(btn_update, "btn-update-render");
    gtk_widget_set_size_request(btn_add_seq, BUTTON_TOP_CONTROL_WIDTH, BUTTON_TOP_CONTROL_HEIGHT);
    gtk_widget_set_size_request(btn_update, BUTTON_TOP_CONTROL_WIDTH, BUTTON_TOP_CONTROL_HEIGHT);
    gtk_box_pack_start(GTK_BOX(button_row), btn_add_seq, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), btn_update, TRUE, TRUE, 0);

    // Layers
    for (int i = 0; i < MAX_LAYERS; i++) {
        GtkWidget *layer = create_layer_component(i);
        if (layer) {
            gtk_box_pack_start(GTK_BOX(left_container), layer, FALSE, FALSE, 0);
        }
    }

    // Log scrolled window
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_name(log_scroll, "main-log-scroll");
    gtk_widget_set_size_request(log_scroll, LEFT_CONTAINER_WIDTH, 60);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(left_container), log_scroll, FALSE, FALSE, 0);

    // Log text view
    GtkWidget *log_view = gtk_text_view_new();
    gtk_widget_set_name(log_view, "main-log-view");
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(log_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(log_view), GTK_WRAP_WORD_CHAR);
    GtkTextBuffer *log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
    gtk_text_buffer_set_text(log_buffer, "[LOG] App Start.\n", -1);
    gtk_container_add(GTK_CONTAINER(log_scroll), log_view);

    // Version label
    GtkWidget *version_label = gtk_label_new("Version 0.0.1 by Rekav");
    gtk_widget_set_name(version_label, "version-label");
    gtk_widget_set_halign(version_label, GTK_ALIGN_CENTER);
    gtk_box_pack_end(GTK_BOX(left_container), version_label, FALSE, FALSE, 0);

    // Right container
    GtkWidget *right_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(right_container, "right-container");
    gtk_box_pack_start(GTK_BOX(main_hbox), right_container, TRUE, TRUE, 0);

    GtkWidget *render_panel = NULL;
    GtkWidget *render_ui = create_render_screen(&render_panel);
    if (render_ui) {
        gtk_box_pack_start(GTK_BOX(right_container), render_ui, TRUE, TRUE, 0);
    }
    GtkWidget *sequencer = create_sequencer_component();
    if (sequencer) {
        gtk_box_pack_start(GTK_BOX(right_container), sequencer, FALSE, FALSE, 0);
    }

    if (render_panel) {
        gtk_widget_set_app_paintable(render_panel, TRUE);
        gtk_widget_set_has_window(render_panel, TRUE);
    }

    // Set up main_ui context
    ctx.main_ui.log_view = GTK_TEXT_VIEW(log_view);
    ctx.main_ui.log_buffer = log_buffer;
    ctx.main_ui.main_container = main_hbox;
    set_app_ctx(&ctx);

    // Connect signals
    g_signal_connect(btn_update, "clicked", G_CALLBACK(on_update_render_clicked), &ctx);
    g_signal_connect(btn_add_seq, "clicked", G_CALLBACK(on_add_button_clicked), main_hbox);
    gtk_widget_add_events(ctx.window, GDK_KEY_PRESS_MASK);
    g_signal_connect(ctx.window, "key-press-event", G_CALLBACK(on_key_press), &ctx);

    // Load CSS
    GtkCssProvider *css = gtk_css_provider_new();
    error = NULL;
    char *style_path = g_build_filename(paths->styles_dir,"style.css",NULL);
    if (!gtk_css_provider_load_from_path(css, style_path, &error)) {
        add_main_log("[ERROR] Failed to load style.css");
        g_clear_error(&error);
    } else {
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
    g_object_unref(css);

    gtk_widget_show_all(ctx.window);
    gtk_main();

    return EXIT_SUCCESS;
}

static void on_update_render_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;

    gboolean has_modified = FALSE;
    for (guint8 i = 0; i < MAX_LAYERS; i++) {
        if (sdl_get_layer_state(i) == LAYER_MODIFIED) {
            has_modified = TRUE;
            break;
        }
    }

    if (!has_modified) {
    	add_main_log("[UPDATE] No layers need updating. Skipping texture update.");
        return;
    }

    sdl_set_render_state(RENDER_STATE_LOADING);
    update_textures_async();
}

static void on_app_destroy(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;
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

