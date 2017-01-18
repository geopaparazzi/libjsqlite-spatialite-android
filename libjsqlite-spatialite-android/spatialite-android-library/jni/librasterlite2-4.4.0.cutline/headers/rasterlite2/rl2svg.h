/*
 rl2svg.h -- RasterLite2 common SVG support
  
 version 2.0, 2014 June 6

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
 
Portions created by the Initial Developer are Copyright (C) 2014
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
 \file rl2svg.h

 WMS support
 */

#ifndef _RL2SVG_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RL2SVG_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 Typedef for RL2 SVG object (opaque, hidden)

 \sa rl2SvgPtr
 */
    typedef struct rl2_svg rl2Svg;
/**
 Typedef for RL2 SVG object pointer (opaque, hidden)

 \sa rl2Svg
 */
    typedef rl2Svg *rl2SvgPtr;

/**
 Allocates and initializes a new SVG object

 \param svg_document XML/SVG document to be parsed.
 \param svg_bytes lenght (in bytes) of the XML/SVG document
 
 \return the pointer to newly created SVG Object: NULL on failure.
 
 \sa rl2_destroy_svg, rl2_get_svg_size, rl2_rgba_from_svg,
 rl2_raster_from_svg
 
 \note you are responsible to destroy (before or after) any allocated 
 SVG object.
 */
    RL2_DECLARE rl2SvgPtr rl2_create_svg (const unsigned char *svg_document,
					  int svg_bytes);

/**
 Destroys an SVG Object

 \param svg_handle pointer to object to be destroyed

 \sa rl2_create_svg
 */
    RL2_DECLARE void rl2_destroy_svg (rl2SvgPtr sgv_handle);

/**
 Retrieving the Width/height dimensions from an SVG Object

 \param svg_handle pointer to the SVG Object.
 \param width on completion the variable referenced by this
 pointer will contain the SVG declared Width.
 \param height on completion the variable referenced by this
 pointer will contain the SVG declared Height.
 
 \return RL2_OK on success: RL2_ERROR on failure.

 \sa rl2_create_raster
 */
    RL2_DECLARE int rl2_get_svg_size (rl2SvgPtr svg_handle, double *width,
				      double *height);

/**
 Allocates and initializes a new Raster object from an in-memory stored SVG object

 \param svg_handle pointer to the memory block storing the input SVG object.
 \param size actual rescaled dimension.
 
 \return the pointer to newly created Raster Object: NULL on failure.
 
 \sa rl2_destroy_raster, rl2_create_raster
 
 \note you are responsible to destroy (before or after) any allocated 
 Raster object.
 */
    RL2_DECLARE rl2RasterPtr rl2_raster_from_svg (rl2SvgPtr svg_handle,
						  double size);


#ifdef __cplusplus
}
#endif

#endif				/* RL2SVG_H */
