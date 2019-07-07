/*

 test_wms2.c -- RasterLite2 Test Case

 Author: Alessandro Furieri <a.furieri@lqt.it>

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

The Original Code is the RasterLite2 library

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"
#include "spatialite/gaiaaux.h"

#include "rasterlite2/rasterlite2.h"

static int
do_export_image (sqlite3 * sqlite, const char *name, const char *sql,
		 const char *suffix)
{
/* testing WMS GetMap */
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;

    path = sqlite3_mprintf ("./%s%s", name, suffix);

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
	    {
		FILE *out;
		blob = (unsigned char *) sqlite3_column_blob (stmt, 0);
		blob_size = sqlite3_column_bytes (stmt, 0);
		out = fopen (path, "wb");
		if (out != NULL)
		  {
		      /* writing the output image */
		      if ((int) fwrite (blob, 1, blob_size, out) == blob_size)
			  retcode = 1;
		      fclose (out);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to GetMap \"%s\"\n", path);
    unlink (path);
    sqlite3_free (path);
    return retcode;
}

int
main (int argc, char *argv[])
{

    int retcode = 0;
    int ret;
    sqlite3 *db_handle;
    const char *sql;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (getenv ("ENABLE_RL2_WEB_TESTS") == NULL)
      {
	  fprintf (stderr, "this testcase has been skipped !!!\n\n"
		   "you can enable all testcases requiring an Internet connection\n"
		   "by setting the environment variable \"ENABLE_RL2_WEB_TESTS=1\"\n");
	  return 0;
      }

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 ("./wms.sqlite", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ofc.10k13', BuildMBR(1658007.615625, 4691986.254005, 1658433.013709, 4692319.565916, 3003), 425, 333, '1.3.0', 'default', 'image/png', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ofc_col_png", sql, ".png"))
      {
	  retcode = -2;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ofc.10k13', BuildMBR(1658007.615625, 4691986.254005, 1658433.013709, 4692319.565916, 3003), 425, 333, '1.3.0', 'default', 'image/jpeg', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ofc_col_jpeg", sql, ".jpg"))
      {
	  retcode = -3;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ofc.10k13', BuildMBR(1658007.615625, 4691986.254005, 1658433.013709, 4692319.565916, 3003), 425, 333, '1.3.0', 'default', 'image/tiff', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ofc_col_tiff", sql, ".tif"))
      {
	  retcode = -4;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ofc.10k78', BuildMBR(1657828.090269, 4691315.626437, 1658543.474435, 4691876.150972, 3003), 425, 333, '1.3.0', 'default', 'image/png', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ofc_bn_png", sql, ".png"))
      {
	  retcode = -5;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ofc.10k78', BuildMBR(1657828.090269, 4691315.626437, 1658543.474435, 4691876.150972, 3003), 425, 333, '1.3.0', 'default', 'image/jpeg', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ofc_bn_jpeg", sql, ".jpg"))
      {
	  retcode = -6;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ofc.10k78', BuildMBR(1657828.090269, 4691315.626437, 1658543.474435, 4691876.150972, 3003), 425, 333, '1.3.0', 'default', 'image/tiff', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ofc_bn_tiff", sql, ".tif"))
      {
	  retcode = -7;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ctr.10k', BuildMBR(658023.924946, 4691314.113873, 658580.475640, 4691750.187711, 25832), 425, 333, '1.3.0', 'default', 'image/png', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ctrt_png", sql, ".png"))
      {
	  retcode = -8;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ctr.10k', BuildMBR(658023.924946, 4691314.113873, 658580.475640, 4691750.187711, 25832), 425, 333, '1.3.0', 'default', 'image/jpeg', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ctrt_jpeg", sql, ".jpg"))
      {
	  retcode = -9;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ctr.10k', BuildMBR(658023.924946, 4691314.113873, 658580.475640, 4691750.187711, 25832), 425, 333, '1.3.0', 'default', 'image/tiff', '#ffffff', 1);";
    if (!do_export_image (db_handle, "ctrt_tiff", sql, ".tif"))
      {
	  retcode = -10;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ucs.iducs.10k.2013.rt.full', BuildMBR(1657606.482573, 4691115.659483, 1658714.065587, 4691983.483350, 3003), 425, 333, '1.3.0', 'default', 'image/png', '#1e90ff', 0);";
    if (!do_export_image (db_handle, "ucs_png", sql, ".png"))
      {
	  retcode = -11;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ucs.iducs.10k.2013.rt.full', BuildMBR(1657606.482573, 4691115.659483, 1658714.065587, 4691983.483350, 3003), 425, 333, '1.3.0', 'default', 'image/jpeg', '#1e90ff', 0);";
    if (!do_export_image (db_handle, "ucs_jpeg", sql, ".jpg"))
      {
	  retcode = -12;
	  goto end;
      }

    sql =
	"SELECT RL2_GetMapImageFromWMS('main', 'rt_ucs.iducs.10k.2013.rt.full', BuildMBR(1657606.482573, 4691115.659483, 1658714.065587, 4691983.483350, 3003), 425, 333, '1.3.0', 'default', 'image/tiff', '#1e90ff', 0);";
    if (!do_export_image (db_handle, "ucs_tiff", sql, ".tif"))
      {
	  retcode = -13;
	  goto end;
      }

/* closing the DB */
  end:
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
    spatialite_shutdown ();
    if (old_SPATIALITE_SECURITY_ENV)
      {
#ifdef _WIN32
	  char *env = sqlite3_mprintf ("SPATIALITE_SECURITY=%s",
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
    return retcode;
}
