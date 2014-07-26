/*

 rl2svg -- SVG related funcions

 version 0.1, 2014 June 6

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

#ifdef __ANDROID__		/* Android specific */
#include <cairo.h>
#else /* any other standard platform (Win, Linux, Mac) */
#include <cairo/cairo.h>
#endif /* end Android conditionals */

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2svg.h"
#include "rasterlite2_private.h"
#include "rl2svg_private.h"

#define svg_PI	3.141592653589793

struct svg_parent_ref
{
/* a parent reference (inheritance chain) */
    rl2PrivSvgGroupPtr parent;
    struct svg_parent_ref *next;
};

struct svg_parents
{
/* inheritance chain */
    struct svg_parent_ref *first;
    struct svg_parent_ref *last;
};

static void
svg_free_parents (struct svg_parents *chain)
{
/* freeing an inheritance chain */
    struct svg_parent_ref *pp;
    struct svg_parent_ref *ppn;
    pp = chain->first;
    while (pp)
      {
	  ppn = pp->next;
	  free (pp);
	  pp = ppn;
      }
}

static void
svg_add_parent (struct svg_parents *chain, rl2PrivSvgGroupPtr parent)
{
/* adding an inheritance item into the chain */
    struct svg_parent_ref *p = malloc (sizeof (struct svg_parent_ref));
    p->parent = parent;
    p->next = chain->first;
/* updating a reverse-ordered linked list */
    if (chain->last == NULL)
	chain->last = p;
    chain->first = p;
}

static void
svg_gradient_transformation (cairo_pattern_t * pattern,
			     rl2PrivSvgTransformPtr trans)
{
/* applying a single transformation */
    double angle;
    double tangent;
    rl2PrivSvgMatrixPtr mtrx;
    rl2PrivSvgTranslatePtr translate;
    rl2PrivSvgScalePtr scale;
    rl2PrivSvgRotatePtr rotate;
    rl2PrivSvgSkewPtr skew;
    cairo_matrix_t matrix;
    cairo_matrix_t matrix_in;

    if (trans->data == NULL)
	return;
    switch (trans->type)
      {
      case RL2_SVG_MATRIX:
	  mtrx = trans->data;
	  cairo_pattern_get_matrix (pattern, &matrix);
	  matrix_in.xx = mtrx->a;
	  matrix_in.yx = mtrx->b;
	  matrix_in.xy = mtrx->c;
	  matrix_in.yy = mtrx->d;
	  matrix_in.x0 = mtrx->e;
	  matrix_in.y0 = mtrx->f;
	  cairo_matrix_multiply (&matrix, &matrix, &matrix_in);
	  cairo_matrix_invert (&matrix);
	  cairo_pattern_set_matrix (pattern, &matrix);
	  break;
      case RL2_SVG_TRANSLATE:
	  translate = trans->data;
	  cairo_pattern_get_matrix (pattern, &matrix);
	  cairo_matrix_translate (&matrix, translate->tx, translate->ty);
	  cairo_matrix_invert (&matrix);
	  cairo_pattern_set_matrix (pattern, &matrix);
	  break;
      case RL2_SVG_SCALE:
	  scale = trans->data;
	  cairo_pattern_get_matrix (pattern, &matrix);
	  cairo_matrix_scale (&matrix, scale->sx, scale->sy);
	  cairo_matrix_invert (&matrix);
	  cairo_pattern_set_matrix (pattern, &matrix);
	  break;
      case RL2_SVG_ROTATE:
	  rotate = trans->data;
	  cairo_pattern_get_matrix (pattern, &matrix);
	  angle = rotate->angle * (svg_PI / 180.0);
	  cairo_matrix_translate (&matrix, rotate->cx, rotate->cy);
	  cairo_matrix_rotate (&matrix, angle);
	  cairo_matrix_translate (&matrix, -1.0 * rotate->cx,
				  -1.0 * rotate->cy);
	  cairo_matrix_invert (&matrix);
	  cairo_pattern_set_matrix (pattern, &matrix);
	  break;
      case RL2_SVG_SKEW_X:
	  skew = trans->data;
	  cairo_pattern_get_matrix (pattern, &matrix);
	  angle = skew->angle * (svg_PI / 180.0);
	  tangent = tan (angle);
	  matrix_in.xx = 1.0;
	  matrix_in.yx = 0.0;
	  matrix_in.xy = tangent;
	  matrix_in.yy = 1.0;
	  matrix_in.x0 = 0.0;
	  matrix_in.y0 = 0.0;
	  cairo_matrix_multiply (&matrix, &matrix_in, &matrix);
	  cairo_matrix_invert (&matrix);
	  cairo_pattern_set_matrix (pattern, &matrix);
	  break;
      case RL2_SVG_SKEW_Y:
	  skew = trans->data;
	  cairo_pattern_get_matrix (pattern, &matrix);
	  angle = skew->angle * (svg_PI / 180.0);
	  tangent = tan (angle);
	  matrix_in.xx = 1.0;
	  matrix_in.yx = tangent;
	  matrix_in.xy = 0.0;
	  matrix_in.yy = 1.0;
	  matrix_in.x0 = 0.0;
	  matrix_in.y0 = 0.0;
	  cairo_matrix_multiply (&matrix, &matrix_in, &matrix);
	  cairo_matrix_invert (&matrix);
	  cairo_pattern_set_matrix (pattern, &matrix);
	  break;
      };
}

static void
svg_apply_gradient_transformations (cairo_pattern_t * pattern,
				    rl2PrivSvgGradientPtr grad)
{
/* applying all Gradient-related transformations */
    rl2PrivSvgTransformPtr trans = grad->first_trans;
    while (trans)
      {
	  svg_gradient_transformation (pattern, trans);
	  trans = trans->next;
      }
}

static void
svg_set_pen (cairo_t * cairo, rl2PrivSvgStylePtr style)
{
/* setting up a Pen for Cairo */
    cairo_pattern_t *pattern;
    double lengths[4];
    lengths[0] = 1.0;
    cairo_set_line_width (cairo, style->stroke_width);
    if (style->stroke_url != NULL && style->stroke_pointer != NULL)
      {
	  rl2PrivSvgGradientPtr grad = style->stroke_pointer;
	  rl2PrivSvgGradientStopPtr stop;
	  if (grad->type == RL2_SVG_LINEAR_GRADIENT)
	    {
		pattern =
		    cairo_pattern_create_linear (grad->x1, grad->y1, grad->x2,
						 grad->y2);
		svg_apply_gradient_transformations (pattern, grad);
		stop = grad->first_stop;
		while (stop)
		  {
		      cairo_pattern_add_color_stop_rgba (pattern, stop->offset,
							 stop->red, stop->green,
							 stop->blue,
							 stop->opacity *
							 style->opacity);
		      stop = stop->next;
		  }
		cairo_set_source (cairo, pattern);
		cairo_set_line_cap (cairo, style->stroke_linecap);
		cairo_set_line_join (cairo, style->stroke_linejoin);
		cairo_set_miter_limit (cairo, style->stroke_miterlimit);
		if (style->stroke_dashitems == 0
		    || style->stroke_dasharray == NULL)
		    cairo_set_dash (cairo, lengths, 0, 0.0);
		else
		    cairo_set_dash (cairo, style->stroke_dasharray,
				    style->stroke_dashitems,
				    style->stroke_dashoffset);
		cairo_pattern_destroy (pattern);
		return;
	    }
	  else if (grad->type == RL2_SVG_RADIAL_GRADIENT)
	    {
		pattern =
		    cairo_pattern_create_radial (grad->cx, grad->cy, 0.0,
						 grad->fx, grad->fy, grad->r);
		svg_apply_gradient_transformations (pattern, grad);
		stop = grad->first_stop;
		while (stop)
		  {
		      cairo_pattern_add_color_stop_rgba (pattern, stop->offset,
							 stop->red, stop->green,
							 stop->blue,
							 stop->opacity *
							 style->opacity);
		      stop = stop->next;
		  }
		cairo_set_source (cairo, pattern);
		cairo_set_line_cap (cairo, style->stroke_linecap);
		cairo_set_line_join (cairo, style->stroke_linejoin);
		cairo_set_miter_limit (cairo, style->stroke_miterlimit);
		if (style->stroke_dashitems == 0
		    || style->stroke_dasharray == NULL)
		    cairo_set_dash (cairo, lengths, 0, 0.0);
		else
		    cairo_set_dash (cairo, style->stroke_dasharray,
				    style->stroke_dashitems,
				    style->stroke_dashoffset);
		cairo_pattern_destroy (pattern);
		return;
	    }
      }
    cairo_set_source_rgba (cairo, style->stroke_red, style->stroke_green,
			   style->stroke_blue,
			   style->stroke_opacity * style->opacity);
    cairo_set_line_cap (cairo, style->stroke_linecap);
    cairo_set_line_join (cairo, style->stroke_linejoin);
    cairo_set_miter_limit (cairo, style->stroke_miterlimit);
    if (style->stroke_dashitems == 0 || style->stroke_dasharray == NULL)
	cairo_set_dash (cairo, lengths, 0, 0.0);
    else
	cairo_set_dash (cairo, style->stroke_dasharray, style->stroke_dashitems,
			style->stroke_dashoffset);
}

