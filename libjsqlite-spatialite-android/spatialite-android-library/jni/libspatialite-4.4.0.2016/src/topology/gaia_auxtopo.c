/*

 gaia_auxtopo.c -- implementation of the Topology module methods
    
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

CIG: 6038019AE5

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

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
#include <spatialite/gaia_network.h>
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#include <liblwgeom.h>
#include <liblwgeom_topo.h>

#include <lwn_network.h>

#include "topology_private.h"
#include "network_private.h"

#define GAIA_UNUSED() if (argc || argv) argc = argc;

SPATIALITE_PRIVATE void
free_internal_cache_topologies (void *firstTopology)
{
/* destroying all Topologies registered into the Internal Connection Cache */
    struct gaia_topology *p_topo = (struct gaia_topology *) firstTopology;
    struct gaia_topology *p_topo_n;

    while (p_topo != NULL)
      {
	  p_topo_n = p_topo->next;
	  gaiaTopologyDestroy ((GaiaTopologyAccessorPtr) p_topo);
	  p_topo = p_topo_n;
      }
}

static int
do_create_topologies (sqlite3 * handle)
{
/* attempting to create the Topologies table (if not already existing) */
    const char *sql;
    char *err_msg = NULL;
    int ret;

    sql = "CREATE TABLE IF NOT EXISTS topologies (\n"
	"\ttopology_name TEXT NOT NULL PRIMARY KEY,\n"
	"\tsrid INTEGER NOT NULL,\n"
	"\ttolerance DOUBLE NOT NULL,\n"
	"\thas_z INTEGER NOT NULL,\n"
	"\tnext_edge_id INTEGER NOT NULL DEFAULT 1,\n"
	"\tCONSTRAINT topo_srid_fk FOREIGN KEY (srid) "
	"REFERENCES spatial_ref_sys (srid))";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topologies - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating Topologies triggers */
    sql = "CREATE TRIGGER IF NOT EXISTS topology_name_insert\n"
	"BEFORE INSERT ON 'topologies'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must not contain a single quote')\n"
	"WHERE NEW.topology_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must not contain a double quote')\n"
	"WHERE NEW.topology_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must be lower case')\n"
	"WHERE NEW.topology_name <> lower(NEW.topology_name);\nEND";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER IF NOT EXISTS topology_name_update\n"
	"BEFORE UPDATE OF 'topology_name' ON 'topologies'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must not contain a single quote')\n"
	"WHERE NEW.topology_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must not contain a double quote')\n"
	"WHERE NEW.topology_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must be lower case')\n"
	"WHERE NEW.topology_name <> lower(NEW.topology_name);\nEND";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
check_new_topology (sqlite3 * handle, const char *topo_name)
{
/* testing if some already defined DB object forbids creating the new Topology */
    char *sql;
    char *prev;
    char *table;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    const char *value;
    int error = 0;

/* testing if the same Topology is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.topologies WHERE "
			   "Lower(topology_name) = Lower(%Q)", topo_name);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if some table/geom is already defined in geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'mbr')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if some Spatial View is already defined in views_geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM views_geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'mbr')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if some table is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM sqlite_master WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("%s Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_node_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_edge_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_face_mbr", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_seeds_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

    return 1;
}

static int
do_create_face (sqlite3 * handle, const char *topo_name, int srid)
{
/* attempting to create the Topology Face table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tface_id INTEGER PRIMARY KEY AUTOINCREMENT)",
			   xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-FACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Face BBOX Geometry */
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'mbr', %d, 'POLYGON', 'XY')",
	 table, srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-FACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Face Geometry */
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'mbr')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-FACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* inserting the default World Face */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" VALUES (0, NULL)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT WorldFACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_node (sqlite3 * handle, const char *topo_name, int srid, int has_z)
{
/* attempting to create the Topology Node table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint;
    char *xmother;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node_face_fk", topo_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xmother = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tnode_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tcontaining_face INTEGER,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (containing_face) "
			   "REFERENCES \"%s\" (face_id))", xtable, xconstraint,
			   xmother);
    free (xtable);
    free (xconstraint);
    free (xmother);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Node Geometry */
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geom', %d, 'POINT', %Q, 1)", table,
	 srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Node Geometry */
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "containing_face" */
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_node_contface", topo_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (containing_face)",
			 xconstraint, xtable);
    free (xtable);
    free (xconstraint);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX node-contface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_edge (sqlite3 * handle, const char *topo_name, int srid, int has_z)
{
/* attempting to create the Topology Edge table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint1;
    char *xconstraint2;
    char *xconstraint3;
    char *xconstraint4;
    char *xnodes;
    char *xfaces;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_node_start_fk", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_node_end_fk", topo_name);
    xconstraint2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_face_left_fk", topo_name);
    xconstraint3 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_face_right_fk", topo_name);
    xconstraint4 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo_name);
    xnodes = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tedge_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tstart_node INTEGER NOT NULL,\n"
			   "\tend_node INTEGER NOT NULL,\n"
			   "\tnext_left_edge INTEGER NOT NULL,\n"
			   "\tnext_right_edge INTEGER NOT NULL,\n"
			   "\tleft_face INTEGER NOT NULL,\n"
			   "\tright_face INTEGER NOT NULL,\n"
			   "\ttimestamp DATETIME,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (start_node) "
			   "REFERENCES \"%s\" (node_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (end_node) "
			   "REFERENCES \"%s\" (node_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (left_face) "
			   "REFERENCES \"%s\" (face_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (right_face) "
			   "REFERENCES \"%s\" (face_id))",
			   xtable, xconstraint1, xnodes, xconstraint2, xnodes,
			   xconstraint3, xfaces, xconstraint4, xfaces);
    free (xtable);
    free (xconstraint1);
    free (xconstraint2);
    free (xconstraint3);
    free (xconstraint4);
    free (xnodes);
    free (xfaces);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_edge_ins" trigger */
    trigger = sqlite3_mprintf ("%s_edge_next_ins", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE topologies SET next_edge_id = NEW.edge_id + 1 "
			   "WHERE Lower(topology_name) = Lower(%Q) AND next_edge_id < NEW.edge_id + 1;\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE edge_id = NEW.edge_id;"
			   "END", xtrigger, xtable, topo_name, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "edge_update" trigger */
    trigger = sqlite3_mprintf ("%s_edge_update", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE edge_id = NEW.edge_id;"
			   "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_edge_upd" trigger */
    trigger = sqlite3_mprintf ("%s_edge_next_upd", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF edge_id ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "\tUPDATE topologies SET next_edge_id = NEW.edge_id + 1 "
	 "WHERE Lower(topology_name) = Lower(%Q) AND next_edge_id < NEW.edge_id + 1;\n"
	 "END", xtrigger, xtable, topo_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Edge Geometry */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geom', %d, 'LINESTRING', %Q, 1)",
	 table, srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Edge Geometry */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "start_node" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_start_node", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (start_node)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-startnode - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "end_node" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_end_node", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (end_node)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-endnode - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "left_face" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_edge_leftface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (left_face)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-leftface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "right_face" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_edge_rightface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (right_face)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-rightface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "timestamp" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_timestamp", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (timestamp)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-timestamps - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_seeds (sqlite3 * handle, const char *topo_name, int srid, int has_z)
{
/* attempting to create the Topology Seeds table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint1;
    char *xconstraint2;
    char *xedges;
    char *xfaces;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds_edge_fk", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds_face_fk", topo_name);
    xconstraint2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tseed_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tedge_id INTEGER,\n"
			   "\tface_id INTEGER,\n"
			   "\ttimestamp DATETIME,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (edge_id) "
			   "REFERENCES \"%s\" (edge_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (face_id) "
			   "REFERENCES \"%s\" (face_id) ON DELETE CASCADE)",
			   xtable, xconstraint1, xedges, xconstraint2, xfaces);
    free (xtable);
    free (xconstraint1);
    free (xconstraint2);
    free (xedges);
    free (xfaces);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "seeds_ins" trigger */
    trigger = sqlite3_mprintf ("%s_seeds_ins", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE seed_id = NEW.seed_id;"
			   "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-SEEDS next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "seeds_update" trigger */
    trigger = sqlite3_mprintf ("%s_seeds_update", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE seed_id = NEW.seed_id;"
			   "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-SEED next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Seeds Geometry */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geom', %d, 'POINT', %Q, 1)",
	 table, srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Seeds Geometry */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "edge_id" */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_sdedge", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (edge_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX seeds-edge - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "face_id" */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_sdface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (face_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX seeds-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "timestamp" */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_seeds_timestamp", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (timestamp)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX seeds-timestamps - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_edge_seeds (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the Edge Seeds view */
    char *sql;
    char *table;
    char *xtable;
    char *xview;
    char *err_msg = NULL;
    int ret;

/* creating the view */
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    xview = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS\n"
			   "SELECT seed_id AS rowid, edge_id AS edge_id, geom AS geom\n"
			   "FROM \"%s\"\n"
			   "WHERE edge_id IS NOT NULL", xview, xtable);
    free (xtable);
    free (xview);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW topology-EDGE-SEEDS - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* registering a Spatial View */
    xview = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    xtable = sqlite3_mprintf ("%s_seeds", topo_name);
    sql = sqlite3_mprintf ("INSERT INTO views_geometry_columns (view_name, "
			   "view_geometry, view_rowid, f_table_name, f_geometry_column, read_only) "
			   "VALUES (Lower(%Q), 'geom', 'rowid', Lower(%Q), 'geom', 1)",
			   xview, xtable);
    sqlite3_free (xview);
    sqlite3_free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Registering Spatial VIEW topology-EDGE-SEEDS - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_face_seeds (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the Face Seeds view */
    char *sql;
    char *table;
    char *xtable;
    char *xview;
    char *err_msg = NULL;
    int ret;

/* creating the view */
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    xview = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS\n"
			   "SELECT seed_id AS rowid, face_id AS face_id, geom AS geom\n"
			   "FROM \"%s\"\n"
			   "WHERE face_id IS NOT NULL", xview, xtable);
    free (xtable);
    free (xview);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW topology-FACE-SEEDS - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* registering a Spatial View */
    xview = sqlite3_mprintf ("%s_face_seeds", topo_name);
    xtable = sqlite3_mprintf ("%s_seeds", topo_name);
    sql = sqlite3_mprintf ("INSERT INTO views_geometry_columns (view_name, "
			   "view_geometry, view_rowid, f_table_name, f_geometry_column, read_only) "
			   "VALUES (Lower(%Q), 'geom', 'rowid', Lower(%Q), 'geom', 1)",
			   xview, xtable);
    sqlite3_free (xview);
    sqlite3_free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Registering Spatial VIEW topology-FACE-SEEDS - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_face_geoms (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the Face Geoms view */
    char *sql;
    char *table;
    char *xtable;
    char *xview;
    char *err_msg = NULL;
    int ret;

/* creating the view */
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    xview = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS\n"
			   "SELECT face_id AS rowid, ST_GetFaceGeometry(%Q, face_id) AS geom\n"
			   "FROM \"%s\"\n"
			   "WHERE face_id <> 0", xview, topo_name, xtable);
    free (xtable);
    free (xview);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW topology-FACE-GEOMS - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* registering a Spatial View */
    xview = sqlite3_mprintf ("%s_face_geoms", topo_name);
    xtable = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("INSERT INTO views_geometry_columns (view_name, "
			   "view_geometry, view_rowid, f_table_name, f_geometry_column, read_only) "
			   "VALUES (Lower(%Q), 'geom', 'rowid', Lower(%Q), 'mbr', 1)",
			   xview, xtable);
    sqlite3_free (xview);
    sqlite3_free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Registering Spatial VIEW topology-FACE-GEOMS - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_topolayers (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the TopoLayers table */
    char *sql;
    char *table;
    char *xtable;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\ttopolayer_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\ttopolayer_name NOT NULL UNIQUE)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-TOPOLAYERS - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating TopoLayers triggers */
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topolayer_name_insert", topo_name);
    xtrigger = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER IF NOT EXISTS \"%s\"\n"
			   "BEFORE INSERT ON \"%s\"\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT,'insert on topolayers violates constraint: "
			   "topolayer_name value must not contain a single quote')\n"
			   "WHERE NEW.topolayer_name LIKE ('%%''%%');\n"
			   "SELECT RAISE(ABORT,'insert on topolayers violates constraint: "
			   "topolayers_name value must not contain a double quote')\n"
			   "WHERE NEW.topolayer_name LIKE ('%%\"%%');\n"
			   "SELECT RAISE(ABORT,'insert on topolayers violates constraint: "
			   "topolayer_name value must be lower case')\n"
			   "WHERE NEW.topolayer_name <> lower(NEW.topolayer_name);\nEND",
			   xtrigger, xtable);
    free (xtable);
    free (xtrigger);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topolayer_name_update", topo_name);
    xtrigger = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER IF NOT EXISTS \"%s\"\n"
			   "BEFORE UPDATE OF 'topolayer_name' ON \"%s\"\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT,'update on topolayers violates constraint: "
			   "topolayer_name value must not contain a single quote')\n"
			   "WHERE NEW.topolayer_name LIKE ('%%''%%');\n"
			   "SELECT RAISE(ABORT,'update on topolayers violates constraint: "
			   "topolayer_name value must not contain a double quote')\n"
			   "WHERE NEW.topolayer_name LIKE ('%%\"%%');\n"
			   "SELECT RAISE(ABORT,'update on topolayers violates constraint: "
			   "topolayer_name value must be lower case')\n"
			   "WHERE NEW.topolayer_name <> lower(NEW.topolayer_name);\nEND",
			   xtrigger, xtable);
    free (xtable);
    free (xtrigger);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_topofeatures (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the TopoFeatures table */
    char *sql;
    char *table;
    char *xtable;
    char *xtable1;
    char *xtable2;
    char *xtable3;
    char *xtable4;
    char *xconstraint1;
    char *xconstraint2;
    char *xconstraint3;
    char *xconstraint4;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable3 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable4 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("fk_%s_ftnode", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("fk_%s_ftedge", topo_name);
    xconstraint2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("fk_%s_ftface", topo_name);
    xconstraint3 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("fk_%s_topolayer", topo_name);
    xconstraint4 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tuid INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tnode_id INTEGER,\n\tedge_id INTEGER,\n"
			   "\tface_id INTEGER,\n\ttopolayer_id INTEGER NOT NULL,\n"
			   "\tfid INTEGER NOT NULL,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (node_id) "
			   "REFERENCES \"%s\" (node_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (edge_id) "
			   "REFERENCES \"%s\" (edge_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (face_id) "
			   "REFERENCES \"%s\" (face_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (topolayer_id) "
			   "REFERENCES \"%s\" (topolayer_id) ON DELETE CASCADE)",
			   xtable, xconstraint1, xtable1, xconstraint2, xtable2,
			   xconstraint3, xtable3, xconstraint4, xtable4);
    free (xtable);
    free (xtable1);
    free (xtable2);
    free (xtable3);
    free (xtable4);
    free (xconstraint1);
    free (xconstraint2);
    free (xconstraint3);
    free (xconstraint4);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-TOPOFEATURES - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "node_id" */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_ftnode", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (node_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX topofeatures-node - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "edge_id" */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_ftedge", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (edge_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX topofeatures-edge - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "face_id" */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_ftface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (face_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX topofeatures-face - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "topolayers_id" */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_fttopolayers", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (topolayer_id, fid)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX topofeatures-topolayers - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopologyCreate (sqlite3 * handle, const char *topo_name, int srid,
		    double tolerance, int has_z)
{
/* attempting to create a new Topology */
    int ret;
    char *sql;

/* creating the Topologies table (just in case) */
    if (!do_create_topologies (handle))
	return 0;

/* testing for forbidding objects */
    if (!check_new_topology (handle, topo_name))
	return 0;

/* creating the Topology own Tables */
    if (!do_create_face (handle, topo_name, srid))
	goto error;
    if (!do_create_node (handle, topo_name, srid, has_z))
	goto error;
    if (!do_create_edge (handle, topo_name, srid, has_z))
	goto error;
    if (!do_create_seeds (handle, topo_name, srid, has_z))
	goto error;
    if (!do_create_edge_seeds (handle, topo_name))
	goto error;
    if (!do_create_face_seeds (handle, topo_name))
	goto error;
    if (!do_create_face_geoms (handle, topo_name))
	goto error;
    if (!do_create_topolayers (handle, topo_name))
	goto error;
    if (!do_create_topofeatures (handle, topo_name))
	goto error;

/* registering the Topology */
    sql = sqlite3_mprintf ("INSERT INTO MAIN.topologies (topology_name, "
			   "srid, tolerance, has_z) VALUES (Lower(%Q), %d, %f, %d)",
			   topo_name, srid, tolerance, has_z);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    return 1;

  error:
    return 0;
}

static int
check_existing_topology (sqlite3 * handle, const char *topo_name,
			 int full_check)
{
/* testing if a Topology is already defined */
    char *sql;
    char *prev;
    char *table;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    const char *value;
    int error = 0;

/* testing if the Topology is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.topologies WHERE "
			   "Lower(topology_name) = Lower(%Q)", topo_name);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 1)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;
    if (!full_check)
	return 1;

/* testing if all table/geom are correctly defined in geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'mbr')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 3)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if all Spatial Views are correctly defined in geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM views_geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(view_name) = Lower(%Q) AND view_geometry = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(view_name) = Lower(%Q) AND view_geometry = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(view_name) = Lower(%Q) AND view_geometry = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 3)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;


/* testing if all tables are already defined */
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM sqlite_master WHERE (type = 'table' AND (");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("%s Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_node_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_edge_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_face_mbr", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)))", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    sql =
	sqlite3_mprintf ("%s OR (type = 'view' AND (Lower(name) = Lower(%Q)",
			 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)))", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 9)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

    return 1;
}

static int
do_drop_topo_face (sqlite3 * handle, const char *topo_name)
{
/* attempting to drop the Topology-Face table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

/* disabling the corresponding Spatial Index */
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("SELECT DisableSpatialIndex(%Q, 'mbr')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DisableSpatialIndex topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* discarding the Geometry column */
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("SELECT DiscardGeometryColumn(%Q, 'mbr')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DisableGeometryColumn topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the main table */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the corresponding Spatial Index */
    table = sqlite3_mprintf ("idx_%s_face_mbr", topo_name);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DROP SpatialIndex topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_drop_topo_table (sqlite3 * handle, const char *topo_name, const char *which,
		    int spatial)
{
/* attempting to drop some Topology table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

    if (strcmp (which, "face") == 0)
	return do_drop_topo_face (handle, topo_name);

    if (spatial)
      {
	  /* disabling the corresponding Spatial Index */
	  table = sqlite3_mprintf ("%s_%s", topo_name, which);
	  sql =
	      sqlite3_mprintf ("SELECT DisableSpatialIndex(%Q, 'geom')", table);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
	  sqlite3_free (table);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e
		    ("DisableSpatialIndex topology-%s - error: %s\n", which,
		     err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
	  /* discarding the Geometry column */
	  table = sqlite3_mprintf ("%s_%s", topo_name, which);
	  sql =
	      sqlite3_mprintf ("SELECT DiscardGeometryColumn(%Q, 'geom')",
			       table);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
	  sqlite3_free (table);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e
		    ("DisableGeometryColumn topology-%s - error: %s\n", which,
		     err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
      }

/* dropping the main table */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP topology-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    if (spatial)
      {
	  /* dropping the corresponding Spatial Index */
	  table = sqlite3_mprintf ("idx_%s_%s_geom", topo_name, which);
	  sql = sqlite3_mprintf ("DROP TABLE IF EXISTS MAIN.\"%s\"", table);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
	  sqlite3_free (table);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e
		    ("DROP SpatialIndex topology-%s - error: %s\n", which,
		     err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
      }

    return 1;
}

static int
do_drop_topo_view (sqlite3 * handle, const char *topo_name, const char *which)
{
/* attempting to drop some Topology view */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

/* unregistering the Spatial View */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    sql =
	sqlite3_mprintf
	("DELETE FROM views_geometry_columns WHERE view_name = Lower(%Q)",
	 table);
    sqlite3_free (table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Unregister Spatial View -%s - error: %s\n", which,
			err_msg);
	  sqlite3_free (err_msg);
      }

/* dropping the view */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP VIEW IF EXISTS MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP topology-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_drop_topofeature_tables (sqlite3 * handle, const char *topo_name)
{
/* dropping any eventual topofeatures table */
    char *table;
    char *xtable;
    char *sql;
    char *err_msg = NULL;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;

    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT topolayer_id FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 1;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *id = results[(i * columns) + 0];
		table = sqlite3_mprintf ("%s_topofeatures_%s", topo_name, id);
		xtable = gaiaDoubleQuotedSql (table);
		sqlite3_free (table);
		sql =
		    sqlite3_mprintf ("DROP TABLE IF EXISTS MAIN.\"%s\"",
				     xtable);
		free (xtable);
		ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("DROP topology-features (%s) - error: %s\n",
				    id, err_msg);
		      sqlite3_free (err_msg);
		      return 0;
		  }
	    }
      }
    sqlite3_free_table (results);
    return 1;
}

static int
do_get_topology (sqlite3 * handle, const char *topo_name, char **topology_name,
		 int *srid, double *tolerance, int *has_z)
{
/* retrieving a Topology configuration */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;
    char *xtopology_name = NULL;
    int xsrid;
    double xtolerance;
    int xhas_z;

/* preparing the SQL query */
    sql =
	sqlite3_mprintf
	("SELECT topology_name, srid, tolerance, has_z FROM MAIN.topologies WHERE "
	 "Lower(topology_name) = Lower(%Q)", topo_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM topologys error: \"%s\"\n",
			sqlite3_errmsg (handle));
	  return 0;
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
		return 0;
	    }
      }
    sqlite3_finalize (stmt);

    if (ok)
      {
	  *topology_name = xtopology_name;
	  *srid = xsrid;
	  *tolerance = xtolerance;
	  *has_z = xhas_z;
	  return 1;
      }

    if (xtopology_name != NULL)
	free (xtopology_name);
    return 0;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaGetTopology (sqlite3 * handle, const void *cache, const char *topo_name)
{
/* attempting to get a reference to some Topology Accessor Object */
    GaiaTopologyAccessorPtr accessor;

/* attempting to retrieve an alredy cached definition */
    accessor = gaiaTopologyFromCache (cache, topo_name);
    if (accessor != NULL)
	return accessor;

/* attempting to create a new Topology Accessor */
    accessor = gaiaTopologyFromDBMS (handle, cache, topo_name);
    return accessor;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaTopologyFromCache (const void *p_cache, const char *topo_name)
{
/* attempting to retrieve an already defined Topology Accessor Object from the Connection Cache */
    struct gaia_topology *ptr;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == 0)
	return NULL;

    ptr = (struct gaia_topology *) (cache->firstTopology);
    while (ptr != NULL)
      {
	  /* checking for an already registered Topology */
	  if (strcasecmp (topo_name, ptr->topology_name) == 0)
	      return (GaiaTopologyAccessorPtr) ptr;
	  ptr = ptr->next;
      }
    return NULL;
}

GAIATOPO_DECLARE int
gaiaReadTopologyFromDBMS (sqlite3 *
			  handle,
			  const char
			  *topo_name, char **topology_name, int *srid,
			  double *tolerance, int *has_z)
{
/* testing for existing DBMS objects */
    if (!check_existing_topology (handle, topo_name, 1))
	return 0;

/* retrieving the Topology configuration */
    if (!do_get_topology
	(handle, topo_name, topology_name, srid, tolerance, has_z))
	return 0;
    return 1;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaTopologyFromDBMS (sqlite3 * handle, const void *p_cache,
		      const char *topo_name)
{
/* attempting to create a Topology Accessor Object into the Connection Cache */
    LWT_BE_CALLBACKS *callbacks;
    struct gaia_topology *ptr;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == 0)
	return NULL;

/* allocating and initializing the opaque object */
    ptr = malloc (sizeof (struct gaia_topology));
    ptr->db_handle = handle;
    ptr->cache = cache;
    ptr->topology_name = NULL;
    ptr->srid = -1;
    ptr->tolerance = 0;
    ptr->has_z = 0;
    ptr->inside_lwt_callback = 0;
    ptr->last_error_message = NULL;
    ptr->lwt_iface = lwt_CreateBackendIface ((const LWT_BE_DATA *) ptr);
    ptr->prev = cache->lastTopology;
    ptr->next = NULL;

    callbacks = malloc (sizeof (LWT_BE_CALLBACKS));
    callbacks->lastErrorMessage = callback_lastErrorMessage;
    callbacks->topoGetSRID = callback_topoGetSRID;
    callbacks->topoGetPrecision = callback_topoGetPrecision;
    callbacks->topoHasZ = callback_topoHasZ;
    callbacks->createTopology = NULL;
    callbacks->loadTopologyByName = callback_loadTopologyByName;
    callbacks->freeTopology = callback_freeTopology;
    callbacks->getNodeById = callback_getNodeById;
    callbacks->getNodeWithinDistance2D = callback_getNodeWithinDistance2D;
    callbacks->insertNodes = callback_insertNodes;
    callbacks->getEdgeById = callback_getEdgeById;
    callbacks->getEdgeWithinDistance2D = callback_getEdgeWithinDistance2D;
    callbacks->getNextEdgeId = callback_getNextEdgeId;
    callbacks->insertEdges = callback_insertEdges;
    callbacks->updateEdges = callback_updateEdges;
    callbacks->getFaceById = callback_getFaceById;
    callbacks->getFaceContainingPoint = callback_getFaceContainingPoint;
    callbacks->deleteEdges = callback_deleteEdges;
    callbacks->getNodeWithinBox2D = callback_getNodeWithinBox2D;
    callbacks->getEdgeWithinBox2D = callback_getEdgeWithinBox2D;
    callbacks->getEdgeByNode = callback_getEdgeByNode;
    callbacks->updateNodes = callback_updateNodes;
    callbacks->insertFaces = callback_insertFaces;
    callbacks->updateFacesById = callback_updateFacesById;
    callbacks->deleteFacesById = callback_deleteFacesById;
    callbacks->getRingEdges = callback_getRingEdges;
    callbacks->updateEdgesById = callback_updateEdgesById;
    callbacks->getEdgeByFace = callback_getEdgeByFace;
    callbacks->getNodeByFace = callback_getNodeByFace;
    callbacks->updateNodesById = callback_updateNodesById;
    callbacks->deleteNodesById = callback_deleteNodesById;
    callbacks->updateTopoGeomEdgeSplit = callback_updateTopoGeomEdgeSplit;
    callbacks->updateTopoGeomFaceSplit = callback_updateTopoGeomFaceSplit;
    callbacks->checkTopoGeomRemEdge = callback_checkTopoGeomRemEdge;
    callbacks->updateTopoGeomFaceHeal = callback_updateTopoGeomFaceHeal;
    callbacks->checkTopoGeomRemNode = callback_checkTopoGeomRemNode;
    callbacks->updateTopoGeomEdgeHeal = callback_updateTopoGeomEdgeHeal;
    callbacks->getFaceWithinBox2D = callback_getFaceWithinBox2D;
    ptr->callbacks = callbacks;

    lwt_BackendIfaceRegisterCallbacks (ptr->lwt_iface, callbacks);
    ptr->lwt_topology = lwt_LoadTopology (ptr->lwt_iface, topo_name);

    ptr->stmt_getNodeWithinDistance2D = NULL;
    ptr->stmt_insertNodes = NULL;
    ptr->stmt_getEdgeWithinDistance2D = NULL;
    ptr->stmt_getNextEdgeId = NULL;
    ptr->stmt_setNextEdgeId = NULL;
    ptr->stmt_insertEdges = NULL;
    ptr->stmt_getFaceContainingPoint_1 = NULL;
    ptr->stmt_getFaceContainingPoint_2 = NULL;
    ptr->stmt_deleteEdges = NULL;
    ptr->stmt_getNodeWithinBox2D = NULL;
    ptr->stmt_getEdgeWithinBox2D = NULL;
    ptr->stmt_getFaceWithinBox2D = NULL;
    ptr->stmt_updateNodes = NULL;
    ptr->stmt_insertFaces = NULL;
    ptr->stmt_updateFacesById = NULL;
    ptr->stmt_deleteFacesById = NULL;
    ptr->stmt_deleteNodesById = NULL;
    ptr->stmt_getRingEdges = NULL;
    if (ptr->lwt_topology == NULL)
	goto invalid;

/* creating the SQL prepared statements */
    create_topogeo_prepared_stmts ((GaiaTopologyAccessorPtr) ptr);

    return (GaiaTopologyAccessorPtr) ptr;

  invalid:
    gaiaTopologyDestroy ((GaiaTopologyAccessorPtr) ptr);
    return NULL;
}

GAIATOPO_DECLARE void
gaiaTopologyDestroy (GaiaTopologyAccessorPtr topo_ptr)
{
/* destroying a Topology Accessor Object */
    struct gaia_topology *prev;
    struct gaia_topology *next;
    struct splite_internal_cache *cache;
    struct gaia_topology *ptr = (struct gaia_topology *) topo_ptr;
    if (ptr == NULL)
	return;

    prev = ptr->prev;
    next = ptr->next;
    cache = (struct splite_internal_cache *) (ptr->cache);
    if (ptr->lwt_topology != NULL)
	lwt_FreeTopology ((LWT_TOPOLOGY *) (ptr->lwt_topology));
    if (ptr->lwt_iface != NULL)
	lwt_FreeBackendIface ((LWT_BE_IFACE *) (ptr->lwt_iface));
    if (ptr->callbacks != NULL)
	free (ptr->callbacks);
    if (ptr->topology_name != NULL)
	free (ptr->topology_name);
    if (ptr->last_error_message != NULL)
	free (ptr->last_error_message);

    finalize_topogeo_prepared_stmts (topo_ptr);
    free (ptr);

/* unregistering from the Internal Cache double linked list */
    if (prev != NULL)
	prev->next = next;
    if (next != NULL)
	next->prev = prev;
    if (cache->firstTopology == ptr)
	cache->firstTopology = next;
    if (cache->lastTopology == ptr)
	cache->lastTopology = prev;
}

TOPOLOGY_PRIVATE void
finalize_topogeo_prepared_stmts (GaiaTopologyAccessorPtr accessor)
{
/* finalizing the SQL prepared statements */
    struct gaia_topology *ptr = (struct gaia_topology *) accessor;
    if (ptr->stmt_getNodeWithinDistance2D != NULL)
	sqlite3_finalize (ptr->stmt_getNodeWithinDistance2D);
    if (ptr->stmt_insertNodes != NULL)
	sqlite3_finalize (ptr->stmt_insertNodes);
    if (ptr->stmt_getEdgeWithinDistance2D != NULL)
	sqlite3_finalize (ptr->stmt_getEdgeWithinDistance2D);
    if (ptr->stmt_getNextEdgeId != NULL)
	sqlite3_finalize (ptr->stmt_getNextEdgeId);
    if (ptr->stmt_setNextEdgeId != NULL)
	sqlite3_finalize (ptr->stmt_setNextEdgeId);
    if (ptr->stmt_insertEdges != NULL)
	sqlite3_finalize (ptr->stmt_insertEdges);
    if (ptr->stmt_getFaceContainingPoint_1 != NULL)
	sqlite3_finalize (ptr->stmt_getFaceContainingPoint_1);
    if (ptr->stmt_getFaceContainingPoint_2 != NULL)
	sqlite3_finalize (ptr->stmt_getFaceContainingPoint_2);
    if (ptr->stmt_deleteEdges != NULL)
	sqlite3_finalize (ptr->stmt_deleteEdges);
    if (ptr->stmt_getNodeWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getNodeWithinBox2D);
    if (ptr->stmt_getEdgeWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getEdgeWithinBox2D);
    if (ptr->stmt_getFaceWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getFaceWithinBox2D);
    if (ptr->stmt_updateNodes != NULL)
	sqlite3_finalize (ptr->stmt_updateNodes);
    if (ptr->stmt_insertFaces != NULL)
	sqlite3_finalize (ptr->stmt_insertFaces);
    if (ptr->stmt_updateFacesById != NULL)
	sqlite3_finalize (ptr->stmt_updateFacesById);
    if (ptr->stmt_deleteFacesById != NULL)
	sqlite3_finalize (ptr->stmt_deleteFacesById);
    if (ptr->stmt_deleteNodesById != NULL)
	sqlite3_finalize (ptr->stmt_deleteNodesById);
    if (ptr->stmt_getRingEdges != NULL)
	sqlite3_finalize (ptr->stmt_getRingEdges);
    ptr->stmt_getNodeWithinDistance2D = NULL;
    ptr->stmt_insertNodes = NULL;
    ptr->stmt_getEdgeWithinDistance2D = NULL;
    ptr->stmt_getNextEdgeId = NULL;
    ptr->stmt_setNextEdgeId = NULL;
    ptr->stmt_insertEdges = NULL;
    ptr->stmt_getFaceContainingPoint_1 = NULL;
    ptr->stmt_getFaceContainingPoint_2 = NULL;
    ptr->stmt_deleteEdges = NULL;
    ptr->stmt_getNodeWithinBox2D = NULL;
    ptr->stmt_getEdgeWithinBox2D = NULL;
    ptr->stmt_getFaceWithinBox2D = NULL;
    ptr->stmt_updateNodes = NULL;
    ptr->stmt_insertFaces = NULL;
    ptr->stmt_updateFacesById = NULL;
    ptr->stmt_deleteFacesById = NULL;
    ptr->stmt_deleteNodesById = NULL;
    ptr->stmt_getRingEdges = NULL;
}

TOPOLOGY_PRIVATE void
create_topogeo_prepared_stmts (GaiaTopologyAccessorPtr accessor)
{
/* creating the SQL prepared statements */
    struct gaia_topology *ptr = (struct gaia_topology *) accessor;
    finalize_topogeo_prepared_stmts (accessor);
    ptr->stmt_getNodeWithinDistance2D =
	do_create_stmt_getNodeWithinDistance2D (accessor);
    ptr->stmt_insertNodes = do_create_stmt_insertNodes (accessor);
    ptr->stmt_getEdgeWithinDistance2D =
	do_create_stmt_getEdgeWithinDistance2D (accessor);
    ptr->stmt_getNextEdgeId = do_create_stmt_getNextEdgeId (accessor);
    ptr->stmt_setNextEdgeId = do_create_stmt_setNextEdgeId (accessor);
    ptr->stmt_insertEdges = do_create_stmt_insertEdges (accessor);
    ptr->stmt_getFaceContainingPoint_1 =
	do_create_stmt_getFaceContainingPoint_1 (accessor);
    ptr->stmt_getFaceContainingPoint_2 =
	do_create_stmt_getFaceContainingPoint_2 (accessor);
    ptr->stmt_deleteEdges = NULL;
    ptr->stmt_getNodeWithinBox2D = do_create_stmt_getNodeWithinBox2D (accessor);
    ptr->stmt_getEdgeWithinBox2D = do_create_stmt_getEdgeWithinBox2D (accessor);
    ptr->stmt_getFaceWithinBox2D = do_create_stmt_getFaceWithinBox2D (accessor);
    ptr->stmt_updateNodes = NULL;
    ptr->stmt_insertFaces = do_create_stmt_insertFaces (accessor);
    ptr->stmt_updateFacesById = do_create_stmt_updateFacesById (accessor);
    ptr->stmt_deleteFacesById = do_create_stmt_deleteFacesById (accessor);
    ptr->stmt_deleteNodesById = do_create_stmt_deleteNodesById (accessor);
    ptr->stmt_getRingEdges = do_create_stmt_getRingEdges (accessor);
}

TOPOLOGY_PRIVATE void
finalize_all_topo_prepared_stmts (const void *p_cache)
{
/* finalizing all Topology-related prepared Stms */
    struct gaia_topology *p_topo;
    struct gaia_network *p_network;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;

    p_topo = (struct gaia_topology *) cache->firstTopology;
    while (p_topo != NULL)
      {
	  finalize_topogeo_prepared_stmts ((GaiaTopologyAccessorPtr) p_topo);
	  p_topo = p_topo->next;
      }

    p_network = (struct gaia_network *) cache->firstNetwork;
    while (p_network != NULL)
      {
	  finalize_toponet_prepared_stmts ((GaiaNetworkAccessorPtr) p_network);
	  p_network = p_network->next;
      }
}

TOPOLOGY_PRIVATE void
create_all_topo_prepared_stmts (const void *p_cache)
{
/* (re)creating all Topology-related prepared Stms */
    struct gaia_topology *p_topo;
    struct gaia_network *p_network;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;

    p_topo = (struct gaia_topology *) cache->firstTopology;
    while (p_topo != NULL)
      {
	  create_topogeo_prepared_stmts ((GaiaTopologyAccessorPtr) p_topo);
	  p_topo = p_topo->next;
      }

    p_network = (struct gaia_network *) cache->firstNetwork;
    while (p_network != NULL)
      {
	  create_toponet_prepared_stmts ((GaiaNetworkAccessorPtr) p_network);
	  p_network = p_network->next;
      }
}

TOPOLOGY_PRIVATE void
gaiatopo_reset_last_error_msg (GaiaTopologyAccessorPtr accessor)
{
/* resets the last Topology error message */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return;

    if (topo->last_error_message != NULL)
	free (topo->last_error_message);
    topo->last_error_message = NULL;
}

TOPOLOGY_PRIVATE void
gaiatopo_set_last_error_msg (GaiaTopologyAccessorPtr accessor, const char *msg)
{
/* sets the last Topology error message */
    int len;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (msg == NULL)
	msg = "no message available";

    spatialite_e ("%s\n", msg);
    if (topo == NULL)
	return;

    if (topo->last_error_message != NULL)
	return;

    len = strlen (msg);
    topo->last_error_message = malloc (len + 1);
    strcpy (topo->last_error_message, msg);
}

TOPOLOGY_PRIVATE const char *
gaiatopo_get_last_exception (GaiaTopologyAccessorPtr accessor)
{
/* returns the last Topology error message */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return NULL;

    return topo->last_error_message;
}

GAIATOPO_DECLARE int
gaiaTopologyDrop (sqlite3 * handle, const char *topo_name)
{
/* attempting to drop an already existing Topology */
    int ret;
    char *sql;
    int i;
    char **results;
    int rows;
    int columns;
    int count = 1;

/* creating the Topologies table (just in case) */
    if (!do_create_topologies (handle))
	return 0;

/* testing for existing DBMS objects */
    if (!check_existing_topology (handle, topo_name, 0))
	return 0;

/* dropping all topofeature tables (if any) */
    if (!do_drop_topofeature_tables (handle, topo_name))
	goto error;

/* dropping the Topology own Tables */
    if (!do_drop_topo_view (handle, topo_name, "edge_seeds"))
	goto error;
    if (!do_drop_topo_view (handle, topo_name, "face_seeds"))
	goto error;
    if (!do_drop_topo_view (handle, topo_name, "face_geoms"))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "topofeatures", 0))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "topolayers", 0))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "seeds", 1))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "edge", 1))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "node", 1))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "face", 1))
	goto error;

