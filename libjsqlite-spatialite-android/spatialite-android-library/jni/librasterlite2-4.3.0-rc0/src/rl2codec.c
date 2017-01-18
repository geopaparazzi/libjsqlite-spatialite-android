/*

 rl2codec -- encoding/decoding functions

 version 0.1, 2013 April 1

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

#include <zlib.h>
#include <lzma.h>

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2tiff.h"
#include "rasterlite2_private.h"

static int
endianArch ()
{
/* checking if target CPU is a little-endian one */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 0;
    return 1;
}

static void
exportU16 (unsigned char *p, unsigned short value, int little_endian,
	   int little_endian_arch)
{
/* stores a 16bit int into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[2];
	unsigned short int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 1) = convert.byte[0];
		*(p + 0) = convert.byte[1];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 1) = convert.byte[0];
		*(p + 0) = convert.byte[1];
	    }
      }
}

static short
import16 (const unsigned char *p, int little_endian, int little_endian_arch)
{
/* fetches a 16bit int from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[2];
	short int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 1);
		convert.byte[1] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 1);
		convert.byte[1] = *(p + 0);
	    }
      }
    return convert.int_value;
}

static void
export16 (unsigned char *p, short value, int little_endian,
	  int little_endian_arch)
{
/* stores a 16bit int into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[2];
	short int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 1) = convert.byte[0];
		*(p + 0) = convert.byte[1];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 1) = convert.byte[0];
		*(p + 0) = convert.byte[1];
	    }
      }
}

static unsigned short
importU16 (const unsigned char *p, int little_endian, int little_endian_arch)
{
/* fetches a 16bit uint from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[2];
	unsigned short int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 1);
		convert.byte[1] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 1);
		convert.byte[1] = *(p + 0);
	    }
      }
    return convert.int_value;
}

static void
exportU32 (unsigned char *p, unsigned int value, int little_endian,
	   int little_endian_arch)
{
/* stores a 32bit int into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	unsigned int int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
      }
}

static unsigned int
importU32 (const unsigned char *p, int little_endian, int little_endian_arch)
{
/* fetches a 32bit uint from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	unsigned int int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
      }
    return convert.int_value;
}

static void
export32 (unsigned char *p, int value, int little_endian,
	  int little_endian_arch)
{
/* stores a 32bit int into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
      }
}

static int
import32 (const unsigned char *p, int little_endian, int little_endian_arch)
{
/* fetches a 32bit int from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
      }
    return convert.int_value;
}

static void
exportFloat (unsigned char *p, float value, int little_endian,
	     int little_endian_arch)
{
/* stores a 32bit Float into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	float flt_value;
    } convert;
    convert.flt_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 3) = convert.byte[0];
		*(p + 2) = convert.byte[1];
		*(p + 1) = convert.byte[2];
		*(p + 0) = convert.byte[3];
	    }
      }
}

static float
importFloat (const unsigned char *p, int little_endian, int little_endian_arch)
{
/* fetches a 32bit Float from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	float flt_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 3);
		convert.byte[1] = *(p + 2);
		convert.byte[2] = *(p + 1);
		convert.byte[3] = *(p + 0);
	    }
      }
    return convert.flt_value;
}

static void
exportDouble (unsigned char *p, double value, int little_endian,
	      int little_endian_arch)
{
/* stores a Double into a BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[8];
	double dbl_value;
    } convert;
    convert.dbl_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 7) = convert.byte[0];
		*(p + 6) = convert.byte[1];
		*(p + 5) = convert.byte[2];
		*(p + 4) = convert.byte[3];
		*(p + 3) = convert.byte[4];
		*(p + 2) = convert.byte[5];
		*(p + 1) = convert.byte[6];
		*(p + 0) = convert.byte[7];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
		*(p + 4) = convert.byte[4];
		*(p + 5) = convert.byte[5];
		*(p + 6) = convert.byte[6];
		*(p + 7) = convert.byte[7];
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		*(p + 0) = convert.byte[0];
		*(p + 1) = convert.byte[1];
		*(p + 2) = convert.byte[2];
		*(p + 3) = convert.byte[3];
		*(p + 4) = convert.byte[4];
		*(p + 5) = convert.byte[5];
		*(p + 6) = convert.byte[6];
		*(p + 7) = convert.byte[7];
	    }
	  else
	    {
		/* Little Endian data */
		*(p + 7) = convert.byte[0];
		*(p + 6) = convert.byte[1];
		*(p + 5) = convert.byte[2];
		*(p + 4) = convert.byte[3];
		*(p + 3) = convert.byte[4];
		*(p + 2) = convert.byte[5];
		*(p + 1) = convert.byte[6];
		*(p + 0) = convert.byte[7];
	    }
      }
}

static double
importDouble (const unsigned char *p, int little_endian, int little_endian_arch)
{
/* fetches a Double from BLOB respecting declared endiannes */
    union cvt
    {
	unsigned char byte[8];
	double dbl_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 7);
		convert.byte[1] = *(p + 6);
		convert.byte[2] = *(p + 5);
		convert.byte[3] = *(p + 4);
		convert.byte[4] = *(p + 3);
		convert.byte[5] = *(p + 2);
		convert.byte[6] = *(p + 1);
		convert.byte[7] = *(p + 0);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
		convert.byte[4] = *(p + 4);
		convert.byte[5] = *(p + 5);
		convert.byte[6] = *(p + 6);
		convert.byte[7] = *(p + 7);
	    }
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  if (!little_endian)
	    {
		/* Big Endian data */
		convert.byte[0] = *(p + 0);
		convert.byte[1] = *(p + 1);
		convert.byte[2] = *(p + 2);
		convert.byte[3] = *(p + 3);
		convert.byte[4] = *(p + 4);
		convert.byte[5] = *(p + 5);
		convert.byte[6] = *(p + 6);
		convert.byte[7] = *(p + 7);
	    }
	  else
	    {
		/* Little Endian data */
		convert.byte[0] = *(p + 7);
		convert.byte[1] = *(p + 6);
		convert.byte[2] = *(p + 5);
		convert.byte[3] = *(p + 4);
		convert.byte[4] = *(p + 3);
		convert.byte[5] = *(p + 2);
		convert.byte[6] = *(p + 1);
		convert.byte[7] = *(p + 0);
	    }
      }
    return convert.dbl_value;
}

static short
swapINT16 (short value)
{
/* swaps a INT16 respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	short value;
    } convert1;
    union cvt convert2;
    convert1.value = value;
    convert2.byte[0] = convert1.byte[1];
    convert2.byte[1] = convert1.byte[0];
    return convert2.value;
}

static unsigned short
swapUINT16 (unsigned short value)
{
/* swaps a UINT16 respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	unsigned short value;
    } convert1;
    union cvt convert2;
    convert1.value = value;
    convert2.byte[0] = convert1.byte[1];
    convert2.byte[1] = convert1.byte[0];
    return convert2.value;
}

static int
swapINT32 (int value)
{
/* swaps an INT32 respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	int value;
    } convert1;
    union cvt convert2;
    convert1.value = value;
    convert2.byte[0] = convert1.byte[3];
    convert2.byte[1] = convert1.byte[2];
    convert2.byte[2] = convert1.byte[1];
    convert2.byte[3] = convert1.byte[0];
    return convert2.value;
}

static unsigned int
swapUINT32 (unsigned int value)
{
/* swaps a UINT32 respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	unsigned int value;
    } convert1;
    union cvt convert2;
    convert1.value = value;
    convert2.byte[0] = convert1.byte[3];
    convert2.byte[1] = convert1.byte[2];
    convert2.byte[2] = convert1.byte[1];
    convert2.byte[3] = convert1.byte[0];
    return convert2.value;
}

static float
swapFloat (float value)
{
/* swaps a Float respecting declared endiannes */
    union cvt
    {
	unsigned char byte[4];
	float value;
    } convert1;
    union cvt convert2;
    convert1.value = value;
    convert2.byte[0] = convert1.byte[3];
    convert2.byte[1] = convert1.byte[2];
    convert2.byte[2] = convert1.byte[1];
    convert2.byte[3] = convert1.byte[0];
    return convert2.value;
}

static double
swapDouble (double value)
{
/* swaps a Double respecting declared endiannes */
    union cvt
    {
	unsigned char byte[8];
	double value;
    } convert1;
    union cvt convert2;
    convert1.value = value;
    convert2.byte[0] = convert1.byte[7];
    convert2.byte[1] = convert1.byte[6];
    convert2.byte[2] = convert1.byte[5];
    convert2.byte[3] = convert1.byte[4];
    convert2.byte[4] = convert1.byte[3];
    convert2.byte[5] = convert1.byte[2];
    convert2.byte[6] = convert1.byte[1];
    convert2.byte[7] = convert1.byte[0];
    return convert2.value;
}

static int
check_encode_self_consistency (unsigned char sample_type,
			       unsigned char pixel_type,
			       unsigned char num_samples,
			       unsigned char compression)
{
/* checking overall self-consistency for coverage params */
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
	  if (sample_type != RL2_SAMPLE_1_BIT || num_samples != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_DEFLATE_NO:
	    case RL2_COMPRESSION_LZMA:
	    case RL2_COMPRESSION_LZMA_NO:
	    case RL2_COMPRESSION_PNG:
	    case RL2_COMPRESSION_CCITTFAX4:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_PALETTE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_DEFLATE_NO:
	    case RL2_COMPRESSION_LZMA:
	    case RL2_COMPRESSION_LZMA_NO:
	    case RL2_COMPRESSION_PNG:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  switch (compression)
	    {
	    case RL2_COMPRESSION_NONE:
	    case RL2_COMPRESSION_DEFLATE:
	    case RL2_COMPRESSION_DEFLATE_NO:
	    case RL2_COMPRESSION_LZMA:
	    case RL2_COMPRESSION_LZMA_NO:
	    case RL2_COMPRESSION_PNG:
	    case RL2_COMPRESSION_JPEG:
	    case RL2_COMPRESSION_LOSSY_WEBP:
	    case RL2_COMPRESSION_LOSSLESS_WEBP:
	    case RL2_COMPRESSION_CHARLS:
	    case RL2_COMPRESSION_LOSSY_JP2:
	    case RL2_COMPRESSION_LOSSLESS_JP2:
		break;
	    default:
		return 0;
	    };
	  break;
      case RL2_PIXEL_RGB:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_UINT16:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 3)
	      return 0;
	  if (sample_type == RL2_SAMPLE_UINT16)
	    {
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		  case RL2_COMPRESSION_DEFLATE:
		  case RL2_COMPRESSION_DEFLATE_NO:
		  case RL2_COMPRESSION_LZMA:
		  case RL2_COMPRESSION_LZMA_NO:
		  case RL2_COMPRESSION_PNG:
		  case RL2_COMPRESSION_CHARLS:
		  case RL2_COMPRESSION_LOSSY_JP2:
		  case RL2_COMPRESSION_LOSSLESS_JP2:
		      break;
		  default:
		      return 0;
		  };
	    }
	  else
	    {
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		  case RL2_COMPRESSION_DEFLATE:
		  case RL2_COMPRESSION_DEFLATE_NO:
		  case RL2_COMPRESSION_LZMA:
		  case RL2_COMPRESSION_LZMA_NO:
		  case RL2_COMPRESSION_PNG:
		  case RL2_COMPRESSION_JPEG:
		  case RL2_COMPRESSION_LOSSY_WEBP:
		  case RL2_COMPRESSION_LOSSLESS_WEBP:
		  case RL2_COMPRESSION_CHARLS:
		  case RL2_COMPRESSION_LOSSY_JP2:
		  case RL2_COMPRESSION_LOSSLESS_JP2:
		      break;
		  default:
		      return 0;
		  };
	    }
	  break;
      case RL2_PIXEL_MULTIBAND:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_UINT16:
		break;
	    default:
		return 0;
	    };
	  if (num_samples < 2)
	      return 0;
	  if (num_samples == 3 || num_samples == 4)
	    {
		if (sample_type == RL2_SAMPLE_UINT16)
		  {
		      switch (compression)
			{
			case RL2_COMPRESSION_NONE:
			case RL2_COMPRESSION_DEFLATE:
			case RL2_COMPRESSION_DEFLATE_NO:
			case RL2_COMPRESSION_LZMA:
			case RL2_COMPRESSION_LZMA_NO:
			case RL2_COMPRESSION_PNG:
			case RL2_COMPRESSION_CHARLS:
			case RL2_COMPRESSION_LOSSY_JP2:
			case RL2_COMPRESSION_LOSSLESS_JP2:
			    break;
			default:
			    return 0;
			};
		  }
		else
		  {
		      switch (compression)
			{
			case RL2_COMPRESSION_NONE:
			case RL2_COMPRESSION_DEFLATE:
			case RL2_COMPRESSION_DEFLATE_NO:
			case RL2_COMPRESSION_LZMA:
			case RL2_COMPRESSION_LZMA_NO:
			case RL2_COMPRESSION_PNG:
			case RL2_COMPRESSION_LOSSY_WEBP:
			case RL2_COMPRESSION_LOSSLESS_WEBP:
			case RL2_COMPRESSION_CHARLS:
			case RL2_COMPRESSION_LOSSY_JP2:
			case RL2_COMPRESSION_LOSSLESS_JP2:
			    break;
			default:
			    return 0;
			};
		  }
	    }
	  else
	    {
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		  case RL2_COMPRESSION_DEFLATE:
		  case RL2_COMPRESSION_DEFLATE_NO:
		  case RL2_COMPRESSION_LZMA:
		  case RL2_COMPRESSION_LZMA_NO:
		      break;
		  default:
		      return 0;
		  };
	    }
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
	    case RL2_SAMPLE_UINT8:
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_UINT16:
	    case RL2_SAMPLE_INT32:
	    case RL2_SAMPLE_UINT32:
	    case RL2_SAMPLE_FLOAT:
	    case RL2_SAMPLE_DOUBLE:
		break;
	    default:
		return 0;
	    };
	  if (num_samples != 1)
	      return 0;
	  if (sample_type == RL2_SAMPLE_UINT8
	      || sample_type == RL2_SAMPLE_UINT16)
	    {
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		  case RL2_COMPRESSION_DEFLATE:
		  case RL2_COMPRESSION_DEFLATE_NO:
		  case RL2_COMPRESSION_LZMA:
		  case RL2_COMPRESSION_LZMA_NO:
		  case RL2_COMPRESSION_PNG:
		  case RL2_COMPRESSION_CHARLS:
		  case RL2_COMPRESSION_LOSSY_JP2:
		  case RL2_COMPRESSION_LOSSLESS_JP2:
		      break;
		  default:
		      return 0;
		  };
	    }
	  else
	    {
		switch (compression)
		  {
		  case RL2_COMPRESSION_NONE:
		  case RL2_COMPRESSION_DEFLATE:
		  case RL2_COMPRESSION_DEFLATE_NO:
		  case RL2_COMPRESSION_LZMA:
		  case RL2_COMPRESSION_LZMA_NO:
		      break;
		  default:
		      return 0;
		  };
	    }
	  break;
      };
    return 1;
}

static int
pack_rle_rows (rl2PrivRasterPtr raster, unsigned char *in,
	       unsigned char **pixels, int *size)
{
/* creating an RLE encoded 1-BIT pixel buffer */
    int sz = 0;
    unsigned char *p_in = in;
    unsigned char *rle;
    unsigned char *p_out;
    unsigned int row;
    unsigned int col;

    for (row = 0; row < raster->height; row++)
      {
	  int cnt = 0;
	  int pix = *p_in;
	  for (col = 0; col < raster->width; col++)
	    {
		/* computing the required size */
		if (pix == *p_in)
		  {
		      if (cnt == 128)
			{
			    sz++;
			    cnt = 1;
			}
		      else
			  cnt++;
		  }
		else
		  {
		      sz++;
		      pix = *p_in;
		      cnt = 1;
		  }
		p_in++;
	    }
	  sz++;
      }

    rle = malloc (sz);
    p_out = rle;
    p_in = in;
    for (row = 0; row < raster->height; row++)
      {
	  char byte;
	  int cnt = 0;
	  int pix = *p_in;
	  for (col = 0; col < raster->width; col++)
	    {
		/* RLE encoding */
		if (pix == *p_in)
		  {
		      if (cnt == 128)
			{
			    if (pix == 1)
			      {
				  byte = 127;
				  *p_out++ = byte;
			      }
			    else
			      {
				  byte = -128;
				  *p_out++ = byte;
			      }
			    cnt = 1;
			}
		      else
			  cnt++;
		  }
		else
		  {
		      if (pix == 1)
			{
			    byte = cnt - 1;
			    *p_out++ = byte;
			}
		      else
			{
			    byte = cnt * -1;
			    *p_out++ = byte;
			}
		      pix = *p_in;
		      cnt = 1;
		  }
		p_in++;
	    }
	  if (pix == 1)
	    {
		byte = cnt - 1;
		*p_out++ = byte;
	    }
	  else
	    {
		byte = cnt * -1;
		*p_out++ = byte;
	    }
      }
    *pixels = rle;
    *size = sz;
    return 1;
}

static int
unpack_rle (unsigned short width, unsigned short height,
	    const unsigned char *pixels_in, int pixels_in_sz,
	    unsigned char **pixels, int *pixels_sz)
{
/* unpacking an RLE encoded 1-BIT raster */
    unsigned char *buf;
    int buf_size;
    int col;
    int byte;
    int i;
    unsigned char px;
    int row_stride;
    int row_no;
    int cnt;
    unsigned char *p_out;
    const char *p_in;

    p_in = (char *) pixels_in;
    row_stride = 0;
    row_no = 0;
    for (col = 0; col < pixels_in_sz; col++)
      {
	  /* checking the encoded buffer for validity */
	  byte = *p_in++;
	  if (byte < 0)
	      row_stride += byte * -1;
	  else
	      row_stride += byte + 1;
	  if (row_stride == width)
	    {
		row_stride = 0;
		row_no++;
	    }
	  else if (row_stride > width)
	      goto error;
      }

    buf_size = width * height;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    p_in = (char *) pixels_in;
    p_out = buf;
    row_stride = 0;
    row_no = 0;
    for (col = 0; col < pixels_in_sz; col++)
      {
	  /* decoding the buffer */
	  byte = *p_in++;
	  if (byte < 0)
	    {
		px = 0;
		cnt = byte * -1;
	    }
	  else
	    {
		px = 1;
		cnt = byte + 1;
	    }
	  for (i = 0; i < cnt; i++)
	      *p_out++ = px;
      }

    *pixels = buf;
    *pixels_sz = buf_size;
    return 1;
  error:
    return 0;
}

static int
pack_1bit_rows (rl2PrivRasterPtr raster, unsigned char *in, int *xrow_stride,
		unsigned char **pixels, int *size)
{
/* creating a packed 1-BIT pixel buffer */
    int row_stride = 0;
    unsigned char *pix_buf = NULL;
    int pix_size;
    unsigned int row;
    unsigned int col;
    int cnt = 0;
    unsigned char *p_in = in;

/* computing the required sizes */
    for (col = 0; col < raster->width; col++)
      {
	  if (cnt == 0)
	      row_stride++;
	  if (cnt == 7)
	      cnt = 0;
	  else
	      cnt++;
      }
    pix_size = raster->height * row_stride;

/* allocating the pixel buffers */
    pix_buf = malloc (pix_size);
    if (pix_buf == NULL)
	return 0;

/* pixels packing */
    for (row = 0; row < raster->height; row++)
      {
	  unsigned char *p_out = pix_buf + (row_stride * row);
	  unsigned char packed = 0x00;
	  unsigned char pixel;
	  cnt = 0;
	  for (col = 0; col < raster->width; col++)
	    {
		pixel = *p_in++;
		switch (cnt)
		  {
		  case 0:
		      if (pixel != 0)
			  packed |= 0x80;
		      break;
		  case 1:
		      if (pixel != 0)
			  packed |= 0x40;
		      break;
		  case 2:
		      if (pixel != 0)
			  packed |= 0x20;
		      break;
		  case 3:
		      if (pixel != 0)
			  packed |= 0x10;
		      break;
		  case 4:
		      if (pixel != 0)
			  packed |= 0x08;
		      break;
		  case 5:
		      if (pixel != 0)
			  packed |= 0x04;
		      break;
		  case 6:
		      if (pixel != 0)
			  packed |= 0x02;
		      break;
		  case 7:
		      if (pixel != 0)
			  packed |= 0x01;
		      break;
		  };
		if (cnt == 7)
		  {
		      *p_out++ = packed;
		      packed = 0x00;
		      cnt = 0;
		  }
		else
		    cnt++;
	    }
	  if (cnt != 0)
	      *p_out++ = packed;
      }

    *xrow_stride = row_stride;
    *pixels = pix_buf;
    *size = pix_size;
    return 1;
}

static int
pack_2bit_rows (rl2PrivRasterPtr raster, int *xrow_stride,
		unsigned char **pixels, int *size)
{
/* creating a packed 2-BIT pixel buffer */
    int row_stride = 0;
    unsigned char *pix_buf = NULL;
    int pix_size;
    unsigned int row;
    unsigned int col;
    int cnt = 0;
    unsigned char *p_in = raster->rasterBuffer;

/* computing the required sizes */
    for (col = 0; col < raster->width; col++)
      {
	  if (cnt == 0)
	      row_stride++;
	  if (cnt == 3)
	      cnt = 0;
	  else
	      cnt++;
      }
    pix_size = raster->height * row_stride;

/* allocating the pixel buffers */
    pix_buf = malloc (pix_size);
    if (pix_buf == NULL)
	return 0;

/* pixels packing */
    for (row = 0; row < raster->height; row++)
      {
	  unsigned char *p_out = pix_buf + (row_stride * row);
	  unsigned char packed = 0x00;
	  unsigned char pixel;
	  cnt = 0;
	  for (col = 0; col < raster->width; col++)
	    {
		pixel = *p_in++;
		switch (cnt)
		  {
		  case 0:
		      switch (pixel)
			{
			case 1:
			    packed |= 0x40;
			    break;
			case 2:
			    packed |= 0x80;
			    break;
			case 3:
			    packed |= 0xc0;
			    break;
			};
		      break;
		  case 1:
		      switch (pixel)
			{
			case 1:
			    packed |= 0x10;
			    break;
			case 2:
			    packed |= 0x20;
			    break;
			case 3:
			    packed |= 0x30;
			    break;
			};
		      break;
		  case 2:
		      switch (pixel)
			{
			case 1:
			    packed |= 0x04;
			    break;
			case 2:
			    packed |= 0x08;
			    break;
			case 3:
			    packed |= 0x0c;
			    break;
			};
		      break;
		  case 3:
		      switch (pixel)
			{
			case 1:
			    packed |= 0x01;
			    break;
			case 2:
			    packed |= 0x02;
			    break;
			case 3:
			    packed |= 0x03;
			    break;
			};
		      break;
		  };
		if (cnt == 3)
		  {
		      *p_out++ = packed;
		      packed = 0x00;
		      cnt = 0;
		  }
		else
		    cnt++;
	    }
	  if (cnt != 0)
	      *p_out++ = packed;
      }

    *xrow_stride = row_stride;
    *pixels = pix_buf;
    *size = pix_size;
    return 1;
}

