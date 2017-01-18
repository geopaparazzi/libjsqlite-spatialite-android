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

typedef struct rl2_multi_stroke_item
{
    rl2GraphicsPatternPtr pattern;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    double opacity;
    double width;
    int dash_count;
    double *dash_list;
    double dash_offset;
    int pen_cap;
    int pen_join;
    struct rl2_multi_stroke_item *next;
} rl2PrivMultiStrokeItem;
typedef rl2PrivMultiStrokeItem *rl2PrivMultiStrokeItemPtr;

typedef struct rl2_multi_stroke
{
    rl2PrivMultiStrokeItemPtr first;
    rl2PrivMultiStrokeItemPtr last;
} rl2PrivMultiStroke;
typedef rl2PrivMultiStroke *rl2PrivMultiStrokePtr;

static void
rl2_destroy_multi_stroke_item (rl2PrivMultiStrokeItemPtr item)
{
/* destroying a MultiStroke Item */
    if (item == NULL)
	return;
    if (item->pattern != NULL)
	rl2_graph_destroy_pattern (item->pattern);
    if (item->dash_list != NULL)
	free (item->dash_list);
    free (item);
}

static rl2PrivMultiStrokePtr
rl2_create_multi_stroke ()
{
/* creating an empty MultiStroke container */
    rl2PrivMultiStrokePtr multi = malloc (sizeof (rl2PrivMultiStroke));
    if (multi == NULL)
	return NULL;
    multi->first = NULL;
    multi->last = NULL;
    return multi;
}

static void
rl2_destroy_multi_stroke (rl2PrivMultiStrokePtr multi)
{
/* destroying a MultiStroke container */
    rl2PrivMultiStrokeItemPtr item;
    rl2PrivMultiStrokeItemPtr item_n;
    if (multi == NULL)
	return;
    item = multi->first;
    while (item != NULL)
      {
	  item_n = item->next;
	  rl2_destroy_multi_stroke_item (item);
	  item = item_n;
      }
    free (multi);
}

static void
rl2_add_pattern_to_multi_stroke (rl2PrivMultiStrokePtr multi,
				 rl2GraphicsPatternPtr pattern, double width,
				 int pen_cap, int pen_join)
{
/* adding a pattern-based stroke to the MultiStroke container */
    rl2PrivMultiStrokeItemPtr item;
    if (multi == NULL || pattern == NULL)
      {
	  rl2_graph_destroy_pattern (pattern);
	  return;
      }

    item = malloc (sizeof (rl2PrivMultiStrokeItem));
    item->pattern = pattern;
    item->width = width;
    item->pen_cap = pen_cap;
    item->pen_join = pen_join;
    item->dash_count = 0;
    item->dash_list = NULL;
    item->next = NULL;
    if (multi->first == NULL)
	multi->first = item;
    if (multi->last != NULL)
	multi->last->next = item;
    multi->last = item;
}

static void
rl2_add_pattern_to_multi_stroke_dash (rl2PrivMultiStrokePtr multi,
				      rl2GraphicsPatternPtr pattern,
				      double width, int pen_cap, int pen_join,
				      int dash_count, double *dash_list,
				      double dash_offset)
{
/* adding a pattern-based stroke (dash) to the MultiStroke container */
    int d;
    rl2PrivMultiStrokeItemPtr item;
    if (multi == NULL || pattern == NULL)
      {
	  rl2_graph_destroy_pattern (pattern);
	  return;
      }

    item = malloc (sizeof (rl2PrivMultiStrokeItem));
    item->pattern = pattern;
    item->width = width;
    item->pen_cap = pen_cap;
    item->pen_join = pen_join;
    item->dash_count = dash_count;
    item->dash_list = malloc (sizeof (double) * dash_count);
    for (d = 0; d < dash_count; d++)
	*(item->dash_list + d) = *(dash_list + d);
    item->dash_offset = dash_offset;
    item->next = NULL;
    if (multi->first == NULL)
	multi->first = item;
    if (multi->last != NULL)
	multi->last->next = item;
    multi->last = item;
}

static void
rl2_add_to_multi_stroke (rl2PrivMultiStrokePtr multi, unsigned char red,
			 unsigned char green, unsigned char blue,
			 double opacity, double width, int pen_cap,
			 int pen_join)
{
/* adding an RGB stroke to the MultiStroke container */
    rl2PrivMultiStrokeItemPtr item;
    if (multi == NULL)
	return;

    item = malloc (sizeof (rl2PrivMultiStrokeItem));
    item->pattern = NULL;
    item->red = red;
    item->green = green;
    item->blue = blue;
    item->opacity = opacity;
    item->width = width;
    item->pen_cap = pen_cap;
    item->pen_join = pen_join;
    item->dash_count = 0;
    item->dash_list = NULL;
    item->next = NULL;
    if (multi->first == NULL)
	multi->first = item;
    if (multi->last != NULL)
	multi->last->next = item;
    multi->last = item;
}

