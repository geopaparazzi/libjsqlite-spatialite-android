/* 
 rl2graphics.h -- RasterLite2 common graphics support
  
 version 2.0, 2013 September 29

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
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

Contributor(s):


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
 \file rl2graphics.h

 Graphics (Cairo) support
 */

#ifndef _RL2GRAPHICS_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RL2GRAPHICS_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define RL2_FONTSTYLE_NORMAL	5101
#define RL2_FONTSTYLE_ITALIC	5102
#define RL2_FONTSTYLE_OBLIQUE	5103

#define RL2_FONTWEIGHT_NORMAL	5201
#define RL2_FONTWEIGHT_BOLD	5202

#define RL2_CLEAR_PATH		5100
#define RL2_PRESERVE_PATH	5101

#define RL2_PEN_CAP_BUTT	5210
#define RL2_PEN_CAP_ROUND	5211
#define RL2_PEN_CAP_SQUARE	5212

#define RL2_PEN_JOIN_MITER	5261
#define RL2_PEN_JOIN_ROUND	5262
#define RL2_PEN_JOIN_BEVEL	5263

#define RL2_UNKNOWN_CANVAS	0x03a
#define RL2_VECTOR_CANVAS	0x03b
#define RL2_TOPOLOGY_CANVAS	0x03c
#define RL2_NETWORK_CANVAS	0x03d
#define RL2_RASTER_CANVAS	0x03e
#define RL2_WMS_CANVAS		0x03f

#define RL2_CANVAS_UNKNOWN_CTX		5500
#define RL2_CANVAS_BASE_CTX			5501
#define RL2_CANVAS_LABELS_CTX		5502
#define RL2_CANVAS_NODES_CTX		5503
#define RL2_CANVAS_EDGES_CTX		5504
#define RL2_CANVAS_LINKS_CTX		5505
#define RL2_CANVAS_FACES_CTX		5506
#define RL2_CANVAS_EDGE_SEEDS_CTX	5507
#define RL2_CANVAS_LINK_SEEDS_CTX	5508
#define RL2_CANVAS_FACE_SEEDS_CTX	5509

    typedef struct rl2_graphics_context rl2GraphicsContext;
    typedef rl2GraphicsContext *rl2GraphicsContextPtr;

    typedef struct rl2_graphics_pattern rl2GraphicsPattern;
    typedef rl2GraphicsPattern *rl2GraphicsPatternPtr;

    typedef struct rl2_graphics_font rl2GraphicsFont;
    typedef rl2GraphicsFont *rl2GraphicsFontPtr;

    typedef struct rl2_graphics_bitmap rl2GraphicsBitmap;
    typedef rl2GraphicsBitmap *rl2GraphicsBitmapPtr;

    typedef struct rl2_affine_transform_data rl2AffineTransformData;
    typedef rl2AffineTransformData *rl2AffineTransformDataPtr;

/**
 Creates a generic Graphics Context 

 \param width canvas width (in pixels)
 \param height canvas height (in pixels)

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_destroy_context, rl2_graph_create_svg_context,
 rl2_graph_create_pdf_context, rl2_graph_create_mem_pdf_context,
 rl2_graph_context_get_dimensions, rl2_graph_create_context_rgba,
 rl2_prime_background
 
 \note you are responsible to destroy (before or after) any Graphics Context
 returned by rl2_graph_create_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr rl2_graph_create_context (int width,
								int height);

/**
 Creates a generic Graphics Context initialized from an RGBA image

 \param width canvas width (in pixels)
 \param height canvas height (in pixels)
 \param rgbaArray pointer to an array of RGBA pixels representing the bitmap

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_destroy_context, rl2_graph_create_svg_context,
 rl2_graph_create_pdf_context, rl2_graph_create_mem_pdf_context,
 rl2_graph_context_get_dimensions, rl2_graph_create_context
 
 \note you are responsible to destroy (before or after) any Graphics Context
 returned by rl2_graph_create_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr rl2_graph_create_context_rgba (int width,
								     int height,
								     unsigned
								     char
								     *rgbaArray);

/**
 Destroys a Graphics Context object freeing any allocated resource 

 \param handle the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context()
 
 \sa rl2_graph_create_context
 */
    RL2_DECLARE void rl2_graph_destroy_context (rl2GraphicsContextPtr context);

/**
 Creates an SVG Graphics Context 

 \param path pathname of the target SVG output file
 \param width canvas width (in pixels)
 \param height canvas height (in pixels)

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_create_context, rl2_graph_destroy_context, rl2_graph_create_pdf_context,
 rl2_graph_create_mem_pdf_context, rl2_graph_context_get_dimensions
 
 \note you are responsible to destroy (before or after) any SVG Graphics Context
 returned by rl2_graph_create_svg_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr rl2_graph_create_svg_context (const char
								    *path,
								    int width,
								    int height);

/**
 Creates a PDF Graphics Context 

 \param path pathname of the target PDF output file
 \param dpi the PDF printing resolution measured in DPI 
 \param page_width total page width (in inches) including any margin
 \param page_height total page height (in inches) including any margin
 \param margin_width horizontal margin width (in inches)
 \param margin_height vertical margin height (in inches)

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, rl2_graph_create_mem_pdf_context,
 rl2_graph_destroy_context, rl2_graph_context_get_dimensions
 
 \note you are responsible to destroy (before or after) any PDF Graphics Context
 returned by rl2_graph_create_pdf_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr rl2_graph_create_pdf_context (const char
								    *path,
								    int dpi,
								    double
								    page_width,
								    double
								    page_height,
								    double
								    margin_width,
								    double
								    margin_height);

/**
 Creates an in-memory PDF Graphics Context 

 \param mem handle to an in-memory PDF target create by rl2_create_mem_pdf_target()
 \param dpi the PDF printing resolution measured in DPI 
 \param page_width total page width (in inches) including any margin
 \param page_height total page height (in inches) including any margin
 \param margin_width horizontal margin width (in inches)
 \param margin_height vertical margin height (in inches)

 \return the pointer to the corresponding Graphics Context object: NULL on failure
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, rl2_graph_create_pdf_context,
 rl2_graph_destroy_context, rl2_create_mem_pdf_target, rl2_graph_context_get_dimensions
 
 \note you are responsible to destroy (before or after) any PDF Graphics Context
 returned by rl2_graph_create_mem_pdf_context() by invoking rl2_graph_destroy_context().
 */
    RL2_DECLARE rl2GraphicsContextPtr
	rl2_graph_create_mem_pdf_context (rl2MemPdfPtr pdf, int dpi,
					  double page_width, double page_height,
					  double margin_width,
					  double margin_height);

/**
 Retrieves the Width and Height from a Graphics Context
  
 \param handle the pointer to a valid Graphics Context.
 \param width on succesful completione the variable referenced by this
 pointer will contain the Width value.
 \param height on succesful completione the variable referenced by this
 pointer will contain the Height value.
 
 \return RL2_OK on succes; RL2_ERROR on failure
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, rl2_graph_create_mem_pdf_context
 */
    RL2_DECLARE int
	rl2_graph_context_get_dimensions (rl2GraphicsContextPtr handle,
					  int *width, int *height);

/**
 Initialiting the background

 \param handle the pointer to a valid Graphics Context.
 \param red Red component of the background colour. 
 \param green Green component of the background colour. 
 \param blue Blue component of the background colour. 
 \param alpha Alpha component (transparency) of the background colour. 
 
 \sa rl2_graph_create_context
 */
    RL2_DECLARE void
	rl2_prime_background (void *pctx, unsigned char red,
			      unsigned char green, unsigned char blue,
			      unsigned char alpha);