static int
pack_4bit_rows (rl2PrivRasterPtr raster, int *xrow_stride,
		unsigned char **pixels, int *size)
{
/* creating a packed 4-BIT pixel buffer */
    int row_stride = 0;
    unsigned char *pix_buf = NULL;
    int pix_size;
    unsigned int row;
    unsigned int col;
    int cnt = 0;
    unsigned char *p_in = raster->rasterBuffer;

/* computing the required sizes */
    for (col = 0; col < raster->width; col++)
      {
	  if (cnt == 0)
	      row_stride++;
	  if (cnt == 1)
	      cnt = 0;
	  else
	      cnt++;
      }
    pix_size = raster->height * row_stride;

/* allocating the pixel buffers */
    pix_buf = malloc (pix_size);
    if (pix_buf == NULL)
	return 0;

/* pixels packing */
    for (row = 0; row < raster->height; row++)
      {
	  unsigned char *p_out = pix_buf + (row_stride * row);
	  unsigned char packed = 0x00;
	  unsigned char pixel;
	  cnt = 0;
	  for (col = 0; col < raster->width; col++)
	    {
		pixel = *p_in++;
		switch (cnt)
		  {
		  case 0:
		      switch (pixel)
			{
			case 1:
			    packed |= 0x10;
			    break;
			case 2:
			    packed |= 0x20;
			    break;
			case 3:
			    packed |= 0x30;
			    break;
			case 4:
			    packed |= 0x40;
			    break;
			case 5:
			    packed |= 0x50;
			    break;
			case 6:
			    packed |= 0x60;
			    break;
			case 7:
			    packed |= 0x70;
			    break;
			case 8:
			    packed |= 0x80;
			    break;
			case 9:
			    packed |= 0x90;
			    break;
			case 10:
			    packed |= 0xa0;
			    break;
			case 11:
			    packed |= 0xb0;
			    break;
			case 12:
			    packed |= 0xc0;
			    break;
			case 13:
			    packed |= 0xd0;
			    break;
			case 14:
			    packed |= 0xe0;
			    break;
			case 15:
			    packed |= 0xf0;
			    break;
			};
		      break;
		  case 1:
		      switch (pixel)
			{
			case 1:
			    packed |= 0x01;
			    break;
			case 2:
			    packed |= 0x02;
			    break;
			case 3:
			    packed |= 0x03;
			    break;
			case 4:
			    packed |= 0x04;
			    break;
			case 5:
			    packed |= 0x05;
			    break;
			case 6:
			    packed |= 0x06;
			    break;
			case 7:
			    packed |= 0x07;
			    break;
			case 8:
			    packed |= 0x08;
			    break;
			case 9:
			    packed |= 0x09;
			    break;
			case 10:
			    packed |= 0x0a;
			    break;
			case 11:
			    packed |= 0x0b;
			    break;
			case 12:
			    packed |= 0x0c;
			    break;
			case 13:
			    packed |= 0x0d;
			    break;
			case 14:
			    packed |= 0x0e;
			    break;
			case 15:
			    packed |= 0x0f;
			    break;
			};
		      break;
		  };
		if (cnt == 1)
		  {
		      *p_out++ = packed;
		      packed = 0x00;
		      cnt = 0;
		  }
		else
		    cnt++;
	    }
	  if (cnt != 0)
	      *p_out++ = packed;
      }

    *xrow_stride = row_stride;
    *pixels = pix_buf;
    *size = pix_size;
    return 1;
}

static void
feed_odd_even_int8 (void *in, unsigned int width, unsigned int height,
		    int bands, void *pix_odd, void *pix_even)
{
/* feeding Odd/Even pixel buffers - INT8 */
    char *p_in;
    char *p_odd = pix_odd;
    char *p_even = pix_even;
    unsigned int row;
    unsigned int col;

    p_in = in;
    for (row = 0; row < height; row += 2)
      {
	  for (col = 0; col < width * bands; col++)
	      *p_odd++ = *p_in++;
	  p_in += width * bands;
      }

    p_in = in;
    for (row = 1; row < height; row += 2)
      {
	  p_in += width * bands;
	  for (col = 0; col < width * bands; col++)
	      *p_even++ = *p_in++;
      }
}

static void
feed_odd_even_uint8 (void *in, unsigned int width, unsigned int height,
		     int bands, void *pix_odd, void *pix_even)
{
/* feeding Odd/Even pixel buffers - UINT8 */
    unsigned char *p_in;
    unsigned char *p_odd = pix_odd;
    unsigned char *p_even = pix_even;
    unsigned int row;
    unsigned int col;

    p_in = in;
    for (row = 0; row < height; row += 2)
      {
	  for (col = 0; col < width * bands; col++)
	      *p_odd++ = *p_in++;
	  p_in += width * bands;
      }

    p_in = in;
    for (row = 1; row < height; row += 2)
      {
	  p_in += width * bands;
	  for (col = 0; col < width * bands; col++)
	      *p_even++ = *p_in++;
      }
}

static void
feed_odd_even_int16 (void *in, unsigned int width, unsigned int height,
		     int bands, void *pix_odd, void *pix_even, int swap)
{
/* feeding Odd/Even pixel buffers - INT16 */
    short *p_in;
    short *p_odd = pix_odd;
    short *p_even = pix_even;
    unsigned int row;
    unsigned int col;

    p_in = in;
    for (row = 0; row < height; row += 2)
      {
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_odd++ = swapINT16 (*p_in++);
		else
		    *p_odd++ = *p_in++;
	    }
	  p_in += width * bands;
      }

    p_in = in;
    for (row = 1; row < height; row += 2)
      {
	  p_in += width * bands;
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_even++ = swapINT16 (*p_in++);
		else
		    *p_even++ = *p_in++;
	    }
      }
}

static void
feed_odd_even_uint16 (void *in, unsigned int width, unsigned int height,
		      int bands, void *pix_odd, void *pix_even, int swap)
{
/* feeding Odd/Even pixel buffers - UINT16 */
    unsigned short *p_in;
    unsigned short *p_odd = pix_odd;
    unsigned short *p_even = pix_even;
    unsigned int row;
    unsigned int col;

    p_in = in;
    for (row = 0; row < height; row += 2)
      {
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_odd++ = swapUINT16 (*p_in++);
		else
		    *p_odd++ = *p_in++;
	    }
	  p_in += width * bands;
      }

    p_in = in;
    for (row = 1; row < height; row += 2)
      {
	  p_in += width * bands;
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_even++ = swapUINT16 (*p_in++);
		else
		    *p_even++ = *p_in++;
	    }
      }
}

static void
feed_odd_even_int32 (void *in, unsigned int width, unsigned int height,
		     int bands, void *pix_odd, void *pix_even, int swap)
{
/* feeding Odd/Even pixel buffers - INT32 */
    int *p_in;
    int *p_odd = pix_odd;
    int *p_even = pix_even;
    unsigned int row;
    unsigned int col;

    p_in = in;
    for (row = 0; row < height; row += 2)
      {
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_odd++ = swapINT32 (*p_in++);
		else
		    *p_odd++ = *p_in++;
	    }
	  p_in += width * bands;
      }

    p_in = in;
    for (row = 1; row < height; row += 2)
      {
	  p_in += width * bands;
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_even++ = swapINT32 (*p_in++);
		else
		    *p_even++ = *p_in++;
	    }
      }
}

static void
feed_odd_even_uint32 (void *in, unsigned int width, unsigned int height,
		      int bands, void *pix_odd, void *pix_even, int swap)
{
/* feeding Odd/Even pixel buffers - UINT32 */
    unsigned int *p_in;
    unsigned int *p_odd = pix_odd;
    unsigned int *p_even = pix_even;
    unsigned int row;
    unsigned int col;

    p_in = in;
    for (row = 0; row < height; row += 2)
      {
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_odd++ = swapUINT32 (*p_in++);
		else
		    *p_odd++ = *p_in++;
	    }
	  p_in += width * bands;
      }

    p_in = in;
    for (row = 1; row < height; row += 2)
      {
	  p_in += width * bands;
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_even++ = swapUINT32 (*p_in++);
		else
		    *p_even++ = *p_in++;
	    }
      }
}

static void
feed_odd_even_float (void *in, unsigned int width, unsigned int height,
		     int bands, void *pix_odd, void *pix_even, int swap)
{
/* feeding Odd/Even pixel buffers - FLOAT */
    float *p_in;
    float *p_odd = pix_odd;
    float *p_even = pix_even;
    unsigned int row;
    unsigned int col;

    p_in = in;
    for (row = 0; row < height; row += 2)
      {
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_odd++ = swapFloat (*p_in++);
		else
		    *p_odd++ = *p_in++;
	    }
	  p_in += width * bands;
      }

    p_in = in;
    for (row = 1; row < height; row += 2)
      {
	  p_in += width * bands;
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_even++ = swapFloat (*p_in++);
		else
		    *p_even++ = *p_in++;
	    }
      }
}

static void
feed_odd_even_double (void *in, unsigned int width, unsigned int height,
		      int bands, void *pix_odd, void *pix_even, int swap)
{
/* feeding Odd/Even pixel buffers - DOUBLE */
    double *p_in;
    double *p_odd = pix_odd;
    double *p_even = pix_even;
    unsigned int row;
    unsigned int col;

    p_in = in;
    for (row = 0; row < height; row += 2)
      {
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_odd++ = swapDouble (*p_in++);
		else
		    *p_odd++ = *p_in++;
	    }
	  p_in += width * bands;
      }

    p_in = in;
    for (row = 1; row < height; row += 2)
      {
	  p_in += width * bands;
	  for (col = 0; col < width * bands; col++)
	    {
		if (swap)
		    *p_even++ = swapDouble (*p_in++);
		else
		    *p_even++ = *p_in++;
	    }
      }
}

static int
odd_even_rows (rl2PrivRasterPtr raster, int *odd_rows, int *row_stride_odd,
	       unsigned char **pixels_odd, int *size_odd, int *even_rows,
	       int *row_stride_even, unsigned char **pixels_even,
	       int *size_even, int little_endian)
{
/* creating both Odd and Even rows buffers */
    int o_rows = 0;
    int e_rows = 0;
    int o_stride = 0;
    int e_stride = 0;
    unsigned char *pix_odd = NULL;
    unsigned char *pix_even = NULL;
    int o_size;
    int e_size;
    unsigned int row;
    int pix_size = 1;
    int swap = 0;
    if (little_endian != endianArch ())
	swap = 1;
    else
	swap = 0;

/* computing the required sizes */
    for (row = 0; row < raster->height; row += 2)
	o_rows++;
    for (row = 1; row < raster->height; row += 2)
	e_rows++;
    switch (raster->sampleType)
      {
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
	  pix_size = 1;
	  break;
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pix_size = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pix_size = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pix_size = 8;
	  break;
      };
    o_size = raster->width * o_rows * pix_size * raster->nBands;
    e_size = raster->width * e_rows * pix_size * raster->nBands;
    o_stride = raster->width * pix_size * raster->nBands;
    e_stride = o_stride;

/* allocating the pixel buffers */
    pix_odd = malloc (o_size);
    if (pix_odd == NULL)
	return 0;
    pix_even = malloc (e_size);
    if (pix_even == NULL)
      {
	  free (pix_odd);
	  return 0;
      }
    memset (pix_odd, 0, o_size);
    memset (pix_even, 0, e_size);

/* feeding the pixel buffers */
    switch (raster->sampleType)
      {
      case RL2_SAMPLE_INT8:
	  feed_odd_even_int8 (raster->rasterBuffer, raster->width,
			      raster->height, raster->nBands, pix_odd,
			      pix_even);
	  break;
      case RL2_SAMPLE_UINT8:
	  feed_odd_even_uint8 (raster->rasterBuffer, raster->width,
			       raster->height, raster->nBands, pix_odd,
			       pix_even);
	  break;
      case RL2_SAMPLE_INT16:
	  feed_odd_even_int16 (raster->rasterBuffer, raster->width,
			       raster->height, raster->nBands, pix_odd,
			       pix_even, swap);
	  break;
      case RL2_SAMPLE_UINT16:
	  feed_odd_even_uint16 (raster->rasterBuffer, raster->width,
				raster->height, raster->nBands, pix_odd,
				pix_even, swap);
	  break;
      case RL2_SAMPLE_INT32:
	  feed_odd_even_int32 (raster->rasterBuffer, raster->width,
			       raster->height, raster->nBands, pix_odd,
			       pix_even, swap);
	  break;
      case RL2_SAMPLE_UINT32:
	  feed_odd_even_uint32 (raster->rasterBuffer, raster->width,
				raster->height, raster->nBands, pix_odd,
				pix_even, swap);
	  break;
      case RL2_SAMPLE_FLOAT:
	  feed_odd_even_float (raster->rasterBuffer, raster->width,
			       raster->height, raster->nBands, pix_odd,
			       pix_even, swap);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  feed_odd_even_double (raster->rasterBuffer, raster->width,
				raster->height, raster->nBands, pix_odd,
				pix_even, swap);
	  break;
      };
    *odd_rows = o_rows;
    *even_rows = e_rows;
    *row_stride_odd = o_stride;
    *row_stride_even = e_stride;
    *pixels_odd = pix_odd;
    *pixels_even = pix_even;
    *size_odd = o_size;
    *size_even = e_size;
    return 1;
}

