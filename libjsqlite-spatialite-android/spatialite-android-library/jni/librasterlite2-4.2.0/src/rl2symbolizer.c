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

The Original Code is the SpatiaLite library

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

#include <spatialite/gaiaaux.h>

#define RL2_UNUSED() if (argc || argv) argc = argc;

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
parse_sld_se_opacity (xmlNodePtr node, rl2PrivRasterStylePtr style)
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
parse_sld_se_channels (xmlNodePtr node, rl2PrivRasterStylePtr style)
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
parse_sld_se_color (const char *color, unsigned char *red, unsigned char *green,
		    unsigned char *blue)
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
parse_sld_se_channel_selection (xmlNodePtr node, rl2PrivRasterStylePtr style)
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
parse_sld_se_categorize (xmlNodePtr node, rl2PrivRasterStylePtr style)
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
						    style->
							categorize->baseGreen =
							green;
						    style->
							categorize->baseBlue =
							blue;
						}
					      else
						{
						    style->categorize->
							last->red = red;
						    style->categorize->
							last->green = green;
						    style->categorize->
							last->blue = blue;
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
parse_sld_se_interpolation_point (xmlNodePtr node, rl2PrivRasterStylePtr style)
{
/* parsing RasterSymbolizer ColorMap InterpolationPoint */
    double value;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
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
parse_sld_se_interpolate (xmlNodePtr node, rl2PrivRasterStylePtr style)
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
parse_sld_se_color_map (xmlNodePtr node, rl2PrivRasterStylePtr style)
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
parse_sld_se_shaded_relief (xmlNodePtr node, rl2PrivRasterStylePtr style)
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
							style->brightnessOnly =
							    atoi (value);
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
parse_raster_symbolizer (xmlNodePtr node, rl2PrivRasterStylePtr style)
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

static int
find_raster_symbolizer (xmlNodePtr node, rl2PrivRasterStylePtr style, int *loop)
{
/* recursively searching an SLD/SE RasterSymbolizer */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "RasterSymbolizer") == 0)
		  {
		      int ret = parse_raster_symbolizer (node, style);
		      *loop = 0;
		      return ret;
		  }
		if (strcmp (name, "CoverageStyle") == 0 ||
		    strcmp (name, "Rule") == 0)
		  {
		      int ret =
			  find_raster_symbolizer (node->children, style, loop);
		      if (*loop == 0)
			  return ret;
		  }
	    }
	  node = node->next;
      }
    return 0;
}

RL2_PRIVATE rl2RasterStylePtr
raster_style_from_sld_se_xml (char *name, char *title, char *abstract,
			      unsigned char *xml)
{
/* attempting to build a RasterStyle object from an SLD/SE XML style */
    rl2PrivRasterStylePtr style = NULL;
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    int loop = 1;
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) dummySilentError;

    style = malloc (sizeof (rl2PrivRasterStyle));
    if (style == NULL)
	return NULL;
    style->name = name;
    style->title = title;
    style->abstract = abstract;
    style->opacity = 1.0;
    style->bandSelection = NULL;
    style->categorize = NULL;
    style->interpolate = NULL;
    style->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_NONE;
    style->gammaValue = 1.0;
    style->shadedRelief = 0;
    style->brightnessOnly = 0;
    style->reliefFactor = 55.0;

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
    if (!find_raster_symbolizer (root, style, &loop))
	goto error;
    xmlFreeDoc (xml_doc);
    free (xml);
    xml = NULL;

    if (style->name == NULL)
	goto error;

    return (rl2RasterStylePtr) style;

  error:
    if (xml != NULL)
	free (xml);
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    if (style != NULL)
	rl2_destroy_raster_style ((rl2RasterStylePtr) style);
    return NULL;
}

RL2_DECLARE void
rl2_destroy_raster_style (rl2RasterStylePtr style)
{
/* destroying a RasterStyle object */
    rl2PrivColorMapPointPtr pC;
    rl2PrivColorMapPointPtr pCn;
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return;

    if (stl->name != NULL)
	free (stl->name);
    if (stl->title != NULL)
	free (stl->title);
    if (stl->abstract != NULL)
	free (stl->abstract);
    if (stl->bandSelection != NULL)
	free (stl->bandSelection);
    if (stl->categorize != NULL)
      {
	  pC = stl->categorize->first;
	  while (pC != NULL)
	    {
		pCn = pC->next;
		free (pC);
		pC = pCn;
	    }
	  free (stl->categorize);
      }
    if (stl->interpolate != NULL)
      {
	  pC = stl->interpolate->first;
	  while (pC != NULL)
	    {
		pCn = pC->next;
		free (pC);
		pC = pCn;
	    }
	  free (stl->interpolate);
      }
    free (stl);
}

