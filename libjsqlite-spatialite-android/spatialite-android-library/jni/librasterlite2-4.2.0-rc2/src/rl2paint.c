/*

 rl2paint -- Cairco graphics functions

 version 0.1, 2013 September 29

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
 
Portions created by the Initial Developer are Copyright (C) 2013
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
#include <math.h>

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

#ifdef __ANDROID__	/* Android specific */
#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#else	/* any other standard platform (Win, Linux, Mac) */
#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>
#endif	/* end Android conditionals */

#define RL2_SURFACE_IMG 2671
#define RL2_SURFACE_SVG 1267
#define RL2_SURFACE_PDF	1276

struct rl2_graphics_pen
{
/* a struct wrapping a Cairo Pen */
    double red;
    double green;
    double blue;
    double alpha;
    double width;
    double lengths[4];
    int lengths_count;
};

struct rl2_graphics_brush
{
/* a struct wrapping a Cairo Brush */
    int is_solid_color;
    int is_linear_gradient;
    int is_pattern;
    double red;
    double green;
    double blue;
    double alpha;
    double x0;
    double y0;
    double x1;
    double y1;
    double red2;
    double green2;
    double blue2;
    double alpha2;
    cairo_pattern_t *pattern;
};

typedef struct rl2_graphics_context
{
/* a Cairo based painting context */
    int type;
    cairo_surface_t *surface;
    cairo_surface_t *clip_surface;
    cairo_t *cairo;
    cairo_t *clip_cairo;
    struct rl2_graphics_pen current_pen;
    struct rl2_graphics_brush current_brush;
    double font_red;
    double font_green;
    double font_blue;
    double font_alpha;
    int is_font_outlined;
    double font_outline_width;
} RL2GraphContext;
typedef RL2GraphContext *RL2GraphContextPtr;

typedef struct rl2_graphics_pattern_brush
{
/* a Cairo based pattern brush */
    int width;
    int height;
    unsigned char *rgba;
    cairo_surface_t *bitmap;
    cairo_pattern_t *pattern;
} RL2GraphPatternBrush;
typedef RL2GraphPatternBrush *RL2GraphPatternBrushPtr;

typedef struct rl2_graphics_font
{
/* a struct wrapping a Cairo Font */
    double size;
    int is_outlined;
    double outline_width;
    int style;
    int weight;
    double red;
    double green;
    double blue;
    double alpha;
} RL2GraphFont;
typedef RL2GraphFont *RL2GraphFontPtr;

