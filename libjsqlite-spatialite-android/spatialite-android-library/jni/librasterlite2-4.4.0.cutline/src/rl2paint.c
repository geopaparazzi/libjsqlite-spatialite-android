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
#include "rasterlite2/rl2svg.h"
#include "rasterlite2_private.h"

#ifdef __ANDROID__		/* Android specific */
#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#include <cairo-ft.h>
#else /* any other standard platform (Win, Linux, Mac) */
#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ft.h>
#endif /* end Android conditionals */

#define RL2_SURFACE_IMG 2671
#define RL2_SURFACE_SVG 1267
#define RL2_SURFACE_PDF	1276

struct rl2_graphics_pen
{
/* a struct wrapping a Cairo Pen */
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
    double width;
    double *dash_array;
    int dash_count;
    double dash_offset;
    int line_cap;
    int line_join;
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
    int with_font_halo;
    double halo_radius;
    double halo_red;
    double halo_green;
    double halo_blue;
    double halo_alpha;
} RL2GraphContext;
typedef RL2GraphContext *RL2GraphContextPtr;

typedef struct rl2_priv_graphics_pattern
{
/* a Cairo based pattern */
    int width;
    int height;
    unsigned char *rgba;
    cairo_surface_t *bitmap;
    cairo_pattern_t *pattern;
} RL2PrivGraphPattern;
typedef RL2PrivGraphPattern *RL2PrivGraphPatternPtr;

