/*

 test_map_ascii.c -- RasterLite-2 Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

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

#include "rasterlite2/rasterlite2.h"

#define TILE_256	256
#define TILE_1024	1024

/* global variable used to alternatively enable/disable multithreading */
int multithreading = 1;

static int
execute_check (sqlite3 * sqlite, const char *sql)
{
/* executing an SQL statement returning True/False */
    sqlite3_stmt *stmt;
    int ret;
    int retcode = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return SQLITE_ERROR;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    if (retcode == 1)
	return SQLITE_OK;
    return SQLITE_ERROR;
}

static int
do_export_ascii (sqlite3 * sqlite, const char *coverage, gaiaGeomCollPtr geom,
		 int scale)
{
/* exporting an ASCII Grid */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    double res = 1.0 * (double) scale;

    path = sqlite3_mprintf ("./%s_%d.asc", coverage, scale);

    sql = "SELECT RL2_WriteAsciiGrid('main', ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, 1024);
    sqlite3_bind_int (stmt, 4, 1024);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 5, blob, blob_size, free);
    sqlite3_bind_double (stmt, 6, res);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_int (stmt, 8, 2);
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
    return retcode;
}

static int
do_export_section_ascii (sqlite3 * sqlite, const char *coverage,
			 gaiaGeomCollPtr geom, int scale)
{
/* exporting an ASCII Grid */
    char *sql;
    char *path;
    sqlite3_stmt *stmt;
    int ret;
    unsigned char *blob;
    int blob_size;
    int retcode = 0;
    double res = 1.0 * (double) scale;

    path = sqlite3_mprintf ("./%s_sect1_%d.asc", coverage, scale);

    sql = "SELECT RL2_WriteSectionAsciiGrid('main', ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, 1);
    sqlite3_bind_text (stmt, 3, path, strlen (path), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, 1024);
    sqlite3_bind_int (stmt, 5, 1024);
    gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, free);
    sqlite3_bind_double (stmt, 7, res);
    sqlite3_bind_int (stmt, 8, 1);
    sqlite3_bind_int (stmt, 9, 2);
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
    return retcode;
}

static gaiaGeomCollPtr
get_center_point (sqlite3 * sqlite, const char *coverage)
{
/* attempting to retrieve the Coverage's Center Point */
    char *sql;
    sqlite3_stmt *stmt;
    gaiaGeomCollPtr geom = NULL;
    int ret;

    sql = sqlite3_mprintf ("SELECT MakePoint("
			   "extent_minx + ((extent_maxx - extent_minx) / 2.0), "
			   "extent_miny + ((extent_maxy - extent_miny) / 2.0)) "
			   "FROM raster_coverages WHERE coverage_name = %Q",
			   coverage);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return NULL;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		int blob_sz = sqlite3_column_bytes (stmt, 0);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
	    }
      }
    sqlite3_finalize (stmt);
    return geom;
}

static int
test_histogram (sqlite3 * sqlite, const char *coverage)
{
/* testing Band Histogram */
    int ret = 0;
    char *path = sqlite3_mprintf ("histogram_%s.png", coverage);
    char *sql =
	sqlite3_mprintf
	("SELECT BlobToFile(RL2_GetBandStatistics_Histogram(statistics, 0), %Q) "
	 "FROM raster_coverages WHERE Lower(coverage_name) = Lower(%Q)", path,
	 coverage);
    if (execute_check (sqlite, sql) == SQLITE_OK)
	ret = 1;
    sqlite3_free (sql);
    unlink (path);
    sqlite3_free (path);
    return ret;
}

static int
test_statistics (sqlite3 * sqlite, const char *coverage, int *retcode)
{
/* testing Coverage and Band statistics */
    int ret;
    char *err_msg = NULL;
    char *sql;
    char **results;
    int rows;
    int columns;
    const char *string;
    int intval;

/* testing RasterStatistics */
    sql =
	sqlite3_mprintf
	("SELECT RL2_GetRasterStatistics_NoDataPixelsCount(statistics), "
	 "RL2_GetRasterStatistics_ValidPixelsCount(statistics), "
	 "RL2_GetRasterStatistics_SampleType(statistics), "
	 "RL2_GetRasterStatistics_BandsCount(statistics) "
	 "FROM raster_coverages WHERE Lower(coverage_name) = Lower(%Q)",
	 coverage);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (rows != 1 || columns != 4)
      {
	  fprintf (stderr, "Unexpected error: bad result: %i/%i.\n", rows,
		   columns);
	  *retcode += -2;
	  return 0;
      }

/* NoData Pixels */
    string = results[4];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (NoDataPixelsCount)\n");
	  *retcode += -3;
	  return 0;
      }

/* Valid Pixels */
    string = results[5];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (ValidPixelsCount)\n");
	  *retcode += -5;
	  return 0;
      }

