/*

 test_wms1.c -- RasterLite2 Test Case

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

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2wms.h"

static int
test_GetCapabilities_tuscany (rl2WmsCachePtr cache)
{
/* testing GetCapabilities and friends [Tuscany] */
    rl2WmsCatalogPtr catalog;
    rl2WmsLayerPtr layer;
    rl2WmsLayerPtr child;
    const char *url;
    int count;
    const char *str;
    int len;
    int int_res;
    double dbl_res;
    double minx;
    double miny;
    double maxx;
    double maxy;

/* WMS GetCapabilities (valid) */
    url =
	"http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsambamm&service=WMS&request=GetCapabilities";
    catalog = create_wms_catalog (cache, url, NULL);
    if (catalog == NULL)
      {
	  fprintf (stderr, "Unable to create a WMS Catalog\n");
	  return -2;
      }

/* testing Layer */
    count = get_wms_catalog_count (catalog);
    if (count != 1)
      {
	  fprintf (stderr, "CatalogCount: unexpected result %d\n", count);
	  return -3;
      }
    layer = get_wms_catalog_layer (catalog, 0);
    if (layer == NULL)
      {
	  fprintf (stderr, "GetWmsCatalogLayer: unexpected NULL\n");
	  return -4;
      }
    str = get_wms_layer_name (layer);
    if (strcmp (str, "rt_ambamm") != 0)
      {
	  fprintf (stderr, "GetWmsLayerName: unexpected result \"%s\"\n", str);
	  return -5;
      }
    str = get_wms_layer_title (layer);
    if (strcmp (str, "Geoscopio_wms AMBITI_AMMINISTRATIVI") != 0)
      {
	  fprintf (stderr, "GetWmsLayerTitle: unexpected result \"%s\"\n", str);
	  return -6;
      }
    str = get_wms_layer_abstract (layer);
    len = strlen (str);
    if (len != 435)
      {
	  fprintf (stderr, "GetWmsLayerAbstract: unexpected result %d\n", len);
	  return -7;
      }
    int_res = is_wms_layer_opaque (layer);
    if (int_res == 0)
      {
	  fprintf (stderr, "IsWmsLayerOpaque: unexpected result %d\n", int_res);
	  return -8;
      }
    int_res = is_wms_layer_queryable (layer);
    if (int_res == 0)
      {
	  fprintf (stderr, "IsWmsLayerQueryable: unexpected result %d\n",
		   int_res);
	  return -9;
      }
    dbl_res = get_wms_layer_min_scale_denominator (layer);
    if (dbl_res != 1.0)
      {
	  fprintf (stderr,
		   "GetWmsLayerMinScaleDenominator: unexpected result %1.6f\n",
		   dbl_res);
	  return -10;
      }
    dbl_res = get_wms_layer_max_scale_denominator (layer);
    if (dbl_res != 10000000.000000)
      {
	  fprintf (stderr,
		   "GetWmsLayerMaxScaleDenominator: unexpected result %1.6f\n",
		   dbl_res);
	  return -11;
      }
    if (get_wms_layer_geo_bbox (layer, &minx, &miny, &maxx, &maxy) != 1)
      {
	  fprintf (stderr, "GetWmsLayerGeoBBox: unexpected result FALSE\n");
	  return -12;
      }
    else
      {
	  if (maxx != 35.12930000)
	    {
		fprintf (stderr,
			 "GetWmsLayerGeoBBox (MaxX): unexpected result %1.8f\n",
			 maxx);
		return -13;
	    }
      }
    count = get_wms_layer_crs_count (layer);
    if (count != 11)
      {
	  fprintf (stderr, "GetWmsLayerCrsCount: unexpected result %d\n",
		   count);
	  return -14;
      }
    str = get_wms_layer_crs (layer, 0);
    if (strcmp (str, "EPSG:3003") != 0)
      {
	  fprintf (stderr, "GetWmsLayerCRS: unexpected result \"%s\"\n", str);
	  return -15;
      }
    if (get_wms_layer_bbox (layer, "EPSG:3003", &minx, &miny, &maxx, &maxy) !=
	1)
      {
	  fprintf (stderr, "GetWmsLayerBBox: unexpected result FALSE\n");
	  return -16;
      }
    else
      {
	  if (maxx != 3925120.00000000)
	    {
		fprintf (stderr,
			 "GetWmsLayerBBox (MaxX): unexpected result %1.8f\n",
			 maxx);
		return -17;
	    }
      }
    count = get_wms_layer_style_count (layer);
    if (count != 0)
      {
	  fprintf (stderr, "GetWmsLayerStyleCount: unexpected result %d\n",
		   count);
	  return -18;
      }
    str = get_wms_layer_style_name (layer, 0);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsLayerStyleName: unexpected result \"%s\"\n",
		   str);
	  return -19;
      }
    str = get_wms_layer_style_title (layer, 0);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsLayerStyleTitle: unexpected result \"%s\"\n",
		   str);
	  return -20;
      }
    str = get_wms_layer_style_abstract (layer, 0);
    if (str != NULL)
      {
	  fprintf (stderr,
		   "GetWmsLayerStyleAbstract: unexpected result \"%s\"\n", str);
	  return -21;
      }
    int_res = wms_layer_has_children (layer);
    if (int_res != 1)
      {
	  fprintf (stderr, "WmsLayerHasChildren: unexpected result %d\n",
		   int_res);
	  return -22;
      }

