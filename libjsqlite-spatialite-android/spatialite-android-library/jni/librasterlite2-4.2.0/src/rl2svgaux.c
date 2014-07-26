/*

 rl2svgaux -- SVG helper auxiliary funcions

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

RL2_PRIVATE void
svg_free_transform (rl2PrivSvgTransformPtr p)
{
/* freeing an SVG Transform */
    if (p == NULL)
	return;
    if (p->data != NULL)
	free (p->data);
    free (p);
}

RL2_PRIVATE void
svg_free_polyline (rl2PrivSvgPolylinePtr p)
{
/* freeing an SVG Polyline */
    if (p == NULL)
	return;
    if (p->x != NULL)
	free (p->x);
    if (p->y != NULL)
	free (p->y);
    free (p);
}

RL2_PRIVATE void
svg_free_polygon (rl2PrivSvgPolygonPtr p)
{
/* freeing an SVG Polygon */
    if (p == NULL)
	return;
    if (p->x != NULL)
	free (p->x);
    if (p->y != NULL)
	free (p->y);
    free (p);
}

RL2_PRIVATE void
svg_free_path_item (rl2PrivSvgPathItemPtr p)
{
/* freeing an SVG Path item */
    if (p == NULL)
	return;
    if (p->data != NULL)
	free (p->data);
    free (p);
}

RL2_PRIVATE void
svg_free_path (rl2PrivSvgPathPtr p)
{
/* freeing an SVG Path */
    rl2PrivSvgPathItemPtr pp;
    rl2PrivSvgPathItemPtr ppn;
    if (p == NULL)
	return;
    pp = p->first;
    while (pp != NULL)
      {
	  ppn = pp->next;
	  svg_free_path_item (pp);
	  pp = ppn;
      }
    free (p);
}

RL2_PRIVATE void
svg_free_shape (rl2PrivSvgShapePtr p)
{
/* freeing an SVG Shape */
    rl2PrivSvgTransformPtr ptn;
    rl2PrivSvgTransformPtr pt = p->first_trans;
    if (p->id != NULL)
	free (p->id);
    while (pt)
      {
	  ptn = pt->next;
	  svg_free_transform (pt);
	  pt = ptn;
      }
    switch (p->type)
      {
      case RL2_SVG_POLYLINE:
	  svg_free_polyline (p->data);
	  break;
      case RL2_SVG_POLYGON:
	  svg_free_polygon (p->data);
	  break;
      case RL2_SVG_PATH:
	  svg_free_path (p->data);
	  break;
      default:
	  if (p->data)
	      free (p->data);
	  break;
      };
    svg_style_cleanup (&(p->style));
    free (p);
}

RL2_PRIVATE void
svg_free_use (rl2PrivSvgUsePtr p)
{
/* freeing an SVG Use (xlink:href) */
    rl2PrivSvgTransformPtr pt;
    rl2PrivSvgTransformPtr ptn;
    if (p->xlink_href != NULL)
	free (p->xlink_href);
    pt = p->first_trans;
    while (pt)
      {
	  ptn = pt->next;
	  svg_free_transform (pt);
	  pt = ptn;
      }
    svg_style_cleanup (&(p->style));
    free (p);
}

RL2_PRIVATE void
svg_free_item (rl2PrivSvgItemPtr p)
{
/* freeing an SVG Item */
    if (p->type == RL2_SVG_ITEM_GROUP)
	svg_free_group ((rl2PrivSvgGroupPtr) (p->pointer));
    if (p->type == RL2_SVG_ITEM_SHAPE)
	svg_free_shape ((rl2PrivSvgShapePtr) (p->pointer));
    if (p->type == RL2_SVG_ITEM_CLIP)
	svg_free_clip ((rl2PrivSvgClipPtr) (p->pointer));
    if (p->type == RL2_SVG_ITEM_USE)
	svg_free_use ((rl2PrivSvgUsePtr) (p->pointer));
    free (p);
}

RL2_PRIVATE void
svg_free_clip (rl2PrivSvgClipPtr p)
{
/* freeing an SVG Clip */
    rl2PrivSvgItemPtr pi;
    rl2PrivSvgItemPtr pin;
    if (p->id != NULL)
	free (p->id);
    pi = p->first;
    while (pi)
      {
	  pin = pi->next;
	  svg_free_item (pi);
	  pi = pin;
      }
    free (p);
}

RL2_PRIVATE void
svg_free_group (rl2PrivSvgGroupPtr p)
{
/* freeing an SVG Group <g> */
    rl2PrivSvgItemPtr pi;
    rl2PrivSvgItemPtr pin;
    rl2PrivSvgTransformPtr pt;
    rl2PrivSvgTransformPtr ptn;
    if (p->id != NULL)
	free (p->id);
    pi = p->first;
    while (pi)
      {
	  pin = pi->next;
	  svg_free_item (pi);
	  pi = pin;
      }
    pt = p->first_trans;
    while (pt)
      {
	  ptn = pt->next;
	  svg_free_transform (pt);
	  pt = ptn;
      }
    svg_style_cleanup (&(p->style));
    free (p);
}

RL2_PRIVATE void
svg_free_gradient_stop (rl2PrivSvgGradientStopPtr p)
{
/* freeing an SVG GradientStop */
    free (p);
}

RL2_PRIVATE void
svg_free_gradient (rl2PrivSvgGradient * p)
{
/* freeing an SVG Gradient */
    rl2PrivSvgTransformPtr pt;
    rl2PrivSvgTransformPtr ptn;
    rl2PrivSvgGradientStopPtr ps;
    rl2PrivSvgGradientStopPtr psn;
    if (p->id != NULL)
	free (p->id);
    if (p->xlink_href != NULL)
	free (p->xlink_href);
    pt = p->first_trans;
    while (pt)
      {
	  ptn = pt->next;
	  svg_free_transform (pt);
	  pt = ptn;
      }
    ps = p->first_stop;
    while (ps)
      {
	  psn = ps->next;
	  svg_free_gradient_stop (ps);
	  ps = psn;
      }
    free (p);
}

RL2_PRIVATE void
svg_free_document (rl2PrivSvgDocument * p)
{
/* freeing an SVG Document */
    rl2PrivSvgItemPtr pi;
    rl2PrivSvgItemPtr pin;
    rl2PrivSvgGradientPtr pg;
    rl2PrivSvgGradientPtr pgn;
    pi = p->first;
    while (pi)
      {
	  pin = pi->next;
	  if (pi->type == RL2_SVG_ITEM_GROUP)
	      svg_free_group ((rl2PrivSvgGroupPtr) (pi->pointer));
	  if (pi->type == RL2_SVG_ITEM_SHAPE)
	      svg_free_shape ((rl2PrivSvgShapePtr) (pi->pointer));
	  if (pi->type == RL2_SVG_ITEM_USE)
	      svg_free_use ((rl2PrivSvgUsePtr) (pi->pointer));
	  if (pi->type == RL2_SVG_ITEM_CLIP)
	      svg_free_clip ((rl2PrivSvgClipPtr) (pi->pointer));
	  free (pi);
	  pi = pin;
      }
    pg = p->first_grad;
    while (pg)
      {
	  pgn = pg->next;
	  svg_free_gradient (pg);
	  pg = pgn;
      }
    free (p);
}

RL2_PRIVATE void
svg_init_style (rl2PrivSvgStylePtr style)
{
/* initializing a void style */
    style->visibility = -1;
    style->opacity = 1.0;
    style->fill = -1;
    style->no_fill = -1;
    style->fill_rule = -1;
    style->fill_url = NULL;
    style->fill_pointer = NULL;
    style->fill_red = -1.0;
    style->fill_green = -1.0;
    style->fill_blue = -1.0;
    style->fill_opacity = -1.0;
    style->stroke = -1;
    style->no_stroke = -1;
    style->stroke_width = -1.0;
    style->stroke_linecap = -1;
    style->stroke_linejoin = -1;
    style->stroke_miterlimit = -1.0;
    style->stroke_dashitems = 0;
    style->stroke_dasharray = NULL;
    style->stroke_dashoffset = 0.0;
    style->stroke_url = NULL;
    style->stroke_pointer = NULL;
    style->stroke_red = -1.0;
    style->stroke_green = -1.0;
    style->stroke_blue = -1.0;
    style->stroke_opacity = -1.0;
    style->clip_url = NULL;
    style->clip_pointer = NULL;
}

RL2_PRIVATE void
svg_style_cleanup (rl2PrivSvgStylePtr style)
{
/* style cleanup */
    if (style->stroke_dasharray != NULL)
	free (style->stroke_dasharray);
    if (style->fill_url != NULL)
	free (style->fill_url);
    if (style->stroke_url != NULL)
	free (style->stroke_url);
    if (style->clip_url != NULL)
	free (style->clip_url);
}

