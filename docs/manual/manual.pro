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

TEMPLATE = aux
CONFIG  -= debug_and_release

manual.dir      = $$OUT_PWD/manual
manual.target   = Mapper-Manual
manual.depends  = $$OUT_PWD/../../toolchain.cmake
manual.commands = @\
  mkdir -p "$$manual.dir" && \
  cd "$$manual.dir" && \
  if [ ! -f "html/index.qhp" ] ; then \
    if [ -d CMakeFiles -o -f CMakeCache.txt ] ; then rm -R CMake*; fi ; \
    cmake "$$PWD" -DCMAKE_TOOLCHAIN_FILE="$$OUT_PWD/../../toolchain.cmake" ; \
  fi && \
  $(MAKE)

QMAKE_EXTRA_TARGETS += manual
PRE_TARGETDEPS      += $$manual.target
QMAKE_CLEAN         += $$manual.target \
                       $$manual.dir/html/*
android {
	INSTALLS += manual_data
	manual_data.path = /assets/docs/manual
	manual_data.extra = $(MAKE) -C manual install DESTDIR=$(INSTALL_ROOT)
}

OTHER_FILES += \
  CMakeLists.txt \
  Doxyfile.in \
  footer.html \
  header.html \
  Manual.qhcp.in \
  postprocess-qhp.cmake.in \
  preprocess-markdown.cmake.in \
  style.css
