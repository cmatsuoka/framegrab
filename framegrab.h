#ifndef FRAMEGRAB_H_
#define FRAMEGRAB_H_

#include <unistd.h>

typedef void *fg_handle;

/* fourcc constants */
#define FG_FORMAT_YUYV	0x56595559
#define FG_FORMAT_RGB24	0x33424752

struct fg_image {
	int width;
	int height;
	int format;
};

fg_handle fg_init(char *, int);
int fg_deinit(fg_handle);
int fg_start(fg_handle);
int fg_stop(fg_handle);
int fg_set_format(fg_handle, struct fg_image *);
int fg_get_format(fg_handle, struct fg_image *);
int fg_get_frame(fg_handle, void *, size_t len);
int fg_write_jpeg(char *, int, struct fg_image *, void *);

#endif
