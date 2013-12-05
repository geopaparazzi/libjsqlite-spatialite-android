/*

 check_spatialindex.c -- SpatiaLite Test Case

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
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

int do_test(sqlite3 *handle, int legacy)
{
#ifndef OMIT_ICONV	/* only if ICONV is supported */
    int ret;
    char *err_msg = NULL;
    int row_count;
    char **results;
    int rows;
    int columns;
    char sql[1024];
	
    ret = load_shapefile (handle, "shp/foggia/local_councils", "Councils",
			  "CP1252", 23032, "geom", 1, 0, 1, 0, &row_count,
			  err_msg);
    if (!ret) {
        fprintf (stderr, "load_shapefile() error: %s\n", err_msg);
	sqlite3_close(handle);
	return -3;
    }
    if (row_count != 61) {
	fprintf (stderr, "unexpected number of rows loaded: %i\n", row_count);
	sqlite3_close(handle);
	return -4;
    }
    
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE MbrWithin(geom, BuildMbr(1040523, 4010000, 1140523, 4850000)) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -5;
    }
    if ((rows != 22) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return  -6;
    }
    if (strcmp(results[1], "Ascoli Satriano") != 0) {
	fprintf (stderr, "unexpected result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -7;
    }
    if (strcmp(results[22], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected result at 22: %s\n", results[22]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -8;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "SELECT CreateSpatialIndex('Councils', 'geom');",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CreateSpatialIndex error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -9;
    }

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT pkid FROM idx_Councils_geom;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in idx SELECT: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -10;
    }
    if ((rows != 61) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select pkid bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return  -11;
    }
    sqlite3_free_table (results);

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE ROWID IN (SELECT pkid FROM idx_Councils_geom WHERE xmin > 1040523 AND ymin > 4010000 AND xmax < 1140523 AND ymax < 4850000) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT Contains: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -12;
    }
    if ((rows != 22) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return  -13;
    }
    if (strcmp(results[1], "Ascoli Satriano") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -14;
    }
    if (strcmp(results[22], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected mbr result at 22: %s\n", results[22]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -15;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "INSERT INTO Councils (lc_name, geom) VALUES ('Quairading', GeomFromText('MULTIPOLYGON(((997226.750031 4627372.000018, 997301.750031 4627332.000018, 997402.500031 4627344.000018, 997541.500031 4627326.500018,997226.750031 4627372.000018)))', 23032));",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "INSERT error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -16;
    }

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE ROWID IN (SELECT pkid FROM idx_Councils_geom WHERE xmin > 1040523 AND ymin > 4010000 AND xmax < 1140523 AND ymax < 4850000) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT2: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -17;
    }
    if ((rows != 22) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -18;
    }
    if (strcmp(results[1], "Ascoli Satriano") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -19;
    }
    if (strcmp(results[22], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected mbr result at 22: %s\n", results[22]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -20;
    }
    sqlite3_free_table (results);
  
    ret = sqlite3_exec (handle, "UPDATE Councils SET geom = GeomFromText('MULTIPOLYGON(((987226.750031 4627372.000018, 997301.750031 4627331.000018, 997402.500032 4627344.000018, 997541.500031 4627326.500018, 987226.750031 4627372.000018)))', 23032) WHERE lc_name = \"Quairading\";",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "UPDATE error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -34;
    }

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE ROWID IN (SELECT pkid FROM idx_Councils_geom WHERE xmin > 1040523 AND ymin > 4010000 AND xmax < 1140523 AND ymax < 4850000) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT3: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -35;
    }
    if ((rows != 22) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -36;
    }
    if (strcmp(results[1], "Ascoli Satriano") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -37;
    }
    if (strcmp(results[22], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected mbr result at 22: %s\n", results[22]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -38;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "DELETE FROM Councils WHERE lc_name = \"Ascoli Satriano\";",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DELETE error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -39;
    }

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT lc_name FROM Councils WHERE ROWID IN (SELECT pkid FROM idx_Councils_geom WHERE xmin > 1040523 AND ymin > 4010000 AND xmax < 1140523 AND ymax < 4850000) ORDER BY lc_name;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT3: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -40;
    }
    if ((rows != 21) || (columns != 1)) {
	fprintf (stderr, "Unexpected error: select lc_name bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -41;
    }
    if (strcmp(results[1], "Cagnano Varano") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[1]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -42;
    }
    if (strcmp(results[21], "Zapponeta") != 0) {
	fprintf (stderr, "unexpected mbr result at 21: %s\n", results[21]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -43;
    }
    sqlite3_free_table (results);

    rows = 0;
    columns = 0;
    ret = sqlite3_get_table (handle, "SELECT rowid, nodeno FROM idx_Councils_geom_rowid ORDER BY rowid;",
			     &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Error in Mbr SELECT: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -44;
    }
    if ((rows != 61) || (columns != 2)) {
	fprintf (stderr, "Unexpected error: select rowid bad idx result: %i/%i.\n", rows, columns);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return  -45;
    }
    if (strcmp(results[2], "1") != 0) {
	fprintf (stderr, "unexpected mbr result at 1: %s\n", results[2]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -46;
    }
    if (strcmp(results[12], "6") != 0) {
	fprintf (stderr, "unexpected mbr result at 6: %s\n", results[12]);
	sqlite3_free_table (results);
	sqlite3_close(handle);
	return -47;
    }
    sqlite3_free_table (results);

    ret = sqlite3_exec (handle, "SELECT CheckSpatialIndex('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CheckSpatialIndex (1) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -48;
    }

    ret = sqlite3_exec (handle, "SELECT CheckSpatialIndex();", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CheckSpatialIndex (2) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -49;
    }

    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverSpatialIndex (1) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -50;
    }

    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex();", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverSpatialIndex (2) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -51;
    }

/*
/ going to create a broken/corrupted SpatialIndex 
/
/ - we'll create a new table (with no Primary Key)
/ - then we'll delete some rows
/ - and finally we'll perform a Vacuum
/ - all this notoriously causes R*Tree corruption
*/
    ret = sqlite3_exec (handle, "CREATE TABLE bad_councils AS SELECT lc_name, geom FROM Councils", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CREATE TABLE bad_councils error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -52;
    }

    ret = sqlite3_exec (handle, "SELECT RecoverGeometryColumn(1, 'geom', 23032, 'MULTIPOLYGON', 'XY')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -54;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverGeometryColumn('bad_councils', 1, 23032, 'MULTIPOLYGON', 'XY')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -55;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverGeometryColumn('bad_councils', 'geom', 23032.5, 'MULTIPOLYGON', 'XY')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -56;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverGeometryColumn('bad_councils', 'geom', 23032, 1, 'XY')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -57;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverGeometryColumn('bad_councils', 'geom', 23032, 'MULTIPOLYGON', 1.5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -58;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverGeometryColumn('bad_councils', 'geom', 23032, 'DUMMY', 'XY')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -59;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverGeometryColumn('bad_councils', 'geom', 23032, 'MULTIPOLYGON', 'DUMMY')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -60;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverGeometryColumn('bad_councils', 'geom', 23032, 'MULTIPOLYGON', 2)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverGeometryColumn(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -61;
    }

    ret = sqlite3_exec (handle, "SELECT CreateSpatialIndex('bad_councils', 'geom')", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CreateSpatialIndex(bad_councils) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -62;
    }

    ret = sqlite3_exec (handle, "DELETE FROM bad_councils WHERE lc_name LIKE 'C%'", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DELETE FROM bad_councils error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -63;
    }

    ret = sqlite3_exec (handle, "VACUUM", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "VACUUM error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -64;
    }
    ret = sqlite3_exec (handle, "SELECT CheckSpatialIndex('bad_councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "CheckSpatialIndex (3) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -65;
    }

    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex('bad_councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "RecoverSpatialIndex (3) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -66;
    }

    ret = sqlite3_exec (handle, "SELECT UpdateLayerStatistics('bad_councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "UpdateLayerStatistics (1) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -67;
    }

    ret = sqlite3_exec (handle, "SELECT UpdateLayerStatistics();", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "UpdateLayerStatistics (2) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -68;
    }
/* END broken SpatialIndex check/recover */

    ret = sqlite3_exec (handle, "SELECT Extent(geom) FROM Councils;", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Extent (1) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -69;
    }

    strcpy (sql, "SELECT Count(*) FROM Councils WHERE ROWID IN (");
    strcat (sql, "SELECT ROWID FROM SpatialIndex WHERE ");
    strcat (sql, "f_table_name = 'Councils' AND search_frame = ");
    strcat (sql, "BuildMbr(10, 10, 20, 20));");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "VirtualSpatialIndex (1) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -70;
    }

    strcpy (sql, "SELECT Count(*) FROM Councils WHERE ROWID IN (");
    strcat (sql, "SELECT ROWID FROM SpatialIndex WHERE ");
    strcat (sql, "f_table_name = 'Councils' AND f_geometry_column = ");
    strcat (sql, "'geom' AND search_frame = BuildCircleMbr(1019000, ");
    strcat (sql, "4592000, 10000));");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "VirtualSpatialIndex (2) error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -71;
    }

    ret = sqlite3_exec (handle, "SELECT RebuildGeometryTriggers('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Rebuild triggers error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -72;
    }

    ret = sqlite3_exec (handle, "SELECT DisableSpatialIndex('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Disable index error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -73;
    }

    ret = sqlite3_exec (handle, "SELECT RebuildGeometryTriggers('Councils', 'geom');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "Disable index error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -74;
    }

    ret = sqlite3_exec (handle, "DROP TABLE Councils;", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DROP TABLE Councils error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -75;
    }

    ret = sqlite3_exec (handle, "SELECT DiscardGeometryColumn(1, 'a');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "invalid DiscardGeometryColumn error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -76;
    }
    ret = sqlite3_exec (handle, "SELECT DiscardGeometryColumn('a', 1);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "invalid DiscardGeometryColumn error: %s\n", err_msg);
	sqlite3_free (err_msg);
	return -77;
    }

    ret = sqlite3_exec (handle, "SELECT CheckSpatialIndex(1, 'a');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid CheckSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -78;
    }
    ret = sqlite3_exec (handle, "SELECT CheckSpatialIndex('a', 2);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid CheckSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -79;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex(1, 'a');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid RecoverSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -80;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex('a', 2);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid RecoverSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -81;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex('a');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid RecoverSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -82;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex(1);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid RecoverSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -83;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex('a', 'b', 'c');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid RecoverSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -84;
    }
    ret = sqlite3_exec (handle, "SELECT RecoverSpatialIndex('a', 'b', 1);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid RecoverSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -85;
    }
    ret = sqlite3_exec (handle, "SELECT CreateSpatialIndex(1, 'a');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid CreateSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -86;
    }
    ret = sqlite3_exec (handle, "SELECT CreateSpatialIndex('a', 2);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid CreateSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -87;
    }
    ret = sqlite3_exec (handle, "SELECT DisableSpatialIndex(1, 'a');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid DisableSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -88;
    }
    ret = sqlite3_exec (handle, "SELECT DisableSpatialIndex('a', 2);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid DisableSpatialIndex error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -89;
    }
    ret = sqlite3_exec (handle, "SELECT RebuildGeometryTriggers(1, 'a');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid RebuildGeometryTriggers error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -90;
    }
    ret = sqlite3_exec (handle, "SELECT RebuildGeometryTriggers('a', 2);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid RebuildGeometryTriggers error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -91;
    }
    ret = sqlite3_exec (handle, "SELECT UpdateLayerStatistics('a', 2);", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid UpdateLayerStatistics error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -92;
    }
    ret = sqlite3_exec (handle, "SELECT UpdateLayerStatistics(2, 'a');", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "invalid UpdateLayerStatistics error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return -93;
    }

