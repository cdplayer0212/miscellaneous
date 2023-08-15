#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>	      /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "camera_util.h"

#include "mfg_logger.h"

static int xioctl(int *fd, int request, void *arg)
{
	int r;
	int retry = 5;

	do {
		r = ioctl(*fd, request, arg);
		if (--retry == 0)
			break;
	} while (-1 == r && EINTR == errno);

	return r;
}

int yuv422_rgb24(unsigned char *yuv_buf, unsigned char *rgb_buf,
					unsigned int width, unsigned int height)
{
#define Y0	0
#define U	1
#define Y1	2
#define V	3

#define R	0
#define G	1
#define B	2
	int yuvdata[4];
	int rgbdata[3];
	unsigned char *rgb_temp;
	unsigned int i, j;

	rgb_temp = rgb_buf;
	for (i = 0; i < height * 2; i++) {
		for (j = 0; j < width; j += 4) {
			/* get Y0 U Y1 V */
			yuvdata[Y0] = *(yuv_buf + i * width + j + 0);
			yuvdata[U]  = *(yuv_buf + i * width + j + 1);
			yuvdata[Y1] = *(yuv_buf + i * width + j + 2);
			yuvdata[V]  = *(yuv_buf + i * width + j + 3);

			/* the first pixel */
			rgbdata[R] = yuvdata[Y0] + (yuvdata[V] - 128) +
					(((yuvdata[V] - 128) * 104) >> 8);
			rgbdata[G] = yuvdata[Y0] -
					(((yuvdata[U] - 128) * 89) >> 8) -
					(((yuvdata[V] - 128) * 183) >> 8);
			rgbdata[B] = yuvdata[Y0] + (yuvdata[U] - 128) +
					(((yuvdata[U] - 128) * 199) >> 8);

			if (rgbdata[R] > 255)
				rgbdata[R] = 255;

			if (rgbdata[R] < 0)
				rgbdata[R] = 0;

			if (rgbdata[G] > 255)
				rgbdata[G] = 255;

			if (rgbdata[G] < 0)
				rgbdata[G] = 0;

			if (rgbdata[B] > 255)
				rgbdata[B] = 255;

			if (rgbdata[B] < 0)
				rgbdata[B] = 0;

			*(rgb_temp++) = rgbdata[R];
			*(rgb_temp++) = rgbdata[G];
			*(rgb_temp++) = rgbdata[B];

			/* the second pix */
			rgbdata[R] = yuvdata[Y1] + (yuvdata[V] - 128) +
					(((yuvdata[V] - 128) * 104) >> 8);
			rgbdata[G] = yuvdata[Y1] -
					(((yuvdata[U] - 128) * 89) >> 8) -
					(((yuvdata[V] - 128) * 183) >> 8);
			rgbdata[B] = yuvdata[Y1] + (yuvdata[U] - 128) +
					(((yuvdata[U] - 128) * 199) >> 8);

			if (rgbdata[R] > 255)
				rgbdata[R] = 255;

			if (rgbdata[R] < 0)
				rgbdata[R] = 0;

			if (rgbdata[G] > 255)
				rgbdata[G] = 255;

			if (rgbdata[G] < 0)
				rgbdata[G] = 0;

			if (rgbdata[B] > 255)
				rgbdata[B] = 255;

			if (rgbdata[B] < 0)
				rgbdata[B] = 0;

			*(rgb_temp++) = rgbdata[R];
			*(rgb_temp++) = rgbdata[G];
			*(rgb_temp++) = rgbdata[B];
		}
	}
	return 0;
}

