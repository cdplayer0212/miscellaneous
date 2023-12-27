#ifndef __ZIP_FUNCS_H__
#define __ZIP_FUNCS_H__

const char *get_zip_libzip_version(void);

int zip_initialize(struct __global_data *gdata);

int zip_add_file(char *src_file, char *base_dir, char *zip_file_name);

int duplicate_zip_directory(char *src, char *base_dir, char *dest);

int zip_uninitialize(struct __global_data *gdata);

#endif /* __ZIP_FUNCS_H__ */
