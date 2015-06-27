/*

 rl2charls -- CharLS related functions

 version 0.1, 2014 September 7

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

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#ifndef OMIT_CHARLS		/* only if CharLS is enabled */

#ifdef __ANDROID__		/* Android specific */
#include <interface.h>
#else
#include <CharLS/interface.h>
#endif

static int
endianness ()
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
export_u16 (unsigned char *p, unsigned short value)
{
/* stores a 16bit int into a BLOB respecting declared endianness */
    int little_endian_arch = endianness ();
    union cvt
    {
	unsigned char byte[2];
	unsigned short int_value;
    } convert;
    convert.int_value = value;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  *(p + 0) = convert.byte[0];
	  *(p + 1) = convert.byte[1];
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  *(p + 1) = convert.byte[0];
	  *(p + 0) = convert.byte[1];
      }
}

static unsigned short
import_u16 (const unsigned char *p)
{
/* fetches a 16bit uint from BLOB respecting declared endiannes */
    int little_endian_arch = endianness ();
    union cvt
    {
	unsigned char byte[2];
	unsigned short int_value;
    } convert;
    if (little_endian_arch)
      {
	  /* Litte-Endian architecture [e.g. x86] */
	  convert.byte[0] = *(p + 0);
	  convert.byte[1] = *(p + 1);
      }
    else
      {
	  /* Big Endian architecture [e.g. PPC] */
	  convert.byte[0] = *(p + 1);
	  convert.byte[1] = *(p + 0);
      }
    return convert.int_value;
}

static void
prepare_ilv_buffer_16 (unsigned short *out, unsigned short *in, int width,
		       int height, int num_bands)
{
/* rearranging pixels by separate LINE components - UINT 16 */
    int x;
    int y;
    int ib;
    unsigned short *p_out = out;

    for (y = 0; y < height; y++)
      {
	  for (ib = 0; ib < num_bands; ib++)
	    {
		unsigned short *p_in = in + ((y * width * num_bands) + ib);
		for (x = 0; x < width; x++)
		  {
		      *p_out++ = *p_in;
		      p_in += num_bands;
		  }
	    }
      }
}

static void
prepare_ilv_buffer_8 (unsigned char *out, unsigned char *in, int width,
		      int height, int num_bands)
{
/* rearranging pixels by separate LINE components - UINT 8 */
    int x;
    int y;
    int ib;
    unsigned char *p_out = out;

    for (y = 0; y < height; y++)
      {
	  for (ib = 0; ib < num_bands; ib++)
	    {
		unsigned char *p_in = in + ((y * width * num_bands) + ib);
		for (x = 0; x < width; x++)
		  {
		      *p_out++ = *p_in;
		      p_in += num_bands;
		  }
	    }
      }
}

RL2_PRIVATE int
rl2_data_to_charls (const unsigned char *pixels, unsigned int width,
		    unsigned int height, unsigned char sample_type,
		    unsigned char pixel_type, unsigned char num_bands,
		    unsigned char **compr_data, int *compressed_size)
{
/* compressing a block of pixels as CharLS - strictly lossless */
    size_t in_sz;
    size_t out_sz;
    size_t compr_sz;
    void *in_buffer = NULL;
    unsigned char *out_buffer = NULL;
    int sample_sz = 1;
    struct JlsParameters params;

    if (sample_type == RL2_SAMPLE_UINT16)
	sample_sz = 2;
/* initializing CharLS params */
    memset (&params, 0, sizeof (params));
    params.width = width;
    params.height = height;
    if (sample_type == RL2_SAMPLE_UINT16)
	params.bitspersample = 16;
    else
	params.bitspersample = 8;
    params.bytesperline = width * num_bands * sample_sz;
    params.components = num_bands;
    params.allowedlossyerror = 0;
    if (num_bands == 1)
	params.ilv = ILV_NONE;
    else
	params.ilv = ILV_LINE;
    params.colorTransform = 0;
    params.outputBgr = 0;
    params.custom.MAXVAL = 0;
    params.custom.T1 = 0;
    params.custom.T2 = 0;
    params.custom.T3 = 0;
    params.custom.RESET = 0;
    params.jfif.Ver = 0;
/* allocating and populating the uncompressed buffer */
    in_sz = width * height * num_bands * sample_sz;
    in_buffer = malloc (in_sz);
    if (num_bands == 1)
	memcpy (in_buffer, pixels, in_sz);
    else
      {
	  if (sample_type == RL2_SAMPLE_UINT16)
	      prepare_ilv_buffer_16 ((unsigned short *) in_buffer,
				     (unsigned short *) pixels, width, height,
				     num_bands);
	  else
	      prepare_ilv_buffer_8 ((unsigned char *) in_buffer,
				    (unsigned char *) pixels, width, height,
				    num_bands);
      }
/* pre-allocating the compressed buffer */
    out_sz = width * height * num_bands * sample_sz;
    out_buffer = malloc (out_sz + 9);

    if (JpegLsEncode
	(out_buffer + 9, out_sz, &compr_sz, in_buffer, in_sz, &params) != 0)
	goto error;

/* installing the CharLS micro-header */
    *(out_buffer + 0) = RL2_CHARLS_START;
    *(out_buffer + 1) = sample_type;
    *(out_buffer + 2) = pixel_type;
    *(out_buffer + 3) = num_bands;
    export_u16 (out_buffer + 4, width);
    export_u16 (out_buffer + 6, height);
    *(out_buffer + 8) = RL2_CHARLS_END;

    *compr_data = out_buffer;
    *compressed_size = compr_sz + 9;
    free (in_buffer);
    return RL2_OK;

  error:
    if (in_buffer != NULL)
	free (in_buffer);
    if (out_buffer != NULL)
	free (out_buffer);
    return RL2_ERROR;
}

