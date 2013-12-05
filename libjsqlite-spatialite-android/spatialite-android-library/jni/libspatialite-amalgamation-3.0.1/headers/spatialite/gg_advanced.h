/*
 gg_advanced.h -- Gaia common support for geometries: advanced
  
 version 3.0, 2011 July 20

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

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008
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
 \file gg_advanced.h

 Geometry handling functions: advanced
 */

#ifndef _GG_ADVANCED_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_ADVANCED_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef OMIT_PROJ		/* including PROJ.4 */

/**
 Converts and angle from Radians into Degrees
 \param rads the angle measured in Radians.
 
 \return the angle measured in Degrees.

 \sa gaiaDegsToRads

 \remark \b PROJ.4 support required
 */
    GAIAGEO_DECLARE double gaiaRadsToDegs (double rads);

/**
 Converts and angle from Degrees into Radians
 \param degs the angle measured in Degrees.

 \return the angle measured in Radians.

 \sa gaiaRadsToDegs

 \remark \b PROJ.4 support required
 */
    GAIAGEO_DECLARE double gaiaDegsToRads (double degs);

/**
 Tansforms a Geometry object into a different Reference System
 [aka Reprojection]
 \param org pointer to input Geometry object.
 \param proj_from geodetic parameters string [EPSG format] qualifying the
 input Reference System
 \param proj_to geodetic parameters string [EPSG format] qualifying the
 output Reference System

 \return the pointer to newly created Geometry object: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,  this including any Geometry returned by gaiaGeometryTransform()

 \remark \b PROJ.4 support required
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaTransform (gaiaGeomCollPtr org,
						   char *proj_from,
						   char *proj_to);

#endif				/* end including PROJ.4 */

#ifndef OMIT_GEOS		/* including GEOS */

