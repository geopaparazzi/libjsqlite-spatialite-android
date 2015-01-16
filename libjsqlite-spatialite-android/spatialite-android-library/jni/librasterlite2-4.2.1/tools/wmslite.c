/* 
/ wmslite
/
/ a light-weight WMS server supporting RasterLite2 DataSources
/
/ version 1.0, 2014 January 29
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
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <float.h>

#ifdef _WIN32
/* This code is for win32 only */
#include <windows.h>
#include <process.h>
#else
/* this code is for any sane minded system (*nix) */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#endif

#include <rasterlite2/rasterlite2.h>
#include <spatialite.h>

#define ARG_NONE		0
#define ARG_DB_PATH		1
#define ARG_IP_PORT		2
#define ARG_CACHE_SIZE		3

#define WMS_ILLEGAL_REQUEST	0
#define WMS_GET_CAPABILITIES	1
#define WMS_GET_MAP		2

#define WMS_UNKNOWN		-1
#define WMS_TRANSPARENT		10
#define WMS_OPAQUE		11

#define WMS_INVALID_CRS		101
#define WMS_INVALID_DIMENSION	102
#define WMS_INVALID_BBOX	103
#define WMS_INVALID_LAYER	104
#define WMS_INVALID_GROUP	105
#define WMS_INVALID_BGCOLOR	106
#define WMS_INVALID_STYLE	107
#define WMS_INVALID_FORMAT	108
#define WMS_INVALID_TRANSPARENT	109
#define WMS_NOT_EXISTING_LAYER	110
#define WMS_LAYER_OUT_OF_BBOX	111
#define WMS_MISMATCHING_SRID	112

#define WMS_VERSION_UNKNOWN	0
#define WMS_VERSION_100		100
#define WMS_VERSION_110		110
#define WMS_VERSION_111		111
#define WMS_VERSION_130		130

#define CONNECTION_INVALID	0
#define CONNECTION_AVAILABLE	1
#define CONNECTION_BUSY		2

#define LOG_SLOT_AVAILABLE	-100
#define LOG_SLOT_BUSY		-200
#define LOG_SLOT_READY		-300

#define SEND_BLOK_SZ		8192
#define MAX_CONN		8
#define MAX_LOG			256

#ifndef timersub
#define timersub(a, b, result) \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
	if ((result)->tv_usec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_usec += 1000000; \
	} \

#endif

int debug_mode = 0;

struct glob_var
{
/* global objects */
    sqlite3 *handle;
    sqlite3_stmt *stmt_log;
    char *cached_capabilities;
    struct server_log *log;
    struct wms_list *list;
    struct connections_pool *pool;
    void *cache;
} glob;

struct neutral_socket
{
#ifdef _WIN32
    SOCKET socket;
#else
    int socket;
#endif
};

struct wms_style
{
/* a struct wrapping a WMS style */
    int valid;
    char *name;
    char *title;
    char *abstract;
    struct wms_style *next;
};

struct wms_layer
{
/* a struct wrapping a WMS layer */
    int valid;
    char *layer_name;
    char *title;
    char *abstract;
    int srid;
    int has_flipped_axes;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double geo_minx;
    double geo_miny;
    double geo_maxx;
    double geo_maxy;
    unsigned char sample;
    unsigned char pixel;
    unsigned char num_bands;
    int jpeg;
    int png;
    int child_layer;
    struct wms_style *first_style;
    struct wms_style *last_style;
    struct wms_layer *next;
};

struct read_connection
{
    void *cache;
    sqlite3 *handle;
    sqlite3_stmt *stmt_get_map;
    int status;
};

struct connections_pool
{
    struct read_connection connections[MAX_CONN];
};

struct wms_layer_ref
{
/* a struct wrapping a WMS layer reference */
    struct wms_layer *layer_ref;
    struct wms_layer_ref *next;
};

struct wms_group
{
/* a struct wrapping a WMS group of layers */
    int valid;
    char *group_name;
    char *title;
    char *abstract;
    int srid;
    int has_flipped_axes;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double geo_minx;
    double geo_miny;
    double geo_maxx;
    double geo_maxy;
    struct wms_style *first_style;
    struct wms_style *last_style;
    struct wms_layer_ref *first_child;
    struct wms_layer_ref *last_child;
    struct wms_group *next;
};

struct wms_list
{
/* a struct wrapping a list of WMS layers */
    struct wms_layer *first_layer;
    struct wms_layer *last_layer;
    struct wms_group *first_group;
    struct wms_group *last_group;
};

struct wms_argument
{
/* a struct wrapping a single WMS arg */
    char *arg_name;
    char *arg_value;
    struct wms_argument *next;
};

struct wms_args
{
/* a struct wrapping a WMS request URL */
    sqlite3 *db_handle;
    sqlite3_stmt *stmt_get_map;
    char *service_name;
    struct wms_argument *first;
    struct wms_argument *last;
    int request_type;
    int wms_version;
    int error;
    const char *layer;
    int srid;
    int swap_xy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    unsigned short width;
    unsigned short height;
    char *style;
    unsigned char format;
    int transparent;
    int has_bgcolor;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

struct http_request
{
/* a struct wrapping an HTTP request */
    unsigned int id;		/* request ID */
    int port_no;
#ifdef _WIN32
    SOCKET socket;		/* Socket on which to receive data */
#else
    int socket;			/* Socket on which to receive data */
#endif
    struct wms_list *list;
    struct read_connection *conn;
    sqlite3 *log_handle;
    char *cached_capabilities;
    int cached_capabilities_len;
    struct server_log_item *log;
};

struct server_log_item
{
/* a struct supporting log infos */
    char *client_ip_addr;
    unsigned short client_ip_port;
    char *timestamp;
    char *http_method;
    char *request_url;
    int http_status;
    int response_length;
    int wms_request;
    int wms_version;
    const char *wms_layer;
    int wms_srid;
    double wms_bbox_minx;
    double wms_bbox_miny;
    double wms_bbox_maxx;
    double wms_bbox_maxy;
    unsigned short wms_width;
    unsigned short wms_height;
    char *wms_style;
    unsigned char wms_format;
    int wms_transparent;
    int has_bgcolor;
    unsigned char wms_bgcolor_red;
    unsigned char wms_bgcolor_green;
    unsigned char wms_bgcolor_blue;
    struct timeval begin_time;
    int milliseconds;
    int status;
};

struct server_log
{
/* log container */
    struct server_log_item items[MAX_LOG];
    int next_item;
    time_t last_update;
};

static struct wms_style *
alloc_wms_style (const char *name, const char *title, const char *abstract)
{
/* creating a WMS Raster Style */
    int len;
    struct wms_style *style = malloc (sizeof (struct wms_style));
    style->valid = 1;
    len = strlen (name);
    style->name = malloc (len + 1);
    strcpy (style->name, name);
    if (title == NULL)
	style->title = NULL;
    else
      {
	  len = strlen (title);
	  style->title = malloc (len + 1);
	  strcpy (style->title, title);
      }
    if (abstract == NULL)
	style->abstract = NULL;
    else
      {
	  len = strlen (abstract);
	  style->abstract = malloc (len + 1);
	  strcpy (style->abstract, abstract);
      }
    style->next = NULL;
    return style;
}

static void
destroy_wms_style (struct wms_style *style)
{
/* memory cleanup - destroying a Raster Style */
    if (style == NULL)
	return;
    if (style->name != NULL)
	free (style->name);
    if (style->title != NULL)
	free (style->title);
    if (style->abstract != NULL)
	free (style->abstract);
    free (style);
}

static struct wms_layer *
alloc_wms_layer (const char *layer, const char *title, const char *abstract,
		 int srid, int has_flipped_axes, double minx, double miny,
		 double maxx, double maxy, unsigned char sample,
		 unsigned char pixel, unsigned char num_bands)
{
/* creating a WMS layer item */
    int len;
    struct wms_layer *lyr = malloc (sizeof (struct wms_layer));
    lyr->valid = 1;
    len = strlen (layer);
    lyr->layer_name = malloc (len + 1);
    strcpy (lyr->layer_name, layer);
    len = strlen (title);
    lyr->title = malloc (len + 1);
    strcpy (lyr->title, title);
    len = strlen (abstract);
    lyr->abstract = malloc (len + 1);
    strcpy (lyr->abstract, abstract);
    lyr->srid = srid;
    lyr->has_flipped_axes = has_flipped_axes;
    lyr->minx = minx;
    lyr->miny = miny;
    lyr->maxx = maxx;
    lyr->maxy = maxy;
    lyr->sample = sample;
    lyr->pixel = pixel;
    lyr->num_bands = num_bands;
    if (pixel == RL2_PIXEL_MONOCHROME || pixel == RL2_PIXEL_PALETTE)
      {
	  lyr->png = 1;
	  lyr->jpeg = 0;
      }
    else
      {
	  lyr->png = 1;
	  lyr->jpeg = 1;
      }
    lyr->child_layer = 0;
    lyr->first_style = NULL;
    lyr->last_style = NULL;
    lyr->next = NULL;
    return lyr;
}

static void
destroy_wms_layer (struct wms_layer *lyr)
{
/* memory cleanup - freeing a WMS layer item */
    struct wms_style *style;
    struct wms_style *style_n;
    if (lyr == NULL)
	return;
    if (lyr->layer_name != NULL)
	free (lyr->layer_name);
    if (lyr->title != NULL)
	free (lyr->title);
    if (lyr->abstract != NULL)
	free (lyr->abstract);
    style = lyr->first_style;
    while (style != NULL)
      {
	  style_n = style->next;
	  destroy_wms_style (style);
	  style = style_n;
      }
    free (lyr);
}

static struct wms_layer_ref *
alloc_wms_layer_ref (struct wms_layer *layer_ref)
{
/* creating a WMS layer_ref item */
    struct wms_layer_ref *ref = malloc (sizeof (struct wms_layer_ref));
    ref->layer_ref = layer_ref;
    ref->next = NULL;
    return ref;
}

static void
destroy_wms_layer_ref (struct wms_layer_ref *ref)
{
/* memory cleanup - freeing a WMS layer_ref item */
    if (ref == NULL)
	return;
    free (ref);
}

static struct wms_group *
alloc_wms_group (const char *name, const char *title, const char *abstract)
{
/* creating a WMS group item */
    int len;
    struct wms_group *grp = malloc (sizeof (struct wms_group));
    grp->valid = 1;
    len = strlen (name);
    grp->group_name = malloc (len + 1);
    strcpy (grp->group_name, name);
    len = strlen (title);
    grp->title = malloc (len + 1);
    strcpy (grp->title, title);
    len = strlen (abstract);
    grp->abstract = malloc (len + 1);
    strcpy (grp->abstract, abstract);
    grp->first_style = NULL;
    grp->last_style = NULL;
    grp->first_child = NULL;
    grp->last_child = NULL;
    grp->next = NULL;
    return grp;
}

static void
destroy_wms_group (struct wms_group *grp)
{
/* memory cleanup - freeing a WMS group item */
    struct wms_style *style;
    struct wms_style *style_n;
    struct wms_layer_ref *child;
    struct wms_layer_ref *child_n;
    if (grp == NULL)
	return;
    if (grp->group_name != NULL)
	free (grp->group_name);
    if (grp->title != NULL)
	free (grp->title);
    if (grp->abstract != NULL)
	free (grp->abstract);
    style = grp->first_style;
    while (style != NULL)
      {
	  style_n = style->next;
	  destroy_wms_style (style);
	  style = style_n;
      }
    child = grp->first_child;
    while (child != NULL)
      {
	  child_n = child->next;
	  destroy_wms_layer_ref (child);
	  child = child_n;
      }
    free (grp);
}

static struct wms_list *
alloc_wms_list ()
{
/* allocating a list of WMS layers */
    struct wms_list *list = malloc (sizeof (struct wms_list));
    list->first_layer = NULL;
    list->last_layer = NULL;
    list->first_group = NULL;
    list->last_group = NULL;
    return list;
}

static void
destroy_wms_list (struct wms_list *list)
{
/* memory cleanup - destroying a list of WMS layers */
    struct wms_layer *pl;
    struct wms_layer *pln;
    struct wms_group *pg;
    struct wms_group *pgn;
    if (list == NULL)
	return;
    pl = list->first_layer;
    while (pl != NULL)
      {
	  pln = pl->next;
	  destroy_wms_layer (pl);
	  pl = pln;
      }
    pg = list->first_group;
    while (pg != NULL)
      {
	  pgn = pg->next;
	  destroy_wms_group (pg);
	  pg = pgn;
      }
    free (list);
}

static void
add_style_to_wms_layer (struct wms_list *list, const char *coverage_name,
			const char *name, const char *title,
			const char *abstract)
{
/* appending a Raster Style to a WMS Layer */
    struct wms_layer *lyr;
    if (list == NULL)
	return;
    lyr = list->first_layer;
    while (lyr != NULL)
      {
	  if (strcmp (lyr->layer_name, coverage_name) == 0)
	    {
		struct wms_style *style =
		    alloc_wms_style (name, title, abstract);
		if (lyr->first_style == NULL)
		    lyr->first_style = style;
		if (lyr->last_style != NULL)
		    lyr->last_style->next = style;
		lyr->last_style = style;
		return;
	    }
	  lyr = lyr->next;
      }
}

static void
add_default_styles (struct wms_list *list)
{
/* appending an implicit Default Style to each WMS Layer */
    struct wms_layer *lyr;
    if (list == NULL)
	return;
    lyr = list->first_layer;
    while (lyr != NULL)
      {
	  struct wms_style *style;
	  int has_default = 0;
	  int count = 0;
	  style = lyr->first_style;
	  while (style != NULL)
	    {
		if (strcasecmp (style->name, "default") == 0)
		    has_default = 1;
		count++;
		style = style->next;
	    }
	  if (count && !has_default)
	    {
		/* appending a Default style */
		struct wms_style *style =
		    alloc_wms_style ("default", NULL, NULL);
		if (lyr->first_style == NULL)
		    lyr->first_style = style;
		if (lyr->last_style != NULL)
		    lyr->last_style->next = style;
		lyr->last_style = style;
	    }
	  lyr = lyr->next;
      }
}

static void
add_style_to_wms_group (struct wms_list *list, const char *group_name,
			const char *name, const char *title,
			const char *abstract)
{
/* appending a Style to a WMS Group */
    struct wms_group *grp;
    if (list == NULL)
	return;
    grp = list->first_group;
    while (grp != NULL)
      {
	  if (strcmp (grp->group_name, group_name) == 0)
	    {
		struct wms_style *style =
		    alloc_wms_style (name, title, abstract);
		if (grp->first_style == NULL)
		    grp->first_style = style;
		if (grp->last_style != NULL)
		    grp->last_style->next = style;
		grp->last_style = style;
		return;
	    }
	  grp = grp->next;
      }
}

static void
add_default_group_styles (struct wms_list *list)
{
/* appending an implicit Default Style to each WMS Group */
    struct wms_group *grp;
    if (list == NULL)
	return;
    grp = list->first_group;
    while (grp != NULL)
      {
	  struct wms_style *style;
	  int has_default = 0;
	  int count = 0;
	  style = grp->first_style;
	  while (style != NULL)
	    {
		if (strcasecmp (style->name, "default") == 0)
		    has_default = 1;
		count++;
		style = style->next;
	    }
	  if (count && !has_default)
	    {
		/* appending a Default style */
		struct wms_style *style =
		    alloc_wms_style ("default", NULL, NULL);
		if (grp->first_style == NULL)
		    grp->first_style = style;
		if (grp->last_style != NULL)
		    grp->last_style->next = style;
		grp->last_style = style;
	    }
	  grp = grp->next;
      }
}

static struct wms_argument *
alloc_wms_argument (char *name, char *value)
{
/* allocating a WMS argument */
    struct wms_argument *arg;
    arg = malloc (sizeof (struct wms_argument));
    arg->arg_name = name;
    arg->arg_value = value;
    arg->next = NULL;
    return arg;
}

static void
destroy_wms_argument (struct wms_argument *arg)
{
/* memory cleanup - destroying a WMS arg struct */
    if (arg == NULL)
	return;
    if (arg->arg_name != NULL)
	free (arg->arg_name);
    if (arg->arg_value != NULL)
	free (arg->arg_value);
    free (arg);
}

static void
close_connection (struct read_connection *conn)
{
/* closing a connection */
    if (conn == NULL)
	return;
    if (conn->stmt_get_map != NULL)
	sqlite3_finalize (conn->stmt_get_map);
    if (conn->handle != NULL)
	sqlite3_close (conn->handle);
    if (conn->cache != NULL)
	spatialite_cleanup_ex (conn->cache);
    conn->status = CONNECTION_INVALID;
}

static void
destroy_connections_pool (struct connections_pool *pool)
{
/* memory clean-up: destroying a connections pool */
    int i;
    struct read_connection *conn;

    if (pool == NULL)
	return;
    for (i = 0; i < MAX_CONN; i++)
      {
	  /* closing all connections */
	  conn = &(pool->connections[i]);
	  close_connection (conn);
      }
    free (pool);
}

static void
connection_init (struct read_connection *conn, const char *path)
{
/* creating a read connection */
    int ret;
    sqlite3 *db_handle;
    sqlite3_stmt *stmt;
    void *cache;
    const char *sql;

    ret =
	sqlite3_open_v2 (path, &db_handle,
			 SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX |
			 SQLITE_OPEN_SHAREDCACHE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open '%s': %s\n", path,
		   sqlite3_errmsg (db_handle));
	  sqlite3_close (db_handle);
	  return;
      }
    cache = spatialite_alloc_connection ();
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, 0);