typedef struct rl2_graphics_font
{
/* a struct wrapping a Cairo/FreeType Font */
    int toy_font;
    char *facename;
    cairo_font_face_t *cairo_font;
    cairo_scaled_font_t *cairo_scaled_font;
    struct rl2_private_tt_font *tt_font;
    double size;
    double font_red;
    double font_green;
    double font_blue;
    double font_alpha;
    int with_halo;
    double halo_radius;
    double halo_red;
    double halo_green;
    double halo_blue;
    double halo_alpha;
    int style;
    int weight;
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
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
    ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_count = 0;
    ctx->current_pen.dash_offset = 0.0;
    ctx->current_pen.pattern = NULL;

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
    ctx->with_font_halo = 0;
    ctx->halo_radius = 0.0;
    ctx->halo_red = 1.0;
    ctx->halo_green = 1.0;
    ctx->halo_blue = 1.0;
    ctx->halo_alpha = 1.0;
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
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    cairo_destroy (ctx->cairo);
    cairo_surface_finish (ctx->surface);
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
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
    ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_count = 0;
    ctx->current_pen.dash_offset = 0.0;
    ctx->current_pen.pattern = NULL;

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
    ctx->with_font_halo = 0;
    ctx->halo_radius = 0.0;
    ctx->halo_red = 1.0;
    ctx->halo_green = 1.0;
    ctx->halo_blue = 1.0;
    ctx->halo_alpha = 1.0;
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
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
    ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_count = 0;
    ctx->current_pen.dash_offset = 0.0;
    ctx->current_pen.pattern = NULL;

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
    ctx->with_font_halo = 0;
    ctx->halo_radius = 0.0;
    ctx->halo_red = 1.0;
    ctx->halo_green = 1.0;
    ctx->halo_blue = 1.0;
    ctx->halo_alpha = 1.0;
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
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
    ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_count = 0;
    ctx->current_pen.dash_offset = 0.0;
    ctx->current_pen.pattern = NULL;

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
    ctx->with_font_halo = 0;
    ctx->halo_radius = 0.0;
    ctx->halo_red = 1.0;
    ctx->halo_green = 1.0;
    ctx->halo_blue = 1.0;
    ctx->halo_alpha = 1.0;
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
rl2_graph_set_solid_pen (rl2GraphicsContextPtr context, unsigned char red,
			 unsigned char green, unsigned char blue,
			 unsigned char alpha, double width, int line_cap,
			 int line_join)
{
/* creating a Color Pen - solid style */
    double d_red = (double) red / 255.0;
    double d_green = (double) green / 255.0;
    double d_blue = (double) blue / 255.0;
    double d_alpha = (double) alpha / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_pen.width = width;
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.dash_count = 0;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_offset = 0.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_dashed_pen (rl2GraphicsContextPtr context, unsigned char red,
			  unsigned char green, unsigned char blue,
			  unsigned char alpha, double width, int line_cap,
			  int line_join, int dash_count, double dash_list[],
			  double dash_offset)
{
/* creating a Color Pen - dashed style */
    int d;
    double d_red = (double) red / 255.0;
    double d_green = (double) green / 255.0;
    double d_blue = (double) blue / 255.0;
    double d_alpha = (double) alpha / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (dash_count <= 0 || dash_list == NULL)
	return 0;

    ctx->current_pen.width = width;
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.dash_count = dash_count;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = malloc (sizeof (double) * dash_count);
    for (d = 0; d < dash_count; d++)
	*(ctx->current_pen.dash_array + d) = *(dash_list + d);
    ctx->current_pen.dash_offset = dash_offset;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_linear_gradient_solid_pen (rl2GraphicsContextPtr context,
					 double x, double y, double width,
					 double height, unsigned char red1,
					 unsigned char green1,
					 unsigned char blue1,
					 unsigned char alpha1,
					 unsigned char red2,
					 unsigned char green2,
					 unsigned char blue2,
					 unsigned char alpha2,
					 double pen_width, int line_cap,
					 int line_join)
{
/* setting up a Linear Gradient Pen - solid style */
    double d_red = (double) red1 / 255.0;
    double d_green = (double) green1 / 255.0;
    double d_blue = (double) blue1 / 255.0;
    double d_alpha = (double) alpha1 / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_pen.width = pen_width;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.is_solid_color = 0;
    ctx->current_pen.is_linear_gradient = 1;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    ctx->current_pen.x0 = x;
    ctx->current_pen.y0 = y;
    ctx->current_pen.x1 = x + width;
    ctx->current_pen.y1 = y + height;
    d_red = (double) red2 / 255.0;
    d_green = (double) green2 / 255.0;
    d_blue = (double) blue2 / 255.0;
    d_alpha = (double) alpha2 / 255.0;
    ctx->current_pen.red2 = d_red;
    ctx->current_pen.green2 = d_green;
    ctx->current_pen.blue2 = d_blue;
    ctx->current_pen.alpha2 = d_alpha;
    ctx->current_pen.dash_count = 0;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_offset = 0.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_linear_gradient_dashed_pen (rl2GraphicsContextPtr context,
					  double x, double y, double width,
					  double height, unsigned char red1,
					  unsigned char green1,
					  unsigned char blue1,
					  unsigned char alpha1,
					  unsigned char red2,
					  unsigned char green2,
					  unsigned char blue2,
					  unsigned char alpha2,
					  double pen_width, int line_cap,
					  int line_join, int dash_count,
					  double dash_list[],
					  double dash_offset)
{
/* setting up a Linear Gradient Pen - dashed style */
    int d;
    double d_red = (double) red1 / 255.0;
    double d_green = (double) green1 / 255.0;
    double d_blue = (double) blue1 / 255.0;
    double d_alpha = (double) alpha1 / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (dash_count <= 0 || dash_list == NULL)
	return 0;

    ctx->current_pen.width = pen_width;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.is_solid_color = 0;
    ctx->current_pen.is_linear_gradient = 1;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    ctx->current_pen.x0 = x;
    ctx->current_pen.y0 = y;
    ctx->current_pen.x1 = x + width;
    ctx->current_pen.y1 = y + height;
    d_red = (double) red2 / 255.0;
    d_green = (double) green2 / 255.0;
    d_blue = (double) blue2 / 255.0;
    d_alpha = (double) alpha2 / 255.0;
    ctx->current_pen.red2 = d_red;
    ctx->current_pen.green2 = d_green;
    ctx->current_pen.blue2 = d_blue;
    ctx->current_pen.alpha2 = d_alpha;
    ctx->current_pen.dash_count = dash_count;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = malloc (sizeof (double) * dash_count);
    for (d = 0; d < dash_count; d++)
	*(ctx->current_pen.dash_array + d) = *(dash_list + d);
    ctx->current_pen.dash_offset = dash_offset;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_pattern_solid_pen (rl2GraphicsContextPtr context,
				 rl2GraphicsPatternPtr brush,
				 double width, int line_cap, int line_join)
{
/* setting up a Pattern Pen - solid style */
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) brush;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (pattern == NULL)
	return 0;

    ctx->current_pen.width = width;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.is_solid_color = 0;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 1;
    ctx->current_pen.pattern = pattern->pattern;
    ctx->current_pen.dash_count = 0;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_offset = 0.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_pattern_dashed_pen (rl2GraphicsContextPtr context,
				  rl2GraphicsPatternPtr brush,
				  double width, int line_cap, int line_join,
				  int dash_count, double dash_list[],
				  double dash_offset)
{
/* setting up a Pattern Pen - dashed style */
    int d;
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) brush;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (pattern == NULL)
	return 0;
    if (dash_count <= 0 || dash_list == NULL)
	return 0;

    ctx->current_pen.width = width;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.is_solid_color = 0;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 1;
    ctx->current_pen.pattern = pattern->pattern;
    ctx->current_pen.dash_count = dash_count;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = malloc (sizeof (double) * dash_count);
    for (d = 0; d < dash_count; d++)
	*(ctx->current_pen.dash_array + d) = *(dash_list + d);
    ctx->current_pen.dash_offset = dash_offset;
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
				     unsigned char blue1,
				     unsigned char alpha1, unsigned char red2,
				     unsigned char green2,
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
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) brush;
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

static void
rl2_priv_graph_destroy_font (RL2GraphFontPtr fnt)
{
/* destroying a font (not yet cached) */
    if (fnt == NULL)
	return;

    if (fnt->tt_font != NULL)
      {
	  if (fnt->tt_font->facename != NULL)
	      free (fnt->tt_font->facename);
	  if (fnt->tt_font->FTface != NULL)
	      FT_Done_Face ((FT_Face) (fnt->tt_font->FTface));
	  if (fnt->tt_font->ttf_data != NULL)
	      free (fnt->tt_font->ttf_data);
      }
    free (fnt);
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

    size = fnt->size;
    ctx->font_red = fnt->font_red;
    ctx->font_green = fnt->font_green;
    ctx->font_blue = fnt->font_blue;
    ctx->font_alpha = fnt->font_alpha;
    ctx->with_font_halo = fnt->with_halo;
    if (fnt->with_halo)
      {
	  ctx->halo_radius = fnt->halo_radius;
	  ctx->halo_red = fnt->halo_red;
	  ctx->halo_green = fnt->halo_green;
	  ctx->halo_blue = fnt->halo_blue;
	  ctx->halo_alpha = fnt->halo_alpha;
	  size += fnt->halo_radius;
      }
    if (fnt->toy_font)
      {
	  /* using a CAIRO built-in "toy" font */
	  if (fnt->style == RL2_FONTSTYLE_ITALIC)
	      style = CAIRO_FONT_SLANT_ITALIC;
	  if (fnt->style == RL2_FONTSTYLE_OBLIQUE)
	      style = CAIRO_FONT_SLANT_OBLIQUE;
	  if (fnt->weight == RL2_FONTWEIGHT_BOLD)
	      weight = CAIRO_FONT_WEIGHT_BOLD;
	  cairo_select_font_face (cairo, fnt->facename, style, weight);
	  cairo_set_font_size (cairo, size);
      }
    else
      {
	  /* using a TrueType font */
	  cairo_font_options_t *font_options = cairo_font_options_create ();
	  cairo_matrix_t ctm;
	  cairo_matrix_t font_matrix;
	  cairo_get_matrix (cairo, &ctm);
	  cairo_matrix_init (&font_matrix, size, 0.0, 0.0, size, 0.0, 0.0);
	  fnt->cairo_scaled_font =
	      cairo_scaled_font_create (fnt->cairo_font, &font_matrix, &ctm,
					font_options);
	  cairo_font_options_destroy (font_options);
	  cairo_set_scaled_font (cairo, fnt->cairo_scaled_font);
      }

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
rl2_graph_create_pattern (unsigned char *rgbaArray, int width, int height,
			  int extend)
{
/* creating a pattern brush */
    RL2PrivGraphPatternPtr pattern;

    if (rgbaArray == NULL)
	return NULL;

    adjust_for_endianness (rgbaArray, width, height);
    pattern = malloc (sizeof (RL2PrivGraphPattern));
    if (pattern == NULL)
	return NULL;
    pattern->width = width;
    pattern->height = height;
    pattern->rgba = rgbaArray;
    pattern->bitmap =
	cairo_image_surface_create_for_data (rgbaArray, CAIRO_FORMAT_ARGB32,
					     width, height, width * 4);
    pattern->pattern = cairo_pattern_create_for_surface (pattern->bitmap);
    if (extend)
	cairo_pattern_set_extend (pattern->pattern, CAIRO_EXTEND_REPEAT);
    else
	cairo_pattern_set_extend (pattern->pattern, CAIRO_EXTEND_NONE);
    return (rl2GraphicsPatternPtr) pattern;
}

RL2_DECLARE rl2GraphicsPatternPtr
rl2_create_pattern_from_external_graphic (sqlite3 * handle,
					  const char *xlink_href, int extend)
{
/* creating a pattern brush from an External Graphic resource */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char *rgbaArray = NULL;
    int rgbaSize;
    unsigned int width;
    unsigned int height;
    if (xlink_href == NULL)
	return NULL;

/* preparing the SQL query statement */
    sql =
	"SELECT resource, GetMimeType(resource) FROM SE_external_graphics "
	"WHERE Lower(xlink_href) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href), SQLITE_STATIC);
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
		      const char *mime =
			  (const char *) sqlite3_column_text (stmt, 1);
		      if (strcmp (mime, "image/jpeg") == 0)
			{
			    if (raster != NULL)
				rl2_destroy_raster (raster);
			    raster = rl2_raster_from_jpeg (blob, blob_sz);
			}
		      if (strcmp (mime, "image/png") == 0)
			{
			    if (raster != NULL)
				rl2_destroy_raster (raster);
			    raster = rl2_raster_from_png (blob, blob_sz, 1);
			}
		      if (strcmp (mime, "image/gif") == 0)
			{
			    if (raster != NULL)
				rl2_destroy_raster (raster);
			    raster = rl2_raster_from_gif (blob, blob_sz);
			}
		  }
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (raster == NULL)
	goto error;

/* retieving the raster RGBA map */
    if (rl2_get_raster_size (raster, &width, &height) == RL2_OK)
      {
	  if (rl2_raster_data_to_RGBA (raster, &rgbaArray, &rgbaSize) != RL2_OK)
	      rgbaArray = NULL;
      }
    rl2_destroy_raster (raster);
    raster = NULL;
    if (rgbaArray == NULL)
	goto error;
    return rl2_graph_create_pattern (rgbaArray, width, height, extend);

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return NULL;
}

RL2_DECLARE rl2GraphicsPatternPtr
rl2_create_pattern_from_external_svg (sqlite3 * handle,
				      const char *xlink_href, double size)
{
/* creating a pattern brush from an External SVG resource */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char *rgbaArray = NULL;
    int rgbaSize;
    unsigned int width;
    unsigned int height;
    if (xlink_href == NULL)
	return NULL;
    if (size <= 0.0)
	return NULL;

/* preparing the SQL query statement */
    sql =
	"SELECT XB_GetPayload(resource) FROM SE_external_graphics "
	"WHERE Lower(xlink_href) = Lower(?) AND "
	"GetMimeType(resource) = 'image/svg+xml'";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href), SQLITE_STATIC);
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
		      rl2SvgPtr svg_handle = rl2_create_svg (blob, blob_sz);
		      if (svg_handle != NULL)
			{
			    double svgWidth;
			    double svgHeight;
			    if (rl2_get_svg_size
				(svg_handle, &svgWidth, &svgHeight) == RL2_OK)
			      {
				  double sz;
				  double w = svgWidth;
				  double h = svgHeight;
				  if (w < size && h < size)
				    {
					while (w < size && h < size)
					  {
					      /* rescaling */
					      w *= 1.0001;
					      h *= 1.0001;
					  }
				    }
				  else
				    {
					while (w > size || h > size)
					  {
					      /* rescaling */
					      w *= 0.9;
					      h *= 0.9;
					  }
				    }
				  sz = w;
				  if (h > sz)
				      sz = h;
				  if (raster != NULL)
				      rl2_destroy_raster (raster);
				  raster =
				      rl2_raster_from_svg (svg_handle, size);
			      }
			    rl2_destroy_svg (svg_handle);
			}
		  }
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (raster == NULL)
	goto error;

/* retieving the raster RGBA map */
    if (rl2_get_raster_size (raster, &width, &height) == RL2_OK)
      {
	  if (rl2_raster_data_to_RGBA (raster, &rgbaArray, &rgbaSize) != RL2_OK)
	      rgbaArray = NULL;
      }
    rl2_destroy_raster (raster);
    raster = NULL;
    if (rgbaArray == NULL)
	goto error;
    return rl2_graph_create_pattern (rgbaArray, width, height, 0);

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return NULL;
}

RL2_DECLARE void
rl2_graph_destroy_pattern (rl2GraphicsPatternPtr brush)
{
/* destroying a pattern brush */
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) brush;

    if (pattern == NULL)
	return;

    cairo_pattern_destroy (pattern->pattern);
    cairo_surface_destroy (pattern->bitmap);
    if (pattern->rgba != NULL)
	free (pattern->rgba);
    free (pattern);
}

