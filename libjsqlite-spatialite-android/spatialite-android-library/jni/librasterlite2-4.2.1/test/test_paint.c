/*

 test_paint.c -- RasterLite2 Test Case

 Author: Alessandro Furieri <a.furieri@lqt.it>

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
 
Portions created by the Initial Developer are Copyright (C) 2013
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2graphics.h"

static int
do_paint_test (rl2GraphicsContextPtr ctx)
{
    unsigned char *buffer;
    int buf_size;
    unsigned int width;
    unsigned int height;
    rl2GraphicsBitmapPtr bmp;
    rl2GraphicsPatternPtr pattern;
    rl2RasterPtr rst;
    rl2SectionPtr img;
    unsigned char *rgba;
    unsigned char *p_rgba;
    int row;
    int col;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    rl2GraphicsFontPtr font;
    double pre_x;
    double pre_y;
    double w;
    double h;
    double post_x;
    double post_y;

/* loading a sample Image (RGB) */
    img = rl2_section_from_jpeg ("./jpeg1.jpg");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./jpeg1.jpg");
	  return -10;
      }
    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "\"%s\" invalid raster pointer\n", "./jpeg1.jpg");
	  return -11;
      }
    rl2_get_raster_size (rst, &width, &height);
    if (width != 558)
      {
	  fprintf (stderr, "\"%s\" unexpected raster width %d\n", "./jpeg1.jpg",
		   width);
	  return -12;
      }
    if (height != 543)
      {
	  fprintf (stderr, "\"%s\" unexpected raster width %d\n", "./jpeg1.jpg",
		   height);
	  return -13;
      }

/* extracting RGBA data */
    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: \"%s\"\n", "./jpeg1.jpg");
	  return -14;
      }
    rl2_destroy_section (img);

/* creating a Graphics Bitmap */
    bmp = rl2_graph_create_bitmap (buffer, width, height);
    if (bmp == NULL)
      {
	  fprintf (stderr, "Unable to create a Graphics Bitmap\n");
	  return -15;
      }

/* rendering the Bitmap */
    if (!rl2_graph_draw_bitmap (ctx, bmp, 256, 256))
      {
	  fprintf (stderr,
		   "Unable to render the Bitmap #1 into the Graphics backend\n");
	  return -16;
      }
    rl2_graph_destroy_bitmap (bmp);

/* loading a sample Image (Grayscale) */
    img = rl2_section_from_jpeg ("./jpeg2.jpg");
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to read: %s\n", "./jpeg2.jpg");
	  return -17;
      }
    rst = rl2_get_section_raster (img);
    if (rst == NULL)
      {
	  fprintf (stderr, "\"%s\" invalid raster pointer\n", "./jpeg2.jpg");
	  return -18;
      }
    rl2_get_raster_size (rst, &width, &height);
    if (width != 558)
      {
	  fprintf (stderr, "\"%s\" unexpected raster width %d\n", "./jpeg2.jpg",
		   width);
	  return -19;
      }
    if (height != 543)
      {
	  fprintf (stderr, "\"%s\" unexpected raster width %d\n", "./jpeg2.jpg",
		   height);
	  return -20;
      }

/* extracting RGBA data */
    if (rl2_raster_data_to_RGBA (rst, &buffer, &buf_size) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RGBA data: \"%s\"\n", "./jpeg2.jpg");
	  return -21;
      }
    rl2_destroy_section (img);

/* creating a Graphics Bitmap */
    bmp = rl2_graph_create_bitmap (buffer, width, height);
    if (bmp == NULL)
      {
	  fprintf (stderr, "Unable to create a Graphics Bitmap\n");
	  return -22;
      }

/* rendering the Bitmap */
    if (!rl2_graph_draw_bitmap (ctx, bmp, 700, 600))
      {
	  fprintf (stderr,
		   "Unable to render the Bitmap #2 into the Graphics backend\n");
	  return -23;
      }
    rl2_graph_destroy_bitmap (bmp);

/* setting up a RED dotted pen */
    if (!rl2_graph_set_pen (ctx, 255, 0, 0, 255, 8.0, RL2_PENSTYLE_DOT))
      {
	  fprintf (stderr, "Unable to set a Red Dotted Pen\n");
	  return -24;
      }

/* setting up a Green solid semi-transparent Brush */
    if (!rl2_graph_set_brush (ctx, 0, 255, 0, 128))
      {
	  fprintf (stderr, "Unable to set a Green semi-transparent Brush\n");
	  return -25;
      }

/* drawing a rectangle */
    if (!rl2_graph_draw_rectangle (ctx, 650, 500, 200, 200))
      {
	  fprintf (stderr, "Unable to draw a rectangle\n");
	  return -26;
      }

/* drawing a rounded rectangle */
    if (!rl2_graph_draw_rounded_rectangle (ctx, 650, 950, 300, 100, 30))
      {
	  fprintf (stderr, "Unable to draw a rounded rectangle\n");
	  return -27;
      }

