#include "modal_add_sequence.h"
#include "utils.h"
#include "sdl_utilities.h"

void on_add_button_clicked(GtkButton *button, gpointer user_data) {

	sdl_set_playing(0);

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

	// Button row (Back + Add)
	GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(button_row, "button-row");
	gtk_widget_set_size_request(button_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), button_row, FALSE, FALSE, 0);

	// Back button
	GtkWidget *btn_back = gtk_button_new_with_label("BACK");
	gtk_widget_set_name(btn_back, "modal-cancel-button");
	gtk_widget_set_size_request(btn_back, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_back, TRUE, TRUE, 0);

	// Add button
	GtkWidget *btn_add_sequence = gtk_button_new_with_label("EXPORT");
	gtk_widget_set_name(btn_add_sequence, "modal-export-button");
	gtk_widget_set_size_request(btn_add_sequence, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_add_sequence, TRUE, TRUE, 0);

	// Controls row: Duration + Scale
	GtkWidget *controls_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(controls_row, "controls-row");
	gtk_widget_set_size_request(controls_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), controls_row, FALSE, FALSE, 5);

	// Duration spin button
	GtkWidget *duration_spin = gtk_spin_button_new_with_range(1, 60, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(duration_spin), 10);
	gtk_widget_set_name(controls_row, "spin-button");
	gtk_widget_set_size_request(duration_spin, 150, 42);
	gtk_box_pack_start(GTK_BOX(controls_row), duration_spin, TRUE, TRUE, 0);

	// Scale combo box
	GtkWidget *scale_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "1080p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "720p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "480p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "360p");
	gtk_combo_box_set_active(GTK_COMBO_BOX(scale_combo), 2);
	gtk_widget_set_size_request(scale_combo, 150, 42);
	gtk_box_pack_start(GTK_BOX(controls_row), scale_combo, TRUE, TRUE, 0);
	
	// Labels row: "Duration" and "Quality"
	GtkWidget *labels_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_size_request(labels_row, 400, 20);

	// FPS & Quality label
	GtkWidget *duration_label = gtk_label_new("Duration");
	gtk_widget_set_halign(duration_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(labels_row), duration_label, TRUE, TRUE, 0);
	GtkWidget *quality_label = gtk_label_new("Quality");
	gtk_widget_set_halign(quality_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(labels_row), quality_label, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(black_box), labels_row, FALSE, FALSE, 0);
	
	// Adding progress bar
	GtkWidget *progress_bar = gtk_progress_bar_new();
	gtk_widget_set_name(progress_bar, "export-progress");
	gtk_widget_set_size_request(progress_bar, 360, 20);
	
	// Initial state of progress bar
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "0%");
	gtk_box_pack_end(GTK_BOX(black_box), progress_bar, FALSE, FALSE, 10);
	

	g_signal_connect(btn_back, "clicked", G_CALLBACK(on_modal_back_clicked), global_modal_layer);
	g_signal_connect(btn_add_sequence, "clicked", G_CALLBACK(on_add_sequence_clicked), NULL);

	gtk_container_add(GTK_CONTAINER(global_modal_layer), black_box);
	gtk_widget_show_all(global_modal_layer);
}

void on_add_sequence_clicked(GtkButton *button, gpointer user_data) {
    g_print("on add sequence start now");
}
