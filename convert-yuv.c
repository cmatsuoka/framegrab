/*
 * Copyright (C) 2013 Claudio Matsuoka
 * Copyright (C) 2013 CITS - Centro Internacional de Tecnologia de Software
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include "convert.h"

typedef unsigned char uint8;

static inline void yuv2rgb(int y, int u, int v, uint8 *r, uint8 *g, uint8 *b)
{
	int r1, g1, b1;
	int c = y - 16, d = u - 128, e = v - 128;

	r1 = (298 * c + 409 * e + 128) >> 8;
	g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
	b1 = (298 * c + 516 * d + 128) >> 8;

	CLAMP(r1, 0, 255);
	CLAMP(g1, 0, 255);
	CLAMP(b1, 0, 255);

	*r = r1;
	*g = g1;
	*b = b1;
}

void yuyv_to_rgb(uint8 *out_buf, uint8 *in_buf, int width, int height)
{
	int i, size;

	size = width * height / 2;

	for (i = 0; i < size; i++) {
		int y, u, v, y2;
		uint8 r, g, b;

		y  = in_buf[0];
		u  = in_buf[1];
		y2 = in_buf[2];
		v  = in_buf[3];

		yuv2rgb(y, u, v, &r, &g, &b);	/* 1st pixel */

		*out_buf++ = r;
		*out_buf++ = g;
		*out_buf++ = b;

		yuv2rgb(y2, u, v, &r, &g, &b);	/* 2nd pixel */

		*out_buf++ = r;
		*out_buf++ = g;
		*out_buf++ = b;

		in_buf += 4;
	}
}

void yuyv_to_grayscale(uint8 *out_buf, uint8 *in_buf, int width, int height)
{
	int i, size;

	size = width * height / 2;

	for (i = 0; i < size; i++) {
		int y, u, v, y2;
		uint8 r, g, b;

		y  = in_buf[0];
		u  = in_buf[1];
		y2 = in_buf[2];
		v  = in_buf[3];

		yuv2rgb(y, u, v, &r, &g, &b);	/* 1st pixel */

		*out_buf++ = (76 * r + 150 * g + 29 * b) >> 8;

		yuv2rgb(y2, u, v, &r, &g, &b);	/* 2nd pixel */

		*out_buf++ = (76 * r + 150 * g + 29 * b) >> 8;

		in_buf += 4;
	}
}