static void
from_ilv_buffer_16 (unsigned short *out, unsigned short *in, int width,
		    int height, int num_bands)
{
/* rearranging pixels from separate LINE components - UINT 16 */
    int x;
    int y;
    int ib;
    unsigned short *p_in = in;

    for (y = 0; y < height; y++)
      {
	  for (ib = 0; ib < num_bands; ib++)
	    {
		unsigned short *p_out = out + ((y * width * num_bands) + ib);
		for (x = 0; x < width; x++)
		  {
		      *p_out = *p_in++;
		      p_out += num_bands;
		  }
	    }
      }
}

static void
from_ilv_buffer_8 (unsigned char *out, unsigned char *in, int width,
		   int height, int num_bands)
{
/* rearranging pixels from separate LINE components - UINT 8 */
    int x;
    int y;
    int ib;
    unsigned char *p_in = in;

    for (y = 0; y < height; y++)
      {
	  for (ib = 0; ib < num_bands; ib++)
	    {
		unsigned char *p_out = out + ((y * width * num_bands) + ib);
		for (x = 0; x < width; x++)
		  {
		      *p_out = *p_in++;
		      p_out += num_bands;
		  }
	    }
      }
}

RL2_PRIVATE int
rl2_decode_charls (const unsigned char *charls_buf, int charls_sz,
		   unsigned int *width, unsigned int *height,
		   unsigned char *sample_type, unsigned char *pixel_type,
		   unsigned char *num_bands, unsigned char **pixels,
		   int *pixels_sz)
{
/* decompressing a block of pixels from CharLS */
    size_t out_sz;
    void *out_buffer = NULL;
    int sample_sz = 1;
    struct JlsParameters params;

/* checking the CharLS micro-header for validity */
    if (charls_sz < 10)
	return RL2_ERROR;
    if (*(charls_buf + 0) == RL2_CHARLS_START
	&& *(charls_buf + 8) == RL2_CHARLS_END)
	;
    else
	return RL2_ERROR;

/* retrieving SampleType, PixelType and NumBands */
    *sample_type = RL2_SAMPLE_UNKNOWN;
    *pixel_type = RL2_PIXEL_UNKNOWN;
    *num_bands = 0;
    *width = import_u16 (charls_buf + 4);
    *height = import_u16 (charls_buf + 6);
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT8
	&& *(charls_buf + 2) == RL2_PIXEL_GRAYSCALE && *(charls_buf + 3) == 1)
      {
	  /* GRAYSCALE 8 bits */
	  *sample_type = RL2_SAMPLE_UINT8;
	  *pixel_type = RL2_PIXEL_GRAYSCALE;
	  *num_bands = 1;
      }
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT8
	&& *(charls_buf + 2) == RL2_PIXEL_DATAGRID && *(charls_buf + 3) == 1)
      {
	  /* DATAGRID 8 bits */
	  *sample_type = RL2_SAMPLE_UINT8;
	  *pixel_type = RL2_PIXEL_DATAGRID;
	  *num_bands = 1;
      }
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT16
	&& *(charls_buf + 2) == RL2_PIXEL_DATAGRID && *(charls_buf + 3) == 1)
      {
	  /* GRAYSCALE 16 bits */
	  *sample_type = RL2_SAMPLE_UINT16;
	  *pixel_type = RL2_PIXEL_DATAGRID;
	  *num_bands = 1;
      }
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT8
	&& *(charls_buf + 2) == RL2_PIXEL_RGB && *(charls_buf + 3) == 3)
      {
	  /* RGB 8 bits */
	  *sample_type = RL2_SAMPLE_UINT8;
	  *pixel_type = RL2_PIXEL_RGB;
	  *num_bands = 3;
      }
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT16
	&& *(charls_buf + 2) == RL2_PIXEL_RGB && *(charls_buf + 3) == 3)
      {
	  /* RGB 16 bits */
	  *sample_type = RL2_SAMPLE_UINT16;
	  *pixel_type = RL2_PIXEL_RGB;
	  *num_bands = 3;
      }
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT8
	&& *(charls_buf + 2) == RL2_PIXEL_MULTIBAND && *(charls_buf + 3) == 3)
      {
	  /* 3-bands 8 bits */
	  *sample_type = RL2_SAMPLE_UINT8;
	  *pixel_type = RL2_PIXEL_MULTIBAND;
	  *num_bands = 3;
      }
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT16
	&& *(charls_buf + 2) == RL2_PIXEL_MULTIBAND && *(charls_buf + 3) == 3)
      {
	  /* 3-bands 16 bits */
	  *sample_type = RL2_SAMPLE_UINT16;
	  *pixel_type = RL2_PIXEL_MULTIBAND;
	  *num_bands = 3;
      }
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT8
	&& *(charls_buf + 2) == RL2_PIXEL_MULTIBAND && *(charls_buf + 3) == 4)
      {
	  /* 4-bands 8 bits */
	  *sample_type = RL2_SAMPLE_UINT8;
	  *pixel_type = RL2_PIXEL_MULTIBAND;
	  *num_bands = 4;
      }
    if (*(charls_buf + 1) == RL2_SAMPLE_UINT16
	&& *(charls_buf + 2) == RL2_PIXEL_MULTIBAND && *(charls_buf + 3) == 4)
      {
	  /* 4-bands 16 bits */
	  *sample_type = RL2_SAMPLE_UINT16;
	  *pixel_type = RL2_PIXEL_MULTIBAND;
	  *num_bands = 4;
      }
    if (*sample_type == RL2_SAMPLE_UNKNOWN || *pixel_type == RL2_PIXEL_UNKNOWN
	|| *num_bands == 0)
	goto error;

    if (*sample_type == RL2_SAMPLE_UINT16)
	sample_sz = 2;
