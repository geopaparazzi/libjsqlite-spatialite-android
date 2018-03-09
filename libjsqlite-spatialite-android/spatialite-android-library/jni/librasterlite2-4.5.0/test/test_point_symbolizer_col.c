/*

 test_point_symbolizer_col.c -- RasterLite-2 Test Case

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
/* testing a PointSymbolizer */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2PointSymbolizerPtr point;
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
    if (intval != RL2_POINT_SYMBOLIZER)
      {
	  fprintf (stderr,
		   "%s: Unexpected Vector Symbolizer Item Type #1: %d\n",
		   style_name, intval);
	  *retcode += 8;
	  return 0;
      }

    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "%s: Unable to get a Point Symbolizer #1\n",
		   style_name);
	  *retcode += 9;
	  return 0;
      }

    if (rl2_point_symbolizer_get_count (point, &intval) != RL2_OK)
      {
	  fprintf (stderr, "%s: Unable to get Point Symbolizer Count #1\n",
		   style_name);
	  *retcode += 10;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "%s: Unexpected Point Symbolizer Count #1: %d\n",
		   style_name, intval);
	  *retcode += 11;
	  return 0;
      }

    if (rl2_point_symbolizer_is_graphic (point, 0, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer IsGraphic #1\n",
		   style_name);
	  *retcode += 12;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "point_1_col") == 0
	|| strcmp (style_name, "point_2_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer IsGraphic #1: %d\n",
		   style_name, intval);
	  *retcode += 13;
	  return 0;
      }

    string = rl2_point_symbolizer_get_col_graphic_href (point, 0);
    intval = 0;
    if (strcmp (style_name, "point_1_col") == 0
	|| strcmp (style_name, "point_2_col") == 0)
      {
	  if (string == NULL)
	      intval = 1;
      }
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (string != NULL)
	    {
		if (strcmp (string, "symbol_href") == 0)
		    intval = 1;
	    }
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Point Symbolizer GetColGraphicHref #1: %s\n",
		   style_name, string);
	  *retcode += 14;
	  return 0;
      }

    if (rl2_point_symbolizer_is_mark (point, 0, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer IsMark #1\n",
		   style_name);
	  *retcode += 15;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "point_1_col") == 0
	|| strcmp (style_name, "point_2_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer IsMark #1: %d\n",
		   style_name, intval);
	  *retcode += 16;
	  return 0;
      }

    ret = rl2_point_symbolizer_mark_has_stroke (point, 0, &intval);
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark HasStroke #1\n",
			 style_name);
		*retcode += 17;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark HasStroke #1\n",
			 style_name);
		*retcode += 18;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark HasStroke #1: %d\n",
		   style_name, intval);
	  *retcode += 19;
	  return 0;
      }

    string = rl2_point_symbolizer_mark_get_col_stroke_color (point, 0);
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetColStrokeColor #1\n",
			 style_name);
		*retcode += 20;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark GetColStrokeColor #1\n",
			 style_name);
		*retcode += 21;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1_col") == 0
	&& strcmp (style_name, "point_2_col") == 0)
      {
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetColStrokeColor #1: %s\n",
		   style_name, string);
	  *retcode += 22;
	  return 0;
      }

    string = rl2_point_symbolizer_mark_get_col_stroke_width (point, 0);
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetStrokeColWidth #1\n",
			 style_name);
		*retcode += 23;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark GetStrokeColWidth #1\n",
			 style_name);
		*retcode += 24;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (strcmp (string, "width") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeColWidth #1: %s\n",
		   style_name, string);
	  *retcode += 25;
	  return 0;
      }

    ret = rl2_point_symbolizer_mark_get_stroke_linejoin (point, 0, &red);
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetStrokeLinejoin #1\n",
			 style_name);
		*retcode += 26;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark GetStrokeLinejoin #1\n",
			 style_name);
		*retcode += 27;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_UNKNOWN)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeLinejoin #1: %02x\n",
		   style_name, red);
	  *retcode += 28;
	  return 0;
      }

    ret = rl2_point_symbolizer_mark_get_stroke_linecap (point, 0, &red);
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetStrokeLinecap #1\n",
			 style_name);
		*retcode += 29;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark GetStrokeLinecap #1\n",
			 style_name);
		*retcode += 30;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_UNKNOWN)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeLinecap #1: %02x\n",
		   style_name, red);
	  *retcode += 31;
	  return 0;
      }

    ret = rl2_point_symbolizer_mark_get_stroke_dash_count (point, 0, &intval);
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetStrokeDashCount #1\n",
			 style_name);
		*retcode += 32;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer GetStrokeDashCount #1\n",
			 style_name);
		*retcode += 33;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    else
	red = 1;
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeDashCount #1: %d\n",
		   style_name, intval);
	  *retcode += 34;
	  return 0;
      }


    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (rl2_point_symbolizer_mark_get_stroke_dash_item
	      (point, 0, 0, &dblval) == RL2_OK)
	    {
		fprintf (stderr,
			 "Expected failure, got success: Point Symbolizer Mark GetStrokeDashItem #1\n");
		*retcode += 35;
		return 0;
	    }
      }
    else
      {
	  if (rl2_point_symbolizer_mark_get_stroke_dash_item
	      (point, 0, 0, &dblval) != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark GetStrokeDashItem #1\n",
			 style_name);
		*retcode += 36;
		return 0;
	    }
	  if (dblval != 20.0)
	    {
		fprintf (stderr,
			 "Unexpected Point Symbolizer Mark GetStrokDashItem #1: %1.4f\n",
			 dblval);
		*retcode += 37;
		return 0;
	    }
	  if (rl2_point_symbolizer_mark_get_stroke_dash_item
	      (point, 0, 1, &dblval) != RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Point Symbolizer Mark GetStrokeDashItem #2\n");
		*retcode += 38;
		return 0;
	    }
	  if (dblval != 10)
	    {
		fprintf (stderr,
			 "Unexpected Point Symbolizer Mark GetStrokDashItem #2: %1.4f\n",
			 dblval);
		*retcode += 39;
		return 0;
	    }
      }

    ret = rl2_point_symbolizer_mark_get_stroke_dash_offset (point, 0, &dblval);
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetStrokeDashOffset #1\n",
			 style_name);
		*retcode += 40;
		return 0;
	    }
	  dblval = 0.0;
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark GetStrokeDashOffset #1\n",
			 style_name);
		*retcode += 41;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (dblval == 0.0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeDashOffset #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 42;
	  return 0;
      }

    ret = rl2_point_symbolizer_mark_has_fill (point, 0, &intval);
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark HasFill #1\n",
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
			 "%s: Unable to get Point Symbolizer Mark HasFill #1\n",
			 style_name);
		*retcode += 44;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "point_1_col") == 0
	|| strcmp (style_name, "point_2_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    else
	red = 1;
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark HasFill #1: %d\n",
		   style_name, intval);
	  *retcode += 45;
	  return 0;
      }

    string = rl2_point_symbolizer_mark_get_col_fill_color (point, 0);
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetColFillColor #1\n",
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
			 "%s: Unable to get Point Symbolizer Mark GetColFillColor #1\n",
			 style_name);
		*retcode += 47;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1_col") == 0
	|| strcmp (style_name, "point_2_col") == 0)
      {
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetColFillColor #1: %s\n",
		   style_name, string);
	  *retcode += 48;
	  return 0;
      }

    string = rl2_point_symbolizer_get_col_opacity (point);
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Point Symbolizer GetColOpacity #1\n",
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
			 "%s: Unable to get Point Symbolizer GetColOpacity #1\n",
			 style_name);
		*retcode += 49;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (strcmp (string, "opacity") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetColOpacity #1: %s\n",
		   style_name, string);
	  *retcode += 50;
	  return 0;
      }

    string = rl2_point_symbolizer_get_col_size (point);
    if (string == NULL)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer GetColSize #1\n",
		   style_name);
	  *retcode += 51;
	  return 0;
      }
    intval = 0;
    if (strcmp (string, "size") == 0)
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetColSize #1: %sn",
		   style_name, string);
	  *retcode += 52;
	  return 0;
      }

    string = rl2_point_symbolizer_get_col_rotation (point);
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Point Symbolizer GetColRotation #1\n",
			 style_name);
		*retcode += 53;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer GetColRotation #1\n",
			 style_name);
		*retcode += 53;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (strcmp (string, "rotation") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetColRotation #1: %s\n",
		   style_name, string);
	  *retcode += 54;
	  return 0;
      }

    string = rl2_point_symbolizer_get_col_anchor_point_x (point);
    if (strcmp (style_name, "point_1_col") == 0
	|| strcmp (style_name, "point_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Point Symbolizer GetColAnchorPoint X #1\n",
			 style_name);
		*retcode += 55;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer GetColAnchorPoint X #1\n",
			 style_name);
		*retcode += 55;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (strcmp (string, "point_x") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetColAnchorPoint X #1: %s\n",
		   style_name, string);
	  *retcode += 56;
	  return 0;
      }

    string = rl2_point_symbolizer_get_col_anchor_point_y (point);
    if (strcmp (style_name, "point_1_col") == 0
	|| strcmp (style_name, "point_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Point Symbolizer GetColAnchorPoint Y #1\n",
			 style_name);
		*retcode += 57;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer GetColAnchorPoint Y #1\n",
			 style_name);
		*retcode += 58;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (strcmp (string, "point_y") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetColAnchorPoint Y #1: %s\n",
		   style_name, string);
	  *retcode += 59;
	  return 0;
      }

    string = rl2_point_symbolizer_get_col_displacement_x (point);
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Point Symbolizer GetColDisplacement X #1\n",
			 style_name);
		*retcode += 60;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer GetColDisplacement X #1\n",
			 style_name);
		*retcode += 60;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (strcmp (string, "displ_x") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetColDisplacement X #1: %s\n",
		   style_name, string);
	  *retcode += 61;
	  return 0;
      }

    string = rl2_point_symbolizer_get_col_displacement_y (point);
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Point Symbolizer GetColDisplacement Y #1\n",
			 style_name);
		*retcode += 62;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer GetColDisplacement Y #1\n",
			 style_name);
		*retcode += 63;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (strcmp (string, "displ_y") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetColDisplacement Y #1: %s\n",
		   style_name, string);
	  *retcode += 64;
	  return 0;
      }

    string =
	rl2_point_symbolizer_get_col_graphic_recode_color (point, 0, 0, &index);
    if (strcmp (style_name, "point_1_col") == 0
	|| strcmp (style_name, "point_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer GetColGraphicRecodeColor #1\n",
			 style_name);
		*retcode += 65;
		return 0;
	    }
	  intval = 1;
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer GetColGraphicRecodeColor #1\n",
			 style_name);
		*retcode += 63;
		return 0;
	    }
	  intval = 0;
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetColGraphicRecodeColor #1: %s\n",
		   style_name, string);
	  *retcode += 66;
	  return 0;
      }

    intval = rl2_get_feature_type_style_columns_count (style);
    red = 0;
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (intval == 4)
	      red = 1;
      }
    if (strcmp (style_name, "point_2_col") == 0)
      {
	  if (intval == 7)
	      red = 1;
      }
    if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (intval == 9)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer StyleColumnsCount #1: %d\n",
		   style_name, intval);
	  *retcode += 67;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 0);
    if (string == NULL)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer StyleColumnName #1\n",
		   style_name);
	  *retcode += 68;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "point_2_col") == 0
	|| strcmp (style_name, "point_3_col") == 0)
      {
	  if (strcmp (string, "opacity") == 0)
	      intval = 1;
      }
    else
      {
	  if (strcmp (string, "size") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer StyleColumnsName #1: %s\n",
		   style_name, string);
	  *retcode += 69;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 5);
    if (strcmp (style_name, "point_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Point Symbolizer StyleColumnName #2\n",
			 style_name);
		*retcode += 70;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer StyleColumnName #2\n",
			 style_name);
		*retcode += 71;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_2_col") == 0)
      {
	  if (strcmp (string, "mark") == 0)
	      intval = 1;
      }
    else if (strcmp (style_name, "point_3_col") == 0)
      {
	  if (strcmp (string, "displ_x") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer StyleColumnsName #2: %s\n",
		   style_name, string);
	  *retcode += 72;
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
    if (!test_symbolizer (db_handle, "point1", "point_1_col", &ret))
	return ret;
    ret = -200;
    if (!test_symbolizer (db_handle, "point2", "point_2_col", &ret))
	return ret;
    ret = -300;
    if (!test_symbolizer (db_handle, "point2", "point_3_col", &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    return result;
}
