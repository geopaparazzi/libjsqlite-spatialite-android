/*

 virtualgeojson.c -- SQLite3 extension [VIRTUAL TABLE GeoJson]

 version 5.0, 2018 December 12

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
 
Portions created by the Initial Developer are Copyright (C) 2016
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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite/gaiaaux.h>
#include <spatialite.h>
#include <spatialite/spatialite.h>
#include <spatialite/geojson.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

static struct sqlite3_module my_geojson_module;

typedef struct VirtualGeoJsonStruct
{
/* extends the sqlite3_vtab struct */
    const sqlite3_module *pModule;	/* ptr to sqlite module: USED INTERNALLY BY SQLITE */
    int nRef;			/* # references: USED INTERNALLY BY SQLITE */
    char *zErrMsg;		/* error message: USE INTERNALLY BY SQLITE */
    sqlite3 *db;		/* the sqlite db holding the virtual table */
    int Srid;			/* the GeoJSON SRID */
    char *TableName;		/* the VirtualTable name */
    int Valid;			/* validity flag */
    geojson_parser_ptr Parser;	/* the GeoJSON Parser */
    int DeclaredType;		/* Geometry DeclaredType */
    int DimensionModel;		/* Geometry Dimension Model */
    double MinX;		/* the GeoJSON Full Extent */
    double MinY;
    double MaxX;
    double MaxY;
} VirtualGeoJson;
typedef VirtualGeoJson *VirtualGeoJsonPtr;

typedef struct VirtualGeoJsonConstraintStruct
{
/* a constraint to be verified for xFilter */
    int iColumn;		/* Column on left-hand side of constraint */
    int op;			/* Constraint operator */
    char valueType;		/* value Type (GEOJSON_TEXT, GEOJSON_INTEGER, GEOJSON_DOUBLE etc) */
    sqlite3_int64 intValue;	/* Int64 comparison value */
    double dblValue;		/* Double comparison value */
    char *txtValue;		/* Text comparison value */
    struct VirtualGeoJsonConstraintStruct *next;
} VirtualGeoJsonConstraint;
typedef VirtualGeoJsonConstraint *VirtualGeoJsonConstraintPtr;

typedef struct VirtualGeoJsonCursorStruct
{
/* extends the sqlite3_vtab_cursor struct */
    VirtualGeoJsonPtr pVtab;	/* Virtual table of this cursor */
    int current_fid;		/* the current row FID */
    geojson_feature_ptr Feature;	/* pointer to the current Feature */
    int eof;			/* the EOF marker */
    VirtualGeoJsonConstraintPtr firstConstraint;
    VirtualGeoJsonConstraintPtr lastConstraint;
} VirtualGeoJsonCursor;
typedef VirtualGeoJsonCursor *VirtualGeoJsonCursorPtr;


static int
geojson_is_float (const char *buf)
{
/* checking for a possible floating point number */
    int ok = 0;
    int err = 0;
    unsigned int i;
    for (i = 0; i < strlen (buf); i++)
      {
	  if (i == 0 && (*(buf + i) == '-' || *(buf + i) == '+'))
	      continue;
	  if (*(buf + i) == '.')
	    {
		ok++;
		continue;
	    }
	  if (*(buf + i) == 'e' || *(buf + i) == 'E')
	    {
		ok++;
		continue;
	    }
	  if (*(buf + i) >= '0' && *(buf + i) <= '9')
	      ;
	  else
	      err++;
      }
    if (!err && ok == 1)
	return 1;
    return 0;
}

static geojson_property_ptr
geojson_create_property ()
{
/* creating an empty GeoJSON property */
    geojson_property_ptr prop = malloc (sizeof (geojson_property));
    prop->name = NULL;
    prop->type = GEOJSON_UNKNOWN;
    prop->txt_value = NULL;
    prop->next = NULL;
    return prop;
}

static void
geojson_destroy_property (geojson_property_ptr prop)
{
/* destroying a GeoJSON property */
    if (prop->name != NULL)
	free (prop->name);
    if (prop->txt_value != NULL)
	free (prop->txt_value);
    free (prop);
}

static void
geojson_init_property (geojson_property_ptr prop)
{
/* initializing an empty GeoJSON property */
    prop->name = NULL;
    prop->type = GEOJSON_UNKNOWN;
    prop->txt_value = NULL;
    prop->next = NULL;
}

static void
geojson_reset_property (geojson_property_ptr prop)
{
/* resetting a GeoJSON property */
    if (prop->name != NULL)
	free (prop->name);
    if (prop->txt_value != NULL)
	free (prop->txt_value);
    geojson_init_property (prop);
}

static geojson_column_ptr
geojson_create_column (const char *name, int type)
{
/* creating a GeoJSON column object */
    int len;
    geojson_column_ptr col = malloc (sizeof (geojson_column));
    len = strlen (name);
    col->name = malloc (len + 1);
    strcpy (col->name, name);
    col->n_text = 0;
    col->n_int = 0;
    col->n_double = 0;
    col->n_bool = 0;
    col->n_null = 0;
    col->next = NULL;
    switch (type)
      {
      case GEOJSON_TEXT:
	  col->n_text = 1;
	  break;
      case GEOJSON_INTEGER:
	  col->n_int = 1;
	  break;
      case GEOJSON_DOUBLE:
	  col->n_double = 1;
	  break;
      case GEOJSON_TRUE:
      case GEOJSON_FALSE:
	  col->n_bool = 1;
	  break;
      case GEOJSON_NULL:
	  col->n_null = 1;
	  break;
      };
    return col;
}

static geojson_stack_ptr
geojson_create_stack ()
{
/* creating an empty GeoJSON stack object */
    int i;
    geojson_stack_ptr ptr = malloc (sizeof (geojson_stack));
    ptr->level = -1;
    memset (ptr->key, '\0', GEOJSON_MAX);
    ptr->key_idx = 0;
    memset (ptr->value, '\0', GEOJSON_MAX);
    ptr->value_idx = 0;
    memset (ptr->numvalue, '\0', GEOJSON_MAX);
    ptr->numvalue_idx = 0;
    for (i = 0; i < GEOJSON_STACK; i++)
      {
	  geojson_stack_entry_ptr p = ptr->entries + i;
	  p->obj = NULL;
	  p->first = NULL;
	  p->last = NULL;
      }
    return ptr;
}

static void
geojson_destroy_stack (geojson_stack_ptr ptr)
{
/* memory cleanup - destroying a GeoJSON stack object */
    int i;
    if (ptr == NULL)
	return;
    for (i = 0; i < GEOJSON_STACK; i++)
      {
	  geojson_stack_entry_ptr p = ptr->entries + i;
	  geojson_keyval_ptr pkv = p->first;
	  while (pkv != NULL)
	    {
		geojson_keyval_ptr pn = pkv->next;
		if (pkv->key != NULL)
		    free (pkv->key);
		if (pkv->value != NULL)
		    free (pkv->value);
		free (pkv);
		pkv = pn;
	    }
      }
    free (ptr);
}

static int
geojson_parse_key (geojson_stack_ptr stack, char c, char **error_message)
{
/* parsing a GeoJSON Object's Key string */
    if (stack->key_idx >= GEOJSON_MAX - 1)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON Object's Key string: len > %d chars\n",
			       GEOJSON_MAX);
	  return 0;
      }
    *(stack->key + stack->key_idx) = c;
    stack->key_idx += 1;
    return 1;
}

static int
geojson_parse_value (geojson_stack_ptr stack, char c, char **error_message)
{
/* parsing a GeoJSON Object's Value string */
    if (stack->key_idx >= GEOJSON_MAX - 1)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON Object's Value string: len > %d chars\n", GEOJSON_MAX);
	  return 0;
      }
    *(stack->value + stack->value_idx) = c;
    stack->value_idx += 1;
    return 1;
}

static int
geojson_parse_numvalue (geojson_stack_ptr stack, char c, char **error_message)
{
/* parsing a GeoJSON Object's numeric or special Value */
    if (stack->numvalue_idx >= GEOJSON_MAX - 1)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON Object's Numeric Value: len > %d chars\n", GEOJSON_MAX);
	  return 0;
      }
    *(stack->numvalue + stack->numvalue_idx) = c;
    stack->numvalue_idx += 1;
    return 1;
}

static void
geojson_add_keyval (geojson_stack_ptr stack, int level)
{
/* adding a Key-Value item to a GeoJSON Object */
    geojson_stack_entry_ptr entry;
    geojson_keyval_ptr pkv;
    int len;

    if (*(stack->key) == '\0')
	goto reset;
    entry = stack->entries + level;
/* setting the Key name */
    pkv = malloc (sizeof (geojson_keyval));
    len = strlen (stack->key);
    if (len > 0)
      {
	  pkv->key = malloc (len + 1);
	  strcpy (pkv->key, stack->key);
      }
    else
	pkv->key = NULL;
    len = strlen (stack->value);
    if (len > 0)
      {
	  /* setting a string value */
	  pkv->value = malloc (len + 1);
	  strcpy (pkv->value, stack->value);
	  pkv->numvalue = 0;
      }
    else
	pkv->value = NULL;
    if (pkv->value == NULL)
      {
	  len = strlen (stack->numvalue);
	  if (len > 0)
	    {
		/* setting a numeric or special value */
		pkv->value = malloc (len + 1);
		strcpy (pkv->value, stack->numvalue);
		pkv->numvalue = 1;
	    }
      }
    pkv->next = NULL;
/* updating the Key-Value linked list */
    if (entry->first == NULL)
	entry->first = pkv;
    if (entry->last != NULL)
	entry->last->next = pkv;
    entry->last = pkv;

/* resetting the Key-Value input buffers */
  reset:
    memset (stack->key, '\0', GEOJSON_MAX);
    stack->key_idx = 0;
    memset (stack->value, '\0', GEOJSON_MAX);
    stack->value_idx = 0;
    memset (stack->numvalue, '\0', GEOJSON_MAX);
    stack->numvalue_idx = 0;
}

static geojson_block_ptr
geojson_add_block (geojson_parser_ptr parser)
{
/* adding a new block of objects to the GeoJSON parser */
    int i;
    geojson_block_ptr block;
    if (parser == NULL)
	return NULL;
    block = malloc (sizeof (geojson_block));
/* initializing an empty block */
    for (i = 0; i < GEOJSON_BLOCK; i++)
      {
	  geojson_entry_ptr entry = block->entries + i;
	  entry->parent_key = NULL;
	  entry->type = GEOJSON_UNKNOWN;
	  entry->properties = 0;
	  entry->geometry = 0;
	  entry->offset_start = -1;
	  entry->offset_end = -1;
      }
    block->next_free_entry = 0;
    block->next = NULL;
/* appending to the linked list */
    if (parser->first == NULL)
	parser->first = block;
    if (parser->last != NULL)
	parser->last->next = block;
    parser->last = block;
    return block;
}

static void
geojson_add_column (geojson_parser_ptr parser, const char *name, int type)
{
/* updating the Columns list */
    geojson_column_ptr col;

    col = parser->first_col;
    while (col != NULL)
      {
	  if (strcasecmp (col->name, name) == 0)
	    {
		/* already defined: updating stats */
		switch (type)
		  {
		  case GEOJSON_TEXT:
		      col->n_text += 1;
		      break;
		  case GEOJSON_INTEGER:
		      col->n_int += 1;
		      break;
		  case GEOJSON_DOUBLE:
		      col->n_double += 1;
		      break;
		  case GEOJSON_TRUE:
		  case GEOJSON_FALSE:
		      col->n_bool += 1;
		      break;
		  case GEOJSON_NULL:
		      col->n_null += 1;
		      break;
		  };
		return;
	    }
	  col = col->next;
      }

/* inserting a new column into the list */
    col = geojson_create_column (name, type);
    if (parser->first_col == NULL)
	parser->first_col = col;
    if (parser->last_col != NULL)
	parser->last_col->next = col;
    parser->last_col = col;
}

