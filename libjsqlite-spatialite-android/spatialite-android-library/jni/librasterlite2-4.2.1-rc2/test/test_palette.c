/*

 test_palette.c -- RasterLite2 Test Case

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

The Original Code is the SpatiaLite library

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"

rl2PalettePtr
create_monochrome_palette ()
{
/* creating a MONOCHROME palette */
    rl2PalettePtr palette = rl2_create_palette (2);
    rl2_set_palette_color (palette, 0, 255, 255, 255);
    rl2_set_palette_color (palette, 1, 0, 0, 0);
    return palette;
}

rl2PalettePtr
create_gray4_palette ()
{
/* creating a GRAY-4 palette */
    rl2PalettePtr palette = rl2_create_palette (4);
    rl2_set_palette_color (palette, 0, 0, 0, 0);
    rl2_set_palette_color (palette, 1, 86, 86, 86);
    rl2_set_palette_color (palette, 2, 170, 170, 170);
    rl2_set_palette_color (palette, 3, 255, 255, 255);
    return palette;
}

rl2PalettePtr
create_gray16_palette ()
{
/* creating a GRAY-16 palette */
    rl2PalettePtr palette = rl2_create_palette (16);
    rl2_set_palette_color (palette, 0, 0, 0, 0);
    rl2_set_palette_color (palette, 1, 17, 17, 17);
    rl2_set_palette_color (palette, 2, 34, 34, 34);
    rl2_set_palette_color (palette, 3, 51, 51, 51);
    rl2_set_palette_color (palette, 4, 68, 68, 68);
    rl2_set_palette_color (palette, 5, 85, 85, 85);
    rl2_set_palette_color (palette, 6, 102, 102, 102);
    rl2_set_palette_color (palette, 7, 119, 119, 119);
    rl2_set_palette_color (palette, 8, 137, 137, 137);
    rl2_set_palette_color (palette, 9, 154, 154, 154);
    rl2_set_palette_color (palette, 10, 171, 171, 171);
    rl2_set_palette_color (palette, 11, 188, 188, 188);
    rl2_set_palette_color (palette, 12, 205, 205, 205);
    rl2_set_palette_color (palette, 13, 222, 222, 222);
    rl2_set_palette_color (palette, 14, 239, 239, 239);
    rl2_set_palette_color (palette, 15, 255, 255, 255);
    return palette;
}

rl2PalettePtr
create_gray256_palette ()
{
/* creating a GRAY-256 palette */
    int i;
    rl2PalettePtr palette = rl2_create_palette (256);
    for (i = 0; i < 256; i++)
	rl2_set_palette_color (palette, i, i, i, i);
    return palette;
}

rl2PalettePtr
create_bicolor_palette ()
{
/* creating a BICOLOR palette */
    rl2PalettePtr palette = rl2_create_palette (2);
    rl2_set_palette_color (palette, 0, 255, 0, 255);
    rl2_set_palette_color (palette, 1, 255, 0, 255);
    return palette;
}

rl2PalettePtr
create_rgb4_palette ()
{
/* creating an RGB-4 palette */
    rl2PalettePtr palette = rl2_create_palette (4);
    rl2_set_palette_color (palette, 0, 255, 0, 0);
    rl2_set_palette_color (palette, 1, 255, 86, 86);
    rl2_set_palette_color (palette, 2, 255, 170, 170);
    rl2_set_palette_color (palette, 3, 255, 255, 255);
    return palette;
}

rl2PalettePtr
create_rgb16_palette ()
{
/* creating an RGB-16 palette */
    rl2PalettePtr palette = rl2_create_palette (16);
    rl2_set_palette_color (palette, 0, 255, 0, 0);
    rl2_set_palette_color (palette, 1, 255, 17, 17);
    rl2_set_palette_color (palette, 2, 255, 34, 34);
    rl2_set_palette_color (palette, 3, 255, 51, 51);
    rl2_set_palette_color (palette, 4, 68, 255, 68);
    rl2_set_palette_color (palette, 5, 85, 255, 85);
    rl2_set_palette_color (palette, 6, 102, 255, 102);
    rl2_set_palette_color (palette, 7, 119, 255, 119);
    rl2_set_palette_color (palette, 8, 137, 255, 137);
    rl2_set_palette_color (palette, 9, 154, 255, 154);
    rl2_set_palette_color (palette, 10, 171, 171, 255);
    rl2_set_palette_color (palette, 11, 188, 188, 255);
    rl2_set_palette_color (palette, 12, 205, 205, 255);
    rl2_set_palette_color (palette, 13, 222, 222, 255);
    rl2_set_palette_color (palette, 14, 239, 239, 255);
    rl2_set_palette_color (palette, 15, 255, 255, 255);
    return palette;
}

