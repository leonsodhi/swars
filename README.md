# swars

This is a continuation of the project created by Unavowed and Gynvael Coldwind. It also incorporates one of the fixes discovered by [mefistotelis](https://github.com/mefistotelis/swars) related to structure packing. See http://swars.vexillium.org for details.

# Building on Windows 7 (64-bit) from the original source

* Download mingw-get-setup.exe and install it to `C:\MinGW`
* In the MinGW Installation Manager, install the `mingw-developer-toolkit` package from Basic Setup
* Run `C:\MinGW\msys\1.0\msys.bat`
* `mingw-get install 'gcc=4.8.1.*'` (It's likely that a newer release would be fine but this was the version I used)
* `mingw-get install 'libz=1.2.8.*'`
* Close the `MINGW32` command prompt window
* Download and extract http://swars.vexillium.org/files/swars-0.3.tar.bz2 to `C:\projects\swars-0.3`
* Download `python-2.7.13.msi` from https://www.python.org/ and install it, ensuring that "Add python.exe to Path" is selected
  
## Installing dependencies

* Download and extract the following:
  * [SDL-1.2.14](https://www.libsdl.org/release/SDL-devel-1.2.14-mingw32.tar.gz) to `C:\projects\libs\SDL-1.2.14`
  * [OpenAL-1.17.2](http://kcat.strangesoft.net/openal-binaries/openal-soft-1.17.2-bin.zip) to `C:\projects\lib\openal-soft-1.17.2-bin`
  * [libvorbis-1.3.5](http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.5.zip) to `C:\projects\lib\libvorbis-1.3.5`
  * [libogg-1.3.2](http://downloads.xiph.org/releases/ogg/libogg-1.3.2.zip) to `C:\projects\libs\libogg-1.3.2`
  * [libpng-1.2.37](http://downloads.sourceforge.net/gnuwin32/libpng-1.2.37-bin.zip) to `C:\projects\lib\libpng-1.2.37-bin`
  * [libpng-1.2.37 developer files](http://downloads.sourceforge.net/gnuwin32/libpng-1.2.37-lib.zip) to `C:\projects\lib\libpng-1.2.37-lib`
* Build and install `libogg` as described below
* Build and install `libvorbis` as described below
* Copy the contents of `C:\projects\lib\libpng-1.2.37-bin` to `C:\mingw`
* Copy the contents of `C:\projects\libs\libpng-1.2.37-lib` to `C:\mingw`
* Edit `C:\projects\libs\SDL-1.2.14\bin\sdl-config` and change `prefix=/usr/local` to `prefix=C:/mingw`
* Copy the directories `bin`, `include`, `lib`, `man`, and `share` from `C:\projects\libs\SDL-1.2.14` to `C:\mingw`
* Copy `bin\Win32\soft_oal.dll` from `C:\projects\libs\openal-soft-1.17.2-bin` to `C:\mingw\bin`
* Copy the directory `C:\projects\libs\openal-soft-1.17.2-bin\include` to `C:\mingw`
* Copy the contents of `C:\projects\libs\openal-soft-1.17.2-bin\libs\Win32` to `C:\mingw\lib`
  
### Building & installing libogg

* Run `C:\MinGW\msys\1.0\msys.bat`
* `cd C:/projects/libs/libogg-1.3.2`
* `LDFLAGS='-mwindows' ./configure --prefix=c:/mingw --host=i686-w64-mingw32`
* `make && make install`
  
### Building & installing libvorbis

* Run `C:\MinGW\msys\1.0\msys.bat`
* `cd C:/projects/libs/libvorbis-1.3.5`
* `LDFLAGS='-mwindows' ./configure --prefix=c:/mingw --host=i686-w64-mingw32`
* `make && make install`

## Updating the Makefile

* Edit `C:\projects\swars-0.3\src\Makefile.windows`
* Change the `LIBS` variable at the top to the following:
```
LIBS	 = -lmsvcrt $(shell sdl-config --libs) -lOpenAL32 -lvorbisfile -lvorbis -logg -lpng -lz
```
* Add `-mno-ms-bitfields` to the beginning of `CFLAGS` and `CPPFLAGS`. This is required due to [this](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52991) bug related to structure packing
* Delete `$(RES)` from the `OBJ` list

## Fixing up the source

* Edit `C:\projects\swars-0.3\src\windows.config.h` and uncomment `HAVE_GETTIMEOFDAY`
* Edit `C:\projects\swars-0.3\src\util.h` and add `#include <sys/stat.h>` under `#include <stdbool.h>`
* Download the [game speed patch](https://14383785703902031330.googlegroups.com/attach/b34bd2e7b0056ebb/0001-Lower-max_fps-and-fix-overly-long-sleep-times.patch?part=0.1&view=1&vt=ANaJVrFKe14UwEqfRPlA6nPecfZKmgCnDayZjNGNbXbYhX4OdPisTi7CS9EIS6eCiWcuBYACfwdEpUAk9mLhb9_0NTMgNPeXJQSXXoxw-zQSBc2O_TPrFCk) to `C:\projects\swars-0.3`
* Run `C:\MinGW\msys\1.0\msys.bat`
* `cd c:/projects/swars-0.3`
* `patch -p1 < 0001-Lower-max_fps-and-fix-overly-long-sleep-times.patch`

## Compiling and installing

* Run `C:\MinGW\msys\1.0\msys.bat`
* `cd c:/projects/swars-0.3/src`
* `make -f Makefile.windows`
* Copy `swars.exe` from `C:\projects\swars-0.3\src` to the root of your Syndicate Wars install directory
* Copy `C:\MinGW\bin\soft_oal.dll` to the root of your Syndicate Wars install directory and rename it to `OpenAL32.dll`
* Copy the following files from `C:\MinGW\bin` to the root of your Syndicate Wars install directory
```
libpng3.dll
zlib1.dll
libgcc_s_dw2-1.dll
SDL.dll
libvorbisfile-3.dll
libvorbis-0.dll
libogg-0.dll
```

# Building on Windows 7 (64-bit) using this repo

* Follow the first section above to setup `mingw` & Python. Use the source code from this repository instead
* Install all dependencies with the exception of SDL
* Install the SDL2 development library:
  * Download and extract [SDL2-2.0.5](https://www.libsdl.org/release/SDL2-devel-2.0.5-mingw.tar.gz) to `C:\projects\libs\SDL2-2.0.5`
  * Edit `C:\projects\libs\SDL2-2.0.5\i686-w64-mingw32\bin\sdl2-config` and change the `prefix` to `prefix=C:/mingw`
  * Copy the contents of `C:\projects\libs\SDL2-2.0.5\i686-w64-mingw32` to `C:\mingw`
* Skip the makefile changes
* Skip fixing up the source
* Compile and install but copy SDL2.dll instead of SDL.dll
