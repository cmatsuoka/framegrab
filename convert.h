#ifndef FRAMEGRAB_CONVERT_H_
#define FRAMEGRAB_CONVERT_H_

#define CLAMP(x,y,z) do { \
	if ((x) < (y)) (x) = (y); \
	else if ((x) > (z)) (x) = (z); \
} while (0)

void yuyv_to_rgb(unsigned char *, unsigned char *, int, int);

#endif
