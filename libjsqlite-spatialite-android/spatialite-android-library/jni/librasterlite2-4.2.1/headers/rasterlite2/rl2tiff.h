/*

 rl2tiff.h -- RasterLite2 common TIFF support

 version 0.1, 2013 November 20

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
 
Portions created by the Initial Developer are Copyright (C) 2013
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

/**
 \file rl2tiff.h

 TIFF support
 */

#ifndef _RL2TIFF_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RL2TIFF_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/** RasterLite2 TIFF constant: No Geo-referencing */
#define RL2_TIFF_NO_GEOREF			0xf1
/** RasterLite2 TIFF constant: Auto Geo-referencing GeoTiff priority */
#define RL2_TIFF_GEOTIFF			0xf2
/** RasterLite2 TIFF constant: Auto Geo-referencing Worldfile priority */
#define RL2_TIFF_WORLDFILE			0xf3

    RL2_DECLARE rl2TiffOriginPtr rl2_create_tiff_origin (const char *path,
							 int georef_priority,
							 int srid,
							 unsigned char
							 force_sample_type,
							 unsigned char
							 force_pixel_type,
							 unsigned char
							 force_num_bands);

    RL2_DECLARE rl2TiffOriginPtr rl2_create_geotiff_origin (const char *path,
							    int force_srid,
							    unsigned char
							    force_sample_type,
							    unsigned char
							    force_pixel_type,
							    unsigned char
							    force_num_bands);

    RL2_DECLARE rl2TiffOriginPtr rl2_create_tiff_worldfile_origin (const char
								   *path,
								   int srid,
								   unsigned char
								   force_sample_type,
								   unsigned char
								   force_pixel_type,
								   unsigned char
								   force_num_bands);

    RL2_DECLARE void rl2_destroy_tiff_origin (rl2TiffOriginPtr tiff);

    RL2_DECLARE int rl2_is_geotiff_origin (rl2TiffOriginPtr tiff, int *geotiff);

    RL2_DECLARE int rl2_is_tiff_worldfile_origin (rl2TiffOriginPtr
						  tiff, int *tiff_worldfile);

    RL2_DECLARE const char *rl2_get_tiff_origin_path (rl2TiffOriginPtr tiff);

    RL2_DECLARE const char
	*rl2_get_tiff_origin_worldfile_path (rl2TiffOriginPtr tiff);

    RL2_DECLARE int rl2_set_tiff_origin_not_referenced (rl2TiffOriginPtr tiff);

    RL2_DECLARE int
	rl2_get_tiff_origin_size (rl2TiffOriginPtr tiff, unsigned int *width,
				  unsigned int *height);

    RL2_DECLARE int
	rl2_get_tiff_origin_type (rl2TiffOriginPtr tiff,
				  unsigned char *sample_type,
				  unsigned char *pixel_type,
				  unsigned char *alias_pixel_type,
				  unsigned char *num_bands);

    RL2_DECLARE int
	rl2_get_tiff_origin_forced_type (rl2TiffOriginPtr tiff,
					 unsigned char *sample_type,
					 unsigned char *pixel_type,
					 unsigned char *num_bands);

    RL2_DECLARE int rl2_get_tiff_origin_compression (rl2TiffOriginPtr tiff,
						     unsigned char
						     *compression);

    RL2_DECLARE int rl2_get_tiff_origin_srid (rl2TiffOriginPtr tiff, int *srid);

    RL2_DECLARE int
	rl2_get_tiff_origin_extent (rl2TiffOriginPtr tiff, double *minX,
				    double *minY, double *maxX, double *maxY);

    RL2_DECLARE int
	rl2_get_tiff_origin_resolution (rl2TiffOriginPtr tiff,
					double *hResolution,
					double *vResolution);

    RL2_DECLARE int
	rl2_eval_tiff_origin_compatibility (rl2CoveragePtr cvg,
					    rl2TiffOriginPtr tiff,
					    int forced_srid, int verbose);

    RL2_DECLARE int rl2_is_tiled_tiff_origin (rl2TiffOriginPtr tiff,
					      int *is_tiled);

    RL2_DECLARE int
	rl2_get_tiff_origin_tile_size (rl2TiffOriginPtr tiff,
				       unsigned int *tile_width,
				       unsigned int *tile_height);

    RL2_DECLARE int
	rl2_get_tiff_origin_strip_size (rl2TiffOriginPtr tiff,
					unsigned int *strip_size);

    RL2_DECLARE rl2RasterPtr
	rl2_get_tile_from_tiff_origin (rl2CoveragePtr cvg,
				       rl2TiffOriginPtr tiff,
				       unsigned int startRow,
				       unsigned int startCol, int force_srid,
				       int verbose);

    RL2_DECLARE rl2TiffDestinationPtr
	rl2_create_tiff_destination (const char *path, unsigned int width,
				     unsigned int height,
				     unsigned char sample_type,
				     unsigned char pixel_type,
				     unsigned char num_bands, rl2PalettePtr plt,
				     unsigned char tiff_compression, int tiled,
				     unsigned int tile_size);

    RL2_DECLARE rl2TiffDestinationPtr
	rl2_create_geotiff_destination (const char *path, sqlite3 * handle,
					unsigned int width,
					unsigned int height,
					unsigned char sample_type,
					unsigned char pixel_type,
					unsigned char num_bands,
					rl2PalettePtr plt,
					unsigned char tiff_compression,
					int tiles, unsigned int tile_size,
					int srid, double minX, double minY,
					double maxX, double maxY,
					double hResolution, double vResolution,
					int with_worldfile);

    RL2_DECLARE rl2TiffDestinationPtr
	rl2_create_tiff_worldfile_destination (const char *path,
					       unsigned int width,
					       unsigned int height,
					       unsigned char sample_type,
					       unsigned char pixel_type,
					       unsigned char num_bands,
					       rl2PalettePtr plt,
					       unsigned char tiff_compression,
					       int tiles,
					       unsigned int tile_size, int srid,
					       double minX, double minY,
					       double maxX, double maxY,
					       double hResolution,
					       double vResolution);

    RL2_DECLARE void rl2_destroy_tiff_destination (rl2TiffDestinationPtr tiff);

    RL2_DECLARE int rl2_is_geotiff_destination (rl2TiffDestinationPtr tiff,
						int *geotiff);

    RL2_DECLARE int rl2_is_tiff_worldfile_destination (rl2TiffDestinationPtr
						       tiff,
						       int *tiff_worldfile);

    RL2_DECLARE const char *rl2_get_tiff_destination_path (rl2TiffDestinationPtr
							   tiff);

    RL2_DECLARE const char
	*rl2_get_tiff_destination_worldfile_path (rl2TiffDestinationPtr tiff);

    RL2_DECLARE int
	rl2_get_tiff_destination_size (rl2TiffDestinationPtr tiff,
				       unsigned int *width,
				       unsigned int *height);

    RL2_DECLARE int
	rl2_get_tiff_destination_type (rl2TiffDestinationPtr tiff,
				       unsigned char *sample_type,
				       unsigned char *pixel_type,
				       unsigned char *alias_pixel_type,
				       unsigned char *num_bands);

    RL2_DECLARE int rl2_get_tiff_destination_compression (rl2TiffDestinationPtr
							  tiff,
							  unsigned char
							  *compression);

    RL2_DECLARE int rl2_get_tiff_destination_srid (rl2TiffDestinationPtr tiff,
						   int *srid);

    RL2_DECLARE int
	rl2_get_tiff_destination_extent (rl2TiffDestinationPtr tiff,
					 double *minX, double *minY,
					 double *maxX, double *maxY);

    RL2_DECLARE int
	rl2_get_tiff_destination_resolution (rl2TiffDestinationPtr tiff,
					     double *hResolution,
					     double *vResolution);

    RL2_DECLARE int rl2_is_tiled_tiff_destination (rl2TiffDestinationPtr tiff,
						   int *is_tiled);

    RL2_DECLARE int
	rl2_get_tiff_destination_tile_size (rl2TiffDestinationPtr tiff,
					    unsigned int *tile_width,
					    unsigned int *tile_height);

    RL2_DECLARE int
	rl2_get_tiff_destination_strip_size (rl2TiffDestinationPtr tiff,
					     unsigned int *strip_size);

    RL2_DECLARE int
	rl2_write_tiff_tile (rl2TiffDestinationPtr tiff, rl2RasterPtr raster,
			     unsigned int startRow, unsigned int startCol);

    RL2_DECLARE int
	rl2_write_tiff_scanline (rl2TiffDestinationPtr tiff,
				 rl2RasterPtr raster, unsigned int row);

    RL2_DECLARE int rl2_write_tiff_worldfile (rl2TiffDestinationPtr tiff);

    RL2_DECLARE void
	rl2_prime_void_tile (void *pixels, unsigned int width,
			     unsigned int height, unsigned char sample_type,
			     unsigned char num_bands, rl2PixelPtr no_data);

    RL2_DECLARE void
	rl2_prime_void_tile_palette (void *pixels, unsigned int width,
				     unsigned int height, rl2PixelPtr no_data);

    RL2_DECLARE int
	rl2_raster_to_tiff_mono4 (rl2RasterPtr rst, unsigned char **tiff,
				  int *tiff_size);

    RL2_DECLARE char *rl2_build_tiff_xml_summary (rl2TiffOriginPtr tiff);

#ifdef __cplusplus
}
#endif

#endif				/* RL2TIFF_H */
