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

#define RL2_UNUSED() if (argc || argv) argc = argc;

RL2_PRIVATE int
get_coverage_sample_bands (sqlite3 * sqlite, const char *coverage,
			   unsigned char *sample_type, unsigned char *num_bands)
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

    sql =
	sqlite3_mprintf ("SELECT sample_type, num_bands FROM raster_coverages "
			 "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
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
get_coverage_defs (sqlite3 * sqlite, const char *coverage,
		   unsigned int *tile_width, unsigned int *tile_height,
		   unsigned char *sample_type, unsigned char *pixel_type,
		   unsigned char *num_bands, unsigned char *compression)
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

    sql = sqlite3_mprintf ("SELECT sample_type, pixel_type, num_bands, "
			   "compression, tile_width, tile_height FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
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
		 double *x, double *y)
{
/* attempts to get the coordinates from a POINT */
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    double pt_x;
    double pt_y;
    int count = 0;

    sql = "SELECT ST_X(?), ST_Y(?) WHERE ST_GeometryType(?) IN "
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
do_insert_wms_tile (sqlite3 * handle, unsigned char *blob_odd, int blob_odd_sz,
		    unsigned char *blob_even, int blob_even_sz,
		    sqlite3_int64 section_id, int srid, double res_x,
		    double res_y, unsigned int tile_w, unsigned int tile_h,
		    double miny, double maxx, double tile_minx,
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
		       int section_paths, int section_md5, int section_summary,
		       sqlite3_stmt * stmt_sect, sqlite3_int64 * id)
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
    if (rl2_build_bbox (handle, srid, minx, miny, maxx, maxy, &blob, &blob_size)
	!= RL2_OK)
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

    if (rl2_get_coverage_resolution (ptr->coverage, &base_res_x, &base_res_y) !=
	RL2_OK)
	goto error;
    if (*first)
      {
	  /* INSERTing the section */
	  *first = 0;
	  if (!rl2_do_insert_section
	      (ptr->sqlite, "WMS Service", ptr->sect_name, ptr->srid,
	       ptr->width, ptr->height, ptr->minx, ptr->miny, ptr->maxx,
	       ptr->maxy, ptr->xml_summary, ptr->sectionPaths, ptr->sectionMD5,
	       ptr->sectionSummary, ptr->stmt_sect, section_id))
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
	 *section_id, ptr->srid, ptr->horz_res, ptr->vert_res, ptr->tile_width,
	 ptr->tile_height, ptr->miny, ptr->maxx, tile_minx, tile_miny,
	 tile_maxx, tile_maxy, NULL, ptr->no_data, ptr->stmt_tils,
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
rl2_find_best_resolution_level (sqlite3 * handle, const char *coverage,
				int by_section, sqlite3_int64 section_id,
				double x_res, double y_res, int *level_id,
				int *scale, int *real_scale, double *xx_res,
				double *yy_res)
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

    if (coverage == NULL)
	return 0;

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
	       "x_resolution_1_1, y_resolution_1_1 FROM \"%s\" "
	       "WHERE section_id = %s ORDER BY pyramid_level DESC", xxcoverage,
	       sctn);
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
	       "x_resolution_1_1, y_resolution_1_1 FROM \"%s\" "
	       "ORDER BY pyramid_level DESC", xxcoverage);
      }
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
				    sqlite3 * handle, double minx, double miny,
				    double maxx, double maxy, int srid,
				    unsigned char *pixels, unsigned char format,
				    int quality, unsigned char **image,
				    int *image_sz)
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
		    (width, height, handle, minx, miny, maxx, maxy, srid, gray,
		     image, image_sz) != RL2_OK)
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
			  (width, height, handle, minx, miny, maxx, maxy, srid,
			   rgb, image, image_sz) != RL2_OK)
			  goto error;
		  }
		else
		  {
		      if (rl2_rgb_to_tiff (width, height, rgb, image, image_sz)
			  != RL2_OK)
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
			  (width, height, handle, minx, miny, maxx, maxy, srid,
			   gray, image, image_sz) != RL2_OK)
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
				   unsigned char *pixels, unsigned char format,
				   int quality, unsigned char **image,
				   int *image_sz)
{
/* input: Grayscale    output: Grayscale */
    int ret;
    unsigned char *rgba = NULL;

    if (format == RL2_OUTPUT_FORMAT_JPEG)
      {
	  if (rl2_gray_to_jpeg (width, height, pixels, quality, image, image_sz)
	      != RL2_OK)
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
		if (rl2_gray_to_tiff (width, height, pixels, image, image_sz) !=
		    RL2_OK)
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
	  if (rl2_rgb_to_jpeg (width, height, pixels, quality, image, image_sz)
	      != RL2_OK)
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
		if (rl2_rgb_to_tiff (width, height, pixels, image, image_sz) !=
		    RL2_OK)
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
				  unsigned char bg_green, unsigned char bg_blue,
				  double opacity)
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
    return 0;
}

