/*

 rl2sqlaux -- private SQL helper methods

 version 0.1, 2014 February 28

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
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2wms.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

#include <spatialite/gg_const.h>

struct aux_raster_image
{
/* a common struct for PNG/JPEG/etc output images */
    unsigned char bg_red;
    unsigned char bg_green;
    unsigned char bg_blue;
    int transparent;
    const char *format;
    int quality;
    unsigned char *img;
    int img_size;
};

struct aux_raster_render
{
/* an ancillary struct for Map Raster rendering */
    sqlite3 *sqlite;
    const void *data;
    rl2CanvasPtr canvas;
    const char *db_prefix;
    const char *cvg_name;
    const unsigned char *blob;
    int blob_sz;
    int width;
    int height;
    int reaspect;
    const char *style;
    unsigned char *xml_style;
    unsigned char out_pixel;
    struct aux_raster_image *output;
};

struct aux_vector_render
{
/* an ancillary struct for Map Vector rendering */
    sqlite3 *sqlite;
    const void *data;
    rl2CanvasPtr canvas;
    const char *db_prefix;
    const char *cvg_name;
    const unsigned char *blob;
    int blob_sz;
    int width;
    int height;
    int reaspect;
    const char *style;
    const unsigned char *quick_style;
    int with_nodes;
    int with_edges_or_links;
    int with_faces;
    int with_edge_or_link_seeds;
    int with_face_seeds;
    struct aux_raster_image *output;
};

#define RL2_UNUSED() if (argc || argv) argc = argc;

RL2_PRIVATE int
get_coverage_sample_bands (sqlite3 * sqlite, const char *db_prefix,
			   const char *coverage, unsigned char *sample_type,
			   unsigned char *num_bands)
{
/* attempting to retrieve the SampleType and NumBands from a Coverage */
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    int ret;
    const char *sample;
    int bands;
    unsigned char xsample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char xnum_bands = RL2_BANDS_UNKNOWN;
    char *xdb_prefix;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);

    sql =
	sqlite3_mprintf
	("SELECT sample_type, num_bands FROM \"%s\".raster_coverages "
	 "WHERE Lower(coverage_name) = Lower(%Q)", xdb_prefix, coverage);
    free (xdb_prefix);
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
		sample = results[(i * columns) + 0];
		if (strcmp (sample, "1-BIT") == 0)
		    xsample_type = RL2_SAMPLE_1_BIT;
		if (strcmp (sample, "2-BIT") == 0)
		    xsample_type = RL2_SAMPLE_2_BIT;
		if (strcmp (sample, "4-BIT") == 0)
		    xsample_type = RL2_SAMPLE_4_BIT;
		if (strcmp (sample, "INT8") == 0)
		    xsample_type = RL2_SAMPLE_INT8;
		if (strcmp (sample, "UINT8") == 0)
		    xsample_type = RL2_SAMPLE_UINT8;
		if (strcmp (sample, "INT16") == 0)
		    xsample_type = RL2_SAMPLE_INT16;
		if (strcmp (sample, "UINT16") == 0)
		    xsample_type = RL2_SAMPLE_UINT16;
		if (strcmp (sample, "INT32") == 0)
		    xsample_type = RL2_SAMPLE_INT32;
		if (strcmp (sample, "UINT32") == 0)
		    xsample_type = RL2_SAMPLE_UINT32;
		if (strcmp (sample, "FLOAT") == 0)
		    xsample_type = RL2_SAMPLE_FLOAT;
		if (strcmp (sample, "DOUBLE") == 0)
		    xsample_type = RL2_SAMPLE_DOUBLE;
		bands = atoi (results[(i * columns) + 1]);
		if (bands > 0 && bands < 256)
		    xnum_bands = bands;
	    }
      }
    sqlite3_free_table (results);
    if (xsample_type == RL2_SAMPLE_UNKNOWN || xnum_bands == RL2_BANDS_UNKNOWN)
	return 0;
    *sample_type = xsample_type;
    *num_bands = xnum_bands;
    return 1;
}

RL2_PRIVATE int
get_coverage_defs (sqlite3 * sqlite, const char *db_prefix,
		   const char *coverage, unsigned int *tile_width,
		   unsigned int *tile_height, unsigned char *sample_type,
		   unsigned char *pixel_type, unsigned char *num_bands,
		   unsigned char *compression)
{
/* attempting to retrieve the main definitions from a Coverage */
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    int ret;
    const char *sample;
    const char *pixel;
    const char *compr;
    int bands;
    unsigned char xsample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char xpixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char xnum_bands = RL2_BANDS_UNKNOWN;
    unsigned char xcompression = RL2_COMPRESSION_UNKNOWN;
    unsigned short xtile_width = RL2_TILESIZE_UNDEFINED;
    unsigned short xtile_height = RL2_TILESIZE_UNDEFINED;
    char *xdb_prefix;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    sql = sqlite3_mprintf ("SELECT sample_type, pixel_type, num_bands, "
			   "compression, tile_width, tile_height FROM \"%s\".raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)",
			   xdb_prefix, coverage);
    free (xdb_prefix);
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
		sample = results[(i * columns) + 0];
		if (strcmp (sample, "1-BIT") == 0)
		    xsample_type = RL2_SAMPLE_1_BIT;
		if (strcmp (sample, "2-BIT") == 0)
		    xsample_type = RL2_SAMPLE_2_BIT;
		if (strcmp (sample, "4-BIT") == 0)
		    xsample_type = RL2_SAMPLE_4_BIT;
		if (strcmp (sample, "INT8") == 0)
		    xsample_type = RL2_SAMPLE_INT8;
		if (strcmp (sample, "UINT8") == 0)
		    xsample_type = RL2_SAMPLE_UINT8;
		if (strcmp (sample, "INT16") == 0)
		    xsample_type = RL2_SAMPLE_INT16;
		if (strcmp (sample, "UINT16") == 0)
		    xsample_type = RL2_SAMPLE_UINT16;
		if (strcmp (sample, "INT32") == 0)
		    xsample_type = RL2_SAMPLE_INT32;
		if (strcmp (sample, "UINT32") == 0)
		    xsample_type = RL2_SAMPLE_UINT32;
		if (strcmp (sample, "FLOAT") == 0)
		    xsample_type = RL2_SAMPLE_FLOAT;
		if (strcmp (sample, "DOUBLE") == 0)
		    xsample_type = RL2_SAMPLE_DOUBLE;
		pixel = results[(i * columns) + 1];
		if (strcmp (pixel, "MONOCHROME") == 0)
		    xpixel_type = RL2_PIXEL_MONOCHROME;
		if (strcmp (pixel, "PALETTE") == 0)
		    xpixel_type = RL2_PIXEL_PALETTE;
		if (strcmp (pixel, "GRAYSCALE") == 0)
		    xpixel_type = RL2_PIXEL_GRAYSCALE;
		if (strcmp (pixel, "RGB") == 0)
		    xpixel_type = RL2_PIXEL_RGB;
		if (strcmp (pixel, "MULTIBAND") == 0)
		    xpixel_type = RL2_PIXEL_MULTIBAND;
		if (strcmp (pixel, "DATAGRID") == 0)
		    xpixel_type = RL2_PIXEL_DATAGRID;
		bands = atoi (results[(i * columns) + 2]);
		if (bands > 0 && bands < 256)
		    xnum_bands = bands;
		compr = results[(i * columns) + 3];
		if (strcmp (compr, "NONE") == 0)
		    xcompression = RL2_COMPRESSION_NONE;
		if (strcmp (compr, "DEFLATE") == 0)
		    xcompression = RL2_COMPRESSION_DEFLATE;
		if (strcmp (compr, "DEFLATE_NO") == 0)
		    xcompression = RL2_COMPRESSION_DEFLATE_NO;
		if (strcmp (compr, "LZ4") == 0)
		    xcompression = RL2_COMPRESSION_LZ4;
		if (strcmp (compr, "LZ4_NO") == 0)
		    xcompression = RL2_COMPRESSION_LZ4_NO;
		if (strcmp (compr, "ZSTD") == 0)
		    xcompression = RL2_COMPRESSION_ZSTD;
		if (strcmp (compr, "ZSTD_NO") == 0)
		    xcompression = RL2_COMPRESSION_ZSTD_NO;
		if (strcmp (compr, "LZMA") == 0)
		    xcompression = RL2_COMPRESSION_LZMA;
		if (strcmp (compr, "LZMA_NO") == 0)
		    xcompression = RL2_COMPRESSION_LZMA_NO;
		if (strcmp (compr, "PNG") == 0)
		    xcompression = RL2_COMPRESSION_PNG;
		if (strcmp (compr, "JPEG") == 0)
		    xcompression = RL2_COMPRESSION_JPEG;
		if (strcmp (compr, "LOSSY_WEBP") == 0)
		    xcompression = RL2_COMPRESSION_LOSSY_WEBP;
		if (strcmp (compr, "LOSSLESS_WEBP") == 0)
		    xcompression = RL2_COMPRESSION_LOSSLESS_WEBP;
		if (strcmp (compr, "CCITTFAX4") == 0)
		    xcompression = RL2_COMPRESSION_CCITTFAX4;
		if (strcmp (compr, "CHARLS") == 0)
		    xcompression = RL2_COMPRESSION_CHARLS;
		if (strcmp (compr, "LOSSY_JP2") == 0)
		    xcompression = RL2_COMPRESSION_LOSSY_JP2;
		if (strcmp (compr, "LOSSLESS_JP2") == 0)
		    xcompression = RL2_COMPRESSION_LOSSLESS_JP2;
		xtile_width = atoi (results[(i * columns) + 4]);
		xtile_height = atoi (results[(i * columns) + 5]);
	    }
      }
    sqlite3_free_table (results);
    if (xsample_type == RL2_SAMPLE_UNKNOWN || xpixel_type == RL2_PIXEL_UNKNOWN
	|| xnum_bands == RL2_BANDS_UNKNOWN
	|| xcompression == RL2_COMPRESSION_UNKNOWN
	|| xtile_width == RL2_TILESIZE_UNDEFINED
	|| xtile_height == RL2_TILESIZE_UNDEFINED)
	return 0;
    *sample_type = xsample_type;
    *pixel_type = xpixel_type;
    *num_bands = xnum_bands;
    *compression = xcompression;
    *tile_width = xtile_width;
    *tile_height = xtile_height;
    return 1;
}

