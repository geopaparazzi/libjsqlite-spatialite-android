/* 

sample1.c

Author: Sandro Furieri a.furieri@lqt.it
 
This software is provided 'as-is', without any express or implied
warranty.  In no event will the author be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
these headers are required in order to support
SQLite, SpatiaLite and RasterLite2 
*/
#include <rasterlite2/rasterlite2.h>
#include <spatialite.h>

static void
fill_horz (unsigned char r, unsigned char g, unsigned char b,
	   unsigned char *pix, int strip, unsigned int width,
	   unsigned int height)
{
/* color-filling an horizontal strip */
    unsigned int strip0 = height / 3;
    unsigned int strip1 = height - strip0;
    unsigned int strip2 = height;
    unsigned int min;
    unsigned int max;
    unsigned int x;
    unsigned int y;
    if (strip == 1)
      {
	  min = strip0;
	  max = strip1;
      }
    else if (strip == 2)
      {
	  min = strip1;
	  max = strip2;
      }
    else
      {
	  min = 0;
	  max = strip1;
      }
    for (y = min; y < max; y++)
      {
	  unsigned char *p = pix + (y * width * 3);
	  for (x = 0; x < width; x++)
	    {
		*p++ = r;
		*p++ = g;
		*p++ = b;
	    }
      }
}

static void
fill_vert (unsigned char r, unsigned char g, unsigned char b,
	   unsigned char *pix, int strip,
	   unsigned int width, unsigned int height)
{
/* color-filling a vertical strip */
    unsigned int strip0 = width / 3;
    unsigned int strip1 = width - strip0;
    unsigned int strip2 = width;
    unsigned int min;
    unsigned int max;
    unsigned int x;
    unsigned int y;
    if (strip == 1)
      {
	  min = strip0;
	  max = strip1;
      }
    else if (strip == 2)
      {
	  min = strip1;
	  max = strip2;
      }
    else
      {
	  min = 0;
	  max = strip1;
      }
    for (y = 0; y < height; y++)
      {
	  unsigned char *p = pix + (y * width * 3) + (min * 3);
	  for (x = min; x < max; x++)
	    {
		*p++ = r;
		*p++ = g;
		*p++ = b;
	    }
      }
}

static void
add_border (unsigned char *pixels, unsigned int width, unsigned int height)
{
/* adding a narrow gray border */
    unsigned char *p = pixels;
    unsigned int y;
    unsigned int x;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		int mark = 0;
		if (x < 8 || x > (width - 9))
		    mark = 1;
		if (y < 8 || y > (height - 9))
		    mark = 1;
		if (mark)
		  {
		      *p++ = 192;
		      *p++ = 192;
		      *p++ = 192;
		  }
		else
		    p += 3;
	    }
      }
}

static void
create_austria (unsigned int flag_base, unsigned char **pixels, int *pixels_sz)
{
/* creating the austrian flag (RGB pixel buffer) */
    unsigned int width = flag_base * 3;
    unsigned int height = flag_base * 2;
    unsigned int sz = width * height * 3;
    unsigned char *pix = malloc (sz);
    fill_horz (237, 41, 35, pix, 0, width, height);	/* red */
    fill_horz (255, 255, 255, pix, 1, width, height);	/* white */
    fill_horz (237, 41, 35, pix, 2, width, height);	/* red */
    add_border (pix, width, height);
    *pixels = pix;
    *pixels_sz = sz;
}


static void
create_netherlands (unsigned int flag_base, unsigned char **pixels,
		    int *pixels_sz)
{
/* creating the dutch flag (RGB pixel buffer) */
    unsigned int width = flag_base * 3;
    unsigned int height = flag_base * 2;
    unsigned int sz = width * height * 3;
    unsigned char *pix = malloc (sz);
    fill_horz (174, 28, 40, pix, 0, width, height);	/* red */
    fill_horz (255, 255, 255, pix, 1, width, height);	/* white */
    fill_horz (33, 70, 139, pix, 2, width, height);	/* blue */
    add_border (pix, width, height);
    *pixels = pix;
    *pixels_sz = sz;
}

static void
create_germany (unsigned int flag_base, unsigned char **pixels, int *pixels_sz)
{
/* creating the german flag (RGB pixel buffer) */
    unsigned int width = flag_base * 3;
    unsigned int height = flag_base * 2;
    unsigned int sz = width * height * 3;
    unsigned char *pix = malloc (sz);
    fill_horz (1, 1, 1, pix, 0, width, height);	/* black */
    fill_horz (221, 0, 0, pix, 1, width, height);	/* red */
    fill_horz (255, 206, 0, pix, 2, width, height);	/* yellow */
    add_border (pix, width, height);
    *pixels = pix;
    *pixels_sz = sz;
}

