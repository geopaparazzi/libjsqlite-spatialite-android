/*

 test_map_trento.c -- RasterLite-2 Test Case

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

#include "sqlite3.h"
#include "spatialite.h"
#include "spatialite/gaiaaux.h"

#include "rasterlite2/rasterlite2.h"

#define TILE_256	256
#define TILE_512	512
#define TILE_1024	1024

/* global variable used to alternatively enable/disable multithreading */
int multithreading = 1;


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
    sqlite3_bind_text (stmt, 9, "JPEG", 4, SQLITE_TRANSIENT);
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
do_export_jpeg (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		int scale)
{
/* exporting a JPEG [no Worldfile] */
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

    path = sqlite3_mprintf ("./%s_%d.jpg", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteJpeg(?, ?, ?, ?, ?, ?, ?, ?)";
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
    sqlite3_bind_int (stmt, 8, 90);
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
do_export_jpeg_jgw (sqlite3 * sqlite, const char *coverage,
		    gaiaGeomCollPtr geom, int scale)
{
/* exporting a JPEG + Worldfile */
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

    path = sqlite3_mprintf ("./%s_jgw_%d.jpg", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteJpegJgw(?, ?, ?, ?, ?, ?, ?, ?)";
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
    sqlite3_bind_int (stmt, 8, 40);
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
    path = sqlite3_mprintf ("./%s_jgw_%d.jgw", coverage, scale);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_jpeg (sqlite3 * sqlite, const char *coverage,
			gaiaGeomCollPtr geom, int scale)
{
/* exporting a JPEG [no Worldfile] - Section */
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

    path = sqlite3_mprintf ("./%s_sect1_%d.jpg", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteSectionJpeg(?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_int (stmt, 9, 90);
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
do_export_section_jpeg_jgw (sqlite3 * sqlite, const char *coverage,
			    gaiaGeomCollPtr geom, int scale)
{
/* exporting a JPEG + Worldfile - Section */
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

    path = sqlite3_mprintf ("./%s_sect1_jgw_%d.jpg", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteSectionJpegJgw(?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_int (stmt, 9, 40);
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
    path = sqlite3_mprintf ("./%s_sect1_jgw_%d.jgw", coverage, scale);
    unlink (path);
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
do_export_image (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		 double radius, const char *suffix)
{
/* exporting a PNG/JPEG/TIFF/PDF image */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    const char *mime_type = "text/plain";

    path = sqlite3_mprintf ("./%s_%1.0f%s", coverage, radius, suffix);

    sql =
	"SELECT RL2_GetMapImageFromRaster(?, ST_Buffer(?, ?), 512, 512, 'default', ?, '#ffffff', 1, 80)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
    sqlite3_bind_double (stmt, 3, radius);
    if (strcmp (suffix, ".png") == 0)
	mime_type = "image/png";
    if (strcmp (suffix, ".jpg") == 0)
	mime_type = "image/jpeg";
    if (strcmp (suffix, ".tif") == 0)
	mime_type = "image/tiff";
    if (strcmp (suffix, ".pdf") == 0)
	mime_type = "application/x-pdf";
    sqlite3_bind_text (stmt, 4, mime_type, strlen (mime_type),
		       SQLITE_TRANSIENT);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
	    {
		FILE *out;
		blob = (unsigned char *) sqlite3_column_blob (stmt, 0);
		blob_size = sqlite3_column_bytes (stmt, 0);
		out = fopen (path, "wb");
		if (out != NULL)
		  {
		      /* writing the output image */
		      if ((int) fwrite (blob, 1, blob_size, out) == blob_size)
			  retcode = 1;
		      fclose (out);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to GetMap \"%s\"\n", path);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
test_coverage (sqlite3 * sqlite, unsigned char pixel, unsigned char compression,
	       int type, int *retcode)
{
/* testing some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    const char *sample_name = NULL;
    const char *pixel_name = NULL;
    unsigned char num_bands = 1;
    const char *compression_name = NULL;
    const char *indir;
    const char *reload;
    int qlty = 100;
    char *sql;
    int tile_size = 256;
    gaiaGeomCollPtr geom;
    unsigned int width;
    unsigned int height;
    unsigned char pixel_type;
    char *md5;

    if (!type)
      {
	  indir = "map_samples/trento-gray";
	  reload = "map_samples/trento-gray/trento-gray1.jpg";
      }
    else
      {
	  indir = "map_samples/trento-rgb";
	  reload = "map_samples/trento-rgb/trento-rgb1.jpg";
      }

/* setting the coverage name */
    switch (type)
      {
      case 0:
	  switch (pixel)
	    {
	    case RL2_PIXEL_RGB:
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		      coverage = "gray_rgb_none_512";
		      break;
		  case RL2_COMPRESSION_PNG:
		      coverage = "gray_rgb_png_512";
		      break;
		  case RL2_COMPRESSION_JPEG:
		      coverage = "gray_rgb_jpeg_512";
		      break;
		  };
		break;
	    case RL2_PIXEL_GRAYSCALE:
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		      coverage = "gray_gray_none_512";
		      break;
		  case RL2_COMPRESSION_PNG:
		      coverage = "gray_gray_png_512";
		      break;
		  case RL2_COMPRESSION_JPEG:
		      coverage = "gray_gray_jpeg_512";
		      break;
		  };
		break;
	    };
	  break;
      case 1:
	  switch (pixel)
	    {
	    case RL2_PIXEL_RGB:
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		      coverage = "rgb_rgb_none_512";
		      break;
		  case RL2_COMPRESSION_PNG:
		      coverage = "rgb_rgb_png_512";
		      break;
		  case RL2_COMPRESSION_JPEG:
		      coverage = "rgb_rgb_jpeg_512";
		      break;
		  };
		break;
	    case RL2_PIXEL_GRAYSCALE:
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		      coverage = "rgb_gray_none_512";
		      break;
		  case RL2_COMPRESSION_PNG:
		      coverage = "rgb_gray_png_512";
		      break;
		  case RL2_COMPRESSION_JPEG:
		      coverage = "rgb_gray_jpeg_512";
		      break;
		  };
		break;
	    };
	  break;
      };

/* preparing misc Coverage's parameters */
    sample_name = "UINT8";
    switch (pixel)
      {
      case RL2_PIXEL_RGB:
	  num_bands = 3;
	  pixel_name = "RGB";
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  num_bands = 1;
	  pixel_name = "GRAYSCALE";
	  break;
      };
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  compression_name = "NONE";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_PNG:
	  compression_name = "PNG";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_JPEG:
	  compression_name = "JPEG";
	  qlty = 80;
	  break;
      };
    tile_size = 512;

/* setting the MultiThreading mode alternatively on/off */
    if (multithreading)
      {
	  sql = "SELECT RL2_SetMaxThreads(2)";
	  multithreading = 0;
      }
    else
      {
	  sql = "SELECT RL2_SetMaxThreads(1)";
	  multithreading = 1;
      }
    execute_check (sqlite, sql);

/* creating the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateRasterCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.16f, %1.16f)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 32632,
			   0.5, 0.5);
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
	("SELECT RL2_LoadRastersFromDir(%Q, %Q, %Q, 0, 32632, 0, 1)", coverage,
	 indir, ".jpg");
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

/* deleting the first section */
    sql = sqlite3_mprintf ("SELECT RL2_DeleteSection(%Q, 1, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DeleteSection \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -3;
	  return 0;
      }

/* re-loading yet again the first section */
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 0, 32632, 1, 1)",
			   coverage, reload);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
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
	  *retcode += -5;
	  return 0;
      }

/* export tests */
    geom = get_center_point (sqlite, coverage);
    if (geom == NULL)
      {
	  *retcode += -6;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 1))
      {
	  *retcode += -7;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 2))
      {
	  *retcode += -8;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 4))
      {
	  *retcode += -9;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, geom, 8))
      {
	  *retcode += -10;
	  return 0;
      }
    if (!do_export_jpeg (sqlite, coverage, geom, 1))
      {
	  *retcode += -11;
	  return 0;
      }
    if (!do_export_jpeg (sqlite, coverage, geom, 2))
      {
	  *retcode += -12;
	  return 0;
      }
    if (!do_export_jpeg (sqlite, coverage, geom, 4))
      {
	  *retcode += -13;
	  return 0;
      }
    if (!do_export_jpeg (sqlite, coverage, geom, 8))
      {
	  *retcode += -14;
	  return 0;
      }
    if (!do_export_jpeg_jgw (sqlite, coverage, geom, 1))
      {
	  *retcode += -15;
	  return 0;
      }
    if (!do_export_jpeg_jgw (sqlite, coverage, geom, 2))
      {
	  *retcode += -16;
	  return 0;
      }
    if (!do_export_jpeg_jgw (sqlite, coverage, geom, 4))
      {
	  *retcode += -17;
	  return 0;
      }
    if (!do_export_jpeg_jgw (sqlite, coverage, geom, 8))
      {
	  *retcode += -18;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 600.015, ".png"))
      {
	  *retcode += -19;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 20.3, ".png"))
      {
	  *retcode += -20;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 256.0, ".png"))
      {
	  *retcode += -21;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 600.015, ".pdf"))
      {
	  *retcode += -22;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 20.3, ".pdf"))
      {
	  *retcode += -23;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 256.0, ".pdf"))
      {
	  *retcode += -24;
	  return 0;
      }
    if (!do_export_section_jpeg (sqlite, coverage, geom, 1))
      {
	  *retcode += -25;
	  return 0;
      }
    if (!do_export_section_jpeg (sqlite, coverage, geom, 2))
      {
	  *retcode += -26;
	  return 0;
      }
    if (!do_export_section_jpeg (sqlite, coverage, geom, 4))
      {
	  *retcode += -27;
	  return 0;
      }
    if (!do_export_section_jpeg (sqlite, coverage, geom, 8))
      {
	  *retcode += -28;
	  return 0;
      }
    if (!do_export_section_jpeg_jgw (sqlite, coverage, geom, 1))
      {
	  *retcode += -29;
	  return 0;
      }
    if (!do_export_section_jpeg_jgw (sqlite, coverage, geom, 2))
      {
	  *retcode += -30;
	  return 0;
      }
    if (!do_export_section_jpeg_jgw (sqlite, coverage, geom, 4))
      {
	  *retcode += -31;
	  return 0;
      }
    if (!do_export_section_jpeg_jgw (sqlite, coverage, geom, 8))
      {
	  *retcode += -32;
	  return 0;
      }
    gaiaFreeGeomColl (geom);

