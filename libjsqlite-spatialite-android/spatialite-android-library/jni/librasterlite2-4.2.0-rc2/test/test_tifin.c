/*

 test_tifin.c -- RasterLite2 Test Case

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
#include <string.h>

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"

static int
test_mem_tiff (const char *name)
{
/* testing a simple MEM-TIFF source */
    char path[1024];
    unsigned char *p_blob = NULL;
    int n_bytes;
    int rd;
    FILE *in = NULL;
    rl2RasterPtr rst = NULL;
    char *msg;

/* loading the image in-memory */
    sprintf (path, "./%s.tif", name);
    msg = sqlite3_mprintf ("Unable to load \"%s\" in-memory\n", path);
    in = fopen (path, "rb");
    if (in == NULL)
	goto error;
    if (fseek (in, 0, SEEK_END) < 0)
	goto error;
    n_bytes = ftell (in);
    rewind (in);
    p_blob = malloc (n_bytes);
    rd = fread (p_blob, 1, n_bytes, in);
    fclose (in);
    in = NULL;
    if (rd != n_bytes)
	goto error;
/* decoding */
    sqlite3_free (msg);
    msg = sqlite3_mprintf ("Unable to decode in-memory \"%s\"\n", path);
    rst = rl2_raster_from_tiff (p_blob, n_bytes);
    if (rst == NULL)
	goto error;
    free (p_blob);
    rl2_destroy_raster (rst);
    sqlite3_free (msg);
    return 0;

  error:
    if (p_blob != NULL)
	free (p_blob);
    if (in != NULL)
	fclose (in);
    fprintf (stderr, msg);
    sqlite3_free (msg);
    return -100;
}

static int
test_tiff (const char *name, unsigned char sample_type,
	   unsigned char pixel_type, unsigned char nBands)
{
/* testing a TIFF source */
    rl2TiffOriginPtr origin = NULL;
    rl2CoveragePtr coverage = NULL;
    rl2RasterPtr raster = NULL;
    rl2SectionPtr img = NULL;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
    unsigned char compression;
    char tile_name[256];
    char path[1024];
    int is_geotiff;
    int is_tfw;
    const char *tiff_path;
    const char *tfw_path;

    coverage =
	rl2_create_coverage (name, sample_type, pixel_type, nBands,
			     RL2_COMPRESSION_PNG, 0, 432, 432, NULL);
    if (coverage == NULL)
      {
	  fprintf (stderr, "ERROR: unable to create the Coverage\n");
	  return -1;
      }

    sprintf (path, "./%s.tif", name);
    origin =
	rl2_create_tiff_origin (path, RL2_TIFF_GEOTIFF, -1, RL2_SAMPLE_UNKNOWN,
				RL2_PIXEL_UNKNOWN, RL2_BANDS_UNKNOWN);
    if (origin == NULL)
      {
	  fprintf (stderr, "ERROR: unable to open %s\n", path);
	  return -2;
      }

    if (rl2_eval_tiff_origin_compatibility (coverage, origin, -1) != RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/TIFF mismatch: %s\n", name);
	  return -3;
      }

    if (rl2_get_tile_from_tiff_origin (coverage, origin, 1, 0, -1) != NULL)
      {
	  fprintf (stderr, "Unexpected result: startRow mismatch\n");
	  return -4;
      }

    if (rl2_get_tile_from_tiff_origin (coverage, origin, 0, 1, -1) != NULL)
      {
	  fprintf (stderr, "Unexpected result: startCol mismatch\n");
	  return -5;
      }

    if (rl2_get_tile_from_tiff_origin (coverage, origin, 22000, 0, -1) != NULL)
      {
	  fprintf (stderr, "Unexpected result: startRow too big\n");
	  return -6;
      }

    if (rl2_get_tile_from_tiff_origin (coverage, origin, 0, 22000, -1) != NULL)
      {
	  fprintf (stderr, "Unexpected result: startCol too big\n");
	  return -7;
      }

    if (rl2_get_tiff_origin_compression (origin, &compression) != RL2_OK)
      {
	  fprintf (stderr, "Error - TIFF compression\n");
	  return -8;
      }

    if (rl2_get_tiff_origin_size (origin, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "Error - TIFF size\n");
	  return -9;
      }
    if (rl2_is_geotiff_origin (origin, &is_geotiff) != RL2_OK)
      {
	  fprintf (stderr, "Error - IsGeoTIFF\n");
	  return -10;
      }
    if (is_geotiff != 0)
      {
	  fprintf (stderr, "Unexpected IsGeoTIFF %d\n", is_geotiff);
	  return -11;
      }
    if (rl2_is_tiff_worldfile_origin (origin, &is_tfw) != RL2_OK)
      {
	  fprintf (stderr, "Error - TIFF IsWorldFile\n");
	  return -12;
      }
    if (is_tfw != 0)
      {
	  fprintf (stderr, "Unexpected IsWorldFile %d\n", is_tfw);
	  return -13;
      }
    tiff_path = rl2_get_tiff_origin_path (origin);
    if (tiff_path == NULL)
      {
	  fprintf (stderr, "Error - NULL TIFF Path\n");
	  return -14;
      }
    if (strcmp (tiff_path, path) != 0)
      {
	  fprintf (stderr, "Error - mismatching TIFF Path \"%s\"\n", tiff_path);
	  return -15;
      }
    tfw_path = rl2_get_tiff_origin_worldfile_path (origin);
    if (tfw_path != NULL)
      {
	  fprintf (stderr, "Error - TIFF TFW Path\n");
	  return -16;
      }

    for (row = 0; row < height; row += 432)
      {
	  for (col = 0; col < width; col += 432)
	    {
		sprintf (tile_name, "%s_%04d_%04d", name, row, col);
		raster =
		    rl2_get_tile_from_tiff_origin (coverage, origin, row, col,
						   -1);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "ERROR: unable to get tile [Row=%d Col=%d] from %s\n",
			       row, col, name);
		      return -17;
		  }
		img =
		    rl2_create_section (name, RL2_COMPRESSION_NONE,
					RL2_TILESIZE_UNDEFINED,
					RL2_TILESIZE_UNDEFINED, raster);
		if (img == NULL)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "ERROR: unable to create a Section [%s]\n",
			       name);
		      return -18;
		  }
		sprintf (path, "./%s.png", tile_name);
		if (rl2_section_to_png (img, path) != RL2_OK)
		  {
		      fprintf (stderr, "Unable to write: %s\n", path);
		      return -19;
		  }
		rl2_destroy_section (img);
		unlink (path);
	    }
      }

    rl2_destroy_coverage (coverage);
    rl2_destroy_tiff_origin (origin);
    return 0;
}

