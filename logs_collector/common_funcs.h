#ifndef __COMMON_FUNCS_H__
#define __COMMON_FUNCS_H__

#define DEVICE_NAME_PATH			"/sys/q_conf/model"
#define DEVICE_SN_PATH				"/sys/q_conf/serial_no"

enum __file_type {
	FILE_TYPE_UNKNOWN = 0,
	FILE_TYPE_REGULAR_FILE,
	FILE_TYPE_DIRECTORY,
	FILE_TYPE_SYMBOLIC_LINK,
	FILE_TYPE_SOCKET,
	FILE_TYPE_BLOCK_DEVICE,
	FILE_TYPE_CHAR_DEVICE,
	FILE_TYPE_FIFO,
	FILE_TYPE_LAST,
};

int init_check_system_apps(void);
char *get_system_app_path(char *app);

int do_popen_cmd(char *cmd, char *data, ssize_t data_size);

int check_file_is_exist(char *file);
int get_file_stat(char *file, struct stat *file_stat);
int get_symbolic_link_stat(char *file, struct stat *file_stat);
enum __file_type get_file_type(struct stat *file_stat);
const char *get_file_type_string(enum __file_type type);

int hex_string_to_long(char *str, long *val);
int dec_string_to_long(char *str, long *val);

int check_string_is_same(char *str1, char *str2);
void remove_string_eol(char *str);

int get_iso8601_timestamp(char *datetime, size_t len);

int read_data_from_file(char *filename, void *val, size_t val_len);
int get_device_name(char *name, size_t name_len);
int get_device_sn(char *sn, size_t sn_len);

char *strrstr(char *haystack, const char *needle);

int make_dir(char *dirpath);
int duplicate_file(char *src, char *dest);
int duplicate_directory(char *src, char *dest);
int get_real_path(char *path, char *real_path, size_t real_path_len);
#endif /* __COMMON_FUNCS_H__ */