/* unregistering the Topology */
    sql =
	sqlite3_mprintf
	("DELETE FROM MAIN.topologies WHERE Lower(topology_name) = Lower(%Q)",
	 topo_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* counting how many Topologies are still there */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.topologies");
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 1;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	      count = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);
    if (count == 0)
      {
	  /* attempting to drop the master "topologies" table */
	  sql = sqlite3_mprintf ("DROP TABLE MAIN.topologies");
	  ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
      }

    return 1;

  error:
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddIsoNode (GaiaTopologyAccessorPtr accessor,
		sqlite3_int64 face, gaiaPointPtr pt, int skip_checks)
{
/* LWT wrapper - AddIsoNode */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddIsoNode ((LWT_TOPOLOGY *) (topo->lwt_topology), face, lw_pt,
			skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoint_free (lw_pt);
    return ret;
}

GAIATOPO_DECLARE int
gaiaMoveIsoNode (GaiaTopologyAccessorPtr accessor,
		 sqlite3_int64 node, gaiaPointPtr pt)
{
/* LWT wrapper - MoveIsoNode */
    int ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_MoveIsoNode ((LWT_TOPOLOGY *) (topo->lwt_topology), node, lw_pt);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoint_free (lw_pt);
    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE int
gaiaRemIsoNode (GaiaTopologyAccessorPtr accessor, sqlite3_int64 node)
{
/* LWT wrapper - RemIsoNode */
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_RemoveIsoNode ((LWT_TOPOLOGY *) (topo->lwt_topology), node);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddIsoEdge (GaiaTopologyAccessorPtr accessor,
		sqlite3_int64 start_node, sqlite3_int64 end_node,
		gaiaLinestringPtr ln)
{
/* LWT wrapper - AddIsoEdge */
    sqlite3_int64 ret;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddIsoEdge ((LWT_TOPOLOGY *) (topo->lwt_topology), start_node,
			end_node, lw_line);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    return ret;
}

GAIATOPO_DECLARE int
gaiaRemIsoEdge (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge)
{
/* LWT wrapper - RemIsoEdge */
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_RemIsoEdge ((LWT_TOPOLOGY *) (topo->lwt_topology), edge);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE int
gaiaChangeEdgeGeom (GaiaTopologyAccessorPtr accessor,
		    sqlite3_int64 edge_id, gaiaLinestringPtr ln)
{
/* LWT wrapper - ChangeEdgeGeom  */
    int ret;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_ChangeEdgeGeom ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id,
			    lw_line);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    if (ret == 0)
	return 1;
    return 0;
}

static void
do_check_mod_split_edge3d (struct gaia_topology *topo, gaiaPointPtr pt,
			   sqlite3_int64 old_edge)
{
/*
/ defensive programming: carefully ensuring that lwgeom could
/ never shift Edges' start/end points after computing ModSplit
/ 3D topology
*/
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    double x1s;
    double y1s;
    double z1s;
    double x1e;
    double y1e;
    double z1e;
    double x2s;
    double y2s;
    double z2s;
    double x2e;
    double y2e;
    double z2e;
    sqlite3_int64 new_edge = sqlite3_last_insert_rowid (topo->db_handle);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("SELECT ST_X(ST_StartPoint(geom)), ST_Y(ST_StartPoint(geom)), "
	 "ST_Z(ST_StartPoint(geom)), ST_X(ST_EndPoint(geom)), ST_Y(ST_EndPoint(geom)), "
	 "ST_Z(ST_EndPoint(geom)) FROM \"%s\" WHERE edge_id = ?", xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, old_edge);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		x1s = sqlite3_column_double (stmt, 0);
		y1s = sqlite3_column_double (stmt, 1);
		z1s = sqlite3_column_double (stmt, 2);
		x1e = sqlite3_column_double (stmt, 3);
		y1e = sqlite3_column_double (stmt, 4);
		z1e = sqlite3_column_double (stmt, 5);
	    }
	  else
	      goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, new_edge);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		x2s = sqlite3_column_double (stmt, 0);
		y2s = sqlite3_column_double (stmt, 1);
		z2s = sqlite3_column_double (stmt, 2);
		x2e = sqlite3_column_double (stmt, 3);
		y2e = sqlite3_column_double (stmt, 4);
		z2e = sqlite3_column_double (stmt, 5);
	    }
	  else
	      goto error;
      }
    if (x1s == x2e && y1s == y2e && z1s == z2e)
      {
	  /* just silencing stupid compiler warnings */
	  ;
      }
    if (x1e == x2s && y1e == y2s && z1e == z2s)
      {
	  if (x1e != pt->X || y1e != pt->Y || z1e != pt->Z)
	      goto fixme;
      }
  error:
    sqlite3_finalize (stmt);
    return;

  fixme:
    sqlite3_finalize (stmt);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET geom = ST_SetEndPoint(geom, MakePointZ(?, ?, ?)) WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->X);
    sqlite3_bind_double (stmt, 2, pt->Y);
    sqlite3_bind_double (stmt, 3, pt->Z);
    sqlite3_bind_int64 (stmt, 4, old_edge);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	goto error2;

    sqlite3_finalize (stmt);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET geom = ST_SetStartPoint(geom, MakePointZ(?, ?, ?)) WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->X);
    sqlite3_bind_double (stmt, 2, pt->Y);
    sqlite3_bind_double (stmt, 3, pt->Z);
    sqlite3_bind_int64 (stmt, 4, new_edge);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	goto error2;

  error2:
    sqlite3_finalize (stmt);
    return;
}

