/*

 rl2symclone -- private SQL helper methods

 version 0.1, 2015 January 18

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
#include <float.h>

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

RL2_PRIVATE void
rl2_add_dyn_symbolizer (rl2PrivVectorSymbolizerPtr dyn, int type,
			void *symbolizer)
{
/* adding an individual Symbolizer to a Dynamic Vector Symbolizer */
    rl2PrivVectorSymbolizerItemPtr item =
	malloc (sizeof (rl2PrivVectorSymbolizerItem));
    item->symbolizer_type = type;
    item->symbolizer = symbolizer;
    item->next = NULL;
    if (dyn->first == NULL)
	dyn->first = item;
    if (dyn->last != NULL)
	dyn->last->next = item;
    dyn->last = item;
}

static int
parse_hex_byte (const char hi, const char lo, unsigned char *value)
{
/* attempting to parse an hexadecimal byte */
    unsigned char hex;
    switch (hi)
      {
      case 'f':
      case 'F':
	  hex = 15 * 16;
	  break;
      case 'e':
      case 'E':
	  hex = 14 * 16;
	  break;
      case 'd':
      case 'D':
	  hex = 13 * 16;
	  break;
      case 'c':
      case 'C':
	  hex = 12 * 16;
	  break;
      case 'b':
      case 'B':
	  hex = 11 * 16;
	  break;
      case 'a':
      case 'A':
	  hex = 10 * 16;
	  break;
      case '9':
	  hex = 9 * 16;
	  break;
      case '8':
	  hex = 8 * 16;
	  break;
      case '7':
	  hex = 7 * 16;
	  break;
      case '6':
	  hex = 6 * 16;
	  break;
      case '5':
	  hex = 5 * 16;
	  break;
      case '4':
	  hex = 4 * 16;
	  break;
      case '3':
	  hex = 3 * 16;
	  break;
      case '2':
	  hex = 2 * 16;
	  break;
      case '1':
	  hex = 1 * 16;
	  break;
      case '0':
	  hex = 0;
	  break;
      default:
	  return 0;
      };
    switch (lo)
      {
      case 'f':
      case 'F':
	  hex += 15;
	  break;
      case 'e':
      case 'E':
	  hex += 14;
	  break;
      case 'd':
      case 'D':
	  hex += 13;
	  break;
      case 'c':
      case 'C':
	  hex += 12;
	  break;
      case 'b':
      case 'B':
	  hex += 11;
	  break;
      case 'a':
      case 'A':
	  hex += 10;
	  break;
      case '9':
	  hex += 9;
	  break;
      case '8':
	  hex += 8;
	  break;
      case '7':
	  hex += 7;
	  break;
      case '6':
	  hex += 6;
	  break;
      case '5':
	  hex = 5;
	  break;
      case '4':
	  hex += 4;
	  break;
      case '3':
	  hex += 3;
	  break;
      case '2':
	  hex += 2;
	  break;
      case '1':
	  hex += 1;
	  break;
      case '0':
	  hex += 0;
	  break;
      default:
	  return 0;
      };
    *value = hex;
    return 1;
}

static void
find_variant_color (rl2PrivVariantArrayPtr var, const char *column_name,
		    unsigned char *red, unsigned char *green,
		    unsigned char *blue)
{
/* attempting to parse and Hex Color (#rrggbb) from Variant Array */
    int i;
    for (i = 0; i < var->count; i++)
      {
	  rl2PrivVariantValuePtr variant = *(var->array + i);
	  if (strcasecmp (variant->column_name, column_name) == 0)
	    {
		if (variant->sqlite3_type == SQLITE_TEXT)
		  {
		      unsigned char r;
		      unsigned char g;
		      unsigned char b;
		      const char *value = (const char *) (variant->text_value);
		      if (strlen (value) != 7)
			  return;
		      if (*value != '#')
			  return;
		      if (!parse_hex_byte (*(value + 1), *(value + 2), &r))
			  return;
		      if (!parse_hex_byte (*(value + 3), *(value + 4), &g))
			  return;
		      if (!parse_hex_byte (*(value + 5), *(value + 6), &b))
			  return;
		      *red = r;
		      *green = g;
		      *blue = b;
		      return;
		  }
		return;
	    }
      }
}

static int
find_variant_double_value (rl2PrivVariantArrayPtr var, const char *column_name,
			   double *value)
{
/* attempting to retry a Double value from Variant Array */
    int i;
    for (i = 0; i < var->count; i++)
      {
	  rl2PrivVariantValuePtr variant = *(var->array + i);
	  if (strcasecmp (variant->column_name, column_name) == 0)
	    {
		if (variant->sqlite3_type == SQLITE_FLOAT)
		    *value = variant->dbl_value;
		if (variant->sqlite3_type == SQLITE_INTEGER)
		    *value = (double) (variant->int_value);
		return 1;
	    }
      }
    return 0;
}

static void
find_variant_text_value (rl2PrivVariantArrayPtr var, const char *column_name,
			 const char **value)
{
/* attempting to retry a Text value from Variant Array */
    int i;
    for (i = 0; i < var->count; i++)
      {
	  rl2PrivVariantValuePtr variant = *(var->array + i);
	  if (strcasecmp (variant->column_name, column_name) == 0)
	    {
		if (variant->sqlite3_type == SQLITE_TEXT)
		    *value = (const char *) (variant->text_value);
		return;
	    }
      }
}

static void
find_variant_href (rl2PrivVariantArrayPtr var, const char *column_name,
		   char **value)
{
/* attempting to retry a Text value from Variant Array */
    int i;
    for (i = 0; i < var->count; i++)
      {
	  rl2PrivVariantValuePtr variant = *(var->array + i);
	  if (strcasecmp (variant->column_name, column_name) == 0)
	    {
		if (variant->sqlite3_type == SQLITE_TEXT)
		  {
		      *value =
			  sqlite3_mprintf ("http://www.utopia.gov/%s",
					   (const char
					    *) (variant->text_value));
		  }
		return;
	    }
      }
}

