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

#include <stdio.h>
#include "gif_lib.h"
#include "framegrab.h"
#include "convert.h"

#define OLD_API

#ifdef OLD_API
#define GifMakeMapObject MakeMapObject
#define GifQuantizeBuffer QuantizeBuffer
#define EGifOpen_(x,y,z) EGifOpen(x,y)
#else
#define EGifOpen_(x,y,z) EGifOpen(x,y,z)
#endif

static int write_data(GifFileType* file, const GifByteType* data, int len)
{
	return fwrite(data, 1, len, (FILE *)file->UserData);
}

static void decode_planes(unsigned char *r, unsigned char *g, unsigned char *b,
		 unsigned char *rgb, int length)
{
	while (length--) {
		*r++ = *rgb++;
		*g++ = *rgb++;
		*b++ = *rgb++;
	}
}

int fg_write_gif(char *filename, struct fg_image *image, void *raw, int flags)
{
	GifFileType *file;
	ColorMapObject* cmap;
	unsigned char *data, *r, *g, *b, *qdata, *q;
	FILE *f;
	int i, len, colors = 256;
#ifndef OLD_API
	int error;
#endif

	len = image->width * image->height;

	if ((data = malloc(len * 3)) == NULL)
		goto err;

	if ((r = malloc(len)) == NULL)
		goto err1;

	if ((g = malloc(len)) == NULL)
		goto err2;

	if ((b = malloc(len)) == NULL)
		goto err3;

	if ((qdata = malloc(len)) == NULL)
		goto err4;

	if (fg_convert_rgb(data, raw, image) < 0)
		goto err5;

	decode_planes(r, g, b, data, len);

	/* quantize colors */
	if ((cmap = GifMakeMapObject(colors, NULL)) == NULL)
		goto err5;

	if (GifQuantizeBuffer(image->width, image->height, &colors, r, g, b,
					qdata, cmap->Colors) == GIF_ERROR) {
		fprintf(stderr, "quantization error\n");
		goto err6;
	}

	if ((f = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		goto err6;
	}

	if ((file = EGifOpen_(f, write_data, &error)) == NULL) {
		perror("EGifOpen");
		goto err7;
	}

	/* write screen description */
	if (EGifPutScreenDesc(file, image->width, image->height, 8, 0, cmap)
							== GIF_ERROR) {
		perror("EGifPutScreenDesc");
		goto err8;
	}

	/* write image description */
	if (EGifPutImageDesc(file, 0, 0, image->width, image->height, 0, NULL)
							== GIF_ERROR) {
		perror("EGifPutImageDesc");
		goto err8;
	}

	/* write data */
	q = qdata;
	for (i = 0;  i < image->height; i++) {
		if (EGifPutLine(file, q, image->width) == GIF_ERROR) {
			perror("EGifPutLine");
			goto err8;
		}

		q += image->width;
	}

	EGifCloseFile(file);
	fclose(f);
	free(cmap->Colors);
	free(cmap);
	free(qdata);
	free(b);
	free(g);
	free(r);
	free(data);

	return 0;

      err8:
	EGifCloseFile(file);
      err7:
	fclose(f);
      err6:
	free(cmap->Colors);
	free(cmap);
      err5:
	free(qdata);
      err4:
	free(b);
      err3:
	free(g);
      err2:
	free(r);
      err1:
	free(data);
      err:
	return -1;
}
