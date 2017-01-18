/*

 rl2import -- DBMS import functions

 version 0.1, 2014 January 9

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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <time.h>

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

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

static void
destroyAuxImporterTile (rl2AuxImporterTilePtr tile)
{
/* destroying an AuxImporter Tile */
    if (tile == NULL)
	return;
    if (tile->opaque_thread_id != NULL)
	free (tile->opaque_thread_id);
    if (tile->raster != NULL)
	rl2_destroy_raster (tile->raster);
    if (tile->blob_odd != NULL)
	free (tile->blob_odd);
    if (tile->blob_even != NULL)
	free (tile->blob_even);
    free (tile);
}

static void
doAuxImporterTileCleanup (rl2AuxImporterTilePtr tile)
{
/* AuxTile cleanup */
    if (tile == NULL)
	return;
    tile->blob_odd = NULL;
    tile->blob_even = NULL;
    rl2_destroy_raster (tile->raster);
    tile->raster = NULL;
}

static void
addTile2AuxImporter (rl2AuxImporterPtr aux, unsigned int row,
		     unsigned int col, double minx, double maxy)
{
/* adding a Tile to some AuxImporter container */
    rl2AuxImporterTilePtr tile;
    if (aux == NULL)
	return;

    tile = malloc (sizeof (rl2AuxImporterTile));
    tile->opaque_thread_id = NULL;
    tile->mother = aux;
    tile->raster = NULL;
    tile->row = row;
    tile->col = col;
    tile->minx = minx;
    tile->maxx = minx + ((double) (aux->tile_w) * aux->res_x);
    if (tile->maxx > aux->maxx)
	tile->maxx = aux->maxx;
    tile->maxy = maxy;
    tile->miny = tile->maxy - ((double) (aux->tile_h) * aux->res_y);
    if (tile->miny < aux->miny)
	tile->miny = aux->miny;
    tile->retcode = RL2_ERROR;
    tile->blob_odd = NULL;
    tile->blob_even = NULL;
    tile->blob_odd_sz = 0;
    tile->blob_even_sz = 0;
    tile->next = NULL;
/* appending to the double linked list */
    if (aux->first == NULL)
	aux->first = tile;
    if (aux->last != NULL)
	aux->last->next = tile;
    aux->last = tile;
}

static rl2AuxImporterPtr
createAuxImporter (rl2PrivCoveragePtr coverage, int srid, double maxx,
		   double miny, unsigned int tile_w, unsigned int tile_h,
		   double res_x, double res_y, unsigned char origin_type,
		   const void *origin, unsigned char forced_conversion,
		   int verbose, unsigned char compression, int quality)
{
/* creating an AuxImporter container */
    rl2AuxImporterPtr aux = malloc (sizeof (rl2AuxImporter));
    aux->coverage = coverage;
    aux->srid = srid;
    aux->maxx = maxx;
    aux->miny = miny;
    aux->tile_w = tile_w;
    aux->tile_h = tile_h;
    aux->res_x = res_x;
    aux->res_y = res_y;
    aux->origin_type = origin_type;
    aux->origin = origin;
    aux->forced_conversion = forced_conversion;
    aux->verbose = verbose;
    aux->compression = compression;
    aux->quality = quality;
    aux->first = NULL;
    aux->last = NULL;
    return aux;
}

static void
destroyAuxImporter (rl2AuxImporterPtr aux)
{
/* destroying an AuxImporter container */
    rl2AuxImporterTilePtr tile;
    rl2AuxImporterTilePtr tile_n;
    if (aux == NULL)
	return;
    tile = aux->first;
    while (tile != NULL)
      {
	  tile_n = tile->next;
	  destroyAuxImporterTile (tile);
	  tile = tile_n;
      }
    free (aux);
}

static char *
formatFloat (double value)
{
/* nicely formatting a float value */
    int i;
    int len;
    char *fmt = sqlite3_mprintf ("%1.24f", value);
    len = strlen (fmt);
    for (i = len - 1; i >= 0; i--)
      {
	  if (fmt[i] == '0')
	      fmt[i] = '\0';
	  else
	      break;
      }
    len = strlen (fmt);
    if (fmt[len - 1] == '.')
	fmt[len] = '0';
    return fmt;
}

static char *
formatFloat2 (double value)
{
/* nicely formatting a float value (2 decimals) */
    char *fmt = sqlite3_mprintf ("%1.2f", value);
    return fmt;
}

static char *
formatFloat6 (double value)
{
/* nicely formatting a float value (6 decimals) */
    char *fmt = sqlite3_mprintf ("%1.2f", value);
    return fmt;
}

static char *
formatLong (double value)
{
/* nicely formatting a Longitude */
    if (value >= -180.0 && value <= 180.0)
	return formatFloat6 (value);
    return formatFloat2 (value);
}

static char *
formatLat (double value)
{
/* nicely formatting a Latitude */
    if (value >= -90.0 && value <= 90.0)
	return formatFloat6 (value);
    return formatFloat2 (value);
}

static int
do_insert_tile (sqlite3 * handle, unsigned char *blob_odd, int blob_odd_sz,
		unsigned char *blob_even, int blob_even_sz,
		sqlite3_int64 section_id, int srid, double tile_minx,
		double tile_miny, double tile_maxx, double tile_maxy,
		rl2PalettePtr aux_palette, rl2PixelPtr no_data,
		sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_data,
		rl2RasterStatisticsPtr section_stats)
{
/* INSERTing the tile */
    int ret;
    sqlite3_int64 tile_id;
    rl2RasterStatisticsPtr stats = NULL;

    stats = rl2_get_raster_statistics
	(blob_odd, blob_odd_sz, blob_even, blob_even_sz, aux_palette, no_data);
    if (stats == NULL)
	goto error;
    rl2_aggregate_raster_statistics (stats, section_stats);
    sqlite3_reset (stmt_tils);
    sqlite3_clear_bindings (stmt_tils);
    sqlite3_bind_int64 (stmt_tils, 1, section_id);
    sqlite3_bind_double (stmt_tils, 2, tile_minx);
    sqlite3_bind_double (stmt_tils, 3, tile_miny);
    sqlite3_bind_double (stmt_tils, 4, tile_maxx);
    sqlite3_bind_double (stmt_tils, 5, tile_maxy);
    sqlite3_bind_int (stmt_tils, 6, srid);
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
    rl2_destroy_raster_statistics (stats);
    return 1;
  error:
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    return 0;
}

RL2_PRIVATE void
compute_aggregate_sq_diff (rl2RasterStatisticsPtr section_stats)
{
/* updating aggregate sum_sq_diff */
    int ib;
    rl2PoolVariancePtr pV;
    rl2PrivBandStatisticsPtr st_band;
    rl2PrivRasterStatisticsPtr st = (rl2PrivRasterStatisticsPtr) section_stats;
    if (st == NULL)
	return;

    for (ib = 0; ib < st->nBands; ib++)
      {
	  double sum_var = 0.0;
	  st_band = st->band_stats + ib;
	  pV = st_band->first;
	  while (pV != NULL)
	    {
		sum_var += (pV->count - 1.0) * pV->variance;
		pV = pV->next;
	    }
	  st_band->sum_sq_diff = sum_var;
      }
}

static void
do_get_tile (rl2AuxImporterTilePtr tile)
{
/* loading data required by an AuxImporter Tile */
    rl2AsciiGridOriginPtr ascii_grid_origin = NULL;
    rl2TiffOriginPtr tiff_origin;
    rl2RasterPtr raster_origin;
    rl2AuxImporterPtr aux;
    if (tile == NULL)
	return;

    aux = tile->mother;
    switch (aux->origin_type)
      {
      case RL2_ORIGIN_ASCII_GRID:
	  ascii_grid_origin = (rl2AsciiGridOriginPtr) (aux->origin);
	  tile->raster =
	      rl2_get_tile_from_ascii_grid_origin ((rl2CoveragePtr)
						   (aux->coverage),
						   ascii_grid_origin,
						   tile->row, tile->col,
						   aux->verbose);
	  break;
      case RL2_ORIGIN_JPEG:
	  raster_origin = (rl2RasterPtr) (aux->origin);
	  tile->raster =
	      rl2_get_tile_from_jpeg_origin ((rl2CoveragePtr) (aux->coverage),
					     raster_origin, tile->row,
					     tile->col,
					     aux->forced_conversion,
					     aux->verbose);
	  break;
      case RL2_ORIGIN_JPEG2000:
	  raster_origin = (rl2RasterPtr) (aux->origin);
	  tile->raster =
	      rl2_get_tile_from_jpeg2000_origin ((rl2CoveragePtr)
						 (aux->coverage),
						 raster_origin, tile->row,
						 tile->col,
						 aux->forced_conversion,
						 aux->verbose);
	  break;
      case RL2_ORIGIN_TIFF:
	  tiff_origin = (rl2TiffOriginPtr) (aux->origin);
	  tile->raster =
	      rl2_get_tile_from_tiff_origin ((rl2CoveragePtr) (aux->coverage),
					     tiff_origin, tile->row,
					     tile->col, aux->srid,
					     aux->verbose);
	  break;
      case RL2_ORIGIN_RAW:
	  raster_origin = (rl2RasterPtr) (aux->origin);
	  tile->raster =
	      rl2_get_tile_from_raw_pixels ((rl2CoveragePtr) (aux->coverage),
					    raster_origin, tile->row,
					    tile->col);
	  break;
      };
}

static void
do_encode_tile (rl2AuxImporterTilePtr tile)
{
/* servicising an AuxImporter Tile request */
    rl2AuxImporterPtr aux;
    if (tile == NULL)
	goto error;

    aux = tile->mother;
    if (tile->raster == NULL)
      {
	  fprintf (stderr,
		   "ERROR: unable to get a tile [Row=%d Col=%d]\n",
		   tile->row, tile->col);
	  goto error;
      }
    if (rl2_raster_encode
	(tile->raster, aux->compression, &(tile->blob_odd),
	 &(tile->blob_odd_sz), &(tile->blob_even), &(tile->blob_even_sz),
	 aux->quality, 1) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to encode a tile [Row=%d Col=%d]\n",
		   tile->row, tile->col);
	  goto error;
      }
    tile->retcode = RL2_OK;
    return;

  error:
    doAuxImporterTileCleanup (tile);
    tile->retcode = RL2_ERROR;
}

#ifdef _WIN32
DWORD WINAPI
doRunImportThread (void *arg)
#else
void *
doRunImportThread (void *arg)
#endif
{
/* threaded function: preparing a compressed Tile to be imported */
    rl2AuxImporterTilePtr aux_tile = (rl2AuxImporterTilePtr) arg;
    do_encode_tile (aux_tile);
#ifdef _WIN32
    return 0;
#else
    pthread_exit (NULL);
#endif
}

static void
start_tile_thread (rl2AuxImporterTilePtr aux_tile)
{
/* starting a concurrent thread */
#ifdef _WIN32
    HANDLE thread_handle;
    HANDLE *p_thread;
    DWORD dwThreadId;
    thread_handle =
	CreateThread (NULL, 0, doRunImportThread, aux_tile, 0, &dwThreadId);
    SetThreadPriority (thread_handle, THREAD_PRIORITY_IDLE);
    p_thread = malloc (sizeof (HANDLE));
    *p_thread = thread_handle;
    aux_tile->opaque_thread_id = p_thread;
#else
    pthread_t thread_id;
    pthread_t *p_thread;
    int ok_prior = 0;
    int policy;
    int min_prio;
    pthread_attr_t attr;
    struct sched_param sp;
    pthread_attr_init (&attr);
    if (pthread_attr_setschedpolicy (&attr, SCHED_RR) == 0)
      {
	  /* attempting to set the lowest priority */
	  if (pthread_attr_getschedpolicy (&attr, &policy) == 0)
	    {
		min_prio = sched_get_priority_min (policy);
		sp.sched_priority = min_prio;
		if (pthread_attr_setschedparam (&attr, &sp) == 0)
		  {
		      /* ok, setting the lowest priority */
		      ok_prior = 1;
		      pthread_create (&thread_id, &attr, doRunImportThread,
				      aux_tile);
		  }
	    }
      }
    if (!ok_prior)
      {
	  /* failure: using standard priority */
	  pthread_create (&thread_id, NULL, doRunImportThread, aux_tile);
      }
    p_thread = malloc (sizeof (pthread_t));
    *p_thread = thread_id;
    aux_tile->opaque_thread_id = p_thread;
#endif
}

