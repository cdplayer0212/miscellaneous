#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "logs_collector.h"
#include "logs_collector_ver.h"
#include "logs_collector_conf.h"
#include "logs_collector_app_name.h"
#include "logger.h"
#include "common_funcs.h"
#include "mounts_funcs.h"

// #define ENABLE_CONF_DUMP

#ifdef USE_TAR_BZ2_COMPRESS
#include "tar_bz2_funcs.h"
#endif /* USE_TAR_BZ2_COMPRESS */
#ifdef USE_ZIP_COMPRESS
#include "zip_funcs.h"
#endif /* USE_ZIP_COMPRESS */

static void usage(struct __global_data *gdata)
{
	printf("%s: collect logs and deliver\n"
		"version: %s, built-time: %s, %s\n"
		"  -c,--confile <conf_file>: set config file path\n"
		"    default config file: %s\n"
		"  -d,--debug <level>: set debug level, range %d to %d\n"
		"  -v,--version: show version information\n"
		"  -h,--help: show this message\n",
		APP_NAME, gdata->version, __TIME__, __DATE__,
		DEGAULT_CONFIG_FILE_PATH, _LOG_EMERG, _LOG_DEBUG);
}

static int get_options(struct __global_data *gdata, int argc, char *argv[])
{
	int ret = 0;
	int c;
	static const char *sopts = "c:d:vh";
	struct option lopts[] = {
		{ "confile",    required_argument, NULL, 'c' },
		{ "debug",      required_argument, NULL, 'd' },
		{ "version",    no_argument,       NULL, 'v' },
		{ "help",       no_argument,       NULL, 'h' },
	};
	long long_tmp;

	while ((c = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
		switch (c) {
		case 'c':
			memset(gdata->confile, 0x0, sizeof(gdata->confile));
			strncpy(gdata->confile, optarg,
						sizeof(gdata->confile) - 1);
			break;
		case 'd':
			if (dec_string_to_long(optarg, &long_tmp) != 0) {
				gdata->action = APP_ACTION_USAGE;
				ret = -1;
				break;
			}
			if ((enum logger_level)long_tmp >= _LOG_EMERG &&
				(enum logger_level)long_tmp <= _LOG_DEBUG)
				set_log_level((enum logger_level)long_tmp);

			break;
		case 'v':
			gdata->action = APP_ACTION_VERSION;
			break;
		case 'h':
		default:
			gdata->action = APP_ACTION_USAGE;
			ret = -1;
			break;
		};
	}
	return ret;
}

static void init_default_gdata(struct __global_data *gdata)
{
	memset(gdata, 0x0, sizeof(struct __global_data));
	gdata->version = get_version_string();
	gdata->action = APP_ACTION_RUN;
	strncpy(gdata->confile, DEGAULT_CONFIG_FILE_PATH,
						sizeof(gdata->confile) - 1);
	get_device_name(gdata->device_name, sizeof(gdata->device_name));
	get_device_sn(gdata->serial_number, sizeof(gdata->serial_number));
	gdata->version = get_version_string();
	get_iso8601_timestamp(gdata->timestamp, sizeof(gdata->timestamp));
}

#ifdef ENABLE_CONF_DUMP
static void dump_conf_config(struct __global_data *gdata)
{
	unsigned int i;

	DEBUG("action: %d\n", gdata->action);
	DEBUG("device_name: %s\n", gdata->device_name);
	DEBUG("serial_number: %s\n", gdata->serial_number);
	DEBUG("version: %s\n", gdata->version);

	DEBUG("timestamp: %s\n", gdata->timestamp);

#ifdef USE_TAR_BZ2_COMPRESS
	DEBUG("dest_logs_dir: %s\n", gdata->dest_logs_dir);
	DEBUG("dest_archive_file: %s\n", gdata->dest_archive_file);
	DEBUG("dest_archive_extract_to: %s\n", gdata->dest_archive_extract_to);
	DEBUG("tar_bz2_base_file: %s\n", gdata->tar_bz2_base_file);
	DEBUG("tar_bz2_base_file_path: %s\n", gdata->tar_bz2_base_file_path);
#endif /* USE_TAR_BZ2_COMPRESS */

#ifdef USE_ZIP_COMPRESS
	DEBUG("zip_base_file: %s\n", gdata->zip_base_file);
	DEBUG("zip_base_file_path: %s\n", gdata->zip_base_file_path);
#endif /* USE_ZIP_COMPRESS */

	DEBUG("confile: %s\n", gdata->confile);
	DEBUG("ftp_path[%d]: [%s]\n", i, gdata->ftp_path);

	DEBUG("usb_device_cnt: %d\n", gdata->usb_device_cnt);
	for (i = 0; i < gdata->usb_device_cnt; i++)
		DEBUG("  usb_device_path[%d]: [%s]\n", i,
						gdata->usb_device_path[i]);

	DEBUG("internal_path_cnt: %d\n", gdata->internal_path_cnt);
	for (i = 0; i < gdata->internal_path_cnt; i++)
		DEBUG("  internal_path[%d]: [%s]\n", i,
						gdata->internal_path[i]);

	DEBUG("log_data_cnt: %d\n", gdata->log_data_cnt);
	for (i = 0; i < gdata->log_data_cnt; i++)
		DEBUG("  log_data[%3d]: [%s]\n", i,
						gdata->log_data[i].log_path);
}
#endif /* ENABLE_CONF_DUMP */

static void generate_dest_logs_path(struct __global_data *gdata)
{
#ifdef USE_TAR_BZ2_COMPRESS
	snprintf(gdata->dest_logs_dir, sizeof(gdata->dest_logs_dir),
		"%s/%s_%s_%s", DEFAULT_TEMP_DIR, gdata->device_name,
					gdata->serial_number, gdata->timestamp);
	snprintf(gdata->dest_archive_file, sizeof(gdata->dest_archive_file),
						"%s.tar", gdata->dest_logs_dir);
	snprintf(gdata->dest_archive_extract_to,
					sizeof(gdata->dest_archive_extract_to),
					"%s_%s_%s", gdata->device_name,
					gdata->serial_number, gdata->timestamp);
	snprintf(gdata->tar_bz2_base_file,
				sizeof(gdata->tar_bz2_base_file),
				"%s_%s_%s.tar.bz2", gdata->device_name,
				gdata->serial_number, gdata->timestamp);
	snprintf(gdata->tar_bz2_base_file_path,
				sizeof(gdata->tar_bz2_base_file_path),
				"%s/%s", DEFAULT_TEMP_DIR,
				gdata->tar_bz2_base_file);
#endif /* USE_TAR_BZ2_COMPRESS */
#ifdef USE_ZIP_COMPRESS
	snprintf(gdata->zip_base_file, sizeof(gdata->zip_base_file),
			"%s_%s_%s.zip", gdata->device_name,
			gdata->serial_number, gdata->timestamp);
	snprintf(gdata->zip_base_file_path, sizeof(gdata->zip_base_file_path),
			"%s/%s_%s_%s.zip", DEFAULT_TEMP_DIR, gdata->device_name,
					gdata->serial_number, gdata->timestamp);
#endif /* USE_ZIP_COMPRESS */
}

#ifdef USE_TAR_BZ2_COMPRESS
static int copy_regular_file(struct __global_data *gdata,
						struct __log_data *l_data_p)
{
	const char *lkey = "/";
	char dest_file_path[PATH_MAX];
	char *src_file_name;

	src_file_name = strrstr(l_data_p->log_path, lkey);
	if (src_file_name == NULL) {
		WARNING("cannot found llkey in log_path\n");
		return -1;
	}
	src_file_name++;
	snprintf(dest_file_path, sizeof(dest_file_path), "%s/%s",
					gdata->dest_logs_dir, src_file_name);
	return duplicate_file(l_data_p->log_path, dest_file_path);
}
#endif /* USE_TAR_BZ2_COMPRESS */

#ifdef USE_ZIP_COMPRESS
static int zip_copy_regular_file(struct __global_data *gdata,
						struct __log_data *l_data_p)
{
	const char *lkey = "/";
	char *src_file_name;

	src_file_name = strrstr(l_data_p->log_path, lkey);
	if (src_file_name == NULL) {
		WARNING("cannot found llkey in log_path\n");
		return -1;
	}
	src_file_name++;

	return zip_add_file(l_data_p->log_path, "", src_file_name);
}
#endif /* USE_ZIP_COMPRESS */

#ifdef USE_TAR_BZ2_COMPRESS
static int copy_directory(struct __global_data *gdata,
						struct __log_data *l_data_p)
{
	const char *lkey = "/";
	char dest_dir_path[PATH_MAX];
	char *src_dir_name;

	src_dir_name = strrstr(l_data_p->log_path, lkey);
	if (src_dir_name == NULL) {
		WARNING("cannot found llkey in log_path\n");
		return -1;
	}
	src_dir_name++;
	snprintf(dest_dir_path, sizeof(dest_dir_path), "%s/%s",
					gdata->dest_logs_dir, src_dir_name);
	return duplicate_directory(l_data_p->log_path, dest_dir_path);
}
#endif /* USE_TAR_BZ2_COMPRESS */

#ifdef USE_ZIP_COMPRESS
static int copy_zip_directory(struct __global_data *gdata,
						struct __log_data *l_data_p)
{
	const char *lkey = "/";
	char *src_dir_name;

	src_dir_name = strrstr(l_data_p->log_path, lkey);
	if (src_dir_name == NULL) {
		WARNING("cannot found llkey in log_path\n");
		return -1;
	}
	src_dir_name++;

	return duplicate_zip_directory(l_data_p->log_path, "", src_dir_name);
}
#endif /* USE_ZIP_COMPRESS */

static int copy_all_files_to_dest_dir(struct __global_data *gdata)
{
	int ret = 0;
	unsigned int i;
	struct __log_data *l_data_p;
	enum __file_type file_type;
	size_t log_path_len;

	for (i = 0; i < gdata->log_data_cnt; i++) {
		l_data_p = &gdata->log_data[i];
		log_path_len = strlen(l_data_p->log_path);
		if (l_data_p->log_path[log_path_len - 1] == '/')
			l_data_p->log_path[--log_path_len] = 0x0;

		ret = get_file_stat(l_data_p->log_path, &l_data_p->log_stat);
		if (ret != 0)
			continue;

		file_type = get_file_type(&l_data_p->log_stat);
		switch (file_type) {
		case FILE_TYPE_REGULAR_FILE:
#ifdef USE_TAR_BZ2_COMPRESS
			ret = copy_regular_file(gdata, l_data_p);
			if (ret == 0)
				INFO("copy file [%s] done\n",
							l_data_p->log_path);
			else
				WARNING("copy file [%s] failed\n",
							l_data_p->log_path);
#endif /* USE_TAR_BZ2_COMPRESS */
#ifdef USE_ZIP_COMPRESS
			ret = zip_copy_regular_file(gdata, l_data_p);
			if (ret == 0)
				INFO("copy file [%s] to zip done\n",
							l_data_p->log_path);
			else
				WARNING("copy file [%s] to zip failed\n",
							l_data_p->log_path);
#endif /* USE_ZIP_COMPRESS */
			break;
		case FILE_TYPE_DIRECTORY:
#ifdef USE_TAR_BZ2_COMPRESS
			ret = copy_directory(gdata, l_data_p);
			if (ret == 0)
				INFO("copy dir [%s] done\n",
							l_data_p->log_path);
			else
				WARNING("copy dir [%s] failed\n",
							l_data_p->log_path);
#endif /* USE_TAR_BZ2_COMPRESS */
#ifdef USE_ZIP_COMPRESS
			ret = copy_zip_directory(gdata, l_data_p);
			if (ret == 0)
				INFO("copy dir [%s] on zip done\n",
							l_data_p->log_path);
			else
				WARNING("copy dir [%s] on zip failed\n",
							l_data_p->log_path);
#endif /* USE_ZIP_COMPRESS */
			break;
		case FILE_TYPE_SYMBOLIC_LINK:
		case FILE_TYPE_UNKNOWN:
		case FILE_TYPE_SOCKET:
		case FILE_TYPE_BLOCK_DEVICE:
		case FILE_TYPE_CHAR_DEVICE:
		case FILE_TYPE_FIFO:
		default:
			NOTICE("file [%s] type is: %s, skipped\n",
					l_data_p->log_path,
					get_file_type_string(file_type));
			break;
		};
	}

	return ret;
}

static int duplicate_archive_file_to_usb_dev(struct __global_data *gdata)
{
	int ret = 0;
	uint32_t i;
	char usb_dev_dest_path[PATH_MAX];

	if (!gdata->usb_device_cnt)
		return 0;

	ret = conf_usbdev_mounted_paths(gdata);
	if (ret != 0)
		return ret;

	for (i = 0; i < gdata->usb_device_cnt; i++) {
		if (!strlen(gdata->usb_mount_path[i]))
			continue;

#ifdef USE_ZIP_COMPRESS
		memset(usb_dev_dest_path, 0x0, sizeof(usb_dev_dest_path));
		snprintf(usb_dev_dest_path, sizeof(usb_dev_dest_path), "%s/%s",
						gdata->usb_mount_path[i],
						gdata->zip_base_file);
		if (check_file_is_exist(usb_dev_dest_path) == 0)
			remove(usb_dev_dest_path);

		ret = duplicate_file(gdata->zip_base_file_path,
							usb_dev_dest_path);
		if (ret != 0)
			WARNING("copy archive from [%s] to [%s] failed\n",
				gdata->zip_base_file_path, usb_dev_dest_path);
		else
			INFO("copy archive from [%s] to [%s] completed\n",
				gdata->zip_base_file_path, usb_dev_dest_path);
#endif /* USE_ZIP_COMPRESS */
#ifdef USE_TAR_BZ2_COMPRESS
		memset(usb_dev_dest_path, 0x0, sizeof(usb_dev_dest_path));
		snprintf(usb_dev_dest_path, sizeof(usb_dev_dest_path), "%s/%s",
						gdata->usb_mount_path[i],
						gdata->tar_bz2_base_file);
		if (check_file_is_exist(usb_dev_dest_path) == 0)
			remove(usb_dev_dest_path);

		ret = duplicate_file(gdata->tar_bz2_base_file_path,
							usb_dev_dest_path);
		if (ret != 0)
			WARNING("copy archive from [%s] to [%s] failed\n",
						gdata->tar_bz2_base_file_path,
						usb_dev_dest_path);
		else
			INFO("copy archive from [%s] to [%s] completed\n",
						gdata->tar_bz2_base_file_path,
						usb_dev_dest_path);
#endif /* USE_TAR_BZ2_COMPRESS */
	}

	return 0;
}

static int duplicate_archive_file_to_internal(struct __global_data *gdata)
{
	int ret = 0;
	uint32_t i;
	char int_dest_path[PATH_MAX];

	if (!gdata->internal_path_cnt)
		return 0;

	for (i = 0; i < gdata->internal_path_cnt; i++) {
		if (check_file_is_exist(gdata->internal_path[i]) != 0)
			make_dir(gdata->internal_path[i]);

#ifdef USE_ZIP_COMPRESS
		memset(int_dest_path, 0x0, sizeof(int_dest_path));
		snprintf(int_dest_path, sizeof(int_dest_path), "%s/%s",
						gdata->internal_path[i],
						gdata->zip_base_file);
		if (check_file_is_exist(int_dest_path) == 0)
			remove(int_dest_path);

		ret = duplicate_file(gdata->zip_base_file_path, int_dest_path);
		if (ret != 0)
			WARNING("copy archive from [%s] to [%s] failed\n",
				gdata->zip_base_file_path, int_dest_path);
		else
			INFO("copy archive from [%s] to [%s] completed\n",
				gdata->zip_base_file_path, int_dest_path);
#endif /* USE_ZIP_COMPRESS */
#ifdef USE_TAR_BZ2_COMPRESS
		memset(int_dest_path, 0x0, sizeof(int_dest_path));
		snprintf(int_dest_path, sizeof(int_dest_path), "%s/%s",
						gdata->internal_path[i],
						gdata->tar_bz2_base_file);
		if (check_file_is_exist(int_dest_path) == 0)
			remove(int_dest_path);

		ret = duplicate_file(gdata->tar_bz2_base_file_path,
								int_dest_path);
		if (ret != 0)
			WARNING("copy archive from [%s] to [%s] failed\n",
						gdata->tar_bz2_base_file_path,
						int_dest_path);
		else
			INFO("copy archive from [%s] to [%s] completed\n",
						gdata->tar_bz2_base_file_path,
						int_dest_path);
#endif /* USE_TAR_BZ2_COMPRESS */
	}
	return 0;
}

static int collect_logs_func(struct __global_data *gdata)
{
	int ret = 0;

	ret = load_config_from_conf_file(gdata);
	if (ret != 0)
		return ret;

	generate_dest_logs_path(gdata);

#ifdef ENABLE_CONF_DUMP
	if (get_log_level() == LOG_DEBUG)
		dump_conf_config(gdata);
#endif /* ENABLE_CONF_DUMP */

#ifdef USE_TAR_BZ2_COMPRESS
	ret = make_dir(gdata->dest_logs_dir);
	if (ret != 0)
		return ret;

#endif /* USE_TAR_BZ2_COMPRESS */

#ifdef USE_ZIP_COMPRESS
	ret = zip_initialize(gdata);
	if (ret != 0)
		return ret;
#endif /* USE_ZIP_COMPRESS */

	ret = copy_all_files_to_dest_dir(gdata);
	if (ret != 0)
		return ret;

#ifdef USE_TAR_BZ2_COMPRESS
	ret = tar_bz2_compress_dir(gdata);
	if (ret != 0)
		return ret;

	INFO("archive tar_bz2 file: %s generation completed\n",
					gdata->tar_bz2_base_file);
#endif /* USE_TAR_BZ2_COMPRESS */

#ifdef USE_ZIP_COMPRESS
	ret = zip_uninitialize(gdata);
	if (ret != 0)
		return ret;

	INFO("archive zip file: %s generation completed\n",
						gdata->zip_base_file);
#endif /* USE_ZIP_COMPRESS */

	ret = duplicate_archive_file_to_usb_dev(gdata);
	if (ret != 0)
		return ret;

	ret = duplicate_archive_file_to_internal(gdata);
	if (ret != 0)
		return ret;

	return ret;
}

int main(int argc, char *argv[])
{
	struct __global_data gdata;
	int ret = 0;

	log_initialize(_LOG_INFO, _LOG_PRINTF | _LOG_FILE);

	init_default_gdata(&gdata);
	init_check_system_apps();

	get_options(&gdata, argc, argv);

	switch (gdata.action) {
	case APP_ACTION_VERSION:
		printf("%s\n", gdata.version);
		break;
	case APP_ACTION_RUN:
		INFO("start collect logs, time: %s\n", gdata.timestamp);
#ifdef USE_TAR_BZ2_COMPRESS
		DEBUG("libtar version: %s\n", get_libtar_version());
		DEBUG("libbz2 version: %s\n", get_libbz2_version());
#endif /* USE_TAR_BZ2_COMPRESS */
#ifdef USE_ZIP_COMPRESS
		DEBUG("libzip version: %s\n", get_zip_libzip_version());
#endif /* USE_ZIP_COMPRESS */
		ret = collect_logs_func(&gdata);
		INFO("execute log collector completed, ret: %d\n", ret);
		break;
	case APP_ACTION_USAGE:
	default:
		usage(&gdata);
		break;
	};
	log_uninitialize();
	return ret;
}
