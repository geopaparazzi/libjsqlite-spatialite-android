/*

 rl2import -- DBMS import functions

 version 0.1, 2014 January 9

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
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
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
the Initial Developer. All Rights Reserved.

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
#include <string.h>
#include <float.h>

#include <sys/types.h>
#if defined(_WIN32) && !defined(__MINGW32__)
#include <io.h>
#include <direct.h>
#else
#include <dirent.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

#include <spatialite/gaiaaux.h>

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%I64d\n"
#else
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%lld\n"
#endif

static char *
formatFloat (double value)
{
/* nicely formatting a float value */
    int i;
    int len;
    char *fmt = sqlite3_mprintf ("%1.24f", value);
    len = strlen (fmt);
    for (i = len - 1; i >= 0; i--)
      {
	  if (fmt[i] == '0')
	      fmt[i] = '\0';
	  else
	      break;
      }
    len = strlen (fmt);
    if (fmt[len - 1] == '.')
	fmt[len] = '0';
    return fmt;
}

static char *
formatFloat2 (double value)
{
/* nicely formatting a float value (2 decimals) */
    char *fmt = sqlite3_mprintf ("%1.2f", value);
    return fmt;
}

static char *
formatFloat6 (double value)
{
/* nicely formatting a float value (6 decimals) */
    char *fmt = sqlite3_mprintf ("%1.2f", value);
    return fmt;
}

static char *
formatLong (double value)
{
/* nicely formatting a Longitude */
    if (value >= -180.0 && value <= 180.0)
	return formatFloat6 (value);
    return formatFloat2 (value);
}

static char *
formatLat (double value)
{
/* nicely formatting a Latitude */
    if (value >= -90.0 && value <= 90.0)
	return formatFloat6 (value);
    return formatFloat2 (value);
}

static int
do_insert_pyramid_levels (sqlite3 * handle, int id_level, double res_x,
			  double res_y, sqlite3_stmt * stmt_levl)
{
/* INSERTing the Pyramid levels */
    int ret;
    sqlite3_reset (stmt_levl);
    sqlite3_clear_bindings (stmt_levl);
    sqlite3_bind_int (stmt_levl, 1, id_level);
    sqlite3_bind_double (stmt_levl, 2, res_x);
    sqlite3_bind_double (stmt_levl, 3, res_y);
    sqlite3_bind_double (stmt_levl, 4, res_x * 2.0);
    sqlite3_bind_double (stmt_levl, 5, res_y * 2.0);
    sqlite3_bind_double (stmt_levl, 6, res_x * 4.0);
    sqlite3_bind_double (stmt_levl, 7, res_y * 4.0);
    sqlite3_bind_double (stmt_levl, 8, res_x * 8.0);
    sqlite3_bind_double (stmt_levl, 9, res_y * 8.0);
    ret = sqlite3_step (stmt_levl);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO levels; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    return 1;
  error:
    return 0;
}

static int
do_insert_tile (sqlite3 * handle, unsigned char *blob_odd, int blob_odd_sz,
		unsigned char *blob_even, int blob_even_sz,
		sqlite3_int64 section_id, int srid, double res_x, double res_y,
		unsigned short tile_w, unsigned short tile_h, double miny,
		double maxx, double *tile_minx, double *tile_miny,
		double *tile_maxx, double *tile_maxy, rl2PalettePtr aux_palette,
		rl2PixelPtr no_data, sqlite3_stmt * stmt_tils,
		sqlite3_stmt * stmt_data, rl2RasterStatisticsPtr section_stats)
{
/* INSERTing the tile */
    int ret;
    sqlite3_int64 tile_id;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;
    rl2RasterStatisticsPtr stats = NULL;

    stats = rl2_get_raster_statistics
	(blob_odd, blob_odd_sz, blob_even, blob_even_sz, aux_palette, no_data);
    if (stats == NULL)
	goto error;
    rl2_aggregate_raster_statistics (stats, section_stats);
    sqlite3_reset (stmt_tils);
    sqlite3_clear_bindings (stmt_tils);
    sqlite3_bind_int64 (stmt_tils, 1, section_id);
    *tile_maxx = *tile_minx + ((double) tile_w * res_x);
    if (*tile_maxx > maxx)
	*tile_maxx = maxx;
    *tile_miny = *tile_maxy - ((double) tile_h * res_y);
    if (*tile_miny < miny)
	*tile_miny = miny;
    geom = build_extent (srid, *tile_minx, *tile_miny, *tile_maxx, *tile_maxy);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    gaiaFreeGeomColl (geom);
    sqlite3_bind_blob (stmt_tils, 2, blob, blob_size, free);
    ret = sqlite3_step (stmt_tils);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO tiles; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    tile_id = sqlite3_last_insert_rowid (handle);
    /* INSERTing tile data */
    sqlite3_reset (stmt_data);
    sqlite3_clear_bindings (stmt_data);
    sqlite3_bind_int64 (stmt_data, 1, tile_id);
    sqlite3_bind_blob (stmt_data, 2, blob_odd, blob_odd_sz, free);
    if (blob_even == NULL)
	sqlite3_bind_null (stmt_data, 3);
    else
	sqlite3_bind_blob (stmt_data, 3, blob_even, blob_even_sz, free);
    ret = sqlite3_step (stmt_data);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO tile_data; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    rl2_destroy_raster_statistics (stats);
    return 1;
  error:
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    return 0;
}

static int
do_insert_pyramid_tile (sqlite3 * handle, unsigned char *blob_odd,
			int blob_odd_sz, unsigned char *blob_even,
			int blob_even_sz, int id_level,
			sqlite3_int64 section_id, int srid, double minx,
			double miny, double maxx, double maxy,
			sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_data)
{
/* INSERTing a Pyramid tile */
    int ret;
    sqlite3_int64 tile_id;
    unsigned char *blob;
    int blob_size;
    gaiaGeomCollPtr geom;

    sqlite3_reset (stmt_tils);
    sqlite3_clear_bindings (stmt_tils);
    sqlite3_bind_int (stmt_tils, 1, id_level);
    sqlite3_bind_int64 (stmt_tils, 2, section_id);
    geom = build_extent (srid, minx, miny, maxx, maxy);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    gaiaFreeGeomColl (geom);
    sqlite3_bind_blob (stmt_tils, 3, blob, blob_size, free);
    ret = sqlite3_step (stmt_tils);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO tiles; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    tile_id = sqlite3_last_insert_rowid (handle);
    /* INSERTing tile data */
    sqlite3_reset (stmt_data);
    sqlite3_clear_bindings (stmt_data);
    sqlite3_bind_int64 (stmt_data, 1, tile_id);
    sqlite3_bind_blob (stmt_data, 2, blob_odd, blob_odd_sz, free);
    if (blob_even == NULL)
	sqlite3_bind_null (stmt_data, 3);
    else
	sqlite3_bind_blob (stmt_data, 3, blob_even, blob_even_sz, free);
    ret = sqlite3_step (stmt_data);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO tile_data; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    return 1;
  error:
    return 0;
}

RL2_PRIVATE void
compute_aggregate_sq_diff (rl2RasterStatisticsPtr section_stats)
{
/* updating aggregate sum_sq_diff */
    int ib;
    rl2PoolVariancePtr pV;
    rl2PrivBandStatisticsPtr st_band;
    rl2PrivRasterStatisticsPtr st = (rl2PrivRasterStatisticsPtr) section_stats;
    if (st == NULL)
	return;

    for (ib = 0; ib < st->nBands; ib++)
      {
	  double sum_var = 0.0;
	  st_band = st->band_stats + ib;
	  pV = st_band->first;
	  while (pV != NULL)
	    {
		sum_var += (pV->count - 1.0) * pV->variance;
		pV = pV->next;
	    }
	  st_band->sum_sq_diff = sum_var;
      }
}