static void
svg_set_brush (cairo_t * cairo, rl2PrivSvgStylePtr style)
{
/* setting up a Brush for Cairo */
    cairo_pattern_t *pattern;
    if (style->fill_url != NULL && style->fill_pointer != NULL)
      {
	  rl2PrivSvgGradientPtr grad = style->fill_pointer;
	  rl2PrivSvgGradientStopPtr stop;
	  if (grad->type == RL2_SVG_LINEAR_GRADIENT)
	    {
		pattern =
		    cairo_pattern_create_linear (grad->x1, grad->y1, grad->x2,
						 grad->y2);
		svg_apply_gradient_transformations (pattern, grad);
		stop = grad->first_stop;
		while (stop)
		  {
		      cairo_pattern_add_color_stop_rgba (pattern, stop->offset,
							 stop->red, stop->green,
							 stop->blue,
							 stop->opacity *
							 style->opacity);
		      stop = stop->next;
		  }
		cairo_set_source (cairo, pattern);
		cairo_pattern_destroy (pattern);
		return;
	    }
	  else if (grad->type == RL2_SVG_RADIAL_GRADIENT)
	    {
		pattern =
		    cairo_pattern_create_radial (grad->cx, grad->cy, 0.0,
						 grad->fx, grad->fy, grad->r);
		svg_apply_gradient_transformations (pattern, grad);
		stop = grad->first_stop;
		while (stop)
		  {
		      cairo_pattern_add_color_stop_rgba (pattern, stop->offset,
							 stop->red, stop->green,
							 stop->blue,
							 stop->opacity *
							 style->opacity);
		      stop = stop->next;
		  }
		cairo_set_source (cairo, pattern);
		cairo_pattern_destroy (pattern);
		return;
	    }
      }

    cairo_set_source_rgba (cairo, style->fill_red, style->fill_green,
			   style->fill_blue,
			   style->fill_opacity * style->opacity);
    cairo_set_fill_rule (cairo, style->fill_rule);
}

static void
svg_draw_rect (cairo_t * cairo, rl2PrivSvgShapePtr shape,
	       rl2PrivSvgStylePtr style)
{
/* drawing an SVG Rect */
    rl2PrivSvgRectPtr rect = shape->data;
    if (rect->width == 0 || rect->height == 0)
	return;
    if (style->visibility == 0)
	return;
    if (style->fill == 0 && style->stroke == 0)
	return;

    if (rect->rx <= 0 || rect->ry <= 0)
      {
	  /* normal Rect */
	  cairo_rectangle (cairo, rect->x, rect->y, rect->width, rect->height);
      }
    else
      {
	  /* rounded Rect */
	  cairo_new_sub_path (cairo);
	  cairo_save (cairo);
	  cairo_translate (cairo, rect->x + rect->rx, rect->y + rect->ry);
	  cairo_scale (cairo, rect->rx, rect->ry);
	  cairo_arc (cairo, 0.0, 0.0, 1.0, svg_PI, -1 * svg_PI / 2.0);
	  cairo_restore (cairo);
	  cairo_save (cairo);
	  cairo_translate (cairo, rect->x + rect->width - rect->rx,
			   rect->y + rect->ry);
	  cairo_scale (cairo, rect->rx, rect->ry);
	  cairo_arc (cairo, 0.0, 0.0, 1.0, -1 * svg_PI / 2.0, 0.0);
	  cairo_restore (cairo);
	  cairo_save (cairo);
	  cairo_translate (cairo, rect->x + rect->width - rect->rx,
			   rect->y + rect->height - rect->ry);
	  cairo_scale (cairo, rect->rx, rect->ry);
	  cairo_arc (cairo, 0.0, 0.0, 1.0, 0.0, svg_PI / 2.0);
	  cairo_restore (cairo);
	  cairo_save (cairo);
	  cairo_translate (cairo, rect->x + rect->rx,
			   rect->y + rect->height - rect->ry);
	  cairo_scale (cairo, rect->rx, rect->ry);
	  cairo_arc (cairo, 0.0, 0.0, 1.0, svg_PI / 2.0, svg_PI);
	  cairo_restore (cairo);
	  cairo_close_path (cairo);
      }

/* drawing the Rect */
    if (style->fill)
      {
	  /* filling */
	  svg_set_brush (cairo, style);
	  if (style->stroke)
	      cairo_fill_preserve (cairo);
	  else
	      cairo_fill (cairo);
      }
    if (style->stroke)
      {
	  /* stroking */
	  svg_set_pen (cairo, style);
	  cairo_stroke (cairo);
      }
}

static void
svg_clip_rect (cairo_t * cairo, rl2PrivSvgShapePtr shape)
{
/* clipping an SVG Rect */
    rl2PrivSvgRectPtr rect = shape->data;
    if (rect->width == 0 || rect->height == 0)
	return;

    if (rect->rx <= 0 || rect->ry <= 0)
      {
	  /* normal Rect */
	  cairo_rectangle (cairo, rect->x, rect->y, rect->width, rect->height);
      }
    else
      {
	  /* rounded Rect */
	  cairo_new_sub_path (cairo);
	  cairo_save (cairo);
	  cairo_translate (cairo, rect->x + rect->rx, rect->y + rect->ry);
	  cairo_scale (cairo, rect->rx, rect->ry);
	  cairo_arc (cairo, 0.0, 0.0, 1.0, svg_PI, -1 * svg_PI / 2.0);
	  cairo_restore (cairo);
	  cairo_save (cairo);
	  cairo_translate (cairo, rect->x + rect->width - rect->rx,
			   rect->y + rect->ry);
	  cairo_scale (cairo, rect->rx, rect->ry);
	  cairo_arc (cairo, 0.0, 0.0, 1.0, -1 * svg_PI / 2.0, 0.0);
	  cairo_restore (cairo);
	  cairo_save (cairo);
	  cairo_translate (cairo, rect->x + rect->width - rect->rx,
			   rect->y + rect->height - rect->ry);
	  cairo_scale (cairo, rect->rx, rect->ry);
	  cairo_arc (cairo, 0.0, 0.0, 1.0, 0.0, svg_PI / 2.0);
	  cairo_restore (cairo);
	  cairo_save (cairo);
	  cairo_translate (cairo, rect->x + rect->rx,
			   rect->y + rect->height - rect->ry);
	  cairo_scale (cairo, rect->rx, rect->ry);
	  cairo_arc (cairo, 0.0, 0.0, 1.0, svg_PI / 2.0, svg_PI);
	  cairo_restore (cairo);
	  cairo_close_path (cairo);
      }
    cairo_clip (cairo);
}

static void
svg_draw_circle (cairo_t * cairo, rl2PrivSvgShapePtr shape,
		 rl2PrivSvgStylePtr style)
{
/* drawing an SVG Circle */
    rl2PrivSvgCirclePtr circle = shape->data;
    if (circle->r <= 0)
	return;
    if (style->visibility == 0)
	return;
    if (style->fill == 0 && style->stroke == 0)
	return;

/* drawing the Arc */
    cairo_arc (cairo, circle->cx, circle->cy, circle->r, 0.0, 2.0 * svg_PI);
    if (style->fill)
      {
	  /* filling */
	  svg_set_brush (cairo, style);
	  if (style->stroke)
	      cairo_fill_preserve (cairo);
	  else
	      cairo_fill (cairo);
      }
    if (style->stroke)
      {
	  /* stroking */
	  svg_set_pen (cairo, style);
	  cairo_stroke (cairo);
      }
}

static void
svg_clip_circle (cairo_t * cairo, rl2PrivSvgShapePtr shape)
{
/* clipping an SVG Circle */
    rl2PrivSvgCirclePtr circle = shape->data;
    if (circle->r <= 0)
	return;

/* clipping the Arc */
    cairo_arc (cairo, circle->cx, circle->cy, circle->r, 0.0, 2.0 * svg_PI);
    cairo_clip (cairo);
}

