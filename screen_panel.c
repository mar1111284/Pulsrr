#include "utils.h"
#include "sdl.h"
#include "screen_panel.h"
#include "sdl_utilities.h"

void on_drawarea_map(GtkWidget *widget, gpointer data)
{
    sdl_embed_in_gtk(widget);
}

gboolean sdl_refresh_loop(gpointer data)
{
    //GtkWidget *render_panel = GTK_WIDGET(data);

    // Call your SDL rendering function (draw current frame)
    sdl_draw_frame();   // <-- create this in your sdl.c

    // Force SDL to update GTK XID surface if needed
    SDL_RenderPresent(sdl_get_renderer()); // <-- getter for renderer

    return TRUE; // keep running
}

GtkWidget* create_render_screen(GtkWidget **out_render_panel)
{
    GtkWidget *render_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(render_frame, "render-panel");
    gtk_widget_set_hexpand(render_frame, TRUE);
    gtk_widget_set_vexpand(render_frame, TRUE);

    // ---- SDL Drawing Area ----
    GtkWidget *render_panel = gtk_drawing_area_new();
    gtk_widget_set_hexpand(render_panel, TRUE);
    gtk_widget_set_vexpand(render_panel, TRUE);
    gtk_box_pack_start(GTK_BOX(render_frame), render_panel, TRUE, TRUE, 0);

    // ---- Controls ----
    GtkWidget *render_control = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(render_control, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(render_frame), render_control, FALSE, FALSE, 5);

    GtkWidget *btn_play = gtk_button_new_from_icon_name(
        "media-playback-start", GTK_ICON_SIZE_BUTTON);
    GtkWidget *btn_pause = gtk_button_new_from_icon_name(
        "media-playback-pause", GTK_ICON_SIZE_BUTTON);

    gtk_widget_set_name(btn_play, "btn-play");
    gtk_widget_set_name(btn_pause, "btn-pause");

    gtk_box_pack_start(GTK_BOX(render_control), btn_play, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(render_control), btn_pause, FALSE, FALSE, 0);

    // ---- Signals ----
    g_signal_connect(render_panel, "map",
                     G_CALLBACK(on_drawarea_map), NULL);
    g_signal_connect(btn_play, "clicked",
                     G_CALLBACK(on_play_clicked), NULL);
    g_signal_connect(btn_pause, "clicked",
                     G_CALLBACK(on_pause_clicked), NULL);

    // ---- SDL ----
    sdl_embed_in_gtk(render_panel);
    g_timeout_add(33, sdl_refresh_loop, NULL);

    if (out_render_panel)
        *out_render_panel = render_panel;

    return render_frame;
}

// Callback for Play button
void on_play_clicked(GtkWidget *widget, gpointer user_data) {
    if (!sdl_is_playing())
        sdl_set_playing(1);
}

void on_pause_clicked(GtkWidget *widget, gpointer user_data) {
    if (sdl_is_playing())
        sdl_set_playing(0);
}
