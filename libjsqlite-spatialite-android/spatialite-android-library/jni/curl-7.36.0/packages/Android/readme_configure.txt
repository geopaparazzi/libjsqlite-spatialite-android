
https://groups.google.com/forum/#!searchin/android-ndk/libcurl/android-ndk/lB2HBeznz3U/AcERxoMv5LgJ
2010
Android native support for external libraries (an example using curl and Android 1.6)

Hi
I needed curl support native in Android and trying to do it found that
the existing help available was rather pitiful. There appear to be
many people trying this and while a few claim success it looks like
many more are just giving up.
So here is my story in the hope that it will help someone else. I
finely managed to get curl in and  spurred by that success tried (and
succeeded) adding json-c. So it is distinctly possible; don’t give up!

First thing first I would like to say that the 3 days of my life I
spent on this I am never getting back. Guys from Google who might be
reading this: would it kill you to either bake more useful libraries
into the standard build? Curl is not exactly a novelty.

Anyways if you want to add a lib these are the steps. In this case we
are using Android 1.6 and curl 7.20 building on Ubuntu

1) Get the source code and drop it into a dir under /external. In our
case /external/curl
2) In the case of curl you will find a helpful Android.mk file under
the source with some nice instructions (G-D bless your soul Dan
Fandrich for this one). (if whatever you are trying to add does not
have an Android.mk don't despair. We will get to how to create your
own in a few steps.)
3) Now comes the first hard part: you need to configure the package
using the cross compiler.
Change to the external/curl dir and run ./configure just to make sure
the dame thing runs before we do anything special. Assuming that
worked now lets set all the special environment vars.
These are the ones that worked for me:
//the dir where your android source is
export A=~/mydroid

//the path to the cross compiler
export CC=$A/prebuilt/linux-x86/toolchain/arm-eabi-4.2.1/bin/arm-eabi-
gcc

//the path to the ndk
export SYSROOT=$A/development/ndk/build/platforms/android-4/arch-arm

export CFLAGS="-march=armv5te -mtune=xscale -msoft-float -mandroid -
fPIC -mthumb-interwork -mthumb -mlong-calls -ffunction-sections -
fstack-protector -fno-short-enums -fomit-frame-pointer -fno-strict-
aliasing -finline-limit=64 -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -
D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -DANDROID -DOS_ANDROID -D__NEW__ -
D__SGI_STL_INTERNAL_PAIR_H -I$SYSROOT/usr/include"

export LDFLAGS="-L$SYSROOT/usr/lib -Wl,--gc-sections -nostdlib  -lc -
lm -ldl -llog -lgcc -Wl,--no-undefined,-z,nocopyreloc -Wl,-dynamic-
linker,/system/bin/linker "

./configure --host=arm-eabi
NOTE: in my case I wanted a static lib and I specifically did not want
docs or zip support so I added these 2 flags to the configure command:
--without-zlib --disable-manual

A quick note about SYSROOT. My Ubuntu make does not like having a
SYSROOT param passed in the LDFLAGS so I took it out. Yours may
require it in which case add “—sysroot=$SYSROOT”

When you run ./configure you will probably get some error telling you
to check the config.log. Do that. Find the error and take it from
there. Nobody ever said this would be easy. If your configure is
suspiciously successful check the output to make sure its using the
cross compiler. You should see that long path you put into CC in the
output. If you don’t then it didn’t catch and go check your
environment variables.

I will now quote from Dan Fandrich’s Android.mk file as to how to come
up with these parameters. A trick you may have to use if the above
don’t quite work right:
“Perform a normal Android build with libcurl in the source tree,
providing the target "showcommands" to make. The build will eventually
fail (because curl_config.h doesn't exist yet), but the compiler
commands used to build curl will be shown. Now, from the external/
curl/ directory, run curl's normal configure command with flags that
match what Android itself uses. This will mean putting the compiler
directory into the PATH, putting the -I, -isystem and -D options into
CPPFLAGS, putting the -m, -f, -O and -nostdlib options into CFLAGS,
and putting the -Wl, -L and -l options into LIBS, along with the path
to the files libgcc.a, crtbegin_dynamic.o, and ccrtend_android.o.
Remember that the paths must be absolute since you will not be running
configure from the same directory as the Android make.”

In general it is hard to overemphasis how useful a trick “make
showcommands”.

4) Now the second hard part: make
Go to your android root and run make curl. You want to “make curl”
because that will catch errors and run way way way faster then a
general make.
You WILL get errors. And here comes the really dirty part. Go find the
errors and “fix” them. Yes you will have to change some code. No I am
not going to post the code that I changed, mostly because I changed it
at 3AM and I don’t remember it all, but also because case is a little
different (I disabled allot of stuff that I didn’t need/want) and once
you know you need to do this its fairly straight forward.
Here however are some examples:
a)        configure uses zlib to compress the manual it builds into
hugehelp.c. Our compiler does not like the resulting very large array.
In my case I disabled the manual and zlib so this went away.
b)        Main.c has a call to a function called getpass in line 1542. This
fails compile for some reason and I removed it (again in my case this
made sense. In yours you might need this function).
c)        url.c has a declaration checking for HAVE_SOCKET. For some reason
this declaration fails even though socket support is most certainly
there. In my case I simply removed the test.
d)        Etc.

Note: this is all assuming that you have an Android.mk file in under /
external/curl (which in the case of curl you do). If you are using
these instructions to compile a lib that does not just make your own.
I personally used the Android.mk file that Dan Fandrich was nice
enough to provide for curl as a basis for the Android.mk file I used
for json-c. check here http://blog.chinaunix.net/u/8059/showart_1420446.html
for a useful guide if you are not an expert in Android build files
(which I am not).

When all is said and done you should see your new library in out/
target/product/generic/obj/STATIC_LIBRARIES/ or out/target/product/
generic/obj/SHARED_LIBRARIES/ depending on if you compiled it as a
static or shared lib.

If its there, you are done with this step.

5) Now the part that should be simple but for some strange reason is
not: Linking
You need to add the new lib to the make files in the local code where
you want to use said library.
In particular you need to add the path to the .h files to
LOCAL_C_INCLUDES and the lib module name to
LOCAL_WHOLE_STATIC_LIBRARIES. First go read http://blog.chinaunix.net/u/8059/showart_1420446.html
to get comfortable with what these parameters mean.

Now in a perfect world both parameters would be set in the Android.mk
file on whatever level you need to use them. For some reasons that I
can’t fathom it don’t work that way. Don’t ask me why. After playing
around with it for way too long this is what worked for me:
Add the header include dir to the LOCAL_C_INCLUDES (in our case
external/curl/include) in whatever level you need to use it.
In your local code add an include (ex. #include <curl/curl.h>) and see
if it compiles.

Now the harder part. Use the new library somewhere in your code and
notice that it dies in linking. You need to add the lib to the list of
linked libraries. If my case adding it to LOCAL_WHOLE_STATIC_LIBRARIES
in the local level did not work. Don’t ask me why. The easiest way to
debug this is to make with the showcommends option and see what the
linking commands are to the compiler. You want to see a –llibcurl
command. If you don’t take a look as some of the libs that are being
linked and search for them in surrounding make files. In my case I
just added curl in the same place that the rest of the linked libs
were being added.

6) Try it out!
There is always a non-zero chance that after all this when you
actually try using the lib it will fail miserable and crash. Life
sucks like that sometimes. You can try debugging it or look for a
different library.


Well hope this helps. Please feel free to add to this threads with
your own success stories and hints.


Thanks

Zeev
