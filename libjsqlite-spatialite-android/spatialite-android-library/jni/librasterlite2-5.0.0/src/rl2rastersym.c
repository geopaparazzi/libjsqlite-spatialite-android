/*

 rl2rastersym -- Raster Symbolizer - applying the style

 version 0.1, 2014 March 23

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

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%I64d\n"
#else
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%lld\n"
#endif

static void
copy_int8_raw_pixels (const char *buffer, const unsigned char *mask,
		      char *outbuf, unsigned short width,
		      unsigned short height, double x_res, double y_res,
		      double minx, double maxy, double tile_minx,
		      double tile_maxy, unsigned short tile_width,
		      unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying INT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const char *p_in = buffer;
    const unsigned char *p_msk = mask;
    char *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const char *p_save = p_in;
		      char sample = 0;
		      rl2_get_pixel_sample_int8 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_int8_raw_pixels_transparent (const char *buffer, const unsigned char *mask,
				  char *outbuf, unsigned char *outmask,
				  unsigned short width, unsigned short height,
				  double x_res, double y_res, double minx,
				  double maxy, double tile_minx,
				  double tile_maxy, unsigned short tile_width,
				  unsigned short tile_height,
				  rl2PixelPtr no_data)
{
/* copying INT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const char *p_in = buffer;
    const unsigned char *p_msk = mask;
    char *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const char *p_save = p_in;
		      char sample = 0;
		      rl2_get_pixel_sample_int8 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
	    }
      }
}

static unsigned char *
apply_color_map (double mono, unsigned char *p_out,
		 rl2BandHandlingPtr mono_handling)
{
/* applying a ColorMap */
    int found = 0;
    rl2ColorMapRefPtr rule;
    rl2ColorMapItemPtr color;
    double scaled =
	((double) mono - mono_handling->minValue) / mono_handling->scaleFactor;
    int i = (int) scaled;
    if (i < 0)
	i = 0;
    if (i > 255)
	i = 255;
    color = &(mono_handling->colorMap->look_up[i]);
    rule = color->first;
    while (rule != NULL)
      {
	  if (rule->min <= mono && rule->max > mono)
	    {
		if (mono_handling->colorMap->interpolate)
		  {
		      /* Interpolate */
		      double span = rule->max - rule->min;
		      double pos = (double) mono - rule->min;
		      double scale = pos / span;
		      double red_span = (double) (rule->maxRed - rule->red);
		      double green_span =
			  (double) (rule->maxGreen - rule->green);
		      double blue_span = (double) (rule->maxBlue - rule->blue);
		      double red = (double) (rule->red) + (red_span * scale);
		      double green =
			  (double) (rule->green) + (green_span * scale);
		      double blue = (double) (rule->blue) + (blue_span * scale);
		      *p_out++ = (unsigned char) red;
		      *p_out++ = (unsigned char) green;
		      *p_out++ = (unsigned char) blue;
		  }
		else
		  {
		      /* Categorize */
		      *p_out++ = rule->red;
		      *p_out++ = rule->green;
		      *p_out++ = rule->blue;
		  }
		found = 1;
		break;
	    }
	  rule = rule->next;
      }
    if (!found)
      {
	  /* applying the default RGB color */
	  *p_out++ = mono_handling->colorMap->red;
	  *p_out++ = mono_handling->colorMap->green;
	  *p_out++ = mono_handling->colorMap->blue;
      }
    return p_out;
}

static unsigned char *
ndvi_uint8_pixel_handler (const unsigned char *p_in, unsigned char *p_out,
			  unsigned char red_band, unsigned char nir_band,
			  rl2BandHandlingPtr ndvi_handling)
{
/* styling an opaque NDVI pixel - UINT8 */
    unsigned char red = *(p_in + red_band);
    unsigned char nir = *(p_in + nir_band);
    double ndvi = ((double) nir - (double) red) / ((double) nir + (double) red);
/* applying the ColorMap */
    return apply_color_map (ndvi, p_out, ndvi_handling);
}

static void
copy_uint8_ndvi_pixels (const unsigned char *buffer,
			const unsigned char *mask, unsigned char *outbuf,
			unsigned short width, unsigned short height,
			unsigned char out_num_bands,
			unsigned char num_bands, double x_res, double y_res,
			double minx, double maxy, double tile_minx,
			double tile_maxy, unsigned short tile_width,
			unsigned short tile_height, rl2PixelPtr no_data,
			unsigned char red_band, unsigned char nir_band,
			rl2BandHandlingPtr ndvi_handling)
{
/* copying UINT8 NDVI pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				ndvi_uint8_pixel_handler (p_in, p_out,
							  red_band, nir_band,
							  ndvi_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned char sample = 0;
			    rl2_get_pixel_sample_uint8 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				ndvi_uint8_pixel_handler (p_in, p_out,
							  red_band, nir_band,
							  ndvi_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_uint8_ndvi_pixels_transparent (const unsigned char *buffer,
				    const unsigned char *mask,
				    unsigned char *outbuf,
				    unsigned char *outmask,
				    unsigned short width, unsigned short height,
				    unsigned char out_num_bands,
				    unsigned char num_bands, double x_res,
				    double y_res, double minx, double maxy,
				    double tile_minx, double tile_maxy,
				    unsigned short tile_width,
				    unsigned short tile_height,
				    rl2PixelPtr no_data, unsigned char red_band,
				    unsigned char nir_band,
				    rl2BandHandlingPtr ndvi_handling)
{
/* copying UINT8 NDVI pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				ndvi_uint8_pixel_handler (p_in, p_out,
							  red_band, nir_band,
							  ndvi_handling);
			    *p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned char sample = 0;
			    rl2_get_pixel_sample_uint8 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				ndvi_uint8_pixel_handler (p_in, p_out,
							  red_band, nir_band,
							  ndvi_handling);
			    *p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static unsigned char *
ndvi_uint16_pixel_handler (const unsigned short *p_in, unsigned char *p_out,
			   unsigned char red_band, unsigned char nir_band,
			   rl2BandHandlingPtr ndvi_handling)
{
/* styling an opaque NDVI pixel - UINT16 */
    unsigned short red = *(p_in + red_band);
    unsigned short nir = *(p_in + nir_band);
    double ndvi = ((double) nir - (double) red) / ((double) nir + (double) red);
/* applying the ColorMap */
    return apply_color_map (ndvi, p_out, ndvi_handling);
}

static void
copy_uint16_ndvi_pixels (const unsigned short *buffer,
			 const unsigned char *mask, unsigned char *outbuf,
			 unsigned short width, unsigned short height,
			 unsigned char out_num_bands,
			 unsigned char num_bands, double x_res,
			 double y_res, double minx, double maxy,
			 double tile_minx, double tile_maxy,
			 unsigned short tile_width,
			 unsigned short tile_height, rl2PixelPtr no_data,
			 unsigned char red_band, unsigned char nir_band,
			 rl2BandHandlingPtr ndvi_handling)
{
/* copying UINT16 NDVI pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				ndvi_uint16_pixel_handler (p_in, p_out,
							   red_band, nir_band,
							   ndvi_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				ndvi_uint16_pixel_handler (p_in, p_out,
							   red_band, nir_band,
							   ndvi_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_uint16_ndvi_pixels_transparent (const unsigned short *buffer,
				     const unsigned char *mask,
				     unsigned char *outbuf,
				     unsigned char *outmask,
				     unsigned short width,
				     unsigned short height,
				     unsigned char out_num_bands,
				     unsigned char num_bands, double x_res,
				     double y_res, double minx, double maxy,
				     double tile_minx, double tile_maxy,
				     unsigned short tile_width,
				     unsigned short tile_height,
				     rl2PixelPtr no_data,
				     unsigned char red_band,
				     unsigned char nir_band,
				     rl2BandHandlingPtr ndvi_handling)
{
/* copying UINT16 NDVI pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				ndvi_uint16_pixel_handler (p_in, p_out,
							   red_band, nir_band,
							   ndvi_handling);
			    *p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				ndvi_uint16_pixel_handler (p_in, p_out,
							   red_band, nir_band,
							   ndvi_handling);
			    *p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk = 0;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static unsigned char *
apply_contrast_enhancement (double mono, unsigned char *p_out,
			    rl2BandHandlingPtr mono_handling)
{
/* applying Contrast Enhancement */
    double scaled;
    if (mono_handling->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_GAMMA)
      {
	  /* applying Gamma Value */
	  if (mono <= mono_handling->minValue)
	      *p_out++ = mono_handling->look_up[0];
	  else if (mono >= mono_handling->maxValue)
	      *p_out++ = mono_handling->look_up[255];
	  else
	    {
		scaled =
		    1.0 +
		    (((double) mono -
		      mono_handling->minValue) / mono_handling->scaleFactor);
		*p_out++ = mono_handling->look_up[(unsigned char) scaled];
	    }
      }
    else if (mono_handling->contrastEnhancement ==
	     RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
      {
	  /* applying Stretching/Normalize */
	  if (mono <= mono_handling->minValue)
	      *p_out++ = 0;
	  else if (mono >= mono_handling->maxValue)
	      *p_out++ = 255;
	  else
	    {
		scaled =
		    1.0 +
		    (((double) mono -
		      mono_handling->minValue) / mono_handling->scaleFactor);
		*p_out++ = (unsigned char) scaled;
	    }
      }
    else
      {
	  /* applying Trivial Normalization */
	  if (mono_handling->scaleFactor == 1.0)
	      *p_out++ = mono - mono_handling->minValue;
	  else
	    {
		if (mono <= mono_handling->minValue)
		    *p_out++ = 0;
		else if (mono >= mono_handling->maxValue)
		    *p_out++ = 255;
		else
		  {
		      scaled =
			  1.0 +
			  (((double) mono -
			    mono_handling->minValue) /
			   mono_handling->scaleFactor);
		      *p_out++ = (char) scaled;
		  }
	    }
      }
    return p_out;
}

static unsigned char *
mono_int8_pixel_handler (const char *p_in, unsigned char *p_out,
			 unsigned char mono_band,
			 rl2BandHandlingPtr mono_handling)
{
/* styling an opaque pixel - INT8 */
    char mono = *(p_in + mono_band);
    if (mono_handling->colorMap != NULL)
      {
	  /* applying a ColorMap */
	  return apply_color_map ((double) mono, p_out, mono_handling);
      }
    return apply_contrast_enhancement ((double) mono, p_out, mono_handling);
}

static void
copy_int8_raw_mono_pixels (const char *buffer,
			   const unsigned char *mask, unsigned char *outbuf,
			   unsigned short width, unsigned short height,
			   unsigned char out_num_bands, double x_res,
			   double y_res, double minx, double maxy,
			   double tile_minx, double tile_maxy,
			   unsigned short tile_width,
			   unsigned short tile_height, rl2PixelPtr no_data,
			   unsigned char mono_band,
			   rl2BandHandlingPtr mono_handling)
{
/* copying INT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_int8_pixel_handler (p_in, p_out,
							 mono_band,
							 mono_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const char *p_save = p_in;
		      char sample = 0;
		      rl2_get_pixel_sample_int8 (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_int8_pixel_handler (p_in, p_out,
							 mono_band,
							 mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_int8_raw_mono_pixels_transparent (const char *buffer,
				       const unsigned char *mask,
				       unsigned char *outbuf,
				       unsigned char *outmask,
				       unsigned short width,
				       unsigned short height,
				       unsigned char out_num_bands,
				       double x_res, double y_res, double minx,
				       double maxy, double tile_minx,
				       double tile_maxy,
				       unsigned short tile_width,
				       unsigned short tile_height,
				       rl2PixelPtr no_data,
				       unsigned char mono_band,
				       rl2BandHandlingPtr mono_handling)
{
/* copying INT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_int8_pixel_handler (p_in, p_out,
							 mono_band,
							 mono_handling);
			    *p_outmsk = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const char *p_save = p_in;
		      char sample = 0;
		      rl2_get_pixel_sample_int8 (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_int8_pixel_handler (p_in, p_out,
							 mono_band,
							 mono_handling);
			    *p_outmsk = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_uint8_raw_pixels (const unsigned char *buffer, const unsigned char *mask,
		       unsigned char *outbuf, unsigned short width,
		       unsigned short height, unsigned char num_bands,
		       double x_res, double y_res, double minx, double maxy,
		       double tile_minx, double tile_maxy,
		       unsigned short tile_width, unsigned short tile_height,
		       rl2PixelPtr no_data, unsigned char raster_type)
{
/* copying UINT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_1_BIT
		    || sample_type == RL2_SAMPLE_2_BIT
		    || sample_type == RL2_SAMPLE_4_BIT
		    || sample_type == RL2_SAMPLE_UINT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * num_bands) + (out_x * num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      for (b = 0; b < num_bands; b++)
			{
			    if (transparent)
			      {
				  /* skipping a transparent pixel */
				  p_out++;
				  p_in++;
			      }
			    else
				*p_out++ = *p_in++;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      if (raster_type == RL2_SAMPLE_1_BIT && num_bands == 1)
			{
			    /* special case: MONOCHROME */
			    if (*p_in++ == 1)
				*p_out++ = 1;
			    else
				p_out++;
			    match++;
			}
		      else
			{
			    for (b = 0; b < num_bands; b++)
			      {
				  unsigned char sample = 0;
				  switch (sample_type)
				    {
				    case RL2_SAMPLE_1_BIT:
					rl2_get_pixel_sample_1bit (no_data,
								   &sample);
					break;
				    case RL2_SAMPLE_2_BIT:
					rl2_get_pixel_sample_2bit (no_data,
								   &sample);
					break;
				    case RL2_SAMPLE_4_BIT:
					rl2_get_pixel_sample_4bit (no_data,
								   &sample);
					break;
				    case RL2_SAMPLE_UINT8:
					rl2_get_pixel_sample_uint8 (no_data, b,
								    &sample);
					break;
				    };
				  if (sample == *p_in++)
				      match++;
			      }
			    if (match != num_bands)
			      {
				  /* opaque pixel */
				  p_in = p_save;
				  for (b = 0; b < num_bands; b++)
				      *p_out++ = *p_in++;
			      }
			    else
			      {
				  /* NO-DATA pixel */
				  for (b = 0; b < num_bands; b++)
				      p_out++;
			      }
			}
		  }
	    }
      }
}

static void
copy_uint8_raw_pixels_transparent (const unsigned char *buffer,
				   const unsigned char *mask,
				   unsigned char *outbuf,
				   unsigned char *outmask, unsigned short width,
				   unsigned short height,
				   unsigned char num_bands, double x_res,
				   double y_res, double minx, double maxy,
				   double tile_minx, double tile_maxy,
				   unsigned short tile_width,
				   unsigned short tile_height,
				   rl2PixelPtr no_data)
{
/* copying UINT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_1_BIT
		    || sample_type == RL2_SAMPLE_2_BIT
		    || sample_type == RL2_SAMPLE_4_BIT
		    || sample_type == RL2_SAMPLE_UINT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * num_bands) + (out_x * num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      for (b = 0; b < num_bands; b++)
			{
			    if (transparent)
			      {
				  /* skipping a transparent pixel */
				  p_out++;
				  p_in++;
			      }
			    else
				*p_out++ = *p_in++;
			}
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned char sample = 0;
			    switch (sample_type)
			      {
			      case RL2_SAMPLE_1_BIT:
				  rl2_get_pixel_sample_1bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_2_BIT:
				  rl2_get_pixel_sample_2bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_4_BIT:
				  rl2_get_pixel_sample_4bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_UINT8:
				  rl2_get_pixel_sample_uint8 (no_data, b,
							      &sample);
				  break;
			      };
			    if (sample == *p_in++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    for (b = 0; b < num_bands; b++)
				*p_out++ = *p_in++;
			    if (outmask != NULL)
				*p_outmsk++ = 1;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (b = 0; b < num_bands; b++)
				p_out++;
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		  }
	    }
      }
}

static unsigned char *
mono_uint8_pixel_handler (const unsigned char *p_in, unsigned char *p_out,
			  unsigned char mono_band,
			  rl2BandHandlingPtr mono_handling)
{
/* styling an opaque pixel - UINT8 */
    unsigned char mono = *(p_in + mono_band);
    if (mono_handling->colorMap != NULL)
      {
	  /* applying a ColorMap */
	  return apply_color_map ((double) mono, p_out, mono_handling);
      }
    return apply_contrast_enhancement ((double) mono, p_out, mono_handling);
}

