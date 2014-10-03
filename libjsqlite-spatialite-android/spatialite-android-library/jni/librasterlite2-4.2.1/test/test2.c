/*

 test2.c -- RasterLite2 Test Case

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
create_rainbow ()
{
/* creating a synthetic "rainbow" image */
    rl2SectionPtr scn;
    rl2RasterPtr rst;
    int row;
    int col;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    red = 0;
    for (row = 0; row < 256; row++)
      {
	  green = 0;
	  blue = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		green++;
		blue--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = red;
		*p++ = 127;
		*p++ = 127;
	    }
	  green = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = 255 - red;
		*p++ = green;
		*p++ = blue;
		green--;
		blue++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = 127;
		*p++ = red;
		*p++ = red;
	    }
	  red++;
      }

    green = 0;
    for (row = 256; row < 512; row++)
      {
	  red = 0;
	  blue = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		red++;
		blue--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = 127;
		*p++ = green;
		*p++ = 127;
	    }
	  red = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = red;
		*p++ = 255 - green;
		*p++ = blue;
		red--;
		blue++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = green;
		*p++ = 127;
		*p++ = green;
	    }
	  green++;
      }

    blue = 0;
    for (row = 512; row < 768; row++)
      {
	  red = 0;
	  green = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		red++;
		green--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = 127;
		*p++ = 127;
		*p++ = blue;
	    }
	  red = 255;
	  green = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = 255 - blue;
		red--;
		green++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = blue;
		*p++ = blue;
		*p++ = 127;
	    }
	  blue++;
      }

    red = 0;
    for (row = 768; row < 1024; row++)
      {
	  for (col = 0; col < 256; col++)
	    {
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = red;
		*p++ = red;
		*p++ = red;
	    }
	  green = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = 255 - red;
		*p++ = 255 - red;
		*p++ = 255 - red;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = 255;
		*p++ = 255;
		*p++ = 255;
	    }
	  red++;
      }

    rst =
	rl2_create_raster (1024, 1024, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   bufpix, bufsize, NULL, NULL, 0, NULL);
    if (rst == NULL)
      {
	  free (bufpix);
	  return NULL;
      }

    scn = rl2_create_section ("rainbow", RL2_COMPRESSION_NONE,
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
    unsigned char *blob_odd_zip;
    int blob_odd_sz_zip;
    unsigned char *blob_even_zip;
    int blob_even_sz_zip;
    unsigned char *blob_odd_lzma;
    int blob_odd_sz_lzma;
    unsigned char *blob_even_lzma;
    int blob_even_sz_lzma;
    unsigned char *blob_odd_jpeg;
    int blob_odd_sz_jpeg;
    unsigned char *blob_even_jpeg;
    int blob_even_sz_jpeg;
    unsigned char *blob_odd_lossy_webp;
    int blob_odd_sz_lossy_webp;
    unsigned char *blob_even_lossy_webp;
    int blob_even_sz_lossy_webp;
    unsigned char *blob_odd_lossless_webp;
    int blob_odd_sz_lossless_webp;
    unsigned char *blob_even_lossless_webp;
    int blob_even_sz_lossless_webp;
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
    int anti_endian = antiEndian ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    img = create_rainbow ();
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create image: rainbow\n");
	  return -1;
      }

    if (rl2_section_to_jpeg (img, "./rainbow.jpg", 70) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow.jpg\n");
	  return -2;
      }

    if (rl2_section_to_png (img, "./rainbow.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow.png\n");
	  return -3;
      }

    rl2_destroy_section (img);

    unlink ("./rainbow.jpg");

    img = rl2_section_from_png ("./rainbow.png");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./rainbow.png");
	  return -4;
      }

    unlink ("./rainbow.png");

    if (rl2_section_to_png (img, "./from_rainbow.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: from_rainbow.png\n");
	  return -5;
      }

    unlink ("./from_rainbow.png");

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
	(raster, RL2_COMPRESSION_DEFLATE, &blob_odd_zip, &blob_odd_sz_zip,
	 &blob_even_zip, &blob_even_sz_zip, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - DEFLATE compressed\n");
	  return -8;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_LZMA, &blob_odd_lzma, &blob_odd_sz_lzma,
	 &blob_even_lzma, &blob_even_sz_lzma, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - LZMA compressed\n");
	  return -9;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_JPEG, &blob_odd_jpeg, &blob_odd_sz_jpeg,
	 &blob_even_jpeg, &blob_even_sz_jpeg, 70, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - JPEG compressed\n");
	  return -10;
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

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_LOSSY_WEBP, &blob_odd_lossy_webp,
	 &blob_odd_sz_lossy_webp, &blob_even_lossy_webp,
	 &blob_even_sz_lossy_webp, 10, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - lossy WEBP compressed\n");
	  return -11;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_LOSSLESS_WEBP, &blob_odd_lossless_webp,
	 &blob_odd_sz_lossless_webp, &blob_even_lossless_webp,
	 &blob_even_sz_lossless_webp, 10, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - lossless WEBP compressed\n");
	  return -12;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_PNG, &blob_odd_png, &blob_odd_sz_png,
	 &blob_even_png, &blob_even_sz_png, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - PNG compressed\n");
	  return -13;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_GIF, &blob_odd_gif, &blob_odd_sz_gif,
	 &blob_even_gif, &blob_even_sz_gif, 0, anti_endian) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected result - GIF compressed\n");
	  return -14;
      }

    rl2_destroy_section (img);

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - uncompressed\n");
	  return -15;
      }

    img = rl2_create_section ("rainbow 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - uncompressed\n");
	  return -16;
      }

    if (rl2_section_to_png (img, "./rainbow_1_1_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_1_uncompressed.png\n");
	  return -17;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_1_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - uncompressed\n");
	  return -18;
      }

    img = rl2_create_section ("rainbow 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - uncompressed\n");
	  return -19;
      }

    if (rl2_section_to_png (img, "./rainbow_1_2_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_2_uncompressed.png\n");
	  return -20;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_2_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - uncompressed\n");
	  return -21;
      }

    img = rl2_create_section ("rainbow 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - uncompressed\n");
	  return -22;
      }

    if (rl2_section_to_png (img, "./rainbow_1_4_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_4_uncompressed.png\n");
	  return -23;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_4_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - uncompressed\n");
	  return -24;
      }

    img = rl2_create_section ("rainbow 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - uncompressed\n");
	  return -25;
      }

    if (rl2_section_to_png (img, "./rainbow_1_8_uncompressed.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_8_uncompressed.png\n");
	  return -26;
      }
    rl2_destroy_section (img);
    free (blob_odd);
    free (blob_even);

    unlink ("./rainbow_1_8_uncompressed.png");

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_jpeg, blob_odd_sz_jpeg,
			   blob_even_jpeg, blob_even_sz_jpeg, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - jpeg\n");
	  return -51;
      }

    img = rl2_create_section ("rainbow 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - jpeg\n");
	  return -52;
      }

    if (rl2_section_to_png (img, "./rainbow_1_1_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_1_jpeg.png\n");
	  return -53;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_1_jpeg.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd_jpeg, blob_odd_sz_jpeg,
			   blob_even_jpeg, blob_even_sz_jpeg, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - jpeg\n");
	  return -54;
      }

    img = rl2_create_section ("rainbow 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - jpeg\n");
	  return -55;
      }

    if (rl2_section_to_png (img, "./rainbow_1_2_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_2_jpeg.png\n");
	  return -56;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_2_jpeg.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd_jpeg, blob_odd_sz_jpeg,
			   blob_even_jpeg, blob_even_sz_jpeg, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - jpeg\n");
	  return -57;
      }

    img = rl2_create_section ("rainbow 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - jpeg\n");
	  return -58;
      }

    if (rl2_section_to_png (img, "./rainbow_1_4_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_4_jpeg.png\n");
	  return -59;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_4_jpeg.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd_jpeg, blob_odd_sz_jpeg,
			   blob_even_jpeg, blob_even_sz_jpeg, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - jpeg\n");
	  return -60;
      }

    img = rl2_create_section ("rainbow 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - jpeg\n");
	  return -61;
      }

    if (rl2_section_to_png (img, "./rainbow_1_8_jpeg.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_8_jpeg.png\n");
	  return -62;
      }
    rl2_destroy_section (img);
    free (blob_odd_jpeg);
    free (blob_even_jpeg);

    unlink ("./rainbow_1_8_jpeg.png");

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_lossy_webp,
			   blob_odd_sz_lossy_webp, blob_even_lossy_webp,
			   blob_even_sz_lossy_webp, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - lossy webp\n");
	  return -63;
      }

    img = rl2_create_section ("rainbow 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - lossy webp\n");
	  return -64;
      }

    if (rl2_section_to_png (img, "./rainbow_1_1_lossy_webp.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_1_lossy_webp.png\n");
	  return -65;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_1_lossy_webp.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd_lossy_webp,
			   blob_odd_sz_lossy_webp, blob_even_lossy_webp,
			   blob_even_sz_lossy_webp, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - lossy webp\n");
	  return -66;
      }

    img = rl2_create_section ("rainbow 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - lossy webp\n");
	  return -67;
      }

    if (rl2_section_to_png (img, "./rainbow_1_2_lossy_webp.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_2_lossy_webp.png\n");
	  return -68;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_2_lossy_webp.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd_lossy_webp,
			   blob_odd_sz_lossy_webp, blob_even_lossy_webp,
			   blob_even_sz_lossy_webp, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - lossy webp\n");
	  return -69;
      }

    img = rl2_create_section ("rainbow 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - lossy webp\n");
	  return -70;
      }

    if (rl2_section_to_png (img, "./rainbow_1_4_lossy_webp.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_4_lossy_webp.png\n");
	  return -71;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_4_lossy_webp.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd_lossy_webp,
			   blob_odd_sz_lossy_webp, blob_even_lossy_webp,
			   blob_even_sz_lossy_webp, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - lossy webp\n");
	  return -72;
      }

    img = rl2_create_section ("rainbow 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - lossy webp\n");
	  return -73;
      }

    if (rl2_section_to_png (img, "./rainbow_1_8_lossy_webp.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_8_lossy_webp.png\n");
	  return -74;
      }
    rl2_destroy_section (img);
    free (blob_odd_lossy_webp);
    free (blob_even_lossy_webp);

    unlink ("./rainbow_1_8_lossy_webp.png");

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_lossless_webp,
			   blob_odd_sz_lossless_webp, blob_even_lossless_webp,
			   blob_even_sz_lossless_webp, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - lossless webp\n");
	  return -75;
      }

    img = rl2_create_section ("rainbow 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - lossless webp\n");
	  return -76;
      }

    if (rl2_section_to_png (img, "./rainbow_1_1_lossless_webp.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_1_lossless_webp.png\n");
	  return -77;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_1_lossless_webp.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd_lossless_webp,
			   blob_odd_sz_lossless_webp, blob_even_lossless_webp,
			   blob_even_sz_lossless_webp, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - lossless webp\n");
	  return -78;
      }

    img = rl2_create_section ("rainbow 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - lossless webp\n");
	  return -79;
      }

    if (rl2_section_to_png (img, "./rainbow_1_2_lossless_webp.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_2_lossless_webp.png\n");
	  return -80;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_2_lossless_webp.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd_lossless_webp,
			   blob_odd_sz_lossless_webp, blob_even_lossless_webp,
			   blob_even_sz_lossless_webp, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - lossless webp\n");
	  return -81;
      }

    img = rl2_create_section ("rainbow 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - lossless webp\n");
	  return -82;
      }

    if (rl2_section_to_png (img, "./rainbow_1_4_lossless_webp.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_4_lossless_webp.png\n");
	  return -83;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_4_lossless_webp.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd_lossless_webp,
			   blob_odd_sz_lossless_webp, blob_even_lossless_webp,
			   blob_even_sz_lossless_webp, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - lossless webp\n");
	  return -84;
      }

    img = rl2_create_section ("rainbow 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - lossless webp\n");
	  return -85;
      }

    if (rl2_section_to_png (img, "./rainbow_1_8_lossless_webp.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_8_lossless_webp.png\n");
	  return -86;
      }
    rl2_destroy_section (img);
    free (blob_odd_lossless_webp);
    free (blob_even_lossless_webp);

    unlink ("./rainbow_1_8_lossless_webp.png");

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - png\n");
	  return -87;
      }

    img = rl2_create_section ("rainbow 1:1", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:1 - png\n");
	  return -88;
      }

    if (rl2_section_to_png (img, "./rainbow_1_1_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_1_png.png\n");
	  return -89;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_1_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - png\n");
	  return -90;
      }

    img = rl2_create_section ("rainbow 1:2", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:2 - png\n");
	  return -91;
      }

    if (rl2_section_to_png (img, "./rainbow_1_2_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_2_png.png\n");
	  return -92;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_2_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - png\n");
	  return -93;
      }

    img = rl2_create_section ("rainbow 1:4", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:4 - png\n");
	  return -94;
      }

    if (rl2_section_to_png (img, "./rainbow_1_4_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_4_png.png\n");
	  return -95;
      }
    rl2_destroy_section (img);

    unlink ("./rainbow_1_4_png.png");

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd_png, blob_odd_sz_png,
			   blob_even_png, blob_even_sz_png, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - png\n");
	  return -96;
      }

    img = rl2_create_section ("rainbow 1:8", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Section 1:8 - png\n");
	  return -97;
      }

    if (rl2_section_to_png (img, "./rainbow_1_8_png.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: rainbow_1_8_png.png\n");
	  return -98;
      }
    rl2_destroy_section (img);
    free (blob_odd_png);
    free (blob_even_png);

    unlink ("./rainbow_1_8_png.png");

    return 0;
}