/* setting up a Black solid pen */
    if (!rl2_graph_set_pen (ctx, 0, 0, 0, 255, 2.0, RL2_PENSTYLE_SOLID))
      {
	  fprintf (stderr, "Unable to set a Black solid Pen\n");
	  return -28;
      }

/* setting up a Yellow/Blue linear gradient Brush */
    if (!rl2_graph_set_linear_gradient_brush
	(ctx, 1024, 1500, 200, 100, 255, 255, 0, 255, 0, 0, 255, 255))
      {
	  fprintf (stderr,
		   "Unable to set a Yellow/Blue linear gradient Brush\n");
	  return -29;
      }

/* drawing an Ellipse */
    if (!rl2_graph_draw_ellipse (ctx, 1024, 1500, 200, 100))
      {
	  fprintf (stderr, "Unable to draw an ellipse\n");
	  return -30;
      }

/* setting up a Yellow/Red linear gradient Brush */
    if (!rl2_graph_set_linear_gradient_brush
	(ctx, 1500, 1500, 200, 200, 255, 255, 0, 255, 255, 0, 0, 255))
      {
	  fprintf (stderr,
		   "Unable to set a Yellow/Red linear gradient Brush\n");
	  return -31;
      }

/* drawing a Circular Sector */
    if (!rl2_graph_draw_circle_sector (ctx, 1500, 1500, 100, 270, 180))
      {
	  fprintf (stderr, "Unable to draw a circular sector\n");
	  return -32;
      }

/* setting up a Red solid pen */
    if (!rl2_graph_set_pen (ctx, 255, 0, 0, 255, 16.0, RL2_PENSTYLE_SOLID))
      {
	  fprintf (stderr, "Unable to set a Red solid Pen\n");
	  return -33;
      }

/* creating an RGBA buffer */
    rgba = malloc (64 * 64 * 4);
    p_rgba = rgba;
    for (row = 0; row < 64; row++)
      {
	  for (col = 0; col < 64; col++)
	    {
		if (row >= 32)
		  {
		      if (col >= 32)
			{
			    red = 0;
			    green = 255;
			    blue = 0;
			    alpha = 128;
			}
		      else
			{
			    red = 0;
			    green = 255;
			    blue = 255;
			    alpha = 255;
			}
		  }
		else
		  {
		      if (col >= 32)
			{
			    red = 0;
			    green = 0;
			    blue = 0;
			    alpha = 0;
			}
		      else
			{
			    red = 255;
			    green = 255;
			    blue = 0;
			    alpha = 255;
			}
		  }
		*p_rgba++ = red;
		*p_rgba++ = green;
		*p_rgba++ = blue;
		*p_rgba++ = alpha;
	    }
      }

/* creating and setting up a pattern brush */
    pattern = rl2_graph_create_pattern (rgba, 64, 64);
    if (pattern == NULL)
      {
	  fprintf (stderr, "Unable to create a Pattern Brush\n");
	  return -34;
      }
    if (!rl2_graph_set_pattern_brush (ctx, pattern))
      {
	  fprintf (stderr, "Unable to set up a pattern brush\n");
	  return -35;
      }
    if (!rl2_graph_stroke_line (ctx, 300, 300, 600, 600))
      {
	  fprintf (stderr, "Unable to stroke a line\n");
	  return -36;
      }

/* arbitrary path */
    if (!rl2_graph_move_to_point (ctx, 50, 1400))
      {
	  fprintf (stderr, "Unable to move to point\n");
	  return -37;
      }
    if (!rl2_graph_add_line_to_path (ctx, 250, 1250))
      {
	  fprintf (stderr, "Unable to move to point #1\n");
	  return -38;
      }
    if (!rl2_graph_add_line_to_path (ctx, 500, 1400))
      {
	  fprintf (stderr, "Unable to move to point #2\n");
	  return -39;
      }
    if (!rl2_graph_add_line_to_path (ctx, 750, 1250))
      {
	  fprintf (stderr, "Unable to move to point #3\n");
	  return -40;
      }
    if (!rl2_graph_add_line_to_path (ctx, 1000, 1400))
      {
	  fprintf (stderr, "Unable to move to point #4\n");
	  return -41;
      }
    if (!rl2_graph_add_line_to_path (ctx, 1000, 1500))
      {
	  fprintf (stderr, "Unable to move to point #4\n");
	  return -42;
      }
    if (!rl2_graph_add_line_to_path (ctx, 50, 1500))
      {
	  fprintf (stderr, "Unable to move to point #5\n");
	  return -43;
      }
    if (!rl2_graph_close_subpath (ctx))
      {
	  fprintf (stderr, "Unable to close a sub-path\n");
	  return -44;
      }
    if (!rl2_graph_fill_path (ctx, 1))
      {
	  fprintf (stderr, "Unable to fill a path\n");
	  return -45;
      }
    if (!rl2_graph_stroke_path (ctx, 0))
      {
	  fprintf (stderr, "Unable to stroke a path\n");
	  return -46;
      }

    rl2_graph_destroy_pattern (pattern);