RL2_DECLARE int
rl2_raster_encode (rl2RasterPtr rst, int compression, unsigned char **blob_odd,
		   int *blob_odd_sz, unsigned char **blob_even,
		   int *blob_even_sz, int quality, int little_endian)
{
/* encoding a Raster into the internal RL2 binary format */
    rl2PrivRasterPtr raster = (rl2PrivRasterPtr) rst;
    int odd_rows;
    unsigned char *pixels_odd = NULL;
    int size_odd;
    int even_rows = 0;
    unsigned char *pixels_even = NULL;
    int size_even = 0;
    int row_stride_odd = 0;
    unsigned char *mask_pix = NULL;
    int mask_pix_size = 0;
    unsigned char *block_odd = NULL;
    int block_odd_size = 0;
    int row_stride_even = 0;
    unsigned char *block_even = NULL;
    int block_even_size = 0;
    unsigned char *ptr;
    int uncompressed = 0;
    int compressed;
    int uncompressed_mask = 0;
    int compressed_mask = 0;
    unsigned char *compr_data = NULL;
    unsigned char *compr_mask = NULL;
    unsigned char *to_clean1 = NULL;
    unsigned char *to_clean2 = NULL;
    unsigned char *save_mask = NULL;
    uLong crc;
    int endian_arch = endianArch ();
    int delta_dist;
    *blob_odd = NULL;
    *blob_odd_sz = 0;
    *blob_even = NULL;
    *blob_even_sz = 0;

    if (raster == NULL)
	return RL2_ERROR;
    if (!check_encode_self_consistency
	(raster->sampleType, raster->pixelType, raster->nBands, compression))
	return RL2_ERROR;

    switch (raster->pixelType)
      {
      case RL2_PIXEL_RGB:
	  switch (raster->sampleType)
	    {
	    case RL2_SAMPLE_UINT16:
		delta_dist = 6;
		break;
	    default:
		delta_dist = 3;
		break;
	    };
	  break;
      case RL2_PIXEL_MULTIBAND:
	  switch (raster->sampleType)
	    {
	    case RL2_SAMPLE_UINT16:
		delta_dist = raster->nBands * 2;
		break;
	    default:
		delta_dist = raster->nBands;
		break;
	    };
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (raster->sampleType)
	    {
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_UINT16:
		delta_dist = 2;
		break;
	    case RL2_SAMPLE_INT32:
	    case RL2_SAMPLE_UINT32:
	    case RL2_SAMPLE_FLOAT:
		delta_dist = 4;
		break;
	    case RL2_SAMPLE_DOUBLE:
		delta_dist = 8;
		break;
	    default:
		delta_dist = 1;
		break;
	    };
	  break;
      default:
	  delta_dist = 1;
	  break;
      };

    if (compression == RL2_COMPRESSION_NONE
	|| compression == RL2_COMPRESSION_DEFLATE
	|| compression == RL2_COMPRESSION_DEFLATE_NO
	|| compression == RL2_COMPRESSION_LZMA
	|| compression == RL2_COMPRESSION_LZMA_NO)
      {
	  /* preparing the pixels buffers */
	  if (raster->sampleType == RL2_SAMPLE_1_BIT)
	    {
		/* packing 1-BIT data */
		if (!pack_1bit_rows
		    (raster, raster->rasterBuffer, &row_stride_odd, &pixels_odd,
		     &size_odd))
		    return RL2_ERROR;
		odd_rows = raster->height;
	    }
	  else if (raster->sampleType == RL2_SAMPLE_2_BIT)
	    {
		/* packing 2-BIT data */
		if (!pack_2bit_rows
		    (raster, &row_stride_odd, &pixels_odd, &size_odd))
		    return RL2_ERROR;
		odd_rows = raster->height;
	    }
	  else if (raster->sampleType == RL2_SAMPLE_4_BIT)
	    {
		/* packing 4-BIT data */
		if (!pack_4bit_rows
		    (raster, &row_stride_odd, &pixels_odd, &size_odd))
		    return RL2_ERROR;
		odd_rows = raster->height;
	    }
	  else
	    {
		/* Odd/Even raster */
		if (!odd_even_rows
		    (raster, &odd_rows, &row_stride_odd, &pixels_odd, &size_odd,
		     &even_rows, &row_stride_even, &pixels_even, &size_even,
		     little_endian))
		    return RL2_ERROR;
	    }
      }
    else if (compression == RL2_COMPRESSION_PNG)
      {
	  if (raster->sampleType == RL2_SAMPLE_1_BIT
	      || raster->sampleType == RL2_SAMPLE_2_BIT
	      || raster->sampleType == RL2_SAMPLE_4_BIT)
	    {
		/* no special action is required */
	    }
	  else
	    {
		/* Odd/Even raster */
		if (!odd_even_rows
		    (raster, &odd_rows, &row_stride_odd, &pixels_odd, &size_odd,
		     &even_rows, &row_stride_even, &pixels_even, &size_even,
		     little_endian))
		    return RL2_ERROR;
	    }
      }
    else if (compression == RL2_COMPRESSION_CHARLS)
      {
	  /* Odd/Even raster */
	  if (!odd_even_rows
	      (raster, &odd_rows, &row_stride_odd, &pixels_odd, &size_odd,
	       &even_rows, &row_stride_even, &pixels_even, &size_even,
	       little_endian))
	      return RL2_ERROR;
      }
    else if (compression == RL2_COMPRESSION_JPEG
	     || compression == RL2_COMPRESSION_LOSSY_WEBP
	     || compression == RL2_COMPRESSION_LOSSLESS_WEBP
	     || compression == RL2_COMPRESSION_CCITTFAX4
	     || compression == RL2_COMPRESSION_LOSSY_JP2
	     || compression == RL2_COMPRESSION_LOSSLESS_JP2)
      {
	  /* no special action is required */
      }
    else
	return RL2_ERROR;
    if (raster->maskBuffer != NULL)
      {
	  /* preparing the mask buffer */
	  save_mask = raster->maskBuffer;
	  raster->maskBuffer = NULL;
	  /* packing RLE data */
	  if (!pack_rle_rows (raster, save_mask, &mask_pix, &mask_pix_size))
	      return RL2_ERROR;
      }

    if (compression == RL2_COMPRESSION_NONE)
      {
	  uncompressed = size_odd;
	  compressed = size_odd;
	  compr_data = pixels_odd;
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
      }
    else if (compression == RL2_COMPRESSION_DEFLATE)
      {
	  /* compressing as ZIP DeltaFilter [Deflate] */
	  int ret;
	  uLong zLen = size_odd - 1;
	  unsigned char *zip_buf = malloc (zLen);
	  if (zip_buf == NULL)
	      goto error;
	  if (rl2_delta_encode (pixels_odd, size_odd, delta_dist) != RL2_OK)
	      goto error;
	  ret =
	      compress (zip_buf, &zLen, (const Bytef *) pixels_odd,
			(uLong) size_odd);
	  if (ret == Z_OK)
	    {
		/* ok, ZIP compression was successful */
		uncompressed = size_odd;
		compressed = (int) zLen;
		compr_data = zip_buf;
		to_clean1 = zip_buf;
	    }
	  else if (ret == Z_BUF_ERROR)
	    {
		/* ZIP compression actually causes inflation: saving uncompressed data */
		if (rl2_delta_decode (pixels_odd, size_odd, delta_dist) !=
		    RL2_OK)
		    goto error;
		uncompressed = size_odd;
		compressed = size_odd;
		compr_data = pixels_odd;
		free (zip_buf);
		zip_buf = NULL;
	    }
	  else
	    {
		/* compression error */
		free (zip_buf);
		goto error;
	    }
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
      }
    else if (compression == RL2_COMPRESSION_DEFLATE_NO)
      {
	  /* compressing as ZIP noDelta [Deflate] */
	  int ret;
	  uLong zLen = size_odd - 1;
	  unsigned char *zip_buf = malloc (zLen);
	  if (zip_buf == NULL)
	      goto error;
	  ret =
	      compress (zip_buf, &zLen, (const Bytef *) pixels_odd,
			(uLong) size_odd);
	  if (ret == Z_OK)
	    {
		/* ok, ZIP compression was successful */
		uncompressed = size_odd;
		compressed = (int) zLen;
		compr_data = zip_buf;
		to_clean1 = zip_buf;
	    }
	  else if (ret == Z_BUF_ERROR)
	    {
		/* ZIP compression actually causes inflation: saving uncompressed data */
		uncompressed = size_odd;
		compressed = size_odd;
		compr_data = pixels_odd;
		free (zip_buf);
		zip_buf = NULL;
	    }
	  else
	    {
		/* compression error */
		free (zip_buf);
		goto error;
	    }
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
      }
    else if (compression == RL2_COMPRESSION_LZMA)
      {
#ifndef OMIT_LZMA		/* only if LZMA is enabled */
	  /* compressing as LZMA DeltaFilter */
	  lzma_options_lzma opt_lzma2;
	  lzma_options_delta opt_delta;
	  lzma_ret ret;
	  lzma_filter filters[3];
	  size_t out_pos = 0;
	  size_t lzmaLen = size_odd - 1;
	  unsigned char *lzma_buf = malloc (lzmaLen);
	  if (lzma_buf == NULL)
	      goto error;
	  opt_delta.type = LZMA_DELTA_TYPE_BYTE;
	  opt_delta.dist = delta_dist;
	  lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
	  filters[0].id = LZMA_FILTER_DELTA;
	  filters[0].options = &opt_delta;
	  filters[1].id = LZMA_FILTER_LZMA2;
	  filters[1].options = &opt_lzma2;
	  filters[2].id = LZMA_VLI_UNKNOWN;
	  filters[2].options = NULL;
	  ret =
	      lzma_raw_buffer_encode (filters, NULL,
				      (const uint8_t *) pixels_odd, size_odd,
				      lzma_buf, &out_pos, lzmaLen);
	  if (ret == LZMA_OK)
	    {
		/* ok, LZMA compression was successful */
		uncompressed = size_odd;
		compressed = (int) out_pos;
		compr_data = lzma_buf;
		to_clean1 = lzma_buf;
	    }
	  else if (ret == LZMA_BUF_ERROR)
	    {
		/* LZMA compression actually causes inflation: saving uncompressed data */
		uncompressed = size_odd;
		compressed = size_odd;
		compr_data = pixels_odd;
		free (lzma_buf);
		lzma_buf = NULL;
	    }
	  else
	    {
		/* compression error */
		free (lzma_buf);
		goto error;
	    }
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
#else /* LZMA is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling LZMA support\n");
	  goto error;
#endif /* end LZMA conditional */
      }
    else if (compression == RL2_COMPRESSION_LZMA_NO)
      {
#ifndef OMIT_LZMA		/* only if LZMA is enabled */
	  /* compressing as LZMA noDelta */
	  lzma_options_lzma opt_lzma2;
	  lzma_ret ret;
	  lzma_filter filters[2];
	  size_t out_pos = 0;
	  size_t lzmaLen = size_odd - 1;
	  unsigned char *lzma_buf = malloc (lzmaLen);
	  if (lzma_buf == NULL)
	      goto error;
	  lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
	  filters[0].id = LZMA_FILTER_LZMA2;
	  filters[0].options = &opt_lzma2;
	  filters[1].id = LZMA_VLI_UNKNOWN;
	  filters[1].options = NULL;
	  ret =
	      lzma_raw_buffer_encode (filters, NULL,
				      (const uint8_t *) pixels_odd, size_odd,
				      lzma_buf, &out_pos, lzmaLen);
	  if (ret == LZMA_OK)
	    {
		/* ok, LZMA compression was successful */
		uncompressed = size_odd;
		compressed = (int) out_pos;
		compr_data = lzma_buf;
		to_clean1 = lzma_buf;
	    }
	  else if (ret == LZMA_BUF_ERROR)
	    {
		/* LZMA compression actually causes inflation: saving uncompressed data */
		uncompressed = size_odd;
		compressed = size_odd;
		compr_data = pixels_odd;
		free (lzma_buf);
		lzma_buf = NULL;
	    }
	  else
	    {
		/* compression error */
		free (lzma_buf);
		goto error;
	    }
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
#else /* LZMA is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling LZMA support\n");
	  goto error;
#endif /* end LZMA conditional */
      }
    else if (compression == RL2_COMPRESSION_JPEG)
      {
	  /* compressing as JPEG */
	  if (rl2_raster_to_jpeg (rst, &compr_data, &compressed, quality) ==
	      RL2_OK)
	    {
		/* ok, JPEG compression was successful */
		uncompressed = raster->width * raster->height * raster->nBands;
		to_clean1 = compr_data;
	    }
	  else
	      goto error;
	  odd_rows = raster->height;
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
      }
    else if (compression == RL2_COMPRESSION_LOSSLESS_WEBP)
      {
#ifndef OMIT_WEBP		/* only if WebP is enabled */
	  /* compressing as lossless WEBP */
	  if (rl2_raster_to_lossless_webp (rst, &compr_data, &compressed) ==
	      RL2_OK)
	    {
		/* ok, lossless WEBP compression was successful */
		uncompressed = raster->width * raster->height * raster->nBands;
		to_clean1 = compr_data;
	    }
	  else
	      goto error;
	  odd_rows = raster->height;
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
#else /* WebP is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling WebP support\n");
	  goto error;
#endif /* end WebP conditional */
      }
    else if (compression == RL2_COMPRESSION_LOSSY_WEBP)
      {
#ifndef OMIT_WEBP		/* only if WebP is enabled */
	  /* compressing as lossy WEBP */
	  if (rl2_raster_to_lossy_webp (rst, &compr_data, &compressed, quality)
	      == RL2_OK)
	    {
		/* ok, lossy WEBP compression was successful */
		uncompressed = raster->width * raster->height * raster->nBands;
		to_clean1 = compr_data;
	    }
	  else
	      goto error;
	  odd_rows = raster->height;
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
#else /* WebP is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling WebP support\n");
	  goto error;
#endif /* end WebP conditional */
      }
    else if (compression == RL2_COMPRESSION_PNG)
      {
	  /* compressing as PNG */
	  if (raster->sampleType == RL2_SAMPLE_1_BIT
	      || raster->sampleType == RL2_SAMPLE_2_BIT
	      || raster->sampleType == RL2_SAMPLE_4_BIT)
	    {
		/* solid ODD block */
		if (rl2_raster_to_png (rst, &compr_data, &compressed) == RL2_OK)
		  {
		      /* ok, PNG compression was successful */
		      uncompressed = raster->width * raster->height;
		      to_clean1 = compr_data;
		      odd_rows = raster->height;
		      if (mask_pix == NULL)
			  uncompressed_mask = 0;
		      else
			  uncompressed_mask = raster->width * raster->height;
		      compressed_mask = mask_pix_size;
		      compr_mask = mask_pix;
		  }
		else
		    goto error;
	    }
	  else
	    {
		/* split between Odd/Even Blocks */
		rl2PalettePtr plt = rl2_get_raster_palette (rst);
		if (rl2_data_to_png
		    (pixels_odd, NULL, 1.0, plt, raster->width,
		     odd_rows, raster->sampleType, raster->pixelType,
		     raster->nBands, &compr_data, &compressed) == RL2_OK)
		  {
		      /* ok, PNG compression was successful */
		      uncompressed = size_odd;
		      to_clean1 = compr_data;
		      if (mask_pix == NULL)
			  uncompressed_mask = 0;
		      else
			  uncompressed_mask = raster->width * raster->height;
		      compressed_mask = mask_pix_size;
		      compr_mask = mask_pix;
		  }
		else
		    goto error;
	    }
      }
    else if (compression == RL2_COMPRESSION_CHARLS)
      {
#ifndef OMIT_CHARLS		/* only if CharLS is enabled */
	  /* compressing as CHARLS */
	  if (rl2_data_to_charls
	      (pixels_odd, raster->width,
	       odd_rows, raster->sampleType, raster->pixelType,
	       raster->nBands, &compr_data, &compressed) == RL2_OK)
	    {
		/* ok, CHARLS compression was successful */
		uncompressed = size_odd;
		to_clean1 = compr_data;
		if (mask_pix == NULL)
		    uncompressed_mask = 0;
		else
		    uncompressed_mask = raster->width * raster->height;
		compressed_mask = mask_pix_size;
		compr_mask = mask_pix;
	    }
	  else
	      goto error;
#else /* CharLS is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling CharLS support\n");
	  goto error;
#endif /* end CharLS conditional */
      }
    else if (compression == RL2_COMPRESSION_CCITTFAX4)
      {
	  /* compressing as TIFF FAX4 */
	  if (rl2_raster_to_tiff_mono4
	      (rst, &compr_data, &compressed) == RL2_OK)
	    {
		/* ok, TIFF compression was successful */
		uncompressed = raster->width * raster->height;
		to_clean1 = compr_data;
		odd_rows = raster->height;
		if (mask_pix == NULL)
		    uncompressed_mask = 0;
		else
		    uncompressed_mask = raster->width * raster->height;
		compressed_mask = mask_pix_size;
		compr_mask = mask_pix;
	    }
	  else
	      goto error;
      }
    else if (compression == RL2_COMPRESSION_LOSSLESS_JP2)
      {
#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
	  /* compressing as lossless Jpeg2000 */
	  if (rl2_raster_to_lossless_jpeg2000 (rst, &compr_data, &compressed) ==
	      RL2_OK)
	    {
		/* ok, lossless Jpeg2000 compression was successful */
		uncompressed = raster->width * raster->height * raster->nBands;
		to_clean1 = compr_data;
	    }
	  else
	      goto error;
	  odd_rows = raster->height;
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
#else /* OpenJpeg is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling OpenJpeg support\n");
	  goto error;
#endif /* end OpenJpeg conditional */
      }
    else if (compression == RL2_COMPRESSION_LOSSY_JP2)
      {
#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
	  /* compressing as lossy Jpeg2000 */
	  if (rl2_raster_to_lossy_jpeg2000
	      (rst, &compr_data, &compressed, quality) == RL2_OK)
	    {
		/* ok, lossy Jpeg2000 compression was successful */
		uncompressed = raster->width * raster->height * raster->nBands;
		to_clean1 = compr_data;
	    }
	  else
	      goto error;
	  odd_rows = raster->height;
	  if (mask_pix == NULL)
	      uncompressed_mask = 0;
	  else
	      uncompressed_mask = raster->width * raster->height;
	  compressed_mask = mask_pix_size;
	  compr_mask = mask_pix;
#else /* OpenJpeg is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling OpenJpeg support\n");
	  goto error;
#endif /* end OpenJpeg conditional */
      }

/* preparing the OddBlock */
    block_odd_size = 40 + compressed + compressed_mask;
    block_odd = malloc (block_odd_size);
    if (block_odd == NULL)
	goto error;
    ptr = block_odd;
    *ptr++ = 0x00;		/* start marker */
    *ptr++ = RL2_ODD_BLOCK_START;	/* OddBlock marker */
    if (little_endian)		/* endian marker */
	*ptr++ = RL2_LITTLE_ENDIAN;
    else
	*ptr++ = RL2_BIG_ENDIAN;
    *ptr++ = compression;	/* compression marker */
    *ptr++ = raster->sampleType;	/* sample type marker */
    *ptr++ = raster->pixelType;	/* pixel type marker */
    *ptr++ = raster->nBands;	/* # Bands marker */
    exportU16 (ptr, raster->width, little_endian, endian_arch);	/* the raster width */
    ptr += 2;
    exportU16 (ptr, raster->height, little_endian, endian_arch);	/* the raster height */
    ptr += 2;
    exportU16 (ptr, row_stride_odd, little_endian, endian_arch);	/* the block row stride */
    ptr += 2;
    exportU16 (ptr, odd_rows, little_endian, endian_arch);	/* block #rows */
    ptr += 2;
    exportU32 (ptr, uncompressed, little_endian, endian_arch);	/* uncompressed payload size in bytes */
    ptr += 4;
    exportU32 (ptr, compressed, little_endian, endian_arch);	/* compressed payload size in bytes */
    ptr += 4;
    exportU32 (ptr, uncompressed_mask, little_endian, endian_arch);	/* uncompressed mask size in bytes */
    ptr += 4;
    exportU32 (ptr, compressed_mask, little_endian, endian_arch);	/* compressed mask size in bytes */
    ptr += 4;
    *ptr++ = RL2_DATA_START;
    memcpy (ptr, compr_data, compressed);	/* the payload */
    ptr += compressed;
    *ptr++ = RL2_DATA_END;
    *ptr++ = RL2_MASK_START;
    if (compr_mask != NULL)
      {
	  memcpy (ptr, compr_mask, compressed_mask);	/* the mask */
	  ptr += compressed_mask;
      }
    *ptr++ = RL2_MASK_END;
/* computing the CRC32 */
    crc = crc32 (0L, block_odd, ptr - block_odd);
    exportU32 (ptr, crc, little_endian, endian_arch);	/* the OddBlock own CRC */
    ptr += 4;
    *ptr = RL2_ODD_BLOCK_END;

    if (size_even)
      {
	  /* preparing the EvenBlock */
	  if (compression == RL2_COMPRESSION_NONE)
	    {
		uncompressed = size_even;
		compressed = size_even;
		compr_data = pixels_even;
	    }
	  else if (compression == RL2_COMPRESSION_DEFLATE)
	    {
		/* compressing as ZIP DeltaFilter [Deflate] */
		int ret;
		uLong zLen = compressBound (size_even);
		unsigned char *zip_buf = malloc (zLen);
		if (zip_buf == NULL)
		    goto error;
		if (rl2_delta_encode (pixels_even, size_even, delta_dist) !=
		    RL2_OK)
		    goto error;
		ret =
		    compress (zip_buf, &zLen, (const Bytef *) pixels_even,
			      (uLong) size_even);
		if (ret == Z_OK)
		  {
		      /* ok, ZIP compression was successful */
		      uncompressed = size_even;
		      compressed = (int) zLen;
		      compr_data = zip_buf;
		      to_clean2 = zip_buf;
		  }
		else if (ret == Z_BUF_ERROR)
		  {
		      /* ZIP compression actually causes inflation: saving uncompressed data */
		      if (rl2_delta_decode (pixels_even, size_even, delta_dist)
			  != RL2_OK)
			  goto error;
		      uncompressed = size_even;
		      compressed = size_even;
		      compr_data = pixels_even;
		      free (zip_buf);
		      zip_buf = NULL;
		  }
		else
		  {
		      /* compression error */
		      free (zip_buf);
		      goto error;
		  }
	    }
	  else if (compression == RL2_COMPRESSION_DEFLATE_NO)
	    {
		/* compressing as ZIP noDelta [Deflate] */
		int ret;
		uLong zLen = compressBound (size_even);
		unsigned char *zip_buf = malloc (zLen);
		if (zip_buf == NULL)
		    goto error;
		ret =
		    compress (zip_buf, &zLen, (const Bytef *) pixels_even,
			      (uLong) size_even);
		if (ret == Z_OK)
		  {
		      /* ok, ZIP compression was successful */
		      uncompressed = size_even;
		      compressed = (int) zLen;
		      compr_data = zip_buf;
		      to_clean2 = zip_buf;
		  }
		else if (ret == Z_BUF_ERROR)
		  {
		      /* ZIP compression actually causes inflation: saving uncompressed data */
		      uncompressed = size_even;
		      compressed = size_even;
		      compr_data = pixels_even;
		      free (zip_buf);
		      zip_buf = NULL;
		  }
		else
		  {
		      /* compression error */
		      free (zip_buf);
		      goto error;
		  }
	    }
	  else if (compression == RL2_COMPRESSION_LZMA)
	    {
#ifndef OMIT_LZMA		/* only if LZMA is enabled */
		/* compressing as LZMA DeltaFilter */
		lzma_options_lzma opt_lzma2;
		lzma_options_delta opt_delta;
		lzma_ret ret;
		lzma_filter filters[3];
		size_t out_pos = 0;
		size_t lzmaLen = size_even - 1;
		unsigned char *lzma_buf = malloc (lzmaLen);
		if (lzma_buf == NULL)
		    goto error;
		lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
		opt_delta.type = LZMA_DELTA_TYPE_BYTE;
		opt_delta.dist = delta_dist;
		lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
		filters[0].id = LZMA_FILTER_DELTA;
		filters[0].options = &opt_delta;
		filters[1].id = LZMA_FILTER_LZMA2;
		filters[1].options = &opt_lzma2;
		filters[2].id = LZMA_VLI_UNKNOWN;
		filters[2].options = NULL;
		ret =
		    lzma_raw_buffer_encode (filters, NULL,
					    (const uint8_t *) pixels_even,
					    size_even, lzma_buf, &out_pos,
					    lzmaLen);
		if (ret == LZMA_OK)
		  {
		      /* ok, LZMA compression was successful */
		      uncompressed = size_even;
		      compressed = (int) out_pos;
		      compr_data = lzma_buf;
		      to_clean2 = lzma_buf;
		  }
		else if (ret == LZMA_BUF_ERROR)
		  {
		      /* LZMA compression actually causes inflation: saving uncompressed data */
		      uncompressed = size_even;
		      compressed = size_even;
		      compr_data = pixels_even;
		      free (lzma_buf);
		      lzma_buf = NULL;
		  }
		else
		  {
		      /* compression error */
		      free (lzma_buf);
		      goto error;
		  }
#else /* LZMA is disabled */
		fprintf (stderr,
			 "librasterlite2 was built by disabling LZMA support\n");
		goto error;
#endif /* end LZMA conditional */
	    }
	  else if (compression == RL2_COMPRESSION_LZMA_NO)
	    {
#ifndef OMIT_LZMA		/* only if LZMA is enabled */
		/* compressing as LZMA noDelta */
		lzma_options_lzma opt_lzma2;
		lzma_ret ret;
		lzma_filter filters[2];
		size_t out_pos = 0;
		size_t lzmaLen = size_even - 1;
		unsigned char *lzma_buf = malloc (lzmaLen);
		if (lzma_buf == NULL)
		    goto error;
		lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
		lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
		filters[0].id = LZMA_FILTER_LZMA2;
		filters[0].options = &opt_lzma2;
		filters[1].id = LZMA_VLI_UNKNOWN;
		filters[1].options = NULL;
		ret =
		    lzma_raw_buffer_encode (filters, NULL,
					    (const uint8_t *) pixels_even,
					    size_even, lzma_buf, &out_pos,
					    lzmaLen);
		if (ret == LZMA_OK)
		  {
		      /* ok, LZMA compression was successful */
		      uncompressed = size_even;
		      compressed = (int) out_pos;
		      compr_data = lzma_buf;
		      to_clean2 = lzma_buf;
		  }
		else if (ret == LZMA_BUF_ERROR)
		  {
		      /* LZMA compression actually causes inflation: saving uncompressed data */
		      uncompressed = size_even;
		      compressed = size_even;
		      compr_data = pixels_even;
		      free (lzma_buf);
		      lzma_buf = NULL;
		  }
		else
		  {
		      /* compression error */
		      free (lzma_buf);
		      goto error;
		  }
#else /* LZMA is disabled */
		fprintf (stderr,
			 "librasterlite2 was built by disabling LZMA support\n");
		goto error;
#endif /* end LZMA conditional */
	    }
	  else if (compression == RL2_COMPRESSION_PNG)
	    {
		/* compressing as PNG */
		if (raster->sampleType == RL2_SAMPLE_1_BIT
		    || raster->sampleType == RL2_SAMPLE_2_BIT
		    || raster->sampleType == RL2_SAMPLE_4_BIT)
		    ;
		else
		  {
		      /* split between Odd/Even Blocks */
		      rl2PalettePtr plt = rl2_get_raster_palette (rst);
		      if (rl2_data_to_png
			  (pixels_even, NULL, 1.0, plt, raster->width,
			   even_rows, raster->sampleType, raster->pixelType,
			   raster->nBands, &compr_data, &compressed) == RL2_OK)
			{
			    /* ok, PNG compression was successful */
			    uncompressed = size_even;
			    to_clean2 = compr_data;
			}
		      else
			  goto error;
		  }
	    }
	  else if (compression == RL2_COMPRESSION_CHARLS)
	    {
#ifndef OMIT_CHARLS		/* only if CharLS is enabled */
		/* compressing as CHARLS */
		if (rl2_data_to_charls
		    (pixels_even, raster->width,
		     even_rows, raster->sampleType, raster->pixelType,
		     raster->nBands, &compr_data, &compressed) == RL2_OK)
		  {
		      /* ok, CHARLS compression was successful */
		      uncompressed = size_even;
		      to_clean2 = compr_data;
		  }
		else
		    goto error;
#else /* CharLS is disabled */
		fprintf (stderr,
			 "librasterlite2 was built by disabling CharLS support\n");
		goto error;
#endif /* end CharLS conditional */
	    }
	  block_even_size = 32 + compressed;
	  block_even = malloc (block_even_size);
	  if (block_even == NULL)
	      goto error;
	  ptr = block_even;
	  *ptr++ = 0x00;	/* start marker */
	  *ptr++ = RL2_EVEN_BLOCK_START;	/* EvenBlock marker */
	  if (little_endian)	/* endian marker */
	      *ptr++ = RL2_LITTLE_ENDIAN;
	  else
	      *ptr++ = RL2_BIG_ENDIAN;
	  *ptr++ = compression;	/* compression marker */
	  *ptr++ = raster->sampleType;	/* sample type marker */
	  *ptr++ = raster->pixelType;	/* pixel type marker */
	  *ptr++ = raster->nBands;	/* # Bands marker */
	  exportU16 (ptr, raster->width, little_endian, endian_arch);	/* the raster width */
	  ptr += 2;
	  exportU16 (ptr, raster->height, little_endian, endian_arch);	/* the raster height */
	  ptr += 2;
	  exportU16 (ptr, even_rows, little_endian, endian_arch);	/* block #rows */
	  ptr += 2;
	  exportU32 (ptr, crc, little_endian, endian_arch);	/* the OddBlock own CRC */
	  ptr += 4;
	  exportU32 (ptr, uncompressed, little_endian, endian_arch);	/* uncompressed payload size in bytes */
	  ptr += 4;
	  exportU32 (ptr, compressed, little_endian, endian_arch);	/* compressed payload size in bytes */
	  ptr += 4;
	  *ptr++ = RL2_DATA_START;
	  memcpy (ptr, compr_data, compressed);	/* the payload */
	  ptr += compressed;
	  *ptr++ = RL2_DATA_END;
	  /* computing the CRC32 */
	  crc = crc32 (0L, block_even, ptr - block_even);
	  exportU32 (ptr, crc, little_endian, endian_arch);	/* the EvenBlock own CRC */
	  ptr += 4;
	  *ptr = RL2_EVEN_BLOCK_END;
      }

    if (pixels_odd != NULL)
	free (pixels_odd);
    if (pixels_even != NULL)
	free (pixels_even);
    if (mask_pix != NULL)
	free (mask_pix);
    if (to_clean1 != NULL)
	free (to_clean1);
    if (to_clean2 != NULL)
	free (to_clean2);

    raster->maskBuffer = save_mask;
    *blob_odd = block_odd;
    *blob_odd_sz = block_odd_size;
    *blob_even = block_even;
    *blob_even_sz = block_even_size;
    return RL2_OK;

  error:
    if (pixels_odd != NULL)
	free (pixels_odd);
    if (pixels_even != NULL)
	free (pixels_even);
    if (mask_pix != NULL)
	free (mask_pix);
    if (to_clean1 != NULL)
	free (to_clean1);
    if (to_clean2 != NULL)
	free (to_clean2);
    if (block_odd != NULL)
	free (block_odd);
    if (block_even != NULL)
	free (block_even);
    raster->maskBuffer = save_mask;
    return RL2_ERROR;
}

