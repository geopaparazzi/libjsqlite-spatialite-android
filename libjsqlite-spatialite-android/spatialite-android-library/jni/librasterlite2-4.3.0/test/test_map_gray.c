/*

 test_map_gray.c -- RasterLite-2 Test Case

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
do_export_geotiff (sqlite3 * sqlite, const char *coverage, const char *type,
		   gaiaGeomCollPtr geom, int scale)
{
/* exporting a GeoTiff + Worldfile */
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

    path = sqlite3_mprintf ("./%s_%s_gt_%d.tif", coverage, type, scale);

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
    sqlite3_bind_int (stmt, 8, 1);
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
    path = sqlite3_mprintf ("./%s_%s_gt_%d.tfw", coverage, type, scale);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_geotiff (sqlite3 * sqlite, const char *coverage,
			   gaiaGeomCollPtr geom, int scale)
{
/* exporting a GeoTiff + Worldfile - Section */
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

    path = sqlite3_mprintf ("./%s_sect_gt_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteSectionGeoTiff(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, xx_res);
    sqlite3_bind_double (stmt, 8, yy_res);
    sqlite3_bind_int (stmt, 9, 1);
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
    path = sqlite3_mprintf ("./%s_sect_gt_%d.tfw", coverage, scale);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_section_tiff (sqlite3 * sqlite, const char *coverage,
			gaiaGeomCollPtr geom, int scale)
{
/* exporting a plain Tiff - Section */
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

    path = sqlite3_mprintf ("./%s_sect_tif_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteSectionTiff(?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
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
do_export_section_tiff_tfw (sqlite3 * sqlite, const char *coverage,
			    gaiaGeomCollPtr geom, int scale)
{
/* exporting a Tiff + Worldfile - Section */
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

    path = sqlite3_mprintf ("./%s_sect_tfw_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteSectionTiffTfw(?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
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
    path = sqlite3_mprintf ("./%s_sect_tfw_%d.tfw", coverage, scale);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

static int
do_export_tiff (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		int scale)
{
/* exporting a Tiff (no Worldfile) */
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

    path = sqlite3_mprintf ("./%s_plain_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteTiff(?, ?, ?, ?, ?, ?, ?, ?)";
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
    sqlite3_bind_text (stmt, 8, "LZW", 3, SQLITE_TRANSIENT);
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
	"SELECT RL2_GetMapImageFromRaster(?, ST_Buffer(?, ?), 512, 512, 'default', ?, '#ffffff', 0, 80)";
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
    char *sql;
    int tile_size = 256;
    gaiaGeomCollPtr geom;

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_GRAYSCALE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_none_256";
		      break;
		  case TILE_512:
		      coverage = "gray_none_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_none_1024";
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
	    case RL2_COMPRESSION_JPEG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_jpeg_256";
		      break;
		  case TILE_512:
		      coverage = "gray_jpeg_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_jpeg_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_webp_256";
		      break;
		  case TILE_512:
		      coverage = "gray_webp_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_webp_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_llwebp_256";
		      break;
		  case TILE_512:
		      coverage = "gray_llwebp_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_llwebp_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt_none_256";
		      break;
		  case TILE_512:
		      coverage = "plt_none_512";
		      break;
		  case TILE_1024:
		      coverage = "plt_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt_png_256";
		      break;
		  case TILE_512:
		      coverage = "plt_png_512";
		      break;
		  case TILE_1024:
		      coverage = "plt_png_1024";
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
      case RL2_PIXEL_GRAYSCALE:
	  pixel_name = "GRAYSCALE";
	  break;
      case RL2_PIXEL_PALETTE:
	  pixel_name = "PALETTE";
	  break;
      };
    num_bands = 1;
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
      case RL2_COMPRESSION_LOSSY_WEBP:
	  compression_name = "WEBP";
	  qlty = 80;
	  break;
      case RL2_COMPRESSION_LOSSLESS_WEBP:
	  compression_name = "LL_WEBP";
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
    sql = sqlite3_mprintf ("SELECT RL2_CreateRasterCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %d, %d)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 26914,
			   1, 1);
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
	("SELECT RL2_LoadRastersFromDir(%Q, %Q, %Q, 0, 26914, 0, 1)", coverage,
	 "map_samples/usgs-gray", ".tif");
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

    if (pixel == RL2_PIXEL_GRAYSCALE)
      {
/* building the Pyramid Levels */
	  sql =
	      sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, NULL, 0, 1)",
			       coverage);
	  ret = execute_check (sqlite, sql);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage,
			 err_msg);
		sqlite3_free (err_msg);
		*retcode += -3;
		return 0;
	    }
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
	  *retcode += -4;
	  return 0;
      }

/* re-loading yet again the first section */
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 0, 26914, 0, 1)",
			   coverage, "map_samples/usgs-gray/gray1.tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -5;
	  return 0;
      }

    if (pixel == RL2_PIXEL_GRAYSCALE)
      {
/* building the Pyramid Levels */
	  sql =
	      sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, 2, 1, 1)", coverage);
	  ret = execute_check (sqlite, sql);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage,
			 err_msg);
		sqlite3_free (err_msg);
		*retcode += -6;
		return 0;
	    }

