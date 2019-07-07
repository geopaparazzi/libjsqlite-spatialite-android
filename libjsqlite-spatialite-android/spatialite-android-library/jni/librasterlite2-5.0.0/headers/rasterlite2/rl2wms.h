/*
 rl2wms.h -- RasterLite2 common WMS support
  
 version 2.0, 2013 July 28

 Author: Sandro Furieri a.furieri@lqt.it

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

/**
 \file rl2wms.h

 WMS support
 */

#ifndef _RL2WMS_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RL2WMS_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct rl2_wms_catalog rl2WmsCatalog;
    typedef rl2WmsCatalog *rl2WmsCatalogPtr;

    typedef struct rl2_wms_item rl2WmsLayer;
    typedef rl2WmsLayer *rl2WmsLayerPtr;

    typedef struct rl2_wms_tiled_item rl2WmsTiledLayer;
    typedef rl2WmsTiledLayer *rl2WmsTiledLayerPtr;

    typedef struct rl2_wms_tile_pattern rl2WmsTilePattern;
    typedef rl2WmsTilePattern *rl2WmsTilePatternPtr;

    typedef struct rl2_wms_feature_collection rl2WmsFeatureCollection;
    typedef rl2WmsFeatureCollection *rl2WmsFeatureCollectionPtr;

    typedef struct rl2_wms_feature_member rl2WmsFeatureMember;
    typedef rl2WmsFeatureMember *rl2WmsFeatureMemberPtr;

    typedef struct rl2_wms_cache rl2WmsCache;
    typedef struct rl2_wms_cache *rl2WmsCachePtr;

/**
 Creates a Catalog for some WMS service 

 \param cache_handle handle to local WMS cache
 \param url pointer to some WMS-GetCapabilities XML Document.
 \param proxy pointer to some HTTP PROXY: may be NULL. 

 \return the pointer to the corresponding WMS-Catalog object: NULL on failure
 
 \sa destroy_wms_catalog, get_wms_version, get_wms_name, get_wms_title, get_wms_abstract, 
 is_wms_tile_service, get_wms_catalog_count, get_wms_catalog_layer
 
 \note an eventual error message returned via err_msg requires to be deallocated
 by invoking free().\n
 you are responsible to destroy (before or after) any WMS-Catalog returned by create_wms_catalog().
 */
    RL2_DECLARE rl2WmsCatalogPtr create_wms_catalog (rl2WmsCachePtr
						     cache_handle,
						     const char *url,
						     const char *proxy);

/**
 Destroys a WMS-Catalog object freeing any allocated resource 

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()
 
 \sa create_wms_catalog
 */
    RL2_DECLARE void destroy_wms_catalog (rl2WmsCatalogPtr handle);

/**
 Tests if a WMS-Catalog object actually corresponds to a TileService

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return 0 (false) if not, any other value if yes: a negative value on error.
 
 \sa create_wms_catalog, get_wms_tile_service_name, get_wms_tile_service_title, 
 get_wms_tile_service_abstract, get_wms_tile_service_count, get_wms_catalog_tiled_layer
 */
    RL2_DECLARE int is_wms_tile_service (rl2WmsCatalogPtr handle);

/**
 Return the TileService name corresponding to some WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog (of the TileService type)
 returned by a previous call to get_wms_catalog_layer().

 \return the name corresponding to the WMS TileService (if any)
 
 \sa get_wms_tile_service_title, get_wms_tile_service_abstract
 */
    RL2_DECLARE const char *get_wms_tile_service_name (rl2WmsCatalogPtr handle);

/**
 Return the TileService title corresponding to some WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog (of the TileService type)
 returned by a previous call to get_wms_catalog_layer().

 \return the title corresponding to the WMS TileService (if any)
 
 \sa get_wms_tile_service_name, get_wms_tile_service_abstract
 */
    RL2_DECLARE const char *get_wms_tile_service_title (rl2WmsCatalogPtr
							handle);

/**
 Return the TileService abstract corresponding to some WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog (of the TileService type)
 returned by a previous call to get_wms_catalog_layer().

 \return the abstract corresponding to the WMS TileService (if any)
 
 \sa get_wms_tile_service_title, get_wms_tile_service_title
 */
    RL2_DECLARE const char *get_wms_tile_service_abstract (rl2WmsCatalogPtr
							   handle);

/**
 Return the total count of first-level Layers defined within a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the total count of first-level Layers defined within a WMS-Catalog object: 
 a negative number if the WMS-Catalog isn't valid
 
 \sa create_wms_catalog, get_wms_catalog_layer, get_wms_format_count,
 get_wms_tile_service_count
 */
    RL2_DECLARE int get_wms_catalog_count (rl2WmsCatalogPtr handle);

/**
 Return the total count of first-level Tiled Layers defined within a 
 WMS-Catalog [TileService] object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the total count of first-level Tiled Layers defined within a WMS-Catalog
 [TileService] object: 
 a negative number if the WMS-Catalog isn't valid
 
 \sa create_wms_catalog, get_wms_catalog_tiled, layer, get_wms_format_count,
 get_wms_catalog_count
 */
    RL2_DECLARE int get_wms_tile_service_count (rl2WmsCatalogPtr handle);

/**
 Return the total count of Formats supported by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()
 \param mode TRUE if only valid "image formats" should be considered; 
 if set to FALSE then any possible format will be considered.

 \return the total count of Formats supported by a WMS-Catalog object: 
 ZERO or a a negative number if the WMS-Catalog isn't valid
 
 \sa create_wms_catalog, get_wms_format, get_wms_catalog_count
 */
    RL2_DECLARE int get_wms_format_count (rl2WmsCatalogPtr handle, int mode);

/**
 Return the version string corresponding to some WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to get_wms_catalog_layer().

 \return the version string corresponding to the WMS-Catalog object
 
 \sa get_wms_name, get_wms_title, get_wms_abstract
 */
    RL2_DECLARE const char *get_wms_version (rl2WmsCatalogPtr handle);

/**
 Return the name corresponding to some WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to get_wms_catalog_layer().

 \return the name corresponding to the WMS-Catalog object
 
 \sa get_wms_version, get_wms_title, get_wms_abstract
 */
    RL2_DECLARE const char *get_wms_name (rl2WmsCatalogPtr handle);

/**
 Return the title corresponding to some WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to get_wms_catalog_layer().

 \return the title corresponding to the WMS-Catalog object
 
 \sa get_wms_version, get_wms_name, get_wms_abstract
 */
    RL2_DECLARE const char *get_wms_title (rl2WmsCatalogPtr handle);

/**
 Return the abstract corresponding to some WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to get_wms_catalog_layer().

 \return the abstract corresponding to the WMS-Catalog object
 
 \sa get_wms_version, get_wms_name, get_wms_title
 */
    RL2_DECLARE const char *get_wms_abstract (rl2WmsCatalogPtr handle);

/**
 Return the GetMap URL (method GET) from a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the corresponding URL: NULL if the URL is not defined or if any error is encountered.
 
 \sa create_wms_catalog, get_wms_url_GetMap_post, get_wms_url_GetTileService_get,
 get_wms_url_GetTileService_post, get_wms_url_GetFeatureInfo_get, 
 get_wms_url_GetFeatureInfo_post
 */
    RL2_DECLARE const char *get_wms_url_GetMap_get (rl2WmsCatalogPtr handle);

/**
 Return the GetMap URL (method POST) from a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the corresponding URL: NULL if the URL is not defined or if any error is encountered.
 
 \sa create_wms_catalog, get_wms_url_GetMap_get, get_wms_url_GetTileService_get,
 get_wms_url_GetTileService_post, get_wms_url_GetFeatureInfo_get,
 get_wms_url_GetFeatureInfo_post
 */
    RL2_DECLARE const char *get_wms_url_GetMap_post (rl2WmsCatalogPtr handle);

/**
 Return the GetTileService URL (method GET) from a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the corresponding URL: NULL if the URL is not defined or if any error is encountered.
 
 \sa create_wms_catalog, get_wms_url_GetMap_get, get_wms_url_GetMap_post, 
 get_wms_url_GetTileService_post, get_wms_url_GetFeatureInfo_get, 
 get_wms_url_GetFeatureInfo_post
 */
    RL2_DECLARE const char *get_wms_url_GetTileService_get (rl2WmsCatalogPtr
							    handle);