static int
geojson_push (geojson_stack_ptr stack, geojson_entry_ptr entry, int level,
	      char **error_message)
{
/* GeoJSON stack pseudo-push */
    geojson_stack_entry_ptr p_entry;
    if (stack == NULL || entry == NULL)
      {
	  *error_message = sqlite3_mprintf ("GeoJSON push: NULL pointer\n");
	  return 0;
      }
    if (level < 0 || level >= GEOJSON_STACK)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON push: forbidden nesting level %d\n",
			       level);
	  return 0;
      }
    if (level != (stack->level + 1))
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON push: unexpected nesting level %d (%d)\n", level,
	       stack->level);
	  return 0;
      }
    stack->level += 1;
    p_entry = stack->entries + level;
    if (p_entry->obj != NULL)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON push: unexpected unfreed level %d\n",
			       level);
	  return 0;
      }
/* initializing the stack entry */
    p_entry->obj = entry;
    memset (stack->key, '\0', GEOJSON_MAX);
    stack->key_idx = 0;
    memset (stack->value, '\0', GEOJSON_MAX);
    stack->value_idx = 0;
    memset (stack->numvalue, '\0', GEOJSON_MAX);
    stack->numvalue_idx = 0;
    return 1;
}

static int
geojson_pop (geojson_stack_ptr stack, int level, long offset,
	     char **error_message)
{
/* GeoJSON stack pseudo-pop */
    geojson_stack_entry_ptr p_entry;
    geojson_keyval_ptr pkv;
    if (level < 0 || level >= GEOJSON_STACK)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON pop: forbidden nesting level %d\n",
			       level);
	  return 0;
      }
    if (level != stack->level)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON pop: unexpected nesting level %d (%d)\n", level,
	       stack->level);
	  return 0;
      }
    p_entry = stack->entries + level;
    if (p_entry->obj == NULL)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON pop: unexpected uninitialized level %d\n", level);
	  return 0;
      }
    p_entry->obj->offset_end = offset;
    if (strcasecmp (p_entry->obj->parent_key, "properties") == 0)
	p_entry->obj->type = GEOJSON_PROPERTIES;
    pkv = p_entry->first;
    if (pkv != NULL)
      {
	  if (pkv->key != NULL)
	    {
		if (strcasecmp (pkv->key, "type") == 0)
		  {
		      if (pkv->value != NULL)
			{
			    if (strcasecmp (pkv->value, "FeatureCollection") ==
				0)
				p_entry->obj->type = GEOJSON_FCOLLECTION;
			    if (strcasecmp (pkv->value, "Feature") == 0)
				p_entry->obj->type = GEOJSON_FEATURE;
			    if (strcasecmp
				(p_entry->obj->parent_key, "geometry") == 0)
			      {
				  if (strcasecmp (pkv->value, "Point") == 0)
				      p_entry->obj->type = GEOJSON_POINT;
				  if (strcasecmp (pkv->value, "LineString") ==
				      0)
				      p_entry->obj->type = GEOJSON_LINESTRING;
				  if (strcasecmp (pkv->value, "Polygon") == 0)
				      p_entry->obj->type = GEOJSON_POLYGON;
				  if (strcasecmp (pkv->value, "MultiPoint") ==
				      0)
				      p_entry->obj->type = GEOJSON_MULTIPOINT;
				  if (strcasecmp (pkv->value, "MultiLineString")
				      == 0)
				      p_entry->obj->type =
					  GEOJSON_MULTILINESTRING;
				  if (strcasecmp (pkv->value, "MultiPolygon") ==
				      0)
				      p_entry->obj->type = GEOJSON_MULTIPOLYGON;
				  if (strcasecmp
				      (pkv->value, "GeometryCollection") == 0)
				      p_entry->obj->type =
					  GEOJSON_GEOMCOLLECTION;
			      }
			}
		  }
	    }
      }
    if (p_entry->obj->type == GEOJSON_FEATURE)
      {
	  pkv = p_entry->first;
	  while (pkv != NULL)
	    {
		if (strcasecmp (pkv->key, "geometry") == 0
		    && pkv->value == NULL)
		    p_entry->obj->geometry += 1;
		if (strcasecmp (pkv->key, "properties") == 0
		    && pkv->value == NULL)
		    p_entry->obj->properties += 1;
		pkv = pkv->next;
	    }
      }

/* resetting the Key-Value list */
    pkv = p_entry->first;
    while (pkv != NULL)
      {
	  geojson_keyval_ptr pn = pkv->next;
	  if (pkv->key != NULL)
	      free (pkv->key);
	  if (pkv->value != NULL)
	      free (pkv->value);
	  free (pkv);
	  pkv = pn;
      }
    p_entry->first = NULL;
    p_entry->last = NULL;

/* resetting the stack entry */
    p_entry->obj = NULL;
    memset (stack->key, '\0', GEOJSON_MAX);
    stack->key_idx = 0;
    memset (stack->value, '\0', GEOJSON_MAX);
    stack->value_idx = 0;
    memset (stack->numvalue, '\0', GEOJSON_MAX);
    stack->numvalue_idx = 0;
    stack->level -= 1;
    return 1;
}

static geojson_entry_ptr
geojson_add_object (geojson_block_ptr block, long offset,
		    const char *parent_key)
{
/* adding a new GeoJSON object */
    geojson_entry_ptr entry;
    if (block->next_free_entry < 0 || block->next_free_entry >= GEOJSON_BLOCK)
	return NULL;
    entry = block->entries + block->next_free_entry;
    block->next_free_entry += 1;
    entry->type = GEOJSON_UNKNOWN;
    if (entry->parent_key != NULL)
	free (entry->parent_key);
    entry->parent_key = NULL;
    if (parent_key != NULL)
      {
	  int len = strlen (parent_key);
	  entry->parent_key = malloc (len + 1);
	  strcpy (entry->parent_key, parent_key);
      }
    entry->offset_start = offset;
    entry->offset_end = -1;
    return entry;
}


static int
geojson_start_object (geojson_parser_ptr parser, geojson_stack_ptr stack,
		      int level, long offset, const char *parent_key,
		      char **error_message)
{
/* registering a GeoJSON object */
    int expand = 0;
    geojson_block_ptr block;
    geojson_entry_ptr entry;

    if (parser->last == NULL)
	expand = 1;
    else if (parser->last->next_free_entry >= GEOJSON_BLOCK)
	expand = 1;
    if (expand)
      {
	  /* adding a new GeoJSON Block to the parser's list */
	  block = geojson_add_block (parser);
      }
    else
      {
	  /* continuing to insert into the last Block */
	  block = parser->last;
      }
    if (block == NULL)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON start_object: NULL pointer\n");
	  return 0;
      }
    entry = geojson_add_object (block, offset, parent_key);
    if (!geojson_push (stack, entry, level, error_message))
	return 0;
    return 1;
}

static int
geojson_end_object (geojson_stack_ptr stack, int level, long offset,
		    char **error_message)
{
/* completing the definition of a GeoJSON object */
    if (!geojson_pop (stack, level, offset, error_message))
	return 0;
    return 1;
}

SPATIALITE_DECLARE geojson_parser_ptr
geojson_create_parser (FILE * in)
{
/* creating an empty GeoJSON parser object */
    geojson_parser_ptr ptr = malloc (sizeof (geojson_parser));
    ptr->in = in;
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->count = 0;
    ptr->features = NULL;
    ptr->first_col = NULL;
    ptr->last_col = NULL;
    ptr->n_points = 0;
    ptr->n_linestrings = 0;
    ptr->n_polygons = 0;
    ptr->n_mpoints = 0;
    ptr->n_mlinestrings = 0;
    ptr->n_mpolygons = 0;
    ptr->n_geomcolls = 0;
    ptr->n_geom_2d = 0;
    ptr->n_geom_3d = 0;
    ptr->n_geom_4d = 0;
    *(ptr->cast_type) = '\0';
    *(ptr->cast_dims) = '\0';
    return ptr;
}

SPATIALITE_DECLARE void
geojson_destroy_parser (geojson_parser_ptr ptr)
{
/* memory cleanup - destroying a GeoJSON parser object */
    geojson_block_ptr pb;
    geojson_block_ptr pbn;
    geojson_column_ptr pc;
    geojson_column_ptr pcn;
    int i;
    if (ptr == NULL)
	return;
    pb = ptr->first;
    while (pb != NULL)
      {
	  pbn = pb->next;
	  free (pb);
	  pb = pbn;
      }
    pc = ptr->first_col;
    while (pc != NULL)
      {
	  pcn = pc->next;
	  if (pc->name != NULL)
	      free (pc->name);
	  free (pc);
	  pc = pcn;
      }
    if (ptr->features != NULL)
      {
	  for (i = 0; i < ptr->count; i++)
	    {
		geojson_property_ptr pp;
		geojson_feature_ptr pf = ptr->features + i;
		if (pf->geometry != NULL)
		    free (pf->geometry);
		pp = pf->first;
		while (pp != NULL)
		  {
		      geojson_property_ptr ppn = pp->next;
		      if (pp->name != NULL)
			  free (pp->name);
		      if (pp->txt_value != NULL)
			  free (pp->txt_value);
		      free (pp);
		      pp = ppn;
		  }
	    }
	  free (ptr->features);
      }
/* close the GeoJSON file handle */
    if (ptr->in != NULL)
	fclose (ptr->in);
    free (ptr);
}

SPATIALITE_DECLARE int
geojson_parser_init (geojson_parser_ptr parser, char **error_message)
{
/* initializing the GeoJSON parser object */
    int c;
    long offset;
    int level = -1;
    int is_string = 0;
    int prev_char = '\0';
    int is_first = 0;
    int is_second = 0;
    int is_first_ready = 0;
    int is_second_ready = 0;
    int is_numeric = 0;
    geojson_stack_ptr stack = geojson_create_stack ();
    *error_message = NULL;

    while ((c = getc (parser->in)) != EOF)
      {
	  /* consuming the GeoJSON input file */
	  if (is_string)
	    {
		/* consuming a quoted text string */
		if (c == '"' && prev_char != '/')
		  {
		      is_string = 0;	/* end string marker */
		      if (is_first)
			{
			    /* found the GeoJSON object Key terminator */
			    is_first = 0;
			}
		      if (is_second)
			{
			    /* found the GeoJSON object Value terminator */
			    is_second = 0;
			}
		  }
		else
		  {
		      if (is_first)
			{
			    /* found the GeoJSON object Key */
			    if (!geojson_parse_key (stack, c, error_message))
				goto err;
			}
		      if (is_second)
			{
			    /* found the GeoJSON object Value */
			    if (!geojson_parse_value (stack, c, error_message))
				goto err;
			}
		  }
		prev_char = c;
		continue;
	    }
	  if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
	    {
		/* ignoring white spaces */
		prev_char = c;
		continue;
	    }
	  if (c == '[' || c == ']')
	    {
		prev_char = c;
		is_second_ready = 0;
		is_second = 0;
		is_numeric = 0;
		continue;
	    }
	  if (c == '{')
	    {
		/* found a JSON Start Object marker */
		char parent_key[GEOJSON_MAX];
		strcpy (parent_key, stack->key);
		if (level >= 0)
		    geojson_add_keyval (stack, level);
		level++;
		offset = ftell (parser->in);
		if (!geojson_start_object
		    (parser, stack, level, offset, parent_key, error_message))
		    goto err;
		prev_char = c;
		is_first_ready = 1;
		is_first = 0;
		is_second_ready = 0;
		is_second = 0;
		is_numeric = 0;
		continue;
	    }
	  if (c == '}')
	    {
		/* found a JSON End Object marker */
		geojson_add_keyval (stack, level);
		offset = ftell (parser->in);
		if (!geojson_end_object (stack, level, offset, error_message))
		    goto err;
		level--;
		prev_char = c;
		is_first_ready = 0;
		is_first = 0;
		is_second_ready = 0;
		is_second = 0;
		is_numeric = 0;
		continue;
	    }
	  if (c == ':')
	    {
		prev_char = c;
		is_first_ready = 0;
		is_second_ready = 1;
		continue;
	    }
	  if (c == ',')
	    {
		geojson_add_keyval (stack, level);
		prev_char = c;
		is_first_ready = 1;
		is_first = 0;
		is_second_ready = 0;
		is_second = 0;
		is_numeric = 0;
		continue;
	    }
	  if (c == '"')
	    {
		/* a quoted text string starts here */
		is_string = 1;
		prev_char = c;
		if (is_first_ready)
		  {
		      is_first_ready = 0;
		      is_first = 1;
		  }
		if (is_second_ready)
		  {
		      is_second_ready = 0;
		      is_second = 1;
		  }
		continue;
	    }
	  if (is_second_ready)
	    {
		/* should be the beginning of some numeric value */
		is_second_ready = 0;
		is_numeric = 1;
	    }
	  if (is_numeric)
	    {
		/* consuming a numeric or special value */
		if (!geojson_parse_numvalue (stack, c, error_message))
		    goto err;
		prev_char = c;
		continue;
	    }
	  prev_char = c;
      }
    geojson_destroy_stack (stack);
    return 1;

  err:
    geojson_destroy_stack (stack);
    return 0;
}