static int
test_null ()
{
/* testing null/invalid arguments */
    rl2TiffOriginPtr origin;
    rl2RasterPtr raster;
    rl2CoveragePtr coverage;
    unsigned int width;
    unsigned int height;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char alias_pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned int strip_size;
    int is_tiled;
    int srid;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double hResolution;
    double vResolution;

    origin =
	rl2_create_tiff_origin (NULL, RL2_TIFF_NO_GEOREF, -1,
				RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				RL2_BANDS_UNKNOWN);
    if (origin != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create TIFF Origin - NULL path\n");
	  return -30;
      }
    origin =
	rl2_create_tiff_origin ("not_existing_tiff.tif", RL2_TIFF_NO_GEOREF, -1,
				RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				RL2_BANDS_UNKNOWN);
    if (origin != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create TIFF Origin - no georef\n");
	  return -31;
      }
    origin =
	rl2_create_tiff_origin ("not_existing_tiff.tif", RL2_TIFF_GEOTIFF, -1,
				RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				RL2_BANDS_UNKNOWN);
    if (origin != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create TIFF Origin - GeoTiff\n");
	  return -32;
      }
    origin =
	rl2_create_tiff_origin ("not_existing_tiff.tif", RL2_TIFF_WORLDFILE, -1,
				RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				RL2_BANDS_UNKNOWN);
    if (origin != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create TIFF Origin - Worldfile\n");
	  return -33;
      }
    origin =
	rl2_create_tiff_origin ("gray-striped.tif", RL2_TIFF_NO_GEOREF, -1,
				RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				RL2_BANDS_UNKNOWN);
    if (origin == NULL)
      {
	  fprintf (stderr, "Unexpected fail: Create TIFF Origin - no georef\n");
	  return -34;
      }
    if (rl2_get_tiff_origin_srid (origin, &srid) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Get SRID - no georef\n");
	  return -35;
      }
    if (rl2_get_tiff_origin_extent (origin, &minX, &minY, &maxX, &maxY) ==
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Get Extent - no georef\n");
	  return -36;
      }
    if (rl2_get_tiff_origin_resolution (origin, &hResolution, &vResolution) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Resolution - no georef\n");
	  return -37;
      }
    rl2_destroy_tiff_origin (origin);
    origin =
	rl2_create_tiff_origin ("gray-striped.tif", RL2_TIFF_WORLDFILE, -1,
				RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				RL2_BANDS_UNKNOWN);
    if (origin == NULL)
      {
	  fprintf (stderr, "Unexpected fail: Create TIFF Origin - Worldfile\n");
	  return -38;
      }
    if (rl2_get_tiff_origin_worldfile_path (origin) != NULL)
      {
	  fprintf (stderr, "Unexpected success: TFW Path (not existing)\n");
	  return -39;
      }
    if (rl2_get_tiff_origin_tile_size (origin, &tile_width, &tile_height) ==
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Tile Size\n");
	  return -40;
      }
    rl2_destroy_tiff_origin (origin);
    origin =
	rl2_create_tiff_origin ("gray-tiled.tif", RL2_TIFF_GEOTIFF, -1,
				RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				RL2_BANDS_UNKNOWN);
    if (origin == NULL)
      {
	  fprintf (stderr, "Unexpected fail: Create TIFF Origin - Worldfile\n");
	  return -41;
      }
    if (rl2_get_tiff_origin_strip_size (origin, &strip_size) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Strip Size\n");
	  return -42;
      }
    coverage =
	rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1,
			     RL2_COMPRESSION_PNG, 0, 432, 432, NULL);
    raster = rl2_get_tile_from_tiff_origin (NULL, origin, 0, 0, -1);
    if (raster != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Tile - NULL coverage\n");
	  return -43;
      }
    raster = rl2_get_tile_from_tiff_origin (coverage, NULL, 0, 0, -1);
    if (raster != NULL)
      {
	  fprintf (stderr, "Unexpected success: TIFF Get Tile - NULL origin\n");
	  return -44;
      }
    raster = rl2_get_tile_from_tiff_origin (coverage, origin, 3, 0, -1);
    if (raster != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Tile - invalid tile row\n");
	  return -45;
      }
    raster = rl2_get_tile_from_tiff_origin (coverage, origin, 0, 3, -1);
    if (raster != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Tile - invalid tile col\n");
	  return -46;
      }
    rl2_destroy_coverage (coverage);
    rl2_destroy_tiff_origin (origin);
    origin =
	rl2_create_geotiff_origin ("not_existing_tiff.tif", -1,
				   RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				   RL2_BANDS_UNKNOWN);
    if (origin != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Origin - not existing\n");
	  return -47;
      }
    origin =
	rl2_create_geotiff_origin ("gray-striped.tif", -1, RL2_SAMPLE_UNKNOWN,
				   RL2_PIXEL_UNKNOWN, RL2_BANDS_UNKNOWN);
    if (origin != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Origin - not a GeoTIFF\n");
	  return -48;
      }
    origin =
	rl2_create_tiff_worldfile_origin ("not_existing_tiff.tif", 4326,
					  RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
					  RL2_BANDS_UNKNOWN);
    if (origin != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create TIFF+TFW Origin - not existing\n");
	  return -49;
      }
    origin =
	rl2_create_tiff_worldfile_origin ("gray-striped.tif", 4326,
					  RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
					  RL2_BANDS_UNKNOWN);
    if (origin != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create TIFF+TFW Origin - missing TFW\n");
	  return -50;
      }
    if (rl2_get_tiff_origin_worldfile_path (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected success: TFW Path (NULL origin)\n");
	  return -51;
      }
    if (rl2_get_tiff_origin_path (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected success: TIFF Path (NULL origin)\n");
	  return -52;
      }
    if (rl2_get_tiff_origin_size (NULL, &width, &height) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Size (NULL origin)\n");
	  return -53;
      }
    if (rl2_get_tiff_origin_type
	(NULL, &sample_type, &pixel_type, &alias_pixel_type,
	 &num_bands) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Type (NULL origin)\n");
	  return -54;
      }
    if (rl2_get_tiff_origin_compression (NULL, &compression) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Compression (NULL origin)\n");
	  return -55;
      }
    if (rl2_is_tiled_tiff_origin (NULL, &is_tiled) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF IsTiled - NULL origin\n");
	  return -56;
      }
    if (rl2_get_tiff_origin_tile_size (NULL, &tile_width, &tile_height) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Tile Size - NULL origin\n");
	  return -57;
      }
    if (rl2_get_tiff_origin_strip_size (NULL, &strip_size) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Strip Size - NULL origin\n");
	  return -58;
      }
    if (rl2_get_tiff_origin_srid (NULL, &srid) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Get SRID - NULL origin\n");
	  return -59;
      }
    if (rl2_get_tiff_origin_extent (NULL, &minX, &minY, &maxX, &maxY) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Extent - NULL origin\n");
	  return -60;
      }
    if (rl2_get_tiff_origin_resolution (NULL, &hResolution, &vResolution) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Resolution - NULL origin\n");
	  return -61;
      }

    origin =
	rl2_create_tiff_origin ("rgb-tiled.tif", RL2_TIFF_NO_GEOREF, -1,
				RL2_SAMPLE_UNKNOWN, RL2_PIXEL_UNKNOWN,
				RL2_BANDS_UNKNOWN);
    coverage =
	rl2_create_coverage ("alpha", RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE, 1,
			     RL2_COMPRESSION_PNG, 0, 432, 432, NULL);
    if (rl2_eval_tiff_origin_compatibility (NULL, origin, -1) == RL2_TRUE)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Compatibility - NULL Coverage\n");
	  return -62;
      }
    if (rl2_eval_tiff_origin_compatibility (coverage, NULL, -1) == RL2_TRUE)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Compatibility - NULL origin\n");
	  return -63;
      }
    if (rl2_eval_tiff_origin_compatibility (coverage, origin, -1) == RL2_TRUE)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Compatibility - mismatching sample\n");
	  return -64;
      }
    rl2_destroy_coverage (coverage);
    coverage =
	rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1,
			     RL2_COMPRESSION_PNG, 0, 432, 432, NULL);
    if (rl2_eval_tiff_origin_compatibility (coverage, origin, -1) == RL2_TRUE)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Compatibility - mismatching pixel\n");
	  return -65;
      }
    rl2_destroy_coverage (coverage);
    rl2_destroy_tiff_origin (origin);

    return 0;
}

