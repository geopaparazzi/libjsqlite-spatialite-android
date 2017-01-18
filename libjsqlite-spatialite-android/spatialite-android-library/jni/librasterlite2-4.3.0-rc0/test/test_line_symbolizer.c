/*

 test_line_symbolizer.c -- RasterLite-2 Test Case

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
/* testing a LineSymbolizer */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2LineSymbolizerPtr line;
    int intval;
    int index;
    double dblval;
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
    if (intval != RL2_LINE_SYMBOLIZER)
      {
	  fprintf (stderr,
		   "%s: Unexpected Vector Symbolizer Item Type #1: %d\n",
		   style_name, intval);
	  *retcode += 8;
	  return 0;
      }

    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "%s: Unable to get a Line Symbolizer #1\n",
		   style_name);
	  *retcode += 9;
	  return 0;
      }

    if (rl2_line_symbolizer_has_stroke (line, &intval) != RL2_OK)
      {
	  fprintf (stderr, "%s: Unable to get Line Symbolizer HasStroke #1\n",
		   style_name);
	  *retcode += 10;
	  return 0;
      }
    if (intval != 1)
      {
	  fprintf (stderr, "%s: Unexpected Line Symbolizer HasStroke #1: %d\n",
		   style_name, intval);
	  *retcode += 11;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeColor #1\n",
		   style_name);
	  *retcode += 12;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1") == 0)
      {
	  if (red == 0xff && green == 0xff && blue == 0x00)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (red == 0x00 && green == 0x00 && blue == 0x00)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 13;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_opacity (line, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeOpacity #1\n",
		   style_name);
	  *retcode += 14;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1") == 0)
      {
	  if (dblval == 1.0)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (dblval == 0.5)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
      {
	  if (dblval == 0.75)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeOpacity #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 15;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_width (line, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeWidth #1\n",
		   style_name);
	  *retcode += 16;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1") == 0)
      {
	  if (dblval == 4.0)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (dblval == 1.0)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
      {
	  if (dblval == 2.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeWidth #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 17;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_linejoin (line, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeLinejoin #1\n",
		   style_name);
	  *retcode += 18;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_UNKNOWN)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_ROUND)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_MITRE)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeLinejoin #1: %02x\n",
		   style_name, red);
	  *retcode += 19;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_linecap (line, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeLinecap #1\n",
		   style_name);
	  *retcode += 20;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_UNKNOWN)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_SQUARE)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_ROUND)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeLinecap #1: %02x\n",
		   style_name, red);
	  *retcode += 21;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_dash_count (line, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeDashCount #1\n",
		   style_name);
	  *retcode += 22;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "line_1") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (intval == 6)
	      red = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeDashCount #1: %d\n",
		   style_name, intval);
	  *retcode += 23;
	  return 0;
      }

    if (strcmp (style_name, "line_1") == 0
	|| strcmp (style_name, "line_3") == 0)
      {
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 0, &dblval) ==
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Expected failure, got success: Line Symbolizer GetStrokeDashItem #1\n");
		*retcode += 24;
		return 0;
	    }
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 0, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Line Symbolizer GetStrokeDashItem #1\n",
			 style_name);
		*retcode += 25;
		return 0;
	    }
	  if (dblval != 20.0)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #1: %1.4f\n",
			 dblval);
		*retcode += 26;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 1, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #2\n");
		*retcode += 27;
		return 0;
	    }
	  if (dblval != 10.1)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #2: %1.4f\n",
			 dblval);
		*retcode += 28;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 2, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #3\n");
		*retcode += 29;
		return 0;
	    }
	  if (dblval != 5.5)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #3: %1.4f\n",
			 dblval);
		*retcode += 30;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 3, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #4\n");
		*retcode += 31;
		return 0;
	    }
	  if (dblval != 4.0)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #4: %1.4f\n",
			 dblval);
		*retcode += 32;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 4, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #5\n");
		*retcode += 33;
		return 0;
	    }
	  if (dblval != 6.0)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #4: %1.4f\n",
			 dblval);
		*retcode += 34;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 5, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #5\n");
		*retcode += 35;
		return 0;
	    }
	  if (dblval != 12.0)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #5: %1.4f\n",
			 dblval);
		*retcode += 36;
		return 0;
	    }
      }

    if (rl2_line_symbolizer_get_stroke_dash_offset (line, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeDashOffset #1\n",
		   style_name);
	  *retcode += 37;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1") == 0
	|| strcmp (style_name, "line_3") == 0)
      {
	  if (dblval == 0.0)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (dblval == 10.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeDashOffset #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 38;
	  return 0;
      }

    if (rl2_line_symbolizer_get_perpendicular_offset (line, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetPerpendicularOffset #1\n",
		   style_name);
	  *retcode += 39;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1") == 0
	|| strcmp (style_name, "line_3") == 0)
      {
	  if (dblval == 0.0)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2") == 0)
      {
	  if (dblval == 5.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Line Symbolizer GetPerpendicularOffset #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 40;
	  return 0;
      }

    if (rl2_line_symbolizer_has_graphic_stroke (line, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer HasGraphicStroke #1\n",
		   style_name);
	  *retcode += 41;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "line_1") == 0
	|| strcmp (style_name, "line_2") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Line Symbolizer HasGraphicStroke #1: %d\n",
		   style_name, intval);
	  *retcode += 42;
	  return 0;
      }

    string = rl2_line_symbolizer_get_graphic_stroke_href (line);
    intval = 0;
    if (strcmp (style_name, "line_1") == 0
	|| strcmp (style_name, "line_2") == 0)
      {
	  if (string == NULL)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
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
		   "%s; Unexpected Line Symbolizer GetGraphicStrokeHref #1: %s\n",
		   style_name, string);
	  *retcode += 43;
	  return 0;
      }

    if (rl2_line_symbolizer_get_graphic_stroke_recode_count (line, &intval) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetGraphicStrokeRecodeCount #1\n",
		   style_name);
	  *retcode += 44;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "line_1") == 0
	|| strcmp (style_name, "line_2") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Line Symbolizer GetGraphicStrokeRecodeCount #1: %d\n",
		   style_name, intval);
	  *retcode += 45;
	  return 0;
      }

    intval =
	rl2_line_symbolizer_get_graphic_stroke_recode_color (line, 0, &index,
							     &red, &green,
							     &blue);
    if (strcmp (style_name, "line_1") == 0
	|| strcmp (style_name, "line_2") == 0)
      {
	  if (intval == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Line Symbolizer GetGraphicStrokeRecodeColor #1\n",
			 style_name);
		*retcode += 46;
		return 0;
	    }
	  intval = 1;
      }
    if (strcmp (style_name, "line_3") == 0)
      {
	  if (intval != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Line Symbolizer GetGraphicStrokeRecodeColor #1\n",
			 style_name);
		*retcode += 47;
		return 0;
	    }
	  intval = 0;
	  if (index == 0 && red == 0xff && green == 0x00 && blue == 0x80)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetGraphicStrokeRecodeColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 48;
	  return 0;
      }

    rl2_destroy_feature_type_style (style);
    return 1;
}

