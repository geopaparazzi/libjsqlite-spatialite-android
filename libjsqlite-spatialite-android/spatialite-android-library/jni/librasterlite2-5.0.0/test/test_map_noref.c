/*

 test_map_noref.c -- RasterLite-2 Test Case

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
do_export_tiff (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		int scale)
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

    path = sqlite3_mprintf ("./%s_%d.tif", coverage, scale);

    if (!get_base_resolution (sqlite, coverage, &x_res, &y_res))
	return 0;
    xx_res = x_res * (double) scale;
    yy_res = y_res * (double) scale;

    sql = "SELECT RL2_WriteTiff(NULL, ?, ?, ?, ?, ?, ?, ?, ?)";
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
    sqlite3_bind_text (stmt, 8, "NONE", 7, SQLITE_TRANSIENT);
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
	"SELECT RL2_GetMapImageFromRaster(NULL, ?, ST_Buffer(?, ?), 512, 512, 'default', ?, '#ffffff', 1, 80)";
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
test_coverage (sqlite3 * sqlite, const char *prefix, unsigned char pixel,
	       unsigned char compression, int *retcode)
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
    char *cov_name;
    const char *file_path;

/* setting the file path */
    if (strcmp (prefix, "noref-rgb") == 0)
	file_path = "./rgb-striped.tif";
    if (strcmp (prefix, "noref-gray") == 0)
	file_path = "./gray-striped.tif";
    if (strcmp (prefix, "noref-mono") == 0)
	file_path = "./mono3s.tif";
    if (strcmp (prefix, "noref-plt") == 0)
	file_path = "plt-striped.tif";

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_RGB:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		coverage = "rgb_none_256";
		break;
	    case RL2_COMPRESSION_PNG:
		coverage = "rgb_png_256";
		break;
	    case RL2_COMPRESSION_JPEG:
		coverage = "rgb_jpeg_256";
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		coverage = "rgb_webp_256";
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		coverage = "rgb_llwebp_256";
		break;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		coverage = "gray_none_256";
		break;
	    case RL2_COMPRESSION_PNG:
		coverage = "gray_png_256";
		break;
	    case RL2_COMPRESSION_JPEG:
		coverage = "gray_jpeg_256";
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		coverage = "gray_webp_256";
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		coverage = "gray_llwebp_256";
		break;
	    };
	  break;
      case RL2_PIXEL_MONOCHROME:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		coverage = "mono_none_256";
		break;
	    case RL2_COMPRESSION_PNG:
		coverage = "mono_png_256";
		break;
	    case RL2_COMPRESSION_CCITTFAX4:
		coverage = "mono_fax4_256";
		break;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		coverage = "plt_none_256";
		break;
	    case RL2_COMPRESSION_PNG:
		coverage = "plt_png_256";
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
      case RL2_PIXEL_MONOCHROME:
	  num_bands = 1;
	  pixel_name = "MONOCHROME";
	  sample_name = "1-BIT";
	  break;
      case RL2_PIXEL_PALETTE:
	  num_bands = 1;
	  pixel_name = "PALETTE";
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
      case RL2_COMPRESSION_LOSSY_WEBP:
	  compression_name = "WEBP";
	  qlty = 80;
	  break;
      case RL2_COMPRESSION_LOSSLESS_WEBP:
	  compression_name = "LL_WEBP";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_CCITTFAX4:
	  compression_name = "FAX4";
	  qlty = 100;
	  break;
      };
    tile_size = 256;

/* creating the DBMS Coverage */
    cov_name = sqlite3_mprintf ("%s_%s", prefix, coverage);
    sql = sqlite3_mprintf ("SELECT RL2_CreateRasterCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.16f, %1.16f)",
			   cov_name, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, -1,
			   1.0, 1.0);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoverage \"%s\" error: %s\n", cov_name,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

/* loading the not-referenced image */
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 0, -1, 1, 1)",
			   cov_name, file_path);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster \"%s\" error: %s\n", cov_name, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
	  return 0;
      }

/* building the Pyramid Levels */
    sql = sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, NULL, 0, 1)", cov_name);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", cov_name, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -5;
	  return 0;
      }

