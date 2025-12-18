#include "utils.h"
#include "layer.h"
#include "modal_fx.h"
#include "sdl.h"
#include "modal_load.h"


GtkWidget *layer_preview_boxes[MAX_LAYERS] = { NULL };

GtkWidget* create_layer_component(const char *label_text, int layer_number) {

	// Overlay wrapper
	GtkWidget *overlay = gtk_overlay_new();
	gtk_widget_set_vexpand(overlay, FALSE);
	gtk_widget_set_valign(overlay, GTK_ALIGN_START);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name(vbox, "layer-vbox");
	gtk_container_add(GTK_CONTAINER(overlay), vbox);
	gtk_widget_set_valign(vbox, GTK_ALIGN_START);

	// Layer horizontal container (bordered box)
	GtkWidget *layer_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_widget_set_size_request(layer_hbox, -1, -1);
	gtk_widget_set_name(layer_hbox, "layer-box");
	gtk_box_pack_start(GTK_BOX(vbox), layer_hbox, FALSE, FALSE, 0);

	// Left vertical stack: label + buttons
	GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_set_size_request(left_vbox, -1, LAYER_COMPONENT_HEIGHT);
	gtk_box_pack_start(GTK_BOX(layer_hbox), left_vbox, FALSE, FALSE, 0);

	// Label
	GtkWidget *header_label = gtk_label_new(label_text);
	gtk_widget_set_name(header_label, "layer-header");
	gtk_widget_set_halign(header_label, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(left_vbox), header_label, FALSE, FALSE, 0);

	// Load button
	GtkWidget *btn_load = gtk_button_new_with_label("LOAD");
	gtk_widget_set_name(btn_load, "btn-load");
	gtk_box_pack_start(GTK_BOX(left_vbox), btn_load, FALSE, FALSE, 0);

	// Effects button
	GtkWidget *btn_fx = gtk_button_new_with_label("FX");
	gtk_widget_set_name(btn_fx, "btn-fx");
	gtk_box_pack_start(GTK_BOX(left_vbox), btn_fx, FALSE, FALSE, 0);

	// Preview box on the right
	GtkWidget *preview_box = gtk_event_box_new();
	gtk_widget_set_name(preview_box, "preview-box");
	gtk_widget_set_size_request(preview_box, 130, -1);
	gtk_box_pack_start(GTK_BOX(layer_hbox), preview_box, FALSE, FALSE, 6);

	if (layer_number >= 0 && layer_number <= MAX_LAYERS) {
	layer_preview_boxes[layer_number] = preview_box;
	}

	if (is_frames_file_empty(layer_number)) {
	GtkWidget *preview_label = gtk_label_new("(EMPTY)");
	gtk_widget_set_name(preview_label, "preview-label");
	gtk_widget_set_halign(preview_label, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(preview_label, GTK_ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(preview_box), preview_label);
	}

	// Small menu label in overlay
	GtkWidget *menu_label = gtk_label_new("⋮");
	gtk_widget_set_name(menu_label, "layer-menu-btn");
	gtk_label_set_xalign(GTK_LABEL(menu_label), 0.5);
	gtk_label_set_yalign(GTK_LABEL(menu_label), 0.5);

	GtkWidget *menu_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(menu_box), menu_label);
	gtk_widget_set_size_request(menu_box, 16, 16);
	gtk_widget_set_halign(menu_box, GTK_ALIGN_END);
	gtk_widget_set_valign(menu_box, GTK_ALIGN_START);

	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), menu_box);

	// Connect signals
	g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_button_clicked), GINT_TO_POINTER(layer_number));
	g_signal_connect(btn_fx, "clicked", G_CALLBACK(on_fx_button_clicked), GINT_TO_POINTER(layer_number));
	g_signal_connect(menu_box, "button-press-event", G_CALLBACK(on_layer_menu_label_click), NULL);

	return overlay;
}

void update_layer_preview(int layer_number)
{
    GtkWidget *preview_box = layer_preview_boxes[layer_number];
    if (!preview_box) {
        g_warning("update_layer_preview: preview_box is NULL for layer %d", layer_number);
        return;
    }

    char frame_path[512];
    snprintf(frame_path, sizeof(frame_path),
             "Frames_%d/frame_00001.png", layer_number);

    if (!g_file_test(frame_path, G_FILE_TEST_EXISTS)) {
        g_warning("update_layer_preview: file does NOT exist: %s", frame_path);
        return;
    }

    // Ensure widget has the correct name for CSS selector
    gtk_widget_set_name(preview_box, "sequence-preview-css");

	// Build absolute path first
	gchar *abs_path = g_strdup(frame_path);
	if (!g_path_is_absolute(frame_path)) {
	    gchar *cwd = g_get_current_dir();
	    abs_path = g_build_filename(cwd, frame_path, NULL);
	    g_free(cwd);
	}

	// Convert to URI
	gchar *uri = g_filename_to_uri(abs_path, NULL, NULL);
	g_free(abs_path);

	if (!uri) {
	    g_warning("update_layer_preview: failed to convert path to URI");
	    return;
	}


    // Build CSS
    gchar *css = g_strdup_printf(
        "#sequence-preview-css {"
        "  background-image: url('%s');"
        "  background-repeat: repeat;"
        "  background-position: center;"
        "  background-size: contain;"
        "}",
        uri
    );

    // Apply CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(preview_box);
    gtk_style_context_add_provider(
        context,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    g_message("update_layer_preview: CSS background set for layer %d", layer_number);

    // Cleanup
    g_free(uri);
    g_free(css);
    g_object_unref(provider);
}

gboolean update_preview_idle(gpointer data) {
    int layer_number = GPOINTER_TO_INT(data);
    g_print("[UPDATE PREVIEW] called for layer %d\n", layer_number);

    GtkWidget *preview_box = layer_preview_boxes[layer_number];
    if (!preview_box) {
        g_warning("update_preview_idle: preview_box is NULL for layer %d", layer_number);
        return FALSE;
    }

    // Clear previous children (this will remove "(EMPTY)" label if present)
    GList *children = gtk_container_get_children(GTK_CONTAINER(preview_box));
    for (GList *l = children; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    // Now call the real update
    update_layer_preview(layer_number);

    return FALSE; // remove from idle
}

gboolean on_layer_menu_label_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    g_print("open or toggle\n");
    return TRUE;  // stop further propagation
}

