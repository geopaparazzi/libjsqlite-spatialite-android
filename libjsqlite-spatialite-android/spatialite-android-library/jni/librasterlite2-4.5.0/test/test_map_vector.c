/*

 test_aux_geom.c -- RasterLite2 Test Case

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

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"
#include "spatialite/gaiaaux.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2graphics.h"

static gaiaGeomCollPtr
get_extent (int srid)
{
/* setting the Map's Full Extent */
    gaiaGeomCollPtr geom = NULL;
    double minx = 0.0;
    double maxx = 0.0;
    double miny = 0.0;
    double maxy = 0.0;

    if (srid == 4326)
      {
	  minx = 2.0;
	  miny = 49.0;
	  maxx = 8.0;
	  maxy = 55.0;
      }
    else
      {
	  minx = 450000.0;
	  miny = 5450000.0;
	  maxx = 780000.0;
	  maxy = 5780000.0;
      }

    geom = gaiaAllocGeomColl ();
    geom->Srid = srid;
    gaiaAddPointToGeomColl (geom, minx, miny);
    gaiaAddPointToGeomColl (geom, maxx, maxy);
    return geom;
}

static int
test_map_vector (sqlite3 * sqlite, const char *coverage, const char *style,
		 gaiaGeomCollPtr geom, int *extret)
{
/* exporting a Map Image (full rendered) */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    const char *format = "image/png";
    int i;

    path =
	sqlite3_mprintf ("./%s_map_%s_%d.%s", coverage, style, geom->Srid,
			 "png");
    for (i = 0; i < (int) strlen (path); i++)
      {
	  if (path[i] == ' ')
	      path[i] = '_';
      }

    sql =
	"SELECT BlobToFile(RL2_GetMapImageFromVector(NULL, ?, ?, ?, ?, ?, ?, ?, ?), ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_text (stmt, 5, style, strlen (style), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 6, format, strlen (format), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 7, "#804040", 7, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 8, 1);
    sqlite3_bind_text (stmt, 9, path, strlen (path), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    *extret += retcode;
    return retcode;
}

static int
test_coverage (sqlite3 * sqlite, const char *coverage, const char *style,
	       gaiaGeomCollPtr geom, int *extret)
{
/* testing XY, XYM, XYZ and XYZM geometries */
    char cvg[1024];

/* testing XY */
    sprintf (cvg, "%s", coverage);
    if (!test_map_vector (sqlite, cvg, style, geom, extret))
	return 0;
    *extret += 2;
/* testing XYZ */
    sprintf (cvg, "%s_z", coverage);
    if (!test_map_vector (sqlite, cvg, style, geom, extret))
	return 0;
    *extret += 2;
/* testing XYM */
    sprintf (cvg, "%s_m", coverage);
    if (!test_map_vector (sqlite, cvg, style, geom, extret))
	return 0;
    *extret += 2;
/* testing XYZM */
    sprintf (cvg, "%s_zm", coverage);
    if (!test_map_vector (sqlite, cvg, style, geom, extret))
	return 0;
    *extret += 2;
/* testing the DEFAULT style */
    sprintf (cvg, "%s", coverage);
    if (!test_map_vector (sqlite, cvg, "default", geom, extret))
	return 0;

    return 1;
}

static int
test_capi_base (sqlite3 * sqlite, const char *coverage, const char *style,
		gaiaGeomCollPtr geom, int toponet, int topogeo, int mode,
		const void *data, int *extret)
{
/* testing the C API */
    int ret;
    unsigned char *blob;
    int blob_sz;
    int retcode = 1;
    char *path;
    int i;
    unsigned char *rgb;
    unsigned char *alpha;
    unsigned char *png = NULL;
    int png_size;
    FILE *out;
    int half_transparent;
    rl2GraphicsContextPtr ctx = NULL;
    rl2GraphicsContextPtr ctx_labels = NULL;
    rl2GraphicsContextPtr ctx_nodes = NULL;
    rl2GraphicsContextPtr ctx_edges = NULL;
    rl2GraphicsContextPtr ctx_links = NULL;
    rl2GraphicsContextPtr ctx_faces = NULL;
    rl2GraphicsContextPtr ctx_edge_seeds = NULL;
    rl2GraphicsContextPtr ctx_link_seeds = NULL;
    rl2GraphicsContextPtr ctx_face_seeds = NULL;
    rl2CanvasPtr canvas;

    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_sz);

    if (toponet)
      {
	  ctx = rl2_graph_create_context (1024, 1024);
	  ctx_labels = rl2_graph_create_context (1024, 1024);
	  ctx_nodes = rl2_graph_create_context (1024, 1024);
	  ctx_links = rl2_graph_create_context (1024, 1024);
	  ctx_link_seeds = rl2_graph_create_context (1024, 1024);
	  canvas =
	      rl2_create_network_canvas (ctx, ctx_labels, ctx_nodes, ctx_links,
					 ctx_link_seeds);
      }
    if (topogeo)
      {
	  ctx = rl2_graph_create_context (1024, 1024);
	  ctx_labels = rl2_graph_create_context (1024, 1024);
	  ctx_nodes = rl2_graph_create_context (1024, 1024);
	  ctx_edges = rl2_graph_create_context (1024, 1024);
	  ctx_faces = rl2_graph_create_context (1024, 1024);
	  ctx_edge_seeds = rl2_graph_create_context (1024, 1024);
	  ctx_face_seeds = rl2_graph_create_context (1024, 1024);
	  canvas =
	      rl2_create_topology_canvas (ctx, ctx_labels, ctx_nodes, ctx_edges,
					  ctx_faces, ctx_edge_seeds,
					  ctx_face_seeds);
      }

    switch (mode)
      {
      case 0:
	  ret =
	      rl2_map_image_paint_from_vector (sqlite, data, canvas, NULL,
					       coverage, blob, blob_sz, style,
					       NULL);
	  break;
      case 1:
	  ret =
	      rl2_map_image_paint_from_vector_ex (sqlite, data, canvas, NULL,
						  coverage, blob, blob_sz,
						  style, NULL, 1, 1, 1, 1, 1);
	  break;
      case 2:
	  ret =
	      rl2_map_image_paint_from_vector_ex (sqlite, data, canvas, NULL,
						  coverage, blob, blob_sz,
						  style, NULL, 0, 1, 0, 1, 0);
	  break;
      case 3:
	  ret =
	      rl2_map_image_paint_from_vector_ex (sqlite, data, canvas, NULL,
						  coverage, blob, blob_sz,
						  style, NULL, 1, 0, 1, 0, 1);
	  break;
      };
    if (ret != RL2_OK)
      {
	  fprintf (stderr, "C API (base) failure !!! (%s - %s)\n", coverage,
		   (style == NULL) ? "NULL" : style);
	  return 0;
      }

    path =
	sqlite3_mprintf ("./%s_ctx%d_%s_%d.%s", coverage, mode,
			 (style == NULL) ? "NULL" : style, geom->Srid, "png");
    for (i = 0; i < (int) strlen (path); i++)
      {
	  if (path[i] == ' ')
	      path[i] = '_';
      }

    rgb = rl2_graph_get_context_rgb_array (ctx);
    alpha = rl2_graph_get_context_alpha_array (ctx, &half_transparent);
    if (rl2_rgb_alpha_to_png (1024, 1024, rgb, alpha, &png, &png_size, 1.0) !=
	RL2_OK)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    else
      {
	  out = fopen (path, "wb");
	  if (out != NULL)
	    {
		fwrite (png, 1, png_size, out);
		fclose (out);
	    }
	  free (png);
	  retcode = 1;
      }
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);

    if (canvas)
	rl2_destroy_canvas (canvas);
    if (ctx)
	rl2_graph_destroy_context (ctx);
    if (ctx_nodes)
	rl2_graph_destroy_context (ctx_nodes);
    if (ctx_edges)
	rl2_graph_destroy_context (ctx_edges);
    if (ctx_links)
	rl2_graph_destroy_context (ctx_links);
    if (ctx_faces)
	rl2_graph_destroy_context (ctx_faces);
    if (ctx_edge_seeds)
	rl2_graph_destroy_context (ctx_edge_seeds);
    if (ctx_link_seeds)
	rl2_graph_destroy_context (ctx_link_seeds);
    if (ctx_face_seeds)
	rl2_graph_destroy_context (ctx_face_seeds);

    unlink (path);
    if (!retcode)
	fprintf (stderr, "ERROR: unable to export \"%s\"\n", path);
    sqlite3_free (path);
    *extret += retcode;
    return retcode;
}

