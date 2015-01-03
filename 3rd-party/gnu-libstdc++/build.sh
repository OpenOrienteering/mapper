#!/bin/bash -e
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
	echo "$0: Error: missing parameter" >&2
	echo "Usage:" >&2
	echo "   $0 /PATH/TO/NDK-DIR" >&2
	exit 1
fi

if [ -d "$NDK_DIR/sources/cxx-stl/gnu-libstdc++/4.8" ]
then
	echo -n "$0: Error: Before running $0, you must (backup! and) remove " >&2
	echo "$NDK_DIR/sources/cxx-stl/gnu-libstdc++/4.8." >&2
	exit 3
fi

shift

TRY64=
if [ -d "$NDK_DIR/toolchains/x86-4.8/prebuilt/linux-x86_64" ]
then
	TRY64=--try-64
fi

echo "Building GNU libstdc++..."
ANDROID_NDK_ROOT="$NDK_DIR" bash "$(pwd)/build/tools/build-gnu-libstdc++.sh" "$(pwd)/src" --gcc-version-list=4.8 --abis=armeabi-v7a,x86 --ndk-dir="$NDK_DIR" $TRY64 $*

echo "Creating source code archiv..."
ARCHIVE_NAME=$(basename "$(pwd)")
( cd .. && tar czf "$ARCHIVE_NAME.tgz" --exclude="$ARCHIVE_NAME/gcc" --exclude="*~"  "$ARCHIVE_NAME" )

echo "*** $ARCHIVE_NAME.tgz: done"
