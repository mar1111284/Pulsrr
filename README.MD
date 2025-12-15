/*
Notes:

Architecture:

Main container: specs: (1200*700); fixed size not expandable; background #FFFFFF

	Left container: specs: (300*700); fixed size not expandable; background #212121
	
		App Logo Image: specs: (300*42); fixed size not expandable; Image/logo.png
		
		Header box: "Layer 1" specs (300*5); fixed size not expandable; text align left
		
		Layer container 1: specs: (300*42); fixed size not expandable;
		
			Left container: specs: (50*42);  fixed size not expandable
				Button Edit
				Button Effects
				Button Replace
				Button Delete
				
			Right container: specs: (250*42); fixed size not expandable
				Image preview; src: empty.png
		
		Header box: "Layer 2" specs (300*5); fixed size not expandable; text align left
				
		Layer container 2 (idem ...)
		
		Header box: "Layer 3" specs (300*5); fixed size not expandable; text align left

		Layer container 3 (idem ...)
		
		Header box: "Layer 4" specs (300*5); fixed size not expandable; text align left
		
		Layer container 4 (idem ...)
		
		Header box: "Layer 5" specs (300*5); fixed size not expandable; text align left
		
		Layer container 5 (idem ...)

	Right container: specs: (900*700); fixed size not expandable; background #FFFFFF
		
		SDL container: specs: (900*600); fixed size not expandable;
		Sequencer container: specs: (900*100); fixed size not expandable;

*/

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>

#define WINDOW_WIDTH   1200
#define WINDOW_HEIGHT  700
#define CONTROL_WIDTH  300
#define SEQ_HEIGHT     150

#define IMAGE_PREVIEW_WIDTH   150
#define IMAGE_PREVIEW_HEIGHT  75

#define BG_COLOR       "#212121"
#define SEQ_COLOR      "#00FF00"
#define LAYER_COLOR    "#FF3333"

// ESC key quits app
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
        return TRUE;
    }
    return FALSE;
}

void on_load_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *controls_box = GTK_WIDGET(user_data); // use controls box to pack previews
    GtkWidget *dialog, *content_area, *file_chooser;
    GtkWidget *fps_spin, *scale_combo;
    GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;

    dialog = gtk_dialog_new_with_buttons("Load Video",
                                         GTK_WINDOW(gtk_widget_get_toplevel(controls_box)),
                                         flags,
                                         "_Load", GTK_RESPONSE_ACCEPT,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // File chooser
    file_chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_box_pack_start(GTK_BOX(content_area), file_chooser, TRUE, TRUE, 5);

    // FPS spin button
    fps_spin = gtk_spin_button_new_with_range(1, 60, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(fps_spin), 25);
    gtk_box_pack_start(GTK_BOX(content_area), fps_spin, FALSE, FALSE, 5);

    // Scale combo box (resolution selector)
    scale_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "1080p");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "720p");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "480p");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "360p");
    gtk_combo_box_set_active(GTK_COMBO_BOX(scale_combo), 2); // default 480p
    gtk_box_pack_start(GTK_BOX(content_area), scale_combo, FALSE, FALSE, 5);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
        int fps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fps_spin));
        const gchar *scale_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(scale_combo));

        int scale_width = 854; // default 480p
        if (g_strcmp0(scale_text, "1080p") == 0) scale_width = 1920;
        else if (g_strcmp0(scale_text, "720p") == 0) scale_width = 1280;
        else if (g_strcmp0(scale_text, "480p") == 0) scale_width = 854;
        else if (g_strcmp0(scale_text, "360p") == 0) scale_width = 640;

        // Determine folder number
        static int video_count = 1;
        char folder[256];
        snprintf(folder, sizeof(folder), "Frames_%d", video_count);

        // Clean folder first
        char clean_cmd[512];
        snprintf(clean_cmd, sizeof(clean_cmd), "mkdir -p \"%s\" && rm -f \"%s\"/*", folder, folder);
        g_print("Cleaning folder: %s\n", clean_cmd);
        system(clean_cmd);

        // Run ffmpeg
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -i \"%s\" -vf scale=%d:-1,fps=%d \"%s/frame_%%05d.png\" > /dev/null 2>&1",
                 filename, scale_width, fps, folder);
        g_print("Running command: %s\n", cmd);
        system(cmd);

        // Display first frame under Load button
        char first_frame[512];
        snprintf(first_frame, sizeof(first_frame), "%s/frame_00001.png", folder);

        GError *error = NULL;
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(first_frame, &error);
        if (!pixbuf) {
            g_print("Error loading image: %s\n", error->message);
            g_error_free(error);
        } else {
            // Scale pixbuf to fixed width while keeping aspect ratio
            int preview_width = 150;
            int preview_height = (int)((double)preview_width / gdk_pixbuf_get_width(pixbuf) * gdk_pixbuf_get_height(pixbuf));
            GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, preview_width, preview_height, GDK_INTERP_BILINEAR);
            GtkWidget *preview = gtk_image_new_from_pixbuf(scaled);

            gtk_box_pack_start(GTK_BOX(controls_box), preview, FALSE, FALSE, 5);
            gtk_widget_show_all(controls_box);

            g_object_unref(pixbuf);
            g_object_unref(scaled);
        }

        g_free(filename);
        video_count++;
    }

    gtk_widget_destroy(dialog);
}

