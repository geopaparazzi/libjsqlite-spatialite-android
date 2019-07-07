/*

 statistics.c -- helper functions updating internal statistics

 version 4.3, 2015 June 29

 Author: Sandro Furieri a.furieri@lqt.it

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2015
the Initial Developer. All Rights Reserved.

Contributor(s):
Pepijn Van Eeckhoudt <pepijnvaneeckhoudt@luciad.com>
(implementing Android support)
Mark Johnson <mj10777@googlemail.com> 
(extending Drop/Rename Table and RenameColumn)

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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <locale.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#if defined(_WIN32) || defined(WIN32)
#include <io.h>
#define isatty	_isatty
#else
#include <unistd.h>
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite/gaiageo.h>
#include <spatialite.h>
#include <spatialite_private.h>
#include <spatialite/gaiaaux.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

struct field_item_infos
{
    int ordinal;
    char *col_name;
    int null_values;
    int integer_values;
    int double_values;
    int text_values;
    int blob_values;
    int max_size;
    int int_minmax_set;
    int int_min;
    int int_max;
    int dbl_minmax_set;
    double dbl_min;
    double dbl_max;
    struct field_item_infos *next;
};

struct field_container_infos
{
    struct field_item_infos *first;
    struct field_item_infos *last;
};

static int
do_update_layer_statistics_v4 (sqlite3 * sqlite, const char *table,
			       const char *column, int count, int has_coords,
			       double min_x, double min_y, double max_x,
			       double max_y)
{
/* update GEOMETRY_COLUMNS_STATISTICS Version >= 4.0.0 */
    char sql[8192];
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;

    strcpy (sql, "INSERT OR REPLACE INTO geometry_columns_statistics ");
    strcat (sql, "(f_table_name, f_geometry_column, last_verified, ");
    strcat (sql, "row_count, extent_min_x, extent_min_y, ");
    strcat (sql, "extent_max_x, extent_max_y) VALUES (?, ?, ");
    strcat (sql, "strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

/* binding INSERT params */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, count);
    if (has_coords)
      {
	  sqlite3_bind_double (stmt, 4, min_x);
	  sqlite3_bind_double (stmt, 5, min_y);
	  sqlite3_bind_double (stmt, 6, max_x);
	  sqlite3_bind_double (stmt, 7, max_y);
      }
    else
      {
	  sqlite3_bind_null (stmt, 4);
	  sqlite3_bind_null (stmt, 5);
	  sqlite3_bind_null (stmt, 6);
	  sqlite3_bind_null (stmt, 7);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	error = 1;
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static int
do_update_layer_statistics (sqlite3 * sqlite, const char *table,
			    const char *column, int count, int has_coords,
			    double min_x, double min_y, double max_x,
			    double max_y)
{
/* update LAYER_STATISTICS [single table/geometry] */
    char sql[8192];
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;
    int metadata_version = checkSpatialMetaData (sqlite);

    if (metadata_version == 3)
      {
	  /* current metadata style >= v.4.0.0 */
	  return do_update_layer_statistics_v4 (sqlite, table, column, count,
						has_coords, min_x, min_y, max_x,
						max_y);
      }

    if (!check_layer_statistics (sqlite))
	return 0;
    strcpy (sql, "INSERT OR REPLACE INTO layer_statistics ");
    strcat (sql, "(raster_layer, table_name, geometry_column, ");
    strcat (sql, "row_count, extent_min_x, extent_min_y, ");
    strcat (sql, "extent_max_x, extent_max_y) ");
    strcat (sql, "VALUES (0, ?, ?, ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
/* binding INSERT params */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, count);
    if (has_coords)
      {
	  sqlite3_bind_double (stmt, 4, min_x);
	  sqlite3_bind_double (stmt, 5, min_y);
	  sqlite3_bind_double (stmt, 6, max_x);
	  sqlite3_bind_double (stmt, 7, max_y);
      }
    else
      {
	  sqlite3_bind_null (stmt, 4);
	  sqlite3_bind_null (stmt, 5);
	  sqlite3_bind_null (stmt, 6);
	  sqlite3_bind_null (stmt, 7);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	error = 1;
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static int
do_update_views_layer_statistics_v4 (sqlite3 * sqlite, const char *table,
				     const char *column, int count,
				     int has_coords, double min_x,
				     double min_y, double max_x, double max_y)
{
/* update VIEWS_GEOMETRY_COLUMNS_STATISTICS Version >= 4.0.0 */
    char sql[8192];
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;

    strcpy (sql, "INSERT OR REPLACE INTO views_geometry_columns_statistics ");
    strcat (sql, "(view_name, view_geometry, last_verified, ");
    strcat (sql, "row_count, extent_min_x, extent_min_y, ");
    strcat (sql, "extent_max_x, extent_max_y) VALUES (?, ?, ");
    strcat (sql, "strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

/* binding INSERT params */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, count);
    if (has_coords)
      {
	  sqlite3_bind_double (stmt, 4, min_x);
	  sqlite3_bind_double (stmt, 5, min_y);
	  sqlite3_bind_double (stmt, 6, max_x);
	  sqlite3_bind_double (stmt, 7, max_y);
      }
    else
      {
	  sqlite3_bind_null (stmt, 4);
	  sqlite3_bind_null (stmt, 5);
	  sqlite3_bind_null (stmt, 6);
	  sqlite3_bind_null (stmt, 7);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	error = 1;
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static int
do_update_views_layer_statistics (sqlite3 * sqlite, const char *table,
				  const char *column, int count,
				  int has_coords, double min_x, double min_y,
				  double max_x, double max_y)
{
/* update VIEWS_LAYER_STATISTICS [single table/geometry] */
    char sql[8192];
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;
    int metadata_version = checkSpatialMetaData (sqlite);

    if (metadata_version == 3)
      {
	  /* current metadata style >= v.4.0.0 */
	  return do_update_views_layer_statistics_v4 (sqlite, table, column,
						      count, has_coords, min_x,
						      min_y, max_x, max_y);
      }

    if (!check_views_layer_statistics (sqlite))
	return 0;
    strcpy (sql, "INSERT OR REPLACE INTO views_layer_statistics ");
    strcat (sql, "(view_name, view_geometry, ");
    strcat (sql, "row_count, extent_min_x, extent_min_y, ");
    strcat (sql, "extent_max_x, extent_max_y) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

/* binding INSERT params */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, count);
    if (has_coords)
      {
	  sqlite3_bind_double (stmt, 4, min_x);
	  sqlite3_bind_double (stmt, 5, min_y);
	  sqlite3_bind_double (stmt, 6, max_x);
	  sqlite3_bind_double (stmt, 7, max_y);
      }
    else
      {
	  sqlite3_bind_null (stmt, 4);
	  sqlite3_bind_null (stmt, 5);
	  sqlite3_bind_null (stmt, 6);
	  sqlite3_bind_null (stmt, 7);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	error = 1;
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static int
do_update_virts_layer_statistics_v4 (sqlite3 * sqlite, const char *table,
				     const char *column, int count,
				     int has_coords, double min_x,
				     double min_y, double max_x, double max_y)
{
/* update VIRTS_GEOMETRY_COLUMNS_STATISTICS Version >= 4.0.0 */
    char sql[8192];
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;

    strcpy (sql, "INSERT OR REPLACE INTO virts_geometry_columns_statistics ");
    strcat (sql, "(virt_name, virt_geometry, last_verified, ");
    strcat (sql, "row_count, extent_min_x, extent_min_y, ");
    strcat (sql, "extent_max_x, extent_max_y) VALUES (?, ?, ");
    strcat (sql, "strftime('%Y-%m-%dT%H:%M:%fZ', 'now'), ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

/* binding INSERT params */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, count);
    if (has_coords)
      {
	  sqlite3_bind_double (stmt, 4, min_x);
	  sqlite3_bind_double (stmt, 5, min_y);
	  sqlite3_bind_double (stmt, 6, max_x);
	  sqlite3_bind_double (stmt, 7, max_y);
      }
    else
      {
	  sqlite3_bind_null (stmt, 4);
	  sqlite3_bind_null (stmt, 5);
	  sqlite3_bind_null (stmt, 6);
	  sqlite3_bind_null (stmt, 7);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	error = 1;
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static int
do_update_virts_layer_statistics (sqlite3 * sqlite, const char *table,
				  const char *column, int count,
				  int has_coords, double min_x, double min_y,
				  double max_x, double max_y)
{
/* update VIRTS_LAYER_STATISTICS [single table/geometry] */
    char sql[8192];
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;
    int metadata_version = checkSpatialMetaData (sqlite);

    if (metadata_version == 3)
      {
	  /* current metadata style >= v.4.0.0 */
	  return do_update_virts_layer_statistics_v4 (sqlite, table, column,
						      count, has_coords, min_x,
						      min_y, max_x, max_y);
      }

    if (!check_virts_layer_statistics (sqlite))
	return 0;
    strcpy (sql, "INSERT OR REPLACE INTO virts_layer_statistics ");
    strcat (sql, "(virt_name, virt_geometry, ");
    strcat (sql, "row_count, extent_min_x, extent_min_y, ");
    strcat (sql, "extent_max_x, extent_max_y) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

/* binding INSERT params */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, count);
    if (has_coords)
      {
	  sqlite3_bind_double (stmt, 4, min_x);
	  sqlite3_bind_double (stmt, 5, min_y);
	  sqlite3_bind_double (stmt, 6, max_x);
	  sqlite3_bind_double (stmt, 7, max_y);
      }
    else
      {
	  sqlite3_bind_null (stmt, 4);
	  sqlite3_bind_null (stmt, 5);
	  sqlite3_bind_null (stmt, 6);
	  sqlite3_bind_null (stmt, 7);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	error = 1;
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static void
update_field_infos (struct field_container_infos *infos, int ordinal,
		    const char *col_name, const char *type, int size, int count)
{
/* updating the field container infos */
    int len;
    struct field_item_infos *p = infos->first;
    while (p)
      {
	  if (strcasecmp (col_name, p->col_name) == 0)
	    {
		/* updating an already defined field */
		if (strcasecmp (type, "null") == 0)
		    p->null_values += count;
		if (strcasecmp (type, "integer") == 0)
		    p->integer_values += count;
		if (strcasecmp (type, "real") == 0)
		    p->double_values += count;
		if (strcasecmp (type, "text") == 0)
		  {
		      p->text_values += count;
		      if (size > p->max_size)
			  p->max_size = size;
		  }
		if (strcasecmp (type, "blob") == 0)
		  {
		      p->blob_values += count;
		      if (size > p->max_size)
			  p->max_size = size;
		  }
		return;
	    }
	  p = p->next;
      }
/* inserting a new field */
    p = malloc (sizeof (struct field_item_infos));
    p->ordinal = ordinal;
    len = strlen (col_name);
    p->col_name = malloc (len + 1);
    strcpy (p->col_name, col_name);
    p->null_values = 0;
    p->integer_values = 0;
    p->double_values = 0;
    p->text_values = 0;
    p->blob_values = 0;
    p->max_size = -1;
    p->int_minmax_set = 0;
    p->int_min = 0;
    p->int_max = 0;
    p->dbl_minmax_set = 0;
    p->dbl_min = 0.0;
    p->dbl_max = 0.0;
    p->next = NULL;
    if (strcasecmp (type, "null") == 0)
	p->null_values += count;
    if (strcasecmp (type, "integer") == 0)
	p->integer_values += count;
    if (strcasecmp (type, "real") == 0)
	p->double_values += count;
    if (strcasecmp (type, "text") == 0)
      {
	  p->text_values += count;
	  if (size > p->max_size)
	      p->max_size = size;
      }
    if (strcasecmp (type, "blob") == 0)
      {
	  p->blob_values += count;
	  if (size > p->max_size)
	      p->max_size = size;
      }
    if (infos->first == NULL)
	infos->first = p;
    if (infos->last != NULL)
	infos->last->next = p;
    infos->last = p;
}

static void
update_field_infos_int_minmax (struct field_container_infos *infos,
			       const char *col_name, int int_min, int int_max)
{
/* updating the field container infos - Int MinMax */
    struct field_item_infos *p = infos->first;
    while (p)
      {
	  if (strcasecmp (col_name, p->col_name) == 0)
	    {
		p->int_minmax_set = 1;
		p->int_min = int_min;
		p->int_max = int_max;
		return;
	    }
	  p = p->next;
      }
}

static void
update_field_infos_double_minmax (struct field_container_infos *infos,
				  const char *col_name, double dbl_min,
				  double dbl_max)
{
/* updating the field container infos - Double MinMax */
    struct field_item_infos *p = infos->first;
    while (p)
      {
	  if (strcasecmp (col_name, p->col_name) == 0)
	    {
		p->dbl_minmax_set = 1;
		p->dbl_min = dbl_min;
		p->dbl_max = dbl_max;
		return;
	    }
	  p = p->next;
      }
}

static void
free_field_infos (struct field_container_infos *infos)
{
/* memory cleanup - freeing a field infos container */
    struct field_item_infos *p = infos->first;
    struct field_item_infos *pn;
    while (p)
      {
	  /* destroying field items */
	  pn = p->next;
	  if (p->col_name)
	      free (p->col_name);
	  free (p);
	  p = pn;
      }
}

static int
do_update_field_infos (sqlite3 * sqlite, const char *table,
		       const char *column, struct field_container_infos *infos)
{
/* update GEOMETRY_COLUMNS_FIELD_INFOS Version >= 4.0.0 */
    char sql[8192];
    char *sql_statement;
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;
    struct field_item_infos *p = infos->first;

/* deleting any previous row */
    sql_statement = sqlite3_mprintf ("DELETE FROM geometry_columns_field_infos "
				     "WHERE Lower(f_table_name) = Lower(%Q) AND "
				     "Lower(f_geometry_column) = Lower(%Q)",
				     table, column);
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;

/* reinserting yet again */
    strcpy (sql, "INSERT INTO geometry_columns_field_infos ");
    strcat (sql, "(f_table_name, f_geometry_column, ordinal, ");
    strcat (sql, "column_name, null_values, integer_values, ");
    strcat (sql, "double_values, text_values, blob_values, max_size, ");
    strcat (sql, "integer_min, integer_max, double_min, double_max) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    while (p)
      {
/* binding INSERT params */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, p->ordinal);
	  sqlite3_bind_text (stmt, 4, p->col_name, strlen (p->col_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 5, p->null_values);
	  sqlite3_bind_int (stmt, 6, p->integer_values);
	  sqlite3_bind_int (stmt, 7, p->double_values);
	  sqlite3_bind_int (stmt, 8, p->text_values);
	  sqlite3_bind_int (stmt, 9, p->blob_values);
	  if (p->max_size < 0)
	      sqlite3_bind_null (stmt, 10);
	  else
	      sqlite3_bind_int (stmt, 10, p->max_size);
	  if (p->int_minmax_set)
	    {
		sqlite3_bind_int (stmt, 11, p->int_min);
		sqlite3_bind_int (stmt, 12, p->int_max);
	    }
	  else
	    {
		sqlite3_bind_null (stmt, 11);
		sqlite3_bind_null (stmt, 12);
	    }
	  if (p->dbl_minmax_set)
	    {
		sqlite3_bind_double (stmt, 13, p->dbl_min);
		sqlite3_bind_double (stmt, 14, p->dbl_max);
	    }
	  else
	    {
		sqlite3_bind_null (stmt, 13);
		sqlite3_bind_null (stmt, 14);
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	      error = 1;
	  p = p->next;
      }
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static int
do_update_views_field_infos (sqlite3 * sqlite, const char *table,
			     const char *column,
			     struct field_container_infos *infos)
{
/* update VIEW_GEOMETRY_COLUMNS_FIELD_INFOS Version >= 4.0.0 */
    char sql[8192];
    char *sql_statement;
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;
    struct field_item_infos *p = infos->first;

/* deleting any previous row */
    sql_statement =
	sqlite3_mprintf ("DELETE FROM views_geometry_columns_field_infos "
			 "WHERE Lower(view_name) = Lower(%Q) AND "
			 "Lower(view_geometry) = Lower(%Q)", table, column);
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;

/* reinserting yet again */
    strcpy (sql, "INSERT INTO views_geometry_columns_field_infos ");
    strcat (sql, "(view_name, view_geometry, ordinal, ");
    strcat (sql, "column_name, null_values, integer_values, ");
    strcat (sql, "double_values, text_values, blob_values, max_size, ");
    strcat (sql, "integer_min, integer_max, double_min, double_max) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    while (p)
      {
/* binding INSERT params */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, p->ordinal);
	  sqlite3_bind_text (stmt, 4, p->col_name, strlen (p->col_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 5, p->null_values);
	  sqlite3_bind_int (stmt, 6, p->integer_values);
	  sqlite3_bind_int (stmt, 7, p->double_values);
	  sqlite3_bind_int (stmt, 8, p->text_values);
	  sqlite3_bind_int (stmt, 9, p->blob_values);
	  if (p->max_size < 0)
	      sqlite3_bind_null (stmt, 10);
	  else
	      sqlite3_bind_int (stmt, 10, p->max_size);
	  if (p->int_minmax_set)
	    {
		sqlite3_bind_int (stmt, 11, p->int_min);
		sqlite3_bind_int (stmt, 12, p->int_max);
	    }
	  else
	    {
		sqlite3_bind_null (stmt, 11);
		sqlite3_bind_null (stmt, 12);
	    }
	  if (p->dbl_minmax_set)
	    {
		sqlite3_bind_double (stmt, 13, p->dbl_min);
		sqlite3_bind_double (stmt, 14, p->dbl_max);
	    }
	  else
	    {
		sqlite3_bind_null (stmt, 13);
		sqlite3_bind_null (stmt, 14);
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	      error = 1;
	  p = p->next;
      }
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static int
do_update_virts_field_infos (sqlite3 * sqlite, const char *table,
			     const char *column,
			     struct field_container_infos *infos)
{
/* update VIRTS_GEOMETRY_COLUMNS_FIELD_INFOS Version >= 4.0.0 */
    char sql[8192];
    char *sql_statement;
    int ret;
    int error = 0;
    sqlite3_stmt *stmt;
    struct field_item_infos *p = infos->first;

/* deleting any previous row */
    sql_statement =
	sqlite3_mprintf ("DELETE FROM virts_geometry_columns_field_infos "
			 "WHERE Lower(virt_name) = Lower(%Q) AND "
			 "Lower(virt_geometry) = Lower(%Q)", table, column);
    ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;

/* reinserting yet again */
    strcpy (sql, "INSERT INTO virts_geometry_columns_field_infos ");
    strcat (sql, "(virt_name, virt_geometry, ordinal, ");
    strcat (sql, "column_name, null_values, integer_values, ");
    strcat (sql, "double_values, text_values, blob_values, max_size, ");
    strcat (sql, "integer_min, integer_max, double_min, double_max) ");
    strcat (sql, "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

/* compiling SQL prepared statement */
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    while (p)
      {
/* binding INSERT params */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, table, strlen (table), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, column, strlen (column), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, p->ordinal);
	  sqlite3_bind_text (stmt, 4, p->col_name, strlen (p->col_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 5, p->null_values);
	  sqlite3_bind_int (stmt, 6, p->integer_values);
	  sqlite3_bind_int (stmt, 7, p->double_values);
	  sqlite3_bind_int (stmt, 8, p->text_values);
	  sqlite3_bind_int (stmt, 9, p->blob_values);
	  if (p->max_size < 0)
	      sqlite3_bind_null (stmt, 10);
	  else
	      sqlite3_bind_int (stmt, 10, p->max_size);
	  if (p->int_minmax_set)
	    {
		sqlite3_bind_int (stmt, 11, p->int_min);
		sqlite3_bind_int (stmt, 12, p->int_max);
	    }
	  else
	    {
		sqlite3_bind_null (stmt, 11);
		sqlite3_bind_null (stmt, 12);
	    }
	  if (p->dbl_minmax_set)
	    {
		sqlite3_bind_double (stmt, 13, p->dbl_min);
		sqlite3_bind_double (stmt, 14, p->dbl_max);
	    }
	  else
	    {
		sqlite3_bind_null (stmt, 13);
		sqlite3_bind_null (stmt, 14);
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	      error = 1;
	  p = p->next;
      }
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    return 1;
}

static int
do_compute_minmax (sqlite3 * sqlite, const char *table,
		   struct field_container_infos *infos)
{
/* Pass2 - computing Integer / Double min/max ranges */
    char *quoted;
    char *sql_statement;
    int int_min;
    int int_max;
    double dbl_min;
    double dbl_max;
    int ret;
    int i;
    int c;
    char **results;
    int rows;
    int columns;
    const char *col_name;
    int is_double;
    int comma = 0;
    int empty = 1;
    gaiaOutBuffer out_buf;
    struct field_item_infos *ptr;

    gaiaOutBufferInitialize (&out_buf);
    gaiaAppendToOutBuffer (&out_buf, "SELECT DISTINCT ");
    ptr = infos->first;
    while (ptr)
      {
	  quoted = gaiaDoubleQuotedSql (ptr->col_name);
	  if (ptr->integer_values >= 0 && ptr->double_values == 0
	      && ptr->blob_values == 0 && ptr->text_values == 0)
	    {
		if (comma)
		    sql_statement =
			sqlite3_mprintf (", 0, %Q, min(\"%s\"), max(\"%s\")",
					 ptr->col_name, quoted, quoted);
		else
		  {
		      comma = 1;
		      sql_statement =
			  sqlite3_mprintf (" 0, %Q, min(\"%s\"), max(\"%s\")",
					   ptr->col_name, quoted, quoted);
		  }
		gaiaAppendToOutBuffer (&out_buf, sql_statement);
		sqlite3_free (sql_statement);
		empty = 0;
	    }
	  if (ptr->double_values >= 0 && ptr->integer_values == 0
	      && ptr->blob_values == 0 && ptr->text_values == 0)
	    {
		if (comma)
		    sql_statement =
			sqlite3_mprintf (", 1, %Q, min(\"%s\"), max(\"%s\")",
					 ptr->col_name, quoted, quoted);
		else
		  {
		      comma = 1;
		      sql_statement =
			  sqlite3_mprintf (" 1, %Q, min(\"%s\"), max(\"%s\")",
					   ptr->col_name, quoted, quoted);
		  }
		gaiaAppendToOutBuffer (&out_buf, sql_statement);
		sqlite3_free (sql_statement);
		empty = 0;
	    }
	  free (quoted);
	  ptr = ptr->next;
      }
    if (out_buf.Buffer == NULL)
	return 0;
    if (empty)
      {
	  /* no columns to check */
	  gaiaOutBufferReset (&out_buf);
	  return 1;

      }
    quoted = gaiaDoubleQuotedSql (table);
    sql_statement = sqlite3_mprintf (" FROM \"%s\"", quoted);
    free (quoted);
    gaiaAppendToOutBuffer (&out_buf, sql_statement);
    sqlite3_free (sql_statement);
/* executing the SQL query */
    ret = sqlite3_get_table (sqlite, out_buf.Buffer, &results, &rows, &columns,
			     NULL);
    gaiaOutBufferReset (&out_buf);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		for (c = 0; c < columns; c += 4)
		  {
		      /* retrieving field infos */
		      is_double = atoi (results[(i * columns) + c + 0]);
		      col_name = results[(i * columns) + c + 1];
		      if (results[(i * columns) + c + 2] != NULL
			  && results[(i * columns) + c + 3] != NULL)
			{
			    if (!is_double)
			      {
				  int_min =
				      atoi (results[(i * columns) + c + 2]);
				  int_max =
				      atoi (results[(i * columns) + c + 3]);
				  update_field_infos_int_minmax (infos,
								 col_name,
								 int_min,
								 int_max);
			      }
			    else
			      {
				  dbl_min =
				      atof (results[(i * columns) + c + 2]);
				  dbl_max =
				      atof (results[(i * columns) + c + 3]);
				  update_field_infos_double_minmax (infos,
								    col_name,
								    dbl_min,
								    dbl_max);
			      }
			}
		  }
	    }
      }
    sqlite3_free_table (results);

    return 1;
}

static void
copy_attributes_into_layer (struct field_container_infos *infos,
			    gaiaVectorLayerPtr lyr)
{
/* copying the AttributeField definitions into the VectorLayer */
    gaiaLayerAttributeFieldPtr fld;
    int len;
    struct field_item_infos *p = infos->first;
    while (p)
      {
	  /* adding an AttributeField definition */
	  fld = malloc (sizeof (gaiaLayerAttributeField));
	  fld->Ordinal = p->ordinal;
	  len = strlen (p->col_name);
	  fld->AttributeFieldName = malloc (len + 1);
	  strcpy (fld->AttributeFieldName, p->col_name);
	  fld->NullValuesCount = p->null_values;
	  fld->IntegerValuesCount = p->integer_values;
	  fld->DoubleValuesCount = p->double_values;
	  fld->TextValuesCount = p->text_values;
	  fld->BlobValuesCount = p->blob_values;
	  fld->MaxSize = NULL;
	  fld->IntRange = NULL;
	  fld->DoubleRange = NULL;
	  if (p->max_size)
	    {
		fld->MaxSize = malloc (sizeof (gaiaAttributeFieldMaxSize));
		fld->MaxSize->MaxSize = p->max_size;
	    }
	  if (p->int_minmax_set)
	    {
		fld->IntRange = malloc (sizeof (gaiaAttributeFieldIntRange));
		fld->IntRange->MinValue = p->int_min;
		fld->IntRange->MaxValue = p->int_max;
	    }
	  if (p->dbl_minmax_set)
	    {
		fld->DoubleRange =
		    malloc (sizeof (gaiaAttributeFieldDoubleRange));
		fld->DoubleRange->MinValue = p->dbl_min;
		fld->DoubleRange->MaxValue = p->dbl_max;
	    }
	  fld->Next = NULL;
	  if (lyr->First == NULL)
	      lyr->First = fld;
	  if (lyr->Last != NULL)
	      lyr->Last->Next = fld;
	  lyr->Last = fld;
	  p = p->next;
      }
}

SPATIALITE_PRIVATE int
doComputeFieldInfos (void *p_sqlite, const char *table,
		     const char *column, int stat_type, void *p_lyr)
{
/* computes FIELD_INFOS [single table/geometry] */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    gaiaVectorLayerPtr lyr = (gaiaVectorLayerPtr) p_lyr;
    char *sql_statement;
    char *quoted;
    int ret;
    int i;
    int c;
    char **results;
    int rows;
    int columns;
    int ordinal;
    const char *col_name;
    const char *type;
    const char *sz;
    int size;
    int count;
    int error = 0;
    int comma = 0;
    gaiaOutBuffer out_buf;
    gaiaOutBuffer group_by;
    struct field_container_infos infos;

    gaiaOutBufferInitialize (&out_buf);
    gaiaOutBufferInitialize (&group_by);
    infos.first = NULL;
    infos.last = NULL;

/* retrieving the column names for the current table */
/* then building the SQL query statement */
    quoted = gaiaDoubleQuotedSql (table);
    sql_statement = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", quoted);
    free (quoted);
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;

    if (rows < 1)
	;
    else
      {
	  gaiaAppendToOutBuffer (&out_buf, "SELECT DISTINCT Count(*)");
	  gaiaAppendToOutBuffer (&group_by, "GROUP BY");
	  for (i = 1; i <= rows; i++)
	    {
		ordinal = atoi (results[(i * columns) + 0]);
		col_name = results[(i * columns) + 1];
		quoted = gaiaDoubleQuotedSql (col_name);
		sql_statement =
		    sqlite3_mprintf
		    (", %d, %Q AS col_%d, typeof(\"%s\") AS typ_%d, max(length(\"%s\"))",
		     ordinal, col_name, ordinal, quoted, ordinal, quoted);
		free (quoted);
		gaiaAppendToOutBuffer (&out_buf, sql_statement);
		sqlite3_free (sql_statement);
		if (!comma)
		  {
		      comma = 1;
		      sql_statement =
			  sqlite3_mprintf (" col_%d, typ_%d", ordinal, ordinal);
		  }
		else
		    sql_statement =
			sqlite3_mprintf (", col_%d, typ_%d", ordinal, ordinal);
		gaiaAppendToOutBuffer (&group_by, sql_statement);
		sqlite3_free (sql_statement);
	    }
      }
    sqlite3_free_table (results);

    if (out_buf.Buffer == NULL)
	return 0;
    quoted = gaiaDoubleQuotedSql (table);
    sql_statement = sqlite3_mprintf (" FROM \"%s\" ", quoted);
    free (quoted);
    gaiaAppendToOutBuffer (&out_buf, sql_statement);
    sqlite3_free (sql_statement);
    gaiaAppendToOutBuffer (&out_buf, group_by.Buffer);
    gaiaOutBufferReset (&group_by);

/* executing the SQL query */
    ret = sqlite3_get_table (sqlite, out_buf.Buffer, &results, &rows, &columns,
			     NULL);
    gaiaOutBufferReset (&out_buf);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		count = atoi (results[(i * columns) + 0]);
		for (c = 1; c < columns; c += 4)
		  {
		      /* retrieving field infos */
		      ordinal = atoi (results[(i * columns) + c + 0]);
		      col_name = results[(i * columns) + c + 1];
		      type = results[(i * columns) + c + 2];
		      sz = results[(i * columns) + c + 3];
		      if (sz == NULL)
			  size = -1;
		      else
			  size = atoi (sz);
		      update_field_infos (&infos, ordinal, col_name, type, size,
					  count);
		  }
	    }
      }
    sqlite3_free_table (results);

/* Pass-2: computing INTEGER and DOUBLE min/max ranges */
    if (!error)
      {
	  if (!do_compute_minmax (sqlite, table, &infos))
	      error = 1;
      }

    switch (stat_type)
      {
      case SPATIALITE_STATISTICS_LEGACY:
	  if (!error)
	      copy_attributes_into_layer (&infos, lyr);
	  free_field_infos (&infos);
	  if (error)
	      return 0;
	  return 1;
	  break;
      case SPATIALITE_STATISTICS_GENUINE:
	  if (!do_update_field_infos (sqlite, table, column, &infos))
	      error = 1;
	  break;
      case SPATIALITE_STATISTICS_VIEWS:
	  if (!do_update_views_field_infos (sqlite, table, column, &infos))
	      error = 1;
	  break;
      case SPATIALITE_STATISTICS_VIRTS:
	  if (!do_update_virts_field_infos (sqlite, table, column, &infos))
	      error = 1;
	  break;
      };
    free_field_infos (&infos);
    if (error)
	return 0;
    return 1;
}

static int
do_compute_layer_statistics (sqlite3 * sqlite, const char *table,
			     const char *column, int stat_type)
{
/* computes LAYER_STATISTICS [single table/geometry] */
    int ret;
    int error = 0;
    int count;
    double min_x = DBL_MAX;
    double min_y = DBL_MAX;
    double max_x = 0.0 - DBL_MAX;
    double max_y = 0.0 - DBL_MAX;
    int has_coords = 1;
    char *quoted;
    char *col_quoted;
    char *sql_statement;
    sqlite3_stmt *stmt;
    int metadata_version = checkSpatialMetaData (sqlite);

    quoted = gaiaDoubleQuotedSql ((const char *) table);
    col_quoted = gaiaDoubleQuotedSql ((const char *) column);

    if (metadata_version == 4)
      {
	  /* GeoPackage Vector only */
	  sql_statement = sqlite3_mprintf ("UPDATE gpkg_contents SET "
					   "min_x = (SELECT Min(MbrMinX(%s)) FROM \"%s\"),"
					   "min_y = (SELECT Min(MbrMinY(%s)) FROM \"%s\"),"
					   "max_x = (SELECT Max(MbrMinX(%s)) FROM \"%s\"),"
					   "max_y = (SELECT Max(MbrMinY(%s)) FROM \"%s\"),"
					   "last_change = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
					   "WHERE ((lower(table_name) = lower('%s')) AND (Lower(data_type) = 'features'))",
					   col_quoted, quoted, col_quoted,
					   quoted, col_quoted, quoted,
					   col_quoted, quoted, quoted);
	  free (quoted);
	  free (col_quoted);
	  if (sqlite3_exec (sqlite, sql_statement, NULL, NULL, NULL) !=
	      SQLITE_OK)
	    {
		sqlite3_free (sql_statement);
		return 0;
	    }
	  sqlite3_free (sql_statement);
	  return 1;
      }
    else
      {
	  sql_statement = sqlite3_mprintf ("SELECT Count(*), "
					   "Min(MbrMinX(\"%s\")), Min(MbrMinY(\"%s\")), Max(MbrMaxX(\"%s\")), Max(MbrMaxY(\"%s\")) "
					   "FROM \"%s\"", col_quoted,
					   col_quoted, col_quoted, col_quoted,
					   quoted);
      }
    free (quoted);
    free (col_quoted);

/* compiling SQL prepared statement */
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		count = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_NULL)
		    has_coords = 0;
		else
		    min_x = sqlite3_column_double (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_NULL)
		    has_coords = 0;
		else
		    min_y = sqlite3_column_double (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_NULL)
		    has_coords = 0;
		else
		    max_x = sqlite3_column_double (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_NULL)
		    has_coords = 0;
		else
		    max_y = sqlite3_column_double (stmt, 4);
		switch (stat_type)
		  {
		  case SPATIALITE_STATISTICS_GENUINE:
		      if (!do_update_layer_statistics
			  (sqlite, table, column, count, has_coords, min_x,
			   min_y, max_x, max_y))
			  error = 1;
		      break;
		  case SPATIALITE_STATISTICS_VIEWS:
		      if (!do_update_views_layer_statistics
			  (sqlite, table, column, count, has_coords, min_x,
			   min_y, max_x, max_y))
			  error = 1;
		      break;
		  case SPATIALITE_STATISTICS_VIRTS:
		      if (!do_update_virts_layer_statistics
			  (sqlite, table, column, count, has_coords, min_x,
			   min_y, max_x, max_y))
			  error = 1;
		      break;
		  };
	    }
	  else
	      error = 1;
      }
    ret = sqlite3_finalize (stmt);
    if (ret != SQLITE_OK)
	return 0;
    if (error)
	return 0;
    if (metadata_version == 3)
      {
	  /* current metadata style >= v.4.0.0 */
	  if (!doComputeFieldInfos (sqlite, table, column, stat_type, NULL))
	      return 0;
      }
    return 1;
}

static int
genuine_layer_statistics_v4 (sqlite3 * sqlite, const char *table,
			     const char *column)
{
/* updating GEOMETRY_COLUMNS_STATISTICS Version >= 4.0.0 */
    char *sql_statement;
    int ret;
    const char *f_table_name;
    const char *f_geometry_column;
    int i;
    char **results;
    int rows;
    int columns;
    int error = 0;

    if (table == NULL && column == NULL)
      {
	  /* processing any table/geometry found in GEOMETRY_COLUMNS */
	  sql_statement =
	      sqlite3_mprintf ("SELECT t.f_table_name, t.f_geometry_column "
			       "FROM geometry_columns_time AS t, "
			       "geometry_columns_statistics AS s "
			       "WHERE Lower(s.f_table_name) = Lower(t.f_table_name) AND "
			       "Lower(s.f_geometry_column) = Lower(t.f_geometry_column) AND "
			       "(s.last_verified < t.last_insert OR "
			       "s.last_verified < t.last_update OR "
			       "s.last_verified < t.last_delete OR "
			       "s.last_verified IS NULL)");
      }
    else if (column == NULL)
      {
	  /* processing any geometry belonging to this table */
	  sql_statement =
	      sqlite3_mprintf ("SELECT t.f_table_name, t.f_geometry_column "
			       "FROM geometry_columns_time AS t, "
			       "geometry_columns_statistics AS s "
			       "WHERE Lower(t.f_table_name) = Lower(%Q) AND "
			       "Lower(s.f_table_name) = Lower(t.f_table_name) AND "
			       "Lower(s.f_geometry_column) = Lower(t.f_geometry_column) AND "
			       "(s.last_verified < t.last_insert OR "
			       "s.last_verified < t.last_update OR "
			       "s.last_verified < t.last_delete OR "
			       "s.last_verified IS NULL)", table);
      }
    else
      {
	  /* processing a single table/geometry entry */
	  sql_statement =
	      sqlite3_mprintf ("SELECT t.f_table_name, t.f_geometry_column "
			       "FROM geometry_columns_time AS t, "
			       "geometry_columns_statistics AS s "
			       "WHERE Lower(t.f_table_name) = Lower(%Q) AND "
			       "Lower(t.f_geometry_column) = Lower(%Q) AND "
			       "Lower(s.f_table_name) = Lower(t.f_table_name) AND "
			       "Lower(s.f_geometry_column) = Lower(t.f_geometry_column) AND "
			       "(s.last_verified < t.last_insert OR "
			       "s.last_verified < t.last_update OR "
			       "s.last_verified < t.last_delete OR "
			       "s.last_verified IS NULL)", table, column);
      }
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;

    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		f_table_name = results[(i * columns) + 0];
		f_geometry_column = results[(i * columns) + 1];
		if (!do_compute_layer_statistics
		    (sqlite, f_table_name, f_geometry_column,
		     SPATIALITE_STATISTICS_GENUINE))
		  {
		      error = 1;
		      break;
		  }
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;
    return 1;
}

static int
genuine_layer_statistics (sqlite3 * sqlite, const char *table,
			  const char *column)
{
/* updating genuine LAYER_STATISTICS metadata */
    char *sql_statement;
    int ret;
    const char *f_table_name;
    const char *f_geometry_column;
    int i;
    char **results;
    int rows;
    int columns;
    int error = 0;
    int metadata_version = checkSpatialMetaData (sqlite);

    if (metadata_version == 3)
      {
	  /* current metadata style >= v.4.0.0 */
	  return genuine_layer_statistics_v4 (sqlite, table, column);
      }

    if (table == NULL && column == NULL)
      {
	  /* processing any table/geometry found in GEOMETRY_COLUMNS */
	  if (metadata_version == 4)
	    {
		/* GeoPackage Vector only */
		sql_statement =
		    sqlite3_mprintf
		    ("SELECT table_name, column_name FROM gpkg_geometry_columns");
	    }
	  else
	    {
		sql_statement =
		    sqlite3_mprintf
		    ("SELECT f_table_name, f_geometry_column FROM geometry_columns");
	    }
      }
    else if (column == NULL)
      {
	  /* processing any geometry belonging to this table */
	  if (metadata_version == 4)
	    {
		/* GeoPackage Vector only */
		sql_statement =
		    sqlite3_mprintf
		    ("SELECT table_name, column_name FROM gpkg_geometry_columns WHERE (lower(table_name) = lower('%s'))",
		     table);
	    }
	  else
	    {
		sql_statement =
		    sqlite3_mprintf ("SELECT f_table_name, f_geometry_column "
				     "FROM geometry_columns "
				     "WHERE Lower(f_table_name) = Lower(%Q)",
				     table);
	    }
      }
    else
      {
	  /* processing a single table/geometry entry */
	  if (metadata_version == 4)
	    {
		/* GeoPackage Vector only */
		sql_statement =
		    sqlite3_mprintf
		    ("SELECT table_name, column_name FROM gpkg_geometry_columns WHERE ((lower(table_name) = lower('%s')) AND (Lower(column_name) = lower('%s')))",
		     table, column);
	    }
	  else
	    {
		sql_statement =
		    sqlite3_mprintf ("SELECT f_table_name, f_geometry_column "
				     "FROM geometry_columns "
				     "WHERE Lower(f_table_name) = Lower(%Q) "
				     "AND Lower(f_geometry_column) = Lower(%Q)",
				     table, column);
	    }
      }
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		f_table_name = results[(i * columns) + 0];
		f_geometry_column = results[(i * columns) + 1];
		if (!do_compute_layer_statistics
		    (sqlite, f_table_name, f_geometry_column,
		     SPATIALITE_STATISTICS_GENUINE))
		  {
		      error = 1;
		      break;
		  }
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;
    return 1;
}

static int
views_layer_statistics (sqlite3 * sqlite, const char *table, const char *column)
{
/* updating VIEWS_LAYER_STATISTICS metadata */
    char *sql_statement;
    int ret;
    const char *view_name;
    const char *view_geometry;
    int i;
    char **results;
    int rows;
    int columns;
    int error = 0;

    if (table == NULL && column == NULL)
      {
	  /* processing any table/geometry found in VIEWS_GEOMETRY_COLUMNS */
	  sql_statement = sqlite3_mprintf ("SELECT view_name, view_geometry "
					   "FROM views_geometry_columns");
      }
    else if (column == NULL)
      {
	  /* processing any geometry belonging to this table */
	  sql_statement = sqlite3_mprintf ("SELECT view_name, view_geometry "
					   "FROM views_geometry_columns "
					   "WHERE Lower(view_name) = Lower(%Q)",
					   table);
      }
    else
      {
	  /* processing a single table/geometry entry */
	  sql_statement = sqlite3_mprintf ("SELECT view_name, view_geometry "
					   "FROM views_geometry_columns "
					   "WHERE Lower(view_name) = Lower(%Q) "
					   "AND Lower(view_geometry) = Lower(%Q)",
					   table, column);
      }
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		view_name = results[(i * columns) + 0];
		view_geometry = results[(i * columns) + 1];
		if (!do_compute_layer_statistics
		    (sqlite, view_name, view_geometry,
		     SPATIALITE_STATISTICS_VIEWS))
		  {
		      error = 1;
		      break;
		  }
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;
    return 1;
}

static int
virts_layer_statistics (sqlite3 * sqlite, const char *table, const char *column)
{
/* updating VIRTS_LAYER_STATISTICS metadata */
    char *sql_statement;
    int ret;
    const char *f_table_name;
    const char *f_geometry_column;
    int i;
    char **results;
    int rows;
    int columns;
    int error = 0;

    if (table == NULL && column == NULL)
      {
	  /* processing any table/geometry found in VIRTS_GEOMETRY_COLUMNS */
	  sql_statement = sqlite3_mprintf ("SELECT virt_name, virt_geometry "
					   "FROM virts_geometry_columns");
      }
    else if (column == NULL)
      {
	  /* processing any geometry belonging to this table */
	  sql_statement = sqlite3_mprintf ("SELECT virt_name, virt_geometry "
					   "FROM virts_geometry_columns "
					   "WHERE Lower(virt_name) = Lower(%Q)",
					   table);
      }
    else
      {
	  /* processing a single table/geometry entry */
	  sql_statement = sqlite3_mprintf ("SELECT virt_name, virt_geometry "
					   "FROM virts_geometry_columns "
					   "WHERE Lower(virt_name) = Lower(%Q) "
					   "AND Lower(virt_geometry) = Lower(%Q)",
					   table, column);
      }
    ret =
	sqlite3_get_table (sqlite, sql_statement, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		f_table_name = results[(i * columns) + 0];
		f_geometry_column = results[(i * columns) + 1];
		if (!do_compute_layer_statistics
		    (sqlite, f_table_name, f_geometry_column,
		     SPATIALITE_STATISTICS_VIRTS))
		  {
		      error = 1;
		      break;
		  }
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;
    return 1;
}

static int
has_views_metadata (sqlite3 * sqlite)
{
/* testing if the VIEWS_GEOMETRY_COLUMNS table exists */
    char **results;
    int rows;
    int columns;
    int ret;
    int i;
    int defined = 0;
    ret =
	sqlite3_get_table (sqlite, "PRAGMA table_info(views_geometry_columns)",
			   &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	      defined = 1;
      }
    sqlite3_free_table (results);
    return defined;
}

static int
has_virts_metadata (sqlite3 * sqlite)
{
/* testing if the VIRTS_GEOMETRY_COLUMNS table exists */
    char **results;
    int rows;
    int columns;
    int ret;
    int i;
    int defined = 0;
    ret =
	sqlite3_get_table (sqlite, "PRAGMA table_info(virts_geometry_columns)",
			   &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	      defined = 1;
      }
    sqlite3_free_table (results);
    return defined;
}

SPATIALITE_DECLARE int
update_layer_statistics (sqlite3 * sqlite, const char *table,
			 const char *column)
{
/* updating LAYER_STATISTICS metadata [main] */
    if (!genuine_layer_statistics (sqlite, table, column))
	return 0;
    if (has_views_metadata (sqlite))
      {
	  if (!views_layer_statistics (sqlite, table, column))
	      return 0;
      }
    if (has_virts_metadata (sqlite))
      {
	  if (!virts_layer_statistics (sqlite, table, column))
	      return 0;
      }
    return 1;
}

struct table_params
{
/* a struct supporting Drop Table / Rename Table / Rename Column */
    char **rtrees;
    int n_rtrees;
    int is_view;
    int ok_geometry_columns;
    int ok_geometry_columns_time;
    int ok_views_geometry_columns;
    int ok_virts_geometry_columns;
    int ok_geometry_columns_auth;
    int ok_geometry_columns_field_infos;
    int ok_geometry_columns_statistics;
    int ok_views_geometry_columns_auth;
    int ok_views_geometry_columns_field_infos;
    int ok_views_geometry_columns_statistics;
    int ok_virts_geometry_columns_auth;
    int ok_virts_geometry_columns_field_infos;
    int ok_virts_geometry_columns_statistics;
    int ok_layer_statistics;
    int ok_views_layer_statistics;
    int ok_virts_layer_statistics;
    int ok_layer_params;
    int ok_layer_sub_classes;
    int ok_layer_table_layout;
    int ok_vector_coverages;
    int ok_vector_coverages_keyword;
    int ok_vector_coverages_srid;
    int ok_se_vector_styled_layers;
    int ok_se_raster_styled_layers;
    int metadata_version;
    int ok_gpkg_geometry_columns;
    int ok_gpkg_contents;
    int ok_gpkg_extensions;
    int ok_gpkg_tile_matrix;
    int ok_gpkg_tile_matrix_set;
    int ok_gpkg_ogr_contents;
    int ok_gpkg_metadata_reference;
    int gpkg_table_type;
    int ok_table_exists;
    int is_geometry_column;
    int count_geometry_columns;
    int has_topologies;
    int has_raster_coverages;
    int is_raster_coverage_entry;
    int table_type;
    int command_type;
    char *error_message;
};

/*/ -- -- ---------------------------------- --
// do_check_existing
// -- -- ---------------------------------- --
// table_type:
// - 0 [default]: table and views
// - 1: table only
// - 2: view only
// - 3: table,view,index and trigger [and anything else that may exist]
// -- -- ---------------------------------- --
// Author: Mark Johnson <mj10777@googlemail.com> 2019-01-12
// -- -- ---------------------------------- --
*/
static int
do_check_existing (sqlite3 * sqlite, const char *prefix, const char *table,
		   int table_type)
{
/* checking for an existing Table or View */
    char *sql;
    char *q_prefix;
    int count = 0;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;

    q_prefix = gaiaDoubleQuotedSql (prefix);
    switch (table_type)
      {
      case 1:
	  sql = sqlite3_mprintf ("SELECT Count(*) FROM \"%s\".sqlite_master "
				 "WHERE Upper(name) = Upper(%Q) AND type = 'table'",
				 q_prefix, table);
	  break;
      case 2:
	  sql = sqlite3_mprintf ("SELECT Count(*) FROM \"%s\".sqlite_master "
				 "WHERE Upper(name) = Upper(%Q) AND type = 'view'",
				 q_prefix, table);
	  break;
      case 3:
	  sql = sqlite3_mprintf ("SELECT Count(*) FROM \"%s\".sqlite_master "
				 "WHERE Upper(name) = Upper(%Q)", q_prefix,
				 table);
	  break;
      case 0:
      default:
	  sql = sqlite3_mprintf ("SELECT Count(*) FROM \"%s\".sqlite_master "
				 "WHERE Upper(name) = Upper(%Q) AND type IN ('table', 'view')",
				 q_prefix, table);
	  break;
      }
    free (q_prefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	      count = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);
  unknown:
    return count;
}

static int
do_check_existing_column (sqlite3 * sqlite, const char *prefix,
			  const char *table, const char *column)
{
/* checking for an existing Table Column */
    char *sql;
    char *q_prefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int exists = 0;

    q_prefix = gaiaDoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(%Q)", q_prefix, table);
    free (q_prefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *col = results[(i * columns) + 1];
		if (strcasecmp (column, col) == 0)
		    exists = 1;
	    }
      }
    sqlite3_free_table (results);
  unknown:
    return exists;
}

static char *
do_retrieve_coverage_name (sqlite3 * sqlite, const char *prefix,
			   const char *table, int table_only)
{
/* checking for a registered Vector Coverage */
    char *sql;
    char *q_prefix;
    char *coverage_name = NULL;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;

    q_prefix = gaiaDoubleQuotedSql (prefix);
    if (table_only)
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT coverage_name FROM \"%s\".vector_coverages "
	       "WHERE f_table_name = %Q", q_prefix, table);
      }
    else
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT coverage_name FROM \"%s\".vector_coverages "
	       "WHERE f_table_name = %Q OR view_name = %Q OR virt_name = %Q",
	       q_prefix, table, table, table);
      }
    free (q_prefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto unknown;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *cvg = results[(i * columns) + 0];
		if (cvg != NULL)
		  {
		      int len = strlen (cvg);
		      if (coverage_name != NULL)
			  free (coverage_name);
		      coverage_name = malloc (len + 1);
		      strcpy (coverage_name, cvg);
		  }
	    }
      }
    sqlite3_free_table (results);
  unknown:
    return coverage_name;
}

/*/ -- -- ---------------------------------- --
// do_drop_raster_triggers_index
// -- -- ---------------------------------- --
// Drop any TRIGGERs, INDEXes belonging to a raster_coverage Admin TABLE
// * geometry entry and its SpatialIndex
// * DROP the raster_coverage Admin TABLE when compleated
// -- -- ---------------------------------- --
// Author: Mark Johnson <mj10777@googlemail.com> 2019-01-12
// -- -- ---------------------------------- --
*/
static int
do_drop_raster_triggers_index (sqlite3 * sqlite, const char *prefix,
			       const char *tbl_name, int contains_geometry,
			       struct table_params *aux)
{
    if ((aux) && (aux->is_raster_coverage_entry == 1))
      {
	  /* the table name is a verified raster_coverage entry */
	  int ret;
	  char *errMsg = NULL;
	  if (prefix == NULL)
	      prefix = "main";
	  char *xprefix = gaiaDoubleQuotedSql (prefix);
	  char **results = NULL;
	  int rows = 0;
	  int columns = 0;
	  int i = 0;
	  char *sql_statement =
	      sqlite3_mprintf
	      ("SELECT type,name FROM \"%s\".sqlite_master WHERE ((type IN ('trigger','index')) AND (lower(tbl_name) = lower(%Q)))",
	       xprefix, tbl_name);
	  ret =
	      sqlite3_get_table (sqlite, sql_statement, &results, &rows,
				 &columns, NULL);
	  sqlite3_free (sql_statement);
	  if ((ret == SQLITE_OK) && (rows > 0) && (results))
	    {
		/* Use sqlite3_get_table so that 'sqlite_master' will not be locked during the 'DROP TRIGGER/INDEX' commands */
		for (i = 1; i <= rows; i++)
		  {
		      const char *type = results[(i * columns) + 0];
		      const char *type_name = results[(i * columns) + 1];
		      char *xtype_name = gaiaDoubleQuotedSql (type_name);
		      if (strcmp (type, "trigger") == 0)
			{
			    /* DROP all TRIGGERs belonging to this TABLE */
			    sql_statement =
				sqlite3_mprintf ("DROP TRIGGER \"%s\".\"%s\"",
						 xprefix, type_name);
			}
		      else
			{
			    /* DROP all INDEXes belonging to this TABLE */
			    sql_statement =
				sqlite3_mprintf ("DROP INDEX \"%s\".\"%s\"",
						 xprefix, type_name);
			}
		      free (xtype_name);
		      xtype_name = NULL;
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      sql_statement = NULL;
		      if (ret != SQLITE_OK)
			{
			    if (strcmp (type, "trigger") == 0)
			      {
				  aux->error_message =
				      sqlite3_mprintf
				      ("DROP of TRIGGER [%s] failed with rc=%d reason: %s",
				       type_name, ret, errMsg);
			      }
			    else
			      {
				  aux->error_message =
				      sqlite3_mprintf
				      ("DROP of INDEX [%s] failed with rc=%d reason: %s",
				       type_name, ret, errMsg);
			      }
			    sqlite3_free (errMsg);
			    errMsg = NULL;
			    free (xprefix);
			    xprefix = NULL;
			    sqlite3_free_table (results);
			    results = NULL;
			    return 0;
			}
		  }
		/* free the used results in case of further errors */
		sqlite3_free_table (results);
		results = NULL;
		if (contains_geometry)
		  {
		      char jolly = '%';
		      sql_statement =
			  sqlite3_mprintf
			  ("SELECT name FROM \"%s\".sqlite_master WHERE type = 'table' AND "
			   "Lower(name) IN (SELECT "
			   "Lower('idx_' || f_table_name || '_' || f_geometry_column) "
			   "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q)) "
			   "AND sql LIKE('%cvirtual%c') AND sql LIKE('%crtree%c')",
			   xprefix, xprefix, tbl_name, jolly, jolly, jolly,
			   jolly);
		      ret =
			  sqlite3_get_table (sqlite, sql_statement, &results,
					     &rows, &columns, NULL);
		      sqlite3_free (sql_statement);
		      sql_statement = NULL;
		      if ((ret == SQLITE_OK) && (rows > 0) && (results))
			{
			    /* Use sqlite3_get_table so that 'sqlite_master' will not be locked during the 'DROP TRIGGER/INDEX' commands */
			    for (i = 1; i <= rows; i++)
			      {
				  const char *rtree_name =
				      results[(i * columns) + 0];
				  char *q_rtree =
				      gaiaDoubleQuotedSql (rtree_name);
				  sql_statement =
				      sqlite3_mprintf
				      ("DROP TABLE \"%s\".\"%s\"", xprefix,
				       q_rtree);
				  free (q_rtree);
				  q_rtree = NULL;
				  ret =
				      sqlite3_exec (sqlite, sql_statement, NULL,
						    NULL, &errMsg);
				  sqlite3_free (sql_statement);
				  sql_statement = NULL;
				  if (ret != SQLITE_OK)
				    {
					aux->error_message =
					    sqlite3_mprintf
					    ("DROP of SpatialIndex TABLE [%s] failed with rc=%d reason: %s",
					     rtree_name, ret, errMsg);
					sqlite3_free_table (results);
					results = NULL;
					free (xprefix);
					xprefix = NULL;
					return 0;
				    }
			      }
			}
		      sqlite3_free_table (results);
		      results = NULL;
		      /*/
		         // ON DELETE CASCADE will remove the cousins:
		         // - geometry_columns_statistics, geometry_columns_field_infos, geometry_columns_time and geometry_columns_auth
		       */
		      sql_statement =
			  sqlite3_mprintf
			  ("DELETE FROM \"%s\".geometry_columns WHERE lower(f_table_name) = lower(%Q)",
			   xprefix, tbl_name);
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      sql_statement = NULL;
		      if (ret != SQLITE_OK)
			{
			    aux->error_message =
				sqlite3_mprintf
				("DELETE of  geometry_columns entry for [%s] failed with rc=%d reason: %s",
				 tbl_name, ret, errMsg);
			    sqlite3_free (errMsg);
			    errMsg = NULL;
			    free (xprefix);
			    xprefix = NULL;
			    return 0;
			}
		  }
	    }
	  if (results)
	    {
		/*  makes valgrind happy when placed here */
		sqlite3_free_table (results);
		results = NULL;
	    }
	  /* DROP the TABLE after all of the TRIGGERs and INDEXes have been removed */
	  sql_statement =
	      sqlite3_mprintf ("DROP TABLE \"%s\".\"%s\"", xprefix, tbl_name);
	  ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
	  sqlite3_free (sql_statement);
	  sql_statement = NULL;
	  if (ret != SQLITE_OK)
	    {
		aux->error_message =
		    sqlite3_mprintf
		    ("DROP of TABLE [%s] failed with rc=%d reason: %s",
		     tbl_name, ret, errMsg);
		sqlite3_free (errMsg);
		errMsg = NULL;
		free (xprefix);
		xprefix = NULL;
		return 0;
	    }
	  free (xprefix);
	  xprefix = NULL;
	  if (errMsg)
	    {
		sqlite3_free (errMsg);
		errMsg = NULL;
	    }
	  return 1;
      }
    return 0;
}

/*/ -- -- ---------------------------------- --
// do_drop_raster_coverage
// * check_for_system_tables will allow only when within the 'main' Database
// -- -- ---------------------------------- --
// Admin Tables for each raster_coverage entry:
// * 'raster_coverage'_levels
// --> FK reference: none
// --> TRIGGERS and index: 
// * 'raster_coverage'_sections
// --> FK reference: none
// --> TRIGGERs and index: tbl_name='raster_coverage'_sections
// * 'raster_coverage'_tiles
// --> FK reference to levels and sections
// --> TRIGGERs and index: tbl_name='raster_coverage'_tiles
// * 'raster_coverage'_tile_data
// --> FK reference to tiles
// --> TRIGGERs : tbl_name='raster_coverage'_tile_data
// -- -- ---------------------------------- --
// Delete any registred raster-styles
// -- -- ---------------------------------- --
// Author: Mark Johnson <mj10777@googlemail.com> 2019-01-12
// -- -- ---------------------------------- --
*/
static int
do_drop_raster_coverage (sqlite3 * sqlite, const char *prefix,
			 const char *table, struct table_params *aux)
{
    if ((aux) && (aux->is_raster_coverage_entry == 1))
      {
	  /*
	     // the table name is a verified raster_coverage entry
	     // the proper order is important
	   */
	  char *tbl_name = sqlite3_mprintf ("%s_tile_data", table);	/* no geometry column */
	  if (!do_drop_raster_triggers_index (sqlite, prefix, tbl_name, 0, aux))
	    {
		/* aux->error_message will contain any error message */
		sqlite3_free (tbl_name);
		tbl_name = NULL;
		return 0;
	    }
	  sqlite3_free (tbl_name);
	  tbl_name = NULL;
	  tbl_name = sqlite3_mprintf ("%s_tiles", table);	/* with geometry column */
	  if (!do_drop_raster_triggers_index (sqlite, prefix, tbl_name, 1, aux))
	    {
		/* aux->error_message will contain any error message */
		sqlite3_free (tbl_name);
		tbl_name = NULL;
		return 0;
	    }
	  sqlite3_free (tbl_name);
	  tbl_name = NULL;
	  tbl_name = sqlite3_mprintf ("%s_sections", table);	/* with geometry column */
	  if (!do_drop_raster_triggers_index (sqlite, prefix, tbl_name, 1, aux))
	    {
		/* aux->error_message will coniain any error message */
		sqlite3_free (tbl_name);
		tbl_name = NULL;
		return 0;
	    }
	  sqlite3_free (tbl_name);
	  tbl_name = NULL;
	  tbl_name = sqlite3_mprintf ("%s_levels", table);	/* no geometry column */
	  if (!do_drop_raster_triggers_index (sqlite, prefix, tbl_name, 0, aux))
	    {
		/* aux->error_message will contain any error message */
		sqlite3_free (tbl_name);
		tbl_name = NULL;
		return 0;
	    }
	  sqlite3_free (tbl_name);
	  tbl_name = NULL;
	  if (prefix == NULL)
	      prefix = "main";
	  char *errMsg = NULL;
	  char *xprefix = gaiaDoubleQuotedSql (prefix);
	  /*
	     // ON DELETE CASCADE will remove the cousins:
	     // - raster_coverages_keyword and raster_coverages_srid
	   */
	  char *sql_statement =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".raster_coverages WHERE lower(coverage_name) = lower(%Q)",
	       xprefix, table);
	  int ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
	  sqlite3_free (sql_statement);
	  sql_statement = NULL;
	  if (ret != SQLITE_OK)
	    {
		free (xprefix);
		xprefix = NULL;
		aux->error_message =
		    sqlite3_mprintf
		    ("DELETE of  raster_coverages entry for [%s] failed with rc=%d reason: %s",
		     table, ret, errMsg);
		sqlite3_free (errMsg);
		errMsg = NULL;
		return 0;
	    }
	  if (aux->ok_se_raster_styled_layers)
	    {
		/* - delete any registred raster-styles entries [no error if none exist] */
		sql_statement =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".SE_raster_styled_layers WHERE lower(coverage_name) = lower(%Q)",
		     xprefix, table);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		sql_statement = NULL;
		if (ret != SQLITE_OK)
		  {
		      free (xprefix);
		      xprefix = NULL;
		      aux->error_message =
			  sqlite3_mprintf
			  ("DELETE of  SE_raster_styled_layers entry for [%s] failed with rc=%d reason: %s",
			   table, ret, errMsg);
		      sqlite3_free (errMsg);
		      errMsg = NULL;
		      return 0;
		  }
	    }
	  if (xprefix)
	    {
		free (xprefix);
		xprefix = NULL;
	    }
	  return 1;
      }
    return 0;
}

/*/ -- -- ---------------------------------- --
// do_rename_raster_triggers_index
// -- -- ---------------------------------- --
// Drop any TRIGGERs, INDEXes belonging to a raster_coverage Admin TABLE
// * geometry entry and its SpatialIndex
// * DROP the raster_coverage Admin TABLE when compleated
// -- -- ---------------------------------- --
// Author: Mark Johnson <mj10777@googlemail.com> 2019-01-12
// -- -- ---------------------------------- --
*/
static int
do_rename_raster_triggers_index (sqlite3 * sqlite, const char *prefix,
				 const char *tbl_name, const char *tbl_new,
				 const char *name_old, const char *name_new,
				 int contains_geometry,
				 struct table_params *aux)
{
    if ((aux) && (aux->is_raster_coverage_entry == 1))
      {
	  /* the table name is a verified raster_coverage entry */
	  int ret;
	  char *errMsg = NULL;
	  if (prefix == NULL)
	      prefix = "main";
	  char *xprefix = gaiaDoubleQuotedSql (prefix);
	  char **results = NULL;
	  int rows = 0;
	  int columns = 0;
	  int i = 0;
	  char *q_old;
	  char *q_new;
	  q_old = gaiaDoubleQuotedSql (tbl_name);
	  q_new = gaiaDoubleQuotedSql (tbl_new);
	  /* ALTER TABLE "main"."middle_earth_1418sr_sections" RENAME TO "replace('middle_earth_1418sr_sections','middle_earth_1418sr','center_earth_1418sr')"" */
	  char *sql_statement =
	      sqlite3_mprintf ("ALTER TABLE \"%s\".\"%s\" RENAME TO \"%s\"",
			       xprefix, q_old, q_new);
	  free (q_old);
	  q_old = NULL;
	  free (q_new);
	  q_new = NULL;
	  ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
	  sqlite3_free (sql_statement);
	  sql_statement = NULL;
	  if (ret != SQLITE_OK)
	    {
		aux->error_message =
		    sqlite3_mprintf
		    ("RENAME TABLE from [%s] to [%s] failed with rc=%d reason: %s",
		     tbl_name, tbl_new, ret, errMsg);
		sqlite3_free (errMsg);
		errMsg = NULL;
		free (xprefix);
		xprefix = NULL;
		return 0;
	    }
	  /* Note: having been renamed, the 'tbl_name' in sqlite_master now contains the new name */
	  if (contains_geometry)
	    {
		/* Must be compleated first to avoid TRIGGER errors [geometry_columns still contains the old name] */
		char jolly = '%';
		sql_statement =
		    sqlite3_mprintf
		    ("SELECT name, replace(name,%Q,%Q) AS name_new FROM \"%s\".sqlite_master WHERE type = 'table' AND "
		     "Lower(name) IN (SELECT "
		     "Lower('idx_' || f_table_name || '_' || f_geometry_column) "
		     "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q)) "
		     "AND sql LIKE('%cvirtual%c') AND sql LIKE('%crtree%c')",
		     name_old, name_new, xprefix, xprefix, tbl_name, jolly,
		     jolly, jolly, jolly);
		ret =
		    sqlite3_get_table (sqlite, sql_statement, &results, &rows,
				       &columns, NULL);
		sqlite3_free (sql_statement);
		sql_statement = NULL;
		if ((ret == SQLITE_OK) && (rows > 0) && (results))
		  {
		      /* Use sqlite3_get_table so that 'sqlite_master' will not be locked during the 'DROP TRIGGER/INDEX' commands */
		      for (i = 1; i <= rows; i++)
			{
			    const char *rtree_name = results[(i * columns) + 0];
			    const char *rtree_name_new =
				results[(i * columns) + 1];
			    char *q_rtree = gaiaDoubleQuotedSql (rtree_name);
			    char *q_rtree_new =
				gaiaDoubleQuotedSql (rtree_name_new);
			    sql_statement =
				sqlite3_mprintf
				("ALTER TABLE \"%s\".\"%s\" RENAME TO \"%s\"",
				 xprefix, q_rtree, q_rtree_new);
			    free (q_rtree);
			    q_rtree = NULL;
			    free (q_rtree_new);
			    q_rtree_new = NULL;
			    ret =
				sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					      &errMsg);
			    sqlite3_free (sql_statement);
			    sql_statement = NULL;
			    if (ret != SQLITE_OK)
			      {
				  aux->error_message =
				      sqlite3_mprintf
				      ("ALTER of SpatialIndex TABLE from [%s] to [%s] failed with rc=%d reason: %s",
				       rtree_name, rtree_name_new, ret, errMsg);
				  sqlite3_free_table (results);
				  results = NULL;
				  sqlite3_free (errMsg);
				  errMsg = NULL;
				  free (xprefix);
				  xprefix = NULL;
				  return 0;
			      }
			}
		  }
		sqlite3_free_table (results);
		results = NULL;
		/*
		   // Rename the entries in the cousins:
		   // - geometry_columns_statistics, geometry_columns_field_infos, geometry_columns_time and geometry_columns_auth
		 */
		sql_statement =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns SET f_table_name =  lower(%Q) WHERE lower(f_table_name) = lower(%Q);"
		     "UPDATE \"%s\".geometry_columns_auth SET f_table_name =  lower(%Q) WHERE lower(f_table_name) = lower(%Q);"
		     "UPDATE \"%s\".geometry_columns_time SET f_table_name =  lower(%Q) WHERE lower(f_table_name) = lower(%Q);"
		     "UPDATE \"%s\".geometry_columns_field_infos SET f_table_name =  lower(%Q) WHERE lower(f_table_name) = lower(%Q);"
		     "UPDATE \"%s\".geometry_columns_statistics SET f_table_name =  lower(%Q) WHERE lower(f_table_name) = lower(%Q);",
		     xprefix, tbl_new, tbl_name, xprefix, tbl_new, tbl_name,
		     xprefix, tbl_new, tbl_name, xprefix, tbl_new, tbl_name,
		     xprefix, tbl_new, tbl_name);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		sql_statement = NULL;
		if (ret != SQLITE_OK)
		  {
		      aux->error_message =
			  sqlite3_mprintf
			  ("UPDATE of  geometry_columns entry from [%s] to  [%s] failed with rc=%d reason: %s",
			   tbl_name, tbl_new, ret, errMsg);
		      sqlite3_free (errMsg);
		      errMsg = NULL;
		      free (xprefix);
		      xprefix = NULL;
		      return 0;
		  }
	    }
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT type,name,replace(name,%Q,%Q) AS name_new, replace(sql,%Q,%Q) AS sql_new FROM \"%s\".sqlite_master WHERE ((type IN ('trigger','index')) AND (lower(tbl_name) = lower(%Q)))",
	       name_old, name_new, name_old, name_new, xprefix, tbl_new);
	  ret =
	      sqlite3_get_table (sqlite, sql_statement, &results, &rows,
				 &columns, NULL);
	  sqlite3_free (sql_statement);
	  if ((ret == SQLITE_OK) && (rows > 0) && (results))
	    {
		/* Use sqlite3_get_table so that 'sqlite_master' will not be locked during the 'DROP TRIGGER/INDEX' commands */
		for (i = 1; i <= rows; i++)
		  {
		      const char *type = results[(i * columns) + 0];
		      const char *type_name = results[(i * columns) + 1];
		      const char *type_name_new = results[(i * columns) + 2];
		      const char *type_sql = results[(i * columns) + 3];
		      char *xtype_name = gaiaDoubleQuotedSql (type_name);
		      if (strcmp (type, "trigger") == 0)
			{
			    /* DROP all TRIGGERs belonging to this TABLE */
			    sql_statement =
				sqlite3_mprintf ("DROP TRIGGER \"%s\".\"%s\"",
						 xprefix, xtype_name);
			}
		      else
			{
			    /*
			       // DROP all INDEXes belonging to this TABLE
			       // DROP INDEX idx_middle_earth_1418sr_sect_name ; CREATE INDEX "idx_center_earth_1418sr_sect_name" ON "center_earth_1418sr_sections" (section_name)
			     */
			    sql_statement =
				sqlite3_mprintf ("DROP INDEX \"%s\".\"%s\"",
						 xprefix, xtype_name);
			}
		      free (xtype_name);
		      xtype_name = NULL;
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      sql_statement = NULL;
		      if (ret != SQLITE_OK)
			{
			    if (strcmp (type, "trigger") == 0)
			      {
				  aux->error_message =
				      sqlite3_mprintf
				      ("DROP of TRIGGER [%s] failed with rc=%d reason: %s",
				       type_name, ret, errMsg);
			      }
			    else
			      {
				  aux->error_message =
				      sqlite3_mprintf
				      ("DROP of INDEX [%s] failed with rc=%d reason: %s",
				       type_name, ret, errMsg);
			      }
			    sqlite3_free (errMsg);
			    errMsg = NULL;
			    free (xprefix);
			    xprefix = NULL;
			    sqlite3_free_table (results);
			    results = NULL;
			    return 0;
			}
		      sql_statement = sqlite3_mprintf ("%s", type_sql);
		      ret =
			  sqlite3_exec (sqlite, sql_statement, NULL, NULL,
					&errMsg);
		      sqlite3_free (sql_statement);
		      sql_statement = NULL;
		      if (ret != SQLITE_OK)
			{
			    if (strcmp (type, "trigger") == 0)
			      {
				  aux->error_message =
				      sqlite3_mprintf
				      ("CREATE of TRIGGER [%s] failed with rc=%d reason: %s",
				       type_name_new, ret, errMsg);
			      }
			    else
			      {
				  aux->error_message =
				      sqlite3_mprintf
				      ("CREATE of INDEX [%s] failed with rc=%d reason: %s",
				       type_name_new, ret, errMsg);
			      }
			    sqlite3_free (errMsg);
			    errMsg = NULL;
			    free (xprefix);
			    xprefix = NULL;
			    sqlite3_free_table (results);
			    results = NULL;
			    return 0;
			}
		  }
	    }
	  if (results)
	    {			/* makes valgrind happy when placed here */
		sqlite3_free_table (results);
		results = NULL;
	    }
	  if (xprefix)
	    {
		free (xprefix);
		xprefix = NULL;
	    }
      }
    return 1;
}

/*/ -- -- ---------------------------------- --
// do_rename_raster_coverage
// -- -- ---------------------------------- --
// Admin Tables for each raster_coverage entry:
// * 'raster_coverage'_levels
// --> FK reference: none
// --> TRIGGERS and index: 
// * 'raster_coverage'_sections
// --> FK reference: none
// --> TRIGGERs and index: tbl_name='raster_coverage'_sections
// * 'raster_coverage'_tiles
// --> FK reference to levels and sections
// --> TRIGGERs and index: tbl_name='raster_coverage'_tiles
// * 'raster_coverage'_tile_data
// --> FK reference to tiles
// --> TRIGGERs : tbl_name='raster_coverage'_tile_data
// -- -- ---------------------------------- --
// Update any registred raster-styles
// -- -- ---------------------------------- --
// Author: Mark Johnson <mj10777@googlemail.com> 2019-01-12
// -- -- ---------------------------------- --
*/
static int
do_rename_raster_coverage (sqlite3 * sqlite, const char *prefix,
			   const char *name_old, const char *name_new,
			   struct table_params *aux)
{
    if ((aux) && (aux->is_raster_coverage_entry == 1))
      {
	  /*            
	     // the old_name is a verified raster_coverage entry, the new_name does not exist in raster_coverage
	     // the proper order is important
	   */
	  char *tbl_name = sqlite3_mprintf ("%s_tiles", name_old);	/* with geometry column */
	  char *tbl_new = sqlite3_mprintf ("%s_tiles", name_new);	/* with geometry column */
	  if (!do_rename_raster_triggers_index
	      (sqlite, prefix, tbl_name, tbl_new, name_old, name_new, 1, aux))
	    {
		/* aux->error_message will contain any error message */
		sqlite3_free (tbl_name);
		tbl_name = NULL;
		sqlite3_free (tbl_new);
		tbl_new = NULL;
		return 0;
	    }
	  sqlite3_free (tbl_name);
	  tbl_name = NULL;
	  sqlite3_free (tbl_new);
	  tbl_new = NULL;
	  tbl_name = sqlite3_mprintf ("%s_tile_data", name_old);	/* no geometry column */
	  tbl_new = sqlite3_mprintf ("%s_tile_data", name_new);	/* no geometry column */
	  if (!do_rename_raster_triggers_index
	      (sqlite, prefix, tbl_name, tbl_new, name_old, name_new, 0, aux))
	    {			/* aux->error_message will contain any error message */
		sqlite3_free (tbl_name);
		tbl_name = NULL;
		sqlite3_free (tbl_new);
		tbl_new = NULL;
		return 0;
	    }
	  sqlite3_free (tbl_name);
	  tbl_name = NULL;
	  sqlite3_free (tbl_new);
	  tbl_new = NULL;
	  tbl_name = sqlite3_mprintf ("%s_sections", name_old);	/* with geometry column */
	  tbl_new = sqlite3_mprintf ("%s_sections", name_new);	/* with geometry column */
	  if (!do_rename_raster_triggers_index
	      (sqlite, prefix, tbl_name, tbl_new, name_old, name_new, 1, aux))
	    {
		/* aux->error_message will contain any error message */
		sqlite3_free (tbl_name);
		tbl_name = NULL;
		sqlite3_free (tbl_new);
		tbl_new = NULL;
		return 0;
	    }
	  sqlite3_free (tbl_name);
	  tbl_name = NULL;
	  sqlite3_free (tbl_new);
	  tbl_new = NULL;
	  tbl_name = sqlite3_mprintf ("%s_levels", name_old);	/* no geometry column */
	  tbl_new = sqlite3_mprintf ("%s_levels", name_new);	/* no geometry column */
	  if (!do_rename_raster_triggers_index
	      (sqlite, prefix, tbl_name, tbl_new, name_old, name_new, 0, aux))
	    {
		/* aux->error_message will contain any error message */
		sqlite3_free (tbl_name);
		tbl_name = NULL;
		sqlite3_free (tbl_new);
		tbl_new = NULL;
		return 0;
	    }
	  sqlite3_free (tbl_name);
	  tbl_name = NULL;
	  sqlite3_free (tbl_new);
	  tbl_new = NULL;
	  if (prefix == NULL)
	      prefix = "main";
	  char *errMsg = NULL;
	  char *xprefix = gaiaDoubleQuotedSql (prefix);
	  /*
	     // ON DELETE CASCADE will remove the cousins:
	     // - raster_coverages_keyword and raster_coverages_srid
	   */
	  char *sql_statement =
	      sqlite3_mprintf
	      ("UPDATE \"%s\".raster_coverages SET coverage_name =  lower(%Q) WHERE lower(coverage_name) = lower(%Q)",
	       xprefix, name_new, name_old);
	  int ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
	  sqlite3_free (sql_statement);
	  sql_statement = NULL;
	  if (ret != SQLITE_OK)
	    {
		free (xprefix);
		xprefix = NULL;
		aux->error_message =
		    sqlite3_mprintf
		    ("UPDATE of  raster_coverages entry from [%s] to [%s] failed with rc=%d reason: %s",
		     name_old, name_new, ret, errMsg);
		sqlite3_free (errMsg);
		errMsg = NULL;
		return 0;
	    }
	  if (aux->ok_se_raster_styled_layers)
	    {
		/* - update any registred raster-styles  [no error if none exist] */
		sql_statement =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".SE_raster_styled_layers SET coverage_name =  lower(%Q) WHERE lower(coverage_name) = lower(%Q)",
		     xprefix, name_new, name_old);
		ret = sqlite3_exec (sqlite, sql_statement, NULL, NULL, &errMsg);
		sqlite3_free (sql_statement);
		sql_statement = NULL;
		if (ret != SQLITE_OK)
		  {
		      free (xprefix);
		      xprefix = NULL;
		      aux->error_message =
			  sqlite3_mprintf
			  ("UPDATE of  SE_raster_styled_layers from [%s] to [%s] failed with rc=%d reason: %s",
			   name_old, name_new, ret, errMsg);
		      sqlite3_free (errMsg);
		      errMsg = NULL;
		      return 0;
		  }
	    }
	  if (xprefix)
	    {
		free (xprefix);
		xprefix = NULL;
	    }
	  return 1;
      }
    return 0;
}

static int
do_drop_table (sqlite3 * sqlite, const char *prefix, const char *table,
	       struct table_params *aux)
{
/* performing the actual work */
    char *sql;
    char *q_prefix;
    char *q_name;
    int i;
    int ret;
    char *errMsg = NULL;

    if (!do_check_existing (sqlite, prefix, table, 0))
	return 0;

    if (aux->ok_vector_coverages)
      {
	  /* deleting from VECTOR_COVERAGES */
	  char *coverage = do_retrieve_coverage_name (sqlite, prefix, table, 0);
	  if (coverage != NULL)
	    {
		q_prefix = gaiaDoubleQuotedSql (prefix);
		if (aux->ok_vector_coverages_srid)
		  {
		      /* deleting from VECTOR_COVERAGES_SRID  */
		      sql =
			  sqlite3_mprintf
			  ("DELETE FROM \"%s\".vector_coverages_srid "
			   "WHERE lower(coverage_name) = lower(%Q)", q_prefix,
			   coverage);
		      ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    aux->error_message = errMsg;
			    return 0;
			}
		  }
		if (aux->ok_vector_coverages_keyword)
		  {
		      /* deleting from VECTOR_COVERAGES_KEYWORD  */
		      sql =
			  sqlite3_mprintf
			  ("DELETE FROM \"%s\".vector_coverages_keyword "
			   "WHERE lower(coverage_name) = lower(%Q)", q_prefix,
			   coverage);
		      ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    aux->error_message = errMsg;
			    return 0;
			}
		  }
		if (aux->ok_se_vector_styled_layers)
		  {
		      /* deleting from SE_VECTOR_STYLED_LAYERS  */
		      sql =
			  sqlite3_mprintf
			  ("DELETE FROM \"%s\".SE_vector_styled_layers "
			   "WHERE lower(coverage_name) = lower(%Q)", q_prefix,
			   coverage);
		      ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    aux->error_message = errMsg;
			    return 0;
			}
		  }
		/* and finally deleting from VECTOR COVERAGES */
		sql = sqlite3_mprintf ("DELETE FROM \"%s\".vector_coverages "
				       "WHERE lower(coverage_name) = lower(%Q)",
				       q_prefix, coverage);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      aux->error_message = errMsg;
		      return 0;
		  }
		free (q_prefix);
		free (coverage);
	    }
      }

    if (aux->is_view)
      {
	  /* dropping a View */
	  q_name = gaiaDoubleQuotedSql (table);
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf ("DROP VIEW IF EXISTS \"%s\".\"%s\"", q_prefix,
			       q_name);
	  free (q_prefix);
	  free (q_name);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    else
      {
	  /* dropping a Table */
	  q_name = gaiaDoubleQuotedSql (table);
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\".\"%s\"", q_prefix,
			       q_name);
	  free (q_prefix);
	  free (q_name);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }

    for (i = 0; i < aux->n_rtrees; i++)
      {
	  /* dropping any R*Tree */
	  q_name = gaiaDoubleQuotedSql (*(aux->rtrees + i));
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\".\"%s\"", q_prefix,
			       q_name);
	  free (q_prefix);
	  free (q_name);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }

    if (aux->ok_layer_params)
      {
	  /* deleting from LAYER_PARAMS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".layer_params "
				 "WHERE lower(table_name) = lower(%Q)",
				 q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_layer_sub_classes)
      {
	  /* deleting from LAYER_SUB_CLASSES */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".layer_sub_classes "
				 "WHERE lower(table_name) = lower(%Q)",
				 q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_layer_table_layout)
      {
	  /* deleting from LAYER_TABLE_LAYOUT */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".layer_table_layout "
				 "WHERE lower(table_name) = lower(%Q)",
				 q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_geometry_columns_auth)
      {
	  /* deleting from GEOMETRY_COLUMNS_AUTH */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".geometry_columns_auth "
				 "WHERE lower(f_table_name) = lower(%Q)",
				 q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_geometry_columns_time)
      {
	  /* deleting from GEOMETRY_COLUMNS_TIME */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".geometry_columns_time "
				 "WHERE lower(f_table_name) = lower(%Q)",
				 q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_geometry_columns_field_infos)
      {
	  /* deleting from GEOMETRY_COLUMNS_FIELD_INFOS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".geometry_columns_field_infos "
	       "WHERE lower(f_table_name) = lower(%Q)", q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_geometry_columns_statistics)
      {
	  /* deleting from GEOMETRY_COLUMNS_STATISTICS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf ("DELETE FROM \"%s\".geometry_columns_statistics "
			       "WHERE lower(f_table_name) = lower(%Q)",
			       q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_views_geometry_columns_auth)
      {
	  /* deleting from VIEWS_GEOMETRY_COLUMNS_AUTH */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf ("DELETE FROM \"%s\".views_geometry_columns_auth "
			       "WHERE lower(view_name) = lower(%Q)", q_prefix,
			       table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_views_geometry_columns_field_infos)
      {
	  /* deleting from VIEWS_GEOMETRY_COLUMNS_FIELD_INFOS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".views_geometry_columns_field_infos "
	       "WHERE view_name = %Q", q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_views_geometry_columns_statistics)
      {
	  /* deleting from VIEWS_GEOMETRY_COLUMNS_STATISTICS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".views_geometry_columns_statistics "
	       "WHERE lower(view_name) = lower(%Q)", q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_virts_geometry_columns_auth)
      {
	  /* deleting from VIRTS_GEOMETRY_COLUMNS_AUTH */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf ("DELETE FROM \"%s\".virts_geometry_columns_auth "
			       "WHERE lower(virt_name) = lower(%Q)", q_prefix,
			       table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_virts_geometry_columns_field_infos)
      {
	  /* deleting from VIRTS_GEOMETRY_COLUMNS_FIELD_INFOS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".virts_geometry_columns_field_infos "
	       "WHERE lower(virt_name) = lower(%Q)", q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_virts_geometry_columns_statistics)
      {
	  /* deleting from VIRTS_GEOMETRY_COLUMNS_STATISTICS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".virts_geometry_columns_statistics "
	       "WHERE lower(virt_name) = lower(%Q)", q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_layer_statistics)
      {
	  /* deleting from LAYER_STATISTICS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".layer_statistics "
				 "WHERE lower(table_name) = lower(%Q)",
				 q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_views_layer_statistics)
      {
	  /* deleting from VIEWS_LAYER_STATISTICS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".views_layer_statistics "
				 "WHERE lower(view_name) = lower(%Q)", q_prefix,
				 table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_virts_layer_statistics)
      {
	  /* deleting from VIRTS_LAYER_STATISTICS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".virts_layer_statistics "
				 "WHERE lower(virt_name) = lower(%Q)", q_prefix,
				 table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_geometry_columns)
      {
	  /* deleting from GEOMETRY_COLUMNS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".geometry_columns "
				 "WHERE lower(f_table_name) = lower(%Q)",
				 q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_views_geometry_columns)
      {
	  /* deleting from VIEWS_GEOMETRY_COLUMNS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".views_geometry_columns "
				 "WHERE lower(view_name) = lower(%Q)", q_prefix,
				 table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_virts_geometry_columns)
      {
	  /* deleting from VIEWS_GEOMETRY_COLUMNS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql = sqlite3_mprintf ("DELETE FROM \"%s\".virts_geometry_columns "
				 "WHERE lower(virt_name) = lower(%Q)", q_prefix,
				 table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }

    return 1;
}

static int
doDropGeometryTriggers (sqlite3 * sqlite, const char *table, const char *geom,
			struct table_params *aux, char **error_message)
{
/* dropping Geometry Triggers */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    char *string;
    char *errMsg = NULL;
    if ((aux) && (aux->metadata_version == 2))
      {
	  return 1;		/* Fdo has no TRIGGERs */
      }

/* searching all Geometry Triggers to be dropped [for GeoPackage: wildcard before and after needed] */
    string = sqlite3_mprintf ("%%_%s_%s%%", table, geom);
    sql =
	sqlite3_mprintf
	("SELECT name FROM MAIN.sqlite_master WHERE name LIKE %Q "
	 "AND type = 'trigger'", string);
    sqlite3_free (string);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		/* found a Geometry Trigger */
		const char *trigger = results[(i * columns) + 0];
		char *q_trigger = gaiaDoubleQuotedSql (trigger);
		sql = sqlite3_mprintf ("DROP TRIGGER main.\"%s\"", q_trigger);
		free (q_trigger);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
      }
    sqlite3_free_table (results);
    return 1;
}

/*/ -- -- ---------------------------------- --
// doUpdateGeometryTriggers
// -- -- ---------------------------------- --
// Rebuild of TRIGGERs and SpatialIndex
// - based on Database Type
// - Spatialite: calls default updateGeometryTriggers
// - GeoPackage: calls 'gpkgAddGeometryTriggers' and 'gpkgAddSpatialIndex'
// -> building Index
// - Fdo: return 1 with no further activity
// -- -- ---------------------------------- --
// Author: Mark Johnson <mj10777@googlemail.com> 2019-01-12
// -- -- ---------------------------------- --
*/
static int
doUpdateGeometryTriggers (sqlite3 * sqlite, const char *table,
			  const char *column, struct table_params *aux,
			  char **error_message)
{
    char *sql = NULL;
    switch (aux->metadata_version)
      {
      case 1:
      case 3:
	  updateGeometryTriggers (sqlite, table, column);	/* returns void */
	  return 1;
	  break;
      case 2:
	  return 1;		/* Fdo has no TRIGGERs */
	  break;
      case 4:
	  if (aux->ok_gpkg_extensions)
	    {
		/* Note: fnct_gpkgAddGeometryTriggers does not check tha validaty of the arguments! */
		sql =
		    sqlite3_mprintf ("SELECT gpkgAddGeometryTriggers(%Q,%Q);",
				     table, column);
	    }
	  break;
      }
    if ((sql) && (aux->metadata_version == 4) && (aux->ok_gpkg_extensions))
      {
	  /* Only for valid GeoPackage Databases */
	  char *errMsg = NULL;
	  int ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		if (error_message != NULL)
		  {
		      *error_message =
			  sqlite3_mprintf
			  ("gpkgAddGeometryTriggers for [%s(%s)] failed with rc=%d reason: %s",
			   table, column, ret, errMsg);
		  }
		sqlite3_free (errMsg);
		errMsg = NULL;
		return 0;
	    }
	  /* Previous SpatialIndex entry in gpkg_extensions was removed [new entry will be added during gpkgAddSpatialIndex] */
	  sql =
	      sqlite3_mprintf ("SELECT gpkgAddSpatialIndex(%Q,%Q);", table,
			       column);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);	/* Usage of '&errMsg 'here causes a leak */
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		if (error_message != NULL)
		  {
		      *error_message =
			  sqlite3_mprintf
			  ("gpkgAddSpatialIndex for [%s(%s)] failed with rc=%d reason: %s",
			   table, column, ret, errMsg);
		  }
		sqlite3_free (errMsg);
		errMsg = NULL;
		return 0;
	    }
	  /* Rebuild SpatialIndex [old SpatialIndex was removed] */
	  sql =
	      sqlite3_mprintf
	      ("INSERT INTO \"rtree_%s_%s\" (id,minx,maxx,miny,maxy)  SELECT ROWID, ST_MinX(\"%s\"),ST_MaxX(\"%s\"), ST_MinY(\"%s\"),ST_MaxY(\"%s\") FROM %Q;",
	       table, column, column, column, column, column, table);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		if (error_message != NULL)
		  {
		      *error_message =
			  sqlite3_mprintf
			  ("filling rtree for [%s(%s)] failed with rc=%d reason: %s",
			   table, column, ret, errMsg);
		  }
		sqlite3_free (errMsg);
		errMsg = NULL;
		return 0;
	    }
	  if (errMsg)
	    {
		sqlite3_free (errMsg);
		errMsg = NULL;
	    }
      }
    return 1;
}

