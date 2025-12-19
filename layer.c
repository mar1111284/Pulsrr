#include "utils.h"
#include "layer.h"
#include "modal_fx.h"
#include "sdl.h"
#include "modal_load.h"

// Globals
GtkWidget *layer_preview_boxes[MAX_LAYERS] = { NULL };

GtkWidget* create_layer_component(guint8 layer_index) {

	// Overlay wrapper useless for now but in case
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
	char label_buf[32];
	g_snprintf(label_buf, sizeof(label_buf), "LAYER %u", layer_index + 1);
	GtkWidget *header_label = gtk_label_new(label_buf);
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

	if (layer_index >= 0 && layer_index <= MAX_LAYERS) {
		layer_preview_boxes[layer_index] = preview_box;
	}

	char folder_name[128];
	snprintf(folder_name, sizeof(folder_name), "Frames_%u", layer_index);

	// Only add "(EMPTY)" label if no frames exist
	if (count_frames(folder_name) == 0) {
		GtkWidget *preview_label = gtk_label_new("(EMPTY)");
		gtk_widget_set_name(preview_label, "preview-label");
		gtk_widget_set_halign(preview_label, GTK_ALIGN_CENTER);
		gtk_widget_set_valign(preview_label, GTK_ALIGN_CENTER);
		gtk_container_add(GTK_CONTAINER(preview_box), preview_label);
	}

	// Small menu label in overlay (not implemented now)
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
	g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_button_clicked), GINT_TO_POINTER(layer_index));
	//g_signal_connect(btn_fx, "clicked", G_CALLBACK(on_fx_button_clicked), GINT_TO_POINTER(layer_index));
	g_signal_connect(menu_box, "button-press-event", G_CALLBACK(on_layer_menu_label_click), NULL);

	return overlay;
}

void set_preview_thumbnail(guint8 layer_index)
{
    if (layer_index >= MAX_LAYERS) {
        g_warning("set_preview_thumbnail: invalid layer_index %d", layer_index);
        return;
    }

    GtkWidget *preview_box = layer_preview_boxes[layer_index];
    if (!preview_box) {
        g_warning("set_preview_thumbnail: preview_box is NULL for layer %d", layer_index);
        return;
    }

    // Clear previous children
    GList *children = gtk_container_get_children(GTK_CONTAINER(preview_box));
    for (GList *l = children; l; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);

    char folder[128];
    snprintf(folder, sizeof(folder), "Frames_%u", layer_index);

    // Check if folder has frames
    if (count_frames(folder) == 0) {
        GtkWidget *empty_label = gtk_label_new("(EMPTY)");
        gtk_widget_set_name(empty_label, "preview-label");
        gtk_widget_set_halign(empty_label, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(empty_label, GTK_ALIGN_CENTER);
        gtk_container_add(GTK_CONTAINER(preview_box), empty_label);
        gtk_widget_show_all(preview_box);
        return;
    }

    // First frame path
    char frame_path[512];
    snprintf(frame_path, sizeof(frame_path), "%s/frame_00001.png", folder);

    gchar *abs_path = g_strdup(frame_path);
    if (!g_path_is_absolute(frame_path)) {
        gchar *cwd = g_get_current_dir();
        gchar *tmp = g_build_filename(cwd, frame_path, NULL);
        g_free(abs_path);
        abs_path = tmp;
        g_free(cwd);
    }

    // Convert to URI
    gchar *uri = g_filename_to_uri(abs_path, NULL, NULL);
    g_free(abs_path);
    if (!uri) {
        g_warning("set_preview_thumbnail: failed to convert path to URI");
        return;
    }

    // Apply CSS
    gtk_widget_set_name(preview_box, "sequence-preview-css");
    gchar *css = g_strdup_printf(
        "#sequence-preview-css {"
        "  background-image: url('%s');"
        "  background-repeat: repeat;"
        "  background-position: center;"
        "  background-size: contain;"
        "}",
        uri
    );

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(preview_box);
    gtk_style_context_add_provider(
        context,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    // Cleanup
    g_free(uri);
    g_free(css);
    g_object_unref(provider);

    gtk_widget_show_all(preview_box);
}

gboolean on_layer_menu_label_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    g_print("open or toggle\n");
    return TRUE;  // stop further propagation
}

