#ifndef __LOGS_COLLECTOR_CONF_H__
#define __LOGS_COLLECTOR_CONF_H__

enum __conf_opts {
	CONF_DEBUG_LEVEL = 0,
	CONF_USB_DEVICE,
	CONF_INTERNAL_PATH,
	CONF_LOG_PATH,
	CONF_MAX_INDEX, /* keep at last */
};

int load_config_from_conf_file(struct __global_data *gdata);

#endif /* __LOGS_COLLECTOR_CONF_H__ */