RL2_PRIVATE rl2PixelPtr
default_nodata (unsigned char sample, unsigned char pixel,
		unsigned char num_bands)
{
/* creating a default NO-DATA value */
    int nb;
    rl2PixelPtr pxl = rl2_create_pixel (sample, pixel, num_bands);
    if (pxl == NULL)
	return NULL;
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  rl2_set_pixel_sample_1bit (pxl, 0);
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (sample)
	    {
	    case RL2_SAMPLE_1_BIT:
		rl2_set_pixel_sample_1bit (pxl, 0);
		break;
	    case RL2_SAMPLE_2_BIT:
		rl2_set_pixel_sample_2bit (pxl, 0);
		break;
	    case RL2_SAMPLE_4_BIT:
		rl2_set_pixel_sample_4bit (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (sample)
	    {
	    case RL2_SAMPLE_1_BIT:
		rl2_set_pixel_sample_1bit (pxl, 1);
		break;
	    case RL2_SAMPLE_2_BIT:
		rl2_set_pixel_sample_2bit (pxl, 3);
		break;
	    case RL2_SAMPLE_4_BIT:
		rl2_set_pixel_sample_4bit (pxl, 15);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_RGB:
	  switch (sample)
	    {
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 255);
		rl2_set_pixel_sample_uint8 (pxl, 1, 255);
		rl2_set_pixel_sample_uint8 (pxl, 2, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		rl2_set_pixel_sample_uint16 (pxl, 1, 0);
		rl2_set_pixel_sample_uint16 (pxl, 2, 0);
		break;
	    };
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (sample)
	    {
	    case RL2_SAMPLE_INT8:
		rl2_set_pixel_sample_int8 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_set_pixel_sample_uint8 (pxl, 0, 0);
		break;
	    case RL2_SAMPLE_INT16:
		rl2_set_pixel_sample_int16 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT16:
		rl2_set_pixel_sample_uint16 (pxl, 0, 0);
		break;
	    case RL2_SAMPLE_INT32:
		rl2_set_pixel_sample_int32 (pxl, 0);
		break;
	    case RL2_SAMPLE_UINT32:
		rl2_set_pixel_sample_uint32 (pxl, 0);
		break;
	    case RL2_SAMPLE_FLOAT:
		rl2_set_pixel_sample_float (pxl, 0.0);
		break;
	    case RL2_SAMPLE_DOUBLE:
		rl2_set_pixel_sample_double (pxl, 0.0);
		break;
	    };
	  break;
      case RL2_PIXEL_MULTIBAND:
	  switch (sample)
	    {
	    case RL2_SAMPLE_UINT8:
		for (nb = 0; nb < num_bands; nb++)
		    rl2_set_pixel_sample_uint8 (pxl, nb, 255);
		break;
	    case RL2_SAMPLE_UINT16:
		for (nb = 0; nb < num_bands; nb++)
		    rl2_set_pixel_sample_uint16 (pxl, nb, 0);
		break;
	    };
	  break;
      };
    return pxl;
}

RL2_PRIVATE WmsRetryListPtr
alloc_retry_list ()
{
/* initializing an empty WMS retry-list */
    WmsRetryListPtr lst = malloc (sizeof (WmsRetryList));
    lst->first = NULL;
    lst->last = NULL;
    return lst;
}

RL2_PRIVATE void
free_retry_list (WmsRetryListPtr lst)
{
/* memory cleanup - destroying a list of WMS retry requests */
    WmsRetryItemPtr p;
    WmsRetryItemPtr pn;
    if (lst == NULL)
	return;
    p = lst->first;
    while (p != NULL)
      {
	  pn = p->next;
	  free (p);
	  p = pn;
      }
    free (lst);
}

RL2_PRIVATE void
add_retry (WmsRetryListPtr lst, double minx, double miny, double maxx,
	   double maxy)
{
/* inserting a WMS retry request into the list */
    WmsRetryItemPtr p;
    if (lst == NULL)
	return;
    p = malloc (sizeof (WmsRetryItem));
    p->done = 0;
    p->count = 0;
    p->minx = minx;
    p->miny = miny;
    p->maxx = maxx;
    p->maxy = maxy;
    p->next = NULL;
    if (lst->first == NULL)
	lst->first = p;
    if (lst->last != NULL)
	lst->last->next = p;
    lst->last = p;
}

RL2_PRIVATE int
rl2_parse_point (sqlite3 * handle, const unsigned char *blob, int blob_sz,
		 double *x, double *y, int *srid)
{
/* attempts to get the coordinates from a POINT */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    double pt_x;
    double pt_y;
    int srid_val;
    int count = 0;

    sql = "SELECT ST_X(?), ST_Y(?), ST_SRID(?) WHERE ST_GeometryType(?) IN "
	"('POINT', 'POINT Z', 'POINT M', 'POINT ZM')";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT rl2_parse_point SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 2, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 3, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 4, blob, blob_sz, SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		pt_x = sqlite3_column_double (stmt, 0);
		pt_y = sqlite3_column_double (stmt, 1);
		srid_val = sqlite3_column_int (stmt, 2);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT rl2_parse_point; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return RL2_ERROR;
    *x = pt_x;
    *y = pt_y;
    *srid = srid_val;
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_PRIVATE int
rl2_parse_point_generic (sqlite3 * handle, const unsigned char *blob,
			 int blob_sz, double *x, double *y)
{
/* attempts to get X,Y coordinates from a generic Geometry */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    double pt_x;
    double pt_y;
    int count = 0;

    sql = "SELECT ST_X(ST_GeometryN(DissolvePoints(?), 1)), "
	"ST_Y(ST_GeometryN(DissolvePoints(?), 1))";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT rl2_parse_point_generic SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 2, blob, blob_sz, SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		pt_x = sqlite3_column_double (stmt, 0);
		pt_y = sqlite3_column_double (stmt, 1);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT rl2_parse_point_generic; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return RL2_ERROR;
    *x = pt_x;
    *y = pt_y;
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_PRIVATE int
rl2_parse_bbox (sqlite3 * handle, const unsigned char *blob, int blob_sz,
		double *minx, double *miny, double *maxx, double *maxy)
{
/* attempts the get the BBOX from a BLOB geometry request */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    double mnx;
    double mny;
    double mxx;
    double mxy;
    int count = 0;

    sql = "SELECT MBRMinX(?), MBRMinY(?), MBRMaxX(?), MBRMaxY(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT rl2_parse_bbox SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 2, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 3, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 4, blob, blob_sz, SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		mnx = sqlite3_column_double (stmt, 0);
		mny = sqlite3_column_double (stmt, 1);
		mxx = sqlite3_column_double (stmt, 2);
		mxy = sqlite3_column_double (stmt, 3);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT rl2_parse_bbox; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return RL2_ERROR;
    *minx = mnx;
    *miny = mny;
    *maxx = mxx;
    *maxy = mxy;
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_PRIVATE int
rl2_parse_bbox_srid (sqlite3 * handle, const unsigned char *blob, int blob_sz,
		     int *srid, double *minx, double *miny, double *maxx,
		     double *maxy)
{
/* attempts the get the BBOX and SRID from a BLOB geometry request */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int srd;
    double mnx;
    double mny;
    double mxx;
    double mxy;
    int count = 0;

    sql = "SELECT ST_Srid(?), MBRMinX(?), MBRMinY(?), MBRMaxX(?), MBRMaxY(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT rl2_parse_bbox SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 2, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 3, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 4, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 5, blob, blob_sz, SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		srd = sqlite3_column_int (stmt, 0);
		mnx = sqlite3_column_double (stmt, 1);
		mny = sqlite3_column_double (stmt, 2);
		mxx = sqlite3_column_double (stmt, 3);
		mxy = sqlite3_column_double (stmt, 4);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT rl2_parse_bbox; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return RL2_ERROR;
    *srid = srd;
    *minx = mnx;
    *miny = mny;
    *maxx = mxx;
    *maxy = mxy;
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_PRIVATE int
rl2_build_bbox (sqlite3 * handle, int srid, double minx, double miny,
		double maxx, double maxy, unsigned char **blob, int *blob_sz)
{
/* building an MBR (Envelope) */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    const unsigned *x_blob;
    unsigned char *p_blob;
    int p_blob_sz;
    int count = 0;

    sql = "SELECT BuildMBR(?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT rl2_build_bbox SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, minx);
    sqlite3_bind_double (stmt, 2, miny);
    sqlite3_bind_double (stmt, 3, maxx);
    sqlite3_bind_double (stmt, 4, maxy);
    sqlite3_bind_int (stmt, 5, srid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      x_blob = sqlite3_column_blob (stmt, 0);
		      p_blob_sz = sqlite3_column_bytes (stmt, 0);
		      p_blob = malloc (p_blob_sz);
		      memcpy (p_blob, x_blob, p_blob_sz);
		      count++;
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT rl2_build_bbox; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return RL2_ERROR;
    *blob = p_blob;
    *blob_sz = p_blob_sz;
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

static int
do_insert_wms_tile (sqlite3 * handle, unsigned char *blob_odd,
		    int blob_odd_sz, unsigned char *blob_even,
		    int blob_even_sz, sqlite3_int64 section_id, int srid,
		    double res_x, double res_y, unsigned int tile_w,
		    unsigned int tile_h, double miny, double maxx,
		    double tile_minx, double tile_miny, double tile_maxx,
		    double tile_maxy, rl2PalettePtr aux_palette,
		    rl2PixelPtr no_data, sqlite3_stmt * stmt_tils,
		    sqlite3_stmt * stmt_data,
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
    tile_maxx = tile_minx + ((double) tile_w * res_x);
    if (tile_maxx > maxx)
	tile_maxx = maxx;
    tile_miny = tile_maxy - ((double) tile_h * res_y);
    if (tile_miny < miny)
	tile_miny = miny;
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

RL2_PRIVATE int
rl2_do_insert_levels (sqlite3 * handle, double base_res_x, double base_res_y,
		      double factor, unsigned char sample_type,
		      sqlite3_stmt * stmt_levl)
{
/* INSERTing the base-levels - single resolution Coverage */
    int ret;
    double res_x = base_res_x * factor;
    double res_y = base_res_y * factor;
    sqlite3_reset (stmt_levl);
    sqlite3_clear_bindings (stmt_levl);
    sqlite3_bind_double (stmt_levl, 1, res_x);
    sqlite3_bind_double (stmt_levl, 2, res_y);
    if (sample_type == RL2_SAMPLE_1_BIT || sample_type == RL2_SAMPLE_2_BIT
	|| sample_type == RL2_SAMPLE_4_BIT)
      {
	  sqlite3_bind_null (stmt_levl, 3);
	  sqlite3_bind_null (stmt_levl, 4);
	  sqlite3_bind_null (stmt_levl, 5);
	  sqlite3_bind_null (stmt_levl, 6);
	  sqlite3_bind_null (stmt_levl, 7);
	  sqlite3_bind_null (stmt_levl, 8);
      }
    else
      {
	  sqlite3_bind_double (stmt_levl, 3, res_x * 2.0);
	  sqlite3_bind_double (stmt_levl, 4, res_y * 2.0);
	  sqlite3_bind_double (stmt_levl, 5, res_x * 4.0);
	  sqlite3_bind_double (stmt_levl, 6, res_y * 4.0);
	  sqlite3_bind_double (stmt_levl, 7, res_x * 8.0);
	  sqlite3_bind_double (stmt_levl, 8, res_y * 8.0);
      }
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

RL2_PRIVATE int
rl2_do_insert_section_levels (sqlite3 * handle, sqlite3_int64 section_id,
			      double base_res_x, double base_res_y,
			      double factor, unsigned char sample_type,
			      sqlite3_stmt * stmt_levl)
{
/* INSERTing the base-levels - mixed resolution Coverage */
    int ret;
    double res_x = base_res_x * factor;
    double res_y = base_res_y * factor;
    sqlite3_reset (stmt_levl);
    sqlite3_clear_bindings (stmt_levl);
    sqlite3_bind_int64 (stmt_levl, 1, section_id);
    sqlite3_bind_double (stmt_levl, 2, res_x);
    sqlite3_bind_double (stmt_levl, 3, res_y);
    if (sample_type == RL2_SAMPLE_1_BIT || sample_type == RL2_SAMPLE_2_BIT
	|| sample_type == RL2_SAMPLE_4_BIT)
      {
	  sqlite3_bind_null (stmt_levl, 4);
	  sqlite3_bind_null (stmt_levl, 5);
	  sqlite3_bind_null (stmt_levl, 6);
	  sqlite3_bind_null (stmt_levl, 7);
	  sqlite3_bind_null (stmt_levl, 8);
	  sqlite3_bind_null (stmt_levl, 9);
      }
    else
      {
	  sqlite3_bind_double (stmt_levl, 4, res_x * 2.0);
	  sqlite3_bind_double (stmt_levl, 5, res_y * 2.0);
	  sqlite3_bind_double (stmt_levl, 6, res_x * 4.0);
	  sqlite3_bind_double (stmt_levl, 7, res_y * 4.0);
	  sqlite3_bind_double (stmt_levl, 8, res_x * 8.0);
	  sqlite3_bind_double (stmt_levl, 9, res_y * 8.0);
      }
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

RL2_PRIVATE int
rl2_do_insert_stats (sqlite3 * handle, rl2RasterStatisticsPtr section_stats,
		     sqlite3_int64 section_id, sqlite3_stmt * stmt_upd_sect)
{
/* updating the Section's Statistics */
    unsigned char *blob_stats;
    int blob_stats_sz;
    int ret;

    sqlite3_reset (stmt_upd_sect);
    sqlite3_clear_bindings (stmt_upd_sect);
    rl2_serialize_dbms_raster_statistics (section_stats, &blob_stats,
					  &blob_stats_sz);
    sqlite3_bind_blob (stmt_upd_sect, 1, blob_stats, blob_stats_sz, free);
    sqlite3_bind_int64 (stmt_upd_sect, 2, section_id);
    ret = sqlite3_step (stmt_upd_sect);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "UPDATE sections; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    return 1;
  error:
    return 0;
}

RL2_PRIVATE char *
get_section_name (const char *src_path)
{
/* attempting to extract the section name from source path */
    int pos1 = 0;
    int pos2 = 0;
    int xpos2;
    int len;
    char *name;
    const char *p;
    if (src_path == NULL)
	return NULL;
    pos2 = strlen (src_path) - 1;
    xpos2 = pos2;
    pos1 = 0;
    p = src_path + pos2;
    while (p >= src_path)
      {
	  if (*p == '.' && pos2 == xpos2)
	      pos2 = (p - 1) - src_path;
	  if (*p == '/')
	    {
		pos1 = (p + 1) - src_path;
		break;
	    }
	  p--;
      }
    len = pos2 - pos1 + 1;
    name = malloc (len + 1);
    memset (name, '\0', len + 1);
    memcpy (name, src_path + pos1, len);
    return name;
}

RL2_DECLARE char *
rl2_compute_file_md5_checksum (const char *src_path)
{
/* attempting to compute an MD5 checksum from a file */
    size_t rd;
    size_t blk = 1024 * 1024;
    unsigned char *buf;
    void *p_md5;
    char *md5;
    FILE *in = fopen (src_path, "rb");
    if (in == NULL)
	return NULL;
    buf = malloc (blk);
    p_md5 = rl2_CreateMD5Checksum ();
    while (1)
      {
	  rd = fread (buf, 1, blk, in);
	  if (rd == 0)
	      break;
	  rl2_UpdateMD5Checksum (p_md5, buf, rd);
      }
    free (buf);
    fclose (in);
    md5 = rl2_FinalizeMD5Checksum (p_md5);
    rl2_FreeMD5Checksum (p_md5);
    return md5;
}

RL2_PRIVATE int
rl2_do_insert_section (sqlite3 * handle, const char *src_path,
		       const char *section, int srid, unsigned int width,
		       unsigned int height, double minx, double miny,
		       double maxx, double maxy, char *xml_summary,
		       int section_paths, int section_md5,
		       int section_summary, sqlite3_stmt * stmt_sect,
		       sqlite3_int64 * id)
{
/* INSERTing the section */
    int ret;
    unsigned char *blob;
    int blob_size;
    sqlite3_int64 section_id;

    sqlite3_reset (stmt_sect);
    sqlite3_clear_bindings (stmt_sect);
    if (section != NULL)
	sqlite3_bind_text (stmt_sect, 1, section, strlen (section),
			   SQLITE_STATIC);
    else
      {
	  char *sect_name = get_section_name (src_path);
	  if (sect_name != NULL)
	      sqlite3_bind_text (stmt_sect, 1, sect_name, strlen (sect_name),
				 free);
      }
    if (section_paths)
	sqlite3_bind_text (stmt_sect, 2, src_path, strlen (src_path),
			   SQLITE_STATIC);
    else
	sqlite3_bind_null (stmt_sect, 2);
    if (section_md5)
      {
	  char *md5 = rl2_compute_file_md5_checksum (src_path);
	  if (md5 == NULL)
	      sqlite3_bind_null (stmt_sect, 3);
	  else
	      sqlite3_bind_text (stmt_sect, 3, md5, strlen (md5), free);
      }
    else
	sqlite3_bind_null (stmt_sect, 3);
    if (section_summary)
      {
	  if (xml_summary == NULL)
	      sqlite3_bind_null (stmt_sect, 4);
	  else
	      sqlite3_bind_blob (stmt_sect, 4, xml_summary,
				 strlen (xml_summary), free);
      }
    else
      {
	  sqlite3_bind_null (stmt_sect, 4);
	  if (xml_summary != NULL)
	      free (xml_summary);
      }
    sqlite3_bind_int (stmt_sect, 5, width);
    sqlite3_bind_int (stmt_sect, 6, height);
    if (rl2_build_bbox
	(handle, srid, minx, miny, maxx, maxy, &blob, &blob_size) != RL2_OK)
	goto error;
    sqlite3_bind_blob (stmt_sect, 7, blob, blob_size, free);
    ret = sqlite3_step (stmt_sect);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	section_id = sqlite3_last_insert_rowid (handle);
    else
      {
	  fprintf (stderr,
		   "INSERT INTO sections; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    *id = section_id;
    return 1;
  error:
    return 0;
}

RL2_PRIVATE rl2RasterPtr
build_wms_tile (rl2CoveragePtr coverage, const unsigned char *rgba_tile)
{
/* building a raster starting from an RGBA buffer */
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    const unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    int requires_mask = 0;
    unsigned int x;
    unsigned int y;

    if (coverage == NULL || rgba_tile == NULL)
	return NULL;

    /* supporting only RGB, GRAYSCALE or MONOCHROME coverages */
    if (cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	pixels_sz = cvg->tileWidth * cvg->tileHeight * 3;
    if (cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	pixels_sz = cvg->tileWidth * cvg->tileHeight;
    if (cvg->pixelType == RL2_PIXEL_MONOCHROME && cvg->nBands == 1)
	pixels_sz = cvg->tileWidth * cvg->tileHeight;
    if (pixels_sz <= 0)
	return NULL;
    mask_size = cvg->tileWidth * cvg->tileHeight;

    /* allocating the raster and mask buffers */
    pixels = malloc (pixels_sz);
    if (pixels == NULL)
	return NULL;
    mask = malloc (mask_size);
    if (mask == NULL)
      {
	  free (pixels);
	  return NULL;
      }

    p_msk = mask;
    for (x = 0; x < (unsigned int) mask_size; x++)
      {
	  /* priming a full opaque mask */
	  *p_msk++ = 1;
      }

    /* copying pixels */
    p_in = rgba_tile;
    p_out = pixels;
    p_msk = mask;
    if (cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
      {
	  for (y = 0; y < cvg->tileHeight; y++)
	    {
		for (x = 0; x < cvg->tileWidth; x++)
		  {
		      unsigned char red = *p_in++;
		      unsigned char green = *p_in++;
		      unsigned char blue = *p_in++;
		      p_in++;
		      *p_out++ = red;
		      *p_out++ = green;
		      *p_out++ = blue;
		  }
	    }
      }
    if (cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
      {
	  for (y = 0; y < cvg->tileHeight; y++)
	    {
		for (x = 0; x < cvg->tileWidth; x++)
		  {
		      unsigned char red = *p_in++;
		      p_in += 3;
		      *p_out++ = red;
		  }
	    }
      }
    if (cvg->pixelType == RL2_PIXEL_MONOCHROME && cvg->nBands == 1)
      {
	  for (y = 0; y < cvg->tileHeight; y++)
	    {
		for (x = 0; x < cvg->tileWidth; x++)
		  {
		      unsigned char red = *p_in++;
		      p_in += 3;
		      if (red == 255)
			  *p_out++ = 0;
		      else
			  *p_out++ = 1;
		  }
	    }
      }

    if (!requires_mask)
      {
	  /* no mask required */
	  free (mask);
	  mask = NULL;
	  mask_size = 0;
      }

    raster =
	rl2_create_raster (cvg->tileWidth, cvg->tileHeight,
			   cvg->sampleType, cvg->pixelType,
			   cvg->nBands, pixels, pixels_sz, NULL, mask,
			   mask_size, NULL);
    if (raster == NULL)
	goto error;
    return raster;

  error:
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (pixels != NULL)
	free (pixels);
    if (mask != NULL)
	free (mask);
    return NULL;
}

RL2_PRIVATE int
insert_wms_tile (InsertWmsPtr ptr, int *first,
		 rl2RasterStatisticsPtr * section_stats,
		 sqlite3_int64 * section_id)
{
    double tile_minx;
    double tile_miny;
    double tile_maxx;
    double tile_maxy;
    unsigned char *blob_odd;
    int blob_odd_sz;
    unsigned char *blob_even;
    int blob_even_sz;
    rl2RasterPtr raster = NULL;
    double base_res_x;
    double base_res_y;

    if (rl2_get_coverage_resolution (ptr->coverage, &base_res_x, &base_res_y)
	!= RL2_OK)
	goto error;
    if (*first)
      {
	  /* INSERTing the section */
	  *first = 0;
	  if (!rl2_do_insert_section
	      (ptr->sqlite, "WMS Service", ptr->sect_name, ptr->srid,
	       ptr->width, ptr->height, ptr->minx, ptr->miny, ptr->maxx,
	       ptr->maxy, ptr->xml_summary, ptr->sectionPaths,
	       ptr->sectionMD5, ptr->sectionSummary, ptr->stmt_sect,
	       section_id))
	      goto error;
	  *section_stats =
	      rl2_create_raster_statistics (ptr->sample_type, ptr->num_bands);
	  if (*section_stats == NULL)
	      goto error;
	  /* INSERTing the base-levels */
	  if (ptr->mixedResolutions)
	    {
		/* multiple resolutions Coverage */
		if (!rl2_do_insert_section_levels
		    (ptr->sqlite, *section_id, base_res_x, base_res_y, 1.0,
		     RL2_SAMPLE_UNKNOWN, ptr->stmt_levl))
		    goto error;
	    }
	  else
	    {
		/* single resolution Coverage */
		if (!rl2_do_insert_levels
		    (ptr->sqlite, base_res_x, base_res_y, 1.0,
		     RL2_SAMPLE_UNKNOWN, ptr->stmt_levl))
		    goto error;
	    }
      }

    /* building the raster tile */
    raster = build_wms_tile (ptr->coverage, ptr->rgba_tile);
    if (raster == NULL)
      {
	  fprintf (stderr, "ERROR: unable to get a WMS tile\n");
	  goto error;
      }
    if (rl2_raster_encode
	(raster, ptr->compression, &blob_odd, &blob_odd_sz, &blob_even,
	 &blob_even_sz, 100, 1) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to encode a WMS tile\n");
	  goto error;
      }

    /* INSERTing the tile */
    tile_minx = ptr->x;
    tile_maxx = tile_minx + ptr->tilew;
    tile_maxy = ptr->y;
    tile_miny = tile_maxy - ptr->tileh;
    if (!do_insert_wms_tile
	(ptr->sqlite, blob_odd, blob_odd_sz, blob_even, blob_even_sz,
	 *section_id, ptr->srid, ptr->horz_res, ptr->vert_res,
	 ptr->tile_width, ptr->tile_height, ptr->miny, ptr->maxx, tile_minx,
	 tile_miny, tile_maxx, tile_maxy, NULL, ptr->no_data, ptr->stmt_tils,
	 ptr->stmt_data, *section_stats))
	goto error;
    blob_odd = NULL;
    blob_even = NULL;
    rl2_destroy_raster (raster);
    free (ptr->rgba_tile);
    ptr->rgba_tile = NULL;
    return 1;
  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (blob_odd != NULL)
	free (blob_odd);
    if (blob_even != NULL)
	free (blob_even);
    free (ptr->rgba_tile);
    ptr->rgba_tile = NULL;
    return 0;
}

RL2_PRIVATE ResolutionsListPtr
alloc_resolutions_list ()
{
/* allocating an empty list */
    ResolutionsListPtr list = malloc (sizeof (ResolutionsList));
    if (list == NULL)
	return NULL;
    list->first = NULL;
    list->last = NULL;
    return list;
}

RL2_PRIVATE void
destroy_resolutions_list (ResolutionsListPtr list)
{
/* memory cleanup - destroying a list */
    ResolutionLevelPtr res;
    ResolutionLevelPtr resn;
    if (list == NULL)
	return;
    res = list->first;
    while (res != NULL)
      {
	  resn = res->next;
	  free (res);
	  res = resn;
      }
    free (list);
}

RL2_PRIVATE void
add_base_resolution (ResolutionsListPtr list, int level, int scale,
		     double x_res, double y_res)
{
/* inserting a base resolution into the list */
    ResolutionLevelPtr res;
    if (list == NULL)
	return;

    res = list->first;
    while (res != NULL)
      {
	  if (res->x_resolution == x_res && res->y_resolution == y_res)
	    {
		/* already defined: skipping */
		return;
	    }
	  res = res->next;
      }

/* inserting */
    res = malloc (sizeof (ResolutionLevel));
    res->level = level;
    res->scale = scale;
    res->x_resolution = x_res;
    res->y_resolution = y_res;
    res->prev = list->last;
    res->next = NULL;
    if (list->first == NULL)
	list->first = res;
    if (list->last != NULL)
	list->last->next = res;
    list->last = res;
}

RL2_PRIVATE int
rl2_find_best_resolution_level (sqlite3 * handle, const char *db_prefix,
				const char *coverage, int by_section,
				sqlite3_int64 section_id, double x_res,
				double y_res, int *level_id, int *scale,
				int *real_scale, double *xx_res, double *yy_res)
{
/* attempting to identify the optimal resolution level */
    int ret;
    int found = 0;
    int z_level;
    int z_scale;
    int z_real;
    double z_x_res;
    double z_y_res;
    char *xcoverage;
    char *xxcoverage;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    ResolutionsListPtr list = NULL;
    ResolutionLevelPtr res;
    char *xdb_prefix;

    if (coverage == NULL)
	return 0;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);

    if (by_section)
      {
	  /* Multi Resolution Coverage */
	  char sctn[1024];
#if defined(_WIN32) && !defined(__MINGW32__)
	  sprintf (sctn, "%I64d", section_id);
#else
	  sprintf (sctn, "%lld", section_id);
#endif
	  xcoverage = sqlite3_mprintf ("%s_section_levels", coverage);
	  xxcoverage = rl2_double_quoted_sql (xcoverage);
	  sqlite3_free (xcoverage);
	  sql =
	      sqlite3_mprintf
	      ("SELECT pyramid_level, x_resolution_1_8, y_resolution_1_8, "
	       "x_resolution_1_4, y_resolution_1_4, x_resolution_1_2, y_resolution_1_2, "
	       "x_resolution_1_1, y_resolution_1_1 FROM \"%s\".\"%s\" "
	       "WHERE section_id = %s ORDER BY pyramid_level DESC",
	       xdb_prefix, xxcoverage, sctn);
      }
    else
      {
	  /* ordinary Coverage */
	  xcoverage = sqlite3_mprintf ("%s_levels", coverage);
	  xxcoverage = rl2_double_quoted_sql (xcoverage);
	  sqlite3_free (xcoverage);
	  sql =
	      sqlite3_mprintf
	      ("SELECT pyramid_level, x_resolution_1_8, y_resolution_1_8, "
	       "x_resolution_1_4, y_resolution_1_4, x_resolution_1_2, y_resolution_1_2, "
	       "x_resolution_1_1, y_resolution_1_1 FROM \"%s\".\"%s\" "
	       "ORDER BY pyramid_level DESC", xdb_prefix, xxcoverage);
      }
    free (xdb_prefix);
    free (xxcoverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_free (sql);

    list = alloc_resolutions_list ();
    if (list == NULL)
	goto error;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int lvl = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		  {
		      z_x_res = sqlite3_column_double (stmt, 1);
		      z_y_res = sqlite3_column_double (stmt, 2);
		      add_base_resolution (list, lvl, RL2_SCALE_8, z_x_res,
					   z_y_res);
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 4) == SQLITE_FLOAT)
		  {
		      z_x_res = sqlite3_column_double (stmt, 3);
		      z_y_res = sqlite3_column_double (stmt, 4);
		      add_base_resolution (list, lvl, RL2_SCALE_4, z_x_res,
					   z_y_res);
		  }
		if (sqlite3_column_type (stmt, 5) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 6) == SQLITE_FLOAT)
		  {
		      z_x_res = sqlite3_column_double (stmt, 5);
		      z_y_res = sqlite3_column_double (stmt, 6);
		      add_base_resolution (list, lvl, RL2_SCALE_2, z_x_res,
					   z_y_res);
		  }
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      z_x_res = sqlite3_column_double (stmt, 7);
		      z_y_res = sqlite3_column_double (stmt, 8);
		      add_base_resolution (list, lvl, RL2_SCALE_1, z_x_res,
					   z_y_res);
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

/* adjusting real scale factors */
    z_real = 1;
    res = list->last;
    while (res != NULL)
      {
	  res->real_scale = z_real;
	  z_real *= 2;
	  res = res->prev;
      }
/* retrieving the best resolution level */
    found = 0;
    res = list->last;
    while (res != NULL)
      {
	  if (res->x_resolution <= x_res && res->y_resolution <= y_res)
	    {
		found = 1;
		z_level = res->level;
		z_scale = res->scale;
		z_real = res->real_scale;
		z_x_res = res->x_resolution;
		z_y_res = res->y_resolution;
	    }
	  res = res->prev;
      }
    if (found)
      {
	  *level_id = z_level;
	  *scale = z_scale;
	  *real_scale = z_real;
	  *xx_res = z_x_res;
	  *yy_res = z_y_res;
      }
    else if (list->last != NULL)
      {
	  res = list->last;
	  *level_id = res->level;
	  *scale = res->scale;
	  *xx_res = res->x_resolution;
	  *yy_res = res->y_resolution;
      }
    else
	goto error;
    destroy_resolutions_list (list);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (list != NULL)
	destroy_resolutions_list (list);
    return 0;
}

RL2_PRIVATE unsigned char
get_palette_format (rl2PrivPalettePtr plt)
{
/* testing for a Grayscale or RGB palette */
    int is_gray = 0;
    int i;
    for (i = 0; i < plt->nEntries; i++)
      {
	  rl2PrivPaletteEntryPtr entry = plt->entries + i;
	  if (entry->red == entry->green && entry->red == entry->blue)
	      is_gray++;
      }
    if (is_gray == plt->nEntries)
	return RL2_PIXEL_GRAYSCALE;
    else
	return RL2_PIXEL_RGB;
}

static unsigned char *
gray_to_rgba (unsigned short width, unsigned short height, unsigned char *gray)
{
/* transforming an RGB buffer to RGBA */
    unsigned char *rgba = NULL;
    unsigned char *p_out;
    const unsigned char *p_in;
    int x;
    int y;

    rgba = malloc (width * height * 4);
    if (rgba == NULL)
	return NULL;
    p_in = gray;
    p_out = rgba;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char x = *p_in++;
		*p_out++ = x;	/* red */
		*p_out++ = x;	/* green */
		*p_out++ = x;	/* blue */
		*p_out++ = 255;	/* alpha */
	    }
      }
    return rgba;
}

static unsigned char *
rgb_to_rgba (unsigned int width, unsigned int height, unsigned char *rgb)
{
/* transforming an RGB buffer to RGBA */
    unsigned char *rgba = NULL;
    unsigned char *p_out;
    const unsigned char *p_in;
    unsigned int x;
    unsigned int y;

    rgba = malloc (width * height * 4);
    if (rgba == NULL)
	return NULL;
    p_in = rgb;
    p_out = rgba;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		*p_out++ = *p_in++;	/* red */
		*p_out++ = *p_in++;	/* green */
		*p_out++ = *p_in++;	/* blue */
		*p_out++ = 255;	/* alpha */
	    }
      }
    return rgba;
}

RL2_PRIVATE int
get_payload_from_monochrome_opaque (unsigned int width, unsigned int height,
				    sqlite3 * handle, double minx,
				    double miny, double maxx, double maxy,
				    int srid, unsigned char *pixels,
				    unsigned char format, int quality,
				    unsigned char **image, int *image_sz)
{
/* input: Monochrome    output: Grayscale */
    int ret;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *gray = NULL;
    unsigned int row;
    unsigned int col;
    unsigned char *rgba = NULL;

    gray = malloc (width * height);
    if (gray == NULL)
	goto error;
    p_in = pixels;
    p_out = gray;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_in++ == 1)
		    *p_out++ = 0;	/* Black */
		else
		    *p_out++ = 255;	/* White */
	    }
      }
    free (pixels);
    pixels = NULL;
    if (format == RL2_OUTPUT_FORMAT_JPEG)
      {
	  if (rl2_gray_to_jpeg (width, height, gray, quality, image, image_sz)
	      != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_gray_to_png (width, height, gray, image, image_sz) != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_TIFF)
      {
	  if (srid > 0)
	    {
		if (rl2_gray_to_geotiff
		    (width, height, handle, minx, miny, maxx, maxy, srid,
		     gray, image, image_sz) != RL2_OK)
		    goto error;
	    }
	  else
	    {
		if (rl2_gray_to_tiff (width, height, gray, image, image_sz) !=
		    RL2_OK)
		    goto error;
	    }
      }
    else if (format == RL2_OUTPUT_FORMAT_PDF)
      {
	  rgba = gray_to_rgba (width, height, gray);
	  if (rgba == NULL)
	      goto error;
	  ret = rl2_rgba_to_pdf (width, height, rgba, image, image_sz);
	  rgba = NULL;
	  if (ret != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (gray);
    return 1;

  error:
    if (pixels != NULL)
	free (pixels);
    if (gray != NULL)
	free (gray);
    if (rgba != NULL)
	free (rgba);
    return 0;
}

RL2_PRIVATE int
get_payload_from_monochrome_transparent (unsigned int width,
					 unsigned int height,
					 unsigned char *pixels,
					 unsigned char format, int quality,
					 unsigned char **image, int *image_sz,
					 double opacity)
{
/* input: Monochrome    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned char *gray = NULL;
    unsigned char *mask = NULL;
    unsigned int row;
    unsigned int col;

    if (quality > 100)
	quality = 100;
    gray = malloc (width * height);
    if (gray == NULL)
	goto error;
    mask = malloc (width * height);
    if (mask == NULL)
	goto error;
    p_in = pixels;
    p_out = gray;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_in++ == 1)
		  {
		      *p_out++ = 0;	/* Black */
		      *p_msk++ = 1;	/* Opaque */
		  }
		else
		  {
		      *p_out++ = 1;	/* White */
		      *p_msk++ = 0;	/* Transparent */
		  }
	    }
      }
    free (pixels);
    pixels = NULL;
    if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_gray_alpha_to_png
	      (width, height, gray, mask, image, image_sz, opacity) != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (gray);
    free (mask);
    return 1;

  error:
    if (pixels != NULL)
	free (pixels);
    if (gray != NULL)
	free (gray);
    if (mask != NULL)
	free (mask);
    return 0;
}

RL2_PRIVATE int
get_payload_from_palette_opaque (unsigned int width, unsigned int height,
				 sqlite3 * handle, double minx, double miny,
				 double maxx, double maxy, int srid,
				 unsigned char *pixels, rl2PalettePtr palette,
				 unsigned char format, int quality,
				 unsigned char **image, int *image_sz)
{
/* input: Palette    output: Grayscale or RGB */
    int ret;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *gray = NULL;
    unsigned char *rgb = NULL;
    unsigned int row;
    unsigned int col;
    unsigned char out_format;
    unsigned char *rgba = NULL;

    out_format = get_palette_format (plt);
    if (out_format == RL2_PIXEL_RGB)
      {
	  /* converting from Palette to RGB */
	  rgb = malloc (width * height * 3);
	  p_in = pixels;
	  p_out = rgb;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char red = 0;
		      unsigned char green = 0;
		      unsigned char blue = 0;
		      unsigned char index = *p_in++;
		      if (index < plt->nEntries)
			{
			    rl2PrivPaletteEntryPtr entry = plt->entries + index;
			    red = entry->red;
			    green = entry->green;
			    blue = entry->blue;
			}
		      *p_out++ = red;	/* red */
		      *p_out++ = green;	/* green */
		      *p_out++ = blue;	/* blue */
		  }
	    }
	  free (pixels);
	  pixels = NULL;
	  if (format == RL2_OUTPUT_FORMAT_JPEG)
	    {
		if (rl2_rgb_to_jpeg
		    (width, height, rgb, quality, image, image_sz) != RL2_OK)
		    goto error;
	    }
	  else if (format == RL2_OUTPUT_FORMAT_PNG)
	    {
		if (rl2_rgb_to_png (width, height, rgb, image, image_sz) !=
		    RL2_OK)
		    goto error;
	    }
	  else if (format == RL2_OUTPUT_FORMAT_TIFF)
	    {
		if (srid > 0)
		  {
		      if (rl2_rgb_to_geotiff
			  (width, height, handle, minx, miny, maxx, maxy,
			   srid, rgb, image, image_sz) != RL2_OK)
			  goto error;
		  }
		else
		  {
		      if (rl2_rgb_to_tiff
			  (width, height, rgb, image, image_sz) != RL2_OK)
			  goto error;
		  }
	    }
	  else if (format == RL2_OUTPUT_FORMAT_PDF)
	    {
		rgba = rgb_to_rgba (width, height, rgb);
		if (rgba == NULL)
		    goto error;
		ret = rl2_rgba_to_pdf (width, height, rgba, image, image_sz);
		rgba = NULL;
		if (ret != RL2_OK)
		    goto error;
	    }
	  else
	      goto error;
	  free (rgb);
      }
    else if (out_format == RL2_PIXEL_GRAYSCALE)
      {
	  /* converting from Palette to Grayscale */
	  gray = malloc (width * height);
	  p_in = pixels;
	  p_out = gray;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char value = 0;
		      unsigned char index = *p_in++;
		      if (index < plt->nEntries)
			{
			    rl2PrivPaletteEntryPtr entry = plt->entries + index;
			    value = entry->red;
			}
		      *p_out++ = value;	/* gray */
		  }
	    }
	  free (pixels);
	  if (format == RL2_OUTPUT_FORMAT_JPEG)
	    {
		if (rl2_gray_to_jpeg
		    (width, height, gray, quality, image, image_sz) != RL2_OK)
		    goto error;
	    }
	  else if (format == RL2_OUTPUT_FORMAT_PNG)
	    {
		if (rl2_gray_to_png (width, height, gray, image, image_sz) !=
		    RL2_OK)
		    goto error;
	    }
	  else if (format == RL2_OUTPUT_FORMAT_TIFF)
	    {
		if (srid > 0)
		  {
		      if (rl2_gray_to_geotiff
			  (width, height, handle, minx, miny, maxx, maxy,
			   srid, gray, image, image_sz) != RL2_OK)
			  goto error;
		  }
		else
		  {
		      if (rl2_gray_to_tiff
			  (width, height, gray, image, image_sz) != RL2_OK)
			  goto error;
		  }
	    }
	  else if (format == RL2_OUTPUT_FORMAT_PDF)
	    {
		rgba = gray_to_rgba (width, height, gray);
		if (rgba == NULL)
		    goto error;
		ret = rl2_rgba_to_pdf (width, height, rgba, image, image_sz);
		rgba = NULL;
		if (ret != RL2_OK)
		    goto error;
	    }
	  else
	      goto error;
	  free (gray);
      }
    else
	goto error;
    return 1;

  error:
    if (pixels != NULL)
	free (pixels);
    if (gray != NULL)
	free (gray);
    if (rgb != NULL)
	free (rgb);
    if (rgba != NULL)
	free (rgba);
    return 0;
}

RL2_PRIVATE int
get_payload_from_palette_transparent (unsigned int width,
				      unsigned int height,
				      unsigned char *pixels,
				      rl2PalettePtr palette,
				      unsigned char format, int quality,
				      unsigned char **image, int *image_sz,
				      unsigned char bg_red,
				      unsigned char bg_green,
				      unsigned char bg_blue, double opacity)
{
/* input: Palette    output: Grayscale or RGB */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned char *gray = NULL;
    unsigned char *rgb = NULL;
    unsigned char *mask = NULL;
    unsigned int row;
    unsigned int col;
    unsigned char out_format;

    if (quality > 100)
	quality = 100;
    out_format = get_palette_format (plt);
    if (out_format == RL2_PIXEL_RGB)
      {
	  /* converting from Palette to RGB */
	  rgb = malloc (width * height * 3);
	  if (rgb == NULL)
	      goto error;
	  mask = malloc (width * height);
	  if (mask == NULL)
	      goto error;
	  p_in = pixels;
	  p_out = rgb;
	  p_msk = mask;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char red = 0;
		      unsigned char green = 0;
		      unsigned char blue = 0;
		      unsigned char index = *p_in++;
		      if (index < plt->nEntries)
			{
			    rl2PrivPaletteEntryPtr entry = plt->entries + index;
			    red = entry->red;
			    green = entry->green;
			    blue = entry->blue;
			}
		      *p_out++ = red;	/* red */
		      *p_out++ = green;	/* green */
		      *p_out++ = blue;	/* blue */
		      if (red == bg_red && green == bg_green && blue == bg_blue)
			  *p_msk++ = 0;	/* Transparent */
		      else
			  *p_msk++ = 1;	/* Opaque */
		  }
	    }
	  free (pixels);
	  if (format == RL2_OUTPUT_FORMAT_PNG)
	    {
		if (rl2_rgb_to_png (width, height, rgb, image, image_sz) !=
		    RL2_OK)
		    goto error;
	    }
	  else
	      goto error;
	  free (rgb);
	  free (mask);
      }
    else if (out_format == RL2_PIXEL_GRAYSCALE)
      {
	  /* converting from Palette to Grayscale */
	  gray = malloc (width * height);
	  if (gray == NULL)
	      goto error;
	  mask = malloc (width * height);
	  if (mask == NULL)
	      goto error;
	  p_in = pixels;
	  p_out = gray;
	  p_msk = mask;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char value = 0;
		      unsigned char index = *p_in++;
		      if (index < plt->nEntries)
			{
			    rl2PrivPaletteEntryPtr entry = plt->entries + index;
			    value = entry->red;
			}
		      *p_out++ = value;	/* gray */
		      if (value == bg_red)
			  *p_msk++ = 0;	/* Transparent */
		      else
			  *p_msk++ = 1;	/* Opaque */
		  }
	    }
	  free (pixels);
	  if (format == RL2_OUTPUT_FORMAT_PNG)
	    {
		if (rl2_gray_alpha_to_png
		    (width, height, gray, mask, image, image_sz,
		     opacity) != RL2_OK)
		    goto error;
	    }
	  else
	      goto error;
	  free (gray);
	  free (mask);
      }
    else
	goto error;
    return 1;

  error:
    if (pixels != NULL)
	free (pixels);
    if (gray != NULL)
	free (gray);
    if (rgb != NULL)
	free (rgb);
    if (mask != NULL)
	free (mask);
    return 0;
}

