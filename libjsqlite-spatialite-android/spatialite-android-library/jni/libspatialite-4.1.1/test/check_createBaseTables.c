/*

 check_createBaseTables.c - Test case for GeoPackage Extensions

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "test_helpers.h"

int main (int argc UNUSED, char *argv[] UNUSED)
{
    sqlite3 *db_handle = NULL;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    // For debugging / testing if required
    // ret = sqlite3_open_v2 ("check_createBaseTables.sqlite", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    spatialite_init_ex(db_handle, cache, 0);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "cannot open in-memory db: %s\n", sqlite3_errmsg (db_handle));
      sqlite3_close (db_handle);
      db_handle = NULL;
      return -1;
    }
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
	return -100;
    }

    /* check raster_columns table is OK */
    ret = sqlite3_exec (db_handle, "INSERT INTO raster_columns VALUES (\"sample_matrix_tiles\", \"tile_data\", 100, 0, 4326)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "Unexpected INSERT INTO raster_columns result: %i, (%s)\n", ret, err_msg);
	sqlite3_free (err_msg);
	return -101;
    }
    
    /* check tile_table_metadata table is OK */
    ret = sqlite3_exec (db_handle, "INSERT INTO tile_table_metadata VALUES (\"sample_matrix_tiles\", 1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "Unexpected INSERT INTO tile_table_metadata result: %i, (%s)\n", ret, err_msg);
	sqlite3_free (err_msg);
	return -102;
    }
    
    /* check tile_matrix_metadata table is OK */
    ret = sqlite3_exec (db_handle, "INSERT INTO tile_matrix_metadata VALUES (\"sample_matrix_tiles\", 0, 1, 1, 512, 512, 2.0, 2.0)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "Unexpected INSERT INTO tile_matrix_metadata result: %i, (%s)\n", ret, err_msg);
	sqlite3_free (err_msg);
	return -103;
    }
    
    /* check xml_metadata table is OK */
    ret = sqlite3_get_table (db_handle, "SELECT id, md_scope, metadata_standard_URI, metadata FROM xml_metadata", &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "Error1: %s\n", err_msg);
      sqlite3_free (err_msg);
      return -104;
    }
    if ((rows != 1) || (columns != 4))
    {
	sqlite3_free_table(results);
	fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows, columns);
	return -105;
    }
    if (strcmp(results[1 * columns + 0], "0") != 0)
    {
	fprintf (stderr, "Unexpected id result (got %s, expected 0)", results[1 * columns + 0]);
	sqlite3_free_table(results);
	return -106;
    }
    if (strcmp(results[1 * columns + 1], "undefined") != 0)
    {
	fprintf (stderr, "Unexpected md_scope result (got %s, expected undefined)", results[1 * columns + 1]);
	sqlite3_free_table(results);
	return -107;
    }
    if (strcmp(results[1 * columns + 2], "http://schemas.opengis.net/iso/19139/") != 0)
    {
	fprintf (stderr, "Unexpected metadata_standard_URI result (got %s, expected http://schemas.opengis.net/iso/19139/)", results[1 * columns + 2]);
	sqlite3_free_table(results);
	return -108;
    }
    sqlite3_free_table(results);
   
    /* check metadata_reference table is OK */
    ret = sqlite3_exec (db_handle, "INSERT INTO metadata_reference VALUES ('table','sample_matrix_tiles','undefined', 0, '2012-08-17T14:49:32.932Z', 98, 99)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "Unexpected INSERT INTO metadata_reference result: %i, (%s)\n", ret, err_msg);
	sqlite3_free (err_msg);
	return -109;
    }
    
    /* TODO: check each trigger works as expected */

    /* check creation when the tables already exist */
    ret = sqlite3_exec (db_handle, "SELECT gpkgCreateBaseTables()", NULL, NULL, &err_msg);
    if (ret != SQLITE_ERROR) {
	fprintf(stderr, "Unexpected duplicate gpkgCreateBaseTables() result: %i, (%s)\n", ret, err_msg);
	return -110;
    }
    if (strcmp("table geopackage_contents already exists", err_msg) != 0)
    {
	fprintf(stderr, "Unexpected duplicate gpkgCreateBaseTables() error message: %s\n", err_msg);
	return -111;
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
