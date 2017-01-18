/*

 test_map_orbetello.c -- RasterLite-2 Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"
#include "spatialite/gaiaaux.h"

#include "rasterlite2/rasterlite2.h"

#define TILE_256	256
#define TILE_512	512
#define TILE_1024	1024

static int
execute_check (sqlite3 * sqlite, const char *sql)
{
/* executing an SQL statement returning True/False */
    sqlite3_stmt *stmt;
    int ret;
    int retcode = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return SQLITE_ERROR;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    if (retcode == 1)
	return SQLITE_OK;
    return SQLITE_ERROR;
}

static int
execute_check_blob (sqlite3 * sqlite, const char *sql)
{
/* executing an SQL statement returning a BLOB */
    sqlite3_stmt *stmt;
    int ret;
    int retcode = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return SQLITE_ERROR;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    if (retcode == 1)
	return SQLITE_OK;
    return SQLITE_ERROR;
}

static int
get_max_tile_id (sqlite3 * sqlite, const char *coverage)
{
/* retriving the Max tile_id for a given Coverage */
    char *sql;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt;
    int ret;
    int max = 0;

    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Max(tile_id) FROM \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      max = sqlite3_column_int (stmt, 0);
      }
    sqlite3_finalize (stmt);
    return max;
}

static int
do_export_tile_image (sqlite3 * sqlite, const char *coverage, int tile_id,
		      int band_mix)
{
/* attempting to export a visible Tile */
    char *sql;
    char *path;
    int ret;
    int transparent = 1;
    unsigned char red_band = 2;
    unsigned char green_band = 1;
    unsigned char blue_band = 0;

    if (band_mix == 1)
      {
	  red_band = 3;
	  green_band = 1;
	  blue_band = 0;
      }
    if (band_mix == 2)
      {
	  red_band = 2;
	  green_band = 1;
	  blue_band = 3;
      }
    if (tile_id <= 1)
	transparent = 0;
    if (tile_id < 0)
	tile_id = get_max_tile_id (sqlite, coverage);
    path = sqlite3_mprintf ("./%s_tile_%d_%d.png", coverage, tile_id, band_mix);
    sql =
	sqlite3_mprintf
	("SELECT BlobToFile(RL2_GetTripleBandTileImage(%Q, %d, %d, %d, %d, '#e0ffe0', %d), %Q)",
	 coverage, tile_id, red_band, green_band, blue_band, transparent, path);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    unlink (path);
    sqlite3_free (path);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "ERROR: Unable to export an Image from \"%s\" tile_id=%d\n",
		   coverage, tile_id);
	  return 0;
      }
    return 1;
}

static int
do_export_mono_tile_image (sqlite3 * sqlite, const char *coverage, int tile_id,
			   int band_mix)
{
/* attempting to export a visible Tile (Mono-Band) */
    char *sql;
    char *path;
    int ret;
    int transparent = 1;
    unsigned char mono_band = 0;

    if (band_mix == 1)
	mono_band = 2;
    if (band_mix == 2)
	mono_band = 3;
    if (tile_id <= 1)
	transparent = 0;
    if (tile_id < 0)
	tile_id = get_max_tile_id (sqlite, coverage);
    path =
	sqlite3_mprintf ("./%s_mono_tile_%d_%d.png", coverage, tile_id,
			 band_mix);
    sql =
	sqlite3_mprintf
	("SELECT BlobToFile(RL2_GetMonoBandTileImage(%Q, %d, %d, '#e0ffe0', %d), %Q)",
	 coverage, tile_id, mono_band, transparent, path);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    unlink (path);
    sqlite3_free (path);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "ERROR: Unable to export an Image from \"%s\" tile_id=%d\n",
		   coverage, tile_id);
	  return 0;
      }
/* testing GetBandHistogramFromImage() */
    if (tile_id == 1)
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT RL2_GetBandHistogramFromImage(RL2_GetMonoBandTileImage(%Q, %d, %d, '#e0ffe0', %d), 'image/png', 0)",
	       coverage, tile_id, mono_band, transparent);
	  ret = execute_check_blob (sqlite, sql);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr,
			 "ERROR: Unable to test RL2_GetBandHistogramFromImage()\n");
		return 0;
	    }
      }
    return 1;
}

