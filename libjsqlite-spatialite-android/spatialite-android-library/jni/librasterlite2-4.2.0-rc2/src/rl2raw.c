/*

 rl2raw -- raw buffer export functions

 version 0.1, 2013 April 1

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

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

static int
check_as_rgb (rl2PrivRasterPtr rst)
{
/* check if this raster could be exported as RGB */
    switch (rst->pixelType)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
	  return 1;
      };
    return 0;
}

static void
grayscale_as_rgb (unsigned char sample_type, unsigned char gray,
		  unsigned char *r, unsigned char *g, unsigned char *b)
{
/* grayscale 8bit resolution as RGB */
    unsigned char index;
    *r = 0;
    *g = 0;
    *b = 0;
    switch (sample_type)
      {
      case RL2_SAMPLE_2_BIT:
	  switch (gray)
	    {
	    case 3:
		index = 255;
		break;
	    case 2:
		index = 170;
		break;
	    case 1:
		index = 86;
		break;
	    case 0:
	    default:
		index = 0;
		break;
	    };
	  *r = index;
	  *g = index;
	  *b = index;
	  break;
      case RL2_SAMPLE_4_BIT:
	  switch (gray)
	    {
	    case 15:
		index = 255;
		break;
	    case 14:
		index = 239;
		break;
	    case 13:
		index = 222;
		break;
	    case 12:
		index = 205;
		break;
	    case 11:
		index = 188;
		break;
	    case 10:
		index = 171;
		break;
	    case 9:
		index = 154;
		break;
	    case 8:
		index = 137;
		break;
	    case 7:
		index = 119;
		break;
	    case 6:
		index = 102;
		break;
	    case 5:
		index = 85;
		break;
	    case 4:
		index = 68;
		break;
	    case 3:
		index = 51;
		break;
	    case 2:
		index = 34;
		break;
	    case 1:
		index = 17;
		break;
	    case 0:
	    default:
		index = 0;
		break;
	    };
	  *r = index;
	  *g = index;
	  *b = index;
	  break;
      case RL2_SAMPLE_UINT8:
	  *r = gray;
	  *g = gray;
	  *b = gray;
	  break;
      };
}

RL2_DECLARE int
rl2_raster_data_to_RGB (rl2RasterPtr ptr, unsigned char **buffer, int *buf_size)
{
/* attempting to export Raster pixel data as an RGB array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned short max_palette;
    unsigned char *red = NULL;
    unsigned char *green = NULL;
    unsigned char *blue = NULL;
    unsigned char index;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_rgb (rst))
	return RL2_ERROR;

    if (rst->pixelType == RL2_PIXEL_PALETTE)
      {
	  /* there is a palette */
	  if (rl2_get_palette_colors
	      ((rl2PalettePtr) (rst->Palette), &max_palette, &red, &green,
	       &blue) != RL2_OK)
	      return RL2_ERROR;
      }

    sz = rst->width * rst->height * 3;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = rst->rasterBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	    {
		switch (rst->pixelType)
		  {
		  case RL2_PIXEL_MONOCHROME:
		      if (*p_in++ == 0)
			  index = 255;
		      else
			  index = 0;
		      *p_out++ = index;
		      *p_out++ = index;
		      *p_out++ = index;
		      break;
		  case RL2_PIXEL_PALETTE:
		      index = *p_in++;
		      if (index < max_palette)
			{
			    *p_out++ = *(red + index);
			    *p_out++ = *(green + index);
			    *p_out++ = *(blue + index);
			}
		      else
			{
			    /* default - inserting a BLACK pixel */
			    *p_out++ = 0;
			    *p_out++ = 0;
			    *p_out++ = 0;
			}
		      break;
		  case RL2_PIXEL_GRAYSCALE:
		      grayscale_as_rgb (rst->sampleType, *p_in++, &r, &g, &b);
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		      break;
		  case RL2_PIXEL_RGB:
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      break;
		  };
	    }
      }

    *buffer = buf;
    *buf_size = sz;
    if (red != NULL)
	free (red);
    if (green != NULL)
	free (green);
    if (blue != NULL)
	free (blue);
    return RL2_OK;
}

