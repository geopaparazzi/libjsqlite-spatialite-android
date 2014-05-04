/*

 rl2png -- PNG related functions

 version 0.1, 2013 March 29

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
the Initial Developer. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <png.h>

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

struct png_memory_buffer
{
    unsigned char *buffer;
    size_t size;
    size_t off;
};

static void
rl2_png_read_data (png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct png_memory_buffer *p = png_get_io_ptr (png_ptr);
    size_t rd = length;
    if (p->off + length > p->size)
	rd = p->size - p->off;
    if (rd != 0)
      {
	  /* copy bytes into buffer */
	  memcpy (data, p->buffer + p->off, rd);
	  p->off += rd;
      }
    if (rd != length)
	png_error (png_ptr, "Read Error: truncated data");
}

static void
rl2_png_write_data (png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct png_memory_buffer *p = png_get_io_ptr (png_ptr);
    size_t nsize = p->size + length;

    /* allocate or grow buffer */
    if (p->buffer)
	p->buffer = realloc (p->buffer, nsize);
    else
	p->buffer = malloc (nsize);

    if (!p->buffer)
	png_error (png_ptr, "Write Error");

    /* copy new bytes to end of buffer */
    memcpy (p->buffer + p->size, data, length);
    p->size += length;
}

static void
rl2_png_flush (png_structp png_ptr)
{
/* just silencing stupid compiler warnings */
    if (png_ptr == NULL)
	png_ptr = NULL;
}

static int
compress_palette_png (const unsigned char *pixels, unsigned short width,
		      unsigned short height, rl2PalettePtr plt,
		      unsigned char sample_type, unsigned char **png,
		      int *png_size)
{
/* compressing a PNG image of the PALETTE type */
    png_structp png_ptr;
    png_infop info_ptr;
    int bit_depth;
    png_bytep *row_pointers = NULL;
    png_bytep p_out;
    int row;
    int col;
    png_color palette[256];
    unsigned short num_entries;
    unsigned char *red = NULL;
    unsigned char *green = NULL;
    unsigned char *blue = NULL;
    int i;
    const unsigned char *p_in;
    struct png_memory_buffer membuf;
    membuf.buffer = NULL;
    membuf.size = 0;

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
	return RL2_ERROR;
    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr)
      {
	  png_destroy_write_struct (&png_ptr, NULL);
	  return RL2_ERROR;
      }
    if (setjmp (png_jmpbuf (png_ptr)))
      {
	  goto error;
      }

    png_set_write_fn (png_ptr, &membuf, rl2_png_write_data, rl2_png_flush);
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
	  bit_depth = 1;
	  break;
      case RL2_SAMPLE_2_BIT:
	  bit_depth = 2;
	  break;
      case RL2_SAMPLE_4_BIT:
	  bit_depth = 4;
	  break;
      case RL2_SAMPLE_UINT8:
	  bit_depth = 8;
	  break;
      };
    png_set_IHDR (png_ptr, info_ptr, width, height, bit_depth,
		  PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
		  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
/* setting the palette */
    if (plt == NULL)
	goto error;
    if (rl2_get_palette_colors (plt, &num_entries, &red, &green, &blue)
	!= RL2_OK)
	goto error;
    for (i = 0; i < num_entries; i++)
      {
	  palette[i].red = *(red + i);
	  palette[i].green = *(green + i);
	  palette[i].blue = *(blue + i);
      }
    png_set_PLTE (png_ptr, info_ptr, palette, num_entries);
    png_write_info (png_ptr, info_ptr);
    png_set_packing (png_ptr);
    row_pointers = malloc (sizeof (png_bytep) * height);
    if (row_pointers == NULL)
	goto error;
    for (row = 0; row < height; ++row)
	row_pointers[row] = NULL;
    p_in = pixels;
    for (row = 0; row < height; row++)
      {
	  if ((row_pointers[row] = malloc (width)) == NULL)
	      goto error;
	  p_out = row_pointers[row];
	  for (col = 0; col < width; col++)
	      *p_out++ = *p_in++;
      }
    png_write_image (png_ptr, row_pointers);
    png_write_end (png_ptr, info_ptr);
    for (row = 0; row < height; ++row)
	free (row_pointers[row]);
    free (row_pointers);
    png_destroy_write_struct (&png_ptr, &info_ptr);
    if (red != NULL)
	rl2_free (red);
    if (green != NULL)
	rl2_free (green);
    if (blue != NULL)
	rl2_free (blue);
    *png = membuf.buffer;
    *png_size = membuf.size;
    return RL2_OK;

  error:
    png_destroy_write_struct (&png_ptr, &info_ptr);
    for (row = 0; row < height; ++row)
	free (row_pointers[row]);
    free (row_pointers);
    if (membuf.buffer != NULL)
	free (membuf.buffer);
    if (red != NULL)
	rl2_free (red);
    if (green != NULL)
	rl2_free (green);
    if (blue != NULL)
	rl2_free (blue);
    return RL2_ERROR;
}