RL2_PRIVATE int
get_payload_from_grayscale_opaque (unsigned int width, unsigned int height,
				   sqlite3 * handle, double minx, double miny,
				   double maxx, double maxy, int srid,
				   unsigned char *pixels,
				   unsigned char format, int quality,
				   unsigned char **image, int *image_sz)
{
/* input: Grayscale    output: Grayscale */
    int ret;
    unsigned char *rgba = NULL;

    if (format == RL2_OUTPUT_FORMAT_JPEG)
      {
	  if (rl2_gray_to_jpeg
	      (width, height, pixels, quality, image, image_sz) != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_gray_to_png (width, height, pixels, image, image_sz) !=
	      RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_TIFF)
      {
	  if (srid > 0)
	    {
		if (rl2_gray_to_geotiff
		    (width, height, handle, minx, miny, maxx, maxy, srid,
		     pixels, image, image_sz) != RL2_OK)
		    goto error;
	    }
	  else
	    {
		if (rl2_gray_to_tiff (width, height, pixels, image, image_sz)
		    != RL2_OK)
		    goto error;
	    }
      }
    else if (format == RL2_OUTPUT_FORMAT_PDF)
      {
	  rgba = gray_to_rgba (width, height, pixels);
	  if (rgba == NULL)
	      goto error;
	  ret = rl2_rgba_to_pdf (width, height, rgba, image, image_sz);
	  rgba = NULL;
	  if (ret != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (pixels);
    if (rgba != NULL)
	free (rgba);
    return 1;

  error:
    free (pixels);
    return 0;
}

RL2_PRIVATE int
get_payload_from_grayscale_transparent (unsigned int width,
					unsigned int height,
					unsigned char *pixels,
					unsigned char format, int quality,
					unsigned char **image, int *image_sz,
					unsigned char bg_gray, double opacity)
{
/* input: Grayscale    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_msk;
    unsigned char *mask = NULL;
    unsigned short row;
    unsigned short col;

    if (quality > 100)
	quality = 100;
    mask = malloc (width * height);
    if (mask == NULL)
	goto error;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_in++ == bg_gray)
		    *p_msk++ = 0;	/* Transparent */
		else
		    *p_msk++ = 255;	/* Opaque */
	    }
      }
    if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_gray_alpha_to_png
	      (width, height, pixels, mask, image, image_sz, opacity) != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (pixels);
    free (mask);
    return 1;

  error:
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 0;
}

RL2_PRIVATE int
get_payload_from_rgb_opaque (unsigned int width, unsigned int height,
			     sqlite3 * handle, double minx, double miny,
			     double maxx, double maxy, int srid,
			     unsigned char *pixels, unsigned char format,
			     int quality, unsigned char **image, int *image_sz)
{
/* input: RGB    output: RGB */
    int ret;
    unsigned char *rgba = NULL;

    if (format == RL2_OUTPUT_FORMAT_JPEG)
      {
	  if (rl2_rgb_to_jpeg
	      (width, height, pixels, quality, image, image_sz) != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_rgb_to_png (width, height, pixels, image, image_sz) != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_TIFF)
      {
	  if (srid > 0)
	    {
		if (rl2_rgb_to_geotiff
		    (width, height, handle, minx, miny, maxx, maxy, srid,
		     pixels, image, image_sz) != RL2_OK)
		    goto error;
	    }
	  else
	    {
		if (rl2_rgb_to_tiff (width, height, pixels, image, image_sz)
		    != RL2_OK)
		    goto error;
	    }
      }
    else if (format == RL2_OUTPUT_FORMAT_PDF)
      {
	  rgba = rgb_to_rgba (width, height, pixels);
	  if (rgba == NULL)
	      goto error;
	  ret = rl2_rgba_to_pdf (width, height, rgba, image, image_sz);
	  rgba = NULL;
	  if (ret != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (pixels);
    return 1;

  error:
    free (pixels);
    if (rgba != NULL)
	free (rgba);
    return 0;
}

RL2_PRIVATE int
get_payload_from_rgb_transparent (unsigned int width, unsigned int height,
				  unsigned char *pixels, unsigned char format,
				  int quality, unsigned char **image,
				  int *image_sz, unsigned char bg_red,
				  unsigned char bg_green,
				  unsigned char bg_blue, double opacity)
{
/* input: RGB    output: RGB */
    unsigned char *p_in;
    unsigned char *p_msk;
    unsigned char *mask = NULL;
    unsigned int row;
    unsigned int col;

    if (quality > 100)
	quality = 100;
    mask = malloc (width * height);
    if (mask == NULL)
	goto error;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char red = *p_in++;
		unsigned char green = *p_in++;
		unsigned char blue = *p_in++;
		if (red == bg_red && green == bg_green && blue == bg_blue)
		    *p_msk++ = 0;	/* Transparent */
		else
		    *p_msk++ = 1;	/* Opaque */
	    }
      }
    if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_rgb_alpha_to_png
	      (width, height, pixels, mask, image, image_sz, opacity) != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (pixels);
    free (mask);
    return 1;

  error:
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 0;
}

static int
test_no_data_8 (rl2PrivPixelPtr no_data, char *p_in)
{
/* testing for NO-DATA - INT8 */
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none ((rl2PixelPtr) no_data) == RL2_FALSE)
	    {
		unsigned char band;
		int match = 0;
		rl2PrivSamplePtr sample;
		for (band = 0; band < no_data->nBands; band++)
		  {
		      sample = no_data->Samples + band;
		      if (*(p_in + band) == sample->int8)
			  match++;
		  }
		if (match == no_data->nBands)
		    return 1;
	    }
      }
    return 0;
}

static int
test_no_data_u8 (rl2PrivPixelPtr no_data, unsigned char *p_in)
{
/* testing for NO-DATA - UINT8 */
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none ((rl2PixelPtr) no_data) == RL2_FALSE)
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
      }
    return 0;
}

static int
test_no_data_16 (rl2PrivPixelPtr no_data, short *p_in)
{
/* testing for NO-DATA - INT16 */
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none ((rl2PixelPtr) no_data) == RL2_FALSE)
	    {
		unsigned char band;
		int match = 0;
		rl2PrivSamplePtr sample;
		for (band = 0; band < no_data->nBands; band++)
		  {
		      sample = no_data->Samples + band;
		      if (*(p_in + band) == sample->int16)
			  match++;
		  }
		if (match == no_data->nBands)
		    return 1;
	    }
      }
    return 0;
}

static int
test_no_data_u16 (rl2PrivPixelPtr no_data, unsigned short *p_in)
{
/* testing for NO-DATA - UINT16 */
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none ((rl2PixelPtr) no_data) == RL2_FALSE)
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
      }
    return 0;
}

static int
test_no_data_32 (rl2PrivPixelPtr no_data, int *p_in)
{
/* testing for NO-DATA - INT32 */
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none ((rl2PixelPtr) no_data) == RL2_FALSE)
	    {
		unsigned char band;
		int match = 0;
		rl2PrivSamplePtr sample;
		for (band = 0; band < no_data->nBands; band++)
		  {
		      sample = no_data->Samples + band;
		      if (*(p_in + band) == sample->int32)
			  match++;
		  }
		if (match == no_data->nBands)
		    return 1;
	    }
      }
    return 0;
}

static int
test_no_data_u32 (rl2PrivPixelPtr no_data, unsigned int *p_in)
{
/* testing for NO-DATA - UINT32 */
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none ((rl2PixelPtr) no_data) == RL2_FALSE)
	    {
		unsigned char band;
		int match = 0;
		rl2PrivSamplePtr sample;
		for (band = 0; band < no_data->nBands; band++)
		  {
		      sample = no_data->Samples + band;
		      if (*(p_in + band) == sample->uint32)
			  match++;
		  }
		if (match == no_data->nBands)
		    return 1;
	    }
      }
    return 0;
}

static int
test_no_data_flt (rl2PrivPixelPtr no_data, float *p_in)
{
/* testing for NO-DATA - FLOAT */
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none ((rl2PixelPtr) no_data) == RL2_FALSE)
	    {
		unsigned char band;
		int match = 0;
		rl2PrivSamplePtr sample;
		for (band = 0; band < no_data->nBands; band++)
		  {
		      sample = no_data->Samples + band;
		      if (*(p_in + band) == sample->float32)
			  match++;
		  }
		if (match == no_data->nBands)
		    return 1;
	    }
      }
    return 0;
}

static int
test_no_data_dbl (rl2PrivPixelPtr no_data, double *p_in)
{
/* testing for NO-DATA - DOUBLE */
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none ((rl2PixelPtr) no_data) == RL2_FALSE)
	    {
		unsigned char band;
		int match = 0;
		rl2PrivSamplePtr sample;
		for (band = 0; band < no_data->nBands; band++)
		  {
		      sample = no_data->Samples + band;
		      if (*(p_in + band) == sample->float64)
			  match++;
		  }
		if (match == no_data->nBands)
		    return 1;
	    }
      }
    return 0;
}

RL2_PRIVATE int
get_rgba_from_monochrome_mask (unsigned int width, unsigned int height,
			       unsigned char *pixels, unsigned char *mask,
			       unsigned char *rgba)
{
/* input: Monochrome    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (*p_in == 0)
		    transparent = 1;
		p_in++;
		if (transparent)
		    p_out += 4;
		else
		  {
		      *p_out++ = 0;
		      *p_out++ = 0;
		      *p_out++ = 0;
		      *p_out++ = 255;	/* block, opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_monochrome_transparent (unsigned int width,
				      unsigned int height,
				      unsigned char *pixels,
				      unsigned char *rgba)
{
/* input: Monochrome    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;

    p_in = pixels;
    p_out = rgba;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_in++ == 0)
		  {
		      *p_out++ = 255;	/* White */
		      *p_out++ = 255;
		      *p_out++ = 255;
		      *p_out++ = 0;	/* alpha */
		  }
		else
		  {
		      *p_out++ = 0;	/* Black */
		      *p_out++ = 0;
		      *p_out++ = 0;
		      *p_out++ = 255;	/* alpha */
		  }
	    }
      }
    free (pixels);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_monochrome_transparent_mask (unsigned int width,
					   unsigned int height,
					   unsigned char *pixels,
					   unsigned char *mask,
					   unsigned char *rgba)
{
/* input: Monochrome    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_msk;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;

    p_in = pixels;
    p_msk = mask;
    p_out = rgba;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_msk++ == 0)
		  {
		      /* Opaque pixel  */
		      if (*p_in++ == 0)
			{
			    /* White is assumed to be always transparent */
			    p_out += 4;
			}
		      else
			{
			    *p_out++ = 0;	/* Black */
			    *p_out++ = 0;
			    *p_out++ = 0;
			    *p_out++ = 255;	/* alpha */
			}
		  }
		else
		  {
		      /* Transparent Pixel */
		      p_in++;
		      p_out += 4;
		  }
	    }
      }
    free (pixels);
    free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_palette_mask (unsigned int width, unsigned int height,
			    unsigned char *pixels, unsigned char *mask,
			    rl2PalettePtr palette, rl2PrivPixelPtr no_data,
			    unsigned char *rgba)
{
/* input: Palette    output: Grayscale or RGB */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    unsigned char out_format;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    out_format = get_palette_format (plt);
    if (out_format == RL2_PIXEL_RGB)
      {
	  /* converting from Palette to RGB */
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char red = 0;
		      unsigned char green = 0;
		      unsigned char blue = 0;
		      unsigned char index;
		      transparent = 0;
		      if (p_msk != NULL)
			{
			    if (*p_msk++ == 0)
				transparent = 1;
			}
		      if (!transparent)
			  transparent = test_no_data_u8 (no_data, p_in);
		      if (transparent)
			{
			    p_out += 4;
			    p_in++;
			}
		      else
			{
			    index = *p_in++;
			    if (index < plt->nEntries)
			      {
				  rl2PrivPaletteEntryPtr entry =
				      plt->entries + index;
				  red = entry->red;
				  green = entry->green;
				  blue = entry->blue;
			      }
			    *p_out++ = red;	/* red */
			    *p_out++ = green;	/* green */
			    *p_out++ = blue;	/* blue */
			    *p_out++ = 255;	/* opaque */
			}
		  }
	    }
      }
    else if (out_format == RL2_PIXEL_GRAYSCALE)
      {
	  /* converting from Palette to Grayscale */
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char value = 0;
		      unsigned char index = *p_in++;
		      if (index < plt->nEntries)
			{
			    rl2PrivPaletteEntryPtr entry = plt->entries + index;
			    value = entry->red;
			}
		      transparent = 0;
		      if (p_msk != NULL)
			{
			    if (*p_msk++ == 0)
				transparent = 1;
			}
		      if (transparent)
			  p_out += 4;
		      else
			{
			    *p_out++ = value;	/* red */
			    *p_out++ = value;	/* green */
			    *p_out++ = value;	/* blue */
			    *p_out++ = 255;	/* opaque */
			}
		  }
	    }
      }
    else
	goto error;
    free (pixels);
    if (mask)
	free (mask);
    return 1;

  error:
    free (pixels);
    if (mask)
	free (mask);
    return 0;
}

RL2_PRIVATE int
get_rgba_from_palette (unsigned int width, unsigned int height,
		       unsigned char *pixels, unsigned char *mask,
		       rl2PalettePtr palette, unsigned char *rgba)
{
/* input: Palette    output: Grayscale or RGB */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    unsigned char out_format;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    out_format = get_palette_format (plt);
    if (out_format == RL2_PIXEL_RGB)
      {
	  /* converting from Palette to RGB */
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char red = 0;
		      unsigned char green = 0;
		      unsigned char blue = 0;
		      unsigned char index;
		      transparent = 0;
		      if (p_msk != NULL)
			{
			    if (*p_msk++ != 0)
				transparent = 1;
			}
		      if (transparent)
			{
			    p_out += 4;
			    p_in++;
			}
		      else
			{
			    index = *p_in++;
			    if (index < plt->nEntries)
			      {
				  rl2PrivPaletteEntryPtr entry =
				      plt->entries + index;
				  red = entry->red;
				  green = entry->green;
				  blue = entry->blue;
			      }
			    *p_out++ = red;	/* red */
			    *p_out++ = green;	/* green */
			    *p_out++ = blue;	/* blue */
			    *p_out++ = 255;	/* opaque */
			}
		  }
	    }
      }
    else if (out_format == RL2_PIXEL_GRAYSCALE)
      {
	  /* converting from Palette to Grayscale */
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char value = 0;
		      unsigned char index = *p_in++;
		      if (index < plt->nEntries)
			{
			    rl2PrivPaletteEntryPtr entry = plt->entries + index;
			    value = entry->red;
			}
		      transparent = 0;
		      if (p_msk != NULL)
			{
			    if (*p_msk++ != 0)
				transparent = 1;
			}
		      if (transparent)
			  p_out += 4;
		      else
			{
			    *p_out++ = value;	/* red */
			    *p_out++ = value;	/* green */
			    *p_out++ = value;	/* blue */
			    *p_out++ = 255;	/* opaque */
			}
		  }
	    }
      }
    else
	goto error;
    free (pixels);
    if (mask)
	free (mask);
    return 1;

  error:
    free (pixels);
    if (mask)
	free (mask);
    return 0;
}

RL2_PRIVATE int
get_rgba_from_palette_transparent (unsigned int width, unsigned int height,
				   unsigned char *pixels,
				   rl2PalettePtr palette, unsigned char *rgba,
				   unsigned char bg_red,
				   unsigned char bg_green,
				   unsigned char bg_blue)
{
/* input: Palette    output: Grayscale or RGB */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;
    unsigned char out_format;

    p_in = pixels;
    p_out = rgba;
    out_format = get_palette_format (plt);
    if (out_format == RL2_PIXEL_RGB)
      {
	  /* converting from Palette to RGB */
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char red = 0;
		      unsigned char green = 0;
		      unsigned char blue = 0;
		      unsigned char index = *p_in++;
		      if (index < plt->nEntries)
			{
			    rl2PrivPaletteEntryPtr entry = plt->entries + index;
			    red = entry->red;
			    green = entry->green;
			    blue = entry->blue;
			}
		      *p_out++ = red;	/* red */
		      *p_out++ = green;	/* green */
		      *p_out++ = blue;	/* blue */
		      if (red == bg_red && green == bg_green && blue == bg_blue)
			  *p_out++ = 0;	/* Transparent */
		      else
			  *p_out++ = 255;	/* Opaque */
		  }
	    }
      }
    else if (out_format == RL2_PIXEL_GRAYSCALE)
      {
	  /* converting from Palette to Grayscale */
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      unsigned char value = 0;
		      unsigned char index = *p_in++;
		      if (index < plt->nEntries)
			{
			    rl2PrivPaletteEntryPtr entry = plt->entries + index;
			    value = entry->red;
			}
		      *p_out++ = value;	/* red */
		      *p_out++ = value;	/* green */
		      *p_out++ = value;	/* blue */
		      if (value == bg_red)
			  *p_out++ = 0;	/* Transparent */
		      else
			  *p_out++ = 255;	/* Opaque */
		  }
	    }
      }
    else
	goto error;
    free (pixels);
    return 1;

  error:
    free (pixels);
    return 0;
}