static int
do_drop_geotriggers (sqlite3 * sqlite, const char *table, const char *column,
		     struct table_params *aux, char **error_message)
{
/* dropping any previous Geometry Trigger */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    if ((aux) && (aux->metadata_version > 0)
	&& ((aux->count_geometry_columns > 0)
	    || (aux->is_geometry_column == 1)))
      {
	  /*            
	     // This is a spatialite, GeoPackage/FDO TABLE that contains a geometry-column [not a sqlite3 TABLE]
	     // searching all Geometry Triggers defined by the old Table 
	   */
	  switch (aux->metadata_version)
	    {
	    case 1:
	    case 3:
		if (column)
		    sql =
			sqlite3_mprintf
			("SELECT f_geometry_column FROM MAIN.geometry_columns WHERE Lower(f_table_name) = Lower(%Q) AND lower(f_geometry_column) = lower(%Q)",
			 table, column);
		else
		    sql =
			sqlite3_mprintf
			("SELECT f_geometry_column FROM MAIN.geometry_columns WHERE Lower(f_table_name) = Lower(%Q)",
			 table);
		break;
	    case 2:
		return 1;	/* Fdo has no TRIGGERs */
		break;
	    case 4:
		sql =
		    sqlite3_mprintf
		    ("SELECT column_name FROM MAIN.gpkg_geometry_columns WHERE Lower(table_name) = Lower(%Q)",
		     table);
		break;
	    }
	  ret =
	      sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (rows < 1)
	      ;
	  else
	    {
		for (i = 1; i <= rows; i++)
		  {
		      /* dropping Triggers for some Geometry Column */
		      const char *geom = results[(i * columns) + 0];
		      if (!doDropGeometryTriggers
			  (sqlite, table, geom, aux, error_message))
			{
			    sqlite3_free_table (results);
			    return 0;
			}
		  }
	    }
	  sqlite3_free_table (results);
      }
    return 1;
}