static int
test_style (sqlite3 * db_handle, const char *coverage,
	    const char *style_name, int *retcode)
{
/* testing a FeatureType Style (LineSymbolizers) */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2LineSymbolizerPtr line;
    int intval;
    int index;
    double dblval;
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
    if (strcmp (string, "line_style") != 0)
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
    if (intval != RL2_LINE_SYMBOLIZER)
      {
	  fprintf (stderr,
		   "%s: Unexpected Vector Symbolizer Item Type #2: %d\n",
		   style_name, intval);
	  *retcode += 8;
	  return 0;
      }

    line = rl2_get_line_symbolizer (symbolizer, 1);
    if (line == NULL)
      {
	  fprintf (stderr, "%s: Unable to get a Line Symbolizer #2\n",
		   style_name);
	  *retcode += 9;
	  return 0;
      }

    if (rl2_get_text_symbolizer (symbolizer, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Text Symbolizer #1\n");
	  *retcode += 10;
	  return 0;
      }

    if (rl2_get_point_symbolizer (symbolizer, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Point Symbolizer #1\n");
	  *retcode += 11;
	  return 0;
      }

    if (rl2_get_text_symbolizer (NULL, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Text Symbolizer #2\n");
	  *retcode += 12;
	  return 0;
      }

    if (rl2_get_point_symbolizer (NULL, 0) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Point Symbolizer #2\n");
	  *retcode += 13;
	  return 0;
      }

    if (rl2_get_vector_symbolizer_count (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: Vector Symbolizer Count #2\n");
	  *retcode += 14;
	  return 0;
      }

    if (rl2_get_vector_symbolizer_item_type (symbolizer, 10, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Vector Symbolizer GetItemType #2\n");
	  *retcode += 15;
	  return 0;
      }

    if (rl2_get_vector_symbolizer_item_type (NULL, 0, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Vector Symbolizer GetItemType #3\n");
	  *retcode += 16;
	  return 0;
      }

    if (rl2_line_symbolizer_has_stroke (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer HasStroke #2\n");
	  *retcode += 17;
	  return 0;
      }

    if (rl2_line_symbolizer_has_graphic_stroke (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer HasGraphicStroke #2\n");
	  *retcode += 18;
	  return 0;
      }

    if (rl2_line_symbolizer_get_graphic_stroke_href (NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetGraphicStrokeHref #2\n");
	  *retcode += 19;
	  return 0;
      }

    if (rl2_line_symbolizer_get_graphic_stroke_recode_count (NULL, &intval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetGraphicStrokeRecodeCount #2\n");
	  *retcode += 20;
	  return 0;
      }

    if (rl2_polygon_symbolizer_get_graphic_stroke_recode_color (NULL, 0,
								&index, &red,
								&green,
								&blue) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetGraphicStrokeRecodeColor #2\n");
	  *retcode += 21;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_color (NULL, &red, &green, &blue) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetStrokeColor #2\n");
	  *retcode += 22;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_opacity (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetStrokeOpacity #2\n");
	  *retcode += 23;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_width (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetStrokeWidth #2\n");
	  *retcode += 24;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_linejoin (NULL, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetStrokeLinejoin #2\n");
	  *retcode += 25;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_linecap (NULL, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetStrokeLinecap #2\n");
	  *retcode += 26;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_dash_count (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetStrokeDashCount #2\n");
	  *retcode += 27;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_dash_item (NULL, 0, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetStrokeDashItem #2\n");
	  *retcode += 28;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_dash_offset (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetStrokeDashOffset #2\n");
	  *retcode += 29;
	  return 0;
      }

    if (rl2_line_symbolizer_get_perpendicular_offset (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Line Symbolizer GetPerpendicularOffset #2\n");
	  *retcode += 30;
	  return 0;
      }

    rl2_destroy_feature_type_style (style);
    return 1;
}

static int
test_filter (sqlite3 * db_handle, const char *coverage,
	     const char *style_name, int *retcode)
{
/* testing Filter from a FeatureType Style (LineSymbolizers) */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2LineSymbolizerPtr line;
    rl2VariantArrayPtr value;
    int intval;
    const char *string;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int scale_forbidden;
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
    if (rl2_set_variant_double (value, 0, "some_column", 6000000000.4) !=
	RL2_OK)
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #2\n");
	  *retcode += 5;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer GetStrokeColor #2\n");
	  *retcode += 6;
	  return 0;
      }
    if (red != 0x01 || green != 0xff || blue != 0x02)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #2: %02x%02x%02x\n",
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
    if (rl2_set_variant_double (value, 0, "some_column", 4.4) != RL2_OK)
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #3\n");
	  *retcode += 11;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer GetStrokeColor #3\n");
	  *retcode += 12;
	  return 0;
      }
    if (red != 0xff || green != 0x01 || blue != 0x20)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #3: %02x%02x%02x\n",
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
    if (rl2_set_variant_blob (value, 0, "some_column", blob, 8) != RL2_OK)
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #4\n");
	  *retcode += 17;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer GetStrokeColor #4\n");
	  *retcode += 18;
	  return 0;
      }
    if (red != 0x00 || green != 0x00 || blue != 0xff)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #4: %02x%02x%02x\n",
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
    if (rl2_set_variant_int (value, 0, "some_column", 6000000000) != RL2_OK)
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #5\n");
	  *retcode += 23;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer GetStrokeColor #5\n");
	  *retcode += 24;
	  return 0;
      }
    if (red != 0x01 || green != 0xff || blue != 0x02)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #5: %02x%02x%02x\n",
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
    if (rl2_set_variant_int (value, 0, "some_column", 2) != RL2_OK)
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #6\n");
	  *retcode += 29;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer GetStrokeColor #6\n");
	  *retcode += 30;
	  return 0;
      }
    if (red != 0x12 || green != 0x34 || blue != 0x56)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #6: %02x%02x%02x\n",
		   red, green, blue);
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
    if (rl2_set_variant_double (value, 0, "some_column", 2.5) != RL2_OK)
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #7\n");
	  *retcode += 35;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer GetStrokeColor #7\n");
	  *retcode += 37;
	  return 0;
      }
    if (red != 0x12 || green != 0x34 || blue != 0x56)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #7: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 38;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #6\n");
	  *retcode += 38;
	  return 0;
      }
    string = "0 walhalla";
    if (rl2_set_variant_text (value, 0, "some_column", string, strlen (string))
	!= RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #6\n");
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #8\n");
	  *retcode += 41;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer GetStrokeColor #8\n");
	  *retcode += 42;
	  return 0;
      }
    if (red != 0x12 || green != 0x34 || blue != 0x56)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #8: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 43;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #7\n");
	  *retcode += 44;
	  return 0;
      }
    string = "100200300";
    if (rl2_set_variant_text (value, 0, "some_column", string, strlen (string))
	!= RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #7\n");
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #9\n");
	  *retcode += 47;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer GetStrokeColor #9\n");
	  *retcode += 48;
	  return 0;
      }
    if (red != 0x00 || green != 0x00 || blue != 0xff)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #9: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 49;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #8\n");
	  *retcode += 50;
	  return 0;
      }
    if (rl2_set_variant_int (value, 0, "some_column", 100200300) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #8\n");
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #10\n");
	  *retcode += 53;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Line Symbolizer GetStrokeColor #10\n");
	  *retcode += 54;
	  return 0;
      }
    if (red != 0x00 || green != 0x00 || blue != 0xff)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #10: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 55;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #9\n");
	  *retcode += 56;
	  return 0;
      }
    if (rl2_set_variant_double (value, 0, "some_column", 100200300.0) != RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #9\n");
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #11\n");
	  *retcode += 59;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Line Symbolizer GetStrokeColor #11\n");
	  *retcode += 60;
	  return 0;
      }
    if (red != 0x00 || green != 0x00 || blue != 0xff)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #11: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 61;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #10\n");
	  *retcode += 62;
	  return 0;
      }
    string = "TOSCANA";
    if (rl2_set_variant_text (value, 0, "name", string, strlen (string)) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #10\n");
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
    line = rl2_get_line_symbolizer (symbolizer, 0);
    if (line == NULL)
      {
	  fprintf (stderr, "Unable to get Line Symbolizer #12\n");
	  *retcode += 65;
	  return 0;
      }
    if (rl2_line_symbolizer_get_stroke_color (line, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to get Line Symbolizer GetStrokeColor #12\n");
	  *retcode += 66;
	  return 0;
      }
    if (red != 0xff || green != 0xa0 || blue != 0x80)
      {
	  fprintf (stderr,
		   "Unexpected Line Symbolizer GetStrokeColor #12: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 67;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    intval = rl2_get_feature_type_style_columns_count (style);
    if (intval != 2)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnsCount #1: %d\n",
		   intval);
	  *retcode += 68;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 0);
    if (strcasecmp (string, "name") != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #1: \"%s\"\n",
		   string);
	  *retcode += 69;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 1);
    if (strcasecmp (string, "some_column") != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #2: \"%s\"\n",
		   string);
	  *retcode += 70;
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
    if (!test_symbolizer (db_handle, "line1", "line_1", &ret))
	return ret;
    ret = -200;
    if (!test_symbolizer (db_handle, "line2", "line_2", &ret))
	return ret;
    ret = -300;
    if (!test_symbolizer (db_handle, "line2", "line_3", &ret))
	return ret;
    ret = -400;
    if (!test_style (db_handle, "line1", "line_style", &ret))
	return ret;
    ret = -500;
    if (!test_filter (db_handle, "line1", "line_style", &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    return result;
}
