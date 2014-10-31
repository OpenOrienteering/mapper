#/bin/sh -e
#
# (C) 2014 Kai Pastor
#
# This file is part of OpenOrienteering.
#
# OpenOrienteering is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OpenOrienteering is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.

LANG=C
MXE_TARGET="i686-w64-mingw32.shared x86_64-w64-mingw32.shared"
MXE_DIR=gcc-mxe-$(date +%Y%m%d)
PKG_LIST="binutils cloog gcc gcc-cloog gcc-isl gcc-gmp gcc-mpc gcc-mpfr gmp isl mingw-w64 mpc mpfr pkgconf"
PKG_LIST="$PKG_LIST gdb expat libiconv pdcurses readline zlib"

git clone -b master https://github.com/mxe/mxe.git $MXE_DIR

( cd $MXE_DIR ; git show-branch --list --reflog=1 ) > $MXE_DIR/GIT_SHOW_BRANCH

for I in $MXE_DIR/.git* $MXE_DIR/assets $MXE_DIR/doc $MXE_DIR/tools/* src/cmake
do
	echo $I
	case "$I" in
	*/tools/compat-init.sh)
		;; # keep it
	*/tools/make-shared-from-static)
		;; # keep it
	*/tools/update-*)
		;; # keep it
	*)
		rm -Rf "$I"
		;;
	esac
done

for I in $(sed  -n 's/^.* class="package">\([^<]*\)<.*$/\1/p' "$MXE_DIR/index.html")
do
	echo " $PKG_LIST " | grep " $I " > /dev/null && I=
	if [ -n "$I" ]
	then
		rm $MXE_DIR/src/$I.mk $MXE_DIR/src/$I-* 2>/dev/null || true
		sed -i -e '/<tr>/N;/^.* class="package">'"$I"'</{N;N;d}' "$MXE_DIR/index.html"
	fi
done

sed -i -e '/enable-languages/{s/,objc//;s/,fortran//};/addprefix/s/ gfortran//' $MXE_DIR/src/gcc.mk

sed -i -e "1ifirst: gcc" $MXE_DIR/Makefile
sed -i -e "s/git show-branch --list --reflog=1/echo '$MXE_DIR'/" $MXE_DIR/Makefile

cat > $MXE_DIR/settings.mk << END_SETTINGS
MXE_TARGETS := $MXE_TARGET
#JOBS := 6

END_SETTINGS

mkdir $MXE_DIR/OpenOrienteering
cp "$0" $MXE_DIR/OpenOrienteering/
cat > $MXE_DIR/README << END_README
$(LANG=C date) OpenOrienteering

This is a clone of MXE.cc modified to fulfil the sole purpose of a controlled
build of a $(echo ${MXE_TARGET} | sed -e 's/ /\//') C/C++ toolchain. 
See OpenOrienteering/make-source.sh for the modifications.

To build gcc and its dependencies, run

  make gcc

This will use existing sources which came with this package.

To build gdb, run

  make gdb

The sources of gdb and of its dependencies will be downloaded on first build.

END_README

make -C $MXE_DIR download-gcc

fakeroot tar cvzf $MXE_DIR-src.tar.gz $MXE_DIR