static int
do_rebuild_geotriggers (sqlite3 * sqlite, const char *table, const char *column,
			struct table_params *aux)
{
/* re-installing all Geometry Triggers */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    if ((aux) && (aux->metadata_version > 0)
	&& ((aux->count_geometry_columns > 0)
	    || (aux->is_geometry_column == 1)))
      {
	  /*            
	     // This is a spatialite, GeoPackage/FDO TABLE that contains a geometry-column [not a sqlite3 TABLE]
	     // searching all Geometry Triggers defined by the Table 
	   */
	  switch (aux->metadata_version)
	    {
	    case 1:
	    case 3:
		if (column)
		    sql =
			sqlite3_mprintf
			("SELECT f_geometry_column FROM MAIN.geometry_columns WHERE Lower(f_table_name) = Lower(%Q) AND lower(f_geometry_column) = lower(%Q)",
			 table, column);
		else
		    sql =
			sqlite3_mprintf
			("SELECT f_geometry_column FROM MAIN.geometry_columns WHERE Lower(f_table_name) = Lower(%Q)",
			 table);
		break;
	    case 2:
		return 1;	/* Fdo has no TRIGGERs */
		break;
	    case 4:
		sql =
		    sqlite3_mprintf
		    ("SELECT column_name FROM MAIN.gpkg_geometry_columns WHERE Lower(table_name) = Lower(%Q)",
		     table);
		break;
	    }
	  ret =
	      sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return 0;
	  if (rows < 1)
	      ;
	  else
	    {
		for (i = 1; i <= rows; i++)
		  {
		      /* found a Geometry Column, Rebuild of TRIGGERs and SpatialIndex based on Database Type */
		      const char *geom = results[(i * columns) + 0];
		      if (!doUpdateGeometryTriggers
			  (sqlite, table, geom, aux, &aux->error_message))
			{
			    sqlite3_free_table (results);
			    return 0;
			}
		  }
	    }
	  sqlite3_free_table (results);
      }
    return 1;
}