static void
create_france (unsigned int flag_base, unsigned char **pixels, int *pixels_sz)
{
/* creating the french flag (RGB pixel buffer) */
    unsigned int width = flag_base * 3;
    unsigned int height = flag_base * 2;
    unsigned int sz = width * height * 3;
    unsigned char *pix = malloc (sz);
    fill_vert (0, 85, 164, pix, 0, width, height);	/* blue */
    fill_vert (255, 255, 255, pix, 1, width, height);	/* white */
    fill_vert (239, 65, 53, pix, 2, width, height);	/* red */
    add_border (pix, width, height);
    *pixels = pix;
    *pixels_sz = sz;
}

static void
create_belgium (unsigned int flag_base, unsigned char **pixels, int *pixels_sz)
{
/* creating the french flag (RGB pixel buffer) */
    unsigned int width = flag_base * 3;
    unsigned int height = flag_base * 2;
    unsigned int sz = width * height * 3;
    unsigned char *pix = malloc (sz);
    fill_vert (1, 1, 1, pix, 0, width, height);	/* black */
    fill_vert (255, 233, 54, pix, 1, width, height);	/* yellow */
    fill_vert (255, 15, 33, pix, 2, width, height);	/* red */
    add_border (pix, width, height);
    *pixels = pix;
    *pixels_sz = sz;
}

static void
create_italy (unsigned int flag_base, unsigned char **pixels, int *pixels_sz)
{
/* creating the italian flag (RGB pixel buffer) */
    unsigned int width = flag_base * 3;
    unsigned int height = flag_base * 2;
    unsigned int sz = width * height * 3;
    unsigned char *pix = malloc (sz);
    fill_vert (0, 146, 80, pix, 0, width, height);	/* green */
    fill_vert (241, 242, 241, pix, 1, width, height);	/* white */
    fill_vert (206, 43, 55, pix, 2, width, height);	/* red */
    add_border (pix, width, height);
    *pixels = pix;
    *pixels_sz = sz;
}

