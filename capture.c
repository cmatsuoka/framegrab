#include "framegrab.h"
#include "capture.h"

extern struct fg_driver v4l2_driver;

static struct fg_driver *driver = &v4l2_driver;

fg_handle fg_init(char *dev, int format)
{
	return driver->init(dev, format);
}

int fg_deinit(fg_handle handle)
{
	return driver->deinit(handle);
}

int fg_start(fg_handle handle)
{
	return driver->start(handle);
}

int fg_stop(fg_handle handle)
{
	return driver->stop(handle);
}

int fg_set_format(fg_handle handle, struct fg_image *image)
{
	return driver->set_format(handle, image);
}

int fg_get_format(fg_handle handle, struct fg_image *image)
{
	return driver->get_format(handle, image);
}

int fg_set_control(fg_handle handle, int parm, int val)
{
	return driver->set_control(handle, parm, val);
}

int fg_get_control(fg_handle handle, int parm)
{
	return driver->get_control(handle, parm);
}

int fg_get_frame(fg_handle handle, void *data, size_t len)
{
	return driver->get_frame(handle, data, len);
}

int fg_get_device_info(fg_handle handle, struct fg_device *info)
{
	return driver->get_device_info(handle, info);
}

int fg_get_image_size(struct fg_image *image)
{
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

	return image->width * image->height * bytes_per_pixel;
}
