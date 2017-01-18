/*

 rl2dbms -- DBMS related functions

 version 0.1, 2013 March 29

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
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
#include <string.h>
#include <float.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include "config.h"

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%I64d\n"
#else
#define ERR_FRMT64 "ERROR: unable to decode Tile ID=%lld\n"
#endif

/**
 Return an  an approximate Zoom-Level for a resolution
 - as an information for Zoom-Level driven applications

 \param handle Database connection
 \param coverage to query (if NULL: a general query)
 \param i_srid of the resolution
 \param x_res resolution to check
 \param tile_width tile width of the coverage
 \param zoom_min pointer to store the calculated i_zoom_min value to extract
 \param zoom_default pointer to store the calculated i_zoom_default value to extract
 \param zoom_max pointer to store the calculated i_zoom_max value to extract
 
 \note Goals:
  - Zoom-Level driven applications have problems with wms-based servers, 
     since they often needed a min/max Zoom-Level to insure that
     when a map is loaded, something can be seen
    This avoids the user getting lost in a sea of white tiles
  - The Database-Designer should be offered the functionality to change the default values that were calculated.
      - togeather with a default position, this will help the user to orientate themselfs
  - It will be up to the Application using the library on how to use this
     - I use a central, well known point, assuming the user can move in the direction they are looking for 
  - 'zoom_defaul' corresponds to the original raster_coverage 'horz_resolution' value

 */
RL2_DECLARE int
rl2_get_zoom_levels_for_resolutions(sqlite3 * handle,const char *coverage,int i_srid,double x_res,double y_res,double min_x,double min_y,
                                                                unsigned char *zoom_min,unsigned char  *zoom_default,unsigned char  *zoom_max,
                                                                int *result_parm,char **result_text)
{
 unsigned char zl_max=30;
 char *coverage_tiles;
 char *geometry_ewkt;
 char *wsg84_ewkt;
 char *tile_zxy;
 int create_ewkt=0;
 char * sql;
 int i_have_levels=0;
 int i_have_section_levels=0;
 char *coverage_section;
 int i_rc=RL2_OK;
 int i_sql_rc=0;
 int i_srid_mercator=3395;
 int i_srid_wsg84=4326;
 int i_srid_dhdn_bbtor=187999; //  'DHDN / Brandenburger Tor'
 int result_text_parm=-1;
 sqlite3_stmt *stmt;
 double zl_meters_width[31]; 
 unsigned char zoom_level=0;
 double d_width_min=0.0;
 double d_width_max=0.0;
 double d_height_min=0.0;
 double d_height_max=0.0;
 // min_x and min_y as wsg84 for calculation to tiles
 double d_wsg84_x=0.0;
 double d_wsg84_y=0.0;
 int i_tile_width=256;
 int i_tile_height=256;
 int tile_width=256;
 int tile_height=256;
 int i_tile_x=0;
 int i_tile_y_osm=0;
 int i_tile_y_tms=0;
 int i_length=0;
 if ((result_parm) && (result_text))
 {
  result_text_parm=*result_parm;
  if (*result_text != NULL)
  { // use only sqlite3_malloc/sqlite3_free for '*result_text'
   sqlite3_free(*result_text);
   *result_text=NULL;
  }
  // 1=used sql; 2= meters min/max ; 3=sql+meters ;4=ewkt ;6= meters+ewkt
  if ((result_text_parm == 4) || (result_text_parm == 6))
   create_ewkt=1;
 }
 // 'WGS 84 / World Mercator' (3395) - UNIT=Meters            
 // Zoom-Level, Width in Meters of a 256x256 Tile
 // Value returned by gdalinfo for a geo-referenced Tile
 // values based on the position: 13.37771496361961 52.51628011262304 [Brandenburg Gate, Berlin, Germany]
 // 18_140813_85975_osm 176168_tms
 // SELECT cast(floor(((13.37771496361961+180)/360)*(1<<18)) AS int) AS tile_x; [140813]
 // SELECT cast(floor(((1.0-LN(TAN(RADIANS(52.51628011262304))+1.0/COS(RADIANS(52.51628011262304)))/PI())/2.0)*(1 << 18)) AS int) AS tile_y_osm;
 zl_meters_width[0]=40032406.294;
 zl_meters_width[1]=20016203.147;
 zl_meters_width[2]=9999156.402;
 zl_meters_width[3]=4995382.840;
 zl_meters_width[4]=2501025.826;
 zl_meters_width[5]=1250779.611;
 zl_meters_width[6]=625329.273;
 zl_meters_width[7]=312680.586;
 zl_meters_width[8]=156344.180;
 zl_meters_width[9]=78173.049;
 zl_meters_width[10]=39086.762;
 zl_meters_width[11]=19543.440 ;
 zl_meters_width[12]=9771.764;
 zl_meters_width[13]=4885.886;
 zl_meters_width[14]=2442.942;
 zl_meters_width[15]=1221.471;
 zl_meters_width[16]=610.735;
 zl_meters_width[17]=305.367;
 zl_meters_width[18]=152.683;
 zl_meters_width[19]=76.341;
 zl_meters_width[20]=38.170;
 zl_meters_width[21]=19.085;
 zl_meters_width[22]=9.542; 
 zl_meters_width[23]=4.771; 
 zl_meters_width[24]=2.385; 
 zl_meters_width[25]=1.192; 
 zl_meters_width[26]=0.596; 
 zl_meters_width[27]=0.298; 
 zl_meters_width[28]=0.149; 
 zl_meters_width[29]=0.074; 
 zl_meters_width[30]=0.037; // Last possible value that gdalwarp can create
 // Zoom 31. [gdalwarp] Failed to compute GCP transform: Transform is not solvable    
 if ((i_srid == 0) || (zoom_min == NULL) || (zoom_max == NULL) || (zoom_default == NULL))
  i_rc=-1;
 if (i_rc == RL2_OK)
 {
  // SELECT (((ST_MaxX(ST_Transform(geometry,3395))-ST_MinX(ST_Transform(geometry,3395)))/512)*256) FROM '1750.berlin_schmettau_tiles_tiles' ORDER BY pyramid_level ASC LIMIT 1 [228.893828]
 // printf ("-I-> rl2_get_zoom_levels_for_resolutions -begin-\n");
  if (i_srid < 0)
   i_srid=i_srid_dhdn_bbtor; //  'DHDN / Brandenburger Tor'
  if (i_srid == i_srid_wsg84)
   wsg84_ewkt = sqlite3_mprintf("ST_X(MakePoint(%2.7f,%2.7f,%d)) AS wsg84_x,"
                                                      "ST_Y(MakePoint(%2.7f,%2.7f,%d)) AS wsg84_y",
                                                      min_x,min_y,i_srid,min_x,min_y,i_srid);
  else
   wsg84_ewkt = sqlite3_mprintf("ST_X(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d)) AS wsg84_x,"
                                                         "ST_Y(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d)) AS wsg84_y",
                                                         min_x,min_y,i_srid,i_srid_wsg84,min_x,min_y,i_srid,i_srid_wsg84);
  if (coverage == NULL)
  { // to be used before tiles have been added
   if (i_srid ==  i_srid_mercator) 
   {     
    geometry_ewkt = sqlite3_mprintf("AsEWKT(BuildMbr(ST_MaxX(MakePoint(%2.7f+(%d*%2.7f),%2.7f+(%d*%2.7f),%d)),"
                                        "ST_MaxY(MakePoint(%2.7f+(%d*%2.7f),%2.7f+(%d*%2.7f),%d)),"
	  		                                   "ST_MinX(MakePoint(%2.7f,%2.7f,%d)),"
                                        "ST_MinY(MakePoint(%2.7f,%2.7f,%d)),%d)) AS bbox_ewkt,%s",
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,
                                        min_x,min_y,i_srid,min_x,min_y,i_srid,i_srid,wsg84_ewkt);
    sql = sqlite3_mprintf("SELECT (ST_MaxX(MakePoint(%2.7f+(%d*%2.7f),%2.7f+(%d*%2.7f)%d))-ST_MinX(MakePoint(%2.7f,%2.7f,%d))) AS width_max,"
                                        "(ST_MaxX(MakePoint(%2.7f+(%d*(%2.7f*8)),%2.7f+(%d*(%2.7f*8)),%d))-ST_MinX(MakePoint(%2.7f,%2.7f,%d))) AS width_min,"
                                        "(ST_MaxY(MakePoint(%2.7f+(%d*%2.7f),%2.7f+(%d*%2.7f)%d))-ST_MinY(MakePoint(%2.7f,%2.7f,%d))) AS height_max,"
                                        "(ST_MaxY(MakePoint(%2.7f+(%d*(%2.7f*8)),%2.7f+(%d*(%2.7f*8)),%d))-ST_MinY(MakePoint(%2.7f,%2.7f,%d))) AS height_min,%s",
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,min_x,min_y,i_srid,
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,min_x,min_y,i_srid,
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,min_x,min_y,i_srid,
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,min_x,min_y,i_srid,geometry_ewkt);   
   }
   else
   { // 
    geometry_ewkt = sqlite3_mprintf("AsEWKT(BuildMbr(ST_MaxX(ST_Transform(MakePoint(%2.7f+(%d*%2.7f),%2.7f+(%d*%2.7f),%d),%d)),"
                                        "ST_MaxY(ST_Transform(MakePoint(%2.7f+(%d*%2.7f),%2.7f+(%d*%2.7f),%d),%d)),"
	  		                                   "ST_MinX(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d)),"
                                        "ST_MinY(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d)),%d)) AS bbox_ewkt,%s",
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,i_srid_mercator,
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,i_srid_mercator,
                                        min_x,min_y,i_srid,i_srid_mercator,min_x,min_y,i_srid,i_srid_mercator,i_srid_mercator,
                                        wsg84_ewkt);
    sql = sqlite3_mprintf("SELECT (ST_MaxX(ST_Transform(MakePoint(%2.7f+(%d*%2.7f),%2.7f+(%d*%2.7f),%d),%d))-"
	  		                                   "ST_MinX(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d))) AS width_max,"
                                        "(ST_MaxX(ST_Transform(MakePoint(%2.7f+(%d*(%2.7f*8)),%2.7f+(%d*(%2.7f*8)),%d),%d))-"
                                        "ST_MinX(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d))) AS width_min,"
                                        "(ST_MaxY(ST_Transform(MakePoint(%2.7f+(%d*%2.7f),%2.7f+(%d*%2.7f),%d),%d))-"
	  		                                   "ST_MinY(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d))) AS height_max,"
                                        "(ST_MaxY(ST_Transform(MakePoint(%2.7f+(%d*(%2.7f*8)),%2.7f+(%d*(%2.7f*8)),%d),%d))-"
                                        "ST_MinY(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d))) AS height_min,%s",
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,i_srid_mercator,
                                        min_x,min_y,i_srid,i_srid_mercator,
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,i_srid_mercator,
                                        min_x,min_y,i_srid,i_srid_mercator,
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,i_srid_mercator,
                                        min_x,min_y,i_srid,i_srid_mercator,
                                        min_x,i_tile_width,x_res,min_y,i_tile_height,y_res,i_srid,i_srid_mercator,
                                        min_x,min_y,i_srid,i_srid_mercator,geometry_ewkt);   
   }
  }
  else
  { // to be used after tiles have been added
   // Check for *_levels table. When missing (0): tile_width and tile_height are needed
   sql = sqlite3_mprintf("SELECT "
                                        "(SELECT count(name) FROM sqlite_master WHERE type='table' AND name='%s_levels'),"
                                        "(SELECT count(name) FROM sqlite_master WHERE type='table' AND name='%s_section_levels'),"
                                        "(SELECT tile_width FROM raster_coverages WHERE coverage_name='%s'),"
                                        "(SELECT tile_height FROM raster_coverages WHERE coverage_name='%s')",
                                        coverage,coverage,coverage,coverage);
   i_sql_rc = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
   if (i_sql_rc != SQLITE_OK)
   {
    i_have_levels=0;
    fprintf (stderr, "get_zoom_levels_for_resolutions[%d]: SQL error: [%s]\n%s\n",i_rc, sql, sqlite3_errmsg (handle)); 	 
   }
   else
   {
    while (1)
    {
     i_sql_rc=sqlite3_step (stmt);
     if (i_sql_rc == SQLITE_DONE)
      break;
     if (i_sql_rc == SQLITE_ROW)
     {
      i_have_levels = sqlite3_column_int(stmt, 0);
      i_have_section_levels = sqlite3_column_int(stmt, 1);
      tile_width = sqlite3_column_int(stmt, 2);
      tile_height = sqlite3_column_int(stmt, 3);
     }
    }
    if (stmt != NULL)
  	  sqlite3_finalize (stmt);
   }
   sqlite3_free(sql);
   sql=NULL;
   if (i_have_levels == 1) 
   {    
    coverage_section=sqlite3_mprintf("%s",coverage);
   }
   if ((i_have_levels == 0) && (i_have_section_levels == 1))
   {    
    i_have_levels=1;
    coverage_section=sqlite3_mprintf("%s_section",coverage);
   }
   i_have_levels = 0;
   if (i_srid == i_srid_mercator) 
   {
    if (i_have_levels == 0)
    { // This brings the most accurate results  [for zoom-levels]
     geometry_ewkt = sqlite3_mprintf("AsEWKT(geometry) FROM '%s_tiles' ORDER BY pyramid_level ASC LIMIT 1) ,%s",
                                         coverage,wsg84_ewkt);
     // We must make sure that we have complete tiles in width and height
     sql = sqlite3_mprintf("SELECT (SELECT (((ST_MaxX(geometry)-"
                                         "ST_MinX(geometry))/%d)*%d) "
                                         "FROM '%s_tiles' ORDER BY pyramid_level ASC LIMIT 1) AS width_max,"
                                         "(SELECT (((ST_MaxX(geometry)-"
                                         "ST_MinX(geometry))/%d)*%d) "
                                         "FROM '%s_tiles' ORDER BY pyramid_level DESC LIMIT 1  OFFSET 1) AS width_min,"
                                         "(SELECT (((ST_MaxY(geometry)-"
                                         "ST_MinY(geometry))/%d)*%d) "
                                         "FROM '%s_tiles' WHERE pyramid_level in ("
                                         "(SELECT max(pyramid_level) FROM '%s_tiles')) LIMIT 1) AS height_max,"
                                         "(SELECT (((ST_MaxY(geometry)-"
                                         "ST_MinY(geometry))/%d)*%d) "
                                         "FROM '%s_tiles' WHERE pyramid_level in ("
                                         "(SELECT max(pyramid_level) FROM '%s_tiles')) LIMIT 1) AS height_min",
                                         tile_width,i_tile_width,coverage_section,tile_width,i_tile_width,coverage_section,
                                         tile_width,i_tile_width,coverage_section,coverage_section,tile_width,i_tile_width,
                                        coverage_section,coverage_section,geometry_ewkt);
    }
    else
    { // This brings unusable results  [for zoom-levels]
     geometry_ewkt = sqlite3_mprintf("AsEWKT(BuildMbr(ST_MaxX(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%d)),"
                                         "ST_MaxY(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%d)),"
	  	 	                                   "ST_MinX(MakePoint(%2.7f,%2.7f,%d)),"
                                         "ST_MinY(MakePoint(%2.7f,%2.7f,%d)),%d)) AS bbox_ewkt,%s",
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,
                                         min_x,min_y,i_srid,min_x,min_y,i_srid,i_srid,wsg84_ewkt);
     sql = sqlite3_mprintf("SELECT (ST_MaxX(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%d))-ST_MinX(MakePoint(%2.7f,%2.7f,%d))) AS width_max,"
                                         "(ST_MaxX(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level DESC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level DESC LIMIT 1"
                                         ")),%d))-ST_MinX(MakePoint(%2.7f,%2.7f,%d))) AS width_min,"
                                         "(ST_MaxY(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%d))-ST_MinY(MakePoint(%2.7f,%2.7f,%d))) AS height_max,"
                                         "(ST_MaxY(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level DESC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level DESC LIMIT 1"
                                         ")),%d))-ST_MinY(MakePoint(%2.7f,%2.7f,%d))) AS height_min,%s",
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,min_x,min_y,i_srid,
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,min_x,min_y,i_srid,
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,min_x,min_y,i_srid,
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,min_x,min_y,i_srid,geometry_ewkt);
    }
   }
   else
   { 
    if (i_have_levels == 0)
    { // This brings the most accurate results [for zoom-levels]
     if (i_srid == i_srid_dhdn_bbtor) 
     {
      geometry_ewkt = sqlite3_mprintf("AsEWKT(ST_Transform(SetSRID(geometry,%d),%d)) FROM '%s_tiles' ORDER BY pyramid_level ASC LIMIT 1) ,%s",
                                          i_srid_dhdn_bbtor,i_srid_mercator,coverage,wsg84_ewkt);
      // We must make sure that we have complete tiles in width and height
      sql = sqlite3_mprintf("SELECT (SELECT (((ST_MaxX(ST_Transform(SetSRID(geometry,%d),%d))-"
                                          "ST_MinX(ST_Transform(SetSRID(geometry,%d),%d)))/%d)*%d) "
                                          "FROM '%s_tiles' ORDER BY pyramid_level ASC LIMIT 1) AS width_max,"
                                          "(SELECT (((ST_MaxX(ST_Transform(SetSRID(geometry,%d),%d))-"
                                          "ST_MinX(ST_Transform(SetSRID(geometry,%d),%d)))/%d)*%d) "
                                          "FROM '%s_tiles' ORDER BY pyramid_level DESC LIMIT 1  OFFSET 1) AS width_min,"
                                          "(SELECT (((ST_MaxY(ST_Transform(SetSRID(geometry,%d),%d))-"
                                          "ST_MinY(ST_Transform(SetSRID(geometry,%d),%d)))/%d)*%d) "
                                          "FROM '%s_tiles' WHERE pyramid_level in ("
                                          "(SELECT max(pyramid_level) FROM '%s_tiles')) LIMIT 1) AS height_max,"
                                          "(SELECT (((ST_MaxY(ST_Transform(SetSRID(geometry,%d),%d))-"
                                          "ST_MinY(ST_Transform(SetSRID(geometry,%d),%d)))/%d)*%d) "
                                          "FROM '%s_tiles' WHERE pyramid_level in ("
                                          "(SELECT max(pyramid_level) FROM '%s_tiles')) LIMIT 1) AS height_min",
                                          i_srid_dhdn_bbtor,i_srid_mercator,i_srid_dhdn_bbtor,i_srid_mercator,tile_width,i_tile_width,coverage,
                                          i_srid_dhdn_bbtor,i_srid_mercator,i_srid_dhdn_bbtor,i_srid_mercator,tile_width,i_tile_width,coverage,
                                          i_srid_dhdn_bbtor,i_srid_mercator,i_srid_dhdn_bbtor,i_srid_mercator,tile_width,i_tile_width,coverage,coverage,
                                          i_srid_dhdn_bbtor,i_srid_mercator,i_srid_dhdn_bbtor,i_srid_mercator,tile_width,i_tile_width,coverage,coverage,geometry_ewkt);
     }
     else
     { // This brings unusable results [for zoom-levels]
      geometry_ewkt = sqlite3_mprintf("AsEWKT(ST_Transform(geometry,%d)) FROM '%s_tiles' ORDER BY pyramid_level ASC LIMIT 1) ,%s",
                                         i_srid_mercator,coverage,wsg84_ewkt);
      // We must make sure that we have complete tiles in width and height
      sql = sqlite3_mprintf("SELECT (SELECT (((ST_MaxX(ST_Transform(geometry,%d))-"
                                          "ST_MinX(ST_Transform(geometry,%d)))/%d)*%d) "
                                          "FROM '%s_tiles' ORDER BY pyramid_level ASC LIMIT 1) AS width_max,"
                                          "(SELECT (((ST_MaxX(ST_Transform(geometry,%d))-"
                                          "ST_MinX(ST_Transform(geometry,%d)))/%d)*%d) "
                                          "FROM '%s_tiles' ORDER BY pyramid_level DESC LIMIT 1  OFFSET 1) AS width_min,"
                                          "(SELECT (((ST_MaxY(ST_Transform(geometry,%d))-"
                                          "ST_MinY(ST_Transform(geometry,%d)))/%d)*%d) "
                                          "FROM '%s_tiles' WHERE pyramid_level in ("
                                          "(SELECT max(pyramid_level) FROM '%s_tiles')) LIMIT 1) AS height_max,"
                                          "(SELECT (((ST_MaxY(ST_Transform(geometry,%d))-"
                                          "ST_MinY(ST_Transform(geometry,%d)))/%d)*%d) "
                                          "FROM '%s_tiles' WHERE pyramid_level in ("
                                          "(SELECT max(pyramid_level) FROM '%s_tiles')) LIMIT 1) AS height_min",
                                          i_srid_mercator,i_srid_mercator,tile_width,i_tile_width,coverage,
                                          i_srid_mercator,i_srid_mercator,tile_width,i_tile_width,coverage,
                                          i_srid_mercator,i_srid_mercator,tile_width,i_tile_width,coverage,coverage,
                                          i_srid_mercator,i_srid_mercator,tile_width,i_tile_width,coverage,coverage,geometry_ewkt);   
     }
    }
    else
    {
     geometry_ewkt = sqlite3_mprintf("AsEWKT(BuildMbr(ST_MaxX(ST_Transform(MakePoint(%2.7f+(%d*("
                                        "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                        ")),%2.7f+(%d*("
                                        "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                        ")),%d),%d)),"
                                        "ST_MaxY(ST_Transform(MakePoint(%2.7f+(%d*(("
                                        "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                        ")*8)),%2.7f+(%d*("
                                        "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                        ")),%d),%d)),"
	  		                                   "ST_MinX(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d)),"
                                        "ST_MinY(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d)),%d)) AS bbox_ewkt,%s",                                      
                                        min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,i_srid_mercator,
                                        min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,i_srid_mercator,
                                        min_x,min_y,i_srid,i_srid_mercator,min_x,min_y,i_srid,i_srid_mercator,i_srid_mercator,
                                        wsg84_ewkt);
     sql = sqlite3_mprintf("SELECT (ST_MaxX(ST_Transform(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%d),%d))-"
	  		                                    "ST_MinX(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d))) AS width_max,"
                                         "(ST_MaxX(ST_Transform(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level DESC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level DESC LIMIT 1"
                                         ")),%d),%d))-" 
                                         "ST_MinX(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d))) AS width_min,"
                                         "(ST_MaxY(ST_Transform(MakePoint(%2.7f+(%d*("
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level ASC LIMIT 1"
                                         ")),%d),%d))-"
	  		                                    "ST_MinY(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d))) AS height_max," 
                                         "(ST_MaxY(ST_Transform(MakePoint(%2.7f+(%d*(" 
                                         "SELECT x_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level DESC LIMIT 1"
                                         ")),%2.7f+(%d*("
                                         "SELECT y_resolution_1_1 FROM '%s_levels' ORDER BY pyramid_level DESC LIMIT 1"
                                         ")),%d),%d))-"
                                         "ST_MinY(ST_Transform(MakePoint(%2.7f,%2.7f,%d),%d))) AS height_min,%s",
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,i_srid_mercator,
                                         min_x,min_y,i_srid,i_srid_mercator, 
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,i_srid_mercator, 
                                         min_x,min_y,i_srid,i_srid_mercator, 
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,i_srid_mercator,
                                         min_x,min_y,i_srid,i_srid_mercator, 
                                         min_x,i_tile_width,coverage_section,min_y,i_tile_height,coverage_section,i_srid,i_srid_mercator,
                                         min_x,min_y,i_srid,i_srid_mercator,geometry_ewkt); 
    }
   }
  }
  sqlite3_free(wsg84_ewkt);
  wsg84_ewkt=NULL;
  sqlite3_free(geometry_ewkt);
  geometry_ewkt=NULL;
  i_sql_rc = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
  if (i_sql_rc != SQLITE_OK)
  { // -10 sql prepared for SELECT resolution width/height, AsEWKT OR  transform to Wsg84 has failed
   i_rc=-10;
 	 fprintf (stderr, "get_zoom_levels_for_resolutions[%d]: SQL error: [%s]\n%s\n",i_rc, sql, sqlite3_errmsg (handle)); 	 
  }
  else
  {
   while (1)
   {
    i_sql_rc=sqlite3_step (stmt);
    if (i_sql_rc == SQLITE_DONE)
     break;
    if (i_sql_rc == SQLITE_ROW)
    {       
     if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT)
   	 {
      d_width_max = sqlite3_column_double(stmt, 0);
     }
     if (sqlite3_column_type (stmt, 1) == SQLITE_FLOAT)
   	 {
      d_width_min = sqlite3_column_double(stmt, 1);
     }
     if (sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
   	 {
      d_height_max = sqlite3_column_double(stmt, 2);
     }
     if (sqlite3_column_type (stmt, 3) == SQLITE_FLOAT)
   	 {
      d_height_min = sqlite3_column_double(stmt, 3);
     }
     if (sqlite3_column_type (stmt, 4) == SQLITE_TEXT)
   	 {
      geometry_ewkt=(char *)sqlite3_column_text(stmt, 4);
     }
     if (sqlite3_column_type (stmt, 5) == SQLITE_FLOAT)
   	 {
      d_wsg84_x=sqlite3_column_double(stmt, 5);
     }
     if (sqlite3_column_type (stmt, 6) == SQLITE_FLOAT)
   	 {
      d_wsg84_y=sqlite3_column_double(stmt, 6);
     }
    }
   }
   if (stmt != NULL)
  	 sqlite3_finalize (stmt);
   if ((d_width_min == d_width_max) && (d_width_max == 0.0))
   {
    printf ("-I-> rl2_get_zoom_levels_for_resolutions max[%2.7f,%2.7f] min[%2.7f,%2.7f] wsg84[%2.7f,%2.7f] sql[%s] \n",d_width_max,d_height_max  ,d_width_min,d_height_min,d_wsg84_x,d_wsg84_y,sql); 
   }
   if ((result_text_parm == 1) || (result_text_parm == 3))
   {
    if (*result_text != NULL)
    {
     sql = sqlite3_mprintf("%s;sql[%s]",*result_text,sql);
     sqlite3_free(*result_text);
     *result_text=NULL;
    }
    i_length=strlen(sql);
    *result_text=sqlite3_malloc(i_length+1);
    strcpy(*result_text,sql);
   }
   if (sql != NULL)
    sqlite3_free(sql);
   sql=NULL;
   if ((d_width_min == d_width_max) && (d_width_max == 0.0))
   {  // -11 0 values for width and height from query returned
     i_rc=-11;
   }
   else
   {    
    *zoom_min=255;
    *zoom_default=255;
    *zoom_max=255;
    for (zoom_level=0;zoom_level<=zl_max;zoom_level++)
    { // Based on results tested with geopaparazzi
     double meters=zl_meters_width[zoom_level];
     if (*zoom_min == 255)
     { // Adjustment to avoid incompleate areas
      if (meters < d_width_min)
       *zoom_min=zoom_level+2;
     }
     if (*zoom_default == 255)
     { // corresponds to the original raster_coverage 'horz_resolution' value
      if (meters < d_width_max)
       *zoom_default=zoom_level;
     }
    }
    // Sanity checks
    if (*zoom_min == 255)
     *zoom_min= 0;
    if (*zoom_min > *zoom_default)
    { // switch values [should never happen]
     *zoom_max=*zoom_default;
     *zoom_default=*zoom_min;
     *zoom_min=*zoom_max;
    }
    if (*zoom_default > zl_max)
     *zoom_default=((zl_max-*zoom_min)/2);
    *zoom_max=*zoom_default+(*zoom_default-*zoom_min);
    if (*zoom_max > zl_max)
     *zoom_max = zl_max;
    //-------------------------------------------
    sql = sqlite3_mprintf("SELECT cast(floor(((%2.7f+180)/360)*(1<<%d)) AS int) AS tile_x,"
                                                       "cast(floor(((1.0-LN(TAN(RADIANS(%2.7f))+1.0/COS(RADIANS(%2.7f)))/PI())/2.0)*(1 << %d)) AS int) AS tile_y_osm,"
                                                       "cast((Pow(2,%d)-cast(floor(((1.0-LN(TAN(RADIANS(%2.7f))+1.0/COS(RADIANS(%2.7f)))/PI())/2.0)*(1 << %d)) AS int))-1 AS int) AS    tile_y_tms",
                                                       d_wsg84_x,*zoom_default,d_wsg84_y,d_wsg84_y,*zoom_default,
                                                       *zoom_default,d_wsg84_y,d_wsg84_y,*zoom_default);
    //-------------------------------------------
    i_sql_rc = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (i_sql_rc != SQLITE_OK)
    { // -12 sql prepared for SELECT Wsg84 to tile z/x/y  query has failed
     i_rc=-12;
  	  fprintf (stderr, "get_zoom_levels_for_resolutions[%d]: SQL error: [%s]\n%s\n",i_rc, sql, sqlite3_errmsg (handle));  	 
    }
    else
    {
     while (1)
     {
      i_sql_rc=sqlite3_step (stmt);
      if (i_sql_rc == SQLITE_DONE)
       break;
      if (i_sql_rc == SQLITE_ROW)
      {
       if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
     	 {
        i_tile_x = sqlite3_column_int(stmt, 0);
       }
       if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
     	 {
        i_tile_y_osm = sqlite3_column_int(stmt, 1);
       }
       if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
     	 {
        i_tile_y_tms = sqlite3_column_int(stmt, 2);
       }
      }
     }
     if (stmt != NULL)
    	 sqlite3_finalize (stmt);
     //-------------------------------------------
     tile_zxy = sqlite3_mprintf("%d/%d/%d tms[%d]",*zoom_default,i_tile_x,i_tile_y_osm,i_tile_y_tms);
     //-------------------------------------------
     if ((result_text_parm == 2) || (result_text_parm == 3))
     {
      if (*result_text != NULL)
      {
       sql = sqlite3_mprintf("%s;meters: min[%2.7f,%2.7f] max[%2.7f,%2.7f] tile_zxy[%s] ewkt[%s]",*result_text,d_width_min,d_height_min,
                                           d_width_max,d_height_max,tile_zxy,geometry_ewkt);
       sqlite3_free(*result_text);
       *result_text=NULL;
      }
      else
       sql = sqlite3_mprintf("width_meters: min[%2.7f,%2.7f] max[%2.7f,%2.7f] tile_zxy[%s]  ewkt[%s]",d_width_min,d_height_min,
                                          d_width_max,d_height_max,tile_zxy,geometry_ewkt);
      i_length=strlen(sql);
      *result_text=sqlite3_malloc(i_length+1);
      strcpy(*result_text,sql);  
     }
     // printf ("-I-> rl2_get_zoom_levels_for_resolutions  tile_zxy[%s] \n",tile_zxy); 
     if (sql != NULL)
      sqlite3_free(sql);
     sql=NULL;
     if (tile_zxy != NULL)
      sqlite3_free(tile_zxy);
     tile_zxy=NULL;
    }
   }
  }
 }
 // printf ("-I-> rl2_get_zoom_levels_for_resolutions rc=%d \n",i_rc); 
 switch (i_rc)
 {
  case RL2_OK: // 0
  break;
  // (-1 to -9 parameter errors, -10 to -99) from rl2_get_zoom_levels_for_resolutions
  // parameter erros, srid=0 OR pointers to zoom_* return values NULL
  case -1:
  // -10 sql prepared for SELECT resolution width/height, AsEWKT OR  transform to Wsg84 has failed
  case -10:
  // -11 0 values for width and height from query returned
  case -11:
  // -12 sql prepared for SELECT Wsg84 to tile z/x/y  query has failed
  case -12:
  default:
   // i_rc=RL2_ERROR; // -1 considered as an error
  break;
 }
  //-------------------------------------------
 return  i_rc;
}

/**
 Will retrieve and store the approximate Zoom-Level and center point of a  rl2PrivRasterDefaults

 \param handle Database connection
 \param section_stats to store the values inside the 
 \param coverage_name name of the coverage
 \param srid of the coverage
 \param center_x of the coverage
 \param center_y of the coverage
 
 \note Goals:
  - calls rl2_get_zoom_levels_for_resolutions

 */
RL2_DECLARE int
rl2_get_defaults_stats (sqlite3 * handle, rl2RasterStatisticsPtr section_stats, const char *coverage_name,int coverage_srid,double center_x,double center_y)
{
 int i_rc=RL2_ERROR;
 unsigned char zoom_min=0;
 unsigned char zoom_default=0;
 unsigned char zoom_max=0;
 rl2PrivRasterDefaultsPtr raster_defaults=NULL;
 if (coverage_name != NULL)
 {
  i_rc=rl2_get_zoom_levels_for_resolutions(handle,coverage_name,coverage_srid,0.0,0.0,center_x,center_y,
                                                                       &zoom_min,&zoom_default,&zoom_max,NULL,NULL) ;
  if (i_rc == RL2_OK)
  {       
   if (section_stats != NULL)
   {   
    raster_defaults=((rl2PrivRasterStatisticsPtr)section_stats)->raster_defaults;
    if (raster_defaults != NULL)
    {
     raster_defaults->center_x=center_x;
     raster_defaults->center_y=center_y;
     raster_defaults->zoom_min=zoom_min;
     raster_defaults->zoom_default=zoom_default;
     raster_defaults->zoom_max=zoom_max;
     i_rc=RL2_OK;
    }
    else
    { // could not retrieve RasterDefaults pointer from sent  RasterStatistics
     i_rc=-3;
    }
   }
   else
   { // sent RasterStatistics is NULL
    i_rc=-2;
   }
  }
  else
  { // zoom_levels_for_resolutions has failed
   i_rc+=-100;
  }
 }
 else
 { // Sent coverage-name is NULL
  i_rc=-1;
 }
 // printf ("-I-> rl2_get_defaults_stats rc=%d \n",i_rc); 
 switch (i_rc)
 {
  // rl2RasterStatisticsPtr was retrieved correctly and rl2PrivRasterStatisticsPtr NOT NULL and center and zoom-levels is set
  case RL2_OK: // 0
  break;
  // (-1 to -9 parameter errors, -10 to -99) from rl2_get_defaults_stats
  // Sent coverage-name is NULL
  case -1:
  // zoom_levels_for_resolutions has failed
  // sent RasterStatistics is NULL
  case -2:
  // could not retrieve RasterDefaults pointer from sent  RasterStatistics
  case -3:
   // (-1 to -9 parameter errors, -10 to -99) from rl2_get_zoom_levels_for_resolutions
  // parameter erros, srid=0 OR pointers to zoom_* return values NULL
  case -101:
  // -10 sql prepared for SELECT resolution width/height, AsEWKT OR  transform to Wsg84 has failed
  case -110:
  // -11 0 values for width and height from query returned
  case -111:
  // -12 sql prepared for SELECT Wsg84 to tile z/x/y  query has failed
  case -112:
  default:
   // i_rc=RL2_ERROR; // -1 considered as an error
  break;
 }
 return i_rc;
}