RL2_PRIVATE void
rl2_set_point_symbolizer_dyn_values (rl2PrivVariantArrayPtr var,
				     rl2PrivPointSymbolizerPtr point)
{
/* replacing Point Symbolizer dynamic (column based) values */
    int index;
    int max;
    const char *str =
	rl2_point_symbolizer_get_col_opacity ((rl2PointSymbolizerPtr) point);
    if (str != NULL)
      {
	  double value = 1.0;
	  find_variant_double_value (var, str, &value);
	  point->graphic->opacity = value;
      }
    str = rl2_point_symbolizer_get_col_size ((rl2PointSymbolizerPtr) point);
    if (str != NULL)
      {
	  double value = 6.0;
	  find_variant_double_value (var, str, &value);
	  point->graphic->size = value;
      }
    str = rl2_point_symbolizer_get_col_rotation ((rl2PointSymbolizerPtr) point);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  point->graphic->rotation = value;
      }
    str =
	rl2_point_symbolizer_get_col_anchor_point_x ((rl2PointSymbolizerPtr)
						     point);
    if (str != NULL)
      {
	  double value = 0.5;
	  find_variant_double_value (var, str, &value);
	  point->graphic->anchor_point_x = value;
      }
    str =
	rl2_point_symbolizer_get_col_anchor_point_y ((rl2PointSymbolizerPtr)
						     point);
    if (str != NULL)
      {
	  double value = 0.5;
	  find_variant_double_value (var, str, &value);
	  point->graphic->anchor_point_y = value;
      }
    str =
	rl2_point_symbolizer_get_col_displacement_x ((rl2PointSymbolizerPtr)
						     point);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  point->graphic->displacement_x = value;
      }
    str =
	rl2_point_symbolizer_get_col_displacement_y ((rl2PointSymbolizerPtr)
						     point);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  point->graphic->displacement_y = value;
      }
    if (rl2_point_symbolizer_get_count ((rl2PointSymbolizerPtr) point, &max) !=
	RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int repl_index;
	  int max_repl;
	  str =
	      rl2_point_symbolizer_mark_get_col_well_known_type ((rl2PointSymbolizerPtr) point, index);
	  if (str != NULL)
	    {
		rl2PrivMarkPtr mark =
		    rl2_point_symbolizer_get_mark_ref ((rl2PointSymbolizerPtr)
						       point, index);
		if (mark != NULL)
		  {
		      const char *value = NULL;
		      find_variant_text_value (var, str, &value);
		      mark->well_known_type = RL2_GRAPHIC_MARK_SQUARE;
		      if (value != NULL)
			{
			    if (strcasecmp (value, "square") == 0)
				mark->well_known_type = RL2_GRAPHIC_MARK_SQUARE;
			    else if (strcasecmp (value, "circle") == 0)
				mark->well_known_type = RL2_GRAPHIC_MARK_CIRCLE;
			    else if (strcasecmp (value, "triangle") == 0)
				mark->well_known_type =
				    RL2_GRAPHIC_MARK_TRIANGLE;
			    else if (strcasecmp (value, "star") == 0)
				mark->well_known_type = RL2_GRAPHIC_MARK_STAR;
			    else if (strcasecmp (value, "cross") == 0)
				mark->well_known_type = RL2_GRAPHIC_MARK_CROSS;
			    else if (strcasecmp (value, "x") == 0)
				mark->well_known_type = RL2_GRAPHIC_MARK_X;
			}
		  }
	    }
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_color ((rl2PointSymbolizerPtr) point, index);
	  if (str != NULL)
	    {
		rl2PrivMarkPtr mark =
		    rl2_point_symbolizer_get_mark_ref ((rl2PointSymbolizerPtr)
						       point, index);
		if (mark != NULL)
		  {
		      unsigned char red = 0;
		      unsigned char green = 0;
		      unsigned char blue = 0;
		      find_variant_color (var, str, &red, &green, &blue);
		      mark->stroke->red = red;
		      mark->stroke->green = green;
		      mark->stroke->blue = blue;
		  }
	    }
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_width ((rl2PointSymbolizerPtr) point, index);
	  if (str != NULL)
	    {
		rl2PrivMarkPtr mark =
		    rl2_point_symbolizer_get_mark_ref ((rl2PointSymbolizerPtr)
						       point, index);
		if (mark != NULL)
		  {
		      double value = 0.0;
		      find_variant_double_value (var, str, &value);
		      mark->stroke->width = value;
		  }
	    }
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_linejoin ((rl2PointSymbolizerPtr) point, index);
	  if (str != NULL)
	    {
		rl2PrivMarkPtr mark =
		    rl2_point_symbolizer_get_mark_ref ((rl2PointSymbolizerPtr)
						       point, index);
		if (mark != NULL)
		  {
		      const char *value = NULL;
		      find_variant_text_value (var, str, &value);
		      mark->stroke->linejoin = RL2_STROKE_LINEJOIN_ROUND;
		      if (value != NULL)
			{
			    if (strcasecmp (value, "mitre") == 0)
				mark->stroke->linejoin =
				    RL2_STROKE_LINEJOIN_MITRE;
			    else if (strcasecmp (value, "round") == 0)
				mark->stroke->linejoin =
				    RL2_STROKE_LINEJOIN_ROUND;
			    else if (strcasecmp (value, "bevel") == 0)
				mark->stroke->linejoin =
				    RL2_STROKE_LINEJOIN_BEVEL;
			}
		  }
	    }
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_linecap ((rl2PointSymbolizerPtr) point, index);
	  if (str != NULL)
	    {
		rl2PrivMarkPtr mark =
		    rl2_point_symbolizer_get_mark_ref ((rl2PointSymbolizerPtr)
						       point, index);
		if (mark != NULL)
		  {
		      const char *value = NULL;
		      find_variant_text_value (var, str, &value);
		      mark->stroke->linejoin = RL2_STROKE_LINECAP_ROUND;
		      if (value != NULL)
			{
			    if (strcasecmp (value, "butt") == 0)
				mark->stroke->linejoin =
				    RL2_STROKE_LINECAP_BUTT;
			    else if (strcasecmp (value, "round") == 0)
				mark->stroke->linejoin =
				    RL2_STROKE_LINECAP_ROUND;
			    else if (strcasecmp (value, "square") == 0)
				mark->stroke->linejoin =
				    RL2_STROKE_LINECAP_SQUARE;
			}
		  }
	    }
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_dash_array ((rl2PointSymbolizerPtr) point, index);
	  if (str != NULL)
	    {
		rl2PrivMarkPtr mark =
		    rl2_point_symbolizer_get_mark_ref ((rl2PointSymbolizerPtr)
						       point, index);
		if (mark != NULL)
		  {
		      const char *value = NULL;
		      find_variant_text_value (var, str, &value);
		      if (value == NULL)
			{
			    if (mark->stroke->dash_list != NULL)
				free (mark->stroke->dash_list);
			    mark->stroke->dash_list = NULL;
			    mark->stroke->dash_count = 0;
			}
		      else
			{
			    int count = 0;
			    double *list = NULL;
			    if (parse_sld_se_stroke_dasharray
				(value, &count, &list))
			      {
				  if (mark->stroke->dash_list != NULL)
				      free (mark->stroke->dash_list);
				  mark->stroke->dash_list = list;
				  mark->stroke->dash_count = count;
			      }
			}
		  }
	    }
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_dash_offset ((rl2PointSymbolizerPtr) point, index);
	  if (str != NULL)
	    {
		rl2PrivMarkPtr mark =
		    rl2_point_symbolizer_get_mark_ref ((rl2PointSymbolizerPtr)
						       point, index);
		if (mark != NULL)
		  {
		      double value = 0.0;
		      find_variant_double_value (var, str, &value);
		      mark->stroke->dash_offset = value;
		  }
	    }
	  str =
	      rl2_point_symbolizer_mark_get_col_fill_color ((rl2PointSymbolizerPtr) point, index);
	  if (str != NULL)
	    {
		rl2PrivMarkPtr mark =
		    rl2_point_symbolizer_get_mark_ref ((rl2PointSymbolizerPtr)
						       point, index);
		if (mark != NULL)
		  {
		      unsigned char red = 128;
		      unsigned char green = 128;
		      unsigned char blue = 128;
		      find_variant_color (var, str, &red, &green, &blue);
		      mark->fill->red = red;
		      mark->fill->green = green;
		      mark->fill->blue = blue;
		  }
	    }
	  str =
	      rl2_point_symbolizer_get_col_graphic_href ((rl2PointSymbolizerPtr)
							 point, index);
	  if (str != NULL)
	    {
		rl2PrivExternalGraphicPtr ext =
		    rl2_point_symbolizer_get_external_graphic_ref ((rl2PointSymbolizerPtr) point, index);
		if (ext != NULL)
		  {
		      char *value = NULL;
		      find_variant_href (var, str, &value);
		      if (value != NULL)
			{
			    int len = strlen (value);
			    ext->xlink_href = malloc (len + 1);
			    strcpy (ext->xlink_href, value);
			    sqlite3_free (value);
			}
		  }
	    }
	  if (rl2_point_symbolizer_get_graphic_recode_count
	      ((rl2PointSymbolizerPtr) point, index, &max_repl) != RL2_OK)
	      max_repl = 0;
	  for (repl_index = 0; repl_index < max_repl; repl_index++)
	    {
		int color_index;
		str =
		    rl2_point_symbolizer_get_col_graphic_recode_color
		    ((rl2PointSymbolizerPtr) point, index, repl_index,
		     &color_index);
		if (str != NULL)
		  {
		      rl2PrivColorReplacementPtr repl =
			  rl2_point_symbolizer_get_color_replacement_ref ((rl2PointSymbolizerPtr) point, index, repl_index, &color_index);
		      if (repl != NULL)
			{
			    unsigned char red = 128;
			    unsigned char green = 128;
			    unsigned char blue = 128;
			    find_variant_color (var, str, &red, &green, &blue);
			    repl->index = color_index;
			    repl->red = red;
			    repl->green = green;
			    repl->blue = blue;
			}
		  }
	    }
      }
}

RL2_PRIVATE void
rl2_set_line_symbolizer_dyn_values (rl2PrivVariantArrayPtr var,
				    rl2PrivLineSymbolizerPtr line)
{
/* replacing Line Symbolizer dynamic (column based) values */
    int index;
    int max;
    const char *str =
	rl2_line_symbolizer_get_col_graphic_stroke_href ((rl2LineSymbolizerPtr)
							 line);
    if (str != NULL)
      {
	  rl2PrivExternalGraphicPtr ext =
	      rl2_line_symbolizer_get_stroke_external_graphic_ref ((rl2LineSymbolizerPtr) line);
	  if (ext != NULL)
	    {
		char *value = NULL;
		find_variant_href (var, str, &value);
		if (value != NULL)
		  {
		      int len = strlen (value);
		      ext->xlink_href = malloc (len + 1);
		      strcpy (ext->xlink_href, value);
		      sqlite3_free (value);
		  }
	    }
      }
    if (rl2_line_symbolizer_get_graphic_stroke_recode_count
	((rl2LineSymbolizerPtr) line, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  str =
	      rl2_line_symbolizer_get_col_graphic_stroke_recode_color ((rl2LineSymbolizerPtr) line, index, &color_index);
	  if (str != NULL)
	    {
		rl2PrivColorReplacementPtr repl =
		    rl2_line_symbolizer_get_stroke_color_replacement_ref ((rl2LineSymbolizerPtr) line, index, &color_index);
		if (repl != NULL)
		  {
		      unsigned char red = 128;
		      unsigned char green = 128;
		      unsigned char blue = 128;
		      find_variant_color (var, str, &red, &green, &blue);
		      repl->index = color_index;
		      repl->red = red;
		      repl->green = green;
		      repl->blue = blue;
		  }
	    }
      }
    str =
	rl2_line_symbolizer_get_col_stroke_color ((rl2LineSymbolizerPtr) line);
    if (str != NULL)
      {
	  unsigned char red = 0;
	  unsigned char green = 0;
	  unsigned char blue = 0;
	  find_variant_color (var, str, &red, &green, &blue);
	  line->stroke->red = red;
	  line->stroke->green = green;
	  line->stroke->blue = blue;
      }
    str =
	rl2_line_symbolizer_get_col_stroke_opacity ((rl2LineSymbolizerPtr)
						    line);
    if (str != NULL)
      {
	  double value = 1.0;
	  find_variant_double_value (var, str, &value);
	  line->stroke->opacity = value;
      }
    str =
	rl2_line_symbolizer_get_col_stroke_width ((rl2LineSymbolizerPtr) line);
    if (str != NULL)
      {
	  double value = 1.0;
	  find_variant_double_value (var, str, &value);
	  line->stroke->width = value;
      }
    str =
	rl2_line_symbolizer_get_col_stroke_linejoin ((rl2LineSymbolizerPtr)
						     line);
    if (str != NULL)
      {
	  const char *value = NULL;
	  find_variant_text_value (var, str, &value);
	  line->stroke->linejoin = RL2_STROKE_LINEJOIN_ROUND;
	  if (value != NULL)
	    {
		if (strcasecmp (value, "mitre") == 0)
		    line->stroke->linejoin = RL2_STROKE_LINEJOIN_MITRE;
		else if (strcasecmp (value, "round") == 0)
		    line->stroke->linejoin = RL2_STROKE_LINEJOIN_ROUND;
		else if (strcasecmp (value, "bevel") == 0)
		    line->stroke->linejoin = RL2_STROKE_LINEJOIN_BEVEL;
	    }
      }
    str =
	rl2_line_symbolizer_get_col_stroke_linecap ((rl2LineSymbolizerPtr)
						    line);
    if (str != NULL)
      {
	  const char *value = NULL;
	  find_variant_text_value (var, str, &value);
	  line->stroke->linejoin = RL2_STROKE_LINECAP_ROUND;
	  if (value != NULL)
	    {
		if (strcasecmp (value, "butt") == 0)
		    line->stroke->linejoin = RL2_STROKE_LINECAP_BUTT;
		else if (strcasecmp (value, "round") == 0)
		    line->stroke->linejoin = RL2_STROKE_LINECAP_ROUND;
		else if (strcasecmp (value, "square") == 0)
		    line->stroke->linejoin = RL2_STROKE_LINECAP_SQUARE;
	    }
      }
    str =
	rl2_line_symbolizer_get_col_stroke_dash_array ((rl2LineSymbolizerPtr)
						       line);
    if (str != NULL)
      {
	  const char *value = NULL;
	  find_variant_text_value (var, str, &value);
	  if (value == NULL)
	    {
		if (line->stroke->dash_list != NULL)
		    free (line->stroke->dash_list);
		line->stroke->dash_list = NULL;
		line->stroke->dash_count = 0;
	    }
	  else
	    {
		int count = 0;
		double *list = NULL;
		if (parse_sld_se_stroke_dasharray (value, &count, &list))
		  {
		      if (line->stroke->dash_list != NULL)
			  free (line->stroke->dash_list);
		      line->stroke->dash_list = list;
		      line->stroke->dash_count = count;
		  }
	    }
      }
    str =
	rl2_line_symbolizer_get_col_stroke_dash_offset ((rl2LineSymbolizerPtr)
							line);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  line->stroke->dash_offset = value;
      }
    str =
	rl2_line_symbolizer_get_col_perpendicular_offset ((rl2LineSymbolizerPtr)
							  line);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  line->perpendicular_offset = value;
      }
}

