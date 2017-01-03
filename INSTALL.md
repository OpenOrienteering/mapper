## General

This document is about building OpenOrienteering Mapper from source code. 

The general build process prerequisites are:
 - A supported platform: 
   - Mac OS X 10.10 (Yosemite). Mac OS X 10.7 or newer may work.
   - Linux. Ubuntu 14.04 is known to work.
 - The OpenOrienteering Mapper source code.
 - CMake >= 2.8.12.
   CMake is available from http://www.cmake.org/.
 - A supported C++ compiler toolchain. C++11 is mandatory.

See below for platform-specific details and build instructions.


## Qt Library

This program uses the Qt library from http://www.qt.io which is released
under the terms of the GNU General Public License, Version 3. 
The program needs at least Qt 5.2. Qt 5.5.1 is the recommended version.
By default CMake build settings for OS X and Windows,
a special OpenOrienteering source package of Qt 5.5.1 will be downloaded
and built as part of the build process.
You can change the cmake option `Mapper_BUILD_QT` to adjust this.


## Clipper Library

The program uses the clipper code library from 
http://www.angusj.com/delphi/clipper.php /
http://sourceforge.net/projects/polyclipping/
which is released under the terms of the Boost Software License, Version 1.0.
The program is known to work with release 6.1.3a and not compile with releases
5.x.x or earlier. By default build settings, clipper 6.1.3a will be downloaded
and built as part of the build process. 
You can change the cmake option `Mapper_BUILD_CLIPPER` to adjust this.


## PROJ.4 Cartographic Projections Library

The program uses the PROJ.4 Cartographic Projections Library from
http://trac.osgeo.org/proj/ which is released under permissive license terms.
The program is known to work with the releases 4.8.0 (as contained in Ubuntu
14.04), 4.9.1 and 4.9.2. By default build settings, proj 4.9.2 will be
downloaded and built as part of the build process. On Linux, default settings
will use the installed proj library.
You can change the cmake option `Mapper_BUILD_PROJ` to adjust this.


## Getting the Source

Download a zip or tar.gz Source code archive from

https://github.com/OpenOrienteering/mapper/releases

and unpack it, or checkout the source code with git:


```
git clone https://github.com/OpenOrienteering/mapper.git
```

The directory created by unpacking or checkout will be referred to as the
source directory (`SOURCE_DIR`).


## Compiling on Linux

The standard g++ compiler from a recent distribution should work. The Ubuntu
14.04 g++ is known to work. Make sure that the development packages of the
PROJ.4 library and of CUPS are installed. For a Ubuntu or Debian system, install
`libproj-dev` and `libcups2-dev`.

For using the Qt libraries provided by the repositories, the following packages
need to be installed:
`qt5-default` (>= 5.2), `qtbase5-private-dev`, `qttools5-dev`, `qttools5-dev-tools`, `libqt5sql5-sqlite`

Open a terminal, and create a build directory, e.g. as subdirectory build in
the source directory, and change to that directory. From the build directory,
configure and build like this:

```
cmake PATH/TO/SOURCE_DIR
```

Now you may start the build process by running

```
make
```


## Compiling on OS X

The supported compiler is clang from the Command Line Tools for Xcode.
XCode 7.0.1 and the accompanying command line tools are known to work.

Now create a build directory, e.g. as subdirectory build in the source
directory, and change to that directory. In the build directory, configure
and build like this:

```
cmake PATH/TO/SOURCE_DIR
```

Now you may start the build process by running

```
make
```

and generate a DragNDrop package by

```
make package
```


## Cross-Compiling on Linux for Windows

Some linux distributions may offer cross compiler packages based on MinGW or
MinGW-w64 (for Ubuntu: g++-mingw-w64 et al.), and even NSIS/makensis is
available. MXE.cc provides an alternative cross-compilation environment

For MinGW-w64 i686 (32 bit) builds, configure the build with

```
cmake PATH/TO/SOURCE_DIR -DCMAKE_TOOLCHAIN_FILE=PATH/TO/SOURCE_DIR/cmake/toolchain/i686-w64-mingw32
```

and proceed with

```
make
```


## Cross-Compiling on Linux for Android

In addition to the general build process prerequisites, you need:
 - the Android SDK
 - the Android NDK
 - Qt 5.5.1 for Android which includes Qt Creator

When downloading packages for specific Android API levels,
keep in mind that Qt5 requires at least API Level 9 to work, and apps
built for the armeabi-v7a ABI require at least an emulator image of API
level 14 to work with the emulator.

The build of OpenOrienteering Mapper for Android is controlled by qmake and the
oo-mapper.pro file. The easist way to do the build is using Qt Creator where you
can define different "kits" representing different types of target devices. 
Qt Creator takes care of switching between debug and release configurations.
For setting up the kits, see the Qt Creator documentation:

http://doc.qt.io/qtcreator/creator-targets.html

Mapper's dependencies, such as the PROJ.4 library, are build by cmake, based on
the exisiting CMakeList.txt files and Qt Creator's kit settings.

For development, you may use the Android Qt kits which come with Qt Creator.
For official releases, you need to build the Qt libraries from the sources.
This can be done by loading 3rd-party/qt5/qt5.pro in Qt Creator, and building
it using a kit for Android. The resulting Qt kit can be added to Qt Creator
as another Qt version via  Tools > Options... > Build & Run > Qt Versions.
Qt Creator will automatically create a kit for this, and then you can use this
kit to build official OpenOrienteering releases.
For general Qt5 for Android build instructions, see:

http://qt-project.org/wiki/Qt5ForAndroidBuilding


## Binary Package Distribution

Distributing binary code under open source licenses creates certain legal
obligations, such as the distribution of the corresponding source code and
build instructions for GPL licensed binaries. This is how to do this correctly
for Mapper.

Official release packages need to be build from an official source code archive.
This is to make sure that every user can indeed get all required sources for the
binary release.

Release packages will include the Qt libraries and the Qt assistant executable.
That is why release packages need to build the Qt components from the Qt source
code in the particular version which we distribute along with our source. The
Mapper default build configuration will normally take care of this.

The same principle applies to the other third party components (PROJ.4,
Clipper). If the compiler requires distribution of additional libraries, such
as a libstdc++ DLL, these binaries' licenses must be respected as well, which
may add additional source code and build instruction distribution requirements.

The build shall start from a fresh build directory. The build shall be in
release mode, i.e. with `-DCMAKE_BUILD_TYPE=Release`. Synopsis:

```
tar xvzf openorienteering-mapper_X.Y.Z-src.tgz
cd openorienteering-mapper_X.Y.Z
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

The actual package may be built by `make package`. However, this is used
(i.e. tested) only for OS X at the moment.
Android packages are built in Qt Creator using the provided `oo-mapper.pro` file.
Windows and Linux packages are regularly build on
https://build.opensuse.org/project/show/home:dg0yt, so proper package recipes for
common distributions and package managers can be found there.

Packaging with cmake is not enabled by default for Linux. Add `-DMapper_BUILD_PACKAGE=1`
to change this. As already stated, this is untested, and you might need to look for
additional tweaks.


