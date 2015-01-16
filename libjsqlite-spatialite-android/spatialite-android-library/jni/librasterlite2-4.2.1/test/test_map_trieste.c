/*

 test_map_trieste.c -- RasterLite-2 Test Case

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

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
do_export_tile_image (sqlite3 * sqlite, const char *coverage, int tile_id)
{
/* attempting to export a visible Tile */
    char *sql;
    char *path;
    int ret;
    int transparent = 1;

    if (tile_id <= 1)
	transparent = 0;
    if (tile_id < 0)
	tile_id = get_max_tile_id (sqlite, coverage);
    path = sqlite3_mprintf ("./%s_tile_%d.png", coverage, tile_id);
    sql =
	sqlite3_mprintf
	("SELECT BlobToFile(RL2_GetTileImage(%Q, %d, '#e0ffe0', %d), %Q)",
	 coverage, tile_id, transparent, path);
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
test_statistics (sqlite3 * sqlite, const char *coverage, int *retcode)
{
/* testing Coverage and Band statistics */
    int ret;
    char *err_msg = NULL;
    char *sql;
    char **results;
    int rows;
    int columns;
    const char *string;
    int intval;

/* testing RasterStatistics */
    sql =
	sqlite3_mprintf
	("SELECT RL2_GetRasterStatistics_NoDataPixelsCount(statistics), "
	 "RL2_GetRasterStatistics_ValidPixelsCount(statistics), "
	 "RL2_GetRasterStatistics_SampleType(statistics), "
	 "RL2_GetRasterStatistics_BandsCount(statistics) "
	 "FROM raster_coverages WHERE Lower(coverage_name) = Lower(%Q)",
	 coverage);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (rows != 1 || columns != 4)
      {
	  fprintf (stderr, "Unexpected error: bad result: %i/%i.\n", rows,
		   columns);
	  *retcode += -2;
	  return 0;
      }

/* NoData Pixels */
    string = results[4];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (NoDataPixelsCount)\n");
	  *retcode += -3;
	  return 0;
      }

/* Valid Pixels */
    string = results[5];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (ValidPixelsCount)\n");
	  *retcode += -5;
	  return 0;
      }
    intval = atoi (string);
    if (intval != 719996)
      {
	  fprintf (stderr, "Unexpected ValidPixelsCount: %d\n", intval);
	  *retcode += -6;
	  return 0;
      }

/* Sample Type */
    string = results[6];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (SampleType)\n");
	  *retcode += -7;
	  return 0;
      }
    if (strcmp (string, "UINT16") != 0)
      {
	  fprintf (stderr, "Unexpected SampleType: %s\n", string);
	  *retcode += -8;
	  return 0;
      }

/* Bands */
    string = results[7];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (BandsCount)\n");
	  *retcode += -9;
	  return 0;
      }
    intval = atoi (string);
    if (intval != 1)
      {
	  fprintf (stderr, "Unexpected BandsCount: %d\n", intval);
	  *retcode += -10;
	  return 0;
      }

    sqlite3_free_table (results);

/* testing BandStatistics */
    sql =
	sqlite3_mprintf
	("SELECT RL2_GetBandStatistics_Min(statistics, 0), "
	 "RL2_GetBandStatistics_Max(statistics, 0), "
	 "RL2_GetBandStatistics_Avg(statistics, 0), "
	 "RL2_GetBandStatistics_Var(statistics, 0), "
	 "RL2_GetBandStatistics_StdDev(statistics, 0) "
	 "FROM raster_coverages WHERE Lower(coverage_name) = Lower(%Q)",
	 coverage);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (rows != 1 || columns != 5)
      {
	  fprintf (stderr, "Unexpected error: bad result: %i/%i.\n", rows,
		   columns);
	  *retcode += -2;
	  return 0;
      }

/* Min */
    string = results[5];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band Min)\n");
	  *retcode += -12;
	  return 0;
      }
    intval = atoi (string);
    if (intval != 6)
      {
	  fprintf (stderr, "Unexpected Band Min: %d\n", intval);
	  *retcode += -13;
	  return 0;
      }

/* Max */
    string = results[6];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band Max)\n");
	  *retcode += -14;
	  return 0;
      }
    intval = atoi (string);
    if (intval != 1841)
      {
	  fprintf (stderr, "Unexpected Band Max: %d\n", intval);
	  *retcode += -15;
	  return 0;
      }

/* Avg */
    string = results[7];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band Avg)\n");
	  *retcode += -16;
	  return 0;
      }
    intval = atoi (string);
    if (intval != 183)
      {
	  fprintf (stderr, "Unexpected Band Avg: %d\n", intval);
	  *retcode += -17;
	  return 0;
      }

/* Var */
    string = results[8];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band Var)\n");
	  *retcode += -18;
	  return 0;
      }

/* StdDev */
    string = results[9];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band StdDev)\n");
	  *retcode += -20;
	  return 0;
      }

    sqlite3_free_table (results);

    return 1;
}