/**
 Will retrieve the raster_coverage statistics, with the rl2PrivRasterDefaults
 - to read and reset the Zoom-Level and center point 
 
 \param handle Database connection
 \param coverage_name name of the coverage
 \param srid of the coverage
 
 */
RL2_DECLARE int
rl2_get_raster_coverage_stats (sqlite3 * handle, const char *coverage_name, rl2RasterStatisticsPtr *coverage_stats,int *coverage_srid)
{
 sqlite3_stmt *stmt_stats_in = NULL;
 char * sql;
 unsigned char *blob_stats;
 rl2PrivRasterDefaultsPtr coverage_raster_defaults=NULL;
 int blob_stats_sz=0;
 int i_rc=RL2_OK;
 int i_sql_rc=0;
 double center_x=0.0;
 double center_y=0.0;
 if (coverage_name == NULL)
  i_rc=-1;
 if (coverage_srid == NULL)
  i_rc=-2;
 if (coverage_stats == NULL)
  i_rc=-3;
 if (i_rc == RL2_OK)
 {
  sql = sqlite3_mprintf ("SELECT statistics, srid, extent_minx+((extent_maxx-extent_minx)/2), extent_miny+((extent_maxy-extent_miny)/2)  "
                                       "FROM raster_coverages WHERE coverage_name = Lower(%Q) ", coverage_name);
  i_sql_rc = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_in, NULL);
  sqlite3_free (sql);
  if (i_sql_rc != SQLITE_OK)
  { // sqlite prepair for SELECT  statistics, srid from coverage has failed
   i_rc=-4; 
 	 printf ("rl2_get_raster_coverage_stats[%d] SQL error: %s\n%s\n",i_rc,sql,sqlite3_errmsg (handle));		
  } 
  if (i_rc == RL2_OK)
  {
   while (1)
   { // querying the statistics 
  	 i_sql_rc = sqlite3_step (stmt_stats_in);
  	 if (i_sql_rc == SQLITE_DONE)
  	  break;
    if (i_sql_rc == SQLITE_ROW)
  	 {
  	  blob_stats = (unsigned char *) sqlite3_column_blob (stmt_stats_in, 0);
  		 blob_stats_sz = sqlite3_column_bytes (stmt_stats_in, 0);    
  		 *coverage_stats = rl2_deserialize_dbms_raster_statistics (blob_stats, blob_stats_sz);
  	 	if (*coverage_stats != NULL)
     {    
      coverage_raster_defaults=((rl2PrivRasterStatisticsPtr)*coverage_stats)->raster_defaults;     
      if (coverage_raster_defaults != NULL)
      { 
       *coverage_srid=sqlite3_column_int (stmt_stats_in, 1);   
       if ((coverage_raster_defaults->center_x == 0.0) &&
            (coverage_raster_defaults->center_y  == 0.0) &&
            (coverage_raster_defaults->zoom_min == 0) &&
            (coverage_raster_defaults->zoom_default == 18) &&
            (coverage_raster_defaults->zoom_max == 30))
       { // This will be an older Blob, retrieve the data, but without an UPDATE
        center_x = sqlite3_column_double (stmt_stats_in, 2);
        center_y = sqlite3_column_double (stmt_stats_in, 3);  
        i_rc=rl2_get_defaults_stats(handle,*coverage_stats,coverage_name,*coverage_srid,center_x,center_y);
        if (i_rc == RL2_OK)
        { // this has filled the coverage_stats, but without an UPDATE  [would make the Database unreadble for older versions of the library]
        } 
        else
        {
         i_rc+=-100;
        }
       }  
       else
       {
        i_rc=RL2_OK;
       }    
      }
      else
      {  // could not retrieve RasterDefaults pointer from coverage
       i_rc-6;
      }
     }
     else
     { // deserializing Coverage RasterStatistics has failed
      i_rc-5;
     }
    }
   }
   if (stmt_stats_in != NULL)
    sqlite3_finalize (stmt_stats_in);
  }
 } 
 // printf ("-I-> rl2_get_raster_coverage_stats rc=%d \n",i_rc); 
 switch (i_rc)
 {
  // rl2RasterStatisticsPtr was retrieved correctly and rl2PrivRasterStatisticsPtr NOT NULL and srid is set
  case RL2_OK: // 0
  break;
  // (-1 - -99) from rl2_get_raster_coverage_stats
  // -1: coverage_name was NULL
  case -1:
  // -2:  coverage_srid swas NULL
  case -2:
  // -3: coverage_stats was NULL
  case -3:
  // sqlite prepair for SELECT  statistics, srid from coverage has failed
  case -4:
  // deserializing Coverage RasterStatistics has failed
  case -5:
  // could not retrieve RasterDefaults pointer from coverage
  case -6:
  // (-1 to -9 parameter errors, -10 to -99) from rl2_get_defaults_stats
  // Sent coverage-name is NULL
  case -101:
  // zoom_levels_for_resolutions has failed
  case -102:
  // sent RasterStatistics is NULL
  case -103:
  // could not retrieve RasterDefaults pointer from sent  RasterStatistics
  case -104:
  default:
   // i_rc=RL2_ERROR; // -1 considered as an error
  break;
 }
 return i_rc;
}

/**
 Will update the rl2PrivRasterDefaults portion raster_coverage statistics, with the rl2PrivRasterDefaults
 - to read and reset the Zoom-Level and center point 
 
 \param handle Database connection
 \param coverage_name name of the coverage
 \param defaults_stats storing the changes given and will be freed with  rl2_destroy_raster_statistics
 \param srid of the coverage
 
 \note when setting the center point, the srid of the raster_coverage
 \note other portions of the raster_coverage statistics must NOT be touched
 
 */
RL2_DECLARE int
rl2_set_raster_defaults_stats (sqlite3 * handle, const char *coverage_name, rl2RasterStatisticsPtr defaults_stats, int center_srid)
{
 int i_rc=RL2_OK;
 int coverage_srid=0;
 int center_contained=0;
 rl2RasterStatisticsPtr coverage_stats=NULL;
 rl2PrivRasterDefaultsPtr coverage_raster_defaults=NULL;
 rl2PrivRasterDefaultsPtr coverage_defaults_stats=NULL;
 unsigned char zoom_min=0;
 unsigned char zoom_default=0;
 unsigned char zoom_max=0;
 double center_x=0.0;
 double center_y=0.0;
 double coverage_x=0.0;
 double coverage_y=0.0;
 sqlite3_stmt *stmt_stats_in = NULL;
 char * sql;
 int i_sql_rc=0;
 unsigned char *blob_stats;
 int blob_stats_sz;
 if (defaults_stats != NULL)
 { //Sent from caller, will have been changed
  coverage_defaults_stats=((rl2PrivRasterStatisticsPtr)defaults_stats)->raster_defaults;
  if (coverage_defaults_stats != NULL)
  {
   if (rl2_get_raster_coverage_stats(handle,coverage_name,&coverage_stats,&coverage_srid) == RL2_OK)
   { // Reading the Database version from raster_coverage
    coverage_raster_defaults=((rl2PrivRasterStatisticsPtr)coverage_stats)->raster_defaults;
    if (coverage_raster_defaults != NULL)
    {
     if ((coverage_raster_defaults->center_x == 0.0) &&
          (coverage_raster_defaults->center_y  == 0.0) &&
          (coverage_raster_defaults->zoom_min == 0) &&
          (coverage_raster_defaults->zoom_default == 18) &&
          (coverage_raster_defaults->zoom_max == 30))
     { // UPDATE of an older Blob-Type not permitted [would make the Database unreadble for older versions of the library]
      i_rc=-99;
     }
     if (i_rc == RL2_OK)
     {
      if ((coverage_defaults_stats->center_x != coverage_raster_defaults->center_x) ||
           (coverage_defaults_stats->center_y != coverage_raster_defaults->center_y) ||
           (coverage_defaults_stats->zoom_min != coverage_raster_defaults->zoom_min) ||
           (coverage_defaults_stats->zoom_default != coverage_raster_defaults->zoom_default) ||
           (coverage_defaults_stats->zoom_max != coverage_raster_defaults->zoom_max))
      { // Something has changed, UPDATE needed
       center_x=coverage_defaults_stats->center_x;
       center_y=coverage_defaults_stats->center_y;
       zoom_min=coverage_defaults_stats->zoom_min;
       zoom_default=coverage_defaults_stats->zoom_default;
       zoom_max=coverage_defaults_stats->zoom_max;
       if ((center_x == 0.0) && (center_y == 0.0))
       { // Insure that center values are in the raster_coverage srid
        sql = sqlite3_mprintf ("SELECT extent_minx+((extent_maxx-extent_minx)/2), extent_miny+((extent_maxy-extent_miny)/2) "   
                                            	"FROM raster_coverages WHERE Lower(coverage_name) = Lower('%s')",
                                              coverage_name);
        i_sql_rc = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_in, NULL);      
        
        if (i_sql_rc == SQLITE_OK)
        {
         while (1)
         {
          i_sql_rc = sqlite3_step (stmt_stats_in);
	         if (i_sql_rc == SQLITE_DONE)
	          break;
          if (i_sql_rc == SQLITE_ROW)
	         {
           center_x = sqlite3_column_double (stmt_stats_in, 0);
           center_y = sqlite3_column_double (stmt_stats_in, 1);
          }
         }
	        sqlite3_finalize (stmt_stats_in);
         stmt_stats_in = NULL;
        }
        else
        { // sql failed while retrieving center from extent
         i_rc=-6;
         printf ("-E-> rl2_set_raster_defaults_stats[%s,%d] sql[%s] x_y[%2.7f,%2.7f]\n",coverage_name,i_rc,sql,center_x,center_y); 
        }
        sqlite3_free(sql);
       }
       if (center_srid == 0)
        center_srid=coverage_srid;
       if ((center_srid != coverage_srid) && (center_srid > 0))
       { // Insure that center values are in the raster_coverage srid
        sql = sqlite3_mprintf ("SELECT ST_X(ST_Transform(MakePoint(?,?,%d),%d)), ST_X(ST_Transform(MakePoint(?,?,%d),%d))", 
                                              center_srid,coverage_srid,center_srid,coverage_srid);
        i_sql_rc = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_in, NULL);
        if (i_sql_rc == SQLITE_OK)
        {
         sqlite3_reset (stmt_stats_in);
         sqlite3_clear_bindings (stmt_stats_in);
         sqlite3_bind_double (stmt_stats_in, 1,center_x);
        sqlite3_bind_double (stmt_stats_in, 2, center_y);
         sqlite3_bind_double (stmt_stats_in, 3,center_x);
         sqlite3_bind_double (stmt_stats_in, 4, center_y);
         while (1)
         {
          i_rc = sqlite3_step (stmt_stats_in);
	         if (i_rc == SQLITE_DONE)
	          break;
          if (i_rc == SQLITE_ROW)
	         {
           center_x = sqlite3_column_double (stmt_stats_in, 0);
           center_y = sqlite3_column_double (stmt_stats_in, 1);
          }
         }
	        sqlite3_finalize (stmt_stats_in);
         stmt_stats_in = NULL;
        }
        else
        { // sql failed while transforming
         i_rc=-7;
        //printf ("-I-> rl2_set_raster_defaults_stats[%s,%d] srid[%d,%d]  sql[%s]  x_y[%2.7f,%2.7f]\n",coverage_name,i_rc,center_srid,coverage_srid,sql,center_x,center_y); 
        }
        sqlite3_free(sql);
       }
       if ((center_x != coverage_raster_defaults->center_x) ||
            (center_y != coverage_raster_defaults->center_y))
       { // final check if UPDATE is needed
        sql = sqlite3_mprintf ("SELECT ST_Contains(("
                                            "SELECT geometry FROM '%s_sections' ORDER BY section_id ASC LIMIT 1"
                                             "),MakePoint(?,?,%d))", coverage_name,coverage_srid);
        i_sql_rc = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_in, NULL);
        if (i_sql_rc == SQLITE_OK)
        { // SELECT ST_Contains((SELECT geometry FROM '1926.berlin_eiskeller.cutline_sections' ORDER BY section_id ASC LIMIT  1),MakePoint(6820.87,28707.18,3068))
         sqlite3_reset (stmt_stats_in);
         sqlite3_clear_bindings (stmt_stats_in);
         sqlite3_bind_double (stmt_stats_in, 1,center_x);
         sqlite3_bind_double (stmt_stats_in, 2, center_y);
         while (1)
         {
          i_sql_rc = sqlite3_step (stmt_stats_in);
	         if (i_sql_rc == SQLITE_DONE)
	          break;
          if (i_sql_rc == SQLITE_ROW)
	         {
           center_contained = sqlite3_column_int (stmt_stats_in, 0);
           //printf ("-I-> rl2_set_raster_defaults_stats[%s,%d] 01 sql[%s] x_y[%2.7f,%2.7f]\n",coverage_name,center_contained,sql,center_x,center_y); 
          }
         }
	        sqlite3_finalize (stmt_stats_in);
         stmt_stats_in = NULL;
        }  
        else
        { // sqlite prepared failed for ST_Contains
         i_rc=-8;
         printf ("-E-> rl2_set_raster_defaults_stats[%s,%d] prepared failed for sql[%s] x_y[%2.7f,%2.7f]\n",coverage_name,center_contained,sql,center_x,center_y); 
        }   
        sqlite3_free(sql);  
       }
       else
       { // Sent point has not change, not an error
        center_contained = 1;
       }
       if (center_contained == 1)
       { // Only if given point is inside inside the extent
        if ((zoom_min >=0) && (zoom_max <=30) && (zoom_min < zoom_max))
        { // Sanity checks of zoom-levels [min/default/max : smallest/bigger/biggest: none the same]
         if ((zoom_min < zoom_default) && (zoom_max > zoom_default))
         {  // Sane values, UPDATE RasterDefaults portions of RasterStatistics
          coverage_raster_defaults->center_x=center_x;
          coverage_raster_defaults->center_y=center_y;
          coverage_raster_defaults->zoom_min=zoom_min;
          coverage_raster_defaults->zoom_default=zoom_default;
          coverage_raster_defaults->zoom_max=zoom_max;
          rl2_serialize_dbms_raster_statistics (coverage_stats, &blob_stats,&blob_stats_sz);
          if ((blob_stats != NULL) && (blob_stats_sz > 0))
          {          
           sql = sqlite3_mprintf ("UPDATE raster_coverages SET statistics=? WHERE coverage_name = Lower(%Q) ", coverage_name);
           i_sql_rc = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_in, NULL);          
           if (i_sql_rc == SQLITE_OK)
           {
            sqlite3_reset (stmt_stats_in);
            sqlite3_clear_bindings (stmt_stats_in);
            sqlite3_bind_blob (stmt_stats_in, 1, blob_stats,blob_stats_sz,free);
            i_sql_rc = sqlite3_step (stmt_stats_in);
            if ((i_sql_rc == SQLITE_DONE) || (i_sql_rc == SQLITE_ROW))
            {
             i_rc=RL2_OK;
            }
            else
            { // sqlite update has failed
              i_rc=-14;
              printf ("-E-> rl2_set_raster_defaults_stats[%s] update failed for sql[%s] \n",coverage_name,sql); 
            }
            sqlite3_finalize (stmt_stats_in);
            stmt_stats_in = NULL;
           }  
           else
           { // sqlite prepare for update has failed
             i_rc=-13;
             printf ("-E-> rl2_set_raster_defaults_stats[%s] prepared failed for sql[%s] \n",coverage_name,sql); 
           }      
           sqlite3_free(sql);  
          }
          else
          { // serializization of adapted coverage RasterStatistics has failed
           i_rc=-12;
          }
         }
         else
         { // default zoom is not between min an max
          i_rc=-11;
         }
        }
        else
        { // min/max zooms are out of range [0-30]
         i_rc=-10;
        }
       }
       else
       { // Sent point is outside the coverage
        i_rc=-9;
       }
      }
      else
      { // nothing has changed, no UPDATE needed
       i_rc=-5;
      }
     }
    }
    else
    { // could not retrieve RasterDefaults pointer from coverage
     i_rc=-4;
    }
    rl2_destroy_raster_statistics (coverage_stats);
    coverage_stats=NULL;
   }  
   else
   { // Coverage RasterStatistics is NULL
    i_rc=-3;
   } 
  }  
  else
  { // could not retrieve RasterDefaults pointer from sent RasterStatistics
   i_rc=-2;
  }
  rl2_destroy_raster_statistics (defaults_stats);
  defaults_stats=NULL;
 } 
 else
 { // Sent RasterStatistics is NULL
  i_rc=-1;
 }
 switch (i_rc)
 {
  // sqlite update was compleated correctly
  case RL2_OK: // 0
  break;
  //  (-1 - -99) rl2_set_raster_defaults_stats
  // nothing has changed, no UPDATE needed
  case -5:
   // i_rc=0; // NOT considered as an error
  break;
  //  (-1 - -99) rl2_set_raster_defaults_stats
  // Sent RasterStatistics is NULL
  case -1:
  // could not retrieve RasterDefaults pointer from sent RasterStatistics
  case -2:
  // Coverage RasterStatistics is NULL
  case -3:
  // could not retrieve RasterDefaults pointer from coverage
  case -4:
  // sql failed while retrieving center from extent
  case -6:
  // sql failed while transforming
  case -7:
  // sqlite prepared failed for ST_Contains
  case -8:
  // Sent point is outside the coverage
  case -9:
  // min/max zooms are out of range [0-30]
  case -10:
  // default zoom is not between min an max
  case -11:
  // serializization of adapted coverage RasterStatistics has failed
  case -12:
  // sqlite prepare for update has failed
  case -13:
  // sqlite update has failed
  case -14:
  // UPDATE of an older Blob-Type not permitted
  case -99:
  default:
   // i_rc=RL2_ERROR; // -1 considered as an error
  break;
 }
 // printf ("-I-> rl2_set_raster_defaults_stats[%s]  rc=%d \n",coverage_name,i_rc); 
 return i_rc;
}

static int
insert_into_raster_coverages (sqlite3 * handle, const char *coverage,
			      unsigned char sample, unsigned char pixel,
			      unsigned char num_bands,
			      unsigned char compression, int quality,
			      unsigned int tile_width,
			      unsigned int tile_height, int srid,
			      double x_res, double y_res,
			      unsigned char *blob, int blob_sz,
			      unsigned char *blob_no_data,
			      int blob_no_data_sz, int strict_resolution,
			      int mixed_resolutions, int section_paths,
			      int section_md5, int section_summary)
{
/* inserting into "raster_coverages" */
    int ret;
    char *sql;
    sqlite3_stmt *stmt;
    const char *xsample = "UNKNOWN";
    const char *xpixel = "UNKNOWN";
    const char *xcompression = "UNKNOWN";

    sql = "INSERT INTO raster_coverages (coverage_name, sample_type, "
	"pixel_type, num_bands, compression, quality, tile_width, "
	"tile_height, horz_resolution, vert_resolution, srid, "
	"nodata_pixel, palette, strict_resolution, mixed_resolutions, "
	"section_paths, section_md5, section_summary) VALUES "
	"(Lower(?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  return 0;
      }
    switch (sample)
      {
      case RL2_SAMPLE_1_BIT:
	  xsample = "1-BIT";
	  break;
      case RL2_SAMPLE_2_BIT:
	  xsample = "2-BIT";
	  break;
      case RL2_SAMPLE_4_BIT:
	  xsample = "4-BIT";
	  break;
      case RL2_SAMPLE_INT8:
	  xsample = "INT8";
	  break;
      case RL2_SAMPLE_UINT8:
	  xsample = "UINT8";
	  break;
      case RL2_SAMPLE_INT16:
	  xsample = "INT16";
	  break;
      case RL2_SAMPLE_UINT16:
	  xsample = "UINT16";
	  break;
      case RL2_SAMPLE_INT32:
	  xsample = "INT32";
	  break;
      case RL2_SAMPLE_UINT32:
	  xsample = "UINT32";
	  break;
      case RL2_SAMPLE_FLOAT:
	  xsample = "FLOAT";
	  break;
      case RL2_SAMPLE_DOUBLE:
	  xsample = "DOUBLE";
	  break;
      };
    switch (pixel)
      {
      case RL2_PIXEL_MONOCHROME:
	  xpixel = "MONOCHROME";
	  break;
      case RL2_PIXEL_PALETTE:
	  xpixel = "PALETTE";
	  break;
      case RL2_PIXEL_GRAYSCALE:
	  xpixel = "GRAYSCALE";
	  break;
      case RL2_PIXEL_RGB:
	  xpixel = "RGB";
	  break;
      case RL2_PIXEL_MULTIBAND:
	  xpixel = "MULTIBAND";
	  break;
      case RL2_PIXEL_DATAGRID:
	  xpixel = "DATAGRID";
	  break;
      };
    switch (compression)
      {
      case RL2_COMPRESSION_NONE:
	  xcompression = "NONE";
	  break;
      case RL2_COMPRESSION_DEFLATE:
	  xcompression = "DEFLATE";
	  break;
      case RL2_COMPRESSION_DEFLATE_NO:
	  xcompression = "DEFLATE_NO";
	  break;
      case RL2_COMPRESSION_LZMA:
	  xcompression = "LZMA";
	  break;
      case RL2_COMPRESSION_LZMA_NO:
	  xcompression = "LZMA_NO";
	  break;
      case RL2_COMPRESSION_PNG:
	  xcompression = "PNG";
	  break;
      case RL2_COMPRESSION_JPEG:
	  xcompression = "JPEG";
	  break;
      case RL2_COMPRESSION_LOSSY_WEBP:
	  xcompression = "LOSSY_WEBP";
	  break;
      case RL2_COMPRESSION_LOSSLESS_WEBP:
	  xcompression = "LOSSLESS_WEBP";
	  break;
      case RL2_COMPRESSION_CCITTFAX4:
	  xcompression = "CCITTFAX4";
	  break;
      case RL2_COMPRESSION_CHARLS:
	  xcompression = "CHARLS";
	  break;
      case RL2_COMPRESSION_LOSSY_JP2:
	  xcompression = "LOSSY_JP2";
	  break;
      case RL2_COMPRESSION_LOSSLESS_JP2:
	  xcompression = "LOSSLESS_JP2";
	  break;
      };
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, xsample, strlen (xsample), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, xpixel, strlen (xpixel), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 4, num_bands);
    sqlite3_bind_text (stmt, 5, xcompression, strlen (xcompression),
		       SQLITE_STATIC);
    sqlite3_bind_int (stmt, 6, quality);
    sqlite3_bind_int (stmt, 7, tile_width);
    sqlite3_bind_int (stmt, 8, tile_height);
    if (mixed_resolutions)
      {
	  /* conventional setting for mixed resolutions Coverage */
	  sqlite3_bind_double (stmt, 9, 999999.999999);
	  sqlite3_bind_double (stmt, 10, 999999.999999);
      }
    else
      {
	  /* real resolution in any other case */
	  sqlite3_bind_double (stmt, 9, x_res);
	  sqlite3_bind_double (stmt, 10, y_res);
      }
    sqlite3_bind_int (stmt, 11, srid);
    if (blob_no_data == NULL)
	sqlite3_bind_null (stmt, 12);
    else
	sqlite3_bind_blob (stmt, 12, blob_no_data, blob_no_data_sz, free);
    if (blob == NULL)
	sqlite3_bind_null (stmt, 13);
    else
	sqlite3_bind_blob (stmt, 13, blob, blob_sz, free);
    sqlite3_bind_int (stmt, 14, strict_resolution);
    sqlite3_bind_int (stmt, 15, mixed_resolutions);
    sqlite3_bind_int (stmt, 16, section_paths);
    sqlite3_bind_int (stmt, 17, section_md5);
    sqlite3_bind_int (stmt, 18, section_summary);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	goto coverage_registered;
    fprintf (stderr,
	     "sqlite3_step() error: INSERT INTO raster_coverages \"%s\"\n",
	     sqlite3_errmsg (handle));
    sqlite3_finalize (stmt);
    return 0;
  coverage_registered:
    sqlite3_finalize (stmt);
    return 1;
}

static int
create_levels (sqlite3 * handle, const char *coverage)
{
/* creating the LEVELS table */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *xcoverage;
    char *xxcoverage;

    xcoverage = sqlite3_mprintf ("%s_levels", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "\tpyramid_level INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tx_resolution_1_1 DOUBLE NOT NULL,\n"
			   "\ty_resolution_1_1 DOUBLE NOT NULL,\n"
			   "\tx_resolution_1_2 DOUBLE,\n"
			   "\ty_resolution_1_2 DOUBLE,\n"
			   "\tx_resolution_1_4 DOUBLE,\n"
			   "\ty_resolution_1_4 DOUBLE,\n"
			   "\tx_resolution_1_8 DOUBLE,\n"
			   "\ty_resolution_1_8 DOUBLE)\n", xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s_levels\" error: %s\n",
		   xxcoverage, sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);
    return 1;
}

static int
create_section_levels (sqlite3 * handle, const char *coverage)
{
/* creating the SECTION_LEVELS table */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *xcoverage;
    char *xxcoverage;
    char *pk_name;
    char *xpk_name;
    char *fk_name;
    char *xfk_name;
    char *mother;
    char *xmother;

    xcoverage = sqlite3_mprintf ("%s_section_levels", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    pk_name = sqlite3_mprintf ("pk_%s_sectlevela", coverage);
    xpk_name = rl2_double_quoted_sql (pk_name);
    sqlite3_free (pk_name);
    fk_name = sqlite3_mprintf ("fk_%s_sectlevels", coverage);
    xfk_name = rl2_double_quoted_sql (fk_name);
    sqlite3_free (fk_name);
    mother = sqlite3_mprintf ("%s_sections", coverage);
    xmother = rl2_double_quoted_sql (mother);
    sqlite3_free (mother);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tsection_id INTEGER NOT NULL,\n"
			   "\tpyramid_level INTEGER NOT NULL,\n"
			   "\tx_resolution_1_1 DOUBLE NOT NULL,\n"
			   "\ty_resolution_1_1 DOUBLE NOT NULL,\n"
			   "\tx_resolution_1_2 DOUBLE,\n"
			   "\ty_resolution_1_2 DOUBLE,\n"
			   "\tx_resolution_1_4 DOUBLE,\n"
			   "\ty_resolution_1_4 DOUBLE,\n"
			   "\tx_resolution_1_8 DOUBLE,\n"
			   "\ty_resolution_1_8 DOUBLE,\n"
			   "\tCONSTRAINT \"%s\" PRIMARY KEY (section_id, pyramid_level)\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (section_id) "
			   "REFERENCES \"%s\" (section_id))\n", xxcoverage,
			   xpk_name, xfk_name, xmother);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    free (xpk_name);
    free (xfk_name);
    free (xmother);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s_levels\" error: %s\n",
		   xxcoverage, sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);
    return 1;
}

static int
create_sections (sqlite3 * handle, const char *coverage, int srid)
{
/* creating the SECTIONS table */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *xcoverage;
    char *xxcoverage;
    char *xindex;
    char *xxindex;
    char *xtrigger;
    char *xxtrigger;

/* creating the SECTIONS table */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "\tsection_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tsection_name TEXT NOT NULL,\n"
			   "\twidth INTEGER NOT NULL,\n"
			   "\theight INTEGER NOT NULL,\n"
			   "\tfile_path TEXT,\n"
			   "\tmd5_checksum TEXT,\n"
			   "\tsummary TEXT,\n"
			   "\tstatistics BLOB)", xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s\" error: %s\n",
		   xxcoverage, sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

/* adding the safeguard Triggers */
    xtrigger = sqlite3_mprintf ("%s_sections_statistics_insert", coverage);
    xxtrigger = rl2_double_quoted_sql (xtrigger);
    sqlite3_free (xtrigger);
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\"\n"
			   "BEFORE INSERT ON %Q\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT,'insert on %s violates constraint: "
			   "invalid statistics')\nWHERE NEW.statistics IS NOT NULL AND "
			   "IsValidRasterStatistics(%Q, NEW.statistics) <> 1;\nEND",
			   xxtrigger, xcoverage, xcoverage, coverage);
    sqlite3_free (xcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TRIGGER \"%s\" error: %s\n", xxtrigger,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxtrigger);
	  return 0;
      }
    free (xxtrigger);
    xtrigger = sqlite3_mprintf ("%s_sections_statistics_update", coverage);
    xxtrigger = rl2_double_quoted_sql (xtrigger);
    sqlite3_free (xtrigger);
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\"\n"
			   "BEFORE UPDATE OF 'statistics' ON %Q"
			   "\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT, 'update on %s violates constraint: "
			   "invalid statistics')\nWHERE NEW.statistics IS NOT NULL AND "
			   "IsValidRasterStatistics(%Q, NEW.statistics) <> 1;\nEND",
			   xxtrigger, xcoverage, xcoverage, coverage);
    sqlite3_free (xcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TRIGGER \"%s\" error: %s\n", xxtrigger,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxtrigger);
	  return 0;
      }
    free (xxtrigger);

/* creating the SECTIONS geometry */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("SELECT AddGeometryColumn("
			   "%Q, 'geometry', %d, 'POLYGON', 'XY')", xcoverage,
			   srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn \"%s\" error: %s\n",
		   xcoverage, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xcoverage);
	  return 0;
      }
    sqlite3_free (xcoverage);

/* creating the SECTIONS spatial index */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex("
			   "%Q, 'geometry')", xcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateSpatialIndex \"%s\" error: %s\n",
		   xcoverage, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xcoverage);
	  return 0;
      }
    sqlite3_free (xcoverage);

/* creating the SECTIONS index by name */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    xindex = sqlite3_mprintf ("idx_%s_sect_name", coverage);
    xxindex = rl2_double_quoted_sql (xindex);
    sqlite3_free (xindex);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (section_name)",
			 xxindex, xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE INDEX \"%s\" error: %s\n", xxindex, sql_err);
	  sqlite3_free (sql_err);
	  free (xxindex);
	  return 0;
      }
    free (xxindex);

/* creating the SECTIONS index by MD5 checksum */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    xindex = sqlite3_mprintf ("idx_%s_sect_md5", coverage);
    xxindex = rl2_double_quoted_sql (xindex);
    sqlite3_free (xindex);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (md5_checksum)",
			 xxindex, xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE INDEX \"%s\" error: %s\n", xxindex, sql_err);
	  sqlite3_free (sql_err);
	  free (xxindex);
	  return 0;
      }
    free (xxindex);
    return 1;
}

static int
create_tiles (sqlite3 * handle, const char *coverage, int srid,
	      int mixed_resolutions)
{
/* creating the TILES table */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *xcoverage;
    char *xxcoverage;
    char *xindex;
    char *xxindex;
    char *xfk;
    char *xxfk;
    char *xmother;
    char *xxmother;
    char *xfk2;
    char *xxfk2;
    char *xmother2;
    char *xxmother2;
    char *xtrigger;
    char *xxtrigger;
    char *xtiles;
    char *xxtiles;

    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    xmother = sqlite3_mprintf ("%s_sections", coverage);
    xxmother = rl2_double_quoted_sql (xmother);
    sqlite3_free (xmother);
    xfk = sqlite3_mprintf ("fk_%s_tiles_section", coverage);
    xxfk = rl2_double_quoted_sql (xfk);
    sqlite3_free (xfk);
    if (mixed_resolutions)
	xmother2 = sqlite3_mprintf ("%s_section_levels", coverage);
    else
	xmother2 = sqlite3_mprintf ("%s_levels", coverage);
    xxmother2 = rl2_double_quoted_sql (xmother2);
    sqlite3_free (xmother2);
    xfk2 = sqlite3_mprintf ("fk_%s_tiles_level", coverage);
    xxfk2 = rl2_double_quoted_sql (xfk2);
    sqlite3_free (xfk2);
    if (mixed_resolutions)
      {
	  sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				 "\ttile_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				 "\tpyramid_level INTEGER NOT NULL,\n"
				 "\tsection_id INTEGER,\n"
				 "\tCONSTRAINT \"%s\" FOREIGN KEY (section_id) "
				 "REFERENCES \"%s\" (section_id) ON DELETE CASCADE,\n"
				 "\tCONSTRAINT \"%s\" FOREIGN KEY (section_id, pyramid_level) "
				 "REFERENCES \"%s\" (section_id, pyramid_level) ON DELETE CASCADE)",
				 xxcoverage, xxfk, xxmother, xxfk2, xxmother2);
      }
    else
      {
	  sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
				 "\ttile_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
				 "\tpyramid_level INTEGER NOT NULL,\n"
				 "\tsection_id INTEGER,\n"
				 "\tCONSTRAINT \"%s\" FOREIGN KEY (section_id) "
				 "REFERENCES \"%s\" (section_id) ON DELETE CASCADE,\n"
				 "\tCONSTRAINT \"%s\" FOREIGN KEY (pyramid_level) "
				 "REFERENCES \"%s\" (pyramid_level) ON DELETE CASCADE)",
				 xxcoverage, xxfk, xxmother, xxfk2, xxmother2);
      }
    free (xxfk);
    free (xxmother);
    free (xxfk2);
    free (xxmother2);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s_tiles\" error: %s\n",
		   xxcoverage, sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

/* creating the TILES geometry */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    sql = sqlite3_mprintf ("SELECT AddGeometryColumn("
			   "%Q, 'geometry', %d, 'POLYGON', 'XY')", xcoverage,
			   srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn \"%s_tiles\" error: %s\n",
		   xcoverage, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xcoverage);
	  return 0;
      }
    sqlite3_free (xcoverage);

/* creating the TILES spatial Index */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex("
			   "%Q, 'geometry')", xcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateSpatialIndex \"%s_tiles\" error: %s\n",
		   xcoverage, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xcoverage);
	  return 0;
      }
    sqlite3_free (xcoverage);

/* creating the TILES index by section */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    xindex = sqlite3_mprintf ("idx_%s_tiles_sect", coverage);
    xxindex = rl2_double_quoted_sql (xindex);
    sqlite3_free (xindex);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (section_id, pyramid_level)", xxindex,
	 xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE INDEX \"%s\" error: %s\n", xxindex, sql_err);
	  sqlite3_free (sql_err);
	  free (xxindex);
	  return 0;
      }
    free (xxindex);

/* creating the TILES index by level */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    xindex = sqlite3_mprintf ("idx_%s_tiles_lev", coverage);
    xxindex = rl2_double_quoted_sql (xindex);
    sqlite3_free (xindex);
    sql =
	sqlite3_mprintf
	("CREATE INDEX \"%s\" ON \"%s\" (pyramid_level, section_id)", xxindex,
	 xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE INDEX \"%s\" error: %s\n", xxindex, sql_err);
	  sqlite3_free (sql_err);
	  free (xxindex);
	  return 0;
      }
    free (xxindex);