static void
do_check_mod_split_edge (struct gaia_topology *topo, gaiaPointPtr pt,
			 sqlite3_int64 old_edge)
{
/*
/ defensive programming: carefully ensuring that lwgeom could
/ never shift Edges' start/end points after computing ModSplit
/ 2D topology
*/
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    double x1s;
    double y1s;
    double x1e;
    double y1e;
    double x2s;
    double y2s;
    double x2e;
    double y2e;
    sqlite3_int64 new_edge;
    if (topo->has_z)
      {
	  do_check_mod_split_edge3d (topo, pt, old_edge);
	  return;
      }

    new_edge = sqlite3_last_insert_rowid (topo->db_handle);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("SELECT ST_X(ST_StartPoint(geom)), ST_Y(ST_StartPoint(geom)), "
	 "ST_X(ST_EndPoint(geom)), ST_Y(ST_EndPoint(geom)) FROM \"%s\" WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, old_edge);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		x1s = sqlite3_column_double (stmt, 0);
		y1s = sqlite3_column_double (stmt, 1);
		x1e = sqlite3_column_double (stmt, 2);
		y1e = sqlite3_column_double (stmt, 3);
	    }
	  else
	      goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, new_edge);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		x2s = sqlite3_column_double (stmt, 0);
		y2s = sqlite3_column_double (stmt, 1);
		x2e = sqlite3_column_double (stmt, 2);
		y2e = sqlite3_column_double (stmt, 3);
	    }
	  else
	      goto error;
      }
    if (x1s == x2e && y1s == y2e)
      {
	  /* just silencing stupid compiler warnings */
	  ;
      }
    if (x1e == x2s && y1e == y2s)
      {
	  if (x1e != pt->X || y1e != pt->Y)
	      goto fixme;
      }
  error:
    sqlite3_finalize (stmt);
    return;

  fixme:
    sqlite3_finalize (stmt);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET geom = ST_SetEndPoint(geom, MakePoint(?, ?)) WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->X);
    sqlite3_bind_double (stmt, 2, pt->Y);
    sqlite3_bind_int64 (stmt, 3, old_edge);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	goto error2;

    sqlite3_finalize (stmt);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET geom = ST_SetStartPoint(geom, MakePoint(?, ?)) WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->X);
    sqlite3_bind_double (stmt, 2, pt->Y);
    sqlite3_bind_int64 (stmt, 3, new_edge);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	goto error2;

  error2:
    sqlite3_finalize (stmt);
    return;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaModEdgeSplit (GaiaTopologyAccessorPtr accessor,
		  sqlite3_int64 edge, gaiaPointPtr pt, int skip_checks)
{
/* LWT wrapper - ModEdgeSplit */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_ModEdgeSplit ((LWT_TOPOLOGY *) (topo->lwt_topology), edge, lw_pt,
			  skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoint_free (lw_pt);

    if (ret > 0)
	do_check_mod_split_edge (topo, pt, edge);

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaNewEdgesSplit (GaiaTopologyAccessorPtr accessor,
		   sqlite3_int64 edge, gaiaPointPtr pt, int skip_checks)
{
/* LWT wrapper - NewEdgesSplit */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_NewEdgesSplit ((LWT_TOPOLOGY *) (topo->lwt_topology), edge, lw_pt,
			   skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoint_free (lw_pt);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddEdgeModFace (GaiaTopologyAccessorPtr accessor,
		    sqlite3_int64 start_node, sqlite3_int64 end_node,
		    gaiaLinestringPtr ln, int skip_checks)
{
/* LWT wrapper - AddEdgeModFace */
    sqlite3_int64 ret;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddEdgeModFace ((LWT_TOPOLOGY *) (topo->lwt_topology), start_node,
			    end_node, lw_line, skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddEdgeNewFaces (GaiaTopologyAccessorPtr accessor,
		     sqlite3_int64 start_node, sqlite3_int64 end_node,
		     gaiaLinestringPtr ln, int skip_checks)
{
/* LWT wrapper - AddEdgeNewFaces */
    sqlite3_int64 ret;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddEdgeNewFaces ((LWT_TOPOLOGY *) (topo->lwt_topology), start_node,
			     end_node, lw_line, skip_checks);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaRemEdgeNewFace (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id)
{
/* LWT wrapper - RemEdgeNewFace */
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_RemEdgeNewFace ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaRemEdgeModFace (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id)
{
/* LWT wrapper - RemEdgeModFace */
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret = lwt_RemEdgeModFace ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaNewEdgeHeal (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id1,
		 sqlite3_int64 edge_id2)
{
/* LWT wrapper - NewEdgeHeal */
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_NewEdgeHeal ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id1,
			 edge_id2);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaModEdgeHeal (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id1,
		 sqlite3_int64 edge_id2)
{
/* LWT wrapper - ModEdgeHeal */
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_ModEdgeHeal ((LWT_TOPOLOGY *) (topo->lwt_topology), edge_id1,
			 edge_id2);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaGetFaceGeometry (GaiaTopologyAccessorPtr accessor, sqlite3_int64 face)
{
/* LWT wrapper - GetFaceGeometry */
    LWGEOM *result = NULL;
    LWPOLY *lwpoly;
    int has_z = 0;
    POINTARRAY *pa;
    POINT4D pt4d;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    gaiaGeomCollPtr geom;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int dimension_model;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (topo->inside_lwt_callback == 0)
      {
	  /* locking the semaphore if not already inside a callback context */
	  splite_lwgeom_semaphore_lock ();
	  splite_lwgeomtopo_init ();
	  gaiaResetLwGeomMsg ();
      }

    result = lwt_GetFaceGeometry ((LWT_TOPOLOGY *) (topo->lwt_topology), face);

    if (topo->inside_lwt_callback == 0)
      {
	  /* unlocking the semaphore if not already inside a callback context */
	  splite_lwgeom_init ();
	  splite_lwgeom_semaphore_unlock ();
      }

    if (result == NULL)
	return NULL;

/* converting the result as a Gaia Geometry */
    lwpoly = (LWPOLY *) result;
    if (lwpoly->nrings <= 0)
      {
	  /* empty geometry */
	  lwgeom_free (result);
	  return NULL;
      }
    pa = lwpoly->rings[0];
    if (pa->npoints <= 0)
      {
	  /* empty geometry */
	  lwgeom_free (result);
	  return NULL;
      }
    if (FLAGS_GET_Z (pa->flags))
	has_z = 1;
    if (has_z)
      {
	  dimension_model = GAIA_XY_Z;
	  geom = gaiaAllocGeomCollXYZ ();
      }
    else
      {
	  dimension_model = GAIA_XY;
	  geom = gaiaAllocGeomColl ();
      }
    pg = gaiaAddPolygonToGeomColl (geom, pa->npoints, lwpoly->nrings - 1);
    rng = pg->Exterior;
    for (iv = 0; iv < pa->npoints; iv++)
      {
	  /* copying Exterior Ring vertices */
	  getPoint4d_p (pa, iv, &pt4d);
	  x = pt4d.x;
	  y = pt4d.y;
	  if (has_z)
	      z = pt4d.z;
	  else
	      z = 0.0;
	  if (dimension_model == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
	    }
	  else
	    {
		gaiaSetPoint (rng->Coords, iv, x, y);
	    }
      }
    for (ib = 1; ib < lwpoly->nrings; ib++)
      {
	  has_z = 0;
	  pa = lwpoly->rings[ib];
	  if (FLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  rng = gaiaAddInteriorRing (pg, ib - 1, pa->npoints);
	  for (iv = 0; iv < pa->npoints; iv++)
	    {
		/* copying Exterior Ring vertices */
		getPoint4d_p (pa, iv, &pt4d);
		x = pt4d.x;
		y = pt4d.y;
		if (has_z)
		    z = pt4d.z;
		else
		    z = 0.0;
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
		  }
		else
		  {
		      gaiaSetPoint (rng->Coords, iv, x, y);
		  }
	    }
      }
    lwgeom_free (result);
    geom->DeclaredType = GAIA_POLYGON;
    geom->Srid = topo->srid;
    return geom;
}

static int
do_check_create_faceedges_table (GaiaTopologyAccessorPtr accessor)
{
/* attemtping to create or validate the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int exists = 0;
    int ok_face_id = 0;
    int ok_sequence = 0;
    int ok_edge_id = 0;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* testing for an already existing table */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("PRAGMA TEMP.table_info(\"%s\")", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *name = results[(i * columns) + 1];
	  const char *type = results[(i * columns) + 2];
	  const char *notnull = results[(i * columns) + 3];
	  const char *dflt_value = results[(i * columns) + 4];
	  const char *pk = results[(i * columns) + 5];
	  if (strcmp (name, "face_id") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "1") == 0)
	      ok_face_id = 1;
	  if (strcmp (name, "sequence") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "2") == 0)
	      ok_sequence = 1;
	  if (strcmp (name, "edge_id") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "0") == 0)
	      ok_edge_id = 1;
	  exists = 1;
      }
    sqlite3_free_table (results);
    if (ok_face_id && ok_sequence && ok_edge_id)
	return 1;		/* already existing and valid */

    if (exists)
	return 0;		/* already existing but invalid */

/* attempting to create the table */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TEMP TABLE \"%s\" (\n\tface_id INTEGER NOT NULL,\n"
	 "\tsequence INTEGER NOT NULL,\n\tedge_id INTEGER NOT NULL,\n"
	 "\tCONSTRAINT pk_topo_facee_edges PRIMARY KEY (face_id, sequence))",
	 xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

    return 1;
}

static int
do_populate_faceedges_table (GaiaTopologyAccessorPtr accessor,
			     sqlite3_int64 face, LWT_ELEMID * edges,
			     int num_edges)
{
/* populating the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    sqlite3_stmt *stmt = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* deleting all rows belonging to Face (if any) */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM TEMP.\"%s\" WHERE face_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, face);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* preparing the INSERT statement */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (face_id, sequence, edge_id) VALUES (?, ?, ?)",
	 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    for (i = 0; i < num_edges; i++)
      {
	  /* inserting all Face/Edges */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, face);
	  sqlite3_bind_int (stmt, 2, i + 1);
	  sqlite3_bind_int64 (stmt, 3, *(edges + i));
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

GAIATOPO_DECLARE int
gaiaGetFaceEdges (GaiaTopologyAccessorPtr accessor, sqlite3_int64 face)
{
/* LWT wrapper - GetFaceEdges */
    LWT_ELEMID *edges = NULL;
    int num_edges;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    /* locking the semaphore */
    splite_lwgeom_semaphore_lock ();
    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();

    num_edges =
	lwt_GetFaceEdges ((LWT_TOPOLOGY *) (topo->lwt_topology), face, &edges);

    /* unlocking the semaphore */
    splite_lwgeom_init ();
    splite_lwgeom_semaphore_unlock ();

    if (num_edges < 0)
	return 0;

    if (num_edges > 0)
      {
	  /* attemtping to create or validate the target table */
	  if (!do_check_create_faceedges_table (accessor))
	    {
		lwfree (edges);
		return 0;
	    }

	  /* populating the target table */
	  if (!do_populate_faceedges_table (accessor, face, edges, num_edges))
	    {
		lwfree (edges);
		return 0;
	    }
      }
    lwfree (edges);
    return 1;
}

static int
do_check_create_validate_topogeo_table (GaiaTopologyAccessorPtr accessor)
{
/* attemtping to create or validate the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    char *errMsg = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* finalizing all prepared Statements */
    finalize_all_topo_prepared_stmts (topo->cache);

/* attempting to drop the table (just in case if it already exists) */
    table = sqlite3_mprintf ("%s_validate_topogeo", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS temp.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    create_all_topo_prepared_stmts (topo->cache);	/* recreating prepared stsms */
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidSpatialNet exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* attempting to create the table */
    table = sqlite3_mprintf ("%s_validate_topogeo", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TEMP TABLE \"%s\" (\n\terror TEXT,\n"
	 "\tprimitive1 INTEGER,\n\tprimitive2 INTEGER)", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidateTopoGeo exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

    return 1;
}

static int
do_topo_check_coincident_nodes (GaiaTopologyAccessorPtr accessor,
				sqlite3_stmt * stmt)
{
/* checking for coincident nodes */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT n1.node_id, n2.node_id FROM MAIN.\"%s\" AS n1 "
			 "JOIN MAIN.\"%s\" AS n2 ON (n1.node_id <> n2.node_id AND "
			 "ST_Equals(n1.geom, n2.geom) = 1 AND n2.node_id IN "
			 "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = n1.geom))",
			 xtable, xtable, table);
    sqlite3_free (table);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - CoicidentNodes error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 node_id1 = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id2 = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "coincident nodes", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, node_id1);
		sqlite3_bind_int64 (stmt, 3, node_id2);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #1 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - CoicidentNodes step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_edge_node (GaiaTopologyAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for edge-node crossing */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT e.edge_id, n.node_id FROM MAIN.\"%s\" AS e "
			   "JOIN MAIN.\"%s\" AS n ON (ST_Distance(e.geom, n.geom) <= 0 "
			   "AND ST_Disjoint(ST_StartPoint(e.geom), n.geom) = 1 AND "
			   "ST_Disjoint(ST_EndPoint(e.geom), n.geom) = 1 AND n.node_id IN "
			   "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND "
			   "f_geometry_column = 'geom' AND search_frame = e.geom))",
			   xtable1, xtable2, table);
    sqlite3_free (table);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - EdgeCrossedNode error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "edge crossed node", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, node_id);
		sqlite3_bind_int64 (stmt, 3, edge_id);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #2 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - EdgeCrossedNode step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_non_simple (GaiaTopologyAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for non-simple edges */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT edge_id FROM MAIN.\"%s\" WHERE ST_IsSimple(geom) = 0", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - NonSimpleEdge error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "edge not simple", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, edge_id);
		sqlite3_bind_null (stmt, 3);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #3 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - NonSimpleEdge step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_edge_edge (GaiaTopologyAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for edge-edge crossing */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT e1.edge_id, e2.edge_id FROM MAIN.\"%s\" AS e1 "
			 "JOIN MAIN.\"%s\" AS e2 ON (e1.edge_id <> e2.edge_id AND "
			 "ST_Crosses(e1.geom, e2.geom) = 1 AND e2.edge_id IN "
			 "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = e1.geom))",
			 xtable, xtable, table);
    sqlite3_free (table);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - EdgeCrossesEdge error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 edge_id1 = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 edge_id2 = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "edge crosses edge", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, edge_id1);
		sqlite3_bind_int64 (stmt, 3, edge_id2);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #4 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - EdgeCrossesEdge step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_start_nodes (GaiaTopologyAccessorPtr accessor,
			   sqlite3_stmt * stmt)
{
/* checking for edges mismatching start nodes */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT e.edge_id, e.start_node FROM MAIN.\"%s\" AS e "
			 "JOIN MAIN.\"%s\" AS n ON (e.start_node = n.node_id) "
			 "WHERE ST_Disjoint(ST_StartPoint(e.geom), n.geom) = 1",
			 xtable1, xtable2);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - StartNodes error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "geometry start mismatch", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, edge_id);
		sqlite3_bind_int64 (stmt, 3, node_id);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #5 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - StartNodes step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_end_nodes (GaiaTopologyAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for edges mismatching end nodes */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT e.edge_id, e.end_node FROM MAIN.\"%s\" AS e "
			   "JOIN MAIN.\"%s\" AS n ON (e.end_node = n.node_id) "
			   "WHERE ST_Disjoint(ST_EndPoint(e.geom), n.geom) = 1",
			   xtable1, xtable2);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidateTopoGeo() - EndNodes error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "geometry end mismatch", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, edge_id);
		sqlite3_bind_int64 (stmt, 3, node_id);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #6 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - EndNodes step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_face_no_edges (GaiaTopologyAccessorPtr accessor,
			     sqlite3_stmt * stmt)
{
/* checking for faces with no edges */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT f.face_id, Count(e1.edge_id) AS cnt1, "
			   "Count(e2.edge_id) AS cnt2 FROM MAIN.\"%s\" AS f "
			   "LEFT JOIN MAIN.\"%s\" AS e1 ON (f.face_id = e1.left_face) "
			   "LEFT JOIN MAIN.\"%s\" AS e2 ON (f.face_id = e2.right_face) "
			   "GROUP BY f.face_id HAVING cnt1 = 0 AND cnt2 = 0",
			   xtable1, xtable2, xtable2);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - FaceNoEdges error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 face_id = sqlite3_column_int64 (stmt_in, 0);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "face without edges", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, face_id);
		sqlite3_bind_null (stmt, 3);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #7 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - FaceNoEdges step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_no_universal_face (GaiaTopologyAccessorPtr accessor,
				 sqlite3_stmt * stmt)
{
/* checking for missing universal face */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int count = 0;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\" WHERE face_id = 0",
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
	  count = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);

    if (count <= 0)
      {
	  /* reporting the error */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, "no universal face", -1, SQLITE_STATIC);
	  sqlite3_bind_null (stmt, 2);
	  sqlite3_bind_null (stmt, 3);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() insert #8 error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    return 1;
}

static int
do_topo_check_create_aux_faces (GaiaTopologyAccessorPtr accessor)
{
/* creating the aux-Face temp table */
    char *table;
    char *xtable;
    char *sql;
    char *errMsg;
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* creating the aux-face Temp Table */
    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, getpid ());
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TEMPORARY TABLE \"%s\" (\n"
			   "\tface_id INTEGER PRIMARY KEY,\n\tgeom BLOB)",
			   xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("CREATE TEMPORARY TABLE aux_face - error: %s\n",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* creating the exotic spatial index */
    table =
	sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name,
			 getpid ());
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIRTUAL TABLE temp.\"%s\" USING RTree "
			   "(id_face, x_min, x_max, y_min, y_max)", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("CREATE TEMPORARY TABLE aux_face - error: %s\n",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

static int
do_topo_check_build_aux_faces (GaiaTopologyAccessorPtr accessor,
			       sqlite3_stmt * stmt)
{
/* populating the aux-face Temp Table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    sqlite3_stmt *stmt_rtree = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* preparing the input SQL statement */
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT face_id, ST_GetFaceGeometry(%Q, face_id) "
			   "FROM MAIN.\"%s\" WHERE face_id <> 0",
			   topo->topology_name, xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - GetFaceGeometry error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, getpid ());
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (face_id, geom) VALUES (?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidateTopoGeo() - AuxFace error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the RTree SQL statement */
    table =
	sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name,
			 getpid ());
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("INSERT INTO TEMP.\"%s\" "
			   "(id_face, x_min, x_max, y_min, y_max) VALUES (?, ?, ?, ?, ?)",
			   xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rtree,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - AuxFaceRTree error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		gaiaGeomCollPtr geom = NULL;
		const unsigned char *blob;
		int blob_sz;
		sqlite3_int64 face_id = sqlite3_column_int64 (stmt_in, 0);
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt_in, 1);
		      blob_sz = sqlite3_column_bytes (stmt_in, 1);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		  }
		if (geom == NULL)
		  {
		      /* reporting the error */
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, "invalid face geometry", -1,
					 SQLITE_STATIC);
		      sqlite3_bind_int64 (stmt, 2, face_id);
		      sqlite3_bind_null (stmt, 3);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("ST_ValidateTopoGeo() insert #9 error: \"%s\"",
				 sqlite3_errmsg (topo->db_handle));
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      double xmin = geom->MinX;
		      double xmax = geom->MaxX;
		      double ymin = geom->MinY;
		      double ymax = geom->MaxY;
		      gaiaFreeGeomColl (geom);
		      /* inserting into AuxFace */
		      sqlite3_reset (stmt_out);
		      sqlite3_clear_bindings (stmt_out);
		      sqlite3_bind_int64 (stmt_out, 1, face_id);
		      sqlite3_bind_blob (stmt_out, 2, blob, blob_sz,
					 SQLITE_STATIC);
		      ret = sqlite3_step (stmt_out);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("ST_ValidateTopoGeo() insert #10 error: \"%s\"",
				 sqlite3_errmsg (topo->db_handle));
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		      /* updating the AuxFaceRTree */
		      sqlite3_reset (stmt_rtree);
		      sqlite3_clear_bindings (stmt_rtree);
		      sqlite3_bind_int64 (stmt_rtree, 1, face_id);
		      sqlite3_bind_double (stmt_rtree, 2, xmin);
		      sqlite3_bind_double (stmt_rtree, 3, xmax);
		      sqlite3_bind_double (stmt_rtree, 4, ymin);
		      sqlite3_bind_double (stmt_rtree, 5, ymax);
		      ret = sqlite3_step (stmt_rtree);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("ST_ValidateTopoGeo() insert #11 error: \"%s\"",
				 sqlite3_errmsg (topo->db_handle));
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - GetFaceGeometry step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    sqlite3_finalize (stmt_rtree);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out == NULL)
	sqlite3_finalize (stmt_out);
    if (stmt_rtree == NULL)
	sqlite3_finalize (stmt_rtree);
    return 0;
}

