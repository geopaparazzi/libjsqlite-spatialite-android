/*

 rl2tiff -- TIFF / GeoTIFF related functions

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
#include <float.h>
#include <inttypes.h>

#include "rasterlite2/sqlite.h"

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"
#include "rasterlite2/rl2tiff.h"

struct memfile
{
/* a struct emulating a file [memory mapped] */
    unsigned char *buffer;
    int malloc_block;
    tsize_t size;
    tsize_t eof;
    toff_t current;
};

static void
memory_realloc (struct memfile *mem, tsize_t req_size)
{
/* expanding the allocated memory */
    unsigned char *new_buffer;
    tsize_t new_size = mem->size;
    while (new_size <= req_size)
	new_size += mem->malloc_block;
    new_buffer = realloc (mem->buffer, new_size);
    if (!new_buffer)
	return;
    mem->buffer = new_buffer;
    memset (mem->buffer + mem->size, '\0', new_size - mem->size);
    mem->size = new_size;
}

static tsize_t
memory_readproc (thandle_t clientdata, tdata_t data, tsize_t size)
{
/* emulating the read()  function */
    struct memfile *mem = clientdata;
    tsize_t len;
    if (mem->current >= (toff_t) mem->eof)
	return 0;
    len = size;
    if ((mem->current + size) >= (toff_t) mem->eof)
	len = (tsize_t) (mem->eof - mem->current);
    memcpy (data, mem->buffer + mem->current, len);
    mem->current += len;
    return len;
}

static tsize_t
memory_writeproc (thandle_t clientdata, tdata_t data, tsize_t size)
{
/* emulating the write()  function */
    struct memfile *mem = clientdata;
    if ((mem->current + size) >= (toff_t) mem->size)
	memory_realloc (mem, mem->current + size);
    if ((mem->current + size) >= (toff_t) mem->size)
	return -1;
    memcpy (mem->buffer + mem->current, (unsigned char *) data, size);
    mem->current += size;
    if (mem->current > (toff_t) mem->eof)
	mem->eof = (tsize_t) (mem->current);
    return size;
}

static toff_t
memory_seekproc (thandle_t clientdata, toff_t offset, int whence)
{
/* emulating the lseek()  function */
    struct memfile *mem = clientdata;
    switch (whence)
      {
      case SEEK_CUR:
	  if ((int) (mem->current + offset) < 0)
	      return (toff_t) - 1;
	  mem->current += offset;
	  if ((toff_t) mem->eof < mem->current)
	      mem->eof = (tsize_t) (mem->current);
	  break;
      case SEEK_END:
	  if ((int) (mem->eof + offset) < 0)
	      return (toff_t) - 1;
	  mem->current = mem->eof + offset;
	  if ((toff_t) mem->eof < mem->current)
	      mem->eof = (tsize_t) (mem->current);
	  break;
      case SEEK_SET:
      default:
	  if ((int) offset < 0)
	      return (toff_t) - 1;
	  mem->current = offset;
	  if ((toff_t) mem->eof < mem->current)
	      mem->eof = (tsize_t) (mem->current);
	  break;
      };
    return mem->current;
}

static int
closeproc (thandle_t clientdata)
{
/* emulating the close()  function */
    if (clientdata)
	return 0;		/* does absolutely nothing - required in order to suppress warnings */
    return 0;
}

static toff_t
memory_sizeproc (thandle_t clientdata)
{
/* returning the pseudo-file current size */
    struct memfile *mem = clientdata;
    return mem->eof;
}

static int
mapproc (thandle_t clientdata, tdata_t * data, toff_t * offset)
{
    if (clientdata || data || offset)
	return 0;		/* does absolutely nothing - required in order to suppress warnings */
    return 0;
}

static void
unmapproc (thandle_t clientdata, tdata_t data, toff_t offset)
{
    if (clientdata || data || offset)
	return;			/* does absolutely nothing - required in order to suppress warnings */
    return;
}

static int
read_from_tiff (rl2PrivTiffOriginPtr origin, unsigned short width,
		unsigned short height, unsigned char sample_type,
		unsigned char pixel_type, unsigned char num_bands,
		unsigned int startRow, unsigned int startCol,
		unsigned char **pixels, int *pixels_sz, rl2PalettePtr palette);

static rl2PrivTiffOriginPtr
create_tiff_origin (const char *path, unsigned char force_sample_type,
		    unsigned char force_pixel_type,
		    unsigned char force_num_bands)
{
/* creating an uninitialized TIFF origin */
    int len;
    rl2PrivTiffOriginPtr origin;
    if (path == NULL)
	return NULL;
    origin = malloc (sizeof (rl2PrivTiffOrigin));
    if (origin == NULL)
	return NULL;

    len = strlen (path);
    origin->path = malloc (len + 1);
    strcpy (origin->path, path);
    origin->tfw_path = NULL;
    origin->isGeoTiff = 0;
    origin->in = (TIFF *) 0;
    origin->tileWidth = 0;
    origin->tileHeight = 0;
    origin->rowsPerStrip = 0;
    origin->maxPalette = 0;
    origin->red = NULL;
    origin->green = NULL;
    origin->blue = NULL;
    origin->remapMaxPalette = 0;
    origin->remapRed = NULL;
    origin->remapGreen = NULL;
    origin->remapBlue = NULL;
    origin->isGeoReferenced = 0;
    origin->Srid = -1;
    origin->srsName = NULL;
    origin->proj4text = NULL;
    origin->forced_sample_type = force_sample_type;
    origin->forced_pixel_type = force_pixel_type;
    origin->forced_num_bands = force_num_bands;
    origin->forced_conversion = RL2_CONVERT_NO;
    return origin;
}

static int
alloc_palette (rl2PrivTiffOriginPtr tiff, int max_palette)
{
/* allocating an empty Palette */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    int i;

    if (origin == NULL)
	return 0;
    if (max_palette < 1 || max_palette > 256)
	return 0;
    origin->maxPalette = max_palette;
    origin->red = malloc (max_palette);
    if (origin->red == NULL)
	return 0;
    origin->green = malloc (max_palette);
    if (origin->green == NULL)
      {
	  free (origin->red);
	  return 0;
      }
    origin->blue = malloc (max_palette);
    if (origin->blue == NULL)
      {
	  free (origin->red);
	  free (origin->green);
	  return 0;
      }
    for (i = 0; i < max_palette; i++)
      {
	  origin->red[i] = 0;
	  origin->green[i] = 0;
	  origin->blue[i] = 0;
      }
    return 1;
}

RL2_DECLARE void
rl2_destroy_tiff_origin (rl2TiffOriginPtr tiff)
{
/* memory cleanup - destroying a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return;
    if (origin->in != (TIFF *) 0)
	TIFFClose (origin->in);
    if (origin->path != NULL)
	free (origin->path);
    if (origin->tfw_path != NULL)
	free (origin->tfw_path);
    if (origin->red != NULL)
	free (origin->red);
    if (origin->green != NULL)
	free (origin->green);
    if (origin->blue != NULL)
	free (origin->blue);
    if (origin->remapRed != NULL)
	free (origin->remapRed);
    if (origin->remapGreen != NULL)
	free (origin->remapGreen);
    if (origin->remapBlue != NULL)
	free (origin->remapBlue);
    if (origin->srsName != NULL)
	free (origin->srsName);
    if (origin->proj4text != NULL)
	free (origin->proj4text);
    free (origin);
}

static void
origin_set_tfw_path (const char *path, const char *suffix,
		     rl2PrivTiffOriginPtr origin)
{
/* building the TFW path (WorldFile) */
    if (origin->tfw_path != NULL)
	free (origin->tfw_path);
    origin->tfw_path = NULL;
    origin->tfw_path = rl2_build_worldfile_path (path, suffix);
}

static int
is_valid_float (char *str)
{
/* testing for a valid worldfile float value */
    char *p = str;
    int point = 0;
    int sign = 0;
    int digit = 0;
    int len = strlen (str);
    int i;

    for (i = len - 1; i >= 0; i--)
      {
	  /* suppressing any trailing blank */
	  if (str[i] == ' ' || str[i] == '\t' || str[i] == '\r')
	      str[i] = '\0';
	  else
	      break;
      }

    while (1)
      {
	  /* skipping leading blanks */
	  if (*p != ' ' && *p != '\t')
	      break;
	  p++;
      }

/* testing for a well formatted float */
    while (1)
      {
	  if (*p == '\0')
	      break;
	  if (*p >= '0' && *p <= '9')
	      digit++;
	  else if (*p == '.')
	      point++;
	  else if (*p == ',')
	    {
		*p = '.';
		point++;
	    }
	  else if (*p == '+' || *p == '-')
	    {
		if (digit == 0 && point == 0 && sign == 0)
		    ;
		else
		    return 0;
	    }
	  else
	      return 0;
	  p++;
      }
    if (digit > 0 && sign <= 1 && point <= 1)
	return 1;
    return 0;
}

RL2_PRIVATE int
parse_worldfile (FILE * in, double *px, double *py, double *pres_x,
		 double *pres_y)
{
/* attemtping to parse a WorldFile */
    int line_no = 0;
    int ok_res_x = 0;
    int ok_res_y = 0;
    int ok_x = 0;
    int ok_y = 0;
    char buf[1024];
    char *p = buf;
    double x = 0.0;
    double y = 0.0;
    double res_x = 0.0;
    double res_y = 0.0;

    if (in == NULL)
	return 0;

    while (1)
      {
	  int c = getc (in);
	  if (c == '\n' || c == EOF)
	    {
		*p = '\0';
		if (line_no == 0)
		  {
		      if (is_valid_float (buf))
			{
			    res_x = atof (buf);
			    ok_res_x = 1;
			}
		  }
		if (line_no == 3)
		  {
		      if (is_valid_float (buf))
			{
			    res_y = atof (buf) * -1.0;
			    ok_res_y = 1;
			}
		  }
		if (line_no == 4)
		  {
		      if (is_valid_float (buf))
			{
			    x = atof (buf);
			    ok_x = 1;
			}
		  }
		if (line_no == 5)
		  {
		      if (is_valid_float (buf))
			{
			    y = atof (buf);
			    ok_y = 1;
			}
		  }
		if (c == EOF)
		    break;
		p = buf;
		line_no++;
		continue;
	    }
	  *p++ = c;
      }

    if (ok_x && ok_y && ok_res_x && ok_res_y)
      {
	  *px = x;
	  *py = y;
	  *pres_x = res_x;
	  *pres_y = res_y;
	  return 1;
      }
    return 0;
}

static void
worldfile_tiff_origin (const char *path, rl2PrivTiffOriginPtr origin, int srid)
{
/* attempting to retrieve georeferencing from a TIFF+TFW origin */
    FILE *tfw = NULL;
    double res_x;
    double res_y;
    double x;
    double y;

    origin_set_tfw_path (path, ".tfw", origin);
    tfw = fopen (origin->tfw_path, "r");
    if (tfw == NULL)
      {
	  /* trying the ".tifw" suffix */
	  origin_set_tfw_path (path, ".tifw", origin);
	  tfw = fopen (origin->tfw_path, "r");
      }
    if (tfw == NULL)
      {
	  /* trying the ".wld" suffix */
	  origin_set_tfw_path (path, ".wld", origin);
	  tfw = fopen (origin->tfw_path, "r");
      }
    if (tfw == NULL)
	goto error;
    if (!parse_worldfile (tfw, &x, &y, &res_x, &res_y))
	goto error;
    fclose (tfw);
    origin->Srid = srid;
    origin->hResolution = res_x;
    origin->vResolution = res_y;
    origin->minX = x;
    origin->maxY = y;
    origin->isGeoReferenced = 1;
    return;

  error:
    if (tfw != NULL)
	fclose (tfw);
    if (origin->tfw_path != NULL)
	free (origin->tfw_path);
    origin->tfw_path = NULL;
}

static void
recover_incomplete_geotiff (rl2PrivTiffOriginPtr origin, TIFF * in,
			    uint32 width, uint32 height, int force_srid)
{
/* final desperate attempt to recover an imcomplete GeoTIFF */
    double *tie_points;
    double *scale;
    uint16 count;
    double res_x = DBL_MAX;
    double res_y = DBL_MAX;
    double x = DBL_MAX;
    double y = DBL_MAX;


    if (force_srid <= 0)
	return;

    if (TIFFGetField (in, TIFFTAG_GEOPIXELSCALE, &count, &scale))
      {
	  if (count >= 2 && scale[0] != 0.0 && scale[1] != 0.0)
	    {
		res_x = scale[0];
		res_y = scale[1];
	    }
      }
    if (TIFFGetField (in, TIFFTAG_GEOTIEPOINTS, &count, &tie_points))
      {
	  int i;
	  int max_count = count / 6;
	  for (i = 0; i < max_count; i++)
	    {
		x = tie_points[i * 6 + 3];
		y = tie_points[i * 6 + 4];
	    }
      }
    if (x == DBL_MAX || y == DBL_MAX || res_x == DBL_MAX || res_y == DBL_MAX)
	return;

/* computing the pixel resolution */
    origin->Srid = force_srid;
    origin->minX = x;
    origin->maxX = x + ((double) width * res_x);
    origin->minY = y - ((double) height * res_y);
    origin->maxY = y;
    origin->hResolution = (origin->maxX - origin->minX) / (double) width;
    origin->vResolution = (origin->maxY - origin->minY) / (double) height;
    origin->isGeoReferenced = 1;
    origin->isGeoTiff = 1;
}

static void
geo_tiff_origin (const char *path, rl2PrivTiffOriginPtr origin, int force_srid)
{
/* attempting to retrieve georeferencing from a GeoTIFF origin */
    uint32 width = 0;
    uint32 height = 0;
    double cx;
    double cy;
    GTIFDefn definition;
    char *pString;
    int len;
    int basic = 0;
    short pixel_mode = RasterPixelIsArea;
    TIFF *in = (TIFF *) 0;
    GTIF *gtif = (GTIF *) 0;

/* suppressing TIFF messages */
    TIFFSetErrorHandler (NULL);
    TIFFSetWarningHandler (NULL);

/* reading from file */
    in = XTIFFOpen (path, "r");
    if (in == NULL)
	goto error;
    gtif = GTIFNew (in);
    if (gtif == NULL)
	goto error;

/* retrieving the TIFF dimensions */
    TIFFGetField (in, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField (in, TIFFTAG_IMAGEWIDTH, &width);
    basic = 1;


    if (!GTIFGetDefn (gtif, &definition))
	goto error;
/* retrieving the EPSG code */
    if (definition.PCS == 32767)
      {
	  if (definition.GCS != 32767)
	      origin->Srid = definition.GCS;
      }
    else
	origin->Srid = definition.PCS;
    if (origin->Srid <= 0)
      {
	  origin->Srid = force_srid;
	  if (origin->Srid <= 0)
	      goto error;
      }
    if (definition.PCS == 32767)
      {
	  /* Get the GCS name if possible */
	  pString = NULL;
	  GTIFGetGCSInfo (definition.GCS, &pString, NULL, NULL, NULL);
	  if (pString != NULL)
	    {
		len = strlen (pString);
		origin->srsName = malloc (len + 1);
		strcpy (origin->srsName, pString);
		CPLFree (pString);
	    }
      }
    else
      {
	  /* Get the PCS name if possible */
	  pString = NULL;
	  GTIFGetPCSInfo (definition.PCS, &pString, NULL, NULL, NULL);
	  if (pString != NULL)
	    {
		len = strlen (pString);
		origin->srsName = malloc (len + 1);
		strcpy (origin->srsName, pString);
		CPLFree (pString);
	    }
      }
/* retrieving the PROJ.4 params */
    pString = GTIFGetProj4Defn (&definition);
    if (pString != NULL)
      {
	  len = strlen (pString);
	  origin->proj4text = malloc (len + 1);
	  strcpy (origin->proj4text, pString);
	  CPLFree (pString);
      }

/* computing the corners coords */
    cx = 0.0;
    cy = 0.0;
    GTIFImageToPCS (gtif, &cx, &cy);
    origin->minX = cx;
    origin->maxY = cy;
    cx = 0.0;
    cy = height;
    GTIFImageToPCS (gtif, &cx, &cy);
    origin->minY = cy;
    cx = width;
    cy = 0.0;
    GTIFImageToPCS (gtif, &cx, &cy);
    origin->maxX = cx;

/* computing the pixel resolution */
    origin->hResolution = (origin->maxX - origin->minX) / (double) width;
    origin->vResolution = (origin->maxY - origin->minY) / (double) height;
    origin->isGeoReferenced = 1;
    origin->isGeoTiff = 1;

/* retrieving GTRasterTypeGeoKey */
    if (!GTIFKeyGet (gtif, GTRasterTypeGeoKey, &pixel_mode, 0, 1))
	pixel_mode = RasterPixelIsArea;
    if (pixel_mode == RasterPixelIsPoint)
      {
	  /* adjusting the BBOX */
	  origin->minX -= origin->hResolution / 2.0;
	  origin->minY -= origin->vResolution / 2.0;
	  origin->maxX += origin->hResolution / 2.0;
	  origin->maxY += origin->vResolution / 2.0;

      }

  error:
    if (basic && origin->isGeoTiff == 0)
	recover_incomplete_geotiff (origin, in, width, height, force_srid);
    if (in != (TIFF *) 0)
	XTIFFClose (in);
    if (gtif != (GTIF *) 0)
	GTIFFree (gtif);
}

static int
check_grayscale_palette (rl2PrivTiffOriginPtr origin)
{
/* checking if a Palette actually is a Grayscale Palette */
    int i;
    if (origin->maxPalette == 0 || origin->maxPalette > 256)
	return 0;
    for (i = 0; i < origin->maxPalette; i++)
      {
	  if (origin->red[i] == origin->green[i]
	      && origin->red[i] == origin->blue[i])
	      ;
	  else
	      return 0;
      }
    return 1;
}

static int
check_tiff_origin_pixel_conversion (rl2PrivTiffOriginPtr origin)
{
/* check if the Color-Space conversion could be accepted */
    if (origin->forced_sample_type == RL2_SAMPLE_UNKNOWN)
      {
	  if (origin->sampleFormat == SAMPLEFORMAT_INT)
	    {
		if (origin->bitsPerSample == 8)
		    origin->forced_sample_type = RL2_SAMPLE_INT8;
		if (origin->bitsPerSample == 16)
		    origin->forced_sample_type = RL2_SAMPLE_INT16;
		if (origin->bitsPerSample == 32)
		    origin->forced_sample_type = RL2_SAMPLE_INT32;
	    }
	  else if (origin->sampleFormat == SAMPLEFORMAT_UINT)
	    {
		if (origin->bitsPerSample == 1)
		    origin->forced_sample_type = RL2_SAMPLE_1_BIT;
		if (origin->bitsPerSample == 2)
		    origin->forced_sample_type = RL2_SAMPLE_2_BIT;
		if (origin->bitsPerSample == 4)
		    origin->forced_sample_type = RL2_SAMPLE_4_BIT;
		if (origin->bitsPerSample == 8)
		    origin->forced_sample_type = RL2_SAMPLE_UINT8;
		if (origin->bitsPerSample == 16)
		    origin->forced_sample_type = RL2_SAMPLE_UINT16;
		if (origin->bitsPerSample == 32)
		    origin->forced_sample_type = RL2_SAMPLE_UINT32;
	    }
	  else if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP)
	    {
		if (origin->bitsPerSample == 32)
		    origin->forced_sample_type = RL2_SAMPLE_FLOAT;
		if (origin->bitsPerSample == 64)
		    origin->forced_sample_type = RL2_SAMPLE_DOUBLE;
	    }
      }
    if (origin->forced_pixel_type == RL2_PIXEL_UNKNOWN)
      {
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->photometric == PHOTOMETRIC_RGB)
	    {
		if (origin->samplesPerPixel == 3)
		    origin->forced_pixel_type = RL2_PIXEL_RGB;
		else
		    origin->forced_pixel_type = RL2_PIXEL_MULTIBAND;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample > 1)
	      origin->forced_pixel_type = RL2_PIXEL_GRAYSCALE;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric == PHOTOMETRIC_PALETTE
	      && origin->bitsPerSample <= 8)
	      origin->forced_pixel_type = RL2_PIXEL_PALETTE;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric <= PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 1)
	      origin->forced_pixel_type = RL2_PIXEL_MONOCHROME;
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 8)
	      origin->forced_pixel_type = RL2_PIXEL_DATAGRID;
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 16)
	      origin->forced_pixel_type = RL2_PIXEL_DATAGRID;
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 32)
	      origin->forced_pixel_type = RL2_PIXEL_DATAGRID;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 32)
	      origin->forced_pixel_type = RL2_PIXEL_DATAGRID;
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 32)
	      origin->forced_pixel_type = RL2_PIXEL_DATAGRID;
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 64)
	      origin->forced_pixel_type = RL2_PIXEL_DATAGRID;
      }
    if (origin->forced_num_bands == RL2_BANDS_UNKNOWN)
	origin->forced_num_bands = origin->samplesPerPixel;

    if (origin->forced_sample_type == RL2_SAMPLE_UINT8
	&& origin->forced_pixel_type == RL2_PIXEL_RGB
	&& origin->forced_num_bands == 3)
      {
	  /* RGB color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 3
	      && origin->photometric == PHOTOMETRIC_RGB)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB)
	    {
		origin->forced_conversion = RL2_CONVERT_GRAYSCALE_TO_RGB;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric == PHOTOMETRIC_PALETTE
	      && origin->bitsPerSample <= 8)
	    {
		origin->forced_conversion = RL2_CONVERT_PALETTE_TO_RGB;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT16
	&& origin->forced_pixel_type == RL2_PIXEL_RGB
	&& origin->forced_num_bands == 3)
      {
	  /* RGB color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 3
	      && origin->photometric == PHOTOMETRIC_RGB)
	      return 1;
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT8
	&& origin->forced_pixel_type == RL2_PIXEL_MULTIBAND)
      {
	  /* MULTIBAND color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == origin->forced_num_bands
	      && origin->photometric == PHOTOMETRIC_RGB)
	      return 1;
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT16
	&& origin->forced_pixel_type == RL2_PIXEL_MULTIBAND)
      {
	  /* MULTIBAND color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == origin->forced_num_bands
	      && origin->photometric == PHOTOMETRIC_RGB)
	      return 1;
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT8
	&& origin->forced_pixel_type == RL2_PIXEL_GRAYSCALE
	&& origin->forced_num_bands == 1)
      {
	  /* GRAYSCALE color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample > 1)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 3
	      && origin->photometric == PHOTOMETRIC_RGB)
	    {
		origin->forced_conversion = RL2_CONVERT_RGB_TO_GRAYSCALE;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric == PHOTOMETRIC_PALETTE
	      && origin->bitsPerSample <= 8 && check_grayscale_palette (origin))
	    {
		origin->forced_conversion = RL2_CONVERT_PALETTE_TO_GRAYSCALE;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT16
	&& origin->forced_pixel_type == RL2_PIXEL_GRAYSCALE
	&& origin->forced_num_bands == 1)
      {
	  /* GRAYSCALE color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB
	      && origin->bitsPerSample > 1)
	      return 1;
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT8
	&& origin->forced_pixel_type == RL2_PIXEL_PALETTE
	&& origin->forced_num_bands == 1)
      {
	  /* PALETTE-256 color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric == PHOTOMETRIC_PALETTE
	      && origin->bitsPerSample <= 8)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric < PHOTOMETRIC_RGB)
	    {
		origin->forced_conversion = RL2_CONVERT_GRAYSCALE_TO_PALETTE;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_4_BIT
	&& origin->forced_pixel_type == RL2_PIXEL_PALETTE
	&& origin->forced_num_bands == 1)
      {
	  /* PALETTE-16 color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric == PHOTOMETRIC_PALETTE
	      && origin->bitsPerSample <= 8)
	      return 1;
      }
    if (origin->forced_sample_type == RL2_SAMPLE_2_BIT
	&& origin->forced_pixel_type == RL2_PIXEL_PALETTE
	&& origin->forced_num_bands == 1)
      {
	  /* PALETTE-4 color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric == PHOTOMETRIC_PALETTE
	      && origin->bitsPerSample <= 8)
	      return 1;
      }
    if (origin->forced_sample_type == RL2_SAMPLE_1_BIT
	&& origin->forced_pixel_type == RL2_PIXEL_PALETTE
	&& origin->forced_num_bands == 1)
      {
	  /* PALETTE-2 color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric == PHOTOMETRIC_PALETTE
	      && origin->bitsPerSample <= 8)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric <= PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 1)
	    {
		origin->forced_conversion = RL2_CONVERT_MONOCHROME_TO_PALETTE;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_1_BIT
	&& origin->forced_pixel_type == RL2_PIXEL_MONOCHROME
	&& origin->forced_num_bands == 1)
      {
	  /* MONOCHROME color-space */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric <= PHOTOMETRIC_RGB
	      && origin->bitsPerSample == 1)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1
	      && origin->photometric == PHOTOMETRIC_PALETTE
	      && origin->bitsPerSample <= 8)
	    {
		origin->forced_conversion = RL2_CONVERT_PALETTE_TO_MONOCHROME;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_INT8
	&& origin->forced_pixel_type == RL2_PIXEL_DATAGRID
	&& origin->forced_num_bands == 1)
      {
	  /* INT8 datagrid */
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT8_TO_INT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT16_TO_INT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT16_TO_INT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT32_TO_INT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT32_TO_INT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_FLOAT_TO_INT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 64)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_DOUBLE_TO_INT8;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT8
	&& origin->forced_pixel_type == RL2_PIXEL_DATAGRID
	&& origin->forced_num_bands == 1)
      {
	  /* UINT8 datagrid */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT8_TO_UINT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT16_TO_UINT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT16_TO_UINT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT32_TO_UINT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT32_TO_UINT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_FLOAT_TO_UINT8;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 64)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_DOUBLE_TO_UINT8;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_INT16
	&& origin->forced_pixel_type == RL2_PIXEL_DATAGRID
	&& origin->forced_num_bands == 1)
      {
	  /* INT16 datagrid */
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT16_TO_INT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT8_TO_INT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT8_TO_INT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT32_TO_INT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT32_TO_INT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_FLOAT_TO_INT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 64)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_DOUBLE_TO_INT16;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT16
	&& origin->forced_pixel_type == RL2_PIXEL_DATAGRID
	&& origin->forced_num_bands == 1)
      {
	  /* UINT16 datagrid */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT16_TO_UINT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT8_TO_UINT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT8_TO_UINT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT32_TO_UINT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT32_TO_UINT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_FLOAT_TO_UINT16;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 64)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_DOUBLE_TO_UINT16;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_INT32
	&& origin->forced_pixel_type == RL2_PIXEL_DATAGRID
	&& origin->forced_num_bands == 1)
      {
	  /* INT32 datagrid */
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT32_TO_INT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT8_TO_INT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT8_TO_INT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT16_TO_INT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT16_TO_INT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_FLOAT_TO_INT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 64)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_DOUBLE_TO_INT32;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_UINT32
	&& origin->forced_pixel_type == RL2_PIXEL_DATAGRID
	&& origin->forced_num_bands == 1)
      {
	  /* UINT32 datagrid */
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT32_TO_UINT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT8_TO_UINT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT8_TO_UINT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT16_TO_UINT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT16_TO_UINT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_FLOAT_TO_UINT32;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 64)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_DOUBLE_TO_UINT32;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_FLOAT
	&& origin->forced_pixel_type == RL2_PIXEL_DATAGRID
	&& origin->forced_num_bands == 1)
      {
	  /* FLOAT datagrid */
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT8_TO_FLOAT;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT16_TO_FLOAT;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT32_TO_FLOAT;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT8_TO_FLOAT;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT16_TO_FLOAT;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT32_TO_FLOAT;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 64)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_DOUBLE_TO_FLOAT;
		return 1;
	    }
      }
    if (origin->forced_sample_type == RL2_SAMPLE_DOUBLE
	&& origin->forced_pixel_type == RL2_PIXEL_DATAGRID
	&& origin->forced_num_bands == 1)
      {
	  /* DOUBLE datagrid */
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 64)
	      return 1;
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT8_TO_DOUBLE;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT16_TO_DOUBLE;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_INT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_INT32_TO_DOUBLE;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 8)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT8_TO_DOUBLE;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 16)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT16_TO_DOUBLE;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_UINT32_TO_DOUBLE;
		return 1;
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP
	      && origin->samplesPerPixel == 1 && origin->bitsPerSample == 32)
	    {
		origin->forced_conversion = RL2_CONVERT_GRID_FLOAT_TO_DOUBLE;
		return 1;
	    }
      }
    return 0;
}