static void
svg_draw_ellipse (cairo_t * cairo, rl2PrivSvgShapePtr shape,
		  rl2PrivSvgStylePtr style)
{
/* drawing an SVG Ellipse */
    rl2PrivSvgEllipsePtr ellipse = shape->data;
    if (ellipse->rx <= 0 || ellipse->ry <= 0)
	return;
    if (style->visibility == 0)
	return;
    if (style->fill == 0 && style->stroke == 0)
	return;

/* drawing the Ellipse */
    cairo_save (cairo);

    cairo_translate (cairo, ellipse->cx + ellipse->rx / 2.0,
		     ellipse->cy + ellipse->ry / 2.0);
    cairo_scale (cairo, ellipse->rx / 2.0, ellipse->ry / 2.0);
    cairo_arc (cairo, 0.0, 0.0, 2.0, 0.0, 2.0 * svg_PI);
    cairo_restore (cairo);
    if (style->fill)
      {
	  /* filling */
	  svg_set_brush (cairo, style);
	  if (style->stroke)
	      cairo_fill_preserve (cairo);
	  else
	      cairo_fill (cairo);
      }
    if (style->stroke)
      {
	  /* stroking */
	  svg_set_pen (cairo, style);
	  cairo_stroke (cairo);
      }
}

static void
svg_clip_ellipse (cairo_t * cairo, rl2PrivSvgShapePtr shape)
{
/* clipping an SVG Ellipse */
    rl2PrivSvgEllipsePtr ellipse = shape->data;
    if (ellipse->rx <= 0 || ellipse->ry <= 0)
	return;

/* clipping the Ellipse */
    cairo_save (cairo);

    cairo_translate (cairo, ellipse->cx + ellipse->rx / 2.0,
		     ellipse->cy + ellipse->ry / 2.0);
    cairo_scale (cairo, ellipse->rx / 2.0, ellipse->ry / 2.0);
    cairo_arc (cairo, 0.0, 0.0, 2.0, 0.0, 2.0 * svg_PI);
    cairo_restore (cairo);
    cairo_clip (cairo);
}

static void
svg_draw_line (cairo_t * cairo, rl2PrivSvgShapePtr shape,
	       rl2PrivSvgStylePtr style)
{
/* drawing an SVG Line */
    rl2PrivSvgLinePtr line = shape->data;
    if (style->visibility == 0)
	return;
    if (style->fill == 0 && style->stroke == 0)
	return;

/* drawing the Line */
    cairo_move_to (cairo, line->x1, line->y1);
    cairo_line_to (cairo, line->x2, line->y2);
    if (style->fill)
      {
	  /* filling */
	  svg_set_brush (cairo, style);
	  cairo_fill (cairo);
      }
    if (style->stroke)
      {
	  /* stroking */
	  svg_set_pen (cairo, style);
	  cairo_stroke (cairo);
      }
}

static void
svg_clip_line (cairo_t * cairo, rl2PrivSvgShapePtr shape)
{
/* clipping an SVG Line */
    rl2PrivSvgLinePtr line = shape->data;

/* clipping the Line */
    cairo_move_to (cairo, line->x1, line->y1);
    cairo_line_to (cairo, line->x2, line->y2);
    cairo_clip (cairo);
}

static void
svg_draw_polyline (cairo_t * cairo, rl2PrivSvgShapePtr shape,
		   rl2PrivSvgStylePtr style)
{
/* drawing an SVG Polyline */
    int iv;
    rl2PrivSvgPolylinePtr poly = shape->data;
    if (poly->points <= 0 || poly->x == NULL || poly->y == NULL)
	return;
    if (style->visibility == 0)
	return;
    if (style->fill == 0 && style->stroke == 0)
	return;

/* drawing the Polyline */
    for (iv = 0; iv < poly->points; iv++)
      {
	  if (iv == 0)
	      cairo_move_to (cairo, *(poly->x + iv), *(poly->y + iv));
	  else
	      cairo_line_to (cairo, *(poly->x + iv), *(poly->y + iv));
      }
    if (style->fill)
      {
	  /* filling */
	  svg_set_brush (cairo, style);
	  if (style->stroke)
	      cairo_fill_preserve (cairo);
	  else
	      cairo_fill (cairo);
      }
    if (style->stroke)
      {
	  /* stroking */
	  svg_set_pen (cairo, style);
	  cairo_stroke (cairo);
      }
}

static void
svg_clip_polyline (cairo_t * cairo, rl2PrivSvgShapePtr shape)
{
/* clipping an SVG Polyline */
    int iv;
    rl2PrivSvgPolylinePtr poly = shape->data;
    if (poly->points <= 0 || poly->x == NULL || poly->y == NULL)
	return;

/* clipping the Polyline */
    for (iv = 0; iv < poly->points; iv++)
      {
	  if (iv == 0)
	      cairo_move_to (cairo, *(poly->x + iv), *(poly->y + iv));
	  else
	      cairo_line_to (cairo, *(poly->x + iv), *(poly->y + iv));
      }
    cairo_clip (cairo);
}

static void
svg_draw_polygon (cairo_t * cairo, rl2PrivSvgShapePtr shape,
		  rl2PrivSvgStylePtr style)
{
/* drawing an SVG Polygon */
    int iv;
    rl2PrivSvgPolygonPtr poly = shape->data;
    if (poly->points <= 0 || poly->x == NULL || poly->y == NULL)
	return;
    if (style->visibility == 0)
	return;
    if (style->fill == 0 && style->stroke == 0)
	return;

/* drawing the Polygon */
    for (iv = 0; iv < poly->points; iv++)
      {
	  if (iv == 0)
	      cairo_move_to (cairo, *(poly->x + iv), *(poly->y + iv));
	  else
	      cairo_line_to (cairo, *(poly->x + iv), *(poly->y + iv));
      }
    cairo_close_path (cairo);
    if (style->fill)
      {
	  /* filling */
	  svg_set_brush (cairo, style);
	  if (style->stroke)
	      cairo_fill_preserve (cairo);
	  else
	      cairo_fill (cairo);
      }
    if (style->stroke)
      {
	  /* stroking */
	  svg_set_pen (cairo, style);
	  cairo_stroke (cairo);
      }
}

static void
svg_clip_polygon (cairo_t * cairo, rl2PrivSvgShapePtr shape)
{
/* clipping an SVG Polygon */
    int iv;
    rl2PrivSvgPolygonPtr poly = shape->data;
    if (poly->points <= 0 || poly->x == NULL || poly->y == NULL)
	return;

/* clipping the Polygon */
    for (iv = 0; iv < poly->points; iv++)
      {
	  if (iv == 0)
	      cairo_move_to (cairo, *(poly->x + iv), *(poly->y + iv));
	  else
	      cairo_line_to (cairo, *(poly->x + iv), *(poly->y + iv));
      }
    cairo_close_path (cairo);
    cairo_clip (cairo);
}

static void
svg_rotate (double x, double y, double angle, double *rx, double *ry)
{
/* rotate a point of an angle around the origin point */
    *rx = x * cos (angle) - y * sin (angle);
    *ry = y * cos (angle) + x * sin (angle);
}

static double
svg_point_angle (double cx, double cy, double px, double py)
{
/* return angle between x axis and point knowing given center */
    return atan2 (py - cy, px - cx);
}

static void
svg_arc_to_cairo (rl2PrivSvgPathEllipticArcPtr arc, double x1, double y1,
		  double *xc, double *yc, double *rx, double *rotation,
		  double *radii_ratio, double *angle1, double *angle2)
{
/*
/ computing the arguments for CairoArc starting from an SVG Elliptic Arc 
/ (simply a C translation of pyCairo Python code) 
*/
    double x3;
    double y3;
    double ry;
    double xe;
    double ye;
    double angle;
/* converting absolute x3 and y3 to relative */
    x3 = arc->x - x1;
    y3 = arc->y - y1;
    *rx = arc->rx;
    ry = arc->ry;
    *radii_ratio = ry / *rx;
    *rotation = arc->rotation * (svg_PI / 180.0);
/* cancel the rotation of the second point */
    svg_rotate (x3, y3, *rotation * -1, &xe, &ye);
    ye /= *radii_ratio;
/* find the angle between the second point and the x axis */
    angle = svg_point_angle (0.0, 0.0, xe, ye);
/* put the second point onto the x axis */
    xe = sqrt ((xe * xe) + (ye * ye));
    ye = 0.0;
/* update the x radius if it is too small */
    if (*rx < xe / 2.0)
	*rx = xe / 2.0;
/* find one circle centre */
    *xc = xe / 2.0;
    *yc = sqrt ((*rx * *rx) - (*xc * *xc));
/* choose between the two circles according to flags */
    if ((arc->large_arc ^ arc->sweep) == 0)
	*yc = *yc * -1.0;
/* put the second point and the center back to their positions */
    svg_rotate (xe, ye, angle, &xe, &ye);
    svg_rotate (*xc, *yc, angle, xc, yc);
/* find the drawing angles */
    *angle1 = svg_point_angle (*xc, *yc, 0.0, 0.0);
    *angle2 = svg_point_angle (*xc, *yc, xe, ye);
}

