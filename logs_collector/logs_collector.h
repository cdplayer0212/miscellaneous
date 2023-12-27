#ifndef __LOGS_COLLECTOR_H__
#define __LOGS_COLLECTOR_H__

#define DEGAULT_CONFIG_FILE_PATH	"/etc/logs_collector.conf"
#define DEFAULT_TEMP_DIR		"/tmp"

#define MAX_USB_DEV_NODE_LEN		64
#define MAX_USB_DEVICE_CNT		4
#define MAX_INTERNAL_PATH_CNT		4
#define MAX_LOG_PATH			256

enum app_action {
	APP_ACTION_USAGE = 0,
	APP_ACTION_VERSION,
	APP_ACTION_RUN,
};

struct __log_data {
	char log_path[PATH_MAX];
	struct stat log_stat;
};

struct __global_data {
	enum app_action action;
	char device_name[32];
	char serial_number[32];
	char *version;
	char timestamp[24];
#ifdef USE_TAR_BZ2_COMPRESS
	/* $(tmp_archive_dir)/$(device_name)_$(serial_number)_$(timestamp) */
	char dest_logs_dir[32 + 1 + 32 + 1 + 32 + 1 + 24 + 1];
	/*
	 * $(tmp_archive_dir)/$(device_name)_$(serial_number)_$(timestamp) \
	 * .tar
	 */
	char dest_archive_file[32 + 1 + 32 + 1 + 32 + 1 + 24 + 4 + 1];
	/* $(device_name)_$(serial_number)_$(timestamp) */
	char dest_archive_extract_to[32 + 1 + 32 + 1 + 24 + 1];
	/*
	 * $(tmp_archive_dir)/$(device_name)_$(serial_number)_$(timestamp) \
	 * .tar.bz2
	 */
	// char dest_archive_compress_file[32 + 1 + 32 + 1 + 32 + 1 + 24 + 8 + 1];
	/*
	 * $(device_name)_$(serial_number)_$(timestamp).tar.bz2
	 */
	char tar_bz2_base_file[32 + 1 + 32 + 1 + 24 + 8 + 1];
	/*
	 * $(tmp_archive_dir)/$(device_name)_$(serial_number)_$(timestamp) \
	 * .tar.bz2
	 */
	char tar_bz2_base_file_path[32 + 1 + 32 + 1 + 32 + 1 + 24 + 8 + 1];
#endif /* USE_TAR_BZ2_COMPRESS */
#ifdef USE_ZIP_COMPRESS
	/* $(device_name)_$(serial_number)_$(timestamp).zip */
	char zip_base_file[32 + 1 + 32 + 1 + 24 + 4 + 1];
	/*
	 * $(tmp_archive_dir)/$(device_name)_$(serial_number)_$(timestamp) \
	 * .zip
	 */
	char zip_base_file_path[32 + 1 + 32 + 1 + 32 + 1 + 24 + 4 + 1];
#endif /* USE_ZIP_COMPRESS */
	char confile[PATH_MAX];
	uint32_t usb_device_cnt;
	char usb_device_path[MAX_USB_DEVICE_CNT][MAX_USB_DEV_NODE_LEN];
	char usb_mount_path[MAX_USB_DEVICE_CNT][PATH_MAX];
	uint32_t internal_path_cnt;
	char internal_path[MAX_INTERNAL_PATH_CNT][PATH_MAX];
	uint32_t log_data_cnt;
	struct __log_data log_data[MAX_LOG_PATH];
};

#endif /* __LOGS_COLLECTOR_GTK_H__ */
