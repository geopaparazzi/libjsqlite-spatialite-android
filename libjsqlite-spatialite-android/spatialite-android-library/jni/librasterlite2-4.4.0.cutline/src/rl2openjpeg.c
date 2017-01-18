/*

 rl2openjpeg -- OpenJPEG (JPEG2000) related functions

 version 0.1, 2014 September 11

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
#include "rasterlite2/rl2tiff.h"
#include "rasterlite2_private.h"

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */

#ifdef HAVE_OPENJPEG_2_1_OPENJPEG_H
#include <openjpeg-2.1/openjpeg.h>
#else
#ifdef __ANDROID__		/* Android specific */
#include <openjpeg.h>
#else
#include <openjpeg-2.0/openjpeg.h>
#endif
#endif

struct jp2_memfile
{
/* a struct emulating a file [memory mapped] */
    unsigned char *buffer;
    int malloc_block;
    tsize_t size;
    tsize_t eof;
    toff_t current;
};

static void
info_callback (const char *msg, void *unused)
{
    if (unused != NULL || msg == NULL)
	unused = NULL;		/* suppressing stupid compiler warnings */
    /* fprintf (stderr, "OpenJpeg Info: %s\n", msg); */
}

static void
warning_callback (const char *msg, void *unused)
{
    if (unused != NULL)
	unused = NULL;		/* suppressing stupid compiler warnings */
    fprintf (stderr, "OpenJpeg Warning: %s\n", msg);
}

static void
error_callback (const char *msg, void *unused)
{
    if (unused != NULL)
	unused = NULL;		/* suppressing stupid compiler warnings */
    fprintf (stderr, "OpenJpeg Error: %s\n", msg);
}

static void
memory_realloc (struct jp2_memfile *mem, tsize_t req_size)
{
/* expanding the allocated memory */
    unsigned char *new_buffer;
    tsize_t new_size = mem->size;
    while (new_size <= req_size)
	new_size += mem->malloc_block;
    new_buffer = realloc (mem->buffer, new_size);
    if (!new_buffer)
	return;
    mem->buffer = new_buffer;
    memset (mem->buffer + mem->size, '\0', new_size - mem->size);
    mem->size = new_size;
}

static OPJ_SIZE_T
write_callback (void *buffer, OPJ_SIZE_T size, void *data)
{
/* emulating the write()  function */
    struct jp2_memfile *mem = data;
    if ((mem->current + size) >= (toff_t) mem->size)
	memory_realloc (mem, mem->current + size);
    if ((mem->current + size) >= (toff_t) mem->size)
	return 0;
    memcpy (mem->buffer + mem->current, (unsigned char *) buffer, size);
    mem->current += size;
    if (mem->current > (toff_t) mem->eof)
	mem->eof = (tsize_t) (mem->current);
    return size;
}

static OPJ_SIZE_T
read_callback (void *buffer, OPJ_SIZE_T size, void *data)
{
/* emulating the read()  function */
    struct jp2_memfile *mem = data;
    tsize_t len;
    if (mem->current >= (toff_t) mem->eof)
	return 0;
    len = size;
    if ((mem->current + size) >= (toff_t) mem->eof)
	len = (tsize_t) (mem->eof - mem->current);
    memcpy (buffer, mem->buffer + mem->current, len);
    mem->current += len;
    return len;
}

static OPJ_BOOL
seek_callback (OPJ_OFF_T offset, void *data)
{
/* emulating the lseek()  function */
    struct jp2_memfile *mem = data;
    mem->current = offset;
    if ((toff_t) mem->eof < mem->current)
	mem->eof = (tsize_t) (mem->current);
    return 1;
}

static OPJ_OFF_T
skip_callback (OPJ_OFF_T size, void *data)
{
/* skip  function */
    struct jp2_memfile *mem = data;
    mem->current += size;
    if ((toff_t) mem->eof < mem->current)
	mem->eof = (tsize_t) (mem->current);
    return size;
}

static int
check_jpeg2000_compatibility (unsigned char sample_type,
			      unsigned char pixel_type,
			      unsigned char num_samples)
{
/* checks for Jpeg2000 compatibility */
    switch (sample_type)
      {
      case RL2_SAMPLE_UINT8:
      case RL2_SAMPLE_UINT16:
	  break;
      default:
	  return RL2_ERROR;
      };
    switch (pixel_type)
      {
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
      case RL2_PIXEL_MULTIBAND:
      case RL2_PIXEL_DATAGRID:
	  break;
      default:
	  return RL2_ERROR;
      };
    if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_DATAGRID)
      {
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_RGB)
      {
	  if (num_samples != 3)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_MULTIBAND)
      {
	  if (num_samples != 3 && num_samples != 4)
	      return RL2_ERROR;
      }
    return RL2_OK;
}

static void
copy_tile_u16 (unsigned short *tile_buf, rl2PrivRasterPtr rst, int start_x,
	       int start_y, int tile_w, int tile_h)
{
/* preparing an UINT16 Jpeg2000 tile */
    int x;
    int y;
    int ib;
    unsigned short *p_in;
    unsigned short *p_out;

    p_out = tile_buf;
    for (ib = 0; ib < rst->nBands; ib++)
      {
	  p_out = tile_buf + (ib * tile_w * tile_h);
	  for (y = 0; y < tile_h; y++)
	    {
		int base_y = start_y + y;
		if (base_y >= (int) (rst->height))
		    break;
		for (x = 0; x < tile_w; x++)
		  {
		      int base_x = start_x + x;
		      if (base_x >= (int) (rst->width))
			{
			    p_out++;
			    continue;
			}
		      p_in = (unsigned short *) (rst->rasterBuffer);
		      p_in +=
			  (base_y * rst->width * rst->nBands) +
			  (base_x * rst->nBands) + ib;
		      *p_out++ = *p_in;
		  }
	    }
      }
}

static void
copy_tile_u8 (unsigned char *tile_buf, rl2PrivRasterPtr rst, int start_x,
	      int start_y, int tile_w, int tile_h)
{
/* preparing an UINT8 Jpeg2000 tile */
    int x;
    int y;
    int ib;
    unsigned char *p_in;
    unsigned char *p_out;

    p_out = tile_buf;
    for (ib = 0; ib < rst->nBands; ib++)
      {
	  p_out = tile_buf + (ib * tile_w * tile_h);
	  for (y = 0; y < tile_h; y++)
	    {
		int base_y = start_y + y;
		if (base_y >= (int) (rst->height))
		    break;
		for (x = 0; x < tile_w; x++)
		  {
		      int base_x = start_x + x;
		      if (base_x >= (int) (rst->width))
			{
			    p_out++;
			    continue;
			}
		      p_in = (unsigned char *) (rst->rasterBuffer);
		      p_in +=
			  (base_y * rst->width * rst->nBands) +
			  (base_x * rst->nBands) + ib;
		      *p_out++ = *p_in;
		  }
	    }
      }
}

