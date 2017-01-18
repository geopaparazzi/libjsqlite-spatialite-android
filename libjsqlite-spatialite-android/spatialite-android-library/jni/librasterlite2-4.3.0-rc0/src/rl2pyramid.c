/*

 rl2pyramid -- DBMS Pyramid functions

 version 0.1, 2014 July 13

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
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
#include <float.h>

#include <sys/types.h>
#if defined(_WIN32) && !defined(__MINGW32__)
#include <io.h>
#include <direct.h>
#else
#include <dirent.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%I64d\n"
#else
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%lld\n"
#endif

static int
do_insert_pyramid_levels (sqlite3 * handle, int id_level, double res_x,
			  double res_y, sqlite3_stmt * stmt_levl)
{
/* INSERTing the Pyramid levels */
    int ret;
    sqlite3_reset (stmt_levl);
    sqlite3_clear_bindings (stmt_levl);
    sqlite3_bind_int (stmt_levl, 1, id_level);
    sqlite3_bind_double (stmt_levl, 2, res_x);
    sqlite3_bind_double (stmt_levl, 3, res_y);
    sqlite3_bind_double (stmt_levl, 4, res_x * 2.0);
    sqlite3_bind_double (stmt_levl, 5, res_y * 2.0);
    sqlite3_bind_double (stmt_levl, 6, res_x * 4.0);
    sqlite3_bind_double (stmt_levl, 7, res_y * 4.0);
    sqlite3_bind_double (stmt_levl, 8, res_x * 8.0);
    sqlite3_bind_double (stmt_levl, 9, res_y * 8.0);
    ret = sqlite3_step (stmt_levl);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO levels; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    return 1;
  error:
    return 0;
}

static int
do_insert_pyramid_section_levels (sqlite3 * handle, sqlite3_int64 section_id,
				  int id_level, double res_x, double res_y,
				  sqlite3_stmt * stmt_levl)
{
/* INSERTing the Pyramid levels - mixed resolutions Coverage */
    int ret;
    sqlite3_reset (stmt_levl);
    sqlite3_clear_bindings (stmt_levl);
    sqlite3_bind_int64 (stmt_levl, 1, section_id);
    sqlite3_bind_int (stmt_levl, 2, id_level);
    sqlite3_bind_double (stmt_levl, 3, res_x);
    sqlite3_bind_double (stmt_levl, 4, res_y);
    sqlite3_bind_double (stmt_levl, 5, res_x * 2.0);
    sqlite3_bind_double (stmt_levl, 6, res_y * 2.0);
    sqlite3_bind_double (stmt_levl, 7, res_x * 4.0);
    sqlite3_bind_double (stmt_levl, 8, res_y * 4.0);
    sqlite3_bind_double (stmt_levl, 9, res_x * 8.0);
    sqlite3_bind_double (stmt_levl, 10, res_y * 8.0);
    ret = sqlite3_step (stmt_levl);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO section_levels; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    return 1;
  error:
    return 0;
}

static int
do_insert_pyramid_tile (sqlite3 * handle, unsigned char *blob_odd,
			int blob_odd_sz, unsigned char *blob_even,
			int blob_even_sz, int id_level,
			sqlite3_int64 section_id, int srid, double minx,
			double miny, double maxx, double maxy,
			sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_data)
{
/* INSERTing a Pyramid tile */
    int ret;
    sqlite3_int64 tile_id;

    sqlite3_reset (stmt_tils);
    sqlite3_clear_bindings (stmt_tils);
    sqlite3_bind_int (stmt_tils, 1, id_level);
    if (section_id < 0)
	sqlite3_bind_null (stmt_tils, 2);
    else
	sqlite3_bind_int64 (stmt_tils, 2, section_id);
    sqlite3_bind_double (stmt_tils, 3, minx);
    sqlite3_bind_double (stmt_tils, 4, miny);
    sqlite3_bind_double (stmt_tils, 5, maxx);
    sqlite3_bind_double (stmt_tils, 6, maxy);
    sqlite3_bind_int (stmt_tils, 7, srid);
    ret = sqlite3_step (stmt_tils);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO tiles; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    tile_id = sqlite3_last_insert_rowid (handle);
    /* INSERTing tile data */
    sqlite3_reset (stmt_data);
    sqlite3_clear_bindings (stmt_data);
    sqlite3_bind_int64 (stmt_data, 1, tile_id);
    sqlite3_bind_blob (stmt_data, 2, blob_odd, blob_odd_sz, free);
    if (blob_even == NULL)
	sqlite3_bind_null (stmt_data, 3);
    else
	sqlite3_bind_blob (stmt_data, 3, blob_even, blob_even_sz, free);
    ret = sqlite3_step (stmt_data);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "INSERT INTO tile_data; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    return 1;
  error:
    return 0;
}

static int
delete_section_pyramid (sqlite3 * handle, const char *coverage,
			sqlite3_int64 section_id)
{
/* attempting to delete a section pyramid */
    char *sql;
    char *table;
    char *xtable;
    char sect_id[1024];
    int ret;
    char *err_msg = NULL;

#if defined(_WIN32) && !defined(__MINGW32__)
    sprintf (sect_id, "%I64d", section_id);
#else
    sprintf (sect_id, "%lld", section_id);
#endif

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("DELETE FROM \"%s\" WHERE pyramid_level > 0 AND section_id = %s",
	 xtable, sect_id);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE FROM \"%s_tiles\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
check_section_pyramid (sqlite3 * handle, const char *coverage,
		       sqlite3_int64 section_id)
{
/* checking if a section's pyramid already exists */
    char *sql;
    char *table;
    char *xtable;
    char sect_id[1024];
    sqlite3_stmt *stmt = NULL;
    int ret;
    int count = 0;

#if defined(_WIN32) && !defined(__MINGW32__)
    sprintf (sect_id, "%I64d", section_id);
#else
    sprintf (sect_id, "%lld", section_id);
#endif

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT Count(*) FROM \"%s\" "
			 "WHERE section_id = %s AND pyramid_level > 0", xtable,
			 sect_id);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 1;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT pyramid_exists; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		count = 0;
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 0)
	return 1;
    return 0;
}

static int
get_section_infos (sqlite3 * handle, const char *coverage,
		   sqlite3_int64 section_id, unsigned int *sect_width,
		   unsigned int *sect_height, double *minx, double *miny,
		   double *maxx, double *maxy, rl2PalettePtr * palette,
		   rl2PixelPtr * no_data)
{
/* retrieving the Section most relevant infos */
    char *table;
    char *xtable;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int ok = 0;

/* Section infos */
    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT width, height, MbrMinX(geometry), "
			 "MbrMinY(geometry), MbrMaxX(geometry), MbrMaxY(geometry) "
			 "FROM \"%s\" WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, section_id);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		*sect_width = sqlite3_column_int (stmt, 0);
		*sect_height = sqlite3_column_int (stmt, 1);
		*minx = sqlite3_column_double (stmt, 2);
		*miny = sqlite3_column_double (stmt, 3);
		*maxx = sqlite3_column_double (stmt, 4);
		*maxy = sqlite3_column_double (stmt, 5);
		ok = 1;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_info; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (!ok)
	goto error;

/* Coverage's palette and no-data */
    sql = sqlite3_mprintf ("SELECT palette, nodata_pixel FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      *palette = rl2_deserialize_dbms_palette (blob, blob_sz);
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 1);
		      int blob_sz = sqlite3_column_bytes (stmt, 1);
		      *no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_info; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
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

static SectionPyramidTileInPtr
alloc_section_pyramid_tile (sqlite3_int64 tile_id, double minx, double miny,
			    double maxx, double maxy)
{
/* allocating a Section Pyramid Tile object */
    SectionPyramidTileInPtr tile = malloc (sizeof (SectionPyramidTileIn));
    if (tile == NULL)
	return NULL;
    tile->tile_id = tile_id;
    tile->cx = minx + ((maxx - minx) / 2.0);
    tile->cy = miny + ((maxy - miny) / 2.0);
    tile->next = NULL;
    return tile;
}

static SectionPyramidPtr
alloc_sect_pyramid (sqlite3_int64 sect_id, unsigned int sect_width,
		    unsigned int sect_height, unsigned char sample_type,
		    unsigned char pixel_type, unsigned char num_samples,
		    unsigned char compression, int quality, int srid,
		    double res_x, double res_y, double tile_width,
		    double tile_height, double minx, double miny, double maxx,
		    double maxy, int scale)
{
/* allocating a Section Pyramid object */
    double ext_x = maxx - minx;
    double ext_y = maxy - miny;
    SectionPyramidPtr pyr = malloc (sizeof (SectionPyramid));
    if (pyr == NULL)
	return NULL;
    pyr->section_id = sect_id;
    pyr->scale = scale;
    pyr->full_width = sect_width;
    pyr->full_height = sect_height;
    pyr->sample_type = sample_type;
    pyr->pixel_type = pixel_type;
    pyr->num_samples = num_samples;
    pyr->compression = compression;
    pyr->quality = quality;
    pyr->srid = srid;
    pyr->res_x = res_x;
    pyr->res_y = res_y;
    pyr->scaled_width = ext_x / res_x;
    pyr->scaled_height = ext_y / res_y;
    pyr->tile_width = tile_width;
    pyr->tile_height = tile_height;
    pyr->minx = minx;
    pyr->miny = miny;
    pyr->maxx = maxx;
    pyr->maxy = maxy;
    pyr->first_in = NULL;
    pyr->last_in = NULL;
    pyr->first_out = NULL;
    pyr->last_out = NULL;
    return pyr;
}

static void
delete_sect_pyramid (SectionPyramidPtr pyr)
{
/* memory cleanup - destroying a Section Pyramid object */
    SectionPyramidTileInPtr tile_in;
    SectionPyramidTileInPtr tile_in_n;
    SectionPyramidTileOutPtr tile_out;
    SectionPyramidTileOutPtr tile_out_n;
    if (pyr == NULL)
	return;
    tile_out = pyr->first_out;
    while (tile_out != NULL)
      {
	  SectionPyramidTileRefPtr ref;
	  SectionPyramidTileRefPtr ref_n;
	  tile_out_n = tile_out->next;
	  ref = tile_out->first;
	  while (ref != NULL)
	    {
		ref_n = ref->next;
		free (ref);
		ref = ref_n;
	    }
	  free (tile_out);
	  tile_out = tile_out_n;
      }
    tile_in = pyr->first_in;
    while (tile_in != NULL)
      {
	  tile_in_n = tile_in->next;
	  free (tile_in);
	  tile_in = tile_in_n;
      }
    free (pyr);
}

static int
insert_tile_into_section_pyramid (SectionPyramidPtr pyr, sqlite3_int64 tile_id,
				  double minx, double miny, double maxx,
				  double maxy)
{
/* inserting a base tile into the Pyramid level */
    SectionPyramidTileInPtr tile;
    if (pyr == NULL)
	return 0;
    tile = alloc_section_pyramid_tile (tile_id, minx, miny, maxx, maxy);
    if (tile == NULL)
	return 0;
    if (pyr->first_in == NULL)
	pyr->first_in = tile;
    if (pyr->last_in != NULL)
	pyr->last_in->next = tile;
    pyr->last_in = tile;
    return 1;
}

static SectionPyramidTileOutPtr
add_pyramid_out_tile (SectionPyramidPtr pyr, unsigned int row,
		      unsigned int col, double minx, double miny, double maxx,
		      double maxy)
{
/* inserting a Parent tile (output) */
    SectionPyramidTileOutPtr tile;
    if (pyr == NULL)
	return NULL;
    tile = malloc (sizeof (SectionPyramidTileOut));
    if (tile == NULL)
	return NULL;
    tile->row = row;
    tile->col = col;
    tile->minx = minx;
    tile->miny = miny;
    tile->maxx = maxx;
    tile->maxy = maxy;
    tile->first = NULL;
    tile->last = NULL;
    tile->next = NULL;
    if (pyr->first_out == NULL)
	pyr->first_out = tile;
    if (pyr->last_out != NULL)
	pyr->last_out->next = tile;
    pyr->last_out = tile;
    return tile;
}

static void
add_pyramid_sub_tile (SectionPyramidTileOutPtr parent,
		      SectionPyramidTileInPtr child)
{
/* inserting a Child tile (input) */
    SectionPyramidTileRefPtr ref;
    if (parent == NULL)
	return;
    ref = malloc (sizeof (SectionPyramidTileRef));
    if (ref == NULL)
	return;
    ref->child = child;
    ref->next = NULL;
    if (parent->first == NULL)
	parent->first = ref;
    if (parent->last != NULL)
	parent->last->next = ref;
    parent->last = ref;
}

static void
set_pyramid_tile_destination (SectionPyramidPtr pyr, double minx, double miny,
			      double maxx, double maxy, unsigned int row,
			      unsigned int col)
{
/* aggregating lower level tiles */
    int first = 1;
    SectionPyramidTileOutPtr out = NULL;
    SectionPyramidTileInPtr tile = pyr->first_in;
    while (tile != NULL)
      {
	  if (tile->cx > minx && tile->cx < maxx && tile->cy > miny
	      && tile->cy < maxy)
	    {
		if (first)
		  {
		      out =
			  add_pyramid_out_tile (pyr, row, col, minx, miny, maxx,
						maxy);
		      first = 0;
		  }
		if (out != NULL)
		    add_pyramid_sub_tile (out, tile);
	    }
	  tile = tile->next;
      }
}

static unsigned char *
load_tile_base (sqlite3_stmt * stmt, sqlite3_int64 tile_id,
		rl2PalettePtr palette, rl2PixelPtr no_data)
{
/* attempting to read a lower-level tile */
    int ret;
    const unsigned char *blob_odd = NULL;
    int blob_odd_sz = 0;
    const unsigned char *blob_even = NULL;
    int blob_even_sz = 0;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr plt = NULL;
    unsigned char *rgba_tile = NULL;
    int rgba_sz;
    rl2PixelPtr nd;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, tile_id);
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
		      blob_odd = sqlite3_column_blob (stmt, 0);
		      blob_odd_sz = sqlite3_column_bytes (stmt, 0);
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_BLOB)
		  {
		      blob_even = sqlite3_column_blob (stmt, 1);
		      blob_even_sz = sqlite3_column_bytes (stmt, 1);
		  }
		plt = rl2_clone_palette (palette);
		raster =
		    rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz,
				       blob_even, blob_even_sz, plt);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      return NULL;
		  }
		nd = rl2_clone_pixel (no_data);
		rl2_set_raster_no_data (raster, nd);
		if (rl2_raster_data_to_RGBA (raster, &rgba_tile, &rgba_sz) !=
		    RL2_OK)
		    rgba_tile = NULL;
		rl2_destroy_raster (raster);
		break;
	    }
	  else
	      return NULL;
      }
    return rgba_tile;
}

static rl2RasterPtr
load_tile_base_generic (sqlite3_stmt * stmt, sqlite3_int64 tile_id)
{
/* attempting to read a lower-level tile */
    int ret;
    const unsigned char *blob_odd = NULL;
    int blob_odd_sz = 0;
    const unsigned char *blob_even = NULL;
    int blob_even_sz = 0;
    rl2RasterPtr raster = NULL;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, tile_id);
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
		      blob_odd = sqlite3_column_blob (stmt, 0);
		      blob_odd_sz = sqlite3_column_bytes (stmt, 0);
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_BLOB)
		  {
		      blob_even = sqlite3_column_blob (stmt, 1);
		      blob_even_sz = sqlite3_column_bytes (stmt, 1);
		  }
		raster =
		    rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz,
				       blob_even, blob_even_sz, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      return NULL;
		  }
		return raster;
	    }
      }
    return NULL;
}

static double
rescale_pixel_int8 (const char *buf_in, unsigned int tileWidth,
		    unsigned int tileHeight, int x, int y, char nd)
{
/* rescaling a DataGrid pixel (8x8) - INT8 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const char *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const char *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (char) (sum / (double) valid);
}

static void
rescale_grid_int8 (char *buf_out, unsigned int tileWidth,
		   unsigned int tileHeight, const char *buf_in, int x, int y,
		   int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - int 8 bit */
    unsigned int row;
    unsigned int col;
    char nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_INT8 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->int8;
	    }
      }

    for (row = 0; row < (unsigned) tic_y; row++)
      {
	  unsigned int yy = row + y;
	  char *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < (unsigned int) tic_x; col++)
	    {
		unsigned int xx = col + x;
		char *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_int8 (buf_in, tileWidth, tileHeight, col * 8,
					row * 8, nd);
	    }
      }
}

static double
rescale_pixel_uint8 (const unsigned char *buf_in, unsigned int tileWidth,
		     unsigned int tileHeight, int x, int y, unsigned char nd)
{
/* rescaling a DataGrid pixel (8x8) - UINT8 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned char *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned char *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned char) (sum / (double) valid);
}

static void
rescale_grid_uint8 (unsigned char *buf_out, unsigned int tileWidth,
		    unsigned int tileHeight, const unsigned char *buf_in,
		    int x, int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - unsigned int 8 bit */
    unsigned int row;
    unsigned int col;
    unsigned char nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_UINT8 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->uint8;
	    }
      }

    for (row = 0; row < (unsigned int) tic_y; row++)
      {
	  unsigned int yy = row + y;
	  unsigned char *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < (unsigned int) tic_x; col++)
	    {
		unsigned int xx = col + x;
		unsigned char *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_uint8 (buf_in, tileWidth, tileHeight, col * 8,
					 row * 8, nd);
	    }
      }
}

static double
rescale_pixel_int16 (const short *buf_in, unsigned int tileWidth,
		     unsigned int tileHeight, int x, int y, short nd)
{
/* rescaling a DataGrid pixel (8x8) - INT32 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const short *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const short *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (short) (sum / (double) valid);
}

static void
rescale_grid_int16 (short *buf_out, unsigned int tileWidth,
		    unsigned int tileHeight, const short *buf_in, int x,
		    int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - int 16 bit */
    unsigned int row;
    unsigned int col;
    short nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_INT16 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->int16;
	    }
      }

    for (row = 0; row < (unsigned int) tic_y; row++)
      {
	  unsigned int yy = row + y;
	  short *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < (unsigned int) tic_x; col++)
	    {
		unsigned int xx = col + x;
		short *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_int16 (buf_in, tileWidth, tileHeight, col * 8,
					 row * 8, nd);
	    }
      }
}

static double
rescale_pixel_uint16 (const unsigned short *buf_in, unsigned int tileWidth,
		      unsigned int tileHeight, int x, int y, unsigned short nd)
{
/* rescaling a DataGrid pixel (8x8) - UINT16 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned short *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned short *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned short) (sum / (double) valid);
}

static void
rescale_grid_uint16 (unsigned short *buf_out, unsigned int tileWidth,
		     unsigned int tileHeight, const unsigned short *buf_in,
		     int x, int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - unsigned int 16 bit */
    unsigned int row;
    unsigned int col;
    unsigned short nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_UINT16 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->uint16;
	    }
      }

    for (row = 0; row < (unsigned int) tic_y; row++)
      {
	  unsigned int yy = row + y;
	  unsigned short *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < (unsigned int) tic_x; col++)
	    {
		unsigned int xx = col + x;
		unsigned short *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_uint16 (buf_in, tileWidth, tileHeight,
					  col * 8, row * 8, nd);
	    }
      }
}

static double
rescale_pixel_int32 (const int *buf_in, unsigned int tileWidth,
		     unsigned int tileHeight, int x, int y, int nd)
{
/* rescaling a DataGrid pixel (8x8) - INT32 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const int *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const int *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (int) (sum / (double) valid);
}

static void
rescale_grid_int32 (int *buf_out, unsigned int tileWidth,
		    unsigned int tileHeight, const int *buf_in, int x, int y,
		    int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - int 32 bit */
    unsigned int row;
    unsigned int col;
    int nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_INT32 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->int32;
	    }
      }

    for (row = 0; row < (unsigned int) tic_y; row++)
      {
	  unsigned int yy = row + y;
	  int *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < (unsigned int) tic_x; col++)
	    {
		unsigned int xx = col + x;
		int *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_int32 (buf_in, tileWidth, tileHeight, col * 8,
					 row * 8, nd);
	    }
      }
}

static double
rescale_pixel_uint32 (const unsigned int *buf_in, unsigned int tileWidth,
		      unsigned int tileHeight, int x, int y, unsigned int nd)
{
/* rescaling a DataGrid pixel (8x8) - UINT32 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned int *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned int *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned int) (sum / (double) valid);
}

static void
rescale_grid_uint32 (unsigned int *buf_out, unsigned int tileWidth,
		     unsigned int tileHeight, const unsigned int *buf_in,
		     int x, int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - unsigned int 32 bit */
    unsigned int row;
    unsigned int col;
    unsigned int nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_UINT32 && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->uint32;
	    }
      }

    for (row = 0; row < (unsigned int) tic_y; row++)
      {
	  unsigned int yy = row + y;
	  unsigned int *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < (unsigned int) tic_x; col++)
	    {
		unsigned int xx = col + x;
		unsigned int *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_uint32 (buf_in, tileWidth, tileHeight,
					  col * 8, row * 8, nd);
	    }
      }
}