static void
svg_draw_path (cairo_t * cairo, rl2PrivSvgShapePtr shape,
	       rl2PrivSvgStylePtr style)
{
/* drawing an SVG Path */
    rl2PrivSvgPathItemPtr item;
    rl2PrivSvgPathPtr path = shape->data;
    rl2PrivSvgPathMovePtr move;
    rl2PrivSvgPathBezierPtr bezier;
    rl2PrivSvgPathEllipticArcPtr arc;
    double x;
    double y;
    double x0;
    double y0;
    double radii_ratio;
    double xc;
    double yc;
    double rx;
    double rotation;
    double angle1;
    double angle2;
    int is_new_path = 0;
    if (path->error)
	return;
    if (style->visibility == 0)
	return;
    if (style->fill == 0 && style->stroke == 0)
	return;

/* drawing the Path */
    item = path->first;
    while (item)
      {
	  if (is_new_path && item->type != RL2_SVG_MOVE_TO)
	    {
		/* implicit MoveTO */
		cairo_get_current_point (cairo, &x, &y);
		cairo_move_to (cairo, x, y);
	    }
	  is_new_path = 0;
	  switch (item->type)
	    {
	    case RL2_SVG_CLOSE_PATH:
		cairo_close_path (cairo);
		is_new_path = 1;
		break;
	    case RL2_SVG_MOVE_TO:
		move = item->data;
		cairo_move_to (cairo, move->x, move->y);
		break;
	    case RL2_SVG_LINE_TO:
		move = item->data;
		cairo_line_to (cairo, move->x, move->y);
		break;
	    case RL2_SVG_CURVE_3:
		bezier = item->data;
		cairo_curve_to (cairo, bezier->x1, bezier->y1, bezier->x2,
				bezier->y2, bezier->x, bezier->y);
		break;
	    case RL2_SVG_CURVE_4:
		bezier = item->data;
		cairo_get_current_point (cairo, &x0, &y0);
		cairo_curve_to (cairo, 2.0 / 3.0 * bezier->x1 + 1.0 / 3.0 * x0,
				2.0 / 3.0 * bezier->y1 + 1.0 / 3.0 * y0,
				2.0 / 3.0 * bezier->x1 + 1.0 / 3.0 * bezier->x2,
				2.0 / 3.0 * bezier->y1 + 1.0 / 3.0 * bezier->y2,
				bezier->y1, bezier->y2);
		break;
	    case RL2_SVG_ELLIPT_ARC:
		arc = item->data;
		cairo_get_current_point (cairo, &x0, &y0);
		svg_arc_to_cairo (arc, x0, y0, &xc, &yc, &rx, &rotation,
				  &radii_ratio, &angle1, &angle2);
		cairo_save (cairo);
		cairo_translate (cairo, x0, y0);
		cairo_rotate (cairo, rotation);
		cairo_scale (cairo, 1.0, radii_ratio);
		if (arc->sweep == 0)
		    cairo_arc_negative (cairo, xc, yc, rx, angle1, angle2);
		else
		    cairo_arc (cairo, xc, yc, rx, angle1, angle2);
		cairo_restore (cairo);
		break;
	    };
	  item = item->next;
      }
    if (style->fill)
      {
	  /* filling */
	  svg_set_brush (cairo, style);
	  if (style->stroke)
	      cairo_fill_preserve (cairo);
	  else
	      cairo_fill (cairo);
      }
    if (style->stroke)
      {
	  /* stroking */
	  svg_set_pen (cairo, style);
	  cairo_stroke (cairo);
      }
}

static void
svg_clip_path (cairo_t * cairo, rl2PrivSvgShapePtr shape)
{
/* clipping an SVG Path */
    rl2PrivSvgPathItemPtr item;
    rl2PrivSvgPathPtr path = shape->data;
    rl2PrivSvgPathMovePtr move;
    rl2PrivSvgPathBezierPtr bezier;
    rl2PrivSvgPathEllipticArcPtr arc;
    double x;
    double y;
    double x0;
    double y0;
    double radii_ratio;
    double xc;
    double yc;
    double rx;
    double rotation;
    double angle1;
    double angle2;
    int is_new_path = 0;
    if (path->error)
	return;
/* clipping the Path */
    item = path->first;
    while (item)
      {
	  if (is_new_path && item->type != RL2_SVG_MOVE_TO)
	    {
		/* implicit MoveTO */
		cairo_get_current_point (cairo, &x, &y);
		cairo_move_to (cairo, x, y);
	    }
	  is_new_path = 0;
	  switch (item->type)
	    {
	    case RL2_SVG_CLOSE_PATH:
		cairo_close_path (cairo);
		is_new_path = 1;
		break;
	    case RL2_SVG_MOVE_TO:
		move = item->data;
		cairo_move_to (cairo, move->x, move->y);
		break;
	    case RL2_SVG_LINE_TO:
		move = item->data;
		cairo_line_to (cairo, move->x, move->y);
		break;
	    case RL2_SVG_CURVE_3:
		bezier = item->data;
		cairo_curve_to (cairo, bezier->x1, bezier->y1, bezier->x2,
				bezier->y2, bezier->x, bezier->y);
		break;
	    case RL2_SVG_CURVE_4:
		bezier = item->data;
		cairo_get_current_point (cairo, &x0, &y0);
		cairo_curve_to (cairo, 2.0 / 3.0 * bezier->x1 + 1.0 / 3.0 * x0,
				2.0 / 3.0 * bezier->y1 + 1.0 / 3.0 * y0,
				2.0 / 3.0 * bezier->x1 + 1.0 / 3.0 * bezier->x2,
				2.0 / 3.0 * bezier->y1 + 1.0 / 3.0 * bezier->y2,
				bezier->y1, bezier->y2);
		break;
	    case RL2_SVG_ELLIPT_ARC:
		arc = item->data;
		cairo_get_current_point (cairo, &x0, &y0);
		svg_arc_to_cairo (arc, x0, y0, &xc, &yc, &rx, &rotation,
				  &radii_ratio, &angle1, &angle2);
		cairo_save (cairo);
		cairo_translate (cairo, x0, y0);
		cairo_rotate (cairo, rotation);
		cairo_scale (cairo, 1.0, radii_ratio);
		if (arc->sweep == 0)
		    cairo_arc_negative (cairo, xc, yc, rx, angle1, angle2);
		else
		    cairo_arc (cairo, xc, yc, rx, angle1, angle2);
		cairo_restore (cairo);
		break;
	    };
	  item = item->next;
      }
    cairo_clip (cairo);
}

static void
svg_draw_shape (cairo_t * cairo, rl2PrivSvgShapePtr shape,
		rl2PrivSvgStylePtr style)
{
/* drawing an SVG Shape */
    if (shape->style.visibility == 0)
	return;
    switch (shape->type)
      {
      case RL2_SVG_RECT:
	  svg_draw_rect (cairo, shape, style);
	  break;
      case RL2_SVG_CIRCLE:
	  svg_draw_circle (cairo, shape, style);
	  break;
      case RL2_SVG_ELLIPSE:
	  svg_draw_ellipse (cairo, shape, style);
	  break;
      case RL2_SVG_LINE:
	  svg_draw_line (cairo, shape, style);
	  break;
      case RL2_SVG_POLYLINE:
	  svg_draw_polyline (cairo, shape, style);
	  break;
      case RL2_SVG_POLYGON:
	  svg_draw_polygon (cairo, shape, style);
	  break;
      case RL2_SVG_PATH:
	  svg_draw_path (cairo, shape, style);
	  break;
      };
}

