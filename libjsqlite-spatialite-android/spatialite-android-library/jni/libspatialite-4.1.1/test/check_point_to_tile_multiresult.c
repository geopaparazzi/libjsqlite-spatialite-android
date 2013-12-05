/*

 check_point_to_tile_multiresult.c - Test case for GeoPackage Extensions

 Author: Brad Hards <bradh@frogmouth.net>

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

The Original Code is GeoPackage extensions

The Initial Developer of the Original Code is Brad Hards
 
Portions created by the Initial Developer are Copyright (C) 2012
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

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sqlite3.h>
#include <spatialite.h>

#include "test_helpers.h"

int main (int argc UNUSED, char *argv[] UNUSED)
{
    sqlite3 *db_handle = NULL;
    char *sql_statement;
    sqlite3_stmt *stmt;
    int ret;
    char *err_msg = NULL;
    int i; 
    const uint8_t *blob0;
    const uint8_t *blob1;
    void *cache = spatialite_alloc_connection();
    char *old_SPATIALITE_SECURITY_ENV = NULL;
#ifdef _WIN32
	char *env;
#endif /* not WIN32 */

    old_SPATIALITE_SECURITY_ENV = getenv("SPATIALITE_SECURITY");
#ifdef _WIN32
	putenv("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv("SPATIALITE_SECURITY", "relaxed", 1);
#endif
    
    ret = sqlite3_open_v2 (":memory:", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    // For debugging / testing if required
    // ret = sqlite3_open_v2 ("check_point_to_tile_multiresult.sqlite", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    spatialite_init_ex(db_handle, cache, 0);
    if (old_SPATIALITE_SECURITY_ENV)
    {
#ifdef _WIN32 
	  env = sqlite3_mprintf("SPATIALITE_SECURITY=%s", old_SPATIALITE_SECURITY_ENV);
	  putenv(env);
	  sqlite3_free(env);
#else /* not WIN32 */
      setenv("SPATIALITE_SECURITY", old_SPATIALITE_SECURITY_ENV, 1);
#endif
    }
    else
    {
#ifdef _WIN32
	  putenv("SPATIALITE_SECURITY=");
#else /* not WIN32 */
      unsetenv("SPATIALITE_SECURITY");
#endif
    }
    if (ret != SQLITE_OK) {
      fprintf (stderr, "cannot open in-memory db: %s\n", sqlite3_errmsg (db_handle));
      sqlite3_close (db_handle);
      db_handle = NULL;
      return -1;
    }
    
    /* Create the base tables */
    ret = sqlite3_exec (db_handle, "SELECT InitSpatialMetadata(1, 'WGS84')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "Unexpected InitSpatialMetadata result: %i, (%s)\n", ret, err_msg);
	sqlite3_free (err_msg);
	return -2;
    }
    
    ret = sqlite3_exec (db_handle, "SELECT gpkgCreateBaseTables()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "Unexpected gpkgCreateBaseTables() result: %i, (%s)\n", ret, err_msg);
	sqlite3_free (err_msg);
	return -3;
    }
    
    /* create a target table */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS test1_tiles", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP test1_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -10;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE test1_tiles (id INTEGER PRIMARY KEY AUTOINCREMENT, zoom_level INTEGER NOT NULL DEFAULT 0, tile_column INTEGER NOT NULL DEFAULT 0, tile_row INTEGER NOT NULL DEFAULT 0, tile_data BLOB NOT NULL DEFAULT (zeroblob(4)))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE mytable_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -11;
    }
    /* add in some test entries */
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_tiles (id, zoom_level, tile_column, tile_row, tile_data) VALUES (1, 0, 0, 0, BlobFromFile('tile000.jpeg'))",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT level0 error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -12;
    }
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_tiles (id, zoom_level, tile_column, tile_row, tile_data) VALUES (2, 1, 0, 0, BlobFromFile('tile100.jpeg'))",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT level1 error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -13;
    }
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_tiles (id, zoom_level, tile_column, tile_row, tile_data) VALUES (3, 1, 0, 1, BlobFromFile('tile101.jpeg'))",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT level1 error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -14;
    }
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_tiles (id, zoom_level, tile_column, tile_row, tile_data) VALUES (4, 1, 1, 1, BlobFromFile('tile111.jpeg'))",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT level1 error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -15;
    }
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS test1_tiles_rt_metadata", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP test1_tiles_rt_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -16;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE test1_tiles_rt_metadata (id INTEGER PRIMARY KEY AUTOINCREMENT, min_x DOUBLE NOT NULL DEFAULT -180.0, min_y DOUBLE NOT NULL DEFAULT -90.0, max_x DOUBLE NOT NULL DEFAULT 180.0, max_y DOUBLE NOT NULL DEFAULT 90.0)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE test1_tiles_rt_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -17;
    }
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_tiles_rt_metadata (id, min_x, min_y, max_x, max_y) VALUES (1, -180.0, -90.0, 180.0, 90.0)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT level0 _rt_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -18;
    }
    /* duplicate area */
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_tiles_rt_metadata (id, min_x, min_y, max_x, max_y) VALUES (2, -40.0, -90.0, 100.0, 0.0)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT level1 (0,0) _rt_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -19;
    }
    /* Deliberately skip 3 */
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_tiles_rt_metadata (id, min_x, min_y, max_x, max_y) VALUES (4, 0.0, -90.0, 180.0, 0.0)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT level1 (1,1) _rt_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -20;
    }
    ret = sqlite3_exec (db_handle, "INSERT INTO raster_columns VALUES (\"test1_tiles\", \"tile_data\", 99, 1, 4326)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT raster_columns row error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -21;
    }
    
    /* try gpkgPointToTile multiple result */
    sql_statement = "SELECT gpkgPointToTile(\"test1_tiles\", 4326, 20.0, -30.0, 1), BlobFromFile(\"tile100.jpeg\")";
    ret = sqlite3_prepare_v2 (db_handle, sql_statement, strlen(sql_statement), &stmt, NULL);
    if (ret != SQLITE_OK)
    {
	fprintf(stderr, "failed to prepare SQL statement: %i (%s)\n", ret, sqlite3_errmsg(db_handle));
        return -30;
    }
    ret = sqlite3_step (stmt);
    if (ret != SQLITE_ROW)
    {
	fprintf(stderr, "unexpected return value for first step: %i\n", ret);
	return -31;
    }
    if (sqlite3_column_type (stmt, 0) != SQLITE_BLOB)
    {
	fprintf(stderr, "bad type for column 0: %i\n", sqlite3_column_type (stmt, 0));
	return -32;
    }
    if (sqlite3_column_type (stmt, 1) != SQLITE_BLOB)
    {
	fprintf(stderr, "bad type for column 1: %i\n", sqlite3_column_type (stmt, 1));
	return -33;
    }
    blob0 = sqlite3_column_blob(stmt, 0);
    blob1 = sqlite3_column_blob(stmt, 1);
    if (sqlite3_column_bytes(stmt, 0) != sqlite3_column_bytes(stmt, 1))
    {
	fprintf(stderr, "mismatch in blob sizes: %i vs %i\n", sqlite3_column_bytes(stmt, 0), sqlite3_column_bytes(stmt, 1));
	return -34;
    }
    for (i = 0; i < sqlite3_column_bytes(stmt, 0); ++i)
    {
	if (blob0[i] != blob1[i])
	{
	    fprintf(stderr, "mismatch in blob content at offset: %i\n", i);
	    return -35;
	}
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE)
    {
	fprintf(stderr, "unexpected return value for second step: %i\n", ret);
	return -36;
    }
    ret = sqlite3_finalize (stmt);
    
    ret = sqlite3_close (db_handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (db_handle));
	return -100;
    }

    spatialite_cleanup_ex(cache);
    return 0;
}
