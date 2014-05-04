/*

 test7.c -- RasterLite2 Test Case

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
/* creating a "256colors" palette */
    int idx;
    const char *rgb[] = {
	"#FFB6C1", "#FFC0CB", "#DC143C", "#FFF0F5", "#DB7093", "#FF69B4",
	"#FF1493", "#C71585", "#DA70D6", "#D8BFD8",
	"#DDA0DD", "#EE82EE", "#FF00FF", "#FF00FF", "#8B008B", "#800080",
	"#BA55D3", "#9400D3", "#9932CC", "#4B0082",
	"#8A2BE2", "#9370DB", "#7B68EE", "#6A5ACD", "#483D8B", "#F8F8FF",
	"#E6E6FA", "#0000FF", "#0000CD", "#00008B",
	"#000080", "#191970", "#4169E1", "#6495ED", "#B0C4DE",
	"#778899", "#708090", "#1E90FF", "#F0F8FF", "#4682B4", "#87CEFA",
	"#87CEEB", "#00BFFF", "#ADD8E6", "#B0E0E6",
	"#5F9EA0", "#00CED1", "#F0FFFF", "#E0FFFF", "#AFEEEE", "#00FFFF",
	"#008B8B", "#008080", "#2F4F4F", "#48D1CC",
	"#20B2AA", "#40E0D0", "#7FFFD4", "#66CDAA", "#00FA9A", "#F5FFFA",
	"#00FF7F", "#3CB371", "#2E8B57", "#F0FFF0",
	"#98FB98", "#90EE90", "#32CD32", "#00FF00", "#228B22", "#008000",
	"#006400", "#7CFC00", "#7FFF00", "#ADFF2F",
	"#556B2F", "#9ACD32", "#6B8E23", "#FFFFF0", "#F5F5DC", "#FFFFE0",
	"#FAFAD2", "#FFFF00", "#808000", "#BDB76B",
	"#EEE8AA", "#FFFACD", "#F0E68C", "#FFD700", "#FFF8DC", "#DAA520",
	"#B8860B", "#FFFAF0",
	"#FDF5E6", "#F5DEB3", "#FFA500", "#FFE4B5", "#FFEFD5", "#FFEBCD",
	"#FFDEAD", "#FAEBD7", "#D2B48C", "#DEB887",
	"#FF8C00", "#FFE4C4", "#FAF0E6", "#CD853F", "#FFDAB9", "#F4A460",
	"#D2691E", "#8B4513", "#FFF5EE", "#A0522D",
	"#FFA07A", "#FF7F50", "#FF4500", "#E9967A", "#FF6347", "#FA8072",
	"#FFE4E1", "#F08080", "#FFFAFA", "#BC8F8F",
	"#CD5C5C", "#FF0000", "#A52A2A", "#B22222", "#8B0000", "#800000",
	"#FFFFFF", "#F5F5F5", "#DCDCDC", "#D3D3D3",
	"#C0C0C0", "#A9A9A9", "#808080", "#696969", "#000000", NULL
    };
    const char *p_rgb;
    rl2PalettePtr palette;

    idx = 0;
    while (1)
      {
	  p_rgb = rgb[idx];
	  if (p_rgb == NULL)
	      break;
	  idx++;
      }
    palette = rl2_create_palette (idx);
    if (palette == NULL)
	goto error;
    idx = 0;
    while (1)
      {
	  p_rgb = rgb[idx];
	  if (p_rgb == NULL)
	      break;
	  if (rl2_set_palette_hexrgb (palette, idx, p_rgb) != RL2_OK)
	      goto error;
	  idx++;
      }
    return palette;
  error:
    return NULL;
}