// Create a single layer skeleton
GtkWidget* create_layer(int layer_number) {
    GtkWidget *layer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(layer_box, CONTROL_WIDTH - 20, 120); // fixed height

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "* { background-color: " LAYER_COLOR "; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(layer_box),
                                   GTK_STYLE_PROVIDER(css),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);

    // Layer title
    char title[32];
    snprintf(title, sizeof(title), "Layer %d", layer_number);
    GtkWidget *label_title = gtk_label_new(title);
    gtk_box_pack_start(GTK_BOX(layer_box), label_title, FALSE, FALSE, 2);

    // Preview placeholder
    GtkWidget *label_preview = gtk_label_new("(Empty)");
    gtk_box_pack_start(GTK_BOX(layer_box), label_preview, TRUE, TRUE, 2);

    // Button menu
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkWidget *btn_replace = gtk_button_new_with_label("Replace");
    GtkWidget *btn_delete  = gtk_button_new_with_label("Delete");
    GtkWidget *btn_effects = gtk_button_new_with_label("Effects");
    GtkWidget *btn_edit    = gtk_button_new_with_label("Edit");
    gtk_box_pack_start(GTK_BOX(button_box), btn_replace, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(button_box), btn_delete,  TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(button_box), btn_effects, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(button_box), btn_edit,    TRUE, TRUE, 2);

    gtk_box_pack_start(GTK_BOX(layer_box), button_box, FALSE, FALSE, 2);

    return layer_box;
}

// ==========================
// MAIN
// ==========================
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "PULSRR");
    gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(window), hbox);

    // ---- Left Control Panel ----
    GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(controls, CONTROL_WIDTH, -1);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "* { background-color: " BG_COLOR "; color: white; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(controls),
                                   GTK_STYLE_PROVIDER(css),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *label_ctrl = gtk_label_new("Controls");
    gtk_box_pack_start(GTK_BOX(controls), label_ctrl, FALSE, FALSE, 10);

    // Load Video Button
    GtkWidget *load_button = gtk_button_new_with_label("Load Video");
    gtk_box_pack_start(GTK_BOX(controls), load_button, FALSE, FALSE, 10);
    g_signal_connect(load_button, "clicked", G_CALLBACK(on_load_video_clicked), controls);

    // --- 5 Layer Skeletons ---
    for (int i = 1; i <= 5; i++) {
        GtkWidget *layer = create_layer(i);
        gtk_box_pack_start(GTK_BOX(controls), layer, FALSE, FALSE, 5);
    }

    // ---- Right Vertical Container ----
    GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(right_vbox, TRUE);
    gtk_widget_set_vexpand(right_vbox, TRUE);

    GtkWidget *render_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(render_area, TRUE);
    gtk_widget_set_vexpand(render_area, TRUE);
    gtk_style_context_add_provider(gtk_widget_get_style_context(render_area),
                                   GTK_STYLE_PROVIDER(css),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *sequencer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(sequencer, -1, SEQ_HEIGHT);
    GtkCssProvider *seq_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(seq_css,
        "* { background-color: " SEQ_COLOR "; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(sequencer),
                                   GTK_STYLE_PROVIDER(seq_css),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *seq_label = gtk_label_new("SEQUENCE");
    gtk_box_pack_start(GTK_BOX(sequencer), seq_label, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(right_vbox), render_area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(right_vbox), sequencer, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), controls, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), right_vbox, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