static int
check_blob_odd (const unsigned char *blob, int blob_sz, unsigned int *xwidth,
		unsigned int *xheight, unsigned char *xsample_type,
		unsigned char *xpixel_type, unsigned char *xnum_bands,
		unsigned char *xcompression, uLong * xcrc)
{
/* checking the OddBlock for validity */
    const unsigned char *ptr;
    unsigned short width;
    unsigned short height;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    int compressed;
    int compressed_mask;
    uLong crc;
    uLong oldCrc;
    int endian;
    int endian_arch = endianArch ();

    if (blob_sz < 41)
	return 0;
    ptr = blob;
    if (*ptr++ != 0x00)
	return 0;		/* invalid start signature */
    if (*ptr++ != RL2_ODD_BLOCK_START)
	return 0;		/* invalid start signature */
    endian = *ptr++;
    if (endian == RL2_LITTLE_ENDIAN || endian == RL2_BIG_ENDIAN)
	;
    else
	return 0;		/* invalid endiannes */
    compression = *ptr++;	/* compression */
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
      case RL2_COMPRESSION_DEFLATE:
      case RL2_COMPRESSION_DEFLATE_NO:
      case RL2_COMPRESSION_LZMA:
      case RL2_COMPRESSION_LZMA_NO:
      case RL2_COMPRESSION_PNG:
      case RL2_COMPRESSION_JPEG:
      case RL2_COMPRESSION_LOSSY_WEBP:
      case RL2_COMPRESSION_LOSSLESS_WEBP:
      case RL2_COMPRESSION_CCITTFAX4:
      case RL2_COMPRESSION_CHARLS:
      case RL2_COMPRESSION_LOSSY_JP2:
      case RL2_COMPRESSION_LOSSLESS_JP2:
	  break;
      default:
	  return 0;
      };
    sample_type = *ptr++;	/* sample type */
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
      case RL2_SAMPLE_DOUBLE:
      case RL2_COMPRESSION_LOSSY_JP2:
      case RL2_COMPRESSION_LOSSLESS_JP2:
	  break;
      default:
	  return 0;
      };
    pixel_type = *ptr++;	/* pixel type */
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
      case RL2_PIXEL_MULTIBAND:
      case RL2_PIXEL_DATAGRID:
	  break;
      default:
	  return 0;
      };
    num_bands = *ptr++;		/* # Bands */
    width = importU16 (ptr, endian, endian_arch);
    ptr += 2;
    height = importU16 (ptr, endian, endian_arch);
    ptr += 2;
    ptr += 4;			/* skipping */
    ptr += 4;			/* skipping the uncompressed payload size */
    compressed = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    ptr += 4;			/* skipping the uncompressed mask size */
    compressed_mask = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (*ptr++ != RL2_DATA_START)
	return 0;
    if (blob_sz < 40 + compressed + compressed_mask)
	return 0;
    ptr += compressed;
    if (*ptr++ != RL2_DATA_END)
	return 0;
    if (*ptr++ != RL2_MASK_START)
	return 0;
    ptr += compressed_mask;
    if (*ptr++ != RL2_MASK_END)
	return 0;
/* computing the CRC32 */
    crc = crc32 (0L, blob, ptr - blob);
    oldCrc = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (crc != oldCrc)
	return 0;
    if (*ptr != RL2_ODD_BLOCK_END)
	return 0;		/* invalid end signature */

    *xwidth = width;
    *xheight = height;
    *xsample_type = sample_type;
    *xpixel_type = pixel_type;
    *xnum_bands = num_bands;
    *xcompression = compression;
    *xcrc = crc;
    return 1;
}

static int
check_blob_even (const unsigned char *blob, int blob_sz, unsigned short xwidth,
		 unsigned short xheight, unsigned char xsample_type,
		 unsigned char xpixel_type, unsigned char xnum_bands,
		 unsigned char xcompression, uLong xcrc)
{
/* checking the EvenBlock for validity */
    const unsigned char *ptr;
    unsigned short width;
    unsigned short height;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    int compressed;
    uLong crc;
    uLong oldCrc;
    int endian;
    int endian_arch = endianArch ();

    if (blob_sz < 33)
	return 0;
    ptr = blob;
    if (*ptr++ != 0x00)
	return 0;		/* invalid start signature */
    if (*ptr++ != RL2_EVEN_BLOCK_START)
	return 0;		/* invalid start signature */
    endian = *ptr++;
    if (endian == RL2_LITTLE_ENDIAN || endian == RL2_BIG_ENDIAN)
	;
    else
	return 0;		/* invalid endiannes */
    compression = *ptr++;	/* compression */
    if (xcompression != compression)
	return 0;
    sample_type = *ptr++;	/* sample type */
    if (xsample_type != sample_type)
	return 0;
    pixel_type = *ptr++;	/* pixel type */
    if (xpixel_type != pixel_type)
	return 0;
    num_bands = *ptr++;		/* # Bands */
    if (num_bands != xnum_bands)
	return 0;
    width = importU16 (ptr, endian, endian_arch);
    ptr += 2;
    if (xwidth != width)
	return 0;
    height = importU16 (ptr, endian, endian_arch);
    ptr += 2;
    if (xheight != height)
	return 0;
    ptr += 2;			/* skipping block # rows */
    crc = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (xcrc != crc)
	return 0;
    ptr += 4;			/* skipping the uncompressed payload size */
    compressed = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (*ptr++ != RL2_DATA_START)
	return 0;
    if (blob_sz < 32 + compressed)
	return 0;
    ptr += compressed;
    if (*ptr++ != RL2_DATA_END)
	return 0;
/* computing the CRC32 */
    crc = crc32 (0L, blob, ptr - blob);
    oldCrc = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (crc != oldCrc)
	return 0;
    if (*ptr != RL2_EVEN_BLOCK_END)
	return 0;		/* invalid end signature */
    return 1;
}

static int
check_scale (int scale, unsigned char sample_type, unsigned char compression,
	     const unsigned char *blob_even)
{
/* checking if the encoded raster could be decoded at given scale */
    switch (scale)
      {
      case RL2_SCALE_1:
	  if (sample_type == RL2_SAMPLE_1_BIT || sample_type == RL2_SAMPLE_2_BIT
	      || sample_type == RL2_SAMPLE_4_BIT)
	      ;
	  else if (compression == RL2_COMPRESSION_JPEG
		   || compression == RL2_COMPRESSION_LOSSY_WEBP
		   || compression == RL2_COMPRESSION_LOSSLESS_WEBP
		   || compression == RL2_COMPRESSION_CCITTFAX4
		   || compression == RL2_COMPRESSION_LOSSY_JP2
		   || compression == RL2_COMPRESSION_LOSSLESS_JP2)
	    {
		if (blob_even != NULL)
		    return 0;
	    }
	  else if (blob_even == NULL)
	      return 0;
	  break;
      case RL2_SCALE_2:
      case RL2_SCALE_4:
      case RL2_SCALE_8:
	  break;
      default:
	  return 0;
      };
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
	  if (scale != RL2_SCALE_1)
	      return 0;
	  break;
      };
    return 1;
}

static void
do_copy2_int8 (const char *p_odd, char *buf, unsigned int width,
	       unsigned int odd_rows)
{
/* reassembling an INT8 raster - scale 1:2 */
    unsigned int row;
    unsigned int col;
    char *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  for (col = 0; col < width; col += 2)
	      *p_out++ = *p_odd++;
	  p_odd++;
      }
}

static void
do_copy2_uint8 (const unsigned char *p_odd, unsigned char *buf,
		unsigned int width, unsigned int odd_rows,
		unsigned char num_bands)
{
/* reassembling a UINT8 raster - scale 1:2 */
    unsigned int row;
    unsigned int col;
    int band;
    unsigned char *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  for (col = 0; col < width; col += 2)
	    {
		for (band = 0; band < num_bands; band++)
		    *p_out++ = *p_odd++;
		p_odd += num_bands;
	    }
      }
}

static void
do_copy2_int16 (int swap, const short *p_odd, short *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling an INT16 raster - scale 1:2 */
    unsigned int row;
    unsigned int col;
    short *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  for (col = 0; col < width; col += 2)
	    {
		if (swap)
		    *p_out++ = swapINT16 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd++;
	    }
      }
}

static void
do_copy2_uint16 (int swap, const unsigned short *p_odd, unsigned short *buf,
		 unsigned int width, unsigned int odd_rows,
		 unsigned char num_bands)
{
/* reassembling a UINT16 raster - scale 1:2 */
    unsigned int row;
    unsigned int col;
    int band;
    unsigned short *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  for (col = 0; col < width; col += 2)
	    {
		for (band = 0; band < num_bands; band++)
		  {
		      if (swap)
			  *p_out++ = swapUINT16 (*p_odd++);
		      else
			  *p_out++ = *p_odd++;
		  }
		p_odd += num_bands;
	    }
      }
}

static void
do_copy2_int32 (int swap, const int *p_odd, int *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling an INT32 raster - scale 1:2 */
    unsigned int row;
    unsigned int col;
    int *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  for (col = 0; col < width; col += 2)
	    {
		if (swap)
		    *p_out++ = swapINT32 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd++;
	    }
      }
}

static void
do_copy2_uint32 (int swap, const unsigned int *p_odd, unsigned int *buf,
		 unsigned int width, unsigned int odd_rows)
{
/* reassembling an UINT32 raster - scale 1:2 */
    unsigned int row;
    unsigned int col;
    unsigned int *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  for (col = 0; col < width; col += 2)
	    {
		if (swap)
		    *p_out++ = swapUINT32 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd++;
	    }
      }
}

static void
do_copy2_float (int swap, const float *p_odd, float *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling a FLOAT raster - scale 1:2 */
    unsigned int row;
    unsigned int col;
    float *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  for (col = 0; col < width; col += 2)
	    {
		if (swap)
		    *p_out++ = swapFloat (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd++;
	    }
      }
}

static void
do_copy2_double (int swap, const double *p_odd, double *buf,
		 unsigned int width, unsigned int odd_rows)
{
/* reassembling a FLOAT raster - scale 1:2 */
    unsigned int row;
    unsigned int col;
    double *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  for (col = 0; col < width; col += 2)
	    {
		if (swap)
		    *p_out++ = swapDouble (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd++;
	    }
      }
}

static int
build_pixel_buffer_2 (int swap, unsigned int *xwidth, unsigned int *xheight,
		      unsigned char sample_type, unsigned char pixel_size,
		      unsigned char num_bands, unsigned short odd_rows,
		      const void *pixels_odd, void **pixels, int *pixels_sz)
{
/* decoding the raster - scale 1:2 */
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int row;
    unsigned int col;
    void *buf;
    int buf_size;

    for (row = 0; row < *xheight; row += 2)
	height++;
    for (col = 0; col < *xwidth; col += 2)
	width++;

    buf_size = width * height * pixel_size * num_bands;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  do_copy2_int8 (pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT8:
	  do_copy2_uint8 (pixels_odd, buf, *xwidth, odd_rows, num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  do_copy2_int16 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT16:
	  do_copy2_uint16 (swap, pixels_odd, buf, *xwidth, odd_rows, num_bands);
	  break;
      case RL2_SAMPLE_INT32:
	  do_copy2_int32 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT32:
	  do_copy2_uint32 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_FLOAT:
	  do_copy2_float (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  do_copy2_double (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      };

    *xwidth = width;
    *xheight = height;
    *pixels = buf;
    *pixels_sz = buf_size;
    return 1;
}

static void
do_copy4_int8 (const char *p_odd, char *buf, unsigned int width,
	       unsigned int odd_rows)
{
/* reassembling an INT8 raster - scale 1:4 */
    unsigned int row;
    unsigned int col;
    char *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 2)
      {
	  for (col = 0; col < width; col += 4)
	    {
		*p_out++ = *p_odd++;
		p_odd += 3;
	    }
	  p_odd += width;
      }
}

static void
do_copy4_uint8 (const unsigned char *p_odd, unsigned char *buf,
		unsigned int width, unsigned int odd_rows,
		unsigned char num_bands)
{
/* reassembling a UINT8 raster - scale 1:4 */
    unsigned int row;
    unsigned int col;
    int band;
    unsigned char *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 2)
      {
	  for (col = 0; col < width; col += 4)
	    {
		for (band = 0; band < num_bands; band++)
		    *p_out++ = *p_odd++;
		p_odd += num_bands * 3;
	    }
	  p_odd += width * num_bands;
      }
}

static void
do_copy4_int16 (int swap, const short *p_odd, short *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling an INT16 raster - scale 1:4 */
    unsigned int row;
    unsigned int col;
    short *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 2)
      {
	  for (col = 0; col < width; col += 4)
	    {
		if (swap)
		    *p_out++ = swapINT16 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 3;
	    }
	  p_odd += width;
      }
}

static void
do_copy4_uint16 (int swap, const unsigned short *p_odd, unsigned short *buf,
		 unsigned int width, unsigned int odd_rows,
		 unsigned char num_bands)
{
/* reassembling a UINT16 raster - scale 1:4 */
    unsigned int row;
    unsigned int col;
    int band;
    unsigned short *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 2)
      {
	  for (col = 0; col < width; col += 4)
	    {
		for (band = 0; band < num_bands; band++)
		  {
		      if (swap)
			  *p_out++ = swapUINT16 (*p_odd++);
		      else
			  *p_out++ = *p_odd++;
		  }
		p_odd += num_bands * 3;
	    }
	  p_odd += width * num_bands;
      }
}

static void
do_copy4_int32 (int swap, const int *p_odd, int *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling an INT32 raster - scale 1:4 */
    unsigned int row;
    unsigned int col;
    int *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 2)
      {
	  for (col = 0; col < width; col += 4)
	    {
		if (swap)
		    *p_out++ = swapINT32 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 3;
	    }
	  p_odd += width;
      }
}

static void
do_copy4_uint32 (int swap, const unsigned int *p_odd, unsigned int *buf,
		 unsigned int width, unsigned int odd_rows)
{
/* reassembling an UINT32 raster - scale 1:4 */
    unsigned int row;
    unsigned int col;
    unsigned int *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 2)
      {
	  for (col = 0; col < width; col += 4)
	    {
		if (swap)
		    *p_out++ = swapUINT32 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 3;
	    }
	  p_odd += width;
      }
}

static void
do_copy4_float (int swap, const float *p_odd, float *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling a FLOAT raster - scale 1:4 */
    unsigned int row;
    unsigned int col;
    float *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 2)
      {
	  for (col = 0; col < width; col += 4)
	    {
		if (swap)
		    *p_out++ = swapFloat (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 3;
	    }
	  p_odd += width;
      }
}

static void
do_copy4_double (int swap, const double *p_odd, double *buf,
		 unsigned int width, unsigned int odd_rows)
{
/* reassembling a DOUBLE raster - scale 1:4 */
    unsigned int row;
    unsigned int col;
    double *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 2)
      {
	  for (col = 0; col < width; col += 4)
	    {
		if (swap)
		    *p_out++ = swapDouble (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 3;
	    }
	  p_odd += width;
      }
}

static int
build_pixel_buffer_4 (int swap, unsigned int *xwidth, unsigned int *xheight,
		      unsigned char sample_type, unsigned char pixel_size,
		      unsigned char num_bands, unsigned int odd_rows,
		      const void *pixels_odd, void **pixels, int *pixels_sz)
{
/* decoding the raster - scale 1:4 */
    unsigned short width = 0;
    unsigned short height = 0;
    unsigned int row;
    unsigned int col;
    void *buf;
    int buf_size;

    for (row = 0; row < *xheight; row += 4)
	height++;
    for (col = 0; col < *xwidth; col += 4)
	width++;

    buf_size = width * height * pixel_size * num_bands;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  do_copy4_int8 (pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT8:
	  do_copy4_uint8 (pixels_odd, buf, *xwidth, odd_rows, num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  do_copy4_int16 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT16:
	  do_copy4_uint16 (swap, pixels_odd, buf, *xwidth, odd_rows, num_bands);
	  break;
      case RL2_SAMPLE_INT32:
	  do_copy4_int32 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT32:
	  do_copy4_uint32 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_FLOAT:
	  do_copy4_float (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  do_copy4_double (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      };

    *xwidth = width;
    *xheight = height;
    *pixels = buf;
    *pixels_sz = buf_size;
    return 1;
}

static void
do_copy8_int8 (const char *p_odd, char *buf, unsigned int width,
	       unsigned int odd_rows)
{
/* reassembling an INT8 raster - scale 1:8 */
    unsigned int row;
    unsigned int col;
    char *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 4)
      {
	  for (col = 0; col < width; col += 8)
	    {
		*p_out++ = *p_odd++;
		p_odd += 7;
	    }
	  p_odd += 3 * width;
      }
}

static void
do_copy8_uint8 (const unsigned char *p_odd, unsigned char *buf,
		unsigned int width, unsigned int odd_rows,
		unsigned char num_bands)
{
/* reassembling a UINT8 raster - scale 1:8 */
    unsigned int row;
    unsigned int col;
    int band;
    unsigned char *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 4)
      {
	  for (col = 0; col < width; col += 8)
	    {
		for (band = 0; band < num_bands; band++)
		    *p_out++ = *p_odd++;
		p_odd += num_bands * 7;
	    }
	  p_odd += 3 * width * num_bands;
      }
}

static void
do_copy8_int16 (int swap, const short *p_odd, short *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling an INT16 raster - scale 1:8 */
    unsigned int row;
    unsigned int col;
    short *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 4)
      {
	  for (col = 0; col < width; col += 8)
	    {
		if (swap)
		    *p_out++ = swapINT16 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 7;
	    }
	  p_odd += 3 * width;
      }
}

static void
do_copy8_uint16 (int swap, const unsigned short *p_odd, unsigned short *buf,
		 unsigned int width, unsigned int odd_rows,
		 unsigned char num_bands)
{
/* reassembling a UINT16 raster - scale 1:8 */
    unsigned int row;
    unsigned int col;
    int band;
    unsigned short *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 4)
      {
	  for (col = 0; col < width; col += 8)
	    {
		for (band = 0; band < num_bands; band++)
		  {
		      if (swap)
			  *p_out++ = swapUINT16 (*p_odd++);
		      else
			  *p_out++ = *p_odd++;
		  }
		p_odd += num_bands * 7;
	    }
	  p_odd += 3 * width * num_bands;
      }
}

static void
do_copy8_int32 (int swap, const int *p_odd, int *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling an INT32 raster - scale 1:8 */
    unsigned int row;
    unsigned int col;
    int *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 4)
      {
	  for (col = 0; col < width; col += 8)
	    {
		if (swap)
		    *p_out++ = swapINT32 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 7;
	    }
	  p_odd += 3 * width;
      }
}

static void
do_copy8_uint32 (int swap, const unsigned int *p_odd, unsigned int *buf,
		 unsigned short width, unsigned short odd_rows)
{
/* reassembling an UINT32 raster - scale 1:8 */
    int row;
    int col;
    unsigned int *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 4)
      {
	  for (col = 0; col < width; col += 8)
	    {
		if (swap)
		    *p_out++ = swapUINT32 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 7;
	    }
	  p_odd += 3 * width;
      }
}

static void
do_copy8_float (int swap, const float *p_odd, float *buf, unsigned int width,
		unsigned int odd_rows)
{
/* reassembling a FLOAT raster - scale 1:8 */
    unsigned int row;
    unsigned int col;
    float *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 4)
      {
	  for (col = 0; col < width; col += 8)
	    {
		if (swap)
		    *p_out++ = swapFloat (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 7;
	    }
	  p_odd += 3 * width;
      }
}

static void
do_copy8_double (int swap, const double *p_odd, double *buf,
		 unsigned int width, unsigned int odd_rows)
{
/* reassembling a DOUBLE raster - scale 1:8 */
    unsigned int row;
    unsigned int col;
    double *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row += 4)
      {
	  for (col = 0; col < width; col += 8)
	    {
		if (swap)
		    *p_out++ = swapDouble (*p_odd++);
		else
		    *p_out++ = *p_odd++;
		p_odd += 7;
	    }
	  p_odd += 3 * width;
      }
}

static int
build_pixel_buffer_8 (int swap, unsigned int *xwidth, unsigned int *xheight,
		      unsigned char sample_type, unsigned char pixel_size,
		      unsigned char num_bands, unsigned short odd_rows,
		      const void *pixels_odd, void **pixels, int *pixels_sz)
{
/* decoding the raster - scale 1:8 */
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int row;
    unsigned int col;
    void *buf;
    int buf_size;

    for (row = 0; row < *xheight; row += 8)
	height++;
    for (col = 0; col < *xwidth; col += 8)
	width++;

    buf_size = width * height * pixel_size * num_bands;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  do_copy8_int8 (pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT8:
	  do_copy8_uint8 (pixels_odd, buf, *xwidth, odd_rows, num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  do_copy8_int16 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT16:
	  do_copy8_uint16 (swap, pixels_odd, buf, *xwidth, odd_rows, num_bands);
	  break;
      case RL2_SAMPLE_INT32:
	  do_copy8_int32 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_UINT32:
	  do_copy8_uint32 (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_FLOAT:
	  do_copy8_float (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  do_copy8_double (swap, pixels_odd, buf, *xwidth, odd_rows);
	  break;
      };

    *xwidth = width;
    *xheight = height;
    *pixels = buf;
    *pixels_sz = buf_size;
    return 1;
}

static void
do_copy_int8 (const char *p_odd, const char *p_even, char *buf,
	      unsigned short width, unsigned short odd_rows,
	      unsigned short even_rows)
{
/* reassembling an INT8 raster - scale 1:1 */
    int row;
    int col;
    char *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  /* Odd scanlines */
	  for (col = 0; col < width; col++)
	      *p_out++ = *p_odd++;
	  p_out += width;
      }

    p_out = buf;
    for (row = 0; row < even_rows; row++)
      {
	  /* Even scanlines */
	  p_out += width;
	  for (col = 0; col < width; col++)
	      *p_out++ = *p_even++;
      }
}

static void
do_copy_uint8 (const unsigned char *p_odd, const unsigned char *p_even,
	       unsigned char *buf, unsigned short width,
	       unsigned short odd_rows, unsigned short even_rows,
	       unsigned char num_bands)
{
/* reassembling a UINT8 raster - scale 1:1 */
    int row;
    int col;
    int band;
    unsigned char *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  /* Odd scanlines */
	  for (col = 0; col < width; col++)
	    {
		for (band = 0; band < num_bands; band++)
		    *p_out++ = *p_odd++;
	    }
	  p_out += width * num_bands;
      }

    p_out = buf;
    for (row = 0; row < even_rows; row++)
      {
	  /* Even scanlines */
	  p_out += width * num_bands;
	  for (col = 0; col < width; col++)
	    {
		for (band = 0; band < num_bands; band++)
		    *p_out++ = *p_even++;
	    }
      }
}

static void
do_copy_int16 (int swap, const short *p_odd, const short *p_even, short *buf,
	       unsigned short width, unsigned short odd_rows,
	       unsigned short even_rows)
{
/* reassembling an INT16 raster - scale 1:1 */
    int row;
    int col;
    short *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  /* Odd scanlines */
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapINT16 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
	    }
	  p_out += width;
      }

    p_out = buf;
    for (row = 0; row < even_rows; row++)
      {
	  /* Even scanlines */
	  p_out += width;
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapINT16 (*p_even++);
		else
		    *p_out++ = *p_even++;
	    }
      }
}

