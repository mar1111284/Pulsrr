#include <gtk/gtk.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "sdl.h"
#include "utils.h"

// SOME GLOBALS

GtkWidget *global_modal_layer = NULL;
GtkWidget *layer_preview_boxes[MAX_LAYERS] = { NULL };

// Callback
void on_update_render_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *render_panel = GTK_WIDGET(user_data);
    sdl_restart(render_panel);
}

static gboolean close_modal_cb(gpointer data) {
    GtkWidget *modal = GTK_WIDGET(data);
    gtk_widget_hide(modal);
    return G_SOURCE_REMOVE; // run once
}

void update_layer_preview(int layer_number) {

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

    // Clear previous preview
    GList *children = gtk_container_get_children(GTK_CONTAINER(preview_box));
    if (children) {
        for (GList *l = children; l; l = l->next) {
            gtk_widget_destroy(GTK_WIDGET(l->data));
        }
        g_list_free(children);
        g_message("update_layer_preview: old preview removed for layer %d", layer_number);
    }

    // Load image
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(frame_path, &error);
    if (!pixbuf) {
        g_warning("update_layer_preview: Pixbuf load error: %s", error->message);
        g_error_free(error);
        return;
    }

    int pw = gdk_pixbuf_get_width(pixbuf);
    int ph = gdk_pixbuf_get_height(pixbuf);

    int target_width = 116;
    int target_height = (int)((double)target_width / pw * ph);

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf,
                                                target_width,
                                                target_height,
                                                GDK_INTERP_BILINEAR);

    if (!scaled) {
        g_warning("update_layer_preview: scaling failed for layer %d", layer_number);
        g_object_unref(pixbuf);
        return;
    }

    GtkWidget *image = gtk_image_new_from_pixbuf(scaled);
    gtk_container_add(GTK_CONTAINER(preview_box), image);
    gtk_widget_queue_draw(preview_box);
    gtk_widget_show_all(preview_box);
    
    g_message("update_layer_preview: new preview set for layer %d", layer_number);

    g_object_unref(pixbuf);
    g_object_unref(scaled);
}



gboolean update_preview_idle(gpointer data) {
    int layer_number = GPOINTER_TO_INT(data);
    int test_layer = GPOINTER_TO_INT(data);
    g_print("[UPDATE PREVIEW] called for layer %d\n", test_layer);

    update_layer_preview(layer_number);
    return FALSE;
}


typedef struct {
    char filename[512];
    int fps;
    int scale_width;
    char folder[256];
    int layer_number;
    GtkWidget *progress_bar;
} ExportJob;



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

typedef struct {
    GtkWidget *progress_bar;
    double fraction;
    char *text;
} ProgressUpdate;

gboolean update_progress_cb(gpointer data) {
    ProgressUpdate *upd = (ProgressUpdate *)data;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(upd->progress_bar), upd->fraction);
    if (upd->text)
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(upd->progress_bar), upd->text);

    g_free(upd->text);
    g_free(upd);

    return G_SOURCE_REMOVE; // run once
}

static gboolean export_done_cb(gpointer data) {
    ExportJob *job = (ExportJob *)data;

    // Ensure UI is at 100%
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(job->progress_bar), 1.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(job->progress_bar), "Export completed");

    // Close modal after 3 seconds
    g_timeout_add_seconds(3, close_modal_cb, global_modal_layer);
    
    // render play 
    sdl_set_playing(1);

    return G_SOURCE_REMOVE;
}