/* creating the GetMap SQL statement */
    sql =
	"SELECT RL2_GetMapImageFromRaster(?, BuildMbr(?, ?, ?, ?), ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  sqlite3_close (db_handle);
	  spatialite_cleanup_ex (cache);
      }
    conn->handle = db_handle;
    conn->stmt_get_map = stmt;
    conn->cache = cache;
    conn->status = CONNECTION_AVAILABLE;
}

static struct connections_pool *
alloc_connections_pool (const char *path)
{
/* creating and initializing the connections pool */
    int i;
    int count;
    struct read_connection *conn;
    struct connections_pool *pool =
	malloc (sizeof (struct read_connection) * MAX_CONN);
    if (pool == NULL)
	return NULL;
    for (i = 0; i < MAX_CONN; i++)
      {
	  /* initializing empty connections */
	  conn = &(pool->connections[i]);
	  conn->handle = NULL;
	  conn->stmt_get_map = NULL;
	  conn->cache = NULL;
	  conn->status = CONNECTION_INVALID;
      }
    for (i = 0; i < MAX_CONN; i++)
      {
	  /* creating the connections */
	  conn = &(pool->connections[i]);
	  connection_init (conn, path);
      }
    count = 0;
    for (i = 0; i < MAX_CONN; i++)
      {
	  /* final validity check */
	  conn = &(pool->connections[i]);
	  if (conn->status == CONNECTION_AVAILABLE)
	      count++;
      }
    if (count == 0)
      {
	  /* invalid pool ... sorry ... */
	  destroy_connections_pool (pool);
	  return NULL;
      }
    return pool;
}

static void
log_cleanup (struct server_log_item *log)
{
/* memory cleanup - resetting a log slot */
    if (log == NULL)
	return;
    if (log->client_ip_addr != NULL)
	free (log->client_ip_addr);
    if (log->timestamp != NULL)
	sqlite3_free (log->timestamp);
    if (log->http_method != NULL)
	free (log->http_method);
    if (log->request_url != NULL)
	free (log->request_url);
    if (log->wms_style != NULL)
	free (log->wms_style);
    log->client_ip_addr = NULL;
    log->timestamp = NULL;
    log->http_method = NULL;
    log->request_url = NULL;
    log->wms_layer = NULL;
    log->wms_style = NULL;
    log->status = LOG_SLOT_AVAILABLE;
}

static struct server_log *
alloc_server_log ()
{
/* allocating an empty server log helper struct */
    int i;
    struct server_log *log = malloc (sizeof (struct server_log));
    if (log == NULL)
	return NULL;
    for (i = 0; i < MAX_LOG; i++)
      {
	  struct server_log_item *item = &(log->items[i]);
	  item->client_ip_addr = NULL;
	  item->timestamp = NULL;
	  item->http_method = NULL;
	  item->request_url = NULL;
	  item->wms_layer = NULL;
	  item->wms_style = NULL;
	  item->status = LOG_SLOT_AVAILABLE;
      }
    log->next_item = 0;
    time (&(log->last_update));
    return log;
}

static void
destroy_server_log (struct server_log *log)
{
/* memory cleanup - destroying the server log helper struct */
    int i;
    if (log == NULL)
	return;
    for (i = 0; i < MAX_LOG; i++)
      {
	  struct server_log_item *item = &(log->items[i]);
	  log_cleanup (item);
      }
    free (log);
}

static void
log_error (struct server_log_item *log, char *timestamp, int status,
	   char *method, char *url, int size)
{
/* logging an ERROR event */
    struct timeval stop_time;
    struct timeval res;
    if (log == NULL)
	return;
    log->timestamp = timestamp;
    log->http_status = status;
    log->http_method = method;
    log->request_url = url;
    log->response_length = size;
    log->wms_request = WMS_ILLEGAL_REQUEST;
    log->wms_version = WMS_VERSION_UNKNOWN;
    log->status = LOG_SLOT_READY;
    gettimeofday (&stop_time, NULL);
    timersub (&(log->begin_time), &stop_time, &res);
    log->milliseconds = res.tv_usec / 1000;
}

static void
log_get_capabilities_1 (struct server_log_item *log, char *timestamp,
			int status, char *method, char *url)
{
/* logging a GetCapabilities event (take #1) */
    if (log == NULL)
	return;
    log->timestamp = timestamp;
    log->http_status = status;
    log->http_method = method;
    log->request_url = url;
    log->wms_request = WMS_GET_CAPABILITIES;
    log->wms_version = WMS_VERSION_UNKNOWN;
}

static void
log_get_capabilities_2 (struct server_log_item *log, int size)
{
/* logging a GetCapabilities event (take #2) */
    struct timeval stop_time;
    struct timeval res;
    if (log == NULL)
	return;
    log->response_length = size;
    log->status = LOG_SLOT_READY;
    gettimeofday (&stop_time, NULL);
    timersub (&(log->begin_time), &stop_time, &res);
    log->milliseconds = res.tv_usec / 1000;
}

static void
log_get_map_1 (struct server_log_item *log, char *timestamp, int status,
	       char *method, char *url, struct wms_args *args)
{
/* logging a GetMap event (take #1) */
    int len;
    if (log == NULL)
	return;
    log->timestamp = timestamp;
    log->http_status = status;
    log->http_method = method;
    log->request_url = url;
    log->wms_request = WMS_GET_MAP;
    log->wms_version = args->wms_version;
    log->wms_layer = args->layer;
    log->wms_srid = args->srid;
    log->wms_bbox_minx = args->minx;
    log->wms_bbox_miny = args->miny;
    log->wms_bbox_maxx = args->maxx;
    log->wms_bbox_maxy = args->maxy;
    log->wms_width = args->width;
    log->wms_height = args->height;
    if (args->style == NULL)
	log->wms_style = NULL;
    else
      {
	  len = strlen (args->style);
	  log->wms_style = malloc (len + 1);
	  strcpy (log->wms_style, args->style);
      }
    log->wms_format = args->format;
    log->wms_transparent = args->transparent;
    log->has_bgcolor = args->has_bgcolor;
    log->wms_bgcolor_red = args->red;
    log->wms_bgcolor_green = args->green;
    log->wms_bgcolor_blue = args->blue;
}

static void
log_get_map_2 (struct server_log_item *log, int size)
{
/* logging a GetMap event (take #2) */
    struct timeval stop_time;
    struct timeval res;
    if (log == NULL)
	return;
    log->response_length = size;
    log->status = LOG_SLOT_READY;
    gettimeofday (&stop_time, NULL);
    timersub (&(log->begin_time), &stop_time, &res);
    log->milliseconds = res.tv_usec / 1000;
}

static void
flush_log (sqlite3 * handle, sqlite3_stmt * stmt, struct server_log *log)
{
/* flushing the LOG */
    int ret;
    int i;
    char dummy[32];
    if (handle == NULL || stmt == NULL || log == NULL)
	return;

    while (1)
      {
	  /* looping until all Log slots are ready */
	  int wait = 0;
	  for (i = 0; i < MAX_LOG; i++)
	    {
		struct server_log_item *item = &(log->items[i]);
		if (item->status == LOG_SLOT_BUSY)
		    wait = 1;
	    }
	  if (wait)
	      usleep (50);
	  else
	      break;
      }

/* starting a DBMS Transaction */
    ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, NULL);
    if (ret != SQLITE_OK)
	goto error;

    for (i = 0; i < MAX_LOG; i++)
      {
	  struct server_log_item *item = &(log->items[i]);
	  if (item->status != LOG_SLOT_READY)
	      continue;
	  /* binding the INSERT values */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (item->timestamp == NULL)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_text (stmt, 1, item->timestamp,
				 strlen (item->timestamp), SQLITE_STATIC);
	  if (item->client_ip_addr == NULL)
	      sqlite3_bind_null (stmt, 2);
	  else
	      sqlite3_bind_text (stmt, 2, item->client_ip_addr,
				 strlen (item->client_ip_addr), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, item->client_ip_port);
	  if (item->http_method == NULL)
	      sqlite3_bind_null (stmt, 4);
	  else
	      sqlite3_bind_text (stmt, 4, item->http_method,
				 strlen (item->http_method), SQLITE_STATIC);
	  if (item->request_url == NULL)
	      sqlite3_bind_null (stmt, 5);
	  else
	      sqlite3_bind_text (stmt, 5, item->request_url,
				 strlen (item->request_url), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 6, item->http_status);
	  sqlite3_bind_int (stmt, 7, item->response_length);
	  switch (item->wms_request)
	    {
	    case WMS_GET_CAPABILITIES:
		sqlite3_bind_text (stmt, 8, "GetCapabilities", 17,
				   SQLITE_STATIC);
		break;
	    case WMS_GET_MAP:
		sqlite3_bind_text (stmt, 8, "GetMap", 8, SQLITE_STATIC);
		break;
	    default:
		sqlite3_bind_null (stmt, 8);
	    };
	  switch (item->wms_version)
	    {
	    case WMS_VERSION_100:
		sqlite3_bind_text (stmt, 9, "1.0.0", 5, SQLITE_TRANSIENT);
		break;
	    case WMS_VERSION_110:
		sqlite3_bind_text (stmt, 9, "1.1.0", 5, SQLITE_TRANSIENT);
		break;
	    case WMS_VERSION_111:
		sqlite3_bind_text (stmt, 9, "1.1.1", 5, SQLITE_TRANSIENT);
		break;
	    case WMS_VERSION_130:
		sqlite3_bind_text (stmt, 9, "1.3.0", 5, SQLITE_TRANSIENT);
		break;
	    default:
		sqlite3_bind_null (stmt, 9);
	    };
	  if (item->wms_request == WMS_GET_MAP)
	    {
		if (item->wms_layer == NULL)
		    sqlite3_bind_null (stmt, 10);
		else
		    sqlite3_bind_text (stmt, 10, item->wms_layer,
				       strlen (item->wms_layer), SQLITE_STATIC);
		sqlite3_bind_int (stmt, 11, item->wms_srid);
		sqlite3_bind_double (stmt, 12, item->wms_bbox_minx);
		sqlite3_bind_double (stmt, 13, item->wms_bbox_miny);
		sqlite3_bind_double (stmt, 14, item->wms_bbox_maxx);
		sqlite3_bind_double (stmt, 15, item->wms_bbox_maxy);
		sqlite3_bind_int (stmt, 16, item->wms_width);
		sqlite3_bind_int (stmt, 17, item->wms_height);
		if (item->wms_style == NULL)
		    sqlite3_bind_null (stmt, 18);
		else
		    sqlite3_bind_text (stmt, 18, item->wms_style,
				       strlen (item->wms_style), SQLITE_STATIC);
		switch (item->wms_format)
		  {
		  case RL2_OUTPUT_FORMAT_JPEG:
		      sqlite3_bind_text (stmt, 19, "image/jpeg", 10,
					 SQLITE_TRANSIENT);
		      break;
		  case RL2_OUTPUT_FORMAT_PNG:
		      sqlite3_bind_text (stmt, 19, "image/png", 9,
					 SQLITE_TRANSIENT);
		      break;
		  case RL2_OUTPUT_FORMAT_TIFF:
		      sqlite3_bind_text (stmt, 19, "image/tiff", 10,
					 SQLITE_TRANSIENT);
		      break;
		  case RL2_OUTPUT_FORMAT_PDF:
		      sqlite3_bind_text (stmt, 19, "application/x-pdf", 9,
					 SQLITE_TRANSIENT);
		      break;
		  default:
		      sqlite3_bind_null (stmt, 19);
		  };
		switch (item->wms_transparent)
		  {
		  case WMS_TRANSPARENT:
		      sqlite3_bind_int (stmt, 20, 1);
		      break;
		  case WMS_OPAQUE:
		      sqlite3_bind_int (stmt, 20, 0);
		      break;
		  default:
		      sqlite3_bind_null (stmt, 20);
		      break;
		  };
		if (item->has_bgcolor == 0)
		    sqlite3_bind_null (stmt, 21);
		else
		  {
		      sprintf (dummy, "#%02x%02x%02x\n", item->wms_bgcolor_red,
			       item->wms_bgcolor_green, item->wms_bgcolor_blue);
		      sqlite3_bind_text (stmt, 21, dummy, strlen (dummy),
					 SQLITE_TRANSIENT);
		  }
	    }
	  else
	    {
		sqlite3_bind_null (stmt, 10);
		sqlite3_bind_null (stmt, 11);
		sqlite3_bind_null (stmt, 12);
		sqlite3_bind_null (stmt, 13);
		sqlite3_bind_null (stmt, 14);
		sqlite3_bind_null (stmt, 15);
		sqlite3_bind_null (stmt, 16);
		sqlite3_bind_null (stmt, 17);
		sqlite3_bind_null (stmt, 18);
		sqlite3_bind_null (stmt, 19);
		sqlite3_bind_null (stmt, 20);
		sqlite3_bind_null (stmt, 21);
	    }
	  sqlite3_bind_int (stmt, 22, item->milliseconds);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		fprintf (stderr,
			 "INSERT INTO WMS-LOG; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
	  log_cleanup (item);
      }

/* committing the still pending transaction */
    ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, NULL);
    if (ret != SQLITE_OK)
	goto error;
    log->next_item = 0;
    time (&(log->last_update));
    return;

  error:
    sqlite3_exec (handle, "ROLLBACK", NULL, NULL, NULL);
    fprintf (stderr, "ERROR: unable to update the server log\n");
}

