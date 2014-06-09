/*

 test_section.c -- RasterLite2 Test Case

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
#include <stdio.h>

#include "rasterlite2/rasterlite2.h"

int
main (int argc, char *argv[])
{
    rl2RasterPtr raster;
    rl2SectionPtr section;
    unsigned char compression;
    int is_compressed;
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned char *bufpix = malloc (256 * 256);

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a raster\n");
	  return -1;
      }

    if (rl2_create_section (NULL, RL2_COMPRESSION_NONE, 1024, 1024, raster) !=
	NULL)
      {
	  fprintf (stderr, "Invalid section - NULL name\n");
	  return -2;
      }

    if (rl2_create_section ("alpha", 0xff, 1024, 1024, raster) != NULL)
      {
	  fprintf (stderr, "Invalid section - invalid compression\n");
	  return -3;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE, 1025, 1024, raster)
	!= NULL)
      {
	  fprintf (stderr, "Invalid section - invalid tile width\n");
	  return -4;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE, 1024, 255, raster) !=
	NULL)
      {
	  fprintf (stderr, "Invalid section - invalid tile height\n");
	  return -5;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE, 256, 510, raster) !=
	NULL)
      {
	  fprintf (stderr, "Invalid section - invalid tile width (x8)\n");
	  return -6;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE, 510, 256, raster) !=
	NULL)
      {
	  fprintf (stderr, "Invalid section - invalid tile height (x8)\n");
	  return -7;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE, 256, 256, NULL) !=
	NULL)
      {
	  fprintf (stderr, "Invalid section - NULL raster\n");
	  return -8;
      }

    section =
	rl2_create_section ("alpha", RL2_COMPRESSION_NONE, 256, 512, raster);
    if (section == NULL)
      {
	  fprintf (stderr, "Unable to create a valid section (256x512)\n");
	  return -9;
      }

    if (rl2_get_section_tile_size (section, &tile_width, &tile_height) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Section Tile Size\n");
	  return -10;
      }

    if (tile_width != 256)
      {
	  fprintf (stderr, "Unexpected section tile width (256)\n");
	  return -11;
      }

    if (tile_height != 512)
      {
	  fprintf (stderr, "Unexpected section tile height (512)\n");
	  return -12;
      }

    rl2_destroy_section (section);

/* re-creating another raster */
    bufpix = malloc (256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a raster (take two)\n");
	  return -13;
      }

    section =
	rl2_create_section ("alpha", RL2_COMPRESSION_NONE,
			    RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			    raster);
    if (section == NULL)
      {
	  fprintf (stderr, "Unable to create a valid section (untiled)\n");
	  return -14;
      }

    if (rl2_get_section_name (section) == NULL)
      {
	  fprintf (stderr, "Unable to retrieve the section name\n");
	  return -15;
      }

    if (rl2_get_section_compression (section, &compression) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get section compression\n");
	  return -16;
      }

    if (compression != RL2_COMPRESSION_NONE)
      {
	  fprintf (stderr, "Unexpected section compression\n");
	  return -17;
      }

    if (rl2_is_section_uncompressed (section, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get section not compressed\n");
	  return -18;
      }

    if (is_compressed != RL2_TRUE)
      {
	  fprintf (stderr, "Unexpected section not compressed\n");
	  return -19;
      }

    if (rl2_is_section_compression_lossless (section, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get section compression lossless\n");
	  return -20;
      }

    if (is_compressed != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected section compression lossless\n");
	  return -21;
      }

    if (rl2_is_section_compression_lossy (section, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get section compression lossy\n");
	  return -22;
      }

    if (is_compressed != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected section compression lossy\n");
	  return -23;
      }

    if (rl2_get_section_tile_size (section, &tile_width, &tile_height) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get tile size (untiled)\n");
	  return -24;
      }
    if (tile_width != RL2_TILESIZE_UNDEFINED)
      {
	  fprintf (stderr, "Unexpected section tile width (untiled)\n");
	  return -25;
      }
    if (tile_height != RL2_TILESIZE_UNDEFINED)
      {
	  fprintf (stderr, "Unexpected section tile width (untiled)\n");
	  return -26;
      }

    rl2_destroy_section (section);

    if (rl2_get_section_name (NULL) != NULL)
      {
	  fprintf (stderr, "ERROR: NULL section name\n");
	  return -27;
      }

    if (rl2_get_section_compression (NULL, &compression) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL section compression\n");
	  return -28;
      }

    if (rl2_is_section_uncompressed (NULL, &is_compressed) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL section not compressed\n");
	  return -29;
      }

    if (rl2_is_section_compression_lossless (NULL, &is_compressed) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL section compression lossless\n");
	  return -30;
      }

    if (rl2_is_section_compression_lossy (NULL, &is_compressed) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL section compression lossy\n");
	  return -31;
      }

    if (rl2_get_section_tile_size (NULL, &tile_width, &tile_height) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL section tile size\n");
	  return -32;
      }

/* re-creating another raster */
    bufpix = malloc (256 * 256);
    raster =
	rl2_create_raster (256, 256, RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			   bufpix, 256 * 256, NULL, NULL, 0, NULL);
    if (raster == NULL)
      {
	  fprintf (stderr, "Unable to create a raster (take three)\n");
	  return -33;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE,
			    RL2_TILESIZE_UNDEFINED, 1204, raster) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a valid section (bad tile width)\n");
	  return -34;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE,
			    RL2_TILESIZE_UNDEFINED, 1204, raster) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a valid section (bad tile height)\n");
	  return -35;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE,
			    1205, 1204, raster) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a valid section (bad tile width)\n");
	  return -36;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE,
			    1204, 1205, raster) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a valid section (bad tile height)\n");
	  return -37;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE,
			    64, 1204, raster) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a valid section (bad tile width)\n");
	  return -38;
      }

    if (rl2_create_section ("alpha", RL2_COMPRESSION_NONE,
			    1204, 64, raster) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a valid section (bad tile height)\n");
	  return -39;
      }

    rl2_destroy_raster (raster);

    return 0;
}
