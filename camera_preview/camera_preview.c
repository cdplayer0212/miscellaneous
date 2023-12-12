#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>

#include <sys/utsname.h>

#include <gtk/gtk.h>

#include "mfg_logger.h"
#include "camera_util.h"

#define CALC_FPS

#define APPLICATION_ID          "org.test"
#define GRID_COL_SIZE           10                                              
#define GRID_ROW_SIZE           10

static GtkApplication *camera_preview_app;
static GtkWidget *camera_preview_window;
static GtkWidget *camera_preview_grid;
static GtkWidget *camera_preview;

static gboolean camera_preview_window_is_on;

static time_t time_tag;

static int app_ret;

static struct camera_util_data *camera_data_priv;

#ifdef CALC_FPS
static void update_fps(struct camera_util_data *cdata, cairo_t *cr,
							GdkPixbuf *pixbuf_rgb)
{
	char str[16];
	double x, y;
	cairo_text_extents_t extents;

	cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL,
						CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 16);
	cairo_set_source_rgb(cr, 0, 0, 1);
	snprintf(str, sizeof(str), "FPS: %ld", cdata->fps_val);
	cairo_text_extents(cr, str, &extents);
	x = CAMERA_PREVIEW_WIDTH - extents.width + extents.x_bearing - 20;
	y = CAMERA_PREVIEW_HEIGHT - extents.height + extents.y_bearing;

	cairo_move_to(cr, x, y);
	cairo_show_text(cr, str);
}
#endif /* CALC_FPS */

static gboolean camera_preview_draw_function(GtkWidget *widget, cairo_t *cr,
								gpointer data_p)
{
	struct camera_util_data *cdata = (struct camera_util_data *)data_p;
	GdkPixbuf *pixbuf_rgb;

	cdata->raw_image_status = RAW_IMAGE_SUBMITTING;

	pixbuf_rgb = gdk_pixbuf_new_from_data((guchar *)cdata->rgb_buffer,
			GDK_COLORSPACE_RGB, FALSE, 8,
			cdata->camera_width, cdata->camera_height,
			cdata->camera_width * cdata->camera_depth,
							NULL, NULL);
	gdk_cairo_set_source_pixbuf(cr, pixbuf_rgb, 0, 0);
	cairo_paint(cr);
#ifdef CALC_FPS
	if (cdata->show_fps)
		update_fps(cdata, cr, pixbuf_rgb);

#endif /* CALC_FPS */
	g_object_unref(pixbuf_rgb);
	cdata->raw_image_status = RAW_IMAGE_EMPTY;
	return FALSE;
}

static gboolean camera_preview_thread(gpointer data)
{
	struct camera_util_data *cdata = (struct camera_util_data *)data;
	int ret;
	static GdkPixbuf *pixbuf_rgb;
	static gboolean show_captured_frame_cnt_flag = TRUE;

	cdata->current_time = time(NULL);
	if (cdata->is_test_completed) {
		INFO("is_test_completed: %d\n", cdata->is_test_completed);
		return FALSE;
	}
	if (camera_preview_window_is_on == FALSE) {
		INFO("preview_window: %d\n", camera_preview_window_is_on);
		return FALSE;
	}
	if (cdata->raw_image_status != RAW_IMAGE_EMPTY)
		return TRUE;

	cdata->raw_image_status = RAW_IMAGE_FETCHING;
	ret = get_camera_frame(cdata);
	if (ret != 0) {
		cdata->raw_image_status = RAW_IMAGE_EMPTY;
		INFO("capture raw image failed(%d), retry\n", ret);
		return TRUE;
	}
	cdata->raw_image_status = RAW_IMAGE_FETCHED;
	if (cdata->show_fps)
		cdata->image_count++;

	gtk_widget_queue_draw(camera_preview);

	if (camera_preview_window_is_on == FALSE)
		return FALSE;

	return TRUE;
}

#ifdef CALC_FPS
static gboolean camera_update_fps_thread(gpointer data)
{
	struct camera_util_data *cdata = (struct camera_util_data *)data;

	if (camera_preview_window_is_on == FALSE)
		return FALSE;

	cdata->fps_val = cdata->image_count;
	cdata->image_count = 0;
	INFO("fps: %ld\n", cdata->fps_val);
	return TRUE;
}
#endif /* CALC_FPS */

