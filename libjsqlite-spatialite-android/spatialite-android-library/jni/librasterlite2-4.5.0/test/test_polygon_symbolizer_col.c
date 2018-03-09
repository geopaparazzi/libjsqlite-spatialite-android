/*

 test_polygon_symbolizer.c -- RasterLite-2 Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

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
 
Portions created by the Initial Developer are Copyright (C) 2017
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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"

static int
test_symbolizer (sqlite3 * db_handle, const char *coverage,
		 const char *style_name, int *retcode)
{
/* testing a PolygonSymbolizer */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2PolygonSymbolizerPtr polyg;
    int intval;
    int index;
    int ret;
    double dblval;
    const char *string;
    unsigned char red;
    int scale_forbidden;
    style =
	rl2_create_feature_type_style_from_dbms (db_handle, NULL, coverage,
						 style_name);
    if (style == NULL)
      {
	  fprintf (stderr, "Unable to retrieve style '%s'\n", style_name);
	  *retcode += 1;
	  return 0;
      }

    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 1.0, NULL,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s)\n",
		   style_name);
	  *retcode += 2;
	  return 0;
      }

    if (rl2_is_valid_vector_symbolizer (symbolizer, &intval) != RL2_OK)
      {
	  fprintf (stderr, "%s: Unable to get Vector Symbolizer Validity #1\n",
		   style_name);
	  *retcode += 3;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "%s: Unexpected Vector Symbolizer Validity #1: %d\n",
		   style_name, intval);
	  *retcode += 4;
	  return 0;
      }

    if (rl2_get_vector_symbolizer_count (symbolizer, &intval) != RL2_OK)
      {
	  fprintf (stderr, "%s: Unable to get Vector Symbolizer Count #1\n",
		   style_name);
	  *retcode += 5;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "%s: Unexpected Vector Symbolizer Count #1: %d\n",
		   style_name, intval);
	  *retcode += 6;
	  return 0;
      }

    if (rl2_get_vector_symbolizer_item_type (symbolizer, 0, &intval) != RL2_OK)
      {
	  fprintf (stderr, "%s: Unable to get Vector Symbolizer Item Type #1\n",
		   style_name);
	  *retcode += 7;
	  return 0;
      }
    if (intval != RL2_POLYGON_SYMBOLIZER)
      {
	  fprintf (stderr,
		   "%s: Unexpected Vector Symbolizer Item Type #1: %d\n",
		   style_name, intval);
	  *retcode += 8;
	  return 0;
      }

    polyg = rl2_get_polygon_symbolizer (symbolizer, 0);
    if (polyg == NULL)
      {
	  fprintf (stderr, "%s: Unable to get a Polygon Symbolizer #1\n",
		   style_name);
	  *retcode += 9;
	  return 0;
      }

    if (rl2_polygon_symbolizer_has_stroke (polyg, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer HasStroke #1\n",
		   style_name);
	  *retcode += 10;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_2_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer HasStroke #1: %d\n",
		   style_name, intval);
	  *retcode += 11;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_stroke_color (polyg);
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColStrokeColor #1\n",
			 style_name);
		*retcode += 12;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColStrokeColor #1\n",
			 style_name);
		*retcode += 13;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (strcmp (string, "stroke_color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetColStrokeColor #1: %s\n",
		   style_name, string);
	  *retcode += 14;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_stroke_opacity (polyg);
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColStrokeOpacity #1\n",
			 style_name);
		*retcode += 15;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColStrokeOpacity #1\n",
			 style_name);
		*retcode += 16;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (strcmp (string, "stroke_opacity") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (strcmp (string, "stroke_opacity") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetColStrokeOpacity #1: %s\n",
		   style_name, string);
	  *retcode += 17;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_stroke_width (polyg);
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColStrokeWidth #1\n",
			 style_name);
		*retcode += 15;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColStrokeWidth #1\n",
			 style_name);
		*retcode += 16;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (strcmp (string, "width") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (strcmp (string, "width") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetColStrokeWidth #1: %s\n",
		   style_name, string);
	  *retcode += 17;
	  return 0;
      }

    ret = rl2_polygon_symbolizer_get_stroke_linejoin (polyg, &red);
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetStrokeLinejoin #1\n",
			 style_name);
		*retcode += 16;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetStrokeLinejoin #1\n",
			 style_name);
		*retcode += 17;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_UNKNOWN)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_BEVEL)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetStrokeLinejoin #1: %02x\n",
		   style_name, red);
	  *retcode += 18;
	  return 0;
      }

    ret = rl2_polygon_symbolizer_get_stroke_linecap (polyg, &red);
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetStrokeLinecap #1\n",
			 style_name);
		*retcode += 19;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetStrokeLinecap #1\n",
			 style_name);
		*retcode += 20;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_UNKNOWN)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_BUTT)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetStrokeLinecap #1: %02x\n",
		   style_name, red);
	  *retcode += 21;
	  return 0;
      }

    ret = rl2_polygon_symbolizer_get_stroke_dash_count (polyg, &intval);
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetStrokeDashCount #1\n",
			 style_name);
		*retcode += 22;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetStrokeDashCount #1\n",
			 style_name);
		*retcode += 23;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
	red = 1;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (intval == 6)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetStrokeDashCount #1: %d\n",
		   style_name, intval);
	  *retcode += 24;
	  return 0;
      }

    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (rl2_polygon_symbolizer_get_stroke_dash_item (polyg, 0, &dblval) ==
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Expected failure, got success: Polygon Symbolizer GetStrokeDashItem #1\n");
		*retcode += 25;
		return 0;
	    }
      }
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (rl2_polygon_symbolizer_get_stroke_dash_item (polyg, 0, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetStrokeDashItem #1\n",
			 style_name);
		*retcode += 26;
		return 0;
	    }
	  if (dblval != 20.0)
	    {
		fprintf (stderr,
			 "Unexpected Polygon Symbolizer GetStrokDashItem #1: %1.4f\n",
			 dblval);
		*retcode += 27;
		return 0;
	    }
	  if (rl2_polygon_symbolizer_get_stroke_dash_item (polyg, 1, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Polygon Symbolizer GetStrokeDashItem #2\n");
		*retcode += 28;
		return 0;
	    }
	  if (dblval != 10.1)
	    {
		fprintf (stderr,
			 "Unexpected Polygon Symbolizer GetStrokDashItem #2: %1.4f\n",
			 dblval);
		*retcode += 29;
		return 0;
	    }
	  if (rl2_polygon_symbolizer_get_stroke_dash_item (polyg, 2, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Polygon Symbolizer GetStrokeDashItem #3\n");
		*retcode += 30;
		return 0;
	    }
	  if (dblval != 5.5)
	    {
		fprintf (stderr,
			 "Unexpected Polygon Symbolizer GetStrokDashItem #3: %1.4f\n",
			 dblval);
		*retcode += 31;
		return 0;
	    }
	  if (rl2_polygon_symbolizer_get_stroke_dash_item (polyg, 3, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Polygon Symbolizer GetStrokeDashItem #4\n");
		*retcode += 32;
		return 0;
	    }
	  if (dblval != 4.0)
	    {
		fprintf (stderr,
			 "Unexpected Polygon Symbolizer GetStrokDashItem #4: %1.4f\n",
			 dblval);
		*retcode += 33;
		return 0;
	    }
	  if (rl2_polygon_symbolizer_get_stroke_dash_item (polyg, 4, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Polygon Symbolizer GetStrokeDashItem #5\n");
		*retcode += 34;
		return 0;
	    }
	  if (dblval != 6.0)
	    {
		fprintf (stderr,
			 "Unexpected Polygon Symbolizer GetStrokDashItem #4: %1.4f\n",
			 dblval);
		*retcode += 35;
		return 0;
	    }
	  if (rl2_polygon_symbolizer_get_stroke_dash_item (polyg, 5, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Polygon Symbolizer GetStrokeDashItem #5\n");
		*retcode += 36;
		return 0;
	    }
	  if (dblval != 12.0)
	    {
		fprintf (stderr,
			 "Unexpected Polygon Symbolizer GetStrokDashItem #5: %1.4f\n",
			 dblval);
		*retcode += 37;
		return 0;
	    }
      }

    ret = rl2_polygon_symbolizer_get_stroke_dash_offset (polyg, &dblval);
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetStrokeDashOffset #1\n",
			 style_name);
		*retcode += 38;
		return 0;
	    }
	  dblval = 0.0;
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetStrokeDashOffset #1\n",
			 style_name);
		*retcode += 39;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (dblval == 0.0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (dblval == 10.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetStrokeDashOffset #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 40;
	  return 0;
      }

    if (rl2_polygon_symbolizer_has_fill (polyg, &intval) != RL2_OK)
      {
	  fprintf (stderr, "%s: Unable to get Polygon Symbolizer HasFill #1\n",
		   style_name);
	  *retcode += 41;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr, "%s: Unexpected Polygon Symbolizer HasFill #1: %d\n",
		   style_name, intval);
	  *retcode += 42;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_fill_color (polyg);
    if (strcmp (style_name, "polygon_2_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColFillColor #1\n",
			 style_name);
		*retcode += 43;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColFillColor #1\n",
			 style_name);
		*retcode += 44;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (strcmp (string, "fill_color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetColFillColor #1: %s\n",
		   style_name, string);
	  *retcode += 45;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_fill_opacity (polyg);
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColFillOpacity #1\n",
			 style_name);
		*retcode += 46;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColFillOpacity #1\n",
			 style_name);
		*retcode += 47;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (strcmp (string, "fill_opacity") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2_col") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (strcmp (string, "fill_opacity") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetColFillOpacity #1: %s\n",
		   style_name, string);
	  *retcode += 48;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_displacement_x (polyg);
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: nexpected success Polygon Symbolizer GetColDisplacementX #1\n",
			 style_name);
		*retcode += 49;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColDisplacementX #1\n",
			 style_name);
		*retcode += 50;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (strcmp (string, "displ_x") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected polygon Symbolizer GetColDisplacementX #1: %s\n",
		   style_name, string);
	  *retcode += 51;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_displacement_y (polyg);
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColDisplacementX #1\n",
			 style_name);
		*retcode += 51;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColDisplacementY #1\n",
			 style_name);
		*retcode += 52;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (strcmp (string, "displ_y") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected polygon Symbolizer GetColDisplacementY #1: %s\n",
		   style_name, string);
	  *retcode += 53;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_perpendicular_offset (polyg);
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColPerpendicularOffset #1\n",
			 style_name);
		*retcode += 54;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColPerpendicularOffset #1\n",
			 style_name);
		*retcode += 55;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (strcmp (string, "perpoff") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected polygon Symbolizer GetColPerpendicularOffset #1: %s\n",
		   style_name, string);
	  *retcode += 56;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_graphic_stroke_href (polyg);
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (string == NULL)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (string != NULL)
	    {
		if (strcmp (string, "pen_href") == 0)
		    intval = 1;
	    }
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer GetColGraphicStrokeHref #1: %s\n",
		   style_name, string);
	  *retcode += 57;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_stroke_recode_count (polyg, &intval)
	!= RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer GetGraphicStrokeRecodeCount #1\n",
		   style_name);
	  *retcode += 58;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer GetColGraphicStrokeRecodeCount #1: %d\n",
		   style_name, intval);
	  *retcode += 59;
	  return 0;
      }

    string =
	rl2_polygon_symbolizer_get_col_graphic_stroke_recode_color (polyg, 0,
								    &index);
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColGraphicStrokeRecodeColor #1\n",
			 style_name);
		*retcode += 60;
		return 0;
	    }
	  intval = 1;
      }
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColGraphicStrokeRecodeColor #1\n",
			 style_name);
		*retcode += 61;
		return 0;
	    }
	  intval = 0;
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetColGraphicStrokeRecodeColor #1: %s\n",
		   style_name, string);
	  *retcode += 62;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_col_graphic_fill_href (polyg);
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (string == NULL)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (string != NULL)
	    {
		if (strcmp (string, "brush_href") == 0)
		    intval = 1;
	    }
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer GetColGraphicFillHref #1: %s\n",
		   style_name, string);
	  *retcode += 63;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_fill_recode_count (polyg, &intval) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer GetGraphicFillRecodeCount #1\n",
		   style_name);
	  *retcode += 64;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer GetGraphicFillRecodeCount #1: %d\n",
		   style_name, intval);
	  *retcode += 65;
	  return 0;
      }

    string =
	rl2_polygon_symbolizer_get_col_graphic_fill_recode_color (polyg, 0,
								  &index);
    if (strcmp (style_name, "polygon_1_col") == 0
	|| strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetColGraphicFillRecodeColor #1\n",
			 style_name);
		*retcode += 66;
		return 0;
	    }
	  intval = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetColGraphicFillRecodeColor #1\n",
			 style_name);
		*retcode += 67;
		return 0;
	    }
	  intval = 0;
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetColGraphicFillRecodeColor #1: %s\n",
		   style_name, string);
	  *retcode += 68;
	  return 0;
      }

    intval = rl2_get_feature_type_style_columns_count (style);
    red = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (intval == 7)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (intval == 6)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer StyleColumnsCount #1: %d\n",
		   style_name, intval);
	  *retcode += 69;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 0);
    if (string == NULL)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer StyleColumnName #1\n",
		   style_name);
	  *retcode += 70;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (strcmp (string, "fill_color") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (strcmp (string, "pen_href") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (strcmp (string, "stroke_color") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer StyleColumnsName #1: %s\n",
		   style_name, string);
	  *retcode += 71;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 5);
    if (strcmp (style_name, "polygon_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Polygon Symbolizer StyleColumnName #2\n",
			 style_name);
		*retcode += 72;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer StyleColumnName #2\n",
			 style_name);
		*retcode += 73;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_2_col") == 0)
      {
	  if (strcmp (string, "perpoff") == 0)
	      intval = 1;
      }
    else if (strcmp (style_name, "polygon_3_col") == 0)
      {
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer StyleColumnsName #2: %s\n",
		   style_name, string);
	  *retcode += 74;
	  return 0;
      }

    rl2_destroy_feature_type_style (style);
    return 1;
}

int
main (int argc, char *argv[])
{
    int result = 0;
    int ret;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 ("symbolizers.sqlite", &db_handle,
			   SQLITE_OPEN_READONLY, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);

/* tests */
    ret = -100;
    if (!test_symbolizer (db_handle, "polyg1", "polygon_1_col", &ret))
	return ret;
    ret = -200;
    if (!test_symbolizer (db_handle, "polyg2", "polygon_2_col", &ret))
	return ret;
    ret = -300;
    if (!test_symbolizer (db_handle, "polyg2", "polygon_3_col", &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    return result;
}