static void
copy_uint8_raw_selected_pixels (const unsigned char *buffer,
				const unsigned char *mask,
				unsigned char *outbuf, unsigned short width,
				unsigned short height,
				unsigned char num_bands, double x_res,
				double y_res, double minx, double maxy,
				double tile_minx, double tile_maxy,
				unsigned short tile_width,
				unsigned short tile_height,
				rl2PixelPtr no_data, unsigned char red_band,
				unsigned char green_band,
				unsigned char blue_band,
				rl2BandHandlingPtr red_handling,
				rl2BandHandlingPtr green_handling,
				rl2BandHandlingPtr blue_handling)
{
/* copying UINT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width * 3) + (out_x * 3);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out += 3;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  red_band,
							  red_handling);
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  green_band,
							  green_handling);
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  blue_band,
							  blue_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned char sample = 0;
			    rl2_get_pixel_sample_uint8 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  red_band,
							  red_handling);
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  green_band,
							  green_handling);
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  blue_band,
							  blue_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out += 3;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_uint8_raw_selected_pixels_transparent (const unsigned char *buffer,
					    const unsigned char *mask,
					    unsigned char *outbuf,
					    unsigned char *outmask,
					    unsigned short width,
					    unsigned short height,
					    unsigned char num_bands,
					    double x_res, double y_res,
					    double minx, double maxy,
					    double tile_minx, double tile_maxy,
					    unsigned short tile_width,
					    unsigned short tile_height,
					    rl2PixelPtr no_data,
					    unsigned char red_band,
					    unsigned char green_band,
					    unsigned char blue_band,
					    rl2BandHandlingPtr red_handling,
					    rl2BandHandlingPtr green_handling,
					    rl2BandHandlingPtr blue_handling)
{
/* copying UINT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;
    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width * 3) + (out_x * 3);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out += 3;
			    p_outmsk++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  red_band,
							  red_handling);
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  green_band,
							  green_handling);
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  blue_band,
							  blue_handling);
			    *p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned char sample = 0;
			    rl2_get_pixel_sample_uint8 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  red_band,
							  red_handling);
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  green_band,
							  green_handling);
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  blue_band,
							  blue_handling);
			    *p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out += 3;
			    p_outmsk++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_uint8_raw_mono_pixels (const unsigned char *buffer,
			    const unsigned char *mask, unsigned char *outbuf,
			    unsigned short width, unsigned short height,
			    unsigned char out_num_bands,
			    unsigned char num_bands, double x_res,
			    double y_res, double minx, double maxy,
			    double tile_minx, double tile_maxy,
			    unsigned short tile_width,
			    unsigned short tile_height, rl2PixelPtr no_data,
			    unsigned char mono_band,
			    rl2BandHandlingPtr mono_handling)
{
/* copying UINT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned char sample = 0;
			    rl2_get_pixel_sample_uint8 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_uint8_raw_mono_pixels_transparent (const unsigned char *buffer,
					const unsigned char *mask,
					unsigned char *outbuf,
					unsigned char *outmask,
					unsigned short width,
					unsigned short height,
					unsigned char out_num_bands,
					unsigned char num_bands, double x_res,
					double y_res, double minx, double maxy,
					double tile_minx, double tile_maxy,
					unsigned short tile_width,
					unsigned short tile_height,
					rl2PixelPtr no_data,
					unsigned char mono_band,
					rl2BandHandlingPtr mono_handling)
{
/* copying UINT8 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT8)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			    *p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned char sample = 0;
			    rl2_get_pixel_sample_uint8 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint8_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			    *p_outmsk = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_int16_raw_pixels (const short *buffer, const unsigned char *mask,
		       short *outbuf, unsigned short width,
		       unsigned short height, double x_res, double y_res,
		       double minx, double maxy, double tile_minx,
		       double tile_maxy, unsigned short tile_width,
		       unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying INT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const short *p_in = buffer;
    const unsigned char *p_msk = mask;
    short *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const short *p_save = p_in;
		      short sample = 0;
		      rl2_get_pixel_sample_int16 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_int16_raw_pixels_transparent (const short *buffer,
				   const unsigned char *mask, short *outbuf,
				   unsigned char *outmask, unsigned short width,
				   unsigned short height, double x_res,
				   double y_res, double minx, double maxy,
				   double tile_minx, double tile_maxy,
				   unsigned short tile_width,
				   unsigned short tile_height,
				   rl2PixelPtr no_data)
{
/* copying INT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const short *p_in = buffer;
    const unsigned char *p_msk = mask;
    short *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const short *p_save = p_in;
		      short sample = 0;
		      rl2_get_pixel_sample_int16 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
	    }
      }
}

static unsigned char *
mono_int16_pixel_handler (const short *p_in, unsigned char *p_out,
			  unsigned char mono_band,
			  rl2BandHandlingPtr mono_handling)
{
/* styling an opaque pixel - INT16 */
    short mono = *(p_in + mono_band);
    if (mono_handling->colorMap != NULL)
      {
	  /* applying a ColorMap */
	  return apply_color_map ((double) mono, p_out, mono_handling);
      }
    return apply_contrast_enhancement ((double) mono, p_out, mono_handling);
}

static void
copy_int16_raw_mono_pixels (const short *buffer,
			    const unsigned char *mask, unsigned char *outbuf,
			    unsigned short width, unsigned short height,
			    unsigned char out_num_bands, double x_res,
			    double y_res, double minx, double maxy,
			    double tile_minx, double tile_maxy,
			    unsigned short tile_width,
			    unsigned short tile_height, rl2PixelPtr no_data,
			    unsigned char mono_band,
			    rl2BandHandlingPtr mono_handling)
{
/* copying INT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_int16_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const short *p_save = p_in;
		      short sample = 0;
		      rl2_get_pixel_sample_int16 (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_int16_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_int16_raw_mono_pixels_transparent (const short *buffer,
					const unsigned char *mask,
					unsigned char *outbuf,
					unsigned char *outmask,
					unsigned short width,
					unsigned short height,
					unsigned char out_num_bands,
					double x_res, double y_res, double minx,
					double maxy, double tile_minx,
					double tile_maxy,
					unsigned short tile_width,
					unsigned short tile_height,
					rl2PixelPtr no_data,
					unsigned char mono_band,
					rl2BandHandlingPtr mono_handling)
{
/* copying INT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_int16_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			    *p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const short *p_save = p_in;
		      short sample = 0;
		      rl2_get_pixel_sample_int16 (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_int16_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			    *p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    p_outmsk++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_uint16_raw_pixels (const unsigned short *buffer,
			const unsigned char *mask, unsigned short *outbuf,
			unsigned short width, unsigned short height,
			unsigned char num_bands, double x_res, double y_res,
			double minx, double maxy, double tile_minx,
			double tile_maxy, unsigned short tile_width,
			unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying UINT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned short *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * num_bands) + (out_x * num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      for (b = 0; b < num_bands; b++)
			{
			    if (transparent)
			      {
				  /* skipping a transparent pixel */
				  p_out++;
				  p_in++;
			      }
			    else
				*p_out++ = *p_in++;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_in++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    for (b = 0; b < num_bands; b++)
				*p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (b = 0; b < num_bands; b++)
				p_out++;
			}
		  }
	    }
      }
}

static void
copy_uint16_raw_pixels_transparent (const unsigned short *buffer,
				    const unsigned char *mask,
				    unsigned short *outbuf,
				    unsigned char *outmask,
				    unsigned short width, unsigned short height,
				    unsigned char num_bands, double x_res,
				    double y_res, double minx, double maxy,
				    double tile_minx, double tile_maxy,
				    unsigned short tile_width,
				    unsigned short tile_height,
				    rl2PixelPtr no_data)
{
/* copying UINT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned short *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * num_bands) + (out_x * num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      for (b = 0; b < num_bands; b++)
			{
			    if (transparent)
			      {
				  /* skipping a transparent pixel */
				  p_out++;
				  p_in++;
			      }
			    else
				*p_out++ = *p_in++;
			}
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_in++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    for (b = 0; b < num_bands; b++)
				*p_out++ = *p_in++;
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (b = 0; b < num_bands; b++)
				p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
	    }
      }
}

static unsigned char *
mono_uint16_pixel_handler (const unsigned short *p_in, unsigned char *p_out,
			   unsigned char mono_band,
			   rl2BandHandlingPtr mono_handling)
{
/* styling an opaque pixel - UINT16 */
    unsigned short mono = *(p_in + mono_band);
    if (mono_handling->colorMap != NULL)
      {
	  /* applying a ColorMap */
	  return apply_color_map ((double) mono, p_out, mono_handling);
      }
    return apply_contrast_enhancement ((double) mono, p_out, mono_handling);
}

static void
copy_uint16_raw_selected_pixels (const unsigned short *buffer,
				 const unsigned char *mask,
				 unsigned char *outbuf, unsigned short width,
				 unsigned short height,
				 unsigned char num_bands, double x_res,
				 double y_res, double minx, double maxy,
				 double tile_minx, double tile_maxy,
				 unsigned short tile_width,
				 unsigned short tile_height,
				 rl2PixelPtr no_data, unsigned char red_band,
				 unsigned char green_band,
				 unsigned char blue_band,
				 rl2BandHandlingPtr red_handling,
				 rl2BandHandlingPtr green_handling,
				 rl2BandHandlingPtr blue_handling)
{
/* copying UINT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width * 3) + (out_x * 3);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out += 3;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   red_band,
							   red_handling);
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   green_band,
							   green_handling);
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   blue_band,
							   blue_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   red_band,
							   red_handling);
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   green_band,
							   green_handling);
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   blue_band,
							   blue_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out += 3;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_uint16_raw_selected_pixels_transparent (const unsigned short *buffer,
					     const unsigned char *mask,
					     unsigned char *outbuf,
					     unsigned char *outmask,
					     unsigned short width,
					     unsigned short height,
					     unsigned char num_bands,
					     double x_res, double y_res,
					     double minx, double maxy,
					     double tile_minx, double tile_maxy,
					     unsigned short tile_width,
					     unsigned short tile_height,
					     rl2PixelPtr no_data,
					     unsigned char red_band,
					     unsigned char green_band,
					     unsigned char blue_band,
					     rl2BandHandlingPtr red_handling,
					     rl2BandHandlingPtr green_handling,
					     rl2BandHandlingPtr blue_handling)
{
/* copying UINT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width * 3) + (out_x * 3);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out += 3;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   red_band,
							   red_handling);
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   green_band,
							   green_handling);
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   blue_band,
							   blue_handling);
			}
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   red_band,
							   red_handling);
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   green_band,
							   green_handling);
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   blue_band,
							   blue_handling);
			    if (outmask != NULL)
				*p_outmsk = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out += 3;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_uint16_raw_mono_pixels (const unsigned short *buffer,
			     const unsigned char *mask, unsigned char *outbuf,
			     unsigned short width, unsigned short height,
			     unsigned char out_num_bands,
			     unsigned char num_bands, double x_res,
			     double y_res, double minx, double maxy,
			     double tile_minx, double tile_maxy,
			     unsigned short tile_width,
			     unsigned short tile_height, rl2PixelPtr no_data,
			     unsigned char mono_band,
			     rl2BandHandlingPtr mono_handling)
{
/* copying UINT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_uint16_raw_mono_pixels_transparent (const unsigned short *buffer,
					 const unsigned char *mask,
					 unsigned char *outbuf,
					 unsigned char *outmask,
					 unsigned short width,
					 unsigned short height,
					 unsigned char out_num_bands,
					 unsigned char num_bands, double x_res,
					 double y_res, double minx, double maxy,
					 double tile_minx, double tile_maxy,
					 unsigned short tile_width,
					 unsigned short tile_height,
					 rl2PixelPtr no_data,
					 unsigned char mono_band,
					 rl2BandHandlingPtr mono_handling)
{
/* copying UINT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int b;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != num_bands)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT16)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (b = 0; b < num_bands; b++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, b, &sample);
			    if (sample == *p_save++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint16_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
		p_in += num_bands;
	    }
      }
}

static void
copy_int32_raw_pixels (const int *buffer, const unsigned char *mask,
		       int *outbuf, unsigned short width,
		       unsigned short height, double x_res, double y_res,
		       double minx, double maxy, double tile_minx,
		       double tile_maxy, unsigned short tile_width,
		       unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying INT32 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const int *p_in = buffer;
    const unsigned char *p_msk = mask;
    int *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT32)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const int *p_save = p_in;
		      int sample = 0;
		      rl2_get_pixel_sample_int32 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_int32_raw_pixels_transparent (const int *buffer, const unsigned char *mask,
				   int *outbuf, unsigned char *outmask,
				   unsigned short width, unsigned short height,
				   double x_res, double y_res, double minx,
				   double maxy, double tile_minx,
				   double tile_maxy, unsigned short tile_width,
				   unsigned short tile_height,
				   rl2PixelPtr no_data)
{
/* copying INT32 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const int *p_in = buffer;
    const unsigned char *p_msk = mask;
    int *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT32)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const int *p_save = p_in;
		      int sample = 0;
		      rl2_get_pixel_sample_int32 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
	    }
      }
}

static unsigned char *
mono_int32_pixel_handler (const int *p_in, unsigned char *p_out,
			  unsigned char mono_band,
			  rl2BandHandlingPtr mono_handling)
{
/* styling an opaque pixel - INT32 */
    int mono = *(p_in + mono_band);
    if (mono_handling->colorMap != NULL)
      {
	  /* applying a ColorMap */
	  return apply_color_map ((double) mono, p_out, mono_handling);
      }
    return apply_contrast_enhancement ((double) mono, p_out, mono_handling);
}

static void
copy_int32_raw_mono_pixels (const int *buffer,
			    const unsigned char *mask, unsigned char *outbuf,
			    unsigned short width, unsigned short height,
			    unsigned char out_num_bands, double x_res,
			    double y_res, double minx, double maxy,
			    double tile_minx, double tile_maxy,
			    unsigned short tile_width,
			    unsigned short tile_height, rl2PixelPtr no_data,
			    unsigned char mono_band,
			    rl2BandHandlingPtr mono_handling)
{
/* copying INT32 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const int *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT32)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_int32_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const int *p_save = p_in;
		      int sample = 0;
		      rl2_get_pixel_sample_int32 (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_int32_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_int32_raw_mono_pixels_transparent (const int *buffer,
					const unsigned char *mask,
					unsigned char *outbuf,
					unsigned char *outmask,
					unsigned short width,
					unsigned short height,
					unsigned char out_num_bands,
					double x_res, double y_res, double minx,
					double maxy, double tile_minx,
					double tile_maxy,
					unsigned short tile_width,
					unsigned short tile_height,
					rl2PixelPtr no_data,
					unsigned char mono_band,
					rl2BandHandlingPtr mono_handling)
{
/* copying INT32 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const int *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_INT32)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_int32_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const int *p_save = p_in;
		      int sample = 0;
		      rl2_get_pixel_sample_int32 (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_int32_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_uint32_raw_pixels (const unsigned int *buffer, const unsigned char *mask,
			unsigned int *outbuf, unsigned short width,
			unsigned short height, double x_res, double y_res,
			double minx, double maxy, double tile_minx,
			double tile_maxy, unsigned short tile_width,
			unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying INT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned int *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned int *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT32)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned int *p_save = p_in;
		      unsigned int sample = 0;
		      rl2_get_pixel_sample_uint32 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_uint32_raw_pixels_transparent (const unsigned int *buffer,
				    const unsigned char *mask,
				    unsigned int *outbuf,
				    unsigned char *outmask,
				    unsigned short width, unsigned short height,
				    double x_res, double y_res, double minx,
				    double maxy, double tile_minx,
				    double tile_maxy, unsigned short tile_width,
				    unsigned short tile_height,
				    rl2PixelPtr no_data)
{
/* copying INT16 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned int *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned int *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT32)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned int *p_save = p_in;
		      unsigned int sample = 0;
		      rl2_get_pixel_sample_uint32 (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
	    }
      }
}

static unsigned char *
mono_uint32_pixel_handler (const unsigned int *p_in, unsigned char *p_out,
			   unsigned char mono_band,
			   rl2BandHandlingPtr mono_handling)
{
/* styling an opaque pixel - UINT32 */
    unsigned int mono = *(p_in + mono_band);
    if (mono_handling->colorMap != NULL)
      {
	  /* applying a ColorMap */
	  return apply_color_map ((double) mono, p_out, mono_handling);
      }
    return apply_contrast_enhancement ((double) mono, p_out, mono_handling);
}

