/*

 rl2sql -- main SQLite extension methods

 version 0.1, 2013 December 22

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
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

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

#include <spatialite/gaiaaux.h>

#define RL2_UNUSED() if (argc || argv) argc = argc;

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%I64d\n"
#else
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%lld\n"
#endif

static void
fnct_rl2_version (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ rl2_version()
/
/ return a text string representing the current RasterLite-2 version
*/
    int len;
    const char *p_result = rl2_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_target_cpu (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ rl2_target_cpu()
/
/ return a text string representing the current RasterLite-2 Target CPU
*/
    int len;
    const char *p_result = rl2_target_cpu ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_IsValidPixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsValidPixel(BLOBencoded pixel, text sample_type, int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *sample;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	sqlite3_result_int (context, -1);
    else
      {
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  bands = sqlite3_value_int (argv[2]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (bands > 0 && bands < 256)
	      num_bands = bands;
	  if (sample_type == RL2_SAMPLE_UNKNOWN
	      || num_bands == RL2_BANDS_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
	  ret = rl2_is_valid_dbms_pixel (blob, blob_sz, sample_type, num_bands);
	  if (ret == RL2_OK)
	      sqlite3_result_int (context, 1);
	  else
	      sqlite3_result_int (context, 0);
      }
}

static void
fnct_IsValidRasterPalette (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterPalette(BLOBencoded palette, text sample_type)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *sample;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (err)
	sqlite3_result_int (context, -1);
    else
      {
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (sample_type == RL2_SAMPLE_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
	  ret = rl2_is_valid_dbms_palette (blob, blob_sz, sample_type);
	  if (ret == RL2_OK)
	      sqlite3_result_int (context, 1);
	  else
	      sqlite3_result_int (context, 0);
      }
}

static void
fnct_IsValidRasterStatistics (sqlite3_context * context, int argc,
			      sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterStatistics(text covarage, BLOBencoded statistics)
/   or
/ IsValidRasterStatistics((BLOBencoded statistics, text sample_type, int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *coverage;
    const char *sample;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    sqlite3 *sqlite;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (argc == 3)
      {
	  if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (err)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  sample = (const char *) sqlite3_value_text (argv[1]);
	  bands = sqlite3_value_int (argv[2]);
	  if (strcmp (sample, "1-BIT") == 0)
	      sample_type = RL2_SAMPLE_1_BIT;
	  if (strcmp (sample, "2-BIT") == 0)
	      sample_type = RL2_SAMPLE_2_BIT;
	  if (strcmp (sample, "4-BIT") == 0)
	      sample_type = RL2_SAMPLE_4_BIT;
	  if (strcmp (sample, "INT8") == 0)
	      sample_type = RL2_SAMPLE_INT8;
	  if (strcmp (sample, "UINT8") == 0)
	      sample_type = RL2_SAMPLE_UINT8;
	  if (strcmp (sample, "INT16") == 0)
	      sample_type = RL2_SAMPLE_INT16;
	  if (strcmp (sample, "UINT16") == 0)
	      sample_type = RL2_SAMPLE_UINT16;
	  if (strcmp (sample, "INT32") == 0)
	      sample_type = RL2_SAMPLE_INT32;
	  if (strcmp (sample, "UINT32") == 0)
	      sample_type = RL2_SAMPLE_UINT32;
	  if (strcmp (sample, "FLOAT") == 0)
	      sample_type = RL2_SAMPLE_FLOAT;
	  if (strcmp (sample, "DOUBLE") == 0)
	      sample_type = RL2_SAMPLE_DOUBLE;
	  if (bands > 0 && bands < 256)
	      num_bands = bands;
	  if (sample_type == RL2_SAMPLE_UNKNOWN
	      || num_bands == RL2_BANDS_UNKNOWN)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
      }
    else
      {
	  if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	      err = 1;
	  if (err)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
	  sqlite = sqlite3_context_db_handle (context);
	  coverage = (const char *) sqlite3_value_text (argv[0]);
	  blob = sqlite3_value_blob (argv[1]);
	  blob_sz = sqlite3_value_bytes (argv[1]);
	  if (!get_coverage_sample_bands
	      (sqlite, coverage, &sample_type, &num_bands))
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret =
	rl2_is_valid_dbms_raster_statistics (blob, blob_sz, sample_type,
					     num_bands);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static void
fnct_IsValidRasterTile (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ IsValidRasterTile(text coverage, integer level, BLOBencoded tile_odd,
/   BLOBencoded tile_even)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    int level;
    const unsigned char *blob_odd;
    int blob_odd_sz;
    const unsigned char *blob_even;
    int blob_even_sz;
    const char *coverage;
    unsigned short tile_width;
    unsigned short tile_height;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    unsigned char compression = RL2_COMPRESSION_UNKNOWN;
    sqlite3 *sqlite;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_BLOB
	&& sqlite3_value_type (argv[3]) != SQLITE_NULL)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    sqlite = sqlite3_context_db_handle (context);
    coverage = (const char *) sqlite3_value_text (argv[0]);
    level = sqlite3_value_int (argv[1]);
    blob_odd = sqlite3_value_blob (argv[2]);
    blob_odd_sz = sqlite3_value_bytes (argv[2]);
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
      {
	  blob_even = NULL;
	  blob_even_sz = 0;
      }
    else
      {
	  blob_even = sqlite3_value_blob (argv[3]);
	  blob_even_sz = sqlite3_value_bytes (argv[3]);
      }
    if (!get_coverage_defs
	(sqlite, coverage, &tile_width, &tile_height, &sample_type, &pixel_type,
	 &num_bands, &compression))
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    ret =
	rl2_is_valid_dbms_raster_tile (level, tile_width, tile_height, blob_odd,
				       blob_odd_sz, blob_even, blob_even_sz,
				       sample_type, pixel_type, num_bands,
				       compression);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static void
fnct_GetPaletteNumEntries (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ GetPaletteNumEntries(BLOB pixel_obj)
/
/ will return the number of Color Entries
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PalettePtr plt = NULL;
    rl2PrivPalettePtr palette;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    plt = rl2_deserialize_dbms_palette (blob, blob_sz);
    if (plt == NULL)
	goto error;
    palette = (rl2PrivPalettePtr) plt;
    sqlite3_result_int (context, palette->nEntries);
    rl2_destroy_palette (plt);
    return;

  error:
    sqlite3_result_null (context);
    if (plt != NULL)
	rl2_destroy_palette (plt);
}

static void
fnct_GetPaletteColorEntry (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ GetPaletteColorEntry(BLOB pixel_obj, INT index)
/
/ will return the corresponding Color entry
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PalettePtr plt = NULL;
    rl2PrivPalettePtr palette;
    rl2PrivPaletteEntryPtr entry;
    int entry_id;
    char color[16];
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    entry_id = sqlite3_value_int (argv[1]);
    plt = rl2_deserialize_dbms_palette (blob, blob_sz);
    if (plt == NULL)
	goto error;
    palette = (rl2PrivPalettePtr) plt;
    if (entry_id >= 0 && entry_id < palette->nEntries)
	;
    else
	goto error;
    entry = palette->entries + entry_id;
    sprintf (color, "#%02x%02x%02x", entry->red, entry->green, entry->blue);
    sqlite3_result_text (context, color, strlen (color), SQLITE_TRANSIENT);
    rl2_destroy_palette (plt);
    return;

  error:
    sqlite3_result_null (context);
    if (plt != NULL)
	rl2_destroy_palette (plt);
}

static void
fnct_SetPaletteColorEntry (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ SetPaletteColorEntry(BLOB pixel_obj, INT index, TEXT color)
/
/ will return a new palette including the changed color
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    unsigned char *blb;
    int blob_sz = 0;
    rl2PalettePtr plt = NULL;
    rl2PrivPalettePtr palette;
    rl2PrivPaletteEntryPtr entry;
    int entry_id;
    const char *color;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	goto error;
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    entry_id = sqlite3_value_int (argv[1]);
    color = (const char *) sqlite3_value_text (argv[2]);
/* parsing the background color */
    if (rl2_parse_hexrgb (color, &red, &green, &blue) != RL2_OK)
	goto error;
    plt = rl2_deserialize_dbms_palette (blob, blob_sz);
    if (plt == NULL)
	goto error;
    palette = (rl2PrivPalettePtr) plt;
    if (entry_id >= 0 && entry_id < palette->nEntries)
	;
    else
	goto error;
    entry = palette->entries + entry_id;
    entry->red = red;
    entry->green = green;
    entry->blue = blue;
    rl2_serialize_dbms_palette (plt, &blb, &blob_sz);
    sqlite3_result_blob (context, blb, blob_sz, free);
    rl2_destroy_palette (plt);
    return;

  error:
    sqlite3_result_null (context);
    if (plt != NULL)
	rl2_destroy_palette (plt);
}

static void
fnct_PaletteEquals (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ PaletteEquals(BLOB palette_obj1, BLOB palette_obj2)
/
/ 1 (TRUE) or 0 (FALSE)
/ or -1 on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PalettePtr plt1 = NULL;
    rl2PalettePtr plt2 = NULL;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    plt1 = rl2_deserialize_dbms_palette (blob, blob_sz);
    if (plt1 == NULL)
	goto error;
    blob = sqlite3_value_blob (argv[1]);
    blob_sz = sqlite3_value_bytes (argv[1]);
    plt2 = rl2_deserialize_dbms_palette (blob, blob_sz);
    if (plt2 == NULL)
	goto error;
    ret = rl2_compare_palettes (plt1, plt2);
    if (ret == RL2_TRUE)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    rl2_destroy_palette (plt1);
    rl2_destroy_palette (plt2);
    return;

  error:
    sqlite3_result_int (context, -1);
    if (plt1 != NULL)
	rl2_destroy_palette (plt1);
    if (plt2 != NULL)
	rl2_destroy_palette (plt2);
}

static void
fnct_CreatePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ CreatePixel(text sample_type, text pixel_type, int num_bands)
/
/ will return a serialized binary Pixel Object 
/ or NULL on failure
*/
    int err = 0;
    const char *sample_type;
    const char *pixel_type;
    int num_bands;
    unsigned char sample;
    unsigned char pixel;
    rl2PixelPtr pxl = NULL;
    unsigned char *blob = NULL;
    int blob_sz = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	goto error;

/* retrieving the arguments */
    sample_type = (const char *) sqlite3_value_text (argv[0]);
    pixel_type = (const char *) sqlite3_value_text (argv[1]);
    num_bands = sqlite3_value_int (argv[2]);

/* preliminary arg checking */
    if (num_bands < 1 || num_bands > 255)
	goto error;

    sample = RL2_SAMPLE_UNKNOWN;
    if (strcasecmp (sample_type, "1-BIT") == 0)
	sample = RL2_SAMPLE_1_BIT;
    if (strcasecmp (sample_type, "2-BIT") == 0)
	sample = RL2_SAMPLE_2_BIT;
    if (strcasecmp (sample_type, "4-BIT") == 0)
	sample = RL2_SAMPLE_4_BIT;
    if (strcasecmp (sample_type, "INT8") == 0)
	sample = RL2_SAMPLE_INT8;
    if (strcasecmp (sample_type, "UINT8") == 0)
	sample = RL2_SAMPLE_UINT8;
    if (strcasecmp (sample_type, "INT16") == 0)
	sample = RL2_SAMPLE_INT16;
    if (strcasecmp (sample_type, "UINT16") == 0)
	sample = RL2_SAMPLE_UINT16;
    if (strcasecmp (sample_type, "INT32") == 0)
	sample = RL2_SAMPLE_INT32;
    if (strcasecmp (sample_type, "UINT32") == 0)
	sample = RL2_SAMPLE_UINT32;
    if (strcasecmp (sample_type, "FLOAT") == 0)
	sample = RL2_SAMPLE_FLOAT;
    if (strcasecmp (sample_type, "DOUBLE") == 0)
	sample = RL2_SAMPLE_DOUBLE;

    pixel = RL2_PIXEL_UNKNOWN;
    if (strcasecmp (pixel_type, "MONOCHROME") == 0)
	pixel = RL2_PIXEL_MONOCHROME;
    if (strcasecmp (pixel_type, "GRAYSCALE") == 0)
	pixel = RL2_PIXEL_GRAYSCALE;
    if (strcasecmp (pixel_type, "PALETTE") == 0)
	pixel = RL2_PIXEL_PALETTE;
    if (strcasecmp (pixel_type, "RGB") == 0)
	pixel = RL2_PIXEL_RGB;
    if (strcasecmp (pixel_type, "DATAGRID") == 0)
	pixel = RL2_PIXEL_DATAGRID;
    if (strcasecmp (pixel_type, "MULTIBAND") == 0)
	pixel = RL2_PIXEL_MULTIBAND;

/* attempting to create the Pixel */
    pxl = rl2_create_pixel (sample, pixel, (unsigned char) num_bands);
    if (pxl == NULL)
	goto error;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelSampleType (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelSampleType(BLOB pixel_obj)
/
/ will return one of "1-BIT", "2-BIT", "4-BIT", "INT8", "UINT8", "INT16",
/      "UINT16", "INT32", "UINT32", "FLOAT", "DOUBLE", "UNKNOWN" 
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    const char *text;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
	  text = "1-BIT";
	  break;
      case RL2_SAMPLE_2_BIT:
	  text = "2-BIT";
	  break;
      case RL2_SAMPLE_4_BIT:
	  text = "4-BIT";
	  break;
      case RL2_SAMPLE_INT8:
	  text = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  text = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  text = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  text = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  text = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  text = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  text = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  text = "DOUBLE";
	  break;
      default:
	  text = "UNKNOWN";
	  break;
      };
    sqlite3_result_text (context, text, strlen (text), SQLITE_TRANSIENT);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelType (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelType(BLOB pixel_obj)
/
/ will return one of "MONOCHROME" "PALETTE", "GRAYSCALE", "RGB",
/      "DATAGRID", "MULTIBAND", "UNKNOWN" 
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    const char *text;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    switch (pixel->pixelType)
      {
      case RL2_PIXEL_MONOCHROME:
	  text = "MONOCHROME";
	  break;
      case RL2_PIXEL_PALETTE:
	  text = "PALETTE";
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  text = "GRAYSCALE";
	  break;
      case RL2_PIXEL_RGB:
	  text = "RGB";
	  break;
      case RL2_PIXEL_DATAGRID:
	  text = "DATAGRID";
	  break;
      case RL2_PIXEL_MULTIBAND:
	  text = "MULTIBAND";
	  break;
      default:
	  text = "UNKNOWN";
	  break;
      };
    sqlite3_result_text (context, text, strlen (text), SQLITE_TRANSIENT);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelNumBands (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelNumBands(BLOB pixel_obj)
/
/ will return the number of Bands
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    sqlite3_result_int (context, pixel->nBands);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_GetPixelValue (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelValue(BLOB pixel_obj, INT band)
/
/ will return the corresponding Band value
/ or NULL on failure
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    rl2PrivSamplePtr sample;
    int band_id;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_id = sqlite3_value_int (argv[1]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (band_id >= 0 && band_id < pixel->nBands)
	;
    else
	goto error;
    sample = pixel->Samples + band_id;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  sqlite3_result_int (context, sample->int8);
	  break;
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  sqlite3_result_int (context, sample->uint8);
	  break;
      case RL2_SAMPLE_INT16:
	  sqlite3_result_int (context, sample->int16);
	  break;
      case RL2_SAMPLE_UINT16:
	  sqlite3_result_int (context, sample->uint16);
	  break;
      case RL2_SAMPLE_INT32:
	  sqlite3_result_int64 (context, sample->int32);
	  break;
      case RL2_SAMPLE_UINT32:
	  sqlite3_result_int64 (context, sample->uint32);
	  break;
      case RL2_SAMPLE_FLOAT:
	  sqlite3_result_double (context, sample->float32);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sqlite3_result_double (context, sample->float64);
	  break;
      default:
	  goto error;
      };
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetPixelValue (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ SetPixelValue(BLOB pixel_obj, INT band, INT value)
/ SetPixelValue(BLOB pixel_obj, INT band, FLOAT value)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    rl2PrivSamplePtr sample;
    int band_id;
    int intval;
    double dblval;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	goto error;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[2]) != SQLITE_FLOAT)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_id = sqlite3_value_int (argv[1]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (band_id >= 0 && band_id < pixel->nBands)
	;
    else
	goto error;
    if (pixel->sampleType == RL2_SAMPLE_FLOAT
	|| pixel->sampleType == RL2_SAMPLE_DOUBLE)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      dblval = sqlite3_value_double (argv[2]);
	  else
	    {
		intval = sqlite3_value_int (argv[2]);
		dblval = intval;
	    }
      }
    else
      {
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      goto error;
	  intval = sqlite3_value_int (argv[2]);
      }
    sample = pixel->Samples + band_id;
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  if (intval < INT8_MIN || intval > INT8_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_1_BIT:
	  if (intval < 0 || intval > 1)
	      goto error;
	  break;
      case RL2_SAMPLE_2_BIT:
	  if (intval < 0 || intval > 3)
	      goto error;
	  break;
      case RL2_SAMPLE_4_BIT:
	  if (intval < 0 || intval > 15)
	      goto error;
	  break;
      case RL2_SAMPLE_UINT8:
	  if (intval < 0 || intval > UINT8_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_INT16:
	  if (intval < INT16_MIN || intval > INT16_MAX)
	      goto error;
	  break;
      case RL2_SAMPLE_UINT16:
	  if (intval < 0 || intval > UINT16_MAX)
	      goto error;
	  break;
      };
    switch (pixel->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  sample->int8 = intval;
	  break;
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  sample->uint8 = intval;
	  break;
      case RL2_SAMPLE_INT16:
	  sample->int16 = intval;
	  break;
      case RL2_SAMPLE_UINT16:
	  sample->uint16 = intval;
	  break;
      case RL2_SAMPLE_INT32:
	  sample->int32 = intval;
	  break;
      case RL2_SAMPLE_UINT32:
	  sample->uint32 = (unsigned int) intval;
	  break;
      case RL2_SAMPLE_FLOAT:
	  sample->float32 = dblval;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample->float64 = dblval;
	  break;
      default:
	  goto error;
      };
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_IsTransparentPixel (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ IsTransparentPixel(BLOB pixel_obj)
/
/ 1 (TRUE) or 0 (FALSE)
/ or -1 on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (pixel->isTransparent == 0)
	sqlite3_result_int (context, 0);
    else
	sqlite3_result_int (context, 1);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_int (context, -1);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_IsOpaquePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsOpaquePixel(BLOB pixel_obj)
/
/ 1 (TRUE) or 0 (FALSE)
/ or -1 on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    if (pixel->isTransparent == 0)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_int (context, -1);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetTransparentPixel (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ SetTransparentPixel(BLOB pixel_obj)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    pixel->isTransparent = 1;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_SetOpaquePixel (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ SetOpaquePixel(BLOB pixel_obj)
/
/ will return a new serialized Pixel Object
/ or NULL on failure
*/
    unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl = NULL;
    rl2PrivPixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;

    blob = (unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl == NULL)
	goto error;
    pixel = (rl2PrivPixelPtr) pxl;
    pixel->isTransparent = 0;
    if (rl2_serialize_dbms_pixel (pxl, &blob, &blob_sz) != RL2_OK)
	goto error;
    sqlite3_result_blob (context, blob, blob_sz, free);
    rl2_destroy_pixel (pxl);
    return;

  error:
    sqlite3_result_null (context);
    if (pxl != NULL)
	rl2_destroy_pixel (pxl);
}

static void
fnct_PixelEquals (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ PixelEquals(BLOB pixel_obj1, BLOB pixel_obj2)
/
/ 1 (TRUE) or 0 (FALSE)
/ or -1 on invalid argument
*/
    const unsigned char *blob = NULL;
    int blob_sz = 0;
    rl2PixelPtr pxl1 = NULL;
    rl2PixelPtr pxl2 = NULL;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	goto error;
    if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	goto error;

    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    pxl1 = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl1 == NULL)
	goto error;
    blob = sqlite3_value_blob (argv[1]);
    blob_sz = sqlite3_value_bytes (argv[1]);
    pxl2 = rl2_deserialize_dbms_pixel (blob, blob_sz);
    if (pxl2 == NULL)
	goto error;
    ret = rl2_compare_pixels (pxl1, pxl2);
    if (ret == RL2_TRUE)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    rl2_destroy_pixel (pxl1);
    rl2_destroy_pixel (pxl2);
    return;

  error:
    sqlite3_result_int (context, -1);
    if (pxl1 != NULL)
	rl2_destroy_pixel (pxl1);
    if (pxl2 != NULL)
	rl2_destroy_pixel (pxl2);
}

static void
fnct_GetRasterStatistics_NoDataPixelsCount (sqlite3_context * context, int argc,
					    sqlite3_value ** argv)
{
/* SQL function:
/ GetRasterStatistics_NoDataPixelsCount(BLOBencoded statistics)
/
/ will return the total count of NoData pixels
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    sqlite3_result_int64 (context, st->no_data);
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetRasterStatistics_ValidPixelsCount (sqlite3_context * context, int argc,
					   sqlite3_value ** argv)
{
/* SQL function:
/ GetRasterStatistics_ValidPixelsCount(BLOBencoded statistics)
/
/ will return the total count of valid pixels
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    sqlite3_result_int64 (context, st->count);
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetRasterStatistics_SampleType (sqlite3_context * context, int argc,
				     sqlite3_value ** argv)
{
/* SQL function:
/ GetRasterStatistics_SampleType(BLOBencoded statistics)
/
/ will return the name of the SampleType
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    const char *name = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    switch (st->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
	  name = "1-BIT";
	  break;
      case RL2_SAMPLE_2_BIT:
	  name = "2-BIT";
	  break;
      case RL2_SAMPLE_4_BIT:
	  name = "4-BIT";
	  break;
      case RL2_SAMPLE_INT8:
	  name = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  name = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  name = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  name = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  name = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  name = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  name = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  name = "DOUBLE";
	  break;
      };
    if (name == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_text (context, name, strlen (name), SQLITE_STATIC);
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetRasterStatistics_BandsCount (sqlite3_context * context, int argc,
				     sqlite3_value ** argv)
{
/* SQL function:
/ GetRasterStatistics_BandsCount(BLOBencoded statistics)
/
/ will return how many Bands are into this statistics object
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    sqlite3_result_int (context, st->nBands);
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetBandStatistics_Min (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ GetBandStatistics_Min(BLOBencoded statistics, int band_index)
/
/ will return the Minimum value for the given Band
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    int band_index;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_index = sqlite3_value_int (argv[1]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    if (band_index < 0 || band_index >= st->nBands)
	sqlite3_result_null (context);
    else
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + band_index;
	  sqlite3_result_double (context, band->min);
      }
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetBandStatistics_Max (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ GetBandStatistics_Max(BLOBencoded statistics, int band_index)
/
/ will return the Maximum value for the given Band
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    int band_index;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_index = sqlite3_value_int (argv[1]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    if (band_index < 0 || band_index >= st->nBands)
	sqlite3_result_null (context);
    else
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + band_index;
	  sqlite3_result_double (context, band->max);
      }
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetBandStatistics_Avg (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ GetBandStatistics_Avg(BLOBencoded statistics, int band_index)
/
/ will return the Average value for the given Band
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    int band_index;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_index = sqlite3_value_int (argv[1]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    if (band_index < 0 || band_index >= st->nBands)
	sqlite3_result_null (context);
    else
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + band_index;
	  sqlite3_result_double (context, band->mean);
      }
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetBandStatistics_Var (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ GetBandStatistics_Var(BLOBencoded statistics, int band_index)
/
/ will return the Variance for the given Band
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    int band_index;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    double variance;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_index = sqlite3_value_int (argv[1]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    if (band_index < 0 || band_index >= st->nBands)
	sqlite3_result_null (context);
    else
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + band_index;
	  if (band->first != NULL)
	    {
		double count = 0.0;
		double sum_var = 0.0;
		double sum_count = 0.0;
		rl2PoolVariancePtr pV = band->first;
		while (pV != NULL)
		  {
		      count += 1.0;
		      sum_var += (pV->count - 1.0) * pV->variance;
		      sum_count += pV->count;
		      pV = pV->next;
		  }
		variance = sum_var / (sum_count - count);
	    }
	  else
	      variance = band->sum_sq_diff / (st->count - 1.0);
	  sqlite3_result_double (context, variance);
      }
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetBandStatistics_StdDev (sqlite3_context * context, int argc,
			       sqlite3_value ** argv)
{
/* SQL function:
/ GetBandStatistics_StdDev(BLOBencoded statistics, int band_index)
/
/ will return the StandardDeviation for the given Band
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    int band_index;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    double variance;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_index = sqlite3_value_int (argv[1]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    if (band_index < 0 || band_index >= st->nBands)
	sqlite3_result_null (context);
    else
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + band_index;
	  if (band->first != NULL)
	    {
		double count = 0.0;
		double sum_var = 0.0;
		double sum_count = 0.0;
		rl2PoolVariancePtr pV = band->first;
		while (pV != NULL)
		  {
		      count += 1.0;
		      sum_var += (pV->count - 1.0) * pV->variance;
		      sum_count += pV->count;
		      pV = pV->next;
		  }
		variance = sum_var / (sum_count - count);
	    }
	  else
	      variance = band->sum_sq_diff / (st->count - 1.0);
	  sqlite3_result_double (context, sqrt (variance));
      }
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetBandStatistics_Histogram (sqlite3_context * context, int argc,
				  sqlite3_value ** argv)
{
/* SQL function:
/ GetBandStatistics_Histogram(BLOBencoded statistics, int band_index)
/
/ will return a PNG image representing the Histogram for the given Band
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    int band_index;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    unsigned char *image = NULL;
    int image_size;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    band_index = sqlite3_value_int (argv[1]);
    stats = rl2_deserialize_dbms_raster_statistics (blob, blob_sz);
    if (stats == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }
    st = (rl2PrivRasterStatisticsPtr) stats;
    if (band_index < 0 || band_index >= st->nBands)
	sqlite3_result_null (context);
    else
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + band_index;
	  if (get_raster_band_histogram (band, &image, &image_size) == RL2_OK)
	      sqlite3_result_blob (context, image, image_size, free);
	  else
	      sqlite3_result_null (context);
      }
    rl2_destroy_raster_statistics (stats);
}

static void
fnct_GetBandHistogramFromImage (sqlite3_context * context, int argc,
				sqlite3_value ** argv)
{
/* SQL function:
/ GetBandHistogramFromImage(BLOBencoded statistics, TEXT mime_type, int band_index)
/
/ will return a PNG image representing the Histogram for the given Band
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    int band_index;
    const char *mime_type = "text/plain";
    rl2RasterPtr raster = NULL;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    unsigned char *image = NULL;
    int image_size;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
      {
	  sqlite3_result_null (context);
	  return;
      }
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
      {
	  sqlite3_result_null (context);
	  return;
      }
    blob = sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    mime_type = (const char *) sqlite3_value_text (argv[1]);
    band_index = sqlite3_value_int (argv[2]);
/* validating the mime-type */
    if (strcmp (mime_type, "image/png") == 0)
	raster = rl2_raster_from_png (blob, blob_sz);
    if (strcmp (mime_type, "image/jpeg") == 0)
	raster = rl2_raster_from_jpeg (blob, blob_sz);
    if (raster == NULL)
	goto error;
/* attempting to build Raster Statistics */
    stats = rl2_build_raster_statistics (raster, NULL);
    if (stats == NULL)
	goto error;
    rl2_destroy_raster (raster);
    raster = NULL;
/* attempting to build the Histogram */
    st = (rl2PrivRasterStatisticsPtr) stats;
    if (band_index < 0 || band_index >= st->nBands)
	sqlite3_result_null (context);
    else
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + band_index;
	  if (get_raster_band_histogram (band, &image, &image_size) == RL2_OK)
	      sqlite3_result_blob (context, image, image_size, free);
	  else
	      sqlite3_result_null (context);
      }
    rl2_destroy_raster_statistics (stats);
    return;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    sqlite3_result_null (context);
}

static void
fnct_CreateCoverage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double res)
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double horz_res, double vert_res)
/ CreateCoverage(text coverage, text sample_type, text pixel_type,
/                int num_bands, text compression, int quality,
/                int tile_width, int tile_height, int srid,
/                double horz_res, double vert_res, BLOB no_data)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *coverage;
    const char *sample_type;
    const char *pixel_type;
    int num_bands;
    const char *compression;
    int quality;
    int tile_width;
    int tile_height;
    int srid;
    double horz_res;
    double vert_res;
    unsigned char sample;
    unsigned char pixel;
    unsigned char compr;
    sqlite3 *sqlite;
    int ret;
    rl2PixelPtr no_data = NULL;
    rl2PalettePtr palette = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 10)
      {
	  if (sqlite3_value_type (argv[10]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[10]) != SQLITE_FLOAT)
	      err = 1;
      }
    if (argc > 11)
      {
	  if (sqlite3_value_type (argv[11]) != SQLITE_BLOB
	      && sqlite3_value_type (argv[11]) != SQLITE_NULL)
	      err = 1;
      }
    if (err)
	goto error;

/* retrieving the arguments */
    coverage = (const char *) sqlite3_value_text (argv[0]);
    sample_type = (const char *) sqlite3_value_text (argv[1]);
    pixel_type = (const char *) sqlite3_value_text (argv[2]);
    num_bands = sqlite3_value_int (argv[3]);
    compression = (const char *) sqlite3_value_text (argv[4]);
    quality = sqlite3_value_int (argv[5]);
    tile_width = sqlite3_value_int (argv[6]);
    tile_height = sqlite3_value_int (argv[7]);
    srid = sqlite3_value_int (argv[8]);
    if (sqlite3_value_type (argv[9]) == SQLITE_FLOAT)
	horz_res = sqlite3_value_double (argv[9]);
    else
      {
	  int val = sqlite3_value_int (argv[9]);
	  horz_res = val;
      }
    if (argc > 10)
      {
	  if (sqlite3_value_type (argv[10]) == SQLITE_FLOAT)
	      vert_res = sqlite3_value_double (argv[10]);
	  else
	    {
		int val = sqlite3_value_int (argv[10]);
		vert_res = val;
	    }
      }
    else
	vert_res = horz_res;
    if (argc > 11 && sqlite3_value_type (argv[11]) == SQLITE_BLOB)
      {
	  /* NO-DATA pixel */
	  const unsigned char *blob = sqlite3_value_blob (argv[11]);
	  int blob_sz = sqlite3_value_bytes (argv[11]);
	  no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
	  if (no_data == NULL)
	      goto error;
      }

/* preliminary arg checking */
    if (num_bands < 1 || num_bands > 255)
	goto error;
    if (quality < 0)
	quality = 0;
    if (quality > 100)
	quality = 100;
    if (tile_width < 0 || tile_width > 65536)
	goto error;
    if (tile_height < 0 || tile_height > 65536)
	goto error;

    sample = RL2_SAMPLE_UNKNOWN;
    if (strcasecmp (sample_type, "1-BIT") == 0)
	sample = RL2_SAMPLE_1_BIT;
    if (strcasecmp (sample_type, "2-BIT") == 0)
	sample = RL2_SAMPLE_2_BIT;
    if (strcasecmp (sample_type, "4-BIT") == 0)
	sample = RL2_SAMPLE_4_BIT;
    if (strcasecmp (sample_type, "INT8") == 0)
	sample = RL2_SAMPLE_INT8;
    if (strcasecmp (sample_type, "UINT8") == 0)
	sample = RL2_SAMPLE_UINT8;
    if (strcasecmp (sample_type, "INT16") == 0)
	sample = RL2_SAMPLE_INT16;
    if (strcasecmp (sample_type, "UINT16") == 0)
	sample = RL2_SAMPLE_UINT16;
    if (strcasecmp (sample_type, "INT32") == 0)
	sample = RL2_SAMPLE_INT32;
    if (strcasecmp (sample_type, "UINT32") == 0)
	sample = RL2_SAMPLE_UINT32;
    if (strcasecmp (sample_type, "FLOAT") == 0)
	sample = RL2_SAMPLE_FLOAT;
    if (strcasecmp (sample_type, "DOUBLE") == 0)
	sample = RL2_SAMPLE_DOUBLE;

    pixel = RL2_PIXEL_UNKNOWN;
    if (strcasecmp (pixel_type, "MONOCHROME") == 0)
	pixel = RL2_PIXEL_MONOCHROME;
    if (strcasecmp (pixel_type, "GRAYSCALE") == 0)
	pixel = RL2_PIXEL_GRAYSCALE;
    if (strcasecmp (pixel_type, "PALETTE") == 0)
	pixel = RL2_PIXEL_PALETTE;
    if (strcasecmp (pixel_type, "RGB") == 0)
	pixel = RL2_PIXEL_RGB;
    if (strcasecmp (pixel_type, "DATAGRID") == 0)
	pixel = RL2_PIXEL_DATAGRID;
    if (strcasecmp (pixel_type, "MULTIBAND") == 0)
	pixel = RL2_PIXEL_MULTIBAND;

    compr = RL2_COMPRESSION_UNKNOWN;
    if (strcasecmp (compression, "NONE") == 0)
	compr = RL2_COMPRESSION_NONE;
    if (strcasecmp (compression, "DEFLATE") == 0)
	compr = RL2_COMPRESSION_DEFLATE;
    if (strcasecmp (compression, "LZMA") == 0)
	compr = RL2_COMPRESSION_LZMA;
    if (strcasecmp (compression, "PNG") == 0)
	compr = RL2_COMPRESSION_PNG;
    if (strcasecmp (compression, "GIF") == 0)
	compr = RL2_COMPRESSION_GIF;
    if (strcasecmp (compression, "JPEG") == 0)
	compr = RL2_COMPRESSION_JPEG;
    if (strcasecmp (compression, "WEBP") == 0)
	compr = RL2_COMPRESSION_LOSSY_WEBP;
    if (strcasecmp (compression, "LL_WEBP") == 0)
	compr = RL2_COMPRESSION_LOSSLESS_WEBP;
    if (strcasecmp (compression, "FAX4") == 0)
	compr = RL2_COMPRESSION_CCITTFAX4;

    if (no_data == NULL)
      {
	  /* creating a default NO-DATA value */
	  no_data = default_nodata (sample, pixel, num_bands);
      }
    if (pixel == RL2_PIXEL_PALETTE)
      {
/* creating a default PALETTE */
	  palette = rl2_create_palette (1);
	  rl2_set_palette_color (palette, 0, 255, 255, 255);
      }

/* attempting to create the DBMS Coverage */
    sqlite = sqlite3_context_db_handle (context);
    ret = rl2_create_dbms_coverage (sqlite, coverage, sample, pixel,
				    (unsigned char) num_bands,
				    compr, quality,
				    (unsigned short) tile_width,
				    (unsigned short) tile_height, srid,
				    horz_res, vert_res, no_data, palette);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return;

  error:
    sqlite3_result_int (context, -1);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
}

static void
fnct_DeleteSection (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ DeleteSection(text coverage, text section)
/ DeleteSection(text coverage, text section, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *coverage;
    const char *section;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    rl2CoveragePtr cvg = NULL;
    sqlite3_int64 section_id;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	goto invalid;

/* retrieving the arguments */
    sqlite = sqlite3_context_db_handle (context);
    coverage = (const char *) sqlite3_value_text (argv[0]);
    section = (const char *) sqlite3_value_text (argv[1]);
    if (argc > 2)
	transaction = sqlite3_value_int (argv[2]);

    cvg = rl2_create_coverage_from_dbms (sqlite, coverage);
    if (cvg == NULL)
	goto error;
    if (rl2_get_dbms_section_id (sqlite, coverage, section, &section_id) !=
	RL2_OK)
	goto error;

    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }

    if (rl2_delete_dbms_section (sqlite, coverage, section_id) != RL2_OK)
	goto error;

    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }
    sqlite3_result_int (context, 1);
    rl2_destroy_coverage (cvg);
    return;

  invalid:
    sqlite3_result_int (context, -1);
    return;
  error:
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    sqlite3_result_int (context, 0);
    if (transaction)
      {
	  /* invalidating the pending transaction */
	  sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
      }
    return;
}

static void
fnct_DropCoverage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ DropCoverage(text coverage)
/ DropCoverage(text coverage, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *coverage;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    rl2CoveragePtr cvg = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	goto invalid;

/* retrieving the arguments */
    sqlite = sqlite3_context_db_handle (context);
    coverage = (const char *) sqlite3_value_text (argv[0]);
    if (argc > 1)
	transaction = sqlite3_value_int (argv[1]);

    cvg = rl2_create_coverage_from_dbms (sqlite, coverage);
    if (cvg == NULL)
	goto error;

    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }

    if (rl2_drop_dbms_coverage (sqlite, coverage) != RL2_OK)
	goto error;

    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }
    sqlite3_result_int (context, 1);
    rl2_destroy_coverage (cvg);
    return;

  invalid:
    sqlite3_result_int (context, -1);
    return;
  error:
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    sqlite3_result_int (context, 0);
    if (transaction)
      {
	  /* invalidating the pending transaction */
	  sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
      }
    return;
}

static void
fnct_LoadRaster (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ LoadRaster(text coverage, text path_to_raster)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile,
/            int force_srid)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile,
/            int force_srid, int pyramidize)
/ LoadRaster(text coverage, text path_to_raster, int with_worldfile,
/            int force_srid, int pyramidize, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int worldfile = 0;
    int force_srid = -1;
    int transaction = 1;
    int pyramidize = 1;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    if (argc > 2)
	worldfile = sqlite3_value_int (argv[2]);
    if (argc > 3)
	force_srid = sqlite3_value_int (argv[3]);
    if (argc > 4)
	pyramidize = sqlite3_value_int (argv[4]);
    if (argc > 5)
	transaction = sqlite3_value_int (argv[5]);


/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* attempting to load the Raster into the DBMS */
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		rl2_destroy_coverage (coverage);
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret = rl2_load_raster_into_dbms (sqlite, path, coverage,
				     worldfile, force_srid, pyramidize);
    rl2_destroy_coverage (coverage);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_LoadRastersFromDir (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ LoadRastersFromDir(text coverage, text dir_path)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile, int force_srid)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile, int force_srid, 
/                    int pyramidize)
/ LoadRastersFromDir(text coverage, text dir_path, text file_ext,
/                    int with_worldfile, int force_srid, 
/                    int pyramidize, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    const char *file_ext;
    int worldfile = 0;
    int force_srid = -1;
    int transaction = 1;
    int pyramidize = 1;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    if (argc > 2)
	file_ext = (const char *) sqlite3_value_text (argv[2]);
    if (argc > 3)
	worldfile = sqlite3_value_int (argv[3]);
    if (argc > 4)
	force_srid = sqlite3_value_int (argv[4]);
    if (argc > 5)
	pyramidize = sqlite3_value_int (argv[5]);
    if (argc > 6)
	transaction = sqlite3_value_int (argv[6]);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* attempting to load the Rasters into the DBMS */
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		rl2_destroy_coverage (coverage);
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret = rl2_load_mrasters_into_dbms (sqlite, path, file_ext, coverage,
				       worldfile, force_srid, pyramidize);
    rl2_destroy_coverage (coverage);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_Pyramidize (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ Pyramidize(text coverage)
/ Pyramidize(text coverage, text section)
/ Pyramidize(text coverage, text section, int force_rebuild)
/ Pyramidize(text coverage, text section, int force_rebuild,
/            int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *sect_name = NULL;
    int forced_rebuild = 0;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_TEXT
	&& sqlite3_value_type (argv[1]) != SQLITE_NULL)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
/* attempting to (re)build Pyramid levels */
    sqlite = sqlite3_context_db_handle (context);
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    if (argc > 1)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	      sect_name = (const char *) sqlite3_value_text (argv[1]);
      }
    if (argc > 2)
	forced_rebuild = sqlite3_value_int (argv[2]);
    if (argc > 3)
	transaction = sqlite3_value_int (argv[3]);
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (sect_name == NULL)
	ret = rl2_build_all_section_pyramids (sqlite, cvg_name, forced_rebuild);
    else
	ret =
	    rl2_build_section_pyramid (sqlite, cvg_name, sect_name,
				       forced_rebuild);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_DePyramidize (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ DePyramidize(text coverage)
/ DePyramidize(text coverage, text section)
/ DePyramidize(text coverage, text section, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *sect_name = NULL;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_TEXT
	&& sqlite3_value_type (argv[1]) != SQLITE_NULL)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
/* attempting to delete all Pyramid levels */
    sqlite = sqlite3_context_db_handle (context);
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    if (argc > 1)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	      sect_name = (const char *) sqlite3_value_text (argv[1]);
      }
    if (argc > 2)
	transaction = sqlite3_value_int (argv[2]);
    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (sect_name == NULL)
	ret = rl2_delete_all_pyramids (sqlite, cvg_name);
    else
	ret = rl2_delete_section_pyramid (sqlite, cvg_name, sect_name);
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }
    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_LoadRasterFromWMS (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, double res)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer,
/                   text wms_style, text wms_format, double horz_res, 
/                   double vert_res)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, text wms_crs,
/                   double horz_res, double vert_res, int opaque)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, double horz_res,
/                   double vert_res, int opaque,
/                   int swap_xy)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, double horz_res,
/                   double vert_res, int opaque, int swap_xy, text proxy)
/ LoadRasterFromWMS(text coverage, text section, text getmap_url,
/                   BLOB geom, text wms_version, text wms_layer, 
/                   text wms_style, text wms_format, double horz_res,
/                   double vert_res,  int opaque, int swap_xy, 
/                   text proxy, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *sect_name;
    const char *url;
    const char *proxy = NULL;
    const char *wms_version;
    const char *wms_layer;
    const char *wms_style = NULL;
    const char *wms_format;
    char *wms_crs = NULL;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    int opaque = 0;
    int swap_xy = 0;
    int srid;
    int transaction = 1;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    rl2CoveragePtr coverage = NULL;
    rl2RasterStatisticsPtr section_stats = NULL;
    sqlite3 *sqlite;
    int ret;
    int n;
    double x;
    double y;
    double tilew;
    double tileh;
    unsigned short tile_width;
    unsigned short tile_height;
    WmsRetryListPtr retry_list = NULL;
    char *table;
    char *xtable;
    char *sql;
    sqlite3_stmt *stmt_data = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_sect = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_upd_sect = NULL;
    int first = 1;
    double ext_x;
    double ext_y;
    int width;
    int height;
    sqlite3_int64 section_id;
    rl2PixelPtr no_data = NULL;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    int quality;
    InsertWms params;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_TEXT
	&& sqlite3_value_type (argv[6]) != SQLITE_NULL)
	err = 1;
    if (sqlite3_value_type (argv[7]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 12)
      {
	  if (sqlite3_value_type (argv[12]) == SQLITE_TEXT)
	      ;
	  else if (sqlite3_value_type (argv[12]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
      }
    if (argc > 13 && sqlite3_value_type (argv[13]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    sect_name = (const char *) sqlite3_value_text (argv[1]);
    url = (const char *) sqlite3_value_text (argv[2]);
    blob = sqlite3_value_blob (argv[3]);
    blob_sz = sqlite3_value_bytes (argv[3]);
    wms_version = (const char *) sqlite3_value_text (argv[4]);
    wms_layer = (const char *) sqlite3_value_text (argv[5]);
    if (sqlite3_value_type (argv[6]) == SQLITE_TEXT)
	wms_style = (const char *) sqlite3_value_text (argv[6]);
    wms_format = (const char *) sqlite3_value_text (argv[7]);
    if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[8]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[8]);
    if (argc > 9)
      {
	  if (sqlite3_value_type (argv[9]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[9]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[9]);
      }
    else
	vert_res = horz_res;
    if (argc > 10)
	opaque = sqlite3_value_int (argv[10]);
    if (argc > 11)
	swap_xy = sqlite3_value_int (argv[11]);
    if (argc > 12 && sqlite3_value_type (argv[12]) == SQLITE_TEXT)
	proxy = (const char *) sqlite3_value_text (argv[12]);
    if (argc > 13)
	transaction = sqlite3_value_int (argv[13]);

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
/* retrieving the BBOX */
    minx = geom->MinX;
    maxx = geom->MaxX;
    miny = geom->MinY;
    maxy = geom->MaxY;
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (rl2_get_coverage_tile_size (coverage, &tile_width, &tile_height) !=
	RL2_OK)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (rl2_get_coverage_srid (coverage, &srid) != RL2_OK)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (rl2_get_coverage_type (coverage, &sample_type, &pixel_type, &num_bands)
	!= RL2_OK)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    no_data = rl2_get_coverage_no_data (coverage);
    if (rl2_get_coverage_compression (coverage, &compression, &quality)
	!= RL2_OK)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    tilew = (double) tile_width *horz_res;
    tileh = (double) tile_height *vert_res;
    ext_x = maxx - minx;
    ext_y = maxy - miny;
    width = ext_x / horz_res;
    height = ext_y / horz_res;
    if (((double) width * horz_res) < ext_x)
	width++;
    if (((double) height * vert_res) < ext_y)
	height++;
    if (width < tile_width || width > UINT16_MAX)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (height < tile_height || height > UINT16_MAX)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		rl2_destroy_coverage (coverage);
		sqlite3_result_int (context, -1);
		return;
	    }
      }

/* SQL prepared statements */
    table = sqlite3_mprintf ("%s_sections", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (section_id, section_name, file_path, "
	 "width, height, geometry) VALUES (NULL, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO sections SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_sections", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET statistics = ? WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE sections SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_levels", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT OR IGNORE INTO \"%s\" (pyramid_level, "
	 "x_resolution_1_1, y_resolution_1_1, "
	 "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, "
	 "y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
	 "VALUES (0, ?, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_levl, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO levels SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tiles", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, 0, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_tils, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tiles SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tile_data", cvg_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (tile_id, tile_data_odd, tile_data_even) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tile_data SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

/* preparing the WMS requests */
    wms_crs = sqlite3_mprintf ("EPSG:%d", srid);
    tilew = (double) tile_width *horz_res;
    tileh = (double) tile_height *vert_res;
    ext_x = maxx - minx;
    ext_y = maxy - miny;
    width = ext_x / horz_res;
    height = ext_y / horz_res;
    if ((width * horz_res) < ext_x)
	width++;
    if ((height * vert_res) < ext_y)
	height++;
    retry_list = alloc_retry_list ();
    for (y = maxy; y > miny; y -= tileh)
      {
	  for (x = minx; x < maxx; x += tilew)
	    {
		char *err_msg = NULL;
		unsigned char *rgba_tile =
		    do_wms_GetMap_get (NULL, url, proxy, wms_version, wms_layer,
				       wms_crs, swap_xy, x, y - tileh,
				       x + tilew, y, tile_width, tile_height,
				       wms_style, wms_format, opaque, 0,
				       &err_msg);
		if (rgba_tile == NULL)
		  {
		      add_retry (retry_list, x, y - tileh, x + tilew, y);
		      continue;
		  }

		params.sqlite = sqlite;
		params.rgba_tile = rgba_tile;
		params.coverage = coverage;
		params.sect_name = sect_name;
		params.x = x;
		params.y = y;
		params.width = width;
		params.height = height;
		params.tilew = tilew;
		params.tileh = tileh;
		params.srid = srid;
		params.minx = minx;
		params.miny = miny;
		params.maxx = maxx;
		params.maxy = maxy;
		params.sample_type = sample_type;
		params.num_bands = num_bands;
		params.compression = compression;
		params.horz_res = horz_res;
		params.vert_res = vert_res;
		params.tile_width = tile_width;
		params.tile_height = tile_height;
		params.no_data = no_data;
		params.stmt_sect = stmt_sect;
		params.stmt_levl = stmt_levl;
		params.stmt_tils = stmt_tils;
		params.stmt_data = stmt_data;
		if (!insert_wms_tile
		    (&params, &first, &section_stats, &section_id))
		    goto error;
	    }
      }
    for (n = 0; n < 5; n++)
      {
	  WmsRetryItemPtr retry = retry_list->first;
	  while (retry != NULL)
	    {
		char *err_msg = NULL;
		unsigned char *rgba_tile = NULL;
		if (retry->done)
		  {
		      retry = retry->next;
		      continue;
		  }
		retry->count += 1;
		rgba_tile =
		    do_wms_GetMap_get (NULL, url, proxy, wms_version, wms_layer,
				       wms_crs, swap_xy, retry->minx,
				       retry->miny, retry->maxx, retry->maxy,
				       tile_width, tile_height, wms_style,
				       wms_format, opaque, 0, &err_msg);
		if (rgba_tile == NULL)
		  {
		      retry = retry->next;
		      continue;
		  }

		params.sqlite = sqlite;
		params.rgba_tile = rgba_tile;
		params.coverage = coverage;
		params.sect_name = sect_name;
		params.x = x;
		params.y = y;
		params.width = width;
		params.height = height;
		params.tilew = tilew;
		params.tileh = tileh;
		params.srid = srid;
		params.minx = minx;
		params.miny = miny;
		params.maxx = maxx;
		params.maxy = maxy;
		params.sample_type = sample_type;
		params.num_bands = num_bands;
		params.compression = compression;
		params.horz_res = horz_res;
		params.vert_res = vert_res;
		params.tile_width = tile_width;
		params.tile_height = tile_height;
		params.no_data = no_data;
		params.stmt_sect = stmt_sect;
		params.stmt_levl = stmt_levl;
		params.stmt_tils = stmt_tils;
		params.stmt_data = stmt_data;
		if (!insert_wms_tile
		    (&params, &first, &section_stats, &section_id))
		    goto error;
		retry->done = 1;
		retry = retry->next;
	    }
      }

    if (!do_insert_stats (sqlite, section_stats, section_id, stmt_upd_sect))
	goto error;
    free_retry_list (retry_list);
    retry_list = NULL;
    sqlite3_free (wms_crs);
    wms_crs = NULL;

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

    rl2_destroy_raster_statistics (section_stats);
    section_stats = NULL;
    rl2_destroy_coverage (coverage);
    coverage = NULL;
    if (ret != RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  return;
      }

    if (rl2_update_dbms_coverage (sqlite, cvg_name) != RL2_OK)
      {
	  fprintf (stderr, "unable to update the Coverage\n");
	  goto error;
      }

    if (transaction)
      {
	  /* committing the still pending transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
    return;

  error:
    if (wms_crs != NULL)
	sqlite3_free (wms_crs);
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
    if (retry_list != NULL)
	free_retry_list (retry_list);
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (section_stats != NULL)
	rl2_destroy_raster_statistics (section_stats);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteGeoTiff (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double resolution)
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile)
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile,
/              text compression)
/ WriteGeoTiff(text coverage, text geotiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile,
/              text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    int worldfile = 0;
    unsigned char compression = RL2_COMPRESSION_NONE;
    int tile_sz = 256;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[5]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[6]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_TEXT)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    blob = sqlite3_value_blob (argv[4]);
    blob_sz = sqlite3_value_bytes (argv[4]);
    if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[5]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[5]);
    if (argc > 6)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[6]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[6]);
      }
    else
	vert_res = horz_res;
    if (argc > 7)
	worldfile = sqlite3_value_int (argv[7]);
    if (argc > 8)
      {
	  const char *compr = (const char *) sqlite3_value_text (argv[8]);
	  compression = RL2_COMPRESSION_UNKNOWN;
	  if (strcasecmp (compr, "NONE") == 0)
	      compression = RL2_COMPRESSION_NONE;
	  if (strcasecmp (compr, "DEFLATE") == 0)
	      compression = RL2_COMPRESSION_DEFLATE;
	  if (strcasecmp (compr, "LZW") == 0)
	      compression = RL2_COMPRESSION_LZW;
	  if (strcasecmp (compr, "JPEG") == 0)
	      compression = RL2_COMPRESSION_JPEG;
	  if (strcasecmp (compr, "FAX3") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX3;
	  if (strcasecmp (compr, "FAX4") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX4;
      }
    if (argc > 9)
	tile_sz = sqlite3_value_int (argv[9]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (tile_sz < 64 || tile_sz > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (compression == RL2_COMPRESSION_UNKNOWN)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    ret =
	rl2_export_geotiff_from_dbms (sqlite, path, coverage, horz_res,
				      vert_res, minx, miny, maxx, maxy, width,
				      height, compression, tile_sz, worldfile);
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteTripleBandGeoTiff (sqlite3_context * context, int argc,
			     sqlite3_value ** argv)
{
/* SQL function:
/ WriteTripleBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int red_band, int green_band, int blue_band,
/              BLOB geom, double resolution)
/ WriteTripleBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int red_band, int green_band, int blue_band, 
/              BLOB geom, double horz_res, double vert_res)
/ WriteTripleBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int red_band, int green_band, int blue_band,
/              BLOB geom, double horz_res, double vert_res, 
/              int with_worldfile)
/ WriteTripleBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int red_band, int green_band, int blue_band, 
/              BLOB geom, double horz_res, double vert_res, 
/              int with_worldfile, text compression)
/ WriteTripleBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int red_band, int green_band, int blue_band, 
/              BLOB geom, double horz_res, double vert_res, 
/              int with_worldfile, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    int red_band;
    int green_band;
    int blue_band;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    int worldfile = 0;
    unsigned char compression = RL2_COMPRESSION_NONE;
    int tile_sz = 256;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[7]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_TEXT)
	err = 1;
    if (argc > 12 && sqlite3_value_type (argv[12]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    red_band = sqlite3_value_int (argv[4]);
    green_band = sqlite3_value_int (argv[5]);
    blue_band = sqlite3_value_int (argv[6]);
    blob = sqlite3_value_blob (argv[7]);
    blob_sz = sqlite3_value_bytes (argv[7]);
    if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[8]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[8]);
    if (argc > 9)
      {
	  if (sqlite3_value_type (argv[9]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[9]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[9]);
      }
    else
	vert_res = horz_res;
    if (argc > 10)
	worldfile = sqlite3_value_int (argv[10]);
    if (argc > 11)
      {
	  const char *compr = (const char *) sqlite3_value_text (argv[11]);
	  compression = RL2_COMPRESSION_UNKNOWN;
	  if (strcasecmp (compr, "NONE") == 0)
	      compression = RL2_COMPRESSION_NONE;
	  if (strcasecmp (compr, "DEFLATE") == 0)
	      compression = RL2_COMPRESSION_DEFLATE;
	  if (strcasecmp (compr, "LZW") == 0)
	      compression = RL2_COMPRESSION_LZW;
	  if (strcasecmp (compr, "JPEG") == 0)
	      compression = RL2_COMPRESSION_JPEG;
	  if (strcasecmp (compr, "FAX3") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX3;
	  if (strcasecmp (compr, "FAX4") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX4;
      }
    if (argc > 12)
	tile_sz = sqlite3_value_int (argv[12]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (red_band < 0 || red_band > 255)
      {
	  errcode = -1;
	  goto error;
      }
    if (green_band < 0 || green_band > 255)
      {
	  errcode = -1;
	  goto error;
      }
    if (blue_band < 0 || blue_band > 255)
      {
	  errcode = -1;
	  goto error;
      }
    if (tile_sz < 64 || tile_sz > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (compression == RL2_COMPRESSION_UNKNOWN)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    ret =
	rl2_export_triple_band_geotiff_from_dbms (sqlite, path, coverage,
						  horz_res, vert_res, minx,
						  miny, maxx, maxy, width,
						  height, red_band,
						  green_band, blue_band,
						  compression, tile_sz,
						  worldfile);
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteMonoBandGeoTiff (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ WriteMonoBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int mono_band, BLOB geom, double resolution)
/ WriteMonoBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int mono_band, BLOB geom, double horz_res,
/              double vert_res)
/ WriteMonoBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int mono_band, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile)
/ WriteMonoBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int mono_band, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile, text compression)
/ WriteMonoBandGeoTiff(text coverage, text geotiff_path, int width,
/              int height, int mono_band, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile, text compression,
/              int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    int mono_band;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    int worldfile = 0;
    unsigned char compression = RL2_COMPRESSION_NONE;
    int tile_sz = 256;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[6]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[7]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_TEXT)
	err = 1;
    if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    mono_band = sqlite3_value_int (argv[4]);
    blob = sqlite3_value_blob (argv[5]);
    blob_sz = sqlite3_value_bytes (argv[5]);
    if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[6]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[6]);
    if (argc > 7)
      {
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[7]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[7]);
      }
    else
	vert_res = horz_res;
    if (argc > 8)
	worldfile = sqlite3_value_int (argv[8]);
    if (argc > 9)
      {
	  const char *compr = (const char *) sqlite3_value_text (argv[9]);
	  compression = RL2_COMPRESSION_UNKNOWN;
	  if (strcasecmp (compr, "NONE") == 0)
	      compression = RL2_COMPRESSION_NONE;
	  if (strcasecmp (compr, "DEFLATE") == 0)
	      compression = RL2_COMPRESSION_DEFLATE;
	  if (strcasecmp (compr, "LZW") == 0)
	      compression = RL2_COMPRESSION_LZW;
	  if (strcasecmp (compr, "JPEG") == 0)
	      compression = RL2_COMPRESSION_JPEG;
	  if (strcasecmp (compr, "FAX3") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX3;
	  if (strcasecmp (compr, "FAX4") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX4;
      }
    if (argc > 10)
	tile_sz = sqlite3_value_int (argv[10]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (mono_band < 0 || mono_band > 255)
      {
	  errcode = -1;
	  goto error;
      }
    if (tile_sz < 64 || tile_sz > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (compression == RL2_COMPRESSION_UNKNOWN)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    ret =
	rl2_export_mono_band_geotiff_from_dbms (sqlite, path, coverage,
						horz_res, vert_res, minx,
						miny, maxx, maxy, width,
						height, mono_band,
						compression, tile_sz,
						worldfile);
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
common_write_tiff (int with_worldfile, sqlite3_context * context, int argc,
		   sqlite3_value ** argv)
{
/* SQL function:
/ WriteTiff?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double resolution)
/ WriteTiff?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double horz_res,
/            double vert_res)
/ WriteTiff?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double horz_res,
/            double vert_res, text compression)
/ WriteTiff?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double horz_res,
/            double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    unsigned char compression = RL2_COMPRESSION_NONE;
    int tile_sz = 256;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[5]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[6]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_TEXT)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    blob = sqlite3_value_blob (argv[4]);
    blob_sz = sqlite3_value_bytes (argv[4]);
    if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[5]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[5]);
    if (argc > 6)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[6]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[6]);
      }
    else
	vert_res = horz_res;
    if (argc > 7)
      {
	  const char *compr = (const char *) sqlite3_value_text (argv[7]);
	  compression = RL2_COMPRESSION_UNKNOWN;
	  if (strcasecmp (compr, "NONE") == 0)
	      compression = RL2_COMPRESSION_NONE;
	  if (strcasecmp (compr, "DEFLATE") == 0)
	      compression = RL2_COMPRESSION_DEFLATE;
	  if (strcasecmp (compr, "LZW") == 0)
	      compression = RL2_COMPRESSION_LZW;
	  if (strcasecmp (compr, "JPEG") == 0)
	      compression = RL2_COMPRESSION_JPEG;
	  if (strcasecmp (compr, "FAX3") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX3;
	  if (strcasecmp (compr, "FAX4") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX4;
      }
    if (argc > 8)
	tile_sz = sqlite3_value_int (argv[8]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (tile_sz < 64 || tile_sz > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (compression == RL2_COMPRESSION_UNKNOWN)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (with_worldfile)
      {
	  /* TIFF + Worldfile */
	  ret =
	      rl2_export_tiff_worldfile_from_dbms (sqlite, path, coverage,
						   horz_res, vert_res, minx,
						   miny, maxx, maxy, width,
						   height, compression,
						   tile_sz);
      }
    else
      {
	  /* plain TIFF, no Worldfile */
	  ret =
	      rl2_export_tiff_from_dbms (sqlite, path, coverage, horz_res,
					 vert_res, minx, miny, maxx, maxy,
					 width, height, compression, tile_sz);
      }
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteTiffTfw (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteTiffTfw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double resolution)
/ WriteTiffTfw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteTiffTfw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, text compression)
/ WriteTiffTfw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_tiff (1, context, argc, argv);
}

static void
fnct_WriteTiff (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res,
/              double vert_res, text compression)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res,
/           double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_tiff (0, context, argc, argv);
}

static void
common_write_jpeg (int with_worldfile, sqlite3_context * context, int argc,
		   sqlite3_value ** argv)
{
/* SQL function:
/ WriteJpeg?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double resolution)
/ WriteJpeg?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double horz_res,
/            double vert_res)
/ WriteJpeg?(text coverage, text tiff_path, int width,
/            int height, BLOB geom, double horz_res,
/            double vert_res, int quality)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    int quality = 80;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[5]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[6]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    blob = sqlite3_value_blob (argv[4]);
    blob_sz = sqlite3_value_bytes (argv[4]);
    if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[5]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[5]);
    if (argc > 6)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[6]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[6]);
      }
    else
	vert_res = horz_res;
    if (argc > 7)
	quality = sqlite3_value_int (argv[7]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (quality < 0)
	quality = 0;
    if (quality > 100)
	quality = 100;

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    ret =
	rl2_export_jpeg_from_dbms (sqlite, path, coverage, horz_res,
				   vert_res, minx, miny, maxx, maxy,
				   width, height, quality, with_worldfile);
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteJpegJgw (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteJpegJgw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double resolution)
/ WriteJpegJgw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteJpegJgw(text coverage, text tiff_path, int width,
/              int height, BLOB geom, double horz_res,
/              double vert_res, int quality)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_jpeg (1, context, argc, argv);
}

static void
fnct_WriteJpeg (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteJpeg(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution)
/ WriteJpeg(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteJpeg(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res,
/              double vert_res, int quality)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_jpeg (0, context, argc, argv);
}

static void
common_write_triple_band_tiff (int with_worldfile, sqlite3_context * context,
			       int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteTripleBandTiff?(text coverage, text tiff_path, int width,
/            int height, int red_band, int green_band, int blue_band,
/            BLOB geom, double resolution)
/ WriteTripleBandTiff?(text coverage, text tiff_path, int width,
/            int height, int red_band, int green_band, int blue_band,
/            BLOB geom, double horz_res, double vert_res)
/ WriteTripleBandTiff?(text coverage, text tiff_path, int width,
/            int height, int red_band, int green_band, int blue_band,
/            BLOB geom, double horz_res, double vert_res,
/            text compression)
/ WriteTripleBandTiff?(text coverage, text tiff_path, int width,
/            int height, int red_band, int green_band, int blue_band, 
/            BLOB geom, double horz_res, double vert_res,
/            text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    int red_band;
    int green_band;
    int blue_band;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    unsigned char compression = RL2_COMPRESSION_NONE;
    int tile_sz = 256;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[7]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_TEXT)
	err = 1;
    if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    red_band = sqlite3_value_int (argv[4]);
    green_band = sqlite3_value_int (argv[5]);
    blue_band = sqlite3_value_int (argv[6]);
    blob = sqlite3_value_blob (argv[7]);
    blob_sz = sqlite3_value_bytes (argv[7]);
    if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[8]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[8]);
    if (argc > 9)
      {
	  if (sqlite3_value_type (argv[9]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[9]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[9]);
      }
    else
	vert_res = horz_res;
    if (argc > 10)
      {
	  const char *compr = (const char *) sqlite3_value_text (argv[10]);
	  compression = RL2_COMPRESSION_UNKNOWN;
	  if (strcasecmp (compr, "NONE") == 0)
	      compression = RL2_COMPRESSION_NONE;
	  if (strcasecmp (compr, "DEFLATE") == 0)
	      compression = RL2_COMPRESSION_DEFLATE;
	  if (strcasecmp (compr, "LZW") == 0)
	      compression = RL2_COMPRESSION_LZW;
	  if (strcasecmp (compr, "JPEG") == 0)
	      compression = RL2_COMPRESSION_JPEG;
	  if (strcasecmp (compr, "FAX3") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX3;
	  if (strcasecmp (compr, "FAX4") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX4;
      }
    if (argc > 11)
	tile_sz = sqlite3_value_int (argv[11]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (red_band < 0 || red_band > 255)
      {
	  errcode = -1;
	  goto error;
      }
    if (green_band < 0 || green_band > 255)
      {
	  errcode = -1;
	  goto error;
      }
    if (blue_band < 0 || blue_band > 255)
      {
	  errcode = -1;
	  goto error;
      }
    if (tile_sz < 64 || tile_sz > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (compression == RL2_COMPRESSION_UNKNOWN)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (with_worldfile)
      {
	  /* TIFF + Worldfile */
	  ret =
	      rl2_export_triple_band_tiff_worldfile_from_dbms (sqlite, path,
							       coverage,
							       horz_res,
							       vert_res, minx,
							       miny, maxx,
							       maxy, width,
							       height,
							       red_band,
							       green_band,
							       blue_band,
							       compression,
							       tile_sz);
      }
    else
      {
	  /* plain TIFF, no Worldfile */
	  ret =
	      rl2_export_triple_band_tiff_from_dbms (sqlite, path, coverage,
						     horz_res, vert_res, minx,
						     miny, maxx, maxy, width,
						     height, red_band,
						     green_band, blue_band,
						     compression, tile_sz);
      }
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteTripleBandTiffTfw (sqlite3_context * context, int argc,
			     sqlite3_value ** argv)
{
/* SQL function:
/ WriteTripleBandTiffTfw(text coverage, text tiff_path, int width,
/              int height, int red_band, int green_band, int blue_band,
/              BLOB geom, double resolution)
/ WriteTripleBandTiffTfw(text coverage, text tiff_path, int width,
/              int height, int red_band, int green_band, int blue_band,
/              BLOB geom, double horz_res, double vert_res)
/ WriteTripleBandTiffTfw(text coverage, text tiff_path, int width,
/              int height, int red_band, int green_band, int blue_band,
/              BLOB geom, double horz_res, double vert_res, 
/              text compression)
/ WriteTripleBandTiffTfw(text coverage, text tiff_path, int width,
/              int height, int red_band, int green_band, int blue_band,
/              BLOB geom, double horz_res, double vert_res, 
/              text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_triple_band_tiff (1, context, argc, argv);
}

static void
fnct_WriteTripleBandTiff (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ WriteTripleBandTiff(text coverage, text tiff_path, int width,
/           int height, int red_band, int green_band, int blue_band,
/           BLOB geom, double resolution)
/ WriteTripleBandTiff(text coverage, text tiff_path, int width,
/           int height, int red_band, int green_band, int blue_band,
/           BLOB geom, double horz_res, double vert_res)
/ WriteTripleBandTiff(text coverage, text tiff_path, int width,
/           int height, int red_band, int green_band, int blue_band,
/           BLOB geom, double horz_res, double vert_res,
/           text compression)
/ WriteTripleBandTiff(text coverage, text tiff_path, int width,
/           int height, int red_band, int green_band, int blue_band,
/           BLOB geom, double horz_res, double vert_res,
/           text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_triple_band_tiff (0, context, argc, argv);
}

static void
common_write_mono_band_tiff (int with_worldfile, sqlite3_context * context,
			     int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteMonoBandTiff?(text coverage, text tiff_path, int width,
/            int height, int mono_band, BLOB geom, double resolution)
/ WriteMonoBandTiff?(text coverage, text tiff_path, int width,
/            int height, int mono_band, BLOB geom, double horz_res,
/            double vert_res)
/ WriteMonoBandTiff?(text coverage, text tiff_path, int width,
/            int height, int mono_band, BLOB geom, double horz_res,
/            double vert_res, text compression)
/ WriteMonoBandTiff?(text coverage, text tiff_path, int width,
/            int height, int mono_band, BLOB geom, double horz_res,
/            double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    int mono_band;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    unsigned char compression = RL2_COMPRESSION_NONE;
    int tile_sz = 256;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[6]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[7]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_TEXT)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    mono_band = sqlite3_value_int (argv[4]);
    blob = sqlite3_value_blob (argv[5]);
    blob_sz = sqlite3_value_bytes (argv[5]);
    if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[6]);
	  horz_res = ival;
      }
    else
	horz_res = sqlite3_value_double (argv[6]);
    if (argc > 7)
      {
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[7]);
		vert_res = ival;
	    }
	  else
	      vert_res = sqlite3_value_double (argv[7]);
      }
    else
	vert_res = horz_res;
    if (argc > 8)
      {
	  const char *compr = (const char *) sqlite3_value_text (argv[8]);
	  compression = RL2_COMPRESSION_UNKNOWN;
	  if (strcasecmp (compr, "NONE") == 0)
	      compression = RL2_COMPRESSION_NONE;
	  if (strcasecmp (compr, "DEFLATE") == 0)
	      compression = RL2_COMPRESSION_DEFLATE;
	  if (strcasecmp (compr, "LZW") == 0)
	      compression = RL2_COMPRESSION_LZW;
	  if (strcasecmp (compr, "JPEG") == 0)
	      compression = RL2_COMPRESSION_JPEG;
	  if (strcasecmp (compr, "FAX3") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX3;
	  if (strcasecmp (compr, "FAX4") == 0)
	      compression = RL2_COMPRESSION_CCITTFAX4;
      }
    if (argc > 9)
	tile_sz = sqlite3_value_int (argv[9]);

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (mono_band < 0 || mono_band > 255)
      {
	  errcode = -1;
	  goto error;
      }
    if (tile_sz < 64 || tile_sz > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (compression == RL2_COMPRESSION_UNKNOWN)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (with_worldfile)
      {
	  /* TIFF + Worldfile */
	  ret =
	      rl2_export_mono_band_tiff_worldfile_from_dbms (sqlite, path,
							     coverage,
							     horz_res,
							     vert_res, minx,
							     miny, maxx,
							     maxy, width,
							     height,
							     mono_band,
							     compression,
							     tile_sz);
      }
    else
      {
	  /* plain TIFF, no Worldfile */
	  ret =
	      rl2_export_mono_band_tiff_from_dbms (sqlite, path, coverage,
						   horz_res, vert_res, minx,
						   miny, maxx, maxy, width,
						   height, mono_band,
						   compression, tile_sz);
      }
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_WriteMonoBandTiffTfw (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ WriteMonoBandTiffTfw(text coverage, text tiff_path, int width,
/              int height, int mono_band, BLOB geom, double resolution)
/ WriteMonoBandTiffTfw(text coverage, text tiff_path, int width,
/              int height, int mono_band, BLOB geom, double horz_res,
/              double vert_res)
/ WriteMonoBandTiffTfw(text coverage, text tiff_path, int width,
/              int height, int mono_band, BLOB geom, double horz_res,
/              double vert_res, text compression)
/ WriteMonoBandTiffTfw(text coverage, text tiff_path, int width,
/              int height, int mono_band, BLOB geom, double horz_res,
/              double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_mono_band_tiff (1, context, argc, argv);
}

static void
fnct_WriteMonoBandTiff (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ WriteMonoBandTiff(text coverage, text tiff_path, int width,
/           int height, int mono_band, BLOB geom, double resolution)
/ WriteMonoBandTiff(text coverage, text tiff_path, int width,
/           int height, int mono_band, BLOB geom, double horz_res,
/           double vert_res)
/ WriteMonoBandTiff(text coverage, text tiff_path, int width,
/           int height, int mono_band, BLOB geom, double horz_res,
/           double vert_res, text compression)
/ WriteMonoBandTiff(text coverage, text tiff_path, int width,
/           int height, int mono_band, BLOB geom, double horz_res,
/           double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_mono_band_tiff (0, context, argc, argv);
}

static void
fnct_WriteAsciiGrid (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteAsciiGrid(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution, 
/           int is_centered)
/ WriteTiff(text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution, 
/           int is_centered, int decimal_digits)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *path;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double resolution;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    int errcode = -1;
    gaiaGeomCollPtr geom;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int is_centered = 1;
    int decimal_digits = 4;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER
	&& sqlite3_value_type (argv[5]) != SQLITE_FLOAT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    path = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    blob = sqlite3_value_blob (argv[4]);
    blob_sz = sqlite3_value_bytes (argv[4]);
    if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
      {
	  int ival = sqlite3_value_int (argv[5]);
	  resolution = ival;
      }
    else
	resolution = sqlite3_value_double (argv[5]);
    if (argc > 6)
	is_centered = sqlite3_value_int (argv[6]);
    if (argc > 7)
	decimal_digits = sqlite3_value_int (argv[7]);

    if (decimal_digits < 1)
	decimal_digits = 0;
    if (decimal_digits > 18)
	decimal_digits = 18;

/* coarse args validation */
    if (width < 0 || width > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0 || height > UINT16_MAX)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  errcode = -1;
	  goto error;
      }
    if (is_point (geom))
      {
	  /* assumed to be the GeoTiff Center Point */
	  gaiaPointPtr pt = geom->FirstPoint;
	  double ext_x = (double) width * resolution;
	  double ext_y = (double) height * resolution;
	  minx = pt->X - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt->Y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else
      {
	  /* assumed to be any possible Geometry defining a BBOX */
	  minx = geom->MinX;
	  maxx = geom->MaxX;
	  miny = geom->MinY;
	  maxy = geom->MaxY;
      }
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    ret =
	rl2_export_ascii_grid_from_dbms (sqlite, path, coverage,
					 resolution, minx,
					 miny, maxx, maxy, width,
					 height, is_centered, decimal_digits);
    if (ret != RL2_OK)
      {
	  errcode = 0;
	  goto error;
      }
    rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, 1);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_int (context, errcode);
}

static void
fnct_GetMapImage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetMapImage(text coverage, BLOB geom, int width, int height)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format, text bg_color)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format, text bg_color, int transparent)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format, text bg_color, int transparent,
/             int quality)
/ GetMapImage(text coverage, BLOB geom, int width, int height,
/             text style, text format, text bg_color, int transparent,
/             int quality, int reaspect)
/
/ will return a BLOB containing the Image payload
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    rl2CoveragePtr coverage = NULL;
    rl2PrivCoveragePtr cvg;
    int width;
    int height;
    int base_width;
    int base_height;
    const unsigned char *blob;
    int blob_sz;
    const char *style = "default";
    const char *format = "image/png";
    const char *bg_color = "#ffffff";
    unsigned char bg_red;
    unsigned char bg_green;
    unsigned char bg_blue;
    int transparent = 0;
    int quality = 80;
    int reaspect = 0;
    sqlite3 *sqlite;
    gaiaGeomCollPtr geom;
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
    double xx_res;
    double yy_res;
    double aspect_org;
    double aspect_dst;
    double rescale_x;
    double rescale_y;
    int ok_style;
    int ok_format;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *image = NULL;
    int image_size;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    rl2PalettePtr palette = NULL;
    double confidence;
    unsigned char format_id = RL2_OUTPUT_FORMAT_UNKNOWN;
    unsigned char out_pixel = RL2_PIXEL_UNKNOWN;
    unsigned char *rgba = NULL;
    unsigned char *gray = NULL;
    rl2GraphicsBitmapPtr base_img = NULL;
    rl2GraphicsContextPtr ctx = NULL;
    rl2RasterStylePtr symbolizer = NULL;
    rl2RasterStatisticsPtr stats = NULL;
    double opacity = 1.0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_TEXT)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_TEXT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_TEXT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    blob = sqlite3_value_blob (argv[1]);
    blob_sz = sqlite3_value_bytes (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    if (argc > 4)
	style = (const char *) sqlite3_value_text (argv[4]);
    if (argc > 5)
	format = (const char *) sqlite3_value_text (argv[5]);
    if (argc > 6)
	bg_color = (const char *) sqlite3_value_text (argv[6]);
    if (argc > 7)
	transparent = sqlite3_value_int (argv[7]);
    if (argc > 8)
	quality = sqlite3_value_int (argv[8]);
    if (argc > 9)
	reaspect = sqlite3_value_int (argv[9]);

/* coarse args validation */
    sqlite = sqlite3_context_db_handle (context);
    if (width < 64 || width > 5000)
	goto error;
    if (height < 64 || height > 5000)
	goto error;
/* validating the style */
    ok_style = 0;
    if (strcasecmp (style, "default") == 0)
	ok_style = 1;
    else
      {
	  /* attempting to get a RasterSymbolizer style */
	  symbolizer =
	      rl2_create_raster_style_from_dbms (sqlite, cvg_name, style);
	  if (symbolizer == NULL)
	      goto error;
	  stats = rl2_create_raster_statistics_from_dbms (sqlite, cvg_name);
	  if (stats == NULL)
	      goto error;
	  ok_style = 1;
      }
    if (!ok_style)
	goto error;
/* validating the format */
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
    if (!ok_format)
	goto error;
/* parsing the background color */
    if (rl2_parse_hexrgb (bg_color, &bg_red, &bg_green, &bg_blue) != RL2_OK)
	goto error;

/* checking the Geometry */
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
	goto error;
    minx = geom->MinX;
    maxx = geom->MaxX;
    miny = geom->MinY;
    maxy = geom->MaxY;
    ext_x = maxx - minx;
    ext_y = maxy - miny;
    if (ext_x <= 0.0 || ext_y <= 0.0)
	goto error;
    x_res = ext_x / (double) width;
    y_res = ext_y / (double) height;
    gaiaFreeGeomColl (geom);

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
	goto error;
    if (rl2_get_coverage_srid (coverage, &srid) != RL2_OK)
	srid = -1;
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

    if ((cvg->pixelType == RL2_PIXEL_DATAGRID
	 || cvg->pixelType == RL2_PIXEL_MULTIBAND) && symbolizer == NULL)
      {
	  /* creating a default RasterStyle */
	  rl2PrivRasterStylePtr symb = malloc (sizeof (rl2PrivRasterStyle));
	  symbolizer = (rl2RasterStylePtr) symb;
	  symb->name = malloc (8);
	  strcpy (symb->name, "default");
	  symb->title = NULL;
	  symb->abstract = NULL;
	  symb->opacity = 1.0;
	  symb->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_NONE;
	  symb->bandSelection = malloc (sizeof (rl2PrivBandSelection));
	  symb->bandSelection->selectionType = RL2_BAND_SELECTION_MONO;
	  symb->bandSelection->grayBand = 0;
	  symb->bandSelection->grayContrast = RL2_CONTRAST_ENHANCEMENT_NONE;
	  symb->categorize = NULL;
	  symb->interpolate = NULL;
	  symb->shadedRelief = 0;
	  if (stats == NULL)
	    {
		stats =
		    rl2_create_raster_statistics_from_dbms (sqlite, cvg_name);
		if (stats == NULL)
		    goto error;
	    }
      }

    if (symbolizer != NULL)
      {
	  /* applying a RasterSymbolizer */
	  int yes_no;
	  if (rl2_is_raster_style_triple_band_selected (symbolizer, &yes_no) ==
	      RL2_OK)
	    {
		if ((cvg->sampleType == RL2_SAMPLE_UINT8
		     || cvg->sampleType == RL2_SAMPLE_UINT16)
		    && (cvg->pixelType == RL2_PIXEL_RGB
			|| cvg->pixelType == RL2_PIXEL_MULTIBAND) && yes_no)
		    out_pixel = RL2_PIXEL_RGB;
	    }
	  if (rl2_is_raster_style_mono_band_selected (symbolizer, &yes_no) ==
	      RL2_OK)
	    {
		if ((cvg->sampleType == RL2_SAMPLE_UINT8
		     || cvg->sampleType == RL2_SAMPLE_UINT16)
		    && (cvg->pixelType == RL2_PIXEL_RGB
			|| cvg->pixelType == RL2_PIXEL_MULTIBAND
			|| cvg->pixelType == RL2_PIXEL_GRAYSCALE) && yes_no)
		    out_pixel = RL2_PIXEL_GRAYSCALE;
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
	  if (rl2_get_raster_style_opacity (symbolizer, &opacity) != RL2_OK)
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

/* retrieving the optimal resolution level */
    if (!find_best_resolution_level
	(sqlite, cvg_name, x_res, y_res, &level_id, &scale, &xscale, &xx_res,
	 &yy_res))
	goto error;
    base_width = (int) (ext_x / xx_res);
    base_height = (int) (ext_y / yy_res);
    if ((base_width <= 0 && base_width >= USHRT_MAX)
	|| (base_height <= 0 && base_height >= USHRT_MAX))
	goto error;
    aspect_org = (double) base_width / (double) base_height;
    aspect_dst = (double) width / (double) height;
    confidence = aspect_org / 100.0;
    if (aspect_dst >= (aspect_org - confidence)
	|| aspect_dst <= (aspect_org + confidence))
	;
    else if (aspect_org != aspect_dst && !reaspect)
	goto error;
    rescale_x = (double) width / (double) base_width;
    rescale_y = (double) height / (double) base_height;

    if (out_pixel == RL2_PIXEL_MONOCHROME)
      {
	  if (level_id != 0 && scale != 1)
	      out_pixel = RL2_PIXEL_GRAYSCALE;
      }
    if (out_pixel == RL2_PIXEL_PALETTE)
      {
	  if (level_id != 0 && scale != 1)
	      out_pixel = RL2_PIXEL_RGB;
      }

    if (rl2_get_raw_raster_data_bgcolor
	(sqlite, coverage, base_width, base_height, minx, miny, maxx, maxy,
	 xx_res, yy_res, &outbuf, &outbuf_size, &palette, &out_pixel, bg_red,
	 bg_green, bg_blue, symbolizer, stats) != RL2_OK)
	goto error;
    if (out_pixel == RL2_PIXEL_PALETTE && palette == NULL)
	goto error;
    if (base_width == width && base_height == height)
      {
	  if (out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* converting from Monochrome to Grayscale */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_payload_from_monochrome_transparent
			  (base_width, base_height, outbuf, format_id, quality,
			   &image, &image_size, opacity))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_payload_from_monochrome_opaque
			  (base_width, base_height, sqlite, minx, miny, maxx,
			   maxy, srid, outbuf, format_id, quality, &image,
			   &image_size))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		outbuf = NULL;
	    }
	  else if (out_pixel == RL2_PIXEL_PALETTE)
	    {
		/* Palette */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_payload_from_palette_transparent
			  (base_width, base_height, outbuf, palette, format_id,
			   quality, &image, &image_size, bg_red, bg_green,
			   bg_blue, opacity))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_payload_from_palette_opaque
			  (base_width, base_height, sqlite, minx, miny, maxx,
			   maxy, srid, outbuf, palette, format_id, quality,
			   &image, &image_size))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		outbuf = NULL;
	    }
	  else if (out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_payload_from_grayscale_transparent
			  (base_width, base_height, outbuf, format_id, quality,
			   &image, &image_size, bg_red, opacity))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_payload_from_grayscale_opaque
			  (base_width, base_height, sqlite, minx, miny, maxx,
			   maxy, srid, outbuf, format_id, quality, &image,
			   &image_size))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		outbuf = NULL;
	    }
	  else
	    {
		/* RGB */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_payload_from_rgb_transparent
			  (base_width, base_height, outbuf, format_id, quality,
			   &image, &image_size, bg_red, bg_green, bg_blue,
			   opacity))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_payload_from_rgb_opaque
			  (base_width, base_height, sqlite, minx, miny, maxx,
			   maxy, srid, outbuf, format_id, quality, &image,
			   &image_size))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		outbuf = NULL;
	    }
	  sqlite3_result_blob (context, image, image_size, free);
      }
    else
      {
	  /* rescaling */
	  ctx = rl2_graph_create_context (width, height);
	  if (ctx == NULL)
	      goto error;
	  rgba = malloc (base_width * base_height * 4);
	  if (out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* Monochrome - upsampled */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_rgba_from_monochrome_transparent
			  (base_width, base_height, outbuf, rgba))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_rgba_from_monochrome_opaque
			  (base_width, base_height, outbuf, rgba))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		outbuf = NULL;
	    }
	  else if (out_pixel == RL2_PIXEL_PALETTE)
	    {
		/* Monochrome - upsampled */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_rgba_from_palette_transparent
			  (base_width, base_height, outbuf, palette, rgba,
			   bg_red, bg_green, bg_blue))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_rgba_from_palette_opaque
			  (base_width, base_height, outbuf, palette, rgba))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		outbuf = NULL;
	    }
	  else if (out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_rgba_from_grayscale_transparent
			  (base_width, base_height, outbuf, rgba, bg_red))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_rgba_from_grayscale_opaque
			  (base_width, base_height, outbuf, rgba))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		outbuf = NULL;
	    }
	  else
	    {
		/* RGB */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_rgba_from_rgb_transparent
			  (base_width, base_height, outbuf, rgba, bg_red,
			   bg_green, bg_blue))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_rgba_from_rgb_opaque
			  (base_width, base_height, outbuf, rgba))
			{
			    outbuf = NULL;
			    goto error;
			}
		  }
		outbuf = NULL;
	    }
	  base_img = rl2_graph_create_bitmap (rgba, base_width, base_height);
	  if (base_img == NULL)
	      goto error;
	  rgba = NULL;
	  rl2_graph_draw_rescaled_bitmap (ctx, base_img, rescale_x, rescale_y,
					  0, 0);
	  rl2_graph_destroy_bitmap (base_img);
	  rgb = rl2_graph_get_context_rgb_array (ctx);
	  alpha = NULL;
	  if (transparent)
	      alpha = rl2_graph_get_context_alpha_array (ctx);
	  rl2_graph_destroy_context (ctx);
	  if (rgb == NULL)
	      goto error;
	  if (out_pixel == RL2_PIXEL_GRAYSCALE
	      || out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* Grayscale or Monochrome upsampled */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (alpha == NULL)
			  goto error;
		      if (!get_payload_from_gray_rgba_transparent
			  (width, height, rgb, alpha, format_id, quality,
			   &image, &image_size, opacity))
			{
			    rgb = NULL;
			    alpha = NULL;
			    goto error;
			}
		      rgb = NULL;
		      alpha = NULL;
		  }
		else
		  {
		      if (alpha != NULL)
			  free (alpha);
		      alpha = NULL;
		      if (!get_payload_from_gray_rgba_opaque
			  (width, height, sqlite, minx, miny, maxx, maxy, srid,
			   rgb, format_id, quality, &image, &image_size))
			{
			    rgb = NULL;
			    goto error;
			}
		      rgb = NULL;
		  }
	    }
	  else
	    {
		/* RGB */
		if (transparent && format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (alpha == NULL)
			  goto error;
		      if (!get_payload_from_rgb_rgba_transparent
			  (width, height, rgb, alpha, format_id, quality,
			   &image, &image_size, opacity))
			{
			    rgb = NULL;
			    alpha = NULL;
			    goto error;
			}
		      rgb = NULL;
		      alpha = NULL;
		  }
		else
		  {
		      if (alpha != NULL)
			  free (alpha);
		      alpha = NULL;
		      if (!get_payload_from_rgb_rgba_opaque
			  (width, height, sqlite, minx, miny, maxx, maxy, srid,
			   rgb, format_id, quality, &image, &image_size))
			{
			    rgb = NULL;
			    goto error;
			}
		      rgb = NULL;
		  }
	    }
	  sqlite3_result_blob (context, image, image_size, free);
      }

    rl2_destroy_coverage (coverage);
    if (outbuf != NULL)
	free (outbuf);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (symbolizer != NULL)
	rl2_destroy_raster_style (symbolizer);
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (outbuf != NULL)
	free (outbuf);
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    if (rgba != NULL)
	free (rgba);
    if (gray != NULL)
	free (gray);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (symbolizer != NULL)
	rl2_destroy_raster_style (symbolizer);
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    sqlite3_result_null (context);
}