static double
rescale_pixel_float (const float *buf_in, unsigned int tileWidth,
		     unsigned int tileHeight, int x, int y, float nd)
{
/* rescaling a DataGrid pixel (8x8) - Float */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const float *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const float *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (float) (sum / (double) valid);
}

static void
rescale_grid_float (float *buf_out, unsigned int tileWidth,
		    unsigned int tileHeight, const float *buf_in, int x,
		    int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - Float */
    unsigned int row;
    unsigned int col;
    float nd = 0.0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_FLOAT && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->float32;
	    }
      }

    for (row = 0; row < (unsigned int) tic_y; row++)
      {
	  unsigned int yy = row + y;
	  float *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < (unsigned int) tic_x; col++)
	    {
		unsigned int xx = col + x;
		float *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_float (buf_in, tileWidth, tileHeight, col * 8,
					 row * 8, nd);
	    }
      }
}

static double
rescale_pixel_double (const double *buf_in, unsigned int tileWidth,
		      unsigned int tileHeight, int x, int y, double nd)
{
/* rescaling a DataGrid pixel (8x8) - Double */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const double *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < 8; col++)
	    {
		const double *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + x;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return sum / (double) valid;
}

static void
rescale_grid_double (double *buf_out, unsigned int tileWidth,
		     unsigned int tileHeight, const double *buf_in, int x,
		     int y, int tic_x, int tic_y, rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile - Double */
    unsigned int row;
    unsigned int col;
    double nd = 0.0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    if (pxl != NULL)
      {
	  /* retrieving the NO-DATA value */
	  if (pxl->sampleType == RL2_SAMPLE_DOUBLE && pxl->nBands == 1)
	    {
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd = sample->float64;
	    }
      }

    for (row = 0; row < (unsigned int) tic_y; row++)
      {
	  unsigned int yy = row + y;
	  double *p_out_base = buf_out + (yy * tileWidth);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < (unsigned int) tic_x; col++)
	    {
		unsigned int xx = col + x;
		double *p_out = p_out_base + xx;
		if (xx >= tileWidth)
		    break;
		*p_out =
		    rescale_pixel_double (buf_in, tileWidth, tileHeight,
					  col * 8, row * 8, nd);
	    }
      }
}

static void
rescale_grid (void *buf_out, unsigned int tileWidth,
	      unsigned int tileHeight, const void *buf_in,
	      unsigned char sample_type, int x, int y, int tic_x, int tic_y,
	      rl2PixelPtr no_data)
{
/* rescaling a DataGrid tile */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  rescale_grid_int8 ((char *) buf_out, tileWidth, tileHeight,
			     (const char *) buf_in, x, y, tic_x, tic_y,
			     no_data);
	  break;
      case RL2_SAMPLE_UINT8:
	  rescale_grid_uint8 ((unsigned char *) buf_out, tileWidth, tileHeight,
			      (const unsigned char *) buf_in, x, y, tic_x,
			      tic_y, no_data);
	  break;
      case RL2_SAMPLE_INT16:
	  rescale_grid_int16 ((short *) buf_out, tileWidth, tileHeight,
			      (const short *) buf_in, x, y, tic_x, tic_y,
			      no_data);
	  break;
      case RL2_SAMPLE_UINT16:
	  rescale_grid_uint16 ((unsigned short *) buf_out, tileWidth,
			       tileHeight, (const unsigned short *) buf_in, x,
			       y, tic_x, tic_y, no_data);
	  break;
      case RL2_SAMPLE_INT32:
	  rescale_grid_int32 ((int *) buf_out, tileWidth, tileHeight,
			      (const int *) buf_in, x, y, tic_x, tic_y,
			      no_data);
	  break;
      case RL2_SAMPLE_UINT32:
	  rescale_grid_uint32 ((unsigned int *) buf_out, tileWidth, tileHeight,
			       (const unsigned int *) buf_in, x, y, tic_x,
			       tic_y, no_data);
	  break;
      case RL2_SAMPLE_FLOAT:
	  rescale_grid_float ((float *) buf_out, tileWidth, tileHeight,
			      (const float *) buf_in, x, y, tic_x, tic_y,
			      no_data);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  rescale_grid_double ((double *) buf_out, tileWidth, tileHeight,
			       (const double *) buf_in, x, y, tic_x, tic_y,
			       no_data);
	  break;
      };
}

static int
update_sect_pyramid_grid (sqlite3 * handle, sqlite3_stmt * stmt_rd,
			  sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_data,
			  SectionPyramid * pyr, unsigned int tileWidth,
			  unsigned int tileHeight, int id_level,
			  rl2PixelPtr no_data, unsigned char sample_type)
{
/* creating and inserting Pyramid tiles */
    unsigned char *buf_out = NULL;
    unsigned char *mask = NULL;
    SectionPyramidTileOutPtr tile_out;
    SectionPyramidTileRefPtr tile_in;
    unsigned int x;
    unsigned int y;
    unsigned int row;
    unsigned int col;
    int tic_x;
    int tic_y;
    double pos_y;
    double pos_x;
    double geo_x;
    double geo_y;
    rl2PixelPtr nd = NULL;
    rl2RasterPtr raster_out = NULL;
    rl2RasterPtr raster_in = NULL;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    int pixel_sz = 1;
    int out_sz;
    int mask_sz = 0;
    unsigned char compression;

    if (pyr == NULL)
	goto error;
    tic_x = tileWidth / pyr->scale;
    tic_y = tileHeight / pyr->scale;
    geo_x = (double) tic_x *pyr->res_x;
    geo_y = (double) tic_y *pyr->res_y;
    compression = pyr->compression;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pixel_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pixel_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pixel_sz = 8;
	  break;
      };
    out_sz = tileWidth * tileHeight * pixel_sz;

    tile_out = pyr->first_out;
    while (tile_out != NULL)
      {
	  rl2PrivRasterPtr rst;
	  /* allocating the output buffer */
	  buf_out = malloc (out_sz);
	  if (buf_out == NULL)
	      goto error;
	  rl2_prime_void_tile (buf_out, tileWidth, tileHeight, sample_type, 1,
			       no_data);

	  if (tile_out->col + tileWidth > pyr->scaled_width
	      || tile_out->row + tileHeight > pyr->scaled_height)
	    {
		/* allocating and initializing a transparency mask */
		unsigned char *p;
		mask_sz = tileWidth * tileHeight;
		mask = malloc (mask_sz);
		if (mask == NULL)
		    goto error;
		p = mask;
		for (row = 0; row < tileHeight; row++)
		  {
		      unsigned int x_row = tile_out->row + row;
		      for (col = 0; col < tileWidth; col++)
			{
			    unsigned int x_col = tile_out->col + col;
			    if (x_row >= pyr->scaled_height
				|| x_col >= pyr->scaled_width)
			      {
				  /* masking any portion of the tile exceeding the scaled section size */
				  *p++ = 0;
			      }
			    else
				*p++ = 1;
			}
		  }
	    }
	  else
	    {
		mask = NULL;
		mask_sz = 0;
	    }

	  /* creating the output (rescaled) tile */
	  tile_in = tile_out->first;
	  while (tile_in != NULL)
	    {
		/* loading and rescaling the base tiles */
		raster_in =
		    load_tile_base_generic (stmt_rd, tile_in->child->tile_id);
		if (raster_in == NULL)
		    goto error;
		pos_y = tile_out->maxy;
		x = 0;
		y = 0;
		for (row = 0; row < tileHeight; row += tic_y)
		  {
		      pos_x = tile_out->minx;
		      for (col = 0; col < tileWidth; col += tic_x)
			{
			    if (tile_in->child->cy < pos_y
				&& tile_in->child->cy > (pos_y - geo_y)
				&& tile_in->child->cx > pos_x
				&& tile_in->child->cx < (pos_x + geo_x))
			      {
				  x = col;
				  y = row;
				  break;
			      }
			    pos_x += geo_x;
			}
		      pos_y -= geo_y;
		  }
		rst = (rl2PrivRasterPtr) raster_in;
		rescale_grid (buf_out, tileWidth, tileHeight, rst->rasterBuffer,
			      sample_type, x, y, tic_x, tic_y, no_data);
		rl2_destroy_raster (raster_in);
		raster_in = NULL;
		tile_in = tile_in->next;
	    }

	  raster_out = NULL;
	  raster_out =
	      rl2_create_raster (tileWidth, tileHeight, sample_type,
				 RL2_PIXEL_DATAGRID, 1, buf_out,
				 out_sz, NULL, mask, mask_sz, nd);
	  buf_out = NULL;
	  mask = NULL;
	  if (raster_out == NULL)
	    {
		fprintf (stderr, "ERROR: unable to create a Pyramid Tile\n");
		goto error;
	    }
	  if (rl2_raster_encode
	      (raster_out, compression, &blob_odd, &blob_odd_sz,
	       &blob_even, &blob_even_sz, 100, 1) != RL2_OK)
	    {
		fprintf (stderr, "ERROR: unable to encode a Pyramid tile\n");
		goto error;
	    }
	  rl2_destroy_raster (raster_out);
	  raster_out = NULL;

	  /* INSERTing the tile */
	  if (!do_insert_pyramid_tile
	      (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz, id_level,
	       pyr->section_id, pyr->srid, tile_out->minx, tile_out->miny,
	       tile_out->maxx, tile_out->maxy, stmt_tils, stmt_data))
	      goto error;

	  tile_out = tile_out->next;
      }

    return 1;

  error:
    if (raster_in != NULL)
	rl2_destroy_raster (raster_in);
    if (raster_out != NULL)
	rl2_destroy_raster (raster_out);
    if (buf_out != NULL)
	free (buf_out);
    if (mask != NULL)
	free (mask);
    return 0;
}

static double
rescale_mb_pixel_uint8 (const unsigned char *buf_in, unsigned int tileWidth,
			unsigned int tileHeight, unsigned int x, unsigned int y,
			unsigned char nd, unsigned char nb,
			unsigned char num_bands)
{
/* rescaling a MultiBand pixel sample (8x8) - UINT8 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned char *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth * num_bands);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned char *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + (xx * num_bands) + nb;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned char) (sum / (double) valid);
}

static void
rescale_multiband_uint8 (unsigned char *buf_out, unsigned int tileWidth,
			 unsigned int tileHeight, const unsigned char *buf_in,
			 unsigned int x, unsigned int y, unsigned int tic_x,
			 unsigned int tic_y, unsigned char num_bands,
			 rl2PixelPtr no_data)
{
/* rescaling a MultiBand tile - unsigned int 8 bit */
    unsigned int row;
    unsigned int col;
    unsigned char nb;
    unsigned char nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    for (row = 0; row < tic_y; row++)
      {
	  unsigned int yy = row + y;
	  unsigned char *p_out_base = buf_out + (yy * tileWidth * num_bands);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned int xx = col + x;
		unsigned char *p_out = p_out_base + (xx * num_bands);
		if (xx >= tileWidth)
		    break;
		for (nb = 0; nb < num_bands; nb++)
		  {
		      if (pxl != NULL)
			{
			    if (pxl->sampleType == RL2_SAMPLE_UINT8
				&& pxl->nBands == num_bands)
			      {
				  rl2PrivSamplePtr sample = pxl->Samples + nb;
				  nd = sample->uint8;
			      }
			}
		      *(p_out + nb) =
			  rescale_mb_pixel_uint8 (buf_in, tileWidth, tileHeight,
						  col * 8, row * 8, nd, nb,
						  num_bands);
		  }
	    }
      }
}

static double
rescale_mb_pixel_uint16 (const unsigned short *buf_in, unsigned int tileWidth,
			 unsigned int tileHeight, unsigned int x,
			 unsigned int y, unsigned short nd, unsigned char nb,
			 unsigned char num_bands)
{
/* rescaling a MultiBand pixel sample (8x8) - UINT16 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;

    for (row = 0; row < 8; row++)
      {
	  const unsigned short *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth * num_bands);
	  for (col = 0; col < 8; col++)
	    {
		const unsigned short *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + (xx * num_bands) + nb;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return nd;
    return (unsigned short) (sum / (double) valid);
}

static void
rescale_multiband_uint16 (unsigned short *buf_out, unsigned int tileWidth,
			  unsigned int tileHeight,
			  const unsigned short *buf_in, unsigned int x,
			  unsigned int y, unsigned int tic_x,
			  unsigned int tic_y, unsigned char num_bands,
			  rl2PixelPtr no_data)
{
/* rescaling a MultiBand tile - unsigned int 16 bit */
    unsigned int row;
    unsigned int col;
    unsigned char nb;
    unsigned short nd = 0;
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;

    for (row = 0; row < tic_y; row++)
      {
	  unsigned int yy = row + y;
	  unsigned short *p_out_base = buf_out + (yy * tileWidth * num_bands);
	  if (yy >= tileHeight)
	      break;
	  for (col = 0; col < tic_x; col++)
	    {
		unsigned int xx = col + x;
		unsigned short *p_out = p_out_base + (xx * num_bands);
		if (xx >= tileWidth)
		    break;
		for (nb = 0; nb < num_bands; nb++)
		  {
		      if (pxl != NULL)
			{
			    if (pxl->sampleType == RL2_SAMPLE_UINT16
				&& pxl->nBands == num_bands)
			      {
				  rl2PrivSamplePtr sample = pxl->Samples + nb;
				  nd = sample->uint16;
			      }
			}
		      *(p_out + nb) =
			  rescale_mb_pixel_uint16 (buf_in, tileWidth,
						   tileHeight, col * 8, row * 8,
						   nd, nb, num_bands);
		  }
	    }
      }
}

static void
rescale_multiband (void *buf_out, unsigned int tileWidth,
		   unsigned int tileHeight, const void *buf_in,
		   unsigned char sample_type, unsigned char num_bands,
		   unsigned int x, unsigned int y, unsigned int tic_x,
		   unsigned int tic_y, rl2PixelPtr no_data)
{
/* rescaling a Multiband tile */
    switch (sample_type)
      {
      case RL2_SAMPLE_UINT8:
	  rescale_multiband_uint8 ((unsigned char *) buf_out, tileWidth,
				   tileHeight, (const unsigned char *) buf_in,
				   x, y, tic_x, tic_y, num_bands, no_data);
	  break;
      case RL2_SAMPLE_UINT16:
	  rescale_multiband_uint16 ((unsigned short *) buf_out, tileWidth,
				    tileHeight, (const unsigned short *) buf_in,
				    x, y, tic_x, tic_y, num_bands, no_data);
	  break;
      };
}

static int
update_sect_pyramid_multiband (sqlite3 * handle, sqlite3_stmt * stmt_rd,
			       sqlite3_stmt * stmt_tils,
			       sqlite3_stmt * stmt_data, SectionPyramid * pyr,
			       unsigned int tileWidth,
			       unsigned int tileHeight, int id_level,
			       rl2PixelPtr no_data, unsigned char sample_type,
			       unsigned char num_bands)
{
/* creating and inserting Pyramid tiles */
    unsigned char *buf_out = NULL;
    unsigned char *mask = NULL;
    SectionPyramidTileOutPtr tile_out;
    SectionPyramidTileRefPtr tile_in;
    unsigned int x;
    unsigned int y;
    unsigned int row;
    unsigned int col;
    unsigned int tic_x;
    unsigned int tic_y;
    double pos_y;
    double pos_x;
    double geo_x;
    double geo_y;
    rl2PixelPtr nd = NULL;
    rl2RasterPtr raster_out = NULL;
    rl2RasterPtr raster_in = NULL;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    int pixel_sz = 1;
    int out_sz;
    int mask_sz = 0;
    unsigned char compression;

    if (pyr == NULL)
	goto error;
    tic_x = tileWidth / pyr->scale;
    tic_y = tileHeight / pyr->scale;
    geo_x = (double) tic_x *pyr->res_x;
    geo_y = (double) tic_y *pyr->res_y;
    compression = pyr->compression;

    switch (sample_type)
      {
      case RL2_SAMPLE_UINT16:
	  pixel_sz = 2;
	  break;
      };
    out_sz = tileWidth * tileHeight * pixel_sz * num_bands;

    tile_out = pyr->first_out;
    while (tile_out != NULL)
      {
	  rl2PrivRasterPtr rst;
	  /* allocating the output buffer */
	  buf_out = malloc (out_sz);
	  if (buf_out == NULL)
	      goto error;
	  rl2_prime_void_tile (buf_out, tileWidth, tileHeight, sample_type,
			       num_bands, no_data);

	  if (tile_out->col + tileWidth > pyr->scaled_width
	      || tile_out->row + tileHeight > pyr->scaled_height)
	    {
		/* allocating and initializing a transparency mask */
		unsigned char *p;
		mask_sz = tileWidth * tileHeight;
		mask = malloc (mask_sz);
		if (mask == NULL)
		    goto error;
		p = mask;
		for (row = 0; row < tileHeight; row++)
		  {
		      unsigned int x_row = tile_out->row + row;
		      for (col = 0; col < tileWidth; col++)
			{
			    unsigned int x_col = tile_out->col + col;
			    if (x_row >= pyr->scaled_height
				|| x_col >= pyr->scaled_width)
			      {
				  /* masking any portion of the tile exceeding the scaled section size */
				  *p++ = 0;
			      }
			    else
				*p++ = 1;
			}
		  }
	    }
	  else
	    {
		mask = NULL;
		mask_sz = 0;
	    }

	  /* creating the output (rescaled) tile */
	  tile_in = tile_out->first;
	  while (tile_in != NULL)
	    {
		/* loading and rescaling the base tiles */
		raster_in =
		    load_tile_base_generic (stmt_rd, tile_in->child->tile_id);
		if (raster_in == NULL)
		    goto error;
		pos_y = tile_out->maxy;
		x = 0;
		y = 0;
		for (row = 0; row < tileHeight; row += tic_y)
		  {
		      pos_x = tile_out->minx;
		      for (col = 0; col < tileWidth; col += tic_x)
			{
			    if (tile_in->child->cy < pos_y
				&& tile_in->child->cy > (pos_y - geo_y)
				&& tile_in->child->cx > pos_x
				&& tile_in->child->cx < (pos_x + geo_x))
			      {
				  x = col;
				  y = row;
				  break;
			      }
			    pos_x += geo_x;
			}
		      pos_y -= geo_y;
		  }
		rst = (rl2PrivRasterPtr) raster_in;
		rescale_multiband (buf_out, tileWidth, tileHeight,
				   rst->rasterBuffer, sample_type, num_bands, x,
				   y, tic_x, tic_y, no_data);
		rl2_destroy_raster (raster_in);
		raster_in = NULL;
		tile_in = tile_in->next;
	    }

	  raster_out = NULL;
	  raster_out =
	      rl2_create_raster (tileWidth, tileHeight, sample_type,
				 RL2_PIXEL_MULTIBAND, num_bands, buf_out,
				 out_sz, NULL, mask, mask_sz, nd);
	  buf_out = NULL;
	  mask = NULL;
	  if (raster_out == NULL)
	    {
		fprintf (stderr, "ERROR: unable to create a Pyramid Tile\n");
		goto error;
	    }
	  if (rl2_raster_encode
	      (raster_out, compression, &blob_odd, &blob_odd_sz,
	       &blob_even, &blob_even_sz, 100, 1) != RL2_OK)
	    {
		fprintf (stderr, "ERROR: unable to encode a Pyramid tile\n");
		goto error;
	    }
	  rl2_destroy_raster (raster_out);
	  raster_out = NULL;

	  /* INSERTing the tile */
	  if (!do_insert_pyramid_tile
	      (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz, id_level,
	       pyr->section_id, pyr->srid, tile_out->minx, tile_out->miny,
	       tile_out->maxx, tile_out->maxy, stmt_tils, stmt_data))
	      goto error;

	  tile_out = tile_out->next;
      }

    return 1;

  error:
    if (raster_in != NULL)
	rl2_destroy_raster (raster_in);
    if (raster_out != NULL)
	rl2_destroy_raster (raster_out);
    if (buf_out != NULL)
	free (buf_out);
    if (mask != NULL)
	free (mask);
    return 0;
}

