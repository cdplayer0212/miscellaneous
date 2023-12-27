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
#include <stdint.h>

#include "logger.h"

// #define ENABLE_LOGS_ROTATION

#ifdef ENABLE_LOGS_ROTATION
static unsigned long size_check_cnt;
#endif /* ENABLE_LOGS_ROTATION */

static uint32_t default_log_level = _LOG_INFO;
static uint32_t default_log_type = _LOG_PRINTF;

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

void log_initialize(uint32_t log_level, uint32_t log_type)
{
#ifdef ENABLE_LOGS_ROTATION
	struct stat logstat;

	size_check_cnt = 0;
#endif /* ENABLE_LOGS_ROTATION */
	default_log_level = log_level;
	default_log_type = log_type;


	if (default_log_type & _LOG_FILE) {
		if (access(DEFAULT_LOGGER_FILE, F_OK) == 0) {
#ifdef ENABLE_LOGS_ROTATION
			memset(&logstat, 0x0, sizeof(logstat));
			if (stat(DEFAULT_LOGGER_FILE, &logstat) == 0)
				size_check_cnt = logstat.st_size;
#else /* ENABLE_LOGS_ROTATION */
			remove(DEFAULT_LOGGER_FILE);
#endif /* ENABLE_LOGS_ROTATION */
		}
	}
	if (default_log_type & _LOG_SYSLOG) {
		printf("open syslog function\n");
		openlog("logs_collector", LOG_PID | LOG_CONS, LOG_USER);
	}
}

void log_uninitialize(void)
{
	if (default_log_type & _LOG_SYSLOG) {
		printf("close syslog function\n");
		closelog();
	}
}

void set_log_level(enum logger_level level)
{
	default_log_level = level;
}

uint32_t get_log_level(void)
{
	return default_log_level;
}

void set_log_type(enum logger_type type)
{
	default_log_type = type;
}

uint32_t get_log_type(void)
{
	return default_log_level;
}

#ifdef ENABLE_LOGS_ROTATION
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
#endif /* ENABLE_LOGS_ROTATION */

int logger(enum logger_level level, char *fmt, ...)
{
	int ret = 0;
	va_list arg_ptr;
	time_t now;
	struct tm *now_tm;
	FILE *fd;
	int fd_tmp;

	if (level > default_log_level)
		return 0;

	now = time(NULL);
	now_tm = localtime(&now);
	if (default_log_type & _LOG_PRINTF) {
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
	if (default_log_type & _LOG_SYSLOG) {
		va_start(arg_ptr, fmt);
		vsyslog(level, fmt, arg_ptr);
		va_end(arg_ptr);
	}
	if (default_log_type & _LOG_FILE) {
#ifdef ENABLE_LOGS_ROTATION
		if (size_check_cnt > MAX_LOG_SIZE_LIMIT &&
				access(DEFAULT_LOGGER_FILE, F_OK) == 0) {
			mv_log_to_old_file();
			size_check_cnt = 0;
		}
#endif /* ENABLE_LOGS_ROTATION */
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
#ifdef ENABLE_LOGS_ROTATION
		size_check_cnt += ret;
#endif /* ENABLE_LOGS_ROTATION */
		va_start(arg_ptr, fmt);
		ret = vfprintf(fd, fmt, arg_ptr);
#ifdef ENABLE_LOGS_ROTATION
		if (ret >= 0)
			size_check_cnt += ret;
#endif /* ENABLE_LOGS_ROTATION */
		va_end(arg_ptr);
		fflush(fd);
		fd_tmp = fileno(fd);
		if (fd_tmp != -1)
			fsync(fd_tmp);

		fclose(fd);
		if (ret < 0)
			return ret;
	}

	return 0;
}
