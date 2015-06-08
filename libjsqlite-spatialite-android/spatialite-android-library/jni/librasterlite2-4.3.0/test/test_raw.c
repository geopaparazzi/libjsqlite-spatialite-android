/*

 test_raw.c -- RasterLite-2 Test Case

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"

static int
is_big_endian_cpu ()
{
/* checking if the target CPU is big-endian */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 1;
    return 0;
}

static void
build_grid_i8 (unsigned int width, unsigned int height, unsigned char **blob,
	       int *blob_sz)
{
/* building an INT8 RAW buffer */
    int buf_sz = width * height * sizeof (char);
    char *buf = malloc (buf_sz);
    char *p = buf;
    unsigned int x;
    unsigned int y;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if ((y % 2) == 1)
		  {
		      if ((x % 2) == 1)
			  *p++ = 123;
		      else
			  *p++ = -98;
		  }
		else
		  {
		      if ((x % 2) == 1)
			  *p++ = -98;
		      else
			  *p++ = 123;
		  }
	    }
      }
    *blob = (unsigned char *) buf;
    *blob_sz = buf_sz;
}

static void
build_grid_u8 (unsigned int width, unsigned int height, unsigned char **blob,
	       int *blob_sz)
{
/* building an UINT8 RAW buffer */
    int buf_sz = width * height * sizeof (unsigned char);
    unsigned char *buf = malloc (buf_sz);
    unsigned char *p = buf;
    unsigned int x;
    unsigned int y;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if ((y % 2) == 1)
		  {
		      if ((x % 2) == 1)
			  *p++ = 125;
		      else
			  *p++ = 97;
		  }
		else
		  {
		      if ((x % 2) == 1)
			  *p++ = 97;
		      else
			  *p++ = 125;
		  }
	    }
      }
    *blob = buf;
    *blob_sz = buf_sz;
}

static void
build_grid_i16 (unsigned int width, unsigned int height, unsigned char **blob,
		int *blob_sz)
{
/* building an INT16 RAW buffer */
    int buf_sz = width * height * sizeof (short);
    short *buf = malloc (buf_sz);
    short *p = buf;
    unsigned int x;
    unsigned int y;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if ((y % 2) == 1)
		  {
		      if ((x % 2) == 1)
			  *p++ = 12345;
		      else
			  *p++ = -9876;
		  }
		else
		  {
		      if ((x % 2) == 1)
			  *p++ = -9876;
		      else
			  *p++ = 12345;
		  }
	    }
      }
    *blob = (unsigned char *) buf;
    *blob_sz = buf_sz;
}

static void
build_grid_u16 (unsigned int width, unsigned int height, unsigned char **blob,
		int *blob_sz)
{
/* building an UINT16 RAW buffer */
    int buf_sz = width * height * sizeof (unsigned short);
    unsigned short *buf = malloc (buf_sz);
    unsigned short *p = buf;
    unsigned int x;
    unsigned int y;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if ((y % 2) == 1)
		  {
		      if ((x % 2) == 1)
			  *p++ = 18345;
		      else
			  *p++ = 9746;
		  }
		else
		  {
		      if ((x % 2) == 1)
			  *p++ = 9746;
		      else
			  *p++ = 18345;
		  }
	    }
      }
    *blob = (unsigned char *) buf;
    *blob_sz = buf_sz;
}

static void
build_grid_i32 (unsigned int width, unsigned int height, unsigned char **blob,
		int *blob_sz)
{
/* building an INT32 RAW buffer */
    int buf_sz = width * height * sizeof (int);
    int *buf = malloc (buf_sz);
    int *p = buf;
    unsigned int x;
    unsigned int y;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if ((y % 2) == 1)
		  {
		      if ((x % 2) == 1)
			  *p++ = 123456789;
		      else
			  *p++ = -987654321;
		  }
		else
		  {
		      if ((x % 2) == 1)
			  *p++ = -987654321;
		      else
			  *p++ = 123456789;
		  }
	    }
      }
    *blob = (unsigned char *) buf;
    *blob_sz = buf_sz;
}