static gboolean camera_preview_start_thread(gpointer data)
{
	int ret;
	struct camera_util_data *cdata = (struct camera_util_data *)data;

	cdata->raw_image_status = RAW_IMAGE_EMPTY;
	cdata->image_count = 0;
	cdata->is_test_completed = 0;
	time_tag = 0;

	INFO("open video device node: %s\n", cdata->dev_path);
	ret = start_camera_preview(cdata);
	if (ret != 0) {
		WARNING("start camera preview failed\n");
		app_ret = -1;
		return FALSE;
	}
	g_usleep(G_USEC_PER_SEC / 5);
	/* 1000 / 33 = 30 fps */
	gdk_threads_add_timeout(30, (GSourceFunc)camera_preview_thread, cdata);
#ifdef CALC_FPS
	if (cdata->show_fps) {
		cdata->fps_val = 0;
		gdk_threads_add_timeout(1000,
				(GSourceFunc)camera_update_fps_thread, cdata);
	}
#endif /* CALC_FPS */
	return FALSE;
}

int camera_data_init(struct camera_util_data *camera_data)
{
	memset(camera_data, 0x0, sizeof(struct camera_util_data));
	strncpy(camera_data->dev_path, CAMERA_DEV,
					sizeof(camera_data->dev_path) - 1);
	camera_data->camera_width = CAMERA_PREVIEW_WIDTH;
	camera_data->camera_height = CAMERA_PREVIEW_HEIGHT;
	camera_data->camera_depth = CAMERA_PREVIEW_DEPTH;
	camera_data->format = CAMERA_FORMAT_MJPEG;

	return 0;
}

int camera_post_exit(struct camera_util_data *camera_data)
{
	return 0;
}

static void camera_preview_destroy_window_cb(GtkWidget *widget,
							gpointer user_data)
{
	struct camera_util_data *camera_data =
					(struct camera_util_data *)user_data;

	camera_preview_window_is_on = FALSE;
}