/* testing GetTileImage() */
    if (!do_export_tile_image (sqlite, coverage, 1))
      {
	  *retcode += -33;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, 2))
      {
	  *retcode += -34;
	  return 0;
      }
    if (!do_export_tile_image (sqlite, coverage, -1))
      {
	  *retcode += -35;
	  return 0;
      }

/* testing GetJpegInfos */
    if (rl2_get_jpeg_infos
	("./map_samples/trento-gray/trento-gray1.jpg", &width, &height,
	 &pixel_type) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to GetJpegInfos from \"trento-gray1.jpg\"\n");
	  *retcode += -36;
	  return 0;
      }
    if (width != 600)
      {
	  fprintf (stderr, "Unexpected Width from \"trento-gray1.jpg\" %u\n",
		   width);
	  *retcode += -37;
	  return 0;
      }
    if (height != 600)
      {
	  fprintf (stderr, "Unexpected Height from \"trento-gray1.jpg\" %u\n",
		   height);
	  *retcode += -38;
	  return 0;
      }
    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr,
		   "Unexpected PixelType from \"trento-gray1.jpg\" %02x\n",
		   pixel_type);
	  *retcode += -39;
	  return 0;
      }
    if (rl2_get_jpeg_infos
	("./map_samples/trento-rgb/trento-rgb1.jpg", &width, &height,
	 &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "Unable to GetJpegInfos from \"trento-rgb1.jpg\"\n");
	  *retcode += -40;
	  return 0;
      }
    if (width != 600)
      {
	  fprintf (stderr, "Unexpected Width from \"trento-rgb1.jpg\" %u\n",
		   width);
	  *retcode += -41;
	  return 0;
      }
    if (height != 600)
      {
	  fprintf (stderr, "Unexpected Height from \"trento-rgb1.jpg\" %u\n",
		   height);
	  *retcode += -42;
	  return 0;
      }
    if (pixel_type != RL2_PIXEL_RGB)
      {
	  fprintf (stderr,
		   "Unexpected PixelType from \"trento-rgb1.jpg\" %02x\n",
		   pixel_type);
	  *retcode += -43;
	  return 0;
      }
