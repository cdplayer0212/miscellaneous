#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#include "logger.h"
#include "common_funcs.h"

#define DATA_MAX_SIZE			(4 * 1024 * 1024)

#define UNKNOWN_STRING			"UNKNOWN"

static uint32_t system_app_cnt;

struct __system_app_data {
	bool is_exist;
	const char *app_name;
	char app_path[PATH_MAX];
};

struct __system_app_data app_data[] = {
	{
		.app_name = "logs_collector",
	},
};

int init_check_system_apps(void)
{
	int i;
	char split_char[2] = ":";
	char *env_path;
	char *strstr_path;
	char app_real_path[PATH_MAX - 1];
	char env_data[80];

	env_path = getenv("PATH");
	if (env_path == NULL) {
		WARNING("get_env failed, %s\n", strerror(errno));
		return -errno;
	}

	/* initialize */
	system_app_cnt = sizeof(app_data) / sizeof(app_data[0]);
	memset(env_data, 0x0, sizeof(env_data));
	strncpy(env_data, env_path, sizeof(env_data) - 1);
	for (i = 0; i < system_app_cnt; i++) {
		app_data[i].is_exist = false;
		memset(app_data[i].app_path, 0x0, sizeof(app_data[i].app_path));
	}

	strstr_path = strtok(env_data, split_char);
	while (strstr_path != NULL) {
		for (i = 0; i < system_app_cnt; i++) {
			memset(app_real_path, 0x0, sizeof(app_real_path));
			snprintf(app_real_path, sizeof(app_real_path), "%s/%s",
				strstr_path, app_data[i].app_name);
			if (access(app_real_path, X_OK) == 0) {
				app_data[i].is_exist = true;
				strncpy(app_data[i].app_path, app_real_path,
					sizeof(app_data[i].app_path));
			}
		}
		strstr_path = strtok(NULL, split_char);
	}


	for (i = 0; i < system_app_cnt; i++)
		DEBUG("app info: name: %s, is exist: %d\n",
				app_data[i].app_name, app_data[i].is_exist);

	return 0;
}

char *get_system_app_path(char *app)
{
	int i;

	for (i = 0; i < system_app_cnt; i++) {
		if (strncmp(app_data[i].app_name, app,
					strlen(app_data[i].app_name)) == 0 &&
					app_data[i].is_exist == true)
			return app_data[i].app_path;
	}
	DEBUG("cannot found the system cmd: %s\n", app);
	return NULL;
}

int do_popen_cmd(char *cmd, char *data, ssize_t data_size)
{
	FILE *fp;
	size_t fread_ret = 0;

	fp = popen(cmd, "r");
	if (fp == NULL)
		return -errno;

	if (data == NULL || data_size == 0) {
		DEBUG("CMD: %s\n", cmd);
		return pclose(fp);
	}

	fread_ret = fread(data, data_size, 1, fp);
	DEBUG("CMD: %s, FREAD: [%s], ret: %ld\n", cmd, data, fread_ret);
	return pclose(fp);
}

int check_file_is_exist(char *file)
{
	if (access(file, F_OK) == 0)
		return 0;

	return -errno;
}

int get_file_stat(char *file, struct stat *file_stat)
{
	int ret;

	if (file == NULL)
		return -EBADF;

	memset(file_stat, 0x0, sizeof(struct stat));
	ret = stat((const char *)file, file_stat);
	if (ret != 0) {
		WARNING("get %s stat failed, %s\n", file, strerror(errno));
		return -errno;
	}

	return 0;
}

int get_symbolic_link_stat(char *file, struct stat *file_stat)
{
	int ret;

	if (file == NULL)
		return -EBADF;

	memset(file_stat, 0x0, sizeof(struct stat));
	ret = lstat((const char *)file, file_stat);
	if (ret != 0) {
		WARNING("get %s lstat failed, %s\n", file, strerror(errno));
		return -errno;
	}

	return 0;
}

enum __file_type get_file_type(struct stat *file_stat)
{
	enum __file_type ret = FILE_TYPE_UNKNOWN;

	switch (file_stat->st_mode & S_IFMT) {
	case S_IFBLK:
		ret = FILE_TYPE_BLOCK_DEVICE;
		break;
	case S_IFCHR:
		ret = FILE_TYPE_CHAR_DEVICE;
		break;
	case S_IFDIR:
		ret = FILE_TYPE_DIRECTORY;
		break;
	case S_IFIFO:
		ret = FILE_TYPE_FIFO;
		break;
	case S_IFLNK:
		ret = FILE_TYPE_SYMBOLIC_LINK;
		break;
	case S_IFREG:
		ret = FILE_TYPE_REGULAR_FILE;
		break;
	case S_IFSOCK:
		ret = FILE_TYPE_SOCKET;
		break;
	default:
		ret = FILE_TYPE_UNKNOWN;
		break;
	}
	return ret;
}

