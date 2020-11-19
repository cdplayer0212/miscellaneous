#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdarg.h>
#include	<unistd.h>

#define LOG_SIZE	(1 * 1024)
#define LOG_TEMP_SIZE	256

struct __logs_data {
	char *logs_data;
	char *logs_temp;
	char *log_head;
	char *log_tail;
	size_t log_size;
	size_t log_remain;
	int log_overwhite;
};

struct __logs_data logs_data;

int logs_initialize(struct __logs_data *logs_p)
{
	memset(logs_p, 0x0, sizeof(struct __logs_data));
	logs_p->logs_data = malloc(LOG_SIZE);
	if (logs_p->logs_data == NULL) {
		fprintf(stderr, "%s: alloc logs data area failed\n",
								__func__);
		return -2;
	}
	memset(logs_p->logs_data, 0x0, sizeof(LOG_SIZE));
	logs_p->logs_temp = malloc(LOG_TEMP_SIZE);
	if (logs_p->logs_temp == NULL) {
		fprintf(stderr, "%s: alloc logs data area failed\n",
								__func__);
		return -2;
	}
	memset(logs_p->logs_temp, 0x0, sizeof(LOG_TEMP_SIZE));
	logs_p->log_head = logs_p->logs_data;
	logs_p->log_tail = logs_p->logs_data;
	logs_p->log_size = LOG_SIZE - 1;
	logs_p->log_remain = LOG_SIZE - 1;

	return 0;
}

void logs_uninitialize(struct __logs_data *logs_p)
{
	free(logs_p->logs_data);
	free(logs_p->logs_temp);
}

size_t logs_dump()
{
	size_t ret = 0;
/*
	printf("++ data: %p, head: %p(%lu), tail: %p(%lu), remain: %ld ==\n",
		logs_data.logs_data,
		logs_data.log_head,
		logs_data.log_head - logs_data.logs_data,
		logs_data.log_tail,
		logs_data.log_tail - logs_data.logs_data,
		logs_data.log_remain);
*/
	printf("%s", logs_data.log_head);
	ret = strlen(logs_data.log_head);
	if (logs_data.log_head > logs_data.log_tail) {
		ret += strlen(logs_data.logs_data);
		printf("%s", logs_data.logs_data);
	}

	return ret;
}

size_t debug_log(const char *fmt, ...)
{
	va_list ap;
	size_t ret = 0;
	size_t remain = 0;

	va_start(ap, fmt);
	remain = vsnprintf(logs_data.log_tail,
			logs_data.log_remain, fmt, ap);
	va_end(ap);
	ret += remain;

	if (remain > logs_data.log_size) {
		printf("log length over buffer\n");
		return 0;
	}
#if 0
	printf("== data: %p, head: %p(%lu), tail: %p(%lu), re: %ld, %ld ==\n",
		logs_data.logs_data,
		logs_data.log_head,
		logs_data.log_head - logs_data.logs_data,
		logs_data.log_tail,
		logs_data.log_tail - logs_data.logs_data,
		logs_data.log_remain, remain);
#endif
	if (remain > logs_data.log_remain) {
		logs_data.log_overwhite = 1;
		logs_data.log_tail = logs_data.logs_data;
		logs_data.log_remain = logs_data.log_size;
		*(logs_data.log_tail + 1) = 0x0;
		logs_data.log_head = logs_data.log_tail + 2;
	} else {
		logs_data.log_tail += remain;
		logs_data.log_remain -= remain;
		if (logs_data.log_overwhite) {
			*(logs_data.log_tail + 1) = 0x0;
			logs_data.log_head = logs_data.log_tail + 2;
		}
	}

	return ret;
}

#define DATA_T	5000

int main(int argc, char *argv[])
{
	int loop_i;
//	char data[DATA_T];
//	char data_c = 0x20;
	logs_initialize(&logs_data);
/*
	for (loop_i = 0; loop_i < sizeof(data); loop_i++) {
		data[loop_i] = data_c++;
		if (data_c == 0x7F)
			data_c = 0x20;
	}
	data[DATA_T - 1] = 0;
*/
	for (loop_i = 0; loop_i < 3; loop_i++) {
		debug_log("data[%4d] 23rniowefwej fhweohifhwe\n2222\n", loop_i);
		debug_log("23sdlfkjselifwoj\nwefkwe123f9vews2q3er2\n");
		debug_log("23sdlfk123123vifwoj\nwefkwe123f9wfwwefwef   vews2q3er2\n");
		debug_log("2  3sdlfk wefwefe wefwehuifwe weiofwef  \n");
	}
	loop_i = logs_dump();
	printf("count: %d\n", loop_i);
	logs_uninitialize(&logs_data);
	return 0;
}

