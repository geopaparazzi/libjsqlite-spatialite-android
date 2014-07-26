/*

 test_wr_tiff.c -- RasterLite2 Test Case

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
#include <math.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"

static unsigned char *
create_rainbow ()
{
/* creating a synthetic "rainbow" RGB buffer */
    int row;
    int col;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    red = 0;
    for (row = 0; row < 256; row++)
      {
	  green = 0;
	  blue = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		green++;
		blue--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = red;
		*p++ = 127;
		*p++ = 127;
	    }
	  green = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = 255 - red;
		*p++ = green;
		*p++ = blue;
		green--;
		blue++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = 127;
		*p++ = red;
		*p++ = red;
	    }
	  red++;
      }

    green = 0;
    for (row = 256; row < 512; row++)
      {
	  red = 0;
	  blue = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		red++;
		blue--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = 127;
		*p++ = green;
		*p++ = 127;
	    }
	  red = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = red;
		*p++ = 255 - green;
		*p++ = blue;
		red--;
		blue++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = green;
		*p++ = 127;
		*p++ = green;
	    }
	  green++;
      }

    blue = 0;
    for (row = 512; row < 768; row++)
      {
	  red = 0;
	  green = 255;
	  for (col = 0; col < 256; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
		red++;
		green--;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = 127;
		*p++ = 127;
		*p++ = blue;
	    }
	  red = 255;
	  green = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = 255 - blue;
		red--;
		green++;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = blue;
		*p++ = blue;
		*p++ = 127;
	    }
	  blue++;
      }

    red = 0;
    for (row = 768; row < 1024; row++)
      {
	  for (col = 0; col < 256; col++)
	    {
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	    }
	  for (col = 256; col < 512; col++)
	    {
		*p++ = red;
		*p++ = red;
		*p++ = red;
	    }
	  green = 255;
	  blue = 0;
	  for (col = 512; col < 768; col++)
	    {
		*p++ = 255 - red;
		*p++ = 255 - red;
		*p++ = 255 - red;
	    }
	  for (col = 768; col < 1024; col++)
	    {
		*p++ = 255;
		*p++ = 255;
		*p++ = 255;
	    }
	  red++;
      }
    return bufpix;
}

static unsigned char *
create_grayband ()
{
/* creating a synthetic "grayband" RGB buffer */
    int row;
    int col;
    unsigned char gray;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    gray = 0;
    for (row = 0; row < 256; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = gray;
		*p++ = gray;
		*p++ = gray;
	    }
	  gray++;
      }

    gray = 255;
    for (row = 256; row < 512; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = gray;
		*p++ = gray;
		*p++ = gray;
	    }
	  gray--;
      }

    gray = 0;
    for (row = 512; row < 768; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = gray;
		*p++ = gray;
		*p++ = gray;
	    }
	  gray++;
      }

    gray = 255;
    for (row = 768; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = gray;
		*p++ = gray;
		*p++ = gray;
	    }
	  gray--;
      }
    return bufpix;
}

static unsigned char *
create_palette ()
{
/* creating a synthetic "palette" RGB buffer */
    int row;
    int col;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    red = 0;
    for (row = 0; row < 256; row++)
      {
	  green = 0;
	  blue = 255;
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
	    }
	  if (red > 247)
	      red = 0;
	  else
	      red += 8;
      }

    red = 255;
    for (row = 256; row < 512; row++)
      {
	  green = 255;
	  blue = 192;
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
	    }
	  if (red < 9)
	      red = 255;
	  else
	      red -= 8;
      }

    red = 0;
    for (row = 512; row < 768; row++)
      {
	  green = 0;
	  blue = 128;
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
	    }
	  if (red > 247)
	      red = 0;
	  else
	      red += 8;
      }

    red = 255;
    for (row = 768; row < 1024; row++)
      {
	  green = 128;
	  blue = 0;
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = red;
		*p++ = green;
		*p++ = blue;
	    }
	  if (red < 9)
	      red = 255;
	  else
	      red -= 8;
      }
    return bufpix;
}

static unsigned char *
create_monochrome ()
{
/* creating a synthetic "monochrome" RGB buffer */
    int row;
    int col;
    int mode = 0;
    int bufsize = 1024 * 1024 * 3;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;

    if (bufpix == NULL)
	return NULL;

    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	    }
      }
    p = bufpix;

    for (row = 0; row < 256; row += 4)
      {
	  int mode2 = mode;
	  for (col = 0; col < 1024; col += 4)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 12;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 4)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 12;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 4)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 4;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 4)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 4;
		      mode2 = 1;
		  }
	    }
	  if (mode)
	      mode = 0;
	  else
	      mode = 1;
      }

    for (row = 256; row < 512; row += 8)
      {
	  int mode2 = mode;
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 24;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 8;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 24;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 8;
		      mode2 = 1;
		  }
	    }
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 8;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 24;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 24;
		      mode2 = 1;
		  }
	    }
	  mode2 = mode;
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 24;
		      mode2 = 1;
		  }
	    }
	  if (mode)
	      mode = 0;
	  else
	      mode = 1;
      }

    for (row = 512; row < 768; row++)
      {
	  int mode2 = 1;
	  for (col = 0; col < 1024; col += 8)
	    {
		if (mode2)
		  {
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      *p++ = 255;
		      mode2 = 0;
		  }
		else
		  {
		      p += 8;
		      mode2 = 1;
		  }
	    }
      }

    mode = 1;
    for (row = 768; row < 1024; row += 8)
      {
	  int x;
	  for (x = 0; x < 8; x++)
	    {
		for (col = 0; col < 1024; col++)
		  {
		      if (mode)
			{
			    *p++ = 255;
			    *p++ = 255;
			    *p++ = 255;
			}
		      else
			  p += 3;
		  }
	    }
	  if (mode)
	      mode = 0;
	  else
	      mode = 1;
      }

    return bufpix;
}

static unsigned char *
create_grid_8 ()
{
/* creating a synthetic "grid_8" RGB buffer */
    int row;
    int col;
    int bufsize = 1024 * 1024;
    unsigned char *bufpix = malloc (bufsize);
    char *p = (char *) bufpix;
    char sample = -128;

    if (bufpix == NULL)
	return NULL;
    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		if (row == 24 && col == 28)
		    *p++ = (char) -17;
		else if (row == 822 && col == 932)
		    *p++ = (char) -117;
		else
		    *p++ = sample;
		if (sample == 127)
		    sample = -128;
		else
		    sample++;
	    }
      }

    return bufpix;
}

static unsigned char *
create_grid_u8 ()
{
/* creating a synthetic "grid_u8" RGB buffer */
    int row;
    int col;
    int bufsize = 1024 * 1024;
    unsigned char *bufpix = malloc (bufsize);
    unsigned char *p = bufpix;
    unsigned char sample = 0;

    if (bufpix == NULL)
	return NULL;
    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		if (row == 24 && col == 28)
		    *p++ = (unsigned char) 17;
		else if (row == 822 && col == 932)
		    *p++ = (unsigned char) 117;
		else
		    *p++ = sample;
		if (sample == 255)
		    sample = 0;
		else
		    sample++;
	    }
      }

    return bufpix;
}

static unsigned char *
create_grid_16 ()
{
/* creating a synthetic "grid_16" RGB buffer */
    int row;
    int col;
    int bufsize = 1024 * 1024 * sizeof (short);
    unsigned char *bufpix = malloc (bufsize);
    short *p = (short *) bufpix;
    short sample = -32768;

    if (bufpix == NULL)
	return NULL;
    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		if (row == 24 && col == 28)
		    *p++ = (short) -1347;
		else if (row == 822 && col == 932)
		    *p++ = (short) -11347;
		else
		    *p++ = sample;
		if (sample == 32767)
		    sample = -32768;
		else
		    sample++;
	    }
      }

    return bufpix;
}

static unsigned char *
create_grid_u16 ()
{
/* creating a synthetic "grid_u16" RGB buffer */
    int row;
    int col;
    int bufsize = 1024 * 1024 * sizeof (unsigned short);
    unsigned char *bufpix = malloc (bufsize);
    unsigned short *p = (unsigned short *) bufpix;
    unsigned short sample = 0;

    if (bufpix == NULL)
	return NULL;
    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		if (row == 24 && col == 28)
		    *p++ = (unsigned short) 1347;
		else if (row == 822 && col == 932)
		    *p++ = (unsigned short) 11347;
		else
		    *p++ = sample;
		if (sample == 65535)
		    sample = 0;
		else
		    sample++;
	    }
      }

    return bufpix;
}

static unsigned char *
create_grid_32 ()
{
/* creating a synthetic "grid_32" RGB buffer */
    int row;
    int col;
    int bufsize = 1024 * 1024 * sizeof (int);
    unsigned char *bufpix = malloc (bufsize);
    int *p = (int *) bufpix;
    int sample = -50000;

    if (bufpix == NULL)
	return NULL;
    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		if (row == 24 && col == 28)
		    *p++ = (int) -1000347;
		else if (row == 822 && col == 932)
		    *p++ = (int) -10001347;
		else
		    *p++ = sample++;
	    }
      }

    return bufpix;
}

static unsigned char *
create_grid_u32 ()
{
/* creating a synthetic "grid_u32" RGB buffer */
    int row;
    int col;
    int bufsize = 1024 * 1024 * sizeof (unsigned int);
    unsigned char *bufpix = malloc (bufsize);
    unsigned int *p = (unsigned int *) bufpix;
    unsigned int sample = 124346;

    if (bufpix == NULL)
	return NULL;
    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		if (row == 24 && col == 28)
		    *p++ = (unsigned int) 1000347;
		else if (row == 822 && col == 932)
		    *p++ = (unsigned int) 10001347;
		else
		    *p++ = sample++;
	    }
      }

    return bufpix;
}