static int
do_import_ascii_grid (sqlite3 * handle, int max_threads, const char *src_path,
		      rl2CoveragePtr cvg, const char *section, int srid,
		      unsigned int tile_w, unsigned int tile_h,
		      int pyramidize, unsigned char sample_type,
		      unsigned char compression, sqlite3_stmt * stmt_data,
		      sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_sect,
		      sqlite3_stmt * stmt_levl, sqlite3_stmt * stmt_upd_sect,
		      int verbose, int current, int total)
{
/* importing an ASCII Data Grid file */
    int ret;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    rl2AsciiGridOriginPtr origin = NULL;
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
    double tile_minx;
    double tile_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double res_x;
    double res_y;
    double base_res_x;
    double base_res_y;
    double confidence;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;
    time_t start;
    time_t now;
    time_t diff;
    int mins;
    int secs;
    char *xml_summary = NULL;
    rl2AuxImporterPtr aux = NULL;
    rl2AuxImporterTilePtr aux_tile;
    rl2AuxImporterTilePtr *thread_slots = NULL;
    int thread_count;

    time (&start);
    if (rl2_get_coverage_resolution (cvg, &base_res_x, &base_res_y) != RL2_OK)
      {
	  if (verbose)
	      fprintf (stderr, "Unknown Coverage Resolution\n");
	  goto error;
      }
    origin = rl2_create_ascii_grid_origin (src_path, srid, sample_type);
    if (origin == NULL)
      {
	  if (verbose)
	      fprintf (stderr, "Invalid ASCII Grid Origin: %s\n", src_path);
	  goto error;
      }
    xml_summary = rl2_build_ascii_xml_summary (origin);

    printf ("------------------\n");
    if (total > 1)
	printf ("%d/%d) Importing: %s\n", current, total,
		rl2_get_ascii_grid_origin_path (origin));
    else
	printf ("Importing: %s\n", rl2_get_ascii_grid_origin_path (origin));
    ret = rl2_get_ascii_grid_origin_size (origin, &width, &height);
    if (ret == RL2_OK)
	printf ("    Image Size (pixels): %d x %d\n", width, height);
    ret = rl2_get_ascii_grid_origin_srid (origin, &srid);
    if (ret == RL2_OK)
	printf ("                   SRID: %d\n", srid);
    ret = rl2_get_ascii_grid_origin_extent (origin, &minx, &miny, &maxx, &maxy);
    if (ret == RL2_OK)
      {
	  dumb1 = formatLong (minx);
	  dumb2 = formatLat (miny);
	  printf ("       LowerLeft Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
	  dumb1 = formatLong (maxx);
	  dumb2 = formatLat (maxy);
	  printf ("      UpperRight Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }
    ret = rl2_get_ascii_grid_origin_resolution (origin, &res_x, &res_y);
    if (ret == RL2_OK)
      {
	  dumb1 = formatFloat (res_x);
	  dumb2 = formatFloat (res_y);
	  printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }
    if (coverage->mixedResolutions)
      {
	  /* accepting any resolution */
      }
    else if (coverage->strictResolution)
      {
	  /* enforcing Strict Resolution check */
	  if (res_x != coverage->hResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Strict) !!!\n");
		goto error;
	    }
	  if (res_y != coverage->vResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Strict) !!!\n");
		goto error;
	    }
      }
    else
      {
	  /* permissive Resolution check */
	  confidence = coverage->hResolution / 100.0;
	  if (res_x < (coverage->hResolution - confidence)
	      || res_x > (coverage->hResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Permissive) !!!\n");
		goto error;
	    }
	  confidence = coverage->vResolution / 100.0;
	  if (res_y < (coverage->vResolution - confidence)
	      || res_y > (coverage->vResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Permissive) !!!\n");
		goto error;
	    }
      }

    if (rl2_eval_ascii_grid_origin_compatibility (cvg, origin, verbose) !=
	RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/ASCII mismatch\n");
	  goto error;
      }
    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!rl2_do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx,
	 maxy, xml_summary, coverage->sectionPaths, coverage->sectionMD5,
	 coverage->sectionSummary, stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, 1);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (coverage->mixedResolutions)
      {
	  /* multiple resolutions Coverage */
	  if (!rl2_do_insert_section_levels
	      (handle, section_id, res_x, res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }
    else
      {
	  /* single resolution Coverage */
	  if (!rl2_do_insert_levels
	      (handle, base_res_x, base_res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }

/* preparing all Tile Requests */
    aux =
	createAuxImporter (coverage, srid, maxx, miny, tile_w, tile_h, res_x,
			   res_y, RL2_ORIGIN_ASCII_GRID, origin,
			   RL2_CONVERT_NO, verbose, compression, 100);
    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		/* adding a Tile request */
		addTile2AuxImporter (aux, row, col, tile_minx, tile_maxy);
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;
/* prepating the thread_slots stuct */
    thread_slots = malloc (sizeof (rl2AuxImporterTilePtr) * max_threads);
    for (thread_count = 0; thread_count < max_threads; thread_count++)
	*(thread_slots + thread_count) = NULL;
    thread_count = 0;
    aux_tile = aux->first;
    while (aux_tile != NULL)
      {
	  /* processing a Tile request (may be under parallel execution) */
	  if (max_threads > 1)
	    {
		/* adopting a multithreaded strategy */
		do_get_tile (aux_tile);
		*(thread_slots + thread_count) = aux_tile;
		thread_count++;
		start_tile_thread (aux_tile);
		if (thread_count == max_threads || aux_tile->next == NULL)
		  {
		      /* waiting until all child threads exit */
#ifdef _WIN32
		      HANDLE *handles;
		      int z;
		      int cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* counting how many active threads we currently have */
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    cnt++;
			}
		      handles = malloc (sizeof (HANDLE) * cnt);
		      cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* initializing the HANDLEs array */
			    HANDLE *pOpaque;
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    pOpaque = (HANDLE *) (pTile->opaque_thread_id);
			    *(handles + cnt) = *pOpaque;
			    cnt++;
			}
		      WaitForMultipleObjects (cnt, handles, TRUE, INFINITE);
#else
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    pthread_t *pOpaque;
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    pOpaque = (pthread_t *) (pTile->opaque_thread_id);
			    pthread_join (*pOpaque, NULL);
			}
#endif

		      /* all children threads have now finished: resuming the main thread */
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    /* checking for eventual errors */
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    if (pTile->retcode != RL2_OK)
				goto error;
			}
#ifdef _WIN32
		      free (handles);
#endif
		      thread_count = 0;
		      /* we can now continue by inserting all tiles into the DBMS */
		  }
		else
		  {
		      aux_tile = aux_tile->next;
		      continue;
		  }
	    }
	  else
	    {
		/* single thread execution */
		do_get_tile (aux_tile);
		*(thread_slots + 0) = aux_tile;
		do_encode_tile (aux_tile);
		if (aux_tile->retcode != RL2_OK)
		    goto error;
	    }

	  for (thread_count = 0; thread_count < max_threads; thread_count++)
	    {
		/* INSERTing the tile(s) */
		rl2AuxImporterTilePtr pTile = *(thread_slots + thread_count);
		if (pTile == NULL)
		    continue;
		if (!do_insert_tile
		    (handle, pTile->blob_odd, pTile->blob_odd_sz,
		     pTile->blob_even, pTile->blob_even_sz, section_id, srid,
		     pTile->minx, pTile->miny, pTile->maxx, pTile->maxy,
		     NULL, no_data, stmt_tils, stmt_data, section_stats))
		  {
		      pTile->blob_odd = NULL;
		      pTile->blob_even = NULL;
		      goto error;
		  }
		doAuxImporterTileCleanup (pTile);
	    }
	  for (thread_count = 0; thread_count < max_threads; thread_count++)
	      *(thread_slots + thread_count) = NULL;
	  thread_count = 0;
	  aux_tile = aux_tile->next;
      }
    destroyAuxImporter (aux);
    aux = NULL;
    free (thread_slots);
    thread_slots = NULL;

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (rl2_get_defaults_stats(handle, section_stats,coverage->coverageName,srid,(minx+((maxx-minx)/2)),(miny+((maxy-miny)/2))))
     goto error;
    if (!rl2_do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_ascii_grid_origin (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;
    time (&now);
    diff = now - start;
    mins = diff / 60;
    secs = diff - (mins * 60);
    printf (">> Grid successfully imported in: %d mins %02d secs\n", mins,
	    secs);

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  if (coverage_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid
	      (handle, max_threads, coverage_name, section_id, 1,
	       verbose) != RL2_OK)
	    {
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
      }

    return 1;

  error:
    if (aux != NULL)
	destroyAuxImporter (aux);
    if (thread_slots != NULL)
	free (thread_slots);
    if (origin != NULL)
	rl2_destroy_ascii_grid_origin (origin);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

static int
check_jpeg_origin_compatibility (rl2RasterPtr raster, rl2CoveragePtr coverage,
				 unsigned int *width, unsigned int *height,
				 unsigned char *forced_conversion)
{
/* checking if the JPEG and the Coverage are mutually compatible */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    if (rst == NULL || cvg == NULL)
	return 0;
    if (rst->sampleType == RL2_SAMPLE_UINT8
	&& rst->pixelType == RL2_PIXEL_GRAYSCALE && rst->nBands == 1)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_GRAYSCALE_TO_RGB;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT8 && rst->pixelType == RL2_PIXEL_RGB
	&& rst->nBands == 3)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_RGB_TO_GRAYSCALE;
		return 1;
	    }
      }
    return 0;
}

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */

static int
check_jpeg2000_origin_compatibility (rl2RasterPtr raster,
				     rl2CoveragePtr coverage,
				     unsigned int *width,
				     unsigned int *height,
				     unsigned char *forced_conversion)
{
/* checking if the Jpeg2000 and the Coverage are mutually compatible */
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    if (rst == NULL || cvg == NULL)
	return 0;
    if (rst->sampleType == RL2_SAMPLE_UINT8
	&& rst->pixelType == RL2_PIXEL_GRAYSCALE && rst->nBands == 1)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_GRAYSCALE_TO_RGB;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT8 && rst->pixelType == RL2_PIXEL_RGB
	&& rst->nBands == 3)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_RGB_TO_GRAYSCALE;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT8
	&& rst->pixelType == RL2_PIXEL_MULTIBAND && (rst->nBands == 3
						     || rst->nBands == 4))
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_MULTIBAND
	      && cvg->nBands == rst->nBands)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT8
	&& rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_DATAGRID && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT16
	&& rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT16
	      && cvg->pixelType == RL2_PIXEL_DATAGRID && cvg->nBands == 1)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT16
	&& rst->pixelType == RL2_PIXEL_RGB && rst->nBands == 3)
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT16
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
      }
    if (rst->sampleType == RL2_SAMPLE_UINT16
	&& rst->pixelType == RL2_PIXEL_MULTIBAND && (rst->nBands == 3
						     || rst->nBands == 4))
      {
	  if (cvg->sampleType == RL2_SAMPLE_UINT16
	      && cvg->pixelType == RL2_PIXEL_MULTIBAND
	      && cvg->nBands == rst->nBands)
	    {
		*width = rst->width;
		*height = rst->height;
		*forced_conversion = RL2_CONVERT_NO;
		return 1;
	    }
      }
    return 0;
}

#endif /* end OpenJpeg conditional */

RL2_DECLARE char *
rl2_build_worldfile_path (const char *path, const char *suffix)
{
/* building a WorldFile path */
    char *wf_path;
    const char *x = NULL;
    const char *p = path;
    int len;

    if (path == NULL || suffix == NULL)
	return NULL;
    len = strlen (path);
    len -= 1;
    while (*p != '\0')
      {
	  if (*p == '.')
	      x = p;
	  p++;
      }
    if (x > path)
	len = x - path;
    wf_path = malloc (len + strlen (suffix) + 1);
    memcpy (wf_path, path, len);
    strcpy (wf_path + len, suffix);
    return wf_path;
}

static int
read_jgw_worldfile (const char *src_path, double *minx, double *maxy,
		    double *pres_x, double *pres_y)
{
/* attempting to retrieve georeferencing from a JPEG+JGW origin */
    FILE *jgw = NULL;
    double res_x;
    double res_y;
    double x;
    double y;
    char *jgw_path = NULL;

    jgw_path = rl2_build_worldfile_path (src_path, ".jgw");
    if (jgw_path == NULL)
	goto error;
    jgw = fopen (jgw_path, "r");
    free (jgw_path);
    jgw_path = NULL;
    if (jgw == NULL)
      {
	  /* trying the ".jpgw" suffix */
	  jgw_path = rl2_build_worldfile_path (src_path, ".jpgw");
	  if (jgw_path == NULL)
	      goto error;
	  jgw = fopen (jgw_path, "r");
	  free (jgw_path);
      }
    if (jgw == NULL)
      {
	  /* trying the ".wld" suffix */
	  jgw_path = rl2_build_worldfile_path (src_path, ".wld");
	  if (jgw_path == NULL)
	      goto error;
	  jgw = fopen (jgw_path, "r");
	  free (jgw_path);
      }
    if (jgw == NULL)
	goto error;
    if (!parse_worldfile (jgw, &x, &y, &res_x, &res_y))
	goto error;
    fclose (jgw);
    *pres_x = res_x;
    *pres_y = res_y;
    *minx = x;
    *maxy = y;
    return 1;

  error:
    if (jgw_path != NULL)
	free (jgw_path);
    if (jgw != NULL)
	fclose (jgw);
    return 0;
}

static void
write_jgw_worldfile (const char *path, double minx, double maxy, double x_res,
		     double y_res)
{
/* exporting a JGW WorldFile */
    FILE *jgw = NULL;
    char *jgw_path = NULL;

    jgw_path = rl2_build_worldfile_path (path, ".jgw");
    if (jgw_path == NULL)
	goto error;
    jgw = fopen (jgw_path, "w");
    free (jgw_path);
    jgw_path = NULL;
    if (jgw == NULL)
	goto error;
    fprintf (jgw, "        %1.16f\n", x_res);
    fprintf (jgw, "        0.0\n");
    fprintf (jgw, "        0.0\n");
    fprintf (jgw, "        -%1.16f\n", y_res);
    fprintf (jgw, "        %1.16f\n", minx);
    fprintf (jgw, "        %1.16f\n", maxy);
    fclose (jgw);
    return;

  error:
    if (jgw_path != NULL)
	free (jgw_path);
    if (jgw != NULL)
	fclose (jgw);
}