int
main (int argc, char *argv[])
{
    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (test_tiff ("gray-striped", RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1) !=
	0)
	return -1;
    if (test_tiff ("gray-tiled", RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1) != 0)
	return -2;
    if (test_tiff ("plt-striped", RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1) != 0)
	return -3;
    if (test_tiff ("plt-tiled", RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1) != 0)
	return -4;
    if (test_tiff ("rgb-striped", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3) != 0)
	return -5;
    if (test_tiff ("rgb-tiled", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3) != 0)
	return -6;
    if (test_tiff ("mono3s", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1) != 0)
	return -7;
    if (test_tiff ("mono4s", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1) != 0)
	return -8;
    if (test_tiff ("mono3t", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1) != 0)
	return -9;
    if (test_tiff ("mono4t", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1) != 0)
	return -10;
    if (test_null () != 0)
	return -11;
    if (test_mem_tiff ("gray-striped") != 0)
	return -12;
    if (test_mem_tiff ("gray-tiled") != 0)
	return -13;
    if (test_mem_tiff ("plt-striped") != 0)
	return -14;
    if (test_mem_tiff ("plt-tiled") != 0)
	return -15;
    if (test_mem_tiff ("rgb-striped") != 0)
	return -16;
    if (test_mem_tiff ("rgb-tiled") != 0)
	return -17;
    if (test_mem_tiff ("mono3s") != 0)
	return -18;
    if (test_mem_tiff ("mono4s") != 0)
	return -18;
    if (test_mem_tiff ("mono3t") != 0)
	return -20;
    if (test_mem_tiff ("mono4t") != 0)
	return -21;

    return 0;
}
