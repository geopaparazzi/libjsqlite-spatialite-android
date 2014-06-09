/*

 rl2svg_xml -- SVG XML parsing methods

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
#include <math.h>
#include <float.h>

#include <libxml/parser.h>
#ifdef __ANDROID__ /* Android specific */
#include <cairo.h>
#else /* any other standard platform (Win, Linux, Mac) */
#include <cairo/cairo.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2svg.h"
#include "rasterlite2_private.h"
#include "rl2svg_private.h"

struct svg_dyn_point
{
/* a temporary struct storing a Point */
    double x;
    double y;
    struct svg_dyn_point *next;
};

struct svg_dyn_points
{
/* a linked list of temporary Points */
    struct svg_dyn_point *first;
    struct svg_dyn_point *last;
    int points;
};

static void
svg_free_dyn_points (struct svg_dyn_points *dyn)
{
/* freeing the auxiliary DynPoints */
    struct svg_dyn_point *pn;
    struct svg_dyn_point *p = dyn->first;
    while (p)
      {
          pn = p->next;
          free (p);
          p = pn;
      }
}

static void
svg_add_point (struct svg_dyn_points *dyn, double x, double y)
{
    struct svg_dyn_point *p = malloc (sizeof (struct svg_dyn_point));
    p->x = x;
    p->y = y;
    p->next = NULL;
    if (dyn->first == NULL)
        dyn->first = p;
    if (dyn->last != NULL)
        dyn->last->next = p;
    dyn->last = p;
    dyn->points += 1;
}

static void
svg_parse_points (const char *str, int *points, double **p_x, double **p_y)
{
/* attempting to parse a list of Coordinates */
    char value[1024];
    char *p_out = value;
    const char *p_in = str;
    double x;
    double y;
    char xy = 'x';
    struct svg_dyn_points dyn;

    dyn.points = 0;
    dyn.first = NULL;
    dyn.last = NULL;

    while (1)
      {
          /* scanning the XML string */
          if (*p_in == '\0')
            {
                /* string end */
                *p_out = '\0';
                if (*value != '\0')
                  {
                      if (xy == 'y')
                        {
                            y = atof (value);
                            svg_add_point (&dyn, x, y);
                            xy = 'x';
                        }
                      else
                          x = atof (value);
                  }
                break;
            }
          if (*p_in == '\n' || *p_in == ' ' || *p_in == '\t' || *p_in == '\r'
              || *p_in == ',')
            {
                /* delimiter */
                *p_out = '\0';
                if (*value != '\0')
                  {
                      if (xy == 'y')
                        {
                            y = atof (value);
                            svg_add_point (&dyn, x, y);
                            xy = 'x';
                        }
                      else
                          x = atof (value);
                  }
                if (*p_in == ',')
                    xy = 'y';
                p_out = value;
                p_in++;
                continue;
            }
          *p_out++ = *p_in++;
      }

    if (dyn.points > 0)
      {
          /* creating the coord arrays to be returned */
          int iv = 0;
          struct svg_dyn_point *p = dyn.first;
          double *xx = malloc (sizeof (double) * dyn.points);
          double *yy = malloc (sizeof (double) * dyn.points);
          while (p)
            {
                *(xx + iv) = p->x;
                *(yy + iv) = p->y;
                iv++;
                p = p->next;
            }
          *points = dyn.points;
          *p_x = xx;
          *p_y = yy;
      }
    else
      {
          /* returning an empty list of coords */
          *points = 0;
          *p_x = NULL;
          *p_y = NULL;
      }
/* freeing the DynPoints */
    svg_free_dyn_points (&dyn);
}

static double
svg_parse_hex_color (char hi, char lo)
{
/* parsing and hex Color value */
    double color = 0;
    switch (hi)
      {
      case '1':
          color = 16;
          break;
      case '2':
          color = 16 * 2;
          break;
      case '3':
          color = 16 * 3;
          break;
      case '4':
          color = 16 * 4;
          break;
      case '5':
          color = 16 * 5;
          break;
      case '6':
          color = 16 * 6;
          break;
      case '7':
          color = 16 * 7;
          break;
      case '8':
          color = 16 * 8;
          break;
      case '9':
          color = 16 * 9;
          break;
      case 'a':
      case 'A':
          color = 16 * 10;
          break;
      case 'b':
      case 'B':
          color = 16 * 11;
          break;
      case 'c':
      case 'C':
          color = 16 * 12;
          break;
      case 'd':
      case 'D':
          color = 16 * 13;
          break;
      case 'e':
      case 'E':
          color = 16 * 14;
          break;
      case 'f':
      case 'F':
          color = 16 * 15;
          break;
      };
    switch (lo)
      {
      case '1':
          color += 1;
          break;
      case '2':
          color += 2;
          break;
      case '3':
          color += 3;
          break;
      case '4':
          color += 4;
          break;
      case '5':
          color += 5;
          break;
      case '6':
          color += 6;
          break;
      case '7':
          color += 7;
          break;
      case '8':
          color += 8;
          break;
      case '9':
          color += 9;
          break;
      case 'a':
      case 'A':
          color += 10;
          break;
      case 'b':
      case 'B':
          color += 11;
          break;
      case 'c':
      case 'C':
          color += 12;
          break;
      case 'd':
      case 'D':
          color += 13;
          break;
      case 'e':
      case 'E':
          color += 14;
          break;
      case 'f':
      case 'F':
          color += 15;
          break;
      };
    return color / 255.0;
}

static void
svg_from_named_color (char *buf, const char *color)
{
/* translating some CSS name into an hex RGB */
    *buf = '\0';
    if (strcmp (color, "black") == 0)
        strcpy (buf, "#000000");
    else if (strcmp (color, "silver") == 0)
        strcpy (buf, "#C0C0C0");
    else if (strcmp (color, "gray") == 0)
        strcpy (buf, "#808080");
    else if (strcmp (color, "white") == 0)
        strcpy (buf, "#FFFFFF");
    else if (strcmp (color, "maroon") == 0)
        strcpy (buf, "#800000");
    else if (strcmp (color, "red") == 0)
        strcpy (buf, "#FF0000");
    else if (strcmp (color, "purple") == 0)
        strcpy (buf, "#800080");
    else if (strcmp (color, "fuchsia") == 0)
        strcpy (buf, "#FF00FF");
    else if (strcmp (color, "green") == 0)
        strcpy (buf, "#008000");
    else if (strcmp (color, "lime") == 0)
        strcpy (buf, "#00FF00");
    else if (strcmp (color, "olive") == 0)
        strcpy (buf, "#808000");
    else if (strcmp (color, "yellow") == 0)
        strcpy (buf, "#FFFF00");
    else if (strcmp (color, "navy") == 0)
        strcpy (buf, "#000080");
    else if (strcmp (color, "blue") == 0)
        strcpy (buf, "#0000FF");
    else if (strcmp (color, "teal") == 0)
        strcpy (buf, "#008080");
    else if (strcmp (color, "aqua") == 0)
        strcpy (buf, "#00FFFF");
}

static int
svg_consume_float (const char **ptr, double *value)
{
/* attempting to parse a Double value from a string */
    char buf[1024];
    char *p_out = buf;
    const char *p_in = *ptr;
    int count_e = 0;
    int count_pt = 0;
    if (ptr == NULL)
        return 0;
    if (*ptr == '\0')
        return 0;

    while (1)
      {
          if (*p_in == '\0')
            {
                *p_out = '\0';
                *ptr = p_in;
                break;
            }
          if (p_out == buf)
            {
                if (*p_in == '-' || *p_in == '+')
                  {
                      *p_out++ = *p_in++;
                      continue;
                  }
                if (*p_in == ',' || *p_in == ' ' || *p_in == '\t'
                    || *p_in == '\r' || *p_in == '\n')
                  {
                      p_in++;
                      continue;
                  }
            }
          if (*p_in >= '0' && *p_in <= '9')
            {
                *p_out++ = *p_in++;
                continue;
            }
          else if (*p_in == 'e' || *p_in == 'E')
            {
                count_e++;
                *p_out++ = *p_in++;
                if (*p_in == '-' || *p_in == '+')
                    *p_out++ = *p_in++;
                continue;
            }
          else if (*p_in == '.')
            {
                count_pt++;
                *p_out++ = *p_in++;
                continue;
            }
          else
            {
                *p_out = '\0';
                switch (*p_in)
                  {
                      /* SVG Path keyword - unput */
                  case 'C':
                  case 'c':
                  case 'S':
                  case 's':
                  case 'Q':
                  case 'q':
                  case 'T':
                  case 't':
                  case 'M':
                  case 'm':
                  case 'L':
                  case 'l':
                  case 'H':
                  case 'h':
                  case 'V':
                  case 'v':
                  case 'A':
                  case 'a':
                  case 'Z':
                  case 'z':
                  case '+':
                  case '-':
                      p_in--;
                      break;
                  };
                break;
            }
      }
    if (count_pt > 1 || count_e > 1)
        return 0;
    if (*buf != '\0')
      {
          *value = atof (buf);
          *ptr = p_in;
          return 1;
      }
    return 0;
}

