#include <stdio.h>
#include <stdlib.h>
#include "framegrab.h"

int main(int argc, char **argv)
{
	fg_handle h;
	struct fg_image image;
	struct fg_device device;
	unsigned char *data;
	int len, val;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <videodev>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("initialize\n");
	if ((h = fg_init(argv[1], FG_FORMAT_YUYV)) == NULL) {
		fprintf(stderr, "Error: fg_init\n");
		exit(EXIT_FAILURE);
	}

	fg_get_device_info(h, &device);
	printf("driver: %s\n", device.driver);
	printf("card: %s\n", device.card);
	printf("bus info: %s\n", device.bus_info);
	printf("version: %#08x\n", device.version);

	image.width = 640;
	image.height = 480;
	image.format = FG_FORMAT_YUYV;

	printf("set format to %dx%d\n", image.width, image.height);
	if (fg_set_format(h, &image) < 0) {
		fprintf(stderr, "Error: fg_set_format\n");
		exit(EXIT_FAILURE);
	}

	printf("get format: ");
	if (fg_get_format(h, &image) < 0) {
		fprintf(stderr, "Error: fg_get_format\n");
		exit(EXIT_FAILURE);
	}
	printf("%dx%d\n", image.width, image.height);

	printf("start capture\n");
	if (fg_start(h) < 0) {
		fprintf(stderr, "Error: fg_start\n");
		exit(EXIT_FAILURE);
	}

	len = fg_get_image_size(&image);
	if ((data = malloc(len)) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	printf("get a frame\n");
	if (fg_get_frame(h, data, len) < 0) {
		fprintf(stderr, "Error: fg_set_format\n");
		exit(EXIT_FAILURE);
	}

	if (fg_write_jpeg("test-color.jpg", &image, data, 0, 80) < 0) {
		fprintf(stderr, "Error: fg_write_jpeg\n");
		exit(EXIT_FAILURE);
	}
	if (fg_write_jpeg("test-grayscale.jpg", &image, data, FG_GRAYSCALE, 80) < 0) {
		fprintf(stderr, "Error: fg_write_jpeg\n");
		exit(EXIT_FAILURE);
	}
	if (fg_write_gif("test-color-256.gif", &image, data, 0, 8) < 0) {
		fprintf(stderr, "Error: fg_write_gif\n");
		exit(EXIT_FAILURE);
	}
	if (fg_write_gif("test-color-64.gif", &image, data, 0, 6) < 0) {
		fprintf(stderr, "Error: fg_write_gif\n");
		exit(EXIT_FAILURE);
	}
	if (fg_write_gif("test-color-16.gif", &image, data, 0, 4) < 0) {
		fprintf(stderr, "Error: fg_write_gif\n");
		exit(EXIT_FAILURE);
	}
	if (fg_write_gif("test-color-4.gif", &image, data, 0, 2) < 0) {
		fprintf(stderr, "Error: fg_write_gif\n");
		exit(EXIT_FAILURE);
	}
	if (fg_write_gif("test-grayscale.gif", &image, data, FG_GRAYSCALE, 8) < 0) {
		fprintf(stderr, "Error: fg_write_gif\n");
		exit(EXIT_FAILURE);
	}

	printf("get brightness = ");
	if ((val = fg_get_control(h, FG_CTRL_BRIGHTNESS)) < 0) {
		fprintf(stderr, "Error: fg_get_control\n");
		exit(EXIT_FAILURE);
	}
	printf("%d\n", val);

	printf("set brightness to 30\n");
	if (fg_set_control(h, FG_CTRL_BRIGHTNESS, 30) < 0) {
		fprintf(stderr, "Error: fg_set_control\n");
		exit(EXIT_FAILURE);
	}

	printf("get a frame\n");
	if (fg_get_frame(h, data, len) < 0) {
		fprintf(stderr, "Error: fg_set_format\n");
		exit(EXIT_FAILURE);
	}

	if (fg_write_jpeg("test-brightness-30.jpg", &image, data, 0, 80) < 0) {
		fprintf(stderr, "Error: fg_write_jpeg\n");
		exit(EXIT_FAILURE);
	}

	printf("set brightness to 230\n");
	if (fg_set_control(h, FG_CTRL_BRIGHTNESS, 230) < 0) {
		fprintf(stderr, "Error: fg_set_control\n");
		exit(EXIT_FAILURE);
	}

	printf("get a frame\n");
	if (fg_get_frame(h, data, len) < 0) {
		fprintf(stderr, "Error: fg_set_format\n");
		exit(EXIT_FAILURE);
	}

	if (fg_write_jpeg("test-brightness-230.jpg", &image, data, 0, 80) < 0) {
		fprintf(stderr, "Error: fg_write_jpeg\n");
		exit(EXIT_FAILURE);
	}
	free(data);

	printf("stop capture\n");
	if (fg_stop(h) < 0) {
		fprintf(stderr, "Error: fg_stop\n");
		exit(EXIT_FAILURE);
	}

	printf("deinitialize\n");
	if (fg_deinit(h) < 0) {
		fprintf(stderr, "Error: fg_deinit\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