static int
do_topo_check_overlapping_faces (GaiaTopologyAccessorPtr accessor,
				 sqlite3_stmt * stmt)
{
/* checking for overlapping faces */
    char *sql;
    char *table;
    char *xtable;
    char *rtree;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;


    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, getpid ());
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name,
			     getpid ());
    rtree = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT a.face_id, b.face_id FROM TEMP.\"%s\" AS a, TEMP.\"%s\" AS b "
	 "WHERE a.geom IS NOT NULL AND a.face_id <> b.face_id AND ST_Overlaps(a.geom, b.geom) = 1 "
	 "AND b.face_id IN (SELECT id_face FROM TEMP.\"%s\" WHERE x_min <= MbrMaxX(a.geom) "
	 "AND x_max >= MbrMinX(a.geom) AND y_min <= MbrMaxY(a.geom) AND y_max >= MbrMinY(a.geom))",
	 xtable, xtable, rtree);
    free (xtable);
    free (rtree);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - OverlappingFaces error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 face_id1 = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 face_id2 = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "face overlaps face", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, face_id1);
		sqlite3_bind_int64 (stmt, 3, face_id2);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #12 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - OverlappingFaces step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_face_within_face (GaiaTopologyAccessorPtr accessor,
				sqlite3_stmt * stmt)
{
/* checking for face-within-face */
    char *sql;
    char *table;
    char *xtable;
    char *rtree;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;


    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, getpid ());
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name,
			     getpid ());
    rtree = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT a.face_id, b.face_id FROM TEMP.\"%s\" AS a, TEMP.\"%s\" AS b "
	 "WHERE a.geom IS NOT NULL AND a.face_id <> b.face_id AND ST_Within(a.geom, b.geom) = 1 "
	 "AND b.face_id IN (SELECT id_face FROM TEMP.\"%s\" WHERE x_min <= MbrMaxX(a.geom) "
	 "AND x_max >= MbrMinX(a.geom) AND y_min <= MbrMaxY(a.geom) AND y_max >= MbrMinY(a.geom))",
	 xtable, xtable, rtree);
    free (xtable);
    free (rtree);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - FaceWithinFace error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
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
		sqlite3_int64 face_id1 = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 face_id2 = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "face within face", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, face_id1);
		sqlite3_bind_int64 (stmt, 3, face_id2);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #13 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - FaceWithinFace step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_topo_check_drop_aux_faces (GaiaTopologyAccessorPtr accessor)
{
/* dropping the aux-Face temp table */
    char *table;
    char *xtable;
    char *sql;
    char *errMsg;
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* finalizing all prepared Statements */
    finalize_all_topo_prepared_stmts (topo->cache);

/* dropping the aux-face Temp Table */
    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, getpid ());
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE TEMP.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    create_all_topo_prepared_stmts (topo->cache);	/* recreating prepared stsms */
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("DROP TABLE temp.aux_face - error: %s\n",
				       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* dropping the aux-face Temp RTree */
    table =
	sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name,
			 getpid ());
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE TEMP.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("DROP TABLE temp.aux_face_rtree - error: %s\n",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaValidateTopoGeo (GaiaTopologyAccessorPtr accessor)
{
/* generating a validity report for a given Topology */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (!do_check_create_validate_topogeo_table (accessor))
	return 0;

    table = sqlite3_mprintf ("%s_validate_topogeo", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (error, primitive1, primitive2) VALUES (?, ?, ?)",
	 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_ValidateTopoGeo error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (!do_topo_check_coincident_nodes (accessor, stmt))
	goto error;

    if (!do_topo_check_edge_node (accessor, stmt))
	goto error;

    if (!do_topo_check_non_simple (accessor, stmt))
	goto error;

    if (!do_topo_check_edge_edge (accessor, stmt))
	goto error;

    if (!do_topo_check_start_nodes (accessor, stmt))
	goto error;

    if (!do_topo_check_end_nodes (accessor, stmt))
	goto error;

    if (!do_topo_check_face_no_edges (accessor, stmt))
	goto error;

    if (!do_topo_check_no_universal_face (accessor, stmt))
	goto error;

    if (!do_topo_check_create_aux_faces (accessor))
	goto error;

    if (!do_topo_check_build_aux_faces (accessor, stmt))
	goto error;

    if (!do_topo_check_overlapping_faces (accessor, stmt))
	goto error;

    if (!do_topo_check_face_within_face (accessor, stmt))
	goto error;

    if (!do_topo_check_drop_aux_faces (accessor))
	goto error;

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaGetNodeByPoint (GaiaTopologyAccessorPtr accessor, gaiaPointPtr pt,
		    double tolerance)
{
/* LWT wrapper - GetNodeByPoint */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_GetNodeByPoint ((LWT_TOPOLOGY *) (topo->lwt_topology), lw_pt,
			    tolerance);
    lwpoint_free (lw_pt);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaGetEdgeByPoint (GaiaTopologyAccessorPtr accessor, gaiaPointPtr pt,
		    double tolerance)
{
/* LWT wrapper - GetEdgeByPoint */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_GetEdgeByPoint ((LWT_TOPOLOGY *) (topo->lwt_topology), lw_pt,
			    tolerance);
    lwpoint_free (lw_pt);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaGetFaceByPoint (GaiaTopologyAccessorPtr accessor, gaiaPointPtr pt,
		    double tolerance)
{
/* LWT wrapper - GetFaceByPoint */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_GetFaceByPoint ((LWT_TOPOLOGY *) (topo->lwt_topology), lw_pt,
			    tolerance);
    lwpoint_free (lw_pt);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaTopoGeo_AddPoint (GaiaTopologyAccessorPtr accessor, gaiaPointPtr pt,
		      double tolerance)
{
/* LWT wrapper - AddPoint */
    sqlite3_int64 ret;
    int has_z = 0;
    LWPOINT *lw_pt;
    POINTARRAY *pa;
    POINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (pa, 0, &point);
    lw_pt = lwpoint_construct (topo->srid, NULL, pa);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    ret =
	lwt_AddPoint ((LWT_TOPOLOGY *) (topo->lwt_topology), lw_pt, tolerance);
    lwpoint_free (lw_pt);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    return ret;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_AddLineString (GaiaTopologyAccessorPtr accessor,
			   gaiaLinestringPtr ln, double tolerance,
			   sqlite3_int64 ** edge_ids, int *ids_count)
{
/* LWT wrapper - AddLinestring */
    int ret = 0;
    LWT_ELEMID *edgeids;
    int nedges;
    int i;
    sqlite3_int64 *ids;
    LWLINE *lw_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_line = gaia_convert_linestring_to_lwline (ln, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    edgeids =
	lwt_AddLine ((LWT_TOPOLOGY *) (topo->lwt_topology), lw_line, tolerance,
		     &nedges);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwline_free (lw_line);
    if (edgeids != NULL)
      {
	  ids = malloc (sizeof (sqlite3_int64) * nedges);
	  for (i = 0; i < nedges; i++)
	      ids[i] = edgeids[i];
	  *edge_ids = ids;
	  *ids_count = nedges;
	  ret = 1;
	  lwfree (edgeids);
      }
    return ret;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_AddPolygon (GaiaTopologyAccessorPtr accessor, gaiaPolygonPtr pg,
			double tolerance, sqlite3_int64 ** face_ids,
			int *ids_count)
{
/* LWT wrapper - AddPolygon */
    int ret = 0;
    LWT_ELEMID *faceids;
    int nfaces;
    int i;
    sqlite3_int64 *ids;
    LWPOLY *lw_polyg;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    lw_polyg = gaia_convert_polygon_to_lwpoly (pg, topo->srid, topo->has_z);

/* locking the semaphore */
    splite_lwgeom_semaphore_lock ();

    splite_lwgeomtopo_init ();
    gaiaResetLwGeomMsg ();
    faceids =
	lwt_AddPolygon ((LWT_TOPOLOGY *) (topo->lwt_topology), lw_polyg,
			tolerance, &nfaces);
    splite_lwgeom_init ();

/* unlocking the semaphore */
    splite_lwgeom_semaphore_unlock ();

    lwpoly_free (lw_polyg);
    if (faceids != NULL)
      {
	  ids = malloc (sizeof (sqlite3_int64) * nfaces);
	  for (i = 0; i < nfaces; i++)
	      ids[i] = faceids[i];
	  *face_ids = ids;
	  *ids_count = nfaces;
	  ret = 1;
	  lwfree (faceids);
      }
    return ret;
}

static void
do_split_line (gaiaGeomCollPtr geom, gaiaDynamicLinePtr dyn)
{
/* inserting a new linestring into the collection of split lines */
    int points = 0;
    int iv = 0;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;

    pt = dyn->First;
    while (pt != NULL)
      {
	  /* counting how many points */
	  points++;
	  pt = pt->Next;
      }

    ln = gaiaAddLinestringToGeomColl (geom, points);
    pt = dyn->First;
    while (pt != NULL)
      {
	  /* copying all points */
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (ln->Coords, iv, pt->X, pt->Y, pt->Z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (ln->Coords, iv, pt->X, pt->Y, pt->M);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (ln->Coords, iv, pt->X, pt->Y, pt->Z, pt->M);
	    }
	  else
	    {
		gaiaSetPoint (ln->Coords, iv, pt->X, pt->Y);
	    }
	  iv++;
	  pt = pt->Next;
      }
}

static void
do_geom_split_line (gaiaGeomCollPtr geom, gaiaLinestringPtr in,
		    int line_max_points, double max_length)
{
/* splitting a Linestring into a collection of shorter Linestrings */
    int iv;
    int count = 0;
    int split = 0;
    double tot_length = 0.0;
    gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();

    for (iv = 0; iv < in->Points; iv++)
      {
	  /* consuming all Points from the input Linestring */
	  double ox;
	  double oy;
	  double x;
	  double y;
	  double z = 0.0;
	  double m = 0.0;
	  if (in->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (in->Coords, iv, &x, &y, &z);
	    }
	  else if (in->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (in->Coords, iv, &x, &y, &m);
	    }
	  else if (in->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (in->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (in->Coords, iv, &x, &y);
	    }

	  split = 0;
	  if (max_length > 0.0)
	    {
		if (tot_length > max_length)
		    split = 1;
	    }
	  if (line_max_points > 0)
	    {
		if (count == line_max_points)
		    split = 1;
	    }
	  if (split && count >= 2)
	    {
		/* line break */
		double oz;
		double om;
		ox = dyn->Last->X;
		oy = dyn->Last->Y;
		if (in->DimensionModel == GAIA_XY_Z
		    || in->DimensionModel == GAIA_XY_Z_M)
		    oz = dyn->Last->Z;
		if (in->DimensionModel == GAIA_XY_M
		    || in->DimensionModel == GAIA_XY_Z_M)
		    om = dyn->Last->M;
		do_split_line (geom, dyn);
		gaiaFreeDynamicLine (dyn);
		dyn = gaiaAllocDynamicLine ();
		/* reinserting the last point */
		if (in->DimensionModel == GAIA_XY_Z)
		    gaiaAppendPointZToDynamicLine (dyn, ox, oy, oz);
		else if (in->DimensionModel == GAIA_XY_M)
		    gaiaAppendPointMToDynamicLine (dyn, ox, oy, om);
		else if (in->DimensionModel == GAIA_XY_Z_M)
		    gaiaAppendPointZMToDynamicLine (dyn, ox, oy, oz, om);
		else
		    gaiaAppendPointToDynamicLine (dyn, ox, oy);
		count = 1;
		tot_length = 0.0;
	    }

	  /* inserting a point */
	  if (in->DimensionModel == GAIA_XY_Z)
	      gaiaAppendPointZToDynamicLine (dyn, x, y, z);
	  else if (in->DimensionModel == GAIA_XY_M)
	      gaiaAppendPointMToDynamicLine (dyn, x, y, m);
	  else if (in->DimensionModel == GAIA_XY_Z_M)
	      gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
	  else
	      gaiaAppendPointToDynamicLine (dyn, x, y);
	  if (count > 0)
	    {
		if (max_length > 0.0)
		  {
		      double dist =
			  sqrt (((ox - x) * (ox - x)) + ((oy - y) * (oy - y)));
		      tot_length += dist;
		  }
	    }
	  ox = x;
	  oy = y;
	  count++;
      }

    if (dyn->First != NULL)
      {
	  /* flushing the last Line */
	  do_split_line (geom, dyn);
      }
    gaiaFreeDynamicLine (dyn);
}

static gaiaGeomCollPtr
do_linearize (gaiaGeomCollPtr geom)
{
/* attempts to transform Polygon Rings into a (multi)linestring */
    gaiaGeomCollPtr result;
    gaiaLinestringPtr new_ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int iv;
    int ib;
    double x;
    double y;
    double m;
    double z;
    if (!geom)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = geom->Srid;

    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* dissolving any POLYGON as simple LINESTRINGs (rings) */
	  rng = pg->Exterior;
	  new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* copying the EXTERIOR RING as LINESTRING */
		if (geom->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
		  }
		else if (geom->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
		  }
		else if (geom->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      gaiaSetPoint (new_ln->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* copying an INTERIOR RING as LINESTRING */
		      if (geom->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
			}
		      else if (geom->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
			}
		      else if (geom->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    gaiaSetPoint (new_ln->Coords, iv, x, y);
			}
		  }
	    }
	  pg = pg->Next;
      }
    if (result->FirstLinestring == NULL)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    return result;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaTopoGeo_SubdivideLines (gaiaGeomCollPtr geom, int line_max_points,
			    double max_length)
{
/* subdividing a (multi)Linestring into collection of simpler Linestrings */
    gaiaLinestringPtr ln;
    gaiaGeomCollPtr result;

    if (geom == NULL)
	return NULL;

    if (geom->FirstPoint != NULL)
	return NULL;
    if (geom->FirstLinestring == NULL && geom->FirstPolygon != NULL)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = geom->Srid;
    result->DeclaredType = GAIA_MULTILINESTRING;
    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  do_geom_split_line (result, ln, line_max_points, max_length);
	  ln = ln->Next;
      }

    if (geom->FirstPolygon != NULL)
      {
	  /* transforming all Polygon Rings into Linestrings */
	  gaiaGeomCollPtr pg_rings = do_linearize (geom);
	  if (pg_rings != NULL)
	    {
		ln = pg_rings->FirstLinestring;
		while (ln != NULL)
		  {
		      do_geom_split_line (result, ln, line_max_points,
					  max_length);
		      ln = ln->Next;
		  }
		gaiaFreeGeomColl (pg_rings);
	    }
      }
    return result;
}

TOPOLOGY_PRIVATE int
auxtopo_insert_into_topology (GaiaTopologyAccessorPtr accessor,
			      gaiaGeomCollPtr geom, double tolerance,
			      int line_max_points, double max_length)
{
/* processing all individual geometry items */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaGeomCollPtr g;
    gaiaGeomCollPtr split = NULL;
    gaiaGeomCollPtr pg_rings;
    sqlite3_int64 *ids;
    int ids_count;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    pt = geom->FirstPoint;
    while (pt != NULL)
      {
	  /* looping on Point items */
	  if (gaiaTopoGeo_AddPoint (accessor, pt, tolerance) < 0)
	      return 0;
	  pt = pt->Next;
      }

    if (line_max_points <= 0 && max_length <= 0.0)
	g = geom;
    else
      {
	  /* subdividing Linestrings */
	  split =
	      gaiaTopoGeo_SubdivideLines (geom, line_max_points, max_length);
	  if (split != NULL)
	      g = split;
	  else
	      g = geom;
      }
    ln = g->FirstLinestring;
    while (ln != NULL)
      {
	  /* looping on Linestrings items */
	  if (gaiaTopoGeo_AddLineString
	      (accessor, ln, tolerance, &ids, &ids_count) == 0)
	      return 0;
	  if (ids != NULL)
	      free (ids);
	  ln = ln->Next;
      }
    if (split != NULL)
	gaiaFreeGeomColl (split);
    split = NULL;

/* transforming all Polygon Rings into Linestrings */
    pg_rings = do_linearize (geom);
    if (pg_rings != NULL)
      {
	  if (line_max_points <= 0 && max_length <= 0.0)
	      g = pg_rings;
	  else
	    {
		/* subdividng Linestrings */
		split =
		    gaiaTopoGeo_SubdivideLines (pg_rings, line_max_points,
						max_length);
		if (split != NULL)
		    g = split;
		else
		    g = pg_rings;
	    }
	  ln = g->FirstLinestring;
	  while (ln != NULL)
	    {
		/* looping on Linestrings items */
		if (gaiaTopoGeo_AddLineString
		    (accessor, ln, tolerance, &ids, &ids_count) == 0)
		    return 0;
		if (ids != NULL)
		    free (ids);
		ln = ln->Next;
	    }
	  gaiaFreeGeomColl (pg_rings);
	  if (split != NULL)
	      gaiaFreeGeomColl (split);
	  split = NULL;
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_FromGeoTable (GaiaTopologyAccessorPtr accessor,
			  const char *db_prefix, const char *table,
			  const char *column, double tolerance,
			  int line_max_points, double max_length)
{
/* attempting to import a whole GeoTable into a Topology-Geometry */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *xprefix;
    char *xtable;
    char *xcolumn;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;

    if (topo == NULL)
	return 0;
    if (topo->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (topo->cache);
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }

/* building the SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    xcolumn = gaiaDoubleQuotedSql (column);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\"", xcolumn,
			 xprefix, xtable);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_FromGeoTable error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_NULL)
		    continue;
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz, gpkg_mode,
						       gpkg_amphibious);
		      if (geom != NULL)
			{
			    if (!auxtopo_insert_into_topology
				(accessor, geom, tolerance, line_max_points,
				 max_length))
			      {
				  gaiaFreeGeomColl (geom);
				  goto error;
			      }
			    gaiaFreeGeomColl (geom);
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("TopoGeo_FromGeoTable error: Invalid Geometry");
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_FromGeoTable error: not a BLOB value");
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_FromGeoTable error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
insert_into_dustbin (sqlite3 * sqlite, const void *cache, sqlite3_stmt * stmt,
		     sqlite3_stmt * stmt_dustbin, const char *message,
		     double tolerance, int *count)
{
/* failing feature: inserting a reference into the dustbin table */
    int icol;
    int maxcol;
    int ret;

    start_topo_savepoint (sqlite, cache);
    sqlite3_reset (stmt_dustbin);
    sqlite3_clear_bindings (stmt_dustbin);
    maxcol = sqlite3_column_count (stmt);
    for (icol = 1; icol < maxcol; icol++)
      {
	  /* binding column values */
	  switch (sqlite3_column_type (stmt, icol))
	    {
	    case SQLITE_INTEGER:
		sqlite3_bind_int64 (stmt_dustbin, icol,
				    sqlite3_column_int64 (stmt, icol));
		break;
	    case SQLITE_FLOAT:
		sqlite3_bind_double (stmt_dustbin, icol,
				     sqlite3_column_double (stmt, icol));
		break;
	    case SQLITE_TEXT:
		sqlite3_bind_text (stmt_dustbin, icol,
				   (const char *) sqlite3_column_text (stmt,
								       icol),
				   sqlite3_column_bytes (stmt, icol),
				   SQLITE_STATIC);
		break;
	    case SQLITE_BLOB:
		sqlite3_bind_blob (stmt_dustbin, icol,
				   sqlite3_column_blob (stmt, icol),
				   sqlite3_column_bytes (stmt, icol),
				   SQLITE_STATIC);
		break;
	    default:
		sqlite3_bind_null (stmt_dustbin, icol);
		break;
	    }
      }
/* binding the error message */
    sqlite3_bind_text (stmt_dustbin, maxcol - 1, message, strlen (message),
		       SQLITE_STATIC);
    sqlite3_bind_double (stmt_dustbin, maxcol, tolerance);
    ret = sqlite3_step (stmt_dustbin);

/* inserting the row into the dustbin table */
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  release_topo_savepoint (sqlite, cache);
	  *count += 1;
	  return 1;
      }

    /* some unexpected error occurred */
    spatialite_e ("TopoGeo_FromGeoTableExt error: \"%s\"",
		  sqlite3_errmsg (sqlite));
    rollback_topo_savepoint (sqlite, cache);
    return 0;
}

static int
do_FromGeoTableExtended_block (GaiaTopologyAccessorPtr accessor,
			       sqlite3_stmt * stmt, sqlite3_stmt * stmt_dustbin,
			       double tolerance, int line_max_points,
			       double max_length, sqlite3_int64 start,
			       sqlite3_int64 * last, sqlite3_int64 * invalid,
			       int *dustbin_count)
{
/* attempting to import a whole block of input features */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    int ret;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    int totcnt = 0;
    sqlite3_int64 last_rowid;

    if (topo->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (topo->cache);
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }

    start_topo_savepoint (topo->db_handle, topo->cache);

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, start);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt, 0);
		int igeo = sqlite3_column_count (stmt) - 1;	/* geometry always corresponds to the last resultset column */
		if (rowid == *invalid)
		  {
		      /* succesfully recovered a previously failing block */
		      release_topo_savepoint (topo->db_handle, topo->cache);
		      *last = last_rowid;
		      return 1;
		  }
		totcnt++;
		if (totcnt > 256)
		  {
		      /* succesfully imported a full block */
		      release_topo_savepoint (topo->db_handle, topo->cache);
		      *last = last_rowid;
		      return 1;
		  }
		if (sqlite3_column_type (stmt, igeo) == SQLITE_NULL)
		  {
		      last_rowid = rowid;
		      continue;
		  }
		if (sqlite3_column_type (stmt, igeo) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt, igeo);
		      int blob_sz = sqlite3_column_bytes (stmt, igeo);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz, gpkg_mode,
						       gpkg_amphibious);
		      if (geom != NULL)
			{
			    gaiatopo_reset_last_error_msg (accessor);
			    if (!auxtopo_insert_into_topology
				(accessor, geom, tolerance, line_max_points,
				 max_length))
			      {
				  char *msg;
				  const char *lw_msg = gaiaGetLwGeomErrorMsg ();
				  if (lw_msg == NULL)
				      msg =
					  sqlite3_mprintf
					  ("TopoGeo_FromGeoTableExt exception: UNKNOWN reason");
				  else
				      msg = sqlite3_mprintf ("%s", lw_msg);
				  gaiaFreeGeomColl (geom);
				  rollback_topo_savepoint (topo->db_handle,
							   topo->cache);
				  if (!insert_into_dustbin
				      (topo->db_handle, topo->cache, stmt,
				       stmt_dustbin, msg, tolerance,
				       dustbin_count))
				      goto error;
				  last_rowid = rowid;
				  *invalid = rowid;
				  return 0;
			      }
			    gaiaFreeGeomColl (geom);
			    last_rowid = rowid;
			}
		      else
			{
			    rollback_topo_savepoint (topo->db_handle,
						     topo->cache);
			    if (!insert_into_dustbin
				(topo->db_handle, topo->cache, stmt,
				 stmt_dustbin,
				 "TopoGeo_FromGeoTableExt error: Invalid Geometry",
				 tolerance, dustbin_count))
				goto error;
			}
		      last_rowid = rowid;
		  }
		else
		  {
		      rollback_topo_savepoint (topo->db_handle, topo->cache);
		      if (!insert_into_dustbin
			  (topo->db_handle, topo->cache, stmt, stmt_dustbin,
			   "TopoGeo_FromGeoTableExt error: not a BLOB value",
			   tolerance, dustbin_count))
			  goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_FromGeoTableExt error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
/* eof */
    release_topo_savepoint (topo->db_handle, topo->cache);
    return 2;

  error:
    rollback_topo_savepoint (topo->db_handle, topo->cache);
    return -1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_FromGeoTableExtended (GaiaTopologyAccessorPtr accessor,
				  const char *sql_in, const char *sql_out,
				  double tolerance, int line_max_points,
				  double max_length)
{
/* attempting to import a whole GeoTable into a Topology-Geometry - Extended mode */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_dustbin = NULL;
    int ret;
    int dustbin_count = 0;
    sqlite3_int64 start = -1;
    sqlite3_int64 last;
    sqlite3_int64 invalid = -1;

    if (topo == NULL)
	return 0;
    if (sql_in == NULL)
	return 0;
    if (sql_out == NULL)
	return 0;

/* building the SQL statement */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql_in, strlen (sql_in), &stmt,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_FromGeoTableExt error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the SQL dustbin statement */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql_out, strlen (sql_out),
			    &stmt_dustbin, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_FromGeoTableExt error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (1)
      {
	  /* main loop: attempting to import a block of features */
	  ret =
	      do_FromGeoTableExtended_block (accessor, stmt, stmt_dustbin,
					     tolerance, line_max_points,
					     max_length, start, &last, &invalid,
					     &dustbin_count);
	  if (ret < 0)		/* some unexpected error occurred */
	      goto error;
	  if (ret > 1)
	    {
		/* eof */
		break;
	    }
	  if (ret == 0)
	    {
		/* found a failing feature; recovering */
		ret =
		    do_FromGeoTableExtended_block (accessor, stmt, stmt_dustbin,
						   tolerance, line_max_points,
						   max_length, start, &last,
						   &invalid, &dustbin_count);
		if (ret != 1)
		    goto error;
		start = invalid;
		invalid = -1;
		continue;
	    }
	  start = last;
	  invalid = -1;
      }

    sqlite3_finalize (stmt);
    sqlite3_finalize (stmt_dustbin);
    return dustbin_count;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_dustbin != NULL)
	sqlite3_finalize (stmt_dustbin);
    return -1;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaGetEdgeSeed (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge)
{
/* attempting to get a Point (seed) identifying a Topology Edge */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    gaiaGeomCollPtr point = NULL;
    if (topo == NULL)
	return NULL;

/* building the SQL statement */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE edge_id = ?",
			 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("GetEdgeSeed error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, edge);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		      if (geom != NULL)
			{
			    int iv;
			    double x;
			    double y;
			    double z = 0.0;
			    double m = 0.0;
			    gaiaLinestringPtr ln = geom->FirstLinestring;
			    if (ln == NULL)
			      {
				  char *msg =
				      sqlite3_mprintf
				      ("TopoGeo_GetEdgeSeed error: Invalid Geometry");
				  gaiatopo_set_last_error_msg (accessor, msg);
				  sqlite3_free (msg);
				  gaiaFreeGeomColl (geom);
				  goto error;
			      }
			    if (ln->Points == 2)
			      {
				  /*
				     // special case: if the Edge has only 2 points then
				     // the Seed will always be placed on the first point
				   */
				  iv = 0;
			      }
			    else
			      {
				  /* ordinary case: placing the Seed into the middle */
				  iv = ln->Points / 2;
			      }
			    if (ln->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
			      }
			    else if (ln->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
			      }
			    else if (ln->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z,
						    &m);
			      }
			    else
			      {
				  gaiaGetPoint (ln->Coords, iv, &x, &y);
			      }
			    gaiaFreeGeomColl (geom);
			    if (topo->has_z)
			      {
				  point = gaiaAllocGeomCollXYZ ();
				  gaiaAddPointToGeomCollXYZ (point, x, y, z);
			      }
			    else
			      {
				  point = gaiaAllocGeomColl ();
				  gaiaAddPointToGeomColl (point, x, y);
			      }
			    point->Srid = topo->srid;
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("TopoGeo_GetEdgeSeed error: Invalid Geometry");
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_GetEdgeSeed error: not a BLOB value");
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_GetEdgeSeed error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    return point;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaGetFaceSeed (GaiaTopologyAccessorPtr accessor, sqlite3_int64 face)
{
/* attempting to get a Point (seed) identifying a Topology Face */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    gaiaGeomCollPtr point = NULL;
    if (topo == NULL)
	return NULL;

/* building the SQL statement */
    sql =
	sqlite3_mprintf ("SELECT ST_PointOnSurface(ST_GetFaceGeometry(%Q, ?))",
			 topo->topology_name);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("GetFaceSeed error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, face);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      point = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		      if (point == NULL)
			{
			    char *msg =
				sqlite3_mprintf
				("TopoGeo_GetFaceSeed error: Invalid Geometry");
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_GetFaceSeed error: not a BLOB value");
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_GetFaceSeed error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    return point;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

static int
delete_all_seeds (struct gaia_topology *topo)
{
/* deleting all existing Seeds */
    char *table;
    char *xtable;
    char *sql;
    char *errMsg;
    int ret;

    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

static int
update_outdated_edge_seeds (struct gaia_topology *topo)
{
/* updating all outdated Edge Seeds */
    char *table;
    char *xseeds;
    char *xedges;
    char *sql;
    int ret;
    sqlite3_stmt *stmt_out;
    sqlite3_stmt *stmt_in;

/* preparing the UPDATE statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET geom = "
			   "TopoGeo_GetEdgeSeed(%Q, edge_id) WHERE edge_id = ?",
			   xseeds, topo->topology_name);
    free (xseeds);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT s.edge_id FROM MAIN.\"%s\" AS s "
			   "JOIN MAIN.\"%s\" AS e ON (e.edge_id = s.edge_id) "
			   "WHERE s.edge_id IS NOT NULL AND e.timestamp > s.timestamp",
			   xseeds, xedges);
    free (xseeds);
    free (xedges);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
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
		sqlite3_bind_int64 (stmt_out, 1,
				    sqlite3_column_int64 (stmt_in, 0));
		/* updating the Seeds table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_UpdateSeeds() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
update_outdated_face_seeds (struct gaia_topology *topo)
{
/* updating all outdated Face Seeds */
    char *table;
    char *xseeds;
    char *xedges;
    char *xfaces;
    char *sql;
    int ret;
    sqlite3_stmt *stmt_out;
    sqlite3_stmt *stmt_in;

/* preparing the UPDATE statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET geom = "
			   "TopoGeo_GetFaceSeed(%Q, face_id) WHERE face_id = ?",
			   xseeds, topo->topology_name);
    free (xseeds);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT x.face_id FROM MAIN.\"%s\" AS s, "
			   "(SELECT f.face_id AS face_id, Max(e.timestamp) AS max_tm "
			   "FROM MAIN.\"%s\" AS f "
			   "JOIN MAIN.\"%s\" AS e ON (e.left_face = f.face_id OR e.right_face = f.face_id) "
			   "GROUP BY f.face_id) AS x "
			   "WHERE s.face_id IS NOT NULL AND s.face_id = x.face_id AND x.max_tm > s.timestamp",
			   xseeds, xfaces, xedges);
    free (xseeds);
    free (xedges);
    free (xfaces);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
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
		sqlite3_bind_int64 (stmt_out, 1,
				    sqlite3_column_int64 (stmt_in, 0));
		/* updating the Seeds table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_UpdateSeeds() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeoUpdateSeeds (GaiaTopologyAccessorPtr accessor, int incremental_mode)
{
/* updating all TopoGeo Seeds */
    char *table;
    char *xseeds;
    char *xedges;
    char *xfaces;
    char *sql;
    char *errMsg;
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (!incremental_mode)
      {
	  /* deleting all existing Seeds */
	  if (!delete_all_seeds (topo))
	      return 0;
      }

/* paranoid precaution: deleting all orphan Edge Seeds */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE edge_id IN ("
			   "SELECT s.edge_id FROM MAIN.\"%s\" AS s "
			   "LEFT JOIN MAIN.\"%s\" AS e ON (s.edge_id = e.edge_id) "
			   "WHERE s.edge_id IS NOT NULL AND e.edge_id IS NULL)",
			   xseeds, xseeds, xedges);
    free (xseeds);
    free (xedges);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* paranoid precaution: deleting all orphan Face Seeds */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE face_id IN ("
			   "SELECT s.face_id FROM MAIN.\"%s\" AS s "
			   "LEFT JOIN MAIN.\"%s\" AS f ON (s.face_id = f.face_id) "
			   "WHERE s.face_id IS NOT NULL AND f.face_id IS NULL)",
			   xseeds, xseeds, xfaces);
    free (xseeds);
    free (xfaces);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* updating all outdated Edge Seeds */
    if (!update_outdated_edge_seeds (topo))
	return 0;

/* updating all outdated Facee Seeds */
    if (!update_outdated_face_seeds (topo))
	return 0;

/* inserting all missing Edge Seeds */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (seed_id, edge_id, face_id, geom) "
	 "SELECT NULL, e.edge_id, NULL, TopoGeo_GetEdgeSeed(%Q, e.edge_id) "
	 "FROM MAIN.\"%s\" AS e "
	 "LEFT JOIN MAIN.\"%s\" AS s ON (e.edge_id = s.edge_id) WHERE s.edge_id IS NULL",
	 xseeds, topo->topology_name, xedges, xseeds);
    free (xseeds);
    free (xedges);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    /* inserting all missing Face Seeds */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (seed_id, edge_id, face_id, geom) "
	 "SELECT NULL, NULL, f.face_id, TopoGeo_GetFaceSeed(%Q, f.face_id) "
	 "FROM MAIN.\"%s\" AS f "
	 "LEFT JOIN MAIN.\"%s\" AS s ON (f.face_id = s.face_id) "
	 "WHERE s.face_id IS NULL AND f.face_id <> 0", xseeds,
	 topo->topology_name, xfaces, xseeds);
    free (xseeds);
    free (xfaces);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaTopoGeoSnapPointToSeed (GaiaTopologyAccessorPtr accessor,
			    gaiaGeomCollPtr pt, double distance)
{
/* snapping a Point to TopoSeeds */
    char *sql;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_snap = NULL;
    int ret;
    unsigned char *blob;
    int blob_size;
    unsigned char *blob2;
    int blob_size2;
    gaiaGeomCollPtr result = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return NULL;

/* preparing the Snap statement */
    sql = "SELECT ST_Snap(?, ?, ?)";
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_snap,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnapPointToSeed() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT geom "
			   "FROM \"%s\" WHERE ST_Distance(?, geom) <= ? AND rowid IN "
			   "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ST_Buffer(?, ?))",
			   xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnapPointToSeed() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* querying Seeds */
    if (topo->has_z)
	result = gaiaAllocGeomCollXYZ ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = pt->Srid;
    gaiaToSpatiaLiteBlobWkb (pt, &blob, &blob_size);
    gaiaToSpatiaLiteBlobWkb (pt, &blob2, &blob_size2);
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, free);
    sqlite3_bind_double (stmt, 2, distance);
    sqlite3_bind_blob (stmt, 3, blob2, blob_size2, free);
    sqlite3_bind_double (stmt, 4, distance * 1.2);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *p_blob = sqlite3_column_blob (stmt, 0);
		int blobsz = sqlite3_column_bytes (stmt, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (p_blob, blobsz);
		if (geom != NULL)
		  {
		      gaiaPointPtr pt = geom->FirstPoint;
		      while (pt != NULL)
			{
			    /* copying all Points into the result Geometry */
			    if (topo->has_z)
				gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y,
							   pt->Z);
			    else
				gaiaAddPointToGeomColl (result, pt->X, pt->Y);
			    pt = pt->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_SnapPointToSeed error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    stmt = NULL;
    if (result->FirstPoint == NULL)
	goto error;

/* Snap */
    gaiaToSpatiaLiteBlobWkb (pt, &blob, &blob_size);
    gaiaToSpatiaLiteBlobWkb (result, &blob2, &blob_size2);
    gaiaFreeGeomColl (result);
    result = NULL;
    sqlite3_reset (stmt_snap);
    sqlite3_clear_bindings (stmt_snap);
    sqlite3_bind_blob (stmt_snap, 1, blob, blob_size, free);
    sqlite3_bind_blob (stmt_snap, 2, blob2, blob_size2, free);
    sqlite3_bind_double (stmt_snap, 3, distance);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_snap);
	  
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_snap, 0) != SQLITE_NULL)
		  {
		      const unsigned char *p_blob =
			  sqlite3_column_blob (stmt_snap, 0);
		      int blobsz = sqlite3_column_bytes (stmt_snap, 0);
		      if (result != NULL)
			  gaiaFreeGeomColl (result);
		      result = gaiaFromSpatiaLiteBlobWkb (p_blob, blobsz);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_SnapPointToSeed error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
	sqlite3_finalize (stmt_snap);
	stmt_snap = NULL;
    if (result == NULL)
	goto error;
    if (result->FirstLinestring != NULL || result->FirstPolygon != NULL)
	goto error;
    if (result->FirstPoint == NULL)
	goto error;
    if (result->FirstPoint != result->LastPoint)
	goto error;
    return result;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_snap != NULL)
	sqlite3_finalize (stmt_snap);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    return NULL;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaTopoGeoSnapLinestringToSeed (GaiaTopologyAccessorPtr accessor,
				 gaiaGeomCollPtr ln, double distance)
{
/* snapping a Linestring to TopoSeeds */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    unsigned char *blob;
    int blob_size;
    unsigned char *blob2;
    int blob_size2;
    gaiaGeomCollPtr result = NULL;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_snap = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return NULL;

/* preparing the Snap statement */
    sql = "SELECT ST_Snap(?, ?, ?)";
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_snap,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnapLinestringToSeed() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT edge_id, geom "
			   "FROM \"%s\" WHERE ST_Distance(?, geom) <= ? AND rowid IN "
			   "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ST_Buffer(?, ?))",
			   xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnapLinestringToSeed() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* querying Seeds */
    if (topo->has_z)
	result = gaiaAllocGeomCollXYZ ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = ln->Srid;
    gaiaToSpatiaLiteBlobWkb (ln, &blob, &blob_size);
    gaiaToSpatiaLiteBlobWkb (ln, &blob2, &blob_size2);
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, free);
    sqlite3_bind_double (stmt, 2, distance);
    sqlite3_bind_blob (stmt, 3, blob2, blob_size2, free);
    sqlite3_bind_double (stmt, 4, distance * 1.2);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) != SQLITE_NULL)
		  {
		      const unsigned char *p_blob =
			  sqlite3_column_blob (stmt, 1);
		      int blobsz = sqlite3_column_bytes (stmt, 1);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkb (p_blob, blobsz);
		      if (geom != NULL)
			{
			    gaiaPointPtr pt = geom->FirstPoint;
			    while (pt != NULL)
			      {
				  /* copying all Points into the result Geometry */
				  if (topo->has_z)
				      gaiaAddPointToGeomCollXYZ (result, pt->X,
								 pt->Y, pt->Z);
				  else
				      gaiaAddPointToGeomColl (result, pt->X,
							      pt->Y);
				  pt = pt->Next;
			      }
			}
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_SnapLinestringToSeed error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    stmt = NULL;
    if (result->FirstPoint == NULL)
	goto error;

/* Snap */
    gaiaToSpatiaLiteBlobWkb (ln, &blob, &blob_size);
    gaiaToSpatiaLiteBlobWkb (result, &blob2, &blob_size2);
    gaiaFreeGeomColl (result);
    result = NULL;
    sqlite3_reset (stmt_snap);
    sqlite3_clear_bindings (stmt_snap);
    sqlite3_bind_blob (stmt_snap, 1, blob, blob_size, free);
    sqlite3_bind_blob (stmt_snap, 2, blob2, blob_size2, free);
    sqlite3_bind_double (stmt_snap, 3, distance);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_snap);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_snap, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *p_blob =
			  sqlite3_column_blob (stmt_snap, 0);
		      int blobsz = sqlite3_column_bytes (stmt_snap, 0);
		      if (result != NULL)
			  gaiaFreeGeomColl (result);
		      result = gaiaFromSpatiaLiteBlobWkb (p_blob, blobsz);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_SnapLinestringToSeed error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
	sqlite3_finalize (stmt_snap);
	stmt_snap = NULL;
    if (result == NULL)
	goto error;
    if (result->FirstPoint != NULL || result->FirstPolygon != NULL)
	goto error;
    if (result->FirstLinestring == NULL)
	goto error;
    if (result->FirstLinestring != result->LastLinestring)
	goto error;
    return result;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_snap != NULL)
	sqlite3_finalize (stmt_snap);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    return NULL;
}

static gaiaGeomCollPtr
make_geom_from_polyg (int srid, gaiaPolygonPtr pg)
{
/* quick constructor: Geometry based on external polyg */
    gaiaGeomCollPtr reference;
    if (pg->DimensionModel == GAIA_XY_Z_M)
	reference = gaiaAllocGeomCollXYZM ();
    else if (pg->DimensionModel == GAIA_XY_Z)
	reference = gaiaAllocGeomCollXYZ ();
    else if (pg->DimensionModel == GAIA_XY_M)
	reference = gaiaAllocGeomCollXYM ();
    else
	reference = gaiaAllocGeomColl ();
    reference->Srid = srid;
    pg->Next = NULL;
    reference->FirstPolygon = pg;
    reference->LastPolygon = pg;
    return reference;
}

static void
do_eval_topogeo_point (struct gaia_topology *topo, gaiaGeomCollPtr result,
		       gaiaGeomCollPtr reference, sqlite3_stmt * stmt_node)
{
/* retrieving Points from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;

/* initializing the Topo-Node query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_node);
    sqlite3_clear_bindings (stmt_node);
    sqlite3_bind_blob (stmt_node, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_node, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_node);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_node, 0);
		int blob_sz = sqlite3_column_bytes (stmt_node, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaPointPtr pt = geom->FirstPoint;
		      while (pt != NULL)
			{
			    /* copying all Points into the result Geometry */
			    if (topo->has_z)
				gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y,
							   pt->Z);
			    else
				gaiaAddPointToGeomColl (result, pt->X, pt->Y);
			    pt = pt->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_collect_topo_edges (struct gaia_topology *topo, gaiaGeomCollPtr sparse,
		       sqlite3_stmt * stmt_edge, sqlite3_int64 edge_id)
{
/* collecting Edge Geometries one by one */
    int ret;

/* initializing the Topo-Edge query */
    sqlite3_reset (stmt_edge);
    sqlite3_clear_bindings (stmt_edge);
    sqlite3_bind_int64 (stmt_edge, 1, edge_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_edge, 0);
		int blob_sz = sqlite3_column_bytes (stmt_edge, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaLinestringPtr ln = geom->FirstLinestring;
		      while (ln != NULL)
			{
			    if (topo->has_z)
				auxtopo_copy_linestring3d (ln, sparse);
			    else
				auxtopo_copy_linestring (ln, sparse);
			    ln = ln->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_eval_topogeo_line (struct gaia_topology *topo, gaiaGeomCollPtr result,
		      gaiaGeomCollPtr reference, sqlite3_stmt * stmt_seed_edge,
		      sqlite3_stmt * stmt_edge)
{
/* retrieving Linestrings from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr sparse;
    gaiaGeomCollPtr rearranged;
    gaiaLinestringPtr ln;

    if (topo->has_z)
	sparse = gaiaAllocGeomCollXYZ ();
    else
	sparse = gaiaAllocGeomColl ();
    sparse->Srid = topo->srid;

/* initializing the Topo-Seed-Edge query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_seed_edge);
    sqlite3_clear_bindings (stmt_seed_edge);
    sqlite3_bind_blob (stmt_seed_edge, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_seed_edge, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_seed_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id =
		    sqlite3_column_int64 (stmt_seed_edge, 0);
		do_collect_topo_edges (topo, sparse, stmt_edge, edge_id);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		gaiaFreeGeomColl (sparse);
		return;
	    }
      }

/* attempting to rearrange sparse lines */
    rearranged = gaiaLineMerge_r (topo->cache, sparse);
    gaiaFreeGeomColl (sparse);
    if (rearranged == NULL)
	return;
    ln = rearranged->FirstLinestring;
    while (ln != NULL)
      {
	  if (topo->has_z)
	      auxtopo_copy_linestring3d (ln, result);
	  else
	      auxtopo_copy_linestring (ln, result);
	  ln = ln->Next;
      }
    gaiaFreeGeomColl (rearranged);
}

static void
do_explode_topo_face (struct gaia_topology *topo, struct face_edges *list,
		      sqlite3_stmt * stmt_face, sqlite3_int64 face_id)
{
/* exploding all Edges required by the same face */
    int ret;

/* initializing the Topo-Face query */
    sqlite3_reset (stmt_face);
    sqlite3_clear_bindings (stmt_face);
    sqlite3_bind_int64 (stmt_face, 1, face_id);
    sqlite3_bind_int64 (stmt_face, 2, face_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_face);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_face, 0);
		sqlite3_int64 left_face = sqlite3_column_int64 (stmt_face, 1);
		sqlite3_int64 right_face = sqlite3_column_int64 (stmt_face, 2);
		const unsigned char *blob = sqlite3_column_blob (stmt_face, 3);
		int blob_sz = sqlite3_column_bytes (stmt_face, 3);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		    auxtopo_add_face_edge (list, face_id, edge_id, left_face,
					   right_face, geom);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_copy_ring3d (gaiaRingPtr in, gaiaRingPtr out)
{
/* inserting/copying a Ring 3D into another Ring */
    int iv;
    double x;
    double y;
    double z;
    for (iv = 0; iv < in->Points; iv++)
      {
	  gaiaGetPointXYZ (in->Coords, iv, &x, &y, &z);
	  gaiaSetPointXYZ (out->Coords, iv, x, y, z);
      }
}

static void
do_copy_ring (gaiaRingPtr in, gaiaRingPtr out)
{
/* inserting/copying a Ring into another Ring */
    int iv;
    double x;
    double y;
    for (iv = 0; iv < in->Points; iv++)
      {
	  gaiaGetPoint (in->Coords, iv, &x, &y);
	  gaiaSetPoint (out->Coords, iv, x, y);
      }
}

static void
do_copy_polygon3d (gaiaPolygonPtr in, gaiaGeomCollPtr geom)
{
/* inserting/copying a Polygon 3D into another Geometry */
    int ib;
    gaiaRingPtr rng_out;
    gaiaRingPtr rng_in = in->Exterior;
    gaiaPolygonPtr out =
	gaiaAddPolygonToGeomColl (geom, rng_in->Points, in->NumInteriors);
    rng_out = out->Exterior;
    do_copy_ring3d (rng_in, rng_out);
    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  rng_in = in->Interiors + ib;
	  rng_out = gaiaAddInteriorRing (out, ib, rng_in->Points);
	  do_copy_ring3d (rng_in, rng_out);
      }
}

static void
do_copy_polygon (gaiaPolygonPtr in, gaiaGeomCollPtr geom)
{
/* inserting/copying a Polygon into another Geometry */
    int ib;
    gaiaRingPtr rng_out;
    gaiaRingPtr rng_in = in->Exterior;
    gaiaPolygonPtr out =
	gaiaAddPolygonToGeomColl (geom, rng_in->Points, in->NumInteriors);
    rng_out = out->Exterior;
    do_copy_ring (rng_in, rng_out);
    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  rng_in = in->Interiors + ib;
	  rng_out = gaiaAddInteriorRing (out, ib, rng_in->Points);
	  do_copy_ring (rng_in, rng_out);
      }
}

static void
do_copy_filter_polygon3d (gaiaPolygonPtr in, gaiaGeomCollPtr geom,
			  const void *cache, double tolerance)
{
/* inserting/copying a Polygon 3D into another Geometry (with tolerance) */
    int ib;
    gaiaGeomCollPtr polyg;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng_out;
    gaiaRingPtr rng_in = in->Exterior;
    gaiaPolygonPtr out;
    double area;
    int ret;
    int numints = 0;

    polyg = gaiaAllocGeomCollXYZ ();
    pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
    rng_out = pg->Exterior;
    do_copy_ring3d (rng_in, rng_out);
    ret = gaiaGeomCollArea_r (cache, polyg, &area);
    gaiaFreeGeomColl (polyg);
    if (!ret)
	return;
    if ((tolerance * tolerance) > area)
	return;			/* skipping smaller polygons */

    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  /* counting how many interior rings do we really have */
	  rng_in = in->Interiors + ib;
	  polyg = gaiaAllocGeomCollXYZ ();
	  pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
	  rng_out = pg->Exterior;
	  do_copy_ring3d (rng_in, rng_out);
	  ret = gaiaGeomCollArea_r (cache, polyg, &area);
	  gaiaFreeGeomColl (polyg);
	  if (!ret)
	      continue;
	  if ((tolerance * tolerance) > area)
	      continue;		/* skipping smaller holes */
	  numints++;
      }

    rng_in = in->Exterior;
    out = gaiaAddPolygonToGeomColl (geom, rng_in->Points, numints);
    rng_out = out->Exterior;
    do_copy_ring3d (rng_in, rng_out);
    numints = 0;
    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  /* copying interior rings */
	  rng_in = in->Interiors + ib;
	  polyg = gaiaAllocGeomCollXYZ ();
	  pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
	  rng_out = pg->Exterior;
	  do_copy_ring3d (rng_in, rng_out);
	  ret = gaiaGeomCollArea_r (cache, polyg, &area);
	  gaiaFreeGeomColl (polyg);
	  if (!ret)
	      continue;
	  if ((tolerance * tolerance) > area)
	      continue;		/* skipping smaller holes */
	  rng_out = gaiaAddInteriorRing (out, numints++, rng_in->Points);
	  do_copy_ring3d (rng_in, rng_out);
      }
}

static void
do_copy_filter_polygon (gaiaPolygonPtr in, gaiaGeomCollPtr geom,
			const void *cache, double tolerance)
{
/* inserting/copying a Polygon into another Geometry (with tolerance) */
    int ib;
    gaiaGeomCollPtr polyg;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng_out;
    gaiaRingPtr rng_in = in->Exterior;
    gaiaPolygonPtr out;
    double area;
    int ret;
    int numints = 0;

    polyg = gaiaAllocGeomColl ();
    pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
    rng_out = pg->Exterior;
    do_copy_ring (rng_in, rng_out);
    ret = gaiaGeomCollArea_r (cache, polyg, &area);
    gaiaFreeGeomColl (polyg);
    if (!ret)
	return;
    if ((tolerance * tolerance) > area)
	return;			/* skipping smaller polygons */

    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  /* counting how many interior rings do we really have */
	  rng_in = in->Interiors + ib;
	  polyg = gaiaAllocGeomColl ();
	  pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
	  rng_out = pg->Exterior;
	  do_copy_ring (rng_in, rng_out);
	  ret = gaiaGeomCollArea_r (cache, polyg, &area);
	  gaiaFreeGeomColl (polyg);
	  if (!ret)
	      continue;
	  if ((tolerance * tolerance) > area)
	      continue;		/* skipping smaller holes */
	  numints++;
      }

    rng_in = in->Exterior;
    out = gaiaAddPolygonToGeomColl (geom, rng_in->Points, numints);
    rng_out = out->Exterior;
    do_copy_ring (rng_in, rng_out);
    numints = 0;
    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  /* copying interior rings */
	  rng_in = in->Interiors + ib;
	  polyg = gaiaAllocGeomColl ();
	  pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
	  rng_out = pg->Exterior;
	  do_copy_ring (rng_in, rng_out);
	  ret = gaiaGeomCollArea_r (cache, polyg, &area);
	  gaiaFreeGeomColl (polyg);
	  if (!ret)
	      continue;
	  if ((tolerance * tolerance) > area)
	      continue;		/* skipping smaller holes */
	  rng_out = gaiaAddInteriorRing (out, numints++, rng_in->Points);
	  do_copy_ring (rng_in, rng_out);
      }
}

static void
do_eval_topo_polyg (struct gaia_topology *topo, gaiaGeomCollPtr result,
		    gaiaGeomCollPtr reference, sqlite3_stmt * stmt_seed_face,
		    sqlite3_stmt * stmt_face)
{
/* retrieving Polygons from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr rearranged;
    gaiaPolygonPtr pg;
    struct face_edges *list =
	auxtopo_create_face_edges (topo->has_z, topo->srid);

/* initializing the Topo-Seed-Face query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_seed_face);
    sqlite3_clear_bindings (stmt_seed_face);
    sqlite3_bind_blob (stmt_seed_face, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_seed_face, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_seed_face);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 face_id =
		    sqlite3_column_int64 (stmt_seed_face, 0);
		do_explode_topo_face (topo, list, stmt_face, face_id);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		auxtopo_free_face_edges (list);
		return;
	    }
      }

/* attempting to rearrange sparse lines into Polygons */
    auxtopo_select_valid_face_edges (list);
    rearranged = auxtopo_polygonize_face_edges (list, topo->cache);
    auxtopo_free_face_edges (list);
    if (rearranged == NULL)
	return;
    pg = rearranged->FirstPolygon;
    while (pg != NULL)
      {
	  if (topo->has_z)
	      do_copy_polygon3d (pg, result);
	  else
	      do_copy_polygon (pg, result);
	  pg = pg->Next;
      }
    gaiaFreeGeomColl (rearranged);
}

static void
do_eval_topo_polyg_generalize (struct gaia_topology *topo,
			       gaiaGeomCollPtr result,
			       gaiaGeomCollPtr reference,
			       sqlite3_stmt * stmt_seed_face,
			       sqlite3_stmt * stmt_face, double tolerance)
{
/* retrieving Polygons from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr rearranged;
    gaiaPolygonPtr pg;
    struct face_edges *list =
	auxtopo_create_face_edges (topo->has_z, topo->srid);

/* initializing the Topo-Seed-Face query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_seed_face);
    sqlite3_clear_bindings (stmt_seed_face);
    sqlite3_bind_blob (stmt_seed_face, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_seed_face, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_seed_face);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 face_id =
		    sqlite3_column_int64 (stmt_seed_face, 0);
		do_explode_topo_face (topo, list, stmt_face, face_id);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		auxtopo_free_face_edges (list);
		return;
	    }
      }

/* attempting to rearrange sparse lines into Polygons */
    auxtopo_select_valid_face_edges (list);
    rearranged = auxtopo_polygonize_face_edges_generalize (list, topo->cache);
    auxtopo_free_face_edges (list);
    if (rearranged == NULL)
	return;
    pg = rearranged->FirstPolygon;
    while (pg != NULL)
      {
	  if (topo->has_z)
	      do_copy_filter_polygon3d (pg, result, topo->cache, tolerance);
	  else
	      do_copy_filter_polygon (pg, result, topo->cache, tolerance);
	  pg = pg->Next;
      }
    gaiaFreeGeomColl (rearranged);
}

static gaiaGeomCollPtr
do_eval_topogeo_geom (struct gaia_topology *topo, gaiaGeomCollPtr geom,
		      sqlite3_stmt * stmt_seed_edge,
		      sqlite3_stmt * stmt_seed_face, sqlite3_stmt * stmt_node,
		      sqlite3_stmt * stmt_edge, sqlite3_stmt * stmt_face,
		      int out_type, double tolerance)
{
/* retrieving Topology-Geometry geometries via matching Seeds */
    gaiaGeomCollPtr result;

    if (topo->has_z)
	result = gaiaAllocGeomCollXYZ ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = topo->srid;
    result->DeclaredType = out_type;

    if (out_type == GAIA_POINT || out_type == GAIA_MULTIPOINT
	|| out_type == GAIA_GEOMETRYCOLLECTION || out_type == GAIA_UNKNOWN)
      {
	  /* processing all Points */
	  gaiaPointPtr pt = geom->FirstPoint;
	  while (pt != NULL)
	    {
		gaiaPointPtr next = pt->Next;
		gaiaGeomCollPtr reference = (gaiaGeomCollPtr)
		    auxtopo_make_geom_from_point (topo->srid, topo->has_z, pt);
		do_eval_topogeo_point (topo, result, reference, stmt_node);
		auxtopo_destroy_geom_from (reference);
		pt->Next = next;
		pt = pt->Next;
	    }
      }

    if (out_type == GAIA_MULTILINESTRING || out_type == GAIA_GEOMETRYCOLLECTION
	|| out_type == GAIA_UNKNOWN)
      {
	  /* processing all Linestrings */
	  gaiaLinestringPtr ln = geom->FirstLinestring;
	  while (ln != NULL)
	    {
		gaiaLinestringPtr next = ln->Next;
		gaiaGeomCollPtr reference = (gaiaGeomCollPtr)
		    auxtopo_make_geom_from_line (topo->srid, ln);
		do_eval_topogeo_line (topo, result, reference, stmt_seed_edge,
				      stmt_edge);
		auxtopo_destroy_geom_from (reference);
		ln->Next = next;
		ln = ln->Next;
	    }
      }

    if (out_type == GAIA_MULTIPOLYGON || out_type == GAIA_GEOMETRYCOLLECTION
	|| out_type == GAIA_UNKNOWN)
      {
	  /* processing all Polygons */
	  gaiaPolygonPtr pg = geom->FirstPolygon;
	  while (pg != NULL)
	    {
		gaiaPolygonPtr next = pg->Next;
		gaiaGeomCollPtr reference =
		    make_geom_from_polyg (topo->srid, pg);
		if (tolerance > 0.0)
		    do_eval_topo_polyg_generalize (topo, result, reference,
						   stmt_seed_face, stmt_face,
						   tolerance);
		else
		    do_eval_topo_polyg (topo, result, reference, stmt_seed_face,
					stmt_face);
		auxtopo_destroy_geom_from (reference);
		pg->Next = next;
		pg = pg->Next;
	    }
      }

    if (result->FirstPoint == NULL && result->FirstLinestring == NULL
	&& result->FirstPolygon == NULL)
	goto error;
    return result;

  error:
    gaiaFreeGeomColl (result);
    return NULL;
}

static int
do_eval_topogeo_seeds (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
		       int ref_geom_col, sqlite3_stmt * stmt_ins,
		       sqlite3_stmt * stmt_seed_edge,
		       sqlite3_stmt * stmt_seed_face, sqlite3_stmt * stmt_node,
		       sqlite3_stmt * stmt_edge, sqlite3_stmt * stmt_face,
		       int out_type, double tolerance)
{
/* querying the ref-table */
    int ret;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      if (icol == ref_geom_col)
			{
			    /* the geometry column */
			    const unsigned char *blob =
				sqlite3_column_blob (stmt_ref, icol);
			    int blob_sz = sqlite3_column_bytes (stmt_ref, icol);
			    gaiaGeomCollPtr geom =
				gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
			    if (geom != NULL)
			      {
				  gaiaGeomCollPtr result;
				  unsigned char *p_blob;
				  int n_bytes;
				  int gpkg_mode = 0;
				  if (topo->cache != NULL)
				    {
					struct splite_internal_cache *cache =
					    (struct splite_internal_cache
					     *) (topo->cache);
					gpkg_mode = cache->gpkg_mode;
				    }
				  result = do_eval_topogeo_geom (topo, geom,
								 stmt_seed_edge,
								 stmt_seed_face,
								 stmt_node,
								 stmt_edge,
								 stmt_face,
								 out_type,
								 tolerance);
				  gaiaFreeGeomColl (geom);
				  if (result != NULL)
				    {
					gaiaToSpatiaLiteBlobWkbEx (result,
								   &p_blob,
								   &n_bytes,
								   gpkg_mode);
					gaiaFreeGeomColl (result);
					sqlite3_bind_blob (stmt_ins, icol + 1,
							   p_blob, n_bytes,
							   free);
				    }
				  else
				      sqlite3_bind_null (stmt_ins, icol + 1);
			      }
			    else
				sqlite3_bind_null (stmt_ins, icol + 1);
			    continue;
			}
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
					   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_ToGeoTableGeneralize (GaiaTopologyAccessorPtr accessor,
				  const char *db_prefix, const char *ref_table,
				  const char *ref_column, const char *out_table,
				  double tolerance, int with_spatial_index)
{
/* 
/ attempting to create and populate a new GeoTable out from a Topology-Geometry 
/ (simplified/generalized form)
*/
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_seed_edge = NULL;
    sqlite3_stmt *stmt_seed_face = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    sqlite3_stmt *stmt_face = NULL;
    int ret;
    char *create;
    char *select;
    char *insert;
    char *sql;
    char *errMsg;
    char *xprefix;
    char *xtable;
    int ref_type;
    const char *type;
    int out_type;
    int ref_geom_col;
    if (topo == NULL)
	return 0;

/* incrementally updating all Topology Seeds */
    if (!gaiaTopoGeoUpdateSeeds (accessor, 1))
	return 0;

/* composing the CREATE TABLE output-table statement */
    if (!auxtopo_create_togeotable_sql
	(topo->db_handle, db_prefix, ref_table, ref_column, out_table, &create,
	 &select, &insert, &ref_geom_col))
	goto error;

/* creating the output-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* checking the Geometry Type */
    if (!auxtopo_retrieve_geometry_type
	(topo->db_handle, db_prefix, ref_table, ref_column, &ref_type))
	goto error;
    switch (ref_type)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
	  type = "POINT";
	  out_type = GAIA_POINT;
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  type = "MULTIPOINT";
	  out_type = GAIA_MULTIPOINT;
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  type = "MULTILINESTRING";
	  out_type = GAIA_MULTILINESTRING;
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  type = "MULTIPOLYGON";
	  out_type = GAIA_MULTIPOLYGON;
	  break;
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  type = "GEOMETRYCOLLECTION";
	  out_type = GAIA_GEOMETRYCOLLECTION;
	  break;
      default:
	  type = "GEOMETRY";
	  out_type = GAIA_UNKNOWN;
	  break;
      };

/* creating the output Geometry Column */
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(Lower(%Q), Lower(%Q), %d, '%s', '%s')",
	 out_table, ref_column, topo->srid, type,
	 (topo->has_z == 0) ? "XY" : "XYZ");
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (with_spatial_index)
      {
	  /* adding a Spatial Index supporting the Geometry Column */
	  sql =
	      sqlite3_mprintf
	      ("SELECT CreateSpatialIndex(Lower(%Q), Lower(%Q))",
	       out_table, ref_column);
	  ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				     errMsg);
		sqlite3_free (errMsg);
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

/* preparing the "SELECT * FROM ref-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO out-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Edges query */
    xprefix = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT edge_id FROM MAIN.\"%s\" "
			   "WHERE edge_id IS NOT NULL AND ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_seed_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Faces query */
    xprefix = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT face_id FROM MAIN.\"%s\" "
			   "WHERE face_id IS NOT NULL AND ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_seed_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" "
			   "WHERE ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Edges query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    if (tolerance > 0.0)
	sql =
	    sqlite3_mprintf
	    ("SELECT ST_SimplifyPreserveTopology(geom, %1.6f) FROM MAIN.\"%s\" WHERE edge_id = ?",
	     tolerance, xtable, xprefix);
    else
	sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE edge_id = ?",
			       xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Faces query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    if (tolerance > 0.0)
	sql =
	    sqlite3_mprintf
	    ("SELECT edge_id, left_face, right_face, ST_SimplifyPreserveTopology(geom, %1.6f) FROM MAIN.\"%s\" "
	     "WHERE left_face = ? OR right_face = ?", tolerance, xtable);
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT edge_id, left_face, right_face, geom FROM MAIN.\"%s\" "
	     "WHERE left_face = ? OR right_face = ?", xtable);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating feature/topology matching via coincident topo-seeds */
    if (!do_eval_topogeo_seeds
	(topo, stmt_ref, ref_geom_col, stmt_ins, stmt_seed_edge, stmt_seed_face,
	 stmt_node, stmt_edge, stmt_face, out_type, tolerance))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_seed_edge);
    sqlite3_finalize (stmt_seed_face);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_face);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_seed_edge != NULL)
	sqlite3_finalize (stmt_seed_edge);
    if (stmt_seed_face != NULL)
	sqlite3_finalize (stmt_seed_face);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_ToGeoTable (GaiaTopologyAccessorPtr accessor,
			const char *db_prefix, const char *ref_table,
			const char *ref_column, const char *out_table,
			int with_spatial_index)
{
/* attempting to create and populate a new GeoTable out from a Topology-Geometry */
    return gaiaTopoGeo_ToGeoTableGeneralize (accessor, db_prefix, ref_table,
					     ref_column, out_table, -1.0,
					     with_spatial_index);
}

static struct face_item *
create_face_item (sqlite3_int64 face_id)
{
/* creating a Face Item */
    struct face_item *item = malloc (sizeof (struct face_item));
    item->face_id = face_id;
    item->next = NULL;
    return item;
}

static void
destroy_face_item (struct face_item *item)
{
/* destroying a Face Item */
    if (item == NULL)
	return;
    free (item);
}

static struct face_edge_item *
create_face_edge_item (sqlite3_int64 edge_id, sqlite3_int64 left_face,
		       sqlite3_int64 right_face, gaiaGeomCollPtr geom)
{
/* creating a Face-Edge Item */
    struct face_edge_item *item = malloc (sizeof (struct face_edge_item));
    item->edge_id = edge_id;
    item->left_face = left_face;
    item->right_face = right_face;
    item->geom = geom;
    item->count = 0;
    item->next = NULL;
    return item;
}

static void
destroy_face_edge_item (struct face_edge_item *item)
{
/* destroying a Face-Edge Item */
    if (item == NULL)
	return;
    if (item->geom != NULL)
	gaiaFreeGeomColl (item->geom);
    free (item);
}

TOPOLOGY_PRIVATE struct face_edges *
auxtopo_create_face_edges (int has_z, int srid)
{
/* creating an empty Face-Edges list */
    struct face_edges *list = malloc (sizeof (struct face_edges));
    list->has_z = has_z;
    list->srid = srid;
    list->first_edge = NULL;
    list->last_edge = NULL;
    list->first_face = NULL;
    list->last_face = NULL;
    return list;
}

TOPOLOGY_PRIVATE void
auxtopo_free_face_edges (struct face_edges *list)
{
/* destroying a Face-Edges list */
    struct face_edge_item *fe;
    struct face_edge_item *fen;
    struct face_item *f;
    struct face_item *fn;
    if (list == NULL)
	return;

    fe = list->first_edge;
    while (fe != NULL)
      {
	  /* destroying all Face-Edge items */
	  fen = fe->next;
	  destroy_face_edge_item (fe);
	  fe = fen;
      }
    f = list->first_face;
    while (f != NULL)
      {
	  /* destroying all Face items */
	  fn = f->next;
	  destroy_face_item (f);
	  f = fn;
      }
    free (list);
}

TOPOLOGY_PRIVATE void
auxtopo_add_face_edge (struct face_edges *list, sqlite3_int64 face_id,
		       sqlite3_int64 edge_id, sqlite3_int64 left_face,
		       sqlite3_int64 right_face, gaiaGeomCollPtr geom)
{
/* adding a Face-Edge Item into the list */
    struct face_item *f;
    struct face_edge_item *fe =
	create_face_edge_item (edge_id, left_face, right_face, geom);
    if (list->first_edge == NULL)
	list->first_edge = fe;
    if (list->last_edge != NULL)
	list->last_edge->next = fe;
    list->last_edge = fe;

    f = list->first_face;
    while (f != NULL)
      {
	  if (f->face_id == face_id)
	      return;
	  f = f->next;
      }

    /* inserting the Face-ID into the list */
    f = create_face_item (face_id);
    if (list->first_face == NULL)
	list->first_face = f;
    if (list->last_face != NULL)
	list->last_face->next = f;
    list->last_face = f;
}

TOPOLOGY_PRIVATE void
auxtopo_select_valid_face_edges (struct face_edges *list)
{
/* identifying all useless Edges */
    struct face_edge_item *fe = list->first_edge;
    while (fe != NULL)
      {
	  struct face_item *f = list->first_face;
	  while (f != NULL)
	    {
		if (f->face_id == fe->left_face)
		    fe->count += 1;
		if (f->face_id == fe->right_face)
		    fe->count += 1;
		f = f->next;
	    }
	  fe = fe->next;
      }
}

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_polygonize_face_edges (struct face_edges *list, const void *cache)
{
/* attempting to reaggregrate Polygons from valid Edges */
    gaiaGeomCollPtr sparse;
    gaiaGeomCollPtr rearranged;
    struct face_edge_item *fe;

    if (list->has_z)
	sparse = gaiaAllocGeomCollXYZ ();
    else
	sparse = gaiaAllocGeomColl ();
    sparse->Srid = list->srid;

    fe = list->first_edge;
    while (fe != NULL)
      {
	  if (fe->count < 2)
	    {
		/* found a valid Edge: adding to the MultiListring */
		gaiaLinestringPtr ln = fe->geom->FirstLinestring;
		while (ln != NULL)
		  {
		      if (list->has_z)
			  auxtopo_copy_linestring3d (ln, sparse);
		      else
			  auxtopo_copy_linestring (ln, sparse);
		      ln = ln->Next;
		  }
	    }
	  fe = fe->next;
      }
    rearranged = gaiaPolygonize_r (cache, sparse, 0);
    gaiaFreeGeomColl (sparse);
    return rearranged;
}

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_polygonize_face_edges_generalize (struct face_edges * list,
					  const void *cache)
{
/* attempting to reaggregrate Polygons from valid Edges */
    gaiaGeomCollPtr sparse;
    gaiaGeomCollPtr renoded;
    gaiaGeomCollPtr rearranged;
    struct face_edge_item *fe;

    if (list->has_z)
	sparse = gaiaAllocGeomCollXYZ ();
    else
	sparse = gaiaAllocGeomColl ();
    sparse->Srid = list->srid;

    fe = list->first_edge;
    while (fe != NULL)
      {
	  if (fe->count < 2)
	    {
		/* found a valid Edge: adding to the MultiListring */
		gaiaLinestringPtr ln = fe->geom->FirstLinestring;
		while (ln != NULL)
		  {
		      if (list->has_z)
			  auxtopo_copy_linestring3d (ln, sparse);
		      else
			  auxtopo_copy_linestring (ln, sparse);
		      ln = ln->Next;
		  }
	    }
	  fe = fe->next;
      }
    renoded = gaiaNodeLines (sparse);
    gaiaFreeGeomColl (sparse);
    if (renoded == NULL)
	return NULL;
    rearranged = gaiaPolygonize_r (cache, renoded, 0);
    gaiaFreeGeomColl (renoded);
    return rearranged;
}

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_make_geom_from_point (int srid, int has_z, gaiaPointPtr pt)
{
/* quick constructor: Geometry based on external point */
    gaiaGeomCollPtr reference;
    if (has_z)
	reference = gaiaAllocGeomCollXYZ ();
    else
	reference = gaiaAllocGeomColl ();
    reference->Srid = srid;
    pt->Next = NULL;
    reference->FirstPoint = pt;
    reference->LastPoint = pt;
    return reference;
}

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_make_geom_from_line (int srid, gaiaLinestringPtr ln)
{
/* quick constructor: Geometry based on external line */
    gaiaGeomCollPtr reference;
    if (ln->DimensionModel == GAIA_XY_Z_M)
	reference = gaiaAllocGeomCollXYZM ();
    else if (ln->DimensionModel == GAIA_XY_Z)
	reference = gaiaAllocGeomCollXYZ ();
    else if (ln->DimensionModel == GAIA_XY_M)
	reference = gaiaAllocGeomCollXYM ();
    else
	reference = gaiaAllocGeomColl ();
    reference->Srid = srid;
    ln->Next = NULL;
    reference->FirstLinestring = ln;
    reference->LastLinestring = ln;
    return reference;
}

TOPOLOGY_PRIVATE void
auxtopo_copy_linestring3d (gaiaLinestringPtr in, gaiaGeomCollPtr geom)
{
/* inserting/copying a Linestring 3D into another Geometry */
    int iv;
    double x;
    double y;
    double z;
    gaiaLinestringPtr out = gaiaAddLinestringToGeomColl (geom, in->Points);
    for (iv = 0; iv < in->Points; iv++)
      {
	  gaiaGetPointXYZ (in->Coords, iv, &x, &y, &z);
	  gaiaSetPointXYZ (out->Coords, iv, x, y, z);
      }
}

TOPOLOGY_PRIVATE void
auxtopo_copy_linestring (gaiaLinestringPtr in, gaiaGeomCollPtr geom)
{
/* inserting/copying a Linestring into another Geometry */
    int iv;
    double x;
    double y;
    gaiaLinestringPtr out = gaiaAddLinestringToGeomColl (geom, in->Points);
    for (iv = 0; iv < in->Points; iv++)
      {
	  gaiaGetPoint (in->Coords, iv, &x, &y);
	  gaiaSetPoint (out->Coords, iv, x, y);
      }
}

TOPOLOGY_PRIVATE void
auxtopo_destroy_geom_from (gaiaGeomCollPtr reference)
{
/* safely destroying a reference geometry */
    if (reference == NULL)
	return;

/* releasing ownership on external points, lines and polygs */
    reference->FirstPoint = NULL;
    reference->LastPoint = NULL;
    reference->FirstLinestring = NULL;
    reference->LastLinestring = NULL;
    reference->FirstPolygon = NULL;
    reference->LastPolygon = NULL;

    gaiaFreeGeomColl (reference);
}

TOPOLOGY_PRIVATE int
auxtopo_retrieve_geometry_type (sqlite3 * db_handle, const char *db_prefix,
				const char *table, const char *column,
				int *ref_type)
{
/* attempting to retrive the reference Geometry Type */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    char *xprefix;
    int type = -1;

/* querying GEOMETRY_COLUMNS */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT geometry_type "
	 "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) AND "
	 "Lower(f_geometry_column) = Lower(%Q)", xprefix, table, column);
    free (xprefix);
    ret =
	sqlite3_get_table (db_handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  type = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);

    if (type < 0)
	return 0;

    *ref_type = type;
    return 1;
}