static void
svg_parse_path_d (rl2PrivSvgPathPtr path, const char *value)
{
/* parsing an SVG Path <d> */
    char keyword = '\0';
    int count = 0;
    const char *p_in = value;
    double coord;
    double coord_array[7];
    double first_x = DBL_MAX;
    double first_y = DBL_MAX;
    double last_x = DBL_MAX;
    double last_y = DBL_MAX;
    double bezier_reflect_x;
    double bezier_reflect_y;
    double x;
    double y;
    double x1;
    double y1;
    double x2;
    double y2;
    void *data;
    int last_m = 0;

    while (1)
      {
          if (*p_in == '\0')
              break;
          switch (*p_in)
            {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                break;
            case 'Z':
            case 'z':
                svg_add_path_item (path, RL2_SVG_CLOSE_PATH, NULL);
                last_x = first_x;
                last_y = first_y;
                keyword = '\0';
                break;
            case 'C':
            case 'c':
            case 'S':
            case 's':
            case 'Q':
            case 'q':
            case 'T':
            case 't':
                keyword = *p_in;
                break;
            case 'M':
            case 'm':
            case 'L':
            case 'l':
            case 'H':
            case 'h':
            case 'V':
            case 'v':
            case 'A':
            case 'a':
                bezier_reflect_x = DBL_MAX;
                bezier_reflect_y = DBL_MAX;     /* invalidating Bezier reflect point */
                keyword = *p_in;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '.':
            case '+':
            case '-':
                if (!svg_consume_float (&p_in, &coord))
                  {
                      path->error = 1;
                      return;
                  }
                if (count < 7)
                    coord_array[count] = coord;
                count++;
                break;
            default:
                path->error = 1;
                break;
            };
          if (last_m && keyword == 'M')
              keyword = 'L';
          if (last_m && keyword == 'm')
              keyword = 'l';
          switch (keyword)
            {
            case 'M':
                if (count == 2)
                  {
                      /* SVG Path MoveTo - absolute */
                      data =
                          svg_alloc_path_move (coord_array[0], coord_array[1]);
                      svg_add_path_item (path, RL2_SVG_MOVE_TO, data);
                      count = 0;
                      last_x = coord_array[0];
                      last_y = coord_array[1];
                      first_x = last_x;
                      first_y = last_y;
                      last_m = 1;
                  }
                break;
            case 'm':
                if (count == 2)
                  {
                      /* SVG Path MoveTo - relative */
                      if (last_x == DBL_MAX || last_y == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            x = coord_array[0];
                            y = coord_array[1];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            x = last_x + coord_array[0];
                            y = last_y + coord_array[1];
                        }
                      data = svg_alloc_path_move (x, y);
                      svg_add_path_item (path, RL2_SVG_MOVE_TO, data);
                      count = 0;
                      last_x = x;
                      last_y = y;
                      last_m = 1;
                      first_x = last_x;
                      first_y = last_y;
                  }
                break;
            case 'L':
                if (count == 2)
                  {
                      /* SVG Path LineTo - absolute */
                      data =
                          svg_alloc_path_move (coord_array[0], coord_array[1]);
                      svg_add_path_item (path, RL2_SVG_LINE_TO, data);
                      count = 0;
                      last_x = coord_array[0];
                      last_y = coord_array[1];
                      last_m = 0;
                  }
                break;
            case 'l':
                if (count == 2)
                  {
                      /* SVG Path LineTo - relative */
                      if (last_x == DBL_MAX || last_y == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            x = coord_array[0];
                            y = coord_array[1];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            x = last_x + coord_array[0];
                            y = last_y + coord_array[1];
                        }
                      data = svg_alloc_path_move (x, y);
                      svg_add_path_item (path, RL2_SVG_LINE_TO, data);
                      count = 0;
                      last_x = x;
                      last_y = y;
                      last_m = 0;
                  }
                break;
            case 'H':
                if (count == 1)
                  {
                      /* SVG Path Horizontal LineTo - absolute */
                      data = svg_alloc_path_move (coord_array[0], last_y);
                      svg_add_path_item (path, RL2_SVG_LINE_TO, data);
                      count = 0;
                      last_x = coord_array[0];
                      last_m = 0;
                  }
                break;
            case 'h':
                if (count == 1)
                  {
                      /* SVG Path Horizontal LineTo - relative */
                      if (last_x == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            x = coord_array[0];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            x = last_x + coord_array[0];
                        }
                      data = svg_alloc_path_move (x, last_y);
                      svg_add_path_item (path, RL2_SVG_LINE_TO, data);
                      count = 0;
                      last_x = x;
                      last_m = 0;
                  }
                break;
            case 'V':
                if (count == 1)
                  {
                      /* SVG Path Vertical LineTo - absolute */
                      data = svg_alloc_path_move (last_x, coord_array[0]);
                      svg_add_path_item (path, RL2_SVG_LINE_TO, data);
                      count = 0;
                      last_y = coord_array[0];
                      last_m = 0;
                  }
                break;
            case 'v':
                if (count == 1)
                  {
                      /* SVG Path Vertical LineTo - relative */
                      if (last_y == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            y = coord_array[0];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            y = last_y + coord_array[0];
                        }
                      data = svg_alloc_path_move (last_x, y);
                      svg_add_path_item (path, RL2_SVG_LINE_TO, data);
                      count = 0;
                      last_y = y;
                      last_m = 0;
                  }
                break;
            case 'C':
                if (count == 6)
                  {
                      /* SVG Path Cubic Bezier CurveTo - absolute */
                      data =
                          svg_alloc_path_bezier (coord_array[0],
                                                 coord_array[1],
                                                 coord_array[2],
                                                 coord_array[3],
                                                 coord_array[4],
                                                 coord_array[5]);
                      svg_add_path_item (path, RL2_SVG_CURVE_3, data);
                      count = 0;
                      last_x = coord_array[4];
                      last_y = coord_array[5];
                      /* reflection of the second control point */
                      bezier_reflect_x =
                          coord_array[4] - (coord_array[2] - coord_array[4]);
                      bezier_reflect_y =
                          coord_array[5] - (coord_array[3] - coord_array[5]);
                      last_m = 0;
                  }
                break;
            case 'c':
                if (count == 6)
                  {
                      /* SVG Path Cubic Bezier CurveTo - relative */
                      if (last_x == DBL_MAX || last_y == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            x1 = coord_array[0];
                            y1 = coord_array[1];
                            x2 = coord_array[2];
                            y2 = coord_array[3];
                            x = coord_array[4];
                            y = coord_array[5];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            x1 = last_x + coord_array[0];
                            y1 = last_y + coord_array[1];
                            x2 = last_x + coord_array[2];
                            y2 = last_y + coord_array[3];
                            x = last_x + coord_array[4];
                            y = last_y + coord_array[5];
                        }
                      data = svg_alloc_path_bezier (x1, y1, x2, y2, x, y);
                      svg_add_path_item (path, RL2_SVG_CURVE_3, data);
                      count = 0;
                      last_x = x;
                      last_y = y;
                      /* reflection of the second control point */
                      bezier_reflect_x = x - (x2 - x);
                      bezier_reflect_y = y - (y2 - y);
                      last_m = 0;
                  }
                break;
            case 'S':
                if (count == 4)
                  {
                      /* SVG Path Cubic Bezier CurveTo [short] - absolute */
                      if (bezier_reflect_x == DBL_MAX
                          || bezier_reflect_y == DBL_MAX)
                        {
                            /* assuming the current point as Bezier reflected point */
                            bezier_reflect_x = coord_array[2];
                            bezier_reflect_y = coord_array[3];
                        }
                      data =
                          svg_alloc_path_bezier (bezier_reflect_x,
                                                 bezier_reflect_y,
                                                 coord_array[0],
                                                 coord_array[1],
                                                 coord_array[2],
                                                 coord_array[3]);
                      svg_add_path_item (path, RL2_SVG_CURVE_3, data);
                      count = 0;
                      last_x = coord_array[2];
                      last_y = coord_array[3];
                      /* reflection of the second control point */
                      bezier_reflect_x =
                          coord_array[2] - (coord_array[0] - coord_array[2]);
                      bezier_reflect_y =
                          coord_array[3] - (coord_array[1] - coord_array[3]);
                      last_m = 0;
                  }
                break;
            case 's':
                if (count == 4)
                  {
                      /* SVG Path Cubic Bezier CurveTo [short] - relative */
                      if (last_x == DBL_MAX || last_y == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            x2 = coord_array[0];
                            y2 = coord_array[1];
                            x = coord_array[2];
                            y = coord_array[3];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            x2 = last_x + coord_array[0];
                            y2 = last_y + coord_array[1];
                            x = last_x + coord_array[2];
                            y = last_y + coord_array[3];
                        }
                      if (bezier_reflect_x == DBL_MAX
                          || bezier_reflect_y == DBL_MAX)
                        {
                            /* assuming the current point as Bezier reflected point */
                            bezier_reflect_x = last_x;
                            bezier_reflect_y = last_y;
                        }
                      data =
                          svg_alloc_path_bezier (bezier_reflect_x,
                                                 bezier_reflect_y, x2, y2, x,
                                                 y);
                      svg_add_path_item (path, RL2_SVG_CURVE_3, data);
                      count = 0;
                      last_x = x;
                      last_y = y;
                      /* reflection of the second control point */
                      bezier_reflect_x = x - (x2 - x);
                      bezier_reflect_y = y - (y2 - y);
                      last_m = 0;
                  }
                break;
            case 'Q':
                if (count == 4)
                  {
                      /* SVG Path Quadratic Bezier CurveTo - absolute */
                      data =
                          svg_alloc_path_bezier (coord_array[0],
                                                 coord_array[1],
                                                 coord_array[2],
                                                 coord_array[3], 0.0, 0.0);
                      svg_add_path_item (path, RL2_SVG_CURVE_4, data);
                      count = 0;
                      last_x = coord_array[2];
                      last_y = coord_array[3];
                      /* reflection of the control point */
                      bezier_reflect_x =
                          coord_array[2] - (coord_array[0] - coord_array[2]);
                      bezier_reflect_y =
                          coord_array[3] - (coord_array[1] - coord_array[3]);
                      last_m = 0;
                  }
                break;
            case 'q':
                if (count == 4)
                  {
                      /* SVG Path Quadratic Bezier CurveTo - relative */
                      if (last_x == DBL_MAX || last_y == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            x1 = coord_array[0];
                            y1 = coord_array[1];
                            x = coord_array[2];
                            y = coord_array[3];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            x1 = last_x + coord_array[0];
                            y1 = last_y + coord_array[1];
                            x = last_x + coord_array[2];
                            y = last_y + coord_array[3];
                        }
                      data = svg_alloc_path_bezier (x1, y1, x, y, 0.0, 0.0);
                      svg_add_path_item (path, RL2_SVG_CURVE_4, data);
                      count = 0;
                      last_x = x;
                      last_y = y;
                      /* reflection of the control point */
                      bezier_reflect_x = x - (x1 - x);
                      bezier_reflect_y = y - (y1 - y);
                      last_m = 0;
                  }
                break;
            case 'T':
                if (count == 2)
                  {
                      /* SVG Path Quadratic Bezier CurveTo [short] - absolute */
                      if (bezier_reflect_x == DBL_MAX
                          || bezier_reflect_y == DBL_MAX)
                        {
                            /* assuming the current point as Bezier reflected point */
                            bezier_reflect_x = coord_array[0];
                            bezier_reflect_y = coord_array[1];
                        }
                      data =
                          svg_alloc_path_bezier (bezier_reflect_x,
                                                 bezier_reflect_y,
                                                 coord_array[0],
                                                 coord_array[1], 0.0, 0.0);
                      svg_add_path_item (path, RL2_SVG_CURVE_4, data);
                      count = 0;
                      last_x = coord_array[0];
                      last_y = coord_array[1];
                      /* reflection of the control point */
                      bezier_reflect_x =
                          coord_array[0] - (bezier_reflect_x - coord_array[0]);
                      bezier_reflect_y =
                          coord_array[1] - (bezier_reflect_y - coord_array[1]);
                      last_m = 0;
                  }
                break;
            case 't':
                if (count == 2)
                  {
                      /* SVG Path Quadratic Bezier CurveTo [short] - relative */
                      if (last_x == DBL_MAX || last_y == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            x = coord_array[0];
                            y = coord_array[1];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            x = last_x + coord_array[0];
                            y = last_y + coord_array[1];
                        }
                      if (bezier_reflect_x == DBL_MAX
                          || bezier_reflect_y == DBL_MAX)
                        {
                            /* assuming the current point as Bezier reflected point */
                            bezier_reflect_x = x;
                            bezier_reflect_y = y;
                        }
                      data =
                          svg_alloc_path_bezier (bezier_reflect_x,
                                                 bezier_reflect_y, x, y, 0.0,
                                                 0.0);
                      svg_add_path_item (path, RL2_SVG_CURVE_4, data);
                      count = 0;
                      last_x = x;
                      last_y = y;
                      /* reflection of the control point */
                      bezier_reflect_x = x - (bezier_reflect_x - x);
                      bezier_reflect_y = y - (bezier_reflect_y - y);
                      last_m = 0;
                  }
                break;
            case 'A':
                if (count == 7)
                  {
                      /* SVG Path EllipticalArc - absolute */
                      data =
                          svg_alloc_path_ellipt_arc (coord_array[0],
                                                     coord_array[1],
                                                     coord_array[2],
                                                     coord_array[3],
                                                     coord_array[4],
                                                     coord_array[5],
                                                     coord_array[6]);
                      svg_add_path_item (path, RL2_SVG_ELLIPT_ARC, data);
                      count = 0;
                      last_x = coord_array[5];
                      last_y = coord_array[6];
                      last_m = 0;
                  }
                break;
            case 'a':
                if (count == 7)
                  {
                      /* SVG Path EllipticalArc - relative */
                      if (last_x == DBL_MAX || last_y == DBL_MAX)
                        {
                            /* assuming absolute coords */
                            x = coord_array[5];
                            y = coord_array[6];
                        }
                      else
                        {
                            /* transforming relative coords into absolute */
                            x = last_x + coord_array[5];
                            y = last_y + coord_array[6];
                        }
                      if (coord_array[0] == 0.0 || coord_array[1] == 0.0)
                        {
                            /* ZERO radius: defaulting to a straight line */
                            data = svg_alloc_path_move (x, y);
                            svg_add_path_item (path, RL2_SVG_LINE_TO, data);
                        }
                      else
                        {
                            data =
                                svg_alloc_path_ellipt_arc (coord_array[0],
                                                           coord_array[1],
                                                           coord_array[2],
                                                           coord_array[3],
                                                           coord_array[4], x,
                                                           y);
                            svg_add_path_item (path, RL2_SVG_ELLIPT_ARC, data);
                        }
                      count = 0;
                      last_x = x;
                      last_y = y;
                      last_m = 0;
                  }
                break;
            };
          if (path->error)
              return;
          if (*p_in != '\0')
              p_in++;
      }
}