/* final DB cleanup */
    ret = sqlite3_exec (handle, "DROP TABLE bad_councils", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DROP TABLE bad_councils error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -94;
    }
    ret = sqlite3_exec (handle, "DROP TABLE idx_bad_councils_geom", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DROP TABLE idx_bad_councils_geom error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -95;
    }
    ret = sqlite3_exec (handle, "DROP TABLE idx_Councils_geom", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DROP TABLE idx_Councils_geom error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -96;
    }
    ret = sqlite3_exec (handle, "DELETE FROM geometry_columns", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DELETE FROM geometry_columns error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -97;
    }
    if (legacy)
    {
    /* only required for legacy style metadata */
        ret = sqlite3_exec (handle, "DELETE FROM layer_statistics", NULL, NULL, &err_msg);
        if (ret != SQLITE_OK) {
	    fprintf (stderr, "DELETE FROM layer_statistics error: %s\n", err_msg);
	    sqlite3_free(err_msg);
	    sqlite3_close(handle);
	    return -98;
	}
    }
    ret = sqlite3_exec (handle, "DELETE FROM spatialite_history WHERE geometry_column IS NOT NULL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "DELETE FROM spatialite_history error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -99;
    }
    ret = sqlite3_exec (handle, "VACUUM", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "VACUUM error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -100;
    }
	
