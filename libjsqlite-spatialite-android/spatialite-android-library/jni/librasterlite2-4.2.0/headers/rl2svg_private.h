/*

 rl2svg_private -- hidden SVG internals 

 version 0.1, 2013 March 29

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

/**
 \file rl2svg_private.h

 RasterLite2 private SVG header file
 */

#include "config.h"

#ifndef _RL2SVG_PRIVATE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RL2SVG_PRIVATE_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* SVG constants */
#define RL2_SVG_UNKNOWN		0

/* SVG basic shapes */
#define RL2_SVG_RECT		1
#define	RL2_SVG_CIRCLE		2
#define RL2_SVG_ELLIPSE		3
#define RL2_SVG_LINE		4
#define RL2_SVG_POLYLINE	5
#define RL2_SVG_POLYGON		6
#define RL2_SVG_PATH		7

/* SVG transformations */
#define RL2_SVG_MATRIX		8
#define RL2_SVG_TRANSLATE	9
#define RL2_SVG_SCALE		10
#define RL2_SVG_ROTATE		11
#define RL2_SVG_SKEW_X		12
#define RL2_SVG_SKEW_Y		13

/* SVG Path actions */
#define RL2_SVG_MOVE_TO		14
#define RL2_SVG_LINE_TO		15
#define RL2_SVG_CURVE_3		16
#define RL2_SVG_CURVE_4		17
#define RL2_SVG_ELLIPT_ARC	18
#define RL2_SVG_CLOSE_PATH	19

/* SGV Items */
#define RL2_SVG_ITEM_GROUP	20
#define RL2_SVG_ITEM_SHAPE	21
#define RL2_SVG_ITEM_USE	22
#define RL2_SVG_ITEM_CLIP	23

