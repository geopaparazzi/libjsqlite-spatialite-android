/*

 rl2symbolizer -- private SQL helper methods

 version 0.1, 2014 March 17

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
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "config.h"

#include <libxml/parser.h>

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#define RL2_UNUSED() if (argc || argv) argc = argc;

static void parse_graphic (xmlNodePtr node, rl2PrivGraphicPtr graphic);

static void
dummySilentError (void *ctx, const char *msg, ...)
{
/* shutting up XML Errors */
    if (ctx != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */
    if (msg != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */
}

static void
parse_sld_se_min_scale_denominator (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* parsing Rule MinScaleDenominator */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MinScaleDenominator") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				rule->min_scale =
				    atof ((const char *) child->content);
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_sld_se_max_scale_denominator (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* parsing Rule MaxScaleDenominator */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MaxScaleDenominator") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				rule->max_scale =
				    atof ((const char *) child->content);
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_sld_se_opacity (xmlNodePtr node, rl2PrivRasterSymbolizerPtr style)
{
/* parsing RasterSymbolizer Opacity */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Opacity") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				style->opacity =
				    atof ((const char *) child->content);
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static int
parse_sld_se_band_name (const char *name, unsigned char *band)
{
/* attempting to parse a band name */
    int digit = 0;
    int nodigit = 0;
    int i;
    for (i = 0; i < (int) strlen (name); i++)
      {
	  if (*(name + i) >= '0' && *(name + i) <= '9')
	      digit++;
	  else
	      nodigit++;
      }
    if (digit && !nodigit)
      {
	  /* band identified by number */
	  int x = atoi (name) - 1;	/* first RL2 band has index 0 !!! */
	  if (x >= 0 && x <= 255)
	    {
		*band = x;
		return 1;
	    }
      }
    if (digit && nodigit)
      {
	  if (strlen (name) > 9 && strncmp (name, "Band.band", 9) == 0)
	    {
		/* band identified by a string like "Band.band1" */
		int x = atoi (name + 9) - 1;	/* first RL2 band has index 0 !!! */
		if (x >= 0 && x <= 255)
		  {
		      *band = x;
		      return 1;
		  }
	    }
      }
    return 0;
}

static int
parse_sld_se_channel_band (xmlNodePtr node, unsigned char *band)
{
/* parsing RasterSymbolizer Channel -> Band */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "SourceChannelName") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *band_name =
				      (const char *) child->content;
				  if (parse_sld_se_band_name (band_name, band))
				      return 1;
				  else
				      return 0;
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
    return 0;
}

static int
parse_sld_se_gamma_value (xmlNodePtr node, double *gamma)
{
/* parsing RasterSymbolizer GammaValue */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "GammaValue") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *gamma_value =
				      (const char *) child->content;
				  double gv = atof (gamma_value);
				  *gamma = gv;
				  return 1;
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
    return 0;
}

static int
parse_sld_se_contrast_enhancement (xmlNodePtr node, unsigned char *mode,
				   double *gamma)
{
/* parsing RasterSymbolizer ContrastEnhancement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ContrastEnhancement") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *xmode =
				      (const char *) (child->name);
				  if (strcmp (xmode, "Normalize") == 0)
				    {
					*mode =
					    RL2_CONTRAST_ENHANCEMENT_NORMALIZE;
					return 1;
				    }
				  if (strcmp (xmode, "Histogram") == 0)
				    {
					*mode =
					    RL2_CONTRAST_ENHANCEMENT_HISTOGRAM;
					return 1;
				    }
				  if (strcmp (xmode, "GammaValue") == 0)
				    {
					if (parse_sld_se_gamma_value
					    (child, gamma))
					  {
					      *mode =
						  RL2_CONTRAST_ENHANCEMENT_GAMMA;
					      return 1;
					  }
					return 1;
				    }
			      }
			    child = child->next;
			}
		      return 0;
		  }
	    }
	  node = node->next;
      }
    return 1;
}

static int
parse_sld_se_channels (xmlNodePtr node, rl2PrivRasterSymbolizerPtr style)
{
/* parsing RasterSymbolizer Channels */
    int has_red = 0;
    int has_green = 0;
    int has_blue = 0;
    int has_gray = 0;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char gray;
    unsigned char red_contrast = RL2_CONTRAST_ENHANCEMENT_NONE;
    unsigned char green_contrast = RL2_CONTRAST_ENHANCEMENT_NONE;
    unsigned char blue_contrast = RL2_CONTRAST_ENHANCEMENT_NONE;
    unsigned char gray_contrast = RL2_CONTRAST_ENHANCEMENT_NONE;
    double red_gamma = 1.0;
    double green_gamma = 1.0;
    double blue_gamma = 1.0;
    double gray_gamma = 1.0;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "RedChannel") == 0)
		  {
		      has_red = 1;
		      if (!parse_sld_se_channel_band (node->children, &red))
			  return 0;
		      if (!parse_sld_se_contrast_enhancement
			  (node->children, &red_contrast, &red_gamma))
			  return 0;
		  }
		if (strcmp (name, "GreenChannel") == 0)
		  {
		      has_green = 1;
		      if (!parse_sld_se_channel_band (node->children, &green))
			  return 0;
		      if (!parse_sld_se_contrast_enhancement
			  (node->children, &green_contrast, &green_gamma))
			  return 0;
		  }
		if (strcmp (name, "BlueChannel") == 0)
		  {
		      has_blue = 1;
		      if (!parse_sld_se_channel_band (node->children, &blue))
			  return 0;
		      if (!parse_sld_se_contrast_enhancement
			  (node->children, &blue_contrast, &blue_gamma))
			  return 0;
		  }
		if (strcmp (name, "GrayChannel") == 0)
		  {
		      has_gray = 1;
		      if (!parse_sld_se_channel_band (node->children, &gray))
			  return 0;
		      if (!parse_sld_se_contrast_enhancement
			  (node->children, &gray_contrast, &gray_gamma))
			  return 0;
		  }
	    }
	  node = node->next;
      }
    if (has_red && has_green && has_blue && !has_gray)
      {
	  /* triple band selection */
	  style->bandSelection = malloc (sizeof (rl2PrivBandSelection));
	  style->bandSelection->selectionType = RL2_BAND_SELECTION_TRIPLE;
	  style->bandSelection->redBand = red;
	  style->bandSelection->greenBand = green;
	  style->bandSelection->blueBand = blue;
	  style->bandSelection->redContrast = red_contrast;
	  style->bandSelection->redGamma = red_gamma;
	  style->bandSelection->greenContrast = green_contrast;
	  style->bandSelection->greenGamma = green_gamma;
	  style->bandSelection->blueContrast = blue_contrast;
	  style->bandSelection->blueGamma = blue_gamma;
	  return 1;
      }
    if (!has_red && !has_green && !has_blue && has_gray)
      {
	  /* mono band selection */
	  if (gray_gamma < 0.0)
	      gray_gamma = 0.0;
	  if (gray_gamma > 1.0)
	      gray_gamma = 1.0;
	  style->bandSelection = malloc (sizeof (rl2PrivBandSelection));
	  style->bandSelection->selectionType = RL2_BAND_SELECTION_MONO;
	  style->bandSelection->grayBand = gray;
	  style->bandSelection->grayContrast = gray_contrast;
	  style->bandSelection->grayGamma = gray_gamma;
	  return 1;
      }

    return 0;
}

static int
parse_hex (unsigned char hi, unsigned char lo, unsigned char *val)
{
/* attempting to parse an hexadecimal byte */
    unsigned char value;
    switch (hi)
      {
      case '0':
	  value = 0;
	  break;
      case '1':
	  value = 1 * 16;
	  break;
      case '2':
	  value = 2 * 16;
	  break;
      case '3':
	  value = 3 * 16;
	  break;
      case '4':
	  value = 4 * 16;
	  break;
      case '5':
	  value = 5 * 16;
	  break;
      case '6':
	  value = 6 * 16;
	  break;
      case '7':
	  value = 7 * 16;
	  break;
      case '8':
	  value = 8 * 16;
	  break;
      case '9':
	  value = 9 * 16;
	  break;
      case 'a':
      case 'A':
	  value = 10 * 16;
	  break;
      case 'b':
      case 'B':
	  value = 11 * 16;
	  break;
      case 'c':
      case 'C':
	  value = 12 * 16;
	  break;
      case 'd':
      case 'D':
	  value = 13 * 16;
	  break;
      case 'e':
      case 'E':
	  value = 14 * 16;
	  break;
      case 'f':
      case 'F':
	  value = 15 * 16;
	  break;
      default:
	  return 0;
	  break;
      };
    switch (lo)
      {
      case '0':
	  value += 0;
	  break;
      case '1':
	  value += 1;
	  break;
      case '2':
	  value += 2;
	  break;
      case '3':
	  value += 3;
	  break;
      case '4':
	  value += 4;
	  break;
      case '5':
	  value += 5;
	  break;
      case '6':
	  value += 6;
	  break;
      case '7':
	  value += 7;
	  break;
      case '8':
	  value += 8;
	  break;
      case '9':
	  value += 9;
	  break;
      case 'a':
      case 'A':
	  value += 10;
	  break;
      case 'b':
      case 'B':
	  value += 11;
	  break;
      case 'c':
      case 'C':
	  value += 12;
	  break;
      case 'd':
      case 'D':
	  value += 13;
	  break;
      case 'e':
      case 'E':
	  value += 14;
	  break;
      case 'f':
      case 'F':
	  value += 15;
	  break;
      default:
	  return 0;
	  break;
      };
    *val = value;
    return 1;
}

static int
parse_sld_se_color (const char *color, unsigned char *red,
		    unsigned char *green, unsigned char *blue)
{
/* attempting to parse a #RRGGBB hexadecimal color */
    unsigned char r;
    unsigned char g;
    unsigned char b;
    if (strlen (color) != 7)
	return 0;
    if (*color != '#')
	return 0;
    if (!parse_hex (*(color + 1), *(color + 2), &r))
	return 0;
    if (!parse_hex (*(color + 3), *(color + 4), &g))
	return 0;
    if (!parse_hex (*(color + 5), *(color + 6), &b))
	return 0;
    *red = r;
    *green = g;
    *blue = b;
    return 1;
}

static int
parse_sld_se_channel_selection (xmlNodePtr node,
				rl2PrivRasterSymbolizerPtr style)
{
/* parsing RasterSymbolizer ChannelSelection */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ChannelSelection") == 0)
		  {
		      if (parse_sld_se_channels (node->children, style))
			  return 1;
		      else
			  return 0;
		  }
	    }
	  node = node->next;
      }
    return 1;
}

static int
parse_sld_se_categorize (xmlNodePtr node, rl2PrivRasterSymbolizerPtr style)
{
/* parsing RasterSymbolizer ColorMap Categorize */
    struct _xmlAttr *attr;
    xmlNodePtr child;
    xmlNodePtr text;

    attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "fallbackValue") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *color =
				      (const char *) (text->content);
				  if (color != NULL)
				    {
					unsigned char red;
					unsigned char green;
					unsigned char blue;
					if (parse_sld_se_color
					    (color, &red, &green, &blue))
					  {
					      style->categorize->dfltRed = red;
					      style->categorize->dfltGreen =
						  green;
					      style->categorize->dfltBlue =
						  blue;
					  }
				    }
			      }
			}
		  }
	    }
	  attr = attr->next;
      }

    child = node->children;
    while (child)
      {
	  if (child->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (child->name);
		if (strcmp (name, "Value") == 0)
		  {
		      text = child->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *color =
				      (const char *) (text->content);
				  if (color != NULL)
				    {
					unsigned char red;
					unsigned char green;
					unsigned char blue;
					if (parse_sld_se_color
					    (color, &red, &green, &blue))
					  {
					      if (style->categorize->last ==
						  NULL)
						{
						    style->categorize->baseRed =
							red;
						    style->categorize->baseGreen
							= green;
						    style->categorize->
							baseBlue = blue;
						}
					      else
						{
						    style->categorize->last->
							red = red;
						    style->categorize->last->
							green = green;
						    style->categorize->last->
							blue = blue;
						}
					  }
					else
					    return 0;
				    }
			      }
			    text = text->next;
			}
		  }
		if (strcmp (name, "Threshold") == 0)
		  {
		      text = child->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				    {
					rl2PrivColorMapPointPtr pt =
					    malloc (sizeof
						    (rl2PrivColorMapPoint));
					pt->value = atof (value);
					pt->red = 0;
					pt->green = 0;
					pt->blue = 0;
					pt->next = NULL;
					if (style->categorize->first == NULL)
					    style->categorize->first = pt;
					if (style->categorize->last != NULL)
					    style->categorize->last->next = pt;
					style->categorize->last = pt;
				    }
			      }
			    text = text->next;
			}
		  }
	    }
	  child = child->next;
      }
    return 1;
}

static int
parse_sld_se_interpolation_point_data (xmlNodePtr node, double *data)
{
/* parsing RasterSymbolizer ColorMap InterpolationPoint Data */
    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		const char *value = (const char *) (node->content);
		if (value != NULL)
		  {
		      *data = atof (value);
		      return 1;
		  }
	    }
	  node = node->next;
      }
    return 0;
}

static int
parse_sld_se_interpolation_point_value (xmlNodePtr node, unsigned char *red,
					unsigned char *green,
					unsigned char *blue)
{
/* parsing RasterSymbolizer ColorMap InterpolationPoint Value */
    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		const char *color = (const char *) (node->content);
		if (color != NULL)
		  {
		      unsigned char r;
		      unsigned char g;
		      unsigned char b;
		      if (parse_sld_se_color (color, &r, &g, &b))
			{
			    *red = r;
			    *green = g;
			    *blue = b;
			    return 1;
			}
		  }
	    }
	  node = node->next;
      }
    return 0;
}

static int
parse_sld_se_interpolation_point (xmlNodePtr node,
				  rl2PrivRasterSymbolizerPtr style)
{
/* parsing RasterSymbolizer ColorMap InterpolationPoint */
    double value = 0.0;
    unsigned char red = 0;
    unsigned char green = 0;
    unsigned char blue = 0;
    int has_data = 0;
    int has_value = 0;

    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Data") == 0)
		  {
		      has_data = 1;
		      if (!parse_sld_se_interpolation_point_data
			  (node->children, &value))
			  return 0;
		  }
		if (strcmp (name, "Value") == 0)
		  {
		      has_value = 1;
		      if (!parse_sld_se_interpolation_point_value
			  (node->children, &red, &green, &blue))
			  return 0;
		  }
	    }
	  node = node->next;
      }

    if (has_data && has_value)
      {
	  rl2PrivColorMapPointPtr pt = malloc (sizeof (rl2PrivColorMapPoint));
	  pt->value = value;
	  pt->red = red;
	  pt->green = green;
	  pt->blue = blue;
	  pt->next = NULL;
	  if (style->interpolate->first == NULL)
	      style->interpolate->first = pt;
	  if (style->interpolate->last != NULL)
	      style->interpolate->last->next = pt;
	  style->interpolate->last = pt;
	  return 1;
      }
    return 0;
}

static int
parse_sld_se_interpolate (xmlNodePtr node, rl2PrivRasterSymbolizerPtr style)
{
/* parsing RasterSymbolizer ColorMap Interpolate */
    struct _xmlAttr *attr;
    xmlNodePtr child;

    attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "fallbackValue") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *color =
				      (const char *) (text->content);
				  if (color != NULL)
				    {
					unsigned char red;
					unsigned char green;
					unsigned char blue;
					if (parse_sld_se_color
					    (color, &red, &green, &blue))
					  {
					      style->interpolate->dfltRed = red;
					      style->interpolate->dfltGreen =
						  green;
					      style->interpolate->dfltBlue =
						  blue;
					  }
				    }
			      }
			}
		  }
	    }
	  attr = attr->next;
      }

    child = node->children;
    while (child)
      {
	  if (child->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (child->name);
		if (strcmp (name, "InterpolationPoint") == 0)
		  {
		      if (!parse_sld_se_interpolation_point
			  (child->children, style))
			  return 0;
		  }
	    }
	  child = child->next;
      }
    return 1;
}

