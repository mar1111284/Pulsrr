#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <gtk/gtk.h> 

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

void on_drag_data_received(GtkWidget *widget,
                                  GdkDragContext *context,
                                  gint x, gint y,
                                  GtkSelectionData *data,
                                  guint info, guint time,
                                  gpointer user_data);


#endif // UTILS_H