static int
update_sect_pyramid (sqlite3 * handle, sqlite3_stmt * stmt_rd,
		     sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_data,
		     SectionPyramid * pyr, unsigned int tileWidth,
		     unsigned int tileHeight, int id_level,
		     rl2PalettePtr palette, rl2PixelPtr no_data)
{
/* creating and inserting Pyramid tiles */
    unsigned char *buf_in;
    SectionPyramidTileOutPtr tile_out;
    SectionPyramidTileRefPtr tile_in;
    rl2GraphicsBitmapPtr base_tile;
    rl2GraphicsContextPtr ctx = NULL;
    unsigned int x;
    unsigned int y;
    unsigned int row;
    unsigned int col;
    unsigned int tic_x;
    unsigned int tic_y;
    double pos_y;
    double pos_x;
    double geo_x;
    double geo_y;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    rl2PixelPtr nd = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    unsigned char *p;
    unsigned char compression;
    int hald_transparent;

    if (pyr == NULL)
	goto error;
    tic_x = tileWidth / pyr->scale;
    tic_y = tileHeight / pyr->scale;
    geo_x = (double) tic_x *pyr->res_x;
    geo_y = (double) tic_y *pyr->res_y;
    compression = pyr->compression;

    tile_out = pyr->first_out;
    while (tile_out != NULL)
      {
	  /* creating the output (rescaled) tile */
	  ctx = rl2_graph_create_context (tileWidth, tileHeight);
	  if (ctx == NULL)
	      goto error;
	  tile_in = tile_out->first;
	  while (tile_in != NULL)
	    {
		/* loading and rescaling the base tiles */
		buf_in =
		    load_tile_base (stmt_rd, tile_in->child->tile_id, palette,
				    no_data);
		if (buf_in == NULL)
		    goto error;
		base_tile =
		    rl2_graph_create_bitmap (buf_in, tileWidth, tileHeight);
		if (base_tile == NULL)
		  {
		      free (buf_in);
		      goto error;
		  }
		pos_y = tile_out->maxy;
		x = 0;
		y = 0;
		for (row = 0; row < tileHeight; row += tic_y)
		  {
		      pos_x = tile_out->minx;
		      for (col = 0; col < tileWidth; col += tic_x)
			{
			    if (tile_in->child->cy < pos_y
				&& tile_in->child->cy > (pos_y - geo_y)
				&& tile_in->child->cx > pos_x
				&& tile_in->child->cx < (pos_x + geo_x))
			      {
				  x = col;
				  y = row;
				  break;
			      }
			    pos_x += geo_x;
			}
		      pos_y -= geo_y;
		  }
		rl2_graph_draw_rescaled_bitmap (ctx, base_tile,
						1.0 / pyr->scale,
						1.0 / pyr->scale, x, y);
		rl2_graph_destroy_bitmap (base_tile);
		tile_in = tile_in->next;
	    }

	  rgb = rl2_graph_get_context_rgb_array (ctx);
	  if (rgb == NULL)
	      goto error;
	  alpha = rl2_graph_get_context_alpha_array (ctx, &hald_transparent);
	  if (alpha == NULL)
	      goto error;
	  p = alpha;
	  for (row = 0; row < tileHeight; row++)
	    {
		unsigned int x_row = tile_out->row + row;
		for (col = 0; col < tileWidth; col++)
		  {
		      unsigned int x_col = tile_out->col + col;
		      if (x_row >= pyr->scaled_height
			  || x_col >= pyr->scaled_width)
			{
			    /* masking any portion of the tile exceeding the scaled section size */
			    *p++ = 0;
			}
		      else
			{
			    if (*p == 0)
				p++;
			    else
				*p++ = 1;
			}
		  }
	    }

	  raster = NULL;
	  if (pyr->pixel_type == RL2_PIXEL_GRAYSCALE
	      || pyr->pixel_type == RL2_PIXEL_MONOCHROME)
	    {
		/* Grayscale Pyramid */
		unsigned char *p_in;
		unsigned char *p_out;
		unsigned char *gray = malloc (tileWidth * tileHeight);
		if (gray == NULL)
		    goto error;
		p_in = rgb;
		p_out = gray;
		for (row = 0; row < tileHeight; row++)
		  {
		      for (col = 0; col < tileWidth; col++)
			{
			    *p_out++ = *p_in++;
			    p_in += 2;
			}
		  }
		free (rgb);
		if (pyr->pixel_type == RL2_PIXEL_MONOCHROME)
		  {
		      if (no_data == NULL)
			  nd = NULL;
		      else
			{
			    /* converting the NO-DATA pixel */
			    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) no_data;
			    rl2PrivSamplePtr sample = pxl->Samples + 0;
			    nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
						   RL2_PIXEL_GRAYSCALE, 1);
			    if (sample->uint8 == 0)
				rl2_set_pixel_sample_uint8 (nd,
							    RL2_GRAYSCALE_BAND,
							    255);
			    else
				rl2_set_pixel_sample_uint8 (nd,
							    RL2_GRAYSCALE_BAND,
							    0);
			}
		      compression = RL2_COMPRESSION_PNG;
		  }
		else
		    nd = rl2_clone_pixel (no_data);
		raster =
		    rl2_create_raster (tileWidth, tileHeight, RL2_SAMPLE_UINT8,
				       RL2_PIXEL_GRAYSCALE, 1, gray,
				       tileWidth * tileHeight, NULL, alpha,
				       tileWidth * tileHeight, nd);
	    }
	  else if (pyr->pixel_type == RL2_PIXEL_RGB)
	    {
		/* RGB Pyramid */
		nd = rl2_clone_pixel (no_data);
		raster =
		    rl2_create_raster (tileWidth, tileHeight, RL2_SAMPLE_UINT8,
				       RL2_PIXEL_RGB, 3, rgb,
				       tileWidth * tileHeight * 3, NULL, alpha,
				       tileWidth * tileHeight, nd);
	    }
	  if (raster == NULL)
	    {
		fprintf (stderr, "ERROR: unable to create a Pyramid Tile\n");
		goto error;
	    }
	  if (rl2_raster_encode
	      (raster, compression, &blob_odd, &blob_odd_sz, &blob_even,
	       &blob_even_sz, 80, 1) != RL2_OK)
	    {
		fprintf (stderr, "ERROR: unable to encode a Pyramid tile\n");
		goto error;
	    }
	  rl2_destroy_raster (raster);
	  raster = NULL;
	  rl2_graph_destroy_context (ctx);
	  ctx = NULL;

	  /* INSERTing the tile */
	  if (!do_insert_pyramid_tile
	      (handle, blob_odd, blob_odd_sz, blob_even, blob_even_sz, id_level,
	       pyr->section_id, pyr->srid, tile_out->minx, tile_out->miny,
	       tile_out->maxx, tile_out->maxy, stmt_tils, stmt_data))
	      goto error;

	  tile_out = tile_out->next;
      }

    return 1;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (ctx != NULL)
	rl2_graph_destroy_context (ctx);
    return 0;
}

static int
rescale_monolithic_rgba (int id_level,
			 unsigned int tileWidth, unsigned int tileHeight,
			 int factor, double res_x, double res_y, double minx,
			 double miny, double maxx, double maxy,
			 unsigned char *buffer, int buf_size,
			 unsigned char *mask, int *mask_size,
			 rl2PalettePtr palette, rl2PixelPtr no_data,
			 sqlite3_stmt * stmt_geo, sqlite3_stmt * stmt_data)
{
/* rescaling a monolithic RGBA tile */
    rl2GraphicsContextPtr ctx = NULL;
    rl2GraphicsBitmapPtr base_tile = NULL;
    unsigned char *rgba = NULL;
    unsigned int x;
    unsigned int y;
    int ret;
    double shift_x;
    double shift_y;
    double scale_x;
    double scale_y;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    int valid_mask = 0;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    int half_transparent;

/* creating a graphics context */
    ctx = rl2_graph_create_context (tileWidth, tileHeight);
    if (ctx == NULL)
	goto error;
/* binding the BBOX to be queried */
    sqlite3_reset (stmt_geo);
    sqlite3_clear_bindings (stmt_geo);
    sqlite3_bind_int (stmt_geo, 1, id_level);
    sqlite3_bind_double (stmt_geo, 2, minx);
    sqlite3_bind_double (stmt_geo, 3, miny);
    sqlite3_bind_double (stmt_geo, 4, maxx);
    sqlite3_bind_double (stmt_geo, 5, maxy);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_geo);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_geo, 0);
		double tile_x = sqlite3_column_double (stmt_geo, 1);
		double tile_y = sqlite3_column_double (stmt_geo, 2);

		rgba = load_tile_base (stmt_data, tile_id, palette, no_data);
		if (rgba == NULL)
		    goto error;
		base_tile =
		    rl2_graph_create_bitmap (rgba, tileWidth, tileHeight);
		if (base_tile == NULL)
		  {
		      free (rgba);
		      goto error;
		  }
		shift_x = tile_x - minx;
		shift_y = maxy - tile_y;
		scale_x = 1.0 / (double) factor;
		scale_y = 1.0 / (double) factor;
		x = (int) (shift_x / res_x);
		y = (int) (shift_y / res_y);
		rl2_graph_draw_rescaled_bitmap (ctx, base_tile,
						scale_x, scale_y, x, y);
		rl2_graph_destroy_bitmap (base_tile);
	    }
      }

    rgb = rl2_graph_get_context_rgb_array (ctx);
    if (rgb == NULL)
	goto error;
    alpha = rl2_graph_get_context_alpha_array (ctx, &half_transparent);
    if (alpha == NULL)
	goto error;
    rl2_graph_destroy_context (ctx);
    if (buf_size == (int) (tileWidth * tileHeight))
      {
	  /* Grayscale */
	  p_in = rgb;
	  p_msk = alpha;
	  p_out = buffer;
	  for (y = 0; y < tileHeight; y++)
	    {
		for (x = 0; x < tileWidth; x++)
		  {
		      if (*p_msk++ < 128)
			{
			    /* skipping a transparent pixel */
			    p_in += 3;
			    p_out++;
			}
		      else
			{
			    /* copying an opaque pixel */
			    *p_out++ = *p_in++;
			    p_in += 2;
			}
		  }
	    }
      }
    else
      {
	  /* RGB */
	  p_in = rgb;
	  p_msk = alpha;
	  p_out = buffer;
	  for (y = 0; y < tileHeight; y++)
	    {
		for (x = 0; x < tileWidth; x++)
		  {
		      if (*p_msk++ < 128)
			{
			    /* skipping a transparent pixel */
			    p_in += 3;
			    p_out += 3;
			}
		      else
			{
			    /* copying an opaque pixel */
			    *p_out++ = *p_in++;
			    *p_out++ = *p_in++;
			    *p_out++ = *p_in++;
			}
		  }
	    }
      }
    free (rgb);
    p_in = alpha;
    p_out = mask;
    for (y = 0; y < tileHeight; y++)
      {
	  for (x = 0; x < tileWidth; x++)
	    {
		if (*p_in++ < 128)
		  {
		      *p_out++ = 0;
		      valid_mask = 1;
		  }
		else
		    *p_out++ = 1;
	    }
      }
    free (alpha);
    if (!valid_mask)
      {
	  free (mask);
	  *mask_size = 0;
      }

    return 1;
  error:
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    if (ctx != NULL)
	rl2_graph_destroy_context (ctx);
    return 0;
}

#define floor2(exp) ((long) exp)

static rl2RasterPtr
create_124_rescaled_raster (const unsigned char *rgba, unsigned char pixel_type,
			    unsigned int tileWidth, unsigned int tileHeight,
			    int scale)
{
/* creating a rescaled raster (1,2 or 4 bit pyramids) 
/
/ this function builds an high quality rescaled sub-image by applying pixel interpolation
/
/ this code is widely inspired by the original GD gdImageCopyResampled() function
*/
    rl2RasterPtr raster;
    unsigned int x;
    unsigned int y;
    double sy1;
    double sy2;
    double sx1;
    double sx2;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    unsigned char *rgb = NULL;
    int rgb_sz;
    unsigned char *mask = NULL;
    int mask_sz;
    unsigned char num_bands;
    const unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int out_width = tileWidth / scale;
    unsigned int out_height = tileHeight / scale;

    mask_sz = out_width * out_height;
    if (pixel_type == RL2_PIXEL_RGB)
      {
	  num_bands = 3;
	  rgb_sz = out_width * out_height * 3;
	  rgb = malloc (rgb_sz);
	  if (rgb == NULL)
	      return NULL;
	  mask = malloc (mask_sz);
	  if (mask == NULL)
	    {
		free (rgb);
		return NULL;
	    }
      }
    else
      {
	  num_bands = 1;
	  rgb_sz = out_width * out_height;
	  rgb = malloc (rgb_sz);
	  if (rgb == NULL)
	      return NULL;
	  mask = malloc (mask_sz);
	  if (mask == NULL)
	    {
		free (rgb);
		return NULL;
	    }
      }
    memset (mask, 0, mask_sz);

    for (y = 0; y < out_height; y++)
      {
	  sy1 = ((double) y) * (double) tileHeight / (double) out_height;
	  sy2 = ((double) (y + 1)) * (double) tileHeight / (double) out_height;
	  for (x = 0; x < out_width; x++)
	    {
		double sx;
		double sy;
		double spixels = 0;
		double red = 0.0;
		double green = 0.0;
		double blue = 0.0;
		double alpha = 0.0;
		sx1 = ((double) x) * (double) tileWidth / (double) out_width;
		sx2 =
		    ((double) (x + 1)) * (double) tileWidth /
		    (double) out_width;
		sy = sy1;
		do
		  {
		      double yportion;
		      if (floor2 (sy) == floor2 (sy1))
			{
			    yportion = 1.0 - (sy - floor2 (sy));
			    if (yportion > sy2 - sy1)
			      {
				  yportion = sy2 - sy1;
			      }
			    sy = floor2 (sy);
			}
		      else if (sy == floor2 (sy2))
			{
			    yportion = sy2 - floor2 (sy2);
			}
		      else
			{
			    yportion = 1.0;
			}
		      sx = sx1;
		      do
			{
			    double xportion;
			    double pcontribution;
			    if (floor2 (sx) == floor2 (sx1))
			      {
				  xportion = 1.0 - (sx - floor2 (sx));
				  if (xportion > sx2 - sx1)
				    {
					xportion = sx2 - sx1;
				    }
				  sx = floor2 (sx);
			      }
			    else if (sx == floor2 (sx2))
			      {
				  xportion = sx2 - floor2 (sx2);
			      }
			    else
			      {
				  xportion = 1.0;
			      }
			    pcontribution = xportion * yportion;
			    /* retrieving the origin pixel */
			    p_in = rgba + ((unsigned int) sy * tileWidth * 4);
			    p_in += (unsigned int) sx *4;
			    r = *p_in++;
			    g = *p_in++;
			    b = *p_in++;
			    a = *p_in++;

			    red += r * pcontribution;
			    green += g * pcontribution;
			    blue += b * pcontribution;
			    alpha += a * pcontribution;
			    spixels += xportion * yportion;
			    sx += 1.0;
			}
		      while (sx < sx2);
		      sy += 1.0;
		  }
		while (sy < sy2);
		if (spixels != 0.0)
		  {
		      red /= spixels;
		      green /= spixels;
		      blue /= spixels;
		  }
		if (red > 255.0)
		    red = 255.0;
		if (green > 255.0)
		    green = 255.0;
		if (blue > 255.0)
		    blue = 255.0;
		if (alpha < 192.0)
		  {
		      /* skipping almost transparent pixels */
		      continue;
		  }
		/* setting the destination pixel */
		if (pixel_type == RL2_PIXEL_RGB)
		    p_out = rgb + (y * out_width * 3);
		else
		    p_out = rgb + (y * out_width);
		p_msk = mask + (y * out_width);
		if (pixel_type == RL2_PIXEL_RGB)
		  {
		      p_out += x * 3;
		      *p_out++ = (unsigned char) red;
		      *p_out++ = (unsigned char) green;
		      *p_out = (unsigned char) blue;
		      p_msk += x;
		      *p_msk = 1;
		  }
		else
		  {
		      if (red <= 224.0)
			{
			    p_out += x;
			    *p_out = (unsigned char) red;
			    p_msk += x;
			    *p_msk = 1;
			}
		  }
	    }
      }

    raster =
	rl2_create_raster (out_width, out_height, RL2_SAMPLE_UINT8, pixel_type,
			   num_bands, rgb, rgb_sz, NULL, mask, mask_sz, NULL);
    return raster;
}

static void
copy_124_rescaled (rl2RasterPtr raster_out, rl2RasterPtr raster_in,
		   unsigned int base_x, unsigned int base_y)
{
/* copying pixels from rescaled to destination tile buffer */
    rl2PrivRasterPtr rst_in = (rl2PrivRasterPtr) raster_in;
    rl2PrivRasterPtr rst_out = (rl2PrivRasterPtr) raster_out;
    unsigned int x;
    unsigned int y;
    int dx;
    int dy;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk_in;
    unsigned char *p_msk_out;

    if (rst_in->sampleType == RL2_SAMPLE_UINT8
	&& rst_out->sampleType == RL2_SAMPLE_UINT8
	&& rst_in->pixelType == RL2_PIXEL_GRAYSCALE
	&& rst_out->pixelType == RL2_PIXEL_GRAYSCALE && rst_in->nBands == 1
	&& rst_out->nBands == 1 && rst_in->maskBuffer != NULL
	&& rst_out->maskBuffer != NULL)
      {
	  /* Grayscale */
	  p_in = rst_in->rasterBuffer;
	  p_msk_in = rst_in->maskBuffer;
	  for (y = 0; y < rst_in->height; y++)
	    {
		dy = base_y + y;
		if (dy < 0 || dy >= (int) (rst_out->height))
		  {
		      p_in += rst_in->width;
		      p_msk_in += rst_in->width;
		      continue;
		  }
		for (x = 0; x < rst_in->width; x++)
		  {
		      dx = base_x + x;
		      if (dx < 0 || dx >= (int) (rst_out->width))
			{
			    p_in++;
			    p_msk_in++;
			    continue;
			}
		      p_out =
			  rst_out->rasterBuffer + (dy * rst_out->width) + dx;
		      p_msk_out =
			  rst_out->maskBuffer + (dy * rst_out->width) + dx;
		      if (*p_msk_in++ == 0)
			  p_in++;
		      else
			{
			    *p_out++ = *p_in++;
			    *p_msk_out++ = 1;
			}
		  }
	    }
      }
    if (rst_in->sampleType == RL2_SAMPLE_UINT8
	&& rst_out->sampleType == RL2_SAMPLE_UINT8
	&& rst_in->pixelType == RL2_PIXEL_RGB
	&& rst_out->pixelType == RL2_PIXEL_RGB && rst_in->nBands == 3
	&& rst_out->nBands == 3 && rst_in->maskBuffer != NULL
	&& rst_out->maskBuffer != NULL)
      {
	  /* RGB */
	  p_in = rst_in->rasterBuffer;
	  p_msk_in = rst_in->maskBuffer;
	  for (y = 0; y < rst_in->height; y++)
	    {
		dy = base_y + y;
		if (dy < 0 || dy >= (int) (rst_out->height))
		  {
		      p_in += rst_in->width * 3;
		      p_msk_in += rst_in->width;
		      continue;
		  }
		for (x = 0; x < rst_in->width; x++)
		  {
		      dx = base_x + x;
		      if (dx < 0 || dx >= (int) (rst_out->width))
			{
			    p_in += 3;
			    p_msk_in++;
			    continue;
			}
		      p_out =
			  rst_out->rasterBuffer + (dy * rst_out->width * 3) +
			  (dx * 3);
		      p_msk_out =
			  rst_out->maskBuffer + (dy * rst_out->width) + dx;
		      if (*p_msk_in++ == 0)
			  p_in += 3;
		      else
			{
			    *p_out++ = *p_in++;
			    *p_out++ = *p_in++;
			    *p_out++ = *p_in++;
			    *p_msk_out++ = 1;
			}
		  }
	    }
      }
}