/**
 Resets the GEOS error and warning messages to an empty state

 \sa gaiaGetGeosErrorMsg, gaiaGetGeosWarningMsg, gaiaSetGeosErrorMsg,
 gaiaSetGeosWarningMsg

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE void gaiaResetGeosMsg (void);

/**
 Return the latest GEOS error message (if any)

 \return the latest GEOS error message: an empty string if no error was
 previoysly found.

 \sa gaiaResetGeosMsg, gaiaGetGeosWarningMsg, gaiaSetGeosErrorMsg,
 gaiaSetGeosWarningMsg

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE const char *gaiaGetGeosErrorMsg (void);

/**
 Return the latest GEOS warning message (if any)

 \return the latest GEOS warning message: an empty string if no warning was 
 previoysly found.

 \sa gaiaResetGeosMsg, gaiaGetGeosErrorMsg, gaiaSetGeosErrorMsg,
 gaiaSetGeosWarningMsg

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE const char *gaiaGetGeosWarningMsg (void);

/**
 Set the current GEOS error message

 \param msg the error message to be set.

 \sa gaiaResetGeosMsg, gaiaGetGeosErrorMsg, gaiaGetGeosWarningMsg,
 gaiaSetGeosWarningMsg

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE void gaiaSetGeosErrorMsg (const char *msg);

/**
 Set the current GEOS warning message

 \param msg the warning message to be set.

 \sa gaiaResetGeosMsg, gaiaGetGeosErrorMsg, gaiaGetGeosWarningMsg,
 gaiaSetGeosErrorMsg

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE void gaiaSetGeosWarningMsg (const char *msg);

/**
 Converts a Geometry object into a GEOS Geometry

 \param gaia pointer to Geometry object

 \return handle to GEOS Geometry
 
 \sa gaiaFromGeos_XY, gaiaFromGeos_XYZ, gaiaFromGeos_XYM, gaiaFromGeos_XYZM

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE void *gaiaToGeos (const gaiaGeomCollPtr gaia);

/**
 Converts a GEOS Geometry into a Geometry object [XY dims]

 \param geos handle to GEOS Geometry

 \return the pointer to the newly created Geometry object

 \sa gaiaToGeos, gaiaFromGeos_XYZ, gaiaFromGeos_XYM, gaiaFromGeos_XYZM

 \note you are responsible to destroy (before or after) any allocated 
 Geometry, this including any Geometry returned by gaiaFromGeos_XY()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromGeos_XY (const void *geos);

/**
 Converts a GEOS Geometry into a Geometry object [XYZ dims]

 \param geos handle to GEOS Geometry
    
 \return the pointer to the newly created Geometry object

 \sa gaiaToGeos, gaiaFromGeos_XY, gaiaFromGeos_XYM, gaiaFromGeos_XYZM
 
 \note you are responsible to destroy (before or after) any allocated 
 Geometry, this including any Geometry returned by gaiaFromGeos_XYZ()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromGeos_XYZ (const void *geos);

/**
 Converts a GEOS Geometry into a Geometry object [XYM dims]

 \param geos handle to GEOS Geometry
    
 \return the pointer to the newly created Geometry object

 \sa gaiaToGeos, gaiaFromGeos_XY, gaiaFromGeos_XYZ, gaiaFromGeos_XYZM
 
 \note you are responsible to destroy (before or after) any allocated 
 Geometry, this including any Geometry returned by gaiaFromGeos_XYM()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromGeos_XYM (const void *geos);

/**
 Converts a GEOS Geometry into a Geometry object [XYZM dims]

 \param geos handle to GEOS Geometry
    
 \return the pointer to the newly created Geometry object

 \sa gaiaToGeos, gaiaFromGeos_XY, gaiaFromGeos_XYZ, gaiaFromGeos_XYM
 
 \note you are responsible to destroy (before or after) any allocated 
 Geometry, this including any Geometry returned by gaiaFromGeos_XYZM()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaFromGeos_XYZM (const void *geos);

/**
 Checks if a Geometry object represents an OGC Simple Geometry

 \param geom pointer to Geometry object.

 \return 0 if false; any other value if true

 \sa gaiaIsClosed, gaiaIsRing, gaiaIsValid

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaIsSimple (gaiaGeomCollPtr geom);

/**
 Checks if a Linestring object represents an OGC Closed Geometry

 \param line pointer to Geometry object.

 \return 0 if false; any other value if true

 \sa gaiaIsSimple, gaiaIsRing, gaiaIsValid

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaIsClosed (gaiaLinestringPtr line);

/**
 Checks if a Linestring object represents an OGC Ring Geometry

 \param line pointer to Geometry object.

 \return 0 if false; any other value if true

 \sa gaiaIsSimple, gaiaIsClosed, gaiaIsValid

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaIsRing (gaiaLinestringPtr line);

/**
 Checks if a Geometry object represents an OGC Valid Geometry

 \param geom pointer to Geometry object.

 \return 0 if false; any other value if true

 \sa gaiaIsSimple, gaiaIsClosed, gaiaIsRing

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaIsValid (gaiaGeomCollPtr geom);

/**
 Measures the total Length for a Geometry object

 \param geom pointer to Geometry object
 \param length on completion this variable will contain the measured length

 \return 0 on failure: any other value on success

 \sa gaiaGeomCollArea, gaiaMeasureLength

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollLength (gaiaGeomCollPtr geom,
					    double *length);
/**
 Measures the total Area for a Geometry object

 \param geom pointer to Geometry object
 \param area on completion this variable will contain the measured area

 \return 0 on failure: any other value on success

 \sa gaiaGeomCollLength, gaiaMeasureArea

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollArea (gaiaGeomCollPtr geom, double *area);


/**
 Attempts to rearrange a generic Geometry object into a Polygon or MultiPolygon

 \param geom the input Geometry object
 \param force_multi if not set to 0, then an eventual Polygon will be 
 returned casted to MultiPolygon

 \return the pointer to newly created Geometry object representing a
 Polygon or MultiPolygon Geometry: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaPolygonize()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaPolygonize (gaiaGeomCollPtr geom,
						    int force_multi);
/**
 Spatial relationship evalution: Equals
 
 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated

 \return 0 if false: any other value if true

 \sa gaiaGeomCollDisjoint, gaiaGeomCollIntersects, gaiaGeomCollOverlaps,
 gaiaGeomCollCrosses, gaiaGeomCollContains, gaiaGeomCollWithin,
 gaiaGeomCollTouches, gaiaGeomCollRelate

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollEquals (gaiaGeomCollPtr geom1,
					    gaiaGeomCollPtr geom2);

/**
 Spatial relationship evalution: Disjoint

 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated

 \return 0 if false: any other value if true

 \sa gaiaGeomCollEquals, gaiaGeomCollIntersects, gaiaGeomCollOverlaps,
 gaiaGeomCollCrosses, gaiaGeomCollContains, gaiaGeomCollWithin,
 gaiaGeomCollTouches, gaiaGeomCollRelate
 
 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollDisjoint (gaiaGeomCollPtr geom1,
					      gaiaGeomCollPtr geom2);

/**
 Spatial relationship evalution: Intesects

 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated

 \return 0 if false: any other value if true

 \sa gaiaGeomCollEquals, gaiaGeomCollDisjoint, gaiaGeomCollOverlaps,
 gaiaGeomCollCrosses, gaiaGeomCollContains, gaiaGeomCollWithin,
 gaiaGeomCollTouches, gaiaGeomCollRelate
 
 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollIntersects (gaiaGeomCollPtr geom1,
						gaiaGeomCollPtr geom2);

/**
 Spatial relationship evalution: Overlaps

 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated

 \return 0 if false: any other value if true

 \sa gaiaGeomCollEquals, gaiaGeomCollDisjoint, gaiaGeomCollIntersects, 
 gaiaGeomCollCrosses, gaiaGeomCollContains, gaiaGeomCollWithin,
 gaiaGeomCollTouches, gaiaGeomCollRelate
 
 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollOverlaps (gaiaGeomCollPtr geom1,
					      gaiaGeomCollPtr geom2);

/**
 Spatial relationship evalution: Crosses

 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated

 \return 0 if false: any other value if true

 \sa gaiaGeomCollEquals, gaiaGeomCollDisjoint, gaiaGeomCollIntersects, 
 gaiaGeomCollOverlaps, gaiaGeomCollContains, gaiaGeomCollWithin,
 gaiaGeomCollTouches, gaiaGeomCollRelate
 
 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollCrosses (gaiaGeomCollPtr geom1,
					     gaiaGeomCollPtr geom2);

/**
 Spatial relationship evalution: Contains

 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated

 \return 0 if false: any other value if true

 \sa gaiaGeomCollEquals, gaiaGeomCollDisjoint, gaiaGeomCollIntersects, 
 gaiaGeomCollOverlaps, gaiaGeomCollCrosses, gaiaGeomCollWithin,
 gaiaGeomCollTouches, gaiaGeomCollRelate
 
 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollContains (gaiaGeomCollPtr geom1,
					      gaiaGeomCollPtr geom2);

/**
 Spatial relationship evalution: Within

 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated

 \return 0 if false: any other value if true

 \sa gaiaGeomCollEquals, gaiaGeomCollDisjoint, gaiaGeomCollIntersects, 
 gaiaGeomCollOverlaps, gaiaGeomCollCrosses, gaiaGeomCollContains, 
 gaiaGeomCollTouches, gaiaGeomCollRelate
 
 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollWithin (gaiaGeomCollPtr geom1,
					    gaiaGeomCollPtr geom2);

/**
 Spatial relationship evalution: Touches

 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated

 \return 0 if false: any other value if true

 \sa gaiaGeomCollEquals, gaiaGeomCollDisjoint, gaiaGeomCollIntersects, 
 gaiaGeomCollOverlaps, gaiaGeomCollCrosses, gaiaGeomCollContains, 
 gaiaGeomCollWithin, gaiaGeomCollRelate
 
 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollTouches (gaiaGeomCollPtr geom1,
					     gaiaGeomCollPtr geom2);

/**
 Spatial relationship evalution: Relate

 \param geom1 the first Geometry object to be evaluated
 \param geom2 the second Geometry object to be evaluated
 \param pattern intersection matrix pattern [DE-9IM]

 \return 0 if false: any other value if true

 \sa gaiaGeomCollEquals, gaiaGeomCollDisjoint, gaiaGeomCollIntersects, 
 gaiaGeomCollOverlaps, gaiaGeomCollCrosses, gaiaGeomCollContains, 
 gaiaGeomCollWithin, gaiaGeomCollTouches
 
 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollRelate (gaiaGeomCollPtr geom1,
					    gaiaGeomCollPtr geom2,
					    const char *pattern);

/**
 Calculates the minimum distance intercurring between two Geometry objects

 \param geom1 the first Geometry object 
 \param geom2 the second Geometry object 
 \param dist on completion this variable will contain the calculated distance

 \return 0 on failuer: any other value on success.

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollDistance (gaiaGeomCollPtr geom1,
					      gaiaGeomCollPtr geom2,
					      double *dist);

/**
 Spatial operator: Intersection
                                              
 \param geom1 the first Geometry object 
 \param geom2 the second Geometry object 

 \return the pointer to newly created Geometry object representing the
 geometry Intersection of both input Geometries: NULL on failure.

 \sa gaiaFreeGeomColl, gaiaGeometryUnion, gaiaGeometryDifference,
 gaiaGeometrySymDifference, gaiaBoundary

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaGeometryIntersection()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaGeometryIntersection (gaiaGeomCollPtr
							      geom1,
							      gaiaGeomCollPtr
							      geom2);

/**
 Spatial operator: Union

 \param geom1 the first Geometry object
 \param geom2 the second Geometry object

 \return the pointer to newly created Geometry object representing the
 geometry Union of both input Geometries: NULL on failure.

 \sa gaiaFreeGeomColl, gaiaUnaryUnion

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaGeometryUnion()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaGeometryUnion (gaiaGeomCollPtr geom1,
						       gaiaGeomCollPtr geom2);

/**
 Spatial operator: Difference

 \param geom1 the first Geometry object
 \param geom2 the second Geometry object

 \return the pointer to newly created Geometry object representing the
 geometry Difference of both input Geometries: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaGeometryDifference()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaGeometryDifference (gaiaGeomCollPtr
							    geom1,
							    gaiaGeomCollPtr
							    geom2);

/**
 Spatial operator: SymDifference

 \param geom1 the first Geometry object
 \param geom2 the second Geometry object

 \return the pointer to newly created Geometry object representing the
 geometry SymDifference of both input Geometries: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaGeometrySymDifference()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaGeometrySymDifference (gaiaGeomCollPtr
							       geom1,
							       gaiaGeomCollPtr
							       geom2);

/**
 Spatial operator: Boundary

 \param geom the Geometry object to be evaluated

 \return the pointer to newly created Geometry object representing the
 geometry Boundary of the input Geometry: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaBoundary()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaBoundary (gaiaGeomCollPtr geom);

/**
 Spatial operator: Centroid

 \param geom pointer to Geometry object.
 \param x on completion this variable will contain the centroid X coordinate 
 \param y on completion this variable will contain the centroid Y coordinate 
 
 \return 0 on failure: any other value on success

 \sa gaiaRingCentroid

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollCentroid (gaiaGeomCollPtr geom, double *x,
					      double *y);

/**
 Spatial operator: PointOnSurface

 \param geom pointer to Geometry object.
 \param x on completion this variable will contain the Point X coordinate  
 \param y on completion this variable will contain the Point Y coordinate
 
 \return 0 on failure: any other value on success

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE int gaiaGetPointOnSurface (gaiaGeomCollPtr geom, double *x,
					       double *y);

/**
 Spatial operator: Simplify

 \param geom the input Geometry object
 \param tolerance approximation threshold

 \return the pointer to newly created Geometry object representing the
 simplified Geometry [applying the Douglas-Peucker algorithm]: NULL on failure.

 \sa gaiaFreeGeomColl, gaiaGeomCollSimplifyPreserveTopology

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaGeomCollSimplify()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaGeomCollSimplify (gaiaGeomCollPtr geom,
							  double tolerance);

/**
 Spatial operator: Simplify [preserving topology]

 \param geom the input Geometry object
 \param tolerance approximation threshold

 \return the pointer to newly created Geometry object representing the
 simplified Geometry [applying the Douglas-Peucker algorithm]: NULL on failure.

 \sa gaiaFreeGeomColl, gaiaGeomCollSimplify

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaGeomCollSimplify()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr
	gaiaGeomCollSimplifyPreserveTopology (gaiaGeomCollPtr geom,
					      double tolerance);

/**
 Spatial operator: ConvexHull

 \param geom the input Geometry object

 \return the pointer to newly created Geometry object representing the
 ConvexHull of input Geometry: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaConvexHull()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaConvexHull (gaiaGeomCollPtr geom);

/** 
 Spatial operator: Buffer

 \param geom the input Geometry object
 \param radius the buffer's radius
 \param points number of points (aka vertices) to be used in order to 
 approximate a circular arc.

 \return the pointer to newly created Geometry object representing the
 Buffer of input Geometry: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaGeomCollBuffer()

 \remark \b GEOS support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaGeomCollBuffer (gaiaGeomCollPtr geom,
							double radius,
							int points);

#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#ifdef GEOS_ADVANCED
#endif

/**
 Calculates the Hausdorff distance intercurring between two Geometry objects

 \param geom1 pointer to first Geometry object
 \param geom2 pointer to second Geometry object
 \param dist on completion this variable will contain the calculated Hausdorff
 distance 

 \return 0 on failure: any other value on success.

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE int gaiaHausdorffDistance (gaiaGeomCollPtr geom1,
					       gaiaGeomCollPtr geom2,
					       double *dist);

/**
 Spatial operator: Offset Curve

 \param geom the input Geometry object
 \param radius the buffer's radius
 \param points number of points (aka vertices) to be used in order to 
 approximate a circular arc.
 \param left_right if set to 1 the left-sided OffsetCurve will be returned;
 otherwise the right-sided one.

 \return the pointer to newly created Geometry object representing the
 OffsetCurve of input Geometry: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaOffsetCurve()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaOffsetCurve (gaiaGeomCollPtr geom,
						     double radius, int points,
						     int left_right);

/**
 Spatial operator: Single Sided Buffer

 \param geom the input Geometry object
 \param radius the buffer's radius
 \param points number of points (aka vertices) to be used in order to
 approximate a circular arc.
 \param left_right if set to 1 the left-sided Buffer will be returned;
 otherwise the right-sided one.

 \return the pointer to newly created Geometry object representing the
 single-sided Buffer of input Geometry: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaSingleSidedBuffer()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaSingleSidedBuffer (gaiaGeomCollPtr geom,
							   double radius,
							   int points,
							   int left_right);

/**
 Spatial operator: Shared Paths

 \param geom1 pointer to first Geometry object
 \param geom2 pointer to second Geometry object

 \return the pointer to newly created Geometry object representing any
 Share Path common to both input geometries: NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaSharedPaths()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaSharedPaths (gaiaGeomCollPtr geom1,
						     gaiaGeomCollPtr geom2);

/**
 Spatial operator: Line Interpolate Point

 \param ln_geom the input Geometry object [expected to be of lineal type]
 \param fraction total length fraction [in the range 0.0 / 1.0]

 \return the pointer to newly created Geometry object representing a Point
 laying on the input Geometry and positioned at the given length fraction:
 NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaLineInterpolatePoint()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaLineInterpolatePoint (gaiaGeomCollPtr
							      ln_geom,
							      double fraction);

/**
 Spatial operator: Line Substring

 \param ln_geom the input Geometry object [expected to be of lineal type]
 \param start_fraction substring start, expressed as total length fraction
 [in the range 0.0 / 1.0]
 \param end_fraction substring end, expressed as total length fraction

 \return the pointer to newly created Geometry object representing a Linestring
 laying on the input Geometry.
 \n this Linestring will begin (and stop) at given total length fractions. 
 NULL on failure.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaLineSubstring()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaLineSubstring (gaiaGeomCollPtr ln_geom,
						       double start_fraction,
						       double end_fraction);

/**
 Spatial operator: Shortest Line

 \param geom1 pointer to the first Geometry object.
 \param geom2 pointer to the second Geometry object.

 \return the pointer to newly created Geometry object representing a Linestring;
 NULL on failure.
 \n the returned Linestring graphically represents the minimum distance 
 intercurrinng between both input geometries.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaShortestLine()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaShortestLine (gaiaGeomCollPtr geom1,
						      gaiaGeomCollPtr geom2);

/**
 Spatial operator: Shortest Line

 \param geom1 pointer to the first Geometry object.
 \param geom2 pointer to the second Geometry object.
 \param tolerance approximation factor

 \return the pointer to newly created Geometry object; NULL on failure.
 \n the returned Geometry represents the first input Geometry (nicely
 \e snapped to the second input Geometry, whenever is possible).

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaShortestLine()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaSnap (gaiaGeomCollPtr geom1,
					      gaiaGeomCollPtr geom2,
					      double tolerance);

/**
 Spatial operator: Line Merge

 \param geom pointer to input Geometry object.

 \return the pointer to newly created Geometry object; NULL on failure.
 \n if possible, this representing a reassembled Linestring or MultiLinestring.

 \sa gaiaFreeGeomColl

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaLineMerge()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaLineMerge (gaiaGeomCollPtr geom);

/**
 Spatial operator: Unary Union

 \param geom the input Geometry object.

 \return the pointer to newly created Geometry object: NULL on failure.
 \n this function is the same as gaiaGeometryUnion, except in that this
 works internally to the input Geometry itself.
 NULL on failure.

 \sa gaiaFreeGeomColl, gaiaGeometryUnion

 \note you are responsible to destroy (before or after) any allocated Geometry,
 this including any Geometry returned by gaiaUnaryUnion()

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE gaiaGeomCollPtr gaiaUnaryUnion (gaiaGeomCollPtr geom);

/**
 Determines the location of the closest Point on Linestring to the given Point

 \param ln_geom pointer to first input Geometry object [expected to be of
 the lineal type].
 \param pt_geom pointer to second input Geometry object [expected to be a
 Point].

 \return the fraction [in the range 0.0 / 1.0] of ln_geom total length
 where the closest Point to pt_geom lays.

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE double gaiaLineLocatePoint (gaiaGeomCollPtr ln_geom,
						gaiaGeomCollPtr pt_geom);

/** 
 Topology check: test if a Geometry covers another one

 \param geom1 pointer to first input Geometry object.
 \param geom2 pointer to second input Geometry object.

 \return 0 if false; any other value if geom1 \e spatially \e covers geom2.

 \sa gaiaGeomCollCoveredBy

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollCovers (gaiaGeomCollPtr geom1,
					    gaiaGeomCollPtr geom2);

/**
 Topology check: test if a Geometry is covered by another one
                                            
 \param geom1 pointer to first input Geometry object.
 \param geom2 pointer to second input Geometry object.
                                               
 \return 0 if false; any other value if geom2 is \e spatially \e covered \e by
 geom1.

 \sa gaiaGeomCollCovers

 \remark \b GEOS-ADVANCED support required.
 */
    GAIAGEO_DECLARE int gaiaGeomCollCoveredBy (gaiaGeomCollPtr geom1,
					       gaiaGeomCollPtr geom2);

#endif				/* end GEOS advanced and experimental features */

#endif				/* end including GEOS */

#ifdef __cplusplus
}
#endif

#endif				/* _GG_ADVANCED_H */