static int
get_base_resolution (sqlite3 * sqlite, const char *coverage, double *x_res,
		     double *y_res)
{
/* attempting to retrieve the Coverage's base resolution */
    char *sql;
    sqlite3_stmt *stmt;
    int ret;
    int ok = 0;

    sql = sqlite3_mprintf ("SELECT horz_resolution, vert_resolution "
			   "FROM raster_coverages WHERE coverage_name = %Q",
			   coverage);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		*x_res = sqlite3_column_double (stmt, 0);
		*y_res = sqlite3_column_double (stmt, 1);
		ok = 1;
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static int
do_export_geotiff (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		   int scale)
{
/* exporting a GeoTiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;

    path = sqlite3_mprintf ("./%s_gt_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteGeoTiff(?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 5, blob, blob_size, free);
    sqlite3_bind_double (stmt, 6, xx_res);
    sqlite3_bind_double (stmt, 7, yy_res);
    sqlite3_bind_int (stmt, 8, 0);
    sqlite3_bind_text (stmt, 9, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_band_composed_geotiff (sqlite3 * sqlite, const char *coverage,
				 gaiaGeomCollPtr geom, int scale, int band_mix,
				 int with_worldfile)
{
/* exporting a GeoTiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 2;
    unsigned char green_band = 1;
    unsigned char blue_band = 0;

    if (band_mix == 1)
      {
	  red_band = 3;
	  green_band = 1;
	  blue_band = 0;
      }
    if (band_mix == 2)
      {
	  red_band = 2;
	  green_band = 1;
	  blue_band = 3;
      }

    path = sqlite3_mprintf ("./%s_bc_gt_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql =
	"SELECT RL2_WriteTripleBandGeoTiff(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, red_band);
    sqlite3_bind_int (stmt, 6, green_band);
    sqlite3_bind_int (stmt, 7, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 8, blob, blob_size, free);
    sqlite3_bind_double (stmt, 9, xx_res);
    sqlite3_bind_double (stmt, 10, yy_res);
    sqlite3_bind_int (stmt, 11, with_worldfile);
    sqlite3_bind_text (stmt, 12, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path = sqlite3_mprintf ("./%s_bc_gt_%d_%d.tfw", coverage, scale, band_mix);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_band_composed_tiff (sqlite3 * sqlite, const char *coverage,
			      gaiaGeomCollPtr geom, int scale, int band_mix)
{
/* exporting a plain Tiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 2;
    unsigned char green_band = 1;
    unsigned char blue_band = 0;

    if (band_mix == 1)
      {
	  red_band = 3;
	  green_band = 1;
	  blue_band = 0;
      }
    if (band_mix == 2)
      {
	  red_band = 2;
	  green_band = 1;
	  blue_band = 3;
      }

    path = sqlite3_mprintf ("./%s_bc_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;
    sql = "SELECT RL2_WriteTripleBandTiff(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, red_band);
    sqlite3_bind_int (stmt, 6, green_band);
    sqlite3_bind_int (stmt, 7, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 8, blob, blob_size, free);
    sqlite3_bind_double (stmt, 9, xx_res);
    sqlite3_bind_double (stmt, 10, yy_res);
    sqlite3_bind_text (stmt, 11, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_band_composed_tiff_tfw (sqlite3 * sqlite, const char *coverage,
				  gaiaGeomCollPtr geom, int scale, int band_mix)
{
/* exporting a Tiff+TFW */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char red_band = 2;
    unsigned char green_band = 1;
    unsigned char blue_band = 0;

    if (band_mix == 1)
      {
	  red_band = 3;
	  green_band = 1;
	  blue_band = 0;
      }
    if (band_mix == 2)
      {
	  red_band = 2;
	  green_band = 1;
	  blue_band = 3;
      }

    path = sqlite3_mprintf ("./%s_bc_tfw_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteTripleBandTiffTfw(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, red_band);
    sqlite3_bind_int (stmt, 6, green_band);
    sqlite3_bind_int (stmt, 7, blue_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 8, blob, blob_size, free);
    sqlite3_bind_double (stmt, 9, xx_res);
    sqlite3_bind_double (stmt, 10, yy_res);
    sqlite3_bind_text (stmt, 11, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path = sqlite3_mprintf ("./%s_bc_tfw_%d_%d.tfw", coverage, scale, band_mix);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_mono_band_geotiff (sqlite3 * sqlite, const char *coverage,
			     gaiaGeomCollPtr geom, int scale, int band_mix,
			     int with_worldfile)
{
/* exporting a GeoTiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char mono_band = 0;

    if (band_mix == 1)
	mono_band = 1;
    if (band_mix == 2)
	mono_band = 3;

    path =
	sqlite3_mprintf ("./%s_mono_gt_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteMonoBandGeoTiff(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, mono_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_int (stmt, 9, with_worldfile);
    sqlite3_bind_text (stmt, 10, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path =
	sqlite3_mprintf ("./%s_mono_gt_%d_%d.tfw", coverage, scale, band_mix);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_mono_band_tiff (sqlite3 * sqlite, const char *coverage,
			  gaiaGeomCollPtr geom, int scale, int band_mix)
{
/* exporting a plain Tiff */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char mono_band = 0;

    if (band_mix == 1)
	mono_band = 1;
    if (band_mix == 2)
	mono_band = 3;

    path = sqlite3_mprintf ("./%s_mono_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteMonoBandTiff(?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, mono_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_text (stmt, 9, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_mono_band_tiff_tfw (sqlite3 * sqlite, const char *coverage,
			      gaiaGeomCollPtr geom, int scale, int band_mix)
{
/* exporting a Tiff+TFW */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    double x_res;
    double y_res;
    double xx_res;
    double yy_res;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    unsigned char mono_band = 0;

    if (band_mix == 1)
	mono_band = 1;
    if (band_mix == 2)
	mono_band = 3;

    path =
	sqlite3_mprintf ("./%s_mono_tfw_%d_%d.tif", coverage, scale, band_mix);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteMonoBandTiffTfw(?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, mono_band);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_text (stmt, 9, "NONE", 4, SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    path =
	sqlite3_mprintf ("./%s_mono_tfw_%d_%d.tfw", coverage, scale, band_mix);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_map_image (sqlite3 * sqlite, const char *coverage,
		     gaiaGeomCollPtr geom, const char *style,
		     const char *suffix)
{
/* exporting a Map Image (full rendered) */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    const char *format = "text/plain";

    if (strcmp (suffix, "png") == 0)
	format = "image/png";
    if (strcmp (suffix, "jpg") == 0)
	format = "image/jpeg";
    if (strcmp (suffix, "tif") == 0)
	format = "image/tiff";
    if (strcmp (suffix, "pdf") == 0)
	format = "application/x-pdf";

    path = sqlite3_mprintf ("./%s_map_%s.%s", coverage, style, suffix);

    sql =
	"SELECT BlobToFile(RL2_GetMapImage(?, ST_Buffer(?, 2000), ?, ?, ?, ?, ?, ?), ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_text (stmt, 5, style, strlen (style), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 6, format, strlen (format), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 7, "#ffe0e0", 7, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 8, 1);
    sqlite3_bind_text (stmt, 9, path, strlen (path), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    return retcode;
}

static gaiaGeomCollPtr
get_center_point (sqlite3 * sqlite, const char *coverage)
{
/* attempting to retrieve the Coverage's Center Point */
    char *sql;
    sqlite3_stmt *stmt;
    gaiaGeomCollPtr geom = NULL;
    int ret;

    sql = sqlite3_mprintf ("SELECT MakePoint("
			   "extent_minx + ((extent_maxx - extent_minx) / 2.0), "
			   "extent_miny + ((extent_maxy - extent_miny) / 2.0)) "
			   "FROM raster_coverages WHERE coverage_name = %Q",
			   coverage);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return NULL;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		int blob_sz = sqlite3_column_bytes (stmt, 0);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
	    }
      }
    sqlite3_finalize (stmt);
    return geom;
}

static int
test_coverage (sqlite3 * sqlite, unsigned char compression, int tile_sz,
	       int *retcode)
{
/* testing some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage;
    const char *sample_name;
    const char *pixel_name;
    unsigned char num_bands;
    const char *compression_name;
    int qlty;
    int tile_size;
    char *sql;
    gaiaGeomCollPtr geom;
    int test_map_image = 0;

/* setting the coverage name */
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "orbetello_none_256";
		test_map_image = 1;
		break;
	    case TILE_512:
		coverage = "orbetello_none_512";
		break;
	    case TILE_1024:
		coverage = "orbetello_none_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_DEFLATE:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "orbetello_zip_256";
		break;
	    case TILE_512:
		coverage = "orbetello_zip_512";
		break;
	    case TILE_1024:
		coverage = "orbetello_zip_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_LZMA:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "orbetello_lzma_256";
		break;
	    case TILE_512:
		coverage = "orbetello_lzma_512";
		break;
	    case TILE_1024:
		coverage = "orbetello_lzma_1024";
		break;
	    };
	  break;
      };

/* preparing misc Coverage's parameters */
    sample_name = "UINT16";
    pixel_name = "MULTIBAND";
    num_bands = 4;
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  compression_name = "NONE";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_DEFLATE:
	  compression_name = "DEFLATE";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_LZMA:
	  compression_name = "LZMA";
	  qlty = 100;
	  break;
      };
    switch (tile_sz)
      {
      case TILE_256:
	  tile_size = 256;
	  break;
      case TILE_512:
	  tile_size = 512;
	  break;
      case TILE_1024:
	  tile_size = 1024;
	  break;
      };

/* creating the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.2f, %1.2f)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 32632,
			   4.06, 4.06);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

/* loading the first section */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRaster(%Q, %Q, 1, 32632, 0, 1)", coverage,
	 "map_samples/orbview3-orbetello/orbetello2.tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRasters #1 \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -2;
	  return 0;
      }

/* loading the second section */
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 0, 32632, 0, 1)",
			   coverage,
			   "map_samples/orbview3-orbetello/orbetello1.tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster #2 \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -3;
	  return 0;
      }

/* re-building the Pyramid Levels */
    sql =
	sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, %Q, 1, 1)", coverage,
			 "orbetello1");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
	  return 0;
      }

