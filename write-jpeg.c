#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#include "framegrab.h"

int write_jpeg(char *filename, int quality, struct fg_image *data)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *f;
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int stride;			/* physical row width in image buffer */

	/* Allocate and initialize JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((f = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return -1;
	}
	jpeg_stdio_dest(&cinfo, f);

	/* Set parameters for compression */
	cinfo.image_width = data->width;
	cinfo.image_height = data->height;
	cinfo.input_components = 3;	/* # of color components per pixel */

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	/* Run compressor */
	jpeg_start_compress(&cinfo, TRUE);
	stride = data->width * 3;

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &data->rgb[cinfo.next_scanline * stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(f);

	jpeg_destroy_compress(&cinfo);

	return 0;
}

/*
 * SOME FINE POINTS:
 *
 * In the above loop, we ignored the return value of jpeg_write_scanlines,
 * which is the number of scanlines actually written.  We could get away
 * with this because we were only relying on the value of cinfo.next_scanline,
 * which will be incremented correctly.  If you maintain additional loop
 * variables then you should be careful to increment them properly.
 * Actually, for output to a stdio stream you needn't worry, because
 * then jpeg_write_scanlines will write all the lines passed (or else exit
 * with a fatal error).  Partial writes can only occur if you use a data
 * destination module that can demand suspension of the compressor.
 * (If you don't know what that's for, you don't need it.)
 *
 * If the compressor requires full-image buffers (for entropy-coding
 * optimization or a multi-scan JPEG file), it will create temporary
 * files for anything that doesn't fit within the maximum-memory setting.
 * (Note that temp files are NOT needed if you use the default parameters.)
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.txt.
 *
 * Scanlines MUST be supplied in top-to-bottom order if you want your JPEG
 * files to be compatible with everyone else's.  If you cannot readily read
 * your data in that order, you'll need an intermediate array to hold the
 * image.  See rdtarga.c or rdbmp.c for examples of handling bottom-to-top
 * source data using the JPEG code's internal virtual-array mechanisms.
 */