RL2_PRIVATE rl2PrivSvgMatrixPtr
svg_alloc_matrix (double a, double b, double c, double d, double e, double f)
{
/* allocating and initializing an SVG Matrix */
    rl2PrivSvgMatrixPtr p = malloc (sizeof (rl2PrivSvgMatrix));
    p->a = a;
    p->b = b;
    p->c = c;
    p->d = d;
    p->e = e;
    p->f = f;
    return p;
}

RL2_PRIVATE rl2PrivSvgMatrixPtr
svg_clone_matrix (rl2PrivSvgMatrixPtr in)
{
/* cloning an SVG Matrix */
    rl2PrivSvgMatrixPtr p = malloc (sizeof (rl2PrivSvgMatrix));
    p->a = in->a;
    p->b = in->b;
    p->c = in->c;
    p->d = in->d;
    p->e = in->e;
    p->f = in->f;
    return p;
}

RL2_PRIVATE rl2PrivSvgTranslatePtr
svg_alloc_translate (double tx, double ty)
{
/* allocating and initializing an SVG Translate */
    rl2PrivSvgTranslatePtr p = malloc (sizeof (rl2PrivSvgTranslate));
    p->tx = tx;
    p->ty = ty;
    return p;
}

RL2_PRIVATE rl2PrivSvgTranslatePtr
svg_clone_translate (rl2PrivSvgTranslatePtr in)
{
/* cloning an SVG Translate */
    rl2PrivSvgTranslatePtr p = malloc (sizeof (rl2PrivSvgTranslate));
    p->tx = in->tx;
    p->ty = in->ty;
    return p;
}

RL2_PRIVATE rl2PrivSvgScalePtr
svg_alloc_scale (double sx, double sy)
{
/* allocating and initializing an SVG Scale */
    rl2PrivSvgScalePtr p = malloc (sizeof (rl2PrivSvgScale));
    p->sx = sx;
    p->sy = sy;
    return p;
}

RL2_PRIVATE rl2PrivSvgScalePtr
svg_clone_scale (rl2PrivSvgScalePtr in)
{
/* cloning an SVG Scale */
    rl2PrivSvgScalePtr p = malloc (sizeof (rl2PrivSvgScale));
    p->sx = in->sx;
    p->sy = in->sy;
    return p;
}

RL2_PRIVATE rl2PrivSvgRotatePtr
svg_alloc_rotate (double angle, double cx, double cy)
{
/* allocating and initializing an SVG Scale */
    rl2PrivSvgRotatePtr p = malloc (sizeof (rl2PrivSvgRotate));
    p->angle = angle;
    p->cx = cx;
    p->cy = cy;
    return p;
}

RL2_PRIVATE rl2PrivSvgRotatePtr
svg_clone_rotate (rl2PrivSvgRotatePtr in)
{
/* cloning an SVG Scale */
    rl2PrivSvgRotatePtr p = malloc (sizeof (rl2PrivSvgRotate));
    p->angle = in->angle;
    p->cx = in->cx;
    p->cy = in->cy;
    return p;
}

RL2_PRIVATE rl2PrivSvgSkewPtr
svg_alloc_skew (double angle)
{
/* allocating and initializing an SVG Skew */
    rl2PrivSvgSkewPtr p = malloc (sizeof (rl2PrivSvgSkew));
    p->angle = angle;
    return p;
}

RL2_PRIVATE rl2PrivSvgSkewPtr
svg_clone_skew (rl2PrivSvgSkewPtr in)
{
/* cloning an SVG Skew */
    rl2PrivSvgSkewPtr p = malloc (sizeof (rl2PrivSvgSkew));
    p->angle = in->angle;
    return p;
}