static int
do_rename_column_pre (sqlite3 * sqlite, const char *prefix, const char *table,
		      const char *old_name, const char *new_name,
		      struct table_params *aux, char **error_message)
{
/* renaming a Column - preparatory steps */
    char *sql;
    char *q_prefix;
    int ret;
    char *errMsg = NULL;
    if ((aux) && (aux->metadata_version > 0) && (aux->is_geometry_column == 1))
      {
	  /*                            
	     // Drop TRIGGERs only if a geometry-column is being renamed and it a spatialite geometry-column [not for GeoPackage or Fdo]
	     // dropping any previous Geometry Trigger  [spatialite only with at least 1 geometry-column ; returns true for others]  
	   */
	  if (!do_drop_geotriggers
	      (sqlite, table, old_name, aux, error_message))
	      return 0;
	  /* updating fist all metadata tables */
	  /* updating GEOMETRY_COLUMNS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  switch (aux->metadata_version)
	    {
	    case 1:
	    case 2:		/* Fdo uses same column/table-name */
	    case 3:
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns SET f_geometry_column = lower(%Q) WHERE lower(f_table_name) = lower(%Q) AND lower(f_geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		break;
	    case 4:
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".gpkg_geometry_columns SET column_name =  lower(%Q) WHERE Lower(table_name) = Lower(%Q)",
		     q_prefix, new_name, table);
		break;
	    }
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		if (error_message != NULL)
		  {
		      if (aux->metadata_version == 4)
			  *error_message =
			      sqlite3_mprintf
			      ("UPDATE of gpkg_geometry_columns for [%s(%s) from ] failed with rc=%d reason: %s",
			       table, new_name, old_name, ret, errMsg);
		      else
			  *error_message =
			      sqlite3_mprintf
			      ("UPDATE of geometry_columns for [%s(%s) from ] failed with rc=%d reason: %s",
			       table, new_name, old_name, ret, errMsg);
		  }
		sqlite3_free (errMsg);
		errMsg = NULL;
		return 0;
	    }
	  if (aux->ok_layer_params)
	    {
		/* updating LAYER_PARAMS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".layer_params SET geometry_column = lower(%Q) "
		     "WHERE lower(table_name) = lower(%Q) AND lower(geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);	/* mj10777: possibly empty */
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_sub_classes)
	    {
		/* updating LAYER_SUB_CLASSES */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".layer_sub_classes SET geometry_column = lower(%Q) "
		     "WHERE lower(table_name) = lower(%Q) AND lower(geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);	/* mj10777: possibly empty */
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_table_layout)
	    {
		/* updating LAYER_TABLE_LAYOUT */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".layer_table_layout SET geometry_column = lower(%Q) "
		     "WHERE lower(table_name) = lower(%Q) AND lower(geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_auth)
	    {
		/* updating GEOMETRY_COLUMNS_AUTH */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns_auth SET f_geometry_column = lower(%Q) "
		     "WHERE lower(f_table_name) = lower(%Q) AND lower(f_geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_time)
	    {
		/* updating GEOMETRY_COLUMNS_TIME */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns_time SET f_geometry_column = lower(%Q) "
		     "WHERE lower(f_table_name) = lower(%Q) AND lower(f_geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_field_infos)
	    {
		/* updating GEOMETRY_COLUMNS_FIELD_INFOS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns_field_infos SET f_geometry_column = lower(%Q) "
		     "WHERE lower(f_table_name) = lower(%Q) AND lower(f_geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_statistics)
	    {
		/* updating GEOMETRY_COLUMNS_STATISTICS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns_statistics SET f_geometry_column = lower(%Q) "
		     "WHERE lower(f_table_name) = lower(%Q) AND lower(f_geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_statistics)
	    {
		/* updating LAYER_STATISTICS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".layer_statistics SET geometry_column = lower(%Q) "
		     "WHERE lower(table_name) = lower(%Q) AND lower(geometry_column) = lower(%Q)",
		     q_prefix, new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_vector_coverages)
	    {
		/* updating VECTOR_COVERAGES */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".vector_coverages SET f_geometry_column = %Q "
		     "WHERE lower(f_table_name) = lower(%Q) AND lower(f_geometry_column) = lower(%Q)",
		     q_prefix, new_name, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_gpkg_extensions)
	    {
		/* remove entry from gpkg_extensions */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql = sqlite3_mprintf ("DELETE FROM \"%s\".gpkg_extensions "
				       "WHERE lower(table_name) = lower(%Q) AND lower(column_name) = lower(%Q)",
				       q_prefix, table, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
      }
    if (errMsg)
      {
	  sqlite3_free (errMsg);
	  errMsg = NULL;
      }
    return 1;
}

static int
do_rename_column_post (sqlite3 * sqlite, const char *prefix, const char *table,
		       const char *old_name, const char *new_name,
		       struct table_params *aux, char **error_message)
{
/* renaming a Column - final steps */
    char *sql;
    char *q_prefix;
    char *q_table;
    char *q_old;
    char *q_new;
    int ret;
    char *errMsg = NULL;

/* renaming the Column itself */
    q_prefix = gaiaDoubleQuotedSql (prefix);
    q_table = gaiaDoubleQuotedSql (table);
    q_old = gaiaDoubleQuotedSql (old_name);
    q_new = gaiaDoubleQuotedSql (new_name);
    sql =
	sqlite3_mprintf
	("ALTER TABLE \"%s\".\"%s\" RENAME COLUMN \"%s\" TO \"%s\"", q_prefix,
	 q_table, q_old, q_new);
    free (q_prefix);
    free (q_table);
    free (q_old);
    free (q_new);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  if (error_message != NULL)
	      *error_message = errMsg;
	  return 0;
      }
/* re-installing all Geometry Triggers [spatialite only with at least 1 geometry-column ; returns true for others] */
    if (!do_rebuild_geotriggers (sqlite, table, new_name, aux))
      {
	  if (aux->error_message)
	    {
		/* Use the prepaired message, if we have one */
		if (error_message)
		  {
		      *error_message =
			  sqlite3_mprintf ("%s", aux->error_message);
		  }
		sqlite3_free (aux->error_message);
		aux->error_message = NULL;
	    }
	  else if (error_message != NULL)
	    {
		*error_message =
		    sqlite3_mprintf ("unable to rebuild Geometry Triggers");
	    }
	  return 0;
      }
    /* update of registred vector-styles is not needed since 'vector_coverages'  now contains the renamed TABLE or COLUMN. The 'coverage_name' remains unchanged. */
    return 1;
}

static int
do_drop_table5 (sqlite3 * sqlite, const char *prefix, const char *table,
		struct table_params *aux, char **error_message)
{
/* dropping a Table */
    char *sql;
    char *q_prefix;
    char *q_table;
    int ret;
    char *errMsg = NULL;
    if ((aux) && (aux->metadata_version > 0)
	&& (aux->count_geometry_columns > 0))
      {
/* updating first all metadata tables */
	  if (aux->ok_geometry_columns)
	    {
		/* deleting from GEOMETRY_COLUMNS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".geometry_columns WHERE lower(f_table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_views_geometry_columns)
	    {
		/* deleting from VIEWS_GEOMETRY_COLUMNS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".views_geometry_columns WHERE lower(view_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }

	  if (aux->ok_layer_params)
	    {
		/* deleting from LAYER_PARAMS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".layer_params WHERE lower(table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_sub_classes)
	    {
		/* deleting from LAYER_SUB_CLASSES */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".layer_sub_classes WHERE lower(table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_table_layout)
	    {
		/* deleting from LAYER_TABLE_LAYOUT */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".layer_table_layout WHERE lower(table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_auth)
	    {
		/* deleting from GEOMETRY_COLUMNS_AUTH */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".geometry_columns_auth WHERE lower(f_table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_time)
	    {
		/* deleting from GEOMETRY_COLUMNS_TIME */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".geometry_columns_time WHERE lower(f_table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_field_infos)
	    {
		/* deleting from GEOMETRY_COLUMNS_FIELD_INFOS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".geometry_columns_field_infos WHERE lower(f_table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_statistics)
	    {
		/* deleting from GEOMETRY_COLUMNS_STATISTICS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".geometry_columns_statistics WHERE lower(f_table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_statistics)
	    {
		/* deleting from LAYER_STATISTICS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".layer_statistics WHERE lower(table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_vector_coverages)
	    {
		/* deleting from VECTOR_COVERAGES */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".vector_coverages WHERE lower(f_table_name) = lower(%Q)",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_views_geometry_columns_auth)
	    {
		/* deleting from VIEWS_GEOMETRY_COLUMNS_AUTH */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".views_geometry_columns_auth "
		     "WHERE lower(view_name) = lower(%Q)", q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      aux->error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_views_geometry_columns_field_infos)
	    {
		/* deleting from VIEWS_GEOMETRY_COLUMNS_FIELD_INFOS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".views_geometry_columns_field_infos "
		     "WHERE view_name = %Q", q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      aux->error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_views_geometry_columns_statistics)
	    {
		/* deleting from VIEWS_GEOMETRY_COLUMNS_STATISTICS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".views_geometry_columns_statistics "
		     "WHERE lower(view_name) = lower(%Q)", q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      aux->error_message = errMsg;
		      return 0;
		  }
	    }
	  if ((aux->ok_gpkg_geometry_columns) && (aux->gpkg_table_type == 1))
	    {
		/* GeoPackage-Vector: must be removed before 'gpkg_contents' entry */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".gpkg_geometry_columns WHERE lower(table_name) = lower(%Q) ",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
      }
    if (aux->ok_gpkg_contents)
      {
	  /* GeoPackage only: update entry in gpkg_content [may be a raster] */
	  if (aux->gpkg_table_type == 2)
	    {
		/* GeoPackage-Raster: must be removed before 'gpkg_contents' entry */
		if (aux->ok_gpkg_tile_matrix)
		  {
		      /* update entry in gpkg_tile_matrix [only raster] */
		      q_prefix = gaiaDoubleQuotedSql (prefix);
		      sql =
			  sqlite3_mprintf
			  ("DELETE FROM \"%s\".gpkg_tile_matrix WHERE lower(table_name) = lower(%Q) ",
			   q_prefix, table);
		      free (q_prefix);
		      ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    if (error_message != NULL)
				*error_message = errMsg;
			    return 0;
			}
		  }
		if (aux->ok_gpkg_tile_matrix_set)
		  {
		      /* update entry in gpkg_tile_matrix_set [only raster] */
		      q_prefix = gaiaDoubleQuotedSql (prefix);
		      sql =
			  sqlite3_mprintf
			  ("DELETE FROM \"%s\".gpkg_tile_matrix_set WHERE lower(table_name) = lower(%Q) ",
			   q_prefix, table);
		      free (q_prefix);
		      ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    if (error_message != NULL)
				*error_message = errMsg;
			    return 0;
			}
		  }
	    }
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".gpkg_contents WHERE lower(table_name) = lower(%Q) ",
	       q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		if (error_message != NULL)
		    *error_message = errMsg;
		return 0;
	    }
	  if (aux->ok_gpkg_extensions)
	    {
		/* update entry in gpkg_extensions [may be a raster] */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".gpkg_extensions WHERE lower(table_name) = lower(%Q) ",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_gpkg_ogr_contents)
	    {
		/* update entry in gpkg_ogr_contents [may be a raster] */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".gpkg_ogr_contents WHERE lower(table_name) = lower(%Q) ",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_gpkg_metadata_reference)
	    {
		/* update entry in gpkg_metadata_reference [mostly raster] */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM \"%s\".gpkg_metadata_reference WHERE lower(table_name) = lower(%Q) ",
		     q_prefix, table);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      /* Note: there may be no entries */
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
      }				/* GeoPackage only */

/* dropping the Table itself */
    q_prefix = gaiaDoubleQuotedSql (prefix);
    q_table = gaiaDoubleQuotedSql (table);
    if (aux->is_view)
	sql = sqlite3_mprintf ("DROP VIEW \"%s\".\"%s\"", q_prefix, table);
    else
	sql = sqlite3_mprintf ("DROP TABLE \"%s\".\"%s\"", q_prefix, table);
    free (q_prefix);
    free (q_table);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  if (error_message != NULL)
	      *error_message = errMsg;
	  return 0;
      }

    if (aux->ok_virts_geometry_columns)
      {
	  /* deleting from VIRTS_GEOMETRY_COLUMNS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".virts_geometry_columns WHERE lower(virt_name) = lower(%Q)",
	       q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		if (error_message != NULL)
		    *error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_virts_geometry_columns_auth)
      {
	  /* deleting from VIRTS_GEOMETRY_COLUMNS_AUTH */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf ("DELETE FROM \"%s\".virts_geometry_columns_auth "
			       "WHERE lower(virt_name) = lower(%Q)", q_prefix,
			       table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_virts_geometry_columns_field_infos)
      {
	  /* deleting from VIRTS_GEOMETRY_COLUMNS_FIELD_INFOS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".virts_geometry_columns_field_infos "
	       "WHERE lower(virt_name) = lower(%Q)", q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }
    if (aux->ok_virts_geometry_columns_statistics)
      {
	  /* deleting from VIRTS_GEOMETRY_COLUMNS_STATISTICS */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("DELETE FROM \"%s\".virts_geometry_columns_statistics "
	       "WHERE lower(virt_name) = lower(%Q)", q_prefix, table);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		aux->error_message = errMsg;
		return 0;
	    }
      }

    return 1;
}

static int
do_rename_table_pre (sqlite3 * sqlite, const char *prefix, const char *old_name,
		     const char *new_name, struct table_params *aux,
		     char **error_message)
{
/* renaming a Table - preparatory steps */
    char *sql;
    char *q_prefix;
    int ret;
    char *errMsg = NULL;
    if ((aux) && (aux->metadata_version > 0)
	&& (aux->count_geometry_columns > 0))
      {
	  /*
	     // This is a spatialite, GeoPackage/FDO TABLE that contains a geometry-column [not a sqlite3 TABLE]
	     // dropping any previous Geometry Trigger  [spatialite and GeoPackage only with at least 1 geometry-column ; returns true for others]  
	   */
	  if (!do_drop_geotriggers (sqlite, old_name, NULL, aux, error_message))
	      return 0;

/* updating fist all metadata tables */
	  if ((aux->ok_geometry_columns) || (aux->ok_gpkg_geometry_columns))
	    {
		/* updating GEOMETRY_COLUMNS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		switch (aux->metadata_version)
		  {
		  case 1:
		  case 2:
		  case 3:
		      sql =
			  sqlite3_mprintf
			  ("UPDATE \"%s\".geometry_columns SET f_table_name = lower(%Q) WHERE lower(f_table_name) = lower(%Q)",
			   q_prefix, new_name, old_name);
		      break;
		  case 4:
		      sql =
			  sqlite3_mprintf
			  ("UPDATE \"%s\".gpkg_geometry_columns SET table_name =  lower(%Q) WHERE Lower(table_name) = Lower(%Q)",
			   q_prefix, new_name, old_name);
		      break;
		  }
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }

	  if (aux->ok_layer_params)
	    {
		/* updating LAYER_PARAMS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".layer_params SET table_name = lower(%Q) "
		     "WHERE lower(table_name) = lower(%Q)", q_prefix, new_name,
		     old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_sub_classes)
	    {
		/* updating LAYER_SUB_CLASSES */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".layer_sub_classes SET table_name = lower(%Q) "
		     "WHERE lower(table_name) = lower(%Q)", q_prefix, new_name,
		     old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_table_layout)
	    {
		/* updating LAYER_TABLE_LAYOUT */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".layer_table_layout SET table_name = lower(%Q) "
		     "WHERE lower(table_name) = lower(%Q)", q_prefix, new_name,
		     old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_auth)
	    {
		/* updating GEOMETRY_COLUMNS_AUTH */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns_auth SET f_table_name = lower(%Q) "
		     "WHERE lower(f_table_name) = lower(%Q)", q_prefix,
		     new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_time)
	    {
		/* updating GEOMETRY_COLUMNS_TIME */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns_time SET f_table_name = lower(%Q) "
		     "WHERE lower(f_table_name) = lower(%Q)", q_prefix,
		     new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_field_infos)
	    {
		/* updating GEOMETRY_COLUMNS_FIELD_INFOS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns_field_infos SET f_table_name = lower(%Q) "
		     "WHERE lower(f_table_name) = lower(%Q)", q_prefix,
		     new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_geometry_columns_statistics)
	    {
		/* updating GEOMETRY_COLUMNS_STATISTICS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".geometry_columns_statistics SET f_table_name = lower(%Q) "
		     "WHERE lower(f_table_name) = lower(%Q)", q_prefix,
		     new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_layer_statistics)
	    {
		/* updating LAYER_STATISTICS */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".layer_statistics SET table_name = lower(%Q) "
		     "WHERE lower(table_name) = lower(%Q)", q_prefix, new_name,
		     old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_vector_coverages)
	    {
		/* updating VECTOR_COVERAGES */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".vector_coverages SET f_table_name = %Q "
		     "WHERE lower(f_table_name) = lower(%Q)", q_prefix,
		     new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
      }
    if (aux->ok_gpkg_contents)
      {
	  /* GeoPackage only: update entry in gpkg_content [may be a raster] */
	  q_prefix = gaiaDoubleQuotedSql (prefix);
	  sql =
	      sqlite3_mprintf
	      ("UPDATE \"%s\".gpkg_contents SET table_name = lower(%Q) WHERE lower(table_name) = lower(%Q) ",
	       q_prefix, new_name, old_name);
	  free (q_prefix);
	  ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		if (error_message != NULL)
		    *error_message = errMsg;
		return 0;
	    }
	  if (aux->ok_gpkg_extensions)
	    {
		/* update entry in gpkg_extensions [may be a raster or SpatialIndex] */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		/* Remove SpatialIndex entry [will be added later during gpkgAddSpatialIndex] */
		sql =
		    sqlite3_mprintf
		    ("DELETE FROM  \"%s\".gpkg_extensions WHERE ((lower(table_name) = lower(%Q)) AND (extension_name = 'gpkg_rtree_index'))",
		     q_prefix, old_name);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      free (q_prefix);
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".gpkg_extensions SET table_name = lower(%Q) WHERE lower(table_name) = lower(%Q) ",
		     q_prefix, new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_gpkg_ogr_contents)
	    {
		/* update entry in gpkg_ogr_contents [may be a raster] */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".gpkg_ogr_contents SET table_name = lower(%Q) WHERE lower(table_name) = lower(%Q) ",
		     q_prefix, new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->ok_gpkg_metadata_reference)
	    {
		/* update entry in gpkg_metadata_reference [mostly raster] */
		q_prefix = gaiaDoubleQuotedSql (prefix);
		sql =
		    sqlite3_mprintf
		    ("UPDATE \"%s\".gpkg_metadata_reference SET table_name = lower(%Q) WHERE lower(table_name) = lower(%Q) ",
		     q_prefix, new_name, old_name);
		free (q_prefix);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      /* Note: there may be no entries */
		      if (error_message != NULL)
			  *error_message = errMsg;
		      return 0;
		  }
	    }
	  if (aux->gpkg_table_type == 2)
	    {
		/* GeoPackage-Rasters only */
		if (aux->ok_gpkg_tile_matrix)
		  {
		      /* update entry in gpkg_tile_matrix [only raster] */
		      q_prefix = gaiaDoubleQuotedSql (prefix);
		      sql =
			  sqlite3_mprintf
			  ("UPDATE \"%s\".gpkg_tile_matrix SET table_name = lower(%Q) WHERE lower(table_name) = lower(%Q) ",
			   q_prefix, new_name, old_name);
		      free (q_prefix);
		      ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    if (error_message != NULL)
				*error_message = errMsg;
			    return 0;
			}
		  }
		if (aux->ok_gpkg_tile_matrix_set)
		  {
		      /* update entry in gpkg_tile_matrix_set [only raster] */
		      q_prefix = gaiaDoubleQuotedSql (prefix);
		      sql =
			  sqlite3_mprintf
			  ("UPDATE \"%s\".gpkg_tile_matrix_set SET table_name = lower(%Q) WHERE lower(table_name) = lower(%Q) ",
			   q_prefix, new_name, old_name);
		      free (q_prefix);
		      ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
		      sqlite3_free (sql);
		      if (ret != SQLITE_OK)
			{
			    if (error_message != NULL)
				*error_message = errMsg;
			    return 0;
			}
		  }
	    }
      }				/* GeoPackage only */
    return 1;
}

static int
do_rename_table_post (sqlite3 * sqlite, const char *prefix,
		      const char *old_name, const char *new_name,
		      struct table_params *aux, char **error_message)
{
/* renaming a Table - final steps */
    char *sql;
    char *q_prefix;
    char *q_old;
    char *q_new;
    int ret;
    char *errMsg = NULL;

/* renaming the Table itself */
    q_prefix = gaiaDoubleQuotedSql (prefix);
    q_old = gaiaDoubleQuotedSql (old_name);
    q_new = gaiaDoubleQuotedSql (new_name);
    sql =
	sqlite3_mprintf ("ALTER TABLE \"%s\".\"%s\" RENAME TO \"%s\"", q_prefix,
			 q_old, q_new);
    free (q_prefix);
    free (q_old);
    free (q_new);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  if (error_message != NULL)
	      *error_message = errMsg;
	  return 0;
      }

/* re-installing all Geometry Triggers [spatialite only with at least 1 geometry-column ; returns true for others]  */
    if (!do_rebuild_geotriggers (sqlite, new_name, NULL, aux))
      {
	  if (aux->error_message)
	    {
		/* Use the prepaired message, if we have one */
		if (error_message)
		  {
		      *error_message =
			  sqlite3_mprintf ("%s", aux->error_message);
		  }
		sqlite3_free (aux->error_message);
		aux->error_message = NULL;
	    }
	  else if (error_message != NULL)
	    {
		*error_message =
		    sqlite3_mprintf ("unable to rebuild Geometry Triggers");
	    }
	  return 0;
      }
    /* update of registred vector-styles is not needed since 'vector_coverages'  now contains the renamed TABLE or COLUMN. The 'coverage_name' remains unchanged. */
    return 1;
}