static void
build_grid_u32 (unsigned int width, unsigned int height, unsigned char **blob,
		int *blob_sz)
{
/* building an UINT32 RAW buffer */
    int buf_sz = width * height * sizeof (unsigned int);
    unsigned int *buf = malloc (buf_sz);
    unsigned int *p = buf;
    unsigned int x;
    unsigned int y;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if ((y % 2) == 1)
		  {
		      if ((x % 2) == 1)
			  *p++ = 123457896;
		      else
			  *p++ = 986543217;
		  }
		else
		  {
		      if ((x % 2) == 1)
			  *p++ = 985432167;
		      else
			  *p++ = 123456789;
		  }
	    }
      }
    *blob = (unsigned char *) buf;
    *blob_sz = buf_sz;
}

static void
build_grid_flt (unsigned int width, unsigned int height, unsigned char **blob,
		int *blob_sz)
{
/* building a FLOAT RAW buffer */
    int buf_sz = width * height * sizeof (float);
    float *buf = malloc (buf_sz);
    float *p = buf;
    unsigned int x;
    unsigned int y;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if ((y % 2) == 1)
		  {
		      if ((x % 2) == 1)
			  *p++ = 1234.5600;
		      else
			  *p++ = -987.6500;
		  }
		else
		  {
		      if ((x % 2) == 1)
			  *p++ = -987.6500;
		      else
			  *p++ = 1234.5600;
		  }
	    }
      }
    *blob = (unsigned char *) buf;
    *blob_sz = buf_sz;
}

static void
build_grid_dbl (unsigned int width, unsigned int height, unsigned char **blob,
		int *blob_sz)
{
/* building a DOUBLE RAW buffer */
    int buf_sz = width * height * sizeof (double);
    double *buf = malloc (buf_sz);
    double *p = buf;
    unsigned int x;
    unsigned int y;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if ((y % 2) == 1)
		  {
		      if ((x % 2) == 1)
			  *p++ = 123456.7800;
		      else
			  *p++ = -98765.4300;
		  }
		else
		  {
		      if ((x % 2) == 1)
			  *p++ = -98765.4300;
		      else
			  *p++ = 123456.7800;
		  }
	    }
      }
    *blob = (unsigned char *) buf;
    *blob_sz = buf_sz;
}

static void
reverse_i16 (unsigned int width, unsigned int height, const short *in,
	     short *out)
{
/* reversing the endianness - INT16 */
    union cvt
    {
	unsigned char byte[2];
	short value;
    };
    union cvt cin;
    union cvt cout;
    unsigned int x;
    unsigned int y;
    const short *p_in = in;
    short *p_out = out;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		cin.value = *p_in++;
		cout.byte[0] = cin.byte[1];
		cout.byte[1] = cin.byte[0];
		*p_out++ = cout.value;
	    }
      }
}

static void
reverse_u16 (unsigned int width, unsigned int height, const unsigned short *in,
	     unsigned short *out)
{
/* reversing the endianness - UINT16 */
    union cvt
    {
	unsigned char byte[2];
	unsigned short value;
    };
    union cvt cin;
    union cvt cout;
    unsigned int x;
    unsigned int y;
    const unsigned short *p_in = in;
    unsigned short *p_out = out;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		cin.value = *p_in++;
		cout.byte[0] = cin.byte[1];
		cout.byte[1] = cin.byte[0];
		*p_out++ = cout.value;
	    }
      }
}

static void
reverse_i32 (unsigned int width, unsigned int height, const int *in, int *out)
{
/* reversing the endianness - INT32 */
    union cvt
    {
	unsigned char byte[4];
	int value;
    };
    union cvt cin;
    union cvt cout;
    unsigned int x;
    unsigned int y;
    const int *p_in = in;
    int *p_out = out;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		cin.value = *p_in++;
		cout.byte[0] = cin.byte[3];
		cout.byte[1] = cin.byte[2];
		cout.byte[2] = cin.byte[1];
		cout.byte[3] = cin.byte[0];
		*p_out++ = cout.value;
	    }
      }
}

static void
reverse_u32 (unsigned int width, unsigned int height, const unsigned int *in,
	     unsigned int *out)
{
/* reversing the endianness - UINT32 */
    union cvt
    {
	unsigned char byte[4];
	unsigned int value;
    };
    union cvt cin;
    union cvt cout;
    unsigned int x;
    unsigned int y;
    const unsigned int *p_in = in;
    unsigned int *p_out = out;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		cin.value = *p_in++;
		cout.byte[0] = cin.byte[3];
		cout.byte[1] = cin.byte[2];
		cout.byte[2] = cin.byte[1];
		cout.byte[3] = cin.byte[0];
		*p_out++ = cout.value;
	    }
      }
}