static void
rl2_add_to_multi_stroke_dash (rl2PrivMultiStrokePtr multi, unsigned char red,
			      unsigned char green, unsigned char blue,
			      double opacity, double width, int pen_cap,
			      int pen_join, int dash_count, double *dash_list,
			      double dash_offset)
{
/* adding an RGB stroke (dash) to the MultiStroke container */
    int d;
    rl2PrivMultiStrokeItemPtr item;
    if (multi == NULL)
	return;

    item = malloc (sizeof (rl2PrivMultiStrokeItem));
    item->pattern = NULL;
    item->red = red;
    item->green = green;
    item->blue = blue;
    item->opacity = opacity;
    item->width = width;
    item->pen_cap = pen_cap;
    item->pen_join = pen_join;
    item->dash_count = dash_count;
    item->dash_list = malloc (sizeof (double) * dash_count);
    for (d = 0; d < dash_count; d++)
	*(item->dash_list + d) = *(dash_list + d);
    item->dash_offset = dash_offset;
    item->next = NULL;
    if (multi->first == NULL)
	multi->first = item;
    if (multi->last != NULL)
	multi->last->next = item;
    multi->last = item;
}

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
	      unsigned char *inbuf, rl2PalettePtr palette,
	      unsigned char bg_red, unsigned char bg_green,
	      unsigned char bg_blue)
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
    int half_transparent;
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
		copy_monochrome (aggreg_rgba, aux->base_width,
				 aux->base_height, aux->outbuf);
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
		copy_grayscale (aggreg_rgba, aux->base_width,
				aux->base_height, aux->outbuf, aux->bg_red);
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
	  alpha = rl2_graph_get_context_alpha_array (ctx, &half_transparent);
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
    int half_transparent;
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
			   aux->srid, aux->outbuf, aux->format_id,
			   aux->quality, &image, &image_size))
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
			   aux->srid, aux->outbuf, aux->palette,
			   aux->format_id, aux->quality, &image, &image_size))
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
			   aux->srid, aux->outbuf, aux->format_id,
			   aux->quality, &image, &image_size))
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
			   aux->srid, aux->outbuf, aux->format_id,
			   aux->quality, &image, &image_size))
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
			  (aux->base_width, aux->base_height, aux->outbuf,
			   rgba, aux->bg_red))
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
			  (aux->base_width, aux->base_height, aux->outbuf,
			   rgba, aux->bg_red, aux->bg_green, aux->bg_blue))
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
	      alpha =
		  rl2_graph_get_context_alpha_array (ctx, &half_transparent);
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
			  (aux->width, aux->height, rgb, alpha,
			   aux->format_id, aux->quality, &image, &image_size,
			   aux->opacity))
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
			  (aux->width, aux->height, rgb, alpha,
			   aux->format_id, aux->quality, &image, &image_size,
			   aux->opacity, 0))
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
	       aux->quality, &image, &image_size, 1.0, 0))
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
	(aux->sqlite, aux->max_threads, aux->coverage, relief_factor,
	 scale_factor, aux->base_width, aux->base_height, aux->minx,
	 aux->miny, aux->maxx, aux->maxy, aux->xx_res, aux->yy_res, &shr_mask,
	 &shr_size) != RL2_OK)
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
    rl2PrivRasterSymbolizerPtr symbolizer = NULL;
    int by_section = 0;
    // Placeholder for final geometry-cutline (simplified(XY) and cut, if valid as cutline)
    // is not used in this function, can be NULL
    unsigned char **geom_cutline_blob=malloc(sizeof(unsigned char *));
    *geom_cutline_blob=NULL;
    int *geom_cutline_blob_sz=malloc(sizeof(int));
    *geom_cutline_blob_sz=0;
    sqlite3 *sqlite = sqlite3_context_db_handle (auxgrp->context);
    int max_threads = 1;
    const void *data = sqlite3_user_data (auxgrp->context);
    if (data != NULL)
      {
	  struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
	  max_threads = priv_data->max_threads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
      }

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
		int categorize;
		int interpolate;
		if (lyr->raster_symbolizer->shadedRelief)
		    is_shaded_relief = 1;
		if (!is_shaded_relief)
		  {
		      if (rl2_is_raster_symbolizer_triple_band_selected
			  ((rl2RasterSymbolizerPtr) (lyr->raster_symbolizer),
			   &yes_no) == RL2_OK)
			{
			    if ((cvg->sampleType == RL2_SAMPLE_UINT8
				 || cvg->sampleType == RL2_SAMPLE_UINT16)
				&& (cvg->pixelType == RL2_PIXEL_RGB
				    || cvg->pixelType == RL2_PIXEL_MULTIBAND)
				&& yes_no)
				out_pixel = RL2_PIXEL_RGB;
			}
		      if (rl2_is_raster_symbolizer_mono_band_selected
			  ((rl2RasterSymbolizerPtr) (lyr->raster_symbolizer),
			   &yes_no, &categorize, &interpolate) == RL2_OK)
			{
			    if ((cvg->sampleType == RL2_SAMPLE_UINT8
				 || cvg->sampleType == RL2_SAMPLE_UINT16)
				&& (cvg->pixelType == RL2_PIXEL_RGB
				    || cvg->pixelType == RL2_PIXEL_MULTIBAND
				    || cvg->pixelType == RL2_PIXEL_GRAYSCALE)
				&& yes_no)
				out_pixel = RL2_PIXEL_GRAYSCALE;
			    if ((cvg->sampleType == RL2_SAMPLE_UINT8
				 || cvg->sampleType == RL2_SAMPLE_UINT16)
				&& cvg->pixelType == RL2_PIXEL_MULTIBAND
				&& yes_no && (categorize || interpolate))
				out_pixel = RL2_PIXEL_RGB;
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
		      if (rl2_get_raster_symbolizer_opacity
			  ((rl2RasterSymbolizerPtr) (lyr->raster_symbolizer),
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
		    (sqlite, max_threads, lyr->coverage, base_width,
		     base_height, auxgrp->minx, auxgrp->miny, auxgrp->maxx,
		     auxgrp->maxy, xx_res, yy_res, &outbuf, &outbuf_size,
		     &palette, &out_pixel, auxgrp->bg_red, auxgrp->bg_green,
		     auxgrp->bg_blue, (rl2RasterSymbolizerPtr) symbolizer,
		     (rl2RasterStatisticsPtr) (lyr->raster_stats),*geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
		    goto error;
		if (out_pixel == RL2_PIXEL_PALETTE && palette == NULL)
		    goto error;
		if (was_monochrome && out_pixel == RL2_PIXEL_GRAYSCALE)
		    symbolizer = NULL;
	    }

/* preparing the aux struct for passing rendering arguments */
	  aux.sqlite = sqlite;
	  aux.max_threads = max_threads;
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
	  aux.symbolizer = (rl2RasterSymbolizerPtr) symbolizer;
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
     if (geom_cutline_blob != NULL)
    {
     if (*geom_cutline_blob != NULL)
      free(*geom_cutline_blob);		
     free(geom_cutline_blob);		  
    }
    if (geom_cutline_blob_sz != NULL)
     free(geom_cutline_blob_sz);
    sqlite3_result_blob (auxgrp->context, image, image_size, free);
    rl2_destroy_group_style (group_style);
    rl2_destroy_group_renderer (group);
    return;

  error:
    if (geom_cutline_blob != NULL)
    {
     if (*geom_cutline_blob != NULL)
      free(*geom_cutline_blob);		
     free(geom_cutline_blob);		  
    }
    if (geom_cutline_blob_sz != NULL)
     free(geom_cutline_blob_sz);		
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
rl2_get_raw_raster_data_mixed_resolutions (sqlite3 * handle, int max_threads,
					   rl2CoveragePtr cvg,
					   unsigned int width,
					   unsigned int height, double minx,
					   double miny, double maxx,
					   double maxy, double x_res,
					   double y_res,
					   unsigned char **buffer,
					   int *buf_size,
					   rl2PalettePtr * palette,
					   unsigned char *out_pixel,
					   unsigned char bg_red,
					   unsigned char bg_green,
					   unsigned char bg_blue,
					   rl2RasterSymbolizerPtr style,
					   rl2RasterStatisticsPtr stats,
        unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
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
    rl2RasterSymbolizerPtr xstyle = style;
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
		    (handle, max_threads, cvg, 1, section_id, w, h, mnx, mny,
		     mxx, mxy, xx_res, yy_res, &bufpix, &bufpix_size, palette,
		     *out_pixel, no_data, xstyle, stats,geom_cutline_blob,geom_cutline_blob_sz) != RL2_OK)
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

static int
point_bbox_matches (rl2PointPtr point, double minx, double miny, double maxx,
		    double maxy)
{
/* checks if the Point is visible */
    if (minx > point->x)
	return 0;
    if (maxx < point->x)
	return 0;
    if (miny > point->y)
	return 0;
    if (maxy < point->y)
	return 0;
    return 1;
}

static void
draw_points (rl2GraphicsContextPtr ctx, sqlite3 * handle,
	     rl2PrivVectorSymbolizerPtr sym, int height, double minx,
	     double miny, double maxx, double maxy, double x_res,
	     double y_res, rl2GeometryPtr geom)
{
/* drawing Point-type features */
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PointPtr point;

    item = sym->first;
    while (item != NULL)
      {
	  if (item->symbolizer_type == RL2_POINT_SYMBOLIZER)
	    {
		rl2PrivPointSymbolizerPtr point_sym =
		    (rl2PrivPointSymbolizerPtr) (item->symbolizer);
		rl2PrivGraphicPtr gr = point_sym->graphic;
		rl2PrivGraphicItemPtr graphic;
		if (gr == NULL)
		  {
		      item = item->next;
		      continue;
		  }
		graphic = point_sym->graphic->first;
		while (graphic != NULL)
		  {
		      /* looping on Graphic definitions */
		      int is_mark = 0;
		      int is_external = 0;
		      unsigned char well_known_type;
		      int fill = 0;
		      int stroke = 0;
		      int pen_cap;
		      int pen_join;
		      double opacity;
		      unsigned char norm_opacity;
		      rl2GraphicsPatternPtr pattern = NULL;
		      rl2GraphicsPatternPtr pattern_fill = NULL;
		      rl2GraphicsPatternPtr pattern_stroke = NULL;

		      if (graphic->type == RL2_MARK_GRAPHIC)
			{
			    rl2PrivMarkPtr mark =
				(rl2PrivMarkPtr) (graphic->item);
			    if (mark != NULL)
			      {
				  well_known_type = mark->well_known_type;
				  is_mark = 1;
				  if (mark->fill != NULL)
				    {
					if (mark->fill->graphic != NULL)
					  {
					      /* external Graphic fill */
					      const char *xlink_href = NULL;
					      int recolor = 0;
					      unsigned char red;
					      unsigned char green;
					      unsigned char blue;
					      pattern_fill = NULL;
					      if (mark->fill->graphic->first !=
						  NULL)
						{
						    if (mark->fill->
							graphic->first->type ==
							RL2_EXTERNAL_GRAPHIC)
						      {
							  rl2PrivExternalGraphicPtr
							      ext =
							      (rl2PrivExternalGraphicPtr)
							      (mark->
							       fill->graphic->
							       first->item);
							  xlink_href =
							      ext->xlink_href;
							  if (ext->first !=
							      NULL)
							    {
								recolor = 1;
								red =
								    ext->
								    first->red;
								green =
								    ext->
								    first->green;
								blue =
								    ext->
								    first->blue;
							    }
						      }
						}
					      if (xlink_href != NULL)
						  pattern_fill =
						      rl2_create_pattern_from_external_graphic
						      (handle, xlink_href, 1);
					      if (pattern_fill != NULL)
						{
						    if (recolor)
						      {
							  /* attempting to recolor the External Graphic resource */
							  rl2_graph_pattern_recolor
							      (pattern_fill,
							       red, green,
							       blue);
						      }
						    if (mark->fill->opacity <=
							0.0)
							norm_opacity = 0;
						    else if (mark->
							     fill->opacity >=
							     1.0)
							norm_opacity = 255;
						    else
						      {
							  opacity =
							      255.0 *
							      mark->
							      fill->opacity;
							  if (opacity <= 0.0)
							      norm_opacity = 0;
							  else if (opacity >=
								   255.0)
							      norm_opacity =
								  255;
							  else
							      norm_opacity =
								  opacity;
						      }
						    if (norm_opacity < 1.0)
							rl2_graph_pattern_transparency
							    (pattern_fill,
							     norm_opacity);
						    rl2_graph_set_pattern_brush
							(ctx, pattern_fill);
						}
					      else
						{
						    /* invalid Pattern: defaulting to a Gray brush */
						    rl2_graph_set_brush (ctx,
									 128,
									 128,
									 128,
									 255);
						}
					      fill = 1;
					  }
					else
					  {
					      /* solid RGB fill */
					      if (gr->opacity <= 0.0)
						  norm_opacity = 0;
					      else if (gr->opacity >= 1.0)
						  norm_opacity = 255;
					      else
						{
						    opacity =
							255.0 * gr->opacity;
						    if (opacity <= 0.0)
							norm_opacity = 0;
						    else if (opacity >= 255.0)
							norm_opacity = 255;
						    else
							norm_opacity = opacity;
						}
					      rl2_graph_set_brush (ctx,
								   mark->
								   fill->red,
								   mark->
								   fill->green,
								   mark->
								   fill->blue,
								   norm_opacity);
					      fill = 1;
					  }
				    }
				  if (mark->stroke != NULL)
				    {
					if (mark->stroke->graphic != NULL)
					  {
					      const char *xlink_href = NULL;
					      int recolor = 0;
					      unsigned char red;
					      unsigned char green;
					      unsigned char blue;
					      pattern_stroke = NULL;
					      if (mark->stroke->
						  graphic->first != NULL)
						{
						    if (mark->stroke->
							graphic->first->type ==
							RL2_EXTERNAL_GRAPHIC)
						      {
							  rl2PrivExternalGraphicPtr
							      ext =
							      (rl2PrivExternalGraphicPtr)
							      (mark->
							       stroke->graphic->
							       first->item);
							  xlink_href =
							      ext->xlink_href;
							  if (ext->first !=
							      NULL)
							    {
								recolor = 1;
								red =
								    ext->
								    first->red;
								green =
								    ext->
								    first->green;
								blue =
								    ext->
								    first->blue;
							    }
						      }
						}
					      if (xlink_href != NULL)
						  pattern_stroke =
						      rl2_create_pattern_from_external_graphic
						      (handle, xlink_href, 1);
					      if (pattern != NULL)
						{
						    if (recolor)
						      {
							  /* attempting to recolor the External Graphic resource */
							  rl2_graph_pattern_recolor
							      (pattern_stroke,
							       red, green,
							       blue);
						      }
						    if (mark->stroke->opacity <=
							0.0)
							norm_opacity = 0;
						    else if (mark->
							     stroke->opacity >=
							     1.0)
							norm_opacity = 255;
						    else
						      {
							  opacity =
							      255.0 *
							      mark->
							      stroke->opacity;
							  if (opacity <= 0.0)
							      norm_opacity = 0;
							  else if (opacity >=
								   255.0)
							      norm_opacity =
								  255;
							  else
							      norm_opacity =
								  opacity;
						      }
						    if (norm_opacity < 1.0)
							rl2_graph_pattern_transparency
							    (pattern_stroke,
							     norm_opacity);
						    if (pattern_stroke != NULL)
						      {
							  switch
							      (mark->
							       stroke->linecap)
							    {
							    case RL2_STROKE_LINECAP_ROUND:
								pen_cap =
								    RL2_PEN_CAP_ROUND;
								break;
							    case RL2_STROKE_LINECAP_SQUARE:
								pen_cap =
								    RL2_PEN_CAP_SQUARE;
								break;
							    default:
								pen_cap =
								    RL2_PEN_CAP_BUTT;
								break;
							    };
							  switch
							      (mark->
							       stroke->linejoin)
							    {
							    case RL2_STROKE_LINEJOIN_BEVEL:
								pen_join =
								    RL2_PEN_JOIN_BEVEL;
								break;
							    case RL2_STROKE_LINEJOIN_ROUND:
								pen_join =
								    RL2_PEN_JOIN_ROUND;
								break;
							    default:
								pen_join =
								    RL2_PEN_JOIN_MITER;
								break;
							    };
							  if (mark->
							      stroke->dash_count
							      > 0
							      && mark->
							      stroke->dash_list
							      != NULL)
							      rl2_graph_set_pattern_dashed_pen
								  (ctx,
								   pattern_stroke,
								   mark->
								   stroke->width,
								   pen_cap,
								   pen_join,
								   mark->
								   stroke->dash_count,
								   mark->
								   stroke->dash_list,
								   mark->
								   stroke->dash_offset);
							  else
							      rl2_graph_set_pattern_solid_pen
								  (ctx,
								   pattern_stroke,
								   mark->
								   stroke->width,
								   pen_cap,
								   pen_join);
							  stroke = 1;
						      }
						}
					  }
					else
					  {
					      /* solid RGB stroke */
					      if (gr->opacity <= 0.0)
						  norm_opacity = 0;
					      else if (gr->opacity >= 1.0)
						  norm_opacity = 255;
					      else
						{
						    opacity =
							255.0 * gr->opacity;
						    if (opacity <= 0.0)
							norm_opacity = 0;
						    else if (opacity >= 255.0)
							norm_opacity = 255;
						    else
							norm_opacity = opacity;
						}
					      switch (mark->stroke->linecap)
						{
						case RL2_STROKE_LINECAP_ROUND:
						    pen_cap = RL2_PEN_CAP_ROUND;
						    break;
						case RL2_STROKE_LINECAP_SQUARE:
						    pen_cap =
							RL2_PEN_CAP_SQUARE;
						    break;
						default:
						    pen_cap = RL2_PEN_CAP_BUTT;
						    break;
						};
					      switch (mark->stroke->linejoin)
						{
						case RL2_STROKE_LINEJOIN_BEVEL:
						    pen_join =
							RL2_PEN_JOIN_BEVEL;
						    break;
						case RL2_STROKE_LINEJOIN_ROUND:
						    pen_join =
							RL2_PEN_JOIN_ROUND;
						    break;
						default:
						    pen_join =
							RL2_PEN_JOIN_MITER;
						    break;
						};
					      if (mark->stroke->dash_count > 0
						  && mark->stroke->dash_list !=
						  NULL)
						  rl2_graph_set_dashed_pen (ctx,
									    mark->stroke->red,
									    mark->stroke->green,
									    mark->stroke->blue,
									    norm_opacity,
									    mark->stroke->width,
									    pen_cap,
									    pen_join,
									    mark->
									    stroke->dash_count,
									    mark->
									    stroke->dash_list,
									    mark->
									    stroke->dash_offset);
					      else
						  rl2_graph_set_solid_pen
						      (ctx, mark->stroke->red,
						       mark->stroke->green,
						       mark->stroke->blue,
						       norm_opacity,
						       mark->stroke->width,
						       pen_cap, pen_join);
					      stroke = 1;
					  }
				    }
			      }
			}
		      if (graphic->type == RL2_EXTERNAL_GRAPHIC)
			{
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (graphic->item);
			    const char *xlink_href = NULL;
			    int recolor = 0;
			    unsigned char red;
			    unsigned char green;
			    unsigned char blue;
			    pattern = NULL;
			    if (ext != NULL)
			      {
				  is_external = 1;
				  if (ext->xlink_href != NULL)
				      xlink_href = ext->xlink_href;
				  if (ext->first != NULL)
				    {
					recolor = 1;
					red = ext->first->red;
					green = ext->first->green;
					blue = ext->first->blue;
				    }
				  if (xlink_href != NULL)
				    {
					/* first attempt: Bitmap */
					pattern =
					    rl2_create_pattern_from_external_graphic
					    (handle, xlink_href, 1);
					if (pattern == NULL)
					  {
					      /* second attempt: SVG */
					      pattern =
						  rl2_create_pattern_from_external_svg
						  (handle, xlink_href,
						   point_sym->graphic->size);
					  }
				    }
			      }
			    if (pattern != NULL)
			      {
				  if (recolor)
				    {
					/* attempting to recolor the External Graphic resource */
					rl2_graph_pattern_recolor (pattern,
								   red, green,
								   blue);
				    }
				  if (point_sym->graphic->opacity <= 0.0)
				      norm_opacity = 0;
				  else if (point_sym->graphic->opacity >= 1.0)
				      norm_opacity = 255;
				  else
				    {
					opacity =
					    255.0 * point_sym->graphic->opacity;
					if (opacity <= 0.0)
					    norm_opacity = 0;
					else if (opacity >= 255.0)
					    norm_opacity = 255;
					else
					    norm_opacity = opacity;
				    }
				  if (norm_opacity < 1.0)
				      rl2_graph_pattern_transparency
					  (pattern, norm_opacity);
				  rl2_graph_set_pattern_brush (ctx, pattern);
			      }
			    else
			      {
				  /* invalid Pattern: defaulting to a Gray brush */
				  rl2_graph_set_brush (ctx, 128, 128, 128, 255);
			      }
			}

		      /* actual Point rendering */
		      point = geom->first_point;
		      while (point)
			{
			    /* drawing a POINT */
			    if (point_bbox_matches
				(point, minx, miny, maxx, maxy))
			      {
				  double x = (point->x - minx) / x_res;
				  double y =
				      (double) height -
				      ((point->y - miny) / y_res);
				  double size2 = gr->size / 2.0;
				  double size4 = gr->size / 4.0;
				  double size6 = gr->size / 6.0;
				  double size13 = gr->size / 3.0;
				  double size23 = (gr->size / 3.0) * 2.0;
				  int i;
				  double rads;
				  if (size2 <= 0.0)
				      size2 = 1.0;
				  if (size4 <= 0.0)
				      size4 = 1.0;
				  if (size6 <= 0.0)
				      size6 = 1.0;
				  if (size13 <= 0.0)
				      size13 = 1.0;
				  if (size23 <= 0.0)
				      size23 = 1.0;
				  if (is_mark)
				    {
					/* drawing a well-known Mark */
					switch (well_known_type)
					  {
					  case RL2_GRAPHIC_MARK_CIRCLE:
					      rads = 0.0;
					      for (i = 0; i < 32; i++)
						{
						    double tic =
							6.28318530718 / 32.0;
						    double cx =
							x +
							(size2 * sin (rads));
						    double cy =
							y +
							(size2 * cos (rads));
						    if (i == 0)
							rl2_graph_move_to_point
							    (ctx, cx, cy);
						    else
							rl2_graph_add_line_to_path
							    (ctx, cx, cy);
						    rads += tic;
						}
					      rl2_graph_close_subpath (ctx);
					      break;
					  case RL2_GRAPHIC_MARK_TRIANGLE:
					      rl2_graph_move_to_point (ctx, x,
								       y -
								       size23);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size2,
									  y +
									  size13);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size2,
									  y +
									  size13);
					      rl2_graph_close_subpath (ctx);
					      break;
					  case RL2_GRAPHIC_MARK_STAR:
					      rads = 3.14159265359;
					      for (i = 0; i < 10; i++)
						{
						    double tic =
							(i % 2) ? size6 : size2;
						    double cx =
							x + (tic * sin (rads));
						    double cy =
							y + (tic * cos (rads));
						    if (i == 0)
							rl2_graph_move_to_point
							    (ctx, cx, cy);
						    else
							rl2_graph_add_line_to_path
							    (ctx, cx, cy);
						    rads += 0.628318530718;
						}
					      rl2_graph_close_subpath (ctx);
					      break;
					  case RL2_GRAPHIC_MARK_CROSS:
					      rl2_graph_move_to_point (ctx,
								       x -
								       size6,
								       y -
								       size2);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size6,
									  y -
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size6,
									  y -
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size2,
									  y -
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size2,
									  y +
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size6,
									  y +
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size6,
									  y +
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size6,
									  y +
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size6,
									  y +
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size2,
									  y +
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size2,
									  y -
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size6,
									  y -
									  size6);
					      rl2_graph_close_subpath (ctx);
					      break;
					  case RL2_GRAPHIC_MARK_X:
					      rl2_graph_move_to_point (ctx,
								       x -
								       size2,
								       y -
								       size2);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size4,
									  y -
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x,
									  y -
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size6,
									  y -
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size2,
									  y -
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size4,
									  y);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size2,
									  y +
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size4,
									  y +
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x,
									  y +
									  size6);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size6,
									  y +
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size2,
									  y +
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size4,
									  y);
					      rl2_graph_close_subpath (ctx);
					      break;
					  case RL2_GRAPHIC_MARK_SQUARE:
					  default:
					      rl2_graph_move_to_point (ctx,
								       x -
								       size2,
								       y -
								       size2);
					      rl2_graph_add_line_to_path (ctx,
									  x -
									  size2,
									  y +
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size2,
									  y +
									  size2);
					      rl2_graph_add_line_to_path (ctx,
									  x +
									  size2,
									  y -
									  size2);
					      rl2_graph_close_subpath (ctx);
					      break;
					  };
					if (fill)
					  {
					      if (stroke)
						  rl2_graph_fill_path (ctx,
								       RL2_PRESERVE_PATH);
					      else
						  rl2_graph_fill_path (ctx,
								       RL2_CLEAR_PATH);
					  }
					if (stroke)
					    rl2_graph_stroke_path (ctx,
								   RL2_CLEAR_PATH);
				    }
				  if (is_external && pattern != NULL)
				    {
					/* drawing an External Graphic pattern */
					unsigned int width;
					unsigned int height;
					rl2_graph_get_pattern_size (pattern,
								    &width,
								    &height);
					double out_width = width;
					double out_height = height;
					rl2_graph_draw_graphic_symbol (ctx,
								       pattern,
								       out_width,
								       out_height,
								       x +
								       point_sym->graphic->displacement_x,
								       y -
								       point_sym->graphic->displacement_y,
								       point_sym->graphic->rotation,
								       point_sym->graphic->anchor_point_x,
								       point_sym->graphic->anchor_point_y);
				    }
			      }
			    point = point->next;
			}

		      /* releasing Patterns */
		      if (pattern != NULL)
			  rl2_graph_destroy_pattern (pattern);
		      if (pattern_fill != NULL)
			{
			    rl2_graph_release_pattern_pen (ctx);
			    rl2_graph_destroy_pattern (pattern_fill);
			}
		      if (pattern_stroke != NULL)
			{
			    rl2_graph_release_pattern_pen (ctx);
			    rl2_graph_destroy_pattern (pattern_stroke);
			}
		      graphic = graphic->next;
		  }
	    }
	  item = item->next;
      }
}