static void
fnct_GetTileImage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetTileImage(text coverage, int tile_id)
/ GetTileImage(text coverage, int tile_id, text bg_color)
/ GetTileImage(text coverage, int tile_id, text bg_color,
/              int transparent)
/
/ will return a BLOB containing the Image payload
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    rl2CoveragePtr coverage = NULL;
    rl2PrivCoveragePtr cvg;
    sqlite3_int64 tile_id;
    int pyramid_level;
    const char *bg_color = "#ffffff";
    int transparent = 0;
    unsigned char bg_red;
    unsigned char bg_green;
    unsigned char bg_blue;
    unsigned short width;
    unsigned short height;
    sqlite3 *sqlite;
    sqlite3_stmt *stmt = NULL;
    int unsupported_tile;
    int has_palette = 0;
    char *table_tile_data;
    char *xtable_tile_data;
    char *table_tiles;
    char *xtable_tiles;
    char *sql;
    int ret;
    const unsigned char *blob_odd = NULL;
    const unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    rl2RasterPtr raster = NULL;
    rl2PrivRasterPtr rst;
    rl2PalettePtr palette = NULL;
    unsigned char *buffer = NULL;
    unsigned char *mask = NULL;
    unsigned char *rgba = NULL;
    unsigned char *p_rgba;
    unsigned short row;
    unsigned short col;
    unsigned char *image = NULL;
    int image_size;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    unsigned char sample_type;
    unsigned char pixel_type;
    rl2PrivPixelPtr no_data = NULL;
    double opacity = 1.0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    tile_id = sqlite3_value_int64 (argv[1]);
    if (argc > 2)
	bg_color = (const char *) sqlite3_value_text (argv[2]);
    if (argc > 3)
	transparent = sqlite3_value_int (argv[3]);

