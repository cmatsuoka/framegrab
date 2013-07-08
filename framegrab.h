#ifndef FRAMEGRAB_H_
#define FRAMEGRAB_H_

typedef void *fg_handle;

#define FG_FORMAT_YUYV	0
#define FG_FORMAT_RGB24	1

struct fg_image {
	int width;
	int height;
	int format;
	int bytes_per_pixel;
	unsigned char *data;
};

fg_handle fg_init(char *);
int fg_deinit(fg_handle);
int fg_start(fg_handle);
int fg_stop(fg_handle);
int fg_set_format(fg_handle, struct fg_image *);
int fg_get_frame(fg_handle, struct fg_image *);
struct fg_image *fg_create_image(int, int, int);
void fg_destroy_image(struct fg_image *);
int fg_write_jpeg(char *, int, struct fg_image *);

#endif