static void
do_copy_uint16 (int swap, const unsigned short *p_odd,
		const unsigned short *p_even, unsigned short *buf,
		unsigned short width, unsigned short odd_rows,
		unsigned short even_rows, unsigned char num_bands)
{
/* reassembling a UINT16 raster - scale 1:1 */
    int row;
    int col;
    int band;
    unsigned short *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  /* Odd scanlines */
	  for (col = 0; col < width; col++)
	    {
		for (band = 0; band < num_bands; band++)
		  {
		      if (swap)
			  *p_out++ = swapUINT16 (*p_odd++);
		      else
			  *p_out++ = *p_odd++;
		  }
	    }
	  p_out += width * num_bands;
      }

    p_out = buf;
    for (row = 0; row < even_rows; row++)
      {
	  /* Even scanlines */
	  p_out += width * num_bands;
	  for (col = 0; col < width; col++)
	    {
		for (band = 0; band < num_bands; band++)
		  {
		      if (swap)
			  *p_out++ = swapUINT16 (*p_even++);
		      else
			  *p_out++ = *p_even++;
		  }
	    }
      }
}

static void
do_copy_int32 (int swap, const int *p_odd, const int *p_even, int *buf,
	       unsigned short width, unsigned short odd_rows,
	       unsigned short even_rows)
{
/* reassembling an INT32 raster - scale 1:1 */
    int row;
    int col;
    int *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  /* Odd scanlines */
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapINT32 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
	    }
	  p_out += width;
      }

    p_out = buf;
    for (row = 0; row < even_rows; row++)
      {
	  /* Even scanlines */
	  p_out += width;
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapINT32 (*p_even++);
		else
		    *p_out++ = *p_even++;
	    }
      }
}

static void
do_copy_uint32 (int swap, const unsigned int *p_odd, const unsigned int *p_even,
		unsigned int *buf, unsigned short width,
		unsigned short odd_rows, unsigned short even_rows)
{
/* reassembling an UINT32 raster - scale 1:1 */
    int row;
    int col;
    unsigned int *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  /* Odd scanlines */
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapUINT32 (*p_odd++);
		else
		    *p_out++ = *p_odd++;
	    }
	  p_out += width;
      }

    p_out = buf;
    for (row = 0; row < even_rows; row++)
      {
	  /* Even scanlines */
	  p_out += width;
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapUINT32 (*p_even++);
		else
		    *p_out++ = *p_even++;
	    }
      }
}

static void
do_copy_float (int swap, const float *p_odd, const float *p_even, float *buf,
	       unsigned short width, unsigned short odd_rows,
	       unsigned short even_rows)
{
/* reassembling a FLOAT raster - scale 1:1 */
    int row;
    int col;
    float *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  /* Odd scanlines */
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapFloat (*p_odd++);
		else
		    *p_out++ = *p_odd++;
	    }
	  p_out += width;
      }

    p_out = buf;
    for (row = 0; row < even_rows; row++)
      {
	  /* Even scanlines */
	  p_out += width;
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapFloat (*p_even++);
		else
		    *p_out++ = *p_even++;
	    }
      }
}

static void
do_copy_double (int swap, const double *p_odd, const double *p_even,
		double *buf, unsigned short width, unsigned short odd_rows,
		unsigned short even_rows)
{
/* reassembling a DOUBLE raster - scale 1:1 */
    int row;
    int col;
    double *p_out;

    p_out = buf;
    for (row = 0; row < odd_rows; row++)
      {
	  /* Odd scanlines */
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapDouble (*p_odd++);
		else
		    *p_out++ = *p_odd++;
	    }
	  p_out += width;
      }

    p_out = buf;
    for (row = 0; row < even_rows; row++)
      {
	  /* Even scanlines */
	  p_out += width;
	  for (col = 0; col < width; col++)
	    {
		if (swap)
		    *p_out++ = swapDouble (*p_even++);
		else
		    *p_out++ = *p_even++;
	    }
      }
}

static int
build_pixel_buffer (int swap, int scale, unsigned int *xwidth,
		    unsigned int *xheight, unsigned char sample_type,
		    unsigned char num_bands, unsigned short odd_rows,
		    const void *pixels_odd, unsigned short even_rows,
		    const void *pixels_even, void **pixels, int *pixels_sz)
{
/* decoding the raster */
    unsigned int width = *xwidth;
    unsigned int height = *xheight;
    void *buf;
    int buf_size;
    unsigned char pixel_size;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pixel_size = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pixel_size = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pixel_size = 8;
	  break;
      default:
	  pixel_size = 1;
	  break;
      };

    if (scale == RL2_SCALE_2)
	return build_pixel_buffer_2 (swap, xwidth, xheight, sample_type,
				     pixel_size, num_bands, odd_rows,
				     pixels_odd, pixels, pixels_sz);
    if (scale == RL2_SCALE_4)
	return build_pixel_buffer_4 (swap, xwidth, xheight, sample_type,
				     pixel_size, num_bands, odd_rows,
				     pixels_odd, pixels, pixels_sz);
    if (scale == RL2_SCALE_8)
	return build_pixel_buffer_8 (swap, xwidth, xheight, sample_type,
				     pixel_size, num_bands, odd_rows,
				     pixels_odd, pixels, pixels_sz);

    buf_size = width * height * pixel_size * num_bands;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  do_copy_int8 (pixels_odd, pixels_even, buf, width, odd_rows,
			even_rows);
	  break;
      case RL2_SAMPLE_UINT8:
	  do_copy_uint8 (pixels_odd, pixels_even, buf, width, odd_rows,
			 even_rows, num_bands);
	  break;
      case RL2_SAMPLE_INT16:
	  do_copy_int16 (swap, pixels_odd, pixels_even, buf, width, odd_rows,
			 even_rows);
	  break;
      case RL2_SAMPLE_UINT16:
	  do_copy_uint16 (swap, pixels_odd, pixels_even, buf, width, odd_rows,
			  even_rows, num_bands);
	  break;
      case RL2_SAMPLE_INT32:
	  do_copy_int32 (swap, pixels_odd, pixels_even, buf, width, odd_rows,
			 even_rows);
	  break;
      case RL2_SAMPLE_UINT32:
	  do_copy_uint32 (swap, pixels_odd, pixels_even, buf, width, odd_rows,
			  even_rows);
	  break;
      case RL2_SAMPLE_FLOAT:
	  do_copy_float (swap, pixels_odd, pixels_even, buf, width, odd_rows,
			 even_rows);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  do_copy_double (swap, pixels_odd, pixels_even, buf, width, odd_rows,
			  even_rows);
	  break;
      };

    *pixels = buf;
    *pixels_sz = buf_size;
    return 1;
}

static int
rescale_mask_2 (unsigned short *xwidth, unsigned short *xheight,
		const unsigned char *mask_pix, unsigned char **mask,
		int *mask_sz)
{
/* rescaling a transparency mask - scale 1:2 */
    int row;
    int col;
    unsigned short width = *xwidth / 2.0;
    unsigned short height = *xheight / 2.0;
    unsigned char *p_out;
    const unsigned char *p_in = mask_pix;

    if ((width * 2) < *xwidth)
	width++;
    if ((height * 2) < *xheight)
	height++;
    *mask_sz = width * height;
    *mask = malloc (*mask_sz);
    if (*mask == NULL)
	return 0;
    p_out = *mask;

    for (row = 0; row < *xheight; row += 2)
      {
	  for (col = 0; col < *xwidth; col += 2)
	    {
		*p_out++ = *p_in++;
		p_in++;
	    }
	  p_in += *xwidth;
      }

    *xwidth = width;
    *xheight = height;
    return 1;
}

static int
rescale_mask_4 (unsigned short *xwidth, unsigned short *xheight,
		const unsigned char *mask_pix, unsigned char **mask,
		int *mask_sz)
{
/* rescaling a transparency mask - scale 1:4 */
    int row;
    int col;
    unsigned short width = *xwidth / 4.0;
    unsigned short height = *xheight / 4.0;
    unsigned char *p_out;
    const unsigned char *p_in = mask_pix;

    if ((width * 4) < *xwidth)
	width++;
    if ((height * 4) < *xheight)
	height++;
    *mask_sz = width * height;
    *mask = malloc (*mask_sz);
    if (*mask == NULL)
	return 0;
    p_out = *mask;

    for (row = 0; row < *xheight; row += 4)
      {
	  for (col = 0; col < *xwidth; col += 4)
	    {
		*p_out++ = *p_in++;
		p_in += 3;
	    }
	  p_in += *xwidth * 3;
      }

    *xwidth = width;
    *xheight = height;
    return 1;
}

static int
rescale_mask_8 (unsigned short *xwidth, unsigned short *xheight,
		const unsigned char *mask_pix, unsigned char **mask,
		int *mask_sz)
{
/* rescaling a transparency mask - scale 1:8 */
    int row;
    int col;
    unsigned short width = *xwidth / 8.0;
    unsigned short height = *xheight / 8.0;
    unsigned char *p_out;
    const unsigned char *p_in = mask_pix;

    if ((width * 8) < *xwidth)
	width++;
    if ((height * 8) < *xheight)
	height++;
    *mask_sz = width * height;
    *mask = malloc (*mask_sz);
    if (*mask == NULL)
	return 0;
    p_out = *mask;

    for (row = 0; row < *xheight; row += 8)
      {
	  for (col = 0; col < *xwidth; col += 8)
	    {
		*p_out++ = *p_in++;
		p_in += 7;
	    }
	  p_in += *xwidth * 7;
      }

    *xwidth = width;
    *xheight = height;
    return 1;
}

static int
rescale_mask (int scale, unsigned short *xwidth, unsigned short *xheight,
	      unsigned char *mask_pix, unsigned char **mask, int *mask_sz)
{
/* rescaling the transparency mask */
    unsigned short width = *xwidth;
    unsigned short height = *xheight;
    unsigned char *buf;
    int buf_size;

    if (scale == RL2_SCALE_2)
	return rescale_mask_2 (xwidth, xheight, mask_pix, mask, mask_sz);
    if (scale == RL2_SCALE_4)
	return rescale_mask_4 (xwidth, xheight, mask_pix, mask, mask_sz);
    if (scale == RL2_SCALE_8)
	return rescale_mask_8 (xwidth, xheight, mask_pix, mask, mask_sz);

    buf_size = width * height;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    memcpy (buf, mask_pix, buf_size);
    *mask = buf;
    *mask_sz = buf_size;
    return 1;
}

static int
unpack_1bit (unsigned short width, unsigned short height,
	     unsigned short row_stride, const unsigned char *pixels_in,
	     unsigned char **pixels, int *pixels_sz)
{
/* unpacking a 1-BIT raster */
    unsigned char *buf;
    int buf_size;
    int row;
    int col;
    unsigned char *p_out;
    const unsigned char *p_in;

    buf_size = width * height;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    p_in = pixels_in;
    p_out = buf;
    for (row = 0; row < height; row++)
      {
	  int x = 0;
	  for (col = 0; col < row_stride; col++)
	    {
		unsigned char byte = *p_in++;
		if ((byte & 0x80) == 0x80)
		    *p_out++ = 1;
		else
		    *p_out++ = 0;
		x++;
		if (x >= width)
		    break;
		if ((byte & 0x40) == 0x40)
		    *p_out++ = 1;
		else
		    *p_out++ = 0;
		x++;
		if (x >= width)
		    break;
		if ((byte & 0x20) == 0x20)
		    *p_out++ = 1;
		else
		    *p_out++ = 0;
		x++;
		if (x >= width)
		    break;
		if ((byte & 0x10) == 0x10)
		    *p_out++ = 1;
		else
		    *p_out++ = 0;
		x++;
		if (x >= width)
		    break;
		if ((byte & 0x08) == 0x08)
		    *p_out++ = 1;
		else
		    *p_out++ = 0;
		if ((byte & 0x04) == 0x04)
		    *p_out++ = 1;
		else
		    *p_out++ = 0;
		x++;
		if (x >= width)
		    break;
		if ((byte & 0x02) == 0x02)
		    *p_out++ = 1;
		else
		    *p_out++ = 0;
		x++;
		if (x >= width)
		    break;
		if ((byte & 0x01) == 0x01)
		    *p_out++ = 1;
		else
		    *p_out++ = 0;
	    }
      }

    *pixels = buf;
    *pixels_sz = buf_size;
    return 1;
}

static int
unpack_2bit (unsigned short width, unsigned short height,
	     unsigned short row_stride, const unsigned char *pixels_in,
	     unsigned char **pixels, int *pixels_sz)
{
/* unpacking a 2-BIT raster */
    void *buf;
    int buf_size;
    int row;
    int col;
    unsigned char *p_out;
    const unsigned char *p_in;

    buf_size = width * height;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    p_in = pixels_in;
    p_out = buf;
    for (row = 0; row < height; row++)
      {
	  int x = 0;
	  for (col = 0; col < row_stride; col++)
	    {
		unsigned char byte = *p_in++;
		unsigned char hi0 = byte & 0xc0;
		unsigned char hi1 = byte & 0x30;
		unsigned char lo0 = byte & 0x0c;
		unsigned char lo1 = byte & 0x03;
		switch (hi0)
		  {
		  case 0x00:
		      *p_out++ = 0;
		      break;
		  case 0x40:
		      *p_out++ = 1;
		      break;
		  case 0x80:
		      *p_out++ = 2;
		      break;
		  case 0xc0:
		      *p_out++ = 3;
		      break;
		  };
		x++;
		if (x >= width)
		    break;
		switch (hi1)
		  {
		  case 0x00:
		      *p_out++ = 0;
		      break;
		  case 0x10:
		      *p_out++ = 1;
		      break;
		  case 0x20:
		      *p_out++ = 2;
		      break;
		  case 0x30:
		      *p_out++ = 3;
		      break;
		  };
		x++;
		if (x >= width)
		    break;
		switch (lo0)
		  {
		  case 0x00:
		      *p_out++ = 0;
		      break;
		  case 0x04:
		      *p_out++ = 1;
		      break;
		  case 0x08:
		      *p_out++ = 2;
		      break;
		  case 0x0c:
		      *p_out++ = 3;
		      break;
		  };
		x++;
		if (x >= width)
		    break;
		switch (lo1)
		  {
		  case 0x00:
		      *p_out++ = 0;
		      break;
		  case 0x01:
		      *p_out++ = 1;
		      break;
		  case 0x02:
		      *p_out++ = 2;
		      break;
		  case 0x03:
		      *p_out++ = 3;
		      break;
		  };
		x++;
	    }
      }

    *pixels = buf;
    *pixels_sz = buf_size;
    return 1;
}

static int
unpack_4bit (unsigned short width, unsigned short height,
	     unsigned short row_stride, const unsigned char *pixels_in,
	     unsigned char **pixels, int *pixels_sz)
{
/* unpacking a 4-BIT raster */
    void *buf;
    int buf_size;
    int row;
    int col;
    unsigned char *p_out;
    const unsigned char *p_in;

    buf_size = width * height;
    buf = malloc (buf_size);
    if (buf == NULL)
	return 0;

    p_in = pixels_in;
    p_out = buf;
    for (row = 0; row < height; row++)
      {
	  int x = 0;
	  for (col = 0; col < row_stride; col++)
	    {
		unsigned char byte = *p_in++;
		unsigned char hi = byte & 0xf0;
		unsigned char lo = byte & 0x0f;
		switch (hi)
		  {
		  case 0x00:
		      *p_out++ = 0;
		      break;
		  case 0x10:
		      *p_out++ = 1;
		      break;
		  case 0x20:
		      *p_out++ = 2;
		      break;
		  case 0x30:
		      *p_out++ = 3;
		      break;
		  case 0x40:
		      *p_out++ = 4;
		      break;
		  case 0x50:
		      *p_out++ = 5;
		      break;
		  case 0x60:
		      *p_out++ = 6;
		      break;
		  case 0x70:
		      *p_out++ = 7;
		      break;
		  case 0x80:
		      *p_out++ = 8;
		      break;
		  case 0x90:
		      *p_out++ = 9;
		      break;
		  case 0xa0:
		      *p_out++ = 10;
		      break;
		  case 0xb0:
		      *p_out++ = 11;
		      break;
		  case 0xc0:
		      *p_out++ = 12;
		      break;
		  case 0xd0:
		      *p_out++ = 13;
		      break;
		  case 0xe0:
		      *p_out++ = 14;
		      break;
		  case 0xf0:
		      *p_out++ = 15;
		      break;
		  };
		x++;
		if (x >= width)
		    break;
		switch (lo)
		  {
		  case 0x00:
		      *p_out++ = 0;
		      break;
		  case 0x01:
		      *p_out++ = 1;
		      break;
		  case 0x02:
		      *p_out++ = 2;
		      break;
		  case 0x03:
		      *p_out++ = 3;
		      break;
		  case 0x04:
		      *p_out++ = 4;
		      break;
		  case 0x05:
		      *p_out++ = 5;
		      break;
		  case 0x06:
		      *p_out++ = 6;
		      break;
		  case 0x07:
		      *p_out++ = 7;
		      break;
		  case 0x08:
		      *p_out++ = 8;
		      break;
		  case 0x09:
		      *p_out++ = 9;
		      break;
		  case 0x0a:
		      *p_out++ = 10;
		      break;
		  case 0x0b:
		      *p_out++ = 11;
		      break;
		  case 0x0c:
		      *p_out++ = 12;
		      break;
		  case 0x0d:
		      *p_out++ = 13;
		      break;
		  case 0x0e:
		      *p_out++ = 14;
		      break;
		  case 0x0f:
		      *p_out++ = 15;
		      break;
		  };
		x++;
	    }
      }

    *pixels = buf;
    *pixels_sz = buf_size;
    return 1;
}

RL2_DECLARE int
rl2_is_valid_dbms_raster_tile (unsigned short level, unsigned int tile_width,
			       unsigned int tile_height,
			       const unsigned char *blob_odd, int blob_odd_sz,
			       const unsigned char *blob_even, int blob_even_sz,
			       unsigned char sample_type,
			       unsigned char pixel_type,
			       unsigned char num_bands,
			       unsigned char compression)
{
/* testing a serialized Raster Tile object for validity */
    unsigned int width;
    unsigned int height;
    unsigned char xsample_type;
    unsigned char xpixel_type;
    unsigned char xnum_bands;
    unsigned char xcompression;
    uLong crc;
    if (!check_blob_odd
	(blob_odd, blob_odd_sz, &width, &height, &xsample_type, &xpixel_type,
	 &xnum_bands, &xcompression, &crc))
	return RL2_ERROR;
    if (blob_even != NULL)
      {
	  if (!check_blob_even
	      (blob_even, blob_even_sz, width, height, xsample_type,
	       xpixel_type, xnum_bands, xcompression, crc))
	      return RL2_ERROR;
      }
    if (width != tile_width || height != tile_height)
	return RL2_ERROR;
    if (level == 0)
      {
	  /* base-level tile */
	  if (sample_type == xsample_type && pixel_type == xpixel_type
	      && num_bands == xnum_bands && compression == xcompression)
	      return RL2_OK;
      }
    else
      {
	  /* Pyramid-level tile */
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_MONOCHROME && num_bands == 1))
	    {
		/* MONOCHROME: expecting a GRAYSCALE/PNG Pyramid tile */
		if (xsample_type == RL2_SAMPLE_UINT8
		    && xpixel_type == RL2_PIXEL_GRAYSCALE && xnum_bands == 1
		    && xcompression == RL2_COMPRESSION_PNG)
		    return RL2_OK;
	    }
	  if ((sample_type == RL2_SAMPLE_1_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_2_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1) ||
	      (sample_type == RL2_SAMPLE_4_BIT
	       && pixel_type == RL2_PIXEL_PALETTE && num_bands == 1))
	    {
		/* small-PALETTE: expecting an RGB/PNG Pyramid tile */
		if (xsample_type == RL2_SAMPLE_UINT8
		    && xpixel_type == RL2_PIXEL_RGB && xnum_bands == 3
		    && xcompression == RL2_COMPRESSION_PNG)
		    return RL2_OK;
	    }
	  if (sample_type == RL2_SAMPLE_UINT8 && pixel_type == RL2_PIXEL_PALETTE
	      && num_bands == 1)
	    {
		/* PALETTE 8bits: expecting an RGB/PNG Pyramid tile */
		if (xsample_type == RL2_SAMPLE_UINT8
		    && xpixel_type == RL2_PIXEL_RGB && xnum_bands == 3
		    && xcompression == RL2_COMPRESSION_PNG)
		    return RL2_OK;
	    }
	  /* any other: expecting unchanged params */
	  if (xsample_type == sample_type
	      && xpixel_type == pixel_type && xnum_bands == num_bands
	      && xcompression == compression)
	      return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE rl2RasterPtr
