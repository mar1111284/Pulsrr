#include "../utils/utils.h"
#include "../sdl/sdl.h"
#include "component_screen.h"


static GtkWidget *global_render_controls = NULL;
extern SDL g_sdl;

// once mapped embed SDL
void on_drawarea_map(GtkWidget *widget, gpointer data)
{
    sdl_embed_in_gtk(widget);
    gtk_widget_queue_resize(widget);
}

// Callback: LIVE mode button
static void on_live_mode_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    g_sdl.screen_mode = LIVE_MODE;
    add_main_log("[INFO] Switch to Live mixing mode.");

}

// Callback: PLAYBACK mode button
static void on_playback_mode_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    g_sdl.screen_mode = PLAYBACK_MODE;
    add_main_log("[INFO] Switch to Playback sequence mode.");
}

// Create render screen
GtkWidget* create_render_screen(GtkWidget **out_render_panel)
{
    // Render frame container
    GtkWidget *render_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(render_frame, "render-panel");
    gtk_widget_set_hexpand(render_frame, TRUE);
    gtk_widget_set_vexpand(render_frame, TRUE);

    // SDL drawing area
    GtkWidget *render_panel = gtk_drawing_area_new();
    gtk_widget_set_hexpand(render_panel, TRUE);
    gtk_widget_set_vexpand(render_panel, TRUE);
    gtk_box_pack_start(GTK_BOX(render_frame), render_panel, TRUE, TRUE, 0);

    // Control bar for mode buttons
    GtkWidget *render_control = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(render_control, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(render_frame), render_control, FALSE, FALSE, 5);
    global_render_controls = render_control;

    // Add LIVE / PLAYBACK buttons
    GtkWidget *btn_live = gtk_button_new_with_label("LIVE");
    GtkWidget *btn_playback = gtk_button_new_with_label("PLAYBACK");

    gtk_box_pack_start(GTK_BOX(render_control), btn_live, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(render_control), btn_playback, FALSE, FALSE, 0);

    g_signal_connect(btn_live, "clicked",G_CALLBACK(on_live_mode_clicked), NULL);
    g_signal_connect(btn_playback, "clicked",G_CALLBACK(on_playback_mode_clicked), NULL);

    // Ensure drawing area is realized
    gtk_widget_show(render_panel);

    // Connect map signal to embed SDL
    g_signal_connect(render_panel, "map", G_CALLBACK(on_drawarea_map), NULL);

    if (out_render_panel)
        *out_render_panel = render_panel;

    return render_frame;
}