RL2_DECLARE int
rl2_graph_get_pattern_size (rl2GraphicsPatternPtr ptr,
			    unsigned int *width, unsigned int *height)
{
/* will return the Pattern dimensions */
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) ptr;

    if (pattern == NULL)
	return RL2_ERROR;
    *width = pattern->width;
    *height = pattern->height;
    return RL2_OK;
}

static void
aux_pattern_get_pixel (int x, int y, int width, unsigned char *bitmap,
		       unsigned char *red, unsigned char *green,
		       unsigned char *blue, unsigned char *alpha)
{
/* get pixel */
    unsigned char *p_in = bitmap + (y * width * 4) + (x * 4);
    int little_endian = rl2cr_endian_arch ();
    if (little_endian)
      {
	  *blue = *p_in++;
	  *green = *p_in++;
	  *red = *p_in++;
	  *alpha = *p_in++;
      }
    else
      {
	  *alpha = *p_in++;
	  *red = *p_in++;
	  *green = *p_in++;
	  *blue = *p_in++;
      }
}

static void
aux_pattern_set_pixel (int x, int y, int width, unsigned char *bitmap,
		       unsigned char red, unsigned char green,
		       unsigned char blue, unsigned char alpha)
{
/* set pixel */
    unsigned char *p_out = bitmap + (y * width * 4) + (x * 4);
    int little_endian = rl2cr_endian_arch ();
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

RL2_DECLARE int
rl2_graph_pattern_recolor (rl2GraphicsPatternPtr ptrn, unsigned char r,
			   unsigned char g, unsigned char b)
{
/* recoloring a Monochrome Pattern */
    int x;
    int y;
    int width;
    int height;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    unsigned char xred;
    unsigned char xgreen;
    unsigned char xblue;
    unsigned char xalpha;
    int valid = 0;
    int has_black = 0;
    unsigned char *bitmap;
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) ptrn;
    if (pattern == NULL)
	return RL2_ERROR;

    width = pattern->width;
    height = pattern->height;
    cairo_surface_flush (pattern->bitmap);
    bitmap = cairo_image_surface_get_data (pattern->bitmap);
    if (bitmap == NULL)
	return RL2_ERROR;
/* checking for a Monochrome Pattern */
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		aux_pattern_get_pixel (x, y, width, bitmap, &red, &green,
				       &blue, &alpha);
		if (alpha != 0)
		  {
		      if (red < 64 && green < 64 && blue < 64)
			  has_black++;
		      if (valid)
			{
			    if (xred == red && xgreen == green
				&& xblue == blue && alpha == xalpha)
				;
			    else
				goto not_mono;
			}
		      else
			{
			    xred = red;
			    xgreen = green;
			    xblue = blue;
			    xalpha = alpha;
			    valid = 1;
			}
		  }
	    }
      }
/* all right, applying the new color */
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		aux_pattern_get_pixel (x, y, width, bitmap, &red, &green,
				       &blue, &alpha);
		if (alpha != 0)
		    aux_pattern_set_pixel (x, y, width, bitmap, r, g, b, alpha);
	    }
      }
    cairo_surface_mark_dirty (pattern->bitmap);
    return RL2_OK;

  not_mono:
    if (has_black)
      {
	  /* recoloring only the black pixels */
	  for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		  {
		      aux_pattern_get_pixel (x, y, width, bitmap, &red,
					     &green, &blue, &alpha);
		      if (red < 64 && green < 64 && blue < 64)
			  aux_pattern_set_pixel (x, y, width, bitmap, r, g, b,
						 alpha);
		  }
	    }
	  cairo_surface_mark_dirty (pattern->bitmap);
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_graph_pattern_transparency (rl2GraphicsPatternPtr ptrn, unsigned char aleph)
{
/* changing the Pattern's transparency */
    int x;
    int y;
    int width;
    int height;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    unsigned char *bitmap;
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) ptrn;
    if (pattern == NULL)
	return RL2_ERROR;

    width = pattern->width;
    height = pattern->height;
    cairo_surface_flush (pattern->bitmap);
    bitmap = cairo_image_surface_get_data (pattern->bitmap);
    if (bitmap == NULL)
	return RL2_ERROR;
