/*
 topology_private.h -- Topology opaque definitions
  
 version 4.3, 2015 July 15

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

/**
 \file topology_private.h

 SpatiaLite Topology private header file
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _WIN32
#ifdef DLL_EXPORT
#define TOPOLOGY_PRIVATE
#else
#define TOPOLOGY_PRIVATE
#endif
#else
#define TOPOLOGY_PRIVATE __attribute__ ((visibility("hidden")))
#endif
#endif

struct gaia_topology
{
/* a struct wrapping a Topology Accessor Object */
    const void *cache;
    sqlite3 *db_handle;
    char *topology_name;
    int srid;
    double tolerance;
    int has_z;
    char *last_error_message;
    sqlite3_stmt *stmt_getNodeWithinDistance2D;
    sqlite3_stmt *stmt_insertNodes;
    sqlite3_stmt *stmt_getEdgeWithinDistance2D;
    sqlite3_stmt *stmt_getNextEdgeId;
    sqlite3_stmt *stmt_setNextEdgeId;
    sqlite3_stmt *stmt_insertEdges;
    sqlite3_stmt *stmt_getFaceContainingPoint_1;
    sqlite3_stmt *stmt_getFaceContainingPoint_2;
    sqlite3_stmt *stmt_deleteEdges;
    sqlite3_stmt *stmt_getNodeWithinBox2D;
    sqlite3_stmt *stmt_getEdgeWithinBox2D;
    sqlite3_stmt *stmt_getFaceWithinBox2D;
    sqlite3_stmt *stmt_updateNodes;
    sqlite3_stmt *stmt_insertFaces;
    sqlite3_stmt *stmt_updateFacesById;
    sqlite3_stmt *stmt_getRingEdges;
    sqlite3_stmt *stmt_deleteFacesById;
    sqlite3_stmt *stmt_deleteNodesById;
    void *callbacks;
    void *lwt_iface;
    void *lwt_topology;
    struct gaia_topology *prev;
    struct gaia_topology *next;
    int inside_lwt_callback;
};

struct face_edge_item
{
/* a struct wrapping a Face-Edge item */
    sqlite3_int64 edge_id;
    sqlite3_int64 left_face;
    sqlite3_int64 right_face;
    gaiaGeomCollPtr geom;
    int count;
    struct face_edge_item *next;
};

struct face_item
{
/* a struct wrapping a Face item */
    sqlite3_int64 face_id;
    struct face_item *next;
};

struct face_edges
{
/* a struct containing Face-Edge items */
    int has_z;
    int srid;
    struct face_edge_item *first_edge;
    struct face_edge_item *last_edge;
    struct face_item *first_face;
    struct face_item *last_face;
};

/* common utilities */
TOPOLOGY_PRIVATE LWLINE *gaia_convert_linestring_to_lwline (gaiaLinestringPtr
							    ln, int srid,
							    int has_z);
TOPOLOGY_PRIVATE LWPOLY *gaia_convert_polygon_to_lwpoly (gaiaPolygonPtr pg,
							 int srid, int has_z);

/* prototypes for functions handling Topology errors */
TOPOLOGY_PRIVATE void gaiatopo_reset_last_error_msg (GaiaTopologyAccessorPtr
						     accessor);

TOPOLOGY_PRIVATE void gaiatopo_set_last_error_msg (GaiaTopologyAccessorPtr
						   accessor, const char *msg);

