#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

#include <gtk/gtk.h>

#include "logger.h"
#include "common_funcs.h"
#include "logs_collector_app_name.h"
#include "logs_collector_gtk.h"
#include "logs_collector_ver.h"

void enable_btn(struct __global_data *gdata, gboolean generate_en,
							gboolean exit_en)
{
	gtk_widget_set_sensitive(gdata->generate_btn, generate_en);
	gtk_widget_set_sensitive(gdata->exit_btn, exit_en);
}

static gboolean logs_collector_run(gpointer data)
{
	struct __global_data *gdata = data;

	gdata->is_app_running = TRUE;
	do_popen_cmd(gdata->logs_collector_path, NULL, 0);
	gdata->is_app_running = FALSE;
	return FALSE;
}

static void update_text_view(struct __global_data *gdata, char *data,
								size_t data_len)
{
	GtkTextMark *mark;

	if (gdata->is_window_launched == FALSE)
		return;

	gtk_text_buffer_get_end_iter(gdata->log_text_view_buffer,
						&gdata->text_view_offset_iter);
	gtk_text_buffer_insert(gdata->log_text_view_buffer,
						&gdata->text_view_offset_iter,
						data, data_len);
	/* scroll window auto scroll to last */
	mark = gtk_text_buffer_get_mark(gdata->log_text_view_buffer, "insert");
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(gdata->log_text_view), mark,
							0.0, FALSE, 0.0, 0.0);
}

static gboolean update_log_msgs(gpointer data)
{
	struct __global_data *gdata = data;
	FILE *fp;
	int ret;
	char msg[128];

	ret = check_file_is_exist(DEFAULT_LOGGER_FILE);
	if (ret != 0) {
		snprintf(msg, sizeof(msg), "cannot found %s",
							DEFAULT_LOGGER_FILE);
		update_text_view(data, msg, sizeof(msg));
		enable_btn(gdata, TRUE, TRUE);
		return FALSE;
	}

	fp = fopen(DEFAULT_LOGGER_FILE, "r");
	if (fp == NULL) {
		snprintf(msg, sizeof(msg), "cannot open %s",
							DEFAULT_LOGGER_FILE);
		update_text_view(data, msg, sizeof(msg));
		enable_btn(gdata, TRUE, TRUE);
		return FALSE;
	}
	while (!feof(fp)) {
		memset(msg, 0x0, sizeof(msg));
		fread(msg, 1, 1, fp);
		update_text_view(data, msg, strlen(msg));
	}
	fclose(fp);
	enable_btn(gdata, TRUE, TRUE);
	return FALSE;
}

static gboolean check_apps_status(gpointer data)
{
	struct __global_data *gdata = data;
	char msg[64];
	gboolean ret = FALSE;

	INFO("elapsed time: %d, is_app_running: %d\n", gdata->timeout_sec,
					gdata->is_app_running);
	if (gdata->is_app_running == TRUE) {
		gdata->timeout_sec++;
		if (gdata->timeout_sec == DEFAULT_TIMEOUT_SEC) {
			snprintf(msg, sizeof(msg),
				"FAILED, Elapsed time: %d secs(timeout)",
				gdata->timeout_sec);
			enable_btn(gdata, FALSE, TRUE);
			ret = FALSE;
		} else {
			snprintf(msg, sizeof(msg), "Elapsed time: %d secs",
						gdata->timeout_sec);
			ret = TRUE;
		}
		gtk_label_set_markup(GTK_LABEL(gdata->status_label), msg);
		return ret;
	}

	snprintf(msg, sizeof(msg), "DONE, Elapsed time: %d secs",
							gdata->timeout_sec);
	gtk_label_set_markup(GTK_LABEL(gdata->status_label), msg);
	gdk_threads_add_timeout(100, (GSourceFunc)update_log_msgs, gdata);
	return FALSE;
}

static void clear_text_view_buffer(struct __global_data *gdata)
{
	static GtkTextIter text_view_start_iter;

	gtk_text_buffer_get_start_iter(gdata->log_text_view_buffer,
						&text_view_start_iter);
	gtk_text_buffer_delete(gdata->log_text_view_buffer,
						&text_view_start_iter,
						&gdata->text_view_offset_iter);
}

void exec_logs_collector(GtkWidget *button, gpointer data)
{
	struct __global_data *gdata = data;

	clear_text_view_buffer(gdata);
	gdata->timeout_sec = 0;
	gdk_threads_add_timeout(1, (GSourceFunc)logs_collector_run, gdata);
	gdk_threads_add_timeout(500, (GSourceFunc)check_apps_status, gdata);
}

static void create_generate_btn(struct __global_data *gdata)
{
	gdata->generate_btn = gtk_button_new_with_label(
						"Execute Logs Collector");
	g_signal_connect(GTK_BUTTON(gdata->generate_btn), "clicked",
			G_CALLBACK(exec_logs_collector), gdata);
	gtk_widget_set_size_request(gdata->generate_btn, BUTTON_WIDTH,
								BUTTON_HEIGHT);
	g_object_set(gdata->generate_btn, "margin", 5, NULL);
	gtk_grid_attach(GTK_GRID(gdata->main_grid), gdata->generate_btn, 0, 6,
								4, 1);
}

