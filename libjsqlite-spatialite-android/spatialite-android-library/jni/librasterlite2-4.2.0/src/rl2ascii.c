/*

 rl2ascii -- ASCII Grids related functions

 version 0.1, 2013 December 26

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

#include "rasterlite2/sqlite.h"

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"
#include "rasterlite2_private.h"

static int
parse_ncols (const char *str, unsigned short *width)
{
/* attempting to parse the NCOLS item */
    if (strncasecmp (str, "ncols ", 6) == 0)
      {
	  *width = atoi (str + 6);
	  return 1;
      }
    return 0;
}

static int
parse_nrows (const char *str, unsigned short *height)
{
/* attempting to parse the NROWS item */
    if (strncasecmp (str, "nrows ", 6) == 0)
      {
	  *height = atoi (str + 6);
	  return 1;
      }
    return 0;
}

static int
parse_xllcorner (const char *str, double *minx)
{
/* attempting to parse the XLLCORNER item */
    if (strncasecmp (str, "xllcorner ", 10) == 0)
      {
	  *minx = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_yllcorner (const char *str, double *miny)
{
/* attempting to parse the YLLCORNER item */
    if (strncasecmp (str, "yllcorner ", 10) == 0)
      {
	  *miny = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_xllcenter (const char *str, double *minx)
{
/* attempting to parse the XLLCENTER item */
    if (strncasecmp (str, "xllcenter ", 10) == 0)
      {
	  *minx = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_yllcenter (const char *str, double *miny)
{
/* attempting to parse the YLLCENTER item */
    if (strncasecmp (str, "yllcenter ", 10) == 0)
      {
	  *miny = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_cellsize (const char *str, double *xres)
{
/* attempting to parse the CELLSIZE item */
    if (strncasecmp (str, "cellsize ", 9) == 0)
      {
	  *xres = atof (str + 9);
	  return 1;
      }
    return 0;
}

static int
parse_nodata (const char *str, double *no_data)
{
/* attempting to parse the NODATA_value item */
    if (strncasecmp (str, "NODATA_value ", 13) == 0)
      {
	  *no_data = atof (str + 13);
	  return 1;
      }
    return 0;
}

static int
get_ascii_header (FILE * in, unsigned short *width, unsigned short *height,
		  double *minx, double *miny, double *maxx, double *maxy,
		  double *xres, double *yres, double *no_data)
{
/* attempting to parse the ASCII Header */
    char buf[1024];
    char *p_out = buf;
    int line_no = 0;
    int c;
    int ok_ncols = 0;
    int ok_nrows = 0;
    int ok_xll = 0;
    int ok_yll = 0;
    int ok_cellsize = 0;
    int ok_nodata = 0;

    while ((c = getc (in)) != EOF)
      {
	  if (c == '\r')
	      continue;
	  if (c == '\n')
	    {
		*p_out = '\0';
		if (parse_ncols (buf, width))
		    ok_ncols++;
		if (parse_nrows (buf, height))
		    ok_nrows++;
		if (parse_xllcorner (buf, minx))
		    ok_xll++;
		if (parse_xllcenter (buf, minx))
		    ok_xll++;
		if (parse_yllcorner (buf, miny))
		    ok_yll++;
		if (parse_yllcenter (buf, miny))
		    ok_yll++;
		if (parse_cellsize (buf, xres))
		    ok_cellsize++;
		if (parse_nodata (buf, no_data))
		    ok_nodata++;
		line_no++;
		if (line_no == 6)
		    break;
		p_out = buf;
		continue;
	    }
	  if ((p_out - buf) >= 1024)
	      goto error;
	  *p_out++ = c;
      }
    if (ok_ncols == 1 && ok_nrows == 1 && ok_xll == 1 && ok_yll == 1
	&& ok_cellsize == 1 && ok_nodata == 1)
	;
    else
	goto error;

    *maxx = *minx + ((double) (*width) * *xres);
    *yres = *xres;
    *maxy = *miny + ((double) (*height) * *yres);
    return 1;

  error:
    *width = 0;
    *height = 0;
    *minx = DBL_MAX;
    *miny = DBL_MAX;
    *maxx = 0.0 - DBL_MAX;
    *maxy = 0.0 - DBL_MAX;
    *xres = 0.0;
    *yres = 0.0;
    *no_data = DBL_MAX;
    return 0;
}

static rl2PrivAsciiOriginPtr
alloc_ascii_origin (const char *path, int srid, unsigned char sample_type,
		    unsigned short width, unsigned short height, double minx,
		    double miny, double maxx, double maxy, double xres,
		    double yres, double no_data)
{
/* allocating and initializing an ASCII Grid origin */
    int len;
    rl2PrivAsciiOriginPtr ascii = malloc (sizeof (rl2PrivAsciiOrigin));
    if (ascii == NULL)
	return NULL;
    len = strlen (path);
    ascii->path = malloc (len + 1);
    strcpy (ascii->path, path);
    ascii->tmp = NULL;
    ascii->width = width;
    ascii->height = height;
    ascii->Srid = srid;
    ascii->hResolution = xres;
    ascii->vResolution = yres;
    ascii->minX = minx;
    ascii->minY = miny;
    ascii->maxX = maxx;
    ascii->maxY = maxy;
    ascii->sample_type = sample_type;
    ascii->noData = no_data;
    return ascii;
}

RL2_DECLARE rl2AsciiGridOriginPtr
rl2_create_ascii_grid_origin (const char *path, int srid,
			      unsigned char sample_type)
{
/* creating an ASCII Grid Origin */
    FILE *in;
    unsigned short width;
    unsigned short height;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double xres;
    double yres;
    double no_data;
    char buf[1024];
    char *p_out = buf;
    int line_no = 0;
    int col_no = 0;
    int new_line = 1;
    int c;
    rl2PrivAsciiOriginPtr ascii = NULL;
    void *scanline = NULL;
    char *p_int8;
    unsigned char *p_uint8;
    short *p_int16;
    unsigned short *p_uint16;
    int *p_int32;
    unsigned int *p_uint32;
    float *p_float;
    double *p_double;
    int sz;

    if (path == NULL)
	return NULL;
    if (srid <= 0)
	return NULL;
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
	  return NULL;
      };

    in = fopen (path, "r");
    if (in == NULL)
      {
	  fprintf (stderr, "ASCII Origin: Unable to open %s\n", path);
	  return NULL;
      }
    if (!get_ascii_header
	(in, &width, &height, &minx, &miny, &maxx, &maxy, &xres, &yres,
	 &no_data))
      {
	  fprintf (stderr, "ASCII Origin: invalid Header found on %s\n", path);
	  goto error;
      }

    ascii =
	alloc_ascii_origin (path, srid, sample_type, width, height, minx, miny,
			    maxx, maxy, xres, yres, no_data);
    if (ascii == NULL)
	goto error;

    *buf = '\0';
    col_no = width;
/* creating the helper Temporary File */
    ascii->tmp = tmpfile ();
    if (ascii->tmp == NULL)
	goto error;
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
	  sz = ascii->width;
	  break;
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  sz = ascii->width * 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  sz = ascii->width * 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sz = ascii->width * 8;
	  break;
      };
    scanline = malloc (sz);
    if (scanline == NULL)
	goto error;

    while ((c = getc (in)) != EOF)
      {
	  if (c == '\r')
	      continue;
	  if (c == ' ' || c == '\n')
	    {
		*p_out = '\0';
		if (*buf != '\0')
		  {
		      char int8_value;
		      unsigned char uint8_value;
		      short int16_value;
		      unsigned short uint16_value;
		      int int32_value;
		      unsigned int uint32_value;
		      float flt_value;
		      double dbl_value = atof (buf);
		      if (new_line)
			{
			    if (col_no != width)
				goto error;
			    line_no++;
			    new_line = 0;
			    col_no = 0;
			    switch (sample_type)
			      {
			      case RL2_SAMPLE_INT8:
				  p_int8 = scanline;
				  break;
			      case RL2_SAMPLE_UINT8:
				  p_uint8 = scanline;
				  break;
			      case RL2_SAMPLE_INT16:
				  p_int16 = scanline;
				  break;
			      case RL2_SAMPLE_UINT16:
				  p_uint16 = scanline;
				  break;
			      case RL2_SAMPLE_INT32:
				  p_int32 = scanline;
				  break;
			      case RL2_SAMPLE_UINT32:
				  p_uint32 = scanline;
				  break;
			      case RL2_SAMPLE_FLOAT:
				  p_float = scanline;
				  break;
			      case RL2_SAMPLE_DOUBLE:
				  p_double = scanline;
				  break;
			      };
			}
		      switch (sample_type)
			{
			case RL2_SAMPLE_INT8:
			    int8_value = truncate_8 (dbl_value);
			    *p_int8++ = int8_value;
			    break;
			case RL2_SAMPLE_UINT8:
			    uint8_value = truncate_u8 (dbl_value);
			    *p_uint8++ = uint8_value;
			    break;
			case RL2_SAMPLE_INT16:
			    int16_value = truncate_16 (dbl_value);
			    *p_int16++ = int16_value;
			    break;
			case RL2_SAMPLE_UINT16:
			    uint16_value = truncate_u16 (dbl_value);
			    *p_uint16++ = uint16_value;
			    break;
			case RL2_SAMPLE_INT32:
			    int32_value = truncate_32 (dbl_value);
			    *p_int32++ = int32_value;
			    break;
			case RL2_SAMPLE_UINT32:
			    uint32_value = truncate_u32 (dbl_value);
			    *p_uint32++ = uint32_value;
			    break;
			case RL2_SAMPLE_FLOAT:
			    flt_value = (float) dbl_value;
			    *p_float++ = flt_value;
			    break;
			case RL2_SAMPLE_DOUBLE:
			    *p_double++ = dbl_value;
			    break;
			};
		      col_no++;
		      if (col_no == ascii->width)
			  fwrite (scanline, sz, 1, ascii->tmp);
		  }
		p_out = buf;
		if (c == '\n')
		    new_line = 1;
		continue;
	    }

	  if ((p_out - buf) >= 1024)
	      goto error;
	  *p_out++ = c;
      }
    if (line_no != height)
	goto error;

    fclose (in);
    free (scanline);
    return (rl2AsciiGridOriginPtr) ascii;

  error:
    if (scanline != NULL)
	free (scanline);
    if (ascii != NULL)
	rl2_destroy_ascii_grid_origin ((rl2AsciiGridOriginPtr) ascii);
    if (in != NULL)
	fclose (in);
    return NULL;
}

RL2_DECLARE void
rl2_destroy_ascii_grid_origin (rl2AsciiGridOriginPtr ascii)
{
/* memory cleanup - destroying an ASCII Grid Origin object */
    rl2PrivAsciiOriginPtr org = (rl2PrivAsciiOriginPtr) ascii;
    if (org == NULL)
	return;
    if (org->path != NULL)
	free (org->path);
    if (org->tmp != NULL)
	fclose (org->tmp);
    free (org);
}

RL2_DECLARE const char *
rl2_get_ascii_grid_origin_path (rl2AsciiGridOriginPtr ascii)
{
/* retrieving the input path from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return NULL;

    return origin->path;
}

RL2_DECLARE int
rl2_get_ascii_grid_origin_size (rl2AsciiGridOriginPtr ascii,
				unsigned short *width, unsigned short *height)
{
/* retrieving Width and Height from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *width = origin->width;
    *height = origin->height;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_grid_origin_srid (rl2AsciiGridOriginPtr ascii, int *srid)
{
/* retrieving the SRID from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *srid = origin->Srid;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_grid_origin_extent (rl2AsciiGridOriginPtr ascii, double *minX,
				  double *minY, double *maxX, double *maxY)
{
/* retrieving the Extent from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *minX = origin->minX;
    *minY = origin->minY;
    *maxX = origin->maxX;
    *maxY = origin->maxY;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_grid_origin_resolution (rl2AsciiGridOriginPtr ascii,
				      double *hResolution, double *vResolution)
{
/* retrieving the Pixel Resolution from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *hResolution = origin->hResolution;
    *vResolution = origin->vResolution;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_grid_origin_type (rl2AsciiGridOriginPtr ascii,
				unsigned char *sample_type,
				unsigned char *pixel_type,
				unsigned char *num_bands)
{
/* retrieving the sample/pixel type from an ASCII Grid origin */
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    if (origin == NULL)
	return RL2_ERROR;

    *sample_type = origin->sample_type;
    *pixel_type = RL2_PIXEL_DATAGRID;
    *num_bands = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_eval_ascii_grid_origin_compatibility (rl2CoveragePtr cvg,
					  rl2AsciiGridOriginPtr ascii)
{
/* testing if a Coverage and an ASCII Grid origin are mutually compatible */
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int srid;
    double hResolution;
    double vResolution;
    double confidence;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;

    if (coverage == NULL || ascii == NULL)
	return RL2_ERROR;
    if (rl2_get_ascii_grid_origin_type
	(ascii, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	return RL2_ERROR;

    if (coverage->sampleType != sample_type)
	return RL2_FALSE;
    if (coverage->pixelType != pixel_type)
	return RL2_FALSE;
    if (coverage->nBands != num_bands)
	return RL2_FALSE;

/* checking for resolution compatibility */
    if (rl2_get_ascii_grid_origin_srid (ascii, &srid) != RL2_OK)
	return RL2_FALSE;
    if (coverage->Srid != srid)
	return RL2_FALSE;
    if (rl2_get_ascii_grid_origin_resolution (ascii, &hResolution, &vResolution)
	!= RL2_OK)
	return RL2_FALSE;
    confidence = coverage->hResolution / 100.0;
    if (hResolution < (coverage->hResolution - confidence)
	|| hResolution > (coverage->hResolution + confidence))
	return RL2_FALSE;
    confidence = coverage->vResolution / 100.0;
    if (vResolution < (coverage->vResolution - confidence)
	|| vResolution > (coverage->vResolution + confidence))
	return RL2_FALSE;
    return RL2_TRUE;
}

static int
read_ascii_int8 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		 unsigned short height, unsigned int startRow,
		 unsigned int startCol, char *pixels)
{
/* reading from the Temporary helper file - INT8 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset = (row * origin->width) + startCol;
	  char *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		char int8;
		if (fread (&int8, sizeof (char), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = int8;
	    }
      }
    return 1;
}

static int
read_ascii_uint8 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned int startRow,
		  unsigned int startCol, unsigned char *pixels)
{
/* reading from the Temporary helper file - UINT8 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset = (row * origin->width) + startCol;
	  unsigned char *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned char uint8;
		if (fread (&uint8, sizeof (unsigned char), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = uint8;
	    }
      }
    return 1;
}

static int
read_ascii_int16 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned int startRow,
		  unsigned int startCol, short *pixels)
{
/* reading from the Temporary helper file - INT16 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (short)) +
	      (startCol * sizeof (short));
	  short *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) < 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		short int16;
		if (fread (&int16, sizeof (short), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = int16;
	    }
      }
    return 1;
}

static int
read_ascii_uint16 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		   unsigned short height, unsigned int startRow,
		   unsigned int startCol, unsigned short *pixels)
{
/* reading from the Temporary helper file - UINT16 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (unsigned short)) +
	      (startCol * sizeof (unsigned short));
	  unsigned short *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned short uint16;
		if (fread (&uint16, sizeof (unsigned short), 1, origin->tmp) <=
		    0)
		    return 0;
		*p_out++ = uint16;
	    }
      }
    return 1;
}

static int
read_ascii_int32 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned int startRow,
		  unsigned int startCol, int *pixels)
{
/* reading from the Temporary helper file - INT32 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (int)) + (startCol * sizeof (int));
	  int *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		int int32;
		if (fread (&int32, sizeof (int), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = int32;
	    }
      }
    return 1;
}

static int
read_ascii_uint32 (rl2PrivAsciiOriginPtr origin, unsigned short width,
		   unsigned short height, unsigned int startRow,
		   unsigned int startCol, unsigned int *pixels)
{
/* reading from the Temporary helper file - UINT32 */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (unsigned int)) +
	      (startCol * sizeof (unsigned int));
	  unsigned int *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned int uint32;
		if (fread (&uint32, sizeof (unsigned int), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = uint32;
	    }
      }
    return 1;
}

static int
read_ascii_float (rl2PrivAsciiOriginPtr origin, unsigned short width,
		  unsigned short height, unsigned int startRow,
		  unsigned int startCol, float *pixels)
{
/* reading from the Temporary helper file - FLOAT */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (float)) +
	      (startCol * sizeof (float));
	  float *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		float flt;
		if (fread (&flt, sizeof (float), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = flt;
	    }
      }
    return 1;
}

static int
read_ascii_double (rl2PrivAsciiOriginPtr origin, unsigned short width,
		   unsigned short height, unsigned int startRow,
		   unsigned int startCol, double *pixels)
{
/* reading from the Temporary helper file - DOUBLE */
    int x;
    int y;
    int row;
    int col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  long offset =
	      (row * origin->width * sizeof (double)) +
	      (startCol * sizeof (double));
	  double *p_out = pixels + (y * width);
	  if (fseek (origin->tmp, offset, SEEK_SET) != 0)
	      return 0;
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		double dbl;
		if (fread (&dbl, sizeof (double), 1, origin->tmp) <= 0)
		    return 0;
		*p_out++ = dbl;
	    }
      }
    return 1;
}

static int
read_ascii_pixels (rl2PrivAsciiOriginPtr origin, unsigned short width,
		   unsigned short height, unsigned char sample_type,
		   unsigned int startRow, unsigned int startCol, void *pixels)
{
/* reading from the Temporary helper file */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  return read_ascii_int8 (origin, width, height, startRow, startCol,
				  (char *) pixels);
      case RL2_SAMPLE_UINT8:
	  return read_ascii_uint8 (origin, width, height, startRow, startCol,
				   (unsigned char *) pixels);
      case RL2_SAMPLE_INT16:
	  return read_ascii_int16 (origin, width, height, startRow, startCol,
				   (short *) pixels);
      case RL2_SAMPLE_UINT16:
	  return read_ascii_uint16 (origin, width, height, startRow, startCol,
				    (unsigned short *) pixels);
      case RL2_SAMPLE_INT32:
	  return read_ascii_int32 (origin, width, height, startRow, startCol,
				   (int *) pixels);
      case RL2_SAMPLE_UINT32:
	  return read_ascii_uint32 (origin, width, height, startRow, startCol,
				    (unsigned int *) pixels);
      case RL2_SAMPLE_FLOAT:
	  return read_ascii_float (origin, width, height, startRow, startCol,
				   (float *) pixels);
      case RL2_SAMPLE_DOUBLE:
	  return read_ascii_double (origin, width, height, startRow, startCol,
				    (double *) pixels);
      };
    return 0;
}

static int
read_from_ascii (rl2PrivAsciiOriginPtr origin, unsigned short width,
		 unsigned short height, unsigned char sample_type,
		 unsigned int startRow, unsigned int startCol,
		 unsigned char **pixels, int *pixels_sz)
{
/* creating a tile from the ASCII Grid origin */
    unsigned char *bufPixels = NULL;
    int bufPixelsSz = 0;
    int pix_sz = 1;
    rl2PixelPtr no_data = NULL;

    no_data = rl2_create_pixel (sample_type, RL2_PIXEL_DATAGRID, 1);

/* allocating the pixels buffer */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  pix_sz = 1;
	  rl2_set_pixel_sample_int8 (no_data, (char) (origin->noData));
	  break;
      case RL2_SAMPLE_UINT8:
	  pix_sz = 1;
	  rl2_set_pixel_sample_uint8 (no_data, 0,
				      (unsigned char) (origin->noData));
	  break;
      case RL2_SAMPLE_INT16:
	  rl2_set_pixel_sample_int16 (no_data, (short) (origin->noData));
	  pix_sz = 2;
	  break;
      case RL2_SAMPLE_UINT16:
	  rl2_set_pixel_sample_uint16 (no_data, 0,
				       (unsigned short) (origin->noData));
	  pix_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
	  pix_sz = 4;
	  rl2_set_pixel_sample_int32 (no_data, (int) (origin->noData));
	  break;
      case RL2_SAMPLE_UINT32:
	  pix_sz = 4;
	  rl2_set_pixel_sample_uint32 (no_data,
				       (unsigned int) (origin->noData));
	  break;
      case RL2_SAMPLE_FLOAT:
	  pix_sz = 4;
	  rl2_set_pixel_sample_float (no_data, (float) (origin->noData));
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pix_sz = 8;
	  rl2_set_pixel_sample_double (no_data, (double) (origin->noData));
	  break;
      };
    bufPixelsSz = width * height * pix_sz;
    bufPixels = malloc (bufPixelsSz);
    if (bufPixels == NULL)
	goto error;
    if ((startRow + height) > origin->height
	|| (startCol + width) > origin->width)
	rl2_prime_void_tile (bufPixels, width, height, sample_type, 1, no_data);

    if (!read_ascii_pixels
	(origin, width, height, sample_type, startRow, startCol, bufPixels))
	goto error;

    *pixels = bufPixels;
    *pixels_sz = bufPixelsSz;
    return RL2_OK;
  error:
    if (bufPixels != NULL)
	free (bufPixels);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE rl2RasterPtr
rl2_get_tile_from_ascii_grid_origin (rl2CoveragePtr cvg,
				     rl2AsciiGridOriginPtr ascii,
				     unsigned int startRow,
				     unsigned int startCol)
{
/* attempting to create a Coverage-tile from an ASCII Grid origin */
    unsigned int x;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    rl2PrivAsciiOriginPtr origin = (rl2PrivAsciiOriginPtr) ascii;
    rl2RasterPtr raster = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    int unused_width = 0;
    int unused_height = 0;

    if (coverage == NULL || origin == NULL)
	return NULL;
    if (rl2_eval_ascii_grid_origin_compatibility (cvg, ascii) != RL2_TRUE)
	return NULL;
    if (origin->tmp == NULL)
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

/* attempting to create the tile */
    if (read_from_ascii
	(origin, coverage->tileWidth, coverage->tileHeight,
	 coverage->sampleType, startRow, startCol, &pixels,
	 &pixels_sz) != RL2_OK)
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
	  int shadow_x = coverage->tileWidth - unused_width;
	  int shadow_y = coverage->tileHeight - unused_height;
	  int row;
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
    raster =
	rl2_create_raster (coverage->tileWidth, coverage->tileHeight,
			   coverage->sampleType, RL2_PIXEL_DATAGRID, 1, pixels,
			   pixels_sz, NULL, mask, mask_size, NULL);
    if (raster == NULL)
	goto error;
    return raster;
  error:
    if (pixels != NULL)
	free (pixels);
    if (mask != NULL)
	free (mask);
    return NULL;
}

static rl2PrivAsciiDestinationPtr
alloc_ascii_destination (const char *path, unsigned short width,
			 unsigned short height, double x, double y,
			 double res, int is_centered, double no_data,
			 int decimal_digits)
{
/* allocating and initializing an ASCII Grid detination */
    int len;
    rl2PrivAsciiDestinationPtr ascii =
	malloc (sizeof (rl2PrivAsciiDestination));
    if (ascii == NULL)
	return NULL;
    len = strlen (path);
    ascii->path = malloc (len + 1);
    strcpy (ascii->path, path);
    ascii->out = NULL;
    ascii->width = width;
    ascii->height = height;
    ascii->Resolution = res;
    ascii->X = x;
    ascii->Y = y;
    ascii->isCentered = is_centered;
    ascii->noData = no_data;
    if (decimal_digits < 0)
	ascii->decimalDigits = 0;
    else if (decimal_digits > 18)
	ascii->decimalDigits = 18;
    else
	ascii->decimalDigits = decimal_digits;
    ascii->headerDone = 'N';
    ascii->nextLineNo = 0;
    ascii->pixels = NULL;
    ascii->sampleType = RL2_SAMPLE_UNKNOWN;
    return ascii;
}

RL2_DECLARE rl2AsciiGridDestinationPtr
rl2_create_ascii_grid_destination (const char *path, unsigned short width,
				   unsigned short height, double resolution,
				   double x, double y, int is_centered,
				   double no_data, int decimal_digits,
				   void *pixels, int pixels_size,
				   unsigned char sample_type)
{
/* creating an ASCII Grid Destination */
    FILE *out;
    rl2PrivAsciiDestinationPtr ascii = NULL;
    int pix_sz = 0;

    if (path == NULL)
	return NULL;
    if (pixels == NULL)
	return NULL;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
	  pix_sz = 1;
	  break;
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
    if (pix_sz < 1)
	return NULL;
    if (pixels_size != (width * height * pix_sz))
	return NULL;

    out = fopen (path, "w");
    if (out == NULL)
      {
	  fprintf (stderr, "ASCII Destination: Unable to open %s\n", path);
	  return NULL;
      }

    ascii =
	alloc_ascii_destination (path, width, height, x, y, resolution,
				 is_centered, no_data, decimal_digits);
    if (ascii == NULL)
	goto error;

/* creating the output File */
    out = fopen (path, "wb");
    if (out == NULL)
	goto error;
    ascii->out = out;
    ascii->pixels = pixels;
    ascii->sampleType = sample_type;

    return (rl2AsciiGridDestinationPtr) ascii;

  error:
    if (ascii != NULL)
	rl2_destroy_ascii_grid_destination ((rl2AsciiGridDestinationPtr) ascii);
    if (out != NULL)
	fclose (out);
    return NULL;
}

RL2_DECLARE void
rl2_destroy_ascii_grid_destination (rl2AsciiGridDestinationPtr ascii)
{
/* memory cleanup - destroying an ASCII Grid destination object */
    rl2PrivAsciiDestinationPtr dst = (rl2PrivAsciiDestinationPtr) ascii;
    if (dst == NULL)
	return;
    if (dst->path != NULL)
	free (dst->path);
    if (dst->out != NULL)
	fclose (dst->out);
    if (dst->pixels != NULL)
	free (dst->pixels);
    free (dst);
}

RL2_DECLARE const char *
rl2_get_ascii_grid_destination_path (rl2AsciiGridDestinationPtr ascii)
{
/* retrieving the input path from an ASCII Grid destination */
    rl2PrivAsciiDestinationPtr dst = (rl2PrivAsciiDestinationPtr) ascii;
    if (dst == NULL)
	return NULL;

    return dst->path;
}

RL2_DECLARE int
rl2_get_ascii_grid_destination_size (rl2AsciiGridDestinationPtr ascii,
				     unsigned short *width,
				     unsigned short *height)
{
/* retrieving Width and Height from an ASCII Grid destination */
    rl2PrivAsciiDestinationPtr dst = (rl2PrivAsciiDestinationPtr) ascii;
    if (dst == NULL)
	return RL2_ERROR;

    *width = dst->width;
    *height = dst->height;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_grid_destination_tiepoint (rl2AsciiGridDestinationPtr ascii,
					 double *X, double *Y)
{
/* retrieving the tiepoint from an ASCII Grid destination */
    rl2PrivAsciiDestinationPtr dst = (rl2PrivAsciiDestinationPtr) ascii;
    if (dst == NULL)
	return RL2_ERROR;

    *X = dst->X;
    *Y = dst->Y;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_ascii_grid_destination_resolution (rl2AsciiGridDestinationPtr ascii,
					   double *resolution)
{
/* retrieving the Pixel Resolution from an ASCII Grid destination */
    rl2PrivAsciiDestinationPtr dst = (rl2PrivAsciiDestinationPtr) ascii;
    if (dst == NULL)
	return RL2_ERROR;

    *resolution = dst->Resolution;
    return RL2_OK;
}

RL2_DECLARE int
rl2_write_ascii_grid_header (rl2AsciiGridDestinationPtr ascii)
{
/* attempting to write the ASCII Grid header */
    rl2PrivAsciiDestinationPtr dst = (rl2PrivAsciiDestinationPtr) ascii;
    if (dst == NULL)
	return RL2_ERROR;
    if (dst->out == NULL)
	return RL2_ERROR;
    if (dst->headerDone != 'N')
	return RL2_ERROR;

    fprintf (dst->out, "ncols %u\r\n", dst->width);
    fprintf (dst->out, "nrows %u\r\n", dst->height);
    if (dst->isCentered)
      {
	  fprintf (dst->out, "xllcenter %1.8f\r\n", dst->X);
	  fprintf (dst->out, "yllcenter %1.8f\r\n", dst->Y);
      }
    else
      {
	  fprintf (dst->out, "xllcorner %1.8f\r\n", dst->X);
	  fprintf (dst->out, "yllcorner %1.8f\r\n", dst->Y);
      }
    fprintf (dst->out, "cellsize %1.8f\r\n", dst->Resolution);
    fprintf (dst->out, "NODATA_value %1.8f\r\n", dst->noData);
    dst->headerDone = 'Y';
    return RL2_OK;
}

static char *
format_pixel (double cell_value, int decimal_digits)
{
/* well formatting an ASCII pixel */
    char format[32];
    char *pixel;
    char *p;
    sprintf (format, " %%1.%df", decimal_digits);
    pixel = sqlite3_mprintf (format, cell_value);
    if (decimal_digits == 0)
	return pixel;
    p = pixel + strlen (pixel) - 1;
    while (1)
      {
	  if (*p == '0')
	      *p = '\0';
	  else if (*p == '.')
	    {
		*p = '\0';
		break;
	    }
	  else
	      break;
	  p--;
      }
    return pixel;
}

RL2_DECLARE int
rl2_write_ascii_grid_scanline (rl2AsciiGridDestinationPtr ascii,
			       unsigned short *line_no)
{
/* attempting to write a scanline into an ASCII Grid */
    char *p8;
    unsigned char *pu8;
    short *p16;
    unsigned short *pu16;
    int *p32;
    unsigned int *pu32;
    float *pflt;
    double *pdbl;
    double cell_value;
    char *pxl;
    int x;
    rl2PrivAsciiDestinationPtr dst = (rl2PrivAsciiDestinationPtr) ascii;

    if (dst == NULL)
	return RL2_ERROR;
    if (dst->out == NULL)
	return RL2_ERROR;
    if (dst->headerDone != 'Y')
	return RL2_ERROR;
    if (dst->nextLineNo >= dst->height)
	return RL2_ERROR;

    switch (dst->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  p8 = dst->pixels;
	  p8 += (dst->nextLineNo * dst->width);
	  break;
      case RL2_SAMPLE_UINT8:
	  pu8 = dst->pixels;
	  pu8 += (dst->nextLineNo * dst->width);
	  break;
      case RL2_SAMPLE_INT16:
	  p16 = dst->pixels;
	  p16 += (dst->nextLineNo * dst->width);
	  break;
      case RL2_SAMPLE_UINT16:
	  pu16 = dst->pixels;
	  pu16 += (dst->nextLineNo * dst->width);
	  break;
      case RL2_SAMPLE_INT32:
	  p32 = dst->pixels;
	  p32 += (dst->nextLineNo * dst->width);
	  break;
      case RL2_SAMPLE_UINT32:
	  pu32 = dst->pixels;
	  pu32 += (dst->nextLineNo * dst->width);
	  break;
      case RL2_SAMPLE_FLOAT:
	  pflt = dst->pixels;
	  pflt += (dst->nextLineNo * dst->width);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pdbl = dst->pixels;
	  pdbl += (dst->nextLineNo * dst->width);
	  break;
      };

    for (x = 0; x < dst->width; x++)
      {
	  switch (dst->sampleType)
	    {
	    case RL2_SAMPLE_INT8:
		cell_value = *p8++;
		break;
	    case RL2_SAMPLE_UINT8:
		cell_value = *pu8++;
		break;
	    case RL2_SAMPLE_INT16:
		cell_value = *p16++;
		break;
	    case RL2_SAMPLE_UINT16:
		cell_value = *pu16++;
		break;
	    case RL2_SAMPLE_INT32:
		cell_value = *p32++;
		break;
	    case RL2_SAMPLE_UINT32:
		cell_value = *pu32++;
		break;
	    case RL2_SAMPLE_FLOAT:
		cell_value = *pflt++;
		break;
	    case RL2_SAMPLE_DOUBLE:
		cell_value = *pdbl++;
		break;
	    };
	  pxl = format_pixel (cell_value, dst->decimalDigits);
	  fprintf (dst->out, "%s", pxl);
	  sqlite3_free (pxl);
      }
    fprintf (dst->out, "\r\n");

    dst->nextLineNo += 1;
    *line_no = dst->nextLineNo;
    return RL2_OK;
}