static int
rescale_monolithic_124 (int id_level,
			unsigned int tileWidth, unsigned int tileHeight,
			int factor, unsigned char pixel_type, double res_x,
			double res_y, double minx, double miny, double maxx,
			double maxy, unsigned char *buffer, int buf_size,
			unsigned char *mask, int *mask_size,
			rl2PalettePtr palette, rl2PixelPtr no_data,
			sqlite3_stmt * stmt_geo, sqlite3_stmt * stmt_data)
{
/* rescaling a monolithic 1,2 or 4 bit tile */
    rl2RasterPtr raster = NULL;
    rl2RasterPtr base_tile = NULL;
    rl2PrivRasterPtr rst;
    unsigned char *rgba = NULL;
    unsigned int x;
    unsigned int y;
    int ret;
    double shift_x;
    double shift_y;
    int valid_mask = 0;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char out_pixel_type;
    unsigned char out_num_bands;
    rl2PixelPtr nd = NULL;

/* creating the output buffers */
    if (pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  out_pixel_type = RL2_PIXEL_GRAYSCALE;
	  out_num_bands = 1;
	  p_out = buffer;
	  for (y = 0; y < tileHeight; y++)
	    {
		/* priming the background color */
		for (x = 0; x < tileWidth; x++)
		    *p_out++ = 255;
	    }
	  nd = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
	  rl2_set_pixel_sample_uint8 (nd, RL2_GRAYSCALE_BAND, 255);
      }
    else
      {
	  out_pixel_type = RL2_PIXEL_RGB;
	  out_num_bands = 3;
	  p_out = buffer;
	  for (y = 0; y < tileHeight; y++)
	    {
		/* priming the background color */
		for (x = 0; x < tileWidth; x++)
		  {
		      *p_out++ = 255;
		      *p_out++ = 255;
		      *p_out++ = 255;
		  }
	    }
	  nd = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
	  rl2_set_pixel_sample_uint8 (nd, RL2_RED_BAND, 255);
	  rl2_set_pixel_sample_uint8 (nd, RL2_GREEN_BAND, 255);
	  rl2_set_pixel_sample_uint8 (nd, RL2_BLUE_BAND, 255);
      }
    p_out = mask;
    for (y = 0; y < tileHeight; y++)
      {
	  /* priming full transparency */
	  for (x = 0; x < tileWidth; x++)
	      *p_out++ = 0;
      }
/* creating the output raster */
    raster =
	rl2_create_raster (tileWidth, tileHeight, RL2_SAMPLE_UINT8,
			   out_pixel_type, out_num_bands, buffer, buf_size,
			   NULL, mask, *mask_size, nd);
    if (raster == NULL)
	goto error;

/* binding the BBOX to be queried */
    sqlite3_reset (stmt_geo);
    sqlite3_clear_bindings (stmt_geo);
    sqlite3_bind_int (stmt_geo, 1, id_level);
    sqlite3_bind_double (stmt_geo, 2, minx);
    sqlite3_bind_double (stmt_geo, 3, miny);
    sqlite3_bind_double (stmt_geo, 4, maxx);
    sqlite3_bind_double (stmt_geo, 5, maxy);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_geo);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_geo, 0);
		double tile_x = sqlite3_column_double (stmt_geo, 1);
		double tile_y = sqlite3_column_double (stmt_geo, 2);

		rgba = load_tile_base (stmt_data, tile_id, palette, no_data);
		if (rgba == NULL)
		    goto error;
		base_tile =
		    create_124_rescaled_raster (rgba, out_pixel_type, tileWidth,
						tileHeight, factor);
		free (rgba);
		if (base_tile == NULL)
		    goto error;
		shift_x = tile_x - minx;
		shift_y = maxy - tile_y;
		x = (int) (shift_x / res_x);
		y = (int) (shift_y / res_y);
		copy_124_rescaled (raster, base_tile, x, y);
		rl2_destroy_raster (base_tile);
	    }
      }

/* releasing buffers ownership */
    rst = (rl2PrivRasterPtr) raster;
    rst->rasterBuffer = NULL;
    rst->maskBuffer = NULL;
    rl2_destroy_raster (raster);

    p_in = mask;
    for (y = 0; y < tileHeight; y++)
      {
	  for (x = 0; x < tileWidth; x++)
	    {
		if (*p_in++ == 0)
		    valid_mask = 1;
	    }
      }
    if (!valid_mask)
      {
	  free (mask);
	  *mask_size = 0;
      }

    return 1;
  error:
    if (raster != NULL)
      {
	  /* releasing buffers ownership */
	  rst = (rl2PrivRasterPtr) raster;
	  rst->rasterBuffer = NULL;
	  rst->maskBuffer = NULL;
	  rl2_destroy_raster (raster);
      }
    return 0;
}

static void
copy_multiband_rescaled (rl2RasterPtr raster_out, rl2RasterPtr raster_in,
			 unsigned int base_x, unsigned int base_y)
{
/* copying pixels from rescaled to destination tile buffer */
    rl2PrivRasterPtr rst_in = (rl2PrivRasterPtr) raster_in;
    rl2PrivRasterPtr rst_out = (rl2PrivRasterPtr) raster_out;
    unsigned int x;
    unsigned int y;
    int dx;
    int dy;
    unsigned char *p_in_u8 = NULL;
    unsigned char *p_out_u8 = NULL;
    unsigned short *p_in_u16 = NULL;
    unsigned short *p_out_u16 = NULL;
    unsigned char *p_msk_in;
    unsigned char *p_msk_out;
    int mismatch = 0;
    int ib;

    if (rst_in->sampleType != rst_out->sampleType)
	mismatch = 1;
    if (rst_in->pixelType != RL2_PIXEL_MULTIBAND
	|| rst_out->pixelType != RL2_PIXEL_MULTIBAND)
	mismatch = 1;
    if (rst_in->nBands != rst_out->nBands)
	mismatch = 1;
    if (rst_in->maskBuffer == NULL || rst_out->maskBuffer == NULL)
	mismatch = 1;
    if (mismatch)
      {
	  fprintf (stderr,
		   "ERROR: Copy MultiBand Rescaled mismatching in/out\n");
	  return;
      }

    switch (rst_in->sampleType)
      {
      case RL2_SAMPLE_UINT8:
	  p_in_u8 = (unsigned char *) (rst_in->rasterBuffer);
	  break;
      case RL2_SAMPLE_UINT16:
	  p_in_u16 = (unsigned short *) (rst_in->rasterBuffer);
	  break;
      };
    p_msk_in = rst_in->maskBuffer;
    for (y = 0; y < rst_in->height; y++)
      {
	  dy = base_y + y;
	  if (dy < 0 || dy >= (int) (rst_out->height))
	    {
		switch (rst_in->sampleType)
		  {
		  case RL2_SAMPLE_UINT8:
		      p_in_u8 += rst_in->width * rst_in->nBands;
		      break;
		  case RL2_SAMPLE_UINT16:
		      p_in_u16 += rst_in->width * rst_in->nBands;
		      break;
		  };
		p_msk_in += rst_in->width;
		continue;
	    }
	  for (x = 0; x < rst_in->width; x++)
	    {
		dx = base_x + x;
		if (dx < 0 || dx >= (int) (rst_out->width))
		  {
		      switch (rst_in->sampleType)
			{
			case RL2_SAMPLE_UINT8:
			    p_in_u8 += rst_in->nBands;
			    break;
			case RL2_SAMPLE_UINT16:
			    p_in_u16 += rst_in->nBands;
			    break;
			};
		      p_msk_in++;
		      continue;
		  }
		switch (rst_out->sampleType)
		  {
		  case RL2_SAMPLE_UINT8:
		      p_out_u8 = (unsigned char *) (rst_out->rasterBuffer);
		      p_out_u8 +=
			  (dy * rst_out->width * rst_out->nBands) +
			  (dx * rst_out->nBands);
		      break;
		  case RL2_SAMPLE_UINT16:
		      p_out_u16 = (unsigned short *) (rst_out->rasterBuffer);
		      p_out_u16 +=
			  (dy * rst_out->width * rst_out->nBands) +
			  (dx * rst_out->nBands);
		      break;
		  };
		p_msk_out = rst_out->maskBuffer + (dy * rst_out->width) + dx;
		if (*p_msk_in++ == 0)
		  {
		      switch (rst_in->sampleType)
			{
			case RL2_SAMPLE_UINT8:
			    p_in_u8 += rst_out->nBands;
			    break;
			case RL2_SAMPLE_UINT16:
			    p_in_u16 += rst_out->nBands;
			    break;
			};
		  }
		else
		  {
		      for (ib = 0; ib < rst_out->nBands; ib++)
			{
			    switch (rst_out->sampleType)
			      {
			      case RL2_SAMPLE_UINT8:
				  *p_out_u8++ = *p_in_u8++;
				  break;
			      case RL2_SAMPLE_UINT16:
				  *p_out_u16++ = *p_in_u16++;
				  break;
			      };
			}
		      *p_msk_out++ = 1;
		  }
	    }
      }
}

static int
is_mb_nodata_u8 (const unsigned char *pixel, unsigned char num_bands,
		 rl2PixelPtr no_data)
{
/* testing for NO-DATA pixel */
    int is_valid = 0;
    rl2PrivPixelPtr nd = (rl2PrivPixelPtr) no_data;
    unsigned char ib;
    const unsigned char *p_in = pixel;
    if (nd != NULL)
      {
	  if (nd->nBands == num_bands && nd->sampleType == RL2_SAMPLE_UINT8
	      && nd->pixelType == RL2_PIXEL_MULTIBAND)
	      is_valid = 1;
      }
    if (!is_valid)
	return 0;
    for (ib = 0; ib < num_bands; ib++)
      {
	  if (*p_in++ != (nd->Samples + ib)->uint8)
	      return 0;
      }
    return 1;
}

static void
rescale_multiband_u8 (unsigned int tileWidth, unsigned int tileHeight,
		      unsigned char num_bands, unsigned int out_width,
		      unsigned int out_height, unsigned int factor,
		      unsigned char *buf_in, const unsigned char *mask_in,
		      unsigned char *buf_out, unsigned char *mask,
		      unsigned int x, unsigned int y, unsigned int ox,
		      unsigned int oy, rl2PixelPtr no_data)
{
/* rescaling a MultiBand (monolithic)- UINT8 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    unsigned char ib;
    double *sum;
    unsigned char *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy * num_bands) + (ox * num_bands);
    p_msk = mask + (out_width * oy) + ox;
    sum = malloc (sizeof (double) * num_bands);
    for (ib = 0; ib < num_bands; ib++)
	*(sum + ib) = 0.0;

    for (row = 0; row < factor; row++)
      {
	  const unsigned char *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth * num_bands);
	  for (col = 0; col < factor; col++)
	    {
		const unsigned char *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + (xx * num_bands);
		if (mask_in != NULL)
		  {
		      /* checking the transparency mask */
		      const unsigned char *p_msk_in =
			  mask_in + (yy * tileWidth) + xx;
		      if (*p_msk_in == 0)
			{
			    nodata++;
			    continue;
			}
		  }
		if (is_mb_nodata_u8 (p_in, num_bands, no_data))
		    nodata++;
		else
		  {
		      valid++;
		      for (ib = 0; ib < num_bands; ib++)
			  *(sum + ib) += *p_in++;
		  }
	    }
      }

    if (nodata >= valid)
      {
	  free (sum);
	  return;
      }
    for (ib = 0; ib < num_bands; ib++)
	*p_out++ = (unsigned char) (*(sum + ib) / (double) valid);
    free (sum);
    *p_msk = 1;
}

static int
is_mb_nodata_u16 (const unsigned short *pixel, unsigned char num_bands,
		  rl2PixelPtr no_data)
{
/* testing for NO-DATA pixel */
    int is_valid = 0;
    rl2PrivPixelPtr nd = (rl2PrivPixelPtr) no_data;
    unsigned char ib;
    const unsigned short *p_in = pixel;
    if (nd != NULL)
      {
	  if (nd->nBands == num_bands && nd->sampleType == RL2_SAMPLE_UINT16
	      && nd->pixelType == RL2_PIXEL_MULTIBAND)
	      is_valid = 1;
      }
    if (!is_valid)
	return 0;
    for (ib = 0; ib < num_bands; ib++)
      {
	  if (*p_in++ != (nd->Samples + ib)->uint16)
	      return 0;
      }
    return 1;
}

static void
rescale_multiband_u16 (unsigned int tileWidth, unsigned int tileHeight,
		       unsigned char num_bands, unsigned int out_width,
		       unsigned int out_height, unsigned int factor,
		       unsigned short *buf_in, const unsigned char *mask_in,
		       unsigned short *buf_out, unsigned char *mask,
		       unsigned int x, unsigned int y, unsigned int ox,
		       unsigned int oy, rl2PixelPtr no_data)
{
/* rescaling a MultiBand (monolithic)- UINT16 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    unsigned char ib;
    double *sum;
    unsigned short *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy * num_bands) + (ox * num_bands);
    p_msk = mask + (out_width * oy) + ox;
    sum = malloc (sizeof (double) * num_bands);
    for (ib = 0; ib < num_bands; ib++)
	*(sum + ib) = 0.0;

    for (row = 0; row < factor; row++)
      {
	  const unsigned short *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth * num_bands);
	  for (col = 0; col < factor; col++)
	    {
		const unsigned short *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		if (mask_in != NULL)
		  {
		      /* checking the transparency mask */
		      const unsigned char *p_msk_in =
			  mask_in + (yy * tileWidth) + xx;
		      if (*p_msk_in == 0)
			{
			    nodata++;
			    continue;
			}
		  }
		p_in = p_in_base + (xx * num_bands);
		if (is_mb_nodata_u16 (p_in, num_bands, no_data))
		    nodata++;
		else
		  {
		      valid++;
		      for (ib = 0; ib < num_bands; ib++)
			  *(sum + ib) += *p_in++;
		  }
	    }
      }

    if (nodata >= valid)
      {
	  free (sum);
	  return;
      }
    for (ib = 0; ib < num_bands; ib++)
	*p_out++ = (unsigned char) (*(sum + ib) / (double) valid);
    free (sum);
    *p_msk = 1;
}

static void
mb_prime_nodata_u8 (unsigned char *buf, unsigned int width, unsigned int height,
		    unsigned char num_bands, rl2PixelPtr no_data)
{
/* priming a void buffer */
    rl2PrivPixelPtr nd = (rl2PrivPixelPtr) no_data;
    unsigned int x;
    unsigned int y;
    unsigned char ib;
    unsigned char *p = buf;
    int is_valid = 0;

    if (nd != NULL)
      {
	  if (nd->nBands == num_bands && nd->sampleType == RL2_SAMPLE_UINT8
	      && nd->pixelType == RL2_PIXEL_MULTIBAND)
	      is_valid = 1;
      }
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (ib = 0; ib < num_bands; ib++)
		  {
		      if (is_valid)
			  *p++ = (nd->Samples + ib)->uint8;
		      else
			  *p++ = 0;
		  }
	    }
      }
}

static void
mb_prime_nodata_u16 (unsigned short *buf, unsigned int width,
		     unsigned int height, unsigned char num_bands,
		     rl2PixelPtr no_data)
{
/* priming a void buffer */
    rl2PrivPixelPtr nd = (rl2PrivPixelPtr) no_data;
    unsigned int x;
    unsigned int y;
    unsigned char ib;
    unsigned short *p = buf;
    int is_valid = 0;

    if (nd != NULL)
      {
	  if (nd->nBands == num_bands && nd->sampleType == RL2_SAMPLE_UINT16
	      && nd->pixelType == RL2_PIXEL_MULTIBAND)
	      is_valid = 1;
      }
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (ib = 0; ib < num_bands; ib++)
		  {
		      if (is_valid)
			  *p++ = (nd->Samples + ib)->uint16;
		      else
			  *p++ = 0;
		  }
	    }
      }
}

static rl2RasterPtr
create_rescaled_multiband_raster (unsigned int factor, unsigned int tileWidth,
				  unsigned int tileHeight, const void *buf_in,
				  const unsigned char *mask_in,
				  unsigned char sample_type,
				  unsigned char num_bands, rl2PixelPtr no_data)
{
/* rescaling a Multiband tile */
    unsigned int x;
    unsigned int y;
    unsigned int ox;
    unsigned int oy;
    rl2RasterPtr raster = NULL;
    unsigned char *mask;
    void *buf;
    unsigned int mask_sz;
    unsigned char pix_sz = 1;
    unsigned int buf_sz;
    unsigned int out_width = tileWidth / factor;
    unsigned int out_height = tileHeight / factor;

    mask_sz = out_width * out_height;
    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pix_sz = 2;
	  break;
      };
    buf_sz = out_width * out_height * pix_sz * num_bands;
    buf = malloc (buf_sz);
    if (buf == NULL)
	return NULL;
    mask = malloc (mask_sz);
    if (mask == NULL)
      {
	  free (buf);
	  return NULL;
      }
    memset (mask, 0, mask_sz);
    if (sample_type == RL2_SAMPLE_UINT16)
	mb_prime_nodata_u16 (buf, out_width, out_height, num_bands, no_data);
    else
	mb_prime_nodata_u8 (buf, out_width, out_height, num_bands, no_data);

    oy = 0;
    for (y = 0; y < tileHeight; y += factor)
      {
	  ox = 0;
	  for (x = 0; x < tileWidth; x += factor)
	    {
		switch (sample_type)
		  {
		  case RL2_SAMPLE_UINT8:
		      rescale_multiband_u8 (tileWidth, tileHeight, num_bands,
					    out_width, out_height, factor,
					    (unsigned char *) buf_in, mask_in,
					    (unsigned char *) buf, mask, x, y,
					    ox, oy, no_data);
		      break;
		  case RL2_SAMPLE_UINT16:
		      rescale_multiband_u16 (tileWidth, tileHeight, num_bands,
					     out_width, out_height, factor,
					     (unsigned short *) buf_in, mask_in,
					     (unsigned short *) buf, mask, x, y,
					     ox, oy, no_data);
		      break;
		  };
		ox++;
	    }
	  oy++;
      }

    raster =
	rl2_create_raster (out_width, out_height, sample_type,
			   RL2_PIXEL_MULTIBAND, num_bands, buf, buf_sz, NULL,
			   mask, mask_sz, NULL);
    return raster;
}