RL2_DECLARE const char *
rl2_get_raster_style_name (rl2RasterStylePtr style)
{
/* return the RasterStyle Name */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->name;
}

RL2_DECLARE const char *
rl2_get_raster_style_title (rl2RasterStylePtr style)
{
/* return the RasterStyle Title */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->title;
}

RL2_DECLARE const char *
rl2_get_raster_style_abstract (rl2RasterStylePtr style)
{
/* return the RasterStyle Abstract */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->abstract;
}

RL2_DECLARE int
rl2_get_raster_style_opacity (rl2RasterStylePtr style, double *opacity)
{
/* return the RasterStyle Opacity */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    *opacity = stl->opacity;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_raster_style_mono_band_selected (rl2RasterStylePtr style, int *selected)
{
/* return if the RasterStyle has a MonoBand selection */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->shadedRelief)
      {
	  /* Shaded Relief */
	  *selected = 1;
	  return RL2_OK;
      }
    if (stl->bandSelection == NULL)
      {
	  if (stl->categorize != NULL)
	    {
		/* Categorize Color Map */
		*selected = 1;
		return RL2_OK;
	    }
	  if (stl->interpolate != NULL)
	    {
		/* Interpolate Color Map */
		*selected = 1;
		return RL2_OK;
	    }
	  if (stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_NORMALIZE ||
	      stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_HISTOGRAM ||
	      stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_GAMMA)
	    {
		/* Contrast Enhancement */
		*selected = 1;
		return RL2_OK;
	    }
      }
    if (stl->bandSelection == NULL)
	*selected = 0;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_MONO)
	*selected = 1;
    else
	*selected = 0;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_style_mono_band_selection (rl2RasterStylePtr style,
					  unsigned char *gray_band)
{
/* return the RasterStyle MonoBand selection */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
      {
	  if (stl->categorize != NULL)
	    {
		/* Categorize Color Map */
		*gray_band = 0;
		return RL2_OK;
	    }
	  if (stl->interpolate != NULL)
	    {
		/* Interpolate Color Map */
		*gray_band = 0;
		return RL2_OK;
	    }
	  /* Interpolate Color Map */
	  *gray_band = 0;
	  return RL2_OK;
      }
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_MONO)
      {
	  *gray_band = stl->bandSelection->grayBand;
	  return RL2_OK;
      }
    else
	return RL2_ERROR;
}

