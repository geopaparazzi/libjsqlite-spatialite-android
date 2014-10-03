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

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"
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
    p_alpha = NULL;
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
		      if (p_alpha != NULL)
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
		unsigned char red = 0;
		unsigned char green = 0;
		unsigned char blue = 0;
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

static int
big_endian_cpu ()
{
/* checking if the target CPU is big-endian */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 1;
    return 0;
}

static short
swap_i16 (short value)
{
/* inverting the endianness - INT16 */
    union cvt
    {
	unsigned char byte[2];
	short value;
    };
    union cvt in;
    union cvt out;
    in.value = value;
    out.byte[0] = in.byte[1];
    out.byte[1] = in.byte[0];
    return out.value;
}

static unsigned short
swap_u16 (unsigned short value)
{
/* inverting the endianness - UINT16 */
    union cvt
    {
	unsigned char byte[2];
	unsigned short value;
    };
    union cvt in;
    union cvt out;
    in.value = value;
    out.byte[0] = in.byte[1];
    out.byte[1] = in.byte[0];
    return out.value;
}

static int
swap_i32 (int value)
{
/* inverting the endianness - INT32 */
    union cvt
    {
	unsigned char byte[4];
	int value;
    };
    union cvt in;
    union cvt out;
    in.value = value;
    out.byte[0] = in.byte[3];
    out.byte[1] = in.byte[2];
    out.byte[2] = in.byte[1];
    out.byte[3] = in.byte[0];
    return out.value;
}

static unsigned int
swap_u32 (unsigned int value)
{
/* inverting the endianness - UINT32 */
    union cvt
    {
	unsigned char byte[4];
	unsigned int value;
    };
    union cvt in;
    union cvt out;
    in.value = value;
    out.byte[0] = in.byte[3];
    out.byte[1] = in.byte[2];
    out.byte[2] = in.byte[1];
    out.byte[3] = in.byte[0];
    return out.value;
}

static float
swap_flt (float value)
{
/* inverting the endianness - FLOAT */
    union cvt
    {
	unsigned char byte[4];
	float value;
    };
    union cvt in;
    union cvt out;
    in.value = value;
    out.byte[0] = in.byte[3];
    out.byte[1] = in.byte[2];
    out.byte[2] = in.byte[1];
    out.byte[3] = in.byte[0];
    return out.value;
}

static double
swap_dbl (double value)
{
/* inverting the endianness - DOUBLE */
    union cvt
    {
	unsigned char byte[8];
	double value;
    };
    union cvt in;
    union cvt out;
    in.value = value;
    out.byte[0] = in.byte[7];
    out.byte[1] = in.byte[6];
    out.byte[2] = in.byte[5];
    out.byte[3] = in.byte[4];
    out.byte[4] = in.byte[3];
    out.byte[5] = in.byte[2];
    out.byte[6] = in.byte[1];
    out.byte[7] = in.byte[0];
    return out.value;
}

static void
copy_endian_raw_i8 (char *outbuf, const char *pixels, unsigned int width,
		    unsigned int height, unsigned char num_bands)
{
/* signed int8 */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    const char *p_in = pixels;
    char *p_out = outbuf;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_endian_raw_u8 (unsigned char *outbuf, const unsigned char *pixels,
		    unsigned int width, unsigned int height,
		    unsigned char num_bands)
{
/* unsigned int8 */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    const unsigned char *p_in = pixels;
    unsigned char *p_out = outbuf;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_endian_raw_i16 (short *outbuf, const short *pixels, unsigned int width,
		     unsigned int height, unsigned char num_bands,
		     int big_endian)
{
/* signed int16 */
    int swap = 1;
    unsigned int x;
    unsigned int y;
    unsigned char b;
    const short *p_in = pixels;
    short *p_out = outbuf;
    if (big_endian_cpu () && big_endian)
	swap = 0;
    if (!big_endian_cpu () && !big_endian)
	swap = 0;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		  {
		      if (swap)
			  *p_out++ = swap_i16 (*p_in++);
		      else
			  *p_out++ = *p_in++;
		  }
	    }
      }
}