/**
 Create an in-memory PDF target
 
 \return the handle to an initially empty in-memory PDF target: NULL on failure
 
 \sa rl2_graph_create_mem_pdf_context, rl2_destroy_mem_pdf_target,
 rl2_get_mem_pdf_buffer, rl2_graph_context_get_dimensions
 
 \note you are responsible to destroy (before or after) any in-memory PDF target
 returned by rl2_create_mem_pdf_target() by invoking rl2_destroy_mem_pdf_target().
 */
    RL2_DECLARE rl2MemPdfPtr rl2_create_mem_pdf_target (void);

/**
 Destroys an in-memory PDF target freeing any allocated resource 

 \param handle the pointer to a valid in-memory PDF target returned by a previous call
 to rl2_create_mem_pdf_target()
 
 \sa rl2_create_mem_pdf_target, rl2_destroy_mem_pdf_target
 */
    RL2_DECLARE void rl2_destroy_mem_pdf_target (rl2MemPdfPtr target);

/**
 Transfers the ownership of the memory buffer contained into a in-memory PDF target

 \param handle the pointer to a valid in-memory PDF target returned by a previous call
 to rl2_create_mem_pdf_target()
 \param buffer on completion this variable will point to the a memory block 
  containing the PDF document (may be NULL if any error occurred).
 \param size on completion this variable will contain the size (in bytes)
 of the memory block containing the PDF document.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including referencing an empty in-memory PDF target).
 
 \sa rl2_create_mem_pdf_target
 
 \note after invoking rl2_get_mem_pdf_buffer() the in-memory PDF target
 will be reset to its initial empty state-
 */
    RL2_DECLARE int rl2_get_mem_pdf_buffer (rl2MemPdfPtr target,
					    unsigned char **buffer, int *size);

/**
 Selects the currently set Pen for a Graphics Context (solid style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param red Pen color: red component.
 \param green Pen color: green component.
 \param blue Pen color: blue component.
 \param alpha Pen transparency (0 full transparent; 255 full opaque).
 \param width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_solid_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen, 
 rl2_graph_set_brush, rl2_graph_set_font
 */
    RL2_DECLARE int rl2_graph_set_solid_pen (rl2GraphicsContextPtr context,
					     unsigned char red,
					     unsigned char green,
					     unsigned char blue,
					     unsigned char alpha, double width,
					     int line_cap, int line_join);

/**
 Selects the currently set Pen for a Graphics Context (dashed style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param red Pen color: red component.
 \param green Pen color: green component.
 \param blue Pen color: blue component.
 \param alpha Pen transparency (0 full transparent; 255 full opaque).
 \param width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL
 \param dash_count the total count of dash_list items
 \param dash_list an array of dash items 
 \param dash_offset start offset

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen,
 rl2_graph_set_linear_gradient_solid_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen, 
 rl2_graph_set_brush, rl2_graph_set_font
 */
    RL2_DECLARE int rl2_graph_set_dashed_pen (rl2GraphicsContextPtr context,
					      unsigned char red,
					      unsigned char green,
					      unsigned char blue,
					      unsigned char alpha, double width,
					      int line_cap, int line_join,
					      int dash_count,
					      double dash_list[],
					      double dash_offset);

/**
 Selects the currently set Linear Gradient Pen for a Graphics Context (solid style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param x start point (X coord, in pixels) of the Gradient.
 \param y start point (Y coord, in pixels) of the Gradient.
 \param width the Gradient width.
 \param height the Gradient height.
 \param red1 first Gradient color: red component.
 \param green1 first Gradient color: green component.
 \param blue1 first Gradient color: blue component.
 \param alpha1 first Gradient color transparency (0 full transparent; 255 full opaque).
 \param red2 second Gradient color: red component.
 \param green2 second Gradient color: green component.
 \param blue2 second Gradient color: blue component.
 \param alpha2 second Gradient color transparency (0 full transparent; 255 full opaque).
 \param width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_brush, 
 rl2_graph_set_dashed_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen, 
 rl2_graph_set_brush, rl2_graph_set_font

 \note a Pen created by this function always is a Linear Gradient Pen.
 */
    RL2_DECLARE int
	rl2_graph_set_linear_gradient_solid_pen (rl2GraphicsContextPtr,
						 double x, double y,
						 double width, double height,
						 unsigned char red1,
						 unsigned char green1,
						 unsigned char blue1,
						 unsigned char alpha1,
						 unsigned char red2,
						 unsigned char green2,
						 unsigned char blue2,
						 unsigned char alpha2,
						 double pen_width, int line_cap,
						 int line_join);

/**
 Selects the currently set Linear Gradient Pen for a Graphics Context (dashed style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param x start point (X coord, in pixels) of the Gradient.
 \param y start point (Y coord, in pixels) of the Gradient.
 \param width the Gradient width.
 \param height the Gradient height.
 \param red1 first Gradient color: red component.
 \param green1 first Gradient color: green component.
 \param blue1 first Gradient color: blue component.
 \param alpha1 first Gradient color transparency (0 full transparent; 255 full opaque).
 \param red2 second Gradient color: red component.
 \param green2 second Gradient color: green component.
 \param blue2 second Gradient color: blue component.
 \param alpha2 second Gradient color transparency (0 full transparent; 255 full opaque).
 \param width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL
 \param dash_count the total count of dash_list items
 \param dash_list an array of dash items 
 \param dash_offset start offset

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, 
 rl2_graph_set_dashed_pen, rl2_graph_set_linear_gradient_solid_pen, 
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen, 
 rl2_graph_set_brush, rl2_graph_set_font

 \note a Pen created by this function always is a Linear Gradient Pen.
 */
    RL2_DECLARE int
	rl2_graph_set_linear_gradient_dashed_pen (rl2GraphicsContextPtr,
						  double x, double y,
						  double width, double height,
						  unsigned char red1,
						  unsigned char green1,
						  unsigned char blue1,
						  unsigned char alpha1,
						  unsigned char red2,
						  unsigned char green2,
						  unsigned char blue2,
						  unsigned char alpha2,
						  double pen_width,
						  int line_cap, int line_join,
						  int dash_count,
						  double dash_list[],
						  double offset);

/**
 Selects the currently set Pattern Pen for a Graphics Context (solid style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param pattern the pointer to a valid Graphics Pattern returned by a previous
 call to rl2_graph_create_pattern().
 \param pen_width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_solid_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_dashed_pen, rl2_graph_set_brush, rl2_graph_set_font, 
 rl2_create_pattern, rl2_graph_realease_pattern_pen

 \note a Pen created by this function always is a Pattern Pen, 
 i.e. a Pen repeatedly using a small bitmap as a filling source.
 */
    RL2_DECLARE int rl2_graph_set_pattern_solid_pen (rl2GraphicsContextPtr
						     context,
						     rl2GraphicsPatternPtr
						     pattern, double width,
						     int line_cap,
						     int line_join);

/**
 Selects the currently set Pattern Pen for a Graphics Context (dashed style)

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param pattern the pointer to a valid Graphics Pattern returned by a previous
 call to rl2_graph_create_pattern().
 \param pen_width Pen width
 \param line_cap one of RL2_PEN_CAP_BUTT, RL2_PEN_CAP_ROUND or RL2_PEN_CAP_SQUARE
 \param line_join one of RL2_PEN_JOIN_MITER, RL2_PEN_JOIN_MITER or RL2_PEN_JOIN_BEVEL
 \param dash_count the total count of dash_list items
 \param dash_list an array of dash items 
 \param dash_offset start offset

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_solid_pen, rl2_graph_set_linear_gradient_dashed_pen,
 rl2_graph_set_pattern_solid_pen, rl2_graph_set_brush, rl2_graph_set_font, 
 rl2_create_pattern, rl2_graph_realease_pattern_pen

 \note a Pen created by this function always is a Pattern Pen, 
 i.e. a Pen repeatedly using a small bitmap as a filling source.
 */
    RL2_DECLARE int rl2_graph_set_pattern_dashed_pen (rl2GraphicsContextPtr
						      context,
						      rl2GraphicsPatternPtr
						      pattern, double width,
						      int line_cap,
						      int line_join,
						      int dash_count,
						      double dash_list[],
						      double offset);