static void
clean_shutdown ()
{
/* performing a clean shutdown */
    fprintf (stderr, "wmslite server shutdown in progress\n");
    flush_log (glob.handle, glob.stmt_log, glob.log);
    if (glob.stmt_log != NULL)
	sqlite3_finalize (glob.stmt_log);
    if (glob.cached_capabilities != NULL)
	free (glob.cached_capabilities);
    if (glob.list != NULL)
	destroy_wms_list (glob.list);
    if (glob.log != NULL)
	destroy_server_log (glob.log);
    if (glob.pool != NULL)
	destroy_connections_pool (glob.pool);
    if (glob.handle != NULL)
	sqlite3_close (glob.handle);
    if (glob.cache != NULL)
	spatialite_cleanup_ex (glob.cache);
    spatialite_shutdown ();
    fprintf (stderr, "wmslite shutdown completed ... bye bye\n\n");
}

#ifdef _WIN32
BOOL WINAPI
signal_handler (DWORD dwCtrlType)
{
/* intercepting some Windows signal */
    clean_shutdown ();
    return FALSE;
}
#else
static void
signal_handler (int signo)
{
/* intercepting some signal */
    if (signo == SIGINT)
	signo = SIGINT;		/* suppressing compiler warnings */
    clean_shutdown ();
    exit (0);
}
#endif

static struct wms_args *
alloc_wms_args (const char *service_name)
{
/* allocating an empty WMS args struct */
    int len;
    struct wms_args *args;
    if (service_name == NULL)
	return NULL;
    args = malloc (sizeof (struct wms_args));
    args->db_handle = NULL;
    args->stmt_get_map = NULL;
    len = strlen (service_name);
    args->service_name = malloc (len + 1);
    strcpy (args->service_name, service_name);
    args->first = NULL;
    args->last = NULL;
    args->style = NULL;
    args->request_type = WMS_ILLEGAL_REQUEST;
    args->error = WMS_UNKNOWN;
    return args;
}

static void
destroy_wms_args (struct wms_args *args)
{
/* memory cleanup - destroying a WMS args struct */
    struct wms_argument *pa;
    struct wms_argument *pan;
    if (args == NULL)
	return;
    if (args->service_name != NULL)
	free (args->service_name);
    if (args->style != NULL)
	free (args->style);
    pa = args->first;
    while (pa != NULL)
      {
	  pan = pa->next;
	  destroy_wms_argument (pa);
	  pa = pan;
      }
    free (args);
}

static int
add_wms_argument (struct wms_args *args, const char *token)
{
/* attempting to add a WMS argument */
    int len;
    struct wms_argument *arg;
    char *name;
    char *value;
    const char *ptr = strstr (token, "=");
    if (ptr == NULL)
	return 0;
    len = strlen (ptr + 1);
    value = malloc (len + 1);
    strcpy (value, ptr + 1);
    len = ptr - token;
    name = malloc (len + 1);
    memcpy (name, token, len);
    *(name + len) = '\0';
    arg = alloc_wms_argument (name, value);
    if (args->first == NULL)
	args->first = arg;
    if (args->last != NULL)
	args->last->next = arg;
    args->last = arg;
    return 1;
}

static int
parse_srs (const char *srs)
{
/* parsing the EPSG:x item */
    int srid;
    if (strlen (srs) < 6)
	return -1;
    if (strncmp (srs, "EPSG:", 4) != 0)
	return -1;
    srid = atoi (srs + 5);
    return srid;
}

static const char *
parse_layers (const char *layers)
{
/* only a single layer for each request is supported */
    int comma = 0;
    const char *p = layers;
    while (*p != '\0')
      {
	  if (*p++ == ',')
	      comma++;
      }
    if (!comma)
	return layers;
    return NULL;
}

static int
parse_dim (const char *dim, unsigned short *value)
{
/* parsing the Width / Height items */
    int x;
    for (x = 0; x < (int) strlen (dim); x++)
      {
	  if (*(dim + x) < '0' || *(dim + x) > '9')
	      return 0;
      }
    x = atoi (dim);
    if (x <= 0 || x > UINT16_MAX)
	return 0;
    *value = x;
    return 1;
}

static char *
parse_style (const char *style)
{
/* parsing the current style */
    char *out_style = NULL;
    if (strlen (style) == 0)
	style = "default";
    out_style = malloc (strlen (style) + 1);
    strcpy (out_style, style);
    return out_style;
}

static int
parse_format (const char *format)
{
/* parsing the output format */
    if (strcasecmp (format, "image/png") == 0)
	return RL2_OUTPUT_FORMAT_PNG;
    if (strcasecmp (format, "image/jpeg") == 0)
	return RL2_OUTPUT_FORMAT_JPEG;
    if (strcasecmp (format, "image/tiff") == 0)
	return RL2_OUTPUT_FORMAT_TIFF;
    if (strcasecmp (format, "application/x-pdf") == 0)
	return RL2_OUTPUT_FORMAT_PDF;
    return WMS_UNKNOWN;
}

static int
parse_transparent (const char *str)
{
/* parsing the Transparent value */
    if (strcasecmp (str, "TRUE") == 0)
	return WMS_TRANSPARENT;
    if (strcasecmp (str, "FALSE") == 0)
	return WMS_OPAQUE;
    return WMS_UNKNOWN;
}

static int
parse_bbox (const char *bbox, double *minx, double *miny, double *maxx,
	    double *maxy)
{
/* attempting to parse the BBOX */
    char buf[2000];
    int count = 0;
    char *out = buf;
    const char *in = bbox;
    while (1)
      {
	  if (*in == '\0')
	    {
		if (count == 3)
		  {
		      *maxy = atof (buf);
		  }
		else
		    return 0;
		break;
	    }
	  if (*in == ',')
	    {
		*out = '\0';
		switch (count)
		  {
		  case 0:
		      *minx = atof (buf);
		      break;
		  case 1:
		      *miny = atof (buf);
		      break;
		  case 2:
		      *maxx = atof (buf);
		      break;
		  default:
		      return 0;
		  };
		out = buf;
		in++;
		count++;
		continue;
	    }
	  *out++ = *in++;
      }
    return 1;
}

static int
parse_hex (char hi, char lo, unsigned char *value)
{
/* parsing an Hex byte */
    unsigned char x;
    switch (hi)
      {
      case '0':
	  x = 0;
	  break;
      case '1':
	  x = 1 * 16;
	  break;
      case '2':
	  x = 2 * 16;
	  break;
      case '3':
	  x = 3 * 16;
	  break;
      case '4':
	  x = 4 * 16;
	  break;
      case '5':
	  x = 5 * 16;
	  break;
      case '6':
	  x = 6 * 16;
	  break;
      case '7':
	  x = 7 * 16;
	  break;
      case '8':
	  x = 8 * 16;
	  break;
      case '9':
	  x = 9 * 16;
	  break;
      case 'a':
      case 'A':
	  x = 10 * 16;
	  break;
      case 'b':
      case 'B':
	  x = 11 * 16;
	  break;
      case 'c':
      case 'C':
	  x = 12 * 16;
	  break;
      case 'd':
      case 'D':
	  x = 13 * 16;
	  break;
      case 'e':
      case 'E':
	  x = 14 * 16;
	  break;
      case 'f':
      case 'F':
	  x = 15 * 16;
	  break;
      default:
	  return 0;
      };
    switch (lo)
      {
      case '0':
	  x += 0;
	  break;
      case '1':
	  x += 1;
	  break;
      case '2':
	  x += 2;
	  break;
      case '3':
	  x += 3;
	  break;
      case '4':
	  x += 4;
	  break;
      case '5':
	  x += 5;
	  break;
      case '6':
	  x += 6;
	  break;
      case '7':
	  x += 7;
	  break;
      case '8':
	  x += 8;
	  break;
      case '9':
	  x += 9;
	  break;
      case 'a':
      case 'A':
	  x += 10;
	  break;
      case 'b':
      case 'B':
	  x += 11;
	  break;
      case 'c':
      case 'C':
	  x += 12;
	  break;
      case 'd':
      case 'D':
	  x += 13;
	  break;
      case 'e':
      case 'E':
	  x += 14;
	  break;
      case 'f':
      case 'F':
	  x += 15;
	  break;
      default:
	  return 0;
      };
    *value = x;
    return 1;
}

static int
parse_bgcolor (const char *bgcolor, unsigned char *red, unsigned char *green,
	       unsigned char *blue)
{
/* attempting to parse an RGB color */
    if (strlen (bgcolor) != 8)
	return 0;
    if (bgcolor[0] != '0')
	return 0;
    if (bgcolor[1] == 'x' || bgcolor[1] == 'X')
	;
    else
	return 0;
    if (!parse_hex (bgcolor[2], bgcolor[3], red))
	return 0;
    if (!parse_hex (bgcolor[4], bgcolor[5], green))
	return 0;
    if (!parse_hex (bgcolor[6], bgcolor[7], blue))
	return 0;
    return 1;
}

static int
exists_layer (struct wms_list *list, const char *layer, int srid,
	      int wms_version, int *swap_xy, double minx, double miny,
	      double maxx, double maxy, const char **layer_name)
{
/* checking a required layer for validity */
    struct wms_group *grp;
    struct wms_layer *lyr = list->first_layer;
    while (lyr != NULL)
      {
	  /* searching a genuine Layer */
	  if (strcmp (lyr->layer_name, layer) == 0)
	    {
		if (lyr->srid != srid)
		    return WMS_MISMATCHING_SRID;
		if (wms_version == WMS_VERSION_130 && lyr->has_flipped_axes)
		    *swap_xy = 1;
		else
		    *swap_xy = 0;
		if (*swap_xy)
		  {
		      if (lyr->minx > maxy)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (lyr->maxx < miny)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (lyr->miny > maxx)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (lyr->maxy < minx)
			  return WMS_LAYER_OUT_OF_BBOX;
		  }
		else
		  {
		      if (lyr->minx > maxx)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (lyr->maxx < minx)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (lyr->miny > maxy)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (lyr->maxy < miny)
			  return WMS_LAYER_OUT_OF_BBOX;
		  }
		*layer_name = lyr->layer_name;
		return 0;
	    }
	  lyr = lyr->next;
      }
    grp = list->first_group;
    while (grp != NULL)
      {
	  /* fallback case: searching a Group of Layers */
	  if (strcmp (grp->group_name, layer) == 0)
	    {
		if (grp->valid == 0)
		    return WMS_INVALID_GROUP;
		if (grp->srid != srid)
		    return WMS_MISMATCHING_SRID;
		if (wms_version == WMS_VERSION_130 && grp->has_flipped_axes)
		    *swap_xy = 1;
		else
		    *swap_xy = 0;
		if (*swap_xy)
		  {
		      if (grp->minx > maxy)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (grp->maxx < miny)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (grp->miny > maxx)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (grp->maxy < minx)
			  return WMS_LAYER_OUT_OF_BBOX;
		  }
		else
		  {
		      if (grp->minx > maxx)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (grp->maxx < minx)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (grp->miny > maxy)
			  return WMS_LAYER_OUT_OF_BBOX;
		      if (grp->maxy < miny)
			  return WMS_LAYER_OUT_OF_BBOX;
		  }
		*layer_name = grp->group_name;
		return 0;
	    }
	  grp = grp->next;
      }
    return WMS_NOT_EXISTING_LAYER;
}

static int
check_wms_request (struct wms_list *list, struct wms_args *args)
{
/* checking for a valid WMS request */
    struct wms_argument *arg;
    int wms_version = WMS_VERSION_130;
    int ok_wms = 0;
    int ok_version = 0;
    int is_get_capabilities = 0;
    int is_get_map = 0;
    const char *p_layers = NULL;
    const char *p_srs = NULL;
    const char *p_bbox = NULL;
    const char *p_width = NULL;
    const char *p_height = NULL;
    const char *p_styles = NULL;
    const char *p_format = NULL;
    const char *p_bgcolor = NULL;
    const char *p_transparent = NULL;

    if (strcasecmp (args->service_name, "GET /wmslite") != 0)
	return 400;
    arg = args->first;
    while (arg != NULL)
      {
	  if (strcasecmp (arg->arg_name, "SERVICE") == 0)
	    {
		if (strcasecmp (arg->arg_value, "WMS") == 0)
		    ok_wms = 1;
	    }
	  if (strcasecmp (arg->arg_name, "VERSION") == 0)
	    {
		wms_version = WMS_VERSION_UNKNOWN;
		if (strcasecmp (arg->arg_value, "1.3.0") == 0)
		  {
		      wms_version = WMS_VERSION_130;
		      ok_version = 1;
		  }
		if (strcasecmp (arg->arg_value, "1.1.1") == 0)
		  {
		      wms_version = WMS_VERSION_111;
		      ok_version = 1;
		  }
		if (strcasecmp (arg->arg_value, "1.1.0") == 0)
		  {
		      wms_version = WMS_VERSION_110;
		      ok_version = 1;
		  }
		if (strcasecmp (arg->arg_value, "1.0.0") == 0)
		  {
		      wms_version = WMS_VERSION_100;
		      ok_version = 1;
		  }
	    }
	  if (strcasecmp (arg->arg_name, "REQUEST") == 0)
	    {
		if (strcasecmp (arg->arg_value, "GetCapabilities") == 0)
		    is_get_capabilities = 1;
		if (strcasecmp (arg->arg_value, "GetMap") == 0)
		    is_get_map = 1;
	    }
	  if (strcasecmp (arg->arg_name, "LAYERS") == 0)
	      p_layers = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "SRS") == 0
	      || strcasecmp (arg->arg_name, "CRS") == 0)
	      p_srs = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "BBOX") == 0)
	      p_bbox = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "WIDTH") == 0)
	      p_width = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "HEIGHT") == 0)
	      p_height = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "STYLES") == 0)
	      p_styles = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "FORMAT") == 0)
	      p_format = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "BGCOLOR") == 0)
	      p_bgcolor = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "TRANSPARENT") == 0)
	      p_transparent = arg->arg_value;
	  arg = arg->next;
      }
    if (is_get_capabilities && !is_get_map
	&& wms_version != WMS_VERSION_UNKNOWN)
      {
	  /* testing for valid GetCapabilities */
	  if (ok_wms)
	    {
		args->request_type = WMS_GET_CAPABILITIES;
		args->wms_version = wms_version;
		return 200;
	    }
      }
    if (is_get_map && !is_get_capabilities
	&& wms_version != WMS_VERSION_UNKNOWN)
      {
	  /* testing for valid GetMap */
	  if (ok_wms && ok_version && p_layers && p_srs && p_bbox && p_width
	      && p_height && p_styles && p_format)
	    {
		int ret;
		int srid;
		const char *layer;
		const char *layer_name;
		int swap_xy = 0;
		double minx;
		double miny;
		double maxx;
		double maxy;
		unsigned short width;
		unsigned short height;
		char *style = NULL;
		int format;
		int transparent = WMS_OPAQUE;
		unsigned char red = 255;
		unsigned char green = 255;
		unsigned char blue = 255;
		srid = parse_srs (p_srs);
		if (srid < -1)
		  {
		      args->error = WMS_INVALID_CRS;
		      return 200;
		  }
		layer = parse_layers (p_layers);
		if (layer == NULL)
		  {
		      args->error = WMS_INVALID_LAYER;
		      return 200;
		  }
		if (!parse_bbox (p_bbox, &minx, &miny, &maxx, &maxy))
		  {
		      args->error = WMS_INVALID_BBOX;
		      return 200;
		  }
		if (!parse_dim (p_width, &width))
		  {
		      args->error = WMS_INVALID_DIMENSION;
		      return 200;
		  }
		if (!parse_dim (p_height, &height))
		  {
		      args->error = WMS_INVALID_DIMENSION;
		      return 200;
		  }
		style = parse_style (p_styles);
		if (style == NULL)
		  {
		      args->error = WMS_INVALID_STYLE;
		      return 200;
		  }
		args->style = style;
		format = parse_format (p_format);
		if (format == WMS_UNKNOWN)
		  {
		      args->error = WMS_INVALID_FORMAT;
		      return 200;
		  }
		if (p_transparent != NULL)
		  {
		      transparent = parse_transparent (p_transparent);
		      if (transparent == WMS_UNKNOWN)
			{
			    args->error = WMS_INVALID_TRANSPARENT;
			    return 200;
			}
		  }
		args->has_bgcolor = 0;
		if (p_bgcolor != NULL)
		  {
		      if (!parse_bgcolor (p_bgcolor, &red, &green, &blue))
			{
			    args->error = WMS_INVALID_BGCOLOR;
			    return 200;
			}
		      args->has_bgcolor = 1;
		  }
		ret =
		    exists_layer (list, layer, srid, wms_version, &swap_xy,
				  minx, miny, maxx, maxy, &layer_name);
		if (ret == WMS_NOT_EXISTING_LAYER || ret == WMS_INVALID_GROUP
		    || ret == WMS_LAYER_OUT_OF_BBOX
		    || ret == WMS_MISMATCHING_SRID)
		  {
		      args->error = ret;
		      return 200;
		  }
		args->request_type = WMS_GET_MAP;
		args->wms_version = wms_version;
		args->layer = layer_name;
		args->srid = srid;
		args->swap_xy = swap_xy;
		if (wms_version == WMS_VERSION_130 && swap_xy)
		  {
		      /* swapping X and Y axis */
		      args->minx = miny;
		      args->miny = minx;
		      args->maxx = maxy;
		      args->maxy = maxx;
		  }
		else
		  {
		      /* normal XY axis ordering */
		      args->minx = minx;
		      args->miny = miny;
		      args->maxx = maxx;
		      args->maxy = maxy;
		  }
		args->width = width;
		args->height = height;
		args->format = format;
		args->transparent = transparent;
		args->red = red;
		args->green = green;
		args->blue = blue;
	    }
      }
    return 200;
}