static int
geojson_get_property (const char *buf, geojson_stack_ptr stack,
		      geojson_property_ptr prop, int *off, char **error_message)
{
/* parsing the Feature's Properties string */
    char c;
    int len;
    int is_string = 0;
    int prev_char = '\0';
    int is_first = 0;
    int is_second = 0;
    int is_first_ready = 1;
    int is_second_ready = 0;
    int is_numeric = 0;
    const char *end_p = buf + strlen (buf);
    const char *p = buf + *off;
    if (p < buf || p >= end_p)
	return -1;		/* the string has been completely parsed */

/* resetting all stack buffers */
    memset (stack->key, '\0', GEOJSON_MAX);
    stack->key_idx = 0;
    memset (stack->value, '\0', GEOJSON_MAX);
    stack->value_idx = 0;
    memset (stack->numvalue, '\0', GEOJSON_MAX);
    stack->numvalue_idx = 0;

    while (1)
      {
	  /* consuming the GeoJSON Properties string */
	  if (p >= end_p)
	      break;
	  c = *p++;
	  if (is_string)
	    {
		/* consuming a quoted text string */
		if (c == '"' && prev_char != '/')
		  {
		      is_string = 0;	/* end string marker */
		      if (is_first)
			{
			    /* found the GeoJSON object Key terminator */
			    is_first = 0;
			}
		      if (is_second)
			{
			    /* found the GeoJSON object Value terminator */
			    is_second = 0;
			}
		  }
		else
		  {
		      if (is_first)
			{
			    /* found the GeoJSON object Key */
			    if (!geojson_parse_key (stack, c, error_message))
				goto err;
			    if (prop->name != NULL)
				free (prop->name);
			    len = strlen (stack->key);
			    if (len == 0)
				prop->name = NULL;
			    else
			      {
				  prop->name = malloc (len + 1);
				  strcpy (prop->name, stack->key);
			      }
			}
		      if (is_second)
			{
			    /* found the GeoJSON object Value */
			    if (!geojson_parse_value (stack, c, error_message))
				goto err;
			    if (prop->txt_value != NULL)
				free (prop->txt_value);
			    prop->txt_value = NULL;
			    len = strlen (stack->value);
			    if (len > 0)
			      {
				  prop->txt_value = malloc (len + 1);
				  strcpy (prop->txt_value, stack->value);
			      }
			    prop->type = GEOJSON_TEXT;
			}
		  }
		prev_char = c;
		continue;
	    }
	  if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
	    {
		/* ignoring white spaces */
		prev_char = c;
		continue;
	    }
	  if (c == ':')
	    {
		prev_char = c;
		is_first = 0;
		is_first_ready = 0;
		is_second_ready = 1;
		continue;
	    }
	  if (c == ',')
	      break;
	  if (c == '"')
	    {
		/* a quoted text string starts here */
		is_string = 1;
		prev_char = c;
		if (is_first_ready)
		  {
		      is_first_ready = 0;
		      is_first = 1;
		  }
		if (is_second_ready)
		  {
		      is_second_ready = 0;
		      is_second = 1;
		  }
		continue;
	    }
	  if (is_second_ready)
	    {
		/* should be the beginning of some numeric value */
		is_second_ready = 0;
		is_numeric = 1;
	    }
	  if (is_numeric)
	    {
		/* consuming a numeric or special value */
		if (!geojson_parse_numvalue (stack, c, error_message))
		    goto err;
		prev_char = c;
		continue;
	    }
	  prev_char = c;
      }
    if (is_numeric)
      {
	  if (strcmp (stack->numvalue, "null") == 0)
	      prop->type = GEOJSON_NULL;
	  else if (strcmp (stack->numvalue, "true") == 0)
	      prop->type = GEOJSON_TRUE;
	  else if (strcmp (stack->numvalue, "false") == 0)
	      prop->type = GEOJSON_FALSE;
	  else
	    {
		len = strlen (stack->numvalue);
		if (len > 0)
		  {
		      if (geojson_is_float (stack->numvalue))
			{
			    prop->dbl_value = atof (stack->numvalue);
			    prop->type = GEOJSON_DOUBLE;
			}
		      else
			{
			    prop->int_value = atoll (stack->numvalue);
			    prop->type = GEOJSON_INTEGER;
			}
		  }
	    }
      }

    *off = p - buf;
    return 1;

  err:
    return 0;
}

static int
geojson_parse_columns (geojson_parser_ptr parser, const char *buf,
		       char **error_message)
{
/* attempting to parse Feature's Properties for detecting Column types */
    int off = 0;
    geojson_stack_ptr stack = geojson_create_stack ();
    geojson_property prop;
    geojson_init_property (&prop);

    while (1)
      {
	  int ret;
	  geojson_reset_property (&prop);
	  ret = geojson_get_property (buf, stack, &prop, &off, error_message);
	  if (ret <= 0)
	      geojson_reset_property (&prop);
	  if (ret < 0)
	      break;
	  if (ret == 0)
	      goto err;
	  if (prop.name != NULL)
	    {
		switch (prop.type)
		  {
		  case GEOJSON_TEXT:
		  case GEOJSON_INTEGER:
		  case GEOJSON_DOUBLE:
		  case GEOJSON_TRUE:
		  case GEOJSON_FALSE:
		  case GEOJSON_NULL:
		      geojson_add_column (parser, prop.name, prop.type);
		      break;
		  default:
		      goto err;
		  };
	    }
	  else
	      goto err;
	  geojson_reset_property (&prop);
      }
    geojson_destroy_stack (stack);
    return 1;

  err:
    geojson_destroy_stack (stack);
    return 0;
}

static int
geojson_parse_properties (geojson_feature_ptr ft, const char *buf,
			  char **error_message)
{
/* attempting to parse Feature's Properties for loading Values */
    int off = 0;
    geojson_stack_ptr stack = geojson_create_stack ();
    geojson_property_ptr prop;

    while (1)
      {
	  int ret;
	  prop = geojson_create_property ();
	  ret = geojson_get_property (buf, stack, prop, &off, error_message);
	  if (ret <= 0)
	      geojson_destroy_property (prop);
	  if (ret < 0)
	      break;
	  if (ret == 0)
	      goto err;
	  if (prop->name != NULL)
	    {
		switch (prop->type)
		  {
		  case GEOJSON_TEXT:
		  case GEOJSON_INTEGER:
		  case GEOJSON_DOUBLE:
		  case GEOJSON_TRUE:
		  case GEOJSON_FALSE:
		  case GEOJSON_NULL:
		      /* adding a Property into the linked list */
		      if (ft->first == NULL)
			  ft->first = prop;
		      if (ft->last != NULL)
			  ft->last->next = prop;
		      ft->last = prop;
		      break;
		  default:
		      geojson_destroy_property (prop);
		      goto err;
		  };
	    }
	  else
	    {
		geojson_destroy_property (prop);
		goto err;
	    }
      }
    geojson_destroy_stack (stack);
    return 1;

  err:
    geojson_destroy_stack (stack);
    return 0;
}

SPATIALITE_DECLARE int
geojson_check_features (geojson_parser_ptr parser, char **error_message)
{
/* checking Features for validity */
    int i;
    int len;
    char *buf;
    gaiaGeomCollPtr geo = NULL;
    *error_message = NULL;

    if (parser == NULL)
      {
	  *error_message = sqlite3_mprintf ("GeoJSON parser: NULL object\n");
	  return 0;
      }
    parser->n_points = 0;
    parser->n_linestrings = 0;
    parser->n_polygons = 0;
    parser->n_mpoints = 0;
    parser->n_mlinestrings = 0;
    parser->n_mpolygons = 0;
    parser->n_geomcolls = 0;
    parser->n_geom_2d = 0;
    parser->n_geom_3d = 0;
    parser->n_geom_4d = 0;
    *(parser->cast_type) = '\0';
    *(parser->cast_dims) = '\0';
    for (i = 0; i < parser->count; i++)
      {
	  /* reading and parsing Properties for each Feature */
	  int ret;
	  geojson_feature_ptr ft = parser->features + i;
	  if (ft->prop_offset_start < 0 || ft->prop_offset_end < 0)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: invalid Properties (fid=%d)\n", ft->fid);
		return 0;
	    }
	  if (ft->prop_offset_end <= ft->prop_offset_start)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: invalid Properties (fid=%d)\n", ft->fid);
		return 0;
	    }
	  ret = fseek (parser->in, ft->prop_offset_start, SEEK_SET);
	  if (ret != 0)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: Properties invalid seek (fid=%d)\n",
		     ft->fid);
		return 0;
	    }
	  len = ft->prop_offset_end - ft->prop_offset_start - 1;
	  buf = malloc (len + 1);
	  if (buf == NULL)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: Properties insufficient memory (fid=%d)\n",
		     ft->fid);
		return 0;
	    }
	  ret = fread (buf, 1, len, parser->in);
	  if (ret != len)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: Properties read error (fid=%d)\n",
		     ft->fid);
		free (buf);
		return 0;
	    }
	  *(buf + len) = '\0';
	  geojson_parse_columns (parser, buf, error_message);
	  free (buf);
      }
    for (i = 0; i < parser->count; i++)
      {
	  /* reading and parsing Geometry for each Feature */
	  int ret;
	  geojson_feature_ptr ft = parser->features + i;
	  if (ft->geom_offset_start < 0 || ft->geom_offset_end < 0)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: invalid Geometry (fid=%d)\n", ft->fid);
		return 0;
	    }
	  if (ft->geom_offset_end <= ft->geom_offset_start)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: invalid Geometry (fid=%d)\n", ft->fid);
		return 0;
	    }
	  ret = fseek (parser->in, ft->geom_offset_start, SEEK_SET);
	  if (ret != 0)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: Geometry invalid seek (fid=%d)\n",
		     ft->fid);
		return 0;
	    }
	  len = ft->geom_offset_end - ft->geom_offset_start;
	  if (len == 0)
	    {
		/* NULL Geometry */
		parser->n_geom_null += 1;
		continue;
	    }
	  buf = malloc (len + 2);
	  if (buf == NULL)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: Geometry insufficient memory (fid=%d)\n",
		     ft->fid);
		return 0;
	    }
	  *buf = '{';
	  ret = fread (buf + 1, 1, len, parser->in);
	  if (ret != len)
	    {
		*error_message =
		    sqlite3_mprintf
		    ("GeoJSON parser: Geometry read error (fid=%d)\n", ft->fid);
		free (buf);
		return 0;
	    }
	  *(buf + len + 1) = '\0';
	  geo = gaiaParseGeoJSON ((const unsigned char *) buf);
	  if (geo != NULL)
	    {
		/* sniffing GeometryType and Dimensions */
		switch (geo->DimensionModel)
		  {
		  case GAIA_XY:
		      parser->n_geom_2d += 1;
		      break;
		  case GAIA_XY_Z:
		      parser->n_geom_3d += 1;
		      break;
		  case GAIA_XY_Z_M:
		      parser->n_geom_4d += 1;
		      break;
		  default:
		      *error_message =
			  sqlite3_mprintf
			  ("GeoJSON parser: Geometry has invalid dimensions (fid=%d)\n",
			   ft->fid);
		      free (buf);
		      gaiaFreeGeomColl (geo);
		      return 0;
		  };
		switch (geo->DeclaredType)
		  {
		  case GAIA_POINT:
		      parser->n_points += 1;
		      break;
		  case GAIA_LINESTRING:
		      parser->n_linestrings += 1;
		      break;
		  case GAIA_POLYGON:
		      parser->n_polygons += 1;
		      break;
		  case GAIA_MULTIPOINT:
		      parser->n_mpoints += 1;
		      break;
		  case GAIA_MULTILINESTRING:
		      parser->n_mlinestrings += 1;
		      break;
		  case GAIA_MULTIPOLYGON:
		      parser->n_mpolygons += 1;
		      break;
		  case GAIA_GEOMETRYCOLLECTION:
		      parser->n_geomcolls += 1;
		      break;
		  default:
		      *error_message =
			  sqlite3_mprintf
			  ("GeoJSON parser: Geometry has an invalid Type (fid=%d)\n",
			   ft->fid);
		      free (buf);
		      gaiaFreeGeomColl (geo);
		      return 0;
		  };
		gaiaFreeGeomColl (geo);
	    }
	  else
	      parser->n_geom_null += 1;
	  free (buf);
      }
    return 1;
}