static int
do_drop_rtree (sqlite3 * sqlite, const char *prefix, const char *rtree,
	       char **error_message)
{
/* dropping some R*Tree */
    char *sql;
    char *q_prefix;
    char *q_rtree;
    int ret;
    char *errMsg = NULL;

    q_prefix = gaiaDoubleQuotedSql (prefix);
    q_rtree = gaiaDoubleQuotedSql (rtree);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\".\"%s\"", q_prefix, q_rtree);
    free (q_prefix);
    free (q_rtree);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  if (error_message != NULL)
	      *error_message = errMsg;
	  return 0;
      }
    return 1;
}

static int
do_drop_sub_view (sqlite3 * sqlite, const char *prefix, const char *table,
		  struct table_params *aux)
{
/* dropping any depending View */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    char *q_prefix;
    struct table_params aux2;

    if (aux->ok_views_geometry_columns == 0)
	return 1;

/* initializing the aux params */
    aux2.rtrees = NULL;
    aux2.n_rtrees = 0;
    aux2.is_view = 1;
    aux2.ok_geometry_columns = 0;
    aux2.ok_geometry_columns_time = aux->ok_geometry_columns_time;
    aux2.ok_views_geometry_columns = aux->ok_views_geometry_columns;
    aux2.ok_virts_geometry_columns = aux->ok_virts_geometry_columns;
    aux2.ok_geometry_columns_auth = aux->ok_geometry_columns_auth;
    aux2.ok_geometry_columns_field_infos = aux->ok_geometry_columns_field_infos;
    aux2.ok_geometry_columns_statistics = aux->ok_geometry_columns_statistics;
    aux2.ok_views_geometry_columns_auth = aux->ok_views_geometry_columns_auth;
    aux2.ok_views_geometry_columns_field_infos =
	aux->ok_views_geometry_columns_field_infos;
    aux2.ok_views_geometry_columns_statistics =
	aux->ok_views_geometry_columns_statistics;
    aux2.ok_virts_geometry_columns_auth = aux->ok_virts_geometry_columns_auth;
    aux2.ok_virts_geometry_columns_field_infos =
	aux->ok_virts_geometry_columns_field_infos;
    aux2.ok_virts_geometry_columns_statistics =
	aux->ok_virts_geometry_columns_statistics;
    aux2.ok_layer_statistics = aux->ok_layer_statistics;
    aux2.ok_views_layer_statistics = aux->ok_views_layer_statistics;
    aux2.ok_virts_layer_statistics = aux->ok_virts_layer_statistics;
    aux2.ok_layer_params = aux->ok_layer_params;
    aux2.ok_layer_sub_classes = aux->ok_layer_sub_classes;
    aux2.ok_layer_table_layout = aux->ok_layer_table_layout;
    aux2.ok_vector_coverages = aux->ok_vector_coverages;
    aux2.ok_vector_coverages_keyword = aux->ok_vector_coverages_keyword;
    aux2.ok_vector_coverages_srid = aux->ok_vector_coverages_srid;
    aux2.ok_se_vector_styled_layers = aux->ok_se_vector_styled_layers;
    aux2.ok_se_raster_styled_layers = aux->ok_se_raster_styled_layers;
    aux2.metadata_version = aux->metadata_version;
    aux2.ok_gpkg_geometry_columns = aux->ok_gpkg_geometry_columns;
    aux2.ok_gpkg_contents = aux->ok_gpkg_contents;
    aux2.ok_gpkg_extensions = aux->ok_gpkg_extensions;
    aux2.ok_gpkg_tile_matrix = aux->ok_gpkg_tile_matrix;
    aux2.ok_gpkg_tile_matrix_set = aux->ok_gpkg_tile_matrix_set;
    aux2.ok_gpkg_ogr_contents = aux->ok_gpkg_ogr_contents;
    aux2.ok_gpkg_metadata_reference = aux->ok_gpkg_metadata_reference;
    aux2.gpkg_table_type = aux->gpkg_table_type;
    /* 'command_type MUST not be reset! */
    aux2.ok_table_exists = 0;
    aux2.error_message = NULL;
    aux2.ok_table_exists = 0;
    aux2.is_geometry_column = 0;
    aux2.count_geometry_columns = 0;
    aux2.is_view = 0;		/* default: not a view */
    aux2.table_type = -1;	/* default: table not found */
    aux2.has_topologies = 0;
    aux2.is_raster_coverage_entry = 0;
    aux2.has_raster_coverages = 0;

/* identifying any View depending on the target */
    q_prefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf ("SELECT view_name FROM \"%s\".views_geometry_columns "
			 "WHERE Lower(f_table_name) = Lower(%Q)", q_prefix,
			 table);
    free (q_prefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *name = results[(i * columns) + 0];
		/* dropping the view itself */
		if (!do_drop_table (sqlite, prefix, name, &aux2))
		    return 0;
	    }
      }
    sqlite3_free_table (results);
    return 1;
}

