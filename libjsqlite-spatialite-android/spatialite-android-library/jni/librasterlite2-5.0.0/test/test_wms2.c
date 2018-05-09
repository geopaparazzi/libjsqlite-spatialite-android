/*

 test_wms2.c -- RasterLite2 Test Case

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
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2wms.h"

static int
test_GetCapabilities (rl2WmsCachePtr cache)
{
/* testing GetCapabilities and friends [TileService] */
    rl2WmsCatalogPtr catalog;
    rl2WmsTiledLayerPtr layer;
    rl2WmsTiledLayerPtr child;
    rl2WmsTilePatternPtr pattern;
    rl2WmsTilePatternPtr clone;
    const char *url;
    char *xurl;
    const char *str;
    int len;
    int int_res;
    double dbl_res;
    double minx;
    double miny;
    double maxx;
    double maxy;
    int width;
    int height;

/* WMS GetCapabilities (valid) */
    url =
	"http://wms.jpl.nasa.gov/wms.cgi?SERVICE=WMS&REQUEST=GetCapabilities&VERSION=1.1.1";
    catalog = create_wms_catalog (cache, url, NULL);
    if (catalog == NULL)
      {
	  fprintf (stderr, "Unable to create a WMS Catalog\n");
	  return -2;
      }

/* testing Service */
    str = get_wms_version (catalog);
    if (strcmp (str, "1.1.1") != 0)
      {
	  fprintf (stderr, "GetWmsVersion: unexpected result \"%s\"\n", str);
	  return -3;
      }
    str = get_wms_name (catalog);
    if (strcmp (str, "OGC:WMS") != 0)
      {
	  fprintf (stderr, "GetWmsName: unexpected result \"%s\"\n", str);
	  return -4;
      }
    str = get_wms_title (catalog);
    if (strcmp (str, "JPL Global Imagery Service") != 0)
      {
	  fprintf (stderr, "GetWmsTitle: unexpected result \"%s\"\n", str);
	  return -5;
      }
    str = get_wms_abstract (catalog);
    if (strcmp
	(str,
	 "WMS Server maintained by JPL, worldwide satellite imagery.") != 0)
      {
	  fprintf (stderr, "GetWmsAbstract: unexpected result \"%s\"\n", str);
	  return -6;
      }
    str = get_wms_url_GetMap_get (catalog);
    if (strcmp (str, "http://wms.jpl.nasa.gov/wms.cgi?") != 0)
      {
	  fprintf (stderr, "GetWmsGetMapGet: unexpected result %s\n", str);
	  return -7;
      }
    str = get_wms_url_GetMap_post (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsGetMapPost: unexpected result %s\n", str);
	  return -8;
      }
    str = get_wms_url_GetFeatureInfo_get (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsGetFeatureInfoGet: unexpected result %s\n",
		   str);
	  return -9;
      }
    str = get_wms_url_GetFeatureInfo_post (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsGetFeatureInfoPost: unexpected result %s\n",
		   str);
	  return -10;
      }
    str = get_wms_url_GetTileService_get (catalog);
    if (strcmp (str, "http://wms.jpl.nasa.gov/wms.cgi?") != 0)
      {
	  fprintf (stderr, "GetWmsGetMapGet: unexpected result %s\n", str);
	  return -11;
      }
    str = get_wms_url_GetTileService_post (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsGetMapPost: unexpected result %s\n", str);
	  return -12;
      }
    str = get_wms_gml_mime_type (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsGmlMimeType: unexpected result %s\n", str);
	  return -13;
      }
    str = get_wms_xml_mime_type (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsXmlMimeType: unexpected result %s\n", str);
	  return -14;
      }
    str = get_wms_contact_person (catalog);
    if (strcmp (str, "Lucian Plesea") != 0)
      {
	  fprintf (stderr, "GetContactPerson: unexpected result %s\n", str);
	  return -15;
      }
    str = get_wms_contact_organization (catalog);
    if (strcmp (str, "JPL") != 0)
      {
	  fprintf (stderr, "GetContactOrganization: unexpected result %s\n",
		   str);
	  return -16;
      }
    str = get_wms_contact_position (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactPosition: unexpected result %s\n", str);
	  return -17;
      }
    str = get_wms_contact_postal_address (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactPostalAddress: unexpected result %s\n",
		   str);
	  return -18;
      }
    str = get_wms_contact_city (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactCity: unexpected result %s\n", str);
	  return -19;
      }
    str = get_wms_contact_state_province (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactStateProvince: unexpected result %s\n",
		   str);
	  return -20;
      }
    str = get_wms_contact_post_code (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactPostCode: unexpected result %s\n", str);
	  return -21;
      }
    str = get_wms_contact_country (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactCountry: unexpected result %s\n", str);
	  return -22;
      }
    str = get_wms_contact_voice_telephone (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactVoiceTelephone: unexpected result %s\n",
		   str);
	  return -23;
      }
    str = get_wms_contact_fax_telephone (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactFaxTelephone: unexpected result %s\n",
		   str);
	  return -24;
      }
    str = get_wms_contact_email_address (catalog);
    if (strcmp (str, "lucian.plesea@jpl.nasa.gov") != 0)
      {
	  fprintf (stderr, "GetContactEMailAddress: unexpected result %s\n",
		   str);
	  return -25;
      }
    str = get_wms_fees (catalog);
    if (strcmp (str, "none") != 0)
      {
	  fprintf (stderr, "GetFees: unexpected result %s\n", str);
	  return -26;
      }
    str = get_wms_access_constraints (catalog);
    if (strcmp (str, "Server is load limited") != 0)
      {
	  fprintf (stderr, "GetAccessConstraints: unexpected result %s\n", str);
	  return -27;
      }
    int_res = get_wms_layer_limit (catalog);
    if (int_res != -1)
      {
	  fprintf (stderr, "GetLayerLimit: unexpected result %d\n", int_res);
	  return -28;
      }
    int_res = get_wms_max_width (catalog);
    if (int_res != -1)
      {
	  fprintf (stderr, "GetMaxWidth: unexpected result %d\n", int_res);
	  return -29;
      }
    int_res = get_wms_max_height (catalog);
    if (int_res != -1)
      {
	  fprintf (stderr, "GetMaxHeight: unexpected result %d\n", int_res);
	  return -30;
      }

