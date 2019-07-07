/*
 geojson.h -- Gaia common support for the GeoJSON parser
  
 version 5.0, 2018 Novenmber 26

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
 
Portions created by the Initial Developer are Copyright (C) 2018
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
 \file geojson.h

 GeoJSON structures
 */

#ifndef _GEOJSON_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GEOJSON_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constant values for GeoJSON objects */

/** UNKNOWN */
#define GEOJSON_UNKNOWN	0

/** FeatureCollection object */
#define GEOJSON_FCOLLECTION	101

/** Feature object */
#define GEOJSON_FEATURE		102

/** Properties object */
#define GEOJSON_PROPERTIES	103


/* constant values for GeoJSON geometries */

/** POINT */
#define GEOJSON_POINT			201

/** LINESTRING */
#define GEOJSON_LINESTRING		202

/** POLYGON */
#define GEOJSON_POLYGON			203

/** MULTIPOINT */
#define GEOJSON_MULTIPOINT		204

/** MULTILINESTRING */
#define GEOJSON_MULTILINESTRING	205

/** MULTIPOLYGON */
#define GEOJSON_MULTIPOLYGON	206

/** GEOMETRYCOLLECTION */
#define GEOJSON_GEOMCOLLECTION	207


/* constant values for GeoJSON datatypes */

/** Text */
#define GEOJSON_TEXT	301

/** Integer (namely Int64) */
#define GEOJSON_INTEGER	302

/** Floating Point Double precision */
#define GEOJSON_DOUBLE	303

/** Boolean - TRUE */
#define GEOJSON_TRUE	304

/** Boolean - FALSE */
#define GEOJSON_FALSE	305

/** NULL */
#define GEOJSON_NULL	306


/* constant values for GeoJSON sizes */

/** number of objects per each block */
#define GEOJSON_BLOCK	4096

/** max fixed length for Text Strings */
#define GEOJSON_MAX		1024

/** number of stack levels */
#define GEOJSON_STACK	16


/* GeoJSON objects and data structures */

/** 
 an object wrapping a GeoJSON Property (aka data attribute) 
*/
    typedef struct geojson_property_str
    {
/** Property name */
	char *name;
/** datatype */
	int type;
/** pointer to Text value */
	char *txt_value;
/** Integer value */
	sqlite3_int64 int_value;
/** Double value */
	double dbl_value;
/** pointer to next item [linked list] */
	struct geojson_property_str *next;
    } geojson_property;

/**
 pointer to Property
*/
    typedef geojson_property *geojson_property_ptr;

/**
 an object wrapping a GeoJSON Feature
*/
    typedef struct geojson_feature_str
    {
/** unique Feature ID */
	int fid;
/** start offset; Geometry object */
	long geom_offset_start;
/** end offset: Geometry object */
	long geom_offset_end;
/** start offset: Properties object */
	long prop_offset_start;
/** end offset: Properties object */
	long prop_offset_end;
/** pointer to the Geometry string */
	char *geometry;
/** linked list of Properties - pointer to first item */
	geojson_property_ptr first;
/** linked list of Properties - pointer to last item */
	geojson_property_ptr last;
    } geojson_feature;

/**
 pointer to Feature
*/
    typedef geojson_feature *geojson_feature_ptr;

/**
 an object wrapping an entry (aka Object) in the GeoJSON file 
*/
    typedef struct geojson_entry_str
    {
/** name of the parent object [key] */
	char *parent_key;
/** object type */
	int type;
/** count of Properties children */
	int properties;
/** count of Geometry children */
	int geometry;
/** object's start offset */
	long offset_start;
/** object's end offset */
	long offset_end;
    } geojson_entry;

/**
 pointer to Entry
*/
    typedef geojson_entry *geojson_entry_ptr;

/**
 an object wrapping a block of entries in the GeoJSON file */
    typedef struct geojson_block_str
    {
/** index of the next free entry in the block */
	int next_free_entry;
/** array of entries */
	geojson_entry entries[GEOJSON_BLOCK];
/** pointer to next item [linked list] */
	struct geojson_block_str *next;
    } geojson_block;

/**
 pointer to Block
*/
    typedef geojson_block *geojson_block_ptr;

/**
 an object wrapping a data Column 
*/
    typedef struct geojson_column_str
    {
/** column name */
	char *name;
/** number of Text values */
	int n_text;
/** number of Int values */
	int n_int;
/** number of Double values */
	int n_double;
/** number of Boolean values */
	int n_bool;
/** number of NULL values */
	int n_null;
/** pointer to next item [linked list] */
	struct geojson_column_str *next;
    } geojson_column;

/**
 pointer to Column
*/
    typedef geojson_column *geojson_column_ptr;

