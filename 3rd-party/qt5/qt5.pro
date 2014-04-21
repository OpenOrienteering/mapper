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

TEMPLATE = aux

OTHER_FILES = \
    CMakeLists.txt \
    qt.conf.in \
    qt.conf.qrc.in \
    qt5-config-host.in \
    qt5-config.cmd.in \
    qt5-config.in \
    qt5-make.in \
    qt5-patchqt.in

qt5.dir      = $$OUT_PWD/qt5
qt5.target   = $$qt5.dir/CMakeFiles/Qt5-complete

android {
	qt5.platform = -xplatform android-g++ \
	               -android-ndk-platform android-9 \
	               -android-arch "$$ANDROID_TARGET_ARCH" \
	               -android-toolchain-version 4.8 \
	               -skip qttranslations \
	               -no-warnings-are-errors
}

CONFIG(debug) {
	qt5.debug = -DQT5_DEBUG:BOOL=ON
} else {
	qt5.debug =
}

qt5.cmake    = \
  cmake "$$PWD" -UQT5_* -DQT5_PLATFORM=\"$$qt5.platform\" $$qt5.debug
qt5.commands = \
  cd "$$qt5.dir" && \
  $$qt5.cmake && \
  PATH="$$NDK_TOOLCHAIN_PATH/bin:${PATH}" $(MAKE) all
  
mkpath($$qt5.dir)
system(cd "$$qt5.dir" && $$qt5.cmake)

QMAKE_EXTRA_TARGETS += qt5
PRE_TARGETDEPS      += $$qt5.target
QMAKE_CLEAN         += $$qt5.target $$qt5.dir/Qt5-prefix/src/Qt5-stamp/Qt5-download