static void
copy_endian_raw_u16 (unsigned short *outbuf, const unsigned short *pixels,
		     unsigned int width, unsigned int height,
		     unsigned char num_bands, int big_endian)
{
/* unsigned int16 */
    int swap = 1;
    unsigned int x;
    unsigned int y;
    unsigned char b;
    const unsigned short *p_in = pixels;
    unsigned short *p_out = outbuf;
    if (big_endian_cpu () && big_endian)
	swap = 0;
    if (!big_endian_cpu () && !big_endian)
	swap = 0;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		  {
		      if (swap)
			  *p_out++ = swap_u16 (*p_in++);
		      else
			  *p_out++ = *p_in++;
		  }
	    }
      }
}

static void
copy_endian_raw_i32 (int *outbuf, const int *pixels, unsigned int width,
		     unsigned int height, unsigned char num_bands,
		     int big_endian)
{
/* signed int32 */
    int swap = 1;
    unsigned int x;
    unsigned int y;
    unsigned char b;
    const int *p_in = pixels;
    int *p_out = outbuf;
    if (big_endian_cpu () && big_endian)
	swap = 0;
    if (!big_endian_cpu () && !big_endian)
	swap = 0;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		  {
		      if (swap)
			  *p_out++ = swap_i32 (*p_in++);
		      else
			  *p_out++ = *p_in++;
		  }
	    }
      }
}

static void
copy_endian_raw_u32 (unsigned int *outbuf, const unsigned int *pixels,
		     unsigned int width, unsigned int height,
		     unsigned char num_bands, int big_endian)
{
/* unsigned int32 */
    int swap = 1;
    unsigned int x;
    unsigned int y;
    unsigned char b;
    const unsigned int *p_in = pixels;
    unsigned int *p_out = outbuf;
    if (big_endian_cpu () && big_endian)
	swap = 0;
    if (!big_endian_cpu () && !big_endian)
	swap = 0;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		  {
		      if (swap)
			  *p_out++ = swap_u32 (*p_in++);
		      else
			  *p_out++ = *p_in++;
		  }
	    }
      }
}

static void
copy_endian_raw_flt (float *outbuf, const float *pixels, unsigned int width,
		     unsigned int height, unsigned char num_bands,
		     int big_endian)
{
/* float */
    int swap = 1;
    unsigned int x;
    unsigned int y;
    unsigned char b;
    const float *p_in = pixels;
    float *p_out = outbuf;
    if (big_endian_cpu () && big_endian)
	swap = 0;
    if (!big_endian_cpu () && !big_endian)
	swap = 0;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		  {
		      if (swap)
			  *p_out++ = swap_flt (*p_in++);
		      else
			  *p_out++ = *p_in++;
		  }
	    }
      }
}

static void
copy_endian_raw_dbl (double *outbuf, const double *pixels, unsigned int width,
		     unsigned int height, unsigned char num_bands,
		     int big_endian)
{
/* double */
    int swap = 1;
    unsigned int x;
    unsigned int y;
    unsigned char b;
    const double *p_in = pixels;
    double *p_out = outbuf;
    if (big_endian_cpu () && big_endian)
	swap = 0;
    if (!big_endian_cpu () && !big_endian)
	swap = 0;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		for (b = 0; b < num_bands; b++)
		  {
		      if (swap)
			  *p_out++ = swap_dbl (*p_in++);
		      else
			  *p_out++ = *p_in++;
		  }
	    }
      }
}

RL2_PRIVATE unsigned char *
rl2_copy_endian_raw_pixels (const unsigned char *pixels, int pixels_sz,
			    unsigned int width, unsigned int height,
			    unsigned char sample_type, unsigned char num_bands,
			    int big_endian)
{
/* copying RAW pixels (in endian safe mode) */
    int sample_bytes = 0;
    int outsize = width * height * num_bands;
    unsigned char *outbuf = NULL;

    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
	  sample_bytes = 1;
	  break;
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  sample_bytes = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  sample_bytes = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample_bytes = 8;
	  break;
      };
    outsize *= sample_bytes;
    if (pixels_sz != outsize)
	return NULL;