static int
init_tiff_origin (const char *path, rl2PrivTiffOriginPtr origin)
{
/* attempting to initialize a file-based TIFF origin */
    uint32 value32;
    uint16 value16;

/* suppressing TIFF messages */
    TIFFSetErrorHandler (NULL);
    TIFFSetWarningHandler (NULL);

/* reading from file */
    origin->in = TIFFOpen (path, "r");
    if (origin->in == NULL)
	goto error;
/* tiled/striped layout */
    origin->isTiled = TIFFIsTiled (origin->in);

/* retrieving TIFF dimensions */
    TIFFGetField (origin->in, TIFFTAG_IMAGELENGTH, &value32);
    origin->height = value32;
    TIFFGetField (origin->in, TIFFTAG_IMAGEWIDTH, &value32);
    origin->width = value32;
    if (origin->isTiled)
      {
	  TIFFGetField (origin->in, TIFFTAG_TILEWIDTH, &value32);
	  origin->tileWidth = value32;
	  TIFFGetField (origin->in, TIFFTAG_TILELENGTH, &value32);
	  origin->tileHeight = value32;
      }
    else
      {
	  TIFFGetField (origin->in, TIFFTAG_ROWSPERSTRIP, &value32);
	  origin->rowsPerStrip = value32;
      }

    if (origin->isGeoReferenced && origin->isGeoTiff == 0)
      {
	  /* completing Worldfile georeferencing */
	  origin->maxX =
	      origin->minX + (origin->hResolution * (double) (origin->width));
	  origin->minY =
	      origin->maxY - (origin->vResolution * (double) (origin->height));
      }

/* retrieving pixel configuration */
    TIFFGetField (origin->in, TIFFTAG_BITSPERSAMPLE, &value16);
    origin->bitsPerSample = value16;
    if (TIFFGetField (origin->in, TIFFTAG_SAMPLESPERPIXEL, &value16) == 0)
      {
	  /* attempting to recover badly formatted TIFFs */
	  origin->samplesPerPixel = 1;
      }
    else
	origin->samplesPerPixel = value16;
    TIFFGetField (origin->in, TIFFTAG_PHOTOMETRIC, &value16);
    origin->photometric = value16;
    TIFFGetField (origin->in, TIFFTAG_COMPRESSION, &value16);
    origin->compression = value16;
    if (TIFFGetField (origin->in, TIFFTAG_SAMPLEFORMAT, &value16) == 0)
	origin->sampleFormat = SAMPLEFORMAT_UINT;
    else
	origin->sampleFormat = value16;
    if (TIFFGetField (origin->in, TIFFTAG_PLANARCONFIG, &value16) == 0)
      {
	  /* attempting to recover badly formatted TIFFs */
	  origin->planarConfig = PLANARCONFIG_CONTIG;
      }
    else
	origin->planarConfig = value16;

    if (origin->bitsPerSample == 16
	&& origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->planarConfig == PLANARCONFIG_SEPARATE)
	;
    else if (origin->bitsPerSample == 8
	     && origin->sampleFormat == SAMPLEFORMAT_UINT
	     && origin->planarConfig == PLANARCONFIG_SEPARATE)
	;
    else if (origin->planarConfig != PLANARCONFIG_CONTIG)
	goto error;

    if (origin->photometric == PHOTOMETRIC_PALETTE
	&& origin->sampleFormat == SAMPLEFORMAT_UINT)
      {
	  /* attempting to retrieve a Palette */
	  uint16 *red;
	  uint16 *green;
	  uint16 *blue;
	  int max_palette = 0;
	  int i;
	  switch (origin->bitsPerSample)
	    {
	    case 1:
		max_palette = 2;
		break;
	    case 2:
		max_palette = 4;
		break;
	    case 3:
		max_palette = 8;
		break;
	    case 4:
		max_palette = 16;
		break;
	    case 5:
		max_palette = 32;
		break;
	    case 6:
		max_palette = 64;
		break;
	    case 7:
		max_palette = 128;
		break;
	    case 8:
		max_palette = 256;
		break;
	    };
	  if (origin->forced_conversion == RL2_CONVERT_GRAYSCALE_TO_PALETTE)
	    {
		/* forced conversion: creating a GRAYSCALE palette */
		max_palette = 256;
		for (i = 0; i < max_palette; i++)
		  {
		      red[i] = i;
		      green[i] = i;
		      blue[i] = i;
		  }
	    }
	  if (max_palette == 0)
	      goto error;
	  if (!alloc_palette (origin, max_palette))
	      goto error;
	  if (TIFFGetField (origin->in, TIFFTAG_COLORMAP, &red, &green, &blue)
	      == 0)
	      goto error;
	  for (i = 0; i < max_palette; i++)
	    {
		if (red[i] < 256)
		    origin->red[i] = red[i];
		else
		    origin->red[i] = red[i] / 256;
		if (green[i] < 256)
		    origin->green[i] = green[i];
		else
		    origin->green[i] = green[i] / 256;
		if (blue[i] < 256)
		    origin->blue[i] = blue[i];
		else
		    origin->blue[i] = blue[i] / 256;
	    }
	  if ((origin->forced_sample_type == RL2_SAMPLE_1_BIT
	       || origin->forced_sample_type == RL2_SAMPLE_2_BIT
	       || origin->forced_sample_type == RL2_SAMPLE_4_BIT)
	      && origin->forced_pixel_type == RL2_PIXEL_PALETTE)
	    {
		int max = 0;
		unsigned char plt[256];
		int j;
		unsigned char *p;
		unsigned short x;
		unsigned short y;
		unsigned short row;
		unsigned short height = 256;
		if (origin->isTiled)
		    height = origin->tileHeight;
		for (row = 0; row < origin->height; row += height)
		  {
		      unsigned char *pixels = NULL;
		      int pixels_sz = 0;
		      rl2PalettePtr pltx = rl2_create_palette (max_palette);
		      for (j = 0; j < max_palette; j++)
			  rl2_set_palette_color (pltx, j, origin->red[j],
						 origin->green[j],
						 origin->blue[j]);
		      if (read_from_tiff
			  (origin, origin->width, height, RL2_SAMPLE_UINT8,
			   RL2_PIXEL_PALETTE, 1, row, 0, &pixels, &pixels_sz,
			   pltx) != RL2_OK)
			  goto error;
		      rl2_destroy_palette (pltx);
		      p = pixels;
		      for (y = 0; y < height; y++)
			{
			    if ((y + row) >= origin->height)
				break;
			    for (x = 0; x < origin->width; x++)
			      {
				  int ok = 0;
				  int index = *p++;
				  for (j = 0; j < max; j++)
				    {
					if (plt[j] == index)
					  {
					      ok = 1;
					      break;
					  }
				    }
				  if (!ok)
				      plt[max++] = index;
			      }
			}
		      free (pixels);
		  }
		if (origin->red != NULL)
		    free (origin->red);
		if (origin->green != NULL)
		    free (origin->green);
		if (origin->blue != NULL)
		    free (origin->blue);
		if (!alloc_palette (origin, max))
		    goto error;
		for (j = 0; j < max; j++)
		  {
		      /* normalized palette */
		      if (red[plt[j]] < 256)
			  origin->red[j] = red[plt[j]];
		      else
			  origin->red[j] = red[plt[j]] / 256;
		      if (green[plt[j]] < 256)
			  origin->green[j] = green[plt[j]];
		      else
			  origin->green[j] = green[plt[j]] / 256;
		      if (blue[plt[j]] < 256)
			  origin->blue[j] = blue[plt[j]];
		      else
			  origin->blue[j] = blue[plt[j]] / 256;
		  }
	    }
      }
    if (!check_tiff_origin_pixel_conversion (origin))
	goto error;

    if (origin->forced_conversion == RL2_CONVERT_GRAYSCALE_TO_PALETTE)
      {
	  /* forced conversion: creating a GRAYSCALE palette */
	  int i;
	  if (!alloc_palette (origin, 256))
	      goto error;
	  for (i = 0; i < 256; i++)
	    {
		origin->red[i] = i;
		origin->green[i] = i;
		origin->blue[i] = i;
	    }
      }
    if (origin->forced_conversion == RL2_CONVERT_MONOCHROME_TO_PALETTE)
      {
	  /* forced conversion: creating a BILEVEL palette */
	  if (!alloc_palette (origin, 2))
	      goto error;
	  origin->red[0] = 255;
	  origin->green[0] = 255;
	  origin->blue[0] = 255;
	  origin->red[1] = 0;
	  origin->green[1] = 0;
	  origin->blue[1] = 0;
      }

    return 1;
  error:
    return 0;
}

RL2_DECLARE rl2TiffOriginPtr
rl2_create_tiff_origin (const char *path, int georef_priority, int srid,
			unsigned char force_sample_type,
			unsigned char force_pixel_type,
			unsigned char force_num_bands)
{
/* attempting to create a file-based TIFF / GeoTIFF origin */
    rl2PrivTiffOriginPtr origin = NULL;

    if (georef_priority == RL2_TIFF_NO_GEOREF
	|| georef_priority == RL2_TIFF_GEOTIFF
	|| georef_priority == RL2_TIFF_WORLDFILE)
	;
    else
	return NULL;
    origin =
	create_tiff_origin (path, force_sample_type, force_pixel_type,
			    force_num_bands);
    if (origin == NULL)
	return NULL;
    if (georef_priority == RL2_TIFF_GEOTIFF)
      {
	  /* attempting first to retrieve GeoTIFF georeferencing */
	  geo_tiff_origin (path, origin, srid);
	  if (origin->isGeoReferenced == 0)
	    {
		/* then attempting to retrieve WorldFile georeferencing */
		worldfile_tiff_origin (path, origin, srid);
	    }
      }
    else if (georef_priority == RL2_TIFF_WORLDFILE)
      {
	  /* attempting first to retrieve Worldfile georeferencing */
	  worldfile_tiff_origin (path, origin, srid);
	  if (origin->isGeoReferenced == 0)
	    {
		/* then attempting to retrieve GeoTIFF georeferencing */
		geo_tiff_origin (path, origin, srid);
	    }
      }
    if (!init_tiff_origin (path, origin))
	goto error;

    return (rl2TiffOriginPtr) origin;
  error:
    if (origin != NULL)
	rl2_destroy_tiff_origin ((rl2TiffOriginPtr) origin);
    return NULL;
}

RL2_DECLARE rl2TiffOriginPtr
rl2_create_geotiff_origin (const char *path, int force_srid,
			   unsigned char force_sample_type,
			   unsigned char force_pixel_type,
			   unsigned char force_num_bands)
{
/* attempting to create a file-based GeoTIFF origin */
    rl2PrivTiffOriginPtr origin = NULL;

    origin =
	create_tiff_origin (path, force_sample_type, force_pixel_type,
			    force_num_bands);
    if (origin == NULL)
	return NULL;

    /* attempting to retrieve GeoTIFF georeferencing */
    geo_tiff_origin (path, origin, force_srid);
    if (origin->isGeoReferenced == 0)
	goto error;
    if (!init_tiff_origin (path, origin))
	goto error;

    return (rl2TiffOriginPtr) origin;
  error:
    if (origin != NULL)
	rl2_destroy_tiff_origin ((rl2TiffOriginPtr) origin);
    return NULL;
}

RL2_DECLARE rl2TiffOriginPtr
rl2_create_tiff_worldfile_origin (const char *path, int srid,
				  unsigned char force_sample_type,
				  unsigned char force_pixel_type,
				  unsigned char force_num_bands)
{
/* attempting to create a file-based TIFF+TFW origin */
    rl2PrivTiffOriginPtr origin = NULL;

    origin =
	create_tiff_origin (path, force_sample_type, force_pixel_type,
			    force_num_bands);
    if (origin == NULL)
	return NULL;

/* attempting to retrieve Worldfile georeferencing */
    worldfile_tiff_origin (path, origin, srid);
    if (origin->isGeoReferenced == 0)
	goto error;
    if (!init_tiff_origin (path, origin))
	goto error;

    return (rl2TiffOriginPtr) origin;
  error:
    if (origin != NULL)
	rl2_destroy_tiff_origin ((rl2TiffOriginPtr) origin);
    return NULL;
}

