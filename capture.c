#include "framegrab.h"
#include "capture.h"

extern struct fg_driver v4l2_driver;

static struct fg_driver *driver = &v4l2_driver;

fg_handle fg_init(char *dev)
{
	return driver->init(dev);
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

int fg_set_format(fg_handle handle, int width, int height)
{
	return driver->set_format(handle, width, height);
}

int fg_get_frame(fg_handle handle, void *data)
{
	return driver->get_frame(handle, data);
}