/* #define STORE_RAW_IMAG_TO_FILE */
static void process_image(struct camera_util_data *data)
{
#ifdef STORE_RAW_IMAG_TO_FILE
	int ret;
	int fd;
#endif /* STORE_RAW_IMAG_TO_FILE */

	yuv422_rgb24(data->buffers[data->buf.index].start,
		data->rgb_buffer, data->camera_width, data->camera_height);

#ifdef STORE_RAW_IMAG_TO_FILE
	/* write to file */
	fd = open("/tmp/video.raw", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		return;

	ret = write(fd, data->buffers[data->buf.index].start,
							data->buf.bytesused);
	if (ret == -1)
		ret = errno;
	fsync(fd);
	close(fd);
#endif /* STORE_RAW_IMAG_TO_FILE */
}

int start_capturing(struct camera_util_data *data)
{
	int i;
	enum v4l2_buf_type type;

	for (i = 0; i < CAMERA_BUFFER_CNT; ++i) {
		memset(&data->buf, 0x0, sizeof(data->buf));
		data->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		data->buf.memory = V4L2_MEMORY_MMAP;
		data->buf.index = i;
		if (-1 == xioctl(&data->camera_fd, VIDIOC_QBUF, &data->buf)) {
			ERR("VIDIOC_QBUF failed, %s\n", strerror(errno));
			return -errno;
		}
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(&data->camera_fd, VIDIOC_STREAMON, &type)) {
		ERR("VIDIOC_STREAMON failed, %s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

int stop_capturing(struct camera_util_data *data)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(&data->camera_fd, VIDIOC_STREAMOFF, &type)) {
		ERR("VIDIOC_STREAMOFF failed, %s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

static int init_mmap(struct camera_util_data *data)
{
	int i;

	memset(&data->req, 0x0, sizeof(data->req));
	data->req.count = CAMERA_BUFFER_CNT;
	data->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	data->req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(&data->camera_fd, VIDIOC_REQBUFS, &data->req)) {
		if (errno == EINVAL) {
			ERR("%s does not support memory mapping\n",
								data->dev_path);
			return -EIO;
		} else {
			WARNING("VIDIOC_REQBUFS failed, %s\n",
							strerror(errno));
		}
	}
	DEBUG("get count: %d\n", data->req.count);
	if (data->req.count < 2) {
		ERR("Insufficient buffer memory on %s\n", data->dev_path);
		return -EIO;
	}

	for (i = 0; i < data->req.count; ++i) {
		memset(&data->buf, 0x0, sizeof(data->buf));

		data->buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		data->buf.memory      = V4L2_MEMORY_MMAP;
		data->buf.index       = i;
		if (-1 == xioctl(&data->camera_fd, VIDIOC_QUERYBUF,
								&data->buf)) {
			ERR("VIDIOC_QUERYBUF failed, %s\n", strerror(errno));
			return -errno;
		}

		data->buffers[i].length = data->buf.length;
		data->buffers[i].start = mmap(NULL /* start anywhere */,
					data->buf.length,
					PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */,
					data->camera_fd, data->buf.m.offset);

		if (data->buffers[i].start == MAP_FAILED) {
			ERR("MMAP mapping failed, %s\n", strerror(errno));
			return -errno;
		}
	}
	return 0;
}

int init_device(struct camera_util_data *data)
{
	unsigned int min;
	int ret = 0;

	if (-1 == xioctl(&data->camera_fd, VIDIOC_QUERYCAP, &data->cap)) {
		if (errno == EINVAL) {
			ERR("%s is no V4L2 device\n", data->dev_path);
			return -errno;
		} else {
			WARNING("xioctl VIDIOC_QUERYCAP failed");
		}
	}

	if (!(data->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		ERR("%s is no video capture device\n", data->dev_path);
		return -errno;
	}
#if 1
	if (!(data->cap.capabilities & V4L2_CAP_STREAMING)) {
		ERR("%s does not support streaming i/o\n", data->dev_path);
		return -EINVAL;
	}
#else
	switch (data->io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			ERR("%s does not support read i/o\n",
							data->dev_path);
			return -EINVAL;
		}
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(data->cap.capabilities & V4L2_CAP_STREAMING)) {
			ERR("%s does not support streaming i/o\n",
				data->dev_path);
			return -EINVAL;
		}
		break;
	}
#endif
	memset(&data->cropcap, 0x0, sizeof(data->cropcap));

	data->cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(&data->camera_fd, VIDIOC_CROPCAP, &data->cropcap)) {
		data->crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		data->crop.c = data->cropcap.defrect; /* reset to default */

		if (-1 == xioctl(&data->camera_fd, VIDIOC_S_CROP,
								&data->crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	memset(&data->fmt, 0x0, sizeof(data->fmt));

	data->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	INFO("Force Format %d\n", data->format);
#if 0
	if (data->format) {
		if (data->format == 2) {
			data->fmt.fmt.pix.width		= data->camera_width;
			data->fmt.fmt.pix.height	= data->camera_height;
			data->fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_H264;
			data->fmt.fmt.pix.field		= V4L2_FIELD_INTERLACED;
		} else if (data->format == 1) {
			data->fmt.fmt.pix.width		= data->camera_width;
			data->fmt.fmt.pix.height	= data->camera_height;
			data->fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_YUYV;
			data->fmt.fmt.pix.field		= V4L2_FIELD_INTERLACED;
		}

		if (-1 == xioctl(&data->camera_fd, VIDIOC_S_FMT, &data->fmt)) {
			ERR("VIDIOC_S_FMT failed, %s\n", strerror(errno));
			return -errno;
		}

		/* Note VIDIOC_S_FMT may change width and height. */
	} else {
		/* Preserve original settings as set by v4l2-ctl for example */
		if (-1 == xioctl(&data->camera_fd, VIDIOC_G_FMT, &data->fmt)) {
			ERR("VIDIOC_G_FMT failed, %s\n", strerror(errno));
			return -errno;
		}

		DEBUG("fmt.fmt.pix.width: %d\n", data->fmt.fmt.pix.width);
		DEBUG("fmt.fmt.pix.height: %d\n", data->fmt.fmt.pix.height);
	}
#else
	switch (data->format) {
	case CAMERA_FORMAT_NONE_SET:
		if (-1 == xioctl(&data->camera_fd, VIDIOC_G_FMT, &data->fmt)) {
			ERR("CAMERA_FORMAT_NONE_SET: VIDIOC_G_FMT failed, "
						"%s\n", strerror(errno));
			return -errno;
		}
		break;
	case CAMERA_FORMAT_YUYV:
		data->fmt.fmt.pix.width		= data->camera_width;
		data->fmt.fmt.pix.height	= data->camera_height;
		data->fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_YUYV;
		data->fmt.fmt.pix.field		= V4L2_FIELD_INTERLACED;
		if (-1 == xioctl(&data->camera_fd, VIDIOC_S_FMT, &data->fmt)) {
			ERR("CAMERA_FORMAT_YUYV: VIDIOC_S_FMT failed, %s\n",
							strerror(errno));
			return -errno;
		}
		break;
	case CAMERA_FORMAT_H264:
		data->fmt.fmt.pix.width		= data->camera_width;
		data->fmt.fmt.pix.height	= data->camera_height;
		data->fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_H264;
		data->fmt.fmt.pix.field		= V4L2_FIELD_INTERLACED;
		if (-1 == xioctl(&data->camera_fd, VIDIOC_S_FMT, &data->fmt)) {
			ERR("CAMERA_FORMAT_H264: VIDIOC_S_FMT failed, %s\n",
							strerror(errno));
			return -errno;
		}
		break;
	default:
		ERR("unknown option(%d) for format\n", data->format);
		return -1;
		break;
	};
#endif
	/* Buggy driver paranoia. */
	min = data->fmt.fmt.pix.width * 2;
	if (data->fmt.fmt.pix.bytesperline < min)
		data->fmt.fmt.pix.bytesperline = min;

	min = data->fmt.fmt.pix.bytesperline * data->fmt.fmt.pix.height;
	if (data->fmt.fmt.pix.sizeimage < min)
		data->fmt.fmt.pix.sizeimage = min;

	data->camera_buff_size = data->fmt.fmt.pix.sizeimage;

	ret = init_mmap(data);

	return 0;
}

static int read_frame(struct camera_util_data *data)
{
	memset(&data->buf, 0x0, sizeof(data->buf));
	data->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	data->buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(&data->camera_fd, VIDIOC_DQBUF, &data->buf)) {
		switch (errno) {
		case EAGAIN:
			return 0;

		case EIO:
			/* Could ignore EIO, see spec. */

			/* fall through */
			ERR("VIDIOC_DQBUF: EIO\n");
			return -2;
			break;
		default:
			ERR("VIDIOC_DQBUF\n");
			return -1;
			break;
		}
	}

	if (data->buf.index < CAMERA_BUFFER_CNT &&
				data->buf.bytesused == data->camera_buff_size)
		process_image(data);
	else
		ERR("incorrect index: %d, %d, size: %d, %d\n",
					data->buf.index, CAMERA_BUFFER_CNT,
					data->buf.bytesused,
					data->camera_buff_size);
	/*
	DEBUG("camera preview captured: idx: %d, addr: %p, size: %d\n",
		data->buf.index, data->buffers[data->buf.index].start,
		data->buf.bytesused);
	*/
	if (-1 == xioctl(&data->camera_fd, VIDIOC_QBUF, &data->buf)) {
		ERR("VIDIOC_QBUF failed\n");
		return -1;
	}

	return 0;
}

int get_camera_frame(struct camera_util_data *data)
{
	fd_set fds;
	struct timeval tv;
	int r;
	int retry = 5;
	int ret = 0;

	while (retry > 0) {
		FD_ZERO(&fds);
		FD_SET(data->camera_fd, &fds);
		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		r = select(data->camera_fd + 1, &fds, NULL, NULL, &tv);
		if (-1 == r) {
			if (EINTR == errno)
				continue;

			WARNING("select");
			ret = -1;
			break;
		}
		if (0 == r) {
			ERR("select timeout\n");
			ret = -2;
			break;
		}
		if (0 == read_frame(data)) {
			ret = 0;
			break;
		}

		if (0 == --retry) {
			ERR("retry failed\n");
			ret = -3;
			break;
		}
	}
	return ret;
}

int uninit_device(struct camera_util_data *data)
{
	int i;
	for (i = 0; i < CAMERA_BUFFER_CNT; ++i) {
		if (-1 == munmap(data->buffers[i].start,
						data->buffers[i].length)) {
			ERR("munmap buffer[%d] failed\n", i);
			return -1;
		}
	}
	return 0;
}

int open_device(struct camera_util_data *data)
{
	struct stat st;

	DEBUG("open dev: %s\n", data->dev_path);
	if (-1 == stat(data->dev_path, &st)) {
		ERR("Cannot identify '%s': %d, %s\n",
			data->dev_path, errno, strerror(errno));
		return -errno;
	}

	if (!S_ISCHR(st.st_mode)) {
		ERR("%s is no device\n", data->dev_path);
		return -errno;
	}

	data->camera_fd = open(data->dev_path, O_RDWR | O_NONBLOCK, 0);

	if (-1 == data->camera_fd) {
		ERR("Cannot open '%s': %d, %s\n",
			data->dev_path, errno, strerror(errno));
		return -errno;
	}
	return 0;
}

int close_device(struct camera_util_data *data)
{
	if (-1 == close(data->camera_fd)) {
		ERR("close %s failed, %s\n", data->dev_path,
							strerror(errno));
		return -errno;
	}
	return 0;
}

int start_camera_preview(struct camera_util_data *data)
{
	int ret;

	DEBUG("enter\n");
	ret = open_device(data);
	if (ret != 0) {
		ERR("open_device %s failed(ret: %d)\n", data->dev_path, ret);
		return ret;
	}
	ret = init_device(data);
	if (ret != 0) {
		ERR("init_device failed(ret: %d)\n", ret);
		return ret;
	}
	ret = start_capturing(data);
	if (ret != 0) {
		ERR("start_capturing failed(ret: %d)\n", ret);
		return ret;
	}
	DEBUG(" exit\n");
	return 0;
}

int stop_camera_preview(struct camera_util_data *data)
{
	int ret;

	DEBUG("enter\n");
	ret = stop_capturing(data);
	if (ret != 0) {
		ERR("stop_capturing failed(ret: %d)\n", ret);
		return ret;
	}
	ret = uninit_device(data);
	if (ret != 0) {
		ERR("uninit_device failed(ret: %d)\n", ret);
		return ret;
	}
	ret = close_device(data);
	if (ret != 0) {
		ERR("close_device failed(ret: %d)\n", ret);
		return ret;
	}
	DEBUG(" exit\n");
	return 0;
}

static int check_discrete_fps_threshold(int fd, unsigned int fmt,
					unsigned int width, unsigned int height)
{
	const float expected_fps_threshold = 30;
	struct v4l2_frmivalenum frmival;
	float fps;
	int ret = 0;

	memset(&frmival, 0, sizeof(frmival));
	frmival.pixel_format = fmt;
	frmival.width = width;
	frmival.height = height;


	while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0) {
		if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
			fps = 1.0 * frmival.discrete.denominator /
					frmival.discrete.numerator;
			DEBUG("camera res: %dx%d, fps: %.0f\n",
						width, height, fps);
			if (fps < expected_fps_threshold)
				ret = -1;

		} /* ------ NOT TEST THEM -----
		else {
			DEBUG("[%dx%d] [%f,%f] fps\n", width, height,
				1.0 * frmival.stepwise.max.denominator /
				frmival.stepwise.max.numerator,
				1.0 * frmival.stepwise.min.denominator /
					frmival.stepwise.min.numerator);
		*/
		frmival.index++;
	}

	return ret;
}

/* check each fps are over 30fps due to hw issue */
int check_fps_is_meet_requirement(struct camera_util_data *data)
{
	struct v4l2_frmsizeenum frmsize;
	struct v4l2_fmtdesc fmt;
	int ret = -1;
	unsigned int check_cnt = 0;
	unsigned int meet_cnt = 0;

	memset(&frmsize, 0, sizeof(frmsize));
	frmsize.pixel_format = V4L2_PIX_FMT_JPEG;

	memset(&fmt, 0, sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (ioctl(data->camera_fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
		frmsize.pixel_format = fmt.pixelformat;
		frmsize.index = 0;
		while (ioctl(data->camera_fd, VIDIOC_ENUM_FRAMESIZES,
							&frmsize) >= 0) {
			check_cnt++;
			if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				ret = check_discrete_fps_threshold(
						data->camera_fd,
						frmsize.pixel_format,
						frmsize.discrete.width,
						frmsize.discrete.height);
				if (ret == 0)
					meet_cnt++;
			} /* ------ NOT TEST THEM -----
			else {
				for (width=frmsize.stepwise.min_width;
					width< frmsize.stepwise.max_width;
					width+=frmsize.stepwise.step_width)
				for (height=frmsize.stepwise.min_height;
					height< frmsize.stepwise.max_height;
					height+=frmsize.stepwise.step_height)
					ret = check_discrete_fps_threshold(
						data->camera_fd,
						frmsize.pixel_format,
						frmsize.discrete.width,
						frmsize.discrete.height);
					if (ret != 0)
						break;
			}
			*/
			frmsize.index++;
		}
		fmt.index++;
	}
	if (check_cnt != meet_cnt)
		ret = -1;

	DEBUG("check_cnt: %d, meet_cnt: %d, ret: %d\n",
						check_cnt, meet_cnt, ret);
	return ret;
}