static void
svg_transformation (cairo_t * cairo, rl2PrivSvgTransformPtr trans)
{
/* applying a single transformation */
    double angle;
    double tangent;
    rl2PrivSvgMatrixPtr mtrx;
    rl2PrivSvgTranslatePtr translate;
    rl2PrivSvgScalePtr scale;
    rl2PrivSvgRotatePtr rotate;
    rl2PrivSvgSkewPtr skew;
    cairo_matrix_t matrix;
    cairo_matrix_t matrix_in;

    if (trans->data == NULL)
	return;
    switch (trans->type)
      {
      case RL2_SVG_MATRIX:
	  mtrx = trans->data;
	  cairo_get_matrix (cairo, &matrix);
	  matrix_in.xx = mtrx->a;
	  matrix_in.yx = mtrx->b;
	  matrix_in.xy = mtrx->c;
	  matrix_in.yy = mtrx->d;
	  matrix_in.x0 = mtrx->e;
	  matrix_in.y0 = mtrx->f;
	  cairo_matrix_multiply (&matrix, &matrix_in, &matrix);
	  cairo_set_matrix (cairo, &matrix);
	  break;
      case RL2_SVG_TRANSLATE:
	  translate = trans->data;
	  cairo_get_matrix (cairo, &matrix);
	  cairo_matrix_translate (&matrix, translate->tx, translate->ty);
	  cairo_set_matrix (cairo, &matrix);
	  break;
      case RL2_SVG_SCALE:
	  scale = trans->data;
	  cairo_get_matrix (cairo, &matrix);
	  cairo_matrix_scale (&matrix, scale->sx, scale->sy);
	  cairo_set_matrix (cairo, &matrix);
	  break;
      case RL2_SVG_ROTATE:
	  rotate = trans->data;
	  cairo_get_matrix (cairo, &matrix);
	  angle = rotate->angle * (svg_PI / 180.0);
	  cairo_matrix_translate (&matrix, rotate->cx, rotate->cy);
	  cairo_matrix_rotate (&matrix, angle);
	  cairo_matrix_translate (&matrix, -1.0 * rotate->cx,
				  -1.0 * rotate->cy);
	  cairo_set_matrix (cairo, &matrix);
	  break;
      case RL2_SVG_SKEW_X:
	  skew = trans->data;
	  cairo_get_matrix (cairo, &matrix);
	  angle = skew->angle * (svg_PI / 180.0);
	  tangent = tan (angle);
	  matrix_in.xx = 1.0;
	  matrix_in.yx = 0.0;
	  matrix_in.xy = tangent;
	  matrix_in.yy = 1.0;
	  matrix_in.x0 = 0.0;
	  matrix_in.y0 = 0.0;
	  cairo_matrix_multiply (&matrix, &matrix_in, &matrix);
	  cairo_set_matrix (cairo, &matrix);
	  break;
      case RL2_SVG_SKEW_Y:
	  skew = trans->data;
	  cairo_get_matrix (cairo, &matrix);
	  angle = skew->angle * (svg_PI / 180.0);
	  tangent = tan (angle);
	  matrix_in.xx = 1.0;
	  matrix_in.yx = tangent;
	  matrix_in.xy = 0.0;
	  matrix_in.yy = 1.0;
	  matrix_in.x0 = 0.0;
	  matrix_in.y0 = 0.0;
	  cairo_matrix_multiply (&matrix, &matrix_in, &matrix);
	  cairo_set_matrix (cairo, &matrix);
	  break;
      };
}

static void
svg_apply_transformations (cairo_t * cairo, rl2PrivSvgDocumentPtr svg_doc,
			   rl2PrivSvgShapePtr shape)
{
/* applying the whole transformations chain (supporting inheritance) */
    rl2PrivSvgGroupPtr parent;
    struct svg_parents chain;
    struct svg_parent_ref *ref;
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgTransformPtr trans;
    cairo_matrix_t matrix;

/* initializing the chain as empty */
    chain.first = NULL;
    chain.last = NULL;

    parent = shape->parent;
    if (parent != NULL)
      {
	  /* identifying all direct parents in reverse order */
	  while (parent)
	    {
		svg_add_parent (&chain, parent);
		parent = parent->parent;
	    }
      }

/* starting from the SVG Document basic transformations */
    matrix.xx = svg_doc->matrix.xx;
    matrix.yx = svg_doc->matrix.yx;
    matrix.xy = svg_doc->matrix.xy;
    matrix.yy = svg_doc->matrix.yy;
    matrix.x0 = svg_doc->matrix.x0;
    matrix.y0 = svg_doc->matrix.y0;
    cairo_set_matrix (cairo, &matrix);

    ref = chain.first;
    while (ref)
      {
	  /* chaining all transformations inherited by ancestors */
	  group = ref->parent;
	  if (group != NULL)
	    {
		trans = group->first_trans;
		while (trans)
		  {
		      svg_transformation (cairo, trans);
		      trans = trans->next;
		  }
	    }
	  ref = ref->next;
      }

/* applying Shape specific transformations */
    trans = shape->first_trans;
    while (trans)
      {
	  svg_transformation (cairo, trans);
	  trans = trans->next;
      }

    svg_free_parents (&chain);
}

