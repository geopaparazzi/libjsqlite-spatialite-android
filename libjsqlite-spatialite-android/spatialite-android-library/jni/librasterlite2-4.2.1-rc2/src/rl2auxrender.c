/*

 rl2auxrender -- auxiliary methods for Raster Group rendering

 version 0.1, 2014 July 30

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

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

static void
copy_monochrome (unsigned char *rgba, unsigned int width, unsigned int height,
		 unsigned char *inbuf)
{
/* copying from Monochrome to RGBA */
    unsigned int x;
    unsigned int y;
    unsigned char *p_in = inbuf;
    unsigned char *p_out = rgba;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char mono = *p_in++;
		if (mono == 0)
		  {
		      /* skipping a transparent pixel */
		      p_out += 4;
		  }
		else
		  {
		      *p_out++ = 0;
		      *p_out++ = 0;
		      *p_out++ = 0;
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
}

static void
copy_palette (unsigned char *rgba, unsigned int width, unsigned int height,
	      unsigned char *inbuf, rl2PalettePtr palette, unsigned char bg_red,
	      unsigned char bg_green, unsigned char bg_blue)
{
/* copying from Palette to RGBA */
    unsigned int x;
    unsigned int y;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;
    unsigned char *p_in = inbuf;
    unsigned char *p_out = rgba;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char red = 0;
		unsigned char green = 0;
		unsigned char blue = 0;
		unsigned char index = *p_in++;
		if (index < plt->nEntries)
		  {
		      rl2PrivPaletteEntryPtr entry = plt->entries + index;
		      red = entry->red;
		      green = entry->green;
		      blue = entry->blue;
		  }
		if (red == bg_red && green == bg_green && blue == bg_blue)
		  {
		      /* skipping a transparent pixel */
		      p_out += 4;
		  }
		else
		  {
		      *p_out++ = red;
		      *p_out++ = green;
		      *p_out++ = blue;
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
}

static void
copy_grayscale (unsigned char *rgba, unsigned int width, unsigned int height,
		unsigned char *inbuf, unsigned char bg_gray)
{
/* copying from Grayscale to RGBA */
    unsigned int x;
    unsigned int y;
    unsigned char *p_in = inbuf;
    unsigned char *p_out = rgba;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char gray = *p_in++;
		if (gray == bg_gray)
		  {
		      /* skipping a transparent pixel */
		      p_out += 4;
		  }
		else
		  {
		      *p_out++ = gray;
		      *p_out++ = gray;
		      *p_out++ = gray;
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
}

static void
copy_rgb (unsigned char *rgba, unsigned int width, unsigned int height,
	  unsigned char *inbuf, unsigned char bg_red, unsigned char bg_green,
	  unsigned char bg_blue)
{
/* copying from RGB to RGBA */
    unsigned int x;
    unsigned int y;
    unsigned char *p_in = inbuf;
    unsigned char *p_out = rgba;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char red = *p_in++;
		unsigned char green = *p_in++;
		unsigned char blue = *p_in++;
		if (red == bg_red && green == bg_green && blue == bg_blue)
		  {
		      /* skipping a transparent pixel */
		      p_out += 4;
		  }
		else
		  {
		      *p_out++ = red;
		      *p_out++ = green;
		      *p_out++ = blue;
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
}

static void
copy_rgb_alpha (unsigned char *rgba, unsigned int width, unsigned int height,
		unsigned char *inbuf, unsigned char *inalpha)
{
/* copying from RGB+Alpha to RGBA */
    unsigned int x;
    unsigned int y;
    unsigned char *p_in = inbuf;
    unsigned char *p_alpha = inalpha;
    unsigned char *p_out = rgba;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char alpha = *p_alpha++;
		unsigned char red = *p_in++;
		unsigned char green = *p_in++;
		unsigned char blue = *p_in++;
		if (alpha < 128)
		  {
		      /* skipping a transparent pixel */
		      p_out += 4;
		  }
		else
		  {
		      *p_out++ = red;
		      *p_out++ = green;
		      *p_out++ = blue;
		      *p_out++ = 255;	/* opaque */
		  }
	    }
      }
}

static int
aux_render_composed_image (struct aux_renderer *aux, unsigned char *aggreg_rgba)
{
/* rendering a raster image (composed) */
    unsigned char *rgba = NULL;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    rl2GraphicsBitmapPtr base_img = NULL;
    rl2GraphicsContextPtr ctx = NULL;
    double rescale_x = (double) aux->width / (double) aux->base_width;
    double rescale_y = (double) aux->height / (double) aux->base_height;

    if (aux->out_pixel == RL2_PIXEL_PALETTE && aux->palette == NULL)
	goto error;

    if (aux->base_width == aux->width && aux->base_height == aux->height)
      {
	  if (aux->out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* Monochrome */
		copy_monochrome (aggreg_rgba, aux->base_width, aux->base_height,
				 aux->outbuf);
		aux->outbuf = NULL;
	    }
	  else if (aux->out_pixel == RL2_PIXEL_PALETTE)
	    {
		/* Palette */
		copy_palette (aggreg_rgba, aux->base_width, aux->base_height,
			      aux->outbuf, aux->palette, aux->bg_red,
			      aux->bg_green, aux->bg_blue);
		aux->outbuf = NULL;
	    }
	  else if (aux->out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		copy_grayscale (aggreg_rgba, aux->base_width, aux->base_height,
				aux->outbuf, aux->bg_red);
		aux->outbuf = NULL;
	    }
	  else
	    {
		/* RGB */
		copy_rgb (aggreg_rgba, aux->base_width, aux->base_height,
			  aux->outbuf, aux->bg_red, aux->bg_green,
			  aux->bg_blue);
		aux->outbuf = NULL;
	    }
      }
    else
      {
	  /* rescaling */
	  ctx = rl2_graph_create_context (aux->width, aux->height);
	  if (ctx == NULL)
	      goto error;
	  rgba = malloc (aux->base_width * aux->base_height * 4);
	  if (aux->out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* Monochrome */
		if (!get_rgba_from_monochrome_transparent
		    (aux->base_width, aux->base_height, aux->outbuf, rgba))
		  {
		      aux->outbuf = NULL;
		      goto error;
		  }
		aux->outbuf = NULL;
	    }
	  else if (aux->out_pixel == RL2_PIXEL_PALETTE)
	    {
		/* Palette */
		if (!get_rgba_from_palette_transparent
		    (aux->base_width, aux->base_height, aux->outbuf,
		     aux->palette, rgba, aux->bg_red, aux->bg_green,
		     aux->bg_blue))
		  {
		      aux->outbuf = NULL;
		      goto error;
		  }
		aux->outbuf = NULL;
	    }
	  else if (aux->out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		if (!get_rgba_from_grayscale_transparent
		    (aux->base_width, aux->base_height, aux->outbuf, rgba,
		     aux->bg_red))
		  {
		      aux->outbuf = NULL;
		      goto error;
		  }
		aux->outbuf = NULL;
	    }
	  else
	    {
		/* RGB */
		if (!get_rgba_from_rgb_transparent
		    (aux->base_width, aux->base_height, aux->outbuf, rgba,
		     aux->bg_red, aux->bg_green, aux->bg_blue))
		  {
		      aux->outbuf = NULL;
		      goto error;
		  }
		aux->outbuf = NULL;
	    }
	  base_img =
	      rl2_graph_create_bitmap (rgba, aux->base_width, aux->base_height);
	  if (base_img == NULL)
	      goto error;
	  rgba = NULL;
	  rl2_graph_draw_rescaled_bitmap (ctx, base_img, rescale_x, rescale_y,
					  0, 0);
	  rl2_graph_destroy_bitmap (base_img);
	  rgb = rl2_graph_get_context_rgb_array (ctx);
	  alpha = rl2_graph_get_context_alpha_array (ctx);
	  rl2_graph_destroy_context (ctx);
	  if (rgb == NULL || alpha == NULL)
	      goto error;

/* copying into the destination buffer */
	  copy_rgb_alpha (aggreg_rgba, aux->width, aux->height, rgb, alpha);
	  free (rgb);
	  free (alpha);
      }
    return 1;

  error:
    if (aux->outbuf != NULL)
	free (aux->outbuf);
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    if (rgba != NULL)
	free (rgba);
    return 0;
}

RL2_PRIVATE int
rl2_aux_render_image (struct aux_renderer *aux, unsigned char **ximage,
		      int *ximage_size)
{
/* rendering a raster image */
    unsigned char *image = NULL;
    int image_size;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    unsigned char *rgba = NULL;
    unsigned char *gray = NULL;
    rl2GraphicsBitmapPtr base_img = NULL;
    rl2GraphicsContextPtr ctx = NULL;
    double rescale_x = (double) aux->width / (double) aux->base_width;
    double rescale_y = (double) aux->height / (double) aux->base_height;

    if (aux->out_pixel == RL2_PIXEL_PALETTE && aux->palette == NULL)
	goto error;

    if (aux->base_width == aux->width && aux->base_height == aux->height)
      {
	  if (aux->out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* converting from Monochrome to Grayscale */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_payload_from_monochrome_transparent
			  (aux->base_width, aux->base_height, aux->outbuf,
			   aux->format_id, aux->quality, &image, &image_size,
			   aux->opacity))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_payload_from_monochrome_opaque
			  (aux->base_width, aux->base_height, aux->sqlite,
			   aux->minx, aux->miny, aux->maxx, aux->maxy,
			   aux->srid, aux->outbuf, aux->format_id, aux->quality,
			   &image, &image_size))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		aux->outbuf = NULL;
	    }
	  else if (aux->out_pixel == RL2_PIXEL_PALETTE)
	    {
		/* Palette */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_payload_from_palette_transparent
			  (aux->base_width, aux->base_height, aux->outbuf,
			   aux->palette, aux->format_id, aux->quality, &image,
			   &image_size, aux->bg_red, aux->bg_green,
			   aux->bg_blue, aux->opacity))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_payload_from_palette_opaque
			  (aux->base_width, aux->base_height, aux->sqlite,
			   aux->minx, aux->miny, aux->maxx, aux->maxy,
			   aux->srid, aux->outbuf, aux->palette, aux->format_id,
			   aux->quality, &image, &image_size))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		aux->outbuf = NULL;
	    }
	  else if (aux->out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_payload_from_grayscale_transparent
			  (aux->base_width, aux->base_height, aux->outbuf,
			   aux->format_id, aux->quality, &image, &image_size,
			   aux->bg_red, aux->opacity))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_payload_from_grayscale_opaque
			  (aux->base_width, aux->base_height, aux->sqlite,
			   aux->minx, aux->miny, aux->maxx, aux->maxy,
			   aux->srid, aux->outbuf, aux->format_id, aux->quality,
			   &image, &image_size))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		aux->outbuf = NULL;
	    }
	  else
	    {
		/* RGB */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_payload_from_rgb_transparent
			  (aux->base_width, aux->base_height, aux->outbuf,
			   aux->format_id, aux->quality, &image, &image_size,
			   aux->bg_red, aux->bg_green, aux->bg_blue,
			   aux->opacity))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_payload_from_rgb_opaque
			  (aux->base_width, aux->base_height, aux->sqlite,
			   aux->minx, aux->miny, aux->maxx, aux->maxy,
			   aux->srid, aux->outbuf, aux->format_id, aux->quality,
			   &image, &image_size))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		aux->outbuf = NULL;
	    }
	  *ximage = image;
	  *ximage_size = image_size;
      }
    else
      {
	  /* rescaling */
	  ctx = rl2_graph_create_context (aux->width, aux->height);
	  if (ctx == NULL)
	      goto error;
	  rgba = malloc (aux->base_width * aux->base_height * 4);
	  if (aux->out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* Monochrome - upsampled */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_rgba_from_monochrome_transparent
			  (aux->base_width, aux->base_height, aux->outbuf,
			   rgba))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_rgba_from_monochrome_opaque
			  (aux->base_width, aux->base_height, aux->outbuf,
			   rgba))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		aux->outbuf = NULL;
	    }
	  else if (aux->out_pixel == RL2_PIXEL_PALETTE)
	    {
		/* Monochrome - upsampled */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_rgba_from_palette_transparent
			  (aux->base_width, aux->base_height, aux->outbuf,
			   aux->palette, rgba, aux->bg_red, aux->bg_green,
			   aux->bg_blue))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_rgba_from_palette_opaque
			  (aux->base_width, aux->base_height, aux->outbuf,
			   aux->palette, rgba))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		aux->outbuf = NULL;
	    }
	  else if (aux->out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* Grayscale */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_rgba_from_grayscale_transparent
			  (aux->base_width, aux->base_height, aux->outbuf, rgba,
			   aux->bg_red))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_rgba_from_grayscale_opaque
			  (aux->base_width, aux->base_height, aux->outbuf,
			   rgba))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		aux->outbuf = NULL;
	    }
	  else
	    {
		/* RGB */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (!get_rgba_from_rgb_transparent
			  (aux->base_width, aux->base_height, aux->outbuf, rgba,
			   aux->bg_red, aux->bg_green, aux->bg_blue))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		else
		  {
		      if (!get_rgba_from_rgb_opaque
			  (aux->base_width, aux->base_height, aux->outbuf,
			   rgba))
			{
			    aux->outbuf = NULL;
			    goto error;
			}
		  }
		aux->outbuf = NULL;
	    }
	  base_img =
	      rl2_graph_create_bitmap (rgba, aux->base_width, aux->base_height);
	  if (base_img == NULL)
	      goto error;
	  rgba = NULL;
	  rl2_graph_draw_rescaled_bitmap (ctx, base_img, rescale_x, rescale_y,
					  0, 0);
	  rl2_graph_destroy_bitmap (base_img);
	  rgb = rl2_graph_get_context_rgb_array (ctx);
	  alpha = NULL;
	  if (aux->transparent)
	      alpha = rl2_graph_get_context_alpha_array (ctx);
	  rl2_graph_destroy_context (ctx);
	  if (rgb == NULL)
	      goto error;
	  if (aux->out_pixel == RL2_PIXEL_GRAYSCALE
	      || aux->out_pixel == RL2_PIXEL_MONOCHROME)
	    {
		/* Grayscale or Monochrome upsampled */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (alpha == NULL)
			  goto error;
		      if (!get_payload_from_gray_rgba_transparent
			  (aux->width, aux->height, rgb, alpha, aux->format_id,
			   aux->quality, &image, &image_size, aux->opacity))
			  goto error;
		  }
		else
		  {
		      if (alpha != NULL)
			  free (alpha);
		      alpha = NULL;
		      if (!get_payload_from_gray_rgba_opaque
			  (aux->width, aux->height, aux->sqlite, aux->minx,
			   aux->miny, aux->maxx, aux->maxy, aux->srid, rgb,
			   aux->format_id, aux->quality, &image, &image_size))
			  goto error;
		  }
	    }
	  else
	    {
		/* RGB */
		if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
		  {
		      if (alpha == NULL)
			  goto error;
		      if (!get_payload_from_rgb_rgba_transparent
			  (aux->width, aux->height, rgb, alpha, aux->format_id,
			   aux->quality, &image, &image_size, aux->opacity))
			  goto error;
		  }
		else
		  {
		      if (alpha != NULL)
			  free (alpha);
		      alpha = NULL;
		      if (!get_payload_from_rgb_rgba_opaque
			  (aux->width, aux->height, aux->sqlite, aux->minx,
			   aux->miny, aux->maxx, aux->maxy, aux->srid, rgb,
			   aux->format_id, aux->quality, &image, &image_size))
			  goto error;
		  }
	    }
	  *ximage = image;
	  *ximage_size = image_size;
      }
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    return 1;

  error:
    if (aux->outbuf != NULL)
	free (aux->outbuf);
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    if (rgba != NULL)
	free (rgba);
    if (gray != NULL)
	free (gray);
    return 0;
}