/* destroying the Pyramid Levels */
    sql = sqlite3_mprintf ("SELECT RL2_DePyramidize(%Q, NULL, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DePyramidize \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -5;
	  return 0;
      }

/* building yet again the Pyramid Levels */
    sql =
	sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, %Q, 1, 1)", coverage,
			 "orbetello1");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -6;
	  return 0;
      }

/* export tests */
    geom = get_center_point (sqlite, coverage);
    if (geom == NULL)
      {
	  *retcode += -7;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 1))
      {
	  *retcode += -8;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 2))
      {
	  *retcode += -9;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 4))
      {
	  *retcode += -10;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 8))
      {
	  *retcode += -11;
	  return 0;
      }

/* testing BandComposed (Geo)TIFF export */
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 1, 0, 0))
      {
	  *retcode += -12;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 1, 1, 0))
      {
	  *retcode += -13;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 1, 2, 1))
      {
	  *retcode += -14;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 4, 0, 0))
      {
	  *retcode += -15;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 4, 1, 1))
      {
	  *retcode += -16;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 4, 2, 0))
      {
	  *retcode += -17;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 8, 0, 1))
      {
	  *retcode += -18;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 8, 1, 0))
      {
	  *retcode += -19;
	  return 0;
      }
    if (!do_export_band_composed_geotiff (sqlite, coverage, geom, 8, 2, 0))
      {
	  *retcode += -20;
	  return 0;
      }

    if (!do_export_band_composed_tiff (sqlite, coverage, geom, 1, 0))
      {
	  *retcode += -21;
	  return 0;
      }
    if (!do_export_band_composed_tiff (sqlite, coverage, geom, 1, 1))
      {
	  *retcode += -22;
	  return 0;
      }
    if (!do_export_band_composed_tiff (sqlite, coverage, geom, 1, 2))
      {
	  *retcode += -23;
	  return 0;
      }

    if (!do_export_band_composed_tiff_tfw (sqlite, coverage, geom, 1, 0))
      {
	  *retcode += -24;
	  return 0;
      }
    if (!do_export_band_composed_tiff_tfw (sqlite, coverage, geom, 1, 1))
      {
	  *retcode += -25;
	  return 0;
      }
    if (!do_export_band_composed_tiff_tfw (sqlite, coverage, geom, 1, 2))
      {
	  *retcode += -26;
	  return 0;
      }

