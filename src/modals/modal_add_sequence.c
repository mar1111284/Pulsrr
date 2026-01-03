#include "modal_add_sequence.h"
#include "../utils/utils.h"
#include "../sdl/sdl.h"
#include "../components/component_sequencer.h"
#include "../utils/accessor.h"
#include <SDL2/SDL_image.h>

int encode_frames_folder_with_ffmpeg(const gchar *frames_folder, const gchar *output_mp4, int fps, int width, int height)
{
    if (!g_file_test(frames_folder, G_FILE_TEST_IS_DIR)) {
        add_main_log(g_strdup_printf("[FFMPEG] Folder not found: %s", frames_folder));
        return -1;
    }

    gchar *cmd = g_strdup_printf(
        "ffmpeg -y -framerate %d -i \"%s/frame_%%05d.png\" -s %dx%d -pix_fmt yuv420p -c:v libx264 \"%s\"",
        fps, frames_folder, width, height, output_mp4
    );

    int ret = system(cmd);
    if (ret != 0) {
        add_main_log(g_strdup_printf("[FFMPEG] Encoding failed (code=%d)", ret));
        g_free(cmd);
        return -1;
    }

    add_main_log("[FFMPEG] Video generated successfully.");
    g_free(cmd);
    return 0;
}

void generate_sequence_frames(int duration, int width, int height, const gchar *sequence_folder, AddSequenceUI *ui)
{
    Layer layers[MAX_LAYERS] = {0};
    gchar layer_folder[PATH_MAX];
    gchar fx_path[PATH_MAX];

    // Parse fx.txt
    snprintf(fx_path, sizeof(fx_path), "%s/fx.txt", sequence_folder);
    FILE *fx_file = fopen(fx_path, "r");
    if (!fx_file) {
        add_log(ui, "[ERROR] Cannot open fx.txt");
        return;
    }

    for (int i = 0; i < MAX_LAYERS; i++) {
        layers[i].speed = 1.0;
        layers[i].grayscale = 0;
        layers[i].alpha = 255;
        layers[i].frame_count = 0;
        layers[i].frames = NULL;
        layers[i].frames_gray = NULL;
        layers[i].frame_folder = NULL;
    }

    int current_layer = -1;
    char line[128];
    while (fgets(line, sizeof(line), fx_file)) {
        if (strncmp(line, "layer=", 6) == 0) {
            current_layer = atoi(line + 6);
        } else if (strncmp(line, "speed=", 6) == 0 && current_layer >= 0) {
            layers[current_layer].speed = atof(line + 6);
        } else if (strncmp(line, "gray=", 5) == 0 && current_layer >= 0) {
            layers[current_layer].grayscale = atoi(line + 5);
        } else if (strncmp(line, "alpha=", 6) == 0 && current_layer >= 0) {
            layers[current_layer].alpha = atoi(line + 6);
        }
    }
    fclose(fx_file);

    // Load frames for each layer
    set_progress_add_sequence(ui, 0.1, "Loading layers...");
    for (int i = 0; i < MAX_LAYERS; i++) {
        snprintf(layer_folder, sizeof(layer_folder), "%s/Frames_%d", sequence_folder, i + 1);
        layers[i].frame_folder = g_strdup(layer_folder);
        layers[i].frame_count = count_frames(layers[i].frame_folder);
        if (layers[i].frame_count == 0) continue;

        layers[i].frames = malloc(sizeof(SDL_Surface*) * layers[i].frame_count);
        layers[i].frames_gray = malloc(sizeof(SDL_Surface*) * layers[i].frame_count);

        for (int f = 0; f < layers[i].frame_count; f++) {
            gchar frame_file[PATH_MAX];
            snprintf(frame_file, sizeof(frame_file), "%s/frame_%05d.png", layers[i].frame_folder, f + 1);
            SDL_Surface *surf = IMG_Load(frame_file);
            layers[i].frames[f] = surf;

			SDL_Surface *surf_gray = NULL;
			if (layers[i].grayscale) {
				surf_gray = create_grayscale_surface(surf);
			} else {
				surf_gray = NULL;
			}
			layers[i].frames_gray[f] = surf_gray;

        }

        add_log(ui, g_strdup_printf("[LOAD] Layer %d loaded (%d frames)", i + 1, layers[i].frame_count));
    }

    // Prepare mixed_frames folder
    gchar mixed_dir[PATH_MAX];
    snprintf(mixed_dir, sizeof(mixed_dir), "%s/mixed_frames", sequence_folder);
    ensure_dir(mixed_dir);

    int total_output_frames = duration * 25; // fixed 25 FPS
    set_progress_add_sequence(ui, 0.5, "Mixing frames...");

    for (int f = 0; f < total_output_frames; f++) {
        SDL_Surface *mixed = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        SDL_FillRect(mixed, NULL, SDL_MapRGBA(mixed->format, 0, 0, 0, 255));

        for (int l = 0; l < MAX_LAYERS; l++) {
            if (layers[l].frame_count == 0) continue;

            int frame_idx;
            if (layers[l].speed >= 1.0)
                frame_idx = (int)(f * layers[l].speed) % layers[l].frame_count;
            else {
                int repeat = (int)(1.0 / layers[l].speed + 0.5);
                frame_idx = (f / repeat) % layers[l].frame_count;
            }

            SDL_Surface *src = layers[l].grayscale ? layers[l].frames_gray[frame_idx] : layers[l].frames[frame_idx];
            if (!src) continue;

            SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_BLEND);
            SDL_SetSurfaceAlphaMod(src, layers[l].alpha);

            SDL_Rect dest = {0, 0, width, height};
            SDL_BlitScaled(src, NULL, mixed, &dest);
        }

        gchar *frame_name = g_strdup_printf("frame_%05d.png", f + 1);
        gchar *filename = g_build_filename(mixed_dir, frame_name, NULL);
        IMG_SavePNG(mixed, filename);
        g_free(frame_name);
        g_free(filename);
        SDL_FreeSurface(mixed);

        if (f % (total_output_frames / 10) == 0)
            set_progress_add_sequence(ui, 0.5 + 0.4 * f / total_output_frames, "Mixing frames...");
    }

    // Cleanup
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (!layers[i].frames) continue;
        for (int f = 0; f < layers[i].frame_count; f++) {
            if (layers[i].frames[f]) SDL_FreeSurface(layers[i].frames[f]);
            if (layers[i].frames_gray[f]) SDL_FreeSurface(layers[i].frames_gray[f]);
        }
        free(layers[i].frames);
        free(layers[i].frames_gray);
        g_free(layers[i].frame_folder);
    }

    set_progress_add_sequence(ui, 1.0, "Completed");
    add_log(ui, "[INFO] Sequence frames generated successfully.");
}


