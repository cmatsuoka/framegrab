#ifndef FRAMEGRAB_COMMON_H_
#define FRAMEGRAB_COMMON_H_

#include "framegrab.h"

struct fg_driver {
	fg_handle (*init)(char *, unsigned);
	int (*deinit)(fg_handle);
	int (*start)(fg_handle);
	int (*stop)(fg_handle);
	int (*set_format)(fg_handle, struct fg_image *);
	int (*get_format)(fg_handle, struct fg_image *);
	int (*get_frame)(fg_handle, void *, size_t);
	int (*get_device_info)(fg_handle, struct fg_device *);
};

#endif
