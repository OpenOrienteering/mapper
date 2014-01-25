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

TEMPLATE = lib
TARGET   = fakeqt5

qt5.dir      = $$OUT_PWD/qt5
qt5.version  = 5.2.0
qt5.target   = $$qt5.dir/Qt5-prefix/bin/qmake

win32 {
	qt5.target   = $$qt5.dir/Qt5-prefix/bin/qmake.exe
}
android {
	qt5.target   = $$qt5.dir/Qt5-prefix/jar/QtAndroid.jar
	qt5.platform = -xplatform android-g++ \
	               -android-ndk-platform android-11 \
	               -skip qttranslations \
	               -nomake tests \
	               -nomake examples \
	               -no-warnings-are-errors
}

qt5.commands = \
  mkdir -p "$$qt5.dir" && \
  cd "$$qt5.dir" && \
  cmake "$$PWD" -DQT5_VERSION=$$qt5.version -DQT5_PLATFORM=\"$$qt5.platform\" && \
  PATH="$$NDK_TOOLCHAIN_PATH/bin:${PATH}" $(MAKE) all && \
  $(MAKE) clean

QMAKE_EXTRA_TARGETS += qt5
PRE_TARGETDEPS      += $$qt5.target
QMAKE_CLEAN         += $$qt5.target $$qt5.dir/Qt5-prefix/src/Qt5-stamp/*
