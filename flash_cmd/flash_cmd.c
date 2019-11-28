#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<unistd.h>
#include	<errno.h>


#define MAX_ENV_VALUE_LEN	(128 * 1024)
#define PATH_ENV		"PATH"
#define MAX_PARTITION_CMD	64
#define MAX_PARTITION_CMD_LEN	80
#define MAX_PARTITION_BUFF	(MAX_PARTITION_CMD * MAX_PARTITION_CMD_LEN)
#define PARTITION_CONF_TXT	"partition.txt"

enum LOG_DEBUG_LEVEL {
	LOG_FATAL = 0,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG,
	LOG_VERBOSE,
	LOG_MAX,
};

const static int debug_level = LOG_MAX;
const static int flash_cmds_method = 2;

const static char *pre_cmds[] = {
	"adb devices",
	"adb reboot bootloader",
	"sleep 5",
	"fastboot devices",
	"fastboot reboot-bootloader",
	"sleep 5",
	NULL,
};

const static char *flash_cmds[] = {
	"fastboot flash modem_b NON-HLOS.bin",
	"fastboot flash sbl1_a sbl1.mbn",
	"fastboot flash rpm_b rpm.mbn",
	"fastboot flash mdtp_b mdtp.img",
	"fastboot flash userdata userdata.img",
	NULL,
};

const static char *post_cmds[] = {
	"fastboot --set-active=a",
	"fastboot -w",
	"fastboot reboot",
	NULL,
};

int verbose_log(const char *fmt, ...)
{
	int ret;
	va_list ap;

	if (debug_level <= LOG_VERBOSE)
		return 0;

	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);
	return (ret);
}

int debug_log(const char *fmt, ...)
{
	int ret;
	va_list ap;

	if (debug_level <= LOG_DEBUG)
		return 0;

	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);
	return (ret);
}

int fatal_log(const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vfprintf(stdout, fmt, ap);
	va_end(ap);
	return (ret);
}

int exec_cmd(const char *cmd)
{
	//return system(cmd);
	return 0;
}

int exec_cmds(const char *cmds[])
{
	int i;
	int ret;
	ret = 0;
	for (i = 0; cmds[i] != NULL; i++) {
		debug_log("%s\n", cmds[i]);
		ret = exec_cmd(cmds[i]);
		if (ret != 0) {
			fatal_log("cmd: \"%s\" failed, ret = %d\n", cmds[i],
									ret);
			fatal_log("failed reason: %s\n", strerror(ret));
			return ret;
		}
	}
	return ret;
}

int data_parsing(char *data, unsigned long data_length, int *__argc,
						char **__argv, int argv_mount)
{
	int pos;

	for (pos = 0; pos < data_length; pos++) {
		if (data[pos] < 0x21 || data[pos] > 0x7E || data[pos] == ',') {
			data[pos] = 0x0;
		} else {
			if (*__argc < argv_mount) {
				__argv[*__argc] = &data[pos];
				*__argc += 1;
			} else
				break;
			for (; pos < data_length; pos++) {
				if (data[pos] < 0x21 || data[pos] > 0x7E ||
							data[pos] == ',') {
					data[pos] = 0x0;
					break;
				}
			}
		}
	}
	return *__argc;
}

int load_partition_data_from_file(char **ptr, char *data) {
	FILE *fp;
	int ret = 0;
	char tmp[64];
	int __argc;
        char *__argv[4];
	int argv_amount = 4;
	int cmd_ptr = 0;
	int offset = 0;
	int data_offset;
	fp = fopen(PARTITION_CONF_TXT, "r");
	if (fp == NULL) {
		fatal_log("open partition txt file failed, ret = %d\n", errno);
		fatal_log("failed reason: %s\n", strerror(errno));
		return errno;
	}
	while (fgets(tmp, sizeof(tmp), fp) != NULL) {
		__argc = 0;
		memset(__argv, 0x0, sizeof(__argv));
		data_parsing(tmp, strlen(tmp), &__argc, __argv, argv_amount);

		if (__argc != 2)
			continue;

		data_offset = snprintf(data + offset, MAX_PARTITION_CMD_LEN,
			"fastboot flash %s %s", __argv[0], __argv[1]);
		ptr[cmd_ptr] = (char *)(data + offset);
		debug_log("get: %s\n", (char *)ptr[cmd_ptr]);
		offset += data_offset + 1;
		cmd_ptr++;
		if (offset >= MAX_PARTITION_BUFF) {
			fatal_log("The data buffer is full, "
				"increase the buffer size and try again.\n");
			ret = EBUSY;
			break;
		}
	}

	fclose(fp);

	return ret;
}

int env_setup(void)
{
	char *path_var;
	int current_path_var_length;
	char current_dir_path[256];
	int current_dir_path_value_length;
	char new_path_env_value[MAX_ENV_VALUE_LEN];

	path_var = getenv(PATH_ENV);
	if (path_var == NULL) {
		fatal_log("cannot get correct environment value\n");
		fatal_log("failed reason: %s\n", strerror(errno));
		return errno;
	}
	current_path_var_length = strlen(path_var);
	verbose_log("get origin env value: %s(%ld)\n", path_var,
							strlen(path_var));

	memset(&current_dir_path, 0x0, sizeof(current_dir_path));
	if (getcwd(current_dir_path, sizeof(current_dir_path) - 1) == NULL) {
		fatal_log("cannot get currect directory path\n");
		fatal_log("failed reason: %s\n", strerror(errno));
		return errno;
	}
	current_dir_path_value_length = strlen(current_dir_path);

	if ((current_path_var_length + current_dir_path_value_length) >=
							MAX_ENV_VALUE_LEN) {
		fatal_log("new environment over 128K\n");
		fatal_log("failed reason: %s\n", strerror(errno));
	}
	memset(&new_path_env_value, 0x0, sizeof(new_path_env_value));
	snprintf(new_path_env_value, sizeof(new_path_env_value) - 1,
		"%s/linux-86:%s", current_dir_path, path_var);

	if (setenv(PATH_ENV, new_path_env_value, 1) != 0) {
		fatal_log("config new environment values failed\n");
		fatal_log("failed reason: %s\n", strerror(errno));
	}
	if (debug_level >= LOG_VERBOSE) {
		path_var = getenv(PATH_ENV);
		verbose_log("get modified ven value: %s(%ld)\n", path_var,
							strlen(path_var));
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	char *partition_file_cmd[MAX_PARTITION_CMD];
	char partition_file_data[MAX_PARTITION_BUFF];
	if (flash_cmds_method == 2) {
		memset(partition_file_cmd, 0x0, sizeof(partition_file_cmd));
		memset(partition_file_data, 0x0, sizeof(partition_file_data));
		ret = load_partition_data_from_file(partition_file_cmd,
							partition_file_data);
		if (ret != 0)
			return ret;
	}

	ret = env_setup();
	if (ret != 0)
		return ret;

	ret = exec_cmds(pre_cmds);
	if (ret != 0)
		return ret;

	debug_log("-------------------------------------------------\n");
	if (flash_cmds_method == 1) {
		exec_cmds(flash_cmds);
		if (ret != 0)
			return ret;
	} else if (flash_cmds_method == 2) {
		ret = exec_cmds((const char **)partition_file_cmd);
		if (ret != 0)
			return ret;
	}
	debug_log("-------------------------------------------------\n");
	ret = exec_cmds(post_cmds);
	return ret;
}