RL2_DECLARE int
rl2_set_tiff_origin_not_referenced (rl2TiffOriginPtr tiff)
{
/* setting up a false GeoReferncing */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    origin->hResolution = 1.0;
    origin->vResolution = 1.0;
    origin->minX = 0.0;
    origin->minY = 0.0;
    origin->maxX = origin->width - 1;
    origin->maxY = origin->height - 1;
    origin->Srid = -1;
    origin->isGeoReferenced = 1;
    origin->isGeoTiff = 0;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_get_tiff_origin_path (rl2TiffOriginPtr tiff)
{
/* retrieving the input path from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return NULL;

    return origin->path;
}

RL2_DECLARE const char *
rl2_get_tiff_origin_worldfile_path (rl2TiffOriginPtr tiff)
{
/* retrieving the Worldfile path from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return NULL;

    return origin->tfw_path;
}

RL2_DECLARE int
rl2_is_geotiff_origin (rl2TiffOriginPtr tiff, int *geotiff)
{
/* detecting if a TIFF origin actually is a GeoTIFF */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    *geotiff = origin->isGeoTiff;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_tiff_worldfile_origin (rl2TiffOriginPtr tiff, int *tiff_worldfile)
{
/* detecting if a TIFF origin actually supports a TFW */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    *tiff_worldfile = 0;
    if (origin->tfw_path != NULL)
	*tiff_worldfile = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_size (rl2TiffOriginPtr tiff, unsigned int *width,
			  unsigned int *height)
{
/* retrieving Width and Height from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;

    *width = origin->width;
    *height = origin->height;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_srid (rl2TiffOriginPtr tiff, int *srid)
{
/* retrieving the SRID from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    if (origin->isGeoReferenced == 0)
	return RL2_ERROR;

    *srid = origin->Srid;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_extent (rl2TiffOriginPtr tiff, double *minX, double *minY,
			    double *maxX, double *maxY)
{
/* retrieving the Extent from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    if (origin->isGeoReferenced == 0)
	return RL2_ERROR;

    *minX = origin->minX;
    *minY = origin->minY;
    *maxX = origin->maxX;
    *maxY = origin->maxY;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_resolution (rl2TiffOriginPtr tiff, double *hResolution,
				double *vResolution)
{
/* retrieving the Pixel Resolution from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    if (origin->isGeoReferenced == 0)
	return RL2_ERROR;

    *hResolution = origin->hResolution;
    *vResolution = origin->vResolution;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_type (rl2TiffOriginPtr tiff, unsigned char *sample_type,
			  unsigned char *pixel_type,
			  unsigned char *alias_pixel_type,
			  unsigned char *num_bands)
{
/* retrieving the sample/pixel type from a TIFF origin */
    int ok = 0;
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;

    if (origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->samplesPerPixel == 1
	&& origin->photometric < PHOTOMETRIC_RGB)
      {
	  /* could be some kind of MONOCHROME */
	  if (origin->bitsPerSample == 1)
	    {
		*sample_type = RL2_SAMPLE_1_BIT;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_MONOCHROME;
		*alias_pixel_type = RL2_PIXEL_MONOCHROME;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->samplesPerPixel == 1
	&& origin->photometric < PHOTOMETRIC_RGB)
      {
	  /* could be some kind of GRAYSCALE */
	  if (origin->bitsPerSample == 2)
	    {
		*sample_type = RL2_SAMPLE_2_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 4)
	    {
		*sample_type = RL2_SAMPLE_4_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 16)
	    {
		*sample_type = RL2_SAMPLE_UINT16;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_GRAYSCALE;
		*alias_pixel_type = RL2_PIXEL_GRAYSCALE;
		if (origin->bitsPerSample == 8 || origin->bitsPerSample == 16)
		    *alias_pixel_type = RL2_PIXEL_DATAGRID;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->samplesPerPixel == 1
	&& origin->photometric == PHOTOMETRIC_PALETTE)
      {
	  /* could be some kind of PALETTE */
	  if (origin->bitsPerSample == 1)
	    {
		*sample_type = RL2_SAMPLE_1_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 2)
	    {
		*sample_type = RL2_SAMPLE_2_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 4)
	    {
		*sample_type = RL2_SAMPLE_4_BIT;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_PALETTE;
		*alias_pixel_type = RL2_PIXEL_PALETTE;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (origin->sampleFormat == SAMPLEFORMAT_UINT
	&& origin->samplesPerPixel == 3
	&& origin->photometric == PHOTOMETRIC_RGB)
      {
	  /* could be some kind of RGB */
	  if (origin->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  else if (origin->bitsPerSample == 16)
	    {
		*sample_type = RL2_SAMPLE_UINT16;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_RGB;
		*alias_pixel_type = RL2_PIXEL_RGB;
		*num_bands = 3;
		return RL2_OK;
	    }
      }
    if (origin->samplesPerPixel == 1 && origin->photometric < PHOTOMETRIC_RGB)
      {
	  /* could be some kind of DATA-GRID */
	  if (origin->sampleFormat == SAMPLEFORMAT_INT)
	    {
		/* Signed Integer */
		if (origin->bitsPerSample == 8)
		  {
		      *sample_type = RL2_SAMPLE_INT8;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 16)
		  {
		      *sample_type = RL2_SAMPLE_INT16;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_INT32;
		      ok = 1;
		  }
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_UINT)
	    {
		/* Unsigned Integer */
		if (origin->bitsPerSample == 8)
		  {
		      *sample_type = RL2_SAMPLE_UINT8;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 16)
		  {
		      *sample_type = RL2_SAMPLE_UINT16;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_UINT32;
		      ok = 1;
		  }
	    }
	  if (origin->sampleFormat == SAMPLEFORMAT_IEEEFP)
	    {
		/* Floating-Point */
		if (origin->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_FLOAT;
		      ok = 1;
		  }
		else if (origin->bitsPerSample == 64)
		  {
		      *sample_type = RL2_SAMPLE_DOUBLE;
		      ok = 1;
		  }
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_DATAGRID;
		*alias_pixel_type = RL2_PIXEL_DATAGRID;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_tiff_origin_forced_type (rl2TiffOriginPtr tiff,
				 unsigned char *sample_type,
				 unsigned char *pixel_type,
				 unsigned char *num_bands)
{
/* retrieving the sample/pixel type (FORCED) from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;

    *sample_type = origin->forced_sample_type;
    *pixel_type = origin->forced_pixel_type;
    *num_bands = origin->forced_num_bands;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_compression (rl2TiffOriginPtr tiff,
				 unsigned char *compression)
{
/* retrieving the sample/pixel type from a TIFF origin */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;

    switch (origin->compression)
      {
      case COMPRESSION_NONE:
	  *compression = RL2_COMPRESSION_NONE;
	  break;
      case COMPRESSION_LZW:
	  *compression = RL2_COMPRESSION_LZW;
	  break;
      case COMPRESSION_DEFLATE:
	  *compression = RL2_COMPRESSION_DEFLATE;
	  break;
      case COMPRESSION_LZMA:
	  *compression = RL2_COMPRESSION_LZMA;
	  break;
      case COMPRESSION_JPEG:
	  *compression = RL2_COMPRESSION_JPEG;
	  break;
      case COMPRESSION_CCITTFAX3:
	  *compression = RL2_COMPRESSION_CCITTFAX3;
	  break;
      case COMPRESSION_CCITTFAX4:
	  *compression = RL2_COMPRESSION_CCITTFAX4;
	  break;
      default:
	  *compression = RL2_COMPRESSION_UNKNOWN;
	  break;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_eval_tiff_origin_compatibility (rl2CoveragePtr cvg, rl2TiffOriginPtr tiff,
				    int force_srid, int verbose)
{
/* testing if a Coverage and a TIFF origin are mutually compatible */
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    double hResolution;
    double vResolution;
    double confidence;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;

    if (coverage == NULL || tiff == NULL)
	return RL2_ERROR;
    if (rl2_get_tiff_origin_forced_type
	(tiff, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	return RL2_ERROR;

/* aliasing GRAYSCALE and DATAGRID for UINT8 */
    if (coverage->sampleType == RL2_SAMPLE_UINT8
	&& coverage->pixelType == RL2_PIXEL_DATAGRID
	&& pixel_type == RL2_PIXEL_GRAYSCALE)
	pixel_type = RL2_PIXEL_DATAGRID;
/* aliasing GRAYSCALE and DATAGRID for UINT16 */
    if (coverage->sampleType == RL2_SAMPLE_UINT16
	&& coverage->pixelType == RL2_PIXEL_DATAGRID
	&& pixel_type == RL2_PIXEL_GRAYSCALE)
	pixel_type = RL2_PIXEL_DATAGRID;

    if (coverage->sampleType != sample_type)
      {
	  if (verbose)
	      fprintf (stderr, "Mismatching SampleType !!!\n");
	  return RL2_FALSE;
      }
    if (coverage->pixelType != pixel_type)
      {
	  if (verbose)
	      fprintf (stderr, "Mismatching PixelType !!!\n");
	  return RL2_FALSE;
      }
    if (coverage->nBands != num_bands)
      {
	  if (verbose)
	      fprintf (stderr, "Mismatching Number of Bands !!!\n");
	  return RL2_FALSE;
      }

    if (coverage->Srid == RL2_GEOREFERENCING_NONE)
	return RL2_TRUE;

/* checking for resolution compatibility */
    if (rl2_get_tiff_origin_srid (tiff, &srid) != RL2_OK)
	return RL2_FALSE;
    if (coverage->Srid != srid)
      {
	  if (force_srid > 0)
	    {
		if (coverage->Srid != force_srid)
		  {
		      if (verbose)
			  fprintf (stderr, "Mismatching SRID !!!\n");
		      return RL2_FALSE;
		  }
	    }
	  else
	    {
		if (verbose)
		    fprintf (stderr, "Mismatching SRID !!!\n");
		return RL2_FALSE;
	    }
      }
    if (rl2_get_tiff_origin_resolution (tiff, &hResolution, &vResolution) !=
	RL2_OK)
	return RL2_FALSE;
    if (coverage->mixedResolutions)
      {
	  /* accepting any resolution */
      }
    else if (coverage->strictResolution)
      {
	  /* enforcing Strict Resolution check */
	  double x_diff = fabs (coverage->hResolution - hResolution);
	  double y_diff = fabs (coverage->vResolution - vResolution);
	  double x_lim = coverage->hResolution / 1000000.0;
	  double y_lim = coverage->vResolution / 1000000.0;
	  if (x_diff > x_lim)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Horizontal Resolution (Strict) !!!\n");
		return RL2_FALSE;
	    }
	  if (y_diff > y_lim)
	    {
		if (verbose)
		    fprintf (stderr,
			     "Mismatching Vertical Resolution (Strict) !!!\n");
		return RL2_FALSE;
	    }
      }
    else
      {
	  /* permissive Resolution check */
	  confidence = coverage->hResolution / 100.0;
	  if (hResolution < (coverage->hResolution - confidence)
	      || hResolution > (coverage->hResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr, "Mismatching Horizontal Resolution !!!\n");
		return RL2_FALSE;
	    }
	  confidence = coverage->vResolution / 100.0;
	  if (vResolution < (coverage->vResolution - confidence)
	      || vResolution > (coverage->vResolution + confidence))
	    {
		if (verbose)
		    fprintf (stderr, "Mismatching Vertical Resolution !!!\n");
		return RL2_FALSE;
	    }
      }
    return RL2_TRUE;
}

RL2_PRIVATE char
truncate_8 (double val)
{
/* truncating to signed 8 bit integer */
    if (val <= INT8_MIN)
	return INT8_MIN;
    if (val >= INT8_MAX)
	return INT8_MAX;
    return (char) val;
}

RL2_PRIVATE unsigned char
truncate_u8 (double val)
{
/* truncating to unsigned 8 bit integer */
    if (val <= 0.0)
	return 0;
    if (val >= UINT8_MAX)
	return UINT8_MAX;
    return (unsigned char) val;
}

RL2_PRIVATE short
truncate_16 (double val)
{
/* truncating to signed 16 bit integer */
    if (val <= INT16_MIN)
	return INT16_MIN;
    if (val >= INT16_MAX)
	return INT16_MAX;
    return (short) val;
}

RL2_PRIVATE unsigned short
truncate_u16 (double val)
{
/* truncating to unsigned 16 bit integer */
    if (val <= 0.0)
	return 0;
    if (val >= UINT16_MAX)
	return UINT16_MAX;
    return (unsigned short) val;
}

RL2_PRIVATE int
truncate_32 (double val)
{
/* truncating to signed 32 bit integer */
    if (val <= INT32_MIN)
	return INT32_MIN;
    if (val >= INT32_MAX)
	return INT32_MAX;
    return (int) val;
}

RL2_PRIVATE unsigned int
truncate_u32 (double val)
{
/* truncating to unsigned 32 bit integer */
    if (val <= 0.0)
	return 0;
    if (val >= UINT32_MAX)
	return UINT32_MAX;
    return (unsigned int) val;
}

static void
copy_convert_tile (rl2PrivTiffOriginPtr origin, void *in, void *out,
		   unsigned int startRow, unsigned int startCol,
		   unsigned short width, unsigned short height, uint32 tile_y,
		   uint32 tile_x, unsigned char convert)
{
/* copying pixels by applying a format conversion */
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
    uint32 x;
    uint32 y;
    unsigned int dest_x;
    unsigned int dest_y;

    for (y = 0; y < origin->tileHeight; y++)
      {
	  dest_y = tile_y + y;
	  if (dest_y < startRow || dest_y >= (startRow + height))
	      continue;

	  switch (convert)
	    {
	    case RL2_CONVERT_GRID_INT8_TO_UINT8:
	    case RL2_CONVERT_GRID_INT8_TO_INT16:
	    case RL2_CONVERT_GRID_INT8_TO_UINT16:
	    case RL2_CONVERT_GRID_INT8_TO_INT32:
	    case RL2_CONVERT_GRID_INT8_TO_UINT32:
	    case RL2_CONVERT_GRID_INT8_TO_FLOAT:
	    case RL2_CONVERT_GRID_INT8_TO_DOUBLE:
		p_in_8 = (char *) in;
		p_in_8 += y * origin->tileWidth;
		break;
	    case RL2_CONVERT_GRID_UINT8_TO_INT8:
	    case RL2_CONVERT_GRID_UINT8_TO_UINT16:
	    case RL2_CONVERT_GRID_UINT8_TO_INT16:
	    case RL2_CONVERT_GRID_UINT8_TO_UINT32:
	    case RL2_CONVERT_GRID_UINT8_TO_INT32:
	    case RL2_CONVERT_GRID_UINT8_TO_FLOAT:
	    case RL2_CONVERT_GRID_UINT8_TO_DOUBLE:
		p_in_u8 = (unsigned char *) in;
		p_in_u8 += y * origin->tileWidth;
		break;
	    case RL2_CONVERT_GRID_INT16_TO_INT8:
	    case RL2_CONVERT_GRID_INT16_TO_UINT8:
	    case RL2_CONVERT_GRID_INT16_TO_UINT16:
	    case RL2_CONVERT_GRID_INT16_TO_INT32:
	    case RL2_CONVERT_GRID_INT16_TO_UINT32:
	    case RL2_CONVERT_GRID_INT16_TO_FLOAT:
	    case RL2_CONVERT_GRID_INT16_TO_DOUBLE:
		p_in_16 = (short *) in;
		p_in_16 += y * origin->tileWidth;
		break;
	    case RL2_CONVERT_GRID_UINT16_TO_INT8:
	    case RL2_CONVERT_GRID_UINT16_TO_UINT8:
	    case RL2_CONVERT_GRID_UINT16_TO_INT16:
	    case RL2_CONVERT_GRID_UINT16_TO_UINT32:
	    case RL2_CONVERT_GRID_UINT16_TO_INT32:
	    case RL2_CONVERT_GRID_UINT16_TO_FLOAT:
	    case RL2_CONVERT_GRID_UINT16_TO_DOUBLE:
		p_in_u16 = (unsigned short *) in;
		p_in_u16 += y * origin->tileWidth;
		break;
	    case RL2_CONVERT_GRID_INT32_TO_INT8:
	    case RL2_CONVERT_GRID_INT32_TO_UINT8:
	    case RL2_CONVERT_GRID_INT32_TO_INT16:
	    case RL2_CONVERT_GRID_INT32_TO_UINT16:
	    case RL2_CONVERT_GRID_INT32_TO_UINT32:
	    case RL2_CONVERT_GRID_INT32_TO_FLOAT:
	    case RL2_CONVERT_GRID_INT32_TO_DOUBLE:
		p_in_32 = (int *) in;
		p_in_32 += y * origin->tileWidth;
		break;
	    case RL2_CONVERT_GRID_UINT32_TO_INT8:
	    case RL2_CONVERT_GRID_UINT32_TO_UINT8:
	    case RL2_CONVERT_GRID_UINT32_TO_INT16:
	    case RL2_CONVERT_GRID_UINT32_TO_UINT16:
	    case RL2_CONVERT_GRID_UINT32_TO_INT32:
	    case RL2_CONVERT_GRID_UINT32_TO_FLOAT:
	    case RL2_CONVERT_GRID_UINT32_TO_DOUBLE:
		p_in_u32 = (unsigned int *) in;
		p_in_u32 += y * origin->tileWidth;
		break;
	    case RL2_CONVERT_GRID_FLOAT_TO_INT8:
	    case RL2_CONVERT_GRID_FLOAT_TO_UINT8:
	    case RL2_CONVERT_GRID_FLOAT_TO_INT16:
	    case RL2_CONVERT_GRID_FLOAT_TO_UINT16:
	    case RL2_CONVERT_GRID_FLOAT_TO_INT32:
	    case RL2_CONVERT_GRID_FLOAT_TO_UINT32:
	    case RL2_CONVERT_GRID_FLOAT_TO_DOUBLE:
		p_in_flt = (float *) in;
		p_in_flt += y * origin->tileWidth;
		break;
	    case RL2_CONVERT_GRID_DOUBLE_TO_INT8:
	    case RL2_CONVERT_GRID_DOUBLE_TO_UINT8:
	    case RL2_CONVERT_GRID_DOUBLE_TO_INT16:
	    case RL2_CONVERT_GRID_DOUBLE_TO_UINT16:
	    case RL2_CONVERT_GRID_DOUBLE_TO_INT32:
	    case RL2_CONVERT_GRID_DOUBLE_TO_UINT32:
	    case RL2_CONVERT_GRID_DOUBLE_TO_FLOAT:
		p_in_dbl = (double *) in;
		p_in_dbl += y * origin->tileWidth;
		break;
	    };

	  for (x = 0; x < origin->tileWidth; x++)
	    {
		dest_x = tile_x + x;
		if (dest_x < startCol || dest_x >= (startCol + width))
		    continue;

		switch (convert)
		  {
		  case RL2_CONVERT_GRID_UINT8_TO_INT8:
		  case RL2_CONVERT_GRID_INT16_TO_INT8:
		  case RL2_CONVERT_GRID_UINT16_TO_INT8:
		  case RL2_CONVERT_GRID_INT32_TO_INT8:
		  case RL2_CONVERT_GRID_UINT32_TO_INT8:
		  case RL2_CONVERT_GRID_FLOAT_TO_INT8:
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT8:
		      p_out_8 = (char *) out;
		      p_out_8 +=
			  ((dest_y - startRow) * width) + (dest_x - startCol);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_UINT8:
		  case RL2_CONVERT_GRID_INT16_TO_UINT8:
		  case RL2_CONVERT_GRID_UINT16_TO_UINT8:
		  case RL2_CONVERT_GRID_INT32_TO_UINT8:
		  case RL2_CONVERT_GRID_UINT32_TO_UINT8:
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT8:
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT8:
		      p_out_u8 = (unsigned char *) out;
		      p_out_u8 +=
			  ((dest_y - startRow) * width) + (dest_x - startCol);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_INT16:
		  case RL2_CONVERT_GRID_UINT8_TO_INT16:
		  case RL2_CONVERT_GRID_UINT16_TO_INT16:
		  case RL2_CONVERT_GRID_INT32_TO_INT16:
		  case RL2_CONVERT_GRID_UINT32_TO_INT16:
		  case RL2_CONVERT_GRID_FLOAT_TO_INT16:
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT16:
		      p_out_16 = (short *) out;
		      p_out_16 +=
			  ((dest_y - startRow) * width) + (dest_x - startCol);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_UINT16:
		  case RL2_CONVERT_GRID_UINT8_TO_UINT16:
		  case RL2_CONVERT_GRID_INT16_TO_UINT16:
		  case RL2_CONVERT_GRID_INT32_TO_UINT16:
		  case RL2_CONVERT_GRID_UINT32_TO_UINT16:
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT16:
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT16:
		      p_out_u16 = (unsigned short *) out;
		      p_out_u16 +=
			  ((dest_y - startRow) * width) + (dest_x - startCol);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_INT32:
		  case RL2_CONVERT_GRID_UINT8_TO_INT32:
		  case RL2_CONVERT_GRID_INT16_TO_INT32:
		  case RL2_CONVERT_GRID_UINT16_TO_INT32:
		  case RL2_CONVERT_GRID_UINT32_TO_INT32:
		  case RL2_CONVERT_GRID_FLOAT_TO_INT32:
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT32:
		      p_out_32 = (int *) out;
		      p_out_32 +=
			  ((dest_y - startRow) * width) + (dest_x - startCol);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_UINT32:
		  case RL2_CONVERT_GRID_UINT8_TO_UINT32:
		  case RL2_CONVERT_GRID_INT16_TO_UINT32:
		  case RL2_CONVERT_GRID_UINT16_TO_UINT32:
		  case RL2_CONVERT_GRID_INT32_TO_UINT32:
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT32:
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT32:
		      p_out_u32 = (unsigned int *) out;
		      p_out_u32 +=
			  ((dest_y - startRow) * width) + (dest_x - startCol);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_FLOAT:
		  case RL2_CONVERT_GRID_UINT8_TO_FLOAT:
		  case RL2_CONVERT_GRID_INT16_TO_FLOAT:
		  case RL2_CONVERT_GRID_UINT16_TO_FLOAT:
		  case RL2_CONVERT_GRID_INT32_TO_FLOAT:
		  case RL2_CONVERT_GRID_UINT32_TO_FLOAT:
		  case RL2_CONVERT_GRID_DOUBLE_TO_FLOAT:
		      p_out_flt = (float *) out;
		      p_out_flt +=
			  ((dest_y - startRow) * width) + (dest_x - startCol);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_DOUBLE:
		  case RL2_CONVERT_GRID_UINT8_TO_DOUBLE:
		  case RL2_CONVERT_GRID_INT16_TO_DOUBLE:
		  case RL2_CONVERT_GRID_UINT16_TO_DOUBLE:
		  case RL2_CONVERT_GRID_INT32_TO_DOUBLE:
		  case RL2_CONVERT_GRID_UINT32_TO_DOUBLE:
		  case RL2_CONVERT_GRID_FLOAT_TO_DOUBLE:
		      p_out_dbl = (double *) out;
		      p_out_dbl +=
			  ((dest_y - startRow) * width) + (dest_x - startCol);
		      break;
		  };

		switch (convert)
		  {
		  case RL2_CONVERT_GRID_INT8_TO_UINT8:
		      *p_out_u8++ = truncate_u8 (*p_in_8++);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_INT16:
		      *p_out_16++ = *p_in_8++;
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_UINT16:
		      *p_out_u16++ = truncate_u16 (*p_in_8++);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_INT32:
		      *p_out_32++ = *p_in_8++;
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_UINT32:
		      *p_out_u32++ = truncate_u32 (*p_in_8++);
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_FLOAT:
		      *p_out_flt++ = *p_in_8++;
		      break;
		  case RL2_CONVERT_GRID_INT8_TO_DOUBLE:
		      *p_out_dbl++ = *p_in_8++;
		      break;
		  case RL2_CONVERT_GRID_UINT8_TO_INT8:
		      *p_out_8++ = truncate_8 (*p_in_u8++);
		      break;
		  case RL2_CONVERT_GRID_UINT8_TO_INT16:
		      *p_out_16++ = truncate_16 (*p_in_u8++);
		      break;
		  case RL2_CONVERT_GRID_UINT8_TO_UINT16:
		      *p_out_u16++ = *p_in_u8++;
		      break;
		  case RL2_CONVERT_GRID_UINT8_TO_UINT32:
		      *p_out_u32++ = *p_in_u8++;
		      break;
		  case RL2_CONVERT_GRID_UINT8_TO_INT32:
		      *p_out_32++ = truncate_32 (*p_in_u8++);
		      break;
		  case RL2_CONVERT_GRID_UINT8_TO_FLOAT:
		      *p_out_flt++ = *p_in_u8++;
		      break;
		  case RL2_CONVERT_GRID_UINT8_TO_DOUBLE:
		      *p_out_dbl++ = *p_in_u8++;
		      break;
		  case RL2_CONVERT_GRID_INT16_TO_INT8:
		      *p_out_8++ = truncate_8 (*p_in_16++);
		      break;
		  case RL2_CONVERT_GRID_INT16_TO_UINT8:
		      *p_out_u8++ = truncate_u8 (*p_in_16++);
		      break;
		  case RL2_CONVERT_GRID_INT16_TO_UINT16:
		      *p_out_u16++ = truncate_u16 (*p_in_16++);
		      break;
		  case RL2_CONVERT_GRID_INT16_TO_INT32:
		      *p_out_32++ = *p_in_16++;
		      break;
		  case RL2_CONVERT_GRID_INT16_TO_UINT32:
		      *p_out_u32++ = truncate_u32 (*p_in_16++);
		      break;
		  case RL2_CONVERT_GRID_INT16_TO_FLOAT:
		      *p_out_flt++ = *p_in_16++;
		      break;
		  case RL2_CONVERT_GRID_INT16_TO_DOUBLE:
		      *p_out_dbl++ = *p_in_16++;
		      break;
		  case RL2_CONVERT_GRID_UINT16_TO_INT8:
		      *p_out_8++ = truncate_8 (*p_in_u16++);
		      break;
		  case RL2_CONVERT_GRID_UINT16_TO_UINT8:
		      *p_out_u8++ = truncate_u8 (*p_in_u16++);
		      break;
		  case RL2_CONVERT_GRID_UINT16_TO_INT16:
		      *p_out_16++ = truncate_16 (*p_in_u16++);
		      break;
		  case RL2_CONVERT_GRID_UINT16_TO_UINT32:
		      *p_out_u32++ = *p_in_u16++;
		      break;
		  case RL2_CONVERT_GRID_UINT16_TO_INT32:
		      *p_out_32++ = truncate_32 (*p_in_u16++);
		      break;
		  case RL2_CONVERT_GRID_UINT16_TO_FLOAT:
		      *p_out_flt++ = *p_in_u16++;
		      break;
		  case RL2_CONVERT_GRID_UINT16_TO_DOUBLE:
		      *p_out_dbl++ = *p_in_u16++;
		      break;
		  case RL2_CONVERT_GRID_INT32_TO_INT8:
		      *p_out_8++ = truncate_8 (*p_in_32++);
		      break;
		  case RL2_CONVERT_GRID_INT32_TO_UINT8:
		      *p_out_u8++ = truncate_u8 (*p_in_32++);
		      break;
		  case RL2_CONVERT_GRID_INT32_TO_INT16:
		      *p_out_16++ = truncate_16 (*p_in_32++);
		      break;
		  case RL2_CONVERT_GRID_INT32_TO_UINT16:
		      *p_out_u16++ = truncate_u16 (*p_in_32++);
		      break;
		  case RL2_CONVERT_GRID_INT32_TO_UINT32:
		      *p_out_u32++ = truncate_u32 (*p_in_32++);
		      break;
		  case RL2_CONVERT_GRID_INT32_TO_FLOAT:
		      *p_out_flt++ = *p_in_32++;
		      break;
		  case RL2_CONVERT_GRID_INT32_TO_DOUBLE:
		      *p_out_dbl++ = *p_in_32++;
		      break;
		  case RL2_CONVERT_GRID_UINT32_TO_INT8:
		      *p_out_8++ = truncate_8 (*p_in_u32++);
		      break;
		  case RL2_CONVERT_GRID_UINT32_TO_UINT8:
		      *p_out_u8++ = truncate_u8 (*p_in_u32++);
		      break;
		  case RL2_CONVERT_GRID_UINT32_TO_INT16:
		      *p_out_16++ = truncate_16 (*p_in_u32++);
		      break;
		  case RL2_CONVERT_GRID_UINT32_TO_UINT16:
		      *p_out_u16++ = truncate_u16 (*p_in_u32++);
		      break;
		  case RL2_CONVERT_GRID_UINT32_TO_INT32:
		      *p_out_32++ = truncate_32 (*p_in_u32++);
		      break;
		  case RL2_CONVERT_GRID_UINT32_TO_FLOAT:
		      *p_out_flt++ = *p_in_u32++;
		      break;
		  case RL2_CONVERT_GRID_UINT32_TO_DOUBLE:
		      *p_out_dbl++ = *p_in_u32++;
		      break;
		  case RL2_CONVERT_GRID_FLOAT_TO_INT8:
		      *p_out_8++ = truncate_8 (*p_in_flt++);
		      break;
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT8:
		      *p_out_u8++ = truncate_u8 (*p_in_flt++);
		      break;
		  case RL2_CONVERT_GRID_FLOAT_TO_INT16:
		      *p_out_16++ = truncate_16 (*p_in_flt++);
		      break;
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT16:
		      *p_out_u16++ = truncate_u16 (*p_in_flt++);
		      break;
		  case RL2_CONVERT_GRID_FLOAT_TO_INT32:
		      *p_out_32++ = truncate_32 (*p_in_flt++);
		      break;
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT32:
		      *p_out_u32++ = truncate_u32 (*p_in_flt++);
		      break;
		  case RL2_CONVERT_GRID_FLOAT_TO_DOUBLE:
		      *p_out_dbl++ = *p_in_flt++;
		      break;
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT8:
		      *p_out_8++ = truncate_8 (*p_in_dbl++);
		      break;
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT8:
		      *p_out_u8++ = truncate_u8 (*p_in_dbl++);
		      break;
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT16:
		      *p_out_16++ = truncate_16 (*p_in_dbl++);
		      break;
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT16:
		      *p_out_u16++ = truncate_u16 (*p_in_dbl++);
		      break;
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT32:
		      *p_out_32++ = truncate_32 (*p_in_dbl++);
		      break;
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT32:
		      *p_out_u32++ = truncate_u32 (*p_in_dbl++);
		      break;
		  case RL2_CONVERT_GRID_DOUBLE_TO_FLOAT:
		      *p_out_flt++ = *p_in_dbl++;
		      break;
		  };
	    }
      }
}

static int
read_raw_tiles (rl2PrivTiffOriginPtr origin, unsigned short width,
		unsigned short height, unsigned char sample_type,
		unsigned char num_bands, unsigned int startRow,
		unsigned int startCol, unsigned char *pixels)
{
/* reading TIFF raw tiles */
    uint32 tile_x;
    uint32 tile_y;
    uint32 x;
    uint32 y;
    uint32 *tiff_tile = NULL;
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
    unsigned int dest_x;
    unsigned int dest_y;
    int skip;
    unsigned char bnd;
    unsigned char convert = origin->forced_conversion;

    tiff_tile = malloc (TIFFTileSize (origin->in));
    if (tiff_tile == NULL)
	goto error;

    for (tile_y = 0; tile_y < origin->height; tile_y += origin->tileHeight)
      {
	  /* scanning tiles by row */
	  unsigned int tiff_min_y = tile_y;
	  unsigned int tiff_max_y = tile_y + origin->tileHeight - 1;
	  skip = 0;
	  if (tiff_min_y > (startRow + height))
	      skip = 1;
	  if (tiff_max_y < startRow)
	      skip = 1;
	  if (skip)
	    {
		/* skipping any not required tile */
		continue;
	    }
	  for (tile_x = 0; tile_x < origin->width; tile_x += origin->tileWidth)
	    {
		/* reading a TIFF tile */
		unsigned int tiff_min_x = tile_x;
		unsigned int tiff_max_x = tile_x + origin->tileWidth - 1;
		skip = 0;
		if (tiff_min_x > (startCol + width))
		    skip = 1;
		if (tiff_max_x < startCol)
		    skip = 1;
		if (skip)
		  {
		      /* skipping any not required tile */
		      continue;
		  }
		if (TIFFReadTile (origin->in, tiff_tile, tile_x, tile_y, 0, 0)
		    < 0)
		    goto error;
		if (convert != RL2_CONVERT_NO)
		  {
		      /* applying some format conversion */
		      copy_convert_tile (origin, tiff_tile, pixels, startRow,
					 startCol, width, height, tile_y,
					 tile_x, convert);
		      continue;
		  }
		for (y = 0; y < origin->tileHeight; y++)
		  {
		      dest_y = tile_y + y;
		      if (dest_y < startRow || dest_y >= (startRow + height))
			  continue;
		      for (x = 0; x < origin->tileWidth; x++)
			{
			    dest_x = tile_x + x;
			    if (dest_x < startCol
				|| dest_x >= (startCol + width))
				continue;
			    switch (sample_type)
			      {
			      case RL2_SAMPLE_INT8:
				  p_in_8 = (char *) tiff_tile;
				  p_in_8 += y * origin->tileWidth;
				  p_in_8 += x;
				  p_out_8 = (char *) pixels;
				  p_out_8 +=
				      ((dest_y - startRow) * width) +
				      (dest_x - startCol);
				  break;
			      case RL2_SAMPLE_UINT8:
				  p_in_u8 = (unsigned char *) tiff_tile;
				  p_in_u8 += y * origin->tileWidth * num_bands;
				  p_in_u8 += x * num_bands;
				  p_out_u8 = (unsigned char *) pixels;
				  p_out_u8 +=
				      ((dest_y -
					startRow) * width * num_bands) +
				      ((dest_x - startCol) * num_bands);
				  break;
			      case RL2_SAMPLE_INT16:
				  p_in_16 = (short *) tiff_tile;
				  p_in_16 += y * origin->tileWidth;
				  p_in_16 += x;
				  p_out_16 = (short *) pixels;
				  p_out_16 +=
				      ((dest_y - startRow) * width) +
				      (dest_x - startCol);
				  break;
			      case RL2_SAMPLE_UINT16:
				  p_in_u16 = (unsigned short *) tiff_tile;
				  p_in_u16 += y * origin->tileWidth * num_bands;
				  p_in_u16 += x * num_bands;
				  p_out_u16 = (unsigned short *) pixels;
				  p_out_u16 +=
				      ((dest_y -
					startRow) * width * num_bands) +
				      ((dest_x - startCol) * num_bands);
				  break;
			      case RL2_SAMPLE_INT32:
				  p_in_32 = (int *) tiff_tile;
				  p_in_32 += y * origin->tileWidth;
				  p_in_32 += x;
				  p_out_32 = (int *) pixels;
				  p_out_32 +=
				      ((dest_y - startRow) * width) +
				      (dest_x - startCol);
				  break;
			      case RL2_SAMPLE_UINT32:
				  p_in_u32 = (unsigned int *) tiff_tile;
				  p_in_u32 += y * origin->tileWidth;
				  p_in_u32 += x;
				  p_out_u32 = (unsigned int *) pixels;
				  p_out_u32 +=
				      ((dest_y - startRow) * width) +
				      (dest_x - startCol);
				  break;
			      case RL2_SAMPLE_FLOAT:
				  p_in_flt = (float *) tiff_tile;
				  p_in_flt += y * origin->tileWidth;
				  p_in_flt += x;
				  p_out_flt = (float *) pixels;
				  p_out_flt +=
				      ((dest_y - startRow) * width) +
				      (dest_x - startCol);
				  break;
			      case RL2_SAMPLE_DOUBLE:
				  p_in_dbl = (double *) tiff_tile;
				  p_in_dbl += y * origin->tileWidth;
				  p_in_dbl += x;
				  p_out_dbl = (double *) pixels;
				  p_out_dbl +=
				      ((dest_y - startRow) * width) +
				      (dest_x - startCol);
				  break;
			      };
			    for (bnd = 0; bnd < num_bands; bnd++)
			      {
				  switch (sample_type)
				    {
				    case RL2_SAMPLE_INT8:
					*p_out_8 = *p_in_8++;
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
			      }
			}
		  }
	    }
      }

    free (tiff_tile);
    return RL2_OK;
  error:
    if (tiff_tile != NULL)
	free (tiff_tile);
    return RL2_ERROR;
}

static void
copy_convert_scanline (rl2PrivTiffOriginPtr origin, void *in, void *out,
		       unsigned int lineNo, unsigned int startCol,
		       unsigned int width, unsigned char convert)
{
/* copying pixels by applying a format conversion */
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
    uint32 x;

    switch (convert)
      {
      case RL2_CONVERT_GRID_INT8_TO_UINT8:
      case RL2_CONVERT_GRID_INT8_TO_INT16:
      case RL2_CONVERT_GRID_INT8_TO_UINT16:
      case RL2_CONVERT_GRID_INT8_TO_INT32:
      case RL2_CONVERT_GRID_INT8_TO_UINT32:
      case RL2_CONVERT_GRID_INT8_TO_FLOAT:
      case RL2_CONVERT_GRID_INT8_TO_DOUBLE:
	  p_in_8 = (char *) in;
	  break;
      case RL2_CONVERT_GRID_UINT8_TO_INT8:
      case RL2_CONVERT_GRID_UINT8_TO_UINT16:
      case RL2_CONVERT_GRID_UINT8_TO_INT16:
      case RL2_CONVERT_GRID_UINT8_TO_UINT32:
      case RL2_CONVERT_GRID_UINT8_TO_INT32:
      case RL2_CONVERT_GRID_UINT8_TO_FLOAT:
      case RL2_CONVERT_GRID_UINT8_TO_DOUBLE:
	  p_in_u8 = (unsigned char *) in;
	  break;
      case RL2_CONVERT_GRID_INT16_TO_INT8:
      case RL2_CONVERT_GRID_INT16_TO_UINT8:
      case RL2_CONVERT_GRID_INT16_TO_UINT16:
      case RL2_CONVERT_GRID_INT16_TO_INT32:
      case RL2_CONVERT_GRID_INT16_TO_UINT32:
      case RL2_CONVERT_GRID_INT16_TO_FLOAT:
      case RL2_CONVERT_GRID_INT16_TO_DOUBLE:
	  p_in_16 = (short *) in;
	  break;
      case RL2_CONVERT_GRID_UINT16_TO_INT8:
      case RL2_CONVERT_GRID_UINT16_TO_UINT8:
      case RL2_CONVERT_GRID_UINT16_TO_INT16:
      case RL2_CONVERT_GRID_UINT16_TO_UINT32:
      case RL2_CONVERT_GRID_UINT16_TO_INT32:
      case RL2_CONVERT_GRID_UINT16_TO_FLOAT:
      case RL2_CONVERT_GRID_UINT16_TO_DOUBLE:
	  p_in_u16 = (unsigned short *) in;
	  break;
      case RL2_CONVERT_GRID_INT32_TO_INT8:
      case RL2_CONVERT_GRID_INT32_TO_UINT8:
      case RL2_CONVERT_GRID_INT32_TO_INT16:
      case RL2_CONVERT_GRID_INT32_TO_UINT16:
      case RL2_CONVERT_GRID_INT32_TO_UINT32:
      case RL2_CONVERT_GRID_INT32_TO_FLOAT:
      case RL2_CONVERT_GRID_INT32_TO_DOUBLE:
	  p_in_32 = (int *) in;
	  break;
      case RL2_CONVERT_GRID_UINT32_TO_INT8:
      case RL2_CONVERT_GRID_UINT32_TO_UINT8:
      case RL2_CONVERT_GRID_UINT32_TO_INT16:
      case RL2_CONVERT_GRID_UINT32_TO_UINT16:
      case RL2_CONVERT_GRID_UINT32_TO_INT32:
      case RL2_CONVERT_GRID_UINT32_TO_FLOAT:
      case RL2_CONVERT_GRID_UINT32_TO_DOUBLE:
	  p_in_u32 = (unsigned int *) in;
	  break;
      case RL2_CONVERT_GRID_FLOAT_TO_INT8:
      case RL2_CONVERT_GRID_FLOAT_TO_UINT8:
      case RL2_CONVERT_GRID_FLOAT_TO_INT16:
      case RL2_CONVERT_GRID_FLOAT_TO_UINT16:
      case RL2_CONVERT_GRID_FLOAT_TO_INT32:
      case RL2_CONVERT_GRID_FLOAT_TO_UINT32:
      case RL2_CONVERT_GRID_FLOAT_TO_DOUBLE:
	  p_in_flt = (float *) in;
	  break;
      case RL2_CONVERT_GRID_DOUBLE_TO_INT8:
      case RL2_CONVERT_GRID_DOUBLE_TO_UINT8:
      case RL2_CONVERT_GRID_DOUBLE_TO_INT16:
      case RL2_CONVERT_GRID_DOUBLE_TO_UINT16:
      case RL2_CONVERT_GRID_DOUBLE_TO_INT32:
      case RL2_CONVERT_GRID_DOUBLE_TO_UINT32:
      case RL2_CONVERT_GRID_DOUBLE_TO_FLOAT:
	  p_in_dbl = (double *) in;
	  break;
      };

    switch (convert)
      {
      case RL2_CONVERT_GRID_UINT8_TO_INT8:
      case RL2_CONVERT_GRID_INT16_TO_INT8:
      case RL2_CONVERT_GRID_UINT16_TO_INT8:
      case RL2_CONVERT_GRID_INT32_TO_INT8:
      case RL2_CONVERT_GRID_UINT32_TO_INT8:
      case RL2_CONVERT_GRID_FLOAT_TO_INT8:
      case RL2_CONVERT_GRID_DOUBLE_TO_INT8:
	  p_out_8 = (char *) out;
	  p_out_8 += lineNo * width;
	  break;
      case RL2_CONVERT_GRID_INT8_TO_UINT8:
      case RL2_CONVERT_GRID_INT16_TO_UINT8:
      case RL2_CONVERT_GRID_UINT16_TO_UINT8:
      case RL2_CONVERT_GRID_INT32_TO_UINT8:
      case RL2_CONVERT_GRID_UINT32_TO_UINT8:
      case RL2_CONVERT_GRID_FLOAT_TO_UINT8:
      case RL2_CONVERT_GRID_DOUBLE_TO_UINT8:
	  p_out_u8 = (unsigned char *) out;
	  p_out_u8 += lineNo * width;
	  break;
      case RL2_CONVERT_GRID_INT8_TO_INT16:
      case RL2_CONVERT_GRID_UINT8_TO_INT16:
      case RL2_CONVERT_GRID_UINT16_TO_INT16:
      case RL2_CONVERT_GRID_INT32_TO_INT16:
      case RL2_CONVERT_GRID_UINT32_TO_INT16:
      case RL2_CONVERT_GRID_FLOAT_TO_INT16:
      case RL2_CONVERT_GRID_DOUBLE_TO_INT16:
	  p_out_16 = (short *) out;
	  p_out_16 += lineNo * width;
	  break;
      case RL2_CONVERT_GRID_INT8_TO_UINT16:
      case RL2_CONVERT_GRID_UINT8_TO_UINT16:
      case RL2_CONVERT_GRID_INT16_TO_UINT16:
      case RL2_CONVERT_GRID_INT32_TO_UINT16:
      case RL2_CONVERT_GRID_UINT32_TO_UINT16:
      case RL2_CONVERT_GRID_FLOAT_TO_UINT16:
      case RL2_CONVERT_GRID_DOUBLE_TO_UINT16:
	  p_out_u16 = (unsigned short *) out;
	  p_out_u16 += lineNo * width;
	  break;
      case RL2_CONVERT_GRID_INT8_TO_INT32:
      case RL2_CONVERT_GRID_UINT8_TO_INT32:
      case RL2_CONVERT_GRID_INT16_TO_INT32:
      case RL2_CONVERT_GRID_UINT16_TO_INT32:
      case RL2_CONVERT_GRID_UINT32_TO_INT32:
      case RL2_CONVERT_GRID_FLOAT_TO_INT32:
      case RL2_CONVERT_GRID_DOUBLE_TO_INT32:
	  p_out_32 = (int *) out;
	  p_out_32 += lineNo * width;
	  break;
      case RL2_CONVERT_GRID_INT8_TO_UINT32:
      case RL2_CONVERT_GRID_UINT8_TO_UINT32:
      case RL2_CONVERT_GRID_INT16_TO_UINT32:
      case RL2_CONVERT_GRID_UINT16_TO_UINT32:
      case RL2_CONVERT_GRID_INT32_TO_UINT32:
      case RL2_CONVERT_GRID_FLOAT_TO_UINT32:
      case RL2_CONVERT_GRID_DOUBLE_TO_UINT32:
	  p_out_u32 = (unsigned int *) out;
	  p_out_u32 += lineNo * width;
	  break;
      case RL2_CONVERT_GRID_INT8_TO_FLOAT:
      case RL2_CONVERT_GRID_UINT8_TO_FLOAT:
      case RL2_CONVERT_GRID_INT16_TO_FLOAT:
      case RL2_CONVERT_GRID_UINT16_TO_FLOAT:
      case RL2_CONVERT_GRID_INT32_TO_FLOAT:
      case RL2_CONVERT_GRID_UINT32_TO_FLOAT:
      case RL2_CONVERT_GRID_DOUBLE_TO_FLOAT:
	  p_out_flt = (float *) out;
	  p_out_flt += lineNo * width;
	  break;
      case RL2_CONVERT_GRID_INT8_TO_DOUBLE:
      case RL2_CONVERT_GRID_UINT8_TO_DOUBLE:
      case RL2_CONVERT_GRID_INT16_TO_DOUBLE:
      case RL2_CONVERT_GRID_UINT16_TO_DOUBLE:
      case RL2_CONVERT_GRID_INT32_TO_DOUBLE:
      case RL2_CONVERT_GRID_UINT32_TO_DOUBLE:
      case RL2_CONVERT_GRID_FLOAT_TO_DOUBLE:
	  p_out_dbl = (double *) out;
	  p_out_dbl += lineNo * width;
	  break;
      };

    for (x = 0; x < origin->width; x++)
      {
	  if (x >= (startCol + width))
	      break;
	  if (x < startCol)
	    {
		switch (convert)
		  {
		  case RL2_CONVERT_GRID_INT8_TO_UINT8:
		  case RL2_CONVERT_GRID_INT8_TO_INT16:
		  case RL2_CONVERT_GRID_INT8_TO_UINT16:
		  case RL2_CONVERT_GRID_INT8_TO_INT32:
		  case RL2_CONVERT_GRID_INT8_TO_UINT32:
		  case RL2_CONVERT_GRID_INT8_TO_FLOAT:
		  case RL2_CONVERT_GRID_INT8_TO_DOUBLE:
		      p_in_8++;
		      break;
		  case RL2_CONVERT_GRID_UINT8_TO_INT8:
		  case RL2_CONVERT_GRID_UINT8_TO_INT16:
		  case RL2_CONVERT_GRID_UINT8_TO_UINT16:
		  case RL2_CONVERT_GRID_UINT8_TO_INT32:
		  case RL2_CONVERT_GRID_UINT8_TO_UINT32:
		  case RL2_CONVERT_GRID_UINT8_TO_FLOAT:
		  case RL2_CONVERT_GRID_UINT8_TO_DOUBLE:
		      p_in_u8++;
		      break;
		  case RL2_CONVERT_GRID_INT16_TO_INT8:
		  case RL2_CONVERT_GRID_INT16_TO_UINT8:
		  case RL2_CONVERT_GRID_INT16_TO_UINT16:
		  case RL2_CONVERT_GRID_INT16_TO_INT32:
		  case RL2_CONVERT_GRID_INT16_TO_UINT32:
		  case RL2_CONVERT_GRID_INT16_TO_FLOAT:
		  case RL2_CONVERT_GRID_INT16_TO_DOUBLE:
		      p_in_16++;
		      break;
		  case RL2_CONVERT_GRID_UINT16_TO_INT8:
		  case RL2_CONVERT_GRID_UINT16_TO_UINT8:
		  case RL2_CONVERT_GRID_UINT16_TO_INT16:
		  case RL2_CONVERT_GRID_UINT16_TO_INT32:
		  case RL2_CONVERT_GRID_UINT16_TO_UINT32:
		  case RL2_CONVERT_GRID_UINT16_TO_FLOAT:
		  case RL2_CONVERT_GRID_UINT16_TO_DOUBLE:
		      p_in_u16++;
		      break;
		  case RL2_CONVERT_GRID_INT32_TO_INT8:
		  case RL2_CONVERT_GRID_INT32_TO_UINT8:
		  case RL2_CONVERT_GRID_INT32_TO_INT16:
		  case RL2_CONVERT_GRID_INT32_TO_UINT16:
		  case RL2_CONVERT_GRID_INT32_TO_UINT32:
		  case RL2_CONVERT_GRID_INT32_TO_FLOAT:
		  case RL2_CONVERT_GRID_INT32_TO_DOUBLE:
		      p_in_32++;
		      break;
		  case RL2_CONVERT_GRID_UINT32_TO_INT8:
		  case RL2_CONVERT_GRID_UINT32_TO_UINT8:
		  case RL2_CONVERT_GRID_UINT32_TO_INT16:
		  case RL2_CONVERT_GRID_UINT32_TO_UINT16:
		  case RL2_CONVERT_GRID_UINT32_TO_INT32:
		  case RL2_CONVERT_GRID_UINT32_TO_FLOAT:
		  case RL2_CONVERT_GRID_UINT32_TO_DOUBLE:
		      p_in_u32++;
		      break;
		  case RL2_CONVERT_GRID_FLOAT_TO_INT8:
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT8:
		  case RL2_CONVERT_GRID_FLOAT_TO_INT16:
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT16:
		  case RL2_CONVERT_GRID_FLOAT_TO_INT32:
		  case RL2_CONVERT_GRID_FLOAT_TO_UINT32:
		  case RL2_CONVERT_GRID_FLOAT_TO_DOUBLE:
		      p_in_flt++;
		      break;
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT8:
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT8:
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT16:
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT16:
		  case RL2_CONVERT_GRID_DOUBLE_TO_INT32:
		  case RL2_CONVERT_GRID_DOUBLE_TO_UINT32:
		  case RL2_CONVERT_GRID_DOUBLE_TO_FLOAT:
		      p_in_dbl++;
		      break;
		  };
		continue;
	    }

	  switch (convert)
	    {
	    case RL2_CONVERT_GRID_INT8_TO_UINT8:
		*p_out_u8++ = truncate_u8 (*p_in_8++);
		break;
	    case RL2_CONVERT_GRID_INT8_TO_INT16:
		*p_out_16++ = *p_in_8++;
		break;
	    case RL2_CONVERT_GRID_INT8_TO_UINT16:
		*p_out_u16++ = truncate_u16 (*p_in_8++);
		break;
	    case RL2_CONVERT_GRID_INT8_TO_INT32:
		*p_out_32++ = *p_in_8++;
		break;
	    case RL2_CONVERT_GRID_INT8_TO_UINT32:
		*p_out_u32++ = truncate_u32 (*p_in_8++);
		break;
	    case RL2_CONVERT_GRID_INT8_TO_FLOAT:
		*p_out_flt++ = *p_in_8++;
		break;
	    case RL2_CONVERT_GRID_INT8_TO_DOUBLE:
		*p_out_dbl++ = *p_in_8++;
		break;
	    case RL2_CONVERT_GRID_UINT8_TO_INT8:
		*p_out_8++ = truncate_8 (*p_in_u8++);
		break;
	    case RL2_CONVERT_GRID_UINT8_TO_INT16:
		*p_out_16++ = truncate_16 (*p_in_u8++);
		break;
	    case RL2_CONVERT_GRID_UINT8_TO_UINT16:
		*p_out_u16++ = *p_in_u8++;
		break;
	    case RL2_CONVERT_GRID_UINT8_TO_UINT32:
		*p_out_u32++ = *p_in_u8++;
		break;
	    case RL2_CONVERT_GRID_UINT8_TO_INT32:
		*p_out_32++ = truncate_32 (*p_in_u8++);
		break;
	    case RL2_CONVERT_GRID_UINT8_TO_FLOAT:
		*p_out_flt++ = *p_in_u8++;
		break;
	    case RL2_CONVERT_GRID_UINT8_TO_DOUBLE:
		*p_out_dbl++ = *p_in_u8++;
		break;
	    case RL2_CONVERT_GRID_INT16_TO_INT8:
		*p_out_8++ = truncate_8 (*p_in_16++);
		break;
	    case RL2_CONVERT_GRID_INT16_TO_UINT8:
		*p_out_u8++ = truncate_u8 (*p_in_16++);
		break;
	    case RL2_CONVERT_GRID_INT16_TO_UINT16:
		*p_out_u16++ = truncate_u16 (*p_in_16++);
		break;
	    case RL2_CONVERT_GRID_INT16_TO_INT32:
		*p_out_32++ = *p_in_16++;
		break;
	    case RL2_CONVERT_GRID_INT16_TO_UINT32:
		*p_out_u32++ = truncate_u32 (*p_in_16++);
		break;
	    case RL2_CONVERT_GRID_INT16_TO_FLOAT:
		*p_out_flt++ = *p_in_16++;
		break;
	    case RL2_CONVERT_GRID_INT16_TO_DOUBLE:
		*p_out_dbl++ = *p_in_16++;
		break;
	    case RL2_CONVERT_GRID_UINT16_TO_INT8:
		*p_out_8++ = truncate_8 (*p_in_u16++);
		break;
	    case RL2_CONVERT_GRID_UINT16_TO_UINT8:
		*p_out_u8++ = truncate_u8 (*p_in_u16++);
		break;
	    case RL2_CONVERT_GRID_UINT16_TO_INT16:
		*p_out_16++ = truncate_16 (*p_in_u16++);
		break;
	    case RL2_CONVERT_GRID_UINT16_TO_UINT32:
		*p_out_u32++ = *p_in_u16++;
		break;
	    case RL2_CONVERT_GRID_UINT16_TO_INT32:
		*p_out_32++ = truncate_32 (*p_in_u16++);
		break;
	    case RL2_CONVERT_GRID_UINT16_TO_FLOAT:
		*p_out_flt++ = *p_in_u16++;
		break;
	    case RL2_CONVERT_GRID_UINT16_TO_DOUBLE:
		*p_out_dbl++ = *p_in_u16++;
		break;
	    case RL2_CONVERT_GRID_INT32_TO_INT8:
		*p_out_8++ = truncate_8 (*p_in_32++);
		break;
	    case RL2_CONVERT_GRID_INT32_TO_UINT8:
		*p_out_u8++ = truncate_u8 (*p_in_32++);
		break;
	    case RL2_CONVERT_GRID_INT32_TO_INT16:
		*p_out_16++ = truncate_16 (*p_in_32++);
		break;
	    case RL2_CONVERT_GRID_INT32_TO_UINT16:
		*p_out_u16++ = truncate_u16 (*p_in_32++);
		break;
	    case RL2_CONVERT_GRID_INT32_TO_UINT32:
		*p_out_u32++ = truncate_u32 (*p_in_32++);
		break;
	    case RL2_CONVERT_GRID_INT32_TO_FLOAT:
		*p_out_flt++ = *p_in_32++;
		break;
	    case RL2_CONVERT_GRID_INT32_TO_DOUBLE:
		*p_out_dbl++ = *p_in_32++;
		break;
	    case RL2_CONVERT_GRID_UINT32_TO_INT8:
		*p_out_8++ = truncate_8 (*p_in_u32++);
		break;
	    case RL2_CONVERT_GRID_UINT32_TO_UINT8:
		*p_out_u8++ = truncate_u8 (*p_in_u32++);
		break;
	    case RL2_CONVERT_GRID_UINT32_TO_INT16:
		*p_out_16++ = truncate_16 (*p_in_u32++);
		break;
	    case RL2_CONVERT_GRID_UINT32_TO_UINT16:
		*p_out_u16++ = truncate_u16 (*p_in_u32++);
		break;
	    case RL2_CONVERT_GRID_UINT32_TO_INT32:
		*p_out_32++ = truncate_32 (*p_in_u32++);
		break;
	    case RL2_CONVERT_GRID_UINT32_TO_FLOAT:
		*p_out_flt++ = *p_in_u32++;
		break;
	    case RL2_CONVERT_GRID_UINT32_TO_DOUBLE:
		*p_out_dbl++ = *p_in_u32++;
		break;
	    case RL2_CONVERT_GRID_FLOAT_TO_INT8:
		*p_out_8++ = truncate_8 (*p_in_flt++);
		break;
	    case RL2_CONVERT_GRID_FLOAT_TO_UINT8:
		*p_out_u8++ = truncate_u8 (*p_in_flt++);
		break;
	    case RL2_CONVERT_GRID_FLOAT_TO_INT16:
		*p_out_16++ = truncate_16 (*p_in_flt++);
		break;
	    case RL2_CONVERT_GRID_FLOAT_TO_UINT16:
		*p_out_u16++ = truncate_u16 (*p_in_flt++);
		break;
	    case RL2_CONVERT_GRID_FLOAT_TO_INT32:
		*p_out_32++ = truncate_32 (*p_in_flt++);
		break;
	    case RL2_CONVERT_GRID_FLOAT_TO_UINT32:
		*p_out_u32++ = truncate_u32 (*p_in_flt++);
		break;
	    case RL2_CONVERT_GRID_FLOAT_TO_DOUBLE:
		*p_out_dbl++ = *p_in_flt++;
		break;
	    case RL2_CONVERT_GRID_DOUBLE_TO_INT8:
		*p_out_8++ = truncate_8 (*p_in_dbl++);
		break;
	    case RL2_CONVERT_GRID_DOUBLE_TO_UINT8:
		*p_out_u8++ = truncate_u8 (*p_in_dbl++);
		break;
	    case RL2_CONVERT_GRID_DOUBLE_TO_INT16:
		*p_out_16++ = truncate_16 (*p_in_dbl++);
		break;
	    case RL2_CONVERT_GRID_DOUBLE_TO_UINT16:
		*p_out_u16++ = truncate_u16 (*p_in_dbl++);
		break;
	    case RL2_CONVERT_GRID_DOUBLE_TO_INT32:
		*p_out_32++ = truncate_32 (*p_in_dbl++);
		break;
	    case RL2_CONVERT_GRID_DOUBLE_TO_UINT32:
		*p_out_u32++ = truncate_u32 (*p_in_dbl++);
		break;
	    case RL2_CONVERT_GRID_DOUBLE_TO_FLOAT:
		*p_out_flt++ = *p_in_dbl++;
		break;
	    };
      }
}

static int
read_raw_scanlines (rl2PrivTiffOriginPtr origin, unsigned short width,
		    unsigned short height, unsigned char sample_type,
		    unsigned char num_bands, unsigned int startRow,
		    unsigned int startCol, unsigned char *pixels)
{
/* reading TIFF raw strips */
    uint32 line_no;
    uint32 x;
    uint32 y;
    uint32 *tiff_scanline = NULL;
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
    unsigned char bnd;
    unsigned char convert = origin->forced_conversion;
    TIFF *in = (TIFF *) 0;

    tiff_scanline = malloc (TIFFScanlineSize (origin->in));
    if (tiff_scanline == NULL)
	goto error;

/*
/ random access doesn't work on compressed scanlines
/ so we'll open an auxiliary TIFF handle, thus ensuring
/ an always clean reading context
*/
    in = TIFFOpen (origin->path, "r");
    if (in == NULL)
	goto error;

    for (y = 0; y < startRow; y++)
      {
	  /* skipping trailing scanlines */
	  if (TIFFReadScanline (in, tiff_scanline, y, 0) < 0)
	      goto error;
      }

    for (y = 0; y < height; y++)
      {
	  /* scanning scanlines by row */
	  line_no = y + startRow;
	  if (line_no >= origin->height)
	      continue;
	  if (TIFFReadScanline (in, tiff_scanline, line_no, 0) < 0)
	      goto error;
	  if (convert != RL2_CONVERT_NO)
	    {
		/* applying some format conversion */
		copy_convert_scanline (origin, tiff_scanline, pixels, y,
				       startCol, width, convert);
		continue;
	    }
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
		p_in_8 = (char *) tiff_scanline;
		p_out_8 = (char *) pixels;
		p_out_8 += y * width;
		break;
	    case RL2_SAMPLE_UINT8:
		p_in_u8 = (unsigned char *) tiff_scanline;
		p_out_u8 = (unsigned char *) pixels;
		p_out_u8 += y * width * num_bands;
		break;
	    case RL2_SAMPLE_INT16:
		p_in_16 = (short *) tiff_scanline;
		p_out_16 = (short *) pixels;
		p_out_16 += y * width;
		break;
	    case RL2_SAMPLE_UINT16:
		p_in_u16 = (unsigned short *) tiff_scanline;
		p_out_u16 = (unsigned short *) pixels;
		p_out_u16 += y * width * num_bands;
		break;
	    case RL2_SAMPLE_INT32:
		p_in_32 = (int *) tiff_scanline;
		p_out_32 = (int *) pixels;
		p_out_32 += y * width;
		break;
	    case RL2_SAMPLE_UINT32:
		p_in_u32 = (unsigned int *) tiff_scanline;
		p_out_u32 = (unsigned int *) pixels;
		p_out_u32 += y * width;
		break;
	    case RL2_SAMPLE_FLOAT:
		p_in_flt = (float *) tiff_scanline;
		p_out_flt = (float *) pixels;
		p_out_flt += y * width;
		break;
	    case RL2_SAMPLE_DOUBLE:
		p_in_dbl = (double *) tiff_scanline;
		p_out_dbl = (double *) pixels;
		p_out_dbl += y * width;
		break;
	    default:
		goto error;
	    };
	  for (x = 0; x < origin->width; x++)
	    {
		if (x >= (startCol + width))
		    break;
		if (x < startCol)
		  {
		      for (bnd = 0; bnd < num_bands; bnd++)
			{
			    switch (sample_type)
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
		      continue;
		  }
		for (bnd = 0; bnd < num_bands; bnd++)
		  {
		      switch (sample_type)
			{
			case RL2_SAMPLE_INT8:
			    *p_out_8++ = *p_in_8++;
			    break;
			case RL2_SAMPLE_UINT8:
			    *p_out_u8++ = *p_in_u8++;
			    break;
			case RL2_SAMPLE_INT16:
			    *p_out_16++ = *p_in_16++;
			    break;
			case RL2_SAMPLE_UINT16:
			    *p_out_u16++ = *p_in_u16++;
			    break;
			case RL2_SAMPLE_INT32:
			    *p_out_32++ = *p_in_32++;
			    break;
			case RL2_SAMPLE_UINT32:
			    *p_out_u32++ = *p_in_u32++;
			    break;
			case RL2_SAMPLE_FLOAT:
			    *p_out_flt++ = *p_in_flt++;
			    break;
			case RL2_SAMPLE_DOUBLE:
			    *p_out_dbl++ = *p_in_dbl++;
			    break;
			};
		  }
	    }
      }

    free (tiff_scanline);
    TIFFClose (in);
    return RL2_OK;
  error:
    if (tiff_scanline != NULL)
	free (tiff_scanline);
    if (in != (TIFF *) 0)
	TIFFClose (in);
    return RL2_ERROR;
}

static int
read_raw_separate_tiles (rl2PrivTiffOriginPtr origin, unsigned short width,
			 unsigned short height, unsigned char sample_type,
			 unsigned char num_bands, unsigned int startRow,
			 unsigned int startCol, void *pixels)
{
/* reading TIFF raw tiles - separate planes */
    uint32 tile_x;
    uint32 tile_y;
    uint32 x;
    uint32 y;
    uint32 *tiff_tile = NULL;
    unsigned char *p_in_u8;
    unsigned char *p_out_u8;
    unsigned short *p_in_u16;
    unsigned short *p_out_u16;
    unsigned int dest_x;
    unsigned int dest_y;
    int skip;
    unsigned char band;

    if (sample_type != RL2_SAMPLE_UINT16 && sample_type != RL2_SAMPLE_UINT8)
	goto error;

    tiff_tile = malloc (TIFFTileSize (origin->in));
    if (tiff_tile == NULL)
	goto error;

    for (tile_y = 0; tile_y < origin->height; tile_y += origin->tileHeight)
      {
	  /* scanning tiles by row */
	  unsigned int tiff_min_y = tile_y;
	  unsigned int tiff_max_y = tile_y + origin->tileHeight - 1;
	  skip = 0;
	  if (tiff_min_y > (startRow + height))
	      skip = 1;
	  if (tiff_max_y < startRow)
	      skip = 1;
	  if (skip)
	    {
		/* skipping any not required tile */
		continue;
	    }
	  for (tile_x = 0; tile_x < origin->width; tile_x += origin->tileWidth)
	    {
		/* reading a TIFF tile */
		unsigned int tiff_min_x = tile_x;
		unsigned int tiff_max_x = tile_x + origin->tileWidth - 1;
		skip = 0;
		if (tiff_min_x > (startCol + width))
		    skip = 1;
		if (tiff_max_x < startCol)
		    skip = 1;
		if (skip)
		  {
		      /* skipping any not required tile */
		      continue;
		  }
		for (band = 0; band < num_bands; band++)
		  {
		      /* one component for each separate plane */
		      if (TIFFReadTile
			  (origin->in, tiff_tile, tile_x, tile_y, 0, band) < 0)
			  goto error;
		      for (y = 0; y < origin->tileHeight; y++)
			{
			    dest_y = tile_y + y;
			    if (dest_y < startRow
				|| dest_y >= (startRow + height))
				continue;
			    for (x = 0; x < origin->tileWidth; x++)
			      {
				  dest_x = tile_x + x;
				  if (dest_x < startCol
				      || dest_x >= (startCol + width))
				      continue;
				  if (sample_type == RL2_SAMPLE_UINT16)
				    {
					p_in_u16 = (unsigned short *) tiff_tile;
					p_in_u16 += y * origin->tileWidth;
					p_in_u16 += x;
					p_out_u16 = (unsigned short *) pixels;
					p_out_u16 +=
					    ((dest_y -
					      startRow) * width * num_bands) +
					    ((dest_x -
					      startCol) * num_bands) + band;
					*p_out_u16 = *p_in_u16;
				    }
				  if (sample_type == RL2_SAMPLE_UINT8)
				    {
					p_in_u8 = (unsigned char *) tiff_tile;
					p_in_u8 += y * origin->tileWidth;
					p_in_u8 += x;
					p_out_u8 = (unsigned char *) pixels;
					p_out_u8 +=
					    ((dest_y -
					      startRow) * width * num_bands) +
					    ((dest_x -
					      startCol) * num_bands) + band;
					*p_out_u8 = *p_in_u8;
				    }
			      }
			}
		  }
	    }
      }

    free (tiff_tile);
    return RL2_OK;
  error:
    if (tiff_tile != NULL)
	free (tiff_tile);
    return RL2_ERROR;
}

static int
read_raw_separate_scanlines (rl2PrivTiffOriginPtr origin,
			     unsigned short width, unsigned short height,
			     unsigned char sample_type,
			     unsigned char num_bands, unsigned int startRow,
			     unsigned int startCol, void *pixels)
{
/* reading TIFF raw strips - separate planes */
    uint32 line_no;
    uint32 x;
    uint32 y;
    uint32 *tiff_scanline = NULL;
    unsigned char *p_in_u8 = NULL;
    unsigned char *p_out_u8 = NULL;
    unsigned char *p_out_u8_base = NULL;
    unsigned short *p_in_u16 = NULL;
    unsigned short *p_out_u16 = NULL;
    unsigned short *p_out_u16_base = NULL;
    unsigned char band;
    TIFF *in = (TIFF *) 0;

    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;

    tiff_scanline = malloc (TIFFScanlineSize (origin->in));
    if (tiff_scanline == NULL)
	goto error;

    for (band = 0; band < num_bands; band++)
      {
	  /* one component for each separate plane */

/*
/ random access doesn't work on compressed scanlines
/ so we'll open an auxiliary TIFF handle, thus ensuring
/ an always clean reading context
*/
	  in = TIFFOpen (origin->path, "r");
	  if (in == NULL)
	      goto error;

	  for (y = 0; y < startRow; y++)
	    {
		/* skipping trailing scanlines */
		if (TIFFReadScanline (in, tiff_scanline, y, band) < 0)
		    goto error;
	    }
	  for (y = 0; y < height; y++)
	    {
		/* scanning scanlines by row */
		line_no = y + startRow;
		if (line_no >= origin->height)
		    continue;
		if (TIFFReadScanline (in, tiff_scanline, line_no, band) < 0)
		    goto error;
		if (sample_type == RL2_SAMPLE_UINT16)
		  {
		      p_in_u16 = (unsigned short *) tiff_scanline;
		      p_in_u16 += startCol;
		      p_out_u16_base = (unsigned short *) pixels;
		      p_out_u16_base += y * width * num_bands;
		  }
		else
		  {
		      p_in_u8 = (unsigned char *) tiff_scanline;
		      p_in_u8 += startCol;
		      p_out_u8_base = (unsigned char *) pixels;
		      p_out_u8_base += y * width * num_bands;
		  }
		for (x = startCol; x < origin->width; x++)
		  {
		      if (x >= (startCol + width))
			  break;
		      if (sample_type == RL2_SAMPLE_UINT16)
			  p_out_u16 =
			      p_out_u16_base + ((x - startCol) * num_bands) +
			      band;
		      else
			  p_out_u8 =
			      p_out_u8_base + ((x - startCol) * num_bands) +
			      band;
		      if (sample_type == RL2_SAMPLE_UINT16)
			  *p_out_u16 = *p_in_u16++;
		      else
			  *p_out_u8 = *p_in_u8++;
		  }
	    }
	  TIFFClose (in);
	  in = (TIFF *) 0;
      }

    free (tiff_scanline);
    return RL2_OK;
  error:
    if (tiff_scanline != NULL)
	free (tiff_scanline);
    if (in != (TIFF *) 0)
	TIFFClose (in);
    return RL2_ERROR;
}

static int
read_RGBA_tiles (rl2PrivTiffOriginPtr origin, unsigned short width,
		 unsigned short height, unsigned char pixel_type,
		 unsigned char num_bands, unsigned int startRow,
		 unsigned int startCol, unsigned char *pixels,
		 rl2PalettePtr palette)
{
/* reading TIFF RGBA tiles */
    uint32 tile_x;
    uint32 tile_y;
    uint32 x;
    uint32 y;
    uint32 pix;
    uint32 *tiff_tile = NULL;
    uint32 *p_in;
    unsigned char *p_out;
    unsigned int dest_x;
    unsigned int dest_y;
    int skip;
    unsigned char index;
    unsigned char red;
    unsigned char green;
    unsigned char blue;

    tiff_tile =
	malloc (sizeof (uint32) * origin->tileWidth * origin->tileHeight);
    if (tiff_tile == NULL)
	goto error;

    for (tile_y = 0; tile_y < origin->height; tile_y += origin->tileHeight)
      {
	  /* scanning tiles by row */
	  unsigned int tiff_min_y = tile_y;
	  unsigned int tiff_max_y = tile_y + origin->tileHeight - 1;
	  skip = 0;
	  if (tiff_min_y > (startRow + height))
	      skip = 1;
	  if (tiff_max_y < startRow)
	      skip = 1;
	  if (skip)
	    {
		/* skipping any not required tile */
		continue;
	    }
	  for (tile_x = 0; tile_x < origin->width; tile_x += origin->tileWidth)
	    {
		/* reading a TIFF tile */
		unsigned int tiff_min_x = tile_x;
		unsigned int tiff_max_x = tile_x + origin->tileWidth - 1;
		skip = 0;
		if (tiff_min_x > (startCol + width))
		    skip = 1;
		if (tiff_max_x < startCol)
		    skip = 1;
		if (skip)
		  {
		      /* skipping any not required tile */
		      continue;
		  }
		if (!TIFFReadRGBATile (origin->in, tile_x, tile_y, tiff_tile))
		    goto error;
		for (y = 0; y < origin->tileHeight; y++)
		  {
		      dest_y = tile_y + (origin->tileHeight - y) - 1;
		      if (dest_y < startRow || dest_y >= (startRow + height))
			  continue;
		      p_in = tiff_tile + (origin->tileWidth * y);
		      for (x = 0; x < origin->tileWidth; x++)
			{
			    dest_x = tile_x + x;
			    if (dest_x < startCol
				|| dest_x >= (startCol + width))
			      {
				  p_in++;
				  continue;
			      }
			    p_out =
				pixels +
				((dest_y - startRow) * width * num_bands) +
				((dest_x - startCol) * num_bands);
			    pix = *p_in++;
			    red = TIFFGetR (pix);
			    green = TIFFGetG (pix);
			    blue = TIFFGetB (pix);
			    if (origin->forced_conversion ==
				RL2_CONVERT_RGB_TO_GRAYSCALE)
			      {
				  /* forced conversion: RGB -> GRAYSCALE */
				  double gray =
				      ((double) red * 0.2126) +
				      ((double) green * 0.7152) +
				      ((double) blue * 0.0722);
				  *p_out++ = (unsigned char) gray;
			      }
			    else if (origin->forced_conversion ==
				     RL2_CONVERT_GRAYSCALE_TO_RGB
				     || origin->forced_conversion ==
				     RL2_CONVERT_PALETTE_TO_RGB)
			      {
				  /* forced conversion: GRAYSCALE or PALETTE -> RGB */
				  *p_out++ = red;
				  *p_out++ = green;
				  *p_out++ = blue;
			      }
			    else if (origin->forced_conversion ==
				     RL2_CONVERT_GRAYSCALE_TO_PALETTE)
			      {
				  /* forced conversion: GRAYSCALE -> PALETTE */
				  *p_out++ = red;
			      }
			    else if (origin->forced_conversion ==
				     RL2_CONVERT_PALETTE_TO_GRAYSCALE)
			      {
				  /* forced conversion: PALETTE -> GRAYSCALE */
				  *p_out++ = red;
			      }
			    else if (origin->forced_conversion ==
				     RL2_CONVERT_PALETTE_TO_MONOCHROME)
			      {
				  /* forced conversion: PALETTE -> MONOCHROME */
				  if (red == 255 && green == 255 && blue == 255)
				      *p_out++ = 0;
				  else
				      *p_out++ = 1;
			      }
			    else if (origin->forced_conversion ==
				     RL2_CONVERT_MONOCHROME_TO_PALETTE)
			      {
				  /* forced conversion: MONOCHROME -> PALETTE */
				  if (red == 0)
				      *p_out++ = 1;
				  else
				      *p_out++ = 0;
			      }
			    else if (pixel_type == RL2_PIXEL_PALETTE)
			      {
				  /* PALETTE image */
				  if (rl2_get_palette_index
				      (palette, &index, red, green,
				       blue) != RL2_OK)
				      index = 0;
				  *p_out++ = index;
			      }
			    else if (pixel_type == RL2_PIXEL_MONOCHROME)
			      {
				  /* MONOCHROME  image */
				  if (red == 0)
				      *p_out++ = 1;
				  else
				      *p_out++ = 0;
			      }
			    else if (pixel_type == RL2_PIXEL_GRAYSCALE)
			      {
				  /* GRAYSCALE  image */
				  *p_out++ = red;
			      }
			    else
			      {
				  /* should be an RGB image */
				  *p_out++ = red;
				  *p_out++ = green;
				  *p_out++ = blue;
			      }
			}
		  }
	    }
      }

    free (tiff_tile);
    return RL2_OK;
  error:
    if (tiff_tile != NULL)
	free (tiff_tile);
    return RL2_ERROR;
}

static int
read_RGBA_strips (rl2PrivTiffOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned char pixel_type,
		  unsigned char num_bands, unsigned int startRow,
		  unsigned int startCol, unsigned char *pixels,
		  rl2PalettePtr palette)
{
/* reading TIFF RGBA strips */
    uint32 strip;
    uint32 x;
    uint32 y;
    uint32 pix;
    uint32 *tiff_strip = NULL;
    uint32 *p_in;
    unsigned char *p_out;
    unsigned int dest_x;
    unsigned int dest_y;
    int skip;
    unsigned char index;
    unsigned char red;
    unsigned char green;
    unsigned char blue;

    tiff_strip =
	malloc (sizeof (uint32) * origin->width * origin->rowsPerStrip);
    if (tiff_strip == NULL)
	goto error;

    for (strip = 0; strip < origin->height; strip += origin->rowsPerStrip)
      {
	  /* scanning strips by row */
	  unsigned int tiff_min_y = strip;
	  unsigned int tiff_max_y = strip + origin->rowsPerStrip - 1;
	  skip = 0;
	  if (tiff_min_y > (startRow + height))
	      skip = 1;
	  if (tiff_max_y < startRow)
	      skip = 1;
	  if (skip)
	    {
		/* skipping any not required strip */
		continue;
	    }
	  if (!TIFFReadRGBAStrip (origin->in, strip, tiff_strip))
	      goto error;
	  for (y = 0; y < origin->rowsPerStrip; y++)
	    {
		dest_y = strip + (origin->rowsPerStrip - y) - 1;
		if (dest_y < startRow || dest_y >= (startRow + height))
		    continue;
		p_in = tiff_strip + (origin->width * y);
		for (x = 0; x < origin->width; x++)
		  {
		      dest_x = x;
		      if (dest_x < startCol || dest_x >= (startCol + width))
			{
			    p_in++;
			    continue;
			}
		      p_out =
			  pixels + ((dest_y - startRow) * width * num_bands) +
			  ((dest_x - startCol) * num_bands);
		      pix = *p_in++;
		      red = TIFFGetR (pix);
		      green = TIFFGetG (pix);
		      blue = TIFFGetB (pix);
		      if (origin->forced_conversion ==
			  RL2_CONVERT_RGB_TO_GRAYSCALE)
			{
			    /* forced conversion: RGB -> GRAYSCALE */
			    double gray =
				((double) red * 0.2126) +
				((double) green * 0.7152) +
				((double) blue * 0.0722);
			    *p_out++ = (unsigned char) gray;
			}
		      else if (origin->forced_conversion ==
			       RL2_CONVERT_GRAYSCALE_TO_RGB
			       || origin->forced_conversion ==
			       RL2_CONVERT_PALETTE_TO_RGB)
			{
			    /* forced conversion: GRAYSCALE or PALETTE -> RGB */
			    *p_out++ = red;
			    *p_out++ = green;
			    *p_out++ = blue;
			}
		      else if (origin->forced_conversion ==
			       RL2_CONVERT_GRAYSCALE_TO_PALETTE)
			{
			    /* forced conversion: GRAYSCALE -> PALETTE */
			    *p_out++ = red;
			}
		      else if (origin->forced_conversion ==
			       RL2_CONVERT_PALETTE_TO_GRAYSCALE)
			{
			    /* forced conversion: PALETTE -> GRAYSCALE */
			    *p_out++ = red;
			}
		      else if (origin->forced_conversion ==
			       RL2_CONVERT_PALETTE_TO_MONOCHROME)
			{
			    /* forced conversion: PALETTE -> MONOCHROME */
			    if (red == 255 && green == 255 && blue == 255)
				*p_out++ = 0;
			    else
				*p_out++ = 1;
			}
		      else if (origin->forced_conversion ==
			       RL2_CONVERT_MONOCHROME_TO_PALETTE)
			{
			    /* forced conversion: MONOCHROME -> PALETTE */
			    if (red == 0)
				*p_out++ = 1;
			    else
				*p_out++ = 0;
			}
		      else if (pixel_type == RL2_PIXEL_PALETTE)
			{
			    /* PALETTE image */
			    if (rl2_get_palette_index
				(palette, &index, red, green, blue) != RL2_OK)
				index = 0;
			    *p_out++ = index;
			}
		      else if (pixel_type == RL2_PIXEL_MONOCHROME)
			{
			    /* MONOCHROME  image */
			    if (red == 0)
				*p_out++ = 1;
			    else
				*p_out++ = 0;
			}
		      else if (pixel_type == RL2_PIXEL_GRAYSCALE)
			{
			    /* GRAYSCALE  image */
			    *p_out++ = red;
			}
		      else
			{
			    /* should be an RGB image */
			    *p_out++ = red;
			    *p_out++ = green;
			    *p_out++ = blue;
			}
		  }
	    }
      }

    free (tiff_strip);
    return RL2_OK;
  error:
    if (tiff_strip != NULL)
	free (tiff_strip);
    return RL2_ERROR;
}

static int
read_from_tiff (rl2PrivTiffOriginPtr origin, unsigned short width,
		unsigned short height, unsigned char sample_type,
		unsigned char pixel_type, unsigned char num_bands,
		unsigned int startRow, unsigned int startCol,
		unsigned char **pixels, int *pixels_sz, rl2PalettePtr palette)
{
/* creating a tile from the Tiff origin */
    int ret;
    unsigned char *bufPixels = NULL;
    int bufPixelsSz = 0;
    int pix_sz = 1;

/* allocating the pixels buffer */
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
    bufPixelsSz = width * height * pix_sz * num_bands;
    bufPixels = malloc (bufPixelsSz);
    if (bufPixels == NULL)
	goto error;
    if ((startRow + height) > origin->height
	|| (startCol + width) > origin->width)
	rl2_prime_void_tile (bufPixels, width, height, sample_type, num_bands,
			     NULL);

    if (origin->planarConfig == PLANARCONFIG_SEPARATE)
      {
	  /* separate planes configuration */
	  if ((origin->bitsPerSample == 16 || origin->bitsPerSample == 8)
	      && origin->sampleFormat == SAMPLEFORMAT_UINT)
	    {
		/* using raw TIFF methods - separate planes */
		if (origin->isTiled)
		    ret =
			read_raw_separate_tiles (origin, width, height,
						 sample_type, num_bands,
						 startRow, startCol,
						 (void *) bufPixels);
		else
		    ret =
			read_raw_separate_scanlines (origin, width, height,
						     sample_type, num_bands,
						     startRow, startCol,
						     (void *) bufPixels);
		if (ret != RL2_OK)
		    goto error;
	    }
	  else
	      goto error;
      }
    else
      {
	  /* contiguous planar configuration */
	  if (origin->bitsPerSample <= 8
	      && origin->sampleFormat == SAMPLEFORMAT_UINT
	      && (origin->samplesPerPixel == 1
		  || origin->samplesPerPixel == 3)
	      && (pixel_type == RL2_PIXEL_MONOCHROME
		  || pixel_type == RL2_PIXEL_PALETTE
		  || pixel_type == RL2_PIXEL_GRAYSCALE
		  || pixel_type == RL2_PIXEL_RGB))
	    {
		/* using the TIFF RGBA methods */
		if (origin->isTiled)
		    ret =
			read_RGBA_tiles (origin, width, height, pixel_type,
					 num_bands, startRow, startCol,
					 bufPixels, palette);
		else
		    ret =
			read_RGBA_strips (origin, width, height, pixel_type,
					  num_bands, startRow, startCol,
					  bufPixels, palette);
		if (ret != RL2_OK)
		    goto error;
	    }
	  else
	    {
		/* using raw TIFF methods */
		if (origin->isTiled)
		    ret =
			read_raw_tiles (origin, width, height, sample_type,
					num_bands, startRow, startCol,
					bufPixels);
		else
		    ret =
			read_raw_scanlines (origin, width, height,
					    sample_type, num_bands, startRow,
					    startCol, bufPixels);
		if (ret != RL2_OK)
		    goto error;
	    }
      }

    *pixels = bufPixels;
    *pixels_sz = bufPixelsSz;
    return RL2_OK;
  error:
    if (bufPixels != NULL)
	free (bufPixels);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_is_tiled_tiff_origin (rl2TiffOriginPtr tiff, int *is_tiled)
{
/* testing if the TIFF Origin is tiled or not */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    *is_tiled = origin->isTiled;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_tile_size (rl2TiffOriginPtr tiff,
			       unsigned int *tile_width,
			       unsigned int *tile_height)
{
/* attempting to return the Tile dimensions */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    if (origin->isTiled == 0)
	return RL2_ERROR;
    *tile_width = origin->tileWidth;
    *tile_height = origin->tileHeight;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_origin_strip_size (rl2TiffOriginPtr tiff, unsigned int *strip_size)
{
/* attempting to return the Strip dimension */
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (origin == NULL)
	return RL2_ERROR;
    if (origin->isTiled != 0)
	return RL2_ERROR;
    *strip_size = origin->rowsPerStrip;
    return RL2_OK;
}

static void
build_remap (rl2PrivTiffOriginPtr origin)
{
/* building a remapped palette identical to the natural one */
    int j;
    if (origin->remapRed != NULL)
	free (origin->remapRed);
    if (origin->remapGreen != NULL)
	free (origin->remapGreen);
    if (origin->remapBlue != NULL)
	free (origin->remapBlue);
    origin->remapMaxPalette = origin->maxPalette;
    origin->remapRed = malloc (origin->remapMaxPalette);
    origin->remapGreen = malloc (origin->remapMaxPalette);
    origin->remapBlue = malloc (origin->remapMaxPalette);
    for (j = 0; j < origin->maxPalette; j++)
      {
	  origin->remapRed[j] = origin->red[j];
	  origin->remapGreen[j] = origin->green[j];
	  origin->remapBlue[j] = origin->blue[j];
      }
}

RL2_DECLARE rl2RasterPtr
rl2_get_tile_from_tiff_origin (rl2CoveragePtr cvg, rl2TiffOriginPtr tiff,
			       unsigned int startRow, unsigned int startCol,
			       int force_srid, int verbose)
{
/* attempting to create a Coverage-tile from a Tiff origin */
    unsigned int x;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    rl2RasterPtr raster = NULL;
    rl2PalettePtr palette = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    unsigned int unused_width = 0;
    unsigned int unused_height = 0;

    if (coverage == NULL || tiff == NULL)
	return NULL;
    if (rl2_eval_tiff_origin_compatibility (cvg, tiff, force_srid, verbose) !=
	RL2_TRUE)
	return NULL;

/* testing for tile's boundary validity */
    if (startCol > origin->width)
	return NULL;
    if (startRow > origin->height)
	return NULL;
    x = startCol / coverage->tileWidth;
    if ((x * coverage->tileWidth) != startCol)
	return NULL;
    x = startRow / coverage->tileHeight;
    if ((x * coverage->tileHeight) != startRow)
	return NULL;

    if ((origin->photometric == PHOTOMETRIC_RGB
	 && origin->forced_pixel_type == RL2_PIXEL_PALETTE)
	|| origin->forced_conversion == RL2_CONVERT_GRAYSCALE_TO_PALETTE)
      {
	  /* creating a remapped Palette */
	  if (origin->remapMaxPalette == 0 && origin->maxPalette > 0
	      && origin->maxPalette <= 256)
	      build_remap (origin);
	  palette = rl2_create_palette (origin->remapMaxPalette);
	  for (x = 0; x < origin->remapMaxPalette; x++)
	    {
		rl2_set_palette_color (palette, x, origin->remapRed[x],
				       origin->remapGreen[x],
				       origin->remapBlue[x]);
	    }
      }
    else if ((origin->photometric < PHOTOMETRIC_RGB
	      && origin->forced_pixel_type == RL2_PIXEL_PALETTE)
	     || origin->forced_conversion == RL2_CONVERT_MONOCHROME_TO_PALETTE)
      {
	  /* creating a remapped Palette */
	  if (origin->remapMaxPalette == 0 && origin->maxPalette > 0
	      && origin->maxPalette <= 2)
	      build_remap (origin);
	  palette = rl2_create_palette (origin->remapMaxPalette);
	  for (x = 0; x < origin->remapMaxPalette; x++)
	    {
		rl2_set_palette_color (palette, x, origin->remapRed[x],
				       origin->remapGreen[x],
				       origin->remapBlue[x]);
	    }
      }
    if (origin->photometric == PHOTOMETRIC_PALETTE)
      {
	  /* creating an ordinary Palette */
	  if (origin->remapMaxPalette > 0)
	    {
		palette = rl2_create_palette (origin->remapMaxPalette);
		for (x = 0; x < origin->remapMaxPalette; x++)
		  {
		      rl2_set_palette_color (palette, x,
					     origin->remapRed[x],
					     origin->remapGreen[x],
					     origin->remapBlue[x]);
		  }
	    }
	  else
	    {
		if (origin->maxPalette > 0)
		  {
		      palette = rl2_create_palette (origin->maxPalette);
		      for (x = 0; x < origin->maxPalette; x++)
			{
			    rl2_set_palette_color (palette, x, origin->red[x],
						   origin->green[x],
						   origin->blue[x]);
			}
		  }
	    }
      }

/* attempting to create the tile */
    if (read_from_tiff
	(origin, coverage->tileWidth, coverage->tileHeight,
	 coverage->sampleType, coverage->pixelType, coverage->nBands,
	 startRow, startCol, &pixels, &pixels_sz, palette) != RL2_OK)
	goto error;
    if (startCol + coverage->tileWidth > origin->width)
	unused_width = (startCol + coverage->tileWidth) - origin->width;
    if (startRow + coverage->tileHeight > origin->height)
	unused_height = (startRow + coverage->tileHeight) - origin->height;
    if (unused_width || unused_height)
      {
	  /* 
	   * creating a Transparency Mask so to shadow any 
	   * unused portion of the current tile 
	   */
	  unsigned int shadow_x = coverage->tileWidth - unused_width;
	  unsigned int shadow_y = coverage->tileHeight - unused_height;
	  unsigned int row;
	  mask_size = coverage->tileWidth * coverage->tileHeight;
	  mask = malloc (mask_size);
	  if (mask == NULL)
	      goto error;
	  /* full Transparent mask */
	  memset (mask, 0, coverage->tileWidth * coverage->tileHeight);
	  for (row = 0; row < coverage->tileHeight; row++)
	    {
		unsigned char *p = mask + (row * coverage->tileWidth);
		if (row < shadow_y)
		  {
		      /* setting opaque pixels */
		      memset (p, 1, shadow_x);
		  }
	    }
      }

    if (origin->forced_conversion == RL2_CONVERT_PALETTE_TO_MONOCHROME ||
	origin->forced_conversion == RL2_CONVERT_PALETTE_TO_GRAYSCALE ||
	origin->forced_conversion == RL2_CONVERT_PALETTE_TO_RGB)
      {
	  rl2_destroy_palette (palette);
	  palette = NULL;
      }
    raster =
	rl2_create_raster (coverage->tileWidth, coverage->tileHeight,
			   coverage->sampleType, coverage->pixelType,
			   coverage->nBands, pixels, pixels_sz, palette, mask,
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

static rl2PrivTiffDestinationPtr
create_tiff_destination (const char *path, int is_geo_tiff)
{
/* creating an uninitialized TIFF destination */
    int len;
    rl2PrivTiffDestinationPtr destination;
    if (path == NULL)
	return NULL;
    destination = malloc (sizeof (rl2PrivTiffDestination));
    if (destination == NULL)
	return NULL;

    len = strlen (path);
    destination->path = malloc (len + 1);
    strcpy (destination->path, path);
    destination->isGeoTiff = is_geo_tiff;
    destination->out = (TIFF *) 0;
    destination->gtif = (GTIF *) 0;
    destination->tiffBuffer = NULL;
    destination->tileWidth = 256;
    destination->tileHeight = 256;
    destination->maxPalette = 0;
    destination->red = NULL;
    destination->green = NULL;
    destination->blue = NULL;
    destination->isGeoReferenced = 0;
    destination->Srid = -1;
    destination->srsName = NULL;
    destination->proj4text = NULL;
    return destination;
}

RL2_DECLARE void
rl2_destroy_tiff_destination (rl2TiffDestinationPtr tiff)
{
/* memory cleanup - destroying a TIFF destination */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return;
    if (destination->isGeoTiff)
      {
	  /* it's a GeoTiff */
	  if (destination->gtif != (GTIF *) 0)
	      GTIFFree (destination->gtif);
	  if (destination->out != (TIFF *) 0)
	      XTIFFClose (destination->out);
      }
    else
      {
	  /* it's a plain ordinary Tiff */
	  if (destination->out != (TIFF *) 0)
	      TIFFClose (destination->out);
      }
    if (destination->path != NULL)
	free (destination->path);
    if (destination->tfw_path != NULL)
	free (destination->tfw_path);
    if (destination->tiffBuffer != NULL)
	free (destination->tiffBuffer);
    if (destination->red != NULL)
	free (destination->red);
    if (destination->green != NULL)
	free (destination->green);
    if (destination->blue != NULL)
	free (destination->blue);
    if (destination->srsName != NULL)
	free (destination->srsName);
    if (destination->proj4text != NULL)
	free (destination->proj4text);
    free (destination);
}

static int
check_color_model (unsigned char sample_type, unsigned char pixel_type,
		   unsigned char num_bands, rl2PalettePtr plt,
		   unsigned char compression)
{
/* checking TIFF arguments for self consistency */
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
	  if (sample_type != RL2_SAMPLE_1_BIT || num_bands != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_CCITTFAX3:
	    case RL2_COMPRESSION_CCITTFAX4:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return 0;
	    };
	  if (num_bands != 1)
	      return 0;
	  if (plt == NULL)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_LZMA:
	    case RL2_COMPRESSION_LZW:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return 0;
	    };
	  if (num_bands != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_LZMA:
	    case RL2_COMPRESSION_LZW:
	    case RL2_COMPRESSION_JPEG:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_RGB:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_UINT16:
		break;
	    default:
		return 0;
	    };
	  if (num_bands != 3)
	      return 0;
	  if (sample_type == RL2_SAMPLE_UINT16)
	    {
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		  case RL2_COMPRESSION_DEFLATE:
		  case RL2_COMPRESSION_LZMA:
		  case RL2_COMPRESSION_LZW:
		      break;
		  default:
		      return 0;
		  };
	    }
	  else
	    {
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		  case RL2_COMPRESSION_DEFLATE:
		  case RL2_COMPRESSION_LZMA:
		  case RL2_COMPRESSION_LZW:
		  case RL2_COMPRESSION_JPEG:
		      break;
		  default:
		      return 0;
		  };
	    }
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_UINT16:
	    case RL2_SAMPLE_INT32:
	    case RL2_SAMPLE_UINT32:
	    case RL2_SAMPLE_FLOAT:
	    case RL2_SAMPLE_DOUBLE:
		break;
	    default:
		return 0;
	    };
	  if (num_bands != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_LZMA:
	    case RL2_COMPRESSION_LZW:
		break;
	    default:
		return 0;
	    };
	  break;
      };
    return 1;
}

static int
set_tiff_destination (rl2PrivTiffDestinationPtr destination,
		      unsigned short width, unsigned short height,
		      unsigned char sample_type, unsigned char pixel_type,
		      unsigned char num_bands, rl2PalettePtr plt,
		      unsigned char tiff_compression)
{
/* setting up the TIFF headers */
    int i;
    uint16 r_plt[256];
    uint16 g_plt[256];
    uint16 b_plt[256];
    tsize_t buf_size;
    void *tiff_buffer = NULL;

    TIFFSetField (destination->out, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField (destination->out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField (destination->out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField (destination->out, TIFFTAG_XRESOLUTION, 300.0);
    TIFFSetField (destination->out, TIFFTAG_YRESOLUTION, 300.0);
    TIFFSetField (destination->out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    if (pixel_type == RL2_PIXEL_MULTIBAND)
	TIFFSetField (destination->out, TIFFTAG_PLANARCONFIG,
		      PLANARCONFIG_SEPARATE);
    else if (pixel_type == RL2_PIXEL_RGB && sample_type == RL2_SAMPLE_UINT16)
	TIFFSetField (destination->out, TIFFTAG_PLANARCONFIG,
		      PLANARCONFIG_SEPARATE);
    else
	TIFFSetField (destination->out, TIFFTAG_PLANARCONFIG,
		      PLANARCONFIG_CONTIG);
    TIFFSetField (destination->out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    if (pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  /* MONOCHROME */
	  destination->sampleFormat = SAMPLEFORMAT_UINT;
	  destination->bitsPerSample = 1;
	  destination->samplesPerPixel = 1;
	  destination->photometric = PHOTOMETRIC_MINISWHITE;
	  TIFFSetField (destination->out, TIFFTAG_SAMPLEFORMAT,
			SAMPLEFORMAT_UINT);
	  TIFFSetField (destination->out, TIFFTAG_SAMPLESPERPIXEL, 1);
	  TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE, 1);
	  TIFFSetField (destination->out, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
	  TIFFSetField (destination->out, TIFFTAG_PHOTOMETRIC,
			PHOTOMETRIC_MINISWHITE);
	  if (tiff_compression == RL2_COMPRESSION_CCITTFAX3)
	    {
		destination->compression = COMPRESSION_CCITTFAX3;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_CCITTFAX3);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_CCITTFAX4)
	    {
		destination->compression = COMPRESSION_CCITTFAX4;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_CCITTFAX4);
	    }
	  else
	    {
		destination->compression = COMPRESSION_NONE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_NONE);
	    }
	  goto header_done;
      }
    else if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  /* PALETTE */
	  unsigned short max_palette;
	  unsigned char *red;
	  unsigned char *green;
	  unsigned char *blue;
	  if (rl2_get_palette_colors
	      (plt, &max_palette, &red, &green, &blue) == RL2_ERROR)
	    {
		fprintf (stderr, "RL2-TIFF writer: invalid Palette\n");
		goto error;
	    }
	  for (i = 0; i < 256; i++)
	    {
		r_plt[i] = 0;
		g_plt[i] = 0;
		b_plt[i] = 0;
	    }
	  for (i = 0; i < max_palette; i++)
	    {
		r_plt[i] = red[i] * 256;
		g_plt[i] = green[i] * 256;
		b_plt[i] = blue[i] * 256;
	    }
	  rl2_free (red);
	  rl2_free (green);
	  rl2_free (blue);
	  destination->sampleFormat = SAMPLEFORMAT_UINT;
	  destination->bitsPerSample = 8;
	  destination->samplesPerPixel = 1;
	  destination->photometric = PHOTOMETRIC_PALETTE;
	  TIFFSetField (destination->out, TIFFTAG_SAMPLEFORMAT,
			SAMPLEFORMAT_UINT);
	  TIFFSetField (destination->out, TIFFTAG_SAMPLESPERPIXEL, 1);
	  TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (destination->out, TIFFTAG_PHOTOMETRIC,
			PHOTOMETRIC_PALETTE);
	  TIFFSetField (destination->out, TIFFTAG_COLORMAP, r_plt, g_plt,
			b_plt);
	  if (tiff_compression == RL2_COMPRESSION_LZW)
	    {
		destination->compression = COMPRESSION_LZW;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZW);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_DEFLATE)
	    {
		destination->compression = COMPRESSION_DEFLATE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_DEFLATE);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_LZMA)
	    {
		destination->compression = COMPRESSION_LZMA;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZMA);
	    }
	  else
	    {
		destination->compression = COMPRESSION_NONE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_NONE);
	    }
	  goto header_done;
      }
    else if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  /* GRAYSCALE */
	  destination->sampleFormat = SAMPLEFORMAT_UINT;
	  destination->bitsPerSample = 8;
	  destination->samplesPerPixel = 1;
	  destination->photometric = PHOTOMETRIC_MINISBLACK;
	  TIFFSetField (destination->out, TIFFTAG_SAMPLEFORMAT,
			SAMPLEFORMAT_UINT);
	  TIFFSetField (destination->out, TIFFTAG_SAMPLESPERPIXEL, 1);
	  TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (destination->out, TIFFTAG_PHOTOMETRIC,
			PHOTOMETRIC_MINISBLACK);
	  if (tiff_compression == RL2_COMPRESSION_LZW)
	    {
		destination->compression = COMPRESSION_LZW;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZW);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_DEFLATE)
	    {
		destination->compression = COMPRESSION_DEFLATE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_DEFLATE);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_LZMA)
	    {
		destination->compression = COMPRESSION_LZMA;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZMA);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_JPEG)
	    {
		destination->compression = COMPRESSION_JPEG;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_JPEG);
	    }
	  else
	    {
		destination->compression = COMPRESSION_NONE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_NONE);
	    }
	  goto header_done;
      }
    else if (pixel_type == RL2_PIXEL_RGB)
      {
	  /* RGB */
	  destination->sampleFormat = SAMPLEFORMAT_UINT;
	  if (sample_type == RL2_SAMPLE_UINT16)
	      destination->bitsPerSample = 16;
	  else
	      destination->bitsPerSample = 8;
	  destination->samplesPerPixel = 3;
	  destination->photometric = PHOTOMETRIC_RGB;
	  TIFFSetField (destination->out, TIFFTAG_SAMPLEFORMAT,
			SAMPLEFORMAT_UINT);
	  TIFFSetField (destination->out, TIFFTAG_SAMPLESPERPIXEL, 3);
	  TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE,
			destination->bitsPerSample);
	  TIFFSetField (destination->out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	  if (tiff_compression == RL2_COMPRESSION_LZW)
	    {
		destination->compression = COMPRESSION_LZW;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZW);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_DEFLATE)
	    {
		destination->compression = COMPRESSION_DEFLATE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_DEFLATE);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_LZMA)
	    {
		destination->compression = COMPRESSION_LZMA;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZMA);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_JPEG)
	    {
		destination->compression = COMPRESSION_JPEG;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_JPEG);
	    }
	  else
	    {
		destination->compression = COMPRESSION_NONE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_NONE);
	    }
	  goto header_done;
      }
    else if (pixel_type == RL2_PIXEL_DATAGRID)
      {
	  /* GRID data */
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_INT32:
		destination->sampleFormat = SAMPLEFORMAT_INT;
		TIFFSetField (destination->out, TIFFTAG_SAMPLEFORMAT,
			      SAMPLEFORMAT_INT);
		break;
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_UINT16:
	    case RL2_SAMPLE_UINT32:
		destination->sampleFormat = SAMPLEFORMAT_UINT;
		TIFFSetField (destination->out, TIFFTAG_SAMPLEFORMAT,
			      SAMPLEFORMAT_UINT);
		break;
	    case RL2_SAMPLE_FLOAT:
	    case RL2_SAMPLE_DOUBLE:
		destination->sampleFormat = SAMPLEFORMAT_IEEEFP;
		TIFFSetField (destination->out, TIFFTAG_SAMPLEFORMAT,
			      SAMPLEFORMAT_IEEEFP);
		break;
	    };
	  destination->samplesPerPixel = 1;
	  TIFFSetField (destination->out, TIFFTAG_SAMPLESPERPIXEL, 1);
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
	    case RL2_SAMPLE_UINT8:
		destination->bitsPerSample = 8;
		TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE, 8);
		break;
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_UINT16:
		destination->bitsPerSample = 16;
		TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE, 16);
		break;
	    case RL2_SAMPLE_INT32:
	    case RL2_SAMPLE_UINT32:
		destination->bitsPerSample = 32;
		TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE, 32);
		break;
	    case RL2_SAMPLE_FLOAT:
		destination->bitsPerSample = 32;
		TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE, 32);
		break;
		break;
	    case RL2_SAMPLE_DOUBLE:
		destination->bitsPerSample = 64;
		TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE, 64);
		break;
	    }
	  destination->photometric = PHOTOMETRIC_MINISBLACK;
	  TIFFSetField (destination->out, TIFFTAG_PHOTOMETRIC,
			PHOTOMETRIC_MINISBLACK);
	  if (tiff_compression == RL2_COMPRESSION_LZW)
	    {
		destination->compression = COMPRESSION_LZW;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZW);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_DEFLATE)
	    {
		destination->compression = COMPRESSION_DEFLATE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_DEFLATE);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_LZMA)
	    {
		destination->compression = COMPRESSION_LZMA;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZMA);
	    }
	  else
	    {
		destination->compression = COMPRESSION_NONE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_NONE);
	    }
      }
    else if (pixel_type == RL2_PIXEL_MULTIBAND)
      {
	  /* MULTIBAND */
	  destination->sampleFormat = SAMPLEFORMAT_UINT;
	  if (sample_type == RL2_SAMPLE_UINT8)
	      destination->bitsPerSample = 8;
	  else if (sample_type == RL2_SAMPLE_UINT16)
	      destination->bitsPerSample = 16;
	  else
	      goto error;
	  destination->samplesPerPixel = num_bands;
	  if (num_bands == 2)
	      destination->photometric = PHOTOMETRIC_MINISBLACK;
	  else
	      destination->photometric = PHOTOMETRIC_RGB;
	  TIFFSetField (destination->out, TIFFTAG_SAMPLEFORMAT,
			SAMPLEFORMAT_UINT);
	  TIFFSetField (destination->out, TIFFTAG_SAMPLESPERPIXEL,
			destination->samplesPerPixel);
	  if (num_bands == 2)
	    {
		uint16 extra[1];
		extra[0] = EXTRASAMPLE_UNSPECIFIED;
		TIFFSetField (destination->out, TIFFTAG_EXTRASAMPLES, 1,
			      &extra);
	    }
	  if (num_bands > 3)
	    {
		int n_extra = num_bands - 3;
		int i_extra;
		uint16 extra[256];
		for (i_extra = 0; i_extra < n_extra; i_extra++)
		    extra[i_extra] = EXTRASAMPLE_UNSPECIFIED;
		TIFFSetField (destination->out, TIFFTAG_EXTRASAMPLES, n_extra,
			      &extra);
	    }
	  TIFFSetField (destination->out, TIFFTAG_BITSPERSAMPLE,
			destination->bitsPerSample);
	  TIFFSetField (destination->out, TIFFTAG_PHOTOMETRIC,
			destination->photometric);
	  if (tiff_compression == RL2_COMPRESSION_LZW)
	    {
		destination->compression = COMPRESSION_LZW;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZW);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_DEFLATE)
	    {
		destination->compression = COMPRESSION_DEFLATE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_DEFLATE);
	    }
	  else if (tiff_compression == RL2_COMPRESSION_LZMA)
	    {
		destination->compression = COMPRESSION_LZMA;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_LZMA);
	    }
	  else
	    {
		destination->compression = COMPRESSION_NONE;
		TIFFSetField (destination->out, TIFFTAG_COMPRESSION,
			      COMPRESSION_NONE);
	    }
	  goto header_done;
      }
    else
	goto error;

  header_done:
    TIFFSetField (destination->out, TIFFTAG_SOFTWARE, "RasterLite-2");
    if (destination->isTiled)
      {
	  TIFFSetField (destination->out, TIFFTAG_TILEWIDTH,
			destination->tileWidth);
	  TIFFSetField (destination->out, TIFFTAG_TILELENGTH,
			destination->tileHeight);
      }
    else
      {
	  if (tiff_compression == RL2_COMPRESSION_JPEG)
	      TIFFSetField (destination->out, TIFFTAG_ROWSPERSTRIP, 8);
	  else
	      TIFFSetField (destination->out, TIFFTAG_ROWSPERSTRIP, 1);
      }