/* parsing the background color */
    if (rl2_parse_hexrgb (bg_color, &bg_red, &bg_green, &bg_blue) != RL2_OK)
	goto error;

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
	goto error;
    cvg = (rl2PrivCoveragePtr) coverage;
    unsupported_tile = 0;
    switch (cvg->pixelType)
      {
      case RL2_PIXEL_PALETTE:
	  has_palette = 1;
	  break;
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
      case RL2_PIXEL_DATAGRID:
	  break;
      default:
	  unsupported_tile = 1;
	  break;
      }
    if (unsupported_tile)
      {
	  fprintf (stderr, "*** Unsupported Tile Type !!!!\n");
	  goto error;
      }
    if (has_palette)
      {
	  /* loading the Coverage's palette */
	  palette = rl2_get_dbms_palette (sqlite, cvg_name);
	  if (palette == NULL)
	      goto error;
      }
    no_data = cvg->noData;

/* querying the tile */
    table_tile_data = sqlite3_mprintf ("%s_tile_data", cvg_name);
    xtable_tile_data = gaiaDoubleQuotedSql (table_tile_data);
    sqlite3_free (table_tile_data);
    table_tiles = sqlite3_mprintf ("%s_tiles", cvg_name);
    xtable_tiles = gaiaDoubleQuotedSql (table_tiles);
    sqlite3_free (table_tiles);
    sql = sqlite3_mprintf ("SELECT d.tile_data_odd, d.tile_data_even, "
			   "t.pyramid_level FROM \"%s\" AS d "
			   "JOIN \"%s\" AS t ON (t.tile_id = d.tile_id) "
			   "WHERE t.tile_id = ?", xtable_tile_data,
			   xtable_tiles);
    free (xtable_tile_data);
    free (xtable_tiles);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (sqlite));
	  goto error;
      }
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
		pyramid_level = sqlite3_column_int (stmt, 2);
		if (has_palette && pyramid_level > 0)
		  {
		      if (palette != NULL)
			  rl2_destroy_palette (palette);
		      palette = NULL;
		  }
		raster =
		    rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz,
				       blob_even, blob_even_sz, palette);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      goto error;
		  }
		rst = (rl2PrivRasterPtr) raster;
		width = rst->width;
		height = rst->height;
		sample_type = rst->sampleType;
		pixel_type = rst->pixelType;
		buffer = rst->rasterBuffer;
		mask = rst->maskBuffer;
		palette = (rl2PalettePtr) (rst->Palette);
		rst->rasterBuffer = NULL;
		rst->maskBuffer = NULL;
		rst->Palette = NULL;
		rl2_destroy_raster (raster);
		rgba = malloc (width * height * 4);
		if (rgba == NULL)
		    goto error;
		/* priming the image background */
		p_rgba = rgba;
		for (row = 0; row < height; row++)
		  {
		      for (col = 0; col < width; col++)
			{
			    *p_rgba++ = bg_red;
			    *p_rgba++ = bg_green;
			    *p_rgba++ = bg_blue;
			    *p_rgba++ = 0;	/* transparent */
			}
		  }
		switch (pixel_type)
		  {
		  case RL2_PIXEL_MONOCHROME:
		      ret =
			  get_rgba_from_monochrome_mask (width, height, buffer,
							 mask, no_data, rgba);
		      buffer = NULL;
		      mask = NULL;
		      if (!ret)
			  goto error;
		      if (!build_rgb_alpha
			  (width, height, rgba, &rgb, &alpha, bg_red, bg_green,
			   bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_gray_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity))
				goto error;
			}
		      else
			{
			    free (alpha);
			    alpha = NULL;
			    if (!get_payload_from_gray_rgba_opaque
				(width, height, sqlite, 0, 0, 0, 0, -1,
				 rgb, RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size))
				goto error;
			}
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  case RL2_PIXEL_PALETTE:
		      ret =
			  get_rgba_from_palette_mask (width, height, buffer,
						      mask, palette, no_data,
						      rgba);
		      buffer = NULL;
		      mask = NULL;
		      if (!ret)
			  goto error;
		      if (!build_rgb_alpha
			  (width, height, rgba, &rgb, &alpha, bg_red, bg_green,
			   bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_rgb_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity))
				goto error;
			}
		      else
			{
			    free (alpha);
			    alpha = NULL;
			    if (!get_payload_from_rgb_rgba_opaque
				(width, height, sqlite, 0, 0, 0, 0, -1,
				 rgb, RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size))
				goto error;
			}
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  case RL2_PIXEL_GRAYSCALE:
		      ret =
			  get_rgba_from_grayscale_mask (width, height, buffer,
							mask, no_data, rgba);
		      buffer = NULL;
		      mask = NULL;
		      if (!ret)
			  goto error;
		      if (!build_rgb_alpha
			  (width, height, rgba, &rgb, &alpha, bg_red, bg_green,
			   bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_gray_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity))
				goto error;
			}
		      else
			{
			    free (alpha);
			    alpha = NULL;
			    if (!get_payload_from_gray_rgba_opaque
				(width, height, sqlite, 0, 0, 0, 0, -1,
				 rgb, RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size))
				goto error;
			}
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  case RL2_PIXEL_DATAGRID:
		      ret =
			  get_rgba_from_datagrid_mask (width, height,
						       sample_type, buffer,
						       mask, no_data, rgba);
		      buffer = NULL;
		      mask = NULL;
		      if (!ret)
			  goto error;
		      if (!build_rgb_alpha
			  (width, height, rgba, &rgb, &alpha, bg_red, bg_green,
			   bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_gray_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity))
				goto error;
			}
		      else
			{
			    free (alpha);
			    alpha = NULL;
			    if (!get_payload_from_gray_rgba_opaque
				(width, height, sqlite, 0, 0, 0, 0, -1,
				 rgb, RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size))
				goto error;
			}
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  case RL2_PIXEL_RGB:
		      if (sample_type == RL2_SAMPLE_UINT16)
			{
			    ret =
				get_rgba_from_multiband16 (width, height, 0, 1,
							   2, 3,
							   (unsigned short *)
							   buffer, mask,
							   no_data, rgba);
			}
		      else
			{
			    ret =
				get_rgba_from_rgb_mask (width, height, buffer,
							mask, no_data, rgba);
			}
		      buffer = NULL;
		      mask = NULL;
		      if (!ret)
			  goto error;
		      if (!build_rgb_alpha
			  (width, height, rgba, &rgb, &alpha, bg_red, bg_green,
			   bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_rgb_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity))
				goto error;
			}
		      else
			{
			    free (alpha);
			    alpha = NULL;
			    if (!get_payload_from_rgb_rgba_opaque
				(width, height, sqlite, 0, 0, 0, 0, -1,
				 rgb, RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size))
				goto error;
			}
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  default:
		      goto error;
		      break;
		  };
		break;
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

    rl2_destroy_coverage (coverage);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (buffer != NULL)
	free (buffer);
    if (mask != NULL)
	free (mask);
    if (rgba != NULL)
	free (rgba);
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    sqlite3_result_null (context);
}