RL2_PRIVATE int
get_rgba_from_palette_transparent_mask (unsigned int width, unsigned int height,
					unsigned char *pixels,
					unsigned char *mask,
					rl2PalettePtr palette,
					unsigned char *rgba)
{
/* input: Palette    output: Grayscale or RGB */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;
    unsigned char *p_in;
    unsigned char *p_msk;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;
    unsigned char out_format;

    p_in = pixels;
    p_msk = mask;
    p_out = rgba;
    out_format = get_palette_format (plt);
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_msk++ == 0)
		  {
		      /* opaque pixel */
		      if (out_format == RL2_PIXEL_RGB)
			{
			    /* converting from Palette to RGB */
			    unsigned char red = 0;
			    unsigned char green = 0;
			    unsigned char blue = 0;
			    unsigned char index = *p_in++;
			    if (index < plt->nEntries)
			      {
				  rl2PrivPaletteEntryPtr entry =
				      plt->entries + index;
				  red = entry->red;
				  green = entry->green;
				  blue = entry->blue;
			      }
			    *p_out++ = red;	/* red */
			    *p_out++ = green;	/* green */
			    *p_out++ = blue;	/* blue */
			    *p_out++ = 255;	/* Opaque */
			}
		      else if (out_format == RL2_PIXEL_GRAYSCALE)
			{
			    /* converting from Palette to Grayscale */
			    unsigned char value = 0;
			    unsigned char index = *p_in++;
			    if (index < plt->nEntries)
			      {
				  rl2PrivPaletteEntryPtr entry =
				      plt->entries + index;
				  value = entry->red;
			      }
			    *p_out++ = value;	/* red */
			    *p_out++ = value;	/* green */
			    *p_out++ = value;	/* blue */
			    *p_out++ = 255;	/* Opaque */
			}
		  }
		else
		  {
		      /* transparent pixel */
		      p_in++;
		      p_out += 4;
		  }
	    }
      }
    free (pixels);
    free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_grayscale_mask (unsigned int width, unsigned int height,
			      unsigned char *pixels, unsigned char *mask,
			      rl2PrivPixelPtr no_data, unsigned char *rgba)
{
/* input: Grayscale    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_u8 (no_data, p_in);
		if (transparent)
		  {
		      p_out += 4;
		      p_in++;
		  }
		else
		  {
		      unsigned char gray = *p_in++;
		      *p_out++ = gray;	/* red */
		      *p_out++ = gray;	/* green */
		      *p_out++ = gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_grayscale (unsigned int width, unsigned int height,
			 unsigned char *pixels, unsigned char *mask,
			 unsigned char *rgba)
{
/* input: Grayscale    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ != 0)
			  transparent = 1;
		  }
		if (transparent)
		  {
		      p_out += 4;
		      p_in++;
		  }
		else
		  {
		      unsigned char gray = *p_in++;
		      *p_out++ = gray;	/* red */
		      *p_out++ = gray;	/* green */
		      *p_out++ = gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_grayscale_transparent (unsigned int width,
				     unsigned int height,
				     unsigned char *pixels,
				     unsigned char *rgba, unsigned char bg_gray)
{
/* input: Grayscale    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;

    p_in = pixels;
    p_out = rgba;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char gray = *p_in++;
		*p_out++ = gray;	/* red */
		*p_out++ = gray;	/* green */
		*p_out++ = gray;	/* blue */
		if (gray == bg_gray)
		    *p_out++ = 0;	/* Transparent */
		else
		    *p_out++ = 255;	/* Opaque */
	    }
      }
    free (pixels);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_grayscale_transparent_mask (unsigned int width,
					  unsigned int height,
					  unsigned char *pixels,
					  unsigned char *mask,
					  unsigned char *rgba)
{
/* input: Grayscale    output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_msk;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;

    p_in = pixels;
    p_msk = mask;
    p_out = rgba;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char gray = *p_in++;
		if (*p_msk++ == 0)
		  {
		      /* Opaque pixel */
		      *p_out++ = gray;	/* red */
		      *p_out++ = gray;	/* green */
		      *p_out++ = gray;	/* blue */
		      *p_out++ = 255;	/* Opaque */
		  }
		else
		    p_out += 4;
	    }
      }
    free (pixels);
    free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_rgb_mask (unsigned int width, unsigned int height,
			unsigned char *pixels, unsigned char *mask,
			rl2PrivPixelPtr no_data, unsigned char *rgba)
{
/* input: RGB    output: RGB */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_u8 (no_data, p_in);
		if (transparent)
		  {
		      p_out += 4;
		      p_in += 3;
		  }
		else
		  {
		      *p_out++ = *p_in++;	/* red */
		      *p_out++ = *p_in++;	/* green */
		      *p_out++ = *p_in++;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_rgb (unsigned int width, unsigned int height,
		   unsigned char *pixels, unsigned char *mask,
		   unsigned char *rgba)
{
/* input: RGB    output: RGB */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ != 0)
			  transparent = 1;
		  }
		if (transparent)
		  {
		      p_out += 4;
		      p_in += 3;
		  }
		else
		  {
		      *p_out++ = *p_in++;	/* red */
		      *p_out++ = *p_in++;	/* green */
		      *p_out++ = *p_in++;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_rgb_transparent (unsigned int width, unsigned int height,
			       unsigned char *pixels, unsigned char *rgba,
			       unsigned char bg_red, unsigned char bg_green,
			       unsigned char bg_blue)
{
/* input: RGB    output: RGB */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;

    p_in = pixels;
    p_out = rgba;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char red = *p_in++;
		unsigned char green = *p_in++;
		unsigned char blue = *p_in++;
		*p_out++ = red;
		*p_out++ = green;
		*p_out++ = blue;
		if (red == bg_red && green == bg_green && blue == bg_blue)
		    *p_out++ = 0;	/* Transparent */
		else
		    *p_out++ = 255;	/* Opaque */
	    }
      }
    free (pixels);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_rgb_transparent_mask (unsigned int width, unsigned int height,
				    unsigned char *pixels, unsigned char *mask,
				    unsigned char *rgba)
{
/* input: RGB    output: RGB */
    unsigned char *p_in;
    unsigned char *p_msk;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;

    p_in = pixels;
    p_msk = mask;
    p_out = rgba;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char red = *p_in++;
		unsigned char green = *p_in++;
		unsigned char blue = *p_in++;
		if (*p_msk++ == 0)
		  {
		      /* opaque pixel */
		      *p_out++ = red;
		      *p_out++ = green;
		      *p_out++ = blue;
		      *p_out++ = 255;
		  }
		else
		    p_out += 4;
	    }
      }
    free (pixels);
    free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_int8 (unsigned int width, unsigned int height,
		char *pixels, unsigned char *mask, rl2PrivPixelPtr no_data,
		unsigned char *rgba)
{
/* input: DataGrid INT8   output: Grayscale */
    char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		char *p_sv = p_in;
		char gray = 128 + *p_in++;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_8 (no_data, p_sv);
		if (transparent)
		    p_out += 4;
		else
		  {
		      *p_out++ = gray;	/* red */
		      *p_out++ = gray;	/* green */
		      *p_out++ = gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_uint8 (unsigned int width, unsigned int height,
		 unsigned char *pixels, unsigned char *mask,
		 rl2PrivPixelPtr no_data, unsigned char *rgba)
{
/* input: DataGrid UINT8   output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char *p_sv = p_in;
		unsigned char gray = *p_in++;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_u8 (no_data, p_sv);
		if (transparent)
		    p_out += 4;
		else
		  {
		      *p_out++ = gray;	/* red */
		      *p_out++ = gray;	/* green */
		      *p_out++ = gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_int16 (unsigned int width, unsigned int height,
		 short *pixels, unsigned char *mask, rl2PrivPixelPtr no_data,
		 unsigned char *rgba)
{
/* input: DataGrid INT16   output: Grayscale */
    short *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    short min = SHRT_MAX;
    short max = SHRT_MIN;
    double min2 = 0.0;
    double max2 = 0.0;
    double tic;
    double tic2;
    int transparent;
    int i;
    int sum;
    int total;
    double percentile2;
    int histogram[1024];

/* identifying Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		short *p_sv = p_in;
		short gray = *p_in++;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_16 (no_data, p_sv))
		    continue;
		if (min > gray)
		    min = gray;
		if (max < gray)
		    max = gray;
		total++;
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building an histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		short *p_sv = p_in;
		double gray = 0.0;
		if (tic > 0.0)
		    gray = (double) (*p_in++ - min) / tic;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_16 (no_data, p_sv))
		    continue;
		if (gray < 0.0)
		    gray = 0.0;
		if (gray > 1023.0)
		    gray = 1023.0;
		histogram[(int) gray] += 1;
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		min2 = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		max2 = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    tic2 = (double) (max2 - min2) / 254.0;

/* rescaling gray-values 0-255 */
    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_16 (no_data, p_in);
		if (transparent)
		  {
		      p_in++;
		      p_out += 4;
		  }
		else
		  {
		      double gray;
		      short val = *p_in++;
		      if (val <= min2)
			  gray = 0.0;
		      else if (val >= max2)
			  gray = 255.0;
		      else
			  gray = 1.0 + (((double) val - min2) / tic2);
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 255.0)
			  gray = 255.0;
		      *p_out++ = (unsigned char) gray;	/* red */
		      *p_out++ = (unsigned char) gray;	/* green */
		      *p_out++ = (unsigned char) gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_uint16 (unsigned int width, unsigned int height,
		  unsigned short *pixels, unsigned char *mask,
		  rl2PrivPixelPtr no_data, unsigned char *rgba)
{
/* input: DataGrid UINT16   output: Grayscale */
    unsigned short *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    unsigned short min = USHRT_MAX;
    unsigned short max = 0;
    double min2 = 0.0;
    double max2 = 0.0;
    double tic;
    double tic2;
    int transparent;
    int i;
    int sum;
    int total;
    double percentile2;
    int histogram[1024];

/* identifying Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned short *p_sv = p_in;
		unsigned short gray = *p_in++;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_u16 (no_data, p_sv))
		    continue;
		if (min > gray)
		    min = gray;
		if (max < gray)
		    max = gray;
		total++;
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building an histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned short *p_sv = p_in;
		double gray = 0.0;
		if (tic > 0.0)
		    gray = 0.0;
		if (tic > 0.0)
		    gray = (double) (*p_in++ - min) / tic;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_u16 (no_data, p_sv))
		    continue;
		if (gray < 0.0)
		    gray = 0.0;
		if (gray > 1023.0)
		    gray = 1023.0;
		histogram[(int) gray] += 1;
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		min2 = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		max2 = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    tic2 = (double) (max2 - min2) / 254.0;

/* rescaling gray-values 0-255 */
    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_u16 (no_data, p_in);
		if (transparent)
		  {
		      p_in++;
		      p_out += 4;
		  }
		else
		  {
		      double gray;
		      unsigned short val = *p_in++;
		      if (val <= min2)
			  gray = 0.0;
		      else if (val >= max2)
			  gray = 255.0;
		      else
			  gray = 1.0 + (((double) val - min2) / tic2);
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 255.0)
			  gray = 255.0;
		      *p_out++ = (unsigned char) gray;	/* red */
		      *p_out++ = (unsigned char) gray;	/* green */
		      *p_out++ = (unsigned char) gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_int32 (unsigned int width, unsigned int height,
		 int *pixels, unsigned char *mask, rl2PrivPixelPtr no_data,
		 unsigned char *rgba)
{
/* input: DataGrid INT32   output: Grayscale */
    int *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int min = INT_MAX;
    int max = INT_MIN;
    double min2 = 0.0;
    double max2 = 0.0;
    double tic;
    double tic2;
    int transparent;
    int i;
    int sum;
    int total;
    double percentile2;
    int histogram[1024];

/* identifying Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		int *p_sv = p_in;
		int gray = *p_in++;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_32 (no_data, p_sv))
		    continue;
		if (min > gray)
		    min = gray;
		if (max < gray)
		    max = gray;
		total++;
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building an histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		int *p_sv = p_in;
		double gray = 0.0;
		if (tic > 0.0)
		    gray = (double) (*p_in++ - min) / tic;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_32 (no_data, p_sv))
		    continue;
		if (gray < 0.0)
		    gray = 0.0;
		if (gray > 1023.0)
		    gray = 1023.0;
		histogram[(int) gray] += 1;
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		min2 = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		max2 = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    tic2 = (double) (max2 - min2) / 254.0;

/* rescaling gray-values 0-255 */
    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_32 (no_data, p_in);
		if (transparent)
		  {
		      p_in++;
		      p_out += 4;
		  }
		else
		  {
		      double gray;
		      int val = *p_in++;
		      if (val <= min2)
			  gray = 0.0;
		      else if (val >= max2)
			  gray = 255.0;
		      else
			  gray = 1.0 + (((double) val - min2) / tic2);
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 255.0)
			  gray = 255.0;
		      *p_out++ = (unsigned char) gray;	/* red */
		      *p_out++ = (unsigned char) gray;	/* green */
		      *p_out++ = (unsigned char) gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_uint32 (unsigned int width, unsigned int height,
		  unsigned int *pixels, unsigned char *mask,
		  rl2PrivPixelPtr no_data, unsigned char *rgba)
{
/* input: DataGrid UINT32   output: Grayscale */
    unsigned int *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    unsigned int min = UINT_MAX;
    unsigned int max = 0;
    double min2 = 0.0;
    double max2 = 0.0;
    double tic;
    double tic2;
    int transparent;
    int i;
    int sum;
    int total;
    double percentile2;
    int histogram[1024];

/* identifying Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned int *p_sv = p_in;
		unsigned int gray = *p_in++;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_u32 (no_data, p_sv))
		    continue;
		if (min > gray)
		    min = gray;
		if (max < gray)
		    max = gray;
		total++;
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building an histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned int *p_sv = p_in;
		double gray = 0.0;
		if (tic > 0.0)
		    gray = (double) (*p_in++ - min) / tic;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_u32 (no_data, p_sv))
		    continue;
		if (gray < 0.0)
		    gray = 0.0;
		if (gray > 1023.0)
		    gray = 1023.0;
		histogram[(int) gray] += 1;
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		min2 = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		max2 = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    tic2 = (double) (max2 - min2) / 254.0;

/* rescaling gray-values 0-255 */
    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_u32 (no_data, p_in);
		if (transparent)
		  {
		      p_in++;
		      p_out += 4;
		  }
		else
		  {
		      double gray;
		      unsigned int val = *p_in++;
		      if (val <= min2)
			  gray = 0.0;
		      else if (val >= max2)
			  gray = 255.0;
		      else
			  gray = 1.0 + (((double) val - min2) / tic2);
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 255.0)
			  gray = 255.0;
		      *p_out++ = (unsigned char) gray;	/* red */
		      *p_out++ = (unsigned char) gray;	/* green */
		      *p_out++ = (unsigned char) gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_float (unsigned int width, unsigned int height,
		 float *pixels, unsigned char *mask, rl2PrivPixelPtr no_data,
		 unsigned char *rgba)
{
/* input: DataGrid FLOAT   output: Grayscale */
    float *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    float min = FLT_MAX;
    float max = 0.0 - FLT_MAX;
    double min2 = 0.0;
    double max2 = 0.0;
    double tic;
    double tic2;
    int transparent;
    int i;
    int sum;
    int total;
    double percentile2;
    int histogram[1024];

/* identifying Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		float *p_sv = p_in;
		float gray = *p_in++;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_flt (no_data, p_sv))
		    continue;
		if (min > gray)
		    min = gray;
		if (max < gray)
		    max = gray;
		total++;
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building an histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		float *p_sv = p_in;
		double gray = 0.0;
		if (tic > 0.0)
		    gray = (double) (*p_in++ - min) / tic;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_flt (no_data, p_sv))
		    continue;
		if (gray < 0.0)
		    gray = 0.0;
		if (gray > 1023.0)
		    gray = 1023.0;
		histogram[(int) gray] += 1;
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		min2 = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		max2 = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    tic2 = (double) (max2 - min2) / 254.0;

/* rescaling gray-values 0-255 */
    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_flt (no_data, p_in);
		if (transparent)
		  {
		      p_in++;
		      p_out += 4;
		  }
		else
		  {
		      double gray;
		      float val = *p_in++;
		      if (val <= min2)
			  gray = 0.0;
		      else if (val >= max2)
			  gray = 255.0;
		      else
			  gray = 1.0 + (((double) val - min2) / tic2);
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 255.0)
			  gray = 255.0;
		      *p_out++ = (unsigned char) gray;	/* red */
		      *p_out++ = (unsigned char) gray;	/* green */
		      *p_out++ = (unsigned char) gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_double (unsigned int width, unsigned int height,
		  double *pixels, unsigned char *mask,
		  rl2PrivPixelPtr no_data, unsigned char *rgba)
{
/* input: DataGrid DOUBLE   output: Grayscale */
    double *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    double min = DBL_MAX;
    double max = 0.0 - DBL_MAX;
    double min2 = 0.0;
    double max2 = 0.0;
    double tic;
    double tic2;
    int transparent;
    int i;
    int sum;
    int total;
    double percentile2;
    int histogram[1024];

/* identifying Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		double *p_sv = p_in;
		double gray = *p_in++;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_dbl (no_data, p_sv))
		    continue;
		if (min > gray)
		    min = gray;
		if (max < gray)
		    max = gray;
		total++;
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building an histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		double *p_sv = p_in;
		double gray = 0.0;
		if (tic > 0.0)
		    gray = (double) (*p_in++ - min) / tic;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_dbl (no_data, p_sv))
		    continue;
		if (gray < 0.0)
		    gray = 0.0;
		if (gray > 1023.0)
		    gray = 1023.0;
		histogram[(int) gray] += 1;
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		min2 = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		max2 = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    tic2 = (double) (max2 - min2) / 254.0;

/* rescaling gray-values 0-255 */
    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_dbl (no_data, p_in);
		if (transparent)
		  {
		      p_in++;
		      p_out += 4;
		  }
		else
		  {
		      double gray;
		      double val = *p_in++;
		      if (val <= min2)
			  gray = 0.0;
		      else if (val >= max2)
			  gray = 255.0;
		      else
			  gray = 1.0 + (((double) val - min2) / tic2);
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 255.0)
			  gray = 255.0;
		      *p_out++ = (unsigned char) gray;	/* red */
		      *p_out++ = (unsigned char) gray;	/* green */
		      *p_out++ = (unsigned char) gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_datagrid_mask (unsigned int width, unsigned int height,
			     unsigned char sample_type, void *pixels,
			     unsigned char *mask, rl2PrivPixelPtr no_data,
			     unsigned char *rgba)
{
/* input: DataGrid    output: Grayscale */
    int ret = 0;
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  ret =
	      rgba_from_int8 (width, height, (char *) pixels, mask, no_data,
			      rgba);
	  break;
      case RL2_SAMPLE_UINT8:
	  ret =
	      rgba_from_uint8 (width, height, (unsigned char *) pixels, mask,
			       no_data, rgba);
	  break;
      case RL2_SAMPLE_INT16:
	  ret =
	      rgba_from_int16 (width, height, (short *) pixels, mask, no_data,
			       rgba);
	  break;
      case RL2_SAMPLE_UINT16:
	  ret =
	      rgba_from_uint16 (width, height, (unsigned short *) pixels,
				mask, no_data, rgba);
	  break;
      case RL2_SAMPLE_INT32:
	  ret =
	      rgba_from_int32 (width, height, (int *) pixels, mask, no_data,
			       rgba);
	  break;
      case RL2_SAMPLE_UINT32:
	  ret =
	      rgba_from_uint32 (width, height, (unsigned int *) pixels, mask,
				no_data, rgba);
	  break;
      case RL2_SAMPLE_FLOAT:
	  ret =
	      rgba_from_float (width, height, (float *) pixels, mask, no_data,
			       rgba);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  ret =
	      rgba_from_double (width, height, (double *) pixels, mask,
				no_data, rgba);
	  break;
      };
    return ret;
}

RL2_PRIVATE int
rgba_from_multi_uint8 (unsigned int width, unsigned int height,
		       unsigned char num_bands, unsigned char *pixels,
		       unsigned char *mask, rl2PrivPixelPtr no_data,
		       unsigned char *rgba)
{
/* input: MultiBand UINT8   output: Grayscale */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char *p_sv = p_in;
		unsigned char gray = *p_in;	/* considering only Band #0 */
		p_in += num_bands;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_u8 (no_data, p_sv);
		if (transparent)
		    p_out += 4;
		else
		  {
		      *p_out++ = gray;	/* red */
		      *p_out++ = gray;	/* green */
		      *p_out++ = gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
rgba_from_multi_uint16 (unsigned int width, unsigned int height,
			unsigned char num_bands, unsigned short *pixels,
			unsigned char *mask, rl2PrivPixelPtr no_data,
			unsigned char *rgba)
{
/* input: MultiBand UINT16   output: Grayscale */
    unsigned short *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    unsigned short min = USHRT_MAX;
    unsigned short max = 0;
    double min2 = 0.0;
    double max2 = 0.0;
    double tic;
    double tic2;
    int transparent;
    int i;
    int sum;
    int total;
    double percentile2;
    int histogram[1024];

/* identifying Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned short *p_sv = p_in;
		unsigned short gray = *p_in;	/* considering only Band #0 */
		p_in += num_bands;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_u16 (no_data, p_sv))
		    continue;
		if (min > gray)
		    min = gray;
		if (max < gray)
		    max = gray;
		total++;
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building an histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned short *p_sv = p_in;
		double gray = 0.0;
		if (tic > 0.0)
		    gray = (double) (*p_in - min) / tic;
		p_in += num_bands;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  continue;
		  }
		if (test_no_data_u16 (no_data, p_sv))
		    continue;
		if (gray < 0.0)
		    gray = 0.0;
		if (gray > 1023.0)
		    gray = 1023.0;
		histogram[(int) gray] += 1;
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		min2 = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		max2 = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    tic2 = (double) (max2 - min2) / 254.0;

/* rescaling gray-values 0-255 */
    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = test_no_data_u16 (no_data, p_in);
		if (transparent)
		  {
		      p_in += num_bands;
		      p_out += 4;
		  }
		else
		  {
		      double gray;
		      unsigned short val = *p_in;
		      p_in += num_bands;	/* considering only Band #0 */
		      if (val <= min2)
			  gray = 0.0;
		      else if (val >= max2)
			  gray = 255.0;
		      else
			  gray = 1.0 + (((double) val - min2) / tic2);
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 255.0)
			  gray = 255.0;
		      *p_out++ = (unsigned char) gray;	/* red */
		      *p_out++ = (unsigned char) gray;	/* green */
		      *p_out++ = (unsigned char) gray;	/* blue */
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_multiband_mask (unsigned int width, unsigned int height,
			      unsigned char sample_type,
			      unsigned char num_bands, void *pixels,
			      unsigned char *mask, rl2PrivPixelPtr no_data,
			      unsigned char *rgba)
{
/* input: MultiBand    output: Grayscale */
    int ret = 0;
    switch (sample_type)
      {
      case RL2_SAMPLE_UINT8:
	  ret =
	      rgba_from_multi_uint8 (width, height, num_bands,
				     (unsigned char *) pixels, mask, no_data,
				     rgba);
	  break;
      case RL2_SAMPLE_UINT16:
	  ret =
	      rgba_from_multi_uint16 (width, height, num_bands,
				      (unsigned short *) pixels, mask,
				      no_data, rgba);
	  break;
      };
    return ret;
}

RL2_PRIVATE int
get_payload_from_gray_rgba_opaque (unsigned int width, unsigned int height,
				   sqlite3 * handle, double minx, double miny,
				   double maxx, double maxy, int srid,
				   unsigned char *rgb, unsigned char format,
				   int quality, unsigned char **image,
				   int *image_sz)
{
/* Grayscale, Opaque */
    int ret;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;
    unsigned char *rgba = NULL;
    unsigned char *gray = malloc (width * height);

    if (gray == NULL)
	goto error;
    p_in = rgb;
    p_out = gray;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		*p_out++ = *p_in++;
		p_in += 2;
	    }
      }
    if (format == RL2_OUTPUT_FORMAT_JPEG)
      {
	  if (rl2_gray_to_jpeg (width, height, gray, quality, image, image_sz)
	      != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_gray_to_png (width, height, gray, image, image_sz) != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_TIFF)
      {
	  if (srid > 0)
	    {
		if (rl2_gray_to_geotiff
		    (width, height, handle, minx, miny, maxx, maxy, srid,
		     gray, image, image_sz) != RL2_OK)
		    goto error;
	    }
	  else
	    {
		if (rl2_gray_to_tiff (width, height, gray, image, image_sz) !=
		    RL2_OK)
		    goto error;
	    }
      }
    else if (format == RL2_OUTPUT_FORMAT_PDF)
      {
	  rgba = gray_to_rgba (width, height, gray);
	  if (rgba == NULL)
	      goto error;
	  ret = rl2_rgba_to_pdf (width, height, rgba, image, image_sz);
	  rgba = NULL;
	  if (ret != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (gray);
    return 1;
  error:
    if (gray != NULL)
	free (gray);
    if (rgba != NULL)
	free (rgba);
    return 0;
}

RL2_PRIVATE int
get_payload_from_gray_rgba_transparent (unsigned int width,
					unsigned int height,
					unsigned char *rgb,
					unsigned char *alpha,
					unsigned char format, int quality,
					unsigned char **image, int *image_sz,
					double opacity)
{
/* Grayscale, Transparent */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned char *p_alpha;
    unsigned short row;
    unsigned short col;
    unsigned char *gray = malloc (width * height);
    unsigned char *mask = malloc (width * height);

    if (quality > 100)
	quality = 100;
    if (gray == NULL)
	goto error;
    if (mask == NULL)
	goto error;
    p_in = rgb;
    p_out = gray;
    p_msk = mask;
    p_alpha = alpha;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		*p_out++ = *p_in++;
		p_in += 2;
		if (*p_alpha++ >= 128)
		    *p_msk++ = 1;	/* Opaque */
		else
		    *p_msk++ = 0;	/* Transparent */
	    }
      }
    if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_gray_alpha_to_png
	      (width, height, gray, mask, image, image_sz, opacity) != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (gray);
    free (mask);
    return 1;
  error:
    if (gray != NULL)
	free (gray);
    if (mask != NULL)
	free (mask);
    return 0;
}

RL2_PRIVATE int
get_payload_from_rgb_rgba_opaque (unsigned int width, unsigned int height,
				  sqlite3 * handle, double minx, double miny,
				  double maxx, double maxy, int srid,
				  unsigned char *rgb, unsigned char format,
				  int quality, unsigned char **image,
				  int *image_sz)
{
/* RGB, Opaque */
    int ret;
    unsigned char *rgba = NULL;

    if (format == RL2_OUTPUT_FORMAT_JPEG)
      {
	  if (rl2_rgb_to_jpeg (width, height, rgb, quality, image, image_sz)
	      != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (rl2_rgb_to_png (width, height, rgb, image, image_sz) != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_TIFF)
      {
	  if (srid > 0)
	    {
		if (rl2_rgb_to_geotiff
		    (width, height, handle, minx, miny, maxx, maxy, srid, rgb,
		     image, image_sz) != RL2_OK)
		    goto error;
	    }
	  else
	    {
		if (rl2_rgb_to_tiff (width, height, rgb, image, image_sz) !=
		    RL2_OK)
		    goto error;
	    }
      }
    else if (format == RL2_OUTPUT_FORMAT_PDF)
      {
	  rgba = rgb_to_rgba (width, height, rgb);
	  if (rgba == NULL)
	      goto error;
	  ret = rl2_rgba_to_pdf (width, height, rgba, image, image_sz);
	  rgba = NULL;
	  if (ret != RL2_OK)
	      goto error;
      }
    else
	goto error;
    return 1;
  error:
    if (rgba != NULL)
	free (rgba);
    return 0;
}

RL2_PRIVATE int
get_payload_from_rgb_rgba_transparent (unsigned int width,
				       unsigned int height,
				       unsigned char *rgb,
				       unsigned char *alpha,
				       unsigned char format, int quality,
				       unsigned char **image, int *image_sz,
				       double opacity, int half_transparency)
{
/* RGB, Transparent */
    int ret;
    unsigned char *p_msk;
    unsigned char *p_alpha;
    unsigned int row;
    unsigned int col;
    unsigned char *mask = malloc (width * height);

    if (quality > 100)
	quality = 100;
    if (mask == NULL)
	goto error;
    p_msk = mask;
    p_alpha = alpha;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_alpha++ > 128)
		    *p_msk++ = 1;	/* Opaque */
		else
		    *p_msk++ = 0;	/* Transparent */
	    }
      }
    if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (half_transparency)
	      ret =
		  rl2_rgb_real_alpha_to_png (width, height, rgb, alpha, image,
					     image_sz);
	  else
	      ret =
		  rl2_rgb_alpha_to_png (width, height, rgb, mask, image,
					image_sz, opacity);
	  if (ret != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_JPEG)
      {
	  ret = rl2_rgb_to_jpeg (width, height, rgb, quality, image, image_sz);
	  if (ret != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_TIFF)
      {
	  ret = rl2_rgb_to_tiff (width, height, rgb, image, image_sz);
	  if (ret != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_PDF)
      {
	  unsigned char *rgba = rgb_to_rgba (width, height, rgb);
	  if (rgba == NULL)
	      goto error;
	  ret = rl2_rgba_to_pdf (width, height, rgba, image, image_sz);
	  rgba = NULL;
	  if (ret != RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (mask);
    return 1;
  error:
    if (mask != NULL)
	free (mask);
    return 0;
}

RL2_PRIVATE int
build_rgb_alpha (unsigned int width, unsigned int height,
		 unsigned char *rgba, unsigned char **rgb,
		 unsigned char **alpha, unsigned char bg_red,
		 unsigned char bg_green, unsigned char bg_blue)
{
/* creating separate RGB and Alpha buffers from RGBA */
    unsigned int row;
    unsigned int col;
    unsigned char *p_in = rgba;
    unsigned char *p_out;
    unsigned char *p_msk;

    *rgb = NULL;
    *alpha = NULL;
    *rgb = malloc (width * height * 3);
    if (*rgb == NULL)
	goto error;
    *alpha = malloc (width * height);
    if (*alpha == NULL)
	goto error;

    p_out = *rgb;
    p_msk = *alpha;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char r = *p_in++;
		unsigned char g = *p_in++;
		unsigned char b = *p_in++;
		unsigned char alpha = *p_in++;
		*p_out++ = r;
		*p_out++ = g;
		*p_out++ = b;
		if (r == bg_red && g == bg_green && b == bg_blue)
		    alpha = 0;
		*p_msk++ = alpha;
	    }
      }
    return 1;

  error:
    if (*rgb != NULL)
	free (*rgb);
    if (*alpha != NULL)
	free (*alpha);
    *rgb = NULL;
    *alpha = NULL;
    return 0;
}

RL2_PRIVATE int
build_rgb_alpha_transparent (unsigned int width, unsigned int height,
			     unsigned char *rgba, unsigned char **rgb,
			     unsigned char **alpha)
{
/* creating separate RGB and Alpha buffers from RGBA */
    unsigned int row;
    unsigned int col;
    unsigned char *p_in = rgba;
    unsigned char *p_out;
    unsigned char *p_msk;

    *rgb = NULL;
    *alpha = NULL;
    *rgb = malloc (width * height * 3);
    if (*rgb == NULL)
	goto error;
    *alpha = malloc (width * height);
    if (*alpha == NULL)
	goto error;

    p_out = *rgb;
    p_msk = *alpha;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		unsigned char r = *p_in++;
		unsigned char g = *p_in++;
		unsigned char b = *p_in++;
		unsigned char alpha = *p_in++;
		*p_out++ = r;
		*p_out++ = g;
		*p_out++ = b;
		*p_msk++ = alpha;
	    }
      }
    return 1;

  error:
    if (*rgb != NULL)
	free (*rgb);
    if (*alpha != NULL)
	free (*alpha);
    *rgb = NULL;
    *alpha = NULL;
    return 0;
}

RL2_PRIVATE int
get_rgba_from_multiband8 (unsigned int width, unsigned int height,
			  unsigned char red_band, unsigned char green_band,
			  unsigned char blue_band, unsigned char num_bands,
			  unsigned char *pixels, unsigned char *mask,
			  rl2PrivPixelPtr no_data, unsigned char *rgba)
{
/* input: MULTIBAND UINT8   output: RGB */
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;

    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent && no_data != NULL)
		  {
		      /* testing for NO-DATA */
		      int match = 0;
		      rl2PrivSamplePtr sample;
		      unsigned char value;
		      if (red_band < no_data->nBands)
			{
			    sample = no_data->Samples + red_band;
			    value = sample->uint8;
			    if (*(p_in + red_band) == value)
				match++;
			}
		      if (green_band < no_data->nBands)
			{
			    sample = no_data->Samples + green_band;
			    value = sample->uint8;
			    if (*(p_in + green_band) == value)
				match++;
			}
		      if (blue_band < no_data->nBands)
			{
			    sample = no_data->Samples + blue_band;
			    value = sample->uint8;
			    if (*(p_in + blue_band) == value)
				match++;
			}
		      if (match == 3)
			  transparent = 1;
		  }
		if (transparent)
		  {
		      p_out += 4;
		      p_in += num_bands;
		  }
		else
		  {
		      *p_out++ = *(p_in + red_band);	/* red */
		      *p_out++ = *(p_in + green_band);	/* green */
		      *p_out++ = *(p_in + blue_band);	/* blue */
		      *p_out++ = 255;	/* opaque */
		      p_in += num_bands;
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_multiband16 (unsigned int width, unsigned int height,
			   unsigned char red_band, unsigned char green_band,
			   unsigned char blue_band, unsigned char num_bands,
			   unsigned short *pixels, unsigned char *mask,
			   rl2PrivPixelPtr no_data, unsigned char *rgba)
{
/* input: MULTIBAND UINT16   output: RGB */
    unsigned short *p_in;
    unsigned char *p_out;
    unsigned char *p_msk;
    unsigned int row;
    unsigned int col;
    int transparent;
    unsigned short min = USHRT_MAX;
    unsigned short max = 0;
    double tic;
    double red_min = DBL_MAX;
    double red_max = 0.0 - DBL_MAX;
    double red_tic;
    double green_min = DBL_MAX;
    double green_max = 0.0 - DBL_MAX;
    double green_tic;
    double blue_min = DBL_MAX;
    double blue_max = 0.0 - DBL_MAX;
    double blue_tic;
    int i;
    int sum;
    int total;
    int band;
    double percentile2;
    int histogram[1024];

/* identifying RED Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			{
			    p_in += num_bands;
			    continue;
			}
		  }
		if (test_no_data_u16 (no_data, p_in))
		  {
		      p_in += num_bands;
		      continue;
		  }
		for (band = 0; band < num_bands; band++)
		  {
		      unsigned short gray = *p_in++;
		      if (band != red_band)
			  continue;
		      if (min > gray)
			  min = gray;
		      if (max < gray)
			  max = gray;
		      total++;
		  }
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building the RED histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			{
			    p_in += num_bands;
			    continue;
			}
		  }
		if (test_no_data_u16 (no_data, p_in))
		  {
		      p_in += num_bands;
		      continue;
		  }
		for (band = 0; band < num_bands; band++)
		  {
		      double gray = 0.0;
		      if (tic > 0.0)
			  gray = (double) (*p_in++ - min) / tic;
		      if (band != red_band)
			  continue;
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 1023.0)
			  gray = 1023.0;
		      histogram[(int) gray] += 1;
		  }
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		red_min = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		red_max = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    red_tic = (double) (red_max - red_min) / 254.0;

/* identifying GREEN Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			{
			    p_in += num_bands;
			    continue;
			}
		  }
		if (test_no_data_u16 (no_data, p_in))
		  {
		      p_in += num_bands;
		      continue;
		  }
		for (band = 0; band < num_bands; band++)
		  {
		      unsigned short gray = *p_in++;
		      if (band != green_band)
			  continue;
		      if (min > gray)
			  min = gray;
		      if (max < gray)
			  max = gray;
		      total++;
		  }
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building the GREEN histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			{
			    p_in += num_bands;
			    continue;
			}
		  }
		if (test_no_data_u16 (no_data, p_in))
		  {
		      p_in += num_bands;
		      continue;
		  }
		for (band = 0; band < num_bands; band++)
		  {
		      double gray = 0.0;
		      if (tic > 0.0)
			  gray = (double) (*p_in++ - min) / tic;
		      if (band != green_band)
			  continue;
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 1023.0)
			  gray = 1023.0;
		      histogram[(int) gray] += 1;
		  }
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		green_min = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		green_max = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    green_tic = (double) (green_max - green_min) / 254.0;

/* identifying BLUE Min/Max values */
    total = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			{
			    p_in += num_bands;
			    continue;
			}
		  }
		if (test_no_data_u16 (no_data, p_in))
		  {
		      p_in += num_bands;
		      continue;
		  }
		for (band = 0; band < num_bands; band++)
		  {
		      unsigned short gray = *p_in++;
		      if (band != blue_band)
			  continue;
		      if (min > gray)
			  min = gray;
		      if (max < gray)
			  max = gray;
		      total++;
		  }
	    }
      }
    tic = (double) (max - min) / 1024.0;
    percentile2 = ((double) total / 100.0) * 2.0;

/* building the BLUE histogram */
    for (i = 0; i < 1024; i++)
	histogram[i] = 0;
    p_in = pixels;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			{
			    p_in += num_bands;
			    continue;
			}
		  }
		if (test_no_data_u16 (no_data, p_in))
		  {
		      p_in += num_bands;
		      continue;
		  }
		for (band = 0; band < num_bands; band++)
		  {
		      double gray = 0.0;
		      if (tic > 0.0)
			  gray = (double) (*p_in++ - min) / tic;
		      if (band != blue_band)
			  continue;
		      if (gray < 0.0)
			  gray = 0.0;
		      if (gray > 1023.0)
			  gray = 1023.0;
		      histogram[(int) gray] += 1;
		  }
	    }
      }
    sum = 0;
    for (i = 0; i < 1024; i++)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		blue_min = (double) min + ((double) i * tic);
		break;
	    }
      }
    sum = 0;
    for (i = 1023; i >= 0; i--)
      {
	  sum += histogram[i];
	  if (sum >= percentile2)
	    {
		blue_max = (double) min + ((double) (i + 1) * tic);
		break;
	    }
      }
    blue_tic = (double) (blue_max - blue_min) / 254.0;

/* rescaling RGB-values 0-255 */
    p_in = pixels;
    p_out = rgba;
    p_msk = mask;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (test_no_data_u16 (no_data, p_in))
		    transparent = 1;
		if (transparent)
		  {
		      p_out += 4;
		      p_in += num_bands;
		  }
		else
		  {
		      double r;
		      double g;
		      double b;
		      unsigned short red = *(p_in + red_band);
		      unsigned short green = *(p_in + green_band);
		      unsigned short blue = *(p_in + blue_band);
		      if (red <= red_min)
			  r = 0.0;
		      else if (red >= red_max)
			  r = 255.0;
		      else
			  r = 1.0 + (((double) red - red_min) / red_tic);
		      if (r < 0.0)
			  r = 0.0;
		      if (r > 255.0)
			  r = 255.0;
		      if (green <= green_min)
			  g = 0.0;
		      else if (green >= green_max)
			  g = 255.0;
		      else
			  g = 1.0 + (((double) green - green_min) / green_tic);
		      if (g < 0.0)
			  g = 0.0;
		      if (g > 255.0)
			  g = 255.0;
		      if (blue <= blue_min)
			  b = 0.0;
		      else if (blue >= blue_max)
			  b = 255.0;
		      else
			  b = 1.0 + (((double) blue - blue_min) / blue_tic);
		      if (b < 0.0)
			  b = 0.0;
		      if (b > 255.0)
			  b = 255.0;
		      *p_out++ = (unsigned char) r;	/* red */
		      *p_out++ = (unsigned char) g;	/* green */
		      *p_out++ = (unsigned char) b;	/* blue */
		      *p_out++ = 255;	/* opaque */
		      p_in += num_bands;
		  }
	    }
      }
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;
}

