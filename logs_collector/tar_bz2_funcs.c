#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>

#include "logs_collector.h"
#include "common_funcs.h"
#include "logger.h"

#include <libtar.h>
#include <bzlib.h>

#define CHUNK					(4096 * 4)

const char *get_libtar_version(void)
{
	return libtar_version;
}

const char *get_libbz2_version(void)
{
	return BZ2_bzlibVersion();
}

int gen_tar_file(char *dir_path, char *archive_file, char *extract_to)
{
	int ret;
	TAR *pTar;

	ret = tar_open(&pTar, archive_file, NULL, O_WRONLY | O_CREAT, 0644,
								TAR_GNU);
	if (ret != 0) {
		WARNING("tar open: %s failed, %s\n", archive_file,
							strerror(errno));
		return ret;
	}
	ret = tar_append_tree(pTar, dir_path, extract_to);
	if (ret != 0) {
		WARNING("tar append tree: %s failed, %s\n", archive_file,
							strerror(errno));
		tar_close(pTar);
		return ret;
	}
	ret = tar_append_eof(pTar);
	if (ret != 0) {
		WARNING("tar append eof: %s failed, %s\n", archive_file,
							strerror(errno));
		tar_close(pTar);
		return ret;
	}
	ret = tar_close(pTar);
	if (ret != 0) {
		WARNING("tar close: %s failed, %s\n", archive_file,
							strerror(errno));
	}
	return ret;
}

int compress_tar_file(char *src_file, char *dest_compress_file)
{
	FILE *src_fp = NULL;
	FILE *bfp = NULL;
	BZFILE *bp = NULL;
	int ret = 0;
	int bzerror;
	size_t remain;
	size_t buff_len;
	size_t fread_len;
	struct stat f_stat;
	unsigned char buff[CHUNK];
	unsigned int nbytes_in = 0;
	unsigned int nbytes_out = 0;

	bfp = fopen(dest_compress_file, "w");
	if (bfp == NULL) {
		WARNING("open file %s failed, %s\n", dest_compress_file,
							strerror(errno));
		return -errno;
	}
	bp = BZ2_bzWriteOpen(&bzerror, bfp, 1, 0, 30);
	if (bp == NULL || bzerror != BZ_OK) {
		WARNING("BZ2_bzWriteOpen file %s failed, err: %d\n",
						dest_compress_file, bzerror);
		ret = -errno;
		goto failed_exit;
	}

	if (stat(src_file, &f_stat) < 0) {
		WARNING("get file stat %s failed, %s\n", src_file,
							strerror(errno));
		ret = -errno;
		goto failed_exit;
	}

	src_fp = fopen(src_file, "r");
	if (src_fp == NULL) {
		WARNING("open src file %s failed, %s\n", src_file,
							strerror(errno));
		ret = -errno;
		goto failed_exit;
	}

	remain = f_stat.st_size;
	while (remain > 0) {
		if (remain > CHUNK)
			buff_len = CHUNK;
		else
			buff_len = remain;
		fread_len = fread(buff, buff_len, 1, src_fp);
		if (fread_len != 1)
			WARNING("incorrect read length: %d\n", fread_len);

		BZ2_bzWrite(&bzerror, bp, buff, buff_len);
		if (bzerror != BZ_OK) {
			WARNING("BZ2_bzWrite failed, ret: %d\n", bzerror);
			ret = bzerror;
			goto failed_exit;
		}

		remain -= buff_len;
		if (feof(src_fp))
			break;
	}
	ret = 0;
failed_exit:
	if (src_fp != NULL) {
		fclose(src_fp);
		src_fp = NULL;
		remove(src_file);
	}

	if (bp != NULL) {
		BZ2_bzWriteClose(&bzerror, bp, 0, &nbytes_in, &nbytes_out);
		INFO("BZ2_bzWriteClose: done, in: %d, out: %d(%d)\n",
						nbytes_in, nbytes_out, bzerror);
	}

	if (bfp != NULL) {
		fclose(bfp);
		bfp = NULL;
	}

	return ret;
}

int tar_bz2_compress_dir(struct __global_data *gdata)
{
	int ret;

	if (check_file_is_exist(gdata->dest_archive_file) == 0) {
		ret = remove(gdata->dest_archive_file);
		if (ret != 0) {
			WARNING("delete [%s] failed, %s\n",
				gdata->dest_archive_file, strerror(errno));
			return -errno;
		}
	}

	if (check_file_is_exist(gdata->tar_bz2_base_file) == 0) {
		ret = remove(gdata->tar_bz2_base_file);
		if (ret != 0) {
			WARNING("delete [%s] failed, %s\n",
					gdata->tar_bz2_base_file,
					strerror(errno));
			return -errno;
		}
	}

	ret = gen_tar_file(gdata->dest_logs_dir, gdata->dest_archive_file,
					gdata->dest_archive_extract_to);
	if (ret != 0)
		return ret;

	ret = compress_tar_file(gdata->dest_archive_file,
					gdata->tar_bz2_base_file_path);
	if (ret != 0)
		return ret;

	return 0;
}
