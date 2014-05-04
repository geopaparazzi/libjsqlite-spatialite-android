libjsqlite-spatialite-android
=============================

Project to create the libjsqlite.so used in geopaparazzi
* `android-ndk-r9c` was used to compile this project properly
   * `LOCAL_LDLIBS is always ignored for static libraries`
* for proper SpatialIndex support, the rtree module must be activated in the sqlite.mk files
   * `-DSQLITE_ENABLE_RTREE=1` in the common_sqlite_flags
* in the `archive` directory there is a 
   * `20140105.libjsqlite.4.1.1.tar.bz2` 
      * with the compiled versions of `libjsqlite.so` for
         * `armeabi`, `armeabi-v7a` and `x86`

---

* most of this information will be found in the Wiki pages

[The Wiki Home Page as a starting point](https://github.com/geopaparazzi/libjsqlite-spatialite-android/wiki)


---
Directory structure:
* **archive**
   * original archives used in project
      * not all of the archives are really used
* **libjsqlite-spatialite-android**
   * based on the original copy create by Brad Hards
      * http://code.google.com/r/bradh-spatialiteandroid
         * `Minor hacks ... This won't be the one you're looking to clone... `
   * which came from:
      * http://code.google.com/p/spatialite-android/
         * `Spatialite for Android`
            * which noted the link for needed corrections of a PROJ.4 4.8.0 bug running on android

---
Configuration used presently [2014-01-04]:
* Android_4.1.1.mk  (Spatialite 4.1.1) [of which `Android.mk` is a copy]
   * iconv-1.13.1.mk
      * using directory `libiconv-1.13.1`
         * For Spatialite with VirtualShapes,VirtualXL support: iconv is needed
   * sqlite-3071700.mk [with support for 'PRAGMA application_id']
      * using directory `sqlite-amalgamation-3071700`
      * for propers SpatialIndex support, the `rtree` module must be activated
         * `-DSQLITE_ENABLE_RTREE=1` in the common_sqlite_flags
   * proj4-4.8.0.mk
      * using directory `proj-4.8.0`
         * `pj_init.c`has been adapted to address a known problem in android:
            * 20121217: Recover gracefully if setlocale() returns NULL like on Android (#204).
               * http://trac.osgeo.org/proj/changeset/2305
   * geos-3.4.2.mk
      * using directory `geos-3.4.2`
   * spatialite-4.1.1.mk
      * using directory `libspatialite-4.1.1`
         * with VirtualShapes,VirtualXL support: iconv is needed
         * with GeosAdvanced: zlib is needed (LOCAL_LDLIBS parameter)
            * with `android-ndk-r9c` this is no longer possible
               * everything compiled with out error, without adding `zlib`
                  * spatialite shows `HasGeosAdvanced[1]`
                  * if problems arise, this will be the cause
   * jsqlite-20120209.mk
      * using directory `javasqlite-20120209`
* Application.mk
   * `armeabi  armeabi-v7a x86`
* version shown in geopaparazzi [`2014-01-05`]:
   * <pre>[javasqlite[20120209],spatialite[4.1.1],proj4[Rel. 4.8.0, 6 March 2012],
   geos[3.4.2-CAPI-1.8.2 r3921],
   spatialite_properties[HasIconv[1],HasMathSql[1],HasGeoCallbacks[0],HasProj[1],
   HasGeos[1],HasGeosAdvanced[1],HasGeosTrunk[0],HasLwGeom[0],
   HasLibXML2[0],HasEpsg[1],HasFreeXL[0]]]
   </pre>

---
`./configure`commands:
* as a general rule, most projects can be configured with:
   * `./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi`
* with `sqlite`this rule does ***NOT*** apply.
   * amalgamation: there is no `./configure` file
   * needed settings must be done in the `sqlite-*.mk` files
* with `spatialite`this rule does ***NOT*** apply.  [spatialite-users: 2014-01-02]
   * you must adapt the `./configure` depending on the GEOS-related settings
      * `GEOS &lt; 3.3.0`
         * `./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi`
            * please note: these versions are nowadays completely obsolete and always impliy: `--disable-geosadvanced=yes`
      * `GEOS 3.3.x`
         * <pre>./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
            --enable-geosadvanced=yes</pre>
            * but always implies: `--disable-geostrunk=yes`
      * `GEOS &gt; 3.4.x`
         * <pre>./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi
           --enable-geosadvanced=yes --enable-geostrunk=yes</pre>
            * (e.g. suppprting the Delaunay Triangulation)
   * for Android, it may be better to set this in the `spatialite-*.mk` files
      * at the moment I am glad that it works at all

* version shown in geopaparazzi:
   * <pre>[javasqlite[20120209],spatialite[4.1.1],proj4[Rel. 4.8.0, 6 March 2012],
   geos[3.4.2-CAPI-1.8.2 r3921],
   spatialite_properties[HasIconv[1],HasMathSql[1],HasGeoCallbacks[0],HasProj[1],
   HasGeos[1],HasGeosAdvanced[1],HasGeosTrunk[1],HasLwGeom[0],
   HasLibXML2[0],HasEpsg[1],HasFreeXL[0]]]
   </pre>

---
Compiling and expected results:
* assuming the platform tools are in your path:
   * my settings in `.bashrc`are:
      * <pre>
export ANDROID_BASE=~/000_links/gnu_source/adt-bundle-linux
export ANDROID_HOME=$ANDROID_BASE/sdk
export PATH=$PATH$ANDROID_HOME/platform-tools:$ANDROID_HOME/tools
export PATH=$PATH:$ANDROID_BASE/android-ndk-r9c
export JAVA_HOME=/usr/lib/jvm/default-java
export ANT_HOME=$ANDROID_BASE/apache-ant
export PATH=$PATH:$ANT_HOME/bin
</pre>
* `ndk-build` in the `libjsqlite-spatialite-android/spatialite-android-library/jni` directory
   * otherwise use a construction such as:
   <pre>
   NDK_DIR=/home/mj10777/gnu_source/adt-bundle-linux/android-ndk-r9c ndk-build
   </pre>

* should bring out the following results :
<pre>
ndk-build -B
Compile thumb  : jsqlite &lt;= sqlite_jni.c
Compile thumb  : sqlite &lt;= sqlite3.c
StaticLibrary  : libsqlite.a
Compile thumb  : spatialite &lt;= dxf_load_distinct.c
...
bla,bla,bla,bla ....
...
Compile++ x86    : geos &lt;= Profiler.cpp
StaticLibrary  : libgeos.a
SharedLibrary  : libjsqlite.so
Install        : libjsqlite.so =&gt; libs/x86/libjsqlite.so
</pre>
   * `libs` should have 3 directories : **armeabi** **armeabi-v7a** **x86**
      * each with one `libjsqlite.so` of about 5.7 MB in size
         * these **directories** should copied to:
            * `geopaparazzi/geopaparazzispatialitelibrary/libs`
      * a archived copy of these libs can be found in `archive/20140105.libjsqlite.4.1.1.tar.bz2`
   * `obj/local` should have 3 directories : **armeabi** **armeabi-v7a** **x86**
      * each with one `libjsqlite.so` and 5 `.a`files
         * `libgeos.a  libiconv.a  libproj.a  libspatialite.a  libsqlite.a`
      * and one `objs` directory with many sub-directories
            * all of which are no longer not needed [and ignored by git]

---
Note:
* after `ndk-build clean`
   * `libs` will still have 3 directories : **armeabi** **armeabi-v7a** **x86**
      * each should be empty
   * `obj/local` will still have 3 directories : **armeabi**, **armeabi-v7a** and **x86**
      * each with one `libjsqlite.so` and 5 `.a` files but no `objs` directory

---
Adding new project sources:
* original archiv should be stored in `archive`
   * sample: a new version of proj4 will soon come out
      * possibly called `proj-4.9.0.tar.gz` --> store in `archive`
         * unpack archiv into `libjsqlite-spatialite-android/spatialite-android-library/jni`
             * run:
                * `./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi`
                   * inside the new directory `proj-4.9.0`
                * `./configure` often creates files needed by the project to compile properly
                    * if you recieve a `No such file or directory`, this can be the cause
                * the project settings are done in the `something.mk`file
                     * and **not** with `./configure`, so no further parameters are needed
         * make a copy of `proj4-4.8.0.mk` as `proj4-4.9.0.mk`
            * check which files have been remove or added
               * adapt as needed, including specfic settings normaly done with `./configure`

---
Known portions of the project that do not work:
* Android_3.0.1.mk (Spatialite 3.0.1)
   * based on 'spatialite-for-android-3.0.1.zip'
      * I could not get this to compile [geos: geom/CoordinateArraySequenceFactory.cpp]
         * it would be nice to get this working, since geopaparazzi uses this at the moment
            * version shown in geopaparazzi:
               * <pre>javasqlite[20120209],spatialite[3.0.1],proj4[Rel. 4.7.1, 23 September 2009],geos[3.2.2-CAPI-1.6.2],exception[? not a spatialite database, or spatialite < 4 ?]]</pre>
* iconv-1.14.mk
   * using directory `libiconv-1.14`
      * recieves:
         * <pre>libiconv-1.14/libcharset/lib/localcharset.c:51:24: fatal error:
                   langinfo.h: No such file or directory</pre>
            * it is not being used
* jsqlite-20131124.mk
    * from `http://www.ch-werner.de/javasqlite/`
    * using directory `javasqlite-20131124`
       * the final result is only about 10% of the expected size
          * it is not being used

* Android_4.0.0.mk  (Spatialite 4.0.0)
    * has not been tested yet, since this version is being skipped
       * recieves
          * <pre>libspatialite-4.0.0/src/spatialite/spatialite.c:22549:
                    error: undefined reference to 'virtualtext_extension_init'</pre>

---
Notes: when the use of `ndk-build clean` fails:
* This is a bug in ndk **r8e**. Works correctly in `r9c`
   * How to fix it:
      * Edit the file $(ndk_dir)build/core/build-binary.mk
      * Change the line
      <pre>
$(cleantarget): PRIVATE_CLEAN_FILES := ($(my)OBJS)
**to**
$(cleantarget): PRIVATE_CLEAN_FILES :=  ***$***($(my)OBJS)
</pre>
         * after that it should work correctly
<pre>
Clean: geos [armeabi]
Clean: gnustl_shared [armeabi]
Clean: gnustl_static [armeabi]
Clean: iconv [armeabi]
Clean: jsqlite [armeabi]
Clean: proj [armeabi]
Clean: spatialite [armeabi]
Clean: sqlite [armeabi]
Clean: geos [armeabi-v7a]
...
Clean: proj [x86]
Clean: spatialite [x86]
Clean: sqlite [x86]
</pre>
* it is important to use `ndk-build clean` before making major configuration changes
   * avoids something in the project getting `confused` ...

---

2014-01-05

---