/* allocating the TIFF write buffer */
    if (destination->isTiled)
	buf_size = TIFFTileSize (destination->out);
    else
	buf_size = TIFFScanlineSize (destination->out);
    tiff_buffer = malloc (buf_size);
    if (!tiff_buffer)
	goto error;
    destination->tiffBuffer = tiff_buffer;

/* NULL georeferencing */
    destination->Srid = -1;
    destination->hResolution = DBL_MAX;
    destination->vResolution = DBL_MAX;
    destination->srsName = NULL;
    destination->proj4text = NULL;
    destination->minX = DBL_MAX;
    destination->minY = DBL_MAX;
    destination->maxX = DBL_MAX;
    destination->maxY = DBL_MAX;
    destination->tfw_path = NULL;

    return 1;
  error:
    return 0;
}

RL2_DECLARE rl2TiffDestinationPtr
rl2_create_tiff_destination (const char *path, unsigned int width,
			     unsigned int height, unsigned char sample_type,
			     unsigned char pixel_type,
			     unsigned char num_bands, rl2PalettePtr plt,
			     unsigned char tiff_compression, int tiled,
			     unsigned int tile_size)
{
/* attempting to create a file-based TIFF destination (no georeferencing) */
    rl2PrivTiffDestinationPtr destination = NULL;
    if (!check_color_model
	(sample_type, pixel_type, num_bands, plt, tiff_compression))
      {
	  fprintf (stderr, "RL2-TIFF writer: unsupported pixel format\n");
	  return NULL;
      }

    destination = create_tiff_destination (path, 0);
    if (destination == NULL)
	return NULL;

    destination->width = width;
    destination->height = height;
    if (tiled)
      {
	  destination->isTiled = 1;
	  destination->tileWidth = tile_size;
	  destination->tileHeight = tile_size;
      }
    else
      {
	  destination->isTiled = 0;
	  destination->rowsPerStrip = 1;
      }

/* suppressing TIFF messages */
    TIFFSetErrorHandler (NULL);
    TIFFSetWarningHandler (NULL);

/* creating a TIFF file */
    destination->out = TIFFOpen (destination->path, "w");
    if (destination->out == NULL)
	goto error;

    if (!set_tiff_destination
	(destination, width, height, sample_type, pixel_type, num_bands, plt,
	 tiff_compression))
	goto error;

    return (rl2TiffDestinationPtr) destination;
  error:
    if (destination != NULL)
	rl2_destroy_tiff_destination ((rl2TiffDestinationPtr) destination);
    return NULL;
}

