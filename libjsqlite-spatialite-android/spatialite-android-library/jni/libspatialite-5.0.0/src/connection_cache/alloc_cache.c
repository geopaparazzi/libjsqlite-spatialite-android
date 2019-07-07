/*
 alloc_cache.c -- Gaia spatial support for SQLite

 version 4.3, 2015 June 29

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

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2013-2015
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
#include <string.h>

#if defined(_WIN32)
#include <libloaderapi.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifdef ENABLE_LIBXML2		/* only if LIBXML2 is supported */
#include <libxml/parser.h>
#endif /* end LIBXML2 conditional */

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite.h>
#include <spatialite_private.h>
#include <spatialite/gg_advanced.h>
#include <spatialite/gaiamatrix.h>

#ifndef OMIT_GEOS		/* including GEOS */
#ifdef GEOS_REENTRANT
#ifdef GEOS_ONLY_REENTRANT
#define GEOS_USE_ONLY_R_API	/* only fully thread-safe GEOS API */
#endif
#endif
#include <geos_c.h>
#endif

#ifndef OMIT_PROJ		/* including PROJ.4 */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
#include <proj.h>
#else /* supporting old PROJ.4 */
#include <proj_api.h>
#endif
#endif

#ifdef ENABLE_RTTOPO		/* including RTTOPO */
#include <librttopo.h>
#endif

#ifndef GEOS_REENTRANT		/* only when using the obsolete partially thread-safe mode */
#include "cache_aux_1.h"
#endif /* end GEOS_REENTRANT */

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

/* GLOBAL variables */
extern char *gaia_geos_error_msg;
extern char *gaia_geos_warning_msg;

/* GLOBAL semaphores */
int gaia_already_initialized = 0;
#if defined(_WIN32) && !defined(__MINGW32__)
static CRITICAL_SECTION gaia_cache_semaphore;
#else
static pthread_mutex_t gaia_cache_semaphore = PTHREAD_MUTEX_INITIALIZER;
#endif

#define GAIA_CONN_RESERVED	(char *)1

#ifdef PROJ_NEW			/* supporting new PROJ.6 */
static void
gaia_proj_log_funct (void *data, int level, const char *err_msg)
{
/* intercepting PROJ.6 error messages */
    gaiaSetProjErrorMsg_r (data, err_msg);
    if (level == 0)
	return;			/* just silencing stupid compiler warnings aboit unused args */
}
#endif

static void
conn_geos_error (const char *msg, void *userdata)
{
/* reporting some GEOS error - thread safe */
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) userdata;
    if (cache == NULL)
	goto invalid_cache;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	goto invalid_cache;

    if (cache->gaia_geos_error_msg != NULL)
	free (cache->gaia_geos_error_msg);
    cache->gaia_geos_error_msg = NULL;
    if (msg)
      {
	  if (cache->silent_mode == 0)
	      spatialite_e ("GEOS error: %s\n", msg);
	  len = strlen (msg);
	  cache->gaia_geos_error_msg = malloc (len + 1);
	  strcpy (cache->gaia_geos_error_msg, msg);
      }
    return;

  invalid_cache:
    if (msg)
	spatialite_e ("GEOS error: %s\n", msg);
}

static void
conn_geos_warning (const char *msg, void *userdata)
{
/* reporting some GEOS warning - thread safe */
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) userdata;
    if (cache == NULL)
	goto invalid_cache;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	goto invalid_cache;

    if (cache->gaia_geos_warning_msg != NULL)
	free (cache->gaia_geos_warning_msg);
    cache->gaia_geos_warning_msg = NULL;
    if (msg)
      {
	  if (cache->silent_mode == 0)
	      spatialite_e ("GEOS warning: %s\n", msg);
	  len = strlen (msg);
	  cache->gaia_geos_warning_msg = malloc (len + 1);
	  strcpy (cache->gaia_geos_warning_msg, msg);
      }
    return;

  invalid_cache:
    if (msg)
	spatialite_e ("GEOS warning: %s\n", msg);
}

static void
conn_rttopo_error (const char *fmt, va_list ap, void *userdata)
{
/* reporting some RTTOPO error - thread safe */
    char *msg = NULL;
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) userdata;
    if (cache == NULL)
	goto invalid_cache;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	goto invalid_cache;

    if (cache->gaia_rttopo_error_msg != NULL)
	free (cache->gaia_rttopo_error_msg);
    cache->gaia_rttopo_error_msg = NULL;

    msg = sqlite3_vmprintf (fmt, ap);
    if (msg)
      {
	  if (strlen (msg) > 0)
	    {
		if (cache->silent_mode == 0)
		    spatialite_e ("RTTOPO error: %s\n\n", msg);
		len = strlen (msg);
		cache->gaia_rttopo_error_msg = malloc (len + 1);
		strcpy (cache->gaia_rttopo_error_msg, msg);
	    }
	  sqlite3_free (msg);
      }
    return;

  invalid_cache:
    if (msg)
      {
	  spatialite_e ("RTTOPO error: %s\n", msg);
	  sqlite3_free (msg);
      }
}

static void
conn_rttopo_warning (const char *fmt, va_list ap, void *userdata)
{
/* reporting some RTTOPO warning - thread safe */
    char *msg = NULL;
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) userdata;
    if (cache == NULL)
	goto invalid_cache;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	goto invalid_cache;

    if (cache->gaia_rttopo_warning_msg != NULL)
	free (cache->gaia_rttopo_warning_msg);
    cache->gaia_rttopo_warning_msg = NULL;

    msg = sqlite3_vmprintf (fmt, ap);
    if (msg)
      {
	  if (strlen (msg) > 0)
	    {
		/* disabled so to stop endless warnings caused by topo-tolerance */
		if (cache->silent_mode == 0)
		    spatialite_e ("RTTOPO warning: %s\n", msg);
		len = strlen (msg);
		cache->gaia_rttopo_warning_msg = malloc (len + 1);
		strcpy (cache->gaia_rttopo_warning_msg, msg);
	    }
	  sqlite3_free (msg);
      }
    return;

  invalid_cache:
    if (msg)
      {
	  spatialite_e ("RTTOPO warning: %s\n\n", msg);
	  sqlite3_free (msg);
      }
}

