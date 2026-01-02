#ifndef SCREEN_PANEL_H
#define SCREEN_PANEL_H

#include <gtk/gtk.h>

// Forward declaration for SDL functions
void on_drawarea_map(GtkWidget *widget, gpointer data);
void on_play_clicked(GtkWidget *widget, gpointer user_data);
void on_pause_clicked(GtkWidget *widget, gpointer user_data);
void ui_reconcile_mode(void);
gboolean sdl_refresh_loop(gpointer data);

// Forward declaration for GTK callbacks used internally
G_MODULE_EXPORT GtkWidget* create_render_screen(GtkWidget **out_render_panel);

#endif // SCREEN_PANEL_H