static int
do_import_ascii_grid (sqlite3 * handle, const char *src_path,
		      rl2CoveragePtr cvg, const char *section, int srid,
		      unsigned short tile_w, unsigned short tile_h,
		      int pyramidize, unsigned char sample_type,
		      unsigned char compression, sqlite3_stmt * stmt_data,
		      sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_sect,
		      sqlite3_stmt * stmt_levl, sqlite3_stmt * stmt_upd_sect)
{
/* importing an ASCII Data Grid file */
    int ret;
    rl2AsciiGridOriginPtr origin = NULL;
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    int row;
    int col;
    unsigned short width;
    unsigned short height;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    double tile_minx;
    double tile_miny;
    double tile_maxx;
    double tile_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double res_x;
    double res_y;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;

    origin = rl2_create_ascii_grid_origin (src_path, srid, sample_type);
    if (origin == NULL)
	goto error;

    printf ("Importing: %s\n", rl2_get_ascii_grid_origin_path (origin));
    printf ("------------------\n");
    ret = rl2_get_ascii_grid_origin_size (origin, &width, &height);
    if (ret == RL2_OK)
	printf ("    Image Size (pixels): %d x %d\n", width, height);
    ret = rl2_get_ascii_grid_origin_srid (origin, &srid);
    if (ret == RL2_OK)
	printf ("                   SRID: %d\n", srid);
    ret = rl2_get_ascii_grid_origin_extent (origin, &minx, &miny, &maxx, &maxy);
    if (ret == RL2_OK)
      {
	  dumb1 = formatLong (minx);
	  dumb2 = formatLat (miny);
	  printf ("       LowerLeft Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
	  dumb1 = formatLong (maxx);
	  dumb2 = formatLat (maxy);
	  printf ("      UpperRight Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }
    ret = rl2_get_ascii_grid_origin_resolution (origin, &res_x, &res_y);
    if (ret == RL2_OK)
      {
	  dumb1 = formatFloat (res_x);
	  dumb2 = formatFloat (res_y);
	  printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }

    if (rl2_eval_ascii_grid_origin_compatibility (cvg, origin) != RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/ASCII mismatch\n");
	  goto error;
      }
    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx, maxy,
	 stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, 1);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (!do_insert_levels (handle, sample_type, res_x, res_y, stmt_levl))
	goto error;

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster =
		    rl2_get_tile_from_ascii_grid_origin (cvg, origin, row, col);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "ERROR: unable to get a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }
		if (rl2_raster_encode
		    (raster, compression, &blob_odd, &blob_odd_sz, &blob_even,
		     &blob_even_sz, 100, 1) != RL2_OK)
		  {
		      fprintf (stderr,
			       "ERROR: unable to encode a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }

		/* INSERTing the tile */
		if (!do_insert_tile
		    (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz,
		     section_id, srid, res_x, res_y, tile_w, tile_h, miny,
		     maxx, &tile_minx, &tile_miny, &tile_maxx, &tile_maxy,
		     NULL, no_data, stmt_tils, stmt_data, section_stats))
		    goto error;
		blob_odd = NULL;
		blob_even = NULL;
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (!do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_ascii_grid_origin (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  const char *section_name = section;
	  char *sect_name = NULL;
	  if (coverage_name == NULL)
	      goto error;
	  if (section_name == NULL)
	    {
		sect_name = get_section_name (src_path);
		section_name = sect_name;
	    }
	  if (section_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid (handle, coverage_name, section_name, 1)
	      != RL2_OK)
	    {
		if (sect_name != NULL)
		    free (sect_name);
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
	  if (sect_name != NULL)
	      free (sect_name);
      }

    return 1;

  error:
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    if (origin != NULL)
	rl2_destroy_ascii_grid_origin (origin);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

static int
check_jpeg_origin_compatibility (rl2RasterPtr raster, rl2CoveragePtr coverage,
				 unsigned short *width, unsigned short *height,
				 unsigned char *forced_conversion)
{
/* checking if the JPEG and the Coverage are mutually compatible */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    if (rst == NULL || cvg == NULL)
	return 0;
    if (rst->sampleType == RL2_SAMPLE_UINT8
	&& rst->pixelType == RL2_PIXEL_GRAYSCALE && rst->nBands == 1)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_GRAYSCALE_TO_RGB;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT8 && rst->pixelType == RL2_PIXEL_RGB
	&& rst->nBands == 3)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_RGB_TO_GRAYSCALE;
		return 1;
	    }
      }
    return 0;
}

static char *
build_worldfile_path (const char *path, const char *suffix)
{
/* building the JGW path (WorldFile) */
    char *wf_path;
    const char *x = NULL;
    const char *p = path;
    int len;

    if (path == NULL || suffix == NULL)
	return NULL;
    len = strlen (path);
    len -= 1;
    while (*p != '\0')
      {
	  if (*p == '.')
	      x = p;
	  p++;
      }
    if (x > path)
	len = x - path;
    wf_path = malloc (len + strlen (suffix) + 1);
    memcpy (wf_path, path, len);
    strcpy (wf_path + len, suffix);
    return wf_path;
}

static int
read_jgw_worldfile (const char *src_path, double *minx, double *maxy,
		    double *pres_x, double *pres_y)
{
/* attempting to retrieve georeferencing from a JPEG+JGW origin */
    FILE *jgw = NULL;
    double res_x;
    double res_y;
    double x;
    double y;
    char *jgw_path = NULL;

    jgw_path = build_worldfile_path (src_path, ".jgw");
    if (jgw_path == NULL)
	goto error;
    jgw = fopen (jgw_path, "r");
    free (jgw_path);
    jgw_path = NULL;
    if (jgw == NULL)
	goto error;
    if (!parse_worldfile (jgw, &x, &y, &res_x, &res_y))
	goto error;
    fclose (jgw);
    *pres_x = res_x;
    *pres_y = res_y;
    *minx = x;
    *maxy = y;
    return 1;

  error:
    if (jgw_path != NULL)
	free (jgw_path);
    if (jgw != NULL)
	fclose (jgw);
    return 0;
}

static void
write_jgw_worldfile (const char *path, double minx, double maxy, double x_res,
		     double y_res)
{
/* exporting a JGW WorldFile */
    FILE *jgw = NULL;
    char *jgw_path = NULL;

    jgw_path = build_worldfile_path (path, ".jgw");
    if (jgw_path == NULL)
	goto error;
    jgw = fopen (jgw_path, "w");
    free (jgw_path);
    jgw_path = NULL;
    if (jgw == NULL)
	goto error;
    fprintf (jgw, "        %1.16f\n", x_res);
    fprintf (jgw, "        0.0\n");
    fprintf (jgw, "        0.0\n");
    fprintf (jgw, "        -%1.16f\n", y_res);
    fprintf (jgw, "        %1.16f\n", minx);
    fprintf (jgw, "        %1.16f\n", maxy);
    fclose (jgw);
    return;

  error:
    if (jgw_path != NULL)
	free (jgw_path);
    if (jgw != NULL)
	fclose (jgw);
}

static int
do_import_jpeg_image (sqlite3 * handle, const char *src_path,
		      rl2CoveragePtr cvg, const char *section, int srid,
		      unsigned short tile_w, unsigned short tile_h,
		      int pyramidize, unsigned char sample_type,
		      unsigned char num_bands, unsigned char compression,
		      sqlite3_stmt * stmt_data, sqlite3_stmt * stmt_tils,
		      sqlite3_stmt * stmt_sect, sqlite3_stmt * stmt_levl,
		      sqlite3_stmt * stmt_upd_sect)
{
/* importing a JPEG image file [with optional WorldFile */
    rl2SectionPtr origin;
    rl2RasterPtr rst_in;
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    int row;
    int col;
    unsigned short width;
    unsigned short height;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    double tile_minx;
    double tile_miny;
    double tile_maxx;
    double tile_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double res_x;
    double res_y;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;
    unsigned char forced_conversion = RL2_CONVERT_NO;

    origin = rl2_section_from_jpeg (src_path);
    if (origin == NULL)
	goto error;
    rst_in = rl2_get_section_raster (origin);
    if (!check_jpeg_origin_compatibility
	(rst_in, cvg, &width, &height, &forced_conversion))
	goto error;
    if (read_jgw_worldfile (src_path, &minx, &maxy, &res_x, &res_y))
      {
	  /* georeferenced JPEG */
	  maxx = minx + ((double) width * res_x);
	  miny = maxy - ((double) height * res_y);
      }
    else
      {
	  /* not georeferenced JPEG */
	  if (srid != -1)
	      goto error;
	  minx = 0.0;
	  miny = 0.0;
	  maxx = width - 1.0;
	  maxy = height - 1.0;
	  res_x = 1.0;
	  res_y = 1.0;
      }

    printf ("Importing: %s\n", src_path);
    printf ("------------------\n");
    printf ("    Image Size (pixels): %d x %d\n", width, height);
    printf ("                   SRID: %d\n", srid);
    dumb1 = formatLong (minx);
    dumb2 = formatLat (miny);
    printf ("       LowerLeft Corner: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    dumb1 = formatLong (maxx);
    dumb2 = formatLat (maxy);
    printf ("      UpperRight Corner: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    dumb1 = formatFloat (res_x);
    dumb2 = formatFloat (res_y);
    printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);

    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx, maxy,
	 stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (!do_insert_levels (handle, sample_type, res_x, res_y, stmt_levl))
	goto error;

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster =
		    rl2_get_tile_from_jpeg_origin (cvg, rst_in, row, col,
						   forced_conversion);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "ERROR: unable to get a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }
		if (rl2_raster_encode
		    (raster, compression, &blob_odd, &blob_odd_sz, &blob_even,
		     &blob_even_sz, 100, 1) != RL2_OK)
		  {
		      fprintf (stderr,
			       "ERROR: unable to encode a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }

		/* INSERTing the tile */
		if (!do_insert_tile
		    (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz,
		     section_id, srid, res_x, res_y, tile_w, tile_h, miny,
		     maxx, &tile_minx, &tile_miny, &tile_maxx, &tile_maxy,
		     NULL, no_data, stmt_tils, stmt_data, section_stats))
		    goto error;
		blob_odd = NULL;
		blob_even = NULL;
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (!do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_section (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  const char *section_name = section;
	  char *sect_name = NULL;
	  if (coverage_name == NULL)
	      goto error;
	  if (section_name == NULL)
	    {
		sect_name = get_section_name (src_path);
		section_name = sect_name;
	    }
	  if (section_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid (handle, coverage_name, section_name, 1)
	      != RL2_OK)
	    {
		if (sect_name != NULL)
		    free (sect_name);
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
	  if (sect_name != NULL)
	      free (sect_name);
      }

    return 1;

  error:
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    if (origin != NULL)
	rl2_destroy_section (origin);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

static int
is_ascii_grid (const char *path)
{
/* testing for an ASCII Grid */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".asc") == 0)
	      return 1;
      }
    return 0;
}

static int
is_jpeg_image (const char *path)
{
/* testing for a JPEG image */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".jpg") == 0)
	      return 1;
      }
    return 0;
}

static int
do_import_file (sqlite3 * handle, const char *src_path,
		rl2CoveragePtr cvg, const char *section, int worldfile,
		int force_srid, int pyramidize, unsigned char sample_type,
		unsigned char pixel_type, unsigned char num_bands,
		unsigned short tile_w, unsigned short tile_h,
		unsigned char compression, int quality,
		sqlite3_stmt * stmt_data, sqlite3_stmt * stmt_tils,
		sqlite3_stmt * stmt_sect, sqlite3_stmt * stmt_levl,
		sqlite3_stmt * stmt_upd_sect)
{
/* importing a single Source file */
    int ret;
    rl2TiffOriginPtr origin = NULL;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr aux_palette = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    int row;
    int col;
    unsigned short width;
    unsigned short height;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    int srid;
    int xsrid;
    double tile_minx;
    double tile_miny;
    double tile_maxx;
    double tile_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double res_x;
    double res_y;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;

    if (is_ascii_grid (src_path))
	return do_import_ascii_grid (handle, src_path, cvg, section, force_srid,
				     tile_w, tile_h, pyramidize, sample_type,
				     compression, stmt_data, stmt_tils,
				     stmt_sect, stmt_levl, stmt_upd_sect);

    if (is_jpeg_image (src_path))
	return do_import_jpeg_image (handle, src_path, cvg, section, force_srid,
				     tile_w, tile_h, pyramidize, sample_type,
				     num_bands, compression, stmt_data,
				     stmt_tils, stmt_sect, stmt_levl,
				     stmt_upd_sect);

    if (worldfile)
	origin =
	    rl2_create_tiff_origin (src_path, RL2_TIFF_WORLDFILE, force_srid,
				    sample_type, pixel_type, num_bands);
    else
	origin =
	    rl2_create_tiff_origin (src_path, RL2_TIFF_GEOTIFF, force_srid,
				    sample_type, pixel_type, num_bands);
    if (origin == NULL)
	goto error;
    if (rl2_get_coverage_srid (cvg, &xsrid) == RL2_OK)
      {
	  if (xsrid == RL2_GEOREFERENCING_NONE)
	      rl2_set_tiff_origin_not_referenced (origin);
      }

    printf ("Importing: %s\n", rl2_get_tiff_origin_path (origin));
    printf ("------------------\n");
    ret = rl2_get_tiff_origin_size (origin, &width, &height);
    if (ret == RL2_OK)
	printf ("    Image Size (pixels): %d x %d\n", width, height);
    ret = rl2_get_tiff_origin_srid (origin, &srid);
    if (ret == RL2_OK)
      {
	  if (force_srid > 0 && force_srid != srid)
	    {
		printf ("                   SRID: %d (forced to %d)\n", srid,
			force_srid);
		srid = force_srid;
	    }
	  else
	      printf ("                   SRID: %d\n", srid);
      }
    ret = rl2_get_tiff_origin_extent (origin, &minx, &miny, &maxx, &maxy);
    if (ret == RL2_OK)
      {
	  dumb1 = formatLong (minx);
	  dumb2 = formatLat (miny);
	  printf ("       LowerLeft Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
	  dumb1 = formatLong (maxx);
	  dumb2 = formatLat (maxy);
	  printf ("      UpperRight Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }
    ret = rl2_get_tiff_origin_resolution (origin, &res_x, &res_y);
    if (ret == RL2_OK)
      {
	  dumb1 = formatFloat (res_x);
	  dumb2 = formatFloat (res_y);
	  printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }

    if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  /* remapping the Palette */
	  if (rl2_check_dbms_palette (handle, cvg, origin) != RL2_OK)
	    {
		fprintf (stderr, "Mismatching Palette !!!\n");
		goto error;
	    }
      }

    if (rl2_eval_tiff_origin_compatibility (cvg, origin, force_srid) !=
	RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/TIFF mismatch\n");
	  goto error;
      }
    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx, maxy,
	 stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (!do_insert_levels (handle, sample_type, res_x, res_y, stmt_levl))
	goto error;

    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		raster =
		    rl2_get_tile_from_tiff_origin (cvg, origin, row, col, srid);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "ERROR: unable to get a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }
		if (rl2_raster_encode
		    (raster, compression, &blob_odd, &blob_odd_sz, &blob_even,
		     &blob_even_sz, quality, 1) != RL2_OK)
		  {
		      fprintf (stderr,
			       "ERROR: unable to encode a tile [Row=%d Col=%d]\n",
			       row, col);
		      goto error;
		  }
		/* INSERTing the tile */
		aux_palette =
		    rl2_clone_palette (rl2_get_raster_palette (raster));

		if (!do_insert_tile
		    (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz,
		     section_id, srid, res_x, res_y, tile_w, tile_h, miny,
		     maxx, &tile_minx, &tile_miny, &tile_maxx, &tile_maxy,
		     aux_palette, no_data, stmt_tils, stmt_data, section_stats))
		    goto error;
		blob_odd = NULL;
		blob_even = NULL;
		rl2_destroy_raster (raster);
		raster = NULL;
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (!do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_tiff_origin (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  const char *section_name = section;
	  char *sect_name = NULL;
	  if (coverage_name == NULL)
	      goto error;
	  if (section_name == NULL)
	    {
		sect_name = get_section_name (src_path);
		section_name = sect_name;
	    }
	  if (section_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid (handle, coverage_name, section_name, 1)
	      != RL2_OK)
	    {
		if (sect_name != NULL)
		    free (sect_name);
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
	  if (sect_name != NULL)
	      free (sect_name);
      }

    return 1;

  error:
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    if (origin != NULL)
	rl2_destroy_tiff_origin (origin);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

static int
check_extension_match (const char *file_name, const char *file_ext)
{
/* checks the file extension */
    const char *mark = NULL;
    const char *p = file_name;
    while (*p != '\0')
      {
	  if (*p == '.')
	      mark = p;
	  p++;
      }
    if (mark == NULL)
	return 0;
    if (strcasecmp (mark, file_ext) == 0)
	return 1;
    return 0;
}

static int
do_import_dir (sqlite3 * handle, const char *dir_path, const char *file_ext,
	       rl2CoveragePtr cvg, const char *section, int worldfile,
	       int force_srid, int pyramidize, unsigned char sample_type,
	       unsigned char pixel_type, unsigned char num_bands,
	       unsigned short tile_w, unsigned short tile_h,
	       unsigned char compression, int quality, sqlite3_stmt * stmt_data,
	       sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_sect,
	       sqlite3_stmt * stmt_levl, sqlite3_stmt * stmt_upd_sect)
{
/* importing a whole directory */
#if defined(_WIN32) && !defined(__MINGW32__)
/* Visual Studio .NET */
    struct _finddata_t c_file;
    intptr_t hFile;
    int cnt = 0;
    char *search;
    char *path;
    int ret;
    if (_chdir (dir_path) < 0)
	return 0;
    search = sqlite3_mprintf ("*%s", file_ext);
    if ((hFile = _findfirst (search, &c_file)) == -1L)
	;
    else
      {
	  while (1)
	    {
		if ((c_file.attrib & _A_RDONLY) == _A_RDONLY
		    || (c_file.attrib & _A_NORMAL) == _A_NORMAL)
		  {
		      path = sqlite3_mprintf ("%s/%s", dir_path, c_file.name);
		      ret =
			  do_import_file (handle, path, cvg, section, worldfile,
					  force_srid, pyramidize, sample_type,
					  pixel_type, num_bands, tile_w, tile_h,
					  compression, quality, stmt_data,
					  stmt_tils, stmt_sect, stmt_levl,
					  stmt_upd_sect);
		      sqlite3_free (path);
		      if (!ret)
			  goto error;
		      cnt++;
		  }
		if (_findnext (hFile, &c_file) != 0)
		    break;
	    };
	error:
	  _findclose (hFile);
      }
    sqlite3_free (search);
    return cnt;
#else
/* not Visual Studio .NET */
    int cnt = 0;
    char *path;
    struct dirent *entry;
    int ret;
    DIR *dir = opendir (dir_path);
    if (!dir)
	return 0;
    while (1)
      {
	  /* scanning dir-entries */
	  entry = readdir (dir);
	  if (!entry)
	      break;
	  if (!check_extension_match (entry->d_name, file_ext))
	      continue;
	  path = sqlite3_mprintf ("%s/%s", dir_path, entry->d_name);
	  ret =
	      do_import_file (handle, path, cvg, section, worldfile, force_srid,
			      pyramidize, sample_type, pixel_type, num_bands,
			      tile_w, tile_h, compression, quality, stmt_data,
			      stmt_tils, stmt_sect, stmt_levl, stmt_upd_sect);
	  sqlite3_free (path);
	  if (!ret)
	      goto error;
	  cnt++;
      }
  error:
    closedir (dir);
    return cnt;
#endif
}

static int
do_import_common (sqlite3 * handle, const char *src_path, const char *dir_path,
		  const char *file_ext, rl2CoveragePtr cvg, const char *section,
		  int worldfile, int force_srid, int pyramidize)
{
/* main IMPORT Raster function */
    int ret;
    char *sql;
    const char *coverage;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned short tile_w;
    unsigned short tile_h;
    unsigned char compression;
    int quality;
    char *table;
    char *xtable;
    unsigned short tileWidth;
    unsigned short tileHeight;
    sqlite3_stmt *stmt_data = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_sect = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_upd_sect = NULL;

    if (cvg == NULL)
	goto error;

    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;

    tile_w = tileWidth;
    tile_h = tileHeight;
    rl2_get_coverage_compression (cvg, &compression, &quality);
    rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands);
    coverage = rl2_get_coverage_name (cvg);

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (section_id, section_name, file_path, "
	 "width, height, geometry) VALUES (NULL, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO sections SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET statistics = ? WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_upd_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE sections SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_levels", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT OR IGNORE INTO \"%s\" (pyramid_level, "
	 "x_resolution_1_1, y_resolution_1_1, "
	 "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	 "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	 "VALUES (0, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO levels SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, 0, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tils, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, tile_data_odd, tile_data_even) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tile_data SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    if (dir_path == NULL)
      {
	  /* importing a single Image file */
	  if (!do_import_file
	      (handle, src_path, cvg, section, worldfile, force_srid,
	       pyramidize, sample_type, pixel_type, num_bands, tile_w, tile_h,
	       compression, quality, stmt_data, stmt_tils, stmt_sect,
	       stmt_levl, stmt_upd_sect))
	      goto error;
      }
    else
      {
	  /* importing all Image files from a whole directory */
	  if (!do_import_dir
	      (handle, dir_path, file_ext, cvg, section, worldfile, force_srid,
	       pyramidize, sample_type, pixel_type, num_bands, tile_w, tile_h,
	       compression, quality, stmt_data, stmt_tils, stmt_sect,
	       stmt_levl, stmt_upd_sect))
	      goto error;
      }

    sqlite3_finalize (stmt_upd_sect);
    sqlite3_finalize (stmt_sect);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    stmt_upd_sect = NULL;
    stmt_sect = NULL;
    stmt_levl = NULL;
    stmt_tils = NULL;
    stmt_data = NULL;

    if (rl2_update_dbms_coverage (handle, coverage) != RL2_OK)
      {
	  fprintf (stderr, "unable to update the Coverage\n");
	  goto error;
      }

    return 1;

  error:
    if (stmt_upd_sect != NULL)
	sqlite3_finalize (stmt_upd_sect);
    if (stmt_sect != NULL)
	sqlite3_finalize (stmt_sect);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    return 0;
}

RL2_DECLARE int
rl2_load_raster_into_dbms (sqlite3 * handle, const char *src_path,
			   rl2CoveragePtr coverage, int worldfile,
			   int force_srid, int pyramidize)
{
/* importing a single Raster file */
    if (!do_import_common
	(handle, src_path, NULL, NULL, coverage, NULL, worldfile, force_srid,
	 pyramidize))
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_load_mrasters_into_dbms (sqlite3 * handle, const char *dir_path,
			     const char *file_ext,
			     rl2CoveragePtr coverage, int worldfile,
			     int force_srid, int pyramidize)
{
/* importing multiple Raster files from dir */
    if (!do_import_common
	(handle, NULL, dir_path, file_ext, coverage, NULL, worldfile,
	 force_srid, pyramidize))
	return RL2_ERROR;
    return RL2_OK;
}

static void
copy_int8_outbuf_to_tile (const char *outbuf, char *tile,
			  unsigned short width,
			  unsigned short height,
			  unsigned short tile_width,
			  unsigned short tile_height, int base_y, int base_x)
{
/* copying INT8 pixels from the output buffer into the tile */
    int x;
    int y;
    const char *p_in;
    char *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_uint8_outbuf_to_tile (const unsigned char *outbuf, unsigned char *tile,
			   unsigned char num_bands, unsigned short width,
			   unsigned short height,
			   unsigned short tile_width,
			   unsigned short tile_height, int base_y, int base_x)
{
/* copying UINT8 pixels from the output buffer into the tile */
    int x;
    int y;
    int b;
    const unsigned char *p_in;
    unsigned char *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in =
	      outbuf + ((base_y + y) * width * num_bands) +
	      (base_x * num_bands);
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out += num_bands;
		      p_in += num_bands;
		      continue;
		  }
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_int16_outbuf_to_tile (const short *outbuf, short *tile,
			   unsigned short width,
			   unsigned short height,
			   unsigned short tile_width,
			   unsigned short tile_height, int base_y, int base_x)
{
/* copying INT16 pixels from the output buffer into the tile */
    int x;
    int y;
    const short *p_in;
    short *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_uint16_outbuf_to_tile (const unsigned short *outbuf, unsigned short *tile,
			    unsigned char num_bands, unsigned short width,
			    unsigned short height,
			    unsigned short tile_width,
			    unsigned short tile_height, int base_y, int base_x)
{
/* copying UINT16 pixels from the output buffer into the tile */
    int x;
    int y;
    int b;
    const unsigned short *p_in;
    unsigned short *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in =
	      outbuf + ((base_y + y) * width * num_bands) +
	      (base_x * num_bands);
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out += num_bands;
		      p_in += num_bands;
		      continue;
		  }
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_int32_outbuf_to_tile (const int *outbuf, int *tile,
			   unsigned short width,
			   unsigned short height,
			   unsigned short tile_width,
			   unsigned short tile_height, int base_y, int base_x)
{
/* copying INT32 pixels from the output buffer into the tile */
    int x;
    int y;
    const int *p_in;
    int *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_uint32_outbuf_to_tile (const unsigned int *outbuf, unsigned int *tile,
			    unsigned short width,
			    unsigned short height,
			    unsigned short tile_width,
			    unsigned short tile_height, int base_y, int base_x)
{
/* copying UINT32 pixels from the output buffer into the tile */
    int x;
    int y;
    const unsigned int *p_in;
    unsigned int *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_float_outbuf_to_tile (const float *outbuf, float *tile,
			   unsigned short width,
			   unsigned short height,
			   unsigned short tile_width,
			   unsigned short tile_height, int base_y, int base_x)
{
/* copying FLOAT pixels from the output buffer into the tile */
    int x;
    int y;
    const float *p_in;
    float *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_double_outbuf_to_tile (const double *outbuf, double *tile,
			    unsigned short width,
			    unsigned short height,
			    unsigned short tile_width,
			    unsigned short tile_height, int base_y, int base_x)
{
/* copying DOUBLE pixels from the output buffer into the tile */
    int x;
    int y;
    const double *p_in;
    double *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_from_outbuf_to_tile (const unsigned char *outbuf, unsigned char *tile,
			  unsigned char sample_type, unsigned char num_bands,
			  unsigned short width, unsigned short height,
			  unsigned short tile_width, unsigned short tile_height,
			  int base_y, int base_x)
{
/* copying pixels from the output buffer into the tile */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  copy_int8_outbuf_to_tile ((char *) outbuf,
				    (char *) tile, width, height, tile_width,
				    tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_INT16:
	  copy_int16_outbuf_to_tile ((short *) outbuf,
				     (short *) tile, width, height, tile_width,
				     tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_outbuf_to_tile ((unsigned short *) outbuf,
				      (unsigned short *) tile, num_bands,
				      width, height, tile_width, tile_height,
				      base_y, base_x);
	  break;
      case RL2_SAMPLE_INT32:
	  copy_int32_outbuf_to_tile ((int *) outbuf,
				     (int *) tile, width, height, tile_width,
				     tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_UINT32:
	  copy_uint32_outbuf_to_tile ((unsigned int *) outbuf,
				      (unsigned int *) tile, width, height,
				      tile_width, tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_FLOAT:
	  copy_float_outbuf_to_tile ((float *) outbuf,
				     (float *) tile, width, height, tile_width,
				     tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  copy_double_outbuf_to_tile ((double *) outbuf,
				      (double *) tile, width, height,
				      tile_width, tile_height, base_y, base_x);
	  break;
      default:
	  copy_uint8_outbuf_to_tile ((unsigned char *) outbuf,
				     (unsigned char *) tile, num_bands, width,
				     height, tile_width, tile_height, base_y,
				     base_x);
	  break;
      };
}

static int
mismatching_size (unsigned short width, unsigned short height, double x_res,
		  double y_res, double minx, double miny, double maxx,
		  double maxy)
{
/* checking if the image size and the map extent do match */
    double ext_x = (double) width * x_res;
    double ext_y = (double) height * y_res;
    double img_x = maxx - minx;
    double img_y = maxy = miny;
    double confidence;
    confidence = ext_x / 100.0;
    if (img_x < (ext_x - confidence) || img_x > (ext_x + confidence))
	return 0;
    confidence = ext_y / 100.0;
    if (img_y < (ext_y - confidence) || img_y > (ext_y + confidence))
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_export_geotiff_from_dbms (sqlite3 * handle, const char *dst_path,
			      rl2CoveragePtr cvg, double x_res, double y_res,
			      double minx, double miny, double maxx,
			      double maxy, unsigned short width,
			      unsigned short height, unsigned char compression,
			      unsigned short tile_sz, int with_worldfile)
{
/* exporting a GeoTIFF from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    int base_x;
    int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data = rl2_get_coverage_no_data (cvg);

    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (rl2_get_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, &outbuf, &outbuf_size, &palette, pixel_type) != RL2_OK)
	goto error;

/* computing the sample size */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pix_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pix_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pix_sz = 8;
	  break;
      };

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, pixel_type, num_bands,
					palette, compression, 1,
					tile_sz, srid, minx, miny, maxx,
					maxy, xx_res, yy_res, with_worldfile);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    if (with_worldfile)
      {
	  /* exporting the Worldfile */
	  if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	      goto error;
      }

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_tiff_worldfile_from_dbms (sqlite3 * handle, const char *dst_path,
				     rl2CoveragePtr cvg, double x_res,
				     double y_res, double minx, double miny,
				     double maxx, double maxy,
				     unsigned short width,
				     unsigned short height,
				     unsigned char compression,
				     unsigned short tile_sz)
{
/* exporting a TIFF+TFW from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    int base_x;
    int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data = rl2_get_coverage_no_data (cvg);

    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (rl2_get_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, &outbuf, &outbuf_size, &palette, pixel_type) != RL2_OK)
	goto error;

/* computing the sample size */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pix_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pix_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pix_sz = 8;
	  break;
      };

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, pixel_type,
					       num_bands, palette, compression,
					       1, tile_sz, srid, minx, miny,
					       maxx, maxy, xx_res, yy_res);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

/* exporting the Worldfile */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	goto error;

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
			   rl2CoveragePtr cvg, double x_res, double y_res,
			   double minx, double miny, double maxx,
			   double maxy, unsigned short width,
			   unsigned short height, unsigned char compression,
			   unsigned short tile_sz)
{
/* exporting a plain TIFF from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    int base_x;
    int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data = rl2_get_coverage_no_data (cvg);

    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_UINT8
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (rl2_get_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, &outbuf, &outbuf_size, &palette, pixel_type) != RL2_OK)
	goto error;

/* computing the sample size */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pix_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pix_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pix_sz = 8;
	  break;
      };

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     pixel_type, num_bands, palette,
				     compression, 1, tile_sz);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
					 num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_geotiff_from_dbms (sqlite3 * handle,
					  const char *dst_path,
					  rl2CoveragePtr cvg, double x_res,
					  double y_res, double minx,
					  double miny, double maxx,
					  double maxy, unsigned short width,
					  unsigned short height,
					  unsigned char red_band,
					  unsigned char green_band,
					  unsigned char blue_band,
					  unsigned char compression,
					  unsigned short tile_sz,
					  int with_worldfile)
{
/* exporting a Band-Composed GeoTIFF from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int base_x;
    int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (rl2_get_triple_band_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	 no_data) != RL2_OK)
	goto error;

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, RL2_PIXEL_RGB, 3,
					NULL, compression, 1, tile_sz, srid,
					minx, miny, maxx, maxy, xx_res, yy_res,
					with_worldfile);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    if (with_worldfile)
      {
	  /* exporting the Worldfile */
	  if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	      goto error;
      }

    rl2_destroy_tiff_destination (tiff);
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_geotiff_from_dbms (sqlite3 * handle,
					const char *dst_path,
					rl2CoveragePtr cvg, double x_res,
					double y_res, double minx,
					double miny, double maxx,
					double maxy, unsigned short width,
					unsigned short height,
					unsigned char mono_band,
					unsigned char compression,
					unsigned short tile_sz,
					int with_worldfile)
{
/* exporting a Mono-Band GeoTIFF from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data_mono = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int base_x;
    int base_y;
    unsigned char out_pixel;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_mono = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_mono, mono_band);

    if (rl2_get_mono_band_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, mono_band, &outbuf, &outbuf_size, no_data) != RL2_OK)
	goto error;

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, out_pixel, 1,
					NULL, compression, 1, tile_sz, srid,
					minx, miny, maxx, maxy, xx_res, yy_res,
					with_worldfile);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    if (with_worldfile)
      {
	  /* exporting the Worldfile */
	  if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	      goto error;
      }

    rl2_destroy_tiff_destination (tiff);
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_tiff_worldfile_from_dbms (sqlite3 * handle,
						 const char *dst_path,
						 rl2CoveragePtr cvg,
						 double x_res, double y_res,
						 double minx, double miny,
						 double maxx, double maxy,
						 unsigned short width,
						 unsigned short height,
						 unsigned char red_band,
						 unsigned char green_band,
						 unsigned char blue_band,
						 unsigned char compression,
						 unsigned short tile_sz)
{
/* exporting a Band-Composed TIFF+TFW from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int base_x;
    int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (rl2_get_triple_band_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	 no_data) != RL2_OK)
	goto error;

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, RL2_PIXEL_RGB,
					       3, NULL, compression, 1, tile_sz,
					       srid, minx, miny, maxx, maxy,
					       xx_res, yy_res);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

/* exporting the Worldfile */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	goto error;

    rl2_destroy_tiff_destination (tiff);
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_tiff_worldfile_from_dbms (sqlite3 * handle,
					       const char *dst_path,
					       rl2CoveragePtr cvg,
					       double x_res, double y_res,
					       double minx, double miny,
					       double maxx, double maxy,
					       unsigned short width,
					       unsigned short height,
					       unsigned char mono_band,
					       unsigned char compression,
					       unsigned short tile_sz)
{
/* exporting a Mono-Band TIFF+TFW from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int base_x;
    int base_y;
    unsigned char out_pixel;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_multi, mono_band);

    if (rl2_get_mono_band_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, mono_band, &outbuf, &outbuf_size, no_data) != RL2_OK)
	goto error;

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, out_pixel,
					       1, NULL, compression, 1, tile_sz,
					       srid, minx, miny, maxx, maxy,
					       xx_res, yy_res);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

/* exporting the Worldfile */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	goto error;

    rl2_destroy_tiff_destination (tiff);
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
				       rl2CoveragePtr cvg, double x_res,
				       double y_res, double minx, double miny,
				       double maxx, double maxy,
				       unsigned short width,
				       unsigned short height,
				       unsigned char red_band,
				       unsigned char green_band,
				       unsigned char blue_band,
				       unsigned char compression,
				       unsigned short tile_sz)
{
/* exporting a plain Band-Composed TIFF from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int base_x;
    int base_y;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (rl2_get_triple_band_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	 no_data) != RL2_OK)
	goto error;

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     RL2_PIXEL_RGB, 3, NULL,
				     compression, 1, tile_sz);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    rl2_destroy_tiff_destination (tiff);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
				     rl2CoveragePtr cvg, double x_res,
				     double y_res, double minx, double miny,
				     double maxx, double maxy,
				     unsigned short width,
				     unsigned short height,
				     unsigned char mono_band,
				     unsigned char compression,
				     unsigned short tile_sz)
{
/* exporting a plain Mono-Band TIFF from the DBMS into the file-system */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int base_x;
    int base_y;
    unsigned char out_pixel;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_multi, mono_band);

    if (rl2_get_mono_band_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, mono_band, &outbuf, &outbuf_size, no_data) != RL2_OK)
	goto error;

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     out_pixel, 1, NULL,
				     compression, 1, tile_sz);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    rl2_destroy_tiff_destination (tiff);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_ascii_grid_from_dbms (sqlite3 * handle, const char *dst_path,
				 rl2CoveragePtr cvg, double res,
				 double minx, double miny, double maxx,
				 double maxy, unsigned short width,
				 unsigned short height, int is_centered,
				 int decimal_digits)
{
/* exporting an ASCII Grid from the DBMS into the file-system */
    rl2PalettePtr palette = NULL;
    rl2AsciiGridDestinationPtr ascii = NULL;
    rl2PixelPtr pixel;
    unsigned char level;
    unsigned char scale;
    double xx_res = res;
    double yy_res = res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    double no_data = -9999.0;
    int base_y;
    unsigned char *pixels = NULL;
    int pixels_size;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;

    if (pixel_type != RL2_PIXEL_DATAGRID || num_bands != 1)
	goto error;

    pixel = rl2_get_coverage_no_data (cvg);
    if (pixel != NULL)
      {
	  /* attempting to retrieve the NO-DATA value */
	  unsigned char st;
	  unsigned char pt;
	  unsigned char nb;
	  if (rl2_get_pixel_type (pixel, &st, &pt, &nb) == RL2_OK)
	    {
		if (st == RL2_SAMPLE_INT8)
		  {
		      char v8;
		      if (rl2_get_pixel_sample_int8 (pixel, &v8) == RL2_OK)
			  no_data = v8;
		  }
		if (st == RL2_SAMPLE_UINT8)
		  {
		      unsigned char vu8;
		      if (rl2_get_pixel_sample_uint8 (pixel, 0, &vu8) == RL2_OK)
			  no_data = vu8;
		  }
		if (st == RL2_SAMPLE_INT16)
		  {
		      short v16;
		      if (rl2_get_pixel_sample_int16 (pixel, &v16) == RL2_OK)
			  no_data = v16;
		  }
		if (st == RL2_SAMPLE_UINT16)
		  {
		      unsigned short vu16;
		      if (rl2_get_pixel_sample_uint16 (pixel, 0, &vu16) ==
			  RL2_OK)
			  no_data = vu16;
		  }
		if (st == RL2_SAMPLE_INT32)
		  {
		      int v32;
		      if (rl2_get_pixel_sample_int32 (pixel, &v32) == RL2_OK)
			  no_data = v32;
		  }
		if (st == RL2_SAMPLE_UINT32)
		  {
		      unsigned int vu32;
		      if (rl2_get_pixel_sample_uint32 (pixel, &vu32) == RL2_OK)
			  no_data = vu32;
		  }
		if (st == RL2_SAMPLE_FLOAT)
		  {
		      float vflt;
		      if (rl2_get_pixel_sample_float (pixel, &vflt) == RL2_OK)
			  no_data = vflt;
		  }
		if (st == RL2_SAMPLE_DOUBLE)
		  {
		      double vdbl;
		      if (rl2_get_pixel_sample_double (pixel, &vdbl) == RL2_OK)
			  no_data = vdbl;
		  }
	    }
      }

    if (rl2_get_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, res, res, &pixels,
	 &pixels_size, &palette, RL2_PIXEL_DATAGRID) != RL2_OK)
	goto error;

    ascii =
	rl2_create_ascii_grid_destination (dst_path, width, height,
					   xx_res, minx, miny, is_centered,
					   no_data, decimal_digits, pixels,
					   pixels_size, sample_type);
    if (ascii == NULL)
	goto error;
    pixels = NULL;		/* pix-buffer ownership now belongs to the ASCII object */
/* writing the ASCII Grid header */
    if (rl2_write_ascii_grid_header (ascii) != RL2_OK)
	goto error;
    for (base_y = 0; base_y < height; base_y++)
      {
	  /* exporting all scanlines from the output buffer */
	  unsigned short line_no;
	  if (rl2_write_ascii_grid_scanline (ascii, &line_no) != RL2_OK)
	      goto error;
      }

    rl2_destroy_ascii_grid_destination (ascii);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_OK;

  error:
    if (ascii != NULL)
	rl2_destroy_ascii_grid_destination (ascii);
    if (pixels != NULL)
	free (pixels);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_jpeg_from_dbms (sqlite3 * handle, const char *dst_path,
			   rl2CoveragePtr cvg, double x_res,
			   double y_res, double minx, double miny,
			   double maxx, double maxy,
			   unsigned short width,
			   unsigned short height, int quality,
			   int with_worldfile)
{
/* exporting a JPEG (with possible JGW) from the DBMS into the file-system */
    rl2SectionPtr section = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char *outbuf = NULL;
    int outbuf_size;

    if (rl2_find_matching_resolution
	(handle, cvg, &xx_res, &yy_res, &level, &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (sample_type == RL2_SAMPLE_UINT8 && pixel_type == RL2_PIXEL_GRAYSCALE
	&& num_bands == 1)
	;
    else if (sample_type == RL2_SAMPLE_UINT8 && pixel_type == RL2_PIXEL_RGB
	     && num_bands == 3)
	;
    else
	goto error;

    if (rl2_get_raw_raster_data
	(handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	 yy_res, &outbuf, &outbuf_size, NULL, pixel_type) != RL2_OK)
	goto error;

    raster =
	rl2_create_raster (width, height, sample_type, pixel_type, num_bands,
			   outbuf, outbuf_size, NULL, NULL, 0, NULL);
    outbuf = NULL;
    if (raster == NULL)
	goto error;
    section =
	rl2_create_section ("jpeg", RL2_COMPRESSION_JPEG, 256, 256, raster);
    raster = NULL;
    if (section == NULL)
	goto error;
    if (rl2_section_to_jpeg (section, dst_path, quality) != RL2_OK)
	goto error;

    if (with_worldfile)
      {
	  /* exporting the JGW WorldFile */
	  write_jgw_worldfile (dst_path, minx, maxy, x_res, y_res);
      }

    rl2_destroy_section (section);
    return RL2_OK;

  error:
    if (section != NULL)
	rl2_destroy_section (section);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (outbuf != NULL)
	free (outbuf);
    return RL2_ERROR;
}

static int
resolve_section_id (sqlite3 * handle, const char *coverage, const char *section,
		    sqlite3_int64 * sect_id)
{
/* resolving the Section ID by name */
    char *table;
    char *xtable;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int ok = 0;

/* Section infos */
    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT section_id "
			   "FROM \"%s\" WHERE section_name = %Q", xtable,
			   section);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		*sect_id = sqlite3_column_int64 (stmt, 0);
		ok = 1;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_info; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return ok;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
delete_section_pyramid (sqlite3 * handle, const char *coverage,
			const char *section)
{
/* attempting to delete a section pyramid */
    char *sql;
    char *table;
    char *xtable;
    sqlite3_int64 section_id;
    char sect_id[1024];
    int ret;
    char *err_msg = NULL;

    if (!resolve_section_id (handle, coverage, section, &section_id))
	return 0;
#if defined(_WIN32) && !defined(__MINGW32__)
    sprintf (sect_id, "%I64d", section_id);
#else
    sprintf (sect_id, "%lld", section_id);
#endif

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("DELETE FROM \"%s\" WHERE pyramid_level > 0 AND section_id = %s",
	 xtable, sect_id);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE FROM \"%s_tiles\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
check_section_pyramid (sqlite3 * handle, const char *coverage,
		       const char *section)
{
/* checking if a section's pyramid already exists */
    char *sql;
    char *table;
    char *xtable;
    sqlite3_int64 section_id;
    char sect_id[1024];
    sqlite3_stmt *stmt = NULL;
    int ret;
    int count = 0;

    if (!resolve_section_id (handle, coverage, section, &section_id))
	return 1;
#if defined(_WIN32) && !defined(__MINGW32__)
    sprintf (sect_id, "%I64d", section_id);
#else
    sprintf (sect_id, "%lld", section_id);
#endif

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT Count(*) FROM \"%s\" "
			 "WHERE section_id = %s AND pyramid_level > 0", xtable,
			 sect_id);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 1;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT pyramid_exists; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		count = 0;
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 0)
	return 1;
    return 0;
}

static int
get_section_infos (sqlite3 * handle, const char *coverage, const char *section,
		   sqlite3_int64 * sect_id, unsigned short *sect_width,
		   unsigned short *sect_height, double *minx, double *miny,
		   double *maxx, double *maxy, rl2PalettePtr * palette,
		   rl2PixelPtr * no_data)
{
/* retrieving the Section most relevant infos */
    char *table;
    char *xtable;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int ok = 0;

/* Section infos */
    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT section_id, width, height, MbrMinX(geometry), "
			 "MbrMinY(geometry), MbrMaxX(geometry), MbrMaxY(geometry) "
			 "FROM \"%s\" WHERE section_name = %Q", xtable,
			 section);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		*sect_id = sqlite3_column_int64 (stmt, 0);
		*sect_width = sqlite3_column_int (stmt, 1);
		*sect_height = sqlite3_column_int (stmt, 2);
		*minx = sqlite3_column_double (stmt, 3);
		*miny = sqlite3_column_double (stmt, 4);
		*maxx = sqlite3_column_double (stmt, 5);
		*maxy = sqlite3_column_double (stmt, 6);
		ok = 1;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_info; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (!ok)
	goto error;

/* Coverage's palette and no-data */
    sql = sqlite3_mprintf ("SELECT palette, nodata_pixel FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      *palette = rl2_deserialize_dbms_palette (blob, blob_sz);
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 1);
		      int blob_sz = sqlite3_column_bytes (stmt, 1);
		      *no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_info; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static SectionPyramidTileInPtr
alloc_section_pyramid_tile (sqlite3_int64 tile_id, double minx, double miny,
			    double maxx, double maxy)
{
/* allocating a Section Pyramid Tile object */
    SectionPyramidTileInPtr tile = malloc (sizeof (SectionPyramidTileIn));
    if (tile == NULL)
	return NULL;
    tile->tile_id = tile_id;
    tile->cx = minx + ((maxx - minx) / 2.0);
    tile->cy = miny + ((maxy - miny) / 2.0);
    tile->next = NULL;
    return tile;
}

static SectionPyramidPtr
alloc_sect_pyramid (sqlite3_int64 sect_id, unsigned short sect_width,
		    unsigned short sect_height, unsigned char sample_type,
		    unsigned char pixel_type, unsigned char num_samples,
		    unsigned char compression, int quality, int srid,
		    double res_x, double res_y, double tile_width,
		    double tile_height, double minx, double miny, double maxx,
		    double maxy, int scale)
{
/* allocating a Section Pyramid object */
    double ext_x = maxx - minx;
    double ext_y = maxy - miny;
    SectionPyramidPtr pyr = malloc (sizeof (SectionPyramid));
    if (pyr == NULL)
	return NULL;
    pyr->section_id = sect_id;
    pyr->scale = scale;
    pyr->full_width = sect_width;
    pyr->full_height = sect_height;
    pyr->sample_type = sample_type;
    pyr->pixel_type = pixel_type;
    pyr->num_samples = num_samples;
    pyr->compression = compression;
    pyr->quality = quality;
    pyr->srid = srid;
    pyr->res_x = res_x;
    pyr->res_y = res_y;
    pyr->scaled_width = ext_x / res_x;
    pyr->scaled_height = ext_y / res_y;
    pyr->tile_width = tile_width;
    pyr->tile_height = tile_height;
    pyr->minx = minx;
    pyr->miny = miny;
    pyr->maxx = maxx;
    pyr->maxy = maxy;
    pyr->first_in = NULL;
    pyr->last_in = NULL;
    pyr->first_out = NULL;
    pyr->last_out = NULL;
    return pyr;
}

static void
delete_sect_pyramid (SectionPyramidPtr pyr)
{
/* memory cleanup - destroying a Section Pyramid object */
    SectionPyramidTileInPtr tile_in;
    SectionPyramidTileInPtr tile_in_n;
    SectionPyramidTileOutPtr tile_out;
    SectionPyramidTileOutPtr tile_out_n;
    if (pyr == NULL)
	return;
    tile_out = pyr->first_out;
    while (tile_out != NULL)
      {
	  SectionPyramidTileRefPtr ref;
	  SectionPyramidTileRefPtr ref_n;
	  tile_out_n = tile_out->next;
	  ref = tile_out->first;
	  while (ref != NULL)
	    {
		ref_n = ref->next;
		free (ref);
		ref = ref_n;
	    }
	  free (tile_out);
	  tile_out = tile_out_n;
      }
    tile_in = pyr->first_in;
    while (tile_in != NULL)
      {
	  tile_in_n = tile_in->next;
	  free (tile_in);
	  tile_in = tile_in_n;
      }
    free (pyr);
}

static int
insert_tile_into_section_pyramid (SectionPyramidPtr pyr, sqlite3_int64 tile_id,
				  double minx, double miny, double maxx,
				  double maxy)
{
/* inserting a base tile into the Pyramid level */
    SectionPyramidTileInPtr tile;
    if (pyr == NULL)
	return 0;
    tile = alloc_section_pyramid_tile (tile_id, minx, miny, maxx, maxy);
    if (tile == NULL)
	return 0;
    if (pyr->first_in == NULL)
	pyr->first_in = tile;
    if (pyr->last_in != NULL)
	pyr->last_in->next = tile;
    pyr->last_in = tile;
    return 1;
}

static SectionPyramidTileOutPtr
add_pyramid_out_tile (SectionPyramidPtr pyr, unsigned short row,
		      unsigned short col, double minx, double miny, double maxx,
		      double maxy)
{
/* inserting a Parent tile (output) */
    SectionPyramidTileOutPtr tile;
    if (pyr == NULL)
	return NULL;
    tile = malloc (sizeof (SectionPyramidTileOut));
    if (tile == NULL)
	return NULL;
    tile->row = row;
    tile->col = col;
    tile->minx = minx;
    tile->miny = miny;
    tile->maxx = maxx;
    tile->maxy = maxy;
    tile->first = NULL;
    tile->last = NULL;
    tile->next = NULL;
    if (pyr->first_out == NULL)
	pyr->first_out = tile;
    if (pyr->last_out != NULL)
	pyr->last_out->next = tile;
    pyr->last_out = tile;
    return tile;
}

static void
add_pyramid_sub_tile (SectionPyramidTileOutPtr parent,
		      SectionPyramidTileInPtr child)
{
/* inserting a Child tile (input) */
    SectionPyramidTileRefPtr ref;
    if (parent == NULL)
	return;
    ref = malloc (sizeof (SectionPyramidTileRef));
    if (ref == NULL)
	return;
    ref->child = child;
    ref->next = NULL;
    if (parent->first == NULL)
	parent->first = ref;
    if (parent->last != NULL)
	parent->last->next = ref;
    parent->last = ref;
}

static void
set_pyramid_tile_destination (SectionPyramidPtr pyr, double minx, double miny,
			      double maxx, double maxy, unsigned short row,
			      unsigned short col)
{
/* aggregating lower level tiles */
    int first = 1;
    SectionPyramidTileOutPtr out = NULL;
    SectionPyramidTileInPtr tile = pyr->first_in;
    while (tile != NULL)
      {
	  if (tile->cx > minx && tile->cx < maxx && tile->cy > miny
	      && tile->cy < maxy)
	    {
		if (first)
		  {
		      out =
			  add_pyramid_out_tile (pyr, row, col, minx, miny, maxx,
						maxy);
		      first = 0;
		  }
		if (out != NULL)
		    add_pyramid_sub_tile (out, tile);
	    }
	  tile = tile->next;
      }
}

static unsigned char *
load_tile_base (sqlite3_stmt * stmt, sqlite3_int64 tile_id,
		rl2PalettePtr palette, unsigned char pixel_type)
{
/* attempting to read a lower-level tile */
    int ret;
    const unsigned char *blob_odd = NULL;
    int blob_odd_sz = 0;
    const unsigned char *blob_even = NULL;
    int blob_even_sz = 0;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr plt = NULL;
    unsigned char *rgba_tile = NULL;
    int rgba_sz;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, tile_id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob_odd = sqlite3_column_blob (stmt, 0);
		      blob_odd_sz = sqlite3_column_bytes (stmt, 0);
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_BLOB)
		  {
		      blob_even = sqlite3_column_blob (stmt, 1);
		      blob_even_sz = sqlite3_column_bytes (stmt, 1);
		  }
		plt = rl2_clone_palette (palette);
		raster =
		    rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz,
				       blob_even, blob_even_sz, plt);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      return NULL;
		  }
		if (rl2_raster_data_to_RGBA (raster, &rgba_tile, &rgba_sz) !=
		    RL2_OK)
		    rgba_tile = NULL;
		rl2_destroy_raster (raster);
		break;
	    }
	  else
	      return NULL;
      }
    if (rgba_tile != NULL && pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  /* preserving dark pixels */
	  int i;
	  unsigned char *p = rgba_tile;
	  for (i = 0; i < rgba_sz; i += 4)
	    {
		unsigned char r = *p++;
		p += 2;
		if (*p > 128)
		  {
		      if (r >= 192)
			  *p++ = 32;
		      else if (r >= 128)
			  *p++ = 64;
		      else if (r >= 128)
			  *p++ = 128;
		      else
			  *p++ = 255;
		  }
		else
		    p++;
	    }
      }
    return rgba_tile;
}

static rl2RasterPtr
load_tile_base_generic (sqlite3_stmt * stmt, sqlite3_int64 tile_id)
{
/* attempting to read a lower-level tile */
    int ret;
    const unsigned char *blob_odd = NULL;
    int blob_odd_sz = 0;
    const unsigned char *blob_even = NULL;
    int blob_even_sz = 0;
    rl2RasterPtr raster = NULL;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, tile_id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob_odd = sqlite3_column_blob (stmt, 0);
		      blob_odd_sz = sqlite3_column_bytes (stmt, 0);
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_BLOB)
		  {
		      blob_even = sqlite3_column_blob (stmt, 1);
		      blob_even_sz = sqlite3_column_bytes (stmt, 1);
		  }
		raster =
		    rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz,
				       blob_even, blob_even_sz, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      return NULL;
		  }
		return raster;
	    }
      }
    return NULL;
}

static double
rescale_pixel_int8 (const char *buf_in, unsigned short tileWidth,
		    unsigned short tileHeight, int x, int y, char nd)
{
/* rescaling a DataGrid pixel (8x8) - INT8 */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const char *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const char *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (char) (sum / (double) valid);
}

static void
rescale_grid_int8 (char *buf_out, unsigned short tileWidth,
		   unsigned short tileHeight, const char *buf_in, int x, int y,
		   int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - int 8 bit */
    unsigned short row;
    unsigned short col;
    char nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_INT8 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->int8;
	    }
      }

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  char *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		char *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_int8 (buf_in, tileWidth, tileHeight, col * 8,
					row * 8, nd);
	    }
      }
}

static double
rescale_pixel_uint8 (const unsigned char *buf_in, unsigned short tileWidth,
		     unsigned short tileHeight, int x, int y, unsigned char nd)
{
/* rescaling a DataGrid pixel (8x8) - UINT8 */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned char *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned char *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned char) (sum / (double) valid);
}

static void
rescale_grid_uint8 (unsigned char *buf_out, unsigned short tileWidth,
		    unsigned short tileHeight, const unsigned char *buf_in,
		    int x, int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - unsigned int 8 bit */
    unsigned short row;
    unsigned short col;
    unsigned char nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_UINT8 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->uint8;
	    }
      }

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  unsigned char *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		unsigned char *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_uint8 (buf_in, tileWidth, tileHeight, col * 8,
					 row * 8, nd);
	    }
      }
}