static struct wms_args *
parse_http_request (const char *http_hdr, char **method, char **url)
{
/* attempting to parse an HTTP Request */
    int ok = 1;
    int len;
    struct wms_args *args = NULL;
    char token[2000];
    char *out;
    const char *p;
    const char *start = strstr (http_hdr, "GET ");
    const char *end = NULL;
    *url = NULL;
    if (start == NULL)
	ok = 0;
    if (ok)
      {
	  end = strstr (start, " HTTP/1.1");
	  if (end == NULL)
	      end = strstr (start, " HTTP/1.0");
	  if (end == NULL)
	      return NULL;
      }

    if (ok)
      {
	  *method = malloc (4);
	  strcpy (*method, "GET");
	  len = end - start;
	  len -= 4;
	  len++;
	  *url = malloc (end - start);
	  memcpy (*url, start + 4, len);
	  *(*url + len) = '\0';
      }
    else
      {
	  start = strstr (http_hdr, "POST ");
	  if (start != NULL)
	    {
		end = strstr (start, " HTTP/1.1");
		if (end == NULL)
		    end = strstr (start, " HTTP/1.0");
		if (end == NULL)
		    return NULL;
	    }
	  *method = malloc (5);
	  strcpy (*method, "POST");
	  len = end - start;
	  len -= 5;
	  len++;
	  *url = malloc (end - start);
	  memcpy (*url, start + 5, len);
	  *(*url + len) = '\0';
      }

    p = start;
    out = token;
    while (p < end)
      {
	  if (*p == '?')
	    {
		/* the service name */
		*out = '\0';
		if (args != NULL)
		    goto error;
		args = alloc_wms_args (token);
		out = token;
		p++;
		continue;
	    }
	  if (*p == '&')
	    {
		/* a key-value pair ends here */
		*out = '\0';
		if (args == NULL)
		    goto error;
		if (!add_wms_argument (args, token))
		    goto error;
		out = token;
		p++;
		continue;
	    }
	  *out++ = *p++;
      }
    if (out > token)
      {
	  /* processing the last arg */
	  *out = '\0';
	  if (args == NULL)
	      goto error;
	  if (!add_wms_argument (args, token))
	      goto error;
      }
    if (debug_mode)
	printf ("%s\n", *url);
    return args;

  error:
    if (args != NULL)
	destroy_wms_args (args);
    return NULL;
}

static char *
get_current_timestamp ()
{
/* formatting the current timestamp */
    char *dummy;
    struct tm *xtm;
    time_t now;
    const char *day = NULL;
    const char *month = NULL;
    time (&now);
    xtm = gmtime (&now);
    switch (xtm->tm_wday)
      {
      case 0:
	  day = "Sun";
	  break;
      case 1:
	  day = "Mon";
	  break;
      case 2:
	  day = "Tue";
	  break;
      case 3:
	  day = "Wed";
	  break;
      case 4:
	  day = "Thu";
	  break;
      case 5:
	  day = "Fri";
	  break;
      case 6:
	  day = "Sat";
	  break;
      };
    switch (xtm->tm_mon)
      {
      case 0:
	  month = "Jan";
	  break;
      case 1:
	  month = "Feb";
	  break;
      case 2:
	  month = "Mar";
	  break;
      case 3:
	  month = "Apr";
	  break;
      case 4:
	  month = "May";
	  break;
      case 5:
	  month = "Jun";
	  break;
      case 6:
	  month = "Jul";
	  break;
      case 7:
	  month = "Aug";
	  break;
      case 8:
	  month = "Sep";
	  break;
      case 9:
	  month = "Oct";
	  break;
      case 10:
	  month = "Nov";
	  break;
      case 11:
	  month = "Dec";
	  break;
      };
    dummy =
	sqlite3_mprintf ("Date: %s, %02d %s %04d %02d:%02d:%02d GMT\r\n", day,
			 xtm->tm_mday, month, xtm->tm_year + 1900, xtm->tm_hour,
			 xtm->tm_min, xtm->tm_sec);
    return dummy;
}

static char *
get_sql_timestamp ()
{
/* formatting the current SQL timestamp */
    char *dummy;
    struct tm *xtm;
    time_t now;
    time (&now);
    xtm = gmtime (&now);
    dummy =
	sqlite3_mprintf ("%04d-%02d-%02dT%02d:%02d:%02d", xtm->tm_year + 1900,
			 xtm->tm_mon + 1, xtm->tm_mday, xtm->tm_hour,
			 xtm->tm_min, xtm->tm_sec);
    return dummy;
}

static void
build_wms_exception (struct wms_args *args, gaiaOutBufferPtr xml_response)
{
/* preparing an XML exception */
    char *dummy;
    gaiaOutBuffer xml_text;
    gaiaOutBufferInitialize (&xml_text);
    gaiaAppendToOutBuffer (&xml_text,
			   "<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ServiceExceptionReport version=\"1.3.0\" ");
    gaiaAppendToOutBuffer (&xml_text, "xmlns=\"http://www.opengis.net/ogc\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xsi:schemaLocation=\"http://www.opengis.net/ogc ");
    gaiaAppendToOutBuffer (&xml_text,
			   "http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">\r\n");
    if (args == NULL)
      {
	  gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
	  gaiaAppendToOutBuffer (&xml_text, "General error.\r\n");
      }
    else
      {
	  switch (args->error)
	    {
	    case WMS_INVALID_DIMENSION:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"InvalidDimensionValue\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request is for a Layer not offered by the service instance.\r\n");
		break;
	    case WMS_INVALID_BBOX:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Invalid BBOX parameter.\r\n");
		break;
	    case WMS_NOT_EXISTING_LAYER:
	    case WMS_INVALID_GROUP:
	    case WMS_INVALID_LAYER:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"LayerNotDefined\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request is for a Layer not offered by the service instance.\r\n");
		break;
	    case WMS_INVALID_BGCOLOR:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Invalid BGCOLOR parameter.\r\n");
		break;
	    case WMS_INVALID_STYLE:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"StyleNotDefined\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request is for a Layer in a Style not offered by the service instance.\r\n");
		break;
	    case WMS_INVALID_FORMAT:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"InvalidFormat\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request contains a Format not offered by the service instance.\r\n");
		break;
	    case WMS_INVALID_TRANSPARENT:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Invalid TRANSPARENT parameter.\r\n");
		break;
	    case WMS_LAYER_OUT_OF_BBOX:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "The BBOX parameter is outside the layer's extent.\r\n");
		break;
	    case WMS_INVALID_CRS:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"InvalidSRS\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request contains a malformed SRS.\r\n");
		break;
	    case WMS_MISMATCHING_SRID:
		gaiaAppendToOutBuffer (&xml_text,
				       "<ServiceException code=\"InvalidSRS\">\r\n");
		gaiaAppendToOutBuffer (&xml_text,
				       "Request contains an SRS not offered by the service ");
		gaiaAppendToOutBuffer (&xml_text,
				       "instance for one or more of the Layers in the request.\r\n");
		break;
	    default:
		gaiaAppendToOutBuffer (&xml_text, "<ServiceException>\r\n");
		gaiaAppendToOutBuffer (&xml_text, "Malformed request.\r\n");
		break;
	    }
      }
    gaiaAppendToOutBuffer (&xml_text,
			   "</ServiceException>\r\n</ServiceExceptionReport>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "");
    gaiaAppendToOutBuffer (xml_response, "HTTP/1.1 200 OK\r\n");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response,
			   "Content-Type: text/xml;charset=UTF-8\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", xml_text.WriteOffset);
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response, "Connection: close\r\n\r\n");
    gaiaAppendToOutBuffer (xml_response, xml_text.Buffer);
    gaiaOutBufferReset (&xml_text);
}

