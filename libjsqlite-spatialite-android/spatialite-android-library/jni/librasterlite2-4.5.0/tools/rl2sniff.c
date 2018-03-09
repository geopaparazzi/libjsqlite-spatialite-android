/* 
/ rl2sniff
/
/ a listing/sniffing tool supporting RasterLite2
/
/ version 1.0, 2014 August 12
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ Copyright (C) 2014  Alessandro Furieri
/
/    This program is free software: you can redistribute it and/or modify
/    it under the terms of the GNU General Public License as published by
/    the Free Software Foundation, either version 3 of the License, or
/    (at your option) any later version.
/
/    This program is distributed in the hope that it will be useful,
/    but WITHOUT ANY WARRANTY; without even the implied warranty of
/    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/    GNU General Public License for more details.
/
/    You should have received a copy of the GNU General Public License
/    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <time.h>

#include <sys/types.h>
#if defined(_WIN32) && !defined(__MINGW32__)
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include "config.h"

/*
/ the following patch supporting GeoTiff headers
/ was kindly contributed by Brad Hards on 2011-09-02
/ for libgaigraphics (the ancestor of librasterlite2)
*/
#ifdef HAVE_GEOTIFF_GEOTIFF_H
#include <geotiff/geotiff.h>
#include <geotiff/xtiffio.h>
#include <geotiff/geo_tiffp.h>
#include <geotiff/geo_keyp.h>
#include <geotiff/geovalues.h>
#include <geotiff/geo_normalize.h>
#elif HAVE_LIBGEOTIFF_GEOTIFF_H
#include <libgeotiff/geotiff.h>
#include <libgeotiff/xtiffio.h>
#include <libgeotiff/geo_tiffp.h>
#include <libgeotiff/geo_keyp.h>
#include <libgeotiff/geovalues.h>
#include <libgeotiff/geo_normalize.h>
#else
#include <geotiff.h>
#include <xtiffio.h>
#include <geo_tiffp.h>
#include <geo_keyp.h>
#include <geovalues.h>
#include <geo_normalize.h>
#endif

#include <rasterlite2/rasterlite2.h>
#include <rasterlite2/rl2tiff.h>

#include <spatialite/gaiaaux.h>
#include <spatialite.h>

#define ARG_NONE	0
#define ARG_SRC_PATH	1
#define ARG_DIR_PATH	2
#define ARG_FILE_EXT	3

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

static int
parse_ncols (const char *str, unsigned int *width)
{
/* attempting to parse the NCOLS item */
    if (strncasecmp (str, "ncols ", 6) == 0)
      {
	  *width = atoi (str + 6);
	  return 1;
      }
    return 0;
}

static int
parse_nrows (const char *str, unsigned int *height)
{
/* attempting to parse the NROWS item */
    if (strncasecmp (str, "nrows ", 6) == 0)
      {
	  *height = atoi (str + 6);
	  return 1;
      }
    return 0;
}

static int
parse_xllcorner (const char *str, double *minx)
{
/* attempting to parse the XLLCORNER item */
    if (strncasecmp (str, "xllcorner ", 10) == 0)
      {
	  *minx = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_yllcorner (const char *str, double *miny)
{
/* attempting to parse the YLLCORNER item */
    if (strncasecmp (str, "yllcorner ", 10) == 0)
      {
	  *miny = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_xllcenter (const char *str, double *minx)
{
/* attempting to parse the XLLCENTER item */
    if (strncasecmp (str, "xllcenter ", 10) == 0)
      {
	  *minx = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_yllcenter (const char *str, double *miny)
{
/* attempting to parse the YLLCENTER item */
    if (strncasecmp (str, "yllcenter ", 10) == 0)
      {
	  *miny = atof (str + 10);
	  return 1;
      }
    return 0;
}

static int
parse_cellsize (const char *str, double *xres)
{
/* attempting to parse the CELLSIZE item */
    if (strncasecmp (str, "cellsize ", 9) == 0)
      {
	  *xres = atof (str + 9);
	  return 1;
      }
    return 0;
}

static int
parse_nodata (const char *str, double *no_data)
{
/* attempting to parse the NODATA_value item */
    if (strncasecmp (str, "NODATA_value ", 13) == 0)
      {
	  *no_data = atof (str + 13);
	  return 1;
      }
    return 0;
}

static int
get_ascii_header (FILE * in, unsigned int *width, unsigned int *height,
		  double *minx, double *miny, double *maxx, double *maxy,
		  double *xres, double *yres, double *no_data)
{
/* attempting to parse the ASCII Header */
    char buf[1024];
    char *p_out = buf;
    int line_no = 0;
    int c;
    int ok_ncols = 0;
    int ok_nrows = 0;
    int ok_xll = 0;
    int ok_yll = 0;
    int ok_cellsize = 0;
    int ok_nodata = 0;

    while ((c = getc (in)) != EOF)
      {
	  if (c == '\r')
	      continue;
	  if (c == '\n')
	    {
		*p_out = '\0';
		if (parse_ncols (buf, width))
		    ok_ncols++;
		if (parse_nrows (buf, height))
		    ok_nrows++;
		if (parse_xllcorner (buf, minx))
		    ok_xll++;
		if (parse_xllcenter (buf, minx))
		    ok_xll++;
		if (parse_yllcorner (buf, miny))
		    ok_yll++;
		if (parse_yllcenter (buf, miny))
		    ok_yll++;
		if (parse_cellsize (buf, xres))
		    ok_cellsize++;
		if (parse_nodata (buf, no_data))
		    ok_nodata++;
		line_no++;
		if (line_no == 6)
		    break;
		p_out = buf;
		continue;
	    }
	  if ((p_out - buf) >= 1024)
	      goto error;
	  *p_out++ = c;
      }
    if (ok_ncols == 1 && ok_nrows == 1 && ok_xll == 1 && ok_yll == 1
	&& ok_cellsize == 1 && ok_nodata == 1)
	;
    else
	goto error;

    *maxx = *minx + ((double) (*width) * *xres);
    *yres = *xres;
    *maxy = *miny + ((double) (*height) * *yres);
    return 1;

  error:
    *width = 0;
    *height = 0;
    *minx = DBL_MAX;
    *miny = DBL_MAX;
    *maxx = 0.0 - DBL_MAX;
    *maxy = 0.0 - DBL_MAX;
    *xres = 0.0;
    *yres = 0.0;
    *no_data = DBL_MAX;
    return 0;
}

static int
is_ascii_grid (const char *path)
{
/* testing for an ASCII Grid */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".asc") == 0)
	      return 1;
      }
    return 0;
}

static int
is_jpeg_image (const char *path)
{
/* testing for a JPEG image */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".jpg") == 0)
	      return 1;
      }
    return 0;
}

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */

static int
is_jpeg2000_image (const char *path)
{
/* testing for a Jpeg2000 image */
    int len = strlen (path);
    if (len > 4)
      {
	  if (strcasecmp (path + len - 4, ".jp2") == 0)
	      return 1;
      }
    return 0;
}

#endif /* end OpenJpeg conditional */

static void
do_sniff_ascii_grid (const char *src_path, int with_md5)
{
/* sniffing an ASCII Grid */
    FILE *in;
    unsigned int width = 0;
    unsigned int height = 0;
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    double x_res = 0.0;
    double y_res = 0.0;
    double no_data = 0.0;
    char *md5 = NULL;

    in = fopen (src_path, "rb");
    if (in == NULL)
	return;
    if (!get_ascii_header
	(in, &width, &height, &minX, &minY, &maxX, &maxY, &x_res, &y_res,
	 &no_data))
	goto error;

    if (with_md5)
	md5 = rl2_compute_file_md5_checksum (src_path);

    printf
	("ASCII Grid\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t%1.8f\t%s\t%d\t%1.12f\t%1.12f\t%1.8f\t%1.8f\t%1.8f\t%1.8f\n",
	 (md5 == NULL) ? "" : md5, src_path, width, height, "unkwnown",
	 "DATAGRID", 1, no_data, "NONE", -1, x_res, y_res, minX, minY, maxX,
	 maxY);

  error:
    if (md5)
	free (md5);
    if (in != NULL)
	fclose (in);
}

static int
is_valid_float (char *str)
{
/* testing for a valid worldfile float value */
    char *p = str;
    int point = 0;
    int sign = 0;
    int digit = 0;
    int len = strlen (str);
    int i;

    for (i = len - 1; i >= 0; i--)
      {
	  /* suppressing any trailing blank */
	  if (str[i] == ' ' || str[i] == '\t' || str[i] == '\r')
	      str[i] = '\0';
	  else
	      break;
      }

    while (1)
      {
	  /* skipping leading blanks */
	  if (*p != ' ' && *p != '\t')
	      break;
	  p++;
      }

/* testing for a well formatted float */
    while (1)
      {
	  if (*p == '\0')
	      break;
	  if (*p >= '0' && *p <= '9')
	      digit++;
	  else if (*p == '.')
	      point++;
	  else if (*p == ',')
	    {
		*p = '.';
		point++;
	    }
	  else if (*p == '+' || *p == '-')
	    {
		if (digit == 0 && point == 0 && sign == 0)
		    ;
		else
		    return 0;
	    }
	  else
	      return 0;
	  p++;
      }
    if (digit > 0 && sign <= 1 && point <= 1)
	return 1;
    return 0;
}