static int
parse_sld_se_color_map (xmlNodePtr node, rl2PrivRasterSymbolizerPtr style)
{
/* parsing RasterSymbolizer ColorMap */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ColorMap") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *xmode =
				      (const char *) (child->name);
				  if (strcmp (xmode, "Interpolate") == 0)
				    {
					style->interpolate =
					    malloc (sizeof
						    (rl2PrivColorMapInterpolate));
					style->interpolate->first = NULL;
					style->interpolate->last = NULL;
					style->interpolate->dfltRed = 0;
					style->interpolate->dfltGreen = 0;
					style->interpolate->dfltBlue = 0;
					if (parse_sld_se_interpolate
					    (child, style))
					    return 1;
				    }
				  if (strcmp (xmode, "Categorize") == 0)
				    {
					style->categorize =
					    malloc (sizeof
						    (rl2PrivColorMapCategorize));
					style->categorize->first = NULL;
					style->categorize->last = NULL;
					style->categorize->dfltRed = 0;
					style->categorize->dfltGreen = 0;
					style->categorize->dfltBlue = 0;
					style->categorize->baseRed = 0;
					style->categorize->baseGreen = 0;
					style->categorize->baseBlue = 0;
					if (parse_sld_se_categorize
					    (child, style))
					    return 1;
				    }
			      }
			    child = child->next;
			}
		      return 0;
		  }
	    }
	  node = node->next;
      }
    return 1;
}

static void
parse_sld_se_shaded_relief (xmlNodePtr node, rl2PrivRasterSymbolizerPtr style)
{
/* parsing RasterSymbolizer ShadedRelief */
    xmlNodePtr text;

    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ShadedRelief") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *xmode =
				      (const char *) (child->name);
				  if (strcmp (xmode, "BrightnessOnly") == 0)
				    {
					text = child->children;
					while (text)
					  {
					      if (text->type == XML_TEXT_NODE)
						{
						    const char *value =
							(const char
							 *) (text->content);
						    if (value != NULL)
							style->brightnessOnly
							    = atoi (value);
						}
					      text = text->next;
					  }
				    }
				  if (strcmp (xmode, "ReliefFactor") == 0)
				    {
					text = child->children;
					while (text)
					  {
					      if (text->type == XML_TEXT_NODE)
						{
						    const char *value =
							(const char
							 *) (text->content);
						    if (value != NULL)
							style->reliefFactor =
							    atof (value);
						}
					      text = text->next;
					  }
				    }
			      }
			    child = child->next;
			}
		      style->shadedRelief = 1;
		  }
	    }
	  node = node->next;
      }
}

static int
parse_raster_symbolizer (xmlNodePtr node, rl2PrivRasterSymbolizerPtr style)
{
/* attempting to parse an SLD/SE RasterSymbolizer */
    parse_sld_se_opacity (node->children, style);
    if (!parse_sld_se_channel_selection (node->children, style))
	return 0;
    if (!parse_sld_se_color_map (node->children, style))
	return 0;
    if (!parse_sld_se_contrast_enhancement
	(node->children, &(style->contrastEnhancement), &(style->gammaValue)))
	return 0;
    parse_sld_se_shaded_relief (node->children, style);
    return 1;
}

static void
parse_raster_style_rule (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* attempting to parse an SLD/SE Style Rule (raster type) */
    parse_sld_se_min_scale_denominator (node->children, rule);
    parse_sld_se_max_scale_denominator (node->children, rule);
}

static int
parse_coverage_style (xmlNodePtr node, rl2PrivCoverageStylePtr style)
{
/* attempting to parse an SLD/SE CoverageStyle */
    int count = 0;
    int ret;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rule") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "RasterSymbolizer") == 0)
				    {
					rl2PrivStyleRulePtr rule =
					    rl2_create_default_style_rule ();
					rl2PrivRasterSymbolizerPtr symbolizer
					    =
					    rl2_create_default_raster_symbolizer
					    ();
					if (symbolizer == NULL || rule == NULL)
					  {
					      if (symbolizer != NULL)
						  rl2_destroy_raster_symbolizer
						      (symbolizer);
					      if (rule != NULL)
						  rl2_destroy_style_rule (rule);
					      return 0;
					  }
					rule->style_type = RL2_RASTER_STYLE;
					rule->style = symbolizer;
					parse_raster_style_rule (node, rule);
					ret =
					    parse_raster_symbolizer (child,
								     symbolizer);
					if (ret == 0)
					  {
					      rl2_destroy_style_rule (rule);
					      return 0;
					  }
					/* updating the linked list of rules */
					if (style->first_rule == NULL)
					    style->first_rule = rule;
					if (style->last_rule != NULL)
					    style->last_rule->next = rule;
					style->last_rule = rule;
					count++;
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
    if (count > 0)
	return 1;
    return 0;
}

static int
find_coverage_style (xmlNodePtr node, rl2PrivCoverageStylePtr style, int *loop)
{
/* recursively searching an SLD/SE CoverageStyle */
    int ret;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "RasterSymbolizer") == 0)
		  {
		      rl2PrivStyleRulePtr rule =
			  rl2_create_default_style_rule ();
		      rl2PrivRasterSymbolizerPtr symbolizer =
			  rl2_create_default_raster_symbolizer ();
		      if (symbolizer == NULL || rule == NULL)
			{
			    if (symbolizer != NULL)
				rl2_destroy_raster_symbolizer (symbolizer);
			    if (rule != NULL)
				rl2_destroy_style_rule (rule);
			    return 0;
			}
		      rule->style_type = RL2_RASTER_STYLE;
		      rule->style = symbolizer;
		      style->first_rule = rule;
		      style->last_rule = rule;
		      ret = parse_raster_symbolizer (node, symbolizer);
		      *loop = 0;
		      return ret;
		  }
		if (strcmp (name, "CoverageStyle") == 0)
		  {
		      ret = parse_coverage_style (node->children, style);
		      *loop = 0;
		      return ret;
		  }
	    }
	  node = node->next;
      }
    return 0;
}

RL2_PRIVATE rl2CoverageStylePtr
coverage_style_from_xml (char *name, unsigned char *xml)
{
/* attempting to build a Coverage Style object from an SLD/SE XML style */
    rl2PrivCoverageStylePtr style = NULL;
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    int loop = 1;
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) dummySilentError;

    style = rl2_create_default_coverage_style ();
    if (style == NULL)
	return NULL;
    style->name = name;

/* parsing the XML document */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc =
	xmlReadMemory ((const char *) xml, strlen ((const char *) xml),
		       "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  goto error;
      }
    root = xmlDocGetRootElement (xml_doc);
    if (root == NULL)
	goto error;
    if (!find_coverage_style (root, style, &loop))
	goto error;
    xmlFreeDoc (xml_doc);
    free (xml);
    xml = NULL;

    if (style->name == NULL)
	goto error;

    return (rl2CoverageStylePtr) style;

  error:
    if (xml != NULL)
	free (xml);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    if (style != NULL)
	rl2_destroy_coverage_style ((rl2CoverageStylePtr) style);
    return NULL;
}

static int
svg_parameter_name (xmlNodePtr node, const char **name, const char **value)
{
/* return the Name and Value from a <SvgParameter> */
    struct _xmlAttr *attr;

    *name = NULL;
    *value = NULL;
    attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *nm = (const char *) (attr->name);
		if (strcmp (nm, "name") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				*name = (const char *) (text->content);
			}
		  }
	    }
	  attr = attr->next;
      }
    if (name != NULL)
      {
	  xmlNodePtr child = node->children;
	  while (child)
	    {
		if (child->type == XML_TEXT_NODE && child->content != NULL)
		  {
		      *value = (const char *) (child->content);
		      return 1;
		  }
		child = child->next;
	    }
      }
    return 0;
}

static int
is_table_column (const char *name)
{
/* 
 * testing for a special table column name
 * 
 * expected to be declared as:
 *     @colname@ or
 *     $colname$
 * 
*/
    int len;
    if (name == NULL)
	return 0;
    len = strlen (name);
    if (len < 3)
	return 0;
    if (*(name + 0) == '@' && *(name + len - 1) == '@')
	return 1;
    if (*(name + 0) == '$' && *(name + len - 1) == '$')
	return 1;
    return 0;
}

RL2_PRIVATE int
parse_sld_se_stroke_dasharray (const char *value, int *count, double **list)
{
/* parsing a Stroke Dasharray */
    const char *in = value;
    const char *start;
    const char *end;
    double dblarray[128];
    int cnt = 0;
    int len;
    char *buf;
    if (value == NULL)
	return 0;
    while (*in != '\0')
      {
	  start = in;
	  end = in;
	  while (1)
	    {
		if (*end == ' ' || *end == ',' || *end == '\0')
		  {
		      /* found an item delimiter */
		      break;
		  }
		end++;
	    }
	  len = end - start;
	  if (len > 0)
	    {
		buf = malloc (len + 1);
		memcpy (buf, start, len);
		*(buf + len) = '\0';
		dblarray[cnt++] = atof (buf);
		free (buf);
		in = end;
	    }
	  else
	      in++;
      }
    if (cnt <= 0)
	return 0;
/* allocating the Dash list */
    *count = cnt;
    *list = malloc (sizeof (double) * cnt);
    for (len = 0; len < cnt; len++)
	*(*list + len) = dblarray[len];
    return 1;
}

static char *
parse_graphic_online_resource (xmlNodePtr node)
{
/* parsing OnlineResource xlink_href */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "OnlineResource") == 0)
		  {
		      struct _xmlAttr *attr;
		      attr = node->properties;
		      while (attr != NULL)
			{
			    /* attributes */
			    if (attr->type == XML_ATTRIBUTE_NODE)
			      {
				  xmlNode *text;
				  name = (const char *) (attr->name);
				  if (strcmp (name, "href") == 0)
				    {
					text = attr->children;
					if (text != NULL)
					  {
					      if (text->type == XML_TEXT_NODE)
						{
						    int len;
						    char *xlink_href;
						    const char *href =
							(const char
							 *) (text->content);
						    if (href == NULL)
							return NULL;
						    len = strlen (href);
						    xlink_href =
							malloc (len + 1);
						    strcpy (xlink_href, href);
						    return xlink_href;
						}
					  }
				    }
			      }
			    attr = attr->next;
			}
		  }
	    }
	  node = node->next;
      }
    return NULL;
}

static int
parse_graphic_map_item (xmlNodePtr node, rl2PrivColorReplacementPtr repl)
{
/* parsing MapItem (ColorReplacemente/Recode) */
    int ok_data = 0;
    int ok_value = 0;
    xmlNodePtr child;
    while (node)
      {
	  const char *name = (const char *) (node->name);
	  if (strcmp (name, "Data") == 0)
	    {
		child = node->children;
		while (child)
		  {
		      if (child->type == XML_TEXT_NODE
			  && child->content != NULL)
			{
			    repl->index = atoi ((const char *) child->content);
			    ok_data = 1;
			}
		      child = child->next;
		  }
	    }
	  if (strcmp (name, "Value") == 0)
	    {
		child = node->children;
		while (child)
		  {
		      if (child->type == XML_TEXT_NODE
			  && child->content != NULL)
			{
			    const char *value = (const char *) (child->content);
			    if (repl->col_color != NULL)
				free (repl->col_color);
			    repl->col_color = NULL;
			    if (is_table_column (value))
			      {
				  /* table column name instead of value */
				  int len = strlen (value) - 1;
				  repl->col_color = malloc (len + 1);
				  strcpy (repl->col_color, value + 1);
				  len = strlen (repl->col_color);
				  *(repl->col_color + len - 1) = '\0';
			      }
			    else
			      {
				  unsigned char red;
				  unsigned char green;
				  unsigned char blue;
				  if (parse_sld_se_color
				      (value, &red, &green, &blue))
				    {
					repl->red = red;
					repl->green = green;
					repl->blue = blue;
				    }
			      }
			    ok_value = 1;
			}
		      child = child->next;
		  }
	    }
	  node = node->next;
      }
    if (ok_data && ok_value)
	return 1;
    return 0;
}