/* testing TileService */
    int_res = is_wms_tile_service (catalog);
    if (int_res != 1)
      {
	  fprintf (stderr, "IsTileService: unexpected result %d\n", int_res);
	  return -31;
      }
    str = get_wms_tile_service_name (catalog);
    if (strcmp (str, "OnEarth:WMS:Tile") != 0)
      {
	  fprintf (stderr, "GetTileServiceName: unexpected result \"%s\"\n",
		   str);
	  return -32;
      }
    str = get_wms_tile_service_title (catalog);
    if (strcmp (str, "WMS Tile service") != 0)
      {
	  fprintf (stderr, "GetTileServiceTitle: unexpected result \"%s\"\n",
		   str);
	  return -33;
      }
    str = get_wms_tile_service_abstract (catalog);
    len = strlen (str);
    if (len != 822)
      {
	  fprintf (stderr, "GetTileServiceAbstract: unexpected length %d\n",
		   len);
	  return -34;
      }
    int_res = get_wms_tile_service_count (catalog);
    if (int_res != 17)
      {
	  fprintf (stderr, "GetTileServiceCount: unexpected result %d\n",
		   int_res);
	  return -35;
      }

/* testing TileService layer */
    layer = get_wms_catalog_tiled_layer (catalog, 0);
    if (layer == NULL)
      {
	  fprintf (stderr, "GetTiledLayer: unexpected NULL\n");
	  return -36;
      }
    str = get_wms_tiled_layer_name (layer);
    if (strcmp (str, "Global Mosaic, pan sharpened visual") != 0)
      {
	  fprintf (stderr, "GetWmsTiledLayerName: unexpected result \"%s\"\n",
		   str);
	  return -37;
      }
    str = get_wms_tiled_layer_title (layer);
    if (strcmp (str, "Global Mosaic, pan sharpened visual") != 0)
      {
	  fprintf (stderr, "GetWmsTiledLayerTitle: unexpected result \"%s\"\n",
		   str);
	  return -38;
      }
    str = get_wms_tiled_layer_abstract (layer);
    len = strlen (str);
    if (len != 498)
      {
	  fprintf (stderr, "GetWmsTiledLayerAbstract: unexpected result %d\n",
		   len);
	  return -39;
      }
    str = get_wms_tiled_layer_pad (layer);
    if (strcmp (str, "0") != 0)
      {
	  fprintf (stderr, "GetWmsTiledLayerPad: unexpected result \"%s\"\n",
		   str);
	  return -40;
      }
    str = get_wms_tiled_layer_bands (layer);
    if (strcmp (str, "3") != 0)
      {
	  fprintf (stderr, "GetWmsTiledLayerBands: unexpected result \"%s\"\n",
		   str);
	  return -38;
      }
    str = get_wms_tiled_layer_data_type (layer);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsTiledLayerDataType: unexpected result %s\n",
		   str);
	  return -39;
      }
    if (get_wms_tiled_layer_bbox (layer, &minx, &miny, &maxx, &maxy) != 1)
      {
	  fprintf (stderr, "GetWmsTiledLayerBBox: unexpected result FALSE\n");
	  return -40;
      }
    else
      {
	  if (maxx != 180.0)
	    {
		fprintf (stderr,
			 "GetWmsTiledLayerBBox (MaxX): unexpected result %1.8f\n",
			 maxx);
		return -41;
	    }
      }
    if (get_wms_tiled_layer_tile_size (layer, &width, &height) != 1)
      {
	  fprintf (stderr,
		   "GetWmsTiledLayerTileSize: unexpected result FALSE\n");
	  return -42;
      }
    else
      {
	  if (width != 512)
	    {
		fprintf (stderr,
			 "GetWmsTiledLayerTileSize (Width): unexpected result %d\n",
			 width);
		return -43;
	    }
      }
    str = get_wms_tiled_layer_crs (layer);
    if (strcmp (str, "EPSG:4326") != 0)
      {
	  fprintf (stderr, "GetWmsTiledLayerCrs: unexpected result \"%s\"\n",
		   str);
	  return -44;
      }
    str = get_wms_tiled_layer_format (layer);
    if (strcmp (str, "image/jpeg") != 0)
      {
	  fprintf (stderr, "GetWmsTiledLayerFormat: unexpected result \"%s\"\n",
		   str);
	  return -45;
      }
    str = get_wms_tiled_layer_style (layer);
    if (strcmp (str, "visual") != 0)
      {
	  fprintf (stderr, "GetWmsTiledLayerStyle: unexpected result %s\n",
		   str);
	  return -46;
      }
    int_res = get_wms_tile_pattern_count (layer);
    if (int_res != 13)
      {
	  fprintf (stderr, "GetTilePatternCount: unexpected result %d\n",
		   int_res);
	  return -47;
      }
    pattern = get_wms_tile_pattern_handle (layer, 3);
    if (pattern == NULL)
      {
	  fprintf (stderr, "GetTilePatternHandle: unexpected NULL\n");
	  return -48;
      }
    str = get_wms_tile_pattern_srs (layer, 0);
    if (strcmp (str, "EPSG:4326") != 0)
      {
	  fprintf (stderr, "GetWmsTilePatternCrs: unexpected result \"%s\"\n",
		   str);
	  return -49;
      }
    xurl = get_wms_tile_pattern_sample_url (pattern);
    if (strcmp
	(xurl,
	 "request=GetMap&layers=global_mosaic&srs=EPSG:4326&format=image/jpeg&styles=visual&width=512&height=512&bbox=-180,58,-148,90")
	!= 0)
      {
	  fprintf (stderr,
		   "GetWmsTilePatternSampleURL: unexpected result \"%s\"\n",
		   str);
	  return -50;
      }
    free (xurl);
    int_res = get_wms_tile_pattern_tile_width (layer, 0);
    if (int_res != 512)
      {
	  fprintf (stderr, "GetTilePatternTileWidth: unexpected result %d\n",
		   int_res);
	  return -51;
      }
    int_res = get_wms_tile_pattern_tile_height (layer, 0);
    if (int_res != 512)
      {
	  fprintf (stderr, "GetTilePatternTileHeight: unexpected result %d\n",
		   int_res);
	  return -52;
      }
    dbl_res = get_wms_tile_pattern_base_x (layer, 0);
    if (dbl_res != -180.0)
      {
	  fprintf (stderr, "GetTilePatternTileBaseX: unexpected result %1.8f\n",
		   dbl_res);
	  return -53;
      }
    dbl_res = get_wms_tile_pattern_base_y (layer, 0);
    if (dbl_res != 90.0)
      {
	  fprintf (stderr, "GetTilePatternTileBaseY: unexpected result %1.8f\n",
		   dbl_res);
	  return -54;
      }
    dbl_res = get_wms_tile_pattern_extent_x (layer, 0);
    if (dbl_res != 256.0)
      {
	  fprintf (stderr,
		   "GetTilePatternTileExtentX: unexpected result %1.8f\n",
		   dbl_res);
	  return -55;
      }
    dbl_res = get_wms_tile_pattern_extent_y (layer, 0);
    if (dbl_res != 256.0)
      {
	  fprintf (stderr,
		   "GetTilePatternTileExtentY: unexpected result %1.8f\n",
		   dbl_res);
	  return -56;
      }
    clone = clone_wms_tile_pattern (pattern);
    if (clone == NULL)
      {
	  fprintf (stderr, "CloneTilePattern: unexpected NULL\n");
	  return -57;
      }
    destroy_wms_tile_pattern (clone);
    xurl =
	get_wms_tile_pattern_request_url (pattern,
					  "http://wms.jpl.nasa.gov/wms.cgi?",
					  -180.0, 90.0);
    if (strcmp
	(xurl,
	 "http://wms.jpl.nasa.gov/wms.cgi?request=GetMap&layers=global_mosaic&srs=EPSG:4326&format=image/jpeg&styles=visual&width=512&height=512&bbox=-180.000000,90.000000,-148.000000,122.000000")
	!= 0)
      {
	  fprintf (stderr,
		   "GetWmsTilePatternRequestURL: unexpected result %s\n", xurl);
	  return -58;
      }
    sqlite3_free (xurl);
    int_res = wms_tiled_layer_has_children (layer);
    if (int_res != 0)
      {
	  fprintf (stderr, "GetTiledLayerHasChildren: unexpected result %d\n",
		   int_res);
	  return -59;
      }