#ifndef GEOS_REENTRANT		/* only when using the obsolete partially thread-safe mode */
static void
setGeosErrorMsg (int pool_index, const char *msg)
{
/* setting the latest GEOS error message */
    struct splite_connection *p = &(splite_connection_pool[pool_index]);
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) (p->conn_ptr);
    conn_geos_error (msg, cache);
}

static void
setGeosWarningMsg (int pool_index, const char *msg)
{
/* setting the latest GEOS error message */
    struct splite_connection *p = &(splite_connection_pool[pool_index]);
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) (p->conn_ptr);
    conn_geos_warning (msg, cache);
}

static void
geos_error_r (int pool_index, const char *fmt, va_list ap)
{
/* reporting some GEOS error - thread safe */
    char *msg;
    msg = sqlite3_vmprintf (fmt, ap);
    if (msg)
      {
	  spatialite_e ("GEOS error: %s\n", msg);
	  setGeosErrorMsg (pool_index, msg);
	  sqlite3_free (msg);
      }
    else
	setGeosErrorMsg (pool_index, NULL);
}

static void
geos_warning_r (int pool_index, const char *fmt, va_list ap)
{
/* reporting some GEOS warning - thread safe */
    char *msg;
    msg = sqlite3_vmprintf (fmt, ap);
    if (msg)
      {
	  spatialite_e ("GEOS warning: %s\n", msg);
	  setGeosWarningMsg (pool_index, msg);
	  sqlite3_free (msg);
      }
    else
	setGeosWarningMsg (pool_index, NULL);
}

#include "cache_aux_2.h"

static int
find_free_connection ()
{
    int i;
    for (i = 0; i < SPATIALITE_MAX_CONNECTIONS; i++)
      {
	  struct splite_connection *p = &(splite_connection_pool[i]);
	  if (p->conn_ptr == NULL)
	    {
		p->conn_ptr = GAIA_CONN_RESERVED;
		return i;
	    }
      }
    spatialite_e ("ERROR: Too many connections: max %d\n",
		  SPATIALITE_MAX_CONNECTIONS);
    return -1;
}

static void
confirm (int i, void *cache)
{
/* marking the slot as definitely reserved */
    struct splite_connection *p = &(splite_connection_pool[i]);
    p->conn_ptr = cache;
}

static void
invalidate (int i)
{
/* definitely releasing the slot */
    struct splite_connection *p = &(splite_connection_pool[i]);
    p->conn_ptr = NULL;
}
#endif /* END obsolete partially thread-safe mode */

static void
init_splite_internal_cache (struct splite_internal_cache *cache)
{
/* common initialization of the internal cache */
    gaiaOutBufferPtr out;
    int i;
    const char *tinyPoint;
    struct splite_geos_cache_item *p;
    struct splite_xmlSchema_cache_item *p_xmlSchema;
    if (cache == NULL)
	return;

    cache->magic1 = SPATIALITE_CACHE_MAGIC1;
    cache->magic2 = SPATIALITE_CACHE_MAGIC2;
    cache->gpkg_mode = 0;
    cache->gpkg_amphibious_mode = 0;
    cache->decimal_precision = -1;
    cache->GEOS_handle = NULL;
    cache->PROJ_handle = NULL;
    cache->proj6_cached = 0;
    cache->proj6_cached_pj = NULL;
    cache->proj6_cached_string_1 = NULL;
    cache->proj6_cached_string_2 = NULL;
    cache->proj6_cached_area = NULL;
    cache->is_pause_enabled = 0;
    cache->RTTOPO_handle = NULL;
    cache->cutterMessage = NULL;
    cache->storedProcError = NULL;
    cache->createRoutingError = NULL;
    cache->SqlProcLogfile = NULL;
    cache->SqlProcLogfileAppend = 0;
    cache->SqlProcLog = NULL;
    cache->SqlProcContinue = 1;
    cache->SqlProcRetValue = gaia_alloc_variant ();
    cache->pool_index = -1;
    cache->gaia_proj_error_msg = NULL;
    cache->gaia_geos_error_msg = NULL;
    cache->gaia_geos_warning_msg = NULL;
    cache->gaia_geosaux_error_msg = NULL;
    cache->gaia_rttopo_error_msg = NULL;
    cache->gaia_rttopo_warning_msg = NULL;
    cache->silent_mode = 0;
    cache->tinyPointEnabled = 0;
    tinyPoint = getenv ("SPATIALITE_TINYPOINT");
    if (tinyPoint == NULL)
	;
    else if (atoi (tinyPoint) != 0)
	cache->tinyPointEnabled = 1;
    cache->lastPostgreSqlError = NULL;
    cache->buffer_end_cap_style = GEOSBUF_CAP_ROUND;
    cache->buffer_join_style = GEOSBUF_JOIN_ROUND;
    cache->buffer_mitre_limit = 5.0;
    cache->buffer_quadrant_segments = 30;
/* initializing an empty linked list of Topologies */
    cache->firstTopology = NULL;
    cache->lastTopology = NULL;
    cache->next_topo_savepoint = 0;
    cache->first_topo_svpt = NULL;
    cache->last_topo_svpt = NULL;
    cache->firstNetwork = NULL;
    cache->lastNetwork = NULL;
    cache->next_network_savepoint = 0;
    cache->first_net_svpt = NULL;
    cache->last_net_svpt = NULL;
/* initializing Sequences */
    cache->first_seq = NULL;
    cache->last_seq = NULL;
    cache->ok_last_used_sequence = 0;
    cache->last_used_sequence_val = 0;
/* initializing Virtual BBOXes */
    cache->first_vtable_extent = NULL;
    cache->last_vtable_extent = NULL;
/* initializing the XML error buffers */
    out = malloc (sizeof (gaiaOutBuffer));
    gaiaOutBufferInitialize (out);
    cache->xmlParsingErrors = out;
    out = malloc (sizeof (gaiaOutBuffer));
    gaiaOutBufferInitialize (out);
    cache->xmlSchemaValidationErrors = out;
    out = malloc (sizeof (gaiaOutBuffer));
    gaiaOutBufferInitialize (out);
    cache->xmlXPathErrors = out;
/* initializing the GEOS cache */
    p = &(cache->cacheItem1);
    memset (p->gaiaBlob, '\0', 64);
    p->gaiaBlobSize = 0;
    p->crc32 = 0;
    p->geosGeom = NULL;
    p->preparedGeosGeom = NULL;
    p = &(cache->cacheItem2);
    memset (p->gaiaBlob, '\0', 64);
    p->gaiaBlobSize = 0;
    p->crc32 = 0;
    p->geosGeom = NULL;
    p->preparedGeosGeom = NULL;
    for (i = 0; i < MAX_XMLSCHEMA_CACHE; i++)
      {
	  /* initializing the XmlSchema cache */
	  p_xmlSchema = &(cache->xmlSchemaCache[i]);
	  p_xmlSchema->timestamp = 0;
	  p_xmlSchema->schemaURI = NULL;
	  p_xmlSchema->schemaDoc = NULL;
	  p_xmlSchema->parserCtxt = NULL;
	  p_xmlSchema->schema = NULL;
      }
}

