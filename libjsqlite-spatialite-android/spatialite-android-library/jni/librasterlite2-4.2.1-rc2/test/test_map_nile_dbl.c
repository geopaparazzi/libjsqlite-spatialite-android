/*

 test_map_nile_dbl.c -- RasterLite-2 Test Case

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
test_coverage (sqlite3 * sqlite, unsigned char sample, int tile_sz,
	       int *retcode)
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

/* setting the coverage name */
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "grid_8_256";
		break;
	    case TILE_1024:
		coverage = "grid_8_1024";
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT8:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "grid_u8_256";
		break;
	    case TILE_1024:
		coverage = "grid_u8_1024";
		break;
	    };
	  break;
      case RL2_SAMPLE_INT16:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "grid_16_256";
		break;
	    case TILE_1024:
		coverage = "grid_16_1024";
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT16:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "grid_u16_256";
		break;
	    case TILE_1024:
		coverage = "grid_u16_1024";
		break;
	    };
	  break;
      case RL2_SAMPLE_INT32:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "grid_32_256";
		break;
	    case TILE_1024:
		coverage = "grid_32_1024";
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT32:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "grid_u32_256";
		break;
	    case TILE_1024:
		coverage = "grid_u32_1024";
		break;
	    };
	  break;
      case RL2_SAMPLE_FLOAT:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "grid_flt_256";
		break;
	    case TILE_1024:
		coverage = "grid_flt_1024";
		break;
	    };
	  break;
      case RL2_SAMPLE_DOUBLE:
	  switch (tile_sz)
	    {
	    case TILE_256:
		coverage = "grid_dbl_256";
		break;
	    case TILE_1024:
		coverage = "grid_dbl_1024";
		break;
	    };
	  break;
      };

/* preparing misc Coverage's parameters */
    pixel_name = "DATAGRID";
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  sample_name = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  sample_name = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  sample_name = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  sample_name = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  sample_name = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  sample_name = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  sample_name = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample_name = "DOUBLE";
	  break;
      };
    num_bands = 1;
    compression_name = "NONE";
    switch (tile_sz)
      {
      case TILE_256:
	  tile_size = 256;
	  break;
      case TILE_1024:
	  tile_size = 1024;
	  break;
      };

/* creating the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.8f, %1.8f)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 4326,
			   0.0008333333333333, 0.0008333333333333);
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

/* loading from directory */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRastersFromDir(%Q, %Q, %Q, 0, 4326, 0, 1)", coverage,
	 "map_samples/usgs-nile-dbl", ".tif");
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
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 0, 4326, 0, 1)",
			   coverage, "map_samples/usgs-nile-dbl/nile1-dbl.tif");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
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

/* SRTM NILE-DOUBLE (GRID) tests */
    ret = -100;
    if (!test_coverage (db_handle, RL2_SAMPLE_INT8, TILE_256, &ret))
	return ret;
    ret = -120;
    if (!test_coverage (db_handle, RL2_SAMPLE_INT8, TILE_1024, &ret))
	return ret;
    ret = -140;
    if (!test_coverage (db_handle, RL2_SAMPLE_UINT8, TILE_256, &ret))
	return ret;
    ret = -160;
    if (!test_coverage (db_handle, RL2_SAMPLE_UINT8, TILE_1024, &ret))
	return ret;
    ret = -180;
    if (!test_coverage (db_handle, RL2_SAMPLE_INT16, TILE_256, &ret))
	return ret;
    ret = -200;
    if (!test_coverage (db_handle, RL2_SAMPLE_INT16, TILE_1024, &ret))
	return ret;
    ret = -220;
    if (!test_coverage (db_handle, RL2_SAMPLE_UINT16, TILE_256, &ret))
	return ret;
    ret = -240;
    if (!test_coverage (db_handle, RL2_SAMPLE_UINT16, TILE_1024, &ret))
	return ret;
    ret = -260;
    if (!test_coverage (db_handle, RL2_SAMPLE_INT32, TILE_256, &ret))
	return ret;
    ret = -280;
    if (!test_coverage (db_handle, RL2_SAMPLE_INT32, TILE_1024, &ret))
	return ret;
    ret = -300;
    if (!test_coverage (db_handle, RL2_SAMPLE_UINT32, TILE_256, &ret))
	return ret;
    ret = -320;
    if (!test_coverage (db_handle, RL2_SAMPLE_UINT32, TILE_1024, &ret))
	return ret;
    ret = -340;
    if (!test_coverage (db_handle, RL2_SAMPLE_FLOAT, TILE_256, &ret))
	return ret;
    ret = -360;
    if (!test_coverage (db_handle, RL2_SAMPLE_FLOAT, TILE_1024, &ret))
	return ret;
    ret = -380;
    if (!test_coverage (db_handle, RL2_SAMPLE_DOUBLE, TILE_256, &ret))
	return ret;
    ret = -400;
    if (!test_coverage (db_handle, RL2_SAMPLE_DOUBLE, TILE_1024, &ret))
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