/* applying the new transparency */
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		aux_pattern_get_pixel (x, y, width, bitmap, &red, &green,
				       &blue, &alpha);
		if (alpha != 0)
		    aux_pattern_set_pixel (x, y, width, bitmap, red, green,
					   blue, aleph);
	    }
      }
    cairo_surface_mark_dirty (pattern->bitmap);
    return RL2_OK;
}

RL2_DECLARE rl2GraphicsFontPtr
rl2_graph_create_toy_font (const char *facename, double size, int style,
			   int weight)
{
/* creating a font based on CAIRO internal "toy" fonts */
    RL2GraphFontPtr fnt;
    int len;

    fnt = malloc (sizeof (RL2GraphFont));
    if (fnt == NULL)
	return NULL;
    fnt->toy_font = 1;
    fnt->tt_font = NULL;
    if (facename == NULL)
	facename = "monospace";
    if (strcasecmp (facename, "serif") == 0)
      {
	  len = strlen ("serif");
	  fnt->facename = malloc (len + 1);
	  strcpy (fnt->facename, "serif");
      }
    else if (strcasecmp (facename, "sans-serif") == 0)
      {
	  len = strlen ("sans-serif");
	  fnt->facename = malloc (len + 1);
	  strcpy (fnt->facename, "sans-serif");
      }
    else
      {
	  /* defaulting to "monospace" */
	  len = strlen ("monospace");
	  fnt->facename = malloc (len + 1);
	  strcpy (fnt->facename, "monospace");
      }
    if (size < 1.0)
	fnt->size = 1.0;
    else if (size > 72.0)
	fnt->size = 72.0;
    else
	fnt->size = size;
    if (style == RL2_FONTSTYLE_ITALIC)
	fnt->style = RL2_FONTSTYLE_ITALIC;
    else if (style == RL2_FONTSTYLE_OBLIQUE)
	fnt->style = RL2_FONTSTYLE_OBLIQUE;
    else
	fnt->style = RL2_FONTSTYLE_NORMAL;
    if (weight == RL2_FONTWEIGHT_BOLD)
	fnt->weight = RL2_FONTWEIGHT_BOLD;
    else
	fnt->weight = RL2_FONTWEIGHT_NORMAL;
    fnt->font_red = 0.0;
    fnt->font_green = 0.0;
    fnt->font_blue = 0.0;
    fnt->font_alpha = 1.0;
    fnt->with_halo = 0;
    fnt->halo_radius = 0.0;
    fnt->halo_red = 0.0;
    fnt->halo_green = 0.0;
    fnt->halo_blue = 0.0;
    fnt->halo_alpha = 1.0;
    return (rl2GraphicsFontPtr) fnt;
}

static void
rl2_destroy_private_tt_font (struct rl2_private_tt_font *font)
{
/* destroying a private font */
    if (font == NULL)
	return;

    if (font->facename != NULL)
	free (font->facename);
    if (font->FTface != NULL)
	FT_Done_Face ((FT_Face) (font->FTface));
    if (font->ttf_data != NULL)
	free (font->ttf_data);
    free (font);
}

static void
rl2_font_destructor_callback (void *data)
{
/* font destructor callback */
    struct rl2_private_tt_font *font = (struct rl2_private_tt_font *) data;
    if (font == NULL)
	return;

/* adjusting the double-linked list */
    if (font == font->container->first_font
	&& font == font->container->last_font)
      {
	  /* going to remove the unique item from the list */
	  font->container->first_font = NULL;
	  font->container->last_font = NULL;
      }
    else if (font == font->container->first_font)
      {
	  /* going to remove the first item from the list */
	  font->next->prev = NULL;
	  font->container->first_font = font->next;
      }
    else if (font == font->container->last_font)
      {
	  /* going to remove the last item from the list */
	  font->prev->next = NULL;
	  font->container->last_font = font->prev;
      }
    else
      {
	  /* normal case */
	  font->prev->next = font->next;
	  font->next->prev = font->prev;
      }

/* destroying the cached font */
    rl2_destroy_private_tt_font (font);
}

RL2_DECLARE rl2GraphicsFontPtr
rl2_graph_create_TrueType_font (const void *priv_data,
				const unsigned char *ttf, int ttf_bytes,
				double size)
{
/* creating a TrueType font */
    RL2GraphFontPtr fnt;
    char *facename;
    int is_bold;
    int is_italic;
    unsigned char *font = NULL;
    int font_sz;
    FT_Error error;
    FT_Library library;
    FT_Face face;
    static const cairo_user_data_key_t key;
    struct rl2_private_data *cache = (struct rl2_private_data *) priv_data;
    struct rl2_private_tt_font *tt_font;
    if (cache == NULL)
	return NULL;
    if (cache->FTlibrary == NULL)
	return NULL;
    library = (FT_Library) (cache->FTlibrary);

/* testing the BLOB-encoded TTF object for validity */
    if (ttf == NULL || ttf_bytes <= 0)
	return NULL;
    if (rl2_is_valid_encoded_font (ttf, ttf_bytes) != RL2_OK)
	return NULL;
    facename = rl2_get_encoded_font_facename (ttf, ttf_bytes);
    if (facename == NULL)
	return NULL;
    is_bold = rl2_is_encoded_font_bold (ttf, ttf_bytes);
    is_italic = rl2_is_encoded_font_italic (ttf, ttf_bytes);

/* decoding the TTF BLOB */
    if (rl2_font_decode (ttf, ttf_bytes, &font, &font_sz) != RL2_OK)
	return NULL;
/* creating a FreeType font object */
    error = FT_New_Memory_Face (library, font, font_sz, 0, &face);
    if (error)
      {
	  free (facename);
	  return NULL;
      }

    fnt = malloc (sizeof (RL2GraphFont));
    if (fnt == NULL)
      {
	  free (facename);
	  FT_Done_Face (face);
	  return NULL;
      }
    tt_font = malloc (sizeof (struct rl2_private_tt_font));
    if (tt_font == NULL)
      {
	  free (facename);
	  FT_Done_Face (face);
	  free (fnt);
	  return NULL;
      }
    fnt->toy_font = 0;
    fnt->tt_font = tt_font;
    fnt->tt_font->facename = facename;
    fnt->tt_font->is_bold = is_bold;
    fnt->tt_font->is_italic = is_italic;
    fnt->tt_font->container = cache;
    fnt->tt_font->FTface = face;
    fnt->tt_font->ttf_data = font;
    fnt->cairo_font = cairo_ft_font_face_create_for_ft_face (face, 0);
    if (fnt->cairo_font == NULL)
      {
	  rl2_priv_graph_destroy_font (fnt);
	  return NULL;
      }
    fnt->cairo_scaled_font = NULL;
/* inserting into the cache */
    tt_font->prev = cache->last_font;
    tt_font->next = NULL;
    if (cache->first_font == NULL)
	cache->first_font = tt_font;
    if (cache->last_font != NULL)
	cache->last_font->next = tt_font;
    cache->last_font = tt_font;
/* registering the destructor callback */
    if (cairo_font_face_set_user_data
	(fnt->cairo_font, &key, tt_font,
	 rl2_font_destructor_callback) != CAIRO_STATUS_SUCCESS)
      {
	  rl2_priv_graph_destroy_font (fnt);
	  return NULL;
      }
    if (size < 1.0)
	fnt->size = 1.0;
    else if (size > 72.0)
	fnt->size = 72.0;
    else
	fnt->size = size;
    if (is_italic)
	fnt->style = RL2_FONTSTYLE_ITALIC;
    else
	fnt->style = RL2_FONTSTYLE_NORMAL;
    if (is_bold)
	fnt->weight = RL2_FONTWEIGHT_BOLD;
    else
	fnt->weight = RL2_FONTWEIGHT_NORMAL;
    fnt->font_red = 0.0;
    fnt->font_green = 0.0;
    fnt->font_blue = 0.0;
    fnt->font_alpha = 1.0;
    fnt->with_halo = 0;
    fnt->halo_radius = 0.0;
    fnt->halo_red = 0.0;
    fnt->halo_green = 0.0;
    fnt->halo_blue = 0.0;
    fnt->halo_alpha = 1.0;
    return (rl2GraphicsFontPtr) fnt;
}