RL2_PRIVATE rl2PrivSvgTransformPtr
svg_alloc_transform (int type, void *data)
{
/* allocating and initializing an empty SVG Transform */
    rl2PrivSvgTransformPtr p = malloc (sizeof (rl2PrivSvgTransform));
    p->type = type;
    p->data = data;
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgTransformPtr
svg_clone_transform (rl2PrivSvgTransformPtr in)
{
/* cloning SVG Transform */
    rl2PrivSvgMatrixPtr matrix;
    rl2PrivSvgTranslatePtr translate;
    rl2PrivSvgScalePtr scale;
    rl2PrivSvgRotatePtr rotate;
    rl2PrivSvgSkewPtr skew;
    rl2PrivSvgTransformPtr p = malloc (sizeof (rl2PrivSvgTransform));
    p->type = in->type;
    switch (in->type)
      {
      case RL2_SVG_MATRIX:
	  matrix = in->data;
	  p->data = svg_clone_matrix (matrix);
	  break;
      case RL2_SVG_TRANSLATE:
	  translate = in->data;
	  p->data = svg_clone_translate (translate);
	  break;
      case RL2_SVG_SCALE:
	  scale = in->data;
	  p->data = svg_clone_scale (scale);
	  break;
      case RL2_SVG_ROTATE:
	  rotate = in->data;
	  p->data = svg_clone_rotate (rotate);
	  break;
      case RL2_SVG_SKEW_X:
      case RL2_SVG_SKEW_Y:
	  skew = in->data;
	  p->data = svg_clone_skew (skew);
	  break;
      };
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgRectPtr
svg_alloc_rect (double x, double y, double width, double height, double rx,
		double ry)
{
/* allocating and initializing an SVG Rect */
    rl2PrivSvgRectPtr p = malloc (sizeof (rl2PrivSvgRect));
    p->x = x;
    p->y = y;
    p->width = width;
    p->height = height;
    p->rx = rx;
    p->ry = ry;
    return p;
}

RL2_PRIVATE rl2PrivSvgRectPtr
svg_clone_rect (rl2PrivSvgRectPtr in)
{
/* cloning an SVG Rect */
    rl2PrivSvgRectPtr p = malloc (sizeof (rl2PrivSvgRect));
    p->x = in->x;
    p->y = in->y;
    p->width = in->width;
    p->height = in->height;
    p->rx = in->rx;
    p->ry = in->ry;
    return p;
}

RL2_PRIVATE rl2PrivSvgCirclePtr
svg_alloc_circle (double cx, double cy, double r)
{
/* allocating and initializing an SVG Circle */
    rl2PrivSvgCirclePtr p = malloc (sizeof (rl2PrivSvgCircle));
    p->cx = cx;
    p->cy = cy;
    p->r = r;
    return p;
}

RL2_PRIVATE rl2PrivSvgCirclePtr
svg_clone_circle (rl2PrivSvgCirclePtr in)
{
/* cloning an SVG Circle */
    rl2PrivSvgCirclePtr p = malloc (sizeof (rl2PrivSvgCircle));
    p->cx = in->cx;
    p->cy = in->cy;
    p->r = in->r;
    return p;
}

RL2_PRIVATE rl2PrivSvgEllipsePtr
svg_alloc_ellipse (double cx, double cy, double rx, double ry)
{
/* allocating and initializing an SVG Ellipse */
    rl2PrivSvgEllipsePtr p = malloc (sizeof (rl2PrivSvgEllipse));
    p->cx = cx;
    p->cy = cy;
    p->rx = rx;
    p->ry = ry;
    return p;
}

RL2_PRIVATE rl2PrivSvgEllipsePtr
svg_clone_ellipse (rl2PrivSvgEllipsePtr in)
{
/* cloning an SVG Ellipse */
    rl2PrivSvgEllipsePtr p = malloc (sizeof (rl2PrivSvgEllipse));
    p->cx = in->cx;
    p->cy = in->cy;
    p->rx = in->rx;
    p->ry = in->ry;
    return p;
}

RL2_PRIVATE rl2PrivSvgLinePtr
svg_alloc_line (double x1, double y1, double x2, double y2)
{
/* allocating and initializing an SVG Line */
    rl2PrivSvgLinePtr p = malloc (sizeof (rl2PrivSvgLine));
    p->x1 = x1;
    p->y1 = y1;
    p->x2 = x2;
    p->y2 = y2;
    return p;
}

RL2_PRIVATE rl2PrivSvgLinePtr
svg_clone_line (rl2PrivSvgLinePtr in)
{
/* cloning an SVG Line */
    rl2PrivSvgLinePtr p = malloc (sizeof (rl2PrivSvgLine));
    p->x1 = in->x1;
    p->y1 = in->y1;
    p->x2 = in->x2;
    p->y2 = in->y2;
    return p;
}

RL2_PRIVATE rl2PrivSvgPolylinePtr
svg_alloc_polyline (int points, double *x, double *y)
{
/* allocating and initializing an SVG Polyline */
    rl2PrivSvgPolylinePtr p = malloc (sizeof (rl2PrivSvgPolyline));
    p->points = points;
    p->x = x;
    p->y = y;
    return p;
}

RL2_PRIVATE rl2PrivSvgPolylinePtr
svg_clone_polyline (rl2PrivSvgPolylinePtr in)
{
/* cloning an SVG Polyline */
    int iv;
    rl2PrivSvgPolylinePtr p = malloc (sizeof (rl2PrivSvgPolyline));
    p->points = in->points;
    p->x = malloc (sizeof (double) * in->points);
    p->y = malloc (sizeof (double) * in->points);
    for (iv = 0; iv < in->points; iv++)
      {
	  *(p->x + iv) = *(in->x + iv);
	  *(p->y + iv) = *(in->y + iv);
      }
    return p;
}

RL2_PRIVATE rl2PrivSvgPolygonPtr
svg_alloc_polygon (int points, double *x, double *y)
{
/* allocating and initializing an SVG Polygon */
    rl2PrivSvgPolygonPtr p = malloc (sizeof (rl2PrivSvgPolygon));
    p->points = points;
    p->x = x;
    p->y = y;
    return p;
}

RL2_PRIVATE rl2PrivSvgPolygonPtr
svg_clone_polygon (rl2PrivSvgPolygonPtr in)
{
/* cloning an SVG Polygon */
    int iv;
    rl2PrivSvgPolygonPtr p = malloc (sizeof (rl2PrivSvgPolygon));
    p->points = in->points;
    p->x = malloc (sizeof (double) * in->points);
    p->y = malloc (sizeof (double) * in->points);
    for (iv = 0; iv < in->points; iv++)
      {
	  *(p->x + iv) = *(in->x + iv);
	  *(p->y + iv) = *(in->y + iv);
      }
    return p;
}

RL2_PRIVATE rl2PrivSvgPathMovePtr
svg_alloc_path_move (double x, double y)
{
/* allocating and initializing an SVG Path MoveTo or LineTo */
    rl2PrivSvgPathMovePtr p = malloc (sizeof (rl2PrivSvgPathMove));
    p->x = x;
    p->y = y;
    return p;
}

RL2_PRIVATE rl2PrivSvgPathMovePtr
svg_clone_path_move (rl2PrivSvgPathMovePtr in)
{
/* cloning an SVG Path MoveTo or LineTo */
    rl2PrivSvgPathMovePtr p = malloc (sizeof (rl2PrivSvgPathMove));
    p->x = in->x;
    p->y = in->y;
    return p;
}

RL2_PRIVATE rl2PrivSvgPathBezierPtr
svg_alloc_path_bezier (double x1, double y1, double x2, double y2, double x,
		       double y)
{
/* allocating and initializing an SVG Bezier Curve */
    rl2PrivSvgPathBezierPtr p = malloc (sizeof (rl2PrivSvgPathBezier));
    p->x1 = x1;
    p->y1 = y1;
    p->x2 = x2;
    p->y2 = y2;
    p->x = x;
    p->y = y;
    return p;
}

RL2_PRIVATE rl2PrivSvgPathBezierPtr
svg_clone_path_bezier (rl2PrivSvgPathBezierPtr in)
{
/* cloning an SVG Bezier Curve */
    rl2PrivSvgPathBezierPtr p = malloc (sizeof (rl2PrivSvgPathBezier));
    p->x1 = in->x1;
    p->y1 = in->y1;
    p->x2 = in->x2;
    p->y2 = in->y2;
    p->x = in->x;
    p->y = in->y;
    return p;
}

RL2_PRIVATE rl2PrivSvgPathEllipticArcPtr
svg_alloc_path_ellipt_arc (double rx, double ry, double rotation,
			   int large_arc, int sweep, double x, double y)
{
/* allocating and initializing an SVG Elliptic Arc */
    rl2PrivSvgPathEllipticArcPtr p =
	malloc (sizeof (rl2PrivSvgPathEllipticArc));
    p->rx = rx;
    p->ry = ry;
    p->rotation = rotation;
    if (large_arc == 0)
	p->large_arc = 0;
    else
	p->large_arc = 1;
    if (sweep == 0)
	p->sweep = 0;
    else
	p->sweep = 1;
    p->x = x;
    p->y = y;
    return p;
}

RL2_PRIVATE rl2PrivSvgPathEllipticArcPtr
svg_clone_path_ellipt_arc (rl2PrivSvgPathEllipticArcPtr in)
{
/* cloning an SVG Elliptic Arc */
    rl2PrivSvgPathEllipticArcPtr p =
	malloc (sizeof (rl2PrivSvgPathEllipticArc));
    p->rx = in->rx;
    p->ry = in->ry;
    p->rotation = in->rotation;
    p->large_arc = in->large_arc;
    p->sweep = in->sweep;
    p->x = in->x;
    p->y = in->y;
    return p;
}

RL2_PRIVATE rl2PrivSvgPathItemPtr
svg_alloc_path_item (int type, void *data)
{
/* allocating and initializing an empty SVG Path item */
    rl2PrivSvgPathItemPtr p = malloc (sizeof (rl2PrivSvgPathItem));
    p->type = type;
    p->data = data;
    p->next = NULL;
    return p;
}

RL2_PRIVATE void
svg_add_path_item (rl2PrivSvgPathPtr path, int type, void *data)
{
/* add a Command to an SVG Path */
    rl2PrivSvgPathItemPtr p = svg_alloc_path_item (type, data);
    if (path->first == NULL)
	path->first = p;
    if (path->last != NULL)
	path->last->next = p;
    path->last = p;
}

RL2_PRIVATE rl2PrivSvgPathPtr
svg_alloc_path (void)
{
/* allocating and initializing an empty SVG Path */
    rl2PrivSvgPathPtr p = malloc (sizeof (rl2PrivSvgPath));
    p->first = NULL;
    p->last = NULL;
    p->error = 0;
    return p;
}

RL2_PRIVATE rl2PrivSvgPathPtr
svg_clone_path (rl2PrivSvgPathPtr in)
{
/* cloning an empty SVG Path */
    rl2PrivSvgPathMovePtr move;
    rl2PrivSvgPathBezierPtr bezier;
    rl2PrivSvgPathEllipticArcPtr arc;
    rl2PrivSvgPathItemPtr pp;
    rl2PrivSvgPathPtr p = malloc (sizeof (rl2PrivSvgPath));
    p->first = NULL;
    p->last = NULL;
    pp = in->first;
    while (pp != NULL)
      {
	  switch (pp->type)
	    {
	    case RL2_SVG_MOVE_TO:
	    case RL2_SVG_LINE_TO:
		move = svg_clone_path_move (pp->data);
		svg_add_path_item (p, pp->type, move);
		break;
	    case RL2_SVG_CURVE_3:
	    case RL2_SVG_CURVE_4:
		bezier = svg_clone_path_bezier (pp->data);
		svg_add_path_item (p, pp->type, bezier);
		break;
	    case RL2_SVG_ELLIPT_ARC:
		arc = svg_clone_path_ellipt_arc (pp->data);
		svg_add_path_item (p, pp->type, arc);
		break;
	    case RL2_SVG_CLOSE_PATH:
		svg_add_path_item (p, pp->type, NULL);
		break;
	    };
	  pp = pp->next;
      }
    p->error = in->error;
    return p;
}

RL2_PRIVATE void
svg_add_clip_url (rl2PrivSvgStylePtr style, const char *url)
{
/* adding an URL (ClipPath ref) to some Style */
    int len;
    if (style->clip_url != NULL)
	free (style->clip_url);
    if (url == NULL)
      {
	  style->clip_url = NULL;
	  return;
      }
    len = strlen (url);
    style->clip_url = malloc (len + 1);
    strcpy (style->clip_url, url);
}

RL2_PRIVATE void
svg_add_fill_gradient_url (rl2PrivSvgStylePtr style, const char *url)
{
/* adding an URL (Fill Gradient ref) to some Style */
    int len;
    if (style->fill_url != NULL)
	free (style->fill_url);
    if (url == NULL)
      {
	  style->fill_url = NULL;
	  return;
      }
    len = strlen (url);
    style->fill_url = malloc (len + 1);
    strcpy (style->fill_url, url);
}

RL2_PRIVATE void
svg_add_stroke_gradient_url (rl2PrivSvgStylePtr style, const char *url)
{
/* adding an URL (Stroke Gradient ref) to some Style */
    int len;
    if (style->stroke_url != NULL)
	free (style->stroke_url);
    if (url == NULL)
      {
	  style->stroke_url = NULL;
	  return;
      }
    len = strlen (url);
    style->stroke_url = malloc (len + 1);
    strcpy (style->stroke_url, url);
}

RL2_PRIVATE rl2PrivSvgShapePtr
svg_alloc_shape (int type, void *data, rl2PrivSvgGroupPtr parent)
{
/* allocating and initializing an empty SVG Shape */
    rl2PrivSvgShapePtr p = malloc (sizeof (rl2PrivSvgShape));
    p->id = NULL;
    p->type = type;
    p->data = data;
    p->parent = parent;
    svg_init_style (&(p->style));
    p->first_trans = NULL;
    p->last_trans = NULL;
    p->is_defs = 0;
    p->is_flow_root = 0;
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgShapePtr
svg_clone_shape (rl2PrivSvgShapePtr in, rl2PrivSvgUsePtr use)
{
/* cloning an SVG Shape */
    rl2PrivSvgRectPtr rect;
    rl2PrivSvgCirclePtr circle;
    rl2PrivSvgEllipsePtr ellipse;
    rl2PrivSvgLinePtr line;
    rl2PrivSvgPolylinePtr polyline;
    rl2PrivSvgPolygonPtr polygon;
    rl2PrivSvgPathPtr path;
    rl2PrivSvgTransformPtr pt;
    rl2PrivSvgTransformPtr trans;
    rl2PrivSvgShapePtr out = malloc (sizeof (rl2PrivSvgShape));
    out->id = NULL;
    out->type = in->type;
    switch (in->type)
      {
      case RL2_SVG_RECT:
	  rect = in->data;
	  out->data = svg_clone_rect (rect);
	  break;
      case RL2_SVG_CIRCLE:
	  circle = in->data;
	  out->data = svg_clone_circle (circle);
	  break;
      case RL2_SVG_ELLIPSE:
	  ellipse = in->data;
	  out->data = svg_clone_ellipse (ellipse);
	  break;
      case RL2_SVG_LINE:
	  line = in->data;
	  out->data = svg_clone_line (line);
	  break;
      case RL2_SVG_POLYLINE:
	  polyline = in->data;
	  out->data = svg_clone_polyline (polyline);
	  break;
      case RL2_SVG_POLYGON:
	  polygon = in->data;
	  out->data = svg_clone_polygon (polygon);
	  break;
      case RL2_SVG_PATH:
	  path = in->data;
	  out->data = svg_clone_path (path);
	  break;
      }
    if (use != NULL)
	out->parent = use->parent;
    else
	out->parent = in->parent;
    out->style.visibility = in->style.visibility;
    out->style.opacity = in->style.opacity;
    out->style.fill = in->style.fill;
    out->style.no_fill = in->style.no_fill;
    out->style.fill_rule = in->style.fill_rule;
    out->style.fill_url = NULL;
    out->style.fill_pointer = NULL;
    if (in->style.fill_url != NULL)
	svg_add_fill_gradient_url (&(out->style), in->style.fill_url);
    out->style.fill_red = in->style.fill_red;
    out->style.fill_green = in->style.fill_green;
    out->style.fill_blue = in->style.fill_blue;
    out->style.fill_opacity = in->style.fill_opacity;
    out->style.stroke = in->style.stroke;
    out->style.no_stroke = in->style.no_stroke;
    out->style.stroke_width = in->style.stroke_width;
    out->style.stroke_linecap = in->style.stroke_linecap;
    out->style.stroke_linejoin = in->style.stroke_linejoin;
    out->style.stroke_miterlimit = in->style.stroke_miterlimit;
    out->style.stroke_dashitems = 0;
    out->style.stroke_dasharray = NULL;
    if (in->style.stroke_dashitems > 0)
      {
	  out->style.stroke_dashitems = in->style.stroke_dashitems;
	  if (out->style.stroke_dasharray != NULL)
	      free (out->style.stroke_dasharray);
	  out->style.stroke_dasharray = NULL;
	  if (in->style.stroke_dashitems > 0)
	    {
		int i;
		out->style.stroke_dasharray =
		    malloc (sizeof (double) * in->style.stroke_dashitems);
		for (i = 0; i < in->style.stroke_dashitems; i++)
		    out->style.stroke_dasharray[i] =
			in->style.stroke_dasharray[i];
	    }
	  out->style.stroke_dashoffset = in->style.stroke_dashoffset;
      }
    out->style.stroke_url = NULL;
    out->style.stroke_pointer = NULL;
    if (in->style.stroke_url != NULL)
	svg_add_stroke_gradient_url (&(out->style), in->style.stroke_url);
    out->style.stroke_red = in->style.stroke_red;
    out->style.stroke_green = in->style.stroke_green;
    out->style.stroke_blue = in->style.stroke_blue;
    out->style.stroke_opacity = in->style.stroke_opacity;
    out->style.clip_url = NULL;
    out->style.clip_pointer = NULL;
    if (in->style.clip_url != NULL)
	svg_add_clip_url (&(out->style), in->style.clip_url);
    out->first_trans = NULL;
    out->last_trans = NULL;
    pt = in->first_trans;
    while (pt)
      {
	  /* clonig all transformations */
	  trans = svg_clone_transform (pt);
	  if (out->first_trans == NULL)
	      out->first_trans = trans;
	  if (out->last_trans != NULL)
	      out->last_trans->next = trans;
	  out->last_trans = trans;
	  pt = pt->next;
      }
    out->is_defs = 0;
    out->is_flow_root = 0;
    out->next = NULL;
    if (use != NULL)
      {
	  /* adding the Use-level defs */
	  pt = use->first_trans;
	  while (pt)
	    {
		/* clonig all Use transformations */
		trans = svg_clone_transform (pt);
		if (out->first_trans == NULL)
		    out->first_trans = trans;
		if (out->last_trans != NULL)
		    out->last_trans->next = trans;
		out->last_trans = trans;
		pt = pt->next;
	    }
	  if (use->x != DBL_MAX || use->y != DBL_MAX)
	    {
		/* adding the implict Use Translate transformation */
		rl2PrivSvgTranslatePtr translate = NULL;
		if (use->x != DBL_MAX && use->y != DBL_MAX)
		    translate = svg_alloc_translate (use->x, use->y);
		else if (use->x != DBL_MAX)
		    translate = svg_alloc_translate (use->x, 0.0);
		else if (use->y != DBL_MAX)
		    translate = svg_alloc_translate (0.0, use->y);
		trans = svg_alloc_transform (RL2_SVG_TRANSLATE, translate);
		if (out->first_trans == NULL)
		    out->first_trans = trans;
		if (out->last_trans != NULL)
		    out->last_trans->next = trans;
		out->last_trans = trans;
	    }
	  /* Use-level styles */
	  if (use->style.visibility >= 0)
	      out->style.visibility = use->style.visibility;
	  out->style.opacity = use->style.opacity;
	  if (use->style.fill >= 0)
	      out->style.fill = use->style.fill;
	  if (use->style.no_fill >= 0)
	      out->style.no_fill = use->style.no_fill;
	  if (use->style.fill_rule >= 0)
	      out->style.fill_rule = use->style.fill_rule;
	  if (use->style.fill_url != NULL)
	      svg_add_fill_gradient_url (&(out->style), use->style.fill_url);
	  if (use->style.fill_red >= 0.0)
	      out->style.fill_red = use->style.fill_red;
	  if (use->style.fill_green >= 0.0)
	      out->style.fill_green = use->style.fill_green;
	  if (use->style.fill_blue >= 0.0)
	      out->style.fill_blue = use->style.fill_blue;
	  if (use->style.fill_opacity >= 0.0)
	      out->style.fill_opacity = use->style.fill_opacity;
	  if (use->style.stroke >= 0)
	      out->style.stroke = use->style.stroke;
	  if (use->style.no_stroke >= 0)
	      out->style.no_stroke = use->style.no_stroke;
	  if (use->style.stroke_width >= 0.0)
	      out->style.stroke_width = use->style.stroke_width;
	  if (use->style.stroke_linecap >= 0)
	      out->style.stroke_linecap = use->style.stroke_linecap;
	  if (use->style.stroke_linejoin >= 0)
	      out->style.stroke_linejoin = use->style.stroke_linejoin;
	  if (use->style.stroke_miterlimit >= 0.0)
	      out->style.stroke_miterlimit = use->style.stroke_miterlimit;
	  if (use->style.stroke_dashitems > 0)
	    {
		out->style.stroke_dashitems = use->style.stroke_dashitems;
		if (out->style.stroke_dasharray != NULL)
		    free (out->style.stroke_dasharray);
		out->style.stroke_dasharray = NULL;
		if (use->style.stroke_dashitems > 0)
		  {
		      int i;
		      out->style.stroke_dasharray =
			  malloc (sizeof (double) *
				  use->style.stroke_dashitems);
		      for (i = 0; i < use->style.stroke_dashitems; i++)
			  out->style.stroke_dasharray[i] =
			      use->style.stroke_dasharray[i];
		  }
		out->style.stroke_dashoffset = use->style.stroke_dashoffset;
	    }
	  if (use->style.stroke_url != NULL)
	      svg_add_stroke_gradient_url (&(out->style),
					   use->style.stroke_url);
	  if (use->style.stroke_red >= 0.0)
	      out->style.stroke_red = use->style.stroke_red;
	  if (use->style.stroke_green >= 0.0)
	      out->style.stroke_green = use->style.stroke_green;
	  if (use->style.stroke_blue >= 0.0)
	      out->style.stroke_blue = use->style.stroke_blue;
	  if (use->style.stroke_opacity >= 0.0)
	      out->style.stroke_opacity = use->style.stroke_opacity;
	  if (use->style.clip_url != NULL)
	      svg_add_clip_url (&(out->style), use->style.clip_url);
      }
    return out;
}

RL2_PRIVATE void
svg_add_shape_id (rl2PrivSvgShapePtr shape, const char *id)
{
/* setting the ID for some Shape */
    int len = strlen (id);
    if (shape->id != NULL)
	free (shape->id);
    shape->id = malloc (len + 1);
    strcpy (shape->id, id);
}

RL2_PRIVATE rl2PrivSvgUsePtr
svg_alloc_use (void *parent, const char *xlink_href, double x, double y,
	       double width, double height)
{
/* allocating and initializing an empty SVG Use (xlink:href) */
    int len = strlen (xlink_href);
    rl2PrivSvgUsePtr p = malloc (sizeof (rl2PrivSvgUse));
    p->xlink_href = malloc (len + 1);
    strcpy (p->xlink_href, xlink_href);
    p->x = x;
    p->y = y;
    p->width = width;
    p->height = height;
    p->parent = parent;
    svg_init_style (&(p->style));
    p->first_trans = NULL;
    p->last_trans = NULL;
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgUsePtr
svg_clone_use (rl2PrivSvgUsePtr in)
{
/* cloning an SVG Use (xlink:href) */
    rl2PrivSvgTransformPtr pt;
    rl2PrivSvgTransformPtr trans;
    int len = strlen (in->xlink_href);
    rl2PrivSvgUsePtr out = malloc (sizeof (rl2PrivSvgUse));
    out->xlink_href = malloc (len + 1);
    strcpy (out->xlink_href, in->xlink_href);
    out->x = in->x;
    out->y = in->y;
    out->width = in->width;
    out->height = in->height;
    out->parent = in->parent;
    out->style.visibility = in->style.visibility;
    out->style.opacity = in->style.opacity;
    out->style.fill = in->style.fill;
    out->style.no_fill = in->style.no_fill;
    out->style.fill_rule = in->style.fill_rule;
    out->style.fill_url = NULL;
    out->style.fill_pointer = NULL;
    if (in->style.fill_url != NULL)
	svg_add_fill_gradient_url (&(out->style), in->style.fill_url);
    out->style.fill_red = in->style.fill_red;
    out->style.fill_green = in->style.fill_green;
    out->style.fill_blue = in->style.fill_blue;
    out->style.fill_opacity = in->style.fill_opacity;
    out->style.stroke = in->style.stroke;
    out->style.no_stroke = in->style.no_stroke;
    out->style.stroke_width = in->style.stroke_width;
    out->style.stroke_linecap = in->style.stroke_linecap;
    out->style.stroke_linejoin = in->style.stroke_linejoin;
    out->style.stroke_miterlimit = in->style.stroke_miterlimit;
    out->style.stroke_dashitems = 0;
    out->style.stroke_dasharray = NULL;
    if (in->style.stroke_dashitems > 0)
      {
	  out->style.stroke_dashitems = in->style.stroke_dashitems;
	  if (out->style.stroke_dasharray != NULL)
	      free (out->style.stroke_dasharray);
	  out->style.stroke_dasharray = NULL;
	  if (in->style.stroke_dashitems > 0)
	    {
		int i;
		out->style.stroke_dasharray =
		    malloc (sizeof (double) * in->style.stroke_dashitems);
		for (i = 0; i < in->style.stroke_dashitems; i++)
		    out->style.stroke_dasharray[i] =
			in->style.stroke_dasharray[i];
	    }
	  out->style.stroke_dashoffset = in->style.stroke_dashoffset;
      }
    out->style.stroke_url = NULL;
    out->style.stroke_pointer = NULL;
    if (in->style.stroke_url != NULL)
	svg_add_stroke_gradient_url (&(out->style), in->style.stroke_url);
    out->style.stroke_red = in->style.stroke_red;
    out->style.stroke_green = in->style.stroke_green;
    out->style.stroke_blue = in->style.stroke_blue;
    out->style.stroke_opacity = in->style.stroke_opacity;
    out->style.clip_url = NULL;
    out->style.clip_pointer = NULL;
    if (in->style.clip_url != NULL)
	svg_add_clip_url (&(out->style), in->style.clip_url);
    out->first_trans = NULL;
    out->last_trans = NULL;
    pt = in->first_trans;
    while (pt)
      {
	  /* clonig all transformations */
	  trans = svg_clone_transform (pt);
	  if (out->first_trans == NULL)
	      out->first_trans = trans;
	  if (out->last_trans != NULL)
	      out->last_trans->next = trans;
	  out->last_trans = trans;
	  pt = pt->next;
      }
    out->next = NULL;
    return out;
}

RL2_PRIVATE rl2PrivSvgGroupPtr
svg_alloc_group (void)
{
/* allocating and initializing an empty SVG Group <g> */
    rl2PrivSvgGroupPtr p = malloc (sizeof (rl2PrivSvgGroup));
    p->id = NULL;
    svg_init_style (&(p->style));
    p->parent = NULL;
    p->first = NULL;
    p->last = NULL;
    p->first_trans = NULL;
    p->last_trans = NULL;
    p->is_defs = 0;
    p->is_flow_root = 0;
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgGroupPtr
svg_clone_group (rl2PrivSvgGroupPtr in, rl2PrivSvgUsePtr use)
{
/* cloning an SVG Group */
    rl2PrivSvgTransformPtr pt;
    rl2PrivSvgTransformPtr trans;
    rl2PrivSvgItemPtr item;
    rl2PrivSvgItemPtr itm;
    rl2PrivSvgGroupPtr out = malloc (sizeof (rl2PrivSvgGroup));
    out->id = NULL;
    out->style.visibility = in->style.visibility;
    out->style.opacity = in->style.opacity;
    out->style.fill = in->style.fill;
    out->style.no_fill = in->style.no_fill;
    out->style.fill_rule = in->style.fill_rule;
    out->style.fill_url = NULL;
    out->style.fill_pointer = NULL;
    if (in->style.fill_url != NULL)
	svg_add_fill_gradient_url (&(out->style), in->style.fill_url);
    out->style.fill_red = in->style.fill_red;
    out->style.fill_green = in->style.fill_green;
    out->style.fill_blue = in->style.fill_blue;
    out->style.fill_opacity = in->style.fill_opacity;
    out->style.stroke = in->style.stroke;
    out->style.no_stroke = in->style.no_stroke;
    out->style.stroke_width = in->style.stroke_width;
    out->style.stroke_linecap = in->style.stroke_linecap;
    out->style.stroke_linejoin = in->style.stroke_linejoin;
    out->style.stroke_miterlimit = in->style.stroke_miterlimit;
    out->style.stroke_dashitems = 0;
    out->style.stroke_dasharray = NULL;
    if (in->style.stroke_dashitems > 0)
      {
	  out->style.stroke_dashitems = in->style.stroke_dashitems;
	  if (out->style.stroke_dasharray != NULL)
	      free (out->style.stroke_dasharray);
	  out->style.stroke_dasharray = NULL;
	  if (in->style.stroke_dashitems > 0)
	    {
		int i;
		out->style.stroke_dasharray =
		    malloc (sizeof (double) * in->style.stroke_dashitems);
		for (i = 0; i < in->style.stroke_dashitems; i++)
		    out->style.stroke_dasharray[i] =
			in->style.stroke_dasharray[i];
	    }
	  out->style.stroke_dashoffset = in->style.stroke_dashoffset;
      }
    out->style.stroke_url = NULL;
    out->style.stroke_pointer = NULL;
    if (in->style.stroke_url != NULL)
	svg_add_stroke_gradient_url (&(out->style), in->style.stroke_url);
    out->style.stroke_red = in->style.stroke_red;
    out->style.stroke_green = in->style.stroke_green;
    out->style.stroke_blue = in->style.stroke_blue;
    out->style.stroke_opacity = in->style.stroke_opacity;
    out->style.clip_url = NULL;
    out->style.clip_pointer = NULL;
    if (in->style.clip_url != NULL)
	svg_add_clip_url (&(out->style), in->style.clip_url);
    if (use != NULL)
	out->parent = use->parent;
    else
	out->parent = in->parent;
    out->first = NULL;
    out->last = NULL;
    out->is_defs = 0;
    out->is_flow_root = 0;
    item = in->first;
    while (item)
      {
	  /* looping on Group Items */
	  itm = svg_clone_item (item);
	  svg_set_group_parent (itm, out);
	  if (out->first == NULL)
	      out->first = itm;
	  if (out->last != NULL)
	      out->last->next = itm;
	  out->last = itm;
	  item = item->next;
      }
    out->first_trans = NULL;
    out->last_trans = NULL;
    out->next = NULL;
    if (use != NULL)
      {
	  /* adding the Use-level defs */
	  pt = use->first_trans;
	  while (pt)
	    {
		/* clonig all Use transformations */
		trans = svg_clone_transform (pt);
		if (out->first_trans == NULL)
		    out->first_trans = trans;
		if (out->last_trans != NULL)
		    out->last_trans->next = trans;
		out->last_trans = trans;
		pt = pt->next;
	    }
      }
    pt = in->first_trans;
    while (pt)
      {
	  /* clonig all transformations */
	  trans = svg_clone_transform (pt);
	  if (out->first_trans == NULL)
	      out->first_trans = trans;
	  if (out->last_trans != NULL)
	      out->last_trans->next = trans;
	  out->last_trans = trans;
	  pt = pt->next;
      }
    if (use != NULL)
      {
	  /* adding the Use-level defs */
	  if (use->x != DBL_MAX || use->y != DBL_MAX)
	    {
		/* adding the implict Use Translate transformation */
		rl2PrivSvgTranslatePtr translate = NULL;
		if (use->x != DBL_MAX && use->y != DBL_MAX)
		    translate = svg_alloc_translate (use->x, use->y);
		else if (use->x != DBL_MAX)
		    translate = svg_alloc_translate (use->x, 0.0);
		else if (use->y != DBL_MAX)
		    translate = svg_alloc_translate (0.0, use->y);
		trans = svg_alloc_transform (RL2_SVG_TRANSLATE, translate);
		if (out->first_trans == NULL)
		    out->first_trans = trans;
		if (out->last_trans != NULL)
		    out->last_trans->next = trans;
		out->last_trans = trans;
	    }
	  /* Use-level styles */
	  if (use->style.visibility >= 0)
	      out->style.visibility = use->style.visibility;
	  out->style.opacity = use->style.opacity;
	  if (use->style.fill >= 0)
	      out->style.fill = use->style.fill;
	  if (use->style.no_fill >= 0)
	      out->style.no_fill = use->style.no_fill;
	  if (use->style.fill_rule >= 0)
	      out->style.fill_rule = use->style.fill_rule;
	  if (use->style.fill_url != NULL)
	      svg_add_fill_gradient_url (&(out->style), use->style.fill_url);
	  if (use->style.fill_red >= 0.0)
	      out->style.fill_red = use->style.fill_red;
	  if (use->style.fill_green >= 0.0)
	      out->style.fill_green = use->style.fill_green;
	  if (use->style.fill_blue >= 0.0)
	      out->style.fill_blue = use->style.fill_blue;
	  if (use->style.fill_opacity >= 0.0)
	      out->style.fill_opacity = use->style.fill_opacity;
	  if (use->style.stroke >= 0)
	      out->style.stroke = use->style.stroke;
	  if (use->style.no_stroke >= 0)
	      out->style.no_stroke = use->style.no_stroke;
	  if (use->style.stroke_width >= 0.0)
	      out->style.stroke_width = use->style.stroke_width;
	  if (use->style.stroke_linecap >= 0)
	      out->style.stroke_linecap = use->style.stroke_linecap;
	  if (use->style.stroke_linejoin >= 0)
	      out->style.stroke_linejoin = use->style.stroke_linejoin;
	  if (use->style.stroke_miterlimit >= 0.0)
	      out->style.stroke_miterlimit = use->style.stroke_miterlimit;
	  if (in->style.stroke_dashitems > 0)
	    {
		out->style.stroke_dashitems = use->style.stroke_dashitems;
		if (out->style.stroke_dasharray != NULL)
		    free (out->style.stroke_dasharray);
		out->style.stroke_dasharray = NULL;
		if (use->style.stroke_dashitems > 0)
		  {
		      int i;
		      out->style.stroke_dasharray =
			  malloc (sizeof (double) *
				  use->style.stroke_dashitems);
		      for (i = 0; i < use->style.stroke_dashitems; i++)
			  out->style.stroke_dasharray[i] =
			      use->style.stroke_dasharray[i];
		  }
		out->style.stroke_dashoffset = use->style.stroke_dashoffset;
	    }
	  if (use->style.stroke_url != NULL)
	      svg_add_stroke_gradient_url (&(out->style),
					   use->style.stroke_url);
	  if (use->style.stroke_red >= 0.0)
	      out->style.stroke_red = use->style.stroke_red;
	  if (use->style.stroke_green >= 0.0)
	      out->style.stroke_green = use->style.stroke_green;
	  if (use->style.stroke_blue >= 0.0)
	      out->style.stroke_blue = use->style.stroke_blue;
	  if (use->style.stroke_opacity >= 0.0)
	      out->style.stroke_opacity = use->style.stroke_opacity;
	  if (use->style.clip_url != NULL)
	      svg_add_clip_url (&(out->style), use->style.clip_url);
      }
    out->next = NULL;
    return out;
}

RL2_PRIVATE rl2PrivSvgClipPtr
svg_alloc_clip (void)
{
/* allocating and initializing an empty SVG ClipPath */
    rl2PrivSvgClipPtr p = malloc (sizeof (rl2PrivSvgClip));
    p->id = NULL;
    p->first = NULL;
    p->last = NULL;
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgClipPtr
svg_clone_clip (rl2PrivSvgClipPtr in)
{
/* cloning an SVG ClipPath */
    rl2PrivSvgItemPtr item;
    rl2PrivSvgItemPtr itm;
    rl2PrivSvgClipPtr out = malloc (sizeof (rl2PrivSvgClip));
    out->id = NULL;
    out->first = NULL;
    out->last = NULL;
    item = in->first;
    while (item)
      {
	  /* looping on Group Items */
	  itm = svg_clone_item (item);
	  if (out->first == NULL)
	      out->first = itm;
	  if (out->last != NULL)
	      out->last->next = itm;
	  out->last = itm;
	  item = item->next;
      }
    out->next = NULL;
    return out;
}

RL2_PRIVATE void
svg_add_group_id (rl2PrivSvgGroupPtr group, const char *id)
{
/* setting the ID for some Group */
    int len = strlen (id);
    if (group->id != NULL)
	free (group->id);
    group->id = malloc (len + 1);
    strcpy (group->id, id);
}

RL2_PRIVATE void
svg_add_clip_id (rl2PrivSvgClipPtr clip, const char *id)
{
/* setting the ID for some ClipPath */
    int len = strlen (id);
    if (clip->id != NULL)
	free (clip->id);
    clip->id = malloc (len + 1);
    strcpy (clip->id, id);
}

RL2_PRIVATE rl2PrivSvgItemPtr
svg_alloc_item (int type, void *pointer)
{
/* allocating and initializing an empty SVG item */
    rl2PrivSvgItemPtr p = malloc (sizeof (rl2PrivSvgItem));
    p->type = type;
    p->pointer = pointer;
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgItemPtr
svg_clone_item (rl2PrivSvgItemPtr in)
{
/* cloning an SVG item */
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgShapePtr shape;
    rl2PrivSvgUsePtr use;
    rl2PrivSvgClipPtr clip;
    rl2PrivSvgItemPtr p = malloc (sizeof (rl2PrivSvgItem));
    p->type = in->type;
    switch (in->type)
      {
      case RL2_SVG_ITEM_GROUP:
	  group = in->pointer;
	  p->pointer = svg_clone_group (group, NULL);
	  break;
      case RL2_SVG_ITEM_SHAPE:
	  shape = in->pointer;
	  p->pointer = svg_clone_shape (shape, NULL);
	  break;
      case RL2_SVG_ITEM_USE:
	  use = in->pointer;
	  p->pointer = svg_clone_use (use);
      case RL2_SVG_ITEM_CLIP:
	  clip = in->pointer;
	  p->pointer = svg_clone_clip (clip);
	  break;
      };
    p->next = NULL;
    return p;
}

RL2_PRIVATE void
svg_set_group_parent (rl2PrivSvgItemPtr item, rl2PrivSvgGroupPtr grp)
{
/* cloning an SVG item */
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgShapePtr shape;
    rl2PrivSvgUsePtr use;
    switch (item->type)
      {
      case RL2_SVG_ITEM_GROUP:
	  group = item->pointer;
	  group->parent = grp;
	  break;
      case RL2_SVG_ITEM_SHAPE:
	  shape = item->pointer;
	  shape->parent = grp;
	  break;
      case RL2_SVG_ITEM_USE:
	  use = item->pointer;
	  use->parent = grp;
	  break;
      };
}

RL2_PRIVATE rl2PrivSvgGradientStopPtr
svg_alloc_gradient_stop (double offset, double red, double green,
			 double blue, double opacity)
{
/* allocating and initializing an SVG GradientStop */
    rl2PrivSvgGradientStopPtr p = malloc (sizeof (rl2PrivSvgGradientStop));
    p->offset = offset;
    p->red = red;
    p->green = green;
    p->blue = blue;
    p->opacity = opacity;
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgGradientStopPtr
svg_clone_gradient_stop (rl2PrivSvgGradientStopPtr in)
{
/* cloning an SVG GradientStop */
    rl2PrivSvgGradientStopPtr out = malloc (sizeof (rl2PrivSvgGradientStop));
    out->offset = in->offset;
    out->red = in->red;
    out->green = in->green;
    out->blue = in->blue;
    out->opacity = in->opacity;
    out->next = NULL;
    return out;
}

RL2_PRIVATE rl2PrivSvgGradientPtr
svg_alloc_gradient (void)
{
/* allocating and initializing an empty SVG Gradient */
    rl2PrivSvgGradientPtr p = malloc (sizeof (rl2PrivSvgGradient));
    p->type = -1;
    p->id = NULL;
    p->xlink_href = NULL;
    p->gradient_units = -1;
    p->x1 = DBL_MAX;
    p->y1 = DBL_MAX;
    p->x2 = DBL_MAX;
    p->y2 = DBL_MAX;
    p->cx = DBL_MAX;
    p->cy = DBL_MAX;
    p->fx = DBL_MAX;
    p->fy = DBL_MAX;
    p->r = DBL_MAX;
    p->first_trans = NULL;
    p->last_trans = NULL;
    p->first_stop = NULL;
    p->last_stop = NULL;
    p->next = NULL;
    p->prev = NULL;
    return p;
}

RL2_PRIVATE rl2PrivSvgGradientPtr
svg_clone_gradient (rl2PrivSvgGradientPtr in, rl2PrivSvgGradientPtr old)
{
/* cloning an SVG Gradient */
    rl2PrivSvgTransformPtr pt;
    rl2PrivSvgTransformPtr trans;
    rl2PrivSvgGradientStopPtr ps;
    rl2PrivSvgGradientStopPtr stop;
    int len;
    rl2PrivSvgGradientPtr out = malloc (sizeof (rl2PrivSvgGradient));
    out->type = old->type;
    out->id = NULL;
    out->xlink_href = NULL;
    if (old->id != NULL)
      {
	  len = strlen (old->id);
	  out->id = malloc (len + 1);
	  strcpy (out->id, old->id);
      }
    if (old->xlink_href != NULL)
      {
	  len = strlen (old->xlink_href);
	  out->xlink_href = malloc (len + 1);
	  strcpy (out->xlink_href, old->xlink_href);
      }
    out->gradient_units = in->gradient_units;
    if (old->gradient_units >= 0)
	out->gradient_units = old->gradient_units;
    out->x1 = in->x1;
    if (old->x1 != DBL_MAX)
	out->x1 = old->x1;
    out->y1 = in->y1;
    if (old->y1 != DBL_MAX)
	out->y1 = old->y1;
    out->x2 = in->x2;
    if (old->x2 != DBL_MAX)
	out->x2 = old->x2;
    out->y2 = in->y2;
    if (old->y2 != DBL_MAX)
	out->y2 = old->y2;
    out->cx = in->cx;
    if (old->cx != DBL_MAX)
	out->cx = old->cx;
    out->cy = in->cy;
    if (old->cy != DBL_MAX)
	out->cy = old->cy;
    out->fx = in->fx;
    if (old->fx != DBL_MAX)
	out->fx = old->fx;
    out->fy = in->fy;
    if (old->fy != DBL_MAX)
	out->fy = old->fy;
    out->r = in->r;
    if (old->r != DBL_MAX)
	out->r = old->r;
    out->first_trans = NULL;
    out->last_trans = NULL;
    out->first_stop = NULL;
    out->last_stop = NULL;
    pt = in->first_trans;
    while (pt)
      {
	  /* clonig all inherited transformations */
	  trans = svg_clone_transform (pt);
	  if (out->first_trans == NULL)
	      out->first_trans = trans;
	  if (out->last_trans != NULL)
	      out->last_trans->next = trans;
	  out->last_trans = trans;
	  pt = pt->next;
      }
    pt = old->first_trans;
    while (pt)
      {
	  /* clonig all direct transformations */
	  trans = svg_clone_transform (pt);
	  if (out->first_trans == NULL)
	      out->first_trans = trans;
	  if (out->last_trans != NULL)
	      out->last_trans->next = trans;
	  out->last_trans = trans;
	  pt = pt->next;
      }
    ps = in->first_stop;
    while (ps)
      {
	  /* clonig all inherited Stops */
	  stop = svg_clone_gradient_stop (ps);
	  if (out->first_stop == NULL)
	      out->first_stop = stop;
	  if (out->last_stop != NULL)
	      out->last_stop->next = stop;
	  out->last_stop = stop;
	  ps = ps->next;
      }
    ps = old->first_stop;
    while (ps)
      {
	  /* clonig all inherited Stops */
	  stop = svg_clone_gradient_stop (ps);
	  if (out->first_stop == NULL)
	      out->first_stop = stop;
	  if (out->last_stop != NULL)
	      out->last_stop->next = stop;
	  out->last_stop = stop;
	  ps = ps->next;
      }
    out->next = NULL;
    out->prev = NULL;
    return out;
}

RL2_PRIVATE rl2PrivSvgDocumentPtr
svg_alloc_document (void)
{
/* allocating and initializing an empty SVG Document */
    rl2PrivSvgDocumentPtr p = malloc (sizeof (rl2PrivSvgDocument));
    p->width = 0.0;
    p->height = 0.0;
    p->viewbox_x = DBL_MIN;
    p->viewbox_y = DBL_MIN;
    p->viewbox_width = DBL_MIN;
    p->viewbox_height = DBL_MIN;
    p->first = NULL;
    p->last = NULL;
    p->first_grad = NULL;
    p->last_grad = NULL;
    p->current_group = NULL;
    p->current_shape = NULL;
    p->current_clip = NULL;
    p->defs_count = 0;
    p->flow_root_count = 0;
    return p;
}

RL2_PRIVATE void
svg_close_group (rl2PrivSvgDocumentPtr svg_doc)
{
/* closing the current SVG Group */
    rl2PrivSvgGroupPtr group = svg_doc->current_group;
    svg_doc->current_group = group->parent;
}

RL2_PRIVATE void
svg_insert_group (rl2PrivSvgDocumentPtr svg_doc)
{
/* inserting a new Group into the SVG Document */
    rl2PrivSvgItemPtr item;
    rl2PrivSvgGroupPtr parent;
    rl2PrivSvgGroupPtr group = svg_alloc_group ();
    if (svg_doc->current_group != NULL)
      {
	  /* nested group */
	  parent = svg_doc->current_group;
	  group->parent = parent;
	  if (svg_doc->defs_count > 0)
	      group->is_defs = 1;
	  if (svg_doc->flow_root_count > 0)
	      group->is_flow_root = 1;
	  item = svg_alloc_item (RL2_SVG_ITEM_GROUP, group);
	  if (parent->first == NULL)
	      parent->first = item;
	  if (parent->last != NULL)
	      parent->last->next = item;
	  parent->last = item;
	  svg_doc->current_group = group;
	  return;
      }
    if (svg_doc->current_clip != NULL)
      {
	  /* ClipPath group */
	  if (svg_doc->defs_count > 0)
	      group->is_defs = 1;
	  if (svg_doc->flow_root_count > 0)
	      group->is_flow_root = 1;
	  item = svg_alloc_item (RL2_SVG_ITEM_GROUP, group);
	  if (svg_doc->current_clip->first == NULL)
	      svg_doc->current_clip->first = item;
	  if (svg_doc->current_clip->last != NULL)
	      svg_doc->current_clip->last->next = item;
	  svg_doc->current_clip->last = item;
	  svg_doc->current_group = group;
	  return;
      }
/* first level Group */
    parent = svg_doc->current_group;
    group->parent = parent;
    if (svg_doc->defs_count > 0)
	group->is_defs = 1;
    if (svg_doc->flow_root_count > 0)
	group->is_flow_root = 1;
    item = svg_alloc_item (RL2_SVG_ITEM_GROUP, group);
    if (svg_doc->first == NULL)
	svg_doc->first = item;
    if (svg_doc->last != NULL)
	svg_doc->last->next = item;
    svg_doc->last = item;
    svg_doc->current_group = group;
}

RL2_PRIVATE void
svg_close_clip (rl2PrivSvgDocumentPtr svg_doc)
{
/* closing the current SVG ClipPath */
    svg_doc->current_clip = NULL;
}

RL2_PRIVATE void
svg_insert_clip (rl2PrivSvgDocumentPtr svg_doc)
{
/* inserting a new ClipPath into the SVG Document */
    rl2PrivSvgItemPtr item;
    rl2PrivSvgClipPtr clip = svg_alloc_clip ();
    item = svg_alloc_item (RL2_SVG_ITEM_CLIP, clip);
    if (svg_doc->first == NULL)
	svg_doc->first = item;
    if (svg_doc->last != NULL)
	svg_doc->last->next = item;
    svg_doc->last = item;
    svg_doc->current_clip = clip;
}

RL2_PRIVATE rl2PrivSvgUsePtr
svg_insert_use (rl2PrivSvgDocumentPtr svg_doc, const char *xlink_href,
		double x, double y, double width, double height)
{
/* inserting a new Use (xlink:href) into the SVG Document */
    rl2PrivSvgUsePtr use;
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgClipPtr clip;
    rl2PrivSvgItemPtr item;

    if (svg_doc->current_group == NULL && svg_doc->current_clip == NULL)
      {
	  /* inserting directly into the Document */
	  use = svg_alloc_use (NULL, xlink_href, x, y, width, height);
	  item = svg_alloc_item (RL2_SVG_ITEM_USE, use);
	  if (svg_doc->first == NULL)
	      svg_doc->first = item;
	  if (svg_doc->last != NULL)
	      svg_doc->last->next = item;
	  svg_doc->last = item;
      }
    else if (svg_doc->current_clip != NULL)
      {
	  /* inserting into the current ClipPath */
	  clip = svg_doc->current_clip;
	  use = svg_alloc_use (clip, xlink_href, x, y, width, height);
	  item = svg_alloc_item (RL2_SVG_ITEM_USE, use);
	  if (clip->first == NULL)
	      clip->first = item;
	  if (clip->last != NULL)
	      clip->last->next = item;
	  clip->last = item;
      }
    else if (svg_doc->current_group != NULL)
      {
	  /* inserting into the current Group */
	  group = svg_doc->current_group;
	  use = svg_alloc_use (group, xlink_href, x, y, width, height);
	  item = svg_alloc_item (RL2_SVG_ITEM_USE, use);
	  if (group->first == NULL)
	      group->first = item;
	  if (group->last != NULL)
	      group->last->next = item;
	  group->last = item;
      }
    return use;
}

RL2_PRIVATE void
svg_insert_shape (rl2PrivSvgDocumentPtr svg_doc, int type, void *data)
{
/* inserting a new Shape into the SVG Document */
    rl2PrivSvgGroupPtr group;
    rl2PrivSvgShapePtr shape;
    rl2PrivSvgClipPtr clip;
    rl2PrivSvgItemPtr item;

    if (svg_doc->current_group == NULL && svg_doc->current_clip == NULL)
      {
	  /* inserting directly into the Document */
	  shape = svg_alloc_shape (type, data, NULL);
	  if (svg_doc->defs_count > 0)
	      shape->is_defs = 1;
	  if (svg_doc->flow_root_count > 0)
	      shape->is_flow_root = 1;
	  item = svg_alloc_item (RL2_SVG_ITEM_SHAPE, shape);
	  if (svg_doc->first == NULL)
	      svg_doc->first = item;
	  if (svg_doc->last != NULL)
	      svg_doc->last->next = item;
	  svg_doc->last = item;
      }
    else if (svg_doc->current_group != NULL)
      {
	  /* inserting into the current Group */
	  group = svg_doc->current_group;
	  shape = svg_alloc_shape (type, data, group);
	  if (svg_doc->defs_count > 0)
	      shape->is_defs = 1;
	  if (svg_doc->flow_root_count > 0)
	      shape->is_flow_root = 1;
	  item = svg_alloc_item (RL2_SVG_ITEM_SHAPE, shape);
	  if (group->first == NULL)
	      group->first = item;
	  if (group->last != NULL)
	      group->last->next = item;
	  group->last = item;
      }
    else if (svg_doc->current_clip != NULL)
      {
	  /* inserting into the current ClipPath */
	  clip = svg_doc->current_clip;
	  shape = svg_alloc_shape (type, data, NULL);
	  if (svg_doc->defs_count > 0)
	      shape->is_defs = 1;
	  if (svg_doc->flow_root_count > 0)
	      shape->is_flow_root = 1;
	  item = svg_alloc_item (RL2_SVG_ITEM_SHAPE, shape);
	  if (clip->first == NULL)
	      clip->first = item;
	  if (clip->last != NULL)
	      clip->last->next = item;
	  clip->last = item;
      }
    svg_doc->current_shape = shape;
}

RL2_PRIVATE void
svg_insert_gradient_stop (rl2PrivSvgGradientPtr gradient,
			  double offset, double red, double green,
			  double blue, double opacity)
{
/* inserting a Stop into a Gradient */
    rl2PrivSvgGradientStopPtr stop =
	svg_alloc_gradient_stop (offset, red, green, blue, opacity);
    if (gradient->first_stop == NULL)
	gradient->first_stop = stop;
    if (gradient->last_stop != NULL)
	gradient->last_stop->next = stop;
    gradient->last_stop = stop;
}

RL2_PRIVATE rl2PrivSvgGradientPtr
svg_insert_linear_gradient (rl2PrivSvgDocumentPtr svg_doc,
			    const char *id, const char *xlink_href,
			    double x1, double y1, double x2, double y2,
			    int units)
{
/* inserting a new Linear Gradient into the SVG Document */
    int len;
    rl2PrivSvgGradientPtr gradient = svg_alloc_gradient ();
    gradient->type = RL2_SVG_LINEAR_GRADIENT;
    gradient->id = NULL;
    if (id != NULL)
      {
	  len = strlen (id);
	  gradient->id = malloc (len + 1);
	  strcpy (gradient->id, id);
      }
    gradient->xlink_href = NULL;
    if (xlink_href != NULL)
      {
	  len = strlen (xlink_href);
	  gradient->xlink_href = malloc (len + 1);
	  strcpy (gradient->xlink_href, xlink_href);
      }
    gradient->gradient_units = units;
    gradient->x1 = x1;
    gradient->y1 = y1;
    gradient->x2 = x2;
    gradient->y2 = y2;
    gradient->prev = svg_doc->last_grad;
    if (svg_doc->first_grad == NULL)
	svg_doc->first_grad = gradient;
    if (svg_doc->last_grad != NULL)
	svg_doc->last_grad->next = gradient;
    svg_doc->last_grad = gradient;
    return gradient;
}

RL2_PRIVATE rl2PrivSvgGradientPtr
svg_insert_radial_gradient (rl2PrivSvgDocumentPtr svg_doc,
			    const char *id, const char *xlink_href,
			    double cx, double cy, double fx, double fy,
			    double r, int units)
{
/* inserting a new Linear Gradient into the SVG Document */
    int len;
    rl2PrivSvgGradientPtr gradient = svg_alloc_gradient ();
    gradient->type = RL2_SVG_RADIAL_GRADIENT;
    gradient->id = NULL;
    if (id != NULL)
      {
	  len = strlen (id);
	  gradient->id = malloc (len + 1);
	  strcpy (gradient->id, id);
      }
    gradient->xlink_href = NULL;
    if (xlink_href != NULL)
      {
	  len = strlen (xlink_href);
	  gradient->xlink_href = malloc (len + 1);
	  strcpy (gradient->xlink_href, xlink_href);
      }
    gradient->gradient_units = units;
    gradient->cx = cx;
    gradient->cy = cy;
    gradient->fx = fx;
    gradient->fy = fy;
    gradient->r = r;
    gradient->prev = svg_doc->last_grad;
    if (svg_doc->first_grad == NULL)
	svg_doc->first_grad = gradient;
    if (svg_doc->last_grad != NULL)
	svg_doc->last_grad->next = gradient;
    svg_doc->last_grad = gradient;
    return gradient;
}
