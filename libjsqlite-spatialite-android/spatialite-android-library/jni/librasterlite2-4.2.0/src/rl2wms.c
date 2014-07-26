/*

 rl2wms -- WMS related functions

 version 0.1, 2013 July 28

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
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>
#include <libxml/parser.h>

#include "rasterlite2/sqlite.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2wms.h"

#define WMS_FORMAT_UNKNOWN	0
#define WMS_FORMAT_GIF		1
#define WMS_FORMAT_PNG		2
#define WMS_FORMAT_PNG8		3
#define WMS_FORMAT_PNG24	4
#define WMS_FORMAT_PNG32	5
#define WMS_FORMAT_JPEG		6
#define WMS_FORMAT_TIFF		7

typedef struct wmsMemBufferStruct
{
/* a struct handling a dynamically growing output buffer */
    unsigned char *Buffer;
    size_t WriteOffset;
    size_t BufferSize;
    int Error;
} wmsMemBuffer;
typedef wmsMemBuffer *wmsMemBufferPtr;

typedef struct wmsCachedCapabilitiesStruct
{
/* a WMS Cached GetCapabilities response */
    char *Url;
    unsigned char *Response;
    struct wmsCachedCapabilitiesStruct *Next;
} wmsCachedCapabilities;
typedef wmsCachedCapabilities *wmsCachedCapabilitiesPtr;

typedef struct wmsCachedItemStruct
{
/* a WMS Cached Item */
    char *Url;
    time_t Time;
    int Size;
    unsigned char *Item;
    int ImageFormat;
    struct wmsCachedItemStruct *Prev;
    struct wmsCachedItemStruct *Next;
} wmsCachedItem;
typedef wmsCachedItem *wmsCachedItemPtr;

typedef struct wmsCacheStruct
{
/* a struct implementing a WMS image cache */
    int MaxSize;
    int CurrentSize;
    wmsCachedCapabilitiesPtr FirstCapab;
    wmsCachedCapabilitiesPtr LastCapab;
    wmsCachedItemPtr First;
    wmsCachedItemPtr Last;
    int NumCachedItems;
    wmsCachedItemPtr *SortedByUrl;
    wmsCachedItemPtr *SortedByTime;
    int HitCount;
    int MissCount;
    int FlushedCount;
    double TotalDownload;
} wmsCache;
typedef wmsCache *wmsCachePtr;

typedef struct wmsFormatStruct
{
/* a struct wrapping a WMS Format */
    int FormatCode;
    char *Format;
    struct wmsFormatStruct *next;
} wmsFormat;
typedef wmsFormat *wmsFormatPtr;

typedef struct wmsCrsStruct
{
/* a struct wrapping a WMS CRS */
    char *Crs;
    struct wmsCrsStruct *next;
} wmsCrs;
typedef wmsCrs *wmsCrsPtr;

typedef struct wmsBBoxStruct
{
/* a struct wrapping a WMS Bounding Box */
    char *Crs;
    double MinX;
    double MaxX;
    double MinY;
    double MaxY;
    struct wmsBBoxStruct *next;
} wmsBBox;
typedef wmsBBox *wmsBBoxPtr;

typedef struct wmsStyleStruct
{
/* a struct wrapping a WMS Style */
    char *Name;
    char *Title;
    char *Abstract;
    struct wmsStyleStruct *next;
} wmsStyle;
typedef wmsStyle *wmsStylePtr;

typedef struct wmsUrlArgumentStruct
{
/* struct wrapping a WMS URL argument */
    char *argName;
    char *argValue;
    struct wmsUrlArgumentStruct *next;
} wmsUrlArgument;
typedef wmsUrlArgument *wmsUrlArgumentPtr;

typedef struct wmsTilePatternStruct
{
/* struct wrapping a WMS TilePattern */
    char *Pattern;
    char *Format;
    char *SRS;
    char *Style;
    int TileWidth;
    int TileHeight;
    double TileBaseX;
    double TileBaseY;
    double TileExtentX;
    double TileExtentY;
    wmsUrlArgumentPtr first;
    wmsUrlArgumentPtr last;
    struct wmsTilePatternStruct *next;
} wmsTilePattern;
typedef wmsTilePattern *wmsTilePatternPtr;

typedef struct wmsTiledLayerStruct
{
/* a struct wrapping a WMS Tiled Layer */
    char *Name;
    char *Title;
    char *Abstract;
    double MinLat;
    double MaxLat;
    double MinLong;
    double MaxLong;
    char *Pad;
    char *Bands;
    char *DataType;
    wmsTilePatternPtr firstPattern;
    wmsTilePatternPtr lastPattern;
    struct wmsTiledLayerStruct *firstChild;
    struct wmsTiledLayerStruct *lastChild;
    struct wmsTiledLayerStruct *next;
} wmsTiledLayer;
typedef wmsTiledLayer *wmsTiledLayerPtr;

typedef struct wmsLayerStruct
{
/* a struct wrapping a WMS Layer */
    int Queryable;
    int Opaque;
    char *Name;
    char *Title;
    char *Abstract;
    double MinScaleDenominator;
    double MaxScaleDenominator;
    double MinLat;
    double MaxLat;
    double MinLong;
    double MaxLong;
    wmsBBoxPtr firstBBox;
    wmsBBoxPtr lastBBox;
    wmsCrsPtr firstCrs;
    wmsCrsPtr lastCrs;
    wmsStylePtr firstStyle;
    wmsStylePtr lastStyle;
    struct wmsLayerStruct *Parent;
    struct wmsLayerStruct *firstLayer;
    struct wmsLayerStruct *lastLayer;
    struct wmsLayerStruct *next;
} wmsLayer;
typedef wmsLayer *wmsLayerPtr;

typedef struct wmsCapabilitiesStruct
{
/* a struct wrapping a WMS Capabilities answer */
    char *Version;
    char *Name;
    char *Title;
    char *Abstract;
    char *GetMapURLGet;
    char *GetMapURLPost;
    char *GetFeatureInfoURLGet;
    char *GetFeatureInfoURLPost;
    char *GetTileServiceURLGet;
    char *GetTileServiceURLPost;
    char *GmlMimeType;
    char *XmlMimeType;
    char *ContactPerson;
    char *ContactOrganization;
    char *ContactPosition;
    char *PostalAddress;
    char *City;
    char *StateProvince;
    char *PostCode;
    char *Country;
    char *VoiceTelephone;
    char *FaxTelephone;
    char *EMailAddress;
    char *Fees;
    char *AccessConstraints;
    int LayerLimit;
    int MaxWidth;
    int MaxHeight;
    wmsFormatPtr firstFormat;
    wmsFormatPtr lastFormat;
    wmsLayerPtr firstLayer;
    wmsLayerPtr lastLayer;
    char *TileServiceName;
    char *TileServiceTitle;
    char *TileServiceAbstract;
    wmsTiledLayerPtr firstTiled;
    wmsTiledLayerPtr lastTiled;
} wmsCapabilities;
typedef wmsCapabilities *wmsCapabilitiesPtr;

typedef struct wmsFeatureAttributeStruct
{
/* a struct wrapping a GML FeatureAtrribute */
    char *name;
    char *value;
    gaiaGeomCollPtr geometry;
    struct wmsFeatureAttributeStruct *next;
} wmsFeatureAttribute;
typedef wmsFeatureAttribute *wmsFeatureAttributePtr;

typedef struct wmsFeatureMemberStruct
{
/* a struct wrapping a GML FeatureMember */
    char *layer_name;
    wmsFeatureAttributePtr first;
    wmsFeatureAttributePtr last;
    struct wmsFeatureMemberStruct *next;
} wmsFeatureMember;
typedef wmsFeatureMember *wmsFeatureMemberPtr;

typedef struct wmsFeatureCollectionStruct
{
/* a struct wrapping a WMS FeatureInfo answer */
    wmsFeatureMemberPtr first;
    wmsFeatureMemberPtr last;
} wmsFeatureCollection;
typedef wmsFeatureCollection *wmsFeatureCollectionPtr;

typedef struct wmsSinglePartResponseStruct
{
/* a struct wrapping a single part body response */
    char *body;
    struct wmsSinglePartResponseStruct *next;
} wmsSinglePartResponse;
typedef wmsSinglePartResponse *wmsSinglePartResponsePtr;

typedef struct wmsMultipartCollectionStruct
{
/* a struct wrapping a Multipart HTTP response */
    wmsSinglePartResponse *first;
    wmsSinglePartResponse *last;
} wmsMultipartCollection;
typedef wmsMultipartCollection *wmsMultipartCollectionPtr;

static wmsCachedItemPtr
wmsAllocCachedItem (const char *url, const unsigned char *item, int size,
		    const char *image_format)
{
/* creating a WMS Cached Item */
    int len;
    time_t xtime;
    wmsCachedItemPtr ptr = malloc (sizeof (wmsCachedItem));
    len = strlen (url);
    ptr->Url = malloc (len + 1);
    strcpy (ptr->Url, url);
    time (&xtime);
    ptr->Time = xtime;
    ptr->Size = size;
    ptr->Item = malloc (size);
    memcpy (ptr->Item, item, size);
    ptr->ImageFormat = WMS_FORMAT_UNKNOWN;
    if (strcmp (image_format, "image/gif") == 0)
	ptr->ImageFormat = WMS_FORMAT_GIF;
    if (strcmp (image_format, "image/png") == 0)
	ptr->ImageFormat = WMS_FORMAT_PNG;
    if (strcmp (image_format, "image/jpeg") == 0)
	ptr->ImageFormat = WMS_FORMAT_JPEG;
    ptr->Prev = NULL;
    ptr->Next = NULL;
    return ptr;
}

static void
wmsFreeCachedItem (wmsCachedItemPtr ptr)
{
/* memory cleanup - destroying a WMS Cached Item */
    if (ptr == NULL)
	return;
    if (ptr->Url != NULL)
	free (ptr->Url);
    if (ptr->Item != NULL)
	free (ptr->Item);
    free (ptr);
}

static wmsCachedCapabilitiesPtr
wmsAllocCachedCapabilities (const char *url, const unsigned char *response,
			    int size)
{
/* creating a WMS Cached GetCapabilities response */
    wmsCachedCapabilitiesPtr ptr = malloc (sizeof (wmsCachedCapabilities));
    int len = strlen (url);
    ptr->Url = malloc (len + 1);
    strcpy (ptr->Url, url);
    ptr->Response = malloc (size + 1);
    memcpy (ptr->Response, response, size);
    *(ptr->Response + size) = '\0';
    ptr->Next = NULL;
    return ptr;
}

static void
wmsFreeCachedCapabilities (wmsCachedCapabilitiesPtr ptr)
{
/* memory cleanup - destroying a WMS Cached GetCapabilities Response */
    if (ptr == NULL)
	return;
    if (ptr->Url != NULL)
	free (ptr->Url);
    if (ptr->Response != NULL)
	free (ptr->Response);
    free (ptr);
}

static wmsCachePtr
wmsAllocCache (void)
{
/* creating a WMS Cache */
    wmsCachePtr cache = malloc (sizeof (wmsCache));
    cache->MaxSize = 64 * 1024 * 1024;
    cache->CurrentSize = 0;
    cache->FirstCapab = NULL;
    cache->LastCapab = NULL;
    cache->First = NULL;
    cache->Last = NULL;
    cache->NumCachedItems = 0;
    cache->SortedByUrl = NULL;
    cache->SortedByTime = NULL;
    cache->HitCount = 0;
    cache->MissCount = 0;
    cache->FlushedCount = 0;
    cache->TotalDownload = 0 - 0;
    return cache;
}

static void
wmsCacheReset (wmsCachePtr cache)
{
/* memory cleanup: flushing/resetting a WMS-Cache object */
    wmsCachedItemPtr pI;
    wmsCachedItemPtr pIn;
    wmsCachedCapabilitiesPtr pC;
    wmsCachedCapabilitiesPtr pCn;
    if (cache == NULL)
	return;

    pC = cache->FirstCapab;
    while (pC != NULL)
      {
	  pCn = pC->Next;
	  wmsFreeCachedCapabilities (pC);
	  pC = pCn;
      }
    pI = cache->First;
    while (pI != NULL)
      {
	  pIn = pI->Next;
	  wmsFreeCachedItem (pI);
	  pI = pIn;
      }
    if (cache->SortedByUrl != NULL)
	free (cache->SortedByUrl);
    if (cache->SortedByTime != NULL)
	free (cache->SortedByTime);
    cache->CurrentSize = 0;
    cache->First = NULL;
    cache->Last = NULL;
    cache->FirstCapab = NULL;
    cache->LastCapab = NULL;
    cache->NumCachedItems = 0;
    cache->HitCount = 0;
    cache->MissCount = 0;
    cache->FlushedCount = 0;
    cache->TotalDownload = 0.0;
    cache->SortedByUrl = NULL;
    cache->SortedByTime = NULL;
}

static void
wmsFreeCache (wmsCachePtr cache)
{
/* memory cleanup: freeing a WMS-Cache object */
    if (cache == NULL)
	return;

    wmsCacheReset (cache);
    free (cache);
}

static int
compare_time (const void *p1, const void *p2)
{
/* comparison function for QSort [Time] */
    wmsCachedItemPtr pI1 = *((wmsCachedItemPtr *) p1);
    wmsCachedItemPtr pI2 = *((wmsCachedItemPtr *) p2);
    if (pI1->Time == pI2->Time)
	return 0;
    if (pI1->Time > pI2->Time)
	return 1;
    return -1;
}

static void
wmsCacheUpdateTime (wmsCachePtr cache)
{
/* updating the SortedByTime array */
    wmsCachedItemPtr pI;
    int pos = 0;
    if (cache == NULL)
	return;
    if (cache->SortedByTime != NULL)
	free (cache->SortedByTime);
    cache->SortedByTime = NULL;
    if (cache->NumCachedItems <= 0)
	return;
    cache->SortedByTime =
	malloc (sizeof (wmsCachedItemPtr) * cache->NumCachedItems);
    pI = cache->First;
    while (pI != NULL)
      {
	  /* populating the array */
	  *(cache->SortedByTime + pos) = pI;
	  pos++;
	  pI = pI->Next;
      }
    qsort (cache->SortedByTime, cache->NumCachedItems,
	   sizeof (wmsCachedItemPtr), compare_time);
}

static int
compare_url (const void *p1, const void *p2)
{
/* comparison function for QSort [Time] */
    wmsCachedItemPtr pI1 = *((wmsCachedItemPtr *) p1);
    wmsCachedItemPtr pI2 = *((wmsCachedItemPtr *) p2);
    return strcmp (pI1->Url, pI2->Url);
}

static void
wmsCacheUpdate (wmsCachePtr cache)
{
/* updating the SortedByUrl array */
    wmsCachedItemPtr pI;
    int pos = 0;
    if (cache == NULL)
	return;
    if (cache->SortedByUrl != NULL)
	free (cache->SortedByUrl);
    cache->SortedByUrl = NULL;
    if (cache->NumCachedItems <= 0)
	return;
    cache->SortedByUrl =
	malloc (sizeof (wmsCachedItemPtr) * cache->NumCachedItems);
    pI = cache->First;
    while (pI != NULL)
      {
	  /* populating the array */
	  *(cache->SortedByUrl + pos) = pI;
	  pos++;
	  pI = pI->Next;
      }
    qsort (cache->SortedByUrl, cache->NumCachedItems, sizeof (wmsCachedItemPtr),
	   compare_url);
}

static void
wmsCacheSqueeze (wmsCachePtr cache, int limit)
{
/* squeezing the WMS Cache */
    int i;
    int max;
    if (cache == NULL)
	return;
    if (cache->CurrentSize < limit)
	return;
    wmsCacheUpdateTime (cache);
    max = cache->NumCachedItems;
    for (i = 0; i < max; i++)
      {
	  wmsCachedItemPtr pI = *(cache->SortedByTime + i);
	  if (pI->Prev != NULL)
	      pI->Prev->Next = pI->Next;
	  if (pI->Next != NULL)
	      pI->Next->Prev = pI->Prev;
	  if (cache->First == pI)
	      cache->First = pI->Next;
	  if (cache->Last == pI)
	      cache->Last = pI->Prev;
	  wmsFreeCachedItem (pI);
	  cache->NumCachedItems -= 1;
	  cache->CurrentSize -= pI->Size;
	  cache->FlushedCount += 1;
	  if (cache->CurrentSize < limit)
	      break;
      }
    if (cache->SortedByTime != NULL)
	free (cache->SortedByTime);
    cache->SortedByTime = NULL;
}

static void
wmsAddCachedCapabilities (wmsCachePtr cache, const char *url,
			  const unsigned char *response, int size)
{
/* adding a new WMS Cached GetCapabilitiesItem */
    wmsCachedCapabilitiesPtr ptr;
    if (cache == NULL)
	return;
    ptr = wmsAllocCachedCapabilities (url, response, size);
    if (cache->FirstCapab == NULL)
	cache->FirstCapab = ptr;
    if (cache->LastCapab != NULL)
	cache->LastCapab->Next = ptr;
    cache->LastCapab = ptr;
    cache->TotalDownload += (double) size;
}


static void
wmsAddCachedItem (wmsCachePtr cache, const char *url, const unsigned char *item,
		  int size, const char *image_format)
{
/* adding a new WMS Cached Item */
    wmsCachedItemPtr ptr;
    if (cache == NULL)
	return;
    if (cache->CurrentSize + size > cache->MaxSize)
	wmsCacheSqueeze (cache, cache->MaxSize - size);
    ptr = wmsAllocCachedItem (url, item, size, image_format);
    ptr->Prev = cache->Last;
    if (cache->First == NULL)
	cache->First = ptr;
    if (cache->Last != NULL)
	cache->Last->Next = ptr;
    cache->Last = ptr;
    cache->NumCachedItems += 1;
    cache->CurrentSize += size;
    cache->TotalDownload += (double) size;
    wmsCacheUpdate (cache);
}

static wmsCachedCapabilitiesPtr
getWmsCachedCapabilities (wmsCachePtr cache, const char *url)
{
/* attempting to retrieve a cached GetCapabilities Response by URL */
    wmsCachedCapabilitiesPtr capab;
    if (cache == NULL)
	return NULL;
    capab = cache->FirstCapab;
    while (capab != NULL)
      {
	  if (strcmp (capab->Url, url) == 0)
	      return capab;
	  capab = capab->Next;
      }
    return NULL;
}

static wmsCachedItemPtr
getWmsCachedItem (wmsCachePtr cache, const char *url)
{
/* attempting to retrieve a cached item by URL */
    wmsCachedItem item;
    wmsCachedItemPtr p_item = &item;
    wmsCachedItemPtr found;
    void *x;
    item.Url = (char *) url;
    if (cache == NULL)
	return NULL;
    if (cache->NumCachedItems <= 0)
	return NULL;
    if (cache->SortedByUrl == NULL)
	return NULL;
    x = bsearch (&p_item, cache->SortedByUrl, cache->NumCachedItems,
		 sizeof (wmsCachedItemPtr), compare_url);
    if (x == NULL)
      {
	  cache->MissCount += 1;
	  return NULL;
      }
    found = *((wmsCachedItemPtr *) x);
    cache->HitCount += 1;
    return found;
}

RL2_DECLARE rl2WmsCachePtr
create_wms_cache (void)
{
/* creating a WMS Cache */
    wmsCachePtr cache = wmsAllocCache ();
    return (rl2WmsCachePtr) cache;
}

RL2_DECLARE void
destroy_wms_cache (rl2WmsCachePtr handle)
{
/* memory cleanup: freeing a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return;
    wmsFreeCache (ptr);
}

RL2_DECLARE void
reset_wms_cache (rl2WmsCachePtr handle)
{
/* memory cleanup: flushing/resetting a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return;
    wmsCacheReset (ptr);
}

RL2_DECLARE int
get_wms_cache_max_size (rl2WmsCachePtr handle)
{
/* returns the MAX-SIZE from a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return -1;
    return ptr->MaxSize;
}

RL2_DECLARE void
set_wms_cache_max_size (rl2WmsCachePtr handle, int size)
{
/* changes the MAX-SIZE for a WMS-Cache object */
    int min = 4 * 1024 * 1024;
    int max = 256 * 1024 * 1024;
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return;
    ptr->MaxSize = size;
    if (ptr->MaxSize < min)
	ptr->MaxSize = min;
    if (ptr->MaxSize > max)
	ptr->MaxSize = max;
    if (ptr->CurrentSize > ptr->MaxSize)
      {
	  wmsCacheSqueeze (ptr, ptr->MaxSize);
	  wmsCacheUpdate (ptr);
      }
}

RL2_DECLARE int
get_wms_cache_items_count (rl2WmsCachePtr handle)
{
/* returns the CURRENT-SIZE from a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return -1;
    return ptr->NumCachedItems;
}

RL2_DECLARE int
get_wms_cache_current_size (rl2WmsCachePtr handle)
{
/* returns the CURRENT-SIZE from a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return -1;
    return ptr->CurrentSize;
}

RL2_DECLARE int
get_wms_cache_hit_count (rl2WmsCachePtr handle)
{
/* returns the HIT-COUNT from a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return -1;
    return ptr->HitCount;
}

RL2_DECLARE int
get_wms_cache_miss_count (rl2WmsCachePtr handle)
{
/* returns the MISS-COUNT from a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return -1;
    return ptr->MissCount;
}

RL2_DECLARE int
get_wms_cache_flushed_count (rl2WmsCachePtr handle)
{
/* returns the FLUSHED-COUNT from a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return -1;
    return ptr->FlushedCount;
}

RL2_DECLARE double
get_wms_total_download_size (rl2WmsCachePtr handle)
{
/* returns the TOTAL-DOWNLOAD size from a WMS-Cache object */
    wmsCachePtr ptr = (wmsCachePtr) handle;
    if (ptr == NULL)
	return -1;
    return ptr->TotalDownload;
}

static wmsFormatPtr
wmsAllocFormat (const char *format)
{
/* allocating an empty WMS Format object */
    int len;
    wmsFormatPtr fmt = malloc (sizeof (wmsFormat));
    fmt->FormatCode = WMS_FORMAT_UNKNOWN;
    if (strcmp (format, "image/jpeg") == 0)
	fmt->FormatCode = WMS_FORMAT_JPEG;
    if (strcmp (format, "image/tiff") == 0)
	fmt->FormatCode = WMS_FORMAT_TIFF;
    if (strcmp (format, "image/gif") == 0)
	fmt->FormatCode = WMS_FORMAT_GIF;
    if (strcmp (format, "image/png") == 0)
	fmt->FormatCode = WMS_FORMAT_PNG;
    if (strcmp (format, "image/png8") == 0)
	fmt->FormatCode = WMS_FORMAT_PNG8;
    if (strcmp (format, "image/png; mode=8bit") == 0)
	fmt->FormatCode = WMS_FORMAT_PNG8;
    if (strcmp (format, "image/png24") == 0)
	fmt->FormatCode = WMS_FORMAT_PNG24;
    if (strcmp (format, "image/png; mode=24bit") == 0)
	fmt->FormatCode = WMS_FORMAT_PNG24;
    if (strcmp (format, "image/png32") == 0)
	fmt->FormatCode = WMS_FORMAT_PNG32;
    if (strcmp (format, "image/png; mode=32bit") == 0)
	fmt->FormatCode = WMS_FORMAT_PNG32;
    len = strlen (format);
    fmt->Format = malloc (len + 1);
    strcpy (fmt->Format, format);
    fmt->next = NULL;
    return fmt;
}