const char *get_file_type_string(enum __file_type type)
{
	const char *type_string[FILE_TYPE_LAST] = {
		"UNKNOWN",
		"REGULAR_FILE",
		"DIRECTORY",
		"SYMBOLIC_LINK",
		"SOCKET",
		"BLOCK_DEVICE",
		"CHAR_DEVICE",
		"FIFO",
	};

	if (type >= FILE_TYPE_UNKNOWN && type < FILE_TYPE_LAST)
		return type_string[type];

	return NULL;
}

static int get_strtol_value(char *str, long *val, int base)
{
	long ret;

	ret = strtol(str, NULL, base);
	if (ret == LONG_MAX || ret == LONG_MIN)
		return -errno;

	*val = ret;
	return 0;
}

int hex_string_to_long(char *str, long *val)
{
	return get_strtol_value(str, val, 16);
}

int dec_string_to_long(char *str, long *val)
{
	return get_strtol_value(str, val, 10);
}

int check_string_is_same(char *str1, char *str2)
{
	int ret;
	size_t str1_len;
	size_t str2_len;

	str1_len = strlen(str1);
	str2_len = strlen(str2);
	if (str1_len != str2_len)
		return 0;

	ret = strncmp(str1, str2, str1_len);
	if (ret != 0)
		return 0;

	return 1;
}

void remove_string_eol(char *str)
{
	size_t str_len;

	if (str == NULL)
		return;

	str_len = strlen(str);
	if (str_len < 1)
		return;

	if (str[str_len - 1] == '\n' || str[str_len - 1] == '\r')
		str[--str_len] = 0x0;

	if (str_len < 1)
		return;

	if (str[str_len - 1] == '\n' || str[str_len - 1] == '\r')
		str[--str_len] = 0x0;
}

int get_iso8601_timestamp(char *datetime, size_t len)
{
	time_t t;
	struct tm *tm;
	unsigned int utc_hour;
	unsigned int utc_min;
	unsigned int off = 0;

	t = time(NULL);
	if (t == ((time_t) -1))
		return -errno;

	tm = localtime(&t);

	off += snprintf(datetime, len, "%4d%02d%02dT%02d%02d%02d",
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec);

	if (tm->tm_gmtoff == 0) {
		off += snprintf(datetime + off, len - off, "Z");
	} else {
		utc_hour = abs(tm->tm_gmtoff) / 60 / 60;
		utc_min = (abs(tm->tm_gmtoff) - (utc_hour * 60 * 60)) / 60;
		if (tm->tm_gmtoff > 0)
			off += snprintf(datetime + off, len - off, "+");
		else
			off += snprintf(datetime + off, len - off, "-");

		off += snprintf(datetime + off, len - off,
					"%2.2d%2.2d", utc_hour, utc_min);
	}
	return 0;
}

int read_data_from_file(char *filename, void *val, size_t val_len)
{
	int fd;
	int ret;

	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return -errno;

	ret = read(fd, val, val_len - 1);
	close(fd);
	if (ret < 0)
		return -errno;

	return 0;
}

int get_device_name(char *name, size_t name_len)
{
	int ret;

	ret = read_data_from_file(DEVICE_NAME_PATH, name, name_len);
	if (ret == 0)
		remove_string_eol(name);
	else
		strncpy(name, UNKNOWN_STRING, name_len);

	return ret;
}

int get_device_sn(char *sn, size_t sn_len)
{
	int ret;

	ret = read_data_from_file(DEVICE_SN_PATH, sn, sn_len);
	if (ret == 0)
		remove_string_eol(sn);
	else
		strncpy(sn, UNKNOWN_STRING, sn_len);

	return ret;
}

char *strrstr(char *haystack, const char *needle)
{
	char *r = NULL;
	char *p;

	if (!needle[0])
		return (char *)haystack + strlen(haystack);

	while (1) {
		p = strstr(haystack, needle);
		if (p == NULL)
			return r;
		r = p;
		haystack = p + 1;
	}
}