static int
compress_jpeg2000 (rl2RasterPtr ptr, unsigned char **jpeg2000,
		   int *jpeg2000_size, int quality, int lossy)
{
/* compressing a Jpeg2000 image */
    opj_codec_t *codec = NULL;
    opj_image_t *image = NULL;
    opj_image_cmptparm_t *band_params = NULL;
    opj_stream_t *stream = NULL;
    opj_cparameters_t parameters;
    OPJ_COLOR_SPACE color_space;
    int ib;
    int jp2_tile_x = 1024;
    int jp2_tile_y = 1024;
    int x;
    int y;
    int tile_no = 0;
    double rate = 100.0 / (double) quality;
    void *tile_buf = NULL;
    int tile_sz;
    struct jp2_memfile clientdata;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;

    if ((int) (rst->width) < jp2_tile_x)
	jp2_tile_x = (int) (rst->width);
    if ((int) (rst->height) < jp2_tile_y)
	jp2_tile_y = (int) (rst->height);
/* initializing the memory Write struct */
    clientdata.buffer = NULL;
    clientdata.malloc_block = 1024;
    clientdata.size = 0;
    clientdata.eof = 0;
    clientdata.current = 0;

/* creating and initializing the Jpeg2000 codec */
    codec = opj_create_compress (OPJ_CODEC_JP2);
    if (codec == NULL)
	return RL2_ERROR;
    opj_set_info_handler (codec, info_callback, NULL);
    opj_set_warning_handler (codec, warning_callback, NULL);
    opj_set_error_handler (codec, error_callback, NULL);

    band_params = malloc (rst->nBands * sizeof (opj_image_cmptparm_t));
    for (ib = 0; ib < rst->nBands; ib++)
      {
	  band_params[ib].x0 = 0;
	  band_params[ib].y0 = 0;
	  band_params[ib].dx = 1;
	  band_params[ib].dy = 1;
	  band_params[ib].w = rst->width;
	  band_params[ib].h = rst->height;
	  band_params[ib].sgnd = 0;
	  if (rst->sampleType == RL2_SAMPLE_UINT16)
	      band_params[ib].prec = 16;
	  else
	      band_params[ib].prec = 8;
      }
    if (rst->nBands == 1)
	color_space = OPJ_CLRSPC_GRAY;
    else
	color_space = OPJ_CLRSPC_SRGB;
    image = opj_image_tile_create (rst->nBands, band_params, color_space);
    free (band_params);
    band_params = NULL;
    if (image == NULL)
      {
	  fprintf (stderr, "OpenJpeg Error: opj_image_tile_create() failed\n");
	  opj_destroy_codec (codec);
	  return RL2_ERROR;
      }
    image->x0 = 0;
    image->y0 = 0;
    image->x1 = rst->width;
    image->y1 = rst->height;
    image->color_space = color_space;
    image->numcomps = rst->nBands;

    opj_set_default_encoder_parameters (&parameters);
    parameters.cp_disto_alloc = 1;
    parameters.tcp_numlayers = 1;
    parameters.tcp_rates[0] = rate;
    parameters.cp_tx0 = 0;
    parameters.cp_ty0 = 0;
    parameters.tile_size_on = 1;
    parameters.cp_tdx = jp2_tile_x;
    parameters.cp_tdy = jp2_tile_y;
    if (!lossy)
	parameters.irreversible = 0;
    else
	parameters.irreversible = 1;
    parameters.numresolution = 4;
    parameters.prog_order = OPJ_LRCP;

    if (!opj_setup_encoder (codec, &parameters, image))
      {
	  fprintf (stderr, "OpenJpeg Error: opj_setup_encoder() failed\n");
	  opj_image_destroy (image);
	  opj_destroy_codec (codec);
	  return RL2_ERROR;
      }

/* preparing the output stream */
    stream = opj_stream_create (1024 * 1024, 0);
    opj_stream_set_write_function (stream, write_callback);
    opj_stream_set_seek_function (stream, seek_callback);
    opj_stream_set_skip_function (stream, skip_callback);
#ifdef OPENJPEG_2_1
    opj_stream_set_user_data (stream, &clientdata, NULL);
#else
    opj_stream_set_user_data (stream, &clientdata);
#endif

    if (!opj_start_compress (codec, image, stream))
      {
	  fprintf (stderr, "OpenJpeg Error: opj_start_compress() failed\n");
	  goto error;
      }

/* compressing by tiles */
    if (rst->sampleType == RL2_SAMPLE_UINT16)
	tile_sz = jp2_tile_x * jp2_tile_y * rst->nBands * 2;
    else
	tile_sz = jp2_tile_x * jp2_tile_y * rst->nBands;
    tile_buf = malloc (tile_sz);
    memset (tile_buf, 0, tile_sz);
    for (y = 0; y < (int) (rst->height); y += jp2_tile_y)
      {
	  for (x = 0; x < (int) (rst->width); x += jp2_tile_x)
	    {
		if (rst->sampleType == RL2_SAMPLE_UINT16)
		    copy_tile_u16 (tile_buf, rst, x, y, jp2_tile_x, jp2_tile_y);
		else
		    copy_tile_u8 (tile_buf, rst, x, y, jp2_tile_x, jp2_tile_y);
		if (!opj_write_tile
		    (codec, tile_no++, tile_buf, tile_sz, stream))
		  {
		      fprintf (stderr,
			       "OpenJpeg Error: opj_write_tile() failed\n");
		      goto error;
		  }
	    }
      }
    free (tile_buf);

    if (!opj_end_compress (codec, stream))
      {
	  fprintf (stderr, "OpenJpeg Error: opj_end_compress() failed\n");
	  goto error;
      }
    opj_stream_destroy (stream);
    opj_image_destroy (image);
    opj_destroy_codec (codec);

    *jpeg2000 = clientdata.buffer;
    *jpeg2000_size = clientdata.eof;

    return RL2_OK;

  error:
    opj_stream_destroy (stream);
    opj_image_destroy (image);
    opj_destroy_codec (codec);
    if (clientdata.buffer != NULL)
	free (clientdata.buffer);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_section_to_lossy_jpeg2000 (rl2SectionPtr scn, const char *path, int quality)
{
/* attempting to save a raster section into a Jpeg2000 (lossy) file */
    int blob_size;
    unsigned char *blob;
    rl2RasterPtr rst;
    int ret;

    if (scn == NULL)
	return RL2_ERROR;
    rst = rl2_get_section_raster (scn);
    if (rst == NULL)
	return RL2_ERROR;
/* attempting to export as a Jpeg2000 image */
    if (rl2_raster_to_lossy_jpeg2000 (rst, &blob, &blob_size, quality) !=
	RL2_OK)
	return RL2_ERROR;
    ret = rl2_blob_to_file (path, blob, blob_size);
    free (blob);
    if (ret != RL2_OK)
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_to_lossy_jpeg2000 (rl2RasterPtr rst, unsigned char **jpeg2000,
			      int *jpeg2000_size, int quality)
{
/* creating a Jpeg2000 image (lossy) from a raster */
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_samples;
    unsigned char *blob;
    int blob_size;

    if (rst == NULL)
	return RL2_ERROR;
    if (rl2_get_raster_type (rst, &sample_type, &pixel_type, &num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (check_jpeg2000_compatibility (sample_type, pixel_type, num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (compress_jpeg2000 (rst, &blob, &blob_size, quality, 1) != RL2_OK)
	return RL2_ERROR;
    *jpeg2000 = blob;
    *jpeg2000_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE int
rl2_section_to_lossless_jpeg2000 (rl2SectionPtr scn, const char *path)
{
/* attempting to save a raster section into a Jpeg2000 (lossless) file */
    int blob_size;
    unsigned char *blob;
    rl2RasterPtr rst;
    int ret;

    if (scn == NULL)
	return RL2_ERROR;
    rst = rl2_get_section_raster (scn);
    if (rst == NULL)
	return RL2_ERROR;
/* attempting to export as a Jpeg2000 image */
    if (rl2_raster_to_lossless_jpeg2000 (rst, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    ret = rl2_blob_to_file (path, blob, blob_size);
    free (blob);
    if (ret != RL2_OK)
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_to_lossless_jpeg2000 (rl2RasterPtr rst, unsigned char **jpeg2000,
				 int *jpeg2000_size)
{
/* creating a Jpeg2000 image (lossless) from a raster */
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_samples;
    unsigned char *blob;
    int blob_size;

    if (rst == NULL)
	return RL2_ERROR;
    if (rl2_get_raster_type (rst, &sample_type, &pixel_type, &num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (check_jpeg2000_compatibility (sample_type, pixel_type, num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (compress_jpeg2000 (rst, &blob, &blob_size, 100, 0) != RL2_OK)
	return RL2_ERROR;
    *jpeg2000 = blob;
    *jpeg2000_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE rl2SectionPtr
rl2_section_from_jpeg2000 (const char *path, unsigned char sample_type,
			   unsigned char pixel_type, unsigned char num_bands)
{
/* attempting to create a raster section from a Jpeg2000 file */
    int blob_size;
    unsigned char *blob;
    rl2SectionPtr scn;
    rl2RasterPtr rst;

/* attempting to create a raster */
    if (rl2_blob_from_file (path, &blob, &blob_size) != RL2_OK)
	return NULL;
    rst =
	rl2_raster_from_jpeg2000 (blob, blob_size, sample_type, pixel_type,
				  num_bands);
    free (blob);
    if (rst == NULL)
	return NULL;

/* creating the raster section */
    scn =
	rl2_create_section (path, RL2_COMPRESSION_LOSSY_JP2,
			    RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			    rst);
    return scn;
}

RL2_DECLARE rl2RasterPtr
rl2_raster_from_jpeg2000 (const unsigned char *jpeg2000, int jpeg2000_size,
			  unsigned char sample_type, unsigned char pixel_type,
			  unsigned char num_bands)
{
/* attempting to create a raster from a Jpeg2000 image */
    rl2RasterPtr rst = NULL;
    unsigned char *buf;
    int buf_size;
    unsigned int width;
    unsigned int height;

    if (rl2_decode_jpeg2000_scaled
	(1, jpeg2000, jpeg2000_size, &width, &height, sample_type, pixel_type,
	 num_bands, &buf, &buf_size) != RL2_OK)
	return NULL;

/* creating the raster */
    rst =
	rl2_create_raster (width, height, sample_type, pixel_type, num_bands,
			   buf, buf_size, NULL, NULL, 0, NULL);
    if (rst == NULL)
      {
	  free (buf);
      }
    return rst;
}

static void
save_tile_u16 (unsigned short *buf, unsigned short *jp2_data, int start_x,
	       int start_y, int end_x, int end_y, int num_bands, int width,
	       int height)
{
/* copying pixels from a Jpeg2000 tile into the output raster - UINT16 */
    int x;
    int y;
    int ib;
    unsigned short *p_in = jp2_data;
    unsigned short *p_out;

    for (ib = 0; ib < num_bands; ib++)
      {
	  for (y = start_y; y < end_y; y++)
	    {
		if (y >= height)
		  {
		      for (x = start_x; x < end_x; x++)
			  p_in++;
		      continue;
		  }
		for (x = start_x; x < end_x; x++)
		  {
		      if (x >= width)
			{
			    p_in++;
			    continue;
			}
		      p_out =
			  buf + (y * width * num_bands) + (x * num_bands) + ib;
		      *p_out = *p_in++;
		  }
	    }
      }
}

static void
save_tile_u8 (unsigned char *buf, unsigned char *jp2_data, int start_x,
	      int start_y, int end_x, int end_y, int num_bands, int width,
	      int height)
{
/* copying pixels from a Jpeg2000 tile into the output raster - UINT8 */
    int x;
    int y;
    int ib;
    unsigned char *p_in = jp2_data;
    unsigned char *p_out;

    for (ib = 0; ib < num_bands; ib++)
      {
	  for (y = start_y; y < end_y; y++)
	    {
		if (y >= height)
		  {
		      for (x = start_x; x < end_x; x++)
			  p_in++;
		      continue;
		  }
		for (x = start_x; x < end_x; x++)
		  {
		      if (x >= width)
			{
			    p_in++;
			    continue;
			}
		      p_out =
			  buf + (y * width * num_bands) + (x * num_bands) + ib;
		      *p_out = *p_in++;
		  }
	    }
      }
}

RL2_PRIVATE int
rl2_decode_jpeg2000_scaled (int scale, const unsigned char *jpeg2000,
			    int jpeg2000_sz, unsigned int *xwidth,
			    unsigned int *xheight, unsigned char xsample_type,
			    unsigned char xpixel_type,
			    unsigned char xnum_bands, unsigned char **pixels,
			    int *pixels_size)
{
/* attempting to create a raster from a Jpeg2000 image - supporting rescaled size */
    unsigned char *buf = NULL;
    int buf_size = 0;
    unsigned int width;
    unsigned int height;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned char num_bands;
    unsigned char sample_sz = 1;
    struct jp2_memfile clientdata;
    opj_codec_t *codec;
    opj_dparameters_t parameters;
    opj_stream_t *stream;
    opj_image_t *image = NULL;
    opj_codestream_info_v2_t *code_stream_info;
    OPJ_UINT32 nComponents;
    OPJ_UINT32 tile_index;
    OPJ_UINT32 data_size;
    OPJ_INT32 tile_x0;
    OPJ_INT32 tile_y0;
    OPJ_INT32 tile_x1;
    OPJ_INT32 tile_y1;
    OPJ_UINT32 nb_comps;
    OPJ_BOOL should_go_on;
    OPJ_BYTE *jp2_data = NULL;
    int nResolutions;
    int level;

    if (scale == 1)
	level = 0;
    else if (scale == 2)
	level = 1;
    else if (scale == 4)
	level = 2;
    else if (scale == 8)
	level = 3;
    else
	return RL2_ERROR;

/* creating and initializing the Jpeg2000 decoder */
    codec = opj_create_decompress (OPJ_CODEC_JP2);
    opj_set_info_handler (codec, info_callback, NULL);
    opj_set_warning_handler (codec, warning_callback, NULL);
    opj_set_error_handler (codec, error_callback, NULL);
    opj_set_default_decoder_parameters (&parameters);
    if (!opj_setup_decoder (codec, &parameters))
	return RL2_ERROR;

/* preparing the input stream */
    stream = opj_stream_create (1024, 1);
    opj_stream_set_user_data_length (stream, jpeg2000_sz);
    opj_stream_set_read_function (stream, read_callback);
    opj_stream_set_seek_function (stream, seek_callback);
    opj_stream_set_skip_function (stream, skip_callback);
/* initializing the memory Read struct */
    clientdata.buffer = (unsigned char *) jpeg2000;
    clientdata.malloc_block = 1024;
    clientdata.size = jpeg2000_sz;
    clientdata.eof = jpeg2000_sz;
    clientdata.current = 0;
#ifdef OPENJPEG_2_1
    opj_stream_set_user_data (stream, &clientdata, NULL);
#else
    opj_stream_set_user_data (stream, &clientdata);
#endif
    if (!opj_read_header (stream, codec, &image))
      {
	  fprintf (stderr, "OpenJpeg Error: opj_read_header() failed\n");
	  goto error;
      }
    code_stream_info = opj_get_cstr_info (codec);
    nComponents = code_stream_info->nbcomps;
    nResolutions =
	code_stream_info->m_default_tile_info.tccp_info[0].numresolutions;
    opj_destroy_cstr_info (&code_stream_info);
    if (image == NULL)
	goto error;
    if (nResolutions < 4)
	goto error;
    if (image->comps[0].prec == 16 && image->comps[0].sgnd == 0)
	sample_type = RL2_SAMPLE_UINT16;
    if (image->comps[0].prec == 8 && image->comps[0].sgnd == 0)
	sample_type = RL2_SAMPLE_UINT8;
    if (nComponents == 1)
      {
	  if (sample_type == RL2_SAMPLE_UINT16)
	      pixel_type = RL2_PIXEL_DATAGRID;
	  if (sample_type == RL2_SAMPLE_UINT8)
	    {
		pixel_type = RL2_PIXEL_GRAYSCALE;
		if (xpixel_type == RL2_PIXEL_DATAGRID)
		    pixel_type = RL2_PIXEL_DATAGRID;
	    }
	  num_bands = 1;
      }
    if (nComponents == 3)
      {
	  pixel_type = RL2_PIXEL_RGB;
	  if (xpixel_type == RL2_PIXEL_MULTIBAND)
	      pixel_type = RL2_PIXEL_MULTIBAND;
	  num_bands = 3;
      }
    if (nComponents == 4)
      {
	  pixel_type = RL2_PIXEL_MULTIBAND;
	  num_bands = 4;
      }
    if (sample_type != xsample_type || pixel_type != xpixel_type
	|| num_bands != xnum_bands)
	goto error;

/* validating the Sample and Pixel types */
    if (sample_type == RL2_SAMPLE_UNKNOWN || pixel_type != xpixel_type)
      {
	  fprintf (stderr,
		   "OpenJpeg Error: invalid Sample/Pixel/Bands layout\n");
	  goto error;
      }
    width = image->comps[0].w;
    height = image->comps[0].h;
    if (!opj_set_decoded_resolution_factor (codec, level))
      {
	  fprintf (stderr,
		   "OpenJpeg Error: opj_set_decoded_resolution_factor() failed");
	  goto error;
      }

/* allocating the raster buffer */
    if (sample_type == RL2_SAMPLE_UINT16)
	sample_sz = 2;
    buf_size = (width / scale) * (height / scale) * num_bands * sample_sz;
    buf = malloc (buf_size);
    memset (buf, 0, buf_size);

    while (1)
      {
	  /* decoding one JP2 tile at each time */
	  if (!opj_read_tile_header
	      (codec, stream, &tile_index, &data_size, &tile_x0, &tile_y0,
	       &tile_x1, &tile_y1, &nb_comps, &should_go_on))
	    {
		fprintf (stderr,
			 "OpenJpeg Error: opj_read_tile_header() failed\n");
		goto error;
	    }
	  if (!should_go_on)
	      break;
	  if (nb_comps != num_bands)
	    {
		fprintf (stderr, "OpenJpeg Error: unexpected nb_comps !!!\n");
		goto error;
	    }
	  jp2_data = malloc (data_size);
	  if (!opj_decode_tile_data
	      (codec, tile_index, jp2_data, data_size, stream))
	    {
		fprintf (stderr,
			 "OpenJpeg Error: opj_decode_tile_data() failed\n");
		goto error;
	    }
	  if (sample_type == RL2_SAMPLE_UINT16)
	      save_tile_u16 ((unsigned short *) buf,
			     (unsigned short *) jp2_data, tile_x0 / scale,
			     tile_y0 / scale, tile_x1 / scale,
			     tile_y1 / scale, num_bands, width / scale,
			     height / scale);
	  else
	      save_tile_u8 ((unsigned char *) buf, (unsigned char *) jp2_data,
			    tile_x0 / scale, tile_y0 / scale, tile_x1 / scale,
			    tile_y1 / scale, num_bands, width / scale,
			    height / scale);
	  free (jp2_data);
	  jp2_data = NULL;
      }

    opj_destroy_codec (codec);
    opj_stream_destroy (stream);
    opj_image_destroy (image);

    *xwidth = width / scale;
    *xheight = height / scale;
    *pixels = buf;
    *pixels_size = buf_size;
    return RL2_OK;

  error:
    if (buf != NULL)
	free (buf);
    if (jp2_data != NULL)
	free (jp2_data);
    opj_destroy_codec (codec);
    opj_stream_destroy (stream);
    opj_image_destroy (image);
    return RL2_ERROR;
}

static int
eval_jpeg2000_origin_compatibility (rl2PrivCoveragePtr coverage,
				    rl2PrivRasterPtr raster,
				    unsigned char forced_conversion,
				    int verbose)
{
/* checking for strict compatibility */
    if (coverage->sampleType == RL2_SAMPLE_UINT8
	&& coverage->pixelType == RL2_PIXEL_GRAYSCALE && coverage->nBands == 1)
      {
	  if (raster->sampleType == RL2_SAMPLE_UINT8
	      && raster->pixelType == RL2_PIXEL_GRAYSCALE
	      && raster->nBands == 1 && forced_conversion == RL2_CONVERT_NO)
	      return 1;
	  if (raster->sampleType == RL2_SAMPLE_UINT8
	      && raster->pixelType == RL2_PIXEL_RGB && raster->nBands == 3
	      && forced_conversion == RL2_CONVERT_RGB_TO_GRAYSCALE)
	      return 1;
      }
    if (coverage->sampleType == RL2_SAMPLE_UINT8
	&& coverage->pixelType == RL2_PIXEL_RGB && coverage->nBands == 3)
      {
	  if (raster->sampleType == RL2_SAMPLE_UINT8
	      && raster->pixelType == RL2_PIXEL_RGB && raster->nBands == 3
	      && forced_conversion == RL2_CONVERT_NO)
	      return 1;
	  if (raster->sampleType == RL2_SAMPLE_UINT8
	      && raster->pixelType == RL2_PIXEL_GRAYSCALE
	      && raster->nBands == 1
	      && forced_conversion == RL2_CONVERT_GRAYSCALE_TO_RGB)
	      return 1;
      }
    if (coverage->sampleType == RL2_SAMPLE_UINT8
	&& coverage->pixelType == RL2_PIXEL_DATAGRID && coverage->nBands == 1)
      {
	  if (raster->sampleType == RL2_SAMPLE_UINT8
	      && raster->pixelType == RL2_PIXEL_DATAGRID
	      && raster->nBands == 1 && forced_conversion == RL2_CONVERT_NO)
	      return 1;
      }
    if (coverage->sampleType == RL2_SAMPLE_UINT8
	&& coverage->pixelType == RL2_PIXEL_MULTIBAND
	&& (coverage->nBands == 3 || coverage->nBands == 4))
      {
	  if (raster->sampleType == RL2_SAMPLE_UINT8
	      && raster->pixelType == RL2_PIXEL_MULTIBAND
	      && raster->nBands == coverage->nBands
	      && forced_conversion == RL2_CONVERT_NO)
	      return 1;
      }
    if (coverage->sampleType == RL2_SAMPLE_UINT16
	&& coverage->pixelType == RL2_PIXEL_DATAGRID && coverage->nBands == 1)
      {
	  if (raster->sampleType == RL2_SAMPLE_UINT16
	      && raster->pixelType == RL2_PIXEL_DATAGRID
	      && raster->nBands == 1 && forced_conversion == RL2_CONVERT_NO)
	      return 1;
      }
    if (coverage->sampleType == RL2_SAMPLE_UINT16
	&& coverage->pixelType == RL2_PIXEL_RGB && coverage->nBands == 3)
      {
	  if (raster->sampleType == RL2_SAMPLE_UINT16
	      && raster->pixelType == RL2_PIXEL_RGB && raster->nBands == 3
	      && forced_conversion == RL2_CONVERT_NO)
	      return 1;
      }
    if (coverage->sampleType == RL2_SAMPLE_UINT16
	&& coverage->pixelType == RL2_PIXEL_MULTIBAND
	&& (coverage->nBands == 3 || coverage->nBands == 4))
      {
	  if (raster->sampleType == RL2_SAMPLE_UINT16
	      && raster->pixelType == RL2_PIXEL_MULTIBAND
	      && raster->nBands == coverage->nBands
	      && forced_conversion == RL2_CONVERT_NO)
	      return 1;
      }
    if (verbose)
	fprintf (stderr, "Mismatching Jpeg2000 colorspace !!!\n");
    return 0;
}

static int
read_jpeg2000_pixels_gray_to_rgb (rl2PrivRasterPtr origin,
				  unsigned short width, unsigned short height,
				  unsigned int startRow,
				  unsigned int startCol, unsigned char *pixels)
{
/* Grayscale -> RGB */
    unsigned short x;
    unsigned short y;
    unsigned short row;
    unsigned short col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  unsigned char *p_out = pixels + (y * width * 3);
	  unsigned char *p_in_base =
	      origin->rasterBuffer + (row * origin->width);
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned char *p_in = p_in_base + col;
		unsigned char value = *p_in;
		*p_out++ = value;	/* red */
		*p_out++ = value;	/* green */
		*p_out++ = value;	/* blue */
	    }
      }
    return 1;
}

static int
read_jpeg2000_pixels_rgb_to_gray (rl2PrivRasterPtr origin,
				  unsigned short width, unsigned short height,
				  unsigned int startRow,
				  unsigned int startCol, unsigned char *pixels)
{
/* RGB -> Grayscale */
    unsigned short x;
    unsigned short y;
    unsigned short row;
    unsigned short col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  unsigned char *p_out = pixels + (y * width);
	  unsigned char *p_in_base =
	      origin->rasterBuffer + (row * origin->width * 3);
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned char *p_in = p_in_base + (col * 3);
		unsigned char r = *p_in++;
		unsigned char g = *p_in++;
		unsigned char b = *p_in++;
		double gray =
		    ((double) r * 0.2126) + ((double) g * 0.7152) +
		    ((double) b * 0.0722);
		*p_out++ = (unsigned char) gray;
	    }
      }
    return 1;
}

static int
read_jpeg2000_pixels_u8 (rl2PrivRasterPtr origin, unsigned short width,
			 unsigned short height, unsigned int startRow,
			 unsigned int startCol, unsigned char *pixels,
			 int num_bands)
{
/* no coversion - UINT8 */
    unsigned short x;
    unsigned short y;
    int ib;
    unsigned short row;
    unsigned short col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  unsigned char *p_out = pixels + (y * width * num_bands);
	  unsigned char *p_in_base =
	      origin->rasterBuffer + (row * origin->width * num_bands);
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned char *p_in = p_in_base + (col * num_bands);
		for (ib = 0; ib < num_bands; ib++)
		    *p_out++ = *p_in++;
	    }
      }
    return 1;
}

static int
read_jpeg2000_pixels_u16 (rl2PrivRasterPtr origin, unsigned short width,
			  unsigned short height, unsigned int startRow,
			  unsigned int startCol, unsigned short *pixels,
			  int num_bands)
{
/* no coversion - UINT16 */
    unsigned short x;
    unsigned short y;
    int ib;
    unsigned short row;
    unsigned short col;
    for (y = 0, row = startRow; y < height && row < origin->height; y++, row++)
      {
	  /* looping on rows */
	  unsigned short *p_out = pixels + (y * width * num_bands);
	  unsigned short *p_in_base =
	      (unsigned short *) (origin->rasterBuffer) +
	      (row * origin->width * num_bands);
	  for (x = 0, col = startCol; x < width && col < origin->width;
	       x++, col++)
	    {
		unsigned short *p_in = p_in_base + (col * num_bands);
		for (ib = 0; ib < num_bands; ib++)
		    *p_out++ = *p_in++;
	    }
      }
    return 1;
}

static int
read_from_jpeg2000 (rl2PrivRasterPtr origin, unsigned short width,
		    unsigned short height, unsigned char sample_type,
		    unsigned char pixel_type, unsigned char num_bands,
		    unsigned char forced_conversion, unsigned int startRow,
		    unsigned int startCol, unsigned char **pixels,
		    int *pixels_sz)
{
/* creating a tile from the Jpeg2000 origin */
    int nb;
    unsigned char *bufPixels = NULL;
    int bufPixelsSz = 0;
    rl2PixelPtr no_data = NULL;

    no_data = rl2_create_pixel (sample_type, pixel_type, num_bands);
    if (sample_type == RL2_SAMPLE_UINT8
	&& (pixel_type == RL2_PIXEL_RGB || pixel_type == RL2_PIXEL_GRAYSCALE))
      {
	  for (nb = 0; nb < num_bands; nb++)
	      rl2_set_pixel_sample_uint8 (no_data, nb, 255);
      }
    else
      {
	  if (sample_type == RL2_SAMPLE_UINT8)
	    {
		for (nb = 0; nb < num_bands; nb++)
		    rl2_set_pixel_sample_uint8 (no_data, nb, 0);
	    }
	  if (sample_type == RL2_SAMPLE_UINT16)
	    {
		for (nb = 0; nb < num_bands; nb++)
		    rl2_set_pixel_sample_uint16 (no_data, nb, 0);
	    }
      }

/* allocating the pixels buffer */
    bufPixelsSz = width * height * num_bands;
    bufPixels = malloc (bufPixelsSz);
    if (bufPixels == NULL)
	goto error;
    if ((startRow + height) > origin->height
	|| (startCol + width) > origin->width)
	rl2_prime_void_tile (bufPixels, width, height, sample_type, num_bands,
			     no_data);


    if (pixel_type == RL2_PIXEL_GRAYSCALE && sample_type == RL2_SAMPLE_UINT8
	&& num_bands == 1 && forced_conversion == RL2_CONVERT_RGB_TO_GRAYSCALE)
      {
	  if (!read_jpeg2000_pixels_rgb_to_gray
	      (origin, width, height, startRow, startCol, bufPixels))
	      goto error;
      }
    else if (pixel_type == RL2_PIXEL_RGB && sample_type == RL2_SAMPLE_UINT8
	     && num_bands == 3
	     && forced_conversion == RL2_CONVERT_GRAYSCALE_TO_RGB)
      {
	  if (!read_jpeg2000_pixels_gray_to_rgb
	      (origin, width, height, startRow, startCol, bufPixels))
	      goto error;
      }
    else if (forced_conversion == RL2_CONVERT_NO)
      {
	  if (sample_type == RL2_SAMPLE_UINT8)
	    {
		if (!read_jpeg2000_pixels_u8
		    (origin, width, height, startRow, startCol, bufPixels,
		     num_bands))
		    goto error;
	    }
	  else if (sample_type == RL2_SAMPLE_UINT16)
	    {
		if (!read_jpeg2000_pixels_u16
		    (origin, width, height, startRow, startCol,
		     (unsigned short *) bufPixels, num_bands))
		    goto error;
	    }
	  else
	      goto error;
      }
    else
	goto error;
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);

    *pixels = bufPixels;
    *pixels_sz = bufPixelsSz;
    return RL2_OK;
  error:
    if (bufPixels != NULL)
	free (bufPixels);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    return RL2_ERROR;
}

RL2_DECLARE rl2RasterPtr
rl2_get_tile_from_jpeg2000_origin (rl2CoveragePtr cvg, rl2RasterPtr jpeg2000,
				   unsigned int startRow,
				   unsigned int startCol,
				   unsigned char forced_conversion, int verbose)
{
/* attempting to create a Coverage-tile from a Jpeg2000 origin */
    unsigned int x;
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    rl2PrivRasterPtr origin = (rl2PrivRasterPtr) jpeg2000;
    rl2RasterPtr raster = NULL;
    unsigned char *pixels = NULL;
    int pixels_sz = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    unsigned int unused_width = 0;
    unsigned int unused_height = 0;

    if (coverage == NULL || origin == NULL)
	return NULL;
    if (!eval_jpeg2000_origin_compatibility
	(coverage, origin, forced_conversion, verbose))
	return NULL;

/* testing for tile's boundary validity */
    if (startCol > origin->width)
	return NULL;
    if (startRow > origin->height)
	return NULL;
    x = startCol / coverage->tileWidth;
    if ((x * coverage->tileWidth) != startCol)
	return NULL;
    x = startRow / coverage->tileHeight;
    if ((x * coverage->tileHeight) != startRow)
	return NULL;

/* attempting to create the tile */
    if (read_from_jpeg2000
	(origin, coverage->tileWidth, coverage->tileHeight,
	 coverage->sampleType, coverage->pixelType, coverage->nBands,
	 forced_conversion, startRow, startCol, &pixels, &pixels_sz) != RL2_OK)
	goto error;
    if (startCol + coverage->tileWidth > origin->width)
	unused_width = (startCol + coverage->tileWidth) - origin->width;
    if (startRow + coverage->tileHeight > origin->height)
	unused_height = (startRow + coverage->tileHeight) - origin->height;
    if (unused_width || unused_height)
      {
	  /* 
	   * creating a Transparency Mask so to shadow any 
	   * unused portion of the current tile 
	   */
	  unsigned int shadow_x = coverage->tileWidth - unused_width;
	  unsigned int shadow_y = coverage->tileHeight - unused_height;
	  unsigned int row;
	  mask_size = coverage->tileWidth * coverage->tileHeight;
	  mask = malloc (mask_size);
	  if (mask == NULL)
	      goto error;
	  /* full Transparent mask */
	  memset (mask, 0, coverage->tileWidth * coverage->tileHeight);
	  for (row = 0; row < coverage->tileHeight; row++)
	    {
		unsigned char *p = mask + (row * coverage->tileWidth);
		if (row < shadow_y)
		  {
		      /* setting opaque pixels */
		      memset (p, 1, shadow_x);
		  }
	    }
      }
    raster =
	rl2_create_raster (coverage->tileWidth, coverage->tileHeight,
			   coverage->sampleType, coverage->pixelType,
			   coverage->nBands, pixels, pixels_sz, NULL, mask,
			   mask_size, NULL);
    if (raster == NULL)
	goto error;
    return raster;
  error:
    if (pixels != NULL)
	free (pixels);
    if (mask != NULL)
	free (mask);
    return NULL;
}

RL2_DECLARE int
rl2_get_jpeg2000_infos (const char *path, unsigned int *xwidth,
			unsigned int *xheight, unsigned char *xsample_type,
			unsigned char *xpixel_type, unsigned char *xnum_bands,
			unsigned int *xtile_width, unsigned int *xtile_height,
			unsigned char *num_levels)
{
/* attempting to retrieve the basic infos about some Jpeg2000 */
    unsigned char *jpeg2000 = NULL;
    int jpeg2000_sz;
    unsigned int width;
    unsigned int height;
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    struct jp2_memfile clientdata;
    opj_codec_t *codec;
    opj_dparameters_t parameters;
    opj_stream_t *stream;
    opj_image_t *image = NULL;
    opj_codestream_info_v2_t *code_stream_info;
    OPJ_UINT32 nComponents;
    OPJ_INT32 tile_width;
    OPJ_INT32 tile_height;
    int nResolutions;

/* loading in memory the Jpeg2000 raster */
    if (rl2_blob_from_file (path, &jpeg2000, &jpeg2000_sz) != RL2_OK)
	return RL2_ERROR;

/* creating and initializing the Jpeg2000 decoder */
    codec = opj_create_decompress (OPJ_CODEC_JP2);
    opj_set_info_handler (codec, info_callback, NULL);
    opj_set_warning_handler (codec, warning_callback, NULL);
    opj_set_error_handler (codec, error_callback, NULL);
    opj_set_default_decoder_parameters (&parameters);
    if (!opj_setup_decoder (codec, &parameters))
	return RL2_ERROR;

/* preparing the input stream */
    stream = opj_stream_create (1024, 1);
    opj_stream_set_user_data_length (stream, jpeg2000_sz);
    opj_stream_set_read_function (stream, read_callback);
    opj_stream_set_seek_function (stream, seek_callback);
    opj_stream_set_skip_function (stream, skip_callback);
/* initializing the memory Read struct */
    clientdata.buffer = (unsigned char *) jpeg2000;
    clientdata.malloc_block = 1024;
    clientdata.size = jpeg2000_sz;
    clientdata.eof = jpeg2000_sz;
    clientdata.current = 0;
#ifdef OPENJPEG_2_1
    opj_stream_set_user_data (stream, &clientdata, NULL);
#else
    opj_stream_set_user_data (stream, &clientdata);
#endif
    if (!opj_read_header (stream, codec, &image))
      {
	  fprintf (stderr, "OpenJpeg Error: opj_read_header() failed\n");
	  goto error;
      }
    code_stream_info = opj_get_cstr_info (codec);
    tile_width = code_stream_info->tdx;
    tile_height = code_stream_info->tdy;
    nComponents = code_stream_info->nbcomps;
    nResolutions =
	code_stream_info->m_default_tile_info.tccp_info[0].numresolutions;
    opj_destroy_cstr_info (&code_stream_info);
    if (image == NULL)
	goto error;
    if (nResolutions < 4)
	goto error;
    if (image->comps[0].prec == 16 && image->comps[0].sgnd == 0)
	sample_type = RL2_SAMPLE_UINT16;
    if (image->comps[0].prec == 8 && image->comps[0].sgnd == 0)
	sample_type = RL2_SAMPLE_UINT8;
    if (nComponents == 1)
      {
	  if (sample_type == RL2_SAMPLE_UINT16)
	      pixel_type = RL2_PIXEL_DATAGRID;
	  if (sample_type == RL2_SAMPLE_UINT8)
	      pixel_type = RL2_PIXEL_GRAYSCALE;
      }
    if (nComponents == 3)
	pixel_type = RL2_PIXEL_RGB;
    if (nComponents == 4)
	pixel_type = RL2_PIXEL_MULTIBAND;
    width = image->comps[0].w;
    height = image->comps[0].h;

    opj_destroy_codec (codec);
    opj_stream_destroy (stream);
    opj_image_destroy (image);
    free (jpeg2000);

    *xwidth = width;
    *xheight = height;
    *xsample_type = sample_type;
    *xpixel_type = pixel_type;
    *xnum_bands = nComponents;
    *xtile_width = tile_width;
    *xtile_height = tile_height;
    *num_levels = nResolutions;
    return RL2_OK;

  error:
    opj_destroy_codec (codec);
    opj_stream_destroy (stream);
    opj_image_destroy (image);
    if (jpeg2000 != NULL)
	free (jpeg2000);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_jpeg2000_blob_type (const unsigned char *jpeg2000, int jpeg2000_sz,
			    unsigned char *xsample_type,
			    unsigned char *xpixel_type,
			    unsigned char *xnum_bands)
{
/* attempting to retrieve the basic infos about some Jpeg2000 - BLOB version */
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    struct jp2_memfile clientdata;
    opj_codec_t *codec;
    opj_dparameters_t parameters;
    opj_stream_t *stream;
    opj_image_t *image = NULL;
    opj_codestream_info_v2_t *code_stream_info;
    OPJ_UINT32 nComponents;

/* creating and initializing the Jpeg2000 decoder */
    codec = opj_create_decompress (OPJ_CODEC_JP2);
    opj_set_info_handler (codec, info_callback, NULL);
    opj_set_warning_handler (codec, warning_callback, NULL);
    opj_set_error_handler (codec, error_callback, NULL);
    opj_set_default_decoder_parameters (&parameters);
    if (!opj_setup_decoder (codec, &parameters))
	return RL2_ERROR;

/* preparing the input stream */
    stream = opj_stream_create (1024, 1);
    opj_stream_set_user_data_length (stream, jpeg2000_sz);
    opj_stream_set_read_function (stream, read_callback);
    opj_stream_set_seek_function (stream, seek_callback);
    opj_stream_set_skip_function (stream, skip_callback);
/* initializing the memory Read struct */
    clientdata.buffer = (unsigned char *) jpeg2000;
    clientdata.malloc_block = 1024;
    clientdata.size = jpeg2000_sz;
    clientdata.eof = jpeg2000_sz;
    clientdata.current = 0;
#ifdef OPENJPEG_2_1
    opj_stream_set_user_data (stream, &clientdata, NULL);
#else
    opj_stream_set_user_data (stream, &clientdata);
#endif
    if (!opj_read_header (stream, codec, &image))
      {
	  fprintf (stderr, "OpenJpeg Error: opj_read_header() failed\n");
	  goto error;
      }
    code_stream_info = opj_get_cstr_info (codec);
    nComponents = code_stream_info->nbcomps;
    opj_destroy_cstr_info (&code_stream_info);
    if (image == NULL)
	goto error;
    if (image->comps[0].prec == 16 && image->comps[0].sgnd == 0)
	sample_type = RL2_SAMPLE_UINT16;
    if (image->comps[0].prec == 8 && image->comps[0].sgnd == 0)
	sample_type = RL2_SAMPLE_UINT8;
    if (nComponents == 1)
      {
	  if (sample_type == RL2_SAMPLE_UINT16)
	      pixel_type = RL2_PIXEL_DATAGRID;
	  if (sample_type == RL2_SAMPLE_UINT8)
	      pixel_type = RL2_PIXEL_GRAYSCALE;
      }
    if (nComponents == 3)
	pixel_type = RL2_PIXEL_RGB;
    if (nComponents == 4)
	pixel_type = RL2_PIXEL_MULTIBAND;

    opj_destroy_codec (codec);
    opj_stream_destroy (stream);
    opj_image_destroy (image);

    *xsample_type = sample_type;
    *xpixel_type = pixel_type;
    *xnum_bands = nComponents;
    return RL2_OK;

  error:
    opj_destroy_codec (codec);
    opj_stream_destroy (stream);
    opj_image_destroy (image);
    return RL2_ERROR;
}

RL2_DECLARE char *
rl2_build_jpeg2000_xml_summary (unsigned int width, unsigned int height,
				unsigned char sample_type,
				unsigned char pixel_type,
				unsigned char num_bands, int is_georeferenced,
				double res_x, double res_y, double minx,
				double miny, double maxx, double maxy,
				unsigned int tile_width,
				unsigned int tile_height)
{
/* attempting to build an XML Summary from a Jpeg2000 */
    char *xml;
    char *prev;
    int len;
    int bps = 8;

    xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<ImportedRaster>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<RasterFormat>Jpeg2000</RasterFormat>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<RasterWidth>%u</RasterWidth>", prev, width);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<RasterHeight>%u</RasterHeight>", prev, height);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<TileWidth>%u</TileWidth>", prev, tile_width);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<TileHeight>%u</TileHeight>", prev, tile_height);
    sqlite3_free (prev);
    prev = xml;
    if (sample_type == RL2_SAMPLE_UINT16)
	bps = 16;
    xml = sqlite3_mprintf ("%s<BitsPerSample>%d</BitsPerSample>", prev, bps);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<SamplesPerPixel>%d</SamplesPerPixel>", prev,
			 num_bands);
    sqlite3_free (prev);
    prev = xml;
    if (pixel_type == RL2_PIXEL_RGB)
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>RGB</PhotometricInterpretation>",
	     prev);
    else
	xml =
	    sqlite3_mprintf
	    ("%s<PhotometricInterpretation>min-is-black</PhotometricInterpretation>",
	     prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Compression>Jpeg2000</Compression>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<SampleFormat>unsigned integer</SampleFormat>",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<PlanarConfiguration>single Raster plane</PlanarConfiguration>",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<NoDataPixel>unknown</NoDataPixel>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (is_georeferenced)
      {
	  xml = sqlite3_mprintf ("%s<GeoReferencing>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<SpatialReferenceSystem>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<SRID>unspecified</SRID>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<RefSysName>undeclared</RefSysName>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</SpatialReferenceSystem>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<SpatialResolution>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<HorizontalResolution>%1.10f</HorizontalResolution>", prev,
	       res_x);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<VerticalResolution>%1.10f</VerticalResolution>", prev,
	       res_y);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</SpatialResolution>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<BoundingBox>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<MinX>%1.10f</MinX>", prev, minx);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<MinY>%1.10f</MinY>", prev, miny);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<MaxX>%1.10f</MaxX>", prev, maxx);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<MaxY>%1.10f</MaxY>", prev, maxy);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</BoundingBox>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Extent>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<HorizontalExtent>%1.10f</HorizontalExtent>", prev,
	       maxx - minx);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<VerticalExtent>%1.10f</VerticalExtent>",
			       prev, maxy - miny);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Extent>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</GeoReferencing>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</ImportedRaster>", prev);
    sqlite3_free (prev);
    len = strlen (xml);
    prev = xml;
    xml = malloc (len + 1);
    strcpy (xml, prev);
    sqlite3_free (prev);
    return xml;
}

#endif /* end OpenJpeg conditional */