static int
compress_grayscale_png (const unsigned char *pixels, const unsigned char *mask,
			double opacity, unsigned short width,
			unsigned short height, unsigned char sample_type,
			unsigned char pixel_type, unsigned char **png,
			int *png_size)
{
/* compressing a PNG image of the GRAYSCALE type */
    png_structp png_ptr;
    png_infop info_ptr;
    int bit_depth;
    png_bytep *row_pointers = NULL;
    png_bytep p_out;
    int row;
    int col;
    const unsigned char *p_in;
    const unsigned char *p_mask;
    int nBands;
    int type;
    int is_monochrome = 0;
    unsigned char alpha = 255;
    struct png_memory_buffer membuf;
    membuf.buffer = NULL;
    membuf.size = 0;

    if (opacity < 0.0)
	opacity = 0.0;
    if (opacity > 1.0)
	opacity = 1.0;
    if (opacity < 1.0)
	alpha = (unsigned char) (255.0 * opacity);

    if (pixel_type == RL2_PIXEL_MONOCHROME)
	is_monochrome = 1;

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
	return RL2_ERROR;
    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr)
      {
	  png_destroy_write_struct (&png_ptr, NULL);
	  return RL2_ERROR;
      }
    if (setjmp (png_jmpbuf (png_ptr)))
      {
	  goto error;
      }

    png_set_write_fn (png_ptr, &membuf, rl2_png_write_data, rl2_png_flush);
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
	  bit_depth = 1;
	  break;
      case RL2_SAMPLE_2_BIT:
	  bit_depth = 2;
	  break;
      case RL2_SAMPLE_4_BIT:
	  bit_depth = 4;
	  break;
      case RL2_SAMPLE_UINT8:
	  bit_depth = 8;
	  break;
      };
    p_mask = mask;
    type = PNG_COLOR_TYPE_GRAY;
    nBands = 1;
    if (p_mask != NULL && sample_type == RL2_SAMPLE_UINT8)
      {
	  type = PNG_COLOR_TYPE_GRAY_ALPHA;
	  nBands = 2;
      }
    png_set_IHDR (png_ptr, info_ptr, width, height, bit_depth,
		  type, PNG_INTERLACE_NONE,
		  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info (png_ptr, info_ptr);
    png_set_packing (png_ptr);
    row_pointers = malloc (sizeof (png_bytep) * height);
    if (row_pointers == NULL)
	goto error;
    for (row = 0; row < height; ++row)
	row_pointers[row] = NULL;
    p_in = pixels;
    for (row = 0; row < height; row++)
      {
	  if ((row_pointers[row] = malloc (width * nBands)) == NULL)
	      goto error;
	  p_out = row_pointers[row];
	  for (col = 0; col < width; col++)
	    {
		if (is_monochrome)
		  {
		      if (*p_in++ != 0)
			  *p_out++ = 255;
		      else
			  *p_out++ = 0;
		  }
		else
		    *p_out++ = *p_in++;
		if (type == PNG_COLOR_TYPE_GRAY_ALPHA)
		  {
		      /* ALPHA channel */
		      int transparent = *p_mask++;
		      transparent = !transparent;
		      if (transparent)
			  *p_out++ = 0;
		      else
			  *p_out++ = alpha;
		  }
	    }
      }

    png_write_image (png_ptr, row_pointers);
    png_write_end (png_ptr, info_ptr);
    for (row = 0; row < height; ++row)
	free (row_pointers[row]);
    free (row_pointers);
    png_destroy_write_struct (&png_ptr, &info_ptr);
    *png = membuf.buffer;
    *png_size = membuf.size;
    return RL2_OK;

  error:
    png_destroy_write_struct (&png_ptr, &info_ptr);
    for (row = 0; row < height; ++row)
	free (row_pointers[row]);
    free (row_pointers);
    if (membuf.buffer != NULL)
	free (membuf.buffer);
    return RL2_ERROR;
}