static int
linestring_bbox_matches (rl2LinestringPtr ring, double minx, double miny,
			 double maxx, double maxy)
{
/* checks if the Linestring BBOX is visible */
    if (minx > ring->maxx)
	return 0;
    if (maxx < ring->minx)
	return 0;
    if (miny > ring->maxy)
	return 0;
    if (maxy < ring->miny)
	return 0;
    return 1;
}

static void
draw_lines (rl2GraphicsContextPtr ctx, sqlite3 * handle,
	    rl2PrivVectorSymbolizerPtr sym, int height, double minx,
	    double miny, double maxx, double maxy, double x_res, double y_res,
	    rl2GeometryPtr geom)
{
/* drawing Linear-type features */
    rl2PrivVectorSymbolizerItemPtr item;
    int pen_cap;
    int pen_join;
    double opacity;
    unsigned char norm_opacity;
    rl2LinestringPtr line;
    rl2GraphicsPatternPtr pattern = NULL;
    rl2PrivMultiStrokePtr multi_stroke = rl2_create_multi_stroke ();

    item = sym->first;
    while (item != NULL)
      {
	  if (item->symbolizer_type == RL2_LINE_SYMBOLIZER)
	    {
		rl2PrivLineSymbolizerPtr line_sym =
		    (rl2PrivLineSymbolizerPtr) (item->symbolizer);
		if (line_sym->stroke != NULL)
		  {
		      if (line_sym->stroke->graphic != NULL)
			{
			    /* external Graphic stroke */
			    const char *xlink_href = NULL;
			    int recolor = 0;
			    unsigned char red;
			    unsigned char green;
			    unsigned char blue;
			    pattern = NULL;
			    if (line_sym->stroke->graphic->first != NULL)
			      {
				  if (line_sym->stroke->graphic->first->type ==
				      RL2_EXTERNAL_GRAPHIC)
				    {
					rl2PrivExternalGraphicPtr ext =
					    (rl2PrivExternalGraphicPtr)
					    (line_sym->stroke->graphic->
					     first->item);
					xlink_href = ext->xlink_href;
					if (ext->first != NULL)
					  {
					      recolor = 1;
					      red = ext->first->red;
					      green = ext->first->green;
					      blue = ext->first->blue;
					  }
				    }
			      }
			    if (xlink_href != NULL)
				pattern =
				    rl2_create_pattern_from_external_graphic
				    (handle, xlink_href, 1);
			    if (pattern != NULL)
			      {
				  if (recolor)
				    {
					/* attempting to recolor the External Graphic resource */
					rl2_graph_pattern_recolor (pattern,
								   red, green,
								   blue);
				    }
				  if (line_sym->stroke->opacity <= 0.0)
				      norm_opacity = 0;
				  else if (line_sym->stroke->opacity >= 1.0)
				      norm_opacity = 255;
				  else
				    {
					opacity =
					    255.0 * line_sym->stroke->opacity;
					if (opacity <= 0.0)
					    norm_opacity = 0;
					else if (opacity >= 255.0)
					    norm_opacity = 255;
					else
					    norm_opacity = opacity;
				    }
				  if (norm_opacity < 1.0)
				      rl2_graph_pattern_transparency
					  (pattern, norm_opacity);
				  if (line_sym->stroke->dash_count > 0
				      && line_sym->stroke->dash_list != NULL)
				      rl2_add_pattern_to_multi_stroke_dash
					  (multi_stroke, pattern,
					   line_sym->stroke->width, pen_cap,
					   pen_join,
					   line_sym->stroke->dash_count,
					   line_sym->stroke->dash_list,
					   line_sym->stroke->dash_offset);
				  else
				      rl2_add_pattern_to_multi_stroke
					  (multi_stroke, pattern,
					   line_sym->stroke->width, pen_cap,
					   pen_join);
			      }
			    else
			      {
				  /* invalid Pattern: defaulting to a Gray brush */
				  rl2_graph_set_brush (ctx, 128, 128, 128, 255);
			      }
			}
		      else
			{
			    /* solid RGB stroke */
			    if (line_sym->stroke->opacity <= 0.0)
				norm_opacity = 0;
			    else if (line_sym->stroke->opacity >= 1.0)
				norm_opacity = 255;
			    else
			      {
				  opacity = 255.0 * line_sym->stroke->opacity;
				  if (opacity <= 0.0)
				      norm_opacity = 0;
				  else if (opacity >= 255.0)
				      norm_opacity = 255;
				  else
				      norm_opacity = opacity;
			      }
			    switch (line_sym->stroke->linecap)
			      {
			      case RL2_STROKE_LINECAP_ROUND:
				  pen_cap = RL2_PEN_CAP_ROUND;
				  break;
			      case RL2_STROKE_LINECAP_SQUARE:
				  pen_cap = RL2_PEN_CAP_SQUARE;
				  break;
			      default:
				  pen_cap = RL2_PEN_CAP_BUTT;
				  break;
			      };
			    switch (line_sym->stroke->linejoin)
			      {
			      case RL2_STROKE_LINEJOIN_BEVEL:
				  pen_join = RL2_PEN_JOIN_BEVEL;
				  break;
			      case RL2_STROKE_LINEJOIN_ROUND:
				  pen_join = RL2_PEN_JOIN_ROUND;
				  break;
			      default:
				  pen_join = RL2_PEN_JOIN_MITER;
				  break;
			      };
			    if (line_sym->stroke->dash_count > 0
				&& line_sym->stroke->dash_list != NULL)
				rl2_add_to_multi_stroke_dash (multi_stroke,
							      line_sym->
							      stroke->red,
							      line_sym->
							      stroke->green,
							      line_sym->
							      stroke->blue,
							      norm_opacity,
							      line_sym->
							      stroke->width,
							      pen_cap, pen_join,
							      line_sym->
							      stroke->dash_count,
							      line_sym->
							      stroke->dash_list,
							      line_sym->
							      stroke->dash_offset);
			    else
				rl2_add_to_multi_stroke (multi_stroke,
							 line_sym->stroke->red,
							 line_sym->
							 stroke->green,
							 line_sym->stroke->blue,
							 norm_opacity,
							 line_sym->
							 stroke->width, pen_cap,
							 pen_join);
			}
		  }
	    }
	  item = item->next;
      }
    if (multi_stroke == NULL)
	return;

    line = geom->first_linestring;
    while (line)
      {
	  /* drawing a LINESTRING */
	  if (linestring_bbox_matches (line, minx, miny, maxx, maxy))
	    {
		rl2PrivMultiStrokeItemPtr stroke_item;
		int iv;
		double dx;
		double dy;
		int x;
		int y;
		int lastX = 0;
		int lastY = 0;
		for (iv = 0; iv < line->points; iv++)
		  {
		      rl2GetPoint (line->coords, iv, &dx, &dy);
		      x = (int) ((dx - minx) / x_res);
		      y = height - (int) ((dy - miny) / y_res);
		      if (iv == 0)
			{
			    rl2_graph_move_to_point (ctx, x, y);
			    lastX = x;
			    lastY = y;
			}
		      else
			{
			    if (x == lastX && y == lastY)
				;
			    else
			      {
				  rl2_graph_add_line_to_path (ctx, x, y);
				  lastX = x;
				  lastY = y;
			      }
			}
		  }
		stroke_item = multi_stroke->first;
		while (stroke_item != NULL)
		  {
		      /* applying all strokes, one after the other */
		      if (stroke_item->dash_count > 0
			  && stroke_item->dash_list != NULL)
			{
			    if (stroke_item->pattern != NULL)
				rl2_graph_set_pattern_dashed_pen (ctx,
								  stroke_item->pattern,
								  stroke_item->width,
								  stroke_item->pen_cap,
								  stroke_item->pen_join,
								  stroke_item->dash_count,
								  stroke_item->dash_list,
								  stroke_item->dash_offset);
			    else
				rl2_graph_set_dashed_pen (ctx,
							  stroke_item->red,
							  stroke_item->green,
							  stroke_item->blue,
							  stroke_item->opacity,
							  stroke_item->width,
							  stroke_item->pen_cap,
							  stroke_item->pen_join,
							  stroke_item->dash_count,
							  stroke_item->dash_list,
							  stroke_item->dash_offset);
			}
		      else
			{
			    if (stroke_item->pattern != NULL)
				rl2_graph_set_pattern_solid_pen (ctx,
								 stroke_item->pattern,
								 stroke_item->width,
								 stroke_item->pen_cap,
								 stroke_item->pen_join);
			    else
				rl2_graph_set_solid_pen (ctx,
							 stroke_item->red,
							 stroke_item->green,
							 stroke_item->blue,
							 stroke_item->opacity,
							 stroke_item->width,
							 stroke_item->pen_cap,
							 stroke_item->pen_join);
			}

		      if (stroke_item == multi_stroke->last)
			  rl2_graph_stroke_path (ctx, RL2_CLEAR_PATH);
		      else
			  rl2_graph_stroke_path (ctx, RL2_PRESERVE_PATH);

		      stroke_item = stroke_item->next;
		      if (stroke_item == multi_stroke->last)
			  rl2_graph_release_pattern_pen (ctx);
		  }
	    }
	  line = line->next;
      }
    rl2_destroy_multi_stroke (multi_stroke);
}