static int
test_capi (sqlite3 * sqlite, const char *coverage, const char *style,
	   gaiaGeomCollPtr geom, int toponet, int topogeo, const void *data,
	   int *extret)
{
/* testing the C API (not SQL)  */
    if (!test_capi_base
	(sqlite, coverage, style, geom, toponet, topogeo, 0, data, extret))
	return 0;
    if (!test_capi_base
	(sqlite, coverage, style, geom, toponet, topogeo, 1, data, extret))
	return 0;
    if (!test_capi_base
	(sqlite, coverage, style, geom, toponet, topogeo, 2, data, extret))
	return 0;
    if (!test_capi_base
	(sqlite, coverage, style, geom, toponet, topogeo, 3, data, extret))
	return 0;
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
    char *old_SPATIALITE_SECURITY_ENV = NULL;
    gaiaGeomCollPtr g4326 = NULL;
    gaiaGeomCollPtr g32631 = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 ("NE.sqlite", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);

    g4326 = get_extent (4326);
    g32631 = get_extent (32631);
    if (g4326 == NULL || g32631 == NULL)
	return -2;

/* Vector Layers tests */
    ret = -10;
    if (!test_coverage
	(db_handle, "countries", "Countries - simple", g4326, &ret))
	return ret;
    ret = -20;
    if (!test_coverage
	(db_handle, "countries", "Countries - simple", g32631, &ret))
	return ret;
    ret = -30;
    if (!test_coverage
	(db_handle, "countries", "Country Boundaries", g4326, &ret))
	return ret;
    ret = -40;
    if (!test_coverage
	(db_handle, "countries", "Country Boundaries", g32631, &ret))
	return ret;
    ret = -50;
    if (!test_coverage
	(db_handle, "popplaces", "Populated Places - simple", g4326, &ret))
	return ret;
    ret = -60;
    if (!test_coverage
	(db_handle, "popplaces", "Populated Places - with label", g32631, &ret))
	return ret;
    ret = -70;
    if (!test_coverage
	(db_handle, "popplaces", "Populated Places - categorized", g4326, &ret))
	return ret;
    ret = -80;
    if (!test_coverage
	(db_handle, "popplaces", "Populated Places - categorized", g32631,
	 &ret))
	return ret;
    ret = -90;
    if (!test_coverage (db_handle, "railroads", "Railroads", g4326, &ret))
	return ret;
    ret = -100;
    if (!test_coverage
	(db_handle, "railroads", "Railroads - with label", g32631, &ret))
	return ret;
    ret = -110;
    if (!test_map_vector (db_handle, "railnet", "default", g4326, &ret))
	return ret;
    ret = -112;
    if (!test_map_vector (db_handle, "railnet", "toponet", g4326, &ret))
	return ret;
    ret = -114;
    if (!test_map_vector (db_handle, "railnet", "default", g32631, &ret))
	return ret;
    ret = -116;
    if (!test_map_vector (db_handle, "railnet", "toponet", g32631, &ret))
	return ret;
    ret = -120;
    if (!test_map_vector (db_handle, "toporail", "default", g4326, &ret))
	return ret;
    ret = -122;
    if (!test_map_vector (db_handle, "toporail", "topogeo", g4326, &ret))
	return ret;
    ret = -124;
    if (!test_map_vector (db_handle, "toporail", "default", g32631, &ret))
	return ret;
    ret = -126;
    if (!test_map_vector (db_handle, "toporail", "topogeo", g32631, &ret))
	return ret;
    ret = -130;
    if (!test_capi (db_handle, "railnet", NULL, g4326, 1, 0, priv_data, &ret))
	return ret;
    ret = -132;
    if (!test_capi
	(db_handle, "railnet", "toponet", g4326, 1, 0, priv_data, &ret))
	return ret;
    ret = -134;
    if (!test_capi (db_handle, "toporail", NULL, g4326, 0, 1, priv_data, &ret))
	return ret;
    ret = -136;
    if (!test_capi
	(db_handle, "toporail", "topogeo", g4326, 0, 1, priv_data, &ret))
	return ret;

/* destroying both Geometries */
    gaiaFreeGeomColl (g4326);
    gaiaFreeGeomColl (g32631);

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
    return 0;
}
