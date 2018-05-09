/*

 rl2_internal_data -- Private Connection Data

 version 0.1, 2017 July 14

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

The Original Code is the RasterLite2 library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2017
the Initial Developer. All Rights Reserved.

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
#include <time.h>

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#ifdef __ANDROID__		/* Android specific */
#include <cairo-ft.h>
#else /* any other standard platform (Win, Linux, Mac) */
#include <cairo/cairo-ft.h>
#endif /* end Android conditionals */

RL2_DECLARE void *
rl2_alloc_private (void)
{
/* allocating and initializing default private connection data */
    int i;
    FT_Error error;
    FT_Library library;
    struct rl2_private_data *priv_data =
	malloc (sizeof (struct rl2_private_data));
    if (priv_data == NULL)
	return NULL;
    priv_data->max_threads = 1;
    priv_data->tmp_atm_table = NULL;

/* initializing FreeType */
    error = FT_Init_FreeType (&library);
    if (error)
	priv_data->FTlibrary = NULL;
    else
	priv_data->FTlibrary = library;
    priv_data->first_font = NULL;
    priv_data->last_font = NULL;
    priv_data->raster_cache_items = 4;
    priv_data->raster_cache =
	malloc (sizeof (struct rl2_cached_raster) *
		priv_data->raster_cache_items);
    for (i = 0; i < priv_data->raster_cache_items; i++)
      {
	  struct rl2_cached_raster *ptr = priv_data->raster_cache + i;
	  ptr->db_prefix = NULL;
	  ptr->coverage = NULL;
	  ptr->raster = NULL;
      }
    return priv_data;
}

RL2_PRIVATE char *
rl2_init_tmp_atm_table (void *data)
{
    unsigned char rnd[16];
    char uuid[64];
    char *p = uuid;
    int i;
    struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
    if (priv_data->tmp_atm_table != NULL)
	return priv_data->tmp_atm_table;

/* creating an UUID name for temp ATM table */
    sqlite3_randomness (16, rnd);
    for (i = 0; i < 16; i++)
      {
	  if (i == 4 || i == 6 || i == 8 || i == 10)
	      *p++ = '-';
	  sprintf (p, "%02x", rnd[i]);
	  p += 2;
      }
    *p = '\0';
    uuid[14] = '4';
    uuid[19] = '8';
    priv_data->tmp_atm_table = sqlite3_mprintf ("tmp_atm_%s", uuid);
    return priv_data->tmp_atm_table;
}

RL2_DECLARE void
rl2_cleanup_private (const void *ptr)
{
/* destroying private connection data */
    struct rl2_private_tt_font *pF;
    struct rl2_private_tt_font *pFn;
    int i;
    struct rl2_private_data *priv_data = (struct rl2_private_data *) ptr;
    if (priv_data == NULL)
	return;

    if (priv_data->tmp_atm_table != NULL)
	sqlite3_free (priv_data->tmp_atm_table);
/* cleaning the internal Font Cache */
    pF = priv_data->first_font;
    while (pF != NULL)
      {
	  pFn = pF->next;
	  rl2_destroy_private_tt_font (pF);
	  pF = pFn;
      }
    if (priv_data->FTlibrary != NULL)
	FT_Done_FreeType ((FT_Library) (priv_data->FTlibrary));

/* cleaning the internal Raster Cache */
    for (i = 0; i < priv_data->raster_cache_items; i++)
      {
	  struct rl2_cached_raster *ptr = priv_data->raster_cache + i;
	  if (ptr->db_prefix != NULL)
	      free (ptr->db_prefix);
	  if (ptr->coverage != NULL)
	      free (ptr->coverage);
	  if (ptr->raster != NULL)
	      rl2_destroy_raster ((rl2RasterPtr) (ptr->raster));
      }
    free (priv_data->raster_cache);
    free (priv_data);
}

static int
do_match_raster_bbox (rl2PrivRasterPtr raster, double x, double y)
{
/* testing if the Point matches the Tile's own BBOX */
    if (x < raster->minX)
	return 0;
    if (x > raster->maxX)
	return 0;
    if (y < raster->minY)
	return 0;
    if (y > raster->maxY)
	return 0;
    return 1;
}

RL2_PRIVATE int
rl2_find_cached_raster (const void *data, const char *db_prefix,
			const char *coverage, int pyramid_level, double x,
			double y, rl2RasterPtr * raster)
{
/* will return the pointer to a Raster object stored within the internal Cache */
    int i;
    struct rl2_private_data *priv_data = (struct rl2_private_data *) data;

    *raster = NULL;
    if (priv_data == NULL)
	return RL2_ERROR;

    for (i = 0; i < priv_data->raster_cache_items; i++)
      {
	  int ok = 1;
	  struct rl2_cached_raster *ptr = priv_data->raster_cache + i;
	  if (ptr->raster == NULL)
	      continue;
	  if (ptr->db_prefix == NULL && db_prefix == NULL)
	      ;
	  else if (ptr->db_prefix != NULL && db_prefix != NULL)
	    {
		if (strcasecmp (ptr->db_prefix, db_prefix) != 0)
		    ok = 0;
	    }
	  else
	      ok = 0;
	  if (strcasecmp (ptr->coverage, coverage) != 0)
	      ok = 0;
	  if (ptr->pyramid_level != pyramid_level)
	      ok = 0;
	  if (!do_match_raster_bbox (ptr->raster, x, y))
	      ok = 0;
	  if (ok)
	    {
		*raster = (rl2RasterPtr) (ptr->raster);
		ptr->last_used = time (NULL);
		return RL2_OK;
	    }
      }
    return RL2_ERROR;
}