static int
get_triple_band_tile_image (sqlite3_context * context, const char *cvg_name,
			    sqlite3_int64 tile_id, unsigned char red_band,
			    unsigned char green_band, unsigned char blue_band,
			    unsigned char bg_red, unsigned char bg_green,
			    unsigned char bg_blue, int transparent)
{
/* common implementation: TileImage (mono/triple-band) */
    rl2CoveragePtr coverage = NULL;
    rl2PrivCoveragePtr cvg;
    unsigned short width;
    unsigned short height;
    sqlite3 *sqlite;
    sqlite3_stmt *stmt = NULL;
    int unsupported_tile;
    char *table_tile_data;
    char *xtable_tile_data;
    char *table_tiles;
    char *xtable_tiles;
    char *sql;
    int ret;
    const unsigned char *blob_odd = NULL;
    const unsigned char *blob_even = NULL;
    int blob_odd_sz;
    int blob_even_sz;
    rl2RasterPtr raster = NULL;
    rl2PrivRasterPtr rst;
    unsigned char *buffer = NULL;
    unsigned char *mask = NULL;
    unsigned char *rgba = NULL;
    unsigned char *p_rgba;
    unsigned short row;
    unsigned short col;
    unsigned char *image = NULL;
    int image_size;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    unsigned char sample_type;
    unsigned char num_bands;
    rl2PrivPixelPtr no_data = NULL;
    double opacity = 1.0;

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, cvg_name);
    if (coverage == NULL)
	goto error;
    cvg = (rl2PrivCoveragePtr) coverage;
    unsupported_tile = 0;
    switch (cvg->pixelType)
      {
      case RL2_PIXEL_RGB:
      case RL2_PIXEL_MULTIBAND:
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      || cvg->sampleType == RL2_SAMPLE_UINT16)
	      break;
	  unsupported_tile = 1;
	  break;
      case RL2_PIXEL_DATAGRID:
	  if (cvg->sampleType == RL2_SAMPLE_INT8
	      || cvg->sampleType == RL2_SAMPLE_UINT8
	      || cvg->sampleType == RL2_SAMPLE_INT16
	      || cvg->sampleType == RL2_SAMPLE_UINT16
	      || cvg->sampleType == RL2_SAMPLE_INT32
	      || cvg->sampleType == RL2_SAMPLE_UINT32
	      || cvg->sampleType == RL2_SAMPLE_FLOAT
	      || cvg->sampleType == RL2_SAMPLE_DOUBLE)
	      break;
	  unsupported_tile = 1;
      default:
	  unsupported_tile = 1;
	  break;
      }
    if (unsupported_tile)
      {
	  fprintf (stderr, "*** Unsupported Tile Type !!!!\n");
	  goto error;
      }
    if (red_band >= cvg->nBands)
	goto error;
    if (green_band >= cvg->nBands)
	goto error;
    if (blue_band >= cvg->nBands)
	goto error;
    no_data = cvg->noData;