void set_progress_add_sequence(AddSequenceUI *ui, double fraction, const char *text)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ui->progress_bar), fraction);

    if (text) {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ui->progress_bar), text);
    }

    while (gtk_events_pending())
        gtk_main_iteration();
}

void on_add_button_clicked(GtkButton *button, gpointer user_data) {

	AppContext *app_ctx = get_app_ctx();
	
	// Only pause if frames exist or not loading
    if (sdl_get_render_state() != RENDER_STATE_IDLE &&
        sdl_get_render_state() != RENDER_STATE_LOADING) {
        sdl_set_render_state(RENDER_STATE_PAUSE);
    }

	// Clear previous content if any
	GList *children = gtk_container_get_children(GTK_CONTAINER(app_ctx->modal_layer));
	for (GList *iter = children; iter != NULL; iter = iter->next) {
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);

	// Modal black box
	GtkWidget *black_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_size_request(black_box,400 ,400);
	gtk_widget_set_name(black_box, "black-modal");
	gtk_widget_set_halign(black_box, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(black_box, GTK_ALIGN_CENTER);
	
	// Header label
	GtkWidget *title_label = gtk_label_new("ADD SEQUENCE");
	gtk_widget_set_name(title_label, "modal-title");
	gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top(title_label, 20);
	gtk_widget_set_margin_bottom(title_label, 20);
	gtk_box_pack_start(GTK_BOX(black_box), title_label, FALSE, FALSE, 0);
	
	// Log area
	GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_name(log_scroll, "sequence-log-scroll");
	gtk_widget_set_size_request(log_scroll, 380, 150);
	gtk_widget_set_margin_start(log_scroll, 20);
	gtk_widget_set_margin_end(log_scroll, 20);
	gtk_widget_set_margin_bottom(log_scroll, 20);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scroll),
		                           GTK_POLICY_AUTOMATIC,
		                           GTK_POLICY_AUTOMATIC);

	GtkWidget *log_view = gtk_text_view_new();
	gtk_widget_set_name(log_view, "sequence-log");
	gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(log_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(log_view), GTK_WRAP_WORD_CHAR);

	GtkTextBuffer *log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
	gtk_text_buffer_set_text(log_buffer, "[INFO] Ready to generate sequence…\n", -1);

	gtk_container_add(GTK_CONTAINER(log_scroll), log_view);
	gtk_box_pack_start(GTK_BOX(black_box), log_scroll, TRUE, TRUE, 0);

	// Button row (Back + Add)
	GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(button_row, "button-row");
	gtk_widget_set_size_request(button_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), button_row, FALSE, FALSE, 0);

	// Back button
	GtkWidget *btn_back = gtk_button_new_with_label("BACK");
	gtk_widget_set_name(btn_back, "modal-cancel-button");
	gtk_widget_set_size_request(btn_back, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_back, TRUE, TRUE, 0);

	// Add button
	GtkWidget *btn_add_sequence = gtk_button_new_with_label("EXPORT");
	gtk_widget_set_name(btn_add_sequence, "modal-export-button");
	gtk_widget_set_size_request(btn_add_sequence, 150, 42);
	gtk_box_pack_start(GTK_BOX(button_row), btn_add_sequence, TRUE, TRUE, 0);

	// Controls row: Duration + Scale
	GtkWidget *controls_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_name(controls_row, "controls-row");
	gtk_widget_set_size_request(controls_row, 400, 50);
	gtk_box_pack_end(GTK_BOX(black_box), controls_row, FALSE, FALSE, 5);

	// Duration spin button
	GtkWidget *duration_spin = gtk_spin_button_new_with_range(1, 60, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(duration_spin), 10);
	gtk_widget_set_name(controls_row, "spin-button");
	gtk_widget_set_size_request(duration_spin, 150, 42);
	gtk_box_pack_start(GTK_BOX(controls_row), duration_spin, TRUE, TRUE, 0);

	// Scale combo box
	GtkWidget *scale_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "1080p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "720p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "480p");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(scale_combo), "360p");
	gtk_combo_box_set_active(GTK_COMBO_BOX(scale_combo), 2);
	gtk_widget_set_size_request(scale_combo, 150, 42);
	gtk_box_pack_start(GTK_BOX(controls_row), scale_combo, TRUE, TRUE, 0);
	
	// Labels row: "Duration" and "Quality"
	GtkWidget *labels_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 35);
	gtk_widget_set_size_request(labels_row, 400, 20);

	// FPS & Quality label
	GtkWidget *duration_label = gtk_label_new("Duration");
	gtk_widget_set_halign(duration_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(labels_row), duration_label, TRUE, TRUE, 0);
	GtkWidget *quality_label = gtk_label_new("Quality");
	gtk_widget_set_halign(quality_label, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(labels_row), quality_label, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(black_box), labels_row, FALSE, FALSE, 0);
	
	// Adding progress bar
	GtkWidget *progress_bar = gtk_progress_bar_new();
	gtk_widget_set_name(progress_bar, "export-progress");
	gtk_widget_set_size_request(progress_bar, 360, 20);
	
	// Initial state of progress bar
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "0%");
	gtk_box_pack_end(GTK_BOX(black_box), progress_bar, FALSE, FALSE, 10);
	
	// Populate SequcneUI
	AddSequenceUI *ui = g_new0(AddSequenceUI, 1);
	ui->duration_spin = GTK_SPIN_BUTTON(duration_spin);
	ui->scale_combo = GTK_COMBO_BOX_TEXT(scale_combo);
	ui->log_view = log_view;
	ui->log_buffer = log_buffer;
	ui->progress_bar = progress_bar;
	ui->root_container = GTK_WIDGET(user_data);
	ui->parent_container = black_box;

	g_signal_connect(btn_add_sequence, "clicked", G_CALLBACK(on_add_sequence_clicked), ui);
	g_signal_connect(btn_back, "clicked", G_CALLBACK(on_modal_back_clicked), app_ctx->modal_layer);

	gtk_container_add(GTK_CONTAINER(app_ctx->modal_layer), black_box);
	gtk_widget_show_all(app_ctx->modal_layer);
}