static int
aux_render_final_image (struct aux_group_renderer *aux, sqlite3 * sqlite,
			int srid, unsigned char *rgb, unsigned char *alpha,
			unsigned char **ximage, int *ximage_size)
{
/* rendering a raster image from the cumulative RGB+Alpha buffers */
    unsigned char *image = NULL;
    int image_size;

    if (aux->transparent && aux->format_id == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (!get_payload_from_rgb_rgba_transparent
	      (aux->width, aux->height, rgb, alpha, aux->format_id,
	       aux->quality, &image, &image_size, 1.0))
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
	  free (alpha);
	  alpha = NULL;
	  if (!get_payload_from_rgb_rgba_opaque
	      (aux->width, aux->height, sqlite, aux->minx,
	       aux->miny, aux->maxx, aux->maxy, srid, rgb,
	       aux->format_id, aux->quality, &image, &image_size))
	    {
		rgb = NULL;
		goto error;
	    }
	  rgb = NULL;
      }

    *ximage = image;
    *ximage_size = image_size;
    return 1;

  error:
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    return 0;
}

static int
aux_shaded_relief_mask (struct aux_renderer *aux, double relief_factor,
			unsigned char **shaded_relief)
{
/* attempting to create a Shaded Relief brightness-only mask */
    rl2GraphicsBitmapPtr base_img = NULL;
    rl2GraphicsContextPtr ctx = NULL;
    float *shr_mask;
    int shr_size;
    const char *coverage;
    double scale_factor;
    int row;
    int col;
    float *p_in;
    unsigned char *rgba = NULL;
    unsigned char *rgb = NULL;
    unsigned char *p_out;
    double rescale_x = (double) aux->width / (double) aux->base_width;
    double rescale_y = (double) aux->height / (double) aux->base_height;

    *shaded_relief = NULL;
    coverage = rl2_get_coverage_name (aux->coverage);
    if (coverage == NULL)
	return 0;
    scale_factor = rl2_get_shaded_relief_scale_factor (aux->sqlite, coverage);

    if (rl2_build_shaded_relief_mask
	(aux->sqlite, aux->coverage, relief_factor, scale_factor,
	 aux->base_width, aux->base_height, aux->minx, aux->miny, aux->maxx,
	 aux->maxy, aux->xx_res, aux->yy_res, &shr_mask, &shr_size) != RL2_OK)
	return 0;

/* allocating the RGBA buffer */
    rgba = malloc (aux->base_width * aux->base_height * 4);
    if (rgba == NULL)
	return 0;

/* priming a full transparent RGBA buffer */
    memset (rgba, 0, aux->base_width * aux->base_height * 4);

/* creating a pure brightness mask - RGBA */
    p_in = shr_mask;
    p_out = rgba;
    for (row = 0; row < aux->base_height; row++)
      {
	  for (col = 0; col < aux->base_width; col++)
	    {
		float coeff = *p_in++;
		if (coeff < 0.0)
		    p_out += 4;	/* transparent */
		else
		  {
		      unsigned char gray = (unsigned char) (255.0 * coeff);
		      *p_out++ = gray;
		      *p_out++ = gray;
		      *p_out++ = gray;
		      *p_out++ = 255;
		  }
	    }
      }
    free (shr_mask);

/* rescaling the brightness mask */
    ctx = rl2_graph_create_context (aux->width, aux->height);
    if (ctx == NULL)
	goto error;
    base_img =
	rl2_graph_create_bitmap (rgba, aux->base_width, aux->base_height);
    if (base_img == NULL)
	goto error;
    rgba = NULL;
    rl2_graph_draw_rescaled_bitmap (ctx, base_img, rescale_x, rescale_y, 0, 0);
    rl2_graph_destroy_bitmap (base_img);
    rgb = rl2_graph_get_context_rgb_array (ctx);
    rl2_graph_destroy_context (ctx);
    if (rgb == NULL)
	goto error;

    *shaded_relief = rgb;
    return 1;

  error:
    if (rgb != NULL)
	free (rgb);
    if (rgba != NULL)
	free (rgba);
    return 0;
}