/* querying the tile */
    table_tile_data = sqlite3_mprintf ("%s_tile_data", cvg_name);
    xtable_tile_data = gaiaDoubleQuotedSql (table_tile_data);
    sqlite3_free (table_tile_data);
    table_tiles = sqlite3_mprintf ("%s_tiles", cvg_name);
    xtable_tiles = gaiaDoubleQuotedSql (table_tiles);
    sqlite3_free (table_tiles);
    sql = sqlite3_mprintf ("SELECT d.tile_data_odd, d.tile_data_even, "
			   "t.pyramid_level FROM \"%s\" AS d "
			   "JOIN \"%s\" AS t ON (t.tile_id = d.tile_id) "
			   "WHERE t.tile_id = ?", xtable_tile_data,
			   xtable_tiles);
    free (xtable_tile_data);
    free (xtable_tiles);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (sqlite));
	  goto error;
      }
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
		      goto error;
		  }
		rst = (rl2PrivRasterPtr) raster;
		width = rst->width;
		height = rst->height;
		sample_type = rst->sampleType;
		num_bands = rst->nBands;
		buffer = rst->rasterBuffer;
		mask = rst->maskBuffer;
		rst->rasterBuffer = NULL;
		rst->maskBuffer = NULL;
		rl2_destroy_raster (raster);
		rgba = malloc (width * height * 4);
		if (rgba == NULL)
		    goto error;
		/* priming the image background */
		p_rgba = rgba;
		for (row = 0; row < height; row++)
		  {
		      for (col = 0; col < width; col++)
			{
			    *p_rgba++ = bg_red;
			    *p_rgba++ = bg_green;
			    *p_rgba++ = bg_blue;
			    *p_rgba++ = 0;	/* transparent */
			}
		  }
		switch (sample_type)
		  {
		  case RL2_SAMPLE_UINT8:
		      ret =
			  get_rgba_from_multiband8 (width, height, red_band,
						    green_band, blue_band,
						    num_bands, buffer, mask,
						    no_data, rgba);
		      buffer = NULL;
		      mask = NULL;
		      if (!ret)
			  goto error;
		      if (!build_rgb_alpha
			  (width, height, rgba, &rgb, &alpha, bg_red, bg_green,
			   bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_rgb_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity))
				goto error;
			}
		      else
			{
			    free (alpha);
			    alpha = NULL;
			    if (!get_payload_from_rgb_rgba_opaque
				(width, height, sqlite, 0, 0, 0, 0, -1,
				 rgb, RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size))
				goto error;
			}
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  case RL2_SAMPLE_UINT16:
		      ret =
			  get_rgba_from_multiband16 (width, height, red_band,
						     green_band, blue_band,
						     num_bands,
						     (unsigned short *) buffer,
						     mask, no_data, rgba);
		      if (!build_rgb_alpha
			  (width, height, rgba, &rgb, &alpha, bg_red, bg_green,
			   bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_rgb_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity))
				goto error;
			}
		      else
			{
			    free (alpha);
			    alpha = NULL;
			    if (!get_payload_from_rgb_rgba_opaque
				(width, height, sqlite, 0, 0, 0, 0, -1,
				 rgb, RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size))
				goto error;
			}
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  default:
		      goto error;
		      break;
		  };
		break;
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

    rl2_destroy_coverage (coverage);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (buffer != NULL)
	free (buffer);
    if (mask != NULL)
	free (mask);
    if (rgba != NULL)
	free (rgba);
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    return 0;
}

