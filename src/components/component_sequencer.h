#ifndef COMPONENT_SEQUENCER_H
#define COMPONENT_SEQUENCER_H

#include <gtk/gtk.h>

// Sequencer component
GtkWidget *create_sequencer_component(void);

// Loop bars
extern GtkWidget *loop_start_box;
extern GtkWidget *loop_end_box;
extern int left_bar_x;
extern int right_bar_x;
extern int sequences_overlay_width;

// Callbacks
void on_overlay_size_allocate(GtkWidget *widget,GtkAllocation *allocation,gpointer user_data);
gboolean on_bar_clicked(GtkWidget *widget,GdkEventButton *event,gpointer user_data);
void update_sequencer(void);

// Sequence widget helpers
GtkWidget* create_sequence_widget_css(const char *sequence_folder);

int get_sequence_widget_width(GtkWidget *sequences_box, int num_sequences);
int get_sequence_widget_height(GtkWidget *sequences_box);

// Utilities
int get_number_of_sequences(void);
void on_clear_all_clicked(GtkButton *button, gpointer user_data);
gboolean on_sequence_delete_click(GtkWidget *widget,GdkEventButton *event,gpointer user_data);
gboolean delete_dir_recursive(const char *path);

#endif // SEQUENCER_H