/**
 Return the GetTileService URL (method POST) from a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the corresponding URL: NULL if the URL is not defined or if any error is encountered.
 
 \sa create_wms_catalog, get_wms_url_GetMap_get, get_wms_url_GetMap_post,
 get_wms_url_GetTileService_get, get_wms_url_GetFeatureInfo_get,
 get_wms_url_GetFeatureInfo_post
 */
    RL2_DECLARE const char *get_wms_url_GetTileService_post (rl2WmsCatalogPtr
							     handle);

/**
 Return the GetFeatureInfo URL (method GET) from a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the corresponding URL: NULL if the URL is not defined or if any error is encountered.
 
 \sa create_wms_catalog, get_wms_url_GetMap_get, get_wms_url_GetMap_post, 
 get_wms_url_GetTileService_get, get_wms_url_GetTileService_post, 
 get_wms_url_GetFeatureInfo_post, get_wms_gml_mime_type, get_wms_xml_mime_type
 */
    RL2_DECLARE const char *get_wms_url_GetFeatureInfo_get (rl2WmsCatalogPtr
							    handle);

/**
 Return the GetFeatureInfo URL (method POST) from a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the corresponding URL: NULL if the URL is not defined or if any error is encountered.
 
 \sa create_wms_catalog, get_wms_url_GetMap_get, get_wms_url_GetMap_post,
 get_wms_url_GetTileService_get, get_wms_url_GetTileService_post, 
 get_wms_url_GetFeatureInfo_get, get_wms_gml_mime_type, get_wms_xml_mime_type
 */
    RL2_DECLARE const char *get_wms_url_GetFeatureInfo_post (rl2WmsCatalogPtr
							     handle);

/**
 Return the GML MIME type name from a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the MIME type corresponding to GML: NULL if it's not defined or if any error is encountered.
 
 \sa create_wms_catalog, get_wms_url_GetFeatureInfo_get, get_wms_url_GetFeatureInfo_post, 
 get_wms_xml_mime_type
 */
    RL2_DECLARE const char *get_wms_gml_mime_type (rl2WmsCatalogPtr handle);

/**
 Return the XML MIME type name from a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the MIME type corresponding to XML: NULL if it's not defined or if any error is encountered.
 
 \sa create_wms_catalog, get_wms_url_GetFeatureInfo_get, get_wms_url_GetFeatureInfo_post, 
 get_wms_gml_mime_type
 */
    RL2_DECLARE const char *get_wms_xml_mime_type (rl2WmsCatalogPtr handle);

/**
 Return the Contact Person defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the Contact Person: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_person (rl2WmsCatalogPtr handle);

/**
 Return the Contact Organization defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the Contact Organization: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_organization (rl2WmsCatalogPtr
							  handle);

/**
 Return the Contact Position defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the Contact Position: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_position (rl2WmsCatalogPtr handle);

/**
 Return the Postal Address defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the Postal Address: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_postal_address (rl2WmsCatalogPtr
							    handle);

/**
 Return the City (Postal Address) defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the City (Postal Address): NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_city (rl2WmsCatalogPtr handle);

/**
 Return the State or Provicne (Postal Address) defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the State or Province (Postal Address): NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_state_province (rl2WmsCatalogPtr
							    handle);

/**
 Return the Post Code (Postal Address)  defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the Post Code (Postal Address) : NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_post_code (rl2WmsCatalogPtr handle);

/**
 Return the Country (Postal Address) defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the Country (Postal Address): NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_country (rl2WmsCatalogPtr handle);

/**
 Return the Voice Telephone defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the Voice Telephone: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_voice_telephone (rl2WmsCatalogPtr
							     handle);

/**
 Return the FAX Telephone defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the FAX Telephone: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_fax_telephone (rl2WmsCatalogPtr
							   handle);

/**
 Return the e-mail Address defined by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the e-mail Address: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_contact_email_address (rl2WmsCatalogPtr
							   handle);

/**
 Return the Fees required by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the required Fees: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_fees (rl2WmsCatalogPtr handle);

/**
 Return the Access Constraints supported by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the imposed Access Constraints: NULL if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_access_constraints (rl2WmsCatalogPtr
							handle);

/**
 Return the Layer Limit supported by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the supported Layer Limit: a negative number if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE int get_wms_layer_limit (rl2WmsCatalogPtr handle);

/**
 Return the MaxWidth supported by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the MaxWidth measured in pixels: a negative number if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE int get_wms_max_width (rl2WmsCatalogPtr handle);

/**
 Return the MaxHeight supported by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()

 \return the MaxHeight measured in pixels: a negative number if undeclared/unspecified.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE int get_wms_max_height (rl2WmsCatalogPtr handle);

/**
 Return one of the Formats supported by a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()
 \param index the relative index identifying the required Format (the first 
 Format value supported by a WMS-Catalog object has index ZERO).
 \param mode TRUE if only valid "image formats" should be considered; 
 if set to FALSE then any possible format will be considered.

 \return the Format string: NULL if the required Format isn't defined.
 
 \sa create_wms_catalog, get_wms_format_count
 */
    RL2_DECLARE const char *get_wms_format (rl2WmsCatalogPtr handle,
					    int index, int mode);

/**
 Return the pointer to some specific Layer defined within a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog returned by a previous call
 to create_wms_catalog()
 \param index the relative index identifying the required WMS-Layer (the first 
 Layer in the WMS-Catalaog object has index ZERO).

 \return the pointer to the required WMS-Layer object: NULL if the passed index
 isn't valid
 
 \sa create_wms_catalog, get_wms_catalog_count, get_wms_layer_name, get_wms_layer_title, 
 get_wms_layer_abstract, get_wms_layer_crs_count, get_wms_layer_crs
 */
    RL2_DECLARE rl2WmsLayerPtr get_wms_catalog_layer (rl2WmsCatalogPtr
						      handle, int index);

/**
 Testing if some WMS-Layer object has Layer children

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return 0 (false) if not, any other value if yes
 
 \sa get_wms_catalog_layer, get_wms_layer_children_count, get_wms_child_layer
 */
    RL2_DECLARE int wms_layer_has_children (rl2WmsLayerPtr handle);

/**
 Return the total count of children Layers defined within a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer 

 \return the total count of children Layers defined within a WMS-Layer object: 
 a negative number if the WMS-Layer isn't valid
 
 \sa get_wms_catalog_layer, wms_layer_has_children, get_wms_child_layer
 */
    RL2_DECLARE int get_wms_layer_children_count (rl2WmsLayerPtr handle);

/**
 Return the pointer to some child Layer defined within a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer
 \param index the relative index identifying the required WMS-Layer (the first 
 child Layer in the WMS-Layer object has index ZERO).

 \return the pointer to the required WMS-Layer object: NULL if the passed index
 isn't valid
 
 \sa get_wms_catalog_layer, wms_layer_has_children, get_wms_layer_children_count
 */
    RL2_DECLARE rl2WmsLayerPtr get_wms_child_layer (rl2WmsLayerPtr
						    handle, int index);

/**
 Return the name corresponding to some WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return the name corresponding to the WMS-Layer object
 
 \sa get_wms_layer_title, get_wms_layer_abstract, get_wms_layer_crs_count, get_wms_layer_crs,
 wms_layer_has_children
 */
    RL2_DECLARE const char *get_wms_layer_name (rl2WmsLayerPtr handle);

/**
 Return the title corresponding to some WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return the title corresponding to the WMS-Layer object
 
 \sa get_wms_layer_name, get_wms_layer_abstract, get_wms_layer_crs_count, get_wms_layer_crs
 */
    RL2_DECLARE const char *get_wms_layer_title (rl2WmsLayerPtr handle);

/**
 Return the abstract corresponding to some WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return the abstract corresponding to the WMS-Layer object
 
 \sa get_wms_layer_name, get_wms_layer_title, get_wms_layer_crs_count, get_wms_layer_crs
 */
    RL2_DECLARE const char *get_wms_layer_abstract (rl2WmsLayerPtr handle);

/**
 Return the total count of CRSs supported by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return the total count of CRSs supported by a WMS-Layer object: 
 ZERO or a negative number if the WMS-Layer isn't valid
 
 \sa get_wms_layer_name, get_wms_layer_title, get_wms_layer_abstract, 
 get_wms_layer_crs, get_wms_layer_style_count
 */
    RL2_DECLARE int get_wms_layer_crs_count (rl2WmsLayerPtr handle);

