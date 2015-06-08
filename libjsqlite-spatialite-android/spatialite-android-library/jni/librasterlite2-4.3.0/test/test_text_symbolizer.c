/*

 test_text_symbolizer.c -- RasterLite-2 Test Case

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
/* testing a TextSymbolizer */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2TextSymbolizerPtr text;
    int intval;
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
	  if (strcmp (string, "some_column") == 0)
	      intval = 1;
      }
    if (intval != 1)
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
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_2") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetFontFamiliesCount #1: %d\n",
		   style_name, intval);
	  *retcode += 12;
	  return 0;
      }

    string = rl2_text_symbolizer_get_font_family_name (text, 0);
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetFontFamilyName #1\n",
			 style_name);
		*retcode += 13;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetFontFamilyName #1\n",
			 style_name);
		*retcode += 14;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_2") == 0)
      {
	  if (strcmp (string, "Courier") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Text Symbolizer GetFontFamilyName #1: %s\n",
		   style_name, string);
	  *retcode += 15;
	  return 0;
      }

    string = rl2_text_symbolizer_get_font_family_name (text, 1);
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetFontFamilyName #2\n",
			 style_name);
		*retcode += 16;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetFontFamilyName #2\n",
			 style_name);
		*retcode += 17;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_2") == 0)
      {
	  if (strcmp (string, "Arial") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Text Symbolizer GetFontFamilyName #2: %s\n",
		   style_name, string);
	  *retcode += 18;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_style (text, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer GetFontStyle #1\n",
		   style_name);
	  *retcode += 19;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (red == RL2_FONT_STYLE_ITALIC)
	      intval = 1;
      }
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (red == RL2_FONT_STYLE_OBLIQUE)
	      intval = 1;
      }
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (red == RL2_FONT_STYLE_NORMAL)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetFontStyle #1: %02x\n",
		   style_name, red);
	  *retcode += 20;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_weight (text, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer GetFontWeight #1\n",
		   style_name);
	  *retcode += 21;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (red == RL2_FONT_WEIGHT_BOLD)
	      intval = 1;
      }
    else
      {
	  if (red == RL2_FONT_WEIGHT_NORMAL)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetFontWeight #1: %02x\n",
		   style_name, red);
	  *retcode += 22;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_size (text, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer GetFontSize #1\n",
		   style_name);
	  *retcode += 23;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (dblval == 12.5)
	      intval = 1;
      }
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (dblval == 7.5)
	      intval = 1;
      }
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (dblval == 10.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetFontSize #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 24;
	  return 0;
      }

    if (rl2_text_symbolizer_get_label_placement_mode (text, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer GetLabelPlacementMode #1\n",
		   style_name);
	  *retcode += 25;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (red == RL2_LABEL_PLACEMENT_POINT)
	      intval = 1;
      }
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (red == RL2_LABEL_PLACEMENT_LINE)
	      intval = 1;
      }
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (red == RL2_LABEL_PLACEMENT_UNKNOWN)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLabelPlacementMode #1: %02x\n",
		   style_name, red);
	  *retcode += 26;
	  return 0;
      }

    ret =
	rl2_text_symbolizer_get_point_placement_anchor_point (text, &dblval,
							      &dblval2);
    if (strcmp (style_name, "label_2") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetPointPlacementAnchorPoint #1\n",
			 style_name);
		*retcode += 27;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetPointPlacementAnchorPoint #1\n",
			 style_name);
		*retcode += 28;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (dblval == 2.0 && dblval2 == 4.0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetPointPlacementAnchorPoint #1: %1.4f %1.4f\n",
		   style_name, dblval, dblval2);
	  *retcode += 29;
	  return 0;
      }

    ret =
	rl2_text_symbolizer_get_point_placement_displacement (text, &dblval,
							      &dblval2);
    if (strcmp (style_name, "label_2") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetPointPlacementDisplacement #1\n",
			 style_name);
		*retcode += 30;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetPointPlacementDisplacement #1\n",
			 style_name);
		*retcode += 31;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (dblval == 10.0 && dblval2 == 5.0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetPointPlacementDisplacement #1: %1.4f %1.4f\n",
		   style_name, dblval, dblval2);
	  *retcode += 32;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_point_placement_rotation (text, &dblval);
    if (strcmp (style_name, "label_2") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetPointPlacementRotation #1\n",
			 style_name);
		*retcode += 33;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetPointPlacementRotation #1\n",
			 style_name);
		*retcode += 34;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (dblval == 15.0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetPointPlacementRotation #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 35;
	  return 0;
      }

    ret =
	rl2_text_symbolizer_get_line_placement_perpendicular_offset (text,
								     &dblval);
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementPerpendicularOffset #1\n",
			 style_name);
		*retcode += 36;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementPerpendicularOffset #1\n",
			 style_name);
		*retcode += 37;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (dblval == 13.0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementPerpendicularOffset #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 38;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_line_placement_is_repeated (text, &intval);
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementIsRepeated #1\n",
			 style_name);
		*retcode += 39;
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
		*retcode += 40;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "label_2") == 0)
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
	  *retcode += 41;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_line_placement_initial_gap (text, &dblval);
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementInitialGap #1\n",
			 style_name);
		*retcode += 42;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementInitialGap #1\n",
			 style_name);
		*retcode += 43;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (dblval == 6.5)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementInitialGap #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 44;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_line_placement_gap (text, &dblval);
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementGap #1\n",
			 style_name);
		*retcode += 45;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetLinePlacementGap #1\n",
			 style_name);
		*retcode += 46;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (dblval == 12.0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetLinePlacementGap #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 47;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_line_placement_is_aligned (text, &intval);
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementIsAligned #1\n",
			 style_name);
		*retcode += 48;
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
		*retcode += 49;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "label_2") == 0)
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
	  *retcode += 50;
	  return 0;
      }

    ret =
	rl2_text_symbolizer_get_line_placement_generalize_line (text, &intval);
    if (strcmp (style_name, "label_1") == 0
	|| strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetLinePlacementGeneralizeLine #1\n",
			 style_name);
		*retcode += 51;
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
		*retcode += 52;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "label_2") == 0)
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
	  *retcode += 53;
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
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HasHalo #1: %d\n",
		   style_name, intval);
	  *retcode += 56;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_halo_radius (text, &dblval);
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer GetHaloRadius #1\n",
			 style_name);
		*retcode += 57;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer GetHaloRadius #1\n",
			 style_name);
		*retcode += 58;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (dblval == 1.0)
	      intval = 1;
      }
    else if (strcmp (style_name, "label_2") == 0)
      {
	  if (dblval == 2.5)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer GetHaloRadius #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 59;
	  return 0;
      }

    ret = rl2_text_symbolizer_has_halo_fill (text, &intval);
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer HasHaloFill #1\n",
			 style_name);
		*retcode += 60;
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
		*retcode += 61;
		return 0;
	    }
      }
    red = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_3") == 0)
	red = 1;
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HasHaloFill #1: %d\n",
		   style_name, intval);
	  *retcode += 62;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_halo_fill_color (text, &red, &green, &blue);
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer HaloFillColor #1\n",
			 style_name);
		*retcode += 61;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer HaloFillColor #1\n",
			 style_name);
		*retcode += 62;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (red == 0xff && green == 0x80 && blue == 0x00)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HaloFillColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 63;
	  return 0;
      }

    if (rl2_text_symbolizer_has_fill (text, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Text Symbolizer HasFill #1\n",
		   style_name);
	  *retcode += 64;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_2") == 0)
      {
	  if (intval == 1)
	      red = 1;
      }
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer HasFill #1: %d\n",
		   style_name, intval);
	  *retcode += 65;
	  return 0;
      }

    ret = rl2_text_symbolizer_get_fill_color (text, &red, &green, &blue);
    if (strcmp (style_name, "label_3") == 0)
      {
	  if (ret == RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Text Symbolizer FillColor #1\n",
			 style_name);
		*retcode += 66;
		return 0;
	    }
      }
    else
      {
	  if (ret != RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Text Symbolizer FillColor #1\n",
			 style_name);
		*retcode += 67;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "label_1") == 0)
      {
	  if (red == 0x06 && green == 0x0a && blue == 0x0f)
	      intval = 1;
      }
    else if (strcmp (style_name, "label_2") == 0)
      {
	  if (red == 0x10 && green == 0x20 && blue == 0x30)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Text Symbolizer FillColor #1: #%02x%02x%02x\n",
		   style_name, red, green, blue);
	  *retcode += 69;
	  return 0;
      }

    rl2_destroy_feature_type_style (style);
    return 1;
}

