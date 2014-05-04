/*

 rl2webp -- WebP related functions

 version 0.1, 2013 April 5

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

#include <webp/decode.h>
#include <webp/encode.h>

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

static int
check_webp_compatibility (unsigned char sample_type, unsigned char pixel_type,
			  unsigned char num_samples)
{
/* checks for WebP compatibility */
    switch (sample_type)
      {
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  break;
      default:
	  return RL2_ERROR;
      };
    switch (pixel_type)
      {
      case RL2_PIXEL_MONOCHROME:
      case RL2_PIXEL_PALETTE:
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_RGB:
	  break;
      default:
	  return RL2_ERROR;
      };
    if (pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
	    case RL2_SAMPLE_2_BIT:
	    case RL2_SAMPLE_4_BIT:
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 1)
	      return RL2_ERROR;
      }
    if (pixel_type == RL2_PIXEL_RGB)
      {
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_UINT8:
		break;
	    default:
		return RL2_ERROR;
	    };
	  if (num_samples != 3)
	      return RL2_ERROR;
      }
    return RL2_OK;
}

static int
compress_lossy_alpha_webp (rl2PrivRasterPtr rst, unsigned char **webp,
			   int *webp_size, int quality)
{
/* compressing a WebP (lossy) image (with ALPHA channel) */
    int size;
    unsigned char *output;
    int rgba_size;
    unsigned char *rgba;

    *webp = NULL;
    *webp_size = 0;
    if (rl2_raster_data_to_RGBA ((rl2RasterPtr) rst, &rgba, &rgba_size) ==
	RL2_ERROR)
	return RL2_ERROR;
    if (quality > 100)
	quality = 100;
    if (quality < 0)
	quality = 75;
    size =
	WebPEncodeRGBA (rgba, rst->width, rst->height, rst->width * 4, quality,
			&output);
    free (rgba);
    if (size == 0)
	return RL2_ERROR;
    *webp = output;
    *webp_size = size;
    return RL2_OK;
}

static int
compress_lossy_webp (rl2RasterPtr ptr, unsigned char **webp, int *webp_size,
		     int quality)
{
/* compressing a WebP (lossy) image */
    int size;
    unsigned char *output;
    int rgb_size;
    unsigned char *rgb;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst->maskBuffer != NULL || rst->noData != NULL)
	return compress_lossy_alpha_webp (rst, webp, webp_size, quality);

/* no ALPHA channel */
    *webp = NULL;
    *webp_size = 0;
    if (rl2_raster_data_to_RGB (ptr, &rgb, &rgb_size) == RL2_ERROR)
	return RL2_ERROR;
    if (quality > 100)
	quality = 100;
    if (quality < 0)
	quality = 75;
    size =
	WebPEncodeRGB (rgb, rst->width, rst->height, rst->width * 3, quality,
		       &output);
    free (rgb);
    if (size == 0)
	return RL2_ERROR;
    *webp = output;
    *webp_size = size;
    return RL2_OK;
}

static int
compress_lossless_alpha_webp (rl2PrivRasterPtr rst, unsigned char **webp,
			      int *webp_size)
{
/* compressing a WebP (lossless) image (with ALPHA channel) */
    int size;
    unsigned char *output;
    int rgba_size;
    unsigned char *rgba;

    *webp = NULL;
    *webp_size = 0;
    if (rl2_raster_data_to_RGBA ((rl2RasterPtr) rst, &rgba, &rgba_size) ==
	RL2_ERROR)
	return RL2_ERROR;
    size =
	WebPEncodeLosslessRGBA (rgba, rst->width, rst->height, rst->width * 4,
				&output);
    free (rgba);
    if (size == 0)
	return RL2_ERROR;
    *webp = output;
    *webp_size = size;
    return RL2_OK;
}

static int
compress_lossless_webp (rl2RasterPtr ptr, unsigned char **webp, int *webp_size)
{
/* compressing a WebP (lossless) image */
    int size;
    unsigned char *output;
    int rgb_size;
    unsigned char *rgb;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) ptr;
    if (rst->maskBuffer != NULL || rst->noData != NULL)
	return compress_lossless_alpha_webp (rst, webp, webp_size);