static int
compress_rgb_png (const unsigned char *pixels, const unsigned char *mask,
		  double opacity, unsigned short width, unsigned short height,
		  unsigned char **png, int *png_size)
{
/* compressing a PNG image of the RGB type */
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers = NULL;
    png_bytep p_out;
    int row;
    int col;
    const unsigned char *p_in;
    const unsigned char *p_mask;
    int nBands;
    int type;
    unsigned char alpha = 255;
    struct png_memory_buffer membuf;
    membuf.buffer = NULL;
    membuf.size = 0;

    if (opacity < 0.0)
	opacity = 0.0;
    if (opacity > 1.0)
	opacity = 1.0;
    if (opacity < 1.0)
	alpha = (unsigned char) (255.0 * opacity);

    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
	return RL2_ERROR;
    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr)
      {
	  png_destroy_write_struct (&png_ptr, NULL);
	  return RL2_ERROR;
      }
    if (setjmp (png_jmpbuf (png_ptr)))
      {
	  goto error;
      }

    png_set_write_fn (png_ptr, &membuf, rl2_png_write_data, rl2_png_flush);
    p_mask = mask;
    type = PNG_COLOR_TYPE_RGB;
    nBands = 3;
    if (p_mask != NULL)
      {
	  type = PNG_COLOR_TYPE_RGB_ALPHA;
	  nBands = 4;
      }
    png_set_IHDR (png_ptr, info_ptr, width, height, 8,
		  type, PNG_INTERLACE_NONE,
		  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info (png_ptr, info_ptr);
    row_pointers = malloc (sizeof (png_bytep) * height);
    if (row_pointers == NULL)
	goto error;
    for (row = 0; row < height; ++row)
	row_pointers[row] = NULL;
    p_in = pixels;
    for (row = 0; row < height; row++)
      {
	  if ((row_pointers[row] = malloc (width * nBands)) == NULL)
	      goto error;
	  p_out = row_pointers[row];
	  for (col = 0; col < width; col++)
	    {
		*p_out++ = *p_in++;
		*p_out++ = *p_in++;
		*p_out++ = *p_in++;
		if (p_mask != NULL)
		  {
		      /* ALPHA channel */
		      int transparent = *p_mask++;
		      transparent = !transparent;
		      if (transparent)
			  *p_out++ = 0;
		      else
			  *p_out++ = alpha;
		  }
	    }
      }
    png_write_image (png_ptr, row_pointers);
    png_write_end (png_ptr, info_ptr);
    for (row = 0; row < height; ++row)
	free (row_pointers[row]);
    free (row_pointers);
    png_destroy_write_struct (&png_ptr, &info_ptr);
    *png = membuf.buffer;
    *png_size = membuf.size;
    return RL2_OK;

  error:
    png_destroy_write_struct (&png_ptr, &info_ptr);
    for (row = 0; row < height; ++row)
	free (row_pointers[row]);
    free (row_pointers);
    if (membuf.buffer != NULL)
	free (membuf.buffer);
    return RL2_ERROR;
}