/* creating the TILE_DATA table */
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    xmother = sqlite3_mprintf ("%s_tiles", coverage);
    xxmother = rl2_double_quoted_sql (xmother);
    sqlite3_free (xmother);
    xfk = sqlite3_mprintf ("fk_%s_tile_data", coverage);
    xxfk = rl2_double_quoted_sql (xfk);
    sqlite3_free (xfk);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "\ttile_id INTEGER NOT NULL PRIMARY KEY,\n"
			   "\ttile_data_odd BLOB NOT NULL,\n"
			   "\ttile_data_even BLOB,\n"
			   "CONSTRAINT \"%s\" FOREIGN KEY (tile_id) "
			   "REFERENCES \"%s\" (tile_id) ON DELETE CASCADE)",
			   xxcoverage, xxfk, xxmother);
    free (xxfk);
    free (xxmother);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"%s_tile_data\" error: %s\n",
		   xxcoverage, sql_err);
	  sqlite3_free (sql_err);
	  free (xxcoverage);
	  return 0;
      }
    free (xxcoverage);

/* adding the safeguard Triggers */
    xtrigger = sqlite3_mprintf ("%s_tile_data_insert", coverage);
    xxtrigger = rl2_double_quoted_sql (xtrigger);
    sqlite3_free (xtrigger);
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage);
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = rl2_double_quoted_sql (xtiles);
    sqlite3_free (xtiles);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\"\n"
			   "BEFORE INSERT ON %Q\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT,'insert on %s violates constraint: "
			   "invalid tile_data')\nWHERE IsValidRasterTile(%Q, "
			   "(SELECT t.pyramid_level FROM \"%s\" AS t WHERE t.tile_id = NEW.tile_id), "
			   "NEW.tile_data_odd, NEW.tile_data_even) <> 1;\nEND",
			   xxtrigger, xcoverage, xcoverage, coverage, xxtiles);
    sqlite3_free (xcoverage);
    free (xxtiles);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TRIGGER \"%s\" error: %s\n", xxtrigger,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxtrigger);
	  return 0;
      }
    free (xxtrigger);
    xtrigger = sqlite3_mprintf ("%s_tile_data_update", coverage);
    xxtrigger = rl2_double_quoted_sql (xtrigger);
    sqlite3_free (xtrigger);
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage);
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = rl2_double_quoted_sql (xtiles);
    sqlite3_free (xtiles);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\"\n"
			   "BEFORE UPDATE ON %Q\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT, 'update on %s violates constraint: "
			   "invalid tile_data')\nWHERE IsValidRasterTile(%Q, "
			   "(SELECT t.pyramid_level FROM \"%s\" AS t WHERE t.tile_id = NEW.tile_id), "
			   "NEW.tile_data_odd, NEW.tile_data_even) <> 1;\nEND",
			   xxtrigger, xcoverage, xcoverage, coverage, xxtiles);
    sqlite3_free (xcoverage);
    free (xxtiles);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TRIGGER \"%s\" error: %s\n", xxtrigger,
		   sql_err);
	  sqlite3_free (sql_err);
	  free (xxtrigger);
	  return 0;
      }
    free (xxtrigger);
    return 1;
}

RL2_DECLARE int
rl2_create_dbms_coverage (sqlite3 * handle, const char *coverage,
			  unsigned char sample, unsigned char pixel,
			  unsigned char num_bands, unsigned char compression,
			  int quality, unsigned int tile_width,
			  unsigned int tile_height, int srid, double x_res,
			  double y_res, rl2PixelPtr no_data,
			  rl2PalettePtr palette, int strict_resolution,
			  int mixed_resolutions, int section_paths,
			  int section_md5, int section_summary)
{
/* creating a DBMS-based Coverage */
    unsigned char *blob = NULL;
    int blob_size = 0;
    unsigned char *blob_no_data = NULL;
    int blob_no_data_sz = 0;
    if (pixel == RL2_PIXEL_PALETTE)
      {
	  /* installing a default (empty) Palette */
	  if (rl2_serialize_dbms_palette (palette, &blob, &blob_size) != RL2_OK)
	      goto error;
      }
    if (no_data != NULL)
      {
	  if (rl2_serialize_dbms_pixel
	      (no_data, &blob_no_data, &blob_no_data_sz) != RL2_OK)
	      goto error;
      }
    if (!insert_into_raster_coverages
	(handle, coverage, sample, pixel, num_bands, compression, quality,
	 tile_width, tile_height, srid, x_res, y_res, blob,
	 blob_size, blob_no_data, blob_no_data_sz, strict_resolution,
	 mixed_resolutions, section_paths, section_md5, section_summary))
	goto error;
    if (mixed_resolutions)
      {
	  if (!create_section_levels (handle, coverage))
	      goto error;
      }
    else
      {
	  if (!create_levels (handle, coverage))
	      goto error;
      }
    if (!create_sections (handle, coverage, srid))
	goto error;
    if (!create_tiles (handle, coverage, srid, mixed_resolutions))
	goto error;
    return RL2_OK;
  error:
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_set_dbms_coverage_default_bands (sqlite3 * handle, const char *coverage,
				     unsigned char red_band,
				     unsigned char green_band,
				     unsigned char blue_band,
				     unsigned char nir_band)
{
/* 
/  setting up the default Red, Green, Blue and NIR bands
/  will work only for a MULTIBAND coverage 
*/
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int num_bands;
    int count = 0;

    sql = "SELECT num_bands FROM raster_coverages "
	"WHERE Lower(coverage_name) = Lower(?) AND pixel_type = 'MULTIBAND'";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT MultiBand # Bands SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* querying the covrage */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		num_bands = sqlite3_column_int (stmt, 0);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT MultiBand # Bands; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* validation checks */
    if (count != 1)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;
    if (nir_band >= num_bands)
	goto error;
    if (red_band == green_band || red_band == blue_band || red_band == nir_band)
	goto error;
    if (green_band == blue_band || green_band == nir_band)
	goto error;
    if (blue_band == nir_band)
	goto error;

/* updating the Coverage */
    sql = "UPDATE raster_coverages SET red_band_index = ?, "
	"green_band_index = ?, blue_band_index = ?, nir_band_index = ? "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE MultiBand default Bands SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, red_band);
    sqlite3_bind_int (stmt, 2, green_band);
    sqlite3_bind_int (stmt, 3, blue_band);
    sqlite3_bind_int (stmt, 4, nir_band);
    sqlite3_bind_text (stmt, 5, coverage, strlen (coverage), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "sqlite3_step() error: UPDATE MultiBand default Bands \"%s\"\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_finalize (stmt);
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_dbms_coverage_default_bands (sqlite3 * handle, const char *coverage,
				     unsigned char *red_band,
				     unsigned char *green_band,
				     unsigned char *blue_band,
				     unsigned char *nir_band)
{
/* 
/  attempring to retrieve the default Red, Green, Blue and NIR bands
/  will work only for a MULTIBAND coverage 
*/
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int num_bands;
    int red = -1;
    int green = -1;
    int blue = -1;
    int nir = -1;
    int count = 0;

    sql = "SELECT num_bands, red_band_index, green_band_index, "
	"blue_band_index, nir_band_index FROM raster_coverages "
	"WHERE Lower(coverage_name) = Lower(?) AND pixel_type = 'MULTIBAND'";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT MultiBand default Bands SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* querying the covrage */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		num_bands = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		    red = sqlite3_column_int (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
		    green = sqlite3_column_int (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_INTEGER)
		    blue = sqlite3_column_int (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_INTEGER)
		    nir = sqlite3_column_int (stmt, 4);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT MultiBand default Bands; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* validation checks */
    if (count != 1)
	goto error;
    if (red < 0 || red >= num_bands)
	goto error;
    if (green < 0 || green >= num_bands)
	goto error;
    if (blue < 0 || blue >= num_bands)
	goto error;
    if (nir < 0 || nir >= num_bands)
	goto error;
    if (red == green || red == blue || red == nir)
	goto error;
    if (green == blue || green == nir)
	goto error;
    if (blue == nir)
	goto error;

    *red_band = red;
    *green_band = green;
    *blue_band = blue;
    *nir_band = nir;
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_enable_dbms_coverage_auto_ndvi (sqlite3 * handle, const char *coverage,
				    int on_off)
{
/* 
/  enabling / disabling the Auto NDVI capability
/  will work only for a MULTIBAND coverage explicitly declaring
/  default Red, Green, Blue and NIR bands
*/
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    unsigned char red_band;
    unsigned char green_band;
    unsigned char blue_band;
    unsigned char nir_band;

    if (rl2_get_dbms_coverage_default_bands
	(handle, coverage, &red_band, &green_band, &blue_band,
	 &nir_band) != RL2_OK)
	goto error;

/* updating the Coverage */
    sql = "UPDATE raster_coverages SET enable_auto_ndvi = ? "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE Enable Auto NDVI SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (on_off)
	sqlite3_bind_int (stmt, 1, 1);
    else
	sqlite3_bind_int (stmt, 1, 0);
    sqlite3_bind_text (stmt, 2, coverage, strlen (coverage), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "sqlite3_step() error: UPDATE Enable Auto NDVI \"%s\"\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_finalize (stmt);
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_is_dbms_coverage_auto_ndvi_enabled (sqlite3 * handle, const char *coverage)
{
/* 
/  attempring to retrieve if the Auto NDVI feature is enabled
/  will work only for a MULTIBAND coverage 
*/
    int ret;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int num_bands;
    int red = -1;
    int green = -1;
    int blue = -1;
    int nir = -1;
    int enabled = -1;
    int count = 0;

    sql = "SELECT num_bands, red_band_index, green_band_index, "
	"blue_band_index, nir_band_index, enable_auto_ndvi FROM raster_coverages "
	"WHERE Lower(coverage_name) = Lower(?) AND pixel_type = 'MULTIBAND'";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT IsEnabled Auto NDVI SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* querying the covrage */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		num_bands = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		    red = sqlite3_column_int (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
		    green = sqlite3_column_int (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_INTEGER)
		    blue = sqlite3_column_int (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_INTEGER)
		    nir = sqlite3_column_int (stmt, 4);
		if (sqlite3_column_type (stmt, 5) == SQLITE_INTEGER)
		    enabled = sqlite3_column_int (stmt, 5);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT IsEnabled Auto NDVI; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* validation checks */
    if (count != 1)
	goto error;
    if (red < 0 || red >= num_bands)
	goto error;
    if (green < 0 || green >= num_bands)
	goto error;
    if (blue < 0 || blue >= num_bands)
	goto error;
    if (nir < 0 || nir >= num_bands)
	goto error;
    if (red == green || red == blue || red == nir)
	goto error;
    if (green == blue || green == nir)
	goto error;
    if (blue == nir)
	goto error;
    if (enabled < 0)
	goto error;

    if (enabled)
	return RL2_TRUE;
    else
	return RL2_FALSE;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

static int
resolve_base_resolution_section (sqlite3 * handle, const char *coverage,
				 sqlite3_int64 section_id, double *x_res,
				 double *y_res)
{
/* resolving the Base Resolution - Mixed Resolutions */
    int ret;
    char *sql;
    char *table;
    char *xtable;
    double xres;
    double yres;
    int count = 0;
    sqlite3_stmt *stmt = NULL;

    table = sqlite3_mprintf ("%s_section_levels", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT x_resolution_1_1, y_resolution_1_1 "
			 "FROM \"%s\" WHERE pyramid_level = 0 AND section_id = ?",
			 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT base_resolution SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* querying the section */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, section_id);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		xres = sqlite3_column_double (stmt, 0);
		yres = sqlite3_column_double (stmt, 1);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT base_resolution; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count == 1)
      {
	  *x_res = xres;
	  *y_res = yres;
	  return RL2_OK;
      }

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}
/**
 Sets an given Address of a  Geometry that can be used as a cutline
 - where needed transformed to the raster_coverage srid

 \param Blob containing the Geometry of the cutline
 \param with size of that blob
 \param pointer to store the calculated x_min value to extract
 \param pointer to store the calculated y_min value to extract
 \param pointer to store the calculated x_max value to extract
 \param pointer to store the calculated y_max value to extract
 \param Address (indirect pointer) where to store the created copy XY, transformed Geometry
   as POLYGON or MULTIPOLYGON) that can be used as a cutline,
  or NULL when POINT,MULTIPOINT, non-closed LINESTRINGS or a POLYGON that is only an Envelope
 \param pointer to store the size of that blob
 \param the srid of the raster_coverage

 \note the cultline logic us used ONLY if the given Address (indirect pointer) is NOT NULL
 \note GEOMETRYCOLLECTION: portions that cannot be used should be removed 
 
 \note Goals:
  - avoid use of cutline logic when NOT needed (return NULL if not needed)
  - since only position information is needed, return XY only
  - the created cutline Geometry will transformed to the srid of the raster_coverage
  - the created cutline Geometry will only hold the area of the given min/max values (crop_to_cutline=0)
  - the min/max values of the created cutline Geometry will set given min/max values (crop_to_cutline=1)
  - return POLYGON or MULTIPOLYGON or closed LINESTRINGs only

 \note this function will allocate buffer created for the used cutline, which must be freed
 */
static int
create_cutline_geometry (sqlite3 * handle,const unsigned char *blob_input, int blob_input_sz,double *minx, double *miny, double *maxx,
		     double *maxy,int crop_to_cutline,unsigned char**geom_cutline_blob,int *geom_cutline_blob_sz,int i_srid)
{
 int i_geom_valid=0;
 // Set the given values to NULL and 0 to insure that the citline-logic is not used when NOT needed
 *geom_cutline_blob=NULL;
 *geom_cutline_blob_sz=0;
 if ((handle != NULL) && (blob_input != NULL) && (blob_input_sz > 0))
 {
  char *sql;
  unsigned char *blob_cutline=NULL;
  int blob_cutline_sz=0;
  sqlite3_stmt * stmt_cutline;
  char *s_geometry_type=NULL;
  int i_srid_geom=-1;
  int i_num_points=0;
  int i_num_rings=0;
  int i_closed=0;
  int i_rc=0;
  sql = sqlite3_mprintf("SELECT ST_Srid(?), ST_GeometryType(CastToXY(?)), CastToXY(?), ST_NPoints(?),ST_NRings(?),ST_IsClosed(?)");
  if (sqlite3_prepare_v2 (handle, sql, strlen(sql), &stmt_cutline, NULL) == SQLITE_OK)
  {
   sqlite3_reset (stmt_cutline);
   sqlite3_clear_bindings (stmt_cutline);
   sqlite3_bind_blob (stmt_cutline, 1, blob_input, blob_input_sz, SQLITE_STATIC);
   sqlite3_bind_blob (stmt_cutline, 2, blob_input, blob_input_sz, SQLITE_STATIC);
   sqlite3_bind_blob (stmt_cutline, 3, blob_input, blob_input_sz, SQLITE_STATIC);
   sqlite3_bind_blob (stmt_cutline, 4, blob_input, blob_input_sz, SQLITE_STATIC);
   sqlite3_bind_blob (stmt_cutline, 5, blob_input, blob_input_sz, SQLITE_STATIC);
   sqlite3_bind_blob (stmt_cutline, 6, blob_input, blob_input_sz, SQLITE_STATIC);
   while (1)
   {
    i_rc=sqlite3_step (stmt_cutline);
    if (i_rc == SQLITE_DONE)
	    break;
	   if (i_rc == SQLITE_ROW)
    {       
     if (sqlite3_column_type (stmt_cutline, 0) == SQLITE_INTEGER)
	  	 {
	     i_srid_geom = sqlite3_column_int(stmt_cutline, 0);
     }
     if (sqlite3_column_type (stmt_cutline, 1) == SQLITE_TEXT)
	  	 {      
      const char *str=(const char *)sqlite3_column_text(stmt_cutline,1);
      int len = strlen (str);
      s_geometry_type=malloc(len+1);
		    strcpy (s_geometry_type, str);
     }
     if (sqlite3_column_type (stmt_cutline, 2) == SQLITE_BLOB)
	  	 {
	      blob_cutline = (unsigned char *)sqlite3_column_blob (stmt_cutline,2);
	      blob_cutline_sz = sqlite3_column_bytes (stmt_cutline,2);
     }
     if (sqlite3_column_type (stmt_cutline, 3) == SQLITE_INTEGER)
	  	 {
	     i_num_points = sqlite3_column_int(stmt_cutline, 3);
     }   
     if (sqlite3_column_type (stmt_cutline, 4) == SQLITE_INTEGER)
	  	 {
	     i_num_rings = sqlite3_column_int(stmt_cutline, 4);
     } 
     if (sqlite3_column_type (stmt_cutline, 5) == SQLITE_INTEGER)
	  	 {
	      i_closed = sqlite3_column_int(stmt_cutline, 5);
     }          
    }
	  }
   sqlite3_finalize (stmt_cutline);
  }
  sqlite3_free (sql);
  if (blob_cutline == NULL) 
   i_geom_valid=-1;
  if (blob_cutline_sz < 1)
   i_geom_valid=-2;
  // Note: 'blob_input' will no longer be used
  if ((strcasecmp (s_geometry_type, "MULTIPOLYGON") == 0) || (strcasecmp (s_geometry_type, "GEOMETRYCOLLECTION") == 0))
  { // TODO: checking on what is inside a GEOMETRYCOLLECTION is needed
   i_geom_valid=1;
  }
  if (strcasecmp (s_geometry_type, "POLYGON") == 0)
  { 
   if (((i_num_points > 5) && (i_num_rings == 1)) || (i_num_rings > 1))
   { // Not an Envelope
    i_geom_valid=1;     
   }
   else
    i_geom_valid=-3;
  }
  if ((strcasecmp (s_geometry_type, "LINESTRING") == 0) || (strcasecmp (s_geometry_type, "MULTILINESTRING") == 0))
  {
   if (i_closed == 1)
   { // Only closed LINESTRINGs are usefull
    i_geom_valid=1;
   }
   else
     i_geom_valid=-4;
  }
  if ((strcasecmp (s_geometry_type, "POINT") == 0) || (strcasecmp (s_geometry_type, "MULTIPOINT") == 0))
  {
   i_geom_valid=-5;
  }
  if (s_geometry_type)
   free(s_geometry_type);
  if (i_geom_valid  < 1)
  { // Not considered valid or usefull as a cutline, return [POINT, MULTIPOINT, non-closed LINESTRINGs and Envelopes]
    // printf ("-I-> create_cutline_geometry input_cutline[%p] sz[%d] geometry_type[%s] i_closed[%d]  i_num_points[%d] i_num_rings[%d]\n",blob_cutline, blob_cutline_sz,s_geometry_type,i_closed,i_num_points,i_num_rings);
   return i_geom_valid;
  }
  if ((i_srid > 0) && (i_srid_geom > 0) && (i_srid != i_srid_geom))
  { // Transform original cutline-geometry to the srid being used for the raster
   sql=sqlite3_mprintf("SELECT ST_Transform(?,%d)",i_srid);
   if (sqlite3_prepare_v2 (handle, sql, strlen(sql), &stmt_cutline, NULL) == SQLITE_OK)
   {
    sqlite3_reset (stmt_cutline);
    sqlite3_clear_bindings (stmt_cutline);
    sqlite3_bind_blob (stmt_cutline, 1, blob_cutline, blob_cutline_sz, SQLITE_STATIC);
    while (1)
    {
     i_rc=sqlite3_step (stmt_cutline);
     if (i_rc == SQLITE_DONE)
	     break;
	    if (i_rc == SQLITE_ROW)
     {
      if (sqlite3_column_type (stmt_cutline, 0) == SQLITE_BLOB)
	     { // need to free the blob_cutline beforehand?
	      blob_cutline = (unsigned char *)sqlite3_column_blob (stmt_cutline,0);
	      blob_cutline_sz = sqlite3_column_bytes (stmt_cutline,0);
      } 
     }
    }
    sqlite3_finalize (stmt_cutline);
   }
   sqlite3_free (sql);
  }
  if (crop_to_cutline == 0)
  { // Extract a portion of the geometry that reflects the area being exported
   sql = sqlite3_mprintf("SELECT ST_Intersection(BuildMbr(?,?,?,?,%d),?)",i_srid);
   if (sqlite3_prepare_v2 (handle, sql, strlen(sql), &stmt_cutline, NULL) == SQLITE_OK)
   {
    sqlite3_reset (stmt_cutline);
    sqlite3_clear_bindings (stmt_cutline);
    sqlite3_bind_double (stmt_cutline, 1, *minx);
    sqlite3_bind_double (stmt_cutline, 2, *miny);
    sqlite3_bind_double (stmt_cutline, 3, *maxx);
    sqlite3_bind_double (stmt_cutline, 4, *maxy);
    sqlite3_bind_blob (stmt_cutline, 5, blob_cutline, blob_cutline_sz, SQLITE_STATIC);
    while (1)
    {
     i_rc=sqlite3_step (stmt_cutline);
     if (i_rc == SQLITE_DONE)
	     break;
	    if (i_rc == SQLITE_ROW)
     {
      if (sqlite3_column_type (stmt_cutline, 0) == SQLITE_BLOB)
	     { // need to free blob_cutline beforehand?
	      blob_cutline = (unsigned char *)sqlite3_column_blob (stmt_cutline,0);
	      blob_cutline_sz = sqlite3_column_bytes (stmt_cutline,0);
      } 
     }
    }
    sqlite3_finalize (stmt_cutline);
   }
   sqlite3_free (sql);
  }
  else
  { // Extract the maximun extent of the geometry and set the given min/max values of the Raster to be extracted
   sql = sqlite3_mprintf("SELECT ST_MinX(?), ST_MinY(?), ST_MaxX(?), ST_MaxY(?)");
   if (sqlite3_prepare_v2 (handle, sql, strlen(sql), &stmt_cutline, NULL) == SQLITE_OK)
   {
    sqlite3_reset (stmt_cutline);
    sqlite3_clear_bindings (stmt_cutline);
    sqlite3_bind_blob (stmt_cutline, 1, blob_cutline, blob_cutline_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt_cutline, 2, blob_cutline, blob_cutline_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt_cutline, 3, blob_cutline, blob_cutline_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt_cutline, 4, blob_cutline, blob_cutline_sz, SQLITE_STATIC);
    while (1)
    {
     i_rc=sqlite3_step (stmt_cutline);
     if (i_rc == SQLITE_DONE)
	     break;
	    if (i_rc == SQLITE_ROW)
     {
      if (sqlite3_column_type (stmt_cutline, 0) == SQLITE_FLOAT)
	     { 
	      *minx = sqlite3_column_double (stmt_cutline,0);
      } 
      if (sqlite3_column_type (stmt_cutline, 1) == SQLITE_FLOAT)
	     { 
	      *miny = sqlite3_column_double (stmt_cutline,1);
      } 
      if (sqlite3_column_type (stmt_cutline, 2) == SQLITE_FLOAT)
	     { 
	      *maxx = sqlite3_column_double (stmt_cutline,2);
      } 
      if (sqlite3_column_type (stmt_cutline, 3) == SQLITE_FLOAT)
	     { 
	      *maxy = sqlite3_column_double (stmt_cutline,3);
      }
     }
    }
    sqlite3_finalize (stmt_cutline);
   }
   sqlite3_free (sql);
  }
  if (i_geom_valid == 1)
  {
   if (blob_cutline_sz > 0)
   {
    *geom_cutline_blob_sz=blob_cutline_sz;
    *geom_cutline_blob=malloc(blob_cutline_sz);
    memcpy(*geom_cutline_blob, blob_cutline, blob_cutline_sz);
   }
  }
 }
 return i_geom_valid;
}

/**
 Attempt to resolve missing parameters and create a cutline that can be used

 \param Blob containing the Geometry of the area to extract
 \param with size of that Blob
 \param pointer to store the calculated x_min value to extract
 \param pointer to store the calculated y_min value to extract
 \param pointer to store the calculated x_max value to extract
 \param pointer to store the calculated y_max value to extract
 \param pointer to store the calculated width value of the image to extract
 \param pointer to store the calculated height value of the image to extract
 \param pointer that stores the x_center value, when using  the tie-point logic
 \param pointer that stores the y_center value, when using  the tie-point logic
 \param crop_to_cutline: 1=use the size of the cutline to calculate the area to extract
 \param pointer to store the Blob containing the Geometry of the cutline
 \param pointer to store the size of that blob
 \param the srid of the raster_coverage
 
 \return return code for used with rl2tool for messages

 \return  'geom_cutline_blob' (as POLYGON or MULTIPOLYGON) that can be used as a cutline,
  or NULL when POINT,MULTIPOINT, non-closed LINESTRINGS or a POLYGON that is only an Envelope

 \note  - setting the needed values, were possible [from rl2sql or rl2tool, which will report the errors]
 \note  - if resolution is not set, the raster_coverage resolution will be used
 \note  - final image width/height set is based on the calculated min/max area and resolution 
 \note  - 'mismatching_size' errors are corrrected

 \note 'create_cutline_geometry' will allocate buffer created for the used cutline, which must be freed
 */
RL2_DECLARE int
rl2_resolve_image_sanity(sqlite3 * handle, const char *coverage,
					       const unsigned char *blob, int blob_sz,
				        double *x_min, double *y_min, double *x_max,
				        double *y_max, double *x_res, double *y_res,
            unsigned int *width, unsigned int *height,
					       double *x_center, double *y_center,
            const unsigned char *blob_cutline, int blob_cutline_sz,
            int crop_to_cutline,
            unsigned char **geom_cutline_blob,int *geom_cutline_blob_sz,
            int i_srid)
{ 
 int b_WidthHeight=0;
 int b_Resolution=0;
 int b_MinMax=0;
 int i_rc_cutline=0;
 if ((crop_to_cutline < 0) || (crop_to_cutline > 1))
  crop_to_cutline=0;
 int i_rc=RL2_ERROR;
 if ((x_min == NULL) || (y_min == NULL) || (x_max == NULL) || (y_max == NULL) ||
      (x_res == NULL) || (y_res == NULL) || (width == NULL) || (height == NULL) ||
      (coverage == NULL))
   return RL2_ERROR; // these results MUST be returned, coverage is needed
 if ((*x_res <= 0.0) || (*y_res <= 0.0))
 {  // resolution cannot be minus or 0.0 [0 can be used to set the default resolution is sql]
  if (*x_res <= 0.0)
   *x_res = DBL_MAX;
  if (*y_res <= 0.0)
   *y_res = DBL_MAX;
 }
 if ((*width > 0) && (*height > 0))
  b_WidthHeight=1;
 if (((handle != NULL) && (blob != NULL)) && (blob_sz > 0))
 { // Retrieve from Blob (rl2sql)
  if (rl2_parse_bbox(handle, blob, blob_sz, x_min, y_min, x_max, y_max) == RL2_OK)
  { /* checking the Geometry */
	  b_MinMax=1;
  }
 }
 else
 { // Retrieve from given values (rl2tool)
  if ((*x_min != DBL_MAX) && (*y_min != DBL_MAX) && (*x_max != DBL_MAX)  && (*y_max != DBL_MAX))
  {
   b_MinMax=1;
  }
 }
 /* checking the resolution */
 if ((*x_res != DBL_MAX) && (*y_res != DBL_MAX))
 {
  b_Resolution=1;
 }
 else
 {
  if ((*x_res != DBL_MAX) && (*y_res == DBL_MAX))
  {
   *y_res=*x_res;
   b_Resolution=1;
   }
  else
  {
   if ((*x_res == DBL_MAX) && (*y_res != DBL_MAX))
   {
    *x_res=*y_res;
    b_Resolution=1;
   }
   else
   { // rerieve default resolution of the coverage [--base-resolution]
    if (((b_Resolution == 0) && (handle != NULL)) && ((b_MinMax == 0) || (b_WidthHeight == 0)))
    { // resolve resolution only when NOT 'base'-resolution  (one of MinMax/WidthHeight is NOT valid)
     rl2_resolve_base_resolution_from_dbms (handle,coverage, 0, 0, x_res, y_res);
     if ((*x_res > 0) && (*y_res > 0))
      b_Resolution=1;
    }
   }
  }
 }
 if ((b_MinMax==1) && (b_WidthHeight == 0) &&  (b_Resolution == 1)) 
 { // resolve missing width/height [cutline: no problem]
  *width = (int) ((*x_max-*x_min) / *x_res);
	 *height = (int) ((*y_max-*y_min) / *y_res);
  b_WidthHeight=1;
 }
 /* resolving the resolution*/
 if ((b_MinMax==1) && (b_WidthHeight == 1) &&  (b_Resolution == 0)) 
 { // resolve missing horz/vert resolution  [cutline: no problem]
  *x_res = (double) ((*x_max-*x_min) * *width);
  *y_res = (double) ((*y_max-*y_min) * *height);
  b_Resolution=1;
 }
 if ((b_MinMax == 0) && (b_WidthHeight == 1) &&  (b_Resolution == 1))
 {
  double ext_x = 0.0;
  double ext_y = 0.0;
  if ((handle != NULL) && (blob != NULL))
  { // Retrieve from Blob (rl2sql)
   if (rl2_parse_point (handle, blob, blob_sz, x_center, y_center) == RL2_OK)
   {
    /* assumed to be the GeoTiff Center Point */
    ext_x = (double) *width * *x_res;
    ext_y = (double) *height * *y_res;
    *x_min = *x_center - ext_x / 2.0;
    *x_max = *x_min + ext_x;
    *y_min = *y_center - ext_y / 2.0;
    *y_max = *y_min + ext_y;
    b_MinMax=1;
   }
  }
  else
  { // Retrieve from given values (rl2tool)
   if ((x_center != NULL) && (y_center != NULL))
   { // Optional-Parameter, check for NULL
    if ((*x_min == DBL_MAX) && (*y_min == DBL_MAX) && (*x_max == DBL_MAX)  && (*y_max == DBL_MAX))
	   {     /* tie-point: Center Point */
     if ((*x_center != DBL_MAX) && (*y_center != DBL_MAX))
	    {
     	ext_x = (double) (*width) * *x_res;
		    ext_y = (double) (*height) * *y_res;
		    *x_min = *x_center - (ext_x / 2.0);
		    *x_max = *x_min + ext_x;
		    *y_min = *y_center - (ext_y / 2.0);
		    *y_max = *y_min + ext_y;
      b_MinMax=1;
     }
     else
     {      
      if ((*x_center == DBL_MAX) && (*y_center == DBL_MAX))
      {
       i_rc=100;
      }
      else	  
      {
       if (*x_center != DBL_MAX) 
	       i_rc=101;
       else
        i_rc=102;
      }
     }
    }
    else
    {
     if ((*x_center == DBL_MAX) && (*y_center == DBL_MAX) && (*x_max == DBL_MAX)  && (*y_max == DBL_MAX))
     { 	/* tie-point: LowerLeft Corner */
      if ((*x_min != DBL_MAX) && (*y_min != DBL_MAX) )
	     {
       ext_x = (double) (*width) * *x_res;
	     	ext_y = (double) (*height) * *y_res;
		     *x_max = *x_min + ext_x;
		     *y_max = *y_min + ext_y;
		     *x_center = *x_min + (ext_x / 2.0);
		     *y_center = *y_min + (ext_y / 2.0);
       b_MinMax=1;
      }
      else
      {      
       if ((*x_min  == DBL_MAX) && (*y_min == DBL_MAX))
       {
        i_rc=105;
       }
       else	  
       {
        if (*x_min != DBL_MAX) 
	        i_rc=106;
        else
         i_rc=107;
       }
      }
     }
     else
     {
      if ((*x_center == DBL_MAX) && (*y_center == DBL_MAX) && (*x_min == DBL_MAX) && (*y_max == DBL_MAX))
      { /* tie-point: LowerRight Corner */
       if ((*x_max != DBL_MAX) && (*y_min != DBL_MAX) )
	      {
        ext_x = (double) (*width) * *x_res;
	      	ext_y = (double) (*height) * *y_res;
		      *x_min = *x_max - ext_x;
		      *y_max = *y_min + ext_y;
		      *x_center = *x_min - (ext_x / 2.0);
		      *y_center = *y_min + (ext_y / 2.0);
        b_MinMax=1;
       }
       else
       {      
        if ((*x_max  == DBL_MAX) && (*y_min == DBL_MAX))
        {
         i_rc=110;
        }
        else	  
        {
         if (*x_max != DBL_MAX) 
	         i_rc=111;
         else
          i_rc=112;
        }
       }
      }
      else
      {
       if ((*x_center == DBL_MAX)&& (*y_center == DBL_MAX) && (*x_max == DBL_MAX) && (*y_min == DBL_MAX))
	      { /* tie-point: UpperLeft Corner */
        if ((*x_min != DBL_MAX) && (*y_max != DBL_MAX) )
	       {
         ext_x = (double) (*width) * *x_res;
		       ext_y = (double) (*height) * *y_res;
		       *x_max = *x_min + ext_x;
		       *y_min = *y_max - ext_y;
		       *x_center = *x_min + (ext_x / 2.0);
         *y_center = *y_max - (ext_y / 2.0);
         b_MinMax=1;
        }
        else
        {      
         if ((*x_min  == DBL_MAX) && (*y_max == DBL_MAX))
         {
          i_rc=115;
         }
         else	  
         {
          if (*x_min != DBL_MAX) 
	          i_rc=116;
          else
           i_rc=117;
         }
        }
       }
       else
       {
        if ((*x_center == DBL_MAX) && (*y_center == DBL_MAX) && (*x_min == DBL_MAX)  && (*y_min == DBL_MAX))
	       { 	/* tie-point: UpperRight Corner */
		       if ((*x_max != DBL_MAX) && (*y_max != DBL_MAX))
         {
          ext_x = (double) (*width) * *x_res;
		        ext_y = (double) (*height) * *y_res;
		        *x_min = *x_max - ext_x;
		        *y_min = *y_max - ext_y;
		        *x_center = *x_max - (ext_x / 2.0);
		        *y_center = *y_max - (ext_y / 2.0);
          b_MinMax=1;
         }
         else
         {      
          if ((*x_min  == DBL_MAX) && (*y_max == DBL_MAX))
          {
           i_rc=120;
          }
          else	  
          {
           if (*x_min != DBL_MAX) 
	           i_rc=121;
           else
            i_rc=122;
          }
         }
        }
        else
        {
         if ((b_MinMax == 1) && (b_WidthHeight == 1) &&  (b_Resolution == 1)) 
         {
          ext_x = (double) (*width) * *x_res;
		        ext_y = (double) (*height) * *y_res;
	     	   *x_center = *x_max - (ext_x / 2.0);
	     	   *y_center = *y_min + (ext_y / 2.0);
         }
         else
         {  /* invalid tie-point */
          i_rc=RL2_ERROR;
         }
        }
       }  
      }     
     }
    }
   }
  }
 }
 if ((b_MinMax == 1) && (b_WidthHeight == 1) &&  (b_Resolution == 1)) 
  i_rc=RL2_OK;
 if ((b_MinMax == 1) && (b_WidthHeight == 1) &&  (b_Resolution == 0) && (handle != NULL)) 
 {
  if ((*x_res == DBL_MAX) && (*y_res == DBL_MAX))
  {
   i_rc=125;
  }
  else
  {
   if (*x_res == DBL_MAX)
    i_rc=126;
   else
    i_rc=127;
  }  
 }
 if ((b_Resolution == 1) && ((*x_res <= 0.0) || (*y_res <= 0.0))) 
 {
  if ((*x_res <= 0.0) && (*y_res <= 0.0))
  {
   i_rc=130;
  }
  else
  {
   if (*x_res <= 0.0)
    i_rc=131;
   else
    i_rc=132;
  }
 }
 if ((b_MinMax == 1) && (b_WidthHeight >= 0) &&  (b_Resolution == 0) && (handle == NULL)) 
 { // being called from rl2.tool.check_export_args [without Database handle]: allow this
  i_rc=RL2_OK;
 }
 else
 {
  if (i_rc == RL2_OK)
  { //- (POINT,MULTIPOINT, non-closed LINESTRINGS, POLYGON that is only an Envelope are NOT considered valid)
   i_rc_cutline=create_cutline_geometry(handle,blob_cutline, blob_cutline_sz, x_min, y_min, x_max, y_max,crop_to_cutline,geom_cutline_blob,geom_cutline_blob_sz,i_srid);
   switch (i_rc_cutline)
   {
    case 1:
    { //  1 : Valid  Cutline-Geometry:  Closed LINESTRÌNG / POLYGON (and MULTIs) or (not checked) GEOMETRYCOLLECTION)
     if (crop_to_cutline == 1)
     { // Assume the min/max values have been changed, adapt the image width/height     
      *width = (int) ((*x_max-*x_min) / *x_res);
	     *height = (int) ((*y_max-*y_min) / *y_res);
     }
    }
    break;
    case 0:
     //  0 : no  Input-Geometry
    break;
    case -1:
     //  -1 : no Cutline Geometry created from Input-Geometry
    break;
    case -2:
     //  -2 : size of Cutline  Geometry <=0
    break;
    case -3:
     //   -3 : POLYGON is an envelope
    break;
    case -4:
     //   -4 : Not a closed LINESTRING
    break;
    case -5:
    //   -5 : POINT or MULTIPOINT
    break;
    default:
     //   unknown return code
    break;
   }
   // resolve any 'mismatching_size' error
   double ext_x = (double) *width * *x_res;
   double ext_y = (double) *height * *y_res;
   double img_x = *x_max - *x_min;
   double img_y = *y_max - *y_min;
   double confidence;
   confidence = ext_x / 100.0;
   if ((img_x < (ext_x - confidence)) || (img_x > (ext_x + confidence)))
   {
    *width = (int) ((*x_max-*x_min) / *x_res);
   }
   confidence = ext_y / 100.0;
   if ((img_y < (ext_y - confidence)) || (img_y > (ext_y + confidence)))
   {
    *height = (int) ((*y_max-*y_min) / *y_res);
   }
  }
 }
 printf ("-I-> rl2_resolve_image_sanity[%d] [%d,%d,%d] image[%d,%d] resolution[%2.4f,%2.4f] min_max[%2.4f,%2.4f,%2.4f,%2.4f] cutline[%p] sz[%d] input_cutline[%p] sz[%d] crop_to_cutline[%d] i_rc_cutline[%d]\n",  i_rc, b_WidthHeight,b_Resolution,b_MinMax,*width,*height,*x_res,*y_res, *x_min, *y_min,*x_max, *y_max,*geom_cutline_blob,*geom_cutline_blob_sz,blob_cutline, blob_cutline_sz,crop_to_cutline,i_rc_cutline);
 return i_rc;
}

RL2_DECLARE int
rl2_resolve_base_resolution_from_dbms (sqlite3 * handle, const char *coverage,
				       int by_section,
				       sqlite3_int64 section_id,
				       double *x_res, double *y_res)
{
/* resolving the Base Resolution */
    int ret;
    char *sql;
    double xres;
    double yres;
    int count = 0;
    sqlite3_stmt *stmt = NULL;

    if (rl2_is_mixed_resolutions_coverage (handle, coverage) > 0 && by_section)
	return resolve_base_resolution_section (handle, coverage, section_id,
						x_res, y_res);

    sql =
	sqlite3_mprintf ("SELECT horz_resolution, vert_resolution "
			 "FROM raster_coverages WHERE coverage_name = Lower(%Q)",
			 coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT base_resolution SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		xres = sqlite3_column_double (stmt, 0);
		yres = sqlite3_column_double (stmt, 1);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT base_resolution; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count == 1)
      {
	  *x_res = xres;
	  *y_res = yres;
	  return RL2_OK;
      }

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_resolve_full_section_from_dbms (sqlite3 * handle, const char *coverage,
				    sqlite3_int64 section_id, double x_res,
				    double y_res, double *minX, double *minY,
				    double *maxX, double *maxY,
				    unsigned int *Width, unsigned int *Height)
{
/* resolving a Full Section Extent and related Width and Height */
    rl2CoveragePtr cvg;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char level;
    unsigned char scale;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    int count = 0;
    sqlite3_stmt *stmt = NULL;
    double minx = 0.0;
    double miny = 0.0;
    double maxx = 0.0;
    double maxy = 0.0;
    int width = 0;
    int height = 0;

    cvg = rl2_create_coverage_from_dbms (handle, coverage);
    if (cvg == NULL)
	return RL2_ERROR;
    if (rl2_find_matching_resolution
	(handle, cvg, 1, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
      {
	  rl2_destroy_coverage (cvg);
	  return RL2_ERROR;
      }
    rl2_destroy_coverage (cvg);

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT MbrMinX(geometry), MbrMinY(geometry), "
			 "MbrMaxX(geometry), MbrMaxY(geometry), width, height "
			 "FROM \"%s\" WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT section_full_extent SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

/* querying the section */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, section_id);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		minx = sqlite3_column_double (stmt, 0);
		miny = sqlite3_column_double (stmt, 1);
		maxx = sqlite3_column_double (stmt, 2);
		maxy = sqlite3_column_double (stmt, 3);
		width = sqlite3_column_int (stmt, 4);
		height = sqlite3_column_int (stmt, 5);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_full_extent; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (level == 0 && scale == RL2_SCALE_1)
	;
    else
      {
	  /* rescaling Width and Height */
	  double ext_x = maxx - minx;
	  double ext_y = maxy - miny;
	  width = (unsigned int) (ext_x / xx_res);
	  if (((double) width * xx_res) < ext_x)
	      width++;
	  height = (unsigned int) (ext_y / yy_res);
	  if (((double) height * yy_res) < ext_y)
	      height++;
      }
    if (count == 1)
      {
	  *minX = minx;
	  *minY = miny;
	  *maxX = maxx;
	  *maxY = maxy;
	  *Width = width;
	  *Height = height;
	  return RL2_OK;
      }

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_dbms_section_id (sqlite3 * handle, const char *coverage,
			 const char *section, sqlite3_int64 * section_id,
			 int *duplicate)
{
/* retrieving a Section ID by its name */
    int ret;
    char *sql;
    char *table;
    char *xtable;
    int count = 0;
    sqlite3_stmt *stmt = NULL;

    *duplicate = 0;
    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT section_id FROM \"%s\" WHERE section_name = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT section_id SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

/* querying the section */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, section, strlen (section), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		*section_id = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT section_id; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count == 1)
	return RL2_OK;
    if (count > 1)
	*duplicate = 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_delete_dbms_section (sqlite3 * handle, const char *coverage,
			 sqlite3_int64 section_id)
{
/* deleting a Raster Section */
    int ret;
    char *sql;
    rl2CoveragePtr cvg = NULL;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt = NULL;

    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\" WHERE section_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("DELETE sections SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }

/* DELETing the section */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, section_id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "DELETE sections; sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_finalize (stmt);

    rl2_destroy_coverage (cvg);
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (cvg != NULL)
	rl2_destroy_coverage (cvg);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_drop_dbms_coverage (sqlite3 * handle, const char *coverage)
{
/* dropping a Raster Coverage */
    int ret;
    char *sql;
    char *sql_err = NULL;
    char *table;
    char *xtable;

/* disabling the SECTIONS spatial index */
    xtable = sqlite3_mprintf ("%s_sections", coverage);
    sql = sqlite3_mprintf ("SELECT DisableSpatialIndex("
			   "%Q, 'geometry')", xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DisableSpatialIndex \"%s\" error: %s\n", xtable,
		   sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xtable);
	  goto error;
      }
    sqlite3_free (xtable);

/* dropping the SECTIONS spatial index */
    table = sqlite3_mprintf ("idx_%s_sections_geometry", coverage);
    xtable = rl2_double_quoted_sql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* disabling the TILES spatial index */
    xtable = sqlite3_mprintf ("%s_tiles", coverage);
    sql = sqlite3_mprintf ("SELECT DisableSpatialIndex("
			   "%Q, 'geometry')", xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DisableSpatialIndex \"%s\" error: %s\n", xtable,
		   sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (xtable);
	  goto error;
      }
    sqlite3_free (xtable);

/* dropping the TILES spatial index */
    table = sqlite3_mprintf ("idx_%s_tiles_geometry", coverage);
    xtable = rl2_double_quoted_sql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* dropping the TILE_DATA table */
    table = sqlite3_mprintf ("%s_tile_data", coverage);
    xtable = rl2_double_quoted_sql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* deleting the TILES Geometry definition */
    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = rl2_double_quoted_sql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM geometry_columns "
			   "WHERE Lower(f_table_name) = Lower(%Q)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE TilesGeometry \"%s\" error: %s\n",
		   coverage, sql_err);
	  sqlite3_free (sql_err);
	  goto error;
      }

/* deleting the SECTIONS Geometry definition */
    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sql = sqlite3_mprintf ("DELETE FROM geometry_columns "
			   "WHERE Lower(f_table_name) = Lower(%Q)", xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE SectionsGeometry \"%s\" error: %s\n",
		   coverage, sql_err);
	  sqlite3_free (sql_err);
	  goto error;
      }

/* dropping the TILES table */
    table = sqlite3_mprintf ("%s_tiles", coverage);
    xtable = rl2_double_quoted_sql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* dropping the SECTIONS table */
    table = sqlite3_mprintf ("%s_sections", coverage);
    xtable = rl2_double_quoted_sql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* dropping the LEVELS table */
    table = sqlite3_mprintf ("%s_levels", coverage);
    xtable = rl2_double_quoted_sql (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DROP TABLE \"%s\" error: %s\n", table, sql_err);
	  sqlite3_free (sql_err);
	  sqlite3_free (table);
	  goto error;
      }
    sqlite3_free (table);

/* deleting the Raster Coverage definition */
    sql = sqlite3_mprintf ("DELETE FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &sql_err);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE raster_coverage \"%s\" error: %s\n",
		   coverage, sql_err);
	  sqlite3_free (sql_err);
	  goto error;
      }
    return RL2_OK;

  error:
    return RL2_ERROR;
}

static void
prime_void_tile_int8 (void *pixels, unsigned int width, unsigned int height,
		      rl2PixelPtr no_data)
{
/* priming a void tile buffer - INT8 */
    unsigned int row;
    unsigned int col;
    char *p = pixels;
    char val = 0;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (sample_type != RL2_SAMPLE_INT8 || num_bands != 1)
	      goto done;
	  rl2_get_pixel_sample_int8 (no_data, &val);
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = val;
      }
}

static void
prime_void_tile_uint8 (void *pixels, unsigned int width,
		       unsigned int height, unsigned char num_bands,
		       rl2PixelPtr no_data)
{
/* priming a void tile buffer - UINT8 */
    unsigned int row;
    unsigned int col;
    int band;
    unsigned char *p = pixels;
    unsigned char val = 0;
    int ok_no_data = 0;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (sample_type != RL2_SAMPLE_UINT8)
	      goto done;
	  ok_no_data = 1;
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		for (band = 0; band < num_bands; band++)
		  {
		      if (ok_no_data)
			  rl2_get_pixel_sample_uint8 (no_data, band, &val);
		      *p++ = val;
		  }
	    }
      }
}

static void
prime_void_tile_int16 (void *pixels, unsigned int width,
		       unsigned int height, rl2PixelPtr no_data)
{
/* priming a void tile buffer - INT16 */
    unsigned int row;
    unsigned int col;
    short *p = pixels;
    short val = 0;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (sample_type != RL2_SAMPLE_INT16 || num_bands != 1)
	      goto done;
	  rl2_get_pixel_sample_int16 (no_data, &val);
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = val;
      }
}