rl2PalettePtr
create_rgb256_palette ()
{
/* creating an RGB-256 palette */
    int i;
    rl2PalettePtr palette = rl2_create_palette (256);
    for (i = 0; i < 256; i++)
	rl2_set_palette_color (palette, i, 255 - i, i, 128);
    return palette;
}

static int
test_sql_palette (unsigned char *blob, int blob_size, unsigned char *blob2,
		  int blob_size2, int *retcode)
{
    int ret;
    sqlite3 *db_handle;
    const char *sql;
    sqlite3_stmt *stmt;
    unsigned char *blob3 = NULL;
    int blob_size3 = 0;
    void *cache = spatialite_alloc_connection ();

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return 0;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, 0);

    sql = "SELECT RL2_GetPaletteNumEntries(?), RL2_GetPaletteNumEntries(?), "
	"RL2_GetPaletteColorEntry(?, 0), RL2_GetPaletteColorEntry(?, 0), "
	"RL2_SetPaletteColorEntry(?, 123, '#c000ff'), RL2_PaletteEquals(?, ?), "
	"RL2_PaletteEquals(?, ?)";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: SQL palette stmt #1\n");
	  *retcode -= 1;
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 2, blob2, blob_size2, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 3, blob, blob_size, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 4, blob2, blob_size2, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 5, blob, blob_size, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 6, blob, blob_size, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 7, blob2, blob_size2, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 8, blob, blob_size, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 9, blob, blob_size, SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) != SQLITE_INTEGER)
		  {
		      fprintf (stderr,
			       "ERROR: invalid Palette NumEntries #1\n");
		      *retcode -= 2;
		      goto error;
		  }
		if (sqlite3_column_int (stmt, 0) != 256)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected Palette NumEntries #1: %d\n",
			       sqlite3_column_int (stmt, 0));
		      *retcode -= 3;
		      goto error;
		  }
		if (sqlite3_column_type (stmt, 1) != SQLITE_INTEGER)
		  {
		      fprintf (stderr,
			       "ERROR: invalid Palette NumEntries #2\n");
		      *retcode -= 4;
		      goto error;
		  }
		if (sqlite3_column_int (stmt, 1) != 2)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected Palette NumEntries #2: %d\n",
			       sqlite3_column_int (stmt, 1));
		      *retcode -= 5;
		      goto error;
		  }
		if (sqlite3_column_type (stmt, 2) != SQLITE_TEXT)
		  {
		      fprintf (stderr,
			       "ERROR: invalid Palette ColorEntry #1\n");
		      *retcode -= 6;
		      goto error;
		  }
		if (strcmp
		    ("#000000",
		     (const char *) sqlite3_column_text (stmt, 2)) != 0)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected Palette ColorEntry #1: %s\n",
			       sqlite3_column_text (stmt, 2));
		      *retcode -= 7;
		      goto error;
		  }
		if (sqlite3_column_type (stmt, 3) != SQLITE_TEXT)
		  {
		      fprintf (stderr,
			       "ERROR: invalid Palette ColorEntry #2\n");
		      *retcode -= 8;
		      goto error;
		  }
		if (strcmp
		    ("#ffffff",
		     (const char *) sqlite3_column_text (stmt, 3)) != 0)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected Palette ColorEntry #2: %s\n",
			       sqlite3_column_text (stmt, 3));
		      *retcode -= 9;
		      goto error;
		  }
		if (sqlite3_column_type (stmt, 4) != SQLITE_BLOB)
		  {
		      fprintf (stderr, "ERROR: invalid Palette SetColor\n");
		      *retcode -= 10;
		      goto error;
		  }
		blob_size3 = sqlite3_column_bytes (stmt, 4);
		blob3 = malloc (blob_size3);
		memcpy (blob3, sqlite3_column_blob (stmt, 4), blob_size3);
		if (sqlite3_column_type (stmt, 5) != SQLITE_INTEGER)
		  {
		      fprintf (stderr, "ERROR: invalid Palette Equals #1\n");
		      *retcode -= 11;
		      goto error;
		  }
		if (sqlite3_column_int (stmt, 5) != 0)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected Palette Equals #1: %d\n",
			       sqlite3_column_int (stmt, 5));
		      *retcode -= 12;
		      goto error;
		  }
		if (sqlite3_column_type (stmt, 6) != SQLITE_INTEGER)
		  {
		      fprintf (stderr, "ERROR: invalid Palette Equals #2\n");
		      *retcode -= 13;
		      goto error;
		  }
		if (sqlite3_column_int (stmt, 6) != 1)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected Palette Equals #2: %d\n",
			       sqlite3_column_int (stmt, 7));
		      *retcode -= 14;
		      goto error;
		  }
	    }
	  else
	    {
		fprintf (stderr, "ERROR: SQL palette step #1\n");
		*retcode -= 15;
		goto error;
	    }
      }
    sqlite3_finalize (stmt);

    sql = "SELECT RL2_PaletteEquals(?, ?)";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ERROR: SQL palette stmt #2\n");
	  *retcode -= 16;
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 2, blob3, blob_size3, SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) != SQLITE_INTEGER)
		  {
		      fprintf (stderr, "ERROR: invalid Palette Equals #3\n");
		      *retcode -= 17;
		      goto error;
		  }
		if (sqlite3_column_int (stmt, 0) != 0)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected Palette Equals #3: %d\n",
			       sqlite3_column_int (stmt, 7));
		      *retcode -= 18;
		      goto error;
		  }
	    }
	  else
	    {
		fprintf (stderr, "ERROR: SQL palette step #2\n");
		*retcode -= 19;
		goto error;
	    }
      }
    sqlite3_finalize (stmt);

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    free (blob);
    free (blob2);
    if (blob3 != NULL)
	free (blob3);
    return 1;

  error:
    sqlite3_finalize (stmt);
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    free (blob);
    free (blob2);
    if (blob3 != NULL)
	free (blob3);
    return 0;
}