static int
check_table_layout (sqlite3 * sqlite, const char *prefix, const char *table,
		    struct table_params *aux)
{
/* checking the actual DB configuration */
    int i;
    char **results;
    int rows;
    int columns;
    char jolly = '%';
    int ret;
    char *sql;
    char *q_prefix = gaiaDoubleQuotedSql (prefix);

    if (!aux)
      {
	  return 0;
      }
/* initializing the aux params */
    aux->rtrees = NULL;
    aux->n_rtrees = 0;
    aux->is_view = 0;
    aux->ok_geometry_columns = 0;
    aux->ok_geometry_columns_time = 0;
    aux->ok_views_geometry_columns = 0;
    aux->ok_virts_geometry_columns = 0;
    aux->ok_geometry_columns_auth = 0;
    aux->ok_geometry_columns_field_infos = 0;
    aux->ok_geometry_columns_statistics = 0;
    aux->ok_views_geometry_columns_auth = 0;
    aux->ok_views_geometry_columns_field_infos = 0;
    aux->ok_views_geometry_columns_statistics = 0;
    aux->ok_virts_geometry_columns_auth = 0;
    aux->ok_virts_geometry_columns_field_infos = 0;
    aux->ok_virts_geometry_columns_statistics = 0;
    aux->ok_layer_statistics = 0;
    aux->ok_views_layer_statistics = 0;
    aux->ok_virts_layer_statistics = 0;
    aux->ok_layer_params = 0;
    aux->ok_layer_sub_classes = 0;
    aux->ok_layer_table_layout = 0;
    aux->ok_vector_coverages = 0;
    aux->ok_vector_coverages_keyword = 0;
    aux->ok_vector_coverages_srid = 0;
    aux->ok_se_vector_styled_layers = 0;
    aux->ok_se_raster_styled_layers = 0;
    aux->metadata_version = 0;
    aux->ok_gpkg_geometry_columns = 0;
    aux->ok_gpkg_contents = 0;
    aux->ok_gpkg_extensions = 0;
    aux->ok_gpkg_tile_matrix = 0;
    aux->ok_gpkg_tile_matrix_set = 0;
    aux->ok_gpkg_ogr_contents = 0;
    aux->ok_gpkg_metadata_reference = 0;
    aux->gpkg_table_type = 0;
    /* 'command_type MUST not be reset! */
    aux->ok_table_exists = 0;
    aux->error_message = NULL;

    aux->is_geometry_column = 0;
    aux->count_geometry_columns = 0;
    aux->is_view = 0;		/* default: not a view */
    aux->table_type = -1;	/* default: table not found */
    aux->has_topologies = 0;
    aux->is_raster_coverage_entry = 0;
    aux->has_raster_coverages = 0;
    if (strcasecmp (prefix, "TEMP") == 0)
      {
	  /* TEMPORARY object; unconditioanally returning TRUE */
	  free (q_prefix);
	  aux->ok_table_exists = 1;
	  aux->table_type = 1;
	  return 1;
      }
    sql =
	sqlite3_mprintf
	("SELECT type, name FROM \"%s\".sqlite_master WHERE type = 'table' or type = 'view'",
	 q_prefix);
    free (q_prefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	ret = 0;
    else
      {
	  ret = 1;
	  for (i = 1; i <= rows; i++)
	    {
		const char *type = results[(i * columns) + 0];
		const char *name = results[(i * columns) + 1];
		if (name)
		  {
		      /* checking which tables are actually defined */
		      if (strcasecmp (name, "geometry_columns") == 0)
			  aux->ok_geometry_columns = 1;
		      else if (strcasecmp (name, "geometry_columns_time") == 0)
			  aux->ok_geometry_columns_time = 1;
		      else if (strcasecmp (name, "views_geometry_columns") == 0)
			  aux->ok_views_geometry_columns = 1;
		      else if (strcasecmp (name, "virts_geometry_columns") == 0)
			  aux->ok_virts_geometry_columns = 1;
		      else if (strcasecmp (name, "geometry_columns_auth") == 0)
			  aux->ok_geometry_columns_auth = 1;
		      else if (strcasecmp (name, "views_geometry_columns_auth")
			       == 0)
			  aux->ok_views_geometry_columns_auth = 1;
		      else if (strcasecmp (name, "virts_geometry_columns_auth")
			       == 0)
			  aux->ok_virts_geometry_columns_auth = 1;
		      if (strcasecmp (name, "geometry_columns_statistics") == 0)
			  aux->ok_geometry_columns_statistics = 1;
		      else if (strcasecmp
			       (name, "views_geometry_columns_statistics") == 0)
			  aux->ok_views_geometry_columns_statistics = 1;
		      else if (strcasecmp
			       (name, "virts_geometry_columns_statistics") == 0)
			  aux->ok_virts_geometry_columns_statistics = 1;
		      else if (strcasecmp (name, "geometry_columns_field_infos")
			       == 0)
			  aux->ok_geometry_columns_field_infos = 1;
		      if (strcasecmp
			  (name, "views_geometry_columns_field_infos") == 0)
			  aux->ok_views_geometry_columns_field_infos = 1;
		      else if (strcasecmp
			       (name,
				"virts_geometry_columns_field_infos") == 0)
			  aux->ok_virts_geometry_columns_field_infos = 1;
		      else if (strcasecmp (name, "layer_params") == 0)
			  aux->ok_layer_params = 1;
		      else if (strcasecmp (name, "layer_statistics") == 0)
			  aux->ok_layer_statistics = 1;
		      else if (strcasecmp (name, "layer_sub_classes") == 0)
			  aux->ok_layer_sub_classes = 1;
		      else if (strcasecmp (name, "layer_table_layout") == 0)
			  aux->ok_layer_table_layout = 1;
		      else if (strcasecmp (name, "views_geometry_columns") == 0)
			  aux->ok_views_geometry_columns = 1;
		      else if (strcasecmp (name, "virts_geometry_columns") == 0)
			  aux->ok_virts_geometry_columns = 1;
		      else if (strcasecmp (name, "vector_coverages") == 0)
			  aux->ok_vector_coverages = 1;
		      else if (strcasecmp (name, "vector_coverages_keyword") ==
			       0)
			  aux->ok_vector_coverages_keyword = 1;
		      else if (strcasecmp (name, "vector_coverages_srid") == 0)
			  aux->ok_vector_coverages_srid = 1;
		      else if (strcasecmp (name, "se_vector_styled_layers") ==
			       0)
			  aux->ok_se_vector_styled_layers = 1;
		      else if (strcasecmp (name, "SE_raster_styled_layers") ==
			       0)
			  aux->ok_se_raster_styled_layers = 1;
		      else if (strcasecmp (name, "topologies") == 0)
			  aux->has_topologies = 1;
		      else if (strcasecmp (name, "raster_coverages") == 0)
			  aux->has_raster_coverages = 1;
		      else if (strcasecmp (name, "gpkg_contents") == 0)
			  aux->ok_gpkg_contents = 1;
		      else if (strcasecmp (name, "gpkg_geometry_columns") == 0)
			  aux->ok_gpkg_geometry_columns = 1;
		      else if (strcasecmp (name, "gpkg_extensions") == 0)
			  aux->ok_gpkg_extensions = 1;
		      else if (strcasecmp (name, "gpkg_tile_matrix") == 0)
			  aux->ok_gpkg_tile_matrix = 1;
		      else if (strcasecmp (name, "gpkg_tile_matrix_set") == 0)
			  aux->ok_gpkg_tile_matrix_set = 1;
		      else if (strcasecmp (name, "gpkg_ogr_contents") == 0)
			  aux->ok_gpkg_ogr_contents = 1;
		      else if (strcasecmp (name, "gpkg_metadata_reference") ==
			       0)
			  aux->ok_gpkg_metadata_reference = 1;

		      if (strcasecmp (name, table) == 0)
			{
			    aux->ok_table_exists = 1;
			    /* checking if the target is a view */
			    if (strcasecmp (type, "table") == 0)
				aux->table_type = 0;
			    else if (strcasecmp (type, "view") == 0)
			      {
				  aux->is_view = 1;
				  aux->table_type = 1;
			      }
			    else if (strcasecmp (type, "trigger") == 0)
				aux->table_type = 2;
			    else if (strcasecmp (type, "index") == 0)
				aux->table_type = 3;
			}
		  }
	    }
      }
    sqlite3_free_table (results);
    if (!ret)
	return 0;

    /* Used to avoid damage of Fdo-Tables */
    aux->metadata_version = checkSpatialMetaData (sqlite);

/* identifying any possible R*Tree supporting the main target */
    q_prefix = gaiaDoubleQuotedSql (prefix);
    if (aux->ok_gpkg_geometry_columns == 1)
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT name FROM \"%s\".sqlite_master WHERE type = 'table' AND "
	       "Lower(name) IN (SELECT "
	       "Lower('rtree_' || table_name || '_' || column_name) "
	       "FROM \"%s\".gpkg_geometry_columns  WHERE Lower(table_name) = Lower(%Q)) "
	       "AND sql LIKE('%cvirtual%c') AND sql LIKE('%crtree%c')",
	       q_prefix, q_prefix, table, jolly, jolly, jolly, jolly);
      }
    else
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT name FROM \"%s\".sqlite_master WHERE type = 'table' AND "
	       "Lower(name) IN (SELECT "
	       "Lower('idx_' || f_table_name || '_' || f_geometry_column) "
	       "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q)) "
	       "AND sql LIKE('%cvirtual%c') AND sql LIKE('%crtree%c')",
	       q_prefix, q_prefix, table, jolly, jolly, jolly, jolly);
      }
    free (q_prefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
/* allocating the rtrees array */
	  aux->n_rtrees = rows;
	  aux->rtrees = malloc (sizeof (char **) * aux->n_rtrees);
	  for (i = 1; i <= rows; i++)
	    {
		const char *name = results[(i * columns) + 0];
		int len = strlen (name);
		*(aux->rtrees + (i - 1)) = malloc (len + 1);
		strcpy (*(aux->rtrees + (i - 1)), name);
	    }
      }
    sqlite3_free_table (results);
    return 1;
}

static int
check_topology_table (sqlite3 * sqlite, const char *prefix, const char *table)
{
/* checking for GeoTables belonging to some TopoGeo or TopoNet */
    char *xprefix;
    char *sql;
    char *table_name;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int found = 0;

    if (prefix == NULL)
	prefix = "main";

/* testing within Topologies */
    xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf ("SELECT topology_name FROM \"%s\".topologies",
			 xprefix);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto networks;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *name = results[(i * columns) + 0];
		table_name = sqlite3_mprintf ("%s_node", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_edge", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_face", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_seeds", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_topofeatures", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_topolayers", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
	    }
      }
    sqlite3_free_table (results);
    if (found)
      {
	  spatialite_e ("DropTable: can't drop TopoGeo table \"%s\".\"%s\"",
			prefix, table);
	  return 1;
      }

  networks:
/* testing within Networks */
    xprefix = gaiaDoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("SELECT network_name FROM \"%s\".netowrks", xprefix);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto end;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *name = results[(i * columns) + 0];
		table_name = sqlite3_mprintf ("%s_node", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_link", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_seeds", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
	    }
      }
    sqlite3_free_table (results);
    if (found)
	return 1;

  end:
    return 0;
}