static int
ring_bbox_matches (rl2RingPtr ring, double minx, double miny, double maxx,
		   double maxy)
{
/* checks if the Ring BBOX is visible */
    if (minx > ring->maxx)
	return 0;
    if (maxx < ring->minx)
	return 0;
    if (miny > ring->maxy)
	return 0;
    if (maxy < ring->miny)
	return 0;
    return 1;
}

static void
draw_polygons (rl2GraphicsContextPtr ctx, sqlite3 * handle,
	       rl2PrivVectorSymbolizerPtr sym, int height, double minx,
	       double miny, double maxx, double maxy, double x_res,
	       double y_res, rl2GeometryPtr geom)
{
/* drawing Polygonal-type features */
    rl2PrivVectorSymbolizerItemPtr item;
    int stroke = 0;
    int fill = 0;
    int pen_cap;
    int pen_join;
    double opacity;
    unsigned char norm_opacity;
    rl2PolygonPtr polyg;
    rl2GraphicsPatternPtr pattern_fill = NULL;
    rl2GraphicsPatternPtr pattern_stroke = NULL;

/* selecting the pen and brush requested by the current PolygonSymbolizer */
    item = sym->first;
    while (item != NULL)
      {
	  stroke = 0;
	  fill = 0;
	  if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER)
	    {
		rl2PrivPolygonSymbolizerPtr polyg_sym =
		    (rl2PrivPolygonSymbolizerPtr) (item->symbolizer);
		if (polyg_sym->fill != NULL)
		  {
		      if (polyg_sym->fill->graphic != NULL)
			{
			    /* external Graphic fill */
			    const char *xlink_href = NULL;
			    int recolor = 0;
			    unsigned char red;
			    unsigned char green;
			    unsigned char blue;
			    pattern_fill = NULL;
			    if (polyg_sym->fill->graphic->first != NULL)
			      {
				  if (polyg_sym->fill->graphic->first->type ==
				      RL2_EXTERNAL_GRAPHIC)
				    {
					rl2PrivExternalGraphicPtr ext =
					    (rl2PrivExternalGraphicPtr)
					    (polyg_sym->fill->graphic->
					     first->item);
					xlink_href = ext->xlink_href;
					if (ext->first != NULL)
					  {
					      recolor = 1;
					      red = ext->first->red;
					      green = ext->first->green;
					      blue = ext->first->blue;
					  }
				    }
			      }
			    if (xlink_href != NULL)
				pattern_fill =
				    rl2_create_pattern_from_external_graphic
				    (handle, xlink_href, 1);
			    if (pattern_fill != NULL)
			      {
				  if (recolor)
				    {
					/* attempting to recolor the External Graphic resource */
					rl2_graph_pattern_recolor
					    (pattern_fill, red, green, blue);
				    }
				  if (polyg_sym->fill->opacity <= 0.0)
				      norm_opacity = 0;
				  else if (polyg_sym->fill->opacity >= 1.0)
				      norm_opacity = 255;
				  else
				    {
					opacity =
					    255.0 * polyg_sym->fill->opacity;
					if (opacity <= 0.0)
					    norm_opacity = 0;
					else if (opacity >= 255.0)
					    norm_opacity = 255;
					else
					    norm_opacity = opacity;
				    }
				  if (norm_opacity < 1.0)
				      rl2_graph_pattern_transparency
					  (pattern_fill, norm_opacity);
				  rl2_graph_set_pattern_brush (ctx,
							       pattern_fill);
			      }
			    else
			      {
				  /* invalid Pattern: defaulting to a Gray brush */
				  rl2_graph_set_brush (ctx, 128, 128, 128, 255);
			      }
			    fill = 1;
			}
		      else
			{
			    /* solid RGB fill */
			    if (polyg_sym->fill->opacity <= 0.0)
				norm_opacity = 0;
			    else if (polyg_sym->fill->opacity >= 1.0)
				norm_opacity = 255;
			    else
			      {
				  opacity = 255.0 * polyg_sym->fill->opacity;
				  if (opacity <= 0.0)
				      norm_opacity = 0;
				  else if (opacity >= 255.0)
				      norm_opacity = 255;
				  else
				      norm_opacity = opacity;
			      }
			    rl2_graph_set_brush (ctx, polyg_sym->fill->red,
						 polyg_sym->fill->green,
						 polyg_sym->fill->blue,
						 norm_opacity);
			    fill = 1;
			}
		  }
		if (polyg_sym->stroke != NULL)
		  {
		      if (polyg_sym->stroke->graphic != NULL)
			{
			    /* external Graphic stroke */
			    const char *xlink_href = NULL;
			    int recolor = 0;
			    unsigned char red;
			    unsigned char green;
			    unsigned char blue;
			    pattern_stroke = NULL;
			    if (polyg_sym->stroke->graphic->first != NULL)
			      {
				  if (polyg_sym->stroke->graphic->first->type ==
				      RL2_EXTERNAL_GRAPHIC)
				    {
					rl2PrivExternalGraphicPtr ext =
					    (rl2PrivExternalGraphicPtr)
					    (polyg_sym->stroke->graphic->
					     first->item);
					xlink_href = ext->xlink_href;
					if (ext->first != NULL)
					  {
					      recolor = 1;
					      red = ext->first->red;
					      green = ext->first->green;
					      blue = ext->first->blue;
					  }
				    }
			      }
			    if (xlink_href != NULL)
				pattern_stroke =
				    rl2_create_pattern_from_external_graphic
				    (handle, xlink_href, 1);
			    if (pattern_stroke != NULL)
			      {
				  if (recolor)
				    {
					/* attempting to recolor the External Graphic resource */
					rl2_graph_pattern_recolor
					    (pattern_stroke, red, green, blue);
				    }
				  if (polyg_sym->stroke->opacity <= 0.0)
				      norm_opacity = 0;
				  else if (polyg_sym->stroke->opacity >= 1.0)
				      norm_opacity = 255;
				  else
				    {
					opacity =
					    255.0 * polyg_sym->stroke->opacity;
					if (opacity <= 0.0)
					    norm_opacity = 0;
					else if (opacity >= 255.0)
					    norm_opacity = 255;
					else
					    norm_opacity = opacity;
				    }
				  if (norm_opacity < 1.0)
				      rl2_graph_pattern_transparency
					  (pattern_stroke, norm_opacity);
				  rl2_graph_set_pattern_brush (ctx,
							       pattern_stroke);
			      }
			    else
			      {
				  /* invalid Pattern: defaulting to a Gray brush */
				  rl2_graph_set_brush (ctx, 128, 128, 128, 255);
			      }
			    stroke = 1;
			}
		      else
			{
			    /* solid RGB stroke */
			    if (polyg_sym->stroke->opacity <= 0.0)
				norm_opacity = 0;
			    else if (polyg_sym->stroke->opacity >= 1.0)
				norm_opacity = 255;
			    else
			      {
				  opacity = 255.0 * polyg_sym->stroke->opacity;
				  if (opacity <= 0.0)
				      norm_opacity = 0;
				  else if (opacity >= 255.0)
				      norm_opacity = 255;
				  else
				      norm_opacity = opacity;
			      }
			    switch (polyg_sym->stroke->linecap)
			      {
			      case RL2_STROKE_LINECAP_ROUND:
				  pen_cap = RL2_PEN_CAP_ROUND;
				  break;
			      case RL2_STROKE_LINECAP_SQUARE:
				  pen_cap = RL2_PEN_CAP_SQUARE;
				  break;
			      default:
				  pen_cap = RL2_PEN_CAP_BUTT;
				  break;
			      };
			    switch (polyg_sym->stroke->linejoin)
			      {
			      case RL2_STROKE_LINEJOIN_BEVEL:
				  pen_join = RL2_PEN_JOIN_BEVEL;
				  break;
			      case RL2_STROKE_LINEJOIN_ROUND:
				  pen_join = RL2_PEN_JOIN_ROUND;
				  break;
			      default:
				  pen_join = RL2_PEN_JOIN_MITER;
				  break;
			      };
			    if (polyg_sym->stroke->dash_count > 0
				&& polyg_sym->stroke->dash_list != NULL)
				rl2_graph_set_dashed_pen (ctx,
							  polyg_sym->
							  stroke->red,
							  polyg_sym->
							  stroke->green,
							  polyg_sym->
							  stroke->blue,
							  norm_opacity,
							  polyg_sym->
							  stroke->width,
							  pen_cap, pen_join,
							  polyg_sym->
							  stroke->dash_count,
							  polyg_sym->
							  stroke->dash_list,
							  polyg_sym->
							  stroke->dash_offset);
			    else
				rl2_graph_set_solid_pen (ctx,
							 polyg_sym->stroke->red,
							 polyg_sym->
							 stroke->green,
							 polyg_sym->
							 stroke->blue,
							 norm_opacity,
							 polyg_sym->
							 stroke->width, pen_cap,
							 pen_join);
			    stroke = 1;
			}
		  }
	    }
	  if (!fill && !stroke)
	    {
		item = item->next;
		continue;
	    }

	  polyg = geom->first_polygon;
	  while (polyg)
	    {
		/* drawing a POLYGON */
		int iv;
		double dx;
		double dy;
		int x;
		int y;
		int lastX = 0;
		int lastY = 0;
		int ib;
		rl2RingPtr ring = polyg->exterior;
		/* exterior border */
		if (ring_bbox_matches (ring, minx, miny, maxx, maxy))
		  {
		      for (iv = 0; iv < ring->points; iv++)
			{
			    rl2GetPoint (ring->coords, iv, &dx, &dy);
			    x = (int) ((dx - minx) / x_res);
			    y = height - (int) ((dy - miny) / y_res);
			    if (iv == 0)
			      {
				  rl2_graph_move_to_point (ctx, x, y);
				  lastX = x;
				  lastY = y;
			      }
			    else
			      {
				  if (x == lastX && y == lastY)
				      ;
				  else
				    {
					rl2_graph_add_line_to_path (ctx, x, y);
					lastX = x;
					lastY = y;
				    }
			      }
			}
		      rl2_graph_close_subpath (ctx);
		  }
		else
		  {
		      /* if the exterior ring is invisible we'll ignore all internal rings */
		      polyg = polyg->next;
		      continue;
		  }
		for (ib = 0; ib < polyg->num_interiors; ib++)
		  {
		      /* interior borders */
		      ring = polyg->interiors + ib;
		      if (ring_bbox_matches (ring, minx, miny, maxx, maxy))
			{
			    for (iv = 0; iv < ring->points; iv++)
			      {
				  rl2GetPoint (ring->coords, iv, &dx, &dy);
				  x = (int) ((dx - minx) / x_res);
				  y = height - (int) ((dy - miny) / y_res);
				  if (iv == 0)
				    {
					rl2_graph_move_to_point (ctx, x, y);
					lastX = x;
					lastY = y;
				    }
				  else
				    {
					if (x == lastX && y == lastY)
					    ;
					else
					  {
					      rl2_graph_add_line_to_path (ctx,
									  x, y);
					      lastX = x;
					      lastY = y;
					  }
				    }
			      }
			    rl2_graph_close_subpath (ctx);
			}
		  }
		if (fill)
		  {
		      if (stroke)
			  rl2_graph_fill_path (ctx, RL2_PRESERVE_PATH);
		      else
			  rl2_graph_fill_path (ctx, RL2_CLEAR_PATH);
		  }
		if (stroke)
		    rl2_graph_stroke_path (ctx, RL2_CLEAR_PATH);
		polyg = polyg->next;
	    }
	  if (pattern_fill != NULL)
	    {
		rl2_graph_release_pattern_brush (ctx);
		rl2_graph_destroy_pattern (pattern_fill);
	    }
	  if (pattern_stroke != NULL)
	    {
		rl2_graph_release_pattern_pen (ctx);
		rl2_graph_destroy_pattern (pattern_stroke);
	    }
	  item = item->next;
      }
}