/* testing Child Layer */
    count = get_wms_layer_children_count (layer);
    if (count != 19)
      {
	  fprintf (stderr, "GetWmsLayerChildrenCount: unexpected result %d\n",
		   count);
	  return -23;
      }
    child = get_wms_child_layer (layer, 0);
    if (child == NULL)
      {
	  fprintf (stderr, "GetWmsChildLayer: unexpected NULL\n");
	  return -24;
      }
    str = get_wms_layer_name (child);
    if (strcmp (str, "rt_ambamm.idcomuni.rt.poly") != 0)
      {
	  fprintf (stderr,
		   "GetWmsLayerTitle (Child): unexpected result \"%s\"\n", str);
	  return -25;
      }
    int_res = is_wms_layer_opaque (child);
    if (int_res != 0)
      {
	  fprintf (stderr, "IsWmsLayerOpaque (Child): unexpected result %d\n",
		   int_res);
	  return -26;
      }
    int_res = is_wms_layer_queryable (child);
    if (int_res == 0)
      {
	  fprintf (stderr,
		   "IsWmsLayerQueryable (Child): unexpected result %d\n",
		   int_res);
	  return -27;
      }
    if (get_wms_layer_geo_bbox (child, &minx, &miny, &maxx, &maxy) != 1)
      {
	  fprintf (stderr,
		   "GetWmsLayerGeoBBox (Child): unexpected result FALSE\n");
	  return -28;
      }
    else
      {
	  if (maxx != 42.07900000)
	    {
		fprintf (stderr,
			 "GetWmsLayerGeoBBox (Child-MaxX): unexpected result %1.8f\n",
			 maxx);
		return -29;
	    }
      }
    count = get_wms_layer_crs_count (child);
    if (count != 12)
      {
	  fprintf (stderr,
		   "GetWmsLayerCrsCount (Child): unexpected result %d\n",
		   count);
	  return -30;
      }
    str = get_wms_layer_crs (child, 3);
    if (strcmp (str, "EPSG:6706") != 0)
      {
	  fprintf (stderr,
		   "GetWmsLayerCRS (Child) #1: unexpected result \"%s\"\n",
		   str);
	  return -31;
      }
    count = get_wms_layer_style_count (child);
    if (count != 16)
      {
	  fprintf (stderr,
		   "GetWmsLayerStyleCount (Child): unexpected result %d\n",
		   count);
	  return -32;
      }
    str = get_wms_layer_style_name (child, 5);
    if (strcmp (str, "contorno_spesso_avorio_con_etichette") != 0)
      {
	  fprintf (stderr,
		   "GetWmsLayerStyleName (Child): unexpected result \"%s\"\n",
		   str);
	  return -33;
      }

