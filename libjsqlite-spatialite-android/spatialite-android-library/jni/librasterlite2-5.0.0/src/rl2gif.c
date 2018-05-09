/*

 rl2gif -- GIF related functions

 version 0.1, 2013 April 5

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

#include <gif_lib.h>

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

struct gif_memory_buffer
{
    unsigned char *buffer;
    size_t size;
    size_t off;
};

static int
readGif (GifFileType * gif, GifByteType * buf, int len)
{
    struct gif_memory_buffer *p = gif->UserData;
    size_t rd = len;
    if (p->off + len > p->size)
	rd = p->size - p->off;
    if (rd != 0)
      {
	  /* copy bytes into buffer */
	  memcpy (buf, p->buffer + p->off, rd);
	  p->off += rd;
      }
    return rd;
}

static int
writeGif (GifFileType * gif, const GifByteType * buf, int len)
{
    struct gif_memory_buffer *p = gif->UserData;
    size_t nsize = p->size + len;

    /* allocate or grow buffer */
    if (p->buffer)
	p->buffer = realloc (p->buffer, nsize);
    else
	p->buffer = malloc (nsize);

    if (!p->buffer)
	return 0;

    /* copy new bytes to end of buffer */
    memcpy (p->buffer + p->size, buf, len);
    p->size += len;
    return len;
}

static void
print_gif_error (int ErrorCode)
{
    const char *Err = NULL;
#ifdef GIFLIB_MAJOR
    Err = GifErrorString (ErrorCode);
#endif
    if (Err != NULL)
	fprintf (stderr, "GIF error: %d \"%s\"\n", ErrorCode, Err);
    else
	fprintf (stderr, "GIF error: %d\n", ErrorCode);
}

static int
check_gif_compatibility (unsigned char sample_type, unsigned char pixel_type,
			 unsigned char num_bands)
{
/* checks for GIF compatibility */
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  break;
      default:
	  return RL2_ERROR;
      };
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
	  break;
      default:
	  return RL2_ERROR;
      };
    if (num_bands != 1)
	return RL2_ERROR;
    if (pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
		break;
	    default:
		return RL2_ERROR;
	    };
      }
    if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
      }
    if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
      }
    return RL2_OK;
}

