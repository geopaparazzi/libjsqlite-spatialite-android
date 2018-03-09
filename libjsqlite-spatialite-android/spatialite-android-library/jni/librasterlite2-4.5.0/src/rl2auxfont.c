/*

 rl2auxfont -- FreeType2 and Font related functions

 version 0.1, 2015 February 12

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
 
Portions created by the Initial Developer are Copyright (C) 2015
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

#include <zlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "rasterlite2/sqlite.h"

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

static int
font_endianArch ()
{
/* checking if target CPU is a little-endian one */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 0;
    return 1;
}

static void
font_export16 (unsigned char *p, unsigned short value, int little_endian_arch)
{
/* stores a 16bit int into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[2];
	unsigned short int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  *(p + 0) = convert.byte[0];
	  *(p + 1) = convert.byte[1];
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  *(p + 1) = convert.byte[0];
	  *(p + 0) = convert.byte[1];
      }
}

static unsigned short
font_import16 (const unsigned char *p, int little_endian_arch)
{
/* fetches a 16bit uint from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[2];
	unsigned short int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  convert.byte[0] = *(p + 0);
	  convert.byte[1] = *(p + 1);
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  convert.byte[0] = *(p + 1);
	  convert.byte[1] = *(p + 0);
      }
    return convert.int_value;
}

static void
font_export32 (unsigned char *p, unsigned int value, int little_endian_arch)
{
/* stores a 32bit int into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	unsigned int int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  *(p + 0) = convert.byte[0];
	  *(p + 1) = convert.byte[1];
	  *(p + 2) = convert.byte[2];
	  *(p + 3) = convert.byte[3];
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  *(p + 3) = convert.byte[0];
	  *(p + 2) = convert.byte[1];
	  *(p + 1) = convert.byte[2];
	  *(p + 0) = convert.byte[3];
      }
}

static unsigned int
font_import32 (const unsigned char *p, int little_endian_arch)
{
/* fetches a 32bit uint from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	unsigned int int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  convert.byte[0] = *(p + 0);
	  convert.byte[1] = *(p + 1);
	  convert.byte[2] = *(p + 2);
	  convert.byte[3] = *(p + 3);
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  convert.byte[0] = *(p + 3);
	  convert.byte[1] = *(p + 2);
	  convert.byte[2] = *(p + 1);
	  convert.byte[3] = *(p + 0);
      }
    return convert.int_value;
}

static int
check_font (const unsigned char *buffer, int buf_size, char **family_name,
	    char **style_name, int *is_bold, int *is_italic)
{
/* exploring a FreeType Font */
    FT_Error error;
    FT_Library library;
    FT_Face face;
    int len;

    if (buffer == NULL || buf_size <= 0)
	return RL2_ERROR;

/* initializing a FreeType2 library object */
    error = FT_Init_FreeType (&library);
    if (error)
	return RL2_ERROR;

/* attempting to parse the Font */
    error = FT_New_Memory_Face (library, buffer, buf_size, 0, &face);
    if (error)
	goto stop;

    if (face->family_name == NULL)
	goto stop;
    len = strlen (face->family_name);
    *family_name = malloc (len + 1);
    strcpy (*family_name, face->family_name);
    if (face->style_name == NULL)
	*style_name = NULL;
    else
      {
	  len = strlen (face->style_name);
	  *style_name = malloc (len + 1);
	  strcpy (*style_name, face->style_name);
      }
    if (face->style_flags & FT_STYLE_FLAG_ITALIC)
	*is_italic = 1;
    else
	*is_italic = 0;
    if (face->style_flags & FT_STYLE_FLAG_BOLD)
	*is_bold = 1;
    else
	*is_bold = 0;

/* releasing all resources */
    FT_Done_Face (face);
    FT_Done_FreeType (library);
    return RL2_OK;

  stop:
/* invalid Font */
    FT_Done_FreeType (library);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_font_encode (const unsigned char *font, int font_sz, unsigned char **blob,
		 int *blob_sz)
{
/* attempting to encode a BLOB serialized Font */
    char *family_name = NULL;
    char *style_name = NULL;
    int is_bold = 0;
    int is_italic = 0;
    short family_len;
    short style_len;
    uLong zLen;
    unsigned char *zip_buf = NULL;
    int compressed;
    const unsigned char *compr_data = NULL;
    int ret;
    unsigned char *block = NULL;
    int block_size;
    unsigned char *ptr;
    uLong crc;
    int endian_arch = font_endianArch ();

    *blob = NULL;
    *blob_sz = 0;

    if (font == NULL || font_sz == 0)
	return RL2_ERROR;
    if (check_font
	(font, font_sz, &family_name, &style_name, &is_bold,
	 &is_italic) != RL2_OK)
	return RL2_ERROR;

    family_len = strlen (family_name);
    if (style_name == NULL)
	style_len = 0;
    else
	style_len = strlen (style_name);

/* compressing as ZIP [Deflate] */
    zLen = font_sz - 1;
    zip_buf = malloc (zLen);
    ret = compress (zip_buf, &zLen, (const Bytef *) font, (uLong) font_sz);
    if (ret == Z_OK)
      {
	  /* ok, ZIP compression was successful */
	  compressed = (int) zLen;
	  compr_data = zip_buf;
      }
    else if (ret == Z_BUF_ERROR)
      {
	  /* ZIP compression actually causes inflation: saving uncompressed data */
	  compressed = font_sz;
	  compr_data = font;
	  free (zip_buf);
	  zip_buf = NULL;
      }
    else
      {
	  /* compression error */
	  free (zip_buf);
	  goto error;
      }

/* preparing the serialized BLOB */
    block_size = 26 + compressed + family_len + style_len;
    block = malloc (block_size);
    if (block == NULL)
	goto error;
    ptr = block;
    *ptr++ = 0x00;		/* start marker */
    *ptr++ = RL2_FONT_START;	/* Font Start marker */
    font_export16 (ptr, family_len, endian_arch);	/* Family name length in bytes */
    ptr += 2;
    memcpy (ptr, family_name, family_len);
    ptr += family_len;
    *ptr++ = RL2_DATA_END;
    font_export16 (ptr, style_len, endian_arch);	/* Style name length in bytes */
    ptr += 2;
    if (style_name != NULL)
      {
	  memcpy (ptr, style_name, style_len);
	  ptr += style_len;
      }
    *ptr++ = RL2_DATA_END;
    if (is_bold)
	*ptr++ = 1;
    else
	*ptr++ = 0;
    if (is_italic)
	*ptr++ = 1;
    else
	*ptr++ = 0;
    *ptr++ = RL2_DATA_END;
    font_export32 (ptr, font_sz, endian_arch);	/* uncompressed payload size in bytes */
    ptr += 4;
    font_export32 (ptr, compressed, endian_arch);	/* compressed payload size in bytes */
    ptr += 4;
    *ptr++ = RL2_DATA_START;
    memcpy (ptr, compr_data, compressed);	/* the payload */
    ptr += compressed;
    *ptr++ = RL2_DATA_END;
/* computing the CRC32 */
    crc = crc32 (0L, block, ptr - block);
    font_export32 (ptr, crc, endian_arch);	/* the serialized BLOB own CRC */
    ptr += 4;
    *ptr = RL2_FONT_END;

    *blob = block;
    *blob_sz = block_size;

/* final cleanup */
    if (zip_buf != NULL)
	free (zip_buf);
    free (family_name);
    if (style_name != NULL)
	free (style_name);
    return RL2_OK;

  error:
/* some error occurred */
    if (family_name != NULL)
	free (family_name);
    if (style_name != NULL)
	free (style_name);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_is_valid_encoded_font (const unsigned char *blob, int blob_sz)
{
/* checking a serialized BLOB Font for validity */
    const unsigned char *ptr;
    int endian_arch = font_endianArch ();
    unsigned short len;
    int compressed;
    uLong crc;
    uLong oldCrc;

    if (blob == NULL)
	return RL2_ERROR;
    ptr = blob;

    if (blob_sz < 5)
	return RL2_ERROR;
    if (*ptr++ != 0x00)
	return RL2_ERROR;	/* invalid start signature */
    if (*ptr++ != RL2_FONT_START)
	return RL2_ERROR;	/* invalid start signature */
    len = font_import16 (ptr, endian_arch);	/* Family name length in bytes */
    ptr += 2 + len;
    if ((ptr - blob) >= blob_sz)
	return RL2_ERROR;
    if (*ptr++ != RL2_DATA_END)
	return RL2_ERROR;
    if (((ptr + 2) - blob) >= blob_sz)
	return RL2_ERROR;
    len = font_import16 (ptr, endian_arch);	/* Style name length in bytes */
    ptr += 2 + len;
    if ((ptr - blob) >= blob_sz)
	return RL2_ERROR;
    if (*ptr++ != RL2_DATA_END)
	return RL2_ERROR;
    ptr += 2;			/* skipping Bold and Italic */
    if (((ptr + 2) - blob) >= blob_sz)
	return RL2_ERROR;
    if (*ptr++ != RL2_DATA_END)
	return RL2_ERROR;
    if (((ptr + 4) - blob) >= blob_sz)
	return RL2_ERROR;
    compressed = font_import32 (ptr, endian_arch);	/* uncompressed size in bytes */
    ptr += 4;
    if (((ptr + 4) - blob) >= blob_sz)
	return RL2_ERROR;
    compressed = font_import32 (ptr, endian_arch);	/* compressed size in bytes */
    ptr += 4;
    if ((ptr - blob) >= blob_sz)
	return RL2_ERROR;
    if (*ptr++ != RL2_DATA_START)
	return RL2_ERROR;
    ptr += compressed;
    if ((ptr - blob) >= blob_sz)
	return RL2_ERROR;
    if (*ptr++ != RL2_DATA_END)
	return RL2_ERROR;
/* computing the CRC32 */
    crc = crc32 (0L, blob, ptr - blob);
    if (((ptr + 4) - blob) >= blob_sz)
	return RL2_ERROR;
    oldCrc = font_import32 (ptr, endian_arch);
    ptr += 4;
    if (crc != oldCrc)
	return RL2_ERROR;
    if ((ptr - blob) >= blob_sz)
	return RL2_ERROR;
    if (*ptr != RL2_FONT_END)
	return RL2_ERROR;	/* invalid end signature */
    return RL2_OK;
}

RL2_DECLARE int
rl2_font_decode (const unsigned char *blob, int blob_sz,
		 unsigned char **font, int *font_sz)
{
/* attempting to decode a serialized BLOB Font */
    const unsigned char *ptr;
    int endian_arch = font_endianArch ();
    unsigned short len;
    int compressed;
    int uncompressed;
    unsigned char *data = NULL;

    if (rl2_is_valid_encoded_font (blob, blob_sz) != RL2_OK)
	return RL2_ERROR;

    ptr = blob + 2;
    len = font_import16 (ptr, endian_arch);	/* Family name length in bytes */
    ptr += 3 + len;
    len = font_import16 (ptr, endian_arch);	/* Style name length in bytes */
    ptr += 3 + len;
    ptr += 3;			/* skipping Bold and Italic */
    uncompressed = font_import32 (ptr, endian_arch);	/* uncompressed size in bytes */
    ptr += 4;
    if (((ptr + 4) - blob) >= blob_sz)
	return RL2_ERROR;
    compressed = font_import32 (ptr, endian_arch);	/* compressed size in bytes */
    ptr += 5;

    if (uncompressed != compressed)
      {
	  /* decompressing from ZIP [Deflate] */
	  uLong refLen = uncompressed;
	  const Bytef *in = ptr;
	  data = malloc (uncompressed);
	  if (data == NULL)
	      goto error;
	  if (uncompress (data, &refLen, in, compressed) != Z_OK)
	      goto error;
	  *font = data;
	  *font_sz = uncompressed;
      }
    else
      {
	  /* already uncompressed Font */
	  data = malloc (uncompressed);
	  if (data == NULL)
	      goto error;
	  memcpy (data, ptr, uncompressed);
	  *font = data;
	  *font_sz = uncompressed;
      }
    return RL2_OK;

  error:
    if (data != NULL)
	free (data);
    return RL2_ERROR;
}

RL2_DECLARE char *
rl2_get_encoded_font_facename (const unsigned char *blob, int blob_sz)
{
/* attempting to return the Facename from a serialized BLOB Font */
    const unsigned char *ptr;
    const unsigned char *family;
    const unsigned char *style;
    int endian_arch = font_endianArch ();
    unsigned short len1;
    unsigned short len2;
    char *name = NULL;

    if (rl2_is_valid_encoded_font (blob, blob_sz) != RL2_OK)
	return NULL;

    ptr = blob + 2;
    len1 = font_import16 (ptr, endian_arch);	/* Family name length in bytes */
    family = ptr + 2;
    ptr += 3 + len1;
    len2 = font_import16 (ptr, endian_arch);	/* Style name length in bytes */
    style = ptr + 2;
    if (len2 == 0)
	name = malloc (len1 + 1);
    else
	name = malloc (len1 + len2 + 2);
    memcpy (name, family, len1);
    if (len2 == 0)
	*(name + len1) = '\0';
    else
      {
	  *(name + len1) = '-';
	  memcpy (name + len1 + 1, style, len2);
	  *(name + len1 + len2 + 1) = '\0';
      }
    return name;
}

RL2_DECLARE char *
rl2_get_encoded_font_family (const unsigned char *blob, int blob_sz)
{
/* attempting to return the Family name from a serialized BLOB Font */
    const unsigned char *ptr;
    int endian_arch = font_endianArch ();
    unsigned short len;
    char *name = NULL;

    if (rl2_is_valid_encoded_font (blob, blob_sz) != RL2_OK)
	return NULL;

    ptr = blob + 2;
    len = font_import16 (ptr, endian_arch);	/* Family length in bytes */
    ptr += 2;
    name = malloc (len + 1);
    memcpy (name, ptr, len);
    *(name + len) = '\0';
    return name;
}

RL2_DECLARE char *
rl2_get_encoded_font_style (const unsigned char *blob, int blob_sz)
{
/* attempting to return the  Style name from a serialized BLOB Font */
    const unsigned char *ptr;
    int endian_arch = font_endianArch ();
    unsigned short len;
    char *name = NULL;

    if (rl2_is_valid_encoded_font (blob, blob_sz) != RL2_OK)
	return NULL;

    ptr = blob + 2;
    len = font_import16 (ptr, endian_arch);	/* Family name length in bytes */
    ptr += 3 + len;
    len = font_import16 (ptr, endian_arch);	/* Style name length in bytes */
    if (len == 0)
	return NULL;		/* this Font has no Style name */
    ptr += 2;
    name = malloc (len + 1);
    memcpy (name, ptr, len);
    *(name + len) = '\0';
    return name;
}

RL2_DECLARE int
rl2_is_encoded_font_bold (const unsigned char *blob, int blob_sz)
{
/* testing if a serialized BLOB Font is Bold */
    const unsigned char *ptr;
    int endian_arch = font_endianArch ();
    unsigned short len;

    if (rl2_is_valid_encoded_font (blob, blob_sz) != RL2_OK)
	return -1;

    ptr = blob + 2;
    len = font_import16 (ptr, endian_arch);	/* Family name length in bytes */
    ptr += 3 + len;
    len = font_import16 (ptr, endian_arch);	/* Style name length in bytes */
    ptr += 3 + len;
    return *ptr++;
}

RL2_DECLARE int
rl2_is_encoded_font_italic (const unsigned char *blob, int blob_sz)
{
/* testing if a serialized BLOB Font is Italic */
    const unsigned char *ptr;
    int endian_arch = font_endianArch ();
    unsigned short len;

    if (rl2_is_valid_encoded_font (blob, blob_sz) != RL2_OK)
	return -1;

    ptr = blob + 2;
    len = font_import16 (ptr, endian_arch);	/* Family name length in bytes */
    ptr += 3 + len;
    len = font_import16 (ptr, endian_arch);	/* Style name length in bytes */
    ptr += 4 + len;
    return *ptr++;
}

RL2_PRIVATE int
rl2_load_font_into_dbms (sqlite3 * handle, unsigned char *blob, int blob_sz)
{
/* loading a serialized BLOB Font into the DBMS */
    char *facename = NULL;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    if (rl2_is_valid_encoded_font (blob, blob_sz) != RL2_OK)
	return RL2_ERROR;

    facename = rl2_get_encoded_font_facename (blob, blob_sz);
    if (facename == NULL)
	return RL2_ERROR;

/* inserting the Font */
    sql = "INSERT INTO main.SE_fonts (font_facename, font) VALUES (?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, facename, strlen (facename), SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 2, blob, blob_sz, SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	sqlite3_finalize (stmt);
    else
	goto error;
    free (facename);
    free (blob);
    return RL2_OK;

  error:
    if (facename != NULL)
	free (facename);
    free (blob);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_PRIVATE int
rl2_get_font_from_dbms (sqlite3 * handle, const char *db_prefix,
			const char *facename, unsigned char **font,
			int *font_sz)
{
/* attempting to fetch a Font from the DBMS */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    unsigned char *xfont = NULL;
    int xfont_sz;
    char *xdb_prefix;

    *font = NULL;
    *font_sz = 0;

/* preparing the SQL query statement */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT font FROM \"%s\".SE_fonts WHERE Lower(font_facename) = Lower(?)",
	 xdb_prefix);
    free (xdb_prefix);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, facename, strlen (facename), SQLITE_STATIC);
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
		      if (xfont != NULL)
			{
			    free (xfont);
			    xfont = NULL;
			}
		      if (rl2_font_decode (blob, blob_sz, &xfont, &xfont_sz)
			  == RL2_OK)
			{
			    *font = xfont;
			    *font_sz = xfont_sz;
			}
		  }
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    if (*font == NULL)
	return RL2_ERROR;
    return RL2_OK;

  error:
    if (xfont != NULL)
	free (xfont);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_TrueType_font (sqlite3 * handle, const char *facename,
		       unsigned char **font, int *font_sz)
{
/* attempting to fetch a BLOB-encoded TrueType Font */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    unsigned char *xfont = NULL;
    if (facename == NULL)
	return RL2_ERROR;

    *font = NULL;
    *font_sz = 0;
/* preparing the SQL query statement */
    sql = "SELECT font FROM SE_fonts WHERE Lower(font_facename) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, facename, strlen (facename), SQLITE_STATIC);
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
		      if (xfont != NULL)
			{
			    free (xfont);
			    xfont = NULL;
			}
		      if (rl2_is_valid_encoded_font (blob, blob_sz) == RL2_OK)
			{
			    *font = malloc (blob_sz);
			    *font_sz = blob_sz;
			    memcpy (*font, blob, blob_sz);
			}
		  }
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    if (*font == NULL)
	return RL2_ERROR;
    return RL2_OK;

  error:
    if (xfont != NULL)
	free (xfont);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE rl2GraphicsFontPtr
rl2_search_TrueType_font (sqlite3 * handle, const void *priv_data,
			  const char *facename, double size)
{
/* attempting to fetch and create a TrueType Font */
    rl2GraphicsFontPtr font;
    unsigned char *ttf = NULL;
    int ttf_sz;
    if (facename == NULL)
	return NULL;

    if (rl2_get_TrueType_font (handle, facename, &ttf, &ttf_sz) != RL2_OK)
	return NULL;
    font = rl2_graph_create_TrueType_font (priv_data, ttf, ttf_sz, size);
    if (ttf != NULL)
	free (ttf);
    return font;
}
