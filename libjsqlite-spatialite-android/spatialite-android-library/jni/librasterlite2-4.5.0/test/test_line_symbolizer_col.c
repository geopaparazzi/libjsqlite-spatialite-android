/*

 test_line_symbolizer_col.c -- RasterLite-2 Test Case

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
/* testing a LineSymbolizer */
    rl2FeatureTypeStylePtr style;
    rl2VectorSymbolizerPtr symbolizer;
    rl2LineSymbolizerPtr line;
    int intval;
    int index;
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

    string = rl2_line_symbolizer_get_col_stroke_color (line);
    if (strcmp (style_name, "line_2_col") == 0
	|| strcmp (style_name, "line_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Line Symbolizer GetColStrokeColor #1\n",
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
			 "%s: Unable to get Line Symbolizer GetColStrokeColor #1\n",
			 style_name);
		*retcode += 13;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "line_1_col") == 0)
      {
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetColStrokeColor #11: %s\n",
		   style_name, string);
	  *retcode += 13;
	  return 0;
      }

    string = rl2_line_symbolizer_get_col_stroke_opacity (line);
    if (strcmp (style_name, "line_1_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Line Symbolizer GetColStrokeOpacity #1\n",
			 style_name);
		*retcode += 14;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Line Symbolizer GetColStrokeOpacity #1\n",
			 style_name);
		*retcode += 14;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "line_2_col") == 0
	|| strcmp (style_name, "line_3_col") == 0)
      {
	  if (strcmp (string, "opacity") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetColStrokeOpacity #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 15;
	  return 0;
      }

    string = rl2_line_symbolizer_get_col_stroke_width (line);
    if (strcmp (style_name, "line_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected Success: Line Symbolizer GetColStrokeWidth #1\n",
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
			 "%s: Unable to get Line Symbolizer GetColStrokeWidth #1\n",
			 style_name);
		*retcode += 17;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_3_col") == 0)
      {
	  if (strcmp (string, "width") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetColStrokeWidth #1: %s\n",
		   style_name, string);
	  *retcode += 18;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_linejoin (line, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetColStrokeLinejoin #1\n",
		   style_name);
	  *retcode += 19;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1_col") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_UNKNOWN)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2_col") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_ROUND)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3_col") == 0)
      {
	  if (red == RL2_STROKE_LINEJOIN_MITRE)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetColStrokeLinejoin #1: %02x\n",
		   style_name, red);
	  *retcode += 20;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_linecap (line, &red) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeLinecap #1\n",
		   style_name);
	  *retcode += 21;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1_col") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_UNKNOWN)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2_col") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_SQUARE)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3_col") == 0)
      {
	  if (red == RL2_STROKE_LINECAP_ROUND)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeLinecap #1: %02x\n",
		   style_name, red);
	  *retcode += 22;
	  return 0;
      }

    if (rl2_line_symbolizer_get_stroke_dash_count (line, &intval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeDashCount #1\n",
		   style_name);
	  *retcode += 23;
	  return 0;
      }
    red = 0;
    if (strcmp (style_name, "line_1_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "line_2_col") == 0)
      {
	  if (intval == 6)
	      red = 1;
      }
    if (strcmp (style_name, "line_3_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeDashCount #1: %d\n",
		   style_name, intval);
	  *retcode += 24;
	  return 0;
      }

    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_3_col") == 0)
      {
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 0, &dblval) ==
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Expected failure, got success: Line Symbolizer GetStrokeDashItem #1\n");
		*retcode += 25;
		return 0;
	    }
      }
    if (strcmp (style_name, "line_2_col") == 0)
      {
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 0, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "%s: Unable to get Line Symbolizer GetStrokeDashItem #1\n",
			 style_name);
		*retcode += 26;
		return 0;
	    }
	  if (dblval != 20.0)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #1: %1.4f\n",
			 dblval);
		*retcode += 27;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 1, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #2\n");
		*retcode += 28;
		return 0;
	    }
	  if (dblval != 10.1)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #2: %1.4f\n",
			 dblval);
		*retcode += 29;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 2, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #3\n");
		*retcode += 30;
		return 0;
	    }
	  if (dblval != 5.5)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #3: %1.4f\n",
			 dblval);
		*retcode += 31;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 3, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #4\n");
		*retcode += 32;
		return 0;
	    }
	  if (dblval != 4.0)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #4: %1.4f\n",
			 dblval);
		*retcode += 33;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 4, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #5\n");
		*retcode += 34;
		return 0;
	    }
	  if (dblval != 6.0)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #4: %1.4f\n",
			 dblval);
		*retcode += 35;
		return 0;
	    }
	  if (rl2_line_symbolizer_get_stroke_dash_item (line, 5, &dblval) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to get Line Symbolizer GetStrokeDashItem #5\n");
		*retcode += 36;
		return 0;
	    }
	  if (dblval != 12.0)
	    {
		fprintf (stderr,
			 "Unexpected Line Symbolizer GetStrokDashItem #5: %1.4f\n",
			 dblval);
		*retcode += 37;
		return 0;
	    }
      }

    if (rl2_line_symbolizer_get_stroke_dash_offset (line, &dblval) != RL2_OK)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer GetStrokeDashOffset #1\n",
		   style_name);
	  *retcode += 38;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_3_col") == 0)
      {
	  if (dblval == 0.0)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2_col") == 0)
      {
	  if (dblval == 10.0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetStrokeDashOffset #1: %1.4f\n",
		   style_name, dblval);
	  *retcode += 39;
	  return 0;
      }

    string = rl2_line_symbolizer_get_col_perpendicular_offset (line);
    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_3_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Line Symbolizer GetColPerpendicularOffset #1\n",
			 style_name);
		*retcode += 40;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Line Symbolizer GetColPerpendicularOffset #1\n",
			 style_name);
		*retcode += 41;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "line_2_col") == 0)
      {
	  if (strcmp (string, "perpoff") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s; Unexpected Line Symbolizer GetColPerpendicularOffset #1: %s\n",
		   style_name, string);
	  *retcode += 42;
	  return 0;
      }

    string = rl2_line_symbolizer_get_col_graphic_stroke_href (line);
    intval = 0;
    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_2_col") == 0)
      {
	  if (string == NULL)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3_col") == 0)
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
		   "%s; Unexpected Line Symbolizer GetColGraphicStrokeHref #1: %s\n",
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
    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_2_col") == 0)
      {
	  if (intval == 0)
	      red = 1;
      }
    if (strcmp (style_name, "line_3_col") == 0)
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

    string =
	rl2_line_symbolizer_get_col_graphic_stroke_recode_color (line, 0,
								 &index);
    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success Line Symbolizer GetColGraphicStrokeRecodeColor #1\n",
			 style_name);
		*retcode += 46;
		return 0;
	    }
	  intval = 1;
      }
    if (strcmp (style_name, "line_3_col") == 0)
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Line Symbolizer GetColGraphicStrokeRecodeColor #1\n",
			 style_name);
		*retcode += 47;
		return 0;
	    }
	  intval = 0;
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer GetColGraphicStrokeRecodeColor #1: %s\n",
		   style_name, string);
	  *retcode += 48;
	  return 0;
      }

    intval = rl2_get_feature_type_style_columns_count (style);
    red = 0;
    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_2_col") == 0)
      {
	  if (intval == 2)
	      red = 1;
      }
    if (strcmp (style_name, "line_3_col") == 0)
      {
	  if (intval == 4)
	      red = 1;
      }
    if (red != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer StyleColumnsCount #1: %d\n",
		   style_name, intval);
	  *retcode += 49;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 0);
    if (string == NULL)
      {
	  fprintf (stderr,
		   "%s: Unable to get Line Symbolizer StyleColumnName #1\n",
		   style_name);
	  *retcode += 50;
	  return 0;
      }
    intval = 0;
    if (strcmp (style_name, "line_1_col") == 0)
      {
	  if (strcmp (string, "color") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "line_2_col") == 0)
      {
	  if (strcmp (string, "opacity") == 0)
	      intval = 1;
      }
    if (strcmp (style_name, "line_3_col") == 0)
      {
	  if (strcmp (string, "pen_href") == 0)
	      intval = 1;
      }
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer StyleColumnsName #1: %s\n",
		   style_name, string);
	  *retcode += 51;
	  return 0;
      }

    string = rl2_get_feature_type_style_column_name (style, 2);
    if (strcmp (style_name, "line_1_col") == 0
	|| strcmp (style_name, "line_2_col") == 0)
      {
	  if (string != NULL)
	    {
		fprintf (stderr,
			 "%s: Unexpected success: Line Symbolizer StyleColumnName #2\n",
			 style_name);
		*retcode += 52;
		return 0;
	    }
      }
    else
      {
	  if (string == NULL)
	    {
		fprintf (stderr,
			 "%s: Unable to get Line Symbolizer StyleColumnName #2\n",
			 style_name);
		*retcode += 53;
		return 0;
	    }
      }
    intval = 0;
    if (strcmp (style_name, "line_3_col") == 0)
      {
	  if (strcmp (string, "width") == 0)
	      intval = 1;
      }
    else
	intval = 1;
    if (intval != 1)
      {
	  fprintf (stderr,
		   "%s: Unexpected Line Symbolizer StyleColumnsName #2: %s\n",
		   style_name, string);
	  *retcode += 54;
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
    if (!test_symbolizer (db_handle, "line1", "line_1_col", &ret))
	return ret;
    ret = -200;
    if (!test_symbolizer (db_handle, "line2", "line_2_col", &ret))
	return ret;
    ret = -300;
    if (!test_symbolizer (db_handle, "line2", "line_3_col", &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_shutdown ();
    return result;
}