static void
fnct_GetTripleBandTileImage (sqlite3_context * context, int argc,
			     sqlite3_value ** argv)
{
/* SQL function:
/ GetTripleBandTileImage(text coverage, int tile_id, int red_band,
/                          int green_band, int blue_band)
/ GetTripleBandTileImage(text coverage, int tile_id, int red_band,
/                          int green_band, int blue_band, text bg_color)
/ GetTripleBandTileImage(text coverage, int tile_id, int red_band,
/                          int green_band, int blue_band, text bg_color,
/                          int transparent)
/
/ will return a BLOB containing the Image payload
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    sqlite3_int64 tile_id;
    const char *bg_color = "#ffffff";
    int transparent = 0;
    int red_band;
    int green_band;
    int blue_band;
    unsigned char bg_red;
    unsigned char bg_green;
    unsigned char bg_blue;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_TEXT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    tile_id = sqlite3_value_int64 (argv[1]);
    red_band = sqlite3_value_int (argv[2]);
    green_band = sqlite3_value_int (argv[3]);
    blue_band = sqlite3_value_int (argv[4]);
    if (argc > 5)
	bg_color = (const char *) sqlite3_value_text (argv[5]);
    if (argc > 6)
	transparent = sqlite3_value_int (argv[6]);

/* coarse args validation */
    if (red_band < 0 || red_band > 255)
	goto error;
    if (green_band < 0 || green_band > 255)
	goto error;
    if (blue_band < 0 || blue_band > 255)
	goto error;

/* parsing the background color */
    if (rl2_parse_hexrgb (bg_color, &bg_red, &bg_green, &bg_blue) != RL2_OK)
	goto error;

    if (get_triple_band_tile_image
	(context, cvg_name, tile_id, red_band, green_band, blue_band, bg_red,
	 bg_green, bg_blue, transparent))
	return;

  error:
    sqlite3_result_null (context);
}