/**
 Releases the currently set Pattern Pen (if any) from a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_pattern_solid_pen, rl2_graph_set_pattern_dashed_pen

 \note you should always release a Pattern Pen before attempting to
 destroy the corresponding Pattern object.
 */
    RL2_DECLARE int rl2_graph_release_pattern_pen (rl2GraphicsContextPtr
						   context);

/**
 Selects the currently set Brush for a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param red Brush color: red component.
 \param green Brush color: green component.
 \param blue Brush color: blue component.
 \param alpha Brush transparency (0 full transparent; 255 full opaque).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rrl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_pen, rl2_graph_set_pattern_pen,
 rl2_graph_set_linear_gradient_brush, rl2_graph_set_pattern_brush,
 rl2_graph_set_font

 \note a Brush created by this function always is a solid Brush, may be
 supporting transparency.
 */
    RL2_DECLARE int rl2_graph_set_brush (rl2GraphicsContextPtr context,
					 unsigned char red, unsigned char green,
					 unsigned char blue,
					 unsigned char alpha);

/**
 Selects the currently set Brush for a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param x start point (X coord, in pixels) of the Gradient.
 \param y start point (Y coord, in pixels) of the Gradient.
 \param width the Gradient width.
 \param height the Gradient height.
 \param red1 first Gradient color: red component.
 \param green1 first Gradient color: green component.
 \param blue1 first Gradient color: blue component.
 \param alpha1 first Gradient color transparency (0 full transparent; 255 full opaque).
 \param red2 second Gradient color: red component.
 \param green2 second Gradient color: green component.
 \param blue2 second Gradient color: blue component.
 \param alpha2 second Gradient color transparency (0 full transparent; 255 full opaque).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_linear_gradient_pen, rl2_graph_set_pattern_pen,
 rl2_graph_set_brush, rl2_graph_set_pattern_brush,
 rl2_graph_set_font

 \note a Brush created by this function always is a Linear Gradient Brush.
 */
    RL2_DECLARE int rl2_graph_set_linear_gradient_brush (rl2GraphicsContextPtr,
							 double x, double y,
							 double width,
							 double height,
							 unsigned char red1,
							 unsigned char green1,
							 unsigned char blue1,
							 unsigned char alpha1,
							 unsigned char red2,
							 unsigned char green2,
							 unsigned char blue2,
							 unsigned char alpha2);

/**
 Selects the currently set Brush for a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param pattern the pointer to a valid Graphics Pattern returned by a previous
 call to rl2_graph_create_pattern().

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_brush, rl2_graph_set_linear_gradient_brush, 
 rl2_graph_set_font, rl2_create_pattern, rl2_graph_release_pattern_brush

 \note a Brush created by this function always is a Pattern Brush, 
 i.e. a Brush repeatedly using a small bitmap as a filling source.
 */
    RL2_DECLARE int rl2_graph_set_pattern_brush (rl2GraphicsContextPtr context,
						 rl2GraphicsPatternPtr pattern);

/**
 Releases the currently set Pattern Brush (if any) from a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_pattern_brush

 \note you should always release a Pattern Brush before attempting to
 destroy the corresponding Pattern object.
 */
    RL2_DECLARE int rl2_graph_release_pattern_brush (rl2GraphicsContextPtr
						     context);

/**
 Selects the currently set Font for a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()
 \param font the pointer to a valid Graphics Font returned by a previous call
 to rl2_create_font().

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_context, rl2_graph_create_svg_context, 
 rl2_graph_create_pdf_context, rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen,
 rl2_graph_set_brush, rl2_graph_set_linear_gradient_brush, rl2_graph_set_pattern_brush,
 rl2_create_font
 */
    RL2_DECLARE int rl2_graph_set_font (rl2GraphicsContextPtr context,
					rl2GraphicsFontPtr font);

/**
 Releases the currently set Font out from a Graphics Context

 \param context the pointer to a valid Graphics Context returned by a previous call
 to rl2_graph_create_context(), rl2_graph_create_svg_context() or
 rl2_graph_create_pdf_context()

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_font, rl2_graph_destroy_font
 
 \note you must always call rl2_graph_release_font() before attempting
 to destroy a currently selected TrueType Font.
 */
    RL2_DECLARE int rl2_graph_release_font (rl2GraphicsContextPtr context);

/**
 Creates a Pattern Brush 

 \param rgbaArray pointer to an array of RGBA pixels representing the bitmap
 supporting the Pattern Brush to be created.
 \param width Pattern width (in pixels)
 \param height Pattern height (in pixels)
 \param extend if TRUE the pattern will be infinitely repeated
  so to completely fill the plane 

 \return the pointer to the corresponding Pattern Brush object: NULL on failure
 
 \sa rl2_graph_set_pattern_brush, rl2_graph_destroy_brush,
 rl2_create_pattern_from_external_graphic, rl2_create_pattern_from_external_svg
 
 \note you are responsible to destroy (before or after) any Pattern Brush
 returned by rl2_graph_create_pattern() by invoking rl2_graph_destroy_pattern().
 */
    RL2_DECLARE rl2GraphicsPatternPtr rl2_graph_create_pattern (unsigned char
								*rgbaArray,
								int width,
								int height,
								int extend);

/**
 Creates a Pattern from an External Graphic resource (PNG, GIF, JPEG)

 \param handle SQLite3 connection handle
 \param xlink_href unique External Graphic identifier
 \param extend if TRUE the pattern will be infinitely repeated
  so to completely fill the plane 

 \return the pointer to the corresponding Pattern Brush object: NULL on failure
 
 \sa rl2_graph_set_pattern_brush, rl2_graph_destroy_brush,
 rl2_graph_create_pattern, rl2_create_pattern_from_external_svg,
 rl2_graph_pattern_get_size
 
 \note you are responsible to destroy (before or after) any Pattern Brush
 returned by rl2_create_pattern_from_external_graphic() by invoking 
 rl2_graph_destroy_pattern().
 */
    RL2_DECLARE rl2GraphicsPatternPtr
	rl2_create_pattern_from_external_graphic (sqlite3 * handle,
						  const char *xlink_href,
						  int extend);

/**
 Creates a Pattern from an External Graphic resource (SVG)

 \param handle SQLite3 connection handle
 \param xlink_href unique External Graphic identifier
 \param size the scaled size of the SVG symbol

 \return the pointer to the corresponding Pattern Brush object: NULL on failure
 
 \sa rl2_graph_set_pattern_brush, rl2_graph_destroy_brush,
 rl2_graph_create_pattern, rl2_create_pattern_from_external_graphic,
 rl2_graph_pattern_get_size
 
 \note you are responsible to destroy (before or after) any Pattern Brush
 returned by rl2_create_pattern_from_external_graphic() by invoking 
 rl2_graph_destroy_pattern().
 */
    RL2_DECLARE rl2GraphicsPatternPtr
	rl2_create_pattern_from_external_svg (sqlite3 * handle,
					      const char *xlink_href,
					      double size);

/**
 Destroys a Pattern Brush object freeing any allocated resource 

 \param pattern pointer to a valid Pattern object
 
 \sa rl2_graph_create_pattern, rl2_create_pattern_from_external_graphic

 \note you should always release a Pattern Pen or Pattern Brush from any
 referencing Graphic Context before attempting to destroy the corresponding
 Pattern object.
 */
    RL2_DECLARE void rl2_graph_destroy_pattern (rl2GraphicsPatternPtr pattern);