/* testing TileService Child Layer */
    layer = get_wms_catalog_tiled_layer (catalog, 12);
    if (layer == NULL)
      {
	  fprintf (stderr, "GetTiledLayer: unexpected NULL\n");
	  return -60;
      }
    int_res = wms_tiled_layer_has_children (layer);
    if (int_res == 0)
      {
	  fprintf (stderr, "GetTiledLayerHasChildren: unexpected result %d\n",
		   int_res);
	  return -61;
      }
    int_res = get_wms_tiled_layer_children_count (layer);
    if (int_res != 4)
      {
	  fprintf (stderr, "GetTiledLayerChildrenCount: unexpected result %d\n",
		   int_res);
	  return -62;
      }
    child = get_wms_child_tiled_layer (layer, 2);
    if (child == NULL)
      {
	  fprintf (stderr, "GetChildTiledLayer: unexpected NULL\n");
	  return -63;
      }
    str = get_wms_tiled_layer_title (child);
    if (strcmp (str, "The full resolution BMNG in jpeg tiled format") != 0)
      {
	  fprintf (stderr,
		   "GetWmsTiledLayerTitle Child: unexpected result \"%s\"\n",
		   str);
	  return -64;
      }
    str = get_wms_tiled_layer_abstract (child);
    if (strcmp
	(str,
	 "There are twelve styles of this dataset, each one corresponding to a given month of the year, with 960x960 pixel tiles")
	!= 0)
      {
	  fprintf (stderr,
		   "GetWmsTiledLayerAbstract Child: unexpected result \"%s\"\n",
		   str);
	  return -65;
      }
    str = get_wms_tiled_layer_pad (child);
    if (str != NULL)
      {
	  fprintf (stderr,
		   "GetWmsTiledLayerPad Child: unexpected result \"%s\"\n",
		   str);
	  return -66;
      }
    str = get_wms_tiled_layer_bands (child);
    if (str != NULL)
      {
	  fprintf (stderr,
		   "GetWmsTiledLayerBands Child: unexpected result \"%s\"\n",
		   str);
	  return -67;
      }
    str = get_wms_tiled_layer_data_type (child);
    if (str != NULL)
      {
	  fprintf (stderr,
		   "GetWmsTiledLayerDataType Child: unexpected result %s\n",
		   str);
	  return -68;
      }
    if (get_wms_tiled_layer_bbox (child, &minx, &miny, &maxx, &maxy) != 1)
      {
	  fprintf (stderr,
		   "GetWmsTiledLayerBBox Child: unexpected result FALSE\n");
	  return -69;
      }
    else
      {
	  if (maxx != DBL_MAX)
	    {
		fprintf (stderr,
			 "GetWmsTiledLayerBBox (Child MaxX): unexpected result %1.8f\n",
			 maxx);
		return -70;
	    }
      }
    if (get_wms_tiled_layer_tile_size (child, &width, &height) != 0)
      {
	  fprintf (stderr,
		   "GetWmsTiledLayerTileSize Child: unexpected result TRUE\n");
	  return -71;
      }

