#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <setjmp.h>

#include <jpeglib.h>

struct priv_jpeg_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;		/* for return to caller */
};

struct __jpeg_data {
	bool is_initialized;
	bool jpeg_decompresser_is_ready;

	struct jpeg_decompress_struct cinfo;
	struct priv_jpeg_error_mgr jpeg_err;

	unsigned int jpeg_error_cnt;
};

static struct __jpeg_data jpeg_data = {
		.is_initialized = false,
	};

unsigned int jpeg_get_error_cnt(void)
{
	return jpeg_data.jpeg_error_cnt;
}

void jpeg_set_error_cnt(unsigned int cnt)
{
	jpeg_data.jpeg_error_cnt = cnt;
}

void jpeg_increase_error_cnt(void)
{
	jpeg_data.jpeg_error_cnt++;
}

int jpeg_rgb24_decomp(unsigned char *jpeg_buff, unsigned long jpeg_buff_len,
			unsigned char *dest_buff, int width, int height)
{
	int ret;
	int row_stride;
	struct jpeg_decompress_struct *cinfo_p;
	unsigned char *buffer_array[1];
	unsigned int cnt = 0;

	if (!jpeg_data.is_initialized || !jpeg_data.jpeg_decompresser_is_ready)
		return -EINVAL;

	cinfo_p = &jpeg_data.cinfo;
	jpeg_mem_src(cinfo_p, jpeg_buff, jpeg_buff_len);
	ret = jpeg_read_header(cinfo_p, TRUE);
	if (ret != JPEG_HEADER_OK)
		return -1;

	jpeg_start_decompress(cinfo_p);
	row_stride = cinfo_p->output_width * cinfo_p->output_components;

	while (cinfo_p->output_scanline < cinfo_p->output_height) {
		buffer_array[0] = dest_buff + \
				(cinfo_p->output_scanline) * row_stride;

		jpeg_read_scanlines(cinfo_p, buffer_array, 1);
	}
	jpeg_finish_decompress(cinfo_p);
	return 0;
}

static void jpeg_error_exit_func(j_common_ptr cinfo)
{
	/*
	 * cinfo->err really points to a my_error_mgr struct,
	 * so coerce pointer
	 */
	struct priv_jpeg_error_mgr *myerr =
				(struct priv_jpeg_error_mgr *)cinfo->err;
	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message)(cinfo);
	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

int jpeg_decomp_init(int width, int height, int depth)
{
	if (jpeg_data.is_initialized == true)
		return -EINVAL;

	memset(&jpeg_data, 0x0, sizeof(jpeg_data));
	jpeg_data.cinfo.err = jpeg_std_error(&jpeg_data.jpeg_err.pub);
	jpeg_data.jpeg_err.pub.error_exit = jpeg_error_exit_func;

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jpeg_data.jpeg_err.setjmp_buffer)) {
		/*
		 * If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file,
		 * and return.
		 */
		jpeg_increase_error_cnt();
		return -2;
	}
	jpeg_create_decompress(&jpeg_data.cinfo);
	jpeg_data.jpeg_decompresser_is_ready = true;
	jpeg_data.is_initialized = true;

	return 0;
}

int jpeg_decomp_uninit(void)
{
	if (jpeg_data.is_initialized) {
		jpeg_data.jpeg_decompresser_is_ready = false;
		jpeg_destroy_decompress(&jpeg_data.cinfo);
		jpeg_data.is_initialized = false;
	}
	return 0;
}