static void
reverse_flt (unsigned int width, unsigned int height, const float *in,
	     float *out)
{
/* reversing the endianness - FLOAT */
    union cvt
    {
	unsigned char byte[4];
	float value;
    };
    union cvt cin;
    union cvt cout;
    unsigned int x;
    unsigned int y;
    const float *p_in = in;
    float *p_out = out;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		cin.value = *p_in++;
		cout.byte[0] = cin.byte[3];
		cout.byte[1] = cin.byte[2];
		cout.byte[2] = cin.byte[1];
		cout.byte[3] = cin.byte[0];
		*p_out++ = cout.value;
	    }
      }
}

static void
reverse_dbl (unsigned int width, unsigned int height, const double *in,
	     double *out)
{
/* reversing the endianness - DOUBLE */
    union cvt
    {
	unsigned char byte[8];
	double value;
    };
    union cvt cin;
    union cvt cout;
    unsigned int x;
    unsigned int y;
    const double *p_in = in;
    double *p_out = out;

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		cin.value = *p_in++;
		cout.byte[0] = cin.byte[7];
		cout.byte[1] = cin.byte[6];
		cout.byte[2] = cin.byte[5];
		cout.byte[3] = cin.byte[4];
		cout.byte[4] = cin.byte[3];
		cout.byte[5] = cin.byte[2];
		cout.byte[6] = cin.byte[1];
		cout.byte[7] = cin.byte[0];
		*p_out++ = cout.value;
	    }
      }
}

static void
reverse_endian (unsigned int width, unsigned int height,
		const unsigned char *blob, int blob_sz, unsigned char sample,
		unsigned char **rev_blob)
{
/* reversing the endianness */
    unsigned char *buf = malloc (blob_sz);
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  memcpy (buf, blob, blob_sz);
	  break;
      case RL2_SAMPLE_UINT8:
	  memcpy (buf, blob, blob_sz);
	  break;
      case RL2_SAMPLE_INT16:
	  reverse_i16 (width, height, (const short *) blob, (short *) buf);
	  break;
      case RL2_SAMPLE_UINT16:
	  reverse_u16 (width, height, (const unsigned short *) blob,
		       (unsigned short *) buf);
	  break;
      case RL2_SAMPLE_INT32:
	  reverse_i32 (width, height, (const int *) blob, (int *) buf);
	  break;
      case RL2_SAMPLE_UINT32:
	  reverse_u32 (width, height, (const unsigned int *) blob,
		       (unsigned int *) buf);
	  break;
      case RL2_SAMPLE_FLOAT:
	  reverse_flt (width, height, (const float *) blob, (float *) buf);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  reverse_dbl (width, height, (const double *) blob, (double *) buf);
	  break;
      };
    *rev_blob = buf;
}

static int
execute_check (sqlite3 * sqlite, const char *sql)
{
/* executing an SQL statement returning True/False */
    sqlite3_stmt *stmt;
    int ret;
    int retcode = 0;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return SQLITE_ERROR;
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  if (sqlite3_column_int (stmt, 0) == 1)
	      retcode = 1;
      }
    sqlite3_finalize (stmt);
    if (retcode == 1)
	return SQLITE_OK;
    return SQLITE_ERROR;
}

static int
check_grid_i8 (unsigned int width, unsigned int x, unsigned int y,
	       const char *blob, const char *msg, char value)
{
/* testing an INT8 value */
    const char *p = blob + (y * width) + x;
    if (*p != value)
      {
	  fprintf (stderr, "%s: Unexpected value %d (expected %d)\n", msg, *p,
		   value);
	  return 0;
      }
    return 1;
}

static int
check_grid_u8 (unsigned int width, unsigned int x, unsigned int y,
	       const unsigned char *blob, const char *msg, unsigned char value)
{
/* testing an UINT8 value */
    const unsigned char *p = blob + (y * width) + x;
    if (*p != value)
      {
	  fprintf (stderr, "%s: Unexpected value %d (expected %d)\n", msg, *p,
		   value);
	  return 0;
      }
    return 1;
}

