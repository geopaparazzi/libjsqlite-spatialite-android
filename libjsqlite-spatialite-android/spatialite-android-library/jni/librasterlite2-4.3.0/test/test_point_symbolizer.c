/*

 test_point_symbolizer.c -- RasterLite-2 Test Case

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
    double dblval2;
    const char *string;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int scale_forbidden;
    style =
	rl2_create_feature_type_style_from_dbms (db_handle, coverage,
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
    if (strcmp (style_name, "point_1") == 0
	|| strcmp (style_name, "point_2") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "point_3") == 0)
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

    string = rl2_point_symbolizer_get_graphic_href (point, 0);
    intval = 0;
    if (strcmp (style_name, "point_1") == 0
	|| strcmp (style_name, "point_2") == 0)
      {
	  if (string == NULL)
	      intval = 1;
      }
    if (strcmp (style_name, "point_3") == 0)
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
		   "%s; Unexpected Point Symbolizer GetGraphicHref #1: %s\n",
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
    if (strcmp (style_name, "point_1") == 0
	|| strcmp (style_name, "point_2") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "point_3") == 0)
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
    if (strcmp (style_name, "point_3") == 0)
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
    if (strcmp (style_name, "point_1") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "point_2") == 0
	|| strcmp (style_name, "point_3") == 0)
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

    ret =
	rl2_point_symbolizer_mark_get_stroke_color (point, 0, &red, &green,
						    &blue);
    if (strcmp (style_name, "point_2") == 0
	|| strcmp (style_name, "point_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetStrokeColor #1\n",
			 style_name);
		*retcode += 20;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark GetStrokeColor #1\n",
			 style_name);
		*retcode += 21;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1") == 0)
      {
	  if (red == 0xff && green == 0xff && blue == 0x80)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 22;
	  return 0;
      }

    ret = rl2_point_symbolizer_mark_get_stroke_width (point, 0, &dblval);
    if (strcmp (style_name, "point_2") == 0
	|| strcmp (style_name, "point_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetStrokeWidth #1\n",
			 style_name);
		*retcode += 23;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer Mark GetStrokeWidth #1\n",
			 style_name);
		*retcode += 24;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1") == 0)
      {
	  if (dblval == 2.0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeWidth #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 25;
	  return 0;
      }

    ret = rl2_point_symbolizer_mark_get_stroke_linejoin (point, 0, &red);
    if (strcmp (style_name, "point_2") == 0
	|| strcmp (style_name, "point_3") == 0)
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
    if (strcmp (style_name, "point_1") == 0)
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
    if (strcmp (style_name, "point_2") == 0
	|| strcmp (style_name, "point_3") == 0)
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
    if (strcmp (style_name, "point_1") == 0)
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
    if (strcmp (style_name, "point_2") == 0
	|| strcmp (style_name, "point_3") == 0)
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
    if (strcmp (style_name, "point_1") == 0)
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


    if (strcmp (style_name, "point_2") == 0
	|| strcmp (style_name, "point_3") == 0)
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
    if (strcmp (style_name, "point_2") == 0
	|| strcmp (style_name, "point_3") == 0)
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
    if (strcmp (style_name, "point_1") == 0)
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
    if (strcmp (style_name, "point_3") == 0)
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
    if (strcmp (style_name, "point_1") == 0
	|| strcmp (style_name, "point_2") == 0)
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

    ret =
	rl2_point_symbolizer_mark_get_fill_color (point, 0, &red, &green,
						  &blue);
    if (strcmp (style_name, "point_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer Mark GetFillColor #1\n",
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
			 "%s: Unable to get Point Symbolizer Mark GetFillColor #1\n",
			 style_name);
		*retcode += 47;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "point_1") == 0)
      {
	  if (red == 0x78 && green == 0x90 && blue == 0xcd)
	      intval = 1;
      }
    else if (strcmp (style_name, "point_2") == 0)
      {
	  if (red == 0x62 && green == 0x14 && blue == 0x93)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetFillColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 48;
	  return 0;
      }

    if (rl2_point_symbolizer_get_opacity (point, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer GetOpacity #1\n",
		   style_name);
	  *retcode += 49;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "point_1") == 0)
      {
	  if (dblval == 1.0)
	      intval = 1;
      }
    if (strcmp (style_name, "point_2") == 0)
      {
	  if (dblval == 0.5)
	      intval = 1;
      }
    if (strcmp (style_name, "point_3") == 0)
      {
	  if (dblval == 0.25)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetOpacity #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 50;
	  return 0;
      }

    if (rl2_point_symbolizer_get_size (point, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer GetSize #1\n",
		   style_name);
	  *retcode += 51;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "point_1") == 0)
      {
	  if (dblval == 48.0)
	      intval = 1;
      }
    if (strcmp (style_name, "point_2") == 0)
      {
	  if (dblval == 36.0)
	      intval = 1;
      }
    if (strcmp (style_name, "point_3") == 0)
      {
	  if (dblval == 16.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetSize #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 52;
	  return 0;
      }

    if (rl2_point_symbolizer_get_rotation (point, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer GetRotation #1\n",
		   style_name);
	  *retcode += 53;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "point_1") == 0)
      {
	  if (dblval == 0.0)
	      intval = 1;
      }
    if (strcmp (style_name, "point_2") == 0)
      {
	  if (dblval == 45.0)
	      intval = 1;
      }
    if (strcmp (style_name, "point_3") == 0)
      {
	  if (dblval == 60.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetRotation #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 54;
	  return 0;
      }

    if (rl2_point_symbolizer_get_anchor_point (point, &dblval, &dblval2) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer GetAnchorPoint #1\n",
		   style_name);
	  *retcode += 55;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "point_3") == 0)
      {
	  if (dblval == 3.0 && dblval2 == 2.0)
	      intval = 1;
      }
    else
      {
	  if (dblval == 0.5 && dblval2 == 0.5)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetAnchorPoint #1: %1.4f %1.4f\n",
		   style_name, dblval, dblval2);
	  *retcode += 56;
	  return 0;
      }

    if (rl2_point_symbolizer_get_displacement (point, &dblval, &dblval2) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer GetDisplacement #1\n",
		   style_name);
	  *retcode += 57;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "point_2") == 0)
      {
	  if (dblval == 2.0 && dblval2 == -6.0)
	      intval = 1;
      }
    else if (strcmp (style_name, "point_3") == 0)
      {
	  if (dblval == -2.5 && dblval2 == 6.5)
	      intval = 1;
      }
    else
      {
	  if (dblval == 0.0 && dblval2 == 0.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetDisplacement #1: %1.4f %1.4f\n",
		   style_name, dblval, dblval2);
	  *retcode += 58;
	  return 0;
      }

    ret = rl2_point_symbolizer_get_graphic_recode_color (point, 0, 0, &index,
							 &red, &green, &blue);
    if (strcmp (style_name, "point_1") == 0
	|| strcmp (style_name, "point_2") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Point Symbolizer GetGraphicRecodeColor #1\n",
			 style_name);
		*retcode += 59;
		return 0;
	    }
	  intval = 1;
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Point Symbolizer GetGraphicRecodeColor #1\n",
			 style_name);
		*retcode += 60;
		return 0;
	    }
	  intval = 0;
	  if (index == 0 && red == 0x98 && green == 0x76 && blue == 0x54)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer GetGraphicRecodeColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 61;
	  return 0;
      }

    rl2_destroy_feature_type_style (style);
    return 1;
}

static int
test_style (sqlite3 * db_handle, const char *coverage,
	    const char *style_name, int *retcode)
{
/* testing a FeatureType Style (PointSymbolizers) */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2PointSymbolizerPtr point;
    int intval;
    double dblval;
    double dblval2;
    const char *string;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int scale_forbidden;
    style =
	rl2_create_feature_type_style_from_dbms (db_handle, coverage,
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
    if (strcmp (string, "point_style") != 0)
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
    if (intval != RL2_POINT_SYMBOLIZER)
      {
	  fprintf (stderr,
		   "%s: Unexpected Vector Symbolizer Item Type #2: %d\n",
		   style_name, intval);
	  *retcode += 8;
	  return 0;
      }

    point = rl2_get_point_symbolizer (symbolizer, 1);
    if (point == NULL)
      {
	  fprintf (stderr, "%s: Unable to get a Point Symbolizer #2\n",
		   style_name);
	  *retcode += 9;
	  return 0;
      }

    if (rl2_point_symbolizer_is_mark (point, 1, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer IsMark #2\n",
		   style_name);
	  *retcode += 10;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer IsMark #2: %d\n",
		   style_name, intval);
	  *retcode += 11;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_has_stroke (point, 1, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark HasStroke #21\n",
		   style_name);
	  *retcode += 12;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark HasStroke #2: %d\n",
		   style_name, intval);
	  *retcode += 13;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_color (point, 1, &red, &green,
						    &blue) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark GetStrokeColor #2\n",
		   style_name);
	  *retcode += 14;
	  return 0;
      }
    if (red != 0xff || green != 0xff || blue != 0x80)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeColor #2: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 15;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_width (point, 1, &dblval) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark GetStrokeWidth #2\n",
		   style_name);
	  *retcode += 16;
	  return 0;
      }
    if (dblval != 1.0)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeWidth #2: %1.4f\n",
		   style_name, dblval);
	  *retcode += 17;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_linejoin (point, 1, &red) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark GetStrokeLinejoin #2\n",
		   style_name);
	  *retcode += 18;
	  return 0;
      }
    if (red != RL2_STROKE_LINEJOIN_MITRE)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeLinejoin #2: %02x\n",
		   style_name, red);
	  *retcode += 19;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_linecap (point, 1, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark GetStrokeLinecap #2\n",
		   style_name);
	  *retcode += 20;
	  return 0;
      }
    if (red != RL2_STROKE_LINECAP_ROUND)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeLinecap #2: %02x\n",
		   style_name, red);
	  *retcode += 21;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_dash_count (point, 1, &intval) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer GetStrokeDashCount #2\n",
		   style_name);
	  *retcode += 22;
	  return 0;
      }
    if (intval != 2)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeDashCount #2: %d\n",
		   style_name, intval);
	  *retcode += 23;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_dash_item
	(point, 1, 1, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark GetStrokeDashItem #2\n",
		   style_name);
	  *retcode += 24;
	  return 0;
      }
    if (dblval != 6.0)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetStrokDashItem #2: %1.4f\n",
		   dblval);
	  *retcode += 25;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_dash_offset (point, 1, &dblval) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark GetStrokeDashOffset #2\n",
		   style_name);
	  *retcode += 26;
	  return 0;
      }
    if (dblval != 2.5)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetStrokeDashOffset #2: %1.4f\n",
		   style_name, dblval);
	  *retcode += 27;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_has_fill (point, 1, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark HasFill #2\n",
		   style_name);
	  *retcode += 28;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark HasFill #2: %d\n",
		   style_name, intval);
	  *retcode += 29;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_fill_color (point, 1, &red, &green,
						  &blue) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Point Symbolizer Mark GetFillColor #2\n",
		   style_name);
	  *retcode += 30;
	  return 0;
      }
    if (red != 0x78 || green != 0x90 || blue != 0xcd)
      {
	  fprintf (stderr,
		   "%s: Unexpected Point Symbolizer Mark GetFillColor #2: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 31;
	  return 0;
      }

    if (rl2_get_line_symbolizer (symbolizer, 1) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Line Symbolizer #2\n");
	  *retcode += 32;
	  return 0;
      }

    if (rl2_get_polygon_symbolizer (symbolizer, 1) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Polygon Symbolizer #2\n");
	  *retcode += 33;
	  return 0;
      }

    if (rl2_get_text_symbolizer (symbolizer, 1) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Text Symbolizer #2\n");
	  *retcode += 34;
	  return 0;
      }

    if (rl2_get_point_symbolizer (symbolizer, 10) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Point Symbolizer #2\n");
	  *retcode += 35;
	  return 0;
      }

    if (rl2_get_line_symbolizer (symbolizer, 10) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Line Symbolizer #3\n");
	  *retcode += 36;
	  return 0;
      }

    if (rl2_get_polygon_symbolizer (symbolizer, 10) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Polygon Symbolizer #3\n");
	  *retcode += 37;
	  return 0;
      }

    if (rl2_get_text_symbolizer (symbolizer, 10) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Text Symbolizer #3\n");
	  *retcode += 38;
	  return 0;
      }

    if (rl2_get_point_symbolizer (NULL, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Point Symbolizer #3\n");
	  *retcode += 39;
	  return 0;
      }

    if (rl2_get_line_symbolizer (NULL, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Line Symbolizer #4\n");
	  *retcode += 40;
	  return 0;
      }

    if (rl2_get_polygon_symbolizer (NULL, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Polygon Symbolizer #4\n");
	  *retcode += 41;
	  return 0;
      }

    if (rl2_get_text_symbolizer (NULL, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Text Symbolizer #4\n");
	  *retcode += 42;
	  return 0;
      }

    if (rl2_point_symbolizer_get_count (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer GetCount #2\n");
	  *retcode += 43;
	  return 0;
      }

    if (rl2_point_symbolizer_is_graphic (point, 10, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer IsGraphic #2\n");
	  *retcode += 44;
	  return 0;
      }

    if (rl2_point_symbolizer_is_graphic (NULL, 0, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer IsGraphic #3\n");
	  *retcode += 45;
	  return 0;
      }

    if (rl2_point_symbolizer_get_graphic_href (NULL, 0) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer GetGraphicHref #2\n");
	  *retcode += 46;
	  return 0;
      }

    if (rl2_point_symbolizer_get_graphic_recode_color
	(NULL, 0, 0, &intval, &red, &green, &blue) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer GetGraphicRecodeColor #2\n");
	  *retcode += 47;
	  return 0;
      }

    if (rl2_point_symbolizer_is_mark (point, 10, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: Point Symbolizer IsMark #2\n");
	  *retcode += 48;
	  return 0;
      }

    if (rl2_point_symbolizer_is_mark (NULL, 0, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: Point Symbolizer IsMark #3\n");
	  *retcode += 49;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_has_stroke (NULL, 0, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkHasStroke #2\n");
	  *retcode += 40;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_color
	(NULL, 0, &red, &green, &blue) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkGetStrokeColor #2\n");
	  *retcode += 41;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_width (NULL, 0, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkGetStrokeWidth #2\n");
	  *retcode += 42;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_linejoin (NULL, 0, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkGetStrokeLinejoin #2\n");
	  *retcode += 43;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_linecap (NULL, 0, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkGetStrokeLinecap #2\n");
	  *retcode += 44;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_dash_count (NULL, 0, &intval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkGetStrokeDashCount #2\n");
	  *retcode += 45;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_dash_item (NULL, 0, 0, &dblval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkGetStrokeDashItem #2\n");
	  *retcode += 46;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_stroke_dash_offset (NULL, 0, &dblval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkGetStrokeDashOffset #2\n");
	  *retcode += 47;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_has_fill (NULL, 0, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkHasFill #2\n");
	  *retcode += 48;
	  return 0;
      }

    if (rl2_point_symbolizer_mark_get_fill_color (NULL, 0, &red, &green, &blue)
	== RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer MarkGetFillColor #2\n");
	  *retcode += 49;
	  return 0;
      }

    if (rl2_point_symbolizer_get_opacity (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer GetOpacity #2\n");
	  *retcode += 50;
	  return 0;
      }

    if (rl2_point_symbolizer_get_size (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: Point Symbolizer GetSize #2\n");
	  *retcode += 51;
	  return 0;
      }

    if (rl2_point_symbolizer_get_rotation (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer GetRotation #2\n");
	  *retcode += 52;
	  return 0;
      }

    if (rl2_point_symbolizer_get_anchor_point (NULL, &dblval, &dblval2) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer GetAnchorPoint #2\n");
	  *retcode += 53;
	  return 0;
      }

    if (rl2_point_symbolizer_get_displacement (NULL, &dblval, &dblval2) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Point Symbolizer GetDisplacement #2\n");
	  *retcode += 54;
	  return 0;
      }

    if (rl2_get_feature_type_style_name (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected success: GetFeatureTypeStyleName #2\n");
	  *retcode += 55;
	  return 0;
      }

    rl2_destroy_feature_type_style (style);
    return 1;
}

static int
test_filter (sqlite3 * db_handle, const char *coverage,
	     const char *style_name, int *retcode)
{
/* testing Filter from a FeatureType Style (PointSymbolizers) */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2PointSymbolizerPtr point;
    rl2VariantArrayPtr value;
    int intval;
    int scale_forbidden;
    const char *string;
    unsigned char type;
    unsigned char blob[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    style =
	rl2_create_feature_type_style_from_dbms (db_handle, coverage,
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
    if (rl2_set_variant_int (value, 0, "some_column", 4) != RL2_OK)
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
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #2\n");
	  *retcode += 5;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #1\n");
	  *retcode += 6;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_TRIANGLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #1: %02x\n",
		   type);
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
    if (rl2_set_variant_double (value, 0, "some_column", 14.6) != RL2_OK)
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
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #3\n");
	  *retcode += 11;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #2\n");
	  *retcode += 12;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_TRIANGLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #2: %02x\n",
		   type);
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
    if (rl2_set_variant_null (value, 0, "some_column") != RL2_OK)
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
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #4\n");
	  *retcode += 17;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #3\n");
	  *retcode += 18;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_SQUARE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #3: %02x\n",
		   type);
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
    if (rl2_set_variant_blob (value, 0, "some_column", blob, 8) != RL2_OK)
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
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #5\n");
	  *retcode += 23;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #4\n");
	  *retcode += 24;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_X)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #4: %02x\n",
		   type);
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
    if (rl2_set_variant_int (value, 0, "some_column", 5) != RL2_OK)
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
	  *retcode += 28;
	  return 0;
      }
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #6\n");
	  *retcode += 29;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #5\n");
	  *retcode += 30;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_TRIANGLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #5: %02x\n",
		   type);
	  *retcode += 31;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #6\n");
	  *retcode += 32;
	  return 0;
      }
    if (rl2_set_variant_int (value, 0, "some_column", 54321) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #6\n");
	  *retcode += 33;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #8\n",
		   style_name);
	  *retcode += 34;
	  return 0;
      }
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #7\n");
	  *retcode += 35;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #6\n");
	  *retcode += 36;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_CIRCLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #6: %02x\n",
		   type);
	  *retcode += 37;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #7\n");
	  *retcode += 38;
	  return 0;
      }
    if (rl2_set_variant_double (value, 0, "some_column", 54321.0) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #7\n");
	  *retcode += 39;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #9\n",
		   style_name);
	  *retcode += 40;
	  return 0;
      }
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #8\n");
	  *retcode += 41;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #7\n");
	  *retcode += 42;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_CIRCLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #7: %02x\n",
		   type);
	  *retcode += 43;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #8\n");
	  *retcode += 44;
	  return 0;
      }
    if (rl2_set_variant_int (value, 0, "some_column", 14) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #8\n");
	  *retcode += 45;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #10\n",
		   style_name);
	  *retcode += 46;
	  return 0;
      }
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #9\n");
	  *retcode += 47;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #8\n");
	  *retcode += 48;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_TRIANGLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #8: %02x\n",
		   type);
	  *retcode += 49;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #9\n");
	  *retcode += 50;
	  return 0;
      }
    if (rl2_set_variant_int (value, 0, "some_column", 500) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #9\n");
	  *retcode += 51;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #11\n",
		   style_name);
	  *retcode += 52;
	  return 0;
      }
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #10\n");
	  *retcode += 53;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #9\n");
	  *retcode += 54;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_TRIANGLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #9: %02x\n",
		   type);
	  *retcode += 55;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #10\n");
	  *retcode += 56;
	  return 0;
      }
    if (rl2_set_variant_double (value, 0, "some_column", 500.1) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #10\n");
	  *retcode += 57;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #12\n",
		   style_name);
	  *retcode += 58;
	  return 0;
      }
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #11\n");
	  *retcode += 59;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #10\n");
	  *retcode += 60;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_TRIANGLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #10: %02x\n",
		   type);
	  *retcode += 61;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #11\n");
	  *retcode += 62;
	  return 0;
      }
    string = "TOSCANA";
    if (rl2_set_variant_text (value, 0, "name", string, strlen (string)) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #11\n");
	  *retcode += 63;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #13\n",
		   style_name);
	  *retcode += 64;
	  return 0;
      }
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #12\n");
	  *retcode += 65;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #11\n");
	  *retcode += 66;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_CIRCLE)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #11: %02x\n",
		   type);
	  *retcode += 67;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #12\n");
	  *retcode += 68;
	  return 0;
      }
    string = "Lazio";
    if (rl2_set_variant_text (value, 0, "name", string, strlen (string)) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #12\n");
	  *retcode += 69;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #14\n",
		   style_name);
	  *retcode += 70;
	  return 0;
      }
    point = rl2_get_point_symbolizer (symbolizer, 0);
    if (point == NULL)
      {
	  fprintf (stderr, "Unable to get Point Symbolizer #13\n");
	  *retcode += 71;
	  return 0;
      }
    if (rl2_point_symbolizer_mark_get_well_known_type (point, 0, &type) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Point Symbolizer Mark GetWellKnownType #12\n");
	  *retcode += 72;
	  return 0;
      }
    if (type != RL2_GRAPHIC_MARK_X)
      {
	  fprintf (stderr,
		   "Unexpected Point Symbolizer Mark GetWellKnownType #12: %02x\n",
		   type);
	  *retcode += 73;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    intval = rl2_get_feature_type_style_columns_count (style);
    if (intval != 2)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnsCount #1: %d\n",
		   intval);
	  *retcode += 74;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 0);
    if (strcasecmp (string, "name") != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #1: \"%s\"\n",
		   string);
	  *retcode += 75;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 1);
    if (strcasecmp (string, "some_column") != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #2: \"%s\"\n",
		   string);
	  *retcode += 76;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 2);
    if (string != NULL)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #3: \"%s\"\n",
		   string);
	  *retcode += 77;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (NULL, 0);
    if (string != NULL)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #4: \"%s\"\n",
		   string);
	  *retcode += 78;
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
    if (!test_symbolizer (db_handle, "point1", "point_1", &ret))
	return ret;
    ret = -200;
    if (!test_symbolizer (db_handle, "point2", "point_2", &ret))
	return ret;
    ret = -300;
    if (!test_symbolizer (db_handle, "point2", "point_3", &ret))
	return ret;
    ret = -400;
    if (!test_style (db_handle, "point1", "point_style", &ret))
	return ret;
    ret = -500;
    if (!test_filter (db_handle, "point1", "point_style", &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    return result;
}