/* allocating the output buffer */
    outbuf = malloc (outsize);
    if (outbuf == NULL)
	return NULL;
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  copy_endian_raw_u8 (outbuf, pixels, width, height, num_bands);
	  break;
      case RL2_SAMPLE_INT8:
	  copy_endian_raw_i8 ((char *) outbuf, (const char *) pixels, width,
			      height, num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  copy_endian_raw_i16 ((short *) outbuf, (const short *) pixels, width,
			       height, num_bands, big_endian);
	  break;
      case RL2_SAMPLE_UINT16:
	  copy_endian_raw_u16 ((unsigned short *) outbuf,
			       (const unsigned short *) pixels, width, height,
			       num_bands, big_endian);
	  break;
      case RL2_SAMPLE_INT32:
	  copy_endian_raw_i32 ((int *) outbuf, (const int *) pixels, width,
			       height, num_bands, big_endian);
	  break;
      case RL2_SAMPLE_UINT32:
	  copy_endian_raw_u32 ((unsigned int *) outbuf,
			       (const unsigned int *) pixels, width, height,
			       num_bands, big_endian);
	  break;
      case RL2_SAMPLE_FLOAT:
	  copy_endian_raw_flt ((float *) outbuf, (const float *) pixels, width,
			       height, num_bands, big_endian);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  copy_endian_raw_dbl ((double *) outbuf, (const double *) pixels,
			       width, height, num_bands, big_endian);
	  break;
      };
    return outbuf;
}

static int
eval_raw_pixels_compatibility (rl2PrivCoveragePtr coverage,
			       rl2PrivRasterPtr raster)
{
/* checking for strict compatibility */
    if (coverage->sampleType == raster->sampleType
	&& coverage->pixelType == raster->pixelType
	&& coverage->nBands == raster->nBands)
	return 1;
    fprintf (stderr, "Mismatching RAW pixels !!!\n");
    return 0;
}