static void
svg_apply_style (rl2PrivSvgShapePtr shape, rl2PrivSvgStylePtr style)
{
/* applying the final style (supporting inheritance) */
    rl2PrivSvgGroupPtr parent;
    struct svg_parents chain;
    struct svg_parent_ref *ref;
    rl2PrivSvgGroupPtr group;

/* initializing the chain as empty */
    chain.first = NULL;
    chain.last = NULL;

    parent = shape->parent;
    if (parent != NULL)
      {
	  /* identifying all parents in reverse order */
	  while (parent)
	    {
		svg_add_parent (&chain, parent);
		parent = parent->parent;
	    }
      }

    ref = chain.first;
    while (ref)
      {
	  /* chaining all style definitions inherited by ancestors */
	  group = ref->parent;
	  if (group != NULL)
	    {
		if (group->style.visibility >= 0)
		    style->visibility = group->style.visibility;
		style->opacity = group->style.opacity;
		if (group->style.fill >= 0)
		    style->fill = group->style.fill;
		if (group->style.no_fill)
		    style->no_fill = group->style.no_fill;
		if (group->style.fill_rule >= 0)
		    style->fill_rule = group->style.fill_rule;
		if (group->style.fill_url != NULL)
		    svg_add_fill_gradient_url (style, group->style.fill_url);
		if (group->style.fill_red >= 0.0)
		    style->fill_red = group->style.fill_red;
		if (group->style.fill_green >= 0.0)
		    style->fill_green = group->style.fill_green;
		if (group->style.fill_blue >= 0.0)
		    style->fill_blue = group->style.fill_blue;
		if (group->style.fill_opacity >= 0.0)
		    style->fill_opacity = group->style.fill_opacity;
		if (group->style.stroke >= 0)
		    style->stroke = group->style.stroke;
		if (group->style.no_stroke >= 0)
		    style->no_stroke = group->style.no_stroke;
		if (group->style.stroke_width >= 0.0)
		    style->stroke_width = group->style.stroke_width;
		if (group->style.stroke_linecap >= 0)
		    style->stroke_linecap = group->style.stroke_linecap;
		if (group->style.stroke_linejoin >= 0)
		    style->stroke_linejoin = group->style.stroke_linejoin;
		if (group->style.stroke_miterlimit >= 0.0)
		    style->stroke_miterlimit = group->style.stroke_miterlimit;
		if (group->style.stroke_dashitems > 0)
		  {
		      style->stroke_dashitems = group->style.stroke_dashitems;
		      if (style->stroke_dasharray != NULL)
			  free (style->stroke_dasharray);
		      style->stroke_dasharray = NULL;
		      if (group->style.stroke_dashitems > 0)
			{
			    int i;
			    style->stroke_dasharray =
				malloc (sizeof (double) *
					group->style.stroke_dashitems);
			    for (i = 0; i < group->style.stroke_dashitems; i++)
				style->stroke_dasharray[i] =
				    group->style.stroke_dasharray[i];
			}
		      style->stroke_dashoffset = group->style.stroke_dashoffset;
		  }
		if (group->style.stroke_url != NULL)
		    svg_add_stroke_gradient_url (style,
						 group->style.stroke_url);
		if (group->style.stroke_red >= 0.0)
		    style->stroke_red = group->style.stroke_red;
		if (group->style.stroke_green >= 0.0)
		    style->stroke_green = group->style.stroke_green;
		if (group->style.stroke_blue >= 0.0)
		    style->stroke_blue = group->style.stroke_blue;
		if (group->style.stroke_opacity >= 0.0)
		    style->stroke_opacity = group->style.stroke_opacity;
		if (group->style.clip_url != NULL)
		  {
		      svg_add_clip_url (style, group->style.clip_url);
		      style->clip_pointer = group->style.clip_pointer;
		  }
	    }
	  ref = ref->next;
      }

/* applying Shape specific style definitions */
    if (shape->style.visibility >= 0)
	style->visibility = shape->style.visibility;
    style->opacity = shape->style.opacity;
    if (shape->style.fill >= 0)
	style->fill = shape->style.fill;
    if (shape->style.no_fill >= 0)
	style->no_fill = shape->style.no_fill;
    if (shape->style.fill_rule >= 0)
	style->fill_rule = shape->style.fill_rule;
    if (shape->style.fill_url != NULL)
	svg_add_fill_gradient_url (style, shape->style.fill_url);
    if (shape->style.fill_red >= 0.0)
	style->fill_red = shape->style.fill_red;
    if (shape->style.fill_green >= 0.0)
	style->fill_green = shape->style.fill_green;
    if (shape->style.fill_blue >= 0.0)
	style->fill_blue = shape->style.fill_blue;
    if (shape->style.fill_opacity >= 0.0)
	style->fill_opacity = shape->style.fill_opacity;
    if (shape->style.stroke >= 0)
	style->stroke = shape->style.stroke;
    if (shape->style.no_stroke >= 0)
	style->no_stroke = shape->style.no_stroke;
    if (shape->style.stroke_width >= 0.0)
	style->stroke_width = shape->style.stroke_width;
    if (shape->style.stroke_linecap >= 0)
	style->stroke_linecap = shape->style.stroke_linecap;
    if (shape->style.stroke_linejoin >= 0)
	style->stroke_linejoin = shape->style.stroke_linejoin;
    if (shape->style.stroke_miterlimit >= 0.0)
	style->stroke_miterlimit = shape->style.stroke_miterlimit;
    if (shape->style.stroke_dashitems > 0)
      {
	  style->stroke_dashitems = shape->style.stroke_dashitems;
	  if (style->stroke_dasharray != NULL)
	      free (style->stroke_dasharray);
	  style->stroke_dasharray = NULL;
	  if (shape->style.stroke_dashitems > 0)
	    {
		int i;
		style->stroke_dasharray =
		    malloc (sizeof (double) * shape->style.stroke_dashitems);
		for (i = 0; i < shape->style.stroke_dashitems; i++)
		    style->stroke_dasharray[i] =
			shape->style.stroke_dasharray[i];
	    }
	  style->stroke_dashoffset = shape->style.stroke_dashoffset;
      }
    if (shape->style.stroke_url != NULL)
	svg_add_stroke_gradient_url (style, shape->style.stroke_url);
    if (shape->style.stroke_red >= 0.0)
	style->stroke_red = shape->style.stroke_red;
    if (shape->style.stroke_green >= 0.0)
	style->stroke_green = shape->style.stroke_green;
    if (shape->style.stroke_blue >= 0.0)
	style->stroke_blue = shape->style.stroke_blue;
    if (shape->style.stroke_opacity >= 0.0)
	style->stroke_opacity = shape->style.stroke_opacity;
    if (shape->style.clip_url != NULL)
      {
	  svg_add_clip_url (style, shape->style.clip_url);
	  style->clip_pointer = shape->style.clip_pointer;
      }
/* final adjustements */
    if (style->fill < 0)
	style->fill = 1;
    if (style->stroke < 0)
	style->stroke = 1;
    if (style->no_fill < 0)
	style->no_fill = 0;
    if (style->no_stroke < 0)
	style->no_stroke = 0;
    if (style->fill_red < 0.0 && style->fill_green < 0.0
	&& style->fill_blue < 0.0 && style->fill_url == NULL)
	style->no_fill = 1;
    if (style->stroke_red < 0.0 && style->stroke_green < 0.0
	&& style->stroke_blue < 0.0 && style->stroke_url == NULL)
	style->no_stroke = 1;
    if (style->no_fill)
	style->fill = 0;
    if (style->no_stroke)
	style->stroke = 0;
    if (style->fill)
      {
	  if (style->fill_rule < 0)
	      style->fill_rule = CAIRO_FILL_RULE_WINDING;
	  if (style->fill_red < 0.0 || style->fill_red > 1.0)
	      style->fill_red = 0.0;
	  if (style->fill_green < 0.0 || style->fill_green > 1.0)
	      style->fill_green = 0.0;
	  if (style->fill_blue < 0.0 || style->fill_blue > 1.0)
	      style->fill_blue = 0.0;
	  if (style->fill_opacity < 0.0 || style->fill_opacity > 1.0)
	      style->fill_opacity = 1.0;
      }
    if (style->stroke)
      {
	  if (style->stroke_width <= 0.0)
	      style->stroke_width = 1.0;
	  if (style->stroke_linecap < 0)
	      style->stroke_linecap = CAIRO_LINE_CAP_BUTT;
	  if (style->stroke_linejoin < 0)
	      style->stroke_linejoin = CAIRO_LINE_JOIN_MITER;
	  if (style->stroke_miterlimit < 0)
	      style->stroke_miterlimit = 4.0;
	  if (style->stroke_red < 0.0 || style->stroke_red > 1.0)
	      style->stroke_red = 0.0;
	  if (style->stroke_green < 0.0 || style->stroke_green > 1.0)
	      style->stroke_green = 0.0;
	  if (style->stroke_blue < 0.0 || style->stroke_blue > 1.0)
	      style->stroke_blue = 0.0;
	  if (style->stroke_opacity < 0.0 || style->stroke_opacity > 1.0)
	      style->stroke_opacity = 1.0;
      }
    svg_free_parents (&chain);
}

static void
svg_resolve_fill_url (rl2PrivSvgDocumentPtr svg_doc, rl2PrivSvgStylePtr style)
{
/* attempting to resolve a FillGradient by URL */
    rl2PrivSvgGradientPtr gradient = svg_doc->first_grad;
    while (gradient)
      {
	  if (gradient->id != NULL)
	    {
		if (strcmp (style->fill_url, gradient->id) == 0)
		  {
		      style->fill_pointer = gradient;
		      return;
		  }
	    }
	  gradient = gradient->next;
      }
    style->fill_pointer = NULL;
    style->fill = 0;
}

static void
svg_resolve_stroke_url (rl2PrivSvgDocumentPtr svg_doc, rl2PrivSvgStylePtr style)
{
/* attempting to resolve a StrokeGradient by URL */
    rl2PrivSvgGradientPtr gradient = svg_doc->first_grad;
    while (gradient)
      {
	  if (gradient->id != NULL)
	    {
		if (strcmp (style->stroke_url, gradient->id) == 0)
		  {
		      style->stroke_pointer = gradient;
		      return;
		  }
	    }
	  gradient = gradient->next;
      }
    style->stroke_pointer = NULL;
    style->stroke = 0;
}

static void
svg_apply_clip2 (cairo_t * cairo, rl2PrivSvgItemPtr clip_path)
{
/* attempting to apply a ClipPath - actuation */
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgShapePtr shape;
    rl2PrivSvgClipPtr clip;
    rl2PrivSvgItemPtr item = clip_path;
    while (item)
      {
	  /* looping on Items */
	  if (item->type == RL2_SVG_ITEM_SHAPE && item->pointer != NULL)
	    {
		shape = item->pointer;
		switch (shape->type)
		  {
		  case RL2_SVG_RECT:
		      svg_clip_rect (cairo, shape);
		      break;
		  case RL2_SVG_CIRCLE:
		      svg_clip_circle (cairo, shape);
		      break;
		  case RL2_SVG_ELLIPSE:
		      svg_clip_ellipse (cairo, shape);
		      break;
		  case RL2_SVG_LINE:
		      svg_clip_line (cairo, shape);
		      break;
		  case RL2_SVG_POLYLINE:
		      svg_clip_polyline (cairo, shape);
		      break;
		  case RL2_SVG_POLYGON:
		      svg_clip_polygon (cairo, shape);
		      break;
		  case RL2_SVG_PATH:
		      svg_clip_path (cairo, shape);
		      break;
		  };
	    }
	  if (item->type == RL2_SVG_ITEM_GROUP && item->pointer != NULL)
	    {
		group = item->pointer;
		svg_apply_clip2 (cairo, group->first);
	    }
	  if (item->type == RL2_SVG_ITEM_CLIP && item->pointer != NULL)
	    {
		clip = item->pointer;
		svg_apply_clip2 (cairo, clip->first);
	    }
	  item = item->next;
      }
}

static void
svg_apply_clip (cairo_t * cairo, rl2PrivSvgItemPtr item)
{
/* attempting to apply a ClipPath */
    if (item->type == RL2_SVG_ITEM_CLIP && item->pointer != NULL)
      {
	  rl2PrivSvgClipPtr clip = item->pointer;
	  svg_apply_clip2 (cairo, clip->first);
      }
}

