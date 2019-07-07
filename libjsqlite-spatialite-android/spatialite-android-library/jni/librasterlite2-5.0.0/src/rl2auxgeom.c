/*

 rl2auxgeom -- auxiliary methods supporting basic 2D geometries

 version 0.1, 2015 February 2

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
 
Portions created by the Initial Developer are Copyright (C) 2015
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
#include <float.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#include <spatialite/gg_const.h>

#define MATRIX_MAGIC_START		0x00
#define MATRIX_MAGIC_DELIMITER	0x3a
#define MATRIX_MAGIC_END		0xb3

/*
/
/ PLEASE NOTE: all these functions are roughly equivalent to the 
/ corresponding APIs implemented in libspatialite.
/ this odd code duplication is absolutely required in order to carefully
/ avoid to create any direct reference to link symbols declared by
/ libspatialite, because such an action will irremediabely introduce
/ nasty dependencies to libsqlite3 as well.
/ and this is absolutely not allowed in a "pure" loadable module context.
/
/ a very important difference exists between the RasterLite2 and the
/ SpatiaLite implementations.
/ in RL2 any XYZ, XYM or XYZM dimension will always be parsed as if it
/ was just XY; this is fully consintent with RL2 specific requirements
/ (just rendering 2D geoms via Cairo)
/
*/

static int
rl2GeomEndianArch ()
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

static rl2PointPtr
rl2CreatePointXY (double x, double y)
{
/* POINT object constructor */
    rl2PointPtr p = malloc (sizeof (rl2Point));
    p->x = x;
    p->y = y;
    p->dims = GAIA_XY;
    p->next = NULL;
    return p;
}

static rl2PointPtr
rl2CreatePointXYZ (double x, double y, double z)
{
/* POINT object constructor */
    rl2PointPtr p = malloc (sizeof (rl2Point));
    p->x = x;
    p->y = y;
    p->z = z;
    p->dims = GAIA_XY_Z;
    p->next = NULL;
    return p;
}

static rl2PointPtr
rl2CreatePointXYM (double x, double y, double m)
{
/* POINT object constructor */
    rl2PointPtr p = malloc (sizeof (rl2Point));
    p->x = x;
    p->y = y;
    p->m = m;
    p->dims = GAIA_XY_M;
    p->next = NULL;
    return p;
}

static rl2PointPtr
rl2CreatePointXYZM (double x, double y, double z, double m)
{
/* POINT object constructor */
    rl2PointPtr p = malloc (sizeof (rl2Point));
    p->x = x;
    p->y = y;
    p->z = z;
    p->m = m;
    p->dims = GAIA_XY_Z_M;
    p->next = NULL;
    return p;
}

static void
rl2DestroyPoint (rl2PointPtr ptr)
{
/* POINT object destructor */
    if (ptr != NULL)
	free (ptr);
}

RL2_PRIVATE rl2LinestringPtr
rl2CreateLinestring (int vert, int dims)
{
/* LINESTRING object constructor */
    int n;
    rl2LinestringPtr p = malloc (sizeof (rl2Linestring));
    switch (dims)
      {
      case GAIA_XY_Z:
      case GAIA_XY_M:
	  n = 3;
	  break;
      case GAIA_XY_Z_M:
	  n = 4;
	  break;
      case GAIA_XY:
      default:
	  n = 2;
	  break;
      };
    p->coords = malloc (sizeof (double) * (vert * n));
    p->points = vert;
    p->minx = DBL_MAX;
    p->miny = DBL_MAX;
    p->maxx = 0.0 - DBL_MAX;
    p->maxy = 0.0 - DBL_MAX;
    p->dims = dims;
    p->next = NULL;
    return p;
}

RL2_PRIVATE void
rl2DestroyLinestring (rl2LinestringPtr ptr)
{
/* LINESTRING object desctructror */
    if (ptr)
      {
	  if (ptr->coords)
	      free (ptr->coords);
	  free (ptr);
      }
}

RL2_PRIVATE rl2LinestringPtr
rl2_linestring_to_image (rl2LinestringPtr line, int height, double minx,
			 double miny, double x_res, double y_res)
{
/* creating a Linestring in image coordinates */
    rl2LinestringPtr out = NULL;
    int iv;
    double x;
    double y;
    double dx;
    double dy;

    if (line == NULL)
	return NULL;
    out = rl2CreateLinestring (line->points, GAIA_XY);
    if (out == NULL)
	return out;
    for (iv = 0; iv < line->points; iv++)
      {
	  /* populating the X and Y arrays */
	  rl2GetPoint (line->coords, iv, &x, &y);
	  dx = (x - minx) / x_res;
	  dy = (double) height - ((y - miny) / y_res);
	  rl2SetPoint (out->coords, iv, dx, dy);
      }
    return out;
}

static rl2RingPtr
rl2CreateRing (int vert, int dims)
{
/* ring object constructor */
    int n;
    rl2RingPtr p = malloc (sizeof (rl2Ring));
    switch (dims)
      {
      case GAIA_XY_Z:
      case GAIA_XY_M:
	  n = 3;
	  break;
      case GAIA_XY_Z_M:
	  n = 4;
	  break;
      case GAIA_XY:
      default:
	  n = 2;
	  break;
      };
    p->coords = malloc (sizeof (double) * (vert * n));
    p->points = vert;
    p->minx = DBL_MAX;
    p->miny = DBL_MAX;
    p->maxx = 0.0 - DBL_MAX;
    p->maxy = 0.0 - DBL_MAX;
    p->dims = dims;
    p->next = NULL;
    return p;
}

RL2_PRIVATE rl2RingPtr
rl2_ring_to_image (rl2RingPtr ring, int height, double minx, double miny,
		   double x_res, double y_res)
{
/* creating a Ring in image coordinates */
    rl2RingPtr out = NULL;
    int iv;
    double x;
    double y;
    double dx;
    double dy;

    if (ring == NULL)
	return NULL;
    out = rl2CreateRing (ring->points, GAIA_XY);
    if (out == NULL)
	return out;
    for (iv = 0; iv < ring->points; iv++)
      {
	  /* populating the X and Y arrays */
	  rl2GetPoint (ring->coords, iv, &x, &y);
	  dx = (x - minx) / x_res;
	  dy = (double) height - ((y - miny) / y_res);
	  rl2SetPoint (out->coords, iv, dx, dy);
      }
    return out;
}

RL2_PRIVATE void
rl2DestroyRing (rl2RingPtr ptr)
{
/* ring object destructor */
    if (ptr)
      {
	  if (ptr->coords)
	      free (ptr->coords);
	  free (ptr);
      }
}

static rl2PolygonPtr
rl2CreatePolygon (int vert, int excl, int dims)
{
/* POLYGON object constructor */
    rl2PolygonPtr p;
    rl2RingPtr pP;
    int ind;
    p = malloc (sizeof (rl2Polygon));
    p->exterior = rl2CreateRing (vert, dims);
    p->num_interiors = excl;
    p->dims = dims;
    p->next = NULL;
    if (excl == 0)
	p->interiors = NULL;
    else
	p->interiors = malloc (sizeof (rl2Ring) * excl);
    for (ind = 0; ind < p->num_interiors; ind++)
      {
	  pP = p->interiors + ind;
	  pP->points = 0;
	  pP->coords = NULL;
	  pP->minx = DBL_MAX;
	  pP->miny = DBL_MAX;
	  pP->maxx = 0.0 - DBL_MAX;
	  pP->maxy = 0.0 - DBL_MAX;
	  pP->dims = dims;
      }
    return p;
}

static void
rl2DestroyPolygon (rl2PolygonPtr p)
{
/* POLYGON object destructor */
    rl2RingPtr pP;
    int ind;
    if (p->exterior)
	rl2DestroyRing (p->exterior);
    for (ind = 0; ind < p->num_interiors; ind++)
      {
	  pP = p->interiors + ind;
	  if (pP->coords)
	      free (pP->coords);
      }
    if (p->interiors)
	free (p->interiors);
    free (p);
}

RL2_PRIVATE rl2GeometryPtr
rl2CreateGeometry (int dims, int type)
{
/* GEOMETRYCOLLECTION object constructor */
    rl2GeometryPtr p = malloc (sizeof (rl2Geometry));
    p->first_point = NULL;
    p->last_point = NULL;
    p->first_linestring = NULL;
    p->last_linestring = NULL;
    p->first_polygon = NULL;
    p->last_polygon = NULL;
    p->dims = dims;
    switch (type)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
	  p->type = GAIA_POINT;
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
      case GAIA_COMPRESSED_LINESTRING:
      case GAIA_COMPRESSED_LINESTRINGZ:
      case GAIA_COMPRESSED_LINESTRINGM:
      case GAIA_COMPRESSED_LINESTRINGZM:
	  p->type = GAIA_LINESTRING;
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
      case GAIA_COMPRESSED_POLYGON:
      case GAIA_COMPRESSED_POLYGONZ:
      case GAIA_COMPRESSED_POLYGONM:
      case GAIA_COMPRESSED_POLYGONZM:
	  p->type = GAIA_POLYGON;
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  p->type = GAIA_MULTIPOINT;
	  break;
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  p->type = GAIA_MULTILINESTRING;
	  break;
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  p->type = GAIA_MULTIPOLYGON;
	  break;
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  p->type = GAIA_GEOMETRYCOLLECTION;
	  break;
      default:
	  p->type = type;
	  break;
      };
    return p;
}

static int
rl2GeomImport32 (const unsigned char *p, int little_endian,
		 int little_endian_arch)
{
/* fetches a 32bit int from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
      }
    return convert.int_value;
}

static float
rl2GeomImportF32 (const unsigned char *p, int little_endian,
		  int little_endian_arch)
{
/* fetches a 32bit float from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	float flt_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
      }
    return convert.flt_value;
}

static double
rl2GeomImport64 (const unsigned char *p, int little_endian,
		 int little_endian_arch)
{
/* fetches a 64bit double from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[8];
	double double_value;
    } convert;
    if (little_endian_arch)
      {
/* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 7);
		convert.byte[1] = *(p + 6);
		convert.byte[2] = *(p + 5);
		convert.byte[3] = *(p + 4);
		convert.byte[4] = *(p + 3);
		convert.byte[5] = *(p + 2);
		convert.byte[6] = *(p + 1);
		convert.byte[7] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
		convert.byte[4] = *(p + 4);
		convert.byte[5] = *(p + 5);
		convert.byte[6] = *(p + 6);
		convert.byte[7] = *(p + 7);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
		convert.byte[4] = *(p + 4);
		convert.byte[5] = *(p + 5);
		convert.byte[6] = *(p + 6);
		convert.byte[7] = *(p + 7);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 7);
		convert.byte[1] = *(p + 6);
		convert.byte[2] = *(p + 5);
		convert.byte[3] = *(p + 4);
		convert.byte[4] = *(p + 3);
		convert.byte[5] = *(p + 2);
		convert.byte[6] = *(p + 1);
		convert.byte[7] = *(p + 0);
	    }
      }
    return convert.double_value;
}

static void
rl2GeomExport32 (unsigned char *p, int value, int little_endian,
		 int little_endian_arch)
{
/* stores a 32bit int into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
      }
}

static void
rl2GeomExport64 (unsigned char *p, double value, int little_endian,
		 int little_endian_arch)
{
/* stores a 64bit double into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[8];
	double double_value;
    } convert;
    convert.double_value = value;
    if (little_endian_arch)
      {
/* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 7) = convert.byte[0];
		*(p + 6) = convert.byte[1];
		*(p + 5) = convert.byte[2];
		*(p + 4) = convert.byte[3];
		*(p + 3) = convert.byte[4];
		*(p + 2) = convert.byte[5];
		*(p + 1) = convert.byte[6];
		*(p + 0) = convert.byte[7];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
		*(p + 4) = convert.byte[4];
		*(p + 5) = convert.byte[5];
		*(p + 6) = convert.byte[6];
		*(p + 7) = convert.byte[7];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
		*(p + 4) = convert.byte[4];
		*(p + 5) = convert.byte[5];
		*(p + 6) = convert.byte[6];
		*(p + 7) = convert.byte[7];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 7) = convert.byte[0];
		*(p + 6) = convert.byte[1];
		*(p + 5) = convert.byte[2];
		*(p + 4) = convert.byte[3];
		*(p + 3) = convert.byte[4];
		*(p + 2) = convert.byte[5];
		*(p + 1) = convert.byte[6];
		*(p + 0) = convert.byte[7];
	    }
      }
}

RL2_PRIVATE void
rl2AddPointXYToGeometry (rl2GeometryPtr p, double x, double y)
{
/* adding a POINT to this GEOMETRYCOLLECTION */
    rl2PointPtr point = rl2CreatePointXY (x, y);
    if (p->first_point == NULL)
	p->first_point = point;
    if (p->last_point != NULL)
	p->last_point->next = point;
    p->last_point = point;
}

RL2_PRIVATE void
rl2AddPointXYZToGeometry (rl2GeometryPtr p, double x, double y, double z)
{
/* adding a POINT to this GEOMETRYCOLLECTION */
    rl2PointPtr point = rl2CreatePointXYZ (x, y, z);
    if (p->first_point == NULL)
	p->first_point = point;
    if (p->last_point != NULL)
	p->last_point->next = point;
    p->last_point = point;
}

RL2_PRIVATE void
rl2AddPointXYMToGeometry (rl2GeometryPtr p, double x, double y, double m)
{
/* adding a POINT to this GEOMETRYCOLLECTION */
    rl2PointPtr point = rl2CreatePointXYM (x, y, m);
    if (p->first_point == NULL)
	p->first_point = point;
    if (p->last_point != NULL)
	p->last_point->next = point;
    p->last_point = point;
}

RL2_PRIVATE void
rl2AddPointXYZMToGeometry (rl2GeometryPtr p, double x, double y, double z,
			   double m)
{
/* adding a POINT to this GEOMETRYCOLLECTION */
    rl2PointPtr point = rl2CreatePointXYZM (x, y, z, m);
    if (p->first_point == NULL)
	p->first_point = point;
    if (p->last_point != NULL)
	p->last_point->next = point;
    p->last_point = point;
}