/* testing Service */
    str = get_wms_version (catalog);
    if (strcmp (str, "1.3.0") != 0)
      {
	  fprintf (stderr, "GetWmsVersion: unexpected result \"%s\"\n", str);
	  return -34;
      }
    str = get_wms_name (catalog);
    if (strcmp (str, "WMS") != 0)
      {
	  fprintf (stderr, "GetWmsName: unexpected result \"%s\"\n", str);
	  return -35;
      }
    str = get_wms_title (catalog);
    if (strcmp (str, "Geoscopio_wms AMBITI_AMMINISTRATIVI") != 0)
      {
	  fprintf (stderr, "GetWmsTitle: unexpected result \"%s\"\n", str);
	  return -36;
      }
    str = get_wms_abstract (catalog);
    len = strlen (str);
    if (len != 435)
      {
	  fprintf (stderr, "GetWmsAbstract: unexpected result %d\n", len);
	  return -37;
      }
    str = get_wms_url_GetMap_get (catalog);
    if (strcmp
	(str,
	 "http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsambamm&map_resolution=91&")
	!= 0)
      {
	  fprintf (stderr, "GetWmsGetMapGet: unexpected result \"%s\"\n", str);
	  return -38;
      }
    str = get_wms_url_GetMap_post (catalog);
    if (strcmp
	(str,
	 "http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsambamm&map_resolution=91&")
	!= 0)
      {
	  fprintf (stderr, "GetWmsGetMapPost: unexpected result \"%s\"\n", str);
	  return -39;
      }
    str = get_wms_url_GetFeatureInfo_get (catalog);
    if (strcmp
	(str,
	 "http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsambamm&map_resolution=91&")
	!= 0)
      {
	  fprintf (stderr,
		   "GetWmsGetFeatureInfoGet: unexpected result \"%s\"\n", str);
	  return -40;
      }
    str = get_wms_url_GetFeatureInfo_post (catalog);
    if (strcmp
	(str,
	 "http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsambamm&map_resolution=91&")
	!= 0)
      {
	  fprintf (stderr,
		   "GetWmsGetFeatureInfoPost: unexpected result \"%s\"\n", str);
	  return -41;
      }
    str = get_wms_gml_mime_type (catalog);
    if (strcmp (str, "application/vnd.ogc.gml") != 0)
      {
	  fprintf (stderr, "GetWmsGmlMimeType: unexpected result \"%s\"\n",
		   str);
	  return -42;
      }
    str = get_wms_xml_mime_type (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetWmsXmlMimeType: unexpected result \"%s\"\n",
		   str);
	  return -43;
      }
    str = get_wms_contact_person (catalog);
    if (strcmp (str, "Sistema Informativo Territoriale ed Ambientale") != 0)
      {
	  fprintf (stderr, "GetContactPerson: unexpected result \"%s\"\n", str);
	  return -44;
      }
    str = get_wms_contact_organization (catalog);
    if (strcmp
	(str,
	 "Regione Toscana - Direzione Generale Governo del territorio") != 0)
      {
	  fprintf (stderr, "GetContactOrganization: unexpected result \"%s\"\n",
		   str);
	  return -45;
      }
    str = get_wms_contact_position (catalog);
    if (strcmp (str, "custodian") != 0)
      {
	  fprintf (stderr, "GetContactPosition: unexpected result \"%s\"\n",
		   str);
	  return -46;
      }
    str = get_wms_contact_postal_address (catalog);
    if (strcmp (str, "Via di Novoli, 26") != 0)
      {
	  fprintf (stderr,
		   "GetContactPostalAddress: unexpected result \"%s\"\n", str);
	  return -47;
      }
    str = get_wms_contact_city (catalog);
    if (strcmp (str, "Firenze") != 0)
      {
	  fprintf (stderr, "GetContactCity: unexpected result \"%s\"\n", str);
	  return -48;
      }
    str = get_wms_contact_state_province (catalog);
    if (strcmp (str, "Regione Toscana") != 0)
      {
	  fprintf (stderr,
		   "GetContactStateProvince: unexpected result \"%s\"\n", str);
	  return -49;
      }
    str = get_wms_contact_post_code (catalog);
    if (strcmp (str, "50127") != 0)
      {
	  fprintf (stderr, "GetContactPostCode: unexpected result \"%s\"\n",
		   str);
	  return -50;
      }
    str = get_wms_contact_country (catalog);
    if (strcmp (str, "English") != 0)
      {
	  fprintf (stderr, "GetContactCountry: unexpected result \"%s\"\n",
		   str);
	  return -51;
      }
    str = get_wms_contact_voice_telephone (catalog);
    if (str != NULL)
      {
	  fprintf (stderr,
		   "GetContactVoiceTelephone: unexpected result \"%s\"\n", str);
	  return -52;
      }
    str = get_wms_contact_fax_telephone (catalog);
    if (str != NULL)
      {
	  fprintf (stderr, "GetContactFaxTelephone: unexpected result \"%s\"\n",
		   str);
	  return -53;
      }
    str = get_wms_contact_email_address (catalog);
    if (strcmp (str, "infrastruttura.geografica@regione.toscana.it") != 0)
      {
	  fprintf (stderr, "GetContactEMailAddress: unexpected result \"%s\"\n",
		   str);
	  return -54;
      }
    str = get_wms_fees (catalog);
    if (strcmp (str, "none") != 0)
      {
	  fprintf (stderr, "GetFees: unexpected result \"%s\"\n", str);
	  return -55;
      }
    str = get_wms_access_constraints (catalog);
    if (strcmp (str, "none") != 0)
      {
	  fprintf (stderr, "GetAccessConstraints: unexpected result \"%s\"\n",
		   str);
	  return -56;
      }
    int_res = get_wms_layer_limit (catalog);
    if (int_res != 20)
      {
	  fprintf (stderr, "GetLayerLimit: unexpected result %d\n", int_res);
	  return -57;
      }
    int_res = get_wms_max_width (catalog);
    if (int_res != 4096)
      {
	  fprintf (stderr, "GetMaxWidth: unexpected result %d\n", int_res);
	  return -58;
      }
    int_res = get_wms_max_height (catalog);
    if (int_res != 4096)
      {
	  fprintf (stderr, "GetMaxHeight: unexpected result %d\n", int_res);
	  return -59;
      }
    int_res = get_wms_format_count (catalog, 0);
    if (int_res != 8)
      {
	  fprintf (stderr, "GetWmsFormatCount: unexpected result %d\n",
		   int_res);
	  return -60;
      }
    str = get_wms_format (catalog, 2, 0);
    if (strcmp (str, "image/jpeg") != 0)
      {
	  fprintf (stderr, "GetWmsFormat: unexpected result \"%s\"\n", str);
	  return -61;
      }