static int
test_coverage (sqlite3 * sqlite, unsigned char pixel, unsigned char compression,
	       int tile_sz, int *retcode)
{
/* testing some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    const char *sample_name = NULL;
    const char *pixel_name = NULL;
    unsigned char num_bands = 1;
    const char *compression_name = NULL;
    int qlty = 100;
    int tile_size = 256;
    char *sql;
    gaiaGeomCollPtr geom;

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_GRAYSCALE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_CHARLS:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_charls_256";
		      break;
		  case TILE_512:
		      coverage = "gray_charls_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_charls_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_png_256";
		      break;
		  case TILE_512:
		      coverage = "gray_png_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_png_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSLESS_JP2:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_jp2_256";
		      break;
		  case TILE_512:
		      coverage = "gray_jp2_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_jp2_1024";
		      break;
		  };
		break;
	    };
	  break;
      };

/* preparing misc Coverage's parameters */
    sample_name = "UINT16";
    pixel_name = "DATAGRID";
    num_bands = 1;
    switch (compression)
      {
      case RL2_COMPRESSION_CHARLS:
	  compression_name = "CHARLS";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_PNG:
	  compression_name = "PNG";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_LOSSLESS_JP2:
	  compression_name = "LL_JP2";
	  qlty = 10;
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
    sql = sqlite3_mprintf ("SELECT RL2_CreateRasterCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.2f, %1.2f)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 32633,
			   1.01, 1.01);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

/* loading from directory */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRastersFromDir(%Q, %Q, %Q, 0, 32633, 0, 1)", coverage,
	 "map_samples/orbview3-trieste", ".tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRastersFromDir \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -2;
	  return 0;
      }

/* building the Pyramid Levels */
    sql = sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, NULL, 0, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -3;
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
	  *retcode += -7;
	  return 0;
      }

/* building yet again the Pyramid Levels */
    sql = sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, 2, 1, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -8;
	  return 0;
      }

/* export tests */
    geom = get_center_point (sqlite, coverage);
    if (geom == NULL)
      {
	  *retcode += -9;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 1))
      {
	  *retcode += -10;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 2))
      {
	  *retcode += -11;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 4))
      {
	  *retcode += -12;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 8))
      {
	  *retcode += -13;
	  return 0;
      }
    gaiaFreeGeomColl (geom);

/* testing GetTileImage() */
    if (!do_export_tile_image (sqlite, coverage, 1))
      {
	  *retcode += -23;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, 2))
      {
	  *retcode += -24;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, -1))
      {
	  *retcode += -25;
	  return 0;
      }

    *retcode += -26;
    if (!test_statistics (sqlite, coverage, retcode))
	return 0;

    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, unsigned char compression, int tile_sz,
	       int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    char *sql;

/* setting the coverage name */
    switch (compression)
      {
      case RL2_COMPRESSION_CHARLS:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "gray_charls_256";
		break;
	    case TILE_512:
		coverage = "gray_charls_512";
		break;
	    case TILE_1024:
		coverage = "gray_charls_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_PNG:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "gray_png_256";
		break;
	    case TILE_512:
		coverage = "gray_png_512";
		break;
	    case TILE_1024:
		coverage = "gray_png_1024";
		break;
	    };
	  break;
      case RL2_COMPRESSION_LOSSLESS_JP2:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "gray_jp2_256";
		break;
	    case TILE_512:
		coverage = "gray_jp2_512";
		break;
	    case TILE_1024:
		coverage = "gray_jp2_1024";
		break;
	    };
	  break;
      };

/* dropping the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_DropRasterCoverage(%Q, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropRasterCoverage \"%s\" error: %s\n", coverage,
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

/* GRAYSCALE tests */
#ifndef OMIT_CHARLS		/* only if CharLS is enabled */
    ret = -100;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_CHARLS, TILE_256,
	 &ret))
	return ret;
    ret = -120;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_CHARLS, TILE_512,
	 &ret))
	return ret;
    ret = -140;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_CHARLS, TILE_1024,
	 &ret))
	return ret;
#endif /* end CharLS conditional */

    ret = -200;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -220;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -240;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
    ret = -300;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_JP2, TILE_256,
	 &ret))
	return ret;
    ret = -320;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_JP2, TILE_512,
	 &ret))
	return ret;
    ret = -340;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_JP2,
	 TILE_1024, &ret))
	return ret;
#endif /* end OpenJpeg conditional */

/* dropping all GRAYSCALE Coverages */
#ifndef OMIT_CHARLS		/* only if CharLS is enabled */
    ret = -170;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_256, &ret))
	return ret;
    ret = -180;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_512, &ret))
	return ret;
    ret = -190;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_CHARLS, TILE_1024, &ret))
	return ret;
#endif /* end CharLS conditional */

    ret = -270;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -280;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -290;
    if (!drop_coverage (db_handle, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
    ret = -370;
    if (!drop_coverage
	(db_handle, RL2_COMPRESSION_LOSSLESS_JP2, TILE_256, &ret))
	return ret;
    ret = -380;
    if (!drop_coverage
	(db_handle, RL2_COMPRESSION_LOSSLESS_JP2, TILE_512, &ret))
	return ret;
    ret = -390;
    if (!drop_coverage
	(db_handle, RL2_COMPRESSION_LOSSLESS_JP2, TILE_1024, &ret))
	return ret;
#endif /* end OpenJpeg conditional */

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