#endif	/* end ICONV conditional */

/* ok, succesfull termination */
	return 0;
}

int main (int argc, char *argv[])
{
#ifndef OMIT_ICONV	/* only if ICONV is supported */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

/* testing current style metadata layout >= v.4.0.0 */
    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata()", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
    
    ret = do_test(handle, 0);
    if (ret != 0) {
	fprintf(stderr, "error while testing current style metadata layout\n");
	return ret;
    }

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -101;
    }
    
    spatialite_cleanup_ex (cache);

/* testing legacy style metadata layout <= v.3.1.0 */
    cache = spatialite_alloc_connection();
    ret = system("cp test-legacy-3.0.1.sqlite copy-legacy-3.0.1.sqlite");
    if (ret != 0)
    {
        fprintf(stderr, "cannot copy legacy v.3.0.1 database\n");
        return -1;
    }
    ret = sqlite3_open_v2 ("copy-legacy-3.0.1.sqlite", &handle, SQLITE_OPEN_READWRITE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open legacy v.3.0.1 database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
	
    ret = do_test(handle, 1);
    if (ret != 0) {
	fprintf(stderr, "error while testing legacy style metadata layout\n");
	return ret;
    }

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -101;
    }
    
    spatialite_cleanup_ex (cache);
    ret = unlink("copy-legacy-3.0.1.sqlite");
    if (ret != 0)
    {
        fprintf(stderr, "cannot remove legacy v.3.0.1 database\n");
        return -1;
    }
	
#endif	/* end ICONV conditional */

    return 0;
}