/* Sample Type */
    string = results[6];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (SampleType)\n");
	  *retcode += -7;
	  return 0;
      }

/* Bands */
    string = results[7];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (BandsCount)\n");
	  *retcode += -9;
	  return 0;
      }
    intval = atoi (string);
    if (intval != 1)
      {
	  fprintf (stderr, "Unexpected BandsCount: %d\n", intval);
	  *retcode += -10;
	  return 0;
      }

    sqlite3_free_table (results);

/* testing BandStatistics */
    sql =
	sqlite3_mprintf
	("SELECT RL2_GetBandStatistics_Min(statistics, 0), "
	 "RL2_GetBandStatistics_Max(statistics, 0), "
	 "RL2_GetBandStatistics_Avg(statistics, 0), "
	 "RL2_GetBandStatistics_Var(statistics, 0), "
	 "RL2_GetBandStatistics_StdDev(statistics, 0) "
	 "FROM raster_coverages WHERE Lower(coverage_name) = Lower(%Q)",
	 coverage);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }
    if (rows != 1 || columns != 5)
      {
	  fprintf (stderr, "Unexpected error: bad result: %i/%i.\n", rows,
		   columns);
	  *retcode += -2;
	  return 0;
      }

/* Min */
    string = results[5];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band Min)\n");
	  *retcode += -12;
	  return 0;
      }

/* Max */
    string = results[6];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band Max)\n");
	  *retcode += -14;
	  return 0;
      }

/* Avg */
    string = results[7];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band Avg)\n");
	  *retcode += -16;
	  return 0;
      }

/* Var */
    string = results[8];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band Var)\n");
	  *retcode += -18;
	  return 0;
      }

/* StdDev */
    string = results[9];
    if (string == NULL)
      {
	  fprintf (stderr, "Unexpected NULL (Band StdDev)\n");
	  *retcode += -20;
	  return 0;
      }

    sqlite3_free_table (results);

/* Histogram */
    if (test_histogram (sqlite, coverage) != 1)
      {
	  fprintf (stderr, "Unable to create an Histogram \"%s\"\n", coverage);
	  *retcode += 21;
	  return 0;
      }

    return 1;
}

static int
test_coverage (sqlite3 * sqlite, unsigned char sample,
	       unsigned char compression, int tile_sz, int *retcode)
{
/* testing some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage = NULL;
    const char *sample_name = NULL;
    const char *pixel_name = NULL;
    unsigned char num_bands = 1;
    const char *compression_name = NULL;
    int qlty;
    char *sql;
    int tile_size = 256;
    gaiaGeomCollPtr geom;

/* setting the coverage name */
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_8_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_8_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_8_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_8_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_8_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_8_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT8:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u8_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u8_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u8_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u8_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u8_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u8_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_INT16:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_16_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_16_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_16_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_16_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_16_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_16_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT16:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u16_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u16_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u16_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u16_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u16_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u16_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_INT32:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_32_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_32_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_32_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_32_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_32_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_32_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT32:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u32_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u32_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u32_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u32_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u32_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u32_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_FLOAT:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_flt_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_flt_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_flt_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_flt_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_flt_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_flt_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_DOUBLE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_dbl_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_dbl_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_dbl_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_dbl_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_dbl_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_dbl_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      };

/* preparing misc Coverage's parameters */
    pixel_name = "DATAGRID";
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  sample_name = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  sample_name = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  sample_name = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  sample_name = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  sample_name = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  sample_name = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  sample_name = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample_name = "DOUBLE";
	  break;
      };
    num_bands = 1;
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  compression_name = "NONE";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_DEFLATE_NO:
	  compression_name = "DEFLATE_NO";
	  qlty = 100;
	  break;
      case RL2_COMPRESSION_LZMA_NO:
	  compression_name = "LZMA_NO";
	  qlty = 100;
	  break;
      };
    switch (tile_sz)
      {
      case TILE_256:
	  tile_size = 256;
	  break;
      case TILE_1024:
	  tile_size = 1024;
	  break;
      };

/* setting the MultiThreading mode alternatively on/off */
    if (multithreading)
      {
	  sql = "SELECT RL2_SetMaxThreads(2)";
	  multithreading = 0;
      }
    else
      {
	  sql = "SELECT RL2_SetMaxThreads(1)";
	  multithreading = 1;
      }
    execute_check (sqlite, sql);

/* creating the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateRasterCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.8f, %1.8f)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 3003,
			   1.0, 1.0);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

/* loading from directory */
    sql =
	sqlite3_mprintf
	("SELECT RL2_LoadRastersFromDir(%Q, %Q, %Q, 0, 3003, 1)", coverage,
	 "map_samples/ascii", ".asc");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRastersFromDir \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -2;
	  return 0;
      }