static void
copy_tile_raw_i8 (const char *in, unsigned int in_width, unsigned int in_height,
		  unsigned int startRow, unsigned int startCol, char *out,
		  unsigned int tileWidth, unsigned int tileHeight,
		  unsigned char num_bands)
{
/* signed int8 */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    for (y = 0; y < tileHeight; y++)
      {
	  const char *p_in;
	  if ((startRow + y) >= in_height)
	      break;
	  p_in =
	      in + ((startRow + y) * in_width * num_bands) +
	      (startCol * num_bands);
	  for (x = 0; x < tileWidth; x++)
	    {
		char *p_out;
		if ((startCol + x) >= in_width)
		    break;
		p_out = out + (y * tileWidth * num_bands) + (x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_tile_raw_u8 (const unsigned char *in, unsigned int in_width,
		  unsigned int in_height, unsigned int startRow,
		  unsigned int startCol, unsigned char *out,
		  unsigned int tileWidth, unsigned int tileHeight,
		  unsigned char num_bands)
{
/* unsigned int8 */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    for (y = 0; y < tileHeight; y++)
      {
	  const unsigned char *p_in;
	  if ((startRow + y) >= in_height)
	      break;
	  p_in =
	      in + ((startRow + y) * in_width * num_bands) +
	      (startCol * num_bands);
	  for (x = 0; x < tileWidth; x++)
	    {
		unsigned char *p_out;
		if ((startCol + x) >= in_width)
		    break;
		p_out = out + (y * tileWidth * num_bands) + (x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_tile_raw_i16 (const short *in, unsigned int in_width,
		   unsigned int in_height, unsigned int startRow,
		   unsigned int startCol, short *out, unsigned int tileWidth,
		   unsigned int tileHeight, unsigned char num_bands)
{
/* signed int16 */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    for (y = 0; y < tileHeight; y++)
      {
	  const short *p_in;
	  if ((startRow + y) >= in_height)
	      break;
	  p_in =
	      in + ((startRow + y) * in_width * num_bands) +
	      (startCol * num_bands);
	  for (x = 0; x < tileWidth; x++)
	    {
		short *p_out;
		if ((startCol + x) >= in_width)
		    break;
		p_out = out + (y * tileWidth * num_bands) + (x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_tile_raw_u16 (const unsigned short *in, unsigned int in_width,
		   unsigned int in_height, unsigned int startRow,
		   unsigned int startCol, unsigned short *out,
		   unsigned int tileWidth, unsigned int tileHeight,
		   unsigned char num_bands)
{
/* unsigned int16 */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    for (y = 0; y < tileHeight; y++)
      {
	  const unsigned short *p_in;
	  if ((startRow + y) >= in_height)
	      break;
	  p_in =
	      in + ((startRow + y) * in_width * num_bands) +
	      (startCol * num_bands);
	  for (x = 0; x < tileWidth; x++)
	    {
		unsigned short *p_out;
		if ((startCol + x) >= in_width)
		    break;
		p_out = out + (y * tileWidth * num_bands) + (x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_tile_raw_i32 (const int *in, unsigned int in_width, unsigned int in_height,
		   unsigned int startRow, unsigned int startCol, int *out,
		   unsigned int tileWidth, unsigned int tileHeight,
		   unsigned char num_bands)
{
/* signed int32 */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    for (y = 0; y < tileHeight; y++)
      {
	  const int *p_in;
	  if ((startRow + y) >= in_height)
	      break;
	  p_in =
	      in + ((startRow + y) * in_width * num_bands) +
	      (startCol * num_bands);
	  for (x = 0; x < tileWidth; x++)
	    {
		int *p_out;
		if ((startCol + x) >= in_width)
		    break;
		p_out = out + (y * tileWidth * num_bands) + (x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_tile_raw_u32 (const unsigned int *in, unsigned int in_width,
		   unsigned int in_height, unsigned int startRow,
		   unsigned int startCol, unsigned int *out,
		   unsigned int tileWidth, unsigned int tileHeight,
		   unsigned char num_bands)
{
/* unsigned int16 */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    for (y = 0; y < tileHeight; y++)
      {
	  const unsigned int *p_in;
	  if ((startRow + y) >= in_height)
	      break;
	  p_in =
	      in + ((startRow + y) * in_width * num_bands) +
	      (startCol * num_bands);
	  for (x = 0; x < tileWidth; x++)
	    {
		unsigned int *p_out;
		if ((startCol + x) >= in_width)
		    break;
		p_out = out + (y * tileWidth * num_bands) + (x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_tile_raw_flt (const float *in, unsigned int in_width,
		   unsigned int in_height, unsigned int startRow,
		   unsigned int startCol, float *out, unsigned int tileWidth,
		   unsigned int tileHeight, unsigned char num_bands)
{
/* FLOAT */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    for (y = 0; y < tileHeight; y++)
      {
	  const float *p_in;
	  if ((startRow + y) >= in_height)
	      break;
	  p_in =
	      in + ((startRow + y) * in_width * num_bands) +
	      (startCol * num_bands);
	  for (x = 0; x < tileWidth; x++)
	    {
		float *p_out;
		if ((startCol + x) >= in_width)
		    break;
		p_out = out + (y * tileWidth * num_bands) + (x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static void
copy_tile_raw_dbl (const double *in, unsigned int in_width,
		   unsigned int in_height, unsigned int startRow,
		   unsigned int startCol, double *out, unsigned int tileWidth,
		   unsigned int tileHeight, unsigned char num_bands)
{
/* DOUBLE */
    unsigned int x;
    unsigned int y;
    unsigned char b;
    for (y = 0; y < tileHeight; y++)
      {
	  const double *p_in;
	  if ((startRow + y) >= in_height)
	      break;
	  p_in =
	      in + ((startRow + y) * in_width * num_bands) +
	      (startCol * num_bands);
	  for (x = 0; x < tileWidth; x++)
	    {
		double *p_out;
		if ((startCol + x) >= in_width)
		    break;
		p_out = out + (y * tileWidth * num_bands) + (x * num_bands);
		for (b = 0; b < num_bands; b++)
		    *p_out++ = *p_in++;
	    }
      }
}

static int
build_tile_from_raw_pixels (rl2PrivRasterPtr origin, unsigned int tileWidth,
			    unsigned int tileHeight, unsigned char sample_type,
			    unsigned char num_bands, unsigned int startRow,
			    unsigned int startCol, rl2PixelPtr no_data,
			    unsigned char **pixels, int *pixels_sz)
{
/* extracting a Tile from the RAW buffer */
    unsigned char *out;
    int outsz = tileWidth * tileHeight * num_bands;
    unsigned char sample_sz = 1;
    switch (sample_type)
      {
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
    outsz *= sample_sz;
    out = malloc (outsz);
    if (out == NULL)
	return 0;

    rl2_prime_void_tile (out, tileWidth, tileHeight, sample_type, num_bands,
			 no_data);
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  copy_tile_raw_i8 ((const char *) (origin->rasterBuffer),
			    origin->width, origin->height, startRow, startCol,
			    (char *) out, tileWidth, tileHeight, num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  copy_tile_raw_i16 ((const short *) (origin->rasterBuffer),
			     origin->width, origin->height, startRow, startCol,
			     (short *) out, tileWidth, tileHeight, num_bands);
	  break;
      case RL2_SAMPLE_UINT16:
	  copy_tile_raw_u16 ((const unsigned short *) (origin->rasterBuffer),
			     origin->width, origin->height, startRow, startCol,
			     (unsigned short *) out, tileWidth, tileHeight,
			     num_bands);
	  break;
      case RL2_SAMPLE_INT32:
	  copy_tile_raw_i32 ((const int *) (origin->rasterBuffer),
			     origin->width, origin->height, startRow, startCol,
			     (int *) out, tileWidth, tileHeight, num_bands);
	  break;
      case RL2_SAMPLE_UINT32:
	  copy_tile_raw_u32 ((const unsigned int *) (origin->rasterBuffer),
			     origin->width, origin->height, startRow, startCol,
			     (unsigned int *) out, tileWidth, tileHeight,
			     num_bands);
	  break;
      case RL2_SAMPLE_FLOAT:
	  copy_tile_raw_flt ((const float *) (origin->rasterBuffer),
			     origin->width, origin->height, startRow, startCol,
			     (float *) out, tileWidth, tileHeight, num_bands);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  copy_tile_raw_dbl ((const double *) (origin->rasterBuffer),
			     origin->width, origin->height, startRow, startCol,
			     (double *) out, tileWidth, tileHeight, num_bands);
	  break;
      default:
	  copy_tile_raw_u8 ((const unsigned char *) (origin->rasterBuffer),
			    origin->width, origin->height, startRow, startCol,
			    (unsigned char *) out, tileWidth, tileHeight,
			    num_bands);
	  break;
      };

    *pixels = out;
    *pixels_sz = outsz;
    return 1;
}

RL2_DECLARE rl2RasterPtr
rl2_get_tile_from_raw_pixels (rl2CoveragePtr cvg, rl2RasterPtr jpeg,
			      unsigned int startRow, unsigned int startCol)
{
/* attempting to create a Coverage-tile from a RAW pixel buffer */
    unsigned int x;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    rl2PrivRasterPtr origin = (rl2PrivRasterPtr) jpeg;
    rl2RasterPtr raster = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    unsigned int unused_width = 0;
    unsigned int unused_height = 0;

    if (coverage == NULL || origin == NULL)
	return NULL;
    if (!eval_raw_pixels_compatibility (coverage, origin))
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
    if (!build_tile_from_raw_pixels
	(origin, coverage->tileWidth, coverage->tileHeight,
	 coverage->sampleType, coverage->nBands, startRow, startCol,
	 (rl2PixelPtr) (coverage->noData), &pixels, &pixels_sz) != RL2_OK)
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
    raster =
	rl2_create_raster (coverage->tileWidth, coverage->tileHeight,
			   coverage->sampleType, coverage->pixelType,
			   coverage->nBands, pixels, pixels_sz, NULL, mask,
			   mask_size, NULL);
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

RL2_DECLARE char *
rl2_build_raw_pixels_xml_summary (rl2RasterPtr rst)
{

/* attempting to build an XML Summary from a RAW pixel buffer */
    char *xml;
    char *prev;
    int len;
    unsigned char bps;
    const char *photo;
    const char *frmt;
    rl2PrivRasterPtr raster = (rl2PrivRasterPtr) rst;
    if (raster == NULL)
	return NULL;

    xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<ImportedRaster>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<RasterFormat>RAW pixel buffer</RasterFormat>",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<RasterWidth>%u</RasterWidth>", prev,
			 raster->width);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<RasterHeight>%u</RasterHeight>", prev,
			 raster->height);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<RowsPerStrip>1</RowsPerStrip>", prev);
    sqlite3_free (prev);
    prev = xml;
    switch (raster->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
	  bps = 1;
	  break;
      case RL2_SAMPLE_2_BIT:
	  bps = 2;
	  break;
      case RL2_SAMPLE_4_BIT:
	  bps = 4;
	  break;
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
	  bps = 8;
	  break;
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  bps = 16;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  bps = 32;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  bps = 64;
	  break;
      default:
	  bps = 0;
	  break;
      };
    xml = sqlite3_mprintf ("%s<BitsPerSample>%u</BitsPerSample>", prev, bps);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<SamplesPerPixel>%u</SamplesPerPixel>", prev,
			 raster->nBands);
    sqlite3_free (prev);
    prev = xml;
    switch (raster->pixelType)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_MULTIBAND:
      case RL2_PIXEL_DATAGRID:
	  photo = "min-is-black";
	  break;
      case RL2_PIXEL_PALETTE:
	  photo = "Palette";
	  break;
      case RL2_PIXEL_RGB:
	  photo = "RGB";
	  break;
      default:
	  photo = "unknown";
	  break;
      };
    xml =
	sqlite3_mprintf
	("%s<PhotometricInterpretation>%s</PhotometricInterpretation>",
	 prev, photo);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Compression>none</Compression>", prev);
    sqlite3_free (prev);
    prev = xml;
    switch (raster->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
      case RL2_SAMPLE_UINT16:
      case RL2_SAMPLE_UINT32:
	  frmt = "unsigned integer";
	  break;
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_INT32:
	  frmt = "signed integer";
	  break;
      case RL2_SAMPLE_FLOAT:
      case RL2_SAMPLE_DOUBLE:
	  frmt = "floating point";
	  break;
      default:
	  frmt = "unknown";
	  break;
      };
    xml = sqlite3_mprintf ("%s<SampleFormat>%s</SampleFormat>", prev, frmt);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<PlanarConfiguration>single Raster plane</PlanarConfiguration>",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<NoDataPixel>unknown</NoDataPixel>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<GeoReferencing>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<SpatialReferenceSystem>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<SRID>%d</SRID>", prev, raster->Srid);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<RefSysName>undeclared</RefSysName>", prev);
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
	 raster->hResolution);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<VerticalResolution>%1.10f</VerticalResolution>", prev,
	 raster->vResolution);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</SpatialResolution>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<BoundingBox>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<MinX>%1.10f</MinX>", prev, raster->minX);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<MinY>%1.10f</MinY>", prev, raster->minY);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<MaxX>%1.10f</MaxX>", prev, raster->maxX);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<MaxY>%1.10f</MaxY>", prev, raster->maxY);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</BoundingBox>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Extent>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<HorizontalExtent>%1.10f</HorizontalExtent>",
			 prev, raster->maxX - raster->minX);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<VerticalExtent>%1.10f</VerticalExtent>",
			 prev, raster->maxY - raster->minY);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Extent>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</GeoReferencing>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ImportedRaster>", prev);
    sqlite3_free (prev);
    len = strlen (xml);
    prev = xml;
    xml = malloc (len + 1);
    strcpy (xml, prev);
    sqlite3_free (prev);
    return xml;
}