static void
svg_parse_display (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG Display attribute */
    style->fill = 1;
    if (strcmp (value, "none") == 0)
        style->visibility = 0;
}

static void
svg_parse_visibility (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG Visibility attribute */
    style->fill = 1;
    if (strcmp (value, "hidden") == 0)
        style->visibility = 0;
}

static int
svg_parse_fill_gradient_url (rl2PrivSvgStylePtr style, const char *color)
{
/* parsing an SVG FillColor attribute (URL) */
    int len = strlen (color);
    if (strncmp (color, "url(#", 5) == 0 && *(color + len - 1) == ')')
      {
          char buf[1024];
          strcpy (buf, color + 5);
          len = strlen (buf);
          buf[len - 1] = '\0';
          svg_add_fill_gradient_url (style, buf);
          style->fill = 1;
          return 1;
      }
    return 0;
}

static void
svg_parse_fill_color (rl2PrivSvgStylePtr style, const char *color)
{
/* parsing an SVG FillColor attribute */
    int len = strlen (color);
    char buf[16];
    const char *p_color = NULL;
    if (strcmp (color, "none") == 0)
      {
          style->no_fill = 1;
          return;
      }
    if (svg_parse_fill_gradient_url (style, color) == 1)
        return;
    style->fill = 1;
    if (*color == '#' && len >= 7)
        p_color = color;
    else if (*color == '#' && len == 4)
      {
          buf[0] = *(color + 0);
          buf[1] = *(color + 1);
          buf[2] = *(color + 1);
          buf[3] = *(color + 2);
          buf[4] = *(color + 2);
          buf[5] = *(color + 3);
          buf[6] = *(color + 3);
          p_color = buf;
      }
    else
      {
          svg_from_named_color (buf, color);
          if (*buf != '\0')
              p_color = buf;
          else
              p_color = NULL;
      }
    if (p_color == NULL)
        p_color = "#000000";
    style->fill_red = svg_parse_hex_color (*(p_color + 1), *(p_color + 2));
    style->fill_green = svg_parse_hex_color (*(p_color + 3), *(p_color + 4));
    style->fill_blue = svg_parse_hex_color (*(p_color + 5), *(p_color + 6));
}

