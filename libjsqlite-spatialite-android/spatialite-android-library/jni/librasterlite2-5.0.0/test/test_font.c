/*

 test_font.c -- RasterLite-2 Test Case

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
execute_check (sqlite3 * sqlite, const char *sql)
{
/* executing an SQL statement returning True/False */
    sqlite3_stmt *stmt;
    int ret;
    int retcode = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return SQLITE_ERROR;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    if (retcode == 1)
	return SQLITE_OK;
    return SQLITE_ERROR;
}

static int
execute_test_string (sqlite3 * sqlite, const char *sql, const char *expected)
{
/* executing an SQL statement returning a TEXT resultset */
    sqlite3_stmt *stmt;
    int ret;
    int err = 0;
    int match = 0;
    int count = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return SQLITE_ERROR;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		count++;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      const char *value =
			  (const char *) sqlite3_column_text (stmt, 0);
		      if (strcmp (value, expected) == 0)
			  match++;
		  }
	    }
	  else
	    {
		err = 1;
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1 && match == 1 && err == 0)
	return SQLITE_OK;
    return SQLITE_ERROR;
}

static int
execute_test_boolean (sqlite3 * sqlite, const char *sql, int expected)
{
/* executing an SQL statement returning a BOOLEAN resultset */
    sqlite3_stmt *stmt;
    int ret;
    int err = 0;
    int match = 0;
    int count = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return SQLITE_ERROR;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		count++;
		if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
		  {
		      int value = sqlite3_column_int (stmt, 0);
		      if (value == expected)
			  match++;
		  }
	    }
	  else
	    {
		err = 1;
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1 && match == 1 && err == 0)
	return SQLITE_OK;
    return SQLITE_ERROR;
}

static int
test_font (sqlite3 * sqlite, int *retcode)
{
/* testing SE Font functions */
    int ret;
    const char *sql;

/* testing LoadFontFromFile() */
    sql = "SELECT LoadFontFromFile('./Karla-BoldItalic.ttf')";
    ret = execute_check (sqlite, sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: Unable to import a Font #1\n");
	  *retcode -= 1;
	  return 0;
      }

/* testing LoadFontFromFile() - second time */
    sql = "SELECT LoadFontFromFile('./Karla-BoldItalic.ttf')";
    ret = execute_check (sqlite, sql);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: Unexpected success import Font #2\n");
	  *retcode -= 2;
	  return 0;
      }

/* testing ExportFontToFile() */
    sql =
	"SELECT ExportFontToFile(NULL, 'Karla-BoldItalic', 'out-karla-bold-italic.ttf')";
    ret = execute_check (sqlite, sql);
    unlink ("out-karla-bold-italic.ttf");
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: Unable to export a Font\n");
	  *retcode -= 3;
	  return 0;
      }

/* testing GetFontFamily() */
    sql = "SELECT GetFontFamily(font) FROM SE_fonts "
	"WHERE font_facename = 'Karla-BoldItalic'";
    ret = execute_test_string (sqlite, sql, "Karla");
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: Unable to get the Font Family name\n");
	  *retcode -= 4;
	  return 0;
      }

/* testing GetFontFacename() */
    sql = "SELECT GetFontFacename(font) FROM SE_fonts "
	"WHERE font_facename = 'Karla-BoldItalic'";
    ret = execute_test_string (sqlite, sql, "Karla-BoldItalic");
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: Unable to get the Font Facename\n");
	  *retcode -= 5;
	  return 0;
      }

/* testing IsFontBold() */
    sql = "SELECT IsFontBold(font) FROM SE_fonts "
	"WHERE font_facename = 'Karla-BoldItalic'";
    ret = execute_test_boolean (sqlite, sql, 1);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: Unable to get the Font IsBold\n");
	  *retcode -= 6;
	  return 0;
      }

/* testing IsFontItalic() */
    sql = "SELECT IsFontItalic(font) FROM SE_fonts "
	"WHERE font_facename = 'Karla-BoldItalic'";
    ret = execute_test_boolean (sqlite, sql, 1);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: Unable to get the Font IsItalic\n");
	  *retcode -= 7;
	  return 0;
      }

    return 1;
}

int
main (int argc, char *argv[])
{
    int result = 0;
    int ret;
    sqlite3 *db_handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);
    ret =
	sqlite3_exec (db_handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -2;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT CreateStylingTables()", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateStylingTables() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -3;
      }

/* tests */
    ret = -100;
    if (!test_font (db_handle, &ret))
	return ret;

/* closing the DB */
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
    return result;
}