static int
test_style (sqlite3 * db_handle, const char *coverage,
	    const char *style_name, int *retcode)
{
/* testing a FeatureType Style (TextSymbolizers) */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2TextSymbolizerPtr text;
    rl2VariantArrayPtr value;
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
    if (strcmp (string, "text_style") != 0)
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
    if (intval != RL2_TEXT_SYMBOLIZER)
      {
	  fprintf (stderr,
		   "%s: Unexpected Vector Symbolizer Item Type #2: %d\n",
		   style_name, intval);
	  *retcode += 8;
	  return 0;
      }

    text = rl2_get_text_symbolizer (symbolizer, 1);
    if (text == NULL)
      {
	  fprintf (stderr, "%s: Unable to get a Text Symbolizer #2\n",
		   style_name);
	  *retcode += 9;
	  return 0;
      }

    if (rl2_text_symbolizer_get_label (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected success: Text Symbolizer GetLabel #2\n");
	  *retcode += 10;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_families_count (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetFontFamiliesCount #2\n");
	  *retcode += 11;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_family_name (text, 10) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetFontFamilyName #2\n");
	  *retcode += 12;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_family_name (NULL, 0) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetFontFamilyName #2\n");
	  *retcode += 13;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_style (NULL, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetFontStyle #2\n");
	  *retcode += 14;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_weight (NULL, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetFontWeight #2\n");
	  *retcode += 15;
	  return 0;
      }

    if (rl2_text_symbolizer_get_font_size (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetFontSize #2\n");
	  *retcode += 16;
	  return 0;
      }

    if (rl2_text_symbolizer_get_label_placement_mode (NULL, &red) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetLabelPlacementMode #2\n");
	  *retcode += 17;
	  return 0;
      }

    if (rl2_text_symbolizer_get_point_placement_anchor_point
	(NULL, &dblval, &dblval2) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetPointPlacementAnchorPoint #2\n");
	  *retcode += 18;
	  return 0;
      }

    if (rl2_text_symbolizer_get_point_placement_displacement
	(NULL, &dblval, &dblval2) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetPointPlacementDisplacement #2\n");
	  *retcode += 19;
	  return 0;
      }

    if (rl2_text_symbolizer_get_point_placement_rotation (NULL, &dblval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetPointPlacementRotation #2\n");
	  *retcode += 20;
	  return 0;
      }

    if (rl2_text_symbolizer_get_line_placement_perpendicular_offset
	(NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetLinePlacementPerpendicularOffset #2\n");
	  *retcode += 21;
	  return 0;
      }

    if (rl2_text_symbolizer_get_line_placement_is_repeated (NULL, &intval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetLinePlacementIsRepeated #2\n");
	  *retcode += 22;
	  return 0;
      }

    if (rl2_text_symbolizer_get_line_placement_initial_gap (NULL, &dblval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetLinePlacementInitialGap #2\n");
	  *retcode += 23;
	  return 0;
      }

    if (rl2_text_symbolizer_get_line_placement_gap (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetLinePlacementGap #2\n");
	  *retcode += 24;
	  return 0;
      }

    if (rl2_text_symbolizer_get_line_placement_is_aligned (NULL, &intval) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetLinePlacementIsAligned #2\n");
	  *retcode += 25;
	  return 0;
      }

    if (rl2_text_symbolizer_get_line_placement_generalize_line (NULL, &intval)
	== RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetLinePlacementGeneralizeLine #2\n");
	  *retcode += 26;
	  return 0;
      }

    if (rl2_text_symbolizer_has_halo (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: Text Symbolizer HasHalo #2\n");
	  *retcode += 27;
	  return 0;
      }

    if (rl2_text_symbolizer_get_halo_radius (NULL, &dblval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetHaloRadius #2\n");
	  *retcode += 28;
	  return 0;
      }

    if (rl2_text_symbolizer_has_halo_fill (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer HasHaloFill #2\n");
	  *retcode += 29;
	  return 0;
      }

    if (rl2_text_symbolizer_get_halo_fill_color (NULL, &red, &green, &blue) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetHaloFillColor #2\n");
	  *retcode += 30;
	  return 0;
      }

    if (rl2_text_symbolizer_has_fill (NULL, &intval) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: Text Symbolizer HasFill #2\n");
	  *retcode += 31;
	  return 0;
      }

    if (rl2_text_symbolizer_get_fill_color (NULL, &red, &green, &blue) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: Text Symbolizer GetFillColor #2\n");
	  *retcode += 32;
	  return 0;
      }

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #1\n");
	  *retcode += 33;
	  return 0;
      }
    string = "*emilia#Romagna";
    if (rl2_set_variant_text (value, 0, "name", string, strlen (string)) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #1\n");
	  *retcode += 34;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #2\n",
		   style_name);
	  *retcode += 35;
	  return 0;
      }
    text = rl2_get_text_symbolizer (symbolizer, 0);
    if (text == NULL)
      {
	  fprintf (stderr, "Unable to get Text Symbolizer #2\n");
	  *retcode += 36;
	  return 0;
      }
    if (rl2_text_symbolizer_get_fill_color (text, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Text Symbolizer GetFillColor #2\n");
	  *retcode += 37;
	  return 0;
      }
    if (red != 0x60 || green != 0x72 || blue != 0x84)
      {
	  fprintf (stderr,
		   "Unexpected Text Symbolizer GetStrokeColor #2: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 38;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    value = rl2_create_variant_array (1);
    if (value == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VariantArray #2\n");
	  *retcode += 39;
	  return 0;
      }
    string = "*em*lia#Romagnola";
    if (rl2_set_variant_text (value, 0, "name", string, strlen (string)) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected failure: SetVariantValue #2\n");
	  *retcode += 40;
	  return 0;
      }
    symbolizer =
	rl2_get_symbolizer_from_feature_type_style (style, 5000000.0, value,
						    &scale_forbidden);
    if (symbolizer == NULL)
      {
	  fprintf (stderr, "Unexpected NULL VectorSymbolizer (%s) #3\n",
		   style_name);
	  *retcode += 41;
	  return 0;
      }
    text = rl2_get_text_symbolizer (symbolizer, 0);
    if (text == NULL)
      {
	  fprintf (stderr, "Unable to get Text Symbolizer #3\n");
	  *retcode += 42;
	  return 0;
      }
    if (rl2_text_symbolizer_get_fill_color (text, &red, &green, &blue) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get Text Symbolizer GetFillColor #3\n");
	  *retcode += 43;
	  return 0;
      }
    if (red != 0x60 || green != 0x72 || blue != 0x84)
      {
	  fprintf (stderr,
		   "Unexpected Text Symbolizer GetStrokeColor #3: %02x%02x%02x\n",
		   red, green, blue);
	  *retcode += 44;
	  return 0;
      }
    rl2_destroy_variant_array (value);

    intval = rl2_get_feature_type_style_columns_count (style);
    if (intval != 2)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnsCount #1: %d\n",
		   intval);
	  *retcode += 45;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 0);
    if (strcasecmp (string, "name") != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #1: \"%s\"\n",
		   string);
	  *retcode += 46;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 1);
    if (strcasecmp (string, "some_column") != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnName #2: \"%s\"\n",
		   string);
	  *retcode += 47;
	  return 0;
      }

    intval = rl2_get_feature_type_style_columns_count (NULL);
    if (intval != 0)
      {
	  fprintf (stderr,
		   "Unexpected GetFeatureTypeStyleColumnsCount #2: %d\n",
		   intval);
	  *retcode += 48;
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
    if (!test_symbolizer (db_handle, "points", "label_1", &ret))
	return ret;
    ret = -200;
    if (!test_symbolizer (db_handle, "lines", "label_2", &ret))
	return ret;
    ret = -300;
    if (!test_symbolizer (db_handle, "points", "label_3", &ret))
	return ret;
    ret = -400;
    if (!test_style (db_handle, "lines", "text_style", &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    return result;
}
