#ifndef __CAMERA_PREVIEW_CAMERA_UTIL_JPEG_H__
#define __CAMERA_PREVIEW_CAMERA_UTIL_JPEG_H__

#define SOI_BYTE_0	0xFF
#define SOI_BYTE_1	0xD8

int jpeg_rgb24_decomp(unsigned char *jpeg_buff, unsigned long jpeg_buff_len,
			unsigned char *dest_buff, int width, int height);
int jpeg_decomp_init(int width, int height, int depth);
int jpeg_decomp_uninit(void);

unsigned int jpeg_get_error_cnt(void);
void jpeg_set_error_cnt(unsigned int cnt);
void jpeg_increase_error_cnt(void);
#endif /* __CAMERA_PREVIEW_CAMERA_UTIL_JPEG_H__ */
