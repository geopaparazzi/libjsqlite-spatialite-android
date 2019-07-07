/*

 gg_transform.c -- Gaia PROJ.4 wrapping
  
 version 4.3, 2015 June 29

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2015
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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32)
#include <libloaderapi.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifndef OMIT_PROJ		/* including PROJ.4 */
#ifdef PROJ_NEW			/* supporting PROJ.6 */
#include <proj.h>
#else /* supporting old PROJ.4 */
#include <proj_api.h>
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>
#include <spatialite_private.h>
#include <spatialite.h>

#include <spatialite/gaiageo.h>

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaMakeCircle (double cx, double cy, double radius, double step)
{
/* return a Linestring approximating a Circle */
    return gaiaMakeEllipse (cx, cy, radius, radius, step);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaMakeArc (double cx,
	     double cy, double radius, double start, double stop, double step)
{
/* return a Linestring approximating a Circular Arc */
    return gaiaMakeEllipticArc (cx, cy, radius, radius, start, stop, step);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaMakeEllipse (double cx, double cy, double x_axis, double y_axis,
		 double step)
{
/* return a Linestring approximating an Ellipse */
    gaiaDynamicLinePtr dyn;
    double x;
    double y;
    double angle = 0.0;
    int points = 0;
    gaiaPointPtr pt;
    gaiaGeomCollPtr geom;
    gaiaLinestringPtr ln;
    int iv = 0;

    if (step < 0.0)
	step *= -1.0;
    if (step == 0.0)
	step = 10.0;
    if (step < 0.1)
	step = 0.1;
    if (step > 45.0)
	step = 45.0;
    if (x_axis < 0.0)
	x_axis *= -1.0;
    if (y_axis < 0.0)
	y_axis *= -1.0;
    dyn = gaiaAllocDynamicLine ();
    while (angle < 360.0)
      {
	  double rads = angle * .0174532925199432958;
	  x = cx + (x_axis * cos (rads));
	  y = cy + (y_axis * sin (rads));
	  gaiaAppendPointToDynamicLine (dyn, x, y);
	  angle += step;
      }
/* closing the ellipse */
    gaiaAppendPointToDynamicLine (dyn, dyn->First->X, dyn->First->Y);

    pt = dyn->First;
    while (pt)
      {
	  /* counting how many points */
	  points++;
	  pt = pt->Next;
      }
    if (points == 0)
      {
	  gaiaFreeDynamicLine (dyn);
	  return NULL;
      }

    geom = gaiaAllocGeomColl ();
    ln = gaiaAddLinestringToGeomColl (geom, points);
    pt = dyn->First;
    while (pt)
      {
	  /* setting Vertices */
	  gaiaSetPoint (ln->Coords, iv, pt->X, pt->Y);
	  iv++;
	  pt = pt->Next;
      }
    gaiaFreeDynamicLine (dyn);
    return geom;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaMakeEllipticArc (double cx,
		     double cy, double x_axis, double y_axis, double start,
		     double stop, double step)
{
/* return a Linestring approximating an Elliptic Arc */
    gaiaDynamicLinePtr dyn;
    double x;
    double y;
    double angle;
    double rads;
    int points = 0;
    gaiaPointPtr pt;
    gaiaGeomCollPtr geom;
    gaiaLinestringPtr ln;
    int iv = 0;

    if (step < 0.0)
	step *= -1.0;
    if (step == 0.0)
	step = 10.0;
    if (step < 0.1)
	step = 0.1;
    if (step > 45.0)
	step = 45.0;
    if (x_axis < 0.0)
	x_axis *= -1.0;
    if (y_axis < 0.0)
	y_axis *= -1.0;
/* normalizing Start/Stop angles */
    while (start >= 360.0)
	start -= 360.0;
    while (start <= -720.0)
	start += 360;
    while (stop >= 360.0)
	stop -= 360.0;
    while (stop <= -720.0)
	stop += 360;
    if (start < 0.0)
	start = 360.0 + start;
    if (stop < 0.0)
	stop = 360.0 + stop;
    if (start > stop)
	stop += 360.0;

    dyn = gaiaAllocDynamicLine ();
    angle = start;
    while (angle < stop)
      {
	  rads = angle * .0174532925199432958;
	  x = cx + (x_axis * cos (rads));
	  y = cy + (y_axis * sin (rads));
	  gaiaAppendPointToDynamicLine (dyn, x, y);
	  angle += step;
	  points++;
      }
    if (points == 0)
      {
	  gaiaFreeDynamicLine (dyn);
	  return NULL;
      }
/* closing the arc */
    rads = stop * .0174532925199432958;
    x = cx + (x_axis * cos (rads));
    y = cy + (y_axis * sin (rads));
    if (x != dyn->Last->X || y != dyn->Last->Y)
	gaiaAppendPointToDynamicLine (dyn, x, y);

    pt = dyn->First;
    points = 0;
    while (pt)
      {
	  /* counting how many points */
	  points++;
	  pt = pt->Next;
      }
    if (points == 0)
      {
	  gaiaFreeDynamicLine (dyn);
	  return NULL;
      }

    geom = gaiaAllocGeomColl ();
    ln = gaiaAddLinestringToGeomColl (geom, points);
    pt = dyn->First;
    while (pt)
      {
	  /* setting Vertices */
	  gaiaSetPoint (ln->Coords, iv, pt->X, pt->Y);
	  iv++;
	  pt = pt->Next;
      }
    gaiaFreeDynamicLine (dyn);
    return geom;
}

GAIAGEO_DECLARE void
gaiaShiftCoords (gaiaGeomCollPtr geom, double shift_x, double shift_y)
{
/* returns a geometry that is the old geometry with required shifting applied to coordinates */
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    gaiaPointPtr point;
    gaiaPolygonPtr polyg;
    gaiaLinestringPtr line;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* shifting POINTs */
	  point->X += shift_x;
	  point->Y += shift_y;
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* shifting LINESTRINGs */
	  for (iv = 0; iv < line->Points; iv++)
	    {
		m = 0.0;
		z = 0.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		x += shift_x;
		y += shift_y;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (line->Coords, iv, x, y, z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (line->Coords, iv, x, y, m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (line->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (line->Coords, iv, x, y);
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* shifting POLYGONs */
	  m = 0.0;
	  z = 0.0;
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* shifting the EXTERIOR RING */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		x += shift_x;
		y += shift_y;
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ring->Coords, iv, x, y, m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ring->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* shifting the INTERIOR RINGs */
		ring = polyg->Interiors + ib;
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      x += shift_x;
		      y += shift_y;
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (ring->Coords, iv, x, y, m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (ring->Coords, iv, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    gaiaMbrGeometry (geom);
}

GAIAGEO_DECLARE void
gaiaShiftCoords3D (gaiaGeomCollPtr geom, double shift_x, double shift_y,
		   double shift_z)
{
/* returns a geometry that is the old geometry with required shifting applied to coordinates */
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    gaiaPointPtr point;
    gaiaPolygonPtr polyg;
    gaiaLinestringPtr line;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* shifting POINTs */
	  point->X += shift_x;
	  point->Y += shift_y;
	  if (point->DimensionModel == GAIA_XY_Z
	      || point->DimensionModel == GAIA_XY_Z_M)
	      point->Z += shift_z;
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* shifting LINESTRINGs */
	  for (iv = 0; iv < line->Points; iv++)
	    {
		m = 0.0;
		z = 0.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		x += shift_x;
		y += shift_y;
		z += shift_z;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (line->Coords, iv, x, y, z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (line->Coords, iv, x, y, m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (line->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (line->Coords, iv, x, y);
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* shifting POLYGONs */
	  m = 0.0;
	  z = 0.0;
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* shifting the EXTERIOR RING */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		x += shift_x;
		y += shift_y;
		z += shift_z;
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ring->Coords, iv, x, y, m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ring->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* shifting the INTERIOR RINGs */
		ring = polyg->Interiors + ib;
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      x += shift_x;
		      y += shift_y;
		      z += shift_z;
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (ring->Coords, iv, x, y, m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (ring->Coords, iv, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    gaiaMbrGeometry (geom);
}

GAIAGEO_DECLARE void
gaiaShiftLongitude (gaiaGeomCollPtr geom)
{
/* returns a geometry that is the old geometry with negative longitudes shift by 360 */
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    gaiaPointPtr point;
    gaiaPolygonPtr polyg;
    gaiaLinestringPtr line;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* shifting POINTs */
	  if (point->X < 0)
	    {
		point->X += 360.0;
	    }
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* shifting LINESTRINGs */
	  for (iv = 0; iv < line->Points; iv++)
	    {
		m = 0.0;
		z = 0.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		if (x < 0)
		  {
		      x += 360.0;
		  }
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (line->Coords, iv, x, y, z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (line->Coords, iv, x, y, m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (line->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (line->Coords, iv, x, y);
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* shifting POLYGONs */
	  m = 0.0;
	  z = 0.0;
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* shifting the EXTERIOR RING */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		if (x < 0)
		  {
		      x += 360.0;
		  }
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ring->Coords, iv, x, y, m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ring->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* shifting the INTERIOR RINGs */
		ring = polyg->Interiors + ib;
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      if (x < 0)
			{
			    x += 360.0;
			}
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (ring->Coords, iv, x, y, m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (ring->Coords, iv, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    gaiaMbrGeometry (geom);
}

static void
normalizePoint (double *x, double *y)
{
    if ((-180.0 <= *x) && (*x <= 180.0) && (-90.0 <= *y) && (*y <= 90.0))
      {
	  /* then this point is already OK */
	  return;
      }
    if ((*x > 180.0) || (*x < -180.0))
      {
	  int numCycles = (int) (*x / 360.0);
	  *x -= numCycles * 360.0;
      }
    if (*x > 180.0)
      {
	  *x -= 360.0;
      }
    if (*x < -180.0)
      {
	  *x += 360.0;
      }
    if ((*y > 90.0) || (*y < -90.0))
      {
	  int numCycles = (int) (*y / 360.0);
	  *y -= numCycles * 360.0;
      }
    if (*y > 180.0)
      {
	  *y = -1.0 * (*y - 180.0);
      }
    if (*y < -180.0)
      {
	  *y = -1.0 * (*y + 180.0);
      }
    if (*y > 90.0)
      {
	  *y = 180 - *y;
      }
    if (*y < -90.0)
      {
	  *y = -180.0 - *y;
      }
}

GAIAGEO_DECLARE void
gaiaNormalizeLonLat (gaiaGeomCollPtr geom)
{
/* returns a geometry that is the old geometry with all latitudes shifted
 into the range -90 to 90, and all longitudes shifted into the range -180 to
 180.
*/
    int ib;
    int iv;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    gaiaPointPtr point;
    gaiaPolygonPtr polyg;
    gaiaLinestringPtr line;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  normalizePoint (&(point->X), &(point->Y));
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* shifting LINESTRINGs */
	  for (iv = 0; iv < line->Points; iv++)
	    {
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		normalizePoint (&x, &y);
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (line->Coords, iv, x, y, z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (line->Coords, iv, x, y, m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (line->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (line->Coords, iv, x, y);
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* shifting POLYGONs */
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* shifting the EXTERIOR RING */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		normalizePoint (&x, &y);
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ring->Coords, iv, x, y, m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ring->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* shifting the INTERIOR RINGs */
		ring = polyg->Interiors + ib;
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      normalizePoint (&x, &y);
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (ring->Coords, iv, x, y, m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (ring->Coords, iv, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    gaiaMbrGeometry (geom);
}

GAIAGEO_DECLARE void
gaiaScaleCoords (gaiaGeomCollPtr geom, double scale_x, double scale_y)
{
/* returns a geometry that is the old geometry with required scaling applied to coordinates */
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    gaiaPointPtr point;
    gaiaPolygonPtr polyg;
    gaiaLinestringPtr line;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* scaling POINTs */
	  point->X *= scale_x;
	  point->Y *= scale_y;
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* scaling LINESTRINGs */
	  for (iv = 0; iv < line->Points; iv++)
	    {
		m = 0.0;
		z = 0.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		x *= scale_x;
		y *= scale_y;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (line->Coords, iv, x, y, z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (line->Coords, iv, x, y, m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (line->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (line->Coords, iv, x, y);
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* scaling POLYGONs */
	  m = 0.0;
	  z = 0.0;
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* scaling the EXTERIOR RING */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		x *= scale_x;
		y *= scale_y;
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ring->Coords, iv, x, y, m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ring->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* scaling the INTERIOR RINGs */
		ring = polyg->Interiors + ib;
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      x *= scale_x;
		      y *= scale_y;
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (ring->Coords, iv, x, y, m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (ring->Coords, iv, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    gaiaMbrGeometry (geom);
}

GAIAGEO_DECLARE void
gaiaRotateCoords (gaiaGeomCollPtr geom, double angle)
{
/* returns a geometry that is the old geometry with required rotation applied to coordinates */
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    double nx;
    double ny;
    double rad = angle * 0.0174532925199432958;
    double cosine = cos (rad);
    double sine = sin (rad);
    gaiaPointPtr point;
    gaiaPolygonPtr polyg;
    gaiaLinestringPtr line;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* shifting POINTs */
	  x = point->X;
	  y = point->Y;
	  point->X = (x * cosine) + (y * sine);
	  point->Y = (y * cosine) - (x * sine);
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* rotating LINESTRINGs */
	  for (iv = 0; iv < line->Points; iv++)
	    {
		m = 0.0;
		z = 0.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		nx = (x * cosine) + (y * sine);
		ny = (y * cosine) - (x * sine);
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (line->Coords, iv, nx, ny, z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (line->Coords, iv, nx, ny, m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (line->Coords, iv, nx, ny, z, m);
		  }
		else
		  {
		      gaiaSetPoint (line->Coords, iv, nx, ny);
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* rotating POLYGONs */
	  m = 0.0;
	  z = 0.0;
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* rotating the EXTERIOR RING */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		nx = (x * cosine) + (y * sine);
		ny = (y * cosine) - (x * sine);
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, nx, ny, z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ring->Coords, iv, nx, ny, m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ring->Coords, iv, nx, ny, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ring->Coords, iv, nx, ny);
		  }
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* rotating the INTERIOR RINGs */
		m = 0.0;
		z = 0.0;
		ring = polyg->Interiors + ib;
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      nx = (x * cosine) + (y * sine);
		      ny = (y * cosine) - (x * sine);
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (ring->Coords, iv, nx, ny, z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (ring->Coords, iv, nx, ny, m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (ring->Coords, iv, nx, ny, z, m);
			}
		      else
			{
			    gaiaSetPoint (ring->Coords, iv, nx, ny);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    gaiaMbrGeometry (geom);
}

GAIAGEO_DECLARE void
gaiaReflectCoords (gaiaGeomCollPtr geom, int x_axis, int y_axis)
{
/* returns a geometry that is the old geometry with required reflection applied to coordinates */
    int ib;
    int iv;
    double x;
    double y;
    double z = 0.0;
    double m = 0.0;
    gaiaPointPtr point;
    gaiaPolygonPtr polyg;
    gaiaLinestringPtr line;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* reflecting POINTs */
	  if (x_axis)
	      point->X *= -1.0;
	  if (y_axis)
	      point->Y *= -1.0;
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* reflecting LINESTRINGs */
	  for (iv = 0; iv < line->Points; iv++)
	    {
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		if (x_axis)
		    x *= -1.0;
		if (y_axis)
		    y *= -1.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (line->Coords, iv, x, y, z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (line->Coords, iv, x, y, m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (line->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (line->Coords, iv, x, y);
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* reflecting POLYGONs */
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* reflecting the EXTERIOR RING */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		if (x_axis)
		    x *= -1.0;
		if (y_axis)
		    y *= -1.0;
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ring->Coords, iv, x, y, m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ring->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* reflecting the INTERIOR RINGs */
		ring = polyg->Interiors + ib;
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      if (x_axis)
			  x *= -1.0;
		      if (y_axis)
			  y *= -1.0;
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (ring->Coords, iv, x, y, m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (ring->Coords, iv, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    gaiaMbrGeometry (geom);
}

GAIAGEO_DECLARE void
gaiaSwapCoords (gaiaGeomCollPtr geom)
{
/* returns a geometry that is the old geometry with swapped x- and y-coordinates */
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    double sv;
    gaiaPointPtr point;
    gaiaPolygonPtr polyg;
    gaiaLinestringPtr line;
    gaiaRingPtr ring;
    if (!geom)
	return;
    point = geom->FirstPoint;
    while (point)
      {
	  /* swapping POINTs */
	  sv = point->X;
	  point->X = point->Y;
	  point->Y = sv;
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* swapping LINESTRINGs */
	  for (iv = 0; iv < line->Points; iv++)
	    {
		m = 0.0;
		z = 0.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		sv = x;
		x = y;
		y = sv;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (line->Coords, iv, x, y, z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (line->Coords, iv, x, y, m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (line->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (line->Coords, iv, x, y);
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* swapping POLYGONs */
	  m = 0.0;
	  z = 0.0;
	  ring = polyg->Exterior;
	  for (iv = 0; iv < ring->Points; iv++)
	    {
		/* shifting the EXTERIOR RING */
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ring->Coords, iv, &x, &y);
		  }
		sv = x;
		x = y;
		y = sv;
		if (ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
		  }
		else if (ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ring->Coords, iv, x, y, m);
		  }
		else if (ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ring->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		/* swapping the INTERIOR RINGs */
		m = 0.0;
		z = 0.0;
		ring = polyg->Interiors + ib;
		for (iv = 0; iv < ring->Points; iv++)
		  {
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ring->Coords, iv, &x, &y);
			}
		      sv = x;
		      x = y;
		      y = sv;
		      if (ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (ring->Coords, iv, x, y, z);
			}
		      else if (ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (ring->Coords, iv, x, y, m);
			}
		      else if (ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (ring->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (ring->Coords, iv, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    gaiaMbrGeometry (geom);
}

#ifndef OMIT_PROJ		/* including PROJ.4 */

#ifndef PROJ_NEW		/* supporting old PROJ.4 */
static int
gaiaIsLongLat (const char *str)
{
/* checks if we have to do with ANGLES if +proj=longlat is defined */
    if (str == NULL)
	return 0;
    if (strstr (str, "+proj=longlat") != NULL)
	return 1;
    return 0;
}
#endif

#ifdef PROJ_NEW			/* only if PROJ.6 is supported */

GAIAGEO_DECLARE void
gaiaResetProjErrorMsg_r (const void *p_cache)
{
/* resets the PROJ error message */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	    {
		if (cache->gaia_proj_error_msg != NULL)
		    sqlite3_free (cache->gaia_proj_error_msg);
		cache->gaia_proj_error_msg = NULL;
	    }
      }
}

GAIAGEO_DECLARE const char *
gaiaSetProjDatabasePath (const void *p_cache, const char *path)
{
/* return the currently set PATH leading to the private PROJ.6 database */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	    {
		if (!proj_context_set_database_path
		    (cache->PROJ_handle, path, NULL, NULL))
		    return NULL;
		return proj_context_get_database_path (cache->PROJ_handle);
	    }
      }
    return NULL;
}

GAIAGEO_DECLARE const char *
gaiaGetProjDatabasePath (const void *p_cache)
{
/* return the currently set PATH leading to the private PROJ.6 database */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	      return proj_context_get_database_path (cache->PROJ_handle);
      }
    return NULL;
}

GAIAGEO_DECLARE const char *
gaiaGetProjErrorMsg_r (const void *p_cache)
{
/* return the latest PROJ error message */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	    {
		if (cache->gaia_proj_error_msg != NULL)
		    return cache->gaia_proj_error_msg;
	    }
      }
    return NULL;
}

GAIAGEO_DECLARE void
gaiaSetProjErrorMsg_r (const void *p_cache, const char *msg)
{
/* setting the latest PROJ error message */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	    {
		if (cache->gaia_proj_error_msg != NULL)
		    sqlite3_free (cache->gaia_proj_error_msg);
		cache->gaia_proj_error_msg = sqlite3_mprintf ("%s", msg);
	    }
      }
}
#endif


GAIAGEO_DECLARE double
gaiaRadsToDegs (double rads)
{
/* converts an ANGLE from radians to degrees */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    return proj_todeg (rads);
#else /* supporting old PROJ.4 */
    return rads * RAD_TO_DEG;
#endif
}

GAIAGEO_DECLARE double
gaiaDegsToRads (double degs)
{
/* converts an ANGLE from degrees to radians */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    return proj_torad (degs);
#else /* supporting old PROJ.4 */
    return degs * DEG_TO_RAD;
#endif
}

static int
do_transfom_proj (gaiaGeomCollPtr org, gaiaGeomCollPtr dst, int ignore_z,
		  int ignore_m, int from_radians, int to_radians, void *xfrom,
		  void *xto, void *x_from_to)
{
/* supporting either old PROJ.4 or new PROJ.6 */
    int error = 0;
    int ret;
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    int result_count;
#endif
    int ib;
    int cnt;
    int i;
    double *xx;
    double *yy;
    double *zz;
    double *old_zz = NULL;
    double *mm = NULL;
    double *old_mm = NULL;
    double x;
    double y;
    double z;
    double m;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr dst_ln;
    gaiaPolygonPtr pg;
    gaiaPolygonPtr dst_pg;
    gaiaRingPtr rng;
    gaiaRingPtr dst_rng;
#ifdef PROJ_NEW			/* only if new PROJ.6 is supported */
    PJ *from_to_cs = (PJ *) x_from_to;
    if (xfrom == NULL)
	xfrom = NULL;		/* silencing stupid compiler warnings - unused arg */
    if (xto == NULL)
	xto = NULL;		/* silencing stupid compiler warnings - unused arg */
#else /* only id old PROJ.4 is supported */
    projPJ from_cs = (projPJ) xfrom;
    projPJ to_cs = (projPJ) xto;
    if (x_from_to == NULL)
	x_from_to = NULL;	/* silencing stupid compiler warnings - unused arg */
#endif

    cnt = 0;
    pt = org->FirstPoint;
    while (pt)
      {
	  /* counting POINTs */
	  cnt++;
	  pt = pt->Next;
      }
    if (cnt)
      {
	  /* reprojecting POINTs */
	  xx = malloc (sizeof (double) * cnt);
	  yy = malloc (sizeof (double) * cnt);
	  zz = malloc (sizeof (double) * cnt);
	  mm = malloc (sizeof (double) * cnt);
	  for (i = 0; i < cnt; i++)
	    {
		/* zeroing unused coordinates */
		if (ignore_z || org->DimensionModel == GAIA_XY
		    || org->DimensionModel == GAIA_XY_M)
		    zz[i] = 0.0;
		if (ignore_m || org->DimensionModel == GAIA_XY
		    || org->DimensionModel == GAIA_XY_Z)
		    mm[i] = 0.0;
	    }
	  if (ignore_z
	      && (org->DimensionModel == GAIA_XY_Z
		  || org->DimensionModel == GAIA_XY_Z_M))
	      old_zz = malloc (sizeof (double) * cnt);
	  if (ignore_m
	      && (org->DimensionModel == GAIA_XY_M
		  || org->DimensionModel == GAIA_XY_Z_M))
	      old_mm = malloc (sizeof (double) * cnt);
	  i = 0;
	  pt = org->FirstPoint;
	  while (pt)
	    {
		/* inserting points to be converted in temporary arrays */
		if (from_radians)
		  {
		      xx[i] = gaiaDegsToRads (pt->X);
		      yy[i] = gaiaDegsToRads (pt->Y);
		  }
		else
		  {
		      xx[i] = pt->X;
		      yy[i] = pt->Y;
		  }
		if (org->DimensionModel == GAIA_XY_Z
		    || org->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (ignore_z)
			  old_zz[i] = pt->Z;
		      else
			  zz[i] = pt->Z;
		  }
		if (org->DimensionModel == GAIA_XY_M
		    || org->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (ignore_m)
			  old_mm[i] = pt->M;
		      else
			  mm[i] = pt->M;
		  }
		i++;
		pt = pt->Next;
	    }
	  /* applying reprojection */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
	  result_count =
	      proj_trans_generic (from_to_cs, PJ_FWD, xx, sizeof (double), cnt,
				  yy, sizeof (double), cnt, zz, sizeof (double),
				  cnt, mm, sizeof (double), cnt);
	  if (result_count == cnt)
	      ret = 0;
	  else
	      ret = 1;
#else /* supporting old PROJ.4 */
	  ret = pj_transform (from_cs, to_cs, cnt, 0, xx, yy, zz);
#endif
	  if (ret == 0)
	    {
		/* inserting the reprojected POINTs in the new GEOMETRY */
		for (i = 0; i < cnt; i++)
		  {
		      if (to_radians)
			{
			    x = gaiaRadsToDegs (xx[i]);
			    y = gaiaRadsToDegs (yy[i]);
			}
		      else
			{
			    x = xx[i];
			    y = yy[i];
			}
		      if (org->DimensionModel == GAIA_XY_Z
			  || org->DimensionModel == GAIA_XY_Z_M)
			{
			    if (ignore_z)
				z = old_zz[i];
			    else
				z = zz[i];
			}
		      if (org->DimensionModel == GAIA_XY_M
			  || org->DimensionModel == GAIA_XY_Z_M)
			{
			    if (ignore_m)
				m = old_mm[i];
			    else
				m = mm[i];
			}
		      if (dst->DimensionModel == GAIA_XY_Z)
			  gaiaAddPointToGeomCollXYZ (dst, x, y, z);
		      else if (dst->DimensionModel == GAIA_XY_M)
			  gaiaAddPointToGeomCollXYM (dst, x, y, m);
		      else if (dst->DimensionModel == GAIA_XY_Z_M)
			  gaiaAddPointToGeomCollXYZM (dst, x, y, z, m);
		      else
			  gaiaAddPointToGeomColl (dst, x, y);
		  }
	    }
	  else
	      error = 1;
	  free (xx);
	  free (yy);
	  free (zz);
	  free (mm);
	  if (old_zz != NULL)
	      free (old_zz);
	  if (old_mm != NULL)
	      free (old_mm);
	  xx = NULL;
	  yy = NULL;
	  zz = NULL;
	  mm = NULL;
	  old_zz = NULL;
	  old_mm = NULL;
      }
    if (error)
	goto stop;
    ln = org->FirstLinestring;
    while (ln)
      {
	  /* reprojecting LINESTRINGs */
	  cnt = ln->Points;
	  xx = malloc (sizeof (double) * cnt);
	  yy = malloc (sizeof (double) * cnt);
	  zz = malloc (sizeof (double) * cnt);
	  mm = malloc (sizeof (double) * cnt);
	  for (i = 0; i < cnt; i++)
	    {
		/* zeroing unused coordinates */
		if (ignore_z || org->DimensionModel == GAIA_XY
		    || org->DimensionModel == GAIA_XY_M)
		    zz[i] = 0.0;
		if (ignore_m || org->DimensionModel == GAIA_XY
		    || org->DimensionModel == GAIA_XY_Z)
		    mm[i] = 0.0;
	    }
	  if (ignore_z
	      && (org->DimensionModel == GAIA_XY_Z
		  || org->DimensionModel == GAIA_XY_Z_M))
	      old_zz = malloc (sizeof (double) * cnt);
	  if (ignore_m
	      && (org->DimensionModel == GAIA_XY_M
		  || org->DimensionModel == GAIA_XY_Z_M))
	      old_mm = malloc (sizeof (double) * cnt);
	  for (i = 0; i < cnt; i++)
	    {
		/* inserting points to be converted in temporary arrays */
		z = 0.0;
		m = 0.0;
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, i, &x, &y, &z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, i, &x, &y, &m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, i, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, i, &x, &y);
		  }
		if (from_radians)
		  {
		      xx[i] = gaiaDegsToRads (x);
		      yy[i] = gaiaDegsToRads (y);
		  }
		else
		  {
		      xx[i] = x;
		      yy[i] = y;
		  }
		if (ln->DimensionModel == GAIA_XY_Z
		    || ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (ignore_z)
			  old_zz[i] = z;
		      else
			  zz[i] = z;
		  }
		if (ln->DimensionModel == GAIA_XY_M
		    || ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (ignore_m)
			  old_mm[i] = m;
		      else
			  mm[i] = m;
		  }
	    }
	  /* applying reprojection */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
	  result_count =
	      proj_trans_generic (from_to_cs, PJ_FWD, xx, sizeof (double), cnt,
				  yy, sizeof (double), cnt, zz, sizeof (double),
				  cnt, mm, sizeof (double), cnt);
	  if (result_count == cnt)
	      ret = 0;
	  else
	      ret = 1;
#else /* supporting old PROJ.4 */
	  ret = pj_transform (from_cs, to_cs, cnt, 0, xx, yy, zz);
#endif
	  if (ret == 0)
	    {
		/* inserting the reprojected LINESTRING in the new GEOMETRY */
		dst_ln = gaiaAddLinestringToGeomColl (dst, cnt);
		for (i = 0; i < cnt; i++)
		  {
		      /* setting LINESTRING points */
		      if (to_radians)
			{
			    x = gaiaRadsToDegs (xx[i]);
			    y = gaiaRadsToDegs (yy[i]);
			}
		      else
			{
			    x = xx[i];
			    y = yy[i];
			}
		      if (ln->DimensionModel == GAIA_XY_Z
			  || ln->DimensionModel == GAIA_XY_Z_M)
			{
			    if (ignore_z)
				z = old_zz[i];
			    else
				z = zz[i];
			}
		      if (ln->DimensionModel == GAIA_XY_M
			  || ln->DimensionModel == GAIA_XY_Z_M)
			{
			    if (ignore_m)
				m = old_mm[i];
			    else
				m = mm[i];
			}
		      if (dst_ln->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (dst_ln->Coords, i, x, y, z);
			}
		      else if (dst_ln->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (dst_ln->Coords, i, x, y, m);
			}
		      else if (dst_ln->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (dst_ln->Coords, i, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (dst_ln->Coords, i, x, y);
			}
		  }
	    }
	  else
	      error = 1;
	  free (xx);
	  free (yy);
	  free (zz);
	  free (mm);
	  if (old_zz != NULL)
	      free (old_zz);
	  if (old_mm != NULL)
	      free (old_mm);
	  xx = NULL;
	  yy = NULL;
	  zz = NULL;
	  mm = NULL;
	  old_zz = NULL;
	  old_mm = NULL;
	  if (error)
	      goto stop;
	  ln = ln->Next;
      }
    pg = org->FirstPolygon;
    while (pg)
      {
	  /* reprojecting POLYGONs */
	  rng = pg->Exterior;
	  cnt = rng->Points;
	  dst_pg = gaiaAddPolygonToGeomColl (dst, cnt, pg->NumInteriors);
	  xx = malloc (sizeof (double) * cnt);
	  yy = malloc (sizeof (double) * cnt);
	  zz = malloc (sizeof (double) * cnt);
	  mm = malloc (sizeof (double) * cnt);
	  for (i = 0; i < cnt; i++)
	    {
		/* zeroing unused coordinates */
		if (ignore_z || org->DimensionModel == GAIA_XY
		    || org->DimensionModel == GAIA_XY_M)
		    zz[i] = 0.0;
		if (ignore_m || org->DimensionModel == GAIA_XY
		    || org->DimensionModel == GAIA_XY_Z)
		    mm[i] = 0.0;
	    }
	  if (ignore_z
	      && (org->DimensionModel == GAIA_XY_Z
		  || org->DimensionModel == GAIA_XY_Z_M))
	      old_zz = malloc (sizeof (double) * cnt);
	  if (ignore_m
	      && (org->DimensionModel == GAIA_XY_M
		  || org->DimensionModel == GAIA_XY_Z_M))
	      old_mm = malloc (sizeof (double) * cnt);
	  for (i = 0; i < cnt; i++)
	    {
		/* inserting points to be converted in temporary arrays [EXTERIOR RING] */
		z = 0.0;
		m = 0.0;
		if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, i, &x, &y, &z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, i, &x, &y, &m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, i, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, i, &x, &y);
		  }
		if (from_radians)
		  {
		      xx[i] = gaiaDegsToRads (x);
		      yy[i] = gaiaDegsToRads (y);
		  }
		else
		  {
		      xx[i] = x;
		      yy[i] = y;
		  }
		if (rng->DimensionModel == GAIA_XY_Z
		    || rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (ignore_z)
			  old_zz[i] = z;
		      else
			  zz[i] = z;
		  }
		if (rng->DimensionModel == GAIA_XY_M
		    || rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (ignore_m)
			  old_mm[i] = m;
		      else
			  mm[i] = m;
		  }
	    }
	  /* applying reprojection */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
	  result_count =
	      proj_trans_generic (from_to_cs, PJ_FWD, xx, sizeof (double), cnt,
				  yy, sizeof (double), cnt, zz, sizeof (double),
				  cnt, mm, sizeof (double), cnt);
	  if (result_count == cnt)
	      ret = 0;
	  else
	      ret = 1;
#else /* supporting old PROJ.4 */
	  ret = pj_transform (from_cs, to_cs, cnt, 0, xx, yy, zz);
#endif
	  if (ret == 0)
	    {
		/* inserting the reprojected POLYGON in the new GEOMETRY */
		dst_rng = dst_pg->Exterior;
		for (i = 0; i < cnt; i++)
		  {
		      /* setting EXTERIOR RING points */
		      if (to_radians)
			{
			    x = gaiaRadsToDegs (xx[i]);
			    y = gaiaRadsToDegs (yy[i]);
			}
		      else
			{
			    x = xx[i];
			    y = yy[i];
			}
		      if (rng->DimensionModel == GAIA_XY_Z
			  || rng->DimensionModel == GAIA_XY_Z_M)
			{
			    if (ignore_z)
				z = old_zz[i];
			    else
				z = zz[i];
			}
		      if (rng->DimensionModel == GAIA_XY_M
			  || rng->DimensionModel == GAIA_XY_Z_M)
			{
			    if (ignore_m)
				m = old_mm[i];
			    else
				m = mm[i];
			}
		      if (dst_rng->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (dst_rng->Coords, i, x, y, z);
			}
		      else if (dst_rng->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (dst_rng->Coords, i, x, y, m);
			}
		      else if (dst_rng->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (dst_rng->Coords, i, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (dst_rng->Coords, i, x, y);
			}
		  }
	    }
	  else
	      error = 1;
	  free (xx);
	  free (yy);
	  free (zz);
	  free (mm);
	  if (old_zz != NULL)
	      free (old_zz);
	  if (old_mm != NULL)
	      free (old_mm);
	  xx = NULL;
	  yy = NULL;
	  zz = NULL;
	  mm = NULL;
	  old_zz = NULL;
	  old_mm = NULL;
	  if (error)
	      goto stop;
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* processing INTERIOR RINGS */
		rng = pg->Interiors + ib;
		cnt = rng->Points;
		xx = malloc (sizeof (double) * cnt);
		yy = malloc (sizeof (double) * cnt);
		zz = malloc (sizeof (double) * cnt);
		mm = malloc (sizeof (double) * cnt);
		for (i = 0; i < cnt; i++)
		  {
		      /* zeroing unused coordinates */
		      if (ignore_z || org->DimensionModel == GAIA_XY
			  || org->DimensionModel == GAIA_XY_M)
			  zz[i] = 0.0;
		      if (ignore_m || org->DimensionModel == GAIA_XY
			  || org->DimensionModel == GAIA_XY_Z)
			  mm[i] = 0.0;
		  }
		if (ignore_z
		    && (org->DimensionModel == GAIA_XY_Z
			|| org->DimensionModel == GAIA_XY_Z_M))
		    old_zz = malloc (sizeof (double) * cnt);
		if (ignore_m
		    && (org->DimensionModel == GAIA_XY_M
			|| org->DimensionModel == GAIA_XY_Z_M))
		    old_mm = malloc (sizeof (double) * cnt);
		for (i = 0; i < cnt; i++)
		  {
		      /* inserting points to be converted in temporary arrays [INTERIOR RING] */
		      z = 0.0;
		      m = 0.0;
		      if (rng->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, i, &x, &y, &z);
			}
		      else if (rng->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, i, &x, &y, &m);
			}
		      else if (rng->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, i, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, i, &x, &y);
			}
		      if (from_radians)
			{
			    xx[i] = gaiaDegsToRads (x);
			    yy[i] = gaiaDegsToRads (y);
			}
		      else
			{
			    xx[i] = x;
			    yy[i] = y;
			}
		      if (rng->DimensionModel == GAIA_XY_Z
			  || rng->DimensionModel == GAIA_XY_Z_M)
			{
			    if (ignore_z)
				old_zz[i] = z;
			    else
				zz[i] = z;
			}
		      if (rng->DimensionModel == GAIA_XY_M
			  || rng->DimensionModel == GAIA_XY_Z_M)
			{
			    if (ignore_m)
				old_mm[i] = m;
			    else
				mm[i] = m;
			}
		  }
		/* applying reprojection */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
		result_count =
		    proj_trans_generic (from_to_cs, PJ_FWD, xx, sizeof (double),
					cnt, yy, sizeof (double), cnt, zz,
					sizeof (double), cnt, mm,
					sizeof (double), cnt);
		if (result_count == cnt)
		    ret = 0;
		else
		    ret = 1;
#else /* supporting old PROJ.4 */
		ret = pj_transform (from_cs, to_cs, cnt, 0, xx, yy, zz);
#endif
		if (ret == 0)
		  {
		      /* inserting the reprojected POLYGON in the new GEOMETRY */
		      dst_rng = gaiaAddInteriorRing (dst_pg, ib, cnt);
		      for (i = 0; i < cnt; i++)
			{
			    /* setting INTERIOR RING points */
			    if (to_radians)
			      {
				  x = gaiaRadsToDegs (xx[i]);
				  y = gaiaRadsToDegs (yy[i]);
			      }
			    else
			      {
				  x = xx[i];
				  y = yy[i];
			      }
			    if (rng->DimensionModel == GAIA_XY_Z
				|| rng->DimensionModel == GAIA_XY_Z_M)
			      {
				  if (ignore_z)
				      z = old_zz[i];
				  else
				      z = zz[i];
			      }
			    if (rng->DimensionModel == GAIA_XY_M
				|| rng->DimensionModel == GAIA_XY_Z_M)
			      {
				  if (ignore_m)
				      m = old_mm[i];
				  else
				      m = mm[i];
			      }
			    if (dst_rng->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (dst_rng->Coords, i, x, y, z);
			      }
			    else if (dst_rng->DimensionModel == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (dst_rng->Coords, i, x, y, m);
			      }
			    else if (dst_rng->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (dst_rng->Coords, i, x, y, z,
						    m);
			      }
			    else
			      {
				  gaiaSetPoint (dst_rng->Coords, i, x, y);
			      }
			}
		  }
		else
		    error = 1;
		free (xx);
		free (yy);
		free (zz);
		free (mm);
		if (old_zz != NULL)
		    free (old_zz);
		if (old_mm != NULL)
		    free (old_mm);
		xx = NULL;
		yy = NULL;
		zz = NULL;
		mm = NULL;
		old_zz = NULL;
		old_mm = NULL;
		if (error)
		    goto stop;
	    }
	  pg = pg->Next;
      }
  stop:
#endif
    return error;
}

static gaiaGeomCollPtr
gaiaTransformCommon (void *x_handle, const void *p_cache, gaiaGeomCollPtr org,
		     const char *proj_string_1,
		     const char *proj_string_2, gaiaProjAreaPtr proj_bbox,
		     int ignore_z, int ignore_m)
{
/* creates a new GEOMETRY reprojecting coordinates from the original one */
    int error = 0;
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    PJ_CONTEXT *handle = (PJ_CONTEXT *) x_handle;
    PJ *from_to_pre;
    PJ *from_to_cs;
    int proj_is_cached = 0;
#else /* supporting old PROJ.4 */
    if (p_cache == NULL)
	p_cache = NULL;		/* silencing stupid compiler warnings about unused args */
    projCtx handle = (projCtx) x_handle;
    projPJ from_cs;
    projPJ to_cs;
#endif
    int from_radians;
    int to_radians;
    gaiaGeomCollPtr dst;

/* preliminary validity check */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    gaiaResetProjErrorMsg_r (p_cache);
#endif
    if (proj_bbox == NULL)
	proj_bbox = NULL;	/* silencing stupid compiler warnings about unused args */
    if (proj_string_1 == NULL)
	return NULL;

#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    if (gaiaCurrentCachedProjMatches
	(p_cache, proj_string_1, proj_string_2, proj_bbox))
      {
	  from_to_cs = gaiaGetCurrentCachedProj (p_cache);
	  if (from_to_cs != NULL)
	    {
		proj_is_cached = 1;
		goto skip_cached;
	    }
      }
    if (proj_string_2 != NULL)
      {
	  PJ_AREA *area = NULL;
	  if (proj_bbox != NULL)
	    {
		area = proj_area_create ();
		proj_area_set_bbox (area, proj_bbox->WestLongitude,
				    proj_bbox->SouthLatitude,
				    proj_bbox->EastLongitude,
				    proj_bbox->NorthLatitude);
	    }
	  from_to_pre =
	      proj_create_crs_to_crs (handle, proj_string_1, proj_string_2,
				      area);
	  if (!from_to_pre)
	      return NULL;
	  from_to_cs = proj_normalize_for_visualization (handle, from_to_pre);
	  proj_destroy (from_to_pre);
	  if (area != NULL)
	      proj_area_destroy (area);
	  if (!from_to_cs)
	      return NULL;
	  proj_is_cached =
	      gaiaSetCurrentCachedProj (p_cache, from_to_cs, proj_string_1,
					proj_string_2, proj_bbox);
      }
    else			/* PROJ_STRING - PIPELINE */
      {
	  from_to_cs = proj_create (handle, proj_string_1);
	  if (!from_to_cs)
	      return NULL;
	  proj_is_cached =
	      gaiaSetCurrentCachedProj (p_cache, from_to_cs, proj_string_1,
					NULL, NULL);
      }
  skip_cached:
#else /* supporting old PROJ.4 */
    if (proj_string_2 == NULL)
	return NULL;
    if (handle != NULL)
      {
	  from_cs = pj_init_plus_ctx (handle, proj_string_1);
	  to_cs = pj_init_plus_ctx (handle, proj_string_2);
      }
    else
      {
	  from_cs = pj_init_plus (proj_string_1);
	  to_cs = pj_init_plus (proj_string_2);
      }
    if (!from_cs)
      {
	  if (to_cs)
	      pj_free (to_cs);
	  return NULL;
      }
    if (!to_cs)
      {
	  pj_free (from_cs);
	  return NULL;
      }
#endif
    if (org->DimensionModel == GAIA_XY_Z)
	dst = gaiaAllocGeomCollXYZ ();
    else if (org->DimensionModel == GAIA_XY_M)
	dst = gaiaAllocGeomCollXYM ();
    else if (org->DimensionModel == GAIA_XY_Z_M)
	dst = gaiaAllocGeomCollXYZM ();
    else
	dst = gaiaAllocGeomColl ();
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    from_radians = proj_angular_input (from_to_cs, PJ_FWD);
    to_radians = proj_angular_output (from_to_cs, PJ_FWD);
    error =
	do_transfom_proj (org, dst, ignore_z, ignore_m, from_radians,
			  to_radians, NULL, NULL, from_to_cs);
#else /* supporting old PROJ.4 */
    from_radians = gaiaIsLongLat (proj_string_1);
    to_radians = gaiaIsLongLat (proj_string_2);
    error =
	do_transfom_proj (org, dst, ignore_z, ignore_m, from_radians,
			  to_radians, from_cs, to_cs, NULL);
#endif

/* destroying the PROJ4 params */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    if (!proj_is_cached)
	proj_destroy (from_to_cs);
#else /* supporting old PROJ.4 */
    pj_free (from_cs);
    pj_free (to_cs);
#endif
    if (error)
      {
	  /* some error occurred */
	  gaiaPointPtr pP;
	  gaiaPointPtr pPn;
	  gaiaLinestringPtr pL;
	  gaiaLinestringPtr pLn;
	  gaiaPolygonPtr pA;
	  gaiaPolygonPtr pAn;
	  pP = dst->FirstPoint;
	  while (pP != NULL)
	    {
		pPn = pP->Next;
		gaiaFreePoint (pP);
		pP = pPn;
	    }
	  pL = dst->FirstLinestring;
	  while (pL != NULL)
	    {
		pLn = pL->Next;
		gaiaFreeLinestring (pL);
		pL = pLn;
	    }
	  pA = dst->FirstPolygon;
	  while (pA != NULL)
	    {
		pAn = pA->Next;
		gaiaFreePolygon (pA);
		pA = pAn;
	    }
	  dst->FirstPoint = NULL;
	  dst->LastPoint = NULL;
	  dst->FirstLinestring = NULL;
	  dst->LastLinestring = NULL;
	  dst->FirstPolygon = NULL;
	  dst->LastPolygon = NULL;
      }
    if (dst)
      {
	  gaiaMbrGeometry (dst);
	  dst->DeclaredType = org->DeclaredType;
      }
    return dst;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaTransform (gaiaGeomCollPtr org, const char *proj_from, const char *proj_to)
{
    return gaiaTransformEx (org, proj_from, proj_to, NULL);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaTransform_r (const void *p_cache, gaiaGeomCollPtr org,
		 const char *proj_from, const char *proj_to)
{
    return gaiaTransformEx_r (p_cache, org, proj_from, proj_to, NULL);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaTransformEx (gaiaGeomCollPtr org, const char *proj_string_1,
		 const char *proj_string_2, gaiaProjAreaPtr proj_bbox)
{
    return gaiaTransformCommon (NULL, NULL, org, proj_string_1, proj_string_2,
				proj_bbox, 0, 0);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaTransformEx_r (const void *p_cache, gaiaGeomCollPtr org,
		   const char *proj_string_1, const char *proj_string_2,
		   gaiaProjAreaPtr proj_bbox)
{
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    void *handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->PROJ_handle;
    if (handle == NULL)
	return NULL;
    return gaiaTransformCommon (handle, cache, org, proj_string_1,
				proj_string_2, proj_bbox, 0, 0);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaTransformXY (gaiaGeomCollPtr org, const char *proj_from,
		 const char *proj_to)
{
    return gaiaTransformCommon (NULL, NULL, org,
				proj_from, proj_to, NULL, 1, 1);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaTransformXY_r (const void *p_cache, gaiaGeomCollPtr org,
		   const char *proj_from, const char *proj_to)
{
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    void *handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->PROJ_handle;
    if (handle == NULL)
	return NULL;
    return gaiaTransformCommon (handle, cache, org,
				proj_from, proj_to, NULL, 1, 1);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaTransformXYZ (gaiaGeomCollPtr org, const char *proj_from,
		  const char *proj_to)
{
    return gaiaTransformCommon (NULL, NULL, org,
				proj_from, proj_to, NULL, 0, 1);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaTransformXYZ_r (const void *p_cache, gaiaGeomCollPtr org,
		    const char *proj_from, const char *proj_to)
{
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    void *handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->PROJ_handle;
    if (handle == NULL)
	return NULL;
    return gaiaTransformCommon (handle, cache, org,
				proj_from, proj_to, NULL, 0, 1);
}

#ifdef PROJ_NEW			/* only if new PROJ.6 is supported */
GAIAGEO_DECLARE char *
gaiaGetProjString (const void *p_cache, const char *auth_name, int auth_srid)
{
/* return the proj-string expression corresponding to some CRS */
    PJ *crs_def;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    const char *proj_string;
    int len;
    char *result;
    char xsrid[64];

    sprintf (xsrid, "%d", auth_srid);
    crs_def =
	proj_create_from_database (cache->PROJ_handle, auth_name, xsrid,
				   PJ_CATEGORY_CRS, 0, NULL);
    if (crs_def == NULL)
	return NULL;
    proj_string =
	proj_as_proj_string (cache->PROJ_handle, crs_def, PJ_PROJ_5, NULL);
    if (proj_string == NULL)
      {
	  proj_destroy (crs_def);
	  return NULL;
      }
    len = strlen (proj_string);
    result = malloc (len + 1);
    strcpy (result, proj_string);
    proj_destroy (crs_def);
    return result;
}

GAIAGEO_DECLARE char *
gaiaGetProjWKT (const void *p_cache, const char *auth_name, int auth_srid,
		int style, int indented, int indentation)
{
/* return the WKT expression corresponding to some CRS */
    PJ *crs_def;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    int proj_style;
    const char *wkt;
    int len;
    char *result;
    char xsrid[64];
    char dummy[64];
    char *options[4];
    options[1] = dummy;
    options[2] = "OUTPUT_AXIS=AUTO";
    options[3] = NULL;

    sprintf (xsrid, "%d", auth_srid);
    crs_def =
	proj_create_from_database (cache->PROJ_handle, auth_name, xsrid,
				   PJ_CATEGORY_CRS, 0, NULL);
    if (crs_def == NULL)
	return NULL;

    switch (style)
      {
      case GAIA_PROJ_WKT_GDAL:
	  proj_style = PJ_WKT1_GDAL;
	  break;
      case GAIA_PROJ_WKT_ESRI:
	  proj_style = PJ_WKT1_ESRI;
	  break;
      case GAIA_PROJ_WKT_ISO_2015:
	  proj_style = PJ_WKT2_2015;
	  break;
      default:
	  proj_style = PJ_WKT2_2015;
	  break;
      };
    if (indented)
	options[0] = "MULTILINE=YES";
    else
	options[0] = "MULTILINE=NO";
    if (indentation < 1)
	indentation = 1;
    if (indentation > 8)
	indentation = 8;
    sprintf (dummy, "INDENTATION_WIDTH=%d", indentation);
    wkt =
	proj_as_wkt (cache->PROJ_handle, crs_def, proj_style,
		     (const char *const *) options);
    if (wkt == NULL)
      {
	  proj_destroy (crs_def);
	  return NULL;
      }
    len = strlen (wkt);
    result = malloc (len + 1);
    strcpy (result, wkt);
    proj_destroy (crs_def);
    return result;
}

GAIAGEO_DECLARE int
gaiaGuessSridFromWKT (sqlite3 * sqlite, const void *p_cache, const char *wkt,
		      int *srid)
{
/* return the SRID value corresponding to a given WKT expression */
    PJ *crs1 = NULL;
    PJ *crs2 = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    sqlite3_stmt *stmt = NULL;
    int ret;
    const char *sql;
    int xsrid = -1;

/* sanity check */
    if (cache == NULL)
	goto error;
    if (cache->PROJ_handle == NULL)
	goto error;

/* attempting to parse the WKT expression */
    crs1 = proj_create_from_wkt (cache->PROJ_handle, wkt, NULL, NULL, NULL);
    if (crs1 == NULL)
      {
	  spatialite_e
	      ("gaiaGuessSridFromWKT: invalid/malformed WKT expression\n");
	  goto error;
      }

    sql = "SELECT srid, Upper(auth_name), auth_srid FROM spatial_ref_sys";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("gaiaGuessSridFromWKT: %s\n", sqlite3_errmsg (sqlite));
	  goto error;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		char dummy[64];
		int srid = sqlite3_column_int (stmt, 0);
		const char *auth_name =
		    (const char *) sqlite3_column_text (stmt, 1);
		int auth_srid = sqlite3_column_int (stmt, 2);
		sprintf (dummy, "%d", auth_srid);
		/* parsing some CRS */
		crs2 =
		    proj_create_from_database (cache->PROJ_handle, auth_name,
					       dummy, PJ_CATEGORY_CRS, 0, NULL);
		if (crs2 != NULL)
		  {
		      /* ok, it's a valid CRS - comparing */
		      if (proj_is_equivalent_to
			  (crs1, crs2,
			   PJ_COMP_EQUIVALENT_EXCEPT_AXIS_ORDER_GEOGCRS))
			{
			    xsrid = srid;
			    proj_destroy (crs2);
			    break;
			}
		      proj_destroy (crs2);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    proj_destroy (crs1);
    *srid = xsrid;
    gaiaResetProjErrorMsg_r (cache);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (crs1 != NULL)
	proj_destroy (crs1);
    *srid = xsrid;
    return 0;
}
#endif

#endif /* end including PROJ.4 */
