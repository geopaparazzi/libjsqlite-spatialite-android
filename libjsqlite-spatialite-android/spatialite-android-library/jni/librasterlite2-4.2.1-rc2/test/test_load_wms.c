/*

 test_load_wms.c -- RasterLite-2 Test Case

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

#include "rasterlite2/rasterlite2.h"

#define TILE_256	256
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

int
main (int argc, char *argv[])
{
    int result = 0;
    int ret;
    char *err_msg = NULL;
    sqlite3 *db_handle;
    char *sql;
    gaiaGeomCollPtr geom;
    void *cache = spatialite_alloc_connection ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (getenv ("ENABLE_RL2_WEB_TESTS") == NULL)
      {
	  fprintf (stderr, "this testcase has been skipped !!!\n\n"
		   "you can enable all testcases requiring an Internet connection\n"
		   "by setting the environment variable \"ENABLE_RL2_WEB_TESTS=1\"\n");
	  return 0;
      }

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

/* creating an RGB DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.2f)",
			   "rgb10k", "UINT8", "RGB", 3, "JPEG", 80,
			   512, 512, 3003, 0.5);
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateCoverage \"%s\" error: %s\n", "rgb10k",
		   err_msg);
	  sqlite3_free (err_msg);
	  return -4;
      }

/* creating a Grayscale DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.2f)",
			   "gray10k", "UINT8", "GRAYSCALE", 1, "JPEG", 80,
			   512, 512, 3003, 1.0);
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateCoverage \"%s\" error: %s\n", "gray10k",
		   err_msg);
	  sqlite3_free (err_msg);
	  return -5;
      }

/* creating a Monochrome DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.2f, %1.2f, "
			   "RL2_SetPixelValue(RL2_CreatePixel(%Q, %Q, 1), 0, 0))",
			   "ctrt10k", "1-BIT", "MONOCHROME", 1, "FAX4", 100,
			   512, 512, 3003, 0.5, 0.5, "1-BIT", "MONOCHROME");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateCoverage \"%s\" error: %s\n", "ctrt10k",
		   err_msg);
	  sqlite3_free (err_msg);
	  return -6;
      }

/* loading from WMS (RGB) */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRasterFromWMS(%Q, %Q, %Q, %s, %Q, %Q, NULL, %Q, %1.1f)",
	 "rgb10k", "firenze",
	 "http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsofc&map_resolution=91&language=ita&",
	 "BuildCircleMbr(1681700, 4849150, 500.0, 3003)", "1.3.0",
	 "rt_ofc.10k10", "image/jpeg", 0.5);
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRasterFromWMS \"%s\" error: %s\n", "rgb10k",
		   err_msg);
	  sqlite3_free (err_msg);
	  return -7;
      }

/* loading from WMS (Grayscale) */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRasterFromWMS(%Q, %Q, %Q, %s, %Q, %Q, NULL, %Q, %1.1f)",
	 "gray10k", "firenze",
	 "http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsofc&map_resolution=91&language=ita&",
	 "BuildCircleMbr(1681700, 4849150, 1000.0, 3003)", "1.3.0",
	 "rt_ofc.10k88", "image/jpeg", 1.0);
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRasterFromWMS \"%s\" error: %s\n", "gray10k",
		   err_msg);
	  sqlite3_free (err_msg);
	  return -8;
      }

/* loading from WMS (Monochrome) */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRasterFromWMS(%Q, %Q, %Q, %s, %Q, %Q, NULL, %Q, %1.1f)",
	 "ctrt10k", "firenze",
	 "http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsctr&map_resolution=91&language=ita&",
	 "BuildCircleMbr(1681700, 4849150, 500.0, 3003)", "1.3.0", "rt_ctr.10k",
	 "image/png", 0.5);
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRasterFromWMS \"%s\" error: %s\n", "ctrt10k",
		   err_msg);
	  sqlite3_free (err_msg);
	  return -9;
      }
    sql = sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, 1, 1, 1)", "ctrt10k");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", "ctrt10k", err_msg);
	  sqlite3_free (err_msg);
	  return -10;
      }

/* export tests */
    geom = get_center_point (db_handle, "rgb10k");
    if (geom == NULL)
	return -11;
    if (!do_export_geotiff (db_handle, "rgb10k", geom, 1))
	return -12;
    if (!do_export_geotiff (db_handle, "rgb10k", geom, 2))
	return -13;
    if (!do_export_geotiff (db_handle, "rgb10k", geom, 4))
	return -14;
    if (!do_export_geotiff (db_handle, "rgb10k", geom, 8))
	return -15;
    gaiaFreeGeomColl (geom);
    geom = get_center_point (db_handle, "rgb10k");
    if (geom == NULL)
	return -16;
    if (!do_export_geotiff (db_handle, "gray10k", geom, 1))
	return -17;
    if (!do_export_geotiff (db_handle, "gray10k", geom, 2))
	return -18;
    if (!do_export_geotiff (db_handle, "gray10k", geom, 4))
	return -19;
    if (!do_export_geotiff (db_handle, "gray10k", geom, 8))
	return -20;
    gaiaFreeGeomColl (geom);
    geom = get_center_point (db_handle, "ctrt10k");
    if (geom == NULL)
	return -21;
    if (!do_export_geotiff (db_handle, "ctrt10k", geom, 1))
	return -22;
    if (!do_export_geotiff (db_handle, "ctrt10k", geom, 2))
	return -23;
    if (!do_export_geotiff (db_handle, "ctrt10k", geom, 4))
	return -24;
    if (!do_export_geotiff (db_handle, "ctrt10k", geom, 8))
	return -25;
    gaiaFreeGeomColl (geom);

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