static void
prime_void_tile_uint16 (void *pixels, unsigned int width,
			unsigned int height, unsigned char num_bands,
			rl2PixelPtr no_data)
{
/* priming a void tile buffer - UINT16 */
    unsigned int row;
    unsigned int col;
    int band;
    unsigned short *p = pixels;
    unsigned short val = 0;
    int ok_no_data = 0;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (sample_type != RL2_SAMPLE_UINT16)
	      goto done;
	  ok_no_data = 1;
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		for (band = 0; band < num_bands; band++)
		  {
		      if (ok_no_data)
			  rl2_get_pixel_sample_uint16 (no_data, band, &val);
		      *p++ = val;
		  }
	    }
      }
}

static void
prime_void_tile_int32 (void *pixels, unsigned int width,
		       unsigned int height, rl2PixelPtr no_data)
{
/* priming a void tile buffer - INT32 */
    unsigned int row;
    unsigned int col;
    int *p = pixels;
    int val = 0;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (sample_type != RL2_SAMPLE_INT32 || num_bands != 1)
	      goto done;
	  rl2_get_pixel_sample_int32 (no_data, &val);
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = val;
      }
}

static void
prime_void_tile_uint32 (void *pixels, unsigned int width,
			unsigned int height, rl2PixelPtr no_data)
{
/* priming a void tile buffer - UINT32 */
    unsigned int row;
    unsigned int col;
    unsigned int *p = pixels;
    unsigned int val = 0;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (sample_type != RL2_SAMPLE_UINT32 || num_bands != 1)
	      goto done;
	  rl2_get_pixel_sample_uint32 (no_data, &val);
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = val;
      }
}

static void
prime_void_tile_float (void *pixels, unsigned int width,
		       unsigned int height, rl2PixelPtr no_data)
{
/* priming a void tile buffer - Float */
    unsigned int row;
    unsigned int col;
    float *p = pixels;
    float val = 0.0;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (sample_type != RL2_SAMPLE_FLOAT || num_bands != 1)
	      goto done;
	  rl2_get_pixel_sample_float (no_data, &val);
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = val;
      }
}

static void
prime_void_tile_double (void *pixels, unsigned int width,
			unsigned int height, rl2PixelPtr no_data)
{
/* priming a void tile buffer - Double */
    unsigned int row;
    unsigned int col;
    double *p = pixels;
    double val = 0.0;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (sample_type != RL2_SAMPLE_DOUBLE || num_bands != 1)
	      goto done;
	  rl2_get_pixel_sample_double (no_data, &val);
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = val;
      }
}

RL2_DECLARE void
rl2_prime_void_tile (void *pixels, unsigned int width, unsigned int height,
		     unsigned char sample_type, unsigned char num_bands,
		     rl2PixelPtr no_data)
{
/* priming a void tile buffer */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  prime_void_tile_int8 (pixels, width, height, no_data);
	  break;
      case RL2_SAMPLE_1_BIT:
      case RL2_SAMPLE_2_BIT:
      case RL2_SAMPLE_4_BIT:
      case RL2_SAMPLE_UINT8:
	  prime_void_tile_uint8 (pixels, width, height, num_bands, no_data);
	  break;
      case RL2_SAMPLE_INT16:
	  prime_void_tile_int16 (pixels, width, height, no_data);
	  break;
      case RL2_SAMPLE_UINT16:
	  prime_void_tile_uint16 (pixels, width, height, num_bands, no_data);
	  break;
      case RL2_SAMPLE_INT32:
	  prime_void_tile_int32 (pixels, width, height, no_data);
	  break;
      case RL2_SAMPLE_UINT32:
	  prime_void_tile_uint32 (pixels, width, height, no_data);
	  break;
      case RL2_SAMPLE_FLOAT:
	  prime_void_tile_float (pixels, width, height, no_data);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  prime_void_tile_double (pixels, width, height, no_data);
	  break;
      };
}

RL2_DECLARE void
rl2_prime_void_tile_palette (void *pixels, unsigned int width,
			     unsigned int height, rl2PixelPtr no_data)
{
/* priming a void tile buffer (PALETTE) */
    unsigned int row;
    unsigned int col;
    unsigned char index = 0;
    unsigned char *p = pixels;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (pixel_type != RL2_PIXEL_PALETTE || num_bands != 1)
	      goto done;
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
		rl2_get_pixel_sample_1bit (no_data, &index);
		break;
	    case RL2_SAMPLE_2_BIT:
		rl2_get_pixel_sample_2bit (no_data, &index);
		break;
	    case RL2_SAMPLE_4_BIT:
		rl2_get_pixel_sample_4bit (no_data, &index);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_get_pixel_sample_uint8 (no_data, 0, &index);
		break;
	    };
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = index;
      }
}

RL2_DECLARE rl2CoveragePtr
rl2_create_coverage_from_dbms (sqlite3 * handle, const char *coverage)
{
/* attempting to create a Coverage Object from the DBMS definition */
    char *sql;
    int ret;
    int len;
    sqlite3_stmt *stmt;
    int sample;
    int pixel;
    int num_bands;
    int compression;
    int quality;
    int tile_width;
    int tile_height;
    double x_res;
    double y_res;
    int srid;
    int strict_resolution = 0;
    int mixed_resolutions = 0;
    int section_paths = 0;
    int section_md5 = 0;
    int section_summary = 0;
    char* title;
    char* abstract;
    char* copyright;
    int ok = 0;
    const char *value;
    rl2PixelPtr no_data = NULL;
    rl2CoveragePtr cvg;
/* querying the Coverage metadata defs */
    sql =
	"SELECT sample_type, pixel_type, num_bands, compression, quality, tile_width, "
	"tile_height, horz_resolution, vert_resolution, srid, nodata_pixel, "
	"strict_resolution, mixed_resolutions, section_paths, "
	"section_md5, section_summary, title, abstract, '' AS copyright "
	"FROM raster_coverages WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  return NULL;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_sample = 0;
		int ok_pixel = 0;
		int ok_num_bands = 0;
		int ok_compression = 0;
		int ok_quality = 0;
		int ok_tile_width = 0;
		int ok_tile_height = 0;
		int ok_x_res = 0;
		int ok_y_res = 0;
		int ok_srid = 0;
		int ok_nodata = 1;
		int ok_strict = 0;
		int ok_mixed = 0;
		int ok_paths = 0;
		int ok_md5 = 0;
		int ok_summary = 0;
  int ok_title = 0;
  int ok_abstract = 0;
  int ok_copyright = 0;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 0);
		      if (strcasecmp (value, "1-BIT") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_1_BIT;
			}
		      if (strcasecmp (value, "2-BIT") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_2_BIT;
			}
		      if (strcasecmp (value, "4-BIT") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_4_BIT;
			}
		      if (strcasecmp (value, "INT8") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_INT8;
			}
		      if (strcasecmp (value, "UINT8") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_UINT8;
			}
		      if (strcasecmp (value, "INT16") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_INT16;
			}
		      if (strcasecmp (value, "UINT16") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_UINT16;
			}
		      if (strcasecmp (value, "INT32") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_INT32;
			}
		      if (strcasecmp (value, "UINT32") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_UINT32;
			}
		      if (strcasecmp (value, "FLOAT") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_FLOAT;
			}
		      if (strcasecmp (value, "DOUBLE") == 0)
			{
			    ok_sample = 1;
			    sample = RL2_SAMPLE_DOUBLE;
			}
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 1);
		      if (strcasecmp (value, "MONOCHROME") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_MONOCHROME;
			}
		      if (strcasecmp (value, "PALETTE") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_PALETTE;
			}
		      if (strcasecmp (value, "GRAYSCALE") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_GRAYSCALE;
			}
		      if (strcasecmp (value, "RGB") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_RGB;
			}
		      if (strcasecmp (value, "MULTIBAND") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_MULTIBAND;
			}
		      if (strcasecmp (value, "DATAGRID") == 0)
			{
			    ok_pixel = 1;
			    pixel = RL2_PIXEL_DATAGRID;
			}
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
		  {
		      num_bands = sqlite3_column_int (stmt, 2);
		      ok_num_bands = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 3);
		      if (strcasecmp (value, "NONE") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_NONE;
			}
		      if (strcasecmp (value, "DEFLATE") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_DEFLATE;
			}
		      if (strcasecmp (value, "DEFLATE_NO") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_DEFLATE_NO;
			}
		      if (strcasecmp (value, "LZMA") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LZMA;
			}
		      if (strcasecmp (value, "LZMA_NO") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LZMA_NO;
			}
		      if (strcasecmp (value, "PNG") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_PNG;
			}
		      if (strcasecmp (value, "JPEG") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_JPEG;
			}
		      if (strcasecmp (value, "LOSSY_WEBP") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LOSSY_WEBP;
			}
		      if (strcasecmp (value, "LOSSLESS_WEBP") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LOSSLESS_WEBP;
			}
		      if (strcasecmp (value, "CCITTFAX4") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_CCITTFAX4;
			}
		      if (strcasecmp (value, "CHARLS") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_CHARLS;
			}
		      if (strcasecmp (value, "LOSSY_JP2") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LOSSY_JP2;
			}
		      if (strcasecmp (value, "LOSSLESS_JP2") == 0)
			{
			    ok_compression = 1;
			    compression = RL2_COMPRESSION_LOSSLESS_JP2;
			}
		  }
		if (sqlite3_column_type (stmt, 4) == SQLITE_INTEGER)
		  {
		      quality = sqlite3_column_int (stmt, 4);
		      ok_quality = 1;
		  }
		if (sqlite3_column_type (stmt, 5) == SQLITE_INTEGER)
		  {
		      tile_width = sqlite3_column_int (stmt, 5);
		      ok_tile_width = 1;
		  }
		if (sqlite3_column_type (stmt, 6) == SQLITE_INTEGER)
		  {
		      tile_height = sqlite3_column_int (stmt, 6);
		      ok_tile_height = 1;
		  }
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT)
		  {
		      x_res = sqlite3_column_double (stmt, 7);
		      ok_x_res = 1;
		  }
		if (sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      y_res = sqlite3_column_double (stmt, 8);
		      ok_y_res = 1;
		  }
		if (sqlite3_column_type (stmt, 9) == SQLITE_INTEGER)
		  {
		      srid = sqlite3_column_int (stmt, 9);
		      ok_srid = 1;
		  }
		if (sqlite3_column_type (stmt, 10) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt, 10);
		      int blob_sz = sqlite3_column_bytes (stmt, 10);
		      no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
		      if (no_data == NULL)
			  ok_nodata = 0;
		  }
		if (sqlite3_column_type (stmt, 11) == SQLITE_INTEGER)
		  {
		      strict_resolution = sqlite3_column_int (stmt, 11);
		      ok_strict = 1;
		  }
		if (sqlite3_column_type (stmt, 12) == SQLITE_INTEGER)
		  {
		      mixed_resolutions = sqlite3_column_int (stmt, 12);
		      ok_mixed = 1;
		  }
		if (sqlite3_column_type (stmt, 13) == SQLITE_INTEGER)
		  {
		      section_paths = sqlite3_column_int (stmt, 13);
		      ok_paths = 1;
		  }
		if (sqlite3_column_type (stmt, 14) == SQLITE_INTEGER)
		  {
		      section_md5 = sqlite3_column_int (stmt, 14);
		      ok_md5 = 1;
		  }
		if (sqlite3_column_type (stmt, 15) == SQLITE_INTEGER)
		  {
		      section_summary = sqlite3_column_int (stmt, 15);
		      ok_summary = 1;
		  }
    if (sqlite3_column_type (stmt, 16) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 16);
		      len = strlen (value);
		      title = malloc (len + 1);
		      strcpy (title, value);
        ok_title=1;
		  }
		  if (sqlite3_column_type (stmt, 17) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 17);
		      len = strlen (value);
		      abstract = malloc (len + 1);
		      strcpy (abstract, value);
        ok_abstract=1;
		  }
    if (sqlite3_column_type (stmt, 18) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 18);
		      len = strlen (value);
        if (len  == 0)
        { // If the user, in his/her wisdom, has NOT set a copyright, the raster_coverage name will be used.
         len = strlen (coverage);
         copyright = malloc (len + 1);
         strcpy (copyright, coverage);
        }
        else
        {
		       copyright = malloc (len + 1);
		       strcpy (copyright, value);
        }
        ok_copyright=1;
		  }
		if (ok_sample && ok_pixel && ok_num_bands && ok_compression
		    && ok_quality && ok_tile_width && ok_tile_height
		    && ok_x_res && ok_y_res && ok_srid && ok_nodata
		    && ok_strict && ok_mixed && ok_paths && ok_md5
		    && ok_summary && ok_title && ok_abstract && ok_copyright)
		    ok = 1;
	    }
      }
    sqlite3_finalize (stmt);

    if (!ok)
      {
	  fprintf (stderr, "ERROR: unable to find a Coverage named \"%s\"\n",
		   coverage);
	  return NULL;
      }

    cvg =
	rl2_create_coverage (coverage, sample, pixel, num_bands, compression,
			     quality, tile_width, tile_height, no_data,title,abstract,copyright);
    if (cvg == NULL)
      {
	  fprintf (stderr,
		   "ERROR: unable to create a Coverage Object supporting \"%s\"\n",
		   coverage);
	  return NULL;
      }
    if (rl2_coverage_georeference (cvg, srid, x_res, y_res) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to Georeference a Coverage Object supporting \"%s\"\n",
		   coverage);
	  rl2_destroy_coverage (cvg);
	  return NULL;
      }
    if (rl2_set_coverage_policies
	(cvg, strict_resolution, mixed_resolutions, section_paths,
	 section_md5, section_summary) != RL2_OK)
      {
	  fprintf (stderr,
		   "ERROR: unable to set the Policies on the Coverage Object supporting \"%s\"\n",
		   coverage);
	  rl2_destroy_coverage (cvg);
	  return NULL;
      }
    return cvg;
}

RL2_DECLARE rl2VectorLayerPtr
rl2_create_vector_layer_from_dbms (sqlite3 * handle, const char *coverage)
{
/* attempting to create a Vector Layer Object from the DBMS definition */
    char *sql;
    int ret;
    sqlite3_stmt *stmt;
    char *f_table_name = NULL;
    char *f_geometry_column = NULL;
    int geometry_type;
    int srid;
    int spatial_index;
    int ok = 0;
    rl2VectorLayerPtr vector;

/* querying the Vector Layer metadata defs */
    sql =
	"SELECT c.f_table_name, c.f_geometry_column, g.srid, g.geometry_type, "
	"g.spatial_index_enabled FROM vector_coverages AS c "
	"JOIN geometry_columns AS g ON (c.f_table_name = g.f_table_name "
	"AND c.f_geometry_column = g.f_geometry_column) "
	"WHERE Lower(c.coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  return NULL;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int len;
		const char *name;
		int ok_table_name = 0;
		int ok_geometry_column = 0;
		int ok_type = 0;
		int ok_srid = 0;
		int ok_index = 0;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      name = (const char *) sqlite3_column_text (stmt, 0);
		      len = strlen (name);
		      if (f_table_name != NULL)
			  free (f_table_name);
		      f_table_name = malloc (len + 1);
		      strcpy (f_table_name, name);
		      ok_table_name = 1;
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
		  {
		      name = (const char *) sqlite3_column_text (stmt, 1);
		      len = strlen (name);
		      if (f_geometry_column != NULL)
			  free (f_geometry_column);
		      f_geometry_column = malloc (len + 1);
		      strcpy (f_geometry_column, name);
		      ok_geometry_column = 1;
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
		  {
		      srid = sqlite3_column_int (stmt, 2);
		      ok_srid = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_INTEGER)
		  {
		      geometry_type = sqlite3_column_int (stmt, 3);
		      ok_type = 1;
		  }
		if (sqlite3_column_type (stmt, 4) == SQLITE_INTEGER)
		  {
		      spatial_index = sqlite3_column_int (stmt, 4);
		      ok_index = 1;
		  }
		if (ok_table_name && ok_geometry_column && ok_type && ok_srid
		    && ok_index)
		    ok = 1;
	    }
      }
    sqlite3_finalize (stmt);

    if (!ok)
      {
	  fprintf (stderr,
		   "ERROR: unable to find a Vector Layer named \"%s\"\n",
		   coverage);
	  if (f_table_name != NULL)
	      free (f_table_name);
	  if (f_geometry_column != NULL)
	      free (f_geometry_column);
	  return NULL;
      }

    vector =
	rl2_create_vector_layer (f_table_name, f_geometry_column,
				 geometry_type, srid, spatial_index);
    free (f_table_name);
    free (f_geometry_column);
    if (vector == NULL)
      {
	  fprintf (stderr,
		   "ERROR: unable to create a Vector Layer Object supporting \"%s\"\n",
		   coverage);
	  return NULL;
      }
    return vector;
}

static void
void_int8_raw_buffer (char *buffer, unsigned int width, unsigned int height,
		      rl2PixelPtr no_data)
{
/* preparing an empty/void INT8 raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned int x;
    unsigned int y;
    char *p = buffer;
    char nd_value = 0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->sampleType == RL2_SAMPLE_INT8 && pxl->nBands == 1)
	    {

		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd_value = sample->int8;
	    }
      }
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	      *p++ = nd_value;
      }
}

static void
void_uint8_raw_buffer (unsigned char *buffer, unsigned int width,
		       unsigned int height, unsigned char num_bands,
		       rl2PixelPtr no_data)
{
/* preparing an empty/void UINT8 raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned int x;
    unsigned int y;
    unsigned char b;
    unsigned char *p = buffer;
    int has_nodata = 0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->sampleType == RL2_SAMPLE_UINT8 && pxl->nBands == num_bands)
	      has_nodata = 1;
      }
    if (!has_nodata)
      {
	  for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		  {
		      for (b = 0; b < num_bands; b++)
			  *p++ = 0;
		  }
	    }
      }
    else
      {
	  for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		  {
		      for (b = 0; b < num_bands; b++)
			{
			    rl2PrivSamplePtr sample = pxl->Samples + b;
			    *p++ = sample->uint8;
			}
		  }
	    }
      }
}

static void
void_int16_raw_buffer (short *buffer, unsigned int width,
		       unsigned int height, rl2PixelPtr no_data)
{
/* preparing an empty/void INT16 raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned int x;
    unsigned int y;
    short *p = buffer;
    short nd_value = 0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->sampleType == RL2_SAMPLE_INT16 && pxl->nBands == 1)
	    {

		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd_value = sample->int16;
	    }
      }
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	      *p++ = nd_value;
      }
}

static void
void_uint16_raw_buffer (unsigned short *buffer, unsigned int width,
			unsigned int height, unsigned char num_bands,
			rl2PixelPtr no_data)
{
/* preparing an empty/void UINT16 raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned int x;
    unsigned int y;
    unsigned char b;
    unsigned short *p = buffer;
    int has_nodata = 0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->sampleType == RL2_SAMPLE_UINT16 && pxl->nBands == num_bands)
	      has_nodata = 1;
      }
    if (!has_nodata)
      {
	  for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		  {
		      for (b = 0; b < num_bands; b++)
			  *p++ = 0;
		  }
	    }
      }
    else
      {
	  for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		  {
		      for (b = 0; b < num_bands; b++)
			{
			    rl2PrivSamplePtr sample = pxl->Samples + b;
			    *p++ = sample->uint16;
			}
		  }
	    }
      }
}

static void
void_int32_raw_buffer (int *buffer, unsigned int width, unsigned int height,
		       rl2PixelPtr no_data)
{
/* preparing an empty/void INT32 raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned int x;
    unsigned int y;
    int *p = buffer;
    int nd_value = 0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->sampleType == RL2_SAMPLE_INT32 && pxl->nBands == 1)
	    {

		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd_value = sample->int32;
	    }
      }
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	      *p++ = nd_value;
      }
}

static void
void_uint32_raw_buffer (unsigned int *buffer, unsigned int width,
			unsigned int height, rl2PixelPtr no_data)
{
/* preparing an empty/void UINT32 raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned int x;
    unsigned int y;
    unsigned int *p = buffer;
    unsigned int nd_value = 0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->sampleType == RL2_SAMPLE_UINT32 && pxl->nBands == 1)
	    {

		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd_value = sample->uint32;
	    }
      }
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	      *p++ = nd_value;
      }
}

static void
void_float_raw_buffer (float *buffer, unsigned int width,
		       unsigned int height, rl2PixelPtr no_data)
{
/* preparing an empty/void FLOAT raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned int x;
    unsigned int y;
    float *p = buffer;
    float nd_value = 0.0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->sampleType == RL2_SAMPLE_FLOAT && pxl->nBands == 1)
	    {

		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd_value = sample->float32;
	    }
      }
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	      *p++ = nd_value;
      }
}

static void
void_double_raw_buffer (double *buffer, unsigned int width,
			unsigned int height, rl2PixelPtr no_data)
{
/* preparing an empty/void DOUBLE raw buffer */
    rl2PrivPixelPtr pxl = NULL;
    unsigned int x;
    unsigned int y;
    double *p = buffer;
    double nd_value = 0.0;
    if (no_data != NULL)
      {
	  pxl = (rl2PrivPixelPtr) no_data;
	  if (pxl->sampleType == RL2_SAMPLE_DOUBLE && pxl->nBands == 1)
	    {

		rl2PrivSamplePtr sample = pxl->Samples + 0;
		nd_value = sample->float64;
	    }
      }
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	      *p++ = nd_value;
      }
}