static double
rescale_pixel_int16 (const short *buf_in, unsigned short tileWidth,
		     unsigned short tileHeight, int x, int y, short nd)
{
/* rescaling a DataGrid pixel (8x8) - INT32 */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const short *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const short *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (short) (sum / (double) valid);
}

static void
rescale_grid_int16 (short *buf_out, unsigned short tileWidth,
		    unsigned short tileHeight, const short *buf_in, int x,
		    int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - int 16 bit */
    unsigned short row;
    unsigned short col;
    short nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_INT16 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->int16;
	    }
      }

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  short *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		short *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_int16 (buf_in, tileWidth, tileHeight, col * 8,
					 row * 8, nd);
	    }
      }
}

static double
rescale_pixel_uint16 (const unsigned short *buf_in, unsigned short tileWidth,
		      unsigned short tileHeight, int x, int y,
		      unsigned short nd)
{
/* rescaling a DataGrid pixel (8x8) - UINT16 */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned short *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned short *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned short) (sum / (double) valid);
}

static void
rescale_grid_uint16 (unsigned short *buf_out, unsigned short tileWidth,
		     unsigned short tileHeight, const unsigned short *buf_in,
		     int x, int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - unsigned int 16 bit */
    unsigned short row;
    unsigned short col;
    unsigned short nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_UINT16 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->uint16;
	    }
      }

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  unsigned short *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		unsigned short *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_uint16 (buf_in, tileWidth, tileHeight,
					  col * 8, row * 8, nd);
	    }
      }
}