#ifdef GEOS_REENTRANT		/* reentrant (thread-safe) initialization */
static void *
spatialite_alloc_reentrant ()
{
/* 
 * allocating and initializing an empty internal cache 
 * fully reentrant (thread-safe) version requiring GEOS >= 3.5.0
*/
    struct splite_internal_cache *cache = NULL;
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    int proj_set_ext_var = 0;
    char *proj_db = NULL;
    const char *proj_db_path = NULL;
#ifdef _WIN32
    char *win_prefix = NULL;
#endif
#endif

/* attempting to implicitly initialize the library */
    spatialite_initialize ();

    cache = malloc (sizeof (struct splite_internal_cache));
    if (cache == NULL)
	goto done;
    init_splite_internal_cache (cache);

/* initializing GEOS and PROJ.4 handles */

#ifndef OMIT_GEOS		/* initializing GEOS */
    cache->GEOS_handle = GEOS_init_r (NULL, NULL);
    GEOSContext_setNoticeMessageHandler_r (cache->GEOS_handle,
					   conn_geos_warning, cache);
    GEOSContext_setErrorMessageHandler_r (cache->GEOS_handle, conn_geos_error,
					  cache);
#endif /* end GEOS  */

#ifndef OMIT_PROJ		/* initializing the PROJ.4 context */
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    cache->PROJ_handle = proj_context_create ();
    proj_log_func (cache->PROJ_handle, cache, gaia_proj_log_funct);	/* installing an error handler routine */
    if (getenv ("PROJ_LIB") != NULL)
	proj_db = sqlite3_mprintf ("%s/proj.db", getenv ("PROJ_LIB"));
    if (proj_db != NULL)
      {
	  proj_context_set_database_path (cache->PROJ_handle, proj_db,
					  NULL, NULL);
	  sqlite3_free (proj_db);
	  goto skip_win;
      }
#ifdef _WIN32			/* only for Windows - checking the default locations for PROJ.6 DB */
    proj_set_ext_var = 1;
    proj_db_path = proj_context_get_database_path (cache->PROJ_handle);
    if (proj_db_path == NULL)
      {
	  char *win_path;
	  char *exe_path;
	  int max_len = 8192;
	  int i;
	  exe_path = malloc (max_len);
	  GetModuleFileNameA (NULL, exe_path, max_len);
	  if (exe_path != NULL)
	    {
		/* searching within the EXE's launch dir */
		for (i = strlen (exe_path) - 1; i >= 0; i--)
		  {
		      /* truncating the EXE name */
		      if (exe_path[i] == '\\')
			{
			    exe_path[i] = '\0';
			    break;
			}
		  }
		win_prefix = sqlite3_mprintf ("%s", exe_path);
		win_path = sqlite3_mprintf ("%s\\proj.db", exe_path);
		free (exe_path);
		proj_context_set_database_path (cache->PROJ_handle, win_path,
						NULL, NULL);
		proj_db_path =
		    proj_context_get_database_path (cache->PROJ_handle);
		sqlite3_free (win_path);
	    }
	  if (proj_db_path == NULL)
	    {
		/* searching on PUBLIC dir */
		if (win_prefix != NULL)
		    sqlite3_free (win_prefix);
		win_prefix =
		    sqlite3_mprintf ("%s\\spatialite\\proj", getenv ("PUBLIC"));
		win_path =
		    sqlite3_mprintf ("%s\\spatialite\\proj\\proj.db",
				     getenv ("PUBLIC"));
		proj_context_set_database_path (cache->PROJ_handle, win_path,
						NULL, NULL);
		proj_db_path =
		    proj_context_get_database_path (cache->PROJ_handle);
		sqlite3_free (win_path);
	    }
	  if (proj_db_path == NULL)
	    {
		/* searching on USER dir */
		if (win_prefix != NULL)
		    sqlite3_free (win_prefix);
		win_prefix =
		    sqlite3_mprintf ("%s\\spatialite\\proj",
				     getenv ("USERPROFILE"));
		win_path =
		    sqlite3_mprintf ("%s\\spatialite\\proj\\proj.db",
				     getenv ("USERPROFILE"));
		proj_context_set_database_path (cache->PROJ_handle, win_path,
						NULL, NULL);
		sqlite3_free (win_path);
	    }
      }
#endif
  skip_win:
    proj_db_path = proj_context_get_database_path (cache->PROJ_handle);
#ifdef _WIN32			/* only for Windows - setting PROJ_LIB */
    if (proj_set_ext_var && win_prefix && proj_db_path)
      {
	  char *proj_lib = sqlite3_mprintf ("PROJ_LIB=%s", win_prefix);
	  putenv (proj_lib);
	  sqlite3_free (proj_lib);
      }
    if (win_prefix != NULL)
	sqlite3_free (win_prefix);
#else
/* suppressing stupid compiler warnings about unused args */
    if (proj_db_path == NULL && proj_set_ext_var < 0)
	proj_set_ext_var = 1;
#endif

#else /* supporting old PROJ.4 */
    cache->PROJ_handle = pj_ctx_alloc ();
#endif
#endif /* end PROJ.4  */

#ifdef ENABLE_RTTOPO		/* initializing the RTTOPO context */
    cache->RTTOPO_handle = rtgeom_init (NULL, NULL, NULL);
    rtgeom_set_error_logger (cache->RTTOPO_handle, conn_rttopo_error, cache);
    rtgeom_set_notice_logger (cache->RTTOPO_handle, conn_rttopo_warning, cache);
#endif /* end RTTOPO */

  done:
    return cache;
}
#endif /* end GEOS_REENTRANT */