/* SVG Gradients */
#define RL2_SVG_LINEAR_GRADIENT	24
#define RL2_SVG_RADIAL_GRADIENT	25
#define RL2_SVG_USER_SPACE	26
#define RL2_SVG_BOUNDING_BOX	27

    typedef struct svg_matrix
    {
	/* SVG Matrix data */
	double a;
	double b;
	double c;
	double d;
	double e;
	double f;
    } rl2PrivSvgMatrix;
    typedef rl2PrivSvgMatrix *rl2PrivSvgMatrixPtr;

    typedef struct svg_translate
    {
	/* SVG Translate data */
	double tx;
	double ty;
    } rl2PrivSvgTranslate;
    typedef rl2PrivSvgTranslate *rl2PrivSvgTranslatePtr;

    typedef struct svg_scale
    {
	/* SVG Scale data */
	double sx;
	double sy;
    } rl2PrivSvgScale;
    typedef rl2PrivSvgScale *rl2PrivSvgScalePtr;

    typedef struct svg_rotate
    {
	/* SVG Rotate data */
	double angle;
	double cx;
	double cy;
    } rl2PrivSvgRotate;
    typedef rl2PrivSvgRotate *rl2PrivSvgRotatePtr;

    typedef struct svg_skew
    {
	/* SVG Skew data */
	double angle;
    } rl2PrivSvgSkew;
    typedef rl2PrivSvgSkew *rl2PrivSvgSkewPtr;

    typedef struct svg_transform
    {
	/* SVG Transform (linked list) */
	int type;
	void *data;
	struct svg_transform *next;
    } rl2PrivSvgTransform;
    typedef rl2PrivSvgTransform *rl2PrivSvgTransformPtr;

    typedef struct svg_gradient_stop
    {
	/* SVG Gradient Stop */
	char *id;
	double offset;
	double red;
	double green;
	double blue;
	double opacity;
	struct svg_gradient_stop *next;
    } rl2PrivSvgGradientStop;
    typedef rl2PrivSvgGradientStop *rl2PrivSvgGradientStopPtr;

    typedef struct svg_gradient
    {
	/* SVG Gradient */
	int type;
	char *id;
	char *xlink_href;
	int gradient_units;
	double x1;
	double y1;
	double x2;
	double y2;
	double cx;
	double cy;
	double fx;
	double fy;
	double r;
	rl2PrivSvgGradientStopPtr first_stop;
	rl2PrivSvgGradientStopPtr last_stop;
	rl2PrivSvgTransformPtr first_trans;
	rl2PrivSvgTransformPtr last_trans;
	struct svg_gradient *prev;
	struct svg_gradient *next;
    } rl2PrivSvgGradient;
    typedef rl2PrivSvgGradient *rl2PrivSvgGradientPtr;

    typedef struct svg_rect
    {
	/* SVG Rect data */
	double x;
	double y;
	double width;
	double height;
	double rx;
	double ry;
    } rl2PrivSvgRect;
    typedef rl2PrivSvgRect *rl2PrivSvgRectPtr;

    typedef struct svg_circle
    {
	/* SVG Circle data */
	double cx;
	double cy;
	double r;
    } rl2PrivSvgCircle;
    typedef rl2PrivSvgCircle *rl2PrivSvgCirclePtr;

    typedef struct svg_ellipse
    {
	/* SVG Ellipse data */
	double cx;
	double cy;
	double rx;
	double ry;
    } rl2PrivSvgEllipse;
    typedef rl2PrivSvgEllipse *rl2PrivSvgEllipsePtr;

    typedef struct svg_line
    {
	/* SVG Line data */
	double x1;
	double y1;
	double x2;
	double y2;
    } rl2PrivSvgLine;
    typedef rl2PrivSvgLine *rl2PrivSvgLinePtr;

    typedef struct svg_polyline
    {
	/* SVG Polyline data */
	int points;
	double *x;
	double *y;
    } rl2PrivSvgPolyline;
    typedef rl2PrivSvgPolyline *rl2PrivSvgPolylinePtr;

    typedef struct svg_polygon
    {
	/* SVG Polygon data */
	int points;
	double *x;
	double *y;
    } rl2PrivSvgPolygon;
    typedef rl2PrivSvgPolygon *rl2PrivSvgPolygonPtr;

    typedef struct svg_path_move
    {
	/* SVG Path: MoveTo and LineTo data */
	double x;
	double y;
    } rl2PrivSvgPathMove;
    typedef rl2PrivSvgPathMove *rl2PrivSvgPathMovePtr;

    typedef struct svg_path_bezier
    {
	/* SVG Path: any Bezier Curve */
	double x1;
	double y1;
	double x2;
	double y2;
	double x;
	double y;
    } rl2PrivSvgPathBezier;
    typedef rl2PrivSvgPathBezier *rl2PrivSvgPathBezierPtr;

    typedef struct svg_path_ellipt_arc
    {
	/* SVG Path: Elliptical Arc */
	double rx;
	double ry;
	double rotation;
	int large_arc;
	int sweep;
	double x;
	double y;
    } rl2PrivSvgPathEllipticArc;
    typedef rl2PrivSvgPathEllipticArc *rl2PrivSvgPathEllipticArcPtr;

    typedef struct svg_path_item
    {
	/* SVG Path item (linked list) */
	int type;
	void *data;
	struct svg_path_item *next;
    } rl2PrivSvgPathItem;
    typedef rl2PrivSvgPathItem *rl2PrivSvgPathItemPtr;

    typedef struct svg_path
    {
	/* SVG Path */
	struct svg_path_item *first;
	struct svg_path_item *last;
	int error;
    } rl2PrivSvgPath;
    typedef rl2PrivSvgPath *rl2PrivSvgPathPtr;

    typedef struct svg_style
    {
	/* SVG Style-related definitions */
	char visibility;
	double opacity;
	char fill;
	char no_fill;
	int fill_rule;
	char *fill_url;
	rl2PrivSvgGradientPtr fill_pointer;
	double fill_red;
	double fill_green;
	double fill_blue;
	double fill_opacity;
	char stroke;
	char no_stroke;
	double stroke_width;
	int stroke_linecap;
	int stroke_linejoin;
	double stroke_miterlimit;
	int stroke_dashitems;
	double *stroke_dasharray;
	double stroke_dashoffset;
	char *stroke_url;
	rl2PrivSvgGradientPtr stroke_pointer;
	double stroke_red;
	double stroke_green;
	double stroke_blue;
	double stroke_opacity;
	char *clip_url;
	struct svg_item *clip_pointer;
    } rl2PrivSvgStyle;
    typedef rl2PrivSvgStyle *rl2PrivSvgStylePtr;

    typedef struct svg_shape
    {
	/* generic SVG shape container */
	char *id;
	int type;
	void *data;
	struct svg_group *parent;
	rl2PrivSvgStyle style;
	rl2PrivSvgTransformPtr first_trans;
	rl2PrivSvgTransformPtr last_trans;
	int is_defs;
	int is_flow_root;
	struct svg_shape *next;
    } rl2PrivSvgShape;
    typedef rl2PrivSvgShape *rl2PrivSvgShapePtr;

    typedef struct svg_use
    {
	/* SVG Use (xlink:href) */
	char *xlink_href;
	double x;
	double y;
	double width;
	double height;
	rl2PrivSvgStyle style;
	struct svg_group *parent;
	rl2PrivSvgTransformPtr first_trans;
	rl2PrivSvgTransformPtr last_trans;
	struct svg_use *next;
    } rl2PrivSvgUse;
    typedef rl2PrivSvgUse *rl2PrivSvgUsePtr;

    typedef struct svg_item
    {
	/* SVG generic item */
	int type;
	void *pointer;
	struct svg_item *next;
    } rl2PrivSvgItem;
    typedef rl2PrivSvgItem *rl2PrivSvgItemPtr;

    typedef struct svg_group
    {
	/* SVG group container: <g> */
	char *id;
	rl2PrivSvgStyle style;
	struct svg_group *parent;
	rl2PrivSvgItemPtr first;
	rl2PrivSvgItemPtr last;
	rl2PrivSvgTransformPtr first_trans;
	rl2PrivSvgTransformPtr last_trans;
	int is_defs;
	int is_flow_root;
	struct svg_group *next;
    } rl2PrivSvgGroup;
    typedef rl2PrivSvgGroup *rl2PrivSvgGroupPtr;

    typedef struct svg_clip
    {
	/* SVG group container: <clipPath> */
	char *id;
	struct svg_item *first;
	struct svg_item *last;
	struct svg_clip *next;
    } rl2PrivSvgClip;
    typedef rl2PrivSvgClip *rl2PrivSvgClipPtr;


    typedef struct svg_document
    {
	/* SVG document main container */
	cairo_matrix_t matrix;
	double width;
	double height;
	double viewbox_x;
	double viewbox_y;
	double viewbox_width;
	double viewbox_height;
	rl2PrivSvgItemPtr first;
	rl2PrivSvgItemPtr last;
	rl2PrivSvgGradientPtr first_grad;
	rl2PrivSvgGradientPtr last_grad;
	rl2PrivSvgGroupPtr current_group;
	rl2PrivSvgShapePtr current_shape;
	rl2PrivSvgClipPtr current_clip;
	int defs_count;
	int flow_root_count;
    } rl2PrivSvgDocument;
    typedef rl2PrivSvgDocument *rl2PrivSvgDocumentPtr;

    RL2_PRIVATE rl2PrivSvgDocumentPtr svg_parse_doc (const unsigned char *svg,
						     int svg_len);

    RL2_PRIVATE void svg_free_transform (rl2PrivSvgTransformPtr p);

    RL2_PRIVATE void svg_free_polyline (rl2PrivSvgPolylinePtr p);

    RL2_PRIVATE void svg_free_polyline (rl2PrivSvgPolylinePtr p);

    RL2_PRIVATE void svg_free_path_item (rl2PrivSvgPathItemPtr p);

    RL2_PRIVATE void svg_free_path (rl2PrivSvgPathPtr p);

    RL2_PRIVATE void svg_free_shape (rl2PrivSvgShapePtr p);

    RL2_PRIVATE void svg_free_use (rl2PrivSvgUsePtr p);

    RL2_PRIVATE void svg_free_item (rl2PrivSvgItemPtr p);

    RL2_PRIVATE void svg_free_clip (rl2PrivSvgClipPtr p);

    RL2_PRIVATE void svg_free_group (rl2PrivSvgGroupPtr p);

    RL2_PRIVATE void svg_free_gradient_stop (rl2PrivSvgGradientStopPtr p);

    RL2_PRIVATE void svg_free_gradient (rl2PrivSvgGradient * p);

    RL2_PRIVATE void svg_free_document (rl2PrivSvgDocument * p);

    RL2_PRIVATE rl2PrivSvgMatrixPtr svg_alloc_matrix (double a, double b,
						      double c, double d,
						      double e, double f);

    RL2_PRIVATE rl2PrivSvgMatrixPtr svg_clone_matrix (rl2PrivSvgMatrixPtr in);

    RL2_PRIVATE rl2PrivSvgTranslatePtr svg_alloc_translate (double tx,
							    double ty);

    RL2_PRIVATE rl2PrivSvgTranslatePtr
	svg_clone_translate (rl2PrivSvgTranslatePtr in);

    RL2_PRIVATE rl2PrivSvgScalePtr svg_alloc_scale (double sx, double sy);

    RL2_PRIVATE rl2PrivSvgScalePtr svg_clone_scale (rl2PrivSvgScalePtr in);

    RL2_PRIVATE rl2PrivSvgRotatePtr svg_alloc_rotate (double angle, double cx,
						      double cy);

    RL2_PRIVATE rl2PrivSvgRotatePtr svg_clone_rotate (rl2PrivSvgRotatePtr in);

    RL2_PRIVATE rl2PrivSvgSkewPtr svg_alloc_skew (double angle);

    RL2_PRIVATE rl2PrivSvgSkewPtr svg_clone_skew (rl2PrivSvgSkewPtr in);

    RL2_PRIVATE rl2PrivSvgTransformPtr svg_alloc_transform (int type,
							    void *data);

    RL2_PRIVATE rl2PrivSvgTransformPtr
	svg_clone_transform (rl2PrivSvgTransformPtr in);

    RL2_PRIVATE rl2PrivSvgRectPtr svg_alloc_rect (double x, double y,
						  double width, double height,
						  double rx, double ry);

    RL2_PRIVATE rl2PrivSvgRectPtr svg_clone_rect (rl2PrivSvgRectPtr in);

    RL2_PRIVATE rl2PrivSvgCirclePtr svg_alloc_circle (double cx, double cy,
						      double r);

    RL2_PRIVATE rl2PrivSvgCirclePtr svg_clone_circle (rl2PrivSvgCirclePtr in);

    RL2_PRIVATE rl2PrivSvgEllipsePtr svg_alloc_ellipse (double cx, double cy,
							double rx, double ry);

    RL2_PRIVATE rl2PrivSvgEllipsePtr svg_clone_ellipse (rl2PrivSvgEllipsePtr
							in);

    RL2_PRIVATE rl2PrivSvgLinePtr svg_alloc_line (double x1, double y1,
						  double x2, double y2);

    RL2_PRIVATE rl2PrivSvgLinePtr svg_clone_line (rl2PrivSvgLinePtr in);

    RL2_PRIVATE rl2PrivSvgPolylinePtr svg_alloc_polyline (int points, double *x,
							  double *y);

    RL2_PRIVATE rl2PrivSvgPolylinePtr svg_clone_polyline (rl2PrivSvgPolylinePtr
							  in);

    RL2_PRIVATE rl2PrivSvgPolygonPtr svg_alloc_polygon (int points, double *x,
							double *y);

    RL2_PRIVATE rl2PrivSvgPolygonPtr svg_clone_polygon (rl2PrivSvgPolygonPtr
							in);

    RL2_PRIVATE rl2PrivSvgPathMovePtr svg_alloc_path_move (double x, double y);

    RL2_PRIVATE rl2PrivSvgPathMovePtr svg_clone_path_move (rl2PrivSvgPathMovePtr
							   in);

    RL2_PRIVATE rl2PrivSvgPathBezierPtr svg_alloc_path_bezier (double x1,
							       double y1,
							       double x2,
							       double y2,
							       double x,
							       double y);

    RL2_PRIVATE rl2PrivSvgPathBezierPtr
	svg_clone_path_bezier (rl2PrivSvgPathBezierPtr in);

    RL2_PRIVATE rl2PrivSvgPathEllipticArcPtr svg_alloc_path_ellipt_arc (double
									rx,
									double
									ry,
									double
									rotation,
									int
									large_arc,
									int
									sweep,
									double
									x,
									double
									y);

    RL2_PRIVATE rl2PrivSvgPathEllipticArcPtr
	svg_clone_path_ellipt_arc (rl2PrivSvgPathEllipticArcPtr in);

    RL2_PRIVATE rl2PrivSvgPathItemPtr svg_alloc_path_item (int type,
							   void *data);

    RL2_PRIVATE void svg_add_path_item (rl2PrivSvgPathPtr path, int type,
					void *data);

    RL2_PRIVATE rl2PrivSvgPathPtr svg_alloc_path (void);

    RL2_PRIVATE rl2PrivSvgPathPtr svg_clone_path (rl2PrivSvgPathPtr in);

    RL2_PRIVATE void svg_add_clip_url (rl2PrivSvgStylePtr style,
				       const char *url);

    RL2_PRIVATE void svg_add_fill_gradient_url (rl2PrivSvgStylePtr style,
						const char *url);

    RL2_PRIVATE void svg_add_stroke_gradient_url (rl2PrivSvgStylePtr style,
						  const char *url);

    RL2_PRIVATE rl2PrivSvgShapePtr svg_alloc_shape (int type, void *data,
						    rl2PrivSvgGroupPtr parent);

    RL2_PRIVATE rl2PrivSvgShapePtr svg_clone_shape (rl2PrivSvgShapePtr in,
						    rl2PrivSvgUsePtr use);

    RL2_PRIVATE void svg_add_shape_id (rl2PrivSvgShapePtr shape,
				       const char *id);

    RL2_PRIVATE rl2PrivSvgUsePtr svg_alloc_use (void *parent,
						const char *xlink_href,
						double x, double y,
						double width, double height);

    RL2_PRIVATE rl2PrivSvgUsePtr svg_clone_use (rl2PrivSvgUsePtr in);

    RL2_PRIVATE rl2PrivSvgGroupPtr svg_alloc_group (void);

    RL2_PRIVATE rl2PrivSvgGroupPtr svg_clone_group (rl2PrivSvgGroupPtr in,
						    rl2PrivSvgUsePtr use);

    RL2_PRIVATE rl2PrivSvgClipPtr svg_alloc_clip (void);

    RL2_PRIVATE rl2PrivSvgClipPtr svg_clone_clip (rl2PrivSvgClipPtr in);

    RL2_PRIVATE void svg_add_group_id (rl2PrivSvgGroupPtr group,
				       const char *id);

    RL2_PRIVATE void svg_add_clip_id (rl2PrivSvgClipPtr clip, const char *id);

    RL2_PRIVATE rl2PrivSvgItemPtr svg_alloc_item (int type, void *pointer);

    RL2_PRIVATE rl2PrivSvgItemPtr svg_clone_item (rl2PrivSvgItemPtr in);

    RL2_PRIVATE void svg_set_group_parent (rl2PrivSvgItemPtr item,
					   rl2PrivSvgGroupPtr grp);

    RL2_PRIVATE rl2PrivSvgGradientStopPtr svg_alloc_gradient_stop (double
								   offset,
								   double red,
								   double green,
								   double blue,
								   double
								   opacity);

    RL2_PRIVATE rl2PrivSvgGradientStopPtr
	svg_clone_gradient_stop (rl2PrivSvgGradientStopPtr in);

    RL2_PRIVATE rl2PrivSvgGradientPtr svg_alloc_gradient (void);

    RL2_PRIVATE rl2PrivSvgGradientPtr svg_clone_gradient (rl2PrivSvgGradientPtr
							  in,
							  rl2PrivSvgGradientPtr
							  old);

    RL2_PRIVATE rl2PrivSvgDocumentPtr svg_alloc_document (void);

    RL2_PRIVATE void svg_close_group (rl2PrivSvgDocumentPtr svg_doc);

    RL2_PRIVATE void svg_insert_group (rl2PrivSvgDocumentPtr svg_doc);

    RL2_PRIVATE void svg_close_clip (rl2PrivSvgDocumentPtr svg_doc);

    RL2_PRIVATE void svg_insert_clip (rl2PrivSvgDocumentPtr svg_doc);

    RL2_PRIVATE rl2PrivSvgUsePtr svg_insert_use (rl2PrivSvgDocumentPtr svg_doc,
						 const char *xlink_href,
						 double x, double y,
						 double width, double height);

    RL2_PRIVATE void svg_insert_shape (rl2PrivSvgDocumentPtr svg_doc, int type,
				       void *data);

    RL2_PRIVATE void svg_insert_gradient_stop (rl2PrivSvgGradientPtr gradient,
					       double offset, double red,
					       double green, double blue,
					       double opacity);

    RL2_PRIVATE rl2PrivSvgGradientPtr
	svg_insert_linear_gradient (rl2PrivSvgDocumentPtr svg_doc,
				    const char *id, const char *xlink_href,
				    double x1, double y1, double x2, double y2,
				    int units);

    RL2_PRIVATE rl2PrivSvgGradientPtr
	svg_insert_radial_gradient (rl2PrivSvgDocumentPtr svg_doc,
				    const char *id, const char *xlink_href,
				    double cx, double cy, double fx, double fy,
				    double r, int units);

    RL2_PRIVATE rl2PrivSvgGradientPtr
	svg_insert_radial_gradient (rl2PrivSvgDocumentPtr svg_doc,
				    const char *id, const char *xlink_href,
				    double cx, double cy, double fx, double fy,
				    double r, int units);

    RL2_PRIVATE void svg_init_style (rl2PrivSvgStylePtr style);

    RL2_PRIVATE void svg_style_cleanup (rl2PrivSvgStylePtr style);


#ifdef __cplusplus
}
#endif

#endif				/* _RL2SVG_PRIVATE_H */