static void
svg_parse_fill_rule (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG FillRule attribute */
    style->fill = 1;
    if (strcmp (value, "evenodd") == 0)
        style->stroke_linecap = CAIRO_FILL_RULE_EVEN_ODD;
}

static void
svg_parse_fill_opacity (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG FillOpacity attribute */
    style->fill = 1;
    style->fill_opacity = atof (value);
    if (style->fill_opacity <= 0.0 || style->fill_opacity >= 1.0)
        style->fill_opacity = 1.0;
}

static int
svg_parse_stroke_gradient_url (rl2PrivSvgStylePtr style, const char *color)
{
/* parsing an SVG StrokeColor attribute (URL) */
    int len = strlen (color);
    if (strncmp (color, "url(#", 5) == 0 && *(color + len - 1) == ')')
      {
          char buf[1024];
          strcpy (buf, color + 5);
          len = strlen (buf);
          buf[len - 1] = '\0';
          svg_add_stroke_gradient_url (style, buf);
          return 1;
      }
    return 0;
}

static void
svg_parse_stroke_color (rl2PrivSvgStylePtr style, const char *color)
{
/* parsing an SVG StrokeColor attribute */
    int len = strlen (color);
    char buf[16];
    const char *p_color = NULL;
    style->stroke = 1;
    if (strcmp (color, "none") == 0)
      {
          style->no_stroke = 1;
          return;
      }
    if (svg_parse_stroke_gradient_url (style, color) == 1)
        return;
    if (*color == '#' && len >= 7)
        p_color = color;
    else if (*color == '#' && len == 4)
      {
          buf[0] = *(color + 0);
          buf[1] = *(color + 1);
          buf[2] = *(color + 1);
          buf[3] = *(color + 2);
          buf[4] = *(color + 2);
          buf[5] = *(color + 3);
          buf[6] = *(color + 3);
          p_color = buf;
      }
    else
      {
          svg_from_named_color (buf, color);
          if (*color != '\0')
              p_color = buf;
      }
    if (p_color == NULL)
        p_color = "#000000";
    style->stroke_red = svg_parse_hex_color (*(p_color + 1), *(p_color + 2));
    style->stroke_green = svg_parse_hex_color (*(p_color + 3), *(p_color + 4));
    style->stroke_blue = svg_parse_hex_color (*(p_color + 5), *(p_color + 6));
}

static void
svg_parse_stop_color (const char *color, double *red, double *green,
                      double *blue)
{
/* parsing an SVG StopColor attribute */
    int len = strlen (color);
    char buf[16];
    const char *p_color = NULL;
    if (strcmp (color, "none") == 0)
      {
          *red = -1.0;
          *green = -1.0;
          *blue = -1.0;
          return;
      }
    if (*color == '#' && len >= 7)
        p_color = color;
    else if (*color == '#' && len == 4)
      {
          buf[0] = *(color + 0);
          buf[1] = *(color + 1);
          buf[2] = *(color + 1);
          buf[3] = *(color + 2);
          buf[4] = *(color + 2);
          buf[5] = *(color + 3);
          buf[6] = *(color + 3);
          p_color = buf;
      }
    else
      {
          svg_from_named_color (buf, color);
          if (*buf != '\0')
              p_color = buf;
      }
    if (p_color == NULL)
        p_color = "#000000";
    *red = svg_parse_hex_color (*(p_color + 1), *(p_color + 2));
    *green = svg_parse_hex_color (*(p_color + 3), *(p_color + 4));
    *blue = svg_parse_hex_color (*(p_color + 5), *(p_color + 6));
}

static void
svg_parse_stroke_width (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG StrokeWidth attribute */
    style->stroke = 1;
    style->stroke_width = atof (value);
    if (style->stroke_width <= 0.0)
        style->stroke_width = 1.0;
}

static void
svg_parse_stroke_linecap (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG StrokeLineCap attribute */
    style->stroke = 1;
    if (strcmp (value, "round") == 0)
        style->stroke_linecap = CAIRO_LINE_CAP_ROUND;
    if (strcmp (value, "square") == 0)
        style->stroke_linecap = CAIRO_LINE_CAP_SQUARE;
}

static void
svg_parse_stroke_linejoin (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG StrokeLineJoin attribute */
    style->stroke = 1;
    if (strcmp (value, "round") == 0)
        style->stroke_linejoin = CAIRO_LINE_JOIN_ROUND;
    if (strcmp (value, "bevel") == 0)
        style->stroke_linejoin = CAIRO_LINE_JOIN_BEVEL;
}

static void
svg_parse_stroke_miterlimit (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG MiterLimit attribute */
    style->stroke = 1;
    style->stroke_miterlimit = atof (value);
    if (style->stroke_miterlimit <= 0.0)
        style->stroke_miterlimit = 10.0;
}

static void
svg_parse_stroke_dashoffset (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG DashOffset attribute */
    style->stroke = 1;
    style->stroke_dashoffset = atof (value);
}

static void
svg_parse_stroke_dasharray (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG DashArray attribute */
    double dash_list[1024];
    int items = 0;
    char token[1024];
    char *p_out = token;
    const char *p_in = value;
    if (style->stroke_dasharray != NULL)
        free (style->stroke_dasharray);
    style->stroke_dasharray = NULL;
    style->stroke_dashitems = 0;
    if (strcmp (value, "none") == 0)
        return;
    while (1)
      {
          if (*p_in == ' ' || *p_in == ',' || *p_in == '\0')
            {
                *p_out++ = '\0';
                if (*token != '\0')
                    dash_list[items++] = atof (token);
                if (*p_in == '\0')
                    break;
                p_out = token;
                p_in++;
                continue;
            }
          *p_out++ = *p_in++;
      }
    if (items > 0)
      {
          int i;
          if (items % 2 == 0)
            {
                /* even number */
                style->stroke_dashitems = items;
                style->stroke_dasharray = malloc (sizeof (double) * items);
                for (i = 0; i < items; i++)
                    style->stroke_dasharray[i] = dash_list[i];
            }
          else
            {
                /* odd number: doubling */
                int o = 0;
                style->stroke_dashitems = items * 2;
                style->stroke_dasharray = malloc (sizeof (double) * items * 2);
                for (i = 0; i < items; i++)
                    style->stroke_dasharray[o++] = dash_list[i];
                for (i = 0; i < items; i++)
                    style->stroke_dasharray[o++] = dash_list[i];
            }
      }
    style->stroke = 1;
}

static void
svg_parse_opacity (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG Opacity attribute */
    style->opacity = atof (value);
    if (style->opacity <= 0.0 || style->opacity >= 1.0)
        style->opacity = 1.0;
}

static void
svg_parse_stroke_opacity (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG StrokeOpacity attribute */
    style->stroke = 1;
    style->stroke_opacity = atof (value);
    if (style->stroke_opacity <= 0.0 || style->stroke_opacity >= 1.0)
        style->stroke_opacity = 1.0;
}

static void
svg_parse_stop_opacity (const char *value, double *opacity)
{
/* parsing an SVG Stop attribute */
    *opacity = atof (value);
}