static int
is_projected_srs (const char *proj4text)
{
/* checks if this one is a PCS SRS */
    if (proj4text == NULL)
	return 0;
    if (strstr (proj4text, "+proj=longlat ") != NULL)
	return 0;
    return 1;
}

static void
fetch_crs_params (sqlite3 * handle, int srid, char **srs_name, char **proj4text)
{
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    char *sql = sqlite3_mprintf ("SELECT ref_sys_name, proj4text "
				 "FROM spatial_ref_sys WHERE srid = %d\n",
				 srid);
    *srs_name = NULL;
    *proj4text = NULL;
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    for (i = 1; i <= rows; i++)
      {
	  int len;
	  const char *name = results[(i * columns) + 0];
	  const char *proj4 = results[(i * columns) + 1];
	  if (name != NULL)
	    {
		len = strlen (name);
		*srs_name = malloc (len + 1);
		strcpy (*srs_name, name);
	    }
	  if (proj4 != NULL)
	    {
		len = strlen (proj4);
		*proj4text = malloc (len + 1);
		strcpy (*proj4text, proj4);
	    }
      }
    sqlite3_free_table (results);
}

static void
destination_set_tfw_path (const char *path,
			  rl2PrivTiffDestinationPtr destination)
{
/* building the TFW path (WorldFile) */
    char *tfw;
    const char *x = NULL;
    const char *p = path;
    int len = strlen (path);
    len -= 1;
    while (*p != '\0')
      {
	  if (*p == '.')
	      x = p;
	  p++;
      }
    if (x > path)
	len = x - path;
    tfw = malloc (len + 5);
    memcpy (tfw, path, len);
    memcpy (tfw + len, ".tfw", 4);
    *(tfw + len + 4) = '\0';
    destination->tfw_path = tfw;
}