/* export tests */
    geom = get_center_point (sqlite, cov_name);
    if (geom == NULL)
      {
	  *retcode += -6;
	  return 0;
      }
    if (!do_export_tiff (sqlite, cov_name, geom, 1))
      {
	  *retcode += -7;
	  return 0;
      }
    if (!do_export_tiff (sqlite, cov_name, geom, 2))
      {
	  *retcode += -8;
	  return 0;
      }
    if (!do_export_tiff (sqlite, cov_name, geom, 4))
      {
	  *retcode += -9;
	  return 0;
      }
    if (!do_export_tiff (sqlite, cov_name, geom, 8))
      {
	  *retcode += -10;
	  return 0;
      }
    if (!do_export_image (sqlite, cov_name, geom, 256, ".jpg"))
      {
	  *retcode += -11;
	  return 0;
      }
    if (!do_export_image (sqlite, cov_name, geom, 1111, ".jpg"))
      {
	  *retcode += -12;
	  return 0;
      }
    if (!do_export_image (sqlite, cov_name, geom, 256, ".png"))
      {
	  *retcode += -13;
	  return 0;
      }
    if (!do_export_image (sqlite, cov_name, geom, 1111, ".png"))
      {
	  *retcode += -14;
	  return 0;
      }
    if (!do_export_image (sqlite, cov_name, geom, 256, ".tif"))
      {
	  *retcode += -15;
	  return 0;
      }
    if (!do_export_image (sqlite, cov_name, geom, 1111, ".tif"))
      {
	  *retcode += -16;
	  return 0;
      }
    if (!do_export_image (sqlite, cov_name, geom, 256, ".pdf"))
      {
	  *retcode += -17;
	  return 0;
      }
    if (!do_export_image (sqlite, cov_name, geom, 1111, ".pdf"))
      {
	  *retcode += -18;
	  return 0;
      }
    gaiaFreeGeomColl (geom);
    sqlite3_free (cov_name);

    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, const char *prefix, unsigned char pixel,
	       unsigned char compression, int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    char *cov_name;
    char *sql;

/* setting the coverage name */
    switch (pixel)
      {
      case RL2_PIXEL_RGB:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		coverage = "rgb_none_256";
		break;
	    case RL2_COMPRESSION_PNG:
		coverage = "rgb_png_256";
		break;
	    case RL2_COMPRESSION_JPEG:
		coverage = "rgb_jpeg_256";
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		coverage = "rgb_webp_256";
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		coverage = "rgb_llwebp_256";
		break;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		coverage = "gray_none_256";
		break;
	    case RL2_COMPRESSION_PNG:
		coverage = "gray_png_256";
		break;
	    case RL2_COMPRESSION_JPEG:
		coverage = "gray_jpeg_256";
		break;
	    case RL2_COMPRESSION_LOSSY_WEBP:
		coverage = "gray_webp_256";
		break;
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
		coverage = "gray_llwebp_256";
		break;
	    };
	  break;
      case RL2_PIXEL_MONOCHROME:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		coverage = "mono_none_256";
		break;
	    case RL2_COMPRESSION_PNG:
		coverage = "mono_png_256";
		break;
	    case RL2_COMPRESSION_CCITTFAX4:
		coverage = "mono_fax4_256";
		break;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		coverage = "plt_none_256";
		break;
	    case RL2_COMPRESSION_PNG:
		coverage = "plt_png_256";
		break;
	    };
	  break;
      };

/* dropping the DBMS Coverage */
    cov_name = sqlite3_mprintf ("%s_%s", prefix, coverage);
    sql = sqlite3_mprintf ("SELECT RL2_DropRasterCoverage(%Q, 1)", cov_name);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropRasterCoverage \"%s\" error: %s\n", cov_name,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  sqlite3_free (cov_name);
	  return 0;
      }
    sqlite3_free (cov_name);

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
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, &ret))
	return ret;
    ret = -120;
    if (!test_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, &ret))
	return ret;
    ret = -140;
    if (!test_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, &ret))
	return ret;