static unsigned char *
create_grid_float ()
{
/* creating a synthetic "grid_float" RGB buffer */
    int row;
    int col;
    int bufsize = 1024 * 1024 * sizeof (float);
    unsigned char *bufpix = malloc (bufsize);
    float *p = (float *) bufpix;
    float sample = -1000.5;

    if (bufpix == NULL)
	return NULL;
    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		if (row == 24 && col == 28)
		    *p++ = (float) 10347.25;
		else if (row == 822 && col == 932)
		    *p++ = (float) -101347.75;
		else
		    *p++ = sample;
		sample += 0.5;
	    }
      }

    return bufpix;
}

static unsigned char *
create_grid_double ()
{
/* creating a synthetic "grid_double" RGB buffer */
    int row;
    int col;
    int bufsize = 1024 * 1024 * sizeof (double);
    unsigned char *bufpix = malloc (bufsize);
    double *p = (double *) bufpix;
    double sample = -12000.5;

    if (bufpix == NULL)
	return NULL;
    for (row = 0; row < 1024; row++)
      {
	  for (col = 0; col < 1024; col++)
	    {
		if (row == 24 && col == 28)
		    *p++ = (double) 2000347.25;
		else if (row == 822 && col == 932)
		    *p++ = (double) -20001347.75;
		else
		    *p++ = sample;
		sample += 0.5;
	    }
      }

    return bufpix;
}

static int
do_one_rgb_test (const unsigned char *rgb, const char *path, int tiled,
		 unsigned char compression)
{
/* performing a single RGB test */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *xpath;
    unsigned int xwidth;
    unsigned int xheight;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    unsigned char xcompression;
    if (tiled == 0)
	tile_size = 1;
    tiff = rl2_create_tiff_destination (path, 1024, 1024, RL2_SAMPLE_UINT8,
					RL2_PIXEL_RGB, 3, NULL, compression,
					tiled, tile_size);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create TIFF \"%s\"\n", path);
	  goto error;
      }

    xpath = rl2_get_tiff_destination_path (tiff);
    if (xpath == NULL)
      {
	  fprintf (stderr, "Get destination path: unexpected NULL\n");
	  goto error;
      }
    if (strcmp (xpath, path) != 0)
      {
	  fprintf (stderr, "Mismatching destination path \"%s\"\n", xpath);
	  goto error;
      }
    if (rl2_get_tiff_destination_size (tiff, &xwidth, &xheight) != RL2_OK)
      {
	  fprintf (stderr, "Get destination size error\n");
	  goto error;
      }
    if (xwidth != 1024)
      {
	  fprintf (stderr, "Unexpected Width %d\n", xwidth);
	  goto error;
      }
    if (xheight != 1024)
      {
	  fprintf (stderr, "Unexpected Height %d\n", xheight);
	  goto error;
      }
    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_RGB)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 3)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }
    if (rl2_get_tiff_destination_compression (tiff, &xcompression) != RL2_OK)
      {
	  fprintf (stderr, "Get destination compression error\n");
	  goto error;
      }
    if (xcompression != compression)
      {
	  fprintf (stderr, "Unexpected compression %02x\n", xcompression);
	  goto error;
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size * 3;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				    {
					*p_out++ = 0;
					*p_out++ = 0;
					*p_out++ = 0;
				    }
			      }
			    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					*p_out++ = 0;
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
				  *p_out++ = *p_in++;
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT8,
						  RL2_PIXEL_RGB, 3, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * 3;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = rgb + (row * 1024 * 3);
		p_out = bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					    bufpix, bufpix_size, NULL, NULL, 0,
					    NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }
/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return -1;
}