RL2_PRIVATE void
rl2_aux_group_renderer (struct aux_group_renderer *auxgrp)
{
/* Group Renderer - attempting to render a complex layered image */
    double x_res;
    double y_res;
    double ext_x;
    double ext_y;
    int level_id;
    int scale;
    int xscale;
    double xx_res;
    double yy_res;
    int base_width;
    int base_height;
    double aspect_org;
    double aspect_dst;
    double confidence;
    int srid;
    int i;
    double opacity;
    unsigned char *outbuf = NULL;
    int outbuf_size;
    unsigned char *image = NULL;
    int image_size;
    int was_monochrome;
    unsigned char *rgba = NULL;
    unsigned char *p_rgba;
    unsigned char *rgb = NULL;
    unsigned char *p_rgb;
    unsigned char *alpha = NULL;
    unsigned char *p_alpha;
    unsigned char *shaded_relief_mask = NULL;
    int row;
    int col;
    unsigned char out_pixel = RL2_PIXEL_UNKNOWN;
    rl2GroupStylePtr group_style = NULL;
    rl2GroupRendererPtr group = NULL;
    rl2PrivGroupRendererPtr grp;
    struct aux_renderer aux;
    rl2PalettePtr palette = NULL;
    rl2PrivRasterStylePtr symbolizer = NULL;
    int by_section = 0;
    sqlite3 *sqlite = sqlite3_context_db_handle (auxgrp->context);

    ext_x = auxgrp->maxx - auxgrp->minx;
    ext_y = auxgrp->maxy - auxgrp->miny;
    x_res = ext_x / (double) (auxgrp->width);
    y_res = ext_y / (double) (auxgrp->height);

/* attempting to validate the Group Style */
    group_style =
	rl2_create_group_style_from_dbms (sqlite, auxgrp->group_name,
					  auxgrp->style);
    if (group_style == NULL)
	goto error;

    group = rl2_create_group_renderer (sqlite, group_style);
    if (group == NULL)
	goto error;

/* allocating the cumulative RGBA buffer */
    rgba = malloc (auxgrp->width * auxgrp->height * 4);
    if (rgba == NULL)
	goto error;
    p_rgba = rgba;
    for (row = 0; row < auxgrp->height; row++)
      {
	  for (col = 0; col < auxgrp->width; col++)
	    {
		/* priming the cumulative RGBA buffer */
		*p_rgba++ = auxgrp->bg_red;
		*p_rgba++ = auxgrp->bg_green;
		*p_rgba++ = auxgrp->bg_blue;
		*p_rgba++ = 0;	/* full transparent */
	    }
      }

    grp = (rl2PrivGroupRendererPtr) group;
    for (i = 0; i < grp->count; i++)
      {
	  /* rendering all rasters on the same canvass */
	  int is_shaded_relief = 0;
	  rl2PrivCoveragePtr cvg;
	  rl2PrivGroupRendererLayerPtr lyr = grp->layers + i;
	  if (lyr->layer_type != RL2_GROUP_RENDERER_RASTER_LAYER)
	      continue;
	  cvg = (rl2PrivCoveragePtr) (lyr->coverage);

	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_RGB && cvg->nBands == 3)
	      out_pixel = RL2_PIXEL_RGB;
	  if (cvg->sampleType == RL2_SAMPLE_UINT8
	      && cvg->pixelType == RL2_PIXEL_GRAYSCALE && cvg->nBands == 1)
	      out_pixel = RL2_PIXEL_GRAYSCALE;
	  if (cvg->pixelType == RL2_PIXEL_PALETTE && cvg->nBands == 1)
	      out_pixel = RL2_PIXEL_PALETTE;
	  if (cvg->pixelType == RL2_PIXEL_MONOCHROME && cvg->nBands == 1)
	      out_pixel = RL2_PIXEL_MONOCHROME;

	  if (lyr->raster_symbolizer != NULL)
	    {
		/* applying a RasterSymbolizer */
		int yes_no;
		if (lyr->raster_symbolizer->shadedRelief)
		    is_shaded_relief = 1;
		if (!is_shaded_relief)
		  {
		      if (rl2_is_raster_style_triple_band_selected
			  ((rl2RasterStylePtr) (lyr->raster_symbolizer),
			   &yes_no) == RL2_OK)
			{
			    if ((cvg->sampleType == RL2_SAMPLE_UINT8
				 || cvg->sampleType == RL2_SAMPLE_UINT16)
				&& (cvg->pixelType == RL2_PIXEL_RGB
				    || cvg->pixelType == RL2_PIXEL_MULTIBAND)
				&& yes_no)
				out_pixel = RL2_PIXEL_RGB;
			}
		      if (rl2_is_raster_style_mono_band_selected
			  ((rl2RasterStylePtr) (lyr->raster_symbolizer),
			   &yes_no) == RL2_OK)
			{
			    if ((cvg->sampleType == RL2_SAMPLE_UINT8
				 || cvg->sampleType == RL2_SAMPLE_UINT16)
				&& (cvg->pixelType == RL2_PIXEL_RGB
				    || cvg->pixelType == RL2_PIXEL_MULTIBAND
				    || cvg->pixelType == RL2_PIXEL_GRAYSCALE)
				&& yes_no)
				out_pixel = RL2_PIXEL_GRAYSCALE;
			    if ((cvg->sampleType == RL2_SAMPLE_INT8
				 || cvg->sampleType == RL2_SAMPLE_UINT8
				 || cvg->sampleType == RL2_SAMPLE_INT16
				 || cvg->sampleType == RL2_SAMPLE_UINT16
				 || cvg->sampleType == RL2_SAMPLE_INT32
				 || cvg->sampleType == RL2_SAMPLE_UINT32
				 || cvg->sampleType == RL2_SAMPLE_FLOAT
				 || cvg->sampleType == RL2_SAMPLE_DOUBLE)
				&& cvg->pixelType == RL2_PIXEL_DATAGRID
				&& yes_no)
				out_pixel = RL2_PIXEL_GRAYSCALE;
			}
		      if (rl2_get_raster_style_opacity
			  ((rl2RasterStylePtr) (lyr->raster_symbolizer),
			   &opacity) != RL2_OK)
			  opacity = 1.0;
		      if (opacity > 1.0)
			  opacity = 1.0;
		      if (opacity < 0.0)
			  opacity = 0.0;
		      if (opacity < 1.0)
			  auxgrp->transparent = 1;
		      if (out_pixel == RL2_PIXEL_UNKNOWN)
			{
			    fprintf (stderr, "*** Unsupported Pixel !!!!\n");
			    goto error;
			}
		  }
	    }

	  if (rl2_is_mixed_resolutions_coverage (sqlite, cvg->coverageName) > 0)
	    {
		/* Mixed Resolutions Coverage */
		by_section = 1;
		xx_res = x_res;
		yy_res = y_res;
	    }
	  else
	    {
		/* ordinary Coverage */
		by_section = 0;
		/* retrieving the optimal resolution level */
		if (!rl2_find_best_resolution_level
		    (sqlite, cvg->coverageName, 0, 0, x_res, y_res, &level_id,
		     &scale, &xscale, &xx_res, &yy_res))
		    goto error;
	    }
	  base_width = (int) (ext_x / xx_res);
	  base_height = (int) (ext_y / yy_res);
	  if ((base_width <= 0 && base_width >= USHRT_MAX)
	      || (base_height <= 0 && base_height >= USHRT_MAX))
	      goto error;
	  aspect_org = (double) base_width / (double) base_height;
	  aspect_dst = (double) auxgrp->width / (double) auxgrp->height;
	  confidence = aspect_org / 100.0;
	  if (aspect_dst >= (aspect_org - confidence)
	      || aspect_dst <= (aspect_org + confidence))
	      ;
	  else if (aspect_org != aspect_dst && !(auxgrp->reaspect))
	      goto error;

	  symbolizer = lyr->raster_symbolizer;
	  if (!is_shaded_relief)
	    {
		was_monochrome = 0;
		if (out_pixel == RL2_PIXEL_MONOCHROME)
		  {
		      if (level_id != 0 && scale != 1)
			{
			    out_pixel = RL2_PIXEL_GRAYSCALE;
			    was_monochrome = 1;
			}
		  }
		if (out_pixel == RL2_PIXEL_PALETTE)
		  {
		      if (level_id != 0 && scale != 1)
			  out_pixel = RL2_PIXEL_RGB;
		  }
		if (rl2_get_coverage_srid (lyr->coverage, &srid) != RL2_OK)
		    srid = -1;
		if (rl2_get_raw_raster_data_bgcolor
		    (sqlite, lyr->coverage, base_width, base_height,
		     auxgrp->minx, auxgrp->miny, auxgrp->maxx, auxgrp->maxy,
		     xx_res, yy_res, &outbuf, &outbuf_size, &palette,
		     &out_pixel, auxgrp->bg_red, auxgrp->bg_green,
		     auxgrp->bg_blue, (rl2RasterStylePtr) symbolizer,
		     (rl2RasterStatisticsPtr) (lyr->raster_stats)) != RL2_OK)
		    goto error;
		if (out_pixel == RL2_PIXEL_PALETTE && palette == NULL)
		    goto error;
		if (was_monochrome && out_pixel == RL2_PIXEL_GRAYSCALE)
		    symbolizer = NULL;
	    }

/* preparing the aux struct for passing rendering arguments */
	  aux.sqlite = sqlite;
	  aux.width = auxgrp->width;
	  aux.height = auxgrp->height;
	  aux.base_width = base_width;
	  aux.base_height = base_height;
	  aux.minx = auxgrp->minx;
	  aux.miny = auxgrp->miny;
	  aux.maxx = auxgrp->maxx;
	  aux.maxy = auxgrp->maxy;
	  aux.srid = srid;
	  if (by_section)
	    {
		aux.by_section = 1;
		aux.x_res = x_res;
		aux.y_res = y_res;
	    }
	  else
	    {
		aux.by_section = 0;
		aux.xx_res = xx_res;
		aux.yy_res = yy_res;
	    }
	  aux.transparent = auxgrp->transparent;
	  aux.opacity = opacity;
	  aux.quality = auxgrp->quality;
	  aux.format_id = auxgrp->format_id;
	  aux.bg_red = auxgrp->bg_red;
	  aux.bg_green = auxgrp->bg_green;
	  aux.bg_blue = auxgrp->bg_blue;
	  aux.coverage = lyr->coverage;
	  aux.symbolizer = (rl2RasterStylePtr) symbolizer;
	  aux.stats = (rl2RasterStatisticsPtr) (lyr->raster_stats);
	  aux.outbuf = outbuf;
	  aux.palette = palette;
	  aux.out_pixel = out_pixel;
	  if (is_shaded_relief)
	    {
		/* requesting a shaded relief mask */
		if (shaded_relief_mask != NULL)
		    free (shaded_relief_mask);
		if (!aux_shaded_relief_mask
		    (&aux, symbolizer->reliefFactor, &shaded_relief_mask))
		    goto error;
	    }
	  else
	    {
		/* rendering an image */
		if (!aux_render_composed_image (&aux, rgba))
		    goto error;
	    }
	  if (palette != NULL)
	      rl2_destroy_palette (palette);
	  palette = NULL;
      }