/* initializing CharLS params */
    memset (&params, 0, sizeof (params));
    params.width = *width;
    params.height = *height;
    if (*sample_type == RL2_SAMPLE_UINT16)
	params.bitspersample = 16;
    else
	params.bitspersample = 8;
    params.bytesperline = *width * *num_bands * sample_sz;
    params.components = *num_bands;
    params.allowedlossyerror = 0;
    if (*num_bands == 1)
	params.ilv = ILV_NONE;
    else
	params.ilv = ILV_LINE;
    params.colorTransform = 0;
    params.outputBgr = 0;
    params.custom.MAXVAL = 0;
    params.custom.T1 = 0;
    params.custom.T2 = 0;
    params.custom.T3 = 0;
    params.custom.RESET = 0;
/* pre-allocating the uncompressed buffer */
    out_sz = *width * *height * *num_bands * sample_sz;
    out_buffer = malloc (out_sz);

    if (JpegLsDecode
	(out_buffer, out_sz, charls_buf + 9, charls_sz - 9, &params) != 0)
	goto error;

    *pixels = malloc (out_sz);
    *pixels_sz = out_sz;
    if (*num_bands == 1)
	memcpy (*pixels, out_buffer, out_sz);
    else
      {
	  if (*sample_type == RL2_SAMPLE_UINT16)
	      from_ilv_buffer_16 ((unsigned short *) (*pixels),
				  (unsigned short *) out_buffer, *width,
				  *height, *num_bands);
	  else
	      from_ilv_buffer_8 ((unsigned char *) (*pixels),
				 (unsigned char *) out_buffer, *width,
				 *height, *num_bands);
      }
    free (out_buffer);
    return RL2_OK;

  error:
    if (out_buffer != NULL)
	free (out_buffer);
    return RL2_ERROR;
}

#endif /* end CharLS conditional */
