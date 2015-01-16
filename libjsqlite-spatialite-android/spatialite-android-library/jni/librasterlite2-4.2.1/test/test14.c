/*

 test14.c -- RasterLite2 Test Case

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

static rl2SectionPtr
create_grid_u16 ()
{
/* creating a synthetic "grid_u16" image */
    rl2SectionPtr scn;
    rl2RasterPtr rst;
    int row;
    int col;
    int bufsize = 1024 * 1024 * sizeof (unsigned short);
    unsigned short sample = 0;
    unsigned char *bufpix = malloc (bufsize);
    unsigned short *p = (unsigned short *) bufpix;

    if (bufpix == NULL)
	return NULL;

    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = sample;
		if (sample == 65535)
		    sample = 0;
		else
		    sample++;
	    }
      }

    rst =
	rl2_create_raster (1024, 1024, RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1,
			   bufpix, bufsize, NULL, NULL, 0, NULL);
    if (rst == NULL)
	goto error;

    scn = rl2_create_section ("grid_u16", RL2_COMPRESSION_NONE,
			      RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			      rst);
    if (scn == NULL)
	rl2_destroy_raster (rst);
    return scn;
  error:
    free (bufpix);
    return NULL;
}

static int
test_grid_u16 (rl2SectionPtr img)
{
/* testing GRID-U16 buffer functions */
    unsigned char *ubuffer;
    unsigned short *buffer;
    int buf_size;
    int width = 1024;
    unsigned short *p_data1;
    unsigned short *p_data2;
    unsigned short sample;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int is_transparent;
    rl2PixelPtr pxl;
    rl2RasterPtr rst;

    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "GRID-U16 invalid raster pointer\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGB (rst, &ubuffer, &buf_size) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result - get RGB data: GRID-U16\n");
	  return 0;
      }

    if (rl2_raster_data_to_RGBA (rst, &ubuffer, &buf_size) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result -  get RGBA data: GRID-U16\n");
	  return 0;
      }

    if (rl2_raster_data_to_ARGB (rst, &ubuffer, &buf_size) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result -  get ARGB data: GRID-U16\n");
	  return 0;
      }

    if (rl2_raster_data_to_BGR (rst, &ubuffer, &buf_size) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result -  get BGR data: GRID-U16\n");
	  return 0;
      }

    if (rl2_raster_data_to_BGRA (rst, &ubuffer, &buf_size) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result -  get BGRA data: GRID-U16\n");
	  return 0;
      }

    if (rl2_raster_data_to_uint16 (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get UINT16 data: GRID-U16\n");
	  return 0;
      }
    p_data1 = buffer + (20 * width) + 20;
    p_data2 = buffer + (720 * width) + 510;
    if (*p_data1 != 20500)
      {
	  fprintf (stderr, "Unexpected UINT16 pixel #1: GRID-U16\n");
	  return 0;
      }
    if (*p_data2 != 16894)
      {
	  fprintf (stderr, "Unexpected UINT16 pixel #2: GRID-U16\n");
	  return 0;
      }
    rl2_free (buffer);

    pxl = rl2_create_raster_pixel (rst);
    if (pxl == NULL)
      {
	  fprintf (stderr, "Unable to create Pixel for Raster: GRID-U16\n");
	  return 0;
      }

    if (rl2_get_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Pixel (20,20) from Raster: GRID-U16\n");
	  return 0;
      }

    if (rl2_get_pixel_type (pxl, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to create get Pixel Type: GRID-U16\n");
	  return 0;
      }

    if (sample_type != RL2_SAMPLE_UINT16)
      {
	  fprintf (stderr, "Unexpected Pixel SampleType: GRID-U16\n");
	  return 0;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type: GRID-U16\n");
	  return 0;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected Pixel # Bands: GRID-U16\n");
	  return 0;
      }

    if (rl2_get_pixel_sample_uint16 (pxl, RL2_DATAGRID_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel UINT16: GRID-U16\n");
	  return 0;
      }

    if (sample != 20500)
      {
	  fprintf (stderr,
		   "Unexpected Pixel UINT16 Sample value %d: GRID-U16\n",
		   sample);
	  return 0;
      }

    if (rl2_is_pixel_transparent (pxl, &is_transparent) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get Pixel Transparency: GRID-U16\n");
	  return 0;
      }

    if (is_transparent != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected Pixel Transparency: GRID-U16\n");
	  return 0;
      }

    if (rl2_is_pixel_opaque (pxl, &is_transparent) != RL2_OK)

      {
	  fprintf (stderr, "Unable to get Pixel Opacity: GRID-U16\n");
	  return 0;
      }

    if (is_transparent != RL2_TRUE)

      {
	  fprintf (stderr, "Unexpected Pixel Opacity: GRID-U16\n");
	  return 0;
      }

    if (rl2_set_pixel_sample_uint16 (pxl, RL2_DATAGRID_BAND, sample) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected Pixel SetSample UINT16: GRID-U16\n");
	  return 0;
      }

    if (rl2_set_raster_pixel (rst, pxl, 20, 20) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to set Pixel (20,20) into Raster: GRID-U16\n");
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
    unsigned char *anti_odd;
    int anti_odd_sz;
    unsigned char *anti_even;
    int anti_even_sz;
    unsigned char *blob_stat;
    int blob_stat_size;
    int endian = naturalEndian ();
    int anti_endian = antiEndian ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    img = create_grid_u16 ();
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create image: grid_u16\n");
	  return -1;
      }

    if (!test_grid_u16 (img))
	return -2;

    raster = rl2_get_section_raster (img);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to retrieve the raster pointer\n");
	  return -3;
      }

    if (rl2_raster_encode
	(raster, RL2_COMPRESSION_NONE, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 0, endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - uncompressed\n");
	  return -4;
      }

    stats =
	rl2_get_raster_statistics (blob_odd, blob_odd_sz, blob_even,
				   blob_even_sz, NULL, NULL);
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
	(raster, RL2_COMPRESSION_NONE, &anti_odd, &anti_odd_sz, &anti_even,
	 &anti_even_sz, 0, anti_endian) != RL2_OK)
      {
	  fprintf (stderr, "Unable to Encode - uncompressed (anti-endian)\n");
	  return -5;
      }

    rl2_destroy_section (img);

    raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:1 - uncompressed\n");
	  return -6;
      }
    rl2_destroy_raster (raster);

    raster =
	rl2_raster_decode (RL2_SCALE_2, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:2 - uncompressed\n");
	  return -7;
      }
    rl2_destroy_raster (raster);

    raster =
	rl2_raster_decode (RL2_SCALE_4, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:4 - uncompressed\n");
	  return -8;
      }
    rl2_destroy_raster (raster);

    raster =
	rl2_raster_decode (RL2_SCALE_8, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to Decode 1:8 - uncompressed\n");
	  return -9;
      }
    rl2_destroy_raster (raster);
    free (blob_odd);
    free (blob_even);

    raster =
	rl2_raster_decode (RL2_SCALE_1, anti_odd, anti_odd_sz, anti_even,
			   anti_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr,
		   "Unable to Decode 1:1 - uncompressed (anti-endian)\n");
	  return -10;
      }
    rl2_destroy_raster (raster);

    raster =
	rl2_raster_decode (RL2_SCALE_2, anti_odd, anti_odd_sz, anti_even,
			   anti_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr,
		   "Unable to Decode 1:2 - uncompressed (anti-endian)\n");
	  return -11;
      }
    rl2_destroy_raster (raster);

    raster =
	rl2_raster_decode (RL2_SCALE_4, anti_odd, anti_odd_sz, anti_even,
			   anti_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr,
		   "Unable to Decode 1:4 - uncompressed (anti-endian)\n");
	  return -12;
      }
    rl2_destroy_raster (raster);

    raster =
	rl2_raster_decode (RL2_SCALE_8, anti_odd, anti_odd_sz, anti_even,
			   anti_even_sz, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr,
		   "Unable to Decode 1:8 - uncompressed (anti-endian)\n");
	  return -13;
      }
    rl2_destroy_raster (raster);
    free (anti_odd);
    free (anti_even);

    return 0;
}