gpointer export_thread_func(gpointer data) {
    ExportJob *job = (ExportJob *)data;

    char line[256];
    double fraction = 0.0;

    // Clean folder
    char clean_cmd[512];
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

void on_export_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget **widgets = (GtkWidget **)user_data;

    const gchar *filename = gtk_label_get_text(GTK_LABEL(widgets[0]));
    int fps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widgets[1]));
    const gchar *scale_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widgets[2]));
    int layer_number = GPOINTER_TO_INT(widgets[3]);
    GtkWidget *progress_bar = widgets[4];

    // Debug print
    g_print("[EXPORT_CLICK] layer_number = %d\n", layer_number);

    // Determine scale width
    int scale_width = 854; // default 480p
    if (g_strcmp0(scale_text, "1080p") == 0) scale_width = 1920;
    else if (g_strcmp0(scale_text, "720p") == 0) scale_width = 1280;
    else if (g_strcmp0(scale_text, "480p") == 0) scale_width = 854;
    else if (g_strcmp0(scale_text, "360p") == 0) scale_width = 640;

    // Folder name
    char folder[256];
    snprintf(folder, sizeof(folder), "Frames_%d", layer_number);

    // Create heap-allocated job
    ExportJob *job = g_malloc(sizeof(ExportJob));
    strncpy(job->filename, filename, sizeof(job->filename));
    job->fps = fps;
    job->scale_width = scale_width;
    strncpy(job->folder, folder, sizeof(job->folder));
    job->progress_bar = progress_bar;
    job->layer_number = layer_number;

    // Start thread
    g_thread_new("export-thread", export_thread_func, job);
}



void on_load_button_clicked(GtkButton *button, gpointer user_data) {

	// put pause
	sdl_set_playing(0);

	int layer_number = GPOINTER_TO_INT(user_data);
	g_print("Load clicked on layer %d\n", layer_number); // TO DELETE 

	// Clear previous content if any
	GList *children = gtk_container_get_children(GTK_CONTAINER(global_modal_layer));
	for (GList *iter = children; iter != NULL; iter = iter->next) {
	gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	// Create a black box
	GtkWidget *black_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_size_request(black_box,400 ,400);
	gtk_widget_set_name(black_box, "black-modal");
	gtk_widget_set_halign(black_box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(black_box, GTK_ALIGN_CENTER);

	// --------------------
	// Selected file label
	// --------------------
	GtkWidget *selected_file_label = gtk_label_new("Drag an MP4 file here");
	gtk_widget_set_name(selected_file_label, "selected-file-label");
	gtk_box_pack_start(GTK_BOX(black_box), selected_file_label, FALSE, FALSE, 5);

	// --------------------
	// Enable drag-and-drop on black_box
	// --------------------
	GtkTargetEntry targets[] = {
	{ "text/uri-list", 0, 0 }
	};
	gtk_drag_dest_set(GTK_WIDGET(black_box),
		      GTK_DEST_DEFAULT_ALL,
		      targets,
		      G_N_ELEMENTS(targets),
		      GDK_ACTION_COPY);

	g_signal_connect(black_box, "drag-data-received",G_CALLBACK(on_drag_data_received), selected_file_label);
		     
	// --------------------
	// Button row: Cancel + Export
	// --------------------
	GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35); // spacing 35px
	gtk_widget_set_name(button_row, "button-row");
	gtk_widget_set_size_request(button_row, 400, 50); // modal width, row height
	gtk_box_pack_end(GTK_BOX(black_box), button_row, FALSE, FALSE, 0);

	// Cancel button
	GtkWidget *btn_cancel = gtk_button_new_with_label("CANCEL");
	gtk_widget_set_name(btn_cancel, "modal-cancel-button");
	gtk_widget_set_size_request(btn_cancel, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_cancel, TRUE, TRUE, 0);

	// Export button
	GtkWidget *btn_export = gtk_button_new_with_label("EXPORT");
	gtk_widget_set_name(btn_export, "modal-export-button");
	gtk_widget_set_size_request(btn_export, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_export, TRUE, TRUE, 0);

	// --------------------
	// Controls row: FPS + Scale
	// --------------------
	GtkWidget *controls_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35); // spacing 35px
	gtk_widget_set_name(controls_row, "controls-row");
	gtk_widget_set_size_request(controls_row, 400, 50); // modal width, row height
	gtk_box_pack_end(GTK_BOX(black_box), controls_row, FALSE, FALSE, 5);

	// FPS spin button
	GtkWidget *fps_spin = gtk_spin_button_new_with_range(1, 60, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(fps_spin), 25);
	gtk_widget_set_name(controls_row, "spin-button");
	gtk_widget_set_size_request(fps_spin, 150, 42); // fixed size
	gtk_box_pack_start(GTK_BOX(controls_row), fps_spin, TRUE, TRUE, 0);

	// Scale combo box
	GtkWidget *scale_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "1080p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "720p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "480p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "360p");
	gtk_combo_box_set_active(GTK_COMBO_BOX(scale_combo), 2); // default 480p
	gtk_widget_set_size_request(scale_combo, 150, 42); // fixed size
	gtk_box_pack_start(GTK_BOX(controls_row), scale_combo, TRUE, TRUE, 0);
	
	// --------------------
	// Export progress bar
	// --------------------
	GtkWidget *progress_bar = gtk_progress_bar_new();
	gtk_widget_set_name(progress_bar, "export-progress");
	gtk_widget_set_size_request(progress_bar, 360, 20);

	// Initial state
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "0%");

	// Pack it ABOVE the controls row
	gtk_box_pack_end(GTK_BOX(black_box), progress_bar, FALSE, FALSE, 10);

	// Allocate heap memory for widgets array
	GtkWidget **widgets = g_malloc(sizeof(GtkWidget*) * 5); // add progress bar
	widgets[0] = selected_file_label;
	widgets[1] = fps_spin;
	widgets[2] = scale_combo;
	widgets[3] = GINT_TO_POINTER(layer_number);
	widgets[4] = progress_bar;
	
	g_print("[LOAD_BUTTON] layer_number=%d\n", layer_number);


	
	g_signal_connect(btn_cancel, "clicked", G_CALLBACK(on_modal_cancel_clicked), global_modal_layer);
	g_signal_connect(btn_export, "clicked", G_CALLBACK(on_export_clicked), widgets); // widgets array

	gtk_container_add(GTK_CONTAINER(global_modal_layer), black_box);

	gtk_widget_show_all(global_modal_layer);
}