TOPOLOGY_PRIVATE const char
    *gaiatopo_get_last_exception (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE struct face_edges *auxtopo_create_face_edges (int has_z,
							       int srid);

TOPOLOGY_PRIVATE void auxtopo_free_face_edges (struct face_edges *list);

TOPOLOGY_PRIVATE void auxtopo_add_face_edge (struct face_edges *list,
					     sqlite3_int64 face_id,
					     sqlite3_int64 edge_id,
					     sqlite3_int64 left_face,
					     sqlite3_int64 right_face,
					     gaiaGeomCollPtr geom);

TOPOLOGY_PRIVATE void auxtopo_select_valid_face_edges (struct face_edges *list);

TOPOLOGY_PRIVATE gaiaGeomCollPtr auxtopo_polygonize_face_edges (struct
								face_edges
								*list,
								const void
								*cache);

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_polygonize_face_edges_generalize (struct face_edges *list,
					  const void *cache);

TOPOLOGY_PRIVATE int auxtopo_create_togeotable_sql (sqlite3 * db_handle,
						    const char *db_prefix,
						    const char *ref_table,
						    const char *ref_column,
						    const char *out_table,
						    char **xcreate,
						    char **xselect,
						    char **xinsert,
						    int *ref_geom_col);

TOPOLOGY_PRIVATE int auxtopo_retrieve_geometry_type (sqlite3 * handle,
						     const char *db_prefix,
						     const char *table,
						     const char *column,
						     int *ref_type);

TOPOLOGY_PRIVATE gaiaGeomCollPtr auxtopo_make_geom_from_point (int srid,
							       int has_z,
							       gaiaPointPtr pt);

TOPOLOGY_PRIVATE gaiaGeomCollPtr auxtopo_make_geom_from_line (int srid,
							      gaiaLinestringPtr
							      line);

TOPOLOGY_PRIVATE void auxtopo_destroy_geom_from (gaiaGeomCollPtr reference);

TOPOLOGY_PRIVATE void auxtopo_copy_linestring (gaiaLinestringPtr line,
					       gaiaGeomCollPtr geom);

TOPOLOGY_PRIVATE void auxtopo_copy_linestring3d (gaiaLinestringPtr line,
						 gaiaGeomCollPtr geom);

TOPOLOGY_PRIVATE int auxtopo_insert_into_topology (GaiaTopologyAccessorPtr
						   accessor,
						   gaiaGeomCollPtr geom,
						   double tolerance,
						   int line_max_points,
						   double max_length);


/* prototypes for functions creating some SQL prepared statement */
TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_getNodeWithinDistance2D (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_getNodeWithinBox2D (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_getEdgeWithinDistance2D (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_getEdgeWithinBox2D (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_getFaceWithinBox2D (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    *
do_create_stmt_getFaceContainingPoint_1 (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    *
do_create_stmt_getFaceContainingPoint_2 (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_insertNodes (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_insertEdges (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_updateEdges (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_getNextEdgeId (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_setNextEdgeId (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_getRingEdges (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_insertFaces (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_updateFacesById (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_deleteFacesById (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE sqlite3_stmt
    * do_create_stmt_deleteNodesById (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE void
finalize_topogeo_prepared_stmts (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE void
create_topogeo_prepared_stmts (GaiaTopologyAccessorPtr accessor);

TOPOLOGY_PRIVATE void finalize_all_topo_prepared_stmts (const void *cache);

TOPOLOGY_PRIVATE void create_all_topo_prepared_stmts (const void *cache);


/* callback function prototypes */
const char *callback_lastErrorMessage (const LWT_BE_DATA * be);

int callback_freeTopology (LWT_BE_TOPOLOGY * topo);

LWT_BE_TOPOLOGY *callback_loadTopologyByName (const LWT_BE_DATA * be,
					      const char *name);

LWT_ISO_NODE *callback_getNodeById (const LWT_BE_TOPOLOGY * topo,
				    const LWT_ELEMID * ids, int *numelems,
				    int fields);

LWT_ISO_NODE *callback_getNodeWithinDistance2D (const LWT_BE_TOPOLOGY *
						topo, const LWPOINT * pt,
						double dist, int *numelems,
						int fields, int limit);

int callback_insertNodes (const LWT_BE_TOPOLOGY * topo,
			  LWT_ISO_NODE * nodes, int numelems);

LWT_ISO_EDGE *callback_getEdgeById (const LWT_BE_TOPOLOGY * topo,
				    const LWT_ELEMID * ids, int *numelems,
				    int fields);

LWT_ISO_EDGE *callback_getEdgeWithinDistance2D (const LWT_BE_TOPOLOGY *
						topo, const LWPOINT * pt,
						double dist, int *numelems,
						int fields, int limit);

LWT_ELEMID callback_getNextEdgeId (const LWT_BE_TOPOLOGY * topo);

int callback_insertEdges (const LWT_BE_TOPOLOGY * topo,
			  LWT_ISO_EDGE * edges, int numelems);

int callback_updateEdges (const LWT_BE_TOPOLOGY * topo,
			  const LWT_ISO_EDGE * sel_edge, int sel_fields,
			  const LWT_ISO_EDGE * upd_edge, int upd_fields,
			  const LWT_ISO_EDGE * exc_edge, int exc_fields);

LWT_ISO_FACE *callback_getFaceById (const LWT_BE_TOPOLOGY * topo,
				    const LWT_ELEMID * ids, int *numelems,
				    int fields);

LWT_ELEMID callback_getFaceContainingPoint (const LWT_BE_TOPOLOGY * topo,
					    const LWPOINT * pt);

int callback_deleteEdges (const LWT_BE_TOPOLOGY * topo,
			  const LWT_ISO_EDGE * sel_edge, int sel_fields);

LWT_ISO_NODE *callback_getNodeWithinBox2D (const LWT_BE_TOPOLOGY * topo,
					   const GBOX * box, int *numelems,
					   int fields, int limit);

LWT_ISO_EDGE *callback_getEdgeWithinBox2D (const LWT_BE_TOPOLOGY * topo,
					   const GBOX * box, int *numelems,
					   int fields, int limit);

LWT_ISO_EDGE *callback_getEdgeByNode (const LWT_BE_TOPOLOGY * topo,
				      const LWT_ELEMID * ids,
				      int *numelems, int fields);

int callback_updateNodes (const LWT_BE_TOPOLOGY * topo,
			  const LWT_ISO_NODE * sel_node, int sel_fields,
			  const LWT_ISO_NODE * upd_node, int upd_fields,
			  const LWT_ISO_NODE * exc_node, int exc_fields);

int callback_updateTopoGeomFaceSplit (const LWT_BE_TOPOLOGY * topo,
				      LWT_ELEMID split_face,
				      LWT_ELEMID new_face1,
				      LWT_ELEMID new_face2);

int callback_insertFaces (const LWT_BE_TOPOLOGY * topo,
			  LWT_ISO_FACE * faces, int numelems);

int callback_updateFacesById (const LWT_BE_TOPOLOGY * topo,
			      const LWT_ISO_FACE * faces, int numfaces);

int callback_deleteFacesById (const LWT_BE_TOPOLOGY * topo,
			      const LWT_ELEMID * ids, int numelems);

LWT_ELEMID *callback_getRingEdges (const LWT_BE_TOPOLOGY * topo,
				   LWT_ELEMID edge, int *numedges, int limit);

int callback_updateEdgesById (const LWT_BE_TOPOLOGY * topo,
			      const LWT_ISO_EDGE * edges, int numedges,
			      int upd_fields);

LWT_ISO_EDGE *callback_getEdgeByFace (const LWT_BE_TOPOLOGY * topo,
				      const LWT_ELEMID * ids,
				      int *numelems, int fields,
				      const GBOX * box);

LWT_ISO_NODE *callback_getNodeByFace (const LWT_BE_TOPOLOGY * topo,
				      const LWT_ELEMID * faces,
				      int *numelems, int fields,
				      const GBOX * box);

int callback_updateNodesById (const LWT_BE_TOPOLOGY * topo,
			      const LWT_ISO_NODE * nodes, int numnodes,
			      int upd_fields);

int callback_deleteNodesById (const LWT_BE_TOPOLOGY * topo,
			      const LWT_ELEMID * ids, int numelems);

int callback_updateTopoGeomEdgeSplit (const LWT_BE_TOPOLOGY * topo,
				      LWT_ELEMID split_edge,
				      LWT_ELEMID new_edge1,
				      LWT_ELEMID new_edge2);

int callback_checkTopoGeomRemEdge (const LWT_BE_TOPOLOGY * topo,
				   LWT_ELEMID rem_edge, LWT_ELEMID face_left,
				   LWT_ELEMID face_right);

int callback_updateTopoGeomFaceHeal (const LWT_BE_TOPOLOGY * topo,
				     LWT_ELEMID face1, LWT_ELEMID face2,
				     LWT_ELEMID newface);

int callback_checkTopoGeomRemNode (const LWT_BE_TOPOLOGY * topo,
				   LWT_ELEMID rem_node, LWT_ELEMID e1,
				   LWT_ELEMID e2);

int callback_updateTopoGeomEdgeHeal (const LWT_BE_TOPOLOGY * topo,
				     LWT_ELEMID edge1, LWT_ELEMID edge2,
				     LWT_ELEMID newedge);

LWT_ISO_FACE *callback_getFaceWithinBox2D (const LWT_BE_TOPOLOGY * topo,
					   const GBOX * box, int *numelems,
					   int fields, int limit);

int callback_topoGetSRID (const LWT_BE_TOPOLOGY * topo);

double callback_topoGetPrecision (const LWT_BE_TOPOLOGY * topo);

int callback_topoHasZ (const LWT_BE_TOPOLOGY * topo);