static void
svg_split_css_token (rl2PrivSvgStylePtr style, char *value)
{
/* parsing an SVG CSS Style definition - single item */
    char *p = value;
    char *p_value = NULL;
    while (*p != '\0')
      {
          if (*p == ':')
            {
                *p = '\0';
                p_value = p + 1;
                break;
            }
          p++;
      }
    if (p_value == NULL)
        return;
    if (strcmp (value, "opacity") == 0)
        svg_parse_opacity (style, p_value);
    else if (strcmp (value, "stroke") == 0)
        svg_parse_stroke_color (style, p_value);
    else if (strcmp (value, "stroke-width") == 0)
        svg_parse_stroke_width (style, p_value);
    else if (strcmp (value, "stroke-linecap") == 0)
        svg_parse_stroke_linecap (style, p_value);
    else if (strcmp (value, "stroke-linejoin") == 0)
        svg_parse_stroke_linejoin (style, p_value);
    else if (strcmp (value, "stroke-miterlimit") == 0)
        svg_parse_stroke_miterlimit (style, p_value);
    else if (strcmp (value, "stroke-dasharray") == 0)
        svg_parse_stroke_dasharray (style, p_value);
    else if (strcmp (value, "stroke-dashoffset") == 0)
        svg_parse_stroke_dashoffset (style, p_value);
    else if (strcmp (value, "stroke-opacity") == 0)
        svg_parse_stroke_opacity (style, p_value);
    else if (strcmp (value, "fill") == 0)
        svg_parse_fill_color (style, p_value);
    else if (strcmp (value, "fill-rule") == 0)
        svg_parse_fill_rule (style, p_value);
    else if (strcmp (value, "fill-opacity") == 0)
        svg_parse_fill_opacity (style, p_value);
    else if (strcmp (value, "display") == 0)
        svg_parse_display (style, p_value);
    else if (strcmp (value, "visibility") == 0)
        svg_parse_visibility (style, p_value);
}

static void
svg_parse_css (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG CSS Style definition */
    const char *p_in = value;
    char token[128];
    char *p_out = token;
    while (1)
      {
          if (*p_in == ' ' || *p_in == '\t' || *p_in == '\r' || *p_in == '\n')
            {
                p_in++;
                continue;
            }
          if (*p_in == '\0')
            {
                *p_out = '\0';
                svg_split_css_token (style, token);
                break;
            }
          if (*p_in == ';')
            {
                *p_out = '\0';
                svg_split_css_token (style, token);
                p_out = token;
                p_in++;
                continue;
            }
          *p_out++ = *p_in++;
      }
}

static void
svg_split_stop_token (char *value, double *red, double *green, double *blue,
                      double *opacity)
{
/* parsing an SVG Stop Style definition - single item */
    char *p = value;
    char *p_value = NULL;
    while (*p != '\0')
      {
          if (*p == ':')
            {
                *p = '\0';
                p_value = p + 1;
                break;
            }
          p++;
      }
    if (p_value == NULL)
        return;
    if (strcmp (value, "stop-color") == 0)
        svg_parse_stop_color (p_value, red, green, blue);
    else if (strcmp (value, "stop-opacity") == 0)
        svg_parse_stop_opacity (p_value, opacity);
}

static void
svg_parse_stop_style (const char *value, double *red, double *green,
                      double *blue, double *opacity)
{
/* parsing an SVG Stop-Style */
    const char *p_in = value;
    char token[128];
    char *p_out = token;
    while (1)
      {
          if (*p_in == ' ' || *p_in == '\t' || *p_in == '\r' || *p_in == '\n')
            {
                p_in++;
                continue;
            }
          if (*p_in == '\0')
            {
                *p_out = '\0';
                svg_split_stop_token (token, red, green, blue, opacity);
                break;
            }
          if (*p_in == ';')
            {
                *p_out = '\0';
                svg_split_stop_token (token, red, green, blue, opacity);
                p_out = token;
                p_in++;
                continue;
            }
          *p_out++ = *p_in++;
      }
}

static void
svg_parse_style (rl2PrivSvgGroupPtr group, rl2PrivSvgShapePtr shape,
                 rl2PrivSvgUsePtr use, struct _xmlAttr *attr)
{
/* parsing SVG Style-related definitions */
    rl2PrivSvgStylePtr style = NULL;
    if (group != NULL)
        style = &(group->style);
    else if (use != NULL)
        style = &(use->style);
    else
        style = &(shape->style);

    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "style") == 0)
                          svg_parse_css (style, value);
                      else if (strcmp (name, "stroke") == 0)
                          svg_parse_stroke_color (style, value);
                      else if (strcmp (name, "stroke-width") == 0)
                          svg_parse_stroke_width (style, value);
                      else if (strcmp (name, "stroke-linecap") == 0)
                          svg_parse_stroke_linecap (style, value);
                      else if (strcmp (name, "stroke-linejoin") == 0)
                          svg_parse_stroke_linejoin (style, value);
                      else if (strcmp (name, "stroke-miterlimit") == 0)
                          svg_parse_stroke_miterlimit (style, value);
                      else if (strcmp (name, "stroke-dasharray") == 0)
                          svg_parse_stroke_dasharray (style, value);
                      else if (strcmp (name, "stroke-dashoffset") == 0)
                          svg_parse_stroke_dashoffset (style, value);
                      else if (strcmp (name, "stroke-opacity") == 0)
                          svg_parse_stroke_opacity (style, value);
                      else if (strcmp (name, "fill") == 0)
                          svg_parse_fill_color (style, value);
                      else if (strcmp (name, "fill-rule") == 0)
                          svg_parse_fill_rule (style, value);
                      else if (strcmp (name, "fill-opacity") == 0)
                          svg_parse_fill_opacity (style, value);
                      else if (strcmp (name, "display") == 0)
                          svg_parse_display (style, value);
                      else if (strcmp (name, "visibility") == 0)
                          svg_parse_visibility (style, value);
                  }
            }
          attr = attr->next;
      }
}

static void
svg_parse_id (rl2PrivSvgGroupPtr group, rl2PrivSvgClipPtr clip,
              rl2PrivSvgShapePtr shape, struct _xmlAttr *attr)
{
/* parsing SVG an eventual ID definitions */
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "id") == 0)
                        {
                            if (group != NULL)
                                svg_add_group_id (group, value);
                            if (clip != NULL)
                                svg_add_clip_id (clip, value);
                            if (shape != NULL)
                                svg_add_shape_id (shape, value);
                        }
                  }
            }
          attr = attr->next;
      }
}

static void
svg_consume_whitespace (const char **ptr)
{
/* consuming whitespaces and commas */
    const char *p = *ptr;
    while (1)
      {
          if (*p == ' ' || *p == ',' || *p == '\t' || *p == '\r' || *p == '\n')
            {
                p++;
                continue;
            }
          break;
      }
    *ptr = p;
}

static int
svg_find_transform_mode (const char **ptr)
{
    const char *p = *ptr;
    if (strncmp (p, "matrix", 6) == 0)
      {
          *ptr += 6;
          return RL2_SVG_MATRIX;
      }
    if (strncmp (p, "translate", 9) == 0)
      {
          *ptr += 9;
          return RL2_SVG_TRANSLATE;
      }
    if (strncmp (p, "scale", 5) == 0)
      {
          *ptr += 5;
          return RL2_SVG_SCALE;
      }
    if (strncmp (p, "rotate", 6) == 0)
      {
          *ptr += 6;
          return RL2_SVG_ROTATE;
      }
    if (strncmp (p, "skewX", 5) == 0)
      {
          *ptr += 5;
          return RL2_SVG_SKEW_X;
      }
    if (strncmp (p, "skewY", 5) == 0)
      {
          *ptr += 5;
          return RL2_SVG_SKEW_Y;
      }
    return RL2_SVG_UNKNOWN;
}

static void *
svg_parse_matrix (const char **ptr)
{
/* parsing an SVG Matrix definition */
    const char *p = *ptr;
    double a;
    double b;
    double c;
    double d;
    double e;
    double f;
    int err = 0;
    if (!svg_consume_float (&p, &a))
        err = 1;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &b))
        err = 1;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &c))
        err = 1;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &d))
        err = 1;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &e))
        err = 1;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &f))
        err = 1;
    svg_consume_whitespace (&p);
    if (*p == ')')
        p++;
    else
        err = 1;
    if (!err)
      {
          *ptr = p;
          return svg_alloc_matrix (a, b, c, d, e, f);
      }
    return NULL;
}

static void *
svg_parse_translate (const char **ptr)
{
/* parsing an SVG Translate definition */
    const char *p = *ptr;
    double tx;
    double ty;
    int err = 0;
    if (!svg_consume_float (&p, &tx))
        err = 1;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &ty))
        ty = 0.0;
    svg_consume_whitespace (&p);
    if (*p == ')')
        p++;
    else
        err = 1;
    if (!err)
      {
          *ptr = p;
          return svg_alloc_translate (tx, ty);
      }
    return NULL;
}