rl2_raster_decode (int scale, const unsigned char *blob_odd,
		   int blob_odd_sz, const unsigned char *blob_even,
		   int blob_even_sz, rl2PalettePtr ext_palette)
{
/* decoding from internal RL2 binary format to Raster */
    rl2RasterPtr raster;
    rl2PalettePtr palette = NULL;
    rl2PalettePtr palette2 = NULL;
    unsigned int width;
    unsigned int height;
    unsigned short mask_width = 0;
    unsigned short mask_height = 0;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char compression;
    unsigned short row_stride_odd;
    unsigned int odd_rows;
    unsigned int even_rows;
    int uncompressed_odd;
    int compressed_odd;
    int uncompressed_mask;
    int compressed_mask;
    int uncompressed_even = 0;
    int compressed_even = 0;
    uLong crc;
    const unsigned char *pixels_odd;
    const unsigned char *pixels_even;
    const unsigned char *pixels_mask = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz;
    unsigned char *mask = NULL;
    int mask_sz = 0;
    const unsigned char *ptr;
    unsigned char *odd_data = NULL;
    unsigned char *even_data = NULL;
    unsigned char *odd_mask = NULL;
    unsigned char *even_mask = NULL;
    int odd_mask_sz;
    int even_mask_sz;
    int swap;
    int endian;
    int endian_arch = endianArch ();
    int delta_dist;

    if (blob_odd == NULL)
	return NULL;
    if (!check_blob_odd
	(blob_odd, blob_odd_sz, &width, &height, &sample_type, &pixel_type,
	 &num_bands, &compression, &crc))
	return NULL;
    if (blob_even != NULL)
      {
	  if (!check_blob_even
	      (blob_even, blob_even_sz, width, height, sample_type, pixel_type,
	       num_bands, compression, crc))
	      return NULL;
      }
    if (!check_scale (scale, sample_type, compression, blob_even))
	return NULL;

    switch (pixel_type)
      {
      case RL2_PIXEL_RGB:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT16:
		delta_dist = 6;
		break;
	    default:
		delta_dist = 3;
		break;
	    };
	  break;
      case RL2_PIXEL_MULTIBAND:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT16:
		delta_dist = num_bands * 2;
		break;
	    default:
		delta_dist = num_bands;
		break;
	    };
	  break;
      case RL2_PIXEL_DATAGRID:
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_UINT16:
		delta_dist = 2;
		break;
	    case RL2_SAMPLE_INT32:
	    case RL2_SAMPLE_UINT32:
	    case RL2_SAMPLE_FLOAT:
		delta_dist = 4;
		break;
	    case RL2_SAMPLE_DOUBLE:
		delta_dist = 8;
		break;
	    default:
		delta_dist = 1;
		break;
	    };
	  break;
      default:
	  delta_dist = 1;
	  break;
      };

    endian = *(blob_odd + 2);
    num_bands = *(blob_odd + 6);
    ptr = blob_odd + 11;
    row_stride_odd = importU16 (ptr, endian, endian_arch);
    ptr += 2;
    odd_rows = importU16 (ptr, endian, endian_arch);
    ptr += 2;
    uncompressed_odd = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    compressed_odd = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    uncompressed_mask = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    compressed_mask = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (*ptr++ != RL2_DATA_START)
	return NULL;
    pixels_odd = ptr;
    if (uncompressed_mask > 0)
      {
	  /* retrieving the mask */
	  ptr += compressed_odd;
	  if (*ptr++ != RL2_DATA_END)
	      return NULL;
	  if (*ptr++ != RL2_MASK_START)
	      return NULL;
	  pixels_mask = ptr;
	  mask_width = width;
	  mask_height = height;
	  ptr += compressed_mask;
	  if (*ptr++ != RL2_MASK_END)
	      return NULL;
      }
    if (blob_even != NULL)
      {
	  ptr = blob_even + 11;
	  even_rows = importU16 (ptr, endian, endian_arch);
	  ptr += 2;
	  ptr += 4;		/* skipping OddBlock CRC */
	  uncompressed_even = importU32 (ptr, endian, endian_arch);
	  ptr += 4;
	  compressed_even = importU32 (ptr, endian, endian_arch);
	  ptr += 4;
	  if (*ptr++ != RL2_DATA_START)
	      return NULL;
	  pixels_even = ptr;
      }
    else
      {
	  compressed_even = 0;
	  uncompressed_even = 0;
	  pixels_even = NULL;
      }

    if (compression == RL2_COMPRESSION_DEFLATE
	&& uncompressed_odd != compressed_odd)
      {
	  /* decompressing from ZIP DeltaFilter [Deflate] - ODD Block */
	  uLong refLen = uncompressed_odd;
	  const Bytef *in = pixels_odd;
	  odd_data = malloc (uncompressed_odd);
	  if (odd_data == NULL)
	      goto error;
	  if (uncompress (odd_data, &refLen, in, compressed_odd) != Z_OK)
	      goto error;
	  if (rl2_delta_decode (odd_data, uncompressed_odd, delta_dist) !=
	      RL2_OK)
	      goto error;
	  pixels_odd = odd_data;
	  if (pixels_even != NULL && uncompressed_even != compressed_even)
	    {
		/* decompressing from ZIP DeltaFilter [Deflate] - EVEN Block */
		uLong refLen = uncompressed_even;
		const Bytef *in = pixels_even;
		even_data = malloc (uncompressed_even);
		if (even_data == NULL)
		    goto error;
		if (uncompress (even_data, &refLen, in, compressed_even) !=
		    Z_OK)
		    goto error;
		if (rl2_delta_decode (even_data, uncompressed_even, delta_dist)
		    != RL2_OK)
		    goto error;
		pixels_even = even_data;
	    }
      }
    if (compression == RL2_COMPRESSION_DEFLATE_NO
	&& uncompressed_odd != compressed_odd)
      {
	  /* decompressing from ZIP noDelta [Deflate] - ODD Block */
	  uLong refLen = uncompressed_odd;
	  const Bytef *in = pixels_odd;
	  odd_data = malloc (uncompressed_odd);
	  if (odd_data == NULL)
	      goto error;
	  if (uncompress (odd_data, &refLen, in, compressed_odd) != Z_OK)
	      goto error;
	  pixels_odd = odd_data;
	  if (pixels_even != NULL && uncompressed_even != compressed_even)
	    {
		/* decompressing from ZIP noDelta [Deflate] - EVEN Block */
		uLong refLen = uncompressed_even;
		const Bytef *in = pixels_even;
		even_data = malloc (uncompressed_even);
		if (even_data == NULL)
		    goto error;
		if (uncompress (even_data, &refLen, in, compressed_even) !=
		    Z_OK)
		    goto error;
		pixels_even = even_data;
	    }
      }
    if (compression == RL2_COMPRESSION_LZMA
	&& uncompressed_odd != compressed_odd)
      {
#ifndef OMIT_LZMA		/* only if LZMA is enabled */
	  /* decompressing from LZMA DeltaFilter - ODD Block */
	  lzma_options_lzma opt_lzma2;
	  lzma_options_delta opt_delta;
	  lzma_filter filters[3];
	  size_t in_pos = 0;
	  size_t out_pos = 0;
	  size_t refLen = compressed_odd;
	  odd_data = malloc (uncompressed_odd);
	  if (odd_data == NULL)
	      goto error;
	  lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
	  opt_delta.type = LZMA_DELTA_TYPE_BYTE;
	  opt_delta.dist = delta_dist;
	  lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
	  filters[0].id = LZMA_FILTER_DELTA;
	  filters[0].options = &opt_delta;
	  filters[1].id = LZMA_FILTER_LZMA2;
	  filters[1].options = &opt_lzma2;
	  filters[2].id = LZMA_VLI_UNKNOWN;
	  filters[2].options = NULL;
	  if (lzma_raw_buffer_decode
	      (filters, NULL, pixels_odd, &in_pos, refLen, odd_data, &out_pos,
	       uncompressed_odd) != LZMA_OK)
	      goto error;
	  pixels_odd = odd_data;
	  if (pixels_even != NULL && uncompressed_even != compressed_even)
	    {
		/* decompressing from LZMA DeltaFilter - EVEN Block */
		lzma_options_lzma opt_lzma2;
		lzma_options_delta opt_delta;
		lzma_filter filters[3];
		size_t in_pos = 0;
		size_t out_pos = 0;
		size_t refLen = compressed_even;
		even_data = malloc (uncompressed_even);
		if (even_data == NULL)
		    goto error;
		lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
		opt_delta.type = LZMA_DELTA_TYPE_BYTE;
		opt_delta.dist = delta_dist;
		lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
		filters[0].id = LZMA_FILTER_DELTA;
		filters[0].options = &opt_delta;
		filters[1].id = LZMA_FILTER_LZMA2;
		filters[1].options = &opt_lzma2;
		filters[2].id = LZMA_VLI_UNKNOWN;
		filters[2].options = NULL;
		if (lzma_raw_buffer_decode
		    (filters, NULL, pixels_even, &in_pos, refLen, even_data,
		     &out_pos, uncompressed_even) != LZMA_OK)
		    goto error;
		pixels_even = even_data;
	    }
#else /* LZMA is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling LZMA support\n");
	  goto error;
#endif /* end LZMA conditional */
      }
    if (compression == RL2_COMPRESSION_LZMA_NO
	&& uncompressed_odd != compressed_odd)
      {
#ifndef OMIT_LZMA		/* only if LZMA is enabled */
	  /* decompressing from LZMA noDelta - ODD Block */
	  lzma_options_lzma opt_lzma2;
	  lzma_filter filters[2];
	  size_t in_pos = 0;
	  size_t out_pos = 0;
	  size_t refLen = compressed_odd;
	  odd_data = malloc (uncompressed_odd);
	  if (odd_data == NULL)
	      goto error;
	  lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
	  lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
	  filters[0].id = LZMA_FILTER_LZMA2;
	  filters[0].options = &opt_lzma2;
	  filters[1].id = LZMA_VLI_UNKNOWN;
	  filters[1].options = NULL;
	  if (lzma_raw_buffer_decode
	      (filters, NULL, pixels_odd, &in_pos, refLen, odd_data, &out_pos,
	       uncompressed_odd) != LZMA_OK)
	      goto error;
	  pixels_odd = odd_data;
	  if (pixels_even != NULL && uncompressed_even != compressed_even)
	    {
		/* decompressing from LZMA noDelta - EVEN Block */
		lzma_options_lzma opt_lzma2;
		lzma_filter filters[3];
		size_t in_pos = 0;
		size_t out_pos = 0;
		size_t refLen = compressed_even;
		even_data = malloc (uncompressed_even);
		if (even_data == NULL)
		    goto error;
		lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
		lzma_lzma_preset (&opt_lzma2, LZMA_PRESET_DEFAULT);
		filters[0].id = LZMA_FILTER_LZMA2;
		filters[0].options = &opt_lzma2;
		filters[1].id = LZMA_VLI_UNKNOWN;
		filters[1].options = NULL;
		if (lzma_raw_buffer_decode
		    (filters, NULL, pixels_even, &in_pos, refLen, even_data,
		     &out_pos, uncompressed_even) != LZMA_OK)
		    goto error;
		pixels_even = even_data;
	    }
#else /* LZMA is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling LZMA support\n");
	  goto error;
#endif /* end LZMA conditional */
      }
    if (compression == RL2_COMPRESSION_JPEG)
      {
	  /* decompressing from JPEG - always on the ODD Block */
	  int ret = RL2_ERROR;
	  unsigned char pix_typ;
	  switch (scale)
	    {
	    case RL2_SCALE_1:
		ret =
		    rl2_decode_jpeg_scaled (1, pixels_odd, compressed_odd,
					    &width, &height, &pix_typ, &pixels,
					    &pixels_sz);
		break;
	    case RL2_SCALE_2:
		ret =
		    rl2_decode_jpeg_scaled (2, pixels_odd, compressed_odd,
					    &width, &height, &pix_typ, &pixels,
					    &pixels_sz);
		break;
	    case RL2_SCALE_4:
		ret =
		    rl2_decode_jpeg_scaled (4, pixels_odd, compressed_odd,
					    &width, &height, &pix_typ, &pixels,
					    &pixels_sz);
		break;
	    case RL2_SCALE_8:
		ret =
		    rl2_decode_jpeg_scaled (8, pixels_odd, compressed_odd,
					    &width, &height, &pix_typ, &pixels,
					    &pixels_sz);
		break;
	    };
	  if (ret != RL2_OK)
	      goto error;
	  goto done;
      }
    if (compression == RL2_COMPRESSION_LOSSY_WEBP
	|| compression == RL2_COMPRESSION_LOSSLESS_WEBP)
      {
#ifndef OMIT_WEBP		/* only if WebP is enabled */
	  /* decompressing from WEBP - always on the ODD Block */
	  int ret = RL2_ERROR;
	  switch (scale)
	    {
	    case RL2_SCALE_1:
		ret =
		    rl2_decode_webp_scaled (1, pixels_odd, compressed_odd,
					    &width, &height, pixel_type,
					    &pixels, &pixels_sz, &mask,
					    &mask_sz);
		break;
	    case RL2_SCALE_2:
		ret =
		    rl2_decode_webp_scaled (2, pixels_odd, compressed_odd,
					    &width, &height, pixel_type,
					    &pixels, &pixels_sz, &mask,
					    &mask_sz);
		break;
	    case RL2_SCALE_4:
		ret =
		    rl2_decode_webp_scaled (4, pixels_odd, compressed_odd,
					    &width, &height, pixel_type,
					    &pixels, &pixels_sz, &mask,
					    &mask_sz);
		break;
	    case RL2_SCALE_8:
		ret =
		    rl2_decode_webp_scaled (8, pixels_odd, compressed_odd,
					    &width, &height, pixel_type,
					    &pixels, &pixels_sz, &mask,
					    &mask_sz);
		break;
	    };
	  if (ret != RL2_OK)
	      goto error;
	  goto done;
#else /* WebP is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling WebP support\n");
	  goto error;
#endif /* end WebP conditional */
      }
    if (compression == RL2_COMPRESSION_PNG)
      {
	  /* decompressing from PNG */
	  int ret;
	  if (sample_type == RL2_SAMPLE_1_BIT || sample_type == RL2_SAMPLE_2_BIT
	      || sample_type == RL2_SAMPLE_4_BIT)
	    {
		/* Palette or Grayscale - 1,2 or 4 bit isn't scalable */
		if (scale != RL2_SCALE_1)
		    goto error;
		ret =
		    rl2_decode_png (pixels_odd, compressed_odd,
				    &width, &height, &sample_type, &pixel_type,
				    &num_bands, &pixels, &pixels_sz, &mask,
				    &mask_sz, &palette, 0);
		if (ret != RL2_OK)
		    goto error;
		goto done;
	    }
	  else
	    {
		ret = rl2_decode_png (pixels_odd, compressed_odd,
				      &width, &odd_rows, &sample_type,
				      &pixel_type, &num_bands, &odd_data,
				      &pixels_sz, &odd_mask, &odd_mask_sz,
				      &palette, 0);
		if (ret != RL2_OK)
		    goto error;
		pixels_odd = odd_data;
		if (scale == RL2_SCALE_1)
		  {
		      ret = rl2_decode_png (pixels_even, compressed_even,
					    &width, &even_rows, &sample_type,
					    &pixel_type, &num_bands, &even_data,
					    &pixels_sz, &even_mask,
					    &even_mask_sz, &palette2, 0);
		      if (ret != RL2_OK)
			  goto error;
		      rl2_destroy_palette (palette2);
		  }
		pixels_even = even_data;
		if (odd_mask != NULL)
		    free (odd_mask);
		if (even_mask != NULL)
		    free (even_mask);
		goto merge;
	    }
      }
    if (compression == RL2_COMPRESSION_CHARLS)
      {
#ifndef OMIT_CHARLS		/* only if CharLS is enabled */
	  /* decompressing from CHARLS */
	  int ret = rl2_decode_charls (pixels_odd, compressed_odd,
				       &width, &odd_rows, &sample_type,
				       &pixel_type, &num_bands, &odd_data,
				       &pixels_sz);
	  if (ret != RL2_OK)
	      goto error;
	  pixels_odd = odd_data;
	  if (scale == RL2_SCALE_1)
	    {
		ret = rl2_decode_charls (pixels_even, compressed_even,
					 &width, &even_rows, &sample_type,
					 &pixel_type, &num_bands, &even_data,
					 &pixels_sz);
		if (ret != RL2_OK)
		    goto error;
	    }
	  pixels_even = even_data;
	  goto merge;
#else /* CharLS is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling CharLS support\n");
	  goto error;
#endif /* end CharLS conditional */
      }
    if (compression == RL2_COMPRESSION_CCITTFAX4)
      {
	  /* decompressing from TIFF FAX4 */
	  int ret;
	  if (sample_type == RL2_SAMPLE_1_BIT && scale == RL2_SCALE_1)
	      ;
	  else
	      goto error;
	  ret =
	      rl2_decode_tiff_mono4 (pixels_odd, compressed_odd,
				     &width, &height, &pixels, &pixels_sz);
	  if (ret != RL2_OK)
	      goto error;
	  sample_type = RL2_SAMPLE_1_BIT;
	  pixel_type = RL2_PIXEL_MONOCHROME;
	  num_bands = 1;
	  goto done;
      }
    if (compression == RL2_COMPRESSION_LOSSY_JP2
	|| compression == RL2_COMPRESSION_LOSSLESS_JP2)
      {
#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
	  /* decompressing from Jpeg2000 - always on the ODD Block */
	  int ret = RL2_ERROR;
	  switch (scale)
	    {
	    case RL2_SCALE_1:
		ret =
		    rl2_decode_jpeg2000_scaled (1, pixels_odd, compressed_odd,
						&width, &height, sample_type,
						pixel_type, num_bands, &pixels,
						&pixels_sz);
		break;
	    case RL2_SCALE_2:
		ret =
		    rl2_decode_jpeg2000_scaled (2, pixels_odd, compressed_odd,
						&width, &height, sample_type,
						pixel_type, num_bands, &pixels,
						&pixels_sz);
		break;
	    case RL2_SCALE_4:
		ret =
		    rl2_decode_jpeg2000_scaled (4, pixels_odd, compressed_odd,
						&width, &height, sample_type,
						pixel_type, num_bands, &pixels,
						&pixels_sz);
		break;
	    case RL2_SCALE_8:
		ret =
		    rl2_decode_jpeg2000_scaled (8, pixels_odd, compressed_odd,
						&width, &height, sample_type,
						pixel_type, num_bands, &pixels,
						&pixels_sz);
		break;
	    };
	  if (ret != RL2_OK)
	      goto error;
	  goto done;
#else /* OpenJpeg is disabled */
	  fprintf (stderr,
		   "librasterlite2 was built by disabling OpenJpeg support\n");
	  goto error;
#endif /* end OpenJpeg conditional */
      }

    if (sample_type == RL2_SAMPLE_1_BIT)
      {
	  if (!unpack_1bit
	      (width, height, row_stride_odd, pixels_odd, &pixels, &pixels_sz))
	      goto error;
      }
    else if (sample_type == RL2_SAMPLE_2_BIT)
      {
	  if (!unpack_2bit
	      (width, height, row_stride_odd, pixels_odd, &pixels, &pixels_sz))
	      goto error;
      }
    else if (sample_type == RL2_SAMPLE_4_BIT)
      {
	  if (!unpack_4bit
	      (width, height, row_stride_odd, pixels_odd, &pixels, &pixels_sz))
	      goto error;
      }
    else
      {
	merge:
	  if (endian != endian_arch)
	      swap = 1;
	  else
	      swap = 0;
	  if (!build_pixel_buffer
	      (swap, scale, &width, &height, sample_type, num_bands, odd_rows,
	       pixels_odd, even_rows, pixels_even, (void **) (&pixels),
	       &pixels_sz))
	      goto error;
      }

  done:
    if (pixels_mask != NULL)
      {
	  /* unpacking the mask */
	  unsigned char *mask_pix;
	  int mask_pix_sz;
	  if (uncompressed_mask != (mask_width * mask_height))
	      goto error;
	  if (!unpack_rle
	      (mask_width, mask_height, pixels_mask, compressed_mask, &mask_pix,
	       &mask_pix_sz))
	      goto error;
	  if (!rescale_mask
	      (scale, &mask_width, &mask_height, mask_pix, &mask, &mask_sz))
	    {
		free (mask_pix);
		goto error;
	    }
	  free (mask_pix);
      }

    if (palette == NULL)
      {
	  palette = ext_palette;
	  ext_palette = NULL;
      }
    else
      {
	  if (ext_palette != NULL)
	      rl2_destroy_palette (ext_palette);
      }
    raster =
	rl2_create_raster (width, height, sample_type, pixel_type, num_bands,
			   pixels, pixels_sz, palette, mask, mask_sz, NULL);
    if (raster == NULL)
	goto error;
    if (odd_data != NULL)
	free (odd_data);
    if (even_data != NULL)
	free (even_data);
    return raster;
  error:
    if (odd_mask != NULL)
	free (odd_mask);
    if (even_mask != NULL)
	free (even_mask);
    if (odd_data != NULL)
	free (odd_data);
    if (even_data != NULL)
	free (even_data);
    if (pixels != NULL)
	free (pixels);
    if (mask != NULL)
	free (mask);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (ext_palette != NULL)
	rl2_destroy_palette (ext_palette);
    return NULL;
}

static void
add_pooled_variance (rl2PrivBandStatisticsPtr band_in,
		     rl2PrivBandStatisticsPtr band_out, double count)
{
/* adding a Pooled Variance item */
    rl2PoolVariancePtr pool = malloc (sizeof (rl2PoolVariance));
    pool->count = count;
    pool->variance = band_in->sum_sq_diff / (count - 1.0);
    pool->next = NULL;
    if (band_out->first == NULL)
	band_out->first = pool;
    if (band_out->last != NULL)
	band_out->last->next = pool;
    band_out->last = pool;
}

static void
do_aggregate_histograms (rl2PrivBandStatisticsPtr band_in,
			 rl2PrivBandStatisticsPtr band_out)
{
/* rescaling and aggregating histograms */
    int ih;
    double interval_in = band_in->max - band_in->min;
    double interval_out = band_out->max - band_out->min;
    double step_in = interval_in / ((double) (band_in->nHistogram) - 1.0);
    double step_out = interval_out / ((double) (band_out->nHistogram) - 1.0);
    for (ih = 0; ih < band_in->nHistogram; ih++)
      {
	  double value = (((double) ih + 0.5) * step_in) + band_in->min;
	  double qty = *(band_in->histogram + ih);
	  double index = floor ((value - band_out->min) / step_out);
	  if (index < 0.0)
	      index = 0.0;
	  if (index > 255.0)
	      index = 255.0;
	  *(band_out->histogram + (unsigned int) index) += qty;
      }
}

RL2_DECLARE int
rl2_aggregate_raster_statistics (rl2RasterStatisticsPtr stats_in,
				 rl2RasterStatisticsPtr stats_out)
{
/* aggregating Raster Statistics */
    rl2PrivRasterStatisticsPtr in = (rl2PrivRasterStatisticsPtr) stats_in;
    rl2PrivRasterStatisticsPtr out = (rl2PrivRasterStatisticsPtr) stats_out;
    rl2PrivBandStatisticsPtr band_in;
    rl2PrivBandStatisticsPtr band_out;
    int ib;
    int ih;

    if (in == NULL || out == NULL)
	return RL2_ERROR;
    if (in->sampleType != out->sampleType)
	return RL2_ERROR;
    if (in->nBands != out->nBands)
	return RL2_ERROR;

    if (out->count == 0.0)
      {
	  /* initializing */
	  out->no_data = in->no_data;
	  out->count = in->count;
	  for (ib = 0; ib < in->nBands; ib++)
	    {
		band_in = in->band_stats + ib;
		band_out = out->band_stats + ib;
		band_out->min = band_in->min;
		band_out->max = band_in->max;
		band_out->mean = band_in->mean;
		add_pooled_variance (band_in, band_out, in->count);
		for (ih = 0; ih < band_in->nHistogram; ih++)
		    *(band_out->histogram + ih) = *(band_in->histogram + ih);
	    }
      }
    else
      {
	  /* aggregating */
	  out->no_data += in->no_data;
	  for (ib = 0; ib < in->nBands; ib++)
	    {
		band_in = in->band_stats + ib;
		band_out = out->band_stats + ib;
		if (band_in->min < band_out->min)
		    band_out->min = band_in->min;
		if (band_in->max > band_out->max)
		    band_out->max = band_in->max;
		add_pooled_variance (band_in, band_out, in->count);
		band_out->mean =
		    ((band_out->mean * out->count) +
		     (band_in->mean * in->count)) / (out->count + in->count);
		if (out->sampleType == RL2_SAMPLE_INT8
		    || out->sampleType == RL2_SAMPLE_UINT8)
		  {
		      /* just copying 8-bit histograms */
		      for (ih = 0; ih < band_in->nHistogram; ih++)
			  *(band_out->histogram + ih) +=
			      *(band_in->histogram + ih);
		  }
		else
		  {
		      /* rescaling and aggregating histograms */
		      do_aggregate_histograms (band_in, band_out);
		  }
	    }
	  out->count += in->count;
      }
    return RL2_OK;
}

static void
update_no_data_stats (rl2PrivRasterStatisticsPtr st)
{
/* updating the NoData count */
    st->no_data += 1.0;
}

static void
update_count_stats (rl2PrivRasterStatisticsPtr st)
{
/* updating the samples count */
    st->count += 1.0;
}