/* deleting the first section */
    sql = sqlite3_mprintf ("SELECT RL2_DeleteSection(%Q, 1, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DeleteSection \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -3;
	  return 0;
      }

/* re-loading yet again the first section */
    sql = sqlite3_mprintf ("SELECT RL2_LoadRaster(%Q, %Q, 0, 3003, 1)",
			   coverage, "map_samples/ascii/ascii1.asc");
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LoadRaster \"%s\" error: %s\n", coverage, err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -4;
	  return 0;
      }

/* export tests */
    geom = get_center_point (sqlite, coverage);
    if (geom == NULL)
      {
	  *retcode += -5;
	  return 0;
      }
    if (!do_export_ascii (sqlite, coverage, geom, 1))
      {
	  *retcode += -6;
	  return 0;
      }
    if (!do_export_ascii (sqlite, coverage, geom, 2))
      {
	  *retcode += -7;
	  return 0;
      }
    if (!do_export_ascii (sqlite, coverage, geom, 4))
      {
	  *retcode += -8;
	  return 0;
      }
    if (!do_export_ascii (sqlite, coverage, geom, 8))
      {
	  *retcode += -9;
	  return 0;
      }
    if (!do_export_section_ascii (sqlite, coverage, geom, 1))
      {
	  *retcode += -10;
	  return 0;
      }
    if (!do_export_section_ascii (sqlite, coverage, geom, 2))
      {
	  *retcode += -11;
	  return 0;
      }
    if (!do_export_section_ascii (sqlite, coverage, geom, 4))
      {
	  *retcode += -12;
	  return 0;
      }
    if (!do_export_section_ascii (sqlite, coverage, geom, 8))
      {
	  *retcode += -13;
	  return 0;
      }
    gaiaFreeGeomColl (geom);

    *retcode += -14;
    if (!test_statistics (sqlite, coverage, retcode))
	return 0;

    if (compression == RL2_COMPRESSION_DEFLATE_NO)
      {
	  /* testing a Monolithic Pyramid */
	  sql =
	      sqlite3_mprintf ("SELECT RL2_PyramidizeMonolithic(%Q, 1, 1)",
			       coverage);
	  ret = execute_check (sqlite, sql);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "PyramidizeMonolithic \"%s\" error: %s\n",
			 coverage, err_msg);
		sqlite3_free (err_msg);
		*retcode += -15;
		return 0;
	    }
      }

    return 1;
}

static int
drop_coverage (sqlite3 * sqlite, unsigned char sample,
	       unsigned char compression, int tile_sz, int *retcode)
{
/* dropping some DBMS Coverage */
    int ret;
    char *err_msg = NULL;
    const char *coverage;
    char *sql;

/* setting the coverage name */
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_8_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_8_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_8_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_8_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_8_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_8_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT8:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u8_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u8_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u8_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u8_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u8_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u8_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_INT16:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_16_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_16_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_16_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_16_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_16_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_16_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT16:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u16_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u16_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u16_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u16_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u16_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u16_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_INT32:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_32_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_32_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_32_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_32_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_32_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_32_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_UINT32:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u32_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u32_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u32_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u32_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_u32_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_u32_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_FLOAT:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_flt_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_flt_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_flt_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_flt_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_flt_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_flt_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      case RL2_SAMPLE_DOUBLE:
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_dbl_none_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_dbl_none_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_DEFLATE_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_dbl_deflate_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_dbl_deflate_1024";
		      break;
		  };
		break;
	    case RL2_COMPRESSION_LZMA_NO:
		switch (tile_sz)
		  {
		  case TILE_256:
		      coverage = "grid_dbl_lzma_256";
		      break;
		  case TILE_1024:
		      coverage = "grid_dbl_lzma_1024";
		      break;
		  };
		break;
	    };
	  break;
      };

/* dropping the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_DropRasterCoverage(%Q, 1)", coverage);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropRasterCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

    return 1;
}

int
main (int argc, char *argv[])
{
    rl2AsciiGridDestinationPtr ascii;
    unsigned char *pixels;
    const char *path;
    unsigned int width;
    unsigned int height;
    double x;
    double y;
    double res;
    int result = 0;
    int ret;
    char *err_msg = NULL;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);
    ret =
	sqlite3_exec (db_handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -2;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT CreateRasterCoveragesTable()", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoveragesTable() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -3;
      }

/* ASCII Grid tests */
    ret = -100;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_INT8, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -120;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_INT8, RL2_COMPRESSION_DEFLATE_NO, TILE_1024,
	 &ret))
	return ret;
    ret = -150;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_UINT8, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -170;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_UINT8, RL2_COMPRESSION_DEFLATE_NO, TILE_1024,
	 &ret))
	return ret;
    ret = -200;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_INT16, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;
    ret = -220;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_INT16, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -250;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_UINT16, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;

