#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <time.h>

#define DEFAULT_PRIO_VAL	LOG_INFO
#define DEFAULT_FAC_VAL		LOG_USER
#define DEFAULT_LOG_APP_NAME	"unknown"

#define ONE_KBYTES_CNT		1024

#define MAX_LOG_FILE_COUNT		5
#define MAX_LOG_FILE_NAME_LENGTH	64
#define MAX_LOG_FILE_SIZE		256 /* unit: Kbytes */
// #define DEFAULT_LOG_PATH		"/nvram/logs/persist_messages"
#define DEFAULT_LOG_PATH		"/tmp/persist_messages"

#define MAX_PPID_LENGTH			8
#define MAX_TYPE_NAME_LENGTH		10
#define MAX_LOG_NAME_LENGTH		32
#define MAX_DATE_LENGTH			26
#define MAX_MSG_LENGTH			256

enum {
	FUNC_HELP = 0,
	FUNC_NORMAL_FUNC = 1 << 0,
};

/* porting from syslog.h */
struct syslog_code {
	char *c_name;
	int c_val;
};

#define __INTERNAL_NOPRI	0x10
#define __INTERNAL_MARK		(LOG_NFACILITIES << 3)

static struct syslog_code prio_name[] = {
	{ "alert",   LOG_ALERT      },
	{ "crit",    LOG_CRIT       },
	{ "debug",   LOG_DEBUG      },
	{ "emerg",   LOG_EMERG      },
	{ "err",     LOG_ERR        },
	{ "error",   LOG_ERR        },         /* DEPRECATED */
	{ "info",    LOG_INFO       },
	{ "none",    __INTERNAL_NOPRI },         /* INTERNAL */
	{ "notice",  LOG_NOTICE     },
	{ "panic",   LOG_EMERG      },         /* DEPRECATED */
	{ "warn",    LOG_WARNING    },         /* DEPRECATED */
	{ "warning", LOG_WARNING    },
	{ NULL,      -1             }
};

static struct syslog_code fac_name[] = {
	{ "auth",     LOG_AUTH },
	{ "authpriv", LOG_AUTHPRIV },
	{ "cron",     LOG_CRON },
	{ "daemon",   LOG_DAEMON },
	{ "ftp",      LOG_FTP },
	{ "kern",     LOG_KERN },
	{ "lpr",      LOG_LPR },
	{ "mail",     LOG_MAIL },
	{ "mark",     __INTERNAL_MARK },          /* INTERNAL */
	{ "news",     LOG_NEWS },
	{ "security", LOG_AUTH },           /* DEPRECATED */
	{ "syslog",   LOG_SYSLOG },
	{ "user",     LOG_USER },
	{ "uucp",     LOG_UUCP },
	{ "local0",   LOG_LOCAL0 },
	{ "local1",   LOG_LOCAL1 },
	{ "local2",   LOG_LOCAL2 },
	{ "local3",   LOG_LOCAL3 },
	{ "local4",   LOG_LOCAL4 },
	{ "local5",   LOG_LOCAL5 },
	{ "local6",   LOG_LOCAL6 },
	{ "local7",   LOG_LOCAL7 },
	{ NULL, -1 }
};

struct __logs_file_info {
	int log_file_is_exist;
	char log_file_name[MAX_LOG_FILE_NAME_LENGTH];
	struct stat log_file_stat;
};

struct __info {
	int func;
	long log_size;
	int log_file_cnt;
	// char log_name[MAX_LOG_NAME_LENGTH];
	char *log_name;
	char localtime[MAX_DATE_LENGTH];
	int facility;
	char fac_name[MAX_TYPE_NAME_LENGTH];
	int priority;
	char prio_name[MAX_TYPE_NAME_LENGTH];
	// char msg[MAX_MSG_LENGTH];
	char *msg;
	struct __logs_file_info file_info[MAX_LOG_FILE_COUNT];
};

static struct option opts[] = {
	{ "help",       0, NULL, 'h' },
	{ "log_file",   1, NULL, 'l' },
	{ "log_size",   1, NULL, 's' },
	{ "log_cnt",    1, NULL, 'c' },
	{ "log_name",   1, NULL, 'n' },
	{ "facility",   1, NULL, 'f' },
	{ "priority",   1, NULL, 'p' },
	{ "msg",    1, NULL, 'm' },
};