static void
update_stats (rl2PrivRasterStatisticsPtr st, int band, double value)
{
/* updating the Statistics */
    rl2PrivBandStatisticsPtr band_st = st->band_stats + band;
    if (value < band_st->min)
	band_st->min = value;
    if (value > band_st->max)
	band_st->max = value;
    if (st->count == 0.0)
      {
	  band_st->mean = value;
	  band_st->sum_sq_diff = 0.0;
      }
    else
      {
	  band_st->sum_sq_diff =
	      band_st->sum_sq_diff +
	      (((st->count -
		 1.0) * ((value - band_st->mean) * (value -
						    band_st->mean))) /
	       st->count);
	  band_st->mean = band_st->mean + ((value - band_st->mean) / st->count);
      }
    if (st->sampleType == RL2_SAMPLE_INT8)
      {
	  int idx = value + 128;
	  *(band_st->histogram + idx) += 1.0;
      }
    else if (st->sampleType == RL2_SAMPLE_1_BIT
	     || st->sampleType == RL2_SAMPLE_2_BIT
	     || st->sampleType == RL2_SAMPLE_4_BIT
	     || st->sampleType == RL2_SAMPLE_UINT8)
      {
	  int idx = value;
	  *(band_st->histogram + idx) += 1.0;
      }
}

static void
update_int8_stats (unsigned short width, unsigned short height,
		   const char *pixels, const unsigned char *mask,
		   rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing INT8 tile statistics */
    int x;
    int y;
    const char *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_INT8)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    update_no_data_stats (st);
			    p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const char *p_save = p_in;
		      char sample = 0;
		      rl2_get_pixel_sample_int8 (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		      else
			{
			    /* NO-DATA pixel */
			    update_no_data_stats (st);
			}
		  }
	    }
      }
}

static void
update_uint8_stats (unsigned short width, unsigned short height,
		    unsigned char num_bands,
		    const unsigned char *pixels, const unsigned char *mask,
		    rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing UINT8 tile statistics */
    int x;
    int y;
    int ib;
    const unsigned char *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != num_bands)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_1_BIT || sample_type == RL2_SAMPLE_2_BIT
	      || sample_type == RL2_SAMPLE_4_BIT
	      || sample_type == RL2_SAMPLE_UINT8)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    update_no_data_stats (st);
			    for (ib = 0; ib < num_bands; ib++)
				p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    update_count_stats (st);
			    for (ib = 0; ib < num_bands; ib++)
			      {
				  double value = *p_in++;
				  update_stats (st, ib, value);
			      }
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned char *p_save = p_in;
		      for (ib = 0; ib < num_bands; ib++)
			{
			    unsigned char sample = 0;
			    switch (sample_type)
			      {
			      case RL2_SAMPLE_1_BIT:
				  rl2_get_pixel_sample_1bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_2_BIT:
				  rl2_get_pixel_sample_2bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_4_BIT:
				  rl2_get_pixel_sample_4bit (no_data, &sample);
				  break;
			      case RL2_SAMPLE_UINT8:
				  rl2_get_pixel_sample_uint8 (no_data, ib,
							      &sample);
				  break;
			      };
			    if (sample == *p_in++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    update_count_stats (st);
			    p_in = p_save;
			    for (ib = 0; ib < num_bands; ib++)
			      {
				  double value = *p_in++;
				  update_stats (st, ib, value);
			      }
			}
		      else
			{
			    /* NO-DATA pixel */
			    update_no_data_stats (st);
			}
		  }
	    }
      }
}

static void
update_histogram (rl2PrivRasterStatisticsPtr st, int band, double value)
{
/* updating the Histogram */
    double interval;
    double step;
    double index;
    rl2PrivBandStatisticsPtr band_st = st->band_stats + band;

    interval = band_st->max - band_st->min;
    step = interval / ((double) (band_st->nHistogram) - 1.0);
    index = floor ((value - band_st->min) / step);
    if (index < 0.0)
	index = 0.0;
    if (index > 255.0)
	index = 255.0;
    *(band_st->histogram + (unsigned int) index) += 1.0;
}

static void
compute_int16_histogram (unsigned short width, unsigned short height,
			 const short *pixels, const unsigned char *mask,
			 rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing INT16 tile histogram */
    int x;
    int y;
    const short *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_INT16)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			  p_in++;
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const short *p_save = p_in;
		      short sample = 0;
		      rl2_get_pixel_sample_int16 (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
	    }
      }
}

static void
update_int16_stats (unsigned short width, unsigned short height,
		    const short *pixels, const unsigned char *mask,
		    rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing INT16 tile statistics */
    int x;
    int y;
    const short *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_INT16)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    update_no_data_stats (st);
			    p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const short *p_save = p_in;
		      short sample = 0;
		      rl2_get_pixel_sample_int16 (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		      else
			{
			    /* NO-DATA pixel */
			    update_no_data_stats (st);
			}
		  }
	    }
      }
    compute_int16_histogram (width, height, pixels, mask, st, no_data);
}

static void
compute_uint16_histogram (unsigned short width, unsigned short height,
			  unsigned char num_bands,
			  const unsigned short *pixels,
			  const unsigned char *mask,
			  rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing INT16 tile histogram */
    int x;
    int y;
    int ib;
    const unsigned short *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != num_bands)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_UINT16)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    for (ib = 0; ib < num_bands; ib++)
				p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    for (ib = 0; ib < num_bands; ib++)
			      {
				  double value = *p_in++;
				  update_histogram (st, ib, value);
			      }
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (ib = 0; ib < num_bands; ib++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, ib, &sample);
			    if (sample == *p_in++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    p_in = p_save;
			    for (ib = 0; ib < num_bands; ib++)
			      {
				  double value = *p_in++;
				  update_histogram (st, ib, value);
			      }
			}
		  }
	    }
      }
}

static void
update_uint16_stats (unsigned short width, unsigned short height,
		     unsigned char num_bands,
		     const unsigned short *pixels, const unsigned char *mask,
		     rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing UINT16 tile statistics */
    int x;
    int y;
    int ib;
    const unsigned short *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != num_bands)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_UINT16)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    update_no_data_stats (st);
			    for (ib = 0; ib < num_bands; ib++)
				p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    update_count_stats (st);
			    for (ib = 0; ib < num_bands; ib++)
			      {
				  double value = *p_in++;
				  update_stats (st, ib, value);
			      }
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned short *p_save = p_in;
		      for (ib = 0; ib < num_bands; ib++)
			{
			    unsigned short sample = 0;
			    rl2_get_pixel_sample_uint16 (no_data, ib, &sample);
			    if (sample == *p_in++)
				match++;
			}
		      if (match != num_bands)
			{
			    /* opaque pixel */
			    update_count_stats (st);
			    p_in = p_save;
			    for (ib = 0; ib < num_bands; ib++)
			      {
				  double value = *p_in++;
				  update_stats (st, ib, value);
			      }
			}
		      else
			{
			    /* NO-DATA pixel */
			    update_no_data_stats (st);
			}
		  }
	    }
      }
    compute_uint16_histogram (width, height, num_bands, pixels, mask, st,
			      no_data);
}

static void
compute_int32_histogram (unsigned short width, unsigned short height,
			 const int *pixels, const unsigned char *mask,
			 rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing INT16 tile histogram */
    int x;
    int y;
    const int *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_INT32)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			  p_in++;
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const int *p_save = p_in;
		      int sample = 0;
		      rl2_get_pixel_sample_int32 (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
	    }
      }
}

static void
update_int32_stats (unsigned short width, unsigned short height,
		    const int *pixels, const unsigned char *mask,
		    rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing INT32 tile statistics */
    int x;
    int y;
    const int *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_INT32)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    update_no_data_stats (st);
			    p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const int *p_save = p_in;
		      int sample = 0;
		      rl2_get_pixel_sample_int32 (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		      else
			{
			    /* NO-DATA pixel */
			    update_no_data_stats (st);
			}
		  }
	    }
      }
    compute_int32_histogram (width, height, pixels, mask, st, no_data);
}

static void
compute_uint32_histogram (unsigned short width, unsigned short height,
			  const unsigned int *pixels, const unsigned char *mask,
			  rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing INT16 tile histogram */
    int x;
    int y;
    const unsigned int *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_UINT32)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			  p_in++;
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned int *p_save = p_in;
		      unsigned int sample = 0;
		      rl2_get_pixel_sample_uint32 (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
	    }
      }
}

static void
update_uint32_stats (unsigned short width, unsigned short height,
		     const unsigned int *pixels, const unsigned char *mask,
		     rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing UINT32 tile statistics */
    int x;
    int y;
    const unsigned int *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_UINT32)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    update_no_data_stats (st);
			    p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const unsigned int *p_save = p_in;
		      unsigned int sample = 0;
		      rl2_get_pixel_sample_uint32 (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		      else
			{
			    /* NO-DATA pixel */
			    update_no_data_stats (st);
			}
		  }
	    }
      }
    compute_uint32_histogram (width, height, pixels, mask, st, no_data);
}

static void
compute_float_histogram (unsigned short width, unsigned short height,
			 const float *pixels, const unsigned char *mask,
			 rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing FLOAT tile histogram */
    int x;
    int y;
    const float *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_FLOAT)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			  p_in++;
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const float *p_save = p_in;
		      float sample = 0;
		      rl2_get_pixel_sample_float (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
	    }
      }
}

static void
update_float_stats (unsigned short width, unsigned short height,
		    const float *pixels, const unsigned char *mask,
		    rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing FLOAT tile statistics */
    int x;
    int y;
    const float *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_FLOAT)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    update_no_data_stats (st);
			    p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const float *p_save = p_in;
		      float sample = 0.0;
		      rl2_get_pixel_sample_float (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		      else
			{
			    /* NO-DATA pixel */
			    update_no_data_stats (st);
			}
		  }
	    }
      }
    compute_float_histogram (width, height, pixels, mask, st, no_data);
}

static void
compute_double_histogram (unsigned short width, unsigned short height,
			  const double *pixels, const unsigned char *mask,
			  rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing INT16 tile histogram */
    int x;
    int y;
    const double *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_DOUBLE)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			  p_in++;
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const double *p_save = p_in;
		      double sample = 0;
		      rl2_get_pixel_sample_double (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_histogram (st, 0, value);
			}
		  }
	    }
      }
}

static void
update_double_stats (unsigned short width, unsigned short height,
		     const double *pixels, const unsigned char *mask,
		     rl2PrivRasterStatisticsPtr st, rl2PixelPtr no_data)
{
/* computing DOUBLE tile statistics */
    int x;
    int y;
    const double *p_in = pixels;
    const unsigned char *p_msk = mask;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    int ignore_no_data = 1;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type (no_data, &sample_type, &pixel_type, &nbands)
	      != RL2_OK)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type == RL2_SAMPLE_DOUBLE)
	      ;
	  else
	      ignore_no_data = 1;
      }

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    update_no_data_stats (st);
			    p_in++;
			}
		      else
			{
			    /* opaque pixel */
			    double value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      const double *p_save = p_in;
		      double sample = 0.0;
		      rl2_get_pixel_sample_double (no_data, &sample);
		      if (sample == *p_in++)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    double value;
			    p_in = p_save;
			    value = *p_in++;
			    update_count_stats (st);
			    update_stats (st, 0, value);
			}
		      else
			{
			    /* NO-DATA pixel */
			    update_no_data_stats (st);
			}
		  }
	    }
      }
    compute_double_histogram (width, height, pixels, mask, st, no_data);
}

RL2_DECLARE rl2RasterStatisticsPtr
rl2_build_raster_statistics (rl2RasterPtr raster, rl2PixelPtr noData)
{
/* 
/ building a statistics object from a Raster object
*/
    rl2PrivRasterStatisticsPtr st;
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterPtr rst;
    if (raster == NULL)
	goto error;
    rst = (rl2PrivRasterPtr) raster;

    stats = rl2_create_raster_statistics (rst->sampleType, rst->nBands);
    if (stats == NULL)
	goto error;
    st = (rl2PrivRasterStatisticsPtr) stats;

    switch (rst->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  update_uint8_stats (rst->width, rst->height, rst->nBands,
			      (const unsigned char *) (rst->rasterBuffer),
			      rst->maskBuffer, st, noData);
	  break;
      case RL2_SAMPLE_INT8:
	  update_int8_stats (rst->width, rst->height,
			     (const char *) (rst->rasterBuffer),
			     rst->maskBuffer, st, noData);
	  break;
      case RL2_SAMPLE_UINT16:
	  update_uint16_stats (rst->width, rst->height, rst->nBands,
			       (const unsigned short *) (rst->rasterBuffer),
			       rst->maskBuffer, st, noData);
	  break;
      case RL2_SAMPLE_INT16:
	  update_int16_stats (rst->width, rst->height,
			      (const short *) (rst->rasterBuffer),
			      rst->maskBuffer, st, noData);
	  break;
      case RL2_SAMPLE_UINT32:
	  update_uint32_stats (rst->width, rst->height,
			       (const unsigned int *) (rst->rasterBuffer),
			       rst->maskBuffer, st, noData);
	  break;
      case RL2_SAMPLE_INT32:
	  update_int32_stats (rst->width, rst->height,
			      (const int *) (rst->rasterBuffer),
			      rst->maskBuffer, st, noData);
	  break;
      case RL2_SAMPLE_FLOAT:
	  update_float_stats (rst->width, rst->height,
			      (const float *) (rst->rasterBuffer),
			      rst->maskBuffer, st, noData);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  update_double_stats (rst->width, rst->height,
			       (const double *) (rst->rasterBuffer),
			       rst->maskBuffer, st, noData);
	  break;
      };
    return stats;

  error:
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    return NULL;
}

RL2_DECLARE rl2RasterStatisticsPtr
rl2_get_raster_statistics (const unsigned char *blob_odd,
			   int blob_odd_sz, const unsigned char *blob_even,
			   int blob_even_sz, rl2PalettePtr palette,
			   rl2PixelPtr noData)
{
/* 
/ decoding from internal RL2 binary format to Raster and 
/ building the corresponding statistics object
*/
    rl2RasterStatisticsPtr stats = NULL;
    rl2RasterPtr raster =
	rl2_raster_decode (RL2_SCALE_1, blob_odd, blob_odd_sz, blob_even,
			   blob_even_sz, palette);
    if (raster == NULL)
	goto error;
    palette = NULL;

    stats = rl2_build_raster_statistics (raster, noData);
    if (stats == NULL)
	goto error;
    rl2_destroy_raster (raster);
    return stats;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    return NULL;
}

RL2_DECLARE int
rl2_serialize_dbms_palette (rl2PalettePtr palette, unsigned char **blob,
			    int *blob_size)
{
/* creating a Palette (DBMS serialized format) */
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;
    rl2PrivPaletteEntryPtr entry;
    int sz = 12;
    uLong crc;
    int i;
    int endian_arch = endianArch ();
    unsigned char *p;
    unsigned char *ptr;

    if (plt == NULL)
	return RL2_ERROR;

    sz += plt->nEntries * 3;
    p = malloc (sz);
    if (p == NULL)
	return RL2_ERROR;
    ptr = p;

    *ptr++ = 0x00;		/* start marker */
    *ptr++ = RL2_DATA_START;
    *ptr++ = RL2_LITTLE_ENDIAN;
    exportU16 (ptr, plt->nEntries, 1, endian_arch);	/* # Palette entries */
    ptr += 2;
    *ptr++ = RL2_PALETTE_START;
    for (i = 0; i < plt->nEntries; i++)
      {
	  entry = plt->entries + i;
	  *ptr++ = entry->red;
	  *ptr++ = entry->green;
	  *ptr++ = entry->blue;
      }
    *ptr++ = RL2_PALETTE_END;
/* computing the CRC32 */
    crc = crc32 (0L, p, ptr - p);
    exportU32 (ptr, crc, 1, endian_arch);	/* the Palette own CRC */
    ptr += 4;
    *ptr++ = RL2_DATA_END;
    *blob = p;
    *blob_size = sz;
    return RL2_OK;
}

static int
check_serialized_palette (const unsigned char *blob, int blob_size)
{
/* checking a Raster Statistics serialized object from validity */
    uLong crc;
    uLong oldCrc;
    const unsigned char *ptr = blob;
    int endian;
    unsigned short nEntries;
    int endian_arch = endianArch ();
    if (blob == NULL)
	return 0;
    if (blob_size < 12)
	return 0;

    if (*ptr++ != 0x00)
	return 0;		/* invalid start signature */
    if (*ptr++ != RL2_DATA_START)
	return 0;		/* invalid start signature */
    endian = *ptr++;
    if (endian == RL2_LITTLE_ENDIAN || endian == RL2_BIG_ENDIAN)
	;
    else
	return 0;		/* invalid endiannes */
    nEntries = importU16 (ptr, endian, endian_arch);
    ptr += 2;
    if (blob_size != 12 + (nEntries * 3))
	return 0;		/* invalid size */
    if (*ptr++ != RL2_PALETTE_START)
	return 0;		/* invalid start marker */
    ptr += nEntries * 3;
    if (*ptr++ != RL2_PALETTE_END)
	return 0;
/* computing the CRC32 */
    crc = crc32 (0L, blob, ptr - blob);
    oldCrc = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (crc != oldCrc)
	return 0;
    if (*ptr != RL2_DATA_END)
	return 0;		/* invalid end signature */
    return 1;
}

RL2_DECLARE int
rl2_is_valid_dbms_palette (const unsigned char *blob, int blob_size,
			   unsigned char sample_type)
{
/* testing a serialized Palette object for validity */
    const unsigned char *ptr;
    int endian;
    unsigned short nEntries;
    int endian_arch = endianArch ();
    if (!check_serialized_palette (blob, blob_size))
	return RL2_ERROR;
    ptr = blob + 2;
    endian = *ptr++;
    nEntries = importU16 (ptr, endian, endian_arch);
    if (sample_type == RL2_SAMPLE_1_BIT || sample_type == RL2_SAMPLE_2_BIT
	|| sample_type == RL2_SAMPLE_4_BIT || sample_type == RL2_SAMPLE_UINT8)
	;
    else
	return RL2_ERROR;
    if (sample_type == RL2_SAMPLE_1_BIT && nEntries > 2)
	return RL2_ERROR;
    if (sample_type == RL2_SAMPLE_2_BIT && nEntries > 4)
	return RL2_ERROR;
    if (sample_type == RL2_SAMPLE_4_BIT && nEntries > 16)
	return RL2_ERROR;
    if (sample_type == RL2_SAMPLE_UINT8 && nEntries > 256)
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE rl2PalettePtr
rl2_deserialize_dbms_palette (const unsigned char *blob, int blob_size)
{
/* attempting to deserialize a Palette from DBMS binary format */
    rl2PalettePtr palette = NULL;
    int ip;
    const unsigned char *ptr;
    int endian;
    unsigned short nEntries;
    int endian_arch = endianArch ();
    if (blob == NULL)
	return NULL;
    if (blob_size < 12)
	return NULL;

    if (!check_serialized_palette (blob, blob_size))
	return NULL;
    ptr = blob + 2;
    endian = *ptr++;
    nEntries = importU16 (ptr, endian, endian_arch);
    ptr += 3;
    palette = rl2_create_palette (nEntries);
    if (palette == NULL)
	return NULL;
    for (ip = 0; ip < nEntries; ip++)
      {
	  unsigned char r = *ptr++;
	  unsigned char g = *ptr++;
	  unsigned char b = *ptr++;
	  rl2_set_palette_color (palette, ip, r, g, b);
      }
    return palette;
}

RL2_DECLARE int
rl2_serialize_dbms_raster_statistics (rl2RasterStatisticsPtr stats,
				      unsigned char **blob, int *blob_size)
{
/* creating a Raster Statistics (DBMS serialized format) */
    rl2PrivRasterStatisticsPtr st = (rl2PrivRasterStatisticsPtr) stats;
    rl2PrivBandStatisticsPtr band;
    int sz = 26;
    uLong crc;
    int ib;
    int ih;
    int endian_arch = endianArch ();
    unsigned char *p;
    unsigned char *ptr;

    *blob = NULL;
    *blob_size = 0;
    if (st == NULL)
	return RL2_ERROR;

    for (ib = 0; ib < st->nBands; ib++)
      {
	  band = st->band_stats + ib;
	  sz += 38;
	  sz += band->nHistogram * sizeof (double);
      }
    p = malloc (sz);
    if (p == NULL)
	return RL2_ERROR;
    ptr = p;

    *ptr++ = 0x00;		/* start marker */
    *ptr++ = RL2_STATS_START;
    *ptr++ = RL2_LITTLE_ENDIAN;
    *ptr++ = st->sampleType;
    *ptr++ = st->nBands;
    exportDouble (ptr, st->no_data, 1, endian_arch);	/* # no_data values */
    ptr += 8;
    exportDouble (ptr, st->count, 1, endian_arch);	/* # count */
    ptr += 8;
    for (ib = 0; ib < st->nBands; ib++)
      {
	  *ptr++ = RL2_BAND_STATS_START;
	  band = st->band_stats + ib;
	  exportDouble (ptr, band->min, 1, endian_arch);	/* # Min values */
	  ptr += 8;
	  exportDouble (ptr, band->max, 1, endian_arch);	/* # Max values */
	  ptr += 8;
	  exportDouble (ptr, band->mean, 1, endian_arch);	/* # Mean */
	  ptr += 8;
	  exportDouble (ptr, band->sum_sq_diff, 1, endian_arch);	/* # sum of square differences */
	  ptr += 8;
	  exportU16 (ptr, band->nHistogram, 1, endian_arch);	/* # Histogram entries */
	  ptr += 2;
	  *ptr++ = RL2_HISTOGRAM_START;
	  for (ih = 0; ih < band->nHistogram; ih++)
	    {
		exportDouble (ptr, *(band->histogram + ih), 1, endian_arch);	/* # Histogram value */
		ptr += 8;
	    }
	  *ptr++ = RL2_HISTOGRAM_END;
	  *ptr++ = RL2_BAND_STATS_END;
      }
/* computing the CRC32 */
    crc = crc32 (0L, p, ptr - p);
    exportU32 (ptr, crc, 1, endian_arch);	/* the Raster Statistics own CRC */
    ptr += 4;
    *ptr++ = RL2_STATS_END;
    *blob = p;
    *blob_size = sz;
    return RL2_OK;
}

static int
check_raster_serialized_statistics (const unsigned char *blob, int blob_size)
{
/* checking a Raster Statistics serialized object from validity */
    const unsigned char *ptr = blob;
    int endian;
    uLong crc;
    uLong oldCrc;
    int ib;
    unsigned short nHistogram;
    unsigned char sample_type;
    unsigned char num_bands;
    int endian_arch = endianArch ();

    if (blob == NULL)
	return 0;
    if (blob_size < 27)
	return 0;
    if (*ptr++ != 0x00)
	return 0;		/* invalid start signature */
    if (*ptr++ != RL2_STATS_START)
	return 0;		/* invalid start signature */
    endian = *ptr++;
    if (endian == RL2_LITTLE_ENDIAN || endian == RL2_BIG_ENDIAN)
	;
    else
	return 0;		/* invalid endiannes */
    sample_type = *ptr++;
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
      case RL2_SAMPLE_DOUBLE:
	  break;
      default:
	  return 0;
      };
    num_bands = *ptr++;

    ptr = blob + 21;
    for (ib = 0; ib < num_bands; ib++)
      {
	  if (((ptr - blob) + 38) >= blob_size)
	      return 0;
	  if (*ptr++ != RL2_BAND_STATS_START)
	      return 0;
	  ptr += 32;
	  nHistogram = importU16 (ptr, endian, endian_arch);
	  ptr += 2;
	  if (*ptr++ != RL2_HISTOGRAM_START)
	      return 0;
	  if (((ptr - blob) + 2 + (nHistogram * sizeof (double))) >=
	      (unsigned int) blob_size)
	      return 0;
	  ptr += nHistogram * sizeof (double);
	  if (*ptr++ != RL2_HISTOGRAM_END)
	      return 0;
	  if (*ptr++ != RL2_BAND_STATS_END)
	      return 0;
      }
/* computing the CRC32 */
    crc = crc32 (0L, blob, ptr - blob);
    oldCrc = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (crc != oldCrc)
	return 0;
    if (*ptr != RL2_STATS_END)
	return 0;		/* invalid end signature */

    return 1;
}