/* destroying the WMS-Catalog */
    destroy_wms_catalog (catalog);

    return 0;
}

static int
test_GetFeatureInfo_gml ()
{
/* testing GetFeatureInfo and friends [GML] */
    rl2WmsFeatureCollectionPtr coll;
    rl2WmsFeatureMemberPtr ftr;
    const char *url;
    int count;
    const char *str;
    int ret;
    double mx;
    double my;
    double ratio;
    sqlite3 *sqlite;
    const unsigned char *blob;
    int blob_sz;
    gaiaGeomCollPtr geom;
    void *cache = spatialite_alloc_connection ();

    ret =
	sqlite3_open_v2 (":memory:", &sqlite,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory db: %s\n",
		   sqlite3_errmsg (sqlite));
	  sqlite3_close (sqlite);
	  return -200;
      }

    spatialite_init_ex (sqlite, cache, 0);

    url =
	"http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsambamm&map_resolution=91&";
    coll =
	do_wms_GetFeatureInfo_get (url, NULL, "1.3.0", "text/gml",
				   "rt_ambamm.idcomuni.rt.poly", "EPSG:3003", 0,
				   1590122.238032, 4727226.777078,
				   1618469.603808, 4752914.550184, 437, 396,
				   257, 219);
    if (coll == NULL)
      {
	  fprintf (stderr, "GetFeatureInfo (GML): unexpected NULL\n");
	  return -201;
      }