RL2_PRIVATE void
void_raw_buffer (unsigned char *buffer, unsigned int width,
		 unsigned int height, unsigned char sample_type,
		 unsigned char num_bands, rl2PixelPtr no_data)
{
/* preparing an empty/void buffer */
    switch (sample_type)
      {
      case RL2_SAMPLE_INT8:
	  void_int8_raw_buffer ((char *) buffer, width, height, no_data);
	  break;
      case RL2_SAMPLE_INT16:
	  void_int16_raw_buffer ((short *) buffer, width, height, no_data);
	  break;
      case RL2_SAMPLE_UINT16:
	  void_uint16_raw_buffer ((unsigned short *) buffer, width, height,
				  num_bands, no_data);
	  break;
      case RL2_SAMPLE_INT32:
	  void_int32_raw_buffer ((int *) buffer, width, height, no_data);
	  break;
      case RL2_SAMPLE_UINT32:
	  void_uint32_raw_buffer ((unsigned int *) buffer, width, height,
				  no_data);
	  break;
      case RL2_SAMPLE_FLOAT:
	  void_float_raw_buffer ((float *) buffer, width, height, no_data);
	  break;
      case RL2_SAMPLE_DOUBLE:
	  void_double_raw_buffer ((double *) buffer, width, height, no_data);
	  break;
      default:
	  void_uint8_raw_buffer ((unsigned char *) buffer, width, height,
				 num_bands, no_data);
	  break;
      };
}

RL2_PRIVATE void
void_raw_buffer_palette (unsigned char *buffer, unsigned int width,
			 unsigned int height, rl2PixelPtr no_data)
{
/* preparing an empty/void buffer (PALETTE) */
    unsigned int row;
    unsigned int col;
    unsigned char index = 0;
    unsigned char *p = buffer;

    if (no_data != NULL)
      {
	  /* retrieving the NO-DATA value */
	  unsigned char sample_type;
	  unsigned char pixel_type;
	  unsigned char num_bands;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &num_bands) != RL2_OK)
	      goto done;
	  if (pixel_type != RL2_PIXEL_PALETTE || num_bands != 1)
	      goto done;
	  switch (sample_type)
	    {
	    case RL2_SAMPLE_1_BIT:
		rl2_get_pixel_sample_1bit (no_data, &index);
		break;
	    case RL2_SAMPLE_2_BIT:
		rl2_get_pixel_sample_2bit (no_data, &index);
		break;
	    case RL2_SAMPLE_4_BIT:
		rl2_get_pixel_sample_4bit (no_data, &index);
		break;
	    case RL2_SAMPLE_UINT8:
		rl2_get_pixel_sample_uint8 (no_data, 0, &index);
		break;
	    };
      }

  done:
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	      *p++ = index;
      }
}

static void
do_decode_tile (rl2AuxDecoderPtr decoder)
{
/* servicing an AuxDecoder Tile request */
    decoder->raster =
	(rl2PrivRasterPtr) rl2_raster_decode (decoder->scale,
					      decoder->blob_odd,
					      decoder->blob_odd_sz,
					      decoder->blob_even,
					      decoder->blob_even_sz,
					      (rl2PalettePtr)
					      (decoder->palette));
    if (decoder->blob_odd != NULL)
	free (decoder->blob_odd);
    if (decoder->blob_even != NULL)
	free (decoder->blob_even);
    decoder->blob_odd = NULL;
    decoder->blob_even = NULL;
    decoder->palette = NULL;
    if (decoder->raster == NULL)
      {
	  decoder->retcode = RL2_ERROR;
	  return;
      }
    if (!rl2_copy_raw_pixels
	((rl2RasterPtr) (decoder->raster), decoder->outbuf, decoder->width,
	 decoder->height, decoder->sample_type, decoder->num_bands,
	 decoder->auto_ndvi, decoder->red_band_index, decoder->nir_band_index,
	 decoder->x_res, decoder->y_res, decoder->minx, decoder->maxy,
	 decoder->tile_minx, decoder->tile_maxy,
	 (rl2PixelPtr) (decoder->no_data),
	 (rl2RasterSymbolizerPtr) (decoder->style),
	 (rl2RasterStatisticsPtr) (decoder->stats),
  decoder->geom_contains,
  decoder->geom_cutline_blob,
  decoder->geom_cutline_blob_sz,
  decoder->handle))
      {
	  decoder->retcode = RL2_ERROR;
	  return;
      }
    rl2_destroy_raster ((rl2RasterPtr) (decoder->raster));
    decoder->raster = NULL;
    decoder->retcode = RL2_OK;
}

#ifdef _WIN32
DWORD WINAPI
doRunDecoderThread (void *arg)
#else
void *
doRunDecoderThread (void *arg)
#endif
{
/* threaded function: decoding a Tile */
    rl2AuxDecoderPtr decoder = (rl2AuxDecoderPtr) arg;
    do_decode_tile (decoder);
#ifdef _WIN32
    return 0;
#else
    pthread_exit (NULL);
#endif
}

static void
start_decoder_thread (rl2AuxDecoderPtr decoder)
{
/* starting a concurrent thread */
#ifdef _WIN32
    HANDLE thread_handle;
    HANDLE *p_thread;
    DWORD dwThreadId;
    thread_handle =
	CreateThread (NULL, 0, doRunDecoderThread, decoder, 0, &dwThreadId);
    SetThreadPriority (thread_handle, THREAD_PRIORITY_IDLE);
    p_thread = malloc (sizeof (HANDLE));
    *p_thread = thread_handle;
    decoder->opaque_thread_id = p_thread;
#else
    pthread_t thread_id;
    pthread_t *p_thread;
    int ok_prior = 0;
    int policy;
    int min_prio;
    pthread_attr_t attr;
    struct sched_param sp;
    pthread_attr_init (&attr);
    if (pthread_attr_setschedpolicy (&attr, SCHED_RR) == 0)
      {
	  /* attempting to set the lowest priority */
	  if (pthread_attr_getschedpolicy (&attr, &policy) == 0)
	    {
		min_prio = sched_get_priority_min (policy);
		sp.sched_priority = min_prio;
		if (pthread_attr_setschedparam (&attr, &sp) == 0)
		  {
		      /* ok, setting the lowest priority */
		      ok_prior = 1;
		      pthread_create (&thread_id, &attr, doRunDecoderThread,
				      decoder);
		  }
	    }
      }
    if (!ok_prior)
      {
	  /* failure: using standard priority */
	  pthread_create (&thread_id, NULL, doRunDecoderThread, decoder);
      }
    p_thread = malloc (sizeof (pthread_t));
    *p_thread = thread_id;
    decoder->opaque_thread_id = p_thread;
#endif
}

static int
do_run_decoder_children (rl2AuxDecoderPtr * thread_slots, int thread_count)
{
/* concurrent execution of all decoder children threads */
    rl2AuxDecoderPtr decoder;
    int i;
#ifdef _WIN32
    HANDLE *handles;
#endif

    for (i = 0; i < thread_count; i++)
      {
	  /* starting all children threads */
	  decoder = *(thread_slots + i);
	  start_decoder_thread (decoder);
      }

/* waiting until all child threads exit */
#ifdef _WIN32
    handles = malloc (sizeof (HANDLE) * thread_count);
    for (i = 0; i < thread_count; i++)
      {
	  /* initializing the HANDLEs array */
	  HANDLE *pOpaque;
	  decoder = *(thread_slots + i);
	  pOpaque = (HANDLE *) (decoder->opaque_thread_id);
	  *(handles + i) = *pOpaque;
      }
    WaitForMultipleObjects (thread_count, handles, TRUE, INFINITE);
    free (handles);
#else
    for (i = 0; i < thread_count; i++)
      {
	  pthread_t *pOpaque;
	  decoder = *(thread_slots + i);
	  pOpaque = (pthread_t *) (decoder->opaque_thread_id);
	  pthread_join (*pOpaque, NULL);
      }
#endif

/* all children threads have now finished: resuming the main thread */
    for (i = 0; i < thread_count; i++)
      {
	  /* cleaning up a request slot */
	  decoder = *(thread_slots + i);
	  if (decoder->blob_odd != NULL)
	      free (decoder->blob_odd);
	  if (decoder->blob_even != NULL)
	      free (decoder->blob_even);
	  if (decoder->raster != NULL)
	      rl2_destroy_raster ((rl2RasterPtr) (decoder->raster));
	  if (decoder->palette != NULL)
	      rl2_destroy_palette ((rl2PalettePtr) (decoder->palette));
	  if (decoder->opaque_thread_id != NULL)
	      free (decoder->opaque_thread_id);
	  decoder->blob_odd = NULL;
	  decoder->blob_even = NULL;
	  decoder->blob_odd_sz = 0;
	  decoder->blob_even_sz = 0;
	  decoder->raster = NULL;
	  decoder->palette = NULL;
	  decoder->opaque_thread_id = NULL;
   decoder->geom_contains=1;
   if (decoder->geom_cutline_blob != NULL)
    free (decoder->geom_cutline_blob);
   if (decoder->geom_cutline_blob_sz != NULL)
    free (decoder->geom_cutline_blob_sz);
      }
    for (i = 0; i < thread_count; i++)
      {
	  /* checking for eventual errors */
	  decoder = *(thread_slots + i);
	  if (decoder->retcode != RL2_OK)
	    {
		fprintf (stderr, ERR_FRMT64, decoder->tile_id);
		goto error;
	    }
      }
    return 1;

  error:
    return 0;
}

static int
rl2_load_dbms_tiles_common (sqlite3 * handle, int max_threads,
			    sqlite3_stmt * stmt_tiles,
			    sqlite3_stmt * stmt_data, unsigned char *outbuf,
			    unsigned int width, unsigned int height,
			    unsigned char sample_type,
			    unsigned char num_bands, unsigned char auto_ndvi,
			    unsigned char red_band_index,
			    unsigned char nir_band_index, double x_res,
			    double y_res, double minx, double maxy, int scale,
			    rl2PalettePtr palette, rl2PixelPtr no_data,
			    rl2RasterSymbolizerPtr style,
			    rl2RasterStatisticsPtr stats,
       unsigned char *geom_cutline_blob, 
       int *geom_cutline_blob_sz)
{
/* retrieving a full image from DBMS tiles */
    rl2RasterPtr raster = NULL;
    rl2PalettePtr plt = NULL;
    int ret;
    rl2AuxDecoderPtr aux = NULL;
    rl2AuxDecoderPtr decoder;
    rl2AuxDecoderPtr *thread_slots = NULL;
    int thread_count;
    int iaux;
    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;
/* allocating the AuxDecoder array */
    aux = malloc (sizeof (rl2AuxDecoder) * max_threads);
    if (aux == NULL)
	return 0;
    for (iaux = 0; iaux < max_threads; iaux++)
      {
	  /* initializing an empty AuxDecoder */
	  decoder = aux + iaux;
	  decoder->opaque_thread_id = NULL;
	  decoder->blob_odd = NULL;
	  decoder->blob_even = NULL;
	  decoder->blob_odd_sz = 0;
	  decoder->blob_even_sz = 0;
	  decoder->outbuf = outbuf;
	  decoder->width = width;
	  decoder->height = height;
	  decoder->sample_type = sample_type;
	  decoder->num_bands = num_bands;
	  decoder->auto_ndvi = auto_ndvi;
	  decoder->red_band_index = red_band_index;
	  decoder->nir_band_index = nir_band_index;
	  decoder->x_res = x_res;
	  decoder->y_res = y_res;
	  decoder->scale = scale;
	  decoder->minx = minx;
	  decoder->maxy = maxy;
	  decoder->no_data = (rl2PrivPixelPtr) no_data;
	  decoder->style = (rl2PrivRasterSymbolizerPtr) style;
	  decoder->stats = (rl2PrivRasterStatisticsPtr) stats;
	  decoder->raster = NULL;
	  decoder->palette = NULL;
   decoder->geom_contains=1;
   decoder->geom_cutline_blob=NULL;
   decoder->geom_cutline_blob_sz=NULL;
   decoder->handle=NULL;
      }

/* preparing the thread_slots stuct */
    thread_slots = malloc (sizeof (rl2AuxDecoderPtr) * max_threads);
    for (thread_count = 0; thread_count < max_threads; thread_count++)
	*(thread_slots + thread_count) = NULL;
    thread_count = 0;

/* querying the tiles */
    while (1)
      {
	  ret = sqlite3_step (stmt_tiles);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int ok = 0;
		const unsigned char *blob_odd = NULL;
		int blob_odd_sz = 0;
		const unsigned char *blob_even = NULL;
		int blob_even_sz = 0;
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_tiles, 0);
		double tile_minx = sqlite3_column_double (stmt_tiles, 1);
		double tile_maxy = sqlite3_column_double (stmt_tiles, 2);
  double tile_maxx = 0.0;
	 double tile_miny= 0.0;
		decoder = aux + thread_count;
		decoder->tile_id = tile_id;
		decoder->tile_minx = tile_minx;
		decoder->tile_maxy = tile_maxy;
  // tiles completely outside of the geometry will NOT be selected
  //  geom_contains = 0: A portion of this tile is outside of the geometry
  //  geom_contains = 1: The tile is completely inside of the geometry
  int geom_contains = 1;
  if (sqlite3_data_count(stmt_tiles) > 3)
  {
   tile_maxx = sqlite3_column_double (stmt_tiles, 3);
   tile_miny = sqlite3_column_double (stmt_tiles, 4);
   geom_contains=sqlite3_column_int (stmt_tiles, 5);
  }
  decoder->geom_contains=geom_contains;
  decoder->geom_cutline_blob=NULL;
  decoder->geom_cutline_blob_sz=NULL;  
  if (decoder->geom_contains == -1 )
  { // ST_Contains(?,geometry)  malformed ?, when called with RL2_WriteSectionGeoTiff, despite common sql (other than 'section_id = 1 AND') for both
    printf ("-I-> rl2_load_dbms_tiles_common[%d] geom_cutline_blob[%p] size[%d] contains[%d] bbox[%2.7f,%2.7f,%2.7f,%2.7f]\n",sqlite3_data_count(stmt_tiles), geom_cutline_blob,*geom_cutline_blob_sz,geom_contains,tile_minx,tile_miny,tile_maxx,tile_maxy);
  }
  if (decoder->geom_contains == 0)
  { // Only Tiles that NOT completely contained in the Cutline will recieve a Tile-Cutline Geometry
   if ((geom_cutline_blob != NULL) && (geom_cutline_blob_sz != NULL) && (*geom_cutline_blob_sz > 0))
   { // extract from the cutline a portion corresponding to the tile.
    char *sql;
    char *ewkt_cutline=NULL;
    int i_rc=0;
    const unsigned char *blob_cutline = NULL;
		  int blob_cutline_sz = 0;
    sql=sqlite3_mprintf("SELECT ST_Intersection(SetSRID(BuildMBR(%2.7f,%2.7f,%2.7f,%2.7f),ST_SRID(?)),(?))",tile_minx,tile_miny,tile_maxx,tile_maxy);
    sqlite3_stmt *stmt_cutline = NULL;
    i_rc=sqlite3_prepare_v2 (handle, sql, strlen(sql), &stmt_cutline, NULL);
    sqlite3_free (sql); 
    if (i_rc == SQLITE_OK)
    {
     sqlite3_reset (stmt_cutline);
     sqlite3_clear_bindings (stmt_cutline);
     sqlite3_bind_blob (stmt_cutline, 1, geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
     sqlite3_bind_blob (stmt_cutline, 2, geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
	    i_rc = sqlite3_step (stmt_cutline);
	    if (i_rc == SQLITE_DONE)
	     break;		// end of result set 
	    if (i_rc == SQLITE_ROW)
     {
      if (sqlite3_column_type (stmt_cutline, 0) == SQLITE_BLOB)
	 	   {
       blob_cutline = sqlite3_column_blob (stmt_cutline, 0);
       blob_cutline_sz = sqlite3_column_bytes (stmt_cutline, 0);
       if (blob_cutline_sz > 0)
       {
        decoder->geom_cutline_blob_sz=malloc(sizeof(int));
        *decoder->geom_cutline_blob_sz=blob_cutline_sz;
        decoder->geom_cutline_blob=malloc(blob_cutline_sz); 
        memcpy(decoder->geom_cutline_blob, blob_cutline, blob_cutline_sz);
        decoder->handle=handle; // needed for 'cutline_contains_point'
       }
      }     
     }
     i_rc=sqlite3_finalize (stmt_cutline);  
    }     
   }
   // If no cutline is set, use the whole tile
   if (decoder->geom_cutline_blob == NULL)
    decoder->geom_contains=1;
  }
		/* retrieving tile raw data from BLOBs */
 	sqlite3_reset (stmt_data);
		sqlite3_clear_bindings (stmt_data);  
		sqlite3_bind_int64 (stmt_data, 1, tile_id);
		ret = sqlite3_step (stmt_data);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      /* decoding a Tile - may be by using concurrent multithreading */
		      if (sqlite3_column_type (stmt_data, 0) == SQLITE_BLOB)
			{
			    blob_odd = sqlite3_column_blob (stmt_data, 0);
			    blob_odd_sz = sqlite3_column_bytes (stmt_data, 0);
			    decoder->blob_odd = malloc (blob_odd_sz);
			    if (decoder->blob_odd == NULL)
				goto error;
			    memcpy (decoder->blob_odd, blob_odd, blob_odd_sz);
			    decoder->blob_odd_sz = blob_odd_sz;
			    ok = 1;
			}
		      if (scale == RL2_SCALE_1)
			{
			    if (sqlite3_column_type (stmt_data, 1) ==
				SQLITE_BLOB)
			      {
				  blob_even =
				      sqlite3_column_blob (stmt_data, 1);
				  blob_even_sz =
				      sqlite3_column_bytes (stmt_data, 1);
				  decoder->blob_even = malloc (blob_even_sz);
				  if (decoder->blob_even == NULL)
				      goto error;
				  memcpy (decoder->blob_even, blob_even,
					  blob_even_sz);
				  decoder->blob_even_sz = blob_even_sz;
			      }
			}
		  }
		else
		  {
		      fprintf (stderr,
			       "rl2_load_dbms_tiles_common: SELECT tiles data; sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
		if (!ok)
		  {
     if (decoder->geom_cutline_blob != NULL)
      free (decoder->geom_cutline_blob);
     if (decoder->geom_cutline_blob_sz != NULL)
      free (decoder->geom_cutline_blob_sz);
		      if (decoder->blob_odd != NULL)
			  free (decoder->blob_odd);
		      if (decoder->blob_even != NULL)
			  free (decoder->blob_even);
		      decoder->blob_odd = NULL;
		      decoder->blob_even = NULL;
		      decoder->blob_odd_sz = 0;
		      decoder->blob_even_sz = 0;
		  }
		else
		  {
		      /* processing a Tile request (may be under parallel execution) */
		      decoder->palette =
			  (rl2PrivPalettePtr) rl2_clone_palette (palette);
		      if (max_threads > 1)
			{
			    /* adopting a multithreaded strategy */
			    *(thread_slots + thread_count) = decoder;
			    thread_count++;
			    if (thread_count == max_threads)
			      {
				  if (!do_run_decoder_children
				      (thread_slots, thread_count))
				      goto error;
				  thread_count = 0;
			      }
			}
		      else
			{
			    /* single thread execution */
			    do_decode_tile (decoder);
			    if (decoder->retcode != RL2_OK)
			      {
				  fprintf (stderr, ERR_FRMT64, tile_id);
				  goto error;
			      }
			}
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "rl2_load_dbms_tiles_common: SELECT tiles; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    if (max_threads > 1 && thread_count > 0)
      {
	  /* launching the last multithreaded burst */
	  if (!do_run_decoder_children (thread_slots, thread_count))
	      goto error;
      }

    free (aux);
    free (thread_slots);
    return 1;

  error:
    if (aux != NULL)
      {
	  /* AuxDecoder cleanup */
	  for (iaux = 0; iaux < max_threads; iaux++)
	    {
		decoder = aux + iaux;
  if (decoder->geom_cutline_blob != NULL)
   free (decoder->geom_cutline_blob);
  if (decoder->geom_cutline_blob_sz != NULL)
   free (decoder->geom_cutline_blob_sz);
		if (decoder->blob_odd != NULL)
		    free (decoder->blob_odd);
		if (decoder->blob_even != NULL)
		    free (decoder->blob_even);
		if (decoder->raster != NULL)
		    rl2_destroy_raster ((rl2RasterPtr) (decoder->raster));
		if (decoder->palette != NULL)
		    rl2_destroy_palette ((rl2PalettePtr) (decoder->palette));
		if (decoder->opaque_thread_id != NULL)
		    free (decoder->opaque_thread_id);
	    }
	  free (aux);
      }
    if (thread_slots != NULL)
	free (thread_slots);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    if (plt != NULL)
	rl2_destroy_palette (plt);
    return 0;
}

static int
is_nodata_u16 (rl2PrivPixelPtr no_data, const unsigned short *p_in)
{
/* testing for NO-DATA */
    if (no_data != NULL)
      {
	  unsigned char band;
	  int match = 0;
	  rl2PrivSamplePtr sample;
	  for (band = 0; band < no_data->nBands; band++)
	    {
		sample = no_data->Samples + band;
		if (*(p_in + band) == sample->uint16)
		    match++;
	    }
	  if (match == no_data->nBands)
	      return 1;
      }
    return 0;
}

static int
copy_triple_band_raw_pixels_u16 (rl2RasterPtr raster, unsigned short *outbuf,
				 unsigned int width, unsigned int height,
				 unsigned char red_band,
				 unsigned char green_band,
				 unsigned char blue_band, double x_res,
				 double y_res, double minx, double maxy,
				 double tile_minx, double tile_maxy,
				 rl2PixelPtr no_data,
     unsigned char *geom_cutline_blob, 
     int *geom_cutline_blob_sz)
{
/* copying raw pixels into the output buffer - UINT16 */
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned int x;
    unsigned int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in;
    const unsigned char *p_msk;
    unsigned short *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    unsigned char num_bands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;
    rl2PrivPixelPtr no_data_in;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;

    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;
    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	return 0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
	      ignore_no_data = 1;
	  if (pixel_type != RL2_PIXEL_RGB)
	      ignore_no_data = 1;
	  if (nbands != 3)
	      ignore_no_data = 1;
	  if (sample_type != RL2_SAMPLE_UINT16)
	      ignore_no_data = 1;
      }

    p_in = (const unsigned short *) (rst->rasterBuffer);
    p_msk = (unsigned char *) (rst->maskBuffer);
    no_data_in = rst->noData;

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || (unsigned int) out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || (unsigned int) out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width * 3) + (out_x * 3);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = is_nodata_u16 (no_data_in, p_in);
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out += 3;
			    p_in += num_bands;
			}
		      else
			{
			    unsigned short r = *(p_in + red_band);
			    unsigned short g = *(p_in + green_band);
			    unsigned short b = *(p_in + blue_band);
			    p_in += num_bands;
			    *p_out++ = r;
			    *p_out++ = g;
			    *p_out++ = b;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      unsigned short sample;
		      unsigned short r = *(p_in + red_band);
		      unsigned short g = *(p_in + green_band);
		      unsigned short b = *(p_in + blue_band);
		      p_in += num_bands;
		      rl2_get_pixel_sample_uint16 (no_data, 0, &sample);
		      if (sample == r)
			  match++;
		      rl2_get_pixel_sample_uint16 (no_data, 1, &sample);
		      if (sample == g)
			  match++;
		      rl2_get_pixel_sample_uint16 (no_data, 2, &sample);
		      if (sample == b)
			  match++;
		      if (match != 3)
			{
			    /* opaque pixel */
			    *p_out++ = r;
			    *p_out++ = g;
			    *p_out++ = b;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out += 3;
			}
		  }
	    }
      }

    return 1;
}

static int
is_nodata_u8 (rl2PrivPixelPtr no_data, const unsigned char *p_in)
{
/* testing for NO-DATA */
    if (no_data != NULL)
      {
	  unsigned char band;
	  int match = 0;
	  rl2PrivSamplePtr sample;
	  for (band = 0; band < no_data->nBands; band++)
	    {
		sample = no_data->Samples + band;
		if (*(p_in + band) == sample->uint8)
		    match++;
	    }
	  if (match == no_data->nBands)
	      return 1;
      }
    return 0;
}

static int
copy_triple_band_raw_pixels (rl2RasterPtr raster, unsigned char *outbuf,
			     unsigned int width,
			     unsigned int height, unsigned char red_band,
			     unsigned char green_band,
			     unsigned char blue_band, double x_res,
			     double y_res, double minx, double maxy,
			     double tile_minx, double tile_maxy,
			     rl2PixelPtr no_data,
        unsigned char *geom_cutline_blob, 
        int *geom_cutline_blob_sz)
{
/* copying raw pixels into the output buffer */
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned int x;
    unsigned int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in;
    const unsigned char *p_msk;
    unsigned char *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    unsigned char num_bands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;
    rl2PrivPixelPtr no_data_in;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;

    if (rst->sampleType == RL2_SAMPLE_UINT16)
	return copy_triple_band_raw_pixels_u16 (raster,
						(unsigned short *) outbuf,
						width, height, red_band,
						green_band, blue_band, x_res,
						y_res, minx, maxy, tile_minx,
						tile_maxy, no_data,geom_cutline_blob,geom_cutline_blob_sz);

    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;
    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	return 0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
	      ignore_no_data = 1;
	  if (pixel_type != RL2_PIXEL_RGB)
	      ignore_no_data = 1;
	  if (nbands != 3)
	      ignore_no_data = 1;
	  if (sample_type != RL2_SAMPLE_UINT8)
	      ignore_no_data = 1;
      }

    p_in = rst->rasterBuffer;
    p_msk = rst->maskBuffer;
    no_data_in = rst->noData;

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || (unsigned int) out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || (unsigned int) out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width * 3) + (out_x * 3);
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (!transparent)
		    transparent = is_nodata_u8 (no_data_in, p_in);
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out += 3;
			    p_in += num_bands;
			}
		      else
			{
			    unsigned char r = *(p_in + red_band);
			    unsigned char g = *(p_in + green_band);
			    unsigned char b = *(p_in + blue_band);
			    p_in += num_bands;
			    *p_out++ = r;
			    *p_out++ = g;
			    *p_out++ = b;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      unsigned char sample;
		      unsigned char r = *(p_in + red_band);
		      unsigned char g = *(p_in + green_band);
		      unsigned char b = *(p_in + blue_band);
		      p_in += num_bands;
		      rl2_get_pixel_sample_uint8 (no_data, 0, &sample);
		      if (sample == r)
			  match++;
		      rl2_get_pixel_sample_uint8 (no_data, 1, &sample);
		      if (sample == g)
			  match++;
		      rl2_get_pixel_sample_uint8 (no_data, 2, &sample);
		      if (sample == b)
			  match++;
		      if (match != 3)
			{
			    /* opaque pixel */
			    *p_out++ = r;
			    *p_out++ = g;
			    *p_out++ = b;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out += 3;
			}
		  }
	    }
      }

    return 1;
}