static void
copy_uint32_raw_mono_pixels (const unsigned int *buffer,
			     const unsigned char *mask, unsigned char *outbuf,
			     unsigned short width, unsigned short height,
			     unsigned char out_num_bands, double x_res,
			     double y_res, double minx, double maxy,
			     double tile_minx, double tile_maxy,
			     unsigned short tile_width,
			     unsigned short tile_height, rl2PixelPtr no_data,
			     unsigned char mono_band,
			     rl2BandHandlingPtr mono_handling)
{
/* copying UINT32 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned int *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT32)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint32_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned int *p_save = p_in;
		      unsigned int sample = 0;
		      rl2_get_pixel_sample_uint32 (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint32_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_uint32_raw_mono_pixels_transparent (const unsigned int *buffer,
					 const unsigned char *mask,
					 unsigned char *outbuf,
					 unsigned char *outmask,
					 unsigned short width,
					 unsigned short height,
					 unsigned char out_num_bands,
					 double x_res, double y_res,
					 double minx, double maxy,
					 double tile_minx, double tile_maxy,
					 unsigned short tile_width,
					 unsigned short tile_height,
					 rl2PixelPtr no_data,
					 unsigned char mono_band,
					 rl2BandHandlingPtr mono_handling)
{
/* copying UINT32 raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned int *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_UINT32)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_uint32_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned int *p_save = p_in;
		      unsigned int sample = 0;
		      rl2_get_pixel_sample_uint32 (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_uint32_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_float_raw_pixels (const float *buffer, const unsigned char *mask,
		       float *outbuf, unsigned short width,
		       unsigned short height, double x_res, double y_res,
		       double minx, double maxy, double tile_minx,
		       double tile_maxy, unsigned short tile_width,
		       unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying FLOAT raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const float *p_in = buffer;
    const unsigned char *p_msk = mask;
    float *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_FLOAT)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const float *p_save = p_in;
		      float sample = 0;
		      rl2_get_pixel_sample_float (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_float_raw_pixels_transparent (const float *buffer,
				   const unsigned char *mask, float *outbuf,
				   unsigned char *outmask, unsigned short width,
				   unsigned short height, double x_res,
				   double y_res, double minx, double maxy,
				   double tile_minx, double tile_maxy,
				   unsigned short tile_width,
				   unsigned short tile_height,
				   rl2PixelPtr no_data)
{
/* copying FLOAT raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const float *p_in = buffer;
    const unsigned char *p_msk = mask;
    float *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_FLOAT)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const float *p_save = p_in;
		      float sample = 0;
		      rl2_get_pixel_sample_float (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
	    }
      }
}

static unsigned char *
mono_float_pixel_handler (const float *p_in, unsigned char *p_out,
			  unsigned char mono_band,
			  rl2BandHandlingPtr mono_handling)
{
/* styling an opaque pixel - FLOAT */
    float mono = *(p_in + mono_band);
    if (mono_handling->colorMap != NULL)
      {
	  /* applying a ColorMap */
	  return apply_color_map ((double) mono, p_out, mono_handling);
      }
    return apply_contrast_enhancement ((double) mono, p_out, mono_handling);
}

static void
copy_float_raw_mono_pixels (const float *buffer,
			    const unsigned char *mask, unsigned char *outbuf,
			    unsigned short width, unsigned short height,
			    unsigned char out_num_bands, double x_res,
			    double y_res, double minx, double maxy,
			    double tile_minx, double tile_maxy,
			    unsigned short tile_width,
			    unsigned short tile_height, rl2PixelPtr no_data,
			    unsigned char mono_band,
			    rl2BandHandlingPtr mono_handling)
{
/* copying FLOAT raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const float *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_FLOAT)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_float_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      float sample = 0.0;
		      rl2_get_pixel_sample_float (no_data, &sample);
		      if (sample == *p_in)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_float_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_float_raw_mono_pixels_transparent (const float *buffer,
					const unsigned char *mask,
					unsigned char *outbuf,
					unsigned char *outmask,
					unsigned short width,
					unsigned short height,
					unsigned char out_num_bands,
					double x_res, double y_res, double minx,
					double maxy, double tile_minx,
					double tile_maxy,
					unsigned short tile_width,
					unsigned short tile_height,
					rl2PixelPtr no_data,
					unsigned char mono_band,
					rl2BandHandlingPtr mono_handling)
{
/* copying FLOAT raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const float *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_FLOAT)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_float_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			}
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      float sample = 0.0;
		      rl2_get_pixel_sample_float (no_data, &sample);
		      if (sample == *p_in)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_float_pixel_handler (p_in, p_out,
							  mono_band,
							  mono_handling);
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_double_raw_pixels (const double *buffer, const unsigned char *mask,
			double *outbuf, unsigned short width,
			unsigned short height, double x_res, double y_res,
			double minx, double maxy, double tile_minx,
			double tile_maxy, unsigned short tile_width,
			unsigned short tile_height, rl2PixelPtr no_data)
{
/* copying DOUBLE raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const double *p_in = buffer;
    const unsigned char *p_msk = mask;
    double *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_DOUBLE)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const double *p_save = p_in;
		      double sample = 0;
		      rl2_get_pixel_sample_double (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }
}

static void
copy_double_raw_pixels_transparent (const double *buffer,
				    const unsigned char *mask, double *outbuf,
				    unsigned char *outmask,
				    unsigned short width, unsigned short height,
				    double x_res, double y_res, double minx,
				    double maxy, double tile_minx,
				    double tile_maxy, unsigned short tile_width,
				    unsigned short tile_height,
				    rl2PixelPtr no_data)
{
/* copying DOUBLE raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const double *p_in = buffer;
    const unsigned char *p_msk = mask;
    double *p_out;
    unsigned char *p_outmsk;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_DOUBLE)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in++;
			}
		      else
			  *p_out++ = *p_in++;
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const double *p_save = p_in;
		      double sample = 0;
		      rl2_get_pixel_sample_double (no_data, &sample);
		      if (sample == *p_in++)
			  match = 1;
		      if (!match)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    *p_out++ = *p_in++;
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
	    }
      }
}

static unsigned char *
mono_double_pixel_handler (const double *p_in, unsigned char *p_out,
			   unsigned char mono_band,
			   rl2BandHandlingPtr mono_handling)
{
/* styling an opaque pixel - DOUBLE */
    double mono = *(p_in + mono_band);
    if (mono_handling->colorMap != NULL)
      {
	  /* applying a ColorMap */
	  return apply_color_map ((double) mono, p_out, mono_handling);
      }
    return apply_contrast_enhancement ((double) mono, p_out, mono_handling);
}

static void
copy_double_raw_mono_pixels (const double *buffer,
			     const unsigned char *mask, unsigned char *outbuf,
			     unsigned short width, unsigned short height,
			     unsigned char out_num_bands, double x_res,
			     double y_res, double minx, double maxy,
			     double tile_minx, double tile_maxy,
			     unsigned short tile_width,
			     unsigned short tile_height, rl2PixelPtr no_data,
			     unsigned char mono_band,
			     rl2BandHandlingPtr mono_handling)
{
/* copying DOUBLE raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const double *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_DOUBLE)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_double_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const double *p_save = p_in;
		      double sample = 0;
		      rl2_get_pixel_sample_double (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_double_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		  }
		p_in++;
	    }
      }
}

static void
copy_double_raw_mono_pixels_transparent (const double *buffer,
					 const unsigned char *mask,
					 unsigned char *outbuf,
					 unsigned char *outmask,
					 unsigned short width,
					 unsigned short height,
					 unsigned char out_num_bands,
					 double x_res, double y_res,
					 double minx, double maxy,
					 double tile_minx, double tile_maxy,
					 unsigned short tile_width,
					 unsigned short tile_height,
					 rl2PixelPtr no_data,
					 unsigned char mono_band,
					 rl2BandHandlingPtr mono_handling)
{
/* copying DOUBLE raw pixels from the DBMS tile into the output image */
    int x;
    int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const double *p_in = buffer;
    const unsigned char *p_msk = mask;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    int ib;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    if (no_data != NULL)
      {
	  if (rl2_is_pixel_none (no_data) == RL2_FALSE)
	    {
		ignore_no_data = 0;
		if (rl2_get_pixel_type
		    (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
		    ignore_no_data = 1;
		if (nbands != 1)
		    ignore_no_data = 1;
		if (sample_type == RL2_SAMPLE_DOUBLE)
		    ;
		else
		    ignore_no_data = 1;
	    }
      }

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= height)
	    {
		p_in += tile_width;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= width)
		  {
		      p_in++;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out =
		    outbuf + (out_y * width * out_num_bands) +
		    (out_x * out_num_bands);
		p_outmsk = outmask + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			}
		      else
			{
			    /* opaque pixel */
			    p_out =
				mono_double_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			}
		      if (outmask != NULL)
			{
			    if (transparent)
				p_outmsk++;
			    else
				*p_outmsk++ = 0;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const double *p_save = p_in;
		      double sample = 0;
		      rl2_get_pixel_sample_double (no_data, &sample);
		      if (sample == *p_save++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    p_out =
				mono_double_pixel_handler (p_in, p_out,
							   mono_band,
							   mono_handling);
			    if (outmask != NULL)
				*p_outmsk++ = 0;
			}
		      else
			{
			    /* NO-DATA pixel */
			    for (ib = 0; ib < out_num_bands; ib++)
				p_out++;
			    if (outmask != NULL)
				p_outmsk++;
			}
		  }
		p_in++;
	    }
      }
}

static void
compute_stretching (rl2PrivBandStatisticsPtr band, double *min, double *max,
		    double *scale_factor)
{
/* computing the Stretching/Normalize values */
    int i;
    double total;
    double sum = 0.0;
    double percentile_2;
    double percentile_98;
    double vmin = DBL_MAX;
    double vmax = 0.0 - DBL_MAX;
    double range;

    for (i = 0; i < band->nHistogram; i++)
	sum += *(band->histogram + i);
    total = sum;
    percentile_2 = (sum / 100.0) * 2.0;
    percentile_98 = sum - percentile_2;

    sum = 0.0;
    for (i = 0; i < band->nHistogram; i++)
      {
	  /* identifying the 2^ percentile */
	  if (sum >= percentile_2)
	    {
		double r = (band->max - band->min) / 254.0;
		vmin = band->min + ((double) i * r);
		break;
	    }
	  sum += *(band->histogram + i);
      }
    sum = total;
    for (i = band->nHistogram - 1; i >= 0; i--)
      {
	  /* identifying the 98^ percentile */
	  if (sum <= percentile_98)
	    {
		double r = (band->max - band->min) / 254.0;
		vmax = band->min + ((double) i * r);
		break;
	    }
	  sum -= *(band->histogram + i);
      }
    *min = vmin;
    *max = vmax;
    range = vmax - vmin;
    *scale_factor = range / 254.0;
}

