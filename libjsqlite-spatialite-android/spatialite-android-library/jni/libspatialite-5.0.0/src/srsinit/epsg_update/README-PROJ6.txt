HOW-TO: UPDATE spatial_ref_sys SELF-INITIALIZING C CODE
============================================================================
this procedure is specifically intended to support PROJ.6
============================================================================

STEP #0: building GDAL
--------
- download the latest GDAL 3.0.0 sources
- build and install (no special settings are required)
- CAVEAT: set the LD_LIBRARY_PATH env variable so to be
  absolutely sure to target this custom built GDAL and
  not the default system installation
  

STEP #1: compiling the C GDAL-PROJ.6 utility
--------
# cd {libspatialite-source}/src/srsinit/epsg_update

Linux:
# gcc epsg_from_gdal-proj6.c -o epsg_from_gdal-proj6 -lgdal

Windows [MinGW]:
# gcc -I/usr/local/include epsg_from_gdal-proj6.c -o epsg_from_gdal-proj6.exe \
      -L/usr/local/lib -lgdal



STEP #2: getting the basic EPSG file
--------
# rm epsg-proj6
# epsg_from_gdal-proj6 >epsg

all right: this "epsg-proj6" output file will be used as a "seed" 
into the next step
NOTE: the "epsg-proj4" input file is a required prerequisite



STEP #3: compiling the C generator tool
--------
# cd {libspatialite-source}/src/srsinit/epsg_update

Linux:
# gcc auto_epsg_ext.c -o auto_epsg_ext

Windows [MinGW]:
# gcc auto_epsg_ext.c -o auto_epsg_ext.exe



STEP #4: generating the C code [inlined EPSG dataset]
--------
# rm epsg_inlined_*.c
# ./auto_epsg_ext

at the end of this step several "epsg_inlined_*.c" files will be generated



STEP #5: final setup
--------
- copy the generated file into the parent dir:
  rm ../epsg_inlined*.c
  cp epsg_inlined*.c ..
- be sure to update as required the repository (ADD/DEL)
- be sure to update as required Makefile.am
- and finally commit into the repository



STEP #6: pre-release final check
--------
- after building and installing the new libspatialite
  incorporating the most recent SRSes defined by GDAL
  always remember to run from the shell the test script:
  ./check_srid_spatialite.sh

- then carefully check for any possible error detected
  by the above mentioned test script

