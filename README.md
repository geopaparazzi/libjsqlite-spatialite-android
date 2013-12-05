libjsqlite-spatialite-android
=============================

Project to create the libjsqlite.so used in geopaparazzi

---
Directory structure:
* **archive**
   * original archives used in project
      * not all of the archives are really used
* **libjsqlite-spatialite-android**
   * based on the original copy create by Brad Hards
      * http://code.google.com/r/bradh-spatialiteandroid
         * `Minor hacks ... This won't be the one you're looking to clone... `
      * http://code.google.com/p/spatialite-android/
         * `Spatialite for Android`
            * which noted the link for needed corrections of a PROJ.4 4.8.0 bug running on android

---
Configuration used presently [2013-12-04]:
* Android_4.1.1.mk [of which `Android.mk` is a copy]
   * iconv-1.13.1.mk
      * using directory `libiconv-1.13.1`
   * sqlite-3080100.mk
      * using directory `sqlite-amalgamation-3080100`
   * proj4-4.8.0.mk
      * using directory `proj-4.8.0`
         * `pj_init.c`has been adapted to address a known problem in android:
            * 20121217: Recover gracefully if setlocale() returns NULL like on Android (#204).
               * http://trac.osgeo.org/proj/changeset/2305
   * geos-3.4.2.mk
      * using directory `geos-3.4.2`
   * spatialite-4.1.1.mk
      * using directory `libspatialite-4.1.1`
   * jsqlite-20120209.mk
      * using directory `javasqlite-20120209`
* Application.mk
   * `armeabi  armeabi-v7a x86`

---
Compiling and expected results:
* assuming the platform tools are in your path:
   * my settings in `.bashrc`are:
      * <pre>
export ANDROID_HOME=~/000_links/gnu_source/adt-bundle-linux/sdk
export PATH=$PATH/home/mj10777/gnu_source/adt-bundle-linux/sdk/platform-tools:/home/mj10777/gnu_source/adt-bundle-linux/sdk/tools:/home/mj10777/gnu_source/adt-bundle-linux/android-ndk-r8e
</pre>
* `ndk-build` in the `libjsqlite-spatialite-android/spatialite-android-library/jni` directory
   * otherwise use a construction such as:
   <pre>
   NDK_DIR=/home/mj10777/gnu_source/adt-bundle-linux/android-ndk-r8e ndk-build
   </pre>

* should bring out the correct results:
<pre>
StaticLibrary  : libgeos.a
SharedLibrary  : libjsqlite.so
Install        : libjsqlite.so => libs/x86/libjsqlite.so
</pre>
   * `libs` should have 3 directories : **armeabi** **armeabi-v7a** **x86**
      * each with one `libjsqlite.so` of about 5.7 MB size
         * these **directories** should copied to:
            * `geopaparazzi/geopaparazzispatialitelibrary/libs`
   * `obj/local` should have 3 directories : **armeabi** **armeabi-v7a** **x86**
      * each with one `libjsqlite.so` and 5 `.a`files and one `objs` directory with many sub-directories
         * `libgeos.a  libiconv.a  libproj.a  libspatialite.a  libsqlite.a`

---
Note:
* after `ndk-build clean`
   * `libs` will still have 3 directrories : **armeabi** **armeabi-v7a** **x86**
      * each should be empty
   * `obj/local` will still have 3 directories : **armeabi** **armeabi-v7a** **x86**
      * each with one `libjsqlite.so` and 5 `.a`files but no `objs` directory

---
Adding new project sources:
* original archiv should be stored in `archive`
   * sample: a new version of proj4 will soon come out
      * possibly called `proj-4.9.0.tar.gz` --> store in `archive`
         * unpack archive into `libjsqlite-spatialite-android/spatialite-android-library/jni`
             * run:
                * `./configure --build=x86_64-pc-linux-gnu --host=arm-linux-eabi`
                   * inside the new directory `proj-4.9.0`
                   * `./configure` often creates files needed by the project to compile properly
                       * if you recieve a `No such file or directory`, this can be the cause
                   * the files setting for the project are set in the `something.mk`file and **not** with `./configure`
         * make a copy of `proj4-4.8.0.mk` as `proj4-4.9.0.mk`
            * check which files have been remove or added
               * adapt as needed, including specfic setting normaly done with `./configure`

---

Known portions of the project that do not work:
* Android_3.0.1.mk
   * based on 'spatialite-for-android-3.0.1.zip'
      * I could not get this to compile
* iconv-1.14.mk
   * using directory `libiconv-1.14`
         * recieves:
            * `libiconv-1.14/libcharset/lib/localcharset.c:51:24: fatal error: langinfo.h: No such file or directory`
               * it is not being used
* jsqlite-20131124.mk
   * using directory `javasqlite-20131124`
      * the final result is only about 10% of the expected size
         * it is not being used

---

Notes: when the use of `ndk-build clean` fails:
* This is a bug in ndk **r8e**.
   * How to fix it:
      * Edit the file $(ndk_dir)build/core/build-binary.mk.
      * Change the line
      <pre>
$(cleantarget): PRIVATE_CLEAN_FILES := ($(my)OBJS)
**to**
$(cleantarget): PRIVATE_CLEAN_FILES := **$**($(my)OBJS)
</pre>
         * after that it should work correctly
* it is important to use `ndk-build clean` before making major configuration changes
   * avoids something in the project getting `confused` ...
---

2013-12-05

---