/**
 Return one of the CRSs supported by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().
 \param index the relative index identifying the required CRS (the first 
 CRS value supported by a WMS-Layer object has index ZERO).

 \return the CRS string: NULL if the required CRS isn't defined.
 
 \sa get_wms_layer_name, get_wms_layer_title, get_wms_layer_abstract, 
 get_wms_layer_crs_count 
 */
    RL2_DECLARE const char *get_wms_layer_crs (rl2WmsLayerPtr handle,
					       int index);

/**
 Tests if some WMS-Layer object declares the Opaque property

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return TRUE or FALSE: a negative number if undefined/unspecified or if
 isn't a valid WMS-Layer.
 
 \sa get_wms_layer_name, get_wms_layer_title, get_wms_layer_abstract, 
 is_wms_layer_queriable 
 */
    RL2_DECLARE int is_wms_layer_opaque (rl2WmsLayerPtr handle);

/**
 Tests if some WMS-Layer object declares the Queryable property

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return TRUE or FALSE: a negative number if undefined/unspecified or if
 isn't a valid WMS-Layer.
 
 \sa get_wms_layer_name, get_wms_layer_title, get_wms_layer_abstract, 
 is_wms_layer_transparent 
 */
    RL2_DECLARE int is_wms_layer_queryable (rl2WmsLayerPtr handle);

/**
 Return the total count of Styles supported by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return the total count of Styles supported by a WMS-Layer object: 
 ZERO or a negative number if the WMS-Layer isn't valid
 
 \sa get_wms_layer_crs_count, get_wms_layer_style_name, get_wms_layer_style_title,
 get_wms_layer_style_abstract 
 */
    RL2_DECLARE int get_wms_layer_style_count (rl2WmsLayerPtr handle);

/**
 Return the Min Scale Denominator supported by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return the Min Scale Denominator declared by a WMS-Layer object: 
 DBL_MAX if undeclared or if the WMS-Layer isn't valid
 
 \sa get_wms_layer_crs_count, get_wms_layer_max_scale_denominator
 */
    RL2_DECLARE double get_wms_layer_min_scale_denominator (rl2WmsLayerPtr
							    handle);

/**
 Return the Max Scale Denominator supported by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().

 \return the Max Scale Denominator declared by a WMS-Layer object: 
 DBL_MAX if undeclared or if the WMS-Layer isn't valid
 
 \sa get_wms_layer_crs_count, get_wms_layer_min_scale_denominator
 */
    RL2_DECLARE double get_wms_layer_max_scale_denominator (rl2WmsLayerPtr
							    handle);

/**
 Return the Geographic Bounding Box (long/lat) declared by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().
 \param minx on successful completion will contain the Min Longitude
 \param maxx on successful completion will contain the Max Longitude
 \param miny on successful completion will contain the Min Latitude
 \param maxy on successful completion will contain the Max Latitude

 \return TRUE on success, FALSE if any error occurred
 
 \sa get_wms_layer_bbbox
 */
    RL2_DECLARE int get_wms_layer_geo_bbox (rl2WmsLayerPtr handle, double *minx,
					    double *maxx, double *miny,
					    double *maxy);

/**
 Return the Bounding Box corresponding to some CRS declared by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().
 \param crs the string value identifying some specific CRS
 \param minx on successful completion will contain the Min Longitude
 \param maxx on successful completion will contain the Max Longitude
 \param miny on successful completion will contain the Min Latitude
 \param maxy on successful completion will contain the Max Latitude

 \return TRUE on success, FALSE if any error occurred
 
 \sa get_wms_layer_geo_bbox
 */
    RL2_DECLARE int get_wms_layer_bbox (rl2WmsLayerPtr handle, const char *crs,
					double *minx, double *maxx,
					double *miny, double *maxy);

/**
 Return the Name of some Style supported by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().
 \param index the relative index identifying the required Style (the first 
 Style supported by a WMS-Layer object has index ZERO).

 \return the Style's Name: NULL if the required Style isn't defined.
 
 \sa get_wms_layer_style_count, get_wms_layer_style_title, get_wms_layer_style_abstract
 */
    RL2_DECLARE const char *get_wms_layer_style_name (rl2WmsLayerPtr handle,
						      int index);

/**
 Return the Title of some Style supported by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().
 \param index the relative index identifying the required Style (the first 
 Style supported by a WMS-Layer object has index ZERO).

 \return the Style's Title: NULL if the required Style isn't defined.
 
 \sa get_wms_layer_style_count, get_wms_layer_style_name, get_wms_layer_style_abstract 
 */
    RL2_DECLARE const char *get_wms_layer_style_title (rl2WmsLayerPtr handle,
						       int index);

/**
 Return the Abstract of some Style supported by a WMS-Layer object

 \param handle the pointer to a valid WMS-Layer returned by a previous call
 to get_wms_catalog_layer().
 \param index the relative index identifying the required Style (the first 
 Style supported by a WMS-Layer object has index ZERO).

 \return the Style's Abstract: NULL if the required Style isn't defined.
 
 \sa get_wms_layer_style_count, get_wms_layer_style_name, get_wms_layer_style_title 
 */
    RL2_DECLARE const char *get_wms_layer_style_abstract (rl2WmsLayerPtr handle,
							  int index);

/**
 Return the pointer to some specific Tiled Layer defined within a WMS-Catalog object

 \param handle the pointer to a valid WMS-Catalog [of the TileService type]
 returned by a previous call to create_wms_catalog()
 \param index the relative index identifying the required WMS-TiledLayer (the first 
 Tiled Layer in the WMS-Catalaog object has index ZERO).

 \return the pointer to the required WMS-TiledLayer object: NULL if the passed index
 isn't valid
 
 \sa create_wms_catalog, get_wms_tile_service_count, get_wms_tiled_layer_name, 
 get_wms_tiled_layer_title, get_wms_tiled_layer_abstract, get_wms_tiled_layer_crs
 */
    RL2_DECLARE rl2WmsTiledLayerPtr
	get_wms_catalog_tiled_layer (rl2WmsCatalogPtr handle, int index);

/**
 Testing if some WMS-TiledLayer object has TiledLayer children

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_tiled_layer().

 \return 0 (false) if not, any other value if yes
 
 \sa get_wms_catalog_tiled_layer, get_wms_tiled_layer_children_count, get_wms_child_tiled_layer
 */
    RL2_DECLARE int wms_tiled_layer_has_children (rl2WmsTiledLayerPtr handle);

/**
 Return the total count of children TiledLayers defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 

 \return the total count of children TiledLayers defined within a WMS-TiledLayer object: 
 a negative number if the WMS-TiledLayer isn't valid
 
 \sa get_wms_catalog_tiled_layer, wms_tiled_layer_has_children, get_wms_child_layer
 */
    RL2_DECLARE int get_wms_tiled_layer_children_count (rl2WmsTiledLayerPtr
							handle);

/**
 Return the pointer to some child TiledLayer defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer
 \param index the relative index identifying the required WMS-TiledLayer (the first 
 child Layer in the WMS-TiledLayer object has index ZERO).

 \return the pointer to the required WMS-TiledLayer object: NULL if the passed index
 isn't valid
 
 \sa get_wms_catalog_tiled_layer, wms_tiled_layer_has_children, get_wms_tiled_layer_children_count
 */
    RL2_DECLARE rl2WmsTiledLayerPtr
	get_wms_child_tiled_layer (rl2WmsTiledLayerPtr handle, int index);

/**
 Return the name corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the name corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_title, get_wms_tiled_layer_abstract, get_wms_tiled_layer_crs,
 get_wms_tiled_layer_format, get_wms_tiled_layer_style, get_wms_tiled_layer_pad, 
 get_wms_tiled_layer_bands, get_wms_tiled_layer_data_type, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children, get_wms_tile_pattern_count
 */
    RL2_DECLARE const char *get_wms_tiled_layer_name (rl2WmsTiledLayerPtr
						      handle);

/**
 Return the title corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the title corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_abstract, get_wms_tiled_layer_crs,
 get_wms_tiled_layer_format, get_wms_tiled_layer_style, get_wms_tiled_layer_pad, 
 get_wms_tiled_layer_bands, get_wms_tiled_layer_data_type, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children, get_wms_tile_pattern_count
 */
    RL2_DECLARE const char *get_wms_tiled_layer_title (rl2WmsTiledLayerPtr
						       handle);