static rl2SectionPtr
create_256colors ()
{
/* creating a synthetic "256colors" image */
    rl2SectionPtr scn;
    rl2RasterPtr rst;
    rl2PalettePtr palette = create_palette ();
    int row;
    int col;
    int bufsize = 1024 * 1024;
    int idx;
    unsigned short max;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    if (palette == NULL)
	goto error;

    rl2_get_palette_entries (palette, &max);
    max--;
    idx = 0;
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
		      if (ix > max)
			  ix = 0;
		  }
	    }
	  idx++;
	  if (idx > max)
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
		      if (ix > max)
			  ix = 0;
		  }
	    }
	  idx++;
	  if (idx > max)
	      idx = 0;
      }

    idx = max;
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
		      ix--;
		      if (ix < 0)
			  ix = max;
		  }
	    }
	  idx--;
	  if (idx < 0)
	      idx = max;
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
		      ix--;
		      if (ix < 0)
			  ix = max;
		  }
	    }
	  idx--;
	  if (idx < 0)
	      idx = max;
      }

    rst =
	rl2_create_raster (1024, 1024, RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1,
			   bufpix, bufsize, palette, NULL, 0, NULL);
    if (rst == NULL)
	goto error;

    scn = rl2_create_section ("256colors", RL2_COMPRESSION_NONE,
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
test_256colors (rl2SectionPtr img)
{
/* testing 256-COLORS buffer functions */
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
	  fprintf (stderr, "256-COLORS invalid raster pointer\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGB data: 256-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (720 * width * 3) + (510 * 3);
    if (*(p_data1 + 0) != 221 || *(p_data1 + 1) != 160 || *(p_data1 + 2) != 221)
      {
	  fprintf (stderr, "Unexpected RGB pixel #1: 256-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 228 || *(p_data2 + 2) != 181)
      {
	  fprintf (stderr, "Unexpected RGB pixel #2: 256-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: 256-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (720 * width * 4) + (510 * 4);
    if (*(p_data1 + 0) != 221 || *(p_data1 + 1) != 160 || *(p_data1 + 2) != 221
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #1: 256-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 228 || *(p_data2 + 2) != 181
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected RGBA pixel #2: 256-COLORS\n");
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
    if (*(p_data1 + 0) != 255 || *(p_data1 + 1) != 221 || *(p_data1 + 2) != 160
	|| *(p_data1 + 3) != 221)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #1: 256-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 255 || *(p_data2 + 1) != 255 || *(p_data2 + 2) != 228
	|| *(p_data2 + 3) != 181)
      {
	  fprintf (stderr, "Unexpected ARGB pixel #2: 256-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGR (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGR data: 256-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 3) + (20 * 3);
    p_data2 = buffer + (720 * width * 3) + (510 * 3);
    if (*(p_data1 + 0) != 221 || *(p_data1 + 1) != 160 || *(p_data1 + 2) != 221)
      {
	  fprintf (stderr, "Unexpected BGR pixel #1: 4-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 181 || *(p_data2 + 1) != 228 || *(p_data2 + 2) != 255)
      {
	  fprintf (stderr, "Unexpected BGR pixel #2: 256-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_BGRA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BGRA data: 256-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width * 4) + (20 * 4);
    p_data2 = buffer + (720 * width * 4) + (510 * 4);
    if (*(p_data1 + 0) != 221 || *(p_data1 + 1) != 160 || *(p_data1 + 2) != 221
	|| *(p_data1 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #1: 256-COLORS\n");
	  return 0;
      }
    if (*(p_data2 + 0) != 181 || *(p_data2 + 1) != 228 || *(p_data2 + 2) != 255
	|| *(p_data2 + 3) != 255)
      {
	  fprintf (stderr, "Unexpected BGRA pixel #2: 256-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    if (rl2_raster_data_to_uint8 (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get UINT8 data: 256-COLORS\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width) + 20;
    p_data2 = buffer + (720 * width) + 510;
    if (*p_data1 != 10)
      {
	  fprintf (stderr, "Unexpected UINT8 pixel #1: 256-COLORS\n");
	  return 0;
      }
    if (*p_data2 != 96)
      {
	  fprintf (stderr, "Unexpected UINT8 pixel #2: 256-COLORS\n");
	  return 0;
      }
    rl2_free (buffer);

    pxl = rl2_create_raster_pixel (rst);
    if (pxl == NULL)
      {
	  fprintf (stderr, "Unable to create Pixel for Raster: 256-COLORS\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (20,20) from Raster: 256-COLORS\n");
	  return 0;
      }

    if (rl2_get_pixel_type (pxl, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to create get Pixel Type: 256-COLORS\n");
	  return 0;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected Pixel SampleType: 256-COLORS\n");
	  return 0;
      }

    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected Pixel Type: 256-COLORS\n");
	  return 0;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected Pixel # Bands: 256-COLORS\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_uint8 (pxl, RL2_PALETTE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel UINT8: 256-COLORS\n");
	  return 0;
      }

    if (sample != 10)
      {
	  fprintf (stderr,
		   "Unexpected Pixel UINT8 Sample value %d: 256-COLORS\n",
		   sample);
	  return 0;
      }

    if (rl2_set_pixel_sample_uint8 (pxl, RL2_PALETTE_BAND, sample) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel SetSample UINT8: 256-COLORS\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_uint8 (pxl, RL2_PALETTE_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel SetSample UINT8: 256-COLORS\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected SetPixel (20,20) into Raster (exceeding max-palette): 256-COLORS\n");
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

    img = create_256colors ();
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create image: 256colors\n");
	  return -1;
      }

    if (rl2_section_to_jpeg (img, "./256colors.jpg", 70) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256colors.jpg\n");
	  return -2;
      }

    if (rl2_section_to_png (img, "./256colors.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256colors.png\n");
	  return -3;
      }

    if (!test_256colors (img))
	return -4;

    rl2_destroy_section (img);

    unlink ("./256colors.jpg");

    img = rl2_section_from_png ("./256colors.png");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./256colors.png");
	  return -5;
      }

    unlink ("./256colors.png");

    if (rl2_section_to_png (img, "./from_256colors.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_256colors.png\n");
	  return -6;
      }

    unlink ("./from_256colors.png");

    raster = rl2_get_section_raster (img);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to retrieve the raster pointer\n");
	  return -6;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_NONE, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 0, endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - uncompressed\n");
	  return -7;
      }

    plt2 = rl2_clone_palette (rl2_get_raster_palette (raster));
    stats =
	rl2_get_raster_statistics (blob_odd, blob_odd_sz, blob_even,
				   blob_even_sz, plt2, NULL);
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

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_PNG, &blob_odd_png, &blob_odd_sz_png,
	 &blob_even_png, &blob_even_sz_png, 0, endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - PNG\n");
	  return -8;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_GIF, &blob_odd_gif, &blob_odd_sz_gif,
	 &blob_even_gif, &blob_even_sz_gif, 0, endian) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - GIF\n");
	  return -9;
      }

    rl2_destroy_section (img);

    palette = create_palette ();
    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, palette);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - uncompressed\n");
	  return -10;
      }

    img = rl2_create_section ("256color 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - uncompressed\n");
	  return -11;
      }

    if (rl2_section_to_png (img, "./256color_1_1_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256color_1_1_uncompressed.png\n");
	  return -12;
      }
    rl2_destroy_section (img);

    unlink ("./256color_1_1_uncompressed.png");

    palette = create_palette ();
    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, palette);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - uncompressed\n");
	  return -13;
      }

    img = rl2_create_section ("256color 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - uncompressed\n");
	  return -14;
      }

    if (rl2_section_to_png (img, "./256color_1_2_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256color_1_2_uncompressed.png\n");
	  return -15;
      }
    rl2_destroy_section (img);

    unlink ("./256color_1_2_uncompressed.png");

    palette = create_palette ();
    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, palette);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - uncompressed\n");
	  return -16;
      }

    img = rl2_create_section ("256color 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - uncompressed\n");
	  return -17;
      }

    if (rl2_section_to_png (img, "./256color_1_4_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256color_1_4_uncompressed.png\n");
	  return -18;
      }
    rl2_destroy_section (img);

    unlink ("./256color_1_4_uncompressed.png");

    palette = create_palette ();
    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, palette);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - uncompressed\n");
	  return -19;
      }

    img = rl2_create_section ("256color 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - uncompressed\n");
	  return -20;
      }

    if (rl2_section_to_png (img, "./256color_1_8_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256color_1_8_uncompressed.png\n");
	  return -21;
      }
    rl2_destroy_section (img);
    free (blob_odd);
    free (blob_even);

    unlink ("./256color_1_8_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - png\n");
	  return -22;
      }

    img = rl2_create_section ("256color 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - png\n");
	  return -23;
      }

    if (rl2_section_to_png (img, "./256color_1_1_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256color_1_1_png.png\n");
	  return -24;
      }
    rl2_destroy_section (img);

    unlink ("./256color_1_1_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - png\n");
	  return -25;
      }

    img = rl2_create_section ("256color 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - png\n");
	  return -26;
      }

    if (rl2_section_to_png (img, "./256color_1_2_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256color_1_2_png.png\n");
	  return -27;
      }
    rl2_destroy_section (img);

    unlink ("./256color_1_2_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - png\n");
	  return -28;
      }

    img = rl2_create_section ("256color 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - png\n");
	  return -29;
      }

    if (rl2_section_to_png (img, "./256color_1_4_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256color_1_4_png.png\n");
	  return -30;
      }
    rl2_destroy_section (img);

    unlink ("./256color_1_4_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - png\n");
	  return -31;
      }

    img = rl2_create_section ("256color 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - png\n");
	  return -32;
      }

    if (rl2_section_to_png (img, "./256color_1_8_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: 256color_1_8_png.png\n");
	  return -33;
      }
    rl2_destroy_section (img);

    unlink ("./256color_1_8_png.png");

    free (blob_odd_png);
    free (blob_even_png);

    return 0;
}
