#!/bin/sh -e
#
#    Copyright 2014 Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.

NDK_DIR="$1"
if [ -z "$NDK_DIR" -o ! -d "$NDK_DIR" ]
then
	echo "Usage:"
	echo "   $0 /PATH/TO/NDK-DIR"
	exit 1
fi

PATCHES_DIR=$NDK_DIR/build/tools/toolchain-patches/gcc
if [ ! -d "$PATCHES_DIR" ]
then
	echo "Error: No build/tools/toolchain-patches/gcc in $NDK_DIR"
	exit 2
fi

mkdir -p src/build && touch src/build/configure

export GIT_DIR=$(pwd)/gcc/.git
if [ ! -d "$GIT_DIR" ]
then
	echo "Creating local git repository..."
	git clone --no-checkout https://android.googlesource.com/toolchain/gcc.git
else
	echo "Updating local git repository..."
	git fetch
fi

REVISION=`git rev-list -n 1 --until="2013-12-31" origin -- gcc-4.8`
echo $REVISION > REVISION
echo "Checking out revision $REVISION..."
(
	mkdir -p src/gcc &&
	cd src/gcc &&
	git checkout -f $REVISION -- \
	  gcc-4.6/gcc/config/linux*.h \
	  gcc-4.6/gcc/gthr-posix.h \
	  gcc-4.6/libstdc++-v3/src/Makefile.in \
	  gcc-4.8/ChangeLog* \
	  gcc-4.8/config* \
	  gcc-4.8/COPYING* \
	  gcc-4.8/gcc/BASE-VER \
	  gcc-4.8/gcc/DATESTAMP \
	  gcc-4.8/gcc/config/linux*.h \
	  gcc-4.8/include \
	  gcc-4.8/install-sh \
	  gcc-4.8/LAST_UPDATED* \
	  gcc-4.8/libgcc \
	  gcc-4.8/libiberty \
	  gcc-4.8/libstdc++-v3 \
	  gcc-4.8/libtool* \
	  gcc-4.8/lt* \
	  gcc-4.8/MAINTAINERS* \
	  gcc-4.8/mkinstalldirs \
	  gcc-4.8/NEWS* \
	  gcc-4.8/README*
)

echo "Copying build scripts and patches..."

NDK_SCRIPTS=" \
  build/tools/build-gnu-libstdc++.sh \
  build/tools/dev-defaults.sh \
  build/tools/ndk-common.sh \
  build/tools/prebuilt-common.sh "

mkdir -p build/tools
for I in $NDK_SCRIPTS
do
	cp "$NDK_DIR/$I" build/tools/
done

PATCHES=$(find "$PATCHES_DIR" -name '*.patch' | sort)
if [ -n "$PATCHES" ]
then
	mkdir -p build/tools/toolchain-patches/gcc
	for PATCH in $PATCHES
	do
		cp "$PATCH" build/tools/toolchain-patches/gcc/
	done
fi

echo "Applying patches..."

CHANGE_FILE=CHANGES.txt
echo "*** CHANGE NOTICE: ***" > $CHANGE_FILE

echo "\n$(date +%F) OpenOrienteering (http://www.openorienteering.org)" >> $CHANGE_FILE
echo "           Patches from OpenOrienteering:\n" >> $CHANGE_FILE
BUILD_TOOLS_PATCH=build-tools.patch
echo "*** $BUILD_TOOLS_PATCH" | tee -a $CHANGE_FILE
patch -p1 < $BUILD_TOOLS_PATCH | tee -a $CHANGE_FILE

echo "\n$(date +%F) OpenOrienteering (http://www.openorienteering.org)" >> $CHANGE_FILE
echo "           Patches from build/tools/toolchain-patches/gcc applied to src/:\n" >> $CHANGE_FILE
PATCHES=$(find "$(pwd)/build/tools/toolchain-patches/gcc" -name '*.patch' | sort)
if [ -n "$PATCHES" ]
then
	for PATCH in $PATCHES; do
		echo "*** $(basename $PATCH)"
		( cd src/gcc && patch -p1 < $PATCH ) || exit 1
	done  | tee -a $CHANGE_FILE
fi

echo "Cleanup..."
for I in src/gcc/gcc-4.6 src/gcc/gcc-4.8/lto-plugin src/gcc/gcc-4.8/libgcc/config/*
do
	case $(basename "$I") in
	arm|mips|i386)
		;; 
	*)
		if [ -d "$I" ]
		then
			rm -R "$I"
		fi
		;;
	esac
done