static int
test_no_data_u8 (rl2PrivPixelPtr no_data, unsigned char *p_in)
{
/* testing for NO-DATA - UINT8 */
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
test_no_data_16 (rl2PrivPixelPtr no_data, short *p_in)
{
/* testing for NO-DATA - INT16 */
    if (no_data != NULL)
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
    return 0;
}

static int
test_no_data_u16 (rl2PrivPixelPtr no_data, unsigned short *p_in)
{
/* testing for NO-DATA - UINT16 */
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

static int
test_no_data_32 (rl2PrivPixelPtr no_data, int *p_in)
{
/* testing for NO-DATA - INT32 */
    if (no_data != NULL)
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
    return 0;
}

static int
test_no_data_u32 (rl2PrivPixelPtr no_data, unsigned int *p_in)
{
/* testing for NO-DATA - UINT32 */
    if (no_data != NULL)
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
    return 0;
}

static int
test_no_data_flt (rl2PrivPixelPtr no_data, float *p_in)
{
/* testing for NO-DATA - FLOAT */
    if (no_data != NULL)
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
    return 0;
}

static int
test_no_data_dbl (rl2PrivPixelPtr no_data, double *p_in)
{
/* testing for NO-DATA - DOUBLE */
    if (no_data != NULL)
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
    return 0;
}

RL2_PRIVATE int
get_rgba_from_monochrome_mask (unsigned int width, unsigned int height,
			       unsigned char *pixels, unsigned char *mask,
			       rl2PrivPixelPtr no_data, unsigned char *rgba)
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
		unsigned char value = 255;
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
		      if (*p_in++ == 1)
			  value = 0;
		      *p_out++ = value;
		      *p_out++ = value;
		      *p_out++ = value;
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
get_rgba_from_monochrome_opaque (unsigned int width, unsigned int height,
				 unsigned char *pixels, unsigned char *rgba)
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
		if (*p_in++ == 1)
		  {
		      *p_out++ = 0;	/* Black */
		      *p_out++ = 0;
		      *p_out++ = 0;
		      *p_out++ = 255;	/* alpha */
		  }
		else
		  {
		      *p_out++ = 255;	/* White */
		      *p_out++ = 255;
		      *p_out++ = 255;
		      *p_out++ = 255;	/* alpha */
		  }
	    }
      }
    free (pixels);
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
		if (*p_in++ == 1)
		  {
		      *p_out++ = 0;	/* Black */
		      *p_out++ = 0;
		      *p_out++ = 0;
		      *p_out++ = 255;	/* alpha */
		  }
		else
		  {
		      *p_out++ = 255;	/* White */
		      *p_out++ = 255;
		      *p_out++ = 255;
		      *p_out++ = 0;	/* alpha */
		  }
	    }
      }
    free (pixels);
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
get_rgba_from_palette_opaque (unsigned int width, unsigned int height,
			      unsigned char *pixels, rl2PalettePtr palette,
			      unsigned char *rgba)
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
		      *p_out++ = 255;	/* alpha */
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
		      *p_out++ = 255;	/* alpha */
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
get_rgba_from_palette_transparent (unsigned int width, unsigned int height,
				   unsigned char *pixels, rl2PalettePtr palette,
				   unsigned char *rgba, unsigned char bg_red,
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
get_rgba_from_grayscale_opaque (unsigned int width, unsigned int height,
				unsigned char *pixels, unsigned char *rgba)
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
		*p_out++ = 255;	/* alpha */
	    }
      }
    free (pixels);
    return 1;
}

RL2_PRIVATE int
get_rgba_from_grayscale_transparent (unsigned int width,
				     unsigned int height,
				     unsigned char *pixels, unsigned char *rgba,
				     unsigned char bg_gray)
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
get_rgba_from_rgb_opaque (unsigned int width, unsigned int height,
			  unsigned char *pixels, unsigned char *rgba)
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
		*p_out++ = *p_in++;	/* red */
		*p_out++ = *p_in++;	/* green */
		*p_out++ = *p_in++;	/* blue */
		*p_out++ = 255;	/* alpha */
	    }
      }
    free (pixels);
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
		double gray = (double) (*p_in++ - min) / tic;
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
		double gray = (double) (*p_in++ - min) / tic;
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
		double gray = (double) (*p_in++ - min) / tic;
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
		double gray = (double) (*p_in++ - min) / tic;
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
		double gray = (double) (*p_in++ - min) / tic;
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
		  double *pixels, unsigned char *mask, rl2PrivPixelPtr no_data,
		  unsigned char *rgba)
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
		double gray = (double) (*p_in++ - min) / tic;
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
	      rgba_from_uint16 (width, height, (unsigned short *) pixels, mask,
				no_data, rgba);
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
	      rgba_from_double (width, height, (double *) pixels, mask, no_data,
				rgba);
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
		double gray = (double) (*p_in - min) / tic;
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
				      (unsigned short *) pixels, mask, no_data,
				      rgba);
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
		    (width, height, handle, minx, miny, maxx, maxy, srid, gray,
		     image, image_sz) != RL2_OK)
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
	  if (rl2_rgb_to_jpeg (width, height, rgb, quality, image, image_sz) !=
	      RL2_OK)
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
				       unsigned char *rgb, unsigned char *alpha,
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
		if (*p_alpha++ >= 128)
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
		      double gray = (double) (*p_in++ - min) / tic;
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
		      double gray = (double) (*p_in++ - min) / tic;
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
		      double gray = (double) (*p_in++ - min) / tic;
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
		    const char *title, const char *abstract)
{
/* auxiliary function: updates the Coverage descriptive infos */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

/* checking if the Coverage already exists */
    sql = "SELECT coverage_name FROM raster_coverages "
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
    sql = "UPDATE raster_coverages SET title = ?, abstract = ? "
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
    sqlite3_bind_text (stmt, 2, abstract, strlen (abstract), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	fprintf (stderr, "SetCoverageInfos() error: \"%s\"\n",
		 sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

RL2_PRIVATE int
rl2_test_layer_group (sqlite3 * handle, const char *name)
{
/* testing for an eventual Layer Group */
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    int ok = 0;
/* testing if Layer Group exists */
    char *sql = sqlite3_mprintf ("SELECT group_name FROM SE_styled_groups "
				 "WHERE Lower(group_name) = Lower(%Q)", name);
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
rl2_is_mixed_resolutions_coverage (sqlite3 * handle, const char *coverage)
{
/* querying the Coverage Policies defs */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    int value = -1;
    sql =
	"SELECT mixed_resolutions "
	"FROM raster_coverages WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
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