/* creating and setting up a Green bold italic font */
    font =
	rl2_graph_create_font (32, RL2_FONTSTYLE_ITALIC, RL2_FONTWEIGHT_BOLD);
    if (pattern == NULL)
      {
	  fprintf (stderr, "Unable to create a Font\n");
	  return -47;
      }
    if (!rl2_graph_font_set_color (font, 0, 255, 0, 255))
      {
	  fprintf (stderr, "Unable to set the font color\n");
	  return -48;
      }
    if (!rl2_graph_set_font (ctx, font))
      {
	  fprintf (stderr, "Unable to set up a font\n");
	  return -49;
      }
    if (!rl2_graph_draw_text (ctx, "Armageddon", 1000, 100, 120))
      {
	  fprintf (stderr, "Unable to print text #1\n");
	  return -50;
      }
    rl2_graph_destroy_font (font);

/* creating and setting up a Black outlined font */
    font =
	rl2_graph_create_font (32, RL2_FONTSTYLE_NORMAL, RL2_FONTWEIGHT_BOLD);
    if (pattern == NULL)
      {
	  fprintf (stderr, "Unable to create a Font #2\n");
	  return -51;
      }
    if (!rl2_graph_font_set_color (font, 0, 0, 0, 255))
      {
	  fprintf (stderr, "Unable to set the font color #2\n");
	  return -52;
      }
    if (!rl2_graph_font_set_outline (font, 1.5))
      {
	  fprintf (stderr, "Unable to set the font outline\n");
	  return -53;
      }
    if (!rl2_graph_set_font (ctx, font))
      {
	  fprintf (stderr, "Unable to set up a font #2\n");
	  return -54;
      }
    if (!rl2_graph_draw_text (ctx, "Walhalla", 300, 400, 0))
      {
	  fprintf (stderr, "Unable to print text #2\n");
	  return -55;
      }
    if (!rl2_graph_get_text_extent
	(ctx, "Walhalla", &pre_x, &pre_y, &w, &h, &post_x, &post_y))
      {
	  fprintf (stderr, "Unable to measure text\n");
	  return -56;
      }

    rl2_graph_destroy_font (font);


    return 0;
}

int
main (int argc, char *argv[])
{
    rl2GraphicsContextPtr svg;
    rl2GraphicsContextPtr pdf;
    rl2GraphicsContextPtr ctx;
    rl2RasterPtr rst;
    rl2SectionPtr img;
    int ret;
    unsigned char *rgb;
    unsigned char *alpha;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

/* testing the SVG backend */
    svg = rl2_graph_create_svg_context ("./test_paint.svg", 2048, 2048);
    if (svg == NULL)
      {
	  fprintf (stderr, "Unable to create an SVG backend\n");
	  return -1;
      }
    ret = do_paint_test (svg);
    if (ret < 0)
	return ret;
    rl2_graph_destroy_context (svg);
    unlink ("./test_paint.svg");

/* testing the PDF backend */
    pdf =
	rl2_graph_create_pdf_context ("./test_paint.pdf", 300, 11.7, 8.3, 1.0,
				      1.0);
    if (pdf == NULL)
      {
	  fprintf (stderr, "Unable to create a PDF backend\n");
	  return -2;
      }
    ret = do_paint_test (pdf);
    if (ret < 0)
	return ret;
    rl2_graph_destroy_context (pdf);
    unlink ("./test_paint.pdf");

/* testing an ordinary graphics backend */
    ctx = rl2_graph_create_context (2048, 2048);
    if (ctx == NULL)
      {
	  fprintf (stderr, "Unable to create an ordinary graphics backend\n");
	  return -3;
      }
    ret = do_paint_test (ctx);
    if (ret < 0)
	return ret;
    rgb = rl2_graph_get_context_rgb_array (ctx);
    if (rgb == NULL)
      {
	  fprintf (stderr, "invalid RGB buffer from Graphics Context\n");
	  return -4;
      }
    alpha = rl2_graph_get_context_alpha_array (ctx);
    if (alpha == NULL)
      {
	  fprintf (stderr, "invalid Alpha buffer from Graphics Context\n");
	  return -5;
      }
    free (alpha);
    rl2_graph_destroy_context (ctx);

/* exporting a PNG image */
    rst = rl2_create_raster (2048, 2048, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     rgb, 2048 * 2048 * 3, NULL, NULL, 0, NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to create the output raster+mask\n");
	  return -6;
      }
    img =
	rl2_create_section ("beta", RL2_COMPRESSION_NONE,
			    RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			    rst);
    if (img == NULL)
      {
	  fprintf (stderr, "Unable to create the output section+mask\n");
	  return -7;
      }
    if (rl2_section_to_png (img, "./test_paint.png") != RL2_OK)
      {
	  fprintf (stderr, "Unable to write: test_paint.png\n");
	  return -8;
      }
    rl2_destroy_section (img);
    unlink ("./test_paint.png");

    return 0;
}
