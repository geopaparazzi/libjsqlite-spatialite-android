/*

 test3.c -- RasterLite2 Test Case

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

#include "config.h"

#include "rasterlite2/rasterlite2.h"

static int
antiEndian ()
{
/* ensures to always encode in the opposite endian order */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 1;
    return 0;
}

static rl2SectionPtr
create_grayband ()
{
/* creating a synthetic "grayband" image */
    rl2SectionPtr scn;
    rl2RasterPtr rst;
    int row;
    int col;
    unsigned char gray;
    int bufsize = 1024 * 1024;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    gray = 0;
    for (row = 0; row < 256; row++)
      {
	  for (col = 0; col < 1024; col++)
	      *p++ = gray;
	  gray++;
      }

    gray = 255;
    for (row = 256; row < 512; row++)
      {
	  for (col = 0; col < 1024; col++)
	      *p++ = gray;
	  gray--;
      }

    gray = 0;
    for (row = 512; row < 768; row++)
      {
	  for (col = 0; col < 1024; col++)
	      *p++ = gray;
	  gray++;
      }

    gray = 255;
    for (row = 768; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	      *p++ = gray;
	  gray--;
      }

    rst =
	rl2_create_raster (1024, 1024, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, bufsize, NULL, NULL, 0, NULL);
    if (rst == NULL)
      {
	  free (bufpix);
	  return NULL;
      }

    scn = rl2_create_section ("grayband", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      rst);
    if (scn == NULL)
	rl2_destroy_raster (rst);
    return scn;
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
    unsigned char *blob_odd_jpeg;
    int blob_odd_sz_jpeg;
    unsigned char *blob_even_jpeg;
    int blob_even_sz_jpeg;
    unsigned char *blob_odd_png;
    int blob_odd_sz_png;
    unsigned char *blob_even_png;
    int blob_even_sz_png;
    unsigned char *blob_odd_gif;
    int blob_odd_sz_gif;
    unsigned char *blob_even_gif;
    int blob_even_sz_gif;
    unsigned char *blob_stat;
    int blob_stat_size;

#ifndef OMIT_WEBP		/* only if WebP is enabled */
    unsigned char *blob_odd_lossy_webp;
    int blob_odd_sz_lossy_webp;
    unsigned char *blob_even_lossy_webp;
    int blob_even_sz_lossy_webp;
    unsigned char *blob_odd_lossless_webp;
    int blob_odd_sz_lossless_webp;
    unsigned char *blob_even_lossless_webp;
    int blob_even_sz_lossless_webp;
#endif /* end WebP conditional */

    int anti_endian = antiEndian ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    img = create_grayband ();
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create image: grayband\n");
	  return -1;
      }

    if (rl2_section_to_jpeg (img, "./grayband.jpg", 70) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband.jpg\n");
	  return -2;
      }

    if (rl2_section_to_png (img, "./grayband.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband.png\n");
	  return -3;
      }

    rl2_destroy_section (img);

    unlink ("./grayband.jpg");

    img = rl2_section_from_png ("./grayband.png");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./grayband.png");
	  return -4;
      }

    unlink ("./grayband.png");

    if (rl2_section_to_png (img, "./from_grayband.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_grayband.png\n");
	  return -5;
      }

    unlink ("./from_grayband.png");

    raster = rl2_get_section_raster (img);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to retrieve the raster pointer\n");
	  return -6;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_NONE, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - uncompressed\n");
	  return -7;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_JPEG, &blob_odd_jpeg, &blob_odd_sz_jpeg,
	 &blob_even_jpeg, &blob_even_sz_jpeg, 70, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - JPEG compressed\n");
	  return -8;
      }

    stats =
	rl2_get_raster_statistics (blob_odd_jpeg, blob_odd_sz_jpeg,
				   blob_even_jpeg, blob_even_sz_jpeg, NULL,
				   NULL);
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

#ifndef OMIT_WEBP		/* only if WebP is defined */
    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_LOSSY_WEBP, &blob_odd_lossy_webp,
	 &blob_odd_sz_lossy_webp, &blob_even_lossy_webp,
	 &blob_even_sz_lossy_webp, 10, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - lossy WEBP compressed\n");
	  return -9;
      }
    if (blob_odd_lossy_webp != NULL)
	free (blob_odd_lossy_webp);
    if (blob_even_lossy_webp != NULL)
	free (blob_even_lossy_webp);

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_LOSSLESS_WEBP, &blob_odd_lossless_webp,
	 &blob_odd_sz_lossless_webp, &blob_even_lossless_webp,
	 &blob_even_sz_lossless_webp, 10, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - lossless WEBP compressed\n");
	  return -10;
      }
    if (blob_odd_lossless_webp != NULL)
	free (blob_odd_lossless_webp);
    if (blob_even_lossless_webp != NULL)
	free (blob_even_lossless_webp);