static void
build_http_error (int http_status, gaiaOutBufferPtr xml_response, int port_no)
{
/* preparing an HTTP error */
    char *dummy;
    gaiaOutBuffer http_text;
    gaiaOutBufferInitialize (&http_text);
    gaiaAppendToOutBuffer (&http_text,
			   "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n");
    gaiaAppendToOutBuffer (&http_text, "<html><head>\r\n");
    if (http_status == 400)
      {
	  gaiaAppendToOutBuffer (xml_response, "HTTP/1.1 400 Bad Request\r\n");
	  gaiaAppendToOutBuffer (&http_text,
				 "<title>400 Bad Request</title>\r\n");
	  gaiaAppendToOutBuffer (&http_text, "</head><body>\r\n");
	  gaiaAppendToOutBuffer (&http_text, "<h1>Bad Request</h1>\n");
      }
    else
      {
	  gaiaAppendToOutBuffer (xml_response,
				 "HTTP/1.1 500 Internal Server Error\r\n");
	  gaiaAppendToOutBuffer (&http_text,
				 "<title>500 Internal Server Error</title>\r\n");
	  gaiaAppendToOutBuffer (&http_text, "</head><body>\r\n");
	  gaiaAppendToOutBuffer (&http_text,
				 "<h1>Internal Server Error</h1>\n");
      }
    dummy =
	sqlite3_mprintf
	("<address>WmsLite/%s [%s] at localhost (127.0.0.1) Port %d</address>\r\n",
	 rl2_version (), rl2_target_cpu (), port_no);
    gaiaAppendToOutBuffer (&http_text, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (&http_text, "</body></html>\r\n");
    gaiaAppendToOutBuffer (&http_text, "");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response,
			   "Content-Type: text/html;charset=UTF-8\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", http_text.WriteOffset);
    gaiaAppendToOutBuffer (xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (xml_response, "Connection: close\r\n\r\n");
    gaiaAppendToOutBuffer (xml_response, http_text.Buffer);
    gaiaOutBufferReset (&http_text);
}

static void
build_get_capabilities (struct wms_list *list, char **cached, int *cached_len,
			int port_no)
{
/* preparing the WMS GetCapabilities XML document */
    struct wms_layer *lyr;
    struct wms_group *grp;
    struct wms_style *style;
    char *dummy;
    gaiaOutBuffer xml_text;
    gaiaOutBufferInitialize (&xml_text);
    gaiaAppendToOutBuffer (&xml_text,
			   "<?xml version='1.0' encoding=\"UTF-8\" standalone=\"no\" ?>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<WMS_Capabilities version=\"1.3.0\" ");
    gaiaAppendToOutBuffer (&xml_text, "xmlns=\"http://www.opengis.net/wms\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
    gaiaAppendToOutBuffer (&xml_text,
			   "xsi:schemaLocation=\"http://www.opengis.net/wms ");
    gaiaAppendToOutBuffer (&xml_text,
			   "http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd\">\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Service>\r\n<Name>WMS</Name>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Title>WmsLite test server</Title>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Abstract>A simple light-weight WMS server for testing RasterLite2 Coverages.</Abstract>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<KeywordList>\r\n<Keyword>maps</Keyword>\r\n</KeywordList>\r\n");
    dummy =
	sqlite3_mprintf
	("<OnlineResource xlink:href=\"http://127.0.0.1:%d/wmslite?\" ",
	 port_no);
    gaiaAppendToOutBuffer (&xml_text, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (&xml_text,
			   "xlink:type=\"simple\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"/>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ContactInformation>\r\n<ContactPersonPrimary>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ContactPerson>James T. Kirk</ContactPerson>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ContactOrganization>United Federation of Planets, Starfleet</ContactOrganization>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "</ContactPersonPrimary>\r\n<ContactPosition>Starship Captain.</ContactPosition>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<ContactAddress>\r\n<AddressType>stellar</AddressType>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Address>USS Enterprise</Address>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<City>Planet Earth</City>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<StateOrProvince>Solar System</StateOrProvince>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<PostCode>12345#WYZ47NL@512</PostCode>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Country>Milky Way Galaxy</Country>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "</ContactAddress>\r\n</ContactInformation>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Fees>none</Fees>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<AccessConstraints>none</AccessConstraints>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<LayerLimit>1</LayerLimit>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<MaxWidth>5000</MaxWidth>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<MaxHeight>5000</MaxHeight>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "</Service>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Capability>\r\n<Request>\r\n<GetCapabilities>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Format>text/xml</Format>\r\n<DCPType>\r\n<HTTP>\r\n");
    dummy =
	sqlite3_mprintf
	("<Get><OnlineResource xlink:href=\"http://127.0.0.1:%d/wmslite?\" ",
	 port_no);
    gaiaAppendToOutBuffer (&xml_text, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (&xml_text,
			   "xlink:type=\"simple\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"/></Get>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "</HTTP>\r\n</DCPType>\r\n</GetCapabilities>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<GetMap>\r\n<Format>image/png</Format>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Format>image/jpeg</Format>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Format>application/x-pdf</Format>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<Format>image/tiff</Format>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "<DCPType>\r\n<HTTP>\r\n<Get>");
    dummy =
	sqlite3_mprintf
	("<OnlineResource xlink:href=\"http://127.0.0.1:%d/wmslite?\" ",
	 port_no);
    gaiaAppendToOutBuffer (&xml_text, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (&xml_text,
			   "xlink:type=\"simple\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"/></Get>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "</HTTP>\r\n</DCPType>\r\n</GetMap>\r\n</Request>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Exception>\r\n<Format>XML</Format>\r\n</Exception>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Layer>\r\n<Title>RasterLite2 Coverages</Title>\r\n");
    gaiaAppendToOutBuffer (&xml_text,
			   "<Abstract>OGC WMS compliant Service</Abstract>\r\n");

/* publishing any valid first-level Layer */
    lyr = list->first_layer;
    while (lyr != NULL)
      {
	  /* available Coverages */
	  if (lyr->valid == 0)
	    {
		/* skipping any invalid Layer */
		lyr = lyr->next;
		continue;
	    }
	  if (lyr->child_layer == 1)
	    {
		/* skipping any child layer */
		lyr = lyr->next;
		continue;
	    }
	  gaiaAppendToOutBuffer (&xml_text,
				 "<Layer queryable=\"0\" opaque=\"0\" cascaded=\"0\">\r\n");
	  dummy = sqlite3_mprintf ("<Name>%s</Name>\r\n", lyr->layer_name);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy = sqlite3_mprintf ("<Title>%s</Title>\r\n", lyr->title);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<Abstract>%s</Abstract>\r\n", lyr->abstract);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy = sqlite3_mprintf ("<CRS>EPSG:%d</CRS>\r\n", lyr->srid);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  gaiaAppendToOutBuffer (&xml_text, "<EX_GeographicBoundingBox>");
	  dummy =
	      sqlite3_mprintf ("<westBoundLongitude>%1.6f</westBoundLongitude>",
			       lyr->geo_minx);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<eastBoundLongitude>%1.6f</eastBoundLongitude>",
			       lyr->geo_maxx);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<southBoundLatitude>%1.6f</southBoundLatitude>",
			       lyr->geo_miny);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<northBoundLatitude>%1.6f</northBoundLatitude>",
			       lyr->geo_maxy);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  gaiaAppendToOutBuffer (&xml_text, "</EX_GeographicBoundingBox>\r\n");
	  if (lyr->has_flipped_axes)
	      dummy = sqlite3_mprintf ("<BoundingBox CRS=\"EPSG:%d\" "
				       "minx=\"%1.6f\" miny=\"%1.6f\" maxx=\"%1.6f\" maxy=\"%1.6f\"/>\r\n",
				       lyr->srid, lyr->miny, lyr->minx,
				       lyr->maxy, lyr->maxx);
	  else
	      dummy = sqlite3_mprintf ("<BoundingBox CRS=\"EPSG:%d\" "
				       "minx=\"%1.6f\" miny=\"%1.6f\" maxx=\"%1.6f\" maxy=\"%1.6f\"/>\r\n",
				       lyr->srid, lyr->minx, lyr->miny,
				       lyr->maxx, lyr->maxy);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  style = lyr->first_style;
	  while (style != NULL)
	    {
		if (style->valid == 0)
		  {
		      style = style->next;
		      continue;
		  }
		dummy =
		    sqlite3_mprintf ("<Style>\r\n<Name>%s</Name>\r\n",
				     style->name);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		dummy =
		    sqlite3_mprintf ("<Title>%s</Title>\r\n",
				     (style->title ==
				      NULL) ? style->name : style->title);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		if (style->abstract != NULL)
		  {
		      dummy =
			  sqlite3_mprintf ("<Abstract>%s</Abstract>\r\n",
					   style->abstract);
		      gaiaAppendToOutBuffer (&xml_text, dummy);
		      sqlite3_free (dummy);
		  }
		gaiaAppendToOutBuffer (&xml_text, "</Style>\r\n");
		style = style->next;
	    }
	  gaiaAppendToOutBuffer (&xml_text, "</Layer>\r\n");
	  lyr = lyr->next;
      }

/* publishing any valid Layer Group */
    grp = list->first_group;
    while (grp != NULL)
      {
	  /* available Groups */
	  struct wms_layer_ref *ref;
	  if (grp->valid == 0)
	    {
		/* skipping any invalid Layer */
		grp = grp->next;
		continue;
	    }
	  gaiaAppendToOutBuffer (&xml_text,
				 "<Layer queryable=\"0\" opaque=\"0\" cascaded=\"0\">\r\n");
	  dummy = sqlite3_mprintf ("<Name>%s</Name>\r\n", grp->group_name);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy = sqlite3_mprintf ("<Title>%s</Title>\r\n", grp->title);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<Abstract>%s</Abstract>\r\n", grp->abstract);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy = sqlite3_mprintf ("<CRS>EPSG:%d</CRS>\r\n", grp->srid);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  gaiaAppendToOutBuffer (&xml_text, "<EX_GeographicBoundingBox>");
	  dummy =
	      sqlite3_mprintf ("<westBoundLongitude>%1.6f</westBoundLongitude>",
			       grp->geo_minx);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<eastBoundLongitude>%1.6f</eastBoundLongitude>",
			       grp->geo_maxx);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<southBoundLatitude>%1.6f</southBoundLatitude>",
			       grp->geo_miny);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  dummy =
	      sqlite3_mprintf ("<northBoundLatitude>%1.6f</northBoundLatitude>",
			       grp->geo_maxy);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  gaiaAppendToOutBuffer (&xml_text, "</EX_GeographicBoundingBox>\r\n");
	  if (grp->has_flipped_axes)
	      dummy = sqlite3_mprintf ("<BoundingBox CRS=\"EPSG:%d\" "
				       "minx=\"%1.6f\" miny=\"%1.6f\" maxx=\"%1.6f\" maxy=\"%1.6f\"/>\r\n",
				       grp->srid, grp->miny, grp->minx,
				       grp->maxy, grp->maxx);
	  else
	      dummy = sqlite3_mprintf ("<BoundingBox CRS=\"EPSG:%d\" "
				       "minx=\"%1.6f\" miny=\"%1.6f\" maxx=\"%1.6f\" maxy=\"%1.6f\"/>\r\n",
				       grp->srid, grp->minx, grp->miny,
				       grp->maxx, grp->maxy);
	  gaiaAppendToOutBuffer (&xml_text, dummy);
	  sqlite3_free (dummy);
	  style = grp->first_style;
	  while (style != NULL)
	    {
		if (style->valid == 0)
		  {
		      style = style->next;
		      continue;
		  }
		dummy =
		    sqlite3_mprintf ("<Style>\r\n<Name>%s</Name>\r\n",
				     style->name);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		dummy =
		    sqlite3_mprintf ("<Title>%s</Title>\r\n",
				     (style->title ==
				      NULL) ? style->name : style->title);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		if (style->abstract != NULL)
		  {
		      dummy =
			  sqlite3_mprintf ("<Abstract>%s</Abstract>\r\n",
					   style->abstract);
		      gaiaAppendToOutBuffer (&xml_text, dummy);
		      sqlite3_free (dummy);
		  }
		gaiaAppendToOutBuffer (&xml_text, "</Style>\r\n");
		style = style->next;
	    }
	  /* publishing all valid children Layers */
	  ref = grp->first_child;
	  while (ref != NULL)
	    {
		/* available Coverages */
		if (ref->layer_ref->valid == 0)
		  {
		      /* skipping any invalid Child */
		      ref = ref->next;
		      continue;
		  }
		lyr = ref->layer_ref;
		gaiaAppendToOutBuffer (&xml_text,
				       "<Layer queryable=\"0\" opaque=\"0\" cascaded=\"0\">\r\n");
		dummy =
		    sqlite3_mprintf ("<Name>%s</Name>\r\n", lyr->layer_name);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		dummy = sqlite3_mprintf ("<Title>%s</Title>\r\n", lyr->title);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		dummy =
		    sqlite3_mprintf ("<Abstract>%s</Abstract>\r\n",
				     lyr->abstract);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		dummy = sqlite3_mprintf ("<CRS>EPSG:%d</CRS>\r\n", lyr->srid);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		gaiaAppendToOutBuffer (&xml_text, "<EX_GeographicBoundingBox>");
		dummy =
		    sqlite3_mprintf
		    ("<westBoundLongitude>%1.6f</westBoundLongitude>",
		     lyr->geo_minx);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		dummy =
		    sqlite3_mprintf
		    ("<eastBoundLongitude>%1.6f</eastBoundLongitude>",
		     lyr->geo_maxx);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		dummy =
		    sqlite3_mprintf
		    ("<southBoundLatitude>%1.6f</southBoundLatitude>",
		     lyr->geo_miny);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		dummy =
		    sqlite3_mprintf
		    ("<northBoundLatitude>%1.6f</northBoundLatitude>",
		     lyr->geo_maxy);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		gaiaAppendToOutBuffer (&xml_text,
				       "</EX_GeographicBoundingBox>\r\n");
		if (lyr->has_flipped_axes)
		    dummy = sqlite3_mprintf ("<BoundingBox CRS=\"EPSG:%d\" "
					     "minx=\"%1.6f\" miny=\"%1.6f\" maxx=\"%1.6f\" maxy=\"%1.6f\"/>\r\n",
					     lyr->srid, lyr->miny, lyr->minx,
					     lyr->maxy, lyr->maxx);
		else
		    dummy = sqlite3_mprintf ("<BoundingBox CRS=\"EPSG:%d\" "
					     "minx=\"%1.6f\" miny=\"%1.6f\" maxx=\"%1.6f\" maxy=\"%1.6f\"/>\r\n",
					     lyr->srid, lyr->minx, lyr->miny,
					     lyr->maxx, lyr->maxy);
		gaiaAppendToOutBuffer (&xml_text, dummy);
		sqlite3_free (dummy);
		style = lyr->first_style;
		while (style != NULL)
		  {
		      if (style->valid == 0)
			{
			    style = style->next;
			    continue;
			}
		      dummy =
			  sqlite3_mprintf ("<Style>\r\n<Name>%s</Name>\r\n",
					   style->name);
		      gaiaAppendToOutBuffer (&xml_text, dummy);
		      sqlite3_free (dummy);
		      dummy =
			  sqlite3_mprintf ("<Title>%s</Title>\r\n",
					   (style->title ==
					    NULL) ? style->name : style->title);
		      gaiaAppendToOutBuffer (&xml_text, dummy);
		      sqlite3_free (dummy);
		      if (style->abstract != NULL)
			{
			    dummy =
				sqlite3_mprintf ("<Abstract>%s</Abstract>\r\n",
						 style->abstract);
			    gaiaAppendToOutBuffer (&xml_text, dummy);
			    sqlite3_free (dummy);
			}
		      gaiaAppendToOutBuffer (&xml_text, "</Style>\r\n");
		      style = style->next;
		  }
		gaiaAppendToOutBuffer (&xml_text, "</Layer>\r\n");
		ref = ref->next;
	    }
	  gaiaAppendToOutBuffer (&xml_text, "</Layer>\r\n");
	  grp = grp->next;
      }

    gaiaAppendToOutBuffer (&xml_text,
			   "</Layer>\r\n</Capability>\r\n</WMS_Capabilities>\r\n");
    gaiaAppendToOutBuffer (&xml_text, "");
    *cached = xml_text.Buffer;
    *cached_len = xml_text.WriteOffset;
    xml_text.Buffer = NULL;
    gaiaOutBufferReset (&xml_text);
}

static int
get_xml_bytes (gaiaOutBufferPtr xml_response, int curr, int block_sz)
{
/* determining how many bytes will be sent on the output socket */
    int wr = block_sz;
    if (xml_response->Buffer == NULL || xml_response->Error)
	return 0;
    if (curr + wr > xml_response->WriteOffset)
	wr = xml_response->WriteOffset - curr;
    return wr;
}

static int
get_payload_bytes (int total, int curr, int block_sz)
{
/* determining how many bytes will be sent on the output socket */
    int wr = block_sz;
    if (curr + wr > total)
	wr = total - curr;
    return wr;
}

static void
#ifdef _WIN32
wms_get_capabilities (SOCKET socket, const char *cached,
		      int cached_len, struct server_log_item *log)
#else
wms_get_capabilities (int socket, const char *cached,
		      int cached_len, struct server_log_item *log)
#endif
{
/* preparing the WMS GetCapabilities XML document */
    gaiaOutBuffer xml_response;
    char *dummy;
    int curr;
    int rd;
    int wr;

    gaiaOutBufferInitialize (&xml_response);
    gaiaAppendToOutBuffer (&xml_response, "HTTP/1.1 200 OK\r\n");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (&xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (&xml_response,
			   "Content-Type: text/xml;charset=UTF-8\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", cached_len);
    gaiaAppendToOutBuffer (&xml_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (&xml_response, "Connection: close\r\n\r\n");
    gaiaAppendToOutBuffer (&xml_response, cached);

/* uploading the HTTP response */
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, SEND_BLOK_SZ);
	  if (rd == 0)
	      break;
	  wr = send (socket, xml_response.Buffer + curr, rd, 0);
	  if (wr < 0)
	      break;
	  curr += wr;
      }
    log_get_capabilities_2 (log, xml_response.WriteOffset);
    gaiaOutBufferReset (&xml_response);
}

static void
#ifdef _WIN32
wms_get_map (struct wms_args *args, SOCKET socket, struct server_log_item *log)
#else
wms_get_map (struct wms_args *args, int socket, struct server_log_item *log)
#endif
{
/* preparing the WMS GetMap payload */
    int ret;
    char *dummy;
    int curr;
    int rd;
    int wr;
    int save_sz;
    sqlite3_stmt *stmt = NULL;
    gaiaOutBuffer http_response;
    unsigned char *payload;
    int payload_size;
    unsigned char *black;
    int black_sz;
    char bgcolor[16];

    stmt = args->stmt_get_map;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, args->layer, strlen (args->layer),
		       SQLITE_STATIC);
    sqlite3_bind_double (stmt, 2, args->minx);
    sqlite3_bind_double (stmt, 3, args->miny);
    sqlite3_bind_double (stmt, 4, args->maxx);
    sqlite3_bind_double (stmt, 5, args->maxy);
    sqlite3_bind_int (stmt, 6, args->width);
    sqlite3_bind_int (stmt, 7, args->height);
    sqlite3_bind_text (stmt, 8, args->style, strlen (args->style),
		       SQLITE_STATIC);
    if (args->format == RL2_OUTPUT_FORMAT_TIFF)
	sqlite3_bind_text (stmt, 9, "image/tiff", strlen ("image/tiff"),
			   SQLITE_TRANSIENT);
    else if (args->format == RL2_OUTPUT_FORMAT_PDF)
	sqlite3_bind_text (stmt, 9, "application/x-pdf",
			   strlen ("application/x-pdf"), SQLITE_TRANSIENT);
    else if (args->format == RL2_OUTPUT_FORMAT_JPEG)
	sqlite3_bind_text (stmt, 9, "image/jpeg", strlen ("image/jpeg"),
			   SQLITE_TRANSIENT);
    else
	sqlite3_bind_text (stmt, 9, "image/png", strlen ("image/png"),
			   SQLITE_TRANSIENT);
    if (args->has_bgcolor)
	sprintf (bgcolor, "#%02x%02x%02x", args->red, args->green, args->blue);
    else
	strcpy (bgcolor, "#ffffff");
    sqlite3_bind_text (stmt, 10, bgcolor, strlen (bgcolor), SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 11, args->transparent);
    if (args->format == RL2_OUTPUT_FORMAT_JPEG)
	sqlite3_bind_int (stmt, 12, 80);
    else
	sqlite3_bind_int (stmt, 12, 100);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      payload = (unsigned char *) sqlite3_column_blob (stmt, 0);
		      payload_size = sqlite3_column_bytes (stmt, 0);
		      /* preparing the HTTP response */
		      gaiaOutBufferInitialize (&http_response);
		      gaiaAppendToOutBuffer (&http_response,
					     "HTTP/1.1 200 OK\r\n");
		      dummy = get_current_timestamp ();
		      gaiaAppendToOutBuffer (&http_response, dummy);
		      sqlite3_free (dummy);
		      if (args->format == RL2_OUTPUT_FORMAT_JPEG)
			  gaiaAppendToOutBuffer (&http_response,
						 "Content-Type: image/jpeg\r\n");
		      if (args->format == RL2_OUTPUT_FORMAT_PNG)
			  gaiaAppendToOutBuffer (&http_response,
						 "Content-Type: image/png\r\n");
		      if (args->format == RL2_OUTPUT_FORMAT_TIFF)
			  gaiaAppendToOutBuffer (&http_response,
						 "Content-Type: image/tiff\r\n");
		      if (args->format == RL2_OUTPUT_FORMAT_PDF)
			  gaiaAppendToOutBuffer (&http_response,
						 "Content-Type: application/x-pdf\r\n");
		      dummy =
			  sqlite3_mprintf ("Content-Length: %d\r\n",
					   payload_size);
		      gaiaAppendToOutBuffer (&http_response, dummy);
		      sqlite3_free (dummy);
		      gaiaAppendToOutBuffer (&http_response,
					     "Connection: close\r\n\r\n");
		      /* uploading the HTTP response header */
		      curr = 0;
		      while (1)
			{
			    rd = get_xml_bytes (&http_response, curr,
						SEND_BLOK_SZ);
			    if (rd == 0)
				break;
			    wr = send (socket, http_response.Buffer + curr, rd,
				       0);
			    if (wr < 0)
				break;
			    curr += wr;
			}
		      save_sz = http_response.WriteOffset;
		      gaiaOutBufferReset (&http_response);
		      /* uploading the image payload */
		      curr = 0;
		      while (1)
			{
			    rd = get_payload_bytes (payload_size, curr,
						    SEND_BLOK_SZ);
			    if (rd == 0)
				break;
			    wr = send (socket, payload + curr, rd, 0);
			    if (wr < 0)
				break;
			    curr += wr;
			}
		      log_get_map_2 (log, save_sz + payload_size);
		      return;
		  }
	    }
      }