RL2_DECLARE int
rl2_graph_release_font (rl2GraphicsContextPtr context)
{
/* selecting a default font so to releasee the currently set Font */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_select_font_face (cairo, "monospace", CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cairo, 10.0);
    return 1;
}

RL2_DECLARE void
rl2_graph_destroy_font (rl2GraphicsFontPtr font)
{
/* destroying a font */
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;

    if (fnt == NULL)
	return;
    if (fnt->toy_font == 0)
      {
	  if (fnt->cairo_scaled_font != NULL)
	      cairo_scaled_font_destroy (fnt->cairo_scaled_font);
	  if (fnt->cairo_font != NULL)
	      cairo_font_face_destroy (fnt->cairo_font);
      }
    else
      {
	  if (fnt->facename != NULL)
	      free (fnt->facename);
	  free (fnt);
      }
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

    fnt->font_red = (double) red / 255.0;
    fnt->font_green = (double) green / 255.0;
    fnt->font_blue = (double) blue / 255.0;
    fnt->font_alpha = (double) alpha / 255.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_font_set_halo (rl2GraphicsFontPtr font, double radius,
			 unsigned char red, unsigned char green,
			 unsigned char blue, unsigned char alpha)
{
/* setting up the font Halo */
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;

    if (fnt == NULL)
	return 0;

    if (radius <= 0.0)
      {
	  fnt->with_halo = 0;
	  fnt->halo_radius = 0.0;
      }
    else
      {
	  fnt->with_halo = 1;
	  fnt->halo_radius = radius;
	  fnt->halo_red = (double) red / 255.0;
	  fnt->halo_green = (double) green / 255.0;
	  fnt->halo_blue = (double) blue / 255.0;
	  fnt->halo_alpha = (double) alpha / 255.0;
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

RL2_DECLARE int
rl2_graph_release_pattern_brush (rl2GraphicsContextPtr context)
{
/* releasing the current Pattern Brush */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    cairo_t *cairo;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    if (ctx->current_brush.is_pattern)
      {
	  ctx->current_brush.is_solid_color = 1;
	  ctx->current_brush.is_pattern = 0;
	  cairo_set_source_rgba (cairo, 0.0, 0.0, 0.0, 1.0);
	  ctx->current_brush.pattern = NULL;
	  return 1;
      }
    else
	return 0;
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
    if (ctx->current_pen.is_solid_color)
      {
	  /* using a Solid Color Pen */
	  cairo_set_source_rgba (cairo, ctx->current_pen.red,
				 ctx->current_pen.green,
				 ctx->current_pen.blue, ctx->current_pen.alpha);
      }
    else if (ctx->current_pen.is_linear_gradient)
      {
	  /* using a Linear Gradient Pen */
	  cairo_pattern_t *pattern =
	      cairo_pattern_create_linear (ctx->current_pen.x0,
					   ctx->current_pen.y0,
					   ctx->current_pen.x1,
					   ctx->current_pen.y1);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0,
					     ctx->current_pen.red,
					     ctx->current_pen.green,
					     ctx->current_pen.blue,
					     ctx->current_pen.alpha);
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0,
					     ctx->current_pen.red2,
					     ctx->current_pen.green2,
					     ctx->current_pen.blue2,
					     ctx->current_pen.alpha2);
	  cairo_set_source (cairo, pattern);
	  cairo_pattern_destroy (pattern);
      }
    else if (ctx->current_pen.is_pattern)
      {
	  /* using a Pattern Pen */
	  cairo_set_source (cairo, ctx->current_pen.pattern);
      }
    switch (ctx->current_pen.line_cap)
      {
      case RL2_PEN_CAP_ROUND:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_ROUND);
	  break;
      case RL2_PEN_CAP_SQUARE:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_SQUARE);
	  break;
      default:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_BUTT);
	  break;
      };
    switch (ctx->current_pen.line_join)
      {
      case RL2_PEN_JOIN_ROUND:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_ROUND);
	  break;
      case RL2_PEN_JOIN_BEVEL:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_BEVEL);
	  break;
      default:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_MITER);
	  break;
      };
    if (ctx->current_pen.dash_count == 0 || ctx->current_pen.dash_array == NULL)
	cairo_set_dash (cairo, NULL, 0, 0.0);
    else
	cairo_set_dash (cairo, ctx->current_pen.dash_array,
			ctx->current_pen.dash_count,
			ctx->current_pen.dash_offset);
}