static double
rescale_pixel_int32 (const int *buf_in, unsigned short tileWidth,
		     unsigned short tileHeight, int x, int y, int nd)
{
/* rescaling a DataGrid pixel (8x8) - INT32 */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const int *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const int *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (int) (sum / (double) valid);
}

static void
rescale_grid_int32 (int *buf_out, unsigned short tileWidth,
		    unsigned short tileHeight, const int *buf_in, int x, int y,
		    int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - int 32 bit */
    unsigned short row;
    unsigned short col;
    int nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_INT32 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->int32;
	    }
      }

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  int *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		int *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_int32 (buf_in, tileWidth, tileHeight, col * 8,
					 row * 8, nd);
	    }
      }
}

static double
rescale_pixel_uint32 (const unsigned int *buf_in, unsigned short tileWidth,
		      unsigned short tileHeight, int x, int y, unsigned int nd)
{
/* rescaling a DataGrid pixel (8x8) - UINT32 */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned int *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned int *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned int) (sum / (double) valid);
}

static void
rescale_grid_uint32 (unsigned int *buf_out, unsigned short tileWidth,
		     unsigned short tileHeight, const unsigned int *buf_in,
		     int x, int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - unsigned int 32 bit */
    unsigned short row;
    unsigned short col;
    unsigned int nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_UINT32 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->uint32;
	    }
      }

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  unsigned int *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		unsigned int *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_uint32 (buf_in, tileWidth, tileHeight,
					  col * 8, row * 8, nd);
	    }
      }
}