static void
rl2ParsePoint (rl2GeometryPtr geom, const unsigned char *blob, int size,
	       int endian, int endian_arch, int *offset)
{
/* decodes a POINT from WKB */
    double x;
    double y;
    if (size < *offset + 16)
	return;
    x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
    y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
    *offset += 16;
    rl2AddPointXYToGeometry (geom, x, y);
}

static void
rl2ParsePointZ (rl2GeometryPtr geom, const unsigned char *blob, int size,
		int endian, int endian_arch, int *offset)
{
/* decodes a POINTZ from WKB */
    double x;
    double y;
    double z;
    if (size < *offset + 24)
	return;
    x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
    y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
    z = rl2GeomImport64 (blob + (*offset + 16), endian, endian_arch);
    *offset += 24;
    rl2AddPointXYZToGeometry (geom, x, y, z);
}

static void
rl2ParsePointM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		int endian, int endian_arch, int *offset)
{
/* decodes a POINTM from WKB */
    double x;
    double y;
    double m;
    if (size < *offset + 24)
	return;
    x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
    y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
    m = rl2GeomImport64 (blob + (*offset + 16), endian, endian_arch);
    *offset += 24;
    rl2AddPointXYMToGeometry (geom, x, y, m);
}

static void
rl2ParsePointZM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		 int endian, int endian_arch, int *offset)
{
/* decodes a POINTZM from WKB */
    double x;
    double y;
    double z;
    double m;
    if (size < *offset + 32)
	return;
    x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
    y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
    z = rl2GeomImport64 (blob + (*offset + 16), endian, endian_arch);
    m = rl2GeomImport64 (blob + (*offset + 24), endian, endian_arch);
    *offset += 32;
    rl2AddPointXYZMToGeometry (geom, x, y, z, m);
}

static rl2GeometryPtr
rl2ParseTinyPointBlob (const unsigned char *blob, unsigned int size)
{
/* decoding from SpatiaLite TinyPoint BLOB to GEOMETRY */
    unsigned char pointType;
    int type;
    int dims;
    int offset;
    int little_endian;
    int endian_arch = rl2GeomEndianArch ();
    rl2GeometryPtr geo = NULL;

    if (size < 24)
	return NULL;		/* cannot be an internal BLOB TinyPoint geometry */
    if (*(blob + 0) != GAIA_MARK_START)
	return NULL;		/* failed to recognize START signature */
    if (*(blob + (size - 1)) != GAIA_MARK_END)
	return NULL;		/* failed to recognize END signature */
    if (*(blob + 1) == GAIA_TINYPOINT_LITTLE_ENDIAN)
	little_endian = 1;
    else if (*(blob + 1) == GAIA_TINYPOINT_BIG_ENDIAN)
	little_endian = 0;
    else
	return NULL;		/* unknown encoding; nor little-endian neither big-endian */

    pointType = *(blob + 6);
    switch (pointType)
      {
	  /* setting up DimensionModel */
      case GAIA_TINYPOINT_XYZ:
	  type = GAIA_POINTZ;
	  dims = GAIA_XY_Z;
	  break;
      case GAIA_TINYPOINT_XYM:
	  type = GAIA_POINTM;
	  dims = GAIA_XY_M;
	  break;
      case GAIA_TINYPOINT_XYZM:
	  type = GAIA_POINTZM;
	  dims = GAIA_XY_Z_M;
	  break;
      default:
	  type = GAIA_POINT;
	  dims = GAIA_XY;
	  break;
      };
    geo = rl2CreateGeometry (dims, type);
    geo->srid = rl2GeomImport32 (blob + 2, little_endian, endian_arch);
    offset = 7;
    switch (type)
      {
	  /* parsing elementary geometries */
      case GAIA_POINT:
	  rl2ParsePoint (geo, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_POINTZ:
	  rl2ParsePointZ (geo, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_POINTM:
	  rl2ParsePointM (geo, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_POINTZM:
	  rl2ParsePointZM (geo, blob, size, little_endian, endian_arch,
			   &offset);
	  break;
      default:
	  break;
      };
    geo->minx = geo->first_point->x;
    geo->miny = geo->first_point->y;
    geo->maxx = geo->first_point->x;
    geo->maxy = geo->first_point->y;
    return geo;
}

RL2_PRIVATE rl2LinestringPtr
rl2AddLinestringToGeometry (rl2GeometryPtr p, int vert)
{
/* adding a POINT to this GEOMETRYCOLLECTION */
    rl2LinestringPtr line = rl2CreateLinestring (vert, p->dims);
    if (p->first_linestring == NULL)
	p->first_linestring = line;
    if (p->last_linestring != NULL)
	p->last_linestring->next = line;
    p->last_linestring = line;
    return line;
}

static void
rl2ParseLine (rl2GeometryPtr geom, const unsigned char *blob, int size,
	      int endian, int endian_arch, int *offset)
{
/* decodes a LINESTRING from WKB */
    int points;
    int iv;
    double x;
    double y;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (16 * points))
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
	  y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
	  rl2SetPoint (line->coords, iv, x, y);
	  if (x < line->minx)
	      line->minx = x;
	  if (x > line->maxx)
	      line->maxx = x;
	  if (y < line->miny)
	      line->miny = y;
	  if (y > line->maxy)
	      line->maxy = y;
	  *offset += 16;
      }
}

static void
rl2ParseLineZ (rl2GeometryPtr geom, const unsigned char *blob, int size,
	       int endian, int endian_arch, int *offset)
{
/* decodes a LINESTRINGZ from WKB */
    int points;
    int iv;
    double x;
    double y;
    double z;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (24 * points))
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
	  y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
	  z = rl2GeomImport64 (blob + (*offset + 16), endian, endian_arch);
	  rl2SetPointZ (line->coords, iv, x, y, z);
	  if (x < line->minx)
	      line->minx = x;
	  if (x > line->maxx)
	      line->maxx = x;
	  if (y < line->miny)
	      line->miny = y;
	  if (y > line->maxy)
	      line->maxy = y;
	  *offset += 24;
      }
}

static void
rl2ParseLineM (rl2GeometryPtr geom, const unsigned char *blob, int size,
	       int endian, int endian_arch, int *offset)
{
/* decodes a LINESTRINGM from WKB */
    int points;
    int iv;
    double x;
    double y;
    double m;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (24 * points))
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
	  y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
	  m = rl2GeomImport64 (blob + (*offset + 16), endian, endian_arch);
	  rl2SetPointM (line->coords, iv, x, y, m);
	  if (x < line->minx)
	      line->minx = x;
	  if (x > line->maxx)
	      line->maxx = x;
	  if (y < line->miny)
	      line->miny = y;
	  if (y > line->maxy)
	      line->maxy = y;
	  *offset += 24;
      }
}

static void
rl2ParseLineZM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		int endian, int endian_arch, int *offset)
{
/* decodes a LINESTRINGZM from WKB */
    int points;
    int iv;
    double x;
    double y;
    double z;
    double m;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (32 * points))
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
	  y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
	  z = rl2GeomImport64 (blob + (*offset + 16), endian, endian_arch);
	  m = rl2GeomImport64 (blob + (*offset + 24), endian, endian_arch);
	  rl2SetPointZM (line->coords, iv, x, y, z, m);
	  if (x < line->minx)
	      line->minx = x;
	  if (x > line->maxx)
	      line->maxx = x;
	  if (y < line->miny)
	      line->miny = y;
	  if (y > line->maxy)
	      line->maxy = y;
	  *offset += 32;
      }
}

RL2_PRIVATE rl2PolygonPtr
rl2AddPolygonToGeometry (rl2GeometryPtr p, int vert, int interiors)
{
/* adding a POLYGON to this GEOMETRYCOLLECTION */
    rl2PolygonPtr polyg = rl2CreatePolygon (vert, interiors, p->dims);
    if (p->first_polygon == NULL)
	p->first_polygon = polyg;
    if (p->last_polygon != NULL)
	p->last_polygon->next = polyg;
    p->last_polygon = polyg;
    return polyg;
}

RL2_PRIVATE rl2RingPtr
rl2AddInteriorRing (rl2PolygonPtr p, int pos, int vert)
{
/* adding an interior ring to some polygon */
    int n;
    rl2RingPtr pP = p->interiors + pos;
    switch (p->dims)
      {
      case GAIA_XY_Z:
      case GAIA_XY_M:
	  n = 3;
	  break;
      case GAIA_XY_Z_M:
	  n = 4;
	  break;
      case GAIA_XY:
      default:
	  n = 2;
	  break;
      };
    pP->points = vert;
    pP->coords = malloc (sizeof (double) * (vert * n));
    pP->dims = p->dims;
    return pP;
}

static void
rl2ParsePolygon (rl2GeometryPtr geom, const unsigned char *blob, int size,
		 int endian, int endian_arch, int *offset)
{
/* decodes a POLYGON from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (16 * nverts))
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 16;
		rl2SetPoint (ring->coords, iv, x, y);
		if (x < ring->minx)
		    ring->minx = x;
		if (x > ring->maxx)
		    ring->maxx = x;
		if (y < ring->miny)
		    ring->miny = y;
		if (y > ring->maxy)
		    ring->maxy = y;
	    }
      }
}

static void
rl2ParsePolygonZ (rl2GeometryPtr geom, const unsigned char *blob, int size,
		  int endian, int endian_arch, int *offset)
{
/* decodes a POLYGONZ from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (24 * nverts))
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		z = rl2GeomImport64 (blob + (*offset + 16), endian,
				     endian_arch);
		*offset += 24;
		rl2SetPointZ (ring->coords, iv, x, y, z);
		if (x < ring->minx)
		    ring->minx = x;
		if (x > ring->maxx)
		    ring->maxx = x;
		if (y < ring->miny)
		    ring->miny = y;
		if (y > ring->maxy)
		    ring->maxy = y;
	    }
      }
}

static void
rl2ParsePolygonM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		  int endian, int endian_arch, int *offset)
{
/* decodes a POLYGONM from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double m;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (24 * nverts))
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		m = rl2GeomImport64 (blob + (*offset + 16), endian,
				     endian_arch);
		*offset += 24;
		rl2SetPointM (ring->coords, iv, x, y, m);
		if (x < ring->minx)
		    ring->minx = x;
		if (x > ring->maxx)
		    ring->maxx = x;
		if (y < ring->miny)
		    ring->miny = y;
		if (y > ring->maxy)
		    ring->maxy = y;
	    }
      }
}

static void
rl2ParsePolygonZM (rl2GeometryPtr geom, const unsigned char *blob, int size,
		   int endian, int endian_arch, int *offset)
{
/* decodes a POLYGONZM from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (32 * nverts))
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		z = rl2GeomImport64 (blob + (*offset + 16), endian,
				     endian_arch);
		m = rl2GeomImport64 (blob + (*offset + 24), endian,
				     endian_arch);
		*offset += 32;
		rl2SetPointZM (ring->coords, iv, x, y, z, m);
		if (x < ring->minx)
		    ring->minx = x;
		if (x > ring->maxx)
		    ring->maxx = x;
		if (y < ring->miny)
		    ring->miny = y;
		if (y > ring->maxy)
		    ring->maxy = y;
	    }
      }
}

static void
rl2ParseCompressedLine (rl2GeometryPtr geom, const unsigned char *blob,
			int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED LINESTRING from WKB */
    int points;
    int iv;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (8 * points) + 16)
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  if (iv == 0 || iv == (points - 1))
	    {
		/* first and last vertices are uncompressed */
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		*offset += 16;
	    }
	  else
	    {
		/* any other intermediate vertex is compressed */
		fx = rl2GeomImportF32 (blob + *offset, endian, endian_arch);
		fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
				       endian_arch);
		x = last_x + fx;
		y = last_y + fy;
		*offset += 8;
	    }
	  rl2SetPoint (line->coords, iv, x, y);
	  if (x < line->minx)
	      line->minx = x;
	  if (x > line->maxx)
	      line->maxx = x;
	  if (y < line->miny)
	      line->miny = y;
	  if (y > line->maxy)
	      line->maxy = y;
	  last_x = x;
	  last_y = y;
      }
}

static void
rl2ParseCompressedLineZ (rl2GeometryPtr geom, const unsigned char *blob,
			 int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED LINESTRINGZ from WKB */
    int points;
    int iv;
    double x;
    double y;
    double z;
    double last_x = 0.0;
    double last_y = 0.0;
    double last_z = 0.0;
    float fx;
    float fy;
    float fz;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (12 * points) + 24)
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  if (iv == 0 || iv == (points - 1))
	    {
		/* first and last vertices are uncompressed */
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		z = rl2GeomImport64 (blob + (*offset + 16), endian,
				     endian_arch);
		*offset += 24;
	    }
	  else
	    {
		/* any other intermediate vertex is compressed */
		fx = rl2GeomImportF32 (blob + *offset, endian, endian_arch);
		fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
				       endian_arch);
		fz = rl2GeomImportF32 (blob + (*offset + 8), endian,
				       endian_arch);
		x = last_x + fx;
		y = last_y + fy;
		z = last_z + fz;
		*offset += 12;
	    }
	  rl2SetPointZ (line->coords, iv, x, y, z);
	  if (x < line->minx)
	      line->minx = x;
	  if (x > line->maxx)
	      line->maxx = x;
	  if (y < line->miny)
	      line->miny = y;
	  if (y > line->maxy)
	      line->maxy = y;
	  last_x = x;
	  last_y = y;
	  last_z = z;
      }
}

static void
rl2ParseCompressedLineM (rl2GeometryPtr geom, const unsigned char *blob,
			 int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED LINESTRINGM from WKB */
    int points;
    int iv;
    double x;
    double y;
    double m;
    double last_x = 0.0;
    double last_y = 0.0;
    double last_m = 0.0;
    float fx;
    float fy;
    float fm;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (16 * points) + 16)
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  if (iv == 0 || iv == (points - 1))
	    {
		/* first and last vertices are uncompressed */
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		m = rl2GeomImport64 (blob + (*offset + 16), endian,
				     endian_arch);
		*offset += 24;
	    }
	  else
	    {
		/* any other intermediate vertex is compressed */
		fx = rl2GeomImportF32 (blob + *offset, endian, endian_arch);
		fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
				       endian_arch);
		fm = rl2GeomImportF32 (blob + (*offset + 8), endian,
				       endian_arch);
		x = last_x + fx;
		y = last_y + fy;
		m = last_m + fm;
		*offset += 16;
	    }
	  rl2SetPointM (line->coords, iv, x, y, m);
	  if (x < line->minx)
	      line->minx = x;
	  if (x > line->maxx)
	      line->maxx = x;
	  if (y < line->miny)
	      line->miny = y;
	  if (y > line->maxy)
	      line->maxy = y;
	  last_x = x;
	  last_y = y;
	  last_m = m;
      }
}

