/*

 gg_lwgeom.c -- Gaia LWGEOM support
    
 version 4.0, 2012 August 19

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
 
Portions created by the Initial Developer are Copyright (C) 2012
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

/*
 
CREDITS:

this module (wrapping liblwgeom APIs) has been entierely funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale

*/

#include "config.h"

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>
#include <spatialite/gaiageo.h>
#include <spatialite.h>
#include <spatialite_private.h>

#ifdef ENABLE_LWGEOM		/* enabling LWGEOM support */

#include <liblwgeom.h>

const char splitelwgeomversion[] = LIBLWGEOM_VERSION;

SPATIALITE_PRIVATE const char *
splite_lwgeom_version (void)
{
    return splitelwgeomversion;
}

void
lwgeom_init_allocators (void)
{
/* Set up liblwgeom to run in stand-alone mode using the
* usual system memory handling functions. */
    lwalloc_var = default_allocator;
    lwrealloc_var = default_reallocator;
    lwfree_var = default_freeor;
    lwnotice_var = default_noticereporter;
    lwerror_var = default_errorreporter;
}

static LWGEOM *
toLWGeom (const gaiaGeomCollPtr gaia)
{
/* converting a GAIA Geometry into a LWGEOM Geometry */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int has_z;
    int has_m;
    int ngeoms;
    int numg;
    int ib;
    int iv;
    int type;
    double x;
    double y;
    double z;
    double m;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    POINTARRAY *pa;
    POINTARRAY **ppaa;
    POINT4D point;
    LWGEOM **geoms;

    if (!gaia)
	return NULL;
    pt = gaia->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = gaia->FirstLinestring;
    while (ln)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGONs are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 0 && pgs == 0)
	return NULL;

    if (pts == 1 && lns == 0 && pgs == 0)
      {
	  /* single Point */
	  pt = gaia->FirstPoint;
	  has_z = 0;
	  has_m = 0;
	  if (gaia->DimensionModel == GAIA_XY_Z
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_z = 1;
	  if (gaia->DimensionModel == GAIA_XY_M
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_m = 1;
	  pa = ptarray_construct (has_z, has_m, 1);
	  point.x = pt->X;
	  point.y = pt->Y;
	  if (has_z)
	      point.z = pt->Z;
	  if (has_m)
	      point.m = pt->M;
	  ptarray_set_point4d (pa, 0, &point);
	  return (LWGEOM *) lwpoint_construct (gaia->Srid, NULL, pa);
      }
    else if (pts == 0 && lns == 1 && pgs == 0)
      {
	  /* single Linestring */
	  ln = gaia->FirstLinestring;
	  has_z = 0;
	  has_m = 0;
	  if (gaia->DimensionModel == GAIA_XY_Z
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_z = 1;
	  if (gaia->DimensionModel == GAIA_XY_M
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_m = 1;
	  pa = ptarray_construct (has_z, has_m, ln->Points);
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		/* copying vertices */
		if (gaia->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		  }
		else if (gaia->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		  }
		else if (gaia->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		if (has_m)
		    point.m = m;
		ptarray_set_point4d (pa, iv, &point);
	    }
	  return (LWGEOM *) lwline_construct (gaia->Srid, NULL, pa);
      }
    else if (pts == 0 && lns == 0 && pgs == 1)
      {
	  /* single Polygon */
	  pg = gaia->FirstPolygon;
	  has_z = 0;
	  has_m = 0;
	  if (gaia->DimensionModel == GAIA_XY_Z
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_z = 1;
	  if (gaia->DimensionModel == GAIA_XY_M
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_m = 1;
	  ngeoms = pg->NumInteriors;
	  ppaa = lwalloc (sizeof (POINTARRAY *) * (ngeoms + 1));
	  rng = pg->Exterior;
	  ppaa[0] = ptarray_construct (has_z, has_m, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* copying vertices - Exterior Ring */
		if (gaia->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		  }
		else if (gaia->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		  }
		else if (gaia->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		if (has_m)
		    point.m = m;
		ptarray_set_point4d (ppaa[0], iv, &point);
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* copying vertices - Interior Rings */
		rng = pg->Interiors + ib;
		ppaa[1 + ib] = ptarray_construct (has_z, has_m, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      if (gaia->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			}
		      else if (gaia->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			}
		      else if (gaia->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			}
		      point.x = x;
		      point.y = y;
		      if (has_z)
			  point.z = z;
		      if (has_m)
			  point.m = m;
		      ptarray_set_point4d (ppaa[1 + ib], iv, &point);
		  }
	    }
	  return (LWGEOM *) lwpoly_construct (gaia->Srid, NULL, ngeoms + 1,
					      ppaa);
      }
    else
      {
	  /* some Collection */
	  switch (gaia->DeclaredType)
	    {
	    case GAIA_POINT:
		type = POINTTYPE;
		break;
	    case GAIA_LINESTRING:
		type = LINETYPE;
		break;
	    case GAIA_POLYGON:
		type = POLYGONTYPE;
		break;
	    case GAIA_MULTIPOINT:
		type = MULTIPOINTTYPE;
		break;
	    case GAIA_MULTILINESTRING:
		type = MULTILINETYPE;
		break;
	    case GAIA_MULTIPOLYGON:
		type = MULTIPOLYGONTYPE;
		break;
	    case GAIA_GEOMETRYCOLLECTION:
		type = COLLECTIONTYPE;
		break;
	    default:
		if (lns == 0 && pgs == 0)
		    type = MULTIPOINTTYPE;
		else if (pts == 0 && pgs == 0)
		    type = MULTILINETYPE;
		else if (pts == 0 && lns == 0)
		    type = MULTIPOLYGONTYPE;
		else
		    type = COLLECTIONTYPE;
		break;
	    };
	  numg = pts + lns + pgs;
	  geoms = lwalloc (sizeof (LWGEOM *) * numg);

	  numg = 0;
	  pt = gaia->FirstPoint;
	  while (pt)
	    {
		/* copying POINTs */
		has_z = 0;
		has_m = 0;
		if (gaia->DimensionModel == GAIA_XY_Z
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_z = 1;
		if (gaia->DimensionModel == GAIA_XY_M
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_m = 1;
		pa = ptarray_construct (has_z, has_m, 1);
		point.x = pt->X;
		point.y = pt->Y;
		if (has_z)
		    point.z = pt->Z;
		if (has_m)
		    point.m = pt->M;
		ptarray_set_point4d (pa, 0, &point);
		geoms[numg++] =
		    (LWGEOM *) lwpoint_construct (gaia->Srid, NULL, pa);
		pt = pt->Next;
	    }
	  ln = gaia->FirstLinestring;
	  while (ln)
	    {
		/* copying LINESTRINGs */
		has_z = 0;
		has_m = 0;
		if (gaia->DimensionModel == GAIA_XY_Z
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_z = 1;
		if (gaia->DimensionModel == GAIA_XY_M
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_m = 1;
		pa = ptarray_construct (has_z, has_m, ln->Points);
		for (iv = 0; iv < ln->Points; iv++)
		  {
		      /* copying vertices */
		      if (gaia->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
			}
		      else if (gaia->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
			}
		      else if (gaia->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ln->Coords, iv, &x, &y);
			}
		      point.x = x;
		      point.y = y;
		      if (has_z)
			  point.z = z;
		      if (has_m)
			  point.m = m;
		      ptarray_set_point4d (pa, iv, &point);
		  }
		geoms[numg++] =
		    (LWGEOM *) lwline_construct (gaia->Srid, NULL, pa);
		ln = ln->Next;
	    }
	  pg = gaia->FirstPolygon;
	  while (pg)
	    {
		/* copying POLYGONs */
		has_z = 0;
		has_m = 0;
		if (gaia->DimensionModel == GAIA_XY_Z
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_z = 1;
		if (gaia->DimensionModel == GAIA_XY_M
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_m = 1;
		ngeoms = pg->NumInteriors;
		ppaa = lwalloc (sizeof (POINTARRAY *) * (ngeoms + 1));
		rng = pg->Exterior;
		ppaa[0] = ptarray_construct (has_z, has_m, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* copying vertices - Exterior Ring */
		      if (gaia->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			}
		      else if (gaia->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			}
		      else if (gaia->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			}
		      point.x = x;
		      point.y = y;
		      if (has_z)
			  point.z = z;
		      if (has_m)
			  point.m = m;
		      ptarray_set_point4d (ppaa[0], iv, &point);
		  }
		for (ib = 0; ib < pg->NumInteriors; ib++)
		  {
		      /* copying vertices - Interior Rings */
		      rng = pg->Interiors + ib;
		      ppaa[1 + ib] =
			  ptarray_construct (has_z, has_m, rng->Points);
		      for (iv = 0; iv < rng->Points; iv++)
			{
			    if (gaia->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			      }
			    else if (gaia->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			      }
			    else if (gaia->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (rng->Coords, iv, &x, &y,
						    &z, &m);
			      }
			    else
			      {
				  gaiaGetPoint (rng->Coords, iv, &x, &y);
			      }
			    point.x = x;
			    point.y = y;
			    if (has_z)
				point.z = z;
			    if (has_m)
				point.m = m;
			    ptarray_set_point4d (ppaa[1 + ib], iv, &point);
			}
		  }
		geoms[numg++] =
		    (LWGEOM *) lwpoly_construct (gaia->Srid, NULL, ngeoms + 1,
						 ppaa);
		pg = pg->Next;
	    }
	  return (LWGEOM *) lwcollection_construct (type, gaia->Srid, NULL,
						    numg, geoms);
      }
    return NULL;
}

static gaiaGeomCollPtr
fromLWGeomIncremental (gaiaGeomCollPtr gaia, const LWGEOM * lwgeom)
{
/* converting a LWGEOM Geometry into a GAIA Geometry */
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int dimension_model = gaia->DimensionModel;
    int declared_type = gaia->DeclaredType;
    LWGEOM *lwg2 = NULL;
    LWPOINT *lwp = NULL;
    LWLINE *lwl = NULL;
    LWPOLY *lwpoly = NULL;
    LWCOLLECTION *lwc = NULL;
    POINTARRAY *pa;
    POINT4D pt4d;
    int has_z;
    int has_m;
    int iv;
    int ib;
    int ngeoms;
    int ng;
    double x;
    double y;
    double z;
    double m;

    if (lwgeom == NULL)
	return NULL;
    if (lwgeom_is_empty (lwgeom))
	return NULL;

    switch (lwgeom->type)
      {
      case POINTTYPE:
	  lwp = (LWPOINT *) lwgeom;
	  has_z = 0;
	  has_m = 0;
	  pa = lwp->point;
	  if (FLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  if (FLAGS_GET_M (pa->flags))
	      has_m = 1;
	  getPoint4d_p (pa, 0, &pt4d);
	  x = pt4d.x;
	  y = pt4d.y;
	  if (has_z)
	      z = pt4d.z;
	  else
	      z = 0.0;
	  if (has_m)
	      m = pt4d.m;
	  else
	      m = 0.0;
	  if (dimension_model == GAIA_XY_Z)
	      gaiaAddPointToGeomCollXYZ (gaia, x, y, z);
	  else if (dimension_model == GAIA_XY_M)
	      gaiaAddPointToGeomCollXYM (gaia, x, y, m);
	  else if (dimension_model == GAIA_XY_Z_M)
	      gaiaAddPointToGeomCollXYZM (gaia, x, y, z, m);
	  else
	      gaiaAddPointToGeomColl (gaia, x, y);
	  if (declared_type == GAIA_MULTIPOINT)
	      gaia->DeclaredType = GAIA_MULTIPOINT;
	  else if (declared_type == GAIA_GEOMETRYCOLLECTION)
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      gaia->DeclaredType = GAIA_POINT;
	  break;
      case LINETYPE:
	  lwl = (LWLINE *) lwgeom;
	  has_z = 0;
	  has_m = 0;
	  pa = lwl->points;
	  if (FLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  if (FLAGS_GET_M (pa->flags))
	      has_m = 1;
	  ln = gaiaAddLinestringToGeomColl (gaia, pa->npoints);
	  for (iv = 0; iv < pa->npoints; iv++)
	    {
		/* copying LINESTRING vertices */
		getPoint4d_p (pa, iv, &pt4d);
		x = pt4d.x;
		y = pt4d.y;
		if (has_z)
		    z = pt4d.z;
		else
		    z = 0.0;
		if (has_m)
		    m = pt4d.m;
		else
		    m = 0.0;
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
		  }
		else if (dimension_model == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ln->Coords, iv, x, y, m);
		  }
		else if (dimension_model == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ln->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ln->Coords, iv, x, y);
		  }
	    }
	  if (declared_type == GAIA_MULTILINESTRING)
	      gaia->DeclaredType = GAIA_MULTILINESTRING;
	  else if (declared_type == GAIA_GEOMETRYCOLLECTION)
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      gaia->DeclaredType = GAIA_LINESTRING;
	  break;
      case POLYGONTYPE:
	  lwpoly = (LWPOLY *) lwgeom;
	  has_z = 0;
	  has_m = 0;
	  pa = lwpoly->rings[0];
	  if (FLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  if (FLAGS_GET_M (pa->flags))
	      has_m = 1;
	  pg = gaiaAddPolygonToGeomColl (gaia, pa->npoints, lwpoly->nrings - 1);
	  rng = pg->Exterior;
	  for (iv = 0; iv < pa->npoints; iv++)
	    {
		/* copying Exterion Ring vertices */
		getPoint4d_p (pa, iv, &pt4d);
		x = pt4d.x;
		y = pt4d.y;
		if (has_z)
		    z = pt4d.z;
		else
		    z = 0.0;
		if (has_m)
		    m = pt4d.m;
		else
		    m = 0.0;
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
		  }
		else if (dimension_model == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (rng->Coords, iv, x, y, m);
		  }
		else if (dimension_model == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (rng->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (rng->Coords, iv, x, y);
		  }
	    }
	  for (ib = 1; ib < lwpoly->nrings; ib++)
	    {
		has_z = 0;
		has_m = 0;
		pa = lwpoly->rings[ib];
		if (FLAGS_GET_Z (pa->flags))
		    has_z = 1;
		if (FLAGS_GET_M (pa->flags))
		    has_m = 1;
		rng = gaiaAddInteriorRing (pg, ib - 1, pa->npoints);
		for (iv = 0; iv < pa->npoints; iv++)
		  {
		      /* copying Exterion Ring vertices */
		      getPoint4d_p (pa, iv, &pt4d);
		      x = pt4d.x;
		      y = pt4d.y;
		      if (has_z)
			  z = pt4d.z;
		      else
			  z = 0.0;
		      if (has_m)
			  m = pt4d.m;
		      else
			  m = 0.0;
		      if (dimension_model == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
			}
		      else if (dimension_model == GAIA_XY_M)
			{
			    gaiaSetPointXYM (rng->Coords, iv, x, y, m);
			}
		      else if (dimension_model == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (rng->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (rng->Coords, iv, x, y);
			}
		  }
	    }
	  if (declared_type == GAIA_MULTIPOLYGON)
	      gaia->DeclaredType = GAIA_MULTIPOLYGON;
	  else if (declared_type == GAIA_GEOMETRYCOLLECTION)
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      gaia->DeclaredType = GAIA_POLYGON;
	  break;
      case MULTIPOINTTYPE:
      case MULTILINETYPE:
      case MULTIPOLYGONTYPE:
      case COLLECTIONTYPE:
	  if (lwgeom->type == MULTIPOINTTYPE)
	    {
		if (declared_type == GAIA_GEOMETRYCOLLECTION)
		    gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
		else
		    gaia->DeclaredType = GAIA_MULTIPOINT;
	    }
	  else if (lwgeom->type == MULTILINETYPE)
	    {
		if (declared_type == GAIA_GEOMETRYCOLLECTION)
		    gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
		else
		    gaia->DeclaredType = GAIA_MULTILINESTRING;
	    }
	  else if (lwgeom->type == MULTIPOLYGONTYPE)
	    {
		if (declared_type == GAIA_GEOMETRYCOLLECTION)
		    gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
		else
		    gaia->DeclaredType = GAIA_MULTIPOLYGON;
	    }
	  else
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;

	  lwc = (LWCOLLECTION *) lwgeom;
	  ngeoms = lwc->ngeoms;
	  if (ngeoms == 0)
	    {
		gaiaFreeGeomColl (gaia);
		gaia = NULL;
		break;
	    }
	  for (ng = 0; ng < ngeoms; ++ng)
	    {
		/* looping on elementary geometries */
		lwg2 = lwc->geoms[ng];
		switch (lwg2->type)
		  {
		  case POINTTYPE:
		      lwp = (LWPOINT *) lwg2;
		      has_z = 0;
		      has_m = 0;
		      pa = lwp->point;
		      if (FLAGS_GET_Z (pa->flags))
			  has_z = 1;
		      if (FLAGS_GET_M (pa->flags))
			  has_m = 1;
		      getPoint4d_p (pa, 0, &pt4d);
		      x = pt4d.x;
		      y = pt4d.y;
		      if (has_z)
			  z = pt4d.z;
		      else
			  z = 0.0;
		      if (has_m)
			  m = pt4d.m;
		      else
			  m = 0.0;
		      if (dimension_model == GAIA_XY_Z)
			  gaiaAddPointToGeomCollXYZ (gaia, x, y, z);
		      else if (dimension_model == GAIA_XY_M)
			  gaiaAddPointToGeomCollXYM (gaia, x, y, m);
		      else if (dimension_model == GAIA_XY_Z_M)
			  gaiaAddPointToGeomCollXYZM (gaia, x, y, z, m);
		      else
			  gaiaAddPointToGeomColl (gaia, x, y);
		      break;
		  case LINETYPE:
		      lwl = (LWLINE *) lwg2;
		      has_z = 0;
		      has_m = 0;
		      pa = lwl->points;
		      if (FLAGS_GET_Z (pa->flags))
			  has_z = 1;
		      if (FLAGS_GET_M (pa->flags))
			  has_m = 1;
		      ln = gaiaAddLinestringToGeomColl (gaia, pa->npoints);
		      for (iv = 0; iv < pa->npoints; iv++)
			{
			    /* copying LINESTRING vertices */
			    getPoint4d_p (pa, iv, &pt4d);
			    x = pt4d.x;
			    y = pt4d.y;
			    if (has_z)
				z = pt4d.z;
			    else
				z = 0.0;
			    if (has_m)
				m = pt4d.m;
			    else
				m = 0.0;
			    if (dimension_model == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
			      }
			    else if (dimension_model == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (ln->Coords, iv, x, y, m);
			      }
			    else if (dimension_model == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (ln->Coords, iv, x, y, z, m);
			      }
			    else
			      {
				  gaiaSetPoint (ln->Coords, iv, x, y);
			      }
			}
		      break;
		  case POLYGONTYPE:
		      lwpoly = (LWPOLY *) lwg2;
		      has_z = 0;
		      has_m = 0;
		      pa = lwpoly->rings[0];
		      if (FLAGS_GET_Z (pa->flags))
			  has_z = 1;
		      if (FLAGS_GET_M (pa->flags))
			  has_m = 1;
		      pg = gaiaAddPolygonToGeomColl (gaia, pa->npoints,
						     lwpoly->nrings - 1);
		      rng = pg->Exterior;
		      for (iv = 0; iv < pa->npoints; iv++)
			{
			    /* copying Exterion Ring vertices */
			    getPoint4d_p (pa, iv, &pt4d);
			    x = pt4d.x;
			    y = pt4d.y;
			    if (has_z)
				z = pt4d.z;
			    else
				z = 0.0;
			    if (has_m)
				m = pt4d.m;
			    else
				m = 0.0;
			    if (dimension_model == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
			      }
			    else if (dimension_model == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (rng->Coords, iv, x, y, m);
			      }
			    else if (dimension_model == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (rng->Coords, iv, x, y, z,
						    m);
			      }
			    else
			      {
				  gaiaSetPoint (rng->Coords, iv, x, y);
			      }
			}
		      for (ib = 1; ib < lwpoly->nrings; ib++)
			{
			    has_z = 0;
			    has_m = 0;
			    pa = lwpoly->rings[ib];
			    if (FLAGS_GET_Z (pa->flags))
				has_z = 1;
			    if (FLAGS_GET_M (pa->flags))
				has_m = 1;
			    rng = gaiaAddInteriorRing (pg, ib - 1, pa->npoints);
			    for (iv = 0; iv < pa->npoints; iv++)
			      {
				  /* copying Exterion Ring vertices */
				  getPoint4d_p (pa, iv, &pt4d);
				  x = pt4d.x;
				  y = pt4d.y;
				  if (has_z)
				      z = pt4d.z;
				  else
				      z = 0.0;
				  if (has_m)
				      m = pt4d.m;
				  else
				      m = 0.0;
				  if (dimension_model == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (rng->Coords, iv, x,
							 y, z);
				    }
				  else if (dimension_model == GAIA_XY_M)
				    {
					gaiaSetPointXYM (rng->Coords, iv, x,
							 y, m);
				    }
				  else if (dimension_model == GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (rng->Coords, iv, x,
							  y, z, m);
				    }
				  else
				    {
					gaiaSetPoint (rng->Coords, iv, x, y);
				    }
			      }
			}
		      break;
		  };
	    }
	  break;
      default:
	  gaiaFreeGeomColl (gaia);
	  gaia = NULL;
	  break;
      };

    return gaia;
}

static gaiaGeomCollPtr
fromLWGeom (const LWGEOM * lwgeom, const int dimension_model,
	    const int declared_type)
{
/* converting a LWGEOM Geometry into a GAIA Geometry */
    gaiaGeomCollPtr gaia = NULL;

    if (lwgeom == NULL)
	return NULL;
    if (lwgeom_is_empty (lwgeom))
	return NULL;

    if (dimension_model == GAIA_XY_Z)
	gaia = gaiaAllocGeomCollXYZ ();
    else if (dimension_model == GAIA_XY_M)
	gaia = gaiaAllocGeomCollXYM ();
    else if (dimension_model == GAIA_XY_Z_M)
	gaia = gaiaAllocGeomCollXYZM ();
    else
	gaia = gaiaAllocGeomColl ();
    gaia->DeclaredType = declared_type;
    fromLWGeomIncremental (gaia, lwgeom);

    return gaia;
}

static gaiaGeomCollPtr
fromLWGeomValidated (const LWGEOM * lwgeom, const int dimension_model,
		     const int declared_type)
{
/* 
/ converting a LWGEOM Geometry into a GAIA Geometry 
/ first collection - validated items
*/
    gaiaGeomCollPtr gaia = NULL;
    LWGEOM *lwg2 = NULL;
    LWCOLLECTION *lwc = NULL;
    int ngeoms;

    if (lwgeom == NULL)
	return NULL;
    if (lwgeom_is_empty (lwgeom))
	return NULL;

    switch (lwgeom->type)
      {
      case COLLECTIONTYPE:
	  lwc = (LWCOLLECTION *) lwgeom;
	  ngeoms = lwc->ngeoms;
	  if (ngeoms <= 2)
	    {
		lwg2 = lwc->geoms[0];
		gaia = fromLWGeom (lwg2, dimension_model, declared_type);
	    }
	  break;
      default:
	  gaia = fromLWGeom (lwgeom, dimension_model, declared_type);
	  break;
      }

    return gaia;
}

static gaiaGeomCollPtr
fromLWGeomDiscarded (const LWGEOM * lwgeom, const int dimension_model,
		     const int declared_type)
{
/* 
/ converting a LWGEOM Geometry into a GAIA Geometry 
/ second collection - discarded items
*/
    gaiaGeomCollPtr gaia = NULL;
    LWGEOM *lwg2 = NULL;
    LWCOLLECTION *lwc = NULL;
    int ngeoms;

    if (lwgeom == NULL)
	return NULL;
    if (lwgeom_is_empty (lwgeom))
	return NULL;

    if (lwgeom->type == COLLECTIONTYPE)
      {
	  lwc = (LWCOLLECTION *) lwgeom;
	  ngeoms = lwc->ngeoms;
	  if (ngeoms >= 2)
	    {
		lwg2 = lwc->geoms[1];
		gaia = fromLWGeom (lwg2, dimension_model, declared_type);
	    }
      }

    return gaia;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaMakeValid (gaiaGeomCollPtr geom)
{
/* wrapping LWGEOM MakeValid [collecting valid items] */
    LWGEOM *g1;
    LWGEOM *g2;
    gaiaGeomCollPtr result;

    if (!geom)
	return NULL;
    g1 = toLWGeom (geom);
    g2 = lwgeom_make_valid (g1);
    if (!g2)
      {
	  lwgeom_free (g1);
	  return NULL;
      }
    result = fromLWGeomValidated (g2, geom->DimensionModel, geom->DeclaredType);
    spatialite_init_geos ();
    lwgeom_free (g1);
    lwgeom_free (g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaMakeValidDiscarded (gaiaGeomCollPtr geom)
{
/* wrapping LWGEOM MakeValid [collecting discarder items] */
    LWGEOM *g1;
    LWGEOM *g2;
    gaiaGeomCollPtr result;

    if (!geom)
	return NULL;
    g1 = toLWGeom (geom);
    g2 = lwgeom_make_valid (g1);
    if (!g2)
      {
	  lwgeom_free (g1);
	  return NULL;
      }
    result = fromLWGeomDiscarded (g2, geom->DimensionModel, geom->DeclaredType);
    spatialite_init_geos ();
    lwgeom_free (g1);
    lwgeom_free (g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSegmentize (gaiaGeomCollPtr geom, double dist)
{
/* wrapping LWGEOM Segmentize */
    LWGEOM *g1;
    LWGEOM *g2;
    gaiaGeomCollPtr result;

    if (!geom)
	return NULL;
    if (dist <= 0.0)
	return NULL;
    g1 = toLWGeom (geom);
    g2 = lwgeom_segmentize2d (g1, dist);
    if (!g2)
      {
	  lwgeom_free (g1);
	  return NULL;
      }
    result = fromLWGeom (g2, geom->DimensionModel, geom->DeclaredType);
    spatialite_init_geos ();
    lwgeom_free (g1);
    lwgeom_free (g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

static int
check_split_args (gaiaGeomCollPtr input, gaiaGeomCollPtr blade)
{
/* testing Split arguments */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int i_lns = 0;
    int i_pgs = 0;
    int b_pts = 0;
    int b_lns = 0;

    if (!input)
	return 0;
    if (!blade)
	return 0;

/* testing the Input type */
    if (input->FirstPoint != NULL)
      {
	  /* Point(s) on Input is forbidden !!!! */
	  return 0;
      }
    ln = input->FirstLinestring;
    while (ln)
      {
	  /* counting how many Linestrings are there */
	  i_lns++;
	  ln = ln->Next;
      }
    pg = input->FirstPolygon;
    while (pg)
      {
	  /* counting how many Polygons are there */
	  i_pgs++;
	  pg = pg->Next;
      }
    if (i_lns + i_pgs == 0)
      {
	  /* empty Input */
	  return 0;
      }

/* testing the Blade type */
    pt = blade->FirstPoint;
    while (pt)
      {
	  /* counting how many Points are there */
	  b_pts++;
	  pt = pt->Next;
      }
    if (b_pts > 1)
      {
	  /* MultiPoint on Blade is forbidden !!!! */
	  return 0;
      }
    ln = blade->FirstLinestring;
    while (ln)
      {
	  /* counting how many Linestrings are there */
	  b_lns++;
	  ln = ln->Next;
      }
    if (b_lns > 1)
      {
	  /* MultiLinestring on Blade is forbidden !!!! */
	  return 0;
      }
    if (blade->FirstPolygon != NULL)
      {
	  /* Polygon(s) on Blade is forbidden !!!! */
	  return 0;
      }
    if (b_pts + b_lns == 0)
      {
	  /* empty Blade */
	  return 0;
      }
    if (b_pts + b_lns > 1)
      {
	  /* invalid Blade [point + linestring] */
	  return 0;
      }

/* compatibility check */
    if (b_lns == 1)
      {
	  /* Linestring blade is always valid */
	  return 1;
      }
    if (i_lns >= 1 && b_pts == 1)
      {
	  /* Linestring or MultiLinestring input and Point blade is allowed */
	  return 1;
      }

    return 0;
}

static void
set_split_gtype (gaiaGeomCollPtr geom)
{
/* assignign the actual geometry type */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;

    pt = geom->FirstPoint;
    while (pt)
      {
	  /* counting how many Points are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many Linestrings are there */
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* counting how many Polygons are there */
	  pgs++;
	  pg = pg->Next;
      }

    if (pts == 1 && lns == 0 && pgs == 0)
      {
	  geom->DeclaredType = GAIA_POINT;
	  return;
      }
    if (pts > 1 && lns == 0 && pgs == 0)
      {
	  geom->DeclaredType = GAIA_MULTIPOINT;
	  return;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
      {
	  geom->DeclaredType = GAIA_LINESTRING;
	  return;
      }
    if (pts == 0 && lns > 1 && pgs == 0)
      {
	  geom->DeclaredType = GAIA_MULTILINESTRING;
	  return;
      }
    if (pts == 0 && lns == 0 && pgs == 1)
      {
	  geom->DeclaredType = GAIA_POLYGON;
	  return;
      }
    if (pts == 0 && lns == 0 && pgs > 1)
      {
	  geom->DeclaredType = GAIA_MULTIPOLYGON;
	  return;
      }
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
}

static LWGEOM *
toLWGeomLinestring (gaiaLinestringPtr ln, int srid)
{
/* converting a GAIA Linestring into a LWGEOM Geometry */
    int iv;
    double x;
    double y;
    double z;
    double m;
    int has_z = 0;
    int has_m = 0;
    POINTARRAY *pa;
    POINT4D point;

    if (ln->DimensionModel == GAIA_XY_Z || ln->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    if (ln->DimensionModel == GAIA_XY_M || ln->DimensionModel == GAIA_XY_Z_M)
	has_m = 1;
    pa = ptarray_construct (has_z, has_m, ln->Points);
    for (iv = 0; iv < ln->Points; iv++)
      {
	  /* copying vertices */
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  if (has_m)
	      point.m = m;
	  ptarray_set_point4d (pa, iv, &point);
      }
    return (LWGEOM *) lwline_construct (srid, NULL, pa);
}

static LWGEOM *
toLWGeomPolygon (gaiaPolygonPtr pg, int srid)
{
/* converting a GAIA Linestring into a LWGEOM Geometry */
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;
    int ngeoms;
    int has_z = 0;
    int has_m = 0;
    gaiaRingPtr rng;
    POINTARRAY **ppaa;
    POINT4D point;

    if (pg->DimensionModel == GAIA_XY_Z || pg->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    if (pg->DimensionModel == GAIA_XY_M || pg->DimensionModel == GAIA_XY_Z_M)
	has_m = 1;
    ngeoms = pg->NumInteriors;
    ppaa = lwalloc (sizeof (POINTARRAY *) * (ngeoms + 1));
    rng = pg->Exterior;
    ppaa[0] = ptarray_construct (has_z, has_m, rng->Points);
    for (iv = 0; iv < rng->Points; iv++)
      {
	  /* copying vertices - Exterior Ring */
	  if (pg->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
	    }
	  else if (pg->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
	    }
	  else if (pg->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (rng->Coords, iv, &x, &y);
	    }
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  if (has_m)
	      point.m = m;
	  ptarray_set_point4d (ppaa[0], iv, &point);
      }
    for (ib = 0; ib < pg->NumInteriors; ib++)
      {
	  /* copying vertices - Interior Rings */
	  rng = pg->Interiors + ib;
	  ppaa[1 + ib] = ptarray_construct (has_z, has_m, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		if (pg->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		  }
		else if (pg->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		  }
		else if (pg->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		if (has_m)
		    point.m = m;
		ptarray_set_point4d (ppaa[1 + ib], iv, &point);
	    }
      }
    return (LWGEOM *) lwpoly_construct (srid, NULL, ngeoms + 1, ppaa);
}

static gaiaGeomCollPtr
fromLWGeomLeft (gaiaGeomCollPtr gaia, const LWGEOM * lwgeom)
{
/* 
/ converting a LWGEOM Geometry into a GAIA Geometry 
/ collecting "left side" items
*/
    LWGEOM *lwg2 = NULL;
    LWCOLLECTION *lwc = NULL;
    int ngeoms;
    int ig;

    if (lwgeom == NULL)
	return NULL;
    if (lwgeom_is_empty (lwgeom))
	return NULL;

    if (lwgeom->type == COLLECTIONTYPE)
      {
	  lwc = (LWCOLLECTION *) lwgeom;
	  ngeoms = lwc->ngeoms;
	  for (ig = 0; ig < ngeoms; ig += 2)
	    {
		lwg2 = lwc->geoms[ig];
		fromLWGeomIncremental (gaia, lwg2);
	    }
      }
    else
	gaia = fromLWGeom (lwgeom, gaia->DimensionModel, gaia->DeclaredType);

    return gaia;
}

static gaiaGeomCollPtr
fromLWGeomRight (gaiaGeomCollPtr gaia, const LWGEOM * lwgeom)
{
/* 
/ converting a LWGEOM Geometry into a GAIA Geometry 
/ collecting "right side" items
*/
    LWGEOM *lwg2 = NULL;
    LWCOLLECTION *lwc = NULL;
    int ngeoms;
    int ig;

    if (lwgeom == NULL)
	return NULL;
    if (lwgeom_is_empty (lwgeom))
	return NULL;

    if (lwgeom->type == COLLECTIONTYPE)
      {
	  lwc = (LWCOLLECTION *) lwgeom;
	  ngeoms = lwc->ngeoms;
	  for (ig = 1; ig < ngeoms; ig += 2)
	    {
		lwg2 = lwc->geoms[ig];
		fromLWGeomIncremental (gaia, lwg2);
	    }
      }

    return gaia;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSplit (gaiaGeomCollPtr input, gaiaGeomCollPtr blade)
{
/* wrapping LWGEOM Split */
    LWGEOM *g1;
    LWGEOM *g2;
    LWGEOM *g3;
    gaiaGeomCollPtr result;

    if (!check_split_args (input, blade))
	return NULL;

    g1 = toLWGeom (input);
    g2 = toLWGeom (blade);
    g3 = lwgeom_split (g1, g2);
    if (!g3)
      {
	  lwgeom_free (g1);
	  lwgeom_free (g2);
	  return NULL;
      }
    result = fromLWGeom (g3, input->DimensionModel, input->DeclaredType);
    spatialite_init_geos ();
    lwgeom_free (g1);
    lwgeom_free (g2);
    lwgeom_free (g3);
    if (result == NULL)
	return NULL;
    result->Srid = input->Srid;
    set_split_gtype (result);
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSplitLeft (gaiaGeomCollPtr input, gaiaGeomCollPtr blade)
{
/* wrapping LWGEOM Split [left half] */
    LWGEOM *g1;
    LWGEOM *g2;
    LWGEOM *g3;
    gaiaGeomCollPtr result = NULL;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;

    if (!check_split_args (input, blade))
	return NULL;

    if (input->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (input->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (input->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();

    g2 = toLWGeom (blade);

    ln = input->FirstLinestring;
    while (ln)
      {
	  /* splitting some Linestring */
	  g1 = toLWGeomLinestring (ln, input->Srid);
	  g3 = lwgeom_split (g1, g2);
	  if (g3)
	    {
		result = fromLWGeomLeft (result, g3);
		lwgeom_free (g3);
	    }
	  spatialite_init_geos ();
	  lwgeom_free (g1);
	  ln = ln->Next;
      }
    pg = input->FirstPolygon;
    while (pg)
      {
	  /* splitting some Polygon */
	  g1 = toLWGeomPolygon (pg, input->Srid);
	  g3 = lwgeom_split (g1, g2);
	  if (g3)
	    {
		result = fromLWGeomLeft (result, g3);
		lwgeom_free (g3);
	    }
	  spatialite_init_geos ();
	  lwgeom_free (g1);
	  pg = pg->Next;
      }

    lwgeom_free (g2);
    if (result == NULL)
	return NULL;
    if (result->FirstPoint == NULL && result->FirstLinestring == NULL
	&& result->FirstPolygon == NULL)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    result->Srid = input->Srid;
    set_split_gtype (result);
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSplitRight (gaiaGeomCollPtr input, gaiaGeomCollPtr blade)
{
/* wrapping LWGEOM Split [right half] */
    LWGEOM *g1;
    LWGEOM *g2;
    LWGEOM *g3;
    gaiaGeomCollPtr result = NULL;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;

    if (!check_split_args (input, blade))
	return NULL;

    if (input->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (input->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (input->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();

    g2 = toLWGeom (blade);

    ln = input->FirstLinestring;
    while (ln)
      {
	  /* splitting some Linestring */
	  g1 = toLWGeomLinestring (ln, input->Srid);
	  g3 = lwgeom_split (g1, g2);
	  if (g3)
	    {
		result = fromLWGeomRight (result, g3);
		lwgeom_free (g3);
	    }
	  spatialite_init_geos ();
	  lwgeom_free (g1);
	  ln = ln->Next;
      }
    pg = input->FirstPolygon;
    while (pg)
      {
	  /* splitting some Polygon */
	  g1 = toLWGeomPolygon (pg, input->Srid);
	  g3 = lwgeom_split (g1, g2);
	  if (g3)
	    {
		result = fromLWGeomRight (result, g3);
		lwgeom_free (g3);
	    }
	  spatialite_init_geos ();
	  lwgeom_free (g1);
	  pg = pg->Next;
      }

    lwgeom_free (g2);
    if (result == NULL)
	return NULL;
    if (result->FirstPoint == NULL && result->FirstLinestring == NULL
	&& result->FirstPolygon == NULL)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    result->Srid = input->Srid;
    set_split_gtype (result);
    return result;
}

GAIAGEO_DECLARE int
gaiaAzimuth (double xa, double ya, double xb, double yb, double *azimuth)
{
/* wrapping LWGEOM Azimuth */
    POINT2D pt1;
    POINT2D pt2;
    double az;
    pt1.x = xa;
    pt1.y = ya;
    pt2.x = xb;
    pt2.y = yb;
    if (!azimuth_pt_pt (&pt1, &pt2, &az))
	return 0;
    *azimuth = az;
    return 1;
}

GAIAGEO_DECLARE char *
gaiaGeoHash (gaiaGeomCollPtr geom, int precision)
{
/* wrapping LWGEOM GeoHash */
    LWGEOM *g;
    char *result;
    char *geo_hash;
    int len;

    if (!geom)
	return NULL;
    gaiaMbrGeometry (geom);
    if (geom->MinX < -180.0 || geom->MaxX > 180.0 || geom->MinY < -90.0
	|| geom->MaxY > 90.0)
	return NULL;
    g = toLWGeom (geom);
    result = lwgeom_geohash (g, precision);
    lwgeom_free (g);
    if (result == NULL)
	return NULL;
    len = strlen (result);
    if (len == 0)
      {
	  lwfree (result);
	  return NULL;
      }
    geo_hash = malloc (len + 1);
    strcpy (geo_hash, result);
    lwfree (result);
    return geo_hash;
}

GAIAGEO_DECLARE char *
gaiaAsX3D (gaiaGeomCollPtr geom, const char *srs, int precision, int options,
	   const char *defid)
{
/* wrapping LWGEOM AsX3D */
    LWGEOM *g;
    char *result;
    char *x3d;
    int len;

    if (!geom)
	return NULL;
    gaiaMbrGeometry (geom);
    g = toLWGeom (geom);
    result = lwgeom_to_x3d3 (g, (char *) srs, precision, options, defid);
    lwgeom_free (g);
    if (result == NULL)
	return NULL;
    len = strlen (result);
    if (len == 0)
      {
	  lwfree (result);
	  return NULL;
      }
    x3d = malloc (len + 1);
    strcpy (x3d, result);
    lwfree (result);
    return x3d;
}

GAIAGEO_DECLARE int
gaia3DDistance (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2, double *dist)
{
/* wrapping LWGEOM mindistance3d */
    LWGEOM *g1;
    LWGEOM *g2;
    double d;

    g1 = toLWGeom (geom1);
    g2 = toLWGeom (geom2);

    d = lwgeom_mindistance3d (g1, g2);
    lwgeom_free (g1);
    lwgeom_free (g2);
    *dist = d;
    return 1;
}

GAIAGEO_DECLARE int
gaiaMaxDistance (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2, double *dist)
{
/* wrapping LWGEOM maxdistance2d */
    LWGEOM *g1;
    LWGEOM *g2;
    double d;

    g1 = toLWGeom (geom1);
    g2 = toLWGeom (geom2);

    d = lwgeom_maxdistance2d (g1, g2);
    lwgeom_free (g1);
    lwgeom_free (g2);
    *dist = d;
    return 1;
}

GAIAGEO_DECLARE int
gaia3DMaxDistance (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2, double *dist)
{
/* wrapping LWGEOM maxdistance2d */
    LWGEOM *g1;
    LWGEOM *g2;
    double d;

    g1 = toLWGeom (geom1);
    g2 = toLWGeom (geom2);

    d = lwgeom_maxdistance3d (g1, g2);
    lwgeom_free (g1);
    lwgeom_free (g2);
    *dist = d;
    return 1;
}

#endif /* end enabling LWGEOM support */