static int
check_raster_table (sqlite3 * sqlite, const char *prefix, const char *table,
		    struct table_params *aux)
{
/* checking for Raster Coverage tables */
    char *xprefix;
    char *sql;
    char *table_name;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int found = 0;

    if (prefix == NULL)
	prefix = "main";

/* testing within Raster Coverages */
    xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf ("SELECT coverage_name FROM \"%s\".raster_coverages",
			 xprefix);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto end;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *name = results[(i * columns) + 0];
		if (strcasecmp (table, name) == 0)
		  {
		      aux->is_raster_coverage_entry = 1;
		      found = 1;
		  }
		table_name = sqlite3_mprintf ("%s_node", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_levels", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_sections", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_tiles", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name = sqlite3_mprintf ("%s_tile_data", name);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
	    }
      }
    sqlite3_free_table (results);
    if (found)
	return 1;
  end:
    return 0;
}

static int
check_rtree_internal_table (sqlite3 * sqlite, const char *prefix,
			    const char *table, int is_gpkg)
{
/* checking for R*Tree internal tables */
    char *xprefix;
    char *rtree_type = NULL;
    char *sql;
    char *table_name;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int found = 0;

    if (prefix == NULL)
	prefix = "main";

/* testing within Raster Coverages */
    xprefix = gaiaDoubleQuotedSql (prefix);
    if (!is_gpkg)
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT f_table_name, f_geometry_column FROM \"%s\".geometry_columns WHERE spatial_index_enabled = 1",
	       xprefix);
	  rtree_type = sqlite3_mprintf ("idx");
      }
    else
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT table_name, column_name FROM \"%s\".gpkg_geometry_columns ",
	       xprefix);
	  rtree_type = sqlite3_mprintf ("rtree");
      }
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto end;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *tbl = results[(i * columns) + 0];
		const char *geom = results[(i * columns) + 1];
		table_name =
		    sqlite3_mprintf ("%s_%s_%s_node", rtree_type, tbl, geom);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name =
		    sqlite3_mprintf ("%s_%s_%s_parent", rtree_type, tbl, geom);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
		table_name =
		    sqlite3_mprintf ("%s_%s_%s_rowid", rtree_type, tbl, geom);
		if (strcasecmp (table, table_name) == 0)
		    found = 1;
		sqlite3_free (table_name);
	    }
      }
    sqlite3_free_table (results);
    sqlite3_free (rtree_type);
    rtree_type = NULL;
    if (found)
	return 1;
  end:
    if (rtree_type)
	sqlite3_free (rtree_type);
    return 0;
}

static int
check_spatialite_table (const char *table)
{
/*
// Note: sqlite3 prevents 'sqlite_master' from being droped [will not list itsself, returns 'not existing table']
// checking for SpatiaLite / RasterLite2 internal tables 
*/
    if (strcasecmp (table, "data_licenses") == 0)
	return 1;
    if (strcasecmp (table, "geometry_columns") == 0)
	return 1;
    if (strcasecmp (table, "geometry_columns_time") == 0)
	return 1;
    if (strcasecmp (table, "networks") == 0)
	return 1;
    if (strcasecmp (table, "postgres_geometry_columns") == 0)
	return 1;
    if (strcasecmp (table, "raster_coverages") == 0)
	return 1;
    if (strcasecmp (table, "raster_coverages_keyword") == 0)
	return 1;
    if (strcasecmp (table, "raster_coverages_ref_sys") == 0)
	return 1;
    if (strcasecmp (table, "raster_coverages_srid") == 0)
	return 1;
    if (strcasecmp (table, "spatial_ref_sys") == 0)
	return 1;
    if (strcasecmp (table, "spatial_ref_sys_all") == 0)
	return 1;
    if (strcasecmp (table, "spatial_ref_sys_aux") == 0)
	return 1;
    if (strcasecmp (table, "spatialite_history") == 0)
	return 1;
    if (strcasecmp (table, "stored_procedures") == 0)
	return 1;
    if (strcasecmp (table, "stored_variables") == 0)
	return 1;
    if (strcasecmp (table, "tmp_vector_coverages") == 0)
	return 1;
    if (strcasecmp (table, "topologies") == 0)
	return 1;
    if (strcasecmp (table, "vector_coverages") == 0)
	return 1;
    if (strcasecmp (table, "vector_coverages_keyword") == 0)
	return 1;
    if (strcasecmp (table, "vector_coverages_ref_sys") == 0)
	return 1;
    if (strcasecmp (table, "vector_coverages_srid") == 0)
	return 1;
    if (strcasecmp (table, "vector_layers") == 0)
	return 1;
    if (strcasecmp (table, "views_geometry_columns") == 0)
	return 1;
    if (strcasecmp (table, "virts_geometry_columns") == 0)
	return 1;
    if (strcasecmp (table, "geometry_columns_auth") == 0)
	return 1;
    if (strcasecmp (table, "geometry_columns_field_infos") == 0)
	return 1;
    if (strcasecmp (table, "geometry_columns_statistics") == 0)
	return 1;
    if (strcasecmp (table, "geom_cols_ref_sys") == 0)
	return 1;
    if (strcasecmp (table, "sql_statements_log") == 0)
	return 1;
    if (strcasecmp (table, "vector_layers_auth") == 0)
	return 1;
    if (strcasecmp (table, "vector_layers_field_infos") == 0)
	return 1;
    if (strcasecmp (table, "vector_layers_statistics") == 0)
	return 1;
    if (strcasecmp (table, "views_geometry_columns_auth") == 0)
	return 1;
    if (strcasecmp (table, "views_geometry_columns_field_infos") == 0)
	return 1;
    if (strcasecmp (table, "views_geometry_columns_statistics") == 0)
	return 1;
    if (strcasecmp (table, "virts_geometry_columns_auth") == 0)
	return 1;
    if (strcasecmp (table, "virts_geometry_columns_field_infos") == 0)
	return 1;
    if (strcasecmp (table, "virts_geometry_columns_statistics") == 0)
	return 1;
    if (strcasecmp (table, "SE_external_graphics") == 0)
	return 1;
    if (strcasecmp (table, "SE_fonts") == 0)
	return 1;
    if (strcasecmp (table, "SE_group_styles") == 0)
	return 1;
    if (strcasecmp (table, "SE_raster_styled_layers") == 0)
	return 1;
    if (strcasecmp (table, "SE_styled_group_refs") == 0)
	return 1;
    if (strcasecmp (table, "SE_vector_styled_layers") == 0)
	return 1;
    if (strcasecmp (table, "SE_vector_styles") == 0)
	return 1;
    if (strcasecmp (table, "iso_metadata") == 0)
	return 1;
    if (strcasecmp (table, "iso_metadata_reference") == 0)
	return 1;
    if (strcasecmp (table, "KNN") == 0)
	return 1;
    if (strcasecmp (table, "SpatialIndex") == 0)
	return 1;
    return 0;
}

static int
check_gpkg_table (const char *table)
{
/*
// Note: sqlite3 prevents 'sqlite_master' from being droped [will not list itsself, returns 'not existing table']
// checking for GeoPackage / RasterLite2 internal tables 
*/
    if (strcasecmp (table, "gpkg_contents") == 0)
	return 1;
    if (strcasecmp (table, "gpkg_extensions") == 0)
	return 1;
    if (strcasecmp (table, "gpkg_geometry_columns") == 0)
	return 1;
    if (strcasecmp (table, "gpkg_metadata") == 0)
	return 1;
    if (strcasecmp (table, "gpkg_metadata_reference") == 0)
	return 1;
    if (strcasecmp (table, "gpkg_spatial_ref_sys") == 0)
	return 1;
    if (strcasecmp (table, "gpkg_tile_matrix") == 0)
	return 1;
    if (strcasecmp (table, "gpkg_tile_matrix_set") == 0)
	return 1;
    if (strcasecmp (table, "gpkg_ogr_contents") == 0)
	return 1;
    return 0;
}

/*/ -- -- ---------------------------------- --
// do_check_gpkg_table_type
// -- -- ---------------------------------- --
// Called from
// * check_for_system_tables
// -- -- ---------------------------------- --
// gpkg_type:
// - 0 : table not found in 'gpkg_contents'
// - 1: table is a 'features' type [Vector]
// - 2: table is a 'tiles' type [Raster]
// -- -- ---------------------------------- --
// Author: Mark Johnson <mj10777@googlemail.com> 2019-01-12
// -- -- ---------------------------------- --
*/
static int
do_check_gpkg_table_type (sqlite3 * sqlite, const char *prefix,
			  const char *table)
{
/* checking for a registered GeoPackage Vector or RasterTable */
    char *xprefix;
    char *sql_statement;
    int ret;
    sqlite3_stmt *stmt_sql = NULL;
    int gpkg_type = 0;		/* Table not found */
    if (prefix == NULL)
	prefix = "main";
    xprefix = gaiaDoubleQuotedSql (prefix);
    sql_statement =
	sqlite3_mprintf
	("SELECT CASE WHEN (data_type = 'features') THEN 1 ELSE 2 END FROM \"%s\".gpkg_contents WHERE Upper(table_name) = Upper(%Q)",
	 xprefix, table);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql_statement, -1, &stmt_sql, NULL);
    if (ret == SQLITE_OK)
      {
	  sqlite3_free (sql_statement);
	  while (sqlite3_step (stmt_sql) == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_sql, 0) != SQLITE_NULL)
		  {
		      gpkg_type = sqlite3_column_int (stmt_sql, 0);	/* 1=vector ; 2=raster */
		  }
	    }
	  sqlite3_finalize (stmt_sql);
      }
    return gpkg_type;
}

static int
do_check_geometry_column (sqlite3 * sqlite, const char *prefix,
			  const char *table, const char *column,
			  struct table_params *aux)
{
/* checking for a registered GeoPackage Vector or RasterTable */
    char *xprefix;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int is_geotable = 0;
    if ((aux) && (aux->metadata_version > 0)
	&& ((aux->ok_geometry_columns == 1)
	    || (aux->ok_gpkg_geometry_columns == 1)))
      {
	  /* This is a spatialite, GeoPackage/FDO TABLE that contains a geometry-column [not a sqlite3 TABLE] */
	  if (prefix == NULL)
	      prefix = "main";
	  /* parsing the SQL CREATE statement */
	  xprefix = gaiaDoubleQuotedSql (prefix);
	  if (aux->ok_geometry_columns == 1)
	    {
		if (column)
		  {
		      sql =
			  sqlite3_mprintf
			  ("SELECT Count(*) FROM \"%s\".geometry_columns WHERE ((Upper(f_table_name) = Upper(%Q)) AND (Upper(f_geometry_column) = Upper(%Q)))",
			   xprefix, table, column);
		  }
		else
		  {
		      sql =
			  sqlite3_mprintf
			  ("SELECT Count(*) FROM \"%s\".geometry_columns WHERE (Upper(f_table_name) = Upper(%Q))",
			   xprefix, table);
		  }
	    }
	  else
	    {
		if (column)
		  {
		      sql =
			  sqlite3_mprintf
			  ("SELECT Count(*) FROM \"%s\".gpkg_geometry_columns WHERE ((Upper(table_name) = Upper(%Q)) AND (Upper(column_name) = Upper(%Q)))",
			   xprefix, table, column);
		  }
		else
		  {
		      sql =
			  sqlite3_mprintf
			  ("SELECT Count(*) FROM \"%s\".gpkg_geometry_columns WHERE (Upper(table_name) = Upper(%Q))",
			   xprefix, table);
		  }
	    }
	  free (xprefix);
	  ret =
	      sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      goto end;
	  if (rows < 1)
	      ;
	  else
	    {
		for (i = 1; i <= rows; i++)
		  {
		      const char *count = results[(i * columns) + 0];
		      if (atoi (count) > 0)
			{
			    is_geotable = 1;
			    if (column)
			      {
				  aux->is_geometry_column = 1;
			      }
			    else
			      {
				  aux->count_geometry_columns = atoi (count);
			      }
			}
		  }
	    }
	  sqlite3_free_table (results);
      }
    if (is_geotable)
	return 1;
  end:
    return 0;
}

static int
do_check_geotable (sqlite3 * sqlite, const char *prefix, const char *table)
{
/* checking for a registered GeoTable */
    char *xprefix;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int is_geotable = 0;

    if (prefix == NULL)
	prefix = "main";

/* parsing the SQL CREATE statement */
    xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM \"%s\".geometry_columns WHERE Upper(f_table_name) = Upper(%Q)",
	 xprefix, table);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto end;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *count = results[(i * columns) + 0];
		if (atoi (count) > 0)
		    is_geotable = 1;
	    }
      }
    sqlite3_free_table (results);
    if (is_geotable)
	return 1;
  end:
    return 0;
}

/*/ -- -- ---------------------------------- --
// check_for_system_tables
// -- -- ---------------------------------- --
// Called from
// * gaiaDropTable5 [command_type=0]
// * gaiaDropTableEx3 [command_type=0]
// * gaiaRenameTable [command_type=1 and 10 (for new table-name)]
// * gaiaRenameColumn [command_type=2 and 20]
// -- -- ---------------------------------- --
// table_type: [as found in 'sqlite_master']
// - 0: table
// - 1: view
// - 2: trigger
// - 3: index
// -- -- ---------------------------------- --
// RasterLite2, GeoPackage and Fdo supported: for Raster and Vector
// * gaiaRenameColumn: not supported for RasterLite2 [returns error]
// -- -- ---------------------------------- --
// Styles: 
// - Vector:
// -> Update of registred vector-styles is not needed since 'vector_coverages' will contain the renamed TABLE or COLUMN. 7
// -> The 'coverage_name' remains unchanged.
// - Raster:
//  Update of any registred raster-style is delt with the renamed raster_coverage name
// -- -- ---------------------------------- --
// Author: Mark Johnson <mj10777@googlemail.com> 2019-01-12
// -- -- ---------------------------------- --
*/
static int
check_for_system_tables (sqlite3 * sqlite, const char *prefix,
			 const char *table, const char *column,
			 struct table_params *aux)
{
    if (aux)
      {
	  if (aux->command_type >= 10)
	    {
		/* Avoid 'Uninitialised value was created by a stack allocation' report */
		if (((aux->ok_table_exists == 1) && (aux->table_type == 0))
		    || (aux->is_raster_coverage_entry == 1))
		  {
		      /* 10 and 20 assume that 1 or 2 has been called and the TABLE is valid for changing [not: view, trigger or index] */
		      if (aux->command_type == 10)
			{
			    /* gaiaRenameTable checking if new TABLE name exists as [table, view, trigger or index] */
			    if (aux->is_raster_coverage_entry == 1)
			      {
				  if (check_raster_table
				      (sqlite, prefix, table, aux))
				    {
					/* if the old-table name exists and is a Raster-Coverage-table, then this is an error */
					aux->error_message =
					    sqlite3_mprintf
					    ("forbidden: Raster Coverage name exists [%s.%s]",
					     prefix, table);
					goto error_free_aux_memory;
				    }
			      }
			    else
			      {
				  if (do_check_existing
				      (sqlite, prefix, table, 3))
				    {
					/* if the new table-name exists, then this is an error */
					aux->error_message =
					    sqlite3_mprintf
					    ("already existing table [%s.%s]",
					     prefix, table);
					goto error_free_aux_memory;
				    }
			      }
			    return 1;
			}
		      if (aux->command_type == 20)
			{
			    /* gaiaRenameColumn checking if new COLUMN name exists */
			    if (do_check_existing_column
				(sqlite, prefix, table, column))
			      {
				  /* if the new column-name exists, then this is an error */
				  aux->error_message =
				      sqlite3_mprintf
				      ("column already defined [%s.%s] %s",
				       prefix, table, column);
				  goto error_free_aux_memory;
			      }
			    return 1;
			}
		  }
	    }
	  /* checking the actual DB configuration, storing found values */
	  if (!check_table_layout (sqlite, prefix, table, aux))
	    {
		/* fill 'table_params' with found system-tables [also checking if table exists and table_type] */
		aux->error_message =
		    sqlite3_mprintf ("unable to get the DB layout");
		goto error_free_aux_memory;
	    }
	  if (aux->has_raster_coverages == 1)
	    {
		/*
		   // Only if TABLE 'raster_coverages' exist 
		   // avoiding to drop/rename Raster Coverage tables 
		   // Will set is_raster_coverage_entry = 1 if 'table' is a raster converage entry
		 */
		if (check_raster_table (sqlite, prefix, table, aux))
		  {
		      if (aux->is_raster_coverage_entry == 0)
			{
			    /* if the old-table name exists and is a Raster-Coverage Admin-table, then this is an error */
			    aux->error_message =
				sqlite3_mprintf
				("forbidden: Raster Coverage internal Table [%s.%s]",
				 prefix, table);
			    goto error_free_aux_memory;
			}
		  }
		if (aux->is_raster_coverage_entry == 1)
		  {
		      /*
		         // A raster converage entry was found
		         //  Update/Delete any registred raster-styles delt with
		       */
		      if (aux->command_type == 2)
			{
			    /* gaiaRenameColumn */
			    aux->error_message =
				sqlite3_mprintf
				("forbidden: no column to rename in a Raster Coverage [%s.%s]",
				 prefix, table);
			    goto error_free_aux_memory;
			}
		  }
	    }
	  if ((aux->ok_table_exists == 0)
	      && (aux->is_raster_coverage_entry == 0))
	    {
		/* if the old-table name does NOT exist, then this is an error */
		aux->error_message =
		    sqlite3_mprintf ("not existing table [%s.%s]", prefix,
				     table);
		goto error_free_aux_memory;
	    }
	  if ((aux->command_type == 0) && (aux->is_raster_coverage_entry == 0))
	    {
		/*
		   // gaiaDropTable checking source name [when not a raster_coverage name]
		   // checking for an existing trigger or index 
		 */
		if ((aux->table_type < 0) || (aux->table_type > 1))
		  {
		      /* if the old-table name (as a trigger or index) exists, then this is an error */
		      aux->error_message =
			  sqlite3_mprintf
			  ("forbidden: can't DROP a Trigger or Index, only Table and Views are supported [%s.%s]",
			   prefix, table);
		      goto error_free_aux_memory;
		  }
	    }
	  if (((aux->command_type == 0) || (aux->command_type == 1))
	      && (aux->is_raster_coverage_entry == 0))
	    {
		/* Set aux->count_geometry_columns with the amount of geometry-columns this TABLE contains. */
		do_check_geometry_column (sqlite, prefix, table, NULL, aux);
	    }
	  if ((aux->command_type == 1) || (aux->command_type == 2))
	    {
		/*
		   // gaiaRenameTable checking source name [any trigger or index has already returned with error]
		   // checking for an existing view 
		 */
		if (aux->table_type == 1)
		  {
		      /* if the old-table name (as a view) exists, then this is an error */
		      aux->error_message =
			  sqlite3_mprintf
			  ("forbidden: can't rename a View, only Tables are supported [%s.%s]",
			   prefix, table);
		      goto error_free_aux_memory;
		  }
		if ((strcasecmp (prefix, "main") != 0)
		    && (aux->ok_geometry_columns == 1))
		  {
		      /* Only if TABLE 'geometry_columns' exist [inclusave RasterLite2] */
		      if (aux->is_raster_coverage_entry == 1)
			{
			    /* is a raster converage entry */
			    aux->error_message =
				sqlite3_mprintf
				("forbidden: Raster-Coverage not in the MAIN DB [%s.%s]",
				 prefix, table);
			    goto error_free_aux_memory;
			}
		      else
			{
			    /*/ checking if a GeoTable not in the MAIN DB */
			    if (do_check_geotable (sqlite, prefix, table))
			      {
				  /* if the old-table name (in a ATTACHed database) exists, then this is an error */
				  aux->error_message =
				      sqlite3_mprintf
				      ("forbidden: Spatial Table not in the MAIN DB [%s.%s]",
				       prefix, table);
				  goto error_free_aux_memory;
			      }
			}
		  }
	    }
	  if (check_spatialite_table (table))
	    {
		/* if the old-table name is a system-table, then this is an error [does NOT check if TABLE exists] */
		aux->error_message =
		    sqlite3_mprintf
		    ("forbidden: SpatiaLite internal Table [%s.%s]", prefix,
		     table);
		goto error_free_aux_memory;
	    }
	  if (aux->metadata_version == 2)
	    {
		/* DROPing or Renaming Fdo Vector TABLE allowed */
	    }
	  if (aux->ok_gpkg_contents == 1)
	    {
		/* DROPing or Renaming GeoPackage Vector/Raster internal not supported */
		if (check_gpkg_table (table))
		  {
		      /* if the old-table name is a system-table, then this is an error [does NOT check if TABLE exists] */
		      aux->error_message =
			  sqlite3_mprintf
			  ("forbidden: GeoPackage internal Table [%s.%s]",
			   prefix, table);
		      goto error_free_aux_memory;
		  }
		if ((aux->command_type == 0) || (aux->command_type == 1))
		  {
		      /* checking if a GeoPackage-Vector/Raster System-Table is being called */
		      aux->gpkg_table_type =
			  do_check_gpkg_table_type (sqlite, prefix, table);
		      /* if the result is 0, this is not GeoPackage-Table */
		      if ((aux->gpkg_table_type == 1)
			  || (aux->gpkg_table_type == 2))
			{
			    /* Renaming a non Geometry column in a GeoPackage vector/raster TABLE [1/2] is allowed */
			    if (aux->gpkg_table_type == 1)
			      {
				  /* DROP or Renaming  for GeoPackage vector/raster TABLE [1] is supported */
			      }
			    if (aux->gpkg_table_type == 2)
			      {
				  /* DROP or Renaming for GeoPackage raster TABLE [2] is supported */
			      }
			}
		  }
	    }
	  if (aux->has_topologies == 1)
	    {
		/*
		   // Only if TABLE 'topologies' exist
		   // avoiding to drop/rename TopoGeo or TopoNet tables 
		 */
		if (check_topology_table (sqlite, prefix, table))
		  {
		      /* if the old-table name exists and is a topology-table, then this is an error */
		      aux->error_message =
			  sqlite3_mprintf
			  ("forbidden: Topology internal Table [%s.%s]", prefix,
			   table);
		      goto error_free_aux_memory;
		  }
	    }
	  if (aux->table_type == 0)
	    {
		/*
		   // R*Tree internal tables are of type 'table' [0]
		   // avoiding to drop/rename R*Tree internal tables [Spatialite: 'idx_'] 
		 */
		if (check_rtree_internal_table (sqlite, prefix, table, 0))
		  {
		      /* if the old-table name exists and is a Spatialite-R*Tree-table, then this is an error */
		      aux->error_message =
			  sqlite3_mprintf
			  ("forbidden: R*Tree (Spatial Index) internal Table [%s.%s]",
			   prefix, table);
		      goto error_free_aux_memory;
		  }
		/* avoiding to drop/rename R*Tree internal tables [GeoPackage: 'rtree_'] */
		if (check_rtree_internal_table (sqlite, prefix, table, 1))
		  {
		      /* if the old-table name exists and is a GeoPackage-R*Tree-table, then this is an error */
		      aux->error_message =
			  sqlite3_mprintf
			  ("forbidden: R*Tree (GeoPackage-Spatial Index) internal Table [%s.%s]",
			   prefix, table);
		      goto error_free_aux_memory;
		  }
	    }
	  if ((aux->command_type == 2)
	      && ((aux->table_type == 0) || (aux->table_type == 1)))
	    {
		/* Only tables/views supported will be queried */
		int gpkg_table_type =
		    do_check_geometry_column (sqlite, prefix, table, column,
					      aux);
		if (aux->ok_gpkg_geometry_columns == 1)
		  {
		      /* The gpkg_geometry_columns TABLE exists, check if Column  is registered in gpkg_geometry_columns, renaming GeoPackage Geometry column is supported */
		      gpkg_table_type =
			  do_check_gpkg_table_type (sqlite, prefix, table);
		      /* if the result is 0, this is not GeoPackage-Table and the column may not exist */
		      if ((gpkg_table_type == 1) || (gpkg_table_type == 2))
			{
			    /* Renaming a non Geometry column in a GeoPackage vector TABLE [1] is allowed */
			    if (gpkg_table_type == 2)
			      {
				  /* prevent any column renaming for GeoPackage raster TABLE [2] */
				  aux->error_message =
				      sqlite3_mprintf
				      ("forbidden: GeoPackage Raster column [%s.%s] %s",
				       prefix, table, column);
				  goto error_free_aux_memory;
			      }
			}
		  }
		if (aux->metadata_version == 2)
		  {
		      /* Renaming a Fdo Geometry column is supported */
		  }
		if (aux->is_geometry_column == 0)
		  {
		      /* not a geometry column, checking if the column exists [spatialite, GeoPackage, Fdo] */
		      if (!do_check_existing_column
			  (sqlite, prefix, table, column))
			{
			    /* if the old-column name does NOT exist, then this is an error */
			    aux->error_message =
				sqlite3_mprintf
				("not existing column [%s.%s] %s", prefix,
				 table, column);
			    goto error_free_aux_memory;
			}
		  }
	    }
	  return 1;
      }
  error_free_aux_memory:
    if ((aux->n_rtrees > 0) && (aux->rtrees))
      {
	  /* cleanup any allocated memory on an error condition */
	  int i;
	  for (i = 0; i < aux->n_rtrees; i++)
	    {
		if (*(aux->rtrees + i) != NULL)
		  {
		      free (*(aux->rtrees + i));
		  }
	    }
	  free (aux->rtrees);
	  aux->rtrees = NULL;
	  aux->n_rtrees = 0;
      }
    return 0;			/* misuse of function [no pointer to struct table_params] */
}

