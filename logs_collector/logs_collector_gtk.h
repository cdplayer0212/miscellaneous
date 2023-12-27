#ifndef __LOGS_COLLECTOR_GTK_H__
#define __LOGS_COLLECTOR_GTK_H__

#define APPLICATION_ID		"com.qbic.HCseries.logs_collector"
#define WINDOW_TITLE		"Logs Collector"
#define WINDOW_WIDTH		640
#define WINDOW_HEIGHT		480
#define WINDOW_BORDER_SIZE	10
#define GRID_COL_SIZE		10
#define GRID_ROW_SIZE		10
#define BUTTON_WIDTH		(WINDOW_WIDTH - WINDOW_BORDER_SIZE)
#define BUTTON_HEIGHT		50

#define DEFAULT_TIMEOUT_SEC	180

struct __global_data {
	gboolean is_window_launched;
	gboolean is_app_running;
	uint32_t timeout_sec;
	char logs_collector_path[PATH_MAX];
	char *version;
	GtkApplication *mfg_app;
	GtkWidget *main_window;
	GtkWidget *main_grid;
	/* collector logs text view */
	GtkWidget *scrolled_window;
	GtkWidget *log_text_view;
	GtkTextBuffer *log_text_view_buffer;
	GtkTextIter text_view_offset_iter;
	/* status label */
	GtkWidget *status_label;
	/* button */
	GtkWidget *generate_btn;
	GtkWidget *exit_btn;
};

#endif /* __LOGS_COLLECTOR_GTK_H__ */