/**
 Retrieving the Dimensions from a Pattern Brush object

 \param pattern pointer to a valid Pattern object
 \param width on completion the variable referenced by this
 pointer will contain the Pattern's Width (in pixels).
 \param height on completion the variable referenced by this
 pointer will contain the Pattern's Height (in pixels).
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_graph_create_pattern, rl2_create_pattern_from_external_graphic
 */
    RL2_DECLARE int rl2_graph_get_pattern_size (rl2GraphicsPatternPtr pattern,
						unsigned int *width,
						unsigned int *height);

/**
 Recolors a Monochrome Pattern object  

 \param handle the pointer to a valid Pattern returned by a previous call
 to rl2_graph_create_pattern() or rl2_create_pattern_from_external_graphic()
 \param r the new Red component value to be set
 \param g the new Green component value
 \param b the new Blue component value
 
 \return RL2_OK on success: RL2_ERROR on failure. 
 
 \sa rl2_graph_create_pattern, rl2_create_pattern_from_external_graphic,
 rl2_graph_destroy_pattern, rl2_graph_pattern_transparency,
 rl2_graph_draw_graphic_symbol

 \note only a Monochrome Pattern can be succesfully recolored.
 Completely transparent pixels will be ignored; all opaque pixels
 should declare exactly the same color.
 */
    RL2_DECLARE int rl2_graph_pattern_recolor (rl2GraphicsPatternPtr pattern,
					       unsigned char r, unsigned char g,
					       unsigned char b);

/**
 Sets half-transparency for a Pattern object  

 \param handle the pointer to a valid Pattern returned by a previous call
 to rl2_graph_create_pattern() or rl2_create_pattern_from_external_graphic()
 \param alpha transparency (0 full transparent; 255 full opaque).
 
 \return RL2_OK on success: RL2_ERROR on failure. 
 
 \sa rl2_graph_create_pattern, rl2_create_pattern_from_external_graphic,
 rl2_graph_destroy_pattern, rl2_graph_pattern_recolor, 
 rl2_graph_draw_graphic_symbol

 \note Completely transparent pixels will be always preserved as such; 
 all other pixels will assume the required transparency.
 */
    RL2_DECLARE int rl2_graph_pattern_transparency (rl2GraphicsPatternPtr
						    pattern,
						    unsigned char alpha);

/**
 Draws a Graphic Symbol into the Canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param pattern the pointer to a valid  Pattern returned by a previous call
 \param width of the rendered image.
 \param height of the rendered image.
 \param x the X coordinate of the Anchor Point.
 \param y the Y coordinate of the Anchor Point.
 \param angle an angle (in decimal degrees) to rotate the image.
 \param angle an angle (in decimal degrees) to rotate the image.
 \param anchor_point_x relative X position of the Anchor Point
  expressed as a percent position within the image bounding box
  (expected to be a value between 0.0 and 1.0)
 \param anchor_point_y relative Y position of the Anchor Point
  expressed as a percent position within the image bounding box
  (expected to be a value between 0.0 and 1.0)

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_pattern, rl2_create_pattern_from_external_graphic,
 rl2_graph_destroy_pattern, rl2_graph_pattern_transparency, 
 rl2_graph_pattern_recolor, rl2_graph_draw_mark_symbol
 */
    RL2_DECLARE int rl2_graph_draw_graphic_symbol (rl2GraphicsContextPtr
						   context,
						   rl2GraphicsPatternPtr
						   pattern, double width,
						   double height, double x,
						   double y, double angle,
						   double anchor_point_x,
						   double anchor_point_y);

/**
 Draws a Mark Symbol into the Canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param mark_type expected to be one between RL2_GRAPHIC_MARK_SQUARE,
  RL2_GRAPHIC_MARK_CIRCLE, RL2_GRAPHIC_MARK_TRIANGLE, 
  RL2_GRAPHIC_MARK_STAR, RL2_GRAPHIC_MARK_CROSS and RL2_GRAPHIC_MARK_X 
  (default: RL2_GRAPHIC_MARK_SQUARE).
 \param size the Mark Symbol size (in pixels).
 \param x the X coordinate of the Anchor Point.
 \param y the Y coordinate of the Anchor Point.
 \param angle an angle (in decimal degrees) to rotate the image.
 \param angle an angle (in decimal degrees) to rotate the image.
 \param anchor_point_x relative X position of the Anchor Point
  expressed as a percent position within the image bounding box
  (expected to be a value between 0.0 and 1.0)
 \param anchor_point_y relative Y position of the Anchor Point
  expressed as a percent position within the image bounding box
  (expected to be a value between 0.0 and 1.0)
 \param fill if TRUE the Mark Symboll will be filled using the
  currently set brush
 \param stroke if TRUE the Mark Symboll will be stroked using
  the currently set pen

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_draw_graphic_symbol
 */
    RL2_DECLARE int rl2_graph_draw_mark_symbol (rl2GraphicsContextPtr
						context, int mark_type,
						double size,
						double x, double y,
						double angle,
						double anchor_point_x,
						double anchor_point_y, int fill,
						int stroke);

/**
 Creates a Graphics Font Object (CAIRO built-in "toy" fonts)

 \param facename the font Facename. Expected to be one of "serif", 
 "sans-serif" or "monospace". a NULL facename will default to "monospace".
 \param size the Graphics Font size (in points).
 \param style one of RL2_FONTSTYLE_NORMAL, RL2_FONTSTYLE_ITALIC or
 RL2_FONTSTYLE_OBLIQUE
 \param weight one of RL2_FONTWEIGHT_NORMAL or RL2_FONTWEIGHT_BOLD

 \return the pointer to the corresponding Font object: NULL on failure
 
 \sa rl2_graph_set_font, 
 rl2_graph_destroy_font, rl2_graph_font_set_color,
 rl2_graph_font_set_color, rl2_graph_font_set_halo
 
 \note you are responsible to destroy (before or after) any Pattern Brush
 returned by rl2_graph_create_toy_font() by invoking rl2_graph_destroy_font().
 */
    RL2_DECLARE rl2GraphicsFontPtr rl2_graph_create_toy_font (const char
							      *facename,
							      double size,
							      int style,
							      int weight);

/**
 Will retrieve a BLOB-encoded TrueType Font by its FaceName
 
 \param handle SQLite3 connection handle
 \param fontname name of the required font: e.g. "Roboto-BoldItalic"
 \param font on completion this variable will point to the a memory block 
  containing the BLOB-encoded TrueType font (may be NULL if any error occurred).
 \param font_sz on completion this variable will contain the size (in bytes)
 of the memory block containing the BLOB-encoded TrueType font.
 
 \return RL2_OK on success; RL2_ERROR if any error is encountered (this
 including requesting a not existing TrueType font).
 
 \note you are responsible to free (before or after) the BLOB-encode
 font returned by rl2_get_TrueType_font()
 */

    RL2_DECLARE int rl2_get_TrueType_font (sqlite3 * handle,
					   const char *facename,
					   unsigned char **font, int *font_sz);

/**
 Creates a Graphics Font Object (TrueType fonts)

 \param priv_data pointer to the opaque internal connection object 
 returned by a previous call to rl2_alloc_private() 
 \param ttf the TrueType font definition (internal BLOB format).
 \param ttf_bytes length (in bytes) of the above TTF definition
 \param size the Graphics Font size (in points).

 \return the pointer to the corresponding Font object: NULL on failure
 
 \sa rl2_graph_create_toy_font, rl2_get_TrueType_font, 
 rl2_search_TrueType_font, rl2_graph_set_font, rl2_graph_destroy_font, 
 rl2_graph_font_set_color, rl2_graph_font_set_color, 
 rl2_graph_font_set_halo
 
 \note you are responsible to destroy (before or after) any TrueType
 font returned by rl2_graph_create_TrueType_font() by invoking 
 rl2_graph_destroy_font().
 */
    RL2_DECLARE rl2GraphicsFontPtr rl2_graph_create_TrueType_font (const void
								   *priv_data,
								   const
								   unsigned char
								   *ttf,
								   int
								   ttf_bytes,
								   double size);