static int
check_grid_i16 (unsigned int width, unsigned int x, unsigned int y,
		const short *blob, int big_endian, const char *msg, short value)
{
/* testing an INT16 value */
    union cvt
    {
	unsigned char byte[2];
	short value;
    };
    union cvt cin;
    union cvt cout;
    const short *p = blob + (y * width) + x;
    short v = *p;
    if (is_big_endian_cpu () && big_endian)
	;
    else if (!is_big_endian_cpu () && !big_endian)
	;
    else
      {
	  /* inverting the endianness */
	  cin.value = *p;
	  cout.byte[0] = cin.byte[1];
	  cout.byte[1] = cin.byte[0];
	  v = cout.value;
      }
    if (v != value)
      {
	  fprintf (stderr, "%s: Unexpected value %d (expected %d)\n", msg, v,
		   value);
	  return 0;
      }
    return 1;
}

static int
check_grid_u16 (unsigned int width, unsigned int x, unsigned int y,
		const unsigned short *blob, int big_endian, const char *msg,
		unsigned short value)
{
/* testing an UINT16 value */
    union cvt
    {
	unsigned char byte[2];
	unsigned short value;
    };
    union cvt cin;
    union cvt cout;
    const unsigned short *p = blob + (y * width) + x;
    unsigned short v = *p;
    if (is_big_endian_cpu () && big_endian)
	;
    else if (!is_big_endian_cpu () && !big_endian)
	;
    else
      {
	  /* inverting the endianness */
	  cin.value = *p;
	  cout.byte[0] = cin.byte[1];
	  cout.byte[1] = cin.byte[0];
	  v = cout.value;
      }
    if (v != value)
      {
	  fprintf (stderr, "%s: Unexpected value %u (expected %u)\n", msg, v,
		   value);
	  return 0;
      }
    return 1;
}

static int
check_grid_i32 (unsigned int width, unsigned int x, unsigned int y,
		const int *blob, int big_endian, const char *msg, int value)
{
/* testing an INT32 value */
    union cvt
    {
	unsigned char byte[4];
	int value;
    };
    union cvt cin;
    union cvt cout;
    const int *p = blob + (y * width) + x;
    int v = *p;
    if (is_big_endian_cpu () && big_endian)
	;
    else if (!is_big_endian_cpu () && !big_endian)
	;
    else
      {
	  /* inverting the endianness */
	  cin.value = *p;
	  cout.byte[0] = cin.byte[3];
	  cout.byte[1] = cin.byte[2];
	  cout.byte[2] = cin.byte[1];
	  cout.byte[3] = cin.byte[0];
	  v = cout.value;
      }
    if (v != value)
      {
	  fprintf (stderr, "%s: Unexpected value %d (expected %d)\n", msg, v,
		   value);
	  return 0;
      }
    return 1;
}

static int
check_grid_u32 (unsigned int width, unsigned int x, unsigned int y,
		const unsigned int *blob, int big_endian, const char *msg,
		unsigned int value)
{
/* testing an UINT32 value */
    union cvt
    {
	unsigned char byte[4];
	unsigned int value;
    };
    union cvt cin;
    union cvt cout;
    const unsigned int *p = blob + (y * width) + x;
    unsigned int v = *p;
    if (is_big_endian_cpu () && big_endian)
	;
    else if (!is_big_endian_cpu () && !big_endian)
	;
    else
      {
	  /* inverting the endianness */
	  cin.value = *p;
	  cout.byte[0] = cin.byte[3];
	  cout.byte[1] = cin.byte[2];
	  cout.byte[2] = cin.byte[1];
	  cout.byte[3] = cin.byte[0];
	  v = cout.value;
      }
    if (v != value)
      {
	  fprintf (stderr, "%s: Unexpected value %u (expected %u)\n", msg, v,
		   value);
	  return 0;
      }
    return 1;
}

static int
check_grid_flt (unsigned int width, unsigned int x, unsigned int y,
		const float *blob, int big_endian, const char *msg, float value)
{
/* testing a FLOAT value */
    union cvt
    {
	unsigned char byte[4];
	float value;
    };
    union cvt cin;
    union cvt cout;
    const float *p = blob + (y * width) + x;
    float v = *p;
    if (is_big_endian_cpu () && big_endian)
	;
    else if (!is_big_endian_cpu () && !big_endian)
	;
    else
      {
	  /* inverting the endianness */
	  cin.value = *p;
	  cout.byte[0] = cin.byte[3];
	  cout.byte[1] = cin.byte[2];
	  cout.byte[2] = cin.byte[1];
	  cout.byte[3] = cin.byte[0];
	  v = cout.value;
      }
    if (v != value)
      {
	  fprintf (stderr, "%s: Unexpected value %1.6f (expected %1.6f)\n", msg,
		   v, value);
	  return 0;
      }
    return 1;
}