static void
parse_graphic_color_replacement (xmlNodePtr node, rl2PrivExternalGraphicPtr ext)
{
/* parsing ColorReplacement (Graphic) */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Recode") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    name = (const char *) (child->name);
			    if (strcmp (name, "MapItem") == 0)
			      {
				  rl2PrivColorReplacementPtr repl =
				      rl2_create_default_color_replacement ();
				  if (repl == NULL)
				      return;
				  if (!parse_graphic_map_item
				      (child->children, repl))
				    {
					rl2_destroy_color_replacement (repl);
					return;
				    }
				  if (ext->first == NULL)
				      ext->first = repl;
				  if (ext->last != NULL)
				      ext->last->next = repl;
				  ext->last = repl;
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_mark_well_known_type (xmlNodePtr node, rl2PrivMarkPtr mark)
{
/* parsing Mark WellKnownName */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "WellKnownName") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *type =
				      (const char *) (child->content);
				  if (mark->col_mark_type != NULL)
				      free (mark->col_mark_type);
				  mark->col_mark_type = NULL;
				  if (is_table_column (type))
				    {
					/* table column name instead of value */
					int len = strlen (type) - 1;
					mark->col_mark_type = malloc (len + 1);
					strcpy (mark->col_mark_type, type + 1);
					len = strlen (mark->col_mark_type);
					*(mark->col_mark_type + len - 1) = '\0';
				    }
				  else
				    {
					if (strcasecmp (type, "square") == 0)
					    mark->well_known_type =
						RL2_GRAPHIC_MARK_SQUARE;
					else if (strcasecmp (type, "circle") ==
						 0)
					    mark->well_known_type =
						RL2_GRAPHIC_MARK_CIRCLE;
					else if (strcasecmp (type, "triangle")
						 == 0)
					    mark->well_known_type =
						RL2_GRAPHIC_MARK_TRIANGLE;
					else if (strcasecmp (type, "star") == 0)
					    mark->well_known_type =
						RL2_GRAPHIC_MARK_STAR;
					else if (strcasecmp (type, "cross") ==
						 0)
					    mark->well_known_type =
						RL2_GRAPHIC_MARK_CROSS;
					else if (strcasecmp (type, "x") == 0)
					    mark->well_known_type =
						RL2_GRAPHIC_MARK_X;
					else
					    mark->well_known_type =
						RL2_GRAPHIC_MARK_UNKNOWN;
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_mark_stroke (xmlNodePtr node, rl2PrivMarkPtr mark)
{
/* parsing Mark Stroke */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Stroke") == 0)
		  {
		      xmlNodePtr child = node->children;
		      mark->stroke = rl2_create_default_stroke ();
		      if (mark->stroke == NULL)
			  return;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "GraphicStroke") == 0)
				    {
					xmlNodePtr grandchild = child->children;
					while (grandchild)
					  {
					      name =
						  (const char
						   *) (grandchild->name);
					      if (strcmp (name, "Graphic") == 0)
						{
						    if (mark->stroke->graphic !=
							NULL)
							rl2_destroy_graphic
							    (mark->stroke->
							     graphic);
						    mark->stroke->graphic =
							rl2_create_default_graphic
							();
						    if (mark->stroke->graphic !=
							NULL)
							parse_graphic
							    (grandchild->
							     children,
							     mark->stroke->
							     graphic);
						}
					      grandchild = grandchild->next;
					  }
				    }
				  if (strcmp (name, "SvgParameter") == 0)
				    {
					const char *svg_name;
					const char *svg_value;
					if (!svg_parameter_name
					    (child, &svg_name, &svg_value))
					  {
					      child = child->next;
					      continue;
					  }
					if (strcmp (svg_name, "stroke") == 0)
					  {
					      if (mark->stroke->col_color !=
						  NULL)
						  free (mark->stroke->
							col_color);
					      mark->stroke->col_color = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    mark->stroke->col_color =
							malloc (len + 1);
						    strcpy (mark->stroke->
							    col_color,
							    svg_value + 1);
						    len =
							strlen (mark->stroke->
								col_color);
						    *(mark->stroke->col_color +
						      len - 1) = '\0';
						}
					      else
						{
						    unsigned char red;
						    unsigned char green;
						    unsigned char blue;
						    if (parse_sld_se_color
							(svg_value, &red,
							 &green, &blue))
						      {
							  mark->stroke->red =
							      red;
							  mark->stroke->green =
							      green;
							  mark->stroke->blue =
							      blue;
						      }
						}
					  }
					if (strcmp (svg_name, "stroke-width")
					    == 0)
					  {
					      if (mark->stroke->col_width !=
						  NULL)
						  free (mark->stroke->
							col_width);
					      mark->stroke->col_width = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    mark->stroke->col_width =
							malloc (len + 1);
						    strcpy (mark->stroke->
							    col_width,
							    svg_value + 1);
						    len =
							strlen (mark->stroke->
								col_width);
						    *(mark->stroke->col_width +
						      len - 1) = '\0';
						}
					      else
						{
						    mark->stroke->width =
							atof ((const char *)
							      svg_value);
						}
					  }
					if (strcmp
					    (svg_name, "stroke-linejoin") == 0)
					  {
					      if (mark->stroke->col_join !=
						  NULL)
						  free (mark->stroke->col_join);
					      mark->stroke->col_join = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    mark->stroke->col_join =
							malloc (len + 1);
						    strcpy (mark->stroke->
							    col_join,
							    svg_value + 1);
						    len =
							strlen (mark->stroke->
								col_join);
						    *(mark->stroke->col_join +
						      len - 1) = '\0';
						}
					      else
						{
						    if (strcmp
							(svg_value,
							 "mitre") == 0)
							mark->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_MITRE;
						    if (strcmp
							(svg_value,
							 "round") == 0)
							mark->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_ROUND;
						    if (strcmp
							(svg_value,
							 "bevel") == 0)
							mark->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_BEVEL;
						}
					  }
					if (strcmp
					    (svg_name, "stroke-linecap") == 0)
					  {
					      if (mark->stroke->col_cap != NULL)
						  free (mark->stroke->col_cap);
					      mark->stroke->col_cap = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    mark->stroke->col_cap =
							malloc (len + 1);
						    strcpy (mark->stroke->
							    col_cap,
							    svg_value + 1);
						    len =
							strlen (mark->stroke->
								col_cap);
						    *(mark->stroke->col_cap +
						      len - 1) = '\0';
						}
					      else
						{
						    if (strcmp
							(svg_value,
							 "butt") == 0)
							mark->stroke->linecap =
							    RL2_STROKE_LINECAP_BUTT;
						    if (strcmp
							(svg_value,
							 "round") == 0)
							mark->stroke->linecap =
							    RL2_STROKE_LINECAP_ROUND;
						    if (strcmp
							(svg_value,
							 "square") == 0)
							mark->stroke->linecap =
							    RL2_STROKE_LINECAP_SQUARE;
						}
					  }
					if (strcmp
					    (svg_name, "stroke-dasharray") == 0)
					  {
					      if (mark->stroke->col_dash !=
						  NULL)
						  free (mark->stroke->col_dash);
					      mark->stroke->col_dash = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    mark->stroke->col_dash =
							malloc (len + 1);
						    strcpy (mark->stroke->
							    col_dash,
							    svg_value + 1);
						    len =
							strlen (mark->stroke->
								col_dash);
						    *(mark->stroke->col_dash +
						      len - 1) = '\0';
						}
					      else
						{
						    int dash_count;
						    double *dash_list = NULL;
						    if (parse_sld_se_stroke_dasharray (svg_value, &dash_count, &dash_list))
						      {
							  mark->stroke->
							      dash_count =
							      dash_count;
							  mark->stroke->
							      dash_list =
							      dash_list;
						      }
						}
					  }
					if (strcmp
					    (svg_name,
					     "stroke-dashoffset") == 0)
					  {
					      if (mark->stroke->col_dashoff !=
						  NULL)
						  free (mark->stroke->
							col_dashoff);
					      mark->stroke->col_dashoff = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    mark->stroke->col_dashoff =
							malloc (len + 1);
						    strcpy (mark->stroke->
							    col_dashoff,
							    svg_value + 1);
						    len =
							strlen (mark->stroke->
								col_dashoff);
						    *(mark->stroke->
						      col_dashoff + len - 1) =
					  '\0';
						}
					      else
						  mark->stroke->dash_offset =
						      atof ((const char *)
							    svg_value);
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_mark_fill (xmlNodePtr node, rl2PrivMarkPtr mark)
{
/* parsing Mark Fill */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Fill") == 0)
		  {
		      xmlNodePtr child = node->children;
		      mark->fill = rl2_create_default_fill ();
		      if (mark->fill == NULL)
			  return;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "GraphicFill") == 0)
				    {
					xmlNodePtr grandchild = child->children;
					while (grandchild)
					  {
					      name =
						  (const char
						   *) (grandchild->name);
					      if (strcmp (name, "Graphic") == 0)
						{
						    if (mark->fill->graphic !=
							NULL)
							rl2_destroy_graphic
							    (mark->fill->
							     graphic);
						    mark->fill->graphic =
							rl2_create_default_graphic
							();
						    if (mark->fill->graphic !=
							NULL)
							parse_graphic
							    (grandchild->
							     children,
							     mark->fill->
							     graphic);
						}
					      grandchild = grandchild->next;
					  }
				    }
				  if (strcmp (name, "SvgParameter") == 0)
				    {
					const char *svg_name;
					const char *svg_value;
					if (!svg_parameter_name
					    (child, &svg_name, &svg_value))
					  {
					      child = child->next;
					      continue;
					  }
					if (strcmp (svg_name, "fill") == 0)
					  {
					      if (mark->fill->col_color != NULL)
						  free (mark->fill->col_color);
					      mark->fill->col_color = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    mark->fill->col_color =
							malloc (len + 1);
						    strcpy (mark->fill->
							    col_color,
							    svg_value + 1);
						    len =
							strlen (mark->fill->
								col_color);
						    *(mark->fill->col_color +
						      len - 1) = '\0';
						}
					      else
						{
						    unsigned char red;
						    unsigned char green;
						    unsigned char blue;
						    if (parse_sld_se_color
							(svg_value, &red,
							 &green, &blue))
						      {
							  mark->fill->red = red;
							  mark->fill->green =
							      green;
							  mark->fill->blue =
							      blue;
						      }
						}
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_graphic (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* parsing Graphic */
    while (node)
      {
	  const char *name = (const char *) (node->name);
	  if (strcmp (name, "ExternalGraphic") == 0)
	    {
		xmlNodePtr child = node->children;
		char *href;
		rl2PrivExternalGraphicPtr ext;
		rl2PrivGraphicItemPtr item =
		    rl2_create_default_external_graphic ();
		if (item == NULL)
		    return;
		ext = (rl2PrivExternalGraphicPtr) (item->item);
		href = parse_graphic_online_resource (node->children);
		if (href == NULL)
		  {
		      rl2_destroy_graphic_item (item);
		      return;
		  }
		if (is_table_column (href))
		  {
		      /* table column name instead of value */
		      int len = strlen (href) - 1;
		      ext->col_href = malloc (len + 1);
		      strcpy (ext->col_href, href + 1);
		      len = strlen (ext->col_href);
		      *(ext->col_href + len - 1) = '\0';
		      free (href);
		  }
		else
		    ext->xlink_href = href;
		while (child)
		  {
		      name = (const char *) (child->name);
		      if (strcmp (name, "ColorReplacement") == 0)
			  parse_graphic_color_replacement (child->children,
							   ext);
		      child = child->next;
		  }
		if (graphic->first == NULL)
		    graphic->first = item;
		if (graphic->last != NULL)
		    graphic->last->next = item;
		graphic->last = item;
	    }
	  if (strcmp (name, "Mark") == 0)
	    {
		rl2PrivMarkPtr mark;
		rl2PrivGraphicItemPtr item = rl2_create_default_mark ();
		if (item == NULL)
		    return;
		mark = (rl2PrivMarkPtr) (item->item);
		parse_mark_well_known_type (node->children, mark);
		parse_mark_fill (node->children, mark);
		parse_mark_stroke (node->children, mark);
		if (graphic->first == NULL)
		    graphic->first = item;
		if (graphic->last != NULL)
		    graphic->last->next = item;
		graphic->last = item;
	    }

	  node = node->next;
      }
}

static void
parse_line_stroke (xmlNodePtr node, rl2PrivLineSymbolizerPtr sym)
{
/* parsing LineSymbolizer Stroke */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Stroke") == 0)
		  {
		      xmlNodePtr child = node->children;
		      sym->stroke = rl2_create_default_stroke ();
		      if (sym->stroke == NULL)
			  return;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "GraphicStroke") == 0)
				    {
					xmlNodePtr grandchild = child->children;
					while (grandchild)
					  {
					      name =
						  (const char
						   *) (grandchild->name);
					      if (strcmp (name, "Graphic") == 0)
						{
						    if (sym->stroke->graphic !=
							NULL)
							rl2_destroy_graphic
							    (sym->stroke->
							     graphic);
						    sym->stroke->graphic =
							rl2_create_default_graphic
							();
						    if (sym->stroke->graphic !=
							NULL)
							parse_graphic
							    (grandchild->
							     children,
							     sym->stroke->
							     graphic);
						}
					      grandchild = grandchild->next;
					  }
				    }
				  if (strcmp (name, "SvgParameter") == 0)
				    {
					const char *svg_name;
					const char *svg_value;
					if (!svg_parameter_name
					    (child, &svg_name, &svg_value))
					  {
					      child = child->next;
					      continue;
					  }
					if (strcmp (svg_name, "stroke") == 0)
					  {
					      if (sym->stroke->col_color !=
						  NULL)
						  free (sym->stroke->col_color);
					      sym->stroke->col_color = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_color =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_color,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_color);
						    *(sym->stroke->col_color +
						      len - 1) = '\0';
						}
					      else
						{
						    unsigned char red;
						    unsigned char green;
						    unsigned char blue;
						    if (parse_sld_se_color
							(svg_value, &red,
							 &green, &blue))
						      {
							  sym->stroke->red =
							      red;
							  sym->stroke->green =
							      green;
							  sym->stroke->blue =
							      blue;
						      }
						}
					  }
					if (strcmp
					    (svg_name, "stroke-opacity") == 0)
					  {
					      if (sym->stroke->col_opacity !=
						  NULL)
						  free (sym->stroke->
							col_opacity);
					      sym->stroke->col_opacity = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_opacity =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_opacity,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_opacity);
						    *(sym->stroke->col_opacity +
						      len - 1) = '\0';
						}
					      else
						{
						    sym->stroke->opacity =
							atof ((const char *)
							      svg_value);
						}
					  }
					if (strcmp (svg_name, "stroke-width")
					    == 0)
					  {
					      if (sym->stroke->col_width !=
						  NULL)
						  free (sym->stroke->col_width);
					      sym->stroke->col_width = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_width =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_width,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_width);
						    *(sym->stroke->col_width +
						      len - 1) = '\0';
						}
					      else
						{
						    sym->stroke->width =
							atof ((const char *)
							      svg_value);
						}
					  }
					if (strcmp
					    (svg_name, "stroke-linejoin") == 0)
					  {
					      if (sym->stroke->col_join != NULL)
						  free (sym->stroke->col_join);
					      sym->stroke->col_join = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_join =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_join,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_join);
						    *(sym->stroke->col_join +
						      len - 1) = '\0';
						}
					      else
						{
						    if (strcmp
							(svg_value,
							 "mitre") == 0)
							sym->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_MITRE;
						    if (strcmp
							(svg_value,
							 "round") == 0)
							sym->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_ROUND;
						    if (strcmp
							(svg_value,
							 "bevel") == 0)
							sym->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_BEVEL;
						}
					  }
					if (strcmp
					    (svg_name, "stroke-linecap") == 0)
					  {
					      if (sym->stroke->col_cap != NULL)
						  free (sym->stroke->col_cap);
					      sym->stroke->col_cap = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_cap =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_cap,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_cap);
						    *(sym->stroke->col_cap +
						      len - 1) = '\0';
						}
					      else
						{
						    if (strcmp
							(svg_value,
							 "butt") == 0)
							sym->stroke->linecap =
							    RL2_STROKE_LINECAP_BUTT;
						    if (strcmp
							(svg_value,
							 "round") == 0)
							sym->stroke->linecap =
							    RL2_STROKE_LINECAP_ROUND;
						    if (strcmp
							(svg_value,
							 "square") == 0)
							sym->stroke->linecap =
							    RL2_STROKE_LINECAP_SQUARE;
						}
					  }
					if (strcmp
					    (svg_name, "stroke-dasharray") == 0)
					  {
					      if (sym->stroke->col_dash != NULL)
						  free (sym->stroke->col_dash);
					      sym->stroke->col_dash = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_dash =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_dash,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_dash);
						    *(sym->stroke->col_dash +
						      len - 1) = '\0';
						}
					      else
						{
						    int dash_count;
						    double *dash_list = NULL;
						    if (parse_sld_se_stroke_dasharray (svg_value, &dash_count, &dash_list))
						      {
							  sym->stroke->
							      dash_count =
							      dash_count;
							  sym->stroke->
							      dash_list =
							      dash_list;
						      }
						}
					  }
					if (strcmp
					    (svg_name,
					     "stroke-dashoffset") == 0)
					  {
					      if (sym->stroke->col_dashoff !=
						  NULL)
						  free (sym->stroke->
							col_dashoff);
					      sym->stroke->col_dashoff = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_dashoff =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_dashoff,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_dashoff);
						    *(sym->stroke->col_dashoff +
						      len - 1) = '\0';
						}
					      else
						  sym->stroke->dash_offset =
						      atof ((const char *)
							    svg_value);
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_line_offset (xmlNodePtr node, rl2PrivLineSymbolizerPtr sym)
{
/* parsing LineSymbolizer PerpendicularOffset */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "PerpendicularOffset") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (sym->col_perpoff != NULL)
				      free (sym->col_perpoff);
				  sym->col_perpoff = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					sym->col_perpoff = malloc (len + 1);
					strcpy (sym->col_perpoff, value + 1);
					len = strlen (sym->col_perpoff);
					*(sym->col_perpoff + len - 1) = '\0';
				    }
				  else
				    {
					sym->perpendicular_offset =
					    atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static int
parse_line_symbolizer (xmlNodePtr node, rl2PrivVectorSymbolizerPtr symbolizer)
{
/* attempting to parse an SLD/SE LineSymbolizer */
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivLineSymbolizerPtr sym;
    if (symbolizer == NULL)
	return 0;

/* allocating a default Line Symbolizer */
    item = rl2_create_default_line_symbolizer ();
    if (item == NULL)
	return 0;
    if (item->symbolizer_type != RL2_LINE_SYMBOLIZER
	|| item->symbolizer == NULL)
      {
	  rl2_destroy_vector_symbolizer_item (item);
	  return 0;
      }
    sym = (rl2PrivLineSymbolizerPtr) (item->symbolizer);
    if (symbolizer->first == NULL)
	symbolizer->first = item;
    if (symbolizer->last != NULL)
	symbolizer->last->next = item;
    symbolizer->last = item;

    parse_line_stroke (node->children, sym);
    parse_line_offset (node->children, sym);
    return 1;
}

static void
parse_polygon_stroke (xmlNodePtr node, rl2PrivPolygonSymbolizerPtr sym)
{
/* parsing PolygonSymbolizer Stroke */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Stroke") == 0)
		  {
		      xmlNodePtr child = node->children;
		      sym->stroke = rl2_create_default_stroke ();
		      if (sym->stroke == NULL)
			  return;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "GraphicStroke") == 0)
				    {
					xmlNodePtr grandchild = child->children;
					while (grandchild)
					  {
					      name =
						  (const char
						   *) (grandchild->name);
					      if (strcmp (name, "Graphic") == 0)
						{
						    if (sym->stroke->graphic !=
							NULL)
							rl2_destroy_graphic
							    (sym->stroke->
							     graphic);
						    sym->stroke->graphic =
							rl2_create_default_graphic
							();
						    if (sym->stroke->graphic !=
							NULL)
							parse_graphic
							    (grandchild->
							     children,
							     sym->stroke->
							     graphic);
						}
					      grandchild = grandchild->next;
					  }
				    }
				  if (strcmp (name, "SvgParameter") == 0)
				    {
					const char *svg_name;
					const char *svg_value;
					if (!svg_parameter_name
					    (child, &svg_name, &svg_value))
					  {
					      child = child->next;
					      continue;
					  }
					if (strcmp (svg_name, "stroke") == 0)
					  {
					      if (sym->stroke->col_color !=
						  NULL)
						  free (sym->stroke->col_color);
					      sym->stroke->col_color = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_color =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_color,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_color);
						    *(sym->stroke->col_color +
						      len - 1) = '\0';
						}
					      else
						{
						    unsigned char red;
						    unsigned char green;
						    unsigned char blue;
						    if (parse_sld_se_color
							(svg_value, &red,
							 &green, &blue))
						      {
							  sym->stroke->red =
							      red;
							  sym->stroke->green =
							      green;
							  sym->stroke->blue =
							      blue;
						      }
						}
					  }
					if (strcmp
					    (svg_name, "stroke-opacity") == 0)
					  {
					      if (sym->stroke->col_opacity !=
						  NULL)
						  free (sym->stroke->
							col_opacity);
					      sym->stroke->col_opacity = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_opacity =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_opacity,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_opacity);
						    *(sym->stroke->col_opacity +
						      len - 1) = '\0';
						}
					      else
						{
						    sym->stroke->opacity =
							atof ((const char *)
							      svg_value);
						}
					  }
					if (strcmp (svg_name, "stroke-width")
					    == 0)
					  {
					      if (sym->stroke->col_width !=
						  NULL)
						  free (sym->stroke->col_width);
					      sym->stroke->col_width = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_width =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_width,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_width);
						    *(sym->stroke->col_width +
						      len - 1) = '\0';
						}
					      else
						{
						    sym->stroke->width =
							atof ((const char *)
							      svg_value);
						}
					  }
					if (strcmp
					    (svg_name, "stroke-linejoin") == 0)
					  {
					      if (sym->stroke->col_join != NULL)
						  free (sym->stroke->col_join);
					      sym->stroke->col_join = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_join =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_join,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_join);
						    *(sym->stroke->col_join +
						      len - 1) = '\0';
						}
					      else
						{
						    if (strcmp
							(svg_value,
							 "mitre") == 0)
							sym->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_MITRE;
						    if (strcmp
							(svg_value,
							 "round") == 0)
							sym->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_ROUND;
						    if (strcmp
							(svg_value,
							 "bevel") == 0)
							sym->stroke->linejoin =
							    RL2_STROKE_LINEJOIN_BEVEL;
						}
					  }
					if (strcmp
					    (svg_name, "stroke-linecap") == 0)
					  {
					      if (sym->stroke->col_cap != NULL)
						  free (sym->stroke->col_cap);
					      sym->stroke->col_cap = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_cap =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_cap,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_cap);
						    *(sym->stroke->col_cap +
						      len - 1) = '\0';
						}
					      else
						{
						    if (strcmp
							(svg_value,
							 "butt") == 0)
							sym->stroke->linecap =
							    RL2_STROKE_LINECAP_BUTT;
						    if (strcmp
							(svg_value,
							 "round") == 0)
							sym->stroke->linecap =
							    RL2_STROKE_LINECAP_ROUND;
						    if (strcmp
							(svg_value,
							 "square") == 0)
							sym->stroke->linecap =
							    RL2_STROKE_LINECAP_SQUARE;
						}
					  }
					if (strcmp
					    (svg_name, "stroke-dasharray") == 0)
					  {
					      if (sym->stroke->col_dash != NULL)
						  free (sym->stroke->col_dash);
					      sym->stroke->col_dash = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_dash =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_dash,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_dash);
						    *(sym->stroke->col_dash +
						      len - 1) = '\0';
						}
					      else
						{
						    int dash_count;
						    double *dash_list = NULL;
						    if (parse_sld_se_stroke_dasharray (svg_value, &dash_count, &dash_list))
						      {
							  sym->stroke->
							      dash_count =
							      dash_count;
							  sym->stroke->
							      dash_list =
							      dash_list;
						      }
						}
					  }
					if (strcmp
					    (svg_name,
					     "stroke-dashoffset") == 0)
					  {
					      if (sym->stroke->col_dashoff !=
						  NULL)
						  free (sym->stroke->
							col_dashoff);
					      sym->stroke->col_dashoff = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->stroke->col_dashoff =
							malloc (len + 1);
						    strcpy (sym->stroke->
							    col_dashoff,
							    svg_value + 1);
						    len =
							strlen (sym->stroke->
								col_dashoff);
						    *(sym->stroke->col_dashoff +
						      len - 1) = '\0';
						}
					      else
						  sym->stroke->dash_offset =
						      atof ((const char *)
							    svg_value);
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_polygon_fill (xmlNodePtr node, rl2PrivPolygonSymbolizerPtr sym)
{
/* parsing PolygonSymbolizer Fill */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Fill") == 0)
		  {
		      xmlNodePtr child = node->children;
		      sym->fill = rl2_create_default_fill ();
		      if (sym->fill == NULL)
			  return;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "GraphicFill") == 0)
				    {
					xmlNodePtr grandchild = child->children;
					while (grandchild)
					  {
					      name =
						  (const char
						   *) (grandchild->name);
					      if (strcmp (name, "Graphic") == 0)
						{
						    if (sym->fill->graphic !=
							NULL)
							rl2_destroy_graphic
							    (sym->fill->
							     graphic);
						    sym->fill->graphic =
							rl2_create_default_graphic
							();
						    if (sym->fill->graphic !=
							NULL)
							parse_graphic
							    (grandchild->
							     children,
							     sym->fill->
							     graphic);
						}
					      grandchild = grandchild->next;
					  }
				    }
				  if (strcmp (name, "SvgParameter") == 0)
				    {
					const char *svg_name;
					const char *svg_value;
					if (!svg_parameter_name
					    (child, &svg_name, &svg_value))
					  {
					      child = child->next;
					      continue;
					  }
					if (strcmp (svg_name, "fill") == 0)
					  {
					      if (sym->fill->col_color != NULL)
						  free (sym->fill->col_color);
					      sym->fill->col_color = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->fill->col_color =
							malloc (len + 1);
						    strcpy (sym->fill->
							    col_color,
							    svg_value + 1);
						    len =
							strlen (sym->fill->
								col_color);
						    *(sym->fill->col_color +
						      len - 1) = '\0';
						}
					      else
						{
						    unsigned char red;
						    unsigned char green;
						    unsigned char blue;
						    if (parse_sld_se_color
							(svg_value, &red,
							 &green, &blue))
						      {
							  sym->fill->red = red;
							  sym->fill->green =
							      green;
							  sym->fill->blue =
							      blue;
						      }
						}
					  }
					if (strcmp (svg_name, "fill-opacity")
					    == 0)
					  {
					      if (sym->fill->col_opacity !=
						  NULL)
						  free (sym->fill->col_opacity);
					      sym->fill->col_opacity = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->fill->col_opacity =
							malloc (len + 1);
						    strcpy (sym->fill->
							    col_opacity,
							    svg_value + 1);
						    len =
							strlen (sym->fill->
								col_opacity);
						    *(sym->fill->col_opacity +
						      len - 1) = '\0';
						}
					      else
						{
						    sym->fill->opacity =
							atof (svg_value);
						}
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_polygon_offset (xmlNodePtr node, rl2PrivPolygonSymbolizerPtr sym)
{
/* parsing PolygonSymbolizer PerpendicularOffset */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "PerpendicularOffset") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (sym->col_perpoff != NULL)
				      free (sym->col_perpoff);
				  sym->col_perpoff = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					sym->col_perpoff = malloc (len + 1);
					strcpy (sym->col_perpoff, value + 1);
					len = strlen (sym->col_perpoff);
					*(sym->col_perpoff + len - 1) = '\0';
				    }
				  else
				    {
					sym->perpendicular_offset =
					    atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_polygon_displacement_xy (xmlNodePtr node, rl2PrivPolygonSymbolizerPtr sym)
{
/* parsing PolygonSymbolizer Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "DisplacementX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (sym->col_displ_x != NULL)
				      free (sym->col_displ_x);
				  sym->col_displ_x = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					sym->col_displ_x = malloc (len + 1);
					strcpy (sym->col_displ_x, value + 1);
					len = strlen (sym->col_displ_x);
					*(sym->col_displ_x + len - 1) = '\0';
				    }
				  else
				    {
					sym->displacement_x = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "DisplacementY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (sym->col_displ_y != NULL)
				      free (sym->col_displ_y);
				  sym->col_displ_y = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					sym->col_displ_y = malloc (len + 1);
					strcpy (sym->col_displ_y, value + 1);
					len = strlen (sym->col_displ_y);
					*(sym->col_displ_y + len - 1) = '\0';
				    }
				  else
				    {
					sym->displacement_y = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_polygon_displacement (xmlNodePtr node, rl2PrivPolygonSymbolizerPtr sym)
{
/* parsing PolygonSymbolizer Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Displacement") == 0)
		    parse_polygon_displacement_xy (node->children, sym);
	    }
	  node = node->next;
      }
}

static int
parse_polygon_symbolizer (xmlNodePtr node,
			  rl2PrivVectorSymbolizerPtr symbolizer)
{
/* attempting to parse an SLD/SE PolygonSymbolizer */
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivPolygonSymbolizerPtr sym;
    if (symbolizer == NULL)
	return 0;

/* allocating a default Polygon Symbolizer */
    item = rl2_create_default_polygon_symbolizer ();
    if (item == NULL)
	return 0;
    if (item->symbolizer_type != RL2_POLYGON_SYMBOLIZER
	|| item->symbolizer == NULL)
      {
	  rl2_destroy_vector_symbolizer_item (item);
	  return 0;
      }
    sym = (rl2PrivPolygonSymbolizerPtr) (item->symbolizer);
    if (symbolizer->first == NULL)
	symbolizer->first = item;
    if (symbolizer->last != NULL)
	symbolizer->last->next = item;
    symbolizer->last = item;

    parse_polygon_stroke (node->children, sym);
    parse_polygon_fill (node->children, sym);
    parse_polygon_offset (node->children, sym);
    parse_polygon_displacement (node->children, sym);
    return 1;
}

static void
parse_point_opacity (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* parsing Point Symbolizer Graphic Opacity */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Opacity") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (graphic->col_opacity != NULL)
				      free (graphic->col_opacity);
				  graphic->col_opacity = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					graphic->col_opacity = malloc (len + 1);
					strcpy (graphic->col_opacity,
						value + 1);
					len = strlen (graphic->col_opacity);
					*(graphic->col_opacity +
					  len - 1) = '\0';
				    }
				  else
				    {
					graphic->opacity = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_size (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* parsing Point Symbolizer Graphic Size */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Size") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (graphic->col_size != NULL)
				      free (graphic->col_size);
				  graphic->col_size = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					graphic->col_size = malloc (len + 1);
					strcpy (graphic->col_size, value + 1);
					len = strlen (graphic->col_size);
					*(graphic->col_size + len - 1) = '\0';
				    }
				  else
				    {
					graphic->size = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_rotation (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* parsing Point Symbolizer Graphic Rotation */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rotation") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (graphic->col_rotation != NULL)
				      free (graphic->col_rotation);
				  graphic->col_rotation = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					graphic->col_rotation =
					    malloc (len + 1);
					strcpy (graphic->col_rotation,
						value + 1);
					len = strlen (graphic->col_rotation);
					*(graphic->col_rotation +
					  len - 1) = '\0';
				    }
				  else
				    {
					graphic->rotation = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_anchor_point_xy (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* parsing Point Symbolizer Graphic AnchorPoint XY */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AnchorPointX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (graphic->col_point_x != NULL)
				      free (graphic->col_point_x);
				  graphic->col_point_x = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					graphic->col_point_x = malloc (len + 1);
					strcpy (graphic->col_point_x,
						value + 1);
					len = strlen (graphic->col_point_x);
					*(graphic->col_point_x +
					  len - 1) = '\0';
				    }
				  else
				    {
					graphic->anchor_point_x = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "AnchorPointY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (graphic->col_point_y != NULL)
				      free (graphic->col_point_y);
				  graphic->col_point_y = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					graphic->col_point_y = malloc (len + 1);
					strcpy (graphic->col_point_y,
						value + 1);
					len = strlen (graphic->col_point_y);
					*(graphic->col_point_y +
					  len - 1) = '\0';
				    }
				  else
				    {
					graphic->anchor_point_y = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_anchor_point (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* parsing Point Symbolizer Graphic AnchorPoint */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AnchorPoint") == 0)
		    parse_point_anchor_point_xy (node->children, graphic);
	    }
	  node = node->next;
      }
}

static void
parse_point_displacement_xy (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* parsing Point Symbolizer Graphic Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "DisplacementX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (graphic->col_displ_x != NULL)
				      free (graphic->col_displ_x);
				  graphic->col_displ_x = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					graphic->col_displ_x = malloc (len + 1);
					strcpy (graphic->col_displ_x,
						value + 1);
					len = strlen (graphic->col_displ_x);
					*(graphic->col_displ_x +
					  len - 1) = '\0';
				    }
				  else
				    {
					graphic->displacement_x = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "DisplacementY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (graphic->col_displ_y != NULL)
				      free (graphic->col_displ_y);
				  graphic->col_displ_y = NULL;
				  if (is_table_column (value))
				    {
					/* table column name instead of value */
					int len = strlen (value) - 1;
					graphic->col_displ_y = malloc (len + 1);
					strcpy (graphic->col_displ_y,
						value + 1);
					len = strlen (graphic->col_displ_y);
					*(graphic->col_displ_y +
					  len - 1) = '\0';
				    }
				  else
				    {
					graphic->displacement_y = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_displacement (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* parsing Point Symbolizer Graphic Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Displacement") == 0)
		    parse_point_displacement_xy (node->children, graphic);
	    }
	  node = node->next;
      }
}

static void
parse_point_graphic (xmlNodePtr node, rl2PrivGraphicPtr graphic)
{
/* finding the Graphic tag within a PointSymbolizer */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Graphic") == 0)
		  {
		      parse_graphic (node->children, graphic);
		      parse_point_opacity (node->children, graphic);
		      parse_point_size (node->children, graphic);
		      parse_point_rotation (node->children, graphic);
		      parse_point_anchor_point (node->children, graphic);
		      parse_point_displacement (node->children, graphic);
		      return;
		  }
	    }
	  node = node->next;
      }
}

static int
parse_point_symbolizer (xmlNodePtr node, rl2PrivVectorSymbolizerPtr symbolizer)
{
/* attempting to parse an SLD/SE PointSymbolizer */
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivPointSymbolizerPtr sym;
    if (symbolizer == NULL)
	return 0;

/* allocating a default Point Symbolizer */
    item = rl2_create_default_point_symbolizer ();
    if (item == NULL)
	return 0;
    if (item->symbolizer_type != RL2_POINT_SYMBOLIZER
	|| item->symbolizer == NULL)
      {
	  rl2_destroy_vector_symbolizer_item (item);
	  return 0;
      }
    sym = (rl2PrivPointSymbolizerPtr) (item->symbolizer);
    if (symbolizer->first == NULL)
	symbolizer->first = item;
    if (symbolizer->last != NULL)
	symbolizer->last->next = item;
    symbolizer->last = item;

    if (sym->graphic != NULL)
	rl2_destroy_graphic (sym->graphic);
    sym->graphic = rl2_create_default_graphic ();
    if (sym->graphic != NULL)
	parse_point_graphic (node->children, sym->graphic);
    return 1;
}

static void
parse_text_label (xmlNodePtr node, rl2PrivTextSymbolizerPtr text)
{
/* parsing Text Symbolizer Label */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Label") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  int len;
				  const char *label =
				      (const char *) (child->content);
				  if (text->label != NULL)
				      free (text->label);
				  if (text->col_label != NULL)
				      free (text->col_label);
				  text->label = NULL;
				  text->col_label = NULL;
				  if (is_table_column (label))
				    {
					/* table column name instead of value */
					len = strlen (label) - 1;
					text->col_label = malloc (len + 1);
					strcpy (text->col_label, label + 1);
					len = strlen (text->col_label);
					*(text->col_label + len - 1) = '\0';
					return;
				    }
				  if (label == NULL)
				    {
					text->label = NULL;
					return;
				    }
				  len = strlen (label);
				  text->label = malloc (len + 1);
				  strcpy (text->label, label);
				  return;
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_text_font (xmlNodePtr node, rl2PrivTextSymbolizerPtr sym)
{
/* parsing TextSymbolizer Font */
    int i;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Font") == 0)
		  {
		      xmlNodePtr child = node->children;
		      for (i = 0; i < RL2_MAX_FONT_FAMILIES; i++)
			{
			    if (*(sym->font_families + i) != NULL)
				free (*(sym->font_families + i));
			}
		      sym->font_families_count = 0;
		      for (i = 0; i < RL2_MAX_FONT_FAMILIES; i++)
			  *(sym->font_families + i) = NULL;
		      sym->font_style = RL2_FONT_STYLE_NORMAL;
		      sym->font_weight = RL2_FONT_WEIGHT_NORMAL;
		      sym->font_size = 10.0;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "SvgParameter") == 0)
				    {
					const char *svg_name;
					const char *svg_value;
					if (!svg_parameter_name
					    (child, &svg_name, &svg_value))
					  {
					      child = child->next;
					      continue;
					  }
					if (strcmp (svg_name, "font-family")
					    == 0)
					  {
					      if (sym->col_font != NULL)
						  free (sym->col_font);
					      sym->col_font = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->col_font =
							malloc (len + 1);
						    strcpy (sym->col_font,
							    svg_value + 1);
						    len =
							strlen (sym->col_font);
						    *(sym->col_font + len - 1) =
							'\0';
						}
					      else
						{
						    if (sym->font_families_count
							< RL2_MAX_FONT_FAMILIES)
						      {
							  int idx =
							      sym->
							      font_families_count++;
							  int len =
							      strlen
							      (svg_value);
							  *(sym->font_families +
							    idx) =
					 malloc (len + 1);
							  strcpy (*
								  (sym->
								   font_families
								   + idx),
								  svg_value);
						      }
						}
					  }
					if (strcmp (svg_name, "font-style") ==
					    0)
					  {
					      if (sym->col_style != NULL)
						  free (sym->col_style);
					      sym->col_style = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->col_style =
							malloc (len + 1);
						    strcpy (sym->col_style,
							    svg_value + 1);
						    len =
							strlen (sym->col_style);
						    *(sym->col_style + len -
						      1) = '\0';
						}
					      else
						{
						    if (strcasecmp
							(svg_value,
							 "normal") == 0)
							sym->font_style =
							    RL2_FONT_STYLE_NORMAL;
						    if (strcasecmp
							(svg_value,
							 "italic") == 0)
							sym->font_style =
							    RL2_FONT_STYLE_ITALIC;
						    if (strcasecmp
							(svg_value,
							 "oblique") == 0)
							sym->font_style =
							    RL2_FONT_STYLE_OBLIQUE;
						}
					  }
					if (strcmp (svg_name, "font-weight")
					    == 0)
					  {
					      if (sym->col_weight != NULL)
						  free (sym->col_weight);
					      sym->col_weight = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->col_weight =
							malloc (len + 1);
						    strcpy (sym->col_weight,
							    svg_value + 1);
						    len =
							strlen
							(sym->col_weight);
						    *(sym->col_weight + len -
						      1) = '\0';
						}
					      else
						{
						    if (strcasecmp
							(svg_value,
							 "normal") == 0)
							sym->font_weight =
							    RL2_FONT_WEIGHT_NORMAL;
						    if (strcasecmp
							(svg_value,
							 "bold") == 0)
							sym->font_weight =
							    RL2_FONT_WEIGHT_BOLD;
						}
					  }
					if (strcmp (svg_name, "font-size") == 0)
					  {
					      if (sym->col_size != NULL)
						  free (sym->col_size);
					      sym->col_size = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->col_size =
							malloc (len + 1);
						    strcpy (sym->col_size,
							    svg_value + 1);
						    len =
							strlen (sym->col_size);
						    *(sym->col_size + len - 1) =
							'\0';
						}
					      else
						{
						    sym->font_size =
							atof (svg_value);
						}
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_anchor_point_xy (xmlNodePtr node, rl2PrivPointPlacementPtr place)
{
/* parsing TextSymbolizer label AnchorPoint XY */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AnchorPointX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  if (place->col_point_x != NULL)
				      free (place->col_point_x);
				  place->col_point_x = NULL;
				  if (is_table_column
				      ((const char *) child->content))
				    {
					/* table column name instead of value */
					int len =
					    strlen ((const char
						     *) (child->content)) - 1;
					place->col_point_x = malloc (len + 1);
					strcpy (place->col_point_x,
						(const char *) (child->content)
						+ 1);
					len = strlen (place->col_point_x);
					*(place->col_point_x + len - 1) = '\0';
				    }
				  else
				    {
					place->anchor_point_x =
					    atof ((const char *)
						  child->content);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "AnchorPointY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  if (place->col_point_y != NULL)
				      free (place->col_point_y);
				  place->col_point_y = NULL;
				  if (is_table_column
				      ((const char *) child->content))
				    {
					/* table column name instead of value */
					int len =
					    strlen ((const char
						     *) (child->content)) - 1;
					place->col_point_y = malloc (len + 1);
					strcpy (place->col_point_y,
						(const char *) (child->content)
						+ 1);
					len = strlen (place->col_point_y);
					*(place->col_point_y + len - 1) = '\0';
				    }
				  else
				    {
					place->anchor_point_y =
					    atof ((const char *)
						  child->content);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_anchor_point (xmlNodePtr node, rl2PrivPointPlacementPtr place)
{
/* parsing TextSymbolizer label AnchorPoint */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AnchorPoint") == 0)
		    parse_label_anchor_point_xy (node->children, place);
	    }
	  node = node->next;
      }
}

static void
parse_label_displacement_xy (xmlNodePtr node, rl2PrivPointPlacementPtr place)
{
/* parsing TextSymbolizer label Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "DisplacementX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  if (place->col_displ_x != NULL)
				      free (place->col_displ_x);
				  place->col_displ_x = NULL;
				  if (is_table_column
				      ((const char *) child->content))
				    {
					/* table column name instead of value */
					int len =
					    strlen ((const char
						     *) (child->content)) - 1;
					place->col_displ_x = malloc (len + 1);
					strcpy (place->col_displ_x,
						(const char *) (child->content)
						+ 1);
					len = strlen (place->col_displ_x);
					*(place->col_displ_x + len - 1) = '\0';
				    }
				  else
				    {
					place->displacement_x =
					    atof ((const char *)
						  child->content);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "DisplacementY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  if (place->col_displ_y != NULL)
				      free (place->col_displ_y);
				  place->col_displ_y = NULL;
				  if (is_table_column
				      ((const char *) child->content))
				    {
					/* table column name instead of value */
					int len =
					    strlen ((const char
						     *) (child->content)) - 1;
					place->col_displ_y = malloc (len + 1);
					strcpy (place->col_displ_y,
						(const char *) (child->content)
						+ 1);
					len = strlen (place->col_displ_y);
					*(place->col_displ_y + len - 1) = '\0';
				    }
				  else
				    {
					place->displacement_y =
					    atof ((const char *)
						  child->content);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_displacement (xmlNodePtr node, rl2PrivPointPlacementPtr place)
{
/* parsing TextSymbolizer label Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Displacement") == 0)
		    parse_label_displacement_xy (node->children, place);
	    }
	  node = node->next;
      }
}

static void
parse_label_rotation (xmlNodePtr node, rl2PrivPointPlacementPtr place)
{
/* parsing TextSymbolizer label Rotation */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rotation") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  if (place->col_rotation != NULL)
				      free (place->col_rotation);
				  place->col_rotation = NULL;
				  if (is_table_column
				      ((const char *) child->content))
				    {
					/* table column name instead of value */
					int len =
					    strlen ((const char
						     *) (child->content)) - 1;
					place->col_rotation = malloc (len + 1);
					strcpy (place->col_rotation,
						(const char *) (child->content)
						+ 1);
					len = strlen (place->col_rotation);
					*(place->col_rotation + len - 1) = '\0';
				    }
				  else
				    {
					place->rotation =
					    atof ((const char *)
						  child->content);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_placement (xmlNodePtr node, rl2PrivPointPlacementPtr place)
{
/* parsing Point Placement (TextSymbolizer) */
    parse_label_anchor_point (node->children, place);
    parse_label_displacement (node->children, place);
    parse_label_rotation (node->children, place);
}

static void
parse_label_perpendicular_offset (xmlNodePtr node,
				  rl2PrivLinePlacementPtr place)
{
/* parsing TextSymbolizer label PerpendicularOffset */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "PerpendicularOffset") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  if (place->col_perpoff != NULL)
				      free (place->col_perpoff);
				  place->col_perpoff = NULL;
				  if (is_table_column
				      ((const char *) child->content))
				    {
					/* table column name instead of value */
					int len =
					    strlen ((const char
						     *) (child->content)) - 1;
					place->col_perpoff = malloc (len + 1);
					strcpy (place->col_perpoff,
						(const char *) (child->content)
						+ 1);
					len = strlen (place->col_perpoff);
					*(place->col_perpoff + len - 1) = '\0';
				    }
				  else
				    {
					place->perpendicular_offset =
					    atof ((const char *)
						  child->content);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_initial_gap (xmlNodePtr node, rl2PrivLinePlacementPtr place)
{
/* parsing TextSymbolizer label InitialGap */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "InitialGap") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  if (place->col_inigap != NULL)
				      free (place->col_inigap);
				  place->col_inigap = NULL;
				  if (is_table_column
				      ((const char *) child->content))
				    {
					/* table column name instead of value */
					int len =
					    strlen ((const char
						     *) (child->content)) - 1;
					place->col_inigap = malloc (len + 1);
					strcpy (place->col_inigap,
						(const char *) (child->content)
						+ 1);
					len = strlen (place->col_inigap);
					*(place->col_inigap + len - 1) = '\0';
				    }
				  else
				    {
					place->initial_gap =
					    atof ((const char *)
						  child->content);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_gap (xmlNodePtr node, rl2PrivLinePlacementPtr place)
{
/* parsing TextSymbolizer label Gap */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Gap") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  if (place->col_gap != NULL)
				      free (place->col_gap);
				  place->col_gap = NULL;
				  if (is_table_column
				      ((const char *) child->content))
				    {
					/* table column name instead of value */
					int len =
					    strlen ((const char
						     *) (child->content)) - 1;
					place->col_gap = malloc (len + 1);
					strcpy (place->col_gap,
						(const char *) (child->content)
						+ 1);
					len = strlen (place->col_gap);
					*(place->col_gap + len - 1) = '\0';
				    }
				  else
				    {
					place->gap =
					    atof ((const char *)
						  child->content);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_is_repeated (xmlNodePtr node, rl2PrivLinePlacementPtr place)
{
/* parsing TextSymbolizer label IsRepeated */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "IsRepeated") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) (child->content);
				  if (strcasecmp (value, "true") == 0)
				      place->is_repeated = 1;
				  if (atoi (value) != 0)
				      place->is_repeated = 1;
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_is_aligned (xmlNodePtr node, rl2PrivLinePlacementPtr place)
{
/* parsing TextSymbolizer label IsAligned */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "IsAligned") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) (child->content);
				  if (strcasecmp (value, "true") == 0)
				      place->is_aligned = 1;
				  if (atoi (value) != 0)
				      place->is_aligned = 1;
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_generalize_line (xmlNodePtr node, rl2PrivLinePlacementPtr place)
{
/* parsing TextSymbolizer label GeneralizeLine */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "GeneralizeLine") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  const char *value =
				      (const char *) (child->content);
				  if (strcasecmp (value, "true") == 0)
				      place->generalize_line = 1;
				  if (atoi (value) != 0)
				      place->generalize_line = 1;
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_line_placement (xmlNodePtr node, rl2PrivLinePlacementPtr place)
{
/* parsing Line Placement (TextSymbolizer) */
    parse_label_perpendicular_offset (node->children, place);
    parse_label_is_repeated (node->children, place);
    parse_label_initial_gap (node->children, place);
    parse_label_gap (node->children, place);
    parse_label_is_aligned (node->children, place);
    parse_label_generalize_line (node->children, place);
}

static void
parse_text_label_placement (xmlNodePtr node, rl2PrivTextSymbolizerPtr sym)
{
/* parsing TextSymbolizer LabelPlacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "LabelPlacement") == 0)
		  {
		      xmlNodePtr child = node->children;
		      if (sym->label_placement_type ==
			  RL2_LABEL_PLACEMENT_POINT
			  && sym->label_placement != NULL)
			  rl2_destroy_point_placement ((rl2PrivPointPlacementPtr) (sym->label_placement));
		      if (sym->label_placement_type ==
			  RL2_LABEL_PLACEMENT_LINE
			  && sym->label_placement != NULL)
			  rl2_destroy_line_placement ((rl2PrivLinePlacementPtr)
						      (sym->label_placement));
		      sym->label_placement_type = RL2_LABEL_PLACEMENT_UNKNOWN;
		      sym->label_placement = NULL;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "PointPlacement") == 0)
				    {
					if (sym->label_placement_type ==
					    RL2_LABEL_PLACEMENT_POINT
					    && sym->label_placement != NULL)
					    rl2_destroy_point_placement ((rl2PrivPointPlacementPtr) (sym->label_placement));
					if (sym->label_placement_type ==
					    RL2_LABEL_PLACEMENT_LINE
					    && sym->label_placement != NULL)
					    rl2_destroy_line_placement ((rl2PrivLinePlacementPtr) (sym->label_placement));
					sym->label_placement_type =
					    RL2_LABEL_PLACEMENT_POINT;
					sym->label_placement =
					    rl2_create_default_point_placement
					    ();
					parse_point_placement (child,
							       (rl2PrivPointPlacementPtr)
							       (sym->
								label_placement));
				    }
				  if (strcmp (name, "LinePlacement") == 0)
				    {
					if (sym->label_placement_type ==
					    RL2_LABEL_PLACEMENT_POINT
					    && sym->label_placement != NULL)
					    rl2_destroy_point_placement ((rl2PrivPointPlacementPtr) (sym->label_placement));
					if (sym->label_placement_type ==
					    RL2_LABEL_PLACEMENT_LINE
					    && sym->label_placement != NULL)
					    rl2_destroy_line_placement ((rl2PrivLinePlacementPtr) (sym->label_placement));
					sym->label_placement_type =
					    RL2_LABEL_PLACEMENT_LINE;
					sym->label_placement =
					    rl2_create_default_line_placement
					    ();
					parse_line_placement (child,
							      (rl2PrivLinePlacementPtr)
							      (sym->
							       label_placement));
				    };
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_halo_fill (xmlNodePtr node, rl2PrivHaloPtr halo)
{
/* parsing Halo Fill */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Fill") == 0)
		  {
		      xmlNodePtr child = node->children;
		      if (halo->fill != NULL)
			  rl2_destroy_fill (halo->fill);
		      halo->fill = rl2_create_default_fill ();
		      halo->fill->red = 0xff;
		      halo->fill->green = 0xff;
		      halo->fill->blue = 0xff;
		      if (halo->fill == NULL)
			  return;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "SvgParameter") == 0)
				    {
					const char *svg_name;
					const char *svg_value;
					if (!svg_parameter_name
					    (child, &svg_name, &svg_value))
					  {
					      child = child->next;
					      continue;
					  }
					if (strcmp (svg_name, "fill") == 0)
					  {
					      if (halo->fill->col_color != NULL)
						  free (halo->fill->col_color);
					      halo->fill->col_color = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    halo->fill->col_color =
							malloc (len + 1);
						    strcpy (halo->fill->
							    col_color,
							    svg_value + 1);
						    len =
							strlen (halo->fill->
								col_color);
						    *(halo->fill->col_color +
						      len - 1) = '\0';
						}
					      else
						{
						    unsigned char red;
						    unsigned char green;
						    unsigned char blue;
						    if (parse_sld_se_color
							(svg_value, &red,
							 &green, &blue))
						      {
							  halo->fill->red = red;
							  halo->fill->green =
							      green;
							  halo->fill->blue =
							      blue;
						      }
						}
					  }
					if (strcmp (svg_name, "fill-opacity")
					    == 0)
					  {
					      if (halo->fill->col_opacity !=
						  NULL)
						  free (halo->fill->
							col_opacity);
					      halo->fill->col_opacity = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    halo->fill->col_opacity =
							malloc (len + 1);
						    strcpy (halo->fill->
							    col_opacity,
							    svg_value + 1);
						    len =
							strlen (halo->fill->
								col_opacity);
						    *(halo->fill->col_opacity +
						      len - 1) = '\0';
						}
					      else
						{
						    halo->fill->opacity =
							atof (svg_value);
						}
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_text_halo (xmlNodePtr node, rl2PrivTextSymbolizerPtr sym)
{
/* parsing TextSymbolizer Halo */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Halo") == 0)
		  {
		      xmlNodePtr child = node->children;
		      sym->halo = rl2_create_default_halo ();
		      if (sym->halo == NULL)
			  return;
		      sym->halo->fill = rl2_create_default_fill ();
		      sym->halo->fill->red = 255;
		      sym->halo->fill->green = 255;
		      sym->halo->fill->blue = 255;
		      sym->halo->fill->opacity = 1.0;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "Radius") == 0)
				    {
					xmlNodePtr grandchild = child->children;
					while (grandchild)
					  {
					      if (grandchild->type ==
						  XML_TEXT_NODE
						  && grandchild->content !=
						  NULL)
						{
						    const char *radius =
							(const char
							 *)
							(grandchild->content);
						    if (sym->halo->col_radius !=
							NULL)
							free (sym->halo->
							      col_radius);
						    sym->halo->col_radius =
							NULL;
						    if (is_table_column
							(radius))
						      {
							  /* table column name instead of value */
							  int len =
							      strlen (radius) -
							      1;
							  sym->halo->
							      col_radius =
							      malloc (len + 1);
							  strcpy (sym->halo->
								  col_radius,
								  radius + 1);
							  len =
							      strlen (sym->
								      halo->
								      col_radius);
							  *(sym->halo->
							    col_radius + len -
							    1) = '\0';
						      }
						    else
						      {
							  sym->halo->radius =
							      atof (radius);
						      }
						}
					      grandchild = grandchild->next;
					  }
				    }
				  if (strcmp (name, "Fill") == 0)
				      parse_halo_fill (child, sym->halo);
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_text_fill (xmlNodePtr node, rl2PrivTextSymbolizerPtr sym)
{
/* parsing TextSymbolizer Fill */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Fill") == 0)
		  {
		      xmlNodePtr child = node->children;
		      sym->fill = rl2_create_default_fill ();
		      sym->fill->red = 0x00;
		      sym->fill->green = 0x00;
		      sym->fill->blue = 0x00;
		      sym->fill->red = 0;
		      sym->fill->green = 0;
		      sym->fill->blue = 0;
		      sym->fill->opacity = 1.0;
		      if (sym->fill == NULL)
			  return;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  name = (const char *) (child->name);
				  if (strcmp (name, "SvgParameter") == 0)
				    {
					const char *svg_name;
					const char *svg_value;
					if (!svg_parameter_name
					    (child, &svg_name, &svg_value))
					  {
					      child = child->next;
					      continue;
					  }
					if (strcmp (svg_name, "fill") == 0)
					  {
					      if (sym->fill->col_color != NULL)
						  free (sym->fill->col_color);
					      sym->fill->col_color = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->fill->col_color =
							malloc (len + 1);
						    strcpy (sym->fill->
							    col_color,
							    svg_value + 1);
						    len =
							strlen (sym->fill->
								col_color);
						    *(sym->fill->col_color +
						      len - 1) = '\0';
						}
					      else
						{
						    unsigned char red;
						    unsigned char green;
						    unsigned char blue;
						    if (parse_sld_se_color
							(svg_value, &red,
							 &green, &blue))
						      {
							  sym->fill->red = red;
							  sym->fill->green =
							      green;
							  sym->fill->blue =
							      blue;
						      }
						}
					  }
					if (strcmp (svg_name, "fill-opacity")
					    == 0)
					  {
					      if (sym->fill->col_opacity !=
						  NULL)
						  free (sym->fill->col_opacity);
					      sym->fill->col_opacity = NULL;
					      if (is_table_column (svg_value))
						{
						    /* table column name instead of value */
						    int len =
							strlen (svg_value) - 1;
						    sym->fill->col_opacity =
							malloc (len + 1);
						    strcpy (sym->fill->
							    col_opacity,
							    svg_value + 1);
						    len =
							strlen (sym->fill->
								col_opacity);
						    *(sym->fill->col_opacity +
						      len - 1) = '\0';
						}
					      else
						{
						    sym->fill->opacity =
							atof (svg_value);
						}
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static int
parse_text_symbolizer (xmlNodePtr node, rl2PrivVectorSymbolizerPtr symbolizer)
{
/* attempting to parse an SLD/SE TextSymbolizer */
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivTextSymbolizerPtr sym;
    if (symbolizer == NULL)
	return 0;

/* allocating a default Text Symbolizer */
    item = rl2_create_default_text_symbolizer ();
    if (item == NULL)
	return 0;
    if (item->symbolizer_type != RL2_TEXT_SYMBOLIZER
	|| item->symbolizer == NULL)
      {
	  rl2_destroy_vector_symbolizer_item (item);
	  return 0;
      }
    sym = (rl2PrivTextSymbolizerPtr) (item->symbolizer);
    if (symbolizer->first == NULL)
	symbolizer->first = item;
    if (symbolizer->last != NULL)
	symbolizer->last->next = item;
    symbolizer->last = item;

    parse_text_label (node->children, sym);
    parse_text_font (node->children, sym);
    parse_text_label_placement (node->children, sym);
    parse_text_halo (node->children, sym);
    parse_text_fill (node->children, sym);
    return 1;
}

static void
parse_sld_se_filter_single (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* parsing Rule Filter single arg */
    const char *name = NULL;
    const char *value = NULL;
    rl2PrivRuleSingleArgPtr arg =
	(rl2PrivRuleSingleArgPtr) (rule->comparison_args);
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		xmlNodePtr child;
		const char *nm = (const char *) (node->name);
		if (strcmp (nm, "PropertyName") == 0)
		  {
		      child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				name = (const char *) (child->content);
			    child = child->next;
			}
		  }
		if (strcmp (nm, "Literal") == 0)
		  {
		      child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				value = (const char *) (child->content);
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
    if (name == NULL || value == NULL)
      {
	  if (rule->column_name != NULL)
	      free (rule->column_name);
	  rule->column_name = NULL;
	  if (arg->value != NULL)
	      free (arg->value);
	  arg->value = NULL;
      }
    else
      {
	  int len;
	  if (rule->column_name != NULL)
	      free (rule->column_name);
	  if (arg->value != NULL)
	      free (arg->value);
	  len = strlen (name);
	  rule->column_name = malloc (len + 1);
	  strcpy (rule->column_name, name);
	  len = strlen (value);
	  arg->value = malloc (len + 1);
	  strcpy (arg->value, value);
      }
}

static void
parse_sld_se_filter_like (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* parsing Rule Filter Like arg */
    const char *nm;
    const char *name = NULL;
    const char *wild_card = NULL;
    const char *single_char = NULL;
    const char *escape_char = NULL;
    const char *value = NULL;
    rl2PrivRuleLikeArgsPtr args =
	(rl2PrivRuleLikeArgsPtr) (rule->comparison_args);
    struct _xmlAttr *attr;

    attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		xmlNode *text;
		nm = (const char *) (attr->name);
		if (strcmp (nm, "wildCard") == 0)
		  {
		      text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  wild_card = (const char *) (text->content);

			      }
			}
		  }
		if (strcmp (nm, "singleChar") == 0)
		  {
		      text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  single_char = (const char *) (text->content);

			      }
			}
		  }
		if (strcmp (nm, "escapeChar") == 0)
		  {
		      text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  escape_char = (const char *) (text->content);

			      }
			}
		  }
	    }
	  attr = attr->next;
      }

    node = node->children;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		xmlNodePtr child;
		const char *nm = (const char *) (node->name);
		if (strcmp (nm, "PropertyName") == 0)
		  {
		      child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				name = (const char *) (child->content);
			    child = child->next;
			}
		  }
		if (strcmp (nm, "Literal") == 0)
		  {
		      child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				value = (const char *) (child->content);
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
    if (name == NULL || wild_card == NULL || single_char == NULL
	|| escape_char == NULL || value == NULL)
      {
	  if (rule->column_name != NULL)
	      free (rule->column_name);
	  rule->column_name = NULL;
	  if (args->wild_card != NULL)
	      free (args->wild_card);
	  args->wild_card = NULL;
	  if (args->single_char != NULL)
	      free (args->single_char);
	  args->single_char = NULL;
	  if (args->escape_char != NULL)
	      free (args->escape_char);
	  args->escape_char = NULL;
	  if (args->value != NULL)
	      free (args->value);
	  args->value = NULL;
      }
    else
      {
	  int len;
	  if (rule->column_name != NULL)
	      free (rule->column_name);
	  if (args->wild_card != NULL)
	      free (args->wild_card);
	  if (args->single_char != NULL)
	      free (args->single_char);
	  if (args->escape_char != NULL)
	      free (args->escape_char);
	  if (args->value != NULL)
	      free (args->value);
	  if (args->value != NULL)
	      free (args->value);
	  len = strlen (name);
	  rule->column_name = malloc (len + 1);
	  strcpy (rule->column_name, name);
	  len = strlen (wild_card);
	  args->wild_card = malloc (len + 1);
	  strcpy (args->wild_card, wild_card);
	  len = strlen (single_char);
	  args->single_char = malloc (len + 1);
	  strcpy (args->single_char, single_char);
	  len = strlen (escape_char);
	  args->escape_char = malloc (len + 1);
	  strcpy (args->escape_char, escape_char);
	  len = strlen (value);
	  args->value = malloc (len + 1);
	  strcpy (args->value, value);
      }
}

static void
parse_sld_se_filter_between (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* parsing Rule Filter between args */
    const char *name = NULL;
    const char *lower = NULL;
    const char *upper = NULL;
    rl2PrivRuleBetweenArgsPtr args =
	(rl2PrivRuleBetweenArgsPtr) (rule->comparison_args);
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		xmlNodePtr child;
		const char *nm = (const char *) (node->name);
		if (strcmp (nm, "PropertyName") == 0)
		  {
		      child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				name = (const char *) (child->content);
			    child = child->next;
			}
		  }
		if (strcmp (nm, "LowerBoundary") == 0)
		  {
		      child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  nm = (const char *) (child->name);
				  if (strcmp (nm, "Literal") == 0)
				    {
					xmlNodePtr grandchild = child->children;
					while (grandchild)
					  {
					      if (grandchild->type ==
						  XML_TEXT_NODE
						  && grandchild->content !=
						  NULL)
						  lower =
						      (const char
						       *) (grandchild->content);
					      grandchild = grandchild->next;
					  }
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (nm, "UpperBoundary") == 0)
		  {
		      child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  nm = (const char *) (child->name);
				  if (strcmp (nm, "Literal") == 0)
				    {
					xmlNodePtr grandchild = child->children;
					while (grandchild)
					  {
					      if (grandchild->type ==
						  XML_TEXT_NODE
						  && grandchild->content !=
						  NULL)
						  upper =
						      (const char
						       *) (grandchild->content);
					      grandchild = grandchild->next;
					  }
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
    if (name == NULL || lower == NULL || upper == NULL)
      {
	  if (rule->column_name != NULL)
	      free (rule->column_name);
	  rule->column_name = NULL;
	  if (args->lower != NULL)
	      free (args->lower);
	  args->lower = NULL;
	  if (args->upper != NULL)
	      free (args->upper);
	  args->upper = NULL;
      }
    else
      {
	  int len;
	  if (rule->column_name != NULL)
	      free (rule->column_name);
	  if (args->lower != NULL)
	      free (args->lower);
	  if (args->upper != NULL)
	      free (args->upper);
	  len = strlen (name);
	  rule->column_name = malloc (len + 1);
	  strcpy (rule->column_name, name);
	  len = strlen (lower);
	  args->lower = malloc (len + 1);
	  strcpy (args->lower, lower);
	  len = strlen (upper);
	  args->upper = malloc (len + 1);
	  strcpy (args->upper, upper);
      }
}

static void
parse_sld_se_filter_null (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* parsing Rule Filter NULL */
    const char *name = NULL;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		xmlNodePtr child;
		const char *nm = (const char *) (node->name);
		if (strcmp (nm, "PropertyName") == 0)
		  {
		      child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
				name = (const char *) (child->content);
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
    if (name == NULL)
      {
	  if (rule->column_name != NULL)
	      free (rule->column_name);
	  rule->column_name = NULL;
      }
    else
      {
	  int len;
	  if (rule->column_name != NULL)
	      free (rule->column_name);
	  len = strlen (name);
	  rule->column_name = malloc (len + 1);
	  strcpy (rule->column_name, name);
      }
}

static void
parse_sld_se_filter_args (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* parsing Rule Filter args */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "PropertyIsEqualTo") == 0)
		  {
		      rule->comparison_args =
			  rl2_create_default_rule_single_arg ();
		      rule->comparison_op = RL2_COMPARISON_EQ;
		      parse_sld_se_filter_single (node->children, rule);
		  }
		if (strcmp (name, "PropertyIsNotEqualTo") == 0)
		  {
		      rule->comparison_args =
			  rl2_create_default_rule_single_arg ();
		      rule->comparison_op = RL2_COMPARISON_NE;
		      parse_sld_se_filter_single (node->children, rule);
		  }
		if (strcmp (name, "PropertyIsLessThan") == 0)
		  {
		      rule->comparison_args =
			  rl2_create_default_rule_single_arg ();
		      rule->comparison_op = RL2_COMPARISON_LT;
		      parse_sld_se_filter_single (node->children, rule);
		  }
		if (strcmp (name, "PropertyIsGreaterThan") == 0)
		  {
		      rule->comparison_args =
			  rl2_create_default_rule_single_arg ();
		      rule->comparison_op = RL2_COMPARISON_GT;
		      parse_sld_se_filter_single (node->children, rule);
		  }
		if (strcmp (name, "PropertyIsLessThanOrEqualTo") == 0)
		  {
		      rule->comparison_args =
			  rl2_create_default_rule_single_arg ();
		      rule->comparison_op = RL2_COMPARISON_LE;
		      parse_sld_se_filter_single (node->children, rule);
		  }
		if (strcmp (name, "PropertyIsGreaterThanOrEqualTo") == 0)
		  {
		      rule->comparison_args =
			  rl2_create_default_rule_single_arg ();
		      rule->comparison_op = RL2_COMPARISON_GE;
		      parse_sld_se_filter_single (node->children, rule);
		  }
		if (strcmp (name, "PropertyIsLike") == 0)
		  {
		      rule->comparison_args =
			  rl2_create_default_rule_like_args ();
		      rule->comparison_op = RL2_COMPARISON_LIKE;
		      parse_sld_se_filter_like (node, rule);
		  }
		if (strcmp (name, "PropertyIsBetween") == 0)
		  {
		      rule->comparison_args =
			  rl2_create_default_rule_between_args ();
		      rule->comparison_op = RL2_COMPARISON_BETWEEN;
		      parse_sld_se_filter_between (node->children, rule);
		  }
		if (strcmp (name, "PropertyIsNull") == 0)
		  {
		      rule->comparison_op = RL2_COMPARISON_NULL;
		      parse_sld_se_filter_null (node->children, rule);
		  }
	    }
	  node = node->next;
      }
}

static void
parse_sld_se_filter (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* parsing Rule Filter */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Filter") == 0)
		    parse_sld_se_filter_args (node->children, rule);
		if (strcmp (name, "ElseFilter") == 0)
		    rule->else_rule = 1;
	    }
	  node = node->next;
      }
}

