#include <string.h>
#include "framegrab.h"
#include "convert.h"


int fg_convert_rgb(void *data, void *raw, struct fg_image *image)
{
	switch (image->format) {
	case FG_FORMAT_YUYV:
		yuyv_to_rgb(data, raw, image->width, image->height);
		break;
	case FG_FORMAT_RGB24:
		memcpy(data, raw, image->width * image->height * 3);
		break;
	default:
		return -1;
	};

	return 0;
}

int fg_convert_grayscale(void *data, void *raw, struct fg_image *image)
{
	switch (image->format) {
	case FG_FORMAT_YUYV:
		yuyv_to_grayscale(data, raw, image->width, image->height);
		break;
	case FG_FORMAT_RGB24:
	default:
		return -1;
	};

	return 0;
}