static int
check_grid_dbl (unsigned int width, unsigned int x, unsigned int y,
		const double *blob, int big_endian, const char *msg,
		double value)
{
/* testing a DOUBLE value */
    union cvt
    {
	unsigned char byte[8];
	double value;
    };
    union cvt cin;
    union cvt cout;
    const double *p = blob + (y * width) + x;
    double v = *p;
    if (is_big_endian_cpu () && big_endian)
	;
    else if (!is_big_endian_cpu () && !big_endian)
	;
    else
      {
	  /* inverting the endianness */
	  cin.value = *p;
	  cout.byte[0] = cin.byte[7];
	  cout.byte[1] = cin.byte[6];
	  cout.byte[2] = cin.byte[5];
	  cout.byte[3] = cin.byte[4];
	  cout.byte[4] = cin.byte[3];
	  cout.byte[5] = cin.byte[2];
	  cout.byte[6] = cin.byte[1];
	  cout.byte[7] = cin.byte[0];
	  v = cout.value;
      }
    if (v != value)
      {
	  fprintf (stderr, "%s: Unexpected value %1.6f (expected %1.6f)\n", msg,
		   v, value);
	  return 0;
      }
    return 1;
}

static int
check_grid_odd (unsigned int width, unsigned int x, unsigned int y,
		const unsigned char *blob, unsigned char sample, int big_endian,
		const char *msg)
{
/* checking a RAW sample (odd) */
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  return check_grid_i8 (width, x, y, (const char *) blob, msg, 123);
      case RL2_SAMPLE_UINT8:
	  return check_grid_u8 (width, x, y, blob, msg, 125);
      case RL2_SAMPLE_INT16:
	  return check_grid_i16 (width, x, y, (const short *) blob, big_endian,
				 msg, 12345);
      case RL2_SAMPLE_UINT16:
	  return check_grid_u16 (width, x, y, (const unsigned short *) blob,
				 big_endian, msg, 18345);
      case RL2_SAMPLE_INT32:
	  return check_grid_i32 (width, x, y, (const int *) blob, big_endian,
				 msg, 123456789);
      case RL2_SAMPLE_UINT32:
	  return check_grid_u32 (width, x, y, (const unsigned int *) blob,
				 big_endian, msg, 123457896);
      case RL2_SAMPLE_FLOAT:
	  return check_grid_flt (width, x, y, (const float *) blob, big_endian,
				 msg, 1234.5600);
      case RL2_SAMPLE_DOUBLE:
	  return check_grid_dbl (width, x, y, (const double *) blob, big_endian,
				 msg, 123456.7800);
      };
    return 0;
}

static int
check_grid_even (unsigned int width, unsigned int x, unsigned int y,
		 const unsigned char *blob, unsigned char sample,
		 int big_endian, const char *msg)
{
/* checking a RAW sample (even) */
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  return check_grid_i8 (width, x, y, (const char *) blob, msg, -98);
      case RL2_SAMPLE_UINT8:
	  return check_grid_u8 (width, x, y, blob, msg, 97);
      case RL2_SAMPLE_INT16:
	  return check_grid_i16 (width, x, y, (const short *) blob, big_endian,
				 msg, -9876);
      case RL2_SAMPLE_UINT16:
	  return check_grid_u16 (width, x, y, (const unsigned short *) blob,
				 big_endian, msg, 9746);
      case RL2_SAMPLE_INT32:
	  return check_grid_i32 (width, x, y, (const int *) blob, big_endian,
				 msg, -987654321);
      case RL2_SAMPLE_UINT32:
	  return check_grid_u32 (width, x, y, (const unsigned int *) blob,
				 big_endian, msg, 985432167);
      case RL2_SAMPLE_FLOAT:
	  return check_grid_flt (width, x, y, (const float *) blob, big_endian,
				 msg, -987.6500);
      case RL2_SAMPLE_DOUBLE:
	  return check_grid_dbl (width, x, y, (const double *) blob, big_endian,
				 msg, -98765.4300);
      };
    return 0;
}

static int
check_grid_size (unsigned int width, unsigned int height, unsigned char sample)
{
/* computing the expected RAW buffer size */
    int sample_sz = 1;
    int buf_sz = width * height;
    switch (sample)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  sample_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  sample_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample_sz = 8;
	  break;
      };
    return buf_sz * sample_sz;
}