static void
fnct_GetMonoBandTileImage (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ GetMonoBandTileImage(text coverage, int tile_id, int mono_band)
/ GetMonoBandTileImage(text coverage, int tile_id, int mono_band,
/                      text bg_color)
/ GetMonoBandTileImage(text coverage, int tile_id, int mono_band,
/                      text bg_color, int transparent)
/
/ will return a BLOB containing the Image payload
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    sqlite3_int64 tile_id;
    const char *bg_color = "#ffffff";
    int transparent = 0;
    int mono_band;
    unsigned char bg_red;
    unsigned char bg_green;
    unsigned char bg_blue;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 3 && sqlite3_value_type (argv[3]) != SQLITE_TEXT)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    tile_id = sqlite3_value_int64 (argv[1]);
    mono_band = sqlite3_value_int (argv[2]);
    if (argc > 3)
	bg_color = (const char *) sqlite3_value_text (argv[3]);
    if (argc > 4)
	transparent = sqlite3_value_int (argv[4]);

/* coarse args validation */
    if (mono_band < 0 || mono_band > 255)
	goto error;

/* parsing the background color */
    if (rl2_parse_hexrgb (bg_color, &bg_red, &bg_green, &bg_blue) != RL2_OK)
	goto error;

    if (get_triple_band_tile_image
	(context, cvg_name, tile_id, mono_band, mono_band, mono_band, bg_red,
	 bg_green, bg_blue, transparent))
	return;

  error:
    sqlite3_result_null (context);
}

