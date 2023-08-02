#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

#include <libtar.h>
#include <bzlib.h>

#define CHUNK (4096 * 4)

#define DIR_PATH		"mfgdata"
#define DEFAULT_FILE_PATH	"."
#define DEFAULT_FILE_NAME	DIR_PATH".tar"

struct z_comp_data {
	int z_comp_level;
	char target_dir[64]; /* /data/mfgdata ??? */
	char target_tar_file[128];
	char target_tar_extract_to[32];
	char *target_src_zip_file; /* shall point to target_tar_file */
	char target_dest_zip_file[128];
};

int gen_tar_file(char *dir_path, char *archive_file, char *extract_to)
{
	int ret;
	TAR *pTar;

	ret = tar_open(&pTar, archive_file, NULL, O_WRONLY | O_CREAT, 0644,
								TAR_GNU);
	if (ret != 0) {
		fprintf(stderr, "tar open: %s failed, %s\n",
			archive_file, strerror(errno));
		return ret;
	}
	ret = tar_append_tree(pTar, dir_path, extract_to);
	if (ret != 0) {
		fprintf(stderr, "tar append tree: %s failed, %s\n",
			archive_file, strerror(errno));
		tar_close(pTar);
		return ret;
	}
	ret = tar_append_eof(pTar);
	if (ret != 0) {
		fprintf(stderr, "tar append eof: %s failed, %s\n",
			archive_file, strerror(errno));
		tar_close(pTar);
		return ret;
	}
	ret = tar_close(pTar);
	if (ret != 0) {
		fprintf(stderr, "tar close: %s failed, %s\n",
			archive_file, strerror(errno));
	}
	return ret;
}

int compress_tar_file(struct z_comp_data *data)
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

	bfp = fopen(data->target_dest_zip_file, "w");
	if (bfp == NULL) {
		fprintf(stderr, "%s: open file %s failed, %s\n", __func__,
				data->target_dest_zip_file, strerror(errno));
		return -errno;
	}
	bp = BZ2_bzWriteOpen(&bzerror, bfp, 1, 0, 30);
	if (bp == NULL || bzerror != BZ_OK) {
		fprintf(stderr, "%s: BZ2_bzWriteOpen file %s failed, err: %d\n",
				__func__,
				data->target_dest_zip_file, bzerror);
		ret = -errno;
		goto failed_exit;
	}

	if (stat(data->target_tar_file, &f_stat) < 0) {
		fprintf(stderr, "%s: get file stat %s failed, %s\n", __func__,
				data->target_dest_zip_file, strerror(errno));
		ret = -errno;
		goto failed_exit;
	}

	src_fp = fopen(data->target_tar_file, "r");
	if (src_fp == NULL) {
		fprintf(stderr, "%s: open src file %s failed, %s\n", __func__,
				data->target_tar_file, strerror(errno));
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
		fprintf(stderr, "%s: fread_len: %d, buff_len: %d, remain: %d\n",
			__func__, fread_len, buff_len, remain);
		BZ2_bzWrite(&bzerror, bp, buff, buff_len);
		if (bzerror != BZ_OK) {
			fprintf(stderr, "%s: BZ2_bzWrite failed, ret: %d\n",
				__func__, bzerror);
			ret = bzerror;
			goto failed_exit;
		}

		remain -= buff_len;
		if (feof(src_fp))
			break;
	}
	ret = 0;
failed_exit:
	if (src_fp != NULL)
		fclose(src_fp);

	if (bp != NULL) {
		BZ2_bzWriteClose(&bzerror, bp, 0, &nbytes_in, &nbytes_out);
		fprintf(stderr, "%s: BZ2_bzWriteClose: in: %d, out: %d(%d)\n",
				__func__, nbytes_in, nbytes_out, bzerror);
	}

	if (bfp != NULL)
		fclose(bfp);

	return ret;
}

int compress_dir(struct z_comp_data *data)
{
	int ret;

        ret = gen_tar_file(data->target_dir, data->target_tar_file,
						data->target_tar_extract_to);
        if (ret != 0)
		return ret;

	ret = compress_tar_file(data);
        if (ret != 0)
		return ret;

	return 0;
}

static void init_z_compress_data(struct z_comp_data *data)
{
	memset(data, 0x0, sizeof(struct z_comp_data));
	//data->z_comp_level = Z_DEFAULT_COMPRESSION;

#if 0
	strncpy(data->target_dir, "/data/mfgdata",
				sizeof(data->target_dir) - 1);
	strncpy(data->target_tar_file, "/tmp/mfgdata_XXXXX.tar",
				sizeof(data->target_tar_file) - 1);
	strncpy(data->target_tar_extract_to, "mfgdata_XXXXX",
				sizeof(data->target_tar_extract_to) - 1);
	data->target_src_zip_file = data->target_tar_file;
	strncpy(data->target_dest_zip_file, "/tmp/mfgdata_XXXXX.tar.bz2",
				sizeof(data->target_dest_zip_file) - 1);
#else
	strncpy(data->target_dir, "/home/cdplayer0212/tmp/zlib-1.2.13/examples/qbic/mfgdata",
				sizeof(data->target_dir) - 1);
	strncpy(data->target_tar_file, "/home/cdplayer0212/tmp/zlib-1.2.13/examples/qbic/mfgdata_XXXXX.tar",
				sizeof(data->target_tar_file) - 1);
	strncpy(data->target_tar_extract_to, "mfgdata_XXXXX",
				sizeof(data->target_tar_extract_to) - 1);
	data->target_src_zip_file = data->target_tar_file;
	strncpy(data->target_dest_zip_file, "/home/cdplayer0212/tmp/zlib-1.2.13/examples/qbic/mfgdata_XXXXX.tar.bz2",
				sizeof(data->target_dest_zip_file) - 1);

#endif
}

int main(int argc, char *argv[])
{
	struct z_comp_data zdata;

	init_z_compress_data(&zdata);
	compress_dir(&zdata);
	return 0;
}