static int
test_grid (sqlite3 * sqlite, unsigned char sample, int *retcode)
{
/* testing some DBMS Coverage */
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *err_msg = NULL;
    const char *coverage = NULL;
    const char *sample_name = NULL;
    const char *pixel_name = NULL;
    unsigned char num_bands = 1;
    const char *compression_name = NULL;
    int qlty = 100;
    char *sql;
    int tile_size = 512;
    unsigned int width = 2045;
    unsigned int height = 2543;
    unsigned char *blob;
    unsigned char *little_blob;
    unsigned char *big_blob;
    int blob_sz;
    int ok = -1;
    double x_res;
    double y_res;

/* setting the coverage name */
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  coverage = "grid_8";
	  break;
      case RL2_SAMPLE_UINT8:
	  coverage = "grid_u8";
	  break;
      case RL2_SAMPLE_INT16:
	  coverage = "grid_16";
	  break;
      case RL2_SAMPLE_UINT16:
	  coverage = "grid_u16";
	  break;
      case RL2_SAMPLE_INT32:
	  coverage = "grid_32";
	  break;
      case RL2_SAMPLE_UINT32:
	  coverage = "grid_u32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  coverage = "grid_flt";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  coverage = "grid_dbl";
	  break;
      };

/* preparing misc Coverage's parameters */
    pixel_name = "DATAGRID";
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  sample_name = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  sample_name = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  sample_name = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  sample_name = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  sample_name = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  sample_name = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  sample_name = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  sample_name = "DOUBLE";
	  break;
      };
    num_bands = 1;
    compression_name = "NONE";
    tile_size = 512;

/* creating the DBMS Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_CreateRasterCoverage("
			   "%Q, %Q, %Q, %d, %Q, %d, %d, %d, %d, %1.8f, %1.8f)",
			   coverage, sample_name, pixel_name, num_bands,
			   compression_name, qlty, tile_size, tile_size, 4326,
			   0.01, 0.01);
    ret = execute_check (sqlite, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoverage \"%s\" error: %s\n", coverage,
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode += -1;
	  return 0;
      }

/* preparing the RAW pixel buffer */
    switch (sample)
      {
      case RL2_SAMPLE_INT8:
	  build_grid_i8 (width, height, &blob, &blob_sz);
	  break;
      case RL2_SAMPLE_UINT8:
	  build_grid_u8 (width, height, &blob, &blob_sz);
	  break;
      case RL2_SAMPLE_INT16:
	  build_grid_i16 (width, height, &blob, &blob_sz);
	  break;
      case RL2_SAMPLE_UINT16:
	  build_grid_u16 (width, height, &blob, &blob_sz);
	  break;
      case RL2_SAMPLE_INT32:
	  build_grid_i32 (width, height, &blob, &blob_sz);
	  break;
      case RL2_SAMPLE_UINT32:
	  build_grid_u32 (width, height, &blob, &blob_sz);
	  break;
      case RL2_SAMPLE_FLOAT:
	  build_grid_flt (width, height, &blob, &blob_sz);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  build_grid_dbl (width, height, &blob, &blob_sz);
	  break;
      };
    if (is_big_endian_cpu ())
      {
	  big_blob = blob;
	  reverse_endian (width, height, blob, blob_sz, sample, &little_blob);
      }
    else
      {
	  little_blob = blob;
	  reverse_endian (width, height, blob, blob_sz, sample, &big_blob);
      }

/* Inserting RAW pixels - little endian buffer */
    sql = sqlite3_mprintf ("SELECT RL2_ImportSectionRawPixels("
			   "%Q, %Q, %d, %d, ?, BuildMbr(0, 0, ?, ?, %d), 1, 1, 0)",
			   coverage, "test little endian", width, height, 4326);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Import RAW \"%s\" (little) error: %s\n", coverage,
		   sqlite3_errmsg (sqlite));
	  *retcode += -2;
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, little_blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_double (stmt, 2, (double) width / 100.0);
    sqlite3_bind_double (stmt, 3, (double) height / 100.0);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      ok = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "Insert RAW (little); sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		*retcode += -3;
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (ok <= 0)
      {
	  *retcode += -4;
	  goto error;
      }
    free (little_blob);
    little_blob = NULL;

    ok = -1;