/**
 Return the abstract corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the abstract corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, get_wms_tiled_layer_crs,
 get_wms_tiled_layer_format, get_wms_tiled_layer_style, get_wms_tiled_layer_pad, 
 get_wms_tiled_layer_bands, get_wms_tiled_layer_data_type, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children, get_wms_tile_pattern_count
 */
    RL2_DECLARE const char *get_wms_tiled_layer_abstract (rl2WmsTiledLayerPtr
							  handle);

/**
 Return the CRS corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the CRS corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, get_wms_tiled_layer_abstract,
 get_wms_tiled_layer_format, get_wms_tiled_layer_style, get_wms_tiled_layer_pad, 
 get_wms_tiled_layer_bands, get_wms_tiled_layer_data_type, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children
 */
    RL2_DECLARE const char *get_wms_tiled_layer_crs (rl2WmsTiledLayerPtr
						     handle);

/**
 Return the Format corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the Format corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, get_wms_tiled_layer_abstract,
 get_wms_tiled_layer_crs, get_wms_tiled_layer_style, get_wms_tiled_layer_pad, 
 get_wms_tiled_layer_bands, get_wms_tiled_layer_data_type, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children, get_wms_tile_pattern_count
 */
    RL2_DECLARE const char *get_wms_tiled_layer_format (rl2WmsTiledLayerPtr
							handle);

/**
 Return the Style corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the Style corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, get_wms_tiled_layer_abstract,
 get_wms_tiled_layer_crs, get_wms_tiled_layer_format, get_wms_tiled_layer_pad, 
 get_wms_tiled_layer_bands, get_wms_tiled_layer_data_type, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children, get_wms_tile_pattern_count
 */
    RL2_DECLARE const char *get_wms_tiled_layer_style (rl2WmsTiledLayerPtr
						       handle);

/**
 Return the Pad corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the Pad corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, get_wms_tiled_layer_abstract,
 get_wms_tiled_layer_crs, get_wms_tiled_layer_format, get_wms_tiled_layer_style, 
 get_wms_tiled_layer_bands, get_wms_tiled_layer_data_type, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children
 */
    RL2_DECLARE const char *get_wms_tiled_layer_pad (rl2WmsTiledLayerPtr
						     handle);

/**
 Return the Bands corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the Bands corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, get_wms_tiled_layer_abstract,
 get_wms_tiled_layer_crs, get_wms_tiled_layer_format, get_wms_tiled_layer_style, 
 get_wms_tiled_layer_pad, get_wms_tiled_layer_data_type, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children, get_wms_tile_pattern_count
 */
    RL2_DECLARE const char *get_wms_tiled_layer_bands (rl2WmsTiledLayerPtr
						       handle);

/**
 Return the DataType corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().

 \return the DataType corresponding to the WMS-TiledLayer object
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, get_wms_tiled_layer_abstract,
 get_wms_tiled_layer_crs, get_wms_tiled_layer_format, get_wms_tiled_layer_style, 
 get_wms_tiled_layer_pad, get_wms_tiled_layer_bands, get_wms_tiled_layer_bbox,
 wms_tiled_layer_has_children, get_wms_tile_pattern_count
 */
    RL2_DECLARE const char *get_wms_tiled_layer_data_type (rl2WmsTiledLayerPtr
							   handle);

/**
 Return the BoundingBox corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().
 \param minx on successful completion will contain the Min Longitude
 \param maxx on successful completion will contain the Max Longitude
 \param miny on successful completion will contain the Min Latitude
 \param maxy on successful completion will contain the Max Latitude

 \return 0 on error, any other value on success
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, 
 get_wms_tiled_layer_abstract, get_wms_tiled_layer_crs, get_wms_tiled_layer_format, 
 get_wms_tiled_layer_style, get_wms_tiled_layer_pad, get_wms_tiled_layer_bands,
 get_wms_tiled_layer_data_type, wms_tiled_layer_has_children, 
 get_wms_tiled_layer_tile_size, get_wms_tile_pattern_count, 
 get_wms_tile_pattern_handle
 */
    RL2_DECLARE int get_wms_tiled_layer_bbox (rl2WmsTiledLayerPtr handle,
					      double *minx, double *miny,
					      double *maxx, double *maxy);

/**
 Return the Tile Size corresponding to some WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer returned by a previous call
 to get_wms_catalog_layer().
 \param width on successful completion will contain the Tile Width
 \param height on successful completion will contain the Tile Height

 \return 0 on error, any other value on success
 
 \sa get_wms_tiled_layer_name, get_wms_tiled_layer_title, 
 get_wms_tiled_layer_abstract, get_wms_tiled_layer_crs, get_wms_tiled_layer_format, 
 get_wms_tiled_layer_style, get_wms_tiled_layer_pad, get_wms_tiled_layer_bands,
 get_wms_tiled_layer_data_type, wms_tiled_layer_has_children, 
 get_wms_tiled_layer_bbox, get_wms_tile_pattern_count, 
 get_wms_tile_pattern_handle
 */
    RL2_DECLARE int get_wms_tiled_layer_tile_size (rl2WmsTiledLayerPtr handle,
						   int *width, int *height);

/**
 Return the total count of TiledPatterns defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 

 \return the total count of TiledPatterns defined within a WMS-TiledLayer object: 
 a negative number if the WMS-TiledLayer isn't valid
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_srs, 
 get_wms_tile_pattern_tile_width, get_wms_tile_pattern_tile_height,
 get_wms_tile_pattern_base_x, get_wms_tile_pattern_base_y,
 get_wms_tile_pattern_extent_x, get_wms_tile_pattern_handle
 */
    RL2_DECLARE int get_wms_tile_pattern_count (rl2WmsTiledLayerPtr handle);

/**
 Return the SRS from a TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 
 \param index the relative index identifying the required WMS-TilePattern (the first 
 TilePattern Layer in the WMS-TiledLayer object has index ZERO).

 \return the SRS defined within a WMS-TilePattern object: 
 NULL if the WMS-TilePattern isn't valid.
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_tile_width, 
 get_wms_tile_pattern_tile_height, get_wms_tile_pattern_base_x,
 get_wms_tile_pattern_base_y, get_wms_tile_pattern_extent_x,
 get_wms_tile_pattern_extent_y, get_wms_tile_pattern_handle
 */
    RL2_DECLARE const char *get_wms_tile_pattern_srs (rl2WmsTiledLayerPtr
						      handle, int index);

/**
 Return the TileWidth from a TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 
 \param index the relative index identifying the required WMS-TilePattern (the first 
 TilePattern Layer in the WMS-TiledLayer object has index ZERO).

 \return the TileWidth defined within a WMS-TilePattern object: 
 a negative value if the WMS-TilePattern isn't valid.
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_srs, 
 get_wms_tile_pattern_tile_height, get_wms_tile_pattern_base_x,
 get_wms_tile_pattern_base_y, get_wms_tile_pattern_extent_x,
 get_wms_tile_pattern_extent_y, get_wms_tile_pattern_handle
 */
    RL2_DECLARE int get_wms_tile_pattern_tile_width (rl2WmsTiledLayerPtr
						     handle, int index);

/**
 Return the TileHeight from a TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 
 \param index the relative index identifying the required WMS-TilePattern (the first 
 TilePattern Layer in the WMS-TiledLayer object has index ZERO).

 \return the TileHeight defined within a WMS-TilePattern object: 
 a negative value if the WMS-TilePattern isn't valid.
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_srs, 
 get_wms_tile_pattern_tile_width, get_wms_tile_pattern_base_x,
 get_wms_tile_pattern_base_y, get_wms_tile_pattern_extent_x,
 get_wms_tile_pattern_extent_y, get_wms_tile_pattern_handle
 */
    RL2_DECLARE int get_wms_tile_pattern_tile_height (rl2WmsTiledLayerPtr
						      handle, int index);

/**
 Return the TileBaseX from a TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 
 \param index the relative index identifying the required WMS-TilePattern (the first 
 TilePattern Layer in the WMS-TiledLayer object has index ZERO).

 \return the TileBaseX (leftmost coord) defined within a WMS-TilePattern object: 
 DBL_MAX if the WMS-TilePattern isn't valid.
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_srs, 
 get_wms_tile_pattern_tile_width, get_wms_tile_pattern_tile_height,
 get_wms_tile_pattern_base_y, get_wms_tile_pattern_extent_x,
 get_wms_tile_pattern_extent_y, get_wms_tile_pattern_handle
 */
    RL2_DECLARE double get_wms_tile_pattern_base_x (rl2WmsTiledLayerPtr
						    handle, int index);