static int
parse_vector_style_rule (xmlNodePtr node, rl2PrivStyleRulePtr rule)
{
/* attempting to parse an SLD/SE Style Rule (vector type) */
    int ret;
    int count = 0;
    rl2PrivVectorSymbolizerPtr symb = rl2_create_default_vector_symbolizer ();
    parse_sld_se_filter (node, rule);
    parse_sld_se_min_scale_denominator (node, rule);
    parse_sld_se_max_scale_denominator (node, rule);
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "PointSymbolizer") == 0)
		  {
		      ret = parse_point_symbolizer (node, symb);
		      if (ret)
			{
			    rule->style_type = RL2_VECTOR_STYLE;
			    rule->style = symb;
			    count++;
			}
		  }
		if (strcmp (name, "LineSymbolizer") == 0)
		  {
		      ret = parse_line_symbolizer (node, symb);
		      if (ret)
			{
			    rule->style_type = RL2_VECTOR_STYLE;
			    rule->style = symb;
			    count++;
			}
		  }
		if (strcmp (name, "PolygonSymbolizer") == 0)
		  {
		      ret = parse_polygon_symbolizer (node, symb);
		      if (ret)
			{
			    rule->style_type = RL2_VECTOR_STYLE;
			    rule->style = symb;
			    count++;
			}
		  }
		if (strcmp (name, "TextSymbolizer") == 0)
		  {
		      ret = parse_text_symbolizer (node, symb);
		      if (ret)
			{
			    rule->style_type = RL2_VECTOR_STYLE;
			    rule->style = symb;
			    count++;
			}
		  }
	    }
	  node = node->next;
      }
    if (count <= 0)
      {
	  rl2_destroy_vector_symbolizer (symb);
	  return 0;
      }
    return 1;
}