static int
check_origin (const char *path, const char *tfw_path, int srid, double minx,
	      double miny, double maxx, double maxy, double hres, double vres,
	      unsigned char compression, unsigned char sample_type,
	      unsigned char pixel_type, int mode)
{
/* checking a georeferenced TIFF origin */
    rl2TiffOriginPtr origin = NULL;
    unsigned char xcompression;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    unsigned int xwidth;
    unsigned int xheight;
    int xsrid;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double horzRes;
    double vertRes;
    double diff;
    int num_bands = 1;
    rl2CoveragePtr coverage = NULL;
    rl2RasterPtr raster = NULL;
    rl2PixelPtr pixel = NULL;
    char v8;
    unsigned char vu8;
    short v16;
    unsigned short vu16;
    int v32;
    unsigned int vu32;
    float vflt;
    double vdbl;
    int is_tiled;
    unsigned int tileWidth;
    unsigned int tileHeight;
    unsigned int rowsPerStrip;

    if (pixel_type == RL2_PIXEL_RGB)
	num_bands = 3;

    if (mode == RL2_TIFF_GEOTIFF)
      {
	  origin =
	      rl2_create_geotiff_origin (path, -1, RL2_SAMPLE_UNKNOWN,
					 RL2_PIXEL_UNKNOWN, RL2_BANDS_UNKNOWN);
	  if (origin == NULL)
	    {
		fprintf (stderr, "ERROR: unable to open %s as GeoTIFF\n", path);
		goto error;
	    }
      }
    else if (mode == RL2_TIFF_WORLDFILE)
      {
	  origin =
	      rl2_create_tiff_worldfile_origin (path, 4326, RL2_SAMPLE_UNKNOWN,
						RL2_PIXEL_UNKNOWN,
						RL2_BANDS_UNKNOWN);
	  if (origin == NULL)
	    {
		fprintf (stderr, "ERROR: unable to open %s as TIFF+TFW\n",
			 path);
		goto error;
	    }
      }

    if (rl2_get_tiff_origin_size (origin, &xwidth, &xheight) != RL2_OK)
      {
	  fprintf (stderr, "CheckOrigin - Get TIFF size error\n");
	  goto error;
      }
    if (xwidth != 1024)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF width %u\n", xwidth);
	  goto error;
      }
    if (xheight != 1024)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF height %u\n",
		   xheight);
	  goto error;
      }
    if (rl2_get_tiff_origin_compression (origin, &xcompression) != RL2_OK)
      {
	  fprintf (stderr, "CheckOrigin - Get TIFF compression error\n");
	  goto error;
      }
    if (compression != xcompression)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF compression %02x\n",
		   xcompression);
	  goto error;
      }
    if (rl2_get_tiff_origin_type
	(origin, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "CheckOrigin - Get TIFF type error\n");
	  goto error;
      }
    if (xsample_type != sample_type)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF sample type %02x\n",
		   xsample_type);
	  goto error;
      }
    if (xpixel_type != pixel_type && alias_pixel_type != pixel_type)
      {
	  fprintf (stderr,
		   "CheckOrigin - unexpected TIFF pixel type %02x alias %02x\n",
		   pixel_type, alias_pixel_type);
	  goto error;
      }
    if (xnum_bands != num_bands)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF numBands %02x\n",
		   xnum_bands);
	  goto error;
      }
    if (rl2_get_tiff_origin_srid (origin, &xsrid) != RL2_OK)
      {
	  fprintf (stderr, "CheckOrigin - Get TIFF SRID error\n");
	  goto error;
      }
    if (xsrid != srid)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF SRID %02x\n", xsrid);
	  goto error;
      }
    if (rl2_get_tiff_origin_extent (origin, &minX, &minY, &maxX, &maxY) !=
	RL2_OK)
      {
	  fprintf (stderr, "CheckOrigin - Get TIFF Extent error\n");
	  goto error;
      }
    diff = fabs (minX - minx);
    if (diff > 0.0000001)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF MinX %1.8f\n", minX);
	  goto error;
      }
    diff = fabs (minY - miny);
    if (diff > 0.0000001)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF MinY %1.8f\n", minY);
	  goto error;
      }
    diff = fabs (maxX - maxx);
    if (diff > 0.0000001)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF MaxX %1.8f\n", maxX);
	  goto error;
      }
    diff = fabs (maxY - maxy);
    if (diff > 0.0000001)
      {
	  fprintf (stderr, "CheckOrigin - unexpected TIFF MaxY %1.8f\n", maxY);
	  goto error;
      }
    if (rl2_get_tiff_origin_resolution (origin, &horzRes, &vertRes) != RL2_OK)
      {
	  fprintf (stderr, "CheckOrigin - Get TIFF Resolution error\n");
	  goto error;
      }
    diff = fabs (horzRes - hres);
    if (diff > 0.0000001)
      {
	  fprintf (stderr,
		   "CheckOrigin - unexpected TIFF Horizontal Resolution %1.8f\n",
		   horzRes);
	  goto error;
      }
    diff = fabs (vertRes - vres);
    if (diff > 0.0000001)
      {
	  fprintf (stderr,
		   "CheckOrigin - unexpected TIFF Vertical Resolution %1.8f\n",
		   vertRes);
	  goto error;
      }
    if (rl2_is_tiled_tiff_origin (origin, &is_tiled) != RL2_OK)
      {
	  fprintf (stderr, "CheckOrigin - Is Tile TIFF error\n");
	  goto error;
      }
    if (is_tiled)
      {
	  if (rl2_get_tiff_origin_tile_size (origin, &tileWidth, &tileHeight) !=
	      RL2_OK)
	    {
		fprintf (stderr, "CheckOrigin - Get Tile Size error\n");
		goto error;
	    }
	  if (tileWidth != 128)
	    {
		fprintf (stderr, "CheckOrigin - unexpected TIFF tileWidth %d\n",
			 tileWidth);
		goto error;
	    }
	  if (tileHeight != 128)
	    {
		fprintf (stderr,
			 "CheckOrigin - unexpected TIFF tileHeight %d\n",
			 tileHeight);
		goto error;
	    }
      }
    else
      {
	  if (rl2_get_tiff_origin_strip_size (origin, &rowsPerStrip) != RL2_OK)
	    {
		fprintf (stderr, "CheckOrigin - Get Strip Size error\n");
		goto error;
	    }
	  if (rowsPerStrip != 1 && rowsPerStrip != 8)
	    {
		fprintf (stderr,
			 "CheckOrigin - unexpected TIFF rowsPerStrip %d\n",
			 rowsPerStrip);
		goto error;
	    }
      }

    if (pixel_type == RL2_PIXEL_DATAGRID)
      {
	  /* testing few pixels */
	  coverage =
	      rl2_create_coverage ("test", sample_type, pixel_type, num_bands,
				   RL2_COMPRESSION_NONE, 0, 512, 512, NULL);
	  if (coverage == NULL)
	    {
		fprintf (stderr, "ERROR: unable to create the Coverage\n");
		goto error_pix;
	    }
	  pixel = rl2_create_pixel (sample_type, RL2_PIXEL_DATAGRID, 1);
	  if (pixel == NULL)
	    {
		fprintf (stderr, "ERROR: unable to create a Pixel\n");
	    }
	  raster = rl2_get_tile_from_tiff_origin (coverage, origin, 0, 0, -1);
	  if (raster == NULL)
	    {
		fprintf (stderr, "ERROR: unable to retrieve a TIFF tile (1)\n");
		goto error_pix;
	    }
	  if (rl2_get_raster_pixel (raster, pixel, 24, 28) != RL2_OK)
	    {
		fprintf (stderr,
			 "ERROR: unable to get pixel (24,28) from tile (0,0)\n");
		goto error_pix;
	    }
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
		if (rl2_get_pixel_sample_int8 (pixel, &v8) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (24,28) from tile (0,0)\n");
		      goto error_pix;
		  }
		if (v8 != -17)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected INT8 pixel (24,28) from tile (0,0) %d\n",
			       v8);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_UINT8:
		if (rl2_get_pixel_sample_uint8 (pixel, RL2_DATAGRID_BAND, &vu8)
		    != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (24,28) from tile (0,0)\n");
		      goto error_pix;
		  }
		if (vu8 != 17)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected UINT8 pixel (24,28) from tile (0,0) %d\n",
			       vu8);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_INT16:
		if (rl2_get_pixel_sample_int16 (pixel, &v16) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (24,28) from tile (0,0)\n");
		      goto error_pix;
		  }
		if (v16 != -1347)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected INT16 pixel (24,28) from tile (0,0) %d\n",
			       v16);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_UINT16:
		if (rl2_get_pixel_sample_uint16
		    (pixel, RL2_DATAGRID_BAND, &vu16) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (24,28) from tile (0,0)\n");
		      goto error_pix;
		  }
		if (vu16 != 1347)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected UINT16 pixel (24,28) from tile (0,0) %d\n",
			       vu16);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_INT32:
		if (rl2_get_pixel_sample_int32 (pixel, &v32) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (24,28) from tile (0,0)\n");
		      goto error_pix;
		  }
		if (v32 != -1000347)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected INT32 pixel (24,28) from tile (0,0) %d\n",
			       v32);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_UINT32:
		if (rl2_get_pixel_sample_uint32 (pixel, &vu32) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (24,28) from tile (0,0)\n");
		      goto error_pix;
		  }
		if (vu32 != 1000347)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected UINT32 pixel (24,28) from tile (0,0) %d\n",
			       vu32);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_FLOAT:
		if (rl2_get_pixel_sample_float (pixel, &vflt) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (24,28) from tile (0,0)\n");
		      goto error_pix;
		  }
		if (vflt != 10347.250000)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected FLOAT pixel (24,28) from tile (0,0) %1.6f\n",
			       vflt);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_DOUBLE:
		if (rl2_get_pixel_sample_double (pixel, &vdbl) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (24,28) from tile (0,0)\n");
		      goto error_pix;
		  }
		if (vdbl != 2000347.250000)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected DOUBLE pixel (24,28) from tile (0,0) %1.6f\n",
			       vdbl);
		      goto error_pix;
		  }
		break;
	    };
	  rl2_destroy_raster (raster);
	  raster =
	      rl2_get_tile_from_tiff_origin (coverage, origin, 512, 512, -1);
	  if (raster == NULL)
	    {
		fprintf (stderr, "ERROR: unable to retrieve a TIFF tile (2)\n");
		goto error_pix;
	    }
	  if (rl2_get_raster_pixel (raster, pixel, 310, 420) != RL2_OK)
	    {
		fprintf (stderr,
			 "ERROR: unable to get pixel (310,420) from tile (512,512)\n");
		goto error_pix;
	    }
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
		if (rl2_get_pixel_sample_int8 (pixel, &v8) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (310,420) from tile (512,512)\n");
		      goto error_pix;
		  }
		if (v8 != -117)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected INT8 pixel (310,420) from tile (512,512) %d\n",
			       v8);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_UINT8:
		if (rl2_get_pixel_sample_uint8 (pixel, RL2_DATAGRID_BAND, &vu8)
		    != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (310,420) from tile (512,512)\n");
		      goto error_pix;
		  }
		if (vu8 != 117)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected UINT8 pixel (310,420) from tile (512,512) %d\n",
			       vu8);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_INT16:
		if (rl2_get_pixel_sample_int16 (pixel, &v16) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (310,420) from tile (512,512)\n");
		      goto error_pix;
		  }
		if (v16 != -11347)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected INT16 pixel (310,420) from tile (512,512) %d\n",
			       v16);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_UINT16:
		if (rl2_get_pixel_sample_uint16
		    (pixel, RL2_DATAGRID_BAND, &vu16) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (310,420) from tile (512,512)\n");
		      goto error_pix;
		  }
		if (vu16 != 11347)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected UINT16 pixel (310,420) from tile (512,512) %d\n",
			       vu16);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_INT32:
		if (rl2_get_pixel_sample_int32 (pixel, &v32) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (310,420) from tile (512,512)\n");
		      goto error_pix;
		  }
		if (v32 != -10001347)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected INT32 pixel (310,420) from tile (512,512) %d\n",
			       v32);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_UINT32:
		if (rl2_get_pixel_sample_uint32 (pixel, &vu32) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (310,420) from tile (512,512)\n");
		      goto error_pix;
		  }
		if (vu32 != 10001347)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected UINT32 pixel (310,420) from tile (512,512) %d\n",
			       vu32);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_FLOAT:
		if (rl2_get_pixel_sample_float (pixel, &vflt) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (310,420) from tile (512,512)\n");
		      goto error_pix;
		  }
		if (vflt != -101347.75)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected FLOAT pixel (310,420) from tile (512,512) %1.6f\n",
			       vflt);
		      goto error_pix;
		  }
		break;
	    case RL2_SAMPLE_DOUBLE:
		if (rl2_get_pixel_sample_double (pixel, &vdbl) != RL2_OK)
		  {
		      fprintf (stderr,
			       "invalid pixel (310,420) from tile (512,512)\n");
		      goto error_pix;
		  }
		if (vdbl != -20001347.75)
		  {
		      fprintf (stderr,
			       "ERROR: unexpected DOUBLE pixel (310,420) from tile (512,512) %1.6f\n",
			       vdbl);
		      goto error_pix;
		  }
		break;
	    };
	  rl2_destroy_raster (raster);
	  rl2_destroy_pixel (pixel);
	  rl2_destroy_coverage (coverage);
      }

    rl2_destroy_tiff_origin (origin);
    unlink (path);
    if (tfw_path != NULL)
	unlink (tfw_path);
    return 1;

  error_pix:
    rl2_destroy_raster (raster);
    rl2_destroy_pixel (pixel);
    rl2_destroy_coverage (coverage);

  error:
    if (origin != NULL)
	rl2_destroy_tiff_origin (origin);
    unlink (path);
    if (tfw_path != NULL)
	unlink (tfw_path);
    return 0;
}

