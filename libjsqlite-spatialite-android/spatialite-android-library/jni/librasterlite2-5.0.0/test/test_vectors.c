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
	"GetMapImageFromVector(NULL, coverage_name, BuildMbr(-180,-90,180,90), 1024, 512) "
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
	"GetMapImageFromVector('MAIN', coverage_name, BuildMbr(-180,-90,180,90), 1024, 512, name) "
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

static int
test_vector_layers (int *retcode)
{
/* testing Vector Layer and MultiLayer Objects */
    rl2VectorLayerPtr lyr;
    rl2VectorMultiLayerPtr multi;
    int ret;
    int i;
    int count;
    const char *str;
    unsigned short value_u16;
    int value;
    unsigned char value_u8;

    multi = rl2_create_multi_layer (10);
    if (multi == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - create MultiLayer #1\n");
	  *retcode += 1;
	  return 0;
      }

    lyr =
	rl2_create_vector_layer (NULL, "table", "geom", NULL, NULL, NULL, 3,
				 4326, 1);
    if (lyr == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - create VectorLayer #1\n");
	  *retcode += 2;
	  return 0;
      }

    ret = rl2_add_layer_to_multilayer (multi, lyr);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - add VectorLayer #1\n");
	  *retcode += 3;
	  return 0;
      }

    count = rl2_get_multilayer_count (multi);
    if (count != 10)
      {
	  fprintf (stderr, "Unexpected value (%d) - MultiLayer count #1\n",
		   count);
	  *retcode += 4;
	  return 0;
      }

    for (i = 0; i < count; i++)
      {
	  lyr = rl2_get_multilayer_item (multi, i);
	  if (i == 0 && lyr == NULL)
	    {
		fprintf (stderr, "Unexpected NULL - MultiLayer get item #1\n");
		*retcode += 5;
		return 0;
	    }
	  else if (i > 0 && lyr != NULL)
	    {
		fprintf (stderr,
			 "Unexpected pointer - MultiLayer get item #1\n");
		*retcode += 5;
		return 0;
	    }
      }

    lyr = rl2_get_multilayer_item (multi, 0);
    if (lyr == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - MultiLayer get item #2\n");
	  *retcode += 6;
	  return 0;
      }

    str = rl2_get_vector_prefix (lyr);
    if (str != NULL)
      {
	  fprintf (stderr, "Unexpected NULL - get Layer Prefix #1\n");
	  *retcode += 7;
	  return 0;
      }

    str = rl2_get_vector_table_name (lyr);
    if (str == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - get Layer Table Name #1\n");
	  *retcode += 8;
	  return 0;
      }
    if (strcmp (str, "table") != 0)
      {
	  fprintf (stderr, "Unexpected value (%s) - get Layer Table Name #1\n",
		   str);
	  *retcode += 9;
	  return 0;
      }

    str = rl2_get_vector_geometry_name (lyr);
    if (str == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - get Layer Geometry Name #1\n");
	  *retcode += 10;
	  return 0;
      }
    if (strcmp (str, "geom") != 0)
      {
	  fprintf (stderr,
		   "Unexpected value (%s) - get Layer Geometry Name #1\n", str);
	  *retcode += 11;
	  return 0;
      }

    ret = rl2_get_vector_geometry_type (lyr, &value_u16);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - get Layer Geometry Type #1\n");
	  *retcode += 12;
	  return 0;
      }
    if (value_u16 != 3)
      {
	  fprintf (stderr,
		   "Unexpected value (%d) - get Layer Geometry Type #1\n",
		   value_u16);
	  *retcode += 13;
	  return 0;
      }

    ret = rl2_get_vector_srid (lyr, &value);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - get Layer SRID #1\n");
	  *retcode += 14;
	  return 0;
      }
    if (value != 4326)
      {
	  fprintf (stderr, "Unexpected value (%d) - get Layer SRID #1\n",
		   value);
	  *retcode += 15;
	  return 0;
      }

    ret = rl2_get_vector_spatial_index (lyr, &value_u8);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - get Layer SpatialIndex #1\n");
	  *retcode += 16;
	  return 0;
      }
    if (value_u8 != 1)
      {
	  fprintf (stderr,
		   "Unexpected value (%d) - get Layer SpatialIndex #1\n",
		   value_u8);
	  *retcode += 17;
	  return 0;
      }

    ret = rl2_set_vector_visibility (lyr, 0);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - set Layer Visibility #1\n");
	  *retcode += 18;
	  return 0;
      }
    ret = rl2_is_vector_visible (lyr, &value);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - is Layer Visible #1\n");
	  *retcode += 19;
	  return 0;
      }
    if (value != 0)
      {
	  fprintf (stderr,
		   "Unexpected value (%d) -is Layer Visible #1\n", value);
	  *retcode += 20;
	  return 0;
      }
    rl2_destroy_multi_layer (multi);

    multi = rl2_create_multi_layer (1);
    if (multi == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - create MultiLayer #2\n");
	  *retcode += 21;
	  return 0;
      }

    lyr =
	rl2_create_vector_layer ("main", "table", "geom", NULL, NULL, NULL, 3,
				 4326, 1);
    if (lyr == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - create VectorLayer #2\n");
	  *retcode += 22;
	  return 0;
      }

    ret = rl2_add_layer_to_multilayer (multi, lyr);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - add VectorLayer #2\n");
	  *retcode += 23;
	  return 0;
      }

    ret = rl2_add_layer_to_multilayer (multi, lyr);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected auccess - add VectorLayer #3\n");
	  *retcode += 24;
	  return 0;
      }

    count = rl2_get_multilayer_count (multi);
    if (count != 1)
      {
	  fprintf (stderr, "Unexpected value (%d) - MultiLayer count #2\n",
		   count);
	  *retcode += 25;
	  return 0;
      }

    lyr = rl2_get_multilayer_item (multi, 0);
    if (lyr == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - MultiLayer get item #3\n");
	  *retcode += 26;
	  return 0;
      }

    ret = rl2_set_multilayer_topogeo (multi, 1);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - set MultiLayer TopoGeo #1\n");
	  *retcode += 27;
	  return 0;
      }
    ret = rl2_is_multilayer_topogeo (multi, &value);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - is MultiLayer TopoGeo #1\n");
	  *retcode += 28;
	  return 0;
      }
    if (value != 1)
      {
	  fprintf (stderr,
		   "Unexpected value (%d) -is MultiLayer TopoGeo #1\n", value);
	  *retcode += 29;
	  return 0;
      }

    ret = rl2_set_multilayer_toponet (multi, 1);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - set MultiLayer TopoNet #1\n");
	  *retcode += 30;
	  return 0;
      }
    ret = rl2_is_multilayer_toponet (multi, &value);
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "Unexpected error - is MultiLayer TopoNet #1\n");
	  *retcode += 31;
	  return 0;
      }
    if (value != 1)
      {
	  fprintf (stderr,
		   "Unexpected value (%d) -is MultiLayer TopoNet #1\n", value);
	  *retcode += 32;
	  return 0;
      }

    str = rl2_get_vector_prefix (lyr);
    if (str == NULL)
      {
	  fprintf (stderr, "Unexpected NULL - get Layer Prefix #2\n");
	  *retcode += 33;
	  return 0;
      }
    if (strcmp (str, "main") != 0)
      {
	  fprintf (stderr, "Unexpected value (%s) - get Layer Prefix #2\n",
		   str);
	  *retcode += 34;
	  return 0;
      }

    lyr = rl2_get_multilayer_item (multi, 2);
    if (lyr != NULL)
      {
	  fprintf (stderr, "Unexpected success - MultiLayer get item #4\n");
	  *retcode += 35;
	  return 0;
      }
    rl2_destroy_multi_layer (multi);

    lyr =
	rl2_create_vector_layer (NULL, NULL, "geom", NULL, NULL, NULL, 3, 4326,
				 1);
    if (lyr != NULL)
      {
	  fprintf (stderr, "Unexpected success - create VectorLayer #3\n");
	  *retcode += 36;
	  return 0;
      }

    lyr =
	rl2_create_vector_layer ("main", "table", NULL, NULL, NULL, NULL, 3,
				 4326, 1);
    if (lyr != NULL)
      {
	  fprintf (stderr, "Unexpected success - create VectorLayer #4\n");
	  *retcode += 37;
	  return 0;
      }

    rl2_destroy_multi_layer (NULL);

    str = rl2_get_vector_prefix (NULL);
    if (str != NULL)
      {
	  fprintf (stderr, "Unexpected success - get Layer Prefix #3\n");
	  *retcode += 38;
	  return 0;
      }

    str = rl2_get_vector_table_name (NULL);
    if (str != NULL)
      {
	  fprintf (stderr, "Unexpected success - get Layer Table Name #2\n");
	  *retcode += 39;
	  return 0;
      }

    str = rl2_get_vector_geometry_name (NULL);
    if (str != NULL)
      {
	  fprintf (stderr, "Unexpected success - get Layer Geometry Name #2\n");
	  *retcode += 40;
	  return 0;
      }

    ret = rl2_get_vector_geometry_type (NULL, &value_u16);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success - get Layer Geometry Type #2\n");
	  *retcode += 41;
	  return 0;
      }

    ret = rl2_get_vector_srid (NULL, &value);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success - get Layer SRID #2\n");
	  *retcode += 42;
	  return 0;
      }

    ret = rl2_get_vector_spatial_index (NULL, &value_u8);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success - get Layer SpatialIndex #2\n");
	  *retcode += 44;
	  return 0;
      }

    multi = rl2_create_multi_layer (-1);
    if (multi != NULL)
      {
	  fprintf (stderr, "Unexpected success - create MultiLayer #3\n");
	  *retcode += 45;
	  return 0;
      }

    count = rl2_get_multilayer_count (NULL);
    if (count > 0)
      {
	  fprintf (stderr, "Unexpected value (%d) - MultiLayer count #2\n",
		   count);
	  *retcode += 46;
	  return 0;
      }

    lyr = rl2_get_multilayer_item (NULL, 0);
    if (lyr != NULL)
      {
	  fprintf (stderr, "Unexpected success - MultiLayer get item #5\n");
	  *retcode += 47;
	  return 0;
      }

    ret = rl2_add_layer_to_multilayer (NULL, lyr);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected auccess - add VectorLayer #4\n");
	  *retcode += 48;
	  return 0;
      }

    ret = rl2_set_vector_visibility (NULL, 1);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected auccess - set Vector Visibility #2\n");
	  *retcode += 49;
	  return 0;
      }

    ret = rl2_is_vector_visible (NULL, &value);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected auccess - is Vector Visible #2\n");
	  *retcode += 50;
	  return 0;
      }

    ret = rl2_set_multilayer_topogeo (NULL, 1);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected auccess - set MultiLayer TopoGeo #2\n");
	  *retcode += 51;
	  return 0;
      }

    ret = rl2_is_multilayer_topogeo (NULL, &value);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected auccess - is MultiLayer TopoGeo #2\n");
	  *retcode += 52;
	  return 0;
      }

    ret = rl2_set_multilayer_toponet (NULL, 1);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected auccess - set MultiLayer TopoNet #2\n");
	  *retcode += 53;
	  return 0;
      }

    ret = rl2_is_multilayer_toponet (NULL, &value);
    if (ret == RL2_OK)
      {
	  fprintf (stderr, "Unexpected auccess - is MultiLayer TopoNet #2\n");
	  *retcode += 54;
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
    ret = -10;
    if (!test_default_style (db_handle, &ret))
	return ret;
    ret = -20;
    if (!test_styles (db_handle, &ret))
	return ret;
    ret = -30;
    if (!test_vector_layers (&ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
    spatialite_shutdown ();
    return result;
}