TOPOLOGY_PRIVATE int
auxtopo_create_togeotable_sql (sqlite3 * db_handle, const char *db_prefix,
			       const char *ref_table, const char *ref_column,
			       const char *out_table, char **xcreate,
			       char **xselect, char **xinsert,
			       int *ref_geom_col)
{
/* composing the CREATE TABLE output-table statement */
    char *create = NULL;
    char *select = NULL;
    char *insert = NULL;
    char *prev;
    char *sql;
    char *xprefix;
    char *xtable;
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;
    const char *type;
    int notnull;
    int pk_no;
    int ret;
    int first_create = 1;
    int first_select = 1;
    int first_insert = 1;
    int npk = 0;
    int ipk;
    int ncols = 0;
    int icol;
    int ref_col = 0;
    int xref_geom_col;

    *xcreate = NULL;
    *xselect = NULL;
    *xinsert = NULL;
    *ref_geom_col = -1;

    xtable = gaiaDoubleQuotedSql (out_table);
    create = sqlite3_mprintf ("CREATE TABLE MAIN.\"%s\" (", xtable);
    select = sqlite3_mprintf ("SELECT ");
    insert = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (", xtable);
    free (xtable);

    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		/* counting how many PK columns are there */
		if (atoi (results[(i * columns) + 5]) != 0)
		    npk++;
	    }
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		type = results[(i * columns) + 2];
		notnull = atoi (results[(i * columns) + 3]);
		pk_no = atoi (results[(i * columns) + 5]);
		/* SELECT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = select;
		if (first_select)
		    select = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    select = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_select = 0;
		free (xprefix);
		sqlite3_free (prev);
		if (strcasecmp (name, ref_column) == 0)
		  {
		      /* saving the index of ref-geometry */
		      xref_geom_col = ref_col;
		  }
		ref_col++;
		/* INSERT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = insert;
		if (first_insert)
		    insert = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    insert = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_insert = 0;
		free (xprefix);
		sqlite3_free (prev);
		ncols++;
		/* CREATE: adding a column definition */
		if (strcasecmp (name, ref_column) == 0)
		  {
		      /* skipping the geometry column */
		      continue;
		  }
		prev = create;
		xprefix = gaiaDoubleQuotedSql (name);
		if (first_create)
		  {
		      first_create = 0;
		      if (notnull)
			  create =
			      sqlite3_mprintf ("%s\n\t\"%s\" %s NOT NULL", prev,
					       xprefix, type);
		      else
			  create =
			      sqlite3_mprintf ("%s\n\t\"%s\" %s", prev, xprefix,
					       type);
		  }
		else
		  {
		      if (notnull)
			  create =
			      sqlite3_mprintf ("%s,\n\t\"%s\" %s NOT NULL",
					       prev, xprefix, type);
		      else
			  create =
			      sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev,
					       xprefix, type);
		  }
		free (xprefix);
		sqlite3_free (prev);
		if (npk == 1 && pk_no != 0)
		  {
		      /* declaring a single-column Primary Key */
		      prev = create;
		      create = sqlite3_mprintf ("%s PRIMARY KEY", prev);
		      sqlite3_free (prev);
		  }
	    }
	  if (npk > 1)
	    {
		/* declaring a multi-column Primary Key */
		prev = create;
		sql = sqlite3_mprintf ("pk_%s", out_table);
		xprefix = gaiaDoubleQuotedSql (sql);
		sqlite3_free (sql);
		create =
		    sqlite3_mprintf ("%s,\n\tCONSTRAINT \"%s\" PRIMARY KEY (",
				     prev, xprefix);
		free (xprefix);
		sqlite3_free (prev);
		for (ipk = 1; ipk <= npk; ipk++)
		  {
		      /* searching a Primary Key column */
		      for (i = 1; i <= rows; i++)
			{
			    if (atoi (results[(i * columns) + 5]) == ipk)
			      {
				  /* declaring a Primary Key column */
				  name = results[(i * columns) + 1];
				  xprefix = gaiaDoubleQuotedSql (name);
				  prev = create;
				  if (ipk == 1)
				      create =
					  sqlite3_mprintf ("%s\"%s\"", prev,
							   xprefix);
				  else
				      create =
					  sqlite3_mprintf ("%s, \"%s\"", prev,
							   xprefix);
				  free (xprefix);
				  sqlite3_free (prev);
			      }
			}
		  }
		prev = create;
		create = sqlite3_mprintf ("%s)", prev);
		sqlite3_free (prev);
	    }
      }
    sqlite3_free_table (results);