/**
 an object wrapping a GeoJSON parser 
*/
    typedef struct geojson_parser_str
    {
/** file handle */
	FILE *in;
/** linked list of Blocks - pointer to first item */
	geojson_block_ptr first;
/** linked list of Blocks - pointer to last item */
	geojson_block_ptr last;
/** total number of Features */
	int count;
/** array of Features */
	geojson_feature_ptr features;
/** linked list of Columns - pointer to first item */
	geojson_column_ptr first_col;
/** linked list of Columns - pointer to last item */
	geojson_column_ptr last_col;
/** total number of Point Geometries */
	int n_points;
/** total number of Linestring Geometries */
	int n_linestrings;
/** total number of Polygon Geometries */
	int n_polygons;
/** total number of MultiPoint Geometries */
	int n_mpoints;
/** total number of MultiLinestring Geometries */
	int n_mlinestrings;
/** total number of MultiPolygon Geometries */
	int n_mpolygons;
/** total number of GeometryCollection Geometries */
	int n_geomcolls;
/** total number of NULL Geometries */
	int n_geom_null;
/** total number of 2D Geometries */
	int n_geom_2d;
/** total number of 3D Geometries */
	int n_geom_3d;
/** total number of 4D Geometries */
	int n_geom_4d;
/** Geometry Type cast function */
	char cast_type[64];
/** Geometry Dims cast function */
	char cast_dims[64];
    } geojson_parser;

/**
 pointer to Parser
*/
    typedef geojson_parser *geojson_parser_ptr;

/**
 an object wrapping a Key-Value pair 
*/
    typedef struct geojson_keyval_str
    {
/** pointer to the Key string */
	char *key;
/** pointer to the Value string */
	char *value;
/** FALSE for quoted Text strings */
	int numvalue;
/** pointer to next item [linked list] */
	struct geojson_keyval_str *next;
    } geojson_keyval;

/**
 pointer to KeyValue
*/
    typedef geojson_keyval *geojson_keyval_ptr;

/**
 an object wrapping a stack entry
*/
    typedef struct geojson_stack_entry_str
    {
/** pointer to an Entry object */
	geojson_entry_ptr obj;
/** linked list of KeyValues - pointer to first item */
	geojson_keyval_ptr first;
/** linked list of KeyValues - pointer to last item */
	geojson_keyval_ptr last;
    } geojson_stack_entry;

/**
 pointer to Stakc Entry
*/
    typedef geojson_stack_entry *geojson_stack_entry_ptr;

/**
 an object wrapping a GeoJSON stack 
*/
    typedef struct geojson_stack
    {
	/* a stack for parsing nested GeoJSON objects */
/** current stack level */
	int level;
/** the stack levels array */
	geojson_stack_entry entries[GEOJSON_STACK];
/** the Key parsing buffer */
	char key[GEOJSON_MAX];
/** current Key index - last inserted byte */
	int key_idx;
/** the Value parsing buffer - quote delimited strings */
	char value[GEOJSON_MAX];
/** current Value index - last inserted byte */
	int value_idx;
/** the Values parsing buffer - numeric values */
	char numvalue[GEOJSON_MAX];
/** current numeric Value index - last inserted byte */
	int numvalue_idx;
    } geojson_stack;

/**
 pointer to Stack
*/
    typedef geojson_stack *geojson_stack_ptr;


/* function prototypes */

/**
 Creates a new GeoJSON parser object
 
 \param in an open FILE supposed to contain the GeoJSON text to be parsed

 \return the pointer to newly created object

 \sa geojson_destroy_parser, geojson_parser_init

 \note you are responsible to destroy (before or after) any allocated 
 GeoJSON parser object.
 */
    SPATIALITE_DECLARE geojson_parser_ptr geojson_create_parser (FILE * in);

/**
 Destroys a GeoJSON parser object

 \param p pointer to object to be destroyed

 \sa geojson_create_parser
 */
    SPATIALITE_DECLARE void geojson_destroy_parser (geojson_parser_ptr p);

/**
 Fully initializes a GeoJSON parser object
 
 \param parser pointer to a GeoJSON parser object
 \param error_message: will point to a diagnostic error message
  in case of failure, otherwise NULL

 \return 1 on success. 0 on failure (invalid GeoJSON text).

 \sa geojson_create_parser, geojson_check_features
 
 \note you are expected to free before or later an eventual error
 message by calling sqlite3_free()
 */
    SPATIALITE_DECLARE int geojson_parser_init (geojson_parser_ptr parser,
						char **error_message);

/**
 Checks a fully initialized GeoJSON parser object for containing valid Features
 
 \param parser pointer to a GeoJSON parser object
 \param error_message: will point to a diagnostic error message
  in case of failure, otherwise NULL

 \return 1 on success. 0 on failure (invalid GeoJSON text).

 \sa geojson_parser_init, geojson_create_features_index
 
 \note you are expected to free before or later an eventual error
 message by calling sqlite3_free()
 */
    SPATIALITE_DECLARE int geojson_check_features (geojson_parser_ptr parser,
						   char **error_message);

/**
 Creates the Features Index on a GeoJSON parser object
 
 \param parser pointer to a GeoJSON parser object
 \param error_message: will point to a diagnostic error message
  in case of failure, otherwise NULL

 \return 1 on success. 0 on failure (invalid GeoJSON text).

 \sa geojson_check_features
 
 \note you are expected to free before or later an eventual error
 message by calling sqlite3_free()
 */
    SPATIALITE_DECLARE int geojson_create_features_index (geojson_parser_ptr
							  parser,
							  char **error_message);