static void
wmsFreeFormat (wmsFormatPtr fmt)
{
/* memory cleanup - destroying a WMS Format object */
    if (fmt == NULL)
	return;
    if (fmt->Format != NULL)
	free (fmt->Format);
    free (fmt);
}

static wmsCrsPtr
wmsAllocCrs (const char *crs_str)
{
/* allocating a WMS CRS object */
    int len;
    wmsCrsPtr crs = malloc (sizeof (wmsCrs));
    crs->Crs = NULL;
    if (crs != NULL)
      {
	  len = strlen (crs_str);
	  crs->Crs = malloc (len + 1);
	  strcpy (crs->Crs, crs_str);
      }
    crs->next = NULL;
    return crs;
}

static void
wmsFreeCrs (wmsCrsPtr crs)
{
/* memory cleanup - destroying a WMS CRS object */
    if (crs == NULL)
	return;
    if (crs->Crs != NULL)
	free (crs->Crs);
    free (crs);
}

static wmsBBoxPtr
wmsAllocBBox (const char *crs_str, double minx, double maxx, double miny,
	      double maxy)
{
/* allocating a WMS Bounding Box object */
    int len;
    wmsBBoxPtr bbox = malloc (sizeof (wmsBBox));
    bbox->Crs = NULL;
    if (bbox != NULL)
      {
	  len = strlen (crs_str);
	  bbox->Crs = malloc (len + 1);
	  strcpy (bbox->Crs, crs_str);
      }
    bbox->MinX = minx;
    bbox->MaxX = maxx;
    bbox->MinY = miny;
    bbox->MaxY = maxy;
    bbox->next = NULL;
    return bbox;
}

static void
wmsFreeBBox (wmsBBoxPtr bbox)
{
/* memory cleanup - destroying a WMS BBOX object */
    if (bbox == NULL)
	return;
    if (bbox->Crs != NULL)
	free (bbox->Crs);
    free (bbox);
}

static wmsStylePtr
wmsAllocStyle (const char *name, const char *title, const char *abstract)
{
/* allocating a WMS Style object */
    int len;
    wmsStylePtr stl = malloc (sizeof (wmsStyle));
    stl->Name = NULL;
    stl->Title = NULL;
    stl->Abstract = NULL;
    if (name != NULL)
      {
	  len = strlen (name);
	  stl->Name = malloc (len + 1);
	  strcpy (stl->Name, name);
      }
    if (title != NULL)
      {
	  len = strlen (title);
	  stl->Title = malloc (len + 1);
	  strcpy (stl->Title, title);
      }
    if (abstract != NULL)
      {
	  len = strlen (abstract);
	  stl->Abstract = malloc (len + 1);
	  strcpy (stl->Abstract, abstract);
      }
    stl->next = NULL;
    return stl;
}

static void
wmsFreeStyle (wmsStylePtr stl)
{
/* memory cleanup - destroying a WMS Style object */
    if (stl == NULL)
	return;
    if (stl->Name != NULL)
	free (stl->Name);
    if (stl->Title != NULL)
	free (stl->Title);
    if (stl->Abstract != NULL)
	free (stl->Abstract);
    free (stl);
}

static wmsLayerPtr
wmsAllocLayer (const char *name, const char *title, const char *abstract,
	       wmsLayerPtr parent)
{
/* allocating a WMS Layer object */
    int len;
    wmsLayerPtr lyr = malloc (sizeof (wmsLayer));
    lyr->Queryable = -1;
    lyr->Opaque = -1;
    lyr->Name = NULL;
    lyr->Title = NULL;
    lyr->Abstract = NULL;
    if (name != NULL)
      {
	  len = strlen (name);
	  lyr->Name = malloc (len + 1);
	  strcpy (lyr->Name, name);
      }
    if (title != NULL)
      {
	  len = strlen (title);
	  lyr->Title = malloc (len + 1);
	  strcpy (lyr->Title, title);
      }
    if (abstract != NULL)
      {
	  len = strlen (abstract);
	  lyr->Abstract = malloc (len + 1);
	  strcpy (lyr->Abstract, abstract);
      }
    lyr->Parent = parent;
    lyr->MinScaleDenominator = DBL_MAX;
    lyr->MaxScaleDenominator = DBL_MAX;
    lyr->MinLat = DBL_MAX;
    lyr->MaxLat = DBL_MAX;
    lyr->MinLong = DBL_MAX;
    lyr->MaxLong = DBL_MAX;
    lyr->firstBBox = NULL;
    lyr->lastBBox = NULL;
    lyr->firstCrs = NULL;
    lyr->lastCrs = NULL;
    lyr->firstStyle = NULL;
    lyr->lastStyle = NULL;
    lyr->firstLayer = NULL;
    lyr->lastLayer = NULL;
    lyr->next = NULL;
    return lyr;
}

static void
wmsFreeLayer (wmsLayerPtr lyr)
{
/* memory cleanup - destroying a WMS Layer object */
    wmsCrsPtr pc;
    wmsCrsPtr pcn;
    wmsBBoxPtr pbb;
    wmsBBoxPtr pbbn;
    wmsStylePtr ps;
    wmsStylePtr psn;
    wmsLayerPtr pl;
    wmsLayerPtr pln;
    if (lyr == NULL)
	return;
    if (lyr->Name != NULL)
	free (lyr->Name);
    if (lyr->Title != NULL)
	free (lyr->Title);
    if (lyr->Abstract != NULL)
	free (lyr->Abstract);
    pc = lyr->firstCrs;
    while (pc != NULL)
      {
	  pcn = pc->next;
	  wmsFreeCrs (pc);
	  pc = pcn;
      }
    pbb = lyr->firstBBox;
    while (pbb != NULL)
      {
	  pbbn = pbb->next;
	  wmsFreeBBox (pbb);
	  pbb = pbbn;
      }
    ps = lyr->firstStyle;
    while (ps != NULL)
      {
	  psn = ps->next;
	  wmsFreeStyle (ps);
	  ps = psn;
      }
    pl = lyr->firstLayer;
    while (pl != NULL)
      {
	  pln = pl->next;
	  wmsFreeLayer (pl);
	  pl = pln;
      }
    free (lyr);
}

static wmsUrlArgumentPtr
wmsAllocUrlArgument (char *arg_name, char *arg_value)
{
/* allocating a WMS URL Argument object */
    wmsUrlArgumentPtr arg = malloc (sizeof (wmsUrlArgument));
    arg->argName = arg_name;
    arg->argValue = arg_value;
    arg->next = NULL;
    return arg;
}

static void
wmsFreeUrlArgument (wmsUrlArgumentPtr arg)
{
/* memory cleanup - destroying a WMS URL Argument object */
    if (arg == NULL)
	return;
    if (arg->argName != NULL)
	free (arg->argName);
    if (arg->argValue != NULL)
	free (arg->argValue);
    free (arg);
}

static void
add_url_argument (wmsTilePatternPtr ptr, const char *pattern)
{
/* adding an URL argument */
    char *name = NULL;
    char *value = NULL;
    const char *p = pattern;
    const char *p_eq = pattern;
    wmsUrlArgumentPtr arg;
    int len;

/* splitting arg Name and Value */
    while (1)
      {
	  if (*p == '=')
	      p_eq = p;
	  if (*p == '\0')
	      break;
	  p++;
      }

    len = p_eq - pattern;
    if (len > 0)
      {
	  /* arg Name */
	  name = malloc (len + 1);
	  memcpy (name, pattern, len);
	  *(name + len) = '\0';
      }
    len = strlen (p_eq + 1);
    if (len > 0)
      {
	  /* arg Value */
	  value = malloc (len + 1);
	  strcpy (value, p_eq + 1);
      }

    arg = wmsAllocUrlArgument (name, value);
    if (ptr->first == NULL)
	ptr->first = arg;
    if (ptr->last != NULL)
	ptr->last->next = arg;
    ptr->last = arg;
}

static void
parse_pattern_bbox (const char *value, double *minx, double *miny, double *maxx,
		    double *maxy)
{
/* parsing a BBOX arg [minx,miny,maxx,maxy] */
    int step = 0;
    const char *p_start = value;
    const char *p_end = value;
    *minx = DBL_MAX;
    *miny = DBL_MAX;
    *maxx = DBL_MAX;
    *maxy = DBL_MAX;

    while (1)
      {
	  if (*p_end == ',' || *p_end == '\0')
	    {
		int len = p_end - p_start;
		char *str = malloc (len + 1);
		memcpy (str, p_start, len);
		*(str + len) = '\0';
		if (step == 0)
		    *minx = atof (str);
		if (step == 1)
		    *miny = atof (str);
		if (step == 2)
		    *maxx = atof (str);
		if (step == 3)
		    *maxy = atof (str);
		step++;
		free (str);
		p_start = p_end + 1;
	    }
	  if (*p_end == '\0')
	      break;
	  p_end++;
      }
}

static void
parse_url_arguments (wmsTilePatternPtr ptr, const char *pattern)
{
/* parsing URL arguments */
    const char *p_start = pattern;
    const char *p_end = pattern;
    while (1)
      {
	  if (*p_end == '&' || *p_end == '\0')
	    {
		int len = p_end - p_start;
		char *arg = malloc (len + 1);
		memcpy (arg, p_start, len);
		*(arg + len) = '\0';
		add_url_argument (ptr, arg);
		free (arg);
		p_start = p_end + 1;
	    }
	  if (*p_end == '\0')
	      break;
	  p_end++;
      }
}

static wmsTilePatternPtr
wmsAllocTilePattern (char *pattern)
{
/* allocating a WMS TilePattern object */
    double minx;
    double miny;
    double maxx;
    double maxy;
    wmsUrlArgumentPtr pa;
    wmsTilePatternPtr ptr = malloc (sizeof (wmsTilePattern));
    ptr->Pattern = pattern;
    ptr->Format = NULL;
    ptr->SRS = NULL;
    ptr->Style = NULL;
    ptr->TileWidth = 0;
    ptr->TileHeight = 0;
    ptr->TileBaseX = DBL_MAX;
    ptr->TileBaseY = DBL_MAX;
    ptr->TileExtentX = DBL_MAX;
    ptr->TileExtentY = DBL_MAX;
    ptr->first = NULL;
    ptr->last = NULL;
    parse_url_arguments (ptr, pattern);
    ptr->next = NULL;
/* setting explicit values */
    pa = ptr->first;
    while (pa != NULL)
      {
	  if (strcasecmp (pa->argName, "format") == 0)
	      ptr->Format = pa->argValue;
	  if (strcasecmp (pa->argName, "srs") == 0)
	      ptr->SRS = pa->argValue;
	  if (strcasecmp (pa->argName, "styles") == 0)
	      ptr->Style = pa->argValue;
	  if (strcasecmp (pa->argName, "width") == 0)
	      ptr->TileWidth = atoi (pa->argValue);
	  if (strcasecmp (pa->argName, "width") == 0)
	      ptr->TileHeight = atoi (pa->argValue);
	  if (strcasecmp (pa->argName, "bbox") == 0)
	    {
		parse_pattern_bbox (pa->argValue, &minx, &miny, &maxx, &maxy);
		ptr->TileBaseX = minx;	/* leftmost coord */
		ptr->TileBaseY = maxy;	/* topmost coord */
		ptr->TileExtentX = maxx - minx;
		ptr->TileExtentY = maxy - miny;
	    }
	  pa = pa->next;
      }
    return ptr;
}

static void
wmsFreeTilePattern (wmsTilePatternPtr pattern)
{
/* memory cleanup - destroying a WMS TilePattern object */
    wmsUrlArgumentPtr pa;
    wmsUrlArgumentPtr pan;
    if (pattern == NULL)
	return;
    if (pattern->Pattern != NULL)
	free (pattern->Pattern);
    pa = pattern->first;
    while (pa != NULL)
      {
	  pan = pa->next;
	  wmsFreeUrlArgument (pa);
	  pa = pan;
      }
    free (pattern);
}

static wmsTiledLayerPtr
wmsAllocTiledLayer (const char *name, const char *title, const char *abstract)
{
/* allocating a WMS TiledLayer object */
    int len;
    wmsTiledLayerPtr lyr = malloc (sizeof (wmsTiledLayer));
    lyr->Name = NULL;
    lyr->Title = NULL;
    lyr->Abstract = NULL;
    if (name != NULL)
      {
	  len = strlen (name);
	  lyr->Name = malloc (len + 1);
	  strcpy (lyr->Name, name);
      }
    if (title != NULL)
      {
	  len = strlen (title);
	  lyr->Title = malloc (len + 1);
	  strcpy (lyr->Title, title);
      }
    if (abstract != NULL)
      {
	  len = strlen (abstract);
	  lyr->Abstract = malloc (len + 1);
	  strcpy (lyr->Abstract, abstract);
      }
    lyr->MinLat = DBL_MAX;
    lyr->MaxLat = DBL_MAX;
    lyr->MinLong = DBL_MAX;
    lyr->MaxLong = DBL_MAX;
    lyr->Pad = NULL;
    lyr->Bands = NULL;
    lyr->DataType = NULL;
    lyr->firstPattern = NULL;
    lyr->lastPattern = NULL;
    lyr->firstChild = NULL;
    lyr->lastChild = NULL;
    lyr->next = NULL;
    return lyr;
}

static void
wmsFreeTiledLayer (wmsTiledLayerPtr lyr)
{
/* memory cleanup - destroying a WMS TiledLayer object */
    wmsTilePatternPtr pp;
    wmsTilePatternPtr ppn;
    wmsTiledLayerPtr pt;
    wmsTiledLayerPtr ptn;
    if (lyr == NULL)
	return;
    if (lyr->Name != NULL)
	free (lyr->Name);
    if (lyr->Title != NULL)
	free (lyr->Title);
    if (lyr->Abstract != NULL)
	free (lyr->Abstract);
    if (lyr->Pad != NULL)
	free (lyr->Pad);
    if (lyr->Bands != NULL)
	free (lyr->Bands);
    if (lyr->DataType != NULL)
	free (lyr->DataType);
    pp = lyr->firstPattern;
    while (pp != NULL)
      {
	  ppn = pp->next;
	  wmsFreeTilePattern (pp);
	  pp = ppn;
      }
    pt = lyr->firstChild;
    while (pt != NULL)
      {
	  ptn = pt->next;
	  wmsFreeTiledLayer (pt);
	  pt = ptn;
      }
    free (lyr);
}

static wmsCapabilitiesPtr
wmsAllocCapabilities ()
{
/* allocating an empty WMS GetCapabilities object */
    wmsCapabilitiesPtr cap = malloc (sizeof (wmsCapabilities));
    cap->Version = NULL;
    cap->Name = NULL;
    cap->Title = NULL;
    cap->Abstract = NULL;
    cap->GetMapURLGet = NULL;
    cap->GetMapURLPost = NULL;
    cap->GetFeatureInfoURLGet = NULL;
    cap->GetFeatureInfoURLPost = NULL;
    cap->GetTileServiceURLGet = NULL;
    cap->GetTileServiceURLPost = NULL;
    cap->GmlMimeType = NULL;
    cap->XmlMimeType = NULL;
    cap->ContactPerson = NULL;
    cap->ContactOrganization = NULL;
    cap->ContactPosition = NULL;
    cap->PostalAddress = NULL;
    cap->City = NULL;
    cap->StateProvince = NULL;
    cap->PostCode = NULL;
    cap->Country = NULL;
    cap->VoiceTelephone = NULL;
    cap->FaxTelephone = NULL;
    cap->EMailAddress = NULL;
    cap->Fees = NULL;
    cap->AccessConstraints = NULL;
    cap->LayerLimit = -1;
    cap->MaxWidth = -1;
    cap->MaxHeight = -1;
    cap->firstFormat = NULL;
    cap->lastFormat = NULL;
    cap->firstLayer = NULL;
    cap->lastLayer = NULL;
    cap->TileServiceName = NULL;
    cap->TileServiceTitle = NULL;
    cap->TileServiceAbstract = NULL;
    cap->firstTiled = NULL;
    cap->lastTiled = NULL;
    return cap;
}

static void
wmsFreeCapabilities (wmsCapabilitiesPtr cap)
{
/* memory cleanup - freeing a WMS Capabilities object */
    wmsFormatPtr pf;
    wmsFormatPtr pfn;
    wmsLayerPtr pl;
    wmsLayerPtr pln;
    wmsTiledLayerPtr pt;
    wmsTiledLayerPtr ptn;
    if (cap == NULL)
	return;
    if (cap->Version)
	free (cap->Version);
    if (cap->Name)
	free (cap->Name);
    if (cap->Title)
	free (cap->Title);
    if (cap->Abstract)
	free (cap->Abstract);
    if (cap->GetMapURLGet)
	free (cap->GetMapURLGet);
    if (cap->GetMapURLPost)
	free (cap->GetMapURLPost);
    if (cap->GetFeatureInfoURLGet)
	free (cap->GetFeatureInfoURLGet);
    if (cap->GetFeatureInfoURLPost)
	free (cap->GetFeatureInfoURLPost);
    if (cap->GetTileServiceURLGet)
	free (cap->GetTileServiceURLGet);
    if (cap->GetTileServiceURLPost)
	free (cap->GetTileServiceURLPost);
    if (cap->GmlMimeType)
	free (cap->GmlMimeType);
    if (cap->XmlMimeType)
	free (cap->XmlMimeType);
    if (cap->ContactPerson)
	free (cap->ContactPerson);
    if (cap->ContactOrganization)
	free (cap->ContactOrganization);
    if (cap->ContactPosition)
	free (cap->ContactPosition);
    if (cap->PostalAddress)
	free (cap->PostalAddress);
    if (cap->City)
	free (cap->City);
    if (cap->StateProvince)
	free (cap->StateProvince);
    if (cap->PostCode)
	free (cap->PostCode);
    if (cap->Country)
	free (cap->Country);
    if (cap->VoiceTelephone)
	free (cap->VoiceTelephone);
    if (cap->FaxTelephone)
	free (cap->FaxTelephone);
    if (cap->EMailAddress)
	free (cap->EMailAddress);
    if (cap->Fees)
	free (cap->Fees);
    if (cap->AccessConstraints)
	free (cap->AccessConstraints);
    if (cap->TileServiceName)
	free (cap->TileServiceName);
    if (cap->TileServiceTitle)
	free (cap->TileServiceTitle);
    if (cap->TileServiceAbstract)
	free (cap->TileServiceAbstract);
    pf = cap->firstFormat;
    while (pf != NULL)
      {
	  pfn = pf->next;
	  wmsFreeFormat (pf);
	  pf = pfn;
      }
    pl = cap->firstLayer;
    while (pl != NULL)
      {
	  pln = pl->next;
	  wmsFreeLayer (pl);
	  pl = pln;
      }
    pt = cap->firstTiled;
    while (pt != NULL)
      {
	  ptn = pt->next;
	  wmsFreeTiledLayer (pt);
	  pt = ptn;
      }
    free (cap);
}

static wmsFeatureAttributePtr
wmsAllocFeatureAttribute (const char *name, char *value)
{
/* allocating and initializing a GML Feature Attribute object */
    int len;
    wmsFeatureAttributePtr attr = malloc (sizeof (wmsFeatureAttribute));
    len = strlen (name);
    attr->name = malloc (len + 1);
    strcpy (attr->name, name);
    attr->value = value;
    attr->geometry = NULL;
    attr->next = NULL;
    return attr;
}

static void
wmsFreeFeatureAttribute (wmsFeatureAttributePtr attr)
{
/* memory cleanup - freeing a GML Feature Attribute object */
    if (attr == NULL)
	return;
    if (attr->name != NULL)
	free (attr->name);
    if (attr->value != NULL)
	free (attr->value);
    if (attr->geometry != NULL)
	gaiaFreeGeomColl (attr->geometry);
    free (attr);
}

static wmsFeatureMemberPtr
wmsAllocFeatureMember (const char *name)
{
/* allocating an empty GML Feature Member object */
    int len;
    wmsFeatureMemberPtr member = malloc (sizeof (wmsFeatureMember));
    len = strlen (name);
    member->layer_name = malloc (len + 1);
    strcpy (member->layer_name, name);
    member->first = NULL;
    member->last = NULL;
    member->next = NULL;
    return member;
}

static void
wmsFreeFeatureMember (wmsFeatureMemberPtr member)
{
/* memory cleanup - freeing a GML Feature Member object */
    wmsFeatureAttributePtr pa;
    wmsFeatureAttributePtr pan;
    if (member == NULL)
	return;
    if (member->layer_name != NULL)
	free (member->layer_name);
    pa = member->first;
    while (pa != NULL)
      {
	  pan = pa->next;
	  wmsFreeFeatureAttribute (pa);
	  pa = pan;
      }
    free (member);
}

static void
wmsAddFeatureMemberAttribute (wmsFeatureMemberPtr member, const char *name,
			      char *value)
{
/* adding an attribute+value to some GML Feature Member */
    wmsFeatureAttributePtr attr;
    if (member == NULL)
	return;
    attr = wmsAllocFeatureAttribute (name, value);
    if (member->first == NULL)
	member->first = attr;
    if (member->last != NULL)
	member->last->next = attr;
    member->last = attr;
}

static wmsFeatureCollectionPtr
wmsAllocFeatureCollection ()
{
/* allocating an empty GML Feature Collection object */
    wmsFeatureCollectionPtr coll = malloc (sizeof (wmsFeatureCollection));
    coll->first = NULL;
    coll->last = NULL;
    return coll;
}

static void
wmsFreeFeatureCollection (wmsFeatureCollectionPtr coll)
{
/* memory cleanup - freeing a GML Feature Collection object */
    wmsFeatureMemberPtr pm;
    wmsFeatureMemberPtr pmn;
    if (coll == NULL)
	return;
    pm = coll->first;
    while (pm != NULL)
      {
	  pmn = pm->next;
	  wmsFreeFeatureMember (pm);
	  pm = pmn;
      }
    free (coll);
}