/* completing the SQL statements */
    prev = create;
    create = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);
    prev = select;
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    select = sqlite3_mprintf ("%s FROM \"%s\".\"%s\"", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = insert;
    insert = sqlite3_mprintf ("%s) VALUES (", prev);
    sqlite3_free (prev);
    for (icol = 0; icol < ncols; icol++)
      {
	  prev = insert;
	  if (icol == 0)
	      insert = sqlite3_mprintf ("%s?", prev);
	  else
	      insert = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
      }
    prev = insert;
    insert = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);

    *xcreate = create;
    *xselect = select;
    *xinsert = insert;
    *ref_geom_col = xref_geom_col;
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    return 0;
}

static int
is_geometry_column (sqlite3 * db_handle, const char *db_prefix,
		    const char *table, const char *column)
{
/* testing for Geometry columns */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    char *xprefix;
    int count = 0;

/* querying GEOMETRY_COLUMNS */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT Count(*) "
	 "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) AND "
	 "Lower(f_geometry_column) = Lower(%Q)", xprefix, table, column);
    free (xprefix);
    ret =
	sqlite3_get_table (db_handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  count = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);

    if (count > 0)
	return 1;
    return 0;
}

static int
auxtopo_create_features_sql (sqlite3 * db_handle, const char *db_prefix,
			     const char *ref_table, const char *ref_column,
			     const char *topology_name,
			     sqlite3_int64 topolayer_id, char **xcreate,
			     char **xselect, char **xinsert)
{
/* composing the CREATE TABLE fatures-table statement */
    char *create = NULL;
    char *select = NULL;
    char *insert = NULL;
    char *prev;
    char *sql;
    char *xprefix;
    char *xgeom;
    char *table;
    char *xtable;
    char dummy[64];
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;
    const char *type;
    int notnull;
    int ret;
    int first_select = 1;
    int first_insert = 1;
    int ncols = 0;
    int icol;
    int ref_col = 0;

    *xcreate = NULL;
    *xselect = NULL;
    *xinsert = NULL;

    sprintf (dummy, "%lld", topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    create =
	sqlite3_mprintf
	("CREATE TABLE MAIN.\"%s\" (\n\tfid INTEGER PRIMARY KEY AUTOINCREMENT",
	 xtable);
    select = sqlite3_mprintf ("SELECT ");
    insert = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (", xtable);
    free (xtable);

    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		type = results[(i * columns) + 2];
		notnull = atoi (results[(i * columns) + 3]);
		if (strcasecmp (name, "fid") == 0)
		    continue;
		if (is_geometry_column (db_handle, db_prefix, ref_table, name))
		    continue;
		if (ref_column != NULL)
		  {
		      if (strcasecmp (ref_column, name) == 0)
			  continue;
		  }
		/* SELECT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = select;
		if (first_select)
		    select = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    select = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_select = 0;
		free (xprefix);
		sqlite3_free (prev);
		ref_col++;
		/* INSERT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = insert;
		if (first_insert)
		    insert = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    insert = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_insert = 0;
		free (xprefix);
		sqlite3_free (prev);
		ncols++;
		/* CREATE: adding a column definition */
		prev = create;
		xprefix = gaiaDoubleQuotedSql (name);
		if (notnull)
		    create =
			sqlite3_mprintf ("%s,\n\t\"%s\" %s NOT NULL",
					 prev, xprefix, type);
		else
		    create =
			sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev,
					 xprefix, type);
		free (xprefix);
		sqlite3_free (prev);
	    }
      }
    sqlite3_free_table (results);