RL2_PRIVATE void
rl2_set_polygon_symbolizer_dyn_values (rl2PrivVariantArrayPtr var,
				       rl2PrivPolygonSymbolizerPtr polyg)
{
/* replacing Line Symbolizer dynamic (column based) values */
    int index;
    int max;
    const char *str =
	rl2_polygon_symbolizer_get_col_graphic_stroke_href ((rl2PolygonSymbolizerPtr) polyg);
    if (str != NULL)
      {
	  rl2PrivExternalGraphicPtr ext =
	      rl2_polygon_symbolizer_get_stroke_external_graphic_ref ((rl2PolygonSymbolizerPtr) polyg);
	  if (ext != NULL)
	    {
		char *value = NULL;
		find_variant_href (var, str, &value);
		if (value != NULL)
		  {
		      int len = strlen (value);
		      ext->xlink_href = malloc (len + 1);
		      strcpy (ext->xlink_href, value);
		      sqlite3_free (value);
		  }
	    }
      }
    if (rl2_polygon_symbolizer_get_graphic_stroke_recode_count
	((rl2PolygonSymbolizerPtr) polyg, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  str =
	      rl2_polygon_symbolizer_get_col_graphic_stroke_recode_color ((rl2PolygonSymbolizerPtr) polyg, index, &color_index);
	  if (str != NULL)
	    {
		rl2PrivColorReplacementPtr repl =
		    rl2_polygon_symbolizer_get_stroke_color_replacement_ref ((rl2PolygonSymbolizerPtr) polyg, index, &color_index);
		if (repl != NULL)
		  {
		      unsigned char red = 128;
		      unsigned char green = 128;
		      unsigned char blue = 128;
		      find_variant_color (var, str, &red, &green, &blue);
		      repl->index = color_index;
		      repl->red = red;
		      repl->green = green;
		      repl->blue = blue;
		  }
	    }
      }
    str =
	rl2_polygon_symbolizer_get_col_stroke_color ((rl2PolygonSymbolizerPtr)
						     polyg);
    if (str != NULL)
      {
	  unsigned char red = 0;
	  unsigned char green = 0;
	  unsigned char blue = 0;
	  find_variant_color (var, str, &red, &green, &blue);
	  polyg->stroke->red = red;
	  polyg->stroke->green = green;
	  polyg->stroke->blue = blue;
      }
    str =
	rl2_polygon_symbolizer_get_col_stroke_opacity ((rl2PolygonSymbolizerPtr)
						       polyg);
    if (str != NULL)
      {
	  double value = 1.0;
	  find_variant_double_value (var, str, &value);
	  polyg->stroke->opacity = value;
      }
    str =
	rl2_polygon_symbolizer_get_col_stroke_width ((rl2PolygonSymbolizerPtr)
						     polyg);
    if (str != NULL)
      {
	  double value = 1.0;
	  find_variant_double_value (var, str, &value);
	  polyg->stroke->width = value;
      }
    str =
	rl2_polygon_symbolizer_get_col_stroke_linejoin ((rl2PolygonSymbolizerPtr) polyg);
    if (str != NULL)
      {
	  const char *value = NULL;
	  find_variant_text_value (var, str, &value);
	  polyg->stroke->linejoin = RL2_STROKE_LINEJOIN_ROUND;
	  if (value != NULL)
	    {
		if (strcasecmp (value, "mitre") == 0)
		    polyg->stroke->linejoin = RL2_STROKE_LINEJOIN_MITRE;
		else if (strcasecmp (value, "round") == 0)
		    polyg->stroke->linejoin = RL2_STROKE_LINEJOIN_ROUND;
		else if (strcasecmp (value, "bevel") == 0)
		    polyg->stroke->linejoin = RL2_STROKE_LINEJOIN_BEVEL;
	    }
      }
    str =
	rl2_polygon_symbolizer_get_col_stroke_linecap ((rl2PolygonSymbolizerPtr)
						       polyg);
    if (str != NULL)
      {
	  const char *value = NULL;
	  find_variant_text_value (var, str, &value);
	  polyg->stroke->linejoin = RL2_STROKE_LINECAP_ROUND;
	  if (value != NULL)
	    {
		if (strcasecmp (value, "butt") == 0)
		    polyg->stroke->linejoin = RL2_STROKE_LINECAP_BUTT;
		else if (strcasecmp (value, "round") == 0)
		    polyg->stroke->linejoin = RL2_STROKE_LINECAP_ROUND;
		else if (strcasecmp (value, "square") == 0)
		    polyg->stroke->linejoin = RL2_STROKE_LINECAP_SQUARE;
	    }
      }
    str =
	rl2_polygon_symbolizer_get_col_stroke_dash_array ((rl2PolygonSymbolizerPtr) polyg);
    if (str != NULL)
      {
	  const char *value = NULL;
	  find_variant_text_value (var, str, &value);
	  if (value == NULL)
	    {
		if (polyg->stroke->dash_list != NULL)
		    free (polyg->stroke->dash_list);
		polyg->stroke->dash_list = NULL;
		polyg->stroke->dash_count = 0;
	    }
	  else
	    {
		int count = 0;
		double *list = NULL;
		if (parse_sld_se_stroke_dasharray (value, &count, &list))
		  {
		      if (polyg->stroke->dash_list != NULL)
			  free (polyg->stroke->dash_list);
		      polyg->stroke->dash_list = list;
		      polyg->stroke->dash_count = count;
		  }
	    }
      }
    str =
	rl2_polygon_symbolizer_get_col_stroke_dash_offset ((rl2PolygonSymbolizerPtr) polyg);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  polyg->stroke->dash_offset = value;
      }
    str =
	rl2_polygon_symbolizer_get_col_graphic_fill_href ((rl2PolygonSymbolizerPtr) polyg);
    if (str != NULL)
      {
	  rl2PrivExternalGraphicPtr ext =
	      rl2_polygon_symbolizer_get_fill_external_graphic_ref ((rl2PolygonSymbolizerPtr) polyg);
	  if (ext != NULL)
	    {
		char *value = NULL;
		find_variant_href (var, str, &value);
		if (value != NULL)
		  {
		      int len = strlen (value);
		      ext->xlink_href = malloc (len + 1);
		      strcpy (ext->xlink_href, value);
		      sqlite3_free (value);
		  }
	    }
      }
    if (rl2_polygon_symbolizer_get_graphic_fill_recode_count
	((rl2PolygonSymbolizerPtr) polyg, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  str =
	      rl2_polygon_symbolizer_get_col_graphic_fill_recode_color ((rl2PolygonSymbolizerPtr) polyg, index, &color_index);
	  if (str != NULL)
	    {
		rl2PrivColorReplacementPtr repl =
		    rl2_polygon_symbolizer_get_fill_color_replacement_ref ((rl2PolygonSymbolizerPtr) polyg, index, &color_index);
		if (repl != NULL)
		  {
		      unsigned char red = 128;
		      unsigned char green = 128;
		      unsigned char blue = 128;
		      find_variant_color (var, str, &red, &green, &blue);
		      repl->index = color_index;
		      repl->red = red;
		      repl->green = green;
		      repl->blue = blue;
		  }
	    }
      }
    str =
	rl2_polygon_symbolizer_get_col_fill_color ((rl2PolygonSymbolizerPtr)
						   polyg);
    if (str != NULL)
      {
	  unsigned char red = 128;
	  unsigned char green = 128;
	  unsigned char blue = 128;
	  find_variant_color (var, str, &red, &green, &blue);
	  polyg->fill->red = red;
	  polyg->fill->green = green;
	  polyg->fill->blue = blue;
      }
    str =
	rl2_polygon_symbolizer_get_col_fill_opacity ((rl2PolygonSymbolizerPtr)
						     polyg);
    if (str != NULL)
      {
	  double value = 1.0;
	  find_variant_double_value (var, str, &value);
	  polyg->fill->opacity = value;
      }
    str =
	rl2_polygon_symbolizer_get_col_perpendicular_offset ((rl2PolygonSymbolizerPtr) polyg);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  polyg->perpendicular_offset = value;
      }
    str =
	rl2_polygon_symbolizer_get_col_displacement_x ((rl2PolygonSymbolizerPtr)
						       polyg);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  polyg->displacement_x = value;
      }
    str =
	rl2_polygon_symbolizer_get_col_displacement_y ((rl2PolygonSymbolizerPtr)
						       polyg);
    if (str != NULL)
      {
	  double value = 0.0;
	  find_variant_double_value (var, str, &value);
	  polyg->displacement_y = value;
      }
}