static wmsSinglePartResponsePtr
wmsAllocSinglePartResponse (char *body)
{
/* allocating and initializing a SinglePart Response */
    wmsSinglePartResponsePtr single = malloc (sizeof (wmsSinglePartResponse));
    single->body = body;
    single->next = NULL;
    return single;
}

static void
wmsFreeSinglePartResponse (wmsSinglePartResponsePtr single)
{
/* memory cleanup - freeing a HTTP SinglePart Response object */
    if (single == NULL)
	return;
    if (single->body != NULL)
	free (single->body);
    free (single);
}

static wmsMultipartCollectionPtr
wmsAllocMultipartCollection ()
{
/* allocating an empty HTTP Multipart response object */
    wmsMultipartCollectionPtr coll = malloc (sizeof (wmsMultipartCollection));
    coll->first = NULL;
    coll->last = NULL;
    return coll;
}

static void
wmsFreeMultipartCollection (wmsMultipartCollectionPtr coll)
{
/* memory cleanup - freeing a HTTP Multipart Collection object */
    wmsSinglePartResponsePtr ps;
    wmsSinglePartResponsePtr psn;
    if (coll == NULL)
	return;
    ps = coll->first;
    while (ps != NULL)
      {
	  psn = ps->next;
	  wmsFreeSinglePartResponse (ps);
	  ps = psn;
      }
    free (coll);
}

static void
wmsAddPartToMultipart (wmsMultipartCollectionPtr coll, char *body)
{
/* adding a SinglePart Response to the Collection */
    wmsSinglePartResponsePtr single = wmsAllocSinglePartResponse (body);
    if (coll->first == NULL)
	coll->first = single;
    if (coll->last != NULL)
	coll->last->next = single;
    coll->last = single;
}

static void
wmsMemBufferInitialize (wmsMemBufferPtr buf)
{
/* initializing a dynamically growing output buffer */
    buf->Buffer = NULL;
    buf->WriteOffset = 0;
    buf->BufferSize = 0;
    buf->Error = 0;
}

static void
wmsMemBufferReset (wmsMemBufferPtr buf)
{
/* cleaning a dynamically growing output buffer */
    if (buf->Buffer)
	free (buf->Buffer);
    buf->Buffer = NULL;
    buf->WriteOffset = 0;
    buf->BufferSize = 0;
    buf->Error = 0;
}

static void
wmsMemBufferAppend (wmsMemBufferPtr buf, const unsigned char *payload,
		    size_t size)
{
/* appending into the buffer */
    size_t free_size = buf->BufferSize - buf->WriteOffset;
    if (size > free_size)
      {
	  /* we must allocate a bigger buffer */
	  size_t new_size;
	  unsigned char *new_buf;
	  if (buf->BufferSize == 0)
	      new_size = size + 1024;
	  else if (buf->BufferSize <= 4196)
	      new_size = buf->BufferSize + size + 4196;
	  else if (buf->BufferSize <= 65536)
	      new_size = buf->BufferSize + size + 65536;
	  else
	      new_size = buf->BufferSize + size + (1024 * 1024);
	  new_buf = malloc (new_size);
	  if (!new_buf)
	    {
		buf->Error = 1;
		return;
	    }
	  if (buf->Buffer)
	    {
		memcpy (new_buf, buf->Buffer, buf->WriteOffset);
		free (buf->Buffer);
	    }
	  buf->Buffer = new_buf;
	  buf->BufferSize = new_size;
      }
    memcpy (buf->Buffer + buf->WriteOffset, payload, size);
    buf->WriteOffset += size;
}

static wmsMultipartCollectionPtr
parse_multipart_body (wmsMemBufferPtr buf, const char *multipart_boundary)
{
/* attempting to split a Multipart Response in its single parts */
    const char *p_in;
    char *marker = sqlite3_mprintf ("--%s\r\n", multipart_boundary);
    char *marker_end = sqlite3_mprintf ("--%s--\r\n", multipart_boundary);
    wmsMultipartCollectionPtr coll = wmsAllocMultipartCollection ();
/* ensuring a NULL terminated string */
    if (buf->BufferSize > buf->WriteOffset)
	*(buf->Buffer + buf->WriteOffset) = '\0';
    else
	wmsMemBufferAppend (buf, (const unsigned char *) " ", 2);
    p_in = (const char *) buf->Buffer;
    while (p_in != NULL)
      {
	  int len;
	  const char *start_body;
	  const char *stop;
	  const char *start = strstr (p_in, marker);
	  if (start == NULL)
	      break;
	  start_body = strstr (start, "\r\n\r\n");
	  if (start_body == NULL)
	      break;
	  start_body += 4;
	  stop = strstr (start_body, marker);
	  if (stop != NULL)
	      p_in = stop;
	  else
	    {
		stop = strstr (start_body, marker_end);
		if (stop == NULL)
		    break;
		p_in = NULL;
	    }
	  stop -= 1;
	  len = stop - start_body + 1;
	  if (len > 0)
	    {
		char *body;
		body = malloc (len + 1);
		memcpy (body, start_body, len);
		*(body + len) = '\0';
		wmsAddPartToMultipart (coll, body);
	    }
      }
    sqlite3_free (marker);
    sqlite3_free (marker_end);
    if (coll->first == NULL)
      {
	  wmsFreeMultipartCollection (coll);
	  return NULL;
      }
    return coll;
}

static size_t
store_data (char *ptr, size_t size, size_t nmemb, void *userdata)
{
/* updating the dynamic buffer */
    size_t total = size * nmemb;
    wmsMemBufferAppend (userdata, (unsigned char *) ptr, total);
    return total;
}

static char *
parse_http_format (wmsMemBufferPtr buf)
{
/* parsing the Content-Type from the HTTP header */
    int i;
    unsigned char *p_in = NULL;
    unsigned char *base;
    int size = 0;
    char *tmp;

    if (buf->Buffer == NULL)
	return NULL;
    for (i = 0; i < (int) (buf->WriteOffset) - 15; i++)
      {
	  if (memcmp (buf->Buffer + i, "Content-Type: ", 14) == 0)
	    {
		p_in = buf->Buffer + i + 14;
		break;
	    }
      }
    if (p_in == NULL)
	return NULL;

/* attempting to retrieve the Content-Type */
    base = p_in;
    while ((size_t) (p_in - buf->Buffer) < buf->WriteOffset)
      {
	  if (*p_in == '\r')
	      break;
	  size++;
	  p_in++;
      }
    if (size <= 0)
	return NULL;
    tmp = malloc (size + 1);
    memcpy (tmp, base, size);
    *(tmp + size) = '\0';
    return tmp;
}

static char *
parse_http_redirect (wmsMemBufferPtr buf)
{
/* parsing a redirect location from the HTTP header */
    int i;
    unsigned char *p_in = NULL;
    unsigned char *base;
    int size = 0;
    char *tmp;

    if (buf->Buffer == NULL)
	return NULL;
    for (i = 0; i < (int) (buf->WriteOffset) - 11; i++)
      {
	  if (memcmp (buf->Buffer + i, "Location: ", 10) == 0)
	    {
		p_in = buf->Buffer + i + 10;
		break;
	    }
      }
    if (p_in == NULL)
	return NULL;

/* attempting to retrieve the new HTTP location */
    base = p_in;
    while ((size_t) (p_in - buf->Buffer) < buf->WriteOffset)
      {
	  if (*p_in == '\r')
	      break;
	  size++;
	  p_in++;
      }
    if (size <= 0)
	return NULL;
    tmp = malloc (size + 1);
    memcpy (tmp, base, size);
    *(tmp + size) = '\0';
    return tmp;
}

static void
check_http_header (wmsMemBufferPtr buf, int *http_status, char **http_code)
{
/* checking the HTTP header */
    unsigned char *p_in;
    unsigned char *base_status;
    unsigned char *base_code;
    int size_status = 0;
    int size_code = 0;
    char *tmp;

    *http_status = -1;
    *http_code = NULL;
    if (buf->Buffer == NULL)
	return;
    if (buf->WriteOffset < 10)
	return;
    if (memcmp (buf->Buffer, "HTTP/1.1 ", 9) != 0
	&& memcmp (buf->Buffer, "HTTP/1.0 ", 9) != 0)
	return;

/* attempting to retrieve the HTTP status */
    p_in = buf->Buffer + 9;
    base_status = p_in;
    while ((size_t) (p_in - buf->Buffer) < buf->WriteOffset)
      {
	  if (*p_in == ' ')
	      break;
	  size_status++;
	  p_in++;
      }
    if (size_status <= 0)
	return;
    tmp = malloc (size_status + 1);
    memcpy (tmp, base_status, size_status);
    *(tmp + size_status) = '\0';
    *http_status = atoi (tmp);
    free (tmp);

/* attempting to retrieve the HTTP code */
    p_in = buf->Buffer + 10 + size_status;
    base_code = p_in;
    while ((size_t) (p_in - buf->Buffer) < buf->WriteOffset)
      {
	  if (*p_in == '\r')
	      break;
	  size_code++;
	  p_in++;
      }
    if (size_code <= 0)
	return;
    tmp = malloc (size_code + 1);
    memcpy (tmp, base_code, size_code);
    *(tmp + size_code) = '\0';
    *http_code = tmp;
}

static char *
check_http_multipart_response (wmsMemBufferPtr buf)
{
/* testing for a Content-Type: multipart returning an eventual Boundary delimiter */
    int i;
    unsigned char *p_in = NULL;
    unsigned char *base;
    int size = 0;
    char *mime;
    const char *boundary;
    char *tmp;

    if (buf->Buffer == NULL)
	return NULL;
    for (i = 0; i < (int) (buf->WriteOffset) - 15; i++)
      {
	  if (memcmp (buf->Buffer + i, "Content-Type: ", 14) == 0)
	    {
		p_in = buf->Buffer + i + 14;
		break;
	    }
      }
    if (p_in == NULL)
	return NULL;

/* attempting to retrieve the Content-Type */
    base = p_in;
    while ((size_t) (p_in - buf->Buffer) < buf->WriteOffset)
      {
	  if (*p_in == '\r')
	      break;
	  size++;
	  p_in++;
      }
    if (size <= 0)
	return NULL;
    mime = malloc (size + 1);
    memcpy (mime, base, size);
    *(mime + size) = '\0';
    if (strncmp (mime, "multipart/", 10) != 0)
	goto not_found;
    boundary = strstr (mime, "boundary=");
    if (boundary == NULL)
	goto not_found;
/* attempting to retrieve the Boundary */
    size = strlen (boundary + 9);
    if (size <= 0)
	goto not_found;
    tmp = malloc (size + 1);
    strcpy (tmp, boundary + 9);
    free (mime);
    return tmp;
  not_found:
    free (mime);
    return NULL;
}

static int
start_cdata (const char *p_in, int i, int max)
{
/* testing for "<!CDATA[" - CDATA start marker */
    if (i + 9 >= max)
	return 0;
    if (*(p_in + i) == '<' && *(p_in + i + 1) == '!' && *(p_in + i + 2) == '['
	&& *(p_in + i + 3) == 'C' && *(p_in + i + 4) == 'D'
	&& *(p_in + i + 5) == 'A' && *(p_in + i + 6) == 'T'
	&& *(p_in + i + 7) == 'A' && *(p_in + i + 8) == '[')
	return 1;
    return 0;
}

static int
end_cdata (const char *p_in, int i)
{
/* testing for "]]>" - CDATA end marker */
    if (i < 2)
	return 0;
    if (*(p_in + i - 2) == ']' && *(p_in + i - 1) == ']' && *(p_in + i) == '>')
	return 1;
    return 0;
}

static char *
clean_xml (wmsMemBuffer * in)
{
/* cleaning the XML payload by removing useless whitespaces */
    wmsMemBuffer outbuf;
    char *out;
    const char *p_in;
    int i;
    int j;
    int cdata = 0;
    int ignore = 0;
    if (in->WriteOffset <= 0)
	return NULL;
    wmsMemBufferInitialize (&outbuf);

    p_in = (const char *) (in->Buffer);
    for (i = 0; i < (int) (in->WriteOffset); i++)
      {
	  if (*(p_in + i) == '<')
	    {
		if (!cdata)
		  {
		      if (start_cdata (p_in, i, in->WriteOffset))
			{
			    i += 8;
			    cdata = 1;
			    continue;
			}
		      for (j = outbuf.WriteOffset - 1; j > 0; j--)
			{
			    /* consuming trailing whitespaces */
			    if (*(outbuf.Buffer + j) == ' '
				|| *(outbuf.Buffer + j) == '\t'
				|| *(outbuf.Buffer + j) == '\r'
				|| *(outbuf.Buffer + j) == '\n')
			      {
				  outbuf.WriteOffset -= 1;
				  continue;
			      }
			    break;
			}
		      ignore = 0;
		  }
	    }
	  if (ignore)
	    {
		if (*(p_in + i) == ' ' || *(p_in + i) == '\t'
		    || *(p_in + i) == '\r' || *(p_in + i) == '\n')
		    continue;
		else
		    ignore = 0;
	    }
	  if (*(p_in + i) == '>' && cdata && end_cdata (p_in, i))
	    {
		cdata = 0;
		outbuf.WriteOffset -= 2;
		continue;
	    }
	  if (cdata)
	    {
		/* masking XML special characters */
		if (*(p_in + i) == '<')
		    wmsMemBufferAppend (&outbuf, (const unsigned char *) "&lt;",
					4);
		else if (*(p_in + i) == '>')
		    wmsMemBufferAppend (&outbuf, (const unsigned char *) "&gt;",
					4);
		else if (*(p_in + i) == '&')
		    wmsMemBufferAppend (&outbuf,
					(const unsigned char *) "&amp;", 5);
		else if (*(p_in + i) == '>')
		    wmsMemBufferAppend (&outbuf,
					(const unsigned char *) "&quot;", 6);
		else
		    wmsMemBufferAppend (&outbuf,
					(const unsigned char *) (p_in + i), 1);
	    }
	  else
	      wmsMemBufferAppend (&outbuf, (const unsigned char *) (p_in + i),
				  1);
	  if (*(p_in + i) == '>')
	    {
		if (!cdata)
		    ignore = 1;
	    }
      }
    out = malloc (outbuf.WriteOffset + 1);
    memcpy (out, outbuf.Buffer, outbuf.WriteOffset);
    *(out + outbuf.WriteOffset) = '\0';
    wmsMemBufferReset (&outbuf);
    return out;
}

static char *
clean_xml_str (const char *p_in)
{
/* cleaning the XML payload by removing useless whitespaces */
    wmsMemBuffer outbuf;
    char *out;
    int i;
    int j;
    int cdata = 0;
    int ignore = 0;
    int len = strlen (p_in);
    if (len <= 0)
	return NULL;
    wmsMemBufferInitialize (&outbuf);

    for (i = 0; i < len; i++)
      {
	  if (*(p_in + i) == '<')
	    {
		if (!cdata)
		  {
		      if (start_cdata (p_in, i, len))
			{
			    i += 8;
			    cdata = 1;
			    continue;
			}
		      for (j = outbuf.WriteOffset - 1; j > 0; j--)
			{
			    /* consuming trailing whitespaces */
			    if (*(outbuf.Buffer + j) == ' '
				|| *(outbuf.Buffer + j) == '\t'
				|| *(outbuf.Buffer + j) == '\r'
				|| *(outbuf.Buffer + j) == '\n')
			      {
				  outbuf.WriteOffset -= 1;
				  continue;
			      }
			    break;
			}
		      ignore = 0;
		  }
	    }
	  if (ignore)
	    {
		if (*(p_in + i) == ' ' || *(p_in + i) == '\t'
		    || *(p_in + i) == '\r' || *(p_in + i) == '\n')
		    continue;
		else
		    ignore = 0;
	    }
	  if (*(p_in + i) == '>' && cdata && end_cdata (p_in, i))
	    {
		cdata = 0;
		outbuf.WriteOffset -= 2;
		continue;
	    }
	  if (cdata)
	    {
		/* masking XML special characters */
		if (*(p_in + i) == '<')
		    wmsMemBufferAppend (&outbuf, (const unsigned char *) "&lt;",
					4);
		else if (*(p_in + i) == '>')
		    wmsMemBufferAppend (&outbuf, (const unsigned char *) "&gt;",
					4);
		else if (*(p_in + i) == '&')
		    wmsMemBufferAppend (&outbuf,
					(const unsigned char *) "&amp;", 5);
		else if (*(p_in + i) == '>')
		    wmsMemBufferAppend (&outbuf,
					(const unsigned char *) "&quot;", 6);
		else
		    wmsMemBufferAppend (&outbuf,
					(const unsigned char *) (p_in + i), 1);
	    }
	  else
	      wmsMemBufferAppend (&outbuf, (const unsigned char *) (p_in + i),
				  1);
	  if (*(p_in + i) == '>')
	    {
		if (!cdata)
		    ignore = 1;
	    }
      }
    out = malloc (outbuf.WriteOffset + 1);
    memcpy (out, outbuf.Buffer, outbuf.WriteOffset);
    *(out + outbuf.WriteOffset) = '\0';
    wmsMemBufferReset (&outbuf);
    return out;
}

static void
wmsParsingError (void *ctx, const char *msg, ...)
{
/* appending to the current Parsing Error buffer */
    wmsMemBufferPtr buf = (wmsMemBufferPtr) ctx;
    char out[65536];
    va_list args;

    if (ctx != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */

    va_start (args, msg);
    vsnprintf (out, 65536, msg, args);
    wmsMemBufferAppend (buf, (unsigned char *) out, strlen (out));
    va_end (args);
}

static void
parse_wms_EX_geoBBox (xmlNodePtr node, wmsLayerPtr lyr)
{
/* parsing a WMS Layer/EX_GeographicBoundingBox */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp
		    ((const char *) (cur_node->name),
		     "southBoundLatitude") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				lyr->MinLat =
				    atof ((const char *) (child_node->content));
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "northBoundLatitude") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				lyr->MaxLat =
				    atof ((const char *) (child_node->content));
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "westBoundLongitude") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				lyr->MinLong =
				    atof ((const char *) (child_node->content));
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "eastBoundLongitude") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				lyr->MaxLong =
				    atof ((const char *) (child_node->content));
			}
		  }
	    }
      }
}

static void
parse_wms_geoBBox (struct _xmlAttr *properties, wmsLayerPtr lyr)
{
/* parsing a WMS Layer/LatLonBoundingBox */
    struct _xmlAttr *attr = properties;

    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		xmlNodePtr text;
		if (strcmp ((const char *) (attr->name), "miny") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->MinLat = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "maxy") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->MaxLat = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "minx") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->MinLong = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "maxx") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->MaxLong = atof ((const char *) (text->content));
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wms_BBox (struct _xmlAttr *properties, wmsLayerPtr lyr)
{
/* parsing a WMS Layer/BoundingBox */
    struct _xmlAttr *attr = properties;
    const char *crs = NULL;
    double minx = DBL_MAX;
    double maxx = DBL_MAX;
    double miny = DBL_MAX;
    double maxy = DBL_MAX;
    wmsBBoxPtr bbox;

    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		xmlNodePtr text;
		if (strcmp ((const char *) (attr->name), "CRS") == 0
		    || strcmp ((const char *) (attr->name), "SRS") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  crs = (const char *) (text->content);
		  }
		if (strcmp ((const char *) (attr->name), "miny") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  miny = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "maxy") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  maxy = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "minx") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  minx = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "maxx") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  maxx = atof ((const char *) (text->content));
		  }
	    }
	  attr = attr->next;
      }
    bbox = wmsAllocBBox (crs, minx, maxx, miny, maxy);
    if (lyr->firstBBox == NULL)
	lyr->firstBBox = bbox;
    if (lyr->lastBBox != NULL)
	lyr->lastBBox->next = bbox;
    lyr->lastBBox = bbox;
}

static void
parse_wms_style (xmlNodePtr node, wmsLayerPtr lyr)
{
/* parsing a WMS Style definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    wmsStylePtr stl;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
	    }
      }

    stl = wmsAllocStyle (name, title, abstract);
    if (lyr->firstStyle == NULL)
	lyr->firstStyle = stl;
    if (lyr->lastStyle != NULL)
	lyr->lastStyle->next = stl;
    lyr->lastStyle = stl;
}

static void
parse_wms_layer_in_layer (xmlNodePtr node, struct _xmlAttr *properties,
			  wmsLayerPtr group, int level)
{
/* recursively parsing a WMS Layer definition (2nd level) */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    wmsLayerPtr lyr;
    struct _xmlAttr *attr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
	    }
      }
    lyr = wmsAllocLayer (name, title, abstract, group);
    if (group->firstLayer == NULL)
	group->firstLayer = lyr;
    if (group->lastLayer != NULL)
	group->lastLayer->next = lyr;
    group->lastLayer = lyr;

    attr = properties;
    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		xmlNodePtr text;
		if (strcmp ((const char *) (attr->name), "queryable") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->Queryable =
			      atoi ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "opaque") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->Opaque = atoi ((const char *) (text->content));
		  }
	    }
	  attr = attr->next;
      }

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "CRS") == 0
		    || strcmp ((const char *) (cur_node->name), "SRS") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  wmsCrsPtr crs;
				  const char *crs_string =
				      (const char *) (child_node->content);
				  crs = wmsAllocCrs (crs_string);
				  if (lyr->firstCrs == NULL)
				      lyr->firstCrs = crs;
				  if (lyr->lastCrs != NULL)
				      lyr->lastCrs->next = crs;
				  lyr->lastCrs = crs;
			      }
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "EX_GeographicBoundingBox") == 0)
		    parse_wms_EX_geoBBox (cur_node->children, lyr);
		if (strcmp
		    ((const char *) (cur_node->name), "LatLonBoundingBox") == 0)
		    parse_wms_geoBBox (cur_node->properties, lyr);
		if (strcmp ((const char *) (cur_node->name), "BoundingBox") ==
		    0)
		    parse_wms_BBox (cur_node->properties, lyr);
		if (strcmp ((const char *) (cur_node->name), "Style") == 0)
		    parse_wms_style (cur_node->children, lyr);
		if (strcmp
		    ((const char *) (cur_node->name),
		     "MinScaleDenominator") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  const char *str =
				      (const char *) (child_node->content);
				  lyr->MinScaleDenominator = atof (str);
			      }
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "MaxScaleDenominator") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  const char *str =
				      (const char *) (child_node->content);
				  lyr->MaxScaleDenominator = atof (str);
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Layer") == 0)
		    parse_wms_layer_in_layer (cur_node->children,
					      cur_node->properties, lyr,
					      level + 1);
	    }
      }
}