/* ---------------------------------------------------
 * FX MODAL (STEP 1 – UI SKELETON)
 * --------------------------------------------------- */
typedef struct {
    int layer_number;  	
    GtkSpinButton *speed_spin;
    GtkSpinButton *alpha_spin;
    GtkSpinButton *threshold_spin;
    GtkSpinButton *contrast_spin;
    GtkToggleButton *gray_check;
    GtkToggleButton *invert_check;
} LayerFxWidgets;


void on_fx_apply_clicked(GtkButton *button, gpointer user_data) {
	LayerFxWidgets *fx = (LayerFxWidgets *)user_data;

	int layer = fx->layer_number;
	double speed = gtk_spin_button_get_value(fx->speed_spin);
	int alpha = gtk_spin_button_get_value_as_int(fx->alpha_spin);
	int threshold = gtk_spin_button_get_value_as_int(fx->threshold_spin);
	double contrast = gtk_spin_button_get_value(fx->contrast_spin);
	gboolean grayscale = gtk_toggle_button_get_active(fx->gray_check);
	gboolean invert = gtk_toggle_button_get_active(fx->invert_check);

	g_print("APPLY FX for layer %d:\n", layer);
	g_print("  Speed: %.2f\n", speed);
	g_print("  Alpha: %d\n", alpha);
	g_print("  Threshold: %d\n", threshold);
	g_print("  Contrast: %.2f\n", contrast);
	g_print("  Grayscale: %s\n", grayscale ? "ON" : "OFF");
	g_print("  Invert: %s\n", invert ? "ON" : "OFF");

	// Apply transparency
	set_transparency(layer, alpha);

	// Apply grayscale
	set_gray(layer, grayscale ? 1 : 0);

	// TODO: handle speed, threshold, contrast, invert later
}