void add_log(AddSequenceUI *ui, const char *message) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(ui->log_buffer, &end);
    gtk_text_buffer_insert(ui->log_buffer, &end, message, -1);
    gtk_text_buffer_insert(ui->log_buffer, &end, "\n", -1);
    GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(ui->log_view));
    gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
    while (gtk_events_pending()) gtk_main_iteration();
}

void create_fx_file(const char *path, AddSequenceUI *ui)
{
	set_progress_add_sequence(ui, 0.01, "Creating fx.txt...");
    add_log(ui, "Creating fx.txt...");

    FILE *f = fopen(path, "w");
    if (!f) {
        add_log(ui, "Failed to create fx.txt!");
        return;
    }

    for (int layer = 0; layer < MAX_LAYERS; layer++) {
        double speed = sdl_get_layer_speed(layer);
        int gray = sdl_is_layer_gray(layer);
        int alpha = sdl_get_alpha(layer);

        fprintf(f, "layer=%d\n", layer);
        fprintf(f, "speed=%f\n", speed);
        fprintf(f, "gray=%d\n", gray);
        fprintf(f, "alpha=%d\n\n", alpha);

        gchar log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "Layer %d: speed=%f gray=%d alpha=%d", layer, speed, gray, alpha);
        //add_log(ui, log_msg); /*KEEP FOR DEBUG*/
    }
    fclose(f);
    add_log(ui, "fx.txt created successfully.");
}