static void
parse_wms_layer (xmlNodePtr node, struct _xmlAttr *properties,
		 wmsCapabilitiesPtr cap)
{
/* parsing a WMS Layer definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    wmsLayerPtr lyr;
    struct _xmlAttr *attr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
	    }
      }
    lyr = wmsAllocLayer (name, title, abstract, NULL);
    if (cap->firstLayer == NULL)
	cap->firstLayer = lyr;
    if (cap->lastLayer != NULL)
	cap->lastLayer->next = lyr;
    cap->lastLayer = lyr;

    attr = properties;
    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		xmlNodePtr text;
		if (strcmp ((const char *) (attr->name), "queryable") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->Queryable =
			      atoi ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "opaque") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->Opaque = atoi ((const char *) (text->content));
		  }
	    }
	  attr = attr->next;
      }

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "CRS") == 0
		    || strcmp ((const char *) (cur_node->name), "SRS") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  wmsCrsPtr crs;
				  const char *crs_string =
				      (const char *) (child_node->content);
				  crs = wmsAllocCrs (crs_string);
				  if (lyr->firstCrs == NULL)
				      lyr->firstCrs = crs;
				  if (lyr->lastCrs != NULL)
				      lyr->lastCrs->next = crs;
				  lyr->lastCrs = crs;
			      }
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "EX_GeographicBoundingBox") == 0)
		    parse_wms_EX_geoBBox (cur_node->children, lyr);
		if (strcmp
		    ((const char *) (cur_node->name), "LatLonBoundingBox") == 0)
		    parse_wms_geoBBox (cur_node->properties, lyr);
		if (strcmp ((const char *) (cur_node->name), "BoundingBox") ==
		    0)
		    parse_wms_BBox (cur_node->properties, lyr);
		if (strcmp ((const char *) (cur_node->name), "Style") == 0)
		    parse_wms_style (cur_node->children, lyr);
		if (strcmp
		    ((const char *) (cur_node->name),
		     "MinScaleDenominator") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  const char *str =
				      (const char *) (child_node->content);
				  lyr->MinScaleDenominator = atof (str);
			      }
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "MaxScaleDenominator") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  const char *str =
				      (const char *) (child_node->content);
				  lyr->MaxScaleDenominator = atof (str);
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Layer") == 0)
		    parse_wms_layer_in_layer (cur_node->children,
					      cur_node->properties, lyr, 0);
	    }
      }
}

static void
parse_wms_contact_person (xmlNodePtr node, const char **contact_person,
			  const char **contact_organization)
{
/* parsing a WMS ContactPersonPrimary definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp
		    ((const char *) (cur_node->name),
		     "ContactOrganization") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*contact_organization =
				    (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "ContactPerson") ==
		    0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*contact_person =
				    (const char *) (child_node->content);
			}
		  }
	    }
      }
}

static void
parse_wms_contact_address (xmlNodePtr node, const char **postal_address,
			   const char **city, const char **state_province,
			   const char **post_code, const char **country)
{
/* parsing a WMS ContactAddress definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Address") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*postal_address =
				    (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "City") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*city = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "StateOrProvince")
		    == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*state_province =
				    (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "PostCode") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*post_code =
				    (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Country") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*country = (const char *) (child_node->content);
			}
		  }
	    }
      }
}

static void
parse_wms_contact_information (xmlNodePtr node, const char **contact_person,
			       const char **contact_organization,
			       const char **contact_position,
			       const char **postal_address, const char **city,
			       const char **state_province,
			       const char **post_code, const char **country,
			       const char **voice_telephone,
			       const char **fax_telephone,
			       const char **email_address)
{
/* parsing a WMS ContactInformation definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "ContactPosition")
		    == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*contact_position =
				    (const char *) (child_node->content);
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "ContactPersonPrimary") == 0)
		    parse_wms_contact_person (cur_node->children,
					      contact_person,
					      contact_organization);
		if (strcmp ((const char *) (cur_node->name), "ContactAddress")
		    == 0)
		    parse_wms_contact_address (cur_node->children,
					       postal_address, city,
					       state_province, post_code,
					       country);
		if (strcmp
		    ((const char *) (cur_node->name),
		     "ContactVoiceTelephone") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*voice_telephone =
				    (const char *) (child_node->content);
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "ContactFacsimileTelephone") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*fax_telephone =
				    (const char *) (child_node->content);
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "ContactElectronicMailAddress") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				*email_address =
				    (const char *) (child_node->content);
			}
		  }
	    }
      }
}

static void
parse_wms_service (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* parsing a WMS Service definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    const char *contact_person = NULL;
    const char *contact_organization = NULL;
    const char *contact_position = NULL;
    const char *postal_address = NULL;
    const char *city = NULL;
    const char *state_province = NULL;
    const char *post_code = NULL;
    const char *country = NULL;
    const char *voice_telephone = NULL;
    const char *fax_telephone = NULL;
    const char *email_address = NULL;
    const char *fees = NULL;
    const char *access_constraints = NULL;
    int layer_limit = -1;
    int maxWidth = -1;
    int maxHeight = -1;
    int len;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name),
		     "ContactInformation") == 0)
		    parse_wms_contact_information (cur_node->children,
						   &contact_person,
						   &contact_organization,
						   &contact_position,
						   &postal_address, &city,
						   &state_province, &post_code,
						   &country, &voice_telephone,
						   &fax_telephone,
						   &email_address);
		if (strcmp ((const char *) (cur_node->name), "Fees") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				fees = (const char *) (child_node->content);
			}
		  }
		if (strcmp
		    ((const char *) (cur_node->name), "AccessConstraints") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				access_constraints =
				    (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "LayerLimit") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				layer_limit =
				    atoi ((const char *) (child_node->content));
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "MaxWidth") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				maxWidth =
				    atoi ((const char *) (child_node->content));
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "MaxHeight") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				maxHeight =
				    atoi ((const char *) (child_node->content));
			}
		  }
	    }
      }

    if (cap->Name != NULL)
      {
	  free (cap->Name);
	  cap->Name = NULL;
      }
    if (name != NULL)
      {
	  len = strlen (name);
	  cap->Name = malloc (len + 1);
	  strcpy (cap->Name, name);
      }
    if (cap->Title != NULL)
      {
	  free (cap->Title);
	  cap->Title = NULL;
      }
    if (title != NULL)
      {
	  len = strlen (title);
	  cap->Title = malloc (len + 1);
	  strcpy (cap->Title, title);
      }
    if (cap->Abstract != NULL)
      {
	  free (cap->Abstract);
	  cap->Abstract = NULL;
      }
    if (abstract != NULL)
      {
	  len = strlen (abstract);
	  cap->Abstract = malloc (len + 1);
	  strcpy (cap->Abstract, abstract);
      }
    if (contact_person != NULL)
      {
	  len = strlen (contact_person);
	  cap->ContactPerson = malloc (len + 1);
	  strcpy (cap->ContactPerson, contact_person);
      }
    if (contact_organization != NULL)
      {
	  len = strlen (contact_organization);
	  cap->ContactOrganization = malloc (len + 1);
	  strcpy (cap->ContactOrganization, contact_organization);
      }
    if (contact_position != NULL)
      {
	  len = strlen (contact_position);
	  cap->ContactPosition = malloc (len + 1);
	  strcpy (cap->ContactPosition, contact_position);
      }
    if (postal_address != NULL)
      {
	  len = strlen (postal_address);
	  cap->PostalAddress = malloc (len + 1);
	  strcpy (cap->PostalAddress, postal_address);
      }
    if (city != NULL)
      {
	  len = strlen (city);
	  cap->City = malloc (len + 1);
	  strcpy (cap->City, city);
      }
    if (state_province != NULL)
      {
	  len = strlen (state_province);
	  cap->StateProvince = malloc (len + 1);
	  strcpy (cap->StateProvince, state_province);
      }
    if (post_code != NULL)
      {
	  len = strlen (post_code);
	  cap->PostCode = malloc (len + 1);
	  strcpy (cap->PostCode, post_code);
      }
    if (country != NULL)
      {
	  len = strlen (country);
	  cap->Country = malloc (len + 1);
	  strcpy (cap->Country, country);
      }
    if (voice_telephone != NULL)
      {
	  len = strlen (voice_telephone);
	  cap->VoiceTelephone = malloc (len + 1);
	  strcpy (cap->VoiceTelephone, voice_telephone);
      }
    if (fax_telephone != NULL)
      {
	  len = strlen (fax_telephone);
	  cap->FaxTelephone = malloc (len + 1);
	  strcpy (cap->FaxTelephone, fax_telephone);
      }
    if (email_address != NULL)
      {
	  len = strlen (email_address);
	  cap->EMailAddress = malloc (len + 1);
	  strcpy (cap->EMailAddress, email_address);
      }
    if (fees != NULL)
      {
	  len = strlen (fees);
	  cap->Fees = malloc (len + 1);
	  strcpy (cap->Fees, fees);
      }
    if (access_constraints != NULL)
      {
	  len = strlen (access_constraints);
	  cap->AccessConstraints = malloc (len + 1);
	  strcpy (cap->AccessConstraints, access_constraints);
      }
    if (layer_limit > 0)
	cap->LayerLimit = layer_limit;
    if (maxWidth > 0)
	cap->MaxWidth = maxWidth;
    if (maxHeight > 0)
	cap->MaxHeight = maxHeight;
}

static void
parse_wms_GetMap_HTTP_Get (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetMap/DCPType/HTTP/Get node */
    xmlNodePtr cur_node = NULL;
    int len;
    const char *p;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "OnlineResource")
		    == 0)
		  {
		      struct _xmlAttr *attr = cur_node->properties;
		      while (attr != NULL)
			{
			    if (attr->name != NULL)
			      {
				  if (strcmp
				      ((const char *) (attr->name),
				       "href") == 0)
				    {
					xmlNodePtr text = attr->children;
					if (text->type == XML_TEXT_NODE)
					  {
					      if (cap->GetMapURLGet != NULL)
						{
						    free (cap->GetMapURLGet);
						    cap->GetMapURLGet = NULL;
						}
					      p = (const char
						   *) (text->content);
					      len = strlen (p);
					      cap->GetMapURLGet =
						  malloc (len + 1);
					      strcpy (cap->GetMapURLGet, p);
					  }
				    }
			      }
			    attr = attr->next;
			}
		  }
	    }
      }
}

static void
parse_wms_GetMap_HTTP_Post (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetMap/DCPType/HTTP/Post node */
    xmlNodePtr cur_node = NULL;
    int len;
    const char *p;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "OnlineResource")
		    == 0)
		  {
		      struct _xmlAttr *attr = cur_node->properties;
		      while (attr != NULL)
			{
			    if (attr->name != NULL)
			      {
				  if (strcmp
				      ((const char *) (attr->name),
				       "href") == 0)
				    {
					xmlNodePtr text = attr->children;
					if (text->type == XML_TEXT_NODE)
					  {
					      if (cap->GetMapURLPost != NULL)
						{
						    free (cap->GetMapURLPost);
						    cap->GetMapURLPost = NULL;
						}
					      p = (const char
						   *) (text->content);
					      len = strlen (p);
					      cap->GetMapURLPost =
						  malloc (len + 1);
					      strcpy (cap->GetMapURLPost, p);
					  }
				    }
			      }
			    attr = attr->next;
			}
		  }
	    }
      }
}

static void
parse_wms_GetTileService_HTTP_Get (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetTileService/DCPType/HTTP/Get node */
    xmlNodePtr cur_node = NULL;
    int len;
    const char *p;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "OnlineResource")
		    == 0)
		  {
		      struct _xmlAttr *attr = cur_node->properties;
		      while (attr != NULL)
			{
			    if (attr->name != NULL)
			      {
				  if (strcmp
				      ((const char *) (attr->name),
				       "href") == 0)
				    {
					xmlNodePtr text = attr->children;
					if (text->type == XML_TEXT_NODE)
					  {
					      if (cap->GetTileServiceURLGet !=
						  NULL)
						{
						    free (cap->GetTileServiceURLGet);
						    cap->GetMapURLGet = NULL;
						}
					      p = (const char
						   *) (text->content);
					      len = strlen (p);
					      cap->GetTileServiceURLGet =
						  malloc (len + 1);
					      strcpy (cap->GetTileServiceURLGet,
						      p);
					  }
				    }
			      }
			    attr = attr->next;
			}
		  }
	    }
      }
}

static void
parse_wms_GetTileService_HTTP_Post (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetTileService/DCPType/HTTP/Post node */
    xmlNodePtr cur_node = NULL;
    int len;
    const char *p;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "OnlineResource")
		    == 0)
		  {
		      struct _xmlAttr *attr = cur_node->properties;
		      while (attr != NULL)
			{
			    if (attr->name != NULL)
			      {
				  if (strcmp
				      ((const char *) (attr->name),
				       "href") == 0)
				    {
					xmlNodePtr text = attr->children;
					if (text->type == XML_TEXT_NODE)
					  {
					      if (cap->GetTileServiceURLPost !=
						  NULL)
						{
						    free (cap->GetTileServiceURLPost);
						    cap->GetTileServiceURLPost =
							NULL;
						}
					      p = (const char
						   *) (text->content);
					      len = strlen (p);
					      cap->GetTileServiceURLPost =
						  malloc (len + 1);
					      strcpy
						  (cap->GetTileServiceURLPost,
						   p);
					  }
				    }
			      }
			    attr = attr->next;
			}
		  }
	    }
      }
}

static void
parse_wms_GetInfo_HTTP_Get (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetFeatureInfo/DCPType/HTTP/Get node */
    xmlNodePtr cur_node = NULL;
    int len;
    const char *p;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "OnlineResource")
		    == 0)
		  {
		      struct _xmlAttr *attr = cur_node->properties;
		      while (attr != NULL)
			{
			    if (attr->name != NULL)
			      {
				  if (strcmp
				      ((const char *) (attr->name),
				       "href") == 0)
				    {
					xmlNodePtr text = attr->children;
					if (text->type == XML_TEXT_NODE)
					  {
					      if (cap->GetFeatureInfoURLGet !=
						  NULL)
						{
						    free (cap->
							  GetFeatureInfoURLGet);
						    cap->GetFeatureInfoURLGet =
							NULL;
						}
					      p = (const char
						   *) (text->content);
					      len = strlen (p);
					      cap->GetFeatureInfoURLGet =
						  malloc (len + 1);
					      strcpy (cap->GetFeatureInfoURLGet,
						      p);
					  }
				    }
			      }
			    attr = attr->next;
			}
		  }
	    }
      }
}

static void
parse_wms_GetInfo_HTTP_Post (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetFeatureInfo/DCPType/HTTP/Post node */
    xmlNodePtr cur_node = NULL;
    int len;
    const char *p;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "OnlineResource")
		    == 0)
		  {
		      struct _xmlAttr *attr = cur_node->properties;
		      while (attr != NULL)
			{
			    if (attr->name != NULL)
			      {
				  if (strcmp
				      ((const char *) (attr->name),
				       "href") == 0)
				    {
					xmlNodePtr text = attr->children;
					if (text->type == XML_TEXT_NODE)
					  {
					      if (cap->GetFeatureInfoURLPost !=
						  NULL)
						{
						    free (cap->
							  GetFeatureInfoURLPost);
						    cap->GetFeatureInfoURLPost =
							NULL;
						}
					      p = (const char
						   *) (text->content);
					      len = strlen (p);
					      cap->GetFeatureInfoURLPost =
						  malloc (len + 1);
					      strcpy
						  (cap->GetFeatureInfoURLPost,
						   p);
					  }
				    }
			      }
			    attr = attr->next;
			}
		  }
	    }
      }
}

static void
parse_wms_GetMap_HTTP (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetMap/DCPType node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Get") == 0)
		    parse_wms_GetMap_HTTP_Get (cur_node->children, cap);
		if (strcmp ((const char *) (cur_node->name), "Post") == 0)
		    parse_wms_GetMap_HTTP_Post (cur_node->children, cap);
	    }
      }
}

static void
parse_wms_GetTileService_HTTP (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetTileService/DCPType node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Get") == 0)
		    parse_wms_GetTileService_HTTP_Get (cur_node->children, cap);
		if (strcmp ((const char *) (cur_node->name), "Post") == 0)
		    parse_wms_GetTileService_HTTP_Post (cur_node->children,
							cap);
	    }
      }
}

static void
parse_wms_GetInfo_HTTP (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetFeatureInfo/DCPType node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Get") == 0)
		    parse_wms_GetInfo_HTTP_Get (cur_node->children, cap);
		if (strcmp ((const char *) (cur_node->name), "Post") == 0)
		    parse_wms_GetInfo_HTTP_Post (cur_node->children, cap);
	    }
      }
}

static void
parse_wms_GetMap_DCPType (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetMap/DCPType node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "HTTP") == 0)
		    parse_wms_GetMap_HTTP (cur_node->children, cap);
	    }
      }
}

static void
parse_wms_GetInfo_DCPType (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetFeatureInfo/DCPType node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "HTTP") == 0)
		    parse_wms_GetInfo_HTTP (cur_node->children, cap);
	    }
      }
}

static void
parse_wms_GetTileService_DCPType (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetTileService/DCPType node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "HTTP") == 0)
		    parse_wms_GetTileService_HTTP (cur_node->children, cap);
	    }
      }
}

static void
parse_wms_getMap (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetMap node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Format") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  wmsFormatPtr fmt;
				  const char *format =
				      (const char *) (child_node->content);
				  fmt = wmsAllocFormat (format);
				  if (cap->firstFormat == NULL)
				      cap->firstFormat = fmt;
				  if (cap->lastFormat != NULL)
				      cap->lastFormat->next = fmt;
				  cap->lastFormat = fmt;
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "DCPType") == 0)
		    parse_wms_GetMap_DCPType (cur_node->children, cap);
	    }
      }
}

static void
parse_wms_getInfo (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetFeatureInfo node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Format") == 0)
		  {
		      xmlNodePtr child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  if (cap->GmlMimeType == NULL)
				    {
					int ok = 0;
					const char *format =
					    (const char
					     *) (child_node->content);
					if (strcmp (format, "text/gml") == 0)
					    ok = 1;
					if (strcmp
					    (format,
					     "application/vnd.ogc.gml") == 0)
					    ok = 1;
					if (strcmp
					    (format,
					     "application/vnd.ogc.gml/3.1.1") ==
					    0)
					    ok = 1;
					if (ok)
					  {
					      int len = strlen (format);
					      cap->GmlMimeType =
						  malloc (len + 1);
					      strcpy (cap->GmlMimeType, format);
					  }
				    }
				  if (cap->XmlMimeType == NULL)
				    {
					int ok = 0;
					const char *format =
					    (const char
					     *) (child_node->content);
					if (strcmp (format, "text/xml") == 0)
					    ok = 1;
					if (ok)
					  {
					      int len = strlen (format);
					      cap->XmlMimeType =
						  malloc (len + 1);
					      strcpy (cap->XmlMimeType, format);
					  }
				    }
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "DCPType") == 0)
		    parse_wms_GetInfo_DCPType (cur_node->children, cap);
	    }
      }
}

static void
parse_wms_getTileService (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request/GetTileService node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (strcmp ((const char *) (cur_node->name), "DCPType") == 0)
	      parse_wms_GetTileService_DCPType (cur_node->children, cap);
      }
}

static void
parse_wms_request (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability/Request node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "GetMap") == 0)
		    parse_wms_getMap (cur_node->children, cap);
		if (strcmp ((const char *) (cur_node->name), "GetTileService")
		    == 0)
		    parse_wms_getTileService (cur_node->children, cap);
		if (strcmp ((const char *) (cur_node->name), "GetFeatureInfo")
		    == 0)
		    parse_wms_getInfo (cur_node->children, cap);
	    }
      }
}

static void
parse_wms_capability (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities/Capability node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Request") == 0)
		    parse_wms_request (cur_node->children, cap);
		if (strcmp ((const char *) (cur_node->name), "Layer") == 0)
		    parse_wms_layer (cur_node->children, cur_node->properties,
				     cap);
	    }
      }
}

static void
parse_capabilities (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetCapabilities payload */
    xmlNodePtr cur_node = NULL;

    if (node)
	cur_node = node->children;
    else
	return;

    for (; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Service") == 0)
		    parse_wms_service (cur_node->children, cap);
		if (strcmp ((const char *) (cur_node->name), "Capability") == 0)
		    parse_wms_capability (cur_node->children, cap);
	    }
      }
}

static void
parse_version (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* attempting to extract the Version String */
    const char *version = NULL;
    struct _xmlAttr *attr = node->properties;

    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		xmlNodePtr text;
		if (strcmp ((const char *) (attr->name), "version") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  version = (const char *) (text->content);
		  }
	    }
	  attr = attr->next;
      }
    if (version != NULL)
      {
	  int len = strlen (version);
	  if (cap->Version != NULL)
	      free (cap->Version);
	  cap->Version = malloc (len + 1);
	  strcpy (cap->Version, version);
      }
}

static wmsCapabilitiesPtr
parse_wms_capabilities (const char *buf)
{
/* attempting to parse a WMS GetCapabilities answer */
    xmlDocPtr xml_doc;
    xmlNodePtr root;
    wmsMemBuffer xmlErr;
    wmsCapabilitiesPtr cap = NULL;

/* testing if the XMLDocument is well-formed */
    wmsMemBufferInitialize (&xmlErr);
    xmlSetGenericErrorFunc (&xmlErr, wmsParsingError);
    xml_doc = xmlReadMemory (buf, strlen (buf), "GetCapabilities.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  char *err = NULL;
	  const char *p_err = "error unknown";
	  if (xmlErr.Buffer != NULL)
	    {
		err = malloc (xmlErr.WriteOffset + 1);
		memcpy (err, xmlErr.Buffer, xmlErr.WriteOffset);
		*(err + xmlErr.WriteOffset) = '\0';
		p_err = err;
	    }
	  fprintf (stderr, "XML parsing error: %s\n", p_err);
	  if (err)
	      free (err);
	  wmsMemBufferReset (&xmlErr);
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return NULL;
      }
    if (xmlErr.Buffer != NULL)
      {
	  /* reporting some XML warning */
	  char *err = malloc (xmlErr.WriteOffset + 1);
	  memcpy (err, xmlErr.Buffer, xmlErr.WriteOffset);
	  *(err + xmlErr.WriteOffset) = '\0';
	  fprintf (stderr, "XML parsing warning: %s\n", err);
	  free (err);
      }
    wmsMemBufferReset (&xmlErr);

/* parsing XML nodes */
    cap = wmsAllocCapabilities ();
    root = xmlDocGetRootElement (xml_doc);
    parse_version (root, cap);
    parse_capabilities (root, cap);
    xmlFreeDoc (xml_doc);

    return cap;
}

