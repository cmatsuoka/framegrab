/*
 * Copyright (C) 2013 Claudio Matsuoka
 * Copyright (C) 2013 CITS - Centro Internacional de Tecnologia de Software
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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


static int get_capabilities(struct handle_data *h, unsigned pixelformat)
{
	int i;
	struct v4l2_fmtdesc desc;

	memset(&desc, 0, sizeof(struct v4l2_fmtdesc));

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

		/* printf("format: %s\n", desc.description); */

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
	unsigned i;

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
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof (struct v4l2_buffer));
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
	unsigned i;

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

static int get_format(fg_handle handle, struct fg_image *image)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_format format;

	memset(&format, 0, sizeof (struct v4l2_format));
	format.type = h->fmtdesc.type;

	if (ioctl(h->fd, VIDIOC_G_FMT, &format) < 0) {
		perror("VIDIOC_G_FMT");
		return -1;
	}

	image->width = format.fmt.pix.width;
	image->height = format.fmt.pix.height;
	image->format = format.fmt.pix.pixelformat;

	return 0;
}

static int set_format(fg_handle handle, struct fg_image *image)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_requestbuffers reqbufs;
	int bytes_per_pixel;

	switch (image->format) {
	case FG_FORMAT_YUYV:
		bytes_per_pixel = 2;
		break;
	case FG_FORMAT_RGB24:
		bytes_per_pixel = 3;
		break;
	default:
		return -1;
	}

	/* Can't change the format while buffers are allocated otherwise
	 * we get -EBUSY from ioctl(VIDIOC_S_FMT). */
	memset(&reqbufs, 0, sizeof (struct v4l2_requestbuffers));
	reqbufs.count = 0;
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbufs.memory = V4L2_MEMORY_MMAP;
	ioctl(h->fd, VIDIOC_REQBUFS, &reqbufs);

	h->format.type = h->fmtdesc.type;
	h->format.fmt.pix.pixelformat = h->fmtdesc.pixelformat;
	h->format.fmt.pix.field = V4L2_FIELD_ANY;
	h->format.fmt.pix.width = image->width;
	h->format.fmt.pix.height = image->height;
	h->format.fmt.pix.bytesperline = image->width * bytes_per_pixel;

	if (ioctl(h->fd, VIDIOC_S_FMT, &h->format) < 0) {
		perror("VIDIOC_S_FMT");
		return -1;
	}

	return 0;
}

int get_frame(fg_handle handle, void *data, size_t len)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_buffer buf;
	struct timeval tv;
	fd_set fds;
	int i;

	/* Get a few frames to warm up (otherwise some cameras fail) */
	for (i = 0; i < 4; i++) {
		memset(&buf, 0, sizeof (struct v4l2_buffer));
		buf.type = h->fmtdesc.type;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = 0;
		if (ioctl(h->fd, VIDIOC_QBUF, &buf) < 0) {
			perror("VIDIOC_QBUF");
			return -1;
		}
	
		FD_ZERO(&fds);
		FD_SET(h->fd, &fds);
	
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		if (select(h->fd + 1, &fds, NULL, NULL, &tv) < 0) {
			perror("select");
			return -1;
		}
	
		if (ioctl(h->fd, VIDIOC_DQBUF, &buf) < 0) {
			perror("VIDIOC_DQBUF");
			return -1;
		}
	
		if (len != h->buffers[0].length)
			return -1;
	}

	memcpy(data, h->buffers[0].start, h->buffers[0].length);

	return 0;
}

int get_device_info(fg_handle handle, struct fg_device *info)
{
	struct handle_data *h = (struct handle_data *)handle;

	memset(info, 0, sizeof (struct fg_device));
	strncpy(info->driver, (char *)h->capability.driver, 16);
	strncpy(info->card, (char *)h->capability.card, 32);
	strncpy(info->bus_info, (char *)h->capability.bus_info, 32);
	info->version = h->capability.version;

	return 0;
}

int set_control(fg_handle handle, int parm, int val)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;
	int id;

	switch (parm) {
	case FG_CTRL_BRIGHTNESS:
		id = V4L2_CID_BRIGHTNESS;
		break;
	case FG_CTRL_CONTRAST:
		id = V4L2_CID_CONTRAST;
		break;
	case FG_CTRL_SATURATION:
		id = V4L2_CID_SATURATION;
		break;
	case FG_CTRL_HUE:
		id = V4L2_CID_HUE;
		break;
	default:
		return -1;
	}

	memset(&queryctrl, 0, sizeof (struct v4l2_queryctrl));
	queryctrl.id = id;

	if (ioctl(h->fd, VIDIOC_QUERYCTRL, &queryctrl) < 0)
		return -1;

	if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
		return -1;

	memset(&control, 0, sizeof (struct v4l2_control));
	control.id = id;
	control.value = val;

	if (ioctl(h->fd, VIDIOC_S_CTRL, &control) < 0)
		return -1;

	return 0;
}

int get_control(fg_handle handle, int parm)
{
	struct handle_data *h = (struct handle_data *)handle;
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;
	int id;

	switch (parm) {
	case FG_CTRL_BRIGHTNESS:
		id = V4L2_CID_BRIGHTNESS;
		break;
	case FG_CTRL_CONTRAST:
		id = V4L2_CID_CONTRAST;
		break;
	case FG_CTRL_SATURATION:
		id = V4L2_CID_SATURATION;
		break;
	case FG_CTRL_HUE:
		id = V4L2_CID_HUE;
		break;
	default:
		return -1;
	}

	memset(&queryctrl, 0, sizeof (struct v4l2_queryctrl));
	queryctrl.id = id;

	if (ioctl(h->fd, VIDIOC_QUERYCTRL, &queryctrl) < 0)
		return -1;

	if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
		return -1;

	memset(&control, 0, sizeof (struct v4l2_control));
	control.id = id;

	if (ioctl(h->fd, VIDIOC_G_CTRL, &control) < 0)
		return -1;

	return control.value;
}

struct fg_driver v4l2_driver = {
	init,
	deinit,
	start_streaming,
	stop_streaming,
	set_format,
	get_format,
	set_control,
	get_control,
	get_frame,
	get_device_info
};