static int
compress_gif (rl2RasterPtr rst, unsigned char **gif, int *gif_size)
{
/* attempting to create a GIF compressed image */
    rl2PrivRasterPtr raster = (rl2PrivRasterPtr) rst;
    rl2PalettePtr plt;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_samples;
    unsigned char *blob;
    int blob_size;
    if (rst == NULL)
	return RL2_ERROR;
    if (rl2_get_raster_type (rst, &sample_type, &pixel_type, &num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (check_gif_compatibility (sample_type, pixel_type, num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    plt = rl2_get_raster_palette (rst);
    if (rl2_data_to_gif
	(raster->rasterBuffer, plt, raster->width,
	 raster->height, sample_type, pixel_type, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    *gif = blob;
    *gif_size = blob_size;
    return RL2_OK;
}

RL2_PRIVATE int
rl2_data_to_gif (const unsigned char *pixels, rl2PalettePtr plt,
		 unsigned int width, unsigned int height,
		 unsigned char sample_type, unsigned char pixel_type,
		 unsigned char **gif, int *gif_size)
{
/* attempting to create a GIF compressed image */
    struct gif_memory_buffer membuf;
    GifPixelType *ScanLine = NULL;
    GifFileType *GifFile = NULL;
#ifdef GIFLIB_MAJOR
    int ErrorCode;
#endif
    ColorMapObject *ColorMap = NULL;
    int i;
    unsigned int row;
    unsigned int col;
    const unsigned char *p_data;
    rl2PrivPalettePtr palette = (rl2PrivPalettePtr) plt;
    int max_palette;

    *gif = NULL;
    *gif_size = 0;
    membuf.buffer = NULL;
    membuf.size = 0;
    membuf.off = 0;

/* allocating the GIF scanline */
    if ((ScanLine =
	 (GifPixelType *) malloc (sizeof (GifPixelType) * width)) == NULL)
      {
	  fprintf (stderr, "Failed to allocate GIF scan line, aborted.\n");
	  return RL2_ERROR;
      }

/* opening the GIF */
#ifdef GIFLIB_MAJOR
    GifFile = EGifOpen (&membuf, writeGif, &ErrorCode);
#else
    GifFile = EGifOpen (&membuf, writeGif);
#endif
    if (GifFile == NULL)
      {
#ifdef GIFLIB_MAJOR
	  print_gif_error (ErrorCode);
#else
	  print_gif_error (GifLastError ());
#endif
	  goto error;
      }

/* preparing a GIF Palette object */
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
#ifdef GIFLIB_MAJOR
	  ColorMap = GifMakeMapObject (2, NULL);
#else
	  ColorMap = MakeMapObject (2, NULL);
#endif
	  if (ColorMap == NULL)
	      goto error;
	  ColorMap->BitsPerPixel = 1;
	  ColorMap->Colors[0].Red = 255;
	  ColorMap->Colors[0].Green = 255;
	  ColorMap->Colors[0].Blue = 255;
	  ColorMap->Colors[1].Red = 0;
	  ColorMap->Colors[1].Green = 0;
	  ColorMap->Colors[1].Blue = 0;
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_2_BIT:
#ifdef GIFLIB_MAJOR
		ColorMap = GifMakeMapObject (4, NULL);
#else
		ColorMap = MakeMapObject (4, NULL);
#endif
		if (ColorMap == NULL)
		    goto error;
		ColorMap->Colors[0].Red = 0;
		ColorMap->Colors[0].Green = 0;
		ColorMap->Colors[0].Blue = 0;
		ColorMap->Colors[1].Red = 86;
		ColorMap->Colors[1].Green = 86;
		ColorMap->Colors[1].Blue = 86;
		ColorMap->Colors[2].Red = 170;
		ColorMap->Colors[2].Green = 170;
		ColorMap->Colors[2].Blue = 170;
		ColorMap->Colors[3].Red = 255;
		ColorMap->Colors[3].Green = 255;
		ColorMap->Colors[3].Blue = 255;
		break;
	    case RL2_SAMPLE_4_BIT:
#ifdef GIFLIB_MAJOR
		ColorMap = GifMakeMapObject (16, NULL);
#else
		ColorMap = MakeMapObject (16, NULL);
#endif
		if (ColorMap == NULL)
		    goto error;
		ColorMap->Colors[0].Red = 0;
		ColorMap->Colors[0].Green = 0;
		ColorMap->Colors[0].Blue = 0;
		ColorMap->Colors[1].Red = 17;
		ColorMap->Colors[1].Green = 17;
		ColorMap->Colors[1].Blue = 17;
		ColorMap->Colors[2].Red = 34;
		ColorMap->Colors[2].Green = 34;
		ColorMap->Colors[2].Blue = 34;
		ColorMap->Colors[3].Red = 51;
		ColorMap->Colors[3].Green = 51;
		ColorMap->Colors[3].Blue = 51;
		ColorMap->Colors[4].Red = 68;
		ColorMap->Colors[4].Green = 68;
		ColorMap->Colors[4].Blue = 68;
		ColorMap->Colors[5].Red = 85;
		ColorMap->Colors[5].Green = 85;
		ColorMap->Colors[5].Blue = 85;
		ColorMap->Colors[6].Red = 102;
		ColorMap->Colors[6].Green = 102;
		ColorMap->Colors[6].Blue = 102;
		ColorMap->Colors[7].Red = 119;
		ColorMap->Colors[7].Green = 119;
		ColorMap->Colors[7].Blue = 119;
		ColorMap->Colors[8].Red = 137;
		ColorMap->Colors[8].Green = 137;
		ColorMap->Colors[8].Blue = 137;
		ColorMap->Colors[9].Red = 154;
		ColorMap->Colors[9].Green = 154;
		ColorMap->Colors[9].Blue = 154;
		ColorMap->Colors[10].Red = 171;
		ColorMap->Colors[10].Green = 171;
		ColorMap->Colors[10].Blue = 171;
		ColorMap->Colors[11].Red = 188;
		ColorMap->Colors[11].Green = 188;
		ColorMap->Colors[11].Blue = 188;
		ColorMap->Colors[12].Red = 205;
		ColorMap->Colors[12].Green = 205;
		ColorMap->Colors[12].Blue = 205;
		ColorMap->Colors[13].Red = 222;
		ColorMap->Colors[13].Green = 222;
		ColorMap->Colors[13].Blue = 222;
		ColorMap->Colors[14].Red = 239;
		ColorMap->Colors[14].Green = 239;
		ColorMap->Colors[14].Blue = 239;
		ColorMap->Colors[15].Red = 255;
		ColorMap->Colors[15].Green = 255;
		ColorMap->Colors[15].Blue = 255;
		break;
	    case RL2_SAMPLE_UINT8:
#ifdef GIFLIB_MAJOR
		ColorMap = GifMakeMapObject (256, NULL);
#else
		ColorMap = MakeMapObject (256, NULL);
#endif
		if (ColorMap == NULL)
		    goto error;
		for (i = 0; i < 256; i++)
		  {
		      ColorMap->Colors[i].Red = i;
		      ColorMap->Colors[i].Green = i;
		      ColorMap->Colors[i].Blue = i;
		  }
		break;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  if (palette == NULL)
	      goto error;
	  /* a GIF palette should must containt a power of 2 #entries */
	  if (palette->nEntries <= 2)
	      max_palette = 2;
	  else if (palette->nEntries <= 4)
	      max_palette = 4;
	  else if (palette->nEntries <= 8)
	      max_palette = 8;
	  else if (palette->nEntries <= 16)
	      max_palette = 16;
	  else if (palette->nEntries <= 32)
	      max_palette = 32;
	  else if (palette->nEntries <= 64)
	      max_palette = 64;
	  else if (palette->nEntries <= 128)
	      max_palette = 128;
	  else
	      max_palette = 256;
#ifdef GIFLIB_MAJOR
	  ColorMap = GifMakeMapObject (max_palette, NULL);
#else
	  ColorMap = MakeMapObject (max_palette, NULL);
#endif
	  if (ColorMap == NULL)
	      goto error;
	  for (i = 0; i < max_palette; i++)
	    {
		/* filling the palette with the latest entry */
		rl2PrivPaletteEntryPtr entry =
		    palette->entries + (palette->nEntries - 1);
		ColorMap->Colors[i].Red = entry->red;
		ColorMap->Colors[i].Green = entry->green;
		ColorMap->Colors[i].Blue = entry->blue;
	    }
	  for (i = 0; i < palette->nEntries; i++)
	    {
		/* effective palette entries */
		rl2PrivPaletteEntryPtr entry = palette->entries + i;
		ColorMap->Colors[i].Red = entry->red;
		ColorMap->Colors[i].Green = entry->green;
		ColorMap->Colors[i].Blue = entry->blue;
	    }
	  break;
      };

/* GIF headers */
    if (EGifPutScreenDesc
	(GifFile, width, height, ColorMap->BitsPerPixel, 0,
	 ColorMap) == GIF_ERROR)
      {
#ifdef GIFLIB_MAJOR
	  print_gif_error (ErrorCode);
#else
	  print_gif_error (GifLastError ());
#endif
	  goto error;
      }
    if (EGifPutImageDesc (GifFile, 0, 0, width, height, 0, NULL) == GIF_ERROR)
      {
#ifdef GIFLIB_MAJOR
	  print_gif_error (ErrorCode);
#else
	  print_gif_error (GifLastError ());
#endif
	  goto error;
      }

/* compressing GIF pixels */
    p_data = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      ScanLine[col] = *p_data++;
	  if (EGifPutLine (GifFile, ScanLine, width) == GIF_ERROR)
	    {
#ifdef GIFLIB_MAJOR
		print_gif_error (ErrorCode);
#else
		print_gif_error (GifLastError ());
#endif
		goto error;
	    }
      }

/* closing the GIF */
#if GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
    if (EGifCloseFile (GifFile, NULL) == GIF_ERROR)
#else
    if (EGifCloseFile (GifFile) == GIF_ERROR)
#endif
      {
#ifdef GIFLIB_MAJOR
	  print_gif_error (ErrorCode);
#else
	  print_gif_error (GifLastError ());
#endif
	  goto error;
      }

    free (ScanLine);
#ifdef GIFLIB_MAJOR
    GifFreeMapObject (ColorMap);
#else
    FreeMapObject (ColorMap);
#endif
    *gif = membuf.buffer;
    *gif_size = membuf.size;
    return RL2_OK;
  error:
    if (GifFile != NULL)
#if GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
	EGifCloseFile (GifFile, NULL);
#else
	EGifCloseFile (GifFile);
#endif
    if (ScanLine != NULL)
	free (ScanLine);
    if (ColorMap != NULL)
      {
#ifdef GIFLIB_MAJOR
	  GifFreeMapObject (ColorMap);
#else
	  FreeMapObject (ColorMap);
#endif
      }
    if (membuf.buffer != NULL)
	free (membuf.buffer);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_section_to_gif (rl2SectionPtr scn, const char *path)
{
/* attempting to save a raster section into a GIF file */
    int blob_size;
    unsigned char *blob;
    rl2RasterPtr rst;
    int ret;

    if (scn == NULL)
	return RL2_ERROR;
    rst = rl2_get_section_raster (scn);
    if (rst == NULL)
	return RL2_ERROR;
/* attempting to export as a GIF image */
    if (rl2_raster_to_gif (rst, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    ret = rl2_blob_to_file (path, blob, blob_size);
    free (blob);
    if (ret != RL2_OK)
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_to_gif (rl2RasterPtr rst, unsigned char **gif, int *gif_size)
{
/* creating a GIF image from a raster */
    unsigned char *blob;
    int blob_size;

    if (rst == NULL)
	return RL2_ERROR;

    if (compress_gif (rst, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    *gif = blob;
    *gif_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE rl2SectionPtr
rl2_section_from_gif (const char *path)
{
/* attempting to create a raster section from a GIF file */
    int blob_size;
    unsigned char *blob;
    rl2SectionPtr scn;
    rl2RasterPtr rst;

/* attempting to create a raster */
    if (rl2_blob_from_file (path, &blob, &blob_size) != RL2_OK)
	return NULL;
    rst = rl2_raster_from_gif (blob, blob_size);
    free (blob);
    if (rst == NULL)
	return NULL;

/* creating the raster section */
    scn =
	rl2_create_section (path, RL2_COMPRESSION_GIF, RL2_TILESIZE_UNDEFINED,
			    RL2_TILESIZE_UNDEFINED, rst);
    return scn;
}

RL2_DECLARE rl2RasterPtr
rl2_raster_from_gif (const unsigned char *gif, int gif_size)
{
/* attempting to create a raster from a GIF image */
    rl2RasterPtr rst = NULL;
    unsigned int width;
    unsigned int height;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char *data = NULL;
    int data_size;
    rl2PalettePtr palette = NULL;

    if (rl2_decode_gif
	(gif, gif_size, &width, &height, &sample_type, &pixel_type, &data,
	 &data_size, &palette) != RL2_OK)
	goto error;
    rst =
	rl2_create_raster (width, height, sample_type, pixel_type, 1, data,
			   data_size, palette, NULL, 0, NULL);
    if (rst == NULL)
	goto error;
    return rst;

  error:
    if (data != NULL)
	free (data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return NULL;
}

RL2_PRIVATE int
rl2_decode_gif (const unsigned char *gif, int gif_size, unsigned int *xwidth,
		unsigned int *xheight, unsigned char *xsample_type,
		unsigned char *xpixel_type, unsigned char **blob,
		int *blob_sz, rl2PalettePtr * palette)
{
/* attempting to create a raster from a GIF image - raw block */
    struct gif_memory_buffer membuf;
#ifdef GIFLIB_MAJOR
    int ErrorCode;
#endif
    GifFileType *GifFile = NULL;
    GifPixelType *Line;
    GifRecordType RecordType;
    GifByteType *Extension;
    ColorMapObject *ColorMap;
    int ExtCode;
    unsigned int width;
    unsigned int height;
    int row;
    int col;
    int i;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_PALETTE;
    unsigned char sample_typ;
    unsigned char pixel_typ;
    rl2PalettePtr rl_palette = NULL;
    int nPalette = 0;
    unsigned char *data = NULL;
    unsigned char *p_data;
    int data_size;
    int already_done = 0;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];

    membuf.buffer = (unsigned char *) gif;
    membuf.size = gif_size;
    membuf.off = 0;

/* opening the GIF */
#ifdef GIFLIB_MAJOR
    GifFile = DGifOpen (&membuf, readGif, &ErrorCode);
#else
    GifFile = DGifOpen (&membuf, readGif);
#endif
    if (GifFile == NULL)
      {
#ifdef GIFLIB_MAJOR
	  print_gif_error (ErrorCode);
#else
	  print_gif_error (GifLastError ());
#endif
	  return RL2_ERROR;
      }

    do
      {
	  /* looping on GIF chunks */
	  if (DGifGetRecordType (GifFile, &RecordType) == GIF_ERROR)
	    {
#ifdef GIFLIB_MAJOR
		print_gif_error (GifFile->Error);
#else
		print_gif_error (GifLastError ());
#endif
		goto error;
	    }
	  switch (RecordType)
	    {
	    case IMAGE_DESC_RECORD_TYPE:
		/* we have a valid image */
		if (DGifGetImageDesc (GifFile) == GIF_ERROR)
		  {
#ifdef GIFLIB_MAJOR
		      print_gif_error (GifFile->Error);
#else
		      print_gif_error (GifLastError ());
#endif
		      goto error;
		  }
		if (already_done)
		  {
		      /* if multiple images are found, simply the first one will be imported */
		      break;
		  }
		/* width and height */
		width = GifFile->Image.Width;
		height = GifFile->Image.Height;
		ColorMap = GifFile->SColorMap;
		if (ColorMap->BitsPerPixel == 1)
		    sample_type = RL2_SAMPLE_1_BIT;
		else if (ColorMap->BitsPerPixel == 2)
		    sample_type = RL2_SAMPLE_2_BIT;
		else if (ColorMap->BitsPerPixel <= 4)
		    sample_type = RL2_SAMPLE_4_BIT;
		else
		    sample_type = RL2_SAMPLE_UINT8;
		/* palette */
		ColorMap = GifFile->SColorMap;
		if (ColorMap == NULL)
		    goto error;
		nPalette = ColorMap->ColorCount;
		rl_palette = rl2_create_palette (nPalette);
		if (rl_palette == NULL)
		    goto error;
		for (i = 0; i < nPalette; i++)
		  {
		      red[i] = ColorMap->Colors[i].Red;
		      green[i] = ColorMap->Colors[i].Green;
		      blue[i] = ColorMap->Colors[i].Blue;
		  }
		/* creating the raster data */
		data_size = width * height;
		data = malloc (data_size);
		if (data == NULL)
		    goto error;
		p_data = data;
		Line = (GifPixelType *) malloc (GifFile->Image.Width *
						sizeof (GifPixelType));
		for (row = 0; row < GifFile->Image.Height; row++)
		  {
		      if (DGifGetLine (GifFile, Line, GifFile->Image.Width)
			  == GIF_ERROR)
			{
#ifdef GIFLIB_MAJOR
			    print_gif_error (GifFile->Error);
#else
			    print_gif_error (GifLastError ());
#endif
			    fprintf (stderr, "err GIF %d / %d %d\n", row,
				     GifFile->Image.Height,
				     GifFile->Image.Width);
			    goto error;
			}
		      for (col = 0; col < GifFile->Image.Width; col++)
			  *p_data++ = Line[col];
		  }
		free (Line);
		/* exporting data to be returned */
		already_done = 1;
		break;
	    case EXTENSION_RECORD_TYPE:
		if (DGifGetExtension (GifFile, &ExtCode, &Extension) ==
		    GIF_ERROR)
		  {
#ifdef GIFLIB_MAJOR
		      print_gif_error (GifFile->Error);
#else
		      print_gif_error (GifLastError ());
#endif
		      goto error;
		  }
		for (;;)
		  {
		      if (DGifGetExtensionNext (GifFile, &Extension) ==
			  GIF_ERROR)
			{
#ifdef GIFLIB_MAJOR
			    print_gif_error (GifFile->Error);
#else
			    print_gif_error (GifLastError ());
#endif
			    goto error;
			}
		      if (Extension == NULL)
			  break;
		  }
		break;
	    case TERMINATE_RECORD_TYPE:
		break;
	    default:		/* Should be trapped by DGifGetRecordType */
		break;
	    }
      }
    while (RecordType != TERMINATE_RECORD_TYPE);

/* closing the GIF */
#if GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
    if (DGifCloseFile (GifFile, NULL) == GIF_ERROR)
#else
    if (DGifCloseFile (GifFile) == GIF_ERROR)
#endif
      {
#ifdef GIFLIB_MAJOR
	  print_gif_error (GifFile->Error);
#else
	  print_gif_error (GifLastError ());
#endif
	  goto error;
      }
    if (already_done)
      {
	  for (i = 0; i < nPalette; i++)
	      rl2_set_palette_color (rl_palette, i, red[i], green[i], blue[i]);
	  if (rl2_get_palette_type (rl_palette, &sample_typ, &pixel_typ) !=
	      RL2_OK)
	      goto error;
	  *xwidth = width;
	  *xheight = height;
	  *xsample_type = sample_type;
	  *xpixel_type = pixel_type;
	  if (sample_type == sample_typ)
	      *xpixel_type = pixel_typ;
	  *blob = data;
	  *blob_sz = data_size;
	  *palette = rl_palette;
	  return RL2_OK;
      }
  error:
    if (GifFile != NULL)
#if GIFLIB_MAJOR >= 5 && GIFLIB_MINOR >= 1
	DGifCloseFile (GifFile, NULL);
#else
	DGifCloseFile (GifFile);
#endif
    if (rl_palette != NULL)
	rl2_destroy_palette (rl_palette);
    if (data != NULL)
	free (data);
    *palette = NULL;
    return RL2_ERROR;
}