void on_fx_button_clicked(GtkButton *button, gpointer user_data) {
	int layer_number = GPOINTER_TO_INT(user_data);
	g_print("EFFECTS button clicked for layer %d\n", layer_number);

	/* Pause rendering */
	sdl_set_playing(0);

	/* Clear modal */
	GList *children = gtk_container_get_children(GTK_CONTAINER(global_modal_layer));
	for (GList *iter = children; iter; iter = iter->next)
	gtk_widget_destroy(GTK_WIDGET(iter->data));
	g_list_free(children);

	/* Black modal box */
	GtkWidget *black_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
	gtk_widget_set_size_request(black_box, 420, 420);
	gtk_widget_set_name(black_box, "black-modal");
	gtk_widget_set_halign(black_box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(black_box, GTK_ALIGN_CENTER);

	/* Title */
	GtkWidget *title = gtk_label_new("FX");
	gtk_widget_set_name(title, "modal-title");
	gtk_box_pack_start(GTK_BOX(black_box), title, FALSE, FALSE, 10);

	/* --------------------
	 * FX Row 1: Speed + Alpha (50% each)
	 * -------------------- */
	GtkWidget *fx_row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_hexpand(fx_row1, TRUE);

	/* Speed container */
	GtkWidget *speed_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(speed_box, TRUE);
	GtkWidget *speed_label = gtk_label_new("Speed");
	GtkWidget *speed_spin = gtk_spin_button_new_with_range(0.25, 4.0, 0.05);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(speed_spin), 1.0);
	gtk_box_pack_start(GTK_BOX(speed_box), speed_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(speed_box), speed_spin, TRUE, TRUE, 0);

	/* Alpha container */
	GtkWidget *alpha_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(alpha_box, TRUE);
	GtkWidget *alpha_label = gtk_label_new("Alpha");
	GtkWidget *alpha_spin = gtk_spin_button_new_with_range(0, 255, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(alpha_spin), 255);
	gtk_box_pack_start(GTK_BOX(alpha_box), alpha_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(alpha_box), alpha_spin, TRUE, TRUE, 0);

	/* Pack both 50% boxes */
	gtk_box_pack_start(GTK_BOX(fx_row1), speed_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(fx_row1), alpha_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(black_box), fx_row1, FALSE, FALSE, 5);

	/* --------------------
	 * FX Row 2: Threshold + Contrast
	 * -------------------- */
	GtkWidget *fx_row2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_hexpand(fx_row2, TRUE);

	/* Threshold */
	GtkWidget *threshold_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(threshold_box, TRUE);
	GtkWidget *threshold_label = gtk_label_new("Threshold");
	GtkWidget *threshold_spin = gtk_spin_button_new_with_range(0, 255, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold_spin), 128);
	gtk_box_pack_start(GTK_BOX(threshold_box), threshold_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(threshold_box), threshold_spin, TRUE, TRUE, 0);

	/* Contrast */
	GtkWidget *contrast_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(contrast_box, TRUE);
	GtkWidget *contrast_label = gtk_label_new("Contrast");
	GtkWidget *contrast_spin = gtk_spin_button_new_with_range(0.2, 3.0, 0.05);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(contrast_spin), 1.0);
	gtk_box_pack_start(GTK_BOX(contrast_box), contrast_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(contrast_box), contrast_spin, TRUE, TRUE, 0);

	/* Pack both 50% boxes */
	gtk_box_pack_start(GTK_BOX(fx_row2), threshold_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(fx_row2), contrast_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(black_box), fx_row2, FALSE, FALSE, 5);

	/* --------------------
	 * FX Row 3: Grayscale + Invert
	 * -------------------- */
	GtkWidget *fx_row3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_hexpand(fx_row3, TRUE);

	/* Grayscale */
	GtkWidget *gray_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(gray_box, TRUE);
	GtkWidget *gray_label = gtk_label_new("Gray");
	GtkWidget *gray_check = gtk_check_button_new();

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gray_check), is_layer_gray(layer_number));
	gtk_box_pack_start(GTK_BOX(gray_box), gray_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(gray_box), gray_check, TRUE, TRUE, 0);

	/* Invert */
	GtkWidget *invert_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(invert_box, TRUE);
	GtkWidget *invert_label = gtk_label_new("Invert");
	GtkWidget *invert_check = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invert_check), FALSE);
	gtk_box_pack_start(GTK_BOX(invert_box), invert_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(invert_box), invert_check, TRUE, TRUE, 0);

	/* Pack both 50% boxes */
	gtk_box_pack_start(GTK_BOX(fx_row3), gray_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(fx_row3), invert_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(black_box), fx_row3, FALSE, FALSE, 5);
	
	/* Example: set min width for labels in a row */
	gtk_widget_set_size_request(speed_label, 80, -1);      // 80px min width, height unchanged
	gtk_widget_set_size_request(alpha_label, 80, -1);
	gtk_widget_set_size_request(threshold_label, 80, -1);
	gtk_widget_set_size_request(contrast_label, 80, -1);
	gtk_widget_set_size_request(gray_label, 80, -1);
	gtk_widget_set_size_request(invert_label, 80, -1);

	/* --------------------
	* Buttons
	* -------------------- */
	GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_size_request(button_row, 420, 50);

	GtkWidget *btn_back = gtk_button_new_with_label("BACK");
	GtkWidget *btn_apply = gtk_button_new_with_label("APPLY FX");

	gtk_widget_set_size_request(btn_back, 150, 42);
	gtk_widget_set_size_request(btn_apply, 150, 42);

	gtk_box_pack_start(GTK_BOX(button_row), btn_back, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(button_row), btn_apply, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(black_box), button_row, FALSE, FALSE, 10);

	/* Signals */
	g_signal_connect(btn_back,
		     "clicked",
		     G_CALLBACK(on_modal_cancel_clicked),
		     global_modal_layer);

	LayerFxWidgets *fx_widgets = g_malloc(sizeof(LayerFxWidgets));
	fx_widgets->layer_number = layer_number;  // pass the layer
	fx_widgets->speed_spin = GTK_SPIN_BUTTON(speed_spin);
	fx_widgets->alpha_spin = GTK_SPIN_BUTTON(alpha_spin);
	fx_widgets->threshold_spin = GTK_SPIN_BUTTON(threshold_spin);
	fx_widgets->contrast_spin = GTK_SPIN_BUTTON(contrast_spin);
	fx_widgets->gray_check = GTK_TOGGLE_BUTTON(gray_check);
	fx_widgets->invert_check = GTK_TOGGLE_BUTTON(invert_check);

	/* Connect Apply button */
	g_signal_connect(btn_apply, "clicked", G_CALLBACK(on_fx_apply_clicked), fx_widgets);


	/* Show modal */
	gtk_container_add(GTK_CONTAINER(global_modal_layer), black_box);
	gtk_widget_show_all(global_modal_layer);
}