static double
rescale_pixel_float (const float *buf_in, unsigned short tileWidth,
		     unsigned short tileHeight, int x, int y, float nd)
{
/* rescaling a DataGrid pixel (8x8) - Float */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const float *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const float *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (float) (sum / (double) valid);
}

static void
rescale_grid_float (float *buf_out, unsigned short tileWidth,
		    unsigned short tileHeight, const float *buf_in, int x,
		    int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - Float */
    unsigned short row;
    unsigned short col;
    float nd = 0.0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_FLOAT && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->float32;
	    }
      }

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  float *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		float *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_float (buf_in, tileWidth, tileHeight, col * 8,
					 row * 8, nd);
	    }
      }
}

static double
rescale_pixel_double (const double *buf_in, unsigned short tileWidth,
		      unsigned short tileHeight, int x, int y, double nd)
{
/* rescaling a DataGrid pixel (8x8) - Double */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const double *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const double *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return sum / (double) valid;
}

static void
rescale_grid_double (double *buf_out, unsigned short tileWidth,
		     unsigned short tileHeight, const double *buf_in, int x,
		     int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - Double */
    unsigned short row;
    unsigned short col;
    double nd = 0.0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_DOUBLE && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->float64;
	    }
      }

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  double *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		double *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_double (buf_in, tileWidth, tileHeight,
					  col * 8, row * 8, nd);
	    }
      }
}

static void
rescale_grid (void *buf_out, unsigned short tileWidth,
	      unsigned short tileHeight, const void *buf_in,
	      unsigned char sample_type, int x, int y, int tic_x, int tic_y,
	      rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  rescale_grid_int8 ((char *) buf_out, tileWidth, tileHeight,
			     (const char *) buf_in, x, y, tic_x, tic_y,
			     no_data);
	  break;
      case RL2_SAMPLE_UINT8:
	  rescale_grid_uint8 ((unsigned char *) buf_out, tileWidth, tileHeight,
			      (const unsigned char *) buf_in, x, y, tic_x,
			      tic_y, no_data);
	  break;
      case RL2_SAMPLE_INT16:
	  rescale_grid_int16 ((short *) buf_out, tileWidth, tileHeight,
			      (const short *) buf_in, x, y, tic_x, tic_y,
			      no_data);
	  break;
      case RL2_SAMPLE_UINT16:
	  rescale_grid_uint16 ((unsigned short *) buf_out, tileWidth,
			       tileHeight, (const unsigned short *) buf_in, x,
			       y, tic_x, tic_y, no_data);
	  break;
      case RL2_SAMPLE_INT32:
	  rescale_grid_int32 ((int *) buf_out, tileWidth, tileHeight,
			      (const int *) buf_in, x, y, tic_x, tic_y,
			      no_data);
	  break;
      case RL2_SAMPLE_UINT32:
	  rescale_grid_uint32 ((unsigned int *) buf_out, tileWidth, tileHeight,
			       (const unsigned int *) buf_in, x, y, tic_x,
			       tic_y, no_data);
	  break;
      case RL2_SAMPLE_FLOAT:
	  rescale_grid_float ((float *) buf_out, tileWidth, tileHeight,
			      (const float *) buf_in, x, y, tic_x, tic_y,
			      no_data);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  rescale_grid_double ((double *) buf_out, tileWidth, tileHeight,
			       (const double *) buf_in, x, y, tic_x, tic_y,
			       no_data);
	  break;
      };
}

static int
upate_sect_pyramid_grid (sqlite3 * handle, sqlite3_stmt * stmt_rd,
			 sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_data,
			 SectionPyramid * pyr, unsigned short tileWidth,
			 unsigned short tileHeight, int id_level,
			 rl2PixelPtr no_data, unsigned char sample_type,
			 unsigned char compression)
{
/* creating and inserting Pyramid tiles */
    unsigned char *buf_out = NULL;
    unsigned char *mask = NULL;
    SectionPyramidTileOutPtr tile_out;
    SectionPyramidTileRefPtr tile_in;
    int x;
    int y;
    int row;
    int col;
    int tic_x;
    int tic_y;
    double pos_y;
    double pos_x;
    double geo_x;
    double geo_y;
    rl2PixelPtr nd = NULL;
    rl2RasterPtr raster_out = NULL;
    rl2RasterPtr raster_in = NULL;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    int pixel_sz = 1;
    int out_sz;
    int mask_sz = 0;

    if (pyr == NULL)
	goto error;
    tic_x = tileWidth / pyr->scale;
    tic_y = tileHeight / pyr->scale;
    geo_x = (double) tic_x *pyr->res_x;
    geo_y = (double) tic_y *pyr->res_y;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pixel_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pixel_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pixel_sz = 8;
	  break;
      };
    out_sz = tileWidth * tileHeight * pixel_sz;

    tile_out = pyr->first_out;
    while (tile_out != NULL)
      {
	  rl2PrivRasterPtr rst;
	  /* allocating the output buffer */
	  buf_out = malloc (out_sz);
	  if (buf_out == NULL)
	      goto error;
	  rl2_prime_void_tile (buf_out, tileWidth, tileHeight, sample_type, 1,
			       no_data);

	  if (tile_out->col + tileWidth > pyr->scaled_width
	      || tile_out->row + tileHeight > pyr->scaled_height)
	    {
		/* allocating and initializing a transparency mask */
		unsigned char *p;
		mask_sz = tileWidth * tileHeight;
		mask = malloc (mask_sz);
		if (mask == NULL)
		    goto error;
		p = mask;
		for (row = 0; row < tileHeight; row++)
		  {
		      int x_row = tile_out->row + row;
		      for (col = 0; col < tileWidth; col++)
			{
			    int x_col = tile_out->col + col;
			    if (x_row >= pyr->scaled_height
				|| x_col >= pyr->scaled_width)
			      {
				  /* masking any portion of the tile exceeding the scaled section size */
				  *p++ = 0;
			      }
			    else
				*p++ = 1;
			}
		  }
	    }
	  else
	    {
		mask = NULL;
		mask_sz = 0;
	    }

	  /* creating the output (rescaled) tile */
	  tile_in = tile_out->first;
	  while (tile_in != NULL)
	    {
		/* loading and rescaling the base tiles */
		raster_in =
		    load_tile_base_generic (stmt_rd, tile_in->child->tile_id);
		if (raster_in == NULL)
		    goto error;
		pos_y = tile_out->maxy;
		x = 0;
		y = 0;
		for (row = 0; row < tileHeight; row += tic_y)
		  {
		      pos_x = tile_out->minx;
		      for (col = 0; col < tileWidth; col += tic_x)
			{
			    if (tile_in->child->cy < pos_y
				&& tile_in->child->cy > (pos_y - geo_y)
				&& tile_in->child->cx > pos_x
				&& tile_in->child->cx < (pos_x + geo_x))
			      {
				  x = col;
				  y = row;
				  break;
			      }
			    pos_x += geo_x;
			}
		      pos_y -= geo_y;
		  }
		rst = (rl2PrivRasterPtr) raster_in;
		rescale_grid (buf_out, tileWidth, tileHeight, rst->rasterBuffer,
			      sample_type, x, y, tic_x, tic_y, no_data);
		rl2_destroy_raster (raster_in);
		raster_in = NULL;
		tile_in = tile_in->next;
	    }

	  raster_out = NULL;
	  raster_out =
	      rl2_create_raster (tileWidth, tileHeight, sample_type,
				 RL2_PIXEL_DATAGRID, 1, buf_out,
				 out_sz, NULL, mask, mask_sz, nd);
	  buf_out = NULL;
	  mask = NULL;
	  if (raster_out == NULL)
	    {
		fprintf (stderr, "ERROR: unable to create a Pyramid Tile\n");
		goto error;
	    }
	  if (rl2_raster_encode
	      (raster_out, compression, &blob_odd, &blob_odd_sz, &blob_even,
	       &blob_even_sz, 100, 1) != RL2_OK)
	    {
		fprintf (stderr, "ERROR: unable to encode a Pyramid tile\n");
		goto error;
	    }
	  rl2_destroy_raster (raster_out);
	  raster_out = NULL;

	  /* INSERTing the tile */
	  if (!do_insert_pyramid_tile
	      (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz, id_level,
	       pyr->section_id, pyr->srid, tile_out->minx, tile_out->miny,
	       tile_out->maxx, tile_out->maxy, stmt_tils, stmt_data))
	      goto error;

	  tile_out = tile_out->next;
      }

    return 1;

  error:
    if (raster_in != NULL)
	rl2_destroy_raster (raster_in);
    if (raster_out != NULL)
	rl2_destroy_raster (raster_out);
    if (buf_out != NULL)
	free (buf_out);
    if (mask != NULL)
	free (mask);
    return 0;
}

static double
rescale_mb_pixel_uint8 (const unsigned char *buf_in, unsigned short tileWidth,
			unsigned short tileHeight, int x, int y,
			unsigned char nd, unsigned char nb,
			unsigned char num_bands)
{
/* rescaling a MultiBand pixel sample (8x8) - UINT8 */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned char *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth * num_bands);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned char *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + (xx * num_bands) + nb;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned char) (sum / (double) valid);
}

static void
rescale_multiband_uint8 (unsigned char *buf_out, unsigned short tileWidth,
			 unsigned short tileHeight, const unsigned char *buf_in,
			 int x, int y, int tic_x, int tic_y,
			 unsigned char num_bands, rl2PixelPtr no_data)
{
/* rescaling a MultiBand tile - unsigned int 8 bit */
    unsigned short row;
    unsigned short col;
    unsigned char nb;
    unsigned char nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  unsigned char *p_out_base = buf_out + (yy * tileWidth * num_bands);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		unsigned char *p_out = p_out_base + (xx * num_bands);
		if (xx >= tileWidth)
		    break;
		for (nb = 0; nb < num_bands; nb++)
		  {
		      if (pxl != NULL)
			{
			    if (pxl->sampleType == RL2_SAMPLE_UINT8
				&& pxl->nBands == num_bands)
			      {
				  rl2PrivSamplePtr sample = pxl->Samples + nb;
				  nd = sample->uint8;
			      }
			}
		      *(p_out + nb) =
			  rescale_mb_pixel_uint8 (buf_in, tileWidth, tileHeight,
						  col * 8, row * 8, nd, nb,
						  num_bands);
		  }
	    }
      }
}

static double
rescale_mb_pixel_uint16 (const unsigned short *buf_in, unsigned short tileWidth,
			 unsigned short tileHeight, int x, int y,
			 unsigned short nd, unsigned char nb,
			 unsigned char num_bands)
{
/* rescaling a MultiBand pixel sample (8x8) - UINT16 */
    unsigned short row;
    unsigned short col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned short *p_in_base;
	  unsigned short yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth * num_bands);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned short *p_in;
		unsigned short xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + (xx * num_bands) + nb;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned short) (sum / (double) valid);
}

static void
rescale_multiband_uint16 (unsigned short *buf_out, unsigned short tileWidth,
			  unsigned short tileHeight,
			  const unsigned short *buf_in, int x, int y, int tic_x,
			  int tic_y, unsigned char num_bands,
			  rl2PixelPtr no_data)
{
/* rescaling a MultiBand tile - unsigned int 16 bit */
    unsigned short row;
    unsigned short col;
    unsigned char nb;
    unsigned short nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    for (row = 0; row < tic_y; row++)
      {
	  unsigned short yy = row + y;
	  unsigned short *p_out_base = buf_out + (yy * tileWidth * num_bands);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned short xx = col + x;
		unsigned short *p_out = p_out_base + (xx * num_bands);
		if (xx >= tileWidth)
		    break;
		for (nb = 0; nb < num_bands; nb++)
		  {
		      if (pxl != NULL)
			{
			    if (pxl->sampleType == RL2_SAMPLE_UINT16
				&& pxl->nBands == num_bands)
			      {
				  rl2PrivSamplePtr sample = pxl->Samples + nb;
				  nd = sample->uint16;
			      }
			}
		      *(p_out + nb) =
			  rescale_mb_pixel_uint16 (buf_in, tileWidth,
						   tileHeight, col * 8, row * 8,
						   nd, nb, num_bands);
		  }
	    }
      }
}

static void
rescale_multiband (void *buf_out, unsigned short tileWidth,
		   unsigned short tileHeight, const void *buf_in,
		   unsigned char sample_type, unsigned char num_bands, int x,
		   int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a Multiband tile */
    switch (sample_type)
      {
      case RL2_SAMPLE_UINT8:
	  rescale_multiband_uint8 ((unsigned char *) buf_out, tileWidth,
				   tileHeight, (const unsigned char *) buf_in,
				   x, y, tic_x, tic_y, num_bands, no_data);
	  break;
      case RL2_SAMPLE_UINT16:
	  rescale_multiband_uint16 ((unsigned short *) buf_out, tileWidth,
				    tileHeight, (const unsigned short *) buf_in,
				    x, y, tic_x, tic_y, num_bands, no_data);
	  break;
      };
}