RL2_PRIVATE void
rl2_set_text_symbolizer_dyn_values (rl2PrivVariantArrayPtr var,
				    rl2PrivTextSymbolizerPtr text)
{
/* replacing Text Symbolizer dynamic (column based) values */
    const char *str =
	rl2_text_symbolizer_get_col_label ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  const char *value = NULL;
	  find_variant_text_value (var, str, &value);
	  if (value == NULL)
	    {
		double value = 0.0;
		if (!find_variant_double_value (var, str, &value))
		    text->label = NULL;
		else
		  {
		      /* transforming a Double or Int value into a text string */
		      int len;
		      int i;
		      char *txt = sqlite3_mprintf ("%f", value);
		      for (i = strlen (txt) - 1; i >= 0; i--)
			{
			    if (txt[i] == '0')
				txt[i] = '\0';
			    else
				break;
			}
		      i = strlen (txt) - 1;
		      if (txt[i] == '.')
			  txt[i] = '\0';
		      len = strlen (txt);
		      text->label = malloc (len + 1);
		      strcpy (text->label, txt);
		      sqlite3_free (txt);
		  }
	    }
	  else
	    {
		int len = strlen (value);
		text->label = malloc (len + 1);
		strcpy (text->label, value);
	    }
      }
    str = rl2_text_symbolizer_get_col_font ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  int i;
	  int len;
	  const char *value = NULL;
	  find_variant_text_value (var, str, &value);
	  if (value != NULL)
	    {
		for (i = 0; i < RL2_MAX_FONT_FAMILIES; i++)
		  {
		      if (*(text->font_families + i) != NULL)
			  free (*(text->font_families + i));
		      *(text->font_families + i) = NULL;
		  }
		text->font_families_count = 1;
		len = strlen (value);
		*(text->font_families + 0) = malloc (len + 1);
		strcpy (*(text->font_families + 0), value);
	    }
      }
    str = rl2_text_symbolizer_get_col_style ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  const char *value = "normal";
	  find_variant_text_value (var, str, &value);
	  if (strcasecmp (value, "normal") == 0)
	      text->font_style = RL2_FONT_STYLE_NORMAL;
	  else if (strcasecmp (value, "italic") == 0)
	      text->font_style = RL2_FONT_STYLE_ITALIC;
	  else if (strcasecmp (value, "oblique") == 0)
	      text->font_style = RL2_FONT_STYLE_OBLIQUE;
	  else
	      text->font_style = RL2_FONT_STYLE_NORMAL;
      }
    str = rl2_text_symbolizer_get_col_weight ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  const char *value = "normal";
	  find_variant_text_value (var, str, &value);
	  if (strcasecmp (value, "normal") == 0)
	      text->font_weight = RL2_FONT_WEIGHT_NORMAL;
	  else if (strcasecmp (value, "bold") == 0)
	      text->font_weight = RL2_FONT_WEIGHT_BOLD;
	  else
	      text->font_weight = RL2_FONT_WEIGHT_NORMAL;
      }
    str = rl2_text_symbolizer_get_col_size ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 10.0;
	  find_variant_double_value (var, str, &value);
	  text->font_size = value;
      }
    str =
	rl2_text_symbolizer_get_point_placement_col_anchor_point_x ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 0.5;
	  rl2PrivPointPlacementPtr pl = text->label_placement;
	  find_variant_double_value (var, str, &value);
	  pl->anchor_point_x = value;
      }
    str =
	rl2_text_symbolizer_get_point_placement_col_anchor_point_y ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 0.5;
	  rl2PrivPointPlacementPtr pl = text->label_placement;
	  find_variant_double_value (var, str, &value);
	  pl->anchor_point_y = value;
      }
    str =
	rl2_text_symbolizer_get_point_placement_col_displacement_x ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 0.0;
	  rl2PrivPointPlacementPtr pl = text->label_placement;
	  find_variant_double_value (var, str, &value);
	  pl->displacement_x = value;
      }
    str =
	rl2_text_symbolizer_get_point_placement_col_displacement_y ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 0.0;
	  rl2PrivPointPlacementPtr pl = text->label_placement;
	  find_variant_double_value (var, str, &value);
	  pl->displacement_y = value;
      }
    str =
	rl2_text_symbolizer_get_point_placement_col_rotation ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 0.0;
	  rl2PrivPointPlacementPtr pl = text->label_placement;
	  find_variant_double_value (var, str, &value);
	  pl->rotation = value;
      }
    str =
	rl2_text_symbolizer_get_line_placement_col_perpendicular_offset ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 0.0;
	  rl2PrivLinePlacementPtr pl = text->label_placement;
	  find_variant_double_value (var, str, &value);
	  pl->perpendicular_offset = value;
      }
    str =
	rl2_text_symbolizer_get_line_placement_col_initial_gap ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 0.0;
	  rl2PrivLinePlacementPtr pl = text->label_placement;
	  find_variant_double_value (var, str, &value);
	  pl->initial_gap = value;
      }
    str =
	rl2_text_symbolizer_get_line_placement_col_gap ((rl2TextSymbolizerPtr)
							text);
    if (str != NULL)
      {
	  double value = 0.0;
	  rl2PrivLinePlacementPtr pl = text->label_placement;
	  find_variant_double_value (var, str, &value);
	  pl->gap = value;
      }
    str = rl2_text_symbolizer_get_col_fill_color ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  unsigned char red = 0;
	  unsigned char green = 0;
	  unsigned char blue = 0;
	  find_variant_color (var, str, &red, &green, &blue);
	  text->fill->red = red;
	  text->fill->green = green;
	  text->fill->blue = blue;
      }
    str =
	rl2_text_symbolizer_get_col_fill_opacity ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 1.0;;
	  find_variant_double_value (var, str, &value);
	  text->fill->opacity = value;
      }
    str = rl2_text_symbolizer_get_halo_col_radius ((rl2TextSymbolizerPtr) text);
    if (str != NULL)
      {
	  double value = 1.0;
	  find_variant_double_value (var, str, &value);
	  text->halo->radius = value;
      }
    str =
	rl2_text_symbolizer_get_halo_col_fill_color ((rl2TextSymbolizerPtr)
						     text);
    if (str != NULL)
      {
	  unsigned char red = 255;
	  unsigned char green = 255;
	  unsigned char blue = 255;
	  find_variant_color (var, str, &red, &green, &blue);
	  text->halo->fill->red = red;
	  text->halo->fill->green = green;
	  text->halo->fill->blue = blue;
      }
    str =
	rl2_text_symbolizer_get_halo_col_fill_opacity ((rl2TextSymbolizerPtr)
						       text);
    if (str != NULL)
      {
	  double value = 1.0;
	  find_variant_double_value (var, str, &value);
	  text->halo->fill->opacity = value;
      }
}

