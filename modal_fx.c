#include "modal_fx.h"
#include "layer.h"

void on_fx_apply_clicked(GtkButton *button, gpointer user_data) {

	LayerFxWidgets *fx = (LayerFxWidgets *)user_data;

	guint8 layer_index = fx->layer_index;
	double speed = gtk_spin_button_get_value(fx->speed_spin);
	int alpha = gtk_spin_button_get_value_as_int(fx->alpha_spin);
	int threshold = gtk_spin_button_get_value_as_int(fx->threshold_spin);
	double contrast = gtk_spin_button_get_value(fx->contrast_spin);
	gboolean grayscale = gtk_toggle_button_get_active(fx->gray_check);
	gboolean invert = gtk_toggle_button_get_active(fx->invert_check);

	g_print("APPLY FX for layer %d:\n", layer_index +1);
	g_print("  Speed: %.2f\n", speed);
	g_print("  Alpha: %d\n", alpha);
	g_print("  Threshold: %d\n", threshold);
	g_print("  Contrast: %.2f\n", contrast);
	g_print("  Grayscale: %s\n", grayscale ? "ON" : "OFF");
	g_print("  Invert: %s\n", invert ? "ON" : "OFF");

	// Apply Filter
	set_transparency(layer_index, alpha);
	set_gray(layer_index, grayscale ? 1 : 0);
    set_layer_speed(layer_index, speed);
}

void on_fx_button_clicked(GtkButton *button, gpointer user_data) {

	sdl_set_playing(0);

	guint8 layer_index = GPOINTER_TO_INT(user_data);

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

	// Modal title header
	GtkWidget *title = gtk_label_new("FX");
	gtk_widget_set_name(title, "modal-title");
	gtk_box_pack_start(GTK_BOX(black_box), title, FALSE, FALSE, 10);

	// FX Row 1: Speed + Alpha (50% each)
	GtkWidget *fx_row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_hexpand(fx_row1, TRUE);

	// Speed input
	GtkWidget *speed_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(speed_box, TRUE);
	GtkWidget *speed_label = gtk_label_new("Speed");
	GtkWidget *speed_spin = gtk_spin_button_new_with_range(0.5, 8.0, 0.5);

	// Get actual speed from SDL
	double current_speed = get_layer_speed(layer_index);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(speed_spin), current_speed);

	gtk_box_pack_start(GTK_BOX(speed_box), speed_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(speed_box), speed_spin, TRUE, TRUE, 0);

	// Alpha input
	GtkWidget *alpha_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(alpha_box, TRUE);
	GtkWidget *alpha_label = gtk_label_new("Alpha");
	GtkWidget *alpha_spin = gtk_spin_button_new_with_range(0, 255, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(alpha_spin), 255);
	int current_alpha = get_transparency(layer_index);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(alpha_spin), current_alpha);
	gtk_box_pack_start(GTK_BOX(alpha_box), alpha_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(alpha_box), alpha_spin, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(fx_row1), speed_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(fx_row1), alpha_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(black_box), fx_row1, FALSE, FALSE, 5);

	// FX Row 2: Threshold + Contrast
	GtkWidget *fx_row2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_hexpand(fx_row2, TRUE);

	// Threshold input
	GtkWidget *threshold_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(threshold_box, TRUE);
	GtkWidget *threshold_label = gtk_label_new("Threshold");
	GtkWidget *threshold_spin = gtk_spin_button_new_with_range(0, 255, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold_spin), 128);
	gtk_box_pack_start(GTK_BOX(threshold_box), threshold_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(threshold_box), threshold_spin, TRUE, TRUE, 0);

	// Contrast input
	GtkWidget *contrast_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(contrast_box, TRUE);
	GtkWidget *contrast_label = gtk_label_new("Contrast");
	GtkWidget *contrast_spin = gtk_spin_button_new_with_range(0.2, 3.0, 0.05);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(contrast_spin), 1.0);
	gtk_box_pack_start(GTK_BOX(contrast_box), contrast_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(contrast_box), contrast_spin, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(fx_row2), threshold_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(fx_row2), contrast_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(black_box), fx_row2, FALSE, FALSE, 5);

	// FX Row 3: Grayscale + Invert
	GtkWidget *fx_row3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_hexpand(fx_row3, TRUE);

	// Grayscale
	GtkWidget *gray_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(gray_box, TRUE);
	GtkWidget *gray_label = gtk_label_new("Gray");
	GtkWidget *gray_check = gtk_check_button_new();

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gray_check), is_layer_gray(layer_index));
	gtk_box_pack_start(GTK_BOX(gray_box), gray_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(gray_box), gray_check, TRUE, TRUE, 0);

	// Invert
	GtkWidget *invert_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_hexpand(invert_box, TRUE);
	GtkWidget *invert_label = gtk_label_new("Invert");
	GtkWidget *invert_check = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invert_check), FALSE);
	gtk_box_pack_start(GTK_BOX(invert_box), invert_label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(invert_box), invert_check, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(fx_row3), gray_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(fx_row3), invert_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(black_box), fx_row3, FALSE, FALSE, 5);
	
	// Example: set min width for labels in a row
	gtk_widget_set_size_request(speed_label, 80, -1);
	gtk_widget_set_size_request(alpha_label, 80, -1);
	gtk_widget_set_size_request(threshold_label, 80, -1);
	gtk_widget_set_size_request(contrast_label, 80, -1);
	gtk_widget_set_size_request(gray_label, 80, -1);
	gtk_widget_set_size_request(invert_label, 80, -1);

	// Buttons
	GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_size_request(button_row, 400, 42);

	GtkWidget *btn_back = gtk_button_new_with_label("BACK");
	GtkWidget *btn_apply = gtk_button_new_with_label("APPLY FX");

	gtk_widget_set_size_request(btn_back, 150, 42);
	gtk_widget_set_size_request(btn_apply, 150, 42);

	gtk_box_pack_start(GTK_BOX(button_row), btn_back, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(button_row), btn_apply, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(black_box), button_row, FALSE, FALSE, 10);

	LayerFxWidgets *fx_widgets = g_malloc(sizeof(LayerFxWidgets));
	fx_widgets->layer_index = layer_index;
	fx_widgets->speed_spin = GTK_SPIN_BUTTON(speed_spin);
	fx_widgets->alpha_spin = GTK_SPIN_BUTTON(alpha_spin);
	fx_widgets->threshold_spin = GTK_SPIN_BUTTON(threshold_spin);
	fx_widgets->contrast_spin = GTK_SPIN_BUTTON(contrast_spin);
	fx_widgets->gray_check = GTK_TOGGLE_BUTTON(gray_check);
	fx_widgets->invert_check = GTK_TOGGLE_BUTTON(invert_check);

	g_signal_connect(btn_apply, "clicked", G_CALLBACK(on_fx_apply_clicked), fx_widgets);
	g_signal_connect(btn_back, "clicked", G_CALLBACK(on_modal_back_clicked), global_modal_layer);

	gtk_container_add(GTK_CONTAINER(global_modal_layer), black_box);
	gtk_widget_show_all(global_modal_layer);
}