/**
 Searches and creates a Graphics Font Object (TrueType fonts)

 \param handle SQLite3 connection handle
 \param priv_data pointer to the opaque internal connection object 
 returned by a previous call to rl2_alloc_private() 
 \param fontname name of the required font: e.g. "Roboto-BoldItalic"
 \param size the Graphics Font size (in points).

 \return the pointer to the corresponding Font object: NULL on failure
 
 \sa rl2_graph_create_toy_font, rl2_get_TrueType_font, 
 rl2_graph_create_TrueType_font, rl2_graph_set_font, 
 rl2_graph_destroy_font, rl2_graph_font_set_color,
 rl2_graph_font_set_color, rl2_graph_font_set_halo
 
 \note you are responsible to destroy (before or after) any TrueType
 font returned by rl2_search_TrueType_font() by invoking 
 rl2_graph_destroy_font().
 */
    RL2_DECLARE rl2GraphicsFontPtr rl2_search_TrueType_font (sqlite3 * handle,
							     const void
							     *priv_data,
							     const char
							     *facename,
							     double size);

/**
 Destroys a Graphics Font object freeing any allocated resource 

 \param handle the pointer to a valid Font returned by a previous call
 to rl2_graph_create_toy_font() or rl2_graph_create_TrueType_font()
 
 \sa rl2_graph_create_toy_font, rl2_graph_create_TrueType_font
 
 \note you must always call rl2_graph_release_font() before attempting
 to destroy the currently selected font if it is of the TrueType class.
 */
    RL2_DECLARE void rl2_graph_destroy_font (rl2GraphicsFontPtr font);

/**
 Selects the currently set Color for a Graphics Font

 \param font the pointer to a valid Graphics Font returned by a previous call
 to rl2_graph_create_toy_font() or rl2_graph_create_TrueType_font()
 \param red Font color: red component.
 \param green Font color: green component.
 \param blue Font color: blue component.
 \param alpha Font transparency (0 full transparent; 255 full opaque).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_toy_font, rl2_graph_create_TrueType_font,
 rl2_graph_font_set_halo, rl2_graph_set_font
 */
    RL2_DECLARE int rl2_graph_font_set_color (rl2GraphicsFontPtr font,
					      unsigned char red,
					      unsigned char green,
					      unsigned char blue,
					      unsigned char alpha);

/**
 Selects the currently set Halo for a Graphics Font

 \param font the pointer to a valid Graphics Font returned by a previous call
 to rl2_graph_create_toy_font() or rl2_graph_create_TrueType_font()
 \param radius the Halo Radius (in points); declaring a zero or negative value
 will remove the Halo from the Font.
 \param red Halo color: red component.
 \param green Halo color: green component.
 \param blue Halo color: blue component.
 \param alpha Halo transparency (0 full transparent; 255 full opaque).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_toy_font, rl2_graph_create_TrueType_font, 
 rl2_graph_font_set_color, rl2_graph_set_font
 */
    RL2_DECLARE int rl2_graph_font_set_halo (rl2GraphicsFontPtr font,
					     double radius,
					     unsigned char red,
					     unsigned char green,
					     unsigned char blue,
					     unsigned char alpha);

/**
 Creates a Graphics Bitmap 

 \param rgbaArray pointer to an array of RGBA pixels representing the bitmap.
 \param width Bitmap width (in pixels)
 \param height Bitmap height (in pixels)

 \return the pointer to the corresponding Graphics Bitmap object: NULL on failure
 
 \sa rl2_graph_destroy_bitmap
 
 \note you are responsible to destroy (before or after) any Graphics Bitmap
 returned by rl2_graph_create_bitmap() by invoking rl2_graph_destroy_bitmap().
 */
    RL2_DECLARE rl2GraphicsBitmapPtr rl2_graph_create_bitmap (unsigned char
							      *rgbaArray,
							      int width,
							      int height);

/**
 Destroys a Graphics Bitmap object freeing any allocated resource 

 \param handle the pointer to a valid Font returned by a previous call
 to rl2_graph_create_bitmap()
 
 \sa rl2_graph_create_bitmap
 */
    RL2_DECLARE void rl2_graph_destroy_bitmap (rl2GraphicsBitmapPtr bitmap);

/**
 Drawing a Rectangle into a canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param x the X coordinate of the top left corner of the rectangle.
 \param y the Y coordinate of the top left corner of the rectangle.
 \param width the width of the rectangle.
 \param height the height of the rectangle.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush,
 rl2_graph, draw_rectangle, rl2_graph_draw_ellipse, rl2_graph_draw_circle_sector, 
 rl2_graph_stroke_line, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the rectangle will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_draw_rectangle (rl2GraphicsContextPtr context,
					      double x, double y, double width,
					      double height);

/**
 Drawing a Rectangle with rounded corners into a canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param x the X coordinate of the top left corner of the rectangle.
 \param y the Y coordinate of the top left corner of the rectangle.
 \param width the width of the rectangle.
 \param height the height of the rectangle.
 \param radius used for rounded corners.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush,
 rl2_draw_rectangle, rl2_graph_draw_ellipse, rl2_graph_draw_circle_sector, 
 rl2_graph_stroke_line, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the rectangle will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_draw_rounded_rectangle (rl2GraphicsContextPtr
						      context, double x,
						      double y, double width,
						      double height,
						      double radius);

/**
 Drawing an Ellipse into a canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param x the X coordinate of the top left corner of the rectangle
 into which the ellipse is inscribed.
 \param y the Y coordinate of the top left corner of the rectangle.
 \param width the width of the rectangle.
 \param height the height of the rectangle.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush, 
 rl2_draw_rectangle, rl2_graph_draw_rounded_rectangle, rl2_graph_draw_circle_sector, 
 rl2_graph_stroke_line, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the ellipse will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_draw_ellipse (rl2GraphicsContextPtr context,
					    double x, double y, double width,
					    double height);

/**
 Drawing a Circular Sector into a canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param center_x the X coordinate of the circle's centre.
 \param center_y the Y coordinate of the circle's centre.
 \param radius the circle's radius.
 \param from_angle the angle (in degrees) at wich the sector starts.
 \param to_angle the angle (in degrees) at wich the sector ends.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush, 
 rl2_draw_rectangle, rl2_graph_draw_rounded_rectangle, rl2_graph_draw_ellipse, 
 rl2_graph_stroke_line, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the Sector will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_draw_circle_sector (rl2GraphicsContextPtr context,
						  double center_x,
						  double center_y,
						  double radius,
						  double from_angle,
						  double to_angle);

/**
 Drawing a straight Line into a canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param x0 the X coordinate of the first point.
 \param y0 the Y coordinate of the first point.
 \param x1 the X coordinate of the second point.
 \param y1 the Y coordinate of the second point.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_set_brush, 
 rl2_draw_rectangle, rl2_graph_draw_rounded_rectangle, rl2_graph_draw_ellipse, 
 rl2_graph_draw_circular_sector, rl2_graph_fill_path, rl2_graph_stroke_path

 \note the Sector will be stroked using the current Pen and will
 be filled using the current Brush.
 */
    RL2_DECLARE int rl2_graph_stroke_line (rl2GraphicsContextPtr context,
					   double x0, double y0, double x1,
					   double y1);