static int
upate_sect_pyramid_multiband (sqlite3 * handle, sqlite3_stmt * stmt_rd,
			      sqlite3_stmt * stmt_tils,
			      sqlite3_stmt * stmt_data, SectionPyramid * pyr,
			      unsigned short tileWidth,
			      unsigned short tileHeight, int id_level,
			      rl2PixelPtr no_data, unsigned char sample_type,
			      unsigned char num_bands,
			      unsigned char compression)
{
/* creating and inserting Pyramid tiles */
    unsigned char *buf_out = NULL;
    unsigned char *mask = NULL;
    SectionPyramidTileOutPtr tile_out;
    SectionPyramidTileRefPtr tile_in;
    int x;
    int y;
    int row;
    int col;
    int tic_x;
    int tic_y;
    double pos_y;
    double pos_x;
    double geo_x;
    double geo_y;
    rl2PixelPtr nd = NULL;
    rl2RasterPtr raster_out = NULL;
    rl2RasterPtr raster_in = NULL;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    int pixel_sz = 1;
    int out_sz;
    int mask_sz = 0;

    if (pyr == NULL)
	goto error;
    tic_x = tileWidth / pyr->scale;
    tic_y = tileHeight / pyr->scale;
    geo_x = (double) tic_x *pyr->res_x;
    geo_y = (double) tic_y *pyr->res_y;

    switch (sample_type)
      {
      case RL2_SAMPLE_UINT16:
	  pixel_sz = 2;
	  break;
      };
    out_sz = tileWidth * tileHeight * pixel_sz * num_bands;

    tile_out = pyr->first_out;
    while (tile_out != NULL)
      {
	  rl2PrivRasterPtr rst;
	  /* allocating the output buffer */
	  buf_out = malloc (out_sz);
	  if (buf_out == NULL)
	      goto error;
	  rl2_prime_void_tile (buf_out, tileWidth, tileHeight, sample_type,
			       num_bands, no_data);

	  if (tile_out->col + tileWidth > pyr->scaled_width
	      || tile_out->row + tileHeight > pyr->scaled_height)
	    {
		/* allocating and initializing a transparency mask */
		unsigned char *p;
		mask_sz = tileWidth * tileHeight;
		mask = malloc (mask_sz);
		if (mask == NULL)
		    goto error;
		p = mask;
		for (row = 0; row < tileHeight; row++)
		  {
		      int x_row = tile_out->row + row;
		      for (col = 0; col < tileWidth; col++)
			{
			    int x_col = tile_out->col + col;
			    if (x_row >= pyr->scaled_height
				|| x_col >= pyr->scaled_width)
			      {
				  /* masking any portion of the tile exceeding the scaled section size */
				  *p++ = 0;
			      }
			    else
				*p++ = 1;
			}
		  }
	    }
	  else
	    {
		mask = NULL;
		mask_sz = 0;
	    }

	  /* creating the output (rescaled) tile */
	  tile_in = tile_out->first;
	  while (tile_in != NULL)
	    {
		/* loading and rescaling the base tiles */
		raster_in =
		    load_tile_base_generic (stmt_rd, tile_in->child->tile_id);
		if (raster_in == NULL)
		    goto error;
		pos_y = tile_out->maxy;
		x = 0;
		y = 0;
		for (row = 0; row < tileHeight; row += tic_y)
		  {
		      pos_x = tile_out->minx;
		      for (col = 0; col < tileWidth; col += tic_x)
			{
			    if (tile_in->child->cy < pos_y
				&& tile_in->child->cy > (pos_y - geo_y)
				&& tile_in->child->cx > pos_x
				&& tile_in->child->cx < (pos_x + geo_x))
			      {
				  x = col;
				  y = row;
				  break;
			      }
			    pos_x += geo_x;
			}
		      pos_y -= geo_y;
		  }
		rst = (rl2PrivRasterPtr) raster_in;
		rescale_multiband (buf_out, tileWidth, tileHeight,
				   rst->rasterBuffer, sample_type, num_bands, x,
				   y, tic_x, tic_y, no_data);
		rl2_destroy_raster (raster_in);
		raster_in = NULL;
		tile_in = tile_in->next;
	    }

	  raster_out = NULL;
	  raster_out =
	      rl2_create_raster (tileWidth, tileHeight, sample_type,
				 RL2_PIXEL_MULTIBAND, num_bands, buf_out,
				 out_sz, NULL, mask, mask_sz, nd);
	  buf_out = NULL;
	  mask = NULL;
	  if (raster_out == NULL)
	    {
		fprintf (stderr, "ERROR: unable to create a Pyramid Tile\n");
		goto error;
	    }
	  if (rl2_raster_encode
	      (raster_out, compression, &blob_odd, &blob_odd_sz, &blob_even,
	       &blob_even_sz, 100, 1) != RL2_OK)
	    {
		fprintf (stderr, "ERROR: unable to encode a Pyramid tile\n");
		goto error;
	    }
	  rl2_destroy_raster (raster_out);
	  raster_out = NULL;

	  /* INSERTing the tile */
	  if (!do_insert_pyramid_tile
	      (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz, id_level,
	       pyr->section_id, pyr->srid, tile_out->minx, tile_out->miny,
	       tile_out->maxx, tile_out->maxy, stmt_tils, stmt_data))
	      goto error;

	  tile_out = tile_out->next;
      }

    return 1;

  error:
    if (raster_in != NULL)
	rl2_destroy_raster (raster_in);
    if (raster_out != NULL)
	rl2_destroy_raster (raster_out);
    if (buf_out != NULL)
	free (buf_out);
    if (mask != NULL)
	free (mask);
    return 0;
}

static int
upate_sect_pyramid (sqlite3 * handle, sqlite3_stmt * stmt_rd,
		    sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_data,
		    SectionPyramid * pyr, unsigned short tileWidth,
		    unsigned short tileHeight, int id_level,
		    rl2PalettePtr palette, rl2PixelPtr no_data,
		    unsigned char pixel_type)
{
/* creating and inserting Pyramid tiles */
    unsigned char *buf_in;
    SectionPyramidTileOutPtr tile_out;
    SectionPyramidTileRefPtr tile_in;
    rl2GraphicsBitmapPtr base_tile;
    rl2GraphicsContextPtr ctx = NULL;
    int x;
    int y;
    int row;
    int col;
    int tic_x;
    int tic_y;
    double pos_y;
    double pos_x;
    double geo_x;
    double geo_y;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    rl2PixelPtr nd = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    unsigned char *p;
    unsigned char compression = RL2_COMPRESSION_NONE;

    if (pyr == NULL)
	goto error;
    tic_x = tileWidth / pyr->scale;
    tic_y = tileHeight / pyr->scale;
    geo_x = (double) tic_x *pyr->res_x;
    geo_y = (double) tic_y *pyr->res_y;

    tile_out = pyr->first_out;
    while (tile_out != NULL)
      {
	  /* creating the output (rescaled) tile */
	  ctx = rl2_graph_create_context (tileWidth, tileHeight);
	  if (ctx == NULL)
	      goto error;
	  tile_in = tile_out->first;
	  while (tile_in != NULL)
	    {
		/* loading and rescaling the base tiles */
		buf_in =
		    load_tile_base (stmt_rd, tile_in->child->tile_id, palette,
				    pixel_type);
		if (buf_in == NULL)
		    goto error;
		base_tile =
		    rl2_graph_create_bitmap (buf_in, tileWidth, tileHeight);
		if (base_tile == NULL)
		  {
		      free (buf_in);
		      goto error;
		  }
		pos_y = tile_out->maxy;
		x = 0;
		y = 0;
		for (row = 0; row < tileHeight; row += tic_y)
		  {
		      pos_x = tile_out->minx;
		      for (col = 0; col < tileWidth; col += tic_x)
			{
			    if (tile_in->child->cy < pos_y
				&& tile_in->child->cy > (pos_y - geo_y)
				&& tile_in->child->cx > pos_x
				&& tile_in->child->cx < (pos_x + geo_x))
			      {
				  x = col;
				  y = row;
				  break;
			      }
			    pos_x += geo_x;
			}
		      pos_y -= geo_y;
		  }
		rl2_graph_draw_rescaled_bitmap (ctx, base_tile,
						1.0 / pyr->scale,
						1.0 / pyr->scale, x, y);
		rl2_graph_destroy_bitmap (base_tile);
		tile_in = tile_in->next;
	    }

	  rgb = rl2_graph_get_context_rgb_array (ctx);
	  if (rgb == NULL)
	      goto error;
	  alpha = rl2_graph_get_context_alpha_array (ctx);
	  if (alpha == NULL)
	      goto error;
	  p = alpha;
	  for (row = 0; row < tileHeight; row++)
	    {
		int x_row = tile_out->row + row;
		for (col = 0; col < tileWidth; col++)
		  {
		      int x_col = tile_out->col + col;
		      if (x_row >= pyr->scaled_height
			  || x_col >= pyr->scaled_width)
			{
			    /* masking any portion of the tile exceeding the scaled section size */
			    *p++ = 0;
			}
		      else
			  *p++ = 1;
		  }
	    }

	  raster = NULL;
	  if (pyr->pixel_type == RL2_PIXEL_GRAYSCALE
	      || pyr->pixel_type == RL2_PIXEL_MONOCHROME)
	    {
		/* Grayscale Pyramid */
		unsigned char *p_in;
		unsigned char *p_out;
		unsigned char *gray = malloc (tileWidth * tileHeight);
		if (gray == NULL)
		    goto error;
		p_in = rgb;
		p_out = gray;
		for (row = 0; row < tileHeight; row++)
		  {
		      for (col = 0; col < tileWidth; col++)
			{
			    *p_out++ = *p_in++;
			    p_in += 2;
			}
		  }
		free (rgb);
		if (pyr->pixel_type == RL2_PIXEL_MONOCHROME)
		  {
		      if (no_data == NULL)
			  nd = NULL;
		      else
			{
			    /* converting the NO-DATA pixel */
			    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;
			    rl2PrivSamplePtr sample = pxl->Samples + 0;
			    nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
						   RL2_PIXEL_GRAYSCALE, 1);
			    if (sample->uint8 == 0)
				rl2_set_pixel_sample_uint8 (nd,
							    RL2_GRAYSCALE_BAND,
							    255);
			    else
				rl2_set_pixel_sample_uint8 (nd,
							    RL2_GRAYSCALE_BAND,
							    0);
			}
		      compression = RL2_COMPRESSION_PNG;
		  }
		else
		  {
		      nd = rl2_clone_pixel (no_data);
		      compression = RL2_COMPRESSION_JPEG;
		  }
		raster =
		    rl2_create_raster (tileWidth, tileHeight, RL2_SAMPLE_UINT8,
				       RL2_PIXEL_GRAYSCALE, 1, gray,
				       tileWidth * tileHeight, NULL, alpha,
				       tileWidth * tileHeight, nd);
	    }
	  else if (pyr->pixel_type == RL2_PIXEL_RGB)
	    {
		/* RGB Pyramid */
		nd = rl2_clone_pixel (no_data);
		raster =
		    rl2_create_raster (tileWidth, tileHeight, RL2_SAMPLE_UINT8,
				       RL2_PIXEL_RGB, 3, rgb,
				       tileWidth * tileHeight * 3, NULL, alpha,
				       tileWidth * tileHeight, nd);
		compression = RL2_COMPRESSION_JPEG;
	    }
	  if (raster == NULL)
	    {
		fprintf (stderr, "ERROR: unable to create a Pyramid Tile\n");
		goto error;
	    }
	  if (rl2_raster_encode
	      (raster, compression, &blob_odd, &blob_odd_sz, &blob_even,
	       &blob_even_sz, 80, 1) != RL2_OK)
	    {
		fprintf (stderr, "ERROR: unable to encode a Pyramid tile\n");
		goto error;
	    }
	  rl2_destroy_raster (raster);
	  raster = NULL;
	  rl2_graph_destroy_context (ctx);
	  ctx = NULL;

	  /* INSERTing the tile */
	  if (!do_insert_pyramid_tile
	      (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz, id_level,
	       pyr->section_id, pyr->srid, tile_out->minx, tile_out->miny,
	       tile_out->maxx, tile_out->maxy, stmt_tils, stmt_data))
	      goto error;

	  tile_out = tile_out->next;
      }

    return 1;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (ctx != NULL)
	rl2_graph_destroy_context (ctx);
    return 0;
}

static int
prepare_section_pyramid_stmts (sqlite3 * handle, const char *coverage,
			       sqlite3_stmt ** xstmt_rd,
			       sqlite3_stmt ** xstmt_levl,
			       sqlite3_stmt ** xstmt_tils,
			       sqlite3_stmt ** xstmt_data)
{
/* preparing the section pyramid related SQL statements */
    char *table;
    char *xtable;
    char *table_tile_data;
    char *xtable_tile_data;
    char *sql;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;

    *xstmt_rd = NULL;
    *xstmt_levl = NULL;
    *xstmt_tils = NULL;
    *xstmt_data = NULL;

    table_tile_data = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable_tile_data = gaiaDoubleQuotedSql (table_tile_data);
    sqlite3_free (table_tile_data);
    sql = sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even "
			   "FROM \"%s\" WHERE tile_id = ?", xtable_tile_data);
    free (xtable_tile_data);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_rd, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_levels", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT OR IGNORE INTO \"%s\" (pyramid_level, "
	 "x_resolution_1_1, y_resolution_1_1, "
	 "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	 "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	 "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO levels SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tils, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, tile_data_odd, tile_data_even) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tile_data SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    *xstmt_rd = stmt_rd;
    *xstmt_levl = stmt_levl;
    *xstmt_tils = stmt_tils;
    *xstmt_data = stmt_data;
    return 1;

  error:
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    return 0;
}

static int
do_build_section_pyramid (sqlite3 * handle, const char *coverage,
			  const char *section, unsigned char sample_type,
			  unsigned char pixel_type, unsigned char num_samples,
			  unsigned char compression, int quality, int srid,
			  unsigned short tileWidth, unsigned short tileHeight)
{
/* attempting to (re)build a section pyramid from scratch */
    char *table_levels;
    char *xtable_levels;
    char *table_tiles;
    char *xtable_tiles;
    char *sql;
    int id_level = 0;
    double new_res_x;
    double new_res_y;
    sqlite3_int64 sect_id;
    unsigned short sect_width;
    unsigned short sect_height;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned short row;
    unsigned short col;
    double out_minx;
    double out_miny;
    double out_maxx;
    double out_maxy;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    SectionPyramid *pyr = NULL;
    int ret;
    int first;
    int scale;
    rl2PalettePtr palette = NULL;
    rl2PixelPtr no_data = NULL;

    if (!get_section_infos
	(handle, coverage, section, &sect_id, &sect_width, &sect_height, &minx,
	 &miny, &maxx, &maxy, &palette, &no_data))
	goto error;

    if (!prepare_section_pyramid_stmts
	(handle, coverage, &stmt_rd, &stmt_levl, &stmt_tils, &stmt_data))
	goto error;

    while (1)
      {
	  /* looping on pyramid levels */
	  table_levels = sqlite3_mprintf ("%s_levels", coverage);
	  xtable_levels = gaiaDoubleQuotedSql (table_levels);
	  sqlite3_free (table_levels);
	  table_tiles = sqlite3_mprintf ("%s_tiles", coverage);
	  xtable_tiles = gaiaDoubleQuotedSql (table_tiles);
	  sqlite3_free (table_tiles);
	  sql =
	      sqlite3_mprintf ("SELECT l.x_resolution_1_1, l.y_resolution_1_1, "
			       "t.tile_id, MbrMinX(t.geometry), MbrMinY(t.geometry), "
			       "MbrMaxX(t.geometry), MbrMaxY(t.geometry) "
			       "FROM \"%s\" AS l "
			       "JOIN \"%s\" AS t ON (l.pyramid_level = t.pyramid_level) "
			       "WHERE l.pyramid_level = %d AND t.section_id = %d",
			       xtable_levels, xtable_tiles, id_level, sect_id);
	  free (xtable_levels);
	  free (xtable_tiles);
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
	  first = 1;
	  while (1)
	    {
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      double res_x = sqlite3_column_double (stmt, 0);
		      double res_y = sqlite3_column_double (stmt, 1);
		      sqlite3_int64 tile_id = sqlite3_column_int64 (stmt, 2);
		      double tminx = sqlite3_column_double (stmt, 3);
		      double tminy = sqlite3_column_double (stmt, 4);
		      double tmaxx = sqlite3_column_double (stmt, 5);
		      double tmaxy = sqlite3_column_double (stmt, 6);
		      if (id_level == 0)
			{
			    if (sample_type == RL2_SAMPLE_1_BIT
				|| sample_type == RL2_SAMPLE_2_BIT
				|| sample_type == RL2_SAMPLE_4_BIT)
			      {
				  new_res_x = res_x * 2.0;
				  new_res_y = res_y * 2.0;
				  scale = 2;
			      }
			    else
			      {
				  new_res_x = res_x * 8.0;
				  new_res_y = res_y * 8.0;
				  scale = 8;
			      }
			}
		      else
			{
			    new_res_x = res_x * 8.0;
			    new_res_y = res_y * 8.0;
			    scale = 8;
			}
		      if (first)
			{
			    pyr =
				alloc_sect_pyramid (sect_id, sect_width,
						    sect_height, sample_type,
						    pixel_type, num_samples,
						    compression, quality, srid,
						    new_res_x, new_res_y,
						    (double) tileWidth *
						    new_res_x,
						    (double) tileHeight *
						    new_res_y, minx, miny, maxx,
						    maxy, scale);
			    first = 0;
			    if (pyr == NULL)
				goto error;
			}
		      if (!insert_tile_into_section_pyramid
			  (pyr, tile_id, tminx, tminy, tmaxx, tmaxy))
			  goto error;
		  }
		else
		  {
		      fprintf (stderr,
			       "SELECT base level; sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
	    }
	  sqlite3_finalize (stmt);
	  stmt = NULL;
	  if (pyr == NULL)
	      goto error;

	  out_maxy = maxy;
	  for (row = 0; row < pyr->scaled_height; row += tileHeight)
	    {
		out_miny = out_maxy - pyr->tile_height;
		out_minx = minx;
		for (col = 0; col < pyr->scaled_width; col += tileWidth)
		  {
		      out_maxx = out_minx + pyr->tile_width;
		      set_pyramid_tile_destination (pyr, out_minx, out_miny,
						    out_maxx, out_maxy, row,
						    col);
		      out_minx += pyr->tile_width;
		  }
		out_maxy -= pyr->tile_height;
	    }
	  id_level++;
	  if (pyr->scaled_width <= tileWidth
	      && pyr->scaled_height <= tileHeight)
	      break;
	  if (!do_insert_pyramid_levels
	      (handle, id_level, pyr->res_x, pyr->res_y, stmt_levl))
	      goto error;
	  if (pixel_type == RL2_PIXEL_DATAGRID)
	    {
		/* DataGrid Pyramid */
		if (!upate_sect_pyramid_grid
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, no_data, sample_type, compression))
		    goto error;
	    }
	  else if (pixel_type == RL2_PIXEL_MULTIBAND)
	    {
		/* MultiBand Pyramid */
		if (!upate_sect_pyramid_multiband
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, no_data, sample_type, num_samples,
		     compression))
		    goto error;
	    }
	  else
	    {
		/* any other Pyramid [not a DataGrid] */
		if (!upate_sect_pyramid
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, palette, no_data, pixel_type))
		    goto error;
	    }
	  delete_sect_pyramid (pyr);
	  pyr = NULL;
      }

    if (pyr != NULL)
      {
	  if (!do_insert_pyramid_levels
	      (handle, id_level, pyr->res_x, pyr->res_y, stmt_levl))
	      goto error;
	  if (pixel_type == RL2_PIXEL_DATAGRID)
	    {
		/* DataGrid Pyramid */
		if (!upate_sect_pyramid_grid
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, no_data, sample_type, compression))
		    goto error;
	    }
	  else if (pixel_type == RL2_PIXEL_MULTIBAND)
	    {
		/* MultiBand Pyramid */
		if (!upate_sect_pyramid_multiband
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, no_data, sample_type, num_samples,
		     compression))
		    goto error;
	    }
	  else
	    {
		/* any other Pyramid [not a DataGrid] */
		if (!upate_sect_pyramid
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, palette, no_data, pixel_type))
		    goto error;
	    }
	  delete_sect_pyramid (pyr);
      }
    sqlite3_finalize (stmt_rd);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (pyr != NULL)
	delete_sect_pyramid (pyr);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return 0;
}