static void
copy_datagrid_rescaled (rl2RasterPtr raster_out, rl2RasterPtr raster_in,
			unsigned int base_x, unsigned int base_y)
{
/* copying pixels from rescaled to destination tile buffer */
    rl2PrivRasterPtr rst_in = (rl2PrivRasterPtr) raster_in;
    rl2PrivRasterPtr rst_out = (rl2PrivRasterPtr) raster_out;
    unsigned int x;
    unsigned int y;
    int dx;
    int dy;
    char *p_in_8 = NULL;
    char *p_out_8 = NULL;
    unsigned char *p_in_u8 = NULL;
    unsigned char *p_out_u8 = NULL;
    short *p_in_16 = NULL;
    short *p_out_16 = NULL;
    unsigned short *p_in_u16 = NULL;
    unsigned short *p_out_u16 = NULL;
    int *p_in_32 = NULL;
    int *p_out_32 = NULL;
    unsigned int *p_in_u32 = NULL;
    unsigned int *p_out_u32 = NULL;
    float *p_in_flt = NULL;
    float *p_out_flt = NULL;
    double *p_in_dbl = NULL;
    double *p_out_dbl = NULL;
    unsigned char *p_msk_in;
    unsigned char *p_msk_out;
    int mismatch = 0;

    if (rst_in->sampleType != rst_out->sampleType)
	mismatch = 1;
    if (rst_in->pixelType != RL2_PIXEL_DATAGRID
	|| rst_out->pixelType != RL2_PIXEL_DATAGRID)
	mismatch = 1;
    if (rst_in->nBands != 1 || rst_out->nBands != 1)
	mismatch = 1;
    if (rst_in->maskBuffer == NULL || rst_out->maskBuffer == NULL)
	mismatch = 1;
    if (mismatch)
      {
	  fprintf (stderr,
		   "ERROR: Copy DataGrid Rescaled mismatching in/out\n");
	  return;
      }

    switch (rst_in->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  p_in_8 = (char *) (rst_in->rasterBuffer);
	  break;
      case RL2_SAMPLE_UINT8:
	  p_in_u8 = (unsigned char *) (rst_in->rasterBuffer);
	  break;
      case RL2_SAMPLE_INT16:
	  p_in_16 = (short *) (rst_in->rasterBuffer);
	  break;
      case RL2_SAMPLE_UINT16:
	  p_in_u16 = (unsigned short *) (rst_in->rasterBuffer);
	  break;
      case RL2_SAMPLE_INT32:
	  p_in_32 = (int *) (rst_in->rasterBuffer);
	  break;
      case RL2_SAMPLE_UINT32:
	  p_in_u32 = (unsigned int *) (rst_in->rasterBuffer);
	  break;
      case RL2_SAMPLE_FLOAT:
	  p_in_flt = (float *) (rst_in->rasterBuffer);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  p_in_dbl = (double *) (rst_in->rasterBuffer);
	  break;
      };
    p_msk_in = rst_in->maskBuffer;
    for (y = 0; y < rst_in->height; y++)
      {
	  dy = base_y + y;
	  if (dy < 0 || dy >= (int) (rst_out->height))
	    {
		switch (rst_in->sampleType)
		  {
		  case RL2_SAMPLE_INT8:
		      p_in_8 += rst_in->width;
		      break;
		  case RL2_SAMPLE_UINT8:
		      p_in_u8 += rst_in->width;
		      break;
		  case RL2_SAMPLE_INT16:
		      p_in_16 += rst_in->width;
		      break;
		  case RL2_SAMPLE_UINT16:
		      p_in_u16 += rst_in->width;
		      break;
		  case RL2_SAMPLE_INT32:
		      p_in_32 += rst_in->width;
		      break;
		  case RL2_SAMPLE_UINT32:
		      p_in_u32 += rst_in->width;
		      break;
		  case RL2_SAMPLE_FLOAT:
		      p_in_flt += rst_in->width;
		      break;
		  case RL2_SAMPLE_DOUBLE:
		      p_in_dbl += rst_in->width;
		      break;
		  };
		p_msk_in += rst_in->width;
		continue;
	    }
	  for (x = 0; x < rst_in->width; x++)
	    {
		dx = base_x + x;
		if (dx < 0 || dx >= (int) (rst_out->width))
		  {
		      switch (rst_in->sampleType)
			{
			case RL2_SAMPLE_INT8:
			    p_in_8++;
			    break;
			case RL2_SAMPLE_UINT8:
			    p_in_u8++;
			    break;
			case RL2_SAMPLE_INT16:
			    p_in_16++;
			    break;
			case RL2_SAMPLE_UINT16:
			    p_in_u16++;
			    break;
			case RL2_SAMPLE_INT32:
			    p_in_32++;
			    break;
			case RL2_SAMPLE_UINT32:
			    p_in_u32++;
			    break;
			case RL2_SAMPLE_FLOAT:
			    p_in_flt++;
			    break;
			case RL2_SAMPLE_DOUBLE:
			    p_in_dbl++;
			    break;
			};
		      p_msk_in++;
		      continue;
		  }
		switch (rst_out->sampleType)
		  {
		  case RL2_SAMPLE_INT8:
		      p_out_8 = (char *) (rst_out->rasterBuffer);
		      p_out_8 += (dy * rst_out->width) + dx;
		      break;
		  case RL2_SAMPLE_UINT8:
		      p_out_u8 = (unsigned char *) (rst_out->rasterBuffer);
		      p_out_u8 += (dy * rst_out->width) + dx;
		      break;
		  case RL2_SAMPLE_INT16:
		      p_out_16 = (short *) (rst_out->rasterBuffer);
		      p_out_16 += (dy * rst_out->width) + dx;
		      break;
		  case RL2_SAMPLE_UINT16:
		      p_out_u16 = (unsigned short *) (rst_out->rasterBuffer);
		      p_out_u16 += (dy * rst_out->width) + dx;
		      break;
		  case RL2_SAMPLE_INT32:
		      p_out_32 = (int *) (rst_out->rasterBuffer);
		      p_out_32 += (dy * rst_out->width) + dx;
		      break;
		  case RL2_SAMPLE_UINT32:
		      p_out_u32 = (unsigned int *) (rst_out->rasterBuffer);
		      p_out_u32 += (dy * rst_out->width) + dx;
		      break;
		  case RL2_SAMPLE_FLOAT:
		      p_out_flt = (float *) (rst_out->rasterBuffer);
		      p_out_flt += (dy * rst_out->width) + dx;
		      break;
		  case RL2_SAMPLE_DOUBLE:
		      p_out_dbl = (double *) (rst_out->rasterBuffer);
		      p_out_dbl += (dy * rst_out->width) + dx;
		      break;
		  };
		p_msk_out = rst_out->maskBuffer + (dy * rst_out->width) + dx;
		if (*p_msk_in++ == 0)
		  {
		      switch (rst_in->sampleType)
			{
			case RL2_SAMPLE_INT8:
			    p_in_8++;
			    break;
			case RL2_SAMPLE_UINT8:
			    p_in_u8++;
			    break;
			case RL2_SAMPLE_INT16:
			    p_in_16++;
			    break;
			case RL2_SAMPLE_UINT16:
			    p_in_u16++;
			    break;
			case RL2_SAMPLE_INT32:
			    p_in_32++;
			    break;
			case RL2_SAMPLE_UINT32:
			    p_in_u32++;
			    break;
			case RL2_SAMPLE_FLOAT:
			    p_in_flt++;
			    break;
			case RL2_SAMPLE_DOUBLE:
			    p_in_dbl++;
			    break;
			};
		  }
		else
		  {
		      switch (rst_out->sampleType)
			{
			case RL2_SAMPLE_INT8:
			    *p_out_8++ = *p_in_8++;
			    break;
			case RL2_SAMPLE_UINT8:
			    *p_out_u8 = *p_in_u8++;
			    break;
			case RL2_SAMPLE_INT16:
			    *p_out_16 = *p_in_16++;
			    break;
			case RL2_SAMPLE_UINT16:
			    *p_out_u16 = *p_in_u16++;
			    break;
			case RL2_SAMPLE_INT32:
			    *p_out_32 = *p_in_32++;
			    break;
			case RL2_SAMPLE_UINT32:
			    *p_out_u32 = *p_in_u32++;
			    break;
			case RL2_SAMPLE_FLOAT:
			    *p_out_flt = *p_in_flt++;
			    break;
			case RL2_SAMPLE_DOUBLE:
			    *p_out_dbl = *p_in_dbl++;
			    break;
			};
		      *p_msk_out++ = 1;
		  }
	    }
      }
}

static void
rescale_datagrid_8 (unsigned int tileWidth, unsigned int tileHeight,
		    unsigned int out_width, unsigned int out_height,
		    unsigned int factor, char *buf_in, char *buf_out,
		    unsigned char *mask, unsigned int x, unsigned int y,
		    unsigned int ox, unsigned int oy, char nd)
{
/* rescaling a DataGrid (monolithic)- INT8 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;
    char *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy) + ox;
    p_msk = mask + (out_width * oy) + ox;

    for (row = 0; row < factor; row++)
      {
	  const char *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < factor; col++)
	    {
		const char *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + xx;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return;
    *p_out = (char) (sum / (double) valid);
    *p_msk = 1;
}

static void
rescale_datagrid_u8 (unsigned int tileWidth, unsigned int tileHeight,
		     unsigned int out_width, unsigned int out_height,
		     unsigned int factor, unsigned char *buf_in,
		     unsigned char *buf_out, unsigned char *mask,
		     unsigned int x, unsigned int y, unsigned int ox,
		     unsigned int oy, unsigned char nd)
{
/* rescaling a DataGrid (monolithic)- UINT8 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;
    unsigned char *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy) + ox;
    p_msk = mask + (out_width * oy) + ox;

    for (row = 0; row < factor; row++)
      {
	  const unsigned char *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < factor; col++)
	    {
		const unsigned char *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + xx;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return;
    *p_out = (unsigned char) (sum / (double) valid);
    *p_msk = 1;
}

static void
rescale_datagrid_16 (unsigned int tileWidth, unsigned int tileHeight,
		     unsigned int out_width, unsigned int out_height,
		     unsigned int factor, short *buf_in, short *buf_out,
		     unsigned char *mask, unsigned int x, unsigned int y,
		     unsigned int ox, unsigned int oy, short nd)
{
/* rescaling a DataGrid (monolithic)- INT16 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;
    short *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy) + ox;
    p_msk = mask + (out_width * oy) + ox;

    for (row = 0; row < factor; row++)
      {
	  const short *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < factor; col++)
	    {
		const short *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + xx;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return;
    *p_out = (short) (sum / (double) valid);
    *p_msk = 1;
}

static void
rescale_datagrid_u16 (unsigned int tileWidth, unsigned int tileHeight,
		      unsigned int out_width, unsigned int out_height,
		      unsigned int factor, unsigned short *buf_in,
		      unsigned short *buf_out, unsigned char *mask,
		      unsigned int x, unsigned int y, unsigned int ox,
		      unsigned int oy, unsigned short nd)
{
/* rescaling a DataGrid (monolithic)- UINT16 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;
    unsigned short *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy) + ox;
    p_msk = mask + (out_width * oy) + ox;

    for (row = 0; row < factor; row++)
      {
	  const unsigned short *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < factor; col++)
	    {
		const unsigned short *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + xx;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return;
    *p_out = (unsigned short) (sum / (double) valid);
    *p_msk = 1;
}

static void
rescale_datagrid_32 (unsigned int tileWidth, unsigned int tileHeight,
		     unsigned int out_width, unsigned int out_height,
		     unsigned int factor, int *buf_in, int *buf_out,
		     unsigned char *mask, unsigned int x, unsigned int y,
		     unsigned int ox, unsigned int oy, int nd)
{
/* rescaling a DataGrid (monolithic)- INT32 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;
    int *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy) + ox;
    p_msk = mask + (out_width * oy) + ox;

    for (row = 0; row < factor; row++)
      {
	  const int *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < factor; col++)
	    {
		const int *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + xx;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return;
    *p_out = (int) (sum / (double) valid);
    *p_msk = 1;
}

static void
rescale_datagrid_u32 (unsigned int tileWidth, unsigned int tileHeight,
		      unsigned int out_width, unsigned int out_height,
		      unsigned int factor, unsigned int *buf_in,
		      unsigned int *buf_out, unsigned char *mask,
		      unsigned int x, unsigned int y, unsigned int ox,
		      unsigned int oy, unsigned int nd)
{
/* rescaling a DataGrid (monolithic)- UINT32 */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;
    unsigned int *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy) + ox;
    p_msk = mask + (out_width * oy) + ox;

    for (row = 0; row < factor; row++)
      {
	  const unsigned int *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < factor; col++)
	    {
		const unsigned int *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + xx;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return;
    *p_out = (unsigned int) (sum / (double) valid);
    *p_msk = 1;
}

static void
rescale_datagrid_flt (unsigned int tileWidth, unsigned int tileHeight,
		      unsigned int out_width, unsigned int out_height,
		      unsigned int factor, float *buf_in, float *buf_out,
		      unsigned char *mask, unsigned int x, unsigned int y,
		      unsigned int ox, unsigned int oy, float nd)
{
/* rescaling a DataGrid (monolithic)- FLOAT */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;
    float *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy) + ox;
    p_msk = mask + (out_width * oy) + ox;

    for (row = 0; row < factor; row++)
      {
	  const float *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < factor; col++)
	    {
		const float *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + xx;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return;
    *p_out = (float) (sum / (double) valid);
    *p_msk = 1;
}

static void
rescale_datagrid_dbl (unsigned int tileWidth, unsigned int tileHeight,
		      unsigned int out_width, unsigned int out_height,
		      unsigned int factor, double *buf_in, double *buf_out,
		      unsigned char *mask, unsigned int x, unsigned int y,
		      unsigned int ox, unsigned int oy, double nd)
{
/* rescaling a DataGrid (monolithic)- DOUBLE */
    unsigned int row;
    unsigned int col;
    int nodata = 0;
    int valid = 0;
    double sum = 0.0;
    double *p_out;
    unsigned char *p_msk;

    if (ox >= out_width || oy >= out_height)
	return;
    p_out = buf_out + (out_width * oy) + ox;
    p_msk = mask + (out_width * oy) + ox;

    for (row = 0; row < factor; row++)
      {
	  const double *p_in_base;
	  unsigned int yy = y + row;
	  if (yy >= tileHeight)
	      break;
	  p_in_base = buf_in + (yy * tileWidth);
	  for (col = 0; col < factor; col++)
	    {
		const double *p_in;
		unsigned int xx = x + col;
		if (xx >= tileWidth)
		    break;
		p_in = p_in_base + xx;
		if (*p_in == nd)
		    nodata++;
		else
		  {
		      valid++;
		      sum += *p_in;
		  }
	    }
      }

    if (nodata >= valid)
	return;
    *p_out = (double) (sum / (double) valid);
    *p_msk = 1;
}

static rl2RasterPtr
create_rescaled_datagrid_raster (unsigned int factor, unsigned int tileWidth,
				 unsigned int tileHeight, const void *buf_in,
				 unsigned char sample_type, rl2PixelPtr no_data)
{
/* rescaling a Datagrid tile */
    rl2PrivPixelPtr pxl;
    unsigned int x;
    unsigned int y;
    unsigned int ox;
    unsigned int oy;
    rl2RasterPtr raster = NULL;
    unsigned char *mask;
    void *buf;
    unsigned int mask_sz;
    unsigned char pix_sz = 1;
    unsigned int buf_sz;
    unsigned int out_width = tileWidth / factor;
    unsigned int out_height = tileHeight / factor;
    char no_data_8 = 0;
    unsigned char no_data_u8 = 0;
    short no_data_16 = 0;
    unsigned short no_data_u16 = 0;
    int no_data_32 = 0;
    unsigned int no_data_u32 = 0;
    float no_data_flt = 0.0;
    double no_data_dbl = 0.0;

/* retrieving NO-DATA */
    pxl = (rl2PrivPixelPtr) no_data;
    if (pxl != NULL)
      {
	  if (pxl->nBands == 1 && pxl->pixelType == RL2_PIXEL_DATAGRID
	      && pxl->sampleType == sample_type)
	    {
		rl2PrivSamplePtr sample = pxl->Samples;
		switch (sample_type)
		  {
		  case RL2_SAMPLE_INT8:
		      no_data_8 = sample->int8;
		      break;
		  case RL2_SAMPLE_UINT8:
		      no_data_u8 = sample->uint8;
		      break;
		  case RL2_SAMPLE_INT16:
		      no_data_16 = sample->int16;
		      break;
		  case RL2_SAMPLE_UINT16:
		      no_data_u16 = sample->uint16;
		      break;
		  case RL2_SAMPLE_INT32:
		      no_data_32 = sample->int32;
		      break;
		  case RL2_SAMPLE_UINT32:
		      no_data_u32 = sample->uint32;
		      break;
		  case RL2_SAMPLE_FLOAT:
		      no_data_flt = sample->float32;
		      break;
		  case RL2_SAMPLE_DOUBLE:
		      no_data_dbl = sample->float64;
		      break;
		  };
	    }
      }
/* computing sizes */
    mask_sz = out_width * out_height;
    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pix_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pix_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pix_sz = 8;
	  break;
      };
    buf_sz = out_width * out_height * pix_sz;
    buf = malloc (buf_sz);
    if (buf == NULL)
	return NULL;
    mask = malloc (mask_sz);
    if (mask == NULL)
      {
	  free (buf);
	  return NULL;
      }
    rl2_prime_void_tile (buf, out_width, out_height, sample_type, 1, no_data);
    memset (mask, 0, mask_sz);

    oy = 0;
    for (y = 0; y < tileHeight; y += factor)
      {
	  ox = 0;
	  for (x = 0; x < tileWidth; x += factor)
	    {
		switch (sample_type)
		  {
		  case RL2_SAMPLE_INT8:
		      rescale_datagrid_8 (tileWidth, tileHeight, out_width,
					  out_height, factor, (char *) buf_in,
					  (char *) buf, mask, x, y, ox, oy,
					  no_data_8);
		      break;
		  case RL2_SAMPLE_UINT8:
		      rescale_datagrid_u8 (tileWidth, tileHeight, out_width,
					   out_height, factor,
					   (unsigned char *) buf_in,
					   (unsigned char *) buf, mask, x, y,
					   ox, oy, no_data_u8);
		      break;
		  case RL2_SAMPLE_INT16:
		      rescale_datagrid_16 (tileWidth, tileHeight, out_width,
					   out_height, factor, (short *) buf_in,
					   (short *) buf, mask, x, y, ox, oy,
					   no_data_16);
		      break;
		  case RL2_SAMPLE_UINT16:
		      rescale_datagrid_u16 (tileWidth, tileHeight, out_width,
					    out_height, factor,
					    (unsigned short *) buf_in,
					    (unsigned short *) buf, mask, x, y,
					    ox, oy, no_data_u16);
		      break;
		  case RL2_SAMPLE_INT32:
		      rescale_datagrid_32 (tileWidth, tileHeight, out_width,
					   out_height, factor, (int *) buf_in,
					   (int *) buf, mask, x, y, ox, oy,
					   no_data_32);
		      break;
		  case RL2_SAMPLE_UINT32:
		      rescale_datagrid_u32 (tileWidth, tileHeight, out_width,
					    out_height, factor,
					    (unsigned int *) buf_in,
					    (unsigned int *) buf, mask, x, y,
					    ox, oy, no_data_u32);
		      break;
		  case RL2_SAMPLE_FLOAT:
		      rescale_datagrid_flt (tileWidth, tileHeight, out_width,
					    out_height, factor,
					    (float *) buf_in, (float *) buf,
					    mask, x, y, ox, oy, no_data_flt);
		      break;
		  case RL2_SAMPLE_DOUBLE:
		      rescale_datagrid_dbl (tileWidth, tileHeight, out_width,
					    out_height, factor,
					    (double *) buf_in, (double *) buf,
					    mask, x, y, ox, oy, no_data_dbl);
		      break;
		  };
		ox++;
	    }
	  oy++;
      }

    raster =
	rl2_create_raster (out_width, out_height, sample_type,
			   RL2_PIXEL_DATAGRID, 1, buf, buf_sz, NULL, mask,
			   mask_sz, NULL);
    return raster;
}

static int
rescale_monolithic_multiband (int id_level,
			      unsigned int tileWidth, unsigned int tileHeight,
			      unsigned char sample_type,
			      unsigned char num_bands, int factor, double res_x,
			      double res_y, double minx, double miny,
			      double maxx, double maxy, unsigned char *buffer,
			      int buf_size, unsigned char *mask, int *mask_size,
			      rl2PixelPtr no_data, sqlite3_stmt * stmt_geo,
			      sqlite3_stmt * stmt_data)
{
/* rescaling monolithic MultiBand */
    rl2RasterPtr raster = NULL;
    rl2RasterPtr base_tile = NULL;
    rl2PrivRasterPtr rst;
    unsigned int x;
    unsigned int y;
    int ret;
    double shift_x;
    double shift_y;
    int valid_mask = 0;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PixelPtr nd = NULL;

    nd = rl2_clone_pixel (no_data);
    p_out = mask;
    for (y = 0; y < tileHeight; y++)
      {
	  /* priming full transparency */
	  for (x = 0; x < tileWidth; x++)
	      *p_out++ = 0;
      }
    rl2_prime_void_tile (buffer, tileWidth, tileHeight, sample_type, num_bands,
			 no_data);
/* creating the output raster */
    raster =
	rl2_create_raster (tileWidth, tileHeight, sample_type,
			   RL2_PIXEL_MULTIBAND, num_bands, buffer, buf_size,
			   NULL, mask, *mask_size, nd);
    if (raster == NULL)
	goto error;

/* binding the BBOX to be queried */
    sqlite3_reset (stmt_geo);
    sqlite3_clear_bindings (stmt_geo);
    sqlite3_bind_int (stmt_geo, 1, id_level);
    sqlite3_bind_double (stmt_geo, 2, minx);
    sqlite3_bind_double (stmt_geo, 3, miny);
    sqlite3_bind_double (stmt_geo, 4, maxx);
    sqlite3_bind_double (stmt_geo, 5, maxy);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_geo);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		rl2RasterPtr raster_in = NULL;
		rl2PrivRasterPtr rst_in;
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_geo, 0);
		double tile_x = sqlite3_column_double (stmt_geo, 1);
		double tile_y = sqlite3_column_double (stmt_geo, 2);

		raster_in = load_tile_base_generic (stmt_data, tile_id);
		rst_in = (rl2PrivRasterPtr) raster_in;
		base_tile =
		    create_rescaled_multiband_raster (factor, tileWidth,
						      tileHeight,
						      rst_in->rasterBuffer,
						      rst_in->maskBuffer,
						      sample_type, num_bands,
						      no_data);
		rl2_destroy_raster (raster_in);
		raster_in = NULL;

		if (base_tile == NULL)
		    goto error;
		shift_x = tile_x - minx;
		shift_y = maxy - tile_y;
		x = (int) (shift_x / res_x);
		y = (int) (shift_y / res_y);
		copy_multiband_rescaled (raster, base_tile, x, y);
		rl2_destroy_raster (base_tile);
	    }
      }

/* releasing buffers ownership */
    rst = (rl2PrivRasterPtr) raster;
    rst->rasterBuffer = NULL;
    rst->maskBuffer = NULL;
    rl2_destroy_raster (raster);

    p_in = mask;
    for (y = 0; y < tileHeight; y++)
      {
	  for (x = 0; x < tileWidth; x++)
	    {
		if (*p_in++ == 0)
		    valid_mask = 1;
	    }
      }
    if (!valid_mask)
      {
	  free (mask);
	  *mask_size = 0;
      }

    return 1;
  error:
    if (raster != NULL)
      {
	  /* releasing buffers ownership */
	  rst = (rl2PrivRasterPtr) raster;
	  rst->rasterBuffer = NULL;
	  rst->maskBuffer = NULL;
	  rl2_destroy_raster (raster);
      }
    return 0;
}

