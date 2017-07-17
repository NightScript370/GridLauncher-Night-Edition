/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include "pngloader.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

#include "logText.h"

int x, y;

int pngWidth, pngHeight;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

bool read_png_file(char* file_name) {
	char header[8];	// 8 is the maximum size that can be checked

	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if (!fp) {
		return false;
	}

	fread(header, 1, 8, fp);
	if (png_sig_cmp(png_byte, 0, 8)) {
		logText("[read_png_file] File is not recognized as a PNG file");
		fclose(fp);
		return false;
	}


	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		logText("[read_png_file] png_create_read_struct failed");
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		logText("[read_png_file] png_create_info_struct failed");
		fclose(fp);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		logText("[read_png_file] Error during init_io");
		fclose(fp);
		return false;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	pngWidth = png_get_image_width(png_ptr, info_ptr);
	pngHeight = png_get_image_height(png_ptr, info_ptr);

	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);


	/* read file */
	if (setjmp(png_jmpbuf(png_ptr))) {
		logText("[read_png_file] Error during read_image");
		fclose(fp);
		return false;
	}

	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * pngHeight);
	for (y=0; y<pngHeight; y++)
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

	png_read_image(png_ptr, row_pointers);

	fclose(fp);

	return true;
}

int bytesPerPixel;

u8 * process_png_file(void) {
	bytesPerPixel = 3;

	if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGBA) {
		bytesPerPixel = 4;
	}


	u8 * out = malloc(pngWidth*pngHeight*bytesPerPixel);

	for (y=0; y<pngHeight; y++) {
		int moveRight = pngHeight - y;

		png_byte* row = row_pointers[y];
		for (x=0; x<pngWidth; x++) {
			int moveDown = x * pngHeight;

			png_byte* ptr = &(row[x*bytesPerPixel]);

			int offset = (moveRight + moveDown) * bytesPerPixel;
			offset -= bytesPerPixel;

			out[offset] = ptr[2];
			out[offset+1] = ptr[1];
			out[offset+2] = ptr[0];

			if (bytesPerPixel == 4) {
				out[offset+3] = ptr[3];
			}
		}
	}

	return out;
}