static int
find_base_resolution (sqlite3 * handle, const char *coverage,
		      double *x_res, double *y_res)
{
/* attempting to identify the base resolution level */
    int ret;
    int found = 0;
    double xx_res;
    double yy_res;
    char *xcoverage;
    char *xxcoverage;
    char *sql;
    sqlite3_stmt *stmt = NULL;

    xcoverage = sqlite3_mprintf ("%s_levels", coverage);
    xxcoverage = gaiaDoubleQuotedSql (xcoverage);
    sqlite3_free (xcoverage);
    sql =
	sqlite3_mprintf ("SELECT x_resolution_1_1, y_resolution_1_1 "
			 "FROM \"%s\" WHERE pyramid_level = 0", xxcoverage);
    free (xxcoverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_free (sql);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 1) == SQLITE_FLOAT)
		  {
		      found = 1;
		      xx_res = sqlite3_column_double (stmt, 0);
		      yy_res = sqlite3_column_double (stmt, 1);
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (found)
      {
	  *x_res = xx_res;
	  *y_res = yy_res;
	  return 1;
      }
    return 0;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
get_section_raw_raster_data (sqlite3 * handle, const char *coverage,
			     sqlite3_int64 sect_id, unsigned short width,
			     unsigned short height, unsigned char sample_type,
			     unsigned char pixel_type, unsigned char num_bands,
			     double minx, double maxy,
			     double x_res, double y_res, unsigned char **buffer,
			     int *buf_size, rl2PalettePtr palette,
			     rl2PixelPtr no_data)
{
/* attempting to return a buffer containing raw pixels from the whole DBMS Section */
    unsigned char *bufpix = NULL;
    int bufpix_size;
    char *xtiles;
    char *xxtiles;
    char *xdata;
    char *xxdata;
    char *sql;
    sqlite3_stmt *stmt_tiles = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;

    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
	  break;
      case RL2_SAMPLE_UINT8:
	  if (pixel_type != RL2_PIXEL_PALETTE)
	      goto error;
	  break;
      default:
	  goto error;
	  break;
      };
    bufpix_size = num_bands * width * height;
    bufpix = malloc (bufpix_size);
    if (bufpix == NULL)
      {
	  fprintf (stderr,
		   "get_section_raw_raster_data: Insufficient Memory !!!\n");
	  goto error;
      }
    memset (bufpix, 0, bufpix_size);

/* preparing the "tiles" SQL query */
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = gaiaDoubleQuotedSql (xtiles);
    sql =
	sqlite3_mprintf ("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry) "
			 "FROM \"%s\" "
			 "WHERE pyramid_level = 0 AND section_id = ?", xxtiles);
    sqlite3_free (xtiles);
    free (xxtiles);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tiles, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT section raw tiles SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* preparing the data SQL query - both ODD and EVEN */
    xdata = sqlite3_mprintf ("%s_tile_data", coverage);
    xxdata = gaiaDoubleQuotedSql (xdata);
    sqlite3_free (xdata);
    sql = sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even "
			   "FROM \"%s\" WHERE tile_id = ?", xxdata);
    free (xxdata);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT section raw tiles data(2) SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* preparing a raw pixels buffer */
    if (pixel_type == RL2_PIXEL_PALETTE)
	void_raw_buffer_palette (bufpix, width, height, no_data);
    else
	void_raw_buffer (bufpix, width, height, sample_type, num_bands,
			 no_data);
    if (!load_dbms_tiles_section
	(handle, sect_id, stmt_tiles, stmt_data, bufpix, width, height,
	 sample_type, num_bands, x_res, y_res, minx, maxy, RL2_SCALE_1, palette,
	 no_data))
	goto error;
    sqlite3_finalize (stmt_tiles);
    sqlite3_finalize (stmt_data);
    *buffer = bufpix;
    *buf_size = bufpix_size;
    return 1;

  error:
    if (stmt_tiles != NULL)
	sqlite3_finalize (stmt_tiles);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (bufpix != NULL)
	free (bufpix);
    return 0;
}

#define floor2(exp) ((long) exp)

static void
raster_tile_124_rescaled (unsigned char *outbuf,
			  unsigned char pixel_type, const unsigned char *inbuf,
			  unsigned int section_width,
			  unsigned int section_height, unsigned int out_width,
			  unsigned int out_height, rl2PalettePtr palette)
{
/* 
/ this function builds an high quality rescaled sub-image by applying pixel interpolation
/
/ this code is widely inspired by the original GD gdImageCopyResampled() function
*/
    unsigned int x;
    unsigned int y;
    double sy1;
    double sy2;
    double sx1;
    double sx2;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    const unsigned char *p_in;
    unsigned char *p_out;

    if (pixel_type == RL2_PIXEL_PALETTE && palette == NULL)
	return;

    for (y = 0; y < out_height; y++)
      {
	  sy1 = ((double) y) * (double) section_height / (double) out_height;
	  sy2 =
	      ((double) (y + 1)) * (double) section_height /
	      (double) out_height;
	  for (x = 0; x < out_width; x++)
	    {
		double sx;
		double sy;
		double spixels = 0;
		double red = 0.0;
		double green = 0.0;
		double blue = 0.0;
		sx1 =
		    ((double) x) * (double) section_width / (double) out_width;
		sx2 =
		    ((double) (x + 1)) * (double) section_width /
		    (double) out_width;
		sy = sy1;
		do
		  {
		      double yportion;
		      if (floor2 (sy) == floor2 (sy1))
			{
			    yportion = 1.0 - (sy - floor2 (sy));
			    if (yportion > sy2 - sy1)
			      {
				  yportion = sy2 - sy1;
			      }
			    sy = floor2 (sy);
			}
		      else if (sy == floor2 (sy2))
			{
			    yportion = sy2 - floor2 (sy2);
			}
		      else
			{
			    yportion = 1.0;
			}
		      sx = sx1;
		      do
			{
			    double xportion;
			    double pcontribution;
			    if (floor2 (sx) == floor2 (sx1))
			      {
				  xportion = 1.0 - (sx - floor2 (sx));
				  if (xportion > sx2 - sx1)
				    {
					xportion = sx2 - sx1;
				    }
				  sx = floor2 (sx);
			      }
			    else if (sx == floor2 (sx2))
			      {
				  xportion = sx2 - floor2 (sx2);
			      }
			    else
			      {
				  xportion = 1.0;
			      }
			    pcontribution = xportion * yportion;
			    /* retrieving the origin pixel */
			    if (pixel_type == RL2_PIXEL_RGB)
				p_in =
				    inbuf +
				    ((unsigned int) sy * section_width * 3);
			    else
				p_in =
				    inbuf + ((unsigned int) sy * section_width);
			    if (pixel_type == RL2_PIXEL_PALETTE)
			      {
				  rl2PrivPalettePtr plt =
				      (rl2PrivPalettePtr) palette;
				  unsigned char index;
				  p_in += (unsigned int) sx;
				  index = *p_in;
				  if (index < plt->nEntries)
				    {
					/* resolving the Palette color by index */
					rl2PrivPaletteEntryPtr entry =
					    plt->entries + index;
					r = entry->red;
					g = entry->green;
					b = entry->red;
				    }
				  else
				    {
					r = 0;
					g = 0;
					b = 0;
				    }
			      }
			    else
			      {
				  p_in += (unsigned int) sx;
				  if (*p_in == 1)
				    {
					/* black monochrome pixel */
					r = 0;
					g = 0;
					b = 0;
				    }
				  else
				    {
					/* white monochrome pixel */
					r = 255;
					g = 255;
					b = 255;
				    }
			      }
			    red += r * pcontribution;
			    green += g * pcontribution;
			    blue += b * pcontribution;
			    spixels += xportion * yportion;
			    sx += 1.0;
			}
		      while (sx < sx2);
		      sy += 1.0;
		  }
		while (sy < sy2);
		if (spixels != 0.0)
		  {
		      red /= spixels;
		      green /= spixels;
		      blue /= spixels;
		  }
		if (red > 255.0)
		    red = 255.0;
		if (green > 255.0)
		    green = 255.0;
		if (blue > 255.0)
		    blue = 255.0;
		/* setting the destination pixel */
		if (pixel_type == RL2_PIXEL_PALETTE)
		    p_out = outbuf + (y * out_width * 3);
		else
		    p_out = outbuf + (y * out_width);
		if (pixel_type == RL2_PIXEL_PALETTE)
		  {
		      p_out += x * 3;
		      *p_out++ = (unsigned char) red;
		      *p_out++ = (unsigned char) green;
		      *p_out = (unsigned char) blue;
		  }
		else
		  {
		      p_out += x;
		      if (red < *p_out)
			  *p_out = (unsigned char) red;
		  }
	    }
      }
}

static int
copy_124_tile (unsigned char pixel_type, const unsigned char *outbuf,
	       unsigned char **tilebuf,
	       int *tilebuf_sz, unsigned char **tilemask, int *tilemask_sz,
	       unsigned int row, unsigned int col, unsigned int out_width,
	       unsigned int out_height, unsigned int tileWidth,
	       unsigned int tileHeight, rl2PixelPtr no_data)
{
/* allocating and initializing the resized tile buffers */
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_sz = 0;
    int has_mask = 0;
    unsigned int x;
    unsigned int y;
    const unsigned char *p_inp;
    unsigned char *p_outp;

    if (pixel_type == RL2_PIXEL_RGB)
	pixels_sz = tileWidth * tileHeight * 3;
    else
	pixels_sz = tileWidth * tileHeight;
    pixels = malloc (pixels_sz);
    if (pixels == NULL)
	goto error;

    if (pixel_type == RL2_PIXEL_RGB)
	rl2_prime_void_tile (pixels, tileWidth, tileHeight, RL2_SAMPLE_UINT8,
			     3, no_data);
    else
	rl2_prime_void_tile (pixels, tileWidth, tileHeight, RL2_SAMPLE_UINT8,
			     1, no_data);

    if (row + tileHeight > out_height)
	has_mask = 1;
    if (col + tileWidth > out_width)
	has_mask = 1;
    if (has_mask)
      {
	  /* masking the unused portion of this tile */
	  mask_sz = tileWidth * tileHeight;
	  mask = malloc (mask_sz);
	  if (mask == NULL)
	      goto error;
	  memset (mask, 0, mask_sz);
	  for (y = 0; y < tileHeight; y++)
	    {
		unsigned int y_in = y + row;
		if (y_in >= out_height)
		    continue;
		for (x = 0; x < tileWidth; x++)
		  {
		      unsigned int x_in = x + col;
		      if (x_in >= out_width)
			  continue;
		      p_outp = mask + (y * tileWidth);
		      p_outp += x;
		      *p_outp = 1;
		  }
	    }
      }

    for (y = 0; y < tileHeight; y++)
      {
	  unsigned int y_in = y + row;
	  if (y_in >= out_height)
	      continue;
	  for (x = 0; x < tileWidth; x++)
	    {
		unsigned int x_in = x + col;
		if (x_in >= out_width)
		    continue;
		if (pixel_type == RL2_PIXEL_RGB)
		  {
		      p_inp = outbuf + (y_in * out_width * 3);
		      p_inp += x_in * 3;
		      p_outp = pixels + (y * tileWidth * 3);
		      p_outp += x * 3;
		      *p_outp++ = *p_inp++;
		      *p_outp++ = *p_inp++;
		      *p_outp = *p_inp;
		  }
		else
		  {
		      p_inp = outbuf + (y_in * out_width);
		      p_inp += x_in;
		      p_outp = pixels + (y * tileWidth);
		      p_outp += x;
		      *p_outp = *p_inp;
		  }
	    }
      }

    *tilebuf = pixels;
    *tilebuf_sz = pixels_sz;
    *tilemask = mask;
    *tilemask_sz = mask_sz;
    return 1;

  error:
    if (pixels != NULL)
	free (pixels);
    return 0;
}