/**
 Begins a new SubPath

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param x the X coordinate of the point.
 \param y the Y coordinate of the point.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_add_line_to_path,
 rl2_graph_close_subpath, rl2_graph_fill_path, rl2_graph_stroke_path
 */
    RL2_DECLARE int rl2_graph_move_to_point (rl2GraphicsContextPtr context,
					     double x, double y);

/**
 Add a line into the current SubPath

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param x the X coordinate of the destination point.
 \param y the Y coordinate of the destination point.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_brush, rl2_graph_move_to_point,
 rl2_graph_close_subpath, rl2_graph_fill_path, rl2_graph_stroke_path
 */
    RL2_DECLARE int rl2_graph_add_line_to_path (rl2GraphicsContextPtr context,
						double x, double y);

/**
 Terminates the current SubPath

 \param context the pointer to a valid Graphics Context (aka Canvas).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_brush, rl2_graph_move_to_point,
 rl2_graph_add_line_to_path, rl2_graph_fill_path, rl2_graph_stroke_path
 */
    RL2_DECLARE int rl2_graph_close_subpath (rl2GraphicsContextPtr context);

/**
 Fills the current Path using the currently set Brush

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param preserve if true the current Path will be preserved into the
 current Canvas, otherwise it will be definitely removed.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_brush, rl2_graph_move_to_point,
 rl2_graph_add_line_to_path, rl2_graph_close_subpath, rl2_graph_stroke_path
 */
    RL2_DECLARE int rl2_graph_fill_path (rl2GraphicsContextPtr context,
					 int preserve);

/**
 Strokes the current Path using the currently set Pen

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param preserve if true the current Path will be preserved into the
 current Canvas, otherwise it will be definitely removed.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_solid_pen, rl2_graph_set_dashed_pen, rl2_graph_move_to_point,
 rl2_graph_add_line_to_path, rl2_graph_close_subpath, rl2_graph_fill_path
 */
    RL2_DECLARE int rl2_graph_stroke_path (rl2GraphicsContextPtr context,
					   int preserve);

/**
 Draws a text into the Canvas using the currently set Font

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param text string to be printed into the canvas.
 \param x the X coordinate of the top left corner of the text.
 \param y the Y coordinate of the top left corner of the text.
 \param angle an angle (in decimal degrees) to rotate the text.
 \param anchor_point_x relative X position of the Anchor Point
  expressed as a percent position within the text bounding box
  (expected to be a value between 0.0 and 1.0)
 \param anchor_point_y relative Y position of the Anchor Point
  expressed as a percent position within the text bounding box
  (expected to be a value between 0.0 and 1.0)

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_font, rl2_graph_get_text_extent,
 rl2_graph_draw_warped_text
 */
    RL2_DECLARE int rl2_graph_draw_text (rl2GraphicsContextPtr context,
					 const char *text, double x, double y,
					 double angle,
					 double anchor_point_x,
					 double anchor_point_y);

/**
 Draws a text warped along a curve using the currently set Font

 \param handle connection handle
 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param text string to be printed into the canvas.
 \param points number of item in both X and Y arrays
 \param x array of X coordinates.
 \param y array of Y coordinates; both X and Y arrays represent
  the curve modelling the text to be warped. 
 \param initial_gap white space to be preserved before printing
 the first label occurrence.
 \param gap distance separating two cosecutive label occurrences.
 (both initial_gap and gap are only meaningfull in the repeated case).
 \param repeated if TRUE the text (aka label) will be repeated
  as many times as allowed by the modelling curve lenght.
  If FALSE the text (aka label) will be printed only once near
  the mid-point of the modelling curve.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_font, rl2_graph_draw_text
 
 \note if the modelling curve length is shorter than the text
 (aka label) length then no output at all will be printed.
 */
    RL2_DECLARE int rl2_graph_draw_warped_text (sqlite3 * handle,
						rl2GraphicsContextPtr context,
						const char *text, int points,
						double *x, double *y,
						double initial_gap, double gap,
						int repeated);

/**
 Computes the extent corresponding to a text into the Canvas using the currently set Font

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param text string to be printed into the canvas.
 \param pre_x on completion this variable will contain the horizontal 
  spacing before the text.
 \param pre_y on completion this variable will contain the vertical
  spacing before the text.
 \param width on completion this variable will contain the width of 
 the printed text.
 \param width on completion this variable will contain the height of 
 the printed text.
 \param post_x on completion this variable will contain the horizontal
  spacing after the text.
 \param post_y on completion this variable will contain the vertical
  spacing after the text.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_set_font, rl2_graph_get_text_extent
 */
    RL2_DECLARE int rl2_graph_get_text_extent (rl2GraphicsContextPtr context,
					       const char *text, double *pre_x,
					       double *pre_y, double *width,
					       double *height, double *post_x,
					       double *post_y);

/**
 Draws a Bitmap into the Canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param bitmap the pointer to a valid Graphics Bitmap to be rendered.
 \param x the X coordinate of the image's left upper corner.
 \param y the Y coordinate of the image's left upper corner.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_bitmap, rl2_graph_destroy_bitmap,
 rl2_graph_draw_rescaled_bitmap, rl2_graph_draw_graphic_symbol, 
 rl2_graph_draw_mark_symbol
 */
    RL2_DECLARE int rl2_graph_draw_bitmap (rl2GraphicsContextPtr context,
					   rl2GraphicsBitmapPtr bitmap,
					   double x, double y);

/**
 Draws a Rescaled Bitmap into the Canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param bitmap the pointer to a valid Graphics Bitmap to be rendered.
 \param scale_x the Scale Factor for X axis.
 \param scale_y the Scale Factor for Y axis.
 \param x the X coordinate of the image's left upper corner.
 \param y the Y coordinate of the image's left upper corner.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_bitmap, rl2_graph_destroy_bitmap,
 rl2_graph_draw_bitmap, rl2_graph_draw_tranformed_bitmap,
 rl2_graph_draw_graphic_symbol, rl2_graph_draw_mark_symbol
 */
    RL2_DECLARE int rl2_graph_draw_rescaled_bitmap (rl2GraphicsContextPtr
						    context,
						    rl2GraphicsBitmapPtr bitmap,
						    double scale_x,
						    double scale_y, double x,
						    double y);

/**
 Creates an Affine Transform Data object 

 \param xx the XX component - Affine Transform Matrix.
 \param yx the YX component - Affine Transform Matrix.
 \param xy the XY component - Affine Transform Matrix.
 \param yy the YY component - Affine Transform Matrix.
 \param xoff the X0 component - Affine Transform Matrix.
 \param yoff the Y0 component - Affine Transform Matrix.
 \param max_threads number of concurrent threads. 

 \return the pointer to the corresponding Affine Transform Data object: 
 NULL on failure.
 
 \sa rl2_destroy_affine_transform, rl2_set_affine_transform_origin,
 rl2_set_affine_transform_destination, rl2_transform_bitmap
 
 \note you are responsible to destroy (before or after) any Affine
 Trasform Data object created by rl2_create_affine_transform() by 
 invoking rl2_destroy_affine_transform().
 The object returned by rl2_create_affine_transform() is incomplete
 and not directly usable; you must call both rl2_set_affine_transform_origin()
 and rl2_set_affine_transform_destination() in order to get a complete
 and usable object.
 
 \sa rl2_destroy_affine_transform, rl2_set_affine_transform_origin,
 rl2_set_affine_transform_destination, rl2_is_valid_affine_transform,
 rl2_transform_bitmap
 */
    RL2_DECLARE rl2AffineTransformDataPtr
	rl2_create_affine_transform (double xx, double yx, double xy, double yy,
				     double xoff, double yoff, int max_threads);