static int
load_triple_band_dbms_tiles (sqlite3 * handle, sqlite3_stmt * stmt_tiles,
			     sqlite3_stmt * stmt_data, unsigned char *outbuf,
			     unsigned int width, unsigned int height,
			     unsigned char red_band, unsigned char green_band,
			     unsigned char blue_band, double x_res,
			     double y_res, double minx, double miny,
			     double maxx, double maxy, int level, int scale,
			     rl2PixelPtr no_data,
        unsigned char *geom_cutline_blob, 
        int *geom_cutline_blob_sz)
{
/* retrieving a full image from DBMS tiles */
    rl2RasterPtr raster = NULL;
    int ret;

/* binding the query args */
    sqlite3_reset (stmt_tiles);
    sqlite3_clear_bindings (stmt_tiles);    
    if ((geom_cutline_blob != NULL) && (geom_cutline_blob_sz != NULL) && (*geom_cutline_blob_sz > 0))
    {
     sqlite3_bind_blob (stmt_tiles, 1, geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
     sqlite3_bind_int (stmt_tiles, 2, level);
     sqlite3_bind_double (stmt_tiles, 3, minx);
     sqlite3_bind_double (stmt_tiles, 4, miny);
     sqlite3_bind_double (stmt_tiles, 5, maxx);
     sqlite3_bind_double (stmt_tiles, 6, maxy);
     sqlite3_bind_blob (stmt_tiles, 7, geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
    }
    else
    {
     sqlite3_bind_int (stmt_tiles, 1, level);
     sqlite3_bind_double (stmt_tiles, 2, minx);
     sqlite3_bind_double (stmt_tiles, 3, miny);
     sqlite3_bind_double (stmt_tiles, 4, maxx);
     sqlite3_bind_double (stmt_tiles, 5, maxy);
    }

/* querying the tiles */
    while (1)
      {
	  ret = sqlite3_step (stmt_tiles);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob_odd = NULL;
		int blob_odd_sz = 0;
		const unsigned char *blob_even = NULL;
		int blob_even_sz = 0;
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_tiles, 0);
		double tile_minx = sqlite3_column_double (stmt_tiles, 1);
		double tile_maxy = sqlite3_column_double (stmt_tiles, 2);

		/* retrieving tile raw data from BLOBs */
		sqlite3_reset (stmt_data);
		sqlite3_clear_bindings (stmt_data);
		sqlite3_bind_int64 (stmt_data, 1, tile_id);
		ret = sqlite3_step (stmt_data);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_type (stmt_data, 0) == SQLITE_BLOB)
			{
			    blob_odd = sqlite3_column_blob (stmt_data, 0);
			    blob_odd_sz = sqlite3_column_bytes (stmt_data, 0);
			}
		      if (sqlite3_column_type (stmt_data, 1) == SQLITE_BLOB)
			{
			    blob_even = sqlite3_column_blob (stmt_data, 1);
			    blob_even_sz = sqlite3_column_bytes (stmt_data, 1);
			}
		  }
		else
		  {
		      fprintf (stderr,
			       "load_triple_band_dbms_tiles: SELECT tiles data; sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
		raster =
		    rl2_raster_decode (scale, blob_odd, blob_odd_sz,
				       blob_even, blob_even_sz, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      goto error;
		  }
		if (!copy_triple_band_raw_pixels
		    (raster, outbuf, width, height, red_band, green_band,
		     blue_band, x_res, y_res, minx, maxy, tile_minx,
		     tile_maxy, no_data,geom_cutline_blob,geom_cutline_blob_sz))
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
	  else
	    {
		fprintf (stderr,
			 "load_triple_band_dbms_tiles: SELECT tiles; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }

    return 1;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return 0;
}

static int
copy_mono_band_raw_pixels_u16 (rl2RasterPtr raster, unsigned char *outbuf,
			       unsigned int width, unsigned int height,
			       unsigned char mono_band, double x_res,
			       double y_res, double minx, double maxy,
			       double tile_minx, double tile_maxy,
			       rl2PixelPtr no_data)
{
/* copying raw pixels into the output buffer - UINT16 */
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned int x;
    unsigned int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned short *p_in;
    const unsigned char *p_msk;
    unsigned short *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    unsigned char num_bands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;

    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;
    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	return 0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
	      ignore_no_data = 1;
	  if (pixel_type != RL2_PIXEL_DATAGRID)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type != RL2_SAMPLE_UINT16)
	      ignore_no_data = 1;
      }

    p_in = (const unsigned short *) (rst->rasterBuffer);
    p_msk = (unsigned char *) (rst->maskBuffer);

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < (unsigned int) tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || (unsigned int) out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < (unsigned int) tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || (unsigned int) out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = (unsigned short *) outbuf;
		p_out += (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in += num_bands;
			}
		      else
			{
			    *p_out++ = *(p_in + mono_band);
			    p_in += num_bands;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      unsigned short sample;
		      unsigned short gray = *(p_in + mono_band);
		      p_in += num_bands;
		      rl2_get_pixel_sample_uint16 (no_data, 0, &sample);
		      if (sample == gray)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    *p_out++ = gray;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }

    return 1;
}

static int
copy_mono_band_raw_pixels (rl2RasterPtr raster, unsigned char *outbuf,
			   unsigned int width, unsigned int height,
			   unsigned char mono_band, double x_res,
			   double y_res, double minx, double maxy,
			   double tile_minx, double tile_maxy,
			   rl2PixelPtr no_data)
{
/* copying raw pixels into the output buffer */
    unsigned int tile_width;
    unsigned int tile_height;
    unsigned int x;
    unsigned int y;
    int out_x;
    int out_y;
    double geo_x;
    double geo_y;
    const unsigned char *p_in;
    const unsigned char *p_msk;
    unsigned char *p_out;
    int transparent;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char nbands;
    unsigned char num_bands;
    int ignore_no_data = 1;
    double y_res2 = y_res / 2.0;
    double x_res2 = x_res / 2.0;
    rl2PrivRasterPtr rst = (rl2PrivRasterPtr) raster;

    if (rst->sampleType == RL2_SAMPLE_UINT16)
	return copy_mono_band_raw_pixels_u16 (raster, outbuf, width, height,
					      mono_band, x_res, y_res, minx,
					      maxy, tile_minx, tile_maxy,
					      no_data);

    if (rl2_get_raster_size (raster, &tile_width, &tile_height) != RL2_OK)
	return 0;
    if (rl2_get_raster_type (raster, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	return 0;

    if (no_data != NULL)
      {
	  ignore_no_data = 0;
	  if (rl2_get_pixel_type
	      (no_data, &sample_type, &pixel_type, &nbands) != RL2_OK)
	      ignore_no_data = 1;
	  if (pixel_type != RL2_PIXEL_GRAYSCALE)
	      ignore_no_data = 1;
	  if (nbands != 1)
	      ignore_no_data = 1;
	  if (sample_type != RL2_SAMPLE_UINT8)
	      ignore_no_data = 1;
      }

    p_in = rst->rasterBuffer;
    p_msk = rst->maskBuffer;

    geo_y = tile_maxy + y_res2;
    for (y = 0; y < tile_height; y++)
      {
	  geo_y -= y_res;
	  out_y = (maxy - geo_y) / y_res;
	  if (out_y < 0 || (unsigned int) out_y >= height)
	    {
		p_in += tile_width * num_bands;
		if (p_msk != NULL)
		    p_msk += tile_width;
		continue;
	    }
	  geo_x = tile_minx - x_res2;
	  for (x = 0; x < (unsigned int) tile_width; x++)
	    {
		geo_x += x_res;
		out_x = (geo_x - minx) / x_res;
		if (out_x < 0 || (unsigned int) out_x >= width)
		  {
		      p_in += num_bands;
		      if (p_msk != NULL)
			  p_msk++;
		      continue;
		  }
		p_out = outbuf + (out_y * width) + out_x;
		transparent = 0;
		if (p_msk != NULL)
		  {
		      if (*p_msk++ == 0)
			  transparent = 1;
		  }
		if (transparent || ignore_no_data)
		  {
		      /* already transparent or missing NO-DATA value */
		      if (transparent)
			{
			    /* skipping a transparent pixel */
			    p_out++;
			    p_in += num_bands;
			}
		      else
			{
			    unsigned char gray = *(p_in + mono_band);
			    p_in += num_bands;
			    *p_out++ = gray;
			}
		  }
		else
		  {
		      /* testing for NO-DATA values */
		      int match = 0;
		      unsigned char sample;
		      unsigned char gray = *(p_in + mono_band);
		      p_in += num_bands;
		      rl2_get_pixel_sample_uint8 (no_data, 0, &sample);
		      if (sample == gray)
			  match++;
		      if (match != 1)
			{
			    /* opaque pixel */
			    *p_out++ = gray;
			}
		      else
			{
			    /* NO-DATA pixel */
			    p_out++;
			}
		  }
	    }
      }

    return 1;
}

static int
load_mono_band_dbms_tiles (sqlite3 * handle, sqlite3_stmt * stmt_tiles,
			   sqlite3_stmt * stmt_data, unsigned char *outbuf,
			   unsigned int width, unsigned int height,
			   unsigned char mono_band, double x_res,
			   double y_res, double minx, double miny,
			   double maxx, double maxy, int level, int scale,
			   rl2PixelPtr no_data,
      unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* retrieving a full image from DBMS tiles */
    rl2RasterPtr raster = NULL;
    int ret;

/* binding the query args */
    sqlite3_reset (stmt_tiles);
    sqlite3_clear_bindings (stmt_tiles);
    if ((geom_cutline_blob != NULL) && (geom_cutline_blob_sz != NULL) && (*geom_cutline_blob_sz > 0))
    {
     sqlite3_bind_blob (stmt_tiles, 1, geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
     sqlite3_bind_int (stmt_tiles, 2, level);
     sqlite3_bind_double (stmt_tiles, 3, minx);
     sqlite3_bind_double (stmt_tiles, 4, miny);
     sqlite3_bind_double (stmt_tiles, 5, maxx);
     sqlite3_bind_double (stmt_tiles, 6, maxy);
     sqlite3_bind_blob (stmt_tiles, 7, geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
    }
    else
    {
     sqlite3_bind_int (stmt_tiles, 1, level);
     sqlite3_bind_double (stmt_tiles, 2, minx);
     sqlite3_bind_double (stmt_tiles, 3, miny);
     sqlite3_bind_double (stmt_tiles, 4, maxx);
     sqlite3_bind_double (stmt_tiles, 5, maxy);
    }

/* querying the tiles */
    while (1)
      {
	  ret = sqlite3_step (stmt_tiles);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob_odd = NULL;
		int blob_odd_sz = 0;
		const unsigned char *blob_even = NULL;
		int blob_even_sz = 0;
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_tiles, 0);
		double tile_minx = sqlite3_column_double (stmt_tiles, 1);
		double tile_maxy = sqlite3_column_double (stmt_tiles, 2);

		/* retrieving tile raw data from BLOBs */
		sqlite3_reset (stmt_data);
		sqlite3_clear_bindings (stmt_data);
		sqlite3_bind_int64 (stmt_data, 1, tile_id);
		ret = sqlite3_step (stmt_data);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_type (stmt_data, 0) == SQLITE_BLOB)
			{
			    blob_odd = sqlite3_column_blob (stmt_data, 0);
			    blob_odd_sz = sqlite3_column_bytes (stmt_data, 0);
			}
		      if (sqlite3_column_type (stmt_data, 1) == SQLITE_BLOB)
			{
			    blob_even = sqlite3_column_blob (stmt_data, 1);
			    blob_even_sz = sqlite3_column_bytes (stmt_data, 1);
			}
		  }
		else
		  {
		      fprintf (stderr,
			       "load_mono_band_dbms_tiles: SELECT tiles data; sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
		raster =
		    rl2_raster_decode (scale, blob_odd, blob_odd_sz,
				       blob_even, blob_even_sz, NULL);
		if (raster == NULL)
		  {
		      fprintf (stderr, ERR_FRMT64, tile_id);
		      goto error;
		  }
		if (!copy_mono_band_raw_pixels
		    (raster, outbuf, width, height, mono_band, x_res, y_res,
		     minx, maxy, tile_minx, tile_maxy, no_data))
		    goto error;
		rl2_destroy_raster (raster);
		raster = NULL;
	    }
	  else
	    {
		fprintf (stderr,
			 "load_mono_band_dbms_tiles: SELECT tiles; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }

    return 1;

  error:
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return 0;
}

RL2_PRIVATE int
rl2_load_dbms_tiles (sqlite3 * handle, int max_threads,
		     sqlite3_stmt * stmt_tiles, sqlite3_stmt * stmt_data,
		     unsigned char *outbuf, unsigned int width,
		     unsigned int height, unsigned char sample_type,
		     unsigned char num_bands, unsigned char auto_ndvi,
		     unsigned char red_band_index,
		     unsigned char nir_band_index, double x_res, double y_res,
		     double minx, double miny, double maxx, double maxy,
		     int level, int scale, rl2PalettePtr palette,
		     rl2PixelPtr no_data, rl2RasterSymbolizerPtr style,
		     rl2RasterStatisticsPtr stats,
       unsigned char *geom_cutline_blob, 
       int *geom_cutline_blob_sz)
{
/* binding the query args */
    sqlite3_reset (stmt_tiles);
    sqlite3_clear_bindings (stmt_tiles);    
    if ((geom_cutline_blob != NULL) && (geom_cutline_blob_sz != NULL) && (*geom_cutline_blob_sz > 0))
    { // Will also return the information : isContained (completely inside) and is filtered with Intersects with the Cutline
     sqlite3_bind_blob (stmt_tiles, 1, geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
     sqlite3_bind_int (stmt_tiles, 2, level);
     sqlite3_bind_double (stmt_tiles, 3, minx);
     sqlite3_bind_double (stmt_tiles, 4, miny);
     sqlite3_bind_double (stmt_tiles, 5, maxx);
     sqlite3_bind_double (stmt_tiles, 6, maxy);
     sqlite3_bind_blob (stmt_tiles, 7,geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
    }
    else
    {
     sqlite3_bind_int (stmt_tiles, 1, level);
     sqlite3_bind_double (stmt_tiles, 2, minx);
     sqlite3_bind_double (stmt_tiles, 3, miny);
     sqlite3_bind_double (stmt_tiles, 4, maxx);
     sqlite3_bind_double (stmt_tiles, 5, maxy);
    }

    if (!rl2_load_dbms_tiles_common
	(handle, max_threads, stmt_tiles, stmt_data, outbuf, width, height,
	 sample_type, num_bands, auto_ndvi, red_band_index, nir_band_index,
	 x_res, y_res, minx, maxy, scale, palette, no_data, style, stats,geom_cutline_blob,geom_cutline_blob_sz))
	return 0;
    return 1;
}

RL2_PRIVATE int
rl2_load_dbms_tiles_section (sqlite3 * handle, int max_threads,
			     sqlite3_int64 section_id,
			     sqlite3_stmt * stmt_tiles,
			     sqlite3_stmt * stmt_data, unsigned char *outbuf,
			     unsigned int width, unsigned int height,
			     unsigned char sample_type,
			     unsigned char num_bands, unsigned char auto_ndvi,
			     unsigned char red_band_index,
			     unsigned char nir_band_index, double x_res,
			     double y_res, double minx, double maxy,
			     int scale, rl2PalettePtr palette,
			     rl2PixelPtr no_data,
        unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* binding the query args */
    sqlite3_reset (stmt_tiles);
    sqlite3_clear_bindings (stmt_tiles);
    if ((geom_cutline_blob != NULL) && (geom_cutline_blob_sz != NULL) && (*geom_cutline_blob_sz > 0))
    { // mj10777: not called with RL2_WriteSectionGeoTiff
     sqlite3_bind_blob (stmt_tiles, 1, geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
     sqlite3_bind_int (stmt_tiles, 2, section_id);
     double maxx=(minx+((double)width*x_res));
     double miny=(maxy+((double)height*y_res));
     sqlite3_bind_double (stmt_tiles, 3, minx);
     sqlite3_bind_double (stmt_tiles, 4, miny);
     sqlite3_bind_double (stmt_tiles, 5, maxx);
     sqlite3_bind_double (stmt_tiles, 6, maxy);
     sqlite3_bind_blob (stmt_tiles, 7,geom_cutline_blob, *geom_cutline_blob_sz, SQLITE_STATIC);
    }
    else
    {
     sqlite3_bind_int (stmt_tiles, 1, section_id);
    }
    if (!rl2_load_dbms_tiles_common
	(handle, max_threads, stmt_tiles, stmt_data, outbuf, width, height,
	 sample_type, num_bands, auto_ndvi, red_band_index, nir_band_index,
	 x_res, y_res, minx, maxy, scale, palette, no_data, NULL, NULL,geom_cutline_blob,geom_cutline_blob_sz))
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_find_matching_resolution (sqlite3 * handle, rl2CoveragePtr cvg,
			      int by_section, sqlite3_int64 section_id,
			      double *x_res, double *y_res,
			      unsigned char *level, unsigned char *scale)
{
/* attempting to identify the corresponding resolution level */
    rl2PrivCoveragePtr coverage = (rl2PrivCoveragePtr) cvg;
    int ret;
    int found = 0;
    int x_level;
    int x_scale;
    double z_x_res;
    double z_y_res;
    char *xcoverage;
    char *xxcoverage;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    int mixed_resolutions = 0;

    if (coverage == NULL)
	return RL2_ERROR;
    if (coverage->coverageName == NULL)
	return RL2_ERROR;

    if (rl2_is_mixed_resolutions_coverage (handle, coverage->coverageName) > 0)
	mixed_resolutions = 1;
    if (!by_section && mixed_resolutions)
      {
	  /* undeclared Section on Mixed Resolution Coverage */
	  return RL2_ERROR;
      }

    if (mixed_resolutions)
      {
	  /* Mixed Resolutions */
	  char sctn[1024];
#if defined(_WIN32) && !defined(__MINGW32__)
	  sprintf (sctn, "%I64d", section_id);
#else
	  sprintf (sctn, "%lld", section_id);
#endif
	  xcoverage =
	      sqlite3_mprintf ("%s_section_levels", coverage->coverageName);
	  xxcoverage = rl2_double_quoted_sql (xcoverage);
	  sqlite3_free (xcoverage);
	  sql =
	      sqlite3_mprintf
	      ("SELECT pyramid_level, x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, y_resolution_1_4, "
	       "x_resolution_1_8, y_resolution_1_8 FROM \"%s\""
	       "WHERE section_id = %s", xxcoverage, sctn);
      }
    else
      {
	  /* unique Resolution */
	  xcoverage = sqlite3_mprintf ("%s_levels", coverage->coverageName);
	  xxcoverage = rl2_double_quoted_sql (xcoverage);
	  sqlite3_free (xcoverage);
	  sql =
	      sqlite3_mprintf
	      ("SELECT pyramid_level, x_resolution_1_1, y_resolution_1_1, "
	       "x_resolution_1_2, y_resolution_1_2, x_resolution_1_4, y_resolution_1_4, "
	       "x_resolution_1_8, y_resolution_1_8 FROM \"%s\"", xxcoverage);
      }
    free (xxcoverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_free (sql);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		double xx_res;
		double yy_res;
		double confidence;
		int ok;
		int lvl = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		  {
		      ok = 1;
		      xx_res = sqlite3_column_double (stmt, 1);
		      yy_res = sqlite3_column_double (stmt, 2);
		      confidence = xx_res / 100.0;
		      if (*x_res < (xx_res - confidence)
			  || *x_res > (xx_res + confidence))
			  ok = 0;
		      confidence = yy_res / 100.0;
		      if (*y_res < (yy_res - confidence)
			  || *y_res > (yy_res + confidence))
			  ok = 0;
		      if (ok)
			{
			    found = 1;
			    x_level = lvl;
			    x_scale = RL2_SCALE_1;
			    z_x_res = xx_res;
			    z_y_res = yy_res;
			}
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 4) == SQLITE_FLOAT)
		  {
		      ok = 1;
		      xx_res = sqlite3_column_double (stmt, 3);
		      yy_res = sqlite3_column_double (stmt, 4);
		      confidence = xx_res / 100.0;
		      if (*x_res < (xx_res - confidence)
			  || *x_res > (xx_res + confidence))
			  ok = 0;
		      confidence = yy_res / 100.0;
		      if (*y_res < (yy_res - confidence)
			  || *y_res > (yy_res + confidence))
			  ok = 0;
		      if (ok)
			{
			    found = 1;
			    x_level = lvl;
			    x_scale = RL2_SCALE_2;
			    z_x_res = xx_res;
			    z_y_res = yy_res;
			}
		  }
		if (sqlite3_column_type (stmt, 5) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 6) == SQLITE_FLOAT)
		  {
		      ok = 1;
		      xx_res = sqlite3_column_double (stmt, 5);
		      yy_res = sqlite3_column_double (stmt, 6);
		      confidence = xx_res / 100.0;
		      if (*x_res < (xx_res - confidence)
			  || *x_res > (xx_res + confidence))
			  ok = 0;
		      confidence = yy_res / 100.0;
		      if (*y_res < (yy_res - confidence)
			  || *y_res > (yy_res + confidence))
			  ok = 0;
		      if (ok)
			{
			    found = 1;
			    x_level = lvl;
			    x_scale = RL2_SCALE_4;
			    z_x_res = xx_res;
			    z_y_res = yy_res;
			}
		  }
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT
		    && sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      ok = 1;
		      xx_res = sqlite3_column_double (stmt, 7);
		      yy_res = sqlite3_column_double (stmt, 8);
		      confidence = xx_res / 100.0;
		      if (*x_res < (xx_res - confidence)
			  || *x_res > (xx_res + confidence))
			  ok = 0;
		      confidence = yy_res / 100.0;
		      if (*y_res < (yy_res - confidence)
			  || *y_res > (yy_res + confidence))
			  ok = 0;
		      if (ok)
			{
			    found = 1;
			    x_level = lvl;
			    x_scale = RL2_SCALE_8;
			    z_x_res = xx_res;
			    z_y_res = yy_res;
			}
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (found)
      {
	  *level = x_level;
	  *scale = x_scale;
	  *x_res = z_x_res;
	  *y_res = z_y_res;
	  return RL2_OK;
      }
    return RL2_ERROR;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_PRIVATE int
rl2_has_styled_rgb_colors (rl2RasterSymbolizerPtr style)
{
/* testing for a RasterSymbolizer requiring RGB colors */
    rl2PrivColorMapPointPtr color;
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return 0;
    if (stl->shadedRelief && stl->brightnessOnly)
	return 0;
    if (stl->categorize != NULL)
      {
	  if (stl->categorize->dfltRed == stl->categorize->dfltGreen
	      && stl->categorize->dfltRed == stl->categorize->dfltBlue)
	      ;
	  else
	      return 1;
	  if (stl->categorize->baseRed == stl->categorize->baseGreen
	      && stl->categorize->baseRed == stl->categorize->baseBlue)
	      ;
	  else
	      return 1;
	  color = stl->categorize->first;
	  while (color != NULL)
	    {
		if (color->red == color->green && color->red == color->blue)
		    ;
		else
		    return 1;
		color = color->next;
	    }
      }
    if (stl->interpolate != NULL)
      {
	  if (stl->interpolate->dfltRed == stl->interpolate->dfltGreen
	      && stl->interpolate->dfltRed == stl->interpolate->dfltBlue)
	      ;
	  else
	      return 1;
	  color = stl->interpolate->first;
	  while (color != NULL)
	    {
		if (color->red == color->green && color->red == color->blue)
		    ;
		else
		    return 1;
		color = color->next;
	    }
      }
    return 0;
}

RL2_PRIVATE double
rl2_get_shaded_relief_scale_factor (sqlite3 * handle, const char *coverage)
{
/* return the appropriate Scale Factor for Shaded Relief
/  when SRID is of the Long/Lat type
/  this is strictly required because in this case
/  X and Y are measured in degrees, but elevations 
/  (Z) are measured in meters
*/
    double scale_factor = 1.0;
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    char *sql = sqlite3_mprintf ("SELECT s.srid FROM raster_coverages AS r "
				 "JOIN spatial_ref_sys AS s ON (s.srid = r.srid AND "
				 "s.proj4text LIKE '%%+proj=longlat%%') "
				 "WHERE Lower(r.coverage_name) = Lower(%Q)",
				 coverage);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return scale_factor;
    for (i = 1; i <= rows; i++)
	scale_factor = 11.1120;
    sqlite3_free_table (results);
    return scale_factor;
}

RL2_PRIVATE int
rl2_get_raw_raster_data_common (sqlite3 * handle, int max_threads,
				rl2CoveragePtr cvg, int by_section,
				sqlite3_int64 section_id, unsigned int width,
				unsigned int height, double minx, double miny,
				double maxx, double maxy, double x_res,
				double y_res, unsigned char **buffer,
				int *buf_size, rl2PalettePtr * palette,
				unsigned char out_pixel, rl2PixelPtr bgcolor,
				rl2RasterSymbolizerPtr style,
				rl2RasterStatisticsPtr stats,
    unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    rl2PalettePtr plt = NULL;
    rl2PixelPtr no_data = NULL;
    rl2PixelPtr kill_no_data = NULL;
    const char *coverage;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    int pix_sz = 1;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char cvg_pixel_type;
    unsigned char num_bands;
    char *xtiles;
    char *xxtiles;
    char *xdata;
    char *xxdata;
    char *sql;
    char *s_sql_start;
    char *s_sql_end;
    char *s_sql_statement;
    sqlite3_stmt *stmt_tiles = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;
    int has_shaded_relief;
    int brightness_only;
    double relief_factor;
    float *shaded_relief = NULL;
    int shaded_relief_sz;
    unsigned char red_band = 0;
    unsigned char green_band = 0;
    unsigned char blue_band = 0;
    unsigned char nir_band = 0;
    unsigned char auto_ndvi = 0;
    if (cvg == NULL || handle == NULL)
	goto error;
    coverage = rl2_get_coverage_name (cvg);
    if (coverage == NULL)
	goto error;
    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	goto error;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;

    if (rl2_get_dbms_coverage_default_bands
	(handle, coverage, &red_band, &green_band, &blue_band,
	 &nir_band) == RL2_OK)
      {
	  /* testing for Auto NDVI */
	  if (rl2_is_dbms_coverage_auto_ndvi_enabled (handle, coverage) ==
	      RL2_TRUE)
	      auto_ndvi = 1;
      }

    cvg_pixel_type = pixel_type;

    if (pixel_type == RL2_PIXEL_MONOCHROME && out_pixel == RL2_PIXEL_GRAYSCALE)
      {
	  /* Pyramid tiles MONOCHROME */
	  rl2PixelPtr nd = NULL;
	  nd = rl2_get_coverage_no_data (cvg);
	  if (nd != NULL)
	    {
		/* creating a Grayscale NoData pixel */
		rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) nd;
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		no_data = rl2_create_pixel (RL2_SAMPLE_UINT8,
					    RL2_PIXEL_GRAYSCALE, 1);
		kill_no_data = no_data;
		if (sample->uint8 == 0)
		    rl2_set_pixel_sample_uint8 (no_data,
						RL2_GRAYSCALE_BAND, 255);
		else
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 0);
	    }
	  sample_type = RL2_SAMPLE_UINT8;
	  pixel_type = RL2_PIXEL_GRAYSCALE;
	  num_bands = 1;
      }
    else if (pixel_type == RL2_PIXEL_PALETTE && out_pixel == RL2_PIXEL_RGB)
      {
	  /* Pyramid tiles PALETTE */
	  rl2PixelPtr nd = NULL;
	  nd = rl2_get_coverage_no_data (cvg);
	  plt = rl2_get_dbms_palette (handle, coverage);
	  if (nd != NULL)
	    {
		/* creating an RGB NoData pixel */
		rl2PrivPixelPtr pxl = (rl2PrivPixelPtr) nd;
		rl2PrivSamplePtr sample = pxl->Samples + 0;
		no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
		kill_no_data = no_data;
		if (plt == NULL)
		  {
		      /* default: white */
		      rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, 255);
		      rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, 255);
		      rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, 255);
		  }
		else
		  {
		      /* retrieving the color from within the palette */
		      int ok = 0;
		      unsigned short num_entries;
		      unsigned char *red = NULL;
		      unsigned char *green = NULL;
		      unsigned char *blue = NULL;
		      if (rl2_get_palette_colors
			  (plt, &num_entries, &red, &green, &blue) == RL2_OK)
			{
			    if (sample->uint8 < num_entries)
			      {
				  rl2_set_pixel_sample_uint8 (no_data,
							      RL2_RED_BAND,
							      red
							      [sample->uint8]);
				  rl2_set_pixel_sample_uint8 (no_data,
							      RL2_GREEN_BAND,
							      green
							      [sample->uint8]);
				  rl2_set_pixel_sample_uint8 (no_data,
							      RL2_BLUE_BAND,
							      blue
							      [sample->uint8]);
				  ok = 1;
			      }
			    free (red);
			    free (green);
			    free (blue);
			}
		      if (!ok)
			{
			    /* default: white */
			    rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND,
							255);
			    rl2_set_pixel_sample_uint8 (no_data,
							RL2_GREEN_BAND, 255);
			    rl2_set_pixel_sample_uint8 (no_data,
							RL2_BLUE_BAND, 255);
			}
		  }
	    }
	  if (plt != NULL)
	      rl2_destroy_palette (plt);
	  plt = NULL;
	  sample_type = RL2_SAMPLE_UINT8;
	  pixel_type = RL2_PIXEL_RGB;
	  num_bands = 3;
      }
    else
      {
	  if (pixel_type == RL2_PIXEL_PALETTE)
	    {
		/* attempting to retrieve the Coverage's Palette */
		plt = rl2_get_dbms_palette (handle, coverage);
		if (plt == NULL)
		    goto error;
	    }
	  no_data = rl2_get_coverage_no_data (cvg);
      }

    if (style != NULL && stats != NULL)
      {
	  if (out_pixel == RL2_PIXEL_RGB)
	    {
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		num_bands = 3;
	    }
	  if (out_pixel == RL2_PIXEL_GRAYSCALE)
	    {
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_GRAYSCALE;
		num_bands = 1;
	    }
      }

    switch (sample_type)
      {
      case RL2_SAMPLE_INT16:
      case RL2_SAMPLE_UINT16:
	  pix_sz = 2;
	  break;
      case RL2_SAMPLE_INT32:
      case RL2_SAMPLE_UINT32:
      case RL2_SAMPLE_FLOAT:
	  pix_sz = 4;
	  break;
      case RL2_SAMPLE_DOUBLE:
	  pix_sz = 8;
	  break;
      };
    if (out_pixel == RL2_PIXEL_GRAYSCALE
	&& cvg_pixel_type == RL2_PIXEL_DATAGRID)
      {
	  if (rl2_has_styled_rgb_colors (style))
	    {
		/* RGB RasterSymbolizer: promoting to RGB */
		out_pixel = RL2_PIXEL_RGB;
		sample_type = RL2_SAMPLE_UINT8;
		pixel_type = RL2_PIXEL_RGB;
		pix_sz = 1;
		num_bands = 3;
	    }
      }
    bufpix_size = pix_sz * num_bands * width * height;
    bufpix = malloc (bufpix_size);
    if (bufpix == NULL)
      {
	  fprintf (stderr,
		   "rl2_get_raw_raster_data: Insufficient Memory !!!\n");
	  goto error;
      }

    if (style != NULL)
      {
	  /* testing for Shaded Relief */
	  if (rl2_has_raster_symbolizer_shaded_relief
	      (style, &has_shaded_relief) != RL2_OK)
	      goto error;
	  if (has_shaded_relief)
	    {
		/* preparing a Shaded Relief mask */
		double scale_factor =
		    rl2_get_shaded_relief_scale_factor (handle, coverage);
		if (rl2_get_raster_symbolizer_shaded_relief
		    (style, &brightness_only, &relief_factor) != RL2_OK)
		    goto error;
		if (rl2_build_shaded_relief_mask
		    (handle, max_threads, cvg, relief_factor, scale_factor,
		     width, height, minx, miny, maxx, maxy, x_res, y_res,
		     &shaded_relief, &shaded_relief_sz) != RL2_OK)
		    goto error;

		if (brightness_only || !rl2_has_styled_rgb_colors (style))
		  {
		      /* returning a Grayscale ShadedRelief (BrightnessOnly) */
		      unsigned int row;
		      unsigned int col;
		      float *p_in = shaded_relief;
		      unsigned char *p_out = bufpix;
		      if (bgcolor != NULL)
			  void_raw_buffer (bufpix, width, height, sample_type,
					   num_bands, bgcolor);
		      else
			  void_raw_buffer (bufpix, width, height, sample_type,
					   num_bands, no_data);
		      for (row = 0; row < height; row++)
			{
			    for (col = 0; col < width; col++)
			      {
				  float coeff = *p_in++;
				  if (coeff < 0.0)
				      p_out++;	/* transparent */
				  else
				      *p_out++ =
					  (unsigned char) (255.0 * coeff);
			      }
			}
		      free (shaded_relief);
		      *buffer = bufpix;
		      *buf_size = bufpix_size;
		      if (kill_no_data != NULL)
			  rl2_destroy_pixel (kill_no_data);
		      return RL2_OK;
		  }
	    }
      }
/* preparing the "tiles" SQL query */
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = rl2_double_quoted_sql (xtiles);
    if (geom_cutline_blob != NULL)
    { // add cutline logic to sql (0=A portion of the tile is outside of 'geom_cutline_blob' ; 1 [default] the whole tile is inside of 'geom_cutline_blob')
     // 1 if GEOM-1[geom_cutline] completely contains GEOM-2[geom_search]
     s_sql_start = sqlite3_mprintf("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry),MbrMaxX(geometry), MbrMinY(geometry),ST_Contains(?,geometry)");
    }
    else
     s_sql_start = sqlite3_mprintf("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry)");
    if (by_section)
      {
	  /* only from a single Section */
	  char sctn[1024];
#if defined(_WIN32) && !defined(__MINGW32__)
	  sprintf (sctn, "%I64d", section_id);
#else
	  sprintf (sctn, "%lld", section_id);
#endif
	  sql =
	      sqlite3_mprintf
	      (" FROM \"%s\" "
	       "WHERE section_id = %s AND pyramid_level = ? AND ROWID IN ( "
	       "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	       "AND search_frame = BuildMBR(?, ?, ?, ?)", xxtiles, sctn,
	       xtiles);
      }
    else
      {
	  /* whole Coverage */
	  sql =
	      sqlite3_mprintf
	      ( " FROM \"%s\" " 
         "WHERE pyramid_level = ? AND ROWID IN ( "
	       "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	       "AND search_frame = BuildMBR(?, ?, ?, ?)", xxtiles, xtiles);
      }
      
    if (geom_cutline_blob != NULL)
    { // add cutline logic to sql (ignore all tiles that are completely outside of 'geom_cutline_blob')
     s_sql_end = sqlite3_mprintf(" AND ST_Intersects(geometry,?))");
    }
    else
     s_sql_end = sqlite3_mprintf(")");    
    s_sql_statement = sqlite3_mprintf("%s%s%s",s_sql_start,sql,s_sql_end);
    sqlite3_free (s_sql_end);
    sqlite3_free (s_sql_start);
    sqlite3_free (sql);
    sqlite3_free (xtiles);
    free (xxtiles);
    // printf ("-I-> rl2_get_raw_raster_data_common [stmt_tiles]: by_section[%d]  sql[%s]\n",by_section,  s_sql_statement);
    ret = sqlite3_prepare_v2 (handle, s_sql_statement, strlen (s_sql_statement), &stmt_tiles, NULL);
    sqlite3_free (s_sql_statement);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT raw tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }
    if (scale == RL2_SCALE_1)
      {
	  /* preparing the data SQL query - both ODD and EVEN */
	  xdata = sqlite3_mprintf ("%s_tile_data", coverage);
	  xxdata = rl2_double_quoted_sql (xdata);
	  sqlite3_free (xdata);
	  sql = sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even "
				 "FROM \"%s\" WHERE tile_id = ?", xxdata);
	  free (xxdata);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("SELECT raw tiles data(2) SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }
    else
      {
	  /* preparing the data SQL query - only ODD */
	  xdata = sqlite3_mprintf ("%s_tile_data", coverage);
	  xxdata = rl2_double_quoted_sql (xdata);
	  sqlite3_free (xdata);
	  sql = sqlite3_mprintf ("SELECT tile_data_odd "
				 "FROM \"%s\" WHERE tile_id = ?", xxdata);
	  free (xxdata);
	  ret =
	      sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		printf ("SELECT raw tiles data(1) SQL error: %s\n",
			sqlite3_errmsg (handle));
		goto error;
	    }
      }
/* preparing a raw pixels buffer */
    if (pixel_type == RL2_PIXEL_PALETTE)
	void_raw_buffer_palette (bufpix, width, height, no_data);
    else
      {
	  if (bgcolor != NULL)
	      void_raw_buffer (bufpix, width, height, sample_type, num_bands,
			       bgcolor);
	  else
	      void_raw_buffer (bufpix, width, height, sample_type, num_bands,
			       no_data);
      }      
    if (!rl2_load_dbms_tiles
	(handle, max_threads, stmt_tiles, stmt_data, bufpix, width, height,
	 sample_type, num_bands, auto_ndvi, red_band, nir_band, xx_res,
	 yy_res, minx, miny, maxx, maxy, level, scale, plt, no_data, style,
	 stats,geom_cutline_blob,geom_cutline_blob_sz))
	goto error;
    if (kill_no_data != NULL)
	rl2_destroy_pixel (kill_no_data);
    sqlite3_finalize (stmt_tiles);
    sqlite3_finalize (stmt_data);
    if (shaded_relief != NULL)
      {
	  /* applying the Shaded Relief */
	  unsigned int row;
	  unsigned int col;
	  float *p_in = shaded_relief;
	  unsigned char *p_out = bufpix;
	  for (row = 0; row < height; row++)
	    {
		for (col = 0; col < width; col++)
		  {
		      float coeff = *p_in++;
		      if (coeff < 0.0)
			  p_out += 3;	/* unaffected */
		      else
			{
			    unsigned char r = *p_out;
			    unsigned char g = *(p_out + 1);
			    unsigned char b = *(p_out + 2);
			    *p_out++ = (unsigned char) (r * coeff);
			    *p_out++ = (unsigned char) (g * coeff);
			    *p_out++ = (unsigned char) (b * coeff);
			}
		  }
	    }
      }
    *buffer = bufpix;
    *buf_size = bufpix_size;
    if (palette != NULL)
	*palette = plt;
    if (shaded_relief != NULL)
	free (shaded_relief);
    return RL2_OK;

  error:
    if (stmt_tiles != NULL)
	sqlite3_finalize (stmt_tiles);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (bufpix != NULL)
	free (bufpix);
    if (kill_no_data != NULL)
	rl2_destroy_pixel (kill_no_data);
    if (shaded_relief != NULL)
	free (shaded_relief);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raw_raster_data (sqlite3 * handle, int max_threads,
			 rl2CoveragePtr cvg, unsigned int width,
			 unsigned int height, double minx, double miny,
			 double maxx, double maxy, double x_res, double y_res,
			 unsigned char **buffer, int *buf_size,
			 rl2PalettePtr * palette, unsigned char out_pixel,
    unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    return rl2_get_raw_raster_data_common (handle, max_threads, cvg, 0, 0,
					   width, height, minx, miny, maxx,
					   maxy, x_res, y_res, buffer,
					   buf_size, palette, out_pixel, NULL,
					   NULL, NULL,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_get_section_raw_raster_data (sqlite3 * handle, int max_threads,
				 rl2CoveragePtr cvg, sqlite3_int64 section_id,
				 unsigned int width, unsigned int height,
				 double minx, double miny, double maxx,
				 double maxy, double x_res, double y_res,
				 unsigned char **buffer, int *buf_size,
				 rl2PalettePtr * palette,
				 unsigned char out_pixel,
     unsigned char *geom_cutline_blob, 
     int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage/Section */
    return rl2_get_raw_raster_data_common (handle, max_threads, cvg, 1,
					   section_id, width, height, minx,
					   miny, maxx, maxy, x_res, y_res,
					   buffer, buf_size, palette,
					   out_pixel, NULL, NULL, NULL,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
get_triple_band_raw_raster_data_common (int by_section, sqlite3 * handle,
					rl2CoveragePtr cvg,
					sqlite3_int64 section_id,
					unsigned int width,
					unsigned int height, double minx,
					double miny, double maxx, double maxy,
					double x_res, double y_res,
					unsigned char red_band,
					unsigned char green_band,
					unsigned char blue_band,
					unsigned char **buffer, int *buf_size,
					rl2PixelPtr bgcolor,
     unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    rl2PixelPtr no_data = NULL;
    const char *coverage;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    char *xtiles;
    char *xxtiles;
    char *xdata;
    char *xxdata;
    char *sql;
    char *s_sql_start;
    char *s_sql_end;
    char *s_sql_statement;
    sqlite3_stmt *stmt_tiles = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;
    if (cvg == NULL || handle == NULL)
	goto error;
    coverage = rl2_get_coverage_name (cvg);
    if (coverage == NULL)
	goto error;
    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	goto error;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (red_band >= num_bands)
	goto error;
    if (green_band >= num_bands)
	goto error;
    if (blue_band >= num_bands)
	goto error;

    if (bgcolor != NULL)
      {
	  /* using the externally define background color */
	  no_data = bgcolor;
	  goto ok_no_data;
      }

  ok_no_data:
    bufpix_size = 3 * width * height;
    if (sample_type == RL2_SAMPLE_UINT16)
	bufpix_size *= 2;
    bufpix = malloc (bufpix_size);
    if (bufpix == NULL)
      {
	  fprintf (stderr,
		   "rl2_get_triple_band_raw_raster_data: Insufficient Memory !!!\n");
	  goto error;
      }
    
/* preparing the "tiles" SQL query */
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = rl2_double_quoted_sql (xtiles);
    if (geom_cutline_blob != NULL)
    { // add cutline logic to sql (0=A portion of the tile is outside of 'geom_cutline_blob' ; 1 [default] the whole tile is inside of 'geom_cutline_blob')
     // 1 if GEOM-1[geom_cutline] completely contains GEOM-2[geom_search]
     s_sql_start = sqlite3_mprintf("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry),MbrMaxX(geometry), MbrMinY(geometry),ST_Contains(?,geometry)");
    }
    else
     s_sql_start = sqlite3_mprintf("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry)");
    if (by_section)
      {
	  /* only from a single Section */
	  char sctn[1024];
#if defined(_WIN32) && !defined(__MINGW32__)
	  sprintf (sctn, "%I64d", section_id);
#else
	  sprintf (sctn, "%lld", section_id);
#endif
	  sql =
	      sqlite3_mprintf
	      (" FROM \"%s\" "
	       "WHERE section_id = %s AND pyramid_level = ? AND ROWID IN ( "
	       "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	       "AND search_frame = BuildMBR(?, ?, ?, ?)", xxtiles, sctn,
	       xtiles);
      }
    else
      {
	  /* whole Coverage */
	  sql =
	      sqlite3_mprintf
	      ( " FROM \"%s\" " 
         "WHERE pyramid_level = ? AND ROWID IN ( "
	       "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	       "AND search_frame = BuildMBR(?, ?, ?, ?)", xxtiles, xtiles);
      }
      
    if (geom_cutline_blob != NULL)
    { // add cutline logic to sql (ignore all tiles that are completely outside of 'geom_cutline_blob')
     s_sql_end = sqlite3_mprintf(" AND ST_Intersects(geometry,?))");
    }
    else
     s_sql_end = sqlite3_mprintf(")");    
    s_sql_statement = sqlite3_mprintf("%s%s%s",s_sql_start,sql,s_sql_end);
    sqlite3_free (s_sql_end);
    sqlite3_free (s_sql_start);
    sqlite3_free (sql);
    sqlite3_free (xtiles);
    free (xxtiles);
    ret = sqlite3_prepare_v2 (handle, s_sql_statement, strlen (s_sql_statement), &stmt_tiles, NULL);
    sqlite3_free (s_sql_statement);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT raw tiles data(2) SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
/* preparing a raw pixels buffer */
    void_raw_buffer (bufpix, width, height, sample_type, 3, no_data);
    if (!load_triple_band_dbms_tiles
	(handle, stmt_tiles, stmt_data, bufpix, width, height, red_band,
	 green_band, blue_band, xx_res, yy_res, minx, miny, maxx, maxy, level,
	 scale, no_data,geom_cutline_blob,geom_cutline_blob_sz))
	goto error;
    sqlite3_finalize (stmt_tiles);
    sqlite3_finalize (stmt_data);
    *buffer = bufpix;
    *buf_size = bufpix_size;
    return RL2_OK;

  error:
    if (stmt_tiles != NULL)
	sqlite3_finalize (stmt_tiles);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (bufpix != NULL)
	free (bufpix);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_triple_band_raw_raster_data (sqlite3 * handle, rl2CoveragePtr cvg,
				     unsigned int width,
				     unsigned int height, double minx,
				     double miny, double maxx, double maxy,
				     double x_res, double y_res,
				     unsigned char red_band,
				     unsigned char green_band,
				     unsigned char blue_band,
				     unsigned char **buffer, int *buf_size,
				     rl2PixelPtr bgcolor,
         unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    return get_triple_band_raw_raster_data_common (0, handle, cvg, 0, width,
						   height, minx, miny, maxx,
						   maxy, x_res, y_res,
						   red_band, green_band,
						   blue_band, buffer,
						   buf_size, bgcolor,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_get_section_triple_band_raw_raster_data (sqlite3 * handle,
					     rl2CoveragePtr cvg,
					     sqlite3_int64 section_id,
					     unsigned int width,
					     unsigned int height, double minx,
					     double miny, double maxx,
					     double maxy, double x_res,
					     double y_res,
					     unsigned char red_band,
					     unsigned char green_band,
					     unsigned char blue_band,
					     unsigned char **buffer,
					     int *buf_size, rl2PixelPtr bgcolor,
          unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels - Section */
    return get_triple_band_raw_raster_data_common (1, handle, cvg, section_id,
						   width, height, minx, miny,
						   maxx, maxy, x_res, y_res,
						   red_band, green_band,
						   blue_band, buffer,
						   buf_size, bgcolor,geom_cutline_blob,geom_cutline_blob_sz);
}

static int
get_mono_band_raw_raster_data_common (int by_section, sqlite3 * handle,
				      rl2CoveragePtr cvg,
				      sqlite3_int64 section_id,
				      unsigned int width, unsigned int height,
				      double minx, double miny, double maxx,
				      double maxy, double x_res, double y_res,
				      unsigned char **buffer, int *buf_size,
				      unsigned char mono_band,
				      rl2PixelPtr bgcolor,
          unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    rl2PixelPtr no_data = NULL;
    const char *coverage;
    unsigned char level;
    unsigned char scale;
    double xx_res = x_res;
    double yy_res = y_res;
    unsigned char *bufpix = NULL;
    int bufpix_size;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    char *xtiles;
    char *xxtiles;
    char *xdata;
    char *xxdata;
    char *sql;
    char *s_sql_start;
    char *s_sql_end;
    char *s_sql_statement;
    sqlite3_stmt *stmt_tiles = NULL;
    sqlite3_stmt *stmt_data = NULL;
    int ret;
    if (cvg == NULL || handle == NULL)
	goto error;
    coverage = rl2_get_coverage_name (cvg);
    if (coverage == NULL)
	goto error;
    if (rl2_find_matching_resolution
	(handle, cvg, by_section, section_id, &xx_res, &yy_res, &level,
	 &scale) != RL2_OK)
	goto error;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	goto error;
    if (pixel_type != RL2_PIXEL_RGB && pixel_type != RL2_PIXEL_MULTIBAND)
	goto error;
    if (sample_type != RL2_SAMPLE_UINT8 && sample_type != RL2_SAMPLE_UINT16)
	goto error;
    if (mono_band >= num_bands)
	goto error;

    if (bgcolor != NULL)
      {
	  /* using the externally define background color */
	  no_data = bgcolor;
	  goto ok_no_data;
      }

  ok_no_data:
    bufpix_size = width * height;
    if (sample_type == RL2_SAMPLE_UINT16)
	bufpix_size *= 2;
    bufpix = malloc (bufpix_size);
    if (bufpix == NULL)
      {
	  fprintf (stderr,
		   "rl2_get_mono_band_raw_raster_data: Insufficient Memory !!!\n");
	  goto error;
      }
/* preparing the "tiles" SQL query */
    xtiles = sqlite3_mprintf ("%s_tiles", coverage);
    xxtiles = rl2_double_quoted_sql (xtiles);
    if (geom_cutline_blob != NULL)
    { // add cutline logic to sql (0=A portion of the tile is outside of 'geom_cutline_blob' ; 1 [default] the whole tile is inside of 'geom_cutline_blob')
     // 1 if GEOM-1[geom_cutline] completely contains GEOM-2[geom_search]
     s_sql_start = sqlite3_mprintf("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry),MbrMaxX(geometry), MbrMinY(geometry),ST_Contains(?,geometry)");
    }
    else
     s_sql_start = sqlite3_mprintf("SELECT tile_id, MbrMinX(geometry), MbrMaxY(geometry)");
    if (by_section)
      {
	  /* only from a single Section */
	  char sctn[1024];
#if defined(_WIN32) && !defined(__MINGW32__)
	  sprintf (sctn, "%I64d", section_id);
#else
	  sprintf (sctn, "%lld", section_id);
#endif
	  sql =
	      sqlite3_mprintf
	      (" FROM \"%s\" "
	       "WHERE section_id = %s AND pyramid_level = ? AND ROWID IN ( "
	       "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	       "AND search_frame = BuildMBR(?, ?, ?, ?)", xxtiles, sctn,
	       xtiles);
      }
    else
      {
	  /* whole Coverage */
	  sql =
	      sqlite3_mprintf
	      ( " FROM \"%s\" " 
         "WHERE pyramid_level = ? AND ROWID IN ( "
	       "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q "
	       "AND search_frame = BuildMBR(?, ?, ?, ?)", xxtiles, xtiles);
      }
      
    if (geom_cutline_blob != NULL)
    { // add cutline logic to sql (ignore all tiles that are completely outside of 'geom_cutline_blob')
     s_sql_end = sqlite3_mprintf(" AND ST_Intersects(geometry,?))");
    }
    else
     s_sql_end = sqlite3_mprintf(")");    
    s_sql_statement = sqlite3_mprintf("%s%s%s",s_sql_start,sql,s_sql_end);
    sqlite3_free (s_sql_end);
    sqlite3_free (s_sql_start);
    sqlite3_free (sql);
    sqlite3_free (xtiles);
    free (xxtiles);
    ret = sqlite3_prepare_v2 (handle, s_sql_statement, strlen (s_sql_statement), &stmt_tiles, NULL);
    sqlite3_free (s_sql_statement);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT raw tiles SQL error: %s\n", sqlite3_errmsg (handle));
	  goto error;
      }
    /* preparing the data SQL query - both ODD and EVEN */
    xdata = sqlite3_mprintf ("%s_tile_data", coverage);
    xxdata = rl2_double_quoted_sql (xdata);
    sqlite3_free (xdata);
    sql = sqlite3_mprintf ("SELECT tile_data_odd, tile_data_even "
			   "FROM \"%s\" WHERE tile_id = ?", xxdata);
    free (xxdata);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_data, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT raw tiles data(2) SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
/* preparing a raw pixels buffer */
    void_raw_buffer (bufpix, width, height, sample_type, 1, no_data);
    if (!load_mono_band_dbms_tiles
	(handle, stmt_tiles, stmt_data, bufpix, width, height, mono_band,
	 xx_res, yy_res, minx, miny, maxx, maxy, level, scale, no_data,geom_cutline_blob,geom_cutline_blob_sz))
	goto error;
    sqlite3_finalize (stmt_tiles);
    sqlite3_finalize (stmt_data);
    *buffer = bufpix;
    *buf_size = bufpix_size;
    return RL2_OK;

  error:
    if (stmt_tiles != NULL)
	sqlite3_finalize (stmt_tiles);
    if (stmt_data != NULL)
	sqlite3_finalize (stmt_data);
    if (bufpix != NULL)
	free (bufpix);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_mono_band_raw_raster_data (sqlite3 * handle, rl2CoveragePtr cvg,
				   unsigned int width,
				   unsigned int height, double minx,
				   double miny, double maxx, double maxy,
				   double x_res, double y_res,
				   unsigned char mono_band,
				   unsigned char **buffer, int *buf_size,
				   rl2PixelPtr no_data,
       unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    return get_mono_band_raw_raster_data_common (0, handle, cvg, 0, width,
						 height, minx, miny, maxx,
						 maxy, x_res, y_res, buffer,
						 buf_size, mono_band, no_data,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_get_section_mono_band_raw_raster_data (sqlite3 * handle,
					   rl2CoveragePtr cvg,
					   sqlite3_int64 section_id,
					   unsigned int width,
					   unsigned int height, double minx,
					   double miny, double maxx,
					   double maxy, double x_res,
					   double y_res,
					   unsigned char mono_band,
					   unsigned char **buffer,
					   int *buf_size, rl2PixelPtr no_data,
        unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage */
    return get_mono_band_raw_raster_data_common (1, handle, cvg, section_id,
						 width, height, minx, miny,
						 maxx, maxy, x_res, y_res,
						 buffer, buf_size, mono_band,
						 no_data,geom_cutline_blob,geom_cutline_blob_sz);
}

RL2_DECLARE int
rl2_get_raw_raster_data_bgcolor (sqlite3 * handle, int max_threads,
				 rl2CoveragePtr cvg, unsigned int width,
				 unsigned int height, double minx,
				 double miny, double maxx, double maxy,
				 double x_res, double y_res,
				 unsigned char **buffer, int *buf_size,
				 rl2PalettePtr * palette,
				 unsigned char *out_pixel,
				 unsigned char bg_red, unsigned char bg_green,
				 unsigned char bg_blue,
				 rl2RasterSymbolizerPtr style,
				 rl2RasterStatisticsPtr stats,
     unsigned char *geom_cutline_blob, int *geom_cutline_blob_sz)
{
/* attempting to return a buffer containing raw pixels from the DBMS Coverage + bgcolor */
    int ret;
    rl2PixelPtr no_data = NULL;
    const char *coverage;
    unsigned char sample_type;
    unsigned char pixel_type;
    unsigned char num_bands;
    rl2RasterSymbolizerPtr xstyle = style;

    if (cvg == NULL || handle == NULL)
	return RL2_ERROR;
    if (rl2_get_coverage_type (cvg, &sample_type, &pixel_type, &num_bands) !=
	RL2_OK)
	return RL2_ERROR;
    coverage = rl2_get_coverage_name (cvg);
    if (coverage == NULL)
	return RL2_ERROR;

    if (pixel_type == RL2_PIXEL_MONOCHROME && *out_pixel == RL2_PIXEL_GRAYSCALE)
      {
	  /* Pyramid tiles MONOCHROME - Grayscale pixel */
	  no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, bg_red);
      }
    else if (pixel_type == RL2_PIXEL_PALETTE && *out_pixel == RL2_PIXEL_RGB)
      {
	  /* Pyramid tiles PALETTE - RGB pixel */
	  no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, bg_red);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, bg_green);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, bg_blue);
      }
    else if (pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  /* Monochrome */
	  no_data =
	      rl2_create_pixel (RL2_SAMPLE_1_BIT, RL2_PIXEL_MONOCHROME, 1);
	  if (bg_red > 128)
	      rl2_set_pixel_sample_1bit (no_data, 0);
	  else
	      rl2_set_pixel_sample_1bit (no_data, 1);
      }
    else if (pixel_type == RL2_PIXEL_PALETTE)
      {
	  /* Palette */
	  int index = -1;
	  rl2PalettePtr palette = rl2_get_dbms_palette (handle, coverage);
	  if (palette != NULL)
	    {
		/* searching the background color from within the palette */
		int i;
		unsigned short num_entries;
		unsigned char *red = NULL;
		unsigned char *green = NULL;
		unsigned char *blue = NULL;
		if (rl2_get_palette_colors
		    (palette, &num_entries, &red, &green, &blue) == RL2_OK)
		  {
		      for (i = 0; i < num_entries; i++)
			{
			    if (red[i] == bg_red && green[i] == bg_green
				&& blue[i] == bg_blue)
			      {
				  index = i;
				  break;
			      }
			}
		      free (red);
		      free (green);
		      free (blue);
		      rl2_destroy_palette (palette);
		  }
	    }
	  if (index < 0)
	    {
		/* palette color found */
		switch (sample_type)
		  {
		  case RL2_SAMPLE_1_BIT:
		      no_data =
			  rl2_create_pixel (RL2_SAMPLE_1_BIT,
					    RL2_PIXEL_PALETTE, 1);
		      rl2_set_pixel_sample_1bit (no_data,
						 (unsigned char) index);
		      break;
		  case RL2_SAMPLE_2_BIT:
		      no_data =
			  rl2_create_pixel (RL2_SAMPLE_2_BIT,
					    RL2_PIXEL_PALETTE, 1);
		      rl2_set_pixel_sample_2bit (no_data,
						 (unsigned char) index);
		      break;
		  case RL2_SAMPLE_4_BIT:
		      no_data =
			  rl2_create_pixel (RL2_SAMPLE_4_BIT,
					    RL2_PIXEL_PALETTE, 1);
		      rl2_set_pixel_sample_4bit (no_data,
						 (unsigned char) index);
		      break;
		  case RL2_SAMPLE_UINT8:
		      no_data =
			  rl2_create_pixel (RL2_SAMPLE_UINT8,
					    RL2_PIXEL_PALETTE, 1);
		      rl2_set_pixel_sample_uint8 (no_data, RL2_PALETTE_BAND,
						  (unsigned char) index);
		      break;

		  };
	    }
      }
    else if (pixel_type == RL2_PIXEL_GRAYSCALE)
      {
	  /* Grayscale */
	  if (sample_type == RL2_SAMPLE_UINT8)
	    {
		/* 256 levels grayscale */
		no_data =
		    rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
		rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND,
					    bg_red);
	    }
	  else if (sample_type == RL2_SAMPLE_1_BIT)
	    {
		/* 2 levels grayscale */
		no_data =
		    rl2_create_pixel (RL2_SAMPLE_1_BIT, RL2_PIXEL_GRAYSCALE, 1);
		if (bg_red >= 128)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 1);
		else
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 0);
	    }
	  else if (sample_type == RL2_SAMPLE_2_BIT)
	    {
		/* 4 levels grayscale */
		no_data =
		    rl2_create_pixel (RL2_SAMPLE_1_BIT, RL2_PIXEL_GRAYSCALE, 1);
		if (bg_red >= 192)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 3);
		else if (bg_red >= 128)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 2);
		else if (bg_red >= 64)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 1);
		else
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 0);
	    }
	  else if (sample_type == RL2_SAMPLE_4_BIT)
	    {
		/* 16 levels grayscale */
		no_data =
		    rl2_create_pixel (RL2_SAMPLE_1_BIT, RL2_PIXEL_GRAYSCALE, 1);
		if (bg_red >= 240)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND,
						15);
		else if (bg_red >= 224)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND,
						14);
		else if (bg_red >= 208)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND,
						13);
		else if (bg_red >= 192)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND,
						12);
		else if (bg_red >= 176)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND,
						11);
		else if (bg_red >= 160)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND,
						10);
		else if (bg_red >= 144)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 9);
		else if (bg_red >= 128)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 8);
		else if (bg_red >= 112)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 7);
		else if (bg_red >= 96)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 6);
		else if (bg_red >= 80)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 5);
		else if (bg_red >= 64)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 4);
		else if (bg_red >= 48)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 3);
		else if (bg_red >= 32)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 2);
		else if (bg_red >= 16)
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 1);
		else
		    rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND, 0);
	    }
      }
    else if (pixel_type == RL2_PIXEL_RGB)
      {
	  /* RGB */
	  no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, bg_red);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, bg_green);
	  rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, bg_blue);
      }
    if (no_data == NULL)
      {
	  unsigned char pixel = *out_pixel;
	  if (pixel == RL2_PIXEL_GRAYSCALE && pixel_type == RL2_PIXEL_DATAGRID)
	    {
		if (rl2_has_styled_rgb_colors (style))
		  {
		      /* RGB RasterSymbolizer: promoting to RGB */
		      pixel = RL2_PIXEL_RGB;
		  }
	    }
	  if (pixel == RL2_PIXEL_GRAYSCALE)
	    {
		/* output Grayscale pixel */
		no_data =
		    rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_GRAYSCALE, 1);
		rl2_set_pixel_sample_uint8 (no_data, RL2_GRAYSCALE_BAND,
					    bg_red);
	    }
	  if (pixel == RL2_PIXEL_RGB)
	    {
		/* output RGB pixel */
		no_data = rl2_create_pixel (RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3);
		rl2_set_pixel_sample_uint8 (no_data, RL2_RED_BAND, bg_red);
		rl2_set_pixel_sample_uint8 (no_data, RL2_GREEN_BAND, bg_green);
		rl2_set_pixel_sample_uint8 (no_data, RL2_BLUE_BAND, bg_blue);
	    }
      }
    if (pixel_type == RL2_PIXEL_MONOCHROME)
	xstyle = NULL;
    ret =
	rl2_get_raw_raster_data_common (handle, max_threads, cvg, 0, 0, width,
					height, minx, miny, maxx, maxy, x_res,
					y_res, buffer, buf_size, palette,
					*out_pixel, no_data, xstyle, stats,geom_cutline_blob,geom_cutline_blob_sz);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (*out_pixel == RL2_PIXEL_GRAYSCALE && pixel_type == RL2_PIXEL_DATAGRID)
      {
	  if (rl2_has_styled_rgb_colors (style))
	    {
		/* RGB RasterSymbolizer: promoting to RGB */
		*out_pixel = RL2_PIXEL_RGB;
	    }
      }
    if (pixel_type == RL2_PIXEL_MONOCHROME)
      {
	  unsigned char red = 0;
	  unsigned char green = 0;
	  unsigned char blue = 0;
	  int ok = 0;
	  if (style != NULL)
	    {
		rl2PrivRasterSymbolizerPtr stl =
		    (rl2PrivRasterSymbolizerPtr) style;
		if (stl->categorize != NULL)
		  {
		      rl2PrivColorMapPointPtr color = stl->categorize->first;
		      while (color != NULL)
			{
			    if (color->value == 1.0)
			      {
				  ok = 1;
				  red = color->red;
				  green = color->green;
				  blue = color->blue;
				  break;
			      }
			    color = color->next;
			}
		  }
	    }
	  if (*out_pixel != RL2_PIXEL_MONOCHROME)
	    {
		int i;
		unsigned char *p = *buffer;
		for (i = 0; i < *buf_size; i++)
		  {
		      if (*p > 224)
			  *p = 255;
		      p++;
		  }
		if (ok == 1)
		    ok = 2;
	    }
	  if (ok == 1)
	    {
		/* creating a Palette 0/1 */
		rl2PalettePtr plt = rl2_create_palette (2);
		rl2_set_palette_color (plt, 0, 255, 255, 255);
		rl2_set_palette_color (plt, 1, red, green, blue);
		*palette = plt;
		*out_pixel = RL2_PIXEL_PALETTE;
	    }
	  if (ok == 2)
	    {
		/* creating a Palette 0/255 */
		int i;
		double dr = 255.0 - red;
		double dg = 255.0 - green;
		double db = 255.0 - blue;
		rl2PalettePtr plt = rl2_create_palette (256);
		for (i = 0; i < 256; i++)
		  {
		      double scale = 255.0 / (double) i;
		      double r = (double) red + (dr / scale);
		      double g = (double) green + (dg / scale);
		      double b = (double) blue + (db / scale);
		      if (r < 0.0)
			  r = 0.0;
		      if (r > 255.0)
			  r = 255.0;
		      if (g < 0.0)
			  g = 0.0;
		      if (g > 255.0)
			  g = 255.0;
		      if (b < 0.0)
			  b = 0.0;
		      if (b > 255.0)
			  b = 255.0;
		      rl2_set_palette_color (plt, i, (unsigned char) r,
					     (unsigned char) g,
					     (unsigned char) b);
		  }
		*palette = plt;
		*out_pixel = RL2_PIXEL_PALETTE;
	    }
      }

    return ret;
}