/* destroying the Pyramid Levels */
	  sql =
	      sqlite3_mprintf ("SELECT RL2_DePyramidize(%Q, NULL, 1)",
			       coverage);
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
	  sql =
	      sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, 2, 1, 1)", coverage);
	  ret = execute_check (sqlite, sql);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "Pyramidize \"%s\" error: %s\n", coverage,
			 err_msg);
		sqlite3_free (err_msg);
		*retcode += -8;
		return 0;
	    }
      }

/* export tests */
    geom = get_center_point (sqlite, coverage);
    if (geom == NULL)
      {
	  *retcode += -9;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, "norm", geom, 1))
      {
	  *retcode += -10;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, "norm", geom, 2))
      {
	  *retcode += -11;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, "norm", geom, 4))
      {
	  *retcode += -12;
	  return 0;
      }
    if (!do_export_geotiff (sqlite, coverage, "norm", geom, 8))
      {
	  *retcode += -13;
	  return 0;
      }
    if (!do_export_tiff (sqlite, coverage, geom, 1))
      {
	  *retcode += -14;
	  return 0;
      }
    if (!do_export_tiff (sqlite, coverage, geom, 2))
      {
	  *retcode += -15;
	  return 0;
      }
    if (!do_export_tiff (sqlite, coverage, geom, 4))
      {
	  *retcode += -16;
	  return 0;
      }
    if (!do_export_tiff (sqlite, coverage, geom, 8))
      {
	  *retcode += -17;
	  return 0;
      }
    fprintf (stderr, "*************************************** 3\n");
    if (!do_export_image (sqlite, coverage, geom, 256.0, ".jpg"))
      {
	  *retcode += -18;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 290.0, ".jpg"))
      {
	  *retcode += -19;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 256.0, ".png"))
      {
	  *retcode += -20;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 60.0, ".png"))
      {
	  *retcode += -21;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 256.0, ".tif"))
      {
	  *retcode += -22;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 60.0, ".tif"))
      {
	  *retcode += -23;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 256.0, ".pdf"))
      {
	  *retcode += -24;
	  return 0;
      }
    if (!do_export_image (sqlite, coverage, geom, 60.0, ".pdf"))
      {
	  *retcode += -25;
	  return 0;
      }
    fprintf (stderr, "*************************************** 4\n");


    if (strcmp (coverage, "gray_jpeg_512") == 0)
      {
	  /* testing a Monolithic Pyramid */
	  sql =
	      sqlite3_mprintf ("SELECT RL2_PyramidizeMonolithic(%Q, 2, 1)",
			       coverage);
	  ret = execute_check (sqlite, sql);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "PyramidizeMonolithic \"%s\" error: %s\n",
			 coverage, err_msg);
		sqlite3_free (err_msg);
		*retcode += -26;
		return 0;
	    }

	  /* export tests */
	  if (geom == NULL)
	    {
		*retcode += -27;
		return 0;
	    }
	  if (!do_export_geotiff (sqlite, coverage, "mono", geom, 1))
	    {
		*retcode += -28;
		return 0;
	    }
	  if (!do_export_geotiff (sqlite, coverage, "mono", geom, 2))
	    {
		*retcode += -29;
		return 0;
	    }
	  if (!do_export_geotiff (sqlite, coverage, "mono", geom, 4))
	    {
		*retcode += -30;
		return 0;
	    }
	  if (!do_export_geotiff (sqlite, coverage, "mono", geom, 8))
	    {
		*retcode += -31;
		return 0;
	    }
      }
    if (!do_export_section_geotiff (sqlite, coverage, geom, 1))
      {
	  *retcode += -32;
	  return 0;
      }
    if (!do_export_section_geotiff (sqlite, coverage, geom, 2))
      {
	  *retcode += -33;
	  return 0;
      }
    if (!do_export_section_geotiff (sqlite, coverage, geom, 4))
      {
	  *retcode += -34;
	  return 0;
      }
    if (!do_export_section_geotiff (sqlite, coverage, geom, 8))
      {
	  *retcode += -35;
	  return 0;
      }
    if (!do_export_section_tiff (sqlite, coverage, geom, 1))
      {
	  *retcode += -36;
	  return 0;
      }
    if (!do_export_section_tiff (sqlite, coverage, geom, 2))
      {
	  *retcode += -37;
	  return 0;
      }
    if (!do_export_section_tiff (sqlite, coverage, geom, 4))
      {
	  *retcode += -38;
	  return 0;
      }
    if (!do_export_section_tiff (sqlite, coverage, geom, 8))
      {
	  *retcode += -39;
	  return 0;
      }
    if (!do_export_section_tiff_tfw (sqlite, coverage, geom, 1))
      {
	  *retcode += -40;
	  return 0;
      }
    if (!do_export_section_tiff_tfw (sqlite, coverage, geom, 2))
      {
	  *retcode += -41;
	  return 0;
      }
    if (!do_export_section_tiff_tfw (sqlite, coverage, geom, 4))
      {
	  *retcode += -42;
	  return 0;
      }
    if (!do_export_section_tiff_tfw (sqlite, coverage, geom, 8))
      {
	  *retcode += -43;
	  return 0;
      }

    gaiaFreeGeomColl (geom);

    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, unsigned char pixel, unsigned char compression,
	       int tile_sz, int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    char *sql;

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_GRAYSCALE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_none_256";
		      break;
		  case TILE_512:
		      coverage = "gray_none_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_none_1024";
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
	    case RL2_COMPRESSION_JPEG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_jpeg_256";
		      break;
		  case TILE_512:
		      coverage = "gray_jpeg_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_jpeg_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_webp_256";
		      break;
		  case TILE_512:
		      coverage = "gray_webp_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_webp_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "gray_llwebp_256";
		      break;
		  case TILE_512:
		      coverage = "gray_llwebp_512";
		      break;
		  case TILE_1024:
		      coverage = "gray_llwebp_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt_none_256";
		      break;
		  case TILE_512:
		      coverage = "plt_none_512";
		      break;
		  case TILE_1024:
		      coverage = "plt_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_PNG:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "plt_png_256";
		      break;
		  case TILE_512:
		      coverage = "plt_png_512";
		      break;
		  case TILE_1024:
		      coverage = "plt_png_1024";
		      break;
		  };
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