RL2_DECLARE int
rl2_is_raster_style_triple_band_selected (rl2RasterStylePtr style,
					  int *selected)
{
/* return if the RasterStyle has a TripleBand selection */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
      {
	  if (stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_NORMALIZE ||
	      stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_HISTOGRAM ||
	      stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_GAMMA)
	    {
		/* Contrast Enhancement */
		*selected = 1;
		return RL2_OK;
	    }
      }
    if (stl->bandSelection == NULL)
	*selected = 0;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
	*selected = 1;
    else
	*selected = 0;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_style_triple_band_selection (rl2RasterStylePtr style,
					    unsigned char *red_band,
					    unsigned char *green_band,
					    unsigned char *blue_band)
{
/* return the RasterStyle TripleBand selection */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
      {
	  if (stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_NORMALIZE ||
	      stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_HISTOGRAM ||
	      stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_GAMMA)
	    {
		/* Contrast Enhancement */
		*red_band = 0;
		*green_band = 1;
		*blue_band = 2;
		return RL2_OK;
	    }
      }
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
      {
	  *red_band = stl->bandSelection->redBand;
	  *green_band = stl->bandSelection->greenBand;
	  *blue_band = stl->bandSelection->blueBand;
	  return RL2_OK;
      }
    else
	return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_style_overall_contrast_enhancement (rl2RasterStylePtr style,
						   unsigned char
						   *contrast_enhancement,
						   double *gamma_value)
{
/* return the RasterStyle OverallContrastEnhancement */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    *contrast_enhancement = stl->contrastEnhancement;
    *gamma_value = stl->gammaValue;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_style_red_band_contrast_enhancement (rl2RasterStylePtr style,
						    unsigned char
						    *contrast_enhancement,
						    double *gamma_value)
{
/* return the RasterStyle RedBand ContrastEnhancement */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
      {
	  *contrast_enhancement = stl->bandSelection->redContrast;
	  *gamma_value = stl->bandSelection->redGamma;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_style_green_band_contrast_enhancement (rl2RasterStylePtr style,
						      unsigned char
						      *contrast_enhancement,
						      double *gamma_value)
{
/* return the RasterStyle GreenBand ContrastEnhancement */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
      {
	  *contrast_enhancement = stl->bandSelection->greenContrast;
	  *gamma_value = stl->bandSelection->greenGamma;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_style_blue_band_contrast_enhancement (rl2RasterStylePtr style,
						     unsigned char
						     *contrast_enhancement,
						     double *gamma_value)
{
/* return the RasterStyle BlueBand ContrastEnhancement */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
      {
	  *contrast_enhancement = stl->bandSelection->blueContrast;
	  *gamma_value = stl->bandSelection->blueGamma;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_style_gray_band_contrast_enhancement (rl2RasterStylePtr style,
						     unsigned char
						     *contrast_enhancement,
						     double *gamma_value)
{
/* return the RasterStyle GrayBand ContrastEnhancement */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_MONO)
      {
	  *contrast_enhancement = stl->bandSelection->grayContrast;
	  *gamma_value = stl->bandSelection->grayGamma;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_has_raster_style_shaded_relief (rl2RasterStylePtr style, int *shaded_relief)
{
/* return if the RasterStyle has ShadedRelief */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    *shaded_relief = stl->shadedRelief;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_style_shaded_relief (rl2RasterStylePtr style,
				    int *brightness_only, double *relief_factor)
{
/* return the RasterStyle ShadedRelief parameters */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->shadedRelief)
      {
	  *brightness_only = stl->brightnessOnly;
	  *relief_factor = stl->reliefFactor;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_has_raster_style_color_map_interpolated (rl2RasterStylePtr style,
					     int *interpolated)
{
/* return if the RasterStyle has an Interpolated ColorMap */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->interpolate != NULL)
	*interpolated = 1;
    else
	*interpolated = 0;
    return RL2_OK;
}

RL2_DECLARE int
rl2_has_raster_style_color_map_categorized (rl2RasterStylePtr style,
					    int *categorized)
{
/* return if the RasterStyle has a Categorized ColorMap */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->categorize != NULL)
	*categorized = 1;
    else
	*categorized = 0;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_style_color_map_default (rl2RasterStylePtr style,
					unsigned char *red,
					unsigned char *green,
					unsigned char *blue)
{
/* return the RasterStyle ColorMap Default color */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->interpolate != NULL)
      {
	  *red = stl->interpolate->dfltRed;
	  *green = stl->interpolate->dfltGreen;
	  *blue = stl->interpolate->dfltBlue;
	  return RL2_OK;
      }
    if (stl->categorize != NULL)
      {
	  *red = stl->categorize->dfltRed;
	  *green = stl->categorize->dfltGreen;
	  *blue = stl->categorize->dfltBlue;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_style_color_map_category_base (rl2RasterStylePtr style,
					      unsigned char *red,
					      unsigned char *green,
					      unsigned char *blue)
{
/* return the RasterStyle ColorMap Category base-color */
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->categorize != NULL)
      {
	  *red = stl->categorize->baseRed;
	  *green = stl->categorize->baseGreen;
	  *blue = stl->categorize->baseBlue;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_style_color_map_count (rl2RasterStylePtr style, int *count)
{
/* return the RasterStyle ColorMap items count */
    int cnt;
    rl2PrivColorMapPointPtr pt;
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->categorize != NULL)
      {
	  cnt = 0;
	  pt = stl->categorize->first;
	  while (pt != NULL)
	    {
		cnt++;
		pt = pt->next;
	    }
	  *count = cnt;
	  return RL2_OK;
      }
    if (stl->interpolate != NULL)
      {
	  cnt = 0;
	  pt = stl->interpolate->first;
	  while (pt != NULL)
	    {
		cnt++;
		pt = pt->next;
	    }
	  *count = cnt;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_style_color_map_entry (rl2RasterStylePtr style, int index,
				      double *value, unsigned char *red,
				      unsigned char *green, unsigned char *blue)
{
/* return the RasterStyle ColorMap item values */
    int cnt;
    rl2PrivColorMapPointPtr pt;
    rl2PrivRasterStylePtr stl = (rl2PrivRasterStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->categorize != NULL)
      {
	  cnt = 0;
	  pt = stl->categorize->first;
	  while (pt != NULL)
	    {
		if (index == cnt)
		  {
		      *value = pt->value;
		      *red = pt->red;
		      *green = pt->green;
		      *blue = pt->blue;
		      return RL2_OK;
		  }
		cnt++;
		pt = pt->next;
	    }
      }
    if (stl->interpolate != NULL)
      {
	  cnt = 0;
	  pt = stl->interpolate->first;
	  while (pt != NULL)
	    {
		if (index == cnt)
		  {
		      *value = pt->value;
		      *red = pt->red;
		      *green = pt->green;
		      *blue = pt->blue;
		      return RL2_OK;
		  }
		cnt++;
		pt = pt->next;
	    }
      }
    return RL2_ERROR;
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
group_style_from_sld_xml (char *name, char *title, char *abstract,
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
    style->name = name;
    style->title = title;
    style->abstract = abstract;
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

RL2_DECLARE void
rl2_destroy_group_style (rl2GroupStylePtr style)
{
/* destroying a Group Style object */
    rl2PrivChildStylePtr child;
    rl2PrivChildStylePtr child_n;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return;

    if (stl->name != NULL)
	free (stl->name);
    if (stl->title != NULL)
	free (stl->title);
    if (stl->abstract != NULL)
	free (stl->abstract);
    child = stl->first;
    while (child != NULL)
      {
	  child_n = child->next;
	  if (child->namedLayer != NULL)
	      free (child->namedLayer);
	  if (child->namedStyle != NULL)
	      free (child->namedStyle);
	  free (child);
	  child = child_n;
      }
    free (stl);
}

RL2_DECLARE const char *
rl2_get_group_style_name (rl2GroupStylePtr style)
{
/* return the Group Style Name */
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->name;
}

RL2_DECLARE const char *
rl2_get_group_style_title (rl2GroupStylePtr style)
{
/* return the Group Style Title */
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->title;
}

RL2_DECLARE const char *
rl2_get_group_style_abstract (rl2GroupStylePtr style)
{
/* return the Group Style Abstract */
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->abstract;
}

RL2_DECLARE int
rl2_is_valid_group_style (rl2GroupStylePtr style, int *valid)
{
/* testing a Group Style for validity */
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    *valid = stl->valid;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_group_style_count (rl2GroupStylePtr style, int *count)
{
/* return the total count of Group Style Items */
    int cnt = 0;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    *count = cnt;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_get_group_named_layer (rl2GroupStylePtr style, int index)
{
/* return the Nth NamedLayer from a Group Style */
    int cnt = 0;
    const char *str;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    if (index < 0)
	return NULL;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return NULL;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		str = child->namedLayer;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return str;
}

RL2_DECLARE const char *
rl2_get_group_named_style (rl2GroupStylePtr style, int index)
{
/* return the Nth NamedStyle from a Group Style */
    int cnt = 0;
    const char *str;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    if (index < 0)
	return NULL;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return NULL;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		str = child->namedStyle;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return str;
}

RL2_DECLARE int
rl2_is_valid_group_named_layer (rl2GroupStylePtr style, int index, int *valid)
{
/* testing for validity the Nth NamedLayer from a Group Style */
    int cnt = 0;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (index < 0)
	return RL2_ERROR;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return RL2_ERROR;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		*valid = child->validLayer;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_valid_group_named_style (rl2GroupStylePtr style, int index, int *valid)
{
/* testing for validity the Nth NamedStyle from a Group Style */
    int cnt = 0;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (index < 0)
	return RL2_ERROR;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return RL2_ERROR;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		*valid = child->validStyle;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return RL2_OK;
}
