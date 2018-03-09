/*

 test_text_symbolizer_col.c -- RasterLite-2 Test Case

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
/* testing a TextSymbolizer */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2TextSymbolizerPtr text;
    int intval;
    int ret;
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
    if (intval != RL2_TEXT_SYMBOLIZER)
      {
	  fprintf (stderr,
		   "%s: Unexpected Vector Symbolizer Item Type #1: %d\n",
		   style_name, intval);
	  *retcode += 8;
	  return 0;
      }

    text = rl2_get_text_symbolizer (symbolizer, 0);
    if (text == NULL)
      {
	  fprintf (stderr, "%s: Unable to get a Text Symbolizer #1\n",
		   style_name);
	  *retcode += 9;
	  return 0;
      }

    string = rl2_text_symbolizer_get_label (text);
    intval = 0;
    if (string != NULL)
      {
	  fprintf (stderr,
		   "%s; Unexpected Text Symbolizer GetLabel #1: %s\n",
		   style_name, string);
	  *retcode += 10;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_families_count (text, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer GetFontFamiliesCount #1\n",
		   style_name);
	  *retcode += 11;
	  return 0;
      }
    red = 0;
    if (intval == 0)
	red = 1;
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetFontFamiliesCount #1: %d\n",
		   style_name, intval);
	  *retcode += 12;
	  return 0;
      }

    string = rl2_text_symbolizer_get_col_label (text);
    if (string == NULL)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer GetColLabel #1\n",
		   style_name);
	  *retcode += 13;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "some_column") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (strcmp (string, "label_column") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetColLabel #1: %s\n",
		   style_name, string);
	  *retcode += 14;
	  return 0;
      }

    string = rl2_text_symbolizer_get_col_font (text);
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetColFontName #1\n",
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
			 "%s: Unable to get Text Symbolizer GetColFontName #1\n",
			 style_name);
		*retcode += 15;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "font") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetColFontName #1: %s\n",
		   style_name, string);
	  *retcode += 16;
	  return 0;
      }

    string = rl2_text_symbolizer_get_col_style (text);
    if (string == NULL)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer GetColFontStyle #1\n",
		   style_name);
	  *retcode += 17;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "style") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (strcmp (string, "col_style") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetColFontStyle #1: %s\n",
		   style_name, string);
	  *retcode += 18;
	  return 0;
      }

    string = rl2_text_symbolizer_get_col_weight (text);
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetColFontWeight #1\n",
			 style_name);
		*retcode += 19;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetColFontWeight #1\n",
			 style_name);
		*retcode += 19;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "weight") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetColFontWeight #1: %s\n",
		   style_name, string);
	  *retcode += 20;
	  return 0;
      }

    string = rl2_text_symbolizer_get_col_size (text);
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetColFontSize #1\n",
			 style_name);
		*retcode += 21;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetColFontSize #1\n",
			 style_name);
		*retcode += 21;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "size") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetColFontSize #1: %s\n",
		   style_name, string);
	  *retcode += 22;
	  return 0;
      }

    if (rl2_text_symbolizer_get_label_placement_mode (text, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer GetLabelPlacementMode #1\n",
		   style_name);
	  *retcode += 23;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (red == RL2_LABEL_PLACEMENT_POINT)
	      intval = 1;
      }
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (red == RL2_LABEL_PLACEMENT_LINE)
	      intval = 1;
      }
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (red == RL2_LABEL_PLACEMENT_UNKNOWN)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLabelPlacementMode #1: %02x\n",
		   style_name, red);
	  *retcode += 24;
	  return 0;
      }

    string = rl2_text_symbolizer_get_point_placement_col_anchor_point_x (text);
    if (strcmp (style_name, "label_2_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetPointPlacementAnchorPointX #1\n",
			 style_name);
		*retcode += 25;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetPointPlacementColAnchorPointX #1\n",
			 style_name);
		*retcode += 25;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (strcmp (string, "point_x") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetPointPlacementColAnchorPointX #1: %s\n",
		   style_name, string);
	  *retcode += 26;
	  return 0;
      }

    string = rl2_text_symbolizer_get_point_placement_col_anchor_point_y (text);
    if (strcmp (style_name, "label_2_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetPointPlacementColAnchorPointY #1\n",
			 style_name);
		*retcode += 27;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetPointPlacementColAnchorPointY #1\n",
			 style_name);
		*retcode += 27;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (strcmp (string, "point_y") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetPointPlacementColAnchorPointY #1: %s\n",
		   style_name, string);
	  *retcode += 28;
	  return 0;
      }

    string = rl2_text_symbolizer_get_point_placement_col_displacement_x (text);
    if (strcmp (style_name, "label_2_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetPointPlacementColDisplacemenX #1\n",
			 style_name);
		*retcode += 29;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetPointPlacementColDisplacementX #1\n",
			 style_name);
		*retcode += 29;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (strcmp (string, "displ_x") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetPointPlacementColDisplacementX #1: %s\n",
		   style_name, string);
	  *retcode += 30;
	  return 0;
      }

    string = rl2_text_symbolizer_get_point_placement_col_displacement_y (text);
    if (strcmp (style_name, "label_2_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetPointPlacementColDisplacemenY #1\n",
			 style_name);
		*retcode += 31;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetPointPlacementColDisplacementY #1\n",
			 style_name);
		*retcode += 31;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (strcmp (string, "displ_y") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetPointPlacementColDisplacementY #1: %s\n",
		   style_name, string);
	  *retcode += 32;
	  return 0;
      }

    string = rl2_text_symbolizer_get_point_placement_col_rotation (text);
    if (strcmp (style_name, "label_2_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetPointPlacementColRotation #1\n",
			 style_name);
		*retcode += 33;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetPointPlacementColRotation #1\n",
			 style_name);
		*retcode += 33;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (strcmp (string, "rotation") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetPointPlacementColRotation #1: %s\n",
		   style_name, string);
	  *retcode += 34;
	  return 0;
      }

    string =
	rl2_text_symbolizer_get_line_placement_col_perpendicular_offset (text);
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementColPerpendicularOffset #1\n",
			 style_name);
		*retcode += 35;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementColPerpendicularOffset #1\n",
			 style_name);
		*retcode += 36;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "perpoff") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementColPerpendicularOffset #1: %s\n",
		   style_name, string);
	  *retcode += 37;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_line_placement_is_repeated (text, &intval);
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementIsRepeated #1\n",
			 style_name);
		*retcode += 38;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementIsRepeated #1\n",
			 style_name);
		*retcode += 39;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    else
	red = 1;
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementIsRepeated #1: %d\n",
		   style_name, intval);
	  *retcode += 40;
	  return 0;
      }

    string = rl2_text_symbolizer_get_line_placement_col_initial_gap (text);
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementColInitialGap #1\n",
			 style_name);
		*retcode += 41;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementColInitialGap #1\n",
			 style_name);
		*retcode += 42;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "inigap") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementColInitialGap #1: %s\n",
		   style_name, string);
	  *retcode += 43;
	  return 0;
      }

    string = rl2_text_symbolizer_get_line_placement_col_gap (text);
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementColGap #1\n",
			 style_name);
		*retcode += 44;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementColGap #1\n",
			 style_name);
		*retcode += 45;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "gap") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementColGap #1: %s\n",
		   style_name, string);
	  *retcode += 46;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_line_placement_is_aligned (text, &intval);
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementIsAligned #1\n",
			 style_name);
		*retcode += 47;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementIsAligned #1\n",
			 style_name);
		*retcode += 48;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    else
	red = 1;
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementIsAligned #1: %d\n",
		   style_name, intval);
	  *retcode += 49;
	  return 0;
      }

    ret =
	rl2_text_symbolizer_get_line_placement_generalize_line (text, &intval);
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementGeneralizeLine #1\n",
			 style_name);
		*retcode += 50;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementGeneralizeLine #1\n",
			 style_name);
		*retcode += 51;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    else
	red = 1;
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementGeneralizeLine #1: %d\n",
		   style_name, intval);
	  *retcode += 52;
	  return 0;
      }

    if (rl2_text_symbolizer_has_halo (text, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer HasHalo #1\n",
		   style_name);
	  *retcode += 54;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HasHalo #1: %d\n",
		   style_name, intval);
	  *retcode += 55;
	  return 0;
      }

    string = rl2_text_symbolizer_get_halo_col_radius (text);
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetHaloColRadius #1\n",
			 style_name);
		*retcode += 56;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetHaloColRadius #1\n",
			 style_name);
		*retcode += 57;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "halo_radius") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetHaloColRadius #1: %s\n",
		   style_name, string);
	  *retcode += 58;
	  return 0;
      }

    ret = rl2_text_symbolizer_has_halo_fill (text, &intval);
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer HasHaloFill #1\n",
			 style_name);
		*retcode += 59;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer HasHaloFill #1\n",
			 style_name);
		*retcode += 60;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_3_col") == 0)
	red = 1;
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HasHaloFill #1: %d\n",
		   style_name, intval);
	  *retcode += 61;
	  return 0;
      }

    string = rl2_text_symbolizer_get_halo_col_fill_color (text);
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer HaloColFillColor #1\n",
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
			 "%s: Unable to get Text Symbolizer HaloColFillColor #1\n",
			 style_name);
		*retcode += 63;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (strcmp (string, "halo_color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HaloColFillColor #1: %s\n",
		   style_name, string);
	  *retcode += 64;
	  return 0;
      }

    string = rl2_text_symbolizer_get_halo_col_fill_opacity (text);
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer HaloColFillOpacity #1\n",
			 style_name);
		*retcode += 65;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer HaloColFillOpacity #1\n",
			 style_name);
		*retcode += 66;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "halo_opacity") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HaloColFillOpacity #1: %s\n",
		   style_name, string);
	  *retcode += 67;
	  return 0;
      }

    if (rl2_text_symbolizer_has_fill (text, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer HasFill #1\n",
		   style_name);
	  *retcode += 68;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HasFill #1: %d\n",
		   style_name, intval);
	  *retcode += 69;
	  return 0;
      }

    string = rl2_text_symbolizer_get_col_fill_color (text);
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer ColFillColor #1\n",
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
			 "%s: Unable to get Text Symbolizer ColFillColor #1\n",
			 style_name);
		*retcode += 71;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "fill_color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer ColFillColor #1: %s\n",
		   style_name, string);
	  *retcode += 72;
	  return 0;
      }

    string = rl2_text_symbolizer_get_col_fill_opacity (text);
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer ColFillOpacity #1\n",
			 style_name);
		*retcode += 73;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer ColFillOpacity #1\n",
			 style_name);
		*retcode += 74;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "fill_opacity") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer ColFillOpacity #1: %s\n",
		   style_name, string);
	  *retcode += 75;
	  return 0;
      }

    intval = rl2_get_feature_type_style_columns_count (style);
    red = 0;
    if (strcmp (style_name, "label_1_col") == 0
	|| strcmp (style_name, "label_2_col") == 0)
      {
	  if (intval == 13)
	      red = 1;
      }
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer StyleColumnsCount #1: %d\n",
		   style_name, intval);
	  *retcode += 76;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 0);
    if (string == NULL)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer StyleColumnName #1\n",
		   style_name);
	  *retcode += 77;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (strcmp (string, "label_column") == 0)
	      intval = 1;
      }
    else
      {
	  if (strcmp (string, "some_column") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer StyleColumnsName #1: %s\n",
		   style_name, string);
	  *retcode += 78;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 7);
    if (strcmp (style_name, "label_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Text Symbolizer StyleColumnName #2\n",
			 style_name);
		*retcode += 79;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer StyleColumnName #2\n",
			 style_name);
		*retcode += 80;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1_col") == 0)
      {
	  if (strcmp (string, "displ_x") == 0)
	      intval = 1;
      }
    else if (strcmp (style_name, "label_2_col") == 0)
      {
	  if (strcmp (string, "gap") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer StyleColumnsName #2: %s\n",
		   style_name, string);
	  *retcode += 81;
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
    if (!test_symbolizer (db_handle, "points", "label_1_col", &ret))
	return ret;
    ret = -200;
    if (!test_symbolizer (db_handle, "lines", "label_2_col", &ret))
	return ret;
    ret = -300;
    if (!test_symbolizer (db_handle, "points", "label_3_col", &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    return result;
}