static int
check_png_compatibility (unsigned char sample_type, unsigned char pixel_type,
			 unsigned char num_samples)
{
/* checks for PNG compatibility */
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  break;
      default:
	  return RL2_ERROR;
      };
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
	  break;
      default:
	  return RL2_ERROR;
      };
    if (pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_RGB)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 3)
	      return RL2_ERROR;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_section_to_png (rl2SectionPtr scn, const char *path)
{
/* attempting to save a raster section into a PNG file */
    int blob_size;
    unsigned char *blob;
    rl2RasterPtr rst;
    int ret;

    if (scn == NULL)
	return RL2_ERROR;
    rst = rl2_get_section_raster (scn);
    if (rst == NULL)
	return RL2_ERROR;
/* attempting to export as a PNG image */
    if (rl2_raster_to_png (rst, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    ret = rl2_blob_to_file (path, blob, blob_size);
    free (blob);
    if (ret != RL2_OK)
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_to_png (rl2RasterPtr rst, unsigned char **png, int *png_size)
{
/* creating a PNG image from a raster */
    rl2PrivRasterPtr raster = (rl2PrivRasterPtr) rst;
    rl2PalettePtr plt;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_samples;
    unsigned char *blob;
    int blob_size;
    if (rst == NULL)
	return RL2_ERROR;
    if (rl2_get_raster_type (rst, &sample_type, &pixel_type, &num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (check_png_compatibility (sample_type, pixel_type, num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    plt = rl2_get_raster_palette (rst);

    if (rl2_data_to_png
	(raster->rasterBuffer, raster->maskBuffer, 1.0, plt, raster->width,
	 raster->height, sample_type, pixel_type, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    *png = blob;
    *png_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE int
rl2_rgb_to_png (unsigned short width, unsigned short height,
		const unsigned char *rgb, unsigned char **png, int *png_size)
{
/* creating a PNG image from an RGB buffer */
    unsigned char *blob;
    int blob_size;
    if (rgb == NULL)
	return RL2_ERROR;

    if (rl2_data_to_png
	(rgb, NULL, 1.0, NULL, width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB,
	 &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    *png = blob;
    *png_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE int
rl2_rgb_alpha_to_png (unsigned short width, unsigned short height,
		      const unsigned char *rgb, const unsigned char *alpha,
		      unsigned char **png, int *png_size, double opacity)
{
/* creating a PNG image from two distinct RGB + Alpha buffer */
    unsigned char *blob;
    int blob_size;
    if (rgb == NULL || alpha == NULL)
	return RL2_ERROR;

    if (rl2_data_to_png
	(rgb, alpha, opacity, NULL, width, height, RL2_SAMPLE_UINT8,
	 RL2_PIXEL_RGB, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    *png = blob;
    *png_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE int
rl2_gray_to_png (unsigned short width, unsigned short height,
		 const unsigned char *gray, unsigned char **png, int *png_size)
{
/* creating a PNG image from a Grayscale buffer */
    unsigned char *blob;
    int blob_size;
    if (gray == NULL)
	return RL2_ERROR;

    if (rl2_data_to_png
	(gray, NULL, 1.0, NULL, width, height, RL2_SAMPLE_UINT8,
	 RL2_PIXEL_GRAYSCALE, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    *png = blob;
    *png_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE int
rl2_gray_alpha_to_png (unsigned short width, unsigned short height,
		       const unsigned char *gray, const unsigned char *alpha,
		       unsigned char **png, int *png_size, double opacity)
{
/* creating a PNG image from two distinct Grayscale + Alpha buffer */
    unsigned char *blob;
    int blob_size;
    if (gray == NULL)
	return RL2_ERROR;

    if (rl2_data_to_png
	(gray, alpha, opacity, NULL, width, height, RL2_SAMPLE_UINT8,
	 RL2_PIXEL_GRAYSCALE, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    *png = blob;
    *png_size = blob_size;
    return RL2_OK;
}

RL2_PRIVATE int
rl2_data_to_png (const unsigned char *pixels, const unsigned char *mask,
		 double opacity, rl2PalettePtr plt, unsigned short width,
		 unsigned short height, unsigned char sample_type,
		 unsigned char pixel_type, unsigned char **png, int *png_size)
{
/* encoding a PNG image */
    int ret;
    unsigned char *blob;
    int blob_size;

    if (pixels == NULL)
	return RL2_ERROR;
    switch (pixel_type)
      {
      case RL2_PIXEL_PALETTE:
	  ret =
	      compress_palette_png (pixels, width, height, plt, sample_type,
				    &blob, &blob_size);
	  break;
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_GRAYSCALE:
	  ret =
	      compress_grayscale_png (pixels, mask, opacity, width, height,
				      sample_type, pixel_type, &blob,
				      &blob_size);
	  break;
      case RL2_PIXEL_RGB:
	  ret =
	      compress_rgb_png (pixels, mask, opacity, width, height, &blob,
				&blob_size);
	  break;
      };
    if (ret != RL2_OK)
	return RL2_ERROR;
    *png = blob;
    *png_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE rl2SectionPtr
rl2_section_from_png (const char *path)
{
/* attempting to create a raster section from a PNG file */
    int blob_size;
    unsigned char *blob;
    rl2SectionPtr scn;
    rl2RasterPtr rst;

/* attempting to create a raster */
    if (rl2_blob_from_file (path, &blob, &blob_size) != RL2_OK)
	return NULL;
    rst = rl2_raster_from_png (blob, blob_size);
    free (blob);
    if (rst == NULL)
	return NULL;

/* creating the raster section */
    scn =
	rl2_create_section (path, RL2_COMPRESSION_PNG, RL2_TILESIZE_UNDEFINED,
			    RL2_TILESIZE_UNDEFINED, rst);
    return scn;
}

RL2_DECLARE rl2RasterPtr
rl2_raster_from_png (const unsigned char *blob, int blob_size)
{
/* attempting to create a raster from a PNG image */
    rl2RasterPtr rst = NULL;
    unsigned short width;
    unsigned short height;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nBands;
    unsigned char *data = NULL;
    int data_size;
    unsigned char *mask = NULL;
    int mask_sz;
    rl2PalettePtr palette = NULL;

    if (rl2_decode_png
	(blob, blob_size, &width, &height, &sample_type, &pixel_type, &nBands,
	 &data, &data_size, &mask, &mask_sz, &palette) != RL2_OK)
	goto error;
    rst =
	rl2_create_raster (width, height, sample_type, pixel_type, nBands, data,
			   data_size, palette, mask, mask_sz, NULL);
    if (rst == NULL)
	goto error;
    return rst;

  error:
    if (data != NULL)
	free (data);
    if (mask != NULL)
	free (mask);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return NULL;
}

RL2_PRIVATE int
rl2_decode_png (const unsigned char *blob, int blob_size,
		unsigned short *xwidth, unsigned short *xheight,
		unsigned char *xsample_type, unsigned char *xpixel_type,
		unsigned char *num_bands, unsigned char **pixels,
		int *pixels_sz, unsigned char **xmask, int *xmask_sz,
		rl2PalettePtr * xpalette)
{
/* attempting to decode a PNG image - raw block */
    png_uint_32 width;
    png_uint_32 height;
    png_uint_32 rowbytes;
    int bit_depth;
    int color_type;
    int interlace_type;
    png_structp png_ptr;
    png_infop info_ptr;
    struct png_memory_buffer membuf;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    int nBands;
    int i;
    png_colorp palette;
    int red[256];
    int green[256];
    int blue[256];
    int alpha[256];
    unsigned char *data = NULL;
    unsigned char *p_data;
    unsigned char *mask = NULL;
    int mask_sz = 0;
    unsigned char *p_mask = NULL;
    int data_size;
    png_bytep image_data = NULL;
    png_bytepp row_pointers = NULL;
    int row;
    int col;
    rl2PalettePtr rl_palette = NULL;
    int nPalette = 0;
    png_bytep transp;
    int nTransp;
    png_color_16p transpValues;
    int has_alpha = 0;

    if (blob == NULL || blob_size == 0)
	return RL2_ERROR;
    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
	return RL2_ERROR;
    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr)
      {
	  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	  return RL2_ERROR;
      }
    if (setjmp (png_jmpbuf (png_ptr)))
      {
	  goto error;
      }

    membuf.buffer = (unsigned char *) blob;
    membuf.size = blob_size;
    membuf.off = 0;
    png_set_read_fn (png_ptr, &membuf, rl2_png_read_data);
    png_read_info (png_ptr, info_ptr);
    png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		  &interlace_type, NULL, NULL);
    switch (bit_depth)
      {
      case 1:
	  sample_type = RL2_SAMPLE_1_BIT;
	  break;
      case 2:
	  sample_type = RL2_SAMPLE_2_BIT;
	  break;
      case 4:
	  sample_type = RL2_SAMPLE_4_BIT;
	  break;
      case 8:
	  sample_type = RL2_SAMPLE_UINT8;
	  break;
      };
    if (bit_depth < 8)
	png_set_packing (png_ptr);
    switch (color_type)
      {
      case PNG_COLOR_TYPE_PALETTE:
	  pixel_type = RL2_PIXEL_PALETTE;
	  nBands = 1;
	  png_get_PLTE (png_ptr, info_ptr, &palette, &nPalette);
	  for (i = 0; i < nPalette; i++)
	    {
		red[i] = palette[i].red;
		green[i] = palette[i].green;
		blue[i] = palette[i].blue;
		alpha[i] = 255;
	    }
	  break;
      case PNG_COLOR_TYPE_GRAY:
      case PNG_COLOR_TYPE_GRAY_ALPHA:
	  pixel_type = RL2_PIXEL_GRAYSCALE;
	  if (sample_type == RL2_SAMPLE_1_BIT)
	      pixel_type = RL2_PIXEL_MONOCHROME;
	  nBands = 1;
	  break;
      case PNG_COLOR_TYPE_RGB:
      case PNG_COLOR_TYPE_RGB_ALPHA:
	  pixel_type = RL2_PIXEL_RGB;
	  nBands = 3;
	  break;
      };
    if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  if (png_get_tRNS (png_ptr, info_ptr, &transp, &nTransp, &transpValues)
	      == PNG_INFO_tRNS)
	    {
		/* a Transparency palette is defined */
		int i;
		for (i = 0; i < nTransp; i++)
		    *(alpha + i) = *(transp + i);
		has_alpha = 1;
	    }
      }
/* creating the raster data */
    data_size = width * height * nBands;
    data = malloc (data_size);
    if (data == NULL)
	goto error;
    p_data = data;
    if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA
	|| color_type == PNG_COLOR_TYPE_RGB_ALPHA || has_alpha)
      {
	  /* creating a transparency mask */
	  mask_sz = width * height;
	  mask = malloc (mask_sz);
	  if (mask == NULL)
	      goto error;
	  p_mask = mask;
      }
    png_read_update_info (png_ptr, info_ptr);
    rowbytes = png_get_rowbytes (png_ptr, info_ptr);
    image_data = malloc (rowbytes * height);
    if (!image_data)
	goto error;
    row_pointers = malloc (height * sizeof (png_bytep));
    if (!row_pointers)
	goto error;
    for (row = 0; row < (int) height; row++)
	row_pointers[row] = image_data + (row * rowbytes);
    png_read_image (png_ptr, row_pointers);
    png_read_end (png_ptr, NULL);
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    switch (color_type)
      {
      case PNG_COLOR_TYPE_RGB:
	  for (row = 0; row < (int) height; row++)
	    {
		png_bytep p_in = row_pointers[row];
		for (col = 0; col < (int) width; col++)
		  {
		      *p_data++ = *p_in++;
		      *p_data++ = *p_in++;
		      *p_data++ = *p_in++;
		  }
	    }
	  break;
      case PNG_COLOR_TYPE_RGB_ALPHA:
	  for (row = 0; row < (int) height; row++)
	    {
		png_bytep p_in = row_pointers[row];
		for (col = 0; col < (int) width; col++)
		  {
		      *p_data++ = *p_in++;
		      *p_data++ = *p_in++;
		      *p_data++ = *p_in++;
		      if (p_mask != NULL)
			{
			    if (*p_in++ < 128)
				*p_mask++ = 0;
			    else
				*p_mask++ = 1;
			}
		      else
			  p_in++;
		  }
	    }
	  break;
      case PNG_COLOR_TYPE_GRAY:
	  for (row = 0; row < (int) height; row++)
	    {
		png_bytep p_in = row_pointers[row];
		for (col = 0; col < (int) width; col++)
		  {
		      unsigned char val = *p_in++;
		      switch (sample_type)
			{
			case RL2_SAMPLE_1_BIT:
			case RL2_SAMPLE_2_BIT:
			case RL2_SAMPLE_4_BIT:
			case RL2_SAMPLE_UINT8:
			    break;
			default:
			    val = 0;
			};
		      *p_data++ = val;
		  }
	    }
	  break;
      case PNG_COLOR_TYPE_GRAY_ALPHA:
	  for (row = 0; row < (int) height; row++)
	    {
		png_bytep p_in = row_pointers[row];
		for (col = 0; col < (int) width; col++)
		  {
		      *p_data++ = *p_in++;
		      if (p_mask != NULL)
			{
			    if (*p_in++ < 128)
				*p_mask++ = 0;
			    else
				*p_mask++ = 1;
			}
		      else
			  p_in++;
		  }
	    }
	  break;
      default:			/* palette */
	  for (row = 0; row < (int) height; row++)
	    {
		png_bytep p_in = row_pointers[row];
		for (col = 0; col < (int) width; col++)
		  {
		      *p_data++ = *p_in;
		      if (p_mask != NULL)
			{
			    if (alpha[*p_in] < 128)
				*p_mask++ = 0;
			    else
				*p_mask++ = 1;
			}
		      p_in++;
		  }
	    }
	  break;
      };

    free (image_data);
    free (row_pointers);
/* creating the raster */
    if (nPalette > 0)
      {
	  rl_palette = rl2_create_palette (nPalette);
	  if (rl_palette == NULL)
	      goto error;
	  for (i = 0; i < nPalette; i++)
	      rl2_set_palette_color (rl_palette, i, red[i], green[i], blue[i]);
      }
    *xwidth = width;
    *xheight = height;
    *xsample_type = sample_type;
    *xpixel_type = pixel_type;
    *num_bands = nBands;
    *pixels = data;
    *pixels_sz = data_size;
    *xmask = mask;
    *xmask_sz = mask_sz;
    *xpalette = rl_palette;
    return RL2_OK;

  error:
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    free (image_data);
    if (mask != NULL)
	free (mask);
    free (row_pointers);
    return RL2_ERROR;
}
