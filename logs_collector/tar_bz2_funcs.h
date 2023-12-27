#ifndef __TAR_BZ2_FUNCS_H__
#define __TAR_BZ2_FUNCS_H__

const char *get_libtar_version(void);
const char *get_libbz2_version(void);

int tar_bz2_compress_dir(struct __global_data *a_data);

#endif /* __TAR_BZ2_FUNCS_H__ */
