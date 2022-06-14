#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define RANDOM_DEV_NODE		"/dev/urandom"
//#define RANDOM_DEV_NODE		"/dev/random"
#define BUFF_SIZE		256
//#define PASSWD_SIZE		64
#define PASSWD_SIZE		36
#define CHAR_MAX_REPEAT_CNT	3
#define READ_MAX_LEN		32

struct __data {
	int dev_random;
	int dev_random_is_opened;
	unsigned char buff[BUFF_SIZE + 1];
	char passwd[PASSWD_SIZE + 1];
	unsigned char char_table[0x5F];
};

int open_dev_random(struct __data *data)
{
	data->dev_random = open(RANDOM_DEV_NODE, O_RDONLY);
	if (data->dev_random == -1)
		return -errno;

	data->dev_random_is_opened = 1;
	return 0;
}

int read_dev_random(struct __data *data)
{
	size_t tmp = 0;
	size_t offset = 0;
	size_t remain = sizeof(data->buff) - 1;
	size_t read_cnt;
	unsigned int delay_seed_1, delay_seed_2, delay_seed;

	while (remain != 0) {
		if (remain > READ_MAX_LEN)
			read_cnt = READ_MAX_LEN;
		else
			read_cnt = remain;

		tmp = read(data->dev_random, &data->buff[offset], read_cnt);
		if (tmp == -1) {
			fprintf(stderr, "cannot fetch random data from "
							"device node\n");
			return -errno;
		}
		/*
		printf("%s: get data->buff[%lu]: 0x%2.2X(%d)\n", __func__,
				offset, data->buff[offset], data->buff[offset]);
		*/
		delay_seed_1 = data->buff[offset - 1];
		delay_seed_2 = data->buff[offset];
		delay_seed = delay_seed_1 * (delay_seed_2 / 8) * (delay_seed_1 % 64);
		remain -= tmp;
		offset += tmp;
		/*
		printf("%s: ramdom count: %3lu(remain: %3lu, offset: %3lu), "
			"delay: %d\n",
			__func__,
			tmp,
			remain,
			offset,
			delay_seed);
		*/
		usleep(delay_seed);
	}
	return 0;
}

int close_dev_random(struct __data *data)
{
	int ret = 0;

	if (data->dev_random_is_opened) {
		ret = close(data->dev_random);
		if (ret == 0)
			data->dev_random_is_opened = 0;
		else
			ret = -errno;
	}
	return ret;
}

void dump_raw_random_data(struct __data *data)
{
	/*
	int i;

	for (i = 0; i < sizeof(data->buff) - 1; i += 8) {
		printf("data[%3d]: 0x%2.2X, 0x%2.2X, 0x%2.2X 0x%2.2X, "
			"0x%2.2X, 0x%2.2X, 0x%2.2X 0x%2.2X\n",
			i,
			data->buff[i], data->buff[i + 1],
			data->buff[i + 2], data->buff[i + 3],
			data->buff[i + 4], data->buff[i + 5],
			data->buff[i + 6], data->buff[i + 7]);
	}
	*/
}

int generate_passwd(struct __data *data)
{
	int i;
	int offset = 0;

	for (i = 0; (i < sizeof(data->buff) - 1) &&
				(offset < sizeof(data->passwd) - 1); i++) {
		if (data->buff[i] <= 0x20 || data->buff[i] >= 0x7F)
			continue;

/*
		if (offset > 0 && data->buff[i] == data->passwd[offset - 1])
			continue;
*/

		if (data->char_table[(data->buff[i] - 0x21)] >=
							CHAR_MAX_REPEAT_CNT)
			continue;

		if (data->buff[i] == '"' || data->buff[i] == '`' ||
				data->buff[i] == '\'')
			continue;
/*
		printf("char table: %c: %d\n",
				data->buff[i],
				data->char_table[(data->buff[i] - 0x21)]);
*/
		data->passwd[offset++] = data->buff[i];
		data->char_table[(data->buff[i] - 0x21)] += 1;
	}

	return 0;
}

void dump_passwd_data(struct __data *data)
{
	int i;

	printf("\n");
	for (i = 0; i < sizeof(data->char_table) - 1; i++) {
		if (data->char_table[i] != 0)
			printf("%c: %d", i + 0x21, data->char_table[i]);
		else
			printf("%c:  ", i + 0x21);
		if ((i % 8 == 7) || ((i + 1) == (sizeof(data->char_table) - 1)))
			printf("\n");
		else
			printf("      ");
	}
	/*
	for (i = 0; i < sizeof(data->char_table) - 1; i += 8) {
		printf("%c: %d,    %c: %d,    %c: %d,    %c: %d,"
			"    %c: %d,    %c: %d,    %c: %d,    %c: %d\n",
			i + 0x21, data->char_table[i],
			i + 0x22, data->char_table[i + 1],
			i + 0x23, data->char_table[i + 2],
			i + 0x24, data->char_table[i + 3],
			i + 0x25, data->char_table[i + 4],
			i + 0x26, data->char_table[i + 5],
			i + 0x27, data->char_table[i + 6],
			i + 0x28, data->char_table[i + 7]);
	}
	*/
}

int main(int argc, char *argv[])
{
	int ret = 0;

	struct __data data;

	memset(&data, 0x0, sizeof(data));
	ret = open_dev_random(&data);
	ret = read_dev_random(&data);
	ret = close_dev_random(&data);
	dump_raw_random_data(&data);
	generate_passwd(&data);
	dump_passwd_data(&data);
	printf("\n\nget password: %s\nstring length: %lu\n\n\n", data.passwd,
							strlen(data.passwd));
	return ret;
}