static int debug_level;

int get_facility_name(int facility, char *name, size_t name_size)
{
	int i;

	for (i = 0; i < LOG_NFACILITIES; i++) {
		if (fac_name[i].c_val == facility) {
			strncpy(name, fac_name[i].c_name, name_size - 1);
			return 0;
		}
	}
	return -EINVAL;
}

int get_priority_name(int priority, char *name, size_t name_size)
{
	int i;

	for (i = 0; i < 16; i++) {
		if (prio_name[i].c_name == NULL)
			break;

		if (prio_name[i].c_val == priority) {
			strncpy(name, prio_name[i].c_name, name_size - 1);
			return 0;
		}
	}
	return -EINVAL;
}

int get_local_time_date(char *date, size_t date_size)
{
	time_t T;
	struct tm *local_tm;
	char mon_str[12][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	char week_str[7][4] = {
		"Sun", "Mon", "Tue", "Wed",
		"Thu", "Fri", "Sat"
	};

	T = time(NULL);
	local_tm = localtime(&T);
	snprintf(date, date_size, "%s %s %2.2d:%2.2d:%2.2d %4.4d",
		week_str[local_tm->tm_wday % 7],
		mon_str[local_tm->tm_mon % 12],
		local_tm->tm_hour,
		local_tm->tm_min,
		local_tm->tm_sec,
		local_tm->tm_year + 1900);

	return 0;
}

pid_t get_parent_pid(void)
{
	return getppid();
}

static int parser_argv(int argc, char *argv[], struct __info *info)
{
	int ret = 0;
	int c;
	long long_tmp;
	const char *optstring = "hl:s:c:n:f:p:m:";

	while ((c = getopt_long(argc, argv, optstring, opts, NULL)) != -1) {
		switch (c) {
		case 'h':
			info->func = FUNC_HELP;
			break;
		case 'l':
			memset(info->file_info[0].log_file_name, 0x0,
				sizeof(info->file_info[0].log_file_name));
			strncpy(info->file_info[0].log_file_name, optarg,
				sizeof(info->file_info[0].log_file_name) - 1);
			break;
		case 's':
			long_tmp = strtol(optarg, NULL, 10);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX) {
				info->func = FUNC_HELP;
				ret = -errno;
				goto error_out;
			} else {
				info->log_size = long_tmp * ONE_KBYTES_CNT;
			}
			break;
		case 'c':
			long_tmp = strtol(optarg, NULL, 10);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX) {
				info->func = FUNC_HELP;
				ret = -errno;
				goto error_out;
			} else {
				if (long_tmp > MAX_LOG_FILE_COUNT ||
							long_tmp <= 0) {
					printf("log file count(%ld) over max "
						"val, use default value(%d)\n",
						long_tmp, MAX_LOG_FILE_COUNT);
					info->log_file_cnt = MAX_LOG_FILE_COUNT;
				} else {
					info->log_file_cnt = long_tmp;
				}
			}
			break;
		case 'n':
			info->log_name = optarg;
			break;
		case 'f':
			long_tmp = strtol(optarg, NULL, 10);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX) {
				info->func = FUNC_HELP;
				ret = -errno;
				goto error_out;
			} else {
				info->facility = LOG_FAC((int)long_tmp);
			}
			break;
		case 'p':
			long_tmp = strtol(optarg, NULL, 10);
			if (long_tmp == LONG_MIN || long_tmp == LONG_MAX) {
				info->func = FUNC_HELP;
				ret = -errno;
				goto error_out;
			} else
				info->priority = LOG_PRI((int)long_tmp);

			break;
		case 'm':
			info->msg = optarg;
			break;
		case '?':
		default:
			info->func = FUNC_HELP;
			break;
		}
	}
error_out:
	return ret;
}

void info_initialize(struct __info *info)
{
	memset(info, 0x0, sizeof(struct __info));

	info->func = FUNC_NORMAL_FUNC;
	info->log_size = -1;
	info->log_file_cnt = -1;
	info->facility = -1;
	info->priority = -1;
}