RL2_PRIVATE rl2PrivPointSymbolizerPtr
rl2_clone_point_symbolizer (rl2PrivPointSymbolizerPtr in)
{
/* cloning a Point Symbolizer */
    int len;
    rl2PrivPointSymbolizerPtr out;
    rl2PrivGraphicItemPtr item_in;

    if (in == NULL)
	return NULL;
    if (in->graphic == NULL)
	return NULL;
    out = malloc (sizeof (rl2PrivPointSymbolizer));
    if (out == NULL)
	return NULL;
    out->graphic = rl2_create_default_graphic ();
    if (out->graphic == NULL)
      {
	  free (out);
	  return NULL;
      }
    out->graphic->first = NULL;
    out->graphic->last = NULL;
    out->graphic->opacity = in->graphic->opacity;
    out->graphic->size = in->graphic->size;
    out->graphic->rotation = in->graphic->rotation;
    out->graphic->anchor_point_x = in->graphic->anchor_point_x;
    out->graphic->anchor_point_y = in->graphic->anchor_point_y;
    out->graphic->displacement_x = in->graphic->displacement_x;
    out->graphic->displacement_y = in->graphic->displacement_y;
    out->graphic->col_opacity = NULL;
    if (in->graphic->col_opacity != NULL)
      {
	  len = strlen (in->graphic->col_opacity);
	  out->graphic->col_opacity = malloc (len + 1);
	  strcpy (out->graphic->col_opacity, in->graphic->col_opacity);
      }
    out->graphic->col_size = NULL;
    if (in->graphic->col_size != NULL)
      {
	  len = strlen (in->graphic->col_size);
	  out->graphic->col_size = malloc (len + 1);
	  strcpy (out->graphic->col_size, in->graphic->col_size);
      }
    out->graphic->col_rotation = NULL;
    if (in->graphic->col_rotation != NULL)
      {
	  len = strlen (in->graphic->col_rotation);
	  out->graphic->col_rotation = malloc (len + 1);
	  strcpy (out->graphic->col_rotation, in->graphic->col_rotation);
      }
    out->graphic->col_point_x = NULL;
    if (in->graphic->col_point_x != NULL)
      {
	  len = strlen (in->graphic->col_point_x);
	  out->graphic->col_point_x = malloc (len + 1);
	  strcpy (out->graphic->col_point_x, in->graphic->col_point_x);
      }
    out->graphic->col_point_y = NULL;
    if (in->graphic->col_point_y != NULL)
      {
	  len = strlen (in->graphic->col_point_y);
	  out->graphic->col_point_y = malloc (len + 1);
	  strcpy (out->graphic->col_point_y, in->graphic->col_point_y);
      }
    out->graphic->col_displ_x = NULL;
    if (in->graphic->col_displ_x != NULL)
      {
	  len = strlen (in->graphic->col_displ_x);
	  out->graphic->col_displ_x = malloc (len + 1);
	  strcpy (out->graphic->col_displ_x, in->graphic->col_displ_x);
      }
    out->graphic->col_displ_y = NULL;
    if (in->graphic->col_displ_y != NULL)
      {
	  len = strlen (in->graphic->col_displ_y);
	  out->graphic->col_displ_y = malloc (len + 1);
	  strcpy (out->graphic->col_displ_y, in->graphic->col_displ_y);
      }
    item_in = in->graphic->first;
    while (item_in != NULL)
      {
	  rl2PrivGraphicItemPtr item_out = NULL;
	  if (item_in->type == RL2_EXTERNAL_GRAPHIC)
	    {
		rl2PrivColorReplacementPtr repl_in;
		rl2PrivColorReplacementPtr repl_out;
		rl2PrivExternalGraphicPtr ext_in = item_in->item;
		rl2PrivExternalGraphicPtr ext_out =
		    malloc (sizeof (rl2PrivExternalGraphic));
		item_out = malloc (sizeof (rl2PrivGraphicItem));
		ext_out->xlink_href = NULL;
		if (ext_in->xlink_href != NULL)
		  {
		      len = strlen (ext_in->xlink_href);
		      ext_out->xlink_href = malloc (len + 1);
		      strcpy (ext_out->xlink_href, ext_in->xlink_href);
		  }
		ext_out->col_href = NULL;
		if (ext_in->col_href != NULL)
		  {
		      len = strlen (ext_in->col_href);
		      ext_out->col_href = malloc (len + 1);
		      strcpy (ext_out->col_href, ext_in->col_href);
		  }
		ext_out->first = NULL;
		ext_out->last = NULL;
		repl_in = ext_in->first;
		while (repl_in != NULL)
		  {
		      repl_out = malloc (sizeof (rl2PrivColorReplacement));
		      repl_out->index = repl_in->index;
		      repl_out->red = repl_in->red;
		      repl_out->green = repl_in->green;
		      repl_out->blue = repl_in->blue;
		      repl_out->col_color = NULL;
		      if (repl_in->col_color != NULL)
			{
			    len = strlen (repl_in->col_color);
			    repl_out->col_color = malloc (len + 1);
			    strcpy (repl_out->col_color, repl_in->col_color);
			}
		      repl_out->next = NULL;
		      if (ext_out->first == NULL)
			  ext_out->first = repl_out;
		      if (ext_out->last != NULL)
			  ext_out->last->next = repl_out;
		      ext_out->last = repl_out;
		      repl_in = repl_in->next;
		  }
		item_out->type = RL2_EXTERNAL_GRAPHIC;
		item_out->item = ext_out;
		item_out->next = NULL;
	    }
	  if (item_in->type == RL2_MARK_GRAPHIC)
	    {
		rl2PrivMarkPtr mark_in = item_in->item;
		rl2PrivMarkPtr mark_out = malloc (sizeof (rl2PrivMark));
		item_out = malloc (sizeof (rl2PrivGraphicItem));
		mark_out->well_known_type = mark_in->well_known_type;
		mark_out->stroke = NULL;
		if (mark_in->stroke != NULL)
		  {
		      rl2PrivStrokePtr stroke_in = mark_in->stroke;
		      rl2PrivStrokePtr stroke_out =
			  malloc (sizeof (rl2PrivStroke));
		      stroke_out->graphic = NULL;
		      stroke_out->red = stroke_in->red;
		      stroke_out->green = stroke_in->green;
		      stroke_out->blue = stroke_in->blue;
		      stroke_out->opacity = stroke_in->opacity;
		      stroke_out->width = stroke_in->width;
		      stroke_out->linejoin = stroke_in->linejoin;
		      stroke_out->linecap = stroke_in->linecap;
		      stroke_out->dash_count = stroke_in->dash_count;
		      stroke_out->dash_list = NULL;
		      if (stroke_in->dash_count > 0)
			{
			    int i;
			    stroke_out->dash_list =
				malloc (sizeof (double) *
					stroke_out->dash_count);
			    for (i = 0; i < stroke_out->dash_count; i++)
				*(stroke_out->dash_list + i) =
				    *(stroke_in->dash_list + i);
			}
		      stroke_out->dash_offset = stroke_in->dash_offset;
		      stroke_out->col_color = NULL;
		      if (stroke_in->col_color != NULL)
			{
			    len = strlen (stroke_in->col_color);
			    stroke_out->col_color = malloc (len + 1);
			    strcpy (stroke_out->col_color,
				    stroke_in->col_color);
			}
		      stroke_out->col_opacity = NULL;
		      if (stroke_in->col_opacity != NULL)
			{
			    len = strlen (stroke_in->col_opacity);
			    stroke_out->col_opacity = malloc (len + 1);
			    strcpy (stroke_out->col_opacity,
				    stroke_in->col_opacity);
			}
		      stroke_out->col_width = NULL;
		      if (stroke_in->col_width != NULL)
			{
			    len = strlen (stroke_in->col_width);
			    stroke_out->col_width = malloc (len + 1);
			    strcpy (stroke_out->col_width,
				    stroke_in->col_width);
			}
		      stroke_out->col_join = NULL;
		      if (stroke_in->col_join != NULL)
			{
			    len = strlen (stroke_in->col_join);
			    stroke_out->col_join = malloc (len + 1);
			    strcpy (stroke_out->col_join, stroke_in->col_join);
			}
		      stroke_out->col_cap = NULL;
		      if (stroke_in->col_cap != NULL)
			{
			    len = strlen (stroke_in->col_cap);
			    stroke_out->col_cap = malloc (len + 1);
			    strcpy (stroke_out->col_cap, stroke_in->col_cap);
			}
		      stroke_out->col_dash = NULL;
		      if (stroke_in->col_dash != NULL)
			{
			    len = strlen (stroke_in->col_dash);
			    stroke_out->col_dash = malloc (len + 1);
			    strcpy (stroke_out->col_dash, stroke_in->col_dash);
			}
		      stroke_out->col_dashoff = NULL;
		      if (stroke_in->col_dashoff != NULL)
			{
			    len = strlen (stroke_in->col_dashoff);
			    stroke_out->col_dashoff = malloc (len + 1);
			    strcpy (stroke_out->col_dashoff,
				    stroke_in->col_dashoff);
			}
		      mark_out->stroke = stroke_out;
		  }
		mark_out->fill = NULL;
		if (mark_in->fill != NULL)
		  {
		      rl2PrivFillPtr fill_in = mark_in->fill;
		      rl2PrivFillPtr fill_out = malloc (sizeof (rl2PrivFill));
		      fill_out->graphic = NULL;
		      fill_out->red = fill_in->red;
		      fill_out->green = fill_in->green;
		      fill_out->blue = fill_in->blue;
		      fill_out->opacity = fill_in->opacity;
		      fill_out->col_color = NULL;
		      if (fill_in->col_color != NULL)
			{
			    len = strlen (fill_in->col_color);
			    fill_out->col_color = malloc (len + 1);
			    strcpy (fill_out->col_color, fill_in->col_color);
			}
		      fill_out->col_opacity = NULL;
		      if (fill_in->col_opacity != NULL)
			{
			    len = strlen (fill_in->col_opacity);
			    fill_out->col_opacity = malloc (len + 1);
			    strcpy (fill_out->col_opacity,
				    fill_in->col_opacity);
			}
		      mark_out->fill = fill_out;
		  }
		mark_out->col_mark_type = NULL;
		if (mark_in->col_mark_type != NULL)
		  {
		      len = strlen (mark_in->col_mark_type);
		      mark_out->col_mark_type = malloc (len + 1);
		      strcpy (mark_out->col_mark_type, mark_in->col_mark_type);
		  }
		item_out->type = RL2_MARK_GRAPHIC;
		item_out->item = mark_out;
		item_out->next = NULL;
	    }
	  if (item_out != NULL)
	    {
		if (out->graphic->first == NULL)
		    out->graphic->first = item_out;
		if (out->graphic->last != NULL)
		    out->graphic->last->next = item_out;
		out->graphic->last = item_out;
	    }
	  item_in = item_in->next;
      }
    return out;
}