SPATIALITE_DECLARE int
geojson_create_features_index (geojson_parser_ptr parser, char **error_message)
{
/* creating the index of all GeoJSON Features */
    geojson_block_ptr pb;
    geojson_block_ptr pbn;
    geojson_feature_ptr pf = NULL;
    geojson_entry_ptr entry;
    int count;
    int i;
    *error_message = NULL;

    if (parser == NULL)
      {
	  *error_message = sqlite3_mprintf ("GeoJSON parser: NULL object\n");
	  return 0;
      }

    parser->count = 0;
    pb = parser->first;
    while (pb != NULL)
      {
	  /* counting how many Features are there */
	  for (i = 0; i < pb->next_free_entry; i++)
	    {
		entry = pb->entries + i;
		if (entry->type != GEOJSON_FEATURE)
		    continue;
		parser->count += 1;
	    }
	  pb = pb->next;
      }
    if (parser->features != NULL)
	free (parser->features);
/* allocating the index of all Features */
    if (parser->count <= 0)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON parser: not a single Feature was found ... invalid format ?\n");
	  return 0;
      }
    parser->features = malloc (sizeof (geojson_feature) * parser->count);
    if (parser->features == NULL)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON parser: insufficient memory\n");
	  return 0;
      }
    for (i = 0; i < parser->count; i++)
      {
	  /* initializing empty Features */
	  pf = parser->features + i;
	  pf->fid = i + 1;
	  pf->geom_offset_start = -1;
	  pf->geom_offset_end = -1;
	  pf->prop_offset_start = -1;
	  pf->prop_offset_end = -1;
	  pf->geometry = NULL;
	  pf->first = NULL;
	  pf->last = NULL;
      }

    count = 0;
    pb = parser->first;
    while (pb != NULL)
      {
	  /* properly initializing Features */
	  for (i = 0; i < pb->next_free_entry; i++)
	    {
		entry = pb->entries + i;
		if (entry->type != GEOJSON_FEATURE)
		  {
		      if (pf != NULL)
			{
			    if (entry->type == GEOJSON_POINT ||
				entry->type == GEOJSON_LINESTRING ||
				entry->type == GEOJSON_POLYGON ||
				entry->type == GEOJSON_MULTIPOINT ||
				entry->type == GEOJSON_MULTILINESTRING ||
				entry->type == GEOJSON_MULTIPOLYGON ||
				entry->type == GEOJSON_GEOMCOLLECTION)
			      {
				  pf->geom_offset_start = entry->offset_start;
				  pf->geom_offset_end = entry->offset_end;
			      }
			    if (entry->type == GEOJSON_PROPERTIES)
			      {
				  pf->prop_offset_start = entry->offset_start;
				  pf->prop_offset_end = entry->offset_end;
			      }
			}
		      continue;
		  }
		pf = parser->features + count;
		count += 1;
	    }
	  pb = pb->next;
      }

/* destroying the Pass I dictionary */
    pb = parser->first;
    while (pb != NULL)
      {
	  for (i = 0; i < pb->next_free_entry; i++)
	    {
		geojson_entry_ptr e = pb->entries + i;
		if (e->parent_key != NULL)
		    free (e->parent_key);
	    }
	  pbn = pb->next;
	  free (pb);
	  pb = pbn;
      }
    parser->first = NULL;
    parser->last = NULL;
    return 1;
}

static char *
geojson_normalize_case (const char *name, int colname_case)
{
/* transforming a name in full lower- or upper-case */
    int len = strlen (name);
    char *clean = malloc (len + 1);
    char *p = clean;
    strcpy (clean, name);
    while (*p != '\0')
      {
	  if (colname_case == GAIA_DBF_COLNAME_LOWERCASE)
	    {
		if (*p >= 'A' && *p <= 'Z')
		    *p = *p - 'A' + 'a';
	    }
	  if (colname_case == GAIA_DBF_COLNAME_UPPERCASE)
	    {
		if (*p >= 'a' && *p <= 'z')
		    *p = *p - 'a' + 'A';
	    }
	  p++;
      }
    return clean;
}

static char *
geojson_unique_pk (geojson_parser_ptr parser, const char *pk_base_name)
{
/* will return a surely unique PK name */
    int ok = 0;
    int idx = 0;
    char *pk = sqlite3_mprintf ("%s", pk_base_name);
    while (ok == 0)
      {
	  geojson_column_ptr col = parser->first_col;
	  ok = 1;
	  while (col != NULL)
	    {
		if (strcasecmp (pk, col->name) == 0)
		  {
		      sqlite3_free (pk);
		      pk = sqlite3_mprintf ("%s_%d", pk_base_name, idx++);
		      ok = 0;
		      break;
		  }
		col = col->next;
	    }
      }
    return pk;
}

static char *
geojson_unique_geom (geojson_parser_ptr parser, const char *geom_base_name)
{
/* will return a surely unique PK name */
    int ok = 0;
    int idx = 0;
    char *geom = sqlite3_mprintf ("%s", geom_base_name);
    while (ok == 0)
      {
	  geojson_column_ptr col = parser->first_col;
	  ok = 1;
	  while (col != NULL)
	    {
		if (strcasecmp (geom, col->name) == 0)
		  {
		      sqlite3_free (geom);
		      geom = sqlite3_mprintf ("%s_%d", geom_base_name, idx++);
		      ok = 0;
		      break;
		  }
		col = col->next;
	    }
      }
    return geom;
}

static char *
geojson_sql_create_virtual_table (geojson_parser_ptr parser, const char *table,
				  int colname_case)
{
/* will return the SQL CREATE TABLE statement */
    char *sql;
    char *prev;
    char *xname;
    char *pk_name;
    char *geom_name;
    char *xpk_name;
    char *xgeom_name;
    const char *type;
    geojson_column_ptr col;
    if (table == NULL)
	return NULL;

    xname = gaiaDoubleQuotedSql (table);
    pk_name = geojson_unique_pk (parser, "fid");
    xpk_name = geojson_normalize_case (pk_name, colname_case);
    sqlite3_free (pk_name);
    geom_name = geojson_unique_geom (parser, "geometry");
    xgeom_name = geojson_normalize_case (geom_name, colname_case);
    sqlite3_free (geom_name);
    sql =
	sqlite3_mprintf
	("CREATE TABLE \"%s\" (\n\t%s INTEGER PRIMARY KEY AUTOINCREMENT,\n\t%s BLOB",
	 xname, xpk_name, xgeom_name);
    free (xname);
    free (xpk_name);
    free (xgeom_name);
    col = parser->first_col;
    while (col != NULL)
      {
	  /* adding columns */
	  char *xcol = geojson_normalize_case (col->name, colname_case);
	  xname = gaiaDoubleQuotedSql (xcol);
	  free (xcol);
	  type = "TEXT";
	  if (col->n_null > 0)
	    {
		/* NULL values */
		if (col->n_text > 0 && col->n_int == 0 && col->n_double == 0
		    && col->n_bool == 0)
		    type = "TEXT";
		if (col->n_text == 0 && col->n_int > 0 && col->n_double == 0
		    && col->n_bool == 0)
		    type = "INTEGER";
		if (col->n_text == 0 && (col->n_int > 0 && col->n_bool > 0)
		    && col->n_double == 0)
		    type = "INTEGER";
		if (col->n_text == 0 && col->n_int == 0 && col->n_double > 0
		    && col->n_bool == 0)
		    type = "DOUBLE";
		if (col->n_text == 0 && col->n_int == 0 && col->n_double == 0
		    && col->n_bool > 0)
		    type = "BOOLEAN";
	    }
	  else
	    {
		/* NOT NULL */
		if (col->n_text > 0 && col->n_int == 0 && col->n_double == 0
		    && col->n_bool == 0)
		    type = "TEXT NOT NULL";
		if (col->n_text == 0 && col->n_int > 0 && col->n_double == 0
		    && col->n_bool == 0)
		    type = "INTEGER NOT NULL";
		if (col->n_text == 0 && (col->n_int > 0 && col->n_bool > 0)
		    && col->n_double == 0)
		    type = "INTEGER NOT NULL";
		if (col->n_text == 0 && col->n_int == 0 && col->n_double > 0
		    && col->n_bool == 0)
		    type = "DOUBLE NOT NULL";
		if (col->n_text == 0 && col->n_int == 0 && col->n_double == 0
		    && col->n_bool > 0)
		    type = "BOOLEAN NOT NULL";
	    }
	  prev = sql;
	  sql = sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev, xname, type);
	  free (xname);
	  sqlite3_free (prev);
	  col = col->next;
      }
    prev = sql;
    sql = sqlite3_mprintf ("%s)\n", prev);
    sqlite3_free (prev);
    return sql;
}

SPATIALITE_DECLARE char *
geojson_sql_create_table (geojson_parser_ptr parser, const char *table,
			  int colname_case)
{
/* will return the SQL CREATE TABLE statement */
    char *sql;
    char *prev;
    char *xname;
    char *pk_name;
    char *xpk_name;
    const char *type;
    geojson_column_ptr col;
    if (table == NULL)
	return NULL;

    xname = gaiaDoubleQuotedSql (table);
    pk_name = geojson_unique_pk (parser, "pk_uid");
    xpk_name = geojson_normalize_case (pk_name, colname_case);
    sqlite3_free (pk_name);
    sql =
	sqlite3_mprintf
	("CREATE TABLE \"%s\" (\n\t%s INTEGER PRIMARY KEY AUTOINCREMENT", xname,
	 xpk_name);
    free (xname);
    free (xpk_name);
    col = parser->first_col;
    while (col != NULL)
      {
	  /* adding columns */
	  char *xcol = geojson_normalize_case (col->name, colname_case);
	  xname = gaiaDoubleQuotedSql (xcol);
	  free (xcol);
	  type = "TEXT";
	  if (col->n_null > 0)
	    {
		/* NULL values */
		if (col->n_text > 0 && col->n_int == 0 && col->n_double == 0
		    && col->n_bool == 0)
		    type = "TEXT";
		if (col->n_text == 0 && col->n_int > 0 && col->n_double == 0
		    && col->n_bool == 0)
		    type = "INTEGER";
		if (col->n_text == 0 && (col->n_int > 0 && col->n_bool > 0)
		    && col->n_double == 0)
		    type = "INTEGER";
		if (col->n_text == 0 && col->n_int == 0 && col->n_double > 0
		    && col->n_bool == 0)
		    type = "DOUBLE";
		if (col->n_text == 0 && col->n_int == 0 && col->n_double == 0
		    && col->n_bool > 0)
		    type = "BOOLEAN";
	    }
	  else
	    {
		/* NOT NULL */
		if (col->n_text > 0 && col->n_int == 0 && col->n_double == 0
		    && col->n_bool == 0)
		    type = "TEXT NOT NULL";
		if (col->n_text == 0 && col->n_int > 0 && col->n_double == 0
		    && col->n_bool == 0)
		    type = "INTEGER NOT NULL";
		if (col->n_text == 0 && (col->n_int > 0 && col->n_bool > 0)
		    && col->n_double == 0)
		    type = "INTEGER NOT NULL";
		if (col->n_text == 0 && col->n_int == 0 && col->n_double > 0
		    && col->n_bool == 0)
		    type = "DOUBLE NOT NULL";
		if (col->n_text == 0 && col->n_int == 0 && col->n_double == 0
		    && col->n_bool > 0)
		    type = "BOOLEAN NOT NULL";
	    }
	  prev = sql;
	  sql = sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev, xname, type);
	  free (xname);
	  sqlite3_free (prev);
	  col = col->next;
      }
    prev = sql;
    sql = sqlite3_mprintf ("%s)\n", prev);
    sqlite3_free (prev);
    return sql;
}