static int
eval_transparent_pixels (unsigned char r, unsigned char g, unsigned char b,
			 unsigned char transpR, unsigned char transpG,
			 unsigned char transpB)
{
/* evaluating a transparent color match */
    if (r != transpR)
	return 0;
    if (g != transpG)
	return 0;
    if (b != transpB)
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_raster_data_to_RGBA (rl2RasterPtr ptr, unsigned char **buffer,
			 int *buf_size)
{
/* attempting to export Raster pixel data as an RGBA array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_mask;
    unsigned char *p_out;
    unsigned short max_palette;
    unsigned char *red = NULL;
    unsigned char *green = NULL;
    unsigned char *blue = NULL;
    unsigned char index;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    unsigned char transpR;
    unsigned char transpG;
    unsigned char transpB;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_rgb (rst))
	return RL2_ERROR;

    if (rst->pixelType == RL2_PIXEL_PALETTE)
      {
	  /* there is a palette */
	  if (rl2_get_palette_colors
	      ((rl2PalettePtr) (rst->Palette), &max_palette, &red, &green,
	       &blue) != RL2_OK)
	      return RL2_ERROR;
      }

    sz = rst->width * rst->height * 4;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    if (rst->noData != NULL)
      {
	  /* preparing the transparent color */
	  rl2PrivPixelPtr no_data = rst->noData;
	  switch (no_data->pixelType)
	    {
	    case RL2_PIXEL_MONOCHROME:
		if (no_data->Samples->uint8 == 0)
		  {
		      transpR = 255;
		      transpG = 255;
		      transpB = 255;
		  }
		else
		  {
		      transpR = 0;
		      transpG = 0;
		      transpB = 0;
		  }
		break;
	    case RL2_PIXEL_PALETTE:
		index = no_data->Samples->uint8;
		if (index < max_palette)
		  {
		      transpR = *(red + index);
		      transpG = *(green + index);
		      transpB = *(blue + index);
		  }
		else
		  {
		      /* default - inserting a BLACK pixel */
		      transpR = 0;
		      transpG = 0;
		      transpB = 0;
		  }
		break;
	    case RL2_PIXEL_GRAYSCALE:
		grayscale_as_rgb (rst->sampleType, no_data->Samples->uint8,
				  &transpR, &transpG, &transpB);
		break;
	    case RL2_PIXEL_RGB:
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data, RL2_RED_BAND,
					    &transpR);
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data,
					    RL2_GREEN_BAND, &transpG);
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data,
					    RL2_BLUE_BAND, &transpB);
		break;
	    };
      }

    p_in = rst->rasterBuffer;
    p_mask = rst->maskBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	    {
		switch (rst->pixelType)
		  {
		  case RL2_PIXEL_MONOCHROME:
		      if (*p_in++ == 0)
			  index = 255;
		      else
			  index = 0;
		      r = index;
		      g = index;
		      b = index;
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		      a = 255;
		      if (p_mask != NULL)
			{
			    if (*p_mask++ == 0)
				a = 0;
			}
		      if (rst->noData != NULL)
			{
			    /* evaluating transparent color */
			    if (eval_transparent_pixels
				(r, g, b, transpR, transpG, transpB))
				a = 0;
			}
		      *p_out++ = a;
		      break;
		  case RL2_PIXEL_PALETTE:
		      index = *p_in++;
		      if (index < max_palette)
			{
			    r = *(red + index);
			    g = *(green + index);
			    b = *(blue + index);
			}
		      else
			{
			    /* default - inserting a BLACK pixel */
			    r = 0;
			    g = 0;
			    b = 0;
			}
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		      a = 255;
		      if (p_mask != NULL)
			{
			    if (*p_mask++ == 0)
				a = 0;
			}
		      if (rst->noData != NULL)
			{
			    /* evaluating transparent color */
			    if (eval_transparent_pixels
				(r, g, b, transpR, transpG, transpB))
				a = 0;
			}
		      *p_out++ = a;
		      break;
		  case RL2_PIXEL_GRAYSCALE:
		      grayscale_as_rgb (rst->sampleType, *p_in++, &r, &g, &b);
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		      a = 255;
		      if (p_mask != NULL)
			{
			    if (*p_mask++ == 0)
				a = 0;
			}
		      if (rst->noData != NULL)
			{
			    /* evaluating transparent color */
			    if (eval_transparent_pixels
				(r, g, b, transpR, transpG, transpB))
				a = 0;
			}
		      *p_out++ = a;
		      break;
		  case RL2_PIXEL_RGB:
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		      a = 255;
		      if (p_mask != NULL)
			{
			    if (*p_mask++ == 0)
				a = 0;
			}
		      if (rst->noData != NULL)
			{
			    /* evaluating transparent color */
			    if (eval_transparent_pixels
				(r, g, b, transpR, transpG, transpB))
				a = 0;
			}
		      *p_out++ = a;
		      break;
		  };
	    }
      }

    *buffer = buf;
    *buf_size = sz;
    if (red != NULL)
	free (red);
    if (green != NULL)
	free (green);
    if (blue != NULL)
	free (blue);
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_ARGB (rl2RasterPtr ptr, unsigned char **buffer,
			 int *buf_size)
{
/* attempting to export Raster pixel data as an ARGB array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_mask;
    unsigned char *p_out;
    unsigned short max_palette;
    unsigned char *red = NULL;
    unsigned char *green = NULL;
    unsigned char *blue = NULL;
    unsigned char index;
    unsigned char a;
    unsigned char *p_alpha;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char transpR;
    unsigned char transpG;
    unsigned char transpB;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_rgb (rst))
	return RL2_ERROR;

    if (rst->pixelType == RL2_PIXEL_PALETTE)
      {
	  /* there is a palette */
	  if (rl2_get_palette_colors
	      ((rl2PalettePtr) (rst->Palette), &max_palette, &red, &green,
	       &blue) != RL2_OK)
	      return RL2_ERROR;
      }

    sz = rst->width * rst->height * 4;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    if (rst->noData != NULL)
      {
	  /* preparing the transparent color */
	  rl2PrivPixelPtr no_data = rst->noData;
	  switch (no_data->pixelType)
	    {
	    case RL2_PIXEL_MONOCHROME:
		if (no_data->Samples->uint8 == 0)
		  {
		      transpR = 255;
		      transpG = 255;
		      transpB = 255;
		  }
		else
		  {
		      transpR = 0;
		      transpG = 0;
		      transpB = 0;
		  }
		break;
	    case RL2_PIXEL_PALETTE:
		index = no_data->Samples->uint8;
		if (index < max_palette)
		  {
		      transpR = *(red + index);
		      transpG = *(green + index);
		      transpB = *(blue + index);
		  }
		else
		  {
		      /* default - inserting a BLACK pixel */
		      transpR = 0;
		      transpG = 0;
		      transpB = 0;
		  }
		break;
	    case RL2_PIXEL_GRAYSCALE:
		grayscale_as_rgb (rst->sampleType, no_data->Samples->uint8,
				  &transpR, &transpG, &transpB);
		break;
	    case RL2_PIXEL_RGB:
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data, RL2_RED_BAND,
					    &transpR);
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data,
					    RL2_GREEN_BAND, &transpG);
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data,
					    RL2_BLUE_BAND, &transpB);
		break;
	    };
      }

    p_in = rst->rasterBuffer;
    p_mask = rst->maskBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	    {
		if (p_mask == NULL)
		    a = 255;
		else
		  {
		      if (*p_mask++ == 0)
			  a = 0;
		      else
			  a = 255;
		  }
		switch (rst->pixelType)
		  {
		  case RL2_PIXEL_MONOCHROME:
		      if (*p_in++ == 0)
			  index = 255;
		      else
			  index = 0;
		      p_alpha = p_out++;
		      r = *p_out++ = index;
		      g = *p_out++ = index;
		      b = *p_out++ = index;
		      break;
		  case RL2_PIXEL_PALETTE:
		      index = *p_in++;
		      if (index < max_palette)
			{
			    *p_out++ = 255;
			    *p_out++ = *(red + index);
			    *p_out++ = *(green + index);
			    *p_out++ = *(blue + index);
			}
		      else
			{
			    /* default - inserting a BLACK pixel */
			    *p_out++ = 255;
			    *p_out++ = 0;
			    *p_out++ = 0;
			    *p_out++ = 0;
			}
		      break;
		  case RL2_PIXEL_GRAYSCALE:
		      grayscale_as_rgb (rst->sampleType, *p_in++, &r, &g, &b);
		      p_alpha = p_out++;
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		      break;
		  case RL2_PIXEL_RGB:
		      p_alpha = p_out++;
		      r = *p_out++ = *p_in++;
		      g = *p_out++ = *p_in++;
		      b = *p_out++ = *p_in++;
		      break;
		  };
		if (rst->pixelType != RL2_PIXEL_PALETTE)
		  {
		      if (rst->noData != NULL)
			{
			    /* evaluating transparent color */
			    if (eval_transparent_pixels
				(r, g, b, transpR, transpG, transpB))
				a = 0;
			}
		      *p_alpha = a;
		  }
	    }
      }

    *buffer = buf;
    *buf_size = sz;
    if (red != NULL)
	free (red);
    if (green != NULL)
	free (green);
    if (blue != NULL)
	free (blue);
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_BGR (rl2RasterPtr ptr, unsigned char **buffer, int *buf_size)
{
/* attempting to export Raster pixel data as an BGR array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned short max_palette;
    unsigned char *red = NULL;
    unsigned char *green = NULL;
    unsigned char *blue = NULL;
    unsigned char index;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_rgb (rst))
	return RL2_ERROR;

    if (rst->pixelType == RL2_PIXEL_PALETTE)
      {
	  /* there is a palette */
	  if (rl2_get_palette_colors
	      ((rl2PalettePtr) (rst->Palette), &max_palette, &red, &green,
	       &blue) != RL2_OK)
	      return RL2_ERROR;
      }

    sz = rst->width * rst->height * 3;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = rst->rasterBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	    {
		switch (rst->pixelType)
		  {
		  case RL2_PIXEL_MONOCHROME:
		      if (*p_in++ == 0)
			  index = 255;
		      else
			  index = 0;
		      *p_out++ = index;
		      *p_out++ = index;
		      *p_out++ = index;
		      break;
		  case RL2_PIXEL_PALETTE:
		      index = *p_in++;
		      if (index < max_palette)
			{
			    *p_out++ = *(blue + index);
			    *p_out++ = *(green + index);
			    *p_out++ = *(red + index);
			}
		      else
			{
			    /* default - inserting a BLACK pixel */
			    *p_out++ = 0;
			    *p_out++ = 0;
			    *p_out++ = 0;
			}
		      break;
		  case RL2_PIXEL_GRAYSCALE:
		      grayscale_as_rgb (rst->sampleType, *p_in++, &r, &g, &b);
		      *p_out++ = b;
		      *p_out++ = g;
		      *p_out++ = r;
		      break;
		  case RL2_PIXEL_RGB:
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		      *p_out++ = b;
		      *p_out++ = g;
		      *p_out++ = r;
		      break;
		  };
	    }
      }

    *buffer = buf;
    *buf_size = sz;
    if (red != NULL)
	free (red);
    if (green != NULL)
	free (green);
    if (blue != NULL)
	free (blue);
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_BGRA (rl2RasterPtr ptr, unsigned char **buffer,
			 int *buf_size)
{
/* attempting to export Raster pixel data as an BGRA array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_mask;
    unsigned char *p_out;
    unsigned short max_palette;
    unsigned char *red = NULL;
    unsigned char *green = NULL;
    unsigned char *blue = NULL;
    unsigned char index;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    unsigned char transpR;
    unsigned char transpG;
    unsigned char transpB;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_rgb (rst))
	return RL2_ERROR;

    if (rst->pixelType == RL2_PIXEL_PALETTE)
      {
	  /* there is a palette */
	  if (rl2_get_palette_colors
	      ((rl2PalettePtr) (rst->Palette), &max_palette, &red, &green,
	       &blue) != RL2_OK)
	      return RL2_ERROR;
      }

    sz = rst->width * rst->height * 4;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    if (rst->noData != NULL)
      {
	  /* preparing the transparent color */
	  rl2PrivPixelPtr no_data = rst->noData;
	  switch (no_data->pixelType)
	    {
	    case RL2_PIXEL_MONOCHROME:
		if (no_data->Samples->uint8 == 0)
		  {
		      transpR = 255;
		      transpG = 255;
		      transpB = 255;
		  }
		else
		  {
		      transpR = 0;
		      transpG = 0;
		      transpB = 0;
		  }
		break;
	    case RL2_PIXEL_PALETTE:
		index = no_data->Samples->uint8;
		if (index < max_palette)
		  {
		      transpR = *(red + index);
		      transpG = *(green + index);
		      transpB = *(blue + index);
		  }
		else
		  {
		      /* default - inserting a BLACK pixel */
		      transpR = 0;
		      transpG = 0;
		      transpB = 0;
		  }
		break;
	    case RL2_PIXEL_GRAYSCALE:
		grayscale_as_rgb (rst->sampleType, no_data->Samples->uint8,
				  &transpR, &transpG, &transpB);
		break;
	    case RL2_PIXEL_RGB:
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data, RL2_RED_BAND,
					    &transpR);
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data,
					    RL2_GREEN_BAND, &transpG);
		rl2_get_pixel_sample_uint8 ((rl2PixelPtr) no_data,
					    RL2_BLUE_BAND, &transpB);
		break;
	    };
      }

    p_in = rst->rasterBuffer;
    p_mask = rst->maskBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	    {
		if (p_mask == NULL)
		    a = 255;
		else
		  {
		      if (*p_mask++ == 0)
			  a = 0;
		      else
			  a = 255;
		  }
		switch (rst->pixelType)
		  {
		  case RL2_PIXEL_MONOCHROME:
		      if (*p_in++ == 0)
			  index = 255;
		      else
			  index = 0;
		      b = *p_out++ = index;
		      g = *p_out++ = index;
		      r = *p_out++ = index;
		      break;
		  case RL2_PIXEL_PALETTE:
		      index = *p_in++;
		      if (index < max_palette)
			{
			    *p_out++ = *(blue + index);
			    *p_out++ = *(green + index);
			    *p_out++ = *(red + index);
			    *p_out++ = 255;
			}
		      else
			{
			    /* default - inserting a BLACK pixel */
			    *p_out++ = 0;
			    *p_out++ = 0;
			    *p_out++ = 0;
			    *p_out++ = 255;
			}
		      break;
		  case RL2_PIXEL_GRAYSCALE:
		      grayscale_as_rgb (rst->sampleType, *p_in++, &r, &g, &b);
		      *p_out++ = b;
		      *p_out++ = g;
		      *p_out++ = r;
		      break;
		  case RL2_PIXEL_RGB:
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		      *p_out++ = b;
		      *p_out++ = g;
		      *p_out++ = r;
		      break;
		  };
		if (rst->pixelType != RL2_PIXEL_PALETTE)
		  {
		      if (rst->noData != NULL)
			{
			    /* evaluating transparent color */
			    if (eval_transparent_pixels
				(r, g, b, transpR, transpG, transpB))
				a = 0;
			}
		      *p_out++ = a;
		  }
	    }
      }

    *buffer = buf;
    *buf_size = sz;
    if (red != NULL)
	free (red);
    if (green != NULL)
	free (green);
    if (blue != NULL)
	free (blue);
    return RL2_OK;
}

