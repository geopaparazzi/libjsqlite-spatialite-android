/*

 check_drop_rename.c -- SpatiaLite Test Case
 
 Testing DropTable, RenameTable and RenameColumn

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
 
Portions created by the Initial Developer are Copyright (C) 2018
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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

static int
do_test_rename_table (sqlite3 * handle, void *cache)
{
/* testing Rename Table */
    int ret;
    char *err_msg = NULL;

/* skipping on obsolete sqlite */
    if (sqlite3_libversion_number () < 3025000)
	return 0;

/* renaming the ordinary table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameTable(NULL, 'table_1', 'newtable_1')", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RenameTable \"table_1\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -10;
      }

/* renaming the spatial table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameTable(NULL, 'table_2', 'newtable_2')", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RenameTable \"table_2\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -11;
      }

/* attempting to rename a spatial view */
    ret =
	sqlite3_exec (handle, "SELECT RenameTable(NULL, 'view_0', 'newview_0')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "RenameTable \"view_0\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -12;
      }
    ret =
	strcmp (err_msg,
		"RenameTable exception - forbidden: can't rename a View, only Tables are supported [main.view_0].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameTable \"view_0\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -13;
      }
    sqlite3_free (err_msg);

/* attempting to rename a non-existing table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameTable(NULL, 'wrong_table', 'newwrong')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "RenameTable \"wrong_table\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -14;
      }
    ret =
	strcmp (err_msg,
		"RenameTable exception - not existing table [main.wrong_table].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameTable \"wrong_table\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -15;
      }
    sqlite3_free (err_msg);

/* attempting to rename an R*Tree table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameTable(NULL, 'idx_newtable_2_geom_node', 'new_rtree')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "RenameTable \"idx_newtable_2_geom_node\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -16;
      }
    ret =
	strcmp (err_msg,
		"RenameTable exception - forbidden: R*Tree (Spatial Index) internal Table [main.idx_newtable_2_geom_node].");
    if (ret != 0)
      {
	  fprintf (stderr,
		   "RenameTable \"idx_newtable_2_geom_node\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -17;
      }
    sqlite3_free (err_msg);

/* attempting to rename some SpatiaLite's internal table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameTable(NULL, 'spatial_ref_sys', 'new_ref_sys')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "RenameTable \"spatial_ref_sys\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -18;
      }
    ret =
	strcmp (err_msg,
		"RenameTable exception - forbidden: SpatiaLite internal Table [main.spatial_ref_sys].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameTable \"spatial_ref_sys\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -19;
      }
    sqlite3_free (err_msg);

/* attempting to rename to the same name*/
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameTable(NULL, 'newtable_2', 'newtable_2')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "RenameTable \"newtable_2\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -20;
      }
    ret =
	strcmp (err_msg,
		"RenameTable exception - already existing table [main.newtable_2].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameTable \"newtable_2\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -21;
      }
    sqlite3_free (err_msg);

/* attempting to rename a Virtual Table*/
    ret =
	sqlite3_exec (handle, "SELECT RenameTable(NULL, 'knn', 'new_knn')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "RenameTable \"knn\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -22;
      }
    ret =
	strcmp (err_msg,
		"RenameTable exception - forbidden: SpatiaLite internal Table [main.knn].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameTable \"knn\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -23;
      }
    sqlite3_free (err_msg);

    return 0;
}

static int
do_test_rename_column (sqlite3 * handle, void *cache)
{
/* testing Rename Column */
    int ret;
    char *err_msg = NULL;

/* skipping on obsolete sqlite */
    if (sqlite3_libversion_number () < 3025000)
	return 0;

/* renaming the ordinary table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'table_1', 'name', 'label')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RenameColumne \"table_1\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -20;
      }

/* renaming the spatial table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'table_2', 'name', 'label')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "RenameColumn \"table_2\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -21;
      }

/* attempting to rename a spatial view */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'view_0', 'name1', 'label')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "RenameColumn \"view_0\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -22;
      }
    ret =
	strcmp (err_msg,
		"RenameColumn exception - forbidden: can't rename a View, only Tables are supported [main.view_0].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameColumn \"view_0\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -23;
      }
    sqlite3_free (err_msg);

