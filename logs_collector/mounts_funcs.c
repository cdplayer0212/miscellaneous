#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <sys/stat.h>

#include "logs_collector.h"
#include "common_funcs.h"
#include "logger.h"

#define PROC_MOUNTS_FILE		"/proc/mounts"

struct __mount_data {
	char device_node[PATH_MAX];
	char mount_path[PATH_MAX];
	char fs_type[64];
	char options[1024];
	char dump[2];
	char fsck[2];
};	

static int __conf_usbdev_path(struct __global_data *gdata, char *mount_dev,
							char *mount_path)
{
	int ret = 0;
	uint32_t i = 0;
	char dev_node_path[MAX_USB_DEV_NODE_LEN + 5];

	memset(dev_node_path, 0x0, sizeof(dev_node_path));
	snprintf(dev_node_path, sizeof(dev_node_path), "/dev/%s",
						gdata->usb_device_path[i]);
	ret = check_file_is_exist(dev_node_path);
	if (ret != 0)
		return 0;

	for (i = 0; i < gdata->usb_device_cnt; i++) {
		if (strlen(gdata->usb_mount_path[i]))
			continue;

		if (check_string_is_same(dev_node_path, mount_dev)) {
			snprintf(gdata->usb_mount_path[i],
					sizeof(gdata->usb_mount_path[i]),
					"%s", mount_path);
			ret = 1;
			break;
		}
	}
	return ret;
}

int conf_usbdev_mounted_paths(struct __global_data *gdata)
{
	struct __mount_data m_data;
	FILE *fp;
	char fp_buff[2048];
	size_t fp_buff_len;

	fp = fopen(PROC_MOUNTS_FILE, "r");
	if (fp == NULL) {
		ERR("cannot open [%s], %s\n", PROC_MOUNTS_FILE,
							strerror(errno));
		return -errno;
	}

	while (!feof(fp)) {
		memset(fp_buff, 0x0, sizeof(fp_buff));
		memset(&m_data, 0x0, sizeof(m_data));
		if (fgets(fp_buff, sizeof(fp_buff), fp) == NULL)
			continue;

		fp_buff_len = strlen(fp_buff);
		if (fp_buff_len == 0)
			continue;

		remove_string_eol(fp_buff);
		sscanf(fp_buff, "%s %s %s %s %s %s",
			m_data.device_node, m_data.mount_path,
			m_data.fs_type, m_data.options,
			m_data.dump, m_data.fsck);
		__conf_usbdev_path(gdata, m_data.device_node,
							m_data.mount_path);
	};
	fclose(fp);

	return 0;
}
