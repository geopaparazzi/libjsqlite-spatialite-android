/*

 test_webp.c -- RasterLite2 Test Case

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
#include <memory.h>

#include "rasterlite2/rasterlite2.h"

static int
test_no_alpha_webp (const char *path)
{
    rl2RasterPtr rst;
    unsigned short width;
    unsigned short height;
    int row;
    int col;
    unsigned char *rgbbuf;
    int rgbbuf_sz;
    unsigned char *mskbuf;
    int mskbuf_sz;
    unsigned char *p_mask;
    rl2RasterPtr raster;
    rl2PixelPtr no_data;
    unsigned char *bufpix;

    rl2SectionPtr img = rl2_section_from_webp (path);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", path);
	  return 0;
      }

    if (rl2_section_to_png (img, "./from_webp_no_alpha.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_webp_no_alpha.png\n");
	  return 0;
      }
    unlink ("./from_webp_no_alpha.png");

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

    if (rl2_raster_data_to_RGB (rst, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw buffer: %s\n", path);
	  return 0;
      }
    rl2_destroy_section (img);

    mskbuf_sz = width * height;
    mskbuf = malloc (mskbuf_sz);
    if (mskbuf == NULL)
      {
	  fprintf (stderr, "Unable to create a mask for: %s\n", path);
	  return 0;
      }
    p_mask = mskbuf;

    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (row >= 64 && row <= 96 && col >= 32 && col <= 64)
		    *p_mask++ = 0;
		else
		    *p_mask++ = 1;
	    }
      }

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

    if (rl2_section_to_lossless_webp (img, "./webp_alpha.webp") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: webp_alpha.webp\n");
	  return 0;
      }

    rl2_destroy_section (img);

    img = rl2_section_from_webp ("./webp_alpha.webp");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./webp_alpha.webp");
	  return 0;
      }

    if (rl2_section_to_png (img, "./from_webp_alpha.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_webp_alpha.png\n");
	  return 0;
      }
    unlink ("./from_webp_alpha.png");
    unlink ("./webp_alpha.webp");

    rl2_destroy_section (img);

    if (rl2_raster_data_to_RGB (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw RGB buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGBA (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw RGBA buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_ARGB (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw ARGB buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_BGR (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw BGR buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_BGRA (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw BGRA buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_int8 (NULL, (char **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw INT8 buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_uint8 (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT8 buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_int16 (NULL, (short **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw INT16 buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_uint16
	(NULL, (unsigned short **) (&rgbbuf), &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT16 buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_int32 (NULL, (int **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw INT32 buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_uint32
	(NULL, (unsigned int **) (&rgbbuf), &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT32 buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_float (NULL, (float **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw FLOAT buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_double (NULL, (double **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw DOUBLE buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_1bit (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw 1-BIT buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_2bit (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw 2-BIT buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_data_to_4bit (NULL, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw 4-BIT buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint8 (NULL, 0, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw band UINT8 buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint16
	(NULL, 0, (unsigned short **) (&rgbbuf), &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw band UINT16 buffer: (NULL raster)\n");
	  return 0;
      }

    if (rl2_raster_bands_to_RGB (NULL, 0, 1, 2, &rgbbuf, &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw BandsAsRGB buffer: (NULL raster)\n");
	  return 0;
      }

    bufpix = malloc (256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);

    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create raster (UINT8 Grid)\n");
	  return 0;
      }

    if (rl2_raster_data_to_int8 (raster, (char **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw INT8 buffer: (UINT8 Grid)\n");
	  return 0;
      }

    if (rl2_raster_data_to_int16 (raster, (short **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw INT16 buffer: (UINT8 Grid)\n");
	  return 0;
      }

    if (rl2_raster_data_to_uint16
	(raster, (unsigned short **) (&rgbbuf), &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw INT16 buffer: (UINT8 Grid)\n");
	  return 0;
      }

    if (rl2_raster_data_to_int32 (raster, (int **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw INT32 buffer: (UINT8 Grid)\n");
	  return 0;
      }

    if (rl2_raster_data_to_uint32
	(raster, (unsigned int **) (&rgbbuf), &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw INT32 buffer: (UINT8 Grid)\n");
	  return 0;
      }

    if (rl2_raster_data_to_float (raster, (float **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw FLOAT buffer: (UINT8 Grid)\n");
	  return 0;
      }

    if (rl2_raster_data_to_double (raster, (double **) (&rgbbuf), &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw DOUBLE buffer: (UINT8 Grid)\n");
	  return 0;
      }

    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);

    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create raster (INT8 Grid)\n");
	  return 0;
      }

    if (rl2_raster_data_to_uint8 (raster, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT8 buffer: (INT8 Grid)\n");
	  return 0;
      }

    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256 * 3);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 3,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0, NULL);

    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create raster (MultiBand)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint16
	(raster, 0, (unsigned short **) (&rgbbuf), &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT16 buffer: (MultiBand)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint8 (raster, -1, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT8 buffer: (invalid Band #1)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint8 (raster, 3, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT8 buffer: (invalid Band #2)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint8 (raster, -1, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT8 buffer: (invalid Band #1)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint8 (raster, 3, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT8 buffer: (invalid Band #2)\n");
	  return 0;
      }

    if (rl2_raster_bands_to_RGB (raster, 0, -1, 2, &rgbbuf, &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw BandsAsRGB buffer: (invalid GREEN Band #1)\n");
	  return 0;
      }

    if (rl2_raster_bands_to_RGB (raster, 0, 3, 2, &rgbbuf, &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw BandsAsRGB buffer: (invalid GREEN Band #2)\n");
	  return 0;
      }

    if (rl2_raster_bands_to_RGB (raster, 0, 1, -1, &rgbbuf, &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw BandsAsRGB buffer: (invalid BLUE Band #1)\n");
	  return 0;
      }

    if (rl2_raster_bands_to_RGB (raster, 0, 1, 3, &rgbbuf, &rgbbuf_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw BandsAsRGB buffer: (invalid BLUE Band #2)\n");
	  return 0;
      }

    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256 * 6);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, 3,
			   bufpix, 256 * 256 * 6, NULL, NULL, 0, NULL);

    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create raster (MultiBand16)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint8 (raster, 0, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected resut: get raw UINT8 buffer: (MultiBand16)\n");
	  return 0;
      }

    rl2_destroy_raster (raster);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel (RGB)\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Red NoData (RGB)\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Green NoData (RGB)\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Blue NoData (RGB)\n");
	  return 0;
      }

    bufpix = malloc (256 * 256 * 3);
    memset (bufpix, 0, 256 * 256 * 3);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0, no_data);

    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create raster (RGB)\n");
	  return 0;
      }

    if (rl2_raster_band_to_uint16
	(raster, 0, (unsigned short **) (&rgbbuf), &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected resut: get raw UINT16 buffer: (RGB)\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGBA (raster, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw RGBA buffer (RGB)\n");
	  return 0;
      }
    free (rgbbuf);

    if (rl2_raster_data_to_ARGB (raster, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw ARGB buffer (RGB)\n");
	  return 0;
      }
    free (rgbbuf);

    if (rl2_raster_data_to_BGRA (raster, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw BGRA buffer (RGB)\n");
	  return 0;
      }
    free (rgbbuf);

    if (rl2_raster_data_to_1bit (raster, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected resut: get raw 1-BIT buffer: (RGB)\n");
	  return 0;
      }

    if (rl2_raster_data_to_2bit (raster, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected resut: get raw 2-BIT buffer: (RGB)\n");
	  return 0;
      }

    if (rl2_raster_data_to_4bit (raster, &rgbbuf, &rgbbuf_sz) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected resut: get raw 4-BIT buffer: (RGB)\n");
	  return 0;
      }

    rl2_destroy_raster (raster);

    no_data = rl2_create_pixel (RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel (MONOCHROME)\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_1bit (no_data, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set NoData (MONOCHROME)\n");
	  return 0;
      }

    bufpix = malloc (256 * 256);
    memset (bufpix, 0, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, no_data);

    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create raster (MONOCHROME)\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGBA (raster, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw RGBA buffer (MONOCHROME)\n");
	  return 0;
      }
    free (rgbbuf);

    if (rl2_raster_data_to_ARGB (raster, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw ARGB buffer (MONOCHROME)\n");
	  return 0;
      }
    free (rgbbuf);

    if (rl2_raster_data_to_BGRA (raster, &rgbbuf, &rgbbuf_sz) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raw BGRA buffer (MONOCHROME)\n");
	  return 0;
      }
    free (rgbbuf);

    rl2_destroy_raster (raster);

    return 1;
}

int
main (int argc, char *argv[])
{
    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (!test_no_alpha_webp ("./webp_no_alpha.webp"))
	return -1;

    return 0;
}
