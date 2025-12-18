#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <gtk/gtk.h> 

#define MINIMAL_WINDOW_WIDTH   1200
#define MINIMAL_WINDOW_HEIGHT  900

#define LEFT_CONTAINER_WIDTH     200
#define RIGHT_CONTAINER_WIDTH    1000

#define LOGO_HEIGHT    42
#define HEADER_HEIGHT  18
#define BUTTON_ROW_HEIGHT 30

#define LAYER_COMPONENT_WIDTH 200
#define LAYER_COMPONENT_HEIGHT   42

#define PREVIEW_WIDTH  100
#define PREVIEW_HEIGHT 42

#define MAX_LAYERS 4

#define TIMELINE_PANEL_HEIGHT 200

#define LOOP_BAR_WIDTH 6

typedef enum { BAR_NONE, BAR_START, BAR_END } SelectedBar;

extern SelectedBar selected_bar;  // <- just declare, don't define


typedef enum {
    APHORISM_OK = 0,
    APHORISM_FILE_NOT_FOUND,
    APHORISM_EMPTY_FILE,
    APHORISM_READ_FAILED,
    APHORISM_MEMORY_ERROR
} AphorismErrorCode;

typedef enum {
    DAD_OK = 0,
    DAD_NULL_POINTER,
    DAD_NO_DATA,
    DAD_INVALID_URI,
    DAD_NO_VALID_FILES
} DragErrorCode;

gboolean is_frames_file_empty(int layer_number);
/**
 * Get a random aphorism from a text file.
 * 
 * @param filename Path to the aphorism file.
 * @param out_str Pointer to char* where the result will be stored.
 *                The caller must free it with g_free().
 * @return ErrorCode indicating success or type of error.
 */
AphorismErrorCode get_random_aphorism(const char *filename, char **out_str);

/**
 * Check if a filename has the ".mp4" extension (case-insensitive).
 *
 * @param name The filename to check.
 * @return TRUE if the filename ends with ".mp4" (any case), FALSE otherwise.
 */
static inline gboolean is_mp4_file(const char *name) {
    gchar *lower = g_utf8_strdown(name, -1);
    gboolean result = g_str_has_suffix(lower, ".mp4");
    g_free(lower);
    return result;
}

void on_drag_data_received(GtkWidget *widget,GdkDragContext *context,gint x, gint y,GtkSelectionData *data,guint info, guint time,gpointer user_data);

void cleanup_frames_folders(void);
int get_number_of_sequences(void);

#endif // UTILS_H