RL2_PRIVATE int
get_raster_band_histogram (rl2PrivBandStatisticsPtr band,
			   unsigned char **image, int *image_sz)
{
/* attempting to create an in-memory PNG image representing a Band Histogram */
    int r;
    int c;
    int j;
    int h;
    double count = 0.0;
    double max = 0.0;
    unsigned short width = 512;
    unsigned short height = 128 + 32;
    double scale;
    unsigned char *raster = malloc (width * height);
    unsigned char *p = raster;
    for (r = 0; r < height; r++)
      {
	  /* priming a WHITE background */
	  for (c = 0; c < width; c++)
	      *p++ = 255;
      }
    for (j = 1; j < 256; j++)
      {
	  /* computing optimal height */
	  double value = *(band->histogram + j);
	  count += value;
	  if (max < value)
	      max = value;
      }
    scale = 1.0 / (max / count);
    for (j = 1; j < 256; j++)
      {
	  /* drawing the histogram */
	  double freq = *(band->histogram + j);
	  double high = (height - 32.0) * scale * freq / count;
	  r = (j - 1) * 2;
	  for (c = 0, h = height - 32; c < high; c++, h--)
	    {
		p = raster + (h * width) + r;
		*p++ = 128;
		*p = 128;
	    }
      }
    for (j = 1; j < 256; j++)
      {
	  /* drawing the scale-bar */
	  r = (j - 1) * 2;
	  for (c = 0, h = height - 1; c < 25; c++, h--)
	    {
		p = raster + (h * width) + r;
		*p++ = j;
		*p = j;
	    }
      }
    if (rl2_data_to_png
	(raster, NULL, 1.0, NULL, width, height, RL2_SAMPLE_UINT8,
	 RL2_PIXEL_GRAYSCALE, 1, image, image_sz) == RL2_OK)
      {
	  free (raster);
	  return RL2_OK;
      }
    free (raster);
    return RL2_ERROR;
}

RL2_PRIVATE int
set_coverage_infos (sqlite3 * sqlite, const char *coverage_name,
		    const char *title, const char *abstract, int is_queryable)
{
/* auxiliary function: updates the Coverage descriptive infos */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int exists = 0;
    int retval = 0;

/* checking if the Coverage already exists */
    sql = "SELECT coverage_name FROM main.raster_coverages "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SetCoverageInfos: \"%s\"\n",
		   sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      exists = 1;
      }
    sqlite3_finalize (stmt);

    if (!exists)
	return 0;
/* updating the Coverage */
    if (is_queryable < 0)
      {
	  sql = "UPDATE main.raster_coverages SET title = ?, abstract = ? "
	      "WHERE Lower(coverage_name) = Lower(?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "SetCoverageInfos: \"%s\"\n",
			 sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
      }
    else
      {
	  sql =
	      "UPDATE main.raster_coverages SET title = ?, abstract = ?, is_queryable = ? "
	      "WHERE Lower(coverage_name) = Lower(?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "SetCoverageInfos: \"%s\"\n",
			 sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  sqlite3_bind_int (stmt, 3, is_queryable);
	  sqlite3_bind_text (stmt, 4, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	fprintf (stderr, "SetCoverageInfos() error: \"%s\"\n",
		 sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

RL2_PRIVATE int
set_coverage_copyright (sqlite3 * sqlite, const char *coverage_name,
			const char *copyright, const char *license)
{
/* auxiliary function: updates the copyright infos supporting a Coverage infos */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int exists = 0;

    if (coverage_name == NULL)
	return 0;
    if (copyright == NULL && license == NULL)
	return 1;

/* checking if the Coverage already exists */
    sql = "SELECT coverage_name FROM main.raster_coverages "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SetCoverageInfos: \"%s\"\n",
		   sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      exists = 1;
      }
    sqlite3_finalize (stmt);

    if (!exists)
	return 0;
/* updating the Coverage */

    if (copyright == NULL)
      {
	  /* just updating the License */
	  sql = "UPDATE main.raster_coverages SET license = ("
	      "SELECT id FROM data_licenses WHERE name = ?) "
	      "WHERE Lower(coverage_name) = Lower(?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "setRasterCoverageCopyright: \"%s\"\n",
			 sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, license, strlen (license), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
      }
    else if (license == NULL)
      {
	  /* just updating the Copyright */
	  sql = "UPDATE main.raster_coverages SET copyright = ? "
	      "WHERE Lower(coverage_name) = Lower(?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "setRasterCoverageCopyright: \"%s\"\n",
			 sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, copyright, strlen (copyright),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
      }
    else
      {
	  /* updating both Copyright and License */
	  sql = "UPDATE main.raster_coverages SET copyright = ?, license = ("
	      "SELECT id FROM data_licenses WHERE name = ?) "
	      "WHERE Lower(coverage_name) = Lower(?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "setRasterCoverageCopyright: \"%s\"\n",
			 sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, copyright, strlen (copyright),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, license, strlen (license), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr, "setRasterCoverageCopyright() error: \"%s\"\n",
		   sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
  stop:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

RL2_PRIVATE int
rl2_test_layer_group (sqlite3 * handle, const char *db_prefix, const char *name)
{
/* testing for an eventual Layer Group */
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    int ok = 0;
    char *sql;
    char *xdb_prefix;

/* testing if the Layer Group exists */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT group_name FROM \"%s\".SE_styled_groups "
			 "WHERE Lower(group_name) = Lower(%Q)", xdb_prefix,
			 name);
    free (xdb_prefix);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	ok = 1;
    sqlite3_free_table (results);
    return ok;
}

RL2_PRIVATE int
rl2_is_mixed_resolutions_coverage (sqlite3 * handle, const char *db_prefix,
				   const char *coverage)
{
/* querying the Coverage Policies defs */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    int value = -1;
    char *xdb_prefix;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    sql = sqlite3_mprintf ("SELECT mixed_resolutions "
			   "FROM \"%s\".raster_coverages WHERE Lower(coverage_name) = Lower(?)",
			   xdb_prefix);
    free (xdb_prefix);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  return value;
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
		if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
		    value = sqlite3_column_int (stmt, 0);
	    }
      }
    sqlite3_finalize (stmt);
    return value;
}

RL2_PRIVATE char *
rl2_double_quoted_sql (const char *value)
{
/*
/ returns a well formatted TEXT value for SQL
/ 1] strips trailing spaces
/ 2] masks any QUOTE inside the string, appending another QUOTE
*/
    const char *p_in;
    const char *p_end;
    char qt = '"';
    char *out;
    char *p_out;
    int len = 0;
    int i;

    if (!value)
	return NULL;

    p_end = value;
    for (i = (strlen (value) - 1); i >= 0; i--)
      {
	  /* stripping trailing spaces */
	  p_end = value + i;
	  if (value[i] != ' ')
	      break;
      }

    p_in = value;
    while (p_in <= p_end)
      {
	  /* computing the output length */
	  len++;
	  if (*p_in == qt)
	      len++;
	  p_in++;
      }
    if (len == 1 && *value == ' ')
      {
	  /* empty string */
	  len = 0;
      }

    out = malloc (len + 1);
    if (!out)
	return NULL;

    if (len == 0)
      {
	  /* empty string */
	  *out = '\0';
	  return out;
      }

    p_out = out;
    p_in = value;
    while (p_in <= p_end)
      {
	  /* creating the output string */
	  if (*p_in == qt)
	      *p_out++ = qt;
	  *p_out++ = *p_in++;
      }
    *p_out = '\0';
    return out;
}

static int
test_geographic_srid (sqlite3 * handle, int srid)
{
/* testing if some SRID is of the Geographic type */
    int ret;
    int is_geographic = 0;
    sqlite3_stmt *stmt = NULL;
    const char *sql;

    sql = "SELECT SridIsGeographic(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, srid);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      is_geographic = sqlite3_column_int (stmt, 0);
      }
    sqlite3_finalize (stmt);
    return is_geographic;
}

static double
standard_scale (sqlite3 * handle, int srid, int width, int height,
		double ext_x, double ext_y)
{
/* computing the standard (normalized) scale */
    double linear_res;
    double factor;
    int is_geographic = test_geographic_srid (handle, srid);
    if (is_geographic)
      {
	  /* geographic (long/lat) CRS */
	  double metres = ext_x * (6378137.0 * 2.0 * 3.141592653589793) / 360.0;
	  linear_res = metres / (double) width;
      }
    else
      {
	  /* planar (projected) CRS */
	  double x_res = ext_x / (double) width;
	  double y_res = ext_y / (double) height;
	  linear_res = sqrt (x_res * y_res);
      }
    factor = linear_res / 0.000254;
    return factor * (0.28 / 0.254);
}

static void
do_set_canvas_ready (rl2CanvasPtr ptr, int which)
{
/* setting a Canvas as ready to be used */
    rl2PrivCanvasPtr canvas = (rl2PrivCanvasPtr) ptr;
    if (canvas == NULL)
	return;
    switch (which)
      {
      case RL2_CANVAS_BASE_CTX:
	  canvas->ctx_ready = RL2_TRUE;
	  break;
      case RL2_CANVAS_NODES_CTX:
	  canvas->ctx_nodes_ready = RL2_TRUE;
	  break;
      case RL2_CANVAS_EDGES_CTX:
	  canvas->ctx_edges_ready = RL2_TRUE;
	  break;
      case RL2_CANVAS_LINKS_CTX:
	  canvas->ctx_links_ready = RL2_TRUE;
	  break;
      case RL2_CANVAS_FACES_CTX:
	  canvas->ctx_faces_ready = RL2_TRUE;
	  break;
      case RL2_CANVAS_EDGE_SEEDS_CTX:
	  canvas->ctx_edge_seeds_ready = RL2_TRUE;
	  break;
      case RL2_CANVAS_LINK_SEEDS_CTX:
	  canvas->ctx_link_seeds_ready = RL2_TRUE;
	  break;
      case RL2_CANVAS_FACE_SEEDS_CTX:
	  canvas->ctx_face_seeds_ready = RL2_TRUE;
	  break;
      };
}

