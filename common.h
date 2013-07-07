#ifndef FRAMEGRAB_COMMON_H_
#define FRAMEGRAB_COMMON_H_

#include "framegrab.h"

struct fg_driver {
	fg_handle (*init)(char *);
	int (*deinit)(fg_handle);
	int (*start)(fg_handle);
	int (*stop)(fg_handle);
	int (*set_format)(fg_handle, struct fg_format *);
	int (*get_frame)(fg_handle, void *);
};

#endif