RL2_DECLARE int
rl2_graph_release_pattern_pen (rl2GraphicsContextPtr context)
{
/* releasing the current Pattern Pen */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    cairo_t *cairo;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    if (ctx->current_pen.is_pattern)
      {
	  ctx->current_pen.is_solid_color = 1;
	  ctx->current_pen.is_pattern = 0;
	  cairo_set_source_rgba (cairo, 0.0, 0.0, 0.0, 1.0);
	  ctx->current_pen.pattern = NULL;
	  return 1;
      }
    else
	return 0;
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
			      double center_y, double radius,
			      double from_angle, double to_angle)
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
    cairo_set_fill_rule (cairo, CAIRO_FILL_RULE_EVEN_ODD);
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
    if (text == NULL)
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
rl2_graph_draw_text (rl2GraphicsContextPtr context, const char *text,
		     double x, double y, double angle, double anchor_point_x,
		     double anchor_point_y)
{
/* drawing a text string (using the current font) */
    double rads;
    double pre_x;
    double pre_y;
    double width;
    double height;
    double post_x;
    double post_y;
    double center_x;
    double center_y;
    double cx;
    double cy;
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (text == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

/* setting the Anchor Point */
    rl2_graph_get_text_extent (ctx, text, &pre_x, &pre_y, &width, &height,
			       &post_x, &post_y);
    if (anchor_point_x < 0.0 || anchor_point_x > 1.0 || anchor_point_x == 0.5)
	center_x = width / 2.0;
    else
	center_x = width * anchor_point_x;
    if (anchor_point_y < 0.0 || anchor_point_y > 1.0 || anchor_point_y == 0.5)
	center_y = height / 2.0;
    else
	center_y = height * anchor_point_y;
    cx = 0.0 - center_x;
    cy = 0.0 + center_y;

    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    rads = angle * .0174532925199432958;
    cairo_rotate (cairo, rads);
    if (ctx->with_font_halo)
      {
	  /* font with Halo */
	  cairo_move_to (cairo, cx, cy);
	  cairo_text_path (cairo, text);
	  cairo_set_source_rgba (cairo, ctx->font_red, ctx->font_green,
				 ctx->font_blue, ctx->font_alpha);
	  cairo_fill_preserve (cairo);
	  cairo_set_source_rgba (cairo, ctx->halo_red, ctx->halo_green,
				 ctx->halo_blue, ctx->halo_alpha);
	  cairo_set_line_width (cairo, ctx->halo_radius);
	  cairo_stroke (cairo);
      }
    else
      {
	  /* no Halo */
	  cairo_set_source_rgba (cairo, ctx->font_red, ctx->font_green,
				 ctx->font_blue, ctx->font_alpha);
	  cairo_move_to (cairo, cx, cy);
	  cairo_show_text (cairo, text);
      }
    cairo_restore (cairo);
    return 1;
}

static void
do_estimate_text_length (cairo_t * cairo, const char *text, double *length,
			 double *extra)
{
/* estimating the text length */
    const char *p = text;
    int count = 0;
    double radius = 0.0;
    cairo_font_extents_t extents;

    while (*p++ != '\0')
	count++;
    cairo_font_extents (cairo, &extents);
    radius =
	sqrt ((extents.max_x_advance * extents.max_x_advance) +
	      (extents.height * extents.height)) / 2.0;
    *length = radius * count;
    *extra = radius;
}

static void
get_aux_start_point (rl2GeometryPtr geom, double *x, double *y)
{
/* extracting the first point from a Curve */
    double x0;
    double y0;
    rl2LinestringPtr ln = geom->first_linestring;
    rl2GetPoint (ln->coords, 0, &x0, &y0);
    *x = x0;
    *y = y0;
}

static int
get_aux_interception_point (sqlite3 * handle, rl2GeometryPtr geom,
			    rl2GeometryPtr circle, double *x, double *y)
{
/* computing and interception point */
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int ret;
    unsigned char *blob1;
    int size1;
    unsigned char *blob2;
    int size2;
    int ok = 0;

    rl2_serialize_linestring (geom->first_linestring, &blob1, &size1);
    rl2_serialize_linestring (circle->first_linestring, &blob2, &size2);

/* preparing the SQL query statement */
    sql = "SELECT ST_Intersection(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob1, size1, free);
    sqlite3_bind_blob (stmt, 2, blob2, size2, free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob3 =
			  sqlite3_column_blob (stmt, 0);
		      int size3 = sqlite3_column_bytes (stmt, 0);
		      rl2GeometryPtr result =
			  rl2_geometry_from_blob (blob3, size3);
		      if (result == NULL)
			  break;
		      if (result->first_point == NULL)
			  break;
		      *x = result->first_point->x;
		      *y = result->first_point->y;
		      ok = 1;
		  }
	    }
	  else
	      goto error;
      }
    if (ok == 0)
	goto error;
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
aux_is_discarded_portion (rl2LinestringPtr ln, double x, double y)
{
/* attempting to identify the already processed portion */
    double xx;
    double yy;
    rl2GetPoint (ln->coords, 0, &xx, &yy);
    if (xx == x && yy == y)
	return 1;
    rl2GetPoint (ln->coords, ln->points - 1, &xx, &yy);
    if (xx == x && yy == y)
	return 1;
    return 0;
}

static rl2GeometryPtr
aux_reduce_curve (sqlite3 * handle, rl2GeometryPtr geom,
		  rl2GeometryPtr circle, double x, double y)
{
/* reducing a Curve by discarding the alreasdy processed portion */
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int ret;
    unsigned char *blob1;
    int size1;
    unsigned char *blob2;
    int size2;
    rl2GeometryPtr out = NULL;
    rl2LinestringPtr ln;
    rl2LinestringPtr save_ln = NULL;
    int count = 0;

    rl2_serialize_linestring (geom->first_linestring, &blob1, &size1);
    rl2_serialize_linestring (circle->first_linestring, &blob2, &size2);

/* preparing the SQL query statement */
    sql = "SELECT ST_Split(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob1, size1, free);
    sqlite3_bind_blob (stmt, 2, blob2, size2, free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob3 =
			  sqlite3_column_blob (stmt, 0);
		      int size3 = sqlite3_column_bytes (stmt, 0);
		      rl2GeometryPtr result =
			  rl2_geometry_from_blob (blob3, size3);
		      if (result == NULL)
			  break;
		      ln = result->first_linestring;
		      while (ln != NULL)
			{
			    if (aux_is_discarded_portion (ln, x, y))
				;
			    else
			      {
				  save_ln = ln;
				  count++;
			      }
			    ln = ln->next;
			}
		  }
	    }
	  else
	      goto error;
      }
    if (save_ln == NULL || count != 1)
	goto error;
    out = rl2_clone_linestring (save_ln);
    sqlite3_finalize (stmt);
    return out;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

static int
check_reverse (rl2GeometryPtr geom)
{
/* testing for an inverse label */
    rl2LinestringPtr ln;
    double x0;
    double y0;
    double x1;
    double y1;
    double width;
    double height;
    int last;

    if (geom == NULL)
	return 0;
    ln = geom->first_linestring;
    if (ln == NULL)
	return 0;
    if (ln->points < 2)
	return 0;
    last = ln->points - 1;

    rl2GetPoint (ln->coords, 0, &x0, &y0);
    rl2GetPoint (ln->coords, last, &x1, &y1);
    width = fabs (x0 - x1);
    height = fabs (y0 - y1);
    if (width > 3.0)
      {
	  if (x0 > x1)
	      return 1;
      }
    else
      {
	  if (y0 > y1)
	      return 1;
      }
    return 0;
}

static void
reverse_text (const char *in, char *dest, int len)
{
/* reversing a text string */
    char *out;
    int n = 1;
    while (*in != '\0')
      {
	  out = dest + len - n;
	  *out = *in++;
	  n++;
      }
    *(dest + len) = '\0';
}

static rl2GeometryPtr
rl2_draw_wrapped_label (sqlite3 * handle, rl2GraphicsContextPtr context,
			cairo_t * cairo, const char *text, rl2GeometryPtr geom)
{
/* placing each character along the modelling line */
    double x0;
    double y0;
    double x1;
    double y1;
    double radius;
    double m;
    double rads;
    double angle;
    char buf[2];
    rl2GeometryPtr g2;
    rl2GeometryPtr g = rl2_clone_curve (geom);
    rl2GeometryPtr circle;
    char *rev_text = NULL;
    const char *c = text;
    cairo_font_extents_t extents;

    cairo_font_extents (cairo, &extents);
    radius =
	sqrt ((extents.max_x_advance * extents.max_x_advance) +
	      (extents.height * extents.height)) / 2.0;
    if (check_reverse (g))
      {
	  /* reverse text */
	  int len = strlen (text);
	  rev_text = malloc (len + 1);
	  reverse_text (text, rev_text, len);
	  c = rev_text;
      }
    while (*c != '\0' && g != NULL)
      {
	  buf[0] = *c;
	  buf[1] = '\0';
	  get_aux_start_point (g, &x0, &y0);
	  circle = rl2_build_circle (x0, y0, radius);
	  if (!get_aux_interception_point (handle, g, circle, &x1, &y1))
	    {
		rl2_destroy_geometry (circle);
		rl2_destroy_geometry (g);
		g = NULL;
		break;
	    }
	  m = (y1 - y0) / (x1 - x0);
	  rads = atan (m);
	  angle = rads / .0174532925199432958;
	  if (x1 < x0 && rev_text == NULL)
	      angle += 180.0;
	  rl2_graph_draw_text (context, buf, x0, y0, angle, 0.5, 0.5);
	  c++;
	  g2 = aux_reduce_curve (handle, g, circle, x0, y0);
	  rl2_destroy_geometry (circle);
	  rl2_destroy_geometry (g);
	  g = g2;
      }
    if (rev_text)
	free (rev_text);
    return g;
}