SPATIALITE_DECLARE char *
geojson_sql_add_geometry (geojson_parser_ptr parser, const char *table,
			  const char *geom_col, int colname_case, int srid)
{
/* will return the SQL AddGeometryColumn() statement */
    char *sql;
    char *geom_name;
    char *xgeom_col;
    char *type = "GEOMETRY";
    char *dims = "XY";
    if (table == NULL || geom_col == NULL)
	return NULL;
    if (parser->n_points == 0 && parser->n_linestrings == 0
	&& parser->n_polygons == 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_mpolygons == 0
	&& parser->n_geomcolls == 0)
	return NULL;
    if (parser->n_geom_2d == 0 && parser->n_geom_3d == 0
	&& parser->n_geom_4d == 0)
	return NULL;

    if (parser->n_points > 0 && parser->n_linestrings == 0
	&& parser->n_polygons == 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_mpolygons == 0
	&& parser->n_geomcolls == 0)
      {
	  type = "POINT";
	  strcpy (parser->cast_type, "CastToPoint");
      }
    if (parser->n_mpoints > 0 && parser->n_linestrings == 0
	&& parser->n_polygons == 0 && parser->n_mlinestrings == 0
	&& parser->n_mpolygons == 0 && parser->n_geomcolls == 0)
      {
	  type = "MULTIPOINT";
	  strcpy (parser->cast_type, "CastToMultiPoint");
      }
    if (parser->n_points == 0 && parser->n_linestrings > 0
	&& parser->n_polygons == 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_mpolygons == 0
	&& parser->n_geomcolls == 0)
      {
	  type = "LINESTRING";
	  strcpy (parser->cast_type, "CastToLinestring");
      }
    if (parser->n_mlinestrings > 0 && parser->n_points == 0
	&& parser->n_polygons == 0 && parser->n_mpoints == 0
	&& parser->n_mpolygons == 0 && parser->n_geomcolls == 0)
      {
	  type = "MULTILINESTRING";
	  strcpy (parser->cast_type, "CastToMultiLinestring");
      }
    if (parser->n_points == 0 && parser->n_linestrings > 0
	&& parser->n_polygons > 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_mpolygons == 0
	&& parser->n_geomcolls == 0)
      {
	  type = "POLYGON";
	  strcpy (parser->cast_type, "CastToPolygon");
      }
    if (parser->n_mpolygons > 0 && parser->n_points == 0
	&& parser->n_linestrings == 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_geomcolls == 0)
      {
	  type = "MULTIPOLYGON";
	  strcpy (parser->cast_type, "CastToMultiPolygon");
      }
    if ((parser->n_points + parser->n_mpoints) > 0
	&& (parser->n_linestrings + parser->n_mlinestrings) > 0)
      {
	  type = "GEOMETRYCOLLECTION";
	  strcpy (parser->cast_type, "CastToGeometryCollection");
      }
    if ((parser->n_points + parser->n_mpoints) > 0
	&& (parser->n_polygons + parser->n_mpolygons) > 0)
      {
	  type = "GEOMETRYCOLLECTION";
	  strcpy (parser->cast_type, "CastToGeometryCollection");
      }
    if ((parser->n_linestrings + parser->n_mlinestrings) > 0
	&& (parser->n_polygons + parser->n_mpolygons) > 0)
      {
	  type = "GEOMETRYCOLLECTION";
	  strcpy (parser->cast_type, "CastToGeometryCollection");
      }

    if (parser->n_geom_2d > 0 && parser->n_geom_3d == 0
	&& parser->n_geom_4d == 0)
      {
	  dims = "XY";
	  strcpy (parser->cast_dims, "CastToXY");
      }
    if (parser->n_geom_3d > 0 && parser->n_geom_4d == 0)
      {
	  dims = "XYZ";
	  strcpy (parser->cast_dims, "CastToXYZ");
      }
    if (parser->n_geom_4d > 0)
      {
	  dims = "XYZM";
	  strcpy (parser->cast_dims, "CastToXYZM");
      }
    geom_name = geojson_unique_geom (parser, geom_col);
    xgeom_col = geojson_normalize_case (geom_name, colname_case);
    sqlite3_free (geom_name);
    sql =
	sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, %Q, %d, %Q, %Q)", table,
			 xgeom_col, srid, type, dims);
    free (xgeom_col);
    return sql;
}

SPATIALITE_DECLARE char *
geojson_sql_create_rtree (const char *table, const char *geom_col,
			  int colname_case)
{
/* will return the SQL CreateSpatialIndex() statement */
    char *sql;
    char *xgeom_col;
    if (table == NULL || geom_col == NULL)
	return NULL;
    xgeom_col = geojson_normalize_case (geom_col, colname_case);
    sql =
	sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, %Q)", table, xgeom_col);
    free (xgeom_col);
    return sql;
}

SPATIALITE_DECLARE char *
geojson_sql_insert_into (geojson_parser_ptr parser, const char *table)
{
/* will return the SQL INSERT INTO statement */
    char *sql;
    char *prev;
    char *xname;
    geojson_column_ptr col;
    if (table == NULL)
	return NULL;

    xname = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("INSERT INTO \"%s\" VALUES (NULL", xname);
    free (xname);

    col = parser->first_col;
    while (col != NULL)
      {
	  prev = sql;
	  sql = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
	  col = col->next;
      }
    prev = sql;
/* Geometry is always the last Column */
    sql =
	sqlite3_mprintf ("%s, %s(%s(?)))", prev, parser->cast_dims,
			 parser->cast_type);
    sqlite3_free (prev);
    return sql;
}

SPATIALITE_DECLARE int
geojson_init_feature (geojson_parser_ptr parser, geojson_feature_ptr ft,
		      char **error_message)
{
/* attempting to fully initialize a GeoJSON Feature object */
    int ret;
    int len;
    char *buf;
    geojson_property_ptr prop;
    *error_message = NULL;

    if (ft->prop_offset_start < 0 || ft->prop_offset_end < 0)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON parser: invalid Properties (fid=%d)\n",
			       ft->fid);
	  return 0;
      }
    if (ft->prop_offset_end <= ft->prop_offset_start)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON parser: invalid Properties (fid=%d)\n",
			       ft->fid);
	  return 0;
      }
    ret = fseek (parser->in, ft->prop_offset_start, SEEK_SET);
    if (ret != 0)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON parser: Properties invalid seek (fid=%d)\n", ft->fid);
	  return 0;
      }
    len = ft->prop_offset_end - ft->prop_offset_start - 1;
    buf = malloc (len + 1);
    if (buf == NULL)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON parser: Properties insufficient memory (fid=%d)\n",
	       ft->fid);
	  return 0;
      }
    ret = fread (buf, 1, len, parser->in);
    if (ret != len)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON parser: Properties read error (fid=%d)\n", ft->fid);
	  free (buf);
	  return 0;
      }
    *(buf + len) = '\0';
    geojson_parse_properties (ft, buf, error_message);
    free (buf);

/* checking for duplicate Droperty names */
    prop = ft->first;
    while (prop != NULL)
      {
	  geojson_property_ptr prop2 = prop->next;
	  while (prop2 != NULL)
	    {
		if (strcasecmp (prop->name, prop2->name) == 0)
		  {
		      *error_message =
			  sqlite3_mprintf
			  ("GeoJSON parser: duplicate property name \"%s\" (fid=%d)\n",
			   prop->name, ft->fid);
		      return 0;
		  }
		prop2 = prop2->next;
	    }
	  prop = prop->next;
      }

/* reading the GeoJSON Geometry */
    if (ft->geom_offset_start < 0 || ft->geom_offset_end < 0)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON parser: invalid Geometry (fid=%d)\n",
			       ft->fid);
	  return 0;
      }
    if (ft->geom_offset_end <= ft->geom_offset_start)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON parser: invalid Geometry (fid=%d)\n",
			       ft->fid);
	  return 0;
      }
    ret = fseek (parser->in, ft->geom_offset_start, SEEK_SET);
    if (ret != 0)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON parser: Geometry invalid seek (fid=%d)\n", ft->fid);
	  return 0;
      }
    len = ft->geom_offset_end - ft->geom_offset_start;
    if (len == 0)
      {
	  /* NULL Geometry */
	  if (ft->geometry != NULL)
	      free (ft->geometry);
	  ft->geometry = NULL;
	  return 1;
      }
    buf = malloc (len + 2);
    if (buf == NULL)
      {
	  *error_message =
	      sqlite3_mprintf
	      ("GeoJSON parser: Geometry insufficient memory (fid=%d)\n",
	       ft->fid);
	  return 0;
      }
    *buf = '{';
    ret = fread (buf + 1, 1, len, parser->in);
    if (ret != len)
      {
	  *error_message =
	      sqlite3_mprintf ("GeoJSON parser: Geometry read error (fid=%d)\n",
			       ft->fid);
	  free (buf);
	  return 0;
      }
    *(buf + len + 1) = '\0';
    if (ft->geometry != NULL)
	free (ft->geometry);
    ft->geometry = buf;
    return 1;
}

SPATIALITE_DECLARE void
geojson_reset_feature (geojson_feature_ptr ft)
{
/* memory cleanup - freeing a Feature */
    geojson_property_ptr pp;
    if (ft == NULL)
	return;

    if (ft->geometry != NULL)
	free (ft->geometry);
    pp = ft->first;
    while (pp != NULL)
      {
	  geojson_property_ptr ppn = pp->next;
	  if (pp->name != NULL)
	      free (pp->name);
	  if (pp->txt_value != NULL)
	      free (pp->txt_value);
	  free (pp);
	  pp = ppn;
      }
    ft->geometry = NULL;
    ft->first = NULL;
    ft->last = NULL;
}

SPATIALITE_DECLARE geojson_property_ptr
geojson_get_property_by_name (geojson_feature_ptr ft, const char *name)
{
/* will return the pointer to a Property indentified by its name */
    geojson_property_ptr pp;
    if (ft == NULL || name == NULL)
	return NULL;

    pp = ft->first;
    while (pp != NULL)
      {
	  if (strcasecmp (pp->name, name) == 0)
	      return pp;
	  pp = pp->next;
      }
    return NULL;
}

