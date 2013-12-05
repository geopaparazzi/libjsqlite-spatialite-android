/*

 check_add_tile_triggers.c - Test case for GeoPackage Extensions

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
 
Portions created by the Initial Developer are Copyright (C) 2011
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sqlite3.h>
#include <spatialite.h>

#include "test_helpers.h"

int main (int argc UNUSED, char *argv[] UNUSED)
{
    sqlite3 *db_handle = NULL;
    int ret;
    char *err_msg = NULL;
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
    // ret = sqlite3_open_v2 ("check_add_tile_triggers.sqlite", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
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

    /* create a minimal raster_format_metadata table (not spec compliant) */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS raster_format_metadata", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP raster_format_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -4;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE raster_format_metadata (r_table_name TEXT NOT NULL, r_raster_column TEXT NOT NULL, mime_type TEXT NOT NULL DEFAULT 'image/jpeg', bit_depth INTEGER NOT NULL DEFAULT 24)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE raster_format_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -5;
    }
    /* add in a test entry */
    ret = sqlite3_exec (db_handle, "INSERT INTO raster_format_metadata VALUES (\"test1_matrix_tiles\", \"tile_data\", \"image/png\", 24)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT raster_format_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -6;
    }
    
    /* create a minimal tile_matrix_metadata table (not spec compliant) */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS tile_matrix_metadata", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP tile_matrix_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -7;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE tile_matrix_metadata (t_table_name TEXT NOT NULL, zoom_level INTEGER NOT NULL, matrix_width INTEGER NOT NULL, matrix_height INTEGER NOT NULL)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE tile_matrix_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -8;
    }
    /* add in a test entry */
    ret = sqlite3_exec (db_handle, "INSERT INTO tile_matrix_metadata VALUES (\"test1_matrix_tiles\", 0, 1, 1)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT tile_matrix_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -9;
    }

    /* create a target table */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS test1_matrix_tiles", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP test1_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -10;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE test1_matrix_tiles (id INTEGER PRIMARY KEY AUTOINCREMENT, zoom_level INTEGER NOT NULL DEFAULT 0, tile_column INTEGER NOT NULL DEFAULT 0, tile_row INTEGER NOT NULL DEFAULT 0, tile_data BLOB NOT NULL DEFAULT (zeroblob(4)))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE test1_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -11;
    }
    
    /* test trigger setup */
    ret = sqlite3_exec (db_handle, "SELECT gpkgAddTileTriggers(\"test1_matrix_tiles\")", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "SELECT add_tile_trigger error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -12;
    }
    
    /* test the trigger works - several different cases here */
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles VALUES ( 3, 0, 0, 1, BlobFromFile('empty.png'))", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO 17A result: %i\n", ret);
      return -17;
    }
    if (strcmp(err_msg, "insert on table 'test1_matrix_tiles' violates constraint: tile_row must be < matrix_height specified for table and zoom level in tile_matrix_metadata") !=0) {
	fprintf(stderr, "unexpected INSERT INTO error message 3: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -18;
    }
    sqlite3_free (err_msg); 

    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles VALUES ( 4, 0, 1, 0, BlobFromFile('empty.png'))", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO 17 result: %i\n", ret);
      return -17;
    }
    if (strcmp(err_msg, "insert on table 'test1_matrix_tiles' violates constraint: tile_column must be < matrix_width specified for table and zoom level in tile_matrix_metadata") != 0) {
	fprintf(stderr, "unexpected INSERT INTO error message 4: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -18;
    }
    sqlite3_free (err_msg);

    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles VALUES ( 5, 1, 0, 0, BlobFromFile('empty.png'))", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO 19 result: %i\n", ret);
      return -19;
    }
    if (strcmp(err_msg, "insert on table 'test1_matrix_tiles' violates constraint: zoom_level not specified for table in tile_matrix_metadata") != 0) {
	fprintf(stderr, "unexpected INSERT INTO error message 5: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -20;
    }
    sqlite3_free (err_msg);
    
    /* Check a proper INSERT */
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles VALUES (6, 0, 0 ,0, BlobFromFile('empty.png'))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT error 6: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -21;
    }
    
    /* try some update checks */
    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles SET zoom_level = 1 WHERE id = 6", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE result: %i\n", ret);
      return -24;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles' violates constraint: zoom_level not specified for table in tile_matrix_metadata") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 1: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -25;
    }
    sqlite3_free (err_msg);

   
    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles SET zoom_level = -1 WHERE id = 6", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE 34 result: %i\n", ret);
      return -34;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles' violates constraint: zoom_level not specified for table in tile_matrix_metadata") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 5: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -35;
    }
    sqlite3_free (err_msg);

    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles SET tile_column = -1 WHERE id = 6", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE 36 result: %i\n", ret);
      return -36;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles' violates constraint: tile_column cannot be < 0") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 6: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -37;
    }
    sqlite3_free (err_msg);

    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles SET tile_row = -1 WHERE id = 6", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE 38 result: %i\n", ret);
      return -38;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles' violates constraint: tile_row cannot be < 0") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 7: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -39;
    }
    sqlite3_free (err_msg);

    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles SET tile_row = 1 WHERE id = 6", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE 40 result: %i\n", ret);
      return -40;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles' violates constraint: tile_row must be < matrix_height specified for table and zoom level in tile_matrix_metadata") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 8: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -41;
    }
    sqlite3_free (err_msg);

    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles SET tile_column = 1 WHERE id = 6", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE 42 result: %i\n", ret);
      return -42;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles' violates constraint: tile_column must be < matrix_width specified for table and zoom level in tile_matrix_metadata") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 9: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -43;
    }
    sqlite3_free (err_msg);
    
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles VALUES ( 4, 0, -1, 0, BlobFromFile('empty.png'))", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO 44 result: %i\n", ret);
      return -44;
    }
    if (strcmp(err_msg, "insert on table 'test1_matrix_tiles' violates constraint: tile_column cannot be < 0") != 0) {
	fprintf(stderr, "unexpected INSERT INTO error message 9: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -45;
    }
    sqlite3_free (err_msg);

    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles VALUES ( 4, 0, 0, -1, BlobFromFile('empty.png'))", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO 46 result: %i\n", ret);
      return -46;
    }
    if (strcmp(err_msg, "insert on table 'test1_matrix_tiles' violates constraint: tile_row cannot be < 0") != 0) {
	fprintf(stderr, "unexpected INSERT INTO error message 10: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -47;
    }
    sqlite3_free (err_msg);
    
    /* new tiles table */
    ret = sqlite3_exec (db_handle, "INSERT INTO raster_format_metadata VALUES (\"test2_matrix_tiles\", \"tile_data\", \"image/jpeg\", 24)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT raster_format_metadata jpeg error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -50;
    }
    ret = sqlite3_exec (db_handle, "INSERT INTO tile_matrix_metadata VALUES (\"test2_matrix_tiles\", 0, 1, 1)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT tile_matrix_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -51;
    }
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS test2_matrix_tiles", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP test2_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -52;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE test2_matrix_tiles (id INTEGER PRIMARY KEY AUTOINCREMENT, zoom_level INTEGER NOT NULL DEFAULT 0, tile_column INTEGER NOT NULL DEFAULT 0, tile_row INTEGER NOT NULL DEFAULT 0, tile_data BLOB NOT NULL DEFAULT (zeroblob(4)))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE test2_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -53;
    }
    
    /* test trigger setup */
    ret = sqlite3_exec (db_handle, "SELECT gpkgAddTileTriggers(\"test2_matrix_tiles\")", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "SELECT gpkgAddTileTriggers(test2_matrix_tiles) error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -54;
    }

    /* test trigger setup, bad table type */
    ret = sqlite3_exec (db_handle, "SELECT gpkgAddTileTriggers(0)", NULL, NULL, &err_msg);
    if (ret != SQLITE_ERROR) {
	fprintf(stderr, "unexpected SELECT gpkgAddTileTriggers(0) result: %i\n", ret);
	return -100;
    }
    if (strcmp(err_msg, "gpkgAddTileTriggers() error: argument 1 [table] is not of the String type") != 0) {
      fprintf (stderr, "unexpected SELECT gpkgAddTileTriggers(0) error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -101;
    }
    sqlite3_free(err_msg);
    
    ret = sqlite3_close (db_handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (db_handle));
	return -200;
    }
    
    spatialite_cleanup_ex(cache);
    
    return 0;
}