/* preparing Geometries */
    ratio = (1618469.603808 - 1590122.238032) / (double) 437;
    mx = 1590122.238032 + ((double) 396 * ratio);
    my = 4752914.550184 - ((double) 219 * ratio);
    wms_feature_collection_parse_geometries (coll, 3003, mx, my, sqlite);

/* testing Feature Member */
    count = get_wms_feature_members_count (coll);
    if (count != 1)
      {
	  fprintf (stderr,
		   "GetFeatureMembersCount (GML): unexpected result %d\n",
		   count);
	  return -202;
      }
    ftr = get_wms_feature_member (coll, 0);
    if (ftr == NULL)
      {
	  fprintf (stderr, "GetFeatureMember (GML): unexpected NULL\n");
	  return -203;
      }
    count = get_wms_feature_attributes_count (ftr);
    if (count != 8)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeCount (GML): unexpected result %d\n",
		   count);
	  return -204;
      }
    str = get_wms_feature_attribute_name (ftr, 0);
    if (strcmp (str, "geometryProperty") != 0)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeName (GML-Geom): unexpected result \"%s\"\n",
		   str);
	  return -205;
      }
    str = get_wms_feature_attribute_value (ftr, 0);
    count = strlen (str);
    if (count != 364684)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (GML-Geom): unexpected lenght %d\n",
		   count);
	  return -206;
      }
    if (get_wms_feature_attribute_blob_geometry (ftr, 0, &blob, &blob_sz) !=
	RL2_OK)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (GML-Geom): unexpected failure\n");
	  return -207;
      }
    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
    if (geom == NULL)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (GML-Geom): unexpected failure\n");
	  return -207;
      }
    else if (geom->Srid != 3003)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (GML-Geom): unexpected SRID %d\n",
		   geom->Srid);
	  return -208;
      }
    gaiaFreeGeomColl (geom);
    str = get_wms_feature_attribute_name (ftr, 4);
    if (strcmp (str, "CODCOM") != 0)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeName (GML): unexpected result \"%s\"\n",
		   str);
	  return -209;
      }
    str = get_wms_feature_attribute_value (ftr, 4);
    if (strcmp (str, "049014") != 0)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (GML): unexpected result \"%s\"\n",
		   str);
	  return -210;
      }
    if (get_wms_feature_attribute_blob_geometry (ftr, 4, &blob, &blob_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (GML-Geom): unexpected result\n");
	  return -211;
      }

/* destroying the Feature Collection */
    destroy_wms_feature_collection (coll);

/* closing SQLite/SpatiaLite */
    ret = sqlite3_close (sqlite);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_close() error: %s\n",
		   sqlite3_errmsg (sqlite));
	  return -212;
      }
    spatialite_cleanup_ex (cache);

    return 0;
}

static int
test_GetFeatureInfo_xml ()
{
/* testing GetFeatureInfo and friends [XML] */
    rl2WmsFeatureCollectionPtr coll;
    rl2WmsFeatureMemberPtr ftr;
    const char *url;
    int count;
    const char *str;
    int ret;
    double mx;
    double my;
    double ratio;
    sqlite3 *sqlite;
    const unsigned char *blob;
    int blob_sz;
    void *cache = spatialite_alloc_connection ();

    ret =
	sqlite3_open_v2 (":memory:", &sqlite,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory db: %s\n",
		   sqlite3_errmsg (sqlite));
	  sqlite3_close (sqlite);
	  return -300;
      }

    spatialite_init_ex (sqlite, cache, 0);

    url = "http://servizigis.regione.emilia-romagna.it/wms/dbtr2008?";
    coll =
	do_wms_GetFeatureInfo_get (url, NULL, "1.3.0", "text/xml",
				   "PRV_Provincia", "CRS:84", 0, 9.018668,
				   42.615796, 13.015085, 46.237264, 437, 396,
				   254, 184);
    if (coll == NULL)
      {
	  fprintf (stderr, "GetFeatureInfo (XML): unexpected NULL\n");
	  return -301;
      }