/* GRAYSCALE tests */
    ret = -100;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -120;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -140;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
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
    ret = -300;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_256, &ret))
	return ret;
    ret = -320;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_512, &ret))
	return ret;
    ret = -340;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_1024, &ret))
	return ret;

#ifndef OMIT_WEBP		/* only if WebP is enabled */
    ret = -400;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_256,
	 &ret))
	return ret;
    ret = -420;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_512,
	 &ret))
	return ret;
    ret = -440;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_1024,
	 &ret))
	return ret;
    ret = -500;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_256, &ret))
	return ret;
    ret = -520;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_512, &ret))
	return ret;
    ret = -540;
    if (!test_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_1024, &ret))
	return ret;
#endif /* end WebP conditional */

/* PALETTE tests */
    ret = -600;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -620;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -640;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -700;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -720;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -740;
    if (!test_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;

/* dropping all GRAYSCALE Coverages */
    ret = -170;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -180;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -190;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -270;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -280;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -290;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG, TILE_1024, &ret))
	return ret;
    ret = -370;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_256, &ret))
	return ret;
    ret = -380;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_512, &ret))
	return ret;
    ret = -390;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG, TILE_1024, &ret))
	return ret;

#ifndef OMIT_WEBP		/* only if WebP is enabled */
    ret = -470;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_256,
	 &ret))
	return ret;
    ret = -480;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_512,
	 &ret))
	return ret;
    ret = -490;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSY_WEBP, TILE_1024,
	 &ret))
	return ret;
    ret = -570;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_256, &ret))
	return ret;
    ret = -580;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_512, &ret))
	return ret;
    ret = -590;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_LOSSLESS_WEBP,
	 TILE_1024, &ret))
	return ret;
#endif /* end WebP conditional */

/* dropping all PALETTE Coverages */
    ret = -670;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -680;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_512, &ret))
	return ret;
    ret = -690;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -770;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_256, &ret))
	return ret;
    ret = -780;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_512, &ret))
	return ret;
    ret = -790;
    if (!drop_coverage
	(db_handle, RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, TILE_1024, &ret))
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