static int
do_transform_raster_bbox (struct aux_renderer *aux)
{
/* transforming a BBOX into another SRID */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    const unsigned char *x_blob;
    int x_blob_sz;
    int count = 0;
    rl2LinestringPtr line;
    unsigned char *blob;
    int blob_sz;
    sqlite3 *handle = aux->sqlite;
    int srid_out = aux->out_srid;
    int srid = aux->srid;
    double minx = aux->minx;
    double miny = aux->miny;
    double maxx = aux->maxx;
    double maxy = aux->maxy;
    double cx = minx + ((maxx - minx) / 2.0);
    double cy = miny + ((maxy - miny) / 2.0);

    sql = "SELECT ST_Transform(SetSRID(?, ?), ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT BBOX-reproject SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    line = rl2CreateLinestring (5, GAIA_XY);
    rl2SetPoint (line->coords, 0, minx, miny);
    rl2SetPoint (line->coords, 1, maxx, miny);
    rl2SetPoint (line->coords, 2, maxx, maxy);
    rl2SetPoint (line->coords, 3, minx, maxy);
    rl2SetPoint (line->coords, 4, cx, cy);
    if (!rl2_serialize_linestring (line, &blob, &blob_sz))
	goto error;
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
    rl2DestroyLinestring (line);
    sqlite3_bind_int (stmt, 2, srid_out);
    sqlite3_bind_int (stmt, 3, srid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      double nat_cx;
		      double nat_cy;
		      double nat_ll_x;
		      double nat_ll_y;
		      double nat_lr_x;
		      double nat_lr_y;
		      double nat_ur_x;
		      double nat_ur_y;
		      double nat_ul_x;
		      double nat_ul_y;
		      double nat_minx;
		      double nat_miny;
		      double nat_maxx;
		      double nat_maxy;
		      rl2GeometryPtr geom = NULL;
		      rl2LinestringPtr line = NULL;
		      x_blob = sqlite3_column_blob (stmt, 0);
		      x_blob_sz = sqlite3_column_bytes (stmt, 0);
		      geom = rl2_geometry_from_blob (x_blob, x_blob_sz);
		      if (geom == NULL)
			  goto error;
		      line = geom->first_linestring;
		      if (line == NULL)
			{
			    rl2_destroy_geometry (geom);
			    goto error;
			}
		      if (line->points != 5)
			{
			    rl2_destroy_geometry (geom);
			    goto error;
			}
		      rl2GetPoint (line->coords, 0, &nat_ll_x, &nat_ll_y);
		      rl2GetPoint (line->coords, 1, &nat_lr_x, &nat_lr_y);
		      rl2GetPoint (line->coords, 2, &nat_ur_x, &nat_ur_y);
		      rl2GetPoint (line->coords, 3, &nat_ul_x, &nat_ul_y);
		      rl2GetPoint (line->coords, 4, &nat_cx, &nat_cy);
		      rl2_destroy_geometry (geom);
		      count++;
		      /* determining the BBOX in Native SRID */
		      nat_minx = nat_ll_x;
		      if (nat_lr_x < nat_minx)
			  nat_minx = nat_lr_x;
		      if (nat_ur_x < nat_minx)
			  nat_minx = nat_ur_x;
		      if (nat_ul_x < nat_minx)
			  nat_minx = nat_ul_x;
		      nat_miny = nat_lr_y;
		      if (nat_lr_y < nat_miny)
			  nat_miny = nat_lr_y;
		      if (nat_ur_y < nat_miny)
			  nat_miny = nat_ur_y;
		      if (nat_ul_y < nat_miny)
			  nat_miny = nat_ul_y;
		      nat_maxx = nat_ll_x;
		      if (nat_lr_x > nat_maxx)
			  nat_maxx = nat_lr_x;
		      if (nat_ur_x > nat_maxx)
			  nat_maxx = nat_ur_x;
		      if (nat_ul_x > nat_maxx)
			  nat_maxx = nat_ul_x;
		      nat_maxy = nat_lr_y;
		      if (nat_lr_y > nat_maxy)
			  nat_maxy = nat_lr_y;
		      if (nat_ur_y > nat_maxy)
			  nat_maxy = nat_ur_y;
		      if (nat_ul_y > nat_maxy)
			  nat_maxy = nat_ul_y;

		      aux->native_ll_x = nat_ll_x;
		      aux->native_ll_y = nat_ll_y;
		      aux->native_lr_x = nat_lr_x;
		      aux->native_lr_y = nat_lr_y;
		      aux->native_ur_x = nat_ur_x;
		      aux->native_ur_y = nat_ur_y;
		      aux->native_ul_x = nat_ul_x;
		      aux->native_ul_y = nat_ul_y;
		      aux->native_cx = nat_cx;
		      aux->native_cy = nat_cy;
		      aux->native_minx = nat_minx;
		      aux->native_miny = nat_miny;
		      aux->native_maxx = nat_maxx;
		      aux->native_maxy = nat_maxy;
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT BBOX-reproject; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
do_compute_atm (struct aux_renderer *aux)
{
/* computing polynomial coeffs and Affine Transform Matrix */
    int retcode = 0;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *sql;
    char *tmp_table = NULL;
    char *xtmp_table;
    double xx;
    double yx;
    double xy;
    double yy;
    double xoff;
    double yoff;
    int count = 0;
    sqlite3 *handle = aux->sqlite;
    const void *data = aux->data;
    double dest_ll_x = aux->native_ll_x;
    double dest_ll_y = aux->native_ll_y;
    double dest_lr_x = aux->native_lr_x;
    double dest_lr_y = aux->native_lr_y;
    double dest_ur_x = aux->native_ur_x;
    double dest_ur_y = aux->native_ur_y;
    double dest_ul_x = aux->native_ul_x;
    double dest_ul_y = aux->native_ul_y;
    double orig_ll_x = aux->minx;
    double orig_ll_y = aux->miny;
    double orig_lr_x = aux->maxx;
    double orig_lr_y = aux->miny;
    double orig_ur_x = aux->maxx;
    double orig_ur_y = aux->maxy;
    double orig_ul_x = aux->minx;
    double orig_ul_y = aux->maxy;

    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  tmp_table = rl2_init_tmp_atm_table (priv_data);
      }
    else
	return 0;
    if (tmp_table == NULL)
	return 0;

/* creating a temporary table */
    xtmp_table = rl2_double_quoted_sql (tmp_table);
    sql =
	sqlite3_mprintf
	("CREATE TEMPORARY TABLE IF NOT EXISTS \"%s\" (g1 BLOB, g2 BLOB)",
	 xtmp_table);
    free (xtmp_table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("CREATE TEMPORARY TABLE ATM; SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto stop;
      }

/* deleting all rows from the temp table */
    xtmp_table = rl2_double_quoted_sql (tmp_table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\"", xtmp_table);
    free (xtmp_table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("DELETE FROM ATM; SQL error: %s\n", sqlite3_errmsg (handle));
	  goto stop;
      }

/* inserting GCP points into the tmp table */
    xtmp_table = rl2_double_quoted_sql (tmp_table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" VALUES (MakePoint(?, ?), MakePoint(?, ?))",
	 xtmp_table);
    free (xtmp_table);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSER INTO tmp_ggp; SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto stop;
      }
/* first pair of corresponding points: Lower Left corner */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, orig_ll_x);
    sqlite3_bind_double (stmt, 2, orig_ll_y);
    sqlite3_bind_double (stmt, 3, dest_ll_x);
    sqlite3_bind_double (stmt, 4, dest_ll_y);
    ret = sqlite3_step (stmt);
    if (ret != SQLITE_DONE && ret != SQLITE_ROW)
      {
	  printf ("INSER INTO tmp_gcp; SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto stop;
      }
/* second pair of corresponding points: Lower Right corner */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, orig_lr_x);
    sqlite3_bind_double (stmt, 2, orig_lr_y);
    sqlite3_bind_double (stmt, 3, dest_lr_x);
    sqlite3_bind_double (stmt, 4, dest_lr_y);
    ret = sqlite3_step (stmt);
    if (ret != SQLITE_DONE && ret != SQLITE_ROW)
      {
	  printf ("INSER INTO tmp_gcp; SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto stop;
      }
/* third pair of corresponding points: Upper Right corner */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, orig_ur_x);
    sqlite3_bind_double (stmt, 2, orig_ur_y);
    sqlite3_bind_double (stmt, 3, dest_ur_x);
    sqlite3_bind_double (stmt, 4, dest_ur_y);
    ret = sqlite3_step (stmt);
    if (ret != SQLITE_DONE && ret != SQLITE_ROW)
      {
	  printf ("INSER INTO tmp_gcp; SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto stop;
      }
/* fourth pair of corresponding points: Upper Left corner */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, orig_ul_x);
    sqlite3_bind_double (stmt, 2, orig_ul_y);
    sqlite3_bind_double (stmt, 3, dest_ul_x);
    sqlite3_bind_double (stmt, 4, dest_ul_y);
    ret = sqlite3_step (stmt);
    if (ret != SQLITE_DONE && ret != SQLITE_ROW)
      {
	  printf ("INSER INTO tmp_gcp; SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto stop;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* fetching the Affine Transform Matrix */
    xtmp_table = rl2_double_quoted_sql (tmp_table);
    sql =
	sqlite3_mprintf ("SELECT GCP2ATM(GCP_Compute(g1, g2)) FROM \"%s\"",
			 xtmp_table);
    free (xtmp_table);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT CGCP2ATM() SQL error: %s\n", sqlite3_errmsg (handle));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      rl2PrivAffineTransform matrix;
		      if (!rl2_affine_transform_from_blob
			  (&matrix,
			   (const unsigned char *) sqlite3_column_blob (stmt,
									0),
			   sqlite3_column_bytes (stmt, 0)))
			  goto stop;
		      count++;
		      xx = matrix.xx;
		      yx = matrix.yx;
		      xy = matrix.xy;
		      yy = matrix.yy;
		      xoff = matrix.xoff;
		      yoff = matrix.yoff;
		  }
	    }
	  else
	    {
		printf ("SELECT GCP2ATM SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto stop;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

  stop:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (count == 1)
      {
	  aux->atm_xx = xx;
	  aux->atm_yx = yx;
	  aux->atm_xy = xy;
	  aux->atm_yy = yy;
	  aux->atm_xoff = xoff;
	  aux->atm_yoff = yoff;
	  retcode = 1;
      }
    return retcode;
}

static int
do_get_raw_raster_data (struct aux_renderer *aux, int by_section)
{
/* attempting to load raw raster data */
    int was_monochrome;
    unsigned char out_pixel = aux->out_pixel;
    rl2PixelPtr no_data = aux->no_data;
    sqlite3 *sqlite = aux->sqlite;
    int max_threads = aux->max_threads;
    int base_width = aux->base_width;
    int base_height = aux->base_height;
    int reproject_on_the_fly = aux->reproject_on_the_fly;
    double minx = aux->minx;
    double maxx = aux->maxx;
    double miny = aux->miny;
    double maxy = aux->maxy;
    double xx_res = aux->xx_res;
    double yy_res = aux->yy_res;
    rl2CoveragePtr coverage = aux->coverage;
    rl2RasterSymbolizerPtr symbolizer = aux->symbolizer;
    rl2RasterStatisticsPtr stats = aux->stats;
    rl2PalettePtr palette = aux->palette;
    rl2CoverageStylePtr cvg_stl = aux->cvg_stl;
    int level_id = aux->level_id;
    int scale = aux->scale;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *mask = NULL;
    int mask_size;

    if (reproject_on_the_fly)
      {
	  /* we must always extract raster data using the Coverage's own SRID */
	  minx = aux->native_minx;
	  miny = aux->native_miny;
	  maxx = aux->native_maxx;
	  maxy = aux->native_maxy;
	  xx_res = aux->xx_res;
	  yy_res = aux->yy_res;
	  base_width = aux->base_width;
	  base_height = aux->base_height;
	  level_id = aux->level_id;
	  scale = aux->scale;
      }

    if (by_section)
      {
	  /* Mixed Resolutions Coverage */
	  was_monochrome = 0;
	  if (out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		if (rl2_has_styled_rgb_colors (symbolizer))
		    out_pixel = RL2_PIXEL_RGB;
		else
		    out_pixel = RL2_PIXEL_GRAYSCALE;
		was_monochrome = 1;
	    }
	  if (out_pixel == RL2_PIXEL_PALETTE)
	      out_pixel = RL2_PIXEL_RGB;
	  if (rl2_get_raw_raster_data_mixed_resolutions_transparent
	      (sqlite, max_threads, coverage, base_width, base_height,
	       minx, miny, maxx, maxy, xx_res, yy_res,
	       &outbuf, &outbuf_size, &mask, &mask_size, &palette, &out_pixel,
	       no_data, symbolizer, stats) != RL2_OK)
	      goto error;
	  if (was_monochrome && out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		rl2_destroy_coverage_style (cvg_stl);
		aux->cvg_stl = NULL;
	    }
      }
    else
      {
	  /* ordinary Coverage */
	  was_monochrome = 0;
	  if (out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		if (level_id != 0 && scale != 1)
		  {
		      if (rl2_has_styled_rgb_colors (symbolizer))
			  out_pixel = RL2_PIXEL_RGB;
		      else
			  out_pixel = RL2_PIXEL_GRAYSCALE;
		      was_monochrome = 1;
		  }
	    }
	  if (out_pixel == RL2_PIXEL_PALETTE)
	    {
		if (level_id != 0 && scale != 1)
		    out_pixel = RL2_PIXEL_RGB;
	    }
	  if (rl2_get_raw_raster_data_transparent
	      (sqlite, max_threads, coverage, base_width, base_height,
	       minx, miny, maxx, maxy, xx_res, yy_res,
	       &outbuf, &outbuf_size, &mask, &mask_size, &palette, &out_pixel,
	       no_data, symbolizer, stats) != RL2_OK)
	      goto error;
	  if (was_monochrome
	      && (out_pixel == RL2_PIXEL_GRAYSCALE
		  || out_pixel == RL2_PIXEL_RGB))
	    {
		rl2_destroy_coverage_style (cvg_stl);
		aux->cvg_stl = NULL;
	    }
      }

/* completing the aux struct for passing rendering arguments */
    aux->outbuf = outbuf;
    aux->mask = mask;
    aux->palette = palette;
    aux->out_pixel = out_pixel;
    return 1;

  error:
    return 0;
}

static int
do_paint_map_from_raster (struct aux_raster_render *args)
{
/* 
* rendering a Map Image from a Raster Coverage 
* common implementation
*/
    sqlite3 *sqlite;
    const void *data;
    rl2CanvasPtr canvas;
    const char *db_prefix;
    const char *cvg_name;
    const unsigned char *blob;
    int blob_sz;
    int width;
    int height;
    const char *style;
    const char *format;
    int transparent;
    int quality;
    rl2CoveragePtr coverage = NULL;
    rl2PrivCoveragePtr cvg;
    int out_srid;
    int base_width;
    int base_height;
    unsigned char bg_red = 255;
    unsigned char bg_green = 255;
    unsigned char bg_blue = 255;
    int max_threads = 1;
    double minx;
    double maxx;
    double miny;
    double maxy;
    double ext_x;
    double ext_y;
    double x_res;
    double y_res;
    int srid;
    int level_id;
    int scale;
    int xscale;
    double map_scale;
    double xx_res;
    double yy_res;
    int ok_style;
    int ok_format;
    unsigned char *image = NULL;
    int image_size;
    rl2PalettePtr palette = NULL;
    unsigned char format_id = RL2_OUTPUT_FORMAT_UNKNOWN;
    unsigned char out_pixel = RL2_PIXEL_UNKNOWN;
    rl2CoverageStylePtr cvg_stl = NULL;
    rl2RasterSymbolizerPtr symbolizer = NULL;
    rl2RasterStatisticsPtr stats = NULL;
    int aux_symbolizer = 0;
    double opacity = 1.0;
    struct aux_renderer aux;
    int by_section = 0;
    int reproject_on_the_fly = 0;
    rl2PixelPtr no_data;

    sqlite = args->sqlite;
    data = args->data;
    canvas = args->canvas;
    db_prefix = args->db_prefix;
    cvg_name = args->cvg_name;
    blob = args->blob;
    blob_sz = args->blob_sz;
    width = args->width;
    height = args->height;
    style = args->style;
    if (args->output != NULL)
      {
	  bg_red = args->output->bg_red;
	  bg_green = args->output->bg_green;
	  bg_blue = args->output->bg_blue;
	  transparent = args->output->transparent;
	  format = args->output->format;
	  quality = args->output->quality;
      }
    else
	transparent = 1;

/* coarse args validation */
    if (sqlite == NULL)
	goto error;
    if (canvas == NULL)
	goto error;
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    if (width < 64)
	goto error;
    if (height < 64)
	goto error;
/* validating the format */
    if (args->output == NULL)
	ok_format = 1;
    else
      {
	  ok_format = 0;
	  if (strcmp (format, "image/png") == 0)
	    {
		format_id = RL2_OUTPUT_FORMAT_PNG;
		ok_format = 1;
	    }
	  if (strcmp (format, "image/jpeg") == 0)
	    {
		format_id = RL2_OUTPUT_FORMAT_JPEG;
		ok_format = 1;
	    }
	  if (strcmp (format, "image/tiff") == 0)
	    {
		format_id = RL2_OUTPUT_FORMAT_TIFF;
		ok_format = 1;
	    }
	  if (strcmp (format, "application/x-pdf") == 0)
	    {
		format_id = RL2_OUTPUT_FORMAT_PDF;
		ok_format = 1;
	    }
      }
    if (!ok_format)
	goto error;

/* checking the Geometry */
    if (rl2_parse_bbox_srid
	(sqlite, blob, blob_sz, &out_srid, &minx, &miny, &maxx,
	 &maxy) != RL2_OK)
	goto error;

    ext_x = maxx - minx;
    ext_y = maxy - miny;
    if (ext_x <= 0.0 || ext_y <= 0.0)
	goto error;
    if (rl2_test_layer_group (sqlite, db_prefix, cvg_name))
      {
	  /* switching the whole task to the Group renderer */
	  struct aux_group_renderer aux;
	  fprintf (stderr, "------------ GROUP\n");
	  aux.sqlite = sqlite;
	  aux.data = data;
	  aux.db_prefix = db_prefix;
	  aux.group_name = cvg_name;
	  aux.minx = minx;
	  aux.maxx = maxx;
	  aux.miny = miny;
	  aux.maxy = maxy;
	  aux.width = width;
	  aux.height = height;
	  aux.style = style;
	  aux.format_id = format_id;
	  aux.bg_red = bg_red;
	  aux.bg_green = bg_green;
	  aux.bg_blue = bg_blue;
	  aux.transparent = transparent;
	  aux.quality = quality;
	  if (rl2_aux_group_renderer (&aux, &image, &image_size) == RL2_OK)
	    {
		args->output->img = image;
		args->output->img_size = image_size;
		return RL2_OK;
	    }
	  else
	    {
		args->output->img = NULL;
		args->output->img_size = 0;
		return RL2_ERROR;
	    }
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
	goto error;
    if (rl2_get_coverage_srid (coverage, &srid) != RL2_OK)
	srid = -1;
    if (out_srid > 0 && srid > 0)
      {
	  if (out_srid != srid)
	      reproject_on_the_fly = 1;
      }
    no_data = rl2_get_coverage_no_data (coverage);

    if (reproject_on_the_fly)
      {
	  /* tranforming the BBOX into the Coverage's Native SRID */
	  aux.sqlite = sqlite;
	  aux.data = data;
	  aux.minx = minx;
	  aux.miny = miny;
	  aux.maxx = maxx;
	  aux.maxy = maxy;
	  aux.srid = srid;
	  aux.out_srid = out_srid;
	  if (!do_transform_raster_bbox (&aux))
	      goto error;
	  /* expanding the Native BBOX */
	  ext_x = aux.native_maxx - aux.native_minx;
	  ext_y = aux.native_maxy - aux.native_miny;
	  x_res = ext_x / (double) width;
	  y_res = ext_y / (double) height;
	  aux.native_minx =
	      aux.native_cx -
	      ((((double) width + ((double) width / 5.0)) / 2.0) * x_res);
	  aux.native_maxx =
	      aux.native_cx +
	      ((((double) width + ((double) width / 5.0)) / 2.0) * x_res);
	  aux.native_miny =
	      aux.native_cy -
	      ((((double) height + ((double) height / 5.0)) / 2.0) * y_res);
	  aux.native_maxy =
	      aux.native_cy +
	      ((((double) height + ((double) height / 5.0)) / 2.0) * y_res);
	  ext_x = aux.native_maxx - aux.native_minx;
	  ext_y = aux.native_maxy - aux.native_miny;
	  aux.base_width = ext_x / x_res;
	  aux.base_height = ext_y / y_res;
      }

    if (reproject_on_the_fly)
      {
	  ext_x = aux.native_maxx - aux.native_minx;
	  ext_y = aux.native_maxy - aux.native_miny;
	  x_res = ext_x / (double) (aux.base_width);
	  y_res = ext_y / (double) (aux.base_height);
      }
    else
      {
	  ext_x = maxx - minx;
	  ext_y = maxy - miny;
	  x_res = ext_x / (double) width;
	  y_res = ext_y / (double) height;
      }
    map_scale = standard_scale (sqlite, out_srid, width, height, ext_x, ext_y);

/* validating the style */
    ok_style = 0;
    if (args->xml_style != NULL)
      {
	  /* attempting to apply a QuickStyle */
	  char *style2 = malloc (strlen (style) + 1);
	  strcpy (style2, style);
	  cvg_stl =
	      coverage_style_from_xml (style2,
				       (unsigned char *) (args->xml_style));
	  if (cvg_stl == NULL)
	      goto error;
	  symbolizer =
	      rl2_get_symbolizer_from_coverage_style (cvg_stl, map_scale);
	  if (symbolizer == NULL)
	    {
		/* invisible at the currect scale */
		if (!rl2_aux_default_image
		    (width, height, bg_red, bg_green, bg_blue, format_id,
		     transparent, quality, &image, &image_size))
		    goto error;
		goto done;
	    }
	  stats =
	      rl2_create_raster_statistics_from_dbms (sqlite, db_prefix,
						      cvg_name);
	  if (stats == NULL)
	      goto error;
	  ok_style = 1;
      }
    else
      {
	  if (style == NULL)
	      style = "default";
	  if (strcasecmp (style, "default") == 0)
	      ok_style = 1;
	  else
	    {
		/* attempting to get a Coverage Style */
		cvg_stl =
		    rl2_create_coverage_style_from_dbms (sqlite, db_prefix,
							 cvg_name, style);
		if (cvg_stl == NULL)
		    goto error;
		symbolizer =
		    rl2_get_symbolizer_from_coverage_style (cvg_stl, map_scale);
		if (symbolizer == NULL)
		  {
		      /* invisible at the currect scale */
		      if (!rl2_aux_default_image
			  (width, height, bg_red, bg_green, bg_blue, format_id,
			   transparent, quality, &image, &image_size))
			  goto error;
		      goto done;
		  }
		stats =
		    rl2_create_raster_statistics_from_dbms (sqlite, db_prefix,
							    cvg_name);
		if (stats == NULL)
		    goto error;
		ok_style = 1;
	    }
      }
    if (!ok_style)
	goto error;

    cvg = (rl2PrivCoveragePtr) coverage;
    out_pixel = RL2_PIXEL_UNKNOWN;
    if (cvg->sampleType == RL2_SAMPLE_UINT8 && cvg->pixelType == RL2_PIXEL_RGB
	&& cvg->nBands == 3)
	out_pixel = RL2_PIXEL_RGB;
    if (cvg->sampleType == RL2_SAMPLE_UINT8
	&& cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	out_pixel = RL2_PIXEL_GRAYSCALE;
    if (cvg->pixelType == RL2_PIXEL_PALETTE && cvg->nBands == 1)
	out_pixel = RL2_PIXEL_PALETTE;
    if (cvg->pixelType == RL2_PIXEL_MONOCHROME && cvg->nBands == 1)
	out_pixel = RL2_PIXEL_MONOCHROME;

    if (cvg->pixelType == RL2_PIXEL_MULTIBAND && cvg_stl == NULL)
      {
	  /* attempting to create a default RGB Coverage Style */
	  unsigned char red_band;
	  unsigned char green_band;
	  unsigned char blue_band;
	  unsigned char nir_band;
	  if (rl2_get_dbms_coverage_default_bands
	      (sqlite, db_prefix, cvg_name, &red_band, &green_band,
	       &blue_band, &nir_band) == RL2_OK)
	    {
		/* ok, using the declared default bands */
		rl2PrivCoverageStylePtr stl =
		    rl2_create_default_coverage_style ();
		rl2PrivStyleRulePtr rule = rl2_create_default_style_rule ();
		rl2PrivRasterSymbolizerPtr symb =
		    rl2_create_default_raster_symbolizer ();
		aux_symbolizer = 1;
		symb->bandSelection = malloc (sizeof (rl2PrivBandSelection));
		symb->bandSelection->selectionType = RL2_BAND_SELECTION_TRIPLE;
		symb->bandSelection->redBand = red_band;
		symb->bandSelection->greenBand = green_band;
		symb->bandSelection->blueBand = blue_band;
		symb->bandSelection->redContrast =
		    RL2_CONTRAST_ENHANCEMENT_NONE;
		symb->bandSelection->greenContrast =
		    RL2_CONTRAST_ENHANCEMENT_NONE;
		symb->bandSelection->blueContrast =
		    RL2_CONTRAST_ENHANCEMENT_NONE;
		symb->bandSelection->grayContrast =
		    RL2_CONTRAST_ENHANCEMENT_NONE;
		rule->style_type = RL2_RASTER_STYLE;
		rule->style = symbolizer;
		stl->first_rule = rule;
		stl->last_rule = rule;
		cvg_stl = (rl2CoverageStylePtr) stl;
		symbolizer = (rl2RasterSymbolizerPtr) symb;
		if (stats == NULL)
		  {
		      stats =
			  rl2_create_raster_statistics_from_dbms (sqlite,
								  db_prefix,
								  cvg_name);
		      if (stats == NULL)
			  goto error;
		  }
	    }
      }

    if ((cvg->pixelType == RL2_PIXEL_DATAGRID
	 || cvg->pixelType == RL2_PIXEL_MULTIBAND) && cvg_stl == NULL)
      {
	  /* creating a default Coverage Style */
	  rl2PrivCoverageStylePtr stl = rl2_create_default_coverage_style ();
	  rl2PrivStyleRulePtr rule = rl2_create_default_style_rule ();
	  rl2PrivRasterSymbolizerPtr symb =
	      rl2_create_default_raster_symbolizer ();
	  aux_symbolizer = 1;
	  symb->bandSelection = malloc (sizeof (rl2PrivBandSelection));
	  symb->bandSelection->selectionType = RL2_BAND_SELECTION_MONO;
	  symb->bandSelection->grayBand = 0;
	  symb->bandSelection->grayContrast = RL2_CONTRAST_ENHANCEMENT_NONE;
	  rule->style_type = RL2_RASTER_STYLE;
	  rule->style = symbolizer;
	  stl->first_rule = rule;
	  stl->last_rule = rule;
	  cvg_stl = (rl2CoverageStylePtr) stl;
	  symbolizer = (rl2RasterSymbolizerPtr) symb;
	  if (stats == NULL)
	    {
		stats =
		    rl2_create_raster_statistics_from_dbms (sqlite,
							    db_prefix,
							    cvg_name);
		if (stats == NULL)
		    goto error;
	    }
      }

    if (symbolizer != NULL)
      {
	  /* applying a RasterSymbolizer */
	  int yes_no;
	  int categorize;
	  int interpolate;
	  if (rl2_is_raster_symbolizer_triple_band_selected
	      (symbolizer, &yes_no) == RL2_OK)
	    {
		if ((cvg->sampleType == RL2_SAMPLE_UINT8
		     || cvg->sampleType == RL2_SAMPLE_UINT16)
		    && (cvg->pixelType == RL2_PIXEL_RGB
			|| cvg->pixelType == RL2_PIXEL_MULTIBAND) && yes_no)
		    out_pixel = RL2_PIXEL_RGB;
	    }
	  if (rl2_is_raster_symbolizer_mono_band_selected
	      (symbolizer, &yes_no, &categorize, &interpolate) == RL2_OK)
	    {
		if ((cvg->sampleType == RL2_SAMPLE_UINT8
		     || cvg->sampleType == RL2_SAMPLE_UINT16)
		    && (cvg->pixelType == RL2_PIXEL_RGB
			|| cvg->pixelType == RL2_PIXEL_MULTIBAND
			|| cvg->pixelType == RL2_PIXEL_GRAYSCALE) && yes_no)
		    out_pixel = RL2_PIXEL_GRAYSCALE;
		if ((cvg->sampleType == RL2_SAMPLE_UINT8
		     || cvg->sampleType == RL2_SAMPLE_UINT16)
		    && cvg->pixelType == RL2_PIXEL_MULTIBAND && yes_no
		    && (categorize || interpolate))
		    out_pixel = RL2_PIXEL_RGB;
		if ((cvg->sampleType == RL2_SAMPLE_INT8
		     || cvg->sampleType == RL2_SAMPLE_UINT8
		     || cvg->sampleType == RL2_SAMPLE_INT16
		     || cvg->sampleType == RL2_SAMPLE_UINT16
		     || cvg->sampleType == RL2_SAMPLE_INT32
		     || cvg->sampleType == RL2_SAMPLE_UINT32
		     || cvg->sampleType == RL2_SAMPLE_FLOAT
		     || cvg->sampleType == RL2_SAMPLE_DOUBLE)
		    && cvg->pixelType == RL2_PIXEL_DATAGRID && yes_no)
		    out_pixel = RL2_PIXEL_GRAYSCALE;
	    }
	  if (rl2_get_raster_symbolizer_opacity (symbolizer, &opacity) !=
	      RL2_OK)
	      opacity = 1.0;
	  if (opacity > 1.0)
	      opacity = 1.0;
	  if (opacity < 0.0)
	      opacity = 0.0;
	  if (opacity < 1.0)
	      transparent = 1;
      }
    if (out_pixel == RL2_PIXEL_UNKNOWN)
      {
	  fprintf (stderr, "*** Unsupported Pixel !!!!\n");
	  goto error;
      }

    if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) > 0)
      {
	  /* Mixed Resolutions Coverage */
	  by_section = 1;
	  xx_res = x_res;
	  yy_res = y_res;
      }
    else
      {
	  /* ordinary Coverage */
	  by_section = 0;
	  /* retrieving the optimal resolution level */
	  if (!rl2_find_best_resolution_level
	      (sqlite, db_prefix, cvg_name, 0, 0, x_res, y_res, &level_id,
	       &scale, &xscale, &xx_res, &yy_res))
	      goto error;
      }

    base_width = (int) (ext_x / xx_res);
    base_height = (int) (ext_y / yy_res);
    if ((base_width <= 0 || base_width >= USHRT_MAX)
	|| (base_height <= 0 || base_height >= USHRT_MAX))
	goto error;

    if ((base_width * base_height) > (8192 * 8192))
      {
	  /* warning: this usually implies missing Pyramid support */
	  fprintf (stderr,
		   "ERROR: a really huge image (%u x %u) has been requested;\n"
		   "this is usually caused by a missing multi-resolution Pyramid.\n"
		   "Please build a Pyramid supporting '%s'\n\n", base_width,
		   base_height, cvg_name);
	  goto error;
      }

    if (reproject_on_the_fly)
      {
	  /* computing the Affine Transform Matrix */
	  aux.width = width;
	  aux.height = height;
	  aux.base_width = base_width;
	  aux.base_height = base_height;
	  aux.xx_res = xx_res;
	  aux.yy_res = yy_res;
	  if (!do_compute_atm (&aux))
	      goto error;
      }

/* preparing the aux struct for passing rendering arguments */
    aux.sqlite = sqlite;
    aux.max_threads = max_threads;
    aux.width = width;
    aux.height = height;
    aux.base_width = base_width;
    aux.base_height = base_height;
    aux.minx = minx;
    aux.miny = miny;
    aux.maxx = maxx;
    aux.maxy = maxy;
    aux.srid = srid;
    aux.out_srid = out_srid;
    aux.reproject_on_the_fly = reproject_on_the_fly;
    if (by_section)
	aux.by_section = 1;
    else
	aux.by_section = 0;
    aux.x_res = x_res;
    aux.y_res = y_res;
    aux.xx_res = xx_res;
    aux.yy_res = yy_res;
    aux.transparent = transparent;
    aux.opacity = opacity;
    aux.quality = quality;
    aux.format_id = format_id;
    aux.bg_red = bg_red;
    aux.bg_green = bg_green;
    aux.bg_blue = bg_blue;
    aux.coverage = coverage;
    aux.symbolizer = symbolizer;
    aux.stats = stats;
    aux.cvg_stl = cvg_stl;
    aux.level_id = level_id;
    aux.scale = scale;
    aux.outbuf = NULL;
    aux.mask = NULL;
    aux.palette = NULL;
    aux.no_data = no_data;
    aux.out_pixel = out_pixel;
    if (args->output != NULL)
	aux.is_blob_image = 1;
    else
	aux.is_blob_image = 0;
    aux.image = NULL;
    aux.image_size = 0;
    aux.graphics_ctx = rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);

    if (!do_get_raw_raster_data (&aux, by_section))
      {
	  cvg_stl = aux.cvg_stl;
	  goto error;
      }
    cvg_stl = aux.cvg_stl;
    if (rl2_aux_render_image (&aux) != RL2_OK)
	goto error;

  done:
    if (args->output != NULL)
      {
	  args->output->img = aux.image;
	  args->output->img_size = aux.image_size;
      }
    rl2_destroy_coverage (coverage);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (cvg_stl != NULL)
	rl2_destroy_coverage_style (cvg_stl);
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    if (aux_symbolizer && symbolizer)
	rl2_destroy_raster_symbolizer ((rl2PrivRasterSymbolizerPtr) symbolizer);
    do_set_canvas_ready (canvas, RL2_CANVAS_BASE_CTX);
    return RL2_OK;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (cvg_stl != NULL)
	rl2_destroy_coverage_style (cvg_stl);
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    if (aux_symbolizer && symbolizer)
	rl2_destroy_raster_symbolizer ((rl2PrivRasterSymbolizerPtr) symbolizer);
    if (args->output != NULL)
      {
	  args->output->img = NULL;
	  args->output->img_size = 0;
      }
    return RL2_ERROR;
}

static double
do_compute_bbox_aspect_ratio (sqlite3 * sqlite, const unsigned char *blob,
			      int blob_sz)
{
/* computing the BBOX aspect ratio */
    int out_srid;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double width;
    double height;
    double ratio;

/* checking the Geometry */
    if (rl2_parse_bbox_srid
	(sqlite, blob, blob_sz, &out_srid, &minx, &miny, &maxx,
	 &maxy) != RL2_OK)
	return -1.0;
/* computing width and height */
    width = maxx - minx;
    height = maxy - miny;
/* computing the aspect ratio */
    ratio = width / height;
    return ratio;
}

RL2_DECLARE unsigned char *
rl2_map_image_from_wms (sqlite3 * sqlite,
			const char *db_prefix, const char *cvg_name,
			const unsigned char *blob, int blob_sz,
			int width, int height, const char *version,
			const char *style, const char *format, int transparent,
			const char *bg_color, int *image_size)
{
/* returning a Map Image from a WMS Coverage - BLOB */
    int opaque;
    int srid;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    int ret;
    char *xdb_prefix;
    int ok = 0;
    char *url = NULL;
    char *srs = NULL;
    int flip_axes;
    int valid_bg_color = 1;
    char *xbg_color;
    int len;
    unsigned char *image;

/* checking the Geometry */
    if (rl2_parse_bbox_srid
	(sqlite, blob, blob_sz, &srid, &minx, &miny, &maxx, &maxy) != RL2_OK)
	return NULL;

/* querying the WMS Coverage */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    sql = sqlite3_mprintf ("SELECT w.url, s.has_flipped_axes "
			   "FROM \"%s\".wms_getmap AS w, \"%s\".spatial_ref_sys_aux AS s "
			   "WHERE w.layer_name = %Q AND s.srid = %d",
			   xdb_prefix, xdb_prefix, cvg_name, srid);
    free (xdb_prefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return NULL;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *val = results[(i * columns) + 0];
		if (url != NULL)
		    free (url);
		len = strlen (val);
		url = malloc (len + 1);
		strcpy (url, val);
		val = results[(i * columns) + 1];
		flip_axes = atoi (val);
		ok = 1;
	    }
      }
    sqlite3_free_table (results);
    if (!ok)
	return NULL;

    srs = sqlite3_mprintf ("EPSG:%d", srid);
    if (version == NULL)
	flip_axes = 0;
    else
      {
	  if (strcmp (version, "1.3.0") != 0)
	      flip_axes = 0;
      }

    if (transparent)
	opaque = 0;
    else
	opaque = 1;

/* validating the background color */
    if (strlen (bg_color) != 7)
	valid_bg_color = 0;
    else
      {
	  int i;
	  if (*(bg_color + 0) != '#')
	      valid_bg_color = 0;
	  for (i = 1; i < 7; i++)
	    {
		char h = *(bg_color + i);
		if ((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f')
		    || (h >= 'A' && h <= 'F'))
		    ;
		else
		    valid_bg_color = 0;
	    }
      }
    if (valid_bg_color)
	xbg_color = sqlite3_mprintf ("0x%s", bg_color + 1);
    else
	xbg_color = sqlite3_mprintf ("0xFFFFFF");
    image =
	do_wms_GetMap_blob (url, version, cvg_name,
			    flip_axes, srs, minx, miny, maxx, maxy, width,
			    height, style, format, opaque, xbg_color,
			    image_size);
    sqlite3_free (xbg_color);
    sqlite3_free (srs);
    return image;
}

RL2_DECLARE int
rl2_map_image_blob_from_raster (sqlite3 * sqlite, const void *data,
				const char *db_prefix, const char *cvg_name,
				const unsigned char *blob, int blob_sz,
				int width, int height, const char *style,
				const char *format, const char *bg_color,
				int transparent, int quality, int reaspect,
				unsigned char **img, int *img_size)
{
/* rendering a Map Image from a Raster Coverage - BLOB */
    unsigned char bg_red = 0;
    unsigned char bg_green = 0;
    unsigned char bg_blue = 0;
    unsigned char bg_alpha = 0;
    rl2GraphicsContextPtr ctx = NULL;
    struct aux_raster_render aux;
    aux.sqlite = sqlite;
    aux.data = data;
    aux.canvas = NULL;
    aux.db_prefix = db_prefix;
    aux.cvg_name = cvg_name;
    aux.blob = blob;
    aux.blob_sz = blob_sz;
    aux.width = width;
    aux.height = height;
    aux.style = style;
    aux.out_pixel = RL2_PIXEL_UNKNOWN;
    aux.output = malloc (sizeof (struct aux_raster_image));
    aux.output->bg_red = 255;
    aux.output->bg_green = 255;
    aux.output->bg_blue = 255;
    aux.output->transparent = transparent;
    aux.output->format = format;
    aux.output->quality = quality;
    aux.output->img = NULL;
    aux.output->img_size = 0;
    aux.xml_style = NULL;

    if (reaspect == 0)
      {
	  /* testing for consistent aspect ratios */
	  double aspect_org;
	  double aspect_dst;
	  double confidence;
	  aspect_org = do_compute_bbox_aspect_ratio (sqlite, blob, blob_sz);
	  if (aspect_org < 0.0)
	      goto error;
	  aspect_dst = (double) width / (double) height;
	  confidence = aspect_org / 100.0;
	  if (aspect_dst >= (aspect_org - confidence)
	      && aspect_dst <= (aspect_org + confidence))
	      ;
	  else
	      goto error;
      }

/* creating a Canvas */
    ctx = rl2_graph_create_context (width, height);
    if (ctx == NULL)
	goto error;
    aux.canvas = rl2_create_raster_canvas (ctx);
    if (aux.canvas == NULL)
	goto error;
    if (!transparent)
      {
	  /* parsing the background color */
	  bg_alpha = 255;
	  if (rl2_parse_hexrgb (bg_color, &bg_red, &bg_green, &bg_blue) !=
	      RL2_OK)
	    {
		bg_red = 255;
		bg_green = 255;
		bg_blue = 255;
	    }
      }
    aux.output->bg_red = bg_red;
    aux.output->bg_green = bg_green;
    aux.output->bg_blue = bg_blue;

/* priming the background */
    rl2_prime_background (ctx, bg_red, bg_green, bg_blue, bg_alpha);

    if (do_paint_map_from_raster (&aux) == RL2_OK)
      {
	  *img = aux.output->img;
	  *img_size = aux.output->img_size;
	  free (aux.output);
	  rl2_destroy_canvas (aux.canvas);
	  rl2_graph_destroy_context (ctx);
	  return RL2_OK;
      }

  error:
    if (aux.output != NULL)
	free (aux.output);
    *img = NULL;
    *img_size = 0;
    if (aux.canvas != NULL)
	rl2_destroy_canvas (aux.canvas);
    if (ctx != NULL)
	rl2_graph_destroy_context (ctx);
    return RL2_ERROR;
}

static int
do_transform_point (sqlite3 * handle, const unsigned char *blob, int blob_sz,
		    int srid, double *x, double *y)
{
/* transforming a Point into another SRID */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    const unsigned char *x_blob;
    int x_blob_sz;
    int count = 0;
    double pt_x;
    double pt_y;
    int xsrid;

    sql = "SELECT ST_Transform(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT pixel-reproject SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, srid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      x_blob = sqlite3_column_blob (stmt, 0);
		      x_blob_sz = sqlite3_column_bytes (stmt, 0);
		      count++;
		      if (rl2_parse_point
			  (handle, x_blob, x_blob_sz, &pt_x, &pt_y,
			   &xsrid) != RL2_OK)
			  goto error;
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT pixel-reproject; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    *x = pt_x;
    *y = pt_y;
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static void
do_update_pixel8 (rl2PrivRasterPtr rst, int x, int y, rl2PrivPixelPtr pxl,
		  int is_signed)
{
/* updating the Pixel's values - (U)INT8 */
    int b;
    rl2PrivSamplePtr out = pxl->Samples;
    char *in = (char *) (rst->rasterBuffer);
    in += (rst->width * y * rst->nBands) + (rst->nBands + x);
    for (b = 0; b < rst->nBands; b++)
      {
	  if (is_signed)
	      out->int8 = *in++;
	  else
	      out->uint8 = *((unsigned char *) in);
	  in++;
	  out++;
      }
}

static void
do_update_pixel16 (rl2PrivRasterPtr rst, int x, int y, rl2PrivPixelPtr pxl,
		   int is_signed)
{
/* updating the Pixel's values - (U)INT16 */
    int b;
    rl2PrivSamplePtr out = pxl->Samples;
    short *in = (short *) (rst->rasterBuffer);
    in += (rst->width * y * rst->nBands) + (rst->nBands + x);
    for (b = 0; b < rst->nBands; b++)
      {
	  if (is_signed)
	      out->int16 = *in++;
	  else
	      out->uint16 = *((unsigned short *) in);
	  in++;
	  out++;
      }
}

static void
do_update_pixel32 (rl2PrivRasterPtr rst, int x, int y, rl2PrivPixelPtr pxl,
		   int is_signed)
{
/* updating the Pixel's values - (U)INT32 */
    int b;
    rl2PrivSamplePtr out = pxl->Samples;
    int *in = (int *) (rst->rasterBuffer);
    in += (rst->width * y * rst->nBands) + (rst->nBands + x);
    for (b = 0; b < rst->nBands; b++)
      {
	  if (is_signed)
	      out->int32 = *in++;
	  else
	      out->uint32 = *((unsigned int *) in);
	  in++;
	  out++;
      }
}

static void
do_update_pixelflt (rl2PrivRasterPtr rst, int x, int y, rl2PrivPixelPtr pxl)
{
/* updating the Pixel's values - FLOAT */
    int b;
    rl2PrivSamplePtr out = pxl->Samples;
    float *in = (float *) (rst->rasterBuffer);
    in += (rst->width * y * rst->nBands) + (rst->nBands + x);
    for (b = 0; b < rst->nBands; b++)
      {
	  out->float32 = *in++;
	  out++;
      }
}

static void
do_update_pixeldbl (rl2PrivRasterPtr rst, int x, int y, rl2PrivPixelPtr pxl)
{
/* updating the Pixel's values - DOUBLE */
    int b;
    rl2PrivSamplePtr out = pxl->Samples;
    double *in = (double *) (rst->rasterBuffer);
    in += (rst->width * y * rst->nBands) + (rst->nBands + x);
    for (b = 0; b < rst->nBands; b++)
      {
	  out->float64 = *in++;
	  out++;
      }
}

static void
do_update_pixel (rl2PrivRasterPtr rst, int x, int y, rl2PrivPixelPtr pxl)
{
/* updating the Pixel's values */
    if (rst->maskBuffer != NULL)
      {
	  unsigned char *mask = rst->maskBuffer + (rst->width * y) + x;
	  if (mask == 0)
	    {
		/* transparent Pixel */
		return;
	    }
      }
    switch (rst->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  do_update_pixel8 (rst, x, y, pxl, 0);
	  break;
      case RL2_SAMPLE_INT8:
	  do_update_pixel8 (rst, x, y, pxl, 1);
	  break;
      case RL2_SAMPLE_INT16:
	  do_update_pixel16 (rst, x, y, pxl, 1);
	  break;
      case RL2_SAMPLE_UINT16:
	  do_update_pixel16 (rst, x, y, pxl, 0);
	  break;
      case RL2_SAMPLE_INT32:
	  do_update_pixel32 (rst, x, y, pxl, 1);
	  break;
      case RL2_SAMPLE_UINT32:
	  do_update_pixel32 (rst, x, y, pxl, 0);
	  break;
      case RL2_SAMPLE_FLOAT:
	  do_update_pixelflt (rst, x, y, pxl);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  do_update_pixeldbl (rst, x, y, pxl);
	  break;
      };
}

RL2_DECLARE int
rl2_pixel_from_raster_by_point (sqlite3 * sqlite, const void *data,
				const char *db_prefix, const char *cvg_name,
				int pyramid_level, const unsigned char *blob,
				int blob_sz, rl2PixelPtr * pixel)
{
/* retrieving a Pixel from a Raster Coverage corresponding to a Point Geometry */
    double x;
    double y;
    int srid_pt;
    int srid_cvg;
    rl2CoveragePtr coverage = NULL;
    rl2PalettePtr palette = NULL;
    rl2RasterPtr raster = NULL;
    rl2PixelPtr xpixel = NULL;

    *pixel = NULL;

/* validating the Point Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &x, &y, &srid_pt) != RL2_OK)
	goto error;

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
	goto error;
    if (rl2_get_coverage_srid (coverage, &srid_cvg) != RL2_OK)
	goto error;

/* retrieving the Coverage's Palette */
    palette = rl2_get_dbms_palette (sqlite, db_prefix, cvg_name);

/* retrieving the NO-DATA pixel */
    xpixel = rl2_clone_pixel (rl2_get_coverage_no_data (coverage));
    rl2_destroy_coverage (coverage);
    coverage = NULL;
    if (xpixel == NULL)
	goto error;

    if (srid_cvg != srid_pt)
      {
	  /* transforming the Point into the Raster Coverage SRID */
	  if (!do_transform_point (sqlite, blob, blob_sz, srid_cvg, &x, &y))
	      goto error;
      }

    if (rl2_find_cached_raster
	(data, db_prefix, cvg_name, pyramid_level, x, y, &raster) == RL2_OK)
	;
    else if (rl2_load_cached_raster
	     (sqlite, data, db_prefix, cvg_name, pyramid_level, x, y, palette,
	      &raster) != RL2_OK)
	goto error;
    if (raster != NULL)
      {
	  /* extracting the Pixel at coordinates [X,Y] */
	  rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;
	  rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) xpixel;
	  int dx = (int) ((x - rst->minX) / rst->hResolution);
	  int dy = (int) ((rst->maxY - y) / rst->vResolution);
	  if (dx < 0 || dx >= (int) (rst->width))
	      goto error;
	  if (dy < 0 || dy >= (int) (rst->height))
	      goto error;
	  if (rst->sampleType != pxl->sampleType
	      || rst->pixelType != pxl->pixelType || rst->nBands != pxl->nBands)
	      goto error;
	  do_update_pixel (rst, dx, dy, pxl);
      }
    *pixel = xpixel;
    return RL2_OK;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (xpixel != NULL)
	rl2_destroy_pixel (xpixel);
    *pixel = NULL;
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_map_image_paint_from_raster (sqlite3 * sqlite, const void *data,
				 rl2CanvasPtr canvas, const char *db_prefix,
				 const char *cvg_name,
				 const unsigned char *blob, int blob_sz,
				 const char *style, unsigned char *xml_style)
{
/* rendering a Map Image from a Raster Coverage - Graphics Context */
    struct aux_raster_render aux;
    rl2GraphicsContextPtr ctx;
    int width;
    int height;

    if (canvas == NULL)
	return RL2_ERROR;
    ctx = rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
    if (ctx == NULL)
	return RL2_ERROR;
    if (rl2_graph_context_get_dimensions (ctx, &width, &height) != RL2_OK)
	return RL2_ERROR;

    aux.sqlite = sqlite;
    aux.data = data;
    aux.canvas = canvas;
    aux.db_prefix = db_prefix;
    aux.cvg_name = cvg_name;
    aux.blob = blob;
    aux.blob_sz = blob_sz;
    aux.width = width;
    aux.height = height;
    aux.style = style;
    aux.xml_style = xml_style;
    aux.out_pixel = RL2_PIXEL_RGB;
    aux.output = NULL;
    return do_paint_map_from_raster (&aux);
}

static int
do_paint_map_from_vector (struct aux_vector_render *aux)
{
/* 
* rendering a Map Image from a Vector Coverage 
* common implementation
*/
    rl2VectorMultiLayerPtr multi = NULL;
    sqlite3 *sqlite;
    const void *priv_data;
    rl2CanvasPtr canvas;
    const char *db_prefix;
    const char *cvg_name;
    const unsigned char *blob;
    int blob_sz;
    int width;
    int height;
    const char *style;
    const unsigned char *quickStyle;
    const char *format;
    int quality;
    int out_srid;
    sqlite3_stmt *stmt = NULL;
    double minx;
    double maxx;
    double miny;
    double maxy;
    double ext_min_x;
    double ext_max_x;
    double ext_min_y;
    double ext_max_y;
    double extended_x;
    double extended_y;
    double ext_x;
    double ext_y;
    double x_res;
    double y_res;
    double scale;
    int srid;
    int ok_style;
    int ok_format;
    unsigned char *image = NULL;
    int image_size;
    unsigned char format_id = RL2_OUTPUT_FORMAT_UNKNOWN;
    rl2FeatureTypeStylePtr lyr_stl = NULL;
    rl2VectorSymbolizerPtr symbolizer = NULL;
    char *xdb_prefix;
    char *quoted;
    char *sql;
    char *oldsql;
    int columns = 0;
    int i;
    int j;
    int ret;
    int reproject_on_the_fly;
    int has_extra_columns;
    rl2VariantArrayPtr variant = NULL;
    rl2GraphicsContextPtr ctx = NULL;
    rl2GraphicsContextPtr ctx_labels = NULL;
    int has_labels = 0;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    int half_transparent;
    int is_topogeo = 0;
    int is_toponet = 0;
    int which = RL2_CANVAS_UNKNOWN_CTX;

    sqlite = aux->sqlite;
    priv_data = aux->data;
    canvas = aux->canvas;
    db_prefix = aux->db_prefix;
    cvg_name = aux->cvg_name;
    blob = aux->blob;
    blob_sz = aux->blob_sz;
    width = aux->width;
    height = aux->height;
    style = aux->style;
    quickStyle = aux->quick_style;
    if (aux->output == NULL)
      {
	  format = NULL;
	  quality = 100;
      }
    else
      {
	  format = aux->output->format;
	  quality = aux->output->quality;
      }
    if (canvas == NULL)
	goto error;

/* coarse args validation */
    if (sqlite == NULL)
	goto error;
    if (width < 64)
	goto error;
    if (height < 64)
	goto error;
/* validating the format */
    if (aux->output == NULL)
	ok_format = 1;
    else
      {
	  ok_format = 0;
	  if (strcmp (format, "image/png") == 0)
	    {
		format_id = RL2_OUTPUT_FORMAT_PNG;
		ok_format = 1;
	    }
	  if (strcmp (format, "image/jpeg") == 0)
	    {
		format_id = RL2_OUTPUT_FORMAT_JPEG;
		ok_format = 1;
	    }
	  if (strcmp (format, "image/tiff") == 0)
	    {
		format_id = RL2_OUTPUT_FORMAT_TIFF;
		ok_format = 1;
	    }
	  if (strcmp (format, "application/x-pdf") == 0)
	    {
		format_id = RL2_OUTPUT_FORMAT_PDF;
		ok_format = 1;
	    }
      }
    if (!ok_format)
	goto error;
/* checking the Geometry */
    if (rl2_parse_bbox_srid
	(sqlite, blob, blob_sz, &out_srid, &minx, &miny, &maxx,
	 &maxy) != RL2_OK)
	goto error;
/* computing the extend frame */
    extended_x = (maxx - minx) / 20.0;
    ext_min_x = minx - extended_x;
    ext_max_x = maxx + extended_x;
    extended_y = (maxy - miny) / 20.0;
    ext_min_y = miny - extended_y;
    ext_max_y = maxy + extended_y;

/* attempting to load the VectorLayer definitions from the DBMS */
    multi = rl2_create_vector_layer_from_dbms (sqlite, db_prefix, cvg_name);
    if (multi == NULL)
	goto error;

    ext_x = maxx - minx;
    ext_y = maxy - miny;
    if (ext_x <= 0.0 || ext_y <= 0.0)
	goto error;
    x_res = ext_x / (double) width;
    y_res = ext_y / (double) height;
    scale = standard_scale (sqlite, out_srid, width, height, ext_x, ext_y);
/* validating the style */
    ok_style = 0;
    if (style == NULL)
	style = "default";
    if (strcasecmp (style, "default") == 0)
	ok_style = 1;
    else if (quickStyle != NULL)
      {
	  /* attempting to validate the QuickStyle */
	  lyr_stl = rl2_feature_type_style_from_xml (style, quickStyle);
	  if (lyr_stl == NULL)
	      goto error;
	  ok_style = 1;
      }
    else
      {
	  /* attempting to get a FeatureType Style */
	  lyr_stl =
	      rl2_create_feature_type_style_from_dbms (sqlite, db_prefix,
						       cvg_name, style);
	  if (lyr_stl == NULL)
	      goto error;
	  ok_style = 1;
      }
    if (!ok_style)
	goto error;

    if (lyr_stl != NULL)
      {
	  /* testing for style conditional visibility */
	  if (!rl2_is_visible_style (lyr_stl, scale))
	      goto done;
      }
    rl2_is_multilayer_topogeo (multi, &is_topogeo);
    rl2_is_multilayer_toponet (multi, &is_toponet);

    for (j = 0; j < rl2_get_multilayer_count (multi); j++)
      {
	  /* looping on MultiLayer individual Layers */
	  const char *classname = NULL;
	  char *toponame;
	  int len;
	  int is_face = 0;
	  int is_seed = 0;
	  int visible = 0;
	  rl2VectorLayerPtr layer = NULL;
	  rl2PrivVectorLayerPtr lyr;
	  layer = rl2_get_multilayer_item (multi, j);
	  if (layer == NULL)
	      continue;
	  if (rl2_is_vector_visible (layer, &visible) != RL2_OK)
	      visible = 0;
	  if (!visible)
	      continue;

	  if (rl2_get_vector_srid (layer, &srid) != RL2_OK)
	      srid = -1;
	  lyr = (rl2PrivVectorLayerPtr) layer;
	  if (out_srid <= 0)
	      out_srid = srid;
	  if (srid > 0 && out_srid > 0 && out_srid != srid)
	      reproject_on_the_fly = 1;
	  else
	      reproject_on_the_fly = 0;

	  if (is_topogeo)
	    {
		len = strlen (lyr->f_table_name);
		if (len > 5)
		  {
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_face") == 0)
			{
			    /* using the Canvas Faces Graphics Context */
			    ctx =
				rl2_get_canvas_ctx (canvas,
						    RL2_CANVAS_FACES_CTX);
			}
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_edge") == 0)
			{
			    /* using the Canvas Edges Graphics Context */
			    ctx =
				rl2_get_canvas_ctx (canvas,
						    RL2_CANVAS_EDGES_CTX);
			}
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_node") == 0)
			{
			    /* using the Canvas Nodes Graphics Context */
			    ctx =
				rl2_get_canvas_ctx (canvas,
						    RL2_CANVAS_NODES_CTX);
			}
		  }
		if (strlen (lyr->f_table_name) > 6)
		  {
		      if (strcasecmp
			  (lyr->f_table_name + len - 6, "_seeds") == 0)
			{
			    /* using the Canvas FaceSeeds Graphics Context */
			    ctx =
				rl2_get_canvas_ctx (canvas,
						    RL2_CANVAS_FACE_SEEDS_CTX);
			}
		  }
	    }
	  else if (is_toponet)
	    {
		len = strlen (lyr->f_table_name);
		if (len > 5)
		  {
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_link") == 0)
			{
			    /* using the Canvas Links Graphics Context */
			    ctx =
				rl2_get_canvas_ctx (canvas,
						    RL2_CANVAS_LINKS_CTX);
			}
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_node") == 0)
			{
			    /* using the Canvas Nodes Graphics Context */
			    ctx =
				rl2_get_canvas_ctx (canvas,
						    RL2_CANVAS_NODES_CTX);
			}
		  }
		if (len > 6)
		  {
		      if (strcasecmp
			  (lyr->f_table_name + len - 6, "_seeds") == 0)
			{
			    /* using the Canvas LinkSeeds Graphics Context */
			    ctx =
				rl2_get_canvas_ctx (canvas,
						    RL2_CANVAS_LINK_SEEDS_CTX);
			}
		  }
	    }
	  else
	    {
		/* using the Canvas Base Graphics Context */
		ctx = rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
	    }
	  has_labels = rl2_style_has_labels (lyr_stl);
	  if (has_labels)
	    {
		ctx_labels = rl2_get_canvas_ctx (canvas, RL2_CANVAS_LABELS_CTX);
		if (ctx_labels == NULL)
		    goto error;
	    }
	  if (ctx == NULL)
	      goto error;

	  /* composing the SQL query */
	  toponame = sqlite3_mprintf ("%s", lyr->f_table_name);
	  len = strlen (lyr->f_table_name);
	  if (len > 5)
	    {
		if (strcasecmp (lyr->f_table_name + len - 5, "_face") == 0)
		  {
		      is_face = 1;
		      *(toponame + len - 5) = '\0';
		  }
	    }
	  if (is_topogeo && is_face)
	    {
		if (reproject_on_the_fly)
		    sql =
			sqlite3_mprintf
			("SELECT CastToXY(ST_Intersection(ST_Transform(ST_GetFaceGeometry(%Q, face_id), %d), "
			 "BuildMbr(%f, %f, %f, %f)))",
			 toponame, out_srid, ext_min_x, ext_min_y, ext_max_x,
			 ext_max_y);
		else
		    sql =
			sqlite3_mprintf
			("SELECT CastToXY(ST_Intersection(ST_GetFaceGeometry(%Q, face_id), "
			 "BuildMbr(%f, %f, %f, %f)))", toponame, ext_min_x,
			 ext_min_y, ext_max_x, ext_max_y);
	    }
	  else
	    {
		if (lyr->view_geometry != NULL)
		    quoted = rl2_double_quoted_sql (lyr->view_geometry);
		else
		    quoted = rl2_double_quoted_sql (lyr->f_geometry_column);
		if (reproject_on_the_fly)
		    sql =
			sqlite3_mprintf
			("SELECT CastToXY(ST_Intersection(ST_Transform(\"%s\", %d), "
			 "BuildMbr(%f, %f, %f, %f)))", quoted, out_srid,
			 ext_min_x, ext_min_y, ext_max_x, ext_max_y);
		else
		    sql =
			sqlite3_mprintf
			("SELECT CastToXY(ST_Intersection(\"%s\", "
			 "BuildMbr(%f, %f, %f, %f)))", quoted, ext_min_x,
			 ext_min_y, ext_max_x, ext_max_y);
		free (quoted);
	    }
	  sqlite3_free (toponame);
	  has_extra_columns = 0;
	  if (lyr_stl != NULL)
	    {
		/* may be that the Symbolizer requires some extra column */
		columns = rl2_get_feature_type_style_columns_count (lyr_stl);
		for (i = 0; i < columns; i++)
		  {
		      int skip_topo_sublayer = 0;
		      /* adding columns required by Filters and TextSymbolizers */
		      const char *col_name =
			  rl2_get_feature_type_style_column_name (lyr_stl, i);
		      if (is_topogeo && strcasecmp (col_name, "topoclass") == 0)
			{
			    /* TopoGeo: adding the feature class column for Styling */
			    is_face = 0;
			    is_seed = 0;
			    classname = NULL;
			    len = strlen (lyr->f_table_name);
			    if (len > 5)
			      {
				  if (strcasecmp
				      (lyr->f_table_name + len - 5,
				       "_face") == 0)
				    {
					classname = "face";
					if (aux->with_faces == 0)
					    skip_topo_sublayer = 1;
				    }
				  if (strcasecmp
				      (lyr->f_table_name + len - 5,
				       "_edge") == 0)
				    {
					classname = "edge";
					if (aux->with_edges_or_links == 0)
					    skip_topo_sublayer = 1;
				    }
				  if (strcasecmp
				      (lyr->f_table_name + len - 5,
				       "_node") == 0)
				    {
					classname = "node";
					if (aux->with_nodes == 0)
					    skip_topo_sublayer = 1;
				    }
			      }
			    if (strlen (lyr->f_table_name) > 6)
			      {
				  if (strcasecmp
				      (lyr->f_table_name + len - 6,
				       "_seeds") == 0)
				    {
					classname = "seeds";
					is_seed = 1;
					if (aux->with_edge_or_link_seeds == 0
					    && aux->with_face_seeds == 0)
					    skip_topo_sublayer = 1;
				    }
			      }
			    if (skip_topo_sublayer)
			      {
				  /* skipping this Topo Primitive Class */
				  sqlite3_free (sql);
				  goto skip_topo_sublayer;
			      }
			    if (classname != NULL)
			      {
				  oldsql = sql;
				  if (is_seed)
				    {
					sql =
					    sqlite3_mprintf
					    ("%s, CASE WHEN edge_id IS NOT NULL THEN 'edge_seed' "
					     "WHEN face_id IS NOT NULL THEN 'face_seed' END AS topoclass",
					     oldsql);
				    }
				  else
				      sql =
					  sqlite3_mprintf
					  ("%s, %Q AS topoclass", oldsql,
					   classname);
				  sqlite3_free (oldsql);
				  has_extra_columns = 1;
			      }
			}
		      else if (is_toponet
			       && strcasecmp (col_name, "topoclass") == 0)
			{
			    /* TopoNet: adding the feature class column for Styling */
			    classname = NULL;
			    len = strlen (lyr->f_table_name);
			    if (len > 5)
			      {
				  if (strcasecmp
				      (lyr->f_table_name + len - 5,
				       "_link") == 0)
				    {
					classname = "link";
					if (aux->with_edges_or_links == 0)
					    skip_topo_sublayer = 1;
				    }
				  if (strcasecmp
				      (lyr->f_table_name + len - 5,
				       "_node") == 0)
				    {
					classname = "node";
					if (aux->with_nodes == 0)
					    skip_topo_sublayer = 1;
				    }
			      }
			    if (strlen (lyr->f_table_name) > 6)
			      {
				  if (strcasecmp
				      (lyr->f_table_name + len - 6,
				       "_seeds") == 0)
				    {
					classname = "link_seed";
					if (aux->with_edge_or_link_seeds == 0)
					    skip_topo_sublayer = 1;
				    }
			      }
			    if (skip_topo_sublayer)
			      {
				  /* skipping this Topo Primitive Class */
				  sqlite3_free (sql);
				  goto skip_topo_sublayer;
			      }
			    if (classname != NULL)
			      {
				  oldsql = sql;
				  sql =
				      sqlite3_mprintf ("%s, %Q AS topoclass",
						       oldsql, classname);
				  sqlite3_free (oldsql);
				  has_extra_columns = 1;
			      }
			}
		      else
			{
			    oldsql = sql;
			    quoted = rl2_double_quoted_sql (col_name);
			    sql =
				sqlite3_mprintf ("%s, \"%s\"", oldsql, quoted);
			    free (quoted);
			    sqlite3_free (oldsql);
			    has_extra_columns = 1;
			}
		  }
	    }
	  else
	    {
		/* there is no Symbolizer */
		int skip_topo_sublayer = 0;
		if (is_topogeo)
		  {
		      len = strlen (lyr->f_table_name);
		      if (len > 5)
			{
			    if (strcasecmp
				(lyr->f_table_name + len - 5, "_face") == 0)
			      {
				  if (aux->with_faces == 0)
				      skip_topo_sublayer = 1;
			      }
			    if (strcasecmp
				(lyr->f_table_name + len - 5, "_edge") == 0)
			      {
				  if (aux->with_edges_or_links == 0)
				      skip_topo_sublayer = 1;
			      }
			    if (strcasecmp
				(lyr->f_table_name + len - 5, "_node") == 0)
			      {
				  if (aux->with_nodes == 0)
				      skip_topo_sublayer = 1;
			      }
			}
		      if (strlen (lyr->f_table_name) > 6)
			{
			    if (strcasecmp
				(lyr->f_table_name + len - 6, "_seeds") == 0)
			      {
				  if (aux->with_edge_or_link_seeds == 0
				      && aux->with_face_seeds == 0)
				      skip_topo_sublayer = 1;
			      }
			}
		      if (skip_topo_sublayer)
			{
			    /* skipping this Topo Primitive Class */
			    sqlite3_free (sql);
			    goto skip_topo_sublayer;
			}
		  }
		else if (is_toponet)
		  {
		      len = strlen (lyr->f_table_name);
		      if (len > 5)
			{
			    if (strcasecmp
				(lyr->f_table_name + len - 5, "_link") == 0)
			      {
				  if (aux->with_edges_or_links == 0)
				      skip_topo_sublayer = 1;
			      }
			    if (strcasecmp
				(lyr->f_table_name + len - 5, "_node") == 0)
			      {
				  if (aux->with_nodes == 0)
				      skip_topo_sublayer = 1;
			      }
			}
		      if (strlen (lyr->f_table_name) > 6)
			{
			    if (strcasecmp
				(lyr->f_table_name + len - 6, "_seeds") == 0)
			      {
				  if (aux->with_edge_or_link_seeds == 0)
				      skip_topo_sublayer = 1;
			      }
			}
		      if (skip_topo_sublayer)
			{
			    /* skipping this Topo Primitive Class */
			    sqlite3_free (sql);
			    goto skip_topo_sublayer;
			}
		  }
	    }
	  if (db_prefix == NULL)
	      db_prefix = "main";
	  xdb_prefix = rl2_double_quoted_sql (db_prefix);
	  if (lyr->view_name != NULL)
	      quoted = rl2_double_quoted_sql (lyr->view_name);
	  else
	      quoted = rl2_double_quoted_sql (lyr->f_table_name);
	  oldsql = sql;
	  sql =
	      sqlite3_mprintf ("%s FROM \"%s\".\"%s\"", oldsql, xdb_prefix,
			       quoted);
	  free (xdb_prefix);
	  free (quoted);
	  sqlite3_free (oldsql);
	  if (reproject_on_the_fly)
	    {
		if (lyr->spatial_index)
		  {
		      /* queryng the R*Tree Spatial Index */
		      char *rowid;
		      char *rtree_name = sqlite3_mprintf ("DB=%s.%s", db_prefix,
							  lyr->f_table_name);
		      oldsql = sql;
		      if (lyr->view_rowid != NULL)
			{
			    char *qrowid =
				rl2_double_quoted_sql (lyr->view_rowid);
			    rowid = sqlite3_mprintf ("\"%s\"", qrowid);
			    free (qrowid);
			}
		      else
			  rowid = sqlite3_mprintf ("ROWID");
		      sql =
			  sqlite3_mprintf
			  ("%s WHERE %s IN (SELECT ROWID FROM SpatialIndex "
			   "WHERE f_table_name = %Q AND f_geometry_column = %Q "
			   "AND search_frame = ST_Transform(?, %d))",
			   oldsql, rowid, rtree_name, lyr->f_geometry_column,
			   srid);
		      sqlite3_free (rtree_name);
		      sqlite3_free (rowid);
		      sqlite3_free (oldsql);
		  }
		else
		  {
		      /* applying MBR filtering */
		      oldsql = sql;
		      if (lyr->view_geometry != NULL)
			  quoted = rl2_double_quoted_sql (lyr->view_geometry);
		      else
			  quoted =
			      rl2_double_quoted_sql (lyr->f_geometry_column);
		      sql =
			  sqlite3_mprintf
			  ("%s WHERE MbrIntersects(\"%s\", ST_Transform(?, %d))",
			   oldsql, quoted, srid);
		      free (quoted);
		      sqlite3_free (oldsql);
		  }
	    }
	  else
	    {
		if (lyr->spatial_index)
		  {
		      /* queryng the R*Tree Spatial Index */
		      char *rowid;
		      char *rtree_name = sqlite3_mprintf ("DB=%s.%s", db_prefix,
							  lyr->f_table_name);
		      oldsql = sql;
		      if (lyr->view_rowid != NULL)
			{
			    char *qrowid =
				rl2_double_quoted_sql (lyr->view_rowid);
			    rowid = sqlite3_mprintf ("\"%s\"", qrowid);
			    free (qrowid);
			}
		      else
			  rowid = sqlite3_mprintf ("ROWID");
		      sql =
			  sqlite3_mprintf
			  ("%s WHERE %s IN (SELECT ROWID FROM SpatialIndex "
			   "WHERE f_table_name = %Q AND f_geometry_column = %Q AND search_frame = ?)",
			   oldsql, rowid, rtree_name, lyr->f_geometry_column);
		      sqlite3_free (rtree_name);
		      sqlite3_free (rowid);
		      sqlite3_free (oldsql);
		  }
		else
		  {
		      /* applying MBR filtering */
		      oldsql = sql;
		      if (lyr->view_geometry != NULL)
			  quoted = rl2_double_quoted_sql (lyr->view_geometry);
		      else
			  quoted =
			      rl2_double_quoted_sql (lyr->f_geometry_column);
		      sql =
			  sqlite3_mprintf ("%s WHERE MbrIntersects(\"%s\", ?)",
					   oldsql, quoted);
		      free (quoted);
		      sqlite3_free (oldsql);
		  }
	    }
	  if (is_topogeo)
	    {
		if (is_face)
		  {
		      oldsql = sql;
		      sql = sqlite3_mprintf ("%s AND face_id > 0", oldsql);
		      sqlite3_free (oldsql);
		  }
		if (is_seed)
		  {
		      if (aux->with_edge_or_link_seeds != 0
			  && aux->with_face_seeds == 0)
			{
			    oldsql = sql;
			    sql =
				sqlite3_mprintf ("%s AND edge_id IS NOT NULL",
						 oldsql);
			    sqlite3_free (oldsql);
			}
		      if (aux->with_edge_or_link_seeds == 0
			  && aux->with_face_seeds != 0)
			{
			    oldsql = sql;
			    sql =
				sqlite3_mprintf ("%s AND face_id IS NOT NULL",
						 oldsql);
			    sqlite3_free (oldsql);
			}
		  }
	    }

	  /* preparing the SQL statement */
	  fprintf (stderr, "%s\n", sql);
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (sqlite));
		goto error;
	    }

	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_blob (stmt, 1, blob, blob_sz, SQLITE_STATIC);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      rl2GeometryPtr geom = NULL;
		      if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
			{
			    /* fetching Geometry */
			    const void *g_blob = sqlite3_column_blob (stmt, 0);
			    int g_blob_sz = sqlite3_column_bytes (stmt, 0);
			    geom =
				rl2_geometry_from_blob ((const unsigned char
							 *) g_blob, g_blob_sz);
			}
		      if (has_extra_columns)
			{
			    if (variant != NULL)
				rl2_destroy_variant_array (variant);
			    variant = rl2_create_variant_array (columns);
			    for (i = 0; i < columns; i++)
			      {
				  const char *col_name =
				      rl2_get_feature_type_style_column_name
				      (lyr_stl,
				       i);
				  switch (sqlite3_column_type (stmt, 1 + i))
				    {
				    case SQLITE_INTEGER:
					rl2_set_variant_int (variant, i,
							     col_name,
							     sqlite3_column_int64
							     (stmt, i + 1));
					break;
				    case SQLITE_FLOAT:
					rl2_set_variant_double (variant, i,
								col_name,
								sqlite3_column_double
								(stmt, i + 1));
					break;
				    case SQLITE_TEXT:
					rl2_set_variant_text (variant, i,
							      col_name,
							      (const char *)
							      sqlite3_column_text
							      (stmt, i + 1),
							      sqlite3_column_bytes
							      (stmt, i + 1));
					break;
				    case SQLITE_BLOB:
					rl2_set_variant_blob (variant, i,
							      col_name,
							      sqlite3_column_blob
							      (stmt, i + 1),
							      sqlite3_column_bytes
							      (stmt, i + 1));
					break;
				    default:
					rl2_set_variant_null (variant, i,
							      col_name);
					break;
				    };
			      }
			}
		      if (geom != NULL)
			{
			    /* drawing a styled Feature */
			    int scale_forbidden = 0;
			    symbolizer = NULL;
			    if (lyr_stl != NULL)
				symbolizer =
				    rl2_get_symbolizer_from_feature_type_style
				    (lyr_stl, scale, variant, &scale_forbidden);
			    if (!scale_forbidden)
				rl2_draw_vector_feature (ctx, ctx_labels,
							 sqlite, priv_data,
							 symbolizer, height,
							 minx, miny, maxx, maxy,
							 x_res, y_res, geom,
							 variant);
			    rl2_destroy_geometry (geom);
			}
		  }
	    }
	  sqlite3_finalize (stmt);
	  stmt = NULL;

	skip_topo_sublayer:
	  which = RL2_CANVAS_UNKNOWN_CTX;
	  if (is_topogeo)
	    {
		len = strlen (lyr->f_table_name);
		if (len > 5)
		  {
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_face") == 0)
			{
			    /* using the Canvas Faces Graphics Context */
			    which = RL2_CANVAS_FACES_CTX;
			}
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_edge") == 0)
			{
			    /* using the Canvas Edges Graphics Context */
			    which = RL2_CANVAS_EDGES_CTX;
			}
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_node") == 0)
			{
			    /* using the Canvas Nodes Graphics Context */
			    which = RL2_CANVAS_NODES_CTX;
			}
		  }
		if (strlen (lyr->f_table_name) > 6)
		  {
		      if (strcasecmp
			  (lyr->f_table_name + len - 6, "_seeds") == 0)
			{
			    /* using the Canvas FaceSeeds Graphics Context */
			    which = RL2_CANVAS_FACE_SEEDS_CTX;
			}
		  }
	    }
	  else if (is_toponet)
	    {
		len = strlen (lyr->f_table_name);
		if (len > 5)
		  {
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_link") == 0)
			{
			    /* using the Canvas Links Graphics Context */
			    which = RL2_CANVAS_LINKS_CTX;
			}
		      if (strcasecmp
			  (lyr->f_table_name + len - 5, "_node") == 0)
			{
			    /* using the Canvas Nodes Graphics Context */
			    which = RL2_CANVAS_NODES_CTX;
			}
		  }
		if (strlen (lyr->f_table_name) > 6)
		  {
		      if (strcasecmp
			  (lyr->f_table_name + len - 6, "_seeds") == 0)
			{
			    /* using the Canvas LinkSeeds Graphics Context */
			    which = RL2_CANVAS_LINK_SEEDS_CTX;
			}
		  }
	    }
	  else
	    {
		/* using the Canvas Base Graphics Context */
		which = RL2_CANVAS_BASE_CTX;
	    }
	  do_set_canvas_ready (canvas, which);
      }

  done:
    if (aux->output == NULL)
      {
	  rl2_destroy_multi_layer (multi);
	  if (lyr_stl != NULL)
	      rl2_destroy_feature_type_style (lyr_stl);
	  if (variant != NULL)
	      rl2_destroy_variant_array (variant);
	  if (is_topogeo)
	    {
		/* merging Topology sub-Layers */
		rl2GraphicsContextPtr ctx_in;
		rl2GraphicsContextPtr ctx_out =
		    rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
		ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_FACES_CTX);
		if (ctx_out != NULL && ctx_in != NULL)
		    rl2_graph_merge (ctx_out, ctx_in);
		ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_EDGES_CTX);
		if (ctx_out != NULL && ctx_in != NULL)
		    rl2_graph_merge (ctx_out, ctx_in);
		ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_NODES_CTX);
		if (ctx_out != NULL && ctx_in != NULL)
		    rl2_graph_merge (ctx_out, ctx_in);
		ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_EDGE_SEEDS_CTX);
		if (ctx_out != NULL && ctx_in != NULL)
		    rl2_graph_merge (ctx_out, ctx_in);
		ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_FACE_SEEDS_CTX);
		if (ctx_out != NULL && ctx_in != NULL)
		    rl2_graph_merge (ctx_out, ctx_in);
		do_set_canvas_ready (canvas, RL2_CANVAS_BASE_CTX);
	    }
	  if (is_toponet)
	    {
		/* merging Network sub-Layers */
		rl2GraphicsContextPtr ctx_in;
		rl2GraphicsContextPtr ctx_out =
		    rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
		ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_LINKS_CTX);
		if (ctx_out != NULL && ctx_in != NULL)
		    rl2_graph_merge (ctx_out, ctx_in);
		ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_NODES_CTX);
		if (ctx_out != NULL && ctx_in != NULL)
		    rl2_graph_merge (ctx_out, ctx_in);
		ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_LINK_SEEDS_CTX);
		if (ctx_out != NULL && ctx_in != NULL)
		    rl2_graph_merge (ctx_out, ctx_in);
		do_set_canvas_ready (canvas, RL2_CANVAS_BASE_CTX);
	    }
	  return RL2_OK;
      }

    if (is_topogeo)
      {
	  /* merging Topology sub-Layers */
	  rl2GraphicsContextPtr ctx_in;
	  rl2GraphicsContextPtr ctx_out =
	      rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
	  ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_FACES_CTX);
	  if (ctx_out != NULL && ctx_in != NULL)
	      rl2_graph_merge (ctx_out, ctx_in);
	  ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_EDGES_CTX);
	  if (ctx_out != NULL && ctx_in != NULL)
	      rl2_graph_merge (ctx_out, ctx_in);
	  ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_NODES_CTX);
	  if (ctx_out != NULL && ctx_in != NULL)
	      rl2_graph_merge (ctx_out, ctx_in);
	  ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_EDGE_SEEDS_CTX);
	  if (ctx_out != NULL && ctx_in != NULL)
	      rl2_graph_merge (ctx_out, ctx_in);
	  ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_FACE_SEEDS_CTX);
	  if (ctx_out != NULL && ctx_in != NULL)
	      rl2_graph_merge (ctx_out, ctx_in);
	  do_set_canvas_ready (canvas, RL2_CANVAS_BASE_CTX);
      }
    if (is_toponet)
      {
	  /* merging Network sub-Layers */
	  rl2GraphicsContextPtr ctx_in;
	  rl2GraphicsContextPtr ctx_out =
	      rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
	  ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_LINKS_CTX);
	  if (ctx_out != NULL && ctx_in != NULL)
	      rl2_graph_merge (ctx_out, ctx_in);
	  ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_NODES_CTX);
	  if (ctx_out != NULL && ctx_in != NULL)
	      rl2_graph_merge (ctx_out, ctx_in);
	  ctx_in = rl2_get_canvas_ctx (canvas, RL2_CANVAS_LINK_SEEDS_CTX);
	  if (ctx_out != NULL && ctx_in != NULL)
	      rl2_graph_merge (ctx_out, ctx_in);
	  do_set_canvas_ready (canvas, RL2_CANVAS_BASE_CTX);
      }
    if (ctx_labels != NULL)
      {
	  /* merging Label sub-layer */
	  rl2GraphicsContextPtr ctx_in =
	      rl2_get_canvas_ctx (canvas, RL2_CANVAS_LABELS_CTX);
	  rl2GraphicsContextPtr ctx_out =
	      rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
	  rl2_graph_merge (ctx_out, ctx_in);
      }

    rl2_destroy_multi_layer (multi);
    multi = NULL;
    if (lyr_stl != NULL)
	rl2_destroy_feature_type_style (lyr_stl);
    lyr_stl = NULL;
    if (variant != NULL)
	rl2_destroy_variant_array (variant);
    variant = NULL;

    ctx = rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
    rgb = rl2_graph_get_context_rgb_array (ctx);
    alpha = rl2_graph_get_context_alpha_array (ctx, &half_transparent);
    if (rgb == NULL || alpha == NULL)
	goto error;

    if (!get_payload_from_rgb_rgba_transparent
	(width, height, rgb, alpha, format_id, quality, &image, &image_size,
	 1.0, half_transparent))
	goto error;
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    aux->output->img = image;
    aux->output->img_size = image_size;
    return RL2_OK;

  error:
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    if (canvas == NULL)
      {
	  if (ctx != NULL)
	      rl2_graph_destroy_context (ctx);
      }
    rl2_destroy_multi_layer (multi);
    multi = NULL;
    if (lyr_stl != NULL)
	rl2_destroy_feature_type_style (lyr_stl);
    if (variant != NULL)
	rl2_destroy_variant_array (variant);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    aux->output->img = NULL;
    aux->output->img_size = 0;
    return RL2_ERROR;
}

