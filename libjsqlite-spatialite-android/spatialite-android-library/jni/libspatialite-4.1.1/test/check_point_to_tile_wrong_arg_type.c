/*

 check_point_to_tile_wrong_arg_type.c - Test case for GeoPackage Extensions

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

#include <sqlite3.h>
#include <spatialite.h>

#include "test_helpers.h"

int main (int argc UNUSED, char *argv[] UNUSED)
{
    sqlite3 *db_handle = NULL;
    int ret;
    void *cache = spatialite_alloc_connection();
    char *err_msg = NULL;
    
    ret = sqlite3_open_v2 (":memory:", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    // For debugging / testing if required
    // ret = sqlite3_open_v2 ("check_point_to_tile_wrong_arg_type.sqlite", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    spatialite_init_ex(db_handle, cache, 0);
    if (ret != SQLITE_OK) {
      fprintf (stderr, "cannot open in-memory db: %s\n", sqlite3_errmsg (db_handle));
      sqlite3_close (db_handle);
      db_handle = NULL;
      return -1;
    }

    /* try gpkgPointToTile with the wrong argument types */
    ret = sqlite3_exec (db_handle, "SELECT gpkgPointToTile(1, 2, 3, 4, 5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_ERROR) {
	fprintf(stderr, "Expected error for bad arg 1, got %i\n", ret);
	sqlite3_free (err_msg);
	return -4;
    }
    if (strcmp(err_msg, "gpkgPointToTile() error: argument 1 [table] is not of the String type") != 0)
    {
	fprintf (stderr, "Unexpected error message for bad arg 1: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -5;
    }
    sqlite3_free(err_msg);
    
    ret = sqlite3_exec (db_handle, "SELECT gpkgPointToTile('test1_tiles', 2.3, 3, 4, 5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_ERROR) {
	fprintf(stderr, "Expected error for bad arg 2, got %i\n", ret);
	sqlite3_free (err_msg);
	return -6;
    }
    if (strcmp(err_msg, "gpkgPointToTile() error: argument 2 [srid] is not of the integer type") != 0)
    {
	fprintf (stderr, "Unexpected error message for bad arg 2: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -7;
    }
    sqlite3_free(err_msg);

    ret = sqlite3_exec (db_handle, "SELECT gpkgPointToTile('test1_tiles', 4326, 'hello', 4, 5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_ERROR) {
	fprintf(stderr, "Expected error for bad arg 3, got %i\n", ret);
	sqlite3_free (err_msg);
	return -8;
    }
    if (strcmp(err_msg, "gpkgPointToTile() error: argument 3 [x coordinate] is not of a numerical type") != 0)
    {
	fprintf (stderr, "Unexpected error message for bad arg 3: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -9;
    }
    sqlite3_free(err_msg);
    
    ret = sqlite3_exec (db_handle, "SELECT gpkgPointToTile('test1_tiles', 4326, 3.2, 'bye', 5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_ERROR) {
	fprintf(stderr, "Expected error for bad arg 4, got %i\n", ret);
	sqlite3_free (err_msg);
	return -10;
    }
    if (strcmp(err_msg, "gpkgPointToTile() error: argument 4 [y coordinate] is not of a numerical type") != 0)
    {
	fprintf (stderr, "Unexpected error message for bad arg 4: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -11;
    }
    sqlite3_free(err_msg);
    
    ret = sqlite3_exec (db_handle, "SELECT gpkgPointToTile('test1_tiles', 4326, 3.2, 5.2, 1.9)", NULL, NULL, &err_msg);
    if (ret != SQLITE_ERROR) {
	fprintf(stderr, "Expected error for bad arg 5, got %i\n", ret);
	sqlite3_free (err_msg);
	return -12;
    }
    if (strcmp(err_msg, "gpkgPointToTile() error: argument 5 [zoom level] is not of the integer type") != 0)
    {
	fprintf (stderr, "Unexpected error message for bad arg 5: %s\n", err_msg);
	sqlite3_free(err_msg);
	return -13;
    }
    sqlite3_free(err_msg);
    
    ret = sqlite3_close (db_handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (db_handle));
	return -100;
    }
    
    spatialite_cleanup_ex(cache);

    return 0;
}