#ifndef OMIT_LZMA		/* only if LZMA is enabled */
    ret = -270;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_UINT16, RL2_COMPRESSION_LZMA_NO, TILE_1024,
	 &ret))
	return ret;
#endif

    ret = -300;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_INT32, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -320;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_INT32, RL2_COMPRESSION_DEFLATE_NO, TILE_1024,
	 &ret))
	return ret;
    ret = -350;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_UINT32, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;
    ret = -370;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_UINT32, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -400;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_FLOAT, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;

#ifndef OMIT_LZMA		/* only if LZMA is enabled */
    ret = -420;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_FLOAT, RL2_COMPRESSION_LZMA_NO, TILE_1024, &ret))
	return ret;
#endif

    ret = -450;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_DOUBLE, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;
    ret = -470;
    if (!test_coverage
	(db_handle, RL2_SAMPLE_DOUBLE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;

/* dropping all ASCII Grid Coverages */
    ret = -110;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_INT8, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -130;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_INT8, RL2_COMPRESSION_DEFLATE_NO, TILE_1024,
	 &ret))
	return ret;
    ret = -160;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_UINT8, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -180;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_UINT8, RL2_COMPRESSION_DEFLATE_NO, TILE_1024,
	 &ret))
	return ret;
    ret = -210;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_INT16, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;
    ret = -230;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_INT16, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -260;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_UINT16, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;

#ifndef OMIT_LZMA		/* only if LZMA is enabled */
    ret = -280;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_UINT16, RL2_COMPRESSION_LZMA_NO, TILE_1024,
	 &ret))
	return ret;
#endif

    ret = -310;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_INT32, RL2_COMPRESSION_NONE, TILE_256, &ret))
	return ret;
    ret = -330;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_INT32, RL2_COMPRESSION_DEFLATE_NO, TILE_1024,
	 &ret))
	return ret;
    ret = -360;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_UINT32, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;
    ret = -380;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_UINT32, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;
    ret = -410;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_FLOAT, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;

#ifndef OMIT_LZMA		/* only if LZMA is enabled */
    ret = -430;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_FLOAT, RL2_COMPRESSION_LZMA_NO, TILE_1024, &ret))
	return ret;
#endif

    ret = -460;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_DOUBLE, RL2_COMPRESSION_DEFLATE_NO, TILE_256,
	 &ret))
	return ret;
    ret = -480;
    if (!drop_coverage
	(db_handle, RL2_SAMPLE_DOUBLE, RL2_COMPRESSION_NONE, TILE_1024, &ret))
	return ret;

/* testing an ASCII destination */
    pixels = malloc (100 * 200);
    memset (pixels, 0, 100 * 200);
    ascii =
	rl2_create_ascii_grid_destination ("test_ascii.asc", 100, 200, 1.5,
					   10.0, 20.0, 1, 0, 2, pixels,
					   100 * 200, RL2_SAMPLE_UINT8);
    if (ascii == NULL)
      {
	  fprintf (stderr,
		   "ERROR: unable to create an ASCII Grid destination\n");
	  return 500;
      }
    path = rl2_get_ascii_grid_destination_path (ascii);
    if (path == NULL)
      {
	  fprintf (stderr, "ERROR: unexpected NULL ASCII Grid path\n");
	  return 501;
      }
    if (strcmp (path, "test_ascii.asc") != 0)
      {
	  fprintf (stderr, "ERROR: unexpected ASCII Grid path: %s\n", path);
	  return 502;
      }
    if (rl2_get_ascii_grid_destination_size (ascii, &width, &height) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get ASCII Grid dimensions\n");
	  return 503;
      }
    if (width != 100)
      {
	  fprintf (stderr, "ERROR: unexpected ASCII Grid Width: %u\n", width);
	  return 504;
      }
    if (height != 200)
      {
	  fprintf (stderr, "ERROR: unexpected ASCII Grid Height: %u\n", height);
	  return 505;
      }
    if (rl2_get_ascii_grid_destination_tiepoint (ascii, &x, &y) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get ASCII Grid tiepoint\n");
	  return 506;
      }
    if (x != 10.0)
      {
	  fprintf (stderr, "ERROR: unexpected ASCII Grid X tiepoint: %1.2f\n",
		   x);
	  return 507;
      }
    if (y != 20.0)
      {
	  fprintf (stderr, "ERROR: unexpected ASCII Grid Y tiepoint: %1.2f\n",
		   y);
	  return 508;
      }
    if (rl2_get_ascii_grid_destination_resolution (ascii, &res) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get ASCII Grid resolution\n");
	  return 509;
      }
    if (res != 1.5)
      {
	  fprintf (stderr, "ERROR: unexpected ASCII Grid resolution: %1.2f\n",
		   res);
	  return 510;
      }
    rl2_destroy_ascii_grid_destination (ascii);
    unlink ("test_ascii.asc");

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
}
