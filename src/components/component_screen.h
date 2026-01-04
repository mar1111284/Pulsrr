#ifndef COMPONENT_SCREEN_H
#define COMPONENT_SCREEN_H

#include <gtk/gtk.h>

// Forward declaration for SDL functions
void on_drawarea_map(GtkWidget *widget, gpointer data);
void ui_reconcile_mode(void);
gboolean sdl_refresh_loop(gpointer data);
void update_mode_buttons_active_state(void);
// Forward declaration for GTK callbacks used internally
G_MODULE_EXPORT GtkWidget* create_render_screen(GtkWidget **out_render_panel);

#endif // SCREEN_PANEL_H