static void
register_rl2_sql_functions (void *p_db)
{
    sqlite3 *db = p_db;
    const char *security_level;
    sqlite3_create_function (db, "rl2_version", 0, SQLITE_ANY, 0,
			     fnct_rl2_version, 0, 0);
    sqlite3_create_function (db, "rl2_target_cpu", 0, SQLITE_ANY, 0,
			     fnct_rl2_target_cpu, 0, 0);
    sqlite3_create_function (db, "IsValidPixel", 3, SQLITE_ANY, 0,
			     fnct_IsValidPixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidPixel", 3, SQLITE_ANY, 0,
			     fnct_IsValidPixel, 0, 0);
    sqlite3_create_function (db, "IsValidRasterPalette", 2, SQLITE_ANY, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterPalette", 2, SQLITE_ANY, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 2, SQLITE_ANY, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterStatistics", 2, SQLITE_ANY,
			     0, fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 3, SQLITE_ANY, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterStatistics", 3, SQLITE_ANY,
			     0, fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterTile", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterTile, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterTile", 4, SQLITE_ANY, 0,
			     fnct_IsValidRasterTile, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 10, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 11, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "CreateCoverage", 12, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 10, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 11, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateCoverage", 12, SQLITE_ANY, 0,
			     fnct_CreateCoverage, 0, 0);
    sqlite3_create_function (db, "DeleteSection", 2, SQLITE_ANY, 0,
			     fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "RL2_DeleteSection", 2, SQLITE_ANY, 0,
			     fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "DeleteSection", 3, SQLITE_ANY, 0,
			     fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "RL2_DeleteSection", 3, SQLITE_ANY, 0,
			     fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "DropCoverage", 1, SQLITE_ANY, 0,
			     fnct_DropCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_DropCoverage", 1, SQLITE_ANY, 0,
			     fnct_DropCoverage, 0, 0);
    sqlite3_create_function (db, "DropCoverage", 2, SQLITE_ANY, 0,
			     fnct_DropCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_DropCoverage", 2, SQLITE_ANY, 0,
			     fnct_DropCoverage, 0, 0);
    sqlite3_create_function (db, "GetPaletteNumEntries", 1, SQLITE_ANY, 0,
			     fnct_GetPaletteNumEntries, 0, 0);
    sqlite3_create_function (db, "RL2_GetPaletteNumEntries", 1, SQLITE_ANY, 0,
			     fnct_GetPaletteNumEntries, 0, 0);
    sqlite3_create_function (db, "GetPaletteColorEntry", 2, SQLITE_ANY, 0,
			     fnct_GetPaletteColorEntry, 0, 0);
    sqlite3_create_function (db, "RL2_GetPaletteColorEntry", 2, SQLITE_ANY, 0,
			     fnct_GetPaletteColorEntry, 0, 0);
    sqlite3_create_function (db, "SetPaletteColorEntry", 3, SQLITE_ANY, 0,
			     fnct_SetPaletteColorEntry, 0, 0);
    sqlite3_create_function (db, "RL2_SetPaletteColorEntry", 3, SQLITE_ANY, 0,
			     fnct_SetPaletteColorEntry, 0, 0);
    sqlite3_create_function (db, "PaletteEquals", 2, SQLITE_ANY, 0,
			     fnct_PaletteEquals, 0, 0);
    sqlite3_create_function (db, "RL2_PaletteEquals", 2, SQLITE_ANY, 0,
			     fnct_PaletteEquals, 0, 0);
    sqlite3_create_function (db, "CreatePixel", 3, SQLITE_ANY, 0,
			     fnct_CreatePixel, 0, 0);
    sqlite3_create_function (db, "RL2_CreatePixel", 3, SQLITE_ANY, 0,
			     fnct_CreatePixel, 0, 0);
    sqlite3_create_function (db, "GetPixelType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "GetPixelSampleType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelSampleType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelSampleType", 1, SQLITE_ANY, 0,
			     fnct_GetPixelSampleType, 0, 0);
    sqlite3_create_function (db, "GetPixelNumBands", 1, SQLITE_ANY, 0,
			     fnct_GetPixelNumBands, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelNumBands", 1, SQLITE_ANY, 0,
			     fnct_GetPixelNumBands, 0, 0);
    sqlite3_create_function (db, "GetPixelValue", 2, SQLITE_ANY, 0,
			     fnct_GetPixelValue, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelValue", 2, SQLITE_ANY, 0,
			     fnct_GetPixelValue, 0, 0);
    sqlite3_create_function (db, "SetPixelValue", 3, SQLITE_ANY, 0,
			     fnct_SetPixelValue, 0, 0);
    sqlite3_create_function (db, "RL2_SetPixelValue", 3, SQLITE_ANY, 0,
			     fnct_SetPixelValue, 0, 0);
    sqlite3_create_function (db, "IsTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_IsTransparentPixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_IsTransparentPixel, 0, 0);
    sqlite3_create_function (db, "IsOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_IsOpaquePixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_IsOpaquePixel, 0, 0);
    sqlite3_create_function (db, "SetTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_SetTransparentPixel, 0, 0);
    sqlite3_create_function (db, "RL2_SetTransparentPixel", 1, SQLITE_ANY, 0,
			     fnct_SetTransparentPixel, 0, 0);
    sqlite3_create_function (db, "SetOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_SetOpaquePixel, 0, 0);
    sqlite3_create_function (db, "RL2_SetOpaquePixel", 1, SQLITE_ANY, 0,
			     fnct_SetOpaquePixel, 0, 0);
    sqlite3_create_function (db, "PixelEquals", 2, SQLITE_ANY, 0,
			     fnct_PixelEquals, 0, 0);
    sqlite3_create_function (db, "RL2_PixelEquals", 2, SQLITE_ANY, 0,
			     fnct_PixelEquals, 0, 0);
    sqlite3_create_function (db, "GetRasterStatistics_NoDataPixelsCount", 1,
			     SQLITE_ANY, 0,
			     fnct_GetRasterStatistics_NoDataPixelsCount, 0, 0);
    sqlite3_create_function (db, "RL2_GetRasterStatistics_NoDataPixelsCount", 1,
			     SQLITE_ANY, 0,
			     fnct_GetRasterStatistics_NoDataPixelsCount, 0, 0);
    sqlite3_create_function (db, "GetRasterStatistics_ValidPixelsCount", 1,
			     SQLITE_ANY, 0,
			     fnct_GetRasterStatistics_ValidPixelsCount, 0, 0);
    sqlite3_create_function (db, "RL2_GetRasterStatistics_ValidPixelsCount", 1,
			     SQLITE_ANY, 0,
			     fnct_GetRasterStatistics_ValidPixelsCount, 0, 0);
    sqlite3_create_function (db, "GetRasterStatistics_SampleType", 1,
			     SQLITE_ANY, 0, fnct_GetRasterStatistics_SampleType,
			     0, 0);
    sqlite3_create_function (db, "RL2_GetRasterStatistics_SampleType", 1,
			     SQLITE_ANY, 0, fnct_GetRasterStatistics_SampleType,
			     0, 0);
    sqlite3_create_function (db, "GetRasterStatistics_BandsCount", 1,
			     SQLITE_ANY, 0, fnct_GetRasterStatistics_BandsCount,
			     0, 0);
    sqlite3_create_function (db, "RL2_GetRasterStatistics_BandsCount", 1,
			     SQLITE_ANY, 0, fnct_GetRasterStatistics_BandsCount,
			     0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Min", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_Min, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Min", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_Min, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Max", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_Max, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Max", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_Max, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Avg", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_Avg, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Avg", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_Avg, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Var", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_Var, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Var", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_Var, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_StdDev", 2, SQLITE_ANY, 0,
			     fnct_GetBandStatistics_StdDev, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_StdDev", 2, SQLITE_ANY,
			     0, fnct_GetBandStatistics_StdDev, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Histogram", 2, SQLITE_ANY,
			     0, fnct_GetBandStatistics_Histogram, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Histogram", 2,
			     SQLITE_ANY, 0, fnct_GetBandStatistics_Histogram, 0,
			     0);
    sqlite3_create_function (db, "GetBandHistogramFromImage", 3, SQLITE_ANY,
			     0, fnct_GetBandHistogramFromImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandHistogramFromImage", 3,
			     SQLITE_ANY, 0, fnct_GetBandHistogramFromImage, 0,
			     0);
    sqlite3_create_function (db, "Pyramidize", 1, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 1, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 2, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 2, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 3, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 3, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 4, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 4, SQLITE_ANY, 0,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 1, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 1, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 2, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 2, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 3, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 3, SQLITE_ANY, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 4, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 4, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 5, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 5, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 6, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 6, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 7, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 7, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 8, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 8, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 9, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 9, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetMapImage", 10, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImage", 10, SQLITE_ANY, 0,
			     fnct_GetMapImage, 0, 0);
    sqlite3_create_function (db, "GetTileImage", 2, SQLITE_ANY, 0,
			     fnct_GetTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTileImage", 2, SQLITE_ANY, 0,
			     fnct_GetTileImage, 0, 0);
    sqlite3_create_function (db, "GetTileImage", 3, SQLITE_ANY, 0,
			     fnct_GetTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTileImage", 3, SQLITE_ANY, 0,
			     fnct_GetTileImage, 0, 0);
    sqlite3_create_function (db, "GetTileImage", 4, SQLITE_ANY, 0,
			     fnct_GetTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTileImage", 4, SQLITE_ANY, 0,
			     fnct_GetTileImage, 0, 0);
    sqlite3_create_function (db, "GetTripleBandTileImage", 5, SQLITE_ANY, 0,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTripleBandTileImage", 5, SQLITE_ANY, 0,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetTripleBandTileImage", 6, SQLITE_ANY, 0,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTripleBandTileImage", 6, SQLITE_ANY, 0,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetTripleBandTileImage", 7, SQLITE_ANY, 0,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTripleBandTileImage", 7, SQLITE_ANY, 0,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetMonoBandTileImage", 3, SQLITE_ANY, 0,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMonoBandTileImage", 3, SQLITE_ANY, 0,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetMonoBandTileImage", 4, SQLITE_ANY, 0,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMonoBandTileImage", 4, SQLITE_ANY, 0,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetMonoBandTileImage", 5, SQLITE_ANY, 0,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMonoBandTileImage", 5, SQLITE_ANY, 0,
			     fnct_GetMonoBandTileImage, 0, 0);

/*
// enabling ImportRaster and ExportRaster
//
// these functions could potentially introduce serious security issues,
// most notably when invoked from within some Trigger
// - BlobToFile: some arbitrary code, possibly harmful (e.g. virus or 
//   trojan) could be installed on the local file-system, the user being
//   completely unaware of this
// - ImportRaster: some file could be maliciously "stolen" from the local
//   file system and then inserted into the DB
// - ExportRaster could eventually install some malicious payload into the
//   local filesystem; or could potentially flood the local file-system by
//   outputting a huge size of data
//
// so by default such functions are disabled.
// if for any good/legitimate reason the user really wants to enable them
// the following environment variable has to be explicitly declared:
//
// SPATIALITE_SECURITY=relaxed
//
*/
    security_level = getenv ("SPATIALITE_SECURITY");
    if (security_level == NULL)
	;
    else if (strcasecmp (security_level, "relaxed") == 0)
      {
	  sqlite3_create_function (db, "LoadRaster", 2, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 3, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 4, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 5, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 6, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 2, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 3, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 4, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 5, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 6, SQLITE_ANY, 0,
				   fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 2, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 3, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 4, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 5, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 6, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 7, SQLITE_ANY, 0,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 2, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 3, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 4, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 5, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 6, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 7, SQLITE_ANY,
				   0, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 9, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 9, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 10, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 10, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 11, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 11, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 12, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 12, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 13, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 13, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 14, SQLITE_ANY, 0,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 14, SQLITE_ANY,
				   0, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 6, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 6, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 7, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 7, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 8, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 8, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 9, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 9, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 10, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 10, SQLITE_ANY, 0,
				   fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 6, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 6, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 7, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 7, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 8, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 8, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 9, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 9, SQLITE_ANY, 0,
				   fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 6, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 6, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 7, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 7, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 8, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 8, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 9, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 9, SQLITE_ANY, 0,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteJpegJgw", 6, SQLITE_ANY, 0,
				   fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpegJgw", 6, SQLITE_ANY, 0,
				   fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteJpegJgw", 7, SQLITE_ANY, 0,
				   fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpegJgw", 7, SQLITE_ANY, 0,
				   fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteJpegJgw", 8, SQLITE_ANY, 0,
				   fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpegJgw", 8, SQLITE_ANY, 0,
				   fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteJpeg", 6, SQLITE_ANY, 0,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpeg", 6, SQLITE_ANY, 0,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteJpeg", 7, SQLITE_ANY, 0,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpeg", 7, SQLITE_ANY, 0,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteJpeg", 8, SQLITE_ANY, 0,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpeg", 8, SQLITE_ANY, 0,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 9,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 9,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 10,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 10,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 11,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 11,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 12,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 12,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 13,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 13,
				   SQLITE_ANY, 0, fnct_WriteTripleBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 7,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 7,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 8,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 8,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 9,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 9,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 10,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 10,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 11,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 11,
				   SQLITE_ANY, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiffTfw", 9,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiffTfw", 9,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiffTfw", 10,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiffTfw", 10,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiffTfw", 11,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiffTfw", 11,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiffTfw", 12,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiffTfw", 12,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiffTfw", 7,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiffTfw", 7,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiffTfw", 8,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiffTfw", 8,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiffTfw", 9,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiffTfw", 9,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiffTfw", 10,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiffTfw", 10,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiffTfw,
				   0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiff", 9, SQLITE_ANY,
				   0, fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiff", 9,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiff, 0,
				   0);
	  sqlite3_create_function (db, "WriteTripleBandTiff", 10, SQLITE_ANY,
				   0, fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiff", 10,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiff, 0,
				   0);
	  sqlite3_create_function (db, "WriteTripleBandTiff", 11, SQLITE_ANY,
				   0, fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiff", 11,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiff, 0,
				   0);
	  sqlite3_create_function (db, "WriteTripleBandTiff", 12, SQLITE_ANY,
				   0, fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiff", 12,
				   SQLITE_ANY, 0, fnct_WriteTripleBandTiff, 0,
				   0);
	  sqlite3_create_function (db, "WriteMonoBandTiff", 7, SQLITE_ANY,
				   0, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiff", 7,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiff", 8, SQLITE_ANY,
				   0, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiff", 8,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiff", 9, SQLITE_ANY,
				   0, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiff", 9,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiff", 10, SQLITE_ANY,
				   0, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiff", 10,
				   SQLITE_ANY, 0, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 6, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 6, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 7, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 7, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 8, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 8, SQLITE_ANY, 0,
				   fnct_WriteAsciiGrid, 0, 0);
      }
}

static void
rl2_splash_screen (int verbose)
{
    if (isatty (1))
      {
	  /* printing "hello" message only when stdout is on console */
	  if (verbose)
	    {
		printf ("RasterLite-2 version : %s\n", rl2_version ());
		printf ("TARGET CPU ..........: %s\n", rl2_target_cpu ());
	    }
      }
}

#ifndef LOADABLE_EXTENSION

RL2_DECLARE void
rl2_init (sqlite3 * handle, int verbose)
{
/* used when SQLite initializes as an ordinary lib */
    register_rl2_sql_functions (handle);
    rl2_splash_screen (verbose);
}

#else /* built as LOADABLE EXTENSION only */

SQLITE_EXTENSION_INIT1 static int
init_rl2_extension (sqlite3 * db, char **pzErrMsg,
		    const sqlite3_api_routines * pApi)
{
    SQLITE_EXTENSION_INIT2 (pApi);

    register_rl2_sql_functions (db);
    return 0;
}

#if !(defined _WIN32) || defined(__MINGW32__)
/* MSVC is unable to understand this declaration */
__attribute__ ((visibility ("default")))
#endif
     RL2_DECLARE int
	 sqlite3_modrasterlite_init (sqlite3 * db, char **pzErrMsg,
				     const sqlite3_api_routines * pApi)
{
/* SQLite invokes this routine once when it dynamically loads the extension. */
    return init_rl2_extension (db, pzErrMsg, pApi);
}

#endif