gboolean on_layer_menu_label_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    g_print("open or toggle\n");
    return TRUE;  // stop further propagation
}

// --------------------
// CREATE LAYER COMPONENT
// --------------------
GtkWidget* create_layer_component(const char *label_text, int layer_number) {

	// ====== OVERLAY WRAPPER ======
	GtkWidget *overlay = gtk_overlay_new();
	gtk_widget_set_vexpand(overlay, FALSE);          // prevent overlay from stretching
	gtk_widget_set_valign(overlay, GTK_ALIGN_START); // align top in parent

	// Outer vertical box for the layer
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name(vbox, "layer-vbox");
	gtk_container_add(GTK_CONTAINER(overlay), vbox);
	gtk_widget_set_valign(vbox, GTK_ALIGN_START); // align top inside overlay

        // Layer horizontal container (bordered box)
        GtkWidget *layer_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_size_request(layer_hbox, LAYER_COMPONENT_WIDTH, -1);
        gtk_widget_set_name(layer_hbox, "layer-box");
        gtk_box_pack_start(GTK_BOX(vbox), layer_hbox, FALSE, FALSE, 0);

        // Left vertical stack: label + buttons
        GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_size_request(left_vbox, -1, LAYER_COMPONENT_HEIGHT); // width for buttons stack
        gtk_box_pack_start(GTK_BOX(layer_hbox), left_vbox, FALSE, FALSE, 6);

        // Label
        GtkWidget *header_label = gtk_label_new(label_text);
        gtk_widget_set_name(header_label, "layer-header");
        gtk_widget_set_halign(header_label, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(left_vbox), header_label, FALSE, FALSE, 0);
        
	// Load button
	GtkWidget *btn_load = gtk_button_new_with_label("LOAD");
	gtk_widget_set_name(btn_load, "btn-load");
	gtk_box_pack_start(GTK_BOX(left_vbox), btn_load, FALSE, FALSE, 0);
	// Connect callback

	// Effects button
	GtkWidget *btn_fx = gtk_button_new_with_label("EFFECTS");
	gtk_widget_set_name(btn_fx, "btn-fx");
	gtk_box_pack_start(GTK_BOX(left_vbox), btn_fx, FALSE, FALSE, 0);

	// Preview box on the right
	GtkWidget *preview_box = gtk_event_box_new();
	gtk_widget_set_name(preview_box, "preview-box");
	gtk_widget_set_size_request(preview_box, 200, -1); 
	gtk_box_pack_start(GTK_BOX(layer_hbox), preview_box, FALSE, FALSE, 6);
	
	/* STORE IT */
	if (layer_number >= 0 && layer_number <= MAX_LAYERS) {
	    layer_preview_boxes[layer_number] = preview_box;
	}

	GtkWidget *preview_label = gtk_label_new("(EMPTY)");
	gtk_widget_set_name(preview_label, "preview-label");
	gtk_widget_set_halign(preview_label, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(preview_label, GTK_ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(preview_box), preview_label);
	
	// ===== SMALL ABSOLUTE MENU LABEL ===== //
	GtkWidget *menu_label = gtk_label_new("⋮");  // or "X" if you prefer
	gtk_widget_set_name(menu_label, "layer-menu-btn");
	gtk_label_set_xalign(GTK_LABEL(menu_label), 0.5);
	gtk_label_set_yalign(GTK_LABEL(menu_label), 0.5);

	// Wrap in event box so it can receive clicks
	GtkWidget *menu_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(menu_box), menu_label);
	gtk_widget_set_size_request(menu_box, 16, 16);
	gtk_widget_set_halign(menu_box, GTK_ALIGN_END);   // top-right
	gtk_widget_set_valign(menu_box, GTK_ALIGN_START);

	// Add the box to the overlay
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), menu_box);
	
	g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_button_clicked), GINT_TO_POINTER(layer_number));
	g_signal_connect(menu_box, "button-press-event", G_CALLBACK(on_layer_menu_label_click), NULL);
        g_signal_connect(btn_fx,"clicked",G_CALLBACK(on_fx_button_clicked),GINT_TO_POINTER(layer_number));


        return overlay;
}