/* testing MD5 Checksum */
    md5 =
	rl2_compute_file_md5_checksum
	("./map_samples/trento-gray/trento-gray1.jpg");
    if (md5 == NULL)
      {
	  fprintf (stderr, "Unable to GetMD5 from \"trento-gray1.jpg\"\n");
	  *retcode += -44;
	  return 0;
      }
    if (strcmp (md5, "6750bf6f168200dab2c741017af82f1d") != 0)
      {
	  fprintf (stderr, "Unexpected MD5 for \"trento-gray1.jpg\" %s\n", md5);
	  *retcode += -45;
	  return 0;
      }
    free (md5);
    md5 =
	rl2_compute_file_md5_checksum
	("./map_samples/trento-rgb/trento-rgb1.jpg");
    if (md5 == NULL)
      {
	  fprintf (stderr, "Unable to GetMD5 from \"trento-rgb1.jpg\"\n");
	  *retcode += -46;
	  return 0;
      }
    if (strcmp (md5, "8f0903be04e7ff3a2cb6188d52034502") != 0)
      {
	  fprintf (stderr, "Unexpected MD5 for \"trento-rgb1.jpg\" %s\n", md5);
	  *retcode += -47;
	  return 0;
      }
    free (md5);

    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, unsigned char pixel, unsigned char compression,
	       int type, int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    char *sql;