/* completing the SQL statements */
    prev = create;
    create = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);
    prev = select;
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    if (ref_column != NULL)
      {
	  xgeom = gaiaDoubleQuotedSql (ref_column);
	  select =
	      sqlite3_mprintf ("%s, \"%s\" FROM \"%s\".\"%s\"", prev, xgeom,
			       xprefix, xtable);
	  free (xgeom);
      }
    else
	select =
	    sqlite3_mprintf ("%s FROM \"%s\".\"%s\"", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = insert;
    insert = sqlite3_mprintf ("%s) VALUES (", prev);
    sqlite3_free (prev);
    for (icol = 0; icol < ncols; icol++)
      {
	  prev = insert;
	  if (icol == 0)
	      insert = sqlite3_mprintf ("%s?", prev);
	  else
	      insert = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
      }
    prev = insert;
    insert = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);

    *xcreate = create;
    *xselect = select;
    *xinsert = insert;
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    return 0;
}

static int
do_register_topolayer (struct gaia_topology *topo, const char *topolayer_name,
		       sqlite3_int64 * topolayer_id)
{
/* attempting to register a new TopoLayer */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    char *err_msg = NULL;

    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (topolayer_name) VALUES (Lower(%Q))", xtable,
	 topolayer_name);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("RegisterTopoLayer error: \"%s\"", err_msg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (err_msg);
	  sqlite3_free (msg);
	  return 0;
      }

    *topolayer_id = sqlite3_last_insert_rowid (topo->db_handle);
    return 1;
}

static int
do_eval_topolayer_point (struct gaia_topology *topo, gaiaGeomCollPtr reference,
			 sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_rels,
			 sqlite3_int64 topolayer_id, sqlite3_int64 fid)
{
/* retrieving Points from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;

/* initializing the Topo-Node query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_node);
    sqlite3_clear_bindings (stmt_node);
    sqlite3_bind_blob (stmt_node, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_node, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_node);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_node, 0);
		sqlite3_reset (stmt_rels);
		sqlite3_clear_bindings (stmt_rels);
		sqlite3_bind_int64 (stmt_rels, 1, node_id);
		sqlite3_bind_null (stmt_rels, 2);
		sqlite3_bind_null (stmt_rels, 3);
		sqlite3_bind_int64 (stmt_rels, 4, topolayer_id);
		sqlite3_bind_int64 (stmt_rels, 5, fid);
		ret = sqlite3_step (stmt_rels);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

static int
do_eval_topolayer_line (struct gaia_topology *topo, gaiaGeomCollPtr reference,
			sqlite3_stmt * stmt_edge, sqlite3_stmt * stmt_rels,
			sqlite3_int64 topolayer_id, sqlite3_int64 fid)
{
/* retrieving Linestrings from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;

/* initializing the Topo-Edge query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_edge);
    sqlite3_clear_bindings (stmt_edge);
    sqlite3_bind_blob (stmt_edge, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_edge, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_edge, 0);
		sqlite3_reset (stmt_rels);
		sqlite3_clear_bindings (stmt_rels);
		sqlite3_bind_null (stmt_rels, 1);
		sqlite3_bind_int64 (stmt_rels, 2, edge_id);
		sqlite3_bind_null (stmt_rels, 3);
		sqlite3_bind_int64 (stmt_rels, 4, topolayer_id);
		sqlite3_bind_int64 (stmt_rels, 5, fid);
		ret = sqlite3_step (stmt_rels);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

static int
do_eval_topolayer_polyg (struct gaia_topology *topo, gaiaGeomCollPtr reference,
			 sqlite3_stmt * stmt_face, sqlite3_stmt * stmt_rels,
			 sqlite3_int64 topolayer_id, sqlite3_int64 fid)
{
/* retrieving Polygon from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;

/* initializing the Topo-Face query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_face);
    sqlite3_clear_bindings (stmt_face);
    sqlite3_bind_blob (stmt_face, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_face, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_face);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 face_id = sqlite3_column_int64 (stmt_face, 0);
		sqlite3_reset (stmt_rels);
		sqlite3_clear_bindings (stmt_rels);
		sqlite3_bind_null (stmt_rels, 1);
		sqlite3_bind_null (stmt_rels, 2);
		sqlite3_bind_int64 (stmt_rels, 3, face_id);
		sqlite3_bind_int64 (stmt_rels, 4, topolayer_id);
		sqlite3_bind_int64 (stmt_rels, 5, fid);
		ret = sqlite3_step (stmt_rels);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

static int
do_eval_topolayer_geom (struct gaia_topology *topo, gaiaGeomCollPtr geom,
			sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_edge,
			sqlite3_stmt * stmt_face, sqlite3_stmt * stmt_rels,
			sqlite3_int64 topolayer_id, sqlite3_int64 fid)
{
/* retrieving Features via matching Seeds/Geometries */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int ret;

    /* processing all Points */
    pt = geom->FirstPoint;
    while (pt != NULL)
      {
	  gaiaPointPtr next = pt->Next;
	  gaiaGeomCollPtr reference = (gaiaGeomCollPtr)
	      auxtopo_make_geom_from_point (topo->srid, topo->has_z, pt);
	  ret =
	      do_eval_topolayer_point (topo, reference, stmt_node, stmt_rels,
				       topolayer_id, fid);
	  auxtopo_destroy_geom_from (reference);
	  pt->Next = next;
	  if (ret == 0)
	      return 0;
	  pt = pt->Next;
      }

    /* processing all Linestrings */
    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  gaiaLinestringPtr next = ln->Next;
	  gaiaGeomCollPtr reference = (gaiaGeomCollPtr)
	      auxtopo_make_geom_from_line (topo->srid, ln);
	  ret =
	      do_eval_topolayer_line (topo, reference, stmt_edge, stmt_rels,
				      topolayer_id, fid);
	  auxtopo_destroy_geom_from (reference);
	  ln->Next = next;
	  if (ret == 0)
	      return 0;
	  ln = ln->Next;
      }

    /* processing all Polygons */
    pg = geom->FirstPolygon;
    while (pg != NULL)
      {
	  gaiaPolygonPtr next = pg->Next;
	  gaiaGeomCollPtr reference = make_geom_from_polyg (topo->srid, pg);
	  ret = do_eval_topolayer_polyg (topo, reference, stmt_face, stmt_rels,
					 topolayer_id, fid);
	  auxtopo_destroy_geom_from (reference);
	  pg->Next = next;
	  if (ret == 0)
	      return 0;
	  pg = pg->Next;
      }

    return 1;
}

static int
do_eval_topolayer_seeds (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
			 sqlite3_stmt * stmt_ins, sqlite3_stmt * stmt_rels,
			 sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_edge,
			 sqlite3_stmt * stmt_face, sqlite3_int64 topolayer_id)
{
/* querying the ref-table */
    int ret;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    while (1)
      {
	  /* scrolling the result set rows */
	  gaiaGeomCollPtr geom = NULL;
	  sqlite3_int64 fid;
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      if (icol == ncol - 1)
			{
			    /* the last column is always expected to be the geometry column */
			    const unsigned char *blob =
				sqlite3_column_blob (stmt_ref, icol);
			    int blob_sz = sqlite3_column_bytes (stmt_ref, icol);
			    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
			    continue;
			}
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
		fid = sqlite3_last_insert_rowid (topo->db_handle);
		/* evaluating the feature's geometry */
		if (geom != NULL)
		  {
		      ret =
			  do_eval_topolayer_geom (topo, geom, stmt_node,
						  stmt_edge, stmt_face,
						  stmt_rels, topolayer_id, fid);
		      gaiaFreeGeomColl (geom);
		      if (ret == 0)
			  return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_CreateTopoLayer (GaiaTopologyAccessorPtr accessor,
			     const char *db_prefix, const char *ref_table,
			     const char *ref_column, const char *topolayer_name)
{
/* attempting to create a TopoLayer */
    sqlite3_int64 topolayer_id;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_rels = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    sqlite3_stmt *stmt_face = NULL;
    int ret;
    char *sql;
    char *create;
    char *select;
    char *insert;
    char *xprefix;
    char *table;
    char *xtable;
    char *errMsg;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    if (topo == NULL)
	return 0;

/* attempting to register a new TopoLayer */
    if (!do_register_topolayer (topo, topolayer_name, &topolayer_id))
	return 0;

/* incrementally updating all Topology Seeds */
    if (!gaiaTopoGeoUpdateSeeds (accessor, 1))
	return 0;

/* composing the CREATE TABLE feature-table statement */
    if (!auxtopo_create_features_sql
	(topo->db_handle, db_prefix, ref_table, ref_column, topo->topology_name,
	 topolayer_id, &create, &select, &insert))
	goto error;

/* creating the feature-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "SELECT * FROM ref-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO features-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO features-ref" query */
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (node_id, edge_id, face_id, topolayer_id, fid) "
	 "VALUES (?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rels,
			    NULL);
    sqlite3_free (sql);
    sql = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Edges query */
    xprefix = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT edge_id FROM MAIN.\"%s\" "
			   "WHERE edge_id IS NOT NULL AND ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Faces query */
    xprefix = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT face_id FROM MAIN.\"%s\" "
			   "WHERE face_id IS NOT NULL AND ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT node_id FROM MAIN.\"%s\" "
			   "WHERE ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating feature/topology matching via coincident topo-seeds */
    if (!do_eval_topolayer_seeds
	(topo, stmt_ref, stmt_ins, stmt_rels, stmt_node, stmt_edge, stmt_face,
	 topolayer_id))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_rels);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_face);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_rels != NULL)
	sqlite3_finalize (stmt_rels);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    return 0;
}