RL2_DECLARE rl2PalettePtr
rl2_get_dbms_palette (sqlite3 * handle, const char *coverage)
{
/* attempting to retrieve a Coverage's Palette from the DBMS */
    rl2PalettePtr palette = NULL;
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    if (handle == NULL || coverage == NULL)
	return NULL;

    sql = sqlite3_mprintf ("SELECT palette FROM raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      palette = rl2_deserialize_dbms_palette (blob, blob_sz);
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }

    if (palette == NULL)
	goto error;
    sqlite3_finalize (stmt);
    return palette;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

RL2_DECLARE int
rl2_update_dbms_palette (sqlite3 * handle, const char *coverage,
			 rl2PalettePtr palette)
{
/* attempting to update a Coverage's Palette into the DBMS */
    unsigned char sample_type = RL2_SAMPLE_UNKNOWN;
    unsigned char pixel_type = RL2_PIXEL_UNKNOWN;
    unsigned short num_entries;
    unsigned char *blob;
    int blob_size;
    sqlite3_stmt *stmt = NULL;
    char *sql;
    int ret;
    if (handle == NULL || coverage == NULL || palette == NULL)
	return RL2_ERROR;

    sql =
	sqlite3_mprintf
	("SELECT sample_type, pixel_type FROM raster_coverages "
	 "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *sample =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *pixel =
		    (const char *) sqlite3_column_text (stmt, 1);
		if (strcmp (sample, "1-BIT") == 0)
		    sample_type = RL2_SAMPLE_1_BIT;
		if (strcmp (sample, "2-BIT") == 0)
		    sample_type = RL2_SAMPLE_2_BIT;
		if (strcmp (sample, "4-BIT") == 0)
		    sample_type = RL2_SAMPLE_4_BIT;
		if (strcmp (sample, "UINT8") == 0)
		    sample_type = RL2_SAMPLE_UINT8;
		if (strcmp (pixel, "PALETTE") == 0)
		    pixel_type = RL2_PIXEL_PALETTE;
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* testing for self-consistency */
    if (pixel_type != RL2_PIXEL_PALETTE)
	goto error;
    if (rl2_get_palette_entries (palette, &num_entries) != RL2_OK)
	goto error;
    switch (sample_type)
      {
      case RL2_SAMPLE_UINT8:
	  if (num_entries > 256)
	      goto error;
	  break;
      case RL2_SAMPLE_1_BIT:
	  if (num_entries > 2)
	      goto error;
	  break;
      case RL2_SAMPLE_2_BIT:
	  if (num_entries > 4)
	      goto error;
	  break;
      case RL2_SAMPLE_4_BIT:
	  if (num_entries > 16)
	      goto error;
	  break;
      default:
	  goto error;
      };

    if (rl2_serialize_dbms_palette (palette, &blob, &blob_size) != RL2_OK)
	goto error;
    sql = sqlite3_mprintf ("UPDATE raster_coverages SET palette = ? "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, free);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  sqlite3_finalize (stmt);
	  return RL2_OK;
      }
    fprintf (stderr,
	     "sqlite3_step() error: UPDATE raster_coverages \"%s\"\n",
	     sqlite3_errmsg (handle));

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

static void
set_remapped_palette (rl2PrivTiffOriginPtr origin, rl2PalettePtr palette)
{
/* installing a remapped Palette into the TIFF origin */
    int j;
    rl2PrivPaletteEntryPtr entry;
    rl2PrivPalettePtr plt = (rl2PrivPalettePtr) palette;

    if (plt->nEntries != origin->remapMaxPalette)
      {
	  /* reallocating the remapped palette */
	  if (origin->remapRed != NULL)
	      free (origin->remapRed);
	  if (origin->remapGreen != NULL)
	      free (origin->remapGreen);
	  if (origin->remapBlue != NULL)
	      free (origin->remapBlue);
	  origin->remapMaxPalette = plt->nEntries;
	  origin->remapRed = malloc (origin->remapMaxPalette);
	  origin->remapGreen = malloc (origin->remapMaxPalette);
	  origin->remapBlue = malloc (origin->remapMaxPalette);
      }
    for (j = 0; j < plt->nEntries; j++)
      {
	  entry = plt->entries + j;
	  origin->remapRed[j] = entry->red;
	  origin->remapGreen[j] = entry->green;
	  origin->remapBlue[j] = entry->blue;
      }
}

RL2_DECLARE int
rl2_check_dbms_palette (sqlite3 * handle, rl2CoveragePtr coverage,
			rl2TiffOriginPtr tiff)
{
/*attempting to merge/update a Coverage's Palette */
    int i;
    int j;
    int changed = 0;
    int maxPalette = 0;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
    int ok;
    rl2PalettePtr palette = NULL;
    rl2PrivPaletteEntryPtr entry;
    rl2PrivPalettePtr plt;
    rl2PrivCoveragePtr cvg = (rl2PrivCoveragePtr) coverage;
    rl2PrivTiffOriginPtr origin = (rl2PrivTiffOriginPtr) tiff;
    if (cvg == NULL || origin == NULL)
	return RL2_ERROR;
    palette = rl2_get_dbms_palette (handle, cvg->coverageName);
    if (palette == NULL)
	goto error;
    plt = (rl2PrivPalettePtr) palette;
    for (j = 0; j < plt->nEntries; j++)
      {
	  entry = plt->entries + j;
	  ok = 0;
	  for (i = 0; i < maxPalette; i++)
	    {
		if (red[i] == entry->red && green[i] == entry->green
		    && blue[i] == entry->blue)
		  {
		      ok = 1;
		      break;
		  }
	    }
	  if (ok)
	      continue;
	  if (maxPalette == 256)
	      goto error;
	  red[maxPalette] = entry->red;
	  green[maxPalette] = entry->green;
	  blue[maxPalette] = entry->blue;
	  maxPalette++;
      }

    for (i = 0; i < origin->maxPalette; i++)
      {
	  /* checking TIFF palette entries */
	  unsigned char tiff_red = origin->red[i];
	  unsigned char tiff_green = origin->green[i];
	  unsigned char tiff_blue = origin->blue[i];
	  ok = 0;
	  for (j = 0; j < maxPalette; j++)
	    {
		if (tiff_red == red[j] && tiff_green == green[j]
		    && tiff_blue == blue[j])
		  {
		      /* found a matching color */
		      ok = 1;
		      break;
		  }
	    }
	  if (!ok)
	    {
		/* attempting to insert a new color into the pseudo-Palette */
		if (maxPalette == 256)
		    goto error;
		red[maxPalette] = tiff_red;
		green[maxPalette] = tiff_green;
		blue[maxPalette] = tiff_blue;
		maxPalette++;
		changed = 1;
	    }
      }
    if (changed)
      {
	  /* updating the DBMS Palette */
	  rl2PalettePtr plt2 = rl2_create_palette (maxPalette);
	  if (plt2 == NULL)
	      goto error;
	  rl2_destroy_palette (palette);
	  palette = plt2;
	  for (j = 0; j < maxPalette; j++)
	      rl2_set_palette_color (palette, j, red[j], green[j], blue[j]);
	  if (rl2_update_dbms_palette (handle, cvg->coverageName, palette) !=
	      RL2_OK)
	      goto error;
      }
    set_remapped_palette (origin, palette);
    rl2_destroy_palette (palette);
    return RL2_OK;

  error:
    if (palette != NULL)
	rl2_destroy_palette (palette);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_update_dbms_coverage (sqlite3 * handle, const char *coverage)
{
/*attempting to update a Coverage (statistics and extent) */
    int ret;
    char *sql;
    char *xtable;
    char *xxtable;
    unsigned char *blob_stats;
    int blob_stats_sz;
    int first;
    unsigned char zoom_min=0;
    unsigned char zoom_default=0;
    unsigned char zoom_max=0;
    unsigned char zoom_min_extent=0;
    unsigned char zoom_default_extent=0;
    unsigned char zoom_max_extent=0;
    rl2RasterStatisticsPtr coverage_stats = NULL;
    rl2PrivRasterDefaultsPtr raster_defaults = NULL;
    double center_x_extent=0.0;
    double center_y_extent=0.0;
    int coverage_srid=0;
    sqlite3_stmt *stmt_ext_in = NULL;
    sqlite3_stmt *stmt_ext_out = NULL;
    sqlite3_stmt *stmt_stats_in = NULL;
    sqlite3_stmt *stmt_stats_out = NULL;
    
/* Extent query stmt */
    xtable = sqlite3_mprintf ("%s_sections", coverage);
    xxtable = rl2_double_quoted_sql (xtable);
    sqlite3_free (xtable);
    sql =
	sqlite3_mprintf
	("SELECT Min(MbrMinX(geometry)), Min(MbrMinY(geometry)), "
	 "Max(MbrMaxX(geometry)), Max(MbrMaxY(geometry)) " "FROM \"%s\"",
	 xxtable);
    free (xxtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_ext_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT Coverage extent SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
/* Extent update stmt */
    sql = sqlite3_mprintf ("UPDATE raster_coverages SET extent_minx = ?, "
			   "extent_miny = ?, extent_maxx = ?, extent_maxy = ? "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_ext_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE Coverage extent SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
    }
    while (1)
      {
	  /* querying the extent */
	  ret = sqlite3_step (stmt_ext_in);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		double minx = sqlite3_column_double (stmt_ext_in, 0);
		double miny = sqlite3_column_double (stmt_ext_in, 1);
		double maxx = sqlite3_column_double (stmt_ext_in, 2);
		double maxy = sqlite3_column_double (stmt_ext_in, 3);
  center_x_extent=(maxx+minx)/2;
  center_y_extent=(maxy+miny)/2;

		/* updating the extent */
		sqlite3_reset (stmt_ext_out);
		sqlite3_clear_bindings (stmt_ext_out);
		sqlite3_bind_double (stmt_ext_out, 1, minx);
		sqlite3_bind_double (stmt_ext_out, 2, miny);
		sqlite3_bind_double (stmt_ext_out, 3, maxx);
		sqlite3_bind_double (stmt_ext_out, 4, maxy);
		ret = sqlite3_step (stmt_ext_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    break;
		else
		  {
		      fprintf (stderr,
			       "UPDATE Coverage Extent sqlite3_step() error: %s\n",
			       sqlite3_errmsg (handle));
		      goto error;
		  }
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT Coverage Extent sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_ext_in);
    sqlite3_finalize (stmt_ext_out);
    stmt_ext_in = NULL;
    stmt_ext_out = NULL;

/* Raster Statistics query stmt */
    xtable = sqlite3_mprintf ("%s_sections", coverage);
    xxtable = rl2_double_quoted_sql (xtable);
    sqlite3_free (xtable);
    sql = sqlite3_mprintf ("SELECT statistics, ST_SRID(geometry) FROM \"%s\"", xxtable);
    free (xxtable);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT Coverage Statistics SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }
/* Raster Statistics update stmt */
    sql = sqlite3_mprintf ("UPDATE raster_coverages SET statistics = ? "
			   "WHERE Lower(coverage_name) = Lower(%Q)", coverage);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_stats_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("UPDATE Coverage Statistics SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    first = 1;
    while (1)
      {
	  /* querying the statistics */
	  ret = sqlite3_step (stmt_stats_in);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		rl2RasterStatisticsPtr stats;
		blob_stats =
		    (unsigned char *) sqlite3_column_blob (stmt_stats_in, 0);
		blob_stats_sz = sqlite3_column_bytes (stmt_stats_in, 0);
		stats =
		    rl2_deserialize_dbms_raster_statistics (blob_stats,
							    blob_stats_sz);
		if (stats == NULL)
		    goto error;
  coverage_srid = sqlite3_column_int (stmt_stats_in, 1);

		if (first)
		  {
		      double no_data;
		      double count;
		      unsigned char sample_type;
		      unsigned char num_bands;        
		      if (rl2_get_raster_statistics_summary
			  (stats, &no_data, &count, &sample_type,
			   &num_bands) != RL2_OK)
			  goto error;
		      coverage_stats =
			  rl2_create_raster_statistics (sample_type, num_bands);
		      if (coverage_stats == NULL)
			  goto error;
      if (rl2_get_defaults_stats(handle, coverage_stats,coverage,coverage_srid,center_x_extent,center_y_extent))
       goto error;
		      first = 0;
		  }

		rl2_aggregate_raster_statistics (stats, coverage_stats);
		rl2_destroy_raster_statistics (stats);
	    }
	  else
	    {
		fprintf (stderr,
			 "SELECT Coverage Statistics sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    if (coverage_stats == NULL)
	goto error;
   // here uodate defaults;
    /* updating the statistics */
    compute_aggregate_sq_diff (coverage_stats);
    sqlite3_reset (stmt_stats_out);
    sqlite3_clear_bindings (stmt_stats_out);    
    rl2_serialize_dbms_raster_statistics (coverage_stats, &blob_stats,
					  &blob_stats_sz);
    sqlite3_bind_blob (stmt_stats_out, 1, blob_stats, blob_stats_sz, free);
    ret = sqlite3_step (stmt_stats_out);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  fprintf (stderr,
		   "UPDATE Coverage Statistics sqlite3_step() error: %s\n",
		   sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_finalize (stmt_stats_in);
    sqlite3_finalize (stmt_stats_out);
    rl2_destroy_raster_statistics (coverage_stats);
    return RL2_OK;

  error:
    if (stmt_ext_in != NULL)
	sqlite3_finalize (stmt_ext_in);
    if (stmt_ext_out != NULL)
	sqlite3_finalize (stmt_ext_out);
    if (stmt_stats_in != NULL)
	sqlite3_finalize (stmt_stats_in);
    if (stmt_stats_out != NULL)
	sqlite3_finalize (stmt_stats_out);
    if (coverage_stats != NULL)
	rl2_destroy_raster_statistics (coverage_stats);
    return RL2_ERROR;
}

RL2_DECLARE rl2CoverageStylePtr
rl2_create_coverage_style_from_dbms (sqlite3 * handle, const char *coverage,
				     const char *style)
{
/* attempting to load and parse a Coverage Style */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2CoverageStylePtr stl = NULL;
    char *name = NULL;
    unsigned char *xml = NULL;
    int done = 0;

    sql = "SELECT s.style_name, XB_GetDocument(s.style) "
	"FROM SE_raster_styled_layers AS r "
	"JOIN SE_raster_styles AS s ON (r.style_id = s.style_id) "
	"WHERE Lower(r.coverage_name) = Lower(?) AND "
	"Lower(s.style_name) LIKE Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, style, strlen (style), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int len;
		const char *str;
		const unsigned char *ustr;
		if (!done)
		  {
		      /* reteiving just the first reference in case of duplicates */
		      done = 1;
		      if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
			{
			    str = (const char *) sqlite3_column_text (stmt, 0);
			    len = strlen (str);
			    name = malloc (len + 1);
			    strcpy (name, str);
			}
		      if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
			{
			    ustr = sqlite3_column_text (stmt, 1);
			    len = strlen ((const char *) ustr);
			    xml = malloc (len + 1);
			    strcpy ((char *) xml, (const char *) ustr);
			}
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

    if (name == NULL || xml == NULL)
      {
	  if (name != NULL)
	      free (name);
	  if (xml != NULL)
	      free (xml);
	  goto error;
      }
    stl = coverage_style_from_xml (name, xml);
    if (stl == NULL)
	goto error;
    return stl;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stl != NULL)
	rl2_destroy_coverage_style (stl);
    return NULL;
}

RL2_DECLARE rl2FeatureTypeStylePtr
rl2_create_feature_type_style_from_dbms (sqlite3 * handle,
					 const char *coverage,
					 const char *style)
{
/* attempting to load and parse a Feature Type Style */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2FeatureTypeStylePtr stl = NULL;
    char *name = NULL;
    unsigned char *xml = NULL;
    int done = 0;

    sql = "SELECT s.style_name, XB_GetDocument(s.style) "
	"FROM SE_vector_styled_layers AS v "
	"JOIN SE_vector_styles AS s ON (v.style_id = s.style_id) "
	"WHERE Lower(v.coverage_name) = Lower(?) "
	"AND Lower(s.style_name) LIKE Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, style, strlen (style), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int len;
		const char *str;
		const unsigned char *ustr;
		if (!done)
		  {
		      /* reteiving just the first reference in case of duplicates */
		      done = 1;
		      if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
			{
			    str = (const char *) sqlite3_column_text (stmt, 0);
			    len = strlen (str);
			    name = malloc (len + 1);
			    strcpy (name, str);
			}
		      if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
			{
			    ustr = sqlite3_column_text (stmt, 1);
			    len = strlen ((const char *) ustr);
			    xml = malloc (len + 1);
			    strcpy ((char *) xml, (const char *) ustr);
			}
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

    if (name == NULL || xml == NULL)
      {
	  if (name != NULL)
	      free (name);
	  if (xml != NULL)
	      free (xml);
	  goto error;
      }
    stl = feature_type_style_from_xml (name, xml);
    if (stl == NULL)
	goto error;
    return stl;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stl != NULL)
	rl2_destroy_feature_type_style (stl);
    return NULL;
}

static int
test_named_layer (sqlite3 * handle, const char *groupName,
		  const char *namedLayer)
{
/* testing a Named Layer for validity */
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    int ok = 0;
/* testing if the Raster Coverage exists */
    char *sql = sqlite3_mprintf ("SELECT coverage_name FROM raster_coverages "
				 "WHERE Lower(coverage_name) = Lower(%Q)",
				 namedLayer);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	ok = 1;
    sqlite3_free_table (results);
    if (!ok)
	return 0;

    ok = 0;
/* testing if the Raster Coverage belong to the Layer Group */
    sql =
	sqlite3_mprintf
	("SELECT raster_coverage_name FROM SE_styled_group_refs "
	 "WHERE Lower(group_name) = Lower(%Q) AND "
	 "Lower(raster_coverage_name) = Lower(%Q)", groupName, namedLayer);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	ok = 1;
    sqlite3_free_table (results);
    return ok;
}

static int
test_named_style (sqlite3 * handle, const char *namedLayer,
		  const char *namedStyle)
{
/* testing a Named Style for validity */
    int ret;
    char **results;
    int rows;
    int columns;
    int i;
    int ok = 0;
/* testing if the Layer Style exists */
    char *sql =
	sqlite3_mprintf
	("SELECT style_name FROM SE_raster_styled_layers AS r "
	 "JOIN SE_raster_styles AS s ON (r.style_id = s.style_id) "
	 "WHERE Lower(r.coverage_name) = Lower(%Q) AND "
	 "Lower(s.style_name) = Lower(%Q)", namedLayer,
	 namedStyle);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	ok = 1;
    sqlite3_free_table (results);
    return ok;
}

RL2_DECLARE rl2GroupStylePtr
rl2_create_group_style_from_dbms (sqlite3 * handle, const char *group,
				  const char *style)
{
/* attempting to load and parse a Layer Group style */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2GroupStylePtr stl = NULL;
    char *name = NULL;
    unsigned char *xml = NULL;
    rl2PrivGroupStylePtr grp_stl;
    rl2PrivChildStylePtr child;
    int done = 0;

    sql = "SELECT s.style_name, XB_GetDocument(s.style) "
	"FROM SE_styled_group_styles AS g "
	"JOIN SE_group_styles AS s ON (g.style_id = s.style_id) "
	"WHERE Lower(g.group_name) = Lower(?) AND Lower(s.style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group, strlen (group), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, style, strlen (style), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int len;
		const char *str;
		const unsigned char *ustr;
		if (!done)
		  {
		      /* retrieving just the first reference in case of duplicates */
		      done = 1;
		      if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
			{
			    str = (const char *) sqlite3_column_text (stmt, 0);
			    len = strlen (str);
			    name = malloc (len + 1);
			    strcpy (name, str);
			}
		      if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
			{
			    ustr = sqlite3_column_text (stmt, 1);
			    len = strlen ((const char *) ustr);
			    xml = malloc (len + 1);
			    strcpy ((char *) xml, (const char *) ustr);
			}
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

    if (name == NULL || xml == NULL)
      {
	  if (name != NULL)
	      free (name);
	  if (xml != NULL)
	      free (xml);
	  goto error;
      }

/* final validation */
    stl = group_style_from_sld_xml (name, xml);
    if (stl == NULL)
	goto error;
    grp_stl = (rl2PrivGroupStylePtr) stl;
    child = grp_stl->first;
    while (child != NULL)
      {
	  /* testing NamedLayers and NamedStyles */
	  if (child->namedLayer != NULL)
	    {
		if (test_named_layer (handle, group, child->namedLayer))
		    child->validLayer = 1;
	    }
	  if (child->validLayer == 1)
	    {
		if (child->namedStyle != NULL)
		  {
		      if (strcmp (child->namedStyle, "default") == 0)
			  child->validStyle = 1;
		      else if (test_named_style
			       (handle, child->namedLayer, child->namedStyle))
			  child->validStyle = 1;
		  }
		else
		    child->validStyle = 1;
	    }
	  child = child->next;
      }
    grp_stl->valid = 1;
    child = grp_stl->first;
    while (child != NULL)
      {
	  if (child->validLayer == 0 || child->validStyle == 0)
	      grp_stl->valid = 0;
	  child = child->next;
      }

    return stl;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stl != NULL)
	rl2_destroy_group_style (stl);
    return NULL;
}

RL2_DECLARE rl2RasterStatisticsPtr
rl2_create_raster_statistics_from_dbms (sqlite3 * handle, const char *coverage)
{
/* attempting to load a Covrage's RasterStatistics object */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2RasterStatisticsPtr stats = NULL;

    sql = "SELECT statistics FROM raster_coverages "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (handle));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      stats =
			  rl2_deserialize_dbms_raster_statistics (blob,
								  blob_sz);
		  }
	    }
	  else
	    {
		fprintf (stderr, "SQL error: %s\n%s\n", sql,
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return stats;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}


RL2_DECLARE int
rl2_check_raster_coverage_destination (sqlite3 * sqlite,
				       const char *coverage_name)
{
/* checking if a Raster Coverage does not exist on Main */
    int ret;
    char *xcoverage;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int count = 0;

    sql = "SELECT count(*) FROM main.raster_coverages "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage destination SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

/* querying the raster_covrages meta-table */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage destination sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 0)
	return RL2_ERROR;

/* checking for the coverage-LEVELS table */
    xcoverage = sqlite3_mprintf ("%s_levels", coverage_name);
    sql = "SELECT count(*) FROM main.sqlite_master "
	"WHERE Lower(tbl_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage destination SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xcoverage, strlen (xcoverage), sqlite3_free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage destination sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 0)
	return RL2_ERROR;

/* checking for the coverage-SECTIONS table */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage_name);
    sql = "SELECT count(*) FROM main.sqlite_master "
	"WHERE Lower(tbl_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage destination SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xcoverage, strlen (xcoverage), sqlite3_free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage destination sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 0)
	return RL2_ERROR;

/* checking for the coverage-TILE_DATA table */
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage_name);
    sql = "SELECT count(*) FROM main.sqlite_master "
	"WHERE Lower(tbl_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage destination SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xcoverage, strlen (xcoverage), sqlite3_free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage destination sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 0)
	return RL2_ERROR;

/* checking for the coverage-TILES table */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage_name);
    sql = "SELECT count(*) FROM main.sqlite_master "
	"WHERE Lower(tbl_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage destination SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xcoverage, strlen (xcoverage), sqlite3_free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage destination sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 0)
	return RL2_ERROR;

    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_check_raster_coverage_origin (sqlite3 * sqlite,
				  const char *db_prefix,
				  const char *coverage_name)
{
/* checking if a Raster Coverage does really exist on some attached DB */
    int ret;
    char *xcoverage;
    char *xdb = NULL;
    sqlite3_stmt *stmt = NULL;
    char *sql;
    int count = 0;

    xdb = rl2_double_quoted_sql (db_prefix);
    sql = sqlite3_mprintf ("SELECT count(*) FROM \"%s\".raster_coverages "
			   "WHERE Lower(coverage_name) = Lower(?)", xdb);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage origin SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }

/* querying the raster_covrages meta-table */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage origin sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 1)
	goto error;

/* checking for the coverage-LEVELS table */
    xcoverage = sqlite3_mprintf ("%s_levels", coverage_name);
    sql = sqlite3_mprintf ("SELECT count(*) FROM \"%s\".sqlite_master "
			   "WHERE Lower(tbl_name) = Lower(?) AND type = 'table'",
			   xdb);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage origin SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xcoverage, strlen (xcoverage), sqlite3_free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage origin sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 1)
	goto error;

/* checking for the coverage-SECTIONS table */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage_name);
    sql = sqlite3_mprintf ("SELECT count(*) FROM \"%s\".sqlite_master "
			   "WHERE Lower(tbl_name) = Lower(?) AND type = 'table'",
			   xdb);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage origin SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xcoverage, strlen (xcoverage), sqlite3_free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage origin sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 1)
	goto error;