SPATIALITE_DECLARE int
gaiaDropTable (sqlite3 * sqlite, const char *table)
{
    return gaiaDropTableEx (sqlite, "main", table);
}

SPATIALITE_DECLARE int
gaiaDropTableEx (sqlite3 * sqlite, const char *prefix, const char *table)
{
    return gaiaDropTableEx2 (sqlite, prefix, table, 1);
}

SPATIALITE_DECLARE int
gaiaDropTableEx2 (sqlite3 * sqlite, const char *prefix, const char *table,
		  int transaction)
{
    return gaiaDropTableEx3 (sqlite, prefix, table, transaction, NULL);
}

SPATIALITE_DECLARE int
gaiaDropTableEx3 (sqlite3 * sqlite, const char *prefix, const char *table,
		  int transaction, char **error_message)
{
/*
/ DEPRECATED !!!!
/ use gaiaDropTable5() as a full replacement
/
/ dropping a Spatial Table and any other related stuff 
*/
    int ret;
    /* initializing done during check_for_system_tables call to check_table_layout, retaining command_type value */
    struct table_params aux;
    aux.command_type = 0;

    if (error_message != NULL)
	*error_message = NULL;
    if (prefix == NULL)
	return 0;
    if (table == NULL)
	return 0;

    if (transaction)
      {
	  /* the whole operation is a single transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      return 0;
      }

    if (!check_for_system_tables (sqlite, prefix, table, NULL, &aux))
      {
	  goto rollback;
      }
    if (aux.is_raster_coverage_entry == 1)
      {
	  /* the table name is a verified raster_coverage entry */
	  if (!do_drop_raster_coverage (sqlite, prefix, table, &aux))
	    {
		if (aux.error_message)
		  {
		      if (error_message)
			{
			    *error_message =
				sqlite3_mprintf ("%s", aux.error_message);
			}
		      sqlite3_free (aux.error_message);
		      aux.error_message = NULL;
		  }
		goto rollback;
	    }
	  return 1;
      }
/* recursively dropping any depending View */
    if (!do_drop_sub_view (sqlite, prefix, table, &aux))
	goto rollback;
    if (!do_drop_table (sqlite, prefix, table, &aux))
	goto rollback;

    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto rollback;
      }

    if (aux.rtrees)
      {
	  /* memory cleanup - rtrees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		    free (*(aux.rtrees + i));
	    }
	  free (aux.rtrees);
      }
    return 1;

  rollback:
    if (transaction)
      {
	  /* invalidating the still pending transaction */
	  sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
      }

    if (aux.rtrees)
      {
	  /* memory cleanup - rtrees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		    free (*(aux.rtrees + i));
	    }
	  free (aux.rtrees);
      }
    if (aux.error_message != NULL)
      {
	  if (error_message != NULL)
	      *error_message = aux.error_message;
	  else
	    {
		spatialite_e ("DropGeoTable error: %s\r", aux.error_message);
		sqlite3_free (aux.error_message);
	    }
      }
    return 0;
}

SPATIALITE_DECLARE int
gaiaDropTable5 (sqlite3 * sqlite,
		const char *prefix, const char *table, char **error_message)
{
/* dropping a Spatial Table and any other related stuff */
    int ret;
/* initializing done during check_for_system_tables call to check_table_layout, retaining command_type value */
    struct table_params aux;
    aux.command_type = 0;

    if (error_message != NULL)
	*error_message = NULL;

    if (prefix == NULL)
	prefix = "main";
    if (table == NULL)
	goto invalid_arg;

    if (!check_for_system_tables (sqlite, prefix, table, NULL, &aux))
      {
	  if (aux.error_message)
	    {
		if (error_message)
		  {
		      *error_message =
			  sqlite3_mprintf ("%s", aux.error_message);
		  }
		sqlite3_free (aux.error_message);
		aux.error_message = NULL;
	    }
	  return 0;
      }

/* the whole operation is a single transaction */
    ret = sqlite3_exec (sqlite, "SAVEPOINT drop_table", NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message = sqlite3_mprintf ("unable to set a SAVEPOINT");
	  return 0;
      }
    if (aux.is_raster_coverage_entry == 1)
      {
	  /* the table name is a verified raster_coverage entry */
	  if (!do_drop_raster_coverage (sqlite, prefix, table, &aux))
	    {
		if (aux.error_message)
		  {
		      if (error_message)
			{
			    *error_message =
				sqlite3_mprintf ("%s", aux.error_message);
			}
		      sqlite3_free (aux.error_message);
		      aux.error_message = NULL;
		  }
		goto rollback;
	    }
	  goto savepoint;
      }
    if (!do_drop_table5 (sqlite, prefix, table, &aux, error_message))
	goto rollback;

    if (aux.rtrees)
      {
	  /* dropping old R*Trees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		  {
		      if (!do_drop_rtree
			  (sqlite, prefix, *(aux.rtrees + i), error_message))
			  goto rollback;
		  }
	    }
      }
    if (aux.rtrees)
      {
	  /* memory cleanup - rtrees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		    free (*(aux.rtrees + i));
	    }
	  free (aux.rtrees);
      }

  savepoint:
/* confirming the transaction */
    ret =
	sqlite3_exec (sqlite, "RELEASE SAVEPOINT drop_table", NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message =
		  sqlite3_mprintf ("unable to RELEASE the SAVEPOINT");
	  return 0;
      }
    return 1;

  invalid_arg:
    if (error_message)
	*error_message = sqlite3_mprintf ("invalid argument.");
    return 0;

  rollback:
    if (aux.rtrees)
      {
	  /* memory cleanup - rtrees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		    free (*(aux.rtrees + i));
	    }
	  free (aux.rtrees);
      }
    sqlite3_exec (sqlite, "ROLLBACK TO SAVEPOINT drop_table", NULL, NULL, NULL);
    sqlite3_exec (sqlite, "RELEASE SAVEPOINT drop_table", NULL, NULL, NULL);
    return 0;
}

SPATIALITE_DECLARE int
gaiaRenameTable (sqlite3 * sqlite,
		 const char *prefix,
		 const char *old_name,
		 const char *new_name, char **error_message)
{
/* renaming a Spatial Table and any other related stuff */
    int ret;
/* initializing done during check_for_system_tables call to check_table_layout, retaining command_type value */
    struct table_params aux;
    aux.command_type = 1;
    int fk_on = 1;
    char **results;
    int rows;
    int columns;
    int i;

    if (error_message != NULL)
	*error_message = NULL;

/* checking the version of SQLite */
    if (sqlite3_libversion_number () < 3025000)
      {
	  if (error_message)
	      *error_message =
		  sqlite3_mprintf
		  ("libsqlite 3.25 or later is strictly required");
	  return 0;
      }
    if (prefix == NULL)
	prefix = "main";
    if (old_name == NULL)
	goto invalid_arg;
    if (new_name == NULL)
	goto invalid_arg;

    if (!check_for_system_tables (sqlite, prefix, old_name, NULL, &aux))
      {
	  if (aux.error_message)
	    {
		if (error_message)
		  {
		      *error_message =
			  sqlite3_mprintf ("%s", aux.error_message);
		  }
		sqlite3_free (aux.error_message);
		aux.error_message = NULL;
	    }
	  return 0;
      }
    else
      {
	  /* The source TABLE exists and fulfills all other conditions. check if new_name exists */
	  aux.command_type = 10;	/* check_table_layout will not be called again */
	  if (!check_for_system_tables (sqlite, prefix, new_name, NULL, &aux))
	    {
		if (aux.error_message)
		  {
		      if (error_message)
			{
			    *error_message =
				sqlite3_mprintf ("%s", aux.error_message);
			}
		      sqlite3_free (aux.error_message);
		      aux.error_message = NULL;
		  }
		return 0;
	    }
      }
/* saving the current FKs mode */
    ret =
	sqlite3_get_table (sqlite, "PRAGMA foreign_keys", &results, &rows,
			   &columns, NULL);
    if (ret == SQLITE_OK)
      {
	  if (rows < 1)
	      ;
	  else
	    {
		for (i = 1; i <= rows; i++)
		    fk_on = atoi (results[(i * columns) + 0]);
	    }
	  sqlite3_free_table (results);
      }
    if (fk_on)
      {
	  /* disabling FKs constraints */
	  ret =
	      sqlite3_exec (sqlite, "PRAGMA foreign_keys = 0", NULL, NULL,
			    NULL);
	  if (ret != SQLITE_OK)
	    {
		if (error_message)
		    *error_message =
			sqlite3_mprintf ("unable to disable FKs constraints");
		return 0;
	    }
      }

/* starting a transaction */
    ret = sqlite3_exec (sqlite, "SAVEPOINT rename_table_pre", NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message = sqlite3_mprintf ("unable to set a SAVEPOINT");
	  return 0;
      }
    if (aux.is_raster_coverage_entry == 1)
      {
	  /* the old_name is a verified raster_coverage entry, the new_name does not exist in raster_coverage */
	  if (!do_rename_raster_coverage
	      (sqlite, prefix, old_name, new_name, &aux))
	    {
		if (aux.error_message)
		  {
		      if (error_message)
			{
			    *error_message =
				sqlite3_mprintf ("%s", aux.error_message);
			}
		      sqlite3_free (aux.error_message);
		      aux.error_message = NULL;
		  }
		goto rollback;
	    }
	  goto savepoint;
      }
    if (!do_rename_table_pre
	(sqlite, prefix, old_name, new_name, &aux, error_message))
	goto rollback;

    if (aux.rtrees)
      {
	  /* dropping old R*Trees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		  {
		      if (!do_drop_rtree
			  (sqlite, prefix, *(aux.rtrees + i), error_message))
			  goto rollback;
		  }
	    }
      }
    if (aux.rtrees)
      {
	  /* memory cleanup - rtrees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		    free (*(aux.rtrees + i));
	    }
	  free (aux.rtrees);
      }

  savepoint:
/* confirming the transaction */
    ret =
	sqlite3_exec (sqlite, "RELEASE SAVEPOINT rename_table_pre", NULL, NULL,
		      NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message =
		  sqlite3_mprintf ("unable to RELEASE the SAVEPOINT");
	  return 0;
      }

    if (fk_on)
      {
	  /* re-enabling FKs constraints */
	  ret =
	      sqlite3_exec (sqlite, "PRAGMA foreign_keys = 1", NULL, NULL,
			    NULL);
	  if (ret != SQLITE_OK)
	    {
		if (error_message)
		    *error_message =
			sqlite3_mprintf ("unable to re-enable FKs constraints");
		return 0;
	    }
      }
    if (aux.is_raster_coverage_entry == 1)
      {
	  /* the table name is a verified raster_coverage entry */
	  return 1;
      }
/* and finally renaming the table itself */
    ret =
	sqlite3_exec (sqlite, "SAVEPOINT rename_table_post", NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message = sqlite3_mprintf ("unable to set a SAVEPOINT");
	  return 0;
      }
    if (!do_rename_table_post
	(sqlite, prefix, old_name, new_name, &aux, error_message))
	goto rollback_post;
    ret =
	sqlite3_exec (sqlite, "RELEASE SAVEPOINT rename_table_post", NULL, NULL,
		      NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message =
		  sqlite3_mprintf ("unable to RELEASE the SAVEPOINT");
	  return 0;
      }
    return 1;

  invalid_arg:
    if (error_message)
	*error_message = sqlite3_mprintf ("invalid argument.");
    return 0;

  rollback:
    if (aux.rtrees)
      {
	  /* memory cleanup - rtrees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		    free (*(aux.rtrees + i));
	    }
	  free (aux.rtrees);
      }
    sqlite3_exec (sqlite, "ROLLBACK TO SAVEPOINT rename_table_pre", NULL,
		  NULL, NULL);
    sqlite3_exec (sqlite, "RELEASE SAVEPOINT rename_table_pre", NULL, NULL,
		  NULL);
    return 0;

  rollback_post:
    sqlite3_exec (sqlite, "ROLLBACK TO SAVEPOINT rename_table_post", NULL,
		  NULL, NULL);
    sqlite3_exec (sqlite, "RELEASE SAVEPOINT rename_table_post", NULL, NULL,
		  NULL);
    return 0;
}

SPATIALITE_DECLARE int
gaiaRenameColumn (sqlite3 * sqlite,
		  const char *prefix,
		  const char *table,
		  const char *old_name,
		  const char *new_name, char **error_message)
{
/* renaming a Spatial Table's Column and any other related stuff */
    int ret;
/* initializing done during check_for_system_tables call to check_table_layout, retaining command_type value */
    struct table_params aux;
    aux.command_type = 2;
    int fk_on = 1;
    char **results;
    int rows;
    int columns;
    int i;

    if (error_message != NULL)
	*error_message = NULL;

/* checking the version of SQLite */
    if (sqlite3_libversion_number () < 3025000)
      {
	  if (error_message)
	      *error_message =
		  sqlite3_mprintf
		  ("libsqlite 3.25 or later is strictly required");
	  return 0;
      }
    if (prefix == NULL)
	prefix = "main";
    if (old_name == NULL)
	goto invalid_arg;
    if (new_name == NULL)
	goto invalid_arg;

    if (!check_for_system_tables (sqlite, prefix, table, old_name, &aux))
      {
	  if (aux.error_message)
	    {
		if (error_message)
		  {
		      *error_message =
			  sqlite3_mprintf ("%s", aux.error_message);
		  }
		sqlite3_free (aux.error_message);
		aux.error_message = NULL;
	    }
	  return 0;
      }
    else
      {
	  /* The source TABLE and COLUMN exists and fulfills all other conditions. check if new_name exists */
	  aux.command_type = 20;	/* check_table_layout will not be called again */
	  if (!check_for_system_tables (sqlite, prefix, table, new_name, &aux))
	    {
		if (aux.error_message)
		  {
		      if (error_message)
			{
			    *error_message =
				sqlite3_mprintf ("%s", aux.error_message);
			}
		      sqlite3_free (aux.error_message);
		      aux.error_message = NULL;
		  }
		return 0;
	    }
      }
    if (aux.is_raster_coverage_entry == 1)
      {
	  /* the table name is a verified raster_coverage entry */
	  return 0;		/* should never happen [check_for_system_tables returns as error] */
      }

/* saving the current FKs mode */
    ret =
	sqlite3_get_table (sqlite, "PRAGMA foreign_keys", &results, &rows,
			   &columns, NULL);
    if (ret == SQLITE_OK)
      {
	  if (rows < 1)
	      ;
	  else
	    {
		for (i = 1; i <= rows; i++)
		    fk_on = atoi (results[(i * columns) + 0]);
	    }
	  sqlite3_free_table (results);
      }
    if (fk_on)
      {
	  /* disabling FKs constraints */
	  ret =
	      sqlite3_exec (sqlite, "PRAGMA foreign_keys = 0", NULL, NULL,
			    NULL);
	  if (ret != SQLITE_OK)
	    {
		if (error_message)
		    *error_message =
			sqlite3_mprintf ("unable to disable FKs constraints");
		return 0;
	    }
      }

/* the whole operation is a single transaction */
    ret =
	sqlite3_exec (sqlite, "SAVEPOINT rename_column_pre", NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message = sqlite3_mprintf ("unable to set a SAVEPOINT");
	  return 0;
      }
    if (!do_rename_column_pre
	(sqlite, prefix, table, old_name, new_name, &aux, error_message))
	goto rollback;

    if ((aux.rtrees) && (aux.is_geometry_column == 1))
      {
	  /* dropping old R*Trees, only when renaming a geometry-column */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		  {
		      if (!do_drop_rtree
			  (sqlite, prefix, *(aux.rtrees + i), error_message))
			  goto rollback;
		  }
	    }
      }
    if (aux.rtrees)
      {
	  /* memory cleanup - rtrees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		    free (*(aux.rtrees + i));
	    }
	  free (aux.rtrees);
      }

/* confirming the transaction */
    ret =
	sqlite3_exec (sqlite, "RELEASE SAVEPOINT rename_column_pre", NULL, NULL,
		      NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message =
		  sqlite3_mprintf ("unable to RELEASE the SAVEPOINT");
	  return 0;
      }

    if (fk_on)
      {
	  /* re-enabling FKs constraints */
	  ret =
	      sqlite3_exec (sqlite, "PRAGMA foreign_keys = 1", NULL, NULL,
			    NULL);
	  if (ret != SQLITE_OK)
	    {
		if (error_message)
		    *error_message =
			sqlite3_mprintf ("unable to re-enable FKs constraints");
		return 0;
	    }
      }

/* renaming the column itself */
    ret =
	sqlite3_exec (sqlite, "SAVEPOINT rename_column_post", NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message = sqlite3_mprintf ("unable to set a SAVEPOINT");
	  return 0;
      }
    if (!do_rename_column_post
	(sqlite, prefix, table, old_name, new_name, &aux, error_message))
	goto rollback_post;
    ret =
	sqlite3_exec (sqlite, "RELEASE SAVEPOINT rename_column_post", NULL,
		      NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  if (error_message)
	      *error_message =
		  sqlite3_mprintf ("unable to RELEASE the SAVEPOINT");
	  return 0;
      }
    return 1;

  invalid_arg:
    if (error_message)
	*error_message = sqlite3_mprintf ("invalid argument.");
    return 0;

  rollback:
    if (aux.rtrees)
      {
	  /* memory cleanup - rtrees */
	  int i;
	  for (i = 0; i < aux.n_rtrees; i++)
	    {
		if (*(aux.rtrees + i) != NULL)
		    free (*(aux.rtrees + i));
	    }
	  free (aux.rtrees);
      }
    sqlite3_exec (sqlite, "ROLLBACK TO SAVEPOINT rename_column_pre", NULL,
		  NULL, NULL);
    sqlite3_exec (sqlite, "RELEASE SAVEPOINT rename_column_pre", NULL, NULL,
		  NULL);
    return 0;

  rollback_post:
    sqlite3_exec (sqlite, "ROLLBACK TO SAVEPOINT rename_column_post", NULL,
		  NULL, NULL);
    sqlite3_exec (sqlite, "RELEASE SAVEPOINT rename_column_post", NULL, NULL,
		  NULL);
    return 0;
}
