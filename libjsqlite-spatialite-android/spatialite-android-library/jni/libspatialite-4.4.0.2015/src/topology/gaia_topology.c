/*

 gaia_topology.c -- implementation of Topology SQL functions
    
 version 4.3, 2015 July 15

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
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

/*
 
CREDITS:

this module has been completely funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale
(Topology support) 

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifdef POSTGIS_2_2		/* only if TOPOLOGY is enabled */

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaia_topology.h>
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#include <liblwgeom.h>
#include <liblwgeom_topo.h>

#include "topology_private.h"

#define GAIA_UNUSED() if (argc || argv) argc = argc;


SPATIALITE_PRIVATE void
start_topo_savepoint (const void *handle, const void *data)
{
/* starting a new SAVEPOINT */
    char *sql;
    int ret;
    char *err_msg;
    sqlite3 *sqlite = (sqlite3 *) handle;
    struct splite_internal_cache *cache = (struct splite_internal_cache *) data;
    if (sqlite == NULL || cache == NULL)
	return;

/* creating an unique SavePoint name */
    if (cache->topo_savepoint_name != NULL)
	sqlite3_free (cache->topo_savepoint_name);
    cache->topo_savepoint_name = NULL;
    cache->topo_savepoint_name =
	sqlite3_mprintf ("toposvpt%04x", cache->next_topo_savepoint);
    if (cache->next_topo_savepoint >= 0xffffffffu)
	cache->next_topo_savepoint = 0;
    else
	cache->next_topo_savepoint += 1;

/* starting a SavePoint */
    sql = sqlite3_mprintf ("SAVEPOINT %s", cache->topo_savepoint_name);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("%s - error: %s\n", sql, err_msg);
	  sqlite3_free (err_msg);
      }
    sqlite3_free (sql);
}

SPATIALITE_PRIVATE void
release_topo_savepoint (const void *handle, const void *data)
{
/* releasing the current SAVEPOINT (if any) */
    char *sql;
    int ret;
    char *err_msg;
    sqlite3 *sqlite = (sqlite3 *) handle;
    struct splite_internal_cache *cache = (struct splite_internal_cache *) data;
    if (sqlite == NULL || cache == NULL)
	return;
    if (cache->topo_savepoint_name == NULL)
	return;

/* releasing the current SavePoint */
    sql = sqlite3_mprintf ("RELEASE SAVEPOINT %s", cache->topo_savepoint_name);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("%s - error: %s\n", sql, err_msg);
	  sqlite3_free (err_msg);
      }
    sqlite3_free (sql);
    sqlite3_free (cache->topo_savepoint_name);
    cache->topo_savepoint_name = NULL;
}

SPATIALITE_PRIVATE void
rollback_topo_savepoint (const void *handle, const void *data)
{
/* rolling back the current SAVEPOINT (if any) */
    char *sql;
    int ret;
    char *err_msg;
    sqlite3 *sqlite = (sqlite3 *) handle;
    struct splite_internal_cache *cache = (struct splite_internal_cache *) data;
    if (sqlite == NULL || cache == NULL)
	return;
    if (cache->topo_savepoint_name == NULL)
	return;

/* rolling back the current SavePoint */
    sql =
	sqlite3_mprintf ("ROLLBACK TO SAVEPOINT %s",
			 cache->topo_savepoint_name);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("%s - error: %s\n", sql, err_msg);
	  sqlite3_free (err_msg);
      }
    sqlite3_free (sql);
/* releasing the current SavePoint */
    sql = sqlite3_mprintf ("RELEASE SAVEPOINT %s", cache->topo_savepoint_name);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("%s - error: %s\n", sql, err_msg);
	  sqlite3_free (err_msg);
      }
    sqlite3_free (sql);
    sqlite3_free (cache->topo_savepoint_name);
    cache->topo_savepoint_name = NULL;
}

SPATIALITE_PRIVATE void
fnctaux_GetLastTopologyException (const void *xcontext, int argc,
				  const void *xargv)
{
/* SQL function:
/ GetLastTopologyException  ( text topology-name )
/
/ returns: the more recent exception raised by given Topology
/ NULL on invalid args (or when there is no pending exception)
*/
    const char *topo_name;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
      {
	  sqlite3_result_null (context);
	  return;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }

    sqlite3_result_text (context, gaiatopo_get_last_exception (accessor), -1,
			 SQLITE_STATIC);
}

SPATIALITE_PRIVATE void
fnctaux_CreateTopology (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_InitTopoGeo ( text topology-name )
/ CreateTopology ( text topology-name )
/ CreateTopology ( text topology-name, int srid )
/ CreateTopology ( text topology-name, int srid, double tolerance )
/ CreateTopology ( text topology-name, int srid, double tolerance, 
/                  bool hasZ )
/
/ returns: 1 on success, 0 on failure
/ -1 on invalid args
*/
    int ret;
    const char *topo_name;
    int srid = -1;
    double tolerance = 0.0;
    int has_z = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (argc >= 2)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	      srid = sqlite3_value_int (argv[1]);
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[2]);
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int tol = sqlite3_value_int (argv[2]);
		tolerance = tol;
	    }
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (argc >= 4)
      {
	  if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER)
	      has_z = sqlite3_value_int (argv[3]);
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }

    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopologyCreate (sqlite, topo_name, srid, tolerance, has_z);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    sqlite3_result_int (context, ret);
}

