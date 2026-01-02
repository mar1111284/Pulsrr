#ifndef COMPONENT_APHORISM_H
#define COMPONENT_APHORISM_H

#include <gtk/gtk.h>

typedef enum {
    APHORISM_OK = 0,
    APHORISM_FILE_NOT_FOUND,
    APHORISM_EMPTY_FILE,
    APHORISM_READ_FAILED,
    APHORISM_MEMORY_ERROR
} AphorismErrorCode;

GtkWidget *create_aphorism_widget(void);
AphorismErrorCode get_random_aphorism(const char *filename, char **out_str);

#endif