static void
parse_wms_tiled_geoBBox (struct _xmlAttr *properties, wmsTiledLayerPtr lyr)
{
/* parsing a WMS TiledLayer/LatLonBoundingBox */
    struct _xmlAttr *attr = properties;

    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		xmlNodePtr text;
		if (strcmp ((const char *) (attr->name), "miny") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->MinLat = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "maxy") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->MaxLat = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "minx") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->MinLong = atof ((const char *) (text->content));
		  }
		if (strcmp ((const char *) (attr->name), "maxx") == 0)
		  {
		      text = attr->children;
		      if (text->type == XML_TEXT_NODE)
			  lyr->MaxLong = atof ((const char *) (text->content));
		  }
	    }
	  attr = attr->next;
      }
}

static char *
normalize_pattern (const char *pattern)
{
/* normalizing a tile pattern */
    char *out;
    int len;
    const char *p_end = pattern;
    while (1)
      {
	  if (*p_end == ' ' || *p_end == '\0' || *p_end == '\t'
	      || *p_end == '\r' || *p_end == '\n')
	      break;
	  p_end++;
      }
    len = p_end - pattern;
    if (len <= 0)
	return NULL;
    out = malloc (len + 1);
    memcpy (out, pattern, len);
    *(out + len) = '\0';
    return out;
}

static void
parse_wms_tiled_group_child (xmlNodePtr node, wmsTiledLayerPtr parent)
{
/* parsing a WMS Tiled Layer (child) definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    wmsTiledLayerPtr lyr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
	    }
      }
    lyr = wmsAllocTiledLayer (name, title, abstract);
    if (parent->firstChild == NULL)
	parent->firstChild = lyr;
    if (parent->lastChild != NULL)
	parent->lastChild->next = lyr;
    parent->lastChild = lyr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		int len;
		if (strcmp
		    ((const char *) (cur_node->name), "LatLonBoundingBox") == 0)
		    parse_wms_tiled_geoBBox (cur_node->properties, lyr);
		if (strcmp ((const char *) (cur_node->name), "Pad") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  if (lyr->Pad != NULL)
				      free (lyr->Pad);
				  lyr->Pad = NULL;
				  len =
				      strlen ((const char
					       *) (child_node->content));
				  lyr->Pad = malloc (len + 1);
				  strcpy (lyr->Pad,
					  (const char *) (child_node->content));
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Bands") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  if (lyr->Bands != NULL)
				      free (lyr->Bands);
				  lyr->Bands = NULL;
				  len =
				      strlen ((const char
					       *) (child_node->content));
				  lyr->Bands = malloc (len + 1);
				  strcpy (lyr->Bands,
					  (const char *) (child_node->content));
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "DataType") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  if (lyr->DataType != NULL)
				      free (lyr->DataType);
				  lyr->DataType = NULL;
				  len =
				      strlen ((const char
					       *) (child_node->content));
				  lyr->DataType = malloc (len + 1);
				  strcpy (lyr->DataType,
					  (const char *) (child_node->content));
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "TilePattern") ==
		    0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  const char *pattern =
				      (const char *) (child_node->content);
				  char *norm_pattern =
				      normalize_pattern (pattern);
				  if (norm_pattern != NULL)
				    {
					wmsTilePatternPtr pattern =
					    wmsAllocTilePattern (norm_pattern);
					if (lyr->firstPattern == NULL)
					    lyr->firstPattern = pattern;
					if (lyr->lastPattern != NULL)
					    lyr->lastPattern->next = pattern;
					lyr->lastPattern = pattern;
				    }
			      }
			}
		  }
	    }
      }
}

static void
parse_wms_tiled_groups_child (xmlNodePtr node, wmsTiledLayerPtr parent)
{
/* parsing a WMS Tiled (Group) Layer definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    wmsTiledLayerPtr lyr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
	    }
      }
    lyr = wmsAllocTiledLayer (name, title, abstract);
    if (parent->firstChild == NULL)
	parent->firstChild = lyr;
    if (parent->lastChild != NULL)
	parent->lastChild->next = lyr;
    parent->lastChild = lyr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "TiledGroup") == 0)
		    parse_wms_tiled_group_child (cur_node->children, lyr);
		if (strcmp ((const char *) (cur_node->name), "TiledGroups") ==
		    0)
		    parse_wms_tiled_groups_child (cur_node->children, lyr);
	    }
      }
}

static void
parse_wms_tiled_group (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* parsing a WMS Tiled Layer definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    wmsTiledLayerPtr lyr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
	    }
      }
    lyr = wmsAllocTiledLayer (name, title, abstract);
    if (cap->firstTiled == NULL)
	cap->firstTiled = lyr;
    if (cap->lastTiled != NULL)
	cap->lastTiled->next = lyr;
    cap->lastTiled = lyr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		int len;
		if (strcmp
		    ((const char *) (cur_node->name), "LatLonBoundingBox") == 0)
		    parse_wms_tiled_geoBBox (cur_node->properties, lyr);
		if (strcmp ((const char *) (cur_node->name), "Pad") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  if (lyr->Pad != NULL)
				      free (lyr->Pad);
				  lyr->Pad = NULL;
				  len =
				      strlen ((const char
					       *) (child_node->content));
				  lyr->Pad = malloc (len + 1);
				  strcpy (lyr->Pad,
					  (const char *) (child_node->content));
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Bands") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  if (lyr->Bands != NULL)
				      free (lyr->Bands);
				  lyr->Bands = NULL;
				  len =
				      strlen ((const char
					       *) (child_node->content));
				  lyr->Bands = malloc (len + 1);
				  strcpy (lyr->Bands,
					  (const char *) (child_node->content));
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "DataType") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  if (lyr->DataType != NULL)
				      free (lyr->DataType);
				  lyr->DataType = NULL;
				  len =
				      strlen ((const char
					       *) (child_node->content));
				  lyr->DataType = malloc (len + 1);
				  strcpy (lyr->DataType,
					  (const char *) (child_node->content));
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "TilePattern") ==
		    0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
			      {
				  const char *pattern =
				      (const char *) (child_node->content);
				  char *norm_pattern =
				      normalize_pattern (pattern);
				  if (norm_pattern != NULL)
				    {
					wmsTilePatternPtr pattern =
					    wmsAllocTilePattern (norm_pattern);
					if (lyr->firstPattern == NULL)
					    lyr->firstPattern = pattern;
					if (lyr->lastPattern != NULL)
					    lyr->lastPattern->next = pattern;
					lyr->lastPattern = pattern;
				    }
			      }
			}
		  }
	    }
      }
}

static void
parse_wms_tiled_groups (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* parsing a WMS Tiled (Group) Layer definition */
    xmlNodePtr cur_node = NULL;
    xmlNodePtr child_node = NULL;
    const char *name = NULL;
    const char *title = NULL;
    const char *abstract = NULL;
    wmsTiledLayerPtr lyr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				name = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				title = (const char *) (child_node->content);
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				abstract = (const char *) (child_node->content);
			}
		  }
	    }
      }
    lyr = wmsAllocTiledLayer (name, title, abstract);
    if (cap->firstTiled == NULL)
	cap->firstTiled = lyr;
    if (cap->lastTiled != NULL)
	cap->lastTiled->next = lyr;
    cap->lastTiled = lyr;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "TiledGroup") == 0)
		    parse_wms_tiled_group_child (cur_node->children, lyr);
		if (strcmp ((const char *) (cur_node->name), "TiledGroups") ==
		    0)
		    parse_wms_tiled_groups_child (cur_node->children, lyr);
	    }
      }
}

static void
parse_tile_service_info (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetTileService/Service payload */
    xmlNodePtr cur_node = NULL;

    if (node)
	cur_node = node->children;
    else
	return;

    for (; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		int len;
		xmlNodePtr child_node;
		const char *value;
		if (strcmp ((const char *) (cur_node->name), "Name") == 0)
		  {
		      if (cap->TileServiceName != NULL)
			  free (cap->TileServiceName);
		      cap->TileServiceName = NULL;
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				value = (const char *) (child_node->content);
			    if (value != NULL)
			      {
				  len = strlen (value);
				  cap->TileServiceName = malloc (len + 1);
				  strcpy (cap->TileServiceName, value);
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Title") == 0)
		  {
		      if (cap->TileServiceTitle != NULL)
			  free (cap->TileServiceTitle);
		      cap->TileServiceTitle = NULL;
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				value = (const char *) (child_node->content);
			    if (value != NULL)
			      {
				  len = strlen (value);
				  cap->TileServiceTitle = malloc (len + 1);
				  strcpy (cap->TileServiceTitle, value);
			      }
			}
		  }
		if (strcmp ((const char *) (cur_node->name), "Abstract") == 0)
		  {
		      if (cap->TileServiceAbstract != NULL)
			  free (cap->TileServiceAbstract);
		      cap->TileServiceAbstract = NULL;
		      child_node = cur_node->children;
		      if (child_node != NULL)
			{
			    if (child_node->type == XML_TEXT_NODE)
				value = (const char *) (child_node->content);
			    if (value != NULL)
			      {
				  len = strlen (value);
				  cap->TileServiceAbstract = malloc (len + 1);
				  strcpy (cap->TileServiceAbstract, value);
			      }
			}
		  }
	    }
      }
}

static void
parse_tiled_patterns (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetTileService/TiledPatterns payload */
    xmlNodePtr cur_node = NULL;

    if (node)
	cur_node = node->children;
    else
	return;

    for (; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "TiledGroup") == 0)
		    parse_wms_tiled_group (cur_node->children, cap);
		if (strcmp ((const char *) (cur_node->name), "TiledGroups") ==
		    0)
		    parse_wms_tiled_groups (cur_node->children, cap);
	    }
      }
}

static void
parse_tile_service (xmlNodePtr node, wmsCapabilitiesPtr cap)
{
/* recursively parsing the GetTileService payload */
    xmlNodePtr cur_node = NULL;

    if (node)
	cur_node = node->children;
    else
	return;

    for (; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Service") == 0)
		    parse_tile_service_info (cur_node, cap);
		if (strcmp ((const char *) (cur_node->name), "TiledPatterns") ==
		    0)
		    parse_tiled_patterns (cur_node, cap);
	    }
      }
}

static void
parse_wms_get_tile_service (wmsCapabilitiesPtr capabilities, const char *buf)
{
/* attempting to parse a WMS GetTileService answer */
    xmlDocPtr xml_doc;
    xmlNodePtr root;
    wmsMemBuffer xmlErr;

/* testing if the XMLDocument is well-formed */
    wmsMemBufferInitialize (&xmlErr);
    xmlSetGenericErrorFunc (&xmlErr, wmsParsingError);
    xml_doc = xmlReadMemory (buf, strlen (buf), "GetTileService.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  char *err = NULL;
	  const char *p_err = "error unknown";
	  if (xmlErr.Buffer != NULL)
	    {
		err = malloc (xmlErr.WriteOffset + 1);
		memcpy (err, xmlErr.Buffer, xmlErr.WriteOffset);
		*(err + xmlErr.WriteOffset) = '\0';
		p_err = err;
	    }
	  fprintf (stderr, "XML parsing error: %s\n", p_err);
	  if (err)
	      free (err);
	  wmsMemBufferReset (&xmlErr);
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return;
      }
    if (xmlErr.Buffer != NULL)
      {
	  /* reporting some XML warning */
	  char *err = malloc (xmlErr.WriteOffset + 1);
	  memcpy (err, xmlErr.Buffer, xmlErr.WriteOffset);
	  *(err + xmlErr.WriteOffset) = '\0';
	  fprintf (stderr, "XML parsing warning: %s\n", err);
	  free (err);
      }
    wmsMemBufferReset (&xmlErr);

/* parsing XML nodes */
    root = xmlDocGetRootElement (xml_doc);
    parse_tile_service (root, capabilities);
    xmlFreeDoc (xml_doc);
}

static void
parse_wms_gml_geom (wmsMemBufferPtr gmlBuf, xmlNodePtr node)
{
/* recursively reassembling the GML Geometry */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		/* found some internal GML Geometry Tag */
		struct _xmlAttr *attr;
		char *tag_name;
		if (cur_node->ns == NULL)
		    tag_name = sqlite3_mprintf ("<%s", cur_node->name);
		else
		    tag_name =
			sqlite3_mprintf ("<%s:%s", cur_node->ns->prefix,
					 cur_node->name);
		wmsMemBufferAppend (gmlBuf, (unsigned char *) tag_name,
				    strlen (tag_name));
		sqlite3_free (tag_name);
		attr = cur_node->properties;
		while (attr != NULL)
		  {
		      /* eventual node attributes */
		      if (attr->type == XML_ATTRIBUTE_NODE)
			{
			    const char *value = "";
			    xmlNodePtr text = attr->children;
			    if (text != NULL)
			      {
				  if (text->type == XML_TEXT_NODE)
				      value = (const char *) (text->content);
			      }
			    if (attr->ns == NULL)
				tag_name =
				    sqlite3_mprintf (" %s=\"%s\"", attr->name,
						     value);
			    else
				tag_name =
				    sqlite3_mprintf (" %s:%s=\"%s\"",
						     attr->ns->prefix,
						     attr->name, value);
			    wmsMemBufferAppend (gmlBuf,
						(unsigned char *) tag_name,
						strlen (tag_name));
			    sqlite3_free (tag_name);
			}
		      attr = attr->next;
		  }
		wmsMemBufferAppend (gmlBuf, (unsigned char *) ">", 1);
		parse_wms_gml_geom (gmlBuf, cur_node->children);
		if (cur_node->ns == NULL)
		    tag_name = sqlite3_mprintf ("</%s>", cur_node->name);
		else
		    tag_name =
			sqlite3_mprintf ("</%s:%s>", cur_node->ns->prefix,
					 cur_node->name);
		wmsMemBufferAppend (gmlBuf, (unsigned char *) tag_name,
				    strlen (tag_name));
		sqlite3_free (tag_name);
	    }
	  if (cur_node->type == XML_TEXT_NODE)
	    {
		/* found a Text item */
		wmsMemBufferAppend (gmlBuf,
				    (unsigned char *) (cur_node->content),
				    strlen ((const char
					     *) (cur_node->content)));
	    }
      }
}

static void
parse_wms_feature_attribute (xmlNodePtr node, wmsFeatureMemberPtr member)
{
/* parsing the GetFeatureInfo/featureAttribute node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		xmlNodePtr text = cur_node->children;
		if (text == NULL)
		    continue;
		if (text->type == XML_TEXT_NODE)
		  {
		      /* found an attribute */
		      {
			  char *value = NULL;
			  if (text->content != NULL)
			    {
				int len =
				    strlen ((const char *) (text->content));
				value = malloc (len + 1);
				strcpy (value, (const char *) (text->content));
			    }
			  wmsAddFeatureMemberAttribute (member,
							(const char
							 *) (cur_node->name),
							value);
		      }
		  }
		if (text->type == XML_ELEMENT_NODE)
		  {
		      /* probably found the GML Geometry - attempting to reassemble */
		      char *gml = NULL;
		      wmsMemBuffer gmlBuf;
		      wmsMemBufferInitialize (&gmlBuf);
		      parse_wms_gml_geom (&gmlBuf, text);
		      if (gmlBuf.WriteOffset > 0)
			{
			    gml = malloc (gmlBuf.WriteOffset + 1);
			    memcpy (gml, gmlBuf.Buffer, gmlBuf.WriteOffset);
			    *(gml + gmlBuf.WriteOffset) = '\0';
			}
		      wmsMemBufferReset (&gmlBuf);
		      wmsAddFeatureMemberAttribute (member,
						    (const char
						     *) (cur_node->name), gml);
		  }
	    }
      }
}


static void
parse_wms_feature_member (xmlNodePtr node, wmsFeatureCollectionPtr coll)
{
/* parsing the GetFeatureInfo/featureMember node */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {

		wmsFeatureMemberPtr member =
		    wmsAllocFeatureMember ((const char *) (cur_node->name));
		parse_wms_feature_attribute (cur_node->children, member);
		if (member->first == NULL)
		  {
		      /* empty feature */
		      wmsFreeFeatureMember (member);
		  }
		/* appending the feature to the collection */
		if (coll->first == NULL)
		    coll->first = member;
		if (coll->last != NULL)
		    coll->last->next = member;
		coll->last = member;
	    }
      }
}

static void
parse_ms_layer (xmlNodePtr node, wmsFeatureCollectionPtr coll,
		const char *feature_name)
{
/* recursively parsing msGMLOutput features from a layer */
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), feature_name) == 0)
		    parse_wms_feature_member (cur_node, coll);
		else
		    parse_ms_layer (cur_node->children, coll, feature_name);
	    }
      }
}

static char *
make_feature_name (const char *layer_name)
{
/* building the expected feature name */
    char *name;
    int len = strlen (layer_name);
    if (len <= 6)
	return NULL;
    if (strcmp (layer_name + len - 6, "_layer") != 0)
	return NULL;
    name = malloc (len - 6 + 9);
    strncpy (name, layer_name, len - 6);
    name[len - 6] = '\0';
    strcat (name, "_feature");
    return name;
}

static void
parse_ms_gml_output (xmlNodePtr node, wmsFeatureCollectionPtr coll)
{
/* parsing the msGMLOutput payload */
    xmlNodePtr cur_node = NULL;
    char *feature_name = NULL;
    if (strcmp ((const char *) (node->name), "msGMLOutput") != 0)
	return;

    if (node)
	cur_node = node->children;
    else
	return;

    for (; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (feature_name == NULL)
		    feature_name =
			make_feature_name ((const char *) (cur_node->name));
		if (feature_name == NULL)
		    continue;
		parse_ms_layer (cur_node->children, coll, feature_name);
	    }
      }
    if (feature_name != NULL)
	free (feature_name);
}

static void
parse_esri_xml_field (struct _xmlAttr *properties, wmsFeatureMemberPtr member)
{
/* parsing an ESRI <Fields> based on XML attributes */
    struct _xmlAttr *attr = properties;

    while (attr != NULL)
      {
	  if (attr->name != NULL)
	    {
		char *value = NULL;
		xmlNodePtr text = attr->children;
		if (text != NULL)
		  {
		      if (text->type == XML_TEXT_NODE)
			{
			    if (text->content != NULL)
			      {
				  int len =
				      strlen ((const char *) (text->content));
				  value = malloc (len + 1);
				  strcpy (value,
					  (const char *) (text->content));
			      }
			}
		  }
		else
		  {
		      value = malloc (1);
		      *value = '\0';
		  }
		wmsAddFeatureMemberAttribute (member,
					      (const char *) (attr->name),
					      value);
	    }
	  attr = attr->next;
      }
}

static void
parse_esri_xml_output (xmlNodePtr node, wmsFeatureCollectionPtr coll)
{
/* parsing the ESRI-like XML payload */
    xmlNodePtr cur_node = NULL;
    if (strcmp ((const char *) (node->name), "FeatureInfoResponse") != 0)
	return;

    if (node)
	cur_node = node->children;
    else
	return;

    for (; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "Fields") != 0)
		  {
		      struct _xmlAttr *attr = cur_node->properties;
		      if (attr != NULL)
			{
			    wmsFeatureMemberPtr member =
				wmsAllocFeatureMember ((const char
							*) (cur_node->name));
			    parse_esri_xml_field (attr, member);
			    /* appending the feature to the collection */
			    if (coll->first == NULL)
				coll->first = member;
			    if (coll->last != NULL)
				coll->last->next = member;
			    coll->last = member;
			}
		  }
	    }
      }
}

static void
parse_feature_collection (xmlNodePtr node, wmsFeatureCollectionPtr coll)
{
/* parsing the GetFeatureInfo payload */
    xmlNodePtr cur_node = NULL;
    if (strcmp ((const char *) (node->name), "FeatureInfoResponse") == 0)
      {
	  parse_esri_xml_output (node, coll);
	  return;
      }
    if (strcmp ((const char *) (node->name), "msGMLOutput") == 0)
      {
	  parse_ms_gml_output (node, coll);
	  return;
      }
    if (strcmp ((const char *) (node->name), "FeatureCollection") != 0)
	return;

    if (node)
	cur_node = node->children;
    else
	return;

    for (; cur_node; cur_node = cur_node->next)
      {
	  if (cur_node->type == XML_ELEMENT_NODE)
	    {
		if (strcmp ((const char *) (cur_node->name), "featureMember") ==
		    0)
		    parse_wms_feature_member (cur_node->children, coll);
	    }
      }
}

static wmsFeatureCollectionPtr
parse_wms_feature_collection (const char *buf)
{
/* attempting to parse a WMS GetFeatureInfo answer */
    xmlDocPtr xml_doc;
    xmlNodePtr root;
    wmsMemBuffer xmlErr;
    wmsFeatureCollectionPtr coll = NULL;

/* testing if the XMLDocument is well-formed */
    wmsMemBufferInitialize (&xmlErr);
    xmlSetGenericErrorFunc (&xmlErr, wmsParsingError);
    xml_doc = xmlReadMemory (buf, strlen (buf), "GetFeatureInfo.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  char *err = NULL;
	  const char *p_err = "error unknown";
	  if (xmlErr.Buffer != NULL)
	    {
		err = malloc (xmlErr.WriteOffset + 1);
		memcpy (err, xmlErr.Buffer, xmlErr.WriteOffset);
		*(err + xmlErr.WriteOffset) = '\0';
		p_err = err;
	    }
	  fprintf (stderr, "XML parsing error: %s\n", p_err);
	  if (err)
	      free (err);
	  wmsMemBufferReset (&xmlErr);
	  xmlSetGenericErrorFunc ((void *) stderr, NULL);
	  return NULL;
      }
    if (xmlErr.Buffer != NULL)
      {
	  /* reporting some XML warning */
	  char *err = malloc (xmlErr.WriteOffset + 1);
	  memcpy (err, xmlErr.Buffer, xmlErr.WriteOffset);
	  *(err + xmlErr.WriteOffset) = '\0';
	  fprintf (stderr, "XML parsing warning: %s\n", err);
	  free (err);
      }
    wmsMemBufferReset (&xmlErr);

/* parsing XML nodes */
    coll = wmsAllocFeatureCollection ();
    root = xmlDocGetRootElement (xml_doc);
    parse_feature_collection (root, coll);
    xmlFreeDoc (xml_doc);

    if (coll != NULL)
      {
	  if (coll->first == NULL)
	    {
		/* empty collection */
		wmsFreeFeatureCollection (coll);
		coll = NULL;
	    }
      }

    return coll;
}