SPATIALITE_DECLARE void *
spatialite_alloc_connection ()
{
/* allocating and initializing an empty internal cache */

#ifdef GEOS_REENTRANT		/* reentrant (thread-safe) initialization */
    return spatialite_alloc_reentrant ();
#else /* end GEOS_REENTRANT */
    struct splite_internal_cache *cache = NULL;
    int pool_index;

/* attempting to implicitly initialize the library */
    spatialite_initialize ();

/* locking the semaphore */
    splite_cache_semaphore_lock ();

    pool_index = find_free_connection ();

    if (pool_index < 0)
	goto done;

    cache = malloc (sizeof (struct splite_internal_cache));
    if (cache == NULL)
      {
	  invalidate (pool_index);
	  goto done;
      }
    init_splite_internal_cache (cache);
    cache->pool_index = pool_index;
    confirm (pool_index, cache);

#include "cache_aux_3.h"

/* initializing GEOS and PROJ.4 handles */

#ifndef OMIT_GEOS		/* initializing GEOS */
    cache->GEOS_handle = initGEOS_r (cache->geos_warning, cache->geos_error);
#endif /* end GEOS  */

#ifndef OMIT_PROJ		/* initializing the PROJ.4 context */
    cache->PROJ_handle = pj_ctx_alloc ();
#endif /* end PROJ.4  */

  done:
/* unlocking the semaphore */
    splite_cache_semaphore_unlock ();
    return cache;
#endif
}

SPATIALITE_DECLARE void
spatialite_finalize_topologies (const void *ptr)
{
#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */
/* freeing all Topology Accessor Objects */
    struct splite_savepoint *p_svpt;
    struct splite_savepoint *p_svpt_n;
    struct splite_internal_cache *cache = (struct splite_internal_cache *) ptr;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;
    free_internal_cache_topologies (cache->firstTopology);
    cache->firstTopology = NULL;
    cache->lastTopology = NULL;
    p_svpt = cache->first_topo_svpt;
    while (p_svpt != NULL)
      {
	  p_svpt_n = p_svpt->next;
	  if (p_svpt->savepoint_name != NULL)
	      sqlite3_free (p_svpt->savepoint_name);
	  free (p_svpt);
	  p_svpt = p_svpt_n;
      }
    cache->first_topo_svpt = NULL;
    cache->last_topo_svpt = NULL;
    free_internal_cache_networks (cache->firstNetwork);
    cache->firstNetwork = NULL;
    cache->lastTopology = NULL;
    p_svpt = cache->first_net_svpt;
    while (p_svpt != NULL)
      {
	  p_svpt_n = p_svpt->next;
	  if (p_svpt->savepoint_name != NULL)
	      sqlite3_free (p_svpt->savepoint_name);
	  free (p_svpt);
	  p_svpt = p_svpt_n;
      }
    cache->first_net_svpt = NULL;
    cache->last_net_svpt = NULL;
#else
    if (ptr == NULL)
	return;			/* silencing stupid compiler warnings */
#endif /* end RTTOPO conditionals */
}

static void
free_sequences (struct splite_internal_cache *cache)
{
/* freeing all Sequences */
    gaiaSequencePtr pS;
    gaiaSequencePtr pSn;

    pS = cache->first_seq;
    while (pS != NULL)
      {
	  pSn = pS->next;
	  if (pS->seq_name != NULL)
	      free (pS->seq_name);
	  free (pS);
	  pS = pSn;
      }
}

static void
free_vtable_extents (struct splite_internal_cache *cache)
{
/* freeing all Virtual BBOXes */
    struct splite_vtable_extent *pS;
    struct splite_vtable_extent *pSn;

    pS = cache->first_vtable_extent;
    while (pS != NULL)
      {
	  pSn = pS->next;
	  if (pS->table != NULL)
	      free (pS->table);
	  free (pS);
	  pS = pSn;
      }
}

SPATIALITE_PRIVATE void
add_vtable_extent (const char *table, double minx,
		   double miny, double maxx,
		   double maxy, int srid, const void *p_cache)
{
/* adding a Virtual Full Extent */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    struct splite_vtable_extent *vtable =
	malloc (sizeof (struct splite_vtable_extent));
    int len = strlen (table);
    vtable->table = malloc (len + 1);
    strcpy (vtable->table, table);
    vtable->minx = minx;
    vtable->miny = miny;
    vtable->maxx = maxx;
    vtable->maxy = maxy;
    vtable->srid = srid;
    vtable->prev = cache->last_vtable_extent;
    vtable->next = NULL;
    if (cache->first_vtable_extent == NULL)
	cache->first_vtable_extent = vtable;
    if (cache->last_vtable_extent != NULL)
	cache->last_vtable_extent->next = vtable;
    cache->last_vtable_extent = vtable;
}