/* setting the coverage name */
    switch (type)
      {
      case 0:
	  switch (pixel)
	    {
	    case RL2_PIXEL_RGB:
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		      coverage = "gray_rgb_none_512";
		      break;
		  case RL2_COMPRESSION_PNG:
		      coverage = "gray_rgb_png_512";
		      break;
		  case RL2_COMPRESSION_JPEG:
		      coverage = "gray_rgb_jpeg_512";
		      break;
		  };
		break;
	    case RL2_PIXEL_GRAYSCALE:
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		      coverage = "gray_gray_none_512";
		      break;
		  case RL2_COMPRESSION_PNG:
		      coverage = "gray_gray_png_512";
		      break;
		  case RL2_COMPRESSION_JPEG:
		      coverage = "gray_gray_jpeg_512";
		      break;
		  };
		break;
	    };
	  break;
      case 1:
	  switch (pixel)
	    {
	    case RL2_PIXEL_RGB:
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		      coverage = "rgb_rgb_none_512";
		      break;
		  case RL2_COMPRESSION_PNG:
		      coverage = "rgb_rgb_png_512";
		      break;
		  case RL2_COMPRESSION_JPEG:
		      coverage = "rgb_rgb_jpeg_512";
		      break;
		  };
		break;
	    case RL2_PIXEL_GRAYSCALE:
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		      coverage = "rgb_gray_none_512";
		      break;
		  case RL2_COMPRESSION_PNG:
		      coverage = "rgb_gray_png_512";
		      break;
		  case RL2_COMPRESSION_JPEG:
		      coverage = "rgb_gray_jpeg_512";
		      break;
		  };
		break;
	    };
	  break;
      }

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
    void *priv_data = rl2_alloc_private ();
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
    rl2_init (db_handle, priv_data, 0);
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

/* RGB tests */
    ret = -100;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, 1, &ret))
	return ret;
    ret = -130;
    if (!test_coverage (db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, 1, &ret))
	return ret;
    ret = -160;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, 1, &ret))
	return ret;
    ret = -200;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, 0, &ret))
	return ret;
    ret = -230;
    if (!test_coverage (db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, 0, &ret))
	return ret;
    ret = -260;
    if (!test_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, 0, &ret))
	return ret;

/* GRAYSCALE tests */
    ret = -600;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, 1, &ret))
	return ret;
    ret = -630;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, 1, &ret))
	return ret;
    ret = -660;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, 1, &ret))
	return ret;
    ret = -700;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, 0, &ret))
	return ret;
    ret = -730;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, 0, &ret))
	return ret;
    ret = -760;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, 0, &ret))
	return ret;

/* dropping all RGB Coverages */
    ret = -300;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, 1, &ret))
	return ret;
    ret = -330;
    if (!drop_coverage (db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, 1, &ret))
	return ret;
    ret = -360;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, 1, &ret))
	return ret;
    ret = -400;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, 0, &ret))
	return ret;
    ret = -430;
    if (!drop_coverage (db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, 0, &ret))
	return ret;
    ret = -460;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, 0, &ret))
	return ret;

/* dropping all GRAYSCALE Coverages */
    ret = -800;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, 1, &ret))
	return ret;
    ret = -830;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, 1, &ret))
	return ret;
    ret = -860;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, 1, &ret))
	return ret;
    ret = -900;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, 0, &ret))
	return ret;
    ret = -930;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, 0, &ret))
	return ret;
    ret = -960;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, 0, &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
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
