
#include <stdlib.h>
#include "framegrab.h"


struct fg_image *fg_create_image(int width, int height, int format)
{
	struct fg_image *image;

	if ((image = malloc(sizeof (struct fg_image))) == NULL)
		goto err;

	image->width = width;
	image->height = height;
	image->format = format;

	switch (format) {
	case FG_FORMAT_YUYV:
		image->bytes_per_pixel = 2;
		break;
	case FG_FORMAT_RGB24:
		image->bytes_per_pixel = 3;
		break;
	default:
		goto err1;
	}
 
	image->data = malloc(width * height * image->bytes_per_pixel);
	if (image->data == NULL)
		goto err1;

	return image;

      err1:
	free(image);
      err:
	return NULL;
}


void fg_destroy_image(struct fg_image *image)
{
	free(image->data);
	free(image);
}