static int
parse_worldfile (const char *path, unsigned int width, unsigned int height,
		 double *minx, double *miny, double *maxx, double *maxy,
		 double *pres_x, double *pres_y)
{
/* attemtping to parse a WorldFile */
    int line_no = 0;
    int ok_res_x = 0;
    int ok_res_y = 0;
    int ok_x = 0;
    int ok_y = 0;
    char buf[1024];
    char *p = buf;
    double x = 0.0;
    double y = 0.0;
    double res_x = 0.0;
    double res_y = 0.0;
    FILE *in = fopen (path, "rb");

    if (in == NULL)
	return 0;

    while (1)
      {
	  int c = getc (in);
	  if (c == '\r')
	      continue;
	  if (c == '\n' || c == EOF)
	    {
		*p = '\0';
		if (line_no == 0)
		  {
		      if (is_valid_float (buf))
			{
			    res_x = atof (buf);
			    ok_res_x = 1;
			}
		  }
		if (line_no == 3)
		  {
		      if (is_valid_float (buf))
			{
			    res_y = atof (buf) * -1.0;
			    ok_res_y = 1;
			}
		  }
		if (line_no == 4)
		  {
		      if (is_valid_float (buf))
			{
			    x = atof (buf);
			    ok_x = 1;
			}
		  }
		if (line_no == 5)
		  {
		      if (is_valid_float (buf))
			{
			    y = atof (buf);
			    ok_y = 1;
			}
		  }
		if (c == EOF)
		    break;
		p = buf;
		line_no++;
		continue;
	    }
	  *p++ = c;
      }
    fclose (in);

    if (ok_x && ok_y && ok_res_x && ok_res_y)
      {
	  *minx = x;
	  *maxy = y;
	  *maxx = *minx + ((double) width * res_x);
	  *miny = *maxy - ((double) height * res_y);
	  *pres_x = res_x;
	  *pres_y = res_y;
	  return 1;
      }
    return 0;
}

static void
do_sniff_jpeg_image (const char *src_path, int worldfile, int with_md5)
{
/* sniffing a JPEG image */
    unsigned int width;
    unsigned int height;
    unsigned char pixel_type;
    int is_geo = 0;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double x_res;
    double y_res;
    const char *pixel = "RGB";
    unsigned int bands = 3;
    char *jgw_path = NULL;
    char *md5 = NULL;

    if (rl2_get_jpeg_infos (src_path, &width, &height, &pixel_type) != RL2_OK)
	return;
    if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  pixel = "GRAYSCALE";
	  bands = 1;
      }
    if (worldfile)
      {
	  jgw_path = rl2_build_worldfile_path (src_path, ".jgw");
	  if (jgw_path != NULL)
	    {
		is_geo =
		    parse_worldfile (jgw_path, width, height, &minX, &minY,
				     &maxX, &maxY, &x_res, &y_res);
		free (jgw_path);
	    }
	  if (!is_geo)
	    {
		jgw_path = rl2_build_worldfile_path (src_path, ".jpgw");
		if (jgw_path != NULL)
		  {
		      is_geo =
			  parse_worldfile (jgw_path, width, height, &minX,
					   &minY, &maxX, &maxY, &x_res, &y_res);
		      free (jgw_path);
		  }
	    }
	  if (!is_geo)
	    {
		jgw_path = rl2_build_worldfile_path (src_path, ".wld");
		if (jgw_path != NULL)
		  {
		      is_geo =
			  parse_worldfile (jgw_path, width, height, &minX,
					   &minY, &maxX, &maxY, &x_res, &y_res);
		      free (jgw_path);
		  }
	    }
      }

    if (with_md5)
	md5 = rl2_compute_file_md5_checksum (src_path);
    if (is_geo)
	printf
	    ("JPEG+JGW\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t\t%s\t%d\t%1.12f\t%1.12f\t%1.8f\t%1.8f\t%1.8f\t%1.8f\n",
	     (md5 == NULL) ? "" : md5, src_path, width, height, "UINT8", pixel,
	     bands, "JPEG", -1, x_res, y_res, minX, minY, maxX, maxY);
    else
	printf ("JPEG\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t\t%s\n",
		(md5 == NULL) ? "" : md5, src_path, width, height, "UINT8",
		pixel, bands, "JPEG");
}

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */

static void
do_sniff_jpeg2000_image (const char *src_path, int worldfile, int with_md5)
{
/* sniffing a Jpeg2000 image */
    unsigned int width;
    unsigned int height;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int is_geo = 0;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double x_res;
    double y_res;
    const char *pixel = "UNKNOWN";
    const char *sample = "UNKNOWN";
    char *j2w_path = NULL;
    char *md5 = NULL;
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned char num_levels;

    if (rl2_get_jpeg2000_infos
	(src_path, &width, &height, &sample_type, &pixel_type, &num_bands,
	 &tile_width, &tile_height, &num_levels) != RL2_OK)
	return;
    if (pixel_type == RL2_PIXEL_GRAYSCALE)
	pixel = "GRAYSCALE";
    if (pixel_type == RL2_PIXEL_DATAGRID)
	pixel = "DATAGRID";
    if (pixel_type == RL2_PIXEL_MULTIBAND)
	pixel = "MULTIBAND";
    if (pixel_type == RL2_PIXEL_RGB)
	pixel = "RGB";
    if (sample_type == RL2_SAMPLE_UINT8)
	sample = "UINT8";
    if (sample_type == RL2_SAMPLE_UINT16)
	sample = "UINT16";
    if (worldfile)
      {
	  j2w_path = rl2_build_worldfile_path (src_path, ".j2w");
	  if (j2w_path != NULL)
	    {
		is_geo =
		    parse_worldfile (j2w_path, width, height, &minX, &minY,
				     &maxX, &maxY, &x_res, &y_res);
		free (j2w_path);
	    }
	  if (!is_geo)
	    {
		j2w_path = rl2_build_worldfile_path (src_path, ".wld");
		if (j2w_path != NULL)
		  {
		      is_geo =
			  parse_worldfile (j2w_path, width, height, &minX,
					   &minY, &maxX, &maxY, &x_res, &y_res);
		      free (j2w_path);
		  }
	    }
      }

    if (with_md5)
	md5 = rl2_compute_file_md5_checksum (src_path);
    if (is_geo)
	printf
	    ("Jpeg2000+J2W\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t\t%s\t%d\t%1.12f\t%1.12f\t%1.8f\t%1.8f\t%1.8f\t%1.8f\n",
	     (md5 == NULL) ? "" : md5, src_path, width, height, sample, pixel,
	     num_bands, "Jpeg2000", -1, x_res, y_res, minX, minY, maxX, maxY);
    else
	printf ("JPEG\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t\t%s\n",
		(md5 == NULL) ? "" : md5, src_path, width, height, sample,
		pixel, num_bands, "Jpeg2000");
}