static void
rl2ParseCompressedLineZM (rl2GeometryPtr geom, const unsigned char *blob,
			  int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED LINESTRINGZM from WKB */
    int points;
    int iv;
    double x;
    double y;
    double z;
    double m;
    double last_x = 0.0;
    double last_y = 0.0;
    double last_z = 0.0;
    double last_m = 0.0;
    float fx;
    float fy;
    float fz;
    float fm;
    rl2LinestringPtr line;
    if (size < *offset + 4)
	return;
    points = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    if (size < *offset + (20 * points) + 24)
	return;
    line = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  if (iv == 0 || iv == (points - 1))
	    {
		/* first and last vertices are uncompressed */
		x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		y = rl2GeomImport64 (blob + (*offset + 8), endian, endian_arch);
		z = rl2GeomImport64 (blob + (*offset + 16), endian,
				     endian_arch);
		m = rl2GeomImport64 (blob + (*offset + 24), endian,
				     endian_arch);
		*offset += 32;
	    }
	  else
	    {
		/* any other intermediate vertex is compressed */
		fx = rl2GeomImportF32 (blob + *offset, endian, endian_arch);
		fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
				       endian_arch);
		fz = rl2GeomImportF32 (blob + (*offset + 8), endian,
				       endian_arch);
		fm = rl2GeomImportF32 (blob + (*offset + 12), endian,
				       endian_arch);
		x = last_x + fx;
		y = last_y + fy;
		z = last_z + fz;
		m = last_m + fm;
		*offset += 20;
	    }
	  rl2SetPointZM (line->coords, iv, x, y, z, m);
	  if (x < line->minx)
	      line->minx = x;
	  if (x > line->maxx)
	      line->maxx = x;
	  if (y < line->miny)
	      line->miny = y;
	  if (y > line->maxy)
	      line->maxy = y;
	  last_x = x;
	  last_y = y;
	  last_z = z;
	  last_m = m;
      }
}

static void
rl2ParseCompressedPolygon (rl2GeometryPtr geom, const unsigned char *blob,
			   int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED POLYGON from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (8 * nverts) + 16)
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		if (iv == 0 || iv == (nverts - 1))
		  {
		      /* first and last vertices are uncompressed */
		      x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		      y = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      *offset += 16;
		  }
		else
		  {
		      /* any other intermediate vertex is compressed */
		      fx = rl2GeomImportF32 (blob + *offset, endian,
					     endian_arch);
		      fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
					     endian_arch);
		      x = last_x + fx;
		      y = last_y + fy;
		      *offset += 8;
		  }
		rl2SetPoint (ring->coords, iv, x, y);
		if (x < ring->minx)
		    ring->minx = x;
		if (x > ring->maxx)
		    ring->maxx = x;
		if (y < ring->miny)
		    ring->miny = y;
		if (y > ring->maxy)
		    ring->maxy = y;
		last_x = x;
		last_y = y;
	    }
      }
}

static void
rl2ParseCompressedPolygonZ (rl2GeometryPtr geom, const unsigned char *blob,
			    int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED POLYGONZ from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double last_x = 0.0;
    double last_y = 0.0;
    double last_z = 0.0;
    float fx;
    float fy;
    float fz;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (12 * nverts) + 24)
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		if (iv == 0 || iv == (nverts - 1))
		  {
		      /* first and last vertices are uncompressed */
		      x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		      y = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      z = rl2GeomImport64 (blob + (*offset + 16), endian,
					   endian_arch);
		      *offset += 24;
		  }
		else
		  {
		      /* any other intermediate vertex is compressed */
		      fx = rl2GeomImportF32 (blob + *offset, endian,
					     endian_arch);
		      fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
					     endian_arch);
		      fz = rl2GeomImportF32 (blob + (*offset + 8), endian,
					     endian_arch);
		      x = last_x + fx;
		      y = last_y + fy;
		      z = last_z + fz;
		      *offset += 12;
		  }
		rl2SetPointZ (ring->coords, iv, x, y, z);
		if (x < ring->minx)
		    ring->minx = x;
		if (x > ring->maxx)
		    ring->maxx = x;
		if (y < ring->miny)
		    ring->miny = y;
		if (y > ring->maxy)
		    ring->maxy = y;
		last_x = x;
		last_y = y;
		last_z = z;
	    }
      }
}

static void
rl2ParseCompressedPolygonM (rl2GeometryPtr geom, const unsigned char *blob,
			    int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED POLYGONM from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double m;
    double last_x = 0.0;
    double last_y = 0.0;
    float fx;
    float fy;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (16 * nverts) + 16)
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		if (iv == 0 || iv == (nverts - 1))
		  {
		      /* first and last vertices are uncompressed */
		      x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		      y = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      m = rl2GeomImport64 (blob + (*offset + 16), endian,
					   endian_arch);
		      *offset += 24;
		  }
		else
		  {
		      /* any other intermediate vertex is compressed */
		      fx = rl2GeomImportF32 (blob + *offset, endian,
					     endian_arch);
		      fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
					     endian_arch);
		      m = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      x = last_x + fx;
		      y = last_y + fy;
		      *offset += 16;
		  }
		rl2SetPointM (ring->coords, iv, x, y, m);
		if (x < ring->minx)
		    ring->minx = x;
		if (x > ring->maxx)
		    ring->maxx = x;
		if (y < ring->miny)
		    ring->miny = y;
		if (y > ring->maxy)
		    ring->maxy = y;
		last_x = x;
		last_y = y;
	    }
      }
}

static void
rl2ParseCompressedPolygonZM (rl2GeometryPtr geom, const unsigned char *blob,
			     int size, int endian, int endian_arch, int *offset)
{
/* decodes a COMPRESSED POLYGONZM from WKB */
    int rings;
    int nverts;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;
    double last_x = 0.0;
    double last_y = 0.0;
    double last_z = 0.0;
    float fx;
    float fy;
    float fz;
    rl2PolygonPtr polyg = NULL;
    rl2RingPtr ring;
    if (size < *offset + 4)
	return;
    rings = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (size < *offset + 4)
	      return;
	  nverts = rl2GeomImport32 (blob + *offset, endian, endian_arch);
	  *offset += 4;
	  if (size < *offset + (20 * nverts) + 24)
	      return;
	  if (ib == 0)
	    {
		polyg = rl2AddPolygonToGeometry (geom, nverts, rings - 1);
		ring = polyg->exterior;
	    }
	  else
	      ring = rl2AddInteriorRing (polyg, ib - 1, nverts);
	  for (iv = 0; iv < nverts; iv++)
	    {
		if (iv == 0 || iv == (nverts - 1))
		  {
		      /* first and last vertices are uncompressed */
		      x = rl2GeomImport64 (blob + *offset, endian, endian_arch);
		      y = rl2GeomImport64 (blob + (*offset + 8), endian,
					   endian_arch);
		      z = rl2GeomImport64 (blob + (*offset + 16), endian,
					   endian_arch);
		      m = rl2GeomImport64 (blob + (*offset + 24), endian,
					   endian_arch);
		      *offset += 32;
		  }
		else
		  {
		      /* any other intermediate vertex is compressed */
		      fx = rl2GeomImportF32 (blob + *offset, endian,
					     endian_arch);
		      fy = rl2GeomImportF32 (blob + (*offset + 4), endian,
					     endian_arch);
		      fz = rl2GeomImportF32 (blob + (*offset + 8), endian,
					     endian_arch);
		      m = rl2GeomImport64 (blob + (*offset + 12), endian,
					   endian_arch);
		      x = last_x + fx;
		      y = last_y + fy;
		      z = last_z + fz;
		      *offset += 20;
		  }
		rl2SetPointZM (ring->coords, iv, x, y, z, m);
		if (x < ring->minx)
		    ring->minx = x;
		if (x > ring->maxx)
		    ring->maxx = x;
		if (y < ring->miny)
		    ring->miny = y;
		if (y > ring->maxy)
		    ring->maxy = y;
		last_x = x;
		last_y = y;
		last_z = z;
	    }
      }
}