int
main (int argc, char *argv[])
{
    rl2PalettePtr palette;
    rl2PalettePtr plt2;
    unsigned short num_entries;
    unsigned char *r;
    unsigned char *g;
    unsigned char *b;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char *blob;
    int blob_size;
    unsigned char *blob2;
    int blob_size2;
    int retcode;
    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */
    if (rl2_create_palette (-1) != NULL)
      {
	  fprintf (stderr, "Unexpected valid palette - negative # entries\n");
	  return -1;
      }

    palette = rl2_create_palette (0);
    if (palette == NULL)
      {
	  fprintf (stderr, "Unexpected invalid palette - ZERO # entries\n");
	  return -2;
      }
    rl2_destroy_palette (palette);
    if (rl2_create_palette (257) != NULL)
      {
	  fprintf (stderr, "Unexpected valid palette - 257 # entries\n");
	  return -3;
      }

    palette = rl2_create_palette (2);
    if (palette == NULL)
      {
	  fprintf (stderr, "Unable to create a valid palette\n");
	  return -4;
      }

    if (rl2_set_palette_color (palette, 0, 0, 0, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set palette color #0\n");
	  return -5;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "#aBcDeF") != RL2_OK)
      {
	  fprintf (stderr, "Unable to set palette color #1\n");
	  return -6;
      }

    if (rl2_get_palette_entries (palette, &num_entries) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get palette # entries\n");
	  return -7;
      }

    if (num_entries != 2)
      {
	  fprintf (stderr, "Unexpected palette # entries\n");
	  return -8;
      }

    if (rl2_get_palette_colors (palette, &num_entries, &r, &g, &b) != RL2_OK)
      {
	  fprintf (stderr, "Unable to retrieve palette colors\n");
	  return -9;
      }
    if (num_entries != 2)
      {
	  fprintf (stderr, "Unexpected palette # colors\n");
	  return -10;
      }
    if (*(r + 0) != 0 || *(g + 0) != 0 || *(b + 0) != 0)
      {
	  fprintf (stderr, "Mismatching color #0\n");
	  return -11;
      }
    if (*(r + 1) != 0xab || *(g + 1) != 0xcd || *(b + 1) != 0xef)
      {
	  fprintf (stderr, "Mismatching color #1\n");
	  return -11;
      }
    rl2_free (r);
    rl2_free (g);
    rl2_free (b);
    if (rl2_set_palette_color (NULL, 0, 0, 0, 0) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL palette set color\n");
	  return -12;
      }

    if (rl2_set_palette_color (palette, -1, 0, 0, 0) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: palette set color (negative index)\n");
	  return -13;
      }

    if (rl2_set_palette_color (palette, 2, 0, 0, 0) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: palette set color (exceeding index)\n");
	  return -14;
      }

    if (rl2_set_palette_hexrgb (NULL, 1, "#aBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette HEX color\n");
	  return -15;
      }

    if (rl2_set_palette_hexrgb (palette, -1, "#aBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unable to set palette HEX color (negative index)\n");
	  return -16;
      }

    if (rl2_set_palette_hexrgb (palette, 2, "#aBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unable to set palette HEX color (exceeding index)\n");
	  return -17;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "aBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #1\n");
	  return -18;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "aBcDeF#") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #2\n");
	  return -19;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "#zBcDeF") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #3\n");
	  return -20;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "#aBcDeZ") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #4\n");
	  return -21;
      }

    if (rl2_set_palette_hexrgb (palette, 1, "#aBcGef") != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - invalid-hex #5\n");
	  return -22;
      }

    if (rl2_set_palette_hexrgb (palette, 1, NULL) != RL2_ERROR)
      {
	  fprintf (stderr, "Unable to set palette color - NULL-hex\n");
	  return -23;
      }

    rl2_destroy_palette (palette);
    rl2_destroy_palette (NULL);
    if (rl2_get_palette_entries (NULL, &num_entries) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL palette # entries\n");
	  return -24;
      }

    if (rl2_get_palette_colors (NULL, &num_entries, &r, &g, &b) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL palette - get colors\n");
	  return -25;
      }

    palette = create_rgb256_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 256-RGB\n");
	  return -26;
      }
    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr,
		   "ERROR: palette 256-RGB - unexpected sample type %02x\n",
		   sample_type);
	  return -27;
      }
    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr,
		   "ERROR: palette 256-RGB - unexpected pixel type %02x\n",
		   pixel_type);
	  return -27;
      }
    rl2_destroy_palette (palette);
    palette = create_rgb16_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 16-RGB\n");
	  return -28;
      }
    if (sample_type != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette 16-RGB - unexpected sample type %02x\n",
		   sample_type);
	  return -29;
      }
    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr,
		   "ERROR: palette 16-RGB - unexpected pixel type %02x\n",
		   pixel_type);
	  return -30;
      }
    rl2_destroy_palette (palette);
    palette = create_rgb4_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 4-RGB\n");
	  return -31;
      }
    if (sample_type != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette 4-RGB - unexpected sample type %02x\n",
		   sample_type);
	  return -32;
      }
    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr,
		   "ERROR: palette 4-RGB - unexpected pixel type %02x\n",
		   pixel_type);
	  return -33;
      }
    rl2_destroy_palette (palette);
    palette = create_bicolor_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - BICOLOR\n");
	  return -34;
      }
    if (sample_type != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette BICOLOR - unexpected sample type %02x\n",
		   sample_type);
	  return -35;
      }
    if (pixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr,
		   "ERROR: palette BICOLOR - unexpected pixel type %02x\n",
		   pixel_type);
	  return -36;
      }
    rl2_destroy_palette (palette);
    palette = create_gray256_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to get the Palette type - 256-GRAY\n");
	  return -37;
      }
    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr,
		   "ERROR: palette 256-GRAY - unexpected sample type %02x\n",
		   sample_type);
	  return -38;
      }
    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr,
		   "ERROR: palette 256-GRAY - unexpected pixel type %02x\n",
		   pixel_type);
	  return -39;
      }
    rl2_destroy_palette (palette);
    palette = create_gray16_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 16-GRAY\n");
	  return -40;
      }
    if (sample_type != RL2_SAMPLE_4_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette 16-GRAY - unexpected sample type %02x\n",
		   sample_type);
	  return -41;
      }
    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr,
		   "ERROR: palette 16-GRAY - unexpected pixel type %02x\n",
		   pixel_type);
	  return -42;
      }
    rl2_destroy_palette (palette);
    palette = create_gray4_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get the Palette type - 4-GRAY\n");
	  return -43;
      }
    if (sample_type != RL2_SAMPLE_2_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette 4-GRAY - unexpected sample type %02x\n",
		   sample_type);
	  return -44;
      }
    if (pixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr,
		   "ERROR: palette 4-GRAY - unexpected pixel type %02x\n",
		   pixel_type);
	  return -45;
      }
    rl2_destroy_palette (palette);
    palette = create_monochrome_palette ();
    if (rl2_get_palette_type (palette, &sample_type, &pixel_type) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to get the Palette type - MONOCHROME\n");
	  return -46;
      }
    if (sample_type != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr,
		   "ERROR: palette MONOCHROME - unexpected sample type %02x\n",
		   sample_type);
	  return -47;
      }
    if (pixel_type != RL2_PIXEL_MONOCHROME)
      {
	  fprintf (stderr,
		   "ERROR: palette MONOCHROME - unexpected pixel type %02x\n",
		   pixel_type);
	  return -48;
      }
    if (rl2_serialize_dbms_palette (palette, &blob, &blob_size) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to serialize a palette\n");
	  return -49;
      }
    rl2_destroy_palette (palette);
    palette = rl2_deserialize_dbms_palette (blob, blob_size);
    free (blob);
    if (palette == NULL)
      {
	  fprintf (stderr, "ERROR: unable to deserialize a palette\n");
	  return -50;
      }
    plt2 = rl2_clone_palette (palette);
    rl2_destroy_palette (palette);
    if (plt2 == NULL)
      {
	  fprintf (stderr, "ERROR: unable to clone a palette\n");
	  return 51;
      }
    rl2_destroy_palette (plt2);
    if (rl2_serialize_dbms_palette (NULL, &blob, &blob_size) == RL2_OK)
      {
	  fprintf (stderr, "ERROR: unexpected NULL palette serialization\n");
	  return -53;
      }
    if (rl2_deserialize_dbms_palette (NULL, blob_size) != NULL)
      {
	  fprintf (stderr,
		   "ERROR: unexpected palette deserialization from NULL BLOB\n");
	  return -54;
      }
    if (rl2_deserialize_dbms_palette (blob, 0) != NULL)
      {
	  fprintf (stderr,
		   "ERROR: unexpected palette deserialization from zero-length BLOB\n");
	  return -55;
      }
    if (rl2_clone_palette (NULL) != NULL)
      {
	  fprintf (stderr, "ERROR: unexpected cloned NULL palette\n");
	  return -56;
      }

    palette = create_gray256_palette ();
    if (palette == NULL)
      {
	  fprintf (stderr, "ERROR: unable to create a palette\n");
	  return -57;
      }
    if (rl2_serialize_dbms_palette (palette, &blob, &blob_size) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unexpected NULL palette serialization\n");
	  return -58;
      }
    rl2_destroy_palette (palette);
    palette = create_monochrome_palette ();
    if (palette == NULL)
      {
	  fprintf (stderr, "ERROR: unable to create a palette\n");
	  return -59;
      }
    if (rl2_serialize_dbms_palette (palette, &blob2, &blob_size2) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unexpected NULL palette serialization\n");
	  return -60;
      }
    rl2_destroy_palette (palette);
    retcode = -61;
    if (!test_sql_palette (blob, blob_size, blob2, blob_size2, &retcode))
	return retcode;
    return 0;
}