static int
label_get_xy (sqlite3 * handle, const unsigned char *blob, int size,
	      double *x, double *y)
{
/* resolving Point XY coords */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;

    sql = "SELECT ST_X(?), ST_Y(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, size, SQLITE_STATIC);
    sqlite3_bind_blob (stmt, 2, blob, size, SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		*x = sqlite3_column_double (stmt, 0);
		*y = sqlite3_column_double (stmt, 1);
		ok = 1;
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static int
label_get_centroid (sqlite3 * handle, rl2PolygonPtr polyg, double *x, double *y)
{
/* computing a Polygon Centroid */
    unsigned char *blob;
    int blob_sz;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;

    if (polyg == NULL)
	return 0;
    if (polyg->exterior == NULL)
	return 0;
    if (!rl2_serialize_ring (polyg->exterior, &blob, &blob_sz))
	return 0;

    sql = "SELECT ST_Centroid(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
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
		      const unsigned char *g_blob =
			  (const unsigned char *) sqlite3_column_blob (stmt,
								       0);
		      int g_size = sqlite3_column_bytes (stmt, 0);
		      if (label_get_xy (handle, g_blob, g_size, x, y))
			  ok = 1;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static int
label_get_midpoint (sqlite3 * handle, rl2LinestringPtr line, double *x,
		    double *y)
{
/* computing a Linestring MidPoint */
    unsigned char *blob;
    int blob_sz;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;

    if (line == NULL)
	return 0;
    if (!rl2_serialize_linestring (line, &blob, &blob_sz))
	return 0;

    sql = "SELECT ST_Line_Interpolate_Point(?, 0.5)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
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
		      const unsigned char *g_blob =
			  (const unsigned char *) sqlite3_column_blob (stmt,
								       0);
		      int g_size = sqlite3_column_bytes (stmt, 0);
		      if (label_get_xy (handle, g_blob, g_size, x, y))
			  ok = 1;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static int
label_get_ring_midpoint (sqlite3 * handle, rl2RingPtr ring, double *x,
			 double *y)
{
/* computing a Ring MidPoint */
    unsigned char *blob;
    int blob_sz;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;

    if (ring == NULL)
	return 0;
    if (!rl2_serialize_ring_as_linestring (ring, &blob, &blob_sz))
	return 0;

    sql = "SELECT ST_Line_Interpolate_Point(?, 0.5)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
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
		      const unsigned char *g_blob =
			  (const unsigned char *) sqlite3_column_blob (stmt,
								       0);
		      int g_size = sqlite3_column_bytes (stmt, 0);
		      if (label_get_xy (handle, g_blob, g_size, x, y))
			  ok = 1;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static rl2GeometryPtr
do_generalize_linestring (sqlite3 * handle, rl2LinestringPtr line,
			  double generalize_factor)
{
/* simplifying a Linestring */
    rl2GeometryPtr geom = NULL;
    unsigned char *blob;
    int blob_sz;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    if (line == NULL)
	return NULL;
    if (line->points < 2)
	return NULL;
    if (!rl2_serialize_linestring (line, &blob, &blob_sz))
	return NULL;

    sql = "SELECT ST_Simplify(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return NULL;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
    sqlite3_bind_double (stmt, 2, generalize_factor);
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
		      const unsigned char *g_blob =
			  (const unsigned char *) sqlite3_column_blob (stmt,
								       0);
		      int g_blob_sz = sqlite3_column_bytes (stmt, 0);
		      geom = rl2_geometry_from_blob (g_blob, g_blob_sz);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return geom;
}

static rl2GeometryPtr
do_generalize_ring (sqlite3 * handle, rl2RingPtr ring, double generalize_factor)
{
/* simplifying a Ring */
    rl2GeometryPtr geom = NULL;
    unsigned char *blob;
    int blob_sz;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    if (ring == NULL)
	return NULL;
    if (ring->points < 2)
	return NULL;
    if (!rl2_serialize_ring (ring, &blob, &blob_sz))
	return NULL;

    sql = "SELECT ST_SimplifyPreserveTopology(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return NULL;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
    sqlite3_bind_double (stmt, 2, generalize_factor);
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
		      const unsigned char *g_blob =
			  (const unsigned char *) sqlite3_column_blob (stmt,
								       0);
		      int g_blob_sz = sqlite3_column_bytes (stmt, 0);
		      geom = rl2_geometry_from_blob (g_blob, g_blob_sz);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return geom;
}

static int
check_valid_line (rl2LinestringPtr line)
{
/* testing for a valid linestring */
    int iv;
    int pts = 0;
    double x;
    double y;
    double x0;
    double y0;

    if (line == NULL)
	return 0;
    if (line->points < 2)
	return 0;
    rl2GetPoint (line->coords, 0, &x0, &y0);
    for (iv = 1; iv < line->points; iv++)
      {
	  rl2GetPoint (line->coords, iv, &x, &y);
	  if (x != x0 || y != y0)
	    {
		pts++;
		break;
	    }
      }
    if (pts == 0)
	return 0;
    return 1;
}

static rl2GeometryPtr
do_offset_linestring (sqlite3 * handle, rl2LinestringPtr line,
		      double perpendicular_offset)
{
/* Offest Curve (from Linestring) */
    rl2GeometryPtr geom = NULL;
    unsigned char *blob;
    int blob_sz;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    if (!check_valid_line (line))
	return NULL;
    if (!rl2_serialize_linestring (line, &blob, &blob_sz))
	return NULL;

    sql = "SELECT ST_OffsetCurve(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return NULL;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
    sqlite3_bind_double (stmt, 2, perpendicular_offset);
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
		      const unsigned char *g_blob =
			  (const unsigned char *) sqlite3_column_blob (stmt,
								       0);
		      int g_blob_sz = sqlite3_column_bytes (stmt, 0);
		      geom = rl2_geometry_from_blob (g_blob, g_blob_sz);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return geom;
}

static int
check_valid_ring (rl2RingPtr ring)
{
/* testing for a valid ring */
    int iv;
    int pts = 0;
    int last;
    double x;
    double y;
    double x0;
    double y0;
    double x1;
    double y1;

    if (ring == NULL)
	return 0;
    if (ring->points < 4)
	return 0;
    rl2GetPoint (ring->coords, 0, &x0, &y0);
    for (iv = 1; iv < ring->points; iv++)
      {
	  rl2GetPoint (ring->coords, iv, &x, &y);
	  if (pts == 0)
	    {
		if (x != x0 || y != y0)
		  {
		      pts++;
		      x1 = x;
		      y1 = y;
		  }
	    }
	  else
	    {
		if ((x != x0 || y != y0) && (x != x1 || y != y1))
		  {
		      pts++;
		      break;
		  }
	    }
      }
    last = ring->points - 1;
    rl2GetPoint (ring->coords, last, &x1, &y1);
    if (pts == 2 && x0 == x1 && y0 == y1)
	return 1;
    return 0;
}

static rl2GeometryPtr
do_buffered_ring (sqlite3 * handle, rl2RingPtr ring,
		  double perpendicular_offset)
{
/* Buffer (from Ring) */
    rl2GeometryPtr geom = NULL;
    unsigned char *blob;
    int blob_sz;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    if (!check_valid_ring (ring))
	return NULL;
    if (!rl2_serialize_ring (ring, &blob, &blob_sz))
	return NULL;

    sql = "SELECT ST_Buffer(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return NULL;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
    sqlite3_bind_double (stmt, 2, perpendicular_offset);
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
		      const unsigned char *g_blob =
			  (const unsigned char *) sqlite3_column_blob (stmt,
								       0);
		      int g_blob_sz = sqlite3_column_bytes (stmt, 0);
		      geom = rl2_geometry_from_blob (g_blob, g_blob_sz);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return geom;
}

static void
create_line_array_from_linestring (sqlite3 * handle, rl2LinestringPtr line,
				   double perpendicular_offset, int *points,
				   double **x_array, double **y_array,
				   int generalize_line,
				   double generalize_factor, int height,
				   double minx, double miny, double x_res,
				   double y_res)
{
/* creating the X and Y arrays required by rl2_graph_draw_warped_text() */
    rl2GeometryPtr geom = NULL;
    rl2GeometryPtr geom2 = NULL;
    rl2LinestringPtr aux_line;
    rl2LinestringPtr in_line;
    double *xx = NULL;
    double *yy = NULL;
    int iv;
    double x;
    double y;

    *points = 0;
    *x_array = NULL;
    *y_array = NULL;
    if (line == NULL)
	goto error;

    aux_line = rl2_linestring_to_image (line, height, minx, miny, x_res, y_res);
    if (aux_line == NULL)
	goto error;
    in_line = aux_line;
    if (generalize_line)
      {
	  geom = do_generalize_linestring (handle, in_line, generalize_factor);
	  if (geom == NULL)
	      goto error;
	  in_line = geom->first_linestring;
	  if (in_line == NULL)
	      goto error;
      }
    if (perpendicular_offset != 0.0)
      {
	  geom2 = do_offset_linestring (handle, in_line, perpendicular_offset);
	  if (geom2 == NULL)
	      goto error;
	  in_line = geom2->first_linestring;
	  if (in_line == NULL)
	      goto error;
      }

/* allocating the X and Y arrays */
    if (in_line->points < 2)
	goto error;
    xx = malloc (sizeof (double) * in_line->points);
    yy = malloc (sizeof (double) * in_line->points);
    if (xx == NULL || yy == NULL)
      {
	  if (xx != NULL)
	      free (xx);
	  if (yy != NULL)
	      free (yy);
	  goto error;
      }
    for (iv = 0; iv < in_line->points; iv++)
      {
	  /* populating the X and Y arrays */
	  rl2GetPoint (in_line->coords, iv, &x, &y);
	  *(xx + iv) = x;
	  *(yy + iv) = y;
      }
    *points = in_line->points;
    *x_array = xx;
    *y_array = yy;

  error:
    if (aux_line)
	rl2DestroyLinestring (aux_line);
    if (geom)
	rl2_destroy_geometry (geom);
    if (geom2)
	rl2_destroy_geometry (geom2);
}

static void
create_line_array_from_ring (sqlite3 * handle, rl2RingPtr ring,
			     double perpendicular_offset, int *points,
			     double **x_array, double **y_array,
			     int generalize_line, double generalize_factor,
			     int height, double minx, double miny, double x_res,
			     double y_res)
{
/* creating the X and Y arrays required by rl2_graph_draw_warped_text() */
    rl2GeometryPtr geom = NULL;
    rl2GeometryPtr geom2 = NULL;
    rl2PolygonPtr pg;
    rl2RingPtr aux_ring;
    rl2RingPtr in_ring;
    double *xx = NULL;
    double *yy = NULL;
    int iv;
    double x;
    double y;

    *points = 0;
    *x_array = NULL;
    *y_array = NULL;
    if (ring == NULL)
	goto error;

    aux_ring = rl2_ring_to_image (ring, height, minx, miny, x_res, y_res);
    if (aux_ring == NULL)
	goto error;
    in_ring = aux_ring;
    if (generalize_line)
      {
	  geom = do_generalize_ring (handle, in_ring, generalize_factor);
	  if (geom == NULL)
	      goto error;
	  pg = geom->first_polygon;
	  if (pg == NULL)
	      goto error;
	  in_ring = pg->exterior;
	  if (in_ring == NULL)
	      goto error;
      }
    if (perpendicular_offset != 0.0)
      {
	  geom2 = do_buffered_ring (handle, in_ring, perpendicular_offset);
	  if (geom2 == NULL)
	      goto error;
	  pg = geom2->first_polygon;
	  if (pg == NULL)
	      goto error;
	  in_ring = pg->exterior;
	  if (in_ring == NULL)
	      goto error;
      }

/* allocating the X and Y arrays */
    if (in_ring->points < 2)
	goto error;
    xx = malloc (sizeof (double) * in_ring->points);
    yy = malloc (sizeof (double) * in_ring->points);
    if (xx == NULL || yy == NULL)
      {
	  if (xx != NULL)
	      free (xx);
	  if (yy != NULL)
	      free (yy);
	  goto error;
      }
    for (iv = 0; iv < in_ring->points; iv++)
      {
	  /* populating the X and Y arrays */
	  rl2GetPoint (in_ring->coords, iv, &x, &y);
	  *(xx + iv) = x;
	  *(yy + iv) = y;
      }
    *points = in_ring->points;
    *x_array = xx;
    *y_array = yy;

  error:
    if (aux_ring)
	rl2DestroyRing (aux_ring);
    if (geom)
	rl2_destroy_geometry (geom);
    if (geom2)
	rl2_destroy_geometry (geom2);
}

static void
draw_labels (rl2GraphicsContextPtr ctx, sqlite3 * handle,
	     const void *priv_data, rl2PrivTextSymbolizerPtr sym, int height,
	     double minx, double miny, double maxx, double maxy, double x_res,
	     double y_res, rl2GeometryPtr geom, rl2PrivVariantValuePtr value)
{
/* drawing TextLabels */
    rl2GraphicsFontPtr font = NULL;
    char *dummy = NULL;
    const char *label = NULL;
    int font_style;
    int font_weight;
    double opacity;
    unsigned char norm_opacity;
    rl2PointPtr point;
    rl2LinestringPtr line;
    rl2PolygonPtr polyg;
    double rotation = 0.0;
    double anchor_point_x = 0.0;
    double anchor_point_y = 0.0;
    double displacement_x = 0.0;
    double displacement_y = 0.0;
    double perpendicular_offset = 0.0;
    int is_repeated = 0;
    double initial_gap = 0.0;
    double gap = 0.0;
    int is_aligned = 0;
    int generalize_line = 0;
    int i;

/* preparing the Text */
    if (value->sqlite3_type == SQLITE_INTEGER)
      {
	  dummy = sqlite3_malloc (1024);
#if defined(_WIN32) && !defined(__MINGW32__)
	  sprintf (dummy, "%I64d", value->int_value);
#else
	  sprintf (dummy, "%lld", value->int_value);
#endif
	  label = dummy;
      }
    if (value->sqlite3_type == SQLITE_FLOAT)
      {
	  dummy = sqlite3_mprintf ("1.2f", value->dbl_value);
	  label = dummy;
      }
    if (value->sqlite3_type == SQLITE_TEXT)
	label = (const char *) (value->text_value);
    if (label == NULL)
	return;

/* setting up the Font */
    switch (sym->font_style)
      {
      case RL2_FONT_STYLE_ITALIC:
	  font_style = RL2_FONTSTYLE_ITALIC;
	  break;
      case RL2_FONT_STYLE_OBLIQUE:
	  font_style = RL2_FONTSTYLE_OBLIQUE;
	  break;
      case RL2_FONT_STYLE_NORMAL:
      default:
	  font_style = RL2_FONTSTYLE_NORMAL;
	  break;
      };
    switch (sym->font_weight)
      {
      case RL2_FONT_WEIGHT_BOLD:
	  font_weight = RL2_FONTWEIGHT_BOLD;
	  break;
      case RL2_FONT_WEIGHT_NORMAL:
      default:
	  font_weight = RL2_FONTWEIGHT_NORMAL;
	  break;
      };
    for (i = 0; i < RL2_MAX_FONT_FAMILIES; i++)
      {
	  const char *facename = sym->font_families[i];
	  if (facename != NULL)
	      font =
		  rl2_search_TrueType_font (handle, priv_data, facename,
					    sym->font_size);
	  if (font != NULL)
	      break;
      }
    if (font == NULL)
      {
	  /* defaulting to a toy font */
	  fprintf (stderr, "default toy font\n");
	  font =
	      rl2_graph_create_toy_font (NULL, sym->font_size, font_style,
					 font_weight);
      }
    if (font == NULL)
	goto stop;
    if (sym->fill != NULL)
      {
	  if (sym->fill->opacity <= 0.0)
	      norm_opacity = 0;
	  else if (sym->fill->opacity >= 1.0)
	      norm_opacity = 255;
	  else
	    {
		opacity = 255.0 * sym->fill->opacity;
		if (opacity <= 0.0)
		    norm_opacity = 0;
		else if (opacity >= 255.0)
		    norm_opacity = 255;
		else
		    norm_opacity = opacity;
	    }
	  rl2_graph_font_set_color (font, sym->fill->red, sym->fill->green,
				    sym->fill->blue, norm_opacity);
      }
    if (sym->halo != NULL)
      {
	  if (sym->halo->fill->opacity <= 0.0)
	      norm_opacity = 0;
	  else if (sym->halo->fill->opacity >= 1.0)
	      norm_opacity = 255;
	  else
	    {
		opacity = 255.0 * sym->halo->fill->opacity;
		if (opacity <= 0.0)
		    norm_opacity = 0;
		else if (opacity >= 255.0)
		    norm_opacity = 255;
		else
		    norm_opacity = opacity;
	    }
	  rl2_graph_font_set_halo (font, sym->halo->radius,
				   sym->halo->fill->red,
				   sym->halo->fill->green,
				   sym->halo->fill->blue, norm_opacity);
      }
    rl2_graph_set_font (ctx, font);

    if (sym->label_placement_type == RL2_LABEL_PLACEMENT_POINT)
      {
	  /* retrieving eventual Point Placement arguments */
	  rl2PrivPointPlacementPtr ptpl =
	      (rl2PrivPointPlacementPtr) (sym->label_placement);
	  if (ptpl != NULL)
	    {
		anchor_point_x = ptpl->anchor_point_x;
		anchor_point_y = ptpl->anchor_point_y;
		displacement_x = ptpl->displacement_x;
		displacement_y = ptpl->displacement_y;
		rotation = ptpl->rotation;
	    }
      }
    else if (sym->label_placement_type == RL2_LABEL_PLACEMENT_LINE)
      {
	  /* retrieving eventual Lineo Placement arguments */
	  rl2PrivLinePlacementPtr lnpl =
	      (rl2PrivLinePlacementPtr) (sym->label_placement);
	  if (lnpl != NULL)
	    {
		perpendicular_offset = lnpl->perpendicular_offset;
		is_repeated = lnpl->is_repeated;
		initial_gap = lnpl->initial_gap;
		gap = lnpl->gap;
		is_aligned = lnpl->is_aligned;
		generalize_line = lnpl->generalize_line;
	    }
      }

    if (sym->label_placement_type == RL2_LABEL_PLACEMENT_POINT)
      {
	  /* POINT PLACEMENT */
	  rl2Point pt;
	  double cx;
	  double cy;
	  double x;
	  double y;

	  polyg = geom->first_polygon;
	  while (polyg)
	    {
		/* drawing a POLYGON-based text label */
		if (!label_get_centroid (handle, polyg, &cx, &cy))
		  {
		      polyg = polyg->next;
		      continue;
		  }
		pt.x = cx;
		pt.y = cy;
		if (point_bbox_matches (&pt, minx, miny, maxx, maxy))
		  {
		      x = (cx - minx) / x_res;
		      y = (double) height - ((cy - miny) / y_res);
		      rl2_graph_draw_text (ctx, label, x + displacement_x,
					   y - displacement_y, rotation,
					   anchor_point_x, anchor_point_y);
		  }
		polyg = polyg->next;
	    }

	  line = geom->first_linestring;
	  while (line)
	    {
		/* drawing a LINESTRING-based text label */
		label_get_midpoint (handle, line, &cx, &cy);
		pt.x = cx;
		pt.y = cy;
		if (point_bbox_matches (&pt, minx, miny, maxx, maxy))
		  {
		      x = (cx - minx) / x_res;
		      y = (double) height - ((cy - miny) / y_res);
		      rl2_graph_draw_text (ctx, label, x + displacement_x,
					   y - displacement_y, rotation,
					   anchor_point_x, anchor_point_y);
		  }
		line = line->next;
	    }

	  point = geom->first_point;
	  while (point)
	    {
		/* drawing a POINT-based text label */
		if (point_bbox_matches (point, minx, miny, maxx, maxy))
		  {
		      double x = (point->x - minx) / x_res;
		      double y = (double) height - ((point->y - miny) / y_res);
		      rl2_graph_draw_text (ctx, label, x + displacement_x,
					   y - displacement_y, rotation,
					   anchor_point_x, anchor_point_y);
		  }
		point = point->next;
	    }

      }
    else if (sym->label_placement_type == RL2_LABEL_PLACEMENT_LINE)
      {
	  /* LINE PLACEMENT */
	  rl2Point pt;
	  int ib;
	  double cx;
	  double cy;
	  double x;
	  double y;
	  double generalize_factor = 8.0;

	  line = geom->first_linestring;
	  while (line)
	    {
		/* drawing a LINESTRING-based text label */
		if (!is_aligned)
		  {
		      /* horizontal label aligned to center point */
		      label_get_midpoint (handle, line, &cx, &cy);
		      pt.x = cx;
		      pt.y = cy;
		      if (point_bbox_matches (&pt, minx, miny, maxx, maxy))
			{
			    x = (cx - minx) / x_res;
			    y = (double) height - ((cy - miny) / y_res);
			    rl2_graph_draw_text (ctx, label, x, y, 0.0, 0.5,
						 0.5);
			}
		  }
		else
		  {
		      /* label is warped along the line */
		      double *x_array = NULL;
		      double *y_array = NULL;
		      int points;
		      if (linestring_bbox_matches
			  (line, minx, miny, maxx, maxy))
			{
			    create_line_array_from_linestring (handle, line,
							       perpendicular_offset,
							       &points,
							       &x_array,
							       &y_array,
							       generalize_line,
							       generalize_factor,
							       height, minx,
							       miny, x_res,
							       y_res);
			    if (x_array != NULL && y_array != NULL)
				rl2_graph_draw_warped_text (handle, ctx, label,
							    points, x_array,
							    y_array,
							    initial_gap, gap,
							    is_repeated);
			    if (x_array)
				free (x_array);
			    if (y_array)
				free (y_array);
			}
		  }
		line = line->next;
	    }

	  polyg = geom->first_polygon;
	  while (polyg)
	    {
		/* drawing a POLYGON-based text label */
		rl2RingPtr ring = polyg->exterior;
		/* exterior border */
		if (ring_bbox_matches (ring, minx, miny, maxx, maxy))
		  {
		      if (!is_aligned)
			{
			    /* horizontal label aligned to Ring's center point */
			    label_get_ring_midpoint (handle, ring, &cx, &cy);
			    pt.x = cx;
			    pt.y = cy;
			    if (point_bbox_matches
				(&pt, minx, miny, maxx, maxy))
			      {
				  x = (cx - minx) / x_res;
				  y = (double) height - ((cy - miny) / y_res);
				  rl2_graph_draw_text (ctx, label, x, y, 0.0,
						       0.5, 0.5);
			      }
			}
		      else
			{
			    /* label is warped along the Ring */
			    double *x_array = NULL;
			    double *y_array = NULL;
			    int points;
			    if (ring_bbox_matches
				(ring, minx, miny, maxx, maxy))
			      {
				  create_line_array_from_ring (handle, ring,
							       perpendicular_offset,
							       &points,
							       &x_array,
							       &y_array,
							       generalize_line,
							       generalize_factor,
							       height, minx,
							       miny, x_res,
							       y_res);
				  if (x_array != NULL && y_array != NULL)
				    {
					rl2_graph_draw_warped_text (handle, ctx,
								    label,
								    points,
								    x_array,
								    y_array,
								    initial_gap,
								    gap,
								    is_repeated);
				    }
				  if (x_array)
				      free (x_array);
				  if (y_array)
				      free (y_array);
			      }
			}
		  }
		else
		  {
		      /* if the exterior ring is invisible we'll ignore all internal rings */
		      polyg = polyg->next;
		      continue;
		  }
		for (ib = 0; ib < polyg->num_interiors; ib++)
		  {
		      /* interior borders */
		      ring = polyg->interiors + ib;
		      if (ring_bbox_matches (ring, minx, miny, maxx, maxy))
			{
			    if (!is_aligned)
			      {
				  /* horizontal label aligned to Ring's center point */
				  label_get_ring_midpoint (handle, ring, &cx,
							   &cy);
				  pt.x = cx;
				  pt.y = cy;
				  if (point_bbox_matches
				      (&pt, minx, miny, maxx, maxy))
				    {
					x = (cx - minx) / x_res;
					y = (double) height -
					    ((cy - miny) / y_res);
					rl2_graph_draw_text (ctx, label, x, y,
							     0.0, 0.5, 0.5);
				    }
			      }
			    else
			      {
				  /* label is warped along the Ring */
				  double *x_array = NULL;
				  double *y_array = NULL;
				  int points;
				  if (ring_bbox_matches
				      (ring, minx, miny, maxx, maxy))
				    {
					create_line_array_from_ring (handle,
								     ring,
								     perpendicular_offset,
								     &points,
								     &x_array,
								     &y_array,
								     generalize_line,
								     generalize_factor,
								     height,
								     minx, miny,
								     x_res,
								     y_res);
					if (x_array != NULL && y_array != NULL)
					    rl2_graph_draw_warped_text (handle,
									ctx,
									label,
									points,
									x_array,
									y_array,
									initial_gap,
									gap,
									is_repeated);
					if (x_array)
					    free (x_array);
					if (y_array)
					    free (y_array);
				    }
			      }
			}
		  }
		polyg = polyg->next;
	    }
      }

/* final cleanup - relasing resources */
  stop:
    if (dummy != NULL)
	sqlite3_free (dummy);
    if (font != NULL)
	rl2_graph_destroy_font (font);
}

RL2_PRIVATE void
rl2_draw_vector_feature (void *p_ctx, sqlite3 * handle, const void *priv_data,
			 rl2VectorSymbolizerPtr symbolizer, int height,
			 double minx, double miny, double maxx, double maxy,
			 double x_res, double y_res, rl2GeometryPtr geom,
			 rl2VariantArrayPtr variant)
{
/* drawing a vector feature on the current canvass */
    rl2PrivVectorSymbolizerItemPtr item;
    rl2GraphicsContextPtr ctx = (rl2GraphicsContextPtr) p_ctx;
    rl2PrivVectorSymbolizerPtr sym = (rl2PrivVectorSymbolizerPtr) symbolizer;
    rl2PrivVectorSymbolizerPtr default_symbolizer = NULL;

    if (ctx == NULL || geom == NULL)
	return;

    if (sym == NULL)
      {
	  /* creating a default general purpose Symbolizer */

	  default_symbolizer = rl2_create_default_vector_symbolizer ();
	  item = rl2_create_default_point_symbolizer ();
	  if (item->symbolizer_type == RL2_POINT_SYMBOLIZER
	      && item->symbolizer != NULL)
	    {
		rl2PrivPointSymbolizerPtr s =
		    (rl2PrivPointSymbolizerPtr) (item->symbolizer);
		s->graphic = rl2_create_default_graphic ();
		if (s->graphic != NULL)
		  {
		      s->graphic->size = 16.0;
		      s->graphic->first = rl2_create_default_mark ();
		      if (s->graphic->first != NULL)
			{
			    s->graphic->last = s->graphic->first;
			    if (s->graphic->first->type == RL2_MARK_GRAPHIC
				&& s->graphic->first->item != NULL)
			      {
				  rl2PrivMarkPtr mark =
				      (rl2PrivMarkPtr) (s->graphic->
							first->item);
				  mark->well_known_type =
				      RL2_GRAPHIC_MARK_SQUARE;
				  mark->fill = rl2_create_default_fill ();
				  mark->stroke = rl2_create_default_stroke ();
			      }
			}
		  }
	    }
	  if (default_symbolizer->first == NULL)
	      default_symbolizer->first = item;
	  if (default_symbolizer->last != NULL)
	      default_symbolizer->last->next = item;
	  default_symbolizer->last = item;
	  item = rl2_create_default_line_symbolizer ();
	  if (item->symbolizer_type == RL2_LINE_SYMBOLIZER
	      && item->symbolizer != NULL)
	    {
		rl2PrivLineSymbolizerPtr s =
		    (rl2PrivLineSymbolizerPtr) (item->symbolizer);
		s->stroke = rl2_create_default_stroke ();
	    }
	  if (default_symbolizer->first == NULL)
	      default_symbolizer->first = item;
	  if (default_symbolizer->last != NULL)
	      default_symbolizer->last->next = item;
	  if (default_symbolizer->first == NULL)
	      default_symbolizer->first = item;
	  if (default_symbolizer->last != NULL)
	      default_symbolizer->last->next = item;
	  default_symbolizer->last = item;
	  item = rl2_create_default_polygon_symbolizer ();
	  if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER
	      && item->symbolizer != NULL)
	    {
		rl2PrivPolygonSymbolizerPtr s =
		    (rl2PrivPolygonSymbolizerPtr) (item->symbolizer);
		s->fill = rl2_create_default_fill ();
		s->stroke = rl2_create_default_stroke ();
	    }
	  if (default_symbolizer->first == NULL)
	      default_symbolizer->first = item;
	  if (default_symbolizer->last != NULL)
	      default_symbolizer->last->next = item;
	  default_symbolizer->last = item;
	  item = rl2_create_default_text_symbolizer ();
	  if (default_symbolizer->first == NULL)
	      default_symbolizer->first = item;
	  if (default_symbolizer->last != NULL)
	      default_symbolizer->last->next = item;
	  default_symbolizer->last = item;
	  sym = default_symbolizer;
      }

/* we'll render all geometries first */
    if (geom->first_polygon != NULL)
	draw_polygons (ctx, handle, sym, height, minx, miny, maxx, maxy,
		       x_res, y_res, geom);
    if (geom->first_linestring != NULL)
	draw_lines (ctx, handle, sym, height, minx, miny, maxx, maxy, x_res,
		    y_res, geom);
    if (geom->first_point != NULL)
	draw_points (ctx, handle, sym, height, minx, miny, maxx, maxy, x_res,
		     y_res, geom);

    if (sym != NULL)
      {
	  /* then we'll render any eventual TextSymbolizer */
	  item = sym->first;
	  while (item != NULL)
	    {
		if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER
		    && item->symbolizer != NULL)
		  {
		      rl2PrivTextSymbolizerPtr text =
			  (rl2PrivTextSymbolizerPtr) (item->symbolizer);
		      if (text->label != NULL)
			{
			    int v;
			    rl2PrivVariantArrayPtr var =
				(rl2PrivVariantArrayPtr) variant;
			    if (var != NULL)
			      {
				  for (v = 0; v < var->count; v++)
				    {
					rl2PrivVariantValuePtr val =
					    *(var->array + v);
					if (val == NULL)
					    continue;
					if (val->column_name == NULL)
					    continue;
					if (strcasecmp
					    (text->label,
					     val->column_name) != 0)
					    continue;
					draw_labels (ctx, handle, priv_data,
						     text, height, minx, miny,
						     maxx, maxy, x_res, y_res,
						     geom, val);
				    }
			      }
			}
		  }
		item = item->next;
	    }
      }

    if (default_symbolizer != NULL)
	rl2_destroy_vector_symbolizer (default_symbolizer);
}

RL2_PRIVATE int
rl2_aux_default_image (unsigned int width, unsigned int height,
		       unsigned char red, unsigned char green,
		       unsigned char blue, int format, int transparent,
		       int quality, unsigned char **ximage, int *ximage_sz)
{
/* creating a default image */
    unsigned int x;
    unsigned int y;
    unsigned char *pixels = malloc (width * height * 3);
    unsigned char *po = pixels;
    unsigned char *mask = NULL;
    unsigned char *pm;

    *ximage = NULL;
    *ximage_sz = 0;
    if (pixels == NULL)
	return 0;

    mask = malloc (width * height);
    if (mask == NULL)
	goto error;
    pm = mask;

/* priming the image buffer to background color */
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		*po++ = red;
		*po++ = green;
		*po++ = blue;
		if (mask != NULL)
		    *pm++ = 0;
	    }
      }

    if (format == RL2_OUTPUT_FORMAT_PNG)
      {
	  if (transparent)
	    {
		if (rl2_rgb_alpha_to_png
		    (width, height, pixels, mask, ximage, ximage_sz,
		     1.0) != RL2_OK)
		    goto error;
	    }
	  else
	    {
		if (rl2_rgb_to_png (width, height, pixels, ximage, ximage_sz)
		    != RL2_OK)
		    goto error;
	    }
      }
    else if (format == RL2_OUTPUT_FORMAT_JPEG)
      {
	  if (rl2_rgb_to_jpeg
	      (width, height, pixels, quality, ximage, ximage_sz) != RL2_OK)
	      goto error;
      }
    else if (format == RL2_OUTPUT_FORMAT_TIFF)
      {
	  if (rl2_rgb_to_tiff (width, height, pixels, ximage, ximage_sz) !=
	      RL2_OK)
	      goto error;
      }
    else
	goto error;
    free (pixels);
    if (mask != NULL)
	free (mask);
    return 1;

  error:
    if (pixels != NULL)
	free (pixels);
    if (mask != NULL)
	free (mask);
    return 0;
}