static int
insert_section (sqlite3 * handle, const char *sql, unsigned int width,
		unsigned int height, unsigned char *pixels, int pixels_sz)
{
/* attempting to insert a RAW pixel buffer (Section) */
    sqlite3_stmt *stmt = NULL;
    int ret;
    int ok = 0;

    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("SELECT RL2_ImportSectionRawPixels SQL error: %s\n",
		  sqlite3_errmsg (handle));
	  goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, width);
    sqlite3_bind_int (stmt, 2, height);
    sqlite3_bind_blob (stmt, 3, pixels, pixels_sz, free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      ok = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr,
			 "SELECT RL2_ImportSectionRawPixels; sqlite3_step() error: %s\n",
			 sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    if (ok != 1)
      {
	  fprintf (stderr, "ERROR: unable to insert RAW pixels\n");
	  ok = 0;
      }
    return ok;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

int
main (int argc, char *argv[])
{
    const char *sql;
    int ret;
    unsigned int flag_base = 1234;
    double h_res = 1.5 / ((double) flag_base * 3.0);
    double v_res = 1.0 / ((double) flag_base * 2.0);
    char *err_msg = NULL;
    sqlite3 *handle;
    void *cache;
    unsigned char *pixels;
    int pixels_sz;
    rl2PixelPtr no_data = NULL;
    int err = 0;

    if (argc != 2)
      {
	  fprintf (stderr, "usage: %s test_db_path\n", argv[0]);
	  return -1;
      }

/* trying to connect (and eventually create) the target DB */
    ret =
	sqlite3_open_v2 (argv[1], &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  printf ("cannot open '%s': %s\n", argv[1], sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

/* initializing both SpatiaLite and RasterLite2 */
    cache = spatialite_alloc_connection ();
    spatialite_init_ex (handle, cache, 0);
    rl2_init (handle, 0);


/* showing the SQLite version */
    printf ("SQLite version: %s\n", sqlite3_libversion ());
/* showing the RasterLite2 version */
    printf ("RasterLite2 version: %s\n", rl2_version ());
    printf ("\n\n");

/* initializing the DB Metatables */
    sql = "SELECT InitSpatialMetadata(1)";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  err = 1;
	  goto stop;
      }
    sql = "SELECT CreateRasterCoveragesTable()";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRasterCoveragesTable() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  err = 1;
	  goto stop;
      }
    sql = "SELECT CreateStylingTables()";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateStylingTables() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  err = 1;
	  goto stop;
      }

/* creating a BLACK pixel (NO-DATA / Transparent color) */
    no_data = rl2_create_pixel (RL2_SAMPLE_UINT8,	/* unsigned 8bit sample */
				RL2_PIXEL_RGB,	/* RGB pixel */
				3	/* number of bands */
	);
    if (no_data == NULL)
      {
	  fprintf (stderr, "unable to create a Black NoData pixel\n");
	  err = 1;
	  goto stop;
      }
    rl2_set_pixel_sample_uint8 (no_data, 0, 0);
    rl2_set_pixel_sample_uint8 (no_data, 1, 0);
    rl2_set_pixel_sample_uint8 (no_data, 2, 0);

/* creating a ficticious RGB coverage */
    if (rl2_create_dbms_coverage (handle,	/* DB handle */
				  "euro_flags",	/* Coverage name */
				  RL2_SAMPLE_UINT8,	/* unsigned 8bit sample */
				  RL2_PIXEL_RGB,	/* RGB pixel */
				  3,	/* number of bands */
				  RL2_COMPRESSION_PNG,	/* PNG compressed tiles */
				  100,	/* compression quality */
				  256,	/* tile Width */
				  256,	/* tile Height */
				  4326,	/* pseudo-SRID (this is simply to define a cartesian space) */
				  h_res,	/* horizontal pixel resolution */
				  v_res,	/* vertical pixel resolution */
				  no_data,	/* transparent Color = BLACK */
				  NULL,	/* palette (not required by RGB) */
				  1,	/* enabling StrictResolution mode */
				  0,	/* disabling MixedResolutions mode */
				  1,	/* enabling Section Paths recording */
				  1,	/* enabling Section MD5 checksum */
				  1	/* enabling Section Summary */
	) != RL2_OK)
	return 0;

/* inserting the belgian flag (Section 1) */
    create_belgium (flag_base, &pixels, &pixels_sz);
    sql = "SELECT RL2_ImportSectionRawPixels('euro_flags', 'belgium', "
	"?, ?, ?, BuildMBR(0, 0, 1.5, 1, 4326), 1, 1, 0)";
    if (!insert_section
	(handle, sql, flag_base * 3, flag_base * 2, pixels, pixels_sz))
      {
	  err = 1;
	  goto stop;
      }

/* inserting the german flag (Section 2) */
    create_germany (flag_base, &pixels, &pixels_sz);
    sql = "SELECT RL2_ImportSectionRawPixels('euro_flags', 'germany', "
	"?, ?, ?, BuildMBR(1.5, 0, 3, 1, 4326), 1, 1, 0)";
    if (!insert_section
	(handle, sql, flag_base * 3, flag_base * 2, pixels, pixels_sz))
      {
	  err = 1;
	  goto stop;
      }

/* inserting the italian flag (Section 3) */
    create_italy (flag_base, &pixels, &pixels_sz);
    sql = "SELECT RL2_ImportSectionRawPixels('euro_flags', 'italy', "
	"?, ?, ?, BuildMBR(3, 0, 4.5, 1, 4326), 1, 1, 0)";
    if (!insert_section
	(handle, sql, flag_base * 3, flag_base * 2, pixels, pixels_sz))
      {
	  err = 1;
	  goto stop;
      }

/* inserting the austrian flag (Section 4) */
    create_austria (flag_base, &pixels, &pixels_sz);
    sql = "SELECT RL2_ImportSectionRawPixels('euro_flags', 'austria', "
	"?, ?, ?, BuildMBR(0, 1, 1.5, 2, 4326), 1, 1, 0)";
    if (!insert_section
	(handle, sql, flag_base * 3, flag_base * 2, pixels, pixels_sz))
      {
	  err = 1;
	  goto stop;
      }

/* inserting the french flag (Section 5) */
    create_france (flag_base, &pixels, &pixels_sz);
    sql = "SELECT RL2_ImportSectionRawPixels('euro_flags', 'france', "
	"?, ?, ?, BuildMBR(1.5, 1, 3, 2, 4326), 1, 1, 0)";
    if (!insert_section
	(handle, sql, flag_base * 3, flag_base * 2, pixels, pixels_sz))
      {
	  err = 1;
	  goto stop;
      }

/* inserting the dutch flag (Section 6) */
    create_netherlands (flag_base, &pixels, &pixels_sz);
    sql = "SELECT RL2_ImportSectionRawPixels('euro_flags', 'netherlands', "
	"?, ?, ?, BuildMBR(3, 1, 4.5, 2, 4326), 1, 1, 0)";
    if (!insert_section
	(handle, sql, flag_base * 3, flag_base * 2, pixels, pixels_sz))
      {
	  err = 1;
	  goto stop;
      }

/* disconnecting the DB and final cleanup */
  stop:
    if (no_data != NULL)
	rl2_destroy_pixel (no_data);
    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  printf ("close() error: %s\n", sqlite3_errmsg (handle));
	  return -1;
      }
    if (!err)
	printf ("\n\nsample successfully terminated\n");
    spatialite_cleanup_ex (cache);
    spatialite_shutdown ();
    return 0;
}
