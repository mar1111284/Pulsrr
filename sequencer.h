#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <gtk/gtk.h>

/* =====================================================
 * Sequencer component
 * ===================================================== */

/* Create the full sequencer UI (controls + timeline) */
GtkWidget *create_sequencer_component(void);

/* =====================================================
 * Loop bars (used for dragging / overlay)
 * ===================================================== */
extern GtkWidget *loop_start_box;
extern GtkWidget *loop_end_box;
extern int left_bar_x;
extern int right_bar_x;
extern int sequences_overlay_width;


/* =====================================================
 * Callbacks
 * ===================================================== */

/* Resize handler for sequences overlay */
void on_overlay_size_allocate(GtkWidget *widget,
                             GtkAllocation *allocation,
                             gpointer user_data);

/* Click handler for loop bars */
gboolean on_bar_clicked(GtkWidget *widget,
                        GdkEventButton *event,
                        gpointer user_data);

/* =====================================================
 * Sequence widget helpers
 * ===================================================== */

/* Create a CSS-styled sequence preview widget from a folder */
GtkWidget* create_sequence_widget_css(const char *sequence_folder);

/* Get sequence widget dimensions */
int get_sequence_widget_width(GtkWidget *sequences_box, int num_sequences);
int get_sequence_widget_height(GtkWidget *sequences_box);

/* =====================================================
 * Utilities
 * ===================================================== */

/* Return the number of sequences currently loaded */
int get_number_of_sequences(void);

#endif /* SEQUENCER_H */