/* transforming the RGBA buffer into RGB+Alpha */
    rgb = malloc (auxgrp->width * auxgrp->height * 3);
    alpha = malloc (auxgrp->width * auxgrp->height);
    if (rgb == NULL || alpha == NULL)
	goto error;
    p_rgba = rgba;
    p_rgb = rgb;
    p_alpha = alpha;
    for (row = 0; row < auxgrp->height; row++)
      {
	  for (col = 0; col < auxgrp->width; col++)
	    {
		/* splitting the cumulative RGBA buffer into RGB+Alpha */
		*p_rgb++ = *p_rgba++;
		*p_rgb++ = *p_rgba++;
		*p_rgb++ = *p_rgba++;
		*p_alpha++ = *p_rgba++;
	    }
      }
    free (rgba);
    rgba = NULL;

    if (shaded_relief_mask != NULL)
      {
	  /* applying the Shaded Relief */
	  unsigned char *p_in = shaded_relief_mask;
	  p_rgb = rgb;
	  p_alpha = alpha;
	  for (row = 0; row < auxgrp->height; row++)
	    {
		for (col = 0; col < auxgrp->width; col++)
		  {
		      float coeff;
		      unsigned char v = *p_in;
		      unsigned char a = *p_alpha++;
		      p_in += 3;
		      if (a < 128)
			{
			    /* transparency */
			    p_rgb += 3;
			    continue;
			}
		      if (v == 0)
			  coeff = -1.0;
		      else
			  coeff = (float) v / 255.0;
		      if (coeff < 0.0)
			  p_rgb += 3;	/* unaffected */
		      else
			{
			    unsigned char r = *p_rgb;
			    unsigned char g = *(p_rgb + 1);
			    unsigned char b = *(p_rgb + 2);
			    *p_rgb++ = (unsigned char) (r * coeff);
			    *p_rgb++ = (unsigned char) (g * coeff);
			    *p_rgb++ = (unsigned char) (b * coeff);
			}
		  }
	    }
	  free (shaded_relief_mask);
      }