static int check_and_update_default_val(struct __info *info)
{
	int i;

	if (info->log_size == -1)
		info->log_size = MAX_LOG_FILE_SIZE * ONE_KBYTES_CNT;

	if (info->log_file_cnt == -1)
		info->log_file_cnt = MAX_LOG_FILE_COUNT;

	if (info->facility == -1)
		info->facility = DEFAULT_FAC_VAL;

	if (!strlen(info->fac_name))
		get_facility_name(info->facility, info->fac_name,
						sizeof(info->fac_name));

	if (info->priority == -1)
		info->priority = DEFAULT_PRIO_VAL;

	if (!strlen(info->prio_name))
		get_priority_name(info->priority, info->prio_name,
						sizeof(info->prio_name));

	if (info->log_name == NULL)
		info->log_name = DEFAULT_LOG_APP_NAME;

	if (strlen(info->file_info[0].log_file_name) == 0)
		strncpy(info->file_info[0].log_file_name, DEFAULT_LOG_PATH,
			sizeof(info->file_info[0].log_file_name) - 1);

	if (info->log_file_cnt == 1)
		return 0;

	if (info->log_file_cnt == 2) {
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wformat-truncation"
		if (strlen(info->file_info[0].log_file_name) >
				MAX_LOG_FILE_NAME_LENGTH - 5) {
			printf("log file name: %s over limition: %d, exit\n",
				info->file_info[0].log_file_name,
				MAX_LOG_FILE_NAME_LENGTH - 5);
			return -EINVAL;
		}
		snprintf(info->file_info[1].log_file_name,
			sizeof(info->file_info[1].log_file_name),
			"%s.old",
			info->file_info[0].log_file_name);
//#pragma GCC diagnostic pop
	}
	if (info->log_file_cnt > 2) {
		for (i = 1; i < info->log_file_cnt; i++)
			snprintf(info->file_info[i].log_file_name,
				sizeof(info->file_info[i].log_file_name),
				"%s.%d",
				info->file_info[0].log_file_name, i);
	}
	return 0;
}

static int get_log_files_stat(struct __info *info)
{
	int i;
	int ret = 0;
	struct __logs_file_info *f_info;

	for (i = 0; i < info->log_file_cnt; i++) {
		f_info = &info->file_info[i];
		ret = stat(f_info->log_file_name,
			&f_info->log_file_stat);
		if (ret == 0)
			f_info->log_file_is_exist = 1;
		else
			f_info->log_file_is_exist = 0;
	}
	return ret;
}

/* return 0: no need to rotate, 1: rotate is required */
static int log_file_sould_exec_rotate(struct __info *info)
{
	if (!info->file_info[0].log_file_is_exist)
		return 0;

	if (info->log_size < info->file_info[0].log_file_stat.st_size)
		return 1;

	return 0;
}

static int log_file_rotate(struct __info *info)
{
	int i;
	struct __logs_file_info *f_info = NULL;
	struct __logs_file_info *f_prev_info = NULL;

	for (i = info->log_file_cnt - 1; i >= 0; i--) {
		f_info = &info->file_info[i];
		if (i == info->log_file_cnt && f_info->log_file_is_exist) {
			remove(f_info->log_file_name);
			f_prev_info = f_info;
			continue;
		}
		if (f_info->log_file_is_exist) {
			rename(f_info->log_file_name,
				f_prev_info->log_file_name);
		}
		f_prev_info = f_info;
	}
	return 0;
}

