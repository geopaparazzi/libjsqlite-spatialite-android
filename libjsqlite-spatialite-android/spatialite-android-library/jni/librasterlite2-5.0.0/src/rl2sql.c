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
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2wms.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

#define RL2_UNUSED() if (argc || argv) argc = argc;

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%I64d\n"
#else
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%lld\n"
#endif

#define MAX_FONT_SIZE	2*1024*1024

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
fnct_rl2_cairo_version (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ rl2_cairo_version()
/
/ return a text string representing the current Cairo version
*/
    int len;
    const char *p_result = rl2_cairo_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_curl_version (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ rl2_curl_version()
/
/ return a text string representing the current cURL version
*/
    int len;
    const char *p_result = rl2_curl_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_zlib_version (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ rl2_zlib_version()
/
/ return a text string representing the current zlib version
*/
    int len;
    const char *p_result = rl2_zlib_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_lzma_version (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ rl2_lzma_version()
/
/ return a text string representing the current LZMA version
*/
    int len;
    const char *p_result = rl2_lzma_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_lz4_version (sqlite3_context * context, int argc,
		      sqlite3_value ** argv)
{
/* SQL function:
/ rl2_lz4_version()
/
/ return a text string representing the current LZ4 version
*/
    int len;
    const char *p_result = rl2_lz4_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_zstd_version (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ rl2_zstd_version()
/
/ return a text string representing the current ZSTD version
*/
    int len;
    const char *p_result = rl2_zstd_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_png_version (sqlite3_context * context, int argc,
		      sqlite3_value ** argv)
{
/* SQL function:
/ rl2_png_version()
/
/ return a text string representing the current PNG version
*/
    int len;
    const char *p_result = rl2_png_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_jpeg_version (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ rl2_jpeg_version()
/
/ return a text string representing the current JPEG version
*/
    int len;
    const char *p_result = rl2_jpeg_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_tiff_version (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ rl2_tiff_version()
/
/ return a text string representing the current TIFF version
*/
    int len;
    const char *p_result = rl2_tiff_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_geotiff_version (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ rl2_geotiff_version()
/
/ return a text string representing the current GeoTIFF version
*/
    int len;
    const char *p_result = rl2_geotiff_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_webp_version (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ rl2_webp_version()
/
/ return a text string representing the current WEBP version
*/
    int len;
    const char *p_result = rl2_webp_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_charLS_version (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ rl2_charLS_version()
/
/ return a text string representing the current CharLS version
*/
    int len;
    const char *p_result = rl2_charLS_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_openJPEG_version (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ rl2_openJPEG_version()
/
/ return a text string representing the current OpenJPEG version
*/
    int len;
    const char *p_result = rl2_openJPEG_version ();
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    len = strlen (p_result);
    sqlite3_result_text (context, p_result, len, SQLITE_TRANSIENT);
}

static void
fnct_rl2_has_codec_none (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_none()
/
/ will always return 1 (TRUE)
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_NONE);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_deflate (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_deflate()
/
/ will always return 1 (TRUE)
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_DEFLATE);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_deflate_no (sqlite3_context * context, int argc,
			       sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_deflate_no()
/
/ will always return 1 (TRUE)
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_DEFLATE_NO);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_png (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_png()
/
/ will always return 1 (TRUE)
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_PNG);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_jpeg (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_jpeg()
/
/ will always return 1 (TRUE)
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_JPEG);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_fax4 (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_fax4()
/
/ will always return 1 (TRUE)
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_CCITTFAX4);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_lzma (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_lzma()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_LZMA
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_LZMA);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_lzma_no (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_lzma_no()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_LZMA
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_LZMA_NO);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_lz4 (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_lz4()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_LZ4
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_LZ4);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_lz4_no (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_lz4_no()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_LZ4
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_LZ4_NO);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_zstd (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_zstd()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_ZSTD
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_ZSTD);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_zstd_no (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_zstd_no()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_ZSTD
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_ZSTD_NO);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_charls (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_charls()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_CHARLS
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_CHARLS);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_webp (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_webp()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_WEBP
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_LOSSY_WEBP);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_ll_webp (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_ll_webp()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_WEBP
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_LOSSLESS_WEBP);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_jp2 (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_jp2()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_OPENJPEG
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_LOSSY_JP2);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
}

static void
fnct_rl2_has_codec_ll_jp2 (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ rl2_has_codec_ll_jp2()
/
/ will return 1 (TRUE) or 0 (FALSE) depending of OMIT_OPENJPEG
*/
    int ret = rl2_is_supported_codec (RL2_COMPRESSION_LOSSLESS_JP2);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (ret < 0)
	ret = 0;
    sqlite3_result_int (context, ret);
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
fnct_GetMaxThreads (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ RL2_GetMaxThreads()
/
/ return the currently set Max number of concurrent threads
*/
    int max_threads = 1;
    struct rl2_private_data *priv_data = sqlite3_user_data (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (priv_data != NULL)
	max_threads = priv_data->max_threads;
    sqlite3_result_int (context, max_threads);
}

static void
fnct_SetMaxThreads (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ RL2_SetMaxThreads(INTEGER max)
/
/ return the currently set Max number of concurrent threads (after this call)
/ -1 on invalid arguments
*/
    int max_threads = 1;
    struct rl2_private_data *priv_data = sqlite3_user_data (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
	max_threads = sqlite3_value_int (argv[0]);
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* normalizing between 1 and 64 */
    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;

    if (priv_data != NULL)
	priv_data->max_threads = max_threads;
    else
	max_threads = 1;
    sqlite3_result_int (context, max_threads);
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
fnct_IsPixelNone (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsPixelNone(BLOBencoded pixel)
/
/ will return 1 (TRUE, it's a NONE Pixel) or 0 (FALSE, no it's not a NONE Pixel)
/ or -1 (INVALID ARGS)
/
*/
    const unsigned char *blob;
    int blob_sz;
    int err = 0;
    rl2PixelPtr pixel;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
	err = 1;
    if (err)
	sqlite3_result_int (context, -1);
    else
      {
	  blob = sqlite3_value_blob (argv[0]);
	  blob_sz = sqlite3_value_bytes (argv[0]);
	  pixel = rl2_deserialize_dbms_pixel (blob, blob_sz);
	  if (pixel == NULL)
	      sqlite3_result_int (context, -1);
	  else
	    {
		if (rl2_is_pixel_none (pixel) == RL2_TRUE)
		    sqlite3_result_int (context, 1);
		else
		    sqlite3_result_int (context, 0);
		rl2_destroy_pixel (pixel);
	    }
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
/ IsValidRasterStatistics(text db_prefix, text covarage, 
/                         BLOBencoded statistics)
/   or
/ IsValidRasterStatistics(BLOBencoded statistics, text sample_type, 
/                         int num_bands)
/
/ will return 1 (TRUE, valid) or 0 (FALSE, invalid)
/ or -1 (INVALID ARGS)
/
*/
    int ret;
    const unsigned char *blob;
    int blob_sz;
    const char *db_prefix = NULL;
    const char *coverage;
    const char *sample;
    int bands;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    sqlite3 *sqlite;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) == SQLITE_BLOB
	&& sqlite3_value_type (argv[1]) == SQLITE_TEXT
	&& sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
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
      }
    else if ((sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	     && sqlite3_value_type (argv[1]) == SQLITE_TEXT
	     && sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  sqlite = sqlite3_context_db_handle (context);
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  coverage = (const char *) sqlite3_value_text (argv[1]);
	  blob = sqlite3_value_blob (argv[2]);
	  blob_sz = sqlite3_value_bytes (argv[2]);
	  if (!get_coverage_sample_bands
	      (sqlite, db_prefix, coverage, &sample_type, &num_bands))
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
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
/ IsValidRasterTile(text db_prefix, text coverage, integer level, 
/                   BLOBencoded tile_odd, BLOBencoded tile_even)
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
    const char *db_prefix = NULL;
    const char *coverage;
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char num_bands = RL2_BANDS_UNKNOWN;
    unsigned char compression = RL2_COMPRESSION_UNKNOWN;
    sqlite3 *sqlite;
    int err = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_BLOB
	&& sqlite3_value_type (argv[4]) != SQLITE_NULL)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    sqlite = sqlite3_context_db_handle (context);
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    coverage = (const char *) sqlite3_value_text (argv[1]);
    level = sqlite3_value_int (argv[2]);
    blob_odd = sqlite3_value_blob (argv[3]);
    blob_odd_sz = sqlite3_value_bytes (argv[3]);
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
      {
	  blob_even = NULL;
	  blob_even_sz = 0;
      }
    else
      {
	  blob_even = sqlite3_value_blob (argv[4]);
	  blob_even_sz = sqlite3_value_bytes (argv[4]);
      }

    if (!get_coverage_defs
	(sqlite, db_prefix, coverage, &tile_width, &tile_height, &sample_type,
	 &pixel_type, &num_bands, &compression))
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    ret =
	rl2_is_valid_dbms_raster_tile (level, tile_width, tile_height,
				       blob_odd, blob_odd_sz, blob_even,
				       blob_even_sz, sample_type, pixel_type,
				       num_bands, compression);
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

/* attempting to create a Pixel NONE */
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
fnct_CreatePixelNone (sqlite3_context * context, int argc,
		      sqlite3_value ** argv)
{
/* SQL function:
/ CreatePixelNone()
/
/ will return a serialized binary Pixel Object of the NONE type
/ or NULL on failure
*/
    rl2PixelPtr pxl = NULL;
    unsigned char *blob = NULL;
    int blob_sz = 0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

/* attempting to create the Pixel */
    pxl = rl2_create_pixel_none ();
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
    int intval = 0;
    double dblval = 0.0;
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
fnct_GetRasterStatistics_NoDataPixelsCount (sqlite3_context * context,
					    int argc, sqlite3_value ** argv)
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
fnct_GetRasterStatistics_ValidPixelsCount (sqlite3_context * context,
					   int argc, sqlite3_value ** argv)
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
	raster = rl2_raster_from_png (blob, blob_sz, 0);
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
fnct_CreateRasterCoverage (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ CreateRasterCoverage(text coverage, text sample_type, text pixel_type,
/                      int num_bands, text compression, int quality,
/                      int tile_width, int tile_height, int srid,
/                      double res)
/ CreateRasterCoverage(text coverage, text sample_type, text pixel_type,
/                      int num_bands, text compression, int quality,
/                      int tile_width, int tile_height, int srid,
/                      double horz_res, double vert_res)
/ CreateRasterCoverage(text coverage, text sample_type, text pixel_type,
/                      int num_bands, text compression, int quality,
/                      int tile_width, int tile_height, int srid,
/                      double horz_res, double vert_res, BLOB no_data)
/ CreateRasterCoverage(text coverage, text sample_type, text pixel_type,
/                      int num_bands, text compression, int quality,
/                      int tile_width, int tile_height, int srid,
/                      double horz_res, double vert_res, BLOB no_data,
/                      int strict_resolution, int mixed_resolutions,
/                      int section_paths, int section_md5,
/                      int section_summary)
/ CreateRasterCoverage(text coverage, text sample_type, text pixel_type,
/                      int num_bands, text compression, int quality,
/                      int tile_width, int tile_height, int srid,
/                      double horz_res, double vert_res, BLOB no_data,
/                      int strict_resolution, int mixed_resolutions,
/                      int section_paths, int section_md5,
/                      int section_summary, int is_queryable)
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
    int strict_resolution = 0;
    int mixed_resolutions = 0;
    int section_paths = 0;
    int section_md5 = 0;
    int section_summary = 0;
    int is_queryable = 0;
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
    if (argc > 12 && sqlite3_value_type (argv[12]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 13 && sqlite3_value_type (argv[13]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 14 && sqlite3_value_type (argv[14]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 15 && sqlite3_value_type (argv[15]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 16 && sqlite3_value_type (argv[16]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 17 && sqlite3_value_type (argv[17]) != SQLITE_INTEGER)
	err = 1;
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
    if (argc > 12)
      {
	  strict_resolution = sqlite3_value_int (argv[12]);
	  if (strict_resolution)
	      strict_resolution = 1;
      }
    if (argc > 13)
      {
	  mixed_resolutions = sqlite3_value_int (argv[13]);
	  if (mixed_resolutions)
	      mixed_resolutions = 1;
      }
    if (argc > 14)
      {
	  section_paths = sqlite3_value_int (argv[14]);
	  if (section_paths)
	      section_paths = 1;
      }
    if (argc > 15)
      {
	  section_md5 = sqlite3_value_int (argv[15]);
	  if (section_md5)
	      section_md5 = 1;
      }
    if (argc > 16)
      {
	  section_summary = sqlite3_value_int (argv[16]);
	  if (section_summary)
	      section_summary = 1;
      }
    if (argc > 17)
      {
	  is_queryable = sqlite3_value_int (argv[17]);
	  if (is_queryable)
	      is_queryable = 1;
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
    if (strcasecmp (compression, "DEFLATE_NO") == 0)
	compr = RL2_COMPRESSION_DEFLATE_NO;
    if (strcasecmp (compression, "LZMA") == 0)
	compr = RL2_COMPRESSION_LZMA;
    if (strcasecmp (compression, "LZMA_NO") == 0)
	compr = RL2_COMPRESSION_LZMA_NO;
    if (strcasecmp (compression, "LZ4") == 0)
	compr = RL2_COMPRESSION_LZ4;
    if (strcasecmp (compression, "LZ4_NO") == 0)
	compr = RL2_COMPRESSION_LZ4_NO;
    if (strcasecmp (compression, "ZSTD") == 0)
	compr = RL2_COMPRESSION_ZSTD;
    if (strcasecmp (compression, "ZSTD_NO") == 0)
	compr = RL2_COMPRESSION_ZSTD_NO;
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
    if (strcasecmp (compression, "CHARLS") == 0)
	compr = RL2_COMPRESSION_CHARLS;
    if (strcasecmp (compression, "JP2") == 0)
	compr = RL2_COMPRESSION_LOSSY_JP2;
    if (strcasecmp (compression, "LL_JP2") == 0)
	compr = RL2_COMPRESSION_LOSSLESS_JP2;

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
				    horz_res, vert_res, no_data, palette,
				    strict_resolution, mixed_resolutions,
				    section_paths, section_md5,
				    section_summary, is_queryable);
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
fnct_SetRasterCoverageInfos (sqlite3_context * context, int argc,
			     sqlite3_value ** argv)
{
/* SQL function:
/ SetRasterCoverageInfos(String coverage_name, String title, 
/		   String abstract)
/    of
/ SetRasterCoverageInfos(String coverage_name, String title, 
/		   String abstract, Bool is_queryable)
/
/ inserts or updates the descriptive infos supporting a Raster Coverage
/ returns 1 on success
/ 0 on failure, -1 on invalid arguments
*/
    int ret;
    const char *coverage_name;
    const char *title = NULL;
    const char *abstract = NULL;
    int is_queryable = -1;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    coverage_name = (const char *) sqlite3_value_text (argv[0]);
    title = (const char *) sqlite3_value_text (argv[1]);
    abstract = (const char *) sqlite3_value_text (argv[2]);
    if (argc >= 4)
      {
	  if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
	  is_queryable = sqlite3_value_int (argv[3]);
      }
    ret =
	set_coverage_infos (sqlite, coverage_name, title, abstract,
			    is_queryable);
    sqlite3_result_int (context, ret);
}

static void
fnct_SetRasterCoverageCopyright (sqlite3_context * context, int argc,
				 sqlite3_value ** argv)
{
/* SQL function:
/ SetRasterCoverageCopyright(Text coverage_name, Text copyright)
/    or
/ SetRasterCoverageCopyright(Text coverage_name, Text copyright,
/                            Text license)
/
/ updates copyright infos supporting a Raster Coverage
/ returns 1 on success
/ 0 on failure, -1 on invalid arguments
*/
    int ret;
    const char *coverage_name;
    const char *copyright = NULL;
    const char *license = NULL;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	copyright = (const char *) sqlite3_value_text (argv[1]);
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    coverage_name = (const char *) sqlite3_value_text (argv[0]);
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	      license = (const char *) sqlite3_value_text (argv[2]);
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    ret = set_coverage_copyright (sqlite, coverage_name, copyright, license);
    sqlite3_result_int (context, ret);
}

static void
fnct_SetRasterCoverageDefaultBands (sqlite3_context * context, int argc,
				    sqlite3_value ** argv)
{
/* SQL function:
/ SetRasterCoverageDefaultBands(String coverage_name, int red_band,
/                               green_band, blue_band, nir_band)
/
/ inserts or updates the default band mapping supporting a Raster
/ Coverage of the MULTIBAND type
/ returns 1 on success
/ 0 on failure, -1 on invalid arguments
*/
    const char *coverage_name;
    int red;
    int green;
    int blue;
    int nir;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    coverage_name = (const char *) sqlite3_value_text (argv[0]);
    red = sqlite3_value_int (argv[1]);
    green = sqlite3_value_int (argv[2]);
    blue = sqlite3_value_int (argv[3]);
    nir = sqlite3_value_int (argv[4]);
    if (red < 0 || red > 255)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (green < 0 || green > 255)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (blue < 0 || blue > 255)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (nir < 0 || nir > 255)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (red == green || red == blue || red == nir)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (green == blue || green == nir)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (blue == nir)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (rl2_set_dbms_coverage_default_bands
	(sqlite, coverage_name, red, green, blue, nir) == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static void
fnct_EnableRasterCoverageAutoNDVI (sqlite3_context * context, int argc,
				   sqlite3_value ** argv)
{
/* SQL function:
/ EnableRasterCoverageAutoNDVI(String coverage_name, int on_off)
/
/ enables or disables the AutoNDVI feature on a Raster
/ Coverage of the MULTIBAND type and explicitly declaring
/ a default band mapping
/ returns 1 on success
/ 0 on failure, -1 on invalid arguments
*/
    const char *coverage_name;
    int on_off;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    coverage_name = (const char *) sqlite3_value_text (argv[0]);
    on_off = sqlite3_value_int (argv[1]);
    if (rl2_enable_dbms_coverage_auto_ndvi (sqlite, coverage_name, on_off) ==
	RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static void
fnct_IsRasterCoverageAutoNdviEnabled (sqlite3_context * context, int argc,
				      sqlite3_value ** argv)
{
/* SQL function:
/ IsRasterCoverageAutoNdviEnabled(String db_prefix, String coverage_name)
/
/ checks if a Raster Coverage do actually supports the AutoNDVI feature
/ returns 1 (TRUE) or 0 (FALSE)
/ -1 on invalid arguments or if the Raster Coverage isn't of the
/ MULTIBAND type and explicitly declaring a default band mapping
*/
    const char *db_prefix = NULL;
    const char *coverage_name;
    int ret;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    coverage_name = (const char *) sqlite3_value_text (argv[1]);
    ret =
	rl2_is_dbms_coverage_auto_ndvi_enabled (sqlite, db_prefix,
						coverage_name);
    if (ret == RL2_TRUE)
	sqlite3_result_int (context, 1);
    else if (ret == RL2_FALSE)
	sqlite3_result_int (context, 0);
    else
	sqlite3_result_int (context, -1);
}

static void
fnct_CopyRasterCoverage (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ CopyRasterCoverage(String db_prefix, String coverage_name)
/   or
/ CopyRasterCoverage(String db_prefix, String coverage_name, int transaction)
/
/ copies a whole Raster Coverage from an Attached DB
/ returns 1 on success
/ 0 on failure, -1 on invalid arguments
*/
    int ret;
    const char *db_prefix;
    const char *coverage_name;
    int transaction = 0;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    db_prefix = (const char *) sqlite3_value_text (argv[0]);
    coverage_name = (const char *) sqlite3_value_text (argv[1]);
    if (argc == 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	      transaction = sqlite3_value_int (argv[2]);
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }

    if (transaction)
      {
	  /* starting a DBMS Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
      }

/* just in case, attempting to (re)create raster meta-tables */
    sqlite3_exec (sqlite, "SELECT CreateRasterCoveragesTable()", NULL, NULL,
		  NULL);
    sqlite3_exec (sqlite, "SELECT CreateStylingTables()", NULL, NULL, NULL);

/* checks if a Raster Coverage of the same name already exists on Main */
    if (rl2_check_raster_coverage_destination (sqlite, coverage_name) != RL2_OK)
      {
	  if (transaction)
	      sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	  sqlite3_result_int (context, 0);
	  return;
      }
/* checks if the Raster Coverage origin do really exists */
    if (rl2_check_raster_coverage_origin (sqlite, db_prefix, coverage_name) !=
	RL2_OK)
      {
	  if (transaction)
	      sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	  sqlite3_result_int (context, 0);
	  return;
      }
/* attemtping to copy */
    if (rl2_copy_raster_coverage (sqlite, db_prefix, coverage_name) != RL2_OK)
      {
	  if (transaction)
	      sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	  sqlite3_result_int (context, 0);
	  return;
      }

    if (transaction)
      {
	  /* committing the still pending Transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		sqlite3_result_int (context, 0);
		return;
	    }
      }
    sqlite3_result_int (context, 1);
}

static void
fnct_DeleteSection (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ DeleteSection(text coverage, integer section_id)
/ DeleteSection(text coverage, integer section_id, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *coverage;
    sqlite3_int64 section_id;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    rl2CoveragePtr cvg = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
	goto invalid;

/* retrieving the arguments */
    sqlite = sqlite3_context_db_handle (context);
    coverage = (const char *) sqlite3_value_text (argv[0]);
    section_id = sqlite3_value_int64 (argv[1]);
    if (argc > 2)
	transaction = sqlite3_value_int (argv[2]);

    cvg = rl2_create_coverage_from_dbms (sqlite, NULL, coverage);
    if (cvg == NULL)
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
fnct_DropRasterCoverage (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ DropRasterCoverage(text coverage)
/ DropRasterCoverage(text coverage, int transaction)
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

    cvg = rl2_create_coverage_from_dbms (sqlite, NULL, coverage);
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
fnct_LoadFontFromFile (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ LoadFontFromFile(text font-path)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    FILE *in = NULL;
    unsigned char *buffer = NULL;
    int buf_size;
    const char *font_path;
    sqlite3 *sqlite;
    unsigned char *blob = NULL;
    int blob_sz;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    sqlite = sqlite3_context_db_handle (context);
    font_path = (const char *) sqlite3_value_text (argv[0]);

/* loading the font from the external file */
    in = fopen (font_path, "rb");
    if (in == NULL)
      {
	  sqlite3_result_int (context, 0);
	  return;
      }
    buffer = malloc (MAX_FONT_SIZE);
    if (buffer == NULL)
      {
	  sqlite3_result_int (context, 0);
	  return;
      }
    buf_size = fread (buffer, 1, MAX_FONT_SIZE, in);
    fclose (in);

/* attempting to encode the Font */
    if (rl2_font_encode (buffer, buf_size, &blob, &blob_sz) != RL2_OK)
      {
	  free (buffer);
	  sqlite3_result_int (context, 0);
	  return;
      }
    free (buffer);

/* attempting to insert/update the Font */
    if (rl2_load_font_into_dbms (sqlite, blob, blob_sz) != RL2_OK)
	sqlite3_result_int (context, 0);
    else
	sqlite3_result_int (context, 1);
    return;
}

static void
fnct_ExportFontToFile (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ ExportFontToFile(text db_prefix, text facename, text font-path)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    FILE *out = NULL;
    unsigned char *buffer = NULL;
    int buf_size;
    const char *db_prefix = NULL;
    const char *facename;
    const char *font_path;
    sqlite3 *sqlite;
    int wr;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    else if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    facename = (const char *) sqlite3_value_text (argv[1]);
    if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    font_path = (const char *) sqlite3_value_text (argv[2]);

    sqlite = sqlite3_context_db_handle (context);

/* attempting to get the Font */
    if (rl2_get_font_from_dbms (sqlite, db_prefix, facename, &buffer, &buf_size)
	!= RL2_OK)
      {
	  sqlite3_result_int (context, 0);
	  return;
      }

/* creating/opening the external output file */
    out = fopen (font_path, "wb");
    if (out == NULL)
      {
	  free (buffer);
	  sqlite3_result_int (context, 0);
	  return;
      }
/* exporting the Font */
    wr = fwrite (buffer, 1, buf_size, out);
    if (wr != buf_size)
      {
	  free (buffer);
	  fclose (out);
	  sqlite3_result_int (context, 0);
	  return;
      }
    free (buffer);
    fclose (out);
    sqlite3_result_int (context, 1);
}

static void
fnct_IsValidFont (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsValidFont(BLOB serialized-font)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    const unsigned char *blob = NULL;
    int blob_sz;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    blob = (const unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    ret = rl2_is_valid_encoded_font (blob, blob_sz);
    if (ret == RL2_OK)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}

static void
fnct_CheckFontFacename (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ CheckFontFacename(TEXT facename, BLOB serialized-font)
/
/ checks for a matching facename
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    const unsigned char *blob = NULL;
    int blob_sz;
    const char *facename1;
    char *facename2;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    facename1 = (const char *) sqlite3_value_text (argv[0]);
    if (sqlite3_value_type (argv[1]) != SQLITE_BLOB)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    blob = (const unsigned char *) sqlite3_value_blob (argv[1]);
    blob_sz = sqlite3_value_bytes (argv[1]);
    facename2 = rl2_get_encoded_font_facename (blob, blob_sz);
    if (facename2 == NULL)
	sqlite3_result_int (context, -1);
    else
      {
	  if (strcmp (facename1, facename2) == 0)
	      sqlite3_result_int (context, 1);
	  else
	      sqlite3_result_int (context, 0);
	  free (facename2);
      }
}

static void
fnct_GetFontFamily (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetFontFamily(BLOB serialized-font)
/
/ will return the Font Family name (e.g. Roboto)
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob = NULL;
    int blob_sz;
    char *family_name;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }

    blob = (const unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    family_name = rl2_get_encoded_font_family (blob, blob_sz);
    if (family_name == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_text (context, family_name, strlen (family_name), free);
}

static void
fnct_GetFontFacename (sqlite3_context * context, int argc,
		      sqlite3_value ** argv)
{
/* SQL function:
/ GetFontFacename(BLOB serialized-font)
/
/ will return the Font facename (e.g. Roboto-BoldItalic)
/ or NULL (INVALID ARGS)
/
*/
    const unsigned char *blob = NULL;
    int blob_sz;
    char *facename;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_null (context);
	  return;
      }

    blob = (const unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    facename = rl2_get_encoded_font_facename (blob, blob_sz);
    if (facename == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_text (context, facename, strlen (facename), free);
}

static void
fnct_IsFontBold (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsFontBold(BLOB serialized-font)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    const unsigned char *blob = NULL;
    int blob_sz;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    blob = (const unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    ret = rl2_is_encoded_font_bold (blob, blob_sz);
    sqlite3_result_int (context, ret);
}

static void
fnct_IsFontItalic (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ IsFontItalic(BLOB serialized-font)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    const unsigned char *blob = NULL;
    int blob_sz;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_BLOB)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    blob = (const unsigned char *) sqlite3_value_blob (argv[0]);
    blob_sz = sqlite3_value_bytes (argv[0]);
    ret = rl2_is_encoded_font_italic (blob, blob_sz);
    sqlite3_result_int (context, ret);
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
    const void *data;
    int max_threads = 1;
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
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    coverage = rl2_create_coverage_from_dbms (sqlite, NULL, cvg_name);
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
    ret = rl2_load_raster_into_dbms (sqlite, max_threads, path, coverage,
				     worldfile, force_srid, pyramidize, 0);
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
    const char *file_ext = NULL;
    int worldfile = 0;
    int force_srid = -1;
    int transaction = 1;
    int pyramidize = 1;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    int ret;
    const void *data;
    int max_threads;
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
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    coverage = rl2_create_coverage_from_dbms (sqlite, NULL, cvg_name);
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
    ret =
	rl2_load_mrasters_into_dbms (sqlite, max_threads, path, file_ext,
				     coverage, worldfile, force_srid,
				     pyramidize, 0);
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
/ Pyramidize(text coverage, integer section_id)
/ Pyramidize(text coverage, integer section_id, int force_rebuild)
/ Pyramidize(text coverage, integer section_id, int force_rebuild,
/            int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    sqlite3_int64 section_id = 0;
    int null_id = 1;
    int forced_rebuild = 0;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    const void *data;
    int max_threads = 1;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_INTEGER
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
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    if (argc > 1)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	    {
		section_id = sqlite3_value_int64 (argv[1]);
		null_id = 0;
	    }
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
    if (null_id)
	ret =
	    rl2_build_all_section_pyramids (sqlite, max_threads, cvg_name,
					    forced_rebuild, 1);
    else
	ret =
	    rl2_build_section_pyramid (sqlite, max_threads, cvg_name,
				       section_id, forced_rebuild, 1);
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
fnct_PyramidizeMonolithic (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ PyramidizeMonolithic(text coverage)
/ PyramidizeMonolithic(text coverage, int virt_levels)
/ PyramidizeMonolithic(text coverage, int virt_levels, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    int virt_levels = 0;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 2 && sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
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
	virt_levels = sqlite3_value_int (argv[1]);
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
    ret = rl2_build_monolithic_pyramid (sqlite, cvg_name, virt_levels, 1);
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
/ DePyramidize(text coverage, integer section)
/ DePyramidize(text coverage, integer section, int transaction)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    sqlite3_int64 section_id = 0;
    int null_id = 1;
    int transaction = 1;
    sqlite3 *sqlite;
    int ret;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) != SQLITE_TEXT)
	err = 1;
    if (argc > 1 && sqlite3_value_type (argv[1]) != SQLITE_INTEGER
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
	  if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	    {
		section_id = sqlite3_value_int64 (argv[1]);
		null_id = 0;
	    }
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
    if (null_id)
	ret = rl2_delete_all_pyramids (sqlite, cvg_name);
    else
	ret = rl2_delete_section_pyramid (sqlite, cvg_name, section_id);
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
    rl2CoveragePtr coverage = NULL;
    rl2PrivCoveragePtr cvg;
    rl2RasterStatisticsPtr section_stats = NULL;
    sqlite3 *sqlite;
    int ret;
    int n;
    double x = 0.0;
    double y = 0.0;
    double tilew;
    double tileh;
    unsigned int tile_width;
    unsigned int tile_height;
    WmsRetryListPtr retry_list = NULL;
    char *table;
    char *xtable;
    char *sql;
    sqlite3_stmt *stmt_data = NULL;
    sqlite3_stmt *stmt_tils = NULL;
    sqlite3_stmt *stmt_sect = NULL;
    sqlite3_stmt *stmt_policies = NULL;
    sqlite3_stmt *stmt_levl = NULL;
    sqlite3_stmt *stmt_upd_sect = NULL;
    int first = 1;
    double ext_x;
    double ext_y;
    unsigned int width;
    unsigned int height;
    sqlite3_int64 section_id = 0;
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

/* retrieving the BBOX */
    sqlite = sqlite3_context_db_handle (context);
    if (rl2_parse_bbox (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) !=
	RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, NULL, cvg_name);
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
    if (rl2_get_coverage_type
	(coverage, &sample_type, &pixel_type, &num_bands) != RL2_OK)
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
    if (width < tile_width)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (height < tile_height)
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
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO main.\"%s\" (section_id, section_name, file_path, "
	 "md5_checksum, summary, width, height, geometry) "
	 "VALUES (NULL, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO sections SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

    sql =
	sqlite3_mprintf
	("SELECT strict_resolution, mixed_resolutions, section_paths, "
	 "section_md5, section_summary FROM main.raster_coverages "
	 "WHERE coverage_name = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_policies, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	stmt_policies = NULL;

    table = sqlite3_mprintf ("%s_sections", cvg_name);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE main.\"%s\" SET statistics = ? WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd_sect, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE sections SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_levels", cvg_name);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT OR IGNORE INTO main.\"%s\" (pyramid_level, "
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
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO main.\"%s\" (tile_id, pyramid_level, section_id, geometry) "
	 "VALUES (NULL, 0, ?, BuildMBR(?, ?, ?, ?, ?))", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_tils, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO tiles SQL error: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }

    table = sqlite3_mprintf ("%s_tile_data", cvg_name);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO main.\"%s\" (tile_id, tile_data_odd, tile_data_even) "
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
    cvg = (rl2PrivCoveragePtr) coverage;
    for (y = maxy; y > miny; y -= tileh)
      {
	  for (x = minx; x < maxx; x += tilew)
	    {
		unsigned char *rgba_tile =
		    do_wms_GetMap_get (NULL, url, proxy, wms_version,
				       wms_layer,
				       wms_crs, swap_xy, x, y - tileh,
				       x + tilew, y, tile_width, tile_height,
				       wms_style, wms_format, opaque, 0);
		if (rgba_tile == NULL)
		  {
		      add_retry (retry_list, x, y - tileh, x + tilew, y);
		      continue;
		  }

		params.sqlite = sqlite;
		params.rgba_tile = rgba_tile;
		params.coverage = coverage;
		params.mixedResolutions = cvg->mixedResolutions;
		params.sectionPaths = cvg->sectionPaths;
		params.sectionMD5 = cvg->sectionMD5;
		params.sectionSummary = cvg->sectionSummary;
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
		params.xml_summary = NULL;
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
		unsigned char *rgba_tile = NULL;
		if (retry->done)
		  {
		      retry = retry->next;
		      continue;
		  }
		retry->count += 1;
		rgba_tile =
		    do_wms_GetMap_get (NULL, url, proxy, wms_version,
				       wms_layer, wms_crs, swap_xy,
				       retry->minx, retry->miny, retry->maxx,
				       retry->maxy, tile_width, tile_height,
				       wms_style, wms_format, opaque, 0);
		if (rgba_tile == NULL)
		  {
		      retry = retry->next;
		      continue;
		  }

		params.sqlite = sqlite;
		params.rgba_tile = rgba_tile;
		params.coverage = coverage;
		params.mixedResolutions = cvg->mixedResolutions;
		params.sectionPaths = cvg->sectionPaths;
		params.sectionMD5 = cvg->sectionMD5;
		params.sectionSummary = cvg->sectionSummary;
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
		params.xml_summary = NULL;
		if (!insert_wms_tile
		    (&params, &first, &section_stats, &section_id))
		    goto error;
		retry->done = 1;
		retry = retry->next;
	    }
      }

    if (!rl2_do_insert_stats (sqlite, section_stats, section_id, stmt_upd_sect))
	goto error;
    free_retry_list (retry_list);
    retry_list = NULL;
    sqlite3_free (wms_crs);
    wms_crs = NULL;

    sqlite3_finalize (stmt_upd_sect);
    sqlite3_finalize (stmt_sect);
    if (stmt_policies != NULL)
	sqlite3_finalize (stmt_policies);
    sqlite3_finalize (stmt_levl);
    sqlite3_finalize (stmt_tils);
    sqlite3_finalize (stmt_data);
    stmt_upd_sect = NULL;
    stmt_sect = NULL;
    stmt_levl = NULL;
    stmt_policies = NULL;
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
    if (stmt_policies != NULL)
	sqlite3_finalize (stmt_policies);
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
common_write_geotiff (int by_section, sqlite3_context * context, int argc,
		      sqlite3_value ** argv)
{
/* common implementation for Write GeoTIFF */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    const char *path;
    sqlite3_int64 section_id = 0;
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
    const void *data;
    int max_threads = 1;
    int ret;
    int errcode = -1;
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[7]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	      err = 1;
	  if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_TEXT)
	      err = 1;
	  if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER)
	      err = 1;
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
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
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  blob = sqlite3_value_blob (argv[6]);
	  blob_sz = sqlite3_value_bytes (argv[6]);
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[7]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[7]);
	  if (argc > 8)
	    {
		if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[8]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[8]);
	    }
	  else
	      vert_res = horz_res;
	  if (argc > 9)
	      worldfile = sqlite3_value_int (argv[9]);
	  if (argc > 10)
	    {
		const char *compr =
		    (const char *) sqlite3_value_text (argv[10]);
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
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
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
      }

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
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

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (by_section)
      {
	  /* single Section */
	  ret =
	      rl2_export_section_geotiff_from_dbms (sqlite, max_threads, path,
						    coverage, section_id,
						    horz_res, vert_res, minx,
						    miny, maxx, maxy, width,
						    height, compression,
						    tile_sz, worldfile);
      }
    else
      {
	  /* whole Coverage */
	  ret =
	      rl2_export_geotiff_from_dbms (sqlite, max_threads, path,
					    coverage, horz_res, vert_res,
					    minx, miny, maxx, maxy, width,
					    height, compression, tile_sz,
					    worldfile);
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
fnct_WriteGeoTiff (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/              int width, int height, BLOB geom, double resolution)
/ WriteGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/              int width, int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/              int width, int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile)
/ WriteGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/              int width, int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile,
/              text compression)
/ WriteGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/              int width, int height, BLOB geom, double horz_res,
/              double vert_res, int with_worldfile,
/              text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_geotiff (0, context, argc, argv);
}

static void
fnct_WriteSectionGeoTiff (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionGeoTiff(text db_prefix, text coverage, int section_id, 
/                     text geotiff_path, int width, int height, 
/                     BLOB geom, double resolution)
/ WriteSectionGeoTiff(text db_prefix, text coverage, int section_id, 
/                     text geotiff_path, int width, int height, 
/                     BLOB geom, double horz_res, double vert_res)
/ WriteSectionGeoTiff(text db_prefix, text coverage, int section_id, 
/                     text geotiff_path, int width, int height, 
/                     BLOB geom, double horz_res, double vert_res, 
/                     int with_worldfile)
/ WriteSectionGeoTiff(text db_prefix, text coverage, int section_id, 
/                     text geotiff_path, int width, int height, 
/                     BLOB geom, double horz_res, double vert_res, 
/                     int with_worldfile, text compression)
/ WriteSectionGeoTiff(text db_prefix, text coverage, int section_id, 
/                     text geotiff_path, int width, int height, 
/                     BLOB geom, double horz_res, double vert_res, 
/                     int with_worldfile, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_geotiff (1, context, argc, argv);
}

static void
common_write_triple_band_geotiff (int by_section, sqlite3_context * context,
				  int argc, sqlite3_value ** argv)
{
/* common implementation WriteTripleBandGeoTiff */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    sqlite3_int64 section_id = 0;
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
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[9]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[10]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[10]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[11]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 12 && sqlite3_value_type (argv[12]) != SQLITE_INTEGER)
	      err = 1;
	  if (argc > 13 && sqlite3_value_type (argv[13]) != SQLITE_TEXT)
	      err = 1;
	  if (argc > 14 && sqlite3_value_type (argv[14]) != SQLITE_INTEGER)
	      err = 1;
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[8]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[10]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER)
	      err = 1;
	  if (argc > 12 && sqlite3_value_type (argv[12]) != SQLITE_TEXT)
	      err = 1;
	  if (argc > 13 && sqlite3_value_type (argv[13]) != SQLITE_INTEGER)
	      err = 1;
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  red_band = sqlite3_value_int (argv[6]);
	  green_band = sqlite3_value_int (argv[7]);
	  blue_band = sqlite3_value_int (argv[8]);
	  blob = sqlite3_value_blob (argv[9]);
	  blob_sz = sqlite3_value_bytes (argv[9]);
	  if (sqlite3_value_type (argv[10]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[10]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[10]);
	  if (argc > 11)
	    {
		if (sqlite3_value_type (argv[11]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[11]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[11]);
	    }
	  else
	      vert_res = horz_res;
	  if (argc > 12)
	      worldfile = sqlite3_value_int (argv[12]);
	  if (argc > 13)
	    {
		const char *compr =
		    (const char *) sqlite3_value_text (argv[13]);
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
	  if (argc > 14)
	      tile_sz = sqlite3_value_int (argv[14]);
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
	  red_band = sqlite3_value_int (argv[5]);
	  green_band = sqlite3_value_int (argv[6]);
	  blue_band = sqlite3_value_int (argv[7]);
	  blob = sqlite3_value_blob (argv[8]);
	  blob_sz = sqlite3_value_bytes (argv[8]);
	  if (sqlite3_value_type (argv[9]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[9]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[9]);
	  if (argc > 10)
	    {
		if (sqlite3_value_type (argv[10]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[10]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[10]);
	    }
	  else
	      vert_res = horz_res;
	  if (argc > 11)
	      worldfile = sqlite3_value_int (argv[11]);
	  if (argc > 12)
	    {
		const char *compr =
		    (const char *) sqlite3_value_text (argv[12]);
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
	  if (argc > 13)
	      tile_sz = sqlite3_value_int (argv[13]);
      }

    sqlite = sqlite3_context_db_handle (context);
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
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
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (by_section)
      {
	  /* single Section */
	  ret =
	      rl2_export_section_triple_band_geotiff_from_dbms (sqlite, path,
								coverage,
								section_id,
								horz_res,
								vert_res,
								minx, miny,
								maxx, maxy,
								width, height,
								red_band,
								green_band,
								blue_band,
								compression,
								tile_sz,
								worldfile);
      }
    else
      {
	  /* whole Coverage */

	  ret =
	      rl2_export_triple_band_geotiff_from_dbms (sqlite, path,
							coverage, horz_res,
							vert_res, minx, miny,
							maxx, maxy, width,
							height, red_band,
							green_band, blue_band,
							compression, tile_sz,
							worldfile);
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
fnct_WriteTripleBandGeoTiff (sqlite3_context * context, int argc,
			     sqlite3_value ** argv)
{
/* SQL function:
/ WriteTripleBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                        int width, int height, int red_band, int green_band,
/                        int blue_band, BLOB geom, double resolution)
/ WriteTripleBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                        int width, int height, int red_band, int green_band,
/                        int blue_band, BLOB geom, double horz_res,
/                        double vert_res)
/ WriteTripleBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                        int width, int height, int red_band, int green_band,
/                        int blue_band, BLOB geom, double horz_res,
/                        double vert_res, int with_worldfile)
/ WriteTripleBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                        int width, int height, int red_band, int green_band, 
/                        int blue_band, BLOB geom, double horz_res,
/                        double vert_res, int with_worldfile, 
/                        text compression)
/ WriteTripleBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                        int width, int height, int red_band, int green_band,
/                        int blue_band, BLOB geom, double horz_res,
/                        double vert_res, int with_worldfile,
/                        text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_triple_band_geotiff (0, context, argc, argv);
}

static void
fnct_WriteSectionTripleBandGeoTiff (sqlite3_context * context, int argc,
				    sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionTripleBandGeoTiff(text db_prefix, text coverage, int section_id, 
/                               text geotiff_path, int width, int height,
/                               int red_band, int green_band, int blue_band,
/                               BLOB geom, double resolution)
/ WriteSectionTripleBandGeoTiff(text db_prefix, text coverage, int section_id, 
/                               text geotiff_path, int width, int height,
/                               int red_band, int green_band, int blue_band,
/                               BLOB geom, double horz_res, double vert_res)
/ WriteSectionTripleBandGeoTiff(text db_prefix, text coverage, int section_id, 
/                               text geotiff_path, int width, int height,
/                               int red_band, int green_band, int blue_band,
/                               BLOB geom, double horz_res, double vert_res, 
/                               int with_worldfile)
/ WriteSectionTripleBandGeoTiff(text db_prefix, text coverage, int section_id, 
/                               text geotiff_path, int width, int height,
/                               int red_band, int green_band, int blue_band, 
/                               BLOB geom, double horz_res, double vert_res, 
/                               int with_worldfile, text compression)
/ WriteSectionTripleBandGeoTiff(text db_prefix, text coverage, int section_id,
/                               text geotiff_path, int width, int height,
/                               int red_band, int green_band, int blue_band, 
/                               BLOB geom, double horz_res, double vert_res, 
/                               int with_worldfile, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_triple_band_geotiff (1, context, argc, argv);
}

static void
common_write_mono_band_geotiff (int by_section, sqlite3_context * context,
				int argc, sqlite3_value ** argv)
{
/* common implementation Write MonoBand GeoTiff
*/
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    sqlite3_int64 section_id = 0;
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
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
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
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[7]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	      err = 1;
	  if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_TEXT)
	      err = 1;
	  if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER)
	      err = 1;
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  mono_band = sqlite3_value_int (argv[6]);
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
		const char *compr =
		    (const char *) sqlite3_value_text (argv[11]);
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
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
	  mono_band = sqlite3_value_int (argv[5]);
	  blob = sqlite3_value_blob (argv[6]);
	  blob_sz = sqlite3_value_bytes (argv[6]);
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[7]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[7]);
	  if (argc > 8)
	    {
		if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[8]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[8]);
	    }
	  else
	      vert_res = horz_res;
	  if (argc > 9)
	      worldfile = sqlite3_value_int (argv[9]);
	  if (argc > 10)
	    {
		const char *compr =
		    (const char *) sqlite3_value_text (argv[10]);
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
      }

    sqlite = sqlite3_context_db_handle (context);
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
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
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (by_section)
      {
	  /* single Section */
	  ret =
	      rl2_export_section_mono_band_geotiff_from_dbms (sqlite, path,
							      coverage,
							      section_id,
							      horz_res,
							      vert_res, minx,
							      miny, maxx,
							      maxy, width,
							      height,
							      mono_band,
							      compression,
							      tile_sz,
							      worldfile);
      }
    else
      {
	  /* whole Coverage */
	  ret =
	      rl2_export_mono_band_geotiff_from_dbms (sqlite, path, coverage,
						      horz_res, vert_res,
						      minx, miny, maxx, maxy,
						      width, height,
						      mono_band, compression,
						      tile_sz, worldfile);
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
fnct_WriteMonoBandGeoTiff (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ WriteMonoBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double resolution)
/ WriteMonoBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double horz_res, double vert_res)
/ WriteMonoBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double horz_res, double vert_res, int with_worldfile)
/ WriteMonoBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double horz_res, double vert_res, int with_worldfile, 
/                      text compression)
/ WriteMonoBandGeoTiff(text db_prefix, text coverage, text geotiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double horz_res, double vert_res, int with_worldfile, 
/                      text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_mono_band_geotiff (0, context, argc, argv);
}

static void
fnct_WriteSectionMonoBandGeoTiff (sqlite3_context * context, int argc,
				  sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionMonoBandGeoTiff(text db_prefix, text coverage, int section_id,
/                             text geotiff_path, int width, int height, 
/                             int mono_band, BLOB geom, double resolution)
/ WriteSectionMonoBandGeoTiff(text db_prefix, text coverage, int section_id,
/                             text geotiff_path, int width, int height, 
/                             int mono_band, BLOB geom, double horz_res, 
/                             double vert_res)
/ WriteSectionMonoBandGeoTiff(text db_prefix, text coverage, int section_id,
/                             text geotiff_path, int width, int height, 
/                             int mono_band, BLOB geom, double horz_res, 
/                             double vert_res, int with_worldfile)
/ WriteSectionMonoBandGeoTiff(text db_prefix, text coverage, int section_id,
/                             text geotiff_path, int width, int height, 
/                             int mono_band, BLOB geom, double horz_res, 
/                             double vert_res, int with_worldfile, 
/                             text compression)
/ WriteSectionMonoBandGeoTiff(text db_prefix, text coverage, int section_id,
/                             text geotiff_path, int width, int height, 
/                             int mono_band, BLOB geom, double horz_res, 
/                             double vert_res, int with_worldfile, 
/                             text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_mono_band_geotiff (1, context, argc, argv);
}

static void
common_write_tiff (int by_section, int with_worldfile,
		   sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* common implementation Write TIFF */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    const char *path;
    sqlite3_int64 section_id = 0;
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
    const void *data;
    int max_threads = 1;
    int ret;
    int errcode = -1;
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* filtering by Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[7]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_TEXT)
	      err = 1;
	  if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	      err = 1;
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
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
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* filtering by Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  blob = sqlite3_value_blob (argv[6]);
	  blob_sz = sqlite3_value_bytes (argv[6]);
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[7]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[7]);
	  if (argc > 8)
	    {
		if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[8]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[8]);
	    }
	  else
	      vert_res = horz_res;
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
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
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
      }

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
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

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (by_section)
      {
	  /* only a single Section */
	  if (with_worldfile)
	    {
		/* TIFF + Worldfile */
		ret =
		    rl2_export_section_tiff_worldfile_from_dbms (sqlite,
								 max_threads,
								 path,
								 coverage,
								 section_id,
								 horz_res,
								 vert_res,
								 minx, miny,
								 maxx, maxy,
								 width,
								 height,
								 compression,
								 tile_sz);
	    }
	  else
	    {
		/* plain TIFF, no Worldfile */
		ret =
		    rl2_export_section_tiff_from_dbms (sqlite, max_threads,
						       path, coverage,
						       section_id, horz_res,
						       vert_res, minx, miny,
						       maxx, maxy, width,
						       height, compression,
						       tile_sz);
	    }
      }
    else
      {
	  /* whole Coverage */
	  if (with_worldfile)
	    {
		/* TIFF + Worldfile */
		ret =
		    rl2_export_tiff_worldfile_from_dbms (sqlite, max_threads,
							 path, coverage,
							 horz_res, vert_res,
							 minx, miny, maxx,
							 maxy, width, height,
							 compression, tile_sz);
	    }
	  else
	    {
		/* plain TIFF, no Worldfile */
		ret =
		    rl2_export_tiff_from_dbms (sqlite, max_threads, path,
					       coverage, horz_res, vert_res,
					       minx, miny, maxx, maxy, width,
					       height, compression, tile_sz);
	    }
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
/ WriteTiffTfw(text db_prefix, text coverage, text tiff_path, 
/              int width, int height, BLOB geom, double resolution)
/ WriteTiffTfw(text db_prefix, text coverage, text tiff_path, 
/              int width, int height, BLOB geom, double horz_res,
/              double vert_res)
/ WriteTiffTfw(text db_prefix, text coverage, text tiff_path, 
/              int width, int height, BLOB geom, double horz_res,
/              double vert_res, text compression)
/ WriteTiffTfw(text db_prefix, text coverage, text tiff_path, 
/              int width, int height, BLOB geom, double horz_res,
/              double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_tiff (0, 1, context, argc, argv);
}

static void
fnct_WriteTiff (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteTiff(text db_prefix, text coverage, text tiff_path, int width,
/           int height, BLOB geom, double resolution)
/ WriteTiff(text db_prefix, text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res, double vert_res)
/ WriteTiff(text db_prefix, text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res, double vert_res, 
/           text compression)
/ WriteTiff(text db_prefix, text coverage, text tiff_path, int width,
/           int height, BLOB geom, double horz_res, double vert_res, 
/           text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_tiff (0, 0, context, argc, argv);
}

static void
fnct_WriteSectionTiffTfw (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionTiffTfw(text db_prefix, text coverage, int section_id, 
/                     text tiff_path, int width, int height, BLOB geom, 
/                     double resolution)
/ WriteSectionTiffTfw(text db_prefix, text coverage, int section_id, 
/                     text tiff_path, int width, int height, BLOB geom, 
/                     double horz_res, double vert_res)
/ WriteSectionTiffTfw(text db_prefix, text coverage, int section_id, 
/                     text tiff_path, int width, int height, BLOB geom, 
/                     double horz_res, double vert_res, text compression)
/ WriteSectionTiffTfw(text db_prefix, text coverage, int section_id, 
/                     text tiff_path, int width, int height, BLOB geom, 
/                     double horz_res, double vert_res, text compression, 
/                     int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_tiff (1, 1, context, argc, argv);
}

static void
fnct_WriteSectionTiff (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionTiff(text db_prefix, text coverage, int section_id, 
/                  text tiff_path, int width, int height, BLOB geom, 
/                  double resolution)
/ WriteSectionTiff(text db_prefix, text coverage, int section_id, 
/                  text tiff_path, int width, int height, BLOB geom, 
/                  double horz_res, double vert_res)
/ WriteSectionTiff(text db_prefix, text coverage, int section_id, 
/                  text tiff_path, int width, int height, BLOB geom, 
/                  double horz_res, double vert_res, text compression)
/ WriteSectionTiff(text db_prefix, text coverage, int section_id, 
/                  text tiff_path, int width, int height, BLOB geom, 
/                  double horz_res, double vert_res, text compression, 
/                  int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_tiff (1, 0, context, argc, argv);
}

static void
common_write_jpeg (int with_worldfile, int by_section,
		   sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* common implementation for Write JPEG */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    const char *path;
    sqlite3_int64 section_id = 0;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double horz_res;
    double vert_res;
    int quality = 80;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    const void *data;
    int max_threads = 1;
    int ret;
    int errcode = -1;
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[7]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	      err = 1;
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
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
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  blob = sqlite3_value_blob (argv[6]);
	  blob_sz = sqlite3_value_bytes (argv[6]);
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[7]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[7]);
	  if (argc > 8)
	    {
		if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[8]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[8]);
	    }
	  else
	      vert_res = horz_res;
	  if (argc > 9)
	      quality = sqlite3_value_int (argv[9]);
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
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
	      quality = sqlite3_value_int (argv[8]);
      }

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (quality < 0)
	quality = 0;
    if (quality > 100)
	quality = 100;

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (by_section)
      {
	  /* single Section */
	  ret =
	      rl2_export_section_jpeg_from_dbms (sqlite, max_threads, path,
						 coverage, section_id,
						 horz_res, vert_res, minx,
						 miny, maxx, maxy, width,
						 height, quality,
						 with_worldfile);
      }
    else
      {
	  /* whole Coverage */
	  ret =
	      rl2_export_jpeg_from_dbms (sqlite, max_threads, path, coverage,
					 horz_res, vert_res, minx, miny, maxx,
					 maxy, width, height, quality,
					 with_worldfile);
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
fnct_WriteJpegJgw (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteJpegJgw(text db_prefix, text coverage, text jpeg_path, int width,
/              int height, BLOB geom, double resolution)
/ WriteJpegJgw(text db_prefix, text coverage, text jpeg_path, int width,
/              int height, BLOB geom, double horz_res, double vert_res)
/ WriteJpegJgw(text db_prefix, text coverage, text jpeg_path, int width,
/              int height, BLOB geom, double horz_res, double vert_res, 
/              int quality)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_jpeg (1, 0, context, argc, argv);
}

static void
fnct_WriteJpeg (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteJpeg(text db_prefix, text coverage, text jpeg_path, int width,
/           int height, BLOB geom, double resolution)
/ WriteJpeg(text db_prefix, text coverage, text jpeg_path, int width,
/           int height, BLOB geom, double horz_res, double vert_res)
/ WriteJpeg(text db_prefix, text coverage, text jpeg_path, int width,
/           int height, BLOB geom, double horz_res, double vert_res, 
/           int quality)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_jpeg (0, 0, context, argc, argv);
}

static void
fnct_WriteSectionJpegJgw (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionJpegJgw(text db_prefix, text coverage, int section_id, 
/                     text jpeg_path, int width, int height, BLOB geom, 
/                     double resolution)
/ WriteSectionJpegJgw(text db_prefix, text coverage, int section_id, 
/                     text jpeg_path, int width, int height, BLOB geom,
/                     double horz_res, double vert_res)
/ WriteSectionJpegJgw(text db_prefix, text coverage, int section_id, 
/                     text jpeg_path, int width, int height, BLOB geom,
/                     double horz_res, double vert_res, int quality)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_jpeg (1, 1, context, argc, argv);
}

static void
fnct_WriteSectionJpeg (sqlite3_context * context, int argc,
		       sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionJpeg(text db_prefix, text coverage, int section_id, 
/                  text jpeg_path, int width, int height, BLOB geom, 
/                  double resolution)
/ WriteSectionJpeg(text db_prefix, text coverage, int section_id, 
/                  text jpeg_path, int width, int height, BLOB geom, 
/                  double horz_res, double vert_res)
/ WriteSectionJpeg(text db_prefix, text coverage, int section_id, 
/                  text jpeg_path, int width, int height, BLOB geom, 
/                  double horz_res, double vert_res, int quality)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_jpeg (0, 1, context, argc, argv);
}

static void
common_write_triple_band_tiff (int with_worldfile, int by_section,
			       sqlite3_context * context, int argc,
			       sqlite3_value ** argv)
{
/* common implementation Write TripleBand TIFF */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    sqlite3_int64 section_id = 0;
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
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[9]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[10]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[10]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[11]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 12 && sqlite3_value_type (argv[12]) != SQLITE_TEXT)
	      err = 1;
	  if (argc > 13 && sqlite3_value_type (argv[13]) != SQLITE_INTEGER)
	      err = 1;
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[8]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[10]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_TEXT)
	      err = 1;
	  if (argc > 12 && sqlite3_value_type (argv[12]) != SQLITE_INTEGER)
	      err = 1;
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  red_band = sqlite3_value_int (argv[6]);
	  green_band = sqlite3_value_int (argv[7]);
	  blue_band = sqlite3_value_int (argv[8]);
	  blob = sqlite3_value_blob (argv[9]);
	  blob_sz = sqlite3_value_bytes (argv[9]);
	  if (sqlite3_value_type (argv[10]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[10]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[10]);
	  if (argc > 11)
	    {
		if (sqlite3_value_type (argv[11]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[11]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[11]);
	    }
	  else
	      vert_res = horz_res;
	  if (argc > 12)
	    {
		const char *compr =
		    (const char *) sqlite3_value_text (argv[12]);
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
	  if (argc > 13)
	      tile_sz = sqlite3_value_int (argv[13]);
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
	  red_band = sqlite3_value_int (argv[5]);
	  green_band = sqlite3_value_int (argv[6]);
	  blue_band = sqlite3_value_int (argv[7]);
	  blob = sqlite3_value_blob (argv[8]);
	  blob_sz = sqlite3_value_bytes (argv[8]);
	  if (sqlite3_value_type (argv[9]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[9]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[9]);
	  if (argc > 10)
	    {
		if (sqlite3_value_type (argv[10]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[10]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[10]);
	    }
	  else
	      vert_res = horz_res;
	  if (argc > 11)
	    {
		const char *compr =
		    (const char *) sqlite3_value_text (argv[11]);
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
      }

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
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

    sqlite = sqlite3_context_db_handle (context);
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be theraster's Center Point */
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (by_section)
      {
	  /* single Section */
	  if (with_worldfile)
	    {
		/* TIFF + Worldfile */
		ret =
		    rl2_export_section_triple_band_tiff_worldfile_from_dbms
		    (sqlite, path, coverage, section_id, horz_res, vert_res,
		     minx, miny, maxx, maxy, width, height, red_band,
		     green_band, blue_band, compression, tile_sz);
	    }
	  else
	    {
		/* plain TIFF, no Worldfile */
		ret =
		    rl2_export_section_triple_band_tiff_from_dbms (sqlite,
								   path,
								   coverage,
								   section_id,
								   horz_res,
								   vert_res,
								   minx, miny,
								   maxx, maxy,
								   width,
								   height,
								   red_band,
								   green_band,
								   blue_band,
								   compression,
								   tile_sz);
	    }
      }
    else
      {
	  /* whole Coverage */
	  if (with_worldfile)
	    {
		/* TIFF + Worldfile */
		ret =
		    rl2_export_triple_band_tiff_worldfile_from_dbms (sqlite,
								     path,
								     coverage,
								     horz_res,
								     vert_res,
								     minx,
								     miny,
								     maxx,
								     maxy,
								     width,
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
		    rl2_export_triple_band_tiff_from_dbms (sqlite, path,
							   coverage, horz_res,
							   vert_res, minx,
							   miny, maxx, maxy,
							   width, height,
							   red_band,
							   green_band,
							   blue_band,
							   compression,
							   tile_sz);
	    }
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
/ WriteTripleBandTiffTfw(text db_prefix, text coverage, text tiff_path, 
/                        int width, int height, int red_band, int green_band, 
/                        int blue_band, BLOB geom, double resolution)
/ WriteTripleBandTiffTfw(text db_prefix, text coverage, text tiff_path, 
/                        int width, int height, int red_band, int green_band, 
/                        int blue_band, BLOB geom, double horz_res, double vert_res)
/ WriteTripleBandTiffTfw(text db_prefix, text coverage, text tiff_path, 
/                        int width, int height, int red_band, int green_band, 
/                        int blue_band, BLOB geom, double horz_res, 
/                        double vert_res, text compression)
/ WriteTripleBandTiffTfw(text db_prefix, text coverage, text tiff_path, 
/                        int width, int height, int red_band, int green_band, 
/                        int blue_band, BLOB geom, double horz_res, 
/                        double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_triple_band_tiff (1, 0, context, argc, argv);
}

static void
fnct_WriteTripleBandTiff (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ WriteTripleBandTiff(text db_prefix, text coverage, text tiff_path, 
/                     int width, int height, int red_band, int green_band, 
/                     int blue_band, BLOB geom, double resolution)
/ WriteTripleBandTiff(text db_prefix, text coverage, text tiff_path, 
/                     int width, int height, int red_band, int green_band, 
/                     int blue_band, BLOB geom, double horz_res, 
/                     double vert_res)
/ WriteTripleBandTiff(text db_prefix, text coverage, text tiff_path, 
/                     int width, int height, int red_band, int green_band, 
/                     int blue_band, BLOB geom, double horz_res, 
/                     double vert_res, text compression)
/ WriteTripleBandTiff(text db_prefix, text coverage, text tiff_path, 
/                     int width, int height, int red_band, int green_band, 
/                     int blue_band, BLOB geom, double horz_res, 
/                     double vert_res, text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_triple_band_tiff (0, 0, context, argc, argv);
}

static void
fnct_WriteSectionTripleBandTiffTfw (sqlite3_context * context, int argc,
				    sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionTripleBandTiffTfw(text db_prefix, text coverage, 
/                               int section_id, text tiff_path, 
/                               int width, int height, int red_band,
/                               int green_band, int blue_band, BLOB geom, 
/                               double resolution)
/ WriteSectionTripleBandTiffTfw(text db_prefix, text coverage, 
/                               int section_id, text tiff_path, 
/                               int width, int height, int red_band,
/                               int green_band, int blue_band, BLOB geom, 
/                               double horz_res, double vert_res)
/ WriteSectionTripleBandTiffTfw(text db_prefix, text coverage, 
/                               int section_id, text tiff_path, 
/                               int width, int height, int red_band, 
/                               int green_band, int blue_band, BLOB geom, 
/                               double horz_res, double vert_res, 
/                               text compression)
/ WriteSectionTripleBandTiffTfw(text db_prefix, text coverage, 
/                               int section_id, text tiff_path, 
/                               int width, int height, int red_band,
/                               int green_band, int blue_band, BLOB geom, 
/                               double horz_res, double vert_res, 
/                               text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_triple_band_tiff (1, 1, context, argc, argv);
}

static void
fnct_WriteSectionTripleBandTiff (sqlite3_context * context, int argc,
				 sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionTripleBandTiff(text db_prefix, text coverage, 
/                            int section_id, text tiff_path, 
/                            int width, int height, int red_band, 
/                            int green_band, int blue_band, BLOB geom, 
/                            double resolution)
/ WriteSectionTripleBandTiff(text db_prefix, text coverage, 
/                            int section_id, text tiff_path,
/                            int width, int height, int red_band, 
/                            int green_band, int blue_band, BLOB geom, 
/                            double horz_res, double vert_res)
/ WriteSectionTripleBandTiff(text db_prefix, text coverage, 
/                            int section_id, text tiff_path, 
/                            int width, int height, int red_band, 
/                            int green_band, int blue_band, BLOB geom, 
/                            double horz_res, double vert_res,
/                            text compression)
/ WriteSectionTripleBandTiff(text db_prefix, text coverage, 
/                            int section_id, text tiff_path, 
/                            int width, int height, int red_band, 
/                            int green_band, int blue_band, BLOB geom, 
/                            double horz_res, double vert_res,
/                            text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_triple_band_tiff (0, 1, context, argc, argv);
}

static void
common_write_mono_band_tiff (int with_worldfile, int by_section,
			     sqlite3_context * context, int argc,
			     sqlite3_value ** argv)
{
/* common implementation Write Mono Band TIFF */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    sqlite3_int64 section_id = 0;
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
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
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
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[7]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[8]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_TEXT)
	      err = 1;
	  if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	      err = 1;
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  mono_band = sqlite3_value_int (argv[6]);
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
		const char *compr =
		    (const char *) sqlite3_value_text (argv[10]);
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
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
	  mono_band = sqlite3_value_int (argv[5]);
	  blob = sqlite3_value_blob (argv[6]);
	  blob_sz = sqlite3_value_bytes (argv[6]);
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[7]);
		horz_res = ival;
	    }
	  else
	      horz_res = sqlite3_value_double (argv[7]);
	  if (argc > 8)
	    {
		if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
		  {
		      int ival = sqlite3_value_int (argv[8]);
		      vert_res = ival;
		  }
		else
		    vert_res = sqlite3_value_double (argv[8]);
	    }
	  else
	      vert_res = horz_res;
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
      }

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
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

    sqlite = sqlite3_context_db_handle (context);
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (by_section)
      {
	  /* single Section */
	  if (with_worldfile)
	    {
		/* TIFF + Worldfile */
		ret =
		    rl2_export_section_mono_band_tiff_worldfile_from_dbms
		    (sqlite, path, coverage, section_id, horz_res, vert_res,
		     minx, miny, maxx, maxy, width, height, mono_band,
		     compression, tile_sz);
	    }
	  else
	    {
		/* plain TIFF, no Worldfile */
		ret =
		    rl2_export_section_mono_band_tiff_from_dbms (sqlite, path,
								 coverage,
								 section_id,
								 horz_res,
								 vert_res,
								 minx, miny,
								 maxx, maxy,
								 width,
								 height,
								 mono_band,
								 compression,
								 tile_sz);
	    }
      }
    else
      {
	  /* whole Coverage */
	  if (with_worldfile)
	    {
		/* TIFF + Worldfile */
		ret =
		    rl2_export_mono_band_tiff_worldfile_from_dbms (sqlite,
								   path,
								   coverage,
								   horz_res,
								   vert_res,
								   minx, miny,
								   maxx, maxy,
								   width,
								   height,
								   mono_band,
								   compression,
								   tile_sz);
	    }
	  else
	    {
		/* plain TIFF, no Worldfile */
		ret =
		    rl2_export_mono_band_tiff_from_dbms (sqlite, path,
							 coverage, horz_res,
							 vert_res, minx, miny,
							 maxx, maxy, width,
							 height, mono_band,
							 compression, tile_sz);
	    }
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
/ WriteMonoBandTiffTfw(text db_prefix, text coverage, text tiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double resolution)
/ WriteMonoBandTiffTfw(text db_prefix, text coverage, text tiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double horz_res, double vert_res)
/ WriteMonoBandTiffTfw(text db_prefix, text coverage, text tiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double horz_res, double vert_res, text compression)
/ WriteMonoBandTiffTfw(text db_prefix, text coverage, text tiff_path, 
/                      int width, int height, int mono_band, BLOB geom, 
/                      double horz_res, double vert_res, text compression, 
/                      int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_mono_band_tiff (1, 0, context, argc, argv);
}

static void
fnct_WriteMonoBandTiff (sqlite3_context * context, int argc,
			sqlite3_value ** argv)
{
/* SQL function:
/ WriteMonoBandTiff(text db_prefix, text coverage, text tiff_path, 
/                   int width, int height, int mono_band, BLOB geom, 
/                   double resolution)
/ WriteMonoBandTiff(text db_prefix, text coverage, text tiff_path, 
/                   int width, int height, int mono_band, BLOB geom, 
/                   double horz_res, double vert_res)
/ WriteMonoBandTiff(text db_prefix, text coverage, text tiff_path, 
/                   int width, int height, int mono_band, BLOB geom, 
/                   double horz_res, double vert_res, text compression)
/ WriteMonoBandTiff(text db_prefix, text coverage, text tiff_path, 
/                   int width, int height, int mono_band, BLOB geom, 
/                   double horz_res, double vert_res, text compression, 
/                   int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_mono_band_tiff (0, 0, context, argc, argv);
}

static void
fnct_WriteSectionMonoBandTiffTfw (sqlite3_context * context, int argc,
				  sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionMonoBandTiffTfw(text db_prefix, text coverage, 
/                             int section_id, text tiff_path, int width, 
/                             int height, int mono_band, BLOB geom, 
/                             double resolution)
/ WriteSectionMonoBandTiffTfw(text db_prefix, text coverage, 
/                             int section_id, text tiff_path, int width, 
/                             int height, int mono_band, BLOB geom, 
/                             double horz_res, double vert_res)
/ WriteSectionMonoBandTiffTfw(text db_prefix, text coverage, 
/                             int section_id, text tiff_path, int width, 
/                             int height, int mono_band, BLOB geom, 
/                             double horz_res, double vert_res,
/                             text compression)
/ WriteSectionMonoBandTiffTfw(text db_prefix, text coverage, 
/                             int section_id, text tiff_path, int width, 
/                             int height, int mono_band, BLOB geom, 
/                             double horz_res, double vert_res,
/                             text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_mono_band_tiff (1, 1, context, argc, argv);
}

static void
fnct_WriteSectionMonoBandTiff (sqlite3_context * context, int argc,
			       sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionMonoBandTiff(text db_prefix, text coverage, 
/                          int section_id, text tiff_path, int width, 
/                          int height, int mono_band, BLOB geom, 
/                          double resolution)
/ WriteSectionMonoBandTiff(text db_prefix, text coverage, 
/                          int section_id, text tiff_path, int width, 
/                          int height, int mono_band, BLOB geom, 
/                          double horz_res, double vert_res)
/ WriteSectionMonoBandTiff(text db_prefix, text coverage, 
/                          int section_id, text tiff_path, int width, 
/                          int height, int mono_band, BLOB geom, 
/                          double horz_res, double vert_res,
/                          text compression)
/ WriteSectionMonoBandTiff(text db_prefix, text coverage, 
/                          int section_id, text tiff_path, int width, \
/                          int height, int mono_band, BLOB geom, 
/                          double horz_res, double vert_res,
/                          text compression, int tile_sz)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_mono_band_tiff (0, 1, context, argc, argv);
}

static void
common_write_ascii_grid (int by_section, sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* common export ASCII Grid implementation */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    const char *path;
    sqlite3_int64 section_id = 0;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    double resolution;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    const void *data;
    int max_threads = 1;
    int ret;
    int errcode = -1;
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    int is_centered = 1;
    int decimal_digits = 4;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[7]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	      err = 1;
	  if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	      err = 1;
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
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
	  if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	      err = 1;
	  if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	      err = 1;
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  blob = sqlite3_value_blob (argv[6]);
	  blob_sz = sqlite3_value_bytes (argv[6]);
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[7]);
		resolution = ival;
	    }
	  else
	      resolution = sqlite3_value_double (argv[7]);
	  if (argc > 8)
	      is_centered = sqlite3_value_int (argv[8]);
	  if (argc > 9)
	      decimal_digits = sqlite3_value_int (argv[9]);
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
	  blob = sqlite3_value_blob (argv[5]);
	  blob_sz = sqlite3_value_bytes (argv[5]);
	  if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[6]);
		resolution = ival;
	    }
	  else
	      resolution = sqlite3_value_double (argv[6]);
	  if (argc > 7)
	      is_centered = sqlite3_value_int (argv[7]);
	  if (argc > 8)
	      decimal_digits = sqlite3_value_int (argv[8]);
      }

    if (decimal_digits < 1)
	decimal_digits = 0;
    if (decimal_digits > 18)
	decimal_digits = 18;

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
      {
	  errcode = -1;
	  goto error;
      }

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * resolution;
	  double ext_y = (double) height * resolution;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (by_section)
      {
	  /* single Section */
	  ret =
	      rl2_export_section_ascii_grid_from_dbms (sqlite, max_threads,
						       path, coverage,
						       section_id, resolution,
						       minx, miny, maxx, maxy,
						       width, height,
						       is_centered,
						       decimal_digits);
      }
    else
      {
	  /* whole Coverage */
	  ret =
	      rl2_export_ascii_grid_from_dbms (sqlite, max_threads, path,
					       coverage, resolution, minx,
					       miny, maxx, maxy, width,
					       height, is_centered,
					       decimal_digits);
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
fnct_WriteAsciiGrid (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ WriteAsciiGrid(text db_prefix, text coverage, text ascii_path, 
/                int width, int height, BLOB geom, double resolution)
/ WriteAsciiGrid(text db_prefix, text coverage, text ascii_path, 
/                int width, int height, BLOB geom, double resolution, 
/                int is_centered)
/ WriteAsciiGrid(text db_prefix, text coverage, text ascii_path, 
/                int width, int height, BLOB geom, double resolution, 
/                int is_centered, int decimal_digits)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_ascii_grid (0, context, argc, argv);
}

static void
fnct_WriteSectionAsciiGrid (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionAsciiGrid(text db_prefix, text coverage, int section_id,
/                       text ascii_path, int width, int height, 
/                       BLOB geom, double resolution)
/ WriteSectionAsciiGrid(text db_prefix, text coverage, int section_id,
/                       text ascii_path, int width, int height,
/                       BLOB geom, double resolution, int is_centered)
/ WriteSectionAsciiGrid(text db_prefix, text coverage, int section_id,
/                       text ascii_path, int width, int height,
/                       BLOB geom, double resolution, int is_centered,
/                       int decimal_digits)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_ascii_grid (1, context, argc, argv);
}

static void
common_write_ndvi_ascii_grid (int by_section, sqlite3_context * context,
			      int argc, sqlite3_value ** argv)
{
/* common export NDVI ASCII Grid implementation */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    const char *path;
    sqlite3_int64 section_id = 0;
    int width;
    int height;
    int red_band;
    int nir_band;
    const unsigned char *blob;
    int blob_sz;
    double resolution;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    const void *data;
    int max_threads = 1;
    int ret;
    int errcode = -1;
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    int is_centered = 1;
    int decimal_digits = 4;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[3]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	      err = 1;
	  if (sqlite3_value_type (argv[8]) != SQLITE_BLOB)
	      err = 1;
	  if (sqlite3_value_type (argv[9]) != SQLITE_INTEGER
	      && sqlite3_value_type (argv[9]) != SQLITE_FLOAT)
	      err = 1;
	  if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	      err = 1;
	  if (argc > 11 && sqlite3_value_type (argv[11]) != SQLITE_INTEGER)
	      err = 1;
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	      err = 1;
	  if (sqlite3_value_type (argv[2]) != SQLITE_TEXT)
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
	  if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	      err = 1;
	  if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	      err = 1;
      }
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* single Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  path = (const char *) sqlite3_value_text (argv[3]);
	  width = sqlite3_value_int (argv[4]);
	  height = sqlite3_value_int (argv[5]);
	  red_band = sqlite3_value_int (argv[6]);
	  nir_band = sqlite3_value_int (argv[7]);
	  blob = sqlite3_value_blob (argv[8]);
	  blob_sz = sqlite3_value_bytes (argv[8]);
	  if (sqlite3_value_type (argv[9]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[9]);
		resolution = ival;
	    }
	  else
	      resolution = sqlite3_value_double (argv[9]);
	  if (argc > 10)
	      is_centered = sqlite3_value_int (argv[10]);
	  if (argc > 11)
	      decimal_digits = sqlite3_value_int (argv[11]);
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  path = (const char *) sqlite3_value_text (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
	  red_band = sqlite3_value_int (argv[5]);
	  nir_band = sqlite3_value_int (argv[6]);
	  blob = sqlite3_value_blob (argv[7]);
	  blob_sz = sqlite3_value_bytes (argv[7]);
	  if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
	    {
		int ival = sqlite3_value_int (argv[8]);
		resolution = ival;
	    }
	  else
	      resolution = sqlite3_value_double (argv[8]);
	  if (argc > 9)
	      is_centered = sqlite3_value_int (argv[9]);
	  if (argc > 10)
	      decimal_digits = sqlite3_value_int (argv[10]);
      }

    if (decimal_digits < 1)
	decimal_digits = 0;
    if (decimal_digits > 18)
	decimal_digits = 18;

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
      {
	  errcode = -1;
	  goto error;
      }

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * resolution;
	  double ext_y = (double) height * resolution;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    if (by_section)
      {
	  /* single Section */
	  ret =
	      rl2_export_section_ndvi_ascii_grid_from_dbms (sqlite,
							    max_threads, path,
							    coverage,
							    section_id,
							    resolution, minx,
							    miny, maxx, maxy,
							    width, height,
							    red_band,
							    nir_band,
							    is_centered,
							    decimal_digits);
      }
    else
      {
	  /* whole Coverage */
	  ret =
	      rl2_export_ndvi_ascii_grid_from_dbms (sqlite, max_threads, path,
						    coverage, resolution,
						    minx, miny, maxx, maxy,
						    width, height, red_band,
						    nir_band, is_centered,
						    decimal_digits);
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
fnct_WriteNdviAsciiGrid (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ WriteNdviAsciiGrid(text db_prefix, text coverage, text ascii_path, 
/                    int width, int height, int red_band, int nir_band, 
/                    BLOB geom, double resolution)
/ WriteNdviAsciiGrid(text db_prefix, text coverage, text ascii_path, 
/                    int width, int height, int red_band, int nir_band, 
/                    BLOB geom, double resolution, int is_centered)
/ WriteNdviAsciiGrid(text db_prefix, text coverage, text ascii_path, 
/                    int width, int height, int red_band, int nir_band, 
/                    BLOB geom, double resolution, int is_centered, 
/                    int decimal_digits)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_ndvi_ascii_grid (0, context, argc, argv);
}

static void
fnct_WriteSectionNdviAsciiGrid (sqlite3_context * context, int argc,
				sqlite3_value ** argv)
{
/* SQL function:
/ WriteSectionNdviAsciiGrid(text db_prefix, text coverage, int section_id,
/                           text ascii_path, int width, int height, 
/                           int red_band, int nir_band, BLOB geom, 
/                           double resolution)
/ WriteSectionNdviAsciiGrid(text db_prefix, text coverage, int section_id,
/                           text ascii_path, int width, int height,
/                           int red_band, int nir_band, BLOB geom, 
/                           double resolution, int is_centered)
/ WriteSectionNdviAsciiGrid(text db_prefix, text coverage, int section_id,
/                           text ascii_path, int width, int height,
/                           int red_band, int nir_band, BLOB geom, 
/                           double resolution, int is_centered,
/                           int decimal_digits)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    common_write_ndvi_ascii_grid (1, context, argc, argv);
}

static void
fnct_GetDrapingLastError (sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* SQL function:
/ GetDrapingLastError()
/
/ will return the latest Draping message (Error or Warning)
/ or NULL if no such message exists
/
*/
    struct rl2_private_data *priv_data = sqlite3_user_data (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    if (priv_data == NULL)
	sqlite3_result_null (context);
    else if (priv_data->draping_message == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_text (context, priv_data->draping_message, -1,
			     SQLITE_STATIC);
}

static void
fnct_DrapeGeometries (sqlite3_context * context, int argc,
		      sqlite3_value ** argv)
{
/* SQL function:
/ DrapeGeometries(text db_prefix, text raster_coverage, text coverage_list_table,
/                 text spatial_table, text geom2d, text geom3d)
/    or
/ DrapeGeometries(text db_prefix, text raster_coverage, text coverage_list_table,
/                 text spatial_table, text geom2d, text geom3d, 
/                 double no_data_value)
/    or
/ DrapeGeometries(text db_prefix, text raster_coverage, text coverage_list_table,
/                 text spatial_table, text geom2d, text geom3d, 
/                 double no_data_value, double densify_dist, 
/                 double z_simplify_dist)
/    or
/ DrapeGeometries(text db_prefix, text raster_coverage, text coverage_list_table,
/                 text spatial_table, text geom2d, text geom3d, 
/                 double no_data_value, double densify_dist, 
/                 double z_simplify_dist, bool update_m)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *db_prefix = NULL;
    const char *raster_coverage = NULL;
    const char *coverage_list_table = NULL;
    const char *spatial_table = NULL;
    const char *old_geom;
    const char *new_geom;
    double no_data_value = 0.0;
    double densify_dist = 0.0;
    double z_simplify_dist = 0.0;
    int update_m = 0;
    sqlite3 *sqlite;
    struct rl2_private_data *priv_data = sqlite3_user_data (context);
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */
    sqlite = sqlite3_context_db_handle (context);
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	db_prefix = NULL;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    else
	err = 1;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	raster_coverage = NULL;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	raster_coverage = (const char *) sqlite3_value_text (argv[1]);
    else
	err = 1;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	coverage_list_table = NULL;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	coverage_list_table = (const char *) sqlite3_value_text (argv[2]);
    else
	err = 1;
    if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	spatial_table = (const char *) sqlite3_value_text (argv[3]);
    else
	err = 1;
    if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	old_geom = (const char *) sqlite3_value_text (argv[4]);
    else
	err = 1;
    if (sqlite3_value_type (argv[5]) == SQLITE_TEXT)
	new_geom = (const char *) sqlite3_value_text (argv[5]);
    else
	err = 1;
    if (argc >= 7)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int val = sqlite3_value_int (argv[6]);
		no_data_value = val;
	    }
	  else if (sqlite3_value_type (argv[6]) == SQLITE_FLOAT)
	      no_data_value = sqlite3_value_double (argv[6]);
	  else
	      err = 1;
      }
    if (argc >= 8)
      {
	  if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	    {
		int val = sqlite3_value_int (argv[7]);
		densify_dist = val;
	    }
	  else if (sqlite3_value_type (argv[7]) == SQLITE_FLOAT)
	      densify_dist = sqlite3_value_double (argv[7]);
	  else
	      err = 1;
      }
    if (argc >= 9)
      {
	  if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
	    {
		int val = sqlite3_value_int (argv[8]);
		z_simplify_dist = val;
	    }
	  else if (sqlite3_value_type (argv[8]) == SQLITE_FLOAT)
	      z_simplify_dist = sqlite3_value_double (argv[8]);
	  else
	      err = 1;
      }
    if (argc >= 10)
      {
	  if (sqlite3_value_type (argv[9]) == SQLITE_INTEGER)
	      update_m = sqlite3_value_int (argv[9]);
	  else
	      err = 1;
      }
    if (db_prefix == NULL && raster_coverage == NULL
	&& coverage_list_table == NULL)
	err = 1;
    if (db_prefix == NULL && raster_coverage != NULL
	&& coverage_list_table != NULL)
	err = 1;
    if (db_prefix != NULL && raster_coverage != NULL
	&& coverage_list_table != NULL)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    rl2_reset_draping_message (priv_data);
    if (rl2_drape_geometries
	(sqlite, priv_data, db_prefix, raster_coverage, coverage_list_table,
	 spatial_table, old_geom, new_geom, no_data_value, densify_dist,
	 z_simplify_dist, update_m))
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
}


static int
do_find_section_by_point (sqlite3 * handle, const char *db_prefix,
			  const char *coverage, const unsigned char *blob,
			  int blob_sz, sqlite3_int64 * section_id)
{
/* retrieving the Section ID */
    int ret;
    char *xdb_prefix;
    char *xsections;
    char *xxsections;
    char *sql;
    sqlite3_stmt *stmt = NULL;

    *section_id = 0;
/* preparing the "sections" SQL query */
    if (db_prefix == NULL)
	db_prefix = "main";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    xsections = sqlite3_mprintf ("DB=%s.%s_sections", db_prefix, coverage);
    xxsections = rl2_double_quoted_sql (xsections);
    sql =
	sqlite3_mprintf
	("SELECT section_id FROM \"%s\".\"%s\" WHERE ROWID IN ( "
	 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	 "AND search_frame = ?)", xdb_prefix, xxsections, xsections);
    sqlite3_free (xsections);
    free (xdb_prefix);
    free (xxsections);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT mixed-res Sections SQL error: %s\n",
		  sqlite3_errmsg (handle));
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
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		*section_id = sqlite3_column_int64 (stmt, 0);
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
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

static void
fnct_GetPixelFromRasterByPoint (sqlite3_context * context, int argc,
				sqlite3_value ** argv)
{
/* SQL function:
/ GetPixelFromRasterByPoint(text db_prefix, text coverage, 
/                           BLOB point, int pyramid_level)
/   or
/ GetPixelFromRasterByPoint(text db_prefix, text coverage, 
/                           BLOB point, double x_res, double y_res)
/
/ will return a BLOB-Pixel from a Raster Coverage
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    int pyramid_level;
    double x_res;
    double y_res;
    int by_res;
    const unsigned char *blob;
    int blob_sz;
    sqlite3 *sqlite;
    const void *data;
    rl2PixelPtr pixel = NULL;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

/* testing arguments for validity */
    err = 0;
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_BLOB)
	err = 1;
    if (argc >= 5)
      {
	  if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER
	      || sqlite3_value_type (argv[3]) == SQLITE_FLOAT)
	      ;
	  else
	      err = 1;
	  if (sqlite3_value_type (argv[4]) == SQLITE_INTEGER
	      || sqlite3_value_type (argv[4]) == SQLITE_FLOAT)
	      ;
	  else
	      err = 1;
	  by_res = 1;
      }
    else
      {
	  if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	      err = 1;
	  by_res = 0;
      }
    if (err != 0)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    cvg_name = (const char *) sqlite3_value_text (argv[1]);
    blob = sqlite3_value_blob (argv[2]);
    blob_sz = sqlite3_value_bytes (argv[2]);
    if (argc >= 5)
      {
	  int value;
	  if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER)
	    {
		value = sqlite3_value_int (argv[3]);
		x_res = value;
	    }
	  else
	      x_res = sqlite3_value_double (argv[3]);
	  if (sqlite3_value_type (argv[4]) == SQLITE_INTEGER)
	    {
		value = sqlite3_value_int (argv[4]);
		y_res = value;
	    }
	  else
	      y_res = sqlite3_value_double (argv[4]);
      }
    else
	pyramid_level = sqlite3_value_int (argv[3]);

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);

    if (by_res)
      {
	  /* attempting to retrieve the optimal resolution level */
	  int scale;
	  int xscale;
	  double xx_res;
	  double yy_res;
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	    {
		/* Mixed Resolutions Coverage */
		sqlite3_int64 section_id;
		if (!do_find_section_by_point
		    (sqlite, db_prefix, cvg_name, blob, blob_sz, &section_id))
		    goto error;
		if (!rl2_find_best_resolution_level
		    (sqlite, db_prefix, cvg_name, 1, section_id, x_res, y_res,
		     &pyramid_level, &scale, &xscale, &xx_res, &yy_res))
		    goto error;
	    }
	  else
	    {
		/* ordinary Coverage */
		if (!rl2_find_best_resolution_level
		    (sqlite, db_prefix, cvg_name, 0, 0, x_res, y_res,
		     &pyramid_level, &scale, &xscale, &xx_res, &yy_res))
		    goto error;
	    }
      }

    if (rl2_pixel_from_raster_by_point
	(sqlite, data, db_prefix, cvg_name, pyramid_level, blob, blob_sz,
	 &pixel) != RL2_OK)
	sqlite3_result_null (context);
    else
      {
	  unsigned char *xblob;
	  int xblob_sz;
	  if (rl2_serialize_dbms_pixel (pixel, &xblob, &xblob_sz) != RL2_OK)
	      sqlite3_result_null (context);
	  else
	      sqlite3_result_blob (context, xblob, xblob_sz, free);
      }
    if (pixel != NULL)
	rl2_destroy_pixel (pixel);
    return;

  error:
    sqlite3_result_null (context);
    return;
}

static void
fnct_GetMapImageFromRaster (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ GetMapImageFromRaster(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height)
/ GetMapImageFromRaster(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style)
/ GetMapImageFromRaster(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format)
/ GetMapImageFromRaster(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format, 
/                       text bg_color)
/ GetMapImageFromRaster(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format, 
/                       text bg_color, int transparent)
/ GetMapImageFromRaster(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format, 
/                       text bg_color, int transparent, int quality)
/ GetMapImageFromRaster(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format, 
/                       text bg_color, int transparent, int quality, 
/                       int reaspect)
/
/ will return a BLOB containing the Image payload from a Raster Coverage
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    const char *style = "default";
    const char *format = "image/png";
    const char *bg_color = "#ffffff";
    int transparent = 0;
    int quality = 80;
    int reaspect = 0;
    sqlite3 *sqlite;
    const void *data;
    unsigned char *image = NULL;
    int image_size;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

/* testing arguments for validity */
    err = 0;
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_TEXT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_TEXT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_TEXT)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	err = 1;
    if (err != 0)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    cvg_name = (const char *) sqlite3_value_text (argv[1]);
    blob = sqlite3_value_blob (argv[2]);
    blob_sz = sqlite3_value_bytes (argv[2]);
    width = sqlite3_value_int (argv[3]);
    height = sqlite3_value_int (argv[4]);
    if (argc > 5)
	style = (const char *) sqlite3_value_text (argv[5]);
    if (argc > 6)
	format = (const char *) sqlite3_value_text (argv[6]);
    if (argc > 7)
	bg_color = (const char *) sqlite3_value_text (argv[7]);
    if (argc > 8)
	transparent = sqlite3_value_int (argv[8]);
    if (argc > 9)
	quality = sqlite3_value_int (argv[9]);
    if (argc > 10)
	reaspect = sqlite3_value_int (argv[10]);

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);

    if (strcasecmp (format, "image/png") != 0)
      {
	  /* only PNG can support real transparencies */
	  transparent = 0;
      }

    if (rl2_map_image_blob_from_raster
	(sqlite, data, db_prefix, cvg_name, blob, blob_sz, width, height, style,
	 format, bg_color, transparent, quality, reaspect, &image,
	 &image_size) != RL2_OK)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, image, image_size, free);
}

static void
fnct_GetMapImageFromVector (sqlite3_context * context, int argc,
			    sqlite3_value ** argv)
{
/* SQL function:
/ GetMapImageFromVector(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height)
/ GetMapImageFromVector(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style)
/ GetMapImageFromVector(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format)
/ GetMapImageFromVector(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format, 
/                       text bg_color)
/ GetMapImageFromVector(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format, 
/                       text bg_color, int transparent)
/ GetMapImageFromVector(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format, 
/                       text bg_color, int transparent, int quality)
/ GetMapImageFromVector(text db_prefix, text coverage, BLOB geom, 
/                       int width, int height, text style, text format, 
/                       text bg_color, int transparent, int quality, 
/                       int reaspect)
/
/ will return a BLOB containing the Image payload from a Vector Coverage
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    const char *style = "default";
    const char *format = "image/png";
    const char *bg_color = "#ffffff";
    int transparent = 0;
    int quality = 80;
    int reaspect = 0;
    sqlite3 *sqlite;
    const void *data;
    unsigned char *image = NULL;
    int image_size;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

/* testing arguments for validity */
    err = 0;
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_TEXT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_TEXT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_TEXT)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 10 && sqlite3_value_type (argv[10]) != SQLITE_INTEGER)
	err = 1;
    if (err != 0)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    cvg_name = (const char *) sqlite3_value_text (argv[1]);
    blob = sqlite3_value_blob (argv[2]);
    blob_sz = sqlite3_value_bytes (argv[2]);
    width = sqlite3_value_int (argv[3]);
    height = sqlite3_value_int (argv[4]);
    if (argc > 5)
	style = (const char *) sqlite3_value_text (argv[5]);
    if (argc > 6)
	format = (const char *) sqlite3_value_text (argv[6]);
    if (argc > 7)
	bg_color = (const char *) sqlite3_value_text (argv[7]);
    if (argc > 8)
	transparent = sqlite3_value_int (argv[8]);
    if (argc > 9)
	quality = sqlite3_value_int (argv[9]);
    if (argc > 10)
	reaspect = sqlite3_value_int (argv[10]);

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);

    if (strcasecmp (format, "image/png") != 0)
      {
	  /* only PNG can support real transparencies */
	  transparent = 0;
      }

    if (rl2_map_image_blob_from_vector
	(sqlite, data, db_prefix, cvg_name, blob, blob_sz, width, height, style,
	 format, bg_color, transparent, quality, reaspect, &image,
	 &image_size) != RL2_OK)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, image, image_size, free);
}

static void
fnct_GetMapImageFromWMS (sqlite3_context * context, int argc,
			 sqlite3_value ** argv)
{
/* SQL function:
/ GetMapImageFromWMS(text db_prefix, text coverage, BLOB geom, 
/                    int width, int height)
/ GetMapImageFromWMS(text db_prefix, text coverage, BLOB geom, 
/                    int width, int height, text wms_version,
/                    text style, text format, text bg_color, 
/                    int transparent)
/
/ will return a BLOB containing the Image payload from a WMS Coverage
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    int width;
    int height;
    const unsigned char *blob;
    int blob_sz;
    const char *version = "1.0.0";
    const char *style = "default";
    const char *format = "image/png";
    const char *bg_color = "#ffffff";
    int transparent = 0;
    sqlite3 *sqlite;
    unsigned char *image = NULL;
    int image_size;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

/* testing arguments for validity */
    err = 0;
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_BLOB)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[4]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_TEXT)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_TEXT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_TEXT)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_TEXT)
	err = 1;
    if (argc > 9 && sqlite3_value_type (argv[9]) != SQLITE_INTEGER)
	err = 1;
    if (err != 0)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    cvg_name = (const char *) sqlite3_value_text (argv[1]);
    blob = sqlite3_value_blob (argv[2]);
    blob_sz = sqlite3_value_bytes (argv[2]);
    width = sqlite3_value_int (argv[3]);
    height = sqlite3_value_int (argv[4]);
    if (argc > 5)
	version = (const char *) sqlite3_value_text (argv[5]);
    if (argc > 6)
	style = (const char *) sqlite3_value_text (argv[6]);
    if (argc > 7)
	format = (const char *) sqlite3_value_text (argv[7]);
    if (argc > 8)
	bg_color = (const char *) sqlite3_value_text (argv[8]);
    if (argc > 9)
	transparent = sqlite3_value_int (argv[9]);

    sqlite = sqlite3_context_db_handle (context);

    if (strcasecmp (format, "image/png") != 0)
      {
	  /* only PNG can support real transparencies */
	  transparent = 0;
      }

    image = rl2_map_image_from_wms
	(sqlite, db_prefix, cvg_name, blob, blob_sz, width, height, version,
	 style, format, transparent, bg_color, &image_size);
    if (image == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, image, image_size, free);
}

static void
fnct_GetTileImage (sqlite3_context * context, int argc, sqlite3_value ** argv)
{
/* SQL function:
/ GetTileImage(text db_prefix, text coverage, int tile_id)
/
/ will return a BLOB containing the Image payload
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    rl2CoveragePtr coverage = NULL;
    rl2PrivCoveragePtr cvg;
    sqlite3_int64 tile_id;
    int pyramid_level;
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
    char *xdb_prefix;
    char *sql;
    int ret;
    const unsigned char *blob_odd = NULL;
    const unsigned char *blob_even = NULL;
    int blob_odd_sz = 0;
    int blob_even_sz = 0;
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
    unsigned char num_bands;
    rl2PrivPixelPtr no_data = NULL;
    double opacity = 1.0;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    cvg_name = (const char *) sqlite3_value_text (argv[1]);
    tile_id = sqlite3_value_int64 (argv[2]);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
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
      case RL2_PIXEL_MULTIBAND:
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
	  palette = rl2_get_dbms_palette (sqlite, db_prefix, cvg_name);
	  if (palette == NULL)
	      goto error;
      }
    no_data = cvg->noData;

/* querying the tile */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    table_tile_data = sqlite3_mprintf ("%s_tile_data", cvg_name);
    xtable_tile_data = rl2_double_quoted_sql (table_tile_data);
    sqlite3_free (table_tile_data);
    table_tiles = sqlite3_mprintf ("%s_tiles", cvg_name);
    xtable_tiles = rl2_double_quoted_sql (table_tiles);
    sqlite3_free (table_tiles);
    sql = sqlite3_mprintf ("SELECT d.tile_data_odd, d.tile_data_even, "
			   "t.pyramid_level FROM \"%s\".\"%s\" AS d "
			   "JOIN \"%s\".\"%s\" AS t ON (t.tile_id = d.tile_id) "
			   "WHERE t.tile_id = ?", xdb_prefix, xtable_tile_data,
			   xdb_prefix, xtable_tiles);
    free (xtable_tile_data);
    free (xtable_tiles);
    free (xdb_prefix);
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
		num_bands = rst->nBands;
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
			    *p_rgba++ = 0;
			    *p_rgba++ = 0;
			    *p_rgba++ = 0;
			    *p_rgba++ = 0;	/* transparent */
			}
		  }
		switch (pixel_type)
		  {
		  case RL2_PIXEL_MONOCHROME:
		      ret =
			  get_rgba_from_monochrome_mask (width, height,
							 buffer, mask, rgba);
		      buffer = NULL;
		      mask = NULL;
		      if (!ret)
			  goto error;
		      if (!build_rgb_alpha_transparent
			  (width, height, rgba, &rgb, &alpha))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (!get_payload_from_gray_rgba_transparent
			  (width, height, rgb, alpha,
			   RL2_OUTPUT_FORMAT_PNG, 100, &image,
			   &image_size, opacity))
			  goto error;
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
		      if (!build_rgb_alpha_transparent
			  (width, height, rgba, &rgb, &alpha))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (!get_payload_from_rgb_rgba_transparent
			  (width, height, rgb, alpha,
			   RL2_OUTPUT_FORMAT_PNG, 100, &image,
			   &image_size, opacity, 0))
			  goto error;
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
		      if (!build_rgb_alpha_transparent
			  (width, height, rgba, &rgb, &alpha))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (!get_payload_from_gray_rgba_transparent
			  (width, height, rgb, alpha,
			   RL2_OUTPUT_FORMAT_PNG, 100, &image,
			   &image_size, opacity))
			  goto error;
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
		      if (!build_rgb_alpha_transparent
			  (width, height, rgba, &rgb, &alpha))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (!get_payload_from_gray_rgba_transparent
			  (width, height, rgb, alpha,
			   RL2_OUTPUT_FORMAT_PNG, 100, &image,
			   &image_size, opacity))
			  goto error;
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  case RL2_PIXEL_RGB:
		      if (sample_type == RL2_SAMPLE_UINT16)
			{
			    ret =
				get_rgba_from_multiband16 (width, height, 0,
							   1, 2, 3,
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
		      if (!build_rgb_alpha_transparent
			  (width, height, rgba, &rgb, &alpha))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (!get_payload_from_rgb_rgba_transparent
			  (width, height, rgb, alpha,
			   RL2_OUTPUT_FORMAT_PNG, 100, &image,
			   &image_size, opacity, 0))
			  goto error;
		      sqlite3_result_blob (context, image, image_size, free);
		      break;
		  case RL2_PIXEL_MULTIBAND:
		      ret =
			  get_rgba_from_multiband_mask (width, height,
							sample_type,
							num_bands, buffer,
							mask, no_data, rgba);
		      buffer = NULL;
		      mask = NULL;
		      if (!ret)
			  goto error;
		      if (!build_rgb_alpha_transparent
			  (width, height, rgba, &rgb, &alpha))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (!get_payload_from_gray_rgba_transparent
			  (width, height, rgb, alpha,
			   RL2_OUTPUT_FORMAT_PNG, 100, &image,
			   &image_size, opacity))
			  goto error;
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
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
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
get_triple_band_tile_image (sqlite3_context * context, const char *db_prefix,
			    const char *cvg_name, sqlite3_int64 tile_id,
			    unsigned char red_band, unsigned char green_band,
			    unsigned char blue_band, unsigned char bg_red,
			    unsigned char bg_green, unsigned char bg_blue,
			    int transparent)
{
/* common implementation: TileImage (mono/triple-band) */
    rl2CoveragePtr coverage = NULL;
    rl2PrivCoveragePtr cvg;
    unsigned short width;
    unsigned short height;
    sqlite3 *sqlite;
    sqlite3_stmt *stmt = NULL;
    int unsupported_tile;
    char *xdb_prefix;
    char *table_tile_data;
    char *xtable_tile_data;
    char *table_tiles;
    char *xtable_tiles;
    char *sql;
    int ret;
    const unsigned char *blob_odd = NULL;
    const unsigned char *blob_even = NULL;
    int blob_odd_sz = 0;
    int blob_even_sz = 0;
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
    coverage = rl2_create_coverage_from_dbms (sqlite, NULL, cvg_name);
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
    if (red_band >= cvg->nBands)
	goto error;
    if (green_band >= cvg->nBands)
	goto error;
    if (blue_band >= cvg->nBands)
	goto error;
    no_data = cvg->noData;

/* querying the tile */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    table_tile_data = sqlite3_mprintf ("%s_tile_data", cvg_name);
    xtable_tile_data = rl2_double_quoted_sql (table_tile_data);
    sqlite3_free (table_tile_data);
    table_tiles = sqlite3_mprintf ("%s_tiles", cvg_name);
    xtable_tiles = rl2_double_quoted_sql (table_tiles);
    sqlite3_free (table_tiles);
    sql = sqlite3_mprintf ("SELECT d.tile_data_odd, d.tile_data_even, "
			   "t.pyramid_level FROM \"%s\".\"%s\" AS d "
			   "JOIN \"%s\".\"%s\" AS t ON (t.tile_id = d.tile_id) "
			   "WHERE t.tile_id = ?", xdb_prefix, xtable_tile_data,
			   xdb_prefix, xtable_tiles);
    free (xdb_prefix);
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
			  (width, height, rgba, &rgb, &alpha, bg_red,
			   bg_green, bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_rgb_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity, 0))
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
						     (unsigned short *)
						     buffer, mask, no_data,
						     rgba);
		      if (!build_rgb_alpha
			  (width, height, rgba, &rgb, &alpha, bg_red,
			   bg_green, bg_blue))
			  goto error;
		      free (rgba);
		      rgba = NULL;
		      if (transparent)
			{
			    if (!get_payload_from_rgb_rgba_transparent
				(width, height, rgb, alpha,
				 RL2_OUTPUT_FORMAT_PNG, 100, &image,
				 &image_size, opacity, 0))
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
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
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
/ GetTripleBandTileImage(text db_prefix, text coverage, int tile_id, 
/                        int red_band, int green_band, int blue_band)
/ GetTripleBandTileImage(int db_prefix, text coverage, int tile_id, 
/                        int red_band, int green_band, int blue_band, 
/                        text bg_color)
/ GetTripleBandTileImage(int db_prefix, text coverage, int tile_id, 
/                        int red_band, int green_band, int blue_band, 
/                        text bg_color, int transparent)
/
/ will return a BLOB containing the Image payload
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *db_prefix = NULL;
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

    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
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
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_TEXT)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    cvg_name = (const char *) sqlite3_value_text (argv[1]);
    tile_id = sqlite3_value_int64 (argv[2]);
    red_band = sqlite3_value_int (argv[3]);
    green_band = sqlite3_value_int (argv[4]);
    blue_band = sqlite3_value_int (argv[5]);
    if (argc > 6)
	bg_color = (const char *) sqlite3_value_text (argv[6]);
    if (argc > 7)
	transparent = sqlite3_value_int (argv[7]);

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
	(context, db_prefix, cvg_name, tile_id, red_band, green_band, blue_band,
	 bg_red, bg_green, bg_blue, transparent))
	return;

  error:
    sqlite3_result_null (context);
}

static void
fnct_GetMonoBandTileImage (sqlite3_context * context, int argc,
			   sqlite3_value ** argv)
{
/* SQL function:
/ GetMonoBandTileImage(text db_prefix, text coverage, int tile_id, 
/                      int mono_band)
/ GetMonoBandTileImage(text db_prefix, text coverage, int tile_id, 
/                      int mono_band, text bg_color)
/ GetMonoBandTileImage(text db_prefix, text coverage, int tile_id, 
/                      int mono_band, text bg_color, int transparent)
/
/ will return a BLOB containing the Image payload
/ or NULL (INVALID ARGS)
/
*/
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    sqlite3_int64 tile_id;
    const char *bg_color = "#ffffff";
    int transparent = 0;
    int mono_band;
    unsigned char bg_red;
    unsigned char bg_green;
    unsigned char bg_blue;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	|| sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else
	err = 1;
    if (sqlite3_value_type (argv[1]) != SQLITE_TEXT)
	err = 1;
    if (sqlite3_value_type (argv[2]) != SQLITE_INTEGER)
	err = 1;
    if (sqlite3_value_type (argv[3]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 4 && sqlite3_value_type (argv[4]) != SQLITE_TEXT)
	err = 1;
    if (argc > 5 && sqlite3_value_type (argv[5]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving the arguments */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    cvg_name = (const char *) sqlite3_value_text (argv[1]);
    tile_id = sqlite3_value_int64 (argv[2]);
    mono_band = sqlite3_value_int (argv[3]);
    if (argc > 4)
	bg_color = (const char *) sqlite3_value_text (argv[4]);
    if (argc > 5)
	transparent = sqlite3_value_int (argv[5]);

/* coarse args validation */
    if (mono_band < 0 || mono_band > 255)
	goto error;

/* parsing the background color */
    if (rl2_parse_hexrgb (bg_color, &bg_red, &bg_green, &bg_blue) != RL2_OK)
	goto error;

    if (get_triple_band_tile_image
	(context, db_prefix, cvg_name, tile_id, mono_band, mono_band, mono_band,
	 bg_red, bg_green, bg_blue, transparent))
	return;

  error:
    sqlite3_result_null (context);
}

static void
common_export_raw_pixels (int by_section, sqlite3_context * context, int argc,
			  sqlite3_value ** argv)
{
/* common implementation Export RAW Pixels */
    int err = 0;
    const char *db_prefix = NULL;
    const char *cvg_name;
    sqlite3_int64 section_id = 0;
    int width;
    int height;
    unsigned char *xblob = NULL;
    int xblob_sz;
    const unsigned char *blob = NULL;
    int blob_sz;
    double horz_res;
    double vert_res;
    int big_endian = 0;
    rl2CoveragePtr coverage = NULL;
    sqlite3 *sqlite;
    const void *data;
    int max_threads = 1;
    int ret;
    double pt_x;
    double pt_y;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int srid;
    RL2_UNUSED ();		/* LCOV_EXCL_LINE */

    if (by_section)
      {
	  /* filtering by Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
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
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT
	      || sqlite3_value_type (argv[0]) == SQLITE_NULL)
	      ;
	  else
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
      }
    if (err)
      {
	  sqlite3_result_null (context);
	  return;
      }

/* retrieving all arguments */
    if (by_section)
      {
	  /* filtering by Section */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  section_id = sqlite3_value_int64 (argv[2]);
	  width = sqlite3_value_int (argv[3]);
	  height = sqlite3_value_int (argv[4]);
	  blob = (unsigned char *) sqlite3_value_blob (argv[5]);
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
	      big_endian = sqlite3_value_int (argv[8]);
      }
    else
      {
	  /* whole Coverage */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	      db_prefix = (const char *) sqlite3_value_text (argv[0]);
	  cvg_name = (const char *) sqlite3_value_text (argv[1]);
	  width = sqlite3_value_int (argv[2]);
	  height = sqlite3_value_int (argv[3]);
	  blob = (unsigned char *) sqlite3_value_blob (argv[4]);
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
	      big_endian = sqlite3_value_int (argv[7]);
      }

/* coarse args validation */
    if (width < 0)
	goto error;
    if (height < 0)
	goto error;
    if (big_endian)
	big_endian = 1;

    sqlite = sqlite3_context_db_handle (context);
    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    if (!by_section)
      {
	  /* excluding any Mixed Resolution Coverage */
	  if (rl2_is_mixed_resolutions_coverage (sqlite, db_prefix, cvg_name) >
	      0)
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_point (sqlite, blob, blob_sz, &pt_x, &pt_y, &srid) == RL2_OK)
      {
	  /* assumed to be the raster's Center Point */
	  double ext_x = (double) width * horz_res;
	  double ext_y = (double) height * vert_res;
	  minx = pt_x - ext_x / 2.0;
	  maxx = minx + ext_x;
	  miny = pt_y - ext_y / 2.0;
	  maxy = miny + ext_y;
      }
    else if (rl2_parse_bbox
	     (sqlite, blob, blob_sz, &minx, &miny, &maxx, &maxy) != RL2_OK)
	goto error;

/* attempting to load the Coverage definitions from the DBMS */
    coverage = rl2_create_coverage_from_dbms (sqlite, db_prefix, cvg_name);
    if (coverage == NULL)
	goto error;
    if (by_section)
      {
	  /* only a single Section */
	  ret =
	      rl2_export_section_raw_pixels_from_dbms (sqlite, max_threads,
						       coverage, section_id,
						       horz_res, vert_res,
						       minx, miny, maxx, maxy,
						       width, height,
						       big_endian, &xblob,
						       &xblob_sz);
      }
    else
      {
	  /* whole Coverage */
	  ret =
	      rl2_export_raw_pixels_from_dbms (sqlite, max_threads, coverage,
					       horz_res, vert_res, minx, miny,
					       maxx, maxy, width, height,
					       big_endian, &xblob, &xblob_sz);
      }
    if (ret != RL2_OK)
	goto error;
    rl2_destroy_coverage (coverage);
    sqlite3_result_blob (context, xblob, xblob_sz, free);
    return;

  error:
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    sqlite3_result_null (context);
}

static void
fnct_ExportRawPixels (sqlite3_context * context, int argc,
		      sqlite3_value ** argv)
{
/* SQL function:
/ ExportRawPixels(text db_prefix, text coverage, int width, int height, 
/                 BLOB geom, double resolution)
/ ExportRawPixels(text db_prefix, text coverage, int width, int height, 
/                 BLOB geom, double horz_res, double vert_res)
/ ExportRawPixels(text db_prefix, text coverage, int width, int height, 
/                 BLOB geom, double horz_res, double vert_res, 
/                 int big_endian)
/
/ will return a BLOB pixel buffer
/ or NULL (INVALID ARGS)
/
*/
    common_export_raw_pixels (0, context, argc, argv);
}

static void
fnct_ExportSectionRawPixels (sqlite3_context * context, int argc,
			     sqlite3_value ** argv)
{
/* SQL function:
/ ExportSectionRawPixels(text db_prefix, text coverage, int section_id, 
/                        int width, int height, BLOB geom, double resolution)
/ ExportSectionRawPixels(text db_prefix, text coverage, int section_id, 
/                        int width, int height, BLOB geom, double horz_res,
/                        double vert_res)
/ ExportSectionRawPixels(text db_prefix, text coverage, int section_id, 
/                        int width, int height, BLOB geom, double horz_res,
/                        double vert_res, int big_endian)
/
/ will return a BLOB pixel buffer
/ or NULL (INVALID ARGS)
/
*/
    common_export_raw_pixels (1, context, argc, argv);
}

static void
fnct_ImportSectionRawPixels (sqlite3_context * context, int argc,
			     sqlite3_value ** argv)
{
/* SQL function:
/ ImportSectionRawPixels(text coverage, text section_name, int width,
/                        int height, BLOB pixels, BLOB geom)
/ ImportSectionRawPixels(text coverage, text section_name, int width,
/                        int height, BLOB pixels, BLOB geom,
/                        int pyramidize)
/ ImportSectionRawPixels(text coverage, text section_name, int width,
/                        int height, BLOB pixels, BLOB geom,
/                        int pyramidize, int transaction)
/ ImportSectionRawPixels(text coverage, text section_name, int width,
/                        int height, BLOB pixels, BLOB geom,
/                        int pyramidize, int transaction,
/                        int big_endian)
/
/ will return 1 (TRUE, success) or 0 (FALSE, failure)
/ or -1 (INVALID ARGS)
/
*/
    int err = 0;
    const char *cvg_name;
    const char *sctn_name;
    int width;
    int height;
    const unsigned char *pixels;
    int pixels_sz;
    const unsigned char *blob;
    int blob_sz;
    int transaction = 1;
    int pyramidize = 1;
    int big_endian = 0;
    int srid;
    int cov_srid;
    double minx;
    double maxx;
    double miny;
    double maxy;
    int sample_sz = 0;
    int expected_sz;
    int errcode = -1;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char *bufpix = NULL;
    rl2CoveragePtr coverage = NULL;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr plt = NULL;
    rl2PixelPtr no_data = NULL;
    sqlite3 *sqlite;
    int ret;
    int max_threads = 1;
    const char *data;
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
    if (sqlite3_value_type (argv[5]) != SQLITE_BLOB)
	err = 1;
    if (argc > 6 && sqlite3_value_type (argv[6]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 7 && sqlite3_value_type (argv[7]) != SQLITE_INTEGER)
	err = 1;
    if (argc > 8 && sqlite3_value_type (argv[8]) != SQLITE_INTEGER)
	err = 1;
    if (err)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* retrieving the arguments */
    cvg_name = (const char *) sqlite3_value_text (argv[0]);
    sctn_name = (const char *) sqlite3_value_text (argv[1]);
    width = sqlite3_value_int (argv[2]);
    height = sqlite3_value_int (argv[3]);
    pixels = sqlite3_value_blob (argv[4]);
    pixels_sz = sqlite3_value_bytes (argv[4]);
    blob = sqlite3_value_blob (argv[5]);
    blob_sz = sqlite3_value_bytes (argv[5]);
    if (argc > 6)
	pyramidize = sqlite3_value_int (argv[6]);
    if (argc > 7)
	transaction = sqlite3_value_int (argv[7]);
    if (argc > 8)
	big_endian = sqlite3_value_int (argv[8]);

/* attempting to load the Coverage definitions from the DBMS */
    sqlite = sqlite3_context_db_handle (context);

    data = sqlite3_user_data (context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }
    coverage = rl2_create_coverage_from_dbms (sqlite, NULL, cvg_name);
    if (coverage == NULL)
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

/* coarse args validation */
    if (width < 0)
      {
	  errcode = -1;
	  goto error;
      }
    if (height < 0)
      {
	  errcode = -1;
	  goto error;
      }

/* checking the Geometry */
    if (rl2_parse_bbox_srid
	(sqlite, blob, blob_sz, &srid, &minx, &miny, &maxx, &maxy) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

    if (rl2_get_coverage_type
	(coverage, &sample_type, &pixel_type, &num_bands) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }
    if (rl2_get_coverage_srid (coverage, &cov_srid) != RL2_OK)
      {
	  errcode = -1;
	  goto error;
      }

    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
	  sample_sz = 1;
	  break;
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
      };
    expected_sz = width * height * num_bands * sample_sz;
    if (expected_sz != pixels_sz)
      {
	  fprintf (stderr,
		   "RL2_ImportSectionRawPixels: mismatching BLOB size (expected %u)\n",
		   expected_sz);
	  goto error;
      }
    if (srid != cov_srid)
      {
	  fprintf (stderr,
		   "RL2_ImportSectionRawPixels: mismatching SRID (expected %d)\n",
		   cov_srid);
	  goto error;
      }
    plt = rl2_get_dbms_palette (sqlite, NULL, cvg_name);
    no_data = rl2_clone_pixel (rl2_get_coverage_no_data (coverage));
    bufpix =
	rl2_copy_endian_raw_pixels (pixels, pixels_sz, width, height,
				    sample_type, num_bands, big_endian);
    if (bufpix == NULL)
	goto error;
    raster =
	rl2_create_raster (width, height, sample_type, pixel_type, num_bands,
			   bufpix, pixels_sz, plt, NULL, 0, no_data);
/* releasing ownership */
    bufpix = NULL;
    plt = NULL;
    no_data = NULL;
    if (raster == NULL)
      {
	  errcode = -1;
	  goto error;
      }
/* georeferencing the raster */
    if (rl2_raster_georeference_frame (raster, srid, minx, miny, maxx, maxy)
	!= RL2_OK)
      {
	  errcode = -1;
	  goto error;
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
    ret =
	rl2_load_raw_raster_into_dbms (sqlite, max_threads, coverage,
				       sctn_name, raster, pyramidize);
    rl2_destroy_coverage (coverage);
    rl2_destroy_raster (raster);
    if (ret != RL2_OK)
      {
	  if (transaction)
	    {
		/* invalidating the pending transaction */
		sqlite3_exec (sqlite, "ROLLBACK", NULL, NULL, NULL);
	    }
	  sqlite3_result_int (context, 0);
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
    return;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (coverage != NULL)
	rl2_destroy_coverage (coverage);
    if (plt != NULL)
	rl2_destroy_palette (plt);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (bufpix != NULL)
	free (bufpix);
    sqlite3_result_int (context, errcode);
}

static void
register_rl2_sql_functions (void *p_db, const void *p_data)
{
    sqlite3 *db = p_db;
    struct rl2_private_data *priv_data = (struct rl2_private_data *) p_data;
    const char *security_level;
    sqlite3_create_function (db, "rl2_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_version, 0, 0);
    sqlite3_create_function (db, "rl2_cairo_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_cairo_version, 0, 0);
    sqlite3_create_function (db, "rl2_curl_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_curl_version, 0, 0);
    sqlite3_create_function (db, "rl2_zlib_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_zlib_version, 0, 0);
    sqlite3_create_function (db, "rl2_lzma_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_lzma_version, 0, 0);
    sqlite3_create_function (db, "rl2_lz4_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_lz4_version, 0, 0);
    sqlite3_create_function (db, "rl2_zstd_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_zstd_version, 0, 0);
    sqlite3_create_function (db, "rl2_png_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_png_version, 0, 0);
    sqlite3_create_function (db, "rl2_jpeg_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_jpeg_version, 0, 0);
    sqlite3_create_function (db, "rl2_tiff_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_tiff_version, 0, 0);
    sqlite3_create_function (db, "rl2_geotiff_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_geotiff_version, 0, 0);
    sqlite3_create_function (db, "rl2_webp_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_webp_version, 0, 0);
    sqlite3_create_function (db, "rl2_charLS_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_charLS_version, 0, 0);
    sqlite3_create_function (db, "rl2_openJpeg_version", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_openJPEG_version, 0, 0);
    sqlite3_create_function (db, "rl2_target_cpu", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_target_cpu, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_none", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_none, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_deflate", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_deflate, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_deflate_no", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_deflate_no, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_lzma", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_lzma, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_lzma_no", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_lzma_no, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_lz4", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_lz4, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_lz4_no", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_lz4_no, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_zstd", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_zstd, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_zstd_no", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_zstd_no, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_jpeg", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_jpeg, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_png", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_png, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_fax4", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_fax4, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_charls", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_charls, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_webp", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_webp, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_ll_webp", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_ll_webp, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_jp2", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_jp2, 0, 0);
    sqlite3_create_function (db, "rl2_has_codec_ll_jp2", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_rl2_has_codec_ll_jp2, 0, 0);
    sqlite3_create_function (db, "RL2_GetMaxThreads", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMaxThreads, 0, 0);
    sqlite3_create_function (db, "RL2_SetMaxThreads", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_SetMaxThreads, 0, 0);
    sqlite3_create_function (db, "IsValidPixel", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidPixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidPixel", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidPixel, 0, 0);
    sqlite3_create_function (db, "IsPixelNone", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsPixelNone, 0, 0);
    sqlite3_create_function (db, "RL2_IsPixelNone", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsPixelNone, 0, 0);
    sqlite3_create_function (db, "IsValidRasterPalette", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterPalette", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidRasterPalette, 0, 0);
    sqlite3_create_function (db, "IsValidRasterStatistics", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterStatistics", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidRasterStatistics, 0, 0);
    sqlite3_create_function (db, "IsValidRasterTile", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidRasterTile, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidRasterTile", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidRasterTile, 0, 0);
    sqlite3_create_function (db, "CreateRasterCoverage", 10,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "CreateRasterCoverage", 11,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "CreateRasterCoverage", 12,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "CreateRasterCoverage", 17,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "CreateRasterCoverage", 18,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateRasterCoverage", 10,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateRasterCoverage", 11,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateRasterCoverage", 12,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateRasterCoverage", 17,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CreateRasterCoverage", 18,
			     SQLITE_UTF8, 0, fnct_CreateRasterCoverage, 0, 0);
    sqlite3_create_function (db, "CopyRasterCoverage", 2,
			     SQLITE_UTF8, 0, fnct_CopyRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CopyRasterCoverage", 2,
			     SQLITE_UTF8, 0, fnct_CopyRasterCoverage, 0, 0);
    sqlite3_create_function (db, "CopyRasterCoverage", 3,
			     SQLITE_UTF8, 0, fnct_CopyRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_CopyRasterCoverage", 3,
			     SQLITE_UTF8, 0, fnct_CopyRasterCoverage, 0, 0);
    sqlite3_create_function (db, "DeleteSection", 2,
			     SQLITE_UTF8, 0, fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "RL2_DeleteSection", 2,
			     SQLITE_UTF8, 0, fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "DeleteSection", 3,
			     SQLITE_UTF8, 0, fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "RL2_DeleteSection", 3,
			     SQLITE_UTF8, 0, fnct_DeleteSection, 0, 0);
    sqlite3_create_function (db, "DropRasterCoverage", 1,
			     SQLITE_UTF8, 0, fnct_DropRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_DropRasterCoverage", 1,
			     SQLITE_UTF8, 0, fnct_DropRasterCoverage, 0, 0);
    sqlite3_create_function (db, "DropRasterCoverage", 2,
			     SQLITE_UTF8, 0, fnct_DropRasterCoverage, 0, 0);
    sqlite3_create_function (db, "RL2_DropRasterCoverage", 2,
			     SQLITE_UTF8, 0, fnct_DropRasterCoverage, 0, 0);
    sqlite3_create_function (db, "SetRasterCoverageInfos", 3,
			     SQLITE_UTF8, 0, fnct_SetRasterCoverageInfos, 0, 0);
    sqlite3_create_function (db, "SetRasterCoverageInfos", 4,
			     SQLITE_UTF8, 0, fnct_SetRasterCoverageInfos, 0, 0);
    sqlite3_create_function (db, "RL2_SetRasterCoverageInfos", 3, SQLITE_UTF8,
			     0, fnct_SetRasterCoverageInfos, 0, 0);
    sqlite3_create_function (db, "RL2_SetRasterCoverageInfos", 4, SQLITE_UTF8,
			     0, fnct_SetRasterCoverageInfos, 0, 0);
    sqlite3_create_function (db, "SetRasterCoverageCopyright", 2, SQLITE_ANY,
			     0, fnct_SetRasterCoverageCopyright, 0, 0);
    sqlite3_create_function (db, "SetRasterCoverageCopyright", 3, SQLITE_ANY,
			     0, fnct_SetRasterCoverageCopyright, 0, 0);
    sqlite3_create_function (db, "RL2_SetRasterCoverageCopyright", 2,
			     SQLITE_ANY, 0, fnct_SetRasterCoverageCopyright, 0,
			     0);
    sqlite3_create_function (db, "RL2_SetRasterCoverageCopyright", 3,
			     SQLITE_ANY, 0, fnct_SetRasterCoverageCopyright, 0,
			     0);
    sqlite3_create_function (db, "SetRasterCoverageDefaultBands", 5,
			     SQLITE_UTF8, 0, fnct_SetRasterCoverageDefaultBands,
			     0, 0);
    sqlite3_create_function (db, "RL2_SetRasterCoverageDefaultBands", 5,
			     SQLITE_UTF8, 0, fnct_SetRasterCoverageDefaultBands,
			     0, 0);
    sqlite3_create_function (db, "EnableRasterCoverageAutoNDVI", 2, SQLITE_UTF8,
			     0, fnct_EnableRasterCoverageAutoNDVI, 0, 0);
    sqlite3_create_function (db, "RL2_EnableRasterCoverageAutoNDVI", 2,
			     SQLITE_UTF8, 0, fnct_EnableRasterCoverageAutoNDVI,
			     0, 0);
    sqlite3_create_function (db, "IsRasterCoverageAutoNdviEnabled", 2,
			     SQLITE_UTF8, 0,
			     fnct_IsRasterCoverageAutoNdviEnabled, 0, 0);
    sqlite3_create_function (db, "RL2_IsRasterCoverageAutoNdviEnabled", 2,
			     SQLITE_UTF8, 0,
			     fnct_IsRasterCoverageAutoNdviEnabled, 0, 0);
    sqlite3_create_function (db, "GetPaletteNumEntries", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPaletteNumEntries, 0, 0);
    sqlite3_create_function (db, "RL2_GetPaletteNumEntries", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPaletteNumEntries, 0, 0);
    sqlite3_create_function (db, "GetPaletteColorEntry", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPaletteColorEntry, 0, 0);
    sqlite3_create_function (db, "RL2_GetPaletteColorEntry", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPaletteColorEntry, 0, 0);
    sqlite3_create_function (db, "SetPaletteColorEntry", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_SetPaletteColorEntry, 0, 0);
    sqlite3_create_function (db, "RL2_SetPaletteColorEntry", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_SetPaletteColorEntry, 0, 0);
    sqlite3_create_function (db, "PaletteEquals", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_PaletteEquals, 0, 0);
    sqlite3_create_function (db, "RL2_PaletteEquals", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_PaletteEquals, 0, 0);
    sqlite3_create_function (db, "CreatePixel", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_CreatePixel, 0, 0);
    sqlite3_create_function (db, "RL2_CreatePixel", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_CreatePixel, 0, 0);
    sqlite3_create_function (db, "CreatePixelNone", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_CreatePixelNone, 0, 0);
    sqlite3_create_function (db, "RL2_CreatePixelNone", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_CreatePixelNone, 0, 0);
    sqlite3_create_function (db, "GetPixelType", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelType", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPixelType, 0, 0);
    sqlite3_create_function (db, "GetPixelSampleType", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPixelSampleType, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelSampleType", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPixelSampleType, 0, 0);
    sqlite3_create_function (db, "GetPixelNumBands", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPixelNumBands, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelNumBands", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPixelNumBands, 0, 0);
    sqlite3_create_function (db, "GetPixelValue", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPixelValue, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelValue", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetPixelValue, 0, 0);
    sqlite3_create_function (db, "SetPixelValue", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_SetPixelValue, 0, 0);
    sqlite3_create_function (db, "RL2_SetPixelValue", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_SetPixelValue, 0, 0);
    sqlite3_create_function (db, "IsTransparentPixel", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsTransparentPixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsTransparentPixel", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsTransparentPixel, 0, 0);
    sqlite3_create_function (db, "IsOpaquePixel", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsOpaquePixel, 0, 0);
    sqlite3_create_function (db, "RL2_IsOpaquePixel", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsOpaquePixel, 0, 0);
    sqlite3_create_function (db, "SetTransparentPixel", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_SetTransparentPixel, 0, 0);
    sqlite3_create_function (db, "RL2_SetTransparentPixel", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_SetTransparentPixel, 0, 0);
    sqlite3_create_function (db, "SetOpaquePixel", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_SetOpaquePixel, 0, 0);
    sqlite3_create_function (db, "RL2_SetOpaquePixel", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_SetOpaquePixel, 0, 0);
    sqlite3_create_function (db, "PixelEquals", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_PixelEquals, 0, 0);
    sqlite3_create_function (db, "RL2_PixelEquals", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_PixelEquals, 0, 0);
    sqlite3_create_function (db, "IsValidFont", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidFont, 0, 0);
    sqlite3_create_function (db, "RL2_IsValidFont", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsValidFont, 0, 0);
    sqlite3_create_function (db, "CheckFontFacename", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_CheckFontFacename, 0, 0);
    sqlite3_create_function (db, "RL2_CheckFontFacename", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_CheckFontFacename, 0, 0);
    sqlite3_create_function (db, "GetFontFamily", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetFontFamily, 0, 0);
    sqlite3_create_function (db, "RL2_GetFontFamily", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetFontFamily, 0, 0);
    sqlite3_create_function (db, "GetFontFacename", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetFontFacename, 0, 0);
    sqlite3_create_function (db, "RL2_GetFontFacename", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetFontFacename, 0, 0);
    sqlite3_create_function (db, "IsFontBold", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsFontBold, 0, 0);
    sqlite3_create_function (db, "RL2_IsFontBold", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsFontBold, 0, 0);
    sqlite3_create_function (db, "IsFontItalic", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsFontItalic, 0, 0);
    sqlite3_create_function (db, "RL2_IsFontItalic", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_IsFontItalic, 0, 0);
    sqlite3_create_function (db, "GetRasterStatistics_NoDataPixelsCount", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetRasterStatistics_NoDataPixelsCount, 0, 0);
    sqlite3_create_function (db, "RL2_GetRasterStatistics_NoDataPixelsCount", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetRasterStatistics_NoDataPixelsCount, 0, 0);
    sqlite3_create_function (db, "GetRasterStatistics_ValidPixelsCount", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetRasterStatistics_ValidPixelsCount, 0, 0);
    sqlite3_create_function (db, "RL2_GetRasterStatistics_ValidPixelsCount", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetRasterStatistics_ValidPixelsCount, 0, 0);
    sqlite3_create_function (db, "GetRasterStatistics_SampleType", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetRasterStatistics_SampleType, 0, 0);
    sqlite3_create_function (db, "RL2_GetRasterStatistics_SampleType", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetRasterStatistics_SampleType, 0, 0);
    sqlite3_create_function (db, "GetRasterStatistics_BandsCount", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetRasterStatistics_BandsCount, 0, 0);
    sqlite3_create_function (db, "RL2_GetRasterStatistics_BandsCount", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetRasterStatistics_BandsCount, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Min", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Min, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Min", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Min, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Max", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Max, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Max", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Max, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Avg", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Avg, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Avg", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Avg, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Var", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Var, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Var", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Var, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_StdDev", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_StdDev, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_StdDev", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_StdDev, 0, 0);
    sqlite3_create_function (db, "GetBandStatistics_Histogram", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Histogram, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandStatistics_Histogram", 2,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandStatistics_Histogram, 0, 0);
    sqlite3_create_function (db, "GetBandHistogramFromImage", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandHistogramFromImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetBandHistogramFromImage", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0,
			     fnct_GetBandHistogramFromImage, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 1,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 1, SQLITE_UTF8, priv_data,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 2, SQLITE_UTF8, priv_data,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 2, SQLITE_UTF8, priv_data,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 3, SQLITE_UTF8, priv_data,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 3, SQLITE_UTF8, priv_data,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "Pyramidize", 4, SQLITE_UTF8, priv_data,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_Pyramidize", 4, SQLITE_UTF8, priv_data,
			     fnct_Pyramidize, 0, 0);
    sqlite3_create_function (db, "PyramidizeMonolithic", 1, SQLITE_UTF8, 0,
			     fnct_PyramidizeMonolithic, 0, 0);
    sqlite3_create_function (db, "RL2_PyramidizeMonolithic", 1, SQLITE_UTF8, 0,
			     fnct_PyramidizeMonolithic, 0, 0);
    sqlite3_create_function (db, "PyramidizeMonolithic", 2, SQLITE_UTF8, 0,
			     fnct_PyramidizeMonolithic, 0, 0);
    sqlite3_create_function (db, "RL2_PyramidizeMonolithic", 2, SQLITE_UTF8, 0,
			     fnct_PyramidizeMonolithic, 0, 0);
    sqlite3_create_function (db, "PyramidizeMonolithic", 3, SQLITE_UTF8, 0,
			     fnct_PyramidizeMonolithic, 0, 0);
    sqlite3_create_function (db, "RL2_PyramidizeMonolithic", 3, SQLITE_UTF8, 0,
			     fnct_PyramidizeMonolithic, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 1, SQLITE_UTF8, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 1, SQLITE_UTF8, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 2, SQLITE_UTF8, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 2, SQLITE_UTF8, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "DePyramidize", 3, SQLITE_UTF8, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "RL2_DePyramidize", 3, SQLITE_UTF8, 0,
			     fnct_DePyramidize, 0, 0);
    sqlite3_create_function (db, "GetPixelFromRasterByPoint", 4,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetPixelFromRasterByPoint, 0, 0);
    sqlite3_create_function (db, "GetPixelFromRasterByPoint", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetPixelFromRasterByPoint, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelFromRasterByPoint", 4,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetPixelFromRasterByPoint, 0, 0);
    sqlite3_create_function (db, "RL2_GetPixelFromRasterByPoint", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetPixelFromRasterByPoint, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromRaster", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromRaster", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromRaster", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromRaster", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromRaster", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromRaster", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromRaster", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromRaster", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromRaster", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromRaster", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromRaster", 10,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromRaster", 10,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromRaster", 11,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromRaster", 11,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromRaster", 12,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromRaster", 12,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromRaster, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromVector", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromVector", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromVector", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromVector", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromVector", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromVector", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromVector", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromVector", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromVector", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromVector", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromVector", 10,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromVector", 10,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromVector", 11,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromVector", 11,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromVector", 12,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromVector", 12,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromVector, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromWMS", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromWMS, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromWMS", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromWMS, 0, 0);
    sqlite3_create_function (db, "GetMapImageFromWMS", 10,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromWMS, 0, 0);
    sqlite3_create_function (db, "RL2_GetMapImageFromWMS", 10,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMapImageFromWMS, 0, 0);
    sqlite3_create_function (db, "GetTileImage", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTileImage", 3,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetTileImage, 0, 0);
    sqlite3_create_function (db, "GetTripleBandTileImage", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTripleBandTileImage", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetTripleBandTileImage", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTripleBandTileImage", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetTripleBandTileImage", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetTripleBandTileImage", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetTripleBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetMonoBandTileImage", 4,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMonoBandTileImage", 4,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetMonoBandTileImage", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMonoBandTileImage", 5,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "GetMonoBandTileImage", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "RL2_GetMonoBandTileImage", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetMonoBandTileImage, 0, 0);
    sqlite3_create_function (db, "ExportRawPixels", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ExportRawPixels", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportRawPixels, 0, 0);
    sqlite3_create_function (db, "ExportRawPixels", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ExportRawPixels", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportRawPixels, 0, 0);
    sqlite3_create_function (db, "ExportRawPixels", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ExportRawPixels", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportRawPixels, 0, 0);
    sqlite3_create_function (db, "ExportSectionRawPixels", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ExportSectionRawPixels", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "ExportSectionRawPixels", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ExportSectionRawPixels", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "ExportRawPixels", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ExportSectionRawPixels", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ExportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "ImportSectionRawPixels", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ImportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ImportSectionRawPixels", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ImportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "ImportSectionRawPixels", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ImportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ImportSectionRawPixels", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ImportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "ImportSectionRawPixels", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ImportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ImportSectionRawPixels", 8,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ImportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "ImportSectionRawPixels", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ImportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "RL2_ImportSectionRawPixels", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_ImportSectionRawPixels, 0, 0);
    sqlite3_create_function (db, "GetDrapingLastError", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetDrapingLastError, 0, 0);
    sqlite3_create_function (db, "RL2_GetDrapingLastError", 0,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_GetDrapingLastError, 0, 0);
    sqlite3_create_function (db, "DrapeGeometries", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_DrapeGeometries, 0, 0);
    sqlite3_create_function (db, "RL2_DrapeGeometries", 6,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_DrapeGeometries, 0, 0);
    sqlite3_create_function (db, "DrapeGeometries", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_DrapeGeometries, 0, 0);
    sqlite3_create_function (db, "RL2_DrapeGeometries", 7,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_DrapeGeometries, 0, 0);
    sqlite3_create_function (db, "DrapeGeometries", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_DrapeGeometries, 0, 0);
    sqlite3_create_function (db, "RL2_DrapeGeometries", 9,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_DrapeGeometries, 0, 0);
    sqlite3_create_function (db, "DrapeGeometries", 10,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_DrapeGeometries, 0, 0);
    sqlite3_create_function (db, "RL2_DrapeGeometries", 10,
			     SQLITE_UTF8 | SQLITE_DETERMINISTIC, priv_data,
			     fnct_DrapeGeometries, 0, 0);

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
	  sqlite3_create_function (db, "LoadFontFromFile", 1, SQLITE_UTF8, 0,
				   fnct_LoadFontFromFile, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadFontFromFile", 1, SQLITE_UTF8,
				   0, fnct_LoadFontFromFile, 0, 0);
	  sqlite3_create_function (db, "ExportFontToFile", 3, SQLITE_UTF8, 0,
				   fnct_ExportFontToFile, 0, 0);
	  sqlite3_create_function (db, "RL2_ExportFontToFile", 3, SQLITE_UTF8,
				   0, fnct_ExportFontToFile, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 2, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 3, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 4, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 5, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRaster", 6, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 2, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 3, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 4, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 5, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRaster", 6, SQLITE_UTF8,
				   priv_data, fnct_LoadRaster, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 2, SQLITE_UTF8,
				   priv_data, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 3, SQLITE_UTF8,
				   priv_data, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 4, SQLITE_UTF8,
				   priv_data, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 5, SQLITE_UTF8,
				   priv_data, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 6, SQLITE_UTF8,
				   priv_data, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRastersFromDir", 7, SQLITE_UTF8,
				   priv_data, fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 2,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 3,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 4,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 5,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 6,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRastersFromDir", 7,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRastersFromDir, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 9, SQLITE_UTF8,
				   priv_data, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 10, SQLITE_UTF8,
				   priv_data, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 11, SQLITE_UTF8,
				   priv_data, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 12, SQLITE_UTF8,
				   priv_data, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 13, SQLITE_UTF8,
				   priv_data, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "LoadRasterFromWMS", 14, SQLITE_UTF8,
				   priv_data, fnct_LoadRasterFromWMS, 0, 0);
	  sqlite3_create_function (db, "RL2_LoadRasterFromWMS", 14,
				   SQLITE_UTF8, 0, fnct_LoadRasterFromWMS, 0,
				   0);
	  sqlite3_create_function (db, "WriteGeoTiff", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteGeoTiff", 11, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteGeoTiff", 11, SQLITE_UTF8,
				   priv_data, fnct_WriteGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiffTfw", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiffTfw", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 7, SQLITE_UTF8, priv_data,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 8, SQLITE_UTF8, priv_data,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 9, SQLITE_UTF8, priv_data,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTiff", 10, SQLITE_UTF8, priv_data,
				   fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTiff", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionGeoTiff", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionGeoTiff", 8,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionGeoTiff", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionGeoTiff", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionGeoTiff", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionGeoTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionGeoTiff", 11, SQLITE_UTF8,
				   0, fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionGeoTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionGeoTiff", 12, SQLITE_UTF8,
				   0, fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionGeoTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTiffTfw", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTiffTfw", 8,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTiffTfw", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTiffTfw", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTiffTfw", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTiffTfw", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTiffTfw", 11, SQLITE_UTF8,
				   priv_data, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTiffTfw", 11,
				   SQLITE_UTF8, 0, fnct_WriteTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTiff", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTiff", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTiff", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTiff", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTiff", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTiff", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTiff", 11, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTiff, 0, 0);
	  sqlite3_create_function (db, "WriteJpegJgw", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpegJgw", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteJpegJgw", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpegJgw", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteJpegJgw", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpegJgw", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteJpeg", 7, SQLITE_UTF8, priv_data,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpeg", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteJpeg", 8, SQLITE_UTF8, priv_data,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpeg", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteJpeg", 9, SQLITE_UTF8, priv_data,
				   fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteJpeg", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteSectionJpegJgw", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionJpegJgw", 8,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionJpegJgw", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionJpegJgw", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionJpegJgw", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpegJgw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionJpegJgw", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionJpegJgw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionJpeg", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionJpeg", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteSectionJpeg", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionJpeg", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteSectionJpeg", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpeg, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionJpeg", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteSectionJpeg, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandGeoTiff", 14,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandGeoTiff", 14,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandGeoTiff", 11,
				   SQLITE_UTF8, 0,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandGeoTiff",
				   11, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandGeoTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandGeoTiff",
				   12, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandGeoTiff", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandGeoTiff",
				   13, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandGeoTiff", 14,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandGeoTiff",
				   14, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandGeoTiff", 15,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandGeoTiff",
				   15, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 8, SQLITE_UTF8,
				   0, fnct_WriteMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 8,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 10, SQLITE_UTF8,
				   0, fnct_WriteMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 11,
				   SQLITE_UTF8, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandGeoTiff", 12,
				   SQLITE_UTF8, 0, fnct_WriteMonoBandGeoTiff,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandGeoTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandGeoTiff", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandGeoTiff", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandGeoTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandGeoTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandGeoTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandGeoTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandGeoTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandGeoTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandGeoTiff", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandGeoTiff", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandGeoTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiffTfw", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiffTfw", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiffTfw", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiffTfw", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiffTfw", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiffTfw", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiffTfw", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiffTfw", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandTiffTfw", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandTiffTfw",
				   11, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandTiffTfw", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandTiffTfw",
				   12, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandTiffTfw", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandTiffTfw",
				   13, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandTiffTfw", 14,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandTiffTfw",
				   14, SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandTiffTfw", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandTiffTfw", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandTiffTfw", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandTiffTfw", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandTiffTfw", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandTiffTfw", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandTiffTfw", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandTiffTfw", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiffTfw", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiffTfw", 8,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiffTfw", 9, SQLITE_UTF8,
				   0, fnct_WriteMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiffTfw", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiffTfw", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiffTfw", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiffTfw", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiffTfw", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiffTfw, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiff", 10, SQLITE_UTF8,
				   0, fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiff", 11, SQLITE_UTF8,
				   0, fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiff", 12, SQLITE_UTF8,
				   0, fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteTripleBandTiff", 13, SQLITE_UTF8,
				   0, fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteTripleBandTiff", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandTiff", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandTiff", 13,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionTripleBandTiff", 14,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionTripleBandTiff", 14,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionTripleBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiff", 8, SQLITE_UTF8, 0,
				   fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiff", 8,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiff", 9, SQLITE_UTF8, 0,
				   fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiff", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiff", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteMonoBandTiff", 11, SQLITE_UTF8,
				   priv_data, fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteMonoBandTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandTiff", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandTiff", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandTiff", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandTiff", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteSectionMonoBandTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionMonoBandTiff", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionMonoBandTiff, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 7, SQLITE_UTF8,
				   priv_data, fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 8, SQLITE_UTF8,
				   priv_data, fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteAsciiGrid", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteAsciiGrid", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteSectionAsciiGrid", 8,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionAsciiGrid", 8,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteSectionAsciiGrid", 9,
				   SQLITE_UTF8, 0, fnct_WriteSectionAsciiGrid,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionAsciiGrid", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteSectionAsciiGrid", 10,
				   SQLITE_UTF8, 0, fnct_WriteSectionAsciiGrid,
				   0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionAsciiGrid", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteNdviAsciiGrid", 9, SQLITE_UTF8,
				   priv_data, fnct_WriteNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteNdviAsciiGrid", 9,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteNdviAsciiGrid", 10, SQLITE_UTF8,
				   priv_data, fnct_WriteNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteNdviAsciiGrid", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteNdviAsciiGrid", 11, SQLITE_UTF8,
				   priv_data, fnct_WriteNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteNdviAsciiGrid", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteSectionNdviAsciiGrid", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionNdviAsciiGrid", 10,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteSectionNdviAsciiGrid", 11,
				   SQLITE_UTF8, 0,
				   fnct_WriteSectionNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionNdviAsciiGrid", 11,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "WriteSectionNdviAsciiGrid", 12,
				   SQLITE_UTF8, 0,
				   fnct_WriteSectionNdviAsciiGrid, 0, 0);
	  sqlite3_create_function (db, "RL2_WriteSectionNdviAsciiGrid", 12,
				   SQLITE_UTF8, priv_data,
				   fnct_WriteSectionNdviAsciiGrid, 0, 0);
      }
}

#ifndef LOADABLE_EXTENSION

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

RL2_DECLARE void
rl2_init (sqlite3 * handle, const void *priv_data, int verbose)
{
/* used when SQLite initializes as an ordinary lib */
    register_rl2_sql_functions (handle, priv_data);
    rl2_splash_screen (verbose);
}

#else /* built as LOADABLE EXTENSION only */

SQLITE_EXTENSION_INIT1 static int
init_rl2_extension (sqlite3 * db, char **pzErrMsg,
		    const sqlite3_api_routines * pApi)
{
    void *priv_data = rl2_alloc_private ();
    SQLITE_EXTENSION_INIT2 (pApi);

    register_rl2_sql_functions (db, priv_data);
    *pzErrMsg = NULL;
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