/* exporting the final composite image */
    if (!aux_render_final_image
	(auxgrp, sqlite, srid, rgb, alpha, &image, &image_size))
      {
	  rgb = NULL;
	  alpha = NULL;
	  goto error;
      }
    sqlite3_result_blob (auxgrp->context, image, image_size, free);
    rl2_destroy_group_style (group_style);
    rl2_destroy_group_renderer (group);
    return;

  error:
    if (shaded_relief_mask != NULL)
	free (shaded_relief_mask);
    if (rgba != NULL)
	free (rgba);
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (group_style != NULL)
	rl2_destroy_group_style (group_style);
    if (group != NULL)
	rl2_destroy_group_renderer (group);
    sqlite3_result_null (auxgrp->context);
}

static void
do_copy_rgb (unsigned char *out, const unsigned char *in, unsigned int width,
	     unsigned int height, unsigned int w, unsigned int h,
	     int base_x, int base_y, unsigned char bg_red,
	     unsigned char bg_green, unsigned char bg_blue)
{
/* copying RGB pixels */
    int x;
    int y;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char *p_out;
    const unsigned char *p_in = in;

    for (y = 0; y < (int) h; y++)
      {
	  if ((base_y + y) >= (int) height)
	      break;
	  if ((base_y + y) < 0)
	    {
		p_in += w * 3;
		continue;
	    }
	  for (x = 0; x < (int) w; x++)
	    {
		if ((base_x + x) < 0 || (base_x + x) >= (int) width)
		  {
		      p_in += 3;
		      continue;
		  }
		p_out = out + ((base_y + y) * width * 3) + ((base_x + x) * 3);
		r = *p_in++;
		g = *p_in++;
		b = *p_in++;
		if (r == bg_red && g == bg_green && b == bg_blue)
		  {
		      /* transparent pixel */
		      p_out += 3;
		  }
		else
		  {
		      /* opaque pixel */
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		  }
	    }
      }
}