RL2_PRIVATE rl2PrivLineSymbolizerPtr
rl2_clone_line_symbolizer (rl2PrivLineSymbolizerPtr in)
{
/* cloning a Line Symbolizer */
    int len;
    rl2PrivLineSymbolizerPtr out;

    if (in == NULL)
	return NULL;
    out = malloc (sizeof (rl2PrivLineSymbolizer));
    if (out == NULL)
	return NULL;

    out->stroke = NULL;
    if (in->stroke != NULL)
      {
	  rl2PrivStrokePtr stroke_in = in->stroke;
	  rl2PrivStrokePtr stroke_out = malloc (sizeof (rl2PrivStroke));
	  stroke_out->graphic = NULL;
	  if (stroke_in->graphic != NULL)
	    {
		rl2PrivGraphicItemPtr gr_in;
		rl2PrivGraphicPtr ext_in = stroke_in->graphic;
		rl2PrivGraphicPtr ext_out = malloc (sizeof (rl2PrivGraphic));
		ext_out->first = NULL;
		ext_out->last = NULL;
		gr_in = ext_in->first;
		while (gr_in != NULL)
		  {
		      if (gr_in->type == RL2_EXTERNAL_GRAPHIC)
			{
			    rl2PrivColorReplacementPtr repl_in;
			    rl2PrivExternalGraphicPtr eg_in = gr_in->item;
			    rl2PrivExternalGraphicPtr eg_out =
				malloc (sizeof (rl2PrivExternalGraphic));
			    rl2PrivGraphicItemPtr gr_out =
				malloc (sizeof (rl2PrivGraphicItem));
			    eg_out->first = NULL;
			    eg_out->last = NULL;
			    repl_in = eg_in->first;
			    while (repl_in != NULL)
			      {
				  rl2PrivColorReplacementPtr repl_out =
				      malloc (sizeof (rl2PrivColorReplacement));
				  repl_out->index = repl_in->index;
				  repl_out->red = repl_in->red;
				  repl_out->green = repl_in->green;
				  repl_out->blue = repl_in->blue;
				  repl_out->col_color = NULL;
				  if (repl_in->col_color != NULL)
				    {
					len = strlen (repl_in->col_color);
					repl_out->col_color = malloc (len + 1);
					strcpy (repl_out->col_color,
						repl_in->col_color);
				    }
				  repl_out->next = NULL;
				  if (eg_out->first == NULL)
				      eg_out->first = repl_out;
				  if (eg_out->last != NULL)
				      eg_out->last->next = repl_out;
				  eg_out->last = repl_out;
				  repl_in = repl_in->next;
			      }
			    eg_out->xlink_href = NULL;
			    if (eg_in->xlink_href != NULL)
			      {
				  len = strlen (eg_in->xlink_href);
				  eg_out->xlink_href = malloc (len + 1);
				  strcpy (eg_out->xlink_href,
					  eg_in->xlink_href);
			      }
			    eg_out->col_href = NULL;
			    if (eg_in->col_href != NULL)
			      {
				  len = strlen (eg_in->col_href);
				  eg_out->col_href = malloc (len + 1);
				  strcpy (eg_out->col_href, eg_in->col_href);
			      }
			    gr_out->type = gr_in->type;
			    gr_out->item = eg_out;
			    gr_out->next = NULL;
			    if (ext_out->first == NULL)
				ext_out->first = gr_out;
			    if (ext_out->last != NULL)
				ext_out->last->next = gr_out;
			    ext_out->last = gr_out;
			}
		      gr_in = gr_in->next;
		  }
		ext_out->opacity = ext_in->opacity;
		ext_out->size = ext_in->size;
		ext_out->rotation = ext_in->rotation;
		ext_out->anchor_point_x = ext_in->anchor_point_x;
		ext_out->anchor_point_y = ext_in->anchor_point_y;
		ext_out->displacement_x = ext_in->displacement_x;
		ext_out->displacement_y = ext_in->displacement_y;
		ext_out->col_opacity = NULL;
		ext_out->col_rotation = NULL;
		ext_out->col_size = NULL;
		ext_out->col_point_x = NULL;
		ext_out->col_point_y = NULL;
		ext_out->col_displ_x = NULL;
		ext_out->col_displ_y = NULL;
		stroke_out->graphic = ext_out;
	    }
	  stroke_out->red = stroke_in->red;
	  stroke_out->green = stroke_in->green;
	  stroke_out->blue = stroke_in->blue;
	  stroke_out->opacity = stroke_in->opacity;
	  stroke_out->width = stroke_in->width;
	  stroke_out->linejoin = stroke_in->linejoin;
	  stroke_out->linecap = stroke_in->linecap;
	  stroke_out->dash_count = stroke_in->dash_count;
	  stroke_out->dash_list = NULL;
	  if (stroke_in->dash_count > 0)
	    {
		int i;
		stroke_out->dash_list =
		    malloc (sizeof (double) * stroke_out->dash_count);
		for (i = 0; i < stroke_out->dash_count; i++)
		    *(stroke_out->dash_list + i) = *(stroke_in->dash_list + i);
	    }
	  stroke_out->dash_offset = stroke_in->dash_offset;
	  stroke_out->col_color = NULL;
	  if (stroke_in->col_color != NULL)
	    {
		len = strlen (stroke_in->col_color);
		stroke_out->col_color = malloc (len + 1);
		strcpy (stroke_out->col_color, stroke_in->col_color);
	    }
	  stroke_out->col_opacity = NULL;
	  if (stroke_in->col_opacity != NULL)
	    {
		len = strlen (stroke_in->col_opacity);
		stroke_out->col_opacity = malloc (len + 1);
		strcpy (stroke_out->col_opacity, stroke_in->col_opacity);
	    }
	  stroke_out->col_width = NULL;
	  if (stroke_in->col_width != NULL)
	    {
		len = strlen (stroke_in->col_width);
		stroke_out->col_width = malloc (len + 1);
		strcpy (stroke_out->col_width, stroke_in->col_width);
	    }
	  stroke_out->col_join = NULL;
	  if (stroke_in->col_join != NULL)
	    {
		len = strlen (stroke_in->col_join);
		stroke_out->col_join = malloc (len + 1);
		strcpy (stroke_out->col_join, stroke_in->col_join);
	    }
	  stroke_out->col_cap = NULL;
	  if (stroke_in->col_cap != NULL)
	    {
		len = strlen (stroke_in->col_cap);
		stroke_out->col_cap = malloc (len + 1);
		strcpy (stroke_out->col_cap, stroke_in->col_cap);
	    }
	  stroke_out->col_dash = NULL;
	  if (stroke_in->col_dash != NULL)
	    {
		len = strlen (stroke_in->col_dash);
		stroke_out->col_dash = malloc (len + 1);
		strcpy (stroke_out->col_dash, stroke_in->col_dash);
	    }
	  stroke_out->col_dashoff = NULL;
	  if (stroke_in->col_dashoff != NULL)
	    {
		len = strlen (stroke_in->col_dashoff);
		stroke_out->col_dashoff = malloc (len + 1);
		strcpy (stroke_out->col_dashoff, stroke_in->col_dashoff);
	    }
	  out->stroke = stroke_out;
      }
    out->perpendicular_offset = in->perpendicular_offset;
    out->col_perpoff = NULL;
    if (in->col_perpoff != NULL)
      {
	  len = strlen (in->col_perpoff);
	  out->col_perpoff = malloc (len + 1);
	  strcpy (out->col_perpoff, in->col_perpoff);
      }

    return out;
}