static int
query_TileService (rl2WmsCachePtr cache_handle, wmsCapabilitiesPtr capabilities,
		   const char *proxy)
{
/* attempting to get and parse a WMS GetTileService request */
    CURL *curl = NULL;
    CURLcode res;
    wmsMemBuffer headerBuf;
    wmsMemBuffer bodyBuf;
    int http_status;
    char *http_code;
    char *xml_buf;
    char *url;
    wmsCachePtr cache = (wmsCachePtr) cache_handle;
    int already_cached = 0;
    int retcode = 0;

/* initializes the dynamically growing buffers */
    wmsMemBufferInitialize (&headerBuf);
    wmsMemBufferInitialize (&bodyBuf);
    url =
	sqlite3_mprintf ("%srequest=GetTileService",
			 capabilities->GetTileServiceURLGet);

    if (cache != NULL)
      {
	  /* checks if it's already stored into the WMS Cache */
	  wmsCachedCapabilitiesPtr cachedCapab =
	      getWmsCachedCapabilities (cache, url);
	  if (cachedCapab != NULL)
	    {
		/* ok, found from WMS Cache */
		xml_buf =
		    clean_xml_str ((const char *) (cachedCapab->Response));
		already_cached = 1;
		goto do_tile_service;
	    }
      }

    curl = curl_easy_init ();
    if (curl)
      {
	  /* setting the URL */
	  curl_easy_setopt (curl, CURLOPT_URL, url);

	  if (proxy != NULL)
	    {
		/* setting up the required proxy */
		curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
	    }

	  /* no progress meter please */
	  curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1L);
	  /* setting the output callback function */
	  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, store_data);
	  curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &headerBuf);
	  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &bodyBuf);

	  /* Perform the request, res will get the return code */
	  res = curl_easy_perform (curl);
	  /* Check for errors */
	  if (res != CURLE_OK)
	    {
		fprintf (stderr, "CURL error: %s\n", curl_easy_strerror (res));
		goto stop;
	    }

	  /* verifying the HTTP status code */
	  check_http_header (&headerBuf, &http_status, &http_code);
	  if (http_status != 200)
	    {
		fprintf (stderr, "Invalid HTTP status code: %d %s\n",
			 http_status, http_code);
		if (http_code != NULL)
		    free (http_code);
		goto stop;
	    }
	  if (http_code != NULL)
	      free (http_code);
      }

    /* attempting to parse the GetCapabilities answer */
    xml_buf = clean_xml (&bodyBuf);
  do_tile_service:
    if (xml_buf != NULL)
      {
	  parse_wms_get_tile_service (capabilities, xml_buf);
	  free (xml_buf);
	  retcode = 1;
      }
    if (!already_cached)
      {
	  /* saving into the WMS Cache */
	  wmsAddCachedCapabilities (cache, url, bodyBuf.Buffer,
				    bodyBuf.WriteOffset);
      }

    /* memory cleanup */
  stop:
    wmsMemBufferReset (&headerBuf);
    wmsMemBufferReset (&bodyBuf);
    sqlite3_free (url);
    if (curl != NULL)
	curl_easy_cleanup (curl);
    return retcode;
}

RL2_DECLARE rl2WmsCatalogPtr
create_wms_catalog (rl2WmsCachePtr cache_handle, const char *url,
		    const char *proxy, char **err_msg)
{
/* attempting to get and parse a WMS GetCapabilities request */
    CURL *curl = NULL;
    CURLcode res;
    wmsMemBuffer headerBuf;
    wmsMemBuffer bodyBuf;
    int http_status;
    char *http_code;
    wmsCapabilitiesPtr capabilities = NULL;
    char *xml_buf;
    wmsCachePtr cache = (wmsCachePtr) cache_handle;
    int already_cached = 0;

/* initializes the dynamically growing buffers */
    wmsMemBufferInitialize (&headerBuf);
    wmsMemBufferInitialize (&bodyBuf);

    if (cache != NULL)
      {
	  /* checks if it's already stored into the WMS Cache */
	  wmsCachedCapabilitiesPtr cachedCapab =
	      getWmsCachedCapabilities (cache, url);
	  if (cachedCapab != NULL)
	    {
		/* ok, found from WMS Cache */
		xml_buf =
		    clean_xml_str ((const char *) (cachedCapab->Response));
		already_cached = 1;
		goto do_capabilities;
	    }
      }

    *err_msg = NULL;
    curl = curl_easy_init ();
    if (curl)
      {
	  /* setting the URL */
	  curl_easy_setopt (curl, CURLOPT_URL, url);

	  if (proxy != NULL)
	    {
		/* setting up the required proxy */
		curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
	    }

	  /* no progress meter please */
	  curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1L);
	  /* setting the output callback function */
	  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, store_data);
	  curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &headerBuf);
	  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &bodyBuf);

	  /* Perform the request, res will get the return code */
	  res = curl_easy_perform (curl);
	  /* Check for errors */
	  if (res != CURLE_OK)
	    {
		fprintf (stderr, "CURL error: %s\n", curl_easy_strerror (res));
		goto stop;
	    }

	  /* verifying the HTTP status code */
	  check_http_header (&headerBuf, &http_status, &http_code);
	  if (http_status != 200)
	    {
		fprintf (stderr, "Invalid HTTP status code: %d %s\n",
			 http_status, http_code);
		if (http_code != NULL)
		    free (http_code);
		goto stop;
	    }
	  if (http_code != NULL)
	      free (http_code);

	  /* attempting to parse the GetCapabilities answer */
	  xml_buf = clean_xml (&bodyBuf);
	do_capabilities:
	  if (xml_buf != NULL)
	    {
		capabilities = parse_wms_capabilities (xml_buf);
		free (xml_buf);
	    }

	  if (capabilities != NULL)
	    {
		if (capabilities->GetTileServiceURLGet != NULL)
		  {
		      /* attempting to resolve WMS GetTileService */
		      int retry = 0;
		      /* first attempt */
		      if (!query_TileService
			  (cache_handle, capabilities, proxy))
			  retry = 1;
		      if (retry)
			{
			    /* second attempt */
			    retry = 0;
			    if (!query_TileService
				(cache_handle, capabilities, proxy))
				retry = 1;
			}
		      if (retry)
			{
			    /* third attempt */
			    retry = 0;
			    if (!query_TileService
				(cache_handle, capabilities, proxy))
				retry = 1;
			}
		      if (retry)
			{
			    /* fourth attempt */
			    retry = 0;
			    if (!query_TileService
				(cache_handle, capabilities, proxy))
				retry = 1;
			}
		      if (retry)
			{
			    /* fifth attempt */
			    if (!query_TileService
				(cache_handle, capabilities, proxy))
			      {
				  /* giving up */
				  wmsFreeCapabilities (capabilities);
				  capabilities = NULL;
				  goto stop;
			      }
			}
		  }
		if (!already_cached)
		  {
		      /* saving into the WMS Cache */
		      wmsAddCachedCapabilities (cache, url, bodyBuf.Buffer,
						bodyBuf.WriteOffset);
		  }
	    }
      }

  stop:
    if (curl != NULL)
	curl_easy_cleanup (curl);
    wmsMemBufferReset (&headerBuf);
    wmsMemBufferReset (&bodyBuf);
    return (rl2WmsCatalogPtr) capabilities;
}

RL2_DECLARE void
destroy_wms_catalog (rl2WmsCatalogPtr handle)
{
/* memory cleanup: freeing a WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return;
    wmsFreeCapabilities (ptr);
}

RL2_DECLARE int
get_wms_format_count (rl2WmsCatalogPtr handle, int mode)
{
/* counting how many Formats are supported by a WMS-Catalog */
    int count = 0;
    wmsFormatPtr fmt;
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return -1;

    fmt = ptr->firstFormat;
    while (fmt != NULL)
      {
	  if (mode)
	    {
		if (fmt->FormatCode != WMS_FORMAT_UNKNOWN)
		    count++;
	    }
	  else
	      count++;
	  fmt = fmt->next;
      }
    return count;
}

RL2_DECLARE const char *
get_wms_format (rl2WmsCatalogPtr handle, int index, int mode)
{
/* attempting to get the Nth Format supported by some WMS-Catalog object */
    int count = 0;
    wmsFormatPtr fmt;
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    fmt = ptr->firstFormat;
    while (fmt != NULL)
      {
	  if (mode)
	    {
		if (fmt->FormatCode == WMS_FORMAT_UNKNOWN)
		  {
		      fmt = fmt->next;
		      continue;
		  }
	    }
	  if (count == index)
	      return fmt->Format;
	  count++;
	  fmt = fmt->next;
      }
    return NULL;
}

RL2_DECLARE const char *
get_wms_contact_person (rl2WmsCatalogPtr handle)
{
/* attempting to get the Contact Person defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->ContactPerson;
}

RL2_DECLARE const char *
get_wms_contact_organization (rl2WmsCatalogPtr handle)
{
/* attempting to get the Contact Organization defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->ContactOrganization;
}

RL2_DECLARE const char *
get_wms_contact_position (rl2WmsCatalogPtr handle)
{
/* attempting to get the Contact Position defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->ContactPosition;
}

RL2_DECLARE const char *
get_wms_contact_postal_address (rl2WmsCatalogPtr handle)
{
/* attempting to get the Postal Address defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->PostalAddress;
}

RL2_DECLARE const char *
get_wms_contact_city (rl2WmsCatalogPtr handle)
{
/* attempting to get the City (Postal Address) defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->City;
}

RL2_DECLARE const char *
get_wms_contact_state_province (rl2WmsCatalogPtr handle)
{
/* attempting to get the State or Province (Postal Address) defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->StateProvince;
}

RL2_DECLARE const char *
get_wms_contact_post_code (rl2WmsCatalogPtr handle)
{
/* attempting to get the Post Code (Postal Address) defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->PostCode;
}

RL2_DECLARE const char *
get_wms_contact_country (rl2WmsCatalogPtr handle)
{
/* attempting to get the Country (Postal Address) defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->Country;
}

RL2_DECLARE const char *
get_wms_contact_voice_telephone (rl2WmsCatalogPtr handle)
{
/* attempting to get the Voice Telephone defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->VoiceTelephone;
}

RL2_DECLARE const char *
get_wms_contact_fax_telephone (rl2WmsCatalogPtr handle)
{
/* attempting to get the FAX Telephone defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->FaxTelephone;
}

RL2_DECLARE const char *
get_wms_contact_email_address (rl2WmsCatalogPtr handle)
{
/* attempting to get the e-mail Address defined by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->EMailAddress;
}

RL2_DECLARE const char *
get_wms_fees (rl2WmsCatalogPtr handle)
{
/* attempting to get the Fees required by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->Fees;
}

RL2_DECLARE const char *
get_wms_access_constraints (rl2WmsCatalogPtr handle)
{
/* attempting to get the Access Constraints imposed by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->AccessConstraints;
}

RL2_DECLARE int
get_wms_layer_limit (rl2WmsCatalogPtr handle)
{
/* attempting to get the LayerLimit supported by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return -1;

    return ptr->LayerLimit;
}

RL2_DECLARE int
get_wms_max_width (rl2WmsCatalogPtr handle)
{
/* attempting to get the MaxWidth supported by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return -1;

    return ptr->MaxWidth;
}

RL2_DECLARE int
get_wms_max_height (rl2WmsCatalogPtr handle)
{
/* attempting to get the MaxHeight supported by some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return -1;

    return ptr->MaxHeight;
}

RL2_DECLARE const char *
get_wms_version (rl2WmsCatalogPtr handle)
{
/* attempting to get the Version String from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->Version;
}

RL2_DECLARE const char *
get_wms_name (rl2WmsCatalogPtr handle)
{
/* attempting to get the Name from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->Name;
}

RL2_DECLARE const char *
get_wms_title (rl2WmsCatalogPtr handle)
{
/* attempting to get the Title from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->Title;
}

RL2_DECLARE const char *
get_wms_abstract (rl2WmsCatalogPtr handle)
{
/* attempting to get the Abstract from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->Abstract;
}

RL2_DECLARE const char *
get_wms_url_GetMap_get (rl2WmsCatalogPtr handle)
{
/* attempting to get the GetMap URL (method GET) from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->GetMapURLGet;
}

RL2_DECLARE const char *
get_wms_url_GetMap_post (rl2WmsCatalogPtr handle)
{
/* attempting to get the GetMap URL (method POST) from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->GetMapURLPost;
}

RL2_DECLARE const char *
get_wms_url_GetTileService_get (rl2WmsCatalogPtr handle)
{
/* attempting to get the GetTileService URL (method GET) from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->GetTileServiceURLGet;
}

RL2_DECLARE const char *
get_wms_url_GetTileService_post (rl2WmsCatalogPtr handle)
{
/* attempting to get the GetTileService URL (method POST) from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->GetTileServiceURLPost;
}

RL2_DECLARE const char *
get_wms_url_GetFeatureInfo_get (rl2WmsCatalogPtr handle)
{
/* attempting to get the GetFeatureInfo URL (method GET) from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->GetFeatureInfoURLGet;
}

RL2_DECLARE const char *
get_wms_url_GetFeatureInfo_post (rl2WmsCatalogPtr handle)
{
/* attempting to get the GetFeatureInfo URL (method POST) from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->GetFeatureInfoURLPost;
}

RL2_DECLARE const char *
get_wms_gml_mime_type (rl2WmsCatalogPtr handle)
{
/* attempting to get the GML MIME type from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->GmlMimeType;
}

RL2_DECLARE const char *
get_wms_xml_mime_type (rl2WmsCatalogPtr handle)
{
/* attempting to get the XML MIME type from some WMS-Catalog object */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->XmlMimeType;
}

RL2_DECLARE int
is_wms_tile_service (rl2WmsCatalogPtr handle)
{
/* testing if a WMS-Catalog actually is a TileService */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return -1;

    if (ptr->firstTiled != NULL)
	return 1;
    return 0;
}

RL2_DECLARE const char *
get_wms_tile_service_name (rl2WmsCatalogPtr handle)
{
/* return the TileService name */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->TileServiceName;
}

RL2_DECLARE const char *
get_wms_tile_service_title (rl2WmsCatalogPtr handle)
{
/* return the TileService title */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->TileServiceTitle;
}

RL2_DECLARE const char *
get_wms_tile_service_abstract (rl2WmsCatalogPtr handle)
{
/* return the TileService abstract */
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    return ptr->TileServiceAbstract;
}

RL2_DECLARE int
get_wms_tile_service_count (rl2WmsCatalogPtr handle)
{
/* counting how many first-level tiled layers are defined within a WMS-Catalog */
    int count = 0;
    wmsTiledLayerPtr lyr;
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return -1;

    lyr = ptr->firstTiled;
    while (lyr != NULL)
      {
	  count++;
	  lyr = lyr->next;
      }
    return count;
}

RL2_DECLARE rl2WmsTiledLayerPtr
get_wms_catalog_tiled_layer (rl2WmsCatalogPtr handle, int index)
{
/* attempting to get a reference to some WMS-TiledLayer object */
    int count = 0;
    wmsTiledLayerPtr lyr;
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    lyr = ptr->firstTiled;
    while (lyr != NULL)
      {
	  if (count == index)
	      return (rl2WmsTiledLayerPtr) lyr;
	  count++;
	  lyr = lyr->next;
      }
    return NULL;
}

RL2_DECLARE int
wms_tiled_layer_has_children (rl2WmsTiledLayerPtr handle)
{
/* testing if a WMS-TiledLayer object has TiledLayer children */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return 0;
    if (ptr->firstChild == NULL)
	return 0;
    return 1;
}

RL2_DECLARE int
get_wms_tiled_layer_children_count (rl2WmsTiledLayerPtr handle)
{
/* counting how many children tiled layers are defined within a WMS-TiledLayer */
    int count = 0;
    wmsTiledLayerPtr lyr;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    lyr = ptr->firstChild;
    while (lyr != NULL)
      {
	  count++;
	  lyr = lyr->next;
      }
    return count;
}

RL2_DECLARE rl2WmsTiledLayerPtr
get_wms_child_tiled_layer (rl2WmsTiledLayerPtr handle, int index)
{
/* attempting to get a reference to some child WMS-TiledLayer object */
    int count = 0;
    wmsTiledLayerPtr lyr;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;

    lyr = ptr->firstChild;
    while (lyr != NULL)
      {
	  if (count == index)
	      return (rl2WmsTiledLayerPtr) lyr;
	  count++;
	  lyr = lyr->next;
      }
    return NULL;
}

RL2_DECLARE const char *
get_wms_tiled_layer_name (rl2WmsTiledLayerPtr handle)
{
/* return the name corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->Name;
}

RL2_DECLARE const char *
get_wms_tiled_layer_title (rl2WmsTiledLayerPtr handle)
{
/* return the title corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->Title;
}

RL2_DECLARE const char *
get_wms_tiled_layer_abstract (rl2WmsTiledLayerPtr handle)
{
/* return the abstract corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->Abstract;
}

RL2_DECLARE const char *
get_wms_tiled_layer_pad (rl2WmsTiledLayerPtr handle)
{
/* return the Pad corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->Pad;
}

RL2_DECLARE const char *
get_wms_tiled_layer_bands (rl2WmsTiledLayerPtr handle)
{
/* return the Bands corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->Bands;
}

RL2_DECLARE const char *
get_wms_tiled_layer_data_type (rl2WmsTiledLayerPtr handle)
{
/* return the Bands corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->DataType;
}

RL2_DECLARE int
get_wms_tiled_layer_bbox (rl2WmsTiledLayerPtr handle, double *minx,
			  double *miny, double *maxx, double *maxy)
{
/* return the BBox corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return 0;
    *minx = ptr->MinLong;
    *miny = ptr->MinLat;
    *maxx = ptr->MaxLong;
    *maxy = ptr->MaxLat;
    return 1;
}

RL2_DECLARE int
get_wms_tiled_layer_tile_size (rl2WmsTiledLayerPtr handle, int *width,
			       int *height)
{
/* return the Tile Size corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return 0;
    if (ptr->firstPattern == NULL)
	return 0;
    *width = ptr->firstPattern->TileWidth;
    *height = ptr->firstPattern->TileHeight;
    return 1;
}

RL2_DECLARE const char *
get_wms_tiled_layer_crs (rl2WmsTiledLayerPtr handle)
{
/* return the CRS corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    if (ptr->firstPattern == NULL)
	return NULL;
    return ptr->firstPattern->SRS;
}

RL2_DECLARE const char *
get_wms_tiled_layer_format (rl2WmsTiledLayerPtr handle)
{
/* return the Format corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    if (ptr->firstPattern == NULL)
	return NULL;
    return ptr->firstPattern->Format;
}

RL2_DECLARE const char *
get_wms_tiled_layer_style (rl2WmsTiledLayerPtr handle)
{
/* return the Style corresponding to a WMS-TiledLayer object */
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    if (ptr->firstPattern == NULL)
	return NULL;
    return ptr->firstPattern->Style;
}

RL2_DECLARE int
get_wms_tile_pattern_count (rl2WmsTiledLayerPtr handle)
{
/* counting how many TilePatterns are defined within a WMS-TiledLayer object */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  count++;
	  pattern = pattern->next;
      }
    return count;
}

RL2_DECLARE rl2WmsTilePatternPtr
get_wms_tile_pattern_handle (rl2WmsTiledLayerPtr handle, int index)
{
/* return the Handle from a TilePattern (identified by its index) */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  if (count == index)
	      return (rl2WmsTilePatternPtr) pattern;
	  count++;
	  pattern = pattern->next;
      }
    return NULL;
}

RL2_DECLARE rl2WmsTilePatternPtr
clone_wms_tile_pattern (rl2WmsTilePatternPtr handle)
{
/* clones a WMS TilePattern object */
    char *str;
    int len;
    wmsTilePatternPtr clone;
    wmsTilePatternPtr pattern = (wmsTilePatternPtr) handle;
    if (pattern == NULL)
	return NULL;

    len = strlen (pattern->Pattern);
    str = malloc (len + 1);
    strcpy (str, pattern->Pattern);
    clone = wmsAllocTilePattern (str);
    return (rl2WmsTilePatternPtr) clone;
}

RL2_DECLARE void
destroy_wms_tile_pattern (rl2WmsTilePatternPtr handle)
{
/* memory cleanup - finalizes a WMS-TilePattern object */
    wmsTilePatternPtr pattern = (wmsTilePatternPtr) handle;
    if (pattern == NULL)
	return;
    wmsFreeTilePattern (pattern);
}

RL2_DECLARE const char *
get_wms_tile_pattern_srs (rl2WmsTiledLayerPtr handle, int index)
{
/* return the SRS from a TilePattern (identified by its index) */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return NULL;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  if (count == index)
	      return pattern->SRS;
	  count++;
	  pattern = pattern->next;
      }
    return NULL;
}

RL2_DECLARE int
get_wms_tile_pattern_tile_width (rl2WmsTiledLayerPtr handle, int index)
{
/* return the TileWidth from a TilePattern (identified by its index) */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  if (count == index)
	      return pattern->TileWidth;
	  count++;
	  pattern = pattern->next;
      }
    return -1;
}

RL2_DECLARE int
get_wms_tile_pattern_tile_height (rl2WmsTiledLayerPtr handle, int index)
{
/* return the TileHeight from a TilePattern (identified by its index) */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  if (count == index)
	      return pattern->TileHeight;
	  count++;
	  pattern = pattern->next;
      }
    return -1;
}

RL2_DECLARE double
get_wms_tile_pattern_base_x (rl2WmsTiledLayerPtr handle, int index)
{
/* return the TileBaseX (leftmost coord) from a TilePattern (identified by its index) */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return DBL_MAX;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  if (count == index)
	      return pattern->TileBaseX;
	  count++;
	  pattern = pattern->next;
      }
    return DBL_MAX;
}

RL2_DECLARE double
get_wms_tile_pattern_base_y (rl2WmsTiledLayerPtr handle, int index)
{
/* return the TileBaseY (topmost coord) from a TilePattern (identified by its index) */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return DBL_MAX;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  if (count == index)
	      return pattern->TileBaseY;
	  count++;
	  pattern = pattern->next;
      }
    return DBL_MAX;
}

RL2_DECLARE double
get_wms_tile_pattern_extent_x (rl2WmsTiledLayerPtr handle, int index)
{
/* return the TileExtentX from a TilePattern (identified by its index) */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return DBL_MAX;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  if (count == index)
	      return pattern->TileExtentX;
	  count++;
	  pattern = pattern->next;
      }
    return DBL_MAX;
}

RL2_DECLARE double
get_wms_tile_pattern_extent_y (rl2WmsTiledLayerPtr handle, int index)
{
/* return the TileExtentY from a TilePattern (identified by its index) */
    int count = 0;
    wmsTilePatternPtr pattern;
    wmsTiledLayerPtr ptr = (wmsTiledLayerPtr) handle;
    if (ptr == NULL)
	return DBL_MAX;

    pattern = ptr->firstPattern;
    while (pattern != NULL)
      {
	  if (count == index)
	      return pattern->TileExtentY;
	  count++;
	  pattern = pattern->next;
      }
    return DBL_MAX;
}