static void
rl2ParseGeometry (rl2GeometryPtr geom, const unsigned char *blob, int size,
		  int endian, int endian_arch, int *offset)
{
/* decodes a MULTIxx or GEOMETRYCOLLECTION from SpatiaLite BLOB */
    int entities;
    int type;
    int ie;
    if (size < *offset + 4)
	return;
    entities = rl2GeomImport32 (blob + *offset, endian, endian_arch);
    *offset += 4;
    for (ie = 0; ie < entities; ie++)
      {
	  if (size < *offset + 5)
	      return;
	  type = rl2GeomImport32 (blob + *offset + 1, endian, endian_arch);
	  *offset += 5;
	  switch (type)
	    {
	    case GAIA_POINT:
		rl2ParsePoint (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POINTZ:
		rl2ParsePointZ (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POINTM:
		rl2ParsePointM (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POINTZM:
		rl2ParsePointZM (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_LINESTRING:
		rl2ParseLine (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_LINESTRINGZ:
		rl2ParseLineZ (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_LINESTRINGM:
		rl2ParseLineM (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_LINESTRINGZM:
		rl2ParseLineZM (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POLYGON:
		rl2ParsePolygon (geom, blob, size, endian, endian_arch, offset);
		break;
	    case GAIA_POLYGONZ:
		rl2ParsePolygonZ (geom, blob, size, endian, endian_arch,
				  offset);
		break;
	    case GAIA_POLYGONM:
		rl2ParsePolygonM (geom, blob, size, endian, endian_arch,
				  offset);
		break;
	    case GAIA_POLYGONZM:
		rl2ParsePolygonZM (geom, blob, size, endian, endian_arch,
				   offset);
		break;
	    case GAIA_COMPRESSED_LINESTRING:
		rl2ParseCompressedLine (geom, blob, size, endian, endian_arch,
					offset);
		break;
	    case GAIA_COMPRESSED_LINESTRINGZ:
		rl2ParseCompressedLineZ (geom, blob, size, endian,
					 endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_LINESTRINGM:
		rl2ParseCompressedLineM (geom, blob, size, endian,
					 endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_LINESTRINGZM:
		rl2ParseCompressedLineZM (geom, blob, size, endian,
					  endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_POLYGON:
		rl2ParseCompressedPolygon (geom, blob, size, endian,
					   endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_POLYGONZ:
		rl2ParseCompressedPolygonZ (geom, blob, size, endian,
					    endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_POLYGONM:
		rl2ParseCompressedPolygonM (geom, blob, size, endian,
					    endian_arch, offset);
		break;
	    case GAIA_COMPRESSED_POLYGONZM:
		rl2ParseCompressedPolygonZM (geom, blob, size, endian,
					     endian_arch, offset);
		break;
	    default:
		break;
	    };
      }
}

RL2_PRIVATE rl2GeometryPtr
rl2_geometry_from_blob (const unsigned char *blob, int size)
{
/* decoding from SpatiaLite BLOB to GEOMETRY */
    int type;
    int dims;
    int little_endian;
    int offset;
    int endian_arch = rl2GeomEndianArch ();
    rl2GeometryPtr geom = NULL;

    if (size == 24 || size == 32 || size == 40)
      {
	  /* testing for a possible TinyPoint BLOB */
	  if (*(blob + 0) == GAIA_MARK_START &&
	      (*(blob + 1) == GAIA_TINYPOINT_LITTLE_ENDIAN
	       || *(blob + 1) == GAIA_TINYPOINT_BIG_ENDIAN)
	      && *(blob + (size - 1)) == GAIA_MARK_END)
	      return rl2ParseTinyPointBlob (blob, size);
      }

    if (size < 45)
	return NULL;		/* cannot be an internal BLOB WKB geometry */
    if (*(blob + 0) != GAIA_MARK_START)
	return NULL;		/* failed to recognize START signature */
    if (*(blob + (size - 1)) != GAIA_MARK_END)
	return NULL;		/* failed to recognize END signature */
    if (*(blob + 38) != GAIA_MARK_MBR)
	return NULL;		/* failed to recognize MBR signature */
    if (*(blob + 1) == GAIA_LITTLE_ENDIAN)
	little_endian = 1;
    else if (*(blob + 1) == GAIA_BIG_ENDIAN)
	little_endian = 0;
    else
	return NULL;		/* unknown encoding; nor little-endian neither big-endian */
    type = rl2GeomImport32 (blob + 39, little_endian, endian_arch);
    offset = 43;
    switch (type)
      {
      case GAIA_POINTZ:
      case GAIA_LINESTRINGZ:
      case GAIA_POLYGONZ:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_COMPRESSED_LINESTRINGZ:
      case GAIA_COMPRESSED_POLYGONZ:
	  dims = GAIA_XY_Z;
	  break;
      case GAIA_POINTM:
      case GAIA_LINESTRINGM:
      case GAIA_POLYGONM:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTIPOLYGONM:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_COMPRESSED_LINESTRINGM:
      case GAIA_COMPRESSED_POLYGONM:
	  dims = GAIA_XY_M;
	  break;
      case GAIA_POINTZM:
      case GAIA_LINESTRINGZM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGONZM:
      case GAIA_GEOMETRYCOLLECTIONZM:
      case GAIA_COMPRESSED_LINESTRINGZM:
      case GAIA_COMPRESSED_POLYGONZM:
	  dims = GAIA_XY_Z_M;
	  break;
      case GAIA_POINT:
      case GAIA_LINESTRING:
      case GAIA_POLYGON:
      case GAIA_MULTIPOINT:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTIPOLYGON:
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_COMPRESSED_LINESTRING:
      case GAIA_COMPRESSED_POLYGON:
      default:
	  dims = GAIA_XY;
	  break;
      };
    geom = rl2CreateGeometry (dims, type);
    geom->srid = rl2GeomImport32 (blob + 2, little_endian, endian_arch);
    geom->type = type;
    switch (type)
      {
	  /* parsing elementary geometries */
      case GAIA_POINT:
	  rl2ParsePoint (geom, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_POINTZ:
	  rl2ParsePointZ (geom, blob, size, little_endian, endian_arch,
			  &offset);
	  break;
      case GAIA_POINTM:
	  rl2ParsePointM (geom, blob, size, little_endian, endian_arch,
			  &offset);
	  break;
      case GAIA_POINTZM:
	  rl2ParsePointZM (geom, blob, size, little_endian, endian_arch,
			   &offset);
	  break;
      case GAIA_LINESTRING:
	  rl2ParseLine (geom, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_LINESTRINGZ:
	  rl2ParseLineZ (geom, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_LINESTRINGM:
	  rl2ParseLineM (geom, blob, size, little_endian, endian_arch, &offset);
	  break;
      case GAIA_LINESTRINGZM:
	  rl2ParseLineZM (geom, blob, size, little_endian, endian_arch,
			  &offset);
	  break;
      case GAIA_POLYGON:
	  rl2ParsePolygon (geom, blob, size, little_endian, endian_arch,
			   &offset);
	  break;
      case GAIA_POLYGONZ:
	  rl2ParsePolygonZ (geom, blob, size, little_endian, endian_arch,
			    &offset);
	  break;
      case GAIA_POLYGONM:
	  rl2ParsePolygonM (geom, blob, size, little_endian, endian_arch,
			    &offset);
	  break;
      case GAIA_POLYGONZM:
	  rl2ParsePolygonZM (geom, blob, size, little_endian, endian_arch,
			     &offset);
	  break;
      case GAIA_COMPRESSED_LINESTRING:
	  rl2ParseCompressedLine (geom, blob, size, little_endian,
				  endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_LINESTRINGZ:
	  rl2ParseCompressedLineZ (geom, blob, size, little_endian,
				   endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_LINESTRINGM:
	  rl2ParseCompressedLineM (geom, blob, size, little_endian,
				   endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_LINESTRINGZM:
	  rl2ParseCompressedLineZM (geom, blob, size, little_endian,
				    endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_POLYGON:
	  rl2ParseCompressedPolygon (geom, blob, size, little_endian,
				     endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_POLYGONZ:
	  rl2ParseCompressedPolygonZ (geom, blob, size, little_endian,
				      endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_POLYGONM:
	  rl2ParseCompressedPolygonM (geom, blob, size, little_endian,
				      endian_arch, &offset);
	  break;
      case GAIA_COMPRESSED_POLYGONZM:
	  rl2ParseCompressedPolygonZM (geom, blob, size, little_endian,
				       endian_arch, &offset);
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  rl2ParseGeometry (geom, blob, size, little_endian, endian_arch,
			    &offset);
	  break;
      default:
	  break;
      };
    geom->minx = rl2GeomImport64 (blob + 6, little_endian, endian_arch);
    geom->miny = rl2GeomImport64 (blob + 14, little_endian, endian_arch);
    geom->maxx = rl2GeomImport64 (blob + 22, little_endian, endian_arch);
    geom->maxy = rl2GeomImport64 (blob + 30, little_endian, endian_arch);
    return geom;
}

static void
do_update_mbr (rl2GeometryPtr geom)
{
/* updating the Geometry BBOX */
    rl2PointPtr pt;
    rl2LinestringPtr ln;
    rl2PolygonPtr pg;
    rl2RingPtr rng;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;

    geom->minx = DBL_MAX;
    geom->miny = DBL_MAX;
    geom->maxx = 0.0 - DBL_MAX;
    geom->maxy = 0.0 - DBL_MAX;

    pt = geom->first_point;
    while (pt != NULL)
      {
	  /* POINTs */
	  if (pt->x < geom->minx)
	      geom->minx = pt->x;
	  if (pt->x > geom->maxx)
	      geom->maxx = pt->x;
	  if (pt->y < geom->miny)
	      geom->miny = pt->y;
	  if (pt->y > geom->maxy)
	      geom->maxy = pt->y;
	  pt = pt->next;
      }

    ln = geom->first_linestring;
    while (ln != NULL)
      {
	  /* LINESTRINGs */
	  ln->minx = DBL_MAX;
	  ln->miny = DBL_MAX;
	  ln->maxx = 0.0 - DBL_MAX;
	  ln->maxy = 0.0 - DBL_MAX;
	  for (iv = 0; iv < ln->points; iv++)
	    {
		if (ln->dims == GAIA_XY_Z_M)
		  {
		      rl2GetPointZM (ln->coords, iv, &x, &y, &z, &m);
		  }
		else if (ln->dims == GAIA_XY_Z)
		  {
		      rl2GetPointZ (ln->coords, iv, &x, &y, &z);
		  }
		else if (ln->dims == GAIA_XY_M)
		  {
		      rl2GetPointM (ln->coords, iv, &x, &y, &m);
		  }
		else if (ln->dims == GAIA_XY)
		  {
		      rl2GetPoint (ln->coords, iv, &x, &y);
		  }
		if (x < ln->minx)
		    ln->minx = x;
		if (x > ln->maxx)
		    ln->maxx = x;
		if (y < ln->miny)
		    ln->miny = y;
		if (y > ln->maxy)
		    ln->maxy = y;
		if (x < geom->minx)
		    geom->minx = x;
		if (x > geom->maxx)
		    geom->maxx = x;
		if (y < geom->miny)
		    geom->miny = y;
		if (y > geom->maxy)
		    geom->maxy = y;
	    }
	  ln = ln->next;
      }

    pg = geom->first_polygon;
    while (pg != NULL)
      {
	  /* POLYGONs */
	  rng = pg->exterior;
	  rng->minx = DBL_MAX;
	  rng->miny = DBL_MAX;
	  rng->maxx = 0.0 - DBL_MAX;
	  rng->maxy = 0.0 - DBL_MAX;
	  for (iv = 0; iv < rng->points; iv++)
	    {
		/* EXTERIOR RING */
		if (rng->dims == GAIA_XY_Z_M)
		  {
		      rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
		  }
		else if (rng->dims == GAIA_XY_Z)
		  {
		      rl2GetPointZ (rng->coords, iv, &x, &y, &z);
		  }
		else if (rng->dims == GAIA_XY_M)
		  {
		      rl2GetPointM (rng->coords, iv, &x, &y, &m);
		  }
		else if (rng->dims == GAIA_XY)
		  {
		      rl2GetPoint (rng->coords, iv, &x, &y);
		  }
		if (x < rng->minx)
		    rng->minx = x;
		if (x > rng->maxx)
		    rng->maxx = x;
		if (y < rng->miny)
		    rng->miny = y;
		if (y > rng->maxy)
		    rng->maxy = y;
		if (x < geom->minx)
		    geom->minx = x;
		if (x > geom->maxx)
		    geom->maxx = x;
		if (y < geom->miny)
		    geom->miny = y;
		if (y > geom->maxy)
		    geom->maxy = y;
	    }
	  for (ib = 0; ib < pg->num_interiors; ib++)
	    {
		/* INTERIOR RINGS */
		rng = pg->interiors + ib;
		rng->minx = DBL_MAX;
		rng->miny = DBL_MAX;
		rng->maxx = 0.0 - DBL_MAX;
		rng->maxy = 0.0 - DBL_MAX;
		for (iv = 0; iv < rng->points; iv++)
		  {
		      if (rng->dims == GAIA_XY_Z_M)
			{
			    rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
			}
		      else if (rng->dims == GAIA_XY_Z)
			{
			    rl2GetPointZ (rng->coords, iv, &x, &y, &z);
			}
		      else if (rng->dims == GAIA_XY_M)
			{
			    rl2GetPointM (rng->coords, iv, &x, &y, &m);
			}
		      else if (rng->dims == GAIA_XY)
			{
			    rl2GetPoint (rng->coords, iv, &x, &y);
			}
		      if (x < rng->minx)
			  rng->minx = x;
		      if (x > rng->maxx)
			  rng->maxx = x;
		      if (y < rng->miny)
			  rng->miny = y;
		      if (y > rng->maxy)
			  rng->maxy = y;
		      if (x < geom->minx)
			  geom->minx = x;
		      if (x > geom->maxx)
			  geom->maxx = x;
		      if (y < geom->miny)
			  geom->miny = y;
		      if (y > geom->maxy)
			  geom->maxy = y;
		  }
	    }
	  pg = pg->next;
      }
}

RL2_PRIVATE int
rl2_geometry_to_blob (rl2GeometryPtr geom, unsigned char **result, int *size)
{
/* builds the SpatiaLite BLOB representation for this GEOMETRY */
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;
    int entities = 0;
    int n_points = 0;
    int n_linestrings = 0;
    int n_polygons = 0;
    int type;
    unsigned char *ptr;
    rl2PointPtr pt;
    rl2LinestringPtr ln;
    rl2PolygonPtr pg;
    rl2RingPtr rng;
    rl2PointPtr point = NULL;
    rl2LinestringPtr line = NULL;
    rl2PolygonPtr polyg = NULL;
    int endian_arch = rl2GeomEndianArch ();

/* setting up the MBR */
    do_update_mbr (geom);

/* how many entities, and of what kind, do we have ? */
    pt = geom->first_point;
    while (pt)
      {
	  point = pt;
	  entities++;
	  n_points++;
	  pt = pt->next;
      }
    ln = geom->first_linestring;
    while (ln)
      {
	  line = ln;
	  entities++;
	  n_linestrings++;
	  ln = ln->next;
      }
    pg = geom->first_polygon;
    while (pg)
      {
	  polyg = pg;
	  entities++;
	  n_polygons++;
	  pg = pg->next;
      }
    *size = 0;
    *result = NULL;
    if (n_points == 0 && n_polygons == 0 && n_linestrings == 0)
	return 0;
/* ok, we can determine the geometry class */
    if (n_points == 1 && n_linestrings == 0 && n_polygons == 0)
      {
	  if (geom->type == GAIA_MULTIPOINT)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_MULTIPOINTZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_MULTIPOINTM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_MULTIPOINTZM;
		else
		    type = GAIA_MULTIPOINT;
	    }
	  else if (geom->type == GAIA_GEOMETRYCOLLECTION)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_GEOMETRYCOLLECTIONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_GEOMETRYCOLLECTIONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_GEOMETRYCOLLECTIONZM;
		else
		    type = GAIA_GEOMETRYCOLLECTION;
	    }
	  else
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_POINTZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_POINTM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_POINTZM;
		else
		    type = GAIA_POINT;
	    }
      }
    else if (n_points > 1 && n_linestrings == 0 && n_polygons == 0)
      {
	  if (geom->type == GAIA_GEOMETRYCOLLECTION)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_GEOMETRYCOLLECTIONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_GEOMETRYCOLLECTIONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_GEOMETRYCOLLECTIONZM;
		else
		    type = GAIA_GEOMETRYCOLLECTION;
	    }
	  else
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_MULTIPOINTZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_MULTIPOINTM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_MULTIPOINTZM;
		else
		    type = GAIA_MULTIPOINT;
	    }
      }
    else if (n_points == 0 && n_linestrings == 1 && n_polygons == 0)
      {
	  if (geom->type == GAIA_MULTILINESTRING)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_MULTILINESTRINGZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_MULTILINESTRINGM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_MULTILINESTRINGZM;
		else
		    type = GAIA_MULTILINESTRING;
	    }
	  else if (geom->type == GAIA_GEOMETRYCOLLECTION)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_GEOMETRYCOLLECTIONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_GEOMETRYCOLLECTIONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_GEOMETRYCOLLECTIONZM;
		else
		    type = GAIA_GEOMETRYCOLLECTION;
	    }
	  else
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_LINESTRINGZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_LINESTRINGM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_LINESTRINGZM;
		else
		    type = GAIA_LINESTRING;
	    }
      }
    else if (n_points == 0 && n_linestrings > 1 && n_polygons == 0)
      {
	  if (geom->type == GAIA_GEOMETRYCOLLECTION)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_GEOMETRYCOLLECTIONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_GEOMETRYCOLLECTIONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_GEOMETRYCOLLECTIONZM;
		else
		    type = GAIA_GEOMETRYCOLLECTION;
	    }
	  else
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_MULTILINESTRINGZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_MULTILINESTRINGM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_MULTILINESTRINGZM;
		else
		    type = GAIA_MULTILINESTRING;
	    }
      }
    else if (n_points == 0 && n_linestrings == 0 && n_polygons == 1)
      {
	  if (geom->type == GAIA_MULTIPOLYGON)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_MULTIPOLYGONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_MULTIPOLYGONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_MULTIPOLYGONZM;
		else
		    type = GAIA_MULTIPOLYGON;
	    }
	  else if (geom->type == GAIA_GEOMETRYCOLLECTION)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_GEOMETRYCOLLECTIONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_GEOMETRYCOLLECTIONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_GEOMETRYCOLLECTIONZM;
		else
		    type = GAIA_GEOMETRYCOLLECTION;
	    }
	  else
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_POLYGONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_POLYGONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_POLYGONZM;
		else
		    type = GAIA_POLYGON;
	    }
      }
    else if (n_points == 0 && n_linestrings == 0 && n_polygons > 1)
      {
	  if (geom->type == GAIA_GEOMETRYCOLLECTION)
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_GEOMETRYCOLLECTIONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_GEOMETRYCOLLECTIONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_GEOMETRYCOLLECTIONZM;
		else
		    type = GAIA_GEOMETRYCOLLECTION;
	    }
	  else
	    {
		if (geom->dims == GAIA_XY_Z)
		    type = GAIA_MULTIPOLYGONZ;
		else if (geom->dims == GAIA_XY_M)
		    type = GAIA_MULTIPOLYGONM;
		else if (geom->dims == GAIA_XY_Z_M)
		    type = GAIA_MULTIPOLYGONZM;
		else
		    type = GAIA_MULTIPOLYGON;
	    }
      }
    else
      {
	  if (geom->dims == GAIA_XY_Z)
	      type = GAIA_GEOMETRYCOLLECTIONZ;
	  else if (geom->dims == GAIA_XY_M)
	      type = GAIA_GEOMETRYCOLLECTIONM;
	  else if (geom->dims == GAIA_XY_Z_M)
	      type = GAIA_GEOMETRYCOLLECTIONZM;
	  else
	      type = GAIA_GEOMETRYCOLLECTION;
      }
/* and now we compute the size of BLOB */
    *size = 44;			/* header size */
    switch (type)
      {
      case GAIA_POINT:
	  *size += (sizeof (double) * 2);	/* [x,y] coords */
	  break;
      case GAIA_POINTZ:
	  *size += (sizeof (double) * 3);	/* [x,y,z] coords */
	  break;
      case GAIA_POINTM:
	  *size += (sizeof (double) * 3);	/* [x,y,m] coords */
	  break;
      case GAIA_POINTZM:
	  *size += (sizeof (double) * 4);	/* [x,y,z,m] coords */
	  break;
      case GAIA_LINESTRING:
	  *size += (4 + ((sizeof (double) * 2) * line->points));	/* # points + [x,y] for each vertex */
	  break;
      case GAIA_LINESTRINGZ:
	  *size += (4 + ((sizeof (double) * 3) * line->points));	/* # points + [x,y,z] for each vertex */
	  break;
      case GAIA_LINESTRINGM:
	  *size += (4 + ((sizeof (double) * 3) * line->points));	/* # points + [x,y,m] for each vertex */
	  break;
      case GAIA_LINESTRINGZM:
	  *size += (4 + ((sizeof (double) * 4) * line->points));	/* # points + [x,y,z,m] for each vertex */
	  break;
      case GAIA_POLYGON:
	  rng = polyg->exterior;
	  *size += (8 + ((sizeof (double) * 2) * rng->points));	/* # rings + # points + [x.y] array - exterior ring */
	  for (ib = 0; ib < polyg->num_interiors; ib++)
	    {
		rng = polyg->interiors + ib;
		*size += (4 + ((sizeof (double) * 2) * rng->points));	/* # points + [x,y] array - interior ring */
	    }
	  break;
      case GAIA_POLYGONZ:
	  rng = polyg->exterior;
	  *size += (8 + ((sizeof (double) * 3) * rng->points));	/* # rings + # points + [x,y,z] array - exterior ring */
	  for (ib = 0; ib < polyg->num_interiors; ib++)
	    {
		rng = polyg->interiors + ib;
		*size += (4 + ((sizeof (double) * 3) * rng->points));	/* # points + [x,y,z] array - interior ring */
	    }
	  break;
      case GAIA_POLYGONM:
	  rng = polyg->exterior;
	  *size += (8 + ((sizeof (double) * 3) * rng->points));	/* # rings + # points + [x,y,m] array - exterior ring */
	  for (ib = 0; ib < polyg->num_interiors; ib++)
	    {
		rng = polyg->interiors + ib;
		*size += (4 + ((sizeof (double) * 3) * rng->points));	/* # points + [x,y,m] array - interior ring */
	    }
	  break;
      case GAIA_POLYGONZM:
	  rng = polyg->exterior;
	  *size += (8 + ((sizeof (double) * 4) * rng->points));	/* # rings + # points + [x,y,z,m] array - exterior ring */
	  for (ib = 0; ib < polyg->num_interiors; ib++)
	    {
		rng = polyg->interiors + ib;
		*size += (4 + ((sizeof (double) * 4) * rng->points));	/* # points + [x,y,z,m] array - interior ring */
	    }
	  break;
      default:
	  /* this one is not a simple geometry; should be a MULTIxxxx or a GEOMETRYCOLLECTION */
	  *size += 4;		/* # entities */
	  point = geom->first_point;
	  while (point)
	    {
		*size += 5;	/* entity header */
		if (geom->dims == GAIA_XY_Z || geom->dims == GAIA_XY_M)
		    *size += (sizeof (double) * 3);	/* three doubles for each POINT */
		else if (geom->dims == GAIA_XY_Z_M)
		    *size += (sizeof (double) * 4);	/* four doubles for each POINT */
		else
		    *size += (sizeof (double) * 2);	/* two doubles for each POINT */
		point = point->next;
	    }
	  line = geom->first_linestring;
	  while (line)
	    {
		*size += 5;	/* entity header */
		if (geom->dims == GAIA_XY_Z || geom->dims == GAIA_XY_M)
		    *size += (4 + ((sizeof (double) * 3) * line->points));	/* # points + [x,y,z] for each vertex */
		else if (geom->dims == GAIA_XY_Z_M)
		    *size += (4 + ((sizeof (double) * 4) * line->points));	/* # points + [x,y,z,m] for each vertex */
		else
		    *size += (4 + ((sizeof (double) * 2) * line->points));	/* # points + [x,y] for each vertex */
		line = line->next;
	    }
	  polyg = geom->first_polygon;
	  while (polyg)
	    {
		*size += 5;	/* entity header */
		rng = polyg->exterior;
		if (geom->dims == GAIA_XY_Z || geom->dims == GAIA_XY_M)
		    *size += (8 + ((sizeof (double) * 3) * rng->points));	/* # rings + # points + [x,y,z] array - exterior ring */
		else if (geom->dims == GAIA_XY_Z_M)
		    *size += (8 + ((sizeof (double) * 4) * rng->points));	/* # rings + # points + [x,y,z,m] array - exterior ring */
		else
		    *size += (8 + ((sizeof (double) * 2) * rng->points));	/* # rings + # points + [x,y] array - exterior ring */
		for (ib = 0; ib < polyg->num_interiors; ib++)
		  {
		      rng = polyg->interiors + ib;
		      if (geom->dims == GAIA_XY_Z || geom->dims == GAIA_XY_M)
			  *size += (4 + ((sizeof (double) * 3) * rng->points));	/* # points + [x,y,z] array - interior ring */
		      else if (geom->dims == GAIA_XY_Z_M)
			  *size += (4 + ((sizeof (double) * 4) * rng->points));	/* # points + [x,y,z,m] array - interior ring */
		      else
			  *size += (4 + ((sizeof (double) * 2) * rng->points));	/* # points + [x,y] array - interior ring */
		  }
		polyg = polyg->next;
	    }
      };
    *result = malloc (*size);
    ptr = *result;
/* and finally we build the BLOB */
    switch (type)
      {
      case GAIA_POINT:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum x */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum x */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_POINT, 1, endian_arch);	/* class POINT */
	  rl2GeomExport64 (ptr + 43, point->x, 1, endian_arch);	/* x */
	  rl2GeomExport64 (ptr + 51, point->y, 1, endian_arch);	/* y */
	  *(ptr + 59) = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_POINTZ:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_POINTZ, 1, endian_arch);	/* class POINT XYZ */
	  rl2GeomExport64 (ptr + 43, point->x, 1, endian_arch);	/* X */
	  rl2GeomExport64 (ptr + 51, point->y, 1, endian_arch);	/* Y */
	  rl2GeomExport64 (ptr + 59, point->z, 1, endian_arch);	/* Z */
	  *(ptr + 67) = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_POINTM:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_POINTM, 1, endian_arch);	/* class POINT XYM */
	  rl2GeomExport64 (ptr + 43, point->x, 1, endian_arch);	/* X */
	  rl2GeomExport64 (ptr + 51, point->y, 1, endian_arch);	/* Y */
	  rl2GeomExport64 (ptr + 59, point->m, 1, endian_arch);	/* M */
	  *(ptr + 67) = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_POINTZM:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_POINTZM, 1, endian_arch);	/* class POINT XYZM */
	  rl2GeomExport64 (ptr + 43, point->x, 1, endian_arch);	/* X */
	  rl2GeomExport64 (ptr + 51, point->y, 1, endian_arch);	/* Y */
	  rl2GeomExport64 (ptr + 59, point->z, 1, endian_arch);	/* M */
	  rl2GeomExport64 (ptr + 67, point->m, 1, endian_arch);	/* Z */
	  *(ptr + 75) = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_LINESTRING:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum x */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum x */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_LINESTRING, 1, endian_arch);	/* class LINESTRING */
	  rl2GeomExport32 (ptr + 43, line->points, 1, endian_arch);	/* # points */
	  ptr += 47;
	  for (iv = 0; iv < line->points; iv++)
	    {
		gaiaGetPoint (line->coords, iv, &x, &y);
		rl2GeomExport64 (ptr, x, 1, endian_arch);
		rl2GeomExport64 (ptr + 8, y, 1, endian_arch);
		ptr += 16;
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_LINESTRINGZ:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_LINESTRINGZ, 1, endian_arch);	/* class LINESTRING XYZ */
	  rl2GeomExport32 (ptr + 43, line->points, 1, endian_arch);	/* # points */
	  ptr += 47;
	  for (iv = 0; iv < line->points; iv++)
	    {
		rl2GetPointZ (line->coords, iv, &x, &y, &z);
		rl2GeomExport64 (ptr, x, 1, endian_arch);
		rl2GeomExport64 (ptr + 8, y, 1, endian_arch);
		rl2GeomExport64 (ptr + 16, z, 1, endian_arch);
		ptr += 24;
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_LINESTRINGM:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_LINESTRINGM, 1, endian_arch);	/* class LINESTRING XYM */
	  rl2GeomExport32 (ptr + 43, line->points, 1, endian_arch);	/* # points */
	  ptr += 47;
	  for (iv = 0; iv < line->points; iv++)
	    {
		rl2GetPointM (line->coords, iv, &x, &y, &m);
		rl2GeomExport64 (ptr, x, 1, endian_arch);
		rl2GeomExport64 (ptr + 8, y, 1, endian_arch);
		rl2GeomExport64 (ptr + 16, m, 1, endian_arch);
		ptr += 24;
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_LINESTRINGZM:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_LINESTRINGZM, 1, endian_arch);	/* class LINESTRING XYZM */
	  rl2GeomExport32 (ptr + 43, line->points, 1, endian_arch);	/* # points */
	  ptr += 47;
	  for (iv = 0; iv < line->points; iv++)
	    {
		rl2GetPointZM (line->coords, iv, &x, &y, &z, &m);
		rl2GeomExport64 (ptr, x, 1, endian_arch);
		rl2GeomExport64 (ptr + 8, y, 1, endian_arch);
		rl2GeomExport64 (ptr + 16, z, 1, endian_arch);
		rl2GeomExport64 (ptr + 24, m, 1, endian_arch);
		ptr += 32;
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_POLYGON:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum x */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum x */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_POLYGON, 1, endian_arch);	/* class POLYGON */
	  rl2GeomExport32 (ptr + 43, polyg->num_interiors + 1, 1, endian_arch);	/* # rings */
	  rng = polyg->exterior;
	  rl2GeomExport32 (ptr + 47, rng->points, 1, endian_arch);	/* # points - exterior ring */
	  ptr += 51;
	  for (iv = 0; iv < rng->points; iv++)
	    {
		gaiaGetPoint (rng->coords, iv, &x, &y);
		rl2GeomExport64 (ptr, x, 1, endian_arch);	/* x - exterior ring */
		rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* y - exterior ring */
		ptr += 16;
	    }
	  for (ib = 0; ib < polyg->num_interiors; ib++)
	    {
		rng = polyg->interiors + ib;
		rl2GeomExport32 (ptr, rng->points, 1, endian_arch);	/* # points - interior ring */
		ptr += 4;
		for (iv = 0; iv < rng->points; iv++)
		  {
		      gaiaGetPoint (rng->coords, iv, &x, &y);
		      rl2GeomExport64 (ptr, x, 1, endian_arch);	/* x - interior ring */
		      rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* y - interior ring */
		      ptr += 16;
		  }
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_POLYGONZ:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_POLYGONZ, 1, endian_arch);	/* class POLYGON XYZ */
	  rl2GeomExport32 (ptr + 43, polyg->num_interiors + 1, 1, endian_arch);	/* # rings */
	  rng = polyg->exterior;
	  rl2GeomExport32 (ptr + 47, rng->points, 1, endian_arch);	/* # points - exterior ring */
	  ptr += 51;
	  for (iv = 0; iv < rng->points; iv++)
	    {
		rl2GetPointZ (rng->coords, iv, &x, &y, &z);
		rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - exterior ring */
		rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - exterior ring */
		rl2GeomExport64 (ptr + 16, z, 1, endian_arch);	/* Z - exterior ring */
		ptr += 24;
	    }
	  for (ib = 0; ib < polyg->num_interiors; ib++)
	    {
		rng = polyg->interiors + ib;
		rl2GeomExport32 (ptr, rng->points, 1, endian_arch);	/* # points - interior ring */
		ptr += 4;
		for (iv = 0; iv < rng->points; iv++)
		  {
		      rl2GetPointZ (rng->coords, iv, &x, &y, &z);
		      rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - interior ring */
		      rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - interior ring */
		      rl2GeomExport64 (ptr + 16, z, 1, endian_arch);	/* Z - interior ring */
		      ptr += 24;
		  }
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_POLYGONM:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_POLYGONM, 1, endian_arch);	/* class POLYGON XYM */
	  rl2GeomExport32 (ptr + 43, polyg->num_interiors + 1, 1, endian_arch);	/* # rings */
	  rng = polyg->exterior;
	  rl2GeomExport32 (ptr + 47, rng->points, 1, endian_arch);	/* # points - exterior ring */
	  ptr += 51;
	  for (iv = 0; iv < rng->points; iv++)
	    {
		rl2GetPointM (rng->coords, iv, &x, &y, &m);
		rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - exterior ring */
		rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - exterior ring */
		rl2GeomExport64 (ptr + 16, m, 1, endian_arch);	/* M - exterior ring */
		ptr += 24;
	    }
	  for (ib = 0; ib < polyg->num_interiors; ib++)
	    {
		rng = polyg->interiors + ib;
		rl2GeomExport32 (ptr, rng->points, 1, endian_arch);	/* # points - interior ring */
		ptr += 4;
		for (iv = 0; iv < rng->points; iv++)
		  {
		      rl2GetPointM (rng->coords, iv, &x, &y, &m);
		      rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - interior ring */
		      rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - interior ring */
		      rl2GeomExport64 (ptr + 16, m, 1, endian_arch);	/* M - interior ring */
		      ptr += 24;
		  }
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
	  break;
      case GAIA_POLYGONZM:
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum X */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum Y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum X */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum Y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, GAIA_POLYGONZM, 1, endian_arch);	/* class POLYGON */
	  rl2GeomExport32 (ptr + 43, polyg->num_interiors + 1, 1, endian_arch);	/* # rings */
	  rng = polyg->exterior;
	  rl2GeomExport32 (ptr + 47, rng->points, 1, endian_arch);	/* # points - exterior ring */
	  ptr += 51;
	  for (iv = 0; iv < rng->points; iv++)
	    {
		rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
		rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - exterior ring */
		rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - exterior ring */
		rl2GeomExport64 (ptr + 16, z, 1, endian_arch);	/* Z - exterior ring */
		rl2GeomExport64 (ptr + 24, m, 1, endian_arch);	/* M - exterior ring */
		ptr += 32;
	    }
	  for (ib = 0; ib < polyg->num_interiors; ib++)
	    {
		rng = polyg->interiors + ib;
		rl2GeomExport32 (ptr, rng->points, 1, endian_arch);	/* # points - interior ring */
		ptr += 4;
		for (iv = 0; iv < rng->points; iv++)
		  {
		      rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
		      rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - interior ring */
		      rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - interior ring */
		      rl2GeomExport64 (ptr + 16, z, 1, endian_arch);	/* Z - exterior ring */
		      rl2GeomExport64 (ptr + 24, m, 1, endian_arch);	/* M - exterior ring */
		      ptr += 32;
		  }
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
	  break;
      default:
	  /* this one is a MULTIxxxx or a GEOMETRyCOLLECTION - building the main header */
	  *ptr = GAIA_MARK_START;	/* START signature */
	  *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
	  rl2GeomExport32 (ptr + 2, geom->srid, 1, endian_arch);	/* the SRID */
	  rl2GeomExport64 (ptr + 6, geom->minx, 1, endian_arch);	/* MBR - minimum x */
	  rl2GeomExport64 (ptr + 14, geom->miny, 1, endian_arch);	/* MBR - minimum y */
	  rl2GeomExport64 (ptr + 22, geom->maxx, 1, endian_arch);	/* MBR - maximum x */
	  rl2GeomExport64 (ptr + 30, geom->maxy, 1, endian_arch);	/* MBR - maximum y */
	  *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
	  rl2GeomExport32 (ptr + 39, type, 1, endian_arch);	/* geometric class */
	  rl2GeomExport32 (ptr + 43, entities, 1, endian_arch);	/* # entities */
	  ptr += 47;
	  point = geom->first_point;
	  while (point)
	    {
		*ptr = GAIA_MARK_ENTITY;	/* ENTITY signature */
		if (geom->dims == GAIA_XY_Z)
		  {
		      rl2GeomExport32 (ptr + 1, GAIA_POINTZ, 1, endian_arch);	/* class POINT XYZ */
		      rl2GeomExport64 (ptr + 5, point->x, 1, endian_arch);	/* X */
		      rl2GeomExport64 (ptr + 13, point->y, 1, endian_arch);	/* Y */
		      rl2GeomExport64 (ptr + 21, point->z, 1, endian_arch);	/* Z */
		      ptr += 29;
		  }
		else if (geom->dims == GAIA_XY_M)
		  {
		      rl2GeomExport32 (ptr + 1, GAIA_POINTM, 1, endian_arch);	/* class POINT XYM */
		      rl2GeomExport64 (ptr + 5, point->x, 1, endian_arch);	/* X */
		      rl2GeomExport64 (ptr + 13, point->y, 1, endian_arch);	/* Y */
		      rl2GeomExport64 (ptr + 21, point->m, 1, endian_arch);	/* M */
		      ptr += 29;
		  }
		else if (geom->dims == GAIA_XY_Z_M)
		  {
		      rl2GeomExport32 (ptr + 1, GAIA_POINTZM, 1, endian_arch);	/* class POINT XYZM */
		      rl2GeomExport64 (ptr + 5, point->x, 1, endian_arch);	/* X */
		      rl2GeomExport64 (ptr + 13, point->y, 1, endian_arch);	/* Y */
		      rl2GeomExport64 (ptr + 21, point->z, 1, endian_arch);	/* Z */
		      rl2GeomExport64 (ptr + 29, point->m, 1, endian_arch);	/* M */
		      ptr += 37;
		  }
		else
		  {
		      rl2GeomExport32 (ptr + 1, GAIA_POINT, 1, endian_arch);	/* class POINT */
		      rl2GeomExport64 (ptr + 5, point->x, 1, endian_arch);	/* X */
		      rl2GeomExport64 (ptr + 13, point->y, 1, endian_arch);	/* Y */
		      ptr += 21;
		  }
		point = point->next;
	    }
	  line = geom->first_linestring;
	  while (line)
	    {
		*ptr = GAIA_MARK_ENTITY;	/* ENTITY signature */
		if (geom->dims == GAIA_XY_Z)
		    rl2GeomExport32 (ptr + 1, GAIA_LINESTRINGZ, 1, endian_arch);	/* class LINESTRING XYZ */
		else if (geom->dims == GAIA_XY_M)
		    rl2GeomExport32 (ptr + 1, GAIA_LINESTRINGM, 1, endian_arch);	/* class LINESTRING XYM */
		else if (geom->dims == GAIA_XY_Z_M)
		    rl2GeomExport32 (ptr + 1, GAIA_LINESTRINGZM, 1, endian_arch);	/* class LINESTRING XYZM */
		else
		    rl2GeomExport32 (ptr + 1, GAIA_LINESTRING, 1, endian_arch);	/* class LINESTRING */
		rl2GeomExport32 (ptr + 5, line->points, 1, endian_arch);	/* # points */
		ptr += 9;
		for (iv = 0; iv < line->points; iv++)
		  {
		      if (geom->dims == GAIA_XY_Z)
			{
			    rl2GetPointZ (line->coords, iv, &x, &y, &z);
			}
		      else if (geom->dims == GAIA_XY_M)
			{
			    rl2GetPointM (line->coords, iv, &x, &y, &m);
			}
		      else if (geom->dims == GAIA_XY_Z_M)
			{
			    rl2GetPointZM (line->coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    rl2GetPoint (line->coords, iv, &x, &y);
			}
		      rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X */
		      rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y */
		      ptr += 16;
		      if (geom->dims == GAIA_XY_Z)
			{
			    rl2GeomExport64 (ptr, z, 1, endian_arch);	/* Z */
			    ptr += 8;
			}
		      if (geom->dims == GAIA_XY_M)
			{
			    rl2GeomExport64 (ptr, m, 1, endian_arch);	/* M */
			    ptr += 8;
			}
		      if (geom->dims == GAIA_XY_Z_M)
			{
			    rl2GeomExport64 (ptr, z, 1, endian_arch);	/* Z */
			    rl2GeomExport64 (ptr + 8, m, 1, endian_arch);	/* M */
			    ptr += 16;
			}
		  }
		line = line->next;
	    }
	  polyg = geom->first_polygon;
	  while (polyg)
	    {
		*ptr = GAIA_MARK_ENTITY;	/* ENTITY signature */
		if (geom->dims == GAIA_XY_Z)
		    rl2GeomExport32 (ptr + 1, GAIA_POLYGONZ, 1, endian_arch);	/* class POLYGON XYZ */
		else if (geom->dims == GAIA_XY_M)
		    rl2GeomExport32 (ptr + 1, GAIA_POLYGONM, 1, endian_arch);	/* class POLYGON XYM */
		else if (geom->dims == GAIA_XY_Z_M)
		    rl2GeomExport32 (ptr + 1, GAIA_POLYGONZM, 1, endian_arch);	/* class POLYGON XYZM */
		else
		    rl2GeomExport32 (ptr + 1, GAIA_POLYGON, 1, endian_arch);	/* class POLYGON */
		rl2GeomExport32 (ptr + 5, polyg->num_interiors + 1, 1, endian_arch);	/* # rings */
		rng = polyg->exterior;
		rl2GeomExport32 (ptr + 9, rng->points, 1, endian_arch);	/* # points - exterior ring */
		ptr += 13;
		for (iv = 0; iv < rng->points; iv++)
		  {
		      if (geom->dims == GAIA_XY_Z)
			{
			    rl2GetPointZ (rng->coords, iv, &x, &y, &z);
			}
		      else if (geom->dims == GAIA_XY_M)
			{
			    rl2GetPointM (rng->coords, iv, &x, &y, &m);
			}
		      else if (geom->dims == GAIA_XY_Z_M)
			{
			    rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    rl2GetPoint (rng->coords, iv, &x, &y);
			}
		      rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - exterior ring */
		      rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - exterior ring */
		      ptr += 16;
		      if (geom->dims == GAIA_XY_Z)
			{
			    rl2GeomExport64 (ptr, z, 1, endian_arch);	/* Z */
			    ptr += 8;
			}
		      if (geom->dims == GAIA_XY_M)
			{
			    rl2GeomExport64 (ptr, m, 1, endian_arch);	/* M */
			    ptr += 8;
			}
		      if (geom->dims == GAIA_XY_Z_M)
			{
			    rl2GeomExport64 (ptr, z, 1, endian_arch);	/* Z */
			    rl2GeomExport64 (ptr + 8, m, 1, endian_arch);	/* M */
			    ptr += 16;
			}
		  }
		for (ib = 0; ib < polyg->num_interiors; ib++)
		  {
		      rng = polyg->interiors + ib;
		      rl2GeomExport32 (ptr, rng->points, 1, endian_arch);	/* # points - interior ring */
		      ptr += 4;
		      for (iv = 0; iv < rng->points; iv++)
			{
			    if (geom->dims == GAIA_XY_Z)
			      {
				  rl2GetPointZ (rng->coords, iv, &x, &y, &z);
			      }
			    else if (geom->dims == GAIA_XY_M)
			      {
				  rl2GetPointM (rng->coords, iv, &x, &y, &m);
			      }
			    else if (geom->dims == GAIA_XY_Z_M)
			      {
				  rl2GetPointZM (rng->coords, iv, &x, &y,
						 &z, &m);
			      }
			    else
			      {
				  rl2GetPoint (rng->coords, iv, &x, &y);
			      }
			    rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - interior ring */
			    rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - interior ring */
			    ptr += 16;
			    if (geom->dims == GAIA_XY_Z)
			      {
				  rl2GeomExport64 (ptr, z, 1, endian_arch);	/* Z */
				  ptr += 8;
			      }
			    if (geom->dims == GAIA_XY_M)
			      {
				  rl2GeomExport64 (ptr, m, 1, endian_arch);	/* M */
				  ptr += 8;
			      }
			    if (geom->dims == GAIA_XY_Z_M)
			      {
				  rl2GeomExport64 (ptr, z, 1, endian_arch);	/* Z */
				  rl2GeomExport64 (ptr + 8, m, 1, endian_arch);	/* M */
				  ptr += 16;
			      }
			}
		  }
		polyg = polyg->next;
	    }
	  *ptr = GAIA_MARK_END;	/* END signature */
      };
    return 1;
}

RL2_PRIVATE void
rl2_destroy_geometry (rl2GeometryPtr geom)
{
/* GEOMETRYCOLLECTION object destructor */
    rl2PointPtr pP;
    rl2PointPtr pPn;
    rl2LinestringPtr pL;
    rl2LinestringPtr pLn;
    rl2PolygonPtr pA;
    rl2PolygonPtr pAn;
    if (geom == NULL)
	return;
    pP = geom->first_point;
    while (pP != NULL)
      {
	  pPn = pP->next;
	  rl2DestroyPoint (pP);
	  pP = pPn;
      }
    pL = geom->first_linestring;
    while (pL != NULL)
      {
	  pLn = pL->next;
	  rl2DestroyLinestring (pL);
	  pL = pLn;
      }
    pA = geom->first_polygon;
    while (pA != NULL)
      {
	  pAn = pA->next;
	  rl2DestroyPolygon (pA);
	  pA = pAn;
      }
    free (geom);
}

RL2_PRIVATE int
rl2_serialize_linestring (rl2LinestringPtr line, unsigned char **result,
			  int *size)
{
/* serializing a BLOB Geometry - linestring */
    int iv;
    unsigned char *ptr;
    int endian_arch = rl2GeomEndianArch ();
    double minx = DBL_MAX;
    double maxx = 0.0 - DBL_MAX;
    double miny = DBL_MAX;
    double maxy = 0.0 - DBL_MAX;
    double x;
    double y;

    *result = NULL;
    *size = 0;
    if (line == NULL)
	return 0;

/* computing the MBR */
    for (iv = 0; iv < line->points; iv++)
      {
	  rl2GetPoint (line->coords, iv, &x, &y);
	  if (x < minx)
	      minx = x;
	  if (x > maxx)
	      maxx = x;
	  if (y < miny)
	      miny = y;
	  if (y > maxy)
	      maxy = y;
      }
/* computing the size of BLOB */
    *size = 44;			/* header size */
    *size += (4 + ((sizeof (double) * 2) * line->points));	/* # points + [x,y] for each vertex */
    *result = malloc (*size);
    ptr = *result;
/* building the BLOB */
    *ptr = GAIA_MARK_START;	/* START signature */
    *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
    rl2GeomExport32 (ptr + 2, 4326, 1, endian_arch);	/* the SRID */
    rl2GeomExport64 (ptr + 6, minx, 1, endian_arch);	/* MBR - minimum X */
    rl2GeomExport64 (ptr + 14, miny, 1, endian_arch);	/* MBR - minimum Y */
    rl2GeomExport64 (ptr + 22, maxx, 1, endian_arch);	/* MBR - maximum X */
    rl2GeomExport64 (ptr + 30, maxy, 1, endian_arch);	/* MBR - maximum Y */
    *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
    rl2GeomExport32 (ptr + 39, GAIA_LINESTRING, 1, endian_arch);	/* class LINESTRING */
    rl2GeomExport32 (ptr + 43, line->points, 1, endian_arch);	/* # points */
    ptr += 47;
    for (iv = 0; iv < line->points; iv++)
      {
	  rl2GetPoint (line->coords, iv, &x, &y);
	  rl2GeomExport64 (ptr, x, 1, endian_arch);
	  rl2GeomExport64 (ptr + 8, y, 1, endian_arch);
	  ptr += 16;
      }
    *ptr = GAIA_MARK_END;	/* END signature */
    return 1;
}

RL2_PRIVATE int
rl2_serialize_ring (rl2RingPtr ring, unsigned char **result, int *size)
{
/* serializing a BLOB Geometry - polygon ring */
    int iv;
    unsigned char *ptr;
    int endian_arch = rl2GeomEndianArch ();
    double minx = DBL_MAX;
    double maxx = 0.0 - DBL_MAX;
    double miny = DBL_MAX;
    double maxy = 0.0 - DBL_MAX;
    double x;
    double y;

    *result = NULL;
    *size = 0;
    if (ring == NULL)
	return 0;

/* computing the MBR */
    for (iv = 0; iv < ring->points; iv++)
      {
	  rl2GetPoint (ring->coords, iv, &x, &y);
	  if (x < minx)
	      minx = x;
	  if (x > maxx)
	      maxx = x;
	  if (y < miny)
	      miny = y;
	  if (y > maxy)
	      maxy = y;
      }
/* computing the size of BLOB */
    *size = 44;			/* header size */
    *size += (8 + ((sizeof (double) * 2) * ring->points));	/* # rings + # points + [x.y] array - exterior ring */
    *result = malloc (*size);
    ptr = *result;
/* building the BLOB */
    *ptr = GAIA_MARK_START;	/* START signature */
    *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
    rl2GeomExport32 (ptr + 2, -1, 1, endian_arch);	/* the SRID */
    rl2GeomExport64 (ptr + 6, minx, 1, endian_arch);	/* MBR - minimum X */
    rl2GeomExport64 (ptr + 14, miny, 1, endian_arch);	/* MBR - minimum Y */
    rl2GeomExport64 (ptr + 22, maxx, 1, endian_arch);	/* MBR - maximum X */
    rl2GeomExport64 (ptr + 30, maxy, 1, endian_arch);	/* MBR - maximum Y */
    *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
    rl2GeomExport32 (ptr + 39, GAIA_POLYGON, 1, endian_arch);	/* class POLYGON */
    rl2GeomExport32 (ptr + 43, 1, 1, endian_arch);	/* # rings */
    rl2GeomExport32 (ptr + 47, ring->points, 1, endian_arch);	/* # points - exterior ring */
    ptr += 51;
    for (iv = 0; iv < ring->points; iv++)
      {
	  rl2GetPoint (ring->coords, iv, &x, &y);
	  rl2GeomExport64 (ptr, x, 1, endian_arch);	/* X - exterior ring */
	  rl2GeomExport64 (ptr + 8, y, 1, endian_arch);	/* Y - exterior ring */
	  ptr += 16;
      }
    *ptr = GAIA_MARK_END;	/* END signature */
    return 1;
}

RL2_PRIVATE int
rl2_serialize_ring_as_linestring (rl2RingPtr ring, unsigned char **result,
				  int *size)
{
/* serializing a BLOB Geometry - polygon ring as linestring */
    int iv;
    unsigned char *ptr;
    int endian_arch = rl2GeomEndianArch ();
    double minx = DBL_MAX;
    double maxx = 0.0 - DBL_MAX;
    double miny = DBL_MAX;
    double maxy = 0.0 - DBL_MAX;
    double x;
    double y;

    *result = NULL;
    *size = 0;
    if (ring == NULL)
	return 0;

/* computing the MBR */
    for (iv = 0; iv < ring->points; iv++)
      {
	  rl2GetPoint (ring->coords, iv, &x, &y);
	  if (x < minx)
	      minx = x;
	  if (x > maxx)
	      maxx = x;
	  if (y < miny)
	      miny = y;
	  if (y > maxy)
	      maxy = y;
      }
/* computing the size of BLOB */
    *size = 44;			/* header size */
    *size += (4 + ((sizeof (double) * 2) * ring->points));	/* # points + [x,y] for each vertex */
    *result = malloc (*size);
    ptr = *result;
/* building the BLOB */
    *ptr = GAIA_MARK_START;	/* START signature */
    *(ptr + 1) = GAIA_LITTLE_ENDIAN;	/* byte ordering */
    rl2GeomExport32 (ptr + 2, 4326, 1, endian_arch);	/* the SRID */
    rl2GeomExport64 (ptr + 6, minx, 1, endian_arch);	/* MBR - minimum X */
    rl2GeomExport64 (ptr + 14, miny, 1, endian_arch);	/* MBR - minimum Y */
    rl2GeomExport64 (ptr + 22, maxx, 1, endian_arch);	/* MBR - maximum X */
    rl2GeomExport64 (ptr + 30, maxy, 1, endian_arch);	/* MBR - maximum Y */
    *(ptr + 38) = GAIA_MARK_MBR;	/* MBR signature */
    rl2GeomExport32 (ptr + 39, GAIA_LINESTRING, 1, endian_arch);	/* class LINESTRING */
    rl2GeomExport32 (ptr + 43, ring->points, 1, endian_arch);	/* # points */
    ptr += 47;
    for (iv = 0; iv < ring->points; iv++)
      {
	  rl2GetPoint (ring->coords, iv, &x, &y);
	  rl2GeomExport64 (ptr, x, 1, endian_arch);
	  rl2GeomExport64 (ptr + 8, y, 1, endian_arch);
	  ptr += 16;
      }
    *ptr = GAIA_MARK_END;	/* END signature */
    return 1;
}

RL2_PRIVATE rl2GeometryPtr
rl2_curve_from_XY (int points, double *x, double *y)
{
/* creating a Linestring Geometry from X,Y coordinate arrays */
    rl2GeometryPtr geom = NULL;
    rl2LinestringPtr ln;
    int iv;

    if (points <= 0 || x == NULL || y == NULL)
	return 0;
    geom = rl2CreateGeometry (GAIA_XY, GAIA_LINESTRING);
    ln = rl2AddLinestringToGeometry (geom, points);
    for (iv = 0; iv < points; iv++)
      {
	  rl2SetPoint (ln->coords, iv, *(x + iv), *(y + iv));
	  if (*(x + iv) < ln->minx)
	      ln->minx = *(x + iv);
	  if (*(x + iv) > ln->maxx)
	      ln->maxx = *(x + iv);
	  if (*(y + iv) < ln->miny)
	      ln->miny = *(y + iv);
	  if (*(y + iv) > ln->maxy)
	      ln->maxy = *(y + iv);
      }
    return geom;
}

RL2_PRIVATE double
rl2_compute_curve_length (rl2GeometryPtr geom)
{
/* computing the total length of some curve (single linestring expected) */
    rl2LinestringPtr ln;
    double length = 0.0;
    double x0;
    double y0;
    double x1;
    double y1;
    int iv;

    if (geom == NULL)
	return 0.0;
    if (geom->first_point != NULL || geom->first_polygon != NULL)
	return 0.0;
    if (geom->first_linestring != geom->last_linestring)
	return 0.0;
    ln = geom->first_linestring;
    if (ln == NULL)
	return 0.0;

    for (iv = 0; iv < ln->points; iv++)
      {
	  rl2GetPoint (ln->coords, iv, &x1, &y1);
	  if (iv > 0)
	      length += sqrt ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
	  x0 = x1;
	  y0 = y1;
      }
    return length;
}

RL2_PRIVATE rl2GeometryPtr
rl2_curve_substring (sqlite3 * handle, rl2GeometryPtr geom, double from,
		     double to)
{
/* extracting a sub-path from some Curve */
    rl2LinestringPtr ln;
    rl2GeometryPtr result = NULL;
    unsigned char *blob;
    int blob_sz;
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    if (handle == NULL)
	return NULL;
    if (geom == NULL)
	return NULL;
    if (geom->first_point != NULL || geom->first_polygon != NULL)
	return NULL;
    if (geom->first_linestring != geom->last_linestring)
	return NULL;
    ln = geom->first_linestring;
    if (ln == NULL)
	return NULL;

    if (!rl2_serialize_linestring (ln, &blob, &blob_sz))
	return NULL;

    sql = "SELECT ST_Line_Substring(?, ?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_sz, free);
    sqlite3_bind_double (stmt, 2, from);
    sqlite3_bind_double (stmt, 3, to);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob = (unsigned char *) sqlite3_column_blob (stmt, 0);
		      blob_sz = sqlite3_column_bytes (stmt, 0);
		      result = rl2_geometry_from_blob (blob, blob_sz);
		  }
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return result;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

RL2_PRIVATE rl2GeometryPtr
rl2_clone_curve (rl2GeometryPtr in)
{
/* cloning a Curve geometry */
    rl2GeometryPtr out;
    rl2LinestringPtr ln_in = in->first_linestring;
    rl2LinestringPtr ln_out;
    int iv;

    out = rl2CreateGeometry (GAIA_XY, GAIA_LINESTRING);
    ln_out = rl2AddLinestringToGeometry (out, ln_in->points);
    for (iv = 0; iv < ln_in->points; iv++)
      {
	  double x;
	  double y;
	  rl2GetPoint (ln_in->coords, iv, &x, &y);
	  rl2SetPoint (ln_out->coords, iv, x, y);
	  if (x < ln_out->minx)
	      ln_out->minx = x;
	  if (x > ln_out->maxx)
	      ln_out->maxx = x;
	  if (y < ln_out->miny)
	      ln_out->miny = y;
	  if (y > ln_out->maxy)
	      ln_out->maxy = y;
      }
    return out;
}

RL2_PRIVATE rl2GeometryPtr
rl2_clone_linestring (rl2LinestringPtr in)
{
/* cloning a Curve geometry */
    rl2GeometryPtr out;
    rl2LinestringPtr ln_out;
    int iv;

    out = rl2CreateGeometry (GAIA_XY, GAIA_LINESTRING);
    ln_out = rl2AddLinestringToGeometry (out, in->points);
    for (iv = 0; iv < in->points; iv++)
      {
	  double x;
	  double y;
	  rl2GetPoint (in->coords, iv, &x, &y);
	  rl2SetPoint (ln_out->coords, iv, x, y);
	  if (x < ln_out->minx)
	      ln_out->minx = x;
	  if (x > ln_out->maxx)
	      ln_out->maxx = x;
	  if (y < ln_out->miny)
	      ln_out->miny = y;
	  if (y > ln_out->maxy)
	      ln_out->maxy = y;
      }
    return out;
}

RL2_PRIVATE rl2GeometryPtr
rl2_clone_polygons (rl2GeometryPtr in)
{
/* cloning a (Multi)Polygon geometry */
    rl2GeometryPtr out;
    rl2PolygonPtr pg_in;
    rl2PolygonPtr pg_out;
    rl2RingPtr rng_in;
    rl2RingPtr rng_out;
    int iv;
    int ib;

    out = rl2CreateGeometry (GAIA_XY, GAIA_MULTIPOLYGON);
    pg_in = in->first_polygon;
    while (pg_in != NULL)
      {
	  rng_in = pg_in->exterior;
	  pg_out =
	      rl2AddPolygonToGeometry (out, rng_in->points,
				       pg_in->num_interiors);
	  rng_out = pg_out->exterior;
	  for (iv = 0; iv < rng_in->points; iv++)
	    {
		/* cloning the exterior ring */
		double x;
		double y;
		rl2GetPoint (rng_in->coords, iv, &x, &y);
		rl2SetPoint (rng_out->coords, iv, x, y);
		if (x < rng_out->minx)
		    rng_out->minx = x;
		if (x > rng_out->maxx)
		    rng_out->maxx = x;
		if (y < rng_out->miny)
		    rng_out->miny = y;
		if (y > rng_out->maxy)
		    rng_out->maxy = y;
	    }
	  for (ib = 0; ib < pg_in->num_interiors; ib++)
	    {
		rng_in = pg_in->interiors + ib;
		rng_out = rl2AddInteriorRing (pg_out, ib, rng_in->points);
		for (iv = 0; iv < rng_in->points; iv++)
		  {
		      /* cloning an interior ring */
		      double x;
		      double y;
		      rl2GetPoint (rng_in->coords, iv, &x, &y);
		      rl2SetPoint (rng_out->coords, iv, x, y);
		      if (x < rng_out->minx)
			  rng_out->minx = x;
		      if (x > rng_out->maxx)
			  rng_out->maxx = x;
		      if (y < rng_out->miny)
			  rng_out->miny = y;
		      if (y > rng_out->maxy)
			  rng_out->maxy = y;
		  }
	    }
	  pg_in = pg_in->next;
      }
    return out;
}

RL2_PRIVATE rl2GeometryPtr
rl2_build_circle (double cx, double cy, double radius)
{
/* creating a circle */
    int iv = 0;
    double pi = 3.14159265359;
    double rads;
    double x;
    double y;
    rl2LinestringPtr ln;
    rl2GeometryPtr out = rl2CreateGeometry (GAIA_XY, GAIA_LINESTRING);
    ln = rl2AddLinestringToGeometry (out, 129);
    for (rads = 0.0; rads <= (pi * 2.0); rads += pi / 64.0)
      {
	  x = cx + (radius * cos (rads));
	  y = cy + (radius * sin (rads));
	  rl2SetPoint (ln->coords, iv, x, y);
	  if (x < ln->minx)
	      ln->minx = x;
	  if (x > ln->maxx)
	      ln->maxx = x;
	  if (y < ln->miny)
	      ln->miny = y;
	  if (y > ln->maxy)
	      ln->maxy = y;
	  iv++;
      }
    /* closure */
    rl2GetPoint (ln->coords, 0, &x, &y);
    rl2SetPoint (ln->coords, 128, x, y);
    return out;
}

RL2_PRIVATE int
rl2_affine_transform_from_blob (rl2PrivAffineTransformPtr matrix,
				const unsigned char *blob, int blob_sz)
{
/* decoding an Affine Transform Matrix object */
    int endian;
    int endian_arch = rl2GeomEndianArch ();
    const unsigned char *ptr = blob;
    if (blob == NULL)
	return 0;
    if (blob_sz != 146)
	return 0;

    if (*ptr != MATRIX_MAGIC_START)
	return 0;
    if (*(ptr + 1) == 1)
	endian = 1;
    else if (*(ptr + 1) == 0)
	endian = 0;
    else
	return 0;
    matrix->xx = rl2GeomImport64 (ptr + 2, endian, endian_arch);
    matrix->xy = rl2GeomImport64 (ptr + 11, endian, endian_arch);
    matrix->xz = rl2GeomImport64 (ptr + 20, endian, endian_arch);
    matrix->xoff = rl2GeomImport64 (ptr + 29, endian, endian_arch);
    matrix->yx = rl2GeomImport64 (ptr + 38, endian, endian_arch);
    matrix->yy = rl2GeomImport64 (ptr + 47, endian, endian_arch);
    matrix->yz = rl2GeomImport64 (ptr + 56, endian, endian_arch);
    matrix->yoff = rl2GeomImport64 (ptr + 65, endian, endian_arch);
    matrix->zx = rl2GeomImport64 (ptr + 74, endian, endian_arch);
    matrix->zy = rl2GeomImport64 (ptr + 83, endian, endian_arch);
    matrix->zz = rl2GeomImport64 (ptr + 92, endian, endian_arch);
    matrix->zoff = rl2GeomImport64 (ptr + 101, endian, endian_arch);
    matrix->w1 = rl2GeomImport64 (ptr + 110, endian, endian_arch);
    matrix->w2 = rl2GeomImport64 (ptr + 119, endian, endian_arch);
    matrix->w3 = rl2GeomImport64 (ptr + 128, endian, endian_arch);
    matrix->w4 = rl2GeomImport64 (ptr + 137, endian, endian_arch);
    if (*(ptr + 145) != MATRIX_MAGIC_END)
	return 0;
    return 1;
}

RL2_PRIVATE rl2DynLinePtr
rl2CreateDynLine ()
{
/* creating an empty DynLine object */
    rl2DynLinePtr dyn = malloc (sizeof (rl2DynLine));
    dyn->first = NULL;
    dyn->last = NULL;
    return dyn;
}

RL2_PRIVATE void
rl2DestroyDynLine (rl2DynLinePtr dyn)
{
/* memory cleanup - destroying a DynLine object */
    rl2PointPtr pt;
    rl2PointPtr pt_n;

    if (dyn == NULL)
	return;

    pt = dyn->first;
    while (pt != NULL)
      {
	  pt_n = pt->next;
	  free (pt);
	  pt = pt_n;
      }
    free (dyn);
}

RL2_PRIVATE void
rl2AddDynPoint (rl2DynLinePtr dyn, double x, double y)
{
/* adding a Point XY to a DynLine */
    rl2PointPtr pt = rl2CreatePointXY (x, y);
    if (dyn->first == NULL)
	dyn->first = pt;
    if (dyn->last != NULL)
	dyn->last->next = pt;
    dyn->last = pt;
}

RL2_PRIVATE void
rl2AddDynPointZ (rl2DynLinePtr dyn, double x, double y, double z)
{
/* adding a Point XYZ to a DynLine */
    rl2PointPtr pt = rl2CreatePointXYZ (x, y, z);
    if (dyn->first == NULL)
	dyn->first = pt;
    if (dyn->last != NULL)
	dyn->last->next = pt;
    dyn->last = pt;
}

RL2_PRIVATE void
rl2AddDynPointM (rl2DynLinePtr dyn, double x, double y, double m)
{
/* adding a Point XYM to a DynLine */
    rl2PointPtr pt = rl2CreatePointXYM (x, y, m);
    if (dyn->first == NULL)
	dyn->first = pt;
    if (dyn->last != NULL)
	dyn->last->next = pt;
    dyn->last = pt;
}

RL2_PRIVATE void
rl2AddDynPointZM (rl2DynLinePtr dyn, double x, double y, double z, double m)
{
/* adding a Point XYZM to a DynLine */
    rl2PointPtr pt = rl2CreatePointXYZM (x, y, z, m);
    if (dyn->first == NULL)
	dyn->first = pt;
    if (dyn->last != NULL)
	dyn->last->next = pt;
    dyn->last = pt;
}

RL2_PRIVATE int
rl2CountDynLinePoints (rl2DynLinePtr dyn)
{
/* counting how many points in a DynLine */
    int count = 0;
    rl2PointPtr pt = dyn->first;
    while (pt != NULL)
      {
	  count++;
	  pt = pt->next;
      }
    return count;
}

RL2_PRIVATE void
rl2_destroy_updatable_geometry (rl2UpdatableGeometryPtr geom)
{
/* destroying an UPDATABLE GEOMETRY object */
    rl2CoordSeqPtr pC;
    rl2CoordSeqPtr pCn;
    if (geom == NULL)
	return;

    if (geom->blob != NULL)
	free (geom->blob);
    pC = geom->first;
    while (pC != NULL)
      {
	  pCn = pC->next;
	  free (pC);
	  pC = pCn;
      }
    free (geom);
}

static void
rl2AddCoordSeqToGeometry (rl2UpdatableGeometryPtr geom, int points, int offset)
{
/* adding a CoordSeq to an UPDATABLE GEOMETRY object */
    double x;
    double y;
    int iv;
    const unsigned char *p;
    rl2CoordSeqPtr ptr = malloc (sizeof (rl2CoordSeq));
    ptr->points = points;
    ptr->base_addr = geom->blob + offset;
    ptr->endian = geom->endian;
    ptr->endian_arch = geom->endian_arch;
    ptr->has_z = geom->has_z;
    ptr->has_m = geom->has_m;

/* setting the BBOX */
    p = ptr->base_addr;
    geom->minx = rl2GeomImport64 (p + 0, geom->endian, geom->endian_arch);
    geom->maxx = geom->minx;
    geom->miny = rl2GeomImport64 (p + 8, geom->endian, geom->endian_arch);
    geom->maxy = geom->miny;
    p += 16;
    if (geom->has_z)
	p += 8;
    if (geom->has_m)
	p += 8;
    for (iv = 1; iv < points; iv++)
      {
	  x = rl2GeomImport64 (p + 0, geom->endian, geom->endian_arch);
	  y = rl2GeomImport64 (p + 8, geom->endian, geom->endian_arch);
	  if (x < geom->minx)
	      geom->minx = x;
	  if (x > geom->maxx)
	      geom->maxx = x;
	  if (y < geom->miny)
	      geom->miny = y;
	  if (y > geom->maxy)
	      geom->maxy = y;
	  p += 16;
	  if (geom->has_z)
	      p += 8;
	  if (geom->has_m)
	      p += 8;
      }

    ptr->next = NULL;
    if (geom->first == NULL)
	geom->first = ptr;
    if (geom->last != NULL)
	geom->last->next = ptr;
    geom->last = ptr;
}

RL2_PRIVATE double
rl2_get_coord_seq_value (rl2CoordSeqPtr pCS, int iv, char dim)
{
/* extracting a coordinate value from the Sequence */
    int point_sz = 16;
    int offset;
    double value;

    if (pCS->has_m)
	point_sz += 8;
    if (pCS->has_z)
	point_sz += 8;
    if (dim == 'y')
	offset = 8;
    else if (dim == 'z')
	offset = 16;
    else if (dim == 'm')
      {
	  if (pCS->has_z)
	      offset = 24;
	  else
	      offset = 16;
      }
    else
	offset = 0;
    value =
	rl2GeomImport64 (pCS->base_addr + (point_sz * iv) + offset, pCS->endian,
			 pCS->endian_arch);
    return value;
}

RL2_PRIVATE void
rl2_set_coord_seq_value (double value, rl2CoordSeqPtr pCS, int iv, char dim)
{
/* inserting a coordinate value into the Sequence */
    int point_sz = 16;
    int offset;

    if (pCS->has_m)
	point_sz += 8;
    if (pCS->has_z)
	point_sz += 8;
    if (dim == 'y')
	offset = 8;
    else if (dim == 'z')
	offset = 16;
    else if (dim == 'm')
      {
	  if (pCS->has_z)
	      offset = 24;
	  else
	      offset = 16;
      }
    else
	offset = 0;
    rl2GeomExport64 (pCS->base_addr + (point_sz * iv) + offset, value,
		     pCS->endian, pCS->endian_arch);
}

static int
rl2ParseUpdatablePoint (rl2UpdatableGeometryPtr geom, int *offset)
{
/* decodes a POINT (may be Z,M) from WKB */
    int size = 16;
    if (geom->has_z)
	size += 8;
    if (geom->has_m)
	size += 16;
    if (geom->size < *offset + size)
	return 0;
    rl2AddCoordSeqToGeometry (geom, 1, *offset);
    *offset += size;
    return 1;
}

static int
rl2ParseUpdatableLinestring (rl2UpdatableGeometryPtr geom, int *offset)
{
/* decodes a LINESTRING (may be Z,M) from WKB */
    int points;
    int size = 16;
    if (geom->has_z)
	size += 8;
    if (geom->has_m)
	size += 16;
    if (geom->size < *offset + 4)
	return 0;
    points =
	rl2GeomImport32 (geom->blob + *offset, geom->endian, geom->endian_arch);
    *offset += 4;
    if (geom->size < *offset + (size * points))
	return 0;
    rl2AddCoordSeqToGeometry (geom, points, *offset);
    *offset += (size * points);
    return 1;
}

static int
rl2ParseUpdatablePolygon (rl2UpdatableGeometryPtr geom, int *offset)
{
/* decodes a POLYGON (may be Z,M) from WKB */
    int rings;
    int ib;
    int points;
    int size = 16;
    if (geom->has_z)
	size += 8;
    if (geom->has_m)
	size += 16;
    if (geom->size < *offset + 4)
	return 0;
    rings =
	rl2GeomImport32 (geom->blob + *offset, geom->endian, geom->endian_arch);
    *offset += 4;
    for (ib = 0; ib < rings; ib++)
      {
	  if (geom->size < *offset + 4)
	      return 0;
	  points =
	      rl2GeomImport32 (geom->blob + *offset, geom->endian,
			       geom->endian_arch);
	  *offset += 4;
	  if (geom->size < *offset + (size * points))
	      return 0;
	  rl2AddCoordSeqToGeometry (geom, points, *offset);
	  *offset += (size * points);
      }
    return 1;
}

static int
rl2ParseUpdatableCollection (rl2UpdatableGeometryPtr geom, int *offset)
{
/* decodes a MULTIxxxx/GEOMETRYCOLLECTION (may be Z,M) from WKB */
    int entities;
    int type;
    int ie;
    if (geom->size < *offset + 4)
	return 0;
    entities =
	rl2GeomImport32 (geom->blob + *offset, geom->endian, geom->endian_arch);
    *offset += 4;
    for (ie = 0; ie < entities; ie++)
      {
	  if (geom->size < *offset + 5)
	      return 0;
	  type =
	      rl2GeomImport32 (geom->blob + *offset + 1, geom->endian,
			       geom->endian_arch);
	  *offset += 5;
	  switch (type)
	    {
	    case GAIA_POINT:
	    case GAIA_POINTZ:
	    case GAIA_POINTM:
	    case GAIA_POINTZM:
		if (!rl2ParseUpdatablePoint (geom, offset))
		    return 0;
		break;
	    case GAIA_LINESTRING:
	    case GAIA_LINESTRINGZ:
	    case GAIA_LINESTRINGM:
	    case GAIA_LINESTRINGZM:
		if (!rl2ParseUpdatableLinestring (geom, offset))
		    return 0;
		break;
	    case GAIA_POLYGON:
	    case GAIA_POLYGONZ:
	    case GAIA_POLYGONM:
	    case GAIA_POLYGONZM:
		if (!rl2ParseUpdatablePolygon (geom, offset))
		    return 0;
		break;
	    default:
		return 0;
		break;
	    };
      }
    return 1;
}

RL2_PRIVATE rl2UpdatableGeometryPtr
rl2_create_updatable_geometry (const unsigned char *blob, int size)
{
/* decoding from SpatiaLite BLOB to UPDATABLE GEOMETRY */
    int type;
    int little_endian;
    int offset;
    int endian_arch = rl2GeomEndianArch ();
    rl2UpdatableGeometryPtr geom = NULL;
    if (size < 45)
	return NULL;		/* cannot be an internal BLOB WKB geometry */
    if (*(blob + 0) != GAIA_MARK_START)
	return NULL;		/* failed to recognize START signature */
    if (*(blob + (size - 1)) != GAIA_MARK_END)
	return NULL;		/* failed to recognize END signature */
    if (*(blob + 38) != GAIA_MARK_MBR)
	return NULL;		/* failed to recognize MBR signature */
    if (*(blob + 1) == GAIA_LITTLE_ENDIAN)
	little_endian = 1;
    else if (*(blob + 1) == GAIA_BIG_ENDIAN)
	little_endian = 0;
    else
	return NULL;		/* unknown encoding; nor little-endian neither big-endian */
    type = rl2GeomImport32 (blob + 39, little_endian, endian_arch);

/* creating and initializing the Updatable Geometry object */
    geom = malloc (sizeof (rl2UpdatableGeometry));
    geom->blob = malloc (size);
    geom->size = size;
    memcpy (geom->blob, blob, size);
    geom->endian = little_endian;
    geom->endian_arch = endian_arch;
    switch (type)
      {
	  /* testing for Z-values */
      case GAIA_POINTZ:
      case GAIA_LINESTRINGZ:
      case GAIA_POLYGONZ:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_POINTZM:
      case GAIA_LINESTRINGZM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGONZM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  geom->has_z = 1;
	  break;
      default:
	  geom->has_z = 0;
	  break;
      };
    switch (type)
      {
	  /* testing for Z-values */
      case GAIA_POINTM:
      case GAIA_LINESTRINGM:
      case GAIA_POLYGONM:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTIPOLYGONM:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_POINTZM:
      case GAIA_LINESTRINGZM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGONZM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  geom->has_m = 1;
	  break;
      default:
	  geom->has_m = 0;
	  break;
      };
    geom->minx = rl2GeomImport64 (blob + 6, little_endian, endian_arch);
    geom->miny = rl2GeomImport64 (blob + 14, little_endian, endian_arch);
    geom->maxx = rl2GeomImport64 (blob + 22, little_endian, endian_arch);
    geom->maxy = rl2GeomImport64 (blob + 30, little_endian, endian_arch);
    geom->first = NULL;
    geom->last = NULL;

    offset = 43;
    switch (type)
      {
	  /* parsing elementary geometries */
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
	  if (!rl2ParseUpdatablePoint (geom, &offset))
	      goto error;
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
	  if (!rl2ParseUpdatableLinestring (geom, &offset))
	      goto error;
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
	  if (!rl2ParseUpdatablePolygon (geom, &offset))
	      goto error;
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  if (!rl2ParseUpdatableCollection (geom, &offset))
	      goto error;
	  break;
      default:
	  goto error;
	  break;
      };

    return geom;

  error:
    if (geom != NULL)
	rl2_destroy_updatable_geometry (geom);
    return NULL;
}
