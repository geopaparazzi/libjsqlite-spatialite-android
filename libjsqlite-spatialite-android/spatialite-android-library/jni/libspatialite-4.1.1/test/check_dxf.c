/*

 check_dxf.c -- SpatiaLite Test Case

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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"
#include "spatialite/gg_dxf.h"

static int
check_22_auto()
{
/* testing 22.dxf - pass #1 autoDims */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (-2, GAIA_DXF_AUTO_2D_3D, NULL, NULL, GAIA_DXF_RING_NONE);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"22.dx\" auto)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./22.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"22.dxf\" byLayers auto\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" auto byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\"  auto append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" auto mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" auto append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_22_2d()
{
/* testing 22.dxf - pass #2 force 2D */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_FORCE_2D, "abdc_", NULL, GAIA_DXF_RING_NONE);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"22.dx\" 2D)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./22.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"22.dxf\" byLayers 2D\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" 2D byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\"  2D append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" 2D mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" 2D append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_22_3d()
{
/* testing 22.dxf - pass #3 force 3D */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_FORCE_3D, "abdc_", NULL, GAIA_DXF_RING_NONE);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"22.dx\" 3D)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./22.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"22.dxf\" byLayers 3D\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" 3D byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\"  3D append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" 3D mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" 3D append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_22_single()
{
/* testing 22.dxf - pass #4 single layer */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (-2, GAIA_DXF_AUTO_2D_3D, NULL, "VUOVIA", GAIA_DXF_RING_NONE);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"22.dx\" single)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./22.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"22.dxf\" byLayers single\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" single byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\"  single append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" single mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"22.dxf\" single append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_merano()
{
/* testing f06.dxf / l02.dxf / p02.dxf [merano samples] */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (25832, GAIA_DXF_AUTO_2D_3D, NULL, NULL, GAIA_DXF_RING_UNLINKED);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"f06.dx\")\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./f06.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"f06.dxf\" byLayer\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"f06.dxf\" byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"f06.dxf\"  append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"f06.dxf\" mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"f06.dxf\" append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);
  
    dxf = gaiaCreateDxfParser (25832, GAIA_DXF_AUTO_2D_3D, NULL, NULL, GAIA_DXF_RING_NONE);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"l02.dx\")\n");
	return -9;
    }

    ret = gaiaParseDxfFile (dxf, "./l02.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"l02.dxf\" byLayer\n");
	return -10;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"l02.dxf\" byLayer\n");
	return -11;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"l02.dxf\"  append byLayer\n");
	return -12;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"l02.dxf\" append mixed\n");
	return -13;
    }
    gaiaDestroyDxfParser(dxf);
  
    dxf = gaiaCreateDxfParser (25832, GAIA_DXF_AUTO_2D_3D, NULL, NULL, GAIA_DXF_RING_UNLINKED);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"f06.dx\")\n");
	return -14;
    }

    ret = gaiaParseDxfFile (dxf, "./p05.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"p05.dxf\" byLayer\n");
	return -15;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"p05.dxf\" byLayer\n");
	return -16;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"p05.dxf\"  append byLayer\n");
	return -17;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"p05.dxf\" mixed\n");
	return -18;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"p05.dxf\" append mixed\n");
	return -19;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -20;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_archaic()
{
/* testing archaic.dxf */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_AUTO_2D_3D, NULL, NULL, GAIA_DXF_RING_NONE);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"archaic.dx\" auto)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./archaic.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"archaic.dxf\" byLayers auto\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"archaic.dxf\" auto byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"archaic.dxf\"  auto append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"archaic.dxf\" auto mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"archaic.dxf\" auto append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_linked()
{
/* testing linked.dxf */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_AUTO_2D_3D, "lnk_", NULL, GAIA_DXF_RING_LINKED);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"linked.dx\" auto)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./linked.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"linked.dxf\" byLayers auto\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"linked.dxf\" auto byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"linked.dxf\"  auto append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"linked.dxf\" auto mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"linked.dxf\" auto append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_linked_legacy()
{
/* testing linked.dxf - legacy DB */
    int ret;
    sqlite3 *handle;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

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
	return -2;
    }

    spatialite_init_ex (handle, cache, 0);
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_AUTO_2D_3D, "lnk_", NULL, GAIA_DXF_RING_LINKED);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"linked.dx\" auto)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./linked.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"linked.dxf\" byLayers auto legacy\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"linked.dxf\" auto byLayer legacy\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"linked.dxf\"  auto append byLayer legacy\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"linked.dxf\" auto mixed legacy\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"linked.dxf\" auto append mixed legacy\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    ret = unlink("copy-legacy-3.0.1.sqlite");
    if (ret != 0)
    {
        fprintf(stderr, "cannot remove legacy v.3.0.1 database\n");
        return -10;
    }
    return 0;
}