/* testing MonoBand (Geo)TIFF export */
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 1, 0, 0))
      {
	  *retcode += -27;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 1, 1, 0))
      {
	  *retcode += -28;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 1, 2, 1))
      {
	  *retcode += -29;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 4, 0, 0))
      {
	  *retcode += -30;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 4, 1, 1))
      {
	  *retcode += -31;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 4, 2, 0))
      {
	  *retcode += -32;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 8, 0, 1))
      {
	  *retcode += -33;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 8, 1, 0))
      {
	  *retcode += -34;
	  return 0;
      }
    if (!do_export_mono_band_geotiff (sqlite, coverage, geom, 8, 2, 0))
      {
	  *retcode += -35;
	  return 0;
      }

    if (!do_export_mono_band_tiff (sqlite, coverage, geom, 1, 0))
      {
	  *retcode += -36;
	  return 0;
      }
    if (!do_export_mono_band_tiff (sqlite, coverage, geom, 1, 1))
      {
	  *retcode += -37;
	  return 0;
      }
    if (!do_export_mono_band_tiff (sqlite, coverage, geom, 1, 2))
      {
	  *retcode += -38;
	  return 0;
      }

    if (!do_export_mono_band_tiff_tfw (sqlite, coverage, geom, 1, 0))
      {
	  *retcode += -39;
	  return 0;
      }
    if (!do_export_mono_band_tiff_tfw (sqlite, coverage, geom, 1, 1))
      {
	  *retcode += -40;
	  return 0;
      }
    if (!do_export_mono_band_tiff_tfw (sqlite, coverage, geom, 1, 2))
      {
	  *retcode += -41;
	  return 0;
      }
    if (!test_map_image)
	goto skip;