static void
do_copy_gray (unsigned char *out, const unsigned char *in, unsigned int width,
	      unsigned int height, unsigned int w, unsigned int h,
	      int base_x, int base_y, unsigned char bg_gray)
{
/* copying Grayscale pixels */
    int x;
    int y;
    unsigned char gray;
    unsigned char *p_out;
    const unsigned char *p_in = in;

    for (y = 0; y < (int) h; y++)
      {
	  if ((base_y + y) >= (int) height)
	      break;
	  if ((base_y + y) < 0)
	    {
		p_in += w;
		continue;
	    }
	  for (x = 0; x < (int) w; x++)
	    {
		if ((base_x + x) < 0 || (base_x + x) >= (int) width)
		  {
		      p_in++;
		      continue;
		  }
		p_out = out + ((base_y + y) * width) + (base_x + x);
		gray = *p_in++;
		if (gray == bg_gray)
		  {
		      /* transparent pixel */
		      p_out++;
		  }
		else
		  {
		      /* opaque pixel */
		      *p_out++ = gray;
		  }
	    }
      }
}

RL2_DECLARE int
rl2_get_raw_raster_data_mixed_resolutions (sqlite3 * handle, rl2CoveragePtr cvg,
					   unsigned int width,
					   unsigned int height, double minx,
					   double miny, double maxx,
					   double maxy, double x_res,
					   double y_res, unsigned char **buffer,
					   int *buf_size,
					   rl2PalettePtr * palette,
					   unsigned char *out_pixel,
					   unsigned char bg_red,
					   unsigned char bg_green,
					   unsigned char bg_blue,
					   rl2RasterStylePtr style,
					   rl2RasterStatisticsPtr stats)
{
/* attempting to return raw pixels from the DBMS Coverage - Mixed Resolutions */
    int ret;
    rl2PixelPtr no_data = NULL;
    const char *coverage;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char pixel = *out_pixel;
    unsigned char *outbuf = NULL;
    int out_size;
    unsigned char *p;
    unsigned int x;
    unsigned int y;
    rl2RasterStylePtr xstyle = style;
    char *xsections;
    char *xxsections;
    char *sql;
    sqlite3_stmt *stmt = NULL;

    if (cvg == NULL || handle == NULL)
	return RL2_ERROR;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	return RL2_ERROR;
    coverage = rl2_get_coverage_name (cvg);
    if (coverage == NULL)
	return RL2_ERROR;

    *buffer = NULL;
    *buf_size = 0;
    if (pixel == RL2_PIXEL_GRAYSCALE && pixel_type == RL2_PIXEL_DATAGRID)
      {
	  if (rl2_has_styled_rgb_colors (style))
	    {
		/* RGB RasterSymbolizer: promoting to RGB */
		pixel = RL2_PIXEL_RGB;
	    }
      }
    if (pixel == RL2_PIXEL_GRAYSCALE)
      {
	  /* GRAYSCALE */
	  no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, bg_red);
      }
    else if (pixel == RL2_PIXEL_RGB)
      {
	  /* RGB */
	  *out_pixel = RL2_PIXEL_RGB;
	  no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, bg_red);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, bg_green);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, bg_blue);
      }
    else
	return RL2_ERROR;
    if (pixel_type == RL2_PIXEL_MONOCHROME)
	xstyle = NULL;