SPATIALITE_PRIVATE void
remove_vtable_extent (const char *table, const void *p_cache)
{
/* adding a Virtual Full Extent */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    struct splite_vtable_extent *vtable_n;
    struct splite_vtable_extent *vtable = cache->first_vtable_extent;
    while (vtable != NULL)
      {
	  vtable_n = vtable->next;
	  if (strcasecmp (vtable->table, table) == 0)
	    {
		if (vtable->table != NULL)
		    free (vtable->table);
		if (vtable->next != NULL)
		    vtable->next->prev = vtable->prev;
		if (vtable->prev != NULL)
		    vtable->prev->next = vtable->next;
		if (cache->first_vtable_extent == vtable)
		    cache->first_vtable_extent = vtable->next;
		if (cache->last_vtable_extent == vtable)
		    cache->last_vtable_extent = vtable->prev;
		free (vtable);
	    }
	  vtable = vtable_n;
      }
}

SPATIALITE_PRIVATE int
get_vtable_extent (const char *table, double *minx, double *miny, double *maxx,
		   double *maxy, int *srid, const void *p_cache)
{
/* retrieving a Virtual Full Extent */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    struct splite_vtable_extent *vtable = cache->first_vtable_extent;
    while (vtable != NULL)
      {
	  if (strcasecmp (vtable->table, table) == 0)
	    {
		*minx = vtable->minx;
		*miny = vtable->miny;
		*maxx = vtable->maxx;
		*maxy = vtable->maxy;
		*srid = vtable->srid;
		return 1;
	    }
	  vtable = vtable->next;
      }
    return 0;
}

SPATIALITE_PRIVATE void
free_internal_cache (struct splite_internal_cache *cache)
{
/* freeing an internal cache */
    struct splite_geos_cache_item *p;
#ifndef OMIT_GEOS
    GEOSContextHandle_t handle = NULL;
#endif

#ifdef ENABLE_LIBXML2
    int i;
    struct splite_xmlSchema_cache_item *p_xmlSchema;
#endif

    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;

    if (cache->SqlProcRetValue != NULL)
	gaia_free_variant (cache->SqlProcRetValue);
    cache->SqlProcRetValue = NULL;

#ifndef OMIT_GEOS
    handle = cache->GEOS_handle;
    if (handle != NULL)
#ifdef GEOS_REENTRANT		/* reentrant (thread-safe) initialization */
	GEOS_finish_r (handle);
#else /* end GEOS_REENTRANT */
	finishGEOS_r (handle);
#endif
    cache->GEOS_handle = NULL;
    gaiaResetGeosMsg_r (cache);
#endif

#ifndef OMIT_PROJ
#ifdef PROJ_NEW			/* supporting new PROJ.6 */
    if (cache->proj6_cached_string_1 != NULL)
	free (cache->proj6_cached_string_1);
    if (cache->proj6_cached_string_2 != NULL)
	free (cache->proj6_cached_string_2);
    if (cache->proj6_cached_area != NULL)
	free (cache->proj6_cached_area);
    if (cache->proj6_cached_pj != NULL)
	proj_destroy (cache->proj6_cached_pj);
    if (cache->PROJ_handle != NULL)
	proj_context_destroy (cache->PROJ_handle);
    cache->PROJ_handle = NULL;
    cache->proj6_cached = 0;
    cache->proj6_cached_pj = NULL;
    cache->proj6_cached_string_1 = NULL;
    cache->proj6_cached_string_2 = NULL;
    cache->proj6_cached_area = NULL;
#else /* supporting old PROJ.4 */
    if (cache->PROJ_handle != NULL)
	pj_ctx_free (cache->PROJ_handle);
    cache->PROJ_handle = NULL;
#endif
#endif

/* freeing PROJ error buffer */
    if (cache->gaia_proj_error_msg)
	sqlite3_free (cache->gaia_proj_error_msg);

/* freeing GEOS error buffers */
    if (cache->gaia_geos_error_msg)
	free (cache->gaia_geos_error_msg);
    if (cache->gaia_geos_warning_msg)
	free (cache->gaia_geos_warning_msg);
    if (cache->gaia_geosaux_error_msg)
	free (cache->gaia_geosaux_error_msg);

/* freeing RTTOPO error buffers */
    if (cache->gaia_rttopo_error_msg)
	free (cache->gaia_rttopo_error_msg);
    if (cache->gaia_rttopo_warning_msg)
	free (cache->gaia_rttopo_warning_msg);

/* freeing the XML error buffers */
    gaiaOutBufferReset (cache->xmlParsingErrors);
    gaiaOutBufferReset (cache->xmlSchemaValidationErrors);
    gaiaOutBufferReset (cache->xmlXPathErrors);
    free (cache->xmlParsingErrors);
    free (cache->xmlSchemaValidationErrors);
    free (cache->xmlXPathErrors);

/* freeing the GEOS cache */
    p = &(cache->cacheItem1);
    splite_free_geos_cache_item_r (cache, p);
    p = &(cache->cacheItem2);
    splite_free_geos_cache_item_r (cache, p);
#ifdef ENABLE_LIBXML2
    for (i = 0; i < MAX_XMLSCHEMA_CACHE; i++)
      {
	  /* freeing the XmlSchema cache */
	  p_xmlSchema = &(cache->xmlSchemaCache[i]);
	  splite_free_xml_schema_cache_item (p_xmlSchema);
      }
#endif

    if (cache->lastPostgreSqlError != NULL)
	sqlite3_free (cache->lastPostgreSqlError);

    if (cache->cutterMessage != NULL)
	sqlite3_free (cache->cutterMessage);
    cache->cutterMessage = NULL;
    if (cache->createRoutingError != NULL)
	free (cache->createRoutingError);
    cache->createRoutingError = NULL;
    if (cache->storedProcError != NULL)
	free (cache->storedProcError);
    cache->storedProcError = NULL;
    if (cache->SqlProcLogfile != NULL)
	free (cache->SqlProcLogfile);
    cache->SqlProcLogfile = NULL;
    if (cache->SqlProcLog != NULL)
	fclose (cache->SqlProcLog);
    cache->SqlProcLog = NULL;
    free_sequences (cache);
    free_vtable_extents (cache);

    spatialite_finalize_topologies (cache);

#ifdef ENABLE_RTTOPO
    if (cache->RTTOPO_handle != NULL)
	rtgeom_finish (cache->RTTOPO_handle);
    cache->RTTOPO_handle = NULL;
#endif

#ifndef GEOS_REENTRANT		/* only partially thread-safe mode */
/* releasing the connection pool object */
    invalidate (cache->pool_index);
#endif /* end GEOS_REENTRANT */

/* freeing the cache itself */
    free (cache);
}