/**
 Return the TileBaseY from a TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 
 \param index the relative index identifying the required WMS-TilePattern (the first 
 TilePattern Layer in the WMS-TiledLayer object has index ZERO).

 \return the TileBaseY (topmost coord) defined within a WMS-TilePattern object: 
 DBL_MAX if the WMS-TilePattern isn't valid.
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_srs, 
 get_wms_tile_pattern_tile_width, get_wms_tile_pattern_tile_height,
 get_wms_tile_pattern_base_x, get_wms_tile_pattern_extent_x,
 get_wms_tile_pattern_extent_y, get_wms_tile_pattern_handle
 */
    RL2_DECLARE double get_wms_tile_pattern_base_y (rl2WmsTiledLayerPtr
						    handle, int index);

/**
 Return the TileExtentX from a TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 
 \param index the relative index identifying the required WMS-TilePattern (the first 
 TilePattern Layer in the WMS-TiledLayer object has index ZERO).

 \return the TileExtentX defined within a WMS-TilePattern object: 
 DBL_MAX if the WMS-TilePattern isn't valid.
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_srs, 
 get_wms_tile_pattern_tile_width, get_wms_tile_pattern_tile_height,
 get_wms_tile_pattern_base_x, get_wms_tile_pattern_base_y,
 get_wms_tile_pattern_extent_y, get_wms_tile_pattern_handle
 */
    RL2_DECLARE double get_wms_tile_pattern_extent_x (rl2WmsTiledLayerPtr
						      handle, int index);

/**
 Return the TileExtentY from a TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 
 \param index the relative index identifying the required WMS-TilePattern (the first 
 TilePattern Layer in the WMS-TiledLayer object has index ZERO).

 \return the TileExtentX defined within a WMS-TilePattern object: 
 DBL_MAX if the WMS-TilePattern isn't valid.
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_srs, 
 get_wms_tile_pattern_tile_width, get_wms_tile_pattern_tile_height,
 get_wms_tile_pattern_base_x, get_wms_tile_pattern_base_y,
 get_wms_tile_pattern_extent_x, get_wms_tile_pattern_handle
 */
    RL2_DECLARE double get_wms_tile_pattern_extent_y (rl2WmsTiledLayerPtr
						      handle, int index);

/**
 Return the handle for some TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TiledLayer 
 \param pattern_index the relative index identifying the required WMS-TilePattern (the first 
 TilePattern Layer in the WMS-TiledLayer object has index ZERO).

 \return the handle for some TilePattern object inside
 a WMS-TiledLayer object: NULL if any error is encountered.
 
 \sa get_wms_tile_pattern_count, get_wms_tile_pattern_srs, 
 get_wms_tile_pattern_tile_width, get_wms_tile_pattern_tile_height,
 get_wms_tile_pattern_base_x, get_wms_tile_pattern_base_y,
 get_wms_tile_pattern_extent_x, get_wms_tile_pattern_extent_y
 */
    RL2_DECLARE rl2WmsTilePatternPtr
	get_wms_tile_pattern_handle (rl2WmsTiledLayerPtr handle, int index);

/**
 Return a cloned copy of some TilePattern object

 \param handle the pointer to a valid WMS-TilePattern returned by get_wms_tile_pattern_handle() 

 \return the handle to the new TilePattern: NULL if any error is encountered.
 
 \sa get_wms_tile_pattern_handle, destroy_wms_tile_pattern, 
 get_wms_tile_pattern_sample_url, get_wms_tile_pattern_request_url
 
 \note you are responsible to destroy before or after any WMS-TilePattern 
 object created by clone_wms_tile_pattern() by invoking destroy_wms_tile_pattern().
 */
    RL2_DECLARE rl2WmsTilePatternPtr
	clone_wms_tile_pattern (rl2WmsTilePatternPtr handle);

/**
 Destroys a cloned copy of some TilePattern object

 \param handle the pointer to a valid WMS-TilePattern returned by clone_wms_tile_pattern_handle() 

 
 \sa get_wms_tile_pattern_handle, clone_wms_tile_pattern, 
 get_wms_tile_pattern_sample_url, get_wms_tile_pattern_request_url
 */
    RL2_DECLARE void destroy_wms_tile_pattern (rl2WmsTilePatternPtr handle);

/**
 Return the sample URL representing a TiledPattern defined within a WMS-TiledLayer object

 \param handle the pointer to a valid WMS-TilePattern returned by get_wms_tile_pattern_handle() 

 \return the sample URL representing a TilePattern object inside
 a WMS-TiledLayer object: NULL if any error is encountered.
 
 \sa get_wms_tile_pattern_handle, clone_wms_tile_pattern, destroy_wms_tile_pattern, 
 get_wms_tile_pattern_request_url
 
 \note the returned sample URL corresponds to dynamically allocated memory,
 and thus requires to be deallocated by invoking sqlite3_free().
 */
    RL2_DECLARE char *get_wms_tile_pattern_sample_url (rl2WmsTilePatternPtr
						       handle);

/**
 Return the full request URL for some TiledPattern defined within a WMS-TiledLayer object

 
 \param handle the pointer to a valid WMS-TilePattern returned by get_wms_tile_pattern_handle() 

 \return the full request URL for some TilePattern object inside
 a WMS-TiledLayer object: NULL if any error is encountered.
 
 \sa get_wms_tile_pattern_handle, clone_wms_tile_pattern, destroy_wms_tile_pattern, 
 , get_wms_tile_pattern_sample_url
 
 \note the returned full request URL corresponds to dynamically allocated memory,
 and thus requires to be deallocated by invoking sqlite3_free().
 */
    RL2_DECLARE char *get_wms_tile_pattern_request_url (rl2WmsTilePatternPtr
							handle,
							const char *base_url,
							double min_x,
							double min_y);

/**
 Creates a WMS-Cache object 

 \return the pointer to the corresponding WMS-Cache object: NULL on failure
 
 \sa destroy_wms_cache, reset_wms_cache, get_wms_cache_max_size, set_wms_cache_max_size,
 get_wms_cache_items_count, get_wms_cache_current_size, get_wms_cache_hit_count,
 get_wms_cache_miss_count, get_wms_cache_flushed_count, get_wms_total_download_size
 
 \note you are responsible to destroy (before or after) any WMS-Cache created by create_wms_cache().
 */
    RL2_DECLARE rl2WmsCachePtr create_wms_cache (void);

/**
 Destroys a WMS-Cache object freeing any allocated resource 

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()
 
 \sa create_wms_cache, reset_wms_cache, get_wms_cache_max_size, set_wms_cache_max_size,
 get_wms_cache_items_count, get_wms_cache_current_size, get_wms_cache_hit_count,
 get_wms_cache_miss_count, get_wms_cache_flushed_count, get_wms_total_download_size
 */
    RL2_DECLARE void destroy_wms_cache (rl2WmsCachePtr handle);

/**
 Resets a WMS-Cache object to its initial empty state.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()
 
 \sa create_wms_cache, destroy_wms_cache, get_wms_cache_max_size, set_wms_cache_max_size,
 get_wms_cache_items_count, get_wms_cache_current_size, get_wms_cache_hit_count,
 get_wms_cache_miss_count, get_wms_cache_flushed_count, get_wms_total_download_size
 */
    RL2_DECLARE void reset_wms_cache (rl2WmsCachePtr handle);

/**
 Return the current Max-Size from a WMS-Cache object.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()

 \return the currently set MaxSize (in bytes) from some WMS-Cache object.
 
 \sa create_wms_cache, destroy_wms_cache, reset_wms_cache, set_wms_cache_max_size,
 get_wms_cache_items_count, get_wms_cache_current_size, get_wms_cache_hit_count,
 get_wms_cache_miss_count, get_wms_cache_flushed_count, get_wms_total_download_size
 */
    RL2_DECLARE int get_wms_cache_max_size (rl2WmsCachePtr handle);