/* attempting to rename a non-existing table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'wrong_table', 'name', 'label')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "RenameColumn \"wrong_table\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -14;
      }
    ret =
	strcmp (err_msg,
		"RenameColumn exception - not existing table [main.wrong_table].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameColumn \"wrong_table\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -15;
      }
    sqlite3_free (err_msg);

/* attempting to rename a non-existing column */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'table_1', 'wrong', 'wow')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "RenameColumn \"table_1\" \"wrong\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -16;
      }
    ret =
	strcmp (err_msg,
		"RenameColumn exception - not existing column [main.table_1] wrong.");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameColumn \"table_1\" \"wrong\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -17;
      }
    sqlite3_free (err_msg);

/* attempting to rename an R*Tree table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'idx_table_2_geom_node', 'data', 'binary_data')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "RenameColumn \"idx_table_2_geom_node\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -18;
      }
    ret =
	strcmp (err_msg,
		"RenameColumn exception - forbidden: R*Tree (Spatial Index) internal Table [main.idx_table_2_geom_node].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameColumn \"idx_table_2_geom_node\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -19;
      }
    sqlite3_free (err_msg);

/* attempting to rename some SpatiaLite's internal table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'spatial_ref_sys', 'auth_name', 'name_auth')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "RenameColumn \"spatial_ref_sys\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -20;
      }
    ret =
	strcmp (err_msg,
		"RenameColumn exception - forbidden: SpatiaLite internal Table [main.spatial_ref_sys].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameColumn \"spatial_ref_sys\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -21;
      }
    sqlite3_free (err_msg);

/* attempting to rename to the same name*/
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'table_2', 'id', 'id')", NULL,
		      NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "RenameColumn \"table_2\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -22;
      }
    ret =
	strcmp (err_msg,
		"RenameColumn exception - column already defined [main.table_2] id.");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameColumn \"table_2\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -23;
      }
    sqlite3_free (err_msg);

/* attempting to rename a Virtual Table */
    ret =
	sqlite3_exec (handle,
		      "SELECT RenameColumn(NULL, 'knn', 'f_table_name', 'table_name')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "RenameColumn \"knn\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -24;
      }
    ret =
	strcmp (err_msg,
		"RenameColumn exception - forbidden: SpatiaLite internal Table [main.knn].");
    if (ret != 0)
      {
	  fprintf (stderr, "RenameColumn \"knn\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -25;
      }
    sqlite3_free (err_msg);

    return 0;
}