/* Inserting RAW pixels - big endian buffer */
    sql = sqlite3_mprintf ("SELECT RL2_ImportSectionRawPixels("
			   "%Q, %Q, %d, %d, ?, BuildMbr(0, 0, ?, ?, %d), 1, 1, 1)",
			   coverage, "test little endian", width, height, 4326);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Import RAW \"%s\" (big) error: %s\n", coverage,
		   sqlite3_errmsg (sqlite));
	  *retcode += -5;
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, big_blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_double (stmt, 2, -1.0 * (double) width / 100.0);
    sqlite3_bind_double (stmt, 3, -1.0 * (double) height / 100.0);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      ok = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "Insert RAW (big); sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		*retcode += -6;
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (ok <= 0)
      {
	  *retcode += -7;
	  goto error;
      }
    free (big_blob);
    big_blob = NULL;

    blob = NULL;
    blob_sz = 0;
/* Checking RAW pixels - little endian buffer / Section */
    sql = sqlite3_mprintf ("SELECT RL2_ExportSectionRawPixels("
			   "%Q, 1, %d, %d, BuildMbr(0, 0, ?, ?, %d), 0.01, 0.01, 0)",
			   coverage, width, height, 4326);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ExportSection RAW \"%s\" (little) error: %s\n",
		   coverage, sqlite3_errmsg (sqlite));
	  *retcode += -8;
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, (double) width / 100.0);
    sqlite3_bind_double (stmt, 2, (double) height / 100.0);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob = (unsigned char *) sqlite3_column_blob (stmt, 0);
		      blob_sz = sqlite3_column_bytes (stmt, 0);
		      if (blob_sz != check_grid_size (width, height, sample))
			{
			    fprintf (stderr,
				     "Unexpected ExportSection RAW (little) size: %d\n",
				     blob_sz);
			    *retcode += -9;
			    goto error;
			}
		      if (!check_grid_odd
			  (width, 101, 101, blob, sample, 0,
			   "ExportSection RAW (little)"))
			{
			    *retcode += -10;
			    goto error;
			}
		      if (!check_grid_even
			  (width, 101, 102, blob, sample, 0,
			   "ExportSection RAW (little)"))
			{
			    *retcode += -11;
			    goto error;
			}
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "ExportSection RAW (little); sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		*retcode += -12;
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (ok <= 0)
      {
	  fprintf (stderr, "ERROR: ExportSection RAW (little) \"%s\"\n",
		   coverage);
	  *retcode += -13;
	  goto error;
      }

    blob = NULL;
    blob_sz = 0;
/* Checking RAW pixels - big endian buffer / Coverage */
    sql = sqlite3_mprintf ("SELECT RL2_ExportRawPixels("
			   "%Q, %d, %d, MakePoint(?, ?, %d), 0.01, 0.01, 1)",
			   coverage, width, height, 4326);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Export RAW \"%s\" (big) error: %s\n",
		   coverage, sqlite3_errmsg (sqlite));
	  *retcode += -14;
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, (-1.0 * (double) width / 100.0) / 2.0);
    sqlite3_bind_double (stmt, 2, (-1.0 * (double) height / 100.0) / 2.0);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob = (unsigned char *) sqlite3_column_blob (stmt, 0);
		      blob_sz = sqlite3_column_bytes (stmt, 0);
		      if (blob_sz != check_grid_size (width, height, sample))
			{
			    fprintf (stderr,
				     "Unexpected Export RAW (big) size: %d\n",
				     blob_sz);
			    *retcode += -15;
			    goto error;
			}
		      if (!check_grid_odd
			  (width, 101, 101, blob, sample, 1,
			   "Export RAW (big)"))
			{
			    *retcode += -16;
			    goto error;
			}
		      if (!check_grid_even
			  (width, 101, 102, blob, sample, 1,
			   "ExportSection RAW (big)"))
			{
			    *retcode += -17;
			    goto error;
			}
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "Export RAW (big); sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		*retcode += -18;
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (ok <= 0)
      {
	  fprintf (stderr, "ERROR: Export RAW (big) \"%s\"\n", coverage);
	  *retcode += -19;
	  goto error;
      }

    blob = NULL;
    blob_sz = 0;