/* preparing Geometries */
    ratio = (13.015085 - 9.018668) / (double) 437;
    mx = 9.018668 + ((double) 254 * ratio);
    my = 46.237264 - ((double) 182 * ratio);
    wms_feature_collection_parse_geometries (coll, 4326, mx, my, sqlite);

/* testing Feature Member */
    count = get_wms_feature_members_count (coll);
    if (count != 1)
      {
	  fprintf (stderr,
		   "GetFeatureMembersCount (XML): unexpected result %d\n",
		   count);
	  return -302;
      }
    ftr = get_wms_feature_member (coll, 0);
    if (ftr == NULL)
      {
	  fprintf (stderr, "GetFeatureMember (XML): unexpected NULL\n");
	  return -303;
      }
    count = get_wms_feature_attributes_count (ftr);
    if (count != 26)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeCount (XML): unexpected result %d\n",
		   count);
	  return -304;
      }
    str = get_wms_feature_attribute_name (ftr, 0);
    if (strcmp (str, "Objectid") != 0)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeName (XML-Geom): unexpected result \"%s\"\n",
		   str);
	  return -305;
      }
    str = get_wms_feature_attribute_value (ftr, 0);
    if (strcmp (str, "5") != 0)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (XML): unexpected result \"%s\"\n",
		   str);
	  return -306;
      }
    if (get_wms_feature_attribute_blob_geometry (ftr, 0, &blob, &blob_sz) !=
	RL2_ERROR)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (XML-Geom): unexpected value\n");
	  return -307;
      }
    str = get_wms_feature_attribute_name (ftr, 2);
    if (strcmp (str, "Nm_prv") != 0)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeName (XML): unexpected result \"%s\"\n",
		   str);
	  return -309;
      }
    str = get_wms_feature_attribute_value (ftr, 2);
    if (strcmp (str, "Bologna") != 0)
      {
	  fprintf (stderr,
		   "GetFeatureAttributeValue (XML): unexpected result \"%s\"\n",
		   str);
	  return -310;
      }

/* destroying the Feature Collection */
    destroy_wms_feature_collection (coll);

/* closing SQLite/SpatiaLite */
    ret = sqlite3_close (sqlite);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_close() error: %s\n",
		   sqlite3_errmsg (sqlite));
	  return -311;
      }
    spatialite_cleanup_ex (cache);

    return 0;
}