/**
 Destroys an Affine Transform Data object freeing any allocated resource 

 \param ptr the pointer to a valid Affine Transform Data object
 returned by a previous call to rl2_create_affine_transform()
 
 \sa rl2_create_affine_transform
 */
    RL2_DECLARE void rl2_destroy_affine_transform (rl2AffineTransformDataPtr
						   ptr);

/**
 Assigning the Origin context to an Affine Transform Data object

 \param ptr the pointer to a valid Affine Transform Data object
 returned by a previous call to rl2_create_affine_transform()
 \param width width (in pixels) of the bitmap
 \param height height (in pixels) of the bitmap
 \param minx lower left corner: X coordinate (in map units).
 \param miny lower left corner: Y coordinate (in map units).
 \param maxx upper right corner: X coordinate (in map units).
 \param maxy upper right corner: Y coordinate (in map units).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_create_affine_transform, rl2_set_affine_transform_destination
 */
    RL2_DECLARE int
	rl2_set_affine_transform_origin (rl2AffineTransformDataPtr ptr,
					 int width, int height, double minx,
					 double miny, double maxx, double maxy);

/**
 Assigning the Destination context to an Affine Transform Data object

 \param ptr the pointer to a valid Affine Transform Data object
 returned by a previous call to rl2_create_affine_transform()
 \param width width (in pixels) of the bitmap
 \param height height (in pixels) of the bitmap
 \param minx lower left corner: X coordinate (in map units).
 \param miny lower left corner: Y coordinate (in map units).
 \param maxx upper right corner: X coordinate (in map units).
 \param maxy upper right corner: Y coordinate (in map units).

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_create_affine_transform, rl2_set_affine_transform_origin
 */
    RL2_DECLARE int
	rl2_set_affine_transform_destination (rl2AffineTransformDataPtr ptr,
					      int width, int height,
					      double minx, double miny,
					      double maxx, double maxy);

/**
 Checks an Affine Transform Data object for validity 

 \param ptr the pointer to a valid Affine Transform Data object
 returned by a previous call to rl2_create_affine_transform()

 \return 1 (true) on success, 0 (false) on error
 
 \sa rl2_create_affine_transform, rl2_set_affine_transform_origin,
 rl2_set_affine_transform_destination
 */
    RL2_DECLARE int
	rl2_is_valid_affine_transform (rl2AffineTransformDataPtr ptr);

/**
 Transforms a Bitmap by applying an Affine Transform 

 \param at_data the pointer to valid Affine Transform Data.
 \param bitmap indirect reference to a pointer to a valid Graphics 
 Bitmap to be transformed.

 \return 0 (false) on error, any other value on success.
 
 \sa rl2_graph_create_bitmap, rl2_graph_destroy_bitmap,
 rl2_create_affine_transform, rl2_set_affine_transform_origin,
 rl2_set_affine_transform_destination, rl2_destroy_affine_transform
 
 \note the Graphics Bitmap object passed to this function will be
 always internally destroied and eventually replaced by a new
 transformed Graphics Bitmap; in the failure case bitmap will point
 to NULL.
 */
    RL2_DECLARE int rl2_transform_bitmap (rl2AffineTransformDataPtr at_data,
					  rl2GraphicsBitmapPtr * bitmap);

/**
 Rescales a raw pixbuf (RGB or GRAYSCALE)

 \param inbuf pointer to the input pixbuf.
 \param inwidth the width (measured in pixels) of the input pixbuf.
 \param inheight the height (measured in pixels) of the input pixbuf.
 \param pixtype either RL2_PIXEL_RGB or RL2_PIXEL_GRAYSCALE.
 \param outbuf pointer to the input pixbuf.
 \param outwidth the width (measured in pixels) of the output pixbuf.
 \param outheight the height (measured in pixels) of the output pixbuf.

 \return 0 (false) on error, any other value on success.
 */
    RL2_DECLARE int rl2_rescale_pixbuf (const unsigned char *inbuf,
					unsigned int inwidth,
					unsigned int inheight,
					unsigned char pixtype,
					unsigned char *outbuf,
					unsigned int outwidth,
					unsigned int outheight);

/**
 Rescales a raw pixbuf (RGB or GRAYSCALE) - Transparent mode

 \param inbuf pointer to the input pixbuf.
 \param maskbuf pointer to the input mask.
 \param inwidth the width (measured in pixels) of the input pixbuf.
 \param inheight the height (measured in pixels) of the input pixbuf.
 \param pixtype either RL2_PIXEL_RGB or RL2_PIXEL_GRAYSCALE.
 \param outbuf pointer to the output pixbuf.
 \param outmask pointer to the output mask.
 \param outwidth the width (measured in pixels) of the output pixbuf.
 \param outheight the height (measured in pixels) of the output pixbuf.

 \return 0 (false) on error, any other value on success.
 */
    RL2_DECLARE int rl2_rescale_pixbuf_transparent (const unsigned char *inbuf,
						    const unsigned char
						    *maskbuf,
						    unsigned int inwidth,
						    unsigned int inheight,
						    unsigned char pixtype,
						    unsigned char *outbuf,
						    unsigned char *outmask,
						    unsigned int outwidth,
						    unsigned int outheight);

/**
 Merges a Graphics Context into another

 \param ctx_out the pointer to a valid Graphics Context (aka Canvas)
 receiving the merged result.
 \param ctx_in the pointer to another valid Graphics Context providing
 data to be merged.

 \return RL2_OK on succes; RL2_ERROR on failure.
 
 \note both Graphics Contexts must have identical Width and Height
 */
    RL2_DECLARE int rl2_graph_merge (rl2GraphicsContextPtr ctx_out,
				     rl2GraphicsContextPtr ctx_in);

/**
 Creates an RGB Array corresponding to the current Canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).

 \return the pointer to the RGB Array: NULL on failure.
 
 \sa rl2_graph_get_context_alpha_array
 
 \note you are responsible to destroy (before or after) any RGB Array
 returned by rl2_graph_get_context_rgb_array() by invoking free().
 */
    RL2_DECLARE unsigned char
	*rl2_graph_get_context_rgb_array (rl2GraphicsContextPtr context);

/**
 Creates an Array of Alpha values corresponding to the current Canvas

 \param context the pointer to a valid Graphics Context (aka Canvas).
 \param half_transparent after successful completion this variable will
 ontain 1 or 0, accordingly to the presence of half-transparencies 
 strictly requiring an alpha band.

 \return the pointer to the Array of Alpha Values: NULL on failure.
 
 \sa rl2_graph_get_context_rgb_array
 
 \note you are responsible to destroy (before or after) any RGB Array
 returned by rl2_graph_get_context_alpha_array() by invoking free().
 */
    RL2_DECLARE unsigned char
	*rl2_graph_get_context_alpha_array (rl2GraphicsContextPtr context,
					    int *half_transparent);

/**
 Allocates and initializes a new Canvas object (generic Vector Layer)

 \param ref_ctx pointer to Graphics Context
 \param ref_ctx_labels pointer to Labels Graphics Context
 
 \return the pointer to newly created Canvas Object: NULL on failure.
 
 \sa rl2_destroy_canvas, rl2_create_topology_canvas, rl2_create_network_canvas, 
 rl2_create_raster_canves, rl2_create_wms_canvas, rl2_get_canvas_type,
 rl2_get_canvas_ctx, rl2_is_canvas_ready, rl2_is_canvas_error
 
 \note you are responsible to destroy (before or after) any allocated 
 Canvas object.
 */
    RL2_DECLARE rl2CanvasPtr rl2_create_vector_canvas (rl2GraphicsContextPtr
						       ref_ctx,
						       rl2GraphicsContextPtr
						       ref_ctx_labels);

