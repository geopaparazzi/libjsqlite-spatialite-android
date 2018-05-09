/*

 test_tile_callback.c -- RasterLite2 Test Case

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
#include <math.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"

struct tile_info
{
/* a struct supporting the Tile Callback function */
    unsigned char sample;
    unsigned char pixel;
    unsigned char num_bands;
    int srid;
    const char *coverage;
    unsigned int tile_w;
    unsigned int tile_h;
    rl2PalettePtr palette;
};

static void
set_tile_pixel_monochrome (unsigned char *bufpix, unsigned int x,
			   unsigned int y, unsigned int tile_w, double map_x,
			   double map_y)
{
/* pixel generator - MONOCHROME */
    unsigned char *p = bufpix + ((y * tile_w) + x);
    if ((map_x >= -60.0 && map_x <= -30.0)
	&& (map_y >= -60.0 && map_y <= -30.0))
	*p = 1;
    else if ((map_x >= 20.0 && map_x <= 80.0)
	     && (map_y >= 20.0 && map_y <= 80.0))
	*p = 1;
}

static void
set_tile_pixel_palette1 (unsigned char *bufpix, unsigned int x, unsigned int y,
			 unsigned int tile_w, double map_x, double map_y)
{
/* pixel generator - PALETTE 1-BIT */
    unsigned char *p = bufpix + ((y * tile_w) + x);
    if ((map_x >= -60.0 && map_x <= -30.0)
	&& (map_y >= -60.0 && map_y <= -30.0))
	*p = 1;
    else if ((map_x >= 20.0 && map_x <= 80.0)
	     && (map_y >= 20.0 && map_y <= 80.0))
	*p = 1;
}

static void
set_tile_pixel_1bit (unsigned char *bufpix, unsigned int x, unsigned int y,
		     unsigned int tile_w, double map_x, double map_y,
		     unsigned char pixel)
{
/* pixel generator - 1-BIT */
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  set_tile_pixel_monochrome (bufpix, x, y, tile_w, map_x, map_y);
	  break;
      case RL2_PIXEL_PALETTE:
	  set_tile_pixel_palette1 (bufpix, x, y, tile_w, map_x, map_y);
	  break;
      };
}

static void
set_tile_pixel_palette48 (unsigned char *bufpix, unsigned int x, unsigned int y,
			  unsigned int tile_w, double map_x, double map_y)
{
/* pixel generator - PALETTE UINT8 or 4-BIT */
    unsigned char *p = bufpix + ((y * tile_w) + x);
    if (map_y >= -15.0 && map_y <= 15.0)
      {
	  if (map_x < 0.0)
	      *p = 8;
	  else
	      *p = 7;
      }
    else if (map_y >= -60.0 && map_y <= 60.0)
      {
	  if (map_x < 0.0)
	      *p = 5;
	  else
	      *p = 4;
      }
    else
      {
	  if (map_x < 0.0)
	      *p = 3;
	  else
	      *p = 2;
      }
}

static void
set_tile_pixel_palette2 (unsigned char *bufpix, unsigned int x, unsigned int y,
			 unsigned int tile_w, double map_x, double map_y)
{
/* pixel generator - PALETTE 2-BIT */
    unsigned char *p = bufpix + ((y * tile_w) + x);
    if (map_y >= -15.0 && map_y <= 15.0)
      {
	  if (map_x < 0.0)
	      *p = 3;
	  else
	      *p = 2;
      }
    else if (map_y >= -60.0 && map_y <= 60.0)
      {
	  if (map_x < 0.0)
	      *p = 2;
	  else
	      *p = 1;
      }
    else
      {
	  if (map_x < 0.0)
	      *p = 1;
	  else
	      *p = 0;
      }
}

static void
set_tile_pixel_gray8 (unsigned char *bufpix, unsigned int x, unsigned int y,
		      unsigned int tile_w, double map_x, double map_y)
{
/* pixel generator - GRAYSCALE UINT8 */
    unsigned char *p = bufpix + ((y * tile_w) + x);
    if (map_y >= -15.0 && map_y <= 15.0)
      {
	  if (map_x < 0.0)
	      *p = 248;
	  else
	      *p = 216;
      }
    else if (map_y >= -60.0 && map_y <= 60.0)
      {
	  if (map_x < 0.0)
	      *p = 208;
	  else
	      *p = 192;
      }
    else
      {
	  if (map_x < 0.0)
	      *p = 128;
	  else
	      *p = 96;
      }
}