static void
svg_render_item (cairo_t * cairo, rl2PrivSvgDocumentPtr svg_doc,
		 rl2PrivSvgItemPtr item)
{
/* rendering all SVG Items */
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgShapePtr shape;
    rl2PrivSvgStyle style;

    svg_init_style (&style);
    while (item)
      {
	  /* looping on Items */
	  if (item->type == RL2_SVG_ITEM_SHAPE && item->pointer != NULL)
	    {
		shape = item->pointer;
		if (shape->is_defs || shape->is_flow_root)
		    ;
		else
		  {
		      svg_apply_transformations (cairo, svg_doc, shape);
		      svg_apply_style (shape, &style);
		      if (style.visibility)
			{
			    if (style.fill_url != NULL)
				svg_resolve_fill_url (svg_doc, &style);
			    if (style.stroke_url != NULL)
				svg_resolve_stroke_url (svg_doc, &style);
			    if (style.clip_url != NULL
				&& style.clip_pointer != NULL)
				svg_apply_clip (cairo, style.clip_pointer);
			    svg_draw_shape (cairo, shape, &style);
			    cairo_reset_clip (cairo);
			}
		  }
	    }
	  if (item->type == RL2_SVG_ITEM_GROUP && item->pointer != NULL)
	    {
		group = item->pointer;
		if (group->is_defs || group->is_flow_root)
		    ;
		else
		    svg_render_item (cairo, svg_doc, group->first);
	    }
	  item = item->next;
      }
    svg_style_cleanup (&style);
}

static void
svg_find_href (rl2PrivSvgDocumentPtr svg_doc, rl2PrivSvgItemPtr item,
	       const char *href, rl2PrivSvgItemPtr * pointer)
{
/* attempting to recursively resolve an xlink:href  reference */
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgShapePtr shape;

    while (item)
      {
	  /* looping on Items */
	  if (item->type == RL2_SVG_ITEM_SHAPE && item->pointer != NULL)
	    {
		shape = item->pointer;
		if (shape->id != NULL)
		  {
		      if (strcmp (shape->id, href + 1) == 0)
			{
			    *pointer = item;
			    return;
			}
		  }
	    }
	  if (item->type == RL2_SVG_ITEM_GROUP && item->pointer != NULL)
	    {
		group = item->pointer;
		if (group->id != NULL)
		  {
		      if (strcmp (group->id, href + 1) == 0)
			{
			    *pointer = item;
			    return;
			}
		  }
		svg_find_href (svg_doc, group->first, href, pointer);
	    }
	  item = item->next;
      }
    *pointer = NULL;
}

static void
svg_replace_use (rl2PrivSvgItemPtr item, rl2PrivSvgItemPtr repl)
{
/* replacing an Use with the corrensponding resolved item */
    rl2PrivSvgUsePtr use = item->pointer;

    if (repl->type == RL2_SVG_ITEM_SHAPE && repl->pointer)
      {
	  rl2PrivSvgShapePtr shape = repl->pointer;
	  item->pointer = svg_clone_shape (shape, use);
	  item->type = RL2_SVG_ITEM_SHAPE;
      }
    if (repl->type == RL2_SVG_ITEM_GROUP && repl->pointer)
      {
	  rl2PrivSvgGroupPtr group = repl->pointer;
	  item->pointer = svg_clone_group (group, use);
	  item->type = RL2_SVG_ITEM_GROUP;
      }
    svg_free_use (use);
}

static void
svg_find_gradient_href (rl2PrivSvgDocumentPtr svg_doc, const char *href,
			rl2PrivSvgGradientPtr * pointer)
{
/* attempting to recursively resolve an xlink:href  reference (Gradients) */
    rl2PrivSvgGradientPtr grad = svg_doc->first_grad;

    while (grad)
      {
	  /* looping on Gradients */
	  if (strcmp (grad->id, href + 1) == 0)
	    {
		*pointer = grad;
		return;
	    }
	  grad = grad->next;
      }
}

static rl2PrivSvgGradientPtr
svg_replace_gradient (rl2PrivSvgDocumentPtr svg_doc,
		      rl2PrivSvgGradientPtr gradient,
		      rl2PrivSvgGradientPtr repl)
{
/* replacing a Gradient with the corrensponding resolved item */
    rl2PrivSvgGradientPtr new_grad = svg_clone_gradient (repl, gradient);
    new_grad->prev = gradient->prev;
    new_grad->next = gradient->next;
    if (gradient->prev != NULL)
	gradient->prev->next = new_grad;
    if (gradient->next != NULL)
	gradient->next->prev = new_grad;
    if (svg_doc->first_grad == gradient)
	svg_doc->first_grad = new_grad;
    if (svg_doc->last_grad == gradient)
	svg_doc->last_grad = new_grad;
    svg_free_gradient (gradient);
    return new_grad;
}

static void
svg_resolve_gradients_xlink_href (rl2PrivSvgDocumentPtr svg_doc)
{
/* resolving any indirect reference: Gradient xlink:href */
    rl2PrivSvgGradientPtr grad = svg_doc->first_grad;
    rl2PrivSvgGradientPtr ret;

    while (grad)
      {
	  /* looping on Gradients */
	  if (grad->xlink_href != NULL)
	    {
		svg_find_gradient_href (svg_doc, grad->xlink_href, &ret);
		if (ret != NULL)
		    grad = svg_replace_gradient (svg_doc, grad, ret);
	    }
	  grad = grad->next;
      }
}

static void
svg_resolve_xlink_href (rl2PrivSvgDocumentPtr svg_doc, rl2PrivSvgItemPtr item)
{
/* recursively resolving any indirect reference: xlink:href */
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgUsePtr use;
    rl2PrivSvgClipPtr clip;
    rl2PrivSvgItemPtr ret;

    while (item)
      {
	  /* looping on Items */
	  if (item->type == RL2_SVG_ITEM_USE && item->pointer != NULL)
	    {
		use = item->pointer;
		svg_find_href (svg_doc, svg_doc->first, use->xlink_href, &ret);
		if (ret != NULL)
		    svg_replace_use (item, ret);
	    }
	  if (item->type == RL2_SVG_ITEM_GROUP && item->pointer != NULL)
	    {
		group = item->pointer;
		svg_resolve_xlink_href (svg_doc, group->first);
	    }
	  if (item->type == RL2_SVG_ITEM_CLIP && item->pointer != NULL)
	    {
		clip = item->pointer;
		svg_resolve_xlink_href (svg_doc, clip->first);
	    }
	  item = item->next;
      }
}

static void
svg_find_clip_href (rl2PrivSvgDocumentPtr svg_doc,
		    rl2PrivSvgItemPtr item, const char *href,
		    rl2PrivSvgItemPtr * pointer)
{
/* attempting to recursively resolve an xlink:href reference (ClipPath) */

    rl2PrivSvgGroupPtr group;
    rl2PrivSvgClipPtr clip;

    while (item)
      {
	  /* looping on Items */
	  if (item->type == RL2_SVG_ITEM_CLIP && item->pointer != NULL)
	    {
		clip = item->pointer;
		if (clip->id != NULL)
		  {
		      if (strcmp (clip->id, href) == 0)
			{
			    *pointer = item;
			    return;
			}
		  }
	    }
	  if (item->type == RL2_SVG_ITEM_GROUP && item->pointer != NULL)
	    {
		group = item->pointer;
		if (group->id != NULL)
		  {
		      if (strcmp (group->id, href + 1) == 0)
			{
			    *pointer = item;
			    return;
			}
		  }
		svg_find_clip_href (svg_doc, group->first, href, pointer);
	    }
	  item = item->next;
      }
}

static void
svg_resolve_clip_xlink_href (rl2PrivSvgDocumentPtr svg_doc,
			     rl2PrivSvgItemPtr item)
{
/* recursively resolving any indirect reference: ClipPath url */
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgShapePtr shape;
    rl2PrivSvgUsePtr use;
    rl2PrivSvgItemPtr ret = NULL;

    while (item)
      {
	  /* looping on Items */
	  if (item->type == RL2_SVG_ITEM_USE && item->pointer != NULL)
	    {
		use = item->pointer;
		if (use->style.clip_url != NULL)
		  {
		      svg_find_clip_href (svg_doc, svg_doc->first,
					  use->style.clip_url, &ret);
		      if (ret != NULL)
			  use->style.clip_pointer = ret;
		  }
	    }
	  if (item->type == RL2_SVG_ITEM_SHAPE && item->pointer != NULL)
	    {
		shape = item->pointer;
		if (shape->style.clip_url != NULL)
		  {
		      svg_find_clip_href (svg_doc, svg_doc->first,
					  shape->style.clip_url, &ret);
		      if (ret != NULL)
			  shape->style.clip_pointer = ret;
		  }
	    }
	  if (item->type == RL2_SVG_ITEM_GROUP && item->pointer != NULL)
	    {
		group = item->pointer;
		if (group->style.clip_url != NULL)
		  {
		      svg_find_clip_href (svg_doc, svg_doc->first,
					  group->style.clip_url, &ret);
		      if (ret != NULL)
			  group->style.clip_pointer = ret;
		  }
		svg_resolve_clip_xlink_href (svg_doc, group->first);
	    }
	  item = item->next;
      }
}