RL2_DECLARE char *
get_wms_tile_pattern_sample_url (rl2WmsTilePatternPtr handle)
{
/* return the sample URL for some TilePattern */
    char *url = NULL;
    wmsUrlArgumentPtr arg;
    wmsTilePatternPtr ptr = (wmsTilePatternPtr) handle;
    if (ptr == NULL)
	return NULL;

    arg = ptr->first;
    while (arg != NULL)
      {
	  if (url != NULL)
	    {
		char *str;
		if (arg->argValue == NULL)
		    str = sqlite3_mprintf ("%s&%s=", url, arg->argName);
		else
		    str =
			sqlite3_mprintf ("%s&%s=%s", url,
					 arg->argName, arg->argValue);
		sqlite3_free (url);
		url = str;
	    }
	  else
	    {
		if (arg->argValue == NULL)
		    url = sqlite3_mprintf ("%s=", arg->argName);
		else
		    url =
			sqlite3_mprintf ("%s=%s", arg->argName, arg->argValue);
	    }
	  arg = arg->next;
      }
    return url;
}

RL2_DECLARE char *
get_wms_tile_pattern_request_url (rl2WmsTilePatternPtr
				  handle,
				  const char *base_url,
				  double min_x, double min_y)
{
/* return a full GetMap request URL for some TilePattern */
    char *url = NULL;
    wmsUrlArgumentPtr arg;
    wmsTilePatternPtr ptr = (wmsTilePatternPtr) handle;
    if (ptr == NULL)
	return NULL;

    url = sqlite3_mprintf ("%s", base_url);
    arg = ptr->first;
    while (arg != NULL)
      {
	  char *str;
	  if (strcasecmp (arg->argName, "bbox") == 0)
	    {
		char *bbox =
		    sqlite3_mprintf ("%1.6f,%1.6f,%1.6f,%1.6f", min_x, min_y,
				     min_x + ptr->TileExtentX,
				     min_y + ptr->TileExtentY);
		if (arg != ptr->first)
		  {
		      str = sqlite3_mprintf ("%s&%s=%s", url,
					     arg->argName, bbox);
		  }
		else
		  {
		      str =
			  sqlite3_mprintf ("%s%s=%s", url, arg->argName, bbox);
		  }
		sqlite3_free (bbox);
	    }
	  else
	    {
		if (arg != ptr->first)
		  {
		      if (arg->argValue == NULL)
			  str = sqlite3_mprintf ("%s&%s=", url, arg->argName);
		      else
			  str =
			      sqlite3_mprintf ("%s&%s=%s", url,
					       arg->argName, arg->argValue);
		  }
		else
		  {
		      if (arg->argValue == NULL)
			  str = sqlite3_mprintf ("%s%s=", url, arg->argName);
		      else
			  str =
			      sqlite3_mprintf ("%s%s=%s", url, arg->argName,
					       arg->argValue);
		  }
	    }
	  sqlite3_free (url);
	  url = str;
	  arg = arg->next;
      }
    return url;
}

RL2_DECLARE int
get_wms_catalog_count (rl2WmsCatalogPtr handle)
{
/* counting how many first-level layers are defined within a WMS-Catalog */
    int count = 0;
    wmsLayerPtr lyr;
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return -1;

    lyr = ptr->firstLayer;
    while (lyr != NULL)
      {
	  count++;
	  lyr = lyr->next;
      }
    return count;
}

RL2_DECLARE rl2WmsLayerPtr
get_wms_catalog_layer (rl2WmsCatalogPtr handle, int index)
{
/* attempting to get a reference to some WMS-Layer object */
    int count = 0;
    wmsLayerPtr lyr;
    wmsCapabilitiesPtr ptr = (wmsCapabilitiesPtr) handle;
    if (ptr == NULL)
	return NULL;

    lyr = ptr->firstLayer;
    while (lyr != NULL)
      {
	  if (count == index)
	      return (rl2WmsLayerPtr) lyr;
	  count++;
	  lyr = lyr->next;
      }
    return NULL;
}

RL2_DECLARE int
wms_layer_has_children (rl2WmsLayerPtr handle)
{
/* testing if a WMS-Layer object has Layer children */
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return 0;
    if (ptr->firstLayer == NULL)
	return 0;
    return 1;
}

RL2_DECLARE int
get_wms_layer_children_count (rl2WmsLayerPtr handle)
{
/* counting how many children layers are defined within a WMS-Layer */
    int count = 0;
    wmsLayerPtr lyr;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    lyr = ptr->firstLayer;
    while (lyr != NULL)
      {
	  count++;
	  lyr = lyr->next;
      }
    return count;
}

RL2_DECLARE rl2WmsLayerPtr
get_wms_child_layer (rl2WmsLayerPtr handle, int index)
{
/* attempting to get a reference to some child WMS-Layer object */
    int count = 0;
    wmsLayerPtr lyr;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return NULL;

    lyr = ptr->firstLayer;
    while (lyr != NULL)
      {
	  if (count == index)
	      return (rl2WmsLayerPtr) lyr;
	  count++;
	  lyr = lyr->next;
      }
    return NULL;
}

RL2_DECLARE const char *
get_wms_layer_name (rl2WmsLayerPtr handle)
{
/* return the name corresponding to a WMS-Layer object */
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->Name;
}

RL2_DECLARE const char *
get_wms_layer_title (rl2WmsLayerPtr handle)
{
/* return the title corresponding to a WMS-Layer object */
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->Title;
}

RL2_DECLARE const char *
get_wms_layer_abstract (rl2WmsLayerPtr handle)
{
/* return the abstract corresponding to a WMS-Layer object */
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return NULL;
    return ptr->Abstract;
}

static int
wms_parent_crs_count (wmsLayerPtr lyr)
{
/* recursively counting how many CRSs are supported by a parent Layer (inheritance) */
    int count = 0;
    wmsCrsPtr crs;
    if (lyr == NULL)
	return 0;

    crs = lyr->firstCrs;
    while (crs != NULL)
      {
	  count++;
	  crs = crs->next;
      }
    count += wms_parent_crs_count (lyr->Parent);
    return count;
}

RL2_DECLARE int
get_wms_layer_crs_count (rl2WmsLayerPtr handle)
{
/* counting how many CRSs are supported by a WMS-Layer */
    int count = 0;
    wmsCrsPtr crs;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    crs = ptr->firstCrs;
    while (crs != NULL)
      {
	  count++;
	  crs = crs->next;
      }
    count += wms_parent_crs_count (ptr->Parent);
    return count;
}

static wmsCrsPtr
wms_parent_crs (wmsLayerPtr lyr, int *cnt, int index)
{
/* recursively finding a CRS by index (inheritance) */
    int count = *cnt;
    wmsCrsPtr crs;
    if (lyr == NULL)
	return 0;

    crs = lyr->firstCrs;
    while (crs != NULL)
      {
	  if (count == index)
	      return crs;
	  count++;
	  crs = crs->next;
      }
    *cnt = count;
    crs = wms_parent_crs (lyr->Parent, cnt, index);
    return crs;
}

RL2_DECLARE const char *
get_wms_layer_crs (rl2WmsLayerPtr handle, int index)
{
/* attempting to get the Nth CRS supported by some WMS-Layer object */
    int count = 0;
    wmsCrsPtr crs;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return NULL;

    crs = ptr->firstCrs;
    while (crs != NULL)
      {
	  if (count == index)
	      return crs->Crs;
	  count++;
	  crs = crs->next;
      }
    crs = wms_parent_crs (ptr->Parent, &count, index);
    if (crs != NULL)
	return crs->Crs;
    return NULL;
}

RL2_DECLARE int
get_wms_layer_style_count (rl2WmsLayerPtr handle)
{
/* counting how many Styles are supported by a WMS-Layer */
    int count = 0;
    wmsStylePtr stl;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    stl = ptr->firstStyle;
    while (stl != NULL)
      {
	  count++;
	  stl = stl->next;
      }
    return count;
}

RL2_DECLARE const char *
get_wms_layer_style_name (rl2WmsLayerPtr handle, int index)
{
/* attempting to get the Name of the Nth Style supported by some WMS-Layer object */
    int count = 0;
    wmsStylePtr stl;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return NULL;

    stl = ptr->firstStyle;
    while (stl != NULL)
      {
	  if (count == index)
	      return stl->Name;
	  count++;
	  stl = stl->next;
      }
    return NULL;
}

RL2_DECLARE const char *
get_wms_layer_style_title (rl2WmsLayerPtr handle, int index)
{
/* attempting to get the Title of the Nth Style supported by some WMS-Layer object */
    int count = 0;
    wmsStylePtr stl;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return NULL;

    stl = ptr->firstStyle;
    while (stl != NULL)
      {
	  if (count == index)
	      return stl->Title;
	  count++;
	  stl = stl->next;
      }
    return NULL;
}

RL2_DECLARE const char *
get_wms_layer_style_abstract (rl2WmsLayerPtr handle, int index)
{
/* attempting to get the Abstract of the Nth Style supported by some WMS-Layer object */
    int count = 0;
    wmsStylePtr stl;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return NULL;

    stl = ptr->firstStyle;
    while (stl != NULL)
      {
	  if (count == index)
	      return stl->Abstract;
	  count++;
	  stl = stl->next;
      }
    return NULL;
}

static void
wms_parent_opaque (wmsLayerPtr lyr, int *opaque)
{
/* recursively testing the Opaque property from a parent Layer (inheritance) */
    if (lyr == NULL)
	return;

    if (lyr->Opaque >= 0)
      {
	  *opaque = lyr->Opaque;
	  return;
      }
    wms_parent_opaque (lyr->Parent, opaque);
}

RL2_DECLARE int
is_wms_layer_opaque (rl2WmsLayerPtr handle)
{
/* Tests if some WMS-Layer object declares the Opaque property */
    int opaque = -1;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    if (ptr->Opaque >= 0)
	return ptr->Opaque;
    wms_parent_opaque (ptr->Parent, &opaque);
    return opaque;
}

static void
wms_parent_queryable (wmsLayerPtr lyr, int *queryable)
{
/* recursively testing the Queryable property from a parent Layer (inheritance) */
    if (lyr == NULL)
	return;

    if (lyr->Queryable >= 0)
      {
	  *queryable = lyr->Queryable;
	  return;
      }
    wms_parent_opaque (lyr->Parent, queryable);
}

RL2_DECLARE int
is_wms_layer_queryable (rl2WmsLayerPtr handle)
{
/* Tests if some WMS-Layer object declares the Queryable property */
    int queryable = -1;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return -1;

    if (ptr->Queryable >= 0)
	return ptr->Queryable;
    wms_parent_queryable (ptr->Parent, &queryable);
    return queryable;
}

RL2_DECLARE double
get_wms_layer_min_scale_denominator (rl2WmsLayerPtr handle)
{
/* Return the MinScaleDenominator from some WMS-Layer object */
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return DBL_MAX;

    return ptr->MinScaleDenominator;
}

RL2_DECLARE double
get_wms_layer_max_scale_denominator (rl2WmsLayerPtr handle)
{
/* Return the MaxScaleDenominator from some WMS-Layer object */
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    if (ptr == NULL)
	return DBL_MAX;

    return ptr->MaxScaleDenominator;
}

RL2_DECLARE int
get_wms_layer_geo_bbox (rl2WmsLayerPtr handle, double *minx, double *maxx,
			double *miny, double *maxy)
{
/* Return the Geographic Bounding Box from some WMS-Layer object */
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    *minx = DBL_MAX;
    *maxx = DBL_MAX;
    *miny = DBL_MAX;
    *maxx = DBL_MAX;
    if (ptr == NULL)
	return 0;

    if (ptr->MinLat == DBL_MAX && ptr->MaxLat == DBL_MAX
	&& ptr->MinLong == DBL_MAX && ptr->MaxLong == DBL_MAX)
      {
	  /* undefined: iteratively searching its Parents */
	  wmsLayerPtr parent = ptr->Parent;
	  while (parent != NULL)
	    {
		if (parent->MinLat == DBL_MAX && parent->MaxLat == DBL_MAX
		    && parent->MinLong == DBL_MAX && parent->MaxLong == DBL_MAX)
		  {
		      parent = parent->Parent;
		      continue;
		  }
		*miny = parent->MinLat;
		*maxy = parent->MaxLat;
		*minx = parent->MinLong;
		*maxx = parent->MaxLong;
		return 1;
	    }
      }
    *miny = ptr->MinLat;
    *maxy = ptr->MaxLat;
    *minx = ptr->MinLong;
    *maxx = ptr->MaxLong;
    return 1;
}

RL2_DECLARE int
get_wms_layer_bbox (rl2WmsLayerPtr handle, const char *crs, double *minx,
		    double *maxx, double *miny, double *maxy)
{
/* Return the Bounding Box corresponding to given CRS from some WMS-Layer object */
    wmsBBoxPtr bbox;
    wmsLayerPtr ptr = (wmsLayerPtr) handle;
    wmsLayerPtr parent;
    *minx = DBL_MAX;
    *maxx = DBL_MAX;
    *miny = DBL_MAX;
    *maxx = DBL_MAX;
    if (ptr == NULL)
	return 0;

    bbox = ptr->firstBBox;
    while (bbox != NULL)
      {
	  if (strcmp (bbox->Crs, crs) == 0)
	    {
		*miny = bbox->MinY;
		*maxy = bbox->MaxY;
		*minx = bbox->MinX;
		*maxx = bbox->MaxX;
		return 1;
	    }
	  bbox = bbox->next;
      }

/* not found: iteratively searching its Parents */
    parent = ptr->Parent;
    while (parent != NULL)
      {
	  bbox = parent->firstBBox;
	  while (bbox != NULL)
	    {
		if (strcmp (bbox->Crs, crs) == 0)
		  {
		      *miny = bbox->MinY;
		      *maxy = bbox->MaxY;
		      *minx = bbox->MinX;
		      *maxx = bbox->MaxX;
		      return 1;
		  }
		bbox = bbox->next;
	    }
	  parent = parent->Parent;
      }
    return 0;
}

RL2_DECLARE void
destroy_wms_feature_collection (rl2WmsFeatureCollectionPtr handle)
{
/* memory cleanup: freeing a GML Feature Collection object */
    wmsFeatureCollectionPtr ptr = (wmsFeatureCollectionPtr) handle;
    if (ptr == NULL)
	return;
    wmsFreeFeatureCollection (ptr);
}

static int
check_swap (gaiaGeomCollPtr geom, double point_x, double point_y)
{
/* checking if X and Y axes should be flipped */
    double x;
    double y;
    double z;
    double m;
    double dist;
    double dist_flip;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    pt = geom->FirstPoint;
    if (pt != NULL)
      {
	  x = pt->X;
	  y = pt->Y;
	  goto eval;
      }
    ln = geom->FirstLinestring;
    if (ln != NULL)
      {
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, 0, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, 0, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, 0, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, 0, &x, &y);
	    }
	  goto eval;
      }
    pg = geom->FirstPolygon;
    if (pg != NULL)
      {
	  gaiaRingPtr ring = pg->Exterior;
	  if (ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ring->Coords, 0, &x, &y, &z);
	    }
	  else if (ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ring->Coords, 0, &x, &y, &m);
	    }
	  else if (ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ring->Coords, 0, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ring->Coords, 0, &x, &y);
	    }
	  goto eval;
      }
    return 0;
  eval:
    dist =
	sqrt (((x - point_x) * (x - point_x)) +
	      ((y - point_y) * (y - point_y)));
    dist_flip =
	sqrt (((x - point_y) * (x - point_y)) +
	      ((y - point_x) * (y - point_x)));
    if (dist_flip < dist)
	return 1;
    return 0;
}

static void
getProjParams (void *p_sqlite, int srid, char **proj_params)
{
/* retrieves the PROJ params from SPATIAL_SYS_REF table, if possible */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    char *sql;
    char **results;
    int rows;
    int columns;
    int i;
    int ret;
    int len;
    const char *proj4text;
    char *errMsg = NULL;
    *proj_params = NULL;
    sql = sqlite3_mprintf
	("SELECT proj4text FROM spatial_ref_sys WHERE srid = %d", srid);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "unknown SRID: %d\t<%s>\n", srid, errMsg);
	  sqlite3_free (errMsg);
	  return;
      }
    for (i = 1; i <= rows; i++)
      {
	  proj4text = results[(i * columns)];
	  if (proj4text != NULL)
	    {
		len = strlen (proj4text);
		*proj_params = malloc (len + 1);
		strcpy (*proj_params, proj4text);
	    }
      }
    if (*proj_params == NULL)
	fprintf (stderr, "unknown SRID: %d\n", srid);
    sqlite3_free_table (results);
}

static gaiaGeomCollPtr
reproject (gaiaGeomCollPtr geom, int srid, sqlite3 * sqlite)
{
/* attempting to reproject into a different CRS */
    char *proj_from;
    char *proj_to;
    gaiaGeomCollPtr g2 = NULL;
    getProjParams (sqlite, geom->Srid, &proj_from);
    getProjParams (sqlite, srid, &proj_to);
    if (proj_to == NULL || proj_from == NULL)
	;
    else
	g2 = gaiaTransform (geom, proj_from, proj_to);
    if (proj_from)
	free (proj_from);
    if (proj_to)
	free (proj_to);
    return g2;
}

RL2_DECLARE void
wms_feature_collection_parse_geometries (rl2WmsFeatureCollectionPtr
					 handle, int srid, double point_x,
					 double point_y, sqlite3 * sqlite)
{
/* attempting to parse any GML Geometry contained within a Feature Collection object */
    wmsFeatureMemberPtr member;
    wmsFeatureCollectionPtr ptr = (wmsFeatureCollectionPtr) handle;
    if (ptr == NULL)
	return;

    member = ptr->first;
    while (member != NULL)
      {
	  /* looping on FeatureMembers */
	  wmsFeatureAttributePtr attr;
	  attr = member->first;
	  while (attr != NULL)
	    {
		/* looping on FeatureAttributes */
		if (attr->value != NULL)
		  {
		      /* attempting to parse a possible GML Geometry */
		      gaiaGeomCollPtr geom =
			  gaiaParseGml ((const unsigned char *) (attr->value),
					sqlite);
		      if (geom != NULL)
			{
			    if (geom->Srid > 0 && srid > 0
				&& geom->Srid != srid)
			      {
				  /* attempting to reproject into the Map CRS */
				  gaiaGeomCollPtr g2 =
				      reproject (geom, srid, sqlite);
				  if (g2 != NULL)
				    {
					if (check_swap (g2, point_x, point_y))
					  {
					      gaiaFreeGeomColl (g2);
					      gaiaSwapCoords (geom);
					      g2 = reproject (geom, srid,
							      sqlite);
					      attr->geometry = g2;
					      gaiaFreeGeomColl (geom);
					  }
					else
					  {
					      attr->geometry = g2;
					      gaiaFreeGeomColl (geom);
					  }
				    }
			      }
			    else
			      {
				  if (check_swap (geom, point_x, point_y))
				      gaiaSwapCoords (geom);
				  attr->geometry = geom;
			      }
			}
		  }
		attr = attr->next;
	    }
	  member = member->next;
      }
}

RL2_DECLARE int
get_wms_feature_members_count (rl2WmsFeatureCollectionPtr handle)
{
/* counting how many Feature Members are contained within a WMS-FeatureCollection */
    int count = 0;
    wmsFeatureMemberPtr member;
    wmsFeatureCollectionPtr ptr = (wmsFeatureCollectionPtr) handle;
    if (ptr == NULL)
	return -1;

    member = ptr->first;
    while (member != NULL)
      {
	  count++;
	  member = member->next;
      }
    return count;
}

RL2_DECLARE rl2WmsFeatureMemberPtr
get_wms_feature_member (rl2WmsFeatureCollectionPtr handle, int index)
{
/* attempting to get the Nth FeatureMember from some WMS-FeatureCollection object */
    int count = 0;
    wmsFeatureMemberPtr member;
    wmsFeatureCollectionPtr ptr = (wmsFeatureCollectionPtr) handle;
    if (ptr == NULL)
	return NULL;

    member = ptr->first;
    while (member != NULL)
      {
	  if (count == index)
	      return (rl2WmsFeatureMemberPtr) member;
	  count++;
	  member = member->next;
      }
    return NULL;
}

RL2_DECLARE int
get_wms_feature_attributes_count (rl2WmsFeatureMemberPtr handle)
{
/* counting how many Feature Attributes are contained within a WMS-FeatureMember */
    int count = 0;
    wmsFeatureAttributePtr attr;
    wmsFeatureMemberPtr ptr = (wmsFeatureMemberPtr) handle;
    if (ptr == NULL)
	return -1;

    attr = ptr->first;
    while (attr != NULL)
      {
	  count++;
	  attr = attr->next;
      }
    return count;
}

RL2_DECLARE const char *
get_wms_feature_attribute_name (rl2WmsFeatureMemberPtr handle, int index)
{
/* attempting to get the Nth FeatureAttribute (Name) from some WMS-FeatureMember object */
    int count = 0;
    wmsFeatureAttributePtr attr;
    wmsFeatureMemberPtr ptr = (wmsFeatureMemberPtr) handle;
    if (ptr == NULL)
	return NULL;

    attr = ptr->first;
    while (attr != NULL)
      {
	  if (count == index)
	      return attr->name;
	  count++;
	  attr = attr->next;
      }
    return NULL;
}

RL2_DECLARE gaiaGeomCollPtr
get_wms_feature_attribute_geometry (rl2WmsFeatureMemberPtr handle, int index)
{
/* attempting to get the Nth FeatureAttribute (Geometry) from some WMS-FeatureMember object */
    int count = 0;
    wmsFeatureAttributePtr attr;
    wmsFeatureMemberPtr ptr = (wmsFeatureMemberPtr) handle;
    if (ptr == NULL)
	return NULL;

    attr = ptr->first;
    while (attr != NULL)
      {
	  if (count == index)
	      return attr->geometry;
	  count++;
	  attr = attr->next;
      }
    return NULL;
}

RL2_DECLARE const char *
get_wms_feature_attribute_value (rl2WmsFeatureMemberPtr handle, int index)
{
/* attempting to get the Nth FeatureAttribute (Value) from some WMS-FeatureMember object */
    int count = 0;
    wmsFeatureAttributePtr attr;
    wmsFeatureMemberPtr ptr = (wmsFeatureMemberPtr) handle;
    if (ptr == NULL)
	return NULL;

    attr = ptr->first;
    while (attr != NULL)
      {
	  if (count == index)
	      return attr->value;
	  count++;
	  attr = attr->next;
      }
    return NULL;
}