#ifndef OMIT_WEBP		/* only if WebP is enabled */
    ret = -160;
    if (!test_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSY_WEBP,
	 &ret))
	return ret;
    ret = -170;
    if (!test_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSLESS_WEBP,
	 &ret))
	return ret;
#endif /* end WebP conditional */

/* GRAYSCALE tests */
    ret = -300;
    if (!test_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE,
	 &ret))
	return ret;
    ret = -320;
    if (!test_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG,
	 &ret))
	return ret;
    ret = -340;
    if (!test_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG,
	 &ret))
	return ret;

#ifndef OMIT_WEBP		/* only if WebP is enabled */
    ret = -360;
    if (!test_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE,
	 RL2_COMPRESSION_LOSSY_WEBP, &ret))
	return ret;
    ret = -380;
    if (!test_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE,
	 RL2_COMPRESSION_LOSSLESS_WEBP, &ret))
	return ret;
#endif /* end WebP conditional */

/* MONOCHROME tests */
    ret = -500;
    if (!test_coverage
	(db_handle, "noref-mono", RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_NONE,
	 &ret))
	return ret;
    ret = -520;
    if (!test_coverage
	(db_handle, "noref-mono", RL2_PIXEL_MONOCHROME,
	 RL2_COMPRESSION_CCITTFAX4, &ret))
	return ret;
    ret = -540;
    if (!test_coverage
	(db_handle, "noref-mono", RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_PNG,
	 &ret))
	return ret;

/* PALETTE tests */
    ret = -700;
    if (!test_coverage
	(db_handle, "noref-plt", RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, &ret))
	return ret;
    ret = -720;
    if (!test_coverage
	(db_handle, "noref-plt", RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, &ret))
	return ret;

/* dropping all RGB Coverages */
    ret = -200;
    if (!drop_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_NONE, &ret))
	return ret;
    ret = -210;
    if (!drop_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_PNG, &ret))
	return ret;
    ret = -220;
    if (!drop_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_JPEG, &ret))
	return ret;

#ifndef OMIT_WEBP		/* only if WebP is enabled */
    ret = -230;
    if (!drop_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSY_WEBP,
	 &ret))
	return ret;
    ret = -240;
    if (!drop_coverage
	(db_handle, "noref-rgb", RL2_PIXEL_RGB, RL2_COMPRESSION_LOSSLESS_WEBP,
	 &ret))
	return ret;
#endif /* end WebP conditional */

/* dropping all GRAYSCALE Coverages */
    ret = -400;
    if (!drop_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_NONE,
	 &ret))
	return ret;
    ret = -410;
    if (!drop_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_PNG,
	 &ret))
	return ret;
    ret = -420;
    if (!drop_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE, RL2_COMPRESSION_JPEG,
	 &ret))
	return ret;

#ifndef OMIT_WEBP		/* only if WebP is enabled */
    ret = -430;
    if (!drop_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE,
	 RL2_COMPRESSION_LOSSY_WEBP, &ret))
	return ret;
    ret = -440;
    if (!drop_coverage
	(db_handle, "noref-gray", RL2_PIXEL_GRAYSCALE,
	 RL2_COMPRESSION_LOSSLESS_WEBP, &ret))
	return ret;
#endif /* end WebP conditional */

/* dropping all MONOCHROME Coverages */
    ret = -600;
    if (!drop_coverage
	(db_handle, "noref-mono", RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_NONE,
	 &ret))
	return ret;
    ret = -610;
    if (!drop_coverage
	(db_handle, "noref-mono", RL2_PIXEL_MONOCHROME,
	 RL2_COMPRESSION_CCITTFAX4, &ret))
	return ret;
    ret = -620;
    if (!drop_coverage
	(db_handle, "noref-mono", RL2_PIXEL_MONOCHROME, RL2_COMPRESSION_PNG,
	 &ret))
	return ret;

/* dropping all PALETTE Coverages */
    ret = -800;
    if (!drop_coverage
	(db_handle, "noref-plt", RL2_PIXEL_PALETTE, RL2_COMPRESSION_NONE, &ret))
	return ret;
    ret = -810;
    if (!drop_coverage
	(db_handle, "noref-plt", RL2_PIXEL_PALETTE, RL2_COMPRESSION_PNG, &ret))
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
