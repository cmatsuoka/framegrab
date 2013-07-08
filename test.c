#include <stdio.h>
#include <stdlib.h>
#include "framegrab.h"
#include "convert.h"
#include "format.h"

int main(int argc, char **argv)
{
	fg_handle h;
	struct fg_image image;
	unsigned char *data;
	int width, height;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <videodev>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("initialize\n");
	if ((h = fg_init(argv[1])) == NULL) {
		perror("fg_init");
		exit(EXIT_FAILURE);
	}

	width = 640;
	height = 480;
	printf("set format to %dx%d\n", width, height);
	if (fg_set_format(h, width, height) < 0) {
		perror("fg_set_format");
		exit(EXIT_FAILURE);
	}

	printf("start capture\n");
	if (fg_start(h) < 0) {
		perror("fg_start");
		exit(EXIT_FAILURE);
	}

	/* YUYV buffer */
	data = malloc(width * height * 2);
	if (data == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	printf("get a frame\n");
	if (fg_get_frame(h, data) < 0) {
		perror("fg_set_format");
		exit(EXIT_FAILURE);
	}

	/* RGB24 buffer */
	image.width = width;
	image.height = height;
	image.rgb = malloc(width * height * 3);
	if (image.rgb == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	yuyv2rgb(image.rgb, data, width, height);
	write_jpeg("teste.jpg", 80, &image);

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
