/*

 test6.c -- RasterLite2 Test Case

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

The Original Code is the RasterLite2 library

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
naturalEndian ()
{
/* ensures to always encode in the natural endian order */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 0;
    return 1;
}

static rl2PalettePtr
create_palette ()
{
/* creating a "4colors" image */
    rl2PalettePtr palette = rl2_create_palette (4);
    if (palette == NULL)
	goto error;
    if (rl2_set_palette_color (palette, 0, 255, 255, 204) != RL2_OK)
	goto error;
    if (rl2_set_palette_color (palette, 1, 204, 255, 255) != RL2_OK)
	goto error;
    if (rl2_set_palette_color (palette, 2, 255, 204, 255) != RL2_OK)
	goto error;
    if (rl2_set_palette_color (palette, 3, 204, 204, 204) != RL2_OK)
	goto error;
    return palette;
  error:
    return NULL;
}

static rl2SectionPtr
create_4colors ()
{
/* creating a synthetic "4colors" image */
    rl2SectionPtr scn;
    rl2RasterPtr rst;
    rl2PalettePtr palette = create_palette ();
    int row;
    int col;
    int bufsize = 1024 * 1024;
    int idx = 0;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;
    if (palette == NULL)
	goto error;

    for (row = 0; row < 256; row += 4)
      {
	  int i;
	  for (i = 0; i < 4; i++)
	    {
		int ix = idx;
		for (col = 0; col < 1024; col += 4)
		  {
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      ix++;
		      if (ix > 3)
			  ix = 0;
		  }
	    }
	  idx++;
	  if (idx > 3)
	      idx = 0;
      }

    for (row = 256; row < 512; row += 8)
      {
	  int i;
	  for (i = 0; i < 8; i++)
	    {
		int ix = idx;
		for (col = 0; col < 1024; col += 8)
		  {
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      *p++ = ix;
		      ix++;
		      if (ix > 3)
			  ix = 0;
		  }
	    }
	  idx++;
	  if (idx > 3)
	      idx = 0;
      }

    for (row = 512; row < 768; row++)
      {
	  int ix = idx;
	  for (col = 0; col < 1024; col += 8)
	    {
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		*p++ = ix;
		ix++;
		if (ix > 3)
		    ix = 0;
	    }
      }

    for (row = 768; row < 1024; row += 8)
      {
	  int i;
	  for (i = 0; i < 8; i++)
	    {
		for (col = 0; col < 1024; col++)
		    *p++ = idx;
	    }
	  idx++;
	  if (idx > 3)
	      idx = 0;
      }

    rst =
	rl2_create_raster (1024, 1024, RL2_SAMPLE_2_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, bufsize, palette, NULL, 0, NULL);
    if (rst == NULL)
	goto error;

    scn = rl2_create_section ("4colors", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      rst);
    if (scn == NULL)
	rl2_destroy_raster (rst);
    return scn;
  error:
    free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return NULL;
}