static void
set_tile_pixel_rgb8 (unsigned char *bufpix, unsigned int x, unsigned int y,
		     unsigned int tile_w, double map_x, double map_y,
		     unsigned int num_bands)
{
/* pixel generator - RGB UINT8 */
    unsigned char *p = bufpix + (y * tile_w * num_bands) + (x * num_bands);
    if (map_y >= -15.0 && map_y <= 15.0)
      {
	  if (map_x < 0.0)
	    {
		*p++ = 248;
		*p++ = 255;
		*p++ = 0;
	    }
	  else
	    {
		*p++ = 216;
		*p++ = 248;
		*p++ = 0;
	    }
      }
    else if (map_y >= -60.0 && map_y <= 60.0)
      {
	  if (map_x < 0.0)
	    {
		*p++ = 0;
		*p++ = 208;
		*p++ = 248;
	    }
	  else
	    {
		*p++ = 0;
		*p++ = 192;
		*p++ = 248;
	    }
      }
    else
      {
	  if (map_x < 0.0)
	    {
		*p++ = 192;
		*p++ = 192;
		*p++ = 128;
	    }
	  else
	    {
		*p++ = 192;
		*p++ = 192;
		*p++ = 96;
	    }
      }
}

static void
set_tile_pixel_int16 (unsigned char *bufpix, unsigned int x, unsigned int y,
		      unsigned int tile_w, double map_x, double map_y)
{
/* pixel generator - DATAGRID INT16 */
    short *p = (short *) bufpix;
    p += ((y * tile_w) + x);
    if (map_y >= -15.0 && map_y <= 15.0)
      {
	  if (map_x < 0.0)
	      *p = -10;
	  else
	      *p = 50;
      }
    else if (map_y >= -60.0 && map_y <= 60.0)
      {
	  if (map_x < 0.0)
	      *p = 100;
	  else
	      *p = 200;
      }
    else
      {
	  if (map_x < 0.0)
	      *p = -100;
	  else
	      *p = -50;
      }
}

static void
set_tile_pixel_double (unsigned char *bufpix, unsigned int x, unsigned int y,
		       unsigned int tile_w, double map_x, double map_y)
{
/* pixel generator - DATAGRID DOUBLE */
    double *p = (double *) bufpix;
    p += ((y * tile_w) + x);
    if (map_y >= -15.0 && map_y <= 15.0)
      {
	  if (map_x < 0.0)
	      *p = -10.06;
	  else
	      *p = 50.02;
      }
    else if (map_y >= -60.0 && map_y <= 60.0)
      {
	  if (map_x < 0.0)
	      *p = 100.03;
	  else
	      *p = 200.81;
      }
    else
      {
	  if (map_x < 0.0)
	      *p = -100.23;
	  else
	      *p = -50.41;
      }
}

static void
set_tile_pixel_uint8 (unsigned char *bufpix, unsigned int x, unsigned int y,
		      unsigned int tile_w, double map_x, double map_y,
		      unsigned char pixel, unsigned char num_bands)
{
/* pixel generator - UINT8 */
    switch (pixel)
      {
      case RL2_PIXEL_GRAYSCALE:
	  set_tile_pixel_gray8 (bufpix, x, y, tile_w, map_x, map_y);
	  break;
      case RL2_PIXEL_RGB:
	  set_tile_pixel_rgb8 (bufpix, x, y, tile_w, map_x, map_y, num_bands);
	  break;
      case RL2_PIXEL_PALETTE:
	  set_tile_pixel_palette48 (bufpix, x, y, tile_w, map_x, map_y);
	  break;
      };
}