static int
check_hatch()
{
/* testing hatch.dxf */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_AUTO_2D_3D, "lnk_", NULL, GAIA_DXF_RING_LINKED);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"hatch.dx\" auto)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./hatch.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"hatch.dxf\" byLayers auto\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"hatch.dxf\" auto byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"hatch.dxf\"  auto append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"hatch.dxf\" auto mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"hatch.dxf\" auto append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_hatch_legacy()
{
/* testing hatch.dxf - legacy DB */
    int ret;
    sqlite3 *handle;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

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
	return -2;
    }

    spatialite_init_ex (handle, cache, 0);
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_AUTO_2D_3D, "lnk_", NULL, GAIA_DXF_RING_LINKED);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"hatch.dx\" auto)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./hatch.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"hatch.dxf\" byLayers auto\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"hatch.dxf\" auto byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"hatch.dxf\"  auto append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 0);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"hatch.dxf\" auto mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"hatch.dxf\" auto append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    ret = unlink("copy-legacy-3.0.1.sqlite");
    if (ret != 0)
    {
        fprintf(stderr, "cannot remove legacy v.3.0.1 database\n");
        return -10;
    }
    return 0;
}

static int
check_symbol()
{
/* testing symbol.dxf */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

    ret = sqlite3_open_v2 (":memory:", &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "cannot open in-memory database: %s\n", sqlite3_errmsg (handle));
	sqlite3_close(handle);
	return -1;
    }

    spatialite_init_ex (handle, cache, 0);
    
    ret = sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
	fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	sqlite3_free(err_msg);
	sqlite3_close(handle);
	return -2;
    }
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_AUTO_2D_3D, "lnk_", NULL, GAIA_DXF_RING_LINKED);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"symbol.dx\" auto)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./symbol.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"symbol.dxf\" byLayers auto\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"symbol.dxf\" auto byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"symbol.dxf\"  auto append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"symbol.dxf\" auto mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"symbol.dxf\" auto append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    return 0;
}

static int
check_symbol_legacy()
{
/* testing symbol.dxf - legacy DB */
    int ret;
    sqlite3 *handle;
    gaiaDxfParserPtr dxf;
    void *cache = spatialite_alloc_connection();

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
	return -2;
    }

    spatialite_init_ex (handle, cache, 0);
  
    dxf = gaiaCreateDxfParser (3003, GAIA_DXF_AUTO_2D_3D, "lnk_", NULL, GAIA_DXF_RING_LINKED);
    if (dxf == NULL) {
	fprintf(stderr, "CREATE DXF PARSER: unexpected NULL \"symbol.dx\" auto)\n");
	return -3;
    }

    ret = gaiaParseDxfFile (dxf, "./symbol.dxf");
    if (ret == 0) {
	fprintf(stderr, "Unable to parse \"symbol.dxf\" byLayers auto\n");
	return -4;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"symbol.dxf\" auto byLayer\n");
	return -5;
    } 
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_BY_LAYER, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"symbol.dxf\"  auto append byLayer\n");
	return -6;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"symbol.dxf\" auto mixed\n");
	return -7;
    }
    
    ret = gaiaLoadFromDxfParser (handle, dxf, GAIA_DXF_IMPORT_MIXED, 1);
    if (ret == 0) {
	fprintf(stderr, "Unable to load \"symbol.dxf\" auto append mixed\n");
	return -8;
    }
    gaiaDestroyDxfParser(dxf);

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK) {
        fprintf (stderr, "sqlite3_close() error: %s\n", sqlite3_errmsg (handle));
	return -9;
    }
    
    spatialite_cleanup_ex (cache);
    ret = unlink("copy-legacy-3.0.1.sqlite");
    if (ret != 0)
    {
        fprintf(stderr, "cannot remove legacy v.3.0.1 database\n");
        return -10;
    }
    return 0;
}

int main (int argc, char *argv[])
{
    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (check_22_auto() != 0)
	return -1;

    if (check_22_2d() != 0)
	return -2;

    if (check_22_3d() != 0)
	return -3;

    if (check_22_single() != 0)
	return 4;

    if (check_merano() != 0)
	return -5;

    if (check_archaic() != 0)
	return -6;

    if (check_linked() != 0)
	return -7;

    if (check_linked_legacy() != 0)
	return -8;

    if (check_hatch() != 0)
	return -9;

    if (check_hatch_legacy() != 0)
	return -10;

    if (check_symbol() != 0)
	return -11;

    if (check_symbol_legacy() != 0)
	return -12;

    return 0;
}