typedef struct rl2_graphics_bitmap
{
/* a Cairo based symbol bitmap */
    int width;
    int height;
    unsigned char *rgba;
    cairo_surface_t *bitmap;
    cairo_pattern_t *pattern;
} RL2GraphBitmap;
typedef RL2GraphBitmap *RL2GraphBitmapPtr;

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_context (int width, int height)
{
/* creating a generic Graphics Context */
    RL2GraphContextPtr ctx;

    ctx = malloc (sizeof (RL2GraphContext));
    if (!ctx)
	return NULL;

    ctx->type = RL2_SURFACE_IMG;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface =
	cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* setting up a default Black Pen */
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.lengths[0] = 1.0;
    ctx->current_pen.lengths_count = 1;

/* setting up a default Black Brush */
    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = 0.0;
    ctx->current_brush.green = 0.0;
    ctx->current_brush.blue = 0.0;
    ctx->current_brush.alpha = 1.0;
    ctx->current_brush.pattern = NULL;

/* priming a transparent background */
    cairo_rectangle (ctx->cairo, 0, 0, width, height);
    cairo_set_source_rgba (ctx->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (ctx->cairo);

/* setting up default Font options */
    ctx->font_red = 0.0;
    ctx->font_green = 0.0;
    ctx->font_blue = 0.0;
    ctx->font_alpha = 1.0;
    ctx->is_font_outlined = 0;
    ctx->font_outline_width = 0.0;
    return (rl2GraphicsContextPtr) ctx;
  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

static void
destroy_context (RL2GraphContextPtr ctx)
{
/* memory cleanup - destroying a Graphics Context */
    if (ctx == NULL)
	return;
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    free (ctx);
}

static void
destroy_svg_context (RL2GraphContextPtr ctx)
{
/* freeing an SVG Graphics Context */
    if (ctx == NULL)
	return;
    cairo_surface_show_page (ctx->surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_finish (ctx->surface);
    cairo_surface_destroy (ctx->surface);
    free (ctx);
}

static void
destroy_pdf_context (RL2GraphContextPtr ctx)
{
/* freeing an PDF Graphics Context */
    if (ctx == NULL)
	return;
    cairo_surface_finish (ctx->clip_surface);
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->clip_cairo);
    cairo_surface_show_page (ctx->surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_finish (ctx->surface);
    cairo_surface_destroy (ctx->surface);
    free (ctx);
}

RL2_DECLARE void
rl2_graph_destroy_context (rl2GraphicsContextPtr context)
{
/* memory cleanup - destroying a Graphics Context */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return;
    if (ctx->type == RL2_SURFACE_SVG)
	destroy_svg_context (ctx);
    else if (ctx->type == RL2_SURFACE_PDF)
	destroy_pdf_context (ctx);
    else
	destroy_context (ctx);
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_svg_context (const char *path, int width, int height)
{
/* creating an SVG Graphics Context */
    RL2GraphContextPtr ctx;

    ctx = malloc (sizeof (RL2GraphContext));
    if (!ctx)
	return NULL;

    ctx->type = RL2_SURFACE_SVG;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface =
	cairo_svg_surface_create (path, (double) width, (double) height);

    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* setting up a default Black Pen */
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.lengths[0] = 1.0;
    ctx->current_pen.lengths_count = 1;

/* setting up a default Black Brush */
    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = 0.0;
    ctx->current_brush.green = 0.0;
    ctx->current_brush.blue = 0.0;
    ctx->current_brush.alpha = 1.0;
    ctx->current_brush.pattern = NULL;

/* priming a transparent background */
    cairo_rectangle (ctx->cairo, 0, 0, width, height);
    cairo_set_source_rgba (ctx->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (ctx->cairo);

/* setting up default Font options */
    ctx->font_red = 0.0;
    ctx->font_green = 0.0;
    ctx->font_blue = 0.0;
    ctx->font_alpha = 1.0;
    ctx->is_font_outlined = 0;
    ctx->font_outline_width = 0.0;
    return (rl2GraphicsContextPtr) ctx;
  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_pdf_context (const char *path, int dpi, double page_width,
			      double page_height, double margin_width,
			      double margin_height)
{
/* creating a PDF Graphics Context */
    RL2GraphContextPtr ctx;
    double scale = 72.0 / (double) dpi;
    double page2_width = page_width * 72.0;
    double page2_height = page_height * 72.0;
    double horz_margin_sz = margin_width * 72.0;
    double vert_margin_sz = margin_height * 72.0;
    double img_width = (page_width - (margin_width * 2.0)) * 72.0;
    double img_height = (page_height - (margin_height * 2.0)) * 72.0;

    ctx = malloc (sizeof (RL2GraphContext));
    if (ctx == NULL)
	return NULL;

    ctx->type = RL2_SURFACE_PDF;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface = cairo_pdf_surface_create (path, page2_width, page2_height);
    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* priming a transparent background */
    cairo_rectangle (ctx->cairo, 0, 0, page2_width, page2_height);
    cairo_set_source_rgba (ctx->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (ctx->cairo);

/* clipped surface respecting free margins */
    ctx->clip_surface =
	cairo_surface_create_for_rectangle (ctx->surface, horz_margin_sz,
					    vert_margin_sz, img_width,
					    img_height);
    if (cairo_surface_status (ctx->clip_surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error3;
    ctx->clip_cairo = cairo_create (ctx->clip_surface);
    if (cairo_status (ctx->clip_cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error4;

/* setting up a default Black Pen */
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.lengths[0] = 1.0;
    ctx->current_pen.lengths_count = 1;

/* setting up a default Black Brush */
    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = 0.0;
    ctx->current_brush.green = 0.0;
    ctx->current_brush.blue = 0.0;
    ctx->current_brush.alpha = 1.0;
    ctx->current_brush.pattern = NULL;

/* scaling accordingly to DPI resolution */
    cairo_scale (ctx->clip_cairo, scale, scale);

/* setting up default Font options */
    ctx->font_red = 0.0;
    ctx->font_green = 0.0;
    ctx->font_blue = 0.0;
    ctx->font_alpha = 1.0;
    ctx->is_font_outlined = 0;
    ctx->font_outline_width = 0.0;
    return (rl2GraphicsContextPtr) ctx;
  error4:
    cairo_destroy (ctx->clip_cairo);
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error3:
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

static cairo_status_t
pdf_write_func (void *ptr, const unsigned char *data, unsigned int length)
{
/* writing into the in-memory PDF target */
    rl2PrivMemPdfPtr mem = (rl2PrivMemPdfPtr) ptr;
    if (mem == NULL)
	return CAIRO_STATUS_WRITE_ERROR;

    if (mem->write_offset + (int) length < mem->size)
      {
	  /* inserting into the current buffer */
	  memcpy (mem->buffer + mem->write_offset, data, length);
	  mem->write_offset += length;
      }
    else
      {
	  /* expanding the current buffer */
	  int new_sz = mem->size + length + (64 * 1024);
	  unsigned char *save = mem->buffer;
	  mem->buffer = realloc (mem->buffer, new_sz);
	  if (mem->buffer == NULL)
	    {
		free (save);
		return CAIRO_STATUS_WRITE_ERROR;
	    }
	  mem->size = new_sz;
	  memcpy (mem->buffer + mem->write_offset, data, length);
	  mem->write_offset += length;
      }
    return CAIRO_STATUS_SUCCESS;
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_mem_pdf_context (rl2MemPdfPtr mem_pdf, int dpi,
				  double page_width, double page_height,
				  double margin_width, double margin_height)
{
/* creating an in-memory PDF Graphics Context */
    RL2GraphContextPtr ctx;
    double scale = 72.0 / (double) dpi;
    double page2_width = page_width * 72.0;
    double page2_height = page_height * 72.0;
    double horz_margin_sz = margin_width * 72.0;
    double vert_margin_sz = margin_height * 72.0;
    double img_width = (page_width - (margin_width * 2.0)) * 72.0;
    double img_height = (page_height - (margin_height * 2.0)) * 72.0;

    ctx = malloc (sizeof (RL2GraphContext));
    if (ctx == NULL)
	return NULL;

    ctx->type = RL2_SURFACE_PDF;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface =
	cairo_pdf_surface_create_for_stream (pdf_write_func, mem_pdf,
					     page2_width, page2_height);
    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* priming a transparent background */
    cairo_rectangle (ctx->cairo, 0, 0, page2_width, page2_height);
    cairo_set_source_rgba (ctx->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (ctx->cairo);

/* clipped surface respecting free margins */
    ctx->clip_surface =
	cairo_surface_create_for_rectangle (ctx->surface, horz_margin_sz,
					    vert_margin_sz, img_width,
					    img_height);
    if (cairo_surface_status (ctx->clip_surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error3;
    ctx->clip_cairo = cairo_create (ctx->clip_surface);
    if (cairo_status (ctx->clip_cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error4;

/* setting up a default Black Pen */
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.lengths[0] = 1.0;
    ctx->current_pen.lengths_count = 1;

/* setting up a default Black Brush */
    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = 0.0;
    ctx->current_brush.green = 0.0;
    ctx->current_brush.blue = 0.0;
    ctx->current_brush.alpha = 1.0;
    ctx->current_brush.pattern = NULL;

/* scaling accordingly to DPI resolution */
    cairo_scale (ctx->clip_cairo, scale, scale);

/* setting up default Font options */
    ctx->font_red = 0.0;
    ctx->font_green = 0.0;
    ctx->font_blue = 0.0;
    ctx->font_alpha = 1.0;
    ctx->is_font_outlined = 0;
    ctx->font_outline_width = 0.0;
    return (rl2GraphicsContextPtr) ctx;
  error4:
    cairo_destroy (ctx->clip_cairo);
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error3:
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

RL2_DECLARE int
rl2_graph_set_pen (rl2GraphicsContextPtr context, unsigned char red,
		   unsigned char green, unsigned char blue, unsigned char alpha,
		   double width, int style)
{
/* creating a Color Pen */
    double d_red = (double) red / 255.0;
    double d_green = (double) green / 255.0;
    double d_blue = (double) blue / 255.0;
    double d_alpha = (double) alpha / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_pen.width = width;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    switch (style)
      {
      case RL2_PENSTYLE_DOT:
	  ctx->current_pen.lengths[0] = 2;
	  ctx->current_pen.lengths[1] = 2;
	  ctx->current_pen.lengths_count = 2;
	  break;
      case RL2_PENSTYLE_LONG_DASH:
	  ctx->current_pen.lengths[0] = 16;
	  ctx->current_pen.lengths[1] = 8;
	  ctx->current_pen.lengths_count = 2;
	  break;
      case RL2_PENSTYLE_SHORT_DASH:
	  ctx->current_pen.lengths[0] = 8;
	  ctx->current_pen.lengths[1] = 4;
	  ctx->current_pen.lengths_count = 2;
	  break;
      case RL2_PENSTYLE_DOT_DASH:
	  ctx->current_pen.lengths[0] = 8;
	  ctx->current_pen.lengths[1] = 4;
	  ctx->current_pen.lengths[2] = 2;
	  ctx->current_pen.lengths[3] = 4;
	  ctx->current_pen.lengths_count = 4;
	  break;
      default:
	  ctx->current_pen.lengths[0] = 1;
	  ctx->current_pen.lengths[1] = 0;
	  ctx->current_pen.lengths_count = 2;
      };
    return 1;
}

RL2_DECLARE int
rl2_graph_set_brush (rl2GraphicsContextPtr context, unsigned char red,
		     unsigned char green, unsigned char blue,
		     unsigned char alpha)
{
/* setting up a Color Brush */
    double d_red = (double) red / 255.0;
    double d_green = (double) green / 255.0;
    double d_blue = (double) blue / 255.0;
    double d_alpha = (double) alpha / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = d_red;
    ctx->current_brush.green = d_green;
    ctx->current_brush.blue = d_blue;
    ctx->current_brush.alpha = d_alpha;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_linear_gradient_brush (rl2GraphicsContextPtr context, double x,
				     double y, double width, double height,
				     unsigned char red1, unsigned char green1,
				     unsigned char blue1, unsigned char alpha1,
				     unsigned char red2, unsigned char green2,
				     unsigned char blue2, unsigned char alpha2)
{
/* setting up a Linear Gradient Brush */
    double d_red = (double) red1 / 255.0;
    double d_green = (double) green1 / 255.0;
    double d_blue = (double) blue1 / 255.0;
    double d_alpha = (double) alpha1 / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_brush.is_solid_color = 0;
    ctx->current_brush.is_linear_gradient = 1;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = d_red;
    ctx->current_brush.green = d_green;
    ctx->current_brush.blue = d_blue;
    ctx->current_brush.alpha = d_alpha;
    ctx->current_brush.x0 = x;
    ctx->current_brush.y0 = y;
    ctx->current_brush.x1 = x + width;
    ctx->current_brush.y1 = y + height;
    d_red = (double) red2 / 255.0;
    d_green = (double) green2 / 255.0;
    d_blue = (double) blue2 / 255.0;
    d_alpha = (double) alpha2 / 255.0;
    ctx->current_brush.red2 = d_red;
    ctx->current_brush.green2 = d_green;
    ctx->current_brush.blue2 = d_blue;
    ctx->current_brush.alpha2 = d_alpha;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_pattern_brush (rl2GraphicsContextPtr context,
			     rl2GraphicsPatternPtr brush)
{
/* setting up a Pattern Brush */
    RL2GraphPatternBrushPtr pattern = (RL2GraphPatternBrushPtr) brush;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (pattern == NULL)
	return 0;

    ctx->current_brush.is_solid_color = 0;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 1;

    ctx->current_brush.pattern = pattern->pattern;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_font (rl2GraphicsContextPtr context, rl2GraphicsFontPtr font)
{
/* setting up the current font */
    cairo_t *cairo;
    int style = CAIRO_FONT_SLANT_NORMAL;
    int weight = CAIRO_FONT_WEIGHT_NORMAL;
    double size;
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (fnt == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    if (fnt->style == RL2_FONTSTYLE_ITALIC)
	style = CAIRO_FONT_SLANT_ITALIC;
    if (fnt->weight == RL2_FONTWEIGHT_BOLD)
	weight = CAIRO_FONT_WEIGHT_BOLD;
    cairo_select_font_face (cairo, "monospace", style, weight);
    size = fnt->size;
    if (fnt->is_outlined)
	size += fnt->outline_width;
    cairo_set_font_size (cairo, size);
    ctx->font_red = fnt->red;
    ctx->font_green = fnt->green;
    ctx->font_blue = fnt->blue;
    ctx->font_alpha = fnt->alpha;
    ctx->is_font_outlined = fnt->is_outlined;
    ctx->font_outline_width = fnt->outline_width;

    return 1;
}

static int
rl2cr_endian_arch ()
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
adjust_for_endianness (unsigned char *rgbaArray, int width, int height)
{
/* Adjusting from RGBA to ARGB respecting platform endianness */
    int x;
    int y;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    unsigned char *p_in = rgbaArray;
    unsigned char *p_out = rgbaArray;
    int little_endian = rl2cr_endian_arch ();

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		red = *p_in++;
		green = *p_in++;
		blue = *p_in++;
		alpha = *p_in++;
		if (little_endian)
		  {
		      *p_out++ = blue;
		      *p_out++ = green;
		      *p_out++ = red;
		      *p_out++ = alpha;
		  }
		else
		  {
		      *p_out++ = alpha;
		      *p_out++ = red;
		      *p_out++ = green;
		      *p_out++ = blue;
		  }
	    }
      }
}

RL2_DECLARE rl2GraphicsPatternPtr
rl2_graph_create_pattern (unsigned char *rgbaArray, int width, int height)
{
/* creating a pattern brush */
    RL2GraphPatternBrushPtr pattern;

    if (rgbaArray == NULL)
	return NULL;

    adjust_for_endianness (rgbaArray, width, height);
    pattern = malloc (sizeof (RL2GraphPatternBrush));
    if (pattern == NULL)
	return NULL;
    pattern->width = width;
    pattern->height = height;
    pattern->rgba = rgbaArray;
    pattern->bitmap =
	cairo_image_surface_create_for_data (rgbaArray, CAIRO_FORMAT_ARGB32,
					     width, height, width * 4);
    pattern->pattern = cairo_pattern_create_for_surface (pattern->bitmap);
    cairo_pattern_set_extend (pattern->pattern, CAIRO_EXTEND_REPEAT);
    return (rl2GraphicsPatternPtr) pattern;
}

RL2_DECLARE void
rl2_graph_destroy_pattern (rl2GraphicsPatternPtr brush)
{
/* destroying a pattern brush */
    RL2GraphPatternBrushPtr pattern = (RL2GraphPatternBrushPtr) brush;

    if (pattern == NULL)
	return;

    cairo_pattern_destroy (pattern->pattern);
    cairo_surface_destroy (pattern->bitmap);
    if (pattern->rgba != NULL)
	free (pattern->rgba);
    free (pattern);
}

RL2_DECLARE rl2GraphicsFontPtr
rl2_graph_create_font (double size, int style, int weight)
{
/* creating a font */
    RL2GraphFontPtr fnt;

    fnt = malloc (sizeof (RL2GraphFont));
    if (fnt == NULL)
	return NULL;
    if (size < 1.0)
	fnt->size = 1.0;
    else if (size > 32.0)
	fnt->size = 32.0;
    else
	fnt->size = size;
    if (style == RL2_FONTSTYLE_ITALIC)
	fnt->style = RL2_FONTSTYLE_ITALIC;
    else
	fnt->style = RL2_FONTSTYLE_NORMAL;
    if (weight == RL2_FONTWEIGHT_BOLD)
	fnt->weight = RL2_FONTWEIGHT_BOLD;
    else
	fnt->weight = RL2_FONTWEIGHT_NORMAL;
    fnt->is_outlined = 0;
    fnt->outline_width = 0.0;
    fnt->red = 0.0;
    fnt->green = 0.0;
    fnt->blue = 0.0;
    fnt->alpha = 1.0;
    return (rl2GraphicsFontPtr) fnt;
}

RL2_DECLARE void
rl2_graph_destroy_font (rl2GraphicsFontPtr font)
{
/* destroying a font */
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;

    if (fnt == NULL)
	return;

    free (fnt);
}

RL2_DECLARE int
rl2_graph_font_set_color (rl2GraphicsFontPtr font, unsigned char red,
			  unsigned char green, unsigned char blue,
			  unsigned char alpha)
{
/* setting up the font color */
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;

    if (fnt == NULL)
	return 0;

    fnt->red = (double) red / 255.0;
    fnt->green = (double) green / 255.0;
    fnt->blue = (double) blue / 255.0;
    fnt->alpha = (double) alpha / 255.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_font_set_outline (rl2GraphicsFontPtr font, double width)
{
/* setting up the font outline */
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;

    if (fnt == NULL)
	return 0;

    if (width <= 0.0)
      {
	  fnt->is_outlined = 0;
	  fnt->outline_width = 0.0;
      }
    else
      {
	  fnt->is_outlined = 1;
	  fnt->outline_width = width;
      }
    return 1;
}

RL2_DECLARE rl2GraphicsBitmapPtr
rl2_graph_create_bitmap (unsigned char *rgbaArray, int width, int height)
{
/* creating a bitmap */
    RL2GraphBitmapPtr bmp;

    if (rgbaArray == NULL)
	return NULL;

    adjust_for_endianness (rgbaArray, width, height);
    bmp = malloc (sizeof (RL2GraphBitmap));
    if (bmp == NULL)
	return NULL;
    bmp->width = width;
    bmp->height = height;
    bmp->rgba = rgbaArray;
    bmp->bitmap =
	cairo_image_surface_create_for_data (rgbaArray, CAIRO_FORMAT_ARGB32,
					     width, height, width * 4);
    bmp->pattern = cairo_pattern_create_for_surface (bmp->bitmap);
    return (rl2GraphicsBitmapPtr) bmp;
}

RL2_DECLARE void
rl2_graph_destroy_bitmap (rl2GraphicsBitmapPtr bitmap)
{
/* destroying a bitmap */
    RL2GraphBitmapPtr bmp = (RL2GraphBitmapPtr) bitmap;

    if (bmp == NULL)
	return;

    cairo_pattern_destroy (bmp->pattern);
    cairo_surface_destroy (bmp->bitmap);
    if (bmp->rgba != NULL)
	free (bmp->rgba);
    free (bmp);
}

static void
set_current_brush (RL2GraphContextPtr ctx)
{
/* setting up the current Brush */
    cairo_t *cairo;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    if (ctx->current_brush.is_solid_color)
      {
	  /* using a Solid Color Brush */
	  cairo_set_source_rgba (cairo, ctx->current_brush.red,
				 ctx->current_brush.green,
				 ctx->current_brush.blue,
				 ctx->current_brush.alpha);
      }
    else if (ctx->current_brush.is_linear_gradient)
      {
	  /* using a Linear Gradient Brush */
	  cairo_pattern_t *pattern =
	      cairo_pattern_create_linear (ctx->current_brush.x0,
					   ctx->current_brush.y0,
					   ctx->current_brush.x1,
					   ctx->current_brush.y1);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0,
					     ctx->current_brush.red,
					     ctx->current_brush.green,
					     ctx->current_brush.blue,
					     ctx->current_brush.alpha);
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0,
					     ctx->current_brush.red2,
					     ctx->current_brush.green2,
					     ctx->current_brush.blue2,
					     ctx->current_brush.alpha2);
	  cairo_set_source (cairo, pattern);
	  cairo_pattern_destroy (pattern);
      }
    else if (ctx->current_brush.is_pattern)
      {
	  /* using a Pattern Brush */
	  cairo_set_source (cairo, ctx->current_brush.pattern);
      }
}

static void
set_current_pen (RL2GraphContextPtr ctx)
{
/* setting up the current Pen */
    cairo_t *cairo;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_set_line_width (cairo, ctx->current_pen.width);
    cairo_set_source_rgba (cairo, ctx->current_pen.red,
			   ctx->current_pen.green, ctx->current_pen.blue,
			   ctx->current_pen.alpha);
    cairo_set_line_cap (cairo, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join (cairo, CAIRO_LINE_JOIN_MITER);
    cairo_set_dash (cairo, ctx->current_pen.lengths,
		    ctx->current_pen.lengths_count, 0.0);
}

RL2_DECLARE int
rl2_graph_draw_rectangle (rl2GraphicsContextPtr context, double x, double y,
			  double width, double height)
{
/* Drawing a filled rectangle */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_rectangle (cairo, x, y, width, height);
    set_current_brush (ctx);
    cairo_fill_preserve (cairo);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_rounded_rectangle (rl2GraphicsContextPtr context, double x,
				  double y, double width, double height,
				  double radius)
{
/* Drawing a filled rectangle with rounded corners */
    cairo_t *cairo;
    double degrees = M_PI / 180.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_new_sub_path (cairo);
    cairo_arc (cairo, x + width - radius, y + radius, radius,
	       -90 * degrees, 0 * degrees);
    cairo_arc (cairo, x + width - radius, y + height - radius, radius,
	       0 * degrees, 90 * degrees);
    cairo_arc (cairo, x + radius, y + height - radius, radius,
	       90 * degrees, 180 * degrees);
    cairo_arc (cairo, x + radius, y + radius, radius, 180 * degrees,
	       270 * degrees);
    cairo_close_path (cairo);
    set_current_brush (ctx);
    cairo_fill_preserve (cairo);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_ellipse (rl2GraphicsContextPtr context, double x, double y,
			double width, double height)
{
/* Drawing a filled ellipse */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_save (cairo);
    cairo_translate (cairo, x + (width / 2.0), y + (height / 2.0));
    cairo_scale (cairo, width / 2.0, height / 2.0);
    cairo_arc (cairo, 0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
    cairo_restore (cairo);
    set_current_brush (ctx);
    cairo_fill_preserve (cairo);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_circle_sector (rl2GraphicsContextPtr context, double center_x,
			      double center_y, double radius, double from_angle,
			      double to_angle)
{
/* drawing a filled circular sector */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_move_to (cairo, center_x, center_y);
    cairo_arc (cairo, center_x, center_y, radius, from_angle, to_angle);
    cairo_line_to (cairo, center_x, center_y);
    set_current_brush (ctx);
    cairo_fill_preserve (cairo);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_stroke_line (rl2GraphicsContextPtr context, double x0, double y0,
		       double x1, double y1)
{
/* Stroking a line */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_move_to (cairo, x0, y0);
    cairo_line_to (cairo, x1, y1);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_move_to_point (rl2GraphicsContextPtr context, double x, double y)
{
/* Moving to a Path Point */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_move_to (cairo, x, y);
    return 1;
}

RL2_DECLARE int
rl2_graph_add_line_to_path (rl2GraphicsContextPtr context, double x, double y)
{
/* Adding a Line to a Path */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_line_to (cairo, x, y);
    return 1;
}

RL2_DECLARE int
rl2_graph_close_subpath (rl2GraphicsContextPtr context)
{
/* Closing a SubPath */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_close_path (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_stroke_path (rl2GraphicsContextPtr context, int preserve)
{
/* Stroking a path */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    set_current_pen (ctx);
    if (preserve == RL2_PRESERVE_PATH)
	cairo_stroke_preserve (cairo);
    else
	cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_fill_path (rl2GraphicsContextPtr context, int preserve)
{
/* Filling a path */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    set_current_brush (ctx);
    if (preserve == RL2_PRESERVE_PATH)
	cairo_fill_preserve (cairo);
    else
	cairo_fill (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_get_text_extent (rl2GraphicsContextPtr context, const char *text,
			   double *pre_x, double *pre_y, double *width,
			   double *height, double *post_x, double *post_y)
{
/* measuring the text extent (using the current font) */
    cairo_t *cairo;
    cairo_text_extents_t extents;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_text_extents (cairo, text, &extents);
    *pre_x = extents.x_bearing;
    *pre_y = extents.y_bearing;
    *width = extents.width;
    *height = extents.height;
    *post_x = extents.x_advance;
    *post_y = extents.y_advance;
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_text (rl2GraphicsContextPtr context, const char *text, double x,
		     double y, double angle)
{
/* drawing a text string (using the current font) */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    cairo_rotate (cairo, angle);
    if (ctx->is_font_outlined)
      {
	  /* outlined font */
	  cairo_move_to (cairo, 0.0, 0.0);
	  cairo_text_path (cairo, text);
	  cairo_set_source_rgba (cairo, ctx->font_red, ctx->font_green,
				 ctx->font_blue, ctx->font_alpha);
	  cairo_fill_preserve (cairo);
	  cairo_set_source_rgba (cairo, 1.0, 1.0, 1.0, ctx->font_alpha);
	  cairo_set_line_width (cairo, ctx->font_outline_width);
	  cairo_stroke (cairo);
      }
    else
      {
	  /* no outline */
	  cairo_set_source_rgba (cairo, ctx->font_red, ctx->font_green,
				 ctx->font_blue, ctx->font_alpha);
	  cairo_move_to (cairo, 0.0, 0.0);
	  cairo_show_text (cairo, text);
      }
    cairo_restore (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_bitmap (rl2GraphicsContextPtr context,
		       rl2GraphicsBitmapPtr bitmap, int x, int y)
{
/* drawing a symbol bitmap */
    cairo_t *cairo;
    cairo_surface_t *surface;
    RL2GraphBitmapPtr bmp = (RL2GraphBitmapPtr) bitmap;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (bmp == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
      {
	  surface = ctx->clip_surface;
	  cairo = ctx->clip_cairo;
      }
    else
      {
	  surface = ctx->surface;
	  cairo = ctx->cairo;
      }

    cairo_save (cairo);
    cairo_scale (cairo, 1, 1);
    cairo_translate (cairo, x, y);
    cairo_set_source (cairo, bmp->pattern);
    cairo_rectangle (cairo, 0, 0, bmp->width, bmp->height);
    cairo_fill (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_rescaled_bitmap (rl2GraphicsContextPtr context,
				rl2GraphicsBitmapPtr bitmap, double scale_x,
				double scale_y, int x, int y)
{
/* drawing a rescaled bitmap */
    cairo_t *cairo;
    cairo_surface_t *surface;
    RL2GraphBitmapPtr bmp = (RL2GraphBitmapPtr) bitmap;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (bmp == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
      {
	  surface = ctx->clip_surface;
	  cairo = ctx->clip_cairo;
      }
    else
      {
	  surface = ctx->surface;
	  cairo = ctx->cairo;
      }

    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    cairo_scale (cairo, scale_x, scale_y);
    cairo_set_source (cairo, bmp->pattern);
    cairo_paint (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);
    return 1;
}

RL2_DECLARE unsigned char *
rl2_graph_get_context_rgb_array (rl2GraphicsContextPtr context)
{
/* creating an RGB buffer from the given Context */
    int width;
    int height;
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *rgb;
    int little_endian = rl2cr_endian_arch ();
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return NULL;

    width = cairo_image_surface_get_width (ctx->surface);
    height = cairo_image_surface_get_height (ctx->surface);
    rgb = malloc (width * height * 3);
    if (rgb == NULL)
	return NULL;

    p_in = cairo_image_surface_get_data (ctx->surface);
    p_out = rgb;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		if (little_endian)
		  {
		      b = *p_in++;
		      g = *p_in++;
		      r = *p_in++;
		      p_in++;	/* skipping Alpha */
		  }
		else
		  {
		      p_in++;	/* skipping Alpha */
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		*p_out++ = r;
		*p_out++ = g;
		*p_out++ = b;
	    }
      }
    return rgb;
}

RL2_DECLARE unsigned char *
rl2_graph_get_context_alpha_array (rl2GraphicsContextPtr context)
{
/* creating an Alpha buffer from the given Context */
    int width;
    int height;
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *alpha;
    int little_endian = rl2cr_endian_arch ();
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return NULL;

    width = cairo_image_surface_get_width (ctx->surface);
    height = cairo_image_surface_get_height (ctx->surface);
    alpha = malloc (width * height);
    if (alpha == NULL)
	return NULL;

    p_in = cairo_image_surface_get_data (ctx->surface);
    p_out = alpha;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if (little_endian)
		  {
		      p_in += 3;	/* skipping RGB */
		      *p_out++ = *p_in++;
		  }
		else
		  {
		      *p_out++ = *p_in++;
		      p_in += 3;	/* skipping RGB */
		  }
	    }
      }
    return alpha;
}

RL2_DECLARE int
rl2_rgba_to_pdf (unsigned int width, unsigned int height,
		 unsigned char *rgba, unsigned char **pdf, int *pdf_size)
{
/* attempting to create an RGB PDF map */
    rl2MemPdfPtr mem = NULL;
    rl2GraphicsContextPtr ctx = NULL;
    rl2GraphicsBitmapPtr bmp = NULL;
    int dpi;
    double w_150 = (double) width / 150.0;
    double h_150 = (double) height / 150.0;
    double w_300 = (double) width / 300.0;
    double h_300 = (double) height / 300.0;
    double w_600 = (double) width / 600.0;
    double h_600 = (double) height / 600.0;
    double page_width;
    double page_height;

    if (w_150 <= 6.3 && h_150 <= 9.7)
      {
	  /* A4 portrait - 150 DPI */
	  page_width = 8.3;
	  page_height = 11.7;
	  dpi = 150;
      }
    else if (w_150 <= 9.7 && h_150 < 6.3)
      {
	  /* A4 landscape - 150 DPI */
	  page_width = 11.7;
	  page_height = 8.3;;
	  dpi = 150;
      }
    else if (w_300 <= 6.3 && h_300 <= 9.7)
      {
	  /* A4 portrait - 300 DPI */
	  page_width = 8.3;
	  page_height = 11.7;;
	  dpi = 300;
      }
    else if (w_300 <= 9.7 && h_300 < 6.3)
      {
	  /* A4 landscape - 300 DPI */
	  page_width = 11.7;
	  page_height = 8.3;;
	  dpi = 300;
      }
    else if (w_600 <= 6.3 && h_600 <= 9.7)
      {
	  /* A4 portrait - 600 DPI */
	  page_width = 8.3;
	  page_height = 11.7;;
	  dpi = 600;
      }
    else
      {
	  /* A4 landscape - 600 DPI */
	  page_width = 11.7;
	  page_height = 8.3;;
	  dpi = 600;
      }

    mem = rl2_create_mem_pdf_target ();
    if (mem == NULL)
	goto error;

    ctx =
	rl2_graph_create_mem_pdf_context (mem, dpi, page_width, page_height,
					  1.0, 1.0);
    if (ctx == NULL)
	goto error;
    bmp = rl2_graph_create_bitmap (rgba, width, height);
    rgba = NULL;
    if (bmp == NULL)
	goto error;
/* rendering the Bitmap */
    if (ctx != NULL)
	rl2_graph_draw_bitmap (ctx, bmp, 0, 0);
    rl2_graph_destroy_bitmap (bmp);
    rl2_graph_destroy_context (ctx);
/* retrieving the PDF memory block */
    if (rl2_get_mem_pdf_buffer (mem, pdf, pdf_size) != RL2_OK)
	goto error;
    rl2_destroy_mem_pdf_target (mem);

    return RL2_OK;

  error:
    if (bmp != NULL)
	rl2_graph_destroy_bitmap (bmp);
    if (ctx != NULL)
	rl2_graph_destroy_context (ctx);
    if (mem != NULL)
	rl2_destroy_mem_pdf_target (mem);
    return RL2_ERROR;
}

RL2_DECLARE rl2MemPdfPtr
rl2_create_mem_pdf_target (void)
{
/* creating an initially empty in-memory PDF target */
    rl2PrivMemPdfPtr mem = malloc (sizeof (rl2PrivMemPdf));
    if (mem == NULL)
	return NULL;
    mem->write_offset = 0;
    mem->size = 64 * 1024;
    mem->buffer = malloc (mem->size);
    if (mem->buffer == NULL)
      {
	  free (mem);
	  return NULL;
      }
    return (rl2MemPdfPtr) mem;
}

RL2_DECLARE void
rl2_destroy_mem_pdf_target (rl2MemPdfPtr target)
{
/* memory cleanup - destroying an in-memory PDF target */
    rl2PrivMemPdfPtr mem = (rl2PrivMemPdfPtr) target;
    if (mem == NULL)
	return;
    if (mem->buffer != NULL)
	free (mem->buffer);
    free (mem);
}

RL2_DECLARE int
rl2_get_mem_pdf_buffer (rl2MemPdfPtr target, unsigned char **buffer, int *size)
{
/* exporting the internal buffer */
    rl2PrivMemPdfPtr mem = (rl2PrivMemPdfPtr) target;
    if (mem == NULL)
	return RL2_ERROR;
    if (mem->buffer == NULL)
	return RL2_ERROR;
    *buffer = mem->buffer;
    mem->buffer = NULL;
    *size = mem->write_offset;
    return RL2_OK;
}