#endif /* end OpenJpeg conditional */

static int
recover_incomplete_geotiff (TIFF * in, uint32 width, uint32 height,
			    double *minX, double *minY, double *maxX,
			    double *maxY, double *hResolution,
			    double *vResolution)
{
/* final desperate attempt to recover an incomplete GeoTIFF */
    double *tie_points;
    double *scale;
    uint16 count;
    double res_x = DBL_MAX;
    double res_y = DBL_MAX;
    double x = DBL_MAX;
    double y = DBL_MAX;

    if (TIFFGetField (in, TIFFTAG_GEOPIXELSCALE, &count, &scale))
      {
	  if (count >= 2 && scale[0] != 0.0 && scale[1] != 0.0)
	    {
		res_x = scale[0];
		res_y = scale[1];
	    }
      }
    if (TIFFGetField (in, TIFFTAG_GEOTIEPOINTS, &count, &tie_points))
      {
	  int i;
	  int max_count = count / 6;
	  for (i = 0; i < max_count; i++)
	    {
		x = tie_points[i * 6 + 3];
		y = tie_points[i * 6 + 4];
	    }
      }
    if (x == DBL_MAX || y == DBL_MAX || res_x == DBL_MAX || res_y == DBL_MAX)
	return 0;

/* computing the pixel resolution */
    *minX = x;
    *maxX = x + ((double) width * res_x);
    *minY = y - ((double) height * res_y);
    *maxY = y;
    *hResolution = (*maxX - *minX) / (double) width;
    *vResolution = (*maxY - *minY) / (double) height;
    return 1;
}

static void
sniff_tiff_pixel (TIFF * in, uint16 * bitspersample, uint16 * samplesperpixel,
		  uint16 * photometric, uint16 * sampleformat,
		  uint16 * compression)
{
/* retrieving the TIFF sample and pixel type */
    uint16 value16;
    if (TIFFGetField (in, TIFFTAG_BITSPERSAMPLE, &value16) != 0)
	*bitspersample = value16;
    else
	*bitspersample = 1;
    if (TIFFGetField (in, TIFFTAG_SAMPLESPERPIXEL, &value16) != 0)
	*samplesperpixel = value16;
    else
	*samplesperpixel = 1;
    if (TIFFGetField (in, TIFFTAG_PHOTOMETRIC, &value16) != 0)
	*photometric = value16;
    else
	*photometric = 0;
    if (TIFFGetField (in, TIFFTAG_SAMPLEFORMAT, &value16) != 0)
	*sampleformat = value16;
    else
	*sampleformat = 1;
    if (TIFFGetField (in, TIFFTAG_COMPRESSION, &value16) != 0)
	*compression = value16;
    else
	*compression = 1;

}