void button_clicked_exit(GtkWidget *button, gpointer data)
{
	struct __global_data *gdata = data;

	g_application_quit(G_APPLICATION(gdata->mfg_app));
}

static void create_exit_btn(struct __global_data *gdata)
{
	gdata->exit_btn = gtk_button_new_with_label("Exit");
	g_signal_connect(GTK_BUTTON(gdata->exit_btn), "clicked",
			G_CALLBACK(button_clicked_exit), gdata);
	gtk_widget_set_size_request(gdata->exit_btn, BUTTON_WIDTH,
								BUTTON_HEIGHT);
	g_object_set(gdata->exit_btn, "margin", 5, NULL);
	gtk_grid_attach(GTK_GRID(gdata->main_grid), gdata->exit_btn, 0, 7,
								4, 1);
}

static void create_status_label(struct __global_data *gdata)
{
	gdata->status_label = gtk_label_new(NULL);
	gtk_grid_attach(GTK_GRID(gdata->main_grid),
						gdata->status_label, 0, 5,
						4, 1);
}

static void create_log_text_view(struct __global_data *gdata)
{
	gdata->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(gdata->scrolled_window, BUTTON_WIDTH,
					WINDOW_HEIGHT - (BUTTON_HEIGHT * 2));
	gdata->log_text_view = gtk_text_view_new();
	gdata->log_text_view_buffer =
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(gdata->log_text_view));
	gtk_text_buffer_get_iter_at_offset(gdata->log_text_view_buffer,
					&gdata->text_view_offset_iter, 0);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(gdata->log_text_view), FALSE);
	gtk_container_add(GTK_CONTAINER(gdata->scrolled_window),
							gdata->log_text_view);
	gtk_grid_attach(GTK_GRID(gdata->main_grid),
						gdata->scrolled_window, 0, 0,
						4, 4);
}

static gboolean check_apps_is_exist(gpointer data)
{
	struct __global_data *gdata = data;
	char *app_name = NULL;
	char msg[64 + PATH_MAX];

	app_name = get_system_app_path(APP_NAME);
	if (app_name == NULL) {
		snprintf(msg, sizeof(msg), "cannot found %s", APP_NAME);
		gtk_label_set_markup(GTK_LABEL(gdata->status_label), msg);
		enable_btn(gdata, FALSE, TRUE);
		enable_btn(gdata, TRUE, TRUE);
	} else {
		strncpy(gdata->logs_collector_path, app_name,
					sizeof(gdata->logs_collector_path) - 1);
		snprintf(msg, sizeof(msg), "APP %s found",
						gdata->logs_collector_path);
		gtk_label_set_markup(GTK_LABEL(gdata->status_label), msg);
		enable_btn(gdata, TRUE, TRUE);
	}
	return FALSE;
}

static void activate_func(GtkApplication *app, gpointer user_data)
{
	struct __global_data *gdata = user_data;
	char title[64];

	gdata->main_window = gtk_application_window_new(app);

	gtk_window_set_resizable(GTK_WINDOW(gdata->main_window), FALSE);

	gtk_window_set_default_size(GTK_WINDOW(gdata->main_window), 480, 360);

	gtk_window_set_position(GTK_WINDOW(gdata->main_window),
						GTK_WIN_POS_CENTER_ALWAYS);

	memset(title, 0x0, sizeof(title));
	snprintf(title, sizeof(title), "%s (ver: %s)", WINDOW_TITLE,
								gdata->version);
	gtk_window_set_title(GTK_WINDOW(gdata->main_window), title);
	gtk_container_set_border_width(
		GTK_CONTAINER(gdata->main_window), WINDOW_BORDER_SIZE);
	gdata->main_grid = gtk_grid_new();
	gtk_widget_set_name(gdata->main_grid, "myGrid");
	gtk_grid_set_column_spacing(GTK_GRID(gdata->main_grid), GRID_COL_SIZE);
	gtk_grid_set_row_spacing(GTK_GRID(gdata->main_grid), GRID_ROW_SIZE);
	gtk_container_add(GTK_CONTAINER(gdata->main_window), gdata->main_grid);
	create_log_text_view(gdata);
	create_status_label(gdata);
	create_generate_btn(gdata);
	create_exit_btn(gdata);

	enable_btn(gdata, FALSE, FALSE);
	gdk_threads_add_timeout(300, (GSourceFunc)check_apps_is_exist, gdata);
	gtk_widget_show_all(gdata->main_window);
}

int main(int argc, char *argv[])
{
	struct __global_data gdata;
	int status = 0;

	log_initialize(_LOG_INFO, _LOG_PRINTF);
	init_check_system_apps();
	memset(&gdata, 0x0, sizeof(gdata));
	gdata.version = get_version_string();

	gdata.mfg_app = gtk_application_new(APPLICATION_ID,
						G_APPLICATION_NON_UNIQUE);

	g_signal_connect(gdata.mfg_app, "activate",
					G_CALLBACK(activate_func), &gdata);
	gdata.is_window_launched = TRUE;
	status = g_application_run(G_APPLICATION(gdata.mfg_app), argc, argv);
	gdata.is_window_launched = FALSE;
	g_object_unref(gdata.mfg_app);

	log_uninitialize();
	return status;
}
