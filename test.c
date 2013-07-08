#include <stdio.h>
#include <stdlib.h>
#include "framegrab.h"

int main(int argc, char **argv)
{
	fg_handle h;
	struct fg_image *image;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <videodev>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("initialize\n");
	if ((h = fg_init(argv[1])) == NULL) {
		perror("fg_init");
		exit(EXIT_FAILURE);
	}

	if ((image = fg_create_image(640, 480, FG_FORMAT_YUYV)) == NULL) {
		perror("fg_create_image");
		exit(EXIT_FAILURE);
	}

	printf("set format to %dx%d\n", image->width, image->height);
	if (fg_set_format(h, image) < 0) {
		perror("fg_set_format");
		exit(EXIT_FAILURE);
	}

	printf("start capture\n");
	if (fg_start(h) < 0) {
		perror("fg_start");
		exit(EXIT_FAILURE);
	}

	printf("get a frame\n");
	if (fg_get_frame(h, image) < 0) {
		perror("fg_set_format");
		exit(EXIT_FAILURE);
	}

	if (fg_write_jpeg("teste.jpg", 80, image) < 0) {
		perror("fg_write_jpeg");
		exit(EXIT_FAILURE);
	}

	printf("stop capture\n");
	if (fg_stop(h) < 0) {
		perror("fg_stop");
		exit(EXIT_FAILURE);
	}

	printf("deinitialize\n");
	if (fg_deinit(h) < 0) {
		perror("fg_deinit");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
