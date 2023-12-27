#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>

#include "common_funcs.h"
#include "logs_collector.h"
#include "logs_collector_conf.h"
#include "logger.h"
#include "common_funcs.h"

struct __index_data {
	char *conf_name;
	enum __conf_opts id;
} static const index_data[] = {
	{ .conf_name = "debug_level",   .id = CONF_DEBUG_LEVEL,   },
	{ .conf_name = "usb_device",    .id = CONF_USB_DEVICE,    },
	{ .conf_name = "internal_path", .id = CONF_INTERNAL_PATH, },
	{ .conf_name = "log_path",      .id = CONF_LOG_PATH,      },
};

static int set_config_to_gdata(struct __global_data *gdata, char *set,
								char *val)
{
	unsigned int i;
	int matched;
	const struct __index_data *index;
	long temp_val;
	size_t buff_size;
	unsigned int funcs_data_cnt;

	funcs_data_cnt = sizeof(index_data) / sizeof(index_data[0]);

	for (i = 0; i < funcs_data_cnt; i++) {
		index = &index_data[i];
		matched = check_string_is_same(index->conf_name, set);
		if (matched == 0)
			continue;

		switch (index->id) {
		case CONF_DEBUG_LEVEL:
			dec_string_to_long(val, &temp_val);
			if ((enum logger_level)temp_val >= _LOG_EMERG &&
				(enum logger_level)temp_val <= _LOG_EMERG)
				set_log_level((enum logger_level)temp_val);
			break;
		case CONF_USB_DEVICE:
			if (gdata->usb_device_cnt == MAX_USB_DEVICE_CNT) {
				WARNING("usb device index over than %d\n",
							MAX_USB_DEVICE_CNT);
				break;
			}
			buff_size = sizeof(gdata->usb_device_path[
							gdata->usb_device_cnt]);
			snprintf(gdata->usb_device_path[gdata->usb_device_cnt],
				sizeof(gdata->usb_device_path[
					gdata->usb_device_cnt]),
					"%s", val);
			gdata->usb_device_cnt++;
			break;
		case CONF_INTERNAL_PATH:
			if (gdata->internal_path_cnt == MAX_INTERNAL_PATH_CNT) {
				WARNING("internal path index over than %d\n",
							MAX_INTERNAL_PATH_CNT);
				break;
			}
			buff_size = sizeof(gdata->internal_path[
						gdata->internal_path_cnt]);
			strncpy(gdata->internal_path[gdata->internal_path_cnt],
								val,
								buff_size - 1);
			gdata->internal_path_cnt++;
			break;
		case CONF_LOG_PATH:
			if (gdata->log_data_cnt == MAX_LOG_PATH) {
				WARNING("log path index over than %d\n",
							MAX_LOG_PATH);
				break;
			}
			buff_size = sizeof(gdata->log_data[
						gdata->log_data_cnt].log_path);
			strncpy(gdata->log_data[gdata->log_data_cnt].log_path,
							val, buff_size - 1);
			gdata->log_data_cnt++;
			break;
		case CONF_MAX_INDEX:
		default:
			break;
		};
	}

	return -1;
}

int load_config_from_conf_file(struct __global_data *gdata)
{
	FILE *fp;
	size_t tmp_len;
	char attr_s[PATH_MAX - 1];
	char attr_v[PATH_MAX - 1];
	char *attr_v_p;
	unsigned int attr_v_off;
	char tmp[(PATH_MAX * 2) + 1];
	char sep_char[2] = { '"', '\'' };
	int sep_char_index;

	DEBUG("conf file: %s\n", gdata->confile);
	fp = fopen(gdata->confile, "r");

	if (fp == NULL) {
		WARNING("open file: %s failed, %s\n", gdata->confile,
							strerror(errno));
		return -errno;
	}
	while (!feof(fp)) {
		memset(attr_s, 0x0, sizeof(attr_s));
		memset(attr_v, 0x0, sizeof(attr_v));
		memset(tmp, 0x0, sizeof(tmp));
		if (fgets(tmp, sizeof(tmp), fp) == NULL)
			continue;

		tmp_len = strlen(tmp);
		if (tmp_len == 0)
			continue;

		/* skip commit line */
		if (tmp[0] == '#')
			continue;

		remove_string_eol(tmp);
		sscanf(tmp, "%s %s", attr_s, attr_v);
		if (strlen(attr_s) <= 0 || strlen(attr_v) <= 0)
			continue;

		sep_char_index = -1;
		if (sep_char[0] == attr_v[0])
			sep_char_index = 0;

		if (sep_char[1] == attr_v[0])
			sep_char_index = 1;

		if (sep_char_index != -1) {
			attr_v_off = 0;
			attr_v_p = strstr(tmp, attr_v);
			memset(attr_v, 0x0, sizeof(attr_v));
			attr_v[attr_v_off] = attr_v_p[attr_v_off];
			attr_v_off++;
			do {
				attr_v[attr_v_off] = attr_v_p[attr_v_off];
				attr_v_off++;
				if (attr_v_off == sizeof(attr_v) - 1)
					break;
			} while (attr_v_p[attr_v_off - 1] != 0x0 &&
					attr_v_p[attr_v_off - 1] !=
						sep_char[sep_char_index]);
		}

		set_config_to_gdata(gdata, attr_s, attr_v);
	};
	fclose(fp);
	return 0;
}