/* preparing a default black image */
    black_sz = args->width * args->height;
    black = malloc (black_sz);
    memset (black, 0, black_sz);
    if (args->format == RL2_OUTPUT_FORMAT_JPEG)
	rl2_gray_to_jpeg (args->width, args->height, black, 80, &payload,
			  &payload_size);
    if (args->format == RL2_OUTPUT_FORMAT_PNG)
	rl2_gray_to_png (args->width, args->height, black, &payload,
			 &payload_size);
    if (args->format == RL2_OUTPUT_FORMAT_TIFF)
	rl2_gray_to_tiff (args->width, args->height, black, &payload,
			  &payload_size);
    if (args->format == RL2_OUTPUT_FORMAT_PDF)
	rl2_gray_pdf (args->width, args->height, &payload, &payload_size);
    free (black);
    /* preparing the HTTP response */
    gaiaOutBufferInitialize (&http_response);
    gaiaAppendToOutBuffer (&http_response, "HTTP/1.1 200 OK\r\n");
    dummy = get_current_timestamp ();
    gaiaAppendToOutBuffer (&http_response, dummy);
    sqlite3_free (dummy);
    if (args->format == RL2_OUTPUT_FORMAT_JPEG)
	gaiaAppendToOutBuffer (&http_response, "Content-Type: image/jpeg\r\n");
    if (args->format == RL2_OUTPUT_FORMAT_PNG)
	gaiaAppendToOutBuffer (&http_response, "Content-Type: image/png\r\n");
    if (args->format == RL2_OUTPUT_FORMAT_TIFF)
	gaiaAppendToOutBuffer (&http_response, "Content-Type: image/tiff\r\n");
    if (args->format == RL2_OUTPUT_FORMAT_TIFF)
	gaiaAppendToOutBuffer (&http_response,
			       "Content-Type: application/x-pdf\r\n");
    dummy = sqlite3_mprintf ("Content-Length: %d\r\n", payload_size);
    gaiaAppendToOutBuffer (&http_response, dummy);
    sqlite3_free (dummy);
    gaiaAppendToOutBuffer (&http_response, "Connection: close\r\n\r\n");
    /* uploading the HTTP response header */
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&http_response, curr, SEND_BLOK_SZ);
	  if (rd == 0)
	      break;
	  wr = send (socket, http_response.Buffer + curr, rd, 0);
	  if (wr < 0)
	      break;
	  curr += wr;
      }
    save_sz = http_response.WriteOffset;
    gaiaOutBufferReset (&http_response);
    /* uploading the image payload */
    curr = 0;
    while (1)
      {
	  rd = get_payload_bytes (payload_size, curr, SEND_BLOK_SZ);
	  if (rd == 0)
	      break;
	  wr = send (socket, payload + curr, rd, 0);
	  if (wr < 0)
	      break;
	  curr += wr;
      }
    free (payload);
    log_get_map_2 (log, save_sz + payload_size);
}

#ifdef _WIN32
/* Winsockets - some king of Windows */
static void
win32_http_request (void *data)
{
/* Processing an incoming HTTP request */
    gaiaOutBuffer xml_response;
    struct http_request *req = (struct http_request *) data;
    struct wms_args *args = NULL;
    int curr;
    int rd;
    int wr;
    char *ptr;
    char http_hdr[2000];	/* The HTTP request header */
    int http_status;
    char *method = NULL;
    char *url = NULL;
    char *timestamp = get_sql_timestamp ();

    curr = 0;
    while ((unsigned int) curr < sizeof (http_hdr))
      {
	  rd = recv (req->socket, &http_hdr[curr], sizeof (http_hdr) - 1 - curr,
		     0);
	  if (rd == SOCKET_ERROR)
	      goto end_request;
	  if (rd == 0)
	      break;
	  curr += rd;
	  http_hdr[curr] = '\0';
	  ptr = strstr (http_hdr, "\r\n\r\n");
	  if (ptr)
	      break;
      }
    if ((unsigned int) curr >= sizeof (http_hdr))
      {
	  http_status = 400;
	  goto http_error;
      }

    args = parse_http_request (http_hdr, &method, &url);
    if (args == NULL)
      {
	  http_status = 400;
	  goto http_error;
      }

    http_status = check_wms_request (req->list, args);
    if (http_status != 200)
	goto http_error;
    if (args->request_type == WMS_ILLEGAL_REQUEST)
	goto illegal_request;
    if (args->request_type == WMS_GET_CAPABILITIES)
      {
	  /* preparing the XML WMS GetCapabilities */
	  log_get_capabilities_1 (req->log, timestamp, http_status, method,
				  url);
	  wms_get_capabilities (req->socket, req->cached_capabilities,
				req->cached_capabilities_len, req->log);
      }
    if (args->request_type == WMS_GET_MAP)
      {
	  /* preparing the WMS GetMap payload */
	  args->db_handle = req->conn->handle;
	  args->stmt_get_map = req->conn->stmt_get_map;
	  log_get_map_1 (req->log, timestamp, http_status, method, url, args);
	  wms_get_map (args, req->socket, req->log);
      }
    goto end_request;

/* preparing an HTTP error code */
  http_error:
    gaiaOutBufferInitialize (&xml_response);
    build_http_error (http_status, &xml_response, req->port_no);
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, SEND_BLOK_SZ);
	  if (rd == 0)
	      break;
	  send (req->socket, xml_response.Buffer + curr, rd, 0);
	  curr += rd;
      }
    log_error (req->log, timestamp, http_status, method, url,
	       xml_response.WriteOffset);
    gaiaOutBufferReset (&xml_response);
    goto end_request;

/* preparing an XML WMS Exception message */
  illegal_request:
    gaiaOutBufferInitialize (&xml_response);
    build_wms_exception (args, &xml_response);
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, SEND_BLOK_SZ);
	  if (rd == 0)
	      break;
	  send (req->socket, xml_response.Buffer + curr, rd, 0);
	  curr += rd;
      }
    log_error (req->log, timestamp, http_status, method, url,
	       xml_response.WriteOffset);
    gaiaOutBufferReset (&xml_response);

  end_request:
    if (args != NULL)
	destroy_wms_args (args);
    closesocket (req->socket);
    req->conn->status = CONNECTION_AVAILABLE;
    free (req);
}
#else
/* standard Berkeley Sockets - may be Linux or *nix */
static void *
berkeley_http_request (void *data)
{
/* Processing an incoming HTTP request */
    gaiaOutBuffer xml_response;
    struct http_request *req = (struct http_request *) data;
    struct wms_args *args = NULL;
    int curr;
    int rd;
    char *ptr;
    char http_hdr[2000];	/* The HTTP request header */
    int http_status;
    char *method = NULL;
    char *url = NULL;
    char *timestamp = get_sql_timestamp ();

    curr = 0;
    while ((unsigned int) curr < sizeof (http_hdr))
      {
	  rd = recv (req->socket, &http_hdr[curr], sizeof (http_hdr) - 1 - curr,
		     0);
	  if (rd == -1)
	      goto end_request;
	  if (rd == 0)
	      break;
	  curr += rd;
	  http_hdr[curr] = '\0';
	  ptr = strstr (http_hdr, "\r\n\r\n");
	  if (ptr)
	      break;
      }
    if ((unsigned int) curr >= sizeof (http_hdr))
      {
	  http_status = 400;
	  goto http_error;
      }

    args = parse_http_request (http_hdr, &method, &url);
    if (args == NULL)
      {
	  http_status = 400;
	  goto http_error;
      }

    http_status = check_wms_request (req->list, args);
    if (http_status != 200)
	goto http_error;
    if (args->request_type == WMS_ILLEGAL_REQUEST)
	goto illegal_request;
    if (args->request_type == WMS_GET_CAPABILITIES)
      {
	  /* uploading the XML WMS GetCapabilities */
	  log_get_capabilities_1 (req->log, timestamp, http_status, method,
				  url);
	  wms_get_capabilities (req->socket, req->cached_capabilities,
				req->cached_capabilities_len, req->log);
      }
    if (args->request_type == WMS_GET_MAP)
      {
	  /* preparing the WMS GetMap payload */
	  args->db_handle = req->conn->handle;
	  args->stmt_get_map = req->conn->stmt_get_map;
	  log_get_map_1 (req->log, timestamp, http_status, method, url, args);
	  wms_get_map (args, req->socket, req->log);
      }
    goto end_request;

/* preparing an HTTP error code */
  http_error:
    gaiaOutBufferInitialize (&xml_response);
    build_http_error (http_status, &xml_response, req->port_no);
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, SEND_BLOK_SZ);
	  if (rd == 0)
	      break;
	  send (req->socket, xml_response.Buffer + curr, rd, 0);
	  curr += rd;
      }
    log_error (req->log, timestamp, http_status, method, url,
	       xml_response.WriteOffset);
    gaiaOutBufferReset (&xml_response);
    goto end_request;

/* preparing an XML WMS Exception message */
  illegal_request:
    gaiaOutBufferInitialize (&xml_response);
    build_wms_exception (args, &xml_response);
    curr = 0;
    while (1)
      {
	  rd = get_xml_bytes (&xml_response, curr, SEND_BLOK_SZ);
	  if (rd == 0)
	      break;
	  send (req->socket, xml_response.Buffer + curr, rd, 0);
	  curr += rd;
      }
    log_error (req->log, timestamp, http_status, method, url,
	       xml_response.WriteOffset);
    gaiaOutBufferReset (&xml_response);

  end_request:
    if (args != NULL)
	destroy_wms_args (args);
    close (req->socket);
    req->conn->status = CONNECTION_AVAILABLE;
    free (req);
    pthread_exit (NULL);
}
#endif

