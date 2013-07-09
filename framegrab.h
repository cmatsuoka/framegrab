#ifndef FRAMEGRAB_H_
#define FRAMEGRAB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
# ifdef BUILDING_DLL
#  define EXPORT __declspec(dllexport)
# else
#  define EXPORT __declspec(dllimport)
# endif
#elif __GNUC__ >= 4 || defined(__HP_cc)
# define EXPORT __attribute__((visibility ("default")))
#elif defined(__SUNPRO_C)
# define EXPORT __global
#else
# define EXPORT 
#endif

typedef void *fg_handle;

/* fourcc constants */
#define FG_FORMAT_YUYV	0x56595559
#define FG_FORMAT_RGB24	0x33424752

struct fg_device {
	char driver[16];
	char card[32];
	char bus_info[32];
	unsigned int version;
};

struct fg_image {
	int width;
	int height;
	int format;
};

EXPORT fg_handle fg_init(char *, int);
EXPORT int fg_deinit(fg_handle);
EXPORT int fg_start(fg_handle);
EXPORT int fg_stop(fg_handle);
EXPORT int fg_set_format(fg_handle, struct fg_image *);
EXPORT int fg_get_format(fg_handle, struct fg_image *);
EXPORT int fg_get_frame(fg_handle, void *, size_t len);
EXPORT int fg_get_device_info(fg_handle, struct fg_device *);
EXPORT int fg_get_image_size(struct fg_image *);
EXPORT int fg_write_jpeg(char *, int, struct fg_image *, void *);

#ifdef __cplusplus
}
#endif

#endif
