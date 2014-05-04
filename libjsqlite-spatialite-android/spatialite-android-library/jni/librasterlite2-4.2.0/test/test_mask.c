/*

 test_mask.c -- RasterLite2 Test Case

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
test_rgb_jpeg (const char *path, const char *mask_path)
{
    rl2RasterPtr rst;
    rl2RasterPtr mask;
    rl2SectionPtr mask_img;
    unsigned short width;
    unsigned short height;
    unsigned short width2;
    unsigned short height2;
    unsigned char *rgbbuf;
    int rgbbuf_sz;
    unsigned char *mskbuf;
    int mskbuf_sz;
    unsigned char *buffer;
    int buf_size;
    unsigned char *p_data1;
    unsigned char *p_data2;
    rl2PixelPtr pxl;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int transparent;

    rl2SectionPtr img = rl2_section_from_jpeg (path);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", path);
	  return 0;
      }

    mask_img = rl2_section_from_png (mask_path);
    if (mask_img == NULL)
      {
	  fprintf (stderr, "X Unable to read: %s\n", mask_path);
	  return 0;
      }

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to get the raster: %s\n", path);
	  return 0;
      }

    if (rl2_get_raster_size (rst, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "Invalid width/height: %s\n", path);
	  return 0;
      }

    mask = rl2_get_section_raster (mask_img);
    if (mask == NULL)
      {
	  fprintf (stderr, "Unable to get the raster: %s\n", mask_path);
	  return 0;
      }

    if (rl2_get_raster_size (mask, &width2, &height2) != RL2_OK)
      {
	  fprintf (stderr, "Invalid width/height: %s\n", mask_path);
	  return 0;
      }

    if (width == width2 && height == height2)
	;
    else
      {
	  fprintf (stderr, "Mismatching width/height: %s %s\n", path,
		   mask_path);
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", path);
	  return 0;
      }

    if (rl2_raster_data_to_1bit (mask, &mskbuf, &mskbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", mask_path);
	  return 0;
      }

    rl2_destroy_section (img);
    rl2_destroy_section (mask_img);

    rst = rl2_create_raster (width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     rgbbuf, rgbbuf_sz, NULL, mskbuf, mskbuf_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to create the output raster+mask\n");
	  return 0;
      }

    img =
	rl2_create_section ("beta", RL2_COMPRESSION_JPEG,
			    RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			    rst);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create the output section+mask\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGB data: masked_rgb.jpg\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);
    if (*(p_data1 + 0) != 205 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 204)
      {
	  fprintf (stderr, "Unexpected RGB pixel #1: masked_rgb.jpg\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 196 || *(p_data2 + 1) != 194 || *(p_data2 + 2) != 197)
      {
	  fprintf (stderr, "Unexpected RGB pixel #2: masked_rgb.jpg\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: masked_rgb.jpg\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 205 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 204
	|| *(p_data1 + 3) != 0)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #1: masked_rgb.jpg\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 196 || *(p_data2 + 1) != 194 || *(p_data2 + 2) != 197
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #2: masked_rgb.jpg\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_ARGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ARGB data: masked_rgb.jpg\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);
    if (*(p_data1 + 0) != 0 || *(p_data1 + 1) != 205 || *(p_data1 + 2) != 203
	|| *(p_data1 + 3) != 204)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #1: masked_rgb.jpg\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 196 || *(p_data2 + 2) != 194
	|| *(p_data2 + 3) != 197)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #2: masked_rgb.jpg\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGR (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGR data: masked_rgb.jpg\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (120 * width * 3) + (120 * 3);
    if (*(p_data1 + 0) != 204 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 205)
      {
	  fprintf (stderr, "Unexpected BGR pixel #1: masked_rgb.jpg\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 197 || *(p_data2 + 1) != 194 || *(p_data2 + 2) != 196)
      {
	  fprintf (stderr, "Unexpected BGR pixel #2: masked_rgb.jpg\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGRA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGRA data: masked_rgb.jpg\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (120 * width * 4) + (120 * 4);

    if (*(p_data1 + 0) != 204 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 205
	|| *(p_data1 + 3) != 0)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #1: masked_rgb.jpg\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 197 || *(p_data2 + 1) != 194 || *(p_data2 + 2) != 196
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #2: masked_rgb.jpg\n");
	  return 0;
      }
    rl2_free (buffer);

    pxl = rl2_create_raster_pixel (rst);
    if (pxl == NULL)
      {
	  fprintf (stderr,
		   "Unable to create Pixel for Raster: masked_rgb.jpg\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (20,20) from Raster: masked_rgb.jpg\n");
	  return 0;
      }

    rl2_get_pixel_sample_uint8 (pxl, RL2_RED_BAND, &red);
    rl2_get_pixel_sample_uint8 (pxl, RL2_GREEN_BAND, &green);
    rl2_get_pixel_sample_uint8 (pxl, RL2_BLUE_BAND, &blue);
    rl2_is_pixel_transparent (pxl, &transparent);
    if (red != 205 || green != 203 || blue != 204 || transparent != RL2_TRUE)
      {
	  fprintf (stderr, "Unexpected pixel #1: masked_rgb.jpg\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: masked_rgb.jpg\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 120, 120) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (120,120) from Raster: masked_rgb.jpg\n");
	  return 0;
      }

    rl2_get_pixel_sample_uint8 (pxl, RL2_RED_BAND, &red);
    rl2_get_pixel_sample_uint8 (pxl, RL2_GREEN_BAND, &green);
    rl2_get_pixel_sample_uint8 (pxl, RL2_BLUE_BAND, &blue);
    rl2_is_pixel_transparent (pxl, &transparent);
    if (red != 196 || green != 194 || blue != 197 || transparent != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected pixel #2: masked_rgb.jpg\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 120, 120) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: masked_rgb.jpg\n");
	  return 0;
      }
    rl2_destroy_pixel (pxl);

    if (rl2_section_to_jpeg (img, "./masked_rgb.jpg", 80) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: masked_rgb.jpg\n");
	  return 0;
      }
    unlink ("./masked_rgb_20.jpg");

    if (rl2_section_to_lossy_webp (img, "./masked_rgb_20.webp", 20) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: masked_rgb_20.webp\n");
	  return 0;
      }
    unlink ("./masked_rgb_20.webp");

    if (rl2_section_to_lossless_webp (img, "./masked_rgb.webp") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: masked_rgb.webp\n");
	  return 0;
      }
    unlink ("./masked_rgb.webp");
    unlink ("./masked_rgb.jpg");

    rl2_destroy_section (img);
    return 1;
}

static int
test_gray_jpeg (const char *path, const char *mask_path)
{
    rl2RasterPtr rst;
    rl2RasterPtr mask;
    rl2SectionPtr mask_img;
    unsigned short width;
    unsigned short height;
    unsigned short width2;
    unsigned short height2;
    unsigned char *graybuf;
    int graybuf_sz;
    unsigned char *mskbuf;
    int mskbuf_sz;

    rl2SectionPtr img = rl2_section_from_jpeg (path);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", path);
	  return 0;
      }

    mask_img = rl2_section_from_png (mask_path);
    if (mask_img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", mask_path);
	  return 0;
      }

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to get the raster: %s\n", path);
	  return 0;
      }

    if (rl2_get_raster_size (rst, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "Invalid width/height: %s\n", path);
	  return 0;
      }

    mask = rl2_get_section_raster (mask_img);
    if (mask == NULL)
      {
	  fprintf (stderr, "Unable to get the raster: %s\n", mask_path);
	  return 0;
      }

    if (rl2_get_raster_size (mask, &width2, &height2) != RL2_OK)
      {
	  fprintf (stderr, "Invalid width/height: %s\n", mask_path);
	  return 0;
      }

    if (width == width2 && height == height2)
	;
    else
      {
	  fprintf (stderr, "Mismatching width/height: %s %s\n", path,
		   mask_path);
	  return 0;
      }

    if (rl2_raster_data_to_uint8 (rst, &graybuf, &graybuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", path);
	  return 0;
      }

    if (rl2_raster_data_to_1bit (mask, &mskbuf, &mskbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", mask_path);
	  return 0;
      }

    rl2_destroy_section (img);
    rl2_destroy_section (mask_img);

    rst =
	rl2_create_raster (width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE,
			   1, graybuf, graybuf_sz, NULL, mskbuf, mskbuf_sz,
			   NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to create the output raster+mask\n");
	  return 0;
      }

    img =
	rl2_create_section ("beta", RL2_COMPRESSION_JPEG,
			    RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			    rst);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create the output section+mask\n");
	  return 0;
      }

    if (rl2_section_to_jpeg (img, "./masked_gray.jpg", 80) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: masked_gray.jpg\n");
	  return 0;
      }
    unlink ("./masked_gray.jpg");

    if (rl2_section_to_lossy_webp (img, "./masked_gray_20.webp", 20) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: masked_gray_20.webp\n");
	  return 0;
      }
    unlink ("./masked_gray_20.webp");

    if (rl2_section_to_lossless_webp (img, "./masked_gray.webp") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: masked_gray.webp\n");
	  return 0;
      }
    unlink ("./masked_gray.webp");

    rl2_destroy_section (img);
    return 1;
}

static int
test_rgb_png (const char *path, const char *mask_path)
{
    rl2RasterPtr rst;
    rl2RasterPtr mask;
    rl2SectionPtr mask_img;
    unsigned short width;
    unsigned short height;
    unsigned short width2;
    unsigned short height2;
    unsigned char *rgbbuf;
    int rgbbuf_sz;
    unsigned char *mskbuf;
    int mskbuf_sz;

    rl2SectionPtr img = rl2_section_from_jpeg (path);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", path);
	  return 0;
      }

    mask_img = rl2_section_from_png (mask_path);
    if (mask_img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", mask_path);
	  return 0;
      }

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to get the raster: %s\n", path);
	  return 0;
      }

    if (rl2_get_raster_size (rst, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "Invalid width/height: %s\n", path);
	  return 0;
      }

    mask = rl2_get_section_raster (mask_img);
    if (mask == NULL)
      {
	  fprintf (stderr, "Unable to get the raster: %s\n", mask_path);
	  return 0;
      }

    if (rl2_get_raster_size (mask, &width2, &height2) != RL2_OK)
      {
	  fprintf (stderr, "Invalid width/height: %s\n", mask_path);
	  return 0;
      }

    if (width == width2 && height == height2)
	;
    else
      {
	  fprintf (stderr, "Mismatching width/height: %s %s\n", path,
		   mask_path);
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", path);
	  return 0;
      }

    if (rl2_raster_data_to_1bit (mask, &mskbuf, &mskbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", mask_path);
	  return 0;
      }

    rl2_destroy_section (img);
    rl2_destroy_section (mask_img);

    rst = rl2_create_raster (width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     rgbbuf, rgbbuf_sz, NULL, mskbuf, mskbuf_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to create the output raster+mask\n");
	  return 0;
      }

    img =
	rl2_create_section ("beta", RL2_COMPRESSION_PNG, RL2_TILESIZE_UNDEFINED,
			    RL2_TILESIZE_UNDEFINED, rst);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create the output section+mask\n");
	  return 0;
      }

    if (rl2_section_to_png (img, "./masked_rgb.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: masked_rgb.png\n");
	  return 0;
      }

    rl2_destroy_section (img);

    img = rl2_section_from_png ("./masked_rgb.png");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./masked_rgb.png");
	  return 0;
      }

    if (rl2_section_to_png (img, "./from_masked_rgb.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_masked_rgb.png\n");
	  return 0;
      }
    unlink ("./from_masked_rgb.png");
    unlink ("./masked_rgb.png");

    rl2_destroy_section (img);
    return 1;
}

static int
test_gray_png (const char *path, const char *mask_path)
{
    rl2RasterPtr rst;
    rl2RasterPtr mask;
    rl2SectionPtr mask_img;
    unsigned short width;
    unsigned short height;
    unsigned short width2;
    unsigned short height2;
    unsigned char *graybuf;
    int graybuf_sz;
    unsigned char *mskbuf;
    int mskbuf_sz;
    unsigned char *buffer;
    int buf_size;
    unsigned char *p_data1;
    unsigned char *p_data2;
    rl2PixelPtr pxl;
    unsigned char gray;
    int transparent;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    rl2RasterStatisticsPtr stats;
    rl2RasterStatisticsPtr cumul_stats;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char xnum_bands;
    double no_data;
    double count;
    double min;
    double max;
    double mean;
    double variance;
    double stddev;

    rl2SectionPtr img = rl2_section_from_jpeg (path);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", path);
	  return 0;
      }

    mask_img = rl2_section_from_png (mask_path);
    if (mask_img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", mask_path);
	  return 0;
      }

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to get the raster: %s\n", path);
	  return 0;
      }

    if (rl2_get_raster_size (rst, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "Invalid width/height: %s\n", path);
	  return 0;
      }

    mask = rl2_get_section_raster (mask_img);
    if (mask == NULL)
      {
	  fprintf (stderr, "Unable to get the raster: %s\n", mask_path);
	  return 0;
      }

    if (rl2_get_raster_size (mask, &width2, &height2) != RL2_OK)
      {
	  fprintf (stderr, "Invalid width/height: %s\n", mask_path);
	  return 0;
      }

    if (width == width2 && height == height2)
	;
    else
      {
	  fprintf (stderr, "Mismatching width/height: %s %s\n", path,
		   mask_path);
	  return 0;
      }

    if (rl2_raster_data_to_uint8 (rst, &graybuf, &graybuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", path);
	  return 0;
      }

    if (rl2_raster_data_to_1bit (mask, &mskbuf, &mskbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", mask_path);
	  return 0;
      }

    rl2_destroy_section (img);
    rl2_destroy_section (mask_img);

    rst =
	rl2_create_raster (width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE,
			   1, graybuf, graybuf_sz, NULL, mskbuf, mskbuf_sz,
			   NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to create the output raster+mask\n");
	  return 0;
      }

    if (rl2_raster_encode
	(rst, RL2_COMPRESSION_NONE, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 0, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - uncompressed with mask\n");
	  return 0;
      }

    if (rl2_get_raster_type (rst, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get the Raster Type\n");
	  return 0;
      }
    rl2_destroy_raster (rst);
    cumul_stats = rl2_create_raster_statistics (sample_type, 0);
    if (cumul_stats != NULL)
      {
	  fprintf (stderr, "Unexpeted cumulative statistics 0 Bands\n");
	  return 0;
      }
    cumul_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (cumul_stats == NULL)
      {
	  fprintf (stderr, "Unable to create cumulative statistics\n");
	  return 0;
      }
    stats =
	rl2_get_raster_statistics (blob_odd, blob_odd_sz, blob_even,
				   blob_even_sz, NULL, NULL);
    if (stats == NULL)
      {
	  fprintf (stderr, "Unable to get Raster Statistics #1\n");
	  return 0;
      }
    if (rl2_aggregate_raster_statistics (stats, cumul_stats) != RL2_OK)
      {
	  fprintf (stderr, "Unable to aggregate Raster Statistics #1\n");
	  return 0;
      }
    rl2_destroy_raster_statistics (stats);

    rst =
	rl2_raster_decode (RL2_SCALE_8, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - uncompressed with mask\n");
	  return 0;
      }
    rl2_destroy_raster (rst);

    rst =
	rl2_raster_decode (RL2_SCALE_4, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - uncompressed with mask\n");
	  return 0;
      }
    rl2_destroy_raster (rst);

    rst =
	rl2_raster_decode (RL2_SCALE_2, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - uncompressed with mask\n");
	  return 0;
      }
    rl2_destroy_raster (rst);

    rst =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - uncompressed with mask\n");
	  return 0;
      }
    free (blob_odd);
    free (blob_even);

    if (rl2_raster_encode
	(rst, RL2_COMPRESSION_PNG, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 0, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - PNG with mask\n");
	  return 0;
      }
    rl2_destroy_raster (rst);

    stats =
	rl2_get_raster_statistics (blob_odd, blob_odd_sz, blob_even,
				   blob_even_sz, NULL, NULL);
    if (stats == NULL)
      {
	  fprintf (stderr, "Unable to get Raster Statistics #2\n");
	  return 0;
      }
    if (rl2_aggregate_raster_statistics (stats, cumul_stats) != RL2_OK)
      {
	  fprintf (stderr, "Unable to aggregate Raster Statistics #2\n");
	  return 0;
      }
    if (rl2_get_band_statistics (NULL, 0, &min, &max, &mean, &variance, &stddev)
	!= RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected success on get Band Statistics (NULL)\n");
	  return 0;
      }
    if (rl2_get_band_statistics
	(stats, 4, &min, &max, &mean, &variance, &stddev) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected success on get Band Statistics (band#4)\n");
	  return 0;
      }
    if (rl2_get_band_statistics
	(stats, 0, &min, &max, &mean, &variance, &stddev) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Band Statistics #1\n");
	  return 0;
      }
    rl2_destroy_raster_statistics (stats);

    if (rl2_get_raster_statistics_summary
	(NULL, &no_data, &count, &sample_type, &xnum_bands) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected success on get Statistics Summary (NULL)\n");
	  return 0;
      }
    if (rl2_get_raster_statistics_summary
	(cumul_stats, &no_data, &count, &sample_type, &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get a Statistics Summary\n");
	  return 0;
      }
    if (no_data != 269318.0)
      {
	  fprintf (stderr, "Unexpected result - NO-DATA count: %1.1f\n",
		   no_data);
	  return 0;
      }
    if (count != 336670.0)
      {
	  fprintf (stderr, "Unexpected result - Valid Pixels count: %1.1f\n",
		   count);
	  return 0;
      }
    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected result - Statistics Sample Type: %02x\n",
		   sample_type);
	  return 0;
      }
    if (num_bands != xnum_bands)
      {
	  fprintf (stderr, "Unexpected result - Statistics # Bands: %02x\n",
		   xnum_bands);
	  return 0;
      }

    if (rl2_get_band_statistics
	(cumul_stats, 0, &min, &max, &mean, &variance, &stddev) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Band Statistics #2\n");
	  return 0;
      }
    if (min != 47.0)
      {
	  fprintf (stderr, "Unexpected result - Band #2 Min: %1.1f\n", min);
	  return 0;
      }
    if (max != 255.0)
      {
	  fprintf (stderr, "Unexpected result - Band #2 Max: %1.1f\n", max);
	  return 0;
      }
    if (mean >= 171.4 && mean <= 171.5)
	;
    else
      {
	  fprintf (stderr, "Unexpected result - Band #2 Mean: %1.1f\n", mean);
	  return 0;
      }
    if (variance >= 1089.98 && variance <= 1089.99)
	;
    else
      {
	  fprintf (stderr, "Unexpected result - Band #2 Variance: %1.2f\n",
		   variance);
	  return 0;
      }
    if (stddev >= 33.01 && stddev <= 33.02)
	;
    else
      {
	  fprintf (stderr, "Unexpected result - Band #2 StdDeviation: %1.2f\n",
		   stddev);
	  return 0;
      }
    rl2_destroy_raster_statistics (cumul_stats);

    rst =
	rl2_raster_decode (RL2_SCALE_8, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - PNG with mask\n");
	  return 0;
      }
    rl2_destroy_raster (rst);

    rst =
	rl2_raster_decode (RL2_SCALE_4, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - PNG with mask\n");
	  return 0;
      }
    rl2_destroy_raster (rst);

    rst =
	rl2_raster_decode (RL2_SCALE_2, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - PNG with mask\n");
	  return 0;
      }
    rl2_destroy_raster (rst);

    rst =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - PNG with mask\n");
	  return 0;
      }
    free (blob_odd);
    free (blob_even);

    img =
	rl2_create_section ("beta", RL2_COMPRESSION_PNG, RL2_TILESIZE_UNDEFINED,
			    RL2_TILESIZE_UNDEFINED, rst);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create the output section+mask\n");
	  return 0;
      }

    if (rl2_section_to_png (img, "./masked_gray.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: masked_gray.png\n");
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
	|| *(p_data1 + 3) != 0)
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
    if (*(p_data1 + 0) != 0 || *(p_data1 + 1) != 203 || *(p_data1 + 2) != 203
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
	|| *(p_data1 + 3) != 0)
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

    rl2_get_pixel_sample_uint8 (pxl, RL2_GRAYSCALE_BAND, &gray);
    rl2_is_pixel_transparent (pxl, &transparent);
    if (gray != 203 || transparent != RL2_TRUE)
      {
	  fprintf (stderr, "Unexpected pixel #1: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 120, 120) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (120,120) from Raster: from_gray_jpeg.png\n");
	  return 0;
      }

    rl2_get_pixel_sample_uint8 (pxl, RL2_GRAYSCALE_BAND, &gray);
    rl2_is_pixel_transparent (pxl, &transparent);
    if (gray != 195 || transparent != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected pixel #2: from_gray_jpeg.png\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 120, 120) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (1201,20) into Raster: from_gray_jpeg.png\n");
	  return 0;
      }

    rl2_destroy_pixel (pxl);

    rl2_destroy_section (img);

    img = rl2_section_from_png ("./masked_gray.png");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./masked_gray.png");
	  return 0;
      }

    if (rl2_section_to_png (img, "./from_masked_gray.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_masked_gray.png\n");
	  return 0;
      }
    unlink ("./from_masked_gray.png");
    unlink ("./masked_gray.png");

    rl2_destroy_section (img);

    return 1;
}

int
main (int argc, char *argv[])
{
    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (!test_rgb_jpeg ("./jpeg1.jpg", "./mask1.png"))
	return -1;

    if (!test_gray_jpeg ("./jpeg2.jpg", "./mask1.png"))
	return -2;

    if (!test_rgb_png ("./jpeg1.jpg", "./mask1.png"))
	return -3;

    if (!test_gray_png ("./jpeg2.jpg", "./mask1.png"))
	return -4;

    return 0;
}