static int
rescale_monolithic_datagrid (int id_level,
			     unsigned int tileWidth, unsigned int tileHeight,
			     unsigned char sample_type, int factor,
			     double res_x, double res_y, double minx,
			     double miny, double maxx, double maxy,
			     unsigned char *buffer, int buf_size,
			     unsigned char *mask, int *mask_size,
			     rl2PixelPtr no_data, sqlite3_stmt * stmt_geo,
			     sqlite3_stmt * stmt_data)
{
/* rescaling monolithic DataGrid */
    rl2RasterPtr raster = NULL;
    rl2RasterPtr base_tile = NULL;
    rl2PrivRasterPtr rst;
    unsigned int x;
    unsigned int y;
    int ret;
    double shift_x;
    double shift_y;
    int valid_mask = 0;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PixelPtr nd = NULL;

    nd = rl2_clone_pixel (no_data);
    p_out = mask;
    for (y = 0; y < tileHeight; y++)
      {
	  /* priming full transparency */
	  for (x = 0; x < tileWidth; x++)
	      *p_out++ = 0;
      }
    rl2_prime_void_tile (buffer, tileWidth, tileHeight, sample_type, 1,
			 no_data);
/* creating the output raster */
    raster =
	rl2_create_raster (tileWidth, tileHeight, sample_type,
			   RL2_PIXEL_DATAGRID, 1, buffer, buf_size,
			   NULL, mask, *mask_size, nd);
    if (raster == NULL)
	goto error;

/* binding the BBOX to be queried */
    sqlite3_reset (stmt_geo);
    sqlite3_clear_bindings (stmt_geo);
    sqlite3_bind_int (stmt_geo, 1, id_level);
    sqlite3_bind_double (stmt_geo, 2, minx);
    sqlite3_bind_double (stmt_geo, 3, miny);
    sqlite3_bind_double (stmt_geo, 4, maxx);
    sqlite3_bind_double (stmt_geo, 5, maxy);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_geo);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		rl2RasterPtr raster_in = NULL;
		rl2PrivRasterPtr rst_in;
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_geo, 0);
		double tile_x = sqlite3_column_double (stmt_geo, 1);
		double tile_y = sqlite3_column_double (stmt_geo, 2);

		raster_in = load_tile_base_generic (stmt_data, tile_id);
		rst_in = (rl2PrivRasterPtr) raster_in;
		base_tile =
		    create_rescaled_datagrid_raster (factor, tileWidth,
						     tileHeight,
						     rst_in->rasterBuffer,
						     sample_type, no_data);
		rl2_destroy_raster (raster_in);
		raster_in = NULL;

		if (base_tile == NULL)
		    goto error;
		shift_x = tile_x - minx;
		shift_y = maxy - tile_y;
		x = (int) (shift_x / res_x);
		y = (int) (shift_y / res_y);
		copy_datagrid_rescaled (raster, base_tile, x, y);
		rl2_destroy_raster (base_tile);
	    }
      }

/* releasing buffers ownership */
    rst = (rl2PrivRasterPtr) raster;
    rst->rasterBuffer = NULL;
    rst->maskBuffer = NULL;
    rl2_destroy_raster (raster);

    p_in = mask;
    for (y = 0; y < tileHeight; y++)
      {
	  for (x = 0; x < tileWidth; x++)
	    {
		if (*p_in++ == 0)
		    valid_mask = 1;
	    }
      }
    if (!valid_mask)
      {
	  free (mask);
	  *mask_size = 0;
      }

    return 1;
  error:
    if (raster != NULL)
      {
	  /* releasing buffers ownership */
	  rst = (rl2PrivRasterPtr) raster;
	  rst->rasterBuffer = NULL;
	  rst->maskBuffer = NULL;
	  rl2_destroy_raster (raster);
      }
    return 0;
}

static int
prepare_section_pyramid_stmts (sqlite3 * handle, const char *coverage,
			       int mixed_resolutions, sqlite3_stmt ** xstmt_rd,
			       sqlite3_stmt ** xstmt_levl,
			       sqlite3_stmt ** xstmt_tils,
			       sqlite3_stmt ** xstmt_data)
{
/* preparing the section pyramid related SQL statements */
    char *table_tile_data;
    char *xtable_tile_data;
    char *table;
    char *xtable;
    char *sql;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;

    *xstmt_rd = NULL;
    *xstmt_levl = NULL;
    *xstmt_tils = NULL;
    *xstmt_data = NULL;

    table_tile_data = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable_tile_data = rl2_double_quoted_sql (table_tile_data);
    sqlite3_free (table_tile_data);
    sql = sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even "
			   "FROM \"%s\" WHERE tile_id = ?", xtable_tile_data);
    free (xtable_tile_data);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_rd, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    if (mixed_resolutions)
      {
	  /* mixed resolution Coverage */
	  table = sqlite3_mprintf ("%s_section_levels", coverage);
	  xtable = rl2_double_quoted_sql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf
	      ("INSERT OR IGNORE INTO \"%s\" (section_id, pyramid_level, "
	       "x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	       "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	       "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
      }
    else
      {
	  /* ordinary Coverage */
	  table = sqlite3_mprintf ("%s_levels", coverage);
	  xtable = rl2_double_quoted_sql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf
	      ("INSERT OR IGNORE INTO \"%s\" (pyramid_level, "
	       "x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	       "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	       "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
      }
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO levels SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, ?, ?, BuildMBR(?, ?, ?, ?, ?))", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tils, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, tile_data_odd, tile_data_even) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tile_data SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    *xstmt_rd = stmt_rd;
    *xstmt_levl = stmt_levl;
    *xstmt_tils = stmt_tils;
    *xstmt_data = stmt_data;
    return 1;

  error:
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    return 0;
}

static int
do_build_section_pyramid (sqlite3 * handle, const char *coverage,
			  sqlite3_int64 section_id, unsigned char sample_type,
			  unsigned char pixel_type, unsigned char num_samples,
			  unsigned char compression, int mixed_resolutions,
			  int quality, int srid, unsigned int tileWidth,
			  unsigned int tileHeight)
{
/* attempting to (re)build a section pyramid from scratch */
    char *table_levels;
    char *xtable_levels;
    char *table_tiles;
    char *xtable_tiles;
    char *sql;
    int id_level = 0;
    double new_res_x;
    double new_res_y;
    unsigned int sect_width;
    unsigned int sect_height;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned int row;
    unsigned int col;
    double out_minx;
    double out_miny;
    double out_maxx;
    double out_maxy;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    SectionPyramid *pyr = NULL;
    int ret;
    int first;
    int scale;
    rl2PalettePtr palette = NULL;
    rl2PixelPtr no_data = NULL;

    if (!get_section_infos
	(handle, coverage, section_id, &sect_width, &sect_height, &minx,
	 &miny, &maxx, &maxy, &palette, &no_data))
	goto error;

    if (!prepare_section_pyramid_stmts
	(handle, coverage, mixed_resolutions, &stmt_rd, &stmt_levl, &stmt_tils,
	 &stmt_data))
	goto error;

    while (1)
      {
	  /* looping on pyramid levels */
	  if (mixed_resolutions)
	      table_levels = sqlite3_mprintf ("%s_section_levels", coverage);
	  else
	      table_levels = sqlite3_mprintf ("%s_levels", coverage);
	  xtable_levels = rl2_double_quoted_sql (table_levels);
	  sqlite3_free (table_levels);
	  table_tiles = sqlite3_mprintf ("%s_tiles", coverage);
	  xtable_tiles = rl2_double_quoted_sql (table_tiles);
	  sqlite3_free (table_tiles);
	  if (mixed_resolutions)
	    {
		/* mixed resolutions Coverage */
		sql =
		    sqlite3_mprintf
		    ("SELECT l.x_resolution_1_1, l.y_resolution_1_1, "
		     "t.tile_id, MbrMinX(t.geometry), MbrMinY(t.geometry), "
		     "MbrMaxX(t.geometry), MbrMaxY(t.geometry) "
		     "FROM \"%s\" AS l "
		     "JOIN \"%s\" AS t ON (l.section_id = t.section_id "
		     "AND l.pyramid_level = t.pyramid_level) "
		     "WHERE l.pyramid_level = %d AND l.section_id = ?",
		     xtable_levels, xtable_tiles, id_level);
	    }
	  else
	    {
		/* ordinary Coverage */
		sql =
		    sqlite3_mprintf
		    ("SELECT l.x_resolution_1_1, l.y_resolution_1_1, "
		     "t.tile_id, MbrMinX(t.geometry), MbrMinY(t.geometry), "
		     "MbrMaxX(t.geometry), MbrMaxY(t.geometry) "
		     "FROM \"%s\" AS l "
		     "JOIN \"%s\" AS t ON (l.pyramid_level = t.pyramid_level) "
		     "WHERE l.pyramid_level = %d AND t.section_id = ?",
		     xtable_levels, xtable_tiles, id_level);
	    }
	  free (xtable_levels);
	  free (xtable_tiles);
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, section_id);
	  first = 1;
	  while (1)
	    {
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      double res_x = sqlite3_column_double (stmt, 0);
		      double res_y = sqlite3_column_double (stmt, 1);
		      sqlite3_int64 tile_id = sqlite3_column_int64 (stmt, 2);
		      double tminx = sqlite3_column_double (stmt, 3);
		      double tminy = sqlite3_column_double (stmt, 4);
		      double tmaxx = sqlite3_column_double (stmt, 5);
		      double tmaxy = sqlite3_column_double (stmt, 6);
		      new_res_x = res_x * 8.0;
		      new_res_y = res_y * 8.0;
		      scale = 8;
		      if (first)
			{
			    pyr =
				alloc_sect_pyramid (section_id, sect_width,
						    sect_height, sample_type,
						    pixel_type, num_samples,
						    compression, quality, srid,
						    new_res_x, new_res_y,
						    (double) tileWidth *
						    new_res_x,
						    (double) tileHeight *
						    new_res_y, minx, miny, maxx,
						    maxy, scale);
			    first = 0;
			    if (pyr == NULL)
				goto error;
			}
		      if (!insert_tile_into_section_pyramid
			  (pyr, tile_id, tminx, tminy, tmaxx, tmaxy))
			  goto error;
		  }
		else
		  {
		      fprintf (stderr,
			       "SELECT base level; sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
	    }
	  sqlite3_finalize (stmt);
	  stmt = NULL;
	  if (pyr == NULL)
	      goto error;

	  out_maxy = maxy;
	  for (row = 0; row < pyr->scaled_height; row += tileHeight)
	    {
		out_miny = out_maxy - pyr->tile_height;
		out_minx = minx;
		for (col = 0; col < pyr->scaled_width; col += tileWidth)
		  {
		      out_maxx = out_minx + pyr->tile_width;
		      set_pyramid_tile_destination (pyr, out_minx, out_miny,
						    out_maxx, out_maxy, row,
						    col);
		      out_minx += pyr->tile_width;
		  }
		out_maxy -= pyr->tile_height;
	    }
	  id_level++;
	  if (pyr->scaled_width <= tileWidth
	      && pyr->scaled_height <= tileHeight)
	      break;
	  if (mixed_resolutions)
	    {
		if (!do_insert_pyramid_section_levels
		    (handle, section_id, id_level, pyr->res_x, pyr->res_y,
		     stmt_levl))
		    goto error;
	    }
	  else
	    {
		if (!do_insert_pyramid_levels
		    (handle, id_level, pyr->res_x, pyr->res_y, stmt_levl))
		    goto error;
	    }
	  if (pixel_type == RL2_PIXEL_DATAGRID)
	    {
		/* DataGrid Pyramid */
		if (!update_sect_pyramid_grid
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, no_data, sample_type))
		    goto error;
	    }
	  else if (pixel_type == RL2_PIXEL_MULTIBAND)
	    {
		/* MultiBand Pyramid */
		if (!update_sect_pyramid_multiband
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, no_data, sample_type, num_samples))
		    goto error;
	    }
	  else
	    {
		/* any other Pyramid [not a DataGrid] */
		if (!update_sect_pyramid
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, palette, no_data))
		    goto error;
	    }
	  delete_sect_pyramid (pyr);
	  pyr = NULL;
      }

    if (pyr != NULL)
      {
	  if (mixed_resolutions)
	    {
		if (!do_insert_pyramid_section_levels
		    (handle, section_id, id_level, pyr->res_x, pyr->res_y,
		     stmt_levl))
		    goto error;
	    }
	  else
	    {
		if (!do_insert_pyramid_levels
		    (handle, id_level, pyr->res_x, pyr->res_y, stmt_levl))
		    goto error;
	    }
	  if (pixel_type == RL2_PIXEL_DATAGRID)
	    {
		/* DataGrid Pyramid */
		if (!update_sect_pyramid_grid
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, no_data, sample_type))
		    goto error;
	    }
	  else if (pixel_type == RL2_PIXEL_MULTIBAND)
	    {
		/* MultiBand Pyramid */
		if (!update_sect_pyramid_multiband
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, no_data, sample_type, num_samples))
		    goto error;
	    }
	  else
	    {
		/* any other Pyramid [not a DataGrid] */
		if (!update_sect_pyramid
		    (handle, stmt_rd, stmt_tils, stmt_data, pyr, tileWidth,
		     tileHeight, id_level, palette, no_data))
		    goto error;
	    }
	  delete_sect_pyramid (pyr);
      }
    sqlite3_finalize (stmt_rd);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (pyr != NULL)
	delete_sect_pyramid (pyr);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return 0;
}

static int
get_coverage_extent (sqlite3 * handle, const char *coverage, double *minx,
		     double *miny, double *maxx, double *maxy)
{
/* attempting to get the Extent from a Coverage Object */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    double extent_minx = 0.0;
    double extent_miny = 0.0;
    double extent_maxx = 0.0;
    double extent_maxy = 0.0;
    int ok = 0;

/* querying the Coverage metadata defs */
    sql =
	"SELECT extent_minx, extent_miny, extent_maxx, extent_maxy "
	"FROM raster_coverages WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_minx = 0;
		int ok_miny = 0;
		int ok_maxx = 0;
		int ok_maxy = 0;
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT)
		  {
		      extent_minx = sqlite3_column_double (stmt, 0);
		      ok_minx = 1;
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_FLOAT)
		  {
		      extent_miny = sqlite3_column_double (stmt, 1);
		      ok_miny = 1;
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		  {
		      extent_maxx = sqlite3_column_double (stmt, 2);
		      ok_maxx = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_FLOAT)
		  {
		      extent_maxy = sqlite3_column_double (stmt, 3);
		      ok_maxy = 1;
		  }
		if (ok_minx && ok_miny && ok_maxx && ok_maxy)
		    ok = 1;
	    }
      }
    sqlite3_finalize (stmt);

    if (!ok)
	return 0;
    *minx = extent_minx;
    *miny = extent_miny;
    *maxx = extent_maxx;
    *maxy = extent_maxy;
    return 1;
}

