#ifndef MODAL_DOWNLOAD_H
#define MODAL_DOWNLOAD_H

#include <gtk/gtk.h>
#include "../sdl/sdl.h" 
#include "../utils/utils.h"
#include "../components/component_sequencer.h"
#include "modal_add_sequence.h"

typedef struct {
    AddSequenceUI *ui;
    GList *sequence_paths; // list of folders: sequences/sequence_X/mixed_frames
    int fps;
    const char *scale; // e.g., "1080p", "720p"
} DownloadJob;

// Functions
void on_download_button_clicked(GtkButton *button, gpointer user_data);
void on_dl_sequence_clicked(GtkButton *button, gpointer user_data);
void on_modal_dl_back_clicked(GtkButton *button, gpointer user_data);

// Getter
int get_total_sequences(void);

#endif // MODAL_DOWNLOAD_H