/**
 Allocates and initializes a new Canvas object (Topology Layer)

 \param ref_ctx pointer to base Graphics Context
 \param ref_ctx_labels pointer to Labels Graphics Context
 \param ref_ctx_nodes pointer to Nodes Graphics Context (may be NULL)
 \param ref_ctx_edges pointer to Edges Graphics Context (may be NULL)
 \param ref_ctx_faces pointer to Faces Graphics Context (may be NULL)
 \param ref_ctx_edge_seeds pointer to Edge Seeds Graphics Context (may be NULL)
 \param ref_ctx_face_seeds pointer to Face Seeds Graphics Context (may be NULL)
 
 \return the pointer to newly created Canvas Object: NULL on failure.
 
 \sa rl2_destroy_canvas, rl2_create_vector_canvas, rl2_create_network_canvas, 
 rl2_create_raster_canves, rl2_create_wms_canvas, rl2_get_canvas_type,
 rl2_get_canvas_ctx, rl2_is_canvas_ready, rl2_is_canvas_error
 
 \note you are responsible to destroy (before or after) any allocated 
 Canvas object.
 */
    RL2_DECLARE rl2CanvasPtr rl2_create_topology_canvas (rl2GraphicsContextPtr
							 ref_ctx,
							 rl2GraphicsContextPtr
							 ref_ctx_labels,
							 rl2GraphicsContextPtr
							 ref_ctx_nodes,
							 rl2GraphicsContextPtr
							 ref_ctx_edges,
							 rl2GraphicsContextPtr
							 ref_ctx_faces,
							 rl2GraphicsContextPtr
							 ref_ctx_edge_seeds,
							 rl2GraphicsContextPtr
							 ref_ctx_face_seeds);

/**
 Allocates and initializes a new Canvas object (Topology Layer)

 \param ref_ctx pointer to base Graphics Context
 \param ref_ctx_labels pointer to Labels Graphics Context
 \param ref_ctx_nodes pointer to Nodes Graphics Context (may be NULL)
 \param ref_ctx_links pointer to Links Graphics Context (may be NULL)
 \param ref_ctx_link_seeds pointer to Link Seeds Graphics Context (may be NULL)
 
 \return the pointer to newly created Canvas Object: NULL on failure.
 
 \sa rl2_destroy_canvas, rl2_create_vector_canvas, rl2_create_topology_canvas, 
 rl2_create_raster_canves, rl2_create_wms_canvas, rl2_get_canvas_type,
 rl2_get_canvas_ctx, rl2_is_canvas_ready, rl2_is_canvas_error
 
 \note you are responsible to destroy (before or after) any allocated 
 Canvas object.
 */
    RL2_DECLARE rl2CanvasPtr rl2_create_network_canvas (rl2GraphicsContextPtr
							ref_ctx,
							rl2GraphicsContextPtr
							ref_ctx_labels,
							rl2GraphicsContextPtr
							ref_ctx_nodes,
							rl2GraphicsContextPtr
							ref_ctx_links,
							rl2GraphicsContextPtr
							ref_ctx_link_seeds);

/**
 Allocates and initializes a new Canvas object (Raster Layer)

 \param ref_ctx pointer to Graphics Context
 
 \return the pointer to newly created Canvas Object: NULL on failure.
 
 \sa rl2_destroy_canvas, rl2_create_vector_canvas, rl2_create_topology_canvas, 
 rl2_create_network_canves, rl2_create_wms_canvas, rl2_get_canvas_type,
 rl2_get_canvas_ctx, rl2_is_canvas_ready, rl2_is_canvas_error
 
 \note you are responsible to destroy (before or after) any allocated 
 Canvas object.
 */
    RL2_DECLARE rl2CanvasPtr rl2_create_raster_canvas (rl2GraphicsContextPtr
						       ref_ctx);

/**
 Allocates and initializes a new Canvas object (WMS Layer)

 \param ref_ctx pointer to Graphics Context
 
 \return the pointer to newly created Canvas Object: NULL on failure.
 
 \sa rl2_destroy_canvas, rl2_create_vector_canvas, rl2_create_topology_canvas, 
 rl2_create_network_canves, rl2_create_raster_canvas, rl2_get_canvas_type,
 rl2_get_canvas_ctx, rl2_is_canvas_ready, rl2_is_canvas_error
 
 \note you are responsible to destroy (before or after) any allocated 
 Canvas object.
 */
    RL2_DECLARE rl2CanvasPtr rl2_create_wms_canvas (rl2GraphicsContextPtr
						    ref_ctx);

/**
 Destroys a Canvas Object

 \param canvas pointer to object to be destroyed

 \sa rl2_create_vector_canvas, rl2_create_topology_canvas, rl2_create_network_canvas, 
 rl2_create_raster_canves, rl2_create_wms_canvas
 */
    RL2_DECLARE void rl2_destroy_canvas (rl2CanvasPtr canvas);

/**
 Retrieving the Type of a Canvas object

 \param canvas pointer to a Canvas object
 
 \return the type of the Canvas object
 
 \sa rl2_create_vector_canvas, rl2_create_topology_canvas, 
 rl2_create_network_canves, rl2_create_raster_canvas, rl2_create_wms_canvas,
 rl2_get_canvas_ctx
 */
    RL2_DECLARE int rl2_get_canvas_type (rl2CanvasPtr canvas);

/**
 Retrieving the Graphics Context from a Canvas object

 \param canvas pointer to a Canvas object
 \param which one between RL2_CANVAS_BASE_CTX, RL2_CANVAS_LABELS_CTX,
 RL2_CANVAS_NODES_CTX, RL2_CANVAS_EDGES_CTX, RL2_CANVAS_LINKS_CTX, 
 RL2_CANVAS_FACES_CTX, RL2_CANVAS_EDGE_SEEDS_CTX, RL2_CANVAS_LINK_SEEDS_CTX, 
 RL2_CANVAS_FACE_CTX
 
 \return a pointer to a Graphics Context object (may be NULL)
 
 \sa rl2_create_vector_canvas, rl2_create_topology_canvas, 
 rl2_create_network_canves, rl2_create_raster_canvas, rl2_create_wms_canvas,
 rl2_get_canvas_type, rl2_is_canvas_ready, rl2_is_canvas_error
 */
    RL2_DECLARE rl2GraphicsContextPtr
	rl2_get_canvas_ctx (rl2CanvasPtr canvas, int which);

/**
 Testing if a Canvas object is ready to be used (rendered)

 \param canvas pointer to a Canvas object
 \param which one between RL2_CANVAS_BASE_CTX, RL2_CANVAS_LABELS_CTX,
 RL2_CANVAS_NODES_CTX, RL2_CANVAS_EDGES_CTX, RL2_CANVAS_LINKS_CTX, 
 RL2_CANVAS_FACES_CTX, RL2_CANVAS_EDGE_SEEDS_CTX, RL2_CANVAS_LINK_SEEDS_CTX, 
 RL2_CANVAS_FACE_CTX
 
 \return RL2_TRUE or RL2_FALSE
 
 \sa rl2_create_vector_canvas, rl2_create_topology_canvas, 
 rl2_create_network_canves, rl2_create_raster_canvas, rl2_create_wms_canvas,
 rl2_get_canvas_type, rl2_get_canvas_ctx, rl2_is_canvas_error
 */
    RL2_DECLARE int rl2_is_canvas_ready (rl2CanvasPtr canvas, int wich);

    RL2_DECLARE unsigned char *rl2_get_vector_map (sqlite3 * sqlite,
						   rl2CanvasPtr canvas,
						   const char *db_prefix,
						   const char *layer, int srid,
						   double minx, double miny,
						   double maxx, double maxy,
						   int width, int height,
						   const char *style);

#ifdef __cplusplus
}
#endif

#endif				/* _RL2GPAPHICS_H */