static int
vgeojson_has_metadata (sqlite3 * db, int *geotype)
{
/* testing the layout of virts_geometry_columns table */
    char **results;
    int ret;
    int rows;
    int columns;
    int i;
    int ok_virt_name = 0;
    int ok_virt_geometry = 0;
    int ok_srid = 0;
    int ok_geometry_type = 0;
    int ok_type = 0;
    int ok_coord_dimension = 0;

    ret =
	sqlite3_get_table (db, "PRAGMA table_info(virts_geometry_columns)",
			   &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
      {
	  if (strcasecmp ("virt_name", results[(i * columns) + 1]) == 0)
	      ok_virt_name = 1;
	  if (strcasecmp ("virt_geometry", results[(i * columns) + 1]) == 0)
	      ok_virt_geometry = 1;
	  if (strcasecmp ("srid", results[(i * columns) + 1]) == 0)
	      ok_srid = 1;
	  if (strcasecmp ("geometry_type", results[(i * columns) + 1]) == 0)
	      ok_geometry_type = 1;
	  if (strcasecmp ("type", results[(i * columns) + 1]) == 0)
	      ok_type = 1;
	  if (strcasecmp ("coord_dimension", results[(i * columns) + 1]) == 0)
	      ok_coord_dimension = 1;
      }
    sqlite3_free_table (results);

    if (ok_virt_name && ok_virt_geometry && ok_srid && ok_geometry_type
	&& ok_coord_dimension)
      {
	  *geotype = 1;
	  return 1;
      }
    if (ok_virt_name && ok_virt_geometry && ok_srid && ok_type)
      {
	  *geotype = 0;
	  return 1;
      }
    return 0;
}

static geojson_property_ptr
vgeojson_get_property_by_name (VirtualGeoJsonCursorPtr cursor, const char *name)
{
/* attempting to retrieve a Property from the current Feature (by property name) */
    geojson_feature_ptr ft = cursor->Feature;
    geojson_property_ptr prop;
    if (ft == NULL)
	return NULL;
    prop = ft->first;
    while (prop != NULL)
      {
	  if (prop->name != NULL)
	    {
		if (strcasecmp (prop->name, name) == 0)
		    return prop;
	    }
	  prop = prop->next;
      }
    return NULL;
}

static geojson_property_ptr
vgeojson_get_property_by_col (VirtualGeoJsonCursorPtr cursor, int column)
{
/* attempting to retrieve a Property from the current Feature (by column index) */
    int icol = 0;
    geojson_column_ptr col;
    geojson_parser_ptr parser = cursor->pVtab->Parser;
    if (parser == NULL)
	return NULL;
    col = parser->first_col;
    while (col != NULL)
      {
	  if (icol == column)
	      return vgeojson_get_property_by_name (cursor, col->name);
	  icol++;
	  col = col->next;
      }
    return NULL;
}

static void
vgeojson_get_extent (VirtualGeoJsonPtr p_vt)
{
/* determining the Full Extent */
    int fid;
    geojson_feature_ptr ft;
    char *error_message;
    gaiaGeomCollPtr geom;
    if (!(p_vt->Valid))
	return;

    for (fid = 0; fid < p_vt->Parser->count; fid++)
      {
	  ft = p_vt->Parser->features + fid;
	  if (!geojson_init_feature (p_vt->Parser, ft, &error_message))
	    {
		/* an error occurred */
		spatialite_e ("%s\n", error_message);
		sqlite3_free (error_message);
		p_vt->Valid = 0;
		return;
	    }
	  geom = gaiaParseGeoJSON ((const unsigned char *) (ft->geometry));
	  if (geom != NULL)
	    {
		if (geom->MinX < p_vt->MinX)
		    p_vt->MinX = geom->MinX;
		if (geom->MaxX > p_vt->MaxX)
		    p_vt->MaxX = geom->MaxX;
		if (geom->MinY < p_vt->MinY)
		    p_vt->MinY = geom->MinY;
		if (geom->MaxY > p_vt->MaxY)
		    p_vt->MaxY = geom->MaxY;
		gaiaFreeGeomColl (geom);
	    }
	  geojson_reset_feature (ft);
      }
}

static int
vgeojson_create (sqlite3 * db, void *pAux, int argc, const char *const *argv,
		 sqlite3_vtab ** ppVTab, char **pzErr)
{
/* creates the virtual table connected to some GeoJSON file */
    char *sql;
    VirtualGeoJsonPtr p_vt;
    char path[2048];
    char ColnameCase[128];
    const char *pColnameCase;
    int len;
    const char *pPath = NULL;
    int srid = 4326;
    int colname_case = GAIA_DBF_COLNAME_LOWERCASE;
    char *xname;
    int geotype;
    int ret;
    sqlite3_stmt *stmt = NULL;
    FILE *in;
    char *error_message = NULL;
    geojson_parser_ptr parser;
    if (pAux)
	pAux = pAux;		/* unused arg warning suppression */
/* checking for GeoJSON PATH */
    if (argc == 4 || argc == 5 || argc == 6)
      {
	  pPath = argv[3];
	  len = strlen (pPath);
	  if ((*(pPath + 0) == '\'' || *(pPath + 0) == '"')
	      && (*(pPath + len - 1) == '\'' || *(pPath + len - 1) == '"'))
	    {
		/* the path is enclosed between quotes - we need to dequote it */
		strcpy (path, pPath + 1);
		len = strlen (path);
		*(path + len - 1) = '\0';
	    }
	  else
	      strcpy (path, pPath);
	  if (argc >= 5)
	    {
		srid = atoi (argv[4]);
		if (srid < 0)
		    srid = -1;
	    }
	  if (argc >= 6)
	    {
		pColnameCase = argv[5];
		len = strlen (pColnameCase);
		if ((*(pColnameCase + 0) == '\'' || *(pColnameCase + 0) == '"')
		    && (*(pColnameCase + len - 1) == '\''
			|| *(pColnameCase + len - 1) == '"'))
		  {
		      /* the colcase-name is enclosed between quotes - we need to dequote it */
		      strcpy (ColnameCase, pColnameCase + 1);
		      len = strlen (ColnameCase);
		      *(ColnameCase + len - 1) = '\0';
		  }
		else
		    strcpy (ColnameCase, pColnameCase);
		if (strcasecmp (ColnameCase, "uppercase") == 0
		    || strcasecmp (ColnameCase, "upper") == 0)
		    colname_case = GAIA_DBF_COLNAME_UPPERCASE;
		else if (strcasecmp (ColnameCase, "samecase") == 0
			 || strcasecmp (ColnameCase, "same") == 0)
		    colname_case = GAIA_DBF_COLNAME_CASE_IGNORE;
		else
		    colname_case = GAIA_DBF_COLNAME_LOWERCASE;
	    }
      }
    else
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[VirtualGeoJSON module] CREATE VIRTUAL: illegal arg list {geojson_path [ , srid [ , colname_case ]] }");
	  return SQLITE_ERROR;
      }
    p_vt = (VirtualGeoJsonPtr) sqlite3_malloc (sizeof (VirtualGeoJson));
    if (!p_vt)
	return SQLITE_NOMEM;
    p_vt->pModule = &my_geojson_module;
    p_vt->nRef = 0;
    p_vt->zErrMsg = NULL;
    p_vt->db = db;
    p_vt->Srid = srid;
    p_vt->Valid = 0;
    p_vt->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    p_vt->DimensionModel = GAIA_XY;
    len = strlen (argv[2]);
    p_vt->TableName = malloc (len + 1);
    strcpy (p_vt->TableName, argv[2]);
    p_vt->MinX = DBL_MAX;
    p_vt->MinY = DBL_MAX;
    p_vt->MaxX = -DBL_MAX;
    p_vt->MaxY = -DBL_MAX;

/* attempting to open the GeoJSON file for reading */
    in = fopen (path, "rb");
    if (in == NULL)
      {
	  error_message =
	      sqlite3_mprintf
	      ("GeoJSON parser: unable to open %s for reading\n", path);
	  goto err;
      }
/* creating the GeoJSON parser */
    parser = geojson_create_parser (in);
    if (!geojson_parser_init (parser, &error_message))
	goto err;
    if (!geojson_create_features_index (parser, &error_message))
	goto err;
    if (!geojson_check_features (parser, &error_message))
	goto err;
    p_vt->Valid = 1;
    p_vt->Parser = parser;
    goto ok;
  err:
    if (error_message != NULL)
      {
	  spatialite_e ("%s\n", error_message);
	  sqlite3_free (error_message);
      }
  ok:
    vgeojson_get_extent (p_vt);
    if (!(p_vt->Valid))
      {
	  /* something is going the wrong way; creating a stupid default table */
	  xname = gaiaDoubleQuotedSql ((const char *) argv[2]);
	  sql =
	      sqlite3_mprintf
	      ("CREATE TABLE \"%s\" (FID INTEGER, Geometry BLOB)", xname);
	  free (xname);
	  if (sqlite3_declare_vtab (db, sql) != SQLITE_OK)
	    {
		sqlite3_free (sql);
		*pzErr =
		    sqlite3_mprintf
		    ("[VirtualGeoJSON module] cannot build a table from the GeoJSON file\n");
		return SQLITE_ERROR;
	    }
	  sqlite3_free (sql);
	  *ppVTab = (sqlite3_vtab *) p_vt;
	  return SQLITE_OK;
      }
    if (parser->n_points > 0 && parser->n_linestrings == 0
	&& parser->n_polygons == 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_mpolygons == 0
	&& parser->n_geomcolls == 0)
	p_vt->DeclaredType = GAIA_POINT;
    if (parser->n_mpoints > 0 && parser->n_linestrings == 0
	&& parser->n_polygons == 0 && parser->n_mlinestrings == 0
	&& parser->n_mpolygons == 0 && parser->n_geomcolls == 0)
	p_vt->DeclaredType = GAIA_MULTIPOINT;
    if (parser->n_points == 0 && parser->n_linestrings > 0
	&& parser->n_polygons == 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_mpolygons == 0
	&& parser->n_geomcolls == 0)
	p_vt->DeclaredType = GAIA_LINESTRING;
    if (parser->n_mlinestrings > 0 && parser->n_points == 0
	&& parser->n_polygons == 0 && parser->n_mpoints == 0
	&& parser->n_mpolygons == 0 && parser->n_geomcolls == 0)
	p_vt->DeclaredType = GAIA_MULTILINESTRING;
    if (parser->n_points == 0 && parser->n_linestrings > 0
	&& parser->n_polygons > 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_mpolygons == 0
	&& parser->n_geomcolls == 0)
	p_vt->DeclaredType = GAIA_POLYGON;
    if (parser->n_mpolygons > 0 && parser->n_points == 0
	&& parser->n_linestrings == 0 && parser->n_mpoints == 0
	&& parser->n_mlinestrings == 0 && parser->n_geomcolls == 0)
	p_vt->DeclaredType = GAIA_MULTIPOLYGON;
    if ((parser->n_points + parser->n_mpoints) > 0
	&& (parser->n_linestrings + parser->n_mlinestrings) > 0)
	p_vt->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    if ((parser->n_points + parser->n_mpoints) > 0
	&& (parser->n_polygons + parser->n_mpolygons) > 0)
	p_vt->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    if ((parser->n_linestrings + parser->n_mlinestrings) > 0
	&& (parser->n_polygons + parser->n_mpolygons) > 0)
	p_vt->DeclaredType = GAIA_GEOMETRYCOLLECTION;
    if (parser->n_geom_2d > 0 && parser->n_geom_3d == 0
	&& parser->n_geom_4d == 0)
	p_vt->DimensionModel = GAIA_XY;
    if (parser->n_geom_3d > 0 && parser->n_geom_4d == 0)
	p_vt->DimensionModel = GAIA_XY_Z;
    if (parser->n_geom_4d > 0)
	p_vt->DimensionModel = GAIA_XY_Z_M;
/* preparing the COLUMNs for this VIRTUAL TABLE */
    sql =
	geojson_sql_create_virtual_table (parser, (const char *) argv[2],
					  colname_case);
    ret = sqlite3_declare_vtab (db, sql);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[VirtualGeoJSON module] CREATE VIRTUAL: invalid SQL statement \"%s\"",
	       sql);
	  return SQLITE_ERROR;
      }
    *ppVTab = (sqlite3_vtab *) p_vt;

    if (vgeojson_has_metadata (db, &geotype))
      {
	  /* registering the Virtual Geometry */
	  if (geotype)
	    {
		int xtype = 0;
		int xdims = 0;
		switch (p_vt->DeclaredType)
		  {
		  case GAIA_POINT:
		      switch (p_vt->DimensionModel)
			{
			case GAIA_XY_Z_M:
			    xtype = 3001;
			    xdims = 4;
			    break;
			case GAIA_XY_M:
			    xtype = 2001;
			    xdims = 3;
			    break;
			case GAIA_XY_Z:
			    xtype = 1001;
			    xdims = 3;
			    break;
			default:
			    xtype = 1;
			    xdims = 2;
			    break;
			};
		      break;
		  case GAIA_LINESTRING:
		      switch (p_vt->DimensionModel)
			{
			case GAIA_XY_Z_M:
			    xtype = 3002;
			    xdims = 4;
			    break;
			case GAIA_XY_M:
			    xtype = 2002;
			    xdims = 3;
			    break;
			case GAIA_XY_Z:
			    xtype = 1002;
			    xdims = 3;
			    break;
			default:
			    xtype = 2;
			    xdims = 2;
			    break;
			};
		      break;
		  case GAIA_POLYGON:
		      switch (p_vt->DimensionModel)
			{
			case GAIA_XY_Z_M:
			    xtype = 3003;
			    xdims = 4;
			    break;
			case GAIA_XY_M:
			    xtype = 2003;
			    xdims = 3;
			    break;
			case GAIA_XY_Z:
			    xtype = 1003;
			    xdims = 3;
			    break;
			default:
			    xtype = 3;
			    xdims = 2;
			    break;
			};
		      break;
		  case GAIA_MULTIPOINT:
		      switch (p_vt->DimensionModel)
			{
			case GAIA_XY_Z_M:
			    xtype = 3004;
			    xdims = 4;
			    break;
			case GAIA_XY_M:
			    xtype = 2004;
			    xdims = 3;
			    break;
			case GAIA_XY_Z:
			    xtype = 1004;
			    xdims = 3;
			    break;
			default:
			    xtype = 4;
			    xdims = 2;
			    break;
			};
		      break;
		  case GAIA_MULTILINESTRING:
		      switch (p_vt->DimensionModel)
			{
			case GAIA_XY_Z_M:
			    xtype = 3005;
			    xdims = 4;
			    break;
			case GAIA_XY_M:
			    xtype = 2005;
			    xdims = 3;
			    break;
			case GAIA_XY_Z:
			    xtype = 1005;
			    xdims = 3;
			    break;
			default:
			    xtype = 5;
			    xdims = 2;
			    break;
			};
		      break;
		  case GAIA_MULTIPOLYGON:
		      switch (p_vt->DimensionModel)
			{
			case GAIA_XY_Z_M:
			    xtype = 3006;
			    xdims = 4;
			    break;
			case GAIA_XY_M:
			    xtype = 2006;
			    xdims = 3;
			    break;
			case GAIA_XY_Z:
			    xtype = 1006;
			    xdims = 3;
			    break;
			default:
			    xtype = 6;
			    xdims = 2;
			    break;
			};
		      break;
		  case GAIA_GEOMETRYCOLLECTION:
		      switch (p_vt->DimensionModel)
			{
			case GAIA_XY_Z_M:
			    xtype = 3007;
			    xdims = 4;
			    break;
			case GAIA_XY_M:
			    xtype = 2007;
			    xdims = 3;
			    break;
			case GAIA_XY_Z:
			    xtype = 1007;
			    xdims = 3;
			    break;
			default:
			    xtype = 7;
			    xdims = 2;
			    break;
			};
		      break;
		  };
		sql =
		    sqlite3_mprintf
		    ("INSERT OR IGNORE INTO virts_geometry_columns "
		     "(virt_name, virt_geometry, geometry_type, coord_dimension, srid) "
		     "VALUES (Lower(%Q), 'geometry', %d, %d, %d)", argv[2],
		     xtype, xdims, p_vt->Srid);
	    }
	  else
	    {
		const char *xgtype = "GEOMETRY";
		switch (p_vt->DeclaredType)
		  {
		  case GAIA_POINT:
		      xgtype = "POINT";
		      break;
		  case GAIA_LINESTRING:
		      xgtype = "LINESTRING";
		      break;
		  case GAIA_POLYGON:
		      xgtype = "POLYGON";
		      break;
		  case GAIA_MULTIPOINT:
		      xgtype = "MULTIPOINT";
		      break;
		  case GAIA_MULTILINESTRING:
		      xgtype = "MULTILINESTRING";
		      break;
		  case GAIA_MULTIPOLYGON:
		      xgtype = "MULTIPOLYGON";
		      break;
		  case GAIA_GEOMETRYCOLLECTION:
		      xgtype = "GEOMETRYCOLLECTION";
		      break;
		  };
		sql =
		    sqlite3_mprintf
		    ("INSERT OR IGNORE INTO virts_geometry_columns "
		     "(virt_name, virt_geometry, type, srid) "
		     "VALUES (Lower(%Q), 'geometry', %Q, %d)", argv[2], xgtype,
		     p_vt->Srid);
	    }
	  sqlite3_exec (db, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
      }
    if (checkSpatialMetaData (db) == 3)
      {
	  /* current metadata style >= v.4.0.0 */

	  /* inserting a row into VIRTS_GEOMETRY_COLUMNS_AUTH */
	  sql = sqlite3_mprintf ("INSERT OR IGNORE INTO "
				 "virts_geometry_columns_auth (virt_name, virt_geometry, hidden) "
				 "VALUES (Lower(%Q), 'geometry', 0)", argv[2]);
	  sqlite3_exec (db, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);

	  /* inserting a row into GEOMETRY_COLUMNS_STATISTICS */
	  sql = sqlite3_mprintf ("INSERT OR IGNORE INTO "
				 "virts_geometry_columns_statistics (virt_name, virt_geometry) "
				 "VALUES (Lower(%Q), 'geometry')", argv[2]);
	  sqlite3_exec (db, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
      }

/* inserting into the connection cache: Virtual Extent */
    sql = "SELECT \"*Add-VirtualTable+Extent\"(?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (db, sql, strlen (sql), &stmt, NULL);
    if (ret == SQLITE_OK)
      {
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, argv[2], strlen (argv[2]), SQLITE_STATIC);
	  sqlite3_bind_double (stmt, 2, p_vt->MinX);
	  sqlite3_bind_double (stmt, 3, p_vt->MinY);
	  sqlite3_bind_double (stmt, 4, p_vt->MaxX);
	  sqlite3_bind_double (stmt, 5, p_vt->MaxY);
	  sqlite3_bind_int (stmt, 6, p_vt->Srid);
	  ret = sqlite3_step (stmt);
      }
    sqlite3_finalize (stmt);
    return SQLITE_OK;
}

