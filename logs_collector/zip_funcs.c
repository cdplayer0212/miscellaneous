#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>

#include "logs_collector.h"
#include "common_funcs.h"
#include "logger.h"

#include <zip.h>

struct __zip_data {
	uint8_t is_initialized;
	zip_source_t *zip_src;
	zip_t *zipper;
	zip_error_t zip_err;
	zip_int32_t comp_fmt;
	zip_uint32_t comp_fmt_flags;
};

static struct __zip_data zip_data = {
	.is_initialized = 0,
};

const char *get_zip_libzip_version(void)
{
	return zip_libzip_version();
}

int zip_initialize(struct __global_data *gdata)
{
	int ret = 0;

	if (!zip_data.is_initialized) {
		if (check_file_is_exist(gdata->zip_base_file_path) == 0)
			remove(gdata->zip_base_file_path);

		memset(&zip_data, 0x0, sizeof(zip_data));
		zip_data.comp_fmt = ZIP_CM_DEFLATE;
		zip_data.comp_fmt_flags = 5;
		zip_error_init(&zip_data.zip_err);
		zip_data.zipper = zip_open(gdata->zip_base_file_path,
						ZIP_CREATE | ZIP_EXCL, &ret);
		if (zip_data.zipper == NULL) {
			zip_error_init_with_code(&zip_data.zip_err, ret);
			ERR("zip initial failed, %s\n",
				zip_error_strerror(&zip_data.zip_err));
			return -zip_data.zip_err.zip_err;
		}

		zip_data.is_initialized = 1;
	}

	return ret;
}

int zip_add_file(char *src_file, char *base_dir, char *zip_file_name)
{
	int ret = 0;
	char zip_dest_file[PATH_MAX];
	zip_int64_t zip_ret;

	if (!zip_data.is_initialized) {
		WARNING("zip not initialize, %s\n");
		return -EFAULT;
	}

	zip_data.zip_src = zip_source_file(zip_data.zipper, src_file, 0, -1);
	if (zip_data.zip_src == NULL) {
		WARNING("zip add file failed, %s\n",
				zip_error_strerror(&zip_data.zip_err));
		ret = -zip_data.zip_err.zip_err;
		zip_source_free(zip_data.zip_src);
		return ret;
	}

	memset(zip_dest_file, 0x0, sizeof(zip_dest_file));
	if (strlen(base_dir))
		snprintf(zip_dest_file, sizeof(zip_dest_file), "%s/%s",
						base_dir, zip_file_name);
	else
		snprintf(zip_dest_file, sizeof(zip_dest_file), "%s",
							zip_file_name);

	zip_ret = zip_file_add(zip_data.zipper, zip_dest_file,
					zip_data.zip_src,
					ZIP_FL_ENC_GUESS);
	if (zip_ret == -1) {
		ERR("failed, %s\n", zip_error_strerror(&zip_data.zip_err));
		ret = -zip_data.zip_err.zip_err;
		zip_source_free(zip_data.zip_src);
		return ret;
	}
	ret = zip_set_file_compression(zip_data.zipper, zip_ret,
						zip_data.comp_fmt,
						zip_data.comp_fmt_flags);
	if (ret != 0)
		WARNING("zip set file compression failed\n");

	return ret;
}

static int zip_create_directory(char *zip_dir_name)
{
	zip_int64_t zip_ret;
	int ret = 0;

	zip_ret = zip_dir_add(zip_data.zipper, zip_dir_name, ZIP_FL_ENC_GUESS);
	if (zip_ret == -1) {
		ERR("zip add dir failed, %s\n",
					zip_error_strerror(&zip_data.zip_err));
		ret = -zip_data.zip_err.zip_err;
		return ret;
	}
	ret = zip_set_file_compression(zip_data.zipper, zip_ret,
						zip_data.comp_fmt,
						zip_data.comp_fmt_flags);
	if (ret != 0)
		WARNING("zip set file compression failed\n");

	return ret;
}

int duplicate_zip_directory(char *src, char *base_dir, char *dest)
{
	DIR *d;
	struct dirent *dir;
	char tmp_src_path[PATH_MAX];
	char tmp_base_dir[PATH_MAX];
	struct stat tmp_src_stat;
	enum __file_type file_type;
	int ret = 0;

	d = opendir(src);
	if (d == NULL) {
		WARNING("open dir src: %s failed: %s(%d)\n", src,
							strerror(errno), errno);
		return -errno;
	}

	memset(tmp_base_dir, 0x0, sizeof(tmp_base_dir));
	if (strlen(base_dir))
		snprintf(tmp_base_dir, sizeof(tmp_base_dir), "%s/%s", base_dir,
									dest);
	else
		snprintf(tmp_base_dir, sizeof(tmp_base_dir), "%s", dest);

	zip_create_directory(tmp_base_dir);

	while ((dir = readdir(d)) != NULL) {
		if ((dir->d_name[0] == '.' && strlen(dir->d_name) == 1) ||
			(dir->d_name[0] == '.' && dir->d_name[1] == '.' &&
						strlen(dir->d_name) == 2))
			continue;

		snprintf(tmp_src_path, sizeof(tmp_src_path), "%s/%s", src,
								dir->d_name);

		get_file_stat(tmp_src_path, &tmp_src_stat);
		file_type = get_file_type(&tmp_src_stat);
		switch (file_type) {
		case FILE_TYPE_REGULAR_FILE:
			ret = zip_add_file(tmp_src_path, tmp_base_dir,
								dir->d_name);
			break;
		case FILE_TYPE_DIRECTORY:
			ret = duplicate_zip_directory(tmp_src_path,
						tmp_base_dir, dir->d_name);
			break;
		case FILE_TYPE_SYMBOLIC_LINK:
			break;
		case FILE_TYPE_UNKNOWN:
		case FILE_TYPE_SOCKET:
		case FILE_TYPE_BLOCK_DEVICE:
		case FILE_TYPE_CHAR_DEVICE:
		case FILE_TYPE_FIFO:
		default:
			continue;
			break;
		};
	};
	closedir(d);
	return ret;
}

int zip_uninitialize(struct __global_data *gdata)
{
	int ret = 0;

	if (zip_data.is_initialized) {
		ret = zip_close(zip_data.zipper);
		if (ret != 0) {
			ERR("zip uninitial failed, %s\n",
				zip_error_strerror(&zip_data.zip_err));
			return -zip_data.zip_err.zip_err;
		}
		zip_error_fini(&zip_data.zip_err);
		zip_data.is_initialized = 0;
	}
	return ret;
}
