#include <stdio.h>
#include <stdlib.h>
#include "framegrab.h"

int main(int argc, char **argv)
{
	fg_handle h;
	struct fg_format fmt;
	unsigned char *data;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <videodev>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("initialize\n");
	if ((h = fg_init(argv[1])) == NULL) {
		perror("fg_init");
		exit(EXIT_FAILURE);
	}

	fmt.width = 640;
	fmt.height = 480;
	fmt.bytes_per_pixel = 3;

	printf("set format to %dx%d\n", fmt.width, fmt.height);
	if (fg_set_format(h, &fmt) < 0) {
		perror("fg_set_format");
		exit(EXIT_FAILURE);
	}

	printf("start capture\n");
	if (fg_start(h) < 0) {
		perror("fg_start");
		exit(EXIT_FAILURE);
	}

	data = malloc(fmt.width * fmt.height * fmt.bytes_per_pixel);
	if (data == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	printf("get a frame\n");
	if (fg_get_frame(h, data) < 0) {
		perror("fg_set_format");
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