RL2_DECLARE int
rl2_graph_draw_warped_text (sqlite3 * handle, rl2GraphicsContextPtr context,
			    const char *text, int points, double *x,
			    double *y, double initial_gap, double gap,
			    int repeated)
{
/* drawing a text string warped along a modelling curve (using the current font) */
    double curve_len;
    double text_len;
    double extra_len;
    double start;
    double from;
    rl2GeometryPtr geom = NULL;
    rl2GeometryPtr geom2 = NULL;
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (text == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    geom = rl2_curve_from_XY (points, x, y);
    if (geom == NULL)
	return 0;

    curve_len = rl2_compute_curve_length (geom);
    do_estimate_text_length (cairo, text, &text_len, &extra_len);
    if ((initial_gap + text_len + (2.0 * extra_len)) > curve_len)
	return 0;		/* not enough room to place the label */

    if (repeated)
      {
	  /* repeated labels */
	  int first = 1;
	  rl2GeometryPtr geom3 = rl2_clone_linestring (geom->first_linestring);
	  while (geom3 != NULL)
	    {
		if (first)
		  {
		      start = initial_gap + extra_len;
		      first = 0;
		  }
		else
		    start = gap + extra_len;
		curve_len = rl2_compute_curve_length (geom3);
		if ((start + text_len + extra_len) > curve_len)
		    break;	/* not enough room to place the label */
		from = start / curve_len;
		/* extracting the sub-path modelling the label */
		geom2 = rl2_curve_substring (handle, geom3, from, 1.0);
		rl2_destroy_geometry (geom3);
		if (geom2 == NULL)
		    goto error;
		geom3 =
		    rl2_draw_wrapped_label (handle, context, cairo, text,
					    geom2);
		rl2_destroy_geometry (geom2);
	    }
      }
    else
      {
	  /* single label */
	  start = (curve_len - text_len) / 2.0;
	  from = start / curve_len;
	  /* extracting the sub-path modelling the label */
	  geom2 = rl2_curve_substring (handle, geom, from, 1.0);
	  if (geom2 == NULL)
	      goto error;
	  rl2_draw_wrapped_label (handle, context, cairo, text, geom2);
	  rl2_destroy_geometry (geom2);
      }

    rl2_destroy_geometry (geom);
    return 1;

  error:
    rl2_destroy_geometry (geom);
    return 0;
}

RL2_DECLARE int
rl2_graph_draw_bitmap (rl2GraphicsContextPtr context,
		       rl2GraphicsBitmapPtr bitmap, double x, double y)
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
				double scale_y, double x, double y)
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

RL2_DECLARE int
rl2_graph_draw_graphic_symbol (rl2GraphicsContextPtr context,
			       rl2GraphicsPatternPtr symbol, double width,
			       double height, double x,
			       double y, double angle,
			       double anchor_point_x, double anchor_point_y)
{
/* drawing a Graphic Symbol */
    double rads;
    double scale_x;
    double scale_y;
    double center_x;
    double center_y;
    cairo_t *cairo;
    cairo_surface_t *surface;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) symbol;

    if (ctx == NULL)
	return 0;
    if (pattern == NULL)
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

/* setting the Anchor Point */
    scale_x = width / (double) (pattern->width);
    scale_y = height / (double) (pattern->height);
    if (anchor_point_x < 0.0 || anchor_point_x > 1.0 || anchor_point_x == 0.5)
	center_x = (double) (pattern->width) / 2.0;
    else
	center_x = (double) (pattern->width) * anchor_point_x;
    if (anchor_point_y < 0.0 || anchor_point_y > 1.0 || anchor_point_y == 0.5)
	center_y = (double) (pattern->height) / 2.0;
    else
	center_y = (double) (pattern->height) * anchor_point_y;

    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    cairo_scale (cairo, scale_x, scale_y);
    rads = angle * .0174532925199432958;
    cairo_rotate (cairo, rads);
    cairo_translate (cairo, 0.0 - center_x, 0.0 - center_y);
    cairo_set_source (cairo, pattern->pattern);
    cairo_paint (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);

    return 1;
}