GAIAGEO_DECLARE void
gaiaResetGeosMsg_r (const void *p_cache)
{
/* resets the GEOS error and warning messages */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	;
    else
	return;
    if (cache->gaia_geos_error_msg != NULL)
	free (cache->gaia_geos_error_msg);
    if (cache->gaia_geos_warning_msg != NULL)
	free (cache->gaia_geos_warning_msg);
    if (cache->gaia_geosaux_error_msg != NULL)
	free (cache->gaia_geosaux_error_msg);
    cache->gaia_geos_error_msg = NULL;
    cache->gaia_geos_warning_msg = NULL;
    cache->gaia_geosaux_error_msg = NULL;
}

GAIAGEO_DECLARE const char *
gaiaGetGeosErrorMsg_r (const void *p_cache)
{
/* return the latest GEOS error message */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	;
    else
	return NULL;
    return cache->gaia_geos_error_msg;
}

GAIAGEO_DECLARE const char *
gaiaGetGeosWarningMsg_r (const void *p_cache)
{
/* return the latest GEOS error message */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	;
    else
	return NULL;
    return cache->gaia_geos_warning_msg;
}

GAIAGEO_DECLARE const char *
gaiaGetGeosAuxErrorMsg_r (const void *p_cache)
{
/* return the latest GEOS (auxialiary) error message */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	;
    else
	return NULL;
    return cache->gaia_geosaux_error_msg;
}

GAIAGEO_DECLARE void
gaiaSetGeosErrorMsg_r (const void *p_cache, const char *msg)
{
/* setting the latest GEOS error message */
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	;
    else
	return;
    if (cache->gaia_geos_error_msg != NULL)
	free (cache->gaia_geos_error_msg);
    cache->gaia_geos_error_msg = NULL;
    if (msg == NULL)
	return;
    len = strlen (msg);
    cache->gaia_geos_error_msg = malloc (len + 1);
    strcpy (cache->gaia_geos_error_msg, msg);
}

GAIAGEO_DECLARE void
gaiaSetGeosWarningMsg_r (const void *p_cache, const char *msg)
{
/* setting the latest GEOS error message */
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	;
    else
	return;
    if (cache->gaia_geos_warning_msg != NULL)
	free (cache->gaia_geos_warning_msg);
    cache->gaia_geos_warning_msg = NULL;
    if (msg == NULL)
	return;
    len = strlen (msg);
    cache->gaia_geos_warning_msg = malloc (len + 1);
    strcpy (cache->gaia_geos_warning_msg, msg);
}

GAIAGEO_DECLARE void
gaiaSetGeosAuxErrorMsg_r (const void *p_cache, const char *msg)
{
/* setting the latest GEOS (auxiliary) error message */
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	;
    else
	return;
    if (cache->gaia_geosaux_error_msg != NULL)
	free (cache->gaia_geosaux_error_msg);
    cache->gaia_geosaux_error_msg = NULL;
    if (msg == NULL)
	return;
    len = strlen (msg);
    cache->gaia_geosaux_error_msg = malloc (len + 1);
    strcpy (cache->gaia_geosaux_error_msg, msg);
}

static char *
parse_number_from_msg (const char *str)
{
/* attempting to parse a number from a string */
    int sign = 0;
    int point = 0;
    int digit = 0;
    int err = 0;
    int len;
    char *res;
    const char *p = str;
    while (1)
      {
	  if (*p == '+' || *p == '-')
	    {
		sign++;
		p++;
		continue;
	    }
	  if (*p == '.')
	    {
		point++;
		p++;
		continue;
	    }
	  if (*p >= '0' && *p <= '9')
	    {
		p++;
		digit++;
		continue;
	    }
	  break;
      }
    if (sign > 1)
	err = 1;
    if (sign == 1 && *str != '+' && *str != '-')
	err = 1;
    if (point > 1)
	err = 1;
    if (!digit)
	err = 1;
    if (err)
	return NULL;
    len = p - str;
    res = malloc (len + 1);
    memcpy (res, str, len);
    *(res + len) = '\0';
    return res;
}

static int
check_geos_critical_point (const char *msg, double *x, double *y)
{
/* attempts to extract an [X,Y] Point coords from within a string */
    char *px;
    char *py;
    const char *ref = " at or near point ";
    const char *ref2 = " conflict at ";
    const char *p = strstr (msg, ref);
    if (p != NULL)
	goto ok_ref;
    p = strstr (msg, ref2);
    if (p == NULL)
	return 0;
    p += strlen (ref2);
    goto ok_ref2;
  ok_ref:
    p += strlen (ref);
  ok_ref2:
    px = parse_number_from_msg (p);
    if (px == NULL)
	return 0;
    p += strlen (px) + 1;
    py = parse_number_from_msg (p);
    if (py == NULL)
      {
	  free (px);
	  return 0;
      }
    *x = atof (px);
    *y = atof (py);
    free (px);
    free (py);
    return 1;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaCriticalPointFromGEOSmsg (void)
{
/*
/ Attempts to return a Point Geometry extracted from the latest GEOS 
/ error / warning message
*/
    double x;
    double y;
    gaiaGeomCollPtr geom;
    const char *msg = gaia_geos_error_msg;
    if (msg == NULL)
	msg = gaia_geos_warning_msg;
    if (msg == NULL)
	return NULL;
    if (!check_geos_critical_point (msg, &x, &y))
	return NULL;
    geom = gaiaAllocGeomColl ();
    gaiaAddPointToGeomColl (geom, x, y);
    return geom;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaCriticalPointFromGEOSmsg_r (const void *p_cache)
{
/*
/ Attempts to return a Point Geometry extracted from the latest GEOS 
/ error / warning message
*/
    double x;
    double y;
    gaiaGeomCollPtr geom;
    const char *msg;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;

    if (cache == NULL)
	return NULL;
    if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	;
    else
	return NULL;

    msg = cache->gaia_geos_error_msg;
    if (msg == NULL)
	msg = cache->gaia_geos_warning_msg;
    if (msg == NULL)
	return NULL;
    if (!check_geos_critical_point (msg, &x, &y))
	return NULL;
    geom = gaiaAllocGeomColl ();
    gaiaAddPointToGeomColl (geom, x, y);
    return geom;
}

SPATIALITE_PRIVATE void
splite_cache_semaphore_lock (void)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    EnterCriticalSection (&gaia_cache_semaphore);
#else
    pthread_mutex_lock (&gaia_cache_semaphore);
#endif
}

SPATIALITE_PRIVATE void
splite_cache_semaphore_unlock (void)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    LeaveCriticalSection (&gaia_cache_semaphore);
#else
    pthread_mutex_trylock (&gaia_cache_semaphore);
    pthread_mutex_unlock (&gaia_cache_semaphore);
#endif
}