static int __make_dir(char *dirpath)
{
	DIR *dir;
	int ret;

	dir = opendir(dirpath);
	if (dir != NULL) {
		closedir(dir);
		return 0;
	}
	ret = mkdir(dirpath, 0755);
	if (ret == -1) {
		WARNING("mkdir failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}
	return 0;
}

int make_dir(char *dirpath)
{
	unsigned int dirpath_len;
	char priv_dirpath[PATH_MAX];
	int i;
	int ret = 0;

	if (dirpath == NULL)
		return -EINVAL;

	dirpath_len = strlen(dirpath);
	if (dirpath_len == 0)
		return -EINVAL;

	memset(priv_dirpath, 0x0, sizeof(priv_dirpath));
	priv_dirpath[0] = dirpath[0];
	for (i = 1; i < dirpath_len; i++) {
		if (isascii(dirpath[i]) == 0) {
			ret = -EINVAL;
			break;
		}

		if (dirpath[i] == '/') {
			ret = __make_dir(priv_dirpath);
			if (ret != 0)
				return ret;
		}

		priv_dirpath[i] = dirpath[i];
	}
	if (priv_dirpath[strlen(priv_dirpath) - 1] != '/')
		ret = __make_dir(priv_dirpath);

	return ret;
}

int duplicate_file(char *src, char *dest)
{
	int fd_src;
	int fd_dest;
	unsigned long remain;
	unsigned long buff_size;
	struct stat src_stat;
	int ret = 0;
	int ret_r;
	int ret_w;
	unsigned char *buff;

	if (src != NULL && dest != NULL && (strlen(src) == strlen(dest))) {
		if (strncmp(src, dest, strlen(src)) == 0) {
			WARNING("write to same place, exit\n");
			ret = -1;
			goto error_exit;
		}
	}

	fd_src = open(src, O_RDONLY);
	if (fd_src == -1) {
		WARNING("open src: %s failed: %s(%d)\n", src, strerror(errno),
									errno);
		ret = -errno;
		goto error_exit;
	}
	ret = fstat(fd_src, &src_stat);
	if (ret == -1) {
		WARNING("get src: %s stat failed: %s(%d)\n", src,
							strerror(errno), errno);
		ret = -errno;
		goto error_close_src_fd;
	}

	fd_dest = open(dest, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
	if (fd_dest == -1) {
		WARNING("open dest: %s failed: %s(%d)\n", dest, strerror(errno),
									errno);
		ret = -errno;
		goto error_close_src_fd;
	}

	buff = malloc(DATA_MAX_SIZE);
	if (buff == NULL) {
		WARNING("malloc memory(%d) failed: %s(%d)\n", DATA_MAX_SIZE,
							strerror(errno), errno);
		ret = -errno;
		goto error_close_dest_fd;
	}
	remain = src_stat.st_size;
	while (remain > 0) {
		if (remain > DATA_MAX_SIZE)
			buff_size = DATA_MAX_SIZE;
		else
			buff_size = remain;

		ret_r = read(fd_src, buff, buff_size);
		if (ret_r == -1) {
			WARNING("read data(%ld) failed: %s(%d)\n",
					buff_size, strerror(errno), errno);
			ret = -errno;
			goto error_free_mem;
		}
		buff_size = ret_r;
		ret_w = write(fd_dest, buff, buff_size);
		if (ret_w == -1) {
			WARNING("write data(%ld) failed: %s(%d)\n",
				buff_size, strerror(errno), errno);
			ret = -errno;
			goto error_free_mem;
		}
		remain -= buff_size;
	}
	ret = 0;
	fsync(fd_dest);
error_free_mem:
	free(buff);
error_close_dest_fd:
	close(fd_dest);
error_close_src_fd:
	close(fd_src);

error_exit:
	return ret;
}

int duplicate_directory(char *src, char *dest)
{
	DIR *d;
	struct dirent *dir;
	char tmp_src_path[PATH_MAX];
	char tmp_dest_path[PATH_MAX];
	struct stat tmp_src_stat;
	enum __file_type file_type;
	int ret = 0;

	d = opendir(src);
	if (d == NULL) {
		WARNING("open dir src: %s failed: %s(%d)\n", src,
							strerror(errno), errno);
		return -errno;
	}
	if (check_file_is_exist(dest) != 0)
		make_dir(dest);

	while ((dir = readdir(d)) != NULL) {
		if ((dir->d_name[0] == '.' && strlen(dir->d_name) == 1) ||
			(dir->d_name[0] == '.' && dir->d_name[1] == '.' &&
						strlen(dir->d_name) == 2))
			continue;

		snprintf(tmp_src_path, sizeof(tmp_src_path), "%s/%s", src,
								dir->d_name);
		snprintf(tmp_dest_path, sizeof(tmp_dest_path), "%s/%s", dest,
								dir->d_name);
		get_file_stat(tmp_src_path, &tmp_src_stat);
		file_type = get_file_type(&tmp_src_stat);
		switch (file_type) {
		case FILE_TYPE_REGULAR_FILE:
			ret = duplicate_file(tmp_src_path, tmp_dest_path);
			break;
		case FILE_TYPE_DIRECTORY:
			ret = duplicate_directory(tmp_src_path, tmp_dest_path);
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

int get_real_path(char *path, char *real_path, size_t real_path_len)
{
	int ret = 0;
	char *path_p;
	enum __file_type file_type;
	struct stat path_stat;
	char chk_path[PATH_MAX];

	memset(chk_path, 0x0, sizeof(chk_path));
	snprintf(chk_path, sizeof(chk_path), "%s", path);
	do {
		path_p = realpath(chk_path, NULL);
		if (path_p == NULL) {
			ret = -errno;
			break;
		}
		memset(&path_stat, 0x0, sizeof(path_stat));
		ret = stat(path_p, &path_stat);
		if (ret != 0) {
			free(path_p);
			ret = -errno;
			break;
		}
		file_type = get_file_type(&path_stat);
		if (file_type != FILE_TYPE_SYMBOLIC_LINK) {
			strncpy(real_path, path_p, real_path_len - 1);
			free(path_p);
			break;
		}
		INFO("get %s, type: %s\n", path_p,
					get_file_type_string(file_type));
		memset(chk_path, 0x0, sizeof(chk_path));
		snprintf(chk_path, sizeof(chk_path), "%s", path_p);
		free(path_p);
		continue;
	} while (0);
	INFO("get real path: %s\n", real_path);
	return ret;
}
