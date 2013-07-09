#include <stdio.h>
#include <stdlib.h>
#include "framegrab.h"

int main(int argc, char **argv)
{
	fg_handle h;
	struct fg_image image;
	unsigned char *data;
	int len;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <videodev>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("initialize\n");
	if ((h = fg_init(argv[1], FG_FORMAT_YUYV)) == NULL) {
		perror("fg_init");
		exit(EXIT_FAILURE);
	}

	image.width = 640;
	image.height = 480;
	image.format = FG_FORMAT_YUYV;

	printf("set format to %dx%d\n", image.width, image.height);
	if (fg_set_format(h, &image) < 0) {
		perror("fg_set_format");
		exit(EXIT_FAILURE);
	}

	printf("get format: ");
	if (fg_get_format(h, &image) < 0) {
		perror("fg_get_format");
		exit(EXIT_FAILURE);
	}
	printf("%dx%d\n", image.width, image.height);

	printf("start capture\n");
	if (fg_start(h) < 0) {
		perror("fg_start");
		exit(EXIT_FAILURE);
	}

	len = image.width * image.height * 2;
	if ((data = malloc(len)) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	printf("get a frame\n");
	if (fg_get_frame(h, data, len) < 0) {
		perror("fg_set_format");
		exit(EXIT_FAILURE);
	}

	if (fg_write_jpeg("teste.jpg", 80, &image, data) < 0) {
		perror("fg_write_jpeg");
		exit(EXIT_FAILURE);
	}
	free(data);

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
