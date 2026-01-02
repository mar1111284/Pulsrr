#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <gtk/gtk.h> 

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
#define SPACING_DEFAULT 16
#define BUTTON_SPACING 10

typedef struct {
    char *base_dir;
    char *media_dir;
    char *styles_dir;
    char *sequences_dir;
} AppPaths;

typedef struct {
    GtkTextView *log_view;
    GtkTextBuffer *log_buffer;
    GtkWidget *main_container;
} MainUI;

typedef struct {
    GtkWidget *window;
    GtkWidget *modal_layer;
    MainUI main_ui;
} AppContext;

typedef struct {
    GtkWidget *progress_bar;
    double     fraction;
    char      *text;
} ProgressUpdate;

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

// App paths
const AppPaths *get_app_paths(void);
void init_app_paths(const char *argv0);
void free_app_paths(void);

// App context
AppContext* get_app_ctx(void);
void set_app_ctx(AppContext *ctx);

// Extern Globals
extern SelectedBar selected_bar;

// Utility Functions
void add_main_log(const char *message);

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
int count_files_in_dir(const char *path);

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

