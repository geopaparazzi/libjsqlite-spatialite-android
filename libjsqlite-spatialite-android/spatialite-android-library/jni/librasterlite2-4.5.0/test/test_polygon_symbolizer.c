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
 
Portions created by the Initial Developer are Copyright (C) 2015
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
    double dblval2;
    const char *string;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
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
    if (strcmp (style_name, "polygon_1") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0
	|| strcmp (style_name, "polygon_3") == 0)
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

    ret = rl2_polygon_symbolizer_get_stroke_color (polyg, &red, &green, &blue);
    if (strcmp (style_name, "polygon_1") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetStrokeColor #1\n",
			 style_name);
		*retcode += 12;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetStrokeColor #1\n",
			 style_name);
		*retcode += 13;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (red == 0x00 && green == 0x00 && blue == 0x00)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetStrokeColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 14;
	  return 0;
      }

    ret = rl2_polygon_symbolizer_get_stroke_opacity (polyg, &dblval);
    if (strcmp (style_name, "polygon_1") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetStrokeOpacity #1\n",
			 style_name);
		*retcode += 15;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetStrokeOpacity #1\n",
			 style_name);
		*retcode += 16;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (dblval == 1.0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
      {
	  if (dblval == 0.75)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetStrokeOpacity #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 17;
	  return 0;
      }

    ret = rl2_polygon_symbolizer_get_stroke_width (polyg, &dblval);
    if (strcmp (style_name, "polygon_1") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetStrokeWidth #1\n",
			 style_name);
		*retcode += 15;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetStrokeWidth #1\n",
			 style_name);
		*retcode += 16;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (dblval == 2.0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
      {
	  if (dblval == 2.25)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetStrokeWidth #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 17;
	  return 0;
      }

    ret = rl2_polygon_symbolizer_get_stroke_linejoin (polyg, &red);
    if (strcmp (style_name, "polygon_1") == 0)
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
    if (strcmp (style_name, "polygon_1") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_UNKNOWN)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
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
    if (strcmp (style_name, "polygon_1") == 0)
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
    if (strcmp (style_name, "polygon_1") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_UNKNOWN)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
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
    if (strcmp (style_name, "polygon_1") == 0)
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
    if (strcmp (style_name, "polygon_1") == 0)
	red = 1;
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (intval == 6)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
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

    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
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
    if (strcmp (style_name, "polygon_2") == 0)
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
    if (strcmp (style_name, "polygon_1") == 0)
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
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
      {
	  if (dblval == 0.0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0)
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
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
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

    ret = rl2_polygon_symbolizer_get_fill_color (polyg, &red, &green, &blue);
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetFillColor #1\n",
			 style_name);
		*retcode += 43;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetFillColor #1\n",
			 style_name);
		*retcode += 44;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0)
      {
	  if (red == 0x37 && green == 0x81 && blue == 0xf2)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetFillColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 45;
	  return 0;
      }

    ret = rl2_polygon_symbolizer_get_fill_opacity (polyg, &dblval);
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetFillOpacity #1\n",
			 style_name);
		*retcode += 46;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetFillOpacity #1\n",
			 style_name);
		*retcode += 47;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0)
      {
	  if (dblval == 0.5)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0)
	intval = 1;
    if (strcmp (style_name, "polygon_3") == 0)
      {
	  if (dblval == 0.25)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetFillOpacity #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 48;
	  return 0;
      }

    ret = rl2_polygon_symbolizer_get_displacement (polyg, &dblval, &dblval2);
    if (ret != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer GetDisplacement #1\n",
		   style_name);
	  *retcode += 49;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
      {
	  if (dblval == 0.0 && dblval2 == 0.0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (dblval == 10.0 && dblval2 == 15.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected polygon Symbolizer GetDisplacement #1: x=%1.4f y=%1.4f\n",
		   style_name, dblval, dblval2);
	  *retcode += 50;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_perpendicular_offset (polyg, &dblval) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer GetPerpendicularOffset #1\n",
		   style_name);
	  *retcode += 51;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
      {
	  if (dblval == 0.0)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (dblval == 5.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected polygon Symbolizer GetPerpendicularOffset #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 52;
	  return 0;
      }

    if (rl2_polygon_symbolizer_has_graphic_stroke (polyg, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer HasGraphicStroke #1\n",
		   style_name);
	  *retcode += 53;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer HasGraphicStroke #1: %d\n",
		   style_name, intval);
	  *retcode += 54;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_graphic_stroke_href (polyg);
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
      {
	  if (string == NULL)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (string != NULL)
	    {
		if (strcmp (string, "http:/www.acme.com/sample.png") == 0)
		    intval = 1;
	    }
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer GetGraphicStrokeHref #1: %s\n",
		   style_name, string);
	  *retcode += 55;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_stroke_recode_count (polyg, &intval)
	!= RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer GetGraphicStrokeRecodeCount #1\n",
		   style_name);
	  *retcode += 56;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer GetGraphicStrokeRecodeCount #1: %d\n",
		   style_name, intval);
	  *retcode += 57;
	  return 0;
      }

    intval =
	rl2_polygon_symbolizer_get_graphic_stroke_recode_color (polyg, 0,
								&index, &red,
								&green, &blue);
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_3") == 0)
      {
	  if (intval == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetGraphicStrokeRecodeColor #1\n",
			 style_name);
		*retcode += 58;
		return 0;
	    }
	  intval = 1;
      }
    if (strcmp (style_name, "polygon_2") == 0)
      {
	  if (intval != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetGraphicStrokeRecodeColor #1\n",
			 style_name);
		*retcode += 59;
		return 0;
	    }
	  intval = 0;
	  if (index == 0 && red == 0x12 && green == 0x34 && blue == 0x56)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetGraphicStrokeRecodeColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 60;
	  return 0;
      }

    if (rl2_polygon_symbolizer_has_graphic_fill (polyg, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Polygon Symbolizer HasGraphicFill #1\n",
		   style_name);
	  *retcode += 61;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_2") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer HasGraphicFill #1: %d\n",
		   style_name, intval);
	  *retcode += 62;
	  return 0;
      }

    string = rl2_polygon_symbolizer_get_graphic_fill_href (polyg);
    intval = 0;
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_2") == 0)
      {
	  if (string == NULL)
	      intval = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
      {
	  if (string != NULL)
	    {
		if (strcmp (string, "http:/www.acme.com/sample.png") == 0)
		    intval = 1;
	    }
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Polygon Symbolizer GetGraphicFillHref #1: %s\n",
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
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_2") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
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

    intval =
	rl2_polygon_symbolizer_get_graphic_fill_recode_color (polyg, 0, &index,
							      &red, &green,
							      &blue);
    if (strcmp (style_name, "polygon_1") == 0
	|| strcmp (style_name, "polygon_2") == 0)
      {
	  if (intval == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Polygon Symbolizer GetGraphicFillRecodeColor #1\n",
			 style_name);
		*retcode += 66;
		return 0;
	    }
	  intval = 1;
      }
    if (strcmp (style_name, "polygon_3") == 0)
      {
	  if (intval != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Polygon Symbolizer GetGraphicFillRecodeColor #1\n",
			 style_name);
		*retcode += 67;
		return 0;
	    }
	  intval = 0;
	  if (index == 0 && red == 0x12 && green == 0x34 && blue == 0x56)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Polygon Symbolizer GetGraphicFillRecodeColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 68;
	  return 0;
      }

    rl2_destroy_feature_type_style (style);
    return 1;
}