static int
do_populate_topolayer (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
		       sqlite3_stmt * stmt_ins)
{
/* querying the ref-table */
    int ret;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_InitTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_InitTopoLayer() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_InitTopoLayer (GaiaTopologyAccessorPtr accessor,
			   const char *db_prefix, const char *ref_table,
			   const char *topolayer_name)
{
/* attempting to create a TopoLayer */
    sqlite3_int64 topolayer_id;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    int ret;
    char *create;
    char *select;
    char *insert;
    char *errMsg;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    if (topo == NULL)
	return 0;

/* attempting to register a new TopoLayer */
    if (!do_register_topolayer (topo, topolayer_name, &topolayer_id))
	return 0;

/* composing the CREATE TABLE feature-table statement */
    if (!auxtopo_create_features_sql
	(topo->db_handle, db_prefix, ref_table, NULL, topo->topology_name,
	 topolayer_id, &create, &select, &insert))
	goto error;

/* creating the feature-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_InitTopoLayer() error: \"%s\"",
				       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "SELECT * FROM ref-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO features-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* populating the TopoFeatures table */
    if (!do_populate_topolayer (topo, stmt_ref, stmt_ins))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    return 0;
}

static int
check_topolayer (struct gaia_topology *topo, const char *topolayer_name,
		 sqlite3_int64 * topolayer_id)
{
/* checking if a TopoLayer do really exist */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    int found = 0;
    sqlite3_stmt *stmt = NULL;

/* creating the SQL statement - SELECT */
    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT topolayer_id FROM \"%s\" WHERE topolayer_name = Lower(%Q)",
	 xtable, topolayer_name);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Check_TopoLayer() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* retrieving the TopoLayer ID */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		*topolayer_id = sqlite3_column_int64 (stmt, 0);
		found = 1;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("Check_TopoLayer() error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    if (!found)
	goto error;

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
do_unregister_topolayer (struct gaia_topology *topo, const char *topolayer_name,
			 sqlite3_int64 * topolayer_id)
{
/* attempting to unregister a existing TopoLayer */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    sqlite3_int64 id;
    sqlite3_stmt *stmt = NULL;

    if (!check_topolayer (topo, topolayer_name, &id))
	return 0;

/* creating the first SQL statement - DELETE */
    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\" WHERE topolayer_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    create_all_topo_prepared_stmts (topo->cache);	/* recreating prepared stsms */
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* deleting the TopoLayer */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    *topolayer_id = id;
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_RemoveTopoLayer (GaiaTopologyAccessorPtr accessor,
			     const char *topolayer_name)
{
/* attempting to remove a TopoLayer */
    sqlite3_int64 topolayer_id;
    int ret;
    char *sql;
    char *errMsg;
    char *table;
    char *xtable;
    char *xtable2;
    char dummy[64];
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    if (topo == NULL)
	return 0;

/* deleting all Feature relations */
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\" WHERE topolayer_id = "
			   "(SELECT topolayer_id FROM \"%s\" WHERE topolayer_name = Lower(%Q))",
			   xtable, xtable2, topolayer_name);
    free (xtable);
    free (xtable2);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_RemoveTopoLayer() error: %s\n",
				       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* unregistering the TopoLayer */
    if (!do_unregister_topolayer (topo, topolayer_name, &topolayer_id))
	return 0;

/* finalizing all prepared Statements */
    finalize_all_topo_prepared_stmts (topo->cache);

/* dropping the TopoFeatures Table */
    sprintf (dummy, "%lld", topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    create_all_topo_prepared_stmts (topo->cache);	/* recreating prepared stsms */
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_RemoveTopoLayer() error: %s\n",
				       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

static int
is_unique_geom_name (sqlite3 * sqlite, const char *table, const char *geom)
{
/* checking for duplicate names */
    char *xtable;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;

    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA MAIN.table_info(\"%s\")", xtable);
    free (xtable);
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
		name = results[(i * columns) + 1];
		if (strcasecmp (name, geom) == 0)
		    continue;
	    }
      }
    sqlite3_free_table (results);

    return 1;
}

static int
auxtopo_create_export_sql (struct gaia_topology *topo,
			   const char *topolayer_name, const char *out_table,
			   char **xcreate, char **xselect, char **xinsert,
			   char **geometry, sqlite3_int64 * topolayer_id)
{
/* composing the CREATE TABLE export-table statement */
    char *create = NULL;
    char *select = NULL;
    char *insert = NULL;
    char *prev;
    char *sql;
    char *table;
    char *xtable;
    char *xprefix;
    char dummy[64];
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;
    const char *type;
    int notnull;
    int ret;
    int first_select = 1;
    int first_insert = 1;
    int ncols = 0;
    int icol;
    int ref_col = 0;
    char *geometry_name;
    int geom_alias = 0;

    *xcreate = NULL;
    *xselect = NULL;
    *xinsert = NULL;

/* checking the TopoLayer */
    if (!check_topolayer (topo, topolayer_name, topolayer_id))
	return 0;

    xtable = gaiaDoubleQuotedSql (out_table);
    create =
	sqlite3_mprintf
	("CREATE TABLE MAIN.\"%s\" (\n\tfid INTEGER PRIMARY KEY", xtable);
    select = sqlite3_mprintf ("SELECT fid, ");
    insert = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (fid, ", xtable);
    free (xtable);
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("PRAGMA MAIN.table_info(\"%s\")", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcmp (name, "fid") == 0)
		    continue;
		type = results[(i * columns) + 2];
		notnull = atoi (results[(i * columns) + 3]);
		/* SELECT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = select;
		if (first_select)
		    select = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    select = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_select = 0;
		free (xprefix);
		sqlite3_free (prev);
		ref_col++;
		/* INSERT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = insert;
		if (first_insert)
		    insert = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    insert = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_insert = 0;
		free (xprefix);
		sqlite3_free (prev);
		ncols++;
		/* CREATE: adding a column definition */
		prev = create;
		xprefix = gaiaDoubleQuotedSql (name);
		if (notnull)
		    create =
			sqlite3_mprintf ("%s,\n\t\"%s\" %s NOT NULL",
					 prev, xprefix, type);
		else
		    create =
			sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev,
					 xprefix, type);
		free (xprefix);
		sqlite3_free (prev);
	    }
      }
    sqlite3_free_table (results);

    geometry_name = malloc (strlen ("geometry") + 1);
    strcpy (geometry_name, "geometry");
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    while (1)
      {
	  /* searching an unique Geometry name */
	  if (is_unique_geom_name (topo->db_handle, table, geometry_name))
	      break;
	  sprintf (dummy, "geom_%d", ++geom_alias);
	  free (geometry_name);
	  geometry_name = malloc (strlen (dummy) + 1);
	  strcpy (geometry_name, dummy);
      }
    sqlite3_free (table);

/* completing the SQL statements */
    prev = create;
    create = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);
    prev = select;
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    select = sqlite3_mprintf ("%s FROM MAIN.\"%s\"", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    prev = insert;
    insert = sqlite3_mprintf ("%s, \"%s\") VALUES (?, ", prev, geometry_name);
    sqlite3_free (prev);
    for (icol = 0; icol < ncols; icol++)
      {
	  prev = insert;
	  if (icol == 0)
	      insert = sqlite3_mprintf ("%s?", prev);
	  else
	      insert = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
      }
    prev = insert;
    insert = sqlite3_mprintf ("%s, ?)", prev);
    sqlite3_free (prev);

    *xcreate = create;
    *xselect = select;
    *xinsert = insert;
    *geometry = geometry_name;
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    return 0;
}

static int
auxtopo_retrieve_export_geometry_type (struct gaia_topology *topo,
				       const char *topolayer_name,
				       int *ref_type)
{
/* determining the Geometry Type for Export TopoLayer */
    char *table;
    char *xtable;
    char *xtable2;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int nodes;
    int edges;
    int faces;

    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT Count(f.node_id), Count(f.edge_id), Count(f.face_id) "
	 "FROM \"%s\" AS l JOIN \"%s\" AS f ON (l.topolayer_id = f.topolayer_id) "
	 "WHERE l.topolayer_name = Lower(%Q)", xtable, xtable2, topolayer_name);
    free (xtable);
    free (xtable2);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		nodes = atoi (results[(i * columns) + 0]);
		edges = atoi (results[(i * columns) + 1]);
		faces = atoi (results[(i * columns) + 2]);
	    }
      }
    sqlite3_free_table (results);

    *ref_type = GAIA_UNKNOWN;
    if (nodes && !edges && !faces)
	*ref_type = GAIA_POINT;
    if (!nodes && edges && !faces)
	*ref_type = GAIA_LINESTRING;
    if (!nodes && !edges && faces)
	*ref_type = GAIA_POLYGON;

    return 1;
}

static void
do_eval_topo_node (struct gaia_topology *topo, sqlite3_stmt * stmt_node,
		   sqlite3_int64 node_id, gaiaGeomCollPtr result)
{
/* retrieving Points from Topology Nodes */
    int ret;

/* initializing the Topo-Node query */
    sqlite3_reset (stmt_node);
    sqlite3_clear_bindings (stmt_node);
    sqlite3_bind_int64 (stmt_node, 1, node_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_node);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_node, 0);
		int blob_sz = sqlite3_column_bytes (stmt_node, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaPointPtr pt = geom->FirstPoint;
		      while (pt != NULL)
			{
			    /* copying all Points into the result Geometry */
			    if (topo->has_z)
				gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y,
							   pt->Z);
			    else
				gaiaAddPointToGeomColl (result, pt->X, pt->Y);
			    pt = pt->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_FeatureFromTopoLayer error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_eval_topo_edge (struct gaia_topology *topo, sqlite3_stmt * stmt_edge,
		   sqlite3_int64 edge_id, gaiaGeomCollPtr result)
{
/* retrieving Linestrings from Topology Edges */
    int ret;

/* initializing the Topo-Edge query */
    sqlite3_reset (stmt_edge);
    sqlite3_clear_bindings (stmt_edge);
    sqlite3_bind_int64 (stmt_edge, 1, edge_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_edge, 0);
		int blob_sz = sqlite3_column_bytes (stmt_edge, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaLinestringPtr ln = geom->FirstLinestring;
		      while (ln != NULL)
			{
			    /* copying all Linestrings into the result Geometry */
			    if (topo->has_z)
				auxtopo_copy_linestring3d (ln, result);
			    else
				auxtopo_copy_linestring (ln, result);
			    ln = ln->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_FeatureFromTopoLayer error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static gaiaGeomCollPtr
do_eval_topo_geometry (struct gaia_topology *topo, sqlite3_stmt * stmt_rels,
		       sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_edge,
		       sqlite3_stmt * stmt_face, sqlite3_int64 fid,
		       sqlite3_int64 topolayer_id, int out_type)
{
/* materializing a Geometry out of Topology */
    int ret;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr sparse_lines;
    struct face_edges *list =
	auxtopo_create_face_edges (topo->has_z, topo->srid);

    if (topo->has_z)
      {
	  geom = gaiaAllocGeomCollXYZ ();
	  sparse_lines = gaiaAllocGeomCollXYZ ();
      }
    else
      {
	  geom = gaiaAllocGeomColl ();
	  sparse_lines = gaiaAllocGeomColl ();
      }
    geom->Srid = topo->srid;
    geom->DeclaredType = out_type;

    sqlite3_reset (stmt_rels);
    sqlite3_clear_bindings (stmt_rels);
    sqlite3_bind_int64 (stmt_rels, 1, topolayer_id);
    sqlite3_bind_int64 (stmt_rels, 2, fid);
    while (1)
      {
	  /* scrolling the result set rows */
	  sqlite3_int64 id;
	  ret = sqlite3_step (stmt_rels);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_rels, 0) != SQLITE_NULL)
		  {
		      id = sqlite3_column_int64 (stmt_rels, 0);
		      do_eval_topo_node (topo, stmt_node, id, geom);
		  }
		if (sqlite3_column_type (stmt_rels, 1) != SQLITE_NULL)
		  {
		      id = sqlite3_column_int64 (stmt_rels, 1);
		      do_eval_topo_edge (topo, stmt_edge, id, sparse_lines);
		  }
		if (sqlite3_column_type (stmt_rels, 2) != SQLITE_NULL)
		  {
		      id = sqlite3_column_int64 (stmt_rels, 2);
		      do_explode_topo_face (topo, list, stmt_face, id);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_FeatureFromTopoLayer() error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }


    if (sparse_lines->FirstLinestring != NULL)
      {
	  /* attempting to better rearrange sparse lines */
	  gaiaGeomCollPtr rearranged =
	      gaiaLineMerge_r (topo->cache, sparse_lines);
	  gaiaFreeGeomColl (sparse_lines);
	  if (rearranged != NULL)
	    {
		gaiaLinestringPtr ln = rearranged->FirstLinestring;
		while (ln != NULL)
		  {
		      if (topo->has_z)
			  auxtopo_copy_linestring3d (ln, geom);
		      else
			  auxtopo_copy_linestring (ln, geom);
		      ln = ln->Next;
		  }
		gaiaFreeGeomColl (rearranged);
	    }
      }
    else
	gaiaFreeGeomColl (sparse_lines);
    sparse_lines = NULL;

    if (list->first_edge != NULL)
      {
	  /* attempting to rearrange sparse lines into Polygons */
	  gaiaGeomCollPtr rearranged;
	  auxtopo_select_valid_face_edges (list);
	  rearranged = auxtopo_polygonize_face_edges (list, topo->cache);
	  auxtopo_free_face_edges (list);
	  if (rearranged != NULL)
	    {
		gaiaPolygonPtr pg = rearranged->FirstPolygon;
		while (pg != NULL)
		  {
		      if (topo->has_z)
			  do_copy_polygon3d (pg, geom);
		      else
			  do_copy_polygon (pg, geom);
		      pg = pg->Next;
		  }
		gaiaFreeGeomColl (rearranged);
	    }
      }

    if (geom->FirstPoint == NULL && geom->FirstLinestring == NULL
	&& geom->FirstPolygon == NULL)
	goto error;
    return geom;

  error:
    gaiaFreeGeomColl (geom);
    if (sparse_lines != NULL)
	gaiaFreeGeomColl (sparse_lines);
    return NULL;
}

static int
do_eval_topogeo_features (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
			  sqlite3_stmt * stmt_ins, sqlite3_stmt * stmt_rels,
			  sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_edge,
			  sqlite3_stmt * stmt_face, sqlite3_int64 topolayer_id,
			  int out_type)
{
/* querying the ref-table */
    int ret;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    while (1)
      {
	  /* scrolling the result set rows */
	  gaiaGeomCollPtr geom = NULL;
	  sqlite3_int64 fid;
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		fid = sqlite3_column_int64 (stmt_ref, 0);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		/* the Geometry column */
		ncol = sqlite3_bind_parameter_count (stmt_ins);
		geom =
		    do_eval_topo_geometry (topo, stmt_rels, stmt_node,
					   stmt_edge, stmt_face, fid,
					   topolayer_id, out_type);
		if (geom != NULL)
		  {
		      unsigned char *p_blob;
		      int n_bytes;
		      gaiaToSpatiaLiteBlobWkb (geom, &p_blob, &n_bytes);
		      sqlite3_bind_blob (stmt_ins, ncol, p_blob, n_bytes,
					 SQLITE_TRANSIENT);
		      free (p_blob);
		      gaiaFreeGeomColl (geom);
		  }
		else
		    sqlite3_bind_null (stmt_ins, ncol);
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_ExportTopoLayer (GaiaTopologyAccessorPtr accessor,
			     const char *topolayer_name, const char *out_table,
			     int with_spatial_index, int create_only)
{
/* attempting to export a full TopoLayer */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_rels = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    sqlite3_stmt *stmt_face = NULL;
    int ret;
    char *create = NULL;
    char *select = NULL;
    char *insert;
    char *geometry_name;
    char *sql;
    char *errMsg;
    char *xprefix;
    char *table;
    char *xtable;
    int ref_type;
    const char *type;
    int out_type;
    sqlite3_int64 topolayer_id;
    if (topo == NULL)
	return 0;

/* composing the CREATE TABLE output-table statement */
    if (!auxtopo_create_export_sql
	(topo, topolayer_name, out_table, &create,
	 &select, &insert, &geometry_name, &topolayer_id))
	goto error;

/* creating the output-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* checking the Geometry Type */
    if (!auxtopo_retrieve_export_geometry_type
	(topo, topolayer_name, &ref_type))
	goto error;
    switch (ref_type)
      {
      case GAIA_POINT:
	  type = "MULTIPOINT";
	  out_type = GAIA_MULTIPOINT;
	  break;
      case GAIA_LINESTRING:
	  type = "MULTILINESTRING";
	  out_type = GAIA_MULTILINESTRING;
	  break;
      case GAIA_POLYGON:
	  type = "MULTIPOLYGON";
	  out_type = GAIA_MULTIPOLYGON;
	  break;
      case GAIA_GEOMETRYCOLLECTION:
	  type = "GEOMETRYCOLLECTION";
	  out_type = GAIA_GEOMETRYCOLLECTION;
	  break;
      default:
	  type = "GEOMETRY";
	  out_type = GAIA_UNKNOWN;
	  break;
      };

/* creating the output Geometry Column */
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(Lower(%Q), %Q, %d, '%s', '%s')",
	 out_table, geometry_name, topo->srid, type,
	 (topo->has_z == 0) ? "XY" : "XYZ");
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (with_spatial_index)
      {
	  /* creating a Spatial Index supporting the Geometry Column */
	  sql =
	      sqlite3_mprintf
	      ("SELECT CreateSpatialIndex(Lower(%Q), %Q)",
	       out_table, geometry_name);
	  ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
				     errMsg);
		sqlite3_free (errMsg);
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    free (geometry_name);
    if (create_only)
      {
	  sqlite3_free (select);
	  sqlite3_free (insert);
	  return 1;
      }

/* preparing the "SELECT * FROM topo-features-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO out-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "SELECT * FROM topo-features" query */
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT node_id, edge_id, face_id FROM \"%s\" "
			   "WHERE topolayer_id = ? AND fid = ?", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rels,
			    NULL);
    sqlite3_free (sql);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE node_id = ?",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Edges query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE edge_id = ?",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Faces query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql =
	sqlite3_mprintf
	("SELECT edge_id, left_face, right_face, geom FROM MAIN.\"%s\" "
	 "WHERE left_face = ? OR right_face = ?", xtable);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating TopoLayer's Features */
    if (!do_eval_topogeo_features
	(topo, stmt_ref, stmt_ins, stmt_rels, stmt_node, stmt_edge, stmt_face,
	 topolayer_id, out_type))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_rels);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_face);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_rels != NULL)
	sqlite3_finalize (stmt_rels);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    return 0;
}

static int
check_output_table (struct gaia_topology *topo, const char *out_table,
		    int *out_type)
{
/* checking the output table */
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int count = 0;
    int type;

    sql =
	sqlite3_mprintf
	("SELECT geometry_type FROM MAIN.geometry_columns WHERE f_table_name = Lower(%Q)",
	 out_table);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		type = atoi (results[(i * columns) + 0]);
		count++;
	    }
      }
    sqlite3_free_table (results);

    if (count != 1)
	return 0;

    switch (type)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
	  *out_type = GAIA_POINT;
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  *out_type = GAIA_MULTIPOINT;
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  *out_type = GAIA_MULTILINESTRING;
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  *out_type = GAIA_MULTIPOLYGON;
	  break;
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  *out_type = GAIA_GEOMETRYCOLLECTION;
	  break;
      default:
	  *out_type = GAIA_UNKNOWN;
	  break;
      };
    return 1;
}

static int
auxtopo_export_feature_sql (struct gaia_topology *topo,
			    const char *topolayer_name, const char *out_table,
			    char **xselect, char **xinsert,
			    sqlite3_int64 * topolayer_id, int *out_type)
{
/* composing the CREATE TABLE insert-feature statement */
    char *select = NULL;
    char *insert = NULL;
    char *prev;
    char *sql;
    char *table;
    char *xtable;
    char *xprefix;
    char dummy[64];
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;
    int ret;
    int first_select = 1;
    int first_insert = 1;
    int ncols = 0;
    int icol;
    int ref_col = 0;
    char *geometry_name = NULL;
    int geom_alias = 0;

    *xselect = NULL;
    *xinsert = NULL;

/* checking the TopoLayer */
    if (!check_topolayer (topo, topolayer_name, topolayer_id))
	return 0;

/* checking the output table */
    if (!check_output_table (topo, out_table, out_type))
	return 0;

    xtable = gaiaDoubleQuotedSql (out_table);
    select = sqlite3_mprintf ("SELECT fid, ");
    insert = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (fid, ", xtable);
    free (xtable);
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("PRAGMA MAIN.table_info(\"%s\")", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcmp (name, "fid") == 0)
		    continue;
		/* SELECT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = select;
		if (first_select)
		    select = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    select = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_select = 0;
		free (xprefix);
		sqlite3_free (prev);
		ref_col++;
		/* INSERT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = insert;
		if (first_insert)
		    insert = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    insert = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_insert = 0;
		free (xprefix);
		sqlite3_free (prev);
		ncols++;
	    }
      }
    sqlite3_free_table (results);

    geometry_name = malloc (strlen ("geometry") + 1);
    strcpy (geometry_name, "geometry");
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    while (1)
      {
	  /* searching an unique Geometry name */
	  if (is_unique_geom_name (topo->db_handle, table, geometry_name))
	      break;
	  sprintf (dummy, "geom_%d", ++geom_alias);
	  free (geometry_name);
	  geometry_name = malloc (strlen (dummy) + 1);
	  strcpy (geometry_name, dummy);
      }
    sqlite3_free (table);

/* completing the SQL statements */
    prev = select;
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    select =
	sqlite3_mprintf ("%s FROM MAIN.\"%s\" WHERE fid = ?", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    prev = insert;
    insert = sqlite3_mprintf ("%s, \"%s\") VALUES (?, ", prev, geometry_name);
    sqlite3_free (prev);
    for (icol = 0; icol < ncols; icol++)
      {
	  prev = insert;
	  if (icol == 0)
	      insert = sqlite3_mprintf ("%s?", prev);
	  else
	      insert = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
      }
    prev = insert;
    insert = sqlite3_mprintf ("%s, ?)", prev);
    sqlite3_free (prev);

    free (geometry_name);
    *xselect = select;
    *xinsert = insert;
    return 1;

  error:
    if (geometry_name != NULL)
	free (geometry_name);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    return 0;
}

static int
do_eval_topogeo_single_feature (struct gaia_topology *topo,
				sqlite3_stmt * stmt_ref,
				sqlite3_stmt * stmt_ins,
				sqlite3_stmt * stmt_rels,
				sqlite3_stmt * stmt_node,
				sqlite3_stmt * stmt_edge,
				sqlite3_stmt * stmt_face,
				sqlite3_int64 topolayer_id, int out_type,
				sqlite3_int64 fid)
{
/* querying the ref-table */
    int ret;
    int count = 0;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    sqlite3_bind_int64 (stmt_ref, 1, fid);
    while (1)
      {
	  /* scrolling the result set rows */
	  gaiaGeomCollPtr geom = NULL;
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		/* the Geometry column */
		ncol = sqlite3_bind_parameter_count (stmt_ins);
		geom =
		    do_eval_topo_geometry (topo, stmt_rels, stmt_node,
					   stmt_edge, stmt_face, fid,
					   topolayer_id, out_type);
		if (geom != NULL)
		  {
		      unsigned char *p_blob;
		      int n_bytes;
		      gaiaToSpatiaLiteBlobWkb (geom, &p_blob, &n_bytes);
		      sqlite3_bind_blob (stmt_ins, ncol, p_blob, n_bytes,
					 SQLITE_TRANSIENT);
		      free (p_blob);
		      gaiaFreeGeomColl (geom);
		  }
		else
		    sqlite3_bind_null (stmt_ins, ncol);
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("InsertFeatureFromTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
		count++;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("InsertFeatureFromTopoLayer() error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    if (count <= 0)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("InsertFeatureFromTopoLayer(): not existing TopoFeature");
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_InsertFeatureFromTopoLayer (GaiaTopologyAccessorPtr accessor,
					const char *topolayer_name,
					const char *out_table,
					sqlite3_int64 fid)
{
/* attempting to insert a single TopoLayer's Feature into the output GeoTable */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_rels = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    sqlite3_stmt *stmt_face = NULL;
    int ret;
    char *select;
    char *insert;
    char *sql;
    char *xprefix;
    char *table;
    char *xtable;
    int out_type;
    sqlite3_int64 topolayer_id;
    if (topo == NULL)
	return 0;

/* composing the SQL statements */
    if (!auxtopo_export_feature_sql
	(topo, topolayer_name, out_table, &select, &insert, &topolayer_id,
	 &out_type))
	goto error;

/* preparing the "SELECT * FROM topo-features-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO out-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  goto error;
      }

/* preparing the "SELECT * FROM topo-features" query */
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT node_id, edge_id, face_id FROM \"%s\" "
			   "WHERE topolayer_id = ? AND fid = ?", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rels,
			    NULL);
    sqlite3_free (sql);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE node_id = ?",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Edges query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE edge_id = ?",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Faces query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql =
	sqlite3_mprintf
	("SELECT edge_id, left_face, right_face, geom FROM MAIN.\"%s\" "
	 "WHERE left_face = ? OR right_face = ?", xtable);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating TopoLayer's Features */
    if (!do_eval_topogeo_single_feature
	(topo, stmt_ref, stmt_ins, stmt_rels, stmt_node, stmt_edge, stmt_face,
	 topolayer_id, out_type, fid))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_rels);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_face);
    return 1;

  error:
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_rels != NULL)
	sqlite3_finalize (stmt_rels);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    return 0;
}

#endif /* end TOPOLOGY conditionals */
