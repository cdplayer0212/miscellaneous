#ifndef __CAMERA_PREVIEW_CAMERA_UTIL_H__
#define __CAMERA_PREVIEW_CAMERA_UTIL_H__  

#include <linux/videodev2.h>

#define APP_MAJOR_REV			1
#define APP_MINOR_REV			0
#define APP_MICRO_REV			0

#define CAMERA_DEV				"/dev/video0"

#define CAMERA_PREVIEW_WIDTH			640
#define CAMERA_PREVIEW_HEIGHT			480
#define CAMERA_PREVIEW_DEPTH			3

#define CAMERA_BUFFER_SIZE			(CAMERA_PREVIEW_WIDTH * \
						CAMERA_PREVIEW_HEIGHT * \
						CAMERA_PREVIEW_DEPTH)

#define CAMERA_BUFFER_CNT		4

enum __raw_image_status {
	RAW_IMAGE_EMPTY = 0,
	RAW_IMAGE_FETCHING,
	RAW_IMAGE_FETCHED,
	RAW_IMAGE_SUBMITTING,
	RAW_IMAGE_PASS_IMG,
	RAW_IMAGE_FAIL_IMG,
};

enum __camera_format {
	CAMERA_FORMAT_NONE_SET = 0,
	CAMERA_FORMAT_YUYV,
	CAMERA_FORMAT_MJPEG,
	CAMERA_FORMAT_H264,
};

struct buffer {
	void   *start;
	unsigned long length;
};

enum CAMERA_ACTION {
	CAMERA_HELP_ACTION = 0,
	CAMERA_VERSION_ACTION,
	CAMERA_RUN_ACTION,
};

struct camera_util_data {
	enum CAMERA_ACTION camera_action;
	int camera_fd;
	char dev_path[32];
	enum __camera_format format;
	int camera_width;
	int camera_height;
	int camera_depth;
	unsigned long camera_buff_size;
	unsigned int camera_rotate;
	struct buffer buffers[CAMERA_BUFFER_CNT];
	unsigned char rgb_buffer[CAMERA_BUFFER_SIZE];

	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;

	enum __raw_image_status raw_image_status;
	unsigned long long image_count;
	unsigned int is_test_completed;

	time_t start_time;
	time_t current_time;
	unsigned long long fps_val;
	int show_fps;
};

int open_device(struct camera_util_data *data);
int close_device(struct camera_util_data *data);
int init_device(struct camera_util_data *data);
int uninit_device(struct camera_util_data *data);
int start_capturing(struct camera_util_data *data);
int stop_capturing(struct camera_util_data *data);
int get_camera_frame(struct camera_util_data *data);
int start_camera_preview(struct camera_util_data *data);
int stop_camera_preview(struct camera_util_data *data);
int check_fps_is_meet_requirement(struct camera_util_data *data);
#endif /* __CAMERA_PREVIEW_CAMERA_UTIL_H__ */