static int
do_one_rgb_test_geotiff1 (const unsigned char *rgb, sqlite3 * handle,
			  const char *path, int tiled,
			  unsigned char compression)
{
/* performing a single RGB test - GeoTIFF (1) */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    double res = 180.0 / 1024.0;
    int xsrid;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double hres;
    double vres;
    int is_geotiff;
    int is_tfw;
    const char *tfw_path;
    int is_tiled;
    unsigned int tileWidth;
    unsigned int tileHeight;
    unsigned int rowsPerStrip;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					NULL, compression, tiled, tile_size,
					4326, 0.0, -90.0, 180.0, 90.0, res, res,
					0);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_is_tiled_tiff_destination (tiff, &is_tiled) != RL2_OK)
      {
	  fprintf (stderr,
		   "Unable to test if the destination is Tiled or Striped\n");
	  goto error;
      }
    if (is_tiled)
      {
	  if (rl2_get_tiff_destination_tile_size (tiff, &tileWidth, &tileHeight)
	      != RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to retrieve the destination Tile Size\n");
		goto error;
	    }
	  if (tileWidth != 128)
	    {
		fprintf (stderr, "Unexpected destination tileWidth %d\n",
			 tileWidth);
		goto error;
	    }
	  if (tileHeight != 128)
	    {
		fprintf (stderr, "Unexpected destination tileHeight %d\n",
			 tileHeight);
		goto error;
	    }
      }
    else
      {
	  if (rl2_get_tiff_destination_strip_size (tiff, &rowsPerStrip) !=
	      RL2_OK)
	    {
		fprintf (stderr,
			 "Unable to retrieve the destination RowsPerStrip\n");
		goto error;
	    }
	  if (rowsPerStrip != 1 && rowsPerStrip != 8)
	    {
		fprintf (stderr, "Unexpected destination rowsPerStrip %d\n",
			 rowsPerStrip);
		goto error;
	    }
      }
    if (rl2_get_tiff_destination_srid (tiff, &xsrid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to retrieve the destination SRID\n");
	  goto error;
      }
    if (xsrid != 4326)
      {
	  fprintf (stderr, "Unexpected destination SRID %d\n", xsrid);
	  goto error;
      }
    if (rl2_get_tiff_destination_extent (tiff, &minx, &miny, &maxx, &maxy) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to retrieve the destination Extent\n");
	  goto error;
      }
    if (minx != 0.0)
      {
	  fprintf (stderr, "Unexpected Extent MinX %1.8f\n", minx);
	  goto error;
      }
    if (miny != -90.0)
      {
	  fprintf (stderr, "Unexpected Extent MinY %1.8f\n", miny);
	  goto error;
      }
    if (maxx != 180.0)
      {
	  fprintf (stderr, "Unexpected Extent MaxX %1.8f\n", maxx);
	  goto error;
      }
    if (maxy != 90.0)
      {
	  fprintf (stderr, "Unexpected Extent MaxY %1.8f\n", maxy);
	  goto error;
      }
    if (rl2_get_tiff_destination_resolution (tiff, &hres, &vres) != RL2_OK)
      {
	  fprintf (stderr, "Unable to retrieve the destination Resolution\n");
	  goto error;
      }
    if (hres >= 0.17578126 || hres < 0.17578125)
      {
	  fprintf (stderr, "Unexpected Horizontal Resolution %1.8f\n", hres);
	  goto error;
      }
    if (vres >= 0.17578126 || vres < 0.17578125)
      {
	  fprintf (stderr, "Unexpected Vertical Resolution %1.8f\n", vres);
	  goto error;
      }
    if (rl2_is_geotiff_destination (tiff, &is_geotiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to test IsGeoTIFF\n");
	  goto error;
      }
    if (is_geotiff != 1)
      {
	  fprintf (stderr, "Unexpected IsGeoTIFF %d\n", is_geotiff);
	  goto error;
      }
    if (rl2_is_tiff_worldfile_destination (tiff, &is_tfw) != RL2_OK)
      {
	  fprintf (stderr, "Unable to test IsWorldFile\n");
	  goto error;
      }
    if (is_tfw != 0)
      {
	  fprintf (stderr, "Unexpected IsWorldFile %d\n", is_tfw);
	  goto error;
      }
    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path != NULL)
      {
	  fprintf (stderr, "Unexpected NOT NULL TFW Path\n");
	  goto error;
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size * 3;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				    {
					*p_out++ = 0;
					*p_out++ = 0;
					*p_out++ = 0;
				    }
			      }
			    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					*p_out++ = 0;
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
				  *p_out++ = *p_in++;
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT8,
						  RL2_PIXEL_RGB, 3, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * 3;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = rgb + (row * 1024 * 3);
		p_out = bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					    bufpix, bufpix_size, NULL, NULL, 0,
					    NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW (error expected) */
    if (rl2_write_tiff_worldfile (tiff) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected success TIFF TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw_path, 4326, 0.0, -90.0, 180.0, 90.0, res, res, compression,
	 RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, RL2_TIFF_GEOTIFF))
	goto error;
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return -1;
}

static int
do_one_rgb_test_geotiff2 (const unsigned char *rgb, sqlite3 * handle,
			  const char *path, int tiled,
			  unsigned char compression)
{
/* performing a single RGB test - GeoTIFF (2) */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    int xsrid;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double hres;
    double vres;
    int is_geotiff;
    int is_tfw;
    const char *tfw_path;
    char *tfw = NULL;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_srid (tiff, &xsrid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to retrieve the destination SRID\n");
	  goto error;
      }
    if (xsrid != 32632)
      {
	  fprintf (stderr, "Unexpected destination SRID %d\n", xsrid);
	  goto error;
      }
    if (rl2_get_tiff_destination_extent (tiff, &minx, &miny, &maxx, &maxy) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to retrieve the destination Extent\n");
	  goto error;
      }
    if (minx != 100000.0)
      {
	  fprintf (stderr, "Unexpected Extent MinX %1.8f\n", minx);
	  goto error;
      }
    if (miny != 1000000.0)
      {
	  fprintf (stderr, "Unexpected Extent MinY %1.8f\n", miny);
	  goto error;
      }
    if (maxx != 101024.0)
      {
	  fprintf (stderr, "Unexpected Extent MaxX %1.8f\n", maxx);
	  goto error;
      }
    if (maxy != 1001024.0)
      {
	  fprintf (stderr, "Unexpected Extent MaxY %1.8f\n", maxy);
	  goto error;
      }
    if (rl2_get_tiff_destination_resolution (tiff, &hres, &vres) != RL2_OK)
      {
	  fprintf (stderr, "Unable to retrieve the destination Resolution\n");
	  goto error;
      }
    if (hres >= 1.00000001 || hres < 1.00000000)
      {
	  fprintf (stderr, "Unexpected Horizontal Resolution %1.8f\n", hres);
	  goto error;
      }
    if (vres >= 1.00000001 || vres < 1.00000000)
      {
	  fprintf (stderr, "Unexpected Vertical Resolution %1.8f\n", vres);
	  goto error;
      }
    if (rl2_is_geotiff_destination (tiff, &is_geotiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to test IsGeoTIFF\n");
	  goto error;
      }
    if (is_geotiff != 1)
      {
	  fprintf (stderr, "Unexpected IsGeoTIFF %d\n", is_geotiff);
	  goto error;
      }
    if (rl2_is_tiff_worldfile_destination (tiff, &is_tfw) != RL2_OK)
      {
	  fprintf (stderr, "Unable to test IsWorldFile\n");
	  goto error;
      }
    if (is_tfw != 1)
      {
	  fprintf (stderr, "Unexpected IsWorldFile %d\n", is_tfw);
	  goto error;
      }
    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size * 3;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				    {
					*p_out++ = 0;
					*p_out++ = 0;
					*p_out++ = 0;
				    }
			      }
			    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					*p_out++ = 0;
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
				  *p_out++ = *p_in++;
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT8,
						  RL2_PIXEL_RGB, 3, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * 3;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = rgb + (row * 1024 * 3);
		p_out = bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					    bufpix, bufpix_size, NULL, NULL, 0,
					    NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_rgb_test_tfw (const unsigned char *rgb, const char *path, int tiled,
		     unsigned char compression)
{
/* performing a single RGB test - TFW */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    double res = 180.0 / 1024.0;
    int is_geotiff;
    int is_tfw;
    const char *tfw_path;
    char *tfw = NULL;
    if (tiled == 0)
	tile_size = 1;
    tiff =
	rl2_create_tiff_worldfile_destination (path, 1024, 1024,
					       RL2_SAMPLE_UINT8, RL2_PIXEL_RGB,
					       3, NULL, compression, tiled,
					       tile_size, 4326, 0.0, -90.0,
					       180.0, 90.0, res, res);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create TIFF+TFW \"%s\"\n", path);
	  goto error;
      }
    if (rl2_is_geotiff_destination (tiff, &is_geotiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to test IsGeoTIFF\n");
	  goto error;
      }
    if (is_geotiff != 0)
      {
	  fprintf (stderr, "Unexpected IsGeoTIFF %d\n", is_geotiff);
	  goto error;
      }
    if (rl2_is_tiff_worldfile_destination (tiff, &is_tfw) != RL2_OK)
      {
	  fprintf (stderr, "Unable to test IsWorldFile\n");
	  goto error;
      }
    if (is_tfw != 1)
      {
	  fprintf (stderr, "Unexpected IsWorldFile %d\n", is_tfw);
	  goto error;
      }
    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size * 3;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				    {
					*p_out++ = 0;
					*p_out++ = 0;
					*p_out++ = 0;
				    }
			      }
			    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					*p_out++ = 0;
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
				  *p_out++ = *p_in++;
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT8,
						  RL2_PIXEL_RGB, 3, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * 3;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = rgb + (row * 1024 * 3);
		p_out = bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					    bufpix, bufpix_size, NULL, NULL, 0,
					    NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 4326, 0.0, -90.0, 180.0, 90.0, res, res, compression,
	 RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, RL2_TIFF_WORLDFILE))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_gray_test (const unsigned char *rgb, const char *path, int tiled,
		  unsigned char compression)
{
/* performing a single GrayBand test */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    unsigned char xcompression;
    if (tiled == 0)
	tile_size = 1;
    tiff = rl2_create_tiff_destination (path, 1024, 1024, RL2_SAMPLE_UINT8,
					RL2_PIXEL_GRAYSCALE, 1, NULL,
					compression, tiled, tile_size);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create TIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_GRAYSCALE)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }
    if (rl2_get_tiff_destination_compression (tiff, &xcompression) != RL2_OK)
      {
	  fprintf (stderr, "Get destination compression error\n");
	  goto error;
      }
    if (xcompression != compression)
      {
	  fprintf (stderr, "Unexpected compression %02x\n", xcompression);
	  goto error;
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					p_in += 2;
					continue;
				    }
				  *p_out++ = *p_in++;
				  p_in += 2;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT8,
						  RL2_PIXEL_GRAYSCALE, 1,
						  bufpix, bufpix_size, NULL,
						  NULL, 0, NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = rgb + (row * 1024 * 3);
		p_out = bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		      p_in += 2;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT8,
					    RL2_PIXEL_GRAYSCALE, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }
/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return -1;
}

static void
add_palette_color (int *r, int *g, int *b, unsigned char red,
		   unsigned char green, unsigned char blue)
{
/* inserting a new color into the Palette */
    int i;
    for (i = 0; i < 255; i++)
      {
	  if (*(r + i) == red && *(g + i) == green && *(b + i) == blue)
	      return;
      }
    for (i = 0; i < 255; i++)
      {
	  if (*(r + i) == -1 && *(g + i) == -1 && *(b + i) == -1)
	    {
		*(r + i) = red;
		*(g + i) = green;
		*(b + i) = blue;
		return;
	    }
      }
}