static int
parse_feature_type_style (xmlNodePtr node, rl2PrivFeatureTypeStylePtr style)
{
/* parsing an SLD/SE FeatureType Style */
    int count = 0;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rule") == 0)
		  {
		      rl2PrivStyleRulePtr rule =
			  rl2_create_default_style_rule ();
		      int ret = parse_vector_style_rule (node->children, rule);
		      if (ret)
			{
			    if (rule->else_rule)
			      {
				  /* special case: ElseRule */
				  if (style->else_rule != NULL)
				      rl2_destroy_style_rule (style->else_rule);
				  style->else_rule = rule;
			      }
			    else
			      {
				  /* ordinary Rule */
				  if (style->first_rule == NULL)
				      style->first_rule = rule;
				  if (style->last_rule != NULL)
				      style->last_rule->next = rule;
				  style->last_rule = rule;
			      }
			    count++;
			}
		      else
			  rl2_destroy_style_rule (rule);
		  }
	    }
	  node = node->next;
      }
    if (count <= 0)
	return 0;
    return 1;
}

static int
find_feature_type_style (xmlNodePtr node, rl2PrivFeatureTypeStylePtr style,
			 int *loop)
{
/* recursivly searching an SLD/SE VectorSymbolizer */
    int ret;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "PointSymbolizer") == 0)
		  {
		      rl2PrivVectorSymbolizerPtr symb =
			  rl2_create_default_vector_symbolizer ();
		      ret = parse_point_symbolizer (node, symb);
		      if (ret)
			{
			    rl2PrivStyleRulePtr rule =
				rl2_create_default_style_rule ();
			    if (rule == NULL)
			      {
				  rl2_destroy_vector_symbolizer (symb);
				  ret = 0;
			      }
			    rule->style_type = RL2_VECTOR_STYLE;
			    rule->style = symb;
			    style->else_rule = rule;
			}
		      else
			  rl2_destroy_vector_symbolizer (symb);
		      *loop = 0;
		      return ret;
		  }
		if (strcmp (name, "LineSymbolizer") == 0)
		  {
		      rl2PrivVectorSymbolizerPtr symb =
			  rl2_create_default_vector_symbolizer ();
		      ret = parse_line_symbolizer (node, symb);
		      if (ret)
			{
			    rl2PrivStyleRulePtr rule =
				rl2_create_default_style_rule ();
			    if (rule == NULL)
			      {
				  rl2_destroy_vector_symbolizer (symb);
				  ret = 0;
			      }
			    rule->style_type = RL2_VECTOR_STYLE;
			    rule->style = symb;
			    style->else_rule = rule;
			}
		      else
			  rl2_destroy_vector_symbolizer (symb);
		      *loop = 0;
		      return ret;
		  }
		if (strcmp (name, "PolygonSymbolizer") == 0)
		  {
		      rl2PrivVectorSymbolizerPtr symb =
			  rl2_create_default_vector_symbolizer ();
		      ret = parse_polygon_symbolizer (node, symb);
		      if (ret)
			{
			    rl2PrivStyleRulePtr rule =
				rl2_create_default_style_rule ();
			    if (rule == NULL)
			      {
				  rl2_destroy_vector_symbolizer (symb);
				  ret = 0;
			      }
			    rule->style_type = RL2_VECTOR_STYLE;
			    rule->style = symb;
			    style->else_rule = rule;
			}
		      else
			  rl2_destroy_vector_symbolizer (symb);
		      *loop = 0;
		      return ret;
		  }
		if (strcmp (name, "TextSymbolizer") == 0)
		  {
		      rl2PrivVectorSymbolizerPtr symb =
			  rl2_create_default_vector_symbolizer ();
		      ret = parse_text_symbolizer (node, symb);
		      if (ret)
			{
			    rl2PrivStyleRulePtr rule =
				rl2_create_default_style_rule ();
			    if (rule == NULL)
			      {
				  rl2_destroy_vector_symbolizer (symb);
				  ret = 0;
			      }
			    rule->style_type = RL2_VECTOR_STYLE;
			    rule->style = symb;
			    style->else_rule = rule;
			}
		      else
			  rl2_destroy_vector_symbolizer (symb);
		      *loop = 0;
		      return ret;
		  }
		if (strcmp (name, "FeatureTypeStyle") == 0)
		  {
		      ret = parse_feature_type_style (node->children, style);
		      *loop = 0;
		      return ret;
		  }
	    }
	  node = node->next;
      }
    return 0;
}

