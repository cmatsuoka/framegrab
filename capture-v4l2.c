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

struct handle_data {
	int fd;
	struct v4l2_capability capability;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format format;
	struct v4l2_requestbuffers requestbuffers;
	struct v4l2_fmtdesc fmtdesc;
	struct buffer *buffers;
};

struct buffer {
	void *start;
	size_t length;
};


static int get_capabilities(struct handle_data *h, int pixelformat)
{
	int i;
	struct v4l2_fmtdesc desc = { 0 };

	if (ioctl(h->fd, VIDIOC_QUERYCAP, &h->capability) < 0) {
		perror("VIDIOC_QUERYCAP");
		return -1;
	}

	if ((h->capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
		fprintf(stderr, "can't read/write from device\n");
		return -1;
	}

	for (i = 0; i < 32; i++) {
		desc.index = i;
		desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (ioctl(h->fd, VIDIOC_ENUM_FMT, &desc) < 0)
			break;

		printf("format: %s\n", desc.description);

		if (desc.pixelformat == pixelformat) {
			memcpy(&h->fmtdesc, &desc, sizeof(struct v4l2_fmtdesc));
			return 0;
		}
	}

	return -1;
}

static fg_handle init(char *dev, unsigned pixelformat)
{
	struct handle_data *h;

	if ((h = calloc(1, sizeof(struct handle_data))) == NULL) {
		perror("calloc");
		goto err;
	}

	if ((h->fd = open(dev, O_RDWR)) < 0) {
		perror(dev);
		goto err1;
	}

	if (get_capabilities(h, pixelformat) < 0) {
		goto err2;
	}

	h->cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(h->fd, VIDIOC_CROPCAP, &h->cropcap) == 0) {
		h->crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		h->crop.c = h->cropcap.defrect;
		ioctl(h->fd, VIDIOC_S_CROP, &h->crop);
	}

	/* set a default format */
        h->format.type = h->fmtdesc.type;
        h->format.fmt.pix.field = V4L2_FIELD_ANY;
        h->format.fmt.pix.pixelformat = h->fmtdesc.pixelformat;
        h->format.fmt.pix.width = 640;
        h->format.fmt.pix.height = 480;

	if (ioctl(h->fd, VIDIOC_S_FMT, &h->format) < 0) {
		perror("VIDIOC_S_FMT");
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
		goto err;
	}

	h->buffers = calloc(h->requestbuffers.count, sizeof (struct buffer));
	if (h->buffers == NULL) {
		fprintf(stderr, "can't alloc buffers\n");
		goto err;
	}

	for (i = 0; i < h->requestbuffers.count; i++) {
		struct v4l2_buffer buf = { 0 };

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (ioctl(h->fd, VIDIOC_QUERYBUF, &buf) < 0) {
			perror("VIDIOC_QUERYBUF");
			goto err2;
		}

		h->buffers[i].length = buf.length;
		h->buffers[i].start = mmap(NULL, buf.length, PROT_READ |
					   PROT_WRITE, MAP_SHARED, h->fd,
					   buf.m.offset);
		if (h->buffers[i].start == MAP_FAILED) {
			perror("mmap");
			goto err2;
		}
	}

	buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(h->fd, VIDIOC_STREAMON, &buf_type) < 0) {
		perror("VIDIOC_STREAMON");
		goto err2;
	}

	return 0;

      err2:
	free(h->buffers);
      err:
	return -1;
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

static int set_format(fg_handle handle, struct fg_image *image)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_requestbuffers reqbufs = { 0 };

	/* Can't change the format while buffers are allocated otherwise
	 * we get -EBUSY from ioctl(VIDIOC_S_FMT). */
	reqbufs.count = 0;
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbufs.memory = V4L2_MEMORY_MMAP;
	ioctl(h->fd, VIDIOC_REQBUFS, &reqbufs);

	h->format.type = h->fmtdesc.type;
	h->format.fmt.pix.pixelformat = h->fmtdesc.pixelformat;
	h->format.fmt.pix.field = V4L2_FIELD_ANY;
	h->format.fmt.pix.width = image->width;
	h->format.fmt.pix.height = image->height;
	h->format.fmt.pix.bytesperline = image->width * image->bytes_per_pixel;

	if (ioctl(h->fd, VIDIOC_S_FMT, &h->format) < 0) {
		perror("VIDIOC_S_FMT");
		return -1;
	}

	return 0;
}

static int get_frame(fg_handle handle, struct fg_image *image)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_buffer buf = { 0 };
	struct timeval tv = { 0 };
	fd_set fds;
	int length;

	buf.type = h->fmtdesc.type;
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

	length = image->width * image->height * image->bytes_per_pixel;
	if (length != h->buffers[0].length)
		return -1;

	memcpy(image->data, h->buffers[0].start, h->buffers[0].length);

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
