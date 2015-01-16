/*

 test_raster.c -- RasterLite2 Test Case

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
#include <memory.h>
#include <float.h>

#include "rasterlite2/rasterlite2.h"

#include "spatialite.h"

static rl2PalettePtr
build_palette (int num)
{
    int i;
    rl2PalettePtr plt = rl2_create_palette (num);
    for (i = 0; i < num; i++)
	rl2_set_palette_color (plt, i, 255 - i, 255 - i, 255 - i);
    return plt;
}

int
main (int argc, char *argv[])
{
    rl2PalettePtr palette;
    rl2RasterPtr raster;
    rl2PixelPtr no_data;
    rl2PixelPtr pixel;
    rl2SectionPtr img;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned int width;
    unsigned int height;
    int srid;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double hResolution;
    double vResolution;
    unsigned char sample;
    unsigned char *mask;
    unsigned char *bufpix = malloc (256 * 256);
    memset (bufpix, 255, 256 * 256);

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */


    if (rl2_create_raster (0, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 0 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - ZERO width\n");
	  return -1;
      }

    if (rl2_create_raster (256, 0, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 0, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - ZERO height\n");
	  return -2;
      }

    if (rl2_create_raster (256, 256, 0xff, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - invalid sample\n");
	  return -3;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, 0xff, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - invalid pixel\n");
	  return -4;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   NULL, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - NULL pixmap\n");
	  return -5;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 255 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid raster - mismatching pixmap size\n");
	  return -6;
      }

    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRAYSCALE/256\n");
	  return -7;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRAYSCALE/256r\n");
	  return -8;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample GRAYSCALE/256r\n");
	  return -9;
      }

    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected raster pixel GRAYSCALE/256\n");
	  return -10;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRAYSCALE/256\n");
	  return -11;
      }

    if (rl2_get_raster_size (raster, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster size\n");
	  return -12;
      }

    if (width != 256)
      {
	  fprintf (stderr, "Unexpected raster tile width\n");
	  return -13;
      }

    if (height != 256)
      {
	  fprintf (stderr, "Unexpected raster tile height\n");
	  return -14;
      }

    if (rl2_raster_georeference_center (raster, 4326, 0.5, 0.5, 11.5, 42.5) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to georeference a raster (Center point)\n");
	  return -15;
      }

    if (rl2_get_raster_srid (raster, &srid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster SRID\n");
	  return -16;
      }

    if (srid != 4326)
      {
	  fprintf (stderr, "Unexpected raster SRID\n");
	  return -17;
      }

    if (rl2_get_raster_extent (raster, &minX, &minY, &maxX, &maxY) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster extent (Center point)\n");
	  return -18;
      }

    if (minX != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (Center point)\n");
	  return -19;
      }

    if (minY != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (Center point)\n");
	  return -20;
      }

    if (maxX != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (Center point)\n");
	  return -21;
      }

    if (maxY != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (Center point)\n");
	  return -22;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    memset (bufpix, 0, 256 * 256);
    memset (bufpix + (256 * 128), 1, 256);
    memset (bufpix + (256 * 129), 1, 256);
    memset (bufpix + (256 * 130), 1, 256);
    memset (bufpix + (256 * 131), 1, 256);
    memset (bufpix + (256 * 136), 1, 256);
    memset (bufpix + (256 * 137), 1, 256);
    memset (bufpix + (256 * 138), 1, 256);
    memset (bufpix + (256 * 139), 1, 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a MONOCHROME raster\n");
	  return -23;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster sample MONOCHROME\n");
	  return -24;
      }

    if (sample_type != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample MONOCHROME\n");
	  return -25;
      }

    if (pixel_type != RL2_PIXEL_MONOCHROME)
      {
	  fprintf (stderr, "Unexpected raster pixel MONOCHROME\n");
	  return -26;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands MONOCHROME\n");
	  return -27;
      }

    if (rl2_raster_georeference_upper_left
	(raster, 4326, 0.5, 0.5, -52.5, 106.5) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to georeference a raster (UpperLeft corner)\n");
	  return -28;
      }

    if (rl2_get_raster_extent (raster, &minX, &minY, &maxX, &maxY) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster extent (UpperLeft corner)\n");
	  return -29;
      }

    if (minX != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (UpperLeft corner)\n");
	  return -30;
      }

    if (minY != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (UpperLeft corner)\n");
	  return -31;
      }

    if (maxX != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (UpperLeft corner)\n");
	  return -32;
      }

    if (maxY != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (UpperLeft corner)\n");
	  return -33;
      }

    img = rl2_create_section ("mono", RL2_COMPRESSION_NONE, 256, 256, raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create a Monochrome image\n");
	  return -34;
      }
    if (rl2_section_to_gif (img, "./raster_mono.gif") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_mono.gif\n");
	  return -35;
      }
    unlink ("./raster_mono.gif");

    if (rl2_section_to_png (img, "./raster_mono.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_mono.png\n");
	  return -36;
      }
    unlink ("./raster_mono.png");

    if (rl2_section_to_jpeg (img, "./raster_mono.jpg", 60) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_mono.jpg\n");
	  return -37;
      }
    unlink ("./raster_mono.jpg");

    rl2_destroy_section (img);

    bufpix = malloc (256 * 256);
    memset (bufpix, 3, 256 * 256);
    memset (bufpix + (256 * 120), 2, 256);
    memset (bufpix + (256 * 121), 2, 256);
    memset (bufpix + (256 * 122), 2, 256);
    memset (bufpix + (256 * 123), 2, 256);
    memset (bufpix + (256 * 124), 1, 256);
    memset (bufpix + (256 * 125), 1, 256);
    memset (bufpix + (256 * 126), 1, 256);
    memset (bufpix + (256 * 127), 1, 256);
    memset (bufpix + (256 * 128), 0, 256);
    memset (bufpix + (256 * 129), 0, 256);
    memset (bufpix + (256 * 130), 0, 256);
    memset (bufpix + (256 * 131), 0, 256);
    memset (bufpix + (256 * 132), 1, 256);
    memset (bufpix + (256 * 133), 1, 256);
    memset (bufpix + (256 * 134), 1, 256);
    memset (bufpix + (256 * 135), 1, 256);
    memset (bufpix + (256 * 136), 2, 256);
    memset (bufpix + (256 * 137), 2, 256);
    memset (bufpix + (256 * 138), 2, 256);
    memset (bufpix + (256 * 139), 2, 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_2_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRAYSCALE/4 raster\n");
	  return -38;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster sample GRAYSCALE/4\n");
	  return -39;
      }

    if (sample_type != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample GRAYSCALE/4\n");
	  return -40;
      }

    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected raster pixel GRAYSCALE/4\n");
	  return -41;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRAYSCALE/4\n");
	  return -42;
      }

    if (rl2_raster_georeference_lower_left
	(raster, 4326, 0.5, 0.5, -52.5, -21.5) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to georeference a raster (LowerLeft corner)\n");
	  return -43;
      }

    if (rl2_get_raster_extent (raster, &minX, &minY, &maxX, &maxY) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster extent (LowerLeft corner)\n");
	  return -44;
      }

    if (minX != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (LowerLeft corner)\n");
	  return -45;
      }

    if (minY != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (LowerLeft corner)\n");
	  return -46;
      }

    if (maxX != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (LowerLeft corner)\n");
	  return -47;
      }

    if (maxY != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (LowerLeft corner)\n");
	  return -48;
      }

    img = rl2_create_section ("gray4", RL2_COMPRESSION_NONE, 256, 256, raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to creat a Grayscale/4 image\n");
	  return -49;
      }

    if (rl2_section_to_gif (img, "./raster_gray4.gif") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray4.gif\n");
	  return -50;
      }
    unlink ("./raster_gray4.gif");

    if (rl2_section_to_png (img, "./raster_gray4.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray4.png\n");
	  return -51;
      }
    unlink ("./raster_gray4.png");

    if (rl2_section_to_jpeg (img, "./raster_gray4.jpg", 60) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray4.jpg\n");
	  return -52;
      }
    unlink ("./raster_gray4.jpg");

    rl2_destroy_section (img);

    bufpix = malloc (256 * 256);
    memset (bufpix, 15, 256 * 256);
    memset (bufpix + (256 * 120), 0, 256);
    memset (bufpix + (256 * 122), 0, 256);
    memset (bufpix + (256 * 123), 1, 256);
    memset (bufpix + (256 * 124), 2, 256);
    memset (bufpix + (256 * 125), 3, 256);
    memset (bufpix + (256 * 126), 4, 256);
    memset (bufpix + (256 * 127), 5, 256);
    memset (bufpix + (256 * 128), 6, 256);
    memset (bufpix + (256 * 129), 7, 256);
    memset (bufpix + (256 * 130), 8, 256);
    memset (bufpix + (256 * 131), 9, 256);
    memset (bufpix + (256 * 132), 10, 256);
    memset (bufpix + (256 * 133), 10, 256);
    memset (bufpix + (256 * 134), 12, 256);
    memset (bufpix + (256 * 135), 13, 256);
    memset (bufpix + (256 * 136), 14, 256);
    memset (bufpix + (256 * 137), 15, 256);
    memset (bufpix + (256 * 139), 0, 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRAYSCALE/16 raster\n");
	  return -53;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRAYSCALE/16\n");
	  return -54;
      }

    if (sample_type != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample GRAYSCALE/16\n");
	  return -55;
      }

    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected raster pixel GRAYSCALE/16\n");
	  return -56;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRAYSCALE/16\n");
	  return -57;
      }

    if (rl2_raster_georeference_lower_right
	(raster, 4326, 0.5, 0.5, 75.5, -21.5) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to georeference a raster (LowerRight corner)\n");
	  return -58;
      }

    if (rl2_get_raster_extent (raster, &minX, &minY, &maxX, &maxY) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster extent (LowerRight corner)\n");
	  return -59;
      }

    if (minX != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (LowerRight corner)\n");
	  return -60;
      }

    if (minY != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (LowerRight corner)\n");
	  return -61;
      }

    if (maxX != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (LowerRight corner)\n");
	  return -62;
      }

    if (maxY != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (LowerRight corner)\n");
	  return -63;
      }

    img = rl2_create_section ("gray16", RL2_COMPRESSION_NONE, 256, 256, raster);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to creat a Grayscale/16 image\n");
	  return -64;
      }

    if (rl2_section_to_gif (img, "./raster_gray16.gif") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray16.gif\n");
	  return -65;
      }
    unlink ("./raster_gray16.gif");

    if (rl2_section_to_png (img, "./raster_gray16.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray16.png\n");
	  return -66;
      }
    unlink ("./raster_gray16.png");

    if (rl2_section_to_jpeg (img, "./raster_gray16.jpg", 60) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: raster_gray16.jpg\n");
	  return -67;
      }
    unlink ("./raster_gray16.jpg");

    rl2_destroy_section (img);

    bufpix = malloc (3 * 256 * 256);
    memset (bufpix, 255, 3 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   bufpix, 3 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a RGB raster\n");
	  return -68;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type RGB\n");
	  return -69;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample RGB\n");
	  return -70;
      }

    if (pixel_type != RL2_PIXEL_RGB)
      {
	  fprintf (stderr, "Unexpected raster pixel RGB\n");
	  return -71;
      }

    if (num_bands != 3)
      {
	  fprintf (stderr, "Unexpected raster # bands RGB\n");
	  return -72;
      }

    if (rl2_raster_georeference_upper_right
	(raster, 4326, 0.5, 0.5, 75.5, 106.5) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to georeference a raster (UpperRight corner)\n");
	  return -73;
      }

    if (rl2_get_raster_extent (raster, &minX, &minY, &maxX, &maxY) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster extent (UpperRight corner)\n");
	  return -74;
      }

    if (minX != -52.5)
      {
	  fprintf (stderr, "Unexpected raster MinX (UpperRight corner)\n");
	  return -75;
      }

    if (minY != -21.5)
      {
	  fprintf (stderr, "Unexpected raster MinY (UpperRight corner)\n");
	  return -76;
      }

    if (maxX != 75.5)
      {
	  fprintf (stderr, "Unexpected raster MaxX (UpperRight corner)\n");
	  return -77;
      }

    if (maxY != 106.5)
      {
	  fprintf (stderr, "Unexpected raster MaxY (UpperRight corner)\n");
	  return -78;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (2 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 2 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/U16 raster\n");
	  return -79;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRID/U16\n");
	  return -80;
      }

    if (sample_type != RL2_SAMPLE_UINT16)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/U16\n");
	  return -81;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/U16\n");
	  return -82;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/U16\n");
	  return -83;
      }

    if (rl2_raster_georeference_frame (raster, 4326, -52.5, -21.5, 75.5, 106.5)
	!= RL2_OK)
      {
	  fprintf (stderr, "Unable to georeference a raster (frame)\n");
	  return -84;
      }

    if (rl2_raster_georeference_frame (raster, 4326, 82.5, -21.5, 75.5, 106.5)
	!= RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected: georeference a raster (invalid frame)\n");
	  return -85;
      }

    if (rl2_raster_georeference_frame (raster, 4326, -52.5, 121.5, 75.5, 106.5)
	!= RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected: georeference a raster (invalid frame)\n");
	  return -86;
      }

    if (rl2_get_raster_resolution (raster, &hResolution, &vResolution) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster resolution (frame)\n");
	  return -87;
      }

    if (hResolution != 0.5)
      {
	  fprintf (stderr, "Unexpected raster horizontal resolution (frame)\n");
	  return -88;
      }

    if (vResolution != 0.5)
      {
	  fprintf (stderr, "Unexpected raster vertical resolution (frame)\n");
	  return -89;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (4 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_INT32, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 4 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/32 raster\n");
	  return -90;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRID/32\n");
	  return -91;
      }

    if (sample_type != RL2_SAMPLE_INT32)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/32\n");
	  return -92;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/32\n");
	  return -93;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/32\n");
	  return -94;
      }

    if (rl2_get_raster_resolution (raster, &hResolution, &vResolution) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected raster resolution (GRID/32)\n");
	  return -95;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (2 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_INT16, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 2 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/16 raster\n");
	  return -96;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRID/16\n");
	  return -97;
      }

    if (sample_type != RL2_SAMPLE_INT16)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/16\n");
	  return -98;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/16\n");
	  return -99;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/16\n");
	  return -100;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/8 raster\n");
	  return -101;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRID/8\n");
	  return -102;
      }

    if (sample_type != RL2_SAMPLE_INT8)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/8\n");
	  return -103;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/8\n");
	  return -104;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/8\n");
	  return -105;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/U8 raster\n");
	  return -106;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRID/U8\n");
	  return -107;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/U8\n");
	  return -108;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/U8\n");
	  return -109;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/U8\n");
	  return -110;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (4 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT32, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 4 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/U32 raster\n");
	  return -111;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRID/U32\n");
	  return -112;
      }

    if (sample_type != RL2_SAMPLE_UINT32)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/U32\n");
	  return -113;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/U32\n");
	  return -114;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/U32\n");
	  return -115;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (4 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_FLOAT, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 4 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/FLT raster\n");
	  return -116;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRID/FLT\n");
	  return -117;
      }

    if (sample_type != RL2_SAMPLE_FLOAT)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/FLT\n");
	  return -118;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/FLT\n");
	  return -119;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/FLT\n");
	  return -120;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (8 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_DOUBLE, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 8 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRID/DBL raster\n");
	  return -121;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type GRID/DBL\n");
	  return -122;
      }

    if (sample_type != RL2_SAMPLE_DOUBLE)
      {
	  fprintf (stderr, "Unexpected raster sample GRID/DBL\n");
	  return -123;
      }

    if (pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected raster pixel GRID/DBL\n");
	  return -124;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands GRID/DBL\n");
	  return -125;
      }
    rl2_destroy_raster (raster);

    palette = build_palette (2);
    bufpix = malloc (256 * 256);
    memset (bufpix, 1, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a PALETTE/1 raster\n");
	  return -126;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type PALETTE/1\n");
	  return -127;
      }

    if (sample_type != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample PALETTE/1\n");
	  return -128;
      }

    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected raster pixel PALETTE/1\n");
	  return -129;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands PALETTE/1\n");
	  return -130;
      }
    rl2_destroy_raster (raster);

    palette = build_palette (4);
    bufpix = malloc (256 * 256);
    memset (bufpix, 3, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_2_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a PALETTE/2 raster\n");
	  return -131;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type PALETTE/2\n");
	  return -132;
      }

    if (sample_type != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample PALETTE/2\n");
	  return -133;
      }

    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected raster pixel PALETTE/2\n");
	  return -134;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands PALETTE/2\n");
	  return -135;
      }
    rl2_destroy_raster (raster);

    palette = build_palette (16);
    bufpix = malloc (256 * 256);
    memset (bufpix, 15, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a PALETTE/4 raster\n");
	  return -136;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type PALETTE/4\n");
	  return -137;
      }

    if (sample_type != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr, "Unexpected raster sample PALETTE/4\n");
	  return -138;
      }

    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected raster pixel PALETTE/4\n");
	  return -139;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands PALETTE/4\n");
	  return -140;
      }
    rl2_destroy_raster (raster);

    palette = build_palette (256);
    bufpix = malloc (256 * 256);
    memset (bufpix, 255, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a PALETTE/8 raster\n");
	  return -141;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type PALETTE/8\n");
	  return -142;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample PALETTE/8\n");
	  return -143;
      }

    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected raster pixel PALETTE/8\n");
	  return -144;
      }

    if (num_bands != 1)
      {
	  fprintf (stderr, "Unexpected raster # bands PALETTE/8\n");
	  return -145;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (5 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 5,
			   bufpix, 5 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a MULTIBAND/5 raster\n");
	  return -146;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type MULTIBAND/5\n");
	  return -147;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected raster sample MULTIBAND/5\n");
	  return -148;
      }

    if (pixel_type != RL2_PIXEL_MULTIBAND)
      {
	  fprintf (stderr, "Unexpected raster pixel MULTIBAND/5\n");
	  return -149;
      }

    if (num_bands != 5)
      {
	  fprintf (stderr, "Unexpected raster # bands MULTIBAND/5\n");
	  return -150;
      }
    rl2_destroy_raster (raster);

    bufpix = malloc (6 * 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, 3,
			   bufpix, 6 * 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a MULTIBAND/16 raster\n");
	  return -151;
      }

    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get raster type MULTIBAND/16\n");
	  return -152;
      }

    if (sample_type != RL2_SAMPLE_UINT16)
      {
	  fprintf (stderr, "Unexpected raster sample MULTIBAND/16\n");
	  return -153;
      }

    if (pixel_type != RL2_PIXEL_MULTIBAND)
      {
	  fprintf (stderr, "Unexpected raster pixel MULTIBAND/16\n");
	  return -154;
      }

    if (num_bands != 3)
      {
	  fprintf (stderr, "Unexpected raster # bands MULTIBAND/16\n");
	  return -155;
      }
    rl2_destroy_raster (raster);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -156;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Red NoData #1\n");
	  return -157;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Green NoData #1\n");
	  return -158;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Blue NoData #1\n");
	  return -159;
      }

    bufpix = malloc (256 * 256 * 3);
    memset (bufpix, 255, 256 * 256 * 3);

    raster = rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
				bufpix, 256 * 256 * 3, NULL, NULL, 0, no_data);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a valid raster\n");
	  return -160;
      }

    no_data = rl2_get_raster_no_data (raster);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to get NoData from raster #1\n");
	  return -161;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_RED_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RED NoData from raster #1\n");
	  return -162;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected RED NoData from raster #1\n");
	  return -162;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get GREEN NoData from raster #1\n");
	  return -162;
      }

    if (sample != 255)
      {
	  fprintf (stderr, "Unexpected GREEN NoData from raster #1\n");
	  return -165;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BLUE NoData from raster #1\n");
	  return -166;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected BLUE NoData from raster #1\n");
	  return -167;
      }

    no_data = rl2_create_raster_pixel (raster);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a pixel for raster #1\n");
	  return -168;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Red NoData #2\n");
	  return -169;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Green NoData #2\n");
	  return -170;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Blue NoData #2\n");
	  return -171;
      }
    rl2_destroy_pixel (no_data);

    no_data = rl2_get_raster_no_data (raster);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to get NoData from raster #2\n");
	  return -172;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_RED_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RED NoData from raster #2\n");
	  return -173;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected RED NoData from raster #2\n");
	  return -174;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get GREEN NoData from raster #2\n");
	  return -175;
      }

    if (sample != 255)
      {
	  fprintf (stderr, "Unexpected GREEN NoData from raster #2\n");
	  return -176;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BLUE NoData from raster #2\n");
	  return -177;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected BLUE NoData from raster #2\n");
	  return -178;
      }

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -179;
      }

    bufpix = malloc (256 * 256 * 3);
    memset (bufpix, 255, 256 * 256 * 3);

    if (rl2_create_raster (1024, 1024, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0,
			   no_data) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create raster (invalid NoData)\n");
	  return -180;
      }
    free (bufpix);

    rl2_destroy_pixel (no_data);

    rl2_destroy_raster (raster);

    if (rl2_get_raster_no_data (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: get NoData (NULL raster)\n");
	  return -181;
      }

    if (rl2_create_raster_pixel (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: get pixel (NULL raster)\n");
	  return -182;
      }

    bufpix = malloc (256 * 256);
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 2,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unable to create a MONOCHROME raster\n");
	  return -183;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_2_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL))
      {
	  fprintf (stderr, "Unexpected result: create a MONOCHROME raster\n");
	  return -184;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE, 2,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL))
      {
	  fprintf (stderr, "Unexpected result: create a MONOCHROME raster\n");
	  return -185;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_INT8, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MONOCHROME raster\n");
	  return -186;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 2,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a GRAYSCALE raster\n");
	  return -187;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_INT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a GRAYSCALE raster\n");
	  return -188;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 2,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a DATAGRID raster\n");
	  return -189;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_DATAGRID, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a DATAGRID raster\n");
	  return -190;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_MULTIBAND, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -191;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -192;
      }
    free (bufpix);

    bufpix = malloc (256 * 256 * 3);
    if (rl2_create_raster (256, 256, RL2_SAMPLE_FLOAT, RL2_PIXEL_MULTIBAND, 3,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -193;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_FLOAT, RL2_PIXEL_RGB, 3,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -194;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 2,
			   bufpix, 256 * 256 * 3, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: create a MULTIBAND raster\n");
	  return -195;
      }
    free (bufpix);

    bufpix = malloc (256 * 256 * 2);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 2,
			   bufpix, 256 * 256 * 2, NULL, NULL, 0, NULL);

    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create raster (2 Bands)\n");
	  return -196;
      }

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, 2);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -197;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, no_data) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster with NoData\n");
	  return -198;
      }

    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 3);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -199;
      }

    rl2_destroy_pixel (no_data);
    rl2_destroy_raster (raster);

    bufpix = malloc (256 * 256);
    mask = malloc (256 * 256);
    memset (bufpix, 21, 256 * 256);
    memset (mask, 0, 256 * 256);
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, mask, 256 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a MONOCHROME raster with mask\n");
	  return -200;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, mask, 256 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster with mask\n");
	  return -201;
      }

    palette = build_palette (2);
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, mask, 256 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Palette raster with mask\n");
	  return -202;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Grayscale raster with palette\n");
	  return -203;
      }

    *bufpix = 2;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_PALETTE, 1,
			   bufpix, 256 * 256, palette, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Palette raster [invalid buffer]\n");
	  return -204;
      }
    rl2_destroy_palette (palette);

    *bufpix = 2;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a MONOCHROME raster [invalid buffer]\n");
	  return -205;
      }

    *bufpix = 4;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_2_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/4 raster [invalid buffer]\n");
	  return -206;
      }

    *bufpix = 16;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_4_BIT, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster [invalid buffer]\n");
	  return -207;
      }

    *mask = 2;
    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, mask, 256 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster [invalid mask]\n");
	  return -208;
      }

    if (rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, mask, 255 * 256,
			   NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a Gray/16 raster with mask\n");
	  return -209;
      }
    free (bufpix);
    free (mask);

    if (rl2_get_raster_type (NULL, &sample_type, &pixel_type, &num_bands) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster Sample (NULL)\n");
	  return -210;
      }

    if (rl2_get_raster_size (NULL, &width, &height) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster Width (NULL)\n");
	  return -211;
      }

    if (rl2_get_raster_srid (NULL, &srid) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster SRID (NULL)\n");
	  return -212;
      }

    if (rl2_get_raster_extent (NULL, &minX, &minY, &maxX, &maxY) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster extent (NULL)\n");
	  return -213;
      }

    if (rl2_get_raster_resolution (NULL, &hResolution, &vResolution) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster Resolution (NULL)\n");
	  return -214;
      }

    if (rl2_get_raster_palette (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: Raster Palette (NULL)\n");
	  return -215;
      }

    if (rl2_raster_georeference_center (NULL, 4326, 1, 1, 1, 1) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster GeoRef Center (NULL)\n");
	  return -216;
      }

    if (rl2_raster_georeference_upper_left (NULL, 4326, 1, 1, 1, 1) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: Raster GeoRef UpperLeft (NULL)\n");
	  return -217;
      }

    if (rl2_raster_georeference_upper_right (NULL, 4326, 1, 1, 1, 1) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: Raster GeoRef UpperRight (NULL)\n");
	  return -218;
      }

    if (rl2_raster_georeference_lower_left (NULL, 4326, 1, 1, 1, 1) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: Raster GeoRef LowerLeft (NULL)\n");
	  return -219;
      }

    if (rl2_raster_georeference_lower_right (NULL, 4326, 1, 1, 1, 1) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: Raster GeoRef LowerRight (NULL)\n");
	  return -220;
      }

    if (rl2_raster_georeference_frame (NULL, 4326, 1, 1, 1, 1) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: Raster GeoRef Frame (NULL)\n");
	  return -221;
      }

    rl2_destroy_raster (NULL);

    bufpix = malloc (256 * 256);
    memset (bufpix, 255, 256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a GRAYSCALE/256\n");
	  return -223;
      }

    pixel = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create UINT16 pixels for testing\n");
	  return -224;
      }

    if (rl2_get_raster_pixel (NULL, NULL, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (NULL, NULL)\n");
	  return -225;
      }

    if (rl2_set_raster_pixel (NULL, NULL, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (NULL, NULL)\n");
	  return -226;
      }

    if (rl2_get_raster_pixel (raster, NULL, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (valid, NULL)\n");
	  return -227;
      }

    if (rl2_set_raster_pixel (raster, NULL, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (valid, NULL)\n");
	  return -228;
      }

    if (rl2_get_raster_pixel (NULL, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (NULL, valid)\n");
	  return -229;
      }

    if (rl2_set_raster_pixel (NULL, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (NULL, valid)\n");
	  return -230;
      }

    if (rl2_get_raster_pixel (raster, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (mismatching sample)\n");
	  return -231;
      }

    if (rl2_set_raster_pixel (raster, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (mismatching sample)\n");
	  return -232;
      }
    rl2_destroy_pixel (pixel);

    pixel = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create UINT8 pixels for testing\n");
	  return -233;
      }

    if (rl2_get_raster_pixel (raster, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (mismatching pixel)\n");
	  return -234;
      }

    if (rl2_set_raster_pixel (raster, pixel, 10, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (mismatching pixel)\n");
	  return -235;
      }
    rl2_destroy_pixel (pixel);

    pixel = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create Gray/256 pixels for testing\n");
	  return -236;
      }

    if (rl2_get_raster_pixel (raster, pixel, 2000, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (invalid row)\n");
	  return -237;
      }

    if (rl2_set_raster_pixel (raster, pixel, 2000, 10) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (invalid row)\n");
	  return -238;
      }

    if (rl2_get_raster_pixel (raster, pixel, 10, 2000) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to get raster pixel (invalid col)\n");
	  return -239;
      }

    if (rl2_set_raster_pixel (raster, pixel, 10, 2000) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set raster pixel (invalid col)\n");
	  return -240;
      }
    rl2_destroy_pixel (pixel);

    rl2_destroy_raster (raster);

    return 0;
}