static void
build_triple_band_handling (rl2PrivRasterSymbolizerPtr style,
			    rl2PrivRasterStatisticsPtr stats,
			    unsigned char red_band, unsigned char green_band,
			    unsigned char blue_band,
			    rl2BandHandlingPtr * red_handling,
			    rl2BandHandlingPtr * green_handling,
			    rl2BandHandlingPtr * blue_handling)
{
/* creating BandContrastEnhancement helper structs */
    rl2BandHandlingPtr r = NULL;
    rl2BandHandlingPtr g = NULL;
    rl2BandHandlingPtr b = NULL;
    rl2PrivBandStatisticsPtr band;
    double range;
    int i;
    if (style->bandSelection != NULL)
      {
	  /* attempting to use band specific settings */
	  if (style->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
	    {
		if (red_band < stats->nBands)
		  {
		      band = stats->band_stats + red_band;
		      if (style->bandSelection->redContrast ==
			  RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
			{
			    r = malloc (sizeof (rl2BandHandling));
			    r->colorMap = NULL;
			    r->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
			    compute_stretching (band, &(r->minValue),
						&(r->maxValue),
						&(r->scaleFactor));
			}
		      else if (style->bandSelection->redContrast ==
			       RL2_CONTRAST_ENHANCEMENT_GAMMA)
			{
			    r = malloc (sizeof (rl2BandHandling));
			    r->colorMap = NULL;
			    r->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_GAMMA;
			    r->minValue = band->min;
			    r->maxValue = band->max;
			    range = band->max - band->min;
			    r->scaleFactor = range / 254.0;
			    r->look_up[0] = 0;
			    for (i = 1; i < 255; i++)
				r->look_up[i] =
				    (unsigned
				     char) (pow ((double) i / 254.0,
						 1.0 /
						 style->
						 bandSelection->redGamma) *
					    254 + 0.5);
			    r->look_up[255] = 255;
			}
		      else if (style->bandSelection->redContrast ==
			       RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
			{
			    int j;
			    double count = 0.0;
			    double sum;
			    double his[256];
			    r = malloc (sizeof (rl2BandHandling));
			    r->colorMap = NULL;
			    r->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
			    r->minValue = band->min;
			    r->maxValue = band->max;
			    range = band->max - band->min;
			    r->scaleFactor = range / 254.0;
			    r->look_up[0] = 0;
			    for (i = 1; i < 256; i++)
				count += *(band->histogram + i);
			    for (i = 1; i < 256; i++)
				his[i] = *(band->histogram + i) / count;
			    for (i = 1; i < 256; i++)
			      {
				  sum = 0.0;
				  for (j = 1; j <= i; j++)
				      sum += his[j];
				  r->look_up[i] =
				      (unsigned char) (254.0 * sum + 0.5);
				  r->look_up[255] = 255;
			      }
			}
		  }
		if (green_band < stats->nBands)
		  {
		      band = stats->band_stats + green_band;
		      if (style->bandSelection->greenContrast ==
			  RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
			{
			    g = malloc (sizeof (rl2BandHandling));
			    g->colorMap = NULL;
			    g->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
			    compute_stretching (band, &(g->minValue),
						&(g->maxValue),
						&(g->scaleFactor));
			}
		      else if (style->bandSelection->greenContrast ==
			       RL2_CONTRAST_ENHANCEMENT_GAMMA)
			{
			    g = malloc (sizeof (rl2BandHandling));
			    g->colorMap = NULL;
			    g->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_GAMMA;
			    g->minValue = band->min;
			    g->maxValue = band->max;
			    range = band->max - band->min;
			    g->scaleFactor = range / 254.0;
			    g->look_up[0] = 0;
			    for (i = 1; i < 255; i++)
				g->look_up[i] =
				    (unsigned
				     char) (pow ((double) i / 254.0,
						 1.0 /
						 style->
						 bandSelection->greenGamma) *
					    254 + 0.5);
			    g->look_up[255] = 255;
			}
		      else if (style->bandSelection->greenContrast ==
			       RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
			{
			    int j;
			    double count = 0.0;
			    double sum;
			    double his[256];
			    g = malloc (sizeof (rl2BandHandling));
			    g->colorMap = NULL;
			    g->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
			    g->minValue = band->min;
			    g->maxValue = band->max;
			    range = band->max - band->min;
			    g->scaleFactor = range / 254.0;
			    g->look_up[0] = 0;
			    for (i = 1; i < 256; i++)
				count += *(band->histogram + i);
			    for (i = 1; i < 256; i++)
				his[i] = *(band->histogram + i) / count;
			    for (i = 1; i < 256; i++)
			      {
				  sum = 0.0;
				  for (j = 1; j <= i; j++)
				      sum += his[j];
				  g->look_up[i] =
				      (unsigned char) (254.0 * sum + 0.5);
				  g->look_up[255] = 255;
			      }
			}
		  }
		if (blue_band < stats->nBands)
		  {
		      band = stats->band_stats + blue_band;
		      if (style->bandSelection->blueContrast ==
			  RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
			{
			    b = malloc (sizeof (rl2BandHandling));
			    b->colorMap = NULL;
			    b->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
			    compute_stretching (band, &(b->minValue),
						&(b->maxValue),
						&(b->scaleFactor));
			}
		      else if (style->bandSelection->blueContrast ==
			       RL2_CONTRAST_ENHANCEMENT_GAMMA)
			{
			    b = malloc (sizeof (rl2BandHandling));
			    b->colorMap = NULL;
			    b->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_GAMMA;
			    b->minValue = band->min;
			    b->maxValue = band->max;
			    range = band->max - band->min;
			    b->scaleFactor = range / 254.0;
			    b->look_up[0] = 0;
			    for (i = 1; i < 255; i++)
				b->look_up[i] =
				    (unsigned
				     char) (pow ((double) i / 254.0,
						 1.0 /
						 style->
						 bandSelection->blueGamma) *
					    254 + 0.5);
			    b->look_up[255] = 255;
			}
		      else if (style->bandSelection->blueContrast ==
			       RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
			{
			    int j;
			    double count = 0.0;
			    double sum;
			    double his[256];
			    b = malloc (sizeof (rl2BandHandling));
			    b->colorMap = NULL;
			    b->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
			    b->minValue = band->min;
			    b->maxValue = band->max;
			    range = band->max - band->min;
			    b->scaleFactor = range / 254.0;
			    b->look_up[0] = 0;
			    for (i = 1; i < 256; i++)
				count += *(band->histogram + i);
			    for (i = 1; i < 256; i++)
				his[i] = *(band->histogram + i) / count;
			    for (i = 1; i < 256; i++)
			      {
				  sum = 0.0;
				  for (j = 1; j <= i; j++)
				      sum += his[j];
				  b->look_up[i] =
				      (unsigned char) (254.0 * sum + 0.5);
			      }
			    b->look_up[255] = 255;
			}
		  }
	    }
      }
    if (r == NULL)
      {
	  /* using overall settings */
	  if (red_band < stats->nBands)
	    {
		band = stats->band_stats + red_band;
		if (style->contrastEnhancement ==
		    RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
		  {
		      r = malloc (sizeof (rl2BandHandling));
		      r->colorMap = NULL;
		      r->contrastEnhancement =
			  RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
		      compute_stretching (band, &(r->minValue),
					  &(r->maxValue), &(r->scaleFactor));
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_NONE)
		  {
		      r = malloc (sizeof (rl2BandHandling));
		      r->colorMap = NULL;
		      r->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_NONE;
		      if (band->min >= 0.0 && band->max <= 255.0)
			{
			    r->minValue = 0.0;
			    r->maxValue = 255.0;
			    r->scaleFactor = 1.0;
			}
		      else
			{
			    r->minValue = band->min;
			    r->maxValue = band->max;
			    range = band->max - band->min;
			    r->scaleFactor = range / 254.0;
			}
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_GAMMA)
		  {
		      r = malloc (sizeof (rl2BandHandling));
		      r->colorMap = NULL;
		      r->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_GAMMA;
		      r->minValue = band->min;
		      r->maxValue = band->max;
		      range = band->max - band->min;
		      r->scaleFactor = range / 254.0;
		      r->look_up[0] = 0;
		      for (i = 1; i < 255; i++)
			  r->look_up[i] =
			      (unsigned
			       char) (pow ((double) i / 254.0,
					   1.0 / style->gammaValue) * 254 +
				      0.5);
		      r->look_up[255] = 255;
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
		  {
		      int j;
		      double count = 0.0;
		      double sum;
		      double his[256];
		      r = malloc (sizeof (rl2BandHandling));
		      r->colorMap = NULL;
		      r->contrastEnhancement =
			  RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
		      r->minValue = band->min;
		      r->maxValue = band->max;
		      range = band->max - band->min;
		      r->scaleFactor = range / 254.0;
		      r->look_up[0] = 0;
		      for (i = 1; i < 256; i++)
			  count += *(band->histogram + i);
		      for (i = 1; i < 256; i++)
			  his[i] = *(band->histogram + i) / count;
		      for (i = 1; i < 256; i++)
			{
			    sum = 0.0;
			    for (j = 1; j <= i; j++)
				sum += his[j];
			    r->look_up[i] = (unsigned char) (254.0 * sum + 0.5);
			}
		      r->look_up[255] = 255;
		  }
	    }
      }
    if (g == NULL)
      {
	  /* using overall settings */
	  if (green_band < stats->nBands)
	    {
		band = stats->band_stats + green_band;
		if (style->contrastEnhancement ==
		    RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
		  {
		      g = malloc (sizeof (rl2BandHandling));
		      g->colorMap = NULL;
		      g->contrastEnhancement =
			  RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
		      compute_stretching (band, &(g->minValue),
					  &(g->maxValue), &(g->scaleFactor));
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_NONE)
		  {
		      g = malloc (sizeof (rl2BandHandling));
		      g->colorMap = NULL;
		      g->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_NONE;
		      if (band->min >= 0.0 && band->max <= 255.0)
			{
			    g->minValue = 0.0;
			    g->maxValue = 255.0;
			    g->scaleFactor = 1.0;
			}
		      else
			{
			    g->minValue = band->min;
			    g->maxValue = band->max;
			    range = band->max - band->min;
			    g->scaleFactor = range / 254.0;
			}
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_GAMMA)
		  {
		      g = malloc (sizeof (rl2BandHandling));
		      g->colorMap = NULL;
		      g->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_GAMMA;
		      g->minValue = band->min;
		      g->maxValue = band->max;
		      range = band->max - band->min;
		      g->scaleFactor = range / 254.0;
		      g->look_up[0] = 0;
		      for (i = 1; i < 255; i++)
			  g->look_up[i] =
			      (unsigned
			       char) (pow ((double) i / 254.0,
					   1.0 / style->gammaValue) * 254 +
				      0.5);
		      g->look_up[255] = 255;
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
		  {
		      int j;
		      double count = 0.0;
		      double sum;
		      double his[256];
		      g = malloc (sizeof (rl2BandHandling));
		      g->colorMap = NULL;
		      g->contrastEnhancement =
			  RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
		      g->minValue = band->min;
		      g->maxValue = band->max;
		      range = band->max - band->min;
		      g->scaleFactor = range / 254.0;
		      g->look_up[0] = 0;
		      for (i = 1; i < 256; i++)
			  count += *(band->histogram + i);
		      for (i = 1; i < 256; i++)
			  his[i] = *(band->histogram + i) / count;
		      for (i = 1; i < 256; i++)
			{
			    sum = 0.0;
			    for (j = 1; j <= i; j++)
				sum += his[j];
			    g->look_up[i] = (unsigned char) (254.0 * sum + 0.5);
			}
		      g->look_up[255] = 255;
		  }
	    }
      }
    if (b == NULL)
      {
	  /* using overall settings */
	  if (blue_band < stats->nBands)
	    {
		band = stats->band_stats + blue_band;
		if (style->contrastEnhancement ==
		    RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
		  {
		      b = malloc (sizeof (rl2BandHandling));
		      b->colorMap = NULL;
		      b->contrastEnhancement =
			  RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
		      compute_stretching (band, &(b->minValue),
					  &(b->maxValue), &(b->scaleFactor));
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_NONE)
		  {
		      b = malloc (sizeof (rl2BandHandling));
		      b->colorMap = NULL;
		      b->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_NONE;
		      if (band->min >= 0.0 && band->max <= 255.0)
			{
			    b->minValue = 0.0;
			    b->maxValue = 255.0;
			    b->scaleFactor = 1.0;
			}
		      else
			{
			    b->minValue = band->min;
			    b->maxValue = band->max;
			    range = band->max - band->min;
			    b->scaleFactor = range / 254.0;
			}
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_GAMMA)
		  {
		      b = malloc (sizeof (rl2BandHandling));
		      b->colorMap = NULL;
		      b->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_GAMMA;
		      b->minValue = band->min;
		      b->maxValue = band->max;
		      range = band->max - band->min;
		      b->scaleFactor = range / 254.0;
		      b->look_up[0] = 0;
		      for (i = 1; i < 255; i++)
			  b->look_up[i] =
			      (unsigned
			       char) (pow ((double) i / 254.0,
					   1.0 / style->gammaValue) * 254 +
				      0.5);
		      b->look_up[255] = 255;
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
		  {
		      int j;
		      double count = 0.0;
		      double sum;
		      double his[256];
		      b = malloc (sizeof (rl2BandHandling));
		      b->colorMap = NULL;
		      b->contrastEnhancement =
			  RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
		      b->minValue = band->min;
		      b->maxValue = band->max;
		      range = band->max - band->min;
		      b->scaleFactor = range / 254.0;
		      b->look_up[0] = 0;
		      for (i = 1; i < 256; i++)
			  count += *(band->histogram + i);
		      for (i = 1; i < 256; i++)
			  his[i] = *(band->histogram + i) / count;
		      for (i = 1; i < 256; i++)
			{
			    sum = 0.0;
			    for (j = 1; j <= i; j++)
				sum += his[j];
			    b->look_up[i] = (unsigned char) (254.0 * sum + 0.5);
			}
		      b->look_up[255] = 255;
		  }
	    }
      }
    *red_handling = r;
    *green_handling = g;
    *blue_handling = b;
}

static void
add_color_rule (rl2ColorMapItemPtr c, rl2ColorMapRefPtr col)
{
/* appending a color rule to the look-up table enty */
    rl2ColorMapRefPtr rule = malloc (sizeof (rl2ColorMapRef));
    rule->min = col->min;
    rule->max = col->max;
    rule->red = col->red;
    rule->green = col->green;
    rule->blue = col->blue;
    rule->maxRed = col->maxRed;
    rule->maxGreen = col->maxGreen;
    rule->maxBlue = col->maxBlue;
    rule->next = NULL;
    if (c->first == NULL)
	c->first = rule;
    if (c->last != NULL)
	c->last->next = rule;
    c->last = rule;
}

static void
build_mono_band_handling (rl2PrivRasterSymbolizerPtr style,
			  rl2PrivRasterStatisticsPtr stats,
			  unsigned char mono_band,
			  rl2BandHandlingPtr * mono_handling)
{
/* creating BandContrastEnhancement helper structs */
    rl2BandHandlingPtr g = NULL;
    rl2PrivBandStatisticsPtr band;
    double range;
    int i;
    rl2PrivColorMapPointPtr color;
    rl2PrivColorMapPointPtr prev_color;
    if (style->categorize != NULL)
      {
	  /* using the Categorize ColorMap */
	  band = stats->band_stats + mono_band;
	  g = malloc (sizeof (rl2BandHandling));
	  g->minValue = band->min;
	  g->maxValue = band->max;
	  range = band->max - band->min;
	  g->scaleFactor = range / 256.0;
	  g->colorMap = malloc (sizeof (rl2ColorMapLocator));
	  g->colorMap->interpolate = 0;
	  for (i = 0; i < 256; i++)
	    {
		rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
		c->first = NULL;
		c->last = NULL;
	    }
	  g->colorMap->red = style->categorize->dfltRed;
	  g->colorMap->green = style->categorize->dfltGreen;
	  g->colorMap->blue = style->categorize->dfltBlue;
	  color = style->categorize->first;
	  prev_color = NULL;
	  while (color != NULL)
	    {
		rl2ColorMapRef col;
		if (prev_color == NULL)
		  {
		      /* first category */
		      col.min = 0.0 - DBL_MAX;
		      col.max = color->value;
		      col.red = style->categorize->baseRed;
		      col.green = style->categorize->baseGreen;
		      col.blue = style->categorize->baseBlue;
		      for (i = 0; i < 256; i++)
			{
			    rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
			    double v1 =
				band->min + ((double) i * g->scaleFactor);
			    double v2 =
				band->min + ((double) (i + 1) * g->scaleFactor);
			    if ((v1 >= col.min && v1 < col.max)
				|| (v2 >= col.min && v2 < col.max)
				|| (col.min >= v1 && col.min < v2)
				|| (col.max >= v2 && col.max < v2))
				add_color_rule (c, &col);
			}
		  }
		else
		  {
		      /* any other category */
		      col.min = prev_color->value;
		      col.max = color->value;
		      col.red = prev_color->red;
		      col.green = prev_color->green;
		      col.blue = prev_color->blue;
		      for (i = 0; i < 256; i++)
			{
			    rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
			    double v1 =
				band->min + ((double) i * g->scaleFactor);
			    double v2 =
				band->min + ((double) (i + 1) * g->scaleFactor);
			    if ((v1 >= col.min && v1 < col.max)
				|| (v2 >= col.min && v2 < col.max)
				|| (col.min >= v1 && col.min < v2)
				|| (col.max >= v2 && col.max < v2))
				add_color_rule (c, &col);
			}
		  }
		if (color->next == NULL)
		  {
		      /* last category */
		      col.min = color->value;
		      col.max = DBL_MAX;
		      col.red = color->red;
		      col.green = color->green;
		      col.blue = color->blue;
		      for (i = 0; i < 256; i++)
			{
			    rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
			    double v1 =
				band->min + ((double) i * g->scaleFactor);
			    double v2 =
				band->min + ((double) (i + 1) * g->scaleFactor);
			    if ((v1 >= col.min && v1 < col.max)
				|| (v2 >= col.min && v2 < col.max)
				|| (col.min >= v1 && col.min < v2)
				|| (col.max >= v2 && col.max < v2))
				add_color_rule (c, &col);
			}
		  }
		prev_color = color;
		color = color->next;
	    }
	  *mono_handling = g;
	  return;
      }
    if (style->interpolate != NULL)
      {
	  /* using the Interpolate ColorMap */
	  band = stats->band_stats + mono_band;
	  g = malloc (sizeof (rl2BandHandling));
	  g->minValue = band->min;
	  g->maxValue = band->max;
	  range = band->max - band->min;
	  g->scaleFactor = range / 256.0;
	  g->colorMap = malloc (sizeof (rl2ColorMapLocator));
	  g->colorMap->interpolate = 1;
	  for (i = 0; i < 256; i++)
	    {
		rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
		c->first = NULL;
		c->last = NULL;
	    }
	  g->colorMap->red = style->interpolate->dfltRed;
	  g->colorMap->green = style->interpolate->dfltGreen;
	  g->colorMap->blue = style->interpolate->dfltBlue;
	  color = style->interpolate->first;
	  prev_color = NULL;
	  while (color != NULL)
	    {
		rl2ColorMapRef col;
		if (prev_color != NULL)
		  {
		      col.min = prev_color->value;
		      col.max = color->value;
		      col.red = prev_color->red;
		      col.green = prev_color->green;
		      col.blue = prev_color->blue;
		      col.maxRed = color->red;
		      col.maxGreen = color->green;
		      col.maxBlue = color->blue;
		      for (i = 0; i < 256; i++)
			{
			    rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
			    double v1 =
				band->min + ((double) i * g->scaleFactor);
			    double v2 =
				band->min + ((double) (i + 1) * g->scaleFactor);
			    if ((v1 >= col.min && v1 < col.max)
				|| (v2 >= col.min && v2 < col.max)
				|| (col.min >= v1 && col.min < v2)
				|| (col.max >= v2 && col.max < v2))
				add_color_rule (c, &col);
			}
		  }
		prev_color = color;
		color = color->next;
	    }
	  *mono_handling = g;
	  return;
      }

    if (style->bandSelection != NULL)
      {
	  /* attempting to use band specific settings */
	  if (style->bandSelection->selectionType == RL2_BAND_SELECTION_MONO)
	    {
		if (mono_band < stats->nBands)
		  {
		      band = stats->band_stats + mono_band;
		      if (style->bandSelection->grayContrast ==
			  RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
			{
			    g = malloc (sizeof (rl2BandHandling));
			    g->colorMap = NULL;
			    g->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
			    compute_stretching (band, &(g->minValue),
						&(g->maxValue),
						&(g->scaleFactor));
			}
		      else if (style->bandSelection->grayContrast ==
			       RL2_CONTRAST_ENHANCEMENT_GAMMA)
			{
			    g = malloc (sizeof (rl2BandHandling));
			    g->colorMap = NULL;
			    g->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_GAMMA;
			    g->minValue = band->min;
			    g->maxValue = band->max;
			    range = band->max - band->min;
			    g->scaleFactor = range / 254.0;
			    g->colorMap = NULL;
			    g->look_up[0] = 0;
			    for (i = 1; i < 255; i++)
				g->look_up[i] =
				    (unsigned
				     char) (pow ((double) i / 254.0,
						 1.0 /
						 style->
						 bandSelection->grayGamma) *
					    254 + 0.5);
			    g->look_up[255] = 255;
			}
		      else if (style->bandSelection->grayContrast ==
			       RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
			{
			    int j;
			    double count = 0.0;
			    double sum;
			    double his[256];
			    g = malloc (sizeof (rl2BandHandling));
			    g->colorMap = NULL;
			    g->contrastEnhancement =
				RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
			    g->minValue = band->min;
			    g->maxValue = band->max;
			    range = band->max - band->min;
			    g->scaleFactor = range / 254.0;
			    g->look_up[0] = 0;
			    for (i = 1; i < 256; i++)
				count += *(band->histogram + i);
			    for (i = 1; i < 256; i++)
				his[i] = *(band->histogram + i) / count;
			    for (i = 1; i < 256; i++)
			      {
				  sum = 0.0;
				  for (j = 1; j <= i; j++)
				      sum += his[j];
				  g->look_up[i] =
				      (unsigned char) (254.0 * sum + 0.5);
			      }
			    g->look_up[255] = 255;
			}
		  }
	    }
      }
    if (g == NULL)
      {
	  /* using overall settings */
	  if (mono_band < stats->nBands)
	    {
		band = stats->band_stats + mono_band;
		if (style->contrastEnhancement ==
		    RL2_CONTRAST_ENHANCEMENT_NORMALIZE)
		  {
		      g = malloc (sizeof (rl2BandHandling));
		      g->colorMap = NULL;
		      g->contrastEnhancement =
			  RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
		      compute_stretching (band, &(g->minValue),
					  &(g->maxValue), &(g->scaleFactor));
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_NONE)
		  {
		      g = malloc (sizeof (rl2BandHandling));
		      g->colorMap = NULL;
		      g->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_NONE;
		      if (band->min >= 0.0 && band->max <= 255.0)
			{
			    g->minValue = 0.0;
			    g->maxValue = 255.0;
			    g->scaleFactor = 1.0;
			}
		      else
			{
			    g->minValue = band->min;
			    g->maxValue = band->max;
			    range = band->max - band->min;
			    g->scaleFactor = range / 254.0;
			    g->colorMap = NULL;
			}
		      g->colorMap = NULL;
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_GAMMA)
		  {
		      g = malloc (sizeof (rl2BandHandling));
		      g->colorMap = NULL;
		      g->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_GAMMA;
		      g->minValue = band->min;
		      g->maxValue = band->max;
		      range = band->max - band->min;
		      g->scaleFactor = range / 254.0;
		      g->colorMap = NULL;
		      g->look_up[0] = 0;
		      for (i = 1; i < 255; i++)
			  g->look_up[i] =
			      (unsigned
			       char) (pow ((double) i / 254.0,
					   1.0 / style->gammaValue) * 254 +
				      0.5);
		      g->look_up[255] = 255;
		  }
		else if (style->contrastEnhancement ==
			 RL2_CONTRAST_ENHANCEMENT_HISTOGRAM)
		  {
		      int j;
		      double count = 0.0;
		      double sum;
		      double his[256];
		      g = malloc (sizeof (rl2BandHandling));
		      g->colorMap = NULL;
		      g->contrastEnhancement =
			  RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
		      g->minValue = band->min;
		      g->maxValue = band->max;
		      range = band->max - band->min;
		      g->scaleFactor = range / 254.0;
		      g->look_up[0] = 0;
		      for (i = 1; i < 256; i++)
			  count += *(band->histogram + i);
		      for (i = 1; i < 256; i++)
			  his[i] = *(band->histogram + i) / count;
		      for (i = 1; i < 256; i++)
			{
			    sum = 0.0;
			    for (j = 1; j <= i; j++)
				sum += his[j];
			    g->look_up[i] = (unsigned char) (254.0 * sum + 0.5);
			}
		      g->look_up[255] = 255;
		  }
	    }
      }
    *mono_handling = g;
}

static void
destroy_mono_handling (rl2BandHandlingPtr mono)
{
/* memory cleanup - destroying a MONO handler */
    if (mono == NULL)
	return;
    if (mono->colorMap != NULL)
      {
	  int i;
	  for (i = 0; i < 256; i++)
	    {
		rl2ColorMapRefPtr rule;
		rl2ColorMapRefPtr n_rule;
		rl2ColorMapItemPtr item = &(mono->colorMap->look_up[i]);
		rule = item->first;
		while (rule != NULL)
		  {
		      n_rule = rule->next;
		      free (rule);
		      rule = n_rule;
		  }
	    }
	  free (mono->colorMap);
      }
    free (mono);
}

static void
build_ndvi_handling (rl2PrivRasterSymbolizerPtr style,
		     rl2BandHandlingPtr * ndvi_handling)
{
/* creating NDVI helper structs */
    int i;
    rl2BandHandlingPtr g = NULL;
    rl2PrivColorMapPointPtr color;
    rl2PrivColorMapPointPtr prev_color;
    if (style->categorize != NULL)
      {
	  /* using the Categorize ColorMap */
	  g = malloc (sizeof (rl2BandHandling));
	  g->minValue = 1.0;
	  g->maxValue = 1.0;
	  g->scaleFactor = 2.0 / 256.0;
	  g->colorMap = malloc (sizeof (rl2ColorMapLocator));
	  g->colorMap->interpolate = 0;
	  for (i = 0; i < 256; i++)
	    {
		rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
		c->first = NULL;
		c->last = NULL;
	    }
	  g->colorMap->red = style->categorize->dfltRed;
	  g->colorMap->green = style->categorize->dfltGreen;
	  g->colorMap->blue = style->categorize->dfltBlue;
	  color = style->categorize->first;
	  prev_color = NULL;
	  while (color != NULL)
	    {
		rl2ColorMapRef col;
		if (prev_color == NULL)
		  {
		      /* first category */
		      col.min = 0.0 - DBL_MAX;
		      col.max = color->value;
		      col.red = style->categorize->baseRed;
		      col.green = style->categorize->baseGreen;
		      col.blue = style->categorize->baseBlue;
		      for (i = 0; i < 256; i++)
			{
			    rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
			    double v1 = -1.0 + ((double) i * g->scaleFactor);
			    double v2 =
				-1.0 + ((double) (i + 1) * g->scaleFactor);
			    if ((v1 >= col.min && v1 < col.max)
				|| (v2 >= col.min && v2 < col.max)
				|| (col.min >= v1 && col.min < v2)
				|| (col.max >= v2 && col.max < v2))
				add_color_rule (c, &col);
			}
		  }
		else
		  {
		      /* any other category */
		      col.min = prev_color->value;
		      col.max = color->value;
		      col.red = prev_color->red;
		      col.green = prev_color->green;
		      col.blue = prev_color->blue;
		      for (i = 0; i < 256; i++)
			{
			    rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
			    double v1 = -1.0 + ((double) i * g->scaleFactor);
			    double v2 =
				-1.0 + ((double) (i + 1) * g->scaleFactor);
			    if ((v1 >= col.min && v1 < col.max)
				|| (v2 >= col.min && v2 < col.max)
				|| (col.min >= v1 && col.min < v2)
				|| (col.max >= v2 && col.max < v2))
				add_color_rule (c, &col);
			}
		  }
		if (color->next == NULL)
		  {
		      /* last category */
		      col.min = color->value;
		      col.max = DBL_MAX;
		      col.red = color->red;
		      col.green = color->green;
		      col.blue = color->blue;
		      for (i = 0; i < 256; i++)
			{
			    rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
			    double v1 = -1.0 + ((double) i * g->scaleFactor);
			    double v2 =
				-1.0 + ((double) (i + 1) * g->scaleFactor);
			    if ((v1 >= col.min && v1 < col.max)
				|| (v2 >= col.min && v2 < col.max)
				|| (col.min >= v1 && col.min < v2)
				|| (col.max >= v2 && col.max < v2))
				add_color_rule (c, &col);
			}
		  }
		prev_color = color;
		color = color->next;
	    }
	  *ndvi_handling = g;
	  return;
      }
    if (style->interpolate != NULL)
      {
	  /* using the Interpolate ColorMap */
	  g = malloc (sizeof (rl2BandHandling));
	  g->minValue = -1.0;
	  g->maxValue = 1.0;
	  g->scaleFactor = 2.0 / 256.0;
	  g->colorMap = malloc (sizeof (rl2ColorMapLocator));
	  g->colorMap->interpolate = 1;
	  for (i = 0; i < 256; i++)
	    {
		rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
		c->first = NULL;
		c->last = NULL;
	    }
	  g->colorMap->red = style->interpolate->dfltRed;
	  g->colorMap->green = style->interpolate->dfltGreen;
	  g->colorMap->blue = style->interpolate->dfltBlue;
	  color = style->interpolate->first;
	  prev_color = NULL;
	  while (color != NULL)
	    {
		rl2ColorMapRef col;
		if (prev_color != NULL)
		  {
		      col.min = prev_color->value;
		      col.max = color->value;
		      col.red = prev_color->red;
		      col.green = prev_color->green;
		      col.blue = prev_color->blue;
		      col.maxRed = color->red;
		      col.maxGreen = color->green;
		      col.maxBlue = color->blue;
		      for (i = 0; i < 256; i++)
			{
			    rl2ColorMapItemPtr c = &(g->colorMap->look_up[i]);
			    double v1 = -1.0 + ((double) i * g->scaleFactor);
			    double v2 =
				-1.0 + ((double) (i + 1) * g->scaleFactor);
			    if ((v1 >= col.min && v1 < col.max)
				|| (v2 >= col.min && v2 < col.max)
				|| (col.min >= v1 && col.min < v2)
				|| (col.max >= v2 && col.max < v2))
				add_color_rule (c, &col);
			}
		  }
		prev_color = color;
		color = color->next;
	    }
	  *ndvi_handling = g;
	  return;
      }
    *ndvi_handling = g;
}

static void
destroy_ndvi_handling (rl2BandHandlingPtr ndvi)
{
/* memory cleanup - destroying an NDVI handler */
    if (ndvi == NULL)
	return;
    if (ndvi->colorMap != NULL)
      {
	  int i;
	  for (i = 0; i < 256; i++)
	    {
		rl2ColorMapRefPtr rule;
		rl2ColorMapRefPtr n_rule;
		rl2ColorMapItemPtr item = &(ndvi->colorMap->look_up[i]);
		rule = item->first;
		while (rule != NULL)
		  {
		      n_rule = rule->next;
		      free (rule);
		      rule = n_rule;
		  }
	    }
	  free (ndvi->colorMap);
      }
    free (ndvi);
}

static int
do_copy_raw_selected_pixels (rl2PrivRasterPtr rst, unsigned char *outbuf,
			     unsigned int width, unsigned int height,
			     double x_res, double y_res, double minx,
			     double maxy, double tile_minx, double tile_maxy,
			     unsigned int tile_width,
			     unsigned int tile_height, rl2PixelPtr no_data,
			     unsigned char red_band, unsigned char green_band,
			     unsigned char blue_band,
			     rl2BandHandlingPtr red_handling,
			     rl2BandHandlingPtr green_handling,
			     rl2BandHandlingPtr blue_handling)
{
    switch (rst->sampleType)
      {
      case RL2_SAMPLE_UINT8:
	  copy_uint8_raw_selected_pixels ((const unsigned char
					   *)
					  (rst->rasterBuffer),
					  (const unsigned char
					   *)
					  (rst->maskBuffer),
					  (unsigned char *)
					  outbuf, width,
					  height, rst->nBands,
					  x_res, y_res, minx,
					  maxy, tile_minx,
					  tile_maxy,
					  tile_width,
					  tile_height,
					  no_data, red_band,
					  green_band,
					  blue_band,
					  red_handling,
					  green_handling, blue_handling);
	  if (red_handling != NULL)
	      free (red_handling);
	  if (green_handling != NULL)
	      free (green_handling);
	  if (blue_handling != NULL)
	      free (blue_handling);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_raw_selected_pixels ((const unsigned
					    short
					    *)
					   (rst->rasterBuffer),
					   (const unsigned
					    char
					    *)
					   (rst->maskBuffer),
					   (unsigned char *)
					   outbuf, width,
					   height,
					   rst->nBands, x_res,
					   y_res, minx, maxy,
					   tile_minx,
					   tile_maxy,
					   tile_width,
					   tile_height,
					   no_data, red_band,
					   green_band,
					   blue_band,
					   red_handling,
					   green_handling, blue_handling);
	  if (red_handling != NULL)
	      free (red_handling);
	  if (green_handling != NULL)
	      free (green_handling);
	  if (blue_handling != NULL)
	      free (blue_handling);
	  return 1;
      };
    return 0;
}

static int
do_copy_raw_selected_pixels_transparent (rl2PrivRasterPtr rst,
					 unsigned char *outbuf,
					 unsigned char *mask,
					 unsigned int width,
					 unsigned int height, double x_res,
					 double y_res, double minx, double maxy,
					 double tile_minx, double tile_maxy,
					 unsigned int tile_width,
					 unsigned int tile_height,
					 rl2PixelPtr no_data,
					 unsigned char red_band,
					 unsigned char green_band,
					 unsigned char blue_band,
					 rl2BandHandlingPtr red_handling,
					 rl2BandHandlingPtr green_handling,
					 rl2BandHandlingPtr blue_handling)
{
    switch (rst->sampleType)
      {
      case RL2_SAMPLE_UINT8:
	  copy_uint8_raw_selected_pixels_transparent ((const unsigned char
						       *)
						      (rst->rasterBuffer),
						      (const unsigned char
						       *)
						      (rst->maskBuffer),
						      (unsigned char *)
						      outbuf, mask, width,
						      height, rst->nBands,
						      x_res, y_res, minx,
						      maxy, tile_minx,
						      tile_maxy,
						      tile_width,
						      tile_height,
						      no_data, red_band,
						      green_band,
						      blue_band,
						      red_handling,
						      green_handling,
						      blue_handling);
	  if (red_handling != NULL)
	      free (red_handling);
	  if (green_handling != NULL)
	      free (green_handling);
	  if (blue_handling != NULL)
	      free (blue_handling);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_raw_selected_pixels_transparent ((const unsigned
							short
							*)
						       (rst->rasterBuffer),
						       (const unsigned
							char
							*)
						       (rst->maskBuffer),
						       (unsigned char *)
						       outbuf, mask, width,
						       height,
						       rst->nBands, x_res,
						       y_res, minx, maxy,
						       tile_minx,
						       tile_maxy,
						       tile_width,
						       tile_height,
						       no_data, red_band,
						       green_band,
						       blue_band,
						       red_handling,
						       green_handling,
						       blue_handling);
	  if (red_handling != NULL)
	      free (red_handling);
	  if (green_handling != NULL)
	      free (green_handling);
	  if (blue_handling != NULL)
	      free (blue_handling);
	  return 1;
      };
    return 0;
}

static int
do_copy_raw_mono_pixels (rl2PrivRasterPtr rst, unsigned char *outbuf,
			 unsigned int width, unsigned int height,
			 unsigned char num_bands, double x_res, double y_res,
			 double minx, double maxy, double tile_minx,
			 double tile_maxy, unsigned int tile_width,
			 unsigned int tile_height, rl2PixelPtr no_data,
			 unsigned char mono_band,
			 rl2BandHandlingPtr mono_handling)
{
    switch (rst->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  copy_int8_raw_mono_pixels ((const char
				      *) (rst->rasterBuffer),
				     (const unsigned char
				      *) (rst->maskBuffer),
				     (unsigned char *) outbuf,
				     width, height, num_bands,
				     x_res, y_res, minx, maxy,
				     tile_minx, tile_maxy,
				     tile_width, tile_height,
				     no_data, mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_UINT8:
	  copy_uint8_raw_mono_pixels ((const unsigned char
				       *) (rst->rasterBuffer),
				      (const unsigned char
				       *) (rst->maskBuffer),
				      (unsigned char *)
				      outbuf, width, height,
				      num_bands, rst->nBands,
				      x_res, y_res, minx,
				      maxy, tile_minx,
				      tile_maxy, tile_width,
				      tile_height, no_data,
				      mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_INT16:
	  copy_int16_raw_mono_pixels ((const short
				       *) (rst->rasterBuffer),
				      (const unsigned char
				       *) (rst->maskBuffer),
				      (unsigned char *)
				      outbuf, width, height,
				      num_bands, x_res, y_res,
				      minx, maxy, tile_minx,
				      tile_maxy, tile_width,
				      tile_height, no_data,
				      mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_raw_mono_pixels ((const unsigned short
					*)
				       (rst->rasterBuffer),
				       (const unsigned char
					*) (rst->maskBuffer),
				       (unsigned char *)
				       outbuf, width, height,
				       num_bands, rst->nBands,
				       x_res, y_res, minx,
				       maxy, tile_minx,
				       tile_maxy, tile_width,
				       tile_height, no_data,
				       mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_INT32:
	  copy_int32_raw_mono_pixels ((const int
				       *) (rst->rasterBuffer),
				      (const unsigned char
				       *) (rst->maskBuffer),
				      (unsigned char *)
				      outbuf, width, height,
				      num_bands, x_res, y_res,
				      minx, maxy, tile_minx,
				      tile_maxy, tile_width,
				      tile_height, no_data,
				      mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_UINT32:
	  copy_uint32_raw_mono_pixels ((const unsigned int
					*)
				       (rst->rasterBuffer),
				       (const unsigned char
					*) (rst->maskBuffer),
				       (unsigned char *)
				       outbuf, width, height,
				       num_bands, x_res,
				       y_res, minx, maxy,
				       tile_minx, tile_maxy,
				       tile_width,
				       tile_height, no_data,
				       mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_FLOAT:
	  copy_float_raw_mono_pixels ((const float
				       *) (rst->rasterBuffer),
				      (const unsigned char
				       *) (rst->maskBuffer),
				      (unsigned char *)
				      outbuf, width, height,
				      num_bands, x_res, y_res,
				      minx, maxy, tile_minx,
				      tile_maxy, tile_width,
				      tile_height, no_data,
				      mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_DOUBLE:
	  copy_double_raw_mono_pixels ((const double
					*)
				       (rst->rasterBuffer),
				       (const unsigned char
					*) (rst->maskBuffer),
				       (unsigned char *)
				       outbuf, width, height,
				       num_bands, x_res,
				       y_res, minx, maxy,
				       tile_minx, tile_maxy,
				       tile_width,
				       tile_height, no_data,
				       mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      };
    return 0;
}

static int
do_copy_raw_mono_pixels_transparent (rl2PrivRasterPtr rst,
				     unsigned char *outbuf,
				     unsigned char *outmask, unsigned int width,
				     unsigned int height,
				     unsigned char num_bands, double x_res,
				     double y_res, double minx, double maxy,
				     double tile_minx, double tile_maxy,
				     unsigned int tile_width,
				     unsigned int tile_height,
				     rl2PixelPtr no_data,
				     unsigned char mono_band,
				     rl2BandHandlingPtr mono_handling)
{
    switch (rst->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  copy_int8_raw_mono_pixels_transparent ((const char
						  *) (rst->rasterBuffer),
						 (const unsigned char
						  *) (rst->maskBuffer),
						 outbuf, outmask,
						 width, height, num_bands,
						 x_res, y_res, minx, maxy,
						 tile_minx, tile_maxy,
						 tile_width, tile_height,
						 no_data, mono_band,
						 mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_UINT8:
	  copy_uint8_raw_mono_pixels_transparent ((const unsigned char
						   *) (rst->rasterBuffer),
						  (const unsigned char
						   *) (rst->maskBuffer),
						  outbuf, outmask, width,
						  height, num_bands,
						  rst->nBands, x_res, y_res,
						  minx, maxy, tile_minx,
						  tile_maxy, tile_width,
						  tile_height, no_data,
						  mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_INT16:
	  copy_int16_raw_mono_pixels_transparent ((const short
						   *) (rst->rasterBuffer),
						  (const unsigned char
						   *) (rst->maskBuffer),
						  outbuf, outmask, width,
						  height, num_bands, x_res,
						  y_res, minx, maxy, tile_minx,
						  tile_maxy, tile_width,
						  tile_height, no_data,
						  mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_raw_mono_pixels_transparent ((const unsigned short
						    *)
						   (rst->rasterBuffer),
						   (const unsigned char
						    *) (rst->maskBuffer),
						   outbuf, outmask, width,
						   height, num_bands,
						   rst->nBands, x_res, y_res,
						   minx, maxy, tile_minx,
						   tile_maxy, tile_width,
						   tile_height, no_data,
						   mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_INT32:
	  copy_int32_raw_mono_pixels_transparent ((const int
						   *) (rst->rasterBuffer),
						  (const unsigned char
						   *) (rst->maskBuffer),
						  outbuf, outmask, width,
						  height, num_bands, x_res,
						  y_res, minx, maxy, tile_minx,
						  tile_maxy, tile_width,
						  tile_height, no_data,
						  mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_UINT32:
	  copy_uint32_raw_mono_pixels_transparent ((const unsigned int
						    *)
						   (rst->rasterBuffer),
						   (const unsigned char
						    *) (rst->maskBuffer),
						   outbuf, outmask, width,
						   height, num_bands, x_res,
						   y_res, minx, maxy, tile_minx,
						   tile_maxy, tile_width,
						   tile_height, no_data,
						   mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_FLOAT:
	  copy_float_raw_mono_pixels_transparent ((const float
						   *) (rst->rasterBuffer),
						  (const unsigned char
						   *) (rst->maskBuffer),
						  outbuf, outmask, width,
						  height, num_bands, x_res,
						  y_res, minx, maxy, tile_minx,
						  tile_maxy, tile_width,
						  tile_height, no_data,
						  mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      case RL2_SAMPLE_DOUBLE:
	  copy_double_raw_mono_pixels_transparent ((const double
						    *)
						   (rst->rasterBuffer),
						   (const unsigned char
						    *) (rst->maskBuffer),
						   outbuf, outmask, width,
						   height, num_bands, x_res,
						   y_res, minx, maxy, tile_minx,
						   tile_maxy, tile_width,
						   tile_height, no_data,
						   mono_band, mono_handling);
	  if (mono_handling != NULL)
	      destroy_mono_handling (mono_handling);
	  return 1;
      };
    return 0;
}

static int
do_auto_ndvi_pixels (rl2PrivRasterPtr rst, unsigned char *outbuf,
		     unsigned int width, unsigned int height,
		     unsigned char num_bands, double x_res, double y_res,
		     double minx, double maxy, double tile_minx,
		     double tile_maxy, unsigned int tile_width,
		     unsigned int tile_height, rl2PixelPtr no_data,
		     unsigned char red_band, unsigned char nir_band,
		     rl2BandHandlingPtr ndvi_handling)
{
    switch (rst->sampleType)
      {
      case RL2_SAMPLE_UINT8:
	  copy_uint8_ndvi_pixels ((const unsigned char
				   *) (rst->rasterBuffer),
				  (const unsigned char
				   *) (rst->maskBuffer),
				  (unsigned char *)
				  outbuf, width, height,
				  num_bands, rst->nBands,
				  x_res, y_res, minx,
				  maxy, tile_minx,
				  tile_maxy, tile_width,
				  tile_height, no_data,
				  red_band, nir_band, ndvi_handling);
	  if (ndvi_handling != NULL)
	      destroy_ndvi_handling (ndvi_handling);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_ndvi_pixels ((const unsigned short
				    *)
				   (rst->rasterBuffer),
				   (const unsigned char
				    *) (rst->maskBuffer),
				   (unsigned char *)
				   outbuf, width, height,
				   num_bands, rst->nBands,
				   x_res, y_res, minx,
				   maxy, tile_minx,
				   tile_maxy, tile_width,
				   tile_height, no_data,
				   red_band, nir_band, ndvi_handling);
	  if (ndvi_handling != NULL)
	      destroy_ndvi_handling (ndvi_handling);
	  return 1;
      };
    return 0;
}

static int
do_auto_ndvi_pixels_transparent (rl2PrivRasterPtr rst, unsigned char *outbuf,
				 unsigned char *outmask, unsigned int width,
				 unsigned int height, unsigned char num_bands,
				 double x_res, double y_res, double minx,
				 double maxy, double tile_minx,
				 double tile_maxy, unsigned int tile_width,
				 unsigned int tile_height, rl2PixelPtr no_data,
				 unsigned char red_band, unsigned char nir_band,
				 rl2BandHandlingPtr ndvi_handling)
{
    switch (rst->sampleType)
      {
      case RL2_SAMPLE_UINT8:
	  copy_uint8_ndvi_pixels_transparent ((const unsigned char
					       *) (rst->rasterBuffer),
					      (const unsigned char
					       *) (rst->maskBuffer),
					      (unsigned char *)
					      outbuf, outmask, width, height,
					      num_bands, rst->nBands,
					      x_res, y_res, minx,
					      maxy, tile_minx,
					      tile_maxy, tile_width,
					      tile_height, no_data,
					      red_band, nir_band,
					      ndvi_handling);
	  if (ndvi_handling != NULL)
	      destroy_ndvi_handling (ndvi_handling);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_ndvi_pixels_transparent ((const unsigned short
						*)
					       (rst->rasterBuffer),
					       (const unsigned char
						*) (rst->maskBuffer),
					       (unsigned char *)
					       outbuf, outmask, width, height,
					       num_bands, rst->nBands,
					       x_res, y_res, minx,
					       maxy, tile_minx,
					       tile_maxy, tile_width,
					       tile_height, no_data,
					       red_band, nir_band,
					       ndvi_handling);
	  if (ndvi_handling != NULL)
	      destroy_ndvi_handling (ndvi_handling);
	  return 1;
      };
    return 0;
}

static int
do_copy_raw_pixels (rl2PrivRasterPtr rst, unsigned char *outbuf,
		    unsigned int width, unsigned int height,
		    unsigned char raster_type, unsigned char sample_type,
		    unsigned char num_bands, double x_res, double y_res,
		    double minx, double maxy, double tile_minx,
		    double tile_maxy, unsigned int tile_width,
		    unsigned int tile_height, rl2PixelPtr no_data)
{
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  copy_int8_raw_pixels ((const char *) (rst->rasterBuffer),
				(const unsigned char *) (rst->maskBuffer),
				(char *) outbuf, width, height,
				x_res, y_res, minx, maxy, tile_minx,
				tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_INT16:
	  copy_int16_raw_pixels ((const short *) (rst->rasterBuffer),
				 (const unsigned char *) (rst->maskBuffer),
				 (short *) outbuf, width, height,
				 x_res, y_res, minx, maxy, tile_minx,
				 tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_raw_pixels ((const unsigned short
				   *) (rst->rasterBuffer),
				  (const unsigned char *) (rst->maskBuffer),
				  (unsigned short *) outbuf, width, height,
				  num_bands, x_res, y_res, minx, maxy,
				  tile_minx, tile_maxy, tile_width,
				  tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_INT32:
	  copy_int32_raw_pixels ((const int *) (rst->rasterBuffer),
				 (const unsigned char *) (rst->maskBuffer),
				 (int *) outbuf, width, height,
				 x_res, y_res, minx, maxy, tile_minx,
				 tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_UINT32:
	  copy_uint32_raw_pixels ((const unsigned int *) (rst->rasterBuffer),
				  (const unsigned char *) (rst->maskBuffer),
				  (unsigned int *) outbuf, width, height,
				  x_res, y_res, minx, maxy,
				  tile_minx, tile_maxy, tile_width,
				  tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_FLOAT:
	  copy_float_raw_pixels ((const float *) (rst->rasterBuffer),
				 (const unsigned char *) (rst->maskBuffer),
				 (float *) outbuf, width, height,
				 x_res, y_res, minx, maxy, tile_minx,
				 tile_maxy, tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_DOUBLE:
	  copy_double_raw_pixels ((const double *) (rst->rasterBuffer),
				  (const unsigned char *) (rst->maskBuffer),
				  (double *) outbuf, width, height,
				  x_res, y_res, minx, maxy,
				  tile_minx, tile_maxy, tile_width,
				  tile_height, no_data);
	  return 1;
      default:
	  copy_uint8_raw_pixels ((const unsigned char *) (rst->rasterBuffer),
				 (const unsigned char *) (rst->maskBuffer),
				 (unsigned char *) outbuf, width, height,
				 num_bands, x_res, y_res, minx, maxy,
				 tile_minx, tile_maxy, tile_width,
				 tile_height, no_data, raster_type);
	  return 1;
      };
    return 0;
}

static int
do_copy_raw_pixels_transparent (rl2PrivRasterPtr rst, unsigned char *outbuf,
				unsigned char *outmask, unsigned int width,
				unsigned int height, unsigned char sample_type,
				unsigned char num_bands, double x_res,
				double y_res, double minx, double maxy,
				double tile_minx, double tile_maxy,
				unsigned int tile_width,
				unsigned int tile_height, rl2PixelPtr no_data)
{
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  copy_int8_raw_pixels_transparent ((const char *) (rst->rasterBuffer),
					    (const unsigned char
					     *) (rst->maskBuffer),
					    (char *) outbuf, outmask, width,
					    height, x_res, y_res, minx, maxy,
					    tile_minx, tile_maxy, tile_width,
					    tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_INT16:
	  copy_int16_raw_pixels_transparent ((const short
					      *) (rst->rasterBuffer),
					     (const unsigned char
					      *) (rst->maskBuffer),
					     (short *) outbuf, outmask, width,
					     height, x_res, y_res, minx, maxy,
					     tile_minx, tile_maxy, tile_width,
					     tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_UINT16:
	  copy_uint16_raw_pixels_transparent ((const unsigned short
					       *) (rst->rasterBuffer),
					      (const unsigned char
					       *) (rst->maskBuffer),
					      (unsigned short *) outbuf,
					      outmask, width, height, num_bands,
					      x_res, y_res, minx, maxy,
					      tile_minx, tile_maxy, tile_width,
					      tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_INT32:
	  copy_int32_raw_pixels_transparent ((const int *) (rst->rasterBuffer),
					     (const unsigned char
					      *) (rst->maskBuffer),
					     (int *) outbuf, outmask, width,
					     height, x_res, y_res, minx, maxy,
					     tile_minx, tile_maxy, tile_width,
					     tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_UINT32:
	  copy_uint32_raw_pixels_transparent ((const unsigned int
					       *) (rst->rasterBuffer),
					      (const unsigned char
					       *) (rst->maskBuffer),
					      (unsigned int *) outbuf, outmask,
					      width, height, x_res, y_res, minx,
					      maxy, tile_minx, tile_maxy,
					      tile_width, tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_FLOAT:
	  copy_float_raw_pixels_transparent ((const float
					      *) (rst->rasterBuffer),
					     (const unsigned char
					      *) (rst->maskBuffer),
					     (float *) outbuf, outmask, width,
					     height, x_res, y_res, minx, maxy,
					     tile_minx, tile_maxy, tile_width,
					     tile_height, no_data);
	  return 1;
      case RL2_SAMPLE_DOUBLE:
	  copy_double_raw_pixels_transparent ((const double
					       *) (rst->rasterBuffer),
					      (const unsigned char
					       *) (rst->maskBuffer),
					      (double *) outbuf, outmask, width,
					      height, x_res, y_res, minx, maxy,
					      tile_minx, tile_maxy, tile_width,
					      tile_height, no_data);
	  return 1;
      default:
	  copy_uint8_raw_pixels_transparent ((const unsigned char
					      *) (rst->rasterBuffer),
					     (const unsigned char
					      *) (rst->maskBuffer),
					     (unsigned char *) outbuf, outmask,
					     width, height, num_bands, x_res,
					     y_res, minx, maxy, tile_minx,
					     tile_maxy, tile_width, tile_height,
					     no_data);
	  return 1;
      };
    return 0;
}

RL2_PRIVATE int
rl2_copy_raw_pixels (rl2RasterPtr raster, unsigned char *outbuf,
		     unsigned int width,
		     unsigned int height, unsigned char sample_type,
		     unsigned char num_bands, unsigned char auto_ndvi,
		     unsigned char red_band_index,
		     unsigned char nir_band_index, double x_res, double y_res,
		     double minx, double maxy, double tile_minx,
		     double tile_maxy, rl2PixelPtr no_data,
		     rl2RasterSymbolizerPtr style, rl2RasterStatisticsPtr stats)
{
/* copying raw pixels into the output buffer */
    unsigned int tile_width;
    unsigned int tile_height;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;

    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;
    if (style != NULL && stats != NULL)
      {
	  /* attempting to apply a RasterSymbolizer */
	  int yes_no;
	  int categorize;
	  int interpolate;
	  if (rl2_is_raster_symbolizer_triple_band_selected (style, &yes_no)
	      == RL2_OK)
	    {
		if ((rst->sampleType == RL2_SAMPLE_UINT8
		     || rst->sampleType == RL2_SAMPLE_UINT16)
		    && (rst->pixelType == RL2_PIXEL_RGB
			|| rst->pixelType == RL2_PIXEL_MULTIBAND) && yes_no)
		  {
		      /* triple band selection - false color RGB */
		      unsigned char red_band;
		      unsigned char green_band;
		      unsigned char blue_band;
		      rl2BandHandlingPtr red_handling = NULL;
		      rl2BandHandlingPtr green_handling = NULL;
		      rl2BandHandlingPtr blue_handling = NULL;
		      if (rl2_get_raster_symbolizer_triple_band_selection
			  (style, &red_band, &green_band, &blue_band) != RL2_OK)
			  return 0;
		      if (red_band >= rst->nBands)
			  return 0;
		      if (green_band >= rst->nBands)
			  return 0;
		      if (blue_band >= rst->nBands)
			  return 0;
		      build_triple_band_handling ((rl2PrivRasterSymbolizerPtr)
						  style,
						  (rl2PrivRasterStatisticsPtr)
						  stats, red_band, green_band,
						  blue_band, &red_handling,
						  &green_handling,
						  &blue_handling);
		      if (red_handling == NULL || green_handling == NULL
			  || blue_handling == NULL)
			  return 0;
		      if (do_copy_raw_selected_pixels
			  (rst, outbuf, width, height, x_res, y_res, minx,
			   maxy, tile_minx, tile_maxy, tile_width,
			   tile_height, no_data, red_band, green_band,
			   blue_band, red_handling, green_handling,
			   blue_handling))
			  return 1;
		      if (red_handling != NULL)
			  free (red_handling);
		      if (green_handling != NULL)
			  free (green_handling);
		      if (blue_handling != NULL)
			  free (blue_handling);

		  }
	    }
	  if (rl2_is_raster_symbolizer_mono_band_selected
	      (style, &yes_no, &categorize, &interpolate) == RL2_OK)
	    {
		if ((rst->sampleType == RL2_SAMPLE_UINT8
		     || rst->sampleType == RL2_SAMPLE_UINT16)
		    && rst->pixelType == RL2_PIXEL_MULTIBAND && auto_ndvi
		    && (categorize || interpolate))
		  {
		      /* applying Auto NDVI */
		      rl2BandHandlingPtr ndvi_handling = NULL;
		      build_ndvi_handling ((rl2PrivRasterSymbolizerPtr)
					   style, &ndvi_handling);
		      if (ndvi_handling == NULL)
			  return 0;
		      if (do_auto_ndvi_pixels
			  (rst, outbuf, width, height, num_bands, x_res,
			   y_res, minx, maxy, tile_minx, tile_maxy,
			   tile_width, tile_height, no_data, red_band_index,
			   nir_band_index, ndvi_handling))
			  return 1;
		      if (ndvi_handling != NULL)
			  destroy_ndvi_handling (ndvi_handling);
		  }
		if (((rst->sampleType == RL2_SAMPLE_UINT8
		      || rst->sampleType == RL2_SAMPLE_UINT16)
		     || rst->pixelType == RL2_PIXEL_DATAGRID) && yes_no)
		  {
		      /* mono band selection - false color Grayscale */
		      unsigned char mono_band;
		      rl2BandHandlingPtr mono_handling = NULL;
		      if (rl2_get_raster_symbolizer_mono_band_selection
			  (style, &mono_band) != RL2_OK)
			  return 0;
		      if (mono_band >= rst->nBands)
			  return 0;
		      build_mono_band_handling ((rl2PrivRasterSymbolizerPtr)
						style,
						(rl2PrivRasterStatisticsPtr)
						stats, mono_band,
						&mono_handling);
		      if (mono_handling == NULL)
			  return 0;
		      if (do_copy_raw_mono_pixels
			  (rst, outbuf, width, height, num_bands, x_res,
			   y_res, minx, maxy, tile_minx, tile_maxy,
			   tile_width, tile_height, no_data, mono_band,
			   mono_handling))
			  return 1;
		      if (mono_handling != NULL)
			  destroy_mono_handling (mono_handling);
		  }
	    }
      }

    if (do_copy_raw_pixels
	(rst, outbuf, width, height, rst->sampleType, sample_type, num_bands,
	 x_res, y_res, minx, maxy, tile_minx, tile_maxy, tile_width,
	 tile_height, no_data))
	return 1;

    return 0;
}

RL2_PRIVATE int
rl2_copy_raw_pixels_transparent (rl2RasterPtr raster, unsigned char *outbuf,
				 unsigned char *outmask, unsigned int width,
				 unsigned int height, unsigned char sample_type,
				 unsigned char num_bands,
				 unsigned char auto_ndvi,
				 unsigned char red_band_index,
				 unsigned char nir_band_index, double x_res,
				 double y_res, double minx, double maxy,
				 double tile_minx, double tile_maxy,
				 rl2PixelPtr no_data,
				 rl2RasterSymbolizerPtr style,
				 rl2RasterStatisticsPtr stats)
{
/* copying raw pixels into the output buffer */
    unsigned int tile_width;
    unsigned int tile_height;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;
    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;

    if (rst->sampleType == RL2_SAMPLE_1_BIT
	&& rst->pixelType == RL2_PIXEL_MONOCHROME)
      {
	  /* transforming a Monochrome raster into Grayscale */
	  unsigned int x;
	  unsigned int y;
	  unsigned char *p = rst->rasterBuffer;
	  unsigned char *msk;
	  if (rst->maskBuffer == NULL)
	      rst->maskBuffer = malloc (rst->width * rst->height);
	  msk = rst->maskBuffer;
	  for (y = 0; y < rst->height; y++)
	    {
		for (x = 0; x < rst->width; x++)
		  {
		      if (*p == 1)
			{
			    *p++ = 255;	/* white */
			    *msk++ = 1;	/* always transparent */
			}
		      else
			{
			    *p++ = 0;	/* black */
			    *msk++ = 0;	/* always opaque */
			}
		  }
	    }
	  rst->sampleType = RL2_SAMPLE_UINT8;
	  rst->pixelType = RL2_PIXEL_GRAYSCALE;
      }

    if (style != NULL && stats != NULL)
      {
	  /* attempting to apply a RasterSymbolizer */
	  int yes_no;
	  int categorize;
	  int interpolate;
	  if (rl2_is_raster_symbolizer_triple_band_selected (style, &yes_no)
	      == RL2_OK)
	    {
		if ((rst->sampleType == RL2_SAMPLE_UINT8
		     || rst->sampleType == RL2_SAMPLE_UINT16)
		    && (rst->pixelType == RL2_PIXEL_RGB
			|| rst->pixelType == RL2_PIXEL_MULTIBAND) && yes_no)
		  {
		      /* triple band selection - false color RGB */
		      unsigned char red_band;
		      unsigned char green_band;
		      unsigned char blue_band;
		      rl2BandHandlingPtr red_handling = NULL;
		      rl2BandHandlingPtr green_handling = NULL;
		      rl2BandHandlingPtr blue_handling = NULL;
		      if (rl2_get_raster_symbolizer_triple_band_selection
			  (style, &red_band, &green_band, &blue_band) != RL2_OK)
			  return 0;
		      if (red_band >= rst->nBands)
			  return 0;
		      if (green_band >= rst->nBands)
			  return 0;
		      if (blue_band >= rst->nBands)
			  return 0;
		      build_triple_band_handling ((rl2PrivRasterSymbolizerPtr)
						  style,
						  (rl2PrivRasterStatisticsPtr)
						  stats, red_band, green_band,
						  blue_band, &red_handling,
						  &green_handling,
						  &blue_handling);
		      if (red_handling == NULL || green_handling == NULL
			  || blue_handling == NULL)
			  return 0;
		      if (do_copy_raw_selected_pixels_transparent
			  (rst, outbuf, outmask, width, height, x_res, y_res,
			   minx, maxy, tile_minx, tile_maxy, tile_width,
			   tile_height, no_data, red_band, green_band,
			   blue_band, red_handling, green_handling,
			   blue_handling))
			  return 1;
		      if (red_handling != NULL)
			  free (red_handling);
		      if (green_handling != NULL)
			  free (green_handling);
		      if (blue_handling != NULL)
			  free (blue_handling);

		  }
	    }
	  if (rl2_is_raster_symbolizer_mono_band_selected
	      (style, &yes_no, &categorize, &interpolate) == RL2_OK)
	    {
		if ((rst->sampleType == RL2_SAMPLE_UINT8
		     || rst->sampleType == RL2_SAMPLE_UINT16)
		    && rst->pixelType == RL2_PIXEL_MULTIBAND && auto_ndvi
		    && (categorize || interpolate))
		  {
		      /* applying Auto NDVI */
		      rl2BandHandlingPtr ndvi_handling = NULL;
		      build_ndvi_handling ((rl2PrivRasterSymbolizerPtr)
					   style, &ndvi_handling);
		      if (ndvi_handling == NULL)
			  return 0;
		      if (do_auto_ndvi_pixels_transparent
			  (rst, outbuf, outmask, width, height, num_bands,
			   x_res, y_res, minx, maxy, tile_minx, tile_maxy,
			   tile_width, tile_height, no_data, red_band_index,
			   nir_band_index, ndvi_handling))
			  return 1;
		      if (ndvi_handling != NULL)
			  destroy_ndvi_handling (ndvi_handling);
		  }
		if (((rst->sampleType == RL2_SAMPLE_UINT8
		      || rst->sampleType == RL2_SAMPLE_UINT16)
		     || rst->pixelType == RL2_PIXEL_DATAGRID) && yes_no)
		  {
		      /* mono band selection - false color Grayscale */
		      unsigned char mono_band;
		      rl2BandHandlingPtr mono_handling = NULL;
		      if (rl2_get_raster_symbolizer_mono_band_selection
			  (style, &mono_band) != RL2_OK)
			  return 0;
		      if (mono_band >= rst->nBands)
			  return 0;
		      build_mono_band_handling ((rl2PrivRasterSymbolizerPtr)
						style,
						(rl2PrivRasterStatisticsPtr)
						stats, mono_band,
						&mono_handling);
		      if (mono_handling == NULL)
			  return 0;
		      if (do_copy_raw_mono_pixels_transparent
			  (rst, outbuf, outmask, width, height, num_bands,
			   x_res, y_res, minx, maxy, tile_minx, tile_maxy,
			   tile_width, tile_height, no_data, mono_band,
			   mono_handling))
			  return 1;
		      if (mono_handling != NULL)
			  destroy_mono_handling (mono_handling);
		  }
	    }
      }

    if (do_copy_raw_pixels_transparent
	(rst, outbuf, outmask, width, height, sample_type,
	 num_bands, x_res, y_res, minx, maxy, tile_minx, tile_maxy, tile_width,
	 tile_height, no_data))
	return 1;

    return 0;
}

static void
do_copy_raw_mask (rl2PrivRasterPtr rst, unsigned char *maskbuf,
		  unsigned int width, unsigned int height, double x_res,
		  double y_res, double minx, double maxy, double tile_minx,
		  double tile_maxy, unsigned int tile_width,
		  unsigned int tile_height)
{
/* copying mask pixels from the DBMS tile into the output mask */
    unsigned int x;
    unsigned int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in = rst->maskBuffer;
    unsigned char *p_out;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || out_y >= (int) height)
	    {
		p_in += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || out_x >= (int) width)
		  {
		      p_in++;
		      continue;
		  }
		p_out = maskbuf + (out_y * width) + out_x;
		if (*p_in++ == 0)
		    *p_out++ = 255;
	    }
      }
}

RL2_PRIVATE int
rl2_copy_raw_mask (rl2RasterPtr raster, unsigned char *maskbuf,
		   unsigned int width,
		   unsigned int height, double x_res, double y_res,
		   double minx, double maxy, double tile_minx, double tile_maxy)
{
/* copying a raw transparency mask into the output buffer */
    unsigned int tile_width;
    unsigned int tile_height;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;

    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;

    if (rst->maskBuffer == NULL)
      {
	  /* tile without mask - ok */
	  return 1;
      }
    do_copy_raw_mask (rst, maskbuf, width, height, x_res, y_res, minx, maxy,
		      tile_minx, tile_maxy, tile_width, tile_height);
    return 1;
}

static void
get_int8_ennuple (const char *rawbuf, unsigned short row, unsigned short col,
		  unsigned short row_stride, rl2PixelPtr no_data,
		  double ennuple[], int *has_no_data)
{
/* extracting a 3x3 "super-pixel" - INT8 */
    const char *p_in;
    char nd_val = 0;
    int i;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) == RL2_OK)
	    {
		if (sample_type == RL2_SAMPLE_INT8 || num_bands == 1)
		    rl2_get_pixel_sample_int8 (no_data, &nd_val);
	    }
      }
    p_in = rawbuf + (row * row_stride) + col;
    ennuple[0] = *p_in++;
    ennuple[1] = *p_in++;
    ennuple[2] = *p_in++;
    p_in = rawbuf + ((row + 1) * row_stride) + col;
    ennuple[3] = *p_in++;
    ennuple[4] = *p_in++;
    ennuple[5] = *p_in++;
    p_in = rawbuf + ((row + 2) * row_stride) + col;
    ennuple[6] = *p_in++;
    ennuple[7] = *p_in++;
    ennuple[8] = *p_in++;
    *has_no_data = 0;
    for (i = 0; i < 9; i++)
      {
	  /* checking for NoData values */
	  if (ennuple[i] == nd_val)
	      *has_no_data = 1;
      }
}

static void
get_uint8_ennuple (const unsigned char *rawbuf, unsigned short row,
		   unsigned short col, unsigned short row_stride,
		   rl2PixelPtr no_data, double ennuple[], int *has_no_data)
{
/* extracting a 3x3 "super-pixel" - UINT16 */
    const unsigned char *p_in;
    unsigned char nd_val = 0;
    int i;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) == RL2_OK)
	    {
		if (sample_type == RL2_SAMPLE_UINT8 || num_bands == 1)
		    rl2_get_pixel_sample_uint8 (no_data, 0, &nd_val);
	    }
      }
    p_in = rawbuf + (row * row_stride) + col;
    ennuple[0] = *p_in++;
    ennuple[1] = *p_in++;
    ennuple[2] = *p_in++;
    p_in = rawbuf + ((row + 1) * row_stride) + col;
    ennuple[3] = *p_in++;
    ennuple[4] = *p_in++;
    ennuple[5] = *p_in++;
    p_in = rawbuf + ((row + 2) * row_stride) + col;
    ennuple[6] = *p_in++;
    ennuple[7] = *p_in++;
    ennuple[8] = *p_in++;
    *has_no_data = 0;
    for (i = 0; i < 9; i++)
      {
	  /* checking for NoData values */
	  if (ennuple[i] == nd_val)
	      *has_no_data = 1;
      }
}

static void
get_int16_ennuple (const short *rawbuf, unsigned short row,
		   unsigned short col, unsigned short row_stride,
		   rl2PixelPtr no_data, double ennuple[], int *has_no_data)
{
/* extracting a 3x3 "super-pixel" - INT16 */
    const short *p_in;
    short nd_val = 0;
    int i;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) == RL2_OK)
	    {
		if (sample_type == RL2_SAMPLE_INT16 || num_bands == 1)
		    rl2_get_pixel_sample_int16 (no_data, &nd_val);
	    }
      }
    p_in = rawbuf + (row * row_stride) + col;
    ennuple[0] = *p_in++;
    ennuple[1] = *p_in++;
    ennuple[2] = *p_in++;
    p_in = rawbuf + ((row + 1) * row_stride) + col;
    ennuple[3] = *p_in++;
    ennuple[4] = *p_in++;
    ennuple[5] = *p_in++;
    p_in = rawbuf + ((row + 2) * row_stride) + col;
    ennuple[6] = *p_in++;
    ennuple[7] = *p_in++;
    ennuple[8] = *p_in++;
    *has_no_data = 0;
    for (i = 0; i < 9; i++)
      {
	  /* checking for NoData values */
	  if (ennuple[i] == nd_val)
	      *has_no_data = 1;
      }
}

static void
get_uint16_ennuple (const unsigned short *rawbuf, unsigned short row,
		    unsigned short col, unsigned short row_stride,
		    rl2PixelPtr no_data, double ennuple[], int *has_no_data)
{
/* extracting a 3x3 "super-pixel" - UINT16 */
    const unsigned short *p_in;
    unsigned short nd_val = 0;
    int i;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) == RL2_OK)
	    {
		if (sample_type == RL2_SAMPLE_UINT16 || num_bands == 1)
		    rl2_get_pixel_sample_uint16 (no_data, 0, &nd_val);
	    }
      }
    p_in = rawbuf + (row * row_stride) + col;
    ennuple[0] = *p_in++;
    ennuple[1] = *p_in++;
    ennuple[2] = *p_in++;
    p_in = rawbuf + ((row + 1) * row_stride) + col;
    ennuple[3] = *p_in++;
    ennuple[4] = *p_in++;
    ennuple[5] = *p_in++;
    p_in = rawbuf + ((row + 2) * row_stride) + col;
    ennuple[6] = *p_in++;
    ennuple[7] = *p_in++;
    ennuple[8] = *p_in++;
    *has_no_data = 0;
    for (i = 0; i < 9; i++)
      {
	  /* checking for NoData values */
	  if (ennuple[i] == nd_val)
	      *has_no_data = 1;
      }
}

static void
get_int32_ennuple (const int *rawbuf, unsigned short row, unsigned short col,
		   unsigned short row_stride, rl2PixelPtr no_data,
		   double ennuple[], int *has_no_data)
{
/* extracting a 3x3 "super-pixel" - INT32 */
    const int *p_in;
    int nd_val = 0;
    int i;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) == RL2_OK)
	    {
		if (sample_type == RL2_SAMPLE_INT32 || num_bands == 1)
		    rl2_get_pixel_sample_int32 (no_data, &nd_val);
	    }
      }
    p_in = rawbuf + (row * row_stride) + col;
    ennuple[0] = *p_in++;
    ennuple[1] = *p_in++;
    ennuple[2] = *p_in++;
    p_in = rawbuf + ((row + 1) * row_stride) + col;
    ennuple[3] = *p_in++;
    ennuple[4] = *p_in++;
    ennuple[5] = *p_in++;
    p_in = rawbuf + ((row + 2) * row_stride) + col;
    ennuple[6] = *p_in++;
    ennuple[7] = *p_in++;
    ennuple[8] = *p_in++;
    *has_no_data = 0;
    for (i = 0; i < 9; i++)
      {
	  /* checking for NoData values */
	  if (ennuple[i] == nd_val)
	      *has_no_data = 1;
      }
}

static void
get_uint32_ennuple (const unsigned int *rawbuf, unsigned short row,
		    unsigned short col, unsigned short row_stride,
		    rl2PixelPtr no_data, double ennuple[], int *has_no_data)
{
/* extracting a 3x3 "super-pixel" - UINT32 */
    const unsigned int *p_in;
    unsigned int nd_val = 0;
    int i;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) == RL2_OK)
	    {
		if (sample_type == RL2_SAMPLE_UINT32 || num_bands == 1)
		    rl2_get_pixel_sample_uint32 (no_data, &nd_val);
	    }
      }
    p_in = rawbuf + (row * row_stride) + col;
    ennuple[0] = *p_in++;
    ennuple[1] = *p_in++;
    ennuple[2] = *p_in++;
    p_in = rawbuf + ((row + 1) * row_stride) + col;
    ennuple[3] = *p_in++;
    ennuple[4] = *p_in++;
    ennuple[5] = *p_in++;
    p_in = rawbuf + ((row + 2) * row_stride) + col;
    ennuple[6] = *p_in++;
    ennuple[7] = *p_in++;
    ennuple[8] = *p_in++;
    *has_no_data = 0;
    for (i = 0; i < 9; i++)
      {
	  /* checking for NoData values */
	  if (ennuple[i] == nd_val)
	      *has_no_data = 1;
      }
}

static void
get_float_ennuple (const float *rawbuf, unsigned short row,
		   unsigned short col, unsigned short row_stride,
		   rl2PixelPtr no_data, double ennuple[], int *has_no_data)
{
/* extracting a 3x3 "super-pixel" - FLOAT */
    const float *p_in;
    float nd_val = 0.0;
    int i;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) == RL2_OK)
	    {
		if (sample_type == RL2_SAMPLE_FLOAT || num_bands == 1)
		    rl2_get_pixel_sample_float (no_data, &nd_val);
	    }
      }
    p_in = rawbuf + (row * row_stride) + col;
    ennuple[0] = *p_in++;
    ennuple[1] = *p_in++;
    ennuple[2] = *p_in++;
    p_in = rawbuf + ((row + 1) * row_stride) + col;
    ennuple[3] = *p_in++;
    ennuple[4] = *p_in++;
    ennuple[5] = *p_in++;
    p_in = rawbuf + ((row + 2) * row_stride) + col;
    ennuple[6] = *p_in++;
    ennuple[7] = *p_in++;
    ennuple[8] = *p_in++;
    *has_no_data = 0;
    for (i = 0; i < 9; i++)
      {
	  /* checking for NoData values */
	  if (ennuple[i] == nd_val)
	      *has_no_data = 1;
      }
}

static void
get_double_ennuple (const double *rawbuf, unsigned short row,
		    unsigned short col, unsigned short row_stride,
		    rl2PixelPtr no_data, double ennuple[], int *has_no_data)
{
/* extracting a 3x3 "super-pixel" - DOUBLE */
    const double *p_in;
    double nd_val = 0.0;
    int i;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) == RL2_OK)
	    {
		if (sample_type == RL2_SAMPLE_DOUBLE || num_bands == 1)
		    rl2_get_pixel_sample_double (no_data, &nd_val);
	    }
      }
    p_in = rawbuf + (row * row_stride) + col;
    ennuple[0] = *p_in++;
    ennuple[1] = *p_in++;
    ennuple[2] = *p_in++;
    p_in = rawbuf + ((row + 1) * row_stride) + col;
    ennuple[3] = *p_in++;
    ennuple[4] = *p_in++;
    ennuple[5] = *p_in++;
    p_in = rawbuf + ((row + 2) * row_stride) + col;
    ennuple[6] = *p_in++;
    ennuple[7] = *p_in++;
    ennuple[8] = *p_in++;
    *has_no_data = 0;
    for (i = 0; i < 9; i++)
      {
	  /* checking for NoData values */
	  if (ennuple[i] == nd_val)
	      *has_no_data = 1;
      }
}

static float
compute_shaded_relief (double relief_factor, double scale_factor,
		       double altRadians, double azRadians, double ennuple[])
{
/* actual computation */
    double x;
    double y;
    double z_factor = 0.0033333333 * (relief_factor / 55.0);
    double aspect;
    double slope;
    double value;

/* First Slope ... */
    x = z_factor * ((ennuple[0] + ennuple[3] + ennuple[3] + ennuple[6]) -
		    (ennuple[2] + ennuple[5] + ennuple[5] +
		     ennuple[8])) / scale_factor;
    y = z_factor * ((ennuple[6] + ennuple[7] + ennuple[7] + ennuple[8]) -
		    (ennuple[0] + ennuple[1] + ennuple[1] +
		     ennuple[2])) / scale_factor;
    slope = M_PI / 2 - atan (sqrt (x * x + y * y));
/* ... then aspect... */
    aspect = atan2 (x, y);
/* ... then the shade value */
    value =
	sin (altRadians) * sin (slope) +
	cos (altRadians) * cos (slope) * cos (azRadians - M_PI / 2 - aspect);
/* normalizing */
    if (value < 0.0)
	value = 0.0;
    if (value > 1.0)
	value = 1.0;
    return value;
}

static float
shaded_relief_value (double relief_factor, double scale_factor,
		     double altRadians, double azRadians, void *rawbuf,
		     unsigned short row, unsigned short col,
		     unsigned short row_stride, unsigned char sample_type,
		     rl2PixelPtr no_data)
{
/* computing a ShadedRelief Pixel value */
    double ennuple[9];
    int has_no_data;
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  get_int8_ennuple (rawbuf, row, col, row_stride, no_data,
			    ennuple, &has_no_data);
	  break;
      case RL2_SAMPLE_UINT8:
	  get_uint8_ennuple (rawbuf, row, col, row_stride, no_data,
			     ennuple, &has_no_data);
	  break;
      case RL2_SAMPLE_INT16:
	  get_int16_ennuple (rawbuf, row, col, row_stride, no_data,
			     ennuple, &has_no_data);
	  break;
      case RL2_SAMPLE_UINT16:
	  get_uint16_ennuple (rawbuf, row, col, row_stride, no_data,
			      ennuple, &has_no_data);
	  break;
      case RL2_SAMPLE_INT32:
	  get_int32_ennuple (rawbuf, row, col, row_stride, no_data,
			     ennuple, &has_no_data);
	  break;
      case RL2_SAMPLE_UINT32:
	  get_uint32_ennuple (rawbuf, row, col, row_stride, no_data,
			      ennuple, &has_no_data);
	  break;
      case RL2_SAMPLE_FLOAT:
	  get_float_ennuple (rawbuf, row, col, row_stride, no_data,
			     ennuple, &has_no_data);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  get_double_ennuple (rawbuf, row, col, row_stride, no_data,
			      ennuple, &has_no_data);
	  break;
      default:
	  return -1.0;
      };
    if (has_no_data)
	return -1.0;
    return compute_shaded_relief (relief_factor, scale_factor, altRadians,
				  azRadians, ennuple);
}

#if defined(_WIN32) && !defined(__MINGW32__)
DWORD WINAPI
doRunShadowerThread (void *arg)
#else
void *
doRunShadowerThread (void *arg)
#endif
{
/* threaded function: decoding a Tile */
    float *p_out;
    unsigned short row;
    unsigned short col;
    rl2AuxShadowerPtr shadower = (rl2AuxShadowerPtr) arg;
    for (row = shadower->start_row; row < shadower->height;
	 row += shadower->row_increment)
      {
	  p_out = shadower->sr_mask + (row * shadower->width);
	  for (col = 0; col < shadower->width; col++)
	      *p_out++ =
		  shaded_relief_value (shadower->relief_factor,
				       shadower->scale_factor,
				       shadower->altRadians,
				       shadower->azRadians, shadower->rawbuf,
				       row, col, shadower->row_stride,
				       shadower->sample_type,
				       (rl2PixelPtr) (shadower->no_data));
      }
#if defined(_WIN32) && !defined(__MINGW32__)
    return 0;
#else
    pthread_exit (NULL);
#endif
}

static void
start_shadower_thread (rl2AuxShadowerPtr shadower)
{
/* starting a concurrent thread */
#if defined(_WIN32) && !defined(__MINGW32__)
    HANDLE thread_handle;
    HANDLE *p_thread;
    DWORD dwThreadId;
    thread_handle =
	CreateThread (NULL, 0, doRunShadowerThread, shadower, 0, &dwThreadId);
    SetThreadPriority (thread_handle, THREAD_PRIORITY_IDLE);
    p_thread = malloc (sizeof (HANDLE));
    *p_thread = thread_handle;
    shadower->opaque_thread_id = p_thread;
#else
    pthread_t thread_id;
    pthread_t *p_thread;
    int ok_prior = 0;
    int policy;
    int min_prio;
    pthread_attr_t attr;
    struct sched_param sp;
    pthread_attr_init (&attr);
    if (pthread_attr_setschedpolicy (&attr, SCHED_RR) == 0)
      {
	  /* attempting to set the lowest priority */
	  if (pthread_attr_getschedpolicy (&attr, &policy) == 0)
	    {
		min_prio = sched_get_priority_min (policy);
		sp.sched_priority = min_prio;
		if (pthread_attr_setschedparam (&attr, &sp) == 0)
		  {
		      /* ok, setting the lowest priority */
		      ok_prior = 1;
		      pthread_create (&thread_id, &attr, doRunShadowerThread,
				      shadower);
		  }
	    }
      }
    if (!ok_prior)
      {
	  /* failure: using standard priority */
	  pthread_create (&thread_id, NULL, doRunShadowerThread, shadower);
      }
    p_thread = malloc (sizeof (pthread_t));
    *p_thread = thread_id;
    shadower->opaque_thread_id = p_thread;
#endif
}

static void
do_run_concurrent_shadower (rl2AuxShadowerPtr aux, int max_threads)
{
/* concurrent execution of all shadower children threads */
    rl2AuxShadowerPtr shadower;
    int i;
#if defined(_WIN32) && !defined(__MINGW32__)
    HANDLE *handles;
#endif

    for (i = 0; i < max_threads; i++)
      {
	  /* starting all children threads */
	  shadower = aux + i;
	  start_shadower_thread (shadower);
      }

/* waiting until all child threads exit */
#if defined(_WIN32) && !defined(__MINGW32__)
    handles = malloc (sizeof (HANDLE) * max_threads);
    for (i = 0; i < max_threads; i++)
      {
	  /* initializing the HANDLEs array */
	  HANDLE *pOpaque;
	  shadower = aux + i;
	  pOpaque = (HANDLE *) (shadower->opaque_thread_id);
	  *(handles + i) = *pOpaque;
      }
    WaitForMultipleObjects (max_threads, handles, TRUE, INFINITE);
    free (handles);
#else
    for (i = 0; i < max_threads; i++)
      {
	  pthread_t *pOpaque;
	  shadower = aux + i;
	  pOpaque = (pthread_t *) (shadower->opaque_thread_id);
	  pthread_join (*pOpaque, NULL);
      }
#endif

/* all children threads have now finished: resuming the main thread */
    for (i = 0; i < max_threads; i++)
      {
	  /* cleaning up a request slot */
	  if (shadower->opaque_thread_id != NULL)
	      free (shadower->opaque_thread_id);
	  shadower->opaque_thread_id = NULL;
      }
}

RL2_PRIVATE int
rl2_build_shaded_relief_mask (sqlite3 * handle, int max_threads,
			      rl2CoveragePtr cvg,
			      int by_section, sqlite3_int64 section_id,
			      double relief_factor, double scale_factor,
			      unsigned int width, unsigned int height,
			      double minx, double miny, double maxx,
			      double maxy, double x_res, double y_res,
			      float **shaded_relief, int *shaded_relief_sz)
{
/* attempting to return a Shaded Relief mask from the DBMS Coverage */
    rl2PixelPtr no_data = NULL;
    const char *coverage;
    const char *db_prefix;
    char *xdb_prefix;
    char *xrtree;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    char *xtiles;
    char *xxtiles;
    char *xdata;
    char *xxdata;
    char *sql;
    sqlite3_stmt *stmt_tiles = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;
    void *rawbuf = NULL;
    int rawbuf_size;
    int pix_sz = 1;
    float *sr_mask = NULL;
    int sr_mask_size;
    float *p_out;
    unsigned short row;
    unsigned short col;
    unsigned short row_stride;
    double degreesToRadians = M_PI / 180.0;
    double altRadians = 45.0 * degreesToRadians;	/* altitude: 45.0 */
    double azRadians = 315.0 * degreesToRadians;	/* azimuth: 315.0 */

    if (cvg == NULL || handle == NULL)
	goto error;
    coverage = rl2_get_coverage_name (cvg);
    if (coverage == NULL)
	goto error;
    db_prefix = rl2_get_coverage_prefix (cvg);
    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	goto error;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_DATAGRID && num_bands != 1)
	goto error;
    no_data = rl2_get_coverage_no_data (cvg);
    if (no_data == NULL)
	goto error;


/* preparing the "tiles" SQL query */
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = rl2_double_quoted_sql (xtiles);
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xdb_prefix = rl2_double_quoted_sql (db_prefix);
    xrtree = sqlite3_mprintf ("DB=%s.%s_tiles", db_prefix, coverage);
    sql =
	sqlite3_mprintf
	("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry) "
	 "FROM \"%s\".\"%s\" WHERE pyramid_level = ? AND ROWID IN ( "
	 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	 "AND search_frame = BuildMBR(?, ?, ?, ?))", xdb_prefix, xxtiles,
	 xrtree);
    sqlite3_free (xtiles);
    sqlite3_free (xrtree);
    free (xxtiles);
    free (xdb_prefix);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tiles, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT shadedRelief tiles SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    if (scale == RL2_SCALE_1)
      {
	  /* preparing the data SQL query - both ODD and EVEN */
	  xdata = sqlite3_mprintf ("%s_tile_data", coverage);
	  xxdata = rl2_double_quoted_sql (xdata);
	  sqlite3_free (xdata);
	  if (db_prefix == NULL)
	      db_prefix = "MAIN";
	  xdb_prefix = rl2_double_quoted_sql (db_prefix);
	  sql = sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even "
				 "FROM \"%s\".\"%s\" WHERE tile_id = ?",
				 xdb_prefix, xxdata);
	  free (xxdata);
	  free (xdb_prefix);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("SELECT shadedRelief data(2) SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }
    else
      {
	  /* preparing the data SQL query - only ODD */
	  xdata = sqlite3_mprintf ("%s_tile_data", coverage);
	  xxdata = rl2_double_quoted_sql (xdata);
	  sqlite3_free (xdata);
	  if (db_prefix == NULL)
	      db_prefix = "MAIN";
	  xdb_prefix = rl2_double_quoted_sql (db_prefix);
	  sql = sqlite3_mprintf ("SELECT tile_data_odd "
				 "FROM \"%s\".\"%s\" WHERE tile_id = ?",
				 xdb_prefix, xxdata);
	  free (xxdata);
	  free (xdb_prefix);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("SELECT shadedRelief data(1) SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }

/* preparing a raw pixels buffer */
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
    row_stride = width + 2;
    rawbuf_size = (pix_sz * row_stride) * (height + 2);
    rawbuf = malloc (rawbuf_size);
    if (rawbuf == NULL)
      {
	  fprintf (stderr,
		   "rl2_build_shaded_relief_mask: Insufficient Memory !!!\n");
	  goto error;
      }
    void_raw_buffer (rawbuf, width + 2, height + 2, sample_type, 1, no_data);
    if (!rl2_load_dbms_tiles
	(handle, max_threads, stmt_tiles, stmt_data, rawbuf, width + 2,
	 height + 2, sample_type, 1, 0, 0, 0, xx_res, yy_res, minx - xx_res,
	 miny - yy_res, maxx + xx_res, maxy + yy_res, level, scale, NULL,
	 no_data, NULL, NULL))
	goto error;
    sqlite3_finalize (stmt_tiles);
    sqlite3_finalize (stmt_data);
    stmt_tiles = NULL;
    stmt_data = NULL;

    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;

/* preparing the Shaded Relief mask */
    sr_mask_size = sizeof (float) * width * height;
    sr_mask = malloc (sr_mask_size);
    if (sr_mask == NULL)
      {
	  fprintf (stderr,
		   "rl2_build_shaded_relief_mask: Insufficient Memory !!!\n");
	  goto error;
      }
    if (max_threads == 1)
      {
	  /* executing in a single thread */
	  p_out = sr_mask;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		    *p_out++ =
			shaded_relief_value (relief_factor, scale_factor,
					     altRadians, azRadians, rawbuf,
					     row, col, row_stride,
					     sample_type, no_data);
	    }
      }
    else
      {
	  /* executing as many concurrent threads */
	  rl2AuxShadowerPtr aux = NULL;
	  rl2AuxShadowerPtr shadower;
	  int iaux;
	  aux = malloc (sizeof (rl2AuxShadower) * max_threads);
	  if (aux == NULL)
	      return 0;
	  for (iaux = 0; iaux < max_threads; iaux++)
	    {
		/* initializing an empty AuxShadower slot */
		shadower = aux + iaux;
		shadower->opaque_thread_id = NULL;
		shadower->width = width;
		shadower->height = height;
		shadower->relief_factor = relief_factor;
		shadower->scale_factor = scale_factor;
		shadower->altRadians = altRadians;
		shadower->azRadians = azRadians;
		shadower->rawbuf = rawbuf;
		shadower->start_row = iaux;
		shadower->row_increment = max_threads;
		shadower->row_stride = row_stride;
		shadower->sample_type = sample_type;
		shadower->no_data = (rl2PrivPixelPtr) no_data;
		shadower->sr_mask = sr_mask;
	    }
	  do_run_concurrent_shadower (aux, max_threads);
	  free (aux);
      }

    free (rawbuf);
    *shaded_relief = sr_mask;
    *shaded_relief_sz = sr_mask_size;
    return RL2_OK;

  error:
    if (stmt_tiles != NULL)
	sqlite3_finalize (stmt_tiles);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (rawbuf != NULL)
	free (rawbuf);
    return RL2_ERROR;
}