RL2_DECLARE int
rl2_is_valid_dbms_raster_statistics (const unsigned char *blob, int blob_size,
				     unsigned char sample_type,
				     unsigned char num_bands)
{
/* testing a serialized Raster Statistics object for validity */
    const unsigned char *ptr;
    unsigned char xsample_type;
    unsigned char xnum_bands;
    if (!check_raster_serialized_statistics (blob, blob_size))
	return RL2_ERROR;
    ptr = blob + 3;
    xsample_type = *ptr++;
    xnum_bands = *ptr++;
    if (sample_type == xsample_type && num_bands == xnum_bands)
	return RL2_OK;
    return RL2_ERROR;
}

RL2_DECLARE rl2RasterStatisticsPtr
rl2_deserialize_dbms_raster_statistics (const unsigned char *blob,
					int blob_size)
{
/* attempting to deserialize a Raster Statistics object from DBMS binary format */
    rl2RasterStatisticsPtr stats = NULL;
    rl2PrivRasterStatisticsPtr st;
    unsigned char sample_type;
    unsigned char num_bands;
    int ib;
    int ih;
    const unsigned char *ptr = blob;
    int endian;
    int endian_arch = endianArch ();
    if (!check_raster_serialized_statistics (blob, blob_size))
	return NULL;

    ptr = blob + 2;
    endian = *ptr++;
    sample_type = *ptr++;
    num_bands = *ptr++;
    stats = rl2_create_raster_statistics (sample_type, num_bands);
    if (stats == NULL)
	goto error;
    st = (rl2PrivRasterStatisticsPtr) stats;
    st->no_data = importDouble (ptr, endian, endian_arch);	/* # no_data values */
    ptr += 8;
    st->count = importDouble (ptr, endian, endian_arch);	/* # count */
    ptr += 8;
    for (ib = 0; ib < num_bands; ib++)
      {
	  rl2PrivBandStatisticsPtr band = st->band_stats + ib;
	  ptr++;		/* skipping BAND START marker */
	  band->min = importDouble (ptr, endian, endian_arch);	/* # Min values */
	  ptr += 8;
	  band->max = importDouble (ptr, endian, endian_arch);	/* # Max values */
	  ptr += 8;
	  band->mean = importDouble (ptr, endian, endian_arch);	/* # Mean */
	  ptr += 8;
	  band->sum_sq_diff = importDouble (ptr, endian, endian_arch);	/* # Sum of square differences */
	  ptr += 8;
	  ptr += 3;		/* skipping HISTOGRAM START marker */
	  for (ih = 0; ih < band->nHistogram; ih++)
	    {
		*(band->histogram + ih) = importDouble (ptr, endian, endian_arch);	/* # Histogram values */
		ptr += 8;
	    }
	  ptr += 2;		/* skipping END markers */
      }
    return stats;

  error:
    if (stats != NULL)
	rl2_destroy_raster_statistics (stats);
    return NULL;
}

RL2_DECLARE int
rl2_serialize_dbms_pixel (rl2PixelPtr pixel, unsigned char **blob,
			  int *blob_size)
{
/* creating a NO-DATA (DBMS serialized format) */
    rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) pixel;
    rl2PrivSamplePtr band;
    int sz = 12;
    uLong crc;
    int ib;
    int endian_arch = endianArch ();
    unsigned char *p;
    unsigned char *ptr;

    *blob = NULL;
    *blob_size = 0;
    if (pxl == NULL)
	return RL2_ERROR;

    switch (pxl->sampleType)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_INT8:
	  sz += 3;
	  break;
      case RL2_SAMPLE_UINT8:
	  sz += pxl->nBands * 3;
	  break;
      case RL2_SAMPLE_INT16:
	  sz += 4;
	  break;
      case RL2_SAMPLE_UINT16:
	  sz += (pxl->nBands * 4);
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  sz += 6;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sz += 10;
	  break;
      default:
	  return RL2_ERROR;
      };
    p = malloc (sz);
    if (p == NULL)
	return RL2_ERROR;
    ptr = p;

    *ptr++ = 0x00;		/* start marker */
    *ptr++ = RL2_NO_DATA_START;
    *ptr++ = RL2_LITTLE_ENDIAN;
    *ptr++ = pxl->sampleType;
    *ptr++ = pxl->pixelType;
    *ptr++ = pxl->nBands;
    *ptr++ = pxl->isTransparent;
    for (ib = 0; ib < pxl->nBands; ib++)
      {
	  *ptr++ = RL2_SAMPLE_START;
	  band = pxl->Samples + ib;
	  switch (pxl->sampleType)
	    {
	    case RL2_SAMPLE_INT8:
		*ptr++ = (unsigned char) (band->int8);
		break;
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		*ptr++ = band->uint8;
		break;
	    case RL2_SAMPLE_INT16:
		export16 (ptr, band->int16, 1, endian_arch);
		ptr += 2;
		break;
	    case RL2_SAMPLE_UINT16:
		exportU16 (ptr, band->uint16, 1, endian_arch);
		ptr += 2;
		break;
	    case RL2_SAMPLE_INT32:
		export32 (ptr, band->int32, 1, endian_arch);
		ptr += 4;
		break;
	    case RL2_SAMPLE_UINT32:
		exportU32 (ptr, band->uint32, 1, endian_arch);
		ptr += 4;
		break;
	    case RL2_SAMPLE_FLOAT:
		exportFloat (ptr, band->float32, 1, endian_arch);
		ptr += 4;
		break;
	    case RL2_SAMPLE_DOUBLE:
		exportDouble (ptr, band->float64, 1, endian_arch);
		ptr += 8;
		break;
	    };
	  *ptr++ = RL2_SAMPLE_END;
      }
/* computing the CRC32 */
    crc = crc32 (0L, p, ptr - p);
    exportU32 (ptr, crc, 1, endian_arch);	/* the NO-DATA own CRC */
    ptr += 4;
    *ptr++ = RL2_NO_DATA_END;
    *blob = p;
    *blob_size = sz;
    return RL2_OK;
}

static int
check_raster_serialized_pixel (const unsigned char *blob, int blob_size)
{
/* checking a Pixel value serialized object from validity */
    const unsigned char *ptr = blob;
    int endian;
    uLong crc;
    uLong oldCrc;
    int ib;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int endian_arch = endianArch ();

    if (blob == NULL)
	return 0;
    if (blob_size < 13)
	return 0;
    if (*ptr++ != 0x00)
	return 0;		/* invalid start signature */
    if (*ptr++ != RL2_NO_DATA_START)
	return 0;		/* invalid start signature */
    endian = *ptr++;
    if (endian == RL2_LITTLE_ENDIAN || endian == RL2_BIG_ENDIAN)
	;
    else
	return 0;		/* invalid endiannes */
    sample_type = *ptr++;
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_UINT8:
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
      case RL2_SAMPLE_DOUBLE:
	  break;
      default:
	  return 0;
      };
    pixel_type = *ptr++;
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
      case RL2_PIXEL_MULTIBAND:
      case RL2_PIXEL_DATAGRID:
	  break;
      default:
	  return 0;
      };
    num_bands = *ptr++;
    ptr++;
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
	  if ((pixel_type == RL2_PIXEL_PALETTE
	       || pixel_type == RL2_PIXEL_MONOCHROME) && num_bands == 1)
	      ;
	  else
	      return 0;
	  break;
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
	  if ((pixel_type == RL2_PIXEL_PALETTE
	       || pixel_type == RL2_PIXEL_GRAYSCALE) && num_bands == 1)
	      ;
	  else
	      return 0;
	  break;
      case RL2_SAMPLE_UINT8:
	  if ((pixel_type == RL2_PIXEL_PALETTE
	       || pixel_type == RL2_PIXEL_GRAYSCALE
	       || pixel_type == RL2_PIXEL_DATAGRID) && num_bands == 1)
	      ;
	  else if (pixel_type == RL2_PIXEL_RGB && num_bands == 3)
	      ;
	  else if (pixel_type == RL2_PIXEL_MULTIBAND && num_bands > 1)
	      ;
	  else
	      return 0;
	  break;
      case RL2_SAMPLE_UINT16:
	  if ((pixel_type == RL2_PIXEL_GRAYSCALE
	       || pixel_type == RL2_PIXEL_DATAGRID) && num_bands == 1)
	      ;
	  else if (pixel_type == RL2_PIXEL_RGB && num_bands == 3)
	      ;
	  else if (pixel_type == RL2_PIXEL_MULTIBAND && num_bands > 1)
	      ;
	  else
	      return 0;
	  break;
      case RL2_SAMPLE_INT8:
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
      case RL2_SAMPLE_DOUBLE:
	  if (pixel_type == RL2_PIXEL_DATAGRID && num_bands == 1)
	      ;
	  else
	      return 0;
	  break;
      default:
	  return 0;
      };

    for (ib = 0; ib < num_bands; ib++)
      {
	  if (*ptr++ != RL2_SAMPLE_START)
	      return 0;
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_INT8:
	    case RL2_SAMPLE_UINT8:
		ptr++;
		break;
	    case RL2_SAMPLE_INT16:
	    case RL2_SAMPLE_UINT16:
		ptr += 2;
		break;
	    case RL2_SAMPLE_INT32:
	    case RL2_SAMPLE_UINT32:
	    case RL2_SAMPLE_FLOAT:
		ptr += 4;
		break;
	    case RL2_SAMPLE_DOUBLE:
		ptr += 8;
		break;
	    };
	  if (((ptr - blob) + 6) > blob_size)
	      return 0;
	  if (*ptr++ != RL2_SAMPLE_END)
	      return 0;
      }
/* computing the CRC32 */
    crc = crc32 (0L, blob, ptr - blob);
    oldCrc = importU32 (ptr, endian, endian_arch);
    ptr += 4;
    if (crc != oldCrc)
	return 0;
    if (*ptr != RL2_NO_DATA_END)
	return 0;		/* invalid end signature */

    return 1;
}

RL2_DECLARE int
rl2_is_valid_dbms_pixel (const unsigned char *blob, int blob_size,
			 unsigned char sample_type, unsigned char num_bands)
{
/* testing a serialized Pixel value for validity */
    const unsigned char *ptr;
    unsigned char xsample_type;
    unsigned char xnum_bands;
    if (!check_raster_serialized_pixel (blob, blob_size))
	return RL2_ERROR;
    ptr = blob + 3;
    xsample_type = *ptr++;
    ptr++;
    xnum_bands = *ptr++;
    if (sample_type == xsample_type && num_bands == xnum_bands)
	return RL2_OK;
    return RL2_ERROR;
}

RL2_DECLARE rl2PixelPtr
rl2_deserialize_dbms_pixel (const unsigned char *blob, int blob_size)
{
/* attempting to deserialize a Pixel value from DBMS binary format */
    rl2PixelPtr pixel = NULL;
    rl2PrivPixelPtr pxl;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    unsigned char transparent;
    int ib;
    const unsigned char *ptr = blob;
    int endian;
    int endian_arch = endianArch ();
    if (!check_raster_serialized_pixel (blob, blob_size))
	return NULL;

    ptr = blob + 2;
    endian = *ptr++;
    sample_type = *ptr++;
    pixel_type = *ptr++;
    num_bands = *ptr++;
    transparent = *ptr++;
    pixel = rl2_create_pixel (sample_type, pixel_type, num_bands);
    if (pixel == NULL)
	goto error;
    pxl = (rl2PrivPixelPtr) pixel;
    pxl->isTransparent = transparent;
    for (ib = 0; ib < num_bands; ib++)
      {
	  rl2PrivSamplePtr band = pxl->Samples + ib;
	  ptr++;		/* skipping SAMPLE START marker */
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_INT8:
		band->int8 = *((char *) ptr);
		ptr++;
		break;
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		band->uint8 = *ptr;
		ptr++;
		break;
	    case RL2_SAMPLE_INT16:
		band->int16 = import16 (ptr, endian, endian_arch);
		ptr += 2;
		break;
	    case RL2_SAMPLE_UINT16:
		band->uint16 = importU16 (ptr, endian, endian_arch);
		ptr += 2;
		break;
	    case RL2_SAMPLE_INT32:
		band->int32 = import32 (ptr, endian, endian_arch);
		ptr += 4;
		break;
	    case RL2_SAMPLE_UINT32:
		band->uint32 = importU32 (ptr, endian, endian_arch);
		ptr += 4;
		break;
	    case RL2_SAMPLE_FLOAT:
		band->float32 = importFloat (ptr, endian, endian_arch);
		ptr += 4;
		break;
	    case RL2_SAMPLE_DOUBLE:
		band->float64 = importDouble (ptr, endian, endian_arch);
		ptr += 8;
		break;
	    };
	  ptr++;		/* skipping SAMPLE END marker */
      }
    return pixel;

  error:
    if (pixel != NULL)
	rl2_destroy_pixel (pixel);
    return NULL;
}

static void
delta_encode_1 (unsigned char *buffer, int size)
{
/* Delta encoding - distance 1 */
    int i;
    unsigned char *p = buffer;
    unsigned char history = *p++;
    for (i = 1; i < size; i++)
      {
	  /* computing Deltas */
	  unsigned char tmp = *p - history;
	  history = *p;
	  *p++ = tmp;
      }
}

static void
delta_encode_2 (unsigned char *buffer, int size)
{
/* Delta encoding - distance 2 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[2];
/* saving the first component */
    memcpy (history, p, 2);
    p += 2;
    for (i = 2; i < size; i += 2)
      {
	  /* computing Deltas */
	  unsigned char tmp[2];
	  tmp[0] = *(p + 0) - history[0];
	  tmp[1] = *(p + 1) - history[1];
	  memcpy (history, p, 2);
	  memcpy (p, tmp, 2);
	  p += 2;
      }
}

static void
delta_encode_3 (unsigned char *buffer, int size)
{
/* Delta encoding - distance 3 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[3];
/* saving the first component */
    memcpy (history, p, 3);
    p += 3;
    for (i = 3; i < size; i += 3)
      {
	  /* computing Deltas */
	  unsigned char tmp[3];
	  tmp[0] = *(p + 0) - history[0];
	  tmp[1] = *(p + 1) - history[1];
	  tmp[2] = *(p + 2) - history[2];
	  memcpy (history, p, 3);
	  memcpy (p, tmp, 3);
	  p += 3;
      }
}

static void
delta_encode_4 (unsigned char *buffer, int size)
{
/* Delta encoding - distance 4 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[4];
/* saving the first component */
    memcpy (history, p, 4);
    p += 4;
    for (i = 4; i < size; i += 4)
      {
	  /* computing Deltas */
	  unsigned char tmp[4];
	  tmp[0] = *(p + 0) - history[0];
	  tmp[1] = *(p + 1) - history[1];
	  tmp[2] = *(p + 2) - history[2];
	  tmp[3] = *(p + 3) - history[3];
	  memcpy (history, p, 4);
	  memcpy (p, tmp, 4);
	  p += 4;
      }
}

static void
delta_encode_6 (unsigned char *buffer, int size)
{
/* Delta encoding - distance 6 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[6];
/* saving the first component */
    memcpy (history, p, 6);
    p += 6;
    for (i = 6; i < size; i += 6)
      {
	  /* computing Deltas */
	  unsigned char tmp[6];
	  tmp[0] = *(p + 0) - history[0];
	  tmp[1] = *(p + 1) - history[1];
	  tmp[2] = *(p + 2) - history[2];
	  tmp[3] = *(p + 3) - history[3];
	  tmp[4] = *(p + 4) - history[4];
	  tmp[5] = *(p + 5) - history[5];
	  memcpy (history, p, 6);
	  memcpy (p, tmp, 6);
	  p += 6;
      }
}

static void
delta_encode_8 (unsigned char *buffer, int size)
{
/* Delta encoding - distance 8 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[8];
/* saving the first component */
    memcpy (history, p, 8);
    p += 8;
    for (i = 8; i < size; i += 8)
      {
	  /* computing Deltas */
	  unsigned char tmp[8];
	  tmp[0] = *(p + 0) - history[0];
	  tmp[1] = *(p + 1) - history[1];
	  tmp[2] = *(p + 2) - history[2];
	  tmp[3] = *(p + 3) - history[3];
	  tmp[4] = *(p + 4) - history[4];
	  tmp[5] = *(p + 5) - history[5];
	  tmp[6] = *(p + 6) - history[6];
	  tmp[7] = *(p + 7) - history[7];
	  memcpy (history, p, 8);
	  memcpy (p, tmp, 8);
	  p += 8;
      }
}

RL2_PRIVATE int
rl2_delta_encode (unsigned char *buffer, int size, int distance)
{
/* Delta encoding */
    if ((size % distance) != 0)
	return RL2_ERROR;
    switch (distance)
      {
      case 1:
	  delta_encode_1 (buffer, size);
	  return RL2_OK;
      case 2:
	  delta_encode_2 (buffer, size);
	  return RL2_OK;
      case 3:
	  delta_encode_3 (buffer, size);
	  return RL2_OK;
      case 4:
	  delta_encode_4 (buffer, size);
	  return RL2_OK;
      case 6:
	  delta_encode_6 (buffer, size);
	  return RL2_OK;
      case 8:
	  delta_encode_8 (buffer, size);
	  return RL2_OK;
      };
    return RL2_ERROR;
}

static void
delta_decode_1 (unsigned char *buffer, int size)
{
/* Delta decoding - distance 1 */
    int i;
    unsigned char *p = buffer;
    unsigned char history = *p++;
    for (i = 1; i < size; i++)
      {
	  /* restoring Deltas */
	  unsigned char tmp = history + *p;
	  *p = tmp;
	  history = *p++;
      }
}

static void
delta_decode_2 (unsigned char *buffer, int size)
{
/* Delta decoding - distance 2 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[2];
/* saving the first component */
    memcpy (history, p, 2);
    p += 2;
    for (i = 2; i < size; i += 2)
      {
	  /* restoring Deltas */
	  unsigned char tmp[2];
	  tmp[0] = history[0] + *(p + 0);
	  tmp[1] = history[1] + *(p + 1);
	  memcpy (p, tmp, 2);
	  memcpy (history, p, 2);
	  p += 2;
      }
}

static void
delta_decode_3 (unsigned char *buffer, int size)
{
/* Delta decoding - distance 3 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[3];
/* saving the first component */
    memcpy (history, p, 3);
    p += 3;
    for (i = 3; i < size; i += 3)
      {
	  /* restoring Deltas */
	  unsigned char tmp[3];
	  tmp[0] = history[0] + *(p + 0);
	  tmp[1] = history[1] + *(p + 1);
	  tmp[2] = history[2] + *(p + 2);
	  memcpy (p, tmp, 3);
	  memcpy (history, p, 3);
	  p += 3;
      }
}

static void
delta_decode_4 (unsigned char *buffer, int size)
{
/* Delta decoding - distance 4 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[4];
/* saving the first component */
    memcpy (history, p, 4);
    p += 4;
    for (i = 4; i < size; i += 4)
      {
	  /* restoring Deltas */
	  unsigned char tmp[4];
	  tmp[0] = history[0] + *(p + 0);
	  tmp[1] = history[1] + *(p + 1);
	  tmp[2] = history[2] + *(p + 2);
	  tmp[3] = history[3] + *(p + 3);
	  memcpy (p, tmp, 4);
	  memcpy (history, p, 4);
	  p += 4;
      }
}

static void
delta_decode_6 (unsigned char *buffer, int size)
{
/* Delta decoding - distance 6 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[6];
/* saving the first component */
    memcpy (history, p, 6);
    p += 6;
    for (i = 6; i < size; i += 6)
      {
	  /* restoring Deltas */
	  unsigned char tmp[6];
	  tmp[0] = history[0] + *(p + 0);
	  tmp[1] = history[1] + *(p + 1);
	  tmp[2] = history[2] + *(p + 2);
	  tmp[3] = history[3] + *(p + 3);
	  tmp[4] = history[4] + *(p + 4);
	  tmp[5] = history[5] + *(p + 5);
	  memcpy (p, tmp, 6);
	  memcpy (history, p, 6);
	  p += 6;
      }
}

static void
delta_decode_8 (unsigned char *buffer, int size)
{
/* Delta decoding - distance 8 */
    int i;
    unsigned char *p = buffer;
    unsigned char history[8];
/* saving the first component */
    memcpy (history, p, 8);
    p += 8;
    for (i = 8; i < size; i += 8)
      {
	  /* restoring Deltas */
	  unsigned char tmp[8];
	  tmp[0] = history[0] + *(p + 0);
	  tmp[1] = history[1] + *(p + 1);
	  tmp[2] = history[2] + *(p + 2);
	  tmp[3] = history[3] + *(p + 3);
	  tmp[4] = history[4] + *(p + 4);
	  tmp[5] = history[5] + *(p + 5);
	  tmp[6] = history[6] + *(p + 6);
	  tmp[7] = history[7] + *(p + 7);
	  memcpy (p, tmp, 8);
	  memcpy (history, p, 8);
	  p += 8;
      }
}

RL2_PRIVATE int
rl2_delta_decode (unsigned char *buffer, int size, int distance)
{
/* Delta decoding */
    if ((size % distance) != 0)
	return RL2_ERROR;
    switch (distance)
      {
      case 1:
	  delta_decode_1 (buffer, size);
	  return RL2_OK;
      case 2:
	  delta_decode_2 (buffer, size);
	  return RL2_OK;
      case 3:
	  delta_decode_3 (buffer, size);
	  return RL2_OK;
      case 4:
	  delta_decode_4 (buffer, size);
	  return RL2_OK;
      case 6:
	  delta_decode_6 (buffer, size);
	  return RL2_OK;
      case 8:
	  delta_decode_8 (buffer, size);
	  return RL2_OK;
      };
    return RL2_ERROR;
}
