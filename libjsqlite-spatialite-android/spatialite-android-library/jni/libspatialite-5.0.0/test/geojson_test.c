/*

 gepjspm.c -- SpatiaLite Test Case

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

static int
do_test (sqlite3 * handle)
{
    int ret;
    char *err_msg = NULL;

    ret = sqlite3_exec (handle, "PRAGMA foreign_keys=1", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "PRAGMA foreign_keys=1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
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
	  return -3;
      }
      
/* testing ImportGeoJSON basic options */
	ret = sqlite3_exec(handle, "SELECT ImportGeoJSON('./test.geojson', 'euro_cities')", NULL,  NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ImportGeoJSON() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -4;
      }
      
/* testing ImportGeoJSON full options */
	ret = sqlite3_exec(handle, "SELECT ImportGeoJSON('./test.geojson', 'euro_cities2', 1, 4326, 'UPPER')", NULL,  NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ImportGeoJSON() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -5;
      }
      
/* testing ExportGeoJSON2 basic options */
	ret = sqlite3_exec(handle, "SELECT ExportGeoJSON2('euro_cities', NULL, './text_export1.geojson')", NULL,  NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ExportGeoJSON2() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -6;
      }
      
/* testing ExportGeoJSON2 basic options */
	ret = sqlite3_exec(handle, "SELECT ExportGeoJSON2('euro_cities', NULL, './text_export2.geojson', 8, 1, 0, 'UPPER')", NULL,  NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ExportGeoJSON2() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -7;
      }
      
      return 0;
}

int
main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    void *cache = spatialite_alloc_connection ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;
#ifdef _WIN32
    char *env;
#endif /* not WIN32 */

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory database: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

    spatialite_init_ex (handle, cache, 0);

    ret = do_test (handle);
    if (ret != 0)
	return ret;

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_close() error: %s\n",
		   sqlite3_errmsg (handle));
	  return -61;
      }

    spatialite_cleanup_ex (cache);

/* testing again in legacy mode */
    spatialite_init (0);

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory database: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -62;
      }

    ret = do_test (handle);
    if (ret != 0)
	return ret;

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_close() error: %s\n",
		   sqlite3_errmsg (handle));
	  return -63;

	  spatialite_cleanup ();
      }

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    spatialite_shutdown ();
    if (old_SPATIALITE_SECURITY_ENV)
      {
#ifdef _WIN32
	  env =
	      sqlite3_mprintf ("SPATIALITE_SECURITY=%s",
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
    return 0;
}