static int
check_as_datagrid (rl2PrivRasterPtr rst, unsigned char sample_type)
{
/* check if this raster could be exported as a DATAGRID */
    if (rst->pixelType == RL2_PIXEL_DATAGRID && rst->sampleType == sample_type)
	return 1;
    return 0;
}

static int
check_as_grayscale256 (rl2PrivRasterPtr rst)
{
/* check if this raster could be exported as a GRAYSCALE 8 bit */
    if (rst->pixelType == RL2_PIXEL_GRAYSCALE
	&& rst->sampleType == RL2_SAMPLE_UINT8)
	return 1;
    return 0;
}

static int
check_as_palette256 (rl2PrivRasterPtr rst)
{
/* check if this raster could be exported as a PALETTE 8 bit */
    if (rst->pixelType == RL2_PIXEL_PALETTE
	&& rst->sampleType == RL2_SAMPLE_UINT8)
	return 1;
    return 0;
}

RL2_DECLARE int
rl2_raster_data_to_int8 (rl2RasterPtr ptr, char **buffer, int *buf_size)
{
/* attempting to export Raster DATAGRID data as an INT-8 array */
    char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    char *p_in;
    char *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_datagrid (rst, RL2_SAMPLE_INT8))
	return RL2_ERROR;

    sz = rst->width * rst->height;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = (char *) (rst->rasterBuffer);
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_uint8 (rl2RasterPtr ptr, unsigned char **buffer,
			  int *buf_size)
{
/* attempting to export Raster DATAGRID/GRAYSCALE data as a UINT-8 array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_datagrid (rst, RL2_SAMPLE_UINT8))
      {
	  if (!check_as_grayscale256 (rst) && !check_as_palette256 (rst))
	      return RL2_ERROR;
      }

    sz = rst->width * rst->height;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = rst->rasterBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_int16 (rl2RasterPtr ptr, short **buffer, int *buf_size)
{
/* attempting to export Raster DATAGRID data as an INT-16 array */
    short *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    short *p_in;
    short *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_datagrid (rst, RL2_SAMPLE_INT16))
	return RL2_ERROR;

    sz = rst->width * rst->height * sizeof (short);
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = (short *) (rst->rasterBuffer);
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_uint16 (rl2RasterPtr ptr, unsigned short **buffer,
			   int *buf_size)
{
/* attempting to export Raster DATAGRID data as a UINT-16 array */
    unsigned short *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned short *p_in;
    unsigned short *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_datagrid (rst, RL2_SAMPLE_UINT16))
	return RL2_ERROR;

    sz = rst->width * rst->height * sizeof (unsigned short);
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = (unsigned short *) (rst->rasterBuffer);
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_int32 (rl2RasterPtr ptr, int **buffer, int *buf_size)
{
/* attempting to export Raster DATAGRID data as an INT-32 array */
    int *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    int *p_in;
    int *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_datagrid (rst, RL2_SAMPLE_INT32))
	return RL2_ERROR;

    sz = rst->width * rst->height * sizeof (int);
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = (int *) (rst->rasterBuffer);
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_uint32 (rl2RasterPtr ptr, unsigned int **buffer,
			   int *buf_size)
{
/* attempting to export Raster DATAGRID data as a UINT-32 array */
    unsigned int *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned int *p_in;
    unsigned int *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_datagrid (rst, RL2_SAMPLE_UINT32))
	return RL2_ERROR;

    sz = rst->width * rst->height * sizeof (unsigned int);
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = (unsigned int *) (rst->rasterBuffer);
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_float (rl2RasterPtr ptr, float **buffer, int *buf_size)
{
/* attempting to export Raster DATAGRID data as a FLOAT array */
    float *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    float *p_in;
    float *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_datagrid (rst, RL2_SAMPLE_FLOAT))
	return RL2_ERROR;

    sz = rst->width * rst->height * sizeof (float);
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = (float *) (rst->rasterBuffer);
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_double (rl2RasterPtr ptr, double **buffer, int *buf_size)
{
/* attempting to export Raster DATAGRID data as a DOUBLE array */
    double *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    double *p_in;
    double *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_datagrid (rst, RL2_SAMPLE_DOUBLE))
	return RL2_ERROR;

    sz = rst->width * rst->height * sizeof (double);
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = (double *) (rst->rasterBuffer);
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