static int
find_base_resolution (sqlite3 * handle, const char *coverage,
		      double *x_res, double *y_res)
{
/* attempting to identify the base resolution level */
    int ret;
    int found = 0;
    double xx_res;
    double yy_res;
    char *xcoverage;
    char *xxcoverage;
    char *sql;
    sqlite3_stmt *stmt = NULL;

    xcoverage = sqlite3_mprintf ("%s_levels", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    sql =
	sqlite3_mprintf ("SELECT x_resolution_1_1, y_resolution_1_1 "
			 "FROM \"%s\" WHERE pyramid_level = 0", xxcoverage);
    free (xxcoverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_free (sql);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 1) == SQLITE_FLOAT)
		  {
		      found = 1;
		      xx_res = sqlite3_column_double (stmt, 0);
		      yy_res = sqlite3_column_double (stmt, 1);
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (found)
      {
	  *x_res = xx_res;
	  *y_res = yy_res;
	  return 1;
      }
    return 0;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
find_section_base_resolution (sqlite3 * handle, const char *coverage,
			      sqlite3_int64 section_id, double *x_res,
			      double *y_res)
{
/* attempting to identify the base resolution level */
/*           mixed resolution Coverage              */
    int ret;
    int found = 0;
    double xx_res;
    double yy_res;
    char *xcoverage;
    char *xxcoverage;
    char *sql;
    sqlite3_stmt *stmt = NULL;

    xcoverage = sqlite3_mprintf ("%s_section_levels", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    sql =
	sqlite3_mprintf ("SELECT x_resolution_1_1, y_resolution_1_1 "
			 "FROM \"%s\" WHERE section_id = ? AND pyramid_level = 0",
			 xxcoverage);
    free (xxcoverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_free (sql);

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, section_id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 1) == SQLITE_FLOAT)
		  {
		      found = 1;
		      xx_res = sqlite3_column_double (stmt, 0);
		      yy_res = sqlite3_column_double (stmt, 1);
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (found)
      {
	  *x_res = xx_res;
	  *y_res = yy_res;
	  return 1;
      }
    return 0;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
get_section_raw_raster_data (sqlite3 * handle, int max_threads,
			     const char *coverage, sqlite3_int64 sect_id,
			     unsigned int width, unsigned int height,
			     unsigned char sample_type,
			     unsigned char pixel_type, unsigned char num_bands,
			     double minx, double maxy, double x_res,
			     double y_res, unsigned char **buffer,
			     int *buf_size, rl2PalettePtr palette,
			     rl2PixelPtr no_data)
{
/* attempting to return a buffer containing raw pixels from the whole DBMS Section */
    unsigned char *bufpix = NULL;
    int bufpix_size;
    char *xtiles;
    char *xxtiles;
    char *xdata;
    char *xxdata;
    char *sql;
    sqlite3_stmt *stmt_tiles = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;

    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
	  break;
      case RL2_SAMPLE_UINT8:
	  if (pixel_type != RL2_PIXEL_PALETTE)
	      goto error;
	  break;
      default:
	  goto error;
	  break;
      };
    bufpix_size = num_bands * width * height;
    bufpix = malloc (bufpix_size);
    if (bufpix == NULL)
      {
	  fprintf (stderr,
		   "get_section_raw_raster_data: Insufficient Memory !!!\n");
	  goto error;
      }
    memset (bufpix, 0, bufpix_size);

/* preparing the "tiles" SQL query */
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = rl2_double_quoted_sql (xtiles);
    sql =
	sqlite3_mprintf ("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry) "
			 "FROM \"%s\" "
			 "WHERE pyramid_level = 0 AND section_id = ?", xxtiles);
    sqlite3_free (xtiles);
    free (xxtiles);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tiles, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT section raw tiles SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* preparing the data SQL query - both ODD and EVEN */
    xdata = sqlite3_mprintf ("%s_tile_data", coverage);
    xxdata = rl2_double_quoted_sql (xdata);
    sqlite3_free (xdata);
    sql = sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even "
			   "FROM \"%s\" WHERE tile_id = ?", xxdata);
    free (xxdata);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT section raw tiles data(2) SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* preparing a raw pixels buffer */
    if (pixel_type == RL2_PIXEL_PALETTE)
	void_raw_buffer_palette (bufpix, width, height, no_data);
    else
	void_raw_buffer (bufpix, width, height, sample_type, num_bands,
			 no_data);
    if (!rl2_load_dbms_tiles_section
	(handle, max_threads, sect_id, stmt_tiles, stmt_data, bufpix, width,
	 height, sample_type, num_bands, 0, 0, 0, x_res, y_res, minx, maxy,
	 RL2_SCALE_1, palette, no_data))
	goto error;
    sqlite3_finalize (stmt_tiles);
    sqlite3_finalize (stmt_data);
    *buffer = bufpix;
    *buf_size = bufpix_size;
    return 1;

  error:
    if (stmt_tiles != NULL)
	sqlite3_finalize (stmt_tiles);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (bufpix != NULL)
	free (bufpix);
    return 0;
}

static void
raster_tile_124_rescaled (unsigned char *outbuf,
			  unsigned char pixel_type, const unsigned char *inbuf,
			  unsigned int section_width,
			  unsigned int section_height, unsigned int out_width,
			  unsigned int out_height, rl2PalettePtr palette)
{
/* 
/ this function builds an high quality rescaled sub-image by applying pixel interpolation
/
/ this code is widely inspired by the original GD gdImageCopyResampled() function
*/
    unsigned int x;
    unsigned int y;
    double sy1;
    double sy2;
    double sx1;
    double sx2;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    const unsigned char *p_in;
    unsigned char *p_out;

    if (pixel_type == RL2_PIXEL_PALETTE && palette == NULL)
	return;

    for (y = 0; y < out_height; y++)
      {
	  sy1 = ((double) y) * (double) section_height / (double) out_height;
	  sy2 =
	      ((double) (y + 1)) * (double) section_height /
	      (double) out_height;
	  for (x = 0; x < out_width; x++)
	    {
		double sx;
		double sy;
		double spixels = 0;
		double red = 0.0;
		double green = 0.0;
		double blue = 0.0;
		sx1 =
		    ((double) x) * (double) section_width / (double) out_width;
		sx2 =
		    ((double) (x + 1)) * (double) section_width /
		    (double) out_width;
		sy = sy1;
		do
		  {
		      double yportion;
		      if (floor2 (sy) == floor2 (sy1))
			{
			    yportion = 1.0 - (sy - floor2 (sy));
			    if (yportion > sy2 - sy1)
			      {
				  yportion = sy2 - sy1;
			      }
			    sy = floor2 (sy);
			}
		      else if (sy == floor2 (sy2))
			{
			    yportion = sy2 - floor2 (sy2);
			}
		      else
			{
			    yportion = 1.0;
			}
		      sx = sx1;
		      do
			{
			    double xportion;
			    double pcontribution;
			    if (floor2 (sx) == floor2 (sx1))
			      {
				  xportion = 1.0 - (sx - floor2 (sx));
				  if (xportion > sx2 - sx1)
				    {
					xportion = sx2 - sx1;
				    }
				  sx = floor2 (sx);
			      }
			    else if (sx == floor2 (sx2))
			      {
				  xportion = sx2 - floor2 (sx2);
			      }
			    else
			      {
				  xportion = 1.0;
			      }
			    pcontribution = xportion * yportion;
			    /* retrieving the origin pixel */
			    if (pixel_type == RL2_PIXEL_RGB)
				p_in =
				    inbuf +
				    ((unsigned int) sy * section_width * 3);
			    else
				p_in =
				    inbuf + ((unsigned int) sy * section_width);
			    if (pixel_type == RL2_PIXEL_PALETTE)
			      {
				  rl2PrivPalettePtr plt =
				      (rl2PrivPalettePtr) palette;
				  unsigned char index;
				  p_in += (unsigned int) sx;
				  index = *p_in;
				  if (index < plt->nEntries)
				    {
					/* resolving the Palette color by index */
					rl2PrivPaletteEntryPtr entry =
					    plt->entries + index;
					r = entry->red;
					g = entry->green;
					b = entry->red;
				    }
				  else
				    {
					r = 0;
					g = 0;
					b = 0;
				    }
			      }
			    else
			      {
				  p_in += (unsigned int) sx;
				  if (*p_in == 1)
				    {
					/* black monochrome pixel */
					r = 0;
					g = 0;
					b = 0;
				    }
				  else
				    {
					/* white monochrome pixel */
					r = 255;
					g = 255;
					b = 255;
				    }
			      }
			    red += r * pcontribution;
			    green += g * pcontribution;
			    blue += b * pcontribution;
			    spixels += xportion * yportion;
			    sx += 1.0;
			}
		      while (sx < sx2);
		      sy += 1.0;
		  }
		while (sy < sy2);
		if (spixels != 0.0)
		  {
		      red /= spixels;
		      green /= spixels;
		      blue /= spixels;
		  }
		if (red > 255.0)
		    red = 255.0;
		if (green > 255.0)
		    green = 255.0;
		if (blue > 255.0)
		    blue = 255.0;
		/* setting the destination pixel */
		if (pixel_type == RL2_PIXEL_PALETTE)
		    p_out = outbuf + (y * out_width * 3);
		else
		    p_out = outbuf + (y * out_width);
		if (pixel_type == RL2_PIXEL_PALETTE)
		  {
		      p_out += x * 3;
		      *p_out++ = (unsigned char) red;
		      *p_out++ = (unsigned char) green;
		      *p_out = (unsigned char) blue;
		  }
		else
		  {
		      if (red <= 224.0)
			{
			    p_out += x;
			    if (red < *p_out)
				*p_out = (unsigned char) red;
			}
		  }
	    }
      }
}

static int
copy_124_tile (unsigned char pixel_type, const unsigned char *outbuf,
	       unsigned char **tilebuf,
	       int *tilebuf_sz, unsigned char **tilemask, int *tilemask_sz,
	       unsigned int row, unsigned int col, unsigned int out_width,
	       unsigned int out_height, unsigned int tileWidth,
	       unsigned int tileHeight, rl2PixelPtr no_data)
{
/* allocating and initializing the resized tile buffers */
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_sz = 0;
    int has_mask = 0;
    unsigned int x;
    unsigned int y;
    const unsigned char *p_inp;
    unsigned char *p_outp;

    if (pixel_type == RL2_PIXEL_RGB)
	pixels_sz = tileWidth * tileHeight * 3;
    else
	pixels_sz = tileWidth * tileHeight;
    pixels = malloc (pixels_sz);
    if (pixels == NULL)
	goto error;

    if (pixel_type == RL2_PIXEL_RGB)
	rl2_prime_void_tile (pixels, tileWidth, tileHeight, RL2_SAMPLE_UINT8,
			     3, no_data);
    else
	rl2_prime_void_tile (pixels, tileWidth, tileHeight, RL2_SAMPLE_UINT8,
			     1, no_data);

    if (row + tileHeight > out_height)
	has_mask = 1;
    if (col + tileWidth > out_width)
	has_mask = 1;
    if (has_mask)
      {
	  /* masking the unused portion of this tile */
	  mask_sz = tileWidth * tileHeight;
	  mask = malloc (mask_sz);
	  if (mask == NULL)
	      goto error;
	  memset (mask, 0, mask_sz);
	  for (y = 0; y < tileHeight; y++)
	    {
		unsigned int y_in = y + row;
		if (y_in >= out_height)
		    continue;
		for (x = 0; x < tileWidth; x++)
		  {
		      unsigned int x_in = x + col;
		      if (x_in >= out_width)
			  continue;
		      p_outp = mask + (y * tileWidth);
		      p_outp += x;
		      *p_outp = 1;
		  }
	    }
      }

    for (y = 0; y < tileHeight; y++)
      {
	  unsigned int y_in = y + row;
	  if (y_in >= out_height)
	      continue;
	  for (x = 0; x < tileWidth; x++)
	    {
		unsigned int x_in = x + col;
		if (x_in >= out_width)
		    continue;
		if (pixel_type == RL2_PIXEL_RGB)
		  {
		      p_inp = outbuf + (y_in * out_width * 3);
		      p_inp += x_in * 3;
		      p_outp = pixels + (y * tileWidth * 3);
		      p_outp += x * 3;
		      *p_outp++ = *p_inp++;
		      *p_outp++ = *p_inp++;
		      *p_outp = *p_inp;
		  }
		else
		  {
		      p_inp = outbuf + (y_in * out_width);
		      p_inp += x_in;
		      p_outp = pixels + (y * tileWidth);
		      p_outp += x;
		      *p_outp = *p_inp;
		  }
	    }
      }

    *tilebuf = pixels;
    *tilebuf_sz = pixels_sz;
    *tilemask = mask;
    *tilemask_sz = mask_sz;
    return 1;

  error:
    if (pixels != NULL)
	free (pixels);
    return 0;
}

static int
do_build_124_bit_section_pyramid (sqlite3 * handle, int max_threads,
				  const char *coverage, int mixed_resolutions,
				  sqlite3_int64 section_id,
				  unsigned char sample_type,
				  unsigned char pixel_type,
				  unsigned char num_samples, int srid,
				  unsigned int tileWidth,
				  unsigned int tileHeight, unsigned char bgRed,
				  unsigned char bgGreen, unsigned char bgBlue)
{
/* attempting to (re)build a 1,2,4-bit section pyramid from scratch */
    double base_res_x;
    double base_res_y;
    unsigned int sect_width;
    unsigned int sect_height;
    int id_level = 0;
    int scale;
    double x_res;
    double y_res;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned int row;
    unsigned int col;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    rl2PalettePtr palette = NULL;
    rl2PixelPtr no_data = NULL;
    rl2PixelPtr nd = NULL;
    unsigned char *inbuf = NULL;
    int inbuf_size = 0;
    unsigned char *outbuf = NULL;
    int outbuf_sz = 0;
    unsigned char out_pixel_type;
    unsigned char out_bands;
    unsigned char *tilebuf = NULL;
    int tilebuf_sz = 0;
    unsigned char *tilemask = NULL;
    int tilemask_sz = 0;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    unsigned int x;
    unsigned int y;
    unsigned char *p_out;

    if (!get_section_infos
	(handle, coverage, section_id, &sect_width, &sect_height, &minx,
	 &miny, &maxx, &maxy, &palette, &no_data))
	goto error;

    if (mixed_resolutions)
      {
	  if (!find_section_base_resolution
	      (handle, coverage, section_id, &base_res_x, &base_res_y))
	      goto error;
      }
    else
      {
	  if (!find_base_resolution
	      (handle, coverage, &base_res_x, &base_res_y))
	      goto error;
      }
    if (!get_section_raw_raster_data
	(handle, max_threads, coverage, section_id, sect_width, sect_height,
	 sample_type, pixel_type, num_samples, minx, maxy, base_res_x,
	 base_res_y, &inbuf, &inbuf_size, palette, no_data))
	goto error;

    if (!prepare_section_pyramid_stmts
	(handle, coverage, mixed_resolutions, &stmt_rd, &stmt_levl, &stmt_tils,
	 &stmt_data))
	goto error;

    id_level = 1;
    scale = 2;
    x_res = base_res_x * 2.0;
    y_res = base_res_y * 2.0;
    while (1)
      {
	  /* looping on Pyramid levels */
	  double t_minx;
	  double t_miny;
	  double t_maxx;
	  double t_maxy;
	  unsigned int out_width = sect_width / scale;
	  unsigned int out_height = sect_height / scale;
	  rl2RasterPtr raster = NULL;

	  if (pixel_type == RL2_PIXEL_MONOCHROME)
	    {
		out_pixel_type = RL2_PIXEL_GRAYSCALE;
		out_bands = 1;
		outbuf_sz = out_width * out_height;
		outbuf = malloc (outbuf_sz);
		if (outbuf == NULL)
		    goto error;
		p_out = outbuf;
		for (y = 0; y < out_height; y++)
		  {
		      /* priming the background color */
		      for (x = 0; x < out_width; x++)
			  *p_out++ = bgRed;
		  }
	    }
	  else
	    {
		out_pixel_type = RL2_PIXEL_RGB;
		out_bands = 3;
		outbuf_sz = out_width * out_height * 3;
		outbuf = malloc (outbuf_sz);
		if (outbuf == NULL)
		    goto error;
		p_out = outbuf;
		for (y = 0; y < out_height; y++)
		  {
		      /* priming the background color */
		      for (x = 0; x < out_width; x++)
			{
			    *p_out++ = bgRed;
			    *p_out++ = bgGreen;
			    *p_out++ = bgBlue;
			}
		  }
	    }
	  t_maxy = maxy;
	  raster_tile_124_rescaled (outbuf, pixel_type, inbuf,
				    sect_width, sect_height, out_width,
				    out_height, palette);

	  if (mixed_resolutions)
	    {
		if (!do_insert_pyramid_section_levels
		    (handle, section_id, id_level, x_res, y_res, stmt_levl))
		    goto error;
	    }
	  else
	    {
		if (!do_insert_pyramid_levels
		    (handle, id_level, x_res, y_res, stmt_levl))
		    goto error;
	    }

	  for (row = 0; row < out_height; row += tileHeight)
	    {
		t_minx = minx;
		t_miny = t_maxy - (tileHeight * y_res);
		for (col = 0; col < out_width; col += tileWidth)
		  {
		      if (pixel_type == RL2_PIXEL_MONOCHROME)
			{
			    if (no_data == NULL)
				nd = NULL;
			    else
			      {
				  /* creating a NO-DATA pixel */
				  nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
							 RL2_PIXEL_GRAYSCALE,
							 1);
				  rl2_set_pixel_sample_uint8 (nd,
							      RL2_GRAYSCALE_BAND,
							      bgRed);
			      }
			}
		      else
			{
			    if (no_data == NULL)
				nd = NULL;
			    else
			      {
				  /* converting the NO-DATA pixel */
				  nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
							 RL2_PIXEL_RGB, 3);
				  rl2_set_pixel_sample_uint8 (nd, RL2_RED_BAND,
							      bgRed);
				  rl2_set_pixel_sample_uint8 (nd,
							      RL2_GREEN_BAND,
							      bgGreen);
				  rl2_set_pixel_sample_uint8 (nd, RL2_BLUE_BAND,
							      bgBlue);
			      }
			}
		      t_maxx = t_minx + (tileWidth * x_res);
		      if (!copy_124_tile
			  (out_pixel_type, outbuf, &tilebuf,
			   &tilebuf_sz, &tilemask, &tilemask_sz, row, col,
			   out_width, out_height, tileWidth, tileHeight,
			   no_data))
			{
			    fprintf (stderr,
				     "ERROR: unable to extract a Pyramid Tile\n");
			    goto error;
			}

		      raster =
			  rl2_create_raster (tileWidth, tileHeight,
					     RL2_SAMPLE_UINT8, out_pixel_type,
					     out_bands, tilebuf, tilebuf_sz,
					     NULL, tilemask, tilemask_sz, nd);
		      tilebuf = NULL;
		      tilemask = NULL;
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "ERROR: unable to create a Pyramid Tile\n");
			    goto error;
			}
		      if (rl2_raster_encode
			  (raster, RL2_COMPRESSION_PNG, &blob_odd, &blob_odd_sz,
			   &blob_even, &blob_even_sz, 100, 1) != RL2_OK)
			{
			    fprintf (stderr,
				     "ERROR: unable to encode a Pyramid tile\n");
			    goto error;
			}

		      /* INSERTing the tile */
		      if (!do_insert_pyramid_tile
			  (handle, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, id_level, section_id, srid, t_minx,
			   t_miny, t_maxx, t_maxy, stmt_tils, stmt_data))
			  goto error;
		      rl2_destroy_raster (raster);
		      t_minx += (tileWidth * x_res);
		  }
		t_maxy -= (tileHeight * y_res);
	    }

	  free (outbuf);
	  if (out_width < tileWidth && out_height < tileHeight)
	      break;
	  x_res *= 2.0;
	  y_res *= 2.0;
	  scale *= 2;
	  id_level++;
      }

    free (inbuf);
    sqlite3_finalize (stmt_rd);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 1;

  error:
    if (outbuf != NULL)
	free (outbuf);
    if (tilebuf != NULL)
	free (tilebuf);
    if (tilemask != NULL)
	free (tilemask);
    if (inbuf != NULL)
	free (inbuf);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 0;
}

static int
do_build_palette_section_pyramid (sqlite3 * handle, int max_threads,
				  const char *coverage, int mixed_resolutions,
				  sqlite3_int64 section_id, int srid,
				  unsigned int tileWidth,
				  unsigned int tileHeight, unsigned char bgRed,
				  unsigned char bgGreen, unsigned char bgBlue)
{
/* attempting to (re)build a Palette section pyramid from scratch */
    double base_res_x;
    double base_res_y;
    unsigned int sect_width;
    unsigned int sect_height;
    int id_level = 0;
    int scale;
    double x_res;
    double y_res;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned int row;
    unsigned int col;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    rl2PalettePtr palette = NULL;
    rl2PixelPtr no_data = NULL;
    rl2PixelPtr nd = NULL;
    unsigned char *inbuf = NULL;
    int inbuf_size = 0;
    unsigned char *outbuf = NULL;
    int outbuf_sz = 0;
    unsigned char out_pixel_type;
    unsigned char out_bands;
    unsigned char *tilebuf = NULL;
    int tilebuf_sz = 0;
    unsigned char *tilemask = NULL;
    int tilemask_sz = 0;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    unsigned int x;
    unsigned int y;
    unsigned char *p_out;

    if (!get_section_infos
	(handle, coverage, section_id, &sect_width, &sect_height, &minx,
	 &miny, &maxx, &maxy, &palette, &no_data))
	goto error;
    if (palette == NULL)
	goto error;

    if (mixed_resolutions)
      {
	  if (!find_section_base_resolution
	      (handle, coverage, section_id, &base_res_x, &base_res_y))
	      goto error;
      }
    else
      {
	  if (!find_base_resolution
	      (handle, coverage, &base_res_x, &base_res_y))
	      goto error;
      }
    if (!get_section_raw_raster_data
	(handle, max_threads, coverage, section_id, sect_width, sect_height,
	 RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1, minx, maxy, base_res_x,
	 base_res_y, &inbuf, &inbuf_size, palette, no_data))
	goto error;

    if (!prepare_section_pyramid_stmts
	(handle, coverage, mixed_resolutions, &stmt_rd, &stmt_levl, &stmt_tils,
	 &stmt_data))
	goto error;

    id_level = 1;
    scale = 2;
    x_res = base_res_x * 2.0;
    y_res = base_res_y * 2.0;
    while (1)
      {
	  /* looping on Pyramid levels */
	  double t_minx;
	  double t_miny;
	  double t_maxx;
	  double t_maxy;
	  unsigned int out_width = sect_width / scale;
	  unsigned int out_height = sect_height / scale;
	  rl2RasterPtr raster = NULL;

	  out_pixel_type = RL2_PIXEL_RGB;
	  out_bands = 3;
	  outbuf_sz = out_width * out_height * 3;
	  outbuf = malloc (outbuf_sz);
	  if (outbuf == NULL)
	      goto error;
	  p_out = outbuf;
	  for (y = 0; y < out_height; y++)
	    {
		/* priming the background color */
		for (x = 0; x < out_width; x++)
		  {
		      *p_out++ = bgRed;
		      *p_out++ = bgGreen;
		      *p_out++ = bgBlue;
		  }
	    }
	  t_maxy = maxy;
	  raster_tile_124_rescaled (outbuf, RL2_PIXEL_PALETTE, inbuf,
				    sect_width, sect_height, out_width,
				    out_height, palette);
	  if (mixed_resolutions)
	    {
		if (!do_insert_pyramid_section_levels
		    (handle, section_id, id_level, x_res, y_res, stmt_levl))
		    goto error;
	    }
	  else
	    {
		if (!do_insert_pyramid_levels
		    (handle, id_level, x_res, y_res, stmt_levl))
		    goto error;
	    }

	  for (row = 0; row < out_height; row += tileHeight)
	    {
		t_minx = minx;
		t_miny = t_maxy - (tileHeight * y_res);
		for (col = 0; col < out_width; col += tileWidth)
		  {
		      if (no_data == NULL)
			  nd = NULL;
		      else
			{
			    /* converting the NO-DATA pixel */
			    nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
						   RL2_PIXEL_RGB, 3);
			    rl2_set_pixel_sample_uint8 (nd, RL2_RED_BAND,
							bgRed);
			    rl2_set_pixel_sample_uint8 (nd,
							RL2_GREEN_BAND,
							bgGreen);
			    rl2_set_pixel_sample_uint8 (nd, RL2_BLUE_BAND,
							bgBlue);
			}
		      t_maxx = t_minx + (tileWidth * x_res);
		      if (!copy_124_tile (out_pixel_type, outbuf, &tilebuf,
					  &tilebuf_sz, &tilemask, &tilemask_sz,
					  row, col, out_width, out_height,
					  tileWidth, tileHeight, no_data))
			{
			    fprintf (stderr,
				     "ERROR: unable to extract a Pyramid Tile\n");
			    goto error;
			}

		      raster =
			  rl2_create_raster (tileWidth, tileHeight,
					     RL2_SAMPLE_UINT8, out_pixel_type,
					     out_bands, tilebuf, tilebuf_sz,
					     NULL, tilemask, tilemask_sz, nd);
		      tilebuf = NULL;
		      tilemask = NULL;
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "ERROR: unable to create a Pyramid Tile\n");
			    goto error;
			}
		      if (rl2_raster_encode
			  (raster, RL2_COMPRESSION_PNG, &blob_odd,
			   &blob_odd_sz, &blob_even, &blob_even_sz, 100,
			   1) != RL2_OK)
			{
			    fprintf (stderr,
				     "ERROR: unable to encode a Pyramid tile\n");
			    goto error;
			}

		      /* INSERTing the tile */
		      if (!do_insert_pyramid_tile
			  (handle, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, id_level, section_id, srid, t_minx,
			   t_miny, t_maxx, t_maxy, stmt_tils, stmt_data))
			  goto error;
		      rl2_destroy_raster (raster);
		      t_minx += (tileWidth * x_res);
		  }
		t_maxy -= (tileHeight * y_res);
	    }

	  free (outbuf);
	  if (out_width < tileWidth && out_height < tileHeight)
	      break;
	  x_res *= 4.0;
	  y_res *= 4.0;
	  scale *= 4;
	  id_level++;
      }

    free (inbuf);
    sqlite3_finalize (stmt_rd);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 1;

  error:
    if (outbuf != NULL)
	free (outbuf);
    if (tilebuf != NULL)
	free (tilebuf);
    if (tilemask != NULL)
	free (tilemask);
    if (inbuf != NULL)
	free (inbuf);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return 0;
}