static int
test_4colors (rl2SectionPtr img)
{
/* testing 4-COLORS buffer functions */
    unsigned char *buffer;
    int buf_size;
    int width = 1024;
    unsigned char *p_data1;
    unsigned char *p_data2;
    unsigned char sample;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    rl2PixelPtr pxl;
    rl2RasterPtr rst;

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "4-COLORS invalid raster pointer\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGB data: 4-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (720 * width * 3) + (510 * 3);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 204 || *(p_data1 + 2) != 255)
      {
	  fprintf (stderr, "Unexpected RGB pixel #1: 4-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 204 || *(p_data2 + 1) != 204 || *(p_data2 + 2) != 204)
      {
	  fprintf (stderr, "Unexpected RGB pixel #2: 4-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: 4-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (720 * width * 4) + (510 * 4);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 204 || *(p_data1 + 2) != 255
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #1: 4-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 204 || *(p_data2 + 1) != 204 || *(p_data2 + 2) != 204
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #2: 4-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_ARGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get ARGB data: 4-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (720 * width * 4) + (510 * 4);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 255 || *(p_data1 + 2) != 204
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #1: 4-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 204 || *(p_data2 + 2) != 204
	|| *(p_data2 + 3) != 204)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #2: 4-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGR (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGR data: 4-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (720 * width * 3) + (510 * 3);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 204 || *(p_data1 + 2) != 255)
      {
	  fprintf (stderr, "Unexpected BGR pixel #1: 4-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 204 || *(p_data2 + 1) != 204 || *(p_data2 + 2) != 204)
      {
	  fprintf (stderr, "Unexpected BGR pixel #2: 4-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGRA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGRA data: 4-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (720 * width * 4) + (510 * 4);
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 204 || *(p_data1 + 2) != 255
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #1: 4-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 204 || *(p_data2 + 1) != 204 || *(p_data2 + 2) != 204
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #2: 4-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_2bit (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get 2-BIT data: 4-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width) + 20;
    p_data2 = buffer + (720 * width) + 510;
    if (*p_data1 != 2)
      {
	  fprintf (stderr, "Unexpected 2-BIT pixel #1: 4-COLORS\n");
	  return 0;
      }
    if (*p_data2 != 3)
      {
	  fprintf (stderr, "Unexpected 2-BIT pixel #2: 4-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    pxl = rl2_create_raster_pixel (rst);
    if (pxl == NULL)
      {
	  fprintf (stderr, "Unable to create Pixel for Raster: 4-COLORS\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (20,20) from Raster: 4-COLORS\n");
	  return 0;
      }

    if (rl2_get_pixel_type (pxl, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to create get Pixel Type: 4-COLORS\n");
	  return 0;
      }

    if (sample_type != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr, "Unexpected Pixel SampleType: 4-COLORS\n");
	  return 0;
      }

    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected Pixel Type: 4-COLORS\n");
	  return 0;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected Pixel # Bands: 4-COLORS\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_2bit (pxl, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel 2-BIT: 4-COLORS\n");
	  return 0;
      }

    if (sample != 2)
      {
	  fprintf (stderr,
		   "Unexpected Pixel 2-BIT Sample value %d: 4-COLORS\n",
		   sample);
	  return 0;
      }

    if (rl2_set_pixel_sample_2bit (pxl, sample) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel SetSample 2-BIT: 4-COLORS\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_2bit (pxl, 4) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected Pixel SetSample 2-BIT (exceeding value): 4-COLORS\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: 4-COLORS\n");
	  return 0;
      }

    rl2_destroy_pixel (pxl);

    return 1;
}

int
main (int argc, char *argv[])
{
    rl2SectionPtr img;
    rl2RasterPtr raster;
    rl2RasterStatisticsPtr stats;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    unsigned char *blob_odd_png;
    int blob_odd_sz_png;
    unsigned char *blob_even_png;
    int blob_even_sz_png;
    unsigned char *blob_odd_gif;
    int blob_odd_sz_gif;
    unsigned char *blob_even_gif;
    int blob_even_sz_gif;
    int endian = naturalEndian ();
    rl2PalettePtr palette;
    rl2PalettePtr plt2;
    unsigned char *blob_stat;
    int blob_stat_size;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    img = create_4colors ();
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create image: 4colors\n");
	  return -1;
      }

    if (rl2_section_to_jpeg (img, "./4colors.jpg", 70) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 4colors.jpg\n");
	  return -2;
      }

    if (rl2_section_to_png (img, "./4colors.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 4colors.png\n");
	  return -3;
      }

    if (!test_4colors (img))
	return -4;

    rl2_destroy_section (img);

    unlink ("./4colors.jpg");

    img = rl2_section_from_png ("./4colors.png");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./4colors.png");
	  return -5;
      }

    unlink ("./4colors.png");

    if (rl2_section_to_png (img, "./from_4colors.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_4colors.png\n");
	  return -6;
      }

    unlink ("./from_4colors.png");

    raster = rl2_get_section_raster (img);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to retrieve the raster pointer\n");
	  return -7;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_NONE, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 0, endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - uncompressed\n");
	  return -8;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_PNG, &blob_odd_png, &blob_odd_sz_png,
	 &blob_even_png, &blob_even_sz_png, 0, endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - PNG\n");
	  return -9;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_GIF, &blob_odd_gif, &blob_odd_sz_gif,
	 &blob_even_gif, &blob_even_sz_gif, 0, endian) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - GIF\n");
	  return -10;
      }

    plt2 = rl2_clone_palette (rl2_get_raster_palette (raster));
    stats =
	rl2_get_raster_statistics (blob_odd_png, blob_odd_sz_png, blob_even_png,
				   blob_even_sz_png, plt2, NULL);
    if (stats == NULL)
      {
	  fprintf (stderr, "Unable to get Raster Statistics\n");
	  return -100;
      }
    if (rl2_serialize_dbms_raster_statistics
	(stats, &blob_stat, &blob_stat_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to serialize Raster Statistics\n");
	  return -101;
      }
    rl2_destroy_raster_statistics (stats);
    stats = rl2_deserialize_dbms_raster_statistics (blob_stat, blob_stat_size);
    if (stats == NULL)
      {
	  fprintf (stderr, "Unable to deserialize Raster Statistics\n");
	  return -102;
      }
    free (blob_stat);
    rl2_destroy_raster_statistics (stats);

    rl2_destroy_section (img);

    palette = create_palette ();
    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, palette);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - uncompressed\n");
	  return -11;
      }

    img = rl2_create_section ("4colors 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - uncompressed\n");
	  return -12;
      }

    if (rl2_section_to_png (img, "./4colors_1_1_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 4colors_1_1_uncompressed.png\n");
	  return -13;
      }
    rl2_destroy_section (img);

    unlink ("./4colors_1_1_uncompressed.png");

    if (rl2_raster_decode (RL2_SCALE_2, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:2 - uncompressed\n");
	  return -14;
      }

    if (rl2_raster_decode (RL2_SCALE_4, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:4 - uncompressed\n");
	  return -15;
      }

    if (rl2_raster_decode (RL2_SCALE_8, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:8 - uncompressed\n");
	  return -16;
      }
    free (blob_odd);

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - png\n");
	  return -17;
      }

    img = rl2_create_section ("4colors 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - png\n");
	  return -18;
      }

    if (rl2_section_to_png (img, "./4colors_1_1_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 4colors_1_1_png.png\n");
	  return -19;
      }
    rl2_destroy_section (img);

    unlink ("./4colors_1_1_png.png");

    if (rl2_raster_decode
	(RL2_SCALE_2, blob_odd_png, blob_odd_sz_png, blob_even_png,
	 blob_even_sz_png, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Decode 1:2 - png\n");
	  return -20;
      }
    free (blob_odd_png);

    return 0;
}
