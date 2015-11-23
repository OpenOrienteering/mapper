#!/bin/sh -e
#
#    Copyright 2015 Kai Pastor
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

PATCHES_DIR="$NDK_DIR/build/tools/toolchain-patches/gcc"
if [ ! -d "$PATCHES_DIR" ]
then
	echo "Error: No build/tools/toolchain-patches/gcc in $NDK_DIR"
	exit 2
fi

mkdir -p src/build && touch src/build/configure

#rm -Rf $(pwd)/gcc/.git
export GIT_DIR="$(pwd)/gcc/.git"
if [ ! -d "$GIT_DIR" ]
then
	echo "Creating local git repository..."
	git clone --no-checkout https://android.googlesource.com/toolchain/gcc.git
else
	echo "Updating local git repository..."
	git fetch
fi

REVISION=$(sed -e '/gcc.git/!d;s/^[^ ]* *\|([^ ]* *\| .*//g' "$NDK_DIR/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/SOURCES")
echo $REVISION > REVISION
echo "Checking out revision $REVISION..."
(
	mkdir -p src/gcc &&
	cd src/gcc &&
	git checkout -f $REVISION -- \
	  gcc-4.9/libstdc++-v3 \
	  \
	  gcc-4.9/ChangeLog* \
	  gcc-4.9/config\* \
	  gcc-4.9/COPYING* \
	  gcc-4.9/gcc/DATESTAMP \
	  gcc-4.9/include \
	  gcc-4.9/install-sh \
	  gcc-4.9/libgcc \
	  gcc-4.9/libiberty \
	  gcc-4.9/libtool* \
	  gcc-4.9/lt* \
	  gcc-4.9/MAINTAINERS* \
	  gcc-4.9/mkinstalldirs \
	  gcc-4.9/README* \
	  \
	  gcc-4.8/gcc/config/linux-android.h \
	  gcc-4.9/gcc/config/linux-android.h \
	  \
	  gcc-4.8/libgcc/gthr-posix.h \
	  gcc-4.9/libgcc/gthr-posix.h \
	  \
	  gcc-4.8/libstdc++-v3/src/Makefile.in \
	  gcc-4.9/libstdc++-v3/src/Makefile.in \
	  \
	  gcc-4.9/gcc/BASE-VER \
	  \
	  gcc-4.9/gcc/ChangeLog \
	  gcc-4.9/gcc/config/arm/arm.md \
	  gcc-4.9/gcc/testsuite/ChangeLog \
	  \
	  gcc-4.8/gcc/config/i386/arm_neon.h \
	  gcc-4.9/gcc/config/i386/arm_neon.h \
	  \
	  # End. Covers libstdc++ sources, build dependencies, and extra files touched by patches.
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
		grep -q gcc-4.9 "$PATCH" &&
		  cp "$PATCH" build/tools/toolchain-patches/gcc/
	done
fi

echo "Applying patches..."

CHANGE_FILE=CHANGES.txt
echo "*** CHANGE NOTICE: ***" > $CHANGE_FILE
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
for I in src/gcc/gcc-4.8 src/gcc/gcc-4.9/lto-plugin src/gcc/gcc-4.9/libgcc/config/*
do
	case $(basename "$I") in
	aarch64|arm|mips|i386)
		;; 
	*)
		if [ -d "$I" ]
		then
			rm -R "$I"
		fi
		;;
	esac
done
