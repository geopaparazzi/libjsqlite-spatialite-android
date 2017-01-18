/*

 test_svg.c -- RasterLite2 Test Case

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

The Original Code is the SpatiaLite library

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
#include <string.h>

#include <libxml/parser.h>
#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2svg.h"

static int
test_svg_null()
{
/* testing NULL / invalid */
    double width;
    double height;
    if (rl2_create_svg (NULL, 100) != NULL)
    {
		fprintf(stderr, "Unexpected success: Create SVG from NULL\n");
		return 0;
	}
    if (rl2_create_svg ((unsigned char *)"abcdefghijkhlmnoprqst", 15) != NULL)
    {
		fprintf(stderr, "Unexpected success: Create SVG from invalid XML\n");
		return 0;
	}
    if (rl2_get_svg_size (NULL, &width, &height) == RL2_OK)
    {
		fprintf(stderr, "Unexpected success: SVG size from NULL\n");
		return 0;
	}
    if (rl2_raster_from_svg (NULL, 100.0) != NULL)
    {
		fprintf(stderr, "Unexpected success: raster from NULL SVG\n");
		return 0;
	}
	return 1;
}

static int
test_svg (const char *path, int size, double w, double h)
{
/* rendering an SVG sample */
    char out_path[1024];
    unsigned char *svg_doc = NULL;
    int n_bytes;
    int rd;
    FILE *in = NULL;
    char *msg;
    rl2SvgPtr svg = NULL;
    double width;
    double height;
    double sz;
    rl2SectionPtr section = NULL;
    rl2RasterPtr raster = NULL;

/* loading the SVG in-memory */
    msg = sqlite3_mprintf ("Unable to load \"%s\" in-memory\n", path);
    in = fopen (path, "rb");
    if (in == NULL)
	goto error;
    if (fseek (in, 0, SEEK_END) < 0)
	goto error;
    n_bytes = ftell (in);
    rewind (in);
    svg_doc = malloc (n_bytes);
    rd = fread (svg_doc, 1, n_bytes, in);
    fclose (in);
    in = NULL;
    if (rd != n_bytes)
	goto error;

/* parsing the SVG document */
    sqlite3_free (msg);
    msg = sqlite3_mprintf ("Unable to parse \"%s\"\n", path);
    svg = rl2_create_svg (svg_doc, n_bytes);
    if (svg == NULL)
	goto error;

/* checking width and height */
    if (rl2_get_svg_size (svg, &width, &height) != RL2_OK)
      {
	  sqlite3_free (msg);
	  msg =
	      sqlite3_mprintf
	      ("Unable to retrieve width and height from \"%s\"\n", path);
	  goto error;
      }
    if (width != w)
      {
	  sqlite3_free (msg);
	  msg =
	      sqlite3_mprintf ("\"%s\" got unexpected width: %1.4f\n", path,
			       width);
	  goto error;
      }
    if (height != h)
      {
	  sqlite3_free (msg);
	  msg =
	      sqlite3_mprintf ("\"%s\" got unexpected height: %1.4f\n", path,
			       height);
	  goto error;
      }

/* rendering a PNG image */
    if (size == 0)
	sz = 24.0;
    else if (size == 1)
	sz = 64.0;
    else if (size == 2)
	sz = 128.0;
    else if (size == 3)
	sz = 256.0;
    else
	sz = 1024.0;
    raster = rl2_raster_from_svg (svg, sz);
    if (raster == NULL)
      {
	  sqlite3_free (msg);
	  msg =
	      sqlite3_mprintf
	      ("\"%s\" (%1.0f) unable to render a raster image\n", sz, path);
	  goto error;
      }
    section =
	rl2_create_section ("svg-test", RL2_COMPRESSION_PNG,
			    RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			    raster);
    if (section == NULL)
      {
	  sqlite3_free (msg);
	  msg =
	      sqlite3_mprintf ("\"%s\" (%1.0f) unable to create a Section\n",
			       sz, path);
	  goto error;
      }
    raster = NULL;
    sprintf (out_path, "./%s_%1.0f.png", path, sz);
    if (rl2_section_to_png (section, out_path) != RL2_OK)
      {
	  sqlite3_free (msg);
	  msg = sqlite3_mprintf ("Unable to export \"%s\"\n", out_path);
	  unlink (out_path);
	  goto error;
      }

    free (svg_doc);
    rl2_destroy_svg (svg);
    sqlite3_free (msg);
    rl2_destroy_section (section);
    unlink (out_path);
    return 1;

  error:
    if (section != NULL)
	rl2_destroy_section (section);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (svg_doc != NULL)
	free (svg_doc);
    if (svg != NULL)
	rl2_destroy_svg (svg);
    if (in != NULL)
	fclose (in);
    fprintf (stderr, msg);
    sqlite3_free (msg);
    return 0;
}

int
main (int argc, char *argv[])
{
    int size;
    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    for (size = 0; size < 5; size++)
      {
	  if (!test_svg ("./bicycle.svg", size, 580.0, 580.0))
	      return -1;
	  if (!test_svg ("./fastfood.svg", size, 580.0, 580.0))
	      return -2;
	  if (!test_svg ("./motorcycle.svg", size, 580.0, 580.0))
	      return -3;
	  if (!test_svg ("./restaurant.svg", size, 580.0, 580.0))
	      return -4;
	  if (!test_svg ("./car_repair.svg", size, 580.0, 580.0))
	      return -5;
	  if (!test_svg ("./hospital.svg", size, 580.0, 580.0))
	      return -6;
	  if (!test_svg ("./pharmacy.svg", size, 580.0, 580.0))
	      return -7;
	  if (!test_svg ("./supermarket.svg", size, 580.0, 580.0))
	      return -8;
	  if (!test_svg ("./doctors.svg", size, 580.0, 580.0))
	      return -9;
	  if (!test_svg ("./jewelry.svg", size, 580.0, 580.0))
	      return -10;
	  if (!test_svg ("./photo.svg", size, 580.0, 580.0))
	      return -11;
	  if (!test_svg ("./tobacco.svg", size, 580.0, 580.0))
	      return -12;
	  if (!test_svg ("./Car_Yellow.svg", size, 900.0, 600.0))
	      return -15;
	  if (!test_svg
	      ("./Royal_Coat_of_Arms_of_the_United_Kingdom.svg", size, 1550.0,
	       1500.0))
	      return -16;
	  if (!test_svg
	      ("./Flag_of_the_United_Kingdom.svg", size, 1200.0, 600.0))
	      return -17;
	  if (!test_svg
	      ("./Flag_of_the_United_States.svg", size, 1235.0, 650.0))
	      return -18;
	  if (!test_svg
	      ("./Roundel_of_the_Royal_Canadian_Air_Force_(1946-1965).svg",
	       size, 600.0, 600.0))
	      return -19;
	  if (!test_svg
	      ("./Roundel_of_the_Syrian_Air_Force.svg", size, 600.0, 600.0))
	      return -20;
	  if (!test_svg ("./Netherlands_roundel.svg", size, 600.0, 600.0))
	      return -21;
	  if (!test_svg ("./Coat_of_arms_Holy_See.svg", size, 1160.0, 1300.0))
	      return -22;
	  if (!test_svg
	      ("./Circle_and_quadratic_bezier.svg", size, 256.0, 256.0))
	      return -23;
	  if (!test_svg
	      ("./Negative_and_positive_skew_diagrams_(English).svg", size,
	       445.6460, 158.541990))
	      return -24;
      }
	  if (!test_svg_null())
	      return -25;

    xmlCleanupParser ();

    return 0;
}