static void on_drawarea_map(GtkWidget *widget, gpointer data)
{
    sdl_embed_in_gtk(widget);
}

#include <cairo.h>
#include <gtk/gtk.h>

static gboolean on_render_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    // Clear background
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1); // dark gray
    cairo_paint(cr);

    // Example: draw a moving rectangle
    static int x = 0;
    cairo_set_source_rgb(cr, 1.0, 0.8, 0.0); // yellow
    cairo_rectangle(cr, x, 40, 320, 120);
    cairo_fill(cr);

    x += 5; // move rectangle
    if (x > gtk_widget_get_allocated_width(widget))
        x = -320;

    return FALSE; // drawing done
}


static gboolean refresh_render_panel(gpointer user_data)
{
    GtkWidget *render_panel = GTK_WIDGET(user_data);

    // Queue redraw
    gtk_widget_queue_draw(render_panel);

    // Return TRUE to keep calling this function
    return TRUE;
}

static gboolean sdl_refresh_loop(gpointer data)
{
    GtkWidget *render_panel = GTK_WIDGET(data);

    // Call your SDL rendering function (draw current frame)
    sdl_draw_frame();   // <-- create this in your sdl.c

    // Force SDL to update GTK XID surface if needed
    SDL_RenderPresent(sdl_get_renderer()); // <-- getter for renderer

    return TRUE; // keep running
}

// Callback for Play button
static void on_play_clicked(GtkWidget *widget, gpointer user_data) {
    if (!sdl_is_playing())
        sdl_set_playing(1);
}

static void on_pause_clicked(GtkWidget *widget, gpointer user_data) {
    if (sdl_is_playing())
        sdl_set_playing(0);
}

static void on_app_destroy(GtkWidget *widget, gpointer data)
{
    sdl_set_playing(0);

    cleanup_frames_folders();

    gtk_main_quit();
}