static int
vgeojson_connect (sqlite3 * db, void *pAux, int argc, const char *const *argv,
		  sqlite3_vtab ** ppVTab, char **pzErr)
{
/* connects the virtual table to some GeoJSON file - simply aliases vgeojson_create() */
    return vgeojson_create (db, pAux, argc, argv, ppVTab, pzErr);
}

static int
vgeojson_best_index (sqlite3_vtab * pVTab, sqlite3_index_info * pIndex)
{
/* best index selection */
    int i;
    int iArg = 0;
    char str[2048];
    char buf[64];

    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */

    *str = '\0';
    for (i = 0; i < pIndex->nConstraint; i++)
      {
	  if (pIndex->aConstraint[i].usable)
	    {
		iArg++;
		pIndex->aConstraintUsage[i].argvIndex = iArg;
		pIndex->aConstraintUsage[i].omit = 1;
		sprintf (buf, "%d:%d,", pIndex->aConstraint[i].iColumn,
			 pIndex->aConstraint[i].op);
		strcat (str, buf);
	    }
      }
    if (*str != '\0')
      {
	  pIndex->idxStr = sqlite3_mprintf ("%s", str);
	  pIndex->needToFreeIdxStr = 1;
      }

    return SQLITE_OK;
}

static int
vgeojson_disconnect (sqlite3_vtab * pVTab)
{
/* disconnects the virtual table */
    int ret;
    sqlite3_stmt *stmt;
    const char *sql;
    VirtualGeoJsonPtr p_vt = (VirtualGeoJsonPtr) pVTab;

/* removing from the connection cache: Virtual Extent */
    sql = "SELECT \"*Remove-VirtualTable+Extent\"(?)";
    ret = sqlite3_prepare_v2 (p_vt->db, sql, strlen (sql), &stmt, NULL);
    if (ret == SQLITE_OK)
      {
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, p_vt->TableName, strlen (p_vt->TableName),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
      }
    sqlite3_finalize (stmt);

    if (p_vt->TableName != NULL)
	free (p_vt->TableName);
    sqlite3_free (p_vt);

    return SQLITE_OK;
}

static int
vgeojson_destroy (sqlite3_vtab * pVTab)
{
/* destroys the virtual table - simply aliases vgeojson_disconnect() */
    return vgeojson_disconnect (pVTab);
}

static void
vgeojson_read_row (VirtualGeoJsonCursorPtr cursor)
{
/* trying to read a "row" from the GeoJSON file */
    int fid;
    geojson_feature_ptr ft;
    char *error_message;
    if (!(cursor->pVtab->Valid))
      {
	  cursor->eof = 1;
	  return;
      }
    if (cursor->Feature != NULL)
	geojson_reset_feature (cursor->Feature);
    fid = cursor->current_fid;
    if (fid < 0 || fid >= cursor->pVtab->Parser->count)
      {
	  /* normal GeoJSON EOF */
	  cursor->eof = 1;
	  return;
      }
    ft = cursor->pVtab->Parser->features + fid;
    if (!geojson_init_feature (cursor->pVtab->Parser, ft, &error_message))
      {
	  /* an error occurred */
	  spatialite_e ("%s\n", error_message);
	  sqlite3_free (error_message);
	  cursor->eof = 1;
	  return;
      }
    cursor->Feature = ft;
}

static int
vgeojson_open (sqlite3_vtab * pVTab, sqlite3_vtab_cursor ** ppCursor)
{
/* opening a new cursor */
    VirtualGeoJsonCursorPtr cursor =
	(VirtualGeoJsonCursorPtr)
	sqlite3_malloc (sizeof (VirtualGeoJsonCursor));
    if (cursor == NULL)
	return SQLITE_ERROR;
    cursor->firstConstraint = NULL;
    cursor->lastConstraint = NULL;
    cursor->pVtab = (VirtualGeoJsonPtr) pVTab;
    cursor->current_fid = 0;
    cursor->Feature = NULL;
    cursor->eof = 0;
    *ppCursor = (sqlite3_vtab_cursor *) cursor;
    vgeojson_read_row (cursor);
    return SQLITE_OK;
}

static void
vgeojson_free_constraints (VirtualGeoJsonCursorPtr cursor)
{
/* memory cleanup - cursor constraints */
    VirtualGeoJsonConstraintPtr pC;
    VirtualGeoJsonConstraintPtr pCn;
    pC = cursor->firstConstraint;
    while (pC)
      {
	  pCn = pC->next;
	  if (pC->txtValue)
	      sqlite3_free (pC->txtValue);
	  sqlite3_free (pC);
	  pC = pCn;
      }
    cursor->firstConstraint = NULL;
    cursor->lastConstraint = NULL;
}

static int
vgeojson_close (sqlite3_vtab_cursor * pCursor)
{
/* closing the cursor */
    VirtualGeoJsonCursorPtr cursor = (VirtualGeoJsonCursorPtr) pCursor;
    if (cursor->Feature != NULL)
	geojson_reset_feature (cursor->Feature);
    vgeojson_free_constraints (cursor);
    sqlite3_free (pCursor);
    return SQLITE_OK;
}

static int
vgeojson_parse_constraint (const char *str, int index, int *iColumn, int *op)
{
/* parsing a constraint string */
    char buf[64];
    const char *in = str;
    char *out = buf;
    int i = 0;
    int found = 0;

    *out = '\0';
    while (*in != '\0')
      {
	  if (*in == ',')
	    {
		if (index == i)
		  {
		      *out = '\0';
		      found = 1;
		      break;
		  }
		i++;
		in++;
		continue;
	    }
	  if (index == i)
	      *out++ = *in;
	  in++;
      }
    if (!found)
	return 0;
    in = buf;
    for (i = 0; i < (int) strlen (buf); i++)
      {
	  if (buf[i] == ':')
	    {
		buf[i] = '\0';
		*iColumn = atoi (buf);
		*op = atoi (buf + i + 1);
		return 1;
	    }
	  in++;
      }
    return 0;
}