static rl2PalettePtr
build_palette (const unsigned char *rgb, int *r, int *g, int *b)
{
/* building a Palette matching the RGB pix-buffer */
    int row;
    int col;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    const unsigned char *p_in = rgb;
    rl2PalettePtr palette;
    if (rgb != NULL)
      {
	  for (row = 0; row < 1024; row++)
	    {
		for (col = 0; col < 1024; col++)
		  {
		      red = *p_in++;
		      green = *p_in++;
		      blue = *p_in++;
		      add_palette_color (r, g, b, red, green, blue);
		  }
	    }
      }
    palette = rl2_create_palette (256);
    for (row = 0; row < 256; row++)
	rl2_set_palette_color (palette, row, *(r + row), *(g + row),
			       *(b + row));
    return palette;
}

static int
do_one_palette_test (const unsigned char *rgb, const char *path, int tiled,
		     unsigned char compression)
{
/* performing a single Palette test */
    int row;
    int col;
    int x;
    int y;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char index;
    int r[256];
    int g[256];
    int b[256];
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    rl2PalettePtr palette;
    rl2PalettePtr palette2;
    int tile_size = 128;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    unsigned char xcompression;
    if (tiled == 0)
	tile_size = 1;

    for (row = 0; row < 256; row++)
      {
	  r[row] = -1;
	  g[row] = -1;
	  b[row] = -1;
      }
    palette = build_palette (rgb, r, g, b);
    tiff = rl2_create_tiff_destination (path, 1024, 1024, RL2_SAMPLE_UINT8,
					RL2_PIXEL_PALETTE, 1, palette,
					compression, tiled, tile_size);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create TIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_PALETTE)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }
    if (rl2_get_tiff_destination_compression (tiff, &xcompression) != RL2_OK)
      {
	  fprintf (stderr, "Get destination compression error\n");
	  goto error;
      }
    if (xcompression != compression)
      {
	  fprintf (stderr, "Unexpected compression %02x\n", xcompression);
	  goto error;
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      palette2 = build_palette (NULL, r, g, b);
		      bufpix_size = tile_size * tile_size;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					p_in += 2;
					continue;
				    }
				  red = *p_in++;
				  green = *p_in++;
				  blue = *p_in++;
				  rl2_get_palette_index (palette2, &index, red,
							 green, blue);
				  *p_out++ = index;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT8,
						  RL2_PIXEL_PALETTE, 1, bufpix,
						  bufpix_size, palette2, NULL,
						  0, NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		palette2 = build_palette (NULL, r, g, b);
		bufpix_size = 1024;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = rgb + (row * 1024 * 3);
		p_out = bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      red = *p_in++;
		      green = *p_in++;
		      blue = *p_in++;
		      rl2_get_palette_index (palette2, &index, red, green,
					     blue);
		      *p_out++ = index;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE,
					    1, bufpix, bufpix_size, palette2,
					    NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }
/* destroying the TIFF destination */
    rl2_destroy_palette (palette);
    rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return 0;

  error:
    rl2_destroy_palette (palette);
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return -1;
}

static int
do_one_monochrome_test (const unsigned char *rgb, const char *path, int tiled,
			unsigned char compression)
{
/* performing a single Monochrome test */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    unsigned char xcompression;
    if (tiled == 0)
	tile_size = 1;
    tiff = rl2_create_tiff_destination (path, 1024, 1024, RL2_SAMPLE_1_BIT,
					RL2_PIXEL_MONOCHROME, 1, NULL,
					compression, tiled, tile_size);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create TIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_1_BIT)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_MONOCHROME)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }
    if (rl2_get_tiff_destination_compression (tiff, &xcompression) != RL2_OK)
      {
	  fprintf (stderr, "Get destination compression error\n");
	  goto error;
      }
    if (xcompression != compression)
      {
	  fprintf (stderr, "Unexpected compression %02x\n", xcompression);
	  goto error;
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = rgb + ((row + y) * 1024 * 3) + (col * 3);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					p_in += 2;
					continue;
				    }
				  if (*p_in++ < 128)
				      *p_out++ = 1;
				  else
				      *p_out++ = 0;
				  p_in += 2;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_1_BIT,
						  RL2_PIXEL_MONOCHROME, 1,
						  bufpix, bufpix_size, NULL,
						  NULL, 0, NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = rgb + (row * 1024 * 3);
		p_out = bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      if (*p_in++ < 128)
			  *p_out++ = 1;
		      else
			  *p_out++ = 0;
		      p_in += 2;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_1_BIT,
					    RL2_PIXEL_MONOCHROME, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }
/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    return -1;
}

