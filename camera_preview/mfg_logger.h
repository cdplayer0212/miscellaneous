#ifndef __CAMERA_PREVIEW_LOGGER_H__
#define __CAMERA_PREVIEW_LOGGER_H__

#include <syslog.h>

enum logger_level {
	MFG_LOG_EMERG   = LOG_EMERG,
	MFG_LOG_ALERT   = LOG_ALERT,
	MFG_LOG_CRIT    = LOG_CRIT,
	MFG_LOG_ERR     = LOG_ERR,
	MFG_LOG_WARNING = LOG_WARNING,
	MFG_LOG_NOTICE  = LOG_NOTICE,
	MFG_LOG_INFO    = LOG_INFO,
	MFG_LOG_DEBUG   = LOG_DEBUG,
};

enum logger_type {
	MFG_LOG_PRINTF = 1,
	MFG_LOG_SYSLOG = 1 << 1,
	MFG_LOG_FILE   = 1 << 2,
};

void log_initialize(void);
void log_uninitialize(void);
void set_log_level(enum logger_level level);
u_int32_t get_log_level(void);
int mfg_logger(enum logger_level level, char *fmt, ...);

#define EMERG(fmt, ...) \
		mfg_logger(MFG_LOG_EMERG, "%s[%d]:" fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define ALERT(fmt, ...) \
		mfg_logger(MFG_LOG_ALERT, "%s[%d]:" fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define CRIT(fmt, ...) \
		mfg_logger(MFG_LOG_CRIT, "%s[%d]:" fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define ERR(fmt, ...) \
		mfg_logger(MFG_LOG_ERR, "%s[%d]:" fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define WARNING(fmt, ...) \
		mfg_logger(MFG_LOG_WARNING, "%s[%d]:" fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define NOTICE(fmt, ...) \
		mfg_logger(MFG_LOG_NOTICE, "%s[%d]:" fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define INFO(fmt, ...) \
		mfg_logger(MFG_LOG_INFO, "%s[%d]:" fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define DEBUG(fmt, ...) \
		mfg_logger(MFG_LOG_DEBUG, "%s[%d]:" fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#endif /* __CAMERA_PREVIEW_LOGGER_H__ */