/**
 Will return the SQL CREATE TABLE statement
 
 \param parser pointer to a GeoJSON parser object
 \param table name of the SQL table to be created
 \param colname_case one between GAIA_DBF_COLNAME_LOWERCASE, 
	GAIA_DBF_COLNAME_UPPERCASE or GAIA_DBF_COLNAME_CASE_IGNORE.

 \return the SQL CREATE TABLE statement as a text string; NULL on failure

 \sa geojson_check_features, geojson_sql_add_geometry, geojson_sql_create_rtree
 geojson_insert_into
 
 \note you are expected to free the SQL string returned by this
 function by calling sqlite3_free() when it's no longer useful.
 */
    SPATIALITE_DECLARE char *geojson_sql_create_table (geojson_parser_ptr
						       parser,
						       const char *table,
						       int colname_case);

/**
 Will return the SQL AddGeometryColumn() statement
 
 \param parser pointer to a GeoJSON parser object
 \param table name of the SQL table being created
 \param geom_col name of the Geometry column
 \param colname_case one between GAIA_DBF_COLNAME_LOWERCASE, 
	GAIA_DBF_COLNAME_UPPERCASE or GAIA_DBF_COLNAME_CASE_IGNORE.
 \param srid the corresponding SRID value

 \return the AddGeometryColumn() SQL string; NULL on failure

 \sa geojson_check_features, geojson_sql_create_table, geojson_sql_create_rtree
 geojson_insert_into
 
 \note you are expected to free the SQL string returned by this
 function by calling sqlite3_free() when it's no longer useful.
 */
    SPATIALITE_DECLARE char *geojson_sql_add_geometry (geojson_parser_ptr
						       parser,
						       const char *table,
						       const char *geom_col,
						       int colname_case,
						       int srid);

/**
 Will return the SQL CreateSpatialIndex() statement
 
 \param table name of the SQL table being created
 \param geom_col name of the Geometry column
 \param colname_case one between GAIA_DBF_COLNAME_LOWERCASE, 
	GAIA_DBF_COLNAME_UPPERCASE or GAIA_DBF_COLNAME_CASE_IGNORE.

 \return the CreateSpatialIndex() SQL string; NULL on failure

 \sa geojson_check_features, geojson_sql_create_table, geojson_sql_add_geometry,
 geojson_insert_into
 
 \note you are expected to free the SQL string returned by this
 function by calling sqlite3_free() when it's no longer useful.
 */
    SPATIALITE_DECLARE char *geojson_sql_create_rtree (const char *table,
						       const char *geom_col,
						       int colname_case);

/**
 Will return the SQL INSERT INTO statement
 
 \param parser pointer to a GeoJSON parser object
 \param table name of the SQL table being created
 \param geom_col name of the Geometry column

 \return the INSERT INTO SQL string; NULL on failure

 \sa geojson_check_features, geojson_sql_create_table, geojson_add_geometry,
 geojson_sql_create_rtree
 
 \note you are expected to free the SQL string returned by this
 function by calling sqlite3_free() when it's no longer useful.
 */
    SPATIALITE_DECLARE char *geojson_sql_insert_into (geojson_parser_ptr
						      parser,
						      const char *table);

/**
 Will fully initialize a Feature with all Property and Geometry values 
 
 \param parser pointer to a GeoJSON parser object
 \param ft pointer the some GeoJson Feature object into the Parser 
 \param error_message: will point to a diagnostic error message
  in case of failure, otherwise NULL
 
 \return 1 on success. 0 on failure (invalid GeoJSON Feature).

 \sa geojson_create_features_index, geojson_reset_feature,
 geojson_get_property_by_name
 
 \note you are expected to free all Values returned by this function by 
 calling geojson_reset_feature() when they are no longer useful.
 And you are expected also to free before or later an eventual error
 message by calling sqlite3_free()
 */
    SPATIALITE_DECLARE int geojson_init_feature (geojson_parser_ptr parser,
						 geojson_feature_ptr ft,
						 char **error_message);

/**
 Will reset a Feature by freeing all Property and Geometry Values
 
 \param ft pointer the some GeoJson Feature object into the Parser 

 \sa geojson_create_features_index, geojson_init_feature,
 geojson_get_property_by_name
 */
    SPATIALITE_DECLARE void geojson_reset_feature (geojson_feature_ptr ft);

/**
 Will return the pointer to a given Property within a Feature 
 
 \param ft pointer the some GeoJson Feature object
 \param name the name of some specific Property

 \sa geojson_create_features_index, geojson_init_feature,
 geojson_reset_feature
 */
    SPATIALITE_DECLARE geojson_property_ptr
	geojson_get_property_by_name (geojson_feature_ptr ft, const char *name);

#ifdef __cplusplus
}
#endif

#endif				/* _GEOJSON_H */