RL2_PRIVATE rl2PrivPolygonSymbolizerPtr
rl2_clone_polygon_symbolizer (rl2PrivPolygonSymbolizerPtr in)
{
/* cloning a Polygon Symbolizer */
    int len;
    rl2PrivPolygonSymbolizerPtr out;

    if (in == NULL)
	return NULL;
    out = malloc (sizeof (rl2PrivPolygonSymbolizer));
    if (out == NULL)
	return NULL;

    out->stroke = NULL;
    if (in->stroke != NULL)
      {
	  rl2PrivStrokePtr stroke_in = in->stroke;
	  rl2PrivStrokePtr stroke_out = malloc (sizeof (rl2PrivStroke));
	  stroke_out->graphic = NULL;
	  if (stroke_in->graphic != NULL)
	    {
		rl2PrivGraphicItemPtr gr_in;
		rl2PrivGraphicPtr ext_in = stroke_in->graphic;
		rl2PrivGraphicPtr ext_out = malloc (sizeof (rl2PrivGraphic));
		ext_out->first = NULL;
		ext_out->last = NULL;
		gr_in = ext_in->first;
		while (gr_in != NULL)
		  {
		      if (gr_in->type == RL2_EXTERNAL_GRAPHIC)
			{
			    rl2PrivColorReplacementPtr repl_in;
			    rl2PrivExternalGraphicPtr eg_in = gr_in->item;
			    rl2PrivExternalGraphicPtr eg_out =
				malloc (sizeof (rl2PrivExternalGraphic));
			    rl2PrivGraphicItemPtr gr_out =
				malloc (sizeof (rl2PrivGraphicItem));
			    eg_out->first = NULL;
			    eg_out->last = NULL;
			    repl_in = eg_in->first;
			    while (repl_in != NULL)
			      {
				  rl2PrivColorReplacementPtr repl_out =
				      malloc (sizeof (rl2PrivColorReplacement));
				  repl_out->index = repl_in->index;
				  repl_out->red = repl_in->red;
				  repl_out->green = repl_in->green;
				  repl_out->blue = repl_in->blue;
				  repl_out->col_color = NULL;
				  if (repl_in->col_color != NULL)
				    {
					len = strlen (repl_in->col_color);
					repl_out->col_color = malloc (len + 1);
					strcpy (repl_out->col_color,
						repl_in->col_color);
				    }
				  repl_out->next = NULL;
				  if (eg_out->first == NULL)
				      eg_out->first = repl_out;
				  if (eg_out->last != NULL)
				      eg_out->last->next = repl_out;
				  eg_out->last = repl_out;
				  repl_in = repl_in->next;
			      }
			    eg_out->xlink_href = NULL;
			    if (eg_in->xlink_href != NULL)
			      {
				  len = strlen (eg_in->xlink_href);
				  eg_out->xlink_href = malloc (len + 1);
				  strcpy (eg_out->xlink_href,
					  eg_in->xlink_href);
			      }
			    eg_out->col_href = NULL;
			    if (eg_in->col_href != NULL)
			      {
				  len = strlen (eg_in->col_href);
				  eg_out->col_href = malloc (len + 1);
				  strcpy (eg_out->col_href, eg_in->col_href);
			      }
			    gr_out->type = gr_in->type;
			    gr_out->item = eg_out;
			    gr_out->next = NULL;
			    if (ext_out->first == NULL)
				ext_out->first = gr_out;
			    if (ext_out->last != NULL)
				ext_out->last->next = gr_out;
			    ext_out->last = gr_out;
			}
		      gr_in = gr_in->next;
		  }
		ext_out->opacity = ext_in->opacity;
		ext_out->size = ext_in->size;
		ext_out->rotation = ext_in->rotation;
		ext_out->anchor_point_x = ext_in->anchor_point_x;
		ext_out->anchor_point_y = ext_in->anchor_point_y;
		ext_out->displacement_x = ext_in->displacement_x;
		ext_out->displacement_y = ext_in->displacement_y;
		ext_out->col_opacity = NULL;
		ext_out->col_rotation = NULL;
		ext_out->col_size = NULL;
		ext_out->col_point_x = NULL;
		ext_out->col_point_y = NULL;
		ext_out->col_displ_x = NULL;
		ext_out->col_displ_y = NULL;
		stroke_out->graphic = ext_out;
	    }
	  stroke_out->red = stroke_in->red;
	  stroke_out->green = stroke_in->green;
	  stroke_out->blue = stroke_in->blue;
	  stroke_out->opacity = stroke_in->opacity;
	  stroke_out->width = stroke_in->width;
	  stroke_out->linejoin = stroke_in->linejoin;
	  stroke_out->linecap = stroke_in->linecap;
	  stroke_out->dash_count = stroke_in->dash_count;
	  stroke_out->dash_list = NULL;
	  if (stroke_in->dash_count > 0)
	    {
		int i;
		stroke_out->dash_list =
		    malloc (sizeof (double) * stroke_out->dash_count);
		for (i = 0; i < stroke_out->dash_count; i++)
		    *(stroke_out->dash_list + i) = *(stroke_in->dash_list + i);
	    }
	  stroke_out->dash_offset = stroke_in->dash_offset;
	  stroke_out->col_color = NULL;
	  if (stroke_in->col_color != NULL)
	    {
		len = strlen (stroke_in->col_color);
		stroke_out->col_color = malloc (len + 1);
		strcpy (stroke_out->col_color, stroke_in->col_color);
	    }
	  stroke_out->col_opacity = NULL;
	  if (stroke_in->col_opacity != NULL)
	    {
		len = strlen (stroke_in->col_opacity);
		stroke_out->col_opacity = malloc (len + 1);
		strcpy (stroke_out->col_opacity, stroke_in->col_opacity);
	    }
	  stroke_out->col_width = NULL;
	  if (stroke_in->col_width != NULL)
	    {
		len = strlen (stroke_in->col_width);
		stroke_out->col_width = malloc (len + 1);
		strcpy (stroke_out->col_width, stroke_in->col_width);
	    }
	  stroke_out->col_join = NULL;
	  if (stroke_in->col_join != NULL)
	    {
		len = strlen (stroke_in->col_join);
		stroke_out->col_join = malloc (len + 1);
		strcpy (stroke_out->col_join, stroke_in->col_join);
	    }
	  stroke_out->col_cap = NULL;
	  if (stroke_in->col_cap != NULL)
	    {
		len = strlen (stroke_in->col_cap);
		stroke_out->col_cap = malloc (len + 1);
		strcpy (stroke_out->col_cap, stroke_in->col_cap);
	    }
	  stroke_out->col_dash = NULL;
	  if (stroke_in->col_dash != NULL)
	    {
		len = strlen (stroke_in->col_dash);
		stroke_out->col_dash = malloc (len + 1);
		strcpy (stroke_out->col_dash, stroke_in->col_dash);
	    }
	  stroke_out->col_dashoff = NULL;
	  if (stroke_in->col_dashoff != NULL)
	    {
		len = strlen (stroke_in->col_dashoff);
		stroke_out->col_dashoff = malloc (len + 1);
		strcpy (stroke_out->col_dashoff, stroke_in->col_dashoff);
	    }
	  out->stroke = stroke_out;
      }
    out->fill = NULL;
    if (in->fill != NULL)
      {
	  rl2PrivFillPtr fill_in = in->fill;
	  rl2PrivFillPtr fill_out = malloc (sizeof (rl2PrivFill));
	  fill_out->graphic = NULL;
	  if (fill_in->graphic != NULL)
	    {
		rl2PrivGraphicItemPtr gr_in;
		rl2PrivGraphicPtr ext_in = fill_in->graphic;
		rl2PrivGraphicPtr ext_out = malloc (sizeof (rl2PrivGraphic));
		ext_out->first = NULL;
		ext_out->last = NULL;
		gr_in = ext_in->first;
		while (gr_in != NULL)
		  {
		      if (gr_in->type == RL2_EXTERNAL_GRAPHIC)
			{
			    rl2PrivColorReplacementPtr repl_in;
			    rl2PrivExternalGraphicPtr eg_in = gr_in->item;
			    rl2PrivExternalGraphicPtr eg_out =
				malloc (sizeof (rl2PrivExternalGraphic));
			    rl2PrivGraphicItemPtr gr_out =
				malloc (sizeof (rl2PrivGraphicItem));
			    eg_out->first = NULL;
			    eg_out->last = NULL;
			    repl_in = eg_in->first;
			    while (repl_in != NULL)
			      {
				  rl2PrivColorReplacementPtr repl_out =
				      malloc (sizeof (rl2PrivColorReplacement));
				  repl_out->index = repl_in->index;
				  repl_out->red = repl_in->red;
				  repl_out->green = repl_in->green;
				  repl_out->blue = repl_in->blue;
				  repl_out->col_color = NULL;
				  if (repl_in->col_color != NULL)
				    {
					len = strlen (repl_in->col_color);
					repl_out->col_color = malloc (len + 1);
					strcpy (repl_out->col_color,
						repl_in->col_color);
				    }
				  repl_out->next = NULL;
				  if (eg_out->first == NULL)
				      eg_out->first = repl_out;
				  if (eg_out->last != NULL)
				      eg_out->last->next = repl_out;
				  eg_out->last = repl_out;
				  repl_in = repl_in->next;
			      }
			    eg_out->xlink_href = NULL;
			    if (eg_in->xlink_href != NULL)
			      {
				  len = strlen (eg_in->xlink_href);
				  eg_out->xlink_href = malloc (len + 1);
				  strcpy (eg_out->xlink_href,
					  eg_in->xlink_href);
			      }
			    eg_out->col_href = NULL;
			    if (eg_in->col_href != NULL)
			      {
				  len = strlen (eg_in->col_href);
				  eg_out->col_href = malloc (len + 1);
				  strcpy (eg_out->col_href, eg_in->col_href);
			      }
			    gr_out->type = gr_in->type;
			    gr_out->item = eg_out;
			    gr_out->next = NULL;
			    if (ext_out->first == NULL)
				ext_out->first = gr_out;
			    if (ext_out->last != NULL)
				ext_out->last->next = gr_out;
			    ext_out->last = gr_out;
			}
		      gr_in = gr_in->next;
		  }
		ext_out->opacity = ext_in->opacity;
		ext_out->size = ext_in->size;
		ext_out->rotation = ext_in->rotation;
		ext_out->anchor_point_x = ext_in->anchor_point_x;
		ext_out->anchor_point_y = ext_in->anchor_point_y;
		ext_out->displacement_x = ext_in->displacement_x;
		ext_out->displacement_y = ext_in->displacement_y;
		ext_out->col_opacity = NULL;
		ext_out->col_rotation = NULL;
		ext_out->col_size = NULL;
		ext_out->col_point_x = NULL;
		ext_out->col_point_y = NULL;
		ext_out->col_displ_x = NULL;
		ext_out->col_displ_y = NULL;
		fill_out->graphic = ext_out;
	    }
	  fill_out->red = fill_in->red;
	  fill_out->green = fill_in->green;
	  fill_out->blue = fill_in->blue;
	  fill_out->opacity = fill_in->opacity;
	  fill_out->col_color = NULL;
	  if (fill_in->col_color != NULL)
	    {
		len = strlen (fill_in->col_color);
		fill_out->col_color = malloc (len + 1);
		strcpy (fill_out->col_color, fill_in->col_color);
	    }
	  fill_out->col_opacity = NULL;
	  if (fill_in->col_opacity != NULL)
	    {
		len = strlen (fill_in->col_opacity);
		fill_out->col_opacity = malloc (len + 1);
		strcpy (fill_out->col_opacity, fill_in->col_opacity);
	    }
	  out->fill = fill_out;
      }
    out->displacement_x = in->displacement_x;
    out->displacement_y = in->displacement_y;
    out->perpendicular_offset = in->perpendicular_offset;
    out->col_displ_x = NULL;
    if (in->col_displ_x != NULL)
      {
	  len = strlen (in->col_displ_x);
	  out->col_displ_x = malloc (len + 1);
	  strcpy (out->col_displ_x, in->col_displ_x);
      }
    out->col_displ_y = NULL;
    if (in->col_displ_y != NULL)
      {
	  len = strlen (in->col_displ_y);
	  out->col_displ_y = malloc (len + 1);
	  strcpy (out->col_displ_y, in->col_displ_y);
      }
    out->col_perpoff = NULL;
    if (in->col_perpoff != NULL)
      {
	  len = strlen (in->col_perpoff);
	  out->col_perpoff = malloc (len + 1);
	  strcpy (out->col_perpoff, in->col_perpoff);
      }

    return out;
}