static void *
svg_parse_scale (const char **ptr)
{
/* parsing an SVG Scale definition */
    const char *p = *ptr;
    double sx;
    double sy;
    int err = 0;
    if (!svg_consume_float (&p, &sx))
        err = 1;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &sy))
        sy = 0.0;
    svg_consume_whitespace (&p);
    if (*p == ')')
        p++;
    else
        err = 1;
    if (!err)
      {
          *ptr = p;
          if (sy == 0.0)
              sy = sx;
          return svg_alloc_scale (sx, sy);
      }
    return NULL;
}

static void *
svg_parse_rotate (const char **ptr)
{
/* parsing an SVG Rotate definition */
    const char *p = *ptr;
    double angle;
    double cx;
    double cy;
    int err = 0;
    if (!svg_consume_float (&p, &angle))
        err = 1;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &cx))
        cx = 0.0;
    svg_consume_whitespace (&p);
    if (!svg_consume_float (&p, &cy))
        cy = 0.0;
    svg_consume_whitespace (&p);
    if (*p == ')')
        p++;
    else
        err = 1;
    if (!err)
      {
          *ptr = p;
          return svg_alloc_rotate (angle, cx, cy);
      }
    return NULL;
}

static void *
svg_parse_skew (const char **ptr)
{
/* parsing an SVG Skew definition */
    const char *p = *ptr;
    double skew = 0.0;
    int err = 0;
    if (!svg_consume_float (&p, &skew))
        err = 1;
    if (!err)
      {
          *ptr = p;
          return svg_alloc_skew (skew);
      }
    return NULL;
}

static void
svg_parse_transform_str (rl2PrivSvgGroupPtr group,
                         rl2PrivSvgShapePtr shape, rl2PrivSvgUsePtr use,
                         rl2PrivSvgGradientPtr gradient, const char *str)
{
/* parsing an SVG Transform string */
    rl2PrivSvgTransformPtr trans;
    const char *p_in = str;
    int type;
    void *data;

    while (1)
      {
          svg_consume_whitespace (&p_in);
          if (*p_in == '\0')
              break;
          type = svg_find_transform_mode (&p_in);
          if (type == RL2_SVG_UNKNOWN)
              break;
          svg_consume_whitespace (&p_in);
          if (*p_in == '\0')
              break;
          if (*p_in != '(')
              break;
          p_in++;
          switch (type)
            {
            case RL2_SVG_MATRIX:
                data = svg_parse_matrix (&p_in);
                break;
            case RL2_SVG_TRANSLATE:
                data = svg_parse_translate (&p_in);
                break;
            case RL2_SVG_SCALE:
                data = svg_parse_scale (&p_in);
                break;
            case RL2_SVG_ROTATE:
                data = svg_parse_rotate (&p_in);
                break;
            case RL2_SVG_SKEW_X:
            case RL2_SVG_SKEW_Y:
                data = svg_parse_skew (&p_in);
                break;
            default:
                data = NULL;
                break;
            };
          if (data == NULL)
              return;
          trans = svg_alloc_transform (type, data);
          if (group != NULL)
            {
                if (group->first_trans == NULL)
                    group->first_trans = trans;
                if (group->last_trans != NULL)
                    group->last_trans->next = trans;
                group->last_trans = trans;
            }
          else if (shape != NULL)
            {
                if (shape->first_trans == NULL)
                    shape->first_trans = trans;
                if (shape->last_trans != NULL)
                    shape->last_trans->next = trans;
                shape->last_trans = trans;
            }
          else if (use != NULL)
            {
                if (use->first_trans == NULL)
                    use->first_trans = trans;
                if (use->last_trans != NULL)
                    use->last_trans->next = trans;
                use->last_trans = trans;
            }
          else if (gradient != NULL)
            {
                if (gradient->first_trans == NULL)
                    gradient->first_trans = trans;
                if (gradient->last_trans != NULL)
                    gradient->last_trans->next = trans;
                gradient->last_trans = trans;
            }
      }
}

static void
svg_parse_transform (rl2PrivSvgGroupPtr group, rl2PrivSvgShapePtr shape,
                     rl2PrivSvgUsePtr use,
                     rl2PrivSvgGradientPtr gradient, struct _xmlAttr *attr)
{
/* parsing SVG Transform-related definitions */
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (gradient == NULL)
                        {
                            if (strcmp (name, "transform") == 0)
                                svg_parse_transform_str (group, shape, use,
                                                         NULL, value);
                        }
                      else
                        {
                            if (strcmp (name, "gradientTransform") == 0)
                                svg_parse_transform_str (NULL, NULL, NULL,
                                                         gradient, value);
                        }
                  }
            }
          attr = attr->next;
      }
}

static void
svg_parse_clip_url (rl2PrivSvgStylePtr style, const char *value)
{
/* parsing an SVG clip-path attribute (URL) */
    int len = strlen (value);
    if (strncmp (value, "url(#", 5) == 0 && *(value + len - 1) == ')')
      {
          char buf[1024];
          strcpy (buf, value + 5);
          len = strlen (buf);
          buf[len - 1] = '\0';
          svg_add_clip_url (style, buf);
      }
}

static void
svg_parse_clip_path (rl2PrivSvgGroupPtr group, rl2PrivSvgShapePtr shape,
                     rl2PrivSvgUsePtr use, struct _xmlAttr *attr)
{
/* parsing SVG clip-path definitions */
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "clip-path") == 0)
                        {
                            if (group != NULL)
                                svg_parse_clip_url (&(group->style), value);
                            if (shape != NULL)
                                svg_parse_clip_url (&(shape->style), value);
                            if (use != NULL)
                                svg_parse_clip_url (&(use->style), value);
                        }
                  }
            }
          attr = attr->next;
      }
}

static void
svg_parse_rect (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Rect struct */
    rl2PrivSvgRectPtr rect;
    struct _xmlAttr *attr;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    double rx = -1.0;
    double ry = -1.0;

    attr = node->properties;
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "x") == 0)
                          x = atof (value);
                      if (strcmp (name, "y") == 0)
                          y = atof (value);
                      if (strcmp (name, "width") == 0)
                          width = atof (value);
                      if (strcmp (name, "height") == 0)
                          height = atof (value);
                      if (strcmp (name, "rx") == 0)
                          rx = atof (value);
                      if (strcmp (name, "ry") == 0)
                          ry = atof (value);
                  }
            }
          attr = attr->next;
      }
    if (rx > 0.0 && ry <= 0.0)
        ry = rx;
    if (ry > 0.0 && rx <= 0.0)
        rx = ry;
    rect = svg_alloc_rect (x, y, width, height, rx, ry);
    svg_insert_shape (svg_doc, RL2_SVG_RECT, rect);
    svg_parse_id (NULL, NULL, svg_doc->current_shape, node->properties);
    svg_parse_style (NULL, svg_doc->current_shape, NULL, node->properties);
    svg_parse_transform (NULL, svg_doc->current_shape, NULL, NULL,
                         node->properties);
    svg_parse_clip_path (NULL, svg_doc->current_shape, NULL, node->properties);
}

static void
svg_parse_circle (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Circle struct */
    rl2PrivSvgCirclePtr circle;
    struct _xmlAttr *attr;
    double cx = 0.0;
    double cy = 0.0;
    double r = 0.0;

    attr = node->properties;
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "cx") == 0)
                          cx = atof (value);
                      if (strcmp (name, "cy") == 0)
                          cy = atof (value);
                      if (strcmp (name, "r") == 0)
                          r = atof (value);
                  }
            }
          attr = attr->next;
      }
    circle = svg_alloc_circle (cx, cy, r);
    svg_insert_shape (svg_doc, RL2_SVG_CIRCLE, circle);
    svg_parse_id (NULL, NULL, svg_doc->current_shape, node->properties);
    svg_parse_style (NULL, svg_doc->current_shape, NULL, node->properties);
    svg_parse_transform (NULL, svg_doc->current_shape, NULL, NULL,
                         node->properties);
    svg_parse_clip_path (NULL, svg_doc->current_shape, NULL, node->properties);
}

static void
svg_parse_ellipse (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Ellipse struct */
    rl2PrivSvgEllipsePtr ellipse;
    struct _xmlAttr *attr;
    double cx = 0.0;
    double cy = 0.0;
    double rx = 0.0;
    double ry = 0.0;

    attr = node->properties;
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "cx") == 0)
                          cx = atof (value);
                      if (strcmp (name, "cy") == 0)
                          cy = atof (value);
                      if (strcmp (name, "rx") == 0)
                          rx = atof (value);
                      if (strcmp (name, "ry") == 0)
                          ry = atof (value);
                  }
            }
          attr = attr->next;
      }
    ellipse = svg_alloc_ellipse (cx, cy, rx, ry);
    svg_insert_shape (svg_doc, RL2_SVG_ELLIPSE, ellipse);
    svg_parse_id (NULL, NULL, svg_doc->current_shape, node->properties);
    svg_parse_style (NULL, svg_doc->current_shape, NULL, node->properties);
    svg_parse_transform (NULL, svg_doc->current_shape, NULL, NULL,
                         node->properties);
    svg_parse_clip_path (NULL, svg_doc->current_shape, NULL, node->properties);
}