static int
test_style (sqlite3 * db_handle, const char *coverage,
	    const char *style_name, int *retcode)
{
/* testing a FeatureType Style (PolygonSymbolizers) */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2PolygonSymbolizerPtr polyg;
    int intval;
    int index;
    double dblval;
    double dblval2;
    const char *string;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
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

    string = rl2_get_feature_type_style_name (style);
    if (string == NULL)
      {
	  fprintf (stderr, "Unable to retrieve style name '%s'\n", style_name);
	  *retcode += 2;
	  return 0;
      }
    if (strcmp (string, "polygon_style") != 0)
      {
	  fprintf (stderr, "%s: Unexpected style name '%s'\n", style_name,
		   string);
	  *retcode += 3;
	  return 0;
      }

    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 1.0, NULL,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s)\n",
		   style_name);
	  *retcode += 4;
	  return 0;
      }

    if (rl2_get_vector_symbolizer_count (symbolizer, &intval) != RL2_OK)
      {
	  fprintf (stderr, "%s: Unable to get Vector Symbolizer Count #2\n",
		   style_name);
	  *retcode += 5;
	  return 0;
      }
    if (intval != 2)
      {
	  fprintf (stderr, "%s: Unexpected Vector Symbolizer Count #2: %d\n",
		   style_name, intval);
	  *retcode += 6;
	  return 0;
      }

    if (rl2_get_vector_symbolizer_item_type (symbolizer, 1, &intval) != RL2_OK)
      {
	  fprintf (stderr, "%s: Unable to get Vector Symbolizer Item Type #2\n",
		   style_name);
	  *retcode += 7;
	  return 0;
      }
    if (intval != RL2_POLYGON_SYMBOLIZER)
      {
	  fprintf (stderr,
		   "%s: Unexpected Vector Symbolizer Item Type #2: %d\n",
		   style_name, intval);
	  *retcode += 8;
	  return 0;
      }

    polyg = rl2_get_polygon_symbolizer (symbolizer, 1);
    if (polyg == NULL)
      {
	  fprintf (stderr, "%s: Unable to get a Polygon Symbolizer #2\n",
		   style_name);
	  *retcode += 9;
	  return 0;
      }

    if (rl2_polygon_symbolizer_has_stroke (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer HasStroke #2\n");
	  *retcode += 10;
	  return 0;
      }

    if (rl2_polygon_symbolizer_has_graphic_stroke (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer HasGraphicStroke #2\n");
	  *retcode += 11;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_stroke_href (NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetGraphicStrokeHref #2\n");
	  *retcode += 12;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_stroke_recode_count (NULL, &intval)
	== RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetGraphicStrokeRecodeCount #2\n");
	  *retcode += 13;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_stroke_recode_color (NULL, 0,
								&index, &red,
								&green,
								&blue) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetGraphicStrokeRecodeColor #2\n");
	  *retcode += 14;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_stroke_color (NULL, &red, &green, &blue) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetStrokeColor #2\n");
	  *retcode += 15;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_stroke_opacity (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetStrokeOpacity #2\n");
	  *retcode += 16;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_stroke_width (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetStrokeWidth #2\n");
	  *retcode += 17;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_stroke_linejoin (NULL, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetStrokePolygonjoin #2\n");
	  *retcode += 18;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_stroke_linecap (NULL, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetStrokePolygoncap #2\n");
	  *retcode += 19;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_stroke_dash_count (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetStrokeDashCount #2\n");
	  *retcode += 20;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_stroke_dash_item (NULL, 0, &dblval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetStrokeDashItem #2\n");
	  *retcode += 21;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_stroke_dash_offset (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetStrokeDashOffset #2\n");
	  *retcode += 22;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_perpendicular_offset (NULL, &dblval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetPerpendicularOffset #2\n");
	  *retcode += 23;
	  return 0;
      }

    if (rl2_polygon_symbolizer_has_fill (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer HasFill #2\n");
	  *retcode += 24;
	  return 0;
      }

    if (rl2_polygon_symbolizer_has_graphic_fill (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer HasGraphicFill #2\n");
	  *retcode += 25;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_fill_href (NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetGraphicFillHref #2\n");
	  *retcode += 26;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_fill_recode_count (NULL, &intval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetGraphicFillRecodeCount #2\n");
	  *retcode += 27;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_fill_recode_color (NULL, 0,
							      &index, &red,
							      &green,
							      &blue) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetGraphicFillRecodeColor #2\n");
	  *retcode += 28;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_fill_color (NULL, &red, &green, &blue) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetFillColor #2\n");
	  *retcode += 29;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_fill_opacity (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetFillOpacity #2\n");
	  *retcode += 30;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_displacement (NULL, &dblval, &dblval2) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Polygon Symbolizer GetDisplacement #2\n");
	  *retcode += 31;
	  return 0;
      }

    rl2_destroy_feature_type_style (style);
    return 1;
}

static int
test_filter (sqlite3 * db_handle, const char *coverage,
	     const char *style_name, int *retcode)
{
/* testing Filter from a FeatureType Style (PolygonSymbolizers) */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2PolygonSymbolizerPtr polyg;
    rl2VariantArrayPtr value;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int scale_forbidden;
    int intval;
    const char *string;
    style =
	rl2_create_feature_type_style_from_dbms (db_handle, NULL, coverage,
						 style_name);
    if (style == NULL)
      {
	  fprintf (stderr, "Unable to retrieve style '%s'\n", style_name);
	  *retcode += 1;
	  return 0;
      }

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #1\n");
	  *retcode += 2;
	  return 0;
      }
    string = "Emilia Romagna";
    if (rl2_set_variant_text (value, 0, "some_column", string, strlen (string))
	!= RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #1\n");
	  *retcode += 3;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #3\n",
		   style_name);
	  *retcode += 4;
	  return 0;
      }
    polyg = rl2_get_polygon_symbolizer (symbolizer, 0);
    if (polyg == NULL)
      {
	  fprintf (stderr, "Unable to get Polygon Symbolizer #2\n");
	  *retcode += 5;
	  return 0;
      }
    if (rl2_polygon_symbolizer_get_stroke_color (polyg, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Polygon Symbolizer GetStrokeColor #2\n");
	  *retcode += 6;
	  return 0;
      }
    if (red != 0x10 || green != 0xff || blue != 0x20)
      {
	  fprintf (stderr,
		   "Unexpected Polygon Symbolizer GetStrokeColor #2: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 7;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #2\n");
	  *retcode += 8;
	  return 0;
      }
    string = "Lazio";
    if (rl2_set_variant_text (value, 0, "some_column", string, strlen (string))
	!= RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #2\n");
	  *retcode += 9;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #4\n",
		   style_name);
	  *retcode += 10;
	  return 0;
      }
    polyg = rl2_get_polygon_symbolizer (symbolizer, 0);
    if (polyg == NULL)
      {
	  fprintf (stderr, "Unable to get Polygon Symbolizer #3\n");
	  *retcode += 11;
	  return 0;
      }
    if (rl2_polygon_symbolizer_get_fill_color (polyg, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Polygon Symbolizer GetFillColor #2\n");
	  *retcode += 12;
	  return 0;
      }
    if (red != 0xff || green != 0xdd || blue != 0xaa)
      {
	  fprintf (stderr,
		   "Unexpected Polygon Symbolizer GetStrokeColor #3: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 13;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #3\n");
	  *retcode += 14;
	  return 0;
      }
    string = "Abruzzo";
    if (rl2_set_variant_text (value, 0, "some_column", string, strlen (string))
	!= RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #3\n");
	  *retcode += 15;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #5\n",
		   style_name);
	  *retcode += 16;
	  return 0;
      }
    polyg = rl2_get_polygon_symbolizer (symbolizer, 0);
    if (polyg == NULL)
      {
	  fprintf (stderr, "Unable to get Polygon Symbolizer #4\n");
	  *retcode += 17;
	  return 0;
      }
    if (rl2_polygon_symbolizer_get_fill_color (polyg, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Polygon Symbolizer GetFillColor #3\n");
	  *retcode += 18;
	  return 0;
      }
    if (red != 0x81 || green != 0xfa || blue != 0x91)
      {
	  fprintf (stderr,
		   "Unexpected Polygon Symbolizer GetStrokeColor #4: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 19;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #4\n");
	  *retcode += 20;
	  return 0;
      }
    string = "Campania";
    if (rl2_set_variant_text (value, 0, "some_column", string, strlen (string))
	!= RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #4\n");
	  *retcode += 21;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #6\n",
		   style_name);
	  *retcode += 22;
	  return 0;
      }
    polyg = rl2_get_polygon_symbolizer (symbolizer, 1);
    if (polyg == NULL)
      {
	  fprintf (stderr, "Unable to get Polygon Symbolizer #5\n");
	  *retcode += 23;
	  return 0;
      }
    if (rl2_polygon_symbolizer_get_fill_color (polyg, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Polygon Symbolizer GetFillColor #4\n");
	  *retcode += 24;
	  return 0;
      }
    if (red != 0x37 || green != 0x81 || blue != 0xf2)
      {
	  fprintf (stderr,
		   "Unexpected Polygon Symbolizer GetStrokeColor #5: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 25;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #5\n");
	  *retcode += 26;
	  return 0;
      }
    string = "Trentino Alto Adige";
    if (rl2_set_variant_text (value, 0, "some_column", string, strlen (string))
	!= RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #5\n");
	  *retcode += 27;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #7\n",
		   style_name);
	  *retcode += 29;
	  return 0;
      }
    polyg = rl2_get_polygon_symbolizer (symbolizer, 0);
    if (polyg == NULL)
      {
	  fprintf (stderr, "Unable to get Polygon Symbolizer #6\n");
	  *retcode += 30;
	  return 0;
      }
    if (rl2_polygon_symbolizer_get_fill_color (polyg, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Polygon Symbolizer GetFillColor #5\n");
	  *retcode += 31;
	  return 0;
      }
    if (red != 0x83 || green != 0xfc || blue != 0x93)
      {
	  fprintf (stderr,
		   "Unexpected Polygon Symbolizer GetStrokeColor #6: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 32;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #6\n");
	  *retcode += 26;
	  return 0;
      }
    string = "pseudo-Toscanella";
    if (rl2_set_variant_text (value, 0, "name", string, strlen (string)) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #6\n");
	  *retcode += 27;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #8\n",
		   style_name);
	  *retcode += 28;
	  return 0;
      }
    polyg = rl2_get_polygon_symbolizer (symbolizer, 0);
    if (polyg == NULL)
      {
	  fprintf (stderr, "Unable to get Polygon Symbolizer #7\n");
	  *retcode += 29;
	  return 0;
      }
    if (rl2_polygon_symbolizer_get_fill_color (polyg, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Polygon Symbolizer GetFillColor #6\n");
	  *retcode += 30;
	  return 0;
      }
    if (red != 0xab || green != 0xcd || blue != 0xef)
      {
	  fprintf (stderr,
		   "Unexpected Polygon Symbolizer GetStrokeColor #7: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 32;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #7\n");
	  *retcode += 33;
	  return 0;
      }
    string = "Toscanona";
    if (rl2_set_variant_text (value, 0, "name", string, strlen (string)) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #7\n");
	  *retcode += 34;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #9\n",
		   style_name);
	  *retcode += 35;
	  return 0;
      }
    polyg = rl2_get_polygon_symbolizer (symbolizer, 0);
    if (polyg == NULL)
      {
	  fprintf (stderr, "Unable to get Polygon Symbolizer #8\n");
	  *retcode += 36;
	  return 0;
      }
    if (rl2_polygon_symbolizer_get_fill_color (polyg, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Polygon Symbolizer GetFillColor #7\n");
	  *retcode += 37;
	  return 0;
      }
    if (red != 0xab || green != 0xcd || blue != 0xef)
      {
	  fprintf (stderr,
		   "Unexpected Polygon Symbolizer GetStrokeColor #8: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 38;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    intval = rl2_get_feature_type_style_columns_count (style);
    if (intval != 2)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnsCount #1: %d\n",
		   intval);
	  *retcode += 39;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 0);
    if (strcasecmp (string, "name") != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #1: \"%s\"\n",
		   string);
	  *retcode += 40;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 1);
    if (strcasecmp (string, "some_column") != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #2: \"%s\"\n",
		   string);
	  *retcode += 41;
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
    if (!test_symbolizer (db_handle, "polyg1", "polygon_1", &ret))
	return ret;
    ret = -200;
    if (!test_symbolizer (db_handle, "polyg2", "polygon_2", &ret))
	return ret;
    ret = -300;
    if (!test_symbolizer (db_handle, "polyg2", "polygon_3", &ret))
	return ret;
    ret = -400;
    if (!test_style (db_handle, "polyg1", "polygon_style", &ret))
	return ret;
    ret = -500;
    if (!test_filter (db_handle, "polyg1", "polygon_style", &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    return result;
}
