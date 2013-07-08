#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <errno.h>
#include "framegrab.h"
#include "capture.h"

#define MAX_FORMATS 8

struct handle_data {
	int fd;
	int num_formats;
	struct v4l2_capability capability;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format format;
	struct v4l2_requestbuffers requestbuffers;
	struct v4l2_fmtdesc fmtdesc[MAX_FORMATS];
	struct buffer *buffers;
};

struct buffer {
	void *start;
	size_t length;
};


static int get_capabilities(struct handle_data *h)
{
	int i;

	if (ioctl(h->fd, VIDIOC_QUERYCAP, &h->capability) < 0) {
		perror("VIDIOC_QUERYCAP");
		return -1;
	}

	if ((h->capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
		fprintf(stderr, "can't read/write from device\n");
		return -1;
	}

	for (i = 0; i < MAX_FORMATS; i++) {
		h->num_formats = i;
		h->fmtdesc[i].index = i;
		h->fmtdesc[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (ioctl(h->fd, VIDIOC_ENUM_FMT, &h->fmtdesc[i]) < 0)
			break;

		printf("format: %s\n", h->fmtdesc[i].description);
	}

	return 0;
}

static fg_handle init(char *dev)
{
	struct handle_data *h;

	if ((h = malloc(sizeof(struct handle_data))) == NULL) {
		perror("malloc");
		goto err;
	}

	if ((h->fd = open(dev, O_RDWR)) < 0) {
		perror(dev);
		goto err1;
	}

	if (get_capabilities(h) < 0) {
		goto err2;
	}

	memset(&h->cropcap, 0, sizeof (struct v4l2_cropcap));
	h->cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(h->fd, VIDIOC_CROPCAP, &h->cropcap) == 0) {
		h->crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		h->crop.c = h->cropcap.defrect;
		ioctl(h->fd, VIDIOC_S_CROP, &h->crop);
	}

	/* set a default format */
        memset(&h->format, 0, sizeof (struct v4l2_format));
        h->format.type = h->fmtdesc[0].type;
        h->format.fmt.pix.field = V4L2_FIELD_ANY;
        h->format.fmt.pix.pixelformat = h->fmtdesc[0].pixelformat;
        h->format.fmt.pix.width = 640;
        h->format.fmt.pix.height = 480;

	if (ioctl(h->fd, VIDIOC_S_FMT, &h->format) < 0) {
		perror("VIDIOC_S_FMT");
		goto err2;
	}

	h->buffers = calloc(h->requestbuffers.count, sizeof (struct buffer));
	if (h->buffers == NULL) {
		fprintf(stderr, "can't alloc buffers\n");
		goto err2;
	}


	return (fg_handle) h;

      err2:
	close(h->fd);
      err1:
	free(h);
      err:
	return NULL;
}

static int deinit(fg_handle handle)
{
	struct handle_data *h = (struct handle_data *)handle;
	int ret;

	if ((ret = close(h->fd)) < 0)
		return -1;

	free(h->buffers);
	free(h);

	return 0;
}

static int start_streaming(fg_handle handle)
{
	struct handle_data *h = (struct handle_data *)handle;
	enum v4l2_buf_type buf_type;
	int i;

	h->requestbuffers.count = 1;
	h->requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	h->requestbuffers.memory = V4L2_MEMORY_MMAP;
	if (ioctl(h->fd, VIDIOC_REQBUFS, &h->requestbuffers) < 0) {
		perror("VIDIOC_REQBUFS");
		return -1;
	}

	for (i = 0; i < h->requestbuffers.count; i++) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof (struct v4l2_buffer));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (ioctl(h->fd, VIDIOC_QUERYBUF, &buf) < 0) {
			perror("VIDIOC_QUERYBUF");
			return -1;
		}

		h->buffers[i].length = buf.length;
		h->buffers[i].start = mmap(NULL, buf.length, PROT_READ |
					   PROT_WRITE, MAP_SHARED, h->fd,
					   buf.m.offset);
		if (h->buffers[i].start == MAP_FAILED) {
			perror("mmap");
			return -1;
		}
	}

	buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(h->fd, VIDIOC_STREAMON, &buf_type) < 0) {
		perror("VIDIOC_STREAMON");
		return -1;
	}

	return 0;
}

static int stop_streaming(fg_handle handle)
{
	struct handle_data *h = (struct handle_data *)handle;
	enum v4l2_buf_type buf_type;
	int i;

	buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(h->fd, VIDIOC_STREAMOFF, &buf_type) < 0) {
		perror("VIDIOC_STREAMOFF");
		return -1;
	}

	for (i = 0; i < h->requestbuffers.count; i++) {
        	if (munmap(h->buffers[i].start, h->buffers[i].length) < 0)
			perror("munmap");
	}
 
	return 0;
}

static int set_format(fg_handle handle, int width, int height)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_requestbuffers reqbufs;

	/* Can't change the format while buffers are allocated otherwise
	 * we get -EBUSY from ioctl(VIDIOC_S_FMT). */
	reqbufs.count = 0;
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbufs.memory = V4L2_MEMORY_MMAP;
	ioctl(h->fd, VIDIOC_REQBUFS, &reqbufs);

	h->format.type = h->fmtdesc[0].type;
	h->format.fmt.pix.pixelformat = h->fmtdesc[0].pixelformat;
	h->format.fmt.pix.field = V4L2_FIELD_ANY;
	h->format.fmt.pix.width = width;
	h->format.fmt.pix.height = height;
	h->format.fmt.pix.bytesperline = width * 2;	/* FIXME */

	if (ioctl(h->fd, VIDIOC_S_FMT, &h->format) < 0) {
		perror("VIDIOC_S_FMT");
		return -1;
	}

	return 0;
}

static int get_frame(fg_handle handle, void *data)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_buffer buf = { 0 };
	struct timeval tv = { 0 };
	fd_set fds;

	buf.type = h->fmtdesc[0].type;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;
	if (ioctl(h->fd, VIDIOC_QBUF, &buf) < 0) {
		perror("VIDIOC_QBUF");
		return -1;
	}

	FD_ZERO(&fds);
	FD_SET(h->fd, &fds);

	tv.tv_sec = 2;
	if (select(h->fd + 1, &fds, NULL, NULL, &tv) < 0) {
		perror("select");
		return -1;
	}

	if (ioctl(h->fd, VIDIOC_DQBUF, &buf) < 0) {
		perror("VIDIOC_DQBUF");
		return -1;
	}
	printf("length: %ld\n", h->buffers[0].length);

	memcpy(data, h->buffers[0].start, h->buffers[0].length);

	return 0;
}

struct fg_driver v4l2_driver = {
	init,
	deinit,
	start_streaming,
	stop_streaming,
	set_format,
	get_frame
};