/* preparing the "sections" SQL query */
    xsections = sqlite3_mprintf ("%s_sections", coverage);
    xxsections = rl2_double_quoted_sql (xsections);
    sql =
	sqlite3_mprintf
	("SELECT section_id, MbrMinX(geometry), MbrMinY(geometry), "
	 "MbrMaxX(geometry), MbrMaxY(geometry) "
	 "FROM \"%s\" WHERE ROWID IN ( "
	 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	 "AND search_frame = BuildMBR(?, ?, ?, ?))", xxsections, xsections);
    sqlite3_free (xsections);
    free (xxsections);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT mixed-res Sections SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* allocating the output buffer */
    if (*out_pixel == RL2_PIXEL_RGB)
	out_size = width * height * 3;
    else
	out_size = width * height;
    outbuf = malloc (out_size);
    p = outbuf;
    for (y = 0; y < height; y++)
      {
	  /* priming the background color */
	  for (x = 0; x < width; x++)
	    {
		if (*out_pixel == RL2_PIXEL_RGB)
		  {
		      *p++ = bg_red;
		      *p++ = bg_green;
		      *p++ = bg_blue;
		  }
		else
		    *p++ = bg_red;
	    }
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, minx);
    sqlite3_bind_double (stmt, 2, miny);
    sqlite3_bind_double (stmt, 3, maxx);
    sqlite3_bind_double (stmt, 4, maxy);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 section_id = sqlite3_column_int64 (stmt, 0);
		double section_minx = sqlite3_column_double (stmt, 1);
		double section_miny = sqlite3_column_double (stmt, 2);
		double section_maxx = sqlite3_column_double (stmt, 3);
		double section_maxy = sqlite3_column_double (stmt, 4);
		double xx_res;
		double yy_res;
		int level_id;
		int scale;
		int xscale;
		unsigned int w;
		unsigned int h;
		int base_x;
		int base_y;
		unsigned char *bufpix = NULL;
		int bufpix_size;
		double img_res_x = (maxx - minx) / (double) width;
		double img_res_y = (maxy - miny) / (double) height;
		double mnx = minx;
		double mny = miny;
		double mxx = maxx;
		double mxy = maxy;
		if (section_id > 3)
		    continue;
		/* normalizing the visible portion of the Section */
		if (mnx < section_minx)
		    mnx = section_minx;
		if (mny < section_miny)
		    mny = section_miny;
		if (mxx > section_maxx)
		    mxx = section_maxx;
		if (mxy > section_maxy)
		    mxy = section_maxy;
		/* retrieving the optimal resolution level */
		if (!rl2_find_best_resolution_level
		    (handle, coverage, 1, section_id, x_res, y_res, &level_id,
		     &scale, &xscale, &xx_res, &yy_res))
		    goto error;
		w = (unsigned int) ((mxx - mnx) / xx_res);
		if (((double) w * xx_res) < (mxx - mnx))
		    w++;
		h = (unsigned int) ((mxy - mny) / yy_res);
		if (((double) h * yy_res) < (mxy - mny))
		    h++;
		base_x = (int) ((mnx - minx) / img_res_x);
		base_y = (int) ((maxy - mxy) / img_res_y);
		if (rl2_get_raw_raster_data_common
		    (handle, cvg, 1, section_id, w, h, mnx, mny, mxx, mxy,
		     xx_res, yy_res, &bufpix, &bufpix_size, palette, *out_pixel,
		     no_data, xstyle, stats) != RL2_OK)
		    goto error;
		if (*out_pixel == RL2_PIXEL_RGB)
		    do_copy_rgb (outbuf, bufpix, width, height, w, h, base_x,
				 base_y, bg_red, bg_green, bg_blue);
		else
		    do_copy_gray (outbuf, bufpix, width, height, w, h, base_x,
				  base_y, bg_red);
		free (bufpix);
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);

    if (no_data != NULL)
	rl2_destroy_pixel (no_data);

    *buffer = outbuf;
    *buf_size = out_size;
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (outbuf != NULL)
	free (outbuf);
    return RL2_ERROR;
}