static int
svg_endian_arch ()
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

static int
svg_create_raster_buffers (const unsigned char *in_buf, int width, int height,
			   unsigned char **rgb, int *rgb_bytes,
			   unsigned char **mask, int *mask_bytes)
{
/* creating the RGB and Mask buffers from the Cairo Surface */
    int x;
    int y;
    const unsigned char *p_in;
    unsigned char *p_rgb;
    unsigned char *p_mask;
    int valid_mask = 0;

/* creating the output buffers */
    *rgb = NULL;
    *mask = NULL;
    *rgb_bytes = width * height * 3;
    *mask_bytes = width * height;
    *rgb = malloc (*rgb_bytes);
    if (*rgb == NULL)
	goto error;
    *mask = malloc (*mask_bytes);
    if (*mask == NULL)
	goto error;
    p_in = in_buf;
    p_rgb = *rgb;
    p_mask = *mask;

    for (y = 0; y < height; y++)
      {
	  /* looping on lines */
	  double multiplier;
	  double r;
	  double g;
	  double b;
	  for (x = 0; x < width; x++)
	    {
		/* looping on columns */
		unsigned char alpha;
		unsigned char red;
		unsigned char green;
		unsigned char blue;
		if (svg_endian_arch ())
		  {
		      /* little endian byte order */
		      alpha = *(p_in + 3);
		      red = *(p_in + 2);
		      green = *(p_in + 1);
		      blue = *(p_in + 0);
		  }
		else
		  {
		      /* big endian byte order */
		      alpha = *(p_in + 0);
		      red = *(p_in + 1);
		      green = *(p_in + 2);
		      blue = *(p_in + 3);
		  }
		/* Cairo colors are pre-multiplied; normalizing */
		multiplier = 255.0 / (double) alpha;
		r = (double) red *multiplier;
		g = (double) green *multiplier;
		b = (double) blue *multiplier;
		if (r < 0.0)
		    red = 0;
		else if (r > 255.0)
		    red = 255;
		else
		    red = r;
		if (g < 0.0)
		    green = 0;
		else if (g > 255.0)
		    green = 255;
		else
		    green = g;
		if (b < 0.0)
		    blue = 0;
		else if (b > 255.0)
		    blue = 255;
		else
		    blue = b;
		*p_rgb++ = red;
		*p_rgb++ = green;
		*p_rgb++ = blue;
		if (alpha < 128)
		  {
		      *p_mask++ = 0;
		      valid_mask = 1;
		  }
		else
		    *p_mask++ = 1;
		p_in += 4;
	    }
      }
    if (!valid_mask)
      {
	  free (*mask);
	  *mask = NULL;
	  *mask_bytes = 0;
      }
    return 1;

  error:
    if (*rgb != NULL)
	free (*rgb);
    if (*mask != NULL)
	free (*mask);
    *rgb = NULL;
    *rgb_bytes = 0;
    *mask = NULL;
    *mask_bytes = 0;
    return 0;
}

static rl2RasterPtr
svg_to_raster (rl2PrivSvgDocument * svg, double size)
{
/* rendering an SVG document as an RGBA image */
    cairo_surface_t *surface;
    cairo_t *cairo;
    double ratio_x;
    double ratio_y;
    double width;
    double height;
    double shift_x;
    double shift_y;
    rl2RasterPtr rst = NULL;
    int w;
    int h;
    const unsigned char *in_buf;
    unsigned char *rgb = NULL;
    int rgb_bytes;
    unsigned char *mask = NULL;
    int mask_bytes;

    if (svg->viewbox_x != DBL_MIN && svg->viewbox_y != DBL_MIN
	&& svg->viewbox_width != DBL_MIN && svg->viewbox_height != DBL_MIN)
      {
	  /* setting the SVG dimensions from the ViewBox */
	  if (svg->width <= 0)
	      svg->width = svg->viewbox_width;
	  if (svg->height <= 0)
	      svg->height = svg->viewbox_height;
      }
    else
      {
	  /* setting the ViewBox from the SVG dimensions */
	  svg->viewbox_x = 0.0;
	  svg->viewbox_y = 0.0;
	  svg->viewbox_width = svg->width;
	  svg->viewbox_height = svg->height;
      }
    if (svg->width <= 0.0 || svg->height <= 0.0)
	return NULL;

/* setting the image dimensions */
    ratio_x = svg->width / (double) size;
    ratio_y = svg->height / (double) size;
    if (ratio_x > ratio_y)
      {
	  width = svg->width / ratio_x;
	  height = svg->height / ratio_x;
      }
    else
      {
	  width = svg->width / ratio_y;
	  height = svg->height / ratio_y;
      }

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	return NULL;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
      {
	  fprintf (stderr, "CAIRO reports: Insufficient Memory\n");
	  goto stop;
      }

/* priming a transparent background */
    cairo_rectangle (cairo, 0, 0, width, height);
    cairo_set_source_rgba (cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (cairo);
/* setting the basic document matrix */
    svg->matrix.xx = 1.0;
    svg->matrix.yx = 0.0;
    svg->matrix.xy = 0.0;
    svg->matrix.yy = 1.0;
    svg->matrix.x0 = 0.0;
    svg->matrix.y0 = 0.0;
    ratio_x = width / svg->viewbox_width;
    ratio_y = height / svg->viewbox_height;
    cairo_matrix_scale (&(svg->matrix), ratio_x, ratio_y);
    shift_x = -1 * svg->viewbox_x;
    shift_y = -1 * svg->viewbox_y;
    cairo_matrix_translate (&(svg->matrix), shift_x, shift_y);

/* resolving any xlink:href */
    svg_resolve_gradients_xlink_href (svg);
    svg_resolve_clip_xlink_href (svg, svg->first);
    svg_resolve_xlink_href (svg, svg->first);

/* recursively rendering all SVG Items */
    svg_render_item (cairo, svg, svg->first);

/* accessing the CairoSurface buffer */
    w = cairo_image_surface_get_width (surface);
    h = cairo_image_surface_get_height (surface);
    cairo_surface_flush (surface);
    in_buf = cairo_image_surface_get_data (surface);
    if (in_buf == NULL)
	goto stop;

/* creating the Rasters buffer */
    if (!svg_create_raster_buffers
	(in_buf, w, h, &rgb, &rgb_bytes, &mask, &mask_bytes))
	goto stop;

/* creating the output Raster */
    rst =
	rl2_create_raster (w, h, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3, rgb,
			   rgb_bytes, NULL, mask, mask_bytes, NULL);

  stop:
    cairo_surface_destroy (surface);
    cairo_destroy (cairo);
    return rst;
}

RL2_DECLARE rl2SvgPtr
rl2_create_svg (const unsigned char *svg_document, int svg_bytes)
{
/* allocating and initializing an SVG object */
    rl2PrivSvgDocumentPtr svg = NULL;
    svg = svg_parse_doc (svg_document, svg_bytes);
    return (rl2SvgPtr) svg;
}

RL2_DECLARE void
rl2_destroy_svg (rl2SvgPtr ptr)
{
/* memory cleanup - destroying an SVG object */
    rl2PrivSvgDocumentPtr svg = (rl2PrivSvgDocumentPtr) ptr;
    if (svg == NULL)
	return;
    svg_free_document (svg);
}

RL2_DECLARE int
rl2_get_svg_size (rl2SvgPtr ptr, double *width, double *height)
{
/* return the SVG width and height */
    rl2PrivSvgDocumentPtr svg = (rl2PrivSvgDocumentPtr) ptr;
    if (svg == NULL)
	return RL2_ERROR;
    *width = svg->width;
    *height = svg->height;
    return RL2_OK;
}

RL2_DECLARE rl2RasterPtr
rl2_raster_from_svg (rl2SvgPtr ptr, double size)
{
/* rendering the SVG Document into a raster image */
    rl2PrivSvgDocumentPtr svg = (rl2PrivSvgDocumentPtr) ptr;
    if (svg == NULL)
	return NULL;
/* SVG image rendering */
    return svg_to_raster (svg, size);
}