SPATIALITE_DECLARE void
spatialite_initialize (void)
{
/* initializes the library */
    if (gaia_already_initialized)
	return;

#if defined(_WIN32) && !defined(__MINGW32__)
    InitializeCriticalSection (&gaia_cache_semaphore);
#endif

#ifdef ENABLE_LIBXML2		/* only if LIBXML2 is supported */
    xmlInitParser ();
#endif /* end LIBXML2 conditional */

    gaia_already_initialized = 1;
}

SPATIALITE_DECLARE void
spatialite_shutdown (void)
{
/* finalizes the library */
#ifndef GEOS_REENTRANT
    int i;
#endif
    if (!gaia_already_initialized)
	return;

#if defined(_WIN32) && !defined(__MINGW32__)
    DeleteCriticalSection (&gaia_cache_semaphore);
#endif

#ifdef ENABLE_LIBXML2		/* only if LIBXML2 is supported */
    xmlCleanupParser ();
#endif /* end LIBXML2 conditional */

#ifndef GEOS_REENTRANT		/* only when using the obsolete partially thread-safe mode */
    for (i = 0; i < SPATIALITE_MAX_CONNECTIONS; i++)
      {
	  struct splite_connection *p = &(splite_connection_pool[i]);
	  if (p->conn_ptr != NULL && p->conn_ptr != GAIA_CONN_RESERVED)
	      free_internal_cache (p->conn_ptr);
      }
#endif /* end GEOS_REENTRANT */

    gaia_already_initialized = 0;
}

SPATIALITE_DECLARE void
spatialite_set_silent_mode (const void *p_cache)
{
/* setting up the SILENT mode */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;
    cache->silent_mode = 1;
}

SPATIALITE_DECLARE void
spatialite_set_verbose_mode (const void *p_cache)
{
/* setting up the VERBOSE mode */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;
    cache->silent_mode = 0;
}

SPATIALITE_DECLARE void
enable_tiny_point (const void *p_cache)
{
/* Enabling the BLOB-TinyPoint encoding */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;
    cache->tinyPointEnabled = 1;
}

SPATIALITE_DECLARE void
disable_tiny_point (const void *p_cache)
{
/* Disabling the BLOB-TinyPoint encoding */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;
    cache->tinyPointEnabled = 0;
}

SPATIALITE_DECLARE int
is_tiny_point_enabled (const void *p_cache)
{
/* Checking if the BLOB-TinyPoint encoding is enabled or not */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    return cache->tinyPointEnabled;
}

SPATIALITE_PRIVATE struct gaia_variant_value *
gaia_alloc_variant ()
{
/* allocating and initializing a NULL Variant Value */
    struct gaia_variant_value *var =
	malloc (sizeof (struct gaia_variant_value));
    if (var == NULL)
	return NULL;
    var->dataType = SQLITE_NULL;
    var->textValue = NULL;
    var->blobValue = NULL;
    var->size = 0;
    return var;
}

SPATIALITE_PRIVATE void
gaia_free_variant (struct gaia_variant_value *variant)
{
/* destroying a Variant Value */
    if (variant == NULL)
	return;
    if (variant->textValue != NULL)
	free (variant->textValue);
    if (variant->blobValue != NULL)
	free (variant->blobValue);
    free (variant);
}

SPATIALITE_PRIVATE void
gaia_set_variant_null (struct gaia_variant_value *variant)
{
/* setting a Variant Value - NULL */
    if (variant->textValue != NULL)
	free (variant->textValue);
    if (variant->blobValue != NULL)
	free (variant->blobValue);
    variant->dataType = SQLITE_NULL;
    variant->textValue = NULL;
    variant->blobValue = NULL;
    variant->size = 0;
}

SPATIALITE_PRIVATE void
gaia_set_variant_int64 (struct gaia_variant_value *variant, sqlite3_int64 value)
{
/* setting a Variant Value - INT64 */
    if (variant->textValue != NULL)
	free (variant->textValue);
    if (variant->blobValue != NULL)
	free (variant->blobValue);
    variant->dataType = SQLITE_INTEGER;
    variant->intValue = value;
    variant->textValue = NULL;
    variant->blobValue = NULL;
    variant->size = 0;
}

SPATIALITE_PRIVATE void
gaia_set_variant_double (struct gaia_variant_value *variant, double value)
{
/* setting a Variant Value - DOUBLE */
    if (variant->textValue != NULL)
	free (variant->textValue);
    if (variant->blobValue != NULL)
	free (variant->blobValue);
    variant->dataType = SQLITE_FLOAT;
    variant->dblValue = value;
    variant->textValue = NULL;
    variant->blobValue = NULL;
    variant->size = 0;
}

SPATIALITE_PRIVATE int
gaia_set_variant_text (struct gaia_variant_value *variant, const char *value,
		       int size)
{
/* setting a Variant Value - TEXT */
    char *text;
    if (variant->textValue != NULL)
	free (variant->textValue);
    if (variant->blobValue != NULL)
	free (variant->blobValue);
    text = malloc (size + 1);
    if (text == NULL)
      {
	  variant->dataType = SQLITE_NULL;
	  variant->textValue = NULL;
	  variant->blobValue = NULL;
	  variant->size = 0;
	  return 0;
      }
    variant->dataType = SQLITE_TEXT;
    strcpy (text, value);
    variant->textValue = text;
    variant->blobValue = NULL;
    variant->size = size;
    return 1;
}