static void
svg_parse_line (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Line struct */
    rl2PrivSvgLinePtr line;
    struct _xmlAttr *attr;
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;

    attr = node->properties;
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "x1") == 0)
                          x1 = atof (value);
                      if (strcmp (name, "y1") == 0)
                          y1 = atof (value);
                      if (strcmp (name, "x2") == 0)
                          x2 = atof (value);
                      if (strcmp (name, "y2") == 0)
                          y2 = atof (value);
                  }
            }
          attr = attr->next;
      }
    line = svg_alloc_line (x1, y1, x2, y2);
    svg_insert_shape (svg_doc, RL2_SVG_LINE, line);
    svg_parse_id (NULL, NULL, svg_doc->current_shape, node->properties);
    svg_parse_style (NULL, svg_doc->current_shape, NULL, node->properties);
    svg_parse_transform (NULL, svg_doc->current_shape, NULL, NULL,
                         node->properties);
    svg_parse_clip_path (NULL, svg_doc->current_shape, NULL, node->properties);
}

static void
svg_parse_polyline (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Polyline struct */
    rl2PrivSvgPolylinePtr poly;
    struct _xmlAttr *attr;
    int points = 0;
    double *x = NULL;
    double *y = NULL;

    attr = node->properties;
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "points") == 0)
                          svg_parse_points (value, &points, &x, &y);
                  }
            }
          attr = attr->next;
      }
    poly = svg_alloc_polyline (points, x, y);
    svg_insert_shape (svg_doc, RL2_SVG_POLYLINE, poly);
    svg_parse_id (NULL, NULL, svg_doc->current_shape, node->properties);
    svg_parse_style (NULL, svg_doc->current_shape, NULL, node->properties);
    svg_parse_transform (NULL, svg_doc->current_shape, NULL, NULL,
                         node->properties);
    svg_parse_clip_path (NULL, svg_doc->current_shape, NULL, node->properties);
}

static void
svg_parse_polygon (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Polygon struct */
    rl2PrivSvgPolygonPtr poly;
    struct _xmlAttr *attr;
    int points = 0;
    double *x = NULL;
    double *y = NULL;

    attr = node->properties;
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "points") == 0)
                          svg_parse_points (value, &points, &x, &y);
                  }
            }
          attr = attr->next;
      }
    poly = svg_alloc_polygon (points, x, y);
    svg_insert_shape (svg_doc, RL2_SVG_POLYGON, poly);
    svg_parse_id (NULL, NULL, svg_doc->current_shape, node->properties);
    svg_parse_style (NULL, svg_doc->current_shape, NULL, node->properties);
    svg_parse_transform (NULL, svg_doc->current_shape, NULL, NULL,
                         node->properties);
    svg_parse_clip_path (NULL, svg_doc->current_shape, NULL, node->properties);
}

static void
svg_parse_path (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Path struct */
    rl2PrivSvgPathPtr path = svg_alloc_path ();
    struct _xmlAttr *attr;

    attr = node->properties;
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "d") == 0)
                        {
                            svg_parse_path_d (path, value);
                            if (path->first == NULL || path->error)
                              {
                                  /* invalid path */
                                  svg_free_path (path);
                                  return;
                              }
                        }
                  }
            }
          attr = attr->next;
      }
    svg_insert_shape (svg_doc, RL2_SVG_PATH, path);
    svg_parse_id (NULL, NULL, svg_doc->current_shape, node->properties);
    svg_parse_style (NULL, svg_doc->current_shape, NULL, node->properties);
    svg_parse_transform (NULL, svg_doc->current_shape, NULL, NULL,
                         node->properties);
    svg_parse_clip_path (NULL, svg_doc->current_shape, NULL, node->properties);
}

static void
svg_parse_group (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Group struct */
    svg_insert_group (svg_doc);
    svg_parse_id (svg_doc->current_group, NULL, NULL, node->properties);
    svg_parse_style (svg_doc->current_group, NULL, NULL, node->properties);
    svg_parse_transform (svg_doc->current_group, NULL, NULL, NULL,
                         node->properties);
    svg_parse_clip_path (svg_doc->current_group, NULL, NULL, node->properties);
}

static void
svg_parse_clip (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG ClipPath struct */
    svg_insert_clip (svg_doc);
    svg_parse_id (NULL, svg_doc->current_clip, NULL, node->properties);
}

static void
svg_parse_use (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG Use struct */
    const char *xlink_href = NULL;
    double x = DBL_MAX;
    double y = DBL_MAX;
    double width = DBL_MAX;
    double height = DBL_MAX;
    rl2PrivSvgUsePtr use;
    struct _xmlAttr *attr = node->properties;

    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "href") == 0)
                          xlink_href = value;
                      if (strcmp (name, "x") == 0)
                          x = atof (value);
                      if (strcmp (name, "y") == 0)
                          y = atof (value);
                      if (strcmp (name, "width") == 0)
                          width = atof (value);
                      if (strcmp (name, "height") == 0)
                          height = atof (value);
                  }
            }
          attr = attr->next;
      }
    if (xlink_href == NULL)
        return;

    use = svg_insert_use (svg_doc, xlink_href, x, y, width, height);
    svg_parse_style (NULL, NULL, use, node->properties);
    svg_parse_transform (NULL, NULL, use, NULL, node->properties);
    svg_parse_clip_path (NULL, NULL, use, node->properties);
}

static void
svg_parse_gradient_stop (rl2PrivSvgGradientPtr gradient, xmlNodePtr node)
{
/* parsing an SVG Node - Gradient - Stop */
    while (node)
      {
          if (node->type == XML_ELEMENT_NODE)
            {
                const char *name = (const char *) (node->name);
                if (strcmp (name, "stop") == 0)
                  {
                      double offset = DBL_MAX;
                      double red = -1.0;
                      double green = -1.0;
                      double blue = -1.0;
                      double opacity = -1.0;
                      struct _xmlAttr *attr = node->properties;
                      while (attr)
                        {
                            if (attr->type == XML_ATTRIBUTE_NODE)
                              {
                                  const char *value = NULL;
                                  const char *name;
                                  xmlNodePtr child = attr->children;
                                  name = (const char *) (attr->name);
                                  if (child)
                                      value = (const char *) (child->content);
                                  if (child && value)
                                    {
                                        if (strcmp (name, "offset") == 0)
                                          {
                                              int percent = 0;
                                              int i;
                                              for (i = 0;
                                                   i < (int) strlen (value);
                                                   i++)
                                                {
                                                    if (value[i] == '%')
                                                        percent = 1;
                                                }
                                              offset = atof (value);
                                              if (percent)
                                                  offset /= 100.0;
                                              if (offset < 0.0)
                                                  offset = 0.0;
                                              if (offset > 1.0)
                                                  offset = 1.0;
                                          }
                                        if (strcmp (name, "style") == 0)
                                            svg_parse_stop_style (value,
                                                                  &red,
                                                                  &green,
                                                                  &blue,
                                                                  &opacity);
                                        if (strcmp (name, "stop-color") == 0)
                                          {
                                              opacity = 1.0;
                                              svg_parse_stop_color (value,
                                                                    &red,
                                                                    &green,
                                                                    &blue);
                                          }
                                    }
                              }
                            attr = attr->next;
                        }
                      svg_insert_gradient_stop (gradient, offset, red, green,
                                                blue, opacity);
                  }
            }
          node = node->next;
      }
}

