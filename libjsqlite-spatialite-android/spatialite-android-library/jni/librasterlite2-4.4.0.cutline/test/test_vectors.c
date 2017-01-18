/*

 test_vectors.c -- RasterLite-2 Test Case

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

The Original Code is the RasterLite2 library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2015
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

#include "rasterlite2/rasterlite2.h"

static int
test_default_style (sqlite3 * sqlite, int *retcode)
{
/* testing Default style */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT coverage_name, "
	"GetMapImageFromVector(coverage_name, BuildMbr(-180,-90,180,90), 1024, 512) "
	"FROM vector_coverages WHERE coverage_name like 'test_%'";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error #1: %s\n", sqlite3_errmsg (sqlite));
	  *retcode += -1;
	  return 0;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *name = (const char *) sqlite3_column_text (stmt, 0);
		unsigned char *blob =
		    (unsigned char *) sqlite3_column_blob (stmt, 1);
		int blob_sz = sqlite3_column_bytes (stmt, 1);
		char *path = sqlite3_mprintf ("./vector_%s_default.png", name);
		FILE *out = fopen (path, "wb");
		if (out != NULL)
		  {
		      fwrite (blob, 1, blob_sz, out);
		      fclose (out);
		      unlink (path);
		  }
		sqlite3_free (path);
	    }
	  else
	    {
		fprintf (stderr,
			 "sqlite3_step() error #1: %s\n",
			 sqlite3_errmsg (sqlite));
		*retcode += -2;
		sqlite3_finalize (stmt);
		return 0;
	    }
      }

    sqlite3_finalize (stmt);
    return 1;
}

static int
test_styles (sqlite3 * sqlite, int *retcode)
{
/* testing Styles */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT coverage_name, name, "
	"GetMapImageFromVector(coverage_name, BuildMbr(-180,-90,180,90), 1024, 512, name) "
	"FROM SE_vector_styled_layers_view WHERE coverage_name like 'test_%'";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error #2: %s\n", sqlite3_errmsg (sqlite));
	  *retcode += -1;
	  return 0;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *name = (const char *) sqlite3_column_text (stmt, 0);
		const char *style =
		    (const char *) sqlite3_column_text (stmt, 1);
		unsigned char *blob =
		    (unsigned char *) sqlite3_column_blob (stmt, 2);
		int blob_sz = sqlite3_column_bytes (stmt, 2);
		char *path =
		    sqlite3_mprintf ("./vector_%s_%s.png", name, style);
		FILE *out = fopen (path, "wb");
		if (out != NULL)
		  {
		      fwrite (blob, 1, blob_sz, out);
		      fclose (out);
		      unlink (path);
		  }
		sqlite3_free (path);
	    }
	  else
	    {
		fprintf (stderr,
			 "sqlite3_step() error #1: %s\n",
			 sqlite3_errmsg (sqlite));
		*retcode += -2;
		sqlite3_finalize (stmt);
		return 0;
	    }
      }

    sqlite3_finalize (stmt);
    return 1;
}

int
main (int argc, char *argv[])
{
    int result = 0;
    int ret;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 ("symbolizers.sqlite", &db_handle,
			   SQLITE_OPEN_READONLY, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);

/* tests */
    ret = -100;
    if (!test_default_style (db_handle, &ret))
	return ret;
    ret = -200;
    if (!test_styles (db_handle, &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
    spatialite_shutdown ();
    return result;
}