static int
do_import_jpeg_image (sqlite3 * handle, int max_threads, const char *src_path,
		      rl2CoveragePtr cvg, const char *section, int srid,
		      unsigned int tile_w, unsigned int tile_h,
		      int pyramidize, unsigned char sample_type,
		      unsigned char num_bands, unsigned char compression,
		      int quality, sqlite3_stmt * stmt_data,
		      sqlite3_stmt * stmt_tils, sqlite3_stmt * stmt_sect,
		      sqlite3_stmt * stmt_levl, sqlite3_stmt * stmt_upd_sect,
		      int verbose, int current, int total)
{
/* importing a JPEG image file [with optional WorldFile */
    rl2SectionPtr origin = NULL;
    rl2RasterPtr rst_in;
    rl2PrivRasterPtr raster_in;
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
    double tile_minx;
    double tile_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double res_x;
    double res_y;
    int is_georeferenced = 0;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;
    unsigned char forced_conversion = RL2_CONVERT_NO;
    double base_res_x;
    double base_res_y;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    double confidence;
    time_t start;
    time_t now;
    time_t diff;
    int mins;
    int secs;
    char *xml_summary = NULL;
    rl2AuxImporterPtr aux = NULL;
    rl2AuxImporterTilePtr aux_tile;
    rl2AuxImporterTilePtr *thread_slots = NULL;
    int thread_count;

    if (rl2_get_coverage_resolution (cvg, &base_res_x, &base_res_y) != RL2_OK)
      {
	  if (verbose)
	      fprintf (stderr, "Unknown Coverage Resolution\n");
	  goto error;
      }
    origin = rl2_section_from_jpeg (src_path);
    if (origin == NULL)
      {
	  if (verbose)
	      fprintf (stderr, "Invalid JPEG Origin: %s\n", src_path);
	  goto error;
      }
    time (&start);
    rst_in = rl2_get_section_raster (origin);
    if (!check_jpeg_origin_compatibility
	(rst_in, cvg, &width, &height, &forced_conversion))
	goto error;
    if (read_jgw_worldfile (src_path, &minx, &maxy, &res_x, &res_y))
      {
	  /* georeferenced JPEG */
	  maxx = minx + ((double) width * res_x);
	  miny = maxy - ((double) height * res_y);
	  is_georeferenced = 1;
      }
    else
      {
	  /* not georeferenced JPEG */
	  if (srid != -1)
	      goto error;
	  minx = 0.0;
	  miny = 0.0;
	  maxx = width - 1.0;
	  maxy = height - 1.0;
	  res_x = 1.0;
	  res_y = 1.0;
      }
    raster_in = (rl2PrivRasterPtr) rst_in;
    xml_summary =
	rl2_build_jpeg_xml_summary (width, height, raster_in->pixelType,
				    is_georeferenced, res_x, res_y, minx,
				    miny, maxx, maxy);

    printf ("------------------\n");
    if (total > 1)
	printf ("%d/%d) Importing: %s\n", current, total, src_path);
    else
	printf ("Importing: %s\n", src_path);
    printf ("    Image Size (pixels): %d x %d\n", width, height);
    printf ("                   SRID: %d\n", srid);
    dumb1 = formatLong (minx);
    dumb2 = formatLat (miny);
    printf ("       LowerLeft Corner: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    dumb1 = formatLong (maxx);
    dumb2 = formatLat (maxy);
    printf ("      UpperRight Corner: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    dumb1 = formatFloat (res_x);
    dumb2 = formatFloat (res_y);
    printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    if (coverage->Srid != srid)
      {
	  if (verbose)
	      fprintf (stderr, "Mismatching SRID !!!\n");
	  goto error;
      }
    if (coverage->mixedResolutions)
      {
	  /* accepting any resolution */
      }
    else if (coverage->strictResolution)
      {
	  /* enforcing Strict Resolution check */
	  if (res_x != coverage->hResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Strict) !!!\n");
		goto error;
	    }
	  if (res_y != coverage->vResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Strict) !!!\n");
		goto error;
	    }
      }
    else
      {
	  /* permissive Resolution check */
	  confidence = coverage->hResolution / 100.0;
	  if (res_x < (coverage->hResolution - confidence)
	      || res_x > (coverage->hResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Permissive) !!!\n");
		goto error;
	    }
	  confidence = coverage->vResolution / 100.0;
	  if (res_y < (coverage->vResolution - confidence)
	      || res_y > (coverage->vResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution !(Permissive) !!\n");
		goto error;
	    }
      }

    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!rl2_do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx,
	 maxy, xml_summary, coverage->sectionPaths, coverage->sectionMD5,
	 coverage->sectionSummary, stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (coverage->mixedResolutions)
      {
	  /* multiple resolutions Coverage */
	  if (!rl2_do_insert_section_levels
	      (handle, section_id, res_x, res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }
    else
      {
	  /* single resolution Coverage */
	  if (!rl2_do_insert_levels
	      (handle, base_res_x, base_res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }

/* preparing all Tile Requests */
    aux =
	createAuxImporter (coverage, srid, maxx, miny, tile_w, tile_h, res_x,
			   res_y, RL2_ORIGIN_JPEG, rst_in, forced_conversion,
			   verbose, compression, quality);
    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		/* adding a Tile request */
		addTile2AuxImporter (aux, row, col, tile_minx, tile_maxy);
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;
/* prepating the thread_slots stuct */
    thread_slots = malloc (sizeof (rl2AuxImporterTilePtr) * max_threads);
    for (thread_count = 0; thread_count < max_threads; thread_count++)
	*(thread_slots + thread_count) = NULL;
    thread_count = 0;
    aux_tile = aux->first;
    while (aux_tile != NULL)
      {
	  /* processing a Tile request (may be under parallel execution) */
	  if (max_threads > 1)
	    {
		/* adopting a multithreaded strategy */
		do_get_tile (aux_tile);
		*(thread_slots + thread_count) = aux_tile;
		thread_count++;
		start_tile_thread (aux_tile);
		if (thread_count == max_threads || aux_tile->next == NULL)
		  {
		      /* waiting until all child threads exit */
#ifdef _WIN32
		      HANDLE *handles;
		      int z;
		      int cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* counting how many active threads we currently have */
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    cnt++;
			}
		      handles = malloc (sizeof (HANDLE) * cnt);
		      cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* initializing the HANDLEs array */
			    HANDLE *pOpaque;
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    pOpaque = (HANDLE *) (pTile->opaque_thread_id);
			    *(handles + cnt) = *pOpaque;
			    cnt++;
			}
		      WaitForMultipleObjects (cnt, handles, TRUE, INFINITE);
#else
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    pthread_t *pOpaque;
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    pOpaque = (pthread_t *) (pTile->opaque_thread_id);
			    pthread_join (*pOpaque, NULL);
			}
#endif

		      /* all children threads have now finished: resuming the main thread */
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    if (pTile->retcode != RL2_OK)
				goto error;
			}
#ifdef _WIN32
		      free (handles);
#endif
		      thread_count = 0;
		      /* we can now continue by inserting all tiles into the DBMS */
		  }
		else
		  {
		      aux_tile = aux_tile->next;
		      continue;
		  }
	    }
	  else
	    {
		/* single thread execution */
		do_get_tile (aux_tile);
		*(thread_slots + 0) = aux_tile;
		do_encode_tile (aux_tile);
		if (aux_tile->retcode != RL2_OK)
		    goto error;
	    }

	  for (thread_count = 0; thread_count < max_threads; thread_count++)
	    {
		/* INSERTing the tile(s) */
		rl2AuxImporterTilePtr pTile = *(thread_slots + thread_count);
		if (pTile == NULL)
		    continue;
		if (!do_insert_tile
		    (handle, pTile->blob_odd, pTile->blob_odd_sz,
		     pTile->blob_even, pTile->blob_even_sz, section_id, srid,
		     pTile->minx, pTile->miny, pTile->maxx, pTile->maxy,
		     NULL, no_data, stmt_tils, stmt_data, section_stats))
		  {
		      pTile->blob_odd = NULL;
		      pTile->blob_even = NULL;
		      goto error;
		  }
		doAuxImporterTileCleanup (pTile);
	    }
	  for (thread_count = 0; thread_count < max_threads; thread_count++)
	      *(thread_slots + thread_count) = NULL;
	  thread_count = 0;
	  aux_tile = aux_tile->next;
      }
    destroyAuxImporter (aux);
    aux = NULL;
    free (thread_slots);
    thread_slots = NULL;

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (rl2_get_defaults_stats(handle, section_stats,coverage->coverageName,srid,(minx+((maxx-minx)/2)),(miny+((maxy-miny)/2))))
     goto error;
    if (!rl2_do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_section (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;
    time (&now);
    diff = now - start;
    mins = diff / 60;
    secs = diff - (mins * 60);
    printf (">> Image successfully imported in: %d mins %02d secs\n", mins,
	    secs);

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  if (coverage_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid
	      (handle, max_threads, coverage_name, section_id, 1,
	       verbose) != RL2_OK)
	    {
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
      }

    return 1;

  error:
    if (aux != NULL)
	destroyAuxImporter (aux);
    if (thread_slots != NULL)
	free (thread_slots);
    if (origin != NULL)
	rl2_destroy_section (origin);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */

static int
read_j2w_worldfile (const char *src_path, double *minx, double *maxy,
		    double *pres_x, double *pres_y)
{
/* attempting to retrieve georeferencing from a Jpeg2000+J2W origin */
    FILE *j2w = NULL;
    double res_x;
    double res_y;
    double x;
    double y;
    char *j2w_path = NULL;

    j2w_path = rl2_build_worldfile_path (src_path, ".j2w");
    if (j2w_path == NULL)
	goto error;
    j2w = fopen (j2w_path, "r");
    free (j2w_path);
    j2w_path = NULL;
    if (j2w == NULL)
      {
	  /* trying the ".wld" suffix */
	  j2w_path = rl2_build_worldfile_path (src_path, ".wld");
	  if (j2w_path == NULL)
	      goto error;
	  j2w = fopen (j2w_path, "r");
	  free (j2w_path);
      }
    if (j2w == NULL)
	goto error;
    if (!parse_worldfile (j2w, &x, &y, &res_x, &res_y))
	goto error;
    fclose (j2w);
    *pres_x = res_x;
    *pres_y = res_y;
    *minx = x;
    *maxy = y;
    return 1;

  error:
    if (j2w_path != NULL)
	free (j2w_path);
    if (j2w != NULL)
	fclose (j2w);
    return 0;
}

static int
do_import_jpeg2000_image (sqlite3 * handle, int max_threads,
			  const char *src_path, rl2CoveragePtr cvg,
			  const char *section, int srid, unsigned int tile_w,
			  unsigned int tile_h, int pyramidize,
			  unsigned char sample_type, unsigned char num_bands,
			  unsigned char compression, int quality,
			  sqlite3_stmt * stmt_data, sqlite3_stmt * stmt_tils,
			  sqlite3_stmt * stmt_sect, sqlite3_stmt * stmt_levl,
			  sqlite3_stmt * stmt_upd_sect, int verbose,
			  int current, int total)
{
/* importing a Jpeg2000 image file [with optional WorldFile */
    rl2PrivCoveragePtr p_coverage = (rl2PrivCoveragePtr) cvg;
    rl2SectionPtr origin = NULL;
    rl2RasterPtr rst_in;
    rl2PrivRasterPtr raster_in;
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
    double tile_minx;
    double tile_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double res_x;
    double res_y;
    int is_georeferenced = 0;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;
    unsigned char forced_conversion = RL2_CONVERT_NO;
    double base_res_x;
    double base_res_y;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    double confidence;
    time_t start;
    time_t now;
    time_t diff;
    int mins;
    int secs;
    unsigned char xsample_type;
    unsigned char pixel_type;
    unsigned char xnum_bands;
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned char num_levels;
    char *xml_summary = NULL;
    rl2AuxImporterPtr aux = NULL;
    rl2AuxImporterTilePtr aux_tile;
    rl2AuxImporterTilePtr *thread_slots = NULL;
    int thread_count;

    if (rl2_get_coverage_resolution (cvg, &base_res_x, &base_res_y) != RL2_OK)
      {
	  if (verbose)
	      fprintf (stderr, "Unknown Coverage Resolution\n");
	  goto error;
      }
    origin =
	rl2_section_from_jpeg2000 (src_path, p_coverage->sampleType,
				   p_coverage->pixelType, p_coverage->nBands);
    if (origin == NULL)
      {
	  if (verbose)
	      fprintf (stderr, "Invalid Jpeg2000 Origin: %s\n", src_path);
	  goto error;
      }
    if (rl2_get_jpeg2000_infos
	(src_path, &width, &height, &xsample_type, &pixel_type, &xnum_bands,
	 &tile_width, &tile_height, &num_levels) != RL2_OK)
      {
	  if (verbose)
	      fprintf (stderr, "Invalid Jpeg2000 Origin: %s\n", src_path);
	  goto error;
      }
    time (&start);
    rst_in = rl2_get_section_raster (origin);
    if (!check_jpeg2000_origin_compatibility
	(rst_in, cvg, &width, &height, &forced_conversion))
	goto error;
    if (read_j2w_worldfile (src_path, &minx, &maxy, &res_x, &res_y))
      {
	  /* georeferenced Jpeg2000 */
	  maxx = minx + ((double) width * res_x);
	  miny = maxy - ((double) height * res_y);
	  is_georeferenced = 1;
      }
    else
      {
	  /* not georeferenced Jpeg2000 */
	  if (srid != -1)
	      goto error;
	  minx = 0.0;
	  miny = 0.0;
	  maxx = width - 1.0;
	  maxy = height - 1.0;
	  res_x = 1.0;
	  res_y = 1.0;
      }
    raster_in = (rl2PrivRasterPtr) rst_in;
    xml_summary =
	rl2_build_jpeg2000_xml_summary (width, height, raster_in->sampleType,
					raster_in->pixelType,
					raster_in->nBands, is_georeferenced,
					res_x, res_y, minx, miny, maxx, maxy,
					tile_width, tile_height);

    printf ("------------------\n");
    if (total > 1)
	printf ("%d/%d) Importing: %s\n", current, total, src_path);
    else
	printf ("Importing: %s\n", src_path);
    printf ("    Image Size (pixels): %d x %d\n", width, height);
    printf ("                   SRID: %d\n", srid);
    dumb1 = formatLong (minx);
    dumb2 = formatLat (miny);
    printf ("       LowerLeft Corner: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    dumb1 = formatLong (maxx);
    dumb2 = formatLat (maxy);
    printf ("      UpperRight Corner: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    dumb1 = formatFloat (res_x);
    dumb2 = formatFloat (res_y);
    printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
    sqlite3_free (dumb1);
    sqlite3_free (dumb2);
    if (coverage->Srid != srid)
      {
	  if (verbose)
	      fprintf (stderr, "Mismatching SRID !!!\n");
	  goto error;
      }
    if (coverage->mixedResolutions)
      {
	  /* accepting any resolution */
      }
    else if (coverage->strictResolution)
      {
	  /* enforcing Strict Resolution check */
	  if (res_x != coverage->hResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Strict) !!!\n");
		goto error;
	    }
	  if (res_y != coverage->vResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Strict) !!!\n");
		goto error;
	    }
      }
    else
      {
	  /* permissive Resolution check */
	  confidence = coverage->hResolution / 100.0;
	  if (res_x < (coverage->hResolution - confidence)
	      || res_x > (coverage->hResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Permissive) !!!\n");
		goto error;
	    }
	  confidence = coverage->vResolution / 100.0;
	  if (res_y < (coverage->vResolution - confidence)
	      || res_y > (coverage->vResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution !(Permissive) !!\n");
		goto error;
	    }
      }

    no_data = rl2_get_coverage_no_data (cvg);

/* INSERTing the section */
    if (!rl2_do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx,
	 maxy, xml_summary, coverage->sectionPaths, coverage->sectionMD5,
	 coverage->sectionSummary, stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (coverage->mixedResolutions)
      {
	  /* multiple resolutions Coverage */
	  if (!rl2_do_insert_section_levels
	      (handle, section_id, res_x, res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }
    else
      {
	  /* single resolution Coverage */
	  if (!rl2_do_insert_levels
	      (handle, base_res_x, base_res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }

/* preparing all Tile Requests */
    aux =
	createAuxImporter (coverage, srid, maxx, miny, tile_w, tile_h, res_x,
			   res_y, RL2_ORIGIN_JPEG2000, rst_in,
			   forced_conversion, verbose, compression, quality);
    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		/* adding a Tile request */
		addTile2AuxImporter (aux, row, col, tile_minx, tile_maxy);
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;
/* prepating the thread_slots stuct */
    thread_slots = malloc (sizeof (rl2AuxImporterTilePtr) * max_threads);
    for (thread_count = 0; thread_count < max_threads; thread_count++)
	*(thread_slots + thread_count) = NULL;
    thread_count = 0;
    aux_tile = aux->first;
    while (aux_tile != NULL)
      {
	  /* processing a Tile request (may be under parallel execution) */
	  if (max_threads > 1)
	    {
		/* adopting a multithreaded strategy */
		do_get_tile (aux_tile);
		*(thread_slots + thread_count) = aux_tile;
		thread_count++;
		start_tile_thread (aux_tile);
		if (thread_count == max_threads || aux_tile->next == NULL)
		  {
		      /* waiting until all child threads exit */
#ifdef _WIN32
		      HANDLE *handles;
		      int z;
		      int cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* counting how many active threads we currently have */
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    cnt++;
			}
		      handles = malloc (sizeof (HANDLE) * cnt);
		      cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* initializing the HANDLEs array */
			    HANDLE *pOpaque;
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    pOpaque = (HANDLE *) (pTile->opaque_thread_id);
			    *(handles + cnt) = *pOpaque;
			    cnt++;
			}
		      WaitForMultipleObjects (cnt, handles, TRUE, INFINITE);
#else
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    pthread_t *pOpaque;
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    pOpaque = (pthread_t *) (pTile->opaque_thread_id);
			    pthread_join (*pOpaque, NULL);
			}
#endif

		      /* all children threads have now finished: resuming the main thread */
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    if (pTile->retcode != RL2_OK)
				goto error;
			}
#ifdef _WIN32
		      free (handles);
#endif
		      thread_count = 0;
		      /* we can now continue by inserting all tiles into the DBMS */
		  }
		else
		  {
		      aux_tile = aux_tile->next;
		      continue;
		  }
	    }
	  else
	    {
		/* single thread execution */
		do_get_tile (aux_tile);
		*(thread_slots + 0) = aux_tile;
		do_encode_tile (aux_tile);
		if (aux_tile->retcode != RL2_OK)
		    goto error;
	    }

	  for (thread_count = 0; thread_count < max_threads; thread_count++)
	    {
		/* INSERTing the tile(s) */
		rl2AuxImporterTilePtr pTile = *(thread_slots + thread_count);
		if (pTile == NULL)
		    continue;
		if (!do_insert_tile
		    (handle, pTile->blob_odd, pTile->blob_odd_sz,
		     pTile->blob_even, pTile->blob_even_sz, section_id, srid,
		     pTile->minx, pTile->miny, pTile->maxx, pTile->maxy,
		     NULL, no_data, stmt_tils, stmt_data, section_stats))
		  {
		      pTile->blob_odd = NULL;
		      pTile->blob_even = NULL;
		      goto error;
		  }
		doAuxImporterTileCleanup (pTile);
	    }
	  for (thread_count = 0; thread_count < max_threads; thread_count++)
	      *(thread_slots + thread_count) = NULL;
	  thread_count = 0;
	  aux_tile = aux_tile->next;
      }
    destroyAuxImporter (aux);
    aux = NULL;
    free (thread_slots);
    thread_slots = NULL;

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (rl2_get_defaults_stats(handle, section_stats,p_coverage->coverageName,srid,(minx+((maxx-minx)/2)),(miny+((maxy-miny)/2))))
     goto error;
    if (!rl2_do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_section (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;
    time (&now);
    diff = now - start;
    mins = diff / 60;
    secs = diff - (mins * 60);
    printf (">> Image successfully imported in: %d mins %02d secs\n", mins,
	    secs);

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  if (coverage_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid
	      (handle, max_threads, coverage_name, section_id, 1,
	       verbose) != RL2_OK)
	    {
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
      }

    return 1;

  error:
    if (aux != NULL)
	destroyAuxImporter (aux);
    if (thread_slots != NULL)
	free (thread_slots);
    if (origin != NULL)
	rl2_destroy_section (origin);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

#endif /* end of OpenJpeg conditional */

static int
is_ascii_grid (const char *path)
{
/* testing for an ASCII Grid */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".asc") == 0)
	      return 1;
      }
    return 0;
}

static int
is_jpeg_image (const char *path)
{
/* testing for a JPEG image */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".jpg") == 0)
	      return 1;
      }
    return 0;
}

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */

static int
is_jpeg2000_image (const char *path)
{
/* testing for a Jpeg2000 image */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".jp2") == 0)
	      return 1;
      }
    return 0;
}

#endif /* end OpenJpeg conditional */