static void
do_sniff_geotiff_image (const char *src_path, int with_md5)
{
/* sniffing a GeoTIFF image */
    uint32 width = 0;
    uint32 height = 0;
    uint16 bitspersample;
    uint16 bands;
    uint16 photometric;
    uint16 sampleformat;
    uint16 tiff_compression;
    const char *sample = "unknown";
    const char *pixel = "unknown";
    const char *compression = "unknown";
    int is_geotiff = 0;
    int srid = -1;
    double cx;
    double cy;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double x_res;
    double y_res;
    short pixel_mode = RasterPixelIsArea;
    GTIFDefn definition;
    char *md5 = NULL;
    TIFF *in = (TIFF *) 0;
    GTIF *gtif = (GTIF *) 0;

/* suppressing TIFF messages */
    TIFFSetErrorHandler (NULL);
    TIFFSetWarningHandler (NULL);

/* reading from file */
    in = XTIFFOpen (src_path, "r");
    if (in == NULL)
	goto error;
    gtif = GTIFNew (in);
    if (gtif == NULL)
	goto error;

/* retrieving the TIFF dimensions */
    TIFFGetField (in, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField (in, TIFFTAG_IMAGEWIDTH, &width);
    sniff_tiff_pixel (in, &bitspersample, &bands, &photometric, &sampleformat,
		      &tiff_compression);
    switch (sampleformat)
      {
      case SAMPLEFORMAT_UINT:
	  switch (bitspersample)
	    {
	    case 1:
		sample = "1-BIT";
		break;
	    case 2:
		sample = "2-BIT";
		break;
	    case 4:
		sample = "4-BIT";
		break;
	    case 8:
		sample = "UINT8";
		break;
	    case 16:
		sample = "UINT16";
		break;
	    case 32:
		sample = "UINT32";
		break;
	    };
	  break;
      case SAMPLEFORMAT_INT:
	  switch (bitspersample)
	    {
	    case 8:
		sample = "INT8";
		break;
	    case 16:
		sample = "INT16";
		break;
	    case 32:
		sample = "INT32";
		break;
	    };
	  break;
      case SAMPLEFORMAT_IEEEFP:
	  switch (bitspersample)
	    {
	    case 32:
		sample = "FLOAT";
		break;
	    case 64:
		sample = "DOUBLE";
		break;
	    };
	  break;
      };
    switch (photometric)
      {
      case PHOTOMETRIC_RGB:
	  if (bitspersample == 8 || bitspersample == 16)
	    {
		if (bands == 3)
		    pixel = "RGB";
		else
		    pixel = "MULTIBAND";
	    }
	  break;
      case PHOTOMETRIC_PALETTE:
	  if ((bitspersample == 1 || bitspersample == 2 || bitspersample == 4
	       || bitspersample == 8) && bands == 1)
	      pixel = "PALETTE";
	  break;
      case PHOTOMETRIC_MINISWHITE:
      case PHOTOMETRIC_MINISBLACK:
	  if (bitspersample == 1 && bands == 1)
	      pixel = "MONOCHROME";
	  else if ((bitspersample == 2 || bitspersample == 4
		    || bitspersample == 8) && bands == 1)
	      pixel = "GRAYSCALE";
	  else if ((bitspersample == 16 || bitspersample == 32
		    || bitspersample == 64) && bands == 1)
	      pixel = "DATAGRID";
      };
    switch (tiff_compression)
      {
      case COMPRESSION_NONE:
	  compression = "NONE";
	  break;
      case COMPRESSION_CCITTRLE:
	  compression = "RLE";
	  break;
      case COMPRESSION_CCITTFAX3:
	  compression = "FAX3";
	  break;
      case COMPRESSION_CCITTFAX4:
	  compression = "FAX4";
	  break;
      case COMPRESSION_LZW:
	  compression = "LZW";
	  break;
      case COMPRESSION_OJPEG:
	  compression = "OldJPEG";
	  break;
      case COMPRESSION_JPEG:
	  compression = "JPEG";
	  break;
      case COMPRESSION_DEFLATE:
	  compression = "DEFLATE";
	  break;
      case COMPRESSION_ADOBE_DEFLATE:
	  compression = "AdobeDEFLATE";
	  break;
      case COMPRESSION_JP2000:
	  compression = "JP2000";
	  break;
      case COMPRESSION_LZMA:
	  compression = "LZMA";
	  break;
      };

    if (!GTIFGetDefn (gtif, &definition))
	goto recover;
/* retrieving the EPSG code */
    if (definition.PCS == 32767)
      {
	  if (definition.GCS != 32767)
	      srid = definition.GCS;
      }
    else
	srid = definition.PCS;

/* computing the corners coords */
    cx = 0.0;
    cy = 0.0;
    GTIFImageToPCS (gtif, &cx, &cy);
    minX = cx;
    maxY = cy;
    cx = 0.0;
    cy = height;
    GTIFImageToPCS (gtif, &cx, &cy);
    minY = cy;
    cx = width;
    cy = 0.0;
    GTIFImageToPCS (gtif, &cx, &cy);
    maxX = cx;

/* computing the pixel resolution */
    x_res = (maxX - minX) / (double) width;
    y_res = (maxY - minY) / (double) height;
    is_geotiff = 1;

/* retrieving GTRasterTypeGeoKey */
    if (!GTIFKeyGet (gtif, GTRasterTypeGeoKey, &pixel_mode, 0, 1))
	pixel_mode = RasterPixelIsArea;
    if (pixel_mode == RasterPixelIsPoint)
      {
	  /* adjusting the BBOX */
	  minX -= x_res / 2.0;
	  minY -= y_res / 2.0;
	  maxX += x_res / 2.0;
	  maxY += y_res / 2.0;

      }
    goto print;

  recover:
    is_geotiff =
	recover_incomplete_geotiff (in, width, height, &minX, &minY, &maxX,
				    &maxY, &x_res, &y_res);

  print:
    if (with_md5)
	md5 = rl2_compute_file_md5_checksum (src_path);

    if (is_geotiff)
	printf
	    ("GeoTIFF\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t\t%s\t%d\t%1.12f\t%1.12f\t%1.8f\t%1.8f\t%1.8f\t%1.8f\n",
	     (md5 == NULL) ? "" : md5, src_path, width, height, sample, pixel,
	     bands, compression, srid, x_res, y_res, minX, minY, maxX, maxY);
    else
	printf ("TIFF\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t\t%s\n",
		(md5 == NULL) ? "" : md5, src_path, width, height, sample,
		pixel, bands, compression);

  error:
    if (in != (TIFF *) 0)
	XTIFFClose (in);
    if (gtif != (GTIF *) 0)
	GTIFFree (gtif);
    if (md5)
	free (md5);
}

static void
do_sniff_tiff_image (const char *src_path, int with_md5)
{
/* sniffing a TIFF image */
    uint32 width = 0;
    uint32 height = 0;
    uint16 bitspersample;
    uint16 bands;
    uint16 photometric;
    uint16 sampleformat;
    uint16 tiff_compression;
    const char *sample = "unknown";
    const char *pixel = "unknown";
    const char *compression = "unknown";
    int is_geotiff = 0;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double x_res;
    double y_res;
    char *tfw_path = NULL;
    char *md5 = NULL;
    TIFF *in = (TIFF *) 0;

/* suppressing TIFF messages */
    TIFFSetErrorHandler (NULL);
    TIFFSetWarningHandler (NULL);

/* reading from file */
    in = TIFFOpen (src_path, "r");
    if (in == NULL)
	goto error;

/* retrieving the TIFF dimensions */
    TIFFGetField (in, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField (in, TIFFTAG_IMAGEWIDTH, &width);
    sniff_tiff_pixel (in, &bitspersample, &bands, &photometric, &sampleformat,
		      &tiff_compression);
    switch (sampleformat)
      {
      case SAMPLEFORMAT_UINT:
	  switch (bitspersample)
	    {
	    case 1:
		sample = "1-BIT";
		break;
	    case 2:
		sample = "2-BIT";
		break;
	    case 4:
		sample = "4-BIT";
		break;
	    case 8:
		sample = "UINT8";
		break;
	    case 16:
		sample = "UINT16";
		break;
	    case 32:
		sample = "UINT32";
		break;
	    };
	  break;
      case SAMPLEFORMAT_INT:
	  switch (bitspersample)
	    {
	    case 8:
		sample = "INT8";
		break;
	    case 16:
		sample = "INT16";
		break;
	    case 32:
		sample = "INT32";
		break;
	    };
	  break;
      case SAMPLEFORMAT_IEEEFP:
	  switch (bitspersample)
	    {
	    case 32:
		sample = "FLOAT";
		break;
	    case 64:
		sample = "DOUBLE";
		break;
	    };
	  break;
      };
    switch (photometric)
      {
      case PHOTOMETRIC_RGB:
	  if (bitspersample == 8 || bitspersample == 16)
	    {
		if (bands == 3)
		    pixel = "RGB";
		else
		    pixel = "MULTIBAND";
	    }
	  break;
      case PHOTOMETRIC_PALETTE:
	  if ((bitspersample == 1 || bitspersample == 2 || bitspersample == 4
	       || bitspersample == 8) && bands == 1)
	      pixel = "PALETTE";
	  break;
      case PHOTOMETRIC_MINISWHITE:
      case PHOTOMETRIC_MINISBLACK:
	  if (bitspersample == 1 && bands == 1)
	      pixel = "MONOCHROME";
	  else if ((bitspersample == 2 || bitspersample == 4
		    || bitspersample == 8) && bands == 1)
	      pixel = "GRAYSCALE";
	  else if ((bitspersample == 16 || bitspersample == 32
		    || bitspersample == 64) && bands == 1)
	      pixel = "DATAGRID";
      };
    switch (tiff_compression)
      {
      case COMPRESSION_NONE:
	  compression = "NONE";
	  break;
      case COMPRESSION_CCITTRLE:
	  compression = "RLE";
	  break;
      case COMPRESSION_CCITTFAX3:
	  compression = "FAX3";
	  break;
      case COMPRESSION_CCITTFAX4:
	  compression = "FAX4";
	  break;
      case COMPRESSION_LZW:
	  compression = "LZW";
	  break;
      case COMPRESSION_OJPEG:
	  compression = "OldJPEG";
	  break;
      case COMPRESSION_JPEG:
	  compression = "JPEG";
	  break;
      case COMPRESSION_DEFLATE:
	  compression = "DEFLATE";
	  break;
      case COMPRESSION_ADOBE_DEFLATE:
	  compression = "AdobeDEFLATE";
	  break;
      case COMPRESSION_JP2000:
	  compression = "JP2000";
	  break;
      case COMPRESSION_LZMA:
	  compression = "LZMA";
	  break;
      };

    tfw_path = rl2_build_worldfile_path (src_path, ".tfw");
    if (tfw_path != NULL)
      {
	  is_geotiff =
	      parse_worldfile (tfw_path, width, height, &minX, &minY, &maxX,
			       &maxY, &x_res, &y_res);
	  free (tfw_path);
      }
    if (!is_geotiff)
      {
	  tfw_path = rl2_build_worldfile_path (src_path, ".tifw");
	  if (tfw_path != NULL)
	    {
		is_geotiff =
		    parse_worldfile (tfw_path, width, height, &minX, &minY,
				     &maxX, &maxY, &x_res, &y_res);
		free (tfw_path);
	    }
      }
    if (!is_geotiff)
      {
	  tfw_path = rl2_build_worldfile_path (src_path, ".wld");
	  if (tfw_path != NULL)
	    {
		is_geotiff =
		    parse_worldfile (tfw_path, width, height, &minX, &minY,
				     &maxX, &maxY, &x_res, &y_res);
		free (tfw_path);
	    }
      }

    if (with_md5)
	md5 = rl2_compute_file_md5_checksum (src_path);
    if (is_geotiff)
	printf
	    ("TIFF+TFW\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t\t%s\t%d\t%1.12f\t%1.12f\t%1.8f\t%1.8f\t%1.8f\t%1.8f\n",
	     (md5 == NULL) ? "" : md5, src_path, width, height, sample, pixel,
	     bands, compression, -1, x_res, y_res, minX, minY, maxX, maxY);
    else
	printf ("TIFF\t%s\t%s\t%u\t%u\t%s\t%s\t%u\t\t%s\n",
		(md5 == NULL) ? "" : md5, src_path, width, height, sample,
		pixel, bands, compression);

  error:
    if (in != (TIFF *) 0)
	TIFFClose (in);
    if (md5)
	free (md5);
}

static void
do_sniff_file (const char *src_path, int worldfile, int with_md5)
{
/* sniffing a single file */
    if (is_ascii_grid (src_path))
	do_sniff_ascii_grid (src_path, with_md5);
    if (is_jpeg_image (src_path))
	do_sniff_jpeg_image (src_path, worldfile, with_md5);

#ifndef OMIT_OPENJPEG		/* only if OpenJpeg is enabled */
    if (is_jpeg2000_image (src_path))
	do_sniff_jpeg2000_image (src_path, worldfile, with_md5);
#endif /* end OpenJpeg conditional */

    if (worldfile)
	do_sniff_tiff_image (src_path, with_md5);
    else
	do_sniff_geotiff_image (src_path, with_md5);
}

static int
check_extension_match (const char *file_name, const char *file_ext)
{
/* checks the file extension */
    const char *mark = NULL;
    const char *p = file_name;
    int len;
    char *ext;
    int match = 0;
    if (file_ext == NULL)
      {
	  /* any supported extension */
	  while (*p != '\0')
	    {
		if (*p == '.')
		    mark = p;
		p++;
	    }
	  if (mark == NULL)
	      return 0;
	  if (strcasecmp (mark, ".tif") == 0)
	      return 1;
	  if (strcasecmp (mark, ".jpg") == 0)
	      return 1;
	  if (strcasecmp (mark, ".asc") == 0)
	      return 1;
	  return 0;
      }

    len = strlen (file_ext);
    if (*file_ext == '.')
      {
	  /* file extension starts with dot */
	  ext = malloc (len + 1);
	  strcpy (ext, file_ext);
      }
    else
      {
	  /* file extension doesn't start with dot */
	  ext = malloc (len + 2);
	  *ext = '.';
	  strcpy (ext + 1, file_ext);
      }
    while (*p != '\0')
      {
	  if (*p == '.')
	      mark = p;
	  p++;
      }
    if (mark == NULL)
      {
	  free (ext);
	  return 0;
      }
    match = strcasecmp (mark, ext);
    free (ext);
    if (match == 0)
	return 1;
    return 0;
}

static int
do_sniff_dir (const char *dir_path, const char *file_ext, int worldfile,
	      int with_md5)
{
/* sniffing a whole directory */
#if defined(_WIN32) && !defined(__MINGW32__)
/* Visual Studio .NET */
    struct _finddata_t c_file;
    intptr_t hFile;
    int cnt = 0;
    int total = 0;
    char *search;
    char *path;
    int ret;
    if (_chdir (dir_path) < 0)
	return 0;
    search = sqlite3_mprintf ("*%s", file_ext);
    if ((hFile = _findfirst (search, &c_file)) == -1L)
	;
    else
      {
	  while (1)
	    {
		if ((c_file.attrib & _A_RDONLY) == _A_RDONLY
		    || (c_file.attrib & _A_NORMAL) == _A_NORMAL)
		    total++;
		if (_findnext (hFile, &c_file) != 0)
		    break;
	    }
	  _findclose (hFile);
	  if ((hFile = _findfirst (search, &c_file)) == -1L)
	      ;
	  else
	    {
		while (1)
		  {
		      if ((c_file.attrib & _A_RDONLY) == _A_RDONLY
			  || (c_file.attrib & _A_NORMAL) == _A_NORMAL)
			{
			    path =
				sqlite3_mprintf ("%s/%s", dir_path,
						 c_file.name);
			    ret =
				do_import_file (handle, path, cvg, section,
						worldfile, force_srid,
						pyramidize, sample_type,
						pixel_type, num_bands, tile_w,
						tile_h, compression, quality,
						stmt_data, stmt_tils, stmt_sect,
						stmt_levl, stmt_upd_sect,
						verbose, cnt + 1, total);
			    sqlite3_free (path);
			    if (!ret)
				goto error;
			    cnt++;
			}
		      if (_findnext (hFile, &c_file) != 0)
			  break;
		  }
	      error:
		_findclose (hFile);
	    }
	  sqlite3_free (search);
	  return cnt;
#else
/* not Visual Studio .NET */
    int cnt = 0;
    int total = 0;
    char *path;
    struct dirent *entry;
    char dir_fullpath[4096];
    DIR *dir;
#ifdef _WIN32
    _chdir (dir_path);
#else /* not WIN32 */
    chdir (dir_path);
#endif
    dir = opendir (".");
    if (!dir)
	return 0;
    while (1)
      {
	  /* counting how many valid entries */
	  entry = readdir (dir);
	  if (!entry)
	      break;
	  if (!check_extension_match (entry->d_name, file_ext))
	      continue;
	  total++;
      }
    rewinddir (dir);
#ifdef _WIN32
    _getcwd (dir_fullpath, 4096);
#else /* not WIN32 */
    getcwd (dir_fullpath, 4096);
#endif
    while (1)
      {
	  /* scanning dir-entries */
	  entry = readdir (dir);
	  if (!entry)
	      break;
	  if (!check_extension_match (entry->d_name, file_ext))
	      continue;
#ifdef _WIN32
	  path = sqlite3_mprintf ("%s\\%s", dir_fullpath, entry->d_name);
#else /* not WIN32 */
	  path = sqlite3_mprintf ("%s/%s", dir_fullpath, entry->d_name);
#endif
	  do_sniff_file (path, worldfile, with_md5);
	  sqlite3_free (path);
	  cnt++;
      }
    closedir (dir);
    return cnt;
#endif
}

static void
open_db (sqlite3 ** handle, void *cache, void *priv_data)
{
/* opening the DB */
    sqlite3 *db_handle;
    int ret;

    *handle = NULL;
    fprintf (stderr, "     SQLite version: %s\n", sqlite3_libversion ());
    fprintf (stderr, " SpatiaLite version: %s\n", spatialite_version ());
    fprintf (stderr, "RasterLite2 version: %s\n\n", rl2_version ());

    ret =
	sqlite3_open_v2 (":memory:", &db_handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open ':memory:': %s\n",
		   sqlite3_errmsg (db_handle));
	  sqlite3_close (db_handle);
	  return;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);

    *handle = db_handle;
    return;
}

static void
do_help ()
{
/* printing the argument list */
    fprintf (stderr, "\n\nusage: rl2sniff ARGLIST\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr,
	     "-h or --help                    print this help message\n");
    fprintf (stderr,
	     "-src or --src-path    pathname  single Image/Raster path\n");
    fprintf (stderr, "-dir or --dir-path    pathname  directory path\n");
    fprintf (stderr,
	     "-ext or --file-ext    extension file extension (e.g. .tif)\n");
    fprintf (stderr, "-wf or --worldfile              requires a Worldfile\n");
    fprintf (stderr,
	     "-md5 or --md5-checksum          enabling MD5 checksums\n");
}

int
main (int argc, char *argv[])
{
/* the MAIN function simply perform arguments checking */
    sqlite3 *handle;
    int i;
    int next_arg = ARG_NONE;
    const char *src_path = NULL;
    const char *dir_path = ".";
    const char *file_ext = NULL;
    int worldfile = 0;
    int with_md5 = 0;
    int error = 0;
    void *cache;
    void *priv_data;

    for (i = 1; i < argc; i++)
      {
	  /* parsing the invocation arguments */
	  if (next_arg != ARG_NONE)
	    {
		switch (next_arg)
		  {
		  case ARG_SRC_PATH:
		      src_path = argv[i];
		      break;
		  case ARG_DIR_PATH:
		      dir_path = argv[i];
		      break;
		  case ARG_FILE_EXT:
		      file_ext = argv[i];
		      break;
		  };
		next_arg = ARG_NONE;
		continue;
	    }

	  if (strcasecmp (argv[i], "--help") == 0
	      || strcmp (argv[i], "-h") == 0)
	    {
		do_help ();
		return -1;
	    }
	  if (strcmp (argv[i], "-src") == 0
	      || strcasecmp (argv[i], "--src-path") == 0)
	    {
		next_arg = ARG_SRC_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-dir") == 0
	      || strcasecmp (argv[i], "--dir-path") == 0)
	    {
		next_arg = ARG_DIR_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-ext") == 0
	      || strcasecmp (argv[i], "--file-ext") == 0)
	    {
		next_arg = ARG_FILE_EXT;
		continue;
	    }
	  if (strcmp (argv[i], "-wf") == 0
	      || strcasecmp (argv[i], "--worldfile") == 0)
	    {
		worldfile = 1;
		continue;
	    }
	  if (strcmp (argv[i], "-md5") == 0
	      || strcasecmp (argv[i], "--enable-md5") == 0)
	    {
		with_md5 = 1;
		continue;
	    }
	  fprintf (stderr, "unknown argument: %s\n", argv[i]);
	  error = 1;
      }
    if (error)
      {
	  do_help ();
	  return -1;
      }

/* opening the DB */
    cache = spatialite_alloc_connection ();
    priv_data = rl2_alloc_private ();
    open_db (&handle, cache, priv_data);
    if (!handle)
	return -1;

    printf ("image_format\tMD5_checksum\timage_path\twidth\theight\tsample_type"
	    "\tpixel_type\tbands\tno_data\tcompression\tsrid\tx_resolution"
	    "\ty_resolution\tmin_x\tmin_y\tmax_x\tmax_y\n");
    if (src_path != NULL)
	do_sniff_file (src_path, worldfile, with_md5);
    else
	do_sniff_dir (dir_path, file_ext, worldfile, with_md5);

    sqlite3_close (handle);
    rl2_cleanup_private (priv_data);
    spatialite_cleanup_ex (cache);
    spatialite_shutdown ();
    return 0;
}
