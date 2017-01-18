/*

 test_copy_rastercov.c -- RasterLite-2 Test Case

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

int
main (int argc, char *argv[])
{
    int result = 0;
    int ret;
    char *err_msg = NULL;
    char *sql;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

/*=====================================================================
/
/ step I - we'll start by creating and populating the Origin DB
/
/====================================================================*/

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

/* opening and initializing the "copy_origin" test DB */
    ret = sqlite3_open_v2 ("copy_origin.sqlite", &db_handle,
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

/* creating the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateRasterCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.4f, %1.4f, "
			   "RL2_SetPixelValue(RL2_CreatePixel(%Q, %Q, 1), 0, 0))",
			   "test_coverage", "4-BIT", "PALETTE", 1,
			   "PNG", 100, 512, 512, 26716,
			   2.4384, 2.4384, "4-BIT", "PALETTE");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoverage \"%s\" error: %s\n",
		   "test_coverage", err_msg);
	  sqlite3_free (err_msg);
	  return -4;
      }

/* loading from directory */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRastersFromDir(%Q, %Q, %Q, 0, 26716, 0, 1)",
	 "test_coverage", "map_samples/usgs-indiana", ".tif");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRastersFromDir \"%s\" error: %s\n",
		   "test_coverage", err_msg);
	  sqlite3_free (err_msg);
	  return -5;
      }

/* building the Pyramid Levels */
    sql =
	sqlite3_mprintf ("SELECT RL2_Pyramidize(%Q, NULL, 0, 1)",
			 "test_coverage");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Pyramidize \"%s\" error: %s\n", "test_coverage",
		   err_msg);
	  sqlite3_free (err_msg);
	  return -6;
      }

/* closing the origin DB */
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
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


/*=====================================================================
/
/ step II - we'll now create the Destination DB
/
/====================================================================*/
    cache = spatialite_alloc_connection ();
    priv_data = rl2_alloc_private ();
/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -7;
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
	  return -8;
      }

/* Attaching the Origin DB */
    ret =
	sqlite3_exec (db_handle,
		      "ATTACH DATABASE './copy_origin.sqlite' AS 'origin'",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ATTACH DATABASE error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -9;
      }

/* copying a Raster Coverage */
    sql =
	sqlite3_mprintf ("SELECT RL2_CopyRasterCoverage(%Q, %Q, 1)", "origin",
			 "test_coverage");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CopyRasterCoverage error\n");
	  sqlite3_free (err_msg);
	  return -10;
      }

/* error: not existing origin (bad prefix) */
    sql =
	sqlite3_mprintf ("SELECT RL2_CopyRasterCoverage(%Q, %Q, 1)", "wrong",
			 "test_coverage");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "CopyRasterCoverage unexpected success #1\n");
	  sqlite3_free (err_msg);
	  return -11;
      }

/* error: not existing origin (bad coverage) */
    sql =
	sqlite3_mprintf ("SELECT RL2_CopyRasterCoverage(%Q, %Q, 1)", "origin",
			 "wrong");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "CopyRasterCoverage unexpected success #2\n");
	  sqlite3_free (err_msg);
	  return -12;
      }

/* error: already existing destination */
    sql =
	sqlite3_mprintf ("SELECT RL2_CopyRasterCoverage(%Q, %Q, 1)", "origin",
			 "test_coverage");
    ret = execute_check (db_handle, sql);
    sqlite3_free (sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "CopyRasterCoverage unexpected success #3\n");
	  sqlite3_free (err_msg);
	  return -13;
      }

/* closing the destination DB */
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
    spatialite_shutdown ();
    
    unlink ("copy_origin.sqlite");
    return result;
}