void on_add_sequence_clicked(GtkButton *button, gpointer user_data)
{

    AddSequenceUI *ui = (AddSequenceUI *)user_data;
	gtk_widget_set_sensitive(ui->root_container, FALSE);
	gtk_widget_set_sensitive(ui->parent_container, FALSE);
    gint duration = gtk_spin_button_get_value_as_int(ui->duration_spin);
    const gchar *scale = gtk_combo_box_text_get_active_text(ui->scale_combo);
    ensure_dir("sequences");
    int seq = get_next_sequence_index();
    gchar *seq_dir = g_strdup_printf("sequences/sequence_%d", seq);
    ensure_dir(seq_dir); 
	
	set_progress_add_sequence(ui, 0.02, "Copy frame folders...");

    gchar *dst = NULL;

    dst = g_build_filename(seq_dir, "Frames_1", NULL);
    copy_directory("Frames_1", dst);
    g_free(dst);

    dst = g_build_filename(seq_dir, "Frames_2", NULL);
    copy_directory("Frames_2", dst);
    g_free(dst);

    dst = g_build_filename(seq_dir, "Frames_3", NULL);
    copy_directory("Frames_3", dst);
    g_free(dst);

    dst = g_build_filename(seq_dir, "Frames_4", NULL);
    copy_directory("Frames_4", dst);
    g_free(dst);

	// Create fx.txt
	gchar *fx_path = g_build_filename(seq_dir, "fx.txt", NULL);
	create_fx_file(fx_path, ui);
	g_free(fx_path);
    
    //we gonna figure out the output width and height here
    int output_width = 1280, output_height = 720;
    if (strcmp(scale, "1080p") == 0) { output_width = 1920; output_height = 1080; }
    else if (strcmp(scale, "720p") == 0) { output_width = 1280; output_height = 720; }
    else if (strcmp(scale, "480p") == 0) { output_width = 854; output_height = 480; }
    else if (strcmp(scale, "360p") == 0) { output_width = 640; output_height = 360; }
    
    add_log(ui, "[INFO] Add Sequence process start...");
    generate_sequence_frames(duration, output_width, output_height, seq_dir, ui);
    add_log(ui, "[INFO] Add Sequence process completed.");
    
    set_progress_add_sequence(ui, 0.9, "Updating sequence textures.");
    update_sequence_texture();
    update_sequencer();
    set_progress_add_sequence(ui, 1, "Completed.");

    g_free(seq_dir);
    gtk_widget_set_sensitive(ui->root_container, TRUE);
    gtk_widget_set_sensitive(ui->parent_container, TRUE);
    
    
}