// --------------------
// MAIN
// --------------------
int main(int argc, char *argv[]) {

        gtk_init(&argc, &argv);
        GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(win), "Pulsrr Sequencer");
        gtk_window_set_default_size(GTK_WINDOW(win), MINIMAL_WINDOW_WIDTH, MINIMAL_WINDOW_HEIGHT);
        gtk_window_set_resizable(GTK_WINDOW(win), TRUE);
        g_signal_connect(win, "destroy", G_CALLBACK(on_app_destroy), NULL);

        // Main horizontal container
        GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_name(main_hbox, "main-container");
        
	// ===== GLOBAL OVERLAY =====
	GtkWidget *overlay = gtk_overlay_new();
	gtk_container_add(GTK_CONTAINER(win), overlay);

	// Add your existing main UI
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), main_hbox);

	// ===== MODAL LAYER (hidden by default) =====
	GtkWidget *modal_layer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name(modal_layer, "modal-layer");
	gtk_widget_set_halign(modal_layer, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(modal_layer, GTK_ALIGN_CENTER);
	gtk_widget_hide(modal_layer);

	// Important: let it receive clicks
	gtk_widget_set_hexpand(modal_layer, TRUE);
	gtk_widget_set_vexpand(modal_layer, TRUE);

	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), modal_layer);

	// Store globally
	global_modal_layer = modal_layer;

        // --------------------
        // CONTROL PANEL (LEFT CONTAINER)
        // --------------------
        
        GtkWidget *left_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
        gtk_widget_set_name(left_container, "left-container");
        gtk_widget_set_size_request(left_container, LEFT_CONTAINER_WIDTH,-1);
	gtk_widget_set_hexpand(left_container, FALSE);
	gtk_widget_set_halign(left_container, GTK_ALIGN_START);
	gtk_widget_set_vexpand(left_container, TRUE);
        gtk_box_pack_start(GTK_BOX(main_hbox), left_container, FALSE, FALSE, 0);
        
        // Logo

        GtkWidget *logo = gtk_image_new_from_file("logo.png");
        gtk_widget_set_size_request(logo, LEFT_CONTAINER_WIDTH, LOGO_HEIGHT);
        gtk_box_pack_start(GTK_BOX(left_container), logo, FALSE, FALSE, 0);
        
        // APHORISM SNIPPET
	char *aphorism = NULL;
	AphorismErrorCode err = get_random_aphorism("aphorisms.txt", &aphorism);

	// Create container
	GtkWidget *aphorism_box = gtk_event_box_new();
	gtk_widget_set_hexpand(aphorism_box, FALSE);
	gtk_widget_set_vexpand(aphorism_box, FALSE);
	gtk_widget_set_halign(aphorism_box, GTK_ALIGN_CENTER);

	// Width limit and CSS styling
	gtk_widget_set_size_request(aphorism_box, 200, -1);
	gtk_widget_set_name(aphorism_box, "aphorism-box");

	// Create label
	GtkWidget *aphorism_label;
	if (err == APHORISM_OK && aphorism) {
	    aphorism_label = gtk_label_new(aphorism);
	    g_free(aphorism);
	} else {
	    g_warning("Could not get aphorism (error code: %d)", err);
	    aphorism_label = gtk_label_new("Loop it till it hurts.");
	}


	// Center text horizontally
	gtk_label_set_xalign(GTK_LABEL(aphorism_label), 0.5);
	gtk_label_set_justify(GTK_LABEL(aphorism_label), GTK_JUSTIFY_CENTER);
	gtk_label_set_line_wrap(GTK_LABEL(aphorism_label), TRUE);
	gtk_label_set_line_wrap_mode(GTK_LABEL(aphorism_label), PANGO_WRAP_WORD_CHAR);

	// VERY important: allow narrow wrapping
	gtk_label_set_width_chars(GTK_LABEL(aphorism_label), 25);
	gtk_label_set_max_width_chars(GTK_LABEL(aphorism_label), 25);

	// Add label to box
	gtk_container_add(GTK_CONTAINER(aphorism_box), aphorism_label);

	// Pack the box into the control panel
	gtk_box_pack_start(GTK_BOX(left_container), aphorism_box, FALSE, FALSE, 0);

        GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
        gtk_widget_set_name(button_row, "button-row");
        gtk_widget_set_size_request(button_row, LEFT_CONTAINER_WIDTH, BUTTON_ROW_HEIGHT);
        gtk_box_pack_start(GTK_BOX(left_container), button_row, FALSE, FALSE, 0);

        GtkWidget *btn_create = gtk_button_new_with_label("ADD SEQ.");
        gtk_widget_set_name(btn_create, "btn-add-seq");
        GtkWidget *btn_update = gtk_button_new_with_label("UPD. RENDER");
        gtk_widget_set_name(btn_update, "btn-update-render");
        gtk_widget_set_size_request(btn_create, 150, 42);
	gtk_widget_set_size_request(btn_update, 150, 42);
        gtk_box_pack_start(GTK_BOX(button_row), btn_create, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(button_row), btn_update, TRUE, TRUE, 0);
        
	
	// Add layer components
        gtk_box_pack_start(GTK_BOX(left_container), create_layer_component("LAYER 1",1), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(left_container), create_layer_component("LAYER 2",2), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(left_container), create_layer_component("LAYER 3",3), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(left_container), create_layer_component("LAYER 4",4), FALSE, FALSE, 0);
        
	// Version label
	GtkWidget *version_label = gtk_label_new("Version 0.0.1 by Marin Durand");
	gtk_widget_set_name(version_label, "version-label");
	gtk_widget_set_halign(version_label, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(version_label, GTK_ALIGN_END);
	//gtk_widget_set_margin_top(version_label, 10);
	gtk_box_pack_end(GTK_BOX(left_container), version_label, FALSE, FALSE, 0);

        // --------------------
        // RENDER + TIMELINE PANEL (RIGHT CONTAINER)
        // --------------------
	GtkWidget *right_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name(right_container, "right-container");
	gtk_widget_set_size_request(right_container, -1, -1); // Let GTK decide size
	gtk_widget_set_vexpand(right_container, TRUE);
	gtk_widget_set_hexpand(right_container, TRUE);
	gtk_box_pack_start(GTK_BOX(main_hbox), right_container, TRUE, TRUE, 0);


	GtkWidget *render_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name(render_frame, "render-panel");
	gtk_widget_set_size_request(render_frame, -1, -1);
	gtk_widget_set_vexpand(render_frame, TRUE);
	gtk_widget_set_hexpand(render_frame, TRUE);
	gtk_box_pack_start(GTK_BOX(right_container), render_frame, TRUE, TRUE, 0);

	GtkWidget *render_panel = gtk_drawing_area_new();
	gtk_widget_set_vexpand(render_panel, TRUE);
	gtk_widget_set_hexpand(render_panel, TRUE);
	gtk_box_pack_start(GTK_BOX(render_frame), render_panel, TRUE, TRUE, 0);

	// --- Render control box ---
	GtkWidget *render_control = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
	gtk_widget_set_halign(render_control, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(render_control, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(render_frame), render_control, FALSE, FALSE, 5);

	// --- Play button ---
	GtkWidget *btn_play = gtk_button_new();
	gtk_widget_set_name(btn_play, "btn-play");
	GtkWidget *img_play = gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(btn_play), img_play);
	gtk_box_pack_start(GTK_BOX(render_control), btn_play, FALSE, FALSE, 0);

	// --- Pause button ---
	GtkWidget *btn_pause = gtk_button_new();
	gtk_widget_set_name(btn_pause, "btn-pause");
	GtkWidget *img_pause = gtk_image_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(btn_pause), img_pause);
	gtk_box_pack_start(GTK_BOX(render_control), btn_pause, FALSE, FALSE, 0);

	
    	g_signal_connect(render_panel, "map",G_CALLBACK(on_drawarea_map), NULL);
	g_signal_connect(btn_play, "clicked", G_CALLBACK(on_play_clicked), render_panel);
	g_signal_connect(btn_pause, "clicked", G_CALLBACK(on_pause_clicked), render_panel);
    	
	// Embed SDL in GTK
	sdl_embed_in_gtk(render_panel);

	// Start timer for ~30 FPS
	g_timeout_add(33, sdl_refresh_loop, render_panel);




        // Timeline panel
	GtkWidget *timeline_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name(timeline_panel, "timeline-panel");
	gtk_widget_set_size_request(timeline_panel, -1, TIMELINE_PANEL_HEIGHT);
	gtk_widget_set_vexpand(timeline_panel, FALSE);
	gtk_widget_set_hexpand(timeline_panel, TRUE);
	gtk_box_pack_start(GTK_BOX(right_container), timeline_panel, FALSE, FALSE, 0);
	
	
	g_signal_connect(btn_update, "clicked", G_CALLBACK(on_update_render_clicked), render_panel);

    

        // --------------------
        // LOAD CSS
        // --------------------
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