RL2_DECLARE rl2TiffDestinationPtr
rl2_create_geotiff_destination (const char *path, sqlite3 * handle,
				unsigned int width, unsigned int height,
				unsigned char sample_type,
				unsigned char pixel_type,
				unsigned char num_bands, rl2PalettePtr plt,
				unsigned char tiff_compression, int tiled,
				unsigned int tile_size, int srid, double minX,
				double minY, double maxX, double maxY,
				double hResolution, double vResolution,
				int with_worldfile)
{
/* attempting to create a file-based GeoTIFF destination */
    rl2PrivTiffDestinationPtr destination = NULL;
    double tiepoint[6];
    double pixsize[3];
    char *srs_name = NULL;
    char *proj4text = NULL;
    if (!check_color_model
	(sample_type, pixel_type, num_bands, plt, tiff_compression))
      {
	  fprintf (stderr, "RL2-GeoTIFF writer: unsupported pixel format\n");
	  return NULL;
      }
    if (handle == NULL)
	return NULL;

    destination = create_tiff_destination (path, 1);
    if (destination == NULL)
	return NULL;

    destination->width = width;
    destination->height = height;
    if (tiled)
      {
	  destination->isTiled = 1;
	  destination->tileWidth = tile_size;
	  destination->tileHeight = tile_size;
      }
    else
      {
	  destination->isTiled = 0;
	  destination->rowsPerStrip = 1;
      }

/* suppressing TIFF messages */
    TIFFSetErrorHandler (NULL);
    TIFFSetWarningHandler (NULL);

/* creating a GeoTIFF file */
    destination->out = XTIFFOpen (destination->path, "w");
    if (destination->out == NULL)
	goto error;
    destination->gtif = GTIFNew (destination->out);
    if (destination->gtif == NULL)
	goto error;

    if (!set_tiff_destination
	(destination, width, height, sample_type, pixel_type, num_bands, plt,
	 tiff_compression))
	goto error;

/* attempting to retrieve the CRS params */
    fetch_crs_params (handle, srid, &srs_name, &proj4text);
    if (srs_name == NULL || proj4text == NULL)
	goto error;

/* setting georeferencing infos */
    destination->Srid = srid;
    destination->hResolution = hResolution;
    destination->vResolution = vResolution;
    destination->srsName = srs_name;
    destination->proj4text = proj4text;
    destination->minX = minX;
    destination->minY = minY;
    destination->maxX = maxX;
    destination->maxY = maxY;
    destination->tfw_path = NULL;
    if (with_worldfile)
	destination_set_tfw_path (path, destination);

/* setting up the GeoTIFF Tags */
    pixsize[0] = hResolution;
    pixsize[1] = vResolution;
    pixsize[2] = 0.0;
    TIFFSetField (destination->out, GTIFF_PIXELSCALE, 3, pixsize);
    tiepoint[0] = 0.0;
    tiepoint[1] = 0.0;
    tiepoint[2] = 0.0;
    tiepoint[3] = minX;
    tiepoint[4] = maxY;
    tiepoint[5] = 0.0;
    TIFFSetField (destination->out, GTIFF_TIEPOINTS, 6, tiepoint);
    if (srs_name != NULL)
	TIFFSetField (destination->out, GTIFF_ASCIIPARAMS, srs_name);
    if (proj4text != NULL)
	GTIFSetFromProj4 (destination->gtif, proj4text);
    if (srs_name != NULL)
	GTIFKeySet (destination->gtif, GTCitationGeoKey, TYPE_ASCII, 0,
		    srs_name);
    if (is_projected_srs (proj4text))
	GTIFKeySet (destination->gtif, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
		    srid);
    GTIFWriteKeys (destination->gtif);
    destination->isGeoReferenced = 1;

    return (rl2TiffDestinationPtr) destination;
  error:
    if (destination != NULL)
	rl2_destroy_tiff_destination ((rl2TiffDestinationPtr) destination);
    if (srs_name != NULL)
	free (srs_name);
    if (proj4text != NULL)
	free (proj4text);
    return NULL;
}

RL2_DECLARE rl2TiffDestinationPtr
rl2_create_tiff_worldfile_destination (const char *path, unsigned int width,
				       unsigned int height,
				       unsigned char sample_type,
				       unsigned char pixel_type,
				       unsigned char num_bands,
				       rl2PalettePtr plt,
				       unsigned char tiff_compression,
				       int tiled, unsigned int tile_size,
				       int srid, double minX, double minY,
				       double maxX, double maxY,
				       double hResolution, double vResolution)
{
/* attempting to create a file-based TIFF destination (with worldfile) */
    rl2TiffDestinationPtr destination =
	rl2_create_tiff_destination (path, width, height, sample_type,
				     pixel_type, num_bands,
				     plt, tiff_compression, tiled, tile_size);
    rl2PrivTiffDestinationPtr tiff = (rl2PrivTiffDestinationPtr) destination;
    if (tiff == NULL)
	return NULL;
/* setting georeferencing infos */
    tiff->Srid = srid;
    tiff->hResolution = hResolution;
    tiff->vResolution = vResolution;
    tiff->srsName = NULL;
    tiff->proj4text = NULL;
    tiff->minX = minX;
    tiff->minY = minY;
    tiff->maxX = maxX;
    tiff->maxY = maxY;
    tiff->tfw_path = NULL;
    destination_set_tfw_path (path, tiff);
    tiff->isGeoReferenced = 1;
    return destination;
}

RL2_DECLARE const char *
rl2_get_tiff_destination_path (rl2TiffDestinationPtr tiff)
{
/* retrieving the output path from a TIFF destination */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return NULL;

    return destination->path;
}

RL2_DECLARE const char *
rl2_get_tiff_destination_worldfile_path (rl2TiffDestinationPtr tiff)
{
/* retrieving the Worldfile path from a TIFF destination */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return NULL;

    return destination->tfw_path;
}