/**
 Chages the current Max Size for a WMS-Cache object.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()
 \param size the new Max Size (in bytes) to be set.
 
 \sa create_wms_cache, destroy_wms_cache, reset_wms_cache, get_wms_cache_max_size, 
 get_wms_cache_items_count, get_wms_cache_current_size, get_wms_cache_hit_count,
 get_wms_cache_miss_count, get_wms_cache_flushed_count, get_wms_total_download_size

 \note if the WMS-Cache currently uses more memory than allowed by the new setting
 any allocation in excess will be immediately freed.
 */
    RL2_DECLARE void set_wms_cache_max_size (rl2WmsCachePtr handle, int size);

/**
 Return the current number of cached items stored within a WMS-Cache object.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()

 \return the current number of cached items stored within a WMS-Cache object.
 
 \sa create_wms_cache, destroy_wms_cache, reset_wms_cache, get_wms_cache_max_size, 
 set_wms_cache_max_size, get_wms_cache_current_size, get_wms_cache_hit_count,
 get_wms_cache_miss_count, get_wms_cache_flushed_count, get_wms_total_download_size
 */
    RL2_DECLARE int get_wms_cache_items_count (rl2WmsCachePtr handle);

/**
 Return the current memory allocation used by a WMS-Cache object.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()

 \return the current memory allocation (in bytes) used by a WMS-Cache object.
 
 \sa create_wms_cache, destroy_wms_cache, reset_wms_cache, get_wms_cache_max_size, 
 set_wms_cache_max_size, get_wms_cache_items_count, get_wms_cache_hit_count,
 get_wms_cache_miss_count, get_wms_cache_flushed_count, get_wms_total_download_size
 */
    RL2_DECLARE int get_wms_cache_current_size (rl2WmsCachePtr handle);

/**
 Return the current total number of cache-hit events from a WMS-Cache object.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()

 \return the current total number of cache-hit events from a WMS-Cache object.
 
 \sa create_wms_cache, destroy_wms_cache, reset_wms_cache, get_wms_cache_max_size, 
 set_wms_cache_max_size, get_wms_cache_items_count, get_wms_cache_current_size, 
 get_wms_cache_miss_count, get_wms_cache_flushed_count, get_wms_total_download_size
 */
    RL2_DECLARE int get_wms_cache_hit_count (rl2WmsCachePtr handle);

/**
 Return the current total number of cache-miss events from a WMS-Cache object.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()

 \return the current total number of cache-miss events from a WMS-Cache object.
 
 \sa create_wms_cache, destroy_wms_cache, reset_wms_cache, get_wms_cache_max_size, 
 set_wms_cache_max_size, get_wms_cache_items_count, get_wms_cache_current_size, 
 get_wms_cache_hit_count, get_wms_cache_flushed_count, get_wms_total_download_size
 */

    RL2_DECLARE int get_wms_cache_miss_count (rl2WmsCachePtr handle);

/**
 Return the current size of memory allocations previously used by a WMS-Cache
 object but now definitely released.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()

 \return the current size of memory allocations (in bytes) previously used 
 by a WMS-Cache object but now definitely released.
 
 \sa create_wms_cache, destroy_wms_cache, reset_wms_cache, get_wms_cache_max_size, 
 set_wms_cache_max_size, get_wms_cache_items_count, get_wms_cache_current_size, 
 get_wms_cache_hit_count, get_wms_cache_miss_count, get_wms_total_download_size
 */
    RL2_DECLARE int get_wms_cache_flushed_count (rl2WmsCachePtr handle);

/**
 Return the total size (in bytes) of all cache items since the beginning of
 the file cycle of some WMS-Cache.

 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache()

 \return the total size (in bytes) of all cache items since the beginning of
 the file cycle of some WMS-Cache object.
 
 \sa create_wms_cache, destroy_wms_cache, reset_wms_cache, get_wms_cache_max_size, 
 set_wms_cache_max_size, get_wms_cache_items_count, get_wms_cache_current_size, 
 get_wms_cache_hit_count, get_wms_cache_miss_count, get_wms_cache_flushed_count
 */

    RL2_DECLARE double get_wms_total_download_size (rl2WmsCachePtr handle);

/**
 Performs a WMS GetMap request - HTTP GET
 
 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache() 
 \param url the WebServive base URL.
 \param proxy an optional HTTP Proxy string: could be eventually NULL.
 \param version a string identifying the version of the WMS protocol to be used.
 \param layer name of the requested WMS-Layer.
 \param a string identifying the CRS/SRS.
 \param swap_xy a boolean value used to select between normal [XY] or flipped
 [LatLon] axes ordering.
 \param minx BoundingBox: X min coordinate.
 \param miny BoundingBox: Y min coordinate.
 \param maxx BoundingBox: X max coordinate.
 \param maxy BoundingBox: Y max coordinate.
 \param width horizontal dimension (in pixels) of the requested image.
 \param height vertical dimension (in pixels) of the requested image.
 \param style a string identifying some SLD Style; could be eventually NULL.
 \param format a string indentifying the MIME type of the requested image.
 \param opaque a boolean valued used to select if the requested image
 should be either opaque or transparent.
 \param from_cache boolean value: if TRUE simply an attempt to retrieve
 the requested image from cached data will be performed.\n
 Otherwise a full HTTP request will be forwarded for any uncached request.
 
 \return a pointer to an RGBA buffer containing the requested image:
 NULL if any error is encountered.
 
 \sa do_wms_GetMap_post, do_wms_GetMap_blob, do_wms_GetMap_TileService_get, 
 do_wms_GetMap_TileService_post, do_wms_GetFeatureInfo_get,
 do_wms_GetFeatureInfo_post
 
 \note the returned RGBA corresponds to dynamically allocated memory,
 and thus requires to be deallocated before or after.\n
 An eventual error message returned via err_msg requires to be deallocated
 by invoking free().
 */
    RL2_DECLARE unsigned char *do_wms_GetMap_get (rl2WmsCachePtr handle,
						  const char *url,
						  const char *proxy,
						  const char *version,
						  const char *layer,
						  const char *crs, int swap_xy,
						  double minx, double miny,
						  double maxx, double maxy,
						  int width, int height,
						  const char *style,
						  const char *format,
						  int opaque, int from_cache);

/**
 Performs a WMS GetMap request - HTTP POST
 
 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache() 
 \param url the WebServive base URL.
 \param proxy an optional HTTP Proxy string: could be eventually NULL.
 \param version a string identifying the version of the WMS protocol to be used.
 \param layer name of the requested WMS-Layer.
 \param a string identifying the CRS/SRS
 \param swap_xy a boolean value used to select between normal [XY] or flipped
 [LatLon] axes ordering.
 \param minx BoundingBox: X min coordinate.
 \param miny BoundingBox: Y min coordinate.
 \param maxx BoundingBox: X max coordinate.
 \param maxy BoundingBox: Y max coordinate.
 \param width horizontal dimension (in pixels) of the requested image.
 \param height vertical dimension (in pixels) of the requested image.
 \param style a string identifying some SLD Style; could be eventually NULL.
 \param format a string indentifying the MIME type of the requested image.
 \param opaque a boolean valued used to select if the requested image
 should be either opaque or transparent.
 \param from_cache boolean value: if TRUE simply an attempt to retrieve
 the requested image from cached data will be performed.\n
 Otherwise a full HTTP request will be forwarded for any uncached request.
 
 \return a pointer to an RGBA buffer containing the requested image:
 NULL if any error is encountered.\n
 <bCurrently unimplemented: will always return NULL</b>
 
 \sa do_wms_GetMap_get, do_wms_GetMap_blob, do_wms_GetMap_TileService_get, 
 do_wms_GetMap_TileService_post, do_wms_GetFeatureInfo_get,
 do_wms_GetFeatureInfo_post
 
 \note the returned RGBA corresponds to dynamically allocated memory,
 and thus requires to be deallocated before or after.\n
 An eventual error message returned via err_msg requires to be deallocated
 by invoking free().
 */
    RL2_DECLARE unsigned char *do_wms_GetMap_post (rl2WmsCachePtr handle,
						   const char *url,
						   const char *proxy,
						   const char *version,
						   const char *layer,
						   const char *crs, int swap_xy,
						   double minx, double miny,
						   double maxx, double maxy,
						   int width, int height,
						   const char *style,
						   const char *format,
						   int opaque, int from_cache);

