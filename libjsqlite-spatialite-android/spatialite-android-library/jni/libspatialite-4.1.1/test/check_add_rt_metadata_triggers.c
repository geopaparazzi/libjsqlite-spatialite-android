/*

 check_add_rt_metadata_triggers.c - Test case for GeoPackage Extensions

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
    // ret = sqlite3_open_v2 ("check_add_rt_metadata_triggers.sqlite", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
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

    /* create a minimal raster_columns table */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS raster_columns", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP raster_columns error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -4;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE raster_columns (r_table_name TEXT NOT NULL, r_raster_column TEXT NOT NULL, srid INTEGER NOT NULL DEFAULT 0, CONSTRAINT pk_rc PRIMARY KEY (r_table_name, r_raster_column) ON CONFLICT ROLLBACK, CONSTRAINT fk_rc_r_srid FOREIGN KEY (srid) REFERENCES spatial_ref_sys(srid))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE raster_columns error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -5;
    }

    /* add in a test entry */
    ret = sqlite3_exec (db_handle, "INSERT INTO raster_columns VALUES (\"test1_matrix_tiles\", \"tile_data\", 4326)",  NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT raster_columns error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -6;
    }

    /* create a target data table */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS test1_matrix_tiles", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP test1_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -7;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE test1_matrix_tiles (id INTEGER PRIMARY KEY AUTOINCREMENT, zoom_level INTEGER NOT NULL DEFAULT 0, tile_column INTEGER NOT NULL DEFAULT 0, tile_row INTEGER NOT NULL DEFAULT 0, tile_data BLOB NOT NULL DEFAULT (zeroblob(4)))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE test1_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -8;
    }
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles VALUES (1, 0, 0 ,0, BlobFromFile('empty.png'))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT error 88: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -88;
    }
    
    /* create a target metadata table */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS test1_matrix_tiles_rt_metadata", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP test1_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -9;
    }

    ret = sqlite3_exec (db_handle, "CREATE TABLE test1_matrix_tiles_rt_metadata (row_id_value INTEGER NOT NULL, r_raster_column TEXT NOT NULL DEFAULT 'tile_data', georectification INTEGER NOT NULL DEFAULT 0, min_x DOUBLE NOT NULL DEFAULT -180.0, min_y DOUBLE NOT NULL DEFAULT -90.0, max_x DOUBLE NOT NULL DEFAULT 180.0, max_y DOUBLE NOT NULL DEFAULT 90.0, compr_qual_factor INTEGER NOT NULL DEFAULT 100, CONSTRAINT pk_smt_rm PRIMARY KEY (row_id_value, r_raster_column) ON CONFLICT ROLLBACK)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE test1_matrix_tiles_rt_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -10;
    }
    
    /* test trigger setup */
    ret = sqlite3_exec (db_handle, "SELECT gpkgAddRtMetadataTriggers(\"test1_matrix_tiles\")", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "SELECT gpkgAddRtMetadataTriggers(test1_matrix_tiles)error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -11;
    }
    
    /* create a second target data table */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS test2_matrix_tiles", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP test2_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -12;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE test2_matrix_tiles (id INTEGER PRIMARY KEY AUTOINCREMENT, zoom_level INTEGER NOT NULL DEFAULT 0, tile_column INTEGER NOT NULL DEFAULT 0, tile_row INTEGER NOT NULL DEFAULT 0, tile_data BLOB NOT NULL DEFAULT (zeroblob(4)))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE test2_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -13;
    }
    
    /* create a target metadata table */
    ret = sqlite3_exec (db_handle, "DROP TABLE IF EXISTS test2_matrix_tiles_rt_metadata", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "DROP test2_matrix_tiles error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -14;
    }
    ret = sqlite3_exec (db_handle, "CREATE TABLE test2_matrix_tiles_rt_metadata (row_id_value INTEGER NOT NULL, r_raster_column TEXT NOT NULL DEFAULT 'tile_data', georectification INTEGER NOT NULL DEFAULT 0, min_x DOUBLE NOT NULL DEFAULT -180.0, min_y DOUBLE NOT NULL DEFAULT -90.0, max_x DOUBLE NOT NULL DEFAULT 180.0, max_y DOUBLE NOT NULL DEFAULT 90.0, compr_qual_factor INTEGER NOT NULL DEFAULT 100, CONSTRAINT pk_smt_rm PRIMARY KEY (row_id_value, r_raster_column) ON CONFLICT ROLLBACK)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "CREATE test2_matrix_tiles_rt_metadata error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -15;
    }
    
    ret = sqlite3_exec (db_handle, "SELECT gpkgAddRtMetadataTriggers(\"test2_matrix_tiles\")", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "SELECT gpkgAddRtMetadataTriggers(test2_matrix_tiles)error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -16;
    }
    
    /* Testing trigger:
     *  "CREATE TRIGGER '%s_rt_metadata_r_raster_column_insert'\n"
	"BEFORE INSERT ON '%s_rt_metadata'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'insert on table ''%s_rt_metadata'' violates constraint: r_raster_column must be specified for table %s in table raster_columns')\n"
	"WHERE (NOT (NEW.r_raster_column IN (SELECT DISTINCT r_raster_column FROM raster_columns WHERE r_table_name = '%s')));\n"
	"END\n",
     */
    ret = sqlite3_exec (db_handle, "INSERT INTO test2_matrix_tiles_rt_metadata (row_id_value) VALUES ( 1 )", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO  test2_matrix_tiles_rt_metadata (row_id_value) VALUES ( 1 ) result: %i (%s)\n", ret, err_msg);
      return -17;
    }
    if (strcmp(err_msg, "insert on table test2_matrix_tiles_rt_metadata violates constraint: row_id_value must exist in test2_matrix_tiles table") != 0) {
	fprintf(stderr, "unexpected INSERT INTO error message 1: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -18;
    }
    sqlite3_free (err_msg);    

    /* Testing:
	"CREATE TRIGGER '%q_rt_metadata_georectification_insert'\n"
	"BEFORE INSERT ON '%q_rt_metadata'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'insert on table ''%q_rt_metadata'' violates constraint: georectification must be 0, 1 or 2')\n"
	"WHERE (NOT (NEW.georectification IN (0, 1, 2)));\n"
	"END",
	*/
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles_rt_metadata (row_id_value, georectification) VALUES ( 1, 3 )", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO (georectification insert) result: %i (%s)\n", ret, err_msg);
      return -19;
    }
    if (strcmp(err_msg, "insert on table 'test1_matrix_tiles_rt_metadata' violates constraint: georectification must be 0, 1 or 2") != 0) {
	fprintf(stderr, "unexpected INSERT INTO error message 2: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -20;
    }
    sqlite3_free (err_msg);
    
    /* Testing:
	"CREATE TRIGGER '%s_rt_metadata_compr_qual_factor_insert'\n"
	"BEFORE INSERT ON '%s_rt_metadata'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'insert on table ''%s_rt_metadata'' violates constraint: compr_qual_factor < 1, must be between 1 and 100')\n"
	"WHERE NEW.compr_qual_factor < 1;\n"
	"SELECT RAISE(ROLLBACK, 'insert on table ''%s_rt_metadata'' violates constraint: compr_qual_factor > 100, must be between 1 and 100')\n"
	"WHERE NEW.compr_qual_factor > 100;\n"
	"END\n",
	*/
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles_rt_metadata (row_id_value, compr_qual_factor) VALUES ( 1, 0 )", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO result: %i\n", ret);
      return -25;
    }
    if (strcmp(err_msg, "insert on table 'test1_matrix_tiles_rt_metadata' violates constraint: compr_qual_factor < 1, must be between 1 and 100") != 0) {
	fprintf(stderr, "unexpected INSERT INTO error message 5: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -26;
    }
    sqlite3_free (err_msg);
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles_rt_metadata (row_id_value, compr_qual_factor) VALUES ( 1, 101 )", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected INSERT INTO result: %i\n", ret);
      return -27;
    }
    if (strcmp(err_msg, "insert on table 'test1_matrix_tiles_rt_metadata' violates constraint: compr_qual_factor > 100, must be between 1 and 100") != 0) {
	fprintf(stderr, "unexpected INSERT INTO error message 6: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -28;
    }
    sqlite3_free (err_msg);
    
    /* Add a legitimate entry */
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles VALUES (2, 0, 0 ,0, BlobFromFile('test.webp'))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "INSERT error 32: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -32;
    }
    ret = sqlite3_exec (db_handle, "INSERT INTO test1_matrix_tiles_rt_metadata (row_id_value) VALUES (2)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "unexpected INSERT INTO 33 error: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -33;
    }
    
    /* Testing:
	"CREATE TRIGGER '%q_rt_metadata_r_raster_column_update'\n"
	"BEFORE UPDATE OF r_raster_column ON '%q_rt_metadata'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'update on table ''%q_rt_metadata'' violates constraint: r_raster_column must be specified for table %s in table raster_columns')\n"
	"WHERE (NOT (NEW.r_raster_column IN (SELECT DISTINCT r_raster_column FROM raster_columns WHERE r_table_name = '%s')));\n"
	"END",
      */
    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles_rt_metadata SET r_raster_column=\"none\" WHERE row_id_value = 2", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE r_raster_column result: %i (%s)\n", ret, err_msg);
      return -34;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles_rt_metadata' violates constraint: r_raster_column must be specified for table test1_matrix_tiles in table raster_columns") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 1: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -35;
    }
    sqlite3_free (err_msg);

     /* Testing:

	"CREATE TRIGGER '%q_rt_metadata_georectification_update'\n"
	"BEFORE UPDATE OF georectification ON '%q_rt_metadata'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT, 'insert on table ''%q_rt_metadata'' violates constraint: georectification must be 0, 1 or 2')\n"
	"WHERE (NOT (NEW.georectification IN (0, 1, 2)));\n"
	"END",
	*/
    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles_rt_metadata SET georectification=3 WHERE row_id_value = 2", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE georectification result: %i\n", ret);
      return -36;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles_rt_metadata' violates constraint: georectification must be 0, 1 or 2") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 1: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -37;
    }
    sqlite3_free (err_msg);
    
    /* Testing:
	"CREATE TRIGGER '%s_rt_metadata_compr_qual_factor_update'\n"
	"BEFORE UPDATE OF compr_qual_factor ON '%s_rt_metadata'\n"
	"FOR EACH ROW BEGIN\n"
	"SELECT RAISE(ROLLBACK, 'update on table ''%s_rt_metadata'' violates constraint: compr_qual_factor < 1, must be between 1 and 100')\n"
	"WHERE NEW.compr_qual_factor < 1;\n"
	"SELECT RAISE(ROLLBACK, 'update on table ''%s_rt_metadata'' violates constraint: compr_qual_factor > 100, must be between 1 and 100')\n"
	"WHERE NEW.compr_qual_factor > 100;\n"
	"END\n",
    */
    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles_rt_metadata SET compr_qual_factor=0 WHERE row_id_value = 2", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE compr_qual_factor0 result: %i\n", ret);
      return -48;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles_rt_metadata' violates constraint: compr_qual_factor < 1, must be between 1 and 100") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 5: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -49;
    }
    sqlite3_free (err_msg);
    ret = sqlite3_exec (db_handle, "UPDATE test1_matrix_tiles_rt_metadata SET compr_qual_factor=101 WHERE row_id_value = 2", NULL, NULL, &err_msg);
    if (ret != SQLITE_CONSTRAINT) {
      fprintf (stderr, "unexpected UPDATE compr_qual_factor101 result: %i\n", ret);
      return -50;
    }
    if (strcmp(err_msg, "update on table 'test1_matrix_tiles_rt_metadata' violates constraint: compr_qual_factor > 100, must be between 1 and 100") != 0) {
	fprintf(stderr, "unexpected UPDATE error message 6: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -51;
    }
    sqlite3_free (err_msg);
    
    ret = sqlite3_close (db_handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (db_handle));
	return -200;
    }
    
    spatialite_cleanup_ex(cache);
    
    return 0;
}