SPATIALITE_PRIVATE void
fnctaux_DropTopology (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ DropTopology ( text topology-name )
/
/ returns: 1 on success, 0 on failure
/ -1 on invalid args
*/
    int ret;
    const char *topo_name;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GaiaTopologyAccessorPtr accessor;
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor != NULL)
	gaiaTopologyDestroy (accessor);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopologyDrop (sqlite, topo_name);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    sqlite3_result_int (context, ret);
}

static int
check_matching_srid_dims (GaiaTopologyAccessorPtr accessor, int srid, int dims)
{
/* checking for matching SRID and DIMs */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo->srid != srid)
	return 0;
    if (topo->has_z)
      {
	  if (dims == GAIA_XY_Z || dims == GAIA_XY_Z_M)
	      ;
	  else
	      return 0;
      }
    else
      {
	  if (dims == GAIA_XY_Z || dims == GAIA_XY_Z_M)
	      return 0;
      }
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_AddIsoNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddIsoNode ( text topology-name, int face_id, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 face_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	face_id = -1;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
      {
	  face_id = sqlite3_value_int64 (argv[1]);
	  if (face_id <= 0)
	      face_id = -1;
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaAddIsoNode (accessor, face_id, pt, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret <= 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_MoveIsoNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_MoveIsoNode ( text topology-name, int node_id, Geometry point )
/
/ returns: TEXT (description of new location)
/ raises an exception on failure
*/
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *topo_name;
    sqlite3_int64 node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;
    sprintf (xid, "%lld", node_id);
    newpos =
	sqlite3_mprintf ("Isolated Node %s moved to location %f,%f", xid, pt->X,
			 pt->Y);

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaMoveIsoNode (accessor, node_id, pt);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_topo:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemIsoNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemIsoNode ( text topology-name, int node_id )
/
/ returns: TEXT (description of operation)
/ raises an exception on failure
*/
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *topo_name;
    sqlite3_int64 node_id;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    sprintf (xid, "%lld", node_id);
    newpos = sqlite3_mprintf ("Isolated Node %s removed", xid);

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaRemIsoNode (accessor, node_id);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_topo:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_AddIsoEdge (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddIsoEdge ( text topology-name, int start_node_id, int end_node_id, Geometry linestring )
/
/ returns: the ID of the inserted Edge on success, 0 on failure
/ raises an exception on failure
*/
    int ret;
    const char *topo_name;
    sqlite3_int64 start_node_id;
    sqlite3_int64 end_node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	start_node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	end_node_id = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[3]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[3]);
	  n_bytes = sqlite3_value_bytes (argv[3]);
      }
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    line =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!line)
	goto invalid_arg;
    if (line->FirstPoint != NULL)
	invalid = 1;
    if (line->FirstPolygon != NULL)
	invalid = 1;
    if (line->FirstLinestring != line->LastLinestring
	|| line->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims (accessor, line->Srid, line->DimensionModel))
	goto invalid_geom;
    ln = line->FirstLinestring;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaAddIsoEdge (accessor, start_node_id, end_node_id, ln);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (line);
    line = NULL;
    if (ret <= 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemEdgeModFace (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemEdgeModFace ( text topology-name, int edge_id )
/
/ returns: ID of the Face that takes up the space previously occupied by the removed edge
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaRemEdgeModFace (accessor, edge_id);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (ret < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemEdgeNewFace (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemEdgeNewFace ( text topology-name, int edge_id )
/
/ returns: ID of the created Face 
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaRemEdgeNewFace (accessor, edge_id);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (ret < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ChangeEdgeGeom (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ChangeEdgeGeom ( text topology-name, int edge_id, Geometry linestring )
/
/ returns: TEXT (description of operation)
/ raises an exception on failure
*/
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    line =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!line)
	goto invalid_arg;
    if (line->FirstPoint != NULL)
	invalid = 1;
    if (line->FirstPolygon != NULL)
	invalid = 1;
    if (line->FirstLinestring != line->LastLinestring
	|| line->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims (accessor, line->Srid, line->DimensionModel))
	goto invalid_geom;
    ln = line->FirstLinestring;
    sprintf (xid, "%lld", edge_id);
    newpos = sqlite3_mprintf ("Edge %s changed", xid);

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaChangeEdgeGeom (accessor, edge_id, ln);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (line);
    line = NULL;
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_topo:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ModEdgeSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ModEdgeSplit ( text topology-name, int edge_id, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaModEdgeSplit (accessor, edge_id, pt, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret <= 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_NewEdgesSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_NewEdgesSplit ( text topology-name, int edge_id, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
      {
	  goto invalid_arg;
	  return;
      }
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaNewEdgesSplit (accessor, edge_id, pt, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret <= 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_AddEdgeModFace (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddEdgeModFace ( text topology-name, int start_node_id, int end_node_id, Geometry linestring )
/
/ returns: the ID of the inserted Edge on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 start_node_id;
    sqlite3_int64 end_node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	start_node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	end_node_id = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[3]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[3]);
	  n_bytes = sqlite3_value_bytes (argv[3]);
      }
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    line =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!line)
	goto invalid_arg;
    if (line->FirstPoint != NULL)
	invalid = 1;
    if (line->FirstPolygon != NULL)
	invalid = 1;
    if (line->FirstLinestring != line->LastLinestring
	|| line->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims (accessor, line->Srid, line->DimensionModel))
	goto invalid_geom;
    ln = line->FirstLinestring;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaAddEdgeModFace (accessor, start_node_id, end_node_id, ln, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (line);
    line = NULL;
    if (ret <= 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_AddEdgeNewFaces (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddEdgeNewFaces ( text topology-name, int start_node_id, int end_node_id, Geometry linestring )
/
/ returns: the ID of the inserted Edge on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 start_node_id;
    sqlite3_int64 end_node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	start_node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	end_node_id = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[3]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[3]);
	  n_bytes = sqlite3_value_bytes (argv[3]);
      }
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    line =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!line)
	goto invalid_arg;
    if (line->FirstPoint != NULL)
	invalid = 1;
    if (line->FirstPolygon != NULL)
	invalid = 1;
    if (line->FirstLinestring != line->LastLinestring
	|| line->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims (accessor, line->Srid, line->DimensionModel))
	goto invalid_geom;
    ln = line->FirstLinestring;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaAddEdgeNewFaces (accessor, start_node_id, end_node_id, ln, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (line);
    line = NULL;
    if (ret <= 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ModEdgeHeal (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ModEdgeHeal ( text topology-name, int edge_id1, int edge_id2 )
/
/ returns: ID of the removed Node
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id1;
    sqlite3_int64 edge_id2;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id1 = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	edge_id2 = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaModEdgeHeal (accessor, edge_id1, edge_id2);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (ret < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_NewEdgeHeal (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_NewEdgeHeal ( text topology-name, int edge_id1, int edge_id2 )
/
/ returns: ID of the removed Node
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id1;
    sqlite3_int64 edge_id2;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id1 = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	edge_id2 = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaNewEdgeHeal (accessor, edge_id1, edge_id2);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (ret < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetFaceGeometry (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_GetFaceGeometry ( text topology-name, int face_id )
/
/ returns: the Face's geometry (Polygon)
/ raises an exception on failure
*/
    const char *topo_name;
    sqlite3_int64 face_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
	gpkg_mode = cache->gpkg_mode;
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	face_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    geom = gaiaGetFaceGeometry (accessor, face_id);
    if (geom == NULL)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx (geom, &p_blob, &n_bytes, gpkg_mode);
    gaiaFreeGeomColl (geom);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetFaceEdges (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_GetFaceEdges ( text topology-name, int face_id )
/
/ create/update a table containing an ordered list of EdgeIDs
/
/ returns NULL on success
/ raises an exception on failure
*/
    const char *topo_name;
    sqlite3_int64 face_id;
    int ret;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	face_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaGetFaceEdges (accessor, face_id);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

static int
check_empty_topology (struct gaia_topology *topo)
{
/* checking for an empty Topology */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int already_populated = 0;

/* testing NODE */
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  if (atoi (results[(i * columns) + 0]) > 0)
	      already_populated = 1;
      }
    sqlite3_free_table (results);
    if (already_populated)
	return 0;

/* testing EDGE */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  if (atoi (results[(i * columns) + 0]) > 0)
	      already_populated = 1;
      }
    sqlite3_free_table (results);
    if (already_populated)
	return 0;

/* testing FACE */
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\" WHERE face_id <> 0",
			 xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  if (atoi (results[(i * columns) + 0]) > 0)
	      already_populated = 1;
      }
    sqlite3_free_table (results);
    if (already_populated)
	return 0;

    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_ValidateTopoGeo (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ValidateTopoGeo ( text topology-name )
/
/ create/update a table containing an validation report for a given TopoGeo
/
/ returns NULL on success
/ raises an exception on failure
*/
    const char *topo_name;
    int ret;
    GaiaTopologyAccessorPtr accessor;
    struct gaia_topology *topo;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    topo = (struct gaia_topology *) accessor;
    if (check_empty_topology (topo))
	goto empty;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaValidateTopoGeo (accessor);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  empty:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - empty topology.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetNodeByPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ GetNodeByPoint ( text topology-name, Geometry point, double tolerance )
/
/ returns: the ID of some Node on success, 0 if no Node was found
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int t = sqlite3_value_int (argv[2]);
	  tolerance = t;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_int (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    pt = point->FirstPoint;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaGetNodeByPoint (accessor, pt, tolerance);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetEdgeByPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ GetEdgeByPoint ( text topology-name, Geometry point, double tolerance )
/
/ returns: the ID of some Edge on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int t = sqlite3_value_int (argv[2]);
	  tolerance = t;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_int (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    pt = point->FirstPoint;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaGetEdgeByPoint (accessor, pt, tolerance);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetFaceByPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ GetFaceByPoint ( text topology-name, Geometry point, double tolerance )
/
/ returns: the ID of some Face on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int t = sqlite3_value_int (argv[2]);
	  tolerance = t;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_int (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    pt = point->FirstPoint;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaGetFaceByPoint (accessor, pt, tolerance);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_AddPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_AddPoint ( text topology-name, Geometry (multi)point, double tolerance )
/
/ returns: a comma separated list of all IDs of corresponding Nodes on success
/ raises an exception on failure
*/
    sqlite3_int64 node_id;
    char xnode_id[64];
    char *retlist = NULL;
    char *savelist;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int t = sqlite3_value_int (argv[2]);
	  tolerance = t;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_int (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    pt = point->FirstPoint;
    while (pt != NULL)
      {
	  /* looping on elementary Points */
	  node_id = gaiaTopoGeo_AddPoint (accessor, pt, tolerance);
	  if (node_id < 0)
	      break;
	  sprintf (xnode_id, "%lld", node_id);
	  if (retlist == NULL)
	      retlist = sqlite3_mprintf ("%s", xnode_id);
	  else
	    {
		savelist = retlist;
		retlist = sqlite3_mprintf ("%s, %s", savelist, xnode_id);
		sqlite3_free (savelist);
	    }
	  pt = pt->Next;
      }

    if (node_id < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (node_id < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  if (retlist != NULL)
	      sqlite3_free (retlist);
	  return;
      }
    sqlite3_result_text (context, retlist, strlen (retlist), sqlite3_free);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_AddLineString (const void *xcontext, int argc,
			       const void *xargv)
{
/* SQL function:
/ TopoGeo_AddLineString ( text topology-name, Geometry (multi)linestring, double tolerance )
/
/ returns: a comma separated list of all IDs of corresponding Edges on success
/ raises an exception on failure
*/
    int ret;
    char xedge_id[64];
    sqlite3_int64 *edge_ids = NULL;
    int ids_count = 0;
    char *retlist = NULL;
    char *savelist;
    int i;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr linestring = NULL;
    gaiaLinestringPtr ln;
    double tolerance;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int t = sqlite3_value_int (argv[2]);
	  tolerance = t;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_int (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    linestring =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!linestring)
	goto invalid_arg;
    if (linestring->FirstPoint != NULL)
	invalid = 1;
    if (linestring->FirstPolygon != NULL)
	invalid = 1;
    if (linestring->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims
	(accessor, linestring->Srid, linestring->DimensionModel))
	goto invalid_geom;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ln = linestring->FirstLinestring;
    while (ln != NULL)
      {
	  /* looping on individual Linestrings */
	  ret =
	      gaiaTopoGeo_AddLineString (accessor, ln, tolerance, &edge_ids,
					 &ids_count);
	  if (!ret)
	      break;
	  for (i = 0; i < ids_count; i++)
	    {
		sprintf (xedge_id, "%lld", edge_ids[i]);
		if (retlist == NULL)
		    retlist = sqlite3_mprintf ("%s", xedge_id);
		else
		  {
		      savelist = retlist;
		      retlist = sqlite3_mprintf ("%s, %s", savelist, xedge_id);
		      sqlite3_free (savelist);
		  }
	    }
	  free (edge_ids);
	  ln = ln->Next;
      }

    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (linestring);
    linestring = NULL;
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  sqlite3_free (retlist);
	  return;
      }
    sqlite3_result_text (context, retlist, strlen (retlist), sqlite3_free);
    return;

  no_topo:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_AddPolygon (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_AddPolygon ( text topology-name, Geometry (multi)polygon, double tolerance )
/
/ returns: a comma separated list of all IDs of corresponding Faces on success
/ raises an exception on failure
*/
    int ret;
    char xface_id[64];
    sqlite3_int64 *face_ids = NULL;
    int ids_count = 0;
    char *retlist = NULL;
    char *savelist;
    int i;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr polygon = NULL;
    gaiaPolygonPtr pg;
    double tolerance;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int t = sqlite3_value_int (argv[2]);
	  tolerance = t;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_int (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Polygon Geometry */
    polygon =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!polygon)
	goto invalid_arg;
    if (polygon->FirstPoint != NULL)
	invalid = 1;
    if (polygon->FirstLinestring != NULL)
	invalid = 1;
    if (polygon->FirstPolygon == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    if (!check_matching_srid_dims
	(accessor, polygon->Srid, polygon->DimensionModel))
	goto invalid_geom;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    pg = polygon->FirstPolygon;
    while (pg != NULL)
      {
	  /* looping on individual Polygons */
	  ret =
	      gaiaTopoGeo_AddPolygon (accessor, pg, tolerance, &face_ids,
				      &ids_count);
	  if (!ret)
	      break;
	  for (i = 0; i < ids_count; i++)
	    {
		sprintf (xface_id, "%lld", face_ids[i]);
		if (retlist == NULL)
		    retlist = sqlite3_mprintf ("%s", xface_id);
		else
		  {
		      savelist = retlist;
		      retlist = sqlite3_mprintf ("%s, %s", savelist, xface_id);
		      sqlite3_free (savelist);
		  }
	    }
	  free (face_ids);
	  pg = pg->Next;
      }

    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (polygon);
    polygon = NULL;
    if (ret < 0)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  sqlite3_free (retlist);
	  return;
      }
    sqlite3_result_text (context, retlist, strlen (retlist), sqlite3_free);
    return;

  no_topo:
    if (polygon != NULL)
	gaiaFreeGeomColl (polygon);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (polygon != NULL)
	gaiaFreeGeomColl (polygon);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (polygon != NULL)
	gaiaFreeGeomColl (polygon);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (polygon != NULL)
	gaiaFreeGeomColl (polygon);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_SubdivideLines (const void *xcontext, int argc,
				const void *xargv)
{
/* SQL function:
/ TopoGeo_SubdivideLines ( Geometry geom, int line_max_points,
/                          double max_length )
/
/ returns: a MultiLinestring or NULL on failure
*/
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr result;
    int line_max_points = -1;
    double max_length = -1.0;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[0]);
	  n_bytes = sqlite3_value_bytes (argv[0]);
      }
    else
	goto err;
    if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	line_max_points = sqlite3_value_int (argv[1]);
    else
	goto err;
    if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int max = sqlite3_value_int (argv[2]);
	  max_length = max;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	max_length = sqlite3_value_int (argv[2]);
    else
	goto err;

    if (line_max_points < 2)
	line_max_points = -1;

/* attempting to get a Geometry */
    geom =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!geom)
	goto err;

/* splitting the geometry */
    result = gaiaTopoGeo_SubdivideLines (geom, line_max_points, max_length);
    gaiaFreeGeomColl (geom);
    if (result == NULL)
	goto err;
    gaiaToSpatiaLiteBlobWkbEx (result, &p_blob, &n_bytes, gpkg_mode);
    gaiaFreeGeomColl (result);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  err:
    sqlite3_result_null (context);
    return;
}

static int
check_input_geo_table (sqlite3 * sqlite, const char *db_prefix,
		       const char *table, const char *column, char **xtable,
		       char **xcolumn, int *srid, int *dims)
{
/* checking if an input GeoTable do really exist */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    char *xprefix;
    int len;
    int count = 0;
    char *xx_table = NULL;
    char *xx_column = NULL;
    char *ztable;
    int xtype;
    int xdims;
    int xsrid;

    *xtable = NULL;
    *xcolumn = NULL;
    *srid = -1;
    *dims = GAIA_XY;

/* querying GEOMETRY_COLUMNS */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    if (column == NULL)
	sql =
	    sqlite3_mprintf
	    ("SELECT f_table_name, f_geometry_column, geometry_type, srid "
	     "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q)",
	     xprefix, table);
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT f_table_name, f_geometry_column, geometry_type, srid "
	     "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) AND "
	     "Lower(f_geometry_column) = Lower(%Q)", xprefix, table, column);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *table_name = results[(i * columns) + 0];
	  const char *column_name = results[(i * columns) + 1];
	  xtype = atoi (results[(i * columns) + 2]);
	  xsrid = atoi (results[(i * columns) + 3]);
	  len = strlen (table_name);
	  if (xx_table != NULL)
	      free (xx_table);
	  xx_table = malloc (len + 1);
	  strcpy (xx_table, table_name);
	  len = strlen (column_name);
	  if (xx_column != NULL)
	      free (xx_column);
	  xx_column = malloc (len + 1);
	  strcpy (xx_column, column_name);
	  count++;
      }
    sqlite3_free_table (results);

    if (count != 1)
      {
	  if (xx_table != NULL)
	      free (xx_table);
	  if (xx_column != NULL)
	      free (xx_column);
	  return 0;
      }

/* testing if the GeoTable do really exist */
    count = 0;
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    ztable = gaiaDoubleQuotedSql (xx_table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, ztable);
    free (xprefix);
    free (ztable);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *column_name = results[(i * columns) + 1];
	  if (strcasecmp (column_name, xx_column) == 0)
	      count++;
      }
    sqlite3_free_table (results);

    if (count != 1)
      {
	  if (xx_table != NULL)
	      free (xx_table);
	  if (xx_column != NULL)
	      free (xx_column);
	  return 0;
      }

    switch (xtype)
      {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
	  xdims = GAIA_XY;
	  break;
      case 1000:
      case 1001:
      case 1002:
      case 1003:
      case 1004:
      case 1005:
      case 1006:
      case 1007:
	  xdims = GAIA_XY_Z;
	  break;
      case 2000:
      case 2001:
      case 2002:
      case 2003:
      case 2004:
      case 2005:
      case 2006:
      case 2007:
	  xdims = GAIA_XY_M;
	  break;
      case 3000:
      case 3001:
      case 3002:
      case 3003:
      case 3004:
      case 3005:
      case 3006:
      case 3007:
	  xdims = GAIA_XY_Z_M;
	  break;
      };
    *xtable = xx_table;
    *xcolumn = xx_column;
    *srid = xsrid;
    *dims = xdims;
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_FromGeoTable (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_FromGeoTable ( text topology-name, text db-prefix, text table,
/                        text column, double tolerance )
/ TopoGeo_FromGeoTable ( text topology-name, text db-prefix, text table,
/                        text column, double tolerance, int line_max_points,
/                        double max_length )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *table;
    const char *column;
    char *xtable = NULL;
    char *xcolumn = NULL;
    int srid;
    int dims;
    double tolerance;
    int line_max_points = -1;
    double max_length = -1.0;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_INTEGER)
      {
	  int t = sqlite3_value_int (argv[4]);
	  tolerance = t;
      }
    else if (sqlite3_value_type (argv[4]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_int (argv[4]);
    else
	goto invalid_arg;
    if (argc >= 6)
      {
	  if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
	      line_max_points = sqlite3_value_int (argv[5]);
	  else
	      goto invalid_arg;
      }
    if (argc >= 7)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int max = sqlite3_value_int (argv[6]);
		max_length = max;
	    }
	  else if (sqlite3_value_type (argv[6]) == SQLITE_FLOAT)
	      max_length = sqlite3_value_double (argv[6]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

/* checking the input GeoTable */
    if (!check_input_geo_table
	(sqlite, db_prefix, table, column, &xtable, &xcolumn, &srid, &dims))
	goto no_input;
    if (!check_matching_srid_dims (accessor, srid, dims))
	goto invalid_geom;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_FromGeoTable (accessor, db_prefix, xtable, xcolumn,
				  tolerance, line_max_points, max_length);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xtable);
    free (xcolumn);
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  no_input:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid input GeoTable.",
			  -1);
    return;

  null_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).",
			  -1);
    return;
}

static int
do_clone_face (const char *db_prefix, const char *in_topology_name,
	       struct gaia_topology *topo_out)
{
/* cloning FACE */
    char *sql;
    char *table;
    char *xprefix;
    char *xtable;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;

/* preparing the input SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    table = sqlite3_mprintf ("%s_face", in_topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT face_id, min_x, min_y, max_x, max_y FROM \"%s\".\"%s\" WHERE face_id <> 0",
	 xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_in,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM \"face\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_face", topo_out->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (face_id, min_x, min_y, max_x, max_y) "
	 "VALUES (?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO \"face\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 1,
					sqlite3_column_int64 (stmt_in, 0));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_NULL)
		    sqlite3_bind_null (stmt_out, 2);
		else if (sqlite3_column_type (stmt_in, 1) == SQLITE_FLOAT)
		    sqlite3_bind_double (stmt_out, 2,
					 sqlite3_column_double (stmt_in, 1));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 2) == SQLITE_NULL)
		    sqlite3_bind_null (stmt_out, 3);
		else if (sqlite3_column_type (stmt_in, 2) == SQLITE_FLOAT)
		    sqlite3_bind_double (stmt_out, 3,
					 sqlite3_column_double (stmt_in, 2));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 3) == SQLITE_NULL)
		    sqlite3_bind_null (stmt_out, 4);
		else if (sqlite3_column_type (stmt_in, 3) == SQLITE_FLOAT)
		    sqlite3_bind_double (stmt_out, 4,
					 sqlite3_column_double (stmt_in, 3));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 4) == SQLITE_NULL)
		    sqlite3_bind_null (stmt_out, 5);
		else if (sqlite3_column_type (stmt_in, 4) == SQLITE_FLOAT)
		    sqlite3_bind_double (stmt_out, 5,
					 sqlite3_column_double (stmt_in, 4));
		else
		    goto invalid_value;
		/* inserting into the output table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e ("INSERT INTO \"face\" step error: \"%s\"",
				    sqlite3_errmsg (topo_out->db_handle));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("SELECT FROM \"face\" step error: %s",
			      sqlite3_errmsg (topo_out->db_handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  invalid_value:
    spatialite_e ("SELECT FROM \"face\": found an invalid value");

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
do_clone_node (const char *db_prefix, const char *in_topology_name,
	       struct gaia_topology *topo_out)
{
/* cloning NODE */
    char *sql;
    char *table;
    char *xprefix;
    char *xtable;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;

/* preparing the input SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    table = sqlite3_mprintf ("%s_node", in_topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT node_id, containing_face, geom FROM \"%s\".\"%s\"", xprefix,
	 xtable);
    free (xprefix);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_in,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM \"node\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_node", topo_out->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (node_id, containing_face, geom) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO \"node\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 1,
					sqlite3_column_int64 (stmt_in, 0));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_NULL)
		    sqlite3_bind_null (stmt_out, 2);
		else if (sqlite3_column_type (stmt_in, 1) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 2,
					sqlite3_column_int64 (stmt_in, 1));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 2) == SQLITE_BLOB)
		    sqlite3_bind_blob (stmt_out, 3,
				       sqlite3_column_blob (stmt_in, 2),
				       sqlite3_column_bytes (stmt_in, 2),
				       SQLITE_STATIC);
		else
		    goto invalid_value;
		/* inserting into the output table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e ("INSERT INTO \"node\" step error: \"%s\"",
				    sqlite3_errmsg (topo_out->db_handle));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("SELECT FROM \"node\" step error: %s",
			      sqlite3_errmsg (topo_out->db_handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  invalid_value:
    spatialite_e ("SELECT FROM \"node\": found an invalid value");

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
do_clone_edge (const char *db_prefix, const char *in_topology_name,
	       struct gaia_topology *topo_out)
{
/* cloning EDGE */
    char *sql;
    char *table;
    char *xprefix;
    char *xtable;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;

/* preparing the input SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    table = sqlite3_mprintf ("%s_edge", in_topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT edge_id, start_node, end_node, next_left_edge, "
	 "next_right_edge, left_face, right_face, geom FROM \"%s\".\"%s\"",
	 xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_in,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM \"edge\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_edge", topo_out->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (edge_id, start_node, end_node, "
	 "next_left_edge, next_right_edge, left_face, right_face, geom) "
	 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO \"edge\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 1,
					sqlite3_column_int64 (stmt_in, 0));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 2,
					sqlite3_column_int64 (stmt_in, 1));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 2) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 3,
					sqlite3_column_int64 (stmt_in, 2));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 3) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 4,
					sqlite3_column_int64 (stmt_in, 3));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 4) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 5,
					sqlite3_column_int64 (stmt_in, 4));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 5) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 6,
					sqlite3_column_int64 (stmt_in, 5));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 6) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 7,
					sqlite3_column_int64 (stmt_in, 6));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 7) == SQLITE_BLOB)
		    sqlite3_bind_blob (stmt_out, 8,
				       sqlite3_column_blob (stmt_in, 7),
				       sqlite3_column_bytes (stmt_in, 7),
				       SQLITE_STATIC);
		else
		    goto invalid_value;
		/* inserting into the output table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e ("INSERT INTO \"edge\" step error: \"%s\"",
				    sqlite3_errmsg (topo_out->db_handle));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("SELECT FROM \"edge\" step error: %s",
			      sqlite3_errmsg (topo_out->db_handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  invalid_value:
    spatialite_e ("SELECT FROM \"edge\": found an invalid value");

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
do_clone_topology (const char *db_prefix, const char *in_topology,
		   GaiaTopologyAccessorPtr accessor)
{
/* cloning a full Topology */
    struct gaia_topology *topo_out = (struct gaia_topology *) accessor;

/* cloning FACE */
    if (!do_clone_face (db_prefix, in_topology, topo_out))
	return 0;

/* cloning NODE */
    if (!do_clone_node (db_prefix, in_topology, topo_out))
	return 0;

/* cloning EDGE */
    if (!do_clone_edge (db_prefix, in_topology, topo_out))
	return 0;

    return 1;
}

static char *
gaiaGetAttachedTopology (sqlite3 * handle, const char *db_prefix,
			 const char *topo_name, int *srid, double *tolerance,
			 int *has_z)
{
/* attempting to retrieve the Input Topology for TopoGeo_Clone */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;
    char *xprefix;
    char *xtopology_name = NULL;
    int xsrid;
    double xtolerance;
    int xhas_z;

/* preparing the SQL query */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT topology_name, srid, tolerance, has_z FROM \"%s\".topologies WHERE "
	 "Lower(topology_name) = Lower(%Q)", xprefix, topo_name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM topologys error: \"%s\"\n",
			sqlite3_errmsg (handle));
	  return NULL;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_name = 0;
		int ok_srid = 0;
		int ok_tolerance = 0;
		int ok_z = 0;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      const char *str =
			  (const char *) sqlite3_column_text (stmt, 0);
		      if (xtopology_name != NULL)
			  free (xtopology_name);
		      xtopology_name = malloc (strlen (str) + 1);
		      strcpy (xtopology_name, str);
		      ok_name = 1;
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		  {
		      xsrid = sqlite3_column_int (stmt, 1);
		      ok_srid = 1;
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		  {
		      xtolerance = sqlite3_column_double (stmt, 2);
		      ok_tolerance = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_INTEGER)
		  {
		      xhas_z = sqlite3_column_int (stmt, 3);
		      ok_z = 1;
		  }
		if (ok_name && ok_srid && ok_tolerance && ok_z)
		  {
		      ok = 1;
		      break;
		  }
	    }
	  else
	    {
		spatialite_e
		    ("step: SELECT FROM topologies error: \"%s\"\n",
		     sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		return NULL;
	    }
      }
    sqlite3_finalize (stmt);

    if (ok)
      {
	  *srid = xsrid;
	  *tolerance = xtolerance;
	  *has_z = xhas_z;
	  return xtopology_name;
      }

    if (xtopology_name != NULL)
	free (xtopology_name);
    return NULL;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_Clone (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_Clone ( text db-prefix, text in-topology-name, text out-topology-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *db_prefix = "MAIN";
    const char *in_topo_name;
    const char *out_topo_name;
    char *input_topo_name = NULL;
    int srid;
    double tolerance;
    int has_z;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	in_topo_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	out_topo_name = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;

/* checking the origin Topology */
    input_topo_name =
	gaiaGetAttachedTopology (sqlite, db_prefix, in_topo_name, &srid,
				 &tolerance, &has_z);
    if (input_topo_name == NULL)
	goto no_topo;

/* attempting to create the destination Topology */
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopologyCreate (sqlite, out_topo_name, srid, tolerance, has_z);
    if (!ret)
      {
	  rollback_topo_savepoint (sqlite, cache);
	  goto no_topo2;
      }

/* attempting to get a Topology Accessor (destination) */
    accessor = gaiaGetTopology (sqlite, cache, out_topo_name);
    if (accessor == NULL)
	goto no_topo2;

/* cloning Topology */
    ret = do_clone_topology (db_prefix, input_topo_name, accessor);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  sqlite3_result_error (context, "Clone Topology failure", -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    if (input_topo_name != NULL)
	free (input_topo_name);
    return;

  no_topo:
    if (input_topo_name != NULL)
	free (input_topo_name);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name (origin).",
			  -1);
    return;

  no_topo2:
    if (input_topo_name != NULL)
	free (input_topo_name);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name (destination).",
			  -1);
    return;

  null_arg:
    if (input_topo_name != NULL)
	free (input_topo_name);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (input_topo_name != NULL)
	free (input_topo_name);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

static int
do_simplify_topology (GaiaTopologyAccessorPtr accessor, double tolerance)
{
/* simplifying Topology */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    char *sql;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *msg;

/* preparing the input SQL statement */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE MAIN.\"%s\" SET geom = ST_SimplifyPreserveTopology(geom, ?)",
	 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  msg = sqlite3_mprintf ("UPDATE \"edge\" Simplify error: \"%s\"",
				 sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* updating all Edges */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, tolerance);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  msg = sqlite3_mprintf ("UPDATE \"edge\" Simplify step error: \"%s\"",
				 sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_finalize (stmt);

    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_Simplify (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_Simplify ( text topology-name, double tolerance )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *topo_name;
    double tolerance;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
      {
	  int tol = sqlite3_value_int (argv[1]);
	  tolerance = tol;
      }
    else if (sqlite3_value_type (argv[1]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_double (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

/* simplifying all Topology Edges */
    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = do_simplify_topology (accessor, tolerance);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_GetEdgeSeed (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_GetEdgeSeed ( text topology-name, int edge_id )
/
/ returns: a Point (seed) identifying the Edge
/ raises an exception on failure
*/
    const char *topo_name;
    sqlite3_int64 edge_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
	gpkg_mode = cache->gpkg_mode;
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    geom = gaiaGetEdgeSeed (accessor, edge_id);
    if (geom == NULL)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx (geom, &p_blob, &n_bytes, gpkg_mode);
    gaiaFreeGeomColl (geom);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_GetFaceSeed (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_GetFaceSeed ( text topology-name, int face_id )
/
/ returns: a Point (seed) identifying the Edge
/ raises an exception on failure
*/
    const char *topo_name;
    sqlite3_int64 face_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    GaiaTopologyAccessorPtr accessor;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
	gpkg_mode = cache->gpkg_mode;
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	face_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    geom = gaiaGetFaceSeed (accessor, face_id);
    if (geom == NULL)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx (geom, &p_blob, &n_bytes, gpkg_mode);
    gaiaFreeGeomColl (geom);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_UpdateSeeds (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_UpdateSeeds ( text topology-name )
/ TopoGeo_UpdateSeeds ( text topology-name, int incremental_mode )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *topo_name;
    int incremental_mode = 1;
    int ret;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (argc >= 2)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	      incremental_mode = sqlite3_value_int (argv[1]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeoUpdateSeeds (accessor, incremental_mode);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = gaiaGetLwGeomErrorMsg ();
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

#endif /* end TOPOLOGY conditionals */