/**
 Performs a WMS GetMap request - BLOB result (Image)
 
 \param url the WebServive base URL.
 \param version a string identifying the version of the WMS protocol to be used.
 \param layer name of the requested WMS-Layer.
 \param swap_xy a boolean value used to select between normal [XY] or flipped
 [LatLon] axes ordering.
 \param srs BoundingBox: SRS value (e.g. EPSG:3003)
 \param minx BoundingBox: X min coordinate.
 \param miny BoundingBox: Y min coordinate.
 \param maxx BoundingBox: X max coordinate.
 \param maxy BoundingBox: Y max coordinate.
 \param width horizontal dimension (in pixels) of the requested image.
 \param height vertical dimension (in pixels) of the requested image.
 \param style a string identifying some SLD Style; could be eventually NULL.
 \param format a string indentifying the MIME type of the requested image.
 \param opaque a boolean valued used to select if the requested image
 should be either opaque or transparent.
 \param bg_color the HEX-RGB background color (e.g. 0xff0080)
 \param blob_size the size (in bytes) of the returned BLOB.\n
 
 \return a pointer to a BLOB buffer containing the requested image:
 NULL if any error is encountered.
 
 \sa do_wms_GetMap_get, do_wms_GetMap_post, do_wms_GetMap_TileService_get, 
 do_wms_GetMap_TileService_post, do_wms_GetFeatureInfo_get,
 do_wms_GetFeatureInfo_post
 
 \note the returned BLOB corresponds to dynamically allocated memory,
 and thus requires to be deallocated before or after.
 */
    RL2_DECLARE unsigned char *do_wms_GetMap_blob (const char *url,
						   const char *version,
						   const char *layer,
						   int swap_xy, const char *srs,
						   double minx, double miny,
						   double maxx, double maxy,
						   int width, int height,
						   const char *style,
						   const char *format,
						   int opaque,
						   const char *bg_color,
						   int *blob_size);


/**
 Performs a WMS GetMap [TileService] request - HTTP GET
 
 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache() 
 \param url full TileService GetMap request URL.
 \param proxy an optional HTTP Proxy string: could be eventually NULL.
 \param width horizontal dimension (in pixels) of the requested image.
 \param height vertical dimension (in pixels) of the requested image.
 \param from_cache boolean value: if TRUE simply an attempt to retrieve
 the requested image from cached data will be performed.\n
 Otherwise a full HTTP request will be forwarded for any uncached request.
 
 \return a pointer to an RGBA buffer containing the requested image:
 NULL if any error is encountered.
 
 \sa do_wms_GetMap_get, do_wms_GetMap_post, do_wms_GetMap_blob, 
 do_wms_GetMap_TileService_post, do_wms_GetFeatureInfo_get,
 do_wms_GetFeatureInfo_post
 
 \note the returned RGBA corresponds to dynamically allocated memory,
 and thus requires to be deallocated before or after.\n
 An eventual error message returned via err_msg requires to be deallocated
 by invoking free().
 */
    RL2_DECLARE unsigned char *do_wms_GetMap_TileService_get (rl2WmsCachePtr
							      handle,
							      const char *url,
							      const char *proxy,
							      int width,
							      int height,
							      int from_cache);

/**
 Performs a WMS GetMap [TileService] request - HTTP POST
 
 \param handle the pointer to a valid WMS-Cache returned by a previous call
 to create_wms_cache() 
 \param url full TileService GetMap request URL.
 \param proxy an optional HTTP Proxy string: could be eventually NULL.
 \param width horizontal dimension (in pixels) of the requested image.
 \param height vertical dimension (in pixels) of the requested image.
 \param from_cache boolean value: if TRUE simply an attempt to retrieve
 the requested image from cached data will be performed.\n
 Otherwise a full HTTP request will be forwarded for any uncached request.
 
 \return a pointer to an RGBA buffer containing the requested image:
 NULL if any error is encountered.\n
 <bCurrently unimplemented: will always return NULL</b>
 
 \sa do_wms_GetMap_get, do_wms_GetMap_post, do_wms_GetMap_blob, 
 do_wms_GetMap_TileService_get, do_wms_GetFeatureInfo_get,
 do_wms_GetFeatureInfo_post
 
 \note the returned RGBA corresponds to dynamically allocated memory,
 and thus requires to be deallocated before or after.\n
 An eventual error message returned via err_msg requires to be deallocated
 by invoking free().
 */
    RL2_DECLARE unsigned char *do_wms_GetMap_TileService_post (rl2WmsCachePtr
							       handle,
							       const char *url,
							       const char
							       *proxy,
							       int width,
							       int height,
							       int from_cache);

/**
 Performs a WMS GetFeatureInfo request - HTTP GET
 
 \param url the WebServive base URL.
 \param proxy an optional HTTP Proxy string: could be eventually NULL.
 \param version a string identifying the version of the WMS protocol to be used.
 \param format a string indentifying the MIME type of the requested image.
 \param layer name of the requested WMS-Layer.
 \param a string identifying the CRS/SRS
 \param swap_xy a boolean value used to select between normal [XY] or flipped
 [LatLon] axes ordering.
 \param minx BoundingBox: X min coordinate.
 \param miny BoundingBox: Y min coordinate.
 \param maxx BoundingBox: X max coordinate.
 \param maxy BoundingBox: Y max coordinate.
 \param width horizontal dimension (in pixels) of the requested image.
 \param height vertical dimension (in pixels) of the requested image.
 \param img_x X coordinate (in pixels) of the point to be queryied.
 \param img_y Y coordinate (in pixels) of the point to be queryied.
 
 \return a pointer to a WMS-FeatureCollection object:
 NULL if any error is encountered or if no result is available.
 
 \sa do_wms_GetMap_get, do_wms_GetMap_post, do_wms_GetMap_blob, do_wms_GetMap_TileService_get, 
 do_wms_GetMap_TileService_post, do_wms_GetFeatureInfo_post, 
 destroy_wms_feature_collection, wms_feature_collection_parse_geometries,
 get_wms_feature_members_count
 
 \note the returned WMS-FeatureCollection corresponds to dynamically allocated memory,
 and thus requires to be deallocated before or after by invoking destroy_wms_feature_collection().\n
 An eventual error message returned via err_msg requires to be deallocated
 by invoking free().
 */
    RL2_DECLARE rl2WmsFeatureCollectionPtr
	do_wms_GetFeatureInfo_get (const char *url,
				   const char *proxy,
				   const char *version,
				   const char *format,
				   const char *layer,
				   const char *crs,
				   int swap_xy,
				   double minx,
				   double miny,
				   double maxx,
				   double maxy,
				   int width, int height, int img_x, int img_y);

/**
 Performs a WMS GetFeatureInfo request - HTTP POST
 
 \param url the WebServive base URL.
 \param proxy an optional HTTP Proxy string: could be eventually NULL.
 \param version a string identifying the version of the WMS protocol to be used.
 \param format a string indentifying the MIME type of the requested image.
 \param layer name of the requested WMS-Layer.
 \param a string identifying the CRS/SRS
 \param swap_xy a boolean value used to select between normal [XY] or flipped
 [LatLon] axes ordering.
 \param minx BoundingBox: X min coordinate.
 \param miny BoundingBox: Y min coordinate.
 \param maxx BoundingBox: X max coordinate.
 \param maxy BoundingBox: Y max coordinate.
 \param width horizontal dimension (in pixels) of the requested image.
 \param height vertical dimension (in pixels) of the requested image.
 \param img_x X coordinate (in pixels) of the point to be queried.
 \param img_y Y coordinate (in pixels) of the point to be queried.
 
 \return a pointer to a WMS-FeatureCollection object:
 NULL if any error is encountered or if no result is available.\n
 <bCurrently unimplemented: will always return NULL</b>
 
 \sa do_wms_GetMap_get, do_wms_GetMap_post, do_wms_GetMap_blob, do_wms_GetMap_TileService_get, 
 do_wms_GetMap_TileService_post, do_wms_GetFeatureInfo_get, 
 destroy_wms_feature_collection, wms_feature_collection_parse_geometries,
 get_wms_feature_members_count
 
 \note the returned WMS-FeatureCollection corresponds to dynamically allocated memory,
 and thus requires to be deallocated before or after by invoking destroy_wms_feature_collection().\n
 An eventual error message returned via err_msg requires to be deallocated
 by invoking free().
 */
    RL2_DECLARE rl2WmsFeatureCollectionPtr
	do_wms_GetFeatureInfo_post (const char *url,
				    const char *proxy,
				    const char *version,
				    const char *format,
				    const char *layer,
				    const char *crs,
				    int swap_xy,
				    double minx,
				    double miny,
				    double maxx,
				    double maxy,
				    int width, int height,
				    int img_x, int img_y);