static void
do_accept_loop (struct neutral_socket *skt, struct wms_list *list, int port_no,
		sqlite3 * db_handle, sqlite3_stmt * stmt_log,
		struct connections_pool *pool, struct server_log *log,
		char *cached_capab, int cached_capab_len)
{
/* implementing the ACCEPT loop */
    unsigned int id = 0;
    struct read_connection *conn;
    int ic;
    time_t diff;
    time_t now;
    char *ip_addr;
    char *ip_addr2;
#ifdef _WIN32
    SOCKET socket = skt->socket;
    SOCKET client;
    SOCKADDR_IN client_addr;
    int len = sizeof (client_addr);
    int wsaError;
    struct http_request *req;

    while (1)
      {
	  /* never ending loop */
	  client = accept (socket, (struct sockaddr *) &client_addr, &len);
	  if (client == INVALID_SOCKET)
	    {
		wsaError = WSAGetLastError ();
		if (wsaError == WSAEINTR || wsaError == WSAENOTSOCK)
		  {
		      WSACleanup ();
		      fprintf (stderr, "accept error: %d\n", wsaError);
		      return;
		  }
		else
		  {
		      closesocket (socket);
		      WSACleanup ();
		      fprintf (stderr, "error from accept()\n");
		      return;
		  }
	    }
	  req = malloc (sizeof (struct http_request));
	  req->id = id++;
	  req->port_no = port_no;
	  req->socket = client;
	  req->list = list;
	  req->cached_capabilities = cached_capab;
	  req->cached_capabilities_len = cached_capab_len;
	  while (1)
	    {
		/* looping until an available read connection is found */
		for (ic = 0; ic < MAX_CONN; ic++)
		  {
		      /* selecting an available connection (if any) */
		      struct read_connection *ptr = &(pool->connections[ic]);
		      if (ptr->status == CONNECTION_AVAILABLE)
			{
			    conn = ptr;
			    conn->status = CONNECTION_BUSY;
			    goto conn_found;
			}
		  }
		Sleep (50);
	    }
	conn_found:
	  req->conn = conn;
	  req->log_handle = db_handle;
	  time (&now);
	  diff = now - log->last_update;
	  if (log->next_item < MAX_LOG && diff < 30)
	    {
		/* reserving a free log slot */
		req->log = &(log->items[log->next_item++]);
		req->log->status = LOG_SLOT_BUSY;
		ip_addr = inet_ntoa (client_addr.sin_addr);
		ip_addr2 = NULL;
		if (ip_addr != NULL)
		  {
		      int len = strlen (ip_addr);
		      ip_addr2 = malloc (len + 1);
		      strcpy (ip_addr2, ip_addr);
		  }
		req->log->client_ip_addr = ip_addr2;
		req->log->client_ip_port = client_addr.sin_port;
		gettimeofday (&(req->log->begin_time), NULL);
	    }
	  else
	    {
		/* flushing the log */
		flush_log (db_handle, stmt_log, log);
		/* reserving a free log slot */
		req->log = &(log->items[log->next_item++]);
		req->log->status = LOG_SLOT_BUSY;
		ip_addr = inet_ntoa (client_addr.sin_addr);
		ip_addr2 = NULL;
		if (ip_addr != NULL)
		  {
		      int len = strlen (ip_addr);
		      ip_addr2 = malloc (len + 1);
		      strcpy (ip_addr2, ip_addr);
		  }
		req->log->client_ip_addr = ip_addr2;
		req->log->client_ip_port = client_addr.sin_port;
		gettimeofday (&(req->log->begin_time), NULL);
	    }
	  _beginthread (win32_http_request, 0, (void *) req);
      }
    return;
#else
    pthread_t thread_id;
    int socket = skt->socket;
    int client;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof (struct sockaddr_in);
    struct http_request *req;

    while (1)
      {
	  /* never ending loop */
	  client = accept (socket, (struct sockaddr *) &client_addr, &len);
	  if (client == -1)
	    {
		close (socket);
		fprintf (stderr, "error from accept()\n");
		return;
	    }
	  req = malloc (sizeof (struct http_request));
	  req->id = id++;
	  req->port_no = port_no;
	  req->socket = client;
	  req->list = list;
	  req->cached_capabilities = cached_capab;
	  req->cached_capabilities_len = cached_capab_len;
	  while (1)
	    {
		/* looping until an available read connection is found */
		for (ic = 0; ic < MAX_CONN; ic++)
		  {
		      /* selecting an available connection (if any) */
		      struct read_connection *ptr = &(pool->connections[ic]);
		      if (ptr->status == CONNECTION_AVAILABLE)
			{
			    conn = ptr;
			    conn->status = CONNECTION_BUSY;
			    goto conn_found;
			}
		  }
		usleep (50);
	    }
	conn_found:
	  req->conn = conn;
	  req->log_handle = db_handle;
	  time (&now);
	  diff = now - log->last_update;
	  if (log->next_item < MAX_LOG && diff < 30)
	    {
		/* reserving a free log slot */
		req->log = &(log->items[log->next_item++]);
		req->log->status = LOG_SLOT_BUSY;
		ip_addr = inet_ntoa (client_addr.sin_addr);
		ip_addr2 = NULL;
		if (ip_addr != NULL)
		  {
		      int len = strlen (ip_addr);
		      ip_addr2 = malloc (len + 1);
		      strcpy (ip_addr2, ip_addr);
		  }
		req->log->client_ip_addr = ip_addr2;
		req->log->client_ip_port = client_addr.sin_port;
		gettimeofday (&(req->log->begin_time), NULL);
	    }
	  else
	    {
		/* flushing the log */
		flush_log (db_handle, stmt_log, log);
		/* reserving a free log slot */
		req->log = &(log->items[log->next_item++]);
		req->log->status = LOG_SLOT_BUSY;
		ip_addr = inet_ntoa (client_addr.sin_addr);
		ip_addr2 = NULL;
		if (ip_addr != NULL)
		  {
		      int len = strlen (ip_addr);
		      ip_addr2 = malloc (len + 1);
		      strcpy (ip_addr2, ip_addr);
		  }
		req->log->client_ip_addr = ip_addr2;
		req->log->client_ip_port = client_addr.sin_port;
		gettimeofday (&(req->log->begin_time), NULL);
	    }
	  pthread_create (&thread_id, NULL, berkeley_http_request,
			  (void *) req);
      }
#endif
}

static int
do_start_http (int port_no, struct neutral_socket *srv_skt)
{
/* starting the HTTP server */
#ifdef _WIN32
/* Winsockets */
    WSADATA wd;
    SOCKET skt = INVALID_SOCKET;
    SOCKADDR_IN addr;

    if (WSAStartup (MAKEWORD (1, 1), &wd))
      {
	  fprintf (stderr, "unable to initialize winsock\n");
	  return 0;
      }
    skt = socket (AF_INET, SOCK_STREAM, 0);
    if (skt == INVALID_SOCKET)
      {
	  fprintf (stderr, "unable to create a socket\n");
	  return 0;
      }
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port_no);
    addr.sin_addr.s_addr = inet_addr ("127.0.0.1");
    if (bind (skt, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
      {
	  fprintf (stderr, "unable to bind the socket\n");
	  closesocket (skt);
	  return 0;
      }
    if (listen (skt, SOMAXCONN) == SOCKET_ERROR)
      {
	  fprintf (stderr, "unable to listen on the socket\n");
	  closesocket (skt);
	  return 0;
      }
    srv_skt->socket = skt;
#else
/* standard Berkeley sockets */
    int skt = -1;
    struct sockaddr_in addr;

    skt = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (skt == -1)
      {
	  fprintf (stderr, "unable to create a socket\n");
	  return 0;
      }
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port_no);
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (skt, (struct sockaddr *) &addr, sizeof (addr)) == -1)
      {
	  fprintf (stderr, "unable to bind the socket\n");
	  close (skt);
	  return 0;
      }
    if (listen (skt, SOMAXCONN) == -1)
      {
	  fprintf (stderr, "unable to listen on the socket\n");
	  close (skt);
	  return 0;
      }
    srv_skt->socket = skt;
#endif
    fprintf (stderr,
	     "======================================================\n");
    fprintf (stderr, "    HTTP micro-server listening on port: %d\n", port_no);
    fprintf (stderr,
	     "======================================================\n");
    return 1;
}

static int
get_geographic_extent (sqlite3 * handle, struct wms_layer *lyr)
{
/* computing the Geographic Bounding Box for some WMS layer */
    int retcode = 0;
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int ret;
    sql = "SELECT MbrMinX(g.g), MbrMinY(g.g), MbrMaxX(g.g), MbrMaxY(g.g) "
	"FROM (SELECT ST_Transform(BuildMbr(?, ?, ?, ?, ?), 4326) AS g) AS g";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  lyr->geo_minx = -180.0;
	  lyr->geo_miny = -90.0;
	  lyr->geo_maxx = 180.0;
	  lyr->geo_maxy = 90.0;
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, lyr->minx);
    sqlite3_bind_double (stmt, 2, lyr->miny);
    sqlite3_bind_double (stmt, 3, lyr->maxx);
    sqlite3_bind_double (stmt, 4, lyr->maxy);
    sqlite3_bind_int (stmt, 5, lyr->srid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		lyr->geo_minx = sqlite3_column_double (stmt, 0);
		lyr->geo_miny = sqlite3_column_double (stmt, 1);
		lyr->geo_maxx = sqlite3_column_double (stmt, 2);
		lyr->geo_maxy = sqlite3_column_double (stmt, 3);
		retcode = 1;
	    }
      }
    sqlite3_finalize (stmt);
    return retcode;
}

static void
compute_geographic_extents (sqlite3 * handle, struct wms_list *list)
{
/* computing the Geographic Bounding Box for every WMS layer */
    struct wms_group *grp;
    struct wms_layer *lyr;

/* first level layers */
    lyr = list->first_layer;
    while (lyr != NULL)
      {
	  if (get_geographic_extent (handle, lyr))
	      fprintf (stderr, "Publishing layer \"%s\"\n", lyr->layer_name);
	  else
	      lyr->valid = 0;
	  lyr = lyr->next;
      }

    grp = list->first_group;
    while (grp != NULL)
      {
	  /* validating layer groups */
	  int valid;
	  int count;
	  int srid;
	  struct wms_style *style;
	  struct wms_layer_ref *ref;
	  ref = grp->first_child;
	  valid = 1;
	  count = 0;
	  if (ref != NULL)
	      srid = ref->layer_ref->srid;
	  while (ref != NULL)
	    {
		/* preliminary check */
		if (ref->layer_ref->valid == 0)
		    valid = 0;
		if (ref->layer_ref->srid != srid)
		    valid = 0;
		count++;
		ref = ref->next;
	    }
	  if (count < 1 || valid == 0)
	    {
		/* reporting an error condition */
		fprintf (stderr,
			 "ERROR: layer group \"%s\" has mismatching SRIDs !!!\n"
			 "\t... will not be published\n", grp->group_name);
		grp->valid = 0;
	    }
	  else
	    {
		/* final refinement pass */
		double minx = DBL_MAX;
		double maxx = 0.0 - DBL_MAX;
		double miny = DBL_MAX;
		double maxy = 0.0 - DBL_MAX;
		double geo_minx = DBL_MAX;
		double geo_maxx = 0.0 - DBL_MAX;
		double geo_miny = DBL_MAX;
		double geo_maxy = 0.0 - DBL_MAX;
		ref = grp->first_child;
		while (ref != NULL)
		  {
		      struct wms_layer *l = ref->layer_ref;
		      if (ref == grp->first_child)
			{
			    grp->srid = l->srid;
			    grp->has_flipped_axes = l->has_flipped_axes;
			    if (l->minx < minx)
				minx = l->minx;
			    if (l->maxx > maxx)
				maxx = l->maxx;
			    if (l->miny < miny)
				miny = l->miny;
			    if (l->maxy > maxy)
				maxy = l->maxy;
			    if (l->geo_minx < geo_minx)
				geo_minx = l->geo_minx;
			    if (l->geo_maxx > geo_maxx)
				geo_maxx = l->geo_maxx;
			    if (l->geo_miny < geo_miny)
				geo_miny = l->geo_miny;
			    if (l->geo_maxy > geo_maxy)
				geo_maxy = l->geo_maxy;
			}
		      ref = ref->next;
		  }
		grp->minx = minx;
		grp->maxx = maxx;
		grp->miny = miny;
		grp->maxy = maxy;
		grp->geo_minx = geo_minx;
		grp->geo_maxx = geo_maxx;
		grp->geo_miny = geo_miny;
		grp->geo_maxy = geo_maxy;

		/* validating Group Styles */
		style = grp->first_style;
		while (style != NULL)
		  {
		      int valid;
		      rl2GroupStylePtr stl;
		      if (strcmp (style->name, "default") == 0)
			{
			    /* ignoring default styles */
			    style->valid = 0;
			    style = style->next;
			    continue;
			}
		      stl = rl2_create_group_style_from_dbms (handle,
							      grp->group_name,
							      style->name);
		      if (stl == NULL)
			{
			    /* reporting an error condition */
			    fprintf (stderr,
				     "ERROR: layer group \"%s\" NULL style !!!\n"
				     "\t... will be ignored\n",
				     grp->group_name);
			    style->valid = 0;
			    style = style->next;
			    continue;
			}
		      rl2_is_valid_group_style (stl, &valid);
		      if (!valid)
			{
			    /* reporting an error condition */
			    int count;
			    int idx;
			    fprintf (stderr,
				     "ERROR: layer group \"%s\" invalid style \"%s\" !!!\n"
				     "\t... will be ignored\n", grp->group_name,
				     rl2_get_group_style_name (stl));
			    rl2_get_group_style_count (stl, &count);
			    for (idx = 0; idx < count; idx++)
			      {
				  rl2_is_valid_group_named_layer (stl, idx,
								  &valid);
				  if (!valid)
				      fprintf (stderr,
					       "\t%d/%d\tNamedLayer \"%s\" does not exists !!!\n",
					       idx, count,
					       rl2_get_group_named_layer (stl,
									  idx));
				  rl2_is_valid_group_named_style (stl, idx,
								  &valid);
				  if (!valid)
				      fprintf (stderr,
					       "\t\tNamedStyle \"%s\" does not exists !!!\n",
					       rl2_get_group_named_style (stl,
									  idx));
			      }
			    style->valid = 0;
			    style = style->next;
			    continue;
			}
		      rl2_destroy_group_style (stl);
		      style = style->next;
		  }
		if (grp->valid)
		    fprintf (stderr, "Publishing Layer Group \"%s\"\n",
			     grp->group_name);
		grp = grp->next;
	    }
      }
}

static int
unsupported_codec (const char *compression)
{
/* testing for unsupported optional codecs */
    if (strcasecmp (compression, "LZMA") == 0
	&& rl2_is_supported_codec (RL2_COMPRESSION_LZMA) != 1)
	return 1;
    if (strcasecmp (compression, "LZMA_NO") == 0
	&& rl2_is_supported_codec (RL2_COMPRESSION_LZMA_NO) != 1)
	return 1;
    if (strcasecmp (compression, "CHARLS") == 0
	&& rl2_is_supported_codec (RL2_COMPRESSION_CHARLS) != 1)
	return 1;
    if (strcasecmp (compression, "WEBP") == 0
	&& rl2_is_supported_codec (RL2_COMPRESSION_LOSSY_WEBP) != 1)
	return 1;
    if (strcasecmp (compression, "LL_WEBP") == 0
	&& rl2_is_supported_codec (RL2_COMPRESSION_LOSSLESS_WEBP) != 1)
	return 1;
    if (strcasecmp (compression, "JP2") == 0
	&& rl2_is_supported_codec (RL2_COMPRESSION_LOSSY_JP2) != 1)
	return 1;
    if (strcasecmp (compression, "LL_JP2") == 0
	&& rl2_is_supported_codec (RL2_COMPRESSION_LOSSLESS_JP2) != 1)
	return 1;
    return 0;
}

