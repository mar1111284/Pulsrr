/* Components */
#include "component_layer.h"

/* Modals */
#include "../modals/modal_fx.h"
#include "../modals/modal_load_video.h"

/* SDL */
#include "../sdl/sdl.h"


// Global preview box - add to layer struct ? 
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

	// Delete layer button
	GtkWidget *delete_layer_btn = gtk_label_new("x");
	gtk_widget_set_name(delete_layer_btn, "layer-delete-btn");
	gtk_label_set_xalign(GTK_LABEL(delete_layer_btn), 0.5);
	gtk_label_set_yalign(GTK_LABEL(delete_layer_btn), 0.5);

	GtkWidget *delete_btn_container = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(delete_btn_container), delete_layer_btn);
	gtk_widget_set_size_request(delete_btn_container, 16, 16);
	gtk_widget_set_halign(delete_btn_container, GTK_ALIGN_END);
	gtk_widget_set_valign(delete_btn_container, GTK_ALIGN_START);

	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), delete_btn_container);

	// Connect signals
	g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_button_clicked), GINT_TO_POINTER(layer_index));
	g_signal_connect(btn_fx, "clicked", G_CALLBACK(on_fx_button_clicked), GINT_TO_POINTER(layer_index));
	g_signal_connect(delete_btn_container,"button-press-event",G_CALLBACK(on_layer_delete_click),GINT_TO_POINTER(layer_index));

	return overlay;
}

void set_preview_thumbnail(guint8 layer_index)
{
    if (layer_index >= MAX_LAYERS) {
        add_main_log(g_strdup_printf("[WARN] set_preview_thumbnail: invalid layer_index %u", layer_index));
        return;
    }

    GtkWidget *preview_box = layer_preview_boxes[layer_index];
    if (!preview_box) {
        add_main_log(g_strdup_printf("[WARN] set_preview_thumbnail: preview_box is NULL for layer %u", layer_index));
        return;
    }

    // 1. Clear all child widgets
    GList *children = gtk_container_get_children(GTK_CONTAINER(preview_box));
    for (GList *l = children; l; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);

    // 2. RESET CSS: go back to default name → uses clean style from style.css
    gtk_widget_set_name(preview_box, "preview-box");

    char folder[128];
    snprintf(folder, sizeof(folder), "Frames_%u", layer_index + 1);

    if (count_frames(folder) == 0) {
        // EMPTY STATE
        GtkWidget *empty_label = gtk_label_new("(EMPTY)");
        gtk_widget_set_name(empty_label, "preview-label");
        gtk_widget_set_halign(empty_label, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(empty_label, GTK_ALIGN_CENTER);
        gtk_container_add(GTK_CONTAINER(preview_box), empty_label);

        add_main_log(g_strdup_printf("[INFO] Layer %u preview set to EMPTY", layer_index + 1));
    } else {
        // HAS FRAMES: apply unique CSS background
        char frame_path[512];
        snprintf(frame_path, sizeof(frame_path), "%s/frame_00001.png", folder);

        gchar *abs_path = g_path_is_absolute(frame_path)
            ? g_strdup(frame_path)
            : g_build_filename(g_get_current_dir(), frame_path, NULL);

        gchar *uri = g_filename_to_uri(abs_path, NULL, NULL);
        g_free(abs_path);

        if (!uri) {
            add_main_log("[WARN] Failed to create URI for thumbnail");
            goto show_empty_fallback;
        }

        // Use unique ID to avoid CSS conflicts
        gchar *css_id = g_strdup_printf("preview-layer-%u", layer_index + 1);
        gtk_widget_set_name(preview_box, css_id);

        gchar *css = g_strdup_printf(
            "#%s {"
            "  background-image: url('%s');"
            "  background-repeat: no-repeat;"
            "  background-position: center;"
            "  background-size: contain;"
            "  background-color: black;"
            "}",
            css_id,
            uri
        );

        GtkCssProvider *provider = gtk_css_provider_new();
        GError *error = NULL;
        gtk_css_provider_load_from_data(provider, css, -1, &error);
        if (error) {
            add_main_log(g_strdup_printf("[ERROR] CSS load failed: %s", error->message));
            g_clear_error(&error);
            goto cleanup_css;
        }

        GtkStyleContext *context = gtk_widget_get_style_context(preview_box);
        gtk_style_context_add_provider(context,
                                       GTK_STYLE_PROVIDER(provider),
                                       GTK_STYLE_PROVIDER_PRIORITY_USER);

    cleanup_css:
        g_object_unref(provider);
        g_free(css);
        g_free(css_id);
        g_free(uri);
        add_main_log(g_strdup_printf("[INFO] Layer %u thumbnail updated", layer_index + 1));
        return;

    show_empty_fallback:
        // In case of error, fall back to empty
        GtkWidget *empty_label = gtk_label_new("(EMPTY)");
        gtk_container_add(GTK_CONTAINER(preview_box), empty_label);
    }

    gtk_widget_show_all(preview_box);
}

gboolean on_layer_delete_click(GtkWidget *widget,
                               GdkEventButton *event,
                               gpointer user_data)
{
    (void)widget;
    (void)event;

    guint8 layer_index = GPOINTER_TO_UINT(user_data);

    add_main_log(g_strdup_printf("[UI] User requested delete for layer %u", layer_index + 1));

    // === Delete folder from disk ===
    char folder_name[64];
    snprintf(folder_name, sizeof(folder_name), "Frames_%u", layer_index + 1);

    gchar *abs_folder = g_build_filename(g_get_current_dir(), folder_name, NULL);

    if (g_file_test(abs_folder, G_FILE_TEST_IS_DIR)) {
        GError *error = NULL;
        GDir *dir = g_dir_open(abs_folder, 0, &error);

        if (dir) {
            const char *entry;
            while ((entry = g_dir_read_name(dir))) {
                gchar *file_path = g_build_filename(abs_folder, entry, NULL);
                if (unlink(file_path) != 0) {
                    add_main_log(g_strdup_printf("[WARN] Could not delete file: %s", file_path));
                }
                g_free(file_path);
            }
            g_dir_close(dir);

            if (rmdir(abs_folder) == 0) {
                add_main_log(g_strdup_printf("[INFO] Deleted folder: %s", abs_folder));
            } else {
                add_main_log(g_strdup_printf("[WARN] Could not remove empty folder: %s", abs_folder));
            }
        } else {
            add_main_log(g_strdup_printf("[ERROR] Failed to open folder for deletion: %s", 
                                         error ? error->message : "unknown"));
            g_clear_error(&error);
        }
    } else {
        add_main_log("[INFO] No folder to delete (already empty)");
    }

    g_free(abs_folder);
    sdl_clear_layer(layer_index);
    set_preview_thumbnail(layer_index);
    add_main_log(g_strdup_printf("[UI] Layer %u deleted and cleared successfully", layer_index + 1));

    return TRUE;  // Event consumed
}