/**
 Destroys a WMS-FeatureCollection object freeing any allocated resource 

 \param handle the pointer to a valid WMS-FeatureCollection returned by a previous call
 to do_wms_GetFeatureInfo_get() or do_wms_GetFeatureInfo_post()
 
 \sa do_wms_GetFeatureInfo_get, do_wms_GetFeatureInfo_post,
 wms_feature_collection_parse_geometries, get_wms_feature_members_count, 
 get_wms_feature_member, get_wms_feature_attributes_count, 
 get_wms_feature_attribute_name, get_wms_feature_attribute_value,
 get_wms_feature_attribute_geometry
 */
    RL2_DECLARE void destroy_wms_feature_collection (rl2WmsFeatureCollectionPtr
						     handle);

/**
 Attempts to parse all GML Geometries from within a WMS-FeatureCollection object

 \param handle the pointer to a valid WMS-FeatureCollection returned by a previous call
 to do_wms_GetFeatureInfo_get() or do_wms_GetFeatureInfo_post()
 \param srid the SRID value of the current Map
 \param point_x X coordinate (in the Map CRS) identifying the queried Point.
 \param point_y Y coordinate (in the Map CRS) identifying the queried Point.
 \param sqlite handle to a valid SQLite connection - required in order to support
 coordinate re-projections based on ST_Transform().
 
 \sa do_wms_GetFeatureInfo_get, do_wms_GetFeatureInfo_post,
 destroy_wms_feature_collection, get_wms_feature_members_count, 
 get_wms_feature_member, get_wms_feature_attributes_count, 
 get_wms_feature_attribute_name, get_wms_feature_attribute_value, 
 get_wms_feature_attribute_geometry
 */
    RL2_DECLARE void
	wms_feature_collection_parse_geometries (rl2WmsFeatureCollectionPtr
						 handle, int srid,
						 double point_x, double point_y,
						 sqlite3 * sqlite);

/**
 Return the total count of WMS-FeatureMembers from within a WMS_FeatureCollection object

 \param handle the pointer to a valid WMS-FeatureCollection returned by a previous call
 to do_wms_GetFeatureInfo_get() or do_wms_GetFeatureInfo_post()

 \return the total count of WMS-FeatureMembers from within a WMS_FeatureCollection object.
 
 \sa do_wms_GetFeatureInfo_get, do_wms_GetFeatureInfo_post,
 destroy_wms_feature_collection, wms_feature_collection_parse_geometries, 
 get_wms_feature_member, get_wms_feature_attributes_count, 
 get_wms_feature_attribute_name, get_wms_feature_attribute_value, 
 get_wms_feature_attribute_geometry
 */
    RL2_DECLARE int get_wms_feature_members_count (rl2WmsFeatureCollectionPtr
						   handle);

/**
 Return a pointer referencing the Nth WMS-FeatureMember from within a WMS_FeatureCollection object

 \param handle the pointer to a valid WMS-FeatureCollection returned by a previous call
 to do_wms_GetFeatureInfo_get() or do_wms_GetFeatureInfo_post()
 \param index the relative index identifying the required FeatureMember (the first 
 Member supported by a WMS-FeatureCollection object has index ZERO). 

 \return a pointer referencing the Nth WMS-FeatureMember from within a WMS_FeatureCollection 
 object: NULL for empty/void collections or if any error occurs.
 
 \sa do_wms_GetFeatureInfo_get, do_wms_GetFeatureInfo_post,
 destroy_wms_feature_collection, wms_feature_collection_parse_geometries, 
 get_wms_feature_member, get_wms_feature_attributes_count, 
 get_wms_feature_attribute_name, get_wms_feature_attribute_value, 
 get_wms_feature_attribute_geometry
 */
    RL2_DECLARE rl2WmsFeatureMemberPtr
	get_wms_feature_member (rl2WmsFeatureCollectionPtr handle, int index);

/**
 Return a pointer the Name string of some WMS-FeatureMember object

 \param handle the pointer to a valid WMS-FeatureCollection returned by a previous call
 to do_wms_GetFeatureInfo_get() or do_wms_GetFeatureInfo_post()

 \return a pointer the Name string of some WMS-FeatureMember object:
 NULL if any error occurs.
 
 \sa do_wms_GetFeatureInfo_get, do_wms_GetFeatureInfo_post,
 destroy_wms_feature_collection, wms_feature_collection_parse_geometries, 
 get_wms_feature_member, get_wms_feature_attributes_count, 
 get_wms_feature_attribute_name, get_wms_feature_attribute_value, 
 get_wms_feature_attribute_geometry
 */
    RL2_DECLARE int get_wms_feature_attributes_count (rl2WmsFeatureMemberPtr
						      handle);

/**
 Return a pointer the Nth AttributeName string from within some WMS-FeatureMember object

 \param handle the pointer to a valid WMS-FeatureCollection returned by a previous call
 to do_wms_GetFeatureInfo_get() or do_wms_GetFeatureInfo_post()
 \param index the relative index identifying the required Attribute (the first 
 Attribute supported by a WMS-FeatureMember object has index ZERO). 

 \return a pointer the Nth AttributeName string of some WMS-FeatureMember object:
 NULL if any error occurs.
 
 \sa do_wms_GetFeatureInfo_get, do_wms_GetFeatureInfo_post,
 destroy_wms_feature_collection, wms_feature_collection_parse_geometries, 
 get_wms_feature_member, get_wms_feature_attributes_count, 
 get_wms_feature_attributes_count, get_wms_feature_attribute_value, 
 get_wms_feature_attribute_geometry
 */
    RL2_DECLARE const char
	*get_wms_feature_attribute_name (rl2WmsFeatureMemberPtr handle,
					 int index);

/**
 Return a pointer the Nth AttributeValue string from within some WMS-FeatureMember object

 \param handle the pointer to a valid WMS-FeatureCollection returned by a previous call
 to do_wms_GetFeatureInfo_get() or do_wms_GetFeatureInfo_post()
 \param index the relative index identifying the required Attribute (the first 
 Attribute supported by a WMS-FeatureMember object has index ZERO).

 \return a pointer the Nth AttributeValue string of some WMS-FeatureMember object:
 NULL if any error occurs. (please note: an AttributeValue could eventually correspond
 to a NULL value by itself) 
 
 \sa do_wms_GetFeatureInfo_get, do_wms_GetFeatureInfo_post,
 destroy_wms_feature_collection, wms_feature_collection_parse_geometries, 
 get_wms_feature_member, get_wms_feature_attributes_count, 
 get_wms_feature_attributes_count, get_wms_feature_attribute_name, 
 get_wms_feature_attribute_geometry
 */
    RL2_DECLARE const char
	*get_wms_feature_attribute_value (rl2WmsFeatureMemberPtr handle,
					  int index);

/**
 Retrieves a SpatiaLite's BLOB binery geometry corresponding to the Nth
 AttributeValue from within some WMS-FeatureMember object

 \param handle the pointer to a valid WMS-FeatureCollection returned by a previous call
 to do_wms_GetFeatureInfo_get() or do_wms_GetFeatureInfo_post()
 \param index the relative index identifying the required Attribute (the first 
 Attribute supported by a WMS-FeatureMember object has index ZERO). 
 \param blob on completion will point to the BLOB binary Geometry.
 \param blob_soze on completion the variable referenced by this
 pointer will contain the size (in bytes) of BLOB. Geometry.

 \return RL2_OK on success: RL2_ERROR on failure.
 
 \sa do_wms_GetFeatureInfo_get, do_wms_GetFeatureInfo_post,
 destroy_wms_feature_collection, wms_feature_collection_parse_geometries, 
 get_wms_feature_member, get_wms_feature_attributes_count, 
 get_wms_feature_attributes_count, get_wms_feature_attribute_name, 
 get_wms_feature_attribute_value

 \note the returned BLOB Geometry simply is a reference, and must
 absolutely not be destroyed by directly calling free().
 */
    RL2_DECLARE int
	get_wms_feature_attribute_blob_geometry (rl2WmsFeatureMemberPtr handle,
						 int index,
						 const unsigned char **blob,
						 int *blob_size);

#ifdef __cplusplus
}
#endif

#endif				/* RL2WMS_H */
