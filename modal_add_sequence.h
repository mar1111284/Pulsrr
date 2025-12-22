#ifndef MODAL_ADD_SEQUENCE_H
#define MODAL_ADD_SEQUENCE_H

#include <gtk/gtk.h>
#include <glib.h>
#include <stdint.h>

typedef struct {
	GtkWidget *root_container;  
    GtkSpinButton *duration_spin;
    GtkComboBoxText *scale_combo;
    GtkWidget *log_view;
    GtkTextBuffer *log_buffer;
    GtkWidget *progress_bar;
    GtkWidget *parent_container;
} AddSequenceUI;


// Callbacks
void on_add_button_clicked(GtkButton *button, gpointer user_data);
void on_add_sequence_clicked(GtkButton *button, gpointer user_data);
void create_fx_file(const char *path, AddSequenceUI *ui);
void add_log(AddSequenceUI *ui, const char *message);
void set_progress_add_sequence(AddSequenceUI *ui, double fraction, const char *text);

#endif // MODAL_ADD_SEQUENCE_H