static void
set_tile_pixel (unsigned char *bufpix, unsigned int x, unsigned int y,
		unsigned int tile_w, double map_x, double map_y,
		unsigned char sample, unsigned char pixel,
		unsigned int num_bands)
{
/* generalized pixel generator */
    switch (sample)
      {
      case RL2_SAMPLE_1_BIT:
	  set_tile_pixel_1bit (bufpix, x, y, tile_w, map_x, map_y, pixel);
	  break;
      case RL2_SAMPLE_2_BIT:
	  set_tile_pixel_palette2 (bufpix, x, y, tile_w, map_x, map_y);
	  break;
      case RL2_SAMPLE_4_BIT:
	  set_tile_pixel_palette48 (bufpix, x, y, tile_w, map_x, map_y);
	  break;
      case RL2_SAMPLE_UINT8:
	  set_tile_pixel_uint8 (bufpix, x, y, tile_w, map_x, map_y, pixel,
				num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  set_tile_pixel_int16 (bufpix, x, y, tile_w, map_x, map_y);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  set_tile_pixel_double (bufpix, x, y, tile_w, map_x, map_y);
	  break;
      };
}

static int
tile_callback (void *data, double tile_minx, double tile_miny, double tile_maxx,
	       double tile_maxy, unsigned char *bufpix, rl2PalettePtr * palette)
{
/* callback function initializing a Tile */
    struct tile_info *info = (struct tile_info *) data;
    unsigned int x;
    unsigned int y;
    double res_x = (tile_maxx - tile_minx) / (double) (info->tile_w);
    double res_y = (tile_maxy - tile_miny) / (double) (info->tile_h);

/* setting tile pixels */
    for (y = 0; y < info->tile_h; y++)
      {
	  double map_y = tile_maxy - ((double) y * res_y);
	  if (map_y < tile_miny)
	      continue;
	  for (x = 0; x < info->tile_w; x++)
	    {
		double map_x = tile_minx + ((double) x * res_x);
		if (map_x > tile_maxx)
		    continue;
		set_tile_pixel (bufpix, x, y, info->tile_w, map_x, map_y,
				info->sample, info->pixel, info->num_bands);
	    }
      }

    if (info->palette != NULL)
	*palette = rl2_clone_palette (info->palette);

    return 1;
}

static rl2PixelPtr
default_nodata (unsigned char sample, unsigned char pixel,
		unsigned char num_bands)
{
/* creating a default NO-DATA value */
    int nb;
    rl2PixelPtr pxl = rl2_create_pixel (sample, pixel, num_bands);
    if (pxl == NULL)
	return NULL;
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  rl2_set_pixel_sample_1bit (pxl, 0);
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (sample)
	    {
	    case RL2_SAMPLE_1_BIT:
		rl2_set_pixel_sample_1bit (pxl, 0);
		break;
	    case RL2_SAMPLE_2_BIT:
		rl2_set_pixel_sample_2bit (pxl, 0);
		break;
	    case RL2_SAMPLE_4_BIT:
		rl2_set_pixel_sample_4bit (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (sample)
	    {
	    case RL2_SAMPLE_1_BIT:
		rl2_set_pixel_sample_1bit (pxl, 1);
		break;
	    case RL2_SAMPLE_2_BIT:
		rl2_set_pixel_sample_2bit (pxl, 3);
		break;
	    case RL2_SAMPLE_4_BIT:
		rl2_set_pixel_sample_4bit (pxl, 15);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_RGB:
	  switch (sample)
	    {
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 255);
		rl2_set_pixel_sample_uint8 (pxl, 1, 255);
		rl2_set_pixel_sample_uint8 (pxl, 2, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		rl2_set_pixel_sample_uint16 (pxl, 1, 0);
		rl2_set_pixel_sample_uint16 (pxl, 2, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (sample)
	    {
	    case RL2_SAMPLE_INT8:
		rl2_set_pixel_sample_int8 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 0);
		break;
	    case RL2_SAMPLE_INT16:
		rl2_set_pixel_sample_int16 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		break;
	    case RL2_SAMPLE_INT32:
		rl2_set_pixel_sample_int32 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT32:
		rl2_set_pixel_sample_uint32 (pxl, 0);
		break;
	    case RL2_SAMPLE_FLOAT:
		rl2_set_pixel_sample_float (pxl, 0.0);
		break;
	    case RL2_SAMPLE_DOUBLE:
		rl2_set_pixel_sample_double (pxl, 0.0);
		break;
	    };
	  break;
      case RL2_PIXEL_MULTIBAND:
	  switch (sample)
	    {
	    case RL2_SAMPLE_UINT8:
		for (nb = 0; nb < num_bands; nb++)
		    rl2_set_pixel_sample_uint8 (pxl, nb, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		for (nb = 0; nb < num_bands; nb++)
		    rl2_set_pixel_sample_uint16 (pxl, nb, 0);
		break;
	    };
	  break;
      };
    return pxl;
}

static int
test_uint8_gray (sqlite3 * handle)
{
/* testing UINT8 GRAYSCALE */
    struct tile_info info;
    rl2CoveragePtr cvg;

    info.sample = RL2_SAMPLE_UINT8;
    info.pixel = RL2_PIXEL_GRAYSCALE;
    info.num_bands = 1;
    info.srid = 4326;
    info.coverage = "UINT8_GRAYSCALE";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = NULL;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_PNG, 100, info.tile_w, info.tile_h, info.srid, 0.1,
	 0.1, no_data, NULL, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, NULL, info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    return 1;
}

static int
test_uint8_palette (sqlite3 * handle)
{
/* testing UINT8 PALETTE */
    struct tile_info info;
    rl2CoveragePtr cvg;
    rl2PalettePtr palette = rl2_create_palette (9);
    rl2_set_palette_color (palette, 0, 255, 255, 255);
    rl2_set_palette_color (palette, 1, 255, 0, 0);
    rl2_set_palette_color (palette, 2, 0, 255, 0);
    rl2_set_palette_color (palette, 3, 0, 0, 255);
    rl2_set_palette_color (palette, 4, 255, 255, 0);
    rl2_set_palette_color (palette, 5, 255, 0, 255);
    rl2_set_palette_color (palette, 6, 0, 255, 255);
    rl2_set_palette_color (palette, 7, 192, 192, 192);
    rl2_set_palette_color (palette, 8, 0, 0, 0);

    info.sample = RL2_SAMPLE_UINT8;
    info.pixel = RL2_PIXEL_PALETTE;
    info.num_bands = 1;
    info.srid = 4326;
    info.coverage = "UINT8_PALETTE";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = palette;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_PNG, 100, info.tile_w, info.tile_h, info.srid, 0.1,
	 0.1, no_data, palette, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, "MAIN", info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    rl2_destroy_palette (palette);
    return 1;
}

static int
test_4bit_palette (sqlite3 * handle)
{
/* testing 4-BIT PALETTE */
    struct tile_info info;
    rl2CoveragePtr cvg;
    rl2PalettePtr palette = rl2_create_palette (9);
    rl2_set_palette_color (palette, 0, 255, 255, 255);
    rl2_set_palette_color (palette, 1, 255, 0, 0);
    rl2_set_palette_color (palette, 2, 0, 255, 0);
    rl2_set_palette_color (palette, 3, 0, 0, 255);
    rl2_set_palette_color (palette, 4, 255, 255, 0);
    rl2_set_palette_color (palette, 5, 255, 0, 255);
    rl2_set_palette_color (palette, 6, 0, 255, 255);
    rl2_set_palette_color (palette, 7, 192, 192, 192);
    rl2_set_palette_color (palette, 8, 0, 0, 0);

    info.sample = RL2_SAMPLE_4_BIT;
    info.pixel = RL2_PIXEL_PALETTE;
    info.num_bands = 1;
    info.srid = 4326;
    info.coverage = "4BIT_PALETTE";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = palette;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_PNG, 100, info.tile_w, info.tile_h, info.srid, 0.1,
	 0.1, no_data, palette, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, "main", info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    rl2_destroy_palette (palette);
    return 1;
}

static int
test_2bit_palette (sqlite3 * handle)
{
/* testing 2-BIT PALETTE */
    struct tile_info info;
    rl2CoveragePtr cvg;
    rl2PalettePtr palette = rl2_create_palette (4);
    rl2_set_palette_color (palette, 0, 255, 255, 255);
    rl2_set_palette_color (palette, 1, 255, 0, 0);
    rl2_set_palette_color (palette, 2, 0, 255, 0);
    rl2_set_palette_color (palette, 3, 0, 0, 255);

    info.sample = RL2_SAMPLE_2_BIT;
    info.pixel = RL2_PIXEL_PALETTE;
    info.num_bands = 1;
    info.srid = 4326;
    info.coverage = "2BIT_PALETTE";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = palette;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_PNG, 100, info.tile_w, info.tile_h, info.srid, 0.1,
	 0.1, no_data, palette, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, NULL, info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    rl2_destroy_palette (palette);
    return 1;
}

static int
test_1bit_palette (sqlite3 * handle)
{
/* testing 1-BIT PALETTE */
    struct tile_info info;
    rl2CoveragePtr cvg;
    rl2PalettePtr palette = rl2_create_palette (2);
    rl2_set_palette_color (palette, 0, 255, 255, 255);
    rl2_set_palette_color (palette, 1, 255, 0, 0);

    info.sample = RL2_SAMPLE_1_BIT;
    info.pixel = RL2_PIXEL_PALETTE;
    info.num_bands = 1;
    info.srid = 4326;
    info.coverage = "1BIT_PALETTE";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = palette;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_PNG, 100, info.tile_w, info.tile_h, info.srid, 0.1,
	 0.1, no_data, palette, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, NULL, info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    rl2_destroy_palette (palette);
    return 1;
}

static int
test_uint8_rgb (sqlite3 * handle)
{
/* testing UINT8 RGB */
    struct tile_info info;
    rl2CoveragePtr cvg;

    info.sample = RL2_SAMPLE_UINT8;
    info.pixel = RL2_PIXEL_RGB;
    info.num_bands = 3;
    info.srid = 4326;
    info.coverage = "UINT8_RGB";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = NULL;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_PNG, 100, info.tile_w, info.tile_h, info.srid, 0.1,
	 0.1, no_data, NULL, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, NULL, info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    return 1;
}

static int
test_int16_grid (sqlite3 * handle)
{
/* testing INT8 DATAGRID */
    struct tile_info info;
    rl2CoveragePtr cvg;

    info.sample = RL2_SAMPLE_INT16;
    info.pixel = RL2_PIXEL_DATAGRID;
    info.num_bands = 1;
    info.srid = 4326;
    info.coverage = "INT16_GRID";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = NULL;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_DEFLATE, 100, info.tile_w, info.tile_h, info.srid, 0.1,
	 0.1, no_data, NULL, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, NULL, info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    return 1;
}

static int
test_double_grid (sqlite3 * handle)
{
/* testing DOUBLE DATAGRID */
    struct tile_info info;
    rl2CoveragePtr cvg;

    info.sample = RL2_SAMPLE_DOUBLE;
    info.pixel = RL2_PIXEL_DATAGRID;
    info.num_bands = 1;
    info.srid = 4326;
    info.coverage = "DOUBLE_GRID";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = NULL;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_DEFLATE, 100, info.tile_w, info.tile_h, info.srid, 0.1,
	 0.1, no_data, NULL, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, NULL, info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    return 1;
}

static int
test_monochrome (sqlite3 * handle)
{
/* testing MONOCHROME */
    struct tile_info info;
    rl2CoveragePtr cvg;

    info.sample = RL2_SAMPLE_1_BIT;
    info.pixel = RL2_PIXEL_MONOCHROME;
    info.num_bands = 1;
    info.srid = 4326;
    info.coverage = "MONOCHROME";
    info.tile_w = 512;
    info.tile_h = 512;
    info.palette = NULL;

    rl2PixelPtr no_data =
	default_nodata (info.sample, info.pixel, info.num_bands);
    if (rl2_create_dbms_coverage
	(handle, info.coverage, info.sample, info.pixel, info.num_bands,
	 RL2_COMPRESSION_CCITTFAX4, 100, info.tile_w, info.tile_h, info.srid,
	 0.1, 0.1, no_data, NULL, 1, 0, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to create Coverage \"%s\"\n", info.coverage);
	  return 0;
      }

    cvg = rl2_create_coverage_from_dbms (handle, NULL, info.coverage);
    if (cvg == NULL)
      {
	  rl2_destroy_coverage (cvg);
	  return 0;
      }

    if (rl2_load_raw_tiles_into_dbms
	(handle, cvg, "Alpha", 3600, 1800, info.srid, -180, -90, 180, 90,
	 tile_callback, &info, 1) != RL2_OK)
      {
	  fprintf (stderr, "Unable to populate Tiles on Coverage \"%s\"\n",
		   info.coverage);
	  return 0;
      }

    rl2_destroy_coverage (cvg);
    rl2_destroy_pixel (no_data);
    return 1;
}

int
main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle = NULL;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */
    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory db: %s\n",
		   sqlite3_errmsg (handle));
	  return -1;
      }
    spatialite_init_ex (handle, cache, 0);
    rl2_init (handle, priv_data, 0);

/* the complete test is handled as an unique SQL Transaction */
    ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "BEGIN TRANSACTION error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -2;
      }

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata()", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -3;
      }
    ret =
	sqlite3_exec (handle, "SELECT CreateRasterCoveragesTable()", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoveragesTable() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -4;
      }

    if (!test_uint8_gray (handle))
	return -5;

    if (!test_uint8_rgb (handle))
	return -6;

    if (!test_int16_grid (handle))
	return -7;

    if (!test_double_grid (handle))
	return -8;

    if (!test_monochrome (handle))
	return -9;

    if (!test_uint8_palette (handle))
	return -10;

    if (!test_4bit_palette (handle))
	return -11;

    if (!test_2bit_palette (handle))
	return -12;

    if (!test_1bit_palette (handle))
	return -13;

/* committing the SQL Transaction */
    ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "COMMIT TRANSACTION error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -14;
      }

    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
    spatialite_shutdown ();
    return 0;
}
