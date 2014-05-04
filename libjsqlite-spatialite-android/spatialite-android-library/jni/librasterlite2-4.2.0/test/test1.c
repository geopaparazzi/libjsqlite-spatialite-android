/*

 test1.c -- RasterLite2 Test Case

 Author: Alessandro Furieri <a.furieri@lqt.it>

 ------------------------------------------------------------------------------
 
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
 
Portions created by the Initial Developer are Copyright (C) 2013
the Initial Developer. All Rights Reserved.

Contributor(s):

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
#include <unistd.h>
#include <stdio.h>

#include "rasterlite2/rasterlite2.h"

static int
test_rgb_jpeg (const char *path)
{
    unsigned char *buffer;
    int buf_size;
    unsigned short width;
    unsigned short height;
    unsigned char *p_data1;
    unsigned char *p_data2;
    rl2PixelPtr pxl;
    char sample_8;
    unsigned char sample_u8;
    short sample_16;
    unsigned short sample_u16;
    int sample_32;
    unsigned int sample_u32;
    float sample_flt;
    double sample_dbl;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int is_transparent;
    unsigned char compression;
    int is_compressed;
    unsigned short tile_width;
    unsigned short tile_height;
    int srid;
    double minX;
    double minY;
    double maxX;
    double maxY;
    rl2RasterPtr rst;
    rl2SectionPtr img = rl2_section_from_jpeg (path);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", path);
	  return 0;
      }

    if (rl2_get_section_name (img) == NULL)
      {
	  fprintf (stderr, "\"%s\" missing section name\n", path);
	  return 0;
      }

    if (rl2_get_section_compression (img, &compression) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" Unable to get compression mode\n", path);
	  return 0;
      }

    if (compression != RL2_COMPRESSION_JPEG)
      {
	  fprintf (stderr, "\"%s\" invalid compression mode\n", path);
	  return 0;
      }

    if (rl2_is_section_uncompressed (img, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get uncompressed\n", path);
	  return 0;
      }

    if (is_compressed != RL2_FALSE)
      {
	  fprintf (stderr, "\"%s\" invalid compression lossless\n", path);
	  return 0;
      }

    if (rl2_is_section_compression_lossy (img, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get compression lossy\n", path);
	  return 0;
      }

    if (is_compressed != RL2_TRUE)
      {
	  fprintf (stderr, "\"%s\" invalid compression lossy\n", path);
	  return 0;
      }

    if (rl2_get_section_tile_size (img, &tile_width, &tile_height) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get tile size\n", path);
	  return 0;
      }
    if (tile_width != RL2_TILESIZE_UNDEFINED)
      {
	  fprintf (stderr, "\"%s\" invalid tile section tile width\n", path);
	  return 0;
      }
    if (tile_height != RL2_TILESIZE_UNDEFINED)
      {
	  fprintf (stderr, "\"%s\" invalid tile section tile height\n", path);
	  return 0;
      }

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "\"%s\" invalid raster pointer\n", path);
	  return 0;
      }

    if (rl2_get_raster_size (rst, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get image size\n", path);
	  return 0;
      }

    if (width != 558)
      {
	  fprintf (stderr, "\"%s\" invalid image width\n", path);
	  return 0;
      }

    if (height != 543)
      {
	  fprintf (stderr, "\"%s\" invalid image height\n", path);
	  return 0;
      }

    if (rl2_get_raster_srid (rst, &srid) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get image SRID\n", path);
	  return 0;
      }

    if (srid != RL2_GEOREFERENCING_NONE)
      {
	  fprintf (stderr, "\"%s\" mismatiching image SRID\n", path);
	  return 0;
      }

    if (rl2_get_raster_extent (rst, &minX, &minY, &maxX, &maxY) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get image extent\n", path);
	  return 0;
      }

    if (minX != 0.0)
      {
	  fprintf (stderr, "\"%s\" invalid image MinX\n", path);
	  return 0;
      }

    if (minY != 0.0)
      {
	  fprintf (stderr, "\"%s\" invalid image MinX\n", path);
	  return 0;
      }

    if (maxX != 558.0)
      {
	  fprintf (stderr, "\"%s\" invalid image MaxX\n", path);
	  return 0;
      }

    if (maxY != 543.0)
      {
	  fprintf (stderr, "\"%s\" invalid image MaxX\n", path);
	  return 0;
      }

    if (rl2_section_to_png (img, "./from_rgb_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGB data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);

    if (*(p_data1 + 0) != 205 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 204)
      {
	  fprintf (stderr, "Unexpected RGB pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 196 || *(p_data2 + 1) != 194 || *(p_data2 + 2) != 197)
      {
	  fprintf (stderr, "Unexpected RGB pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }

    rl2_free (buffer);

    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);

    if (*(p_data1 + 0) != 205 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 204
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 196 || *(p_data2 + 1) != 194 || *(p_data2 + 2) != 197
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }

    rl2_free (buffer);

    if (rl2_raster_data_to_ARGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ARGB data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 205 || *(p_data1 + 2) != 203
	|| *(p_data1 + 3) != 204)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 196 || *(p_data2 + 2) != 194
	|| *(p_data2 + 3) != 197)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGR (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGR data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);
    if (*(p_data1 + 0) != 204 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 205)
      {
	  fprintf (stderr, "Unexpected BGR pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 197 || *(p_data2 + 1) != 194 || *(p_data2 + 2) != 196)
      {
	  fprintf (stderr, "Unexpected BGR pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGRA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGRA data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 204 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 205
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 197 || *(p_data2 + 1) != 194 || *(p_data2 + 2) != 196
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_band_to_uint8 (rst, RL2_RED_BAND, &buffer, &buf_size) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get RED band data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width) + 20;
    p_data2 = buffer + (120 * width) + 120;
    if (*p_data1 != 205)
      {
	  fprintf (stderr, "Unexpected RED pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*p_data2 != 196)
      {
	  fprintf (stderr, "Unexpected RED pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_band_to_uint8 (rst, RL2_GREEN_BAND, &buffer, &buf_size) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get GREEN band data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width) + 20;
    p_data2 = buffer + (120 * width) + 120;
    if (*p_data1 != 203)
      {
	  fprintf (stderr, "Unexpected GREEN pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*p_data2 != 194)
      {
	  fprintf (stderr, "Unexpected GREEN pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_band_to_uint8 (rst, RL2_BLUE_BAND, &buffer, &buf_size) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get BLUE band data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width) + 20;
    p_data2 = buffer + (120 * width) + 120;
    if (*p_data1 != 204)
      {
	  fprintf (stderr, "Unexpected BLUE pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*p_data2 != 197)
      {
	  fprintf (stderr, "Unexpected BLUE pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_bands_to_RGB
	(rst, RL2_GREEN_BAND, RL2_RED_BAND, RL2_BLUE_BAND, &buffer,
	 &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGB band data: from_rgb_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);
    if (*(p_data1 + 0) != 203 || *(p_data1 + 1) != 205 || *(p_data1 + 2) != 204)
      {
	  fprintf (stderr, "Unexpected RGB band pixel #1: from_rgb_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 194 || *(p_data2 + 1) != 196 || *(p_data2 + 2) != 197)
      {
	  fprintf (stderr, "Unexpected RGB band pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    pxl = rl2_create_raster_pixel (rst);
    if (pxl == NULL)
      {
	  fprintf (stderr,
		   "Unable to create Pixel for Raster: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (20,20) from Raster: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_type (pxl, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to create get Pixel Type: from_palette_jpeg.png\n");
	  return 0;
      }
    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected Pixel SampleType: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (pixel_type != RL2_PIXEL_RGB)
      {
	  fprintf (stderr, "Unexpected Pixel Type: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (num_bands != 3)
      {
	  fprintf (stderr, "Unexpected Pixel # Bands: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_int8 (pxl, &sample_8) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample INT8: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_int8 (pxl, 1) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample INT8: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_uint8 (pxl, RL2_RED_BAND, &sample_u8) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample RED: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (sample_u8 != 205)
      {
	  fprintf (stderr,
		   "Unexpected Pixel RED Sample value %d: from_rgb_jpeg.png\n",
		   sample_u8);
	  return 0;
      }

    if (rl2_get_pixel_sample_uint8 (pxl, RL2_GREEN_BAND, &sample_u8) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample GREEN: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (sample_u8 != 203)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GREEN Sample value %d: from_rgb_jpeg.png\n",
		   sample_u8);
	  return 0;
      }

    if (rl2_get_pixel_sample_uint8 (pxl, RL2_BLUE_BAND, &sample_u8) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample BLUE: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (sample_u8 != 204)
      {
	  fprintf (stderr,
		   "Unexpected Pixel BLUE Sample value %d: from_rgb_jpeg.png\n",
		   sample_u8);
	  return 0;
      }

    if (rl2_set_pixel_sample_uint8 (pxl, 0, sample_u8) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample UINT8: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_int16 (pxl, &sample_16) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample INT16: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_int16 (pxl, 1) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample INT16: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_uint16 (pxl, 0, &sample_u16) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample UINT16: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_uint16 (pxl, 0, 1) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample UINT16: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_int32 (pxl, &sample_32) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample INT32: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_int32 (pxl, 1) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample INT32: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_uint32 (pxl, &sample_u32) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample UINT32: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_uint32 (pxl, 1) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample UINT32: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_float (pxl, &sample_flt) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample FLOAT: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_float (pxl, 1.5) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample FLOAT: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_double (pxl, &sample_dbl) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample DOUBLE: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_double (pxl, 1.5) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample DOUBLE: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_is_pixel_transparent (pxl, &is_transparent) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel IsTransparent: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (is_transparent != RL2_FALSE)
      {
	  fprintf (stderr,
		   "Unexpected Pixel IsTransparent: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_is_pixel_opaque (pxl, &is_transparent) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Pixel IsOpaque: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (is_transparent != RL2_TRUE)
      {
	  fprintf (stderr, "Unexpected Pixel IsOpaque: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_opaque (pxl) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel SetOpaque: from_rgb_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_pixel_transparent (pxl) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetTransparent: from_rgb_jpeg.png\n");
	  return 0;
      }

    rl2_destroy_pixel (pxl);
    unlink ("./from_rgb_jpeg.png");

    if (rl2_section_to_lossy_webp (img, "./from_rgb_jpeg_10.webp", 10) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_rgb_jpeg_10.webp\n");
	  return 0;
      }
    unlink ("./from_rgb_jpeg_10.webp");

    if (rl2_section_to_lossless_webp (img, "./from_rgb_jpeg.webp") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_rgb_jpeg.webp\n");
	  return 0;
      }
    unlink ("./from_rgb_jpeg.webp");

    rl2_destroy_section (img);
    return 1;
}

static int
test_gray_jpeg (const char *path)
{
    unsigned char *buffer;
    int buf_size;
    int width = 558;
    unsigned char *p_data1;
    unsigned char *p_data2;
    unsigned char sample;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    rl2PixelPtr pxl;
    rl2RasterPtr rst;
    rl2SectionPtr img = rl2_section_from_jpeg (path);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", path);
	  return 0;
      }

    if (rl2_section_to_png (img, "./from_gray_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_gray_jpeg.png\n");
	  return 0;
      }

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "\"%s\" invalid raster pointer\n", path);
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGB data: from_gray_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);
    if (*(p_data1 + 0) != 203 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 203)
      {
	  fprintf (stderr, "Unexpected RGB pixel #1: from_gray_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 195 || *(p_data2 + 1) != 195 || *(p_data2 + 2) != 195)
      {
	  fprintf (stderr, "Unexpected RGB pixel #2: from_gray_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: from_gray_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 203 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 203
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #1: from_gray_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 195 || *(p_data2 + 1) != 195 || *(p_data2 + 2) != 195
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_ARGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ARGB data: from_gray_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 203
	|| *(p_data1 + 3) != 203)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #1: from_gray_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 195 || *(p_data2 + 2) != 195
	|| *(p_data2 + 3) != 195)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #2: from_gray_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGR (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGR data: from_gray_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);
    if (*(p_data1 + 0) != 203 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 203)
      {
	  fprintf (stderr, "Unexpected BGR pixel #1: from_gray_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 195 || *(p_data2 + 1) != 195 || *(p_data2 + 2) != 195)
      {
	  fprintf (stderr, "Unexpected BGR pixel #2: from_gray_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGRA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGRA data: from_gray_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 203 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 203
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #1: from_gray_jpeg.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 195 || *(p_data2 + 1) != 195 || *(p_data2 + 2) != 195
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #2: from_gray_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_uint8 (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get UINT8 data: from_gray_jpeg.png\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width) + 20;
    p_data2 = buffer + (120 * width) + 120;
    if (*p_data1 != 203)
      {
	  fprintf (stderr, "Unexpected UINT8 pixel #1: from_gray_jpeg.png\n");
	  return 0;
      }
    if (*p_data2 != 195)
      {
	  fprintf (stderr, "Unexpected UINT8 pixel #2: from_gray_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_band_to_uint8 (rst, RL2_RED_BAND, &buffer, &buf_size) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "Error RED band data: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint8 (rst, RL2_GREEN_BAND, &buffer, &buf_size) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Error to get GREEN band data: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint8 (rst, RL2_BLUE_BAND, &buffer, &buf_size) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "Error BLUE band data: from_gray_jpeg.png\n");
	  return 0;
      }

    pxl = rl2_create_raster_pixel (rst);
    if (pxl == NULL)
      {
	  fprintf (stderr,
		   "Unable to create Pixel for Raster: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (20,20) from Raster: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_type (pxl, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to create get Pixel Type: from_gray_jpeg.png\n");
	  return 0;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected Pixel SampleType: from_gray_jpeg.png\n");
	  return 0;
      }

    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected Pixel Type: from_gray_jpeg.png\n");
	  return 0;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected Pixel # Bands: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_uint8 (pxl, RL2_GRAYSCALE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample GRAY: from_gray_jpeg.png\n");
	  return 0;
      }

    if (sample != 203)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GRAY Sample value %d: from_gray_jpeg.png\n",
		   sample);
	  return 0;
      }

    if (rl2_set_pixel_sample_uint8 (pxl, 0, sample) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample UINT8: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: from_gray_jpeg.png\n");
	  return 0;
      }

    rl2_destroy_pixel (pxl);
    unlink ("./from_gray_jpeg.png");

    if (rl2_section_to_lossy_webp (img, "./from_gray_jpeg_10.webp", 10) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_gray_jpeg_10.webp\n");
	  return 0;
      }
    unlink ("./from_gray_jpeg_10.webp");

    if (rl2_section_to_lossless_webp (img, "./from_gray_jpeg.webp") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_gray_jpeg.webp\n");
	  return 0;
      }
    unlink ("./from_gray_jpeg.webp");

    rl2_destroy_section (img);
    return 1;
}

static int
test_palette_png (const char *path)
{
    unsigned char *buffer;
    int buf_size;
    unsigned short width;
    unsigned short height;
    unsigned char *p_data1;
    unsigned char *p_data2;
    unsigned char sample;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned short num_entries;
    unsigned char compression;
    int is_compressed;
    unsigned short tile_width;
    unsigned short tile_height;
    int srid;
    double minX;
    double minY;
    double maxX;
    double maxY;
    gaiaGeomCollPtr geom;
    rl2PixelPtr pxl;
    rl2RasterPtr rst;
    rl2SectionPtr img = rl2_section_from_png (path);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", path);
	  return 0;
      }

    if (rl2_get_section_name (img) == NULL)
      {
	  fprintf (stderr, "\"%s\" missing section name\n", path);
	  return 0;
      }

    if (rl2_get_section_compression (img, &compression) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get compression mode\n", path);
	  return 0;
      }

    if (compression != RL2_COMPRESSION_PNG)
      {
	  fprintf (stderr, "\"%s\" invalid compression mode\n", path);
	  return 0;
      }

    if (rl2_is_section_uncompressed (img, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get invalid uncompressed\n", path);
	  return 0;
      }

    if (is_compressed != RL2_FALSE)
      {
	  fprintf (stderr, "\"%s\" invalid uncompressed\n", path);
	  return 0;
      }

    if (rl2_is_section_compression_lossless (img, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get compression lossless\n", path);
	  return 0;
      }

    if (is_compressed != RL2_TRUE)
      {
	  fprintf (stderr, "\"%s\" invalid compression lossless\n", path);
	  return 0;
      }

    if (rl2_is_section_compression_lossy (img, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get compression lossy\n", path);
	  return 0;
      }

    if (is_compressed != RL2_FALSE)
      {
	  fprintf (stderr, "\"%s\" invalid compression lossy\n", path);
	  return 0;
      }

    if (rl2_get_section_tile_size (img, &tile_width, &tile_height) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get tile size\n", path);
	  return 0;
      }
    if (tile_width != RL2_TILESIZE_UNDEFINED)
      {
	  fprintf (stderr, "\"%s\" invalid tile section tile width\n", path);
	  return 0;
      }
    if (tile_height != RL2_TILESIZE_UNDEFINED)
      {
	  fprintf (stderr, "\"%s\" invalid tile section tile height\n", path);
	  return 0;
      }

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "\"%s\" invalid raster pointer\n", path);
	  return 0;
      }

    if (rl2_get_raster_size (rst, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get image size\n", path);
	  return 0;
      }

    if (width != 563)
      {
	  fprintf (stderr, "\"%s\" invalid image width\n", path);
	  return 0;
      }

    if (height != 408)
      {
	  fprintf (stderr, "\"%s\" invalid image height\n", path);
	  return 0;
      }

    if (rl2_get_raster_srid (rst, &srid) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get image SRID\n", path);
	  return 0;
      }

    if (srid != RL2_GEOREFERENCING_NONE)
      {
	  fprintf (stderr, "\"%s\" mismatching image SRID\n", path);
	  return 0;
      }

    if (rl2_get_raster_extent (rst, &minX, &minY, &maxX, &maxY) != RL2_OK)
      {
	  fprintf (stderr, "\"%s\" unable to get image extent\n", path);
	  return 0;
      }

    if (minX != 0.0)
      {
	  fprintf (stderr, "\"%s\" invalid image MinX\n", path);
	  return 0;
      }

    if (minY != 0.0)
      {
	  fprintf (stderr, "\"%s\" invalid image MinX\n", path);
	  return 0;
      }

    if (maxX != 563.0)
      {
	  fprintf (stderr, "\"%s\" invalid image MaxX\n", path);
	  return 0;
      }

    if (maxY != 408.0)
      {
	  fprintf (stderr, "\"%s\" invalid image MaxX\n", path);
	  return 0;
      }

    geom = rl2_get_raster_bbox (rst);
    if (geom != NULL)
      {
	  fprintf (stderr, "\"%s\" unexpected image BBOX\n", path);
	  return 0;
      }

    if (rl2_get_raster_palette (rst) == NULL)
      {
	  fprintf (stderr, "\"%s\" invalid palette\n", path);
	  return 0;
      }

    if (rl2_get_palette_entries (rl2_get_raster_palette (rst), &num_entries) !=
	RL2_OK)
      {
	  fprintf (stderr, "\"%s\" Unable to get palette # entries\n", path);
	  return 0;
      }

    if (num_entries != 30)
      {
	  fprintf (stderr, "\"%s\" invalid palette # entries\n", path);
	  return 0;
      }

    if (rl2_section_to_jpeg (img, "./from_palette_png.jpg", 20) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_palette_png.jpg\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGB data: from_palette_png.png\n");
	  return 0;
      }
    p_data1 = buffer + (19 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);
    if (*(p_data1 + 0) != 237 || *(p_data1 + 1) != 28 || *(p_data1 + 2) != 36)
      {;
	  fprintf (stderr, "Unexpected RGB pixel #1: from_palette_png.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 250 || *(p_data2 + 1) != 250 || *(p_data2 + 2) != 250)
      {
	  fprintf (stderr, "Unexpected RGB pixel #2: from_palette_png.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: from_palette_png.png\n");
	  return 0;
      }
    p_data1 = buffer + (19 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 237 || *(p_data1 + 1) != 28 || *(p_data1 + 2) != 36
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #1: from_palette_png.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 250 || *(p_data2 + 1) != 250 || *(p_data2 + 2) != 250
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #2: from_rgb_jpeg.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_ARGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ARGB data: from_palette_png.png\n");
	  return 0;
      }
    p_data1 = buffer + (19 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 237 || *(p_data1 + 2) != 28
	|| *(p_data1 + 3) != 36)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #1: from_palette_png.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 250 || *(p_data2 + 2) != 250
	|| *(p_data2 + 3) != 250)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #2: from_palette_png.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGR (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGR data: from_palette_png.png\n");
	  return 0;
      }
    p_data1 = buffer + (19 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);
    if (*(p_data1 + 0) != 36 || *(p_data1 + 1) != 28 || *(p_data1 + 2) != 237)
      {
	  fprintf (stderr, "Unexpected BGR pixel #1: from_palette_png.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 250 || *(p_data2 + 1) != 250 || *(p_data2 + 2) != 250)
      {
	  fprintf (stderr, "Unexpected BGR pixel #2: from_palette_png.png\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGRA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGRA data: from_palette_png.png\n");
	  return 0;
      }
    p_data1 = buffer + (19 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 36 || *(p_data1 + 1) != 28 || *(p_data1 + 2) != 237
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #1: from_palette_png.png\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 250 || *(p_data2 + 1) != 250 || *(p_data2 + 2) != 250
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #2: from_palette_png.png\n");
	  return 0;
      }
    rl2_free (buffer);

    pxl = rl2_create_raster_pixel (rst);
    if (pxl == NULL)
      {
	  fprintf (stderr,
		   "Unable to create Pixel for Raster: from_palette_png.png\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (20,20) from Raster: from_palette_png.png\n");
	  return 0;
      }

    if (rl2_get_pixel_type (pxl, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to create get Pixel Type: from_palette_png.png\n");
	  return 0;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SampleType: from_palette_png.png\n");
	  return 0;
      }

    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected Pixel Type: from_palette_png.png\n");
	  return 0;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected Pixel # Bands: from_palette_png.png\n");
	  return 0;
      }


    rl2_get_pixel_type (pxl, &sample_type, &pixel_type, &num_bands);
    if (rl2_get_pixel_sample_uint8 (pxl, RL2_PALETTE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel GetSample PALETTE: from_palette_png.png\n");
	  return 0;
      }

    if (sample != 29)
      {
	  fprintf (stderr,
		   "Unexpected Pixel PALETTE Sample value %d: from_palette_png.png\n",
		   sample);
	  return 0;
      }

    if (rl2_set_pixel_sample_uint8 (pxl, 0, sample) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample UINT8: from_palette_png.png\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: from_palette_png.png\n");
	  return 0;
      }

    rl2_destroy_pixel (pxl);
    unlink ("./from_palette_png.jpg");

    if (rl2_section_to_gif (img, "./from_palette_png.gif") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_palette_png.gif\n");
	  return 0;
      }
    unlink ("./from_palette_png.gif");

    if (rl2_section_to_lossy_webp (img, "./from_palette_png_10.webp", 10) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_palette_png_10.webp\n");
	  return 0;
      }
    unlink ("./from_palette_png_10.webp");

    if (rl2_section_to_lossless_webp (img, "./from_palette_png.webp") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_palette_png.webp\n");
	  return 0;
      }
    unlink ("./from_palette_png.webp");

    rl2_destroy_section (img);
    return 1;
}

int
main (int argc, char *argv[])
{
    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (!test_rgb_jpeg ("./jpeg1.jpg"))
	return -1;

    if (!test_gray_jpeg ("./jpeg2.jpg"))
	return -2;

    if (!test_palette_png ("./png1.png"))
	return -3;

    return 0;
}