RL2_DECLARE int
rl2_graph_draw_mark_symbol (rl2GraphicsContextPtr context, int mark_type,
			    double size,
			    double x, double y,
			    double angle,
			    double anchor_point_x,
			    double anchor_point_y, int fill, int stroke)
{
/* drawing a Mark Symbol */
    double xsize;
    double size2 = size / 2.0;
    double size4 = size / 4.0;
    double size6 = size / 6.0;
    double size8 = size / 8.0;
    double size13 = size / 3.0;
    double size23 = (size / 3.0) * 2.0;
    int i;
    double rads;
    double center_x;
    double center_y;
    cairo_t *cairo;
    cairo_surface_t *surface;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
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
    rads = angle * .0174532925199432958;
    cairo_rotate (cairo, rads);

/* setting the Anchor Point */
    xsize = size;
    if (mark_type == RL2_GRAPHIC_MARK_CIRCLE
	|| mark_type == RL2_GRAPHIC_MARK_TRIANGLE
	|| mark_type == RL2_GRAPHIC_MARK_STAR)
	xsize = size23 * 2.0;
    if (anchor_point_x < 0.0 || anchor_point_x > 1.0 || anchor_point_x == 0.5)
	center_x = 0.0;
    else
	center_x = 0.0 + (xsize / 2.0) - (xsize * anchor_point_x);
    if (anchor_point_y < 0.0 || anchor_point_y > 1.0 || anchor_point_y == 0.5)
	center_y = 0.0;
    else
	center_y = 0.0 - (xsize / 2.0) + (xsize * anchor_point_y);
    x = center_x;
    y = center_y;
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

/* preparing the Mark Symbol path */
    switch (mark_type)
      {
      case RL2_GRAPHIC_MARK_CIRCLE:
	  rads = 0.0;
	  for (i = 0; i < 32; i++)
	    {
		double tic = 6.28318530718 / 32.0;
		double cx = x + (size23 * sin (rads));
		double cy = y + (size23 * cos (rads));
		if (i == 0)
		    rl2_graph_move_to_point (ctx, cx, cy);
		else
		    rl2_graph_add_line_to_path (ctx, cx, cy);
		rads += tic;
	    }
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_TRIANGLE:
	  rads = 0.0;
	  for (i = 0; i < 3; i++)
	    {
		double tic = 6.28318530718 / 3.0;
		double cx = x + (size23 * sin (rads));
		double cy = y + (size23 * cos (rads));
		if (i == 0)
		    rl2_graph_move_to_point (ctx, cx, cy);
		else
		    rl2_graph_add_line_to_path (ctx, cx, cy);
		rads += tic;
	    }
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_STAR:
	  rads = 3.14159265359;
	  for (i = 0; i < 10; i++)
	    {
		double tic = (i % 2) ? size4 : size23;
		double cx = x + (tic * sin (rads));
		double cy = y + (tic * cos (rads));
		if (i == 0)
		    rl2_graph_move_to_point (ctx, cx, cy);
		else
		    rl2_graph_add_line_to_path (ctx, cx, cy);
		rads += 0.628318530718;
	    }
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_CROSS:
	  rl2_graph_move_to_point (ctx, x - size8, y - size2);
	  rl2_graph_add_line_to_path (ctx, x + size8, y - size2);
	  rl2_graph_add_line_to_path (ctx, x + size8, y - size8);
	  rl2_graph_add_line_to_path (ctx, x + size2, y - size8);
	  rl2_graph_add_line_to_path (ctx, x + size2, y + size8);
	  rl2_graph_add_line_to_path (ctx, x + size8, y + size8);
	  rl2_graph_add_line_to_path (ctx, x + size8, y + size2);
	  rl2_graph_add_line_to_path (ctx, x - size8, y + size2);
	  rl2_graph_add_line_to_path (ctx, x - size8, y + size8);
	  rl2_graph_add_line_to_path (ctx, x - size2, y + size8);
	  rl2_graph_add_line_to_path (ctx, x - size2, y - size8);
	  rl2_graph_add_line_to_path (ctx, x - size8, y - size8);
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_X:
	  rl2_graph_move_to_point (ctx, x, y - size6);
	  rl2_graph_add_line_to_path (ctx, x - size4, y - size2);
	  rl2_graph_add_line_to_path (ctx, x - size2, y - size2);
	  rl2_graph_add_line_to_path (ctx, x - size8, y);
	  rl2_graph_add_line_to_path (ctx, x - size2, y + size2);
	  rl2_graph_add_line_to_path (ctx, x - size4, y + size2);
	  rl2_graph_add_line_to_path (ctx, x, y + size6);
	  rl2_graph_add_line_to_path (ctx, x + size4, y + size2);
	  rl2_graph_add_line_to_path (ctx, x + size2, y + size2);
	  rl2_graph_add_line_to_path (ctx, x + size8, y);
	  rl2_graph_add_line_to_path (ctx, x + size2, y - size2);
	  rl2_graph_add_line_to_path (ctx, x + size4, y - size2);
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_SQUARE:
      default:
	  rl2_graph_move_to_point (ctx, x - size2, y - size2);
	  rl2_graph_add_line_to_path (ctx, x - size2, y + size2);
	  rl2_graph_add_line_to_path (ctx, x + size2, y + size2);
	  rl2_graph_add_line_to_path (ctx, x + size2, y - size2);
	  rl2_graph_close_subpath (ctx);
	  break;
      };

/* fillingt and stroking the path */
    if (fill && !stroke)
	rl2_graph_fill_path (ctx, RL2_CLEAR_PATH);
    else if (stroke && !fill)
	rl2_graph_stroke_path (ctx, RL2_CLEAR_PATH);
    else
      {
	  rl2_graph_fill_path (ctx, RL2_PRESERVE_PATH);
	  rl2_graph_stroke_path (ctx, RL2_CLEAR_PATH);
      }
    cairo_restore (cairo);
    cairo_surface_flush (surface);

    return 1;
}

static unsigned char
unpremultiply (unsigned char c, unsigned char a)
{
/* Cairo has premultiplied alphas */
    double x = ((double) c * 255.0) / (double) a;
    if (a == 0)
	return 0;
    return (unsigned char) x;
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
		unsigned char a;
		if (little_endian)
		  {
		      b = *p_in++;
		      g = *p_in++;
		      r = *p_in++;
		      a = *p_in++;
		  }
		else
		  {
		      a = *p_in++;
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		*p_out++ = unpremultiply (r, a);
		*p_out++ = unpremultiply (g, a);
		*p_out++ = unpremultiply (b, a);
	    }
      }
    return rgb;
}

RL2_DECLARE unsigned char *
rl2_graph_get_context_alpha_array (rl2GraphicsContextPtr context,
				   int *half_transparent)
{
/* creating an Alpha buffer from the given Context */
    int width;
    int height;
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *alpha;
    int real_alpha = 0;
    int little_endian = rl2cr_endian_arch ();
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    *half_transparent = 0;

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
		      if (*p_in >= 1 && *p_in <= 254)
			  real_alpha = 1;
		      *p_out++ = *p_in++;
		  }
		else
		  {
		      if (*p_in >= 1 && *p_in <= 254)
			  real_alpha = 1;
		      *p_out++ = *p_in++;
		      p_in += 3;	/* skipping RGB */
		  }
	    }
      }
    if (real_alpha)
	*half_transparent = 1;
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

RL2_DECLARE int
rl2_gray_pdf (unsigned int width, unsigned int height, unsigned char **pdf,
	      int *pdf_size)
{
/* attempting to create an all-Gray PDF */
    rl2MemPdfPtr mem = NULL;
    rl2GraphicsContextPtr ctx = NULL;
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
    rl2_graph_set_solid_pen (ctx, 255, 0, 0, 255, 2.0, RL2_PEN_CAP_BUTT,
			     RL2_PEN_JOIN_MITER);
    rl2_graph_set_brush (ctx, 128, 128, 128, 255);
    rl2_graph_draw_rounded_rectangle (ctx, 0, 0, width, height, width / 10.0);

    rl2_graph_destroy_context (ctx);
/* retrieving the PDF memory block */
    if (rl2_get_mem_pdf_buffer (mem, pdf, pdf_size) != RL2_OK)
	goto error;
    rl2_destroy_mem_pdf_target (mem);

    return RL2_OK;

  error:
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

RL2_DECLARE void *
rl2_alloc_private (void)
{
/* allocating and initializing default private connection data */
    FT_Error error;
    FT_Library library;
    struct rl2_private_data *priv_data =
	malloc (sizeof (struct rl2_private_data));
    if (priv_data == NULL)
	return NULL;
    priv_data->max_threads = 1;
/* initializing FreeType */
    error = FT_Init_FreeType (&library);
    if (error)
	priv_data->FTlibrary = NULL;
    else
	priv_data->FTlibrary = library;
    priv_data->first_font = NULL;
    priv_data->last_font = NULL;
    return priv_data;
}

RL2_DECLARE void
rl2_cleanup_private (const void *ptr)
{
/* destroying private connection data */
    struct rl2_private_tt_font *pF;
    struct rl2_private_tt_font *pFn;
    struct rl2_private_data *priv_data = (struct rl2_private_data *) ptr;
    if (priv_data == NULL)
	return;

/* cleaning the internal Font Cache */
    pF = priv_data->first_font;
    while (pF != NULL)
      {
	  pFn = pF->next;
	  rl2_destroy_private_tt_font (pF);
	  pF = pFn;
      }
    if (priv_data->FTlibrary != NULL)
	FT_Done_FreeType ((FT_Library) (priv_data->FTlibrary));
    free (priv_data);
}