RL2_PRIVATE rl2PrivTextSymbolizerPtr
rl2_clone_text_symbolizer (rl2PrivTextSymbolizerPtr in)
{
/* cloning a Text Symbolizer */
    int i;
    int len;
    rl2PrivTextSymbolizerPtr out;

    if (in == NULL)
	return NULL;
    out = malloc (sizeof (rl2PrivTextSymbolizer));
    if (out == NULL)
	return NULL;

    out->label = NULL;
    if (in->label != NULL)
      {
	  len = strlen (in->label);
	  out->label = malloc (len + 1);
	  strcpy (out->label, in->label);
      }
    out->col_label = NULL;
    if (in->col_label != NULL)
      {
	  len = strlen (in->col_label);
	  out->col_label = malloc (len + 1);
	  strcpy (out->col_label, in->col_label);
      }
    out->col_font = NULL;
    if (in->col_font != NULL)
      {
	  len = strlen (in->col_font);
	  out->col_font = malloc (len + 1);
	  strcpy (out->col_font, in->col_font);
      }
    out->col_style = NULL;
    if (in->col_style != NULL)
      {
	  len = strlen (in->col_style);
	  out->col_style = malloc (len + 1);
	  strcpy (out->col_style, in->col_style);
      }
    out->col_weight = NULL;
    if (in->col_weight != NULL)
      {
	  len = strlen (in->col_weight);
	  out->col_weight = malloc (len + 1);
	  strcpy (out->col_weight, in->col_weight);
      }
    out->col_size = NULL;
    if (in->col_size != NULL)
      {
	  len = strlen (in->col_size);
	  out->col_size = malloc (len + 1);
	  strcpy (out->col_size, in->col_size);
      }
    out->font_families_count = in->font_families_count;
    for (i = 0; i < RL2_MAX_FONT_FAMILIES; i++)
      {
	  *(out->font_families + i) = NULL;
	  if (*(in->font_families + i) != NULL)
	    {
		len = strlen (*(in->font_families + i));
		*(out->font_families + i) = malloc (len + 1);
		strcpy (*(out->font_families + i), *(in->font_families + i));
	    }
      }
    out->font_style = in->font_style;
    out->font_weight = in->font_weight;
    out->font_size = in->font_size;
    out->label_placement_type = in->label_placement_type;
    out->label_placement = NULL;
    if (in->label_placement != NULL)
      {
	  if (in->label_placement_type == RL2_LABEL_PLACEMENT_POINT)
	    {
		rl2PrivPointPlacementPtr pl_in = in->label_placement;
		rl2PrivPointPlacementPtr pl_out =
		    malloc (sizeof (rl2PrivPointPlacement));
		pl_out->anchor_point_x = pl_in->anchor_point_x;
		pl_out->anchor_point_y = pl_in->anchor_point_y;
		pl_out->displacement_x = pl_in->displacement_x;
		pl_out->displacement_y = pl_in->displacement_y;
		pl_out->rotation = pl_in->rotation;
		pl_out->col_point_x = NULL;
		if (pl_in->col_point_x != NULL)
		  {
		      len = strlen (pl_in->col_point_x);
		      pl_out->col_point_x = malloc (len + 1);
		      strcpy (pl_out->col_point_x, pl_in->col_point_x);
		  }
		pl_out->col_point_y = NULL;
		if (pl_in->col_point_y != NULL)
		  {
		      len = strlen (pl_in->col_point_y);
		      pl_out->col_point_y = malloc (len + 1);
		      strcpy (pl_out->col_point_y, pl_in->col_point_y);
		  }
		pl_out->col_displ_x = NULL;
		if (pl_in->col_displ_x != NULL)
		  {
		      len = strlen (pl_in->col_displ_x);
		      pl_out->col_displ_x = malloc (len + 1);
		      strcpy (pl_out->col_displ_x, pl_in->col_displ_x);
		  }
		pl_out->col_displ_y = NULL;
		if (pl_in->col_displ_y != NULL)
		  {
		      len = strlen (pl_in->col_displ_x);
		      pl_out->col_displ_y = malloc (len + 1);
		      strcpy (pl_out->col_displ_y, pl_in->col_displ_y);
		  }
		pl_out->col_rotation = NULL;
		if (pl_in->col_rotation != NULL)
		  {
		      len = strlen (pl_in->col_rotation);
		      pl_out->col_rotation = malloc (len + 1);
		      strcpy (pl_out->col_rotation, pl_in->col_rotation);
		  }
		out->label_placement = pl_out;
	    }
	  if (in->label_placement_type == RL2_LABEL_PLACEMENT_LINE)
	    {
		rl2PrivLinePlacementPtr pl_in = in->label_placement;
		rl2PrivLinePlacementPtr pl_out =
		    malloc (sizeof (rl2PrivLinePlacement));
		pl_out->perpendicular_offset = pl_in->perpendicular_offset;
		pl_out->is_repeated = pl_in->is_repeated;
		pl_out->initial_gap = pl_in->initial_gap;
		pl_out->gap = pl_in->gap;
		pl_out->is_aligned = pl_in->is_aligned;
		pl_out->generalize_line = pl_in->generalize_line;
		pl_out->col_perpoff = NULL;
		if (pl_in->col_perpoff != NULL)
		  {
		      len = strlen (pl_in->col_perpoff);
		      pl_out->col_perpoff = malloc (len + 1);
		      strcpy (pl_out->col_perpoff, pl_in->col_perpoff);
		  }
		pl_out->col_inigap = NULL;
		if (pl_in->col_inigap != NULL)
		  {
		      len = strlen (pl_in->col_inigap);
		      pl_out->col_inigap = malloc (len + 1);
		      strcpy (pl_out->col_inigap, pl_in->col_inigap);
		  }
		pl_out->col_gap = NULL;
		if (pl_in->col_gap != NULL)
		  {
		      len = strlen (pl_in->col_gap);
		      pl_out->col_gap = malloc (len + 1);
		      strcpy (pl_out->col_gap, pl_in->col_gap);
		  }
		out->label_placement = pl_out;
	    }
      }
    out->halo = NULL;
    if (in->halo != NULL)
      {
	  rl2PrivHaloPtr halo_in = in->halo;
	  rl2PrivHaloPtr halo_out = malloc (sizeof (rl2PrivHalo));
	  halo_out->radius = halo_in->radius;
	  halo_out->fill = NULL;
	  if (halo_in->fill != NULL)
	    {
		rl2PrivFillPtr fill_in = halo_in->fill;
		rl2PrivFillPtr fill_out = malloc (sizeof (rl2PrivFill));
		fill_out->graphic = NULL;
		fill_out->red = fill_in->red;
		fill_out->green = fill_in->green;
		fill_out->blue = fill_in->blue;
		fill_out->opacity = fill_in->opacity;
		fill_out->col_color = NULL;
		if (fill_in->col_color != NULL)
		  {
		      len = strlen (fill_in->col_color);
		      fill_out->col_color = malloc (len + 1);
		      strcpy (fill_out->col_color, fill_in->col_color);
		  }
		fill_out->col_opacity = NULL;
		if (fill_in->col_opacity != NULL)
		  {
		      len = strlen (fill_in->col_opacity);
		      fill_out->col_opacity = malloc (len + 1);
		      strcpy (fill_out->col_opacity, fill_in->col_opacity);
		  }
		halo_out->fill = fill_out;
	    }
	  halo_out->col_radius = NULL;
	  if (halo_in->col_radius != NULL)
	    {
		len = strlen (halo_in->col_radius);
		halo_out->col_radius = malloc (len + 1);
		strcpy (halo_out->col_radius, halo_in->col_radius);
	    }
	  out->halo = halo_out;
      }
    out->fill = NULL;
    if (in->fill != NULL)
      {
	  rl2PrivFillPtr fill_in = in->fill;
	  rl2PrivFillPtr fill_out = malloc (sizeof (rl2PrivFill));
	  fill_out->graphic = NULL;
	  fill_out->red = fill_in->red;
	  fill_out->green = fill_in->green;
	  fill_out->blue = fill_in->blue;
	  fill_out->opacity = fill_in->opacity;
	  fill_out->col_color = NULL;
	  if (fill_in->col_color != NULL)
	    {
		len = strlen (fill_in->col_color);
		fill_out->col_color = malloc (len + 1);
		strcpy (fill_out->col_color, fill_in->col_color);
	    }
	  fill_out->col_opacity = NULL;
	  if (fill_in->col_opacity != NULL)
	    {
		len = strlen (fill_in->col_opacity);
		fill_out->col_opacity = malloc (len + 1);
		strcpy (fill_out->col_opacity, fill_in->col_opacity);
	    }
	  out->fill = fill_out;
      }

    return out;
}
