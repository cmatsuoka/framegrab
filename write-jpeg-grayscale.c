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
#include <jpeglib.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "framegrab.h"
#include "convert.h"

int fg_write_jpeg_grayscale(char *filename, int quality, struct fg_image *image, void *raw)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *f;
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int stride;			/* physical row width in image buffer */
	unsigned char *data;

	data = malloc(image->width * image->height);
	if (data == NULL)
		goto err;

	if (fg_convert_grayscale(data, raw, image) < 0)
		goto err1;

	/* Allocate and initialize JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((f = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		goto err1;
	}
	jpeg_stdio_dest(&cinfo, f);

	/* Set parameters for compression */
	cinfo.image_width = image->width;
	cinfo.image_height = image->height;
	cinfo.input_components = 1;	/* # of color components per pixel */
	cinfo.in_color_space = JCS_GRAYSCALE;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	/* Run compressor */
	jpeg_start_compress(&cinfo, TRUE);
	stride = image->width;

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &data[cinfo.next_scanline * stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(f);
	jpeg_destroy_compress(&cinfo);
	free(data);

	return 0;

      err1:
	free(data);
      err:
	return -1;
}