int
main (int argc, char *argv[])
{
    int ret;
    int val;
    double dblval;
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

/* testing the cache */
    set_wms_cache_max_size (cache, 5000000);
    val = get_wms_cache_max_size (cache);
    if (val != 5000000)
      {
	  fprintf (stderr, "GetWmsCacheMaxSize: unexpected result %d\n", val);
	  return -80;
      }
    val = get_wms_cache_items_count (cache);
    if (val != 0)
      {
	  fprintf (stderr, "GetWmsCacheItemsCount: unexpected result %d\n",
		   val);
	  return -81;
      }
    val = get_wms_cache_current_size (cache);
    if (val != 0)
      {
	  fprintf (stderr, "GetWmsCacheCurrentSize: unexpected result %d\n",
		   val);
	  return -82;
      }
    val = get_wms_cache_hit_count (cache);
    if (val != 0)
      {
	  fprintf (stderr, "GetWmsCacheHitCount: unexpected result %d\n", val);
	  return -83;
      }
    val = get_wms_cache_miss_count (cache);
    if (val != 0)
      {
	  fprintf (stderr, "GetWmsCacheMissCount: unexpected result %d\n", val);
	  return -84;
      }
    val = get_wms_cache_flushed_count (cache);
    if (val != 0)
      {
	  fprintf (stderr, "GetWmsCacheFlushedCount: unexpected result %d\n",
		   val);
	  return -85;
      }
    dblval = get_wms_total_download_size (cache);
    if (dblval != 0)
      {
	  fprintf (stderr, "GetWmsTotalDownloadSize: unexpected result %1.2f\n",
		   dblval);
	  return -86;
      }
    reset_wms_cache (cache);

/* GetCapabilities - Pass I [not cached] */
    ret = test_GetCapabilities_tuscany (cache);
    if (ret != 0)
	return ret;
/* GetMap - Pass I [not cached] */
    url =
	"http://www502.regione.toscana.it/wmsraster/com.rt.wms.RTmap/wms?map=wmsambamm&map_resolution=91&";
    rgba =
	do_wms_GetMap_get (cache, url, NULL, "1.3.0",
			   "rt_ambamm.idcomuni.rt.poly", "EPSG:3003", 0,
			   1583097.365776, 4704017.773106, 1611444.731553,
			   4729705.546213, 437, 396, "contorno_con_etichette",
			   "image/png", 0, 1);
    if (rgba != NULL)
      {
	  fprintf (stderr, "GetMap (not cached): unexpected result\n");
	  return -87;
      }
    rgba = do_wms_GetMap_get (cache, url, NULL, "1.3.0",
			      "rt_ambamm.idcomuni.rt.poly", "EPSG:3003", 0,
			      1583097.365776, 4704017.773106, 1611444.731553,
			      4729705.546213, 437, 396,
			      "contorno_con_etichette", "image/png", 0, 0);
    if (rgba == NULL)
      {
	  fprintf (stderr, "GetMap (not cached): unexpected NULL\n");
	  return -88;
      }
    free (rgba);

/* GetCapabilities - Pass II [already cached] */
    ret = test_GetCapabilities_tuscany (cache);
    if (ret != 0)
	return ret;
/* GetMap - Pass II [already cached] */
    rgba = do_wms_GetMap_get (cache, url, NULL, "1.3.0",
			      "rt_ambamm.idcomuni.rt.poly", "EPSG:3003", 0,
			      1583097.365776, 4704017.773106, 1611444.731553,
			      4729705.546213, 437, 396,
			      "contorno_con_etichette", "image/png", 0, 1);
    if (rgba == NULL)
      {
	  fprintf (stderr, "GetMap (cached): unexpected result\n");
	  return -89;
      }
    free (rgba);
    rgba = do_wms_GetMap_get (cache, url, NULL, "1.3.0",
			      "rt_ambamm.idcomuni.rt.poly", "EPSG:3003", 0,
			      1583097.365776, 4704017.773106, 1611444.731553,
			      4729705.546213, 437, 396,
			      "contorno_con_etichette", "image/png", 0, 0);
    if (rgba == NULL)
      {
	  fprintf (stderr, "GetMap (cached): unexpected NULL\n");
	  return -90;
      }
    free (rgba);

/* testing GetFeatureInfo - GML */
    ret = test_GetFeatureInfo_gml ();
    if (ret != 0)
	return ret;

/* testing GetFeatureInfo - XML */
    ret = test_GetFeatureInfo_xml ();
    if (ret != 0)
	return ret;

/* re-testing the cache */
    val = get_wms_cache_items_count (cache);
    if (val != 1)
      {
	  fprintf (stderr, "GetWmsCacheItemsCount: unexpected result %d\n",
		   val);
	  return -91;
      }
    val = get_wms_cache_current_size (cache);
    if (val != 3286)
      {
	  fprintf (stderr, "GetWmsCacheCurrentSize: unexpected result %d\n",
		   val);
	  return -92;
      }
    val = get_wms_cache_hit_count (cache);
    if (val != 2)
      {
	  fprintf (stderr, "GetWmsCacheHitCount: unexpected result %d\n", val);
	  return -93;
      }
    val = get_wms_cache_miss_count (cache);
    if (val != 0)
      {
	  fprintf (stderr, "GetWmsCacheMissCount: unexpected result %d\n", val);
	  return -94;
      }
    val = get_wms_cache_flushed_count (cache);
    if (val != 0)
      {
	  fprintf (stderr, "GetWmsCacheFlushedCount: unexpected result %d\n",
		   val);
	  return -95;
      }
    dblval = get_wms_total_download_size (cache);
    if (dblval != 113055.00)
      {
	  fprintf (stderr, "GetWmsTotalDownloadSize: unexpected result %1.2f\n",
		   dblval);
	  return -96;
      }

/* destroying the WMS-Cache */
    destroy_wms_cache (cache);

    return 0;
}
