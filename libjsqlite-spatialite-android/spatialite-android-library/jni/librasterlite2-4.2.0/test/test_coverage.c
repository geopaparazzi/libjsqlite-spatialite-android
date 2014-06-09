/*

 test_coverage.c -- RasterLite2 Test Case

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
#include <stdio.h>

#include "rasterlite2/rasterlite2.h"

int
main (int argc, char *argv[])
{
    rl2CoveragePtr coverage;
    rl2PixelPtr no_data;
    rl2PixelPtr pix2;
    char int8;
    unsigned char uint8;
    short int16;
    unsigned short uint16;
    int int32;
    unsigned int uint32;
    float flt32;
    double flt64;
    unsigned char sample;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    int is_transparent;
    unsigned char compression;
    int is_compressed;
    int srid;
    int quality;
    unsigned int tileWidth;
    unsigned int tileHeight;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (rl2_create_coverage (NULL, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - NULL name\n");
	  return -1;
      }

    if (rl2_create_coverage ("alpha", 0xff, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - invalid sample\n");
	  return -2;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, 0xff, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - invalid pixel\n");
	  return -3;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 0,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - invalid bands\n");
	  return -4;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     0xff, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - invalid compression\n");
	  return -5;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_NONE, 0, 0, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - invalid tile width\n");
	  return -6;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 0, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - invalid tile height\n");
	  return -7;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_NONE, 0, 510, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - invalid tile width (x8)\n");
	  return -8;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 510, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - invalid tile height (x8)\n");
	  return -9;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_MONOCHROME, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - monochrome\n");
	  return -10;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - monochrome\n");
	  return -11;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1,
			     RL2_COMPRESSION_JPEG, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - monochrome/jpeg\n");
	  return -12;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT16, RL2_PIXEL_PALETTE, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - palette\n");
	  return -13;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE, 1,
			     RL2_COMPRESSION_JPEG, 70, 1024, 1024,
			     NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - palette/jpeg\n");
	  return -14;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE, 2,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - palette/jpeg\n");
	  return -15;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 2,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - grayscale bands\n");
	  return -16;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT16, RL2_PIXEL_GRAYSCALE, 1,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - grayscale 16\n");
	  return -17;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 2,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - rgb bands\n");
	  return -18;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT16, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) == NULL)
      {
	  fprintf (stderr, "Invalid coverage - rgb 16\n");
	  return -19;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_GIF, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - rgb/gif\n");
	  return -20;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 1,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - multiband bands\n");
	  return -21;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_FLOAT, RL2_PIXEL_MULTIBAND, 3,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - multiband float\n");
	  return -22;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, 3,
			     RL2_COMPRESSION_GIF, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - multiband/gif\n");
	  return -23;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_FLOAT, RL2_PIXEL_DATAGRID, 2,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - multiband bands\n");
	  return -24;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_2_BIT, RL2_PIXEL_DATAGRID, 1,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - datagrid 2 bit\n");
	  return -25;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1,
			     RL2_COMPRESSION_JPEG, 0, 1024, 1024, NULL) != NULL)
      {
	  fprintf (stderr, "Invalid coverage - datagrid/jpeg\n");
	  return -26;
      }

    coverage = rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
				    RL2_COMPRESSION_JPEG, 0, 1024, 1024, NULL);
    if (coverage == NULL)
      {
	  fprintf (stderr, "Unable to create a valid coverage\n");
	  return -27;
      }

    if (rl2_coverage_georeference (coverage, 4326, 0.5, 0.5) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set georeferencing: coverage\n");
	  return -28;
      }

    if (rl2_get_coverage_name (coverage) == NULL)
      {
	  fprintf (stderr, "Unable to retrieve the coverage name\n");
	  return -29;
      }

    if (rl2_get_coverage_type (coverage, &sample_type, &pixel_type, &num_bands)
	!= RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage type\n");
	  return -30;
      }

    if (sample_type != RL2_SAMPLE_UINT8)
      {
	  fprintf (stderr, "Unexpected coverage sample\n");
	  return -31;
      }

    if (pixel_type != RL2_PIXEL_RGB)
      {
	  fprintf (stderr, "Unexpected coverage pixel\n");
	  return -32;
      }

    if (num_bands != 3)
      {
	  fprintf (stderr, "Unexpected coverage # bands\n");
	  return -33;
      }

    if (rl2_get_coverage_compression (coverage, &compression, &quality) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage compression\n");
	  return -34;
      }

    if (compression != RL2_COMPRESSION_JPEG)
      {
	  fprintf (stderr, "Unexpected coverage compression\n");
	  return -35;
      }

    if (rl2_is_coverage_uncompressed (coverage, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage not compressed\n");
	  return -36;
      }

    if (is_compressed != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected coverage not compressed\n");
	  return -37;
      }

    if (rl2_is_coverage_compression_lossless (coverage, &is_compressed) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage compression lossless\n");
	  return -38;
      }

    if (is_compressed != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected coverage compression lossless\n");
	  return -39;
      }

    if (rl2_is_coverage_compression_lossy (coverage, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage compression lossy\n");
	  return -40;
      }

    if (is_compressed != RL2_TRUE)
      {
	  fprintf (stderr, "Unexpected coverage compression lossy\n");
	  return -41;
      }

    if (rl2_get_coverage_tile_size (coverage, &tileWidth, &tileHeight) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage tile size\n");
	  return -42;
      }

    if (tileWidth != 1024)
      {
	  fprintf (stderr, "Unexpected coverage tile width\n");
	  return -43;
      }

    if (tileHeight != 1024)
      {
	  fprintf (stderr, "Unexpected coverage tile height\n");
	  return -44;
      }

    if (rl2_get_coverage_srid (coverage, &srid) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage SRID\n");
	  return -45;
      }

    if (srid != 4326)
      {
	  fprintf (stderr, "Unexpected coverage SRID\n");
	  return -46;
      }

    rl2_destroy_coverage (coverage);

    rl2_destroy_coverage (NULL);

    if (rl2_coverage_georeference (NULL, 4326, 0.5, 0.5) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: georeferencing NULL coverage\n");
	  return -47;
      }

    if (rl2_get_coverage_name (NULL) != NULL)
      {
	  fprintf (stderr, "ERROR: NULL coverage name\n");
	  return -48;
      }

    if (rl2_get_coverage_type (NULL, &sample_type, &pixel_type, &num_bands) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL coverage type\n");
	  return -49;
      }

    if (rl2_get_coverage_compression (NULL, &compression, &quality) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL coverage compression\n");
	  return -50;
      }

    if (rl2_is_coverage_uncompressed (NULL, &is_compressed) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL coverage not compressed\n");
	  return -51;
      }

    if (rl2_is_coverage_compression_lossless (NULL, &is_compressed) !=
	RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL coverage compression lossless\n");
	  return -52;
      }

    if (rl2_is_coverage_compression_lossy (NULL, &is_compressed) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL coverage compression lossy\n");
	  return -53;
      }

    if (rl2_get_coverage_tile_size (NULL, &tileWidth, &tileHeight) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL coverage tile width\n");
	  return -54;
      }

    if (rl2_get_coverage_srid (NULL, &srid) != RL2_ERROR)
      {
	  fprintf (stderr, "ERROR: NULL coverage SRID\n");
	  return -56;
      }

    if (rl2_get_coverage_no_data (NULL) != NULL)
      {
	  fprintf (stderr, "ERROR: NULL coverage NoData\n");
	  return -57;
      }

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -58;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Red NoData #1\n");
	  return -59;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Green NoData #1\n");
	  return -60;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Blue NoData #1\n");
	  return -61;
      }

    coverage = rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
				    RL2_COMPRESSION_JPEG, 20, 1024, 1024,
				    no_data);
    if (coverage == NULL)
      {
	  fprintf (stderr, "Unable to create a valid coverage\n");
	  return -62;
      }

    no_data = rl2_get_coverage_no_data (coverage);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to get NoData from coverage #1\n");
	  return -63;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_RED_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RED NoData from coverage #1\n");
	  return -64;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected RED NoData from coverage #1\n");
	  return -65;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get GREEN NoData from coverage #1\n");
	  return -66;
      }

    if (sample != 255)
      {
	  fprintf (stderr, "Unexpected GREEN NoData from coverage #1\n");
	  return -67;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BLUE NoData from coverage #1\n");
	  return -68;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected BLUE NoData from coverage #1\n");
	  return -69;
      }

    no_data = rl2_create_coverage_pixel (coverage);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a pixel for coverage #1\n");
	  return -70;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Red NoData #2\n");
	  return -71;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, 0) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Green NoData #2\n");
	  return -72;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, 255) != RL2_OK)
      {
	  fprintf (stderr, "Unable to set Blue NoData #2\n");
	  return -73;
      }
    rl2_destroy_pixel (no_data);

    no_data = rl2_get_coverage_no_data (coverage);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to get NoData from coverage #2\n");
	  return -74;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_RED_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get RED NoData from coverage #2\n");
	  return -75;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected RED NoData from coverage #2\n");
	  return -76;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get GREEN NoData from coverage #2\n");
	  return -77;
      }

    if (sample != 255)
      {
	  fprintf (stderr, "Unexpected GREEN NoData from coverage #2\n");
	  return -78;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, &sample) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get BLUE NoData from coverage #2\n");
	  return -79;
      }

    if (sample != 0)
      {
	  fprintf (stderr, "Unexpected BLUE NoData from coverage #2\n");
	  return -80;
      }

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -81;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_JPEG, 30, 1024, 1024,
			     no_data) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create coverage (invalid NoData)\n");
	  return -82;
      }

    rl2_destroy_pixel (no_data);

    rl2_destroy_coverage (coverage);

    coverage =
	rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 2,
			     RL2_COMPRESSION_NONE, 0, 1024, 1024, NULL);

    if (coverage == NULL)
      {
	  fprintf (stderr, "Unable to create coverage (invalid NoData)\n");
	  return -83;
      }

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, 3);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -84;
      }

    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 3);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -85;
      }

    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 2);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -86;
      }
    rl2_destroy_pixel (no_data);

    if (rl2_is_coverage_uncompressed (coverage, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage not compressed\n");
	  return -87;
      }

    if (is_compressed != RL2_TRUE)
      {
	  fprintf (stderr, "Unexpected coverage not compressed\n");
	  return -88;
      }

    rl2_destroy_coverage (coverage);

    coverage =
	rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 2,
			     RL2_COMPRESSION_LZMA, 0, 1024, 1024, NULL);

    if (coverage == NULL)
      {
	  fprintf (stderr, "Unable to create coverage (invalid NoData)\n");
	  return -89;
      }

    if (rl2_is_coverage_compression_lossless (coverage, &is_compressed) !=
	RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage compression lossless\n");
	  return -90;
      }

    if (is_compressed != RL2_TRUE)
      {
	  fprintf (stderr, "Unexpected coverage compression lossless\n");
	  return -91;
      }

    if (rl2_is_coverage_compression_lossy (coverage, &is_compressed) != RL2_OK)
      {
	  fprintf (stderr, "Unable to get coverage compression lossy\n");
	  return -92;
      }

    if (is_compressed != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected coverage compression lossy\n");
	  return -93;
      }

    rl2_destroy_coverage (coverage);

    if (rl2_get_coverage_no_data (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: get NoData (NULL coverage)\n");
	  return -94;
      }

    if (rl2_create_coverage_pixel (NULL) != NULL)
      {
	  fprintf (stderr, "Unexpected result: get pixel (NULL coverage)\n");
	  return -94;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_JPEG, 60, 1025, 1024,
			     no_data) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create coverage (invalid NoData)\n");
	  return -96;
      }

    if (rl2_create_coverage ("alpha", RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			     RL2_COMPRESSION_JPEG, 70, 1024, 1025,
			     no_data) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create coverage (invalid NoData)\n");
	  return -97;
      }

    if (rl2_create_pixel (0xff, RL2_PIXEL_RGB, 3) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a pixel (invalid sample)\n");
	  return -98;
      }

    if (rl2_create_pixel (RL2_SAMPLE_UINT8, 0xff, 3) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a pixel (invalid type)\n");
	  return -99;
      }

    if (rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 1) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a pixel (RGB invalid Bands)\n");
	  return -100;
      }

    if (rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 2) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a pixel (Gray invalid Bands)\n");
	  return -101;
      }

    if (rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 1) != NULL)
      {
	  fprintf (stderr,
		   "Unexpected result: create a pixel (MultiBand invalid Bands)\n");
	  return -102;
      }

    rl2_free (NULL);

    rl2_destroy_pixel (NULL);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 2);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create a NoData pixel\n");
	  return -103;
      }

    if (rl2_compare_pixels (no_data, NULL) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (valid, NULL)\n");
	  return -104;
      }

    if (rl2_compare_pixels (NULL, no_data) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (NULL, valid)\n");
	  return -105;
      }

    if (rl2_compare_pixels (NULL, NULL) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (NULL, NULL)\n");
	  return -106;
      }

    pix2 = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, 2);
    if (pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create a MultiBand16 pixel\n");
	  return -107;
      }

    if (rl2_compare_pixels (no_data, pix2) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: compare pixels (mismatching sample)\n");
	  return -108;
      }

    rl2_destroy_pixel (pix2);

    pix2 = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
    if (pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create an RGB pixel\n");
	  return -109;
      }

    if (rl2_compare_pixels (no_data, pix2) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: compare pixels (mismatching type)\n");
	  return -110;
      }

    rl2_destroy_pixel (pix2);

    pix2 = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, 6);
    if (pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create a MultiBand8 pixel\n");
	  return -111;
      }

    if (rl2_compare_pixels (no_data, pix2) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: compare pixels (mismatching Bands)\n");
	  return -112;
      }

    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID, 1);
    pix2 = rl2_create_pixel (RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL || pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create INT8 pixels for comparison\n");
	  return -113;
      }
    rl2_set_pixel_sample_int8 (no_data, -1);
    rl2_set_pixel_sample_int8 (pix2, -2);

    if (rl2_compare_pixels (no_data, pix2) != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (INT8)\n");
	  return -114;
      }
    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1);
    pix2 = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL || pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create UINT8 pixels for comparison\n");
	  return -115;
      }
    rl2_set_pixel_sample_uint8 (no_data, RL2_DATAGRID_BAND, 0);
    rl2_set_pixel_sample_uint8 (pix2, RL2_DATAGRID_BAND, 1);

    if (rl2_compare_pixels (no_data, pix2) != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (UINT8)\n");
	  return -116;
      }
    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_INT16, RL2_PIXEL_DATAGRID, 1);
    pix2 = rl2_create_pixel (RL2_SAMPLE_INT16, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL || pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create INT16 pixels for comparison\n");
	  return -117;
      }
    rl2_set_pixel_sample_int16 (no_data, -1);
    rl2_set_pixel_sample_int16 (pix2, -2);

    if (rl2_compare_pixels (no_data, pix2) != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (INT16)\n");
	  return -118;
      }
    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1);
    pix2 = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL || pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create UINT16 pixels for comparison\n");
	  return -119;
      }
    rl2_set_pixel_sample_uint16 (no_data, RL2_DATAGRID_BAND, 0);
    rl2_set_pixel_sample_uint16 (pix2, RL2_DATAGRID_BAND, 1);

    if (rl2_compare_pixels (no_data, pix2) != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (UINT16)\n");
	  return -120;
      }
    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_INT32, RL2_PIXEL_DATAGRID, 1);
    pix2 = rl2_create_pixel (RL2_SAMPLE_INT32, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL || pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create INT32 pixels for comparison\n");
	  return -121;
      }
    rl2_set_pixel_sample_int32 (no_data, -1);
    rl2_set_pixel_sample_int32 (pix2, -2);

    if (rl2_compare_pixels (no_data, pix2) != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (INT32)\n");
	  return -122;
      }
    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT32, RL2_PIXEL_DATAGRID, 1);
    pix2 = rl2_create_pixel (RL2_SAMPLE_UINT32, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL || pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create UINT32 pixels for comparison\n");
	  return -123;
      }
    rl2_set_pixel_sample_uint32 (no_data, 0);
    rl2_set_pixel_sample_uint32 (pix2, 1);

    if (rl2_compare_pixels (no_data, pix2) != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (UINT32)\n");
	  return -124;
      }
    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_FLOAT, RL2_PIXEL_DATAGRID, 1);
    pix2 = rl2_create_pixel (RL2_SAMPLE_FLOAT, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL || pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create FLOAT pixels for comparison\n");
	  return -125;
      }
    rl2_set_pixel_sample_float (no_data, 0.5);
    rl2_set_pixel_sample_float (pix2, 1.5);

    if (rl2_compare_pixels (no_data, pix2) != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (FLOAT)\n");
	  return -126;
      }
    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_DOUBLE, RL2_PIXEL_DATAGRID, 1);
    pix2 = rl2_create_pixel (RL2_SAMPLE_DOUBLE, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL || pix2 == NULL)
      {
	  fprintf (stderr, "Unable to create DOUBLE pixels for comparison\n");
	  return -127;
      }
    rl2_set_pixel_sample_double (no_data, 0.5);
    rl2_set_pixel_sample_double (pix2, 1.5);

    if (rl2_compare_pixels (no_data, pix2) != RL2_FALSE)
      {
	  fprintf (stderr, "Unexpected result: compare pixels (DOUBLE)\n");
	  return -128;
      }
    rl2_destroy_pixel (pix2);
    rl2_destroy_pixel (no_data);

    if (rl2_get_pixel_type (NULL, &sample_type, &pixel_type, &num_bands) ==
	RL2_OK)
      {
	  fprintf (stderr, "Unexpected result: pixel sample (NULL)\n");
	  return -129;
      }

    if (rl2_get_pixel_sample_1bit (NULL, &uint8) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: get pixel sample 1bit (NULL)\n");
	  return -132;
      }

    if (rl2_set_pixel_sample_1bit (NULL, 0) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: set pixel sample 1bit (NULL)\n");
	  return -133;
      }

    if (rl2_get_pixel_sample_2bit (NULL, &uint8) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: get pixel sample 2bit (NULL)\n");
	  return -134;
      }

    if (rl2_set_pixel_sample_2bit (NULL, 0) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: set pixel sample 2bit (NULL)\n");
	  return -135;
      }

    if (rl2_get_pixel_sample_4bit (NULL, &uint8) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: get pixel sample 4bit (NULL)\n");
	  return -136;
      }

    if (rl2_set_pixel_sample_4bit (NULL, 0) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: set pixel sample 4bit (NULL)\n");
	  return -137;
      }

    if (rl2_get_pixel_sample_uint8 (NULL, RL2_DATAGRID_BAND, &uint8) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample uint8 (NULL)\n");
	  return -138;
      }

    if (rl2_set_pixel_sample_uint8 (NULL, RL2_DATAGRID_BAND, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample uint8 (NULL)\n");
	  return -139;
      }

    if (rl2_get_pixel_sample_int8 (NULL, &int8) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: get pixel sample int8 (NULL)\n");
	  return -140;
      }

    if (rl2_set_pixel_sample_int8 (NULL, 0) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: set pixel sample int8 (NULL)\n");
	  return -141;
      }

    if (rl2_get_pixel_sample_uint16 (NULL, RL2_DATAGRID_BAND, &uint16) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample uint16 (NULL)\n");
	  return -142;
      }

    if (rl2_set_pixel_sample_uint16 (NULL, RL2_DATAGRID_BAND, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample uint16 (NULL)\n");
	  return -143;
      }

    if (rl2_get_pixel_sample_int16 (NULL, &int16) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample int16 (NULL)\n");
	  return -144;
      }

    if (rl2_set_pixel_sample_int16 (NULL, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample int16 (NULL)\n");
	  return -145;
      }

    if (rl2_get_pixel_sample_uint16 (NULL, RL2_DATAGRID_BAND, &uint16) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample uint16 (NULL)\n");
	  return -142;
      }

    if (rl2_set_pixel_sample_uint16 (NULL, RL2_DATAGRID_BAND, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample uint16 (NULL)\n");
	  return -143;
      }

    if (rl2_get_pixel_sample_int32 (NULL, &int32) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample int32 (NULL)\n");
	  return -144;
      }

    if (rl2_set_pixel_sample_int32 (NULL, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample int32 (NULL)\n");
	  return -145;
      }

    if (rl2_get_pixel_sample_uint32 (NULL, &uint32) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample uint32 (NULL)\n");
	  return -146;
      }

    if (rl2_set_pixel_sample_uint32 (NULL, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample uint32 (NULL)\n");
	  return -147;
      }

    if (rl2_get_pixel_sample_float (NULL, &flt32) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample FLOAT (NULL)\n");
	  return -148;
      }

    if (rl2_set_pixel_sample_float (NULL, 0.5) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample FLOAT (NULL)\n");
	  return -149;
      }

    if (rl2_get_pixel_sample_double (NULL, &flt64) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample DOUBLE (NULL)\n");
	  return -150;
      }

    if (rl2_set_pixel_sample_double (NULL, 0.5) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample DOUBLE (NULL)\n");
	  return -151;
      }

    if (rl2_is_pixel_transparent (NULL, &is_transparent) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: is pixel transparent (NULL)\n");
	  return -152;
      }

    if (rl2_is_pixel_opaque (NULL, &is_transparent) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: is pixel opaque (NULL)\n");
	  return -153;
      }

    if (rl2_set_pixel_transparent (NULL) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: set pixel transparent (NULL)\n");
	  return -154;
      }

    if (rl2_set_pixel_opaque (NULL) != RL2_ERROR)
      {
	  fprintf (stderr, "Unexpected result: set pixel opaque (NULL)\n");
	  return -155;
      }

    no_data = rl2_create_pixel (RL2_SAMPLE_DOUBLE, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create DOUBLE pixels for testing\n");
	  return -156;
      }

    if (rl2_get_pixel_sample_1bit (no_data, &uint8) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample 1bit (invalid)\n");
	  return -157;
      }

    if (rl2_set_pixel_sample_1bit (no_data, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample 1bit (invalid)\n");
	  return -158;
      }

    if (rl2_get_pixel_sample_2bit (no_data, &uint8) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample 2bit (invalid)\n");
	  return -159;
      }

    if (rl2_set_pixel_sample_2bit (no_data, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample 2bit (invalid)\n");
	  return -160;
      }

    if (rl2_get_pixel_sample_4bit (no_data, &uint8) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample 4bit (invalid)\n");
	  return -161;
      }

    if (rl2_set_pixel_sample_4bit (no_data, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample 4bit (invalid)\n");
	  return -162;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, RL2_DATAGRID_BAND, &uint8) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample uint8 (invalid)\n");
	  return -163;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, RL2_DATAGRID_BAND, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample uint8 (invalid)\n");
	  return -164;
      }

    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create UINT8 pixels for testing\n");
	  return -165;
      }

    if (rl2_get_pixel_sample_uint8 (no_data, -1, &uint8) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample uint8 (invalid)\n");
	  return -166;
      }

    if (rl2_set_pixel_sample_uint8 (no_data, -1, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample uint8 (invalid)\n");
	  return -167;
      }

    rl2_destroy_pixel (no_data);

    no_data = rl2_create_pixel (RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1);
    if (no_data == NULL)
      {
	  fprintf (stderr, "Unable to create UINT16 pixels for testing\n");
	  return -168;
      }

    if (rl2_get_pixel_sample_uint16 (no_data, -1, &uint16) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: get pixel sample uint16 (invalid)\n");
	  return -169;
      }

    if (rl2_set_pixel_sample_uint16 (no_data, -1, 0) != RL2_ERROR)
      {
	  fprintf (stderr,
		   "Unexpected result: set pixel sample uint16 (invalid)\n");
	  return -170;
      }

    rl2_destroy_pixel (no_data);

    return 0;
}