static int
vgeojson_eval_constraints (VirtualGeoJsonCursorPtr cursor)
{
/* evaluating Filter constraints */
    int nCol;
    geojson_column_ptr col;
    geojson_property_ptr prop;
    VirtualGeoJsonConstraintPtr pC = cursor->firstConstraint;
    if (pC == NULL)
	return 1;
    while (pC)
      {
	  int ok = 0;
	  if (pC->iColumn == 0)
	    {
		/* the PRIMARY KEY column */
		if (pC->valueType == 'I')
		  {
		      switch (pC->op)
			{
			case SQLITE_INDEX_CONSTRAINT_EQ:
			    if (cursor->current_fid == pC->intValue)
				ok = 1;
			    break;
			case SQLITE_INDEX_CONSTRAINT_GT:
			    if (cursor->current_fid > pC->intValue)
				ok = 1;
			    break;
			case SQLITE_INDEX_CONSTRAINT_LE:
			    if (cursor->current_fid <= pC->intValue)
				ok = 1;
			    break;
			case SQLITE_INDEX_CONSTRAINT_LT:
			    if (cursor->current_fid < pC->intValue)
				ok = 1;
			    break;
			case SQLITE_INDEX_CONSTRAINT_GE:
			    if (cursor->current_fid >= pC->intValue)
				ok = 1;
			    break;
			};
		  }
		goto done;
	    }
	  nCol = 2;
	  col = cursor->pVtab->Parser->first_col;
	  while (col)
	    {
		if (nCol != pC->iColumn)
		  {
		      nCol++;
		      col = col->next;
		      continue;
		  }
		if (col->name == NULL)
		  {
		      ok = 1;
		      goto done;
		  }
		prop = vgeojson_get_property_by_name (cursor, col->name);
		if (prop == NULL)
		  {
		      nCol++;
		      col = col->next;
		      continue;
		  }
		switch (prop->type)
		  {
		  case GEOJSON_INTEGER:
		      if (pC->valueType == 'I')
			{
			    switch (pC->op)
			      {
			      case SQLITE_INDEX_CONSTRAINT_EQ:
				  if (prop->int_value == pC->intValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_GT:
				  if (prop->int_value > pC->intValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_LE:
				  if (prop->int_value <= pC->intValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_LT:
				  if (prop->int_value < pC->intValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_GE:
				  if (prop->int_value >= pC->intValue)
				      ok = 1;
				  break;
			      };
			}
		      break;
		  case GEOJSON_DOUBLE:
		      if (pC->valueType == 'I')
			{
			    switch (pC->op)
			      {
			      case SQLITE_INDEX_CONSTRAINT_EQ:
				  if (prop->dbl_value == pC->intValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_GT:
				  if (prop->dbl_value > pC->intValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_LE:
				  if (prop->dbl_value <= pC->intValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_LT:
				  if (prop->dbl_value < pC->intValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_GE:
				  if (prop->dbl_value >= pC->intValue)
				      ok = 1;
				  break;
			      };
			}
		      if (pC->valueType == 'D')
			{
			    switch (pC->op)
			      {
			      case SQLITE_INDEX_CONSTRAINT_EQ:
				  if (prop->dbl_value == pC->dblValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_GT:
				  if (prop->dbl_value > pC->dblValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_LE:
				  if (prop->dbl_value <= pC->dblValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_LT:
				  if (prop->dbl_value < pC->dblValue)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_GE:
				  if (prop->dbl_value >= pC->dblValue)
				      ok = 1;
				  break;
			      }
			}
		      break;
		  case GEOJSON_TEXT:
		      if (pC->valueType == 'T' && pC->txtValue)
			{
			    int ret;
			    ret = strcmp (prop->txt_value, pC->txtValue);
			    switch (pC->op)
			      {
			      case SQLITE_INDEX_CONSTRAINT_EQ:
				  if (ret == 0)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_GT:
				  if (ret > 0)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_LE:
				  if (ret <= 0)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_LT:
				  if (ret < 0)
				      ok = 1;
				  break;
			      case SQLITE_INDEX_CONSTRAINT_GE:
				  if (ret >= 0)
				      ok = 1;
				  break;
#ifdef HAVE_DECL_SQLITE_INDEX_CONSTRAINT_LIKE
			      case SQLITE_INDEX_CONSTRAINT_LIKE:
				  ret =
				      sqlite3_strlike (pC->txtValue,
						       prop->txt_value, 0);
				  if (ret == 0)
				      ok = 1;
				  break;
#endif
			      };
			}
		      break;
		  };
		break;
	    }
	done:
	  if (!ok)
	      return 0;
	  pC = pC->next;
      }
    return 1;
}

static int
vgeojson_filter (sqlite3_vtab_cursor * pCursor, int idxNum, const char *idxStr,
		 int argc, sqlite3_value ** argv)
{
/* setting up a cursor filter */
    int i;
    int iColumn;
    int op;
    int len;
    VirtualGeoJsonConstraintPtr pC;
    VirtualGeoJsonCursorPtr cursor = (VirtualGeoJsonCursorPtr) pCursor;
    if (idxNum)
	idxNum = idxNum;	/* unused arg warning suppression */

/* resetting any previously set filter constraint */
    vgeojson_free_constraints (cursor);

    for (i = 0; i < argc; i++)
      {
	  if (!vgeojson_parse_constraint (idxStr, i, &iColumn, &op))
	      continue;
	  pC = sqlite3_malloc (sizeof (VirtualGeoJsonConstraint));
	  if (!pC)
	      continue;
	  pC->iColumn = iColumn;
	  pC->op = op;
	  pC->valueType = '\0';
	  pC->txtValue = NULL;
	  pC->next = NULL;

	  if (sqlite3_value_type (argv[i]) == SQLITE_INTEGER)
	    {
		pC->valueType = 'I';
		pC->intValue = sqlite3_value_int64 (argv[i]);
	    }
	  if (sqlite3_value_type (argv[i]) == SQLITE_FLOAT)
	    {
		pC->valueType = 'D';
		pC->dblValue = sqlite3_value_double (argv[i]);
	    }
	  if (sqlite3_value_type (argv[i]) == SQLITE_TEXT)
	    {
		pC->valueType = 'T';
		len = sqlite3_value_bytes (argv[i]) + 1;
		pC->txtValue = (char *) sqlite3_malloc (len);
		if (pC->txtValue)
		    strcpy (pC->txtValue,
			    (char *) sqlite3_value_text (argv[i]));
	    }
	  if (cursor->firstConstraint == NULL)
	      cursor->firstConstraint = pC;
	  if (cursor->lastConstraint != NULL)
	      cursor->lastConstraint->next = pC;
	  cursor->lastConstraint = pC;
      }

    cursor->current_fid = 0;
    cursor->eof = 0;
    while (1)
      {
	  vgeojson_read_row (cursor);
	  if (cursor->eof)
	      break;
	  if (vgeojson_eval_constraints (cursor))
	      break;
	  cursor->current_fid += 1;
      }
    return SQLITE_OK;
}

static int
vgeojson_next (sqlite3_vtab_cursor * pCursor)
{
/* fetching a next row from cursor */
    VirtualGeoJsonCursorPtr cursor = (VirtualGeoJsonCursorPtr) pCursor;
    cursor->current_fid += 1;
    while (1)
      {
	  vgeojson_read_row (cursor);
	  if (cursor->eof)
	      break;
	  if (vgeojson_eval_constraints (cursor))
	      break;
	  cursor->current_fid += 1;
      }
    return SQLITE_OK;
}

static int
vgeojson_eof (sqlite3_vtab_cursor * pCursor)
{
/* cursor EOF */
    VirtualGeoJsonCursorPtr cursor = (VirtualGeoJsonCursorPtr) pCursor;
    return cursor->eof;
}

static gaiaGeomCollPtr
vgeojson_get_geometry (VirtualGeoJsonCursorPtr cursor)
{
/* attempting to return a normalized GeoJSON geometry */
    gaiaGeomCollPtr geom;
    if (cursor == NULL)
	return NULL;
    if (cursor->Feature == NULL)
	return NULL;
    if (cursor->Feature->geometry == NULL)
	return NULL;

    geom =
	gaiaParseGeoJSON ((const unsigned char *) (cursor->Feature->geometry));
    if (geom == NULL)
	return NULL;
    geom->Srid = cursor->pVtab->Srid;
    geom->DeclaredType = cursor->pVtab->DeclaredType;
    if (cursor->pVtab->DimensionModel != geom->DimensionModel)
      {
	  /* normalizing Dimensions */
	  gaiaGeomCollPtr g2;
	  switch (cursor->pVtab->DimensionModel)
	    {
	    case GAIA_XY_Z_M:
		g2 = gaiaCastGeomCollToXYZM (geom);
		break;
	    case GAIA_XY_Z:
		g2 = gaiaCastGeomCollToXYZ (geom);
		break;
	    case GAIA_XY_M:
		g2 = gaiaCastGeomCollToXYM (geom);
		break;
	    case GAIA_XY:
	    default:
		g2 = gaiaCastGeomCollToXY (geom);
		break;
	    };
	  gaiaFreeGeomColl (geom);
	  geom = g2;
      }
    return geom;
}

static int
vgeojson_column (sqlite3_vtab_cursor * pCursor, sqlite3_context * pContext,
		 int column)
{
/* fetching value for the Nth column */
    gaiaGeomCollPtr geom;
    geojson_property_ptr prop;
    VirtualGeoJsonCursorPtr cursor = (VirtualGeoJsonCursorPtr) pCursor;
    if (column == 0)
      {
	  /* the PRIMARY KEY column */
	  sqlite3_result_int (pContext, cursor->current_fid);
	  return SQLITE_OK;
      }
    if (column == 1)
      {
	  /* the GEOMETRY column */
	  geom = vgeojson_get_geometry (cursor);
	  if (geom)
	    {
		unsigned char *blob;
		int blobSize;
		gaiaToSpatiaLiteBlobWkb (geom, &blob, &blobSize);
		sqlite3_result_blob (pContext, blob, blobSize, free);
		gaiaFreeGeomColl (geom);
	    }
	  else
	      sqlite3_result_null (pContext);
	  return SQLITE_OK;
      }
    prop = vgeojson_get_property_by_col (cursor, column - 2);
    if (prop == NULL)
      {
	  sqlite3_result_null (pContext);
	  return SQLITE_OK;
      }
    switch (prop->type)
      {
      case GEOJSON_TRUE:
	  sqlite3_result_int (pContext, 1);
	  break;
      case GEOJSON_FALSE:
	  sqlite3_result_int (pContext, 0);
	  break;
      case GEOJSON_INTEGER:
	  sqlite3_result_int64 (pContext, prop->int_value);
	  break;
      case GEOJSON_DOUBLE:
	  sqlite3_result_double (pContext, prop->dbl_value);
	  break;
      case GEOJSON_TEXT:
	  sqlite3_result_text (pContext, prop->txt_value,
			       strlen (prop->txt_value), SQLITE_STATIC);
	  break;
      case GEOJSON_NULL:
      default:
	  sqlite3_result_null (pContext);
	  break;
      };
    return SQLITE_OK;
}

static int
vgeojson_rowid (sqlite3_vtab_cursor * pCursor, sqlite_int64 * pRowid)
{
/* fetching the ROWID */
    VirtualGeoJsonCursorPtr cursor = (VirtualGeoJsonCursorPtr) pCursor;
    *pRowid = cursor->current_fid;
    return SQLITE_OK;
}

static int
vgeojson_update (sqlite3_vtab * pVTab, int argc, sqlite3_value ** argv,
		 sqlite_int64 * pRowid)
{
/* generic update [INSERT / UPDATE / DELETE */
    if (pVTab || argc || argv || pRowid)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_READONLY;
}

static int
vgeojson_begin (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vgeojson_sync (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vgeojson_commit (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vgeojson_rollback (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vgeojson_rename (sqlite3_vtab * pVTab, const char *zNew)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    if (zNew)
	zNew = zNew;		/* unused arg warning suppression */
    return SQLITE_ERROR;
}

static int
spliteVirtualGeoJsonInit (sqlite3 * db)
{
    int rc = SQLITE_OK;
    my_geojson_module.iVersion = 1;
    my_geojson_module.xCreate = &vgeojson_create;
    my_geojson_module.xConnect = &vgeojson_connect;
    my_geojson_module.xBestIndex = &vgeojson_best_index;
    my_geojson_module.xDisconnect = &vgeojson_disconnect;
    my_geojson_module.xDestroy = &vgeojson_destroy;
    my_geojson_module.xOpen = &vgeojson_open;
    my_geojson_module.xClose = &vgeojson_close;
    my_geojson_module.xFilter = &vgeojson_filter;
    my_geojson_module.xNext = &vgeojson_next;
    my_geojson_module.xEof = &vgeojson_eof;
    my_geojson_module.xColumn = &vgeojson_column;
    my_geojson_module.xRowid = &vgeojson_rowid;
    my_geojson_module.xUpdate = &vgeojson_update;
    my_geojson_module.xBegin = &vgeojson_begin;
    my_geojson_module.xSync = &vgeojson_sync;
    my_geojson_module.xCommit = &vgeojson_commit;
    my_geojson_module.xRollback = &vgeojson_rollback;
    my_geojson_module.xFindFunction = NULL;
    my_geojson_module.xRename = &vgeojson_rename;
    sqlite3_create_module_v2 (db, "VirtualGeoJSON", &my_geojson_module, NULL,
			      0);
    return rc;
}

SPATIALITE_PRIVATE int
virtualgeojson_extension_init (void *xdb)
{
    sqlite3 *db = (sqlite3 *) xdb;
    return spliteVirtualGeoJsonInit (db);
}