static void camera_preview_win_activate(GtkApplication *app, gpointer user_data)
{
	int i;
	struct camera_util_data *camera_data =
					(struct camera_util_data *)user_data;

	camera_preview_window = gtk_application_window_new(app);
	gtk_window_set_resizable(GTK_WINDOW(camera_preview_window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(camera_preview_window),
						camera_data->camera_width,
						camera_data->camera_height);
	gtk_window_set_position(GTK_WINDOW(camera_preview_window),
						GTK_WIN_POS_CENTER_ALWAYS);
	gtk_widget_set_valign(camera_preview_window, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(camera_preview_window, GTK_ALIGN_CENTER);
	gtk_window_set_modal(GTK_WINDOW(camera_preview_window), TRUE);

	gtk_window_set_resizable(GTK_WINDOW(camera_preview_window), FALSE);
	gtk_widget_add_events(camera_preview_window, GDK_BUTTON_PRESS_MASK);
	gtk_window_set_position(GTK_WINDOW(camera_preview_window),
						GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_position(GTK_WINDOW(camera_preview_window),
						GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(camera_preview_window),
							"Camera Preview");
	g_signal_connect(G_OBJECT(camera_preview_window),
		"destroy", G_CALLBACK(camera_preview_destroy_window_cb),
		camera_data);

	camera_preview_grid = gtk_grid_new();
	gtk_widget_set_valign(camera_preview_grid, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(camera_preview_grid, GTK_ALIGN_CENTER);
	gtk_widget_set_name(camera_preview_grid, "myGrid");

	gtk_grid_set_column_spacing(GTK_GRID(camera_preview_grid),
								GRID_COL_SIZE);
	gtk_grid_set_row_spacing(GTK_GRID(camera_preview_grid), GRID_ROW_SIZE);

	gtk_container_add(GTK_CONTAINER(camera_preview_window),
							camera_preview_grid);

	for (i = 0; i < sizeof(camera_data->rgb_buffer); i++) {
		if (i % 3 == 2)
			camera_data->rgb_buffer[i] = 0xFF;
		else
			camera_data->rgb_buffer[i] = 0x00;
	}

	camera_preview = gtk_drawing_area_new();
	gtk_widget_set_size_request(camera_preview, CAMERA_PREVIEW_WIDTH,
							CAMERA_PREVIEW_HEIGHT);
	gtk_widget_set_app_paintable(camera_preview, TRUE);
	g_signal_connect(G_OBJECT(camera_preview), "draw",
			G_CALLBACK(camera_preview_draw_function), camera_data);
	gtk_grid_attach(GTK_GRID(camera_preview_grid), camera_preview, 0, 0,
									2, 2);

	camera_preview_window_is_on = TRUE;
	gtk_widget_show_all(camera_preview_window);
	INFO("exit\n");
}

static int destory_camera_preview(struct camera_util_data *camera_data)
{
	int ret = 0;

	return ret;
}

static int parsing_options(struct camera_util_data *camera_data,
						int argc, char *argv[])
{
	int ret = -1;
	GOptionContext *context;
	gchar *tmp;
	gboolean show_ver = FALSE;
	gboolean show_fps = FALSE;
	gboolean parse_ret;
	const GOptionEntry options[] = {
		{
			.long_name = "device",
			.short_name = 'd',
			.flags = G_OPTION_FLAG_NONE,
			.arg = G_OPTION_ARG_STRING,
			.arg_data = &tmp,
			.description = "specific camera device node path",
			.arg_description = "</dev/videoX>",
		},
		{
			.long_name = "version",
			.short_name = 'v',
			.flags = G_OPTION_FLAG_NONE,
			.arg = G_OPTION_ARG_NONE,
			.arg_data = &show_ver,
			.description = "show version",
		},
		{
			.long_name = "fps",
			.short_name = 'f',
			.flags = G_OPTION_FLAG_NONE,
			.arg = G_OPTION_ARG_NONE,
			.arg_data = &show_fps,
			.description = "show fps info on image",
		},
		{
			NULL,
		}
	};

	if (argc == 1)
		return 0;

	context = g_option_context_new("- simple camera preview tool");
	g_option_context_add_main_entries(context, options, NULL);
	parse_ret = g_option_context_parse(context, &argc, &argv, NULL);
	g_option_context_free(context);

	if (parse_ret == FALSE) {
		g_free(tmp);
		return ret;
	}

	ret = 0;
	if (show_ver == TRUE)
		camera_data->camera_action = CAMERA_VERSION_ACTION;

	if (show_fps == TRUE)
		camera_data->show_fps = 1;

	if (tmp != NULL && strlen(tmp)) {
		memset(camera_data->dev_path, 0x0,
					sizeof(camera_data->dev_path));
		strncpy(camera_data->dev_path, tmp,
					sizeof(camera_data->dev_path) - 1);
		INFO("get new dev: [%s]\n", camera_data->dev_path);
		g_free(tmp);
	}

	return ret;
}

static void print_version(void)
{
	printf("%d.%d.%d\n", APP_MAJOR_REV, APP_MINOR_REV, APP_MICRO_REV);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	struct camera_util_data camera_data;
	char app_name[64];

	app_ret = 0;
	camera_data_init(&camera_data);
	ret = parsing_options(&camera_data, argc, argv);
	if (ret != 0) {
		WARNING("parsing argc, argv failed\n");
		return ret;
	}

	if (camera_data.camera_action == CAMERA_VERSION_ACTION) {
		print_version();
		return 0;
	}

	memset(app_name, 0x0, sizeof(app_name));
	snprintf(app_name, sizeof(app_name), "%s.camera_preview",
							APPLICATION_ID);
	camera_preview_app = gtk_application_new(app_name,
						G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(camera_preview_app, "activate",
			G_CALLBACK(camera_preview_win_activate), &camera_data);
	gdk_threads_add_timeout(300, (GSourceFunc)camera_preview_start_thread,
								&camera_data);
	camera_data_priv = &camera_data;
	g_application_run(G_APPLICATION(camera_preview_app), argc, argv);
	g_object_unref(camera_preview_app);
	camera_data_priv = NULL;
	stop_camera_preview(&camera_data);
	destory_camera_preview(&camera_data);
	camera_post_exit(&camera_data);

	return app_ret;
}