#endif /* end WebP conditional */

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_PNG, &blob_odd_png, &blob_odd_sz_png,
	 &blob_even_png, &blob_even_sz_png, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - PNG compressed\n");
	  return -11;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_GIF, &blob_odd_gif, &blob_odd_sz_gif,
	 &blob_even_gif, &blob_even_sz_gif, 0, anti_endian) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - GIF compressed\n");
	  return -12;
      }

    rl2_destroy_section (img);

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - uncompressed\n");
	  return -13;
      }

    img = rl2_create_section ("grayband 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - uncompressed\n");
	  return -14;
      }

    if (rl2_section_to_png (img, "./grayband_1_1_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_1_uncompressed.png\n");
	  return -15;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_1_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - uncompressed\n");
	  return -16;
      }

    img = rl2_create_section ("grayband 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - uncompressed\n");
	  return -17;
      }

    if (rl2_section_to_png (img, "./grayband_1_2_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_2_uncompressed.png\n");
	  return -18;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_2_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - uncompressed\n");
	  return -19;
      }

    img = rl2_create_section ("grayband 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - uncompressed\n");
	  return -20;
      }

    if (rl2_section_to_png (img, "./grayband_1_4_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_4_uncompressed.png\n");
	  return -21;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_4_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - uncompressed\n");
	  return -22;
      }

    img = rl2_create_section ("grayband 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - uncompressed\n");
	  return -23;
      }

    if (rl2_section_to_png (img, "./grayband_1_8_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_8_uncompressed.png\n");
	  return -24;
      }
    rl2_destroy_section (img);
    free (blob_odd);
    free (blob_even);

    unlink ("./grayband_1_8_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_jpeg, blob_odd_sz_jpeg,
			   blob_even_jpeg, blob_even_sz_jpeg, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - jpeg\n");
	  return -25;
      }

    img = rl2_create_section ("grayband 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - jpeg\n");
	  return -26;
      }

    if (rl2_section_to_png (img, "./grayband_1_1_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_1_jpeg.png\n");
	  return -27;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_1_jpeg.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd_jpeg, blob_odd_sz_jpeg,
			   blob_even_jpeg, blob_even_sz_jpeg, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - jpeg\n");
	  return -28;
      }

    img = rl2_create_section ("grayband 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - jpeg\n");
	  return -29;
      }

    if (rl2_section_to_png (img, "./grayband_1_2_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_2_jpeg.png\n");
	  return -30;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_2_jpeg.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd_jpeg, blob_odd_sz_jpeg,
			   blob_even_jpeg, blob_even_sz_jpeg, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - jpeg\n");
	  return -31;
      }

    img = rl2_create_section ("grayband 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - jpeg\n");
	  return -32;
      }

    if (rl2_section_to_png (img, "./grayband_1_4_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_4_jpeg.png\n");
	  return -33;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_4_jpeg.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd_jpeg, blob_odd_sz_jpeg,
			   blob_even_jpeg, blob_even_sz_jpeg, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - jpeg\n");
	  return -34;
      }

    img = rl2_create_section ("grayband 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - jpeg\n");
	  return -35;
      }

    if (rl2_section_to_png (img, "./grayband_1_8_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_8_jpeg.png\n");
	  return -35;
      }
    rl2_destroy_section (img);
    free (blob_odd_jpeg);

    unlink ("./grayband_1_8_jpeg.png");

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - png\n");
	  return -36;
      }

    img = rl2_create_section ("grayband 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - png\n");
	  return -37;
      }

    if (rl2_section_to_png (img, "./grayband_1_1_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_1_png.png\n");
	  return -38;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_1_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - png\n");
	  return -39;
      }

    img = rl2_create_section ("grayband 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - png\n");
	  return -40;
      }

    if (rl2_section_to_png (img, "./grayband_1_2_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_2_png.png\n");
	  return -41;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_2_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - png\n");
	  return -42;
      }

    img = rl2_create_section ("grayband 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - png\n");
	  return -43;
      }

    if (rl2_section_to_png (img, "./grayband_1_4_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_4_png.png\n");
	  return -44;
      }
    rl2_destroy_section (img);

    unlink ("./grayband_1_4_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - png\n");
	  return -45;
      }

    img = rl2_create_section ("grayband 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - png\n");
	  return -46;
      }

    if (rl2_section_to_png (img, "./grayband_1_8_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: grayband_1_8_png.png\n");
	  return -47;
      }
    rl2_destroy_section (img);
    free (blob_odd_png);
    free (blob_even_png);

    unlink ("./grayband_1_8_png.png");

    return 0;
}