static int store_new_log(struct __info *info)
{
	int ret = 0;
	int fd;
	int ppid;
	struct __logs_file_info *f_info;
	char *buff = NULL;
	size_t buff_size;

	f_info = &info->file_info[0];
	buff_size = MAX_DATE_LENGTH + MAX_TYPE_NAME_LENGTH +
		MAX_TYPE_NAME_LENGTH + MAX_LOG_NAME_LENGTH +
		MAX_PPID_LENGTH + MAX_MSG_LENGTH + 6;
	buff = calloc(1, buff_size);
	if (buff == NULL) {
		printf("memory calloc failed, %s\n", strerror(errno));
		return -errno;
	}
	get_local_time_date(info->localtime, sizeof(info->localtime));
	/*
	 * get_facility_name(info->facility, info->fac_name,
	 *					sizeof(info->fac_name));
	 *get_priority_name(info->priority, info->prio_name,
	 *					sizeof(info->prio_name));
	 */
	ppid = get_parent_pid();
	snprintf(buff, buff_size, "%s %s.%s %s[%d]: %s\n",
		info->localtime, info->fac_name, info->prio_name,
		info->log_name, ppid, info->msg);
	fd = open(f_info->log_file_name, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (fd == -1) {
		printf("log file: open %s failed, %s\n", f_info->log_file_name,
							strerror(errno));
		free(buff);
		return -errno;
	}
	ret = write(fd, buff, strlen(buff));
	if (ret == -1) {
		printf("log file: write %s failed, %s\n", f_info->log_file_name,
				strerror(errno));
		ret = -errno;
	}
	free(buff);
	fsync(fd);
	close(fd);
	return ret;
}

static void dump_info(struct __info *info)
{
	int i;

	if (debug_level == 0)
		return;

	printf("  func: %d\n", info->func);
	printf("  log_size: %ld\n", info->log_size);
	printf("  log_file_cnt: %d\n", info->log_file_cnt);
	printf("  log_name: %s\n", info->log_name);
	printf("  localtime: %s\n", info->localtime);
	printf("  facility: %d\n", info->facility);
	printf("  fac_name: %s\n", info->fac_name);
	printf("  priority: %d\n", info->priority);
	printf("  prio_name: %s\n", info->prio_name);
	printf("  msg: %s\n", info->msg);
	for (i = 0; i < info->log_file_cnt; i++) {
		printf("  file_info[%d]\n", i);
		printf("    log_file_is_exist: %d\n",
				info->file_info[i].log_file_is_exist);
		printf("    log_file_name: %s\n",
				info->file_info[i].log_file_name);
		printf("    log_file_stat.st_size: %ld\n",
				info->file_info[i].log_file_stat.st_size);
	}
}

static int execute_funcs(struct __info *info)
{
	int ret;
	int rotate_is_required;

	check_and_update_default_val(info);
	get_log_files_stat(info);
	dump_info(info);
	rotate_is_required = log_file_sould_exec_rotate(info);
	if (rotate_is_required)
		ret = log_file_rotate(info);

	ret = store_new_log(info);
	return ret;
}

static void func_usage(void)
{
	char f_tmp[MAX_TYPE_NAME_LENGTH];
	char p_tmp[MAX_TYPE_NAME_LENGTH];

	memset(f_tmp, 0x0, sizeof(f_tmp));
	get_facility_name(DEFAULT_FAC_VAL, f_tmp, sizeof(f_tmp));
	memset(p_tmp, 0x0, sizeof(p_tmp));
	get_priority_name(DEFAULT_PRIO_VAL, p_tmp, sizeof(p_tmp));
	printf("askey persist logger, save logs to NVRAM direcory "
		"for furture debug\n"
		"  --log_file, -l <log file path>:\n"
		"    specify where the log file should be stored,\n"
		"    default is: %s\n"
		"    please use absolute path\n"
		"  --log_name, -n <app_name>:\n"
		"    Specifies the name of the app that should be logged\n"
		"    default: %s\n"
		"  --log_size, -s <log file size>:\n"
		"    specify the max log file size, default: %d Kbytes\n"
		"  --log_cnt, -c <log file count>:\n"
		"    specify how many the log file exist in system, default is: %d\n"
		"  --facility, -f <facility>:\n"
		"    specify the facility type, default: %s(%d)\n"
		"  --priority, -p <priority>:\n"
		"    specify the priority type, default: %s(%d)\n"
		"  --msg, -m <log message body>: the message body\n"
		"  --help, -h: show usage message\n",
		DEFAULT_LOG_PATH, DEFAULT_LOG_APP_NAME, MAX_LOG_FILE_SIZE,
		MAX_LOG_FILE_COUNT, f_tmp, DEFAULT_FAC_VAL, p_tmp,
		DEFAULT_PRIO_VAL);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	static struct __info info;

	debug_level = 0;
	info_initialize(&info);
	parser_argv(argc, argv, &info);
	switch (info.func) {
	case FUNC_HELP:
	default:
		func_usage();
		break;
	case FUNC_NORMAL_FUNC:
		execute_funcs(&info);
		break;
	};
	return ret;
}