static int
check_as_band (rl2PrivRasterPtr rst, int band, unsigned char sample_type)
{
/* check if this raster could be exported as a MULTIBAND */
    if (rst->pixelType == RL2_PIXEL_MULTIBAND && rst->sampleType == sample_type)
      {
	  if (band >= 0 && band < rst->nBands)
	      return 1;
      }
    if (rst->pixelType == RL2_PIXEL_RGB && rst->sampleType == sample_type)
      {
	  if (band >= 0 && band < rst->nBands)
	      return 1;
      }
    return 0;
}

RL2_DECLARE int
rl2_raster_data_to_1bit (rl2RasterPtr ptr, unsigned char **buffer,
			 int *buf_size)
{
/* attempting to export Raster PALETTE/MONOCHROME 1bit data as a UINT-8 array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (rst->sampleType == RL2_SAMPLE_1_BIT
	&& (rst->pixelType == RL2_PIXEL_MONOCHROME
	    || rst->pixelType == RL2_PIXEL_PALETTE))
	;
    else
	return RL2_ERROR;

    sz = rst->width * rst->height;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = rst->rasterBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_2bit (rl2RasterPtr ptr, unsigned char **buffer,
			 int *buf_size)
{
/* attempting to export Raster PALETTE/GRAYSCALE 2bit data as a UINT-8 array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (rst->sampleType == RL2_SAMPLE_2_BIT
	&& (rst->pixelType == RL2_PIXEL_PALETTE
	    || rst->pixelType == RL2_PIXEL_GRAYSCALE))
	;
    else
	return RL2_ERROR;

    sz = rst->width * rst->height;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = rst->rasterBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_data_to_4bit (rl2RasterPtr ptr, unsigned char **buffer,
			 int *buf_size)
{
/* attempting to export Raster PALETTE/GRAYSCALE 4bit data as a UINT-8 array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (rst->sampleType == RL2_SAMPLE_4_BIT
	&& (rst->pixelType == RL2_PIXEL_PALETTE
	    || rst->pixelType == RL2_PIXEL_GRAYSCALE))
	;
    else
	return RL2_ERROR;

    sz = rst->width * rst->height;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = rst->rasterBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	      *p_out++ = *p_in++;
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_band_to_uint8 (rl2RasterPtr ptr, int band, unsigned char **buffer,
			  int *buf_size)
{
/* attempting to export Raster BAND data as a UINT-8 array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    int nBand;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_band (rst, band, RL2_SAMPLE_UINT8))
	return RL2_ERROR;

    sz = rst->width * rst->height;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = rst->rasterBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	    {
		for (nBand = 0; nBand < rst->nBands; nBand++)
		  {
		      if (nBand == band)
			  *p_out++ = *p_in++;
		      else
			  p_in++;
		  }
	    }
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_band_to_uint16 (rl2RasterPtr ptr, int band, unsigned short **buffer,
			   int *buf_size)
{
/* attempting to export Raster BAND data as a UINT-16 array */
    unsigned short *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    int nBand;
    unsigned short *p_in;
    unsigned short *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_band (rst, band, RL2_SAMPLE_UINT16))
	return RL2_ERROR;

    sz = rst->width * rst->height * sizeof (unsigned short);
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = (unsigned short *) (rst->rasterBuffer);
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	    {
		for (nBand = 0; nBand < rst->nBands; nBand++)
		  {
		      if (nBand == band)
			  *p_out++ = *p_in++;
		      else
			  p_in++;
		  }
	    }
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_bands_to_RGB (rl2RasterPtr ptr, int bandR, int bandG, int bandB,
			 unsigned char **buffer, int *buf_size)
{
/* attempting to export Raster MULTIBAND data as an RGB array */
    unsigned char *buf;
    int sz;
    unsigned int row;
    unsigned int col;
    int nBand;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    *buffer = NULL;
    *buf_size = 0;
    if (rst == NULL)
	return RL2_ERROR;
    if (!check_as_band (rst, bandR, RL2_SAMPLE_UINT8))
	return RL2_ERROR;
    if (!check_as_band (rst, bandG, RL2_SAMPLE_UINT8))
	return RL2_ERROR;
    if (!check_as_band (rst, bandB, RL2_SAMPLE_UINT8))
	return RL2_ERROR;

    sz = rst->width * rst->height * 3;
    buf = malloc (sz);
    if (buf == NULL)
	return RL2_ERROR;

    p_in = rst->rasterBuffer;
    p_out = buf;
    for (row = 0; row < rst->height; row++)
      {
	  for (col = 0; col < rst->width; col++)
	    {
		unsigned char red;
		unsigned char green;
		unsigned char blue;
		for (nBand = 0; nBand < rst->nBands; nBand++)
		  {
		      if (nBand == bandR)
			  red = *p_in;
		      if (nBand == bandG)
			  green = *p_in;
		      if (nBand == bandB)
			  blue = *p_in;
		      p_in++;
		  }
		*p_out++ = red;
		*p_out++ = green;
		*p_out++ = blue;
	    }
      }

    *buffer = buf;
    *buf_size = sz;
    return RL2_OK;
}