/* checking for the coverage-TILE_DATA table */
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage_name);
    sql = sqlite3_mprintf ("SELECT count(*) FROM \"%s\".sqlite_master "
			   "WHERE Lower(tbl_name) = Lower(?) AND type = 'table'",
			   xdb);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage origin SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xcoverage, strlen (xcoverage), sqlite3_free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage origin sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 1)
	goto error;

/* checking for the coverage-TILES table */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage_name);
    sql = sqlite3_mprintf ("SELECT count(*) FROM \"%s\".sqlite_master "
			   "WHERE Lower(tbl_name) = Lower(?) AND type = 'table'",
			   xdb);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT check Raster Coverage origin SQL error: %s\n",
		  sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xcoverage, strlen (xcoverage), sqlite3_free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT check Raster Coverage origin sqlite3_step() error: %s\n",
			 sqlite3_errmsg (sqlite));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (count != 1)
	goto error;

    free (xdb);
    return RL2_OK;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (xdb != NULL)
	free (xdb);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_copy_raster_coverage (sqlite3 * sqlite, const char *db_prefix,
			  const char *coverage_name)
{
/* copying a Raster Coverages from an attached DB into Main */
    int ret;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    char *xdb = NULL;
    char *xcoverage;
    char *xxcoverage;
    const char *sample_type;
    const char *pixel_type;
    int num_bands;
    const char *compression;
    int quality;
    int tile_width;
    int tile_height;
    int srid;
    double horz_res;
    double vert_res;
    unsigned char sample;
    unsigned char pixel;
    unsigned char compr;
    int strict_resolution = 0;
    int mixed_resolutions = 0;
    int section_paths = 0;
    int section_md5 = 0;
    int section_summary = 0;
    rl2PixelPtr no_data = NULL;
    rl2PalettePtr palette = NULL;
    char *title = NULL;
    char *abstract = NULL;
    unsigned char *statistics = NULL;
    int statistics_sz;
    int ok_geo = 0;
    int ok_bbox = 0;
    double geo_minx;
    double geo_miny;
    double geo_maxx;
    double geo_maxy;
    double ext_minx;
    double ext_miny;
    double ext_maxx;
    double ext_maxy;
    int ok_is_queryable = 0;
    int ok_red_band_index = 0;
    int ok_green_band_index = 0;
    int ok_blue_band_index = 0;
    int ok_nir_band_index = 0;
    int ok_enable_auto_ndvi = 0;
    int is_queryable;
    int red_band_index;
    int green_band_index;
    int blue_band_index;
    int nir_band_index;
    int enable_auto_ndvi;
    int ok = 0;

    xdb = rl2_double_quoted_sql (db_prefix);

/* querying the ording Coverage defs */
    sql =
	sqlite3_mprintf
	("SELECT sample_type, pixel_type, num_bands, compression, quality, tile_width, "
	 "tile_height, horz_resolution, vert_resolution, srid, nodata_pixel, "
	 "strict_resolution, mixed_resolutions, section_paths, section_md5, "
	 "section_summary, title, abstract, statistics, geo_minx, geo_miny, "
	 "geo_maxx, geo_maxy, extent_minx, extent_miny, extent_maxx, extent_maxy, "
	 "is_queryable, red_band_index, green_band_index, blue_band_index, "
	 "nir_band_index, enable_auto_ndvi, palette "
	 "FROM \"%s\".raster_coverages WHERE Lower(coverage_name) = Lower(?)",
	 xdb);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_sample = 0;
		int ok_pixel = 0;
		int ok_num_bands = 0;
		int ok_compression = 0;
		int ok_quality = 0;
		int ok_tile_width = 0;
		int ok_tile_height = 0;
		int ok_x_res = 0;
		int ok_y_res = 0;
		int ok_srid = 0;
		int ok_nodata = 1;
		int ok_strict = 0;
		int ok_mixed = 0;
		int ok_paths = 0;
		int ok_md5 = 0;
		int ok_summary = 0;
		const char *value;
		int len;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 0);
		      if (strcasecmp (value, "1-BIT") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "2-BIT") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "4-BIT") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "INT8") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "UINT8") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "INT16") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "UINT16") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "INT32") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "UINT32") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "FLOAT") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      if (strcasecmp (value, "DOUBLE") == 0)
			{
			    ok_sample = 1;
			    sample_type = value;
			}
		      sample = RL2_SAMPLE_UNKNOWN;
		      if (ok_sample)
			{
			    if (strcasecmp (sample_type, "1-BIT") == 0)
				sample = RL2_SAMPLE_1_BIT;
			    if (strcasecmp (sample_type, "2-BIT") == 0)
				sample = RL2_SAMPLE_2_BIT;
			    if (strcasecmp (sample_type, "4-BIT") == 0)
				sample = RL2_SAMPLE_4_BIT;
			    if (strcasecmp (sample_type, "INT8") == 0)
				sample = RL2_SAMPLE_INT8;
			    if (strcasecmp (sample_type, "UINT8") == 0)
				sample = RL2_SAMPLE_UINT8;
			    if (strcasecmp (sample_type, "INT16") == 0)
				sample = RL2_SAMPLE_INT16;
			    if (strcasecmp (sample_type, "UINT16") == 0)
				sample = RL2_SAMPLE_UINT16;
			    if (strcasecmp (sample_type, "INT32") == 0)
				sample = RL2_SAMPLE_INT32;
			    if (strcasecmp (sample_type, "UINT32") == 0)
				sample = RL2_SAMPLE_UINT32;
			    if (strcasecmp (sample_type, "FLOAT") == 0)
				sample = RL2_SAMPLE_FLOAT;
			    if (strcasecmp (sample_type, "DOUBLE") == 0)
				sample = RL2_SAMPLE_DOUBLE;
			}
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 1);
		      if (strcasecmp (value, "MONOCHROME") == 0)
			{
			    ok_pixel = 1;
			    pixel_type = value;
			}
		      if (strcasecmp (value, "PALETTE") == 0)
			{
			    ok_pixel = 1;
			    pixel_type = value;
			}
		      if (strcasecmp (value, "GRAYSCALE") == 0)
			{
			    ok_pixel = 1;
			    pixel_type = value;
			}
		      if (strcasecmp (value, "RGB") == 0)
			{
			    ok_pixel = 1;
			    pixel_type = value;
			}
		      if (strcasecmp (value, "MULTIBAND") == 0)
			{
			    ok_pixel = 1;
			    pixel_type = value;
			}
		      if (strcasecmp (value, "DATAGRID") == 0)
			{
			    ok_pixel = 1;
			    pixel_type = value;
			}
		      pixel = RL2_PIXEL_UNKNOWN;
		      if (ok_pixel)
			{
			    if (strcasecmp (pixel_type, "MONOCHROME") == 0)
				pixel = RL2_PIXEL_MONOCHROME;
			    if (strcasecmp (pixel_type, "GRAYSCALE") == 0)
				pixel = RL2_PIXEL_GRAYSCALE;
			    if (strcasecmp (pixel_type, "PALETTE") == 0)
				pixel = RL2_PIXEL_PALETTE;
			    if (strcasecmp (pixel_type, "RGB") == 0)
				pixel = RL2_PIXEL_RGB;
			    if (strcasecmp (pixel_type, "DATAGRID") == 0)
				pixel = RL2_PIXEL_DATAGRID;
			    if (strcasecmp (pixel_type, "MULTIBAND") == 0)
				pixel = RL2_PIXEL_MULTIBAND;
			}
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
		  {
		      num_bands = sqlite3_column_int (stmt, 2);
		      ok_num_bands = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 3);
		      if (strcasecmp (value, "NONE") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "DEFLATE") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "DEFLATE_NO") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "LZMA") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "LZMA_NO") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "PNG") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "JPEG") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "LOSSY_WEBP") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "LOSSLESS_WEBP") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "CCITTFAX4") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "CHARLS") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "LOSSY_JP2") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      if (strcasecmp (value, "LOSSLESS_JP2") == 0)
			{
			    ok_compression = 1;
			    compression = value;
			}
		      compr = RL2_COMPRESSION_UNKNOWN;
		      if (ok_compression)
			{
			    if (strcasecmp (compression, "NONE") == 0)
				compr = RL2_COMPRESSION_NONE;
			    if (strcasecmp (compression, "DEFLATE") == 0)
				compr = RL2_COMPRESSION_DEFLATE;
			    if (strcasecmp (compression, "DEFLATE_NO") == 0)
				compr = RL2_COMPRESSION_DEFLATE_NO;
			    if (strcasecmp (compression, "LZMA") == 0)
				compr = RL2_COMPRESSION_LZMA;
			    if (strcasecmp (compression, "LZMA_NO") == 0)
				compr = RL2_COMPRESSION_LZMA_NO;
			    if (strcasecmp (compression, "PNG") == 0)
				compr = RL2_COMPRESSION_PNG;
			    if (strcasecmp (compression, "GIF") == 0)
				compr = RL2_COMPRESSION_GIF;
			    if (strcasecmp (compression, "JPEG") == 0)
				compr = RL2_COMPRESSION_JPEG;
			    if (strcasecmp (compression, "WEBP") == 0)
				compr = RL2_COMPRESSION_LOSSY_WEBP;
			    if (strcasecmp (compression, "LL_WEBP") == 0)
				compr = RL2_COMPRESSION_LOSSLESS_WEBP;
			    if (strcasecmp (compression, "FAX4") == 0)
				compr = RL2_COMPRESSION_CCITTFAX4;
			    if (strcasecmp (compression, "CHARLS") == 0)
				compr = RL2_COMPRESSION_CHARLS;
			    if (strcasecmp (compression, "JP2") == 0)
				compr = RL2_COMPRESSION_LOSSY_JP2;
			    if (strcasecmp (compression, "LL_JP2") == 0)
				compr = RL2_COMPRESSION_LOSSLESS_JP2;
			}
		  }
		if (sqlite3_column_type (stmt, 4) == SQLITE_INTEGER)
		  {
		      quality = sqlite3_column_int (stmt, 4);
		      ok_quality = 1;
		  }
		if (sqlite3_column_type (stmt, 5) == SQLITE_INTEGER)
		  {
		      tile_width = sqlite3_column_int (stmt, 5);
		      ok_tile_width = 1;
		  }
		if (sqlite3_column_type (stmt, 6) == SQLITE_INTEGER)
		  {
		      tile_height = sqlite3_column_int (stmt, 6);
		      ok_tile_height = 1;
		  }
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT)
		  {
		      horz_res = sqlite3_column_double (stmt, 7);
		      ok_x_res = 1;
		  }
		if (sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		  {
		      vert_res = sqlite3_column_double (stmt, 8);
		      ok_y_res = 1;
		  }
		if (sqlite3_column_type (stmt, 9) == SQLITE_INTEGER)
		  {
		      srid = sqlite3_column_int (stmt, 9);
		      ok_srid = 1;
		  }
		if (sqlite3_column_type (stmt, 10) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt, 10);
		      int blob_sz = sqlite3_column_bytes (stmt, 10);
		      no_data = rl2_deserialize_dbms_pixel (blob, blob_sz);
		      if (no_data == NULL)
			  ok_nodata = 0;
		  }
		if (sqlite3_column_type (stmt, 11) == SQLITE_INTEGER)
		  {
		      strict_resolution = sqlite3_column_int (stmt, 11);
		      ok_strict = 1;
		  }
		if (sqlite3_column_type (stmt, 12) == SQLITE_INTEGER)
		  {
		      mixed_resolutions = sqlite3_column_int (stmt, 12);
		      ok_mixed = 1;
		  }
		if (sqlite3_column_type (stmt, 13) == SQLITE_INTEGER)
		  {
		      section_paths = sqlite3_column_int (stmt, 13);
		      ok_paths = 1;
		  }
		if (sqlite3_column_type (stmt, 14) == SQLITE_INTEGER)
		  {
		      section_md5 = sqlite3_column_int (stmt, 14);
		      ok_md5 = 1;
		  }
		if (sqlite3_column_type (stmt, 15) == SQLITE_INTEGER)
		  {
		      section_summary = sqlite3_column_int (stmt, 15);
		      ok_summary = 1;
		  }
		if (sqlite3_column_type (stmt, 16) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 16);
		      len = strlen (value);
		      title = malloc (len + 1);
		      strcpy (title, value);
		  }
		if (sqlite3_column_type (stmt, 17) == SQLITE_TEXT)
		  {
		      value = (const char *) sqlite3_column_text (stmt, 17);
		      len = strlen (value);
		      abstract = malloc (len + 1);
		      strcpy (abstract, value);
		  }
		if (sqlite3_column_type (stmt, 18) == SQLITE_BLOB)
		  {
		      statistics_sz = sqlite3_column_bytes (stmt, 18);
		      statistics = malloc (statistics_sz);
		      memcpy (statistics,
			      (const unsigned char *)
			      sqlite3_column_blob (stmt, 18), statistics_sz);
		  }
		if (sqlite3_column_type (stmt, 19) == SQLITE_FLOAT)
		  {
		      geo_minx = sqlite3_column_double (stmt, 19);
		      ok_geo++;
		  }
		if (sqlite3_column_type (stmt, 20) == SQLITE_FLOAT)
		  {
		      geo_miny = sqlite3_column_double (stmt, 20);
		      ok_geo++;
		  }
		if (sqlite3_column_type (stmt, 21) == SQLITE_FLOAT)
		  {
		      geo_maxx = sqlite3_column_double (stmt, 21);
		      ok_geo++;
		  }
		if (sqlite3_column_type (stmt, 22) == SQLITE_FLOAT)
		  {
		      geo_maxy = sqlite3_column_double (stmt, 22);
		      ok_geo++;
		  }
		if (sqlite3_column_type (stmt, 23) == SQLITE_FLOAT)
		  {
		      ext_minx = sqlite3_column_double (stmt, 23);
		      ok_bbox++;
		  }
		if (sqlite3_column_type (stmt, 24) == SQLITE_FLOAT)
		  {
		      ext_miny = sqlite3_column_double (stmt, 24);
		      ok_bbox++;
		  }
		if (sqlite3_column_type (stmt, 25) == SQLITE_FLOAT)
		  {
		      ext_maxx = sqlite3_column_double (stmt, 25);
		      ok_bbox++;
		  }
		if (sqlite3_column_type (stmt, 26) == SQLITE_FLOAT)
		  {
		      ext_maxy = sqlite3_column_double (stmt, 26);
		      ok_bbox++;
		  }
		if (sqlite3_column_type (stmt, 27) == SQLITE_INTEGER)
		  {
		      is_queryable = sqlite3_column_int (stmt, 27);
		      ok_is_queryable = 1;
		  }
		if (sqlite3_column_type (stmt, 28) == SQLITE_INTEGER)
		  {
		      red_band_index = sqlite3_column_int (stmt, 28);
		      ok_red_band_index = 1;
		  }
		if (sqlite3_column_type (stmt, 29) == SQLITE_INTEGER)
		  {
		      green_band_index = sqlite3_column_int (stmt, 29);
		      ok_red_band_index = 1;
		  }
		if (sqlite3_column_type (stmt, 30) == SQLITE_INTEGER)
		  {
		      blue_band_index = sqlite3_column_int (stmt, 30);
		      ok_red_band_index = 1;
		  }
		if (sqlite3_column_type (stmt, 31) == SQLITE_INTEGER)
		  {
		      nir_band_index = sqlite3_column_int (stmt, 31);
		      ok_red_band_index = 1;
		  }
		if (sqlite3_column_type (stmt, 32) == SQLITE_INTEGER)
		  {
		      enable_auto_ndvi = sqlite3_column_int (stmt, 32);
		      ok_enable_auto_ndvi = 1;
		  }
		if (sqlite3_column_type (stmt, 33) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt, 33);
		      int blob_sz = sqlite3_column_bytes (stmt, 33);
		      palette = rl2_deserialize_dbms_palette (blob, blob_sz);
		  }
		if (ok_sample && ok_pixel && ok_num_bands && ok_compression
		    && ok_quality && ok_tile_width && ok_tile_height
		    && ok_x_res && ok_y_res && ok_srid && ok_nodata
		    && ok_strict && ok_mixed && ok_paths && ok_md5
		    && ok_summary)
		    ok = 1;
		if (ok_geo != 4)
		    ok_geo = 0;
		if (ok_bbox != 4)
		    ok_bbox = 0;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (!ok)
	goto error;

/* creating the destination Coverage */
    ret = rl2_create_dbms_coverage (sqlite, coverage_name, sample, pixel,
				    (unsigned char) num_bands,
				    compr, quality,
				    (unsigned short) tile_width,
				    (unsigned short) tile_height, srid,
				    horz_res, vert_res, no_data, palette,
				    strict_resolution, mixed_resolutions,
				    section_paths, section_md5,
				    section_summary);
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    if (palette != NULL)
	rl2_destroy_palette (palette);
    if (ret != RL2_OK)
	goto error;

/* completing the destination coverage */
    sql = "UPDATE main.raster_coverages SET title = ?, "
	"abstract = ?, statistics = ?, geo_minx = ?, geo_miny = ?, geo_maxx = ?, "
	"geo_maxy = ?, extent_minx = ?, extent_miny = ?, extent_maxx = ?, "
	"extent_maxy = ?, is_queryable = ?, red_band_index = ?, green_band_index = ?, "
	"blue_band_index = ?, nir_band_index = ?, enable_auto_ndvi = ? "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SQL error: %s\n%s\n", sql, sqlite3_errmsg (sqlite));
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (title == NULL)
	sqlite3_bind_null (stmt, 1);
    else
	sqlite3_bind_text (stmt, 1, title, strlen (title), free);
    if (abstract == NULL)
	sqlite3_bind_null (stmt, 2);
    else
	sqlite3_bind_text (stmt, 2, abstract, strlen (abstract), free);
    if (statistics == NULL)
	sqlite3_bind_null (stmt, 3);
    else
	sqlite3_bind_blob (stmt, 3, statistics, statistics_sz, free);
    if (ok_geo)
      {
	  sqlite3_bind_double (stmt, 4, geo_minx);
	  sqlite3_bind_double (stmt, 5, geo_miny);
	  sqlite3_bind_double (stmt, 6, geo_maxx);
	  sqlite3_bind_double (stmt, 7, geo_maxy);
      }
    else
      {
	  sqlite3_bind_null (stmt, 4);
	  sqlite3_bind_null (stmt, 5);
	  sqlite3_bind_null (stmt, 6);
	  sqlite3_bind_null (stmt, 7);
      }
    if (ok_bbox)
      {
	  sqlite3_bind_double (stmt, 8, ext_minx);
	  sqlite3_bind_double (stmt, 9, ext_miny);
	  sqlite3_bind_double (stmt, 10, ext_maxx);
	  sqlite3_bind_double (stmt, 11, ext_maxy);
      }
    else
      {
	  sqlite3_bind_null (stmt, 8);
	  sqlite3_bind_null (stmt, 9);
	  sqlite3_bind_null (stmt, 10);
	  sqlite3_bind_null (stmt, 11);
      }
    if (!ok_is_queryable)
	sqlite3_bind_null (stmt, 12);
    else
	sqlite3_bind_int (stmt, 12, is_queryable);
    if (!ok_red_band_index)
	sqlite3_bind_null (stmt, 13);
    else
	sqlite3_bind_int (stmt, 13, red_band_index);
    if (!ok_green_band_index)
	sqlite3_bind_null (stmt, 14);
    else
	sqlite3_bind_int (stmt, 14, green_band_index);
    if (!ok_blue_band_index)
	sqlite3_bind_null (stmt, 15);
    else
	sqlite3_bind_int (stmt, 15, blue_band_index);
    if (!ok_nir_band_index)
	sqlite3_bind_null (stmt, 16);
    else
	sqlite3_bind_int (stmt, 16, nir_band_index);
    if (!ok_enable_auto_ndvi)
	sqlite3_bind_null (stmt, 17);
    else
	sqlite3_bind_int (stmt, 17, enable_auto_ndvi);
    sqlite3_bind_text (stmt, 18, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	goto ok_continue;
    fprintf (stderr,
	     "sqlite3_step() error: UPDATE raster_coverages \"%s\"\n",
	     sqlite3_errmsg (sqlite));
    goto error;

  ok_continue:
/* copying coverage-LEVELS */
    xcoverage = sqlite3_mprintf ("%s_levels", coverage_name);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    sql = sqlite3_mprintf ("INSERT INTO main.\"%s\" (pyramid_level, "
			   "x_resolution_1_1, y_resolution_1_1, x_resolution_1_2, y_resolution_1_2, "
			   "x_resolution_1_4, y_resolution_1_4, x_resolution_1_8, y_resolution_1_8) "
			   "SELECT pyramid_level, x_resolution_1_1, y_resolution_1_1, x_resolution_1_2, "
			   "y_resolution_1_2, x_resolution_1_4, y_resolution_1_4, x_resolution_1_8, "
			   "y_resolution_1_8 FROM \"%s\".\"%s\"", xxcoverage,
			   xdb, xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* copying  coverage-SECTIONS */
    xcoverage = sqlite3_mprintf ("%s_sections", coverage_name);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    sql =
	sqlite3_mprintf ("INSERT INTO main.\"%s\" (section_id, section_name, "
			 "width, height, file_path, md5_checksum, summary, statistics, geometry) "
			 "SELECT section_id, section_name, width, height, file_path, md5_checksum, "
			 "summary, statistics, geometry FROM \"%s\".\"%s\"",
			 xxcoverage, xdb, xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* copying  coverage-TILES */
    xcoverage = sqlite3_mprintf ("%s_tiles", coverage_name);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    sql = sqlite3_mprintf ("INSERT INTO main.\"%s\" (tile_id, pyramid_level, "
			   "section_id, geometry) SELECT tile_id, pyramid_level, section_id, geometry "
			   "FROM \"%s\".\"%s\"", xxcoverage, xdb, xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* copying  coverage-TILE_DATA */
    xcoverage = sqlite3_mprintf ("%s_tile_data", coverage_name);
    xxcoverage = rl2_double_quoted_sql (xcoverage);
    sqlite3_free (xcoverage);
    sql = sqlite3_mprintf ("INSERT INTO main.\"%s\" (tile_id, tile_data_odd, "
			   "tile_data_even) SELECT tile_id, tile_data_odd, tile_data_even "
			   "FROM \"%s\".\"%s\"", xxcoverage, xdb, xxcoverage);
    free (xxcoverage);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* copying KEYWORDS */
    sql = sqlite3_mprintf ("INSERT INTO main.raster_coverages_keyword "
			   "(coverage_name, keyword) SELECT coverage_name, keyword "
			   "FROM \"%s\".raster_coverages_keyword", xdb);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* copying SRID */
    sql = sqlite3_mprintf ("INSERT INTO main.raster_coverages_srid "
			   "(coverage_name, srid, extent_minx, extent_miny, extent_maxx, extent_maxx) "
			   "SELECT coverage_name, srid, extent_minx, extent_miny, extent_maxx, extent_maxx "
			   "FROM \"%s\".raster_coverages_srid", xdb);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    free (xdb);
    return RL2_OK;

  error:
    if (xdb != NULL)
	free (xdb);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return RL2_ERROR;
}