static int
do_one_grid_8_test (const unsigned char *rgb, sqlite3 * handle,
		    const char *path, int tiled, unsigned char compression)
{
/* performing a single Grid Int8 test - GeoTIFF */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const char *p_in;
    char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *tfw_path;
    char *tfw = NULL;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID, 1,
					NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_INT8)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }

    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = (char *) bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = (char *) (rgb + ((row + y) * 1024) + col);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_INT8,
						  RL2_PIXEL_DATAGRID, 1, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = (char *) (rgb + (row * 1024));
		p_out = (char *) bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID,
					    1, bufpix, bufpix_size, NULL, NULL,
					    0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_grid_u8_test (const unsigned char *rgb, sqlite3 * handle,
		     const char *path, int tiled, unsigned char compression)
{
/* performing a single Grid UInt8 test - GeoTIFF */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned char *p_in;
    unsigned char *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *tfw_path;
    char *tfw = NULL;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1,
					NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (alias_pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }

    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size;
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = (unsigned char *) bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in =
				(unsigned char *) (rgb + ((row + y) * 1024) +
						   col);
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT8,
						  RL2_PIXEL_DATAGRID, 1, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024;
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = (unsigned char *) (rgb + (row * 1024));
		p_out = (unsigned char *) bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT8,
					    RL2_PIXEL_DATAGRID, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_grid_16_test (const unsigned char *rgb, sqlite3 * handle,
		     const char *path, int tiled, unsigned char compression)
{
/* performing a single Grid Int16 test - GeoTIFF */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const short *p_in;
    short *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *tfw_path;
    char *tfw = NULL;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_INT16, RL2_PIXEL_DATAGRID, 1,
					NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_INT16)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }

    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size * sizeof (short);
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = (short *) bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = (short *) rgb;
			    p_in += ((row + y) * 1024) + col;
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_INT16,
						  RL2_PIXEL_DATAGRID, 1, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * sizeof (short);
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = (short *) rgb;
		p_in += row * 1024;
		p_out = (short *) bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_INT16,
					    RL2_PIXEL_DATAGRID, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_INT16, RL2_PIXEL_DATAGRID, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_grid_u16_test (const unsigned char *rgb, sqlite3 * handle,
		      const char *path, int tiled, unsigned char compression)
{
/* performing a single Grid UInt16 test - GeoTIFF */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned short *p_in;
    unsigned short *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *tfw_path;
    char *tfw = NULL;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID,
					1, NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_UINT16)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (alias_pixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }

    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size =
			  tile_size * tile_size * sizeof (unsigned short);
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = (unsigned short *) bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = (unsigned short *) rgb;
			    p_in += ((row + y) * 1024) + col;
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT16,
						  RL2_PIXEL_DATAGRID, 1, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * sizeof (unsigned short);
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = (unsigned short *) rgb;
		p_in += row * 1024;
		p_out = (unsigned short *) bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT16,
					    RL2_PIXEL_DATAGRID, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_grid_32_test (const unsigned char *rgb, sqlite3 * handle,
		     const char *path, int tiled, unsigned char compression)
{
/* performing a single Grid Int32 test - GeoTIFF */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const int *p_in;
    int *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *tfw_path;
    char *tfw = NULL;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_INT32, RL2_PIXEL_DATAGRID, 1,
					NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_INT32)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }

    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size * sizeof (int);
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = (int *) bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = (int *) rgb;
			    p_in += ((row + y) * 1024) + col;
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_INT32,
						  RL2_PIXEL_DATAGRID, 1, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * sizeof (int);
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = (int *) rgb;
		p_in += row * 1024;
		p_out = (int *) bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_INT32,
					    RL2_PIXEL_DATAGRID, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_INT32, RL2_PIXEL_DATAGRID, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_grid_u32_test (const unsigned char *rgb, sqlite3 * handle,
		      const char *path, int tiled, unsigned char compression)
{
/* performing a single Grid UInt32 test - GeoTIFF */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const unsigned int *p_in;
    unsigned int *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *tfw_path;
    char *tfw = NULL;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_UINT32, RL2_PIXEL_DATAGRID,
					1, NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_UINT32)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }

    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size =
			  tile_size * tile_size * sizeof (unsigned int);
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = (unsigned int *) bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = (unsigned int *) rgb;
			    p_in += ((row + y) * 1024) + col;
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_UINT32,
						  RL2_PIXEL_DATAGRID, 1, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * sizeof (unsigned int);
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = (unsigned int *) rgb;
		p_in += row * 1024;
		p_out = (unsigned int *) bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_UINT32,
					    RL2_PIXEL_DATAGRID, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_UINT32, RL2_PIXEL_DATAGRID, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_grid_float_test (const unsigned char *rgb, sqlite3 * handle,
			const char *path, int tiled, unsigned char compression)
{
/* performing a single Grid Float test - GeoTIFF */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const float *p_in;
    float *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *tfw_path;
    char *tfw = NULL;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_FLOAT, RL2_PIXEL_DATAGRID, 1,
					NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_FLOAT)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }

    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size * sizeof (float);
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = (float *) bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = (float *) rgb;
			    p_in += ((row + y) * 1024) + col;
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_FLOAT,
						  RL2_PIXEL_DATAGRID, 1, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * sizeof (float);
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = (float *) rgb;
		p_in += row * 1024;
		p_out = (float *) bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_FLOAT,
					    RL2_PIXEL_DATAGRID, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_FLOAT, RL2_PIXEL_DATAGRID, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_one_grid_double_test (const unsigned char *rgb, sqlite3 * handle,
			 const char *path, int tiled, unsigned char compression)
{
/* performing a single Grid Double test - GeoTIFF */
    int row;
    int col;
    int x;
    int y;
    unsigned char *bufpix;
    int bufpix_size;
    const double *p_in;
    double *p_out;
    rl2TiffDestinationPtr tiff = NULL;
    rl2RasterPtr raster;
    int tile_size = 128;
    const char *tfw_path;
    char *tfw;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char alias_pixel_type;
    unsigned char xnum_bands;
    if (tiled == 0)
	tile_size = 1;

    tiff =
	rl2_create_geotiff_destination (path, handle, 1024, 1024,
					RL2_SAMPLE_DOUBLE, RL2_PIXEL_DATAGRID,
					1, NULL, compression, tiled, tile_size,
					32632, 100000.0, 1000000.0, 101024.0,
					1001024.0, 1.0, 1.0, 1);
    if (tiff == NULL)
      {
	  fprintf (stderr, "Unable to create GeoTIFF \"%s\"\n", path);
	  goto error;
      }

    if (rl2_get_tiff_destination_type
	(tiff, &xsample_type, &xpixel_type, &alias_pixel_type,
	 &xnum_bands) != RL2_OK)
      {
	  fprintf (stderr, "Get destination type error\n");
	  goto error;
      }
    if (xsample_type != RL2_SAMPLE_DOUBLE)
      {
	  fprintf (stderr, "Unexpected Sample Type %02x\n", xsample_type);
	  goto error;
      }
    if (xpixel_type != RL2_PIXEL_DATAGRID)
      {
	  fprintf (stderr, "Unexpected Pixel Type %02x\n", xpixel_type);
	  goto error;
      }
    if (xnum_bands != 1)
      {
	  fprintf (stderr, "Unexpected num_bands %d\n", xnum_bands);
	  goto error;
      }

    tfw_path = rl2_get_tiff_destination_worldfile_path (tiff);
    if (tfw_path == NULL)
      {
	  fprintf (stderr, "Unexpected NULL TFW Path\n");
	  goto error;
      }

    if (tfw_path != NULL)
      {
	  int len = strlen (tfw_path);
	  tfw = malloc (len + 1);
	  strcpy (tfw, tfw_path);
      }

    if (tiled)
      {
	  /* Tiled TIFF */
	  for (row = 0; row < 1024; row += tile_size)
	    {
		for (col = 0; col < 1024; col += tile_size)
		  {
		      /* inserting a TIFF tile */
		      bufpix_size = tile_size * tile_size * sizeof (double);
		      bufpix = (unsigned char *) malloc (bufpix_size);
		      p_out = (double *) bufpix;
		      for (y = 0; y < tile_size; y++)
			{
			    if (row + y >= 1024)
			      {
				  for (x = 0; x < tile_size; x++)
				      *p_out++ = 0;
			      }
			    p_in = (double *) rgb;
			    p_in += ((row + y) * 1024) + col;
			    for (x = 0; x < tile_size; x++)
			      {
				  if (col + x >= 1024)
				    {
					*p_out++ = 0;
					continue;
				    }
				  *p_out++ = *p_in++;
			      }
			}
		      raster = rl2_create_raster (tile_size, tile_size,
						  RL2_SAMPLE_DOUBLE,
						  RL2_PIXEL_DATAGRID, 1, bufpix,
						  bufpix_size, NULL, NULL, 0,
						  NULL);
		      if (raster == NULL)
			{
			    fprintf (stderr,
				     "Unable to encode a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      if (rl2_write_tiff_tile (tiff, raster, row, col) !=
			  RL2_OK)
			{
			    rl2_destroy_raster (raster);
			    fprintf (stderr,
				     "Unable to write a tile \"%s\" row=%d col=%d\n",
				     path, row, col);
			    goto error;
			}
		      rl2_destroy_raster (raster);
		  }
	    }
      }
    else
      {
	  /* Striped TIFF */
	  for (row = 0; row < 1024; row++)
	    {
		/* inserting a TIFF strip */
		bufpix_size = 1024 * sizeof (double);
		bufpix = (unsigned char *) malloc (bufpix_size);
		p_in = (double *) rgb;
		p_in += row * 1024;
		p_out = (double *) bufpix;
		for (x = 0; x < 1024; x++)
		  {
		      /* feeding the scanline buffer */
		      *p_out++ = *p_in++;
		  }
		raster = rl2_create_raster (1024, 1,
					    RL2_SAMPLE_DOUBLE,
					    RL2_PIXEL_DATAGRID, 1, bufpix,
					    bufpix_size, NULL, NULL, 0, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr,
			       "Unable to encode a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		if (rl2_write_tiff_scanline (tiff, raster, row) != RL2_OK)
		  {
		      rl2_destroy_raster (raster);
		      fprintf (stderr,
			       "Unable to write a scanline \"%s\" row=%d\n",
			       path, row);
		      goto error;
		  }
		rl2_destroy_raster (raster);
	    }
      }

/* writing a TFW */
    if (rl2_write_tiff_worldfile (tiff) != RL2_OK)
      {
	  fprintf (stderr, "Unable to write a TFW\n");
	  goto error;
      }

/* destroying the TIFF destination */
    rl2_destroy_tiff_destination (tiff);
    tiff = NULL;
    if (!check_origin
	(path, tfw, 32632, 100000.0, 1000000.0, 101024.0, 1001024.0, 1.0, 1.0,
	 compression, RL2_SAMPLE_DOUBLE, RL2_PIXEL_DATAGRID, RL2_TIFF_GEOTIFF))
	goto error;
    if (tfw != NULL)
	free (tfw);
    return 0;

  error:
    if (tiff != NULL)
	rl2_destroy_tiff_destination (tiff);
    unlink (path);
    if (tfw != NULL)
      {
	  unlink (tfw);
	  free (tfw);
      }
    return -1;
}

static int
do_test_rgb (const unsigned char *rgb)
{
/* testing RGB flavours */
    int ret =
	do_one_rgb_test (rgb, "./rgb_strip_none.tif", 0, RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret = do_one_rgb_test (rgb, "./rgb_strip_lzw.tif", 0, RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test (rgb, "./rgb_strip_deflate.tif", 0,
			 RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test (rgb, "./rgb_strip_jpeg.tif", 0, RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    ret = do_one_rgb_test (rgb, "./rgb_tile_none.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret = do_one_rgb_test (rgb, "./rgb_tile_lzw.tif", 1, RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test (rgb, "./rgb_tile_deflate.tif", 1,
			 RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret = do_one_rgb_test (rgb, "./rgb_tile_jpeg.tif", 1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_rgb_geotiff1 (const unsigned char *rgb, sqlite3 * handle)
{
/* testing RGB flavours - GeoTIFF (1) */
    int ret =
	do_one_rgb_test_geotiff1 (rgb, handle, "./rgb_strip_none_geotiff1.tif",
				  0, RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff1 (rgb, handle, "./rgb_strip_lzw_geotiff1.tif",
				  0, RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff1 (rgb, handle,
				  "./rgb_strip_deflate_geotiff1.tif", 0,
				  RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff1 (rgb, handle, "./rgb_strip_jpeg_geotiff1.tif",
				  0, RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff1 (rgb, handle, "./rgb_tile_none_geotiff1.tif",
				  1, RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff1 (rgb, handle, "./rgb_tile_lzw_geotiff1.tif", 1,
				  RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff1 (rgb, handle,
				  "./rgb_tile_deflate_geotiff1.tif", 1,
				  RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff1 (rgb, handle, "./rgb_tile_jpeg_geotiff1.tif",
				  1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_rgb_geotiff2 (const unsigned char *rgb, sqlite3 * handle)
{
/* testing RGB flavours - GeoTIFF (2) */
    int ret =
	do_one_rgb_test_geotiff2 (rgb, handle, "./rgb_strip_none_geotiff2.tif",
				  0, RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff2 (rgb, handle, "./rgb_strip_lzw_geotiff2.tif",
				  0, RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff2 (rgb, handle,
				  "./rgb_strip_deflate_geotiff2.tif", 0,
				  RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff2 (rgb, handle, "./rgb_strip_jpeg_geotiff2.tif",
				  0, RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff2 (rgb, handle, "./rgb_tile_none_geotiff2.tif",
				  1, RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff2 (rgb, handle, "./rgb_tile_lzw_geotiff2.tif", 1,
				  RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff2 (rgb, handle,
				  "./rgb_tile_deflate_geotiff2.tif", 1,
				  RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_geotiff2 (rgb, handle, "./rgb_tile_jpeg_geotiff2.tif",
				  1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_rgb_tfw (const unsigned char *rgb)
{
/* testing RGB flavours - TFW */
    int ret = do_one_rgb_test_tfw (rgb, "./rgb_strip_none_tfw.tif", 0,
				   RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_tfw (rgb, "./rgb_strip_lzw_tfw.tif", 0,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_tfw (rgb, "./rgb_strip_deflate_tfw.tif", 0,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_tfw (rgb, "./rgb_strip_jpeg_tfw.tif", 0,
			     RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_tfw (rgb, "./rgb_tile_none_tfw.tif", 1,
			     RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_tfw (rgb, "./rgb_tile_lzw_tfw.tif", 1,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_tfw (rgb, "./rgb_tile_deflate_tfw.tif", 1,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_rgb_test_tfw (rgb, "./rgb_tile_jpeg_tfw.tif", 1,
			     RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grayband (const unsigned char *rgb)
{
/* testing GrayBand flavours */
    int ret = do_one_gray_test (rgb, "./gray_strip_none.tif", 0,
				RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_gray_test (rgb, "./gray_strip_lzw.tif", 0, RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_gray_test (rgb, "./gray_strip_deflate.tif", 0,
			  RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_gray_test (rgb, "./gray_strip_jpeg.tif", 0,
			  RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    ret =
	do_one_gray_test (rgb, "./gray_tile_none.tif", 1, RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret = do_one_gray_test (rgb, "./gray_tile_lzw.tif", 1, RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_gray_test (rgb, "./gray_tile_deflate.tif", 1,
			  RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_gray_test (rgb, "./gray_tile_jpeg.tif", 1, RL2_COMPRESSION_JPEG);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_palette (const unsigned char *rgb)
{
/* testing Palette flavours */
    int ret = do_one_palette_test (rgb, "./palette_strip_none.tif", 0,
				   RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_palette_test (rgb, "./palette_strip_lzw.tif", 0,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_palette_test (rgb, "./palette_strip_deflate.tif", 0,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_palette_test (rgb, "./palette_tile_none.tif", 1,
			     RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_palette_test (rgb, "./palette_tile_lzw.tif", 1,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_palette_test (rgb, "./palette_tile_deflate.tif", 1,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_monochrome (const unsigned char *rgb)
{
/* testing Monochrome flavours */
    int ret = do_one_monochrome_test (rgb, "./monochrome_strip_none.tif", 0,
				      RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_monochrome_test (rgb, "./monochrome_strip_fax3.tif", 0,
				RL2_COMPRESSION_CCITTFAX3);
    if (ret < 0)
	return ret;
    ret =
	do_one_monochrome_test (rgb, "./monochrome_strip_fax4.tif", 0,
				RL2_COMPRESSION_CCITTFAX4);
    if (ret < 0)
	return ret;
    ret =
	do_one_monochrome_test (rgb, "./monochrome_tile_none.tif", 1,
				RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_monochrome_test (rgb, "./monochrome_tile_fax3.tif", 1,
				RL2_COMPRESSION_CCITTFAX3);
    if (ret < 0)
	return ret;
    ret =
	do_one_monochrome_test (rgb, "./monochrome_tile_fax4.tif", 1,
				RL2_COMPRESSION_CCITTFAX4);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grid_8 (const unsigned char *rgb, sqlite3 * handle)
{
/* testing "grid int8" flavours */
    int ret = do_one_grid_8_test (rgb, handle, "./grid_8_strip_none.tif", 0,
				  RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_8_test (rgb, handle, "./grid_8_strip_lzw.tif", 0,
			    RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_8_test (rgb, handle, "./grid_8_strip_deflate.tif", 0,
			    RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_8_test (rgb, handle, "./grid_8_tile_none.tif", 1,
			    RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_8_test (rgb, handle, "./grid_8_tile_lzw.tif", 1,
			    RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_8_test (rgb, handle, "./grid_8_tile_deflate.tif", 1,
			    RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grid_u8 (const unsigned char *rgb, sqlite3 * handle)
{
/* testing "grid uint8" flavours */
    int ret = do_one_grid_u8_test (rgb, handle, "./grid_u8_strip_none.tif", 0,
				   RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u8_test (rgb, handle, "./grid_u8_strip_lzw.tif", 0,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u8_test (rgb, handle, "./grid_u8_strip_deflate.tif", 0,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u8_test (rgb, handle, "./grid_u8_tile_none.tif", 1,
			     RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u8_test (rgb, handle, "./grid_u8_tile_lzw.tif", 1,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u8_test (rgb, handle, "./grid_u8_tile_deflate.tif", 1,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grid_16 (const unsigned char *rgb, sqlite3 * handle)
{
/* testing "grid int16" flavours */
    int ret = do_one_grid_16_test (rgb, handle, "./grid_16_strip_none.tif", 0,
				   RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_16_test (rgb, handle, "./grid_16_strip_lzw.tif", 0,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_16_test (rgb, handle, "./grid_16_strip_deflate.tif", 0,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_16_test (rgb, handle, "./grid_16_tile_none.tif", 1,
			     RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_16_test (rgb, handle, "./grid_16_tile_lzw.tif", 1,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_16_test (rgb, handle, "./grid_16_tile_deflate.tif", 1,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grid_u16 (const unsigned char *rgb, sqlite3 * handle)
{
/* testing "grid uint16" flavours */
    int ret = do_one_grid_u16_test (rgb, handle, "./grid_u16_strip_none.tif", 0,
				    RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u16_test (rgb, handle, "./grid_u16_strip_lzw.tif", 0,
			      RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u16_test (rgb, handle, "./grid_u16_strip_deflate.tif", 0,
			      RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u16_test (rgb, handle, "./grid_u16_tile_none.tif", 1,
			      RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u16_test (rgb, handle, "./grid_u16_tile_lzw.tif", 1,
			      RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u16_test (rgb, handle, "./grid_u16_tile_deflate.tif", 1,
			      RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grid_32 (const unsigned char *rgb, sqlite3 * handle)
{
/* testing "grid int32" flavours */
    int ret = do_one_grid_32_test (rgb, handle, "./grid_32_strip_none.tif", 0,
				   RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_32_test (rgb, handle, "./grid_32_strip_lzw.tif", 0,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_32_test (rgb, handle, "./grid_32_strip_deflate.tif", 0,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_32_test (rgb, handle, "./grid_32_tile_none.tif", 1,
			     RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_32_test (rgb, handle, "./grid_32_tile_lzw.tif", 1,
			     RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_32_test (rgb, handle, "./grid_32_tile_deflate.tif", 1,
			     RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grid_u32 (const unsigned char *rgb, sqlite3 * handle)
{
/* testing "grid uint32" flavours */
    int ret = do_one_grid_u32_test (rgb, handle, "./grid_u32_strip_none.tif", 0,
				    RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u32_test (rgb, handle, "./grid_u32_strip_lzw.tif", 0,
			      RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u32_test (rgb, handle, "./grid_u32_strip_deflate.tif", 0,
			      RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u32_test (rgb, handle, "./grid_u32_tile_none.tif", 1,
			      RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u32_test (rgb, handle, "./grid_u32_tile_lzw.tif", 1,
			      RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_u32_test (rgb, handle, "./grid_u32_tile_deflate.tif", 1,
			      RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grid_float (const unsigned char *rgb, sqlite3 * handle)
{
/* testing "grid float" flavours */
    int ret =
	do_one_grid_float_test (rgb, handle, "./grid_float_strip_none.tif", 0,
				RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_float_test (rgb, handle, "./grid_float_strip_lzw.tif", 0,
				RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_float_test (rgb, handle, "./grid_float_strip_deflate.tif",
				0, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_float_test (rgb, handle, "./grid_float_tile_none.tif", 1,
				RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_float_test (rgb, handle, "./grid_float_tile_lzw.tif", 1,
				RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_float_test (rgb, handle, "./grid_float_tile_deflate.tif", 1,
				RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
do_test_grid_double (const unsigned char *rgb, sqlite3 * handle)
{
/* testing "grid double" flavours */
    int ret =
	do_one_grid_double_test (rgb, handle, "./grid_double_strip_none.tif", 0,
				 RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_double_test (rgb, handle, "./grid_double_strip_lzw.tif", 0,
				 RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_double_test (rgb, handle, "./grid_double_strip_deflate.tif",
				 0, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_double_test (rgb, handle, "./grid_double_tile_none.tif", 1,
				 RL2_COMPRESSION_NONE);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_double_test (rgb, handle, "./grid_double_tile_lzw.tif", 1,
				 RL2_COMPRESSION_LZW);
    if (ret < 0)
	return ret;
    ret =
	do_one_grid_double_test (rgb, handle, "./grid_double_tile_deflate.tif",
				 1, RL2_COMPRESSION_DEFLATE);
    if (ret < 0)
	return ret;
    return 0;
}

static int
test_null (sqlite3 * handle)
{
/* testing null/invalid arguments */
    rl2TiffDestinationPtr destination;
    rl2PalettePtr palette;
    unsigned int width;
    unsigned int height;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char alias_pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned int strip_size;
    int is_tiled;
    int srid;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double hResolution;
    double vResolution;
    int is_geotiff;
    int is_worldfile;

    destination =
	rl2_create_tiff_destination (NULL, 1024, 1024, RL2_SAMPLE_UINT8,
				     RL2_PIXEL_RGB, 3, NULL,
				     RL2_COMPRESSION_NONE, 0, 0);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create TIFF Destination - NULL path\n");
	  return -30;
      }
    destination =
	rl2_create_geotiff_destination (NULL, handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					NULL, RL2_COMPRESSION_NONE, 0, 0, 4326,
					1000.0, 1000.0, 2000.0, 2000.0, 1.0,
					1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - NULL path\n");
	  return -31;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", NULL, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					NULL, RL2_COMPRESSION_NONE, 0, 0, 4326,
					1000.0, 1000.0, 2000.0, 2000.0, 1.0,
					1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - NULL SQLite handle\n");
	  return -32;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT32, RL2_PIXEL_RGB, 3,
					NULL, RL2_COMPRESSION_NONE, 0, 0, 4326,
					1000.0, 1000.0, 2000.0, 2000.0, 1.0,
					1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid RGB (1)\n");
	  return -33;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 2,
					NULL, RL2_COMPRESSION_NONE, 0, 0, 4326,
					1000.0, 1000.0, 2000.0, 2000.0, 1.0,
					1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid RGB (2)\n");
	  return -34;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
					NULL, RL2_COMPRESSION_CCITTFAX3, 0, 0,
					4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid RGB (3)\n");
	  return -35;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT32, RL2_PIXEL_GRAYSCALE,
					1, NULL, RL2_COMPRESSION_NONE, 0, 0,
					4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Grayscale (1)\n");
	  return -36;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE,
					2, NULL, RL2_COMPRESSION_NONE, 0, 0,
					4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Grayscale (2)\n");
	  return -37;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE,
					1, NULL, RL2_COMPRESSION_CCITTFAX3, 0,
					0, 4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Grayscale (3)\n");
	  return -38;
      }
    palette = rl2_create_palette (256);
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_INT8, RL2_PIXEL_PALETTE, 1,
					palette, RL2_COMPRESSION_NONE, 0, 0,
					4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Palette (1)\n");
	  return -39;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 2,
					palette, RL2_COMPRESSION_NONE, 0, 0,
					4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Palette (2)\n");
	  return -40;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1,
					palette, RL2_COMPRESSION_CCITTFAX3, 0,
					0, 4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Palette (3)\n");
	  return -41;
      }
    rl2_destroy_palette (palette);
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1,
					NULL, RL2_COMPRESSION_NONE, 0, 0, 4326,
					1000.0, 1000.0, 2000.0, 2000.0, 1.0,
					1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Palette (4)\n");
	  return -42;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_MONOCHROME,
					1, NULL, RL2_COMPRESSION_NONE, 0, 0,
					4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Monochrome (1)\n");
	  return -43;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME,
					2, NULL, RL2_COMPRESSION_NONE, 0, 0,
					4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Monochrome (2)\n");
	  return -44;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME,
					1, NULL, RL2_COMPRESSION_JPEG, 0, 0,
					4326, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Monochrome (3)\n");
	  return -45;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_1_BIT, RL2_PIXEL_DATAGRID, 1,
					NULL, RL2_COMPRESSION_NONE, 0, 0, 4326,
					1000.0, 1000.0, 2000.0, 2000.0, 1.0,
					1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Grid (1)\n");
	  return -46;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 2,
					NULL, RL2_COMPRESSION_NONE, 0, 0, 4326,
					1000.0, 1000.0, 2000.0, 2000.0, 1.0,
					1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Grid (2)\n");
	  return -47;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1,
					NULL, RL2_COMPRESSION_JPEG, 0, 0, 4326,
					1000.0, 1000.0, 2000.0, 2000.0, 1.0,
					1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid Grid (3)\n");
	  return -48;
      }
    destination =
	rl2_create_geotiff_destination ("test.tif", handle, 1024, 1024,
					RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1,
					NULL, RL2_COMPRESSION_NONE, 0, 0,
					999999, 1000.0, 1000.0, 2000.0, 2000.0,
					1.0, 1.0, 1);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - invalid SRID\n");
	  return -49;
      }
    destination =
	rl2_create_tiff_worldfile_destination (NULL, 1024, 1024,
					       RL2_SAMPLE_UINT8, RL2_PIXEL_RGB,
					       3, NULL, RL2_COMPRESSION_NONE, 0,
					       0, 4326, 1000.0, 1000.0, 2000.0,
					       2000.0, 1.0, 1.0);
    if (destination != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: Create GeoTIFF Destination - NULL path\n");
	  return -50;
      }
    destination =
	rl2_create_tiff_destination ("test-null.tif", 1024, 1024,
				     RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3, NULL,
				     RL2_COMPRESSION_NONE, 0, 0);
    if (destination == NULL)
      {
	  fprintf (stderr,
		   "Unexpected fail: Create TIFF destination - no georef striped\n");
	  return -51;
      }
    if (rl2_get_tiff_destination_srid (destination, &srid) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Get SRID - no georef\n");
	  return -52;
      }
    if (rl2_get_tiff_destination_extent
	(destination, &minX, &minY, &maxX, &maxY) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Get Extent - no georef\n");
	  return -53;
      }
    if (rl2_get_tiff_destination_resolution
	(destination, &hResolution, &vResolution) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Resolution - no georef\n");
	  return -54;
      }
    if (rl2_get_tiff_destination_worldfile_path (destination) != NULL)
      {
	  fprintf (stderr, "Unexpected success: TFW Path (not existing)\n");
	  return -55;
      }
    if (rl2_get_tiff_destination_tile_size
	(destination, &tile_width, &tile_height) == RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Tile Size\n");
	  return -56;
      }
    rl2_destroy_tiff_destination (destination);
    unlink ("test-null.tif");
    destination =
	rl2_create_tiff_destination ("test-null.tif", 1024, 1024,
				     RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3, NULL,
				     RL2_COMPRESSION_NONE, 1, 256);
    if (destination == NULL)
      {
	  fprintf (stderr,
		   "Unexpected fail: Create TIFF destination - no georef tiled\n");
	  return -57;
      }
    if (rl2_get_tiff_destination_strip_size (destination, &strip_size) ==
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected success: TIFF Strip Size\n");
	  return -58;
      }
    rl2_destroy_tiff_destination (destination);
    unlink ("test-null.tif");

    if (rl2_get_tiff_destination_worldfile_path (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected success: TFW Path (NULL destination)\n");
	  return -59;
      }
    if (rl2_get_tiff_destination_path (NULL) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Path (NULL destination)\n");
	  return -60;
      }
    if (rl2_get_tiff_destination_size (NULL, &width, &height) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Size (NULL destination)\n");
	  return -61;
      }
    if (rl2_get_tiff_destination_type
	(NULL, &sample_type, &pixel_type, &alias_pixel_type,
	 &num_bands) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Type (NULL destination)\n");
	  return -62;
      }
    if (rl2_get_tiff_destination_compression (NULL, &compression) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Compression (NULL destination)\n");
	  return -63;
      }
    if (rl2_is_tiled_tiff_destination (NULL, &is_tiled) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF IsTiled - NULL destination\n");
	  return -64;
      }
    if (rl2_get_tiff_destination_tile_size (NULL, &tile_width, &tile_height) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Tile Size - NULL destination\n");
	  return -65;
      }
    if (rl2_get_tiff_destination_strip_size (NULL, &strip_size) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Strip Size - NULL destination\n");
	  return -66;
      }
    if (rl2_get_tiff_destination_srid (NULL, &srid) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get SRID - NULL destination\n");
	  return -67;
      }
    if (rl2_get_tiff_destination_extent (NULL, &minX, &minY, &maxX, &maxY) ==
	RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Extent - NULL destination\n");
	  return -68;
      }
    if (rl2_get_tiff_destination_resolution (NULL, &hResolution, &vResolution)
	== RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Get Resolution - NULL destination\n");
	  return -69;
      }
    if (rl2_is_geotiff_destination (NULL, &is_geotiff) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF IsGeoTIFF - NULL destination\n");
	  return -70;
      }
    if (rl2_is_tiff_worldfile_destination (NULL, &is_worldfile) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF IsWorldfile - NULL destination\n");
	  return -71;
      }
    if (rl2_write_tiff_worldfile (NULL) == RL2_OK)
      {
	  fprintf (stderr,
		   "Unexpected success: TIFF Write Worldfile - NULL destination\n");
	  return -72;
      }
    unlink ("test.tif");

    return 0;
}

int
main (int argc, char *argv[])
{
    int ret;
    unsigned char *rgb;
    sqlite3 *handle = NULL;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory db: %s\n",
		   sqlite3_errmsg (handle));
	  return -2;
      }
    spatialite_init_ex (handle, cache, 0);

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata()", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -3;
      }

/* testing an RGB image */
    rgb = create_rainbow ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: rainbow\n");
	  return -1;
      }
/* testing RGB flavours */
    ret = do_test_rgb (rgb);
    if (ret < 0)
	return ret;
/* testing RGB flavours - GeoTIFF (1) */
    ret = do_test_rgb_geotiff1 (rgb, handle);
    if (ret < 0)
	return ret;
/* testing RGB flavours - GeoTIFF (3) */
    ret = do_test_rgb_geotiff2 (rgb, handle);
    if (ret < 0)
	return ret;
/* testing RGB flavours - TIFF+TFW */
    ret = do_test_rgb_tfw (rgb);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing GrayBand image */
    rgb = create_grayband ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: grayband\n");
	  return -1;
      }
/* testing GrayBand flavours */
    ret = do_test_grayband (rgb);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing Palette image */
    rgb = create_palette ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: palette\n");
	  return -1;
      }
/* testing Palete flavours */
    ret = do_test_palette (rgb);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing Monochrome image */
    rgb = create_monochrome ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: monochrome\n");
	  return -1;
      }
/* testing Monochrome flavours */
    ret = do_test_monochrome (rgb);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing INT8 Grid */
    rgb = create_grid_8 ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: GridInt8\n");
	  return -1;
      }
/* testing GRID INT8 flavours */
    ret = do_test_grid_8 (rgb, handle);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing UINT8 Grid */
    rgb = create_grid_u8 ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: GridUInt8\n");
	  return -1;
      }
/* testing GRID UINT8 flavours */
    ret = do_test_grid_u8 (rgb, handle);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing INT16 Grid */
    rgb = create_grid_16 ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: GridInt16\n");
	  return -1;
      }
/* testing GRID INT16 flavours */
    ret = do_test_grid_16 (rgb, handle);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing UINT16 Grid */
    rgb = create_grid_u16 ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: GridUInt16\n");
	  return -1;
      }
/* testing GRID UINT16 flavours */
    ret = do_test_grid_u16 (rgb, handle);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing INT32 Grid */
    rgb = create_grid_32 ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: GridInt32\n");
	  return -1;
      }
/* testing GRID INT32 flavours */
    ret = do_test_grid_32 (rgb, handle);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing UINT16 Grid */
    rgb = create_grid_u32 ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: GridUInt32\n");
	  return -1;
      }
/* testing GRID UINT16 flavours */
    ret = do_test_grid_u32 (rgb, handle);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing FLOAT Grid */
    rgb = create_grid_float ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: GridFloat\n");
	  return -1;
      }
/* testing GRID FLOAT flavours */
    ret = do_test_grid_float (rgb, handle);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing DOUBLE Grid */
    rgb = create_grid_double ();
    if (rgb == NULL)
      {
	  fprintf (stderr, "Unable to create image: GridDouble\n");
	  return -1;
      }
/* testing GRID DOUBLE flavours */
    ret = do_test_grid_double (rgb, handle);
/* freeing the RGB buffer */
    free (rgb);
    if (ret < 0)
	return ret;

/* testing null/invalid args */
    ret = test_null (handle);
    if (ret < 0)
	return ret;

    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);

    spatialite_shutdown ();
    return 0;
}
