/*

 check_create.c -- SpatiaLite Test Case

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

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2011
the Initial Developer. All Rights Reserved.

Contributor(s):
Brad Hards <bradh@frogmouth.net>

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

int main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    sqlite3_int64 log_pk;

    spatialite_init (0);
    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory db: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
    ret = sqlite3_exec (handle, "SELECT HasProj()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "HasProj() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -3;
    }
    ret = sqlite3_exec (handle, "SELECT HasGeos()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "HasGeos() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -4;
    }
    ret = sqlite3_exec (handle, "SELECT HasGeosAdvanced()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "HasGeosAdvanced() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -5;
    }
    ret = sqlite3_exec (handle, "SELECT HasIconv()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "HasIconv() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -6;
    }
    ret = sqlite3_exec (handle, "SELECT HasMathSql()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "HasMathSql() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -7;
    }
    ret = sqlite3_exec (handle, "SELECT HasGeoCallbacks()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "HasGeoCallbacks() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -8;
    }
    ret = sqlite3_exec (handle, "SELECT HasFreeXL()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "HasFreeXL() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -9;
    }
    ret = sqlite3_exec (handle, "SELECT HasEpsg()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "HasEpsg() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -10;
    }

    gaiaInsertIntoSqlLog (handle, "test", "sql_statement_ok", &log_pk);
    gaiaUpdateSqlLog (handle, log_pk, 1, NULL);
    gaiaInsertIntoSqlLog (handle, "test", "sql_statement_no", &log_pk);
    gaiaUpdateSqlLog (handle, log_pk, 0, "some error message");

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -11;
    }
    
    spatialite_cleanup();
    
    return 0;
}