/* destroying the WMS-Catalog */
    destroy_wms_catalog (catalog);

    return 0;
}

int
main (int argc, char *argv[])
{
    int ret;
    const char *url;
    unsigned char *rgba;
    rl2WmsCachePtr cache;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    if (getenv ("ENABLE_RL2_WEB_TESTS") == NULL)
      {
	  fprintf (stderr, "this testcase has been skipped !!!\n\n"
		   "you can enable all testcases requiring an Internet connection\n"
		   "by setting the environment variable \"ENABLE_RL2_WEB_TESTS=1\"\n");
	  return 0;
      }

/* creating a WMS-Cache */
    cache = create_wms_cache ();
    if (cache == NULL)
      {
	  fprintf (stderr, "Unable to create a WMS Cache\n");
	  return -1;
      }

/* GetCapabilities - TileService */
    ret = test_GetCapabilities (cache);
    if (ret != 0)
	return ret;

/* GetCapabilities - Pass I [not cached] */
    ret = test_GetCapabilities (cache);
    if (ret != 0)
	return ret;
/* GetMap - Pass I [not cached] */
    url =
	"http://wms.jpl.nasa.gov/wms.cgi?request=GetMap&layers=global_mosaic&srs=EPSG:4326&format=image/jpeg&styles=visual&width=512&height=512&bbox=10.312500,42.812500,10.375000,42.875000";
    rgba = do_wms_GetMap_TileService_get (cache, url, NULL, 512, 512, 1);
    if (rgba != NULL)
      {
	  fprintf (stderr,
		   "GetMap TileService (not cached): unexpected result\n");
	  return -87;
      }
    rgba = do_wms_GetMap_TileService_get (cache, url, NULL, 512, 512, 0);
    if (rgba == NULL)
      {
	  fprintf (stderr,
		   "GetMap TileService (not cached): unexpected NULL\n");
	  return -88;
      }
    free (rgba);

/* GetCapabilities - Pass II [already cached] */
    ret = test_GetCapabilities (cache);
    if (ret != 0)
	return ret;

/* destroying the WMS-Cache */
    destroy_wms_cache (cache);

    return 0;
}