RL2_DECLARE int
rl2_is_geotiff_destination (rl2TiffDestinationPtr tiff, int *geotiff)
{
/* detecting if a TIFF destination actually is a GeoTIFF */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    *geotiff = destination->isGeoTiff;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_tiff_worldfile_destination (rl2TiffDestinationPtr tiff,
				   int *tiff_worldfile)
{
/* detecting if a TIFF destination actually supports a TFW */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    *tiff_worldfile = 0;
    if (destination->tfw_path != NULL)
	*tiff_worldfile = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_destination_size (rl2TiffDestinationPtr tiff,
			       unsigned int *width, unsigned int *height)
{
/* retrieving Width and Height from a TIFF destination */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;

    *width = destination->width;
    *height = destination->height;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_destination_srid (rl2TiffDestinationPtr tiff, int *srid)
{
/* retrieving the SRID from a TIFF destination */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    if (destination->isGeoReferenced == 0)
	return RL2_ERROR;

    *srid = destination->Srid;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_destination_extent (rl2TiffDestinationPtr tiff, double *minX,
				 double *minY, double *maxX, double *maxY)
{
/* retrieving the Extent from a TIFF destination */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    if (destination->isGeoReferenced == 0)
	return RL2_ERROR;

    *minX = destination->minX;
    *minY = destination->minY;
    *maxX = destination->maxX;
    *maxY = destination->maxY;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_destination_resolution (rl2TiffDestinationPtr tiff,
				     double *hResolution, double *vResolution)
{
/* retrieving the Pixel Resolution from a TIFF destination */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    if (destination->isGeoReferenced == 0)
	return RL2_ERROR;

    *hResolution = destination->hResolution;
    *vResolution = destination->vResolution;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_destination_type (rl2TiffDestinationPtr tiff,
			       unsigned char *sample_type,
			       unsigned char *pixel_type,
			       unsigned char *alias_pixel_type,
			       unsigned char *num_bands)
{
/* retrieving the sample/pixel type from a TIFF destination */
    int ok = 0;
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;

    if (destination->sampleFormat == SAMPLEFORMAT_UINT
	&& destination->samplesPerPixel == 1
	&& destination->photometric < PHOTOMETRIC_RGB)
      {
	  /* could be some kind of MONOCHROME */
	  if (destination->bitsPerSample == 1)
	    {
		*sample_type = RL2_SAMPLE_1_BIT;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_MONOCHROME;
		*alias_pixel_type = RL2_PIXEL_MONOCHROME;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (destination->sampleFormat == SAMPLEFORMAT_UINT
	&& destination->samplesPerPixel == 1
	&& destination->photometric < PHOTOMETRIC_RGB)
      {
	  /* could be some kind of GRAYSCALE */
	  if (destination->bitsPerSample == 2)
	    {
		*sample_type = RL2_SAMPLE_2_BIT;
		ok = 1;
	    }
	  else if (destination->bitsPerSample == 4)
	    {
		*sample_type = RL2_SAMPLE_4_BIT;
		ok = 1;
	    }
	  else if (destination->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  else if (destination->bitsPerSample == 16)
	    {
		*sample_type = RL2_SAMPLE_UINT16;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_GRAYSCALE;
		*alias_pixel_type = RL2_PIXEL_GRAYSCALE;
		if (destination->bitsPerSample == 8
		    || destination->bitsPerSample == 16)
		    *alias_pixel_type = RL2_PIXEL_DATAGRID;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (destination->sampleFormat == SAMPLEFORMAT_UINT
	&& destination->samplesPerPixel == 1
	&& destination->photometric == PHOTOMETRIC_PALETTE)
      {
	  /* could be some kind of PALETTE */
	  if (destination->bitsPerSample == 1)
	    {
		*sample_type = RL2_SAMPLE_1_BIT;
		ok = 1;
	    }
	  else if (destination->bitsPerSample == 2)
	    {
		*sample_type = RL2_SAMPLE_2_BIT;
		ok = 1;
	    }
	  else if (destination->bitsPerSample == 4)
	    {
		*sample_type = RL2_SAMPLE_4_BIT;
		ok = 1;
	    }
	  else if (destination->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_PALETTE;
		*alias_pixel_type = RL2_PIXEL_PALETTE;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    if (destination->sampleFormat == SAMPLEFORMAT_UINT
	&& destination->samplesPerPixel == 3
	&& destination->photometric == PHOTOMETRIC_RGB)
      {
	  /* could be some kind of RGB */
	  if (destination->bitsPerSample == 8)
	    {
		*sample_type = RL2_SAMPLE_UINT8;
		ok = 1;
	    }
	  else if (destination->bitsPerSample == 16)
	    {
		*sample_type = RL2_SAMPLE_UINT16;
		ok = 1;
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_RGB;
		*alias_pixel_type = RL2_PIXEL_RGB;
		*num_bands = 3;
		return RL2_OK;
	    }
      }
    if (destination->samplesPerPixel == 1
	&& destination->photometric < PHOTOMETRIC_RGB)
      {
	  /* could be some kind of DATA-GRID */
	  if (destination->sampleFormat == SAMPLEFORMAT_INT)
	    {
		/* Signed Integer */
		if (destination->bitsPerSample == 8)
		  {
		      *sample_type = RL2_SAMPLE_INT8;
		      ok = 1;
		  }
		else if (destination->bitsPerSample == 16)
		  {
		      *sample_type = RL2_SAMPLE_INT16;
		      ok = 1;
		  }
		else if (destination->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_INT32;
		      ok = 1;
		  }
	    }
	  if (destination->sampleFormat == SAMPLEFORMAT_UINT)
	    {
		/* Unsigned Integer */
		if (destination->bitsPerSample == 8)
		  {
		      *sample_type = RL2_SAMPLE_UINT8;
		      ok = 1;
		  }
		else if (destination->bitsPerSample == 16)
		  {
		      *sample_type = RL2_SAMPLE_UINT16;
		      ok = 1;
		  }
		else if (destination->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_UINT32;
		      ok = 1;
		  }
	    }
	  if (destination->sampleFormat == SAMPLEFORMAT_IEEEFP)
	    {
		/* Floating-Point */
		if (destination->bitsPerSample == 32)
		  {
		      *sample_type = RL2_SAMPLE_FLOAT;
		      ok = 1;
		  }
		else if (destination->bitsPerSample == 64)
		  {
		      *sample_type = RL2_SAMPLE_DOUBLE;
		      ok = 1;
		  }
	    }
	  if (ok)
	    {
		*pixel_type = RL2_PIXEL_DATAGRID;
		*alias_pixel_type = RL2_PIXEL_DATAGRID;
		*num_bands = 1;
		return RL2_OK;
	    }
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_tiff_destination_compression (rl2TiffDestinationPtr tiff,
				      unsigned char *compression)
{
/* retrieving the sample/pixel type from a TIFF destination */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;

    switch (destination->compression)
      {
      case COMPRESSION_NONE:
	  *compression = RL2_COMPRESSION_NONE;
	  break;
      case COMPRESSION_LZW:
	  *compression = RL2_COMPRESSION_LZW;
	  break;
      case COMPRESSION_DEFLATE:
	  *compression = RL2_COMPRESSION_DEFLATE;
	  break;
      case COMPRESSION_LZMA:
	  *compression = RL2_COMPRESSION_LZMA;
	  break;
      case COMPRESSION_JPEG:
	  *compression = RL2_COMPRESSION_JPEG;
	  break;
      case COMPRESSION_CCITTFAX3:
	  *compression = RL2_COMPRESSION_CCITTFAX3;
	  break;
      case COMPRESSION_CCITTFAX4:
	  *compression = RL2_COMPRESSION_CCITTFAX4;
	  break;
      default:
	  *compression = RL2_COMPRESSION_UNKNOWN;
	  break;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_tiled_tiff_destination (rl2TiffDestinationPtr tiff, int *is_tiled)
{
/* testing if the TIFF Destination is tiled or not */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    *is_tiled = destination->isTiled;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_destination_tile_size (rl2TiffDestinationPtr tiff,
				    unsigned int *tile_width,
				    unsigned int *tile_height)
{
/* attempting to return the Tile dimensions */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    if (destination->isTiled == 0)
	return RL2_ERROR;
    *tile_width = destination->tileWidth;
    *tile_height = destination->tileHeight;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_tiff_destination_strip_size (rl2TiffDestinationPtr tiff,
				     unsigned int *strip_size)
{
/* attempting to return the Strip dimension */
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    if (destination->isTiled != 0)
	return RL2_ERROR;
    *strip_size = destination->rowsPerStrip;
    return RL2_OK;
}

static int
tiff_write_strip_rgb (rl2PrivTiffDestinationPtr tiff, rl2PrivRasterPtr raster,
		      unsigned int row)
{
/* writing a TIFF RGB scanline */
    unsigned int x;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    for (x = 0; x < raster->width; x++)
      {
	  *p_out++ = *p_in++;
	  *p_out++ = *p_in++;
	  *p_out++ = *p_in++;
	  if (raster->nBands == 4)
	      p_in++;
      }
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_gray (rl2PrivTiffDestinationPtr tiff,
		       rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grayscale scanline */
    unsigned int x;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_palette (rl2PrivTiffDestinationPtr tiff,
			  rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Palette scanline */
    unsigned int x;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_monochrome (rl2PrivTiffDestinationPtr tiff,
			     rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Monochrome */
    unsigned int x;
    unsigned char byte;
    unsigned char pixel;
    int pos;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

/* priming a White scanline */
    for (x = 0; x < TIFFScanlineSize (tiff->out); x++)
	*p_out++ = 0x00;

/* inserting pixels */
    pos = 0;
    byte = 0x00;
    p_out = tiff->tiffBuffer;
    for (x = 0; x < raster->width; x++)
      {
	  pixel = *p_in++;
	  if (pixel == 1)
	    {
		/* handling a black pixel */
		switch (pos)
		  {
		  case 0:
		      byte |= 0x80;
		      break;
		  case 1:
		      byte |= 0x40;
		      break;
		  case 2:
		      byte |= 0x20;
		      break;
		  case 3:
		      byte |= 0x10;
		      break;
		  case 4:
		      byte |= 0x08;
		      break;
		  case 5:
		      byte |= 0x04;
		      break;
		  case 6:
		      byte |= 0x02;
		      break;
		  case 7:
		      byte |= 0x01;
		      break;
		  };
	    }
	  pos++;
	  if (pos > 7)
	    {
		/* exporting an octet */
		*p_out++ = byte;
		byte = 0x00;
		pos = 0;
	    }
      }
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_int8 (rl2PrivTiffDestinationPtr tiff,
		       rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grid Int8 scanline */
    unsigned int x;
    char *p_in = (char *) (raster->rasterBuffer);
    char *p_out = (char *) (tiff->tiffBuffer);

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_uint8 (rl2PrivTiffDestinationPtr tiff,
			rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grid UInt8 scanline */
    unsigned int x;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_int16 (rl2PrivTiffDestinationPtr tiff,
			rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grid Int16 scanline */
    unsigned int x;
    short *p_in = (short *) (raster->rasterBuffer);
    short *p_out = (short *) (tiff->tiffBuffer);

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_uint16 (rl2PrivTiffDestinationPtr tiff,
			 rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grid UInt16 scanline */
    unsigned int x;
    unsigned short *p_in = (unsigned short *) (raster->rasterBuffer);
    unsigned short *p_out = (unsigned short *) (tiff->tiffBuffer);

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_int32 (rl2PrivTiffDestinationPtr tiff,
			rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grid Int32 scanline */
    unsigned int x;
    int *p_in = (int *) (raster->rasterBuffer);
    int *p_out = (int *) (tiff->tiffBuffer);

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_uint32 (rl2PrivTiffDestinationPtr tiff,
			 rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grid UInt32 scanline */
    unsigned int x;
    unsigned int *p_in = (unsigned int *) (raster->rasterBuffer);
    unsigned int *p_out = (unsigned int *) (tiff->tiffBuffer);

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_float (rl2PrivTiffDestinationPtr tiff,
			rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grid Float scanline */
    unsigned int x;
    float *p_in = (float *) (raster->rasterBuffer);
    float *p_out = (float *) (tiff->tiffBuffer);

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_strip_double (rl2PrivTiffDestinationPtr tiff,
			 rl2PrivRasterPtr raster, unsigned int row)
{
/* writing a TIFF Grid Double scanline */
    unsigned int x;
    double *p_in = (double *) (raster->rasterBuffer);
    double *p_out = (double *) (tiff->tiffBuffer);

    for (x = 0; x < raster->width; x++)
	*p_out++ = *p_in++;
    if (TIFFWriteScanline (tiff->out, tiff->tiffBuffer, row, 0) < 0)
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_write_tiff_scanline (rl2TiffDestinationPtr tiff, rl2RasterPtr raster,
			 unsigned int row)
{
/* writing a TIFF scanline */
    int ret = 0;
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;
    if (tiff == NULL)
	return RL2_ERROR;
    if (rst == NULL)
	return RL2_ERROR;

    if (destination->sampleFormat == SAMPLEFORMAT_UINT
	&& destination->samplesPerPixel == 3 && destination->photometric == 2
	&& destination->bitsPerSample == 8
	&& rst->sampleType == RL2_SAMPLE_UINT8
	&& rst->pixelType == RL2_PIXEL_RGB && (rst->nBands == 3
					       || rst->nBands == 4)
	&& destination->width == rst->width)
	ret = tiff_write_strip_rgb (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 8
	     && rst->sampleType == RL2_SAMPLE_UINT8
	     && rst->pixelType == RL2_PIXEL_GRAYSCALE && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_gray (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric == 3
	     && destination->bitsPerSample == 8
	     && rst->sampleType == RL2_SAMPLE_UINT8
	     && rst->pixelType == RL2_PIXEL_PALETTE && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_palette (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 1
	     && rst->sampleType == RL2_SAMPLE_1_BIT
	     && rst->pixelType == RL2_PIXEL_MONOCHROME && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_monochrome (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_INT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 8
	     && rst->sampleType == RL2_SAMPLE_INT8
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_int8 (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 8
	     && rst->sampleType == RL2_SAMPLE_UINT8
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_uint8 (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_INT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 16
	     && rst->sampleType == RL2_SAMPLE_INT16
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_int16 (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 16
	     && rst->sampleType == RL2_SAMPLE_UINT16
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_uint16 (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_INT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 32
	     && rst->sampleType == RL2_SAMPLE_INT32
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_int32 (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 32
	     && rst->sampleType == RL2_SAMPLE_UINT32
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_uint32 (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_IEEEFP
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 32
	     && rst->sampleType == RL2_SAMPLE_FLOAT
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_float (destination, rst, row);
    else if (destination->sampleFormat == SAMPLEFORMAT_IEEEFP
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 64
	     && rst->sampleType == RL2_SAMPLE_DOUBLE
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->width == rst->width)
	ret = tiff_write_strip_double (destination, rst, row);

    if (ret)
	return RL2_OK;
    return RL2_ERROR;
}

static int
tiff_write_tile_multiband8 (rl2PrivTiffDestinationPtr tiff,
			    rl2PrivRasterPtr raster, unsigned int row,
			    unsigned int col)
{
/* writing a TIFF MULTIBAND UINT8 tile - separate planes */
    unsigned int y;
    unsigned int x;
    int band;

    for (band = 0; band < raster->nBands; band++)
      {
	  /* handling separate planes - one for each band */
	  unsigned char *p_in = raster->rasterBuffer;
	  unsigned char *p_out = tiff->tiffBuffer;
	  for (y = 0; y < raster->height; y++)
	    {
		for (x = 0; x < raster->width; x++)
		  {
		      *p_out++ = *(p_in + band);
		      p_in += raster->nBands;
		  }
	    }
	  if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, band) <
	      0)
	      return 0;
      }
    return 1;
}

static int
tiff_write_tile_multiband16 (rl2PrivTiffDestinationPtr tiff,
			     rl2PrivRasterPtr raster, unsigned int row,
			     unsigned int col)
{
/* writing a TIFF MULTIBAND UINT16 tile - separate planes */
    unsigned int y;
    unsigned int x;
    int band;

    for (band = 0; band < raster->nBands; band++)
      {
	  /* handling separate planes - one for each band */
	  unsigned short *p_in = (unsigned short *) (raster->rasterBuffer);
	  unsigned short *p_out = (unsigned short *) (tiff->tiffBuffer);
	  for (y = 0; y < raster->height; y++)
	    {
		for (x = 0; x < raster->width; x++)
		  {
		      *p_out++ = *(p_in + band);
		      p_in += raster->nBands;
		  }
	    }
	  if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, band) <
	      0)
	      return 0;
      }
    return 1;
}

static int
tiff_write_tile_rgb_u8 (rl2PrivTiffDestinationPtr tiff,
			rl2PrivRasterPtr raster, unsigned int row,
			unsigned int col)
{
/* writing a TIFF RGB tile - UINT8 */
    unsigned int y;
    unsigned int x;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	    {
		*p_out++ = *p_in++;
		*p_out++ = *p_in++;
		*p_out++ = *p_in++;
		if (raster->nBands == 4)
		    p_in++;
	    }
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_gray (rl2PrivTiffDestinationPtr tiff, rl2PrivRasterPtr raster,
		      unsigned int row, unsigned int col)
{
/* writing a TIFF Grayscale tile */
    unsigned int y;
    unsigned int x;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_msk = raster->maskBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	    {
		if (p_msk == NULL)
		  {
		      /* there is no Transparency Mask - all opaque pixels */
		      *p_out++ = *p_in++;
		  }
		else
		  {
		      if (*p_msk++ == 0)
			{
			    /* transparent pixel */
			    p_out++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
	    }
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_palette (rl2PrivTiffDestinationPtr tiff,
			 rl2PrivRasterPtr raster, unsigned int row,
			 unsigned int col)
{
/* writing a TIFF Palette tile */
    unsigned int y;
    unsigned int x;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_monochrome (rl2PrivTiffDestinationPtr tiff,
			    rl2PrivRasterPtr raster, unsigned int row,
			    unsigned int col)
{
/* writing a TIFF Monochrome tile */
    unsigned int y;
    unsigned int x;
    unsigned char byte;
    unsigned char pixel;
    int pos;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    /* priming a White tile */
    for (x = 0; x < TIFFTileSize (tiff->out); x++)
	*p_out++ = 0x00;
    p_out = tiff->tiffBuffer;
    for (y = 0; y < raster->height; y++)
      {
	  /* inserting pixels */
	  pos = 0;
	  byte = 0x00;
	  for (x = 0; x < raster->width; x++)
	    {
		pixel = *p_in++;
		if (pixel == 1)
		  {
		      /* handling a black pixel */
		      switch (pos)
			{
			case 0:
			    byte |= 0x80;
			    break;
			case 1:
			    byte |= 0x40;
			    break;
			case 2:
			    byte |= 0x20;
			    break;
			case 3:
			    byte |= 0x10;
			    break;
			case 4:
			    byte |= 0x08;
			    break;
			case 5:
			    byte |= 0x04;
			    break;
			case 6:
			    byte |= 0x02;
			    break;
			case 7:
			    byte |= 0x01;
			    break;
			};
		  }
		pos++;
		if (pos > 7)
		  {
		      /* exporting an octet */
		      *p_out++ = byte;
		      byte = 0x00;
		      pos = 0;
		  }
	    }
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_int8 (rl2PrivTiffDestinationPtr tiff,
		      rl2PrivRasterPtr raster, unsigned int row,
		      unsigned int col)
{
/* writing a TIFF Grid Int8 tile */
    unsigned int y;
    unsigned int x;
    char *p_in = (char *) (raster->rasterBuffer);
    char *p_out = (char *) (tiff->tiffBuffer);

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_uint8 (rl2PrivTiffDestinationPtr tiff,
		       rl2PrivRasterPtr raster, unsigned int row,
		       unsigned int col)
{
/* writing a TIFF Grid UInt8 tile */
    unsigned int y;
    unsigned int x;
    unsigned char *p_in = raster->rasterBuffer;
    unsigned char *p_out = tiff->tiffBuffer;

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_int16 (rl2PrivTiffDestinationPtr tiff,
		       rl2PrivRasterPtr raster, unsigned int row,
		       unsigned int col)
{
/* writing a TIFF Grid Int16 tile */
    unsigned int y;
    unsigned int x;
    short *p_in = (short *) (raster->rasterBuffer);
    short *p_out = (short *) (tiff->tiffBuffer);

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_uint16 (rl2PrivTiffDestinationPtr tiff,
			rl2PrivRasterPtr raster, unsigned int row,
			unsigned int col)
{
/* writing a TIFF Grid UInt16 tile */
    unsigned int y;
    unsigned int x;
    unsigned short *p_in = (unsigned short *) (raster->rasterBuffer);
    unsigned short *p_out = (unsigned short *) (tiff->tiffBuffer);

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_int32 (rl2PrivTiffDestinationPtr tiff,
		       rl2PrivRasterPtr raster, unsigned int row,
		       unsigned int col)
{
/* writing a TIFF Grid Int32 tile */
    unsigned int y;
    unsigned int x;
    int *p_in = (int *) (raster->rasterBuffer);
    int *p_out = (int *) (tiff->tiffBuffer);

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_uint32 (rl2PrivTiffDestinationPtr tiff,
			rl2PrivRasterPtr raster, unsigned int row,
			unsigned int col)
{
/* writing a TIFF Grid UInt32 tile */
    unsigned int y;
    unsigned int x;
    unsigned int *p_in = (unsigned int *) (raster->rasterBuffer);
    unsigned int *p_out = (unsigned int *) (tiff->tiffBuffer);

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_float (rl2PrivTiffDestinationPtr tiff,
		       rl2PrivRasterPtr raster, unsigned int row,
		       unsigned int col)
{
/* writing a TIFF Grid Float tile */
    unsigned int y;
    unsigned int x;
    float *p_in = (float *) (raster->rasterBuffer);
    float *p_out = (float *) (tiff->tiffBuffer);

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

static int
tiff_write_tile_double (rl2PrivTiffDestinationPtr tiff,
			rl2PrivRasterPtr raster, unsigned int row,
			unsigned int col)
{
/* writing a TIFF Grid Double tile */
    unsigned int y;
    unsigned int x;
    double *p_in = (double *) (raster->rasterBuffer);
    double *p_out = (double *) (tiff->tiffBuffer);

    for (y = 0; y < raster->height; y++)
      {
	  for (x = 0; x < raster->width; x++)
	      *p_out++ = *p_in++;
      }
    if (TIFFWriteTile (tiff->out, tiff->tiffBuffer, col, row, 0, 0) < 0)
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_write_tiff_tile (rl2TiffDestinationPtr tiff, rl2RasterPtr raster,
		     unsigned int startRow, unsigned int startCol)
{
/* writing a TIFF tile */
    int ret = 0;
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;
    if (tiff == NULL)
	return RL2_ERROR;
    if (rst == NULL)
	return RL2_ERROR;

    if (destination->sampleFormat == SAMPLEFORMAT_UINT
	&& destination->samplesPerPixel == 3 && destination->photometric == 2
	&& destination->bitsPerSample == 8
	&& rst->sampleType == RL2_SAMPLE_UINT8
	&& rst->pixelType == RL2_PIXEL_RGB && (rst->nBands == 3
					       || rst->nBands == 4)
	&& destination->tileWidth == rst->width
	&& destination->tileHeight == rst->height)
	ret = tiff_write_tile_rgb_u8 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 3
	     && destination->photometric == 2
	     && destination->bitsPerSample == 16
	     && rst->sampleType == RL2_SAMPLE_UINT16
	     && rst->pixelType == RL2_PIXEL_RGB && rst->nBands == 3
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret =
	    tiff_write_tile_multiband16 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel >= 2
	     && destination->bitsPerSample == 8
	     && rst->sampleType == RL2_SAMPLE_UINT8
	     && rst->pixelType == RL2_PIXEL_MULTIBAND
	     && rst->nBands == destination->samplesPerPixel
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_multiband8 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel >= 2
	     && destination->bitsPerSample == 16
	     && rst->sampleType == RL2_SAMPLE_UINT16
	     && rst->pixelType == RL2_PIXEL_MULTIBAND
	     && rst->nBands == destination->samplesPerPixel
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret =
	    tiff_write_tile_multiband16 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 8
	     && rst->sampleType == RL2_SAMPLE_UINT8
	     && rst->pixelType == RL2_PIXEL_GRAYSCALE && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_gray (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric == 3
	     && destination->bitsPerSample == 8
	     && (rst->sampleType == RL2_SAMPLE_UINT8
		 || rst->sampleType == RL2_SAMPLE_1_BIT
		 || rst->sampleType == RL2_SAMPLE_2_BIT
		 || rst->sampleType == RL2_SAMPLE_4_BIT)
	     && rst->pixelType == RL2_PIXEL_PALETTE && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_palette (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 1
	     && rst->sampleType == RL2_SAMPLE_1_BIT
	     && rst->pixelType == RL2_PIXEL_MONOCHROME && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_monochrome (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_INT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 8
	     && rst->sampleType == RL2_SAMPLE_INT8
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_int8 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 8
	     && rst->sampleType == RL2_SAMPLE_UINT8
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_uint8 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_INT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 16
	     && rst->sampleType == RL2_SAMPLE_INT16
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_int16 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 16
	     && rst->sampleType == RL2_SAMPLE_UINT16
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_uint16 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_INT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 32
	     && rst->sampleType == RL2_SAMPLE_INT32
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_int32 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_UINT
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 32
	     && rst->sampleType == RL2_SAMPLE_UINT32
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_uint32 (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_IEEEFP
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 32
	     && rst->sampleType == RL2_SAMPLE_FLOAT
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_float (destination, rst, startRow, startCol);
    else if (destination->sampleFormat == SAMPLEFORMAT_IEEEFP
	     && destination->samplesPerPixel == 1
	     && destination->photometric < 2
	     && destination->bitsPerSample == 64
	     && rst->sampleType == RL2_SAMPLE_DOUBLE
	     && rst->pixelType == RL2_PIXEL_DATAGRID && rst->nBands == 1
	     && destination->tileWidth == rst->width
	     && destination->tileHeight == rst->height)
	ret = tiff_write_tile_double (destination, rst, startRow, startCol);

    if (ret)
	return RL2_OK;
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_write_tiff_worldfile (rl2TiffDestinationPtr tiff)
{
/* writing a Worldfile supporting a TIFF destination */
    FILE *tfw;
    rl2PrivTiffDestinationPtr destination = (rl2PrivTiffDestinationPtr) tiff;
    if (destination == NULL)
	return RL2_ERROR;
    if (destination->tfw_path == NULL)
	return RL2_ERROR;

    tfw = fopen (destination->tfw_path, "w");
    if (tfw == NULL)
      {
	  fprintf (stderr,
		   "RL2-TIFF writer: unable to open Worldfile \"%s\"\n",
		   destination->tfw_path);
	  return RL2_ERROR;
      }
    fprintf (tfw, "        %1.16f\n", destination->hResolution);
    fprintf (tfw, "        0.0\n");
    fprintf (tfw, "        0.0\n");
    fprintf (tfw, "        -%1.16f\n", destination->vResolution);
    fprintf (tfw, "        %1.16f\n", destination->minX);
    fprintf (tfw, "        %1.16f\n", destination->maxY);
    fclose (tfw);
    return RL2_OK;
}

static int
compress_fax4 (const unsigned char *buffer,
	       unsigned short width,
	       unsigned short height, unsigned char **blob, int *blob_size)
{
/* compressing a TIFF FAX4 block - actual work */
    struct memfile clientdata;
    TIFF *out = (TIFF *) 0;
    tsize_t buf_size;
    void *tiff_buffer = NULL;
    int y;
    int x;
    unsigned char byte;
    unsigned char pixel;
    int pos;
    const unsigned char *p_in;
    unsigned char *p_out;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* writing into memory */
    clientdata.buffer = NULL;
    clientdata.malloc_block = 1024;
    clientdata.size = 0;
    clientdata.eof = 0;
    clientdata.current = 0;
    out = TIFFClientOpen ("tiff", "w", &clientdata, memory_readproc,
			  memory_writeproc, memory_seekproc, closeproc,
			  memory_sizeproc, mapproc, unmapproc);
    if (out == NULL)
	return 0;

/* setting up the TIFF headers */
    TIFFSetField (out, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField (out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField (out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField (out, TIFFTAG_XRESOLUTION, 300.0);
    TIFFSetField (out, TIFFTAG_YRESOLUTION, 300.0);
    TIFFSetField (out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField (out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField (out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, 1);
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
    TIFFSetField (out, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
    TIFFSetField (out, TIFFTAG_TILEWIDTH, width);
    TIFFSetField (out, TIFFTAG_TILELENGTH, height);

/* allocating the TIFF write buffer */
    buf_size = TIFFTileSize (out);
    tiff_buffer = malloc (buf_size);
    if (tiff_buffer == NULL)
	goto error;

/* writing a TIFF Monochrome tile */
    p_in = buffer;
    p_out = tiff_buffer;

    /* priming a White tile */
    for (x = 0; x < buf_size; x++)
	*p_out++ = 0x00;
    p_out = tiff_buffer;
    for (y = 0; y < height; y++)
      {
	  /* inserting pixels */
	  pos = 0;
	  byte = 0x00;
	  for (x = 0; x < width; x++)
	    {
		pixel = *p_in++;
		if (pixel == 1)
		  {
		      /* handling a black pixel */
		      switch (pos)
			{
			case 0:
			    byte |= 0x80;
			    break;
			case 1:
			    byte |= 0x40;
			    break;
			case 2:
			    byte |= 0x20;
			    break;
			case 3:
			    byte |= 0x10;
			    break;
			case 4:
			    byte |= 0x08;
			    break;
			case 5:
			    byte |= 0x04;
			    break;
			case 6:
			    byte |= 0x02;
			    break;
			case 7:
			    byte |= 0x01;
			    break;
			};
		  }
		pos++;
		if (pos > 7)
		  {
		      /* exporting an octet */
		      *p_out++ = byte;
		      byte = 0x00;
		      pos = 0;
		  }
	    }
      }
    if (TIFFWriteTile (out, tiff_buffer, 0, 0, 0, 0) < 0)
	goto error;

    TIFFClose (out);
    free (tiff_buffer);
    *blob = clientdata.buffer;
    *blob_size = clientdata.eof;
    return 1;

  error:
    TIFFClose (out);
    if (tiff_buffer != NULL)
	free (tiff_buffer);
    if (clientdata.buffer != NULL)
	free (clientdata.buffer);
    return 0;
}

RL2_DECLARE int
rl2_raster_to_tiff_mono4 (rl2RasterPtr rst, unsigned char **tiff,
			  int *tiff_size)
{
/* creating a TIFF FAX4 block from a raster */
    rl2PrivRasterPtr raster = (rl2PrivRasterPtr) rst;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_samples;
    if (rst == NULL)
	return RL2_ERROR;
    if (rl2_get_raster_type (rst, &sample_type, &pixel_type, &num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (sample_type == RL2_SAMPLE_1_BIT && pixel_type == RL2_PIXEL_MONOCHROME
	&& num_samples == 1)
	;
    else
	return RL2_ERROR;

    if (!compress_fax4 (raster->rasterBuffer, raster->width,
			raster->height, tiff, tiff_size))
	return RL2_ERROR;
    return RL2_OK;
}

RL2_PRIVATE int
rl2_decode_tiff_mono4 (const unsigned char *tiff, int tiff_sz,
		       unsigned int *xwidth, unsigned int *xheight,
		       unsigned char **pixels, int *pixels_sz)
{
/* attempting to decode a TIFF FAX4 block */
    struct memfile clientdata;
    TIFF *in = (TIFF *) 0;
    int is_tiled;
    uint32 width = 0;
    uint32 height = 0;
    uint32 tile_width;
    uint32 tile_height;
    uint16 bits_per_sample;
    uint16 samples_per_pixel;
    uint16 photometric;
    uint16 compression;
    uint16 sample_format;
    uint16 planar_config;
    tsize_t buf_size;
    void *tiff_buffer = NULL;
    unsigned int x;
    unsigned char pixel;
    unsigned char *buffer;
    int buf_sz;
    const unsigned char *p_in;
    unsigned char *p_out;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* reading from memory */
    clientdata.buffer = (unsigned char *) tiff;
    clientdata.malloc_block = 1024;
    clientdata.size = tiff_sz;
    clientdata.eof = tiff_sz;
    clientdata.current = 0;
    in = TIFFClientOpen ("tiff", "r", &clientdata, memory_readproc,
			 memory_writeproc, memory_seekproc, closeproc,
			 memory_sizeproc, mapproc, unmapproc);
    if (in == NULL)
	return RL2_ERROR;

/* retrieving the TIFF dimensions */
    is_tiled = TIFFIsTiled (in);
    if (!is_tiled)
	goto error;
    TIFFGetField (in, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField (in, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField (in, TIFFTAG_TILEWIDTH, &tile_width);
    TIFFGetField (in, TIFFTAG_TILELENGTH, &tile_height);
    if (tile_width != width)
	goto error;
    if (tile_height != height)
	goto error;
    TIFFGetField (in, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
    if (bits_per_sample != 1)
	goto error;
    TIFFGetField (in, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
    if (samples_per_pixel != 1)
	goto error;
    TIFFGetField (in, TIFFTAG_SAMPLEFORMAT, &sample_format);
    if (sample_format != SAMPLEFORMAT_UINT)
	goto error;
    TIFFGetField (in, TIFFTAG_PLANARCONFIG, &planar_config);
    if (planar_config != PLANARCONFIG_CONTIG)
	goto error;
    TIFFGetField (in, TIFFTAG_PHOTOMETRIC, &photometric);
    if (photometric != PHOTOMETRIC_MINISWHITE)
	goto error;
    TIFFGetField (in, TIFFTAG_COMPRESSION, &compression);
    if (compression != COMPRESSION_CCITTFAX4)
	goto error;

/* allocating the tile buffer */
    buf_size = TIFFTileSize (in);
    tiff_buffer = malloc (buf_size);
    if (tiff_buffer == NULL)
	goto error;

/* reading and decoding as a single tile */
    if (!TIFFReadTile (in, tiff_buffer, 0, 0, 0, 0))
	goto error;

/* allocating the output buffer */
    buf_sz = width * height;
    buffer = malloc (buf_sz);
    if (buffer == NULL)
	goto error;

    p_in = tiff_buffer;
    p_out = buffer;
    for (x = 0; x < buf_size; x++)
      {
	  pixel = *p_in++;
	  if ((pixel & 0x80) == 0x80)
	      *p_out++ = 1;
	  else
	      *p_out++ = 0;
	  if ((pixel & 0x40) == 0x40)
	      *p_out++ = 1;
	  else
	      *p_out++ = 0;
	  if ((pixel & 0x20) == 0x20)
	      *p_out++ = 1;
	  else
	      *p_out++ = 0;
	  if ((pixel & 0x10) == 0x10)
	      *p_out++ = 1;
	  else
	      *p_out++ = 0;
	  if ((pixel & 0x08) == 0x08)
	      *p_out++ = 1;
	  else
	      *p_out++ = 0;
	  if ((pixel & 0x04) == 0x04)
	      *p_out++ = 1;
	  else
	      *p_out++ = 0;
	  if ((pixel & 0x02) == 0x02)
	      *p_out++ = 1;
	  else
	      *p_out++ = 0;
	  if ((pixel & 0x01) == 0x01)
	      *p_out++ = 1;
	  else
	      *p_out++ = 0;
      }

    TIFFClose (in);
    if (tiff_buffer != NULL)
	free (tiff_buffer);
    *xwidth = width;
    *xheight = height;
    *pixels = buffer;
    *pixels_sz = buf_sz;
    return RL2_OK;

  error:
    TIFFClose (in);
    if (tiff_buffer != NULL)
	free (tiff_buffer);
    return RL2_ERROR;
}

static int
rgb_tiff_common (TIFF * out, const unsigned char *buffer,
		 unsigned short width, unsigned short height)
{
/* common implementation of RGB TIFF export */
    tsize_t buf_size;
    void *tiff_buffer = NULL;
    int y;
    int x;
    const unsigned char *p_in;
    unsigned char *p_out;

/* setting up the TIFF headers */
    TIFFSetField (out, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField (out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField (out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField (out, TIFFTAG_XRESOLUTION, 300.0);
    TIFFSetField (out, TIFFTAG_YRESOLUTION, 300.0);
    TIFFSetField (out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField (out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField (out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField (out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField (out, TIFFTAG_ROWSPERSTRIP, 1);

/* allocating the TIFF write buffer */
    buf_size = TIFFScanlineSize (out);
    tiff_buffer = malloc (buf_size);
    if (tiff_buffer == NULL)
	goto error;

/* writing TIFF RGB scanlines */
    p_in = buffer;

    for (y = 0; y < height; y++)
      {
	  /* inserting pixels */
	  p_out = tiff_buffer;
	  for (x = 0; x < width; x++)
	    {
		*p_out++ = *p_in++;	/* red */
		*p_out++ = *p_in++;	/* green */
		*p_out++ = *p_in++;	/* blue */
	    }
	  if (TIFFWriteScanline (out, tiff_buffer, y, 0) < 0)
	      goto error;
      }

    free (tiff_buffer);
    return 1;
  error:
    if (tiff_buffer != NULL)
	free (tiff_buffer);
    return 0;
}

static int
output_rgb_tiff (const unsigned char *buffer,
		 unsigned short width,
		 unsigned short height, unsigned char **blob, int *blob_size)
{
/* generating an RGB TIFF - actual work */
    struct memfile clientdata;
    TIFF *out = (TIFF *) 0;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* writing into memory */
    clientdata.buffer = NULL;
    clientdata.malloc_block = 1024;
    clientdata.size = 0;
    clientdata.eof = 0;
    clientdata.current = 0;
    out = TIFFClientOpen ("tiff", "w", &clientdata, memory_readproc,
			  memory_writeproc, memory_seekproc, closeproc,
			  memory_sizeproc, mapproc, unmapproc);
    if (out == NULL)
	return 0;

    if (!rgb_tiff_common (out, buffer, width, height))
	goto error;

    TIFFClose (out);
    *blob = clientdata.buffer;
    *blob_size = clientdata.eof;
    return 1;

  error:
    TIFFClose (out);
    if (clientdata.buffer != NULL)
	free (clientdata.buffer);
    return 0;
}

static int
output_rgb_geotiff (const unsigned char *buffer,
		    unsigned short width,
		    unsigned short height, sqlite3 * handle, double minx,
		    double miny, double maxx, double maxy, int srid,
		    unsigned char **blob, int *blob_size)
{
/* generating an RGB TIFF - actual work */
    struct memfile clientdata;
    double tiepoint[6];
    double pixsize[3];
    double hResolution = (maxx - minx) / (double) width;
    double vResolution = (maxy - miny) / (double) height;
    char *srs_name = NULL;
    char *proj4text = NULL;
    GTIF *gtif = (GTIF *) 0;
    TIFF *out = (TIFF *) 0;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* writing into memory */
    clientdata.buffer = NULL;
    clientdata.malloc_block = 1024;
    clientdata.size = 0;
    clientdata.eof = 0;
    clientdata.current = 0;

    out = XTIFFClientOpen ("tiff", "w", &clientdata, memory_readproc,
			   memory_writeproc, memory_seekproc, closeproc,
			   memory_sizeproc, mapproc, unmapproc);
    if (out == NULL)
	goto error;
    gtif = GTIFNew (out);
    if (gtif == NULL)
	goto error;

/* attempting to retrieve the CRS params */
    fetch_crs_params (handle, srid, &srs_name, &proj4text);
    if (srs_name == NULL || proj4text == NULL)
	goto error;

/* setting up the GeoTIFF Tags */
    pixsize[0] = hResolution;
    pixsize[1] = vResolution;
    pixsize[2] = 0.0;
    TIFFSetField (out, GTIFF_PIXELSCALE, 3, pixsize);
    tiepoint[0] = 0.0;
    tiepoint[1] = 0.0;
    tiepoint[2] = 0.0;
    tiepoint[3] = minx;
    tiepoint[4] = maxy;
    tiepoint[5] = 0.0;
    TIFFSetField (out, GTIFF_TIEPOINTS, 6, tiepoint);
    if (srs_name != NULL)
	TIFFSetField (out, GTIFF_ASCIIPARAMS, srs_name);
    if (proj4text != NULL)
	GTIFSetFromProj4 (gtif, proj4text);
    if (srs_name != NULL)
	GTIFKeySet (gtif, GTCitationGeoKey, TYPE_ASCII, 0, srs_name);
    if (is_projected_srs (proj4text))
	GTIFKeySet (gtif, ProjectedCSTypeGeoKey, TYPE_SHORT, 1, srid);
    GTIFWriteKeys (gtif);

    if (!rgb_tiff_common (out, buffer, width, height))
	goto error;

    GTIFFree (gtif);
    XTIFFClose (out);
    *blob = clientdata.buffer;
    *blob_size = clientdata.eof;
    if (srs_name != NULL)
	free (srs_name);
    if (proj4text != NULL)
	free (proj4text);
    return 1;

  error:
    if (gtif != (GTIF *) 0)
	GTIFFree (gtif);
    if (out != (TIFF *) 0)
	XTIFFClose (out);
    if (srs_name != NULL)
	free (srs_name);
    if (proj4text != NULL)
	free (proj4text);
    if (clientdata.buffer != NULL)
	free (clientdata.buffer);
    return 0;
}

static int
palette_tiff_common (TIFF * out, const unsigned char *buffer,
		     unsigned short width, unsigned short height,
		     unsigned char *red, unsigned char *green,
		     unsigned char *blue, int max_palette)
{
/* common implementation of Palette TIFF export */
    tsize_t buf_size;
    void *tiff_buffer = NULL;
    int y;
    int x;
    int i;
    const unsigned char *p_in;
    unsigned char *p_out;
    uint16 r_plt[256];
    uint16 g_plt[256];
    uint16 b_plt[256];

/* preparing the TIFF palette */
    for (i = 0; i < 256; i++)
      {
	  r_plt[i] = 0;
	  g_plt[i] = 0;
	  b_plt[i] = 0;
      }
    for (i = 0; i < max_palette; i++)
      {
	  r_plt[i] = *(red + i) * 256;
	  g_plt[i] = *(green + i) * 256;
	  b_plt[i] = *(blue + i) * 256;
      }

/* setting up the TIFF headers */
    TIFFSetField (out, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField (out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField (out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField (out, TIFFTAG_XRESOLUTION, 300.0);
    TIFFSetField (out, TIFFTAG_YRESOLUTION, 300.0);
    TIFFSetField (out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField (out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField (out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
    TIFFSetField (out, TIFFTAG_COLORMAP, r_plt, g_plt, b_plt);
    TIFFSetField (out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField (out, TIFFTAG_ROWSPERSTRIP, 1);

/* allocating the TIFF write buffer */
    buf_size = TIFFScanlineSize (out);
    tiff_buffer = malloc (buf_size);
    if (tiff_buffer == NULL)
	goto error;

/* writing TIFF RGB scanlines */
    p_in = buffer;

    for (y = 0; y < height; y++)
      {
	  /* inserting pixels */
	  p_out = tiff_buffer;
	  for (x = 0; x < width; x++)
	    {
		unsigned char r = *p_in++;
		unsigned char g = *p_in++;
		unsigned char b = *p_in++;
		unsigned char index = 0;
		for (i = 0; i < max_palette; i++)
		  {
		      if (*(red + i) == r && *(green + i) == g
			  && *(blue + i) == b)
			{
			    index = i;
			    break;
			}
		  }
		*p_out++ = index;
	    }
	  if (TIFFWriteScanline (out, tiff_buffer, y, 0) < 0)
	      goto error;
      }

    free (tiff_buffer);
    return 1;
  error:
    if (tiff_buffer != NULL)
	free (tiff_buffer);
    return 0;
}

static int
output_palette_tiff (const unsigned char *buffer,
		     unsigned short width,
		     unsigned short height, unsigned char *red,
		     unsigned char *green, unsigned char *blue,
		     int max_palette, unsigned char **blob, int *blob_size)
{
/* generating a PALETTE TIFF - actual work */
    struct memfile clientdata;
    TIFF *out = (TIFF *) 0;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* writing into memory */
    clientdata.buffer = NULL;
    clientdata.malloc_block = 1024;
    clientdata.size = 0;
    clientdata.eof = 0;
    clientdata.current = 0;
    out = TIFFClientOpen ("tiff", "w", &clientdata, memory_readproc,
			  memory_writeproc, memory_seekproc, closeproc,
			  memory_sizeproc, mapproc, unmapproc);
    if (out == NULL)
	return 0;

    if (!palette_tiff_common
	(out, buffer, width, height, red, green, blue, max_palette))
	goto error;

    TIFFClose (out);
    *blob = clientdata.buffer;
    *blob_size = clientdata.eof;
    return 1;

  error:
    TIFFClose (out);
    if (clientdata.buffer != NULL)
	free (clientdata.buffer);
    return 0;
}

static int
output_palette_geotiff (const unsigned char *buffer,
			unsigned short width,
			unsigned short height, sqlite3 * handle, double minx,
			double miny, double maxx, double maxy, int srid,
			unsigned char *red, unsigned char *green,
			unsigned char *blue, int max_palette,
			unsigned char **blob, int *blob_size)
{
/* generating a PALETTE GeoTIFF - actual work */
    struct memfile clientdata;
    double tiepoint[6];
    double pixsize[3];
    double hResolution = (maxx - minx) / (double) width;
    double vResolution = (maxy - miny) / (double) height;
    char *srs_name = NULL;
    char *proj4text = NULL;
    GTIF *gtif = (GTIF *) 0;
    TIFF *out = (TIFF *) 0;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* writing into memory */
    clientdata.buffer = NULL;
    clientdata.malloc_block = 1024;
    clientdata.size = 0;
    clientdata.eof = 0;
    clientdata.current = 0;

    out = XTIFFClientOpen ("tiff", "w", &clientdata, memory_readproc,
			   memory_writeproc, memory_seekproc, closeproc,
			   memory_sizeproc, mapproc, unmapproc);
    if (out == NULL)
	goto error;
    gtif = GTIFNew (out);
    if (gtif == NULL)
	goto error;

/* attempting to retrieve the CRS params */
    fetch_crs_params (handle, srid, &srs_name, &proj4text);
    if (srs_name == NULL || proj4text == NULL)
	goto error;

/* setting up the GeoTIFF Tags */
    pixsize[0] = hResolution;
    pixsize[1] = vResolution;
    pixsize[2] = 0.0;
    TIFFSetField (out, GTIFF_PIXELSCALE, 3, pixsize);
    tiepoint[0] = 0.0;
    tiepoint[1] = 0.0;
    tiepoint[2] = 0.0;
    tiepoint[3] = minx;
    tiepoint[4] = maxy;
    tiepoint[5] = 0.0;
    TIFFSetField (out, GTIFF_TIEPOINTS, 6, tiepoint);
    if (srs_name != NULL)
	TIFFSetField (out, GTIFF_ASCIIPARAMS, srs_name);
    if (proj4text != NULL)
	GTIFSetFromProj4 (gtif, proj4text);
    if (srs_name != NULL)
	GTIFKeySet (gtif, GTCitationGeoKey, TYPE_ASCII, 0, srs_name);
    if (is_projected_srs (proj4text))
	GTIFKeySet (gtif, ProjectedCSTypeGeoKey, TYPE_SHORT, 1, srid);
    GTIFWriteKeys (gtif);

    if (!palette_tiff_common
	(out, buffer, width, height, red, green, blue, max_palette))
	goto error;

    GTIFFree (gtif);
    XTIFFClose (out);
    *blob = clientdata.buffer;
    *blob_size = clientdata.eof;
    if (srs_name != NULL)
	free (srs_name);
    if (proj4text != NULL)
	free (proj4text);
    return 1;

  error:
    if (gtif != (GTIF *) 0)
	GTIFFree (gtif);
    if (out != (TIFF *) 0)
	XTIFFClose (out);
    if (srs_name != NULL)
	free (srs_name);
    if (proj4text != NULL)
	free (proj4text);
    if (clientdata.buffer != NULL)
	free (clientdata.buffer);
    return 0;
}

static int
test_palette_tiff (unsigned short width, unsigned short height,
		   const unsigned char *rgb, unsigned char *red,
		   unsigned char *green, unsigned char *blue, int *max_palette)
{
/* testing for an eventual Palette */
    int next_palette = 0;
    int extra_palette = 0;
    int c;
    int y;
    int x;
    const unsigned char *p_in;

    p_in = rgb;
    for (y = 0; y < height; y++)
      {
	  /* inserting pixels */
	  for (x = 0; x < width; x++)
	    {
		unsigned char r = *p_in++;
		unsigned char g = *p_in++;
		unsigned char b = *p_in++;
		int match = 0;
		for (c = 0; c < next_palette; c++)
		  {
		      if (*(red + c) == r && *(green + c) == g
			  && *(blue + c) == b)
			{
			    /* color already defined into the palette */
			    match = 1;
			    break;
			}
		  }
		if (!match)
		  {
		      /* attempting to insert a color into the palette */
		      if (next_palette < 256)
			{
			    *(red + next_palette) = r;
			    *(green + next_palette) = g;
			    *(blue + next_palette) = b;
			    next_palette++;
			}
		      else
			{
			    extra_palette++;
			    goto palette_invalid;
			}
		  }
	    }
      }
  palette_invalid:
    if (extra_palette)
	return 0;
    *max_palette = next_palette;
    return 1;
}

RL2_DECLARE int
rl2_rgb_to_tiff (unsigned int width, unsigned int height,
		 const unsigned char *rgb, unsigned char **tiff, int *tiff_size)
{
/* creating a TIFF in-memory image from an RGB buffer */
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
    int max_palette = 0;
    if (rgb == NULL)
	return RL2_ERROR;

    if (test_palette_tiff (width, height, rgb, red, green, blue, &max_palette))
      {
	  if (!output_palette_tiff
	      (rgb, width, height, red, green, blue, max_palette, tiff,
	       tiff_size))
	      return RL2_ERROR;
      }
    else
      {
	  if (!output_rgb_tiff (rgb, width, height, tiff, tiff_size))
	      return RL2_ERROR;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_rgb_to_geotiff (unsigned int width, unsigned int height,
		    sqlite3 * handle, double minx, double miny, double maxx,
		    double maxy, int srid, const unsigned char *rgb,
		    unsigned char **tiff, int *tiff_size)
{
/* creating a GeoTIFF in-memory image from an RGB buffer */
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
    int max_palette = 0;
    if (rgb == NULL)
	return RL2_ERROR;

    if (test_palette_tiff (width, height, rgb, red, green, blue, &max_palette))
      {
	  if (!output_palette_geotiff
	      (rgb, width, height, handle, minx, miny, maxx, maxy, srid, red,
	       green, blue, max_palette, tiff, tiff_size))
	      return RL2_ERROR;
      }
    else
      {
	  if (!output_rgb_geotiff
	      (rgb, width, height, handle, minx, miny, maxx, maxy, srid, tiff,
	       tiff_size))
	      return RL2_ERROR;
      }
    return RL2_OK;
}

static int
gray_tiff_common (TIFF * out, const unsigned char *buffer,
		  unsigned short width, unsigned short height)
{
/* common implementation of Grayscale TIFF export */
    tsize_t buf_size;
    void *tiff_buffer = NULL;
    int y;
    int x;
    const unsigned char *p_in;
    unsigned char *p_out;

/* setting up the TIFF headers */
    TIFFSetField (out, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField (out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField (out, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField (out, TIFFTAG_XRESOLUTION, 300.0);
    TIFFSetField (out, TIFFTAG_YRESOLUTION, 300.0);
    TIFFSetField (out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField (out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField (out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField (out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField (out, TIFFTAG_ROWSPERSTRIP, 1);

/* allocating the TIFF write buffer */
    buf_size = TIFFScanlineSize (out);
    tiff_buffer = malloc (buf_size);
    if (tiff_buffer == NULL)
	goto error;

/* writing TIFF RGB scanlines */
    p_in = buffer;

    for (y = 0; y < height; y++)
      {
	  /* inserting pixels */
	  p_out = tiff_buffer;
	  for (x = 0; x < width; x++)
	      *p_out++ = *p_in++;	/* gray */
	  if (TIFFWriteScanline (out, tiff_buffer, y, 0) < 0)
	      goto error;
      }

    free (tiff_buffer);
    return 1;
  error:
    if (tiff_buffer != NULL)
	free (tiff_buffer);
    return 0;
}

static int
output_gray_tiff (const unsigned char *buffer,
		  unsigned short width,
		  unsigned short height, unsigned char **blob, int *blob_size)
{
/* generating a Grayscale TIFF - actual work */
    struct memfile clientdata;
    TIFF *out = (TIFF *) 0;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* writing into memory */
    clientdata.buffer = NULL;
    clientdata.malloc_block = 1024;
    clientdata.size = 0;
    clientdata.eof = 0;
    clientdata.current = 0;
    out = TIFFClientOpen ("tiff", "w", &clientdata, memory_readproc,
			  memory_writeproc, memory_seekproc, closeproc,
			  memory_sizeproc, mapproc, unmapproc);
    if (out == NULL)
	return 0;

    if (!gray_tiff_common (out, buffer, width, height))
	goto error;

    TIFFClose (out);
    *blob = clientdata.buffer;
    *blob_size = clientdata.eof;
    return 1;

  error:
    TIFFClose (out);
    if (clientdata.buffer != NULL)
	free (clientdata.buffer);
    return 0;
}

static int
output_gray_geotiff (const unsigned char *buffer,
		     unsigned short width,
		     unsigned short height, sqlite3 * handle, double minx,
		     double miny, double maxx, double maxy, int srid,
		     unsigned char **blob, int *blob_size)
{
/* generating a Grayscale GeoTIFF - actual work */
    struct memfile clientdata;
    double tiepoint[6];
    double pixsize[3];
    double hResolution = (maxx - minx) / (double) width;
    double vResolution = (maxy - miny) / (double) height;
    char *srs_name = NULL;
    char *proj4text = NULL;
    GTIF *gtif = (GTIF *) 0;
    TIFF *out = (TIFF *) 0;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* writing into memory */
    clientdata.buffer = NULL;
    clientdata.malloc_block = 1024;
    clientdata.size = 0;
    clientdata.eof = 0;
    clientdata.current = 0;

    out = XTIFFClientOpen ("tiff", "w", &clientdata, memory_readproc,
			   memory_writeproc, memory_seekproc, closeproc,
			   memory_sizeproc, mapproc, unmapproc);
    if (out == NULL)
	goto error;
    gtif = GTIFNew (out);
    if (gtif == NULL)
	goto error;

/* attempting to retrieve the CRS params */
    fetch_crs_params (handle, srid, &srs_name, &proj4text);
    if (srs_name == NULL || proj4text == NULL)
	goto error;

/* setting up the GeoTIFF Tags */
    pixsize[0] = hResolution;
    pixsize[1] = vResolution;
    pixsize[2] = 0.0;
    TIFFSetField (out, GTIFF_PIXELSCALE, 3, pixsize);
    tiepoint[0] = 0.0;
    tiepoint[1] = 0.0;
    tiepoint[2] = 0.0;
    tiepoint[3] = minx;
    tiepoint[4] = maxy;
    tiepoint[5] = 0.0;
    TIFFSetField (out, GTIFF_TIEPOINTS, 6, tiepoint);
    if (srs_name != NULL)
	TIFFSetField (out, GTIFF_ASCIIPARAMS, srs_name);
    if (proj4text != NULL)
	GTIFSetFromProj4 (gtif, proj4text);
    if (srs_name != NULL)
	GTIFKeySet (gtif, GTCitationGeoKey, TYPE_ASCII, 0, srs_name);
    if (is_projected_srs (proj4text))
	GTIFKeySet (gtif, ProjectedCSTypeGeoKey, TYPE_SHORT, 1, srid);
    GTIFWriteKeys (gtif);

    if (!gray_tiff_common (out, buffer, width, height))
	goto error;

    GTIFFree (gtif);
    XTIFFClose (out);
    *blob = clientdata.buffer;
    *blob_size = clientdata.eof;
    if (srs_name != NULL)
	free (srs_name);
    if (proj4text != NULL)
	free (proj4text);
    return 1;

  error:
    if (gtif != (GTIF *) 0)
	GTIFFree (gtif);
    if (out != (TIFF *) 0)
	XTIFFClose (out);
    if (srs_name != NULL)
	free (srs_name);
    if (proj4text != NULL)
	free (proj4text);
    if (clientdata.buffer != NULL)
	free (clientdata.buffer);
    return 0;
}

RL2_DECLARE int
rl2_gray_to_tiff (unsigned int width, unsigned int height,
		  const unsigned char *gray, unsigned char **tiff,
		  int *tiff_size)
{
/* creating a TIFF in-memory image from a Grayscale buffer */
    if (gray == NULL)
	return RL2_ERROR;

    if (!output_gray_tiff (gray, width, height, tiff, tiff_size))
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_gray_to_geotiff (unsigned int width, unsigned int height,
		     sqlite3 * handle, double minx, double miny, double maxx,
		     double maxy, int srid, const unsigned char *gray,
		     unsigned char **tiff, int *tiff_size)
{
/* creating a GeoTIFF in-memory image from a Grayscale buffer */
    if (gray == NULL)
	return RL2_ERROR;

    if (!output_gray_geotiff
	(gray, width, height, handle, minx, miny, maxx, maxy, srid, tiff,
	 tiff_size))
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE rl2RasterPtr
rl2_raster_from_tiff (const unsigned char *blob, int blob_size)
{
/* attempting to create a raster from a TIFF image */
    rl2RasterPtr rst = NULL;
    struct memfile clientdata;
    uint32 width = 0;
    uint32 height = 0;
    TIFF *in = (TIFF *) 0;
    unsigned int x;
    unsigned int y;
    unsigned int row;
    uint32 *rgba;
    unsigned char *rgb = NULL;
    unsigned char *mask = NULL;
    int rgb_size;
    int mask_size;
    uint32 *p_in;
    unsigned char *p_rgb;
    unsigned char *p_mask;
    int valid_mask = 0;

/* suppressing TIFF warnings */
    TIFFSetWarningHandler (NULL);

/* reading from memory */
    clientdata.buffer = (unsigned char *) blob;
    clientdata.malloc_block = 1024;
    clientdata.size = blob_size;
    clientdata.eof = blob_size;
    clientdata.current = 0;
    in = TIFFClientOpen ("tiff", "r", &clientdata, memory_readproc,
			 memory_writeproc, memory_seekproc, closeproc,
			 memory_sizeproc, mapproc, unmapproc);
    if (in == NULL)
	return NULL;

/* retrieving the TIFF dimensions */
    TIFFGetField (in, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField (in, TIFFTAG_IMAGEWIDTH, &width);

/* allocating the RGBA buffer */
    rgba = malloc (sizeof (uint32) * width * height);
    if (rgba == NULL)
	goto error;

/* attempting to decode the TIFF */
    if (!TIFFReadRGBAImage (in, width, height, rgba, 1))
	goto error;
    TIFFClose (in);

/* rearranging the RGBA buffer */
    rgb_size = width * height * 3;
    mask_size = width * height;
    rgb = malloc (rgb_size);
    mask = malloc (mask_size);
    if (rgb == NULL || mask == NULL)
	goto error;
    p_in = rgba;
    row = height - 1;
    for (y = 0; y < height; y++)
      {
	  /* the TIFF RGBA buffer is bottom-up !!! */
	  p_rgb = rgb + (row * width * 3);
	  p_mask = mask + (row * width);
	  for (x = 0; x < width; x++)
	    {
		/* copying pixels */
		*p_rgb++ = TIFFGetR (*p_in);
		*p_rgb++ = TIFFGetG (*p_in);
		*p_rgb++ = TIFFGetB (*p_in);
		if (TIFFGetA (*p_in) < 128)
		  {
		      *p_mask++ = 0;
		      valid_mask = 1;
		  }
		else
		    *p_mask++ = 1;
		p_in++;
	    }
	  row--;
      }
    if (!valid_mask)
      {
	  free (mask);
	  mask = NULL;
	  mask_size = 0;
      }
    free (rgba);
    rgba = NULL;

/* creating the raster */
    rst =
	rl2_create_raster (width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   rgb, rgb_size, NULL, mask, mask_size, NULL);
    if (rst == NULL)
	goto error;
    return rst;

  error:
    if (in != (TIFF *) 0)
	TIFFClose (in);
    if (rgba != NULL)
	free (rgba);
    if (rgb != NULL)
	free (rgb);
    if (mask != NULL)
	free (mask);
    return NULL;
}

RL2_DECLARE char *
rl2_build_tiff_xml_summary (rl2TiffOriginPtr tiff)
{
/* attempting to build an XML Summary from a (geo)TIFF */
    char *xml;
    char *prev;
    int len;
    rl2PrivTiffOriginPtr org = (rl2PrivTiffOriginPtr) tiff;
    if (org == NULL)
	return NULL;

    xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<ImportedRaster>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (org->isGeoTiff)
	xml = sqlite3_mprintf ("%s<RasterFormat>GeoTIFF</RasterFormat>", prev);
    else if (org->isGeoReferenced)
	xml =
	    sqlite3_mprintf ("%s<RasterFormat>TIFF+WorldFile</RasterFormat>",
			     prev);
    else
	xml = sqlite3_mprintf ("%s<RasterFormat>TIFF</RasterFormat>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<RasterWidth>%u</RasterWidth>", prev, org->width);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<RasterHeight>%u</RasterHeight>", prev,
			 org->height);
    sqlite3_free (prev);
    prev = xml;
    if (org->isTiled)
      {
	  xml =
	      sqlite3_mprintf ("%s<TileWidth>%u</TileWidth>", prev,
			       org->tileWidth);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<TileHeight>%u</TileHeight>", prev,
			       org->tileHeight);
	  sqlite3_free (prev);
	  prev = xml;
      }
    else
      {
	  xml =
	      sqlite3_mprintf ("%s<RowsPerStrip>%u</RowsPerStrip>", prev,
			       org->rowsPerStrip);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml =
	sqlite3_mprintf ("%s<BitsPerSample>%u</BitsPerSample>", prev,
			 org->bitsPerSample);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<SamplesPerPixel>%u</SamplesPerPixel>", prev,
			 org->samplesPerPixel);
    sqlite3_free (prev);
    prev = xml;
    if (org->photometric == PHOTOMETRIC_MINISBLACK)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>min-is-black</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_MINISWHITE)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>min-is-white</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_RGB)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>RGB</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_PALETTE)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>Palette</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_MASK)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>Mask</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_SEPARATED)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>Separated (CMYC)</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_YCBCR)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>YCbCr</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_CIELAB)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>CIE L*a*b*</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_ICCLAB)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>alternate CIE L*a*b*</PhotometricInterpretation>",
	     prev);
    else if (org->photometric == PHOTOMETRIC_ITULAB)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>ITU L*a*b</PhotometricInterpretation>",
	     prev);
    else
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>%u</PhotometricInterpretation>",
	     prev, org->photometric);
    sqlite3_free (prev);
    prev = xml;
    if (org->compression == COMPRESSION_NONE)
	xml = sqlite3_mprintf ("%s<Compression>none</Compression>", prev);
    else if (org->compression == COMPRESSION_CCITTRLE)
	xml = sqlite3_mprintf ("%s<Compression>CCITT RLE</Compression>", prev);
    else if (org->compression == COMPRESSION_CCITTFAX3)
	xml = sqlite3_mprintf ("%s<Compression>CCITT Fax3</Compression>", prev);
    else if (org->compression == COMPRESSION_CCITTFAX4)
	xml = sqlite3_mprintf ("%s<Compression>CCITT Fax4</Compression>", prev);
    else if (org->compression == COMPRESSION_LZW)
	xml = sqlite3_mprintf ("%s<Compression>LZW</Compression>", prev);
    else if (org->compression == COMPRESSION_OJPEG)
	xml = sqlite3_mprintf ("%s<Compression>old JPEG</Compression>", prev);
    else if (org->compression == COMPRESSION_JPEG)
	xml = sqlite3_mprintf ("%s<Compression>JPEG</Compression>", prev);
    else if (org->compression == COMPRESSION_DEFLATE)
	xml = sqlite3_mprintf ("%s<Compression>DEFLATE</Compression>", prev);
    else if (org->compression == COMPRESSION_ADOBE_DEFLATE)
	xml =
	    sqlite3_mprintf ("%s<Compression>Adobe DEFLATE</Compression>",
			     prev);
    else if (org->compression == COMPRESSION_JBIG)
	xml = sqlite3_mprintf ("%s<Compression>JBIG</Compression>", prev);
    else if (org->compression == COMPRESSION_JP2000)
	xml = sqlite3_mprintf ("%s<Compression>JPEG 2000</Compression>", prev);
    else
	xml =
	    sqlite3_mprintf ("%s<Compression>%u</Compression>", prev,
			     org->compression);
    sqlite3_free (prev);
    prev = xml;
    if (org->sampleFormat == SAMPLEFORMAT_UINT)
	xml =
	    sqlite3_mprintf
	    ("%s<SampleFormat>unsigned integer</SampleFormat>", prev);
    else if (org->sampleFormat == SAMPLEFORMAT_INT)
	xml =
	    sqlite3_mprintf ("%s<SampleFormat>signed integer</SampleFormat>",
			     prev);
    else if (org->sampleFormat == SAMPLEFORMAT_IEEEFP)
	xml =
	    sqlite3_mprintf ("%s<SampleFormat>floating point</SampleFormat>",
			     prev);
    else
	xml =
	    sqlite3_mprintf ("%s<SampleFormat>%u</SampleFormat>", prev,
			     org->sampleFormat);
    sqlite3_free (prev);
    prev = xml;
    if (org->sampleFormat == PLANARCONFIG_SEPARATE)
	xml =
	    sqlite3_mprintf
	    ("%s<PlanarConfiguration>separate Raster planes</PlanarConfiguration>",
	     prev);
    else
	xml =
	    sqlite3_mprintf
	    ("%s<PlanarConfiguration>single Raster plane</PlanarConfiguration>",
	     prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<NoDataPixel>unknown</NoDataPixel>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (org->isGeoReferenced)
      {
	  xml = sqlite3_mprintf ("%s<GeoReferencing>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<SpatialReferenceSystem>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<SRID>%d</SRID>", prev, org->Srid);
	  sqlite3_free (prev);
	  prev = xml;
	  if (org->srsName != NULL)
	      xml =
		  sqlite3_mprintf ("%s<RefSysName>%s</RefSysName>", prev,
				   org->srsName);
	  else
	      xml =
		  sqlite3_mprintf ("%s<RefSysName>undeclared</RefSysName>",
				   prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</SpatialReferenceSystem>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<SpatialResolution>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<HorizontalResolution>%1.10f</HorizontalResolution>", prev,
	       org->hResolution);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<VerticalResolution>%1.10f</VerticalResolution>", prev,
	       org->vResolution);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</SpatialResolution>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<BoundingBox>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<MinX>%1.10f</MinX>", prev, org->minX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<MinY>%1.10f</MinY>", prev, org->minY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<MaxX>%1.10f</MaxX>", prev, org->maxX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<MaxY>%1.10f</MaxY>", prev, org->maxY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</BoundingBox>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Extent>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<HorizontalExtent>%1.10f</HorizontalExtent>", prev,
	       org->maxX - org->minX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<VerticalExtent>%1.10f</VerticalExtent>",
			       prev, org->maxY - org->minY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Extent>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</GeoReferencing>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</ImportedRaster>", prev);
    sqlite3_free (prev);
    len = strlen (xml);
    prev = xml;
    xml = malloc (len + 1);
    strcpy (xml, prev);
    sqlite3_free (prev);
    return xml;
}

RL2_DECLARE const char *
rl2_tiff_version (void)
{
/* returning the TIFF version string */
    static char version[128];
    sprintf (version, "libtiff %d", TIFF_VERSION_CLASSIC);
    return version;
}

RL2_DECLARE const char *
rl2_geotiff_version (void)
{
/* returning the GeoTIFF version string */
    static char version[128];
    sprintf (version, "libgeotiff %d", LIBGEOTIFF_VERSION);
    return version;
}
