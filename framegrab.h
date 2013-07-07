
#ifndef FRAMEGRAB_H_
#define FRAMEGRAB_H_

typedef void *fg_handle;

struct fg_image {
	int width;
	int height;
	unsigned char *rgb;
};

struct fg_format {
	int width;
	int height;
	int bytes_per_pixel;
};


fg_handle fg_init(char *);
int fg_deinit(fg_handle);
int fg_start(fg_handle);
int fg_stop(fg_handle);
int fg_set_format(fg_handle, struct fg_format *);
int fg_get_frame(fg_handle, void *);

#endif
