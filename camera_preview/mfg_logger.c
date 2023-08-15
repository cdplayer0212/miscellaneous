#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

#include "mfg_logger.h"

#define DEFAULT_LOGGER_FILE	"/tmp/camera_preview.log"
#define MAX_LOG_SIZE_LIMIT	(32 * 1024 * 1024)
static unsigned long size_check_cnt;

static u_int32_t default_log_level = MFG_LOG_INFO;
static u_int32_t default_log_type = MFG_LOG_PRINTF;

static const char *logger_string[8] = {
	"EMERG",
	"ALERT",
	"CRIT",
	"ERR",
	"WARN",
	"NOTICE",
	"INFO",
	"DEBUG",
};

void log_initialize(void)
{
	struct stat logstat;

	size_check_cnt = 0;
	if (access(DEFAULT_LOGGER_FILE, F_OK) == 0) {
		memset(&logstat, 0x0, sizeof(logstat));
		if (stat(DEFAULT_LOGGER_FILE, &logstat) == 0)
			size_check_cnt = logstat.st_size;
	}
	if (default_log_type | MFG_LOG_SYSLOG) {
		printf("open syslog function\n");
		openlog("camera_preview", LOG_PID | LOG_CONS, LOG_USER);
	}
}

void log_uninitialize(void)
{
	if (default_log_type | MFG_LOG_SYSLOG) {
		printf("close syslog function\n");
		closelog();
	}
}

void set_log_level(enum logger_level level)
{
	default_log_level = level;
}

u_int32_t get_log_level(void)
{
	return default_log_level;
}

int mv_log_to_old_file(void)
{
	char old_file_name[32];

	memset(old_file_name, 0x0, sizeof(old_file_name));
	snprintf(old_file_name, sizeof(old_file_name), "%s.old",
						DEFAULT_LOGGER_FILE);
	if (access(old_file_name, F_OK) == 0)
		remove(old_file_name);

	return rename(DEFAULT_LOGGER_FILE, old_file_name);
}

int mfg_logger(enum logger_level level, char *fmt, ...)
{
	int ret = 0;
	va_list arg_ptr;
	time_t now;
	struct tm *now_tm;
	FILE *fd;

	if (level > default_log_level)
		return 0;

	now = time(NULL);
	now_tm = localtime(&now);
	if (default_log_type & MFG_LOG_PRINTF) {
		ret = fprintf(stdout, "[%02d:%02d:%02d]%s: ",
			now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec,
			logger_string[level]);
		if (ret < 0)
			return ret;

		va_start(arg_ptr, fmt);
		ret = vfprintf(stdout, fmt, arg_ptr);
		va_end(arg_ptr);
		if (ret < 0)
			return ret;
	}
	if (default_log_type & MFG_LOG_SYSLOG) {
		va_start(arg_ptr, fmt);
		vsyslog(level, fmt, arg_ptr);
		va_end(arg_ptr);
	}
	if (default_log_type & MFG_LOG_FILE) {
		if (size_check_cnt > MAX_LOG_SIZE_LIMIT &&
				access(DEFAULT_LOGGER_FILE, F_OK) == 0) {
			mv_log_to_old_file();
			size_check_cnt = 0;
		}
		fd = fopen(DEFAULT_LOGGER_FILE, "a+");
		if (fd == NULL)
			return -errno;

		ret = fprintf(fd, "[%02d:%02d:%02d]%s: ", now_tm->tm_hour,
					now_tm->tm_min,	now_tm->tm_sec,
					logger_string[level]);
		if (ret < 0) {
			fclose(fd);
			return ret;
		}
		size_check_cnt += ret;
		va_start(arg_ptr, fmt);
		ret = vfprintf(fd, fmt, arg_ptr);
		if (ret >= 0)
			size_check_cnt += ret;

		va_end(arg_ptr);
		fsync(fileno(fd));
		fclose(fd);
		if (ret < 0)
			return ret;
	}

	return 0;
}
