#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <syslog.h>

// #define ENABLE_PRINT_FUNC_LINE

enum logger_level {
	_LOG_EMERG   = LOG_EMERG,
	_LOG_ALERT   = LOG_ALERT,
	_LOG_CRIT    = LOG_CRIT,
	_LOG_ERR     = LOG_ERR,
	_LOG_WARNING = LOG_WARNING,
	_LOG_NOTICE  = LOG_NOTICE,
	_LOG_INFO    = LOG_INFO,
	_LOG_DEBUG   = LOG_DEBUG,
};

enum logger_type {
	_LOG_PRINTF = 1,
	_LOG_SYSLOG = 1 << 1,
	_LOG_FILE   = 1 << 2,
};

#define DEFAULT_LOGGER_FILE			"/tmp/log_corrector.log"
#define MAX_LOG_SIZE_LIMIT			(8 * 1024 * 1024)

void log_initialize(uint32_t log_level, uint32_t log_type);
void log_uninitialize(void);
void set_log_level(enum logger_level level);
uint32_t get_log_level(void);
void set_log_type(enum logger_type type);
uint32_t get_log_type(void);

int logger(enum logger_level level, char *fmt, ...);

#ifdef ENABLE_PRINT_FUNC_LINE

#define EMERG(fmt, ...) \
		logger(_LOG_EMERG, "%s[%d]: " fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define ALERT(fmt, ...) \
		logger(_LOG_ALERT, "%s[%d]: " fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define CRIT(fmt, ...) \
		logger(_LOG_CRIT, "%s[%d]: " fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define ERR(fmt, ...) \
		logger(_LOG_ERR, "%s[%d]: " fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define WARNING(fmt, ...) \
		logger(_LOG_WARNING, "%s[%d]: " fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define NOTICE(fmt, ...) \
		logger(_LOG_NOTICE, "%s[%d]: " fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define INFO(fmt, ...) \
		logger(_LOG_INFO, "%s[%d]: " fmt, __func__, __LINE__, \
		##__VA_ARGS__)
#define DEBUG(fmt, ...) \
		logger(_LOG_DEBUG, "%s[%d]: " fmt, __func__, __LINE__, \
		##__VA_ARGS__)

#else /* ENABLE_PRINT_FUNC_LINE */

#define EMERG(fmt, ...)		logger(_LOG_EMERG, fmt, ##__VA_ARGS__)
#define ALERT(fmt, ...)		logger(_LOG_ALERT, fmt, ##__VA_ARGS__)
#define CRIT(fmt, ...)		logger(_LOG_CRIT, fmt, ##__VA_ARGS__)
#define ERR(fmt, ...)		logger(_LOG_ERR, fmt, ##__VA_ARGS__)
#define WARNING(fmt, ...)	logger(_LOG_WARNING, fmt, ##__VA_ARGS__)
#define NOTICE(fmt, ...)	logger(_LOG_NOTICE, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...)		logger(_LOG_INFO, fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...)		logger(_LOG_DEBUG, fmt, ##__VA_ARGS__)

#endif /* ENABLE_PRINT_FUNC_LINE */

#endif /* __LOGGER_H__ */