static int
do_check_topogeo (sqlite3 * sqlite, const char *db_prefix, const char *coverage)
{
/* checking for a TopoGeo Coverage */
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    int ret;
    char *xdb_prefix;
    int ok = 0;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);

    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM \"%s\".topologies "
	 "WHERE Lower(topology_name) = Lower(%Q)", xdb_prefix, coverage);
    free (xdb_prefix);
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
		if (atoi (results[(i * columns) + 0]) == 1)
		    ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
do_check_toponet (sqlite3 * sqlite, const char *db_prefix, const char *coverage)
{
/* checking for a TopoNet Coverage */
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    int ret;
    char *xdb_prefix;
    int ok = 0;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);

    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM \"%s\".vector_coverages AS a\n"
	 "JOIN \"%s\".networks AS b ON (Lower(a.network_name) = Lower(b.network_name))"
	 "WHERE Lower(coverage_name) = Lower(%Q)", xdb_prefix, xdb_prefix,
	 coverage);
    free (xdb_prefix);
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
		if (atoi (results[(i * columns) + 0]) == 1)
		    ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

RL2_DECLARE int
rl2_map_image_blob_from_vector (sqlite3 * sqlite, const void *data,
				const char *db_prefix, const char *cvg_name,
				const unsigned char *blob, int blob_sz,
				int width, int height, const char *style,
				const char *format, const char *bg_color,
				int transparent, int quality, int reaspect,
				unsigned char **img, int *img_size)
{
/* rendering a Map Image from a Vector Coverage - BLOB */
    unsigned char bg_red = 0;
    unsigned char bg_green = 0;
    unsigned char bg_blue = 0;
    unsigned char bg_alpha = 0;
    int is_topogeo = do_check_topogeo (sqlite, db_prefix, cvg_name);
    int is_toponet = do_check_toponet (sqlite, db_prefix, cvg_name);
    rl2GraphicsContextPtr ctx = NULL;
    rl2GraphicsContextPtr ctx_labels = NULL;
    rl2GraphicsContextPtr ctx_nodes = NULL;
    rl2GraphicsContextPtr ctx_edges = NULL;
    rl2GraphicsContextPtr ctx_edge_seeds = NULL;
    rl2GraphicsContextPtr ctx_faces = NULL;
    rl2GraphicsContextPtr ctx_face_seeds = NULL;
    rl2GraphicsContextPtr ctx_links = NULL;
    rl2GraphicsContextPtr ctx_link_seeds = NULL;
    struct aux_vector_render aux;
    aux.sqlite = sqlite;
    aux.data = data;
    aux.canvas = NULL;
    aux.db_prefix = db_prefix;
    aux.cvg_name = cvg_name;
    aux.blob = blob;
    aux.blob_sz = blob_sz;
    aux.width = width;
    aux.height = height;
    aux.style = style;
    aux.quick_style = NULL;
    aux.with_nodes = 1;
    aux.with_edges_or_links = 1;
    aux.with_faces = 0;
    aux.with_edge_or_link_seeds = 1;
    aux.with_face_seeds = 1;
    aux.output = malloc (sizeof (struct aux_raster_image));
    aux.output->bg_red = 255;
    aux.output->bg_green = 255;
    aux.output->bg_blue = 255;
    aux.output->transparent = transparent;
    aux.output->format = format;
    aux.output->quality = quality;
    aux.output->img = NULL;
    aux.output->img_size = 0;

    if (reaspect == 0)
      {
	  /* testing for consistent aspect ratios */
	  double aspect_org =
	      do_compute_bbox_aspect_ratio (sqlite, blob, blob_sz);
	  double aspect_dst = (double) width / (double) height;
	  double confidence = aspect_org / 100.0;
	  aspect_org = do_compute_bbox_aspect_ratio (sqlite, blob, blob_sz);
	  if (aspect_org < 0.0)
	      goto error;
	  aspect_dst = (double) width / (double) height;
	  confidence = aspect_org / 100.0;
	  if (aspect_dst >= (aspect_org - confidence)
	      && aspect_dst <= (aspect_org + confidence))
	      ;
	  else
	      goto error;
      }

/* creating a Canvas */
    ctx = rl2_graph_create_context (width, height);
    if (ctx == NULL)
	goto error;
    ctx_labels = rl2_graph_create_context (width, height);
    if (ctx_labels == NULL)
	goto error;
    if (is_topogeo)
      {
	  /* Topology canvas */
	  ctx_nodes = rl2_graph_create_context (width, height);
	  if (ctx_nodes == NULL)
	      goto error;
	  ctx_edges = rl2_graph_create_context (width, height);
	  if (ctx_edges == NULL)
	      goto error;
	  ctx_edge_seeds = rl2_graph_create_context (width, height);
	  if (ctx_edge_seeds == NULL)
	      goto error;
	  ctx_faces = rl2_graph_create_context (width, height);
	  if (ctx_faces == NULL)
	      goto error;
	  ctx_face_seeds = rl2_graph_create_context (width, height);
	  if (ctx_face_seeds == NULL)
	      goto error;
	  aux.canvas =
	      rl2_create_topology_canvas (ctx, ctx_labels, ctx_nodes, ctx_edges,
					  ctx_faces, ctx_edge_seeds,
					  ctx_face_seeds);
      }
    else if (is_toponet)
      {
	  /* Network canvas */
	  ctx_nodes = rl2_graph_create_context (width, height);
	  if (ctx_nodes == NULL)
	      goto error;
	  ctx_links = rl2_graph_create_context (width, height);
	  if (ctx_links == NULL)
	      goto error;
	  ctx_link_seeds = rl2_graph_create_context (width, height);
	  if (ctx_link_seeds == NULL)
	      goto error;
	  aux.canvas =
	      rl2_create_network_canvas (ctx, ctx_labels, ctx_nodes, ctx_links,
					 ctx_link_seeds);
      }
    else
      {
	  /* generic Vector canvas */
	  aux.canvas = rl2_create_vector_canvas (ctx, ctx_labels);
      }
    if (aux.canvas == NULL)
	goto error;
    if (!transparent)
      {
	  /* parsing the background color */
	  bg_alpha = 255;
	  if (rl2_parse_hexrgb (bg_color, &bg_red, &bg_green, &bg_blue) !=
	      RL2_OK)
	    {
		bg_red = 255;
		bg_green = 255;
		bg_blue = 255;
	    }
      }
    aux.output->bg_red = bg_red;
    aux.output->bg_green = bg_green;
    aux.output->bg_blue = bg_blue;

/* priming the background */
    rl2_prime_background (ctx, bg_red, bg_green, bg_blue, bg_alpha);
    rl2_prime_background (ctx_labels, 0, 0, 0, 0);	/* labels layer: always transparent */
    if (ctx_nodes != NULL)
	rl2_prime_background (ctx_nodes, 0, 0, 0, 0);	/* Nodes layer: always transparent */
    if (ctx_edges != NULL)
	rl2_prime_background (ctx_edges, 0, 0, 0, 0);	/* Edges layer: always transparent */
    if (ctx_edge_seeds != NULL)
	rl2_prime_background (ctx_edge_seeds, 0, 0, 0, 0);	/* EdgeSeeds layer: always transparent */
    if (ctx_faces != NULL)
	rl2_prime_background (ctx_faces, 0, 0, 0, 0);	/* Faces layer: always transparent */
    if (ctx_face_seeds != NULL)
	rl2_prime_background (ctx_face_seeds, 0, 0, 0, 0);	/* FaceSeeds layer: always transparent */
    if (ctx_links != NULL)
	rl2_prime_background (ctx_links, 0, 0, 0, 0);	/* Links layer: always transparent */
    if (ctx_link_seeds != NULL)
	rl2_prime_background (ctx_link_seeds, 0, 0, 0, 0);	/* LinkSeeds layer: always transparent */

    if (do_paint_map_from_vector (&aux) == RL2_OK)
      {
	  rl2_graph_destroy_context (ctx);
	  rl2_graph_destroy_context (ctx_labels);
	  if (ctx_nodes != NULL)
	      rl2_graph_destroy_context (ctx_nodes);
	  if (ctx_edges != NULL)
	      rl2_graph_destroy_context (ctx_edges);
	  if (ctx_edge_seeds != NULL)
	      rl2_graph_destroy_context (ctx_edge_seeds);
	  if (ctx_faces != NULL)
	      rl2_graph_destroy_context (ctx_faces);
	  if (ctx_face_seeds != NULL)
	      rl2_graph_destroy_context (ctx_face_seeds);
	  if (ctx_links != NULL)
	      rl2_graph_destroy_context (ctx_links);
	  if (ctx_link_seeds != NULL)
	      rl2_graph_destroy_context (ctx_link_seeds);
	  rl2_destroy_canvas (aux.canvas);
	  *img = aux.output->img;
	  *img_size = aux.output->img_size;
	  free (aux.output);
	  return RL2_OK;
      }

  error:
    if (aux.output != NULL)
	free (aux.output);
    if (ctx != NULL)
	rl2_graph_destroy_context (ctx);
    if (ctx_labels != NULL)
	rl2_graph_destroy_context (ctx_labels);
    if (ctx_nodes != NULL)
	rl2_graph_destroy_context (ctx_nodes);
    if (ctx_edges != NULL)
	rl2_graph_destroy_context (ctx_edges);
    if (ctx_edge_seeds != NULL)
	rl2_graph_destroy_context (ctx_edge_seeds);
    if (ctx_faces != NULL)
	rl2_graph_destroy_context (ctx_faces);
    if (ctx_face_seeds != NULL)
	rl2_graph_destroy_context (ctx_face_seeds);
    if (ctx_links != NULL)
	rl2_graph_destroy_context (ctx_links);
    if (ctx_link_seeds != NULL)
	rl2_graph_destroy_context (ctx_link_seeds);
    if (aux.canvas != NULL)
	rl2_destroy_canvas (aux.canvas);
    *img = NULL;
    *img_size = 0;
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_map_image_paint_from_vector (sqlite3 * sqlite, const void *data,
				 rl2CanvasPtr canvas, const char *db_prefix,
				 const char *cvg_name,
				 const unsigned char *blob, int blob_sz,
				 int reaspect, const char *style,
				 const unsigned char *quick_style)
{
/* rendering a Map Image from a Vector Coverage - Graphics Context */
    struct aux_vector_render aux;
    rl2GraphicsContextPtr ctx;
    int width;
    int height;

    if (canvas == NULL)
	return RL2_ERROR;
    ctx = rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
    if (ctx == NULL)
	return RL2_ERROR;
    if (rl2_graph_context_get_dimensions (ctx, &width, &height) != RL2_OK)
	return RL2_ERROR;

    if (reaspect == 0)
      {
	  /* testing for consistent aspect ratios */
	  double aspect_org =
	      do_compute_bbox_aspect_ratio (sqlite, blob, blob_sz);
	  double aspect_dst = (double) width / (double) height;
	  double confidence = aspect_org / 100.0;
	  aspect_org = do_compute_bbox_aspect_ratio (sqlite, blob, blob_sz);
	  if (aspect_org < 0.0)
	      return RL2_ERROR;
	  aspect_dst = (double) width / (double) height;
	  confidence = aspect_org / 100.0;
	  if (aspect_dst >= (aspect_org - confidence)
	      && aspect_dst <= (aspect_org + confidence))
	      ;
	  else
	      return RL2_ERROR;
      }

    aux.sqlite = sqlite;
    aux.data = data;
    aux.canvas = canvas;
    aux.db_prefix = db_prefix;
    aux.cvg_name = cvg_name;
    aux.blob = blob;
    aux.blob_sz = blob_sz;
    aux.width = width;
    aux.height = height;
    aux.style = style;
    aux.quick_style = quick_style;
    aux.output = NULL;
    aux.with_nodes = 1;
    aux.with_edges_or_links = 1;
    aux.with_faces = 0;
    aux.with_edge_or_link_seeds = 1;
    aux.with_face_seeds = 1;
    return do_paint_map_from_vector (&aux);
}

RL2_DECLARE int
rl2_map_image_paint_from_vector_ex (sqlite3 * sqlite, const void *data,
				    rl2CanvasPtr canvas, const char *db_prefix,
				    const char *cvg_name,
				    const unsigned char *blob, int blob_sz,
				    int reaspect, const char *style,
				    const unsigned char *quick_style,
				    int with_nodes, int with_edges_or_links,
				    int with_faces, int with_edge_or_link_seeds,
				    int with_face_seeds)
{
/* rendering a Map Image from a Vector Coverage - Graphics Context - extended */
    struct aux_vector_render aux;
    rl2GraphicsContextPtr ctx;
    int width;
    int height;

    if (canvas == NULL)
	return RL2_ERROR;
    ctx = rl2_get_canvas_ctx (canvas, RL2_CANVAS_BASE_CTX);
    if (ctx == NULL)
	return RL2_ERROR;
    if (rl2_graph_context_get_dimensions (ctx, &width, &height) != RL2_OK)
	return RL2_ERROR;

    if (reaspect == 0)
      {
	  /* testing for consistent aspect ratios */
	  double aspect_org =
	      do_compute_bbox_aspect_ratio (sqlite, blob, blob_sz);
	  double aspect_dst = (double) width / (double) height;
	  double confidence = aspect_org / 100.0;
	  aspect_org = do_compute_bbox_aspect_ratio (sqlite, blob, blob_sz);
	  if (aspect_org < 0.0)
	      return RL2_ERROR;
	  aspect_dst = (double) width / (double) height;
	  confidence = aspect_org / 100.0;
	  if (aspect_dst >= (aspect_org - confidence)
	      && aspect_dst <= (aspect_org + confidence))
	      ;
	  else
	      return RL2_ERROR;
      }

    aux.sqlite = sqlite;
    aux.data = data;
    aux.canvas = canvas;
    aux.db_prefix = db_prefix;
    aux.cvg_name = cvg_name;
    aux.blob = blob;
    aux.blob_sz = blob_sz;
    aux.width = width;
    aux.height = height;
    aux.style = style;
    aux.quick_style = quick_style;
    aux.output = NULL;
    aux.with_nodes = with_nodes;
    aux.with_edges_or_links = with_edges_or_links;
    aux.with_faces = with_faces;
    aux.with_edge_or_link_seeds = with_edge_or_link_seeds;
    aux.with_face_seeds = with_face_seeds;
    return do_paint_map_from_vector (&aux);
}