static int
do_test_drop_table (sqlite3 * handle, void *cache)
{
/* testing Drop Table */
    int ret;
    char *err_msg = NULL;

/* skipping on obsolete sqlite */
    if (sqlite3_libversion_number () < 3025000)
	return 0;

/* dropping the ordinary table - expected failure */
    ret =
	sqlite3_exec (handle, "SELECT DropTable(NULL, 'table_1')", NULL, NULL,
		      &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "DropTable \"table_1\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -30;
      }
    ret =
	strcmp (err_msg,
		"DropTable exception - FOREIGN KEY constraint failed.");
    if (ret != 0)
      {
	  fprintf (stderr, "DropTable \"table_1\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -31;
      }
    sqlite3_free (err_msg);

/* dropping the Spatial View */
    ret =
	sqlite3_exec (handle, "SELECT DropTable(NULL, 'view_0')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropTable \"view_0\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -32;
      }

/* attempting to drop an R*Tree table */
    ret =
	sqlite3_exec (handle, "SELECT DropTable(NULL, 'idx_table_2_geom_node')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "DropTable \"idx_table_2_geom_node\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -33;
      }
    ret =
	strcmp (err_msg,
		"DropTable exception - forbidden: R*Tree (Spatial Index) internal Table [main.idx_table_2_geom_node].");
    if (ret != 0)
      {
	  fprintf (stderr, "DropTable \"idx_table_2_geom_node\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -34;
      }
    sqlite3_free (err_msg);

/* dropping the spatial table */
    ret =
	sqlite3_exec (handle, "SELECT DropTable(NULL, 'table_2')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropTable \"table_2\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -35;
      }

/* dropping the ordinary table */
    ret =
	sqlite3_exec (handle, "SELECT DropTable(NULL, 'table_1')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropTable \"table_1\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -36;
      }

/* attempting to drop a non-existing table */
    ret =
	sqlite3_exec (handle, "SELECT DropTable(NULL, 'wrong_table')", NULL,
		      NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "DropTable \"wrong_table\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -37;
      }
    ret =
	strcmp (err_msg,
		"DropTable exception - not existing table [main.wrong_table].");
    if (ret != 0)
      {
	  fprintf (stderr, "DropTable \"wrong_table\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -38;
      }
    sqlite3_free (err_msg);

/* attempting to rename some SpatiaLite's internal table */
    ret =
	sqlite3_exec (handle, "SELECT DropTable(NULL, 'spatial_ref_sys')", NULL,
		      NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "DropTable \"spatial_ref_sys\" unexpected success\n");
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -20;
      }
    ret =
	strcmp (err_msg,
		"DropTable exception - forbidden: SpatiaLite internal Table [main.spatial_ref_sys].");
    if (ret != 0)
      {
	  fprintf (stderr, "DropTable \"spatial_ref_sys\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -21;
      }
    sqlite3_free (err_msg);

    return 0;
}

static int
do_create_test_db (sqlite3 ** xhandle, void **xcache)
{
/* creating a new test DB */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();
    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open \":memory:\" database: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -1;
      }

    spatialite_init_ex (handle, cache, 0);

/* DB initialization */
    ret = sqlite3_exec (handle, "PRAGMA foreign_keys=1", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "PRAGMA foreign_keys=1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -2;
      }
    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -3;
      }

/* creating an ordinary table */
    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE table_1 (id INTEGER PRIMARY KEY, name TEXT NOT NULL); "
		      "INSERT INTO table_1 VALUES (1, 'alpha'); "
		      "INSERT INTO table_1 VALUES (2, 'beta'); "
		      "INSERT INTO table_1 VALUES (3, 'gamma');", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"table_1\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -4;
      }

/* creating a Spatial table */
    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE table_2 (id INTEGER PRIMARY KEY, fkid INTEGER, name TEXT NOT NULL, "
		      "CONSTRAINT fkx FOREIGN KEY (fkid) REFERENCES table_1 (id)); "
		      "SELECT AddGeometryColumn('table_2', 'geom', 4326, 'POINT', 'XY'); "
		      "SELECT CreateSpatialIndex('table_2', 'geom'); "
		      "INSERT INTO table_2 VALUES (1, 1, 'alpha', MakePoint(0, 0, 4326)); "
		      "INSERT INTO table_2 VALUES (2, 2, 'beta', MakePoint(1, 1, 4326)); "
		      "INSERT INTO table_2 VALUES (3, 3, 'gamma', MakePoint(2, 2, 4326));",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"table_2\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -5;
      }

/* creating a Spatial View */
    ret =
	sqlite3_exec (handle, "CREATE VIEW view_0 AS "
		      "SELECT a.id AS rowid, a.name AS name1, b.name AS name2, a.geom AS geom "
		      "FROM table_2 AS a LEFT JOIN table_1 AS b ON (a.id = b.id); "
		      "INSERT INTO views_geometry_columns VALUES ('view_0', 'geom', 'rowid', 'table_2', 'geom', 1);",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE VIEW \"view_0\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  spatialite_cleanup_ex (cache);
	  return -6;
      }

    *xhandle = handle;
    *xcache = cache;
    return 0;
}

static int
do_close_test_db (sqlite3 * handle, void *cache)
{
/* closing a new test DB */
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);
    return 1;
}

int
main (int argc, char *argv[])
{
    int retcode = 0;
    int ret;
    sqlite3 *handle;
    void *cache;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

/* testing RenameTable */
    ret = do_create_test_db (&handle, &cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }
    ret = do_test_rename_table (handle, cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }
    ret = do_close_test_db (handle, cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }

/* testing RenameColumn */
    ret = do_create_test_db (&handle, &cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }
    ret = do_test_rename_column (handle, cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }
    ret = do_close_test_db (handle, cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }

/* testing DropTable */
    ret = do_create_test_db (&handle, &cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }
    ret = do_test_drop_table (handle, cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }
    ret = do_close_test_db (handle, cache);
    if (ret < 0)
      {
	  retcode = ret;
	  goto end;
      }

  end:
    spatialite_shutdown ();
    return retcode;
}
