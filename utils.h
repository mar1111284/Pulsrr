#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <gtk/gtk.h> 


// Constants / Layout Settings
#define LOGO_WIDTH               185
#define LOGO_HEIGHT               42
#define HEADER_HEIGHT             18

#define MINIMAL_WINDOW_WIDTH    1250
#define MINIMAL_WINDOW_HEIGHT    925

#define LEFT_CONTAINER_WIDTH     250
#define RIGHT_CONTAINER_WIDTH    950

#define BUTTON_TOP_CONTROL_WIDTH  90
#define BUTTON_TOP_CONTROL_HEIGHT 40
#define BUTTON_ROW_HEIGHT         30

#define LAYER_COMPONENT_WIDTH    200
#define LAYER_COMPONENT_HEIGHT    42

#define PREVIEW_WIDTH            100
#define PREVIEW_HEIGHT            42

#define MAX_LAYERS                 4
#define TIMELINE_PANEL_HEIGHT    200
#define LOOP_BAR_WIDTH             6

// Global Types
typedef struct {
    GtkTextView *log_view;
    GtkTextBuffer *log_buffer;
    GtkWidget *main_container;
} MainUI;

typedef enum { 
    BAR_NONE, 
    BAR_START, 
    BAR_END 
} SelectedBar;

typedef enum {
    DAD_OK = 0,
    DAD_NULL_POINTER,
    DAD_NO_DATA,
    DAD_INVALID_URI,
    DAD_NO_VALID_FILES
} DragErrorCode;

// Passing message log safely
typedef struct {
    char *msg;
} LogIdleData;

// Extern Globals
extern MainUI g_main_ui;
extern SelectedBar selected_bar;
extern GtkWidget *global_modal_layer;

// Utility Functions
void add_main_log(const char *message);

// Getter
MainUI* main_ui_get(void);

// Frame / File management
gboolean is_frames_file_empty(int layer_number);
int count_frames(const char *folder);
gchar* get_filesize(const gchar *file_path);
gchar* get_duration(const gchar *file_path);
gchar* get_fps(const gchar *file_path);
gchar* get_resolution(const gchar *file_path);
guint get_duration_in_seconds(const gchar *file_path);
int get_width_from_resolution(const gchar *res);

// Directory / File operations
void ensure_dir(const char *path);
int copy_file(const char *src, const char *dst);
void copy_directory(const char *src, const char *dst);
void cleanup_frames_folders(void);
int get_number_of_sequences(void);

// Sequence / Video management
int encode_sequence_with_ffmpeg(int sequence_number, int fps, int width, int height);
int get_next_sequence_index(void);

// Modal callbacks
void on_modal_back_clicked(GtkButton *button, gpointer user_data);

// Inline Helpers
static inline gboolean is_mp4_file(const char *name) {
    gchar *lower = g_utf8_strdown(name, -1);
    gboolean result = g_str_has_suffix(lower, ".mp4");
    g_free(lower);
    return result;
}

#endif // UTILS_H