static void
svg_parse_linear_gradient (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG LinearGradient struct */
    const char *xlink_href = NULL;
    const char *id = NULL;
    double x1 = DBL_MAX;
    double y1 = DBL_MAX;
    double x2 = DBL_MAX;
    double y2 = DBL_MAX;
    int units = RL2_SVG_BOUNDING_BOX;
    rl2PrivSvgGradientPtr gradient;
    struct _xmlAttr *attr = node->properties;

    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "href") == 0)
                          xlink_href = value;
                      if (strcmp (name, "id") == 0)
                          id = value;
                      if (strcmp (name, "x1") == 0)
                          x1 = atof (value);
                      if (strcmp (name, "y1") == 0)
                          y1 = atof (value);
                      if (strcmp (name, "x2") == 0)
                          x2 = atof (value);
                      if (strcmp (name, "y2") == 0)
                          y2 = atof (value);
                      if (strcmp (name, "gradientUnits") == 0)
                        {
                            if (strcmp (value, "userSpaceOnUse") == 0)
                                units = RL2_SVG_USER_SPACE;
                        }
                  }
            }
          attr = attr->next;
      }
    if (x1 == DBL_MAX)
        x1 = svg_doc->viewbox_x;
    if (y1 == DBL_MAX)
        y1 = svg_doc->viewbox_y;
    if (x2 == DBL_MAX)
        x2 = svg_doc->viewbox_width;
    if (y2 == DBL_MAX)
        y2 = y1;

    gradient =
        svg_insert_linear_gradient (svg_doc, id, xlink_href, x1, y1, x2,
                                    y2, units);
    svg_parse_gradient_stop (gradient, node->children);
    svg_parse_transform (NULL, NULL, NULL, gradient, node->properties);
}

static void
svg_parse_radial_gradient (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* creating and initializing an SVG RadialGradient struct */
    const char *xlink_href = NULL;
    const char *id = NULL;
    double cx = DBL_MAX;
    double cy = DBL_MAX;
    double fx = DBL_MAX;
    double fy = DBL_MAX;
    double r = DBL_MAX;
    int units = RL2_SVG_BOUNDING_BOX;
    rl2PrivSvgGradientPtr gradient;
    struct _xmlAttr *attr = node->properties;

    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      if (strcmp (name, "href") == 0)
                          xlink_href = value;
                      if (strcmp (name, "id") == 0)
                          id = value;
                      if (strcmp (name, "cx") == 0)
                          cx = atof (value);
                      if (strcmp (name, "cy") == 0)
                          cy = atof (value);
                      if (strcmp (name, "fx") == 0)
                          fx = atof (value);
                      if (strcmp (name, "fy") == 0)
                          fy = atof (value);
                      if (strcmp (name, "r") == 0)
                          r = atof (value);
                      if (strcmp (name, "gradientUnits") == 0)
                        {
                            if (strcmp (value, "userSpaceOnUse") == 0)
                                units = RL2_SVG_USER_SPACE;
                        }
                  }
            }
          attr = attr->next;
      }
    if (cx == DBL_MAX)
        cx = svg_doc->viewbox_width / 2.0;
    if (cy == DBL_MAX)
        cy = svg_doc->viewbox_height / 2.0;
    if (r == DBL_MAX)
        r = svg_doc->viewbox_width / 2.0;
    if (fx == DBL_MAX)
        fx = cx;
    if (fy == DBL_MAX)
        fy = cy;

    gradient =
        svg_insert_radial_gradient (svg_doc, id, xlink_href, cx, cy, fx,
                                    fy, r, units);
    svg_parse_gradient_stop (gradient, node->children);
    svg_parse_transform (NULL, NULL, NULL, gradient, node->properties);
}

static void
svg_parse_node (rl2PrivSvgDocumentPtr svg_doc, xmlNodePtr node)
{
/* parsing an SVG Node */
    while (node)
      {
          int is_group = 0;
          int is_defs = 0;
          int is_flow_root = 0;
          int is_clip_path = 0;
          if (node->type == XML_ELEMENT_NODE)
            {
                const char *name = (const char *) (node->name);
                if (strcmp (name, "defs") == 0)
                  {
                      svg_doc->defs_count += 1;
                      is_defs = 1;
                  }
                if (strcmp (name, "flowRoot") == 0)
                  {
                      svg_doc->flow_root_count += 1;
                      is_flow_root = 1;
                  }
                if (strcmp (name, "clipPath") == 0)
                  {
                      svg_parse_clip (svg_doc, node);
                      is_clip_path = 1;
                  }
                if (strcmp (name, "g") == 0)
                  {
                      svg_parse_group (svg_doc, node);
                      is_group = 1;
                  }
                if (strcmp (name, "rect") == 0)
                    svg_parse_rect (svg_doc, node);
                if (strcmp (name, "circle") == 0)
                    svg_parse_circle (svg_doc, node);
                if (strcmp (name, "ellipse") == 0)
                    svg_parse_ellipse (svg_doc, node);
                if (strcmp (name, "line") == 0)
                    svg_parse_line (svg_doc, node);
                if (strcmp (name, "polyline") == 0)
                    svg_parse_polyline (svg_doc, node);
                if (strcmp (name, "polygon") == 0)
                    svg_parse_polygon (svg_doc, node);
                if (strcmp (name, "path") == 0)
                    svg_parse_path (svg_doc, node);
                if (strcmp (name, "use") == 0)
                    svg_parse_use (svg_doc, node);
                if (strcmp (name, "linearGradient") == 0)
                    svg_parse_linear_gradient (svg_doc, node);
                if (strcmp (name, "radialGradient") == 0)
                    svg_parse_radial_gradient (svg_doc, node);
            }
          svg_parse_node (svg_doc, node->children);
          if (is_group)
              svg_close_group (svg_doc);
          if (is_defs)
              svg_doc->defs_count -= 1;
          if (is_flow_root)
              svg_doc->flow_root_count -= 1;
          if (is_clip_path)
              svg_close_clip (svg_doc);
          node = node->next;
      }
}

static void
svg_parse_viewbox (rl2PrivSvgDocumentPtr svg_doc, const char *str)
{
/* parsing an SVG ViewBox */
    double value;
    const char *p = str;
    if (!svg_consume_float (&p, &value))
        return;
    svg_doc->viewbox_x = value;
    if (!svg_consume_float (&p, &value))
        return;
    svg_doc->viewbox_y = value;
    if (!svg_consume_float (&p, &value))
        return;
    svg_doc->viewbox_width = value;
    if (!svg_consume_float (&p, &value))
        return;
    svg_doc->viewbox_height = value;
}

static void
svg_parse_header (rl2PrivSvgDocumentPtr svg_doc, struct _xmlAttr *attr)
{
/* parsing the SVG header definitions */
    while (attr)
      {
          if (attr->type == XML_ATTRIBUTE_NODE)
            {
                const char *value = NULL;
                const char *name;
                xmlNodePtr child = attr->children;
                name = (const char *) (attr->name);
                if (child)
                    value = (const char *) (child->content);
                if (child && value)
                  {
                      int len;
                      double factor = 1.0;
                      if (strcmp (name, "width") == 0)
                        {
                            len = strlen (value);
                            if (len > 3)
                              {
                                  if (strcmp (value + len - 2, "mm") == 0)
                                      factor = 72.0 / 25.4;
                                  else if (strcmp (value + len - 2, "cm") == 0)
                                      factor = 72.0 / 2.54;
                                  else if (strcmp (value + len - 2, "in") == 0)
                                      factor = 72.0;
                                  else if (strcmp (value + len - 2, "pc") == 0)
                                      factor = 72.0 / 6.0;
                              }
                            svg_doc->width = atof (value) * factor;
                        }
                      if (strcmp (name, "height") == 0)
                        {
                            len = strlen (value);
                            if (len > 3)
                              {
                                  if (strcmp (value + len - 2, "mm") == 0)
                                      factor = 72.0 / 25.4;
                                  else if (strcmp (value + len - 2, "cm") == 0)
                                      factor = 72.0 / 2.54;
                                  else if (strcmp (value + len - 2, "in") == 0)
                                      factor = 72.0;
                                  else if (strcmp (value + len - 2, "pc") == 0)
                                      factor = 72.0 / 6.0;
                              }
                            svg_doc->height = atof (value) * factor;
                        }
                      if (strcmp (name, "viewBox") == 0)
                          svg_parse_viewbox (svg_doc, value);
                  }
            }
          attr = attr->next;
      }
}

RL2_PRIVATE rl2PrivSvgDocumentPtr
svg_parse_doc (const unsigned char *svg, int svg_len)
{
/* attempting to parse the SVG Document */
    rl2PrivSvgDocumentPtr svg_doc;
    xmlNodePtr node;
    xmlDocPtr xml_doc =
        xmlReadMemory ((const char *) svg, svg_len, "noname.svg", NULL, 0);
    if (xml_doc == NULL)
      {
          /* parsing error; not a well-formed XML */
          fprintf (stderr, "XML parsing error\n");
          return NULL;
      }
    svg_doc = svg_alloc_document ();
    node = xmlDocGetRootElement (xml_doc);
    svg_parse_header (svg_doc, node->properties);
    svg_parse_node (svg_doc, node);
    xmlFreeDoc (xml_doc);
    return svg_doc;
}
