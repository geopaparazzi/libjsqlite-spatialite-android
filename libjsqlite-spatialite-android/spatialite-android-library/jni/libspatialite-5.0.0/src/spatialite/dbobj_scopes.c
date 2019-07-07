/*

 dbobj_scopes.c -- DB objects scopes

 version 5.0, 2018 May 25

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2018
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

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite.h>
#include <spatialite_private.h>
#include <spatialite/gaiaaux.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#endif /* not WIN32 */

static int
scope_is_spatial_table (sqlite3 * sqlite, const char *db_prefix,
			const char *tbl_name)
{
/* testing for a possible Spatial Table */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM \"%s\".geometry_columns "
			   "WHERE Lower(f_table_name) = Lower(%Q)", xprefix,
			   tbl_name);
    free (xprefix);
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
		if (atoi (results[(i * columns) + 0]) > 0)
		    ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_topology (sqlite3 * sqlite, const char *db_prefix,
		   const char *tbl_name)
{
/* testing for a possible Topology */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT topology_name FROM \"%s\".topologies",
			 xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("%s_face", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_node", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_edge", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_seeds", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_topolayers", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_topofeatures", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_topology_view (sqlite3 * sqlite, const char *db_prefix,
			const char *tbl_name)
{
/* testing for a possible Topology View */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT topology_name FROM \"%s\".topologies",
			 xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("%s_edge_seeds", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_face_seeds", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_face_geoms", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_network (sqlite3 * sqlite, const char *db_prefix, const char *tbl_name)
{
/* testing for a possible Network */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql = sqlite3_mprintf ("SELECT network_name FROM \"%s\".networks", xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("%s_face", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_node", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_link", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_seeds", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_iso_metadata (const char *tbl_name)
{
/* testing for a possible ISO Metadata */
    if (strcasecmp (tbl_name, "ISO_metadata") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_reference") == 0)
	return 1;
    return 0;
}

static int
scope_is_raster_coverage (sqlite3 * sqlite, const char *db_prefix,
			  const char *tbl_name)
{
/* testing for a possible Raster Coverage */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT coverage_name FROM \"%s\".raster_coverages",
			 xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("%s_levels", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_sections", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_tile_data", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_tiles", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_raster_coverage_spatial_index (sqlite3 * sqlite, const char *db_prefix,
					const char *tbl_name, int *sys)
{
/* testing for a possible Raster Coverage Spatial Index */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    *sys = 0;
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT coverage_name FROM \"%s\".raster_coverages ",
			 xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("idx_%s_sections_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("idx_%s_sections_geometry_rowid", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = -1;
		      goto done;
		  }
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("idx_%s_sections_geometry_node", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = -1;
		      goto done;
		  }
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("idx_%s_sections_geometry_parent", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = -1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_tiles_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_tiles_geometry_rowid", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = -1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_tiles_geometry_node", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = -1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_tiles_geometry_parent", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = -1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    if (ok < 0)
	*sys = 1;
    return ok;
}

static int
scope_is_topology_index (sqlite3 * sqlite, const char *db_prefix,
			 const char *tbl_name)
{
/* testing for a possible Topology Index */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT topology_name FROM \"%s\".topologies",
			 xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("idx_%s_node_contface", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_start_node", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_end_node", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_edge_leftface", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_edge_rightface", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_timestamp", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_sdedge", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_sdface", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_seeds_timestamp", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_ftnode", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_ftedge", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_ftface", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_fttopolayers", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_topology_trigger (sqlite3 * sqlite, const char *db_prefix,
			   const char *tbl_name)
{
/* testing for a possible Topology Trigger */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT topology_name FROM \"%s\".topologies",
			 xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("tmd_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmd_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmd_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmd_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmi_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmi_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmi_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmi_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmu_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmu_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmu_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmu_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggu_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggu_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggu_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggu_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gid_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gid_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gid_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gid_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("giu_%s_edge_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("giu_%s_face_mbr", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("giu_%s_node_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("giu_%s_seeds_geom", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_edge_next_ins", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_edge_update", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_edge_next_upd", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_seeds_ins", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_seeds_update", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_topolayer_name_insert", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_topolayer_name_update", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_network_index (sqlite3 * sqlite, const char *db_prefix,
			const char *tbl_name)
{
/* testing for a possible Network Index */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql = sqlite3_mprintf ("SELECT network_name FROM \"%s\".networks", xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("idx_%s_start_node", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_end_node", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_timestamp", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_link", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_seeds_timestamp", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_network_trigger (sqlite3 * sqlite, const char *db_prefix,
			  const char *tbl_name)
{
/* testing for a possible Network Trigger */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql = sqlite3_mprintf ("SELECT network_name FROM \"%s\".networks", xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("tmd_%s_link_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmd_%s_node_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmd_%s_seeds_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmi_%s_link_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmi_%s_node_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmi_%s_seeds_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmu_%s_link_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmu_%s_node_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("tmu_%s_seeds_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_link_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_node_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggi_%s_seeds_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggu_%s_link_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggu_%s_node_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("ggu_%s_seeds_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_link_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_node_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gii_%s_seeds_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("giu_%s_link_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("giu_%s_node_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("giu_%s_seeds_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gid_%s_link_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gid_%s_node_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("gid_%s_seeds_geometry", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_node_next_ins", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_node_next_upd", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_link_next_ins", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_link_update", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_link_next_upd", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_seeds_ins", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_seeds_update", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_raster_coverage_index (sqlite3 * sqlite, const char *db_prefix,
				const char *tbl_name)
{
/* testing for a possible Raster Coverage Index */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT coverage_name FROM \"%s\".raster_coverages",
			 xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("idx_%s_sect_name", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_sect_md5", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_tiles_sect", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("idx_%s_tiles_lev", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_topology_spatial_index (sqlite3 * sqlite, const char *db_prefix,
				 const char *tbl_name, int *sys)
{
/* testing for a possible Topology Spatial Index */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *table;
    int cmp;
    char *idx_name;

    *sys = 0;
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT topology_name FROM \"%s\".topologies", xprefix, tbl_name);
    free (xprefix);
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
		table = results[(i * columns) + 0];
		idx_name = sqlite3_mprintf ("idx_%s_node_geom", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_node_geom_node", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_node_geom_rowid", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_node_geom_parent", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_edge_geom", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_edge_geom_node", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_edge_geom_rowid", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_edge_geom_parent", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_seeds_geom", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_seeds_geom_node", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_seeds_geom_rowid", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_seeds_geom_parent", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_face_mbr", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_face_mbr_node", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_face_mbr_rowid", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_face_mbr_parent", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
	    }
      }
    sqlite3_free_table (results);
    if (ok < 0)
	*sys = 1;
    return ok;
}

static int
scope_is_network_spatial_index (sqlite3 * sqlite, const char *db_prefix,
				const char *tbl_name, int *sys)
{
/* testing for a possible Network Spatial Index */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *table;
    int cmp;
    char *idx_name;

    *sys = 0;
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT network_name FROM \"%s\".networks", xprefix, tbl_name);
    free (xprefix);
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
		table = results[(i * columns) + 0];
		idx_name = sqlite3_mprintf ("idx_%s_node_geometry", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_node_geometry_node", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name =
		    sqlite3_mprintf ("idx_%s_node_geometry_rowid", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name =
		    sqlite3_mprintf ("idx_%s_node_geometry_parent", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_link_geometry", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_link_geometry_node", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name =
		    sqlite3_mprintf ("idx_%s_link_geometry_rowid", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name =
		    sqlite3_mprintf ("idx_%s_link_geometry_parent", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_seeds_geometry", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		idx_name =
		    sqlite3_mprintf ("idx_%s_seeds_geometry_node", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name =
		    sqlite3_mprintf ("idx_%s_seeds_geometry_rowid", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name =
		    sqlite3_mprintf ("idx_%s_seeds_geometry_parent", table);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
	    }
      }
    sqlite3_free_table (results);
    if (ok < 0)
	*sys = 1;
    return ok;
}

static int
scope_is_spatial_index (sqlite3 * sqlite, const char *db_prefix,
			const char *tbl_name, int *sys)
{
/* testing for a possible Spatial Index */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *table;
    const char *geom;
    int cmp;
    char *idx_name;

    *sys = 0;
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT f_table_name, f_geometry_column FROM \"%s\".geometry_columns "
	 "WHERE spatial_index_enabled = 1", xprefix, tbl_name);
    free (xprefix);
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
		table = results[(i * columns) + 0];
		geom = results[(i * columns) + 1];
		idx_name = sqlite3_mprintf ("idx_%s_%s", table, geom);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_%s_rowid", table, geom);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_%s_node", table, geom);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
		idx_name = sqlite3_mprintf ("idx_%s_%s_parent", table, geom);
		cmp = strcasecmp (idx_name, tbl_name);
		sqlite3_free (idx_name);
		if (cmp == 0)
		  {
		      ok = -1;
		      break;
		  }
	    }
      }
    sqlite3_free_table (results);
    if (ok < 0)
	*sys = 1;
    return ok;
}

static int
scope_is_raster_coverage_trigger (sqlite3 * sqlite, const char *db_prefix,
				  const char *tbl_name)
{
/* testing for a possible Raster Coverage trigger */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *cvg;
    char *name;
    int cmp;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT coverage_name FROM \"%s\".raster_coverages ",
			 xprefix);
    free (xprefix);
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
		cvg = results[(i * columns) + 0];
		name = sqlite3_mprintf ("%s_tile_data_insert", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_tile_data_update", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_sections_statistics_insert", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
		name = sqlite3_mprintf ("%s_sections_statistics_update", cvg);
		cmp = strcasecmp (name, tbl_name);
		sqlite3_free (name);
		if (cmp == 0)
		  {
		      ok = 1;
		      goto done;
		  }
	    }
      }
  done:
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_geometry_trigger (sqlite3 * sqlite, const char *db_prefix,
			   const char *tbl_name)
{
/* testing for a possible Geometry Trigger */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *table;
    const char *geom;
    int cmp;
    char *trg_name;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT f_table_name, f_geometry_column FROM \"%s\".geometry_columns",
	 xprefix, tbl_name);
    free (xprefix);
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
		table = results[(i * columns) + 0];
		geom = results[(i * columns) + 1];
		trg_name = sqlite3_mprintf ("tmu_%s_%s", table, geom);
		cmp = strcasecmp (trg_name, tbl_name);
		sqlite3_free (trg_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		trg_name = sqlite3_mprintf ("tmi_%s_%s", table, geom);
		cmp = strcasecmp (trg_name, tbl_name);
		sqlite3_free (trg_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		trg_name = sqlite3_mprintf ("tmd_%s_%s", table, geom);
		cmp = strcasecmp (trg_name, tbl_name);
		sqlite3_free (trg_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		trg_name = sqlite3_mprintf ("ggi_%s_%s", table, geom);
		cmp = strcasecmp (trg_name, tbl_name);
		sqlite3_free (trg_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		trg_name = sqlite3_mprintf ("ggu_%s_%s", table, geom);
		cmp = strcasecmp (trg_name, tbl_name);
		sqlite3_free (trg_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		trg_name = sqlite3_mprintf ("gii_%s_%s", table, geom);
		cmp = strcasecmp (trg_name, tbl_name);
		sqlite3_free (trg_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		trg_name = sqlite3_mprintf ("giu_%s_%s", table, geom);
		cmp = strcasecmp (trg_name, tbl_name);
		sqlite3_free (trg_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
		trg_name = sqlite3_mprintf ("gid_%s_%s", table, geom);
		cmp = strcasecmp (trg_name, tbl_name);
		sqlite3_free (trg_name);
		if (cmp == 0)
		  {
		      ok = 1;
		      break;
		  }
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_spatial_view (sqlite3 * sqlite, const char *db_prefix,
		       const char *tbl_name)
{
/* testing for a possible Spatial View */
    int ok = 0;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM \"%s\".views_geometry_columns "
			   "WHERE Lower(view_name) = Lower(%Q)", xprefix,
			   tbl_name);
    free (xprefix);
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
		if (atoi (results[(i * columns) + 0]) > 0)
		    ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
scope_is_internal_table (const char *tbl_name, char **sys_scope)
{
/* testing for a possible internal table */
    if (strcasecmp (tbl_name, "sqlite_sequence") == 0
	|| strcasecmp (tbl_name, "sqlite_stat1") == 0
	|| strcasecmp (tbl_name, "sqlite_stat3") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("SQLite's own");
	  return 1;
      }
    if (strcasecmp (tbl_name, "geometry_columns") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Spatial Tables Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "views_geometry_columns") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Spatial Views Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "virts_geometry_columns") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Spatial Virtual Tables Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "spatial_ref_sys") == 0
	|| strcasecmp (tbl_name, "spatial_ref_sys_aux") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("CRS Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "spatialite_history") == 0
	|| strcasecmp (tbl_name, "sql_statements_log") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("SQL log");
	  return 1;
      }
    if (strcasecmp (tbl_name, "geometry_columns_statistics") == 0
	|| strcasecmp (tbl_name, "views_geometry_columns_statistics") == 0
	|| strcasecmp (tbl_name, "virts_geometry_columns_statistics") == 0
	|| strcasecmp (tbl_name, "geometry_columns_field_infos") == 0
	|| strcasecmp (tbl_name, "views_geometry_columns_field_infos") == 0
	|| strcasecmp (tbl_name, "virts_geometry_columns_field_infos") == 0
	|| strcasecmp (tbl_name, "geometry_columns_time") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Statistics");
	  return 1;
      }
    if (strcasecmp (tbl_name, "geometry_columns_auth") == 0
	|| strcasecmp (tbl_name, "views_geometry_columns_auth") == 0
	|| strcasecmp (tbl_name, "virts_geometry_columns_auth") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Reserved for future use");
	  return 1;
      }
    if (strcasecmp (tbl_name, "raster_coverages") == 0
	|| strcasecmp (tbl_name, "raster_coverages_srid") == 0
	|| strcasecmp (tbl_name, "raster_coverages_keyword") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Raster Coverages Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "vector_coverages") == 0
	|| strcasecmp (tbl_name, "vector_coverages_srid") == 0
	|| strcasecmp (tbl_name, "vector_coverages_keyword") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Vector Coverages Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "wms_getcapabilities") == 0
	|| strcasecmp (tbl_name, "wms_getmap") == 0
	|| strcasecmp (tbl_name, "wms_settings") == 0
	|| strcasecmp (tbl_name, "wms_ref_sys") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("WMS Coverages Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "data_licenses") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Raster/Vector Coverages Auxiliary");
	  return 1;
      }
    if (strcasecmp (tbl_name, "SE_external_graphics") == 0
	|| strcasecmp (tbl_name, "SE_fonts") == 0
	|| strcasecmp (tbl_name, "SE_vector_styles") == 0
	|| strcasecmp (tbl_name, "SE_raster_styles") == 0
	|| strcasecmp (tbl_name, "SE_group_styles") == 0
	|| strcasecmp (tbl_name, "SE_vector_styled_layers") == 0
	|| strcasecmp (tbl_name, "SE_vector_styled_layers") == 0
	|| strcasecmp (tbl_name, "SE_raster_styled_layers") == 0
	|| strcasecmp (tbl_name, "SE_styled_groups") == 0
	|| strcasecmp (tbl_name, "SE_styled_group_refs") == 0
	|| strcasecmp (tbl_name, "SE_styled_group_styles") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("SLD/SE Styling");
	  return 1;
      }
    if (strcasecmp (tbl_name, "topologies") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Topologies Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "networks") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Networks Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "stored_procedures") == 0
	|| strcasecmp (tbl_name, "stored_variables") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Stored Procs Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "SpatialIndex") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Spatial Index Interface");
	  return 1;
      }
    if (strcasecmp (tbl_name, "KNN") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("KNN Interface");
	  return 1;
      }
    if (strcasecmp (tbl_name, "ElementaryGeometries") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Elementary Geometries Interface");
	  return 1;
      }
    return 0;
}

static int
scope_is_internal_view (const char *tbl_name, char **sys_scope)
{
/* testing for a possible internal view */
    if (strcasecmp (tbl_name, "geom_cols_ref_sys") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("CRS Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "spatial_ref_sys_all") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Spatial Tables Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "raster_coverages_ref_sys") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Raster Coverages Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "vector_coverages_ref_sys") == 0
	|| strcasecmp (tbl_name, "vector_layers") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Vector Coverages Catalog");
	  return 1;
      }
    if (strcasecmp (tbl_name, "vector_layers_statistics") == 0 ||
	strcasecmp (tbl_name, "vector_layers_field_infos") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Statistics");
	  return 1;
      }
    if (strcasecmp (tbl_name, "vector_layers_auth") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("Reserved for future use");
	  return 1;
      }
    if (strcasecmp (tbl_name, "SE_external_graphics_view") == 0
	|| strcasecmp (tbl_name, "SE_fonts_view") == 0
	|| strcasecmp (tbl_name, "SE_vector_styles_view") == 0
	|| strcasecmp (tbl_name, "SE_raster_styles_view") == 0
	|| strcasecmp (tbl_name, "SE_vector_styled_layers_view") == 0
	|| strcasecmp (tbl_name, "SE_raster_styled_layers_view") == 0
	|| strcasecmp (tbl_name, "SE_styled_groups_view") == 0
	|| strcasecmp (tbl_name, "SE_group_styles_view") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("SLD/SE Styling");
	  return 1;
      }
    if (strcasecmp (tbl_name, "ISO_metadata_view") == 0)
      {
	  *sys_scope = sqlite3_mprintf ("ISO Metadata Component");
	  return 1;
      }
    return 0;
}

static int
scope_is_internal_index (const char *tbl_name)
{
/* testing for a possible internal index */
    if (strcasecmp (tbl_name, "idx_spatial_ref_sys") == 0
	|| strcasecmp (tbl_name, "idx_srid_geocols") == 0
	|| strcasecmp (tbl_name, "idx_viewsjoin") == 0
	|| strcasecmp (tbl_name, "idx_virtssrid") == 0)
	return 1;
    if (strcasecmp (tbl_name, "idx_vector_styles") == 0
	|| strcasecmp (tbl_name, "idx_raster_styles") == 0
	|| strcasecmp (tbl_name, "idx_sevstl_style") == 0
	|| strcasecmp (tbl_name, "idx_serstl_style") == 0)
	return 1;
    if (strcasecmp (tbl_name, "idx_ISO_metadata_ids") == 0
	|| strcasecmp (tbl_name, "idx_ISO_metadata_parents") == 0
	|| strcasecmp (tbl_name, "idx_ISO_metadata_reference_ids") == 0
	|| strcasecmp (tbl_name, "idx_ISO_metadata_reference_parents") == 0)
	return 1;
    if (strcasecmp (tbl_name, "idx_SE_styled_vgroups") == 0
	|| strcasecmp (tbl_name, "idx_SE_styled_rgroups") == 0
	|| strcasecmp (tbl_name, "idx_SE_styled_groups_paint") == 0)
	return 1;
    if (strcasecmp (tbl_name, "idx_vector_coverages") == 0
	|| strcasecmp (tbl_name, "idx_wms_getcapabilities") == 0
	|| strcasecmp (tbl_name, "idx_wms_getmap") == 0
	|| strcasecmp (tbl_name, "idx_wms_settings") == 0
	|| strcasecmp (tbl_name, "idx_wms_ref_sys") == 0)
	return 1;
    return 0;
}

static int
scope_is_internal_trigger (const char *tbl_name)
{
/* testing for a possible internal trigger */
    if (strcasecmp (tbl_name, "geometry_columns_f_table_name_insert") == 0
	|| strcasecmp (tbl_name, "geometry_columns_f_table_name_update") == 0
	|| strcasecmp (tbl_name,
		       "geometry_columns_f_geometry_column_insert") == 0
	|| strcasecmp (tbl_name,
		       "geometry_columns_f_geometry_column_update") == 0
	|| strcasecmp (tbl_name, "geometry_columns_geometry_type_insert") == 0
	|| strcasecmp (tbl_name, "geometry_columns_geometry_type_update") == 0
	|| strcasecmp (tbl_name, "geometry_columns_coord_dimension_insert") == 0
	|| strcasecmp (tbl_name,
		       "geometry_columns_coord_dimension_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "vwgc_view_name_insert") == 0
	|| strcasecmp (tbl_name, "vwgc_view_name_update") == 0
	|| strcasecmp (tbl_name, "vwgc_view_geometry_insert") == 0
	|| strcasecmp (tbl_name, "vwgc_view_geometry_update") == 0
	|| strcasecmp (tbl_name, "vwgc_view_rowid_update") == 0
	|| strcasecmp (tbl_name, "vwgc_view_rowid_insert") == 0
	|| strcasecmp (tbl_name, "vwgc_f_table_name_insert") == 0
	|| strcasecmp (tbl_name, "vwgc_f_table_name_update") == 0
	|| strcasecmp (tbl_name, "vwgc_f_geometry_column_insert") == 0
	|| strcasecmp (tbl_name, "vwgc_f_geometry_column_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "vtgc_virt_name_insert") == 0
	|| strcasecmp (tbl_name, "vtgc_virt_name_update") == 0
	|| strcasecmp (tbl_name, "vtgc_geometry_type_update") == 0
	|| strcasecmp (tbl_name, "vtgc_virt_geometry_insert") == 0
	|| strcasecmp (tbl_name, "vtgc_virt_geometry_update") == 0
	|| strcasecmp (tbl_name, "vtgc_geometry_type_insert") == 0
	|| strcasecmp (tbl_name, "vtgc_coord_dimension_insert") == 0
	|| strcasecmp (tbl_name, "vtgc_coord_dimension_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "gcs_f_table_name_insert") == 0
	|| strcasecmp (tbl_name, "gcs_f_table_name_update") == 0
	|| strcasecmp (tbl_name, "gcs_f_geometry_column_insert") == 0
	|| strcasecmp (tbl_name, "gcs_f_geometry_column_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "vwgcs_view_name_insert") == 0
	|| strcasecmp (tbl_name, "vwgcs_view_name_update") == 0
	|| strcasecmp (tbl_name, "vwgcs_view_geometry_insert") == 0
	|| strcasecmp (tbl_name, "vwgcs_view_geometry_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "vtgcs_virt_name_insert") == 0
	|| strcasecmp (tbl_name, "vtgcs_virt_name_update") == 0
	|| strcasecmp (tbl_name, "vtgcs_virt_geometry_insert") == 0
	|| strcasecmp (tbl_name, "vtgcs_virt_geometry_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "gcfi_f_table_name_insert") == 0
	|| strcasecmp (tbl_name, "gcfi_f_table_name_update") == 0
	|| strcasecmp (tbl_name, "gcfi_f_geometry_column_insert") == 0
	|| strcasecmp (tbl_name, "gcfi_f_geometry_column_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "vwgcfi_view_name_insert") == 0
	|| strcasecmp (tbl_name, "vwgcfi_view_name_update") == 0
	|| strcasecmp (tbl_name, "vwgcfi_view_geometry_insert") == 0
	|| strcasecmp (tbl_name, "vwgcfi_view_geometry_update") == 0
	|| strcasecmp (tbl_name, "vtgcfi_virt_name_insert") == 0
	|| strcasecmp (tbl_name, "vtgcfi_virt_name_update") == 0
	|| strcasecmp (tbl_name, "vtgcfi_virt_geometry_insert") == 0
	|| strcasecmp (tbl_name, "vtgcfi_virt_geometry_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "gctm_f_table_name_insert") == 0
	|| strcasecmp (tbl_name, "gctm_f_table_name_update") == 0
	|| strcasecmp (tbl_name, "gctm_f_geometry_column_insert") == 0
	|| strcasecmp (tbl_name, "gctm_f_geometry_column_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "gcau_f_table_name_insert") == 0
	|| strcasecmp (tbl_name, "gcau_f_table_name_update") == 0
	|| strcasecmp (tbl_name, "gcau_f_geometry_column_insert") == 0
	|| strcasecmp (tbl_name, "gcau_f_geometry_column_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "vwgcau_view_name_insert") == 0
	|| strcasecmp (tbl_name, "vwgcau_view_name_update") == 0
	|| strcasecmp (tbl_name, "vwgcau_view_geometry_insert") == 0
	|| strcasecmp (tbl_name, "vwgcau_view_geometry_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "vtgcau_virt_name_insert") == 0
	|| strcasecmp (tbl_name, "vtgcau_virt_name_update") == 0
	|| strcasecmp (tbl_name, "vtgcau_virt_geometry_insert") == 0
	|| strcasecmp (tbl_name, "vtgcau_virt_geometry_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "raster_coverages_name_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_name_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_sample_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_sample_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_pixel_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_pixel_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_bands_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_bands_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_compression_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_compression_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_quality_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_tilew_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_tilew_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_tileh_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_tileh_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "raster_coverages_horzres_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_horzres_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_vertres_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_vertres_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_nodata_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_nodata_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_palette_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_palette_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_statistics_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_statistics_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_monosample_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_monosample_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_monocompr_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_monocompr_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "raster_coverages_monobands_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_monobands_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_pltsample_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_pltsample_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_pltcompr_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_pltcompr_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_pltbands_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_pltbands_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_graysample_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_graysample_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_graybands_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_graybands_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_graycompr_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_graycompr_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "raster_coverages_rgbsample_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_rgbsample_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_rgbcompr_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_rgbcompr_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_rgbbands_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_rgbbands_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_multisample_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_multisample_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_multicompr_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_multicompr_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_multibands_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_multibands_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_graycompr_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_graycompr_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "raster_coverages_gridsample_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_gridsample_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_gridcompr_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_gridcompr_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_gridbands_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_gridbands_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_georef_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_georef_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_delete") == 0
	|| strcasecmp (tbl_name, "raster_coverages_srid_name_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_srid_name_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_keyword_name_insert") == 0
	|| strcasecmp (tbl_name, "raster_coverages_keyword_name_update") == 0
	|| strcasecmp (tbl_name, "raster_coverages_quality_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "topology_name_insert") == 0
	|| strcasecmp (tbl_name, "topology_name_update") == 0
	|| strcasecmp (tbl_name, "network_name_insert") == 0
	|| strcasecmp (tbl_name, "network_name_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "vector_coverages_name_insert") == 0
	|| strcasecmp (tbl_name, "vector_coverages_name_update") == 0
	|| strcasecmp (tbl_name, "vector_coverages_srid_name_insert") == 0
	|| strcasecmp (tbl_name, "vector_coverages_srid_name_update") == 0
	|| strcasecmp (tbl_name, "vector_coverages_keyword_name_insert") == 0
	|| strcasecmp (tbl_name, "vector_coverages_keyword_name_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "sextgr_mime_type_insert") == 0
	|| strcasecmp (tbl_name, "sextgr_mime_type_update") == 0
	|| strcasecmp (tbl_name, "se_font_insert1") == 0
	|| strcasecmp (tbl_name, "se_font_insert2") == 0
	|| strcasecmp (tbl_name, "se_font_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "sevector_style_insert") == 0
	|| strcasecmp (tbl_name, "sevector_style_update") == 0
	|| strcasecmp (tbl_name, "sevector_style_name_ins") == 0
	|| strcasecmp (tbl_name, "sevector_style_name_upd") == 0
	|| strcasecmp (tbl_name, "seraster_style_insert") == 0
	|| strcasecmp (tbl_name, "seraster_style_update") == 0
	|| strcasecmp (tbl_name, "seraster_style_name_ins") == 0
	|| strcasecmp (tbl_name, "seraster_style_name_upd") == 0)
	return 1;
    if (strcasecmp (tbl_name, "segroup_style_insert") == 0
	|| strcasecmp (tbl_name, "segroup_style_update") == 0
	|| strcasecmp (tbl_name, "segroup_style_name_ins") == 0
	|| strcasecmp (tbl_name, "segroup_style_name_upd") == 0)
	return 1;
    if (strcasecmp (tbl_name, "sevstl_coverage_name_insert") == 0
	|| strcasecmp (tbl_name, "sevstl_coverage_name_update") == 0
	|| strcasecmp (tbl_name, "serstl_coverage_name_insert") == 0
	|| strcasecmp (tbl_name, "serstl_coverage_name_update") == 0
	|| strcasecmp (tbl_name, "segrp_group_name_insert") == 0
	|| strcasecmp (tbl_name, "segrp_group_name_update") == 0)
	return 1;
    if (strcasecmp (tbl_name, "segrrefs_group_name_insert") == 0
	|| strcasecmp (tbl_name, "segrrefs_group_name_update") == 0
	|| strcasecmp (tbl_name, "segrrefs_vector_coverage_name_insert") == 0
	|| strcasecmp (tbl_name, "segrrefs_vector_coverage_name_update") == 0
	|| strcasecmp (tbl_name, "segrrefs_raster_coverage_name_insert") == 0
	|| strcasecmp (tbl_name, "segrrefs_raster_coverage_name_update") == 0
	|| strcasecmp (tbl_name, "segrrefs_insert_1") == 0
	|| strcasecmp (tbl_name, "segrrefs_update_1") == 0
	|| strcasecmp (tbl_name, "segrrefs_insert_2") == 0
	|| strcasecmp (tbl_name, "segrrefs_update_2") == 0)
	return 1;
    if (strcasecmp (tbl_name, "segrpstl_group_name_insert") == 0
	|| strcasecmp (tbl_name, "segrpstl_group_name_update") == 0
	|| strcasecmp (tbl_name, "storproc_ins") == 0
	|| strcasecmp (tbl_name, "storproc_upd") == 0)
	return 1;
    if (strcasecmp (tbl_name, "ISO_metadata_md_scope_insert") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_md_scope_update") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_fileIdentifier_insert") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_fileIdentifier_update") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_insert") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_update") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_reference_scope_insert") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_reference_scope_insert") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_reference_scope_update") == 0
	|| strcasecmp (tbl_name,
		       "ISO_metadata_reference_table_name_insert") == 0
	|| strcasecmp (tbl_name,
		       "ISO_metadata_reference_table_name_update") == 0
	|| strcasecmp (tbl_name,
		       "ISO_metadata_reference_row_id_value_insert") == 0
	|| strcasecmp (tbl_name,
		       "ISO_metadata_reference_row_id_value_update") == 0
	|| strcasecmp (tbl_name, "ISO_metadata_reference_timestamp_insert") == 0
	|| strcasecmp (tbl_name,
		       "ISO_metadata_reference_timestamp_update") == 0)
	return 1;
    return 0;
}

SPATIALITE_DECLARE char *
gaiaGetDbObjectScope (sqlite3 * sqlite, const char *db_prefix,
		      const char *obj_name)
{
/* attempting to determine the intended scope for a given DB object */
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;
    const char *type;
    const char *name;
    const char *xsql;
    char *scope = NULL;
    char *sys_scope;
    int sys;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql = sqlite3_mprintf ("SELECT type, name, sql FROM \"%s\".sqlite_master "
			   "WHERE Lower(name) = Lower(%Q)", xprefix, obj_name);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		type = results[(i * columns) + 0];
		name = results[(i * columns) + 1];
		xsql = results[(i * columns) + 2];
		if (strcasecmp (type, "table") == 0)
		  {
		      /* testing tables */
		      if (scope_is_internal_table (name, &sys_scope))
			{
			    scope = sqlite3_mprintf ("system: %s", sys_scope);
			    sqlite3_free (sys_scope);
			    goto done;
			}
		      if (scope_is_topology (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf ("system: Topology Component");
			    goto done;
			}
		      if (scope_is_network (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf ("system: Network Component");
			    goto done;
			}
		      if (scope_is_iso_metadata (name))
			{
			    scope =
				sqlite3_mprintf
				("system: ISO Metadata Component");
			    goto done;
			}
		      if (scope_is_raster_coverage (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf
				("system: Raster Coverage Component");
			    goto done;
			}
		      if (scope_is_spatial_table (sqlite, db_prefix, name))
			{
			    scope = sqlite3_mprintf ("userland: Spatial Table");
			    goto done;
			}
		      if (scope_is_raster_coverage_spatial_index
			  (sqlite, db_prefix, name, &sys))
			{
			    if (sys)
				scope =
				    sqlite3_mprintf
				    ("system: Raster Coverage Component (Spatial Index Component)");
			    else
				scope =
				    sqlite3_mprintf
				    ("system: Raster Coverage Component (Spatial Index)");
			    goto done;
			}
		      if (scope_is_topology_spatial_index
			  (sqlite, db_prefix, name, &sys))
			{
			    if (sys)
				scope =
				    sqlite3_mprintf
				    ("system: Topology Component (Spatial Index Component)");
			    else
				scope =
				    sqlite3_mprintf
				    ("system: Topology Component (Spatial Index)");
			    goto done;
			}
		      if (scope_is_network_spatial_index
			  (sqlite, db_prefix, name, &sys))
			{
			    if (sys)
				scope =
				    sqlite3_mprintf
				    ("system: Network Component (Spatial Index Component)");
			    else
				scope =
				    sqlite3_mprintf
				    ("system: Network Component (Spatial Index)");
			    goto done;
			}
		      if (scope_is_spatial_index
			  (sqlite, db_prefix, name, &sys))
			{
			    if (sys)
				scope =
				    sqlite3_mprintf
				    ("system: Spatial Index Component");
			    else
				scope =
				    sqlite3_mprintf ("system: Spatial Index");
			    goto done;
			}
		      /* it should be an unqualified user Table */
		      scope = sqlite3_mprintf ("userland: Table");
		      goto done;
		  }
		if (strcasecmp (type, "view") == 0)
		  {
		      /* testing views */
		      if (scope_is_internal_view (name, &sys_scope))
			{
			    scope = sqlite3_mprintf ("system: %s", sys_scope);
			    sqlite3_free (sys_scope);
			    goto done;
			}
		      if (scope_is_topology_view (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf ("system: Topology Component");
			    goto done;
			}
		      if (scope_is_spatial_view (sqlite, db_prefix, name))
			{
			    scope = sqlite3_mprintf ("userland: Spatial View");
			    goto done;
			}
		      /* it should be an unqualified user View */
		      scope = sqlite3_mprintf ("userland: View");
		      goto done;
		  }
		if (strcasecmp (type, "index") == 0)
		  {
		      /* testing indices */
		      if (xsql == NULL)
			{
			    scope = sqlite3_mprintf ("system: AutoIndex");
			    goto done;
			}
		      if (scope_is_internal_index (name))
			{
			    scope = sqlite3_mprintf ("system: Internal Index");
			    goto done;
			}
		      if (scope_is_raster_coverage_index
			  (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf
				("system: Raster Coverage Component (index)");
			    goto done;
			}
		      if (scope_is_topology_index (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf
				("system: Topology Component (index)");
			    goto done;
			}
		      if (scope_is_network_index (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf
				("system: Network Component (index)");
			    goto done;
			}
		      /* it should be an unqualified user Index */
		      scope = sqlite3_mprintf ("userland: Index");
		      goto done;
		  }
		if (strcasecmp (type, "trigger") == 0)
		  {
		      /* testing triggers */
		      if (scope_is_internal_trigger (name))
			{
			    scope =
				sqlite3_mprintf
				("system: Internal Constraints Check (trigger)");
			    goto done;
			}
		      if (scope_is_topology_trigger (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf
				("system: Topology Constraints Check (trigger)");
			    goto done;
			}
		      if (scope_is_network_trigger (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf
				("system: Network Constraints Check (trigger)");
			    goto done;
			}
		      if (scope_is_raster_coverage_trigger
			  (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf
				("system: Raster Coverage Constraints Check (trigger)");
			    goto done;
			}
		      if (scope_is_geometry_trigger (sqlite, db_prefix, name))
			{
			    scope =
				sqlite3_mprintf
				("system: Geometry Constraints Check (trigger)");
			    goto done;
			}
		      /* it should be an unqualified user Trigger */
		      scope = sqlite3_mprintf ("userland: Trigger");
		      goto done;
		  }
		/* unknown object */
		scope = sqlite3_mprintf ("unknown scope");
		goto done;
	    }
      }
  done:
    sqlite3_free_table (results);
    return scope;

  error:
    return NULL;
}