static struct wms_layer *
load_layer (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* creating a WMS layer from the resultset */
    struct wms_layer *lyr = NULL;
    const char *coverage_name = (const char *) sqlite3_column_text (stmt, 0);
    const char *title = (const char *) sqlite3_column_text (stmt, 1);
    const char *abstract = (const char *) sqlite3_column_text (stmt, 2);
    const char *sample_type = (const char *) sqlite3_column_text (stmt, 3);
    const char *pixel_type = (const char *) sqlite3_column_text (stmt, 4);
    int num_bands = sqlite3_column_int (stmt, 5);
    int srid = sqlite3_column_int (stmt, 6);
    double minx = sqlite3_column_double (stmt, 7);
    double miny = sqlite3_column_double (stmt, 8);
    double maxx = sqlite3_column_double (stmt, 9);
    double maxy = sqlite3_column_double (stmt, 10);
    const char *compression = (const char *) sqlite3_column_text (stmt, 11);
    int has_flipped_axes = 0;
    if (unsupported_codec (compression))
	return NULL;
    if (!srid_has_flipped_axes (handle, srid, &has_flipped_axes))
	has_flipped_axes = 0;
    if (strcmp (sample_type, "1-BIT") == 0
	&& strcmp (pixel_type, "MONOCHROME") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1);
    if (strcmp (sample_type, "1-BIT") == 0
	&& strcmp (pixel_type, "PALETTE") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_1_BIT, RL2_PIXEL_PALETTE, 1);
    if (strcmp (sample_type, "2-BIT") == 0
	&& strcmp (pixel_type, "PALETTE") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_2_BIT, RL2_PIXEL_PALETTE, 1);
    if (strcmp (sample_type, "4-BIT") == 0
	&& strcmp (pixel_type, "PALETTE") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_4_BIT, RL2_PIXEL_PALETTE, 1);
    if (strcmp (sample_type, "UINT8") == 0
	&& strcmp (pixel_type, "PALETTE") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT8, RL2_PIXEL_PALETTE, 1);
    if (strcmp (sample_type, "UINT8") == 0
	&& strcmp (pixel_type, "GRAYSCALE") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
    if (strcmp (sample_type, "UINT8") == 0
	&& strcmp (pixel_type, "RGB") == 0 && num_bands == 3)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
    if (strcmp (sample_type, "UINT16") == 0
	&& strcmp (pixel_type, "RGB") == 0 && num_bands == 3)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT16, RL2_PIXEL_RGB, 3);
    if (strcmp (sample_type, "UINT8") == 0
	&& strcmp (pixel_type, "MULTIBAND") == 0 && num_bands >= 2
	&& num_bands < 255)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT8, RL2_PIXEL_MULTIBAND, num_bands);
    if (strcmp (sample_type, "UINT16") == 0
	&& strcmp (pixel_type, "MULTIBAND") == 0 && num_bands >= 2
	&& num_bands < 255)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT16, RL2_PIXEL_MULTIBAND, num_bands);
    if (strcmp (sample_type, "INT8") == 0
	&& strcmp (pixel_type, "DATAGRID") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_INT8, RL2_PIXEL_DATAGRID, 1);
    if (strcmp (sample_type, "UINT8") == 0
	&& strcmp (pixel_type, "DATAGRID") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT8, RL2_PIXEL_DATAGRID, 1);
    if (strcmp (sample_type, "INT16") == 0
	&& strcmp (pixel_type, "DATAGRID") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_INT16, RL2_PIXEL_DATAGRID, 1);
    if (strcmp (sample_type, "UINT16") == 0
	&& strcmp (pixel_type, "DATAGRID") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT16, RL2_PIXEL_DATAGRID, 1);
    if (strcmp (sample_type, "INT32") == 0
	&& strcmp (pixel_type, "DATAGRID") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_INT32, RL2_PIXEL_DATAGRID, 1);
    if (strcmp (sample_type, "UINT32") == 0
	&& strcmp (pixel_type, "DATAGRID") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_UINT32, RL2_PIXEL_DATAGRID, 1);
    if (strcmp (sample_type, "FLOAT") == 0
	&& strcmp (pixel_type, "DATAGRID") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_FLOAT, RL2_PIXEL_DATAGRID, 1);
    if (strcmp (sample_type, "DOUBLE") == 0
	&& strcmp (pixel_type, "DATAGRID") == 0 && num_bands == 1)
	lyr =
	    alloc_wms_layer (coverage_name, title, abstract, srid,
			     has_flipped_axes, minx, miny, maxx, maxy,
			     RL2_SAMPLE_DOUBLE, RL2_PIXEL_DATAGRID, 1);
    return lyr;
}

static struct wms_list *
get_raster_coverages (sqlite3 * handle)
{
/* preparing a list of available Raster Coverages */
    struct wms_list *list = NULL;
    int ret;
    sqlite3_stmt *stmt;
    const char *sql;

/* loading all layers */
    sql = "SELECT coverage_name, title, abstract, sample_type, "
	"pixel_type, num_bands, srid, extent_minx, extent_miny, "
	"extent_maxx, extent_maxy, compression "
	"FROM raster_coverages "
	"WHERE extent_minx IS NOT NULL AND extent_miny IS NOT NULL "
	"AND extent_maxx IS NOT NULL AND extent_maxy IS NOT NULL";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return NULL;
    list = alloc_wms_list ();
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		struct wms_layer *lyr = load_layer (handle, stmt);
		if (lyr != NULL)
		  {
		      if (list->first_layer == NULL)
			  list->first_layer = lyr;
		      if (list->last_layer != NULL)
			  list->last_layer->next = lyr;
		      list->last_layer = lyr;
		  }
	    }
      }
    sqlite3_finalize (stmt);

/* loading layer groups */
    sql = "SELECT g.group_name, g.title, g.abstract, c.raster_coverage_name "
	"FROM SE_styled_groups AS g "
	"LEFT JOIN  SE_styled_group_refs AS c ON (g.group_name = c.group_name) "
	"ORDER BY c.paint_order";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		struct wms_group *grp = NULL;
		struct wms_layer_ref *ref = NULL;
		struct wms_layer *lyr = NULL;
		const char *group_name =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *title =
		    (const char *) sqlite3_column_text (stmt, 1);
		const char *abstract =
		    (const char *) sqlite3_column_text (stmt, 2);
		const char *coverage_name =
		    (const char *) sqlite3_column_text (stmt, 3);
		grp = list->first_group;
		while (grp != NULL)
		  {
		      if (strcmp (grp->group_name, group_name) == 0)
			  goto group_found;
		      grp = grp->next;
		  }
		grp = alloc_wms_group (group_name, title, abstract);
		if (list->first_group == NULL)
		    list->first_group = grp;
		if (list->last_group != NULL)
		    list->last_group->next = grp;
		list->last_group = grp;
	      group_found:
		lyr = list->first_layer;
		while (lyr != NULL)
		  {
		      if (strcmp (lyr->layer_name, coverage_name) == 0)
			{
			    /* child layer match */
			    lyr->child_layer = 1;
			    ref = alloc_wms_layer_ref (lyr);
			    break;
			}
		      lyr = lyr->next;
		  }
		if (ref != NULL)
		  {
		      if (grp->first_child == NULL)
			  grp->first_child = ref;
		      if (grp->last_child != NULL)
			  grp->last_child->next = ref;
		      grp->last_child = ref;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    if (list->first_layer == NULL && list->first_group == NULL)
	goto error;

    return list;

  error:
    if (list != NULL)
	destroy_wms_list (list);
    return NULL;
}

static void
get_raster_styles (sqlite3 * handle, struct wms_list *list)
{
/* retrieving all declared Raster Styles */
    int ret;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT coverage_name, name, title, abstract "
	"FROM SE_raster_styled_layers_view " "ORDER BY coverage_name, style_id";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *title = NULL;
		const char *abstract = NULL;
		const char *coverage_name =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *name = (const char *) sqlite3_column_text (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_TEXT)
		    title = (const char *) sqlite3_column_text (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_TEXT)
		    abstract = (const char *) sqlite3_column_text (stmt, 3);
		add_style_to_wms_layer (list, coverage_name, name, title,
					abstract);
	    }
      }
    sqlite3_finalize (stmt);
    add_default_styles (list);
}

static void
get_group_styles (sqlite3 * handle, struct wms_list *list)
{
/* retrieving all declared Group Styles */
    int ret;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT group_name, name, title, abstract "
	"FROM SE_group_styles_view " "ORDER BY group_name, style_id";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *title = NULL;
		const char *abstract = NULL;
		const char *group_name =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *name = (const char *) sqlite3_column_text (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_TEXT)
		    title = (const char *) sqlite3_column_text (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_TEXT)
		    abstract = (const char *) sqlite3_column_text (stmt, 3);
		add_style_to_wms_group (list, group_name, name, title,
					abstract);
	    }
      }
    sqlite3_finalize (stmt);
    add_default_group_styles (list);
}

static void
open_db (const char *path, sqlite3 ** handle, int cache_size, void *cache)
{
/* opening the DB */
    sqlite3 *db_handle;
    int ret;
    char sql[1024];

    *handle = NULL;
    fprintf (stderr,
	     "\n======================================================\n");
    fprintf (stderr, "              WmsLite server startup\n");
    fprintf (stderr,
	     "======================================================\n");
    fprintf (stderr, "         SQLite version: %s\n", sqlite3_libversion ());
    fprintf (stderr, "     SpatiaLite version: %s\n", spatialite_version ());
    fprintf (stderr, "    RasterLite2 version: %s\n", rl2_version ());
    fprintf (stderr,
	     "======================================================\n");
/* enabling the SQLite's shared cache */
    ret = sqlite3_enable_shared_cache (1);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "unable to enable SQLite's shared cache: ERROR %d\n",
		   ret);
	  sqlite3_close (db_handle);
	  return;
      }

/* opening the main READ-WRITE connection */
    ret =
	sqlite3_open_v2 (path, &db_handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX |
			 SQLITE_OPEN_SHAREDCACHE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open '%s': %s\n", path,
		   sqlite3_errmsg (db_handle));
	  sqlite3_close (db_handle);
	  return;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, 0);
/* enabling WAL journaling */
    sprintf (sql, "PRAGMA journal_mode=WAL");
    sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (cache_size > 0)
      {
	  /* setting the CACHE-SIZE */
	  sprintf (sql, "PRAGMA cache_size=%d", cache_size);
	  sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
      }

    *handle = db_handle;
    return;
}

static void
do_help ()
{
/* printing the argument list */
    fprintf (stderr, "\n\nusage: wmslite ARGLIST ]\n");
    fprintf (stderr,
	     "==============================================================\n");
    fprintf (stderr, "-db or --db-path      pathname  RasterLite2 DB path\n");
    fprintf (stderr,
	     "-p or --ip-port       number    IP port number [default: 8080]\n\n");
    fprintf (stderr,
	     "-cs or --cache-size    num      DB cache size (how many pages)\n");
    fprintf (stderr,
	     "-dbg or --debug                 verbose debugginh mode\n");
}

int
main (int argc, char *argv[])
{
/* the MAIN function simply perform arguments checking */
    struct neutral_socket skt_ptr;
    sqlite3 *handle;
    sqlite3_stmt *stmt_log = NULL;
    const char *sql;
    int ret;
    int i;
    int error = 0;
    int next_arg = ARG_NONE;
    const char *db_path = NULL;
    int port_no = 8080;
    int cache_size = 0;
    void *cache;
    struct wms_list *list = NULL;
    struct connections_pool *pool;
    struct server_log *log;
    char *cached_capabilities = NULL;
    int cached_capabilities_len = 0;

/* installing the signal handlers */
    glob.handle = NULL;
    glob.stmt_log = NULL;
    glob.cached_capabilities = NULL;
    glob.log = NULL;
    glob.list = NULL;
    glob.pool = NULL;
    glob.cache = NULL;
#ifdef _WIN32
    SetConsoleCtrlHandler (signal_handler, TRUE);
#else
    signal (SIGINT, signal_handler);
    signal (SIGTERM, signal_handler);
#endif

    for (i = 1; i < argc; i++)
      {
	  /* parsing the invocation arguments */
	  if (next_arg != ARG_NONE)
	    {
		switch (next_arg)
		  {
		  case ARG_DB_PATH:
		      db_path = argv[i];
		      break;
		  case ARG_IP_PORT:
		      port_no = atoi (argv[i]);
		      break;
		  case ARG_CACHE_SIZE:
		      cache_size = atoi (argv[i]);
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
	  if (strcmp (argv[i], "-db") == 0
	      || strcasecmp (argv[i], "--db-path") == 0)
	    {
		next_arg = ARG_DB_PATH;
		continue;
	    }
	  if (strcmp (argv[i], "-p") == 0
	      || strcasecmp (argv[i], "--ip-port") == 0)
	    {
		next_arg = ARG_IP_PORT;
		continue;
	    }
	  if (strcasecmp (argv[i], "--cache-size") == 0
	      || strcmp (argv[i], "-cs") == 0)
	    {
		next_arg = ARG_CACHE_SIZE;
		continue;
	    }
	  if (strcasecmp (argv[i], "--debug") == 0
	      || strcmp (argv[i], "-dbg") == 0)
	    {
		debug_mode = 1;
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

/* checking the arguments */
    if (db_path == NULL)
      {
	  fprintf (stderr, "did you forget to specify the --db-path arg ?\n");
	  error = 1;
      }
    if (error)
      {
	  do_help ();
	  return -1;
      }

/* opening the DB */
    cache = spatialite_alloc_connection ();
    glob.cache = cache;
    open_db (db_path, &handle, cache_size, cache);
    if (!handle)
	return -1;
    glob.handle = handle;

    list = get_raster_coverages (handle);
    if (list == NULL)
      {
	  fprintf (stderr,
		   "the DB \"%s\" doesn't contain any valid Raster Coverage\n",
		   db_path);
	  goto stop;
      }

    get_raster_styles (handle, list);
    get_group_styles (handle, list);
    glob.list = list;
    compute_geographic_extents (handle, list);
    build_get_capabilities (list, &cached_capabilities,
			    &cached_capabilities_len, port_no);
    glob.cached_capabilities = cached_capabilities;

/* creating the read connections pool */
    pool = alloc_connections_pool (db_path);
    if (pool == NULL)
      {
	  fprintf (stderr, "ERROR: unable to initialize a connections pool\n");
	  goto stop;
      }
    glob.pool = pool;

/* creating the server log helper struct */
    log = alloc_server_log ();
    if (log == NULL)
      {
	  fprintf (stderr, "ERROR: unable to initialize the server log\n");
	  goto stop;
      }
    glob.log = log;

/* starting the HTTP server */
    if (!do_start_http (port_no, &skt_ptr))
	goto stop;

/* starting the logging facility */
    sql = "CREATE TABLE IF NOT EXISTS wms_server_log (\n"
	"\tid INTEGER PRIMARY KEY AUTOINCREMENT,\n"
	"\ttimestamp TEXT NOT NULL,\n"
	"\tclient_ip_addr TEXT NOT NULL,\n"
	"\tclient_ip_port INTEGER NOT NULL,\n"
	"\thttp_method TEXT NOT NULL,\n"
	"\trequest_url TEXT NOT NULL,\n"
	"\thttp_status INTEGER NOT NULL,\n"
	"\tresponse_length INTEGER NOT NULL,\n"
	"\twms_request TEXT,\n"
	"\twms_version TEXT,\n"
	"\twms_layer TEXT,\n"
	"\twms_srid INTEGER,\n"
	"\twms_bbox_minx DOUBLE,\n"
	"\twms_bbox_miny DOUBLE,\n"
	"\twms_bbox_maxx DOUBLE,\n"
	"\twms_bbox_maxy DOUBLE,\n"
	"\twms_width INTEGER,\n"
	"\twms_height INTEGER,\n"
	"\twms_style TEXT,\n"
	"\twms_format TEXT,\n"
	"\twms_transparent INTEGER,\n"
	"\twms_bgcolor TEXT,\n" "\tmilliseconds INTEGER)";
    sqlite3_exec (handle, sql, NULL, NULL, NULL);

    sql = "INSERT INTO wms_server_log (id, timestamp, client_ip_addr, "
	"client_ip_port, http_method, request_url, http_status, "
	"response_length, wms_request, wms_version, wms_layer, "
	"wms_srid, wms_bbox_minx, wms_bbox_miny, wms_bbox_maxx, "
	"wms_bbox_maxy, wms_width, wms_height, wms_style, "
	"wms_format, wms_transparent, wms_bgcolor, milliseconds) "
	"VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_log, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("INSERT INTO LOG error: %s\n", sqlite3_errmsg (handle));
	  goto stop;
      }
    glob.stmt_log = stmt_log;

/* looping on requests */
    do_accept_loop (&skt_ptr, list, port_no, handle, stmt_log, pool, log,
		    cached_capabilities, cached_capabilities_len);

  stop:
    destroy_wms_list (list);
    list = NULL;
    glob.list = NULL;
    clean_shutdown ();
    return 0;
}