static void
add_raster2cache (const void *data, const char *db_prefix, const char *coverage,
		  int pyramid_level, rl2RasterPtr raster)
{
// inserting a Raster into the Cache
    int i;
    int index = -1;
    int old_index = -1;
    time_t old_time;
    int len;
    struct rl2_cached_raster *ptr;
    struct rl2_private_data *priv_data = (struct rl2_private_data *) data;

    if (priv_data == NULL)
	return;

    for (i = 0; i < priv_data->raster_cache_items; i++)
      {
	  ptr = priv_data->raster_cache + i;
	  if (ptr->raster == NULL)
	    {
		/* found a free Cache slot */
		index = i;
		break;
	    }
	  if (old_index < 0)
	    {
		old_index = i;
		old_time = ptr->last_used;
	    }
	  else
	    {
		if (ptr->last_used < old_time)
		  {
		      old_time = ptr->last_used;
		      old_index = i;
		  }
	    }
      }

    if (index >= 0)
      {
	  /* using a free Cache slot */
      }
    else
      {
	  /* freeing and reusing the oldest Cache slot */
	  index = old_index;
	  ptr = priv_data->raster_cache + index;
	  if (ptr->db_prefix != NULL)
	      free (ptr->db_prefix);
	  ptr->db_prefix = NULL;
	  if (ptr->coverage != NULL)
	      free (ptr->coverage);
	  if (ptr->raster != NULL)
	      rl2_destroy_raster ((rl2RasterPtr) (ptr->raster));
	  ptr->raster = NULL;
      }

    ptr = priv_data->raster_cache + index;
    if (db_prefix == NULL)
	ptr->db_prefix = NULL;
    else
      {
	  len = strlen (db_prefix);
	  ptr->db_prefix = malloc (len + 1);
	  strcpy (ptr->db_prefix, db_prefix);
      }
    len = strlen (coverage);
    ptr->coverage = malloc (len + 1);
    strcpy (ptr->coverage, coverage);
    ptr->pyramid_level = pyramid_level;
    ptr->raster = (rl2PrivRasterPtr) raster;
    ptr->last_used = time (NULL);
}

RL2_PRIVATE int
rl2_load_cached_raster (sqlite3 * handle, const void *data,
			const char *db_prefix, const char *coverage,
			int pyramid_level, double x, double y,
			rl2PalettePtr palette, rl2RasterPtr * raster)
{
/* will load a new Raster object into the internal Cache */
    char *sql;
    char *xdb_prefix;
    char *tiles;
    char *xtiles;
    char *tile_data;
    char *xtile_data;
    char *idx_tiles;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2RasterPtr xraster;

    *raster = NULL;

/* quarying Coverage Tiles */
    if (db_prefix == NULL)
	db_prefix = "main";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    tiles = sqlite3_mprintf ("%s_tiles", coverage);
    xtiles = rl2_double_quoted_sql (tiles);
    sqlite3_free (tiles);
    idx_tiles = sqlite3_mprintf ("DB=%s.%s_tiles", db_prefix, coverage);
    tile_data = sqlite3_mprintf ("%s_tile_data", coverage);
    xtile_data = rl2_double_quoted_sql (tile_data);
    sqlite3_free (tile_data);
    sql = sqlite3_mprintf ("SELECT MbrMinX(t.geometry), MbrMinY(t.geometry), "
			   "MbrMaxX(t.geometry), MbrMaxY(t.geometry), ST_SRID(t.geometry), "
			   "d.tile_data_odd, d.tile_data_even "
			   "FROM \"%s\".\"%s\" AS t "
			   "JOIN \"%s\".\"%s\" AS d ON (t.tile_id = d.tile_id) "
			   "WHERE t.pyramid_level = ? AND t.ROWID IN ( "
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
			   "AND search_frame = MakePoint(?, ?))",
			   xdb_prefix, xtiles, xdb_prefix, xtile_data,
			   idx_tiles);
    free (xdb_prefix);
    free (xtiles);
    free (xtile_data);
    sqlite3_free (idx_tiles);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT raw tile raster SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, pyramid_level);
    sqlite3_bind_double (stmt, 2, x);
    sqlite3_bind_double (stmt, 3, y);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		double minx;
		double miny;
		double maxx;
		double maxy;
		int srid;
		const unsigned char *blob_odd = NULL;
		int blob_odd_sz = 0;
		const unsigned char *blob_even = NULL;
		int blob_even_sz = 0;
		minx = sqlite3_column_double (stmt, 0);
		miny = sqlite3_column_double (stmt, 1);
		maxx = sqlite3_column_double (stmt, 2);
		maxy = sqlite3_column_double (stmt, 3);
		srid = sqlite3_column_int (stmt, 4);
		if (sqlite3_column_type (stmt, 5) == SQLITE_BLOB)
		  {
		      blob_odd = sqlite3_column_blob (stmt, 5);
		      blob_odd_sz = sqlite3_column_bytes (stmt, 5);
		  }
		if (sqlite3_column_type (stmt, 6) == SQLITE_BLOB)
		  {
		      blob_even = sqlite3_column_blob (stmt, 6);
		      blob_even_sz = sqlite3_column_bytes (stmt, 6);
		  }
		xraster = rl2_raster_decode (RL2_SCALE_1,
					     blob_odd,
					     blob_odd_sz,
					     blob_even, blob_even_sz, palette);
		if (xraster == NULL)
		    goto error;

		rl2_raster_georeference_frame (xraster, srid, minx, miny, maxx,
					       maxy);
		add_raster2cache (data, db_prefix, coverage, pyramid_level,
				  xraster);
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    *raster = xraster;
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}