/* Checking RAW pixels - big endian buffer / Section */
    sql = sqlite3_mprintf ("SELECT RL2_ExportSectionRawPixels("
			   "%Q, 2, %d, %d, MakePoint(?, ?, %d), 0.01, 0.01, 1)",
			   coverage, width, height, 4326);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ExportSection RAW \"%s\" (big) error: %s\n",
		   coverage, sqlite3_errmsg (sqlite));
	  *retcode += -20;
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, (-1.0 * (double) width / 100.0) / 2.0);
    sqlite3_bind_double (stmt, 2, (-1.0 * (double) height / 100.0) / 2.0);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob = (unsigned char *) sqlite3_column_blob (stmt, 0);
		      blob_sz = sqlite3_column_bytes (stmt, 0);
		      if (blob_sz != check_grid_size (width, height, sample))
			{
			    fprintf (stderr,
				     "Unexpected ExportSection RAW (big) size: %d\n",
				     blob_sz);
			    *retcode += -21;
			    goto error;
			}
		      if (!check_grid_odd
			  (width, 101, 101, blob, sample, 1,
			   "ExportSection RAW (big)"))
			{
			    *retcode += -22;
			    goto error;
			}
		      if (!check_grid_even
			  (width, 101, 102, blob, sample, 1,
			   "ExportSection RAW (big)"))
			{
			    *retcode += -23;
			    goto error;
			}
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "ExportSection RAW (big); sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		*retcode += -24;
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (ok <= 0)
      {
	  fprintf (stderr, "ERROR: ExportSection RAW (big) \"%s\"\n", coverage);
	  *retcode += -25;
	  goto error;
      }

/* testing base-resolution - Coverage */
    if (rl2_resolve_base_resolution_from_dbms
	(sqlite, coverage, 0, 0, &x_res, &y_res) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get BaseResolution (Coverage)\n");
	  *retcode += -26;
	  goto error;
      }
    if (x_res != 0.01)
      {
	  fprintf (stderr,
		   "Unexpected BaseResolution (Coverage, horz): %1.6f\n",
		   x_res);
	  *retcode += -27;
	  goto error;
      }
    if (y_res != 0.01)
      {
	  fprintf (stderr,
		   "Unexpected BaseResolution (Coverage, vert): %1.6f\n",
		   y_res);
	  *retcode += -28;
	  goto error;
      }
/* testing base-resolution - Section */
    if (rl2_resolve_base_resolution_from_dbms
	(sqlite, coverage, 1, 2, &x_res, &y_res) != RL2_OK)
      {
	  fprintf (stderr, "ERROR: unable to get BaseResolution (Section)\n");
	  *retcode += -29;
	  goto error;
      }
    if (x_res != 0.01)
      {
	  fprintf (stderr, "Unexpected BaseResolution (Section, horz): %1.6f\n",
		   x_res);
	  *retcode += -30;
	  goto error;
      }
    if (y_res != 0.01)
      {
	  fprintf (stderr, "Unexpected BaseResolution (Sectuib, vert): %1.6f\n",
		   y_res);
	  *retcode += -31;
	  goto error;
      }

    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (little_blob != NULL)
	free (little_blob);
    if (big_blob != NULL)
	free (big_blob);
    return 0;
}

int
main (int argc, char *argv[])
{
    int result = 0;
    int ret;
    char *err_msg = NULL;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle,
			   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);
    ret =
	sqlite3_exec (db_handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -2;
      }
    ret =
	sqlite3_exec (db_handle, "SELECT CreateRasterCoveragesTable()", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoveragesTable() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -3;
      }

/*  RAW UINT8 (GRID) tests */
    ret = -100;
    if (!test_grid (db_handle, RL2_SAMPLE_INT8, &ret))
	return ret;
    ret = -200;
    if (!test_grid (db_handle, RL2_SAMPLE_UINT8, &ret))
	return ret;
    ret = -300;
    if (!test_grid (db_handle, RL2_SAMPLE_INT16, &ret))
	return ret;
    ret = -400;
    if (!test_grid (db_handle, RL2_SAMPLE_UINT16, &ret))
	return ret;
    ret = -500;
    if (!test_grid (db_handle, RL2_SAMPLE_INT32, &ret))
	return ret;
    ret = -600;
    if (!test_grid (db_handle, RL2_SAMPLE_UINT32, &ret))
	return ret;
    ret = -700;
    if (!test_grid (db_handle, RL2_SAMPLE_FLOAT, &ret))
	return ret;
    ret = -800;
    if (!test_grid (db_handle, RL2_SAMPLE_DOUBLE, &ret))
	return ret;

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
    spatialite_shutdown ();
    return result;
}