static void
get_background_color (sqlite3 * handle, rl2CoveragePtr coverage,
		      unsigned char *bgRed, unsigned char *bgGreen,
		      unsigned char *bgBlue)
{
/* retrieving the background color for 1,2,4 bit samples */
    sqlite3_stmt *stmt = NULL;
    char *sql;
    int ret;
    rl2PrivPalettePtr plt;
    rl2PalettePtr palette = NULL;
    rl2PrivSamplePtr smp;
    rl2PrivPixelPtr pxl;
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    unsigned char index;
    *bgRed = 255;
    *bgGreen = 255;
    *bgBlue = 255;

    if (cvg == NULL)
	return;
    if (cvg->noData == NULL)
	return;
    pxl = (rl2PrivPixelPtr) (cvg->noData);
    smp = pxl->Samples;
    index = smp->uint8;

    if (cvg->pixelType == RL2_PIXEL_MONOCHROME)
      {
	  if (index == 1)
	    {
		*bgRed = 0;
		*bgGreen = 0;
		*bgBlue = 0;
	    }
	  return;
      }

/* Coverage's palette */
    sql = sqlite3_mprintf ("SELECT palette FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)",
			   cvg->coverageName);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      palette = rl2_deserialize_dbms_palette (blob, blob_sz);
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_info; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (palette == NULL)
	goto error;

    plt = (rl2PrivPalettePtr) palette;
    if (index < plt->nEntries)
      {
	  rl2PrivPaletteEntryPtr rgb = plt->entries + index;
	  *bgRed = rgb->red;
	  *bgGreen = rgb->green;
	  *bgBlue = rgb->blue;
      }
    rl2_destroy_palette (palette);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (palette != NULL)
	rl2_destroy_palette (palette);
}

RL2_DECLARE int
rl2_build_section_pyramid (sqlite3 * handle, int max_threads,
			   const char *coverage, sqlite3_int64 section_id,
			   int forced_rebuild, int verbose)
{
/* (re)building section-level pyramid for a single Section */
    rl2CoveragePtr cvg = NULL;
    rl2PrivCoveragePtr ptrcvg;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    int quality;
    unsigned int tileWidth;
    unsigned int tileHeight;
    int srid;
    int build = 0;
    unsigned char bgRed;
    unsigned char bgGreen;
    unsigned char bgBlue;

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_compression (cvg, &compression, &quality) != RL2_OK)
	goto error;
    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    ptrcvg = (rl2PrivCoveragePtr) cvg;

    if (!forced_rebuild)
      {
	  /* checking if the section pyramid already exists */
	  build = check_section_pyramid (handle, coverage, section_id);
      }
    else
      {
	  /* unconditional rebuilding */
	  build = 1;
      }

    if (build)
      {
	  /* attempting to delete the section pyramid */
	  if (!delete_section_pyramid (handle, coverage, section_id))
	      goto error;
	  /* attempting to (re)build the section pyramid */
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	      || (sample_type == RL2_SAMPLE_1_BIT
		  && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1)
	      || (sample_type == RL2_SAMPLE_2_BIT
		  && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1)
	      || (sample_type == RL2_SAMPLE_4_BIT
		  && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* special case: 1,2,4 bit Pyramid */
		get_background_color (handle, cvg, &bgRed, &bgGreen, &bgBlue);
		if (!do_build_124_bit_section_pyramid
		    (handle, max_threads, coverage, ptrcvg->mixedResolutions,
		     section_id, sample_type, pixel_type, num_bands, srid,
		     tileWidth, tileHeight, bgRed, bgGreen, bgBlue))
		    goto error;
	    }
	  else if (sample_type == RL2_SAMPLE_UINT8
		   && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1)
	    {
		/* special case: 8 bit Palette Pyramid */
		get_background_color (handle, cvg, &bgRed, &bgGreen, &bgBlue);
		if (!do_build_palette_section_pyramid
		    (handle, max_threads, coverage, ptrcvg->mixedResolutions,
		     section_id, srid, tileWidth, tileHeight, bgRed, bgGreen,
		     bgBlue))
		    goto error;
	    }
	  else
	    {
		/* ordinary RGB, Grayscale, MultiBand or DataGrid Pyramid */
		if (!do_build_section_pyramid
		    (handle, coverage, section_id, sample_type, pixel_type,
		     num_bands, compression, ptrcvg->mixedResolutions, quality,
		     srid, tileWidth, tileHeight))
		    goto error;
	    }
	  if (verbose)
	    {
		printf ("  ----------\n");
#if defined(_WIN32) && !defined(__MINGW32__)
		printf
		    ("    Pyramid levels successfully built for Section %I64d\n",
		     section_id);
#else
		printf
		    ("    Pyramid levels successfully built for Section %lld\n",
		     section_id);
#endif
	    }
      }
    rl2_destroy_coverage (cvg);

    return RL2_OK;

  error:
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_build_all_section_pyramids (sqlite3 * handle, int max_threads,
				const char *coverage, int forced_rebuild,
				int verbose)
{
/* (re)building section-level pyramids for a whole Coverage */
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt;
    char *sql;

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT section_id FROM \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 section_id = sqlite3_column_int64 (stmt, 0);
		if (rl2_build_section_pyramid
		    (handle, max_threads, coverage, section_id, forced_rebuild,
		     verbose) != RL2_OK)
		    goto error;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_id; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return RL2_OK;

  error:
    return RL2_ERROR;
}

static int
is_full_mask (const unsigned char *mask, int mask_size)
{
/* testing for a completely transparent mask */
    const unsigned char *p = mask;
    int i;
    int count = 0;
    if (mask == NULL)
	return 0;
    for (i = 0; i < mask_size; i++)
      {
	  if (*p++ == 0)
	      count++;
      }
    if (count == mask_size)
	return 1;
    return 0;
}

RL2_DECLARE int
rl2_build_monolithic_pyramid (sqlite3 * handle, const char *coverage,
			      int virt_levels, int verbose)
{
/* (re)building monolithic pyramid for a whole coverage */
    rl2CoveragePtr cvg = NULL;
    rl2PrivCoveragePtr cov;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    int quality;
    unsigned int tileWidth;
    unsigned int tileHeight;
    int srid;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned int row;
    unsigned int col;
    unsigned int tot_rows;
    double res_x;
    double res_y;
    int factor;
    int resize_factor;
    int id_level = 0;
    unsigned char out_sample_type;
    unsigned char out_pixel_type;
    unsigned char out_num_bands;
    unsigned char out_compression;
    int out_quality;
    rl2PixelPtr no_data = NULL;
    rl2PixelPtr nd = NULL;
    rl2PalettePtr palette = NULL;
    unsigned char *buffer = NULL;
    int buf_size;
    int sample_sz;
    unsigned char *mask = NULL;
    int mask_size;
    rl2RasterPtr raster = NULL;
    unsigned char *blob_odd = NULL;
    unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    sqlite3_stmt *stmt_geo = NULL;
    sqlite3_stmt *stmt_rd = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_data = NULL;
    char *xtiles;
    char *xxtiles;
    int ret;
    char *sql;
    double tile_minx;
    double tile_miny;
    double tile_maxx;
    double tile_maxy;
    double end_x;
    double end_y;
    int stop = 0;

/* preparing the "tiles" SQL query */
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = rl2_double_quoted_sql (xtiles);
    sql =
	sqlite3_mprintf
	("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry) FROM \"%s\" "
	 "WHERE pyramid_level = ? AND ROWID IN (SELECT ROWID FROM SpatialIndex "
	 "WHERE f_table_name = %Q AND search_frame = BuildMBR(?, ?, ?, ?)) "
	 "ORDER BY ST_Area(geometry)", xxtiles, xtiles);
    sqlite3_free (xtiles);
    free (xxtiles);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_geo, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT monolithic RGBA tiles SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	goto error;
    cov = (rl2PrivCoveragePtr) cvg;
    if (cov->mixedResolutions)
      {
	  fprintf (stderr,
		   "MixedResolutions forbids a Monolithic Pyramid on \"%s\"\n",
		   cov->coverageName);
	  goto error;
      }

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_compression (cvg, &compression, &quality) != RL2_OK)
	goto error;
    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (!get_coverage_extent (handle, coverage, &minx, &miny, &maxx, &maxy))
	goto error;
    no_data = rl2_get_coverage_no_data (cvg);
    palette = rl2_get_dbms_palette (handle, coverage);
    if (!prepare_section_pyramid_stmts
	(handle, coverage, 0, &stmt_rd, &stmt_levl, &stmt_tils, &stmt_data))
	goto error;

    if (sample_type == RL2_SAMPLE_1_BIT
	&& pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
      {
	  /* monochrome: output colorspace is Grayscale compression PNG */
	  out_sample_type = RL2_SAMPLE_UINT8;
	  out_pixel_type = RL2_PIXEL_GRAYSCALE;
	  out_num_bands = 1;
	  out_compression = RL2_COMPRESSION_PNG;
	  out_quality = 100;
	  virt_levels = 1;
      }
    else if ((sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1)
	     || (sample_type == RL2_SAMPLE_2_BIT
		 && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1)
	     || (sample_type == RL2_SAMPLE_4_BIT))
      {
	  /* palette 1,2,4: output colorspace is RGB compression PNG */
	  out_sample_type = RL2_SAMPLE_UINT8;
	  out_pixel_type = RL2_PIXEL_RGB;
	  out_num_bands = 3;
	  out_compression = RL2_COMPRESSION_PNG;
	  out_quality = 100;
	  virt_levels = 1;
      }
    else if (sample_type == RL2_SAMPLE_UINT8 && pixel_type == RL2_PIXEL_PALETTE
	     && num_bands == 1)
      {
	  /* palette 8: output colorspace is RGB compression PNG */
	  out_sample_type = RL2_SAMPLE_UINT8;
	  out_pixel_type = RL2_PIXEL_RGB;
	  out_num_bands = 3;
	  out_compression = RL2_COMPRESSION_PNG;
	  out_quality = 100;
      }
    else
      {
	  /* unaltered output colorspace */
	  out_sample_type = sample_type;
	  out_pixel_type = pixel_type;
	  out_num_bands = num_bands;
	  out_compression = compression;
	  out_quality = quality;
      }

    /* setting the requested virt_levels */
    switch (virt_levels)
      {
      case 1:			/* separating each physical level */
	  resize_factor = 2;
	  break;
      case 2:			/* one physical + one virtual */
	  resize_factor = 4;
	  break;
      case 3:			/* one physical + two virtuals */
	  resize_factor = 8;
	  break;
      default:
	  resize_factor = 8;
	  break;
      };
    factor = resize_factor;

/* computing output tile buffers */
    switch (out_sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  sample_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  sample_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample_sz = 8;
	  break;
      default:
	  sample_sz = 1;
	  break;
      }
    buf_size = tileWidth * tileHeight * out_num_bands * sample_sz;

/* attempting to delete the section pyramid */
    if (rl2_delete_all_pyramids (handle, coverage) != RL2_OK)
	goto error;

/* attempting to (re)build the section pyramid */
    while (1)
      {
	  /* looping on pyramid levels */
	  tile_maxy = maxy;
	  row = 0;
	  res_x = cov->hResolution * (double) factor;
	  res_y = cov->vResolution * (double) factor;
	  if (!do_insert_pyramid_levels
	      (handle, id_level + 1, res_x, res_y, stmt_levl))
	      goto error;
	  tot_rows = 0;
	  while (1)
	    {
		/* pre-computing the max row-number */
		tile_miny = tile_maxy - ((double) tileHeight * res_y);
		tile_minx = minx;
		if (tile_maxy < miny)
		    break;
		tot_rows++;
		tile_maxy = tile_miny;
	    }

	  tile_maxy = maxy;
	  while (1)
	    {
		/* looping on rows */
		tile_miny = tile_maxy - ((double) tileHeight * res_y);
		tile_minx = minx;
		if (tile_maxy < miny)
		    break;
		col = 0;
		while (1)
		  {
		      /* looping on columns */
		      tile_maxx = tile_minx + ((double) tileWidth * res_x);
		      if (tile_minx > maxx)
			  break;
		      /* allocating output tile buffers */
		      buffer = malloc (buf_size);
		      if (buffer == NULL)
			  goto error;
		      memset (buffer, 0, buf_size);
		      mask_size = tileWidth * tileHeight;
		      mask = malloc (mask_size);
		      if (mask == NULL)
			  goto error;
		      memset (mask, 0, mask_size);
		      end_x = tile_maxx;
		      if (tile_maxx > maxx)
			  end_x = maxx;
		      end_y = tile_miny;
		      if (tile_miny < miny)
			  end_y = miny;

		      if ((sample_type == RL2_SAMPLE_UINT8
			   && pixel_type == RL2_PIXEL_GRAYSCALE
			   && num_bands == 1)
			  || (sample_type == RL2_SAMPLE_UINT8
			      && pixel_type == RL2_PIXEL_RGB && num_bands == 3)
			  || (sample_type == RL2_SAMPLE_UINT8
			      && pixel_type == RL2_PIXEL_PALETTE
			      && num_bands == 1))
			{
			    /* RGB, PALETTE or GRAYSCALE datasource (UINT8) */
			    if (!rescale_monolithic_rgba
				(id_level, tileWidth, tileHeight, resize_factor,
				 res_x, res_y, tile_minx, tile_miny,
				 tile_maxx, tile_maxy, buffer, buf_size, mask,
				 &mask_size, palette, no_data, stmt_geo,
				 stmt_rd))
				goto error;
			    if (mask_size == 0)
				mask = NULL;
			}
		      else if (((sample_type == RL2_SAMPLE_1_BIT
				 || sample_type == RL2_SAMPLE_2_BIT
				 || sample_type == RL2_SAMPLE_4_BIT)
				&& pixel_type == RL2_PIXEL_PALETTE
				&& num_bands == 1)
			       || (sample_type == RL2_SAMPLE_1_BIT
				   && pixel_type == RL2_PIXEL_MONOCHROME
				   && num_bands == 1))
			{
			    /* MONOCHROME and 1,2,4 bit PALETTE */
			    if (!rescale_monolithic_124
				(id_level, tileWidth,
				 tileHeight, resize_factor, pixel_type,
				 res_x, res_y, tile_minx, tile_miny,
				 tile_maxx, tile_maxy, buffer, buf_size, mask,
				 &mask_size, palette, no_data, stmt_geo,
				 stmt_rd))
				goto error;
			    if (mask_size == 0)
				mask = NULL;
			}
		      else if (pixel_type == RL2_PIXEL_MULTIBAND)
			{
			    /* MultiBand */
			    if (!rescale_monolithic_multiband
				(id_level, tileWidth, tileHeight, sample_type,
				 num_bands, resize_factor, res_x, res_y,
				 tile_minx, tile_miny, tile_maxx, tile_maxy,
				 buffer, buf_size, mask, &mask_size, no_data,
				 stmt_geo, stmt_rd))
				goto error;
			    if (mask_size == 0)
				mask = NULL;
			}
		      else if (pixel_type == RL2_PIXEL_DATAGRID)
			{
			    /* DataGrid */
			    if (!rescale_monolithic_datagrid
				(id_level, tileWidth, tileHeight, sample_type,
				 resize_factor, res_x, res_y, tile_minx,
				 tile_miny, tile_maxx, tile_maxy, buffer,
				 buf_size, mask, &mask_size, no_data, stmt_geo,
				 stmt_rd))
				goto error;
			    if (mask_size == 0)
				mask = NULL;
			}
		      else
			{
			    /* unknown */
			    fprintf (stderr,
				     "ERROR: unsupported Monolithic pyramid type\n");
			    goto error;
			}
		      if (is_full_mask (mask, mask_size))
			{
			    /* skipping a completely void tile */
			    free (buffer);
			    free (mask);
			    buffer = NULL;
			    mask = NULL;
			    goto done;
			}

		      if (pixel_type == RL2_PIXEL_MONOCHROME)
			{
			    if (no_data == NULL)
				nd = NULL;
			    else
			      {
				  rl2PrivPixelPtr pxl =
				      (rl2PrivPixelPtr) no_data;
				  rl2PrivSamplePtr sample = pxl->Samples + 0;
				  nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
							 RL2_PIXEL_GRAYSCALE,
							 1);
				  if (sample->uint8 == 0)
				      rl2_set_pixel_sample_uint8 (nd,
								  RL2_GRAYSCALE_BAND,
								  255);
				  else
				      rl2_set_pixel_sample_uint8 (nd,
								  RL2_GRAYSCALE_BAND,
								  0);
			      }
			}
		      else if (pixel_type == RL2_PIXEL_PALETTE)
			{
			    if (no_data == NULL)
				nd = NULL;
			    else
			      {
				  nd = rl2_create_pixel (RL2_SAMPLE_UINT8,
							 RL2_PIXEL_RGB, 3);
				  rl2_set_pixel_sample_uint8 (nd, RL2_RED_BAND,
							      255);
				  rl2_set_pixel_sample_uint8 (nd,
							      RL2_GREEN_BAND,
							      255);
				  rl2_set_pixel_sample_uint8 (nd, RL2_BLUE_BAND,
							      255);
			      }
			}
		      else
			  nd = rl2_clone_pixel (no_data);

		      raster =
			  rl2_create_raster (tileWidth, tileHeight,
					     out_sample_type, out_pixel_type,
					     out_num_bands, buffer, buf_size,
					     NULL, mask, mask_size, nd);
		      buffer = NULL;
		      mask = NULL;
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "ERROR: unable to create a Pyramid Tile\n");
			    goto error;
			}
		      if (rl2_raster_encode
			  (raster, out_compression, &blob_odd, &blob_odd_sz,
			   &blob_even, &blob_even_sz, out_quality, 1) != RL2_OK)
			{
			    fprintf (stderr,
				     "ERROR: unable to encode a Pyramid tile\n");
			    goto error;
			}

		      /* INSERTing the tile */
		      if (!do_insert_pyramid_tile
			  (handle, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, id_level + 1, -1, srid, tile_minx,
			   end_y, end_x, tile_maxy, stmt_tils, stmt_data))
			  goto error;
		      rl2_destroy_raster (raster);
		      raster = NULL;

		    done:
		      tile_minx = tile_maxx;
		      col++;
		  }
		tile_maxy = tile_miny;
		row++;
		if (verbose)
		  {
		      printf ("  ----------\n");
		      printf
			  ("    %s: Monolithic Pyramid Level %d - Row %d of %d  successfully built\n",
			   coverage, id_level + 1, row, tot_rows);
		  }
	    }
	  if (stop)
	      break;
	  if ((minx +
	       ((double) tileWidth * res_x) > maxx)
	      && (maxy - ((double) tileHeight * res_y) < maxy))
	      stop = 1;
	  /* setting the requested virt_levels */
	  switch (virt_levels)
	    {
	    case 1:		/* separating each physical level */
		resize_factor = 2;
		break;
	    case 2:		/* one physical + one virtual */
		resize_factor = 4;
		break;
	    case 3:		/* one physical + two virtuals */
		resize_factor = 8;
		break;
	    };
	  factor *= resize_factor;
	  id_level++;
	  if (palette != NULL)
	    {
		/* destroying an eventual Palette after completing the first level */
		rl2_destroy_palette (palette);
		palette = NULL;
	    }
      }
    sqlite3_finalize (stmt_geo);
    sqlite3_finalize (stmt_rd);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    if (verbose)
      {
	  printf ("  ----------\n");
	  printf ("    Monolithic Pyramid levels successfully built for: %s\n",
		  coverage);
      }
    free (buffer);
    free (mask);
    rl2_destroy_coverage (cvg);

    return RL2_OK;

  error:
    if (stmt_geo != NULL)
	sqlite3_finalize (stmt_geo);
    if (stmt_rd != NULL)
	sqlite3_finalize (stmt_rd);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (buffer != NULL)
	free (buffer);
    if (mask != NULL)
	free (mask);
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_delete_all_pyramids (sqlite3 * handle, const char *coverage)
{
/* deleting all pyramids for a whole Coverage */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    char *err_msg = NULL;
    int mixed_resolutions =
	rl2_is_mixed_resolutions_coverage (handle, coverage);

    if (mixed_resolutions < 0)
	return RL2_ERROR;

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("DELETE FROM \"%s\" WHERE pyramid_level > 0", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE FROM \"%s_tiles\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  return RL2_ERROR;
      }

    if (mixed_resolutions)
      {
	  /* Mixed Resolution Coverage */
	  table = sqlite3_mprintf ("%s_section_levels", coverage);
	  xtable = rl2_double_quoted_sql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf ("DELETE FROM \"%s\" WHERE pyramid_level > 0",
			       xtable);
	  free (xtable);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr,
			 "DELETE FROM \"%s_section_levels\" error: %s\n",
			 coverage, err_msg);
		sqlite3_free (err_msg);
		return RL2_ERROR;
	    }
      }
    else
      {
	  /* ordinary Coverage */
	  table = sqlite3_mprintf ("%s_levels", coverage);
	  xtable = rl2_double_quoted_sql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf ("DELETE FROM \"%s\" WHERE pyramid_level > 0",
			       xtable);
	  free (xtable);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "DELETE FROM \"%s_levels\" error: %s\n",
			 coverage, err_msg);
		sqlite3_free (err_msg);
		return RL2_ERROR;
	    }
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_delete_section_pyramid (sqlite3 * handle, const char *coverage,
			    sqlite3_int64 section_id)
{
/* deleting section-level pyramid for a single Section */
    if (!delete_section_pyramid (handle, coverage, section_id))
	return RL2_ERROR;
    return RL2_OK;
}