static int
do_import_file (sqlite3 * handle, int max_threads, const char *src_path,
		rl2CoveragePtr cvg, const char *section, int worldfile,
		int force_srid, int pyramidize, unsigned char sample_type,
		unsigned char pixel_type, unsigned char num_bands,
		unsigned int tile_w, unsigned int tile_h,
		unsigned char compression, int quality,
		sqlite3_stmt * stmt_data, sqlite3_stmt * stmt_tils,
		sqlite3_stmt * stmt_sect, sqlite3_stmt * stmt_levl,
		sqlite3_stmt * stmt_upd_sect, int verbose, int current,
		int total)
{
/* importing a single Source file */
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    int ret;
    rl2TiffOriginPtr origin = NULL;
    rl2PalettePtr aux_palette = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned int row;
    unsigned int col;
    unsigned int width;
    unsigned int height;
    int srid;
    int xsrid;
    double tile_minx;
    double tile_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double res_x;
    double res_y;
    char *dumb1;
    char *dumb2;
    sqlite3_int64 section_id;
    double base_res_x;
    double base_res_y;
    double confidence;
    time_t start;
    time_t now;
    time_t diff;
    int mins;
    int secs;
    char *xml_summary = NULL;
    rl2AuxImporterPtr aux = NULL;
    rl2AuxImporterTilePtr aux_tile;
    rl2AuxImporterTilePtr *thread_slots = NULL;
    int thread_count;
    char *title = NULL;
    char *abstract = NULL;
    char *copyright = NULL;
    char *gdal_nodata = NULL;
    int i_no_title=0;
    int i_no_abstract=0;
    int i_no_copyright=0;
    int i_no_nodata=0;
    int i_gdal_nodata=-1;
    unsigned char zoom_min=0;
    unsigned char zoom_default=0;
    unsigned char zoom_max=0;
    rl2_get_coverage_infos (cvg, title,abstract,copyright);
    if ((title == NULL) || ((title != NULL) && ((*title == '0') || (strcmp(title,"*** missing Title ***") == 0))))
      i_no_title=1;
    if ((abstract == NULL) || ((abstract != NULL) && ((*abstract == '0') || (strcmp(abstract,"*** missing Abstract ***") == 0))))
      i_no_abstract=1;
     if ((copyright == NULL) || (copyright != NULL) && ((*copyright == '0') || (strcmp(copyright,coverage->coverageName) == 0)))
      i_no_copyright=1;
     // Only replace if created with default_nodata(), which is default
     if ((coverage->noData != NULL) && (coverage->noData->isTransparent))
      i_no_nodata=2;     
     
    if (is_ascii_grid (src_path))
	return do_import_ascii_grid (handle, max_threads, src_path, cvg,
				     section, force_srid, tile_w, tile_h,
				     pyramidize, sample_type, compression,
				     stmt_data, stmt_tils, stmt_sect,
				     stmt_levl, stmt_upd_sect, verbose,
				     current, total);

    if (is_jpeg_image (src_path))
	return do_import_jpeg_image (handle, max_threads, src_path, cvg,
				     section, force_srid, tile_w, tile_h,
				     pyramidize, sample_type, num_bands,
				     compression, quality, stmt_data,
				     stmt_tils, stmt_sect, stmt_levl,
				     stmt_upd_sect, verbose, current, total);

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
    if (is_jpeg2000_image (src_path))
	return do_import_jpeg2000_image (handle, max_threads, src_path, cvg,
					 section, force_srid, tile_w, tile_h,
					 pyramidize, sample_type, num_bands,
					 compression, quality, stmt_data,
					 stmt_tils, stmt_sect, stmt_levl,
					 stmt_upd_sect, verbose, current,
					 total);
#endif /* end OpenJpeg conditonal */

    time (&start);
    if (rl2_get_coverage_resolution (cvg, &base_res_x, &base_res_y) != RL2_OK)
      {
	  if (verbose)
	      fprintf (stderr, "Unknown Coverage Resolution\n");
	  goto error;
      }
    if (worldfile)
	origin =
	    rl2_create_tiff_origin (src_path, RL2_TIFF_WORLDFILE, force_srid,
				    sample_type, pixel_type, num_bands);
    else
	origin =
	    rl2_create_tiff_origin (src_path, RL2_TIFF_GEOTIFF, force_srid,
				    sample_type, pixel_type, num_bands);
    if (origin == NULL)
      {
	  if (verbose)
	      fprintf (stderr, "Invalid TIFF Origin: %s\n", src_path);
	  goto error;
      }
    if (rl2_get_coverage_srid (cvg, &xsrid) == RL2_OK)
      {
	  if (xsrid == RL2_GEOREFERENCING_NONE)
	      rl2_set_tiff_origin_not_referenced (origin);
      }
      rl2PrivTiffOriginPtr tiff_origin = (rl2PrivTiffOriginPtr) origin;
      if ((i_no_title == 1) && ((tiff_origin->DOCUMENTNAME != NULL) && (*tiff_origin->DOCUMENTNAME != '0')))
      { // use the TIFTAG_DOCUMENTNAME as raster_coverage.title
       i_no_title = 2;
       if (title != NULL) 
        free(title);
       int len = strlen (tiff_origin->DOCUMENTNAME);
       title=malloc(len+1);
       strcpy(title,tiff_origin->DOCUMENTNAME);
      }
      if ((i_no_abstract == 1) && ((tiff_origin->IMAGEDESCRIPTION != NULL) && (*tiff_origin->IMAGEDESCRIPTION != '0')))
      { // use the TIFTAG_IMAGEDESCRIPTION as raster_coverage.abstract
       i_no_abstract = 2;
       if (abstract != NULL) 
        free(abstract);
       int len = strlen (tiff_origin->IMAGEDESCRIPTION);
       abstract=malloc(len+1);
       strcpy(abstract,tiff_origin->IMAGEDESCRIPTION);
      }
      if ((i_no_copyright == 1) && ((tiff_origin->COPYRIGHT != NULL) && (*tiff_origin->COPYRIGHT != '0')))
      { // use the TIFTAG_IMAGEDESCRIPTION as raster_coverage.copyright
       i_no_copyright = 2;
       if (copyright != NULL) 
        free(copyright);
       int len = strlen (tiff_origin->COPYRIGHT);
       copyright=malloc(len+1);
       strcpy(copyright,tiff_origin->COPYRIGHT);
      }
      if ((i_no_nodata == 1) && (tiff_origin->GDAL_NODATA != NULL))
      { // this can also be a float/double
        i_no_nodata=2;       
      }
      if ((i_no_title == 2) || (i_no_abstract == 2) || (i_no_copyright == 20))
      {
       ret = set_coverage_infos (handle,coverage->coverageName, title, abstract,copyright);
       // printf ("-I-> do_import_file[rc=%d] [%s] no_title[%d,%s] no_abstract[%d,%s] no_copyright[%d,%s] no_nodata[%d]\n",ret,coverage->coverageName,i_no_title,title, i_no_abstract,abstract,i_no_copyright,copyright,i_no_nodata); 
      }      
    xml_summary = rl2_build_tiff_xml_summary (origin);

    printf ("------------------\n");
    if (total > 1)
	printf ("%d/%d) Importing: %s\n", current, total,
		rl2_get_tiff_origin_path (origin));
    else
	printf ("Importing: %s\n", rl2_get_tiff_origin_path (origin));
    ret = rl2_get_tiff_origin_size (origin, &width, &height);
    if (ret == RL2_OK)
	printf ("    Image Size (pixels): %d x %d\n", width, height);
    ret = rl2_get_tiff_origin_srid (origin, &srid);
    if (ret == RL2_OK)
      {
	  if (force_srid > 0 && force_srid != srid)
	    {
		printf ("                   SRID: %d (forced to %d)\n", srid,
			force_srid);
		srid = force_srid;
	    }
	  else
	      printf ("                   SRID: %d\n", srid);
      }
    ret = rl2_get_tiff_origin_extent (origin, &minx, &miny, &maxx, &maxy);
    if (ret == RL2_OK)
      {
	  dumb1 = formatLong (minx);
	  dumb2 = formatLat (miny);
	  printf ("       LowerLeft Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
	  dumb1 = formatLong (maxx);
	  dumb2 = formatLat (maxy);
	  printf ("      UpperRight Corner: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }
    ret = rl2_get_tiff_origin_resolution (origin, &res_x, &res_y);
    if (ret == RL2_OK)
      {
	  dumb1 = formatFloat (res_x);
	  dumb2 = formatFloat (res_y);
	  printf ("       Pixel resolution: X=%s Y=%s\n", dumb1, dumb2);
	  sqlite3_free (dumb1);
	  sqlite3_free (dumb2);
      }
    if (rl2_get_zoom_levels_for_resolutions(handle,NULL,srid,res_x,res_y,(minx+((maxx-minx)/2)),(miny+((maxy-miny)/2)),
                            &zoom_min,&zoom_default,&zoom_max,NULL,NULL) == RL2_OK)
     printf ("    Approx. Zoom-Levels: min[%d] default[%d] max[%d]\n", zoom_min,zoom_default,zoom_max);
    if (coverage->mixedResolutions)
      {
	  /* accepting any resolution */
      }
    else if (coverage->strictResolution)
      {
	  /* enforcing Strict Resolution check */
	  if (res_x != coverage->hResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Strict) !!!\n");
		goto error;
	    }
	  if (res_y != coverage->vResolution)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Strict) !!!\n");
		goto error;
	    }
      }
    else
      {
	  /* permissive Resolution check */
	  confidence = coverage->hResolution / 100.0;
	  if (res_x < (coverage->hResolution - confidence)
	      || res_x > (coverage->hResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Permissive) !!!\n");
		goto error;
	    }
	  confidence = coverage->vResolution / 100.0;
	  if (res_y < (coverage->vResolution - confidence)
	      || res_y > (coverage->vResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution !(Permissive) !!\n");
		goto error;
	    }
      }

    if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  /* remapping the Palette */
	  if (rl2_check_dbms_palette (handle, cvg, origin) != RL2_OK)
	    {
		fprintf (stderr, "Mismatching Palette !!!\n");
		goto error;
	    }
      }

    if (rl2_eval_tiff_origin_compatibility (cvg, origin, force_srid, verbose)
	!= RL2_TRUE)
      {
	  fprintf (stderr, "Coverage/TIFF mismatch\n");
	  goto error;
      }
    no_data = rl2_get_coverage_no_data (cvg);
     //printf ("-I-> do_import_file[5] no_title[%d,%s] no_abstract[%d,%s] no_copyright[%d,%s] no_nodata[%d] isTransparent[%d]\n",i_no_title,title, i_no_abstract,abstract,i_no_copyright,copyright,i_no_nodata, ((rl2PrivPixelPtr)no_data)->isTransparent);  
    if (i_no_nodata == 2)
    {
     rl2PixelPtr no_data_gdal=rl2_set_gdal_nodata(no_data,tiff_origin->GDAL_NODATA); 
     if (no_data_gdal != NULL)
     {
      rl2_destroy_pixel((rl2PixelPtr) (coverage->noData));  
      coverage->noData=(rl2PrivPixelPtr)rl2_clone_pixel(no_data_gdal);
      rl2_destroy_pixel ((rl2PixelPtr) (no_data_gdal)); 
      ret = set_coverage_nodata(handle,coverage->coverageName, (rl2PixelPtr)coverage->noData);
      no_data =rl2_get_coverage_no_data(cvg);
      // printf ("-I-> do_import_file[rc=%d] no_title[%d,%s] no_abstract[%d,%s] no_copyright[%d,%s] no_nodata[%d] \n",ret,i_no_title,title, i_no_abstract,abstract,i_no_copyright,copyright,i_no_nodata);  
     }  
    }

/* INSERTing the section */
    if (!rl2_do_insert_section
	(handle, src_path, section, srid, width, height, minx, miny, maxx,
	 maxy, xml_summary, coverage->sectionPaths, coverage->sectionMD5,
	 coverage->sectionSummary, stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (coverage->mixedResolutions)
      {
	  /* multiple resolutions Coverage */
	  if (!rl2_do_insert_section_levels
	      (handle, section_id, res_x, res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }
    else
      {
	  /* single resolution Coverage */
	  if (!rl2_do_insert_levels
	      (handle, base_res_x, base_res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }

/* preparing all Tile Requests */
    aux =
	createAuxImporter (coverage, srid, maxx, miny, tile_w, tile_h, res_x,
			   res_y, RL2_ORIGIN_TIFF, origin, RL2_CONVERT_NO,
			   verbose, compression, quality);
    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		/* adding a Tile request */
		addTile2AuxImporter (aux, row, col, tile_minx, tile_maxy);
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;
/* prepating the thread_slots stuct */
    thread_slots = malloc (sizeof (rl2AuxImporterTilePtr) * max_threads);
    for (thread_count = 0; thread_count < max_threads; thread_count++)
	*(thread_slots + thread_count) = NULL;
    thread_count = 0;
    aux_tile = aux->first;
    while (aux_tile != NULL)
      {
	  /* processing a Tile request (may be under parallel execution) */
	  if (max_threads > 1)
	    {
		/* adopting a multithreaded strategy */
		do_get_tile (aux_tile);
		*(thread_slots + thread_count) = aux_tile;
		thread_count++;
		start_tile_thread (aux_tile);
		if (thread_count == max_threads || aux_tile->next == NULL)
		  {
		      /* waiting until all child threads exit */
#ifdef _WIN32
		      HANDLE *handles;
		      int z;
		      int cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* counting how many active threads we currently have */
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    cnt++;
			}
		      handles = malloc (sizeof (HANDLE) * cnt);
		      cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* initializing the HANDLEs array */
			    HANDLE *pOpaque;
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    pOpaque = (HANDLE *) (pTile->opaque_thread_id);
			    *(handles + cnt) = *pOpaque;
			    cnt++;
			}
		      WaitForMultipleObjects (cnt, handles, TRUE, INFINITE);
#else
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    pthread_t *pOpaque;
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    pOpaque = (pthread_t *) (pTile->opaque_thread_id);
			    pthread_join (*pOpaque, NULL);
			}
#endif

		      /* all children threads have now finished: resuming the main thread */
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    if (pTile->retcode != RL2_OK)
				goto error;
			}
#ifdef _WIN32
		      free (handles);
#endif
		      thread_count = 0;
		      /* we can now continue by inserting all tiles into the DBMS */
		  }
		else
		  {
		      aux_tile = aux_tile->next;
		      continue;
		  }
	    }
	  else
	    {
		/* single thread execution */
		do_get_tile (aux_tile);
		*(thread_slots + 0) = aux_tile;
		do_encode_tile (aux_tile);
		if (aux_tile->retcode != RL2_OK)
		    goto error;
	    }

	  for (thread_count = 0; thread_count < max_threads; thread_count++)
	    {
		/* INSERTing the tile(s) */
		rl2AuxImporterTilePtr pTile = *(thread_slots + thread_count);
		if (pTile == NULL)
		    continue;
		aux_palette =
		    rl2_clone_palette (rl2_get_raster_palette
				       (aux_tile->raster));
  // printf ("orting: min_max[%2.4f,%2.4f,%2.4f,%2.4f]\n",  pTile->minx, pTile->miny, pTile->maxx, pTile->maxy);
		if (!do_insert_tile
		    (handle, pTile->blob_odd, pTile->blob_odd_sz,
		     pTile->blob_even, pTile->blob_even_sz, section_id, srid,
		     pTile->minx, pTile->miny, pTile->maxx, pTile->maxy,
		     aux_palette, no_data, stmt_tils, stmt_data, section_stats))
		  {
		      pTile->blob_odd = NULL;
		      pTile->blob_even = NULL;
		      goto error;
		  }
		doAuxImporterTileCleanup (pTile);
	    }
	  for (thread_count = 0; thread_count < max_threads; thread_count++)
	      *(thread_slots + thread_count) = NULL;
	  thread_count = 0;
	  aux_tile = aux_tile->next;
      }
    destroyAuxImporter (aux);
    aux = NULL;
    free (thread_slots);
    thread_slots = NULL;

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (rl2_get_defaults_stats(handle, section_stats,coverage->coverageName,srid,(minx+((maxx-minx)/2)),(miny+((maxy-miny)/2))))
     goto error;
    if (!rl2_do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;


    rl2_destroy_tiff_origin (origin);
    rl2_destroy_raster_statistics (section_stats);
    origin = NULL;
    section_stats = NULL;
    time (&now);
    diff = now - start;
    mins = diff / 60;
    secs = diff - (mins * 60);
    printf (">> Image successfully imported in: %d mins %02d secs\n", mins,
	    secs);

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  if (coverage_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid
	      (handle, max_threads, coverage_name, section_id, 1,
	       verbose) != RL2_OK)
	    {
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
      }

    return 1;

  error:
    if (aux != NULL)
	destroyAuxImporter (aux);
    if (thread_slots != NULL)
	free (thread_slots);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    return 0;
}

static int
check_extension_match (const char *file_name, const char *file_ext)
{
/* checks the file extension */
    const char *mark = NULL;
    const char *p = file_name;
    int len;
    char *ext;
    int match = 0;
    if (file_ext == NULL)
	return 0;

    len = strlen (file_ext);
    if (*file_ext == '.')
      {
	  /* file extension starts with dot */
	  ext = malloc (len + 1);
	  strcpy (ext, file_ext);
      }
    else
      {
	  /* file extension doesn't start with dot */
	  ext = malloc (len + 2);
	  *ext = '.';
	  strcpy (ext + 1, file_ext);
      }
    while (*p != '\0')
      {
	  if (*p == '.')
	      mark = p;
	  p++;
      }
    if (mark == NULL)
      {
	  free (ext);
	  return 0;
      }
    match = strcasecmp (mark, ext);
    free (ext);
    if (match == 0)
	return 1;
    return 0;
}

static int
do_import_dir (sqlite3 * handle, int max_threads, const char *dir_path,
	       const char *file_ext, rl2CoveragePtr cvg, const char *section,
	       int worldfile, int force_srid, int pyramidize,
	       unsigned char sample_type, unsigned char pixel_type,
	       unsigned char num_bands, unsigned int tile_w,
	       unsigned int tile_h, unsigned char compression, int quality,
	       sqlite3_stmt * stmt_data, sqlite3_stmt * stmt_tils,
	       sqlite3_stmt * stmt_sect, sqlite3_stmt * stmt_levl,
	       sqlite3_stmt * stmt_upd_sect, int verbose)
{
/* importing a whole directory */
#if defined(_WIN32) && !defined(__MINGW32__)
/* Visual Studio .NET */
    struct _finddata_t c_file;
    intptr_t hFile;
    int cnt = 0;
    int total = 0;
    char *search;
    char *path;
    int ret;
    if (_chdir (dir_path) < 0)
	return 0;
    search = sqlite3_mprintf ("*%s", file_ext);
    if ((hFile = _findfirst (search, &c_file)) == -1L)
	;
    else
      {
	  while (1)
	    {
		if ((c_file.attrib & _A_RDONLY) == _A_RDONLY
		    || (c_file.attrib & _A_NORMAL) == _A_NORMAL)
		    total++;
		if (_findnext (hFile, &c_file) != 0)
		    break;
	    }
	  _findclose (hFile);
	  if ((hFile = _findfirst (search, &c_file)) == -1L)
	      ;
	  else
	    {
		while (1)
		  {
		      if ((c_file.attrib & _A_RDONLY) == _A_RDONLY
			  || (c_file.attrib & _A_NORMAL) == _A_NORMAL)
			{
			    path =
				sqlite3_mprintf ("%s/%s", dir_path,
						 c_file.name);
			    ret =
				do_import_file (handle, max_threads, path,
						cvg, section, worldfile,
						force_srid, pyramidize,
						sample_type, pixel_type,
						num_bands, tile_w, tile_h,
						compression, quality,
						stmt_data, stmt_tils,
						stmt_sect, stmt_levl,
						stmt_upd_sect, verbose,
						cnt + 1, total);
			    sqlite3_free (path);
			    if (!ret)
				goto error;
			    cnt++;
			}
		      if (_findnext (hFile, &c_file) != 0)
			  break;
		  }
	      error:
		_findclose (hFile);
	    }
	  sqlite3_free (search);
	  return cnt;
#else
/* not Visual Studio .NET */
    int cnt = 0;
    int total = 0;
    char *path;
    struct dirent *entry;
    int ret;
    DIR *dir = opendir (dir_path);
    if (!dir)
	return 0;
    while (1)
      {
	  /* counting how many valid entries */
	  entry = readdir (dir);
	  if (!entry)
	      break;
	  if (!check_extension_match (entry->d_name, file_ext))
	      continue;
	  total++;
      }
    rewinddir (dir);
    while (1)
      {
	  /* scanning dir-entries */
	  entry = readdir (dir);
	  if (!entry)
	      break;
	  if (!check_extension_match (entry->d_name, file_ext))
	      continue;
	  path = sqlite3_mprintf ("%s/%s", dir_path, entry->d_name);
	  ret =
	      do_import_file (handle, max_threads, path, cvg, section,
			      worldfile, force_srid, pyramidize, sample_type,
			      pixel_type, num_bands, tile_w, tile_h,
			      compression, quality, stmt_data, stmt_tils,
			      stmt_sect, stmt_levl, stmt_upd_sect, verbose,
			      cnt + 1, total);
	  sqlite3_free (path);
	  if (!ret)
	      goto error;
	  cnt++;
      }
  error:
    closedir (dir);
    return cnt;
#endif
}

static int
do_import_common (sqlite3 * handle, int max_threads, const char *src_path,
		  const char *dir_path, const char *file_ext,
		  rl2CoveragePtr cvg, const char *section, int worldfile,
		  int force_srid, int pyramidize, int verbose)
{
/* main IMPORT Raster function */
    rl2PrivCoveragePtr privcvg = (rl2PrivCoveragePtr) cvg;
    int ret;
    char *sql;
    const char *coverage;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned int tile_w;
    unsigned int tile_h;
    unsigned char compression;
    int quality;
    char *table;
    char *xtable;
    unsigned int tileWidth;
    unsigned int tileHeight;
    sqlite3_stmt *stmt_data = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_sect = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_upd_sect = NULL;

    if (cvg == NULL)
	goto error;

    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;

    tile_w = tileWidth;
    tile_h = tileHeight;
    rl2_get_coverage_compression (cvg, &compression, &quality);
    rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands);
    coverage = rl2_get_coverage_name (cvg);

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (section_id, section_name, file_path, "
	 "md5_checksum, summary, width, height, geometry) "
	 "VALUES (NULL, ?, ?, ?, XB_Create(?), ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO sections SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET statistics = ? WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_upd_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE sections SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

    if (privcvg->mixedResolutions)
      {
	  /* mixed resolutions Coverage */
	  table = sqlite3_mprintf ("%s_section_levels", coverage);
	  xtable = rl2_double_quoted_sql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf
	      ("INSERT OR IGNORE INTO \"%s\" (section_id, pyramid_level, "
	       "x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	       "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	       "VALUES (?, 0, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
	  free (xtable);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("INSERT INTO section_levels SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }
    else
      {
	  /* single resolution Coverage */
	  table = sqlite3_mprintf ("%s_levels", coverage);
	  xtable = rl2_double_quoted_sql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf
	      ("INSERT OR IGNORE INTO \"%s\" (pyramid_level, "
	       "x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	       "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	       "VALUES (0, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
	  free (xtable);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("INSERT INTO levels SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, 0, ?, BuildMBR(?, ?, ?, ?, ?))", xtable);
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

    if (dir_path == NULL)
      {
	  /* importing a single Image file */
	  if (!do_import_file
	      (handle, max_threads, src_path, cvg, section, worldfile,
	       force_srid, pyramidize, sample_type, pixel_type, num_bands,
	       tile_w, tile_h, compression, quality, stmt_data, stmt_tils,
	       stmt_sect, stmt_levl, stmt_upd_sect, verbose, -1, -1))
	      goto error;
      }
    else
      {
	  /* importing all Image files from a whole directory */
	  if (!do_import_dir
	      (handle, max_threads, dir_path, file_ext, cvg, section,
	       worldfile, force_srid, pyramidize, sample_type, pixel_type,
	       num_bands, tile_w, tile_h, compression, quality, stmt_data,
	       stmt_tils, stmt_sect, stmt_levl, stmt_upd_sect, verbose))
	      goto error;
      }
    sqlite3_finalize (stmt_upd_sect);
    sqlite3_finalize (stmt_sect);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data); // causing error
    stmt_upd_sect = NULL;
    stmt_sect = NULL;
    stmt_levl = NULL;
    stmt_tils = NULL;
    stmt_data = NULL;
    if (rl2_update_dbms_coverage (handle, coverage) != RL2_OK)
      {
	  fprintf (stderr, "unable to update the Coverage\n");
	  goto error;
      }

    return 1;

  error:
    if (stmt_upd_sect != NULL)
	sqlite3_finalize (stmt_upd_sect);
    if (stmt_sect != NULL)
	sqlite3_finalize (stmt_sect);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    return 0;
}

RL2_DECLARE int
rl2_load_raster_into_dbms (sqlite3 * handle, int max_threads,
			   const char *src_path, rl2CoveragePtr coverage,
			   int worldfile, int force_srid, int pyramidize,
			   int verbose)
{
/* importing a single Raster file */
    if (!do_import_common
	(handle, max_threads, src_path, NULL, NULL, coverage, NULL, worldfile,
	 force_srid, pyramidize, verbose))
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_load_mrasters_into_dbms (sqlite3 * handle, int max_threads,
			     const char *dir_path, const char *file_ext,
			     rl2CoveragePtr coverage, int worldfile,
			     int force_srid, int pyramidize, int verbose)
{
/* importing multiple Raster files from dir */
    if (!do_import_common
	(handle, max_threads, NULL, dir_path, file_ext, coverage, NULL,
	 worldfile, force_srid, pyramidize, verbose))
	return RL2_ERROR;
    return RL2_OK;
}

static int
mismatching_size (unsigned int width, unsigned int height, double x_res,
		  double y_res, double minx, double miny, double maxx,
		  double maxy)
{
/* checking if the image size and the map extent do match */
    double ext_x = (double) width * x_res;
    double ext_y = (double) height * y_res;
    double img_x = maxx - minx;
    double img_y = maxy - miny;
    double confidence;
    confidence = ext_x / 100.0;
    if (img_x < (ext_x - confidence) || img_x > (ext_x + confidence))
	return 1;
    confidence = ext_y / 100.0;
    if (img_y < (ext_y - confidence) || img_y > (ext_y + confidence))
	return 1;
    return 0;
}

static void
copy_int8_outbuf_to_tile (const char *outbuf, char *tile,
			  unsigned int width,
			  unsigned int height,
			  unsigned int tile_width,
			  unsigned int tile_height, unsigned int base_y,
			  unsigned int base_x)
{
/* copying INT8 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
    const char *p_in;
    char *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_uint8_outbuf_to_tile (const unsigned char *outbuf, unsigned char *tile,
			   unsigned char num_bands, unsigned int width,
			   unsigned int height,
			   unsigned int tile_width,
			   unsigned int tile_height, unsigned int base_y,
			   unsigned int base_x)
{
/* copying UINT8 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
    int b;
    const unsigned char *p_in;
    unsigned char *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in =
	      outbuf + ((base_y + y) * width * num_bands) +
	      (base_x * num_bands);
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out += num_bands;
		      p_in += num_bands;
		      continue;
		  }
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_int16_outbuf_to_tile (const short *outbuf, short *tile,
			   unsigned int width,
			   unsigned int height,
			   unsigned int tile_width,
			   unsigned int tile_height, unsigned int base_y,
			   unsigned int base_x)
{
/* copying INT16 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
    const short *p_in;
    short *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_uint16_outbuf_to_tile (const unsigned short *outbuf,
			    unsigned short *tile, unsigned char num_bands,
			    unsigned int width, unsigned int height,
			    unsigned int tile_width, unsigned int tile_height,
			    unsigned int base_y, unsigned int base_x)
{
/* copying UINT16 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
    int b;
    const unsigned short *p_in;
    unsigned short *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in =
	      outbuf + ((base_y + y) * width * num_bands) +
	      (base_x * num_bands);
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out += num_bands;
		      p_in += num_bands;
		      continue;
		  }
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_int32_outbuf_to_tile (const int *outbuf, int *tile,
			   unsigned int width,
			   unsigned int height,
			   unsigned int tile_width,
			   unsigned int tile_height, unsigned int base_y,
			   unsigned int base_x)
{
/* copying INT32 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
    const int *p_in;
    int *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_uint32_outbuf_to_tile (const unsigned int *outbuf, unsigned int *tile,
			    unsigned int width,
			    unsigned int height,
			    unsigned int tile_width,
			    unsigned int tile_height, int base_y, int base_x)
{
/* copying UINT32 pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
    const unsigned int *p_in;
    unsigned int *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_float_outbuf_to_tile (const float *outbuf, float *tile,
			   unsigned int width,
			   unsigned int height,
			   unsigned int tile_width,
			   unsigned int tile_height, unsigned int base_y,
			   unsigned int base_x)
{
/* copying FLOAT pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
    const float *p_in;
    float *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_double_outbuf_to_tile (const double *outbuf, double *tile,
			    unsigned int width,
			    unsigned int height,
			    unsigned int tile_width,
			    unsigned int tile_height, unsigned int base_y,
			    unsigned int base_x)
{
/* copying DOUBLE pixels from the output buffer into the tile */
    unsigned int x;
    unsigned int y;
    const double *p_in;
    double *p_out = tile;

    for (y = 0; y < tile_height; y++)
      {
	  if ((base_y + y) >= height)
	      break;
	  p_in = outbuf + ((base_y + y) * width) + base_x;
	  for (x = 0; x < tile_width; x++)
	    {
		if ((base_x + x) >= width)
		  {
		      p_out++;
		      p_in++;
		      continue;
		  }
		*p_out++ = *p_in++;
	    }
      }
}

static void
copy_from_outbuf_to_tile (const unsigned char *outbuf, unsigned char *tile,
			  unsigned char sample_type, unsigned char num_bands,
			  unsigned int width, unsigned int height,
			  unsigned int tile_width, unsigned int tile_height,
			  unsigned int base_y, unsigned int base_x)
{
/* copying pixels from the output buffer into the tile */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  copy_int8_outbuf_to_tile ((char *) outbuf,
				    (char *) tile, width, height, tile_width,
				    tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_INT16:
	  copy_int16_outbuf_to_tile ((short *) outbuf,
				     (short *) tile, width, height,
				     tile_width, tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_outbuf_to_tile ((unsigned short *) outbuf,
				      (unsigned short *) tile, num_bands,
				      width, height, tile_width, tile_height,
				      base_y, base_x);
	  break;
      case RL2_SAMPLE_INT32:
	  copy_int32_outbuf_to_tile ((int *) outbuf,
				     (int *) tile, width, height, tile_width,
				     tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_UINT32:
	  copy_uint32_outbuf_to_tile ((unsigned int *) outbuf,
				      (unsigned int *) tile, width, height,
				      tile_width, tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_FLOAT:
	  copy_float_outbuf_to_tile ((float *) outbuf,
				     (float *) tile, width, height,
				     tile_width, tile_height, base_y, base_x);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  copy_double_outbuf_to_tile ((double *) outbuf,
				      (double *) tile, width, height,
				      tile_width, tile_height, base_y, base_x);
	  break;
      default:
	  copy_uint8_outbuf_to_tile ((unsigned char *) outbuf,
				     (unsigned char *) tile, num_bands, width,
				     height, tile_width, tile_height, base_y,
				     base_x);
	  break;
      };
}

static int
export_geotiff_common (sqlite3 * handle, int max_threads,
		       const char *dst_path, rl2CoveragePtr cvg,
		       int by_section, sqlite3_int64 section_id, double x_res,
		       double y_res, double minx, double miny, double maxx,
		       double maxy, unsigned int width, unsigned int height,
		       unsigned char compression, unsigned int tile_sz,
		       int with_worldfile,
         unsigned char *geom_cutline_blob, 
         int *geom_cutline_blob_sz)
{
/* exporting a GeoTIFF common implementation */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    unsigned int base_x;
    unsigned int base_y;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL; 
    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;
    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data = rl2_get_coverage_no_data (cvg);
    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (by_section)
      {
	  /* just a single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, max_threads, cvg, section_id, width, height, minx,
	       miny, maxx, maxy, xx_res, yy_res, &outbuf, &outbuf_size,
	       &palette, pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, max_threads, cvg, width, height, minx, miny, maxx,
	       maxy, xx_res, yy_res, &outbuf, &outbuf_size, &palette,
	       pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

/* computing the sample size */
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

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, pixel_type, num_bands,
					palette, compression, 1,
					tile_sz, srid, minx, miny, maxx,
					maxy, xx_res, yy_res, with_worldfile,
     no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz,
					 sample_type, num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    if (with_worldfile)
      {
	  /* exporting the Worldfile */
	  if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	      goto error;
      }

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_geotiff_from_dbms (sqlite3 * handle, int max_threads,
			      const char *dst_path, rl2CoveragePtr cvg,
			      double x_res, double y_res, double minx,
			      double miny, double maxx, double maxy,
			      unsigned int width, unsigned int height,
			      unsigned char compression, unsigned int tile_sz,
			      int with_worldfile,
         unsigned char *geom_cutline_blob, 
         int *geom_cutline_blob_sz)
{
/* exporting a GeoTIFF from the DBMS into the file-system */
    return export_geotiff_common (handle, max_threads, dst_path, cvg, 0, 0,
				  x_res, y_res, minx, miny, maxx, maxy, width,
				  height, compression, tile_sz, with_worldfile,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_geotiff_from_dbms (sqlite3 * handle, int max_threads,
				      const char *dst_path,
				      rl2CoveragePtr cvg,
				      sqlite3_int64 section_id, double x_res,
				      double y_res, double minx, double miny,
				      double maxx, double maxy,
				      unsigned int width, unsigned int height,
				      unsigned char compression,
				      unsigned int tile_sz, int with_worldfile,
          unsigned char *geom_cutline_blob, 
          int *geom_cutline_blob_sz)
{
/* exporting a GeoTIFF - Section*/
    return export_geotiff_common (handle, max_threads, dst_path, cvg, 1,
				  section_id, x_res, y_res, minx, miny, maxx,
				  maxy, width, height, compression, tile_sz,
				  with_worldfile,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_tiff_worldfile_common (sqlite3 * handle, int max_threads,
			     const char *dst_path, rl2CoveragePtr cvg,
			     int by_section, sqlite3_int64 section_id,
			     double x_res, double y_res, double minx,
			     double miny, double maxx, double maxy,
			     unsigned int width, unsigned int height,
			     unsigned char compression, unsigned int tile_sz,
        unsigned char *geom_cutline_blob, 
        int *geom_cutline_blob_sz)
{
/* exporting a TIFF+TFW common implementation */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    unsigned int base_x;
    unsigned int base_y;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL; 

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data = rl2_get_coverage_no_data (cvg);

    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (by_section)
      {
	  /* just a single select Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, max_threads, cvg, section_id, width, height, minx,
	       miny, maxx, maxy, xx_res, yy_res, &outbuf, &outbuf_size,
	       &palette, pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, max_threads, cvg, width, height, minx, miny, maxx,
	       maxy, xx_res, yy_res, &outbuf, &outbuf_size, &palette,
	       pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

/* computing the sample size */
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

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, pixel_type,
					       num_bands, palette,
					       compression, 1, tile_sz, srid,
					       minx, miny, maxx, maxy, xx_res,
					       yy_res,no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz,
					 sample_type, num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

/* exporting the Worldfile */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	goto error;

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_tiff_worldfile_from_dbms (sqlite3 * handle, int max_threads,
				     const char *dst_path, rl2CoveragePtr cvg,
				     double x_res, double y_res, double minx,
				     double miny, double maxx, double maxy,
				     unsigned int width, unsigned int height,
				     unsigned char compression,
				     unsigned int tile_sz,
         unsigned char *geom_cutline_blob, 
         int *geom_cutline_blob_sz)
{
/* exporting a TIFF+TFW from the DBMS into the file-system */
    return export_tiff_worldfile_common (handle, max_threads, dst_path, cvg, 0,
					0, x_res, y_res, minx, miny, maxx,
					maxy, width, height, compression,
					tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_tiff_worldfile_from_dbms (sqlite3 * handle,
					     int max_threads,
					     const char *dst_path,
					     rl2CoveragePtr cvg,
					     sqlite3_int64 section_id,
					     double x_res, double y_res,
					     double minx, double miny,
					     double maxx, double maxy,
					     unsigned int width,
					     unsigned int height,
					     unsigned char compression,
					     unsigned int tile_sz,
          unsigned char *geom_cutline_blob, 
          int *geom_cutline_blob_sz)
{
/* exporting a TIFF+TFW - single Section */
    return export_tiff_worldfile_common (handle, max_threads, dst_path, cvg, 1,
					section_id, x_res, y_res, minx, miny,
					maxx, maxy, width, height,
					compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_tiff_common (sqlite3 * handle, int max_threads, const char *dst_path,
		    rl2CoveragePtr cvg, int by_section,
		    sqlite3_int64 section_id, double x_res, double y_res,
		    double minx, double miny, double maxx, double maxy,
		    unsigned int width, unsigned int height,
		    unsigned char compression, unsigned int tile_sz,
      unsigned char *geom_cutline_blob, 
      int *geom_cutline_blob_sz)
{
/* exporting a plain TIFF common implementation */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr plt2 = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    unsigned int base_x;
    unsigned int base_y;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL;    

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data = rl2_get_coverage_no_data (cvg);

    if (level > 0)
      {
	  /* special handling for Pyramid tiles */
	  if (sample_type == RL2_SAMPLE_1_BIT
	      && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1)
	    {
		/* expecting a Grayscale/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_UINT8
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* expecting an RGB/PNG Pyramid tile */
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
      }

    if (by_section)
      {
	  /* just a single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, max_threads, cvg, section_id, width, height, minx,
	       miny, maxx, maxy, xx_res, yy_res, &outbuf, &outbuf_size,
	       &palette, pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, max_threads, cvg, width, height, minx, miny, maxx,
	       maxy, xx_res, yy_res, &outbuf, &outbuf_size, &palette,
	       pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

/* computing the sample size */
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

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     pixel_type, num_bands, palette,
				     compression, 1, tile_sz,
         no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = pix_sz * num_bands * tile_sz * tile_sz;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		if (pixel_type == RL2_PIXEL_PALETTE && palette != NULL)
		    rl2_prime_void_tile_palette (bufpix, tile_sz, tile_sz,
						 no_data);
		else
		    rl2_prime_void_tile (bufpix, tile_sz, tile_sz,
					 sample_type, num_bands, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  num_bands, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		plt2 = rl2_clone_palette (palette);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       pixel_type, num_bands, bufpix,
				       bufpix_size, plt2, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    rl2_destroy_tiff_destination (tiff);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_tiff_from_dbms (sqlite3 * handle, int max_threads,
			   const char *dst_path, rl2CoveragePtr cvg,
			   double x_res, double y_res, double minx,
			   double miny, double maxx, double maxy,
			   unsigned int width, unsigned int height,
			   unsigned char compression, unsigned int tile_sz,
      unsigned char *geom_cutline_blob, 
      int *geom_cutline_blob_sz)
{
/* exporting a plain TIFF from the DBMS into the file-system */
    return export_tiff_common (handle, max_threads, dst_path, cvg, 0, 0,
			       x_res, y_res, minx, miny, maxx, maxy, width,
			       height, compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_tiff_from_dbms (sqlite3 * handle, int max_threads,
				   const char *dst_path, rl2CoveragePtr cvg,
				   sqlite3_int64 section_id, double x_res,
				   double y_res, double minx, double miny,
				   double maxx, double maxy,
				   unsigned int width, unsigned int height,
				   unsigned char compression,
				   unsigned int tile_sz,
       unsigned char *geom_cutline_blob, 
       int *geom_cutline_blob_sz)
{
/* exporting a plain TIFF - single Section*/
    return export_tiff_common (handle, max_threads, dst_path, cvg, 1,
			       section_id, x_res, y_res, minx, miny, maxx,
			       maxy, width, height, compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_triple_band_geotiff_common (int by_section, sqlite3 * handle,
				   const char *dst_path,
				   rl2CoveragePtr cvg,
				   sqlite3_int64 section_id, double x_res,
				   double y_res, double minx, double miny,
				   double maxx, double maxy,
				   unsigned int width, unsigned int height,
				   unsigned char red_band,
				   unsigned char green_band,
				   unsigned char blue_band,
				   unsigned char compression,
				   unsigned int tile_sz, int with_worldfile,
       unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* exporting a Band-Composed GeoTIFF - common implementation */
    rl2RasterPtr raster = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    unsigned int base_x;
    unsigned int base_y;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL; 

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_triple_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, red_band, green_band, blue_band, &outbuf,
	       &outbuf_size, no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_triple_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	       no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, RL2_PIXEL_RGB, 3,
					NULL, compression, 1, tile_sz, srid,
					minx, miny, maxx, maxy, xx_res,
					yy_res, with_worldfile,
     no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    if (with_worldfile)
      {
	  /* exporting the Worldfile */
	  if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	      goto error;
      }

    rl2_destroy_tiff_destination (tiff);
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_geotiff_from_dbms (sqlite3 * handle,
					  const char *dst_path,
					  rl2CoveragePtr cvg, double x_res,
					  double y_res, double minx,
					  double miny, double maxx,
					  double maxy, unsigned int width,
					  unsigned int height,
					  unsigned char red_band,
					  unsigned char green_band,
					  unsigned char blue_band,
					  unsigned char compression,
					  unsigned int tile_sz,
					  int with_worldfile,
       unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* exporting a Band-Composed GeoTIFF from the DBMS into the file-system */
    return export_triple_band_geotiff_common (0, handle, dst_path, cvg, 0,
					      x_res, y_res, minx, miny, maxx,
					      maxy, width, height, red_band,
					      green_band, blue_band,
					      compression, tile_sz,
					      with_worldfile,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_triple_band_geotiff_from_dbms (sqlite3 * handle,
						  const char *dst_path,
						  rl2CoveragePtr cvg,
						  sqlite3_int64 section_id,
						  double x_res, double y_res,
						  double minx, double miny,
						  double maxx, double maxy,
						  unsigned int width,
						  unsigned int height,
						  unsigned char red_band,
						  unsigned char green_band,
						  unsigned char blue_band,
						  unsigned char compression,
						  unsigned int tile_sz,
						  int with_worldfile,
        unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* exporting a Band-Composed GeoTIFF - Section */
    return export_triple_band_geotiff_common (1, handle, dst_path, cvg,
					      section_id, x_res, y_res, minx,
					      miny, maxx, maxy, width, height,
					      red_band, green_band, blue_band,
					      compression, tile_sz,
					      with_worldfile,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_mono_band_geotiff_common (int by_section, sqlite3 * handle,
				 const char *dst_path,
				 rl2CoveragePtr cvg, sqlite3_int64 section_id,
				 double x_res, double y_res, double minx,
				 double miny, double maxx, double maxy,
				 unsigned int width, unsigned int height,
				 unsigned char mono_band,
				 unsigned char compression,
				 unsigned int tile_sz, int with_worldfile,
     unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* exporting a Mono-Band GeoTIFF common implementation */
    rl2RasterPtr raster = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    rl2PixelPtr no_data_mono = NULL;
    rl2PixelPtr no_data = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    unsigned int base_x;
    unsigned int base_y;
    unsigned char out_pixel;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL; 

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data_mono = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_mono, mono_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_mono_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, mono_band, &outbuf, &outbuf_size,
	       no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_mono_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, mono_band, &outbuf, &outbuf_size, no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_geotiff_destination (dst_path, handle, width, height,
					sample_type, out_pixel, 1,
					NULL, compression, 1, tile_sz, srid,
					minx, miny, maxx, maxy, xx_res,
					yy_res, with_worldfile,
     no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    if (with_worldfile)
      {
	  /* exporting the Worldfile */
	  if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	      goto error;
      }

    rl2_destroy_tiff_destination (tiff);
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_geotiff_from_dbms (sqlite3 * handle,
					const char *dst_path,
					rl2CoveragePtr cvg, double x_res,
					double y_res, double minx,
					double miny, double maxx,
					double maxy, unsigned int width,
					unsigned int height,
					unsigned char mono_band,
					unsigned char compression,
					unsigned int tile_sz,
					int with_worldfile,
     unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* exporting a Mono-Band GeoTIFF from the DBMS into the file-system */
    return export_mono_band_geotiff_common (0, handle, dst_path, cvg, 0,
					    x_res, y_res, minx, miny, maxx,
					    maxy, width, height, mono_band,
					    compression, tile_sz,
					    with_worldfile,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_mono_band_geotiff_from_dbms (sqlite3 * handle,
						const char *dst_path,
						rl2CoveragePtr cvg,
						sqlite3_int64 section_id,
						double x_res, double y_res,
						double minx, double miny,
						double maxx, double maxy,
						unsigned int width,
						unsigned int height,
						unsigned char mono_band,
						unsigned char compression,
						unsigned int tile_sz,
						int with_worldfile,
      unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* exporting a Mono-Band GeoTIFF - Section */
    return export_mono_band_geotiff_common (1, handle, dst_path, cvg,
					    section_id, x_res, y_res, minx,
					    miny, maxx, maxy, width, height,
					    mono_band, compression, tile_sz,
					    with_worldfile,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_triple_band_tiff_worldfile_common (int by_section, sqlite3 * handle,
					  const char *dst_path,
					  rl2CoveragePtr cvg,
					  sqlite3_int64 section_id,
					  double x_res, double y_res,
					  double minx, double miny,
					  double maxx, double maxy,
					  unsigned int width,
					  unsigned int height,
					  unsigned char red_band,
					  unsigned char green_band,
					  unsigned char blue_band,
					  unsigned char compression,
					  unsigned int tile_sz,
       unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* exporting a Band-Composed TIFF+TFW common implementation */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    unsigned int base_x;
    unsigned int base_y;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL; 

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_triple_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, red_band, green_band, blue_band, &outbuf,
	       &outbuf_size, no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_triple_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	       no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, RL2_PIXEL_RGB,
					       3, NULL, compression, 1,
					       tile_sz, srid, minx, miny,
					       maxx, maxy, xx_res, yy_res,
            no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

/* exporting the Worldfile */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	goto error;

    rl2_destroy_tiff_destination (tiff);
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_tiff_worldfile_from_dbms (sqlite3 * handle,
						 const char *dst_path,
						 rl2CoveragePtr cvg,
						 double x_res, double y_res,
						 double minx, double miny,
						 double maxx, double maxy,
						 unsigned int width,
						 unsigned int height,
						 unsigned char red_band,
						 unsigned char green_band,
						 unsigned char blue_band,
						 unsigned char compression,
						 unsigned int tile_sz,
       unsigned char *geom_cutline_blob, 
       int *geom_cutline_blob_sz)
{
/* exporting a Band-Composed TIFF+TFW from the DBMS into the file-system */
    return export_triple_band_tiff_worldfile_common (0, handle, dst_path, cvg,
						     0, x_res, y_res, minx,
						     miny, maxx, maxy, width,
						     height, red_band,
						     green_band, blue_band,
						     compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_triple_band_tiff_worldfile_from_dbms (sqlite3 * handle,
							 const char *dst_path,
							 rl2CoveragePtr cvg,
							 sqlite3_int64
							 section_id,
							 double x_res,
							 double y_res,
							 double minx,
							 double miny,
							 double maxx,
							 double maxy,
							 unsigned int width,
							 unsigned int height,
							 unsigned char
							 red_band,
							 unsigned char
							 green_band,
							 unsigned char
							 blue_band,
							 unsigned char
							 compression,
							 unsigned int tile_sz,
        unsigned char *geom_cutline_blob, 
        int *geom_cutline_blob_sz)
{
/* exporting a Band-Composed TIFF+TFW - Sction */
    return export_triple_band_tiff_worldfile_common (1, handle, dst_path, cvg,
						     section_id, x_res, y_res,
						     minx, miny, maxx, maxy,
						     width, height, red_band,
						     green_band, blue_band,
						     compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_mono_band_tiff_worldfile_common (int by_section, sqlite3 * handle,
					const char *dst_path,
					rl2CoveragePtr cvg,
					sqlite3_int64 section_id,
					double x_res, double y_res,
					double minx, double miny, double maxx,
					double maxy, unsigned int width,
					unsigned int height,
					unsigned char mono_band,
					unsigned char compression,
					unsigned int tile_sz,
     unsigned char *geom_cutline_blob, 
     int *geom_cutline_blob_sz)
{
/* exporting a Mono-Band TIFF+TFW - common implementation */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    unsigned int base_x;
    unsigned int base_y;
    unsigned char out_pixel;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL; 

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_multi, mono_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_mono_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, mono_band, &outbuf, &outbuf_size,
	       no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_mono_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, mono_band, &outbuf, &outbuf_size, no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_tiff_worldfile_destination (dst_path, width, height,
					       sample_type, out_pixel,
					       1, NULL, compression, 1,
					       tile_sz, srid, minx, miny,
					       maxx, maxy, xx_res, yy_res,
            no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

/* exporting the Worldfile */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
	goto error;

    rl2_destroy_tiff_destination (tiff);
    free (outbuf);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_tiff_worldfile_from_dbms (sqlite3 * handle,
					       const char *dst_path,
					       rl2CoveragePtr cvg,
					       double x_res, double y_res,
					       double minx, double miny,
					       double maxx, double maxy,
					       unsigned int width,
					       unsigned int height,
					       unsigned char mono_band,
					       unsigned char compression,
					       unsigned int tile_sz,
            unsigned char *geom_cutline_blob, 
            int *geom_cutline_blob_sz)
{
/* exporting a Mono-Band TIFF+TFW from the DBMS into the file-system */
    return export_mono_band_tiff_worldfile_common (0, handle, dst_path, cvg,
						   0, x_res, y_res, minx,
						   miny, maxx, maxy, width,
						   height, mono_band,
						   compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_mono_band_tiff_worldfile_from_dbms (sqlite3 * handle,
						       const char *dst_path,
						       rl2CoveragePtr cvg,
						       sqlite3_int64
						       section_id,
						       double x_res,
						       double y_res,
						       double minx,
						       double miny,
						       double maxx,
						       double maxy,
						       unsigned int width,
						       unsigned int height,
						       unsigned char
						       mono_band,
						       unsigned char
						       compression,
						       unsigned int tile_sz,
             unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* exporting a Mono-Band TIFF+TFW - Section */
    return export_mono_band_tiff_worldfile_common (1, handle, dst_path, cvg,
						   section_id, x_res, y_res,
						   minx, miny, maxx, maxy,
						   width, height, mono_band,
						   compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_triple_band_tiff_common (int by_section, sqlite3 * handle,
				const char *dst_path, rl2CoveragePtr cvg,
				sqlite3_int64 section_id, double x_res,
				double y_res, double minx, double miny,
				double maxx, double maxy, unsigned int width,
				unsigned int height, unsigned char red_band,
				unsigned char green_band,
				unsigned char blue_band,
				unsigned char compression, unsigned int tile_sz,
    unsigned char *geom_cutline_blob, 
    int *geom_cutline_blob_sz)
{
/* exporting a plain Band-Composed TIFF common implementation */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    unsigned int base_x;
    unsigned int base_y;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL; 

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data =
	rl2_create_triple_band_pixel (no_data_multi, red_band, green_band,
				      blue_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_triple_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, red_band, green_band, blue_band, &outbuf,
	       &outbuf_size, no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_triple_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, red_band, green_band, blue_band, &outbuf, &outbuf_size,
	       no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     RL2_PIXEL_RGB, 3, NULL,
				     compression, 1, tile_sz,
         no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = 3 * tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     3, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  3, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       RL2_PIXEL_RGB, 3, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    rl2_destroy_tiff_destination (tiff);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_triple_band_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
				       rl2CoveragePtr cvg, double x_res,
				       double y_res, double minx, double miny,
				       double maxx, double maxy,
				       unsigned int width,
				       unsigned int height,
				       unsigned char red_band,
				       unsigned char green_band,
				       unsigned char blue_band,
				       unsigned char compression,
				       unsigned int tile_sz,
           unsigned char *geom_cutline_blob, 
           int *geom_cutline_blob_sz)
{
/* exporting a plain Band-Composed TIFF from the DBMS into the file-system */
    return export_triple_band_tiff_common (0, handle, dst_path, cvg, 0, x_res,
					   y_res, minx, miny, maxx, maxy,
					   width, height, red_band,
					   green_band, blue_band, compression,
					   tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_triple_band_tiff_from_dbms (sqlite3 * handle,
					       const char *dst_path,
					       rl2CoveragePtr cvg,
					       sqlite3_int64 section_id,
					       double x_res, double y_res,
					       double minx, double miny,
					       double maxx, double maxy,
					       unsigned int width,
					       unsigned int height,
					       unsigned char red_band,
					       unsigned char green_band,
					       unsigned char blue_band,
					       unsigned char compression,
					       unsigned int tile_sz,
            unsigned char *geom_cutline_blob, 
            int *geom_cutline_blob_sz)
{
/* exporting a plain Band-Composed TIFF - Section */
    return export_triple_band_tiff_common (1, handle, dst_path, cvg,
					   section_id, x_res, y_res, minx,
					   miny, maxx, maxy, width, height,
					   red_band, green_band, blue_band,
					   compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_mono_band_tiff_common (int by_section, sqlite3 * handle,
			      const char *dst_path, rl2CoveragePtr cvg,
			      sqlite3_int64 section_id, double x_res,
			      double y_res, double minx, double miny,
			      double maxx, double maxy, unsigned int width,
			      unsigned int height, unsigned char mono_band,
			      unsigned char compression, unsigned int tile_sz,
         unsigned char *geom_cutline_blob, 
         int *geom_cutline_blob_sz)
{
/* exporting a plain Mono-Band TIFF - common implementation */
    rl2RasterPtr raster = NULL;
    rl2PixelPtr no_data_multi = NULL;
    rl2PixelPtr no_data = NULL;
    rl2TiffDestinationPtr tiff = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    unsigned int base_x;
    unsigned int base_y;
    unsigned char out_pixel;
    char* title=NULL;
    char* abstract=NULL;
    char *copyright=NULL; 

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;
    if (rl2_get_coverage_srid (cvg, &srid) != RL2_OK)
	goto error;
    if (rl2_get_coverage_infos (cvg, title,abstract,copyright) != RL2_OK)
	goto error;
    no_data_multi = rl2_get_coverage_no_data (cvg);
    no_data = rl2_create_mono_band_pixel (no_data_multi, mono_band);

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_mono_band_raw_raster_data
	      (handle, cvg, section_id, width, height, minx, miny, maxx, maxy,
	       xx_res, yy_res, mono_band, &outbuf, &outbuf_size,
	       no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_mono_band_raw_raster_data
	      (handle, cvg, width, height, minx, miny, maxx, maxy, xx_res,
	       yy_res, mono_band, &outbuf, &outbuf_size, no_data,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

    if (sample_type == RL2_SAMPLE_UINT16)
	out_pixel = RL2_PIXEL_DATAGRID;
    else
	out_pixel = RL2_PIXEL_GRAYSCALE;

    tiff =
	rl2_create_tiff_destination (dst_path, width, height, sample_type,
				     out_pixel, 1, NULL,
				     compression, 1, tile_sz,
         no_data,title,abstract,copyright);
    if (tiff == NULL)
	goto error;
    for (base_y = 0; base_y < height; base_y += tile_sz)
      {
	  for (base_x = 0; base_x < width; base_x += tile_sz)
	    {
		/* exporting all tiles from the output buffer */
		bufpix_size = tile_sz * tile_sz;
		if (sample_type == RL2_SAMPLE_UINT16)
		    bufpix_size *= 2;
		bufpix = malloc (bufpix_size);
		if (bufpix == NULL)
		  {
		      fprintf (stderr,
			       "rl2tool Export: Insufficient Memory !!!\n");
		      goto error;
		  }
		rl2_prime_void_tile (bufpix, tile_sz, tile_sz, sample_type,
				     1, no_data);
		copy_from_outbuf_to_tile (outbuf, bufpix, sample_type,
					  1, width, height, tile_sz,
					  tile_sz, base_y, base_x);
		raster =
		    rl2_create_raster (tile_sz, tile_sz, sample_type,
				       out_pixel, 1, bufpix,
				       bufpix_size, NULL, NULL, 0, NULL);
		bufpix = NULL;
		if (raster == NULL)
		    goto error;
		if (rl2_write_tiff_tile (tiff, raster, base_y, base_x) !=
		    RL2_OK)
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
      }

    rl2_destroy_tiff_destination (tiff);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    free (outbuf);
    return RL2_OK;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    if (outbuf != NULL)
	free (outbuf);
    if (bufpix != NULL)
	free (bufpix);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_mono_band_tiff_from_dbms (sqlite3 * handle, const char *dst_path,
				     rl2CoveragePtr cvg, double x_res,
				     double y_res, double minx, double miny,
				     double maxx, double maxy,
				     unsigned int width,
				     unsigned int height,
				     unsigned char mono_band,
				     unsigned char compression,
				     unsigned int tile_sz,
         unsigned char *geom_cutline_blob, 
         int *geom_cutline_blob_sz)
{
/* exporting a plain Mono-Band TIFF from the DBMS into the file-system */
    return export_mono_band_tiff_common (0, handle, dst_path, cvg, 0, x_res,
					 y_res, minx, miny, maxx, maxy, width,
					 height, mono_band, compression,
					 tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_mono_band_tiff_from_dbms (sqlite3 * handle,
					     const char *dst_path,
					     rl2CoveragePtr cvg,
					     sqlite3_int64 section_id,
					     double x_res, double y_res,
					     double minx, double miny,
					     double maxx, double maxy,
					     unsigned int width,
					     unsigned int height,
					     unsigned char mono_band,
					     unsigned char compression,
					     unsigned int tile_sz,
          unsigned char *geom_cutline_blob, 
          int *geom_cutline_blob_sz)
{
/* exporting a plain Mono-Band TIFF from the DBMS - Section */
    return export_mono_band_tiff_common (1, handle, dst_path, cvg, section_id,
					 x_res, y_res, minx, miny, maxx, maxy,
					 width, height, mono_band,
					 compression, tile_sz,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_ascii_grid_common (int by_section, sqlite3 * handle, int max_threads,
			  const char *dst_path, rl2CoveragePtr cvg,
			  sqlite3_int64 section_id, double res, double minx,
			  double miny, double maxx, double maxy,
			  unsigned int width, unsigned int height,
			  int is_centered, int decimal_digits,
     unsigned char *geom_cutline_blob, 
     int *geom_cutline_blob_sz)
{
/* exporting an ASCII Grid common implementation */
    rl2PalettePtr palette = NULL;
    rl2AsciiGridDestinationPtr ascii = NULL;
    rl2PixelPtr pixel;
    unsigned char level;
    unsigned char scale;
    double xx_res = res;
    double yy_res = res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    double no_data = -9999.0;
    unsigned int base_y;
    unsigned char *pixels = NULL;
    int pixels_size;

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;

    if (pixel_type != RL2_PIXEL_DATAGRID || num_bands != 1)
	goto error;

    pixel = rl2_get_coverage_no_data (cvg);
    if (pixel != NULL)
      {
	  /* attempting to retrieve the NO-DATA value */
	  unsigned char st;
	  unsigned char pt;
	  unsigned char nb;
	  if (rl2_get_pixel_type (pixel, &st, &pt, &nb) == RL2_OK)
	    {
		if (st == RL2_SAMPLE_INT8)
		  {
		      char v8;
		      if (rl2_get_pixel_sample_int8 (pixel, &v8) == RL2_OK)
			  no_data = v8;
		  }
		if (st == RL2_SAMPLE_UINT8)
		  {
		      unsigned char vu8;
		      if (rl2_get_pixel_sample_uint8 (pixel, 0, &vu8) == RL2_OK)
			  no_data = vu8;
		  }
		if (st == RL2_SAMPLE_INT16)
		  {
		      short v16;
		      if (rl2_get_pixel_sample_int16 (pixel, &v16) == RL2_OK)
			  no_data = v16;
		  }
		if (st == RL2_SAMPLE_UINT16)
		  {
		      unsigned short vu16;
		      if (rl2_get_pixel_sample_uint16 (pixel, 0, &vu16) ==
			  RL2_OK)
			  no_data = vu16;
		  }
		if (st == RL2_SAMPLE_INT32)
		  {
		      int v32;
		      if (rl2_get_pixel_sample_int32 (pixel, &v32) == RL2_OK)
			  no_data = v32;
		  }
		if (st == RL2_SAMPLE_UINT32)
		  {
		      unsigned int vu32;
		      if (rl2_get_pixel_sample_uint32 (pixel, &vu32) == RL2_OK)
			  no_data = vu32;
		  }
		if (st == RL2_SAMPLE_FLOAT)
		  {
		      float vflt;
		      if (rl2_get_pixel_sample_float (pixel, &vflt) == RL2_OK)
			  no_data = vflt;
		  }
		if (st == RL2_SAMPLE_DOUBLE)
		  {
		      double vdbl;
		      if (rl2_get_pixel_sample_double (pixel, &vdbl) == RL2_OK)
			  no_data = vdbl;
		  }
	    }
      }

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, max_threads, cvg, section_id, width, height, minx,
	       miny, maxx, maxy, res, res, &pixels, &pixels_size, &palette,
	       RL2_PIXEL_DATAGRID,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, max_threads, cvg, width, height, minx, miny, maxx,
	       maxy, res, res, &pixels, &pixels_size, &palette,
	       RL2_PIXEL_DATAGRID,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

    ascii =
	rl2_create_ascii_grid_destination (dst_path, width, height,
					   xx_res, minx, miny, is_centered,
					   no_data, decimal_digits, pixels,
					   pixels_size, sample_type);
    if (ascii == NULL)
	goto error;
    pixels = NULL;		/* pix-buffer ownership now belongs to the ASCII object */
/* writing the ASCII Grid header */
    if (rl2_write_ascii_grid_header (ascii) != RL2_OK)
	goto error;
    for (base_y = 0; base_y < height; base_y++)
      {
	  /* exporting all scanlines from the output buffer */
	  unsigned int line_no;
	  if (rl2_write_ascii_grid_scanline (ascii, &line_no) != RL2_OK)
	      goto error;
      }

    rl2_destroy_ascii_grid_destination (ascii);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_OK;

  error:
    if (ascii != NULL)
	rl2_destroy_ascii_grid_destination (ascii);
    if (pixels != NULL)
	free (pixels);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_ascii_grid_from_dbms (sqlite3 * handle, int max_threads,
				 const char *dst_path, rl2CoveragePtr cvg,
				 double res, double minx, double miny,
				 double maxx, double maxy, unsigned int width,
				 unsigned int height, int is_centered,
				 int decimal_digits,
     unsigned char *geom_cutline_blob,
     int *geom_cutline_blob_sz)
{
/* exporting an ASCII Grid from the DBMS into the file-system */
    return export_ascii_grid_common (0, handle, max_threads, dst_path, cvg, 0,
				     res, minx, miny, maxx, maxy, width,
				     height, is_centered, decimal_digits,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_ascii_grid_from_dbms (sqlite3 * handle, int max_threads,
					 const char *dst_path,
					 rl2CoveragePtr cvg,
					 sqlite3_int64 section_id, double res,
					 double minx, double miny,
					 double maxx, double maxy,
					 unsigned int width,
					 unsigned int height, int is_centered,
					 int decimal_digits,
      unsigned char *geom_cutline_blob,
      int *geom_cutline_blob_sz)
{
/* exporting an ASCII Grid - Section */
    return export_ascii_grid_common (1, handle, max_threads, dst_path, cvg,
				     section_id, res, minx, miny, maxx, maxy,
				     width, height, is_centered,
				     decimal_digits,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
is_ndvi_nodata_u8 (rl2PrivPixelPtr no_data, const unsigned char *p_in)
{
/* testing for NO-DATA */
    if (no_data != NULL)
      {
	  unsigned char band;
	  int match = 0;
	  rl2PrivSamplePtr sample;
	  for (band = 0; band < no_data->nBands; band++)
	    {
		sample = no_data->Samples + band;
		if (*(p_in + band) == sample->uint8)
		    match++;
	    }
	  if (match == no_data->nBands)
	      return 1;
      }
    return 0;
}

static int
is_ndvi_nodata_u16 (rl2PrivPixelPtr no_data, const unsigned short *p_in)
{
/* testing for NO-DATA */
    if (no_data != NULL)
      {
	  unsigned char band;
	  int match = 0;
	  rl2PrivSamplePtr sample;
	  for (band = 0; band < no_data->nBands; band++)
	    {
		sample = no_data->Samples + band;
		if (*(p_in + band) == sample->uint16)
		    match++;
	    }
	  if (match == no_data->nBands)
	      return 1;
      }
    return 0;
}

static float
compute_ndvi (void *pixels, unsigned char sample_type,
	      unsigned char num_bands, unsigned short width,
	      unsigned char red_band, unsigned char nir_band,
	      unsigned short row, unsigned short col,
	      rl2PrivPixelPtr in_no_data, float out_no_data)
{
/* computing a Normalized Difference Vegetaion Index -NDVI */
    float red;
    float nir;
    unsigned char *p8;
    unsigned short *p16;
    if (sample_type == RL2_SAMPLE_UINT16)
      {
	  /* UINT16 samples */
	  p16 = (unsigned short *) pixels;
	  p16 += (row * width * num_bands) + (col * num_bands);
	  if (is_ndvi_nodata_u16 (in_no_data, p16))
	      return out_no_data;
	  red = *(p16 + red_band);
	  nir = *(p16 + nir_band);
      }
    else
      {
	  /* assuming UINT8 samples */
	  p8 = (unsigned char *) pixels;
	  p8 += (row * width * num_bands) + (col * num_bands);
	  if (is_ndvi_nodata_u8 (in_no_data, p8))
	      return out_no_data;
	  red = *(p8 + red_band);
	  nir = *(p8 + nir_band);
      }
    return (nir - red) / (nir + red);
}

static int
export_ndvi_ascii_grid_common (int by_section, sqlite3 * handle,
			       int max_threads, const char *dst_path,
			       rl2CoveragePtr cvg, sqlite3_int64 section_id,
			       double res, double minx, double miny,
			       double maxx, double maxy, unsigned int width,
			       unsigned int height, int red_band,
			       int nir_band, int is_centered,
			       int decimal_digits,
          unsigned char *geom_cutline_blob, 
          int *geom_cutline_blob_sz)
{
/* exporting an NDVI ASCII Grid common implementation */
    rl2PalettePtr palette = NULL;
    rl2PixelPtr in_no_data;
    rl2AsciiGridDestinationPtr ascii = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = res;
    double yy_res = res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    float out_no_data = -2.0;
    unsigned short row;
    unsigned short col;
    unsigned int base_y;
    unsigned char *pixels = NULL;
    int pixels_size;
    unsigned char *out_pixels = NULL;
    int out_pixels_size;
    float *po;

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    in_no_data = rl2_get_coverage_no_data (cvg);

    if (pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;

    if (red_band < 0 || red_band >= num_bands)
	goto error;

    if (nir_band < 0 || nir_band >= num_bands)
	goto error;

    if (red_band == nir_band)
	goto error;

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, max_threads, cvg, section_id, width, height, minx,
	       miny, maxx, maxy, res, res, &pixels, &pixels_size, &palette,
	       RL2_PIXEL_MULTIBAND,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, max_threads, cvg, width, height, minx, miny, maxx,
	       maxy, res, res, &pixels, &pixels_size, &palette,
	       RL2_PIXEL_MULTIBAND,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

/* creating the output NDVI raster */
    out_pixels_size = width * height * sizeof (float);
    out_pixels = malloc (out_pixels_size);
    if (out_pixels == NULL)
	goto error;
    po = (float *) out_pixels;
    for (row = 0; row < height; row++)
      {
	  /* computing NDVI */
	  for (col = 0; col < width; col++)
	      *po++ =
		  compute_ndvi (pixels, sample_type, num_bands, width,
				red_band, nir_band, row, col,
				(rl2PrivPixelPtr) in_no_data, out_no_data);
      }
    free (pixels);
    pixels = NULL;

    ascii =
	rl2_create_ascii_grid_destination (dst_path, width, height,
					   xx_res, minx, miny, is_centered,
					   out_no_data, decimal_digits,
					   out_pixels, out_pixels_size,
					   RL2_SAMPLE_FLOAT);
    if (ascii == NULL)
	goto error;
    out_pixels = NULL;		/* pix-buffer ownership now belongs to the ASCII object */
/* writing the ASCII Grid header */
    if (rl2_write_ascii_grid_header (ascii) != RL2_OK)
	goto error;
    for (base_y = 0; base_y < height; base_y++)
      {
	  /* exporting all scanlines from the output buffer */
	  unsigned int line_no;
	  if (rl2_write_ascii_grid_scanline (ascii, &line_no) != RL2_OK)
	      goto error;
      }

    rl2_destroy_ascii_grid_destination (ascii);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_OK;

  error:
    if (ascii != NULL)
	rl2_destroy_ascii_grid_destination (ascii);
    if (pixels != NULL)
	free (pixels);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_ndvi_ascii_grid_from_dbms (sqlite3 * handle, int max_threads,
				      const char *dst_path,
				      rl2CoveragePtr cvg, double res,
				      double minx, double miny, double maxx,
				      double maxy, unsigned int width,
				      unsigned int height, int red_band,
				      int nir_band, int is_centered,
				      int decimal_digits,
          unsigned char *geom_cutline_blob, 
          int *geom_cutline_blob_sz)
{
/* exporting an ASCII Grid from the DBMS into the file-system */
    return export_ndvi_ascii_grid_common (0, handle, max_threads, dst_path,
					  cvg, 0, res, minx, miny, maxx, maxy,
					  width, height, red_band, nir_band,
					  is_centered, decimal_digits,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_ndvi_ascii_grid_from_dbms (sqlite3 * handle,
					      int max_threads,
					      const char *dst_path,
					      rl2CoveragePtr cvg,
					      sqlite3_int64 section_id,
					      double res, double minx,
					      double miny, double maxx,
					      double maxy, unsigned int width,
					      unsigned int height,
					      int red_band, int nir_band,
					      int is_centered,
					      int decimal_digits,
           unsigned char *geom_cutline_blob, 
           int *geom_cutline_blob_sz)
{
/* exporting an ASCII Grid - Section */
    return export_ndvi_ascii_grid_common (1, handle, max_threads, dst_path,
					  cvg, section_id, res, minx, miny,
					  maxx, maxy, width, height, red_band,
					  nir_band, is_centered,
					  decimal_digits,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_jpeg_common (int by_section, sqlite3 * handle, int max_threads,
		    const char *dst_path, rl2CoveragePtr cvg,
		    sqlite3_int64 section_id, double x_res, double y_res,
		    double minx, double miny, double maxx, double maxy,
		    unsigned int width, unsigned int height, int quality,
		    int with_worldfile,
      unsigned char *geom_cutline_blob, 
      int *geom_cutline_blob_sz)
{
/* common implementation for Write JPEG */
    rl2SectionPtr section = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char *outbuf = NULL;
    int outbuf_size;

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (sample_type == RL2_SAMPLE_UINT8 && pixel_type == RL2_PIXEL_GRAYSCALE
	&& num_bands == 1)
	;
    else if (sample_type == RL2_SAMPLE_UINT8 && pixel_type == RL2_PIXEL_RGB
	     && num_bands == 3)
	;
    else
	goto error;

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, max_threads, cvg, section_id, width, height, minx,
	       miny, maxx, maxy, xx_res, yy_res, &outbuf, &outbuf_size, NULL,
	       pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, max_threads, cvg, width, height, minx, miny, maxx,
	       maxy, xx_res, yy_res, &outbuf, &outbuf_size, NULL,
	       pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }

    raster =
	rl2_create_raster (width, height, sample_type, pixel_type, num_bands,
			   outbuf, outbuf_size, NULL, NULL, 0, NULL);
    outbuf = NULL;
    if (raster == NULL)
	goto error;
    section =
	rl2_create_section ("jpeg", RL2_COMPRESSION_JPEG, 256, 256, raster);
    raster = NULL;
    if (section == NULL)
	goto error;
    if (rl2_section_to_jpeg (section, dst_path, quality) != RL2_OK)
	goto error;

    if (with_worldfile)
      {
	  /* exporting the JGW WorldFile */
	  write_jgw_worldfile (dst_path, minx, maxy, x_res, y_res);
      }

    rl2_destroy_section (section);
    return RL2_OK;

  error:
    if (section != NULL)
	rl2_destroy_section (section);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (outbuf != NULL)
	free (outbuf);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_jpeg_from_dbms (sqlite3 * handle, int max_threads,
			   const char *dst_path, rl2CoveragePtr cvg,
			   double x_res, double y_res, double minx,
			   double miny, double maxx, double maxy,
			   unsigned int width, unsigned int height,
			   int quality, int with_worldfile,
      unsigned char *geom_cutline_blob, 
      int *geom_cutline_blob_sz)
{
/* exporting a JPEG (with possible JGW) from the DBMS into the file-system */
    return export_jpeg_common (0, handle, max_threads, dst_path, cvg, 0,
			       x_res, y_res, minx, miny, maxx, maxy, width,
			       height, quality, with_worldfile,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_jpeg_from_dbms (sqlite3 * handle, int max_threads,
				   const char *dst_path, rl2CoveragePtr cvg,
				   sqlite3_int64 section_id, double x_res,
				   double y_res, double minx, double miny,
				   double maxx, double maxy,
				   unsigned int width, unsigned int height,
				   int quality, int with_worldfile,
       unsigned char *geom_cutline_blob, 
       int *geom_cutline_blob_sz)
{
/* exporting a JPEG (with possible JGW) - Section */
    return export_jpeg_common (1, handle, max_threads, dst_path, cvg,
			       section_id, x_res, y_res, minx, miny, maxx,
			       maxy, width, height, quality, with_worldfile,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
export_raw_pixels_common (int by_section, sqlite3 * handle, int max_threads,
			  rl2CoveragePtr cvg, sqlite3_int64 section_id,
			  double x_res, double y_res, double minx,
			  double miny, double maxx, double maxy,
			  unsigned int width, unsigned int height,
			  int big_endian, unsigned char **blob, int *blob_size,
     unsigned char *geom_cutline_blob, 
     int *geom_cutline_blob_sz)
{
/* common implementation for Export RAW pixels */
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char *bufpix;
    unsigned char *outbuf = NULL;
    int outbuf_size;

    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	return RL2_ERROR;

    if (mismatching_size
	(width, height, xx_res, yy_res, minx, miny, maxx, maxy))
	goto error;

    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;

    if (by_section)
      {
	  /* single Section */
	  if (rl2_get_section_raw_raster_data
	      (handle, max_threads, cvg, section_id, width, height, minx,
	       miny, maxx, maxy, xx_res, yy_res, &outbuf, &outbuf_size, NULL,
	       pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    else
      {
	  /* whole Coverage */
	  if (rl2_get_raw_raster_data
	      (handle, max_threads, cvg, width, height, minx, miny, maxx,
	       maxy, xx_res, yy_res, &outbuf, &outbuf_size, NULL,
	       pixel_type,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
	      goto error;
      }
    bufpix =
	rl2_copy_endian_raw_pixels (outbuf, outbuf_size, width, height,
				    sample_type, num_bands, big_endian);
    if (bufpix == NULL)
	goto error;
    *blob = bufpix;
    *blob_size = outbuf_size;
    free (outbuf);
    return RL2_OK;

  error:
    if (outbuf != NULL)
	free (outbuf);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_export_raw_pixels_from_dbms (sqlite3 * handle, int max_threads,
				 rl2CoveragePtr coverage, double x_res,
				 double y_res, double minx, double miny,
				 double maxx, double maxy,
				 unsigned int width,
				 unsigned int height, int big_endian,
				 unsigned char **blob, int *blob_size,
     unsigned char *geom_cutline_blob, 
     int *geom_cutline_blob_sz)
{
/* exporting RAW pixel buffer and Transparency Mask from the DBMS */
    return export_raw_pixels_common (0, handle, max_threads, coverage, 0,
				     x_res, y_res, minx, miny, maxx, maxy,
				     width, height, big_endian, blob,
				     blob_size,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_export_section_raw_pixels_from_dbms (sqlite3 * handle, int max_threads,
					 rl2CoveragePtr coverage,
					 sqlite3_int64 section_id,
					 double x_res, double y_res,
					 double minx, double miny,
					 double maxx, double maxy,
					 unsigned int width,
					 unsigned int height,
					 int big_endian,
					 unsigned char **blob, int *blob_size,
      unsigned char *geom_cutline_blob, 
      int *geom_cutline_blob_sz)
{
/* exporting RAW pixel buffer and Transparency Mask - Section */
    return export_raw_pixels_common (1, handle, max_threads, coverage,
				     section_id, x_res, y_res, minx, miny,
				     maxx, maxy, width, height, big_endian,
				     blob, blob_size,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_load_raw_raster_into_dbms (sqlite3 * handle, int max_threads,
			       rl2CoveragePtr cvg, const char *section,
			       rl2RasterPtr rst, int pyramidize)
{
/* main IMPORT Raster function */
    rl2PrivCoveragePtr privcvg = (rl2PrivCoveragePtr) cvg;
    rl2PrivRasterPtr privrst = (rl2PrivRasterPtr) rst;
    int ret;
    char *sql;
    const char *coverage;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned int tile_w;
    unsigned int tile_h;
    unsigned char compression;
    int quality;
    char *table;
    char *xtable;
    unsigned int tileWidth;
    unsigned int tileHeight;
    unsigned int width;
    unsigned int height;
    int srid;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double tile_minx;
    double tile_maxy;
    rl2RasterStatisticsPtr section_stats = NULL;
    rl2PixelPtr no_data = NULL;
    rl2PalettePtr aux_palette = NULL;
    unsigned int row;
    unsigned int col;
    double res_x;
    double res_y;
    double base_res_x;
    double base_res_y;
    char *xml_summary = NULL;
    sqlite3_stmt *stmt_data = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_sect = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_upd_sect = NULL;
    sqlite3_int64 section_id;
    rl2AuxImporterPtr aux = NULL;
    rl2AuxImporterTilePtr aux_tile;
    rl2AuxImporterTilePtr *thread_slots = NULL;
    int thread_count;

    if (cvg == NULL)
	goto error;
    if (section == NULL)
	goto error;
    if (rst == NULL)
	goto error;

    if (rl2_get_coverage_tile_size (cvg, &tileWidth, &tileHeight) != RL2_OK)
	goto error;
    if (rl2_get_raster_size (rst, &width, &height) != RL2_OK)
	goto error;
    if (rl2_get_raster_srid (rst, &srid) != RL2_OK)
	goto error;
    if (rl2_get_raster_extent (rst, &minx, &miny, &maxx, &maxy) != RL2_OK)
	goto error;

    tile_w = tileWidth;
    tile_h = tileHeight;
    rl2_get_coverage_compression (cvg, &compression, &quality);
    rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands);
    coverage = rl2_get_coverage_name (cvg);

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (section_id, section_name, file_path, "
	 "md5_checksum, summary, width, height, geometry) "
	 "VALUES (NULL, ?, ?, ?, XB_Create(?), ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO sections SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET statistics = ? WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_upd_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE sections SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

    if (privcvg->mixedResolutions)
      {
	  /* mixed resolutions Coverage */
	  table = sqlite3_mprintf ("%s_section_levels", coverage);
	  xtable = rl2_double_quoted_sql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf
	      ("INSERT OR IGNORE INTO \"%s\" (section_id, pyramid_level, "
	       "x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	       "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	       "VALUES (?, 0, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
	  free (xtable);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("INSERT INTO section_levels SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }
    else
      {
	  /* single resolution Coverage */
	  table = sqlite3_mprintf ("%s_levels", coverage);
	  xtable = rl2_double_quoted_sql (table);
	  sqlite3_free (table);
	  sql =
	      sqlite3_mprintf
	      ("INSERT OR IGNORE INTO \"%s\" (pyramid_level, "
	       "x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	       "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	       "VALUES (0, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
	  free (xtable);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_levl, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("INSERT INTO levels SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }

    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, 0, ?, BuildMBR(?, ?, ?, ?, ?))", xtable);
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
    res_x = privrst->hResolution;
    res_y = privrst->vResolution;
    base_res_x = privcvg->hResolution;
    base_res_y = privcvg->vResolution;
    xml_summary = rl2_build_raw_pixels_xml_summary (rst);

/* INSERTing the section */
    if (!rl2_do_insert_section
	(handle, "loaded from RAW pixels", section, srid, width, height, minx,
	 miny, maxx, maxy, xml_summary, privcvg->sectionPaths,
	 privcvg->sectionMD5, privcvg->sectionSummary, stmt_sect, &section_id))
	goto error;
    section_stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (section_stats == NULL)
	goto error;
/* INSERTing the base-levels */
    if (privcvg->mixedResolutions)
      {
	  /* multiple resolutions Coverage */
	  if (!rl2_do_insert_section_levels
	      (handle, section_id, res_x, res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }
    else
      {
	  /* single resolution Coverage */
	  if (!rl2_do_insert_levels
	      (handle, base_res_x, base_res_y, 1.0, sample_type, stmt_levl))
	      goto error;
      }

/* preparing all Tile Requests */
    aux =
	createAuxImporter (privcvg, srid, maxx, miny, tile_w, tile_h, res_x,
			   res_y, RL2_ORIGIN_RAW, rst, RL2_CONVERT_NO, 1,
			   compression, quality);
    tile_maxy = maxy;
    for (row = 0; row < height; row += tile_h)
      {
	  tile_minx = minx;
	  for (col = 0; col < width; col += tile_w)
	    {
		/* adding a Tile request */
		addTile2AuxImporter (aux, row, col, tile_minx, tile_maxy);
		tile_minx += (double) tile_w *res_x;
	    }
	  tile_maxy -= (double) tile_h *res_y;
      }

    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;
/* prepating the thread_slots stuct */
    thread_slots = malloc (sizeof (rl2AuxImporterTilePtr) * max_threads);
    for (thread_count = 0; thread_count < max_threads; thread_count++)
	*(thread_slots + thread_count) = NULL;
    thread_count = 0;
    aux_tile = aux->first;
    while (aux_tile != NULL)
      {
	  /* processing a Tile request (may be under parallel execution) */
	  if (max_threads > 1)
	    {
		/* adopting a multithreaded strategy */
		do_get_tile (aux_tile);
		*(thread_slots + thread_count) = aux_tile;
		thread_count++;
		start_tile_thread (aux_tile);
		if (thread_count == max_threads || aux_tile->next == NULL)
		  {
		      /* waiting until all child threads exit */
#ifdef _WIN32
		      HANDLE *handles;
		      int z;
		      int cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* counting how many active threads we currently have */
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    cnt++;
			}
		      handles = malloc (sizeof (HANDLE) * cnt);
		      cnt = 0;
		      for (z = 0; z < max_threads; z++)
			{
			    /* initializing the HANDLEs array */
			    HANDLE *pOpaque;
			    rl2AuxImporterTilePtr pTile = *(thread_slots + z);
			    if (pTile == NULL)
				continue;
			    pOpaque = (HANDLE *) (pTile->opaque_thread_id);
			    *(handles + cnt) = *pOpaque;
			    cnt++;
			}
		      WaitForMultipleObjects (cnt, handles, TRUE, INFINITE);
#else
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    pthread_t *pOpaque;
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    pOpaque = (pthread_t *) (pTile->opaque_thread_id);
			    pthread_join (*pOpaque, NULL);
			}
#endif

		      /* all children threads have now finished: resuming the main thread */
		      for (thread_count = 0; thread_count < max_threads;
			   thread_count++)
			{
			    /* checking for eventual errors */
			    rl2AuxImporterTilePtr pTile =
				*(thread_slots + thread_count);
			    if (pTile == NULL)
				continue;
			    if (pTile->retcode != RL2_OK)
				goto error;
			}
#ifdef _WIN32
		      free (handles);
#endif
		      thread_count = 0;
		      /* we can now continue by inserting all tiles into the DBMS */
		  }
		else
		  {
		      aux_tile = aux_tile->next;
		      continue;
		  }
	    }
	  else
	    {
		/* single thread execution */
		do_get_tile (aux_tile);
		*(thread_slots + 0) = aux_tile;
		do_encode_tile (aux_tile);
		if (aux_tile->retcode != RL2_OK)
		    goto error;
	    }

	  /* INSERTing the tile */
	  aux_palette =
	      rl2_clone_palette (rl2_get_raster_palette (aux_tile->raster));
	  if (!do_insert_tile
	      (handle, aux_tile->blob_odd, aux_tile->blob_odd_sz,
	       aux_tile->blob_even, aux_tile->blob_even_sz, section_id, srid,
	       aux_tile->minx, aux_tile->miny, aux_tile->maxx, aux_tile->maxy,
	       aux_palette, no_data, stmt_tils, stmt_data, section_stats))
	    {
		aux_tile->blob_odd = NULL;
		aux_tile->blob_even = NULL;
		goto error;
	    }
	  doAuxImporterTileCleanup (aux_tile);
	  aux_tile = aux_tile->next;
      }
    destroyAuxImporter (aux);
    aux = NULL;
    free (thread_slots);
    thread_slots = NULL;

/* updating the Section's Statistics */
    compute_aggregate_sq_diff (section_stats);
    if (rl2_get_defaults_stats(handle, section_stats,coverage,srid,(minx+((maxx-minx)/2)),(miny+((maxy-miny)/2))))
     goto error;
    if (!rl2_do_insert_stats (handle, section_stats, section_id, stmt_upd_sect))
	goto error;

    rl2_destroy_raster_statistics (section_stats);
    section_stats = NULL;

    if (pyramidize)
      {
	  /* immediately building the Section's Pyramid */
	  const char *coverage_name = rl2_get_coverage_name (cvg);
	  if (coverage_name == NULL)
	      goto error;
	  if (rl2_build_section_pyramid
	      (handle, max_threads, coverage_name, section_id, 1, 0) != RL2_OK)
	    {
		fprintf (stderr, "unable to build the Section's Pyramid\n");
		goto error;
	    }
      }

    sqlite3_finalize (stmt_upd_sect);
    sqlite3_finalize (stmt_sect);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    stmt_upd_sect = NULL;
    stmt_sect = NULL;
    stmt_levl = NULL;
    stmt_tils = NULL;
    stmt_data = NULL;

    if (rl2_update_dbms_coverage (handle, coverage) != RL2_OK)
      {
	  fprintf (stderr, "unable to update the Coverage\n");
	  goto error;
      }

    return RL2_OK;

  error:
    if (aux != NULL)
	destroyAuxImporter (aux);
    if (thread_slots != NULL)
	free (thread_slots);
    if (stmt_upd_sect != NULL)
	sqlite3_finalize (stmt_upd_sect);
    if (stmt_sect != NULL)
	sqlite3_finalize (stmt_sect);
    if (stmt_levl != NULL)
	sqlite3_finalize (stmt_levl);
    if (stmt_tils != NULL)
	sqlite3_finalize (stmt_tils);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    return RL2_ERROR;
}