static int
check_marker (const char *url)
{
/* testing if some "?" marker is already defined */
    int i;
    int force_marker = 1;
    for (i = 0; i < (int) strlen (url); i++)
      {
	  if (*(url + i) == '?')
	      force_marker = 0;
      }
    return force_marker;
}

RL2_DECLARE unsigned char *
do_wms_GetMap_get (rl2WmsCachePtr cache_handle, const char *url,
		   const char *proxy, const char *version, const char *layer,
		   const char *crs, int swap_xy, double minx, double miny,
		   double maxx, double maxy, int width, int height,
		   const char *style, const char *format, int opaque,
		   int from_cache, char **err_msg)
{
/* attempting to execute a WMS GepMap request [method GET] */
    CURL *curl = NULL;
    CURLcode res;
    wmsMemBuffer headerBuf;
    wmsMemBuffer bodyBuf;
    int http_status;
    char *http_code;
    char *request;
    char *image_format;
    unsigned char *rgba = NULL;
    int force_marker = check_marker (url);
    const char *crs_prefix = "CRS";
    rl2RasterPtr raster = NULL;
    wmsCachePtr cache = (wmsCachePtr) cache_handle;

    *err_msg = NULL;
    if (from_cache && cache == NULL)
	return NULL;

/* masking NULL arguments */
    if (url == NULL)
	url = "";
    if (version == NULL)
	version = "";
    if (layer == NULL)
	layer = "";
    if (crs == NULL)
	crs = "";
    if (style == NULL)
	style = "";
    if (format == NULL)
	format = "";

/* preparing the request URL */
    if (strcmp (version, "1.3.0") < 0)
      {
	  /* earlier versions of the protocol require SRS instead of CRS */
	  crs_prefix = "SRS";
      }
    if (force_marker)
      {
	  /* "?" marker not declared */
	  if (swap_xy)
	      request =
		  sqlite3_mprintf ("%s?SERVICE=WMS&REQUEST=GetMap&VERSION=%s"
				   "&LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
				   "&WIDTH=%d&HEIGHT=%d&STYLES=%s&FORMAT=%s"
				   "&TRANSPARENT=%s&BGCOLOR=0xFFFFFF", url,
				   version, layer, crs_prefix, crs, miny, minx,
				   maxy, maxx, width, height, style, format,
				   (opaque == 0) ? "TRUE" : "FALSE");
	  else
	      request =
		  sqlite3_mprintf ("%s?SERVICE=WMS&REQUEST=GetMap&VERSION=%s"
				   "&LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
				   "&WIDTH=%d&HEIGHT=%d&STYLES=%s&FORMAT=%s"
				   "&TRANSPARENT=%s&BGCOLOR=0xFFFFFF", url,
				   version, layer, crs_prefix, crs, minx, miny,
				   maxx, maxy, width, height, style, format,
				   (opaque == 0) ? "TRUE" : "FALSE");
      }
    else
      {
	  /* "?" marker already defined */
	  if (swap_xy)
	      request =
		  sqlite3_mprintf ("%sSERVICE=WMS&REQUEST=GetMap&VERSION=%s"
				   "&LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
				   "&WIDTH=%d&HEIGHT=%d&STYLES=%s&FORMAT=%s"
				   "&TRANSPARENT=%s&BGCOLOR=0xFFFFFF", url,
				   version, layer, crs_prefix, crs, miny, minx,
				   maxy, maxx, width, height, style, format,
				   (opaque == 0) ? "TRUE" : "FALSE");
	  else
	      request =
		  sqlite3_mprintf ("%sSERVICE=WMS&REQUEST=GetMap&VERSION=%s"
				   "&LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
				   "&WIDTH=%d&HEIGHT=%d&STYLES=%s&FORMAT=%s"
				   "&TRANSPARENT=%s&BGCOLOR=0xFFFFFF", url,
				   version, layer, crs_prefix, crs, minx, miny,
				   maxx, maxy, width, height, style, format,
				   (opaque == 0) ? "TRUE" : "FALSE");
      }

    if (cache != NULL)
      {
	  /* checks if it's already stored into the WMS Cache */
	  wmsCachedItemPtr cachedItem = getWmsCachedItem (cache, request);
	  if (cachedItem != NULL)
	    {
		/* ok, found from WMS Cache */
		time_t xtime;
		time (&xtime);
		cachedItem->Time = xtime;
		/* attempting to decode an RGBA image */
		if (cachedItem->ImageFormat == WMS_FORMAT_GIF)
		    raster =
			rl2_raster_from_gif (cachedItem->Item,
					     cachedItem->Size);
		if (cachedItem->ImageFormat == WMS_FORMAT_PNG)
		    raster =
			rl2_raster_from_png (cachedItem->Item,
					     cachedItem->Size);
		if (cachedItem->ImageFormat == WMS_FORMAT_JPEG)
		    raster =
			rl2_raster_from_jpeg (cachedItem->Item,
					      cachedItem->Size);
		goto image_ready;
	    }
      }
    if (from_cache)
      {
	  sqlite3_free (request);
	  return NULL;
      }

    curl = curl_easy_init ();
    if (curl)
      {
	  /* setting the URL */
	  curl_easy_setopt (curl, CURLOPT_URL, request);

	  if (proxy != NULL)
	    {
		/* setting up the required proxy */
		curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
	    }

	  /* no progress meter please */
	  curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1L);
	  /* setting the output callback function */
	  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, store_data);

	  /* initializes the dynamically growing buffers */
	  wmsMemBufferInitialize (&headerBuf);
	  wmsMemBufferInitialize (&bodyBuf);
	  curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &headerBuf);
	  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &bodyBuf);

	  /* Perform the request, res will get the return code */
	  res = curl_easy_perform (curl);
	  /* Check for errors */
	  if (res != CURLE_OK)
	    {
		fprintf (stderr, "CURL error: %s\n", curl_easy_strerror (res));
		goto stop;
	    }

	  /* verifying the HTTP status code */
	  check_http_header (&headerBuf, &http_status, &http_code);
	  if (http_status == 302)
	    {
		while (1)
		  {
		      /* following a redirect */
		      char *redir = parse_http_redirect (&headerBuf);
		      if (redir == NULL)
			  break;
		      /* resetting all buffers */
		      if (http_code != NULL)
			  free (http_code);
		      wmsMemBufferReset (&headerBuf);
		      wmsMemBufferReset (&bodyBuf);
		      curl_easy_setopt (curl, CURLOPT_URL, redir);
		      if (proxy != NULL)
			  curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
		      res = curl_easy_perform (curl);
		      if (res != CURLE_OK)
			{
			    fprintf (stderr, "CURL error: %s\n",
				     curl_easy_strerror (res));
			    goto stop;
			}
		      free (redir);
		      check_http_header (&headerBuf, &http_status, &http_code);
		      if (http_status == 302)
			  continue;
		      break;
		  }

	    }
	  if (http_status != 200)
	    {
		fprintf (stderr, "Invalid HTTP status code: %d %s\n",
			 http_status, http_code);
		if (http_code != NULL)
		    free (http_code);
		goto stop;
	    }
	  if (http_code != NULL)
	      free (http_code);

	  /* attempting to decode an RGBA image */
	  image_format = parse_http_format (&headerBuf);
	  if (strcmp (image_format, "image/gif") == 0)
	      raster =
		  rl2_raster_from_gif (bodyBuf.Buffer, bodyBuf.WriteOffset);
	  if (strcmp (image_format, "image/png") == 0)
	      raster =
		  rl2_raster_from_png (bodyBuf.Buffer, bodyBuf.WriteOffset);
	  if (strcmp (image_format, "image/jpeg") == 0)
	      raster =
		  rl2_raster_from_jpeg (bodyBuf.Buffer, bodyBuf.WriteOffset);

	  if (raster != NULL)
	    {
		/* saving into the WMS Cache */
		wmsAddCachedItem (cache, request, bodyBuf.Buffer,
				  bodyBuf.WriteOffset, image_format);
	    }

	  if (image_format != NULL)
	      free (image_format);

	stop:
	  wmsMemBufferReset (&headerBuf);
	  wmsMemBufferReset (&bodyBuf);
	  curl_easy_cleanup (curl);
      }

  image_ready:
    sqlite3_free (request);

    if (raster != NULL)
      {
	  /* exporting the image into an RGBA pix-buffer */
	  int size;
	  int ret = rl2_raster_data_to_RGBA (raster, &rgba, &size);
	  rl2_destroy_raster (raster);
	  if (ret == RL2_OK && rgba != NULL && size == (width * height * 4))
	      return rgba;
	  if (rgba != NULL)
	      free (rgba);
	  rgba = NULL;
      }
    return rgba;
}

RL2_DECLARE unsigned char *
do_wms_GetMap_post (rl2WmsCachePtr cache_handle, const char *url,
		    const char *proxy, const char *version, const char *layer,
		    const char *crs, int swap_xy, double minx, double miny,
		    double maxx, double maxy, int width, int height,
		    const char *style, const char *format, int opaque,
		    int from_cache, char **err_msg)
{
/* attempting to execute a WMS GepMap request [method POST] */

/* not yet implemented: just a stupid placeholder always returning NULL */
    if (cache_handle == NULL || url == NULL || proxy == NULL || version == NULL
	|| layer == NULL || crs == NULL)
	return NULL;
    if (minx == miny || maxx == maxy || width == height || opaque == from_cache
	|| width == swap_xy)
	return NULL;
    if (style == NULL || format == NULL || err_msg == NULL)
	return NULL;
    return NULL;
}

RL2_DECLARE unsigned char *
do_wms_GetMap_TileService_get (rl2WmsCachePtr cache_handle, const char *url,
			       const char *proxy, int width, int height,
			       int from_cache, char **err_msg)
{
/* attempting to execute a WMS GepMap TileService request [method GET] */
    CURL *curl = NULL;
    CURLcode res;
    wmsMemBuffer headerBuf;
    wmsMemBuffer bodyBuf;
    int http_status;
    char *http_code;
    char *image_format;
    unsigned char *rgba = NULL;
    rl2RasterPtr raster = NULL;
    wmsCachePtr cache = (wmsCachePtr) cache_handle;

    *err_msg = NULL;
    if (from_cache && cache == NULL)
	return NULL;

/* masking NULL arguments */
    if (url == NULL)
	url = "";

    if (cache != NULL)
      {
	  /* checks if it's already stored into the WMS Cache */
	  wmsCachedItemPtr cachedItem = getWmsCachedItem (cache, url);
	  if (cachedItem != NULL)
	    {
		/* ok, found from WMS Cache */
		time_t xtime;
		time (&xtime);
		cachedItem->Time = xtime;
		/* attempting to decode an RGBA image */
		if (cachedItem->ImageFormat == WMS_FORMAT_GIF)
		    raster =
			rl2_raster_from_gif (cachedItem->Item,
					     cachedItem->Size);
		if (cachedItem->ImageFormat == WMS_FORMAT_PNG)
		    raster =
			rl2_raster_from_png (cachedItem->Item,
					     cachedItem->Size);
		if (cachedItem->ImageFormat == WMS_FORMAT_JPEG)
		    raster =
			rl2_raster_from_jpeg (cachedItem->Item,
					      cachedItem->Size);
		goto image_ready;
	    }
      }
    if (from_cache)
	return NULL;

    curl = curl_easy_init ();
    if (curl)
      {
	  /* setting the URL */
	  curl_easy_setopt (curl, CURLOPT_URL, url);

	  if (proxy != NULL)
	    {
		/* setting up the required proxy */
		curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
	    }

	  /* no progress meter please */
	  curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1L);
	  /* setting the output callback function */
	  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, store_data);

	  /* initializes the dynamically growing buffers */
	  wmsMemBufferInitialize (&headerBuf);
	  wmsMemBufferInitialize (&bodyBuf);
	  curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &headerBuf);
	  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &bodyBuf);

	  /* Perform the request, res will get the return code */
	  res = curl_easy_perform (curl);
	  /* Check for errors */
	  if (res != CURLE_OK)
	    {
		fprintf (stderr, "CURL error: %s\n", curl_easy_strerror (res));
		goto stop;
	    }

	  /* verifying the HTTP status code */
	  check_http_header (&headerBuf, &http_status, &http_code);
	  if (http_status == 302)
	    {
		while (1)
		  {
		      /* following a redirect */
		      char *redir = parse_http_redirect (&headerBuf);
		      if (redir == NULL)
			  break;
		      /* resetting all buffers */
		      if (http_code != NULL)
			  free (http_code);
		      wmsMemBufferReset (&headerBuf);
		      wmsMemBufferReset (&bodyBuf);
		      curl_easy_setopt (curl, CURLOPT_URL, redir);
		      if (proxy != NULL)
			  curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
		      res = curl_easy_perform (curl);
		      if (res != CURLE_OK)
			{
			    fprintf (stderr, "CURL error: %s\n",
				     curl_easy_strerror (res));
			    goto stop;
			}
		      free (redir);
		      check_http_header (&headerBuf, &http_status, &http_code);
		      if (http_status == 302)
			  continue;
		      break;
		  }

	    }
	  if (http_status != 200)
	    {
		fprintf (stderr, "Invalid HTTP status code: %d %s\n",
			 http_status, http_code);
		if (http_code != NULL)
		    free (http_code);
		goto stop;
	    }
	  if (http_code != NULL)
	      free (http_code);

	  /* attempting to decode an RGBA image */
	  image_format = parse_http_format (&headerBuf);
	  if (strcmp (image_format, "image/gif") == 0)
	      raster =
		  rl2_raster_from_gif (bodyBuf.Buffer, bodyBuf.WriteOffset);
	  if (strcmp (image_format, "image/png") == 0)
	      raster =
		  rl2_raster_from_png (bodyBuf.Buffer, bodyBuf.WriteOffset);
	  if (strcmp (image_format, "image/jpeg") == 0)
	      raster =
		  rl2_raster_from_jpeg (bodyBuf.Buffer, bodyBuf.WriteOffset);

	  if (raster != NULL)
	    {
		/* saving into the WMS Cache */
		wmsAddCachedItem (cache, url, bodyBuf.Buffer,
				  bodyBuf.WriteOffset, image_format);
	    }

	  if (image_format != NULL)
	      free (image_format);

	stop:
	  wmsMemBufferReset (&headerBuf);
	  wmsMemBufferReset (&bodyBuf);
	  curl_easy_cleanup (curl);
      }

  image_ready:

    if (raster != NULL)
      {
	  /* exporting the image into an RGBA pix-buffer */
	  int size;
	  int ret = rl2_raster_data_to_RGBA (raster, &rgba, &size);
	  rl2_destroy_raster (raster);
	  if (ret == RL2_OK && rgba != NULL && size == (width * height * 4))
	      return rgba;
	  if (rgba != NULL)
	      free (rgba);
	  rgba = NULL;
      }
    return rgba;
}

RL2_DECLARE unsigned char *
do_wms_GetMap_TileService_post (rl2WmsCachePtr cache_handle, const char *url,
				const char *proxy, int width, int height,
				int from_cache, char **err_msg)
{
/* attempting to execute a WMS GepMap TileService request [method POST] */

/* not yet implemented: just a stupid placeholder always returning NULL */
    if (cache_handle == NULL || url == NULL || proxy == NULL || err_msg == NULL)
	return NULL;
    if (width == height || width == from_cache)
	return NULL;
    return NULL;
}

RL2_DECLARE rl2WmsFeatureCollectionPtr
do_wms_GetFeatureInfo_get (const char *url, const char *proxy,
			   const char *version, const char *format,
			   const char *layer, const char *crs, int swap_xy,
			   double minx, double miny, double maxx, double maxy,
			   int width, int height, int mouse_x, int mouse_y,
			   char **err_msg)
{
/* attempting to execute a WMS GepFeatureInfo request [method GET] */
    CURL *curl = NULL;
    CURLcode res;
    wmsMemBuffer headerBuf;
    wmsMemBuffer bodyBuf;
    wmsFeatureCollectionPtr coll = NULL;
    int http_status;
    char *http_code;
    char *request;
    char *multipart_boundary;
    const char *crs_prefix = "CRS";
    char *xml_buf;
    int force_marker = check_marker (url);

    *err_msg = NULL;

/* masking NULL arguments */
    if (url == NULL)
	url = "";
    if (version == NULL)
	version = "";
    if (format == NULL)
	format = "";
    if (layer == NULL)
	layer = "";
    if (crs == NULL)
	crs = "";

/* preparing the request URL */
    if (strcmp (version, "1.3.0") < 0)
      {
	  /* earlier versions of the protocol require SRS instead of CRS */
	  crs_prefix = "SRS";
      }
    if (force_marker)
      {
	  if (swap_xy)
	      request =
		  sqlite3_mprintf
		  ("%s?SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=%s&LAYERS=%s"
		   "&QUERY_LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
		   "&WIDTH=%d&HEIGHT=%d&X=%d&Y=%d&INFO_FORMAT=%s"
		   "&FEATURE_COUNT=50", url, version, layer, layer, crs_prefix,
		   crs, miny, minx, maxy, maxx, width, height, mouse_x,
		   mouse_y, format);
	  else
	      request =
		  sqlite3_mprintf
		  ("%s?SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=%s&LAYERS=%s"
		   "&QUERY_LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
		   "&WIDTH=%d&HEIGHT=%d&X=%d&Y=%d&INFO_FORMAT=%s"
		   "&FEATURE_COUNT=50", url, version, layer, layer, crs_prefix,
		   crs, minx, miny, maxx, maxy, width, height, mouse_x,
		   mouse_y, format);
      }
    else
      {
	  if (swap_xy)
	      request =
		  sqlite3_mprintf
		  ("%sSERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=%s&LAYERS=%s"
		   "&QUERY_LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
		   "&WIDTH=%d&HEIGHT=%d&X=%d&Y=%d&INFO_FORMAT=%s"
		   "&FEATURE_COUNT=50", url, version, layer, layer, crs_prefix,
		   crs, miny, minx, maxy, maxx, width, height, mouse_x,
		   mouse_y, format);
	  else
	      request =
		  sqlite3_mprintf
		  ("%sSERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=%s&LAYERS=%s"
		   "&QUERY_LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
		   "&WIDTH=%d&HEIGHT=%d&X=%d&Y=%d&INFO_FORMAT=%s"
		   "&FEATURE_COUNT=50", url, version, layer, layer, crs_prefix,
		   crs, minx, miny, maxx, maxy, width, height, mouse_x,
		   mouse_y, format);
      }

    curl = curl_easy_init ();
    if (curl)
      {
	  /* setting the URL */
	  curl_easy_setopt (curl, CURLOPT_URL, request);

	  if (proxy != NULL)
	    {
		/* setting up the required proxy */
		curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
	    }

	  /* no progress meter please */
	  curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1L);
	  /* setting the output callback function */
	  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, store_data);

	  /* initializes the dynamically growing buffers */
	  wmsMemBufferInitialize (&headerBuf);
	  wmsMemBufferInitialize (&bodyBuf);
	  curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &headerBuf);
	  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &bodyBuf);

	  /* Perform the request, res will get the return code */
	  res = curl_easy_perform (curl);
	  /* Check for errors */
	  if (res != CURLE_OK)
	    {
		fprintf (stderr, "CURL error: %s\n", curl_easy_strerror (res));
		goto stop;
	    }

	  /* verifying the HTTP status code */
	  check_http_header (&headerBuf, &http_status, &http_code);
	  if (http_status == 302)
	    {
		while (1)
		  {
		      /* following a redirect */
		      char *redir = parse_http_redirect (&headerBuf);
		      if (redir == NULL)
			  break;
		      /* resetting all buffers */
		      if (http_code != NULL)
			  free (http_code);
		      wmsMemBufferReset (&headerBuf);
		      wmsMemBufferReset (&bodyBuf);
		      curl_easy_setopt (curl, CURLOPT_URL, redir);
		      if (proxy != NULL)
			  curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
		      res = curl_easy_perform (curl);
		      if (res != CURLE_OK)
			{
			    fprintf (stderr, "CURL error: %s\n",
				     curl_easy_strerror (res));
			    goto stop;
			}
		      free (redir);
		      check_http_header (&headerBuf, &http_status, &http_code);
		      if (http_status == 302)
			  continue;
		      break;
		  }

	    }
	  if (http_status != 200)
	    {
		fprintf (stderr, "Invalid HTTP status code: %d %s\n",
			 http_status, http_code);
		if (http_code != NULL)
		    free (http_code);
		goto stop;
	    }
	  if (http_code != NULL)
	      free (http_code);

	  multipart_boundary = check_http_multipart_response (&headerBuf);
	  if (multipart_boundary != NULL)
	    {
		wmsMultipartCollectionPtr multi =
		    parse_multipart_body (&bodyBuf, multipart_boundary);
		free (multipart_boundary);
		if (multi != NULL)
		  {
		      wmsSinglePartResponsePtr single = multi->first;
		      while (single != NULL)
			{
			    xml_buf = clean_xml_str (single->body);
			    if (xml_buf != NULL)
			      {
				  coll = parse_wms_feature_collection (xml_buf);
				  free (xml_buf);
			      }
			    if (coll != NULL)
			      {
				  wmsFreeMultipartCollection (multi);
				  goto stop;
			      }
			    single = single->next;
			}
		      wmsFreeMultipartCollection (multi);
		  }
	    }
	  else
	    {
		xml_buf = clean_xml (&bodyBuf);
		if (xml_buf != NULL)
		  {
		      coll = parse_wms_feature_collection (xml_buf);
		      free (xml_buf);
		  }
	    }

	stop:
	  wmsMemBufferReset (&headerBuf);
	  wmsMemBufferReset (&bodyBuf);
	  curl_easy_cleanup (curl);
      }

    sqlite3_free (request);
    if (coll != NULL)
      {
	  if (coll->first == NULL)
	    {
		wmsFreeFeatureCollection (coll);
		coll = NULL;
	    }
      }
    return (rl2WmsFeatureCollectionPtr) coll;
}

RL2_DECLARE rl2WmsFeatureCollectionPtr
do_wms_GetFeatureInfo_post (const char *url, const char *proxy,
			    const char *version, const char *format,
			    const char *layer, const char *crs, int swap_xy,
			    double minx, double miny, double maxx, double maxy,
			    int width, int height, int mouse_x, int mouse_y,
			    char **err_msg)
{
/* attempting to execute a WMS GepFeatureInfo request [method POST] */

/* not yet implemented: just a stupid placeholder always returning NULL */
    if (url == NULL || proxy == NULL || version == NULL || format == NULL
	|| layer == NULL || crs == NULL)
	return NULL;
    if (minx == miny || maxx == maxy || width == height || mouse_x == mouse_y
	|| swap_xy == mouse_x)
	return NULL;
    if (err_msg == NULL)
	return NULL;
    return NULL;
}