static int
do_build_124_bit_section_pyramid (sqlite3 * handle, const char *coverage,
				  const char *section,
				  unsigned char sample_type,
				  unsigned char pixel_type,
				  unsigned char num_samples, int srid,
				  unsigned short tileWidth,
				  unsigned short tileHeight,
				  unsigned char bgRed, unsigned char bgGreen,
				  unsigned char bgBlue)
{
/* attempting to (re)build a 1,2,4-bit section pyramid from scratch */
    double base_res_x;
    double base_res_y;
    sqlite3_int64 sect_id;
    unsigned short sect_width;
    unsigned short sect_height;
    int id_level = 0;
    int scale;
    double x_res;
    double y_res;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned int row;
    unsigned int col;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    rl2PalettePtr palette = NULL;
    rl2PixelPtr no_data = NULL;
    rl2PixelPtr nd = NULL;
    unsigned char *inbuf = NULL;
    int inbuf_size = 0;
    unsigned char *outbuf = NULL;
    int outbuf_sz = 0;
    unsigned char out_pixel_type;
    unsigned char out_bands;
    unsigned char *tilebuf = NULL;
    int tilebuf_sz = 0;
    unsigned char *tilemask = NULL;
    int tilemask_sz = 0;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    unsigned int x;
    unsigned int y;
    unsigned char *p_out;

    if (!get_section_infos
	(handle, coverage, section, &sect_id, &sect_width, &sect_height, &minx,
	 &miny, &maxx, &maxy, &palette, &no_data))
	goto error;

    if (!find_base_resolution (handle, coverage, &base_res_x, &base_res_y))
	goto error;
    if (!get_section_raw_raster_data
	(handle, coverage, sect_id, sect_width, sect_height, sample_type,
	 pixel_type, num_samples, minx, maxy, base_res_x,
	 base_res_y, &inbuf, &inbuf_size, palette, no_data))
	goto error;

    if (!prepare_section_pyramid_stmts
	(handle, coverage, &stmt_rd, &stmt_levl, &stmt_tils, &stmt_data))
	goto error;

    id_level = 1;
    scale = 2;
    x_res = base_res_x * 2.0;
    y_res = base_res_y * 2.0;
    while (1)
      {
	  /* looping on Pyramid levels */
	  double t_minx;
	  double t_miny;
	  double t_maxx;
	  double t_maxy;
	  unsigned int out_width = sect_width / scale;
	  unsigned int out_height = sect_height / scale;
	  rl2RasterPtr raster = NULL;

	  if (pixel_type == RL2_PIXEL_MONOCHROME)
	    {
		out_pixel_type = RL2_PIXEL_GRAYSCALE;
		out_bands = 1;
		outbuf_sz = out_width * out_height;
		outbuf = malloc (outbuf_sz);
		if (outbuf == NULL)
		    goto error;
		p_out = outbuf;
		for (y = 0; y < out_height; y++)
		  {
		      /* priming the background color */
		      for (x = 0; x < out_width; x++)
			  *p_out++ = bgRed;
		  }
	    }
	  else
	    {
		out_pixel_type = RL2_PIXEL_RGB;
		out_bands = 3;
		outbuf_sz = out_width * out_height * 3;
		outbuf = malloc (outbuf_sz);
		if (outbuf == NULL)
		    goto error;
		p_out = outbuf;
		for (y = 0; y < out_height; y++)
		  {
		      /* priming the background color */
		      for (x = 0; x < out_width; x++)
			{
			    *p_out++ = bgRed;
			    *p_out++ = bgGreen;
			    *p_out++ = bgBlue;
			}
		  }
	    }
	  t_maxy = maxy;
	  raster_tile_124_rescaled (outbuf, pixel_type, inbuf,
				    sect_width, sect_height, out_width,
				    out_height, palette);

	  if (!do_insert_pyramid_levels
	      (handle, id_level, x_res, y_res, stmt_levl))
	      goto error;

	  for (row = 0; row < out_height; row += tileHeight)
	    {
		t_minx = minx;
		t_miny = t_maxy - (tileHeight * y_res);
		for (col = 0; col < out_width; col += tileWidth)
		  {
		      if (pixel_type == RL2_PIXEL_MONOCHROME)
			{
			    if (no_data == NULL)
				nd = NULL;
			    else
			      {
				  /* creating a NO-DATA pixel */
				  nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
							 RL2_PIXEL_GRAYSCALE,
							 1);
				  rl2_set_pixel_sample_uint8 (nd,
							      RL2_GRAYSCALE_BAND,
							      bgRed);
			      }
			}
		      else
			{
			    if (no_data == NULL)
				nd = NULL;
			    else
			      {
				  /* converting the NO-DATA pixel */
				  nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
							 RL2_PIXEL_RGB, 3);
				  rl2_set_pixel_sample_uint8 (nd, RL2_RED_BAND,
							      bgRed);
				  rl2_set_pixel_sample_uint8 (nd,
							      RL2_GREEN_BAND,
							      bgGreen);
				  rl2_set_pixel_sample_uint8 (nd, RL2_BLUE_BAND,
							      bgBlue);
			      }
			}
		      t_maxx = t_minx + (tileWidth * x_res);
		      if (!copy_124_tile
			  (out_pixel_type, outbuf, &tilebuf,
			   &tilebuf_sz, &tilemask, &tilemask_sz, row, col,
			   out_width, out_height, tileWidth, tileHeight,
			   no_data))
			{
			    fprintf (stderr,
				     "ERROR: unable to extract a Pyramid Tile\n");
			    goto error;
			}

		      raster =
			  rl2_create_raster (tileWidth, tileHeight,
					     RL2_SAMPLE_UINT8, out_pixel_type,
					     out_bands, tilebuf, tilebuf_sz,
					     NULL, tilemask, tilemask_sz, nd);
		      tilebuf = NULL;
		      tilemask = NULL;
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "ERROR: unable to create a Pyramid Tile\n");
			    goto error;
			}
		      if (rl2_raster_encode
			  (raster, RL2_COMPRESSION_PNG, &blob_odd, &blob_odd_sz,
			   &blob_even, &blob_even_sz, 100, 1) != RL2_OK)
			{
			    fprintf (stderr,
				     "ERROR: unable to encode a Pyramid tile\n");
			    goto error;
			}

		      /* INSERTing the tile */
		      if (!do_insert_pyramid_tile
			  (handle, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, id_level, sect_id, srid, t_minx,
			   t_miny, t_maxx, t_maxy, stmt_tils, stmt_data))
			  goto error;
		      rl2_destroy_raster (raster);
		      t_minx += (tileWidth * x_res);
		  }
		t_maxy -= (tileHeight * y_res);
	    }

	  free (outbuf);
	  if (out_width < tileWidth && out_height < tileHeight)
	      break;
	  x_res *= 4.0;
	  y_res *= 4.0;
	  scale *= 4;
	  id_level++;
      }

    free (inbuf);
    sqlite3_finalize (stmt_rd);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 1;

  error:
    if (outbuf != NULL)
	free (outbuf);
    if (tilebuf != NULL)
	free (tilebuf);
    if (tilemask != NULL)
	free (tilemask);
    if (inbuf != NULL)
	free (inbuf);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 0;
}

static int
do_build_palette_section_pyramid (sqlite3 * handle, const char *coverage,
				  const char *section, int srid,
				  unsigned short tileWidth,
				  unsigned short tileHeight,
				  unsigned char bgRed, unsigned char bgGreen,
				  unsigned char bgBlue)
{
/* attempting to (re)build a Palette section pyramid from scratch */
    double base_res_x;
    double base_res_y;
    sqlite3_int64 sect_id;
    unsigned short sect_width;
    unsigned short sect_height;
    int id_level = 0;
    int scale;
    double x_res;
    double y_res;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned int row;
    unsigned int col;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    rl2PalettePtr palette = NULL;
    rl2PixelPtr no_data = NULL;
    rl2PixelPtr nd = NULL;
    unsigned char *inbuf = NULL;
    int inbuf_size = 0;
    unsigned char *outbuf = NULL;
    int outbuf_sz = 0;
    unsigned char out_pixel_type;
    unsigned char out_bands;
    unsigned char *tilebuf = NULL;
    int tilebuf_sz = 0;
    unsigned char *tilemask = NULL;
    int tilemask_sz = 0;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    unsigned int x;
    unsigned int y;
    unsigned char *p_out;

    if (!get_section_infos
	(handle, coverage, section, &sect_id, &sect_width, &sect_height, &minx,
	 &miny, &maxx, &maxy, &palette, &no_data))
	goto error;
    if (palette == NULL)
	goto error;

    if (!find_base_resolution (handle, coverage, &base_res_x, &base_res_y))
	goto error;
    if (!get_section_raw_raster_data
	(handle, coverage, sect_id, sect_width, sect_height, RL2_SAMPLE_UINT8,
	 RL2_PIXEL_PALETTE, 1, minx, maxy, base_res_x,
	 base_res_y, &inbuf, &inbuf_size, palette, no_data))
	goto error;

    if (!prepare_section_pyramid_stmts
	(handle, coverage, &stmt_rd, &stmt_levl, &stmt_tils, &stmt_data))
	goto error;

    id_level = 1;
    scale = 2;
    x_res = base_res_x * 2.0;
    y_res = base_res_y * 2.0;
    while (1)
      {
	  /* looping on Pyramid levels */
	  double t_minx;
	  double t_miny;
	  double t_maxx;
	  double t_maxy;
	  unsigned int out_width = sect_width / scale;
	  unsigned int out_height = sect_height / scale;
	  rl2RasterPtr raster = NULL;

	  out_pixel_type = RL2_PIXEL_RGB;
	  out_bands = 3;
	  outbuf_sz = out_width * out_height * 3;
	  outbuf = malloc (outbuf_sz);
	  if (outbuf == NULL)
	      goto error;
	  p_out = outbuf;
	  for (y = 0; y < out_height; y++)
	    {
		/* priming the background color */
		for (x = 0; x < out_width; x++)
		  {
		      *p_out++ = bgRed;
		      *p_out++ = bgGreen;
		      *p_out++ = bgBlue;
		  }
	    }
	  t_maxy = maxy;
	  raster_tile_124_rescaled (outbuf, RL2_PIXEL_PALETTE, inbuf,
				    sect_width, sect_height, out_width,
				    out_height, palette);

	  if (!do_insert_pyramid_levels
	      (handle, id_level, x_res, y_res, stmt_levl))
	      goto error;

	  for (row = 0; row < out_height; row += tileHeight)
	    {
		t_minx = minx;
		t_miny = t_maxy - (tileHeight * y_res);
		for (col = 0; col < out_width; col += tileWidth)
		  {
		      if (no_data == NULL)
			  nd = NULL;
		      else
			{
			    /* converting the NO-DATA pixel */
			    nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
						   RL2_PIXEL_RGB, 3);
			    rl2_set_pixel_sample_uint8 (nd, RL2_RED_BAND,
							bgRed);
			    rl2_set_pixel_sample_uint8 (nd,
							RL2_GREEN_BAND,
							bgGreen);
			    rl2_set_pixel_sample_uint8 (nd, RL2_BLUE_BAND,
							bgBlue);
			}
		      t_maxx = t_minx + (tileWidth * x_res);
		      if (!copy_124_tile (out_pixel_type, outbuf, &tilebuf,
					  &tilebuf_sz, &tilemask, &tilemask_sz,
					  row, col, out_width, out_height,
					  tileWidth, tileHeight, no_data))
			{
			    fprintf (stderr,
				     "ERROR: unable to extract a Pyramid Tile\n");
			    goto error;
			}

		      raster =
			  rl2_create_raster (tileWidth, tileHeight,
					     RL2_SAMPLE_UINT8, out_pixel_type,
					     out_bands, tilebuf, tilebuf_sz,
					     NULL, tilemask, tilemask_sz, nd);
		      tilebuf = NULL;
		      tilemask = NULL;
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "ERROR: unable to create a Pyramid Tile\n");
			    goto error;
			}
		      if (rl2_raster_encode
			  (raster, RL2_COMPRESSION_PNG, &blob_odd, &blob_odd_sz,
			   &blob_even, &blob_even_sz, 100, 1) != RL2_OK)
			{
			    fprintf (stderr,
				     "ERROR: unable to encode a Pyramid tile\n");
			    goto error;
			}

		      /* INSERTing the tile */
		      if (!do_insert_pyramid_tile
			  (handle, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, id_level, sect_id, srid, t_minx,
			   t_miny, t_maxx, t_maxy, stmt_tils, stmt_data))
			  goto error;
		      rl2_destroy_raster (raster);
		      t_minx += (tileWidth * x_res);
		  }
		t_maxy -= (tileHeight * y_res);
	    }

	  free (outbuf);
	  if (out_width < tileWidth && out_height < tileHeight)
	      break;
	  x_res *= 4.0;
	  y_res *= 4.0;
	  scale *= 4;
	  id_level++;
      }

    free (inbuf);
    sqlite3_finalize (stmt_rd);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 1;

  error:
    if (outbuf != NULL)
	free (outbuf);
    if (tilebuf != NULL)
	free (tilebuf);
    if (tilemask != NULL)
	free (tilemask);
    if (inbuf != NULL)
	free (inbuf);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 0;
}

static void
get_background_color (sqlite3 * handle, rl2CoveragePtr coverage,
		      unsigned char *bgRed, unsigned char *bgGreen,
		      unsigned char *bgBlue)
{
/* retrieving the background color for 1,2,4 bit samples */
    sqlite3_stmt *stmt = NULL;
    char *sql;
    int ret;
    rl2PrivPalettePtr plt;
    rl2PalettePtr palette = NULL;
    rl2PrivSamplePtr smp;
    rl2PrivPixelPtr pxl;
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    unsigned char index;
    *bgRed = 255;
    *bgGreen = 255;
    *bgBlue = 255;

    if (cvg == NULL)
	return;
    if (cvg->noData == NULL)
	return;
    pxl = (rl2PrivPixelPtr) (cvg->noData);
    smp = pxl->Samples;
    index = smp->uint8;

    if (cvg->pixelType == RL2_PIXEL_MONOCHROME)
      {
	  if (index == 1)
	    {
		*bgRed = 0;
		*bgGreen = 0;
		*bgBlue = 0;
	    }
	  return;
      }

/* Coverage's palette */
    sql = sqlite3_mprintf ("SELECT palette FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)",
			   cvg->coverageName);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      palette = rl2_deserialize_dbms_palette (blob, blob_sz);
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_info; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (palette == NULL)
	goto error;

    plt = (rl2PrivPalettePtr) palette;
    if (index < plt->nEntries)
      {
	  rl2PrivPaletteEntryPtr rgb = plt->entries + index;
	  *bgRed = rgb->red;
	  *bgGreen = rgb->green;
	  *bgBlue = rgb->blue;
      }
    rl2_destroy_palette (palette);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (palette != NULL)
	rl2_destroy_palette (palette);
}

RL2_DECLARE int
rl2_build_section_pyramid (sqlite3 * handle, const char *coverage,
			   const char *section, int forced_rebuild)
{
/* (re)building section-level pyramid for a single Section */
    rl2CoveragePtr cvg = NULL;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    int quality;
    unsigned short tileWidth;
    unsigned short tileHeight;
    int srid;
    int build = 0;
    unsigned char bgRed;
    unsigned char bgGreen;
    unsigned char bgBlue;

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_compression (cvg, &compression, &quality) != RL2_OK)
	goto error;
    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;

    if (!forced_rebuild)
      {
	  /* checking if the section pyramid already exists */
	  build = check_section_pyramid (handle, coverage, section);
      }
    else
      {
	  /* unconditional rebuilding */
	  build = 1;
      }

    if (build)
      {
	  /* attempting to delete the section pyramid */
	  if (!delete_section_pyramid (handle, coverage, section))
	      goto error;
	  /* attempting to (re)build the section pyramid */
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	      || (sample_type == RL2_SAMPLE_1_BIT
		  && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1)
	      || (sample_type == RL2_SAMPLE_2_BIT
		  && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1)
	      || (sample_type == RL2_SAMPLE_4_BIT
		  && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* special case: 1,2,4 bit Pyramid */
		get_background_color (handle, cvg, &bgRed, &bgGreen, &bgBlue);
		if (!do_build_124_bit_section_pyramid
		    (handle, coverage, section, sample_type, pixel_type,
		     num_bands, srid, tileWidth, tileHeight, bgRed, bgGreen,
		     bgBlue))
		    goto error;
	    }
	  else if (sample_type == RL2_SAMPLE_UINT8
		   && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1)
	    {
		/* special case: 8 bit Palette Pyramid */
		get_background_color (handle, cvg, &bgRed, &bgGreen, &bgBlue);
		if (!do_build_palette_section_pyramid
		    (handle, coverage, section, srid, tileWidth, tileHeight,
		     bgRed, bgGreen, bgBlue))
		    goto error;
	    }
	  else
	    {
		/* ordinary RGB, Grayscale, MultiBand or DataGrid Pyramid */
		if (!do_build_section_pyramid
		    (handle, coverage, section, sample_type, pixel_type,
		     num_bands, compression, quality, srid, tileWidth,
		     tileHeight))
		    goto error;
	    }
	  printf ("  ----------\n");
	  printf ("    Pyramid levels successfully built for: %s\n", section);
      }
    rl2_destroy_coverage (cvg);

    return RL2_OK;

  error:
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_build_all_section_pyramids (sqlite3 * handle, const char *coverage,
				int forced_rebuild)
{
/* (re)building section-level pyramids for a whole Coverage */
    char *table;
    char *xtable;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT section_name FROM \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *section = results[(i * columns) + 0];
		if (rl2_build_section_pyramid
		    (handle, coverage, section, forced_rebuild) != RL2_OK)
		    goto error;
	    }
      }
    sqlite3_free_table (results);
    return RL2_OK;

  error:
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_delete_all_pyramids (sqlite3 * handle, const char *coverage)
{
/* deleting all pyramids for a whole Coverage */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    char *err_msg = NULL;

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("DELETE FROM \"%s\" WHERE pyramid_level > 0", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE FROM \"%s_tiles\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  return RL2_ERROR;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_delete_section_pyramid (sqlite3 * handle, const char *coverage,
			    const char *section)
{
/* deleting section-level pyramid for a single Section */
    if (!delete_section_pyramid (handle, coverage, section))
	return RL2_ERROR;
    return RL2_OK;
}