/* loading the RasterSymbolizers */
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "ir_false_color2.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #1 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -42;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "ir_false_color2_gamma.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #1 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -43;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "ir_gray.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #2 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -44;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "ir_gray_gamma.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #3 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -45;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "rgb_histogram.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #4 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -46;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "rgb_normalize.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #5 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -47;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "rgb_normalize2.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #6 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -48;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "rgb_histogram2.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #7 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -50;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "rgb_gamma.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #8 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -51;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "gray_normalize2.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #9 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -52;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "gray_histogram2.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #10 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -53;
	  return 0;
      }
    sql = sqlite3_mprintf ("SELECT RegisterRasterStyledLayer(%Q, "
			   "XB_Create(XB_LoadXML(%Q), 1, 1))", coverage,
			   "gray_gamma2.xml");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RegisterRasterStyledLayer #11 \"%s\" error: %s\n",
		   coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -54;
	  return 0;
      }

/* testing GetMapImage - IR false color */
    if (!do_export_map_image (sqlite, coverage, geom, "ir_false_color2", "png"))
      {
	  *retcode += 55;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_false_color2", "jpg"))
      {
	  *retcode += 56;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_false_color2", "tif"))
      {
	  *retcode += 57;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_false_color2", "pdf"))
      {
	  *retcode += 58;
	  return 0;
      }

/* testing GetMapImage - IR false color - GammaValue */
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color2_gamma", "png"))
      {
	  *retcode += 59;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color2_gamma", "jpg"))
      {
	  *retcode += 60;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color2_gamma", "tif"))
      {
	  *retcode += 61;
	  return 0;
      }
    if (!do_export_map_image
	(sqlite, coverage, geom, "ir_false_color2_gamma", "pdf"))
      {
	  *retcode += 62;
	  return 0;
      }

/* testing GetMapImage - IR gray */
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray", "png"))
      {
	  *retcode += 63;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray", "jpg"))
      {
	  *retcode += 64;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray", "tif"))
      {
	  *retcode += 65;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray", "pdf"))
      {
	  *retcode += 66;
	  return 0;
      }

/* testing GetMapImage - IR gray - GammaValue */
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray_gamma", "png"))
      {
	  *retcode += 67;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray_gamma", "jpg"))
      {
	  *retcode += 68;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray_gamma", "tif"))
      {
	  *retcode += 69;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "ir_gray_gamma", "pdf"))
      {
	  *retcode += 70;
	  return 0;
      }

/* testing GetMapImage - RGB Normalize */
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_normalize", "png"))
      {
	  *retcode += 71;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_normalize", "jpg"))
      {
	  *retcode += 72;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_normalize", "tif"))
      {
	  *retcode += 73;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_normalize", "pdf"))
      {
	  *retcode += 74;
	  return 0;
      }

/* testing GetMapImage - RGB Histogram */
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_histogram", "png"))
      {
	  *retcode += 75;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_histogram", "jpg"))
      {
	  *retcode += 76;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_histogram", "tif"))
      {
	  *retcode += 77;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_histogram", "pdf"))
      {
	  *retcode += 78;
	  return 0;
      }

/* testing GetMapImage - RGB Normalize */
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_normalize2", "png"))
      {
	  *retcode += 79;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_normalize2", "jpg"))
      {
	  *retcode += 80;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_normalize2", "tif"))
      {
	  *retcode += 81;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_normalize2", "pdf"))
      {
	  *retcode += 82;
	  return 0;
      }

/* testing GetMapImage - RGB Histogram */
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_histogram2", "png"))
      {
	  *retcode += 83;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_histogram2", "jpg"))
      {
	  *retcode += 84;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_histogram2", "tif"))
      {
	  *retcode += 85;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_histogram2", "pdf"))
      {
	  *retcode += 86;
	  return 0;
      }

/* testing GetMapImage - RGB GammaValue */
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_gamma", "png"))
      {
	  *retcode += 87;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_gamma", "jpg"))
      {
	  *retcode += 88;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_gamma", "tif"))
      {
	  *retcode += 89;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "rgb_gamma", "pdf"))
      {
	  *retcode += 90;
	  return 0;
      }

/* testing GetMapImage - Gray Normalize */
    if (!do_export_map_image (sqlite, coverage, geom, "gray_normalize2", "png"))
      {
	  *retcode += 91;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_normalize2", "jpg"))
      {
	  *retcode += 92;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_normalize2", "tif"))
      {
	  *retcode += 93;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_normalize2", "pdf"))
      {
	  *retcode += 94;
	  return 0;
      }

/* testing GetMapImage - GRAY Histogram */
    if (!do_export_map_image (sqlite, coverage, geom, "gray_histogram2", "png"))
      {
	  *retcode += 95;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_histogram2", "jpg"))
      {
	  *retcode += 96;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_histogram2", "tif"))
      {
	  *retcode += 97;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_histogram2", "pdf"))
      {
	  *retcode += 98;
	  return 0;
      }

/* testing GetMapImage - GRAY GammaValue */
    if (!do_export_map_image (sqlite, coverage, geom, "gray_gamma2", "png"))
      {
	  *retcode += 99;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_gamma2", "jpg"))
      {
	  *retcode += 100;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_gamma2", "tif"))
      {
	  *retcode += 101;
	  return 0;
      }
    if (!do_export_map_image (sqlite, coverage, geom, "gray_gamma2", "pdf"))
      {
	  *retcode += 102;
	  return 0;
      }
  skip:
    gaiaFreeGeomColl (geom);

/* testing GetTileImage() */
    if (!do_export_tile_image (sqlite, coverage, 1, 0))
      {
	  *retcode += -62;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, 1, 1))
      {
	  *retcode += -63;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, 1, 2))
      {
	  *retcode += -64;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, -1, 0))
      {
	  *retcode += -65;
	  return 0;
      }

/* testing GetTileImage() - Mono-Band */
    if (!do_export_mono_tile_image (sqlite, coverage, 1, 0))
      {
	  *retcode += -66;
	  return 0;
      }
    if (!do_export_mono_tile_image (sqlite, coverage, 1, 1))
      {
	  *retcode += -67;
	  return 0;
      }
    if (!do_export_mono_tile_image (sqlite, coverage, 1, 2))
      {
	  *retcode += -68;
	  return 0;
      }
    if (!do_export_mono_tile_image (sqlite, coverage, -1, 0))
      {
	  *retcode += -69;
	  return 0;
      }

    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, unsigned char compression, int tile_sz,
	       int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage;
    char *sql;

/* setting the coverage name */
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "orbetello_none_256";
		break;
	    case TILE_512:
		coverage = "orbetello_none_512";
		break;
	    case TILE_1024:
		coverage = "orbetello_none_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_DEFLATE:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "orbetello_zip_256";
		break;
	    case TILE_512:
		coverage = "orbetello_zip_512";
		break;
	    case TILE_1024:
		coverage = "orbetello_zip_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_LZMA:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "orbetello_lzma_256";
		break;
	    case TILE_512:
		coverage = "orbetello_lzma_512";
		break;
	    case TILE_1024:
		coverage = "orbetello_lzma_1024";
		break;
	    };
	  break;
      };

/* setting a Title and Abstract for this DBMS Coverage */
    sql =
	sqlite3_mprintf ("SELECT RL2_SetCoverageInfos(%Q, %Q, %Q)", coverage,
			 "this is a tile", "this is an abstact");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SetCoverageInfos \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

/* dropping the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_DropCoverage(%Q, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

    return 1;
}

int
main (int argc, char *argv[])
{
    int result = 0;
    int ret;
    char *err_msg = NULL;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, 0);
    ret =
	sqlite3_exec (db_handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -2;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT CreateRasterCoveragesTable()", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoveragesTable() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -3;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT CreateStylingTables()", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateStylingTables() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 4;
      }

/* tests */
    ret = -100;
    if (!test_coverage (db_handle, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -120;
    if (!test_coverage (db_handle, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -140;
    if (!test_coverage (db_handle, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -200;
    if (!test_coverage (db_handle, RL2_COMPRESSION_DEFLATE, TILE_256, &ret))
	return ret;
    ret = -220;
    if (!test_coverage (db_handle, RL2_COMPRESSION_DEFLATE, TILE_512, &ret))
	return ret;
    ret = -240;
    if (!test_coverage (db_handle, RL2_COMPRESSION_DEFLATE, TILE_1024, &ret))
	return ret;
    ret = -300;
    if (!test_coverage (db_handle, RL2_COMPRESSION_LZMA, TILE_256, &ret))
	return ret;
    ret = -320;
    if (!test_coverage (db_handle, RL2_COMPRESSION_LZMA, TILE_512, &ret))
	return ret;
    ret = -340;
    if (!test_coverage (db_handle, RL2_COMPRESSION_LZMA, TILE_1024, &ret))
	return ret;

/* dropping all Coverages */
    ret = -170;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -180;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -190;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -270;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_DEFLATE, TILE_256, &ret))
	return ret;
    ret = -280;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_DEFLATE, TILE_512, &ret))
	return ret;
    ret = -290;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_DEFLATE, TILE_1024, &ret))
	return ret;
    ret = -370;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_LZMA, TILE_256, &ret))
	return ret;
    ret = -380;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_LZMA, TILE_512, &ret))
	return ret;
    ret = -390;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_LZMA, TILE_1024, &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    if (old_SPATIALITE_SECURITY_ENV)
      {
#ifdef _WIN32
	  char *env = sqlite3_mprintf ("SPATIALITE_SECURITY=%s",
				       old_SPATIALITE_SECURITY_ENV);
	  putenv (env);
	  sqlite3_free (env);
#else /* not WIN32 */
	  setenv ("SPATIALITE_SECURITY", old_SPATIALITE_SECURITY_ENV, 1);
#endif
      }
    else
      {
#ifdef _WIN32
	  putenv ("SPATIALITE_SECURITY=");
#else /* not WIN32 */
	  unsetenv ("SPATIALITE_SECURITY");
#endif
      }
    return result;
}