SPATIALITE_PRIVATE int
gaia_set_variant_blob (struct gaia_variant_value *variant,
		       const unsigned char *value, int size)
{
/* setting a Variant Value - BLOB */
    unsigned char *blob;
    if (variant->textValue != NULL)
	free (variant->textValue);
    if (variant->blobValue != NULL)
	free (variant->blobValue);
    blob = malloc (size + 1);
    if (blob == NULL)
      {
	  variant->dataType = SQLITE_NULL;
	  variant->textValue = NULL;
	  variant->blobValue = NULL;
	  variant->size = 0;
	  return 0;
      }
    variant->dataType = SQLITE_BLOB;
    memcpy (blob, value, size);
    variant->textValue = NULL;
    variant->blobValue = blob;
    variant->size = size;
    return 1;
}

#ifdef PROJ_NEW			/* only when using new PROJ.6 */
SPATIALITE_DECLARE const void *
gaiaGetCurrentProjContext (const void *p_cache)
{
/* return the current PROJ.6 context (if any) */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	      return cache->PROJ_handle;
      }
    return NULL;
}

SPATIALITE_DECLARE int
gaiaSetCurrentCachedProj (const void
			  *p_cache, void *pj,
			  const char *proj_string_1,
			  const char *proj_string_2, void *area)
{
/* updates the PROJ6 internal cache */
    int ok = 0;
    int len;
    gaiaProjAreaPtr bbox_in = (gaiaProjAreaPtr) area;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	      ok = 1;
      }
    if (!ok)
	return 0;		/* invalid cache */
    if (proj_string_1 == NULL || pj == NULL)
	return 0;

/* resetting the PROJ6 internal cache */
    if (cache->proj6_cached_string_1 != NULL)
	free (cache->proj6_cached_string_1);
    if (cache->proj6_cached_string_2 != NULL)
	free (cache->proj6_cached_string_2);
    if (cache->proj6_cached_area != NULL)
	free (cache->proj6_cached_area);
    if (cache->proj6_cached_pj != NULL)
	proj_destroy (cache->proj6_cached_pj);

/* updating the PROJ6 internal cache */
    cache->proj6_cached = 1;
    cache->proj6_cached_pj = pj;
    len = strlen (proj_string_1);
    cache->proj6_cached_string_1 = malloc (len + 1);
    strcpy (cache->proj6_cached_string_1, proj_string_1);
    if (proj_string_2 == NULL)
	cache->proj6_cached_string_2 = NULL;
    else
      {
	  len = strlen (proj_string_2);
	  cache->proj6_cached_string_2 = malloc (len + 1);
	  strcpy (cache->proj6_cached_string_2, proj_string_2);
      }
    if (bbox_in == NULL)
	cache->proj6_cached_area = NULL;
    else
      {
	  gaiaProjAreaPtr bbox_out =
	      (gaiaProjAreaPtr) (cache->proj6_cached_area);
	  bbox_out = malloc (sizeof (gaiaProjArea));
	  bbox_out->WestLongitude = bbox_in->WestLongitude;
	  bbox_out->SouthLatitude = bbox_in->SouthLatitude;
	  bbox_out->EastLongitude = bbox_in->EastLongitude;
	  bbox_out->NorthLatitude = bbox_in->NorthLatitude;
      }
    return 1;
}

SPATIALITE_DECLARE void *
gaiaGetCurrentCachedProj (const void *p_cache)
{
/* returning the currently cached PROJ6 object */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	    {
		if (cache->proj6_cached)
		    return cache->proj6_cached_pj;
		else
		    return NULL;
	    }
      }
    return NULL;		/* invalid cache */
}

SPATIALITE_DECLARE int
gaiaCurrentCachedProjMatches (const void *p_cache,
			      const char
			      *proj_string_1,
			      const char *proj_string_2, void *area)
{
/* checking if the currently cached PROJ6 object matches */
    int ok = 0;
    gaiaProjAreaPtr bbox_1 = (gaiaProjAreaPtr) area;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache != NULL)
      {
	  if (cache->magic1 == SPATIALITE_CACHE_MAGIC1
	      && cache->magic2 == SPATIALITE_CACHE_MAGIC2)
	      ok = 1;
      }
    if (!ok)
	return 0;		/* invalid cache */
    if (proj_string_1 == NULL)
	return 0;		/* invalid request */
    if (cache->proj6_cached == 0)
	return 0;		/* there is no PROJ6 object currently cached */

/* checking all definitions */
    if (strcmp (proj_string_1, cache->proj6_cached_string_1) != 0)
	return 0;		/* mismatching string #1 */
    if (proj_string_2 == NULL && cache->proj6_cached_string_2 == NULL)
	;
    else if (proj_string_2 != NULL && cache->proj6_cached_string_2 != NULL)
      {
	  if (strcmp (proj_string_2, cache->proj6_cached_string_2) != 0)
	      return 0;		/* mismatching string #2 */
      }
    else
	return 0;		/* mismatching string #2 */
    if (bbox_1 == NULL && cache->proj6_cached_area == NULL)
	;
    else if (bbox_1 != NULL && cache->proj6_cached_area != NULL)
      {
	  gaiaProjAreaPtr bbox_2 = (gaiaProjAreaPtr) (cache->proj6_cached_area);
	  if (bbox_1->WestLongitude != bbox_2->WestLongitude)
	      return 0;
	  if (bbox_1->SouthLatitude != bbox_2->SouthLatitude)
	      return 0;
	  if (bbox_1->EastLongitude != bbox_2->EastLongitude)
	      return 0;
	  if (bbox_1->NorthLatitude != bbox_2->NorthLatitude)
	      return 0;
      }
    else
	return 0;		/* mismatching area */

    return 1;			/* anything nicely matches */
}
#endif