/* no ALPHA channel */
    *webp = NULL;
    *webp_size = 0;
    if (rl2_raster_data_to_RGB (ptr, &rgb, &rgb_size) == RL2_ERROR)
	return RL2_ERROR;
    size =
	WebPEncodeLosslessRGB (rgb, rst->width, rst->height, rst->width * 3,
			       &output);
    free (rgb);
    if (size == 0)
	return RL2_ERROR;
    *webp = output;
    *webp_size = size;
    return RL2_OK;
}

RL2_DECLARE int
rl2_section_to_lossy_webp (rl2SectionPtr scn, const char *path, int quality)
{
/* attempting to save a raster section into a WebP (lossy) file */
    int blob_size;
    unsigned char *blob;
    rl2RasterPtr rst;
    int ret;

    if (scn == NULL)
	return RL2_ERROR;
    rst = rl2_get_section_raster (scn);
    if (rst == NULL)
	return RL2_ERROR;
/* attempting to export as a WebP image */
    if (rl2_raster_to_lossy_webp (rst, &blob, &blob_size, quality) != RL2_OK)
	return RL2_ERROR;
    ret = rl2_blob_to_file (path, blob, blob_size);
    free (blob);
    if (ret != RL2_OK)
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_to_lossy_webp (rl2RasterPtr rst, unsigned char **webp,
			  int *webp_size, int quality)
{
/* creating a WebP image (lossy) from a raster */
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
    if (check_webp_compatibility (sample_type, pixel_type, num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (compress_lossy_webp (rst, &blob, &blob_size, quality) != RL2_OK)
	return RL2_ERROR;
    *webp = blob;
    *webp_size = blob_size;
    return RL2_OK;
}


RL2_DECLARE int
rl2_section_to_lossless_webp (rl2SectionPtr scn, const char *path)
{
/* attempting to save a raster section into a WebP (lossless) file */
    int blob_size;
    unsigned char *blob;
    rl2RasterPtr rst;
    int ret;

    if (scn == NULL)
	return RL2_ERROR;
    rst = rl2_get_section_raster (scn);
    if (rst == NULL)
	return RL2_ERROR;
/* attempting to export as a WebP image */
    if (rl2_raster_to_lossless_webp (rst, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    ret = rl2_blob_to_file (path, blob, blob_size);
    free (blob);
    if (ret != RL2_OK)
	return RL2_ERROR;
    return RL2_OK;
}

RL2_DECLARE int
rl2_raster_to_lossless_webp (rl2RasterPtr rst, unsigned char **webp,
			     int *webp_size)
{
/* creating a WebP image (lossless) from a raster */
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
    if (check_webp_compatibility (sample_type, pixel_type, num_samples) !=
	RL2_OK)
	return RL2_ERROR;
    if (compress_lossless_webp (rst, &blob, &blob_size) != RL2_OK)
	return RL2_ERROR;
    *webp = blob;
    *webp_size = blob_size;
    return RL2_OK;
}

RL2_DECLARE rl2SectionPtr
rl2_section_from_webp (const char *path)
{
/* attempting to create a raster section from a WebP file */
    int blob_size;
    unsigned char *blob;
    rl2SectionPtr scn;
    rl2RasterPtr rst;

/* attempting to create a raster */
    if (rl2_blob_from_file (path, &blob, &blob_size) != RL2_OK)
	return NULL;
    rst = rl2_raster_from_webp (blob, blob_size);
    free (blob);
    if (rst == NULL)
	return NULL;

/* creating the raster section */
    scn =
	rl2_create_section (path, RL2_COMPRESSION_LOSSY_WEBP,
			    RL2_TILESIZE_UNDEFINED, RL2_TILESIZE_UNDEFINED,
			    rst);
    return scn;
}

RL2_DECLARE rl2RasterPtr
rl2_raster_from_webp (unsigned char *webp, int webp_size)
{
/* attempting to create a raster from a WebP image */
    rl2RasterPtr rst = NULL;
    unsigned char *buf;
    int buf_size;
    unsigned char *mask;
    int mask_size;
    unsigned short width;
    unsigned short height;

    if (rl2_decode_webp_scaled
	(1, webp, webp_size, &width, &height, RL2_PIXEL_RGB, &buf, &buf_size,
	 &mask, &mask_size) != RL2_OK)
	return NULL;

/* creating the raster */
    rst =
	rl2_create_raster (width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   buf, buf_size, NULL, mask, mask_size, NULL);
    if (rst == NULL)
      {
	  free (buf);
	  if (mask != NULL)
	      free (mask);
      }
    return rst;
}

RL2_PRIVATE int
rl2_decode_webp_scaled (int scale, const unsigned char *webp, int webp_size,
			unsigned short *xwidth, unsigned short *xheight,
			unsigned char pixel_type, unsigned char **pixels,
			int *pixels_size, unsigned char **xmask,
			int *xmask_size)
{
/* attempting to create a raster from a WebP image - supporting rescaled size */
    unsigned char *buf = NULL;
    int buf_size = 0;
    unsigned char *mask = NULL;
    int mask_size = 0;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *p_mask;
    int width;
    int height;
    int row;
    int col;
    WebPBitstreamFeatures features;
    WebPDecoderConfig config;

    if (scale == 1 || scale == 2 || scale == 4 || scale == 8)
	;
    else
	return RL2_ERROR;

    if (WebPGetFeatures (webp, webp_size, &features) != VP8_STATUS_OK)
	return RL2_ERROR;

/* decoder options */
    WebPInitDecoderConfig (&config);
    if (scale == 1)
      {
	  config.options.no_fancy_upsampling = 1;
	  config.options.use_scaling = 0;
	  width = features.width;
	  height = features.height;
      }
    else
      {
	  config.options.no_fancy_upsampling = 1;
	  config.options.use_scaling = 1;
	  width = features.width / scale;
	  height = features.height / scale;
	  config.options.scaled_width = width;
	  config.options.scaled_height = height;
      }
    if (features.has_alpha)
      {
	  config.output.colorspace = MODE_RGBA;
	  buf_size = width * height * 4;
	  buf = malloc (buf_size);
	  if (buf == NULL)
	      goto error;
	  config.output.u.RGBA.rgba = (uint8_t *) buf;
	  config.output.u.RGBA.stride = width * 4;
	  config.output.u.RGBA.size = buf_size;
	  config.output.is_external_memory = 1;
      }
    else
      {
	  config.output.colorspace = MODE_RGB;
	  buf_size = width * height * 3;
	  buf = malloc (buf_size);
	  if (buf == NULL)
	      goto error;
	  config.output.u.RGBA.rgba = (uint8_t *) buf;
	  config.output.u.RGBA.stride = width * 3;
	  config.output.u.RGBA.size = buf_size;
	  config.output.is_external_memory = 1;
      }

    if (features.has_alpha)
      {
	  if (WebPDecode (webp, webp_size, &config) != VP8_STATUS_OK)
	      goto error;
	  buf_size = width * height * 3;
	  mask_size = width * height;
	  mask = malloc (mask_size);
	  if (mask == NULL)
	      goto error;
	  p_mask = mask;
	  p_in = buf;
	  p_out = buf;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      if (*p_in++ < 128)
			  *p_mask++ = 0;
		      else
			  *p_mask++ = 1;
		  }
	    }
      }
    else
      {
	  if (WebPDecode (webp, webp_size, &config) != VP8_STATUS_OK)
	      goto error;
	  mask = NULL;
	  mask_size = 0;
      }

    if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  /* returning a GRAYSCALE pixel buffer */
	  unsigned char *gray = NULL;
	  int gray_size = width * height;
	  gray = malloc (gray_size);
	  if (gray == NULL)
	      goto error;
	  p_in = buf;
	  p_out = gray;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      *p_out++ = *p_in;
		      p_in += 3;
		  }
	    }
	  free (buf);
	  buf = gray;
	  buf_size = gray_size;
      }

    *xwidth = width;
    *xheight = height;
    *pixels = buf;
    *pixels_size = buf_size;
    *xmask = mask;
    *xmask_size = mask_size;
    return RL2_OK;

  error:
    if (buf != NULL)
	free (buf);
    if (mask != NULL)
	free (mask);
    return RL2_ERROR;
}