static int
count_point_symbolizer_column_names (rl2PointSymbolizerPtr point)
{
/* counting Point Symbolizer column names) */
    int count = 0;
    int index;
    int max;
    if (rl2_point_symbolizer_get_col_opacity (point) != NULL)
	count++;
    if (rl2_point_symbolizer_get_col_size (point) != NULL)
	count++;
    if (rl2_point_symbolizer_get_col_rotation (point) != NULL)
	count++;
    if (rl2_point_symbolizer_get_col_anchor_point_x (point) != NULL)
	count++;
    if (rl2_point_symbolizer_get_col_anchor_point_y (point) != NULL)
	count++;
    if (rl2_point_symbolizer_get_col_displacement_x (point) != NULL)
	count++;
    if (rl2_point_symbolizer_get_col_displacement_y (point) != NULL)
	count++;
    if (rl2_point_symbolizer_get_count (point, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int repl_index;
	  int max_repl;
	  if (rl2_point_symbolizer_mark_get_col_well_known_type (point, index)
	      != NULL)
	      count++;
	  if (rl2_point_symbolizer_mark_get_col_stroke_color (point, index) !=
	      NULL)
	      count++;
	  if (rl2_point_symbolizer_mark_get_col_stroke_width (point, index) !=
	      NULL)
	      count++;
	  if (rl2_point_symbolizer_mark_get_col_stroke_linejoin (point, index)
	      != NULL)
	      count++;
	  if (rl2_point_symbolizer_mark_get_col_stroke_linecap (point, index) !=
	      NULL)
	      count++;
	  if (rl2_point_symbolizer_mark_get_col_stroke_dash_array (point, index)
	      != NULL)
	      count++;
	  if (rl2_point_symbolizer_mark_get_col_stroke_dash_offset
	      (point, index) != NULL)
	      count++;
	  if (rl2_point_symbolizer_mark_get_col_fill_color (point, index) !=
	      NULL)
	      count++;
	  if (rl2_point_symbolizer_get_col_graphic_href (point, index) != NULL)
	      count++;
	  if (rl2_point_symbolizer_get_graphic_recode_count
	      (point, index, &max_repl) != RL2_OK)
	      max_repl = 0;
	  for (repl_index = 0; repl_index < max_repl; repl_index++)
	    {
		int color_index;
		if (rl2_point_symbolizer_get_col_graphic_recode_color
		    (point, index, repl_index, &color_index) != NULL)
		    count++;
	    }
      }
    return count;
}

static int
count_line_symbolizer_column_names (rl2LineSymbolizerPtr line)
{
/* counting Line Symbolizer column names) */
    int count = 0;
    int index;
    int max;
    if (rl2_line_symbolizer_get_col_graphic_stroke_href (line) != NULL)
	count++;
    if (rl2_line_symbolizer_get_col_stroke_color (line) != NULL)
	count++;
    if (rl2_line_symbolizer_get_col_stroke_opacity (line) != NULL)
	count++;
    if (rl2_line_symbolizer_get_col_stroke_width (line) != NULL)
	count++;
    if (rl2_line_symbolizer_get_col_stroke_linejoin (line) != NULL)
	count++;
    if (rl2_line_symbolizer_get_col_stroke_linecap (line) != NULL)
	count++;
    if (rl2_line_symbolizer_get_col_stroke_dash_array (line) != NULL)
	count++;
    if (rl2_line_symbolizer_get_col_stroke_dash_offset (line) != NULL)
	count++;
    if (rl2_line_symbolizer_get_col_perpendicular_offset (line) != NULL)
	count++;
    max = 0;
    if (rl2_line_symbolizer_get_graphic_stroke_recode_count
	(line, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  if (rl2_line_symbolizer_get_col_graphic_stroke_recode_color
	      (line, index, &color_index) != NULL)
	      count++;
      }
    return count;
}

static int
count_polygon_symbolizer_column_names (rl2PolygonSymbolizerPtr polyg)
{
/* counting Polygon Symbolizer column names) */
    int count = 0;
    int index;
    int max;
    if (rl2_polygon_symbolizer_get_col_graphic_stroke_href (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_stroke_color (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_stroke_opacity (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_stroke_width (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_stroke_linejoin (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_stroke_linecap (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_stroke_dash_array (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_stroke_dash_offset (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_graphic_fill_href (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_graphic_fill_href (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_fill_color (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_fill_opacity (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_displacement_x (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_displacement_y (polyg) != NULL)
	count++;
    if (rl2_polygon_symbolizer_get_col_perpendicular_offset (polyg) != NULL)
	count++;
    max = 0;
    if (rl2_polygon_symbolizer_get_graphic_stroke_recode_count
	(polyg, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  if (rl2_polygon_symbolizer_get_col_graphic_stroke_recode_color
	      (polyg, index, &color_index) != NULL)
	      count++;
      }
    max = 0;
    if (rl2_polygon_symbolizer_get_graphic_fill_recode_count
	(polyg, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  if (rl2_polygon_symbolizer_get_col_graphic_fill_recode_color
	      (polyg, index, &color_index) != NULL)
	      count++;
      }
    return count;
}

static int
count_text_symbolizer_column_names (rl2TextSymbolizerPtr text)
{
/* counting Text Symbolizer column names) */
    int count = 0;
    if (rl2_text_symbolizer_get_col_label (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_col_font (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_col_style (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_col_weight (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_col_size (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_point_placement_col_anchor_point_x (text) !=
	NULL)
	count++;
    if (rl2_text_symbolizer_get_point_placement_col_anchor_point_y (text) !=
	NULL)
	count++;
    if (rl2_text_symbolizer_get_point_placement_col_displacement_x (text) !=
	NULL)
	count++;
    if (rl2_text_symbolizer_get_point_placement_col_displacement_y (text) !=
	NULL)
	count++;
    if (rl2_text_symbolizer_get_point_placement_col_rotation (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_line_placement_col_perpendicular_offset (text)
	!= NULL)
	count++;
    if (rl2_text_symbolizer_get_line_placement_col_initial_gap (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_line_placement_col_gap (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_halo_col_radius (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_halo_col_fill_color (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_halo_col_fill_opacity (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_col_fill_color (text) != NULL)
	count++;
    if (rl2_text_symbolizer_get_col_fill_opacity (text) != NULL)
	count++;
    return count;
}

static void
do_add_column_name (char **strings, char *dupl, const char *name, int *index)
{
/* adding a Column Name (may be duplicated) */
    int len;
    int i = *index;
    if (name != NULL)
      {
	  len = strlen (name);
	  *(strings + i) = malloc (len + 1);
	  strcpy (*(strings + i), name);
	  *(dupl + i) = 'N';
	  i++;
      }
    *index = i;
}

static void
get_point_symbolizer_strings (char **strings, char *dupl,
			      rl2PointSymbolizerPtr point, int *idx)
{
/* extracting all Point Symbolizer Column Names */
    const char *str;
    int index;
    int max;
    int i = *idx;
    str = rl2_point_symbolizer_get_col_opacity (point);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_point_symbolizer_get_col_size (point);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_point_symbolizer_get_col_rotation (point);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_point_symbolizer_get_col_anchor_point_x (point);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_point_symbolizer_get_col_anchor_point_y (point);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_point_symbolizer_get_col_displacement_x (point);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_point_symbolizer_get_col_displacement_y (point);
    do_add_column_name (strings, dupl, str, &i);
    if (rl2_point_symbolizer_get_count (point, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int repl_index;
	  int max_repl;
	  str =
	      rl2_point_symbolizer_mark_get_col_well_known_type (point, index);
	  do_add_column_name (strings, dupl, str, &i);
	  str = rl2_point_symbolizer_mark_get_col_stroke_color (point, index);
	  do_add_column_name (strings, dupl, str, &i);
	  str = rl2_point_symbolizer_mark_get_col_stroke_width (point, index);
	  do_add_column_name (strings, dupl, str, &i);
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_linejoin (point, index);
	  do_add_column_name (strings, dupl, str, &i);
	  str = rl2_point_symbolizer_mark_get_col_stroke_linecap (point, index);
	  do_add_column_name (strings, dupl, str, &i);
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_dash_array (point,
								   index);
	  do_add_column_name (strings, dupl, str, &i);
	  str =
	      rl2_point_symbolizer_mark_get_col_stroke_dash_offset (point,
								    index);
	  do_add_column_name (strings, dupl, str, &i);
	  str = rl2_point_symbolizer_mark_get_col_fill_color (point, index);
	  do_add_column_name (strings, dupl, str, &i);
	  str = rl2_point_symbolizer_get_col_graphic_href (point, index);
	  do_add_column_name (strings, dupl, str, &i);
	  if (rl2_point_symbolizer_get_graphic_recode_count
	      (point, index, &max_repl) != RL2_OK)
	      max_repl = 0;
	  for (repl_index = 0; repl_index < max_repl; repl_index++)
	    {
		int color_index;
		str =
		    rl2_point_symbolizer_get_col_graphic_recode_color
		    (point, index, repl_index, &color_index);
		do_add_column_name (strings, dupl, str, &i);
	    }
      }
    *idx = i;
}

static void
get_line_symbolizer_strings (char **strings, char *dupl,
			     rl2LineSymbolizerPtr line, int *idx)
{
/* extracting all Line Symbolizer Column Names */
    const char *str;
    int i = *idx;
    int index;
    int max;
    str = rl2_line_symbolizer_get_col_graphic_stroke_href (line);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_line_symbolizer_get_col_stroke_color (line);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_line_symbolizer_get_col_stroke_opacity (line);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_line_symbolizer_get_col_stroke_width (line);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_line_symbolizer_get_col_stroke_linejoin (line);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_line_symbolizer_get_col_stroke_linecap (line);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_line_symbolizer_get_col_stroke_dash_array (line);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_line_symbolizer_get_col_stroke_dash_offset (line);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_line_symbolizer_get_col_perpendicular_offset (line);
    do_add_column_name (strings, dupl, str, &i);
    max = 0;
    if (rl2_line_symbolizer_get_graphic_stroke_recode_count
	(line, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  str = rl2_line_symbolizer_get_col_graphic_stroke_recode_color
	      (line, index, &color_index);
	  do_add_column_name (strings, dupl, str, &i);
      }
    *idx = i;
}

static void
get_polygon_symbolizer_strings (char **strings, char *dupl,
				rl2PolygonSymbolizerPtr polyg, int *idx)
{
/* extracting all Polygon Symbolizer Column Names */
    const char *str;
    int i = *idx;
    int index;
    int max;
    str = rl2_polygon_symbolizer_get_col_graphic_stroke_href (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_stroke_color (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_stroke_opacity (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_stroke_width (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_stroke_linejoin (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_stroke_linecap (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_stroke_dash_array (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_stroke_dash_offset (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_graphic_fill_href (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_graphic_fill_href (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_fill_color (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_fill_opacity (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_displacement_x (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_displacement_y (polyg);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_polygon_symbolizer_get_col_perpendicular_offset (polyg);
    do_add_column_name (strings, dupl, str, &i);
    max = 0;
    if (rl2_polygon_symbolizer_get_graphic_stroke_recode_count
	(polyg, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  str = rl2_polygon_symbolizer_get_col_graphic_stroke_recode_color
	      (polyg, index, &color_index);
	  do_add_column_name (strings, dupl, str, &i);
      }
    max = 0;
    if (rl2_polygon_symbolizer_get_graphic_fill_recode_count
	(polyg, &max) != RL2_OK)
	max = 0;
    for (index = 0; index < max; index++)
      {
	  int color_index;
	  str = rl2_polygon_symbolizer_get_col_graphic_fill_recode_color
	      (polyg, index, &color_index);
	  do_add_column_name (strings, dupl, str, &i);
      }

    *idx = i;
}

static void
get_text_symbolizer_strings (char **strings, char *dupl,
			     rl2TextSymbolizerPtr text, int *idx)
{
/* extracting all Text Symbolizer Column Names */
    const char *str;
    int i = *idx;
    str = rl2_text_symbolizer_get_col_label (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_col_font (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_col_style (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_col_weight (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_col_size (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_point_placement_col_anchor_point_x (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_point_placement_col_anchor_point_y (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_point_placement_col_displacement_x (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_point_placement_col_displacement_y (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_point_placement_col_rotation (text);
    do_add_column_name (strings, dupl, str, &i);
    str =
	rl2_text_symbolizer_get_line_placement_col_perpendicular_offset (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_line_placement_col_initial_gap (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_line_placement_col_gap (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_halo_col_radius (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_halo_col_fill_color (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_halo_col_fill_opacity (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_col_fill_color (text);
    do_add_column_name (strings, dupl, str, &i);
    str = rl2_text_symbolizer_get_col_fill_opacity (text);
    do_add_column_name (strings, dupl, str, &i);
    *idx = i;
}

static void
build_column_names_array (rl2PrivFeatureTypeStylePtr style)
{
/* building the column names array - Feature Type Style */
    char **strings;
    char *dupl;
    int len;
    int count = 0;
    int count2 = 0;
    int i;
    int j;
    rl2PrivStyleRulePtr pR;
    rl2PrivVectorSymbolizerPtr pV;
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PointSymbolizerPtr point;
    rl2LineSymbolizerPtr line;
    rl2PolygonSymbolizerPtr polyg;
    rl2TextSymbolizerPtr text;

    pR = style->first_rule;
    while (pR != NULL)
      {
	  /* counting max column names */
	  if (pR->column_name != NULL)
	      count++;
	  if (pR->style_type == RL2_VECTOR_STYLE && pR->style != NULL)
	    {
		pV = (rl2PrivVectorSymbolizerPtr) (pR->style);
		item = pV->first;
		while (item != NULL)
		  {
		      if (item->symbolizer_type == RL2_POINT_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    point = item->symbolizer;
			    count +=
				count_point_symbolizer_column_names (point);
			}
		      if (item->symbolizer_type == RL2_LINE_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    line = item->symbolizer;
			    count += count_line_symbolizer_column_names (line);
			}
		      if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    polyg = item->symbolizer;
			    count +=
				count_polygon_symbolizer_column_names (polyg);
			}
		      if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    text = item->symbolizer;
			    count += count_text_symbolizer_column_names (text);
			}
		      item = item->next;
		  }
	    }
	  pR = pR->next;
      }
    pR = style->else_rule;
    if (pR != NULL)
      {
	  if (pR->column_name != NULL)
	      count++;
	  if (pR->style_type == RL2_VECTOR_STYLE && pR->style != NULL)
	    {
		pV = (rl2PrivVectorSymbolizerPtr) (pR->style);
		item = pV->first;
		while (item != NULL)
		  {
		      if (item->symbolizer_type == RL2_POINT_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    point = item->symbolizer;
			    count +=
				count_point_symbolizer_column_names (point);
			}
		      if (item->symbolizer_type == RL2_LINE_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    line = item->symbolizer;
			    count += count_line_symbolizer_column_names (line);
			}
		      if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    polyg = item->symbolizer;
			    count +=
				count_polygon_symbolizer_column_names (polyg);
			}
		      if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    text = item->symbolizer;
			    count += count_text_symbolizer_column_names (text);
			}
		      item = item->next;
		  }
	    }
      }
    if (count == 0)
	return;

    strings = malloc (sizeof (char *) * count);
    dupl = malloc (sizeof (char) * count);
    i = 0;
    pR = style->first_rule;
    while (pR != NULL)
      {
	  /* initializing the column names temporary array */
	  if (pR->column_name != NULL)
	    {
		len = strlen (pR->column_name);
		*(strings + i) = malloc (len + 1);
		strcpy (*(strings + i), pR->column_name);
		*(dupl + i) = 'N';
		i++;
	    }
	  if (pR->style_type == RL2_VECTOR_STYLE && pR->style != NULL)
	    {
		pV = (rl2PrivVectorSymbolizerPtr) (pR->style);
		item = pV->first;
		while (item != NULL)
		  {
		      if (item->symbolizer_type == RL2_POINT_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    point = item->symbolizer;
			    get_point_symbolizer_strings (strings, dupl, point,
							  &i);
			}
		      if (item->symbolizer_type == RL2_LINE_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    line = item->symbolizer;
			    get_line_symbolizer_strings (strings, dupl, line,
							 &i);
			}
		      if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    polyg = item->symbolizer;
			    get_polygon_symbolizer_strings (strings, dupl,
							    polyg, &i);
			}
		      if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    text = item->symbolizer;
			    get_text_symbolizer_strings (strings, dupl, text,
							 &i);
			}
		      item = item->next;
		  }
	    }
	  pR = pR->next;
      }
    pR = style->else_rule;
    if (pR != NULL)
      {
	  if (pR->column_name != NULL)
	    {
		len = strlen (pR->column_name);
		*(strings + i) = malloc (len + 1);
		strcpy (*(strings + i), pR->column_name);
		*(dupl + i) = 'N';
		i++;
	    }
	  if (pR->style_type == RL2_VECTOR_STYLE && pR->style != NULL)
	    {
		pV = (rl2PrivVectorSymbolizerPtr) (pR->style);
		item = pV->first;
		while (item != NULL)
		  {
		      if (item->symbolizer_type == RL2_POINT_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    point = item->symbolizer;
			    get_point_symbolizer_strings (strings, dupl, point,
							  &i);
			}
		      if (item->symbolizer_type == RL2_LINE_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    line = item->symbolizer;
			    get_line_symbolizer_strings (strings, dupl, line,
							 &i);
			}
		      if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    polyg = item->symbolizer;
			    get_polygon_symbolizer_strings (strings, dupl,
							    polyg, &i);
			}
		      if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER
			  && item->symbolizer != NULL)
			{
			    text = item->symbolizer;
			    get_text_symbolizer_strings (strings, dupl, text,
							 &i);
			}
		      item = item->next;
		  }
	    }
      }

    for (i = 0; i < count; i++)
      {
	  /* identifying all duplicates */
	  if (*(dupl + i) == 'Y')
	      continue;
	  for (j = i + 1; j < count; j++)
	    {
		if (strcasecmp (*(strings + i), *(strings + j)) == 0)
		    *(dupl + j) = 'Y';
	    }
      }

/* allocating the final array */
    for (i = 0; i < count; i++)
      {
	  if (*(dupl + i) == 'N')
	      count2++;
      }
    style->columns_count = count2;
    style->column_names = malloc (sizeof (char *) * count2);
    j = 0;
    for (i = 0; i < count; i++)
      {
	  /* initializing the final array */
	  if (*(dupl + i) == 'N')
	    {
		len = strlen (*(strings + i));
		*(style->column_names + j) = malloc (len + 1);
		strcpy (*(style->column_names + j), *(strings + i));
		j++;
	    }
      }

/* final cleanup */
    for (i = 0; i < count; i++)
      {
	  if (*(strings + i) != NULL)
	      free (*(strings + i));
      }
    free (strings);
    free (dupl);
}

RL2_DECLARE rl2FeatureTypeStylePtr
rl2_feature_type_style_from_xml (const char *name, const unsigned char *xml)
{
/* attempting to build a Feature Type Style object from an SLD/SE XML style */
    rl2PrivFeatureTypeStylePtr style = NULL;
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    int loop = 1;
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) dummySilentError;

    style = malloc (sizeof (rl2PrivFeatureTypeStyle));
    if (style == NULL)
	return NULL;
    style->name = name;
    style->first_rule = NULL;
    style->last_rule = NULL;
    style->else_rule = NULL;
    style->columns_count = 0;
    style->column_names = NULL;

/* parsing the XML document */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc =
	xmlReadMemory ((const char *) xml, strlen ((const char *) xml),
		       "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  goto error;
      }
    root = xmlDocGetRootElement (xml_doc);
    if (root == NULL)
	goto error;
    if (!find_feature_type_style (root, style, &loop))
	goto error;
    xmlFreeDoc (xml_doc);
    free (xml);
    xml = NULL;

    if (style->name == NULL)
	goto error;
    build_column_names_array (style);

    return (rl2FeatureTypeStylePtr) style;

  error:
    if (xml != NULL)
	free (xml);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    if (style != NULL)
	rl2_destroy_feature_type_style ((rl2FeatureTypeStylePtr) style);
    return NULL;
}

static void
parse_sld_named_style (xmlNodePtr node, char **namedStyle)
{
/* attempting to parse an SLD Named Style */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Name") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  int len =
				      (int)
				      strlen ((const char *) (child->content));
				  *namedStyle = malloc (len + 1);
				  strcpy (*namedStyle,
					  (const char *) (child->content));
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static int
parse_sld_named_layer (xmlNodePtr node, char **namedLayer, char **namedStyle)
{
/* attempting to parse an SLD Named Layer */
    int ok = 0;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Name") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE
				&& child->content != NULL)
			      {
				  int len =
				      (int)
				      strlen ((const char *) (child->content));
				  *namedLayer = malloc (len + 1);
				  strcpy (*namedLayer,
					  (const char *) (child->content));
				  ok = 1;
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "NamedStyle") == 0)
		    parse_sld_named_style (node->children, namedStyle);
	    }
	  node = node->next;
      }
    return ok;
}

static int
parse_sld (xmlNodePtr node, rl2PrivGroupStylePtr style)
{
/* attempting to parse an SLD Style */
    int ok = 0;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "NamedLayer") == 0)
		  {
		      char *namedLayer = NULL;
		      char *namedStyle = NULL;
		      int ret =
			  parse_sld_named_layer (node->children, &namedLayer,
						 &namedStyle);
		      if (ret)
			{
			    rl2PrivChildStylePtr child =
				malloc (sizeof (rl2PrivChildStyle));
			    child->dbPrefix = style->dbPrefix;
			    child->namedLayer = namedLayer;
			    child->namedStyle = namedStyle;
			    child->validLayer = 0;
			    child->validStyle = 0;
			    child->next = NULL;
			    if (style->first == NULL)
				style->first = child;
			    if (style->last != NULL)
				style->last->next = child;
			    style->last = child;
			    ok = 1;
			}
		  }
	    }
	  node = node->next;
      }
    return ok;
}

static int
parse_group_style (xmlNodePtr node, rl2PrivGroupStylePtr style)
{
/* parsing an SLD Style */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "StyledLayerDescriptor") == 0)
		  {
		      int ret = parse_sld (node->children, style);
		      return ret;
		  }
	    }
	  node = node->next;
      }
    return 0;
}

RL2_PRIVATE rl2GroupStylePtr
group_style_from_sld_xml (const char *db_prefix, const char *name,
			  unsigned char *xml)
{
/* attempting to build a Layer Group Style object from an SLD XML style */
    rl2PrivGroupStylePtr style = NULL;
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) dummySilentError;

    style = malloc (sizeof (rl2PrivGroupStyle));
    if (style == NULL)
	return NULL;
    style->dbPrefix = db_prefix;
    style->name = name;
    style->first = NULL;
    style->last = NULL;
    style->valid = 0;

/* parsing the XML document */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc =
	xmlReadMemory ((const char *) xml, strlen ((const char *) xml),
		       "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  goto error;
      }
    root = xmlDocGetRootElement (xml_doc);
    if (root == NULL)
	goto error;
    if (!parse_group_style (root, style))
	goto error;
    xmlFreeDoc (xml_doc);
    free (xml);
    xml = NULL;

    if (style->name == NULL)
	goto error;

    return (rl2GroupStylePtr) style;

  error:
    if (xml != NULL)
	free (xml);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    if (style != NULL)
	rl2_destroy_group_style ((rl2GroupStylePtr) style);
    return NULL;
}

static rl2PrivGroupRendererPtr
rl2_alloc_group_renderer (int count)
{
/* creating a GroupRenderer object */
    int i;
    rl2PrivGroupRendererPtr ptr = NULL;
    if (count <= 0)
	return NULL;
    ptr = malloc (sizeof (rl2PrivGroupRenderer));
    if (ptr == NULL)
	return NULL;
    ptr->count = count;
    ptr->layers = malloc (sizeof (rl2PrivGroupRendererLayer) * count);
    if (ptr->layers == NULL)
      {
	  free (ptr);
	  return NULL;
      }
    for (i = 0; i < count; i++)
      {
	  rl2PrivGroupRendererLayerPtr lyr = ptr->layers + i;
	  lyr->layer_type = 0;
	  lyr->layer_name = NULL;
	  lyr->coverage = NULL;
	  lyr->raster_style_id = -1;
	  lyr->raster_symbolizer = NULL;
	  lyr->raster_stats = NULL;
      }
    return ptr;
}

static int
rl2_group_renderer_set_raster (rl2PrivGroupRendererPtr group, int index,
			       const char *layer_name,
			       rl2CoveragePtr coverage,
			       sqlite3_int64 style_id,
			       rl2RasterSymbolizerPtr symbolizer,
			       rl2RasterStatisticsPtr stats)
{
/* setting up one of the Layers within the Group */
    int len;
    rl2PrivGroupRendererLayerPtr lyr;
    rl2PrivGroupRendererPtr ptr = (rl2PrivGroupRendererPtr) group;
    if (ptr == NULL)
	return RL2_ERROR;

    if (index >= 0 && index < ptr->count)
	;
    else
	return RL2_ERROR;

    lyr = ptr->layers + index;
    lyr->layer_type = RL2_GROUP_RENDERER_RASTER_LAYER;
    if (lyr->layer_name != NULL)
	free (lyr->layer_name);
    if (layer_name == NULL)
	lyr->layer_name = NULL;
    else
      {
	  len = strlen (layer_name);
	  lyr->layer_name = malloc (len + 1);
	  strcpy (lyr->layer_name, layer_name);
      }
    if (lyr->coverage != NULL)
	rl2_destroy_coverage (lyr->coverage);
    lyr->coverage = (rl2CoveragePtr) coverage;
    lyr->raster_style_id = style_id;
    if (lyr->raster_symbolizer != NULL)
	rl2_destroy_raster_symbolizer (lyr->raster_symbolizer);
    lyr->raster_symbolizer = (rl2PrivRasterSymbolizerPtr) symbolizer;
    if (lyr->raster_stats != NULL)
	rl2_destroy_raster_statistics ((rl2RasterStatisticsPtr)
				       (lyr->raster_stats));
    lyr->raster_stats = (rl2PrivRasterStatisticsPtr) stats;
    return RL2_OK;

}

static int
rl2_is_valid_group_renderer (rl2PrivGroupRendererPtr ptr, int *valid)
{
/* testing a GroupRenderer for validity */
    int i;
    int error = 0;
    if (ptr == NULL)
	return RL2_ERROR;

    for (i = 0; i < ptr->count; i++)
      {
	  rl2PrivGroupRendererLayerPtr lyr = ptr->layers + i;
	  rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) lyr->coverage;
	  if (lyr->layer_type != RL2_GROUP_RENDERER_RASTER_LAYER)
	      error = 1;
	  if (lyr->layer_name == NULL)
	      error = 1;
	  if (lyr->coverage == NULL)
	      error = 1;
	  else
	    {
		if ((cvg->pixelType == RL2_PIXEL_DATAGRID
		     || cvg->pixelType == RL2_PIXEL_MULTIBAND)
		    && lyr->raster_symbolizer == NULL)
		    error = 1;
	    }
	  if (lyr->raster_style_id <= 0)
	      error = 1;
	  if (lyr->raster_stats == NULL)
	      error = 1;
      }
    if (error)
	*valid = 0;
    else
	*valid = 1;
    return RL2_OK;
}

RL2_DECLARE rl2GroupRendererPtr
rl2_create_group_renderer (sqlite3 * sqlite, rl2GroupStylePtr group_style)
{
/* creating a GroupRenderer object */
    rl2PrivGroupRendererPtr group = NULL;
    int valid;
    int count;
    int i;

    if (rl2_is_valid_group_style (group_style, &valid) != RL2_OK)
	goto error;
    if (!valid)
	goto error;
    if (rl2_get_group_style_count (group_style, &count) != RL2_OK)
	goto error;
    group = rl2_alloc_group_renderer (count);
    if (group == NULL)
	goto error;
    for (i = 0; i < count; i++)
      {
	  /* testing individual layers/styles */
	  rl2CoverageStylePtr cvg_stl = NULL;
	  rl2RasterStatisticsPtr stats = NULL;
	  const char *db_prefix = rl2_get_group_prefix (group_style, i);
	  const char *layer_name = rl2_get_group_named_layer (group_style, i);
	  const char *layer_style_name =
	      rl2_get_group_named_style (group_style, i);
	  sqlite3_int64 layer_style_id = -1;
	  rl2CoveragePtr coverage =
	      rl2_create_coverage_from_dbms (sqlite, db_prefix, layer_name);
	  rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
	  if (rl2_is_valid_group_named_layer (group_style, 0, &valid) == RL2_OK)
	    {
		if (valid)
		  {
		      /* validating the style */
		      if (layer_style_id > 0)
			{
			    /* attempting to get a Coverage Style */
			    /*
			       cvg_stl =
			       rl2_create_coverage_style_from_dbms (sqlite,
			       layer_name,
			       layer_style_id);
			     */
			}
		      stats =
			  rl2_create_raster_statistics_from_dbms (sqlite,
								  db_prefix,
								  layer_name);
		  }
		if ((cvg->pixelType == RL2_PIXEL_DATAGRID
		     || cvg->pixelType == RL2_PIXEL_MULTIBAND)
		    && cvg_stl == NULL)
		  {
		      /* creating a default RasterStyle */
		      /*
		         rl2PrivRasterSymbolizerPtr symb =
		         malloc (sizeof (rl2PrivRasterSymbolizer));
		         symbolizer = (rl2RasterSymbolizerPtr) symb;
		         symb->name = malloc (8);
		         strcpy (symb->name, "default");
		         symb->title = NULL;
		         symb->abstract = NULL;
		         symb->opacity = 1.0;
		         symb->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_NONE;
		         symb->bandSelection =
		         malloc (sizeof (rl2PrivBandSelection));
		         symb->bandSelection->selectionType =
		         RL2_BAND_SELECTION_MONO;
		         symb->bandSelection->grayBand = 0;
		         symb->bandSelection->grayContrast =
		         RL2_CONTRAST_ENHANCEMENT_NONE;
		         symb->categorize = NULL;
		         symb->interpolate = NULL;
		         symb->shadedRelief = 0;
		       */
		  }
	    }
	  rl2_group_renderer_set_raster (group, i, layer_name, coverage,
					 layer_style_id, NULL, stats);
      }
    if (rl2_is_valid_group_renderer (group, &valid) != RL2_OK)
	goto error;
    if (!valid)
	goto error;
    return (rl2GroupRendererPtr) group;

  error:
    if (group != NULL)
	rl2_destroy_group_renderer ((rl2GroupRendererPtr) group);
    return NULL;
}
