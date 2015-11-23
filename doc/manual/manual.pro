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

CMAKE_TOOLCHAIN_FILE = $$clean_path($$OUT_PWD/../../toolchain.cmake)
if (exists($$CMAKE_TOOLCHAIN_FILE)) {
    CMAKE_ARGS      += "-DCMAKE_TOOLCHAIN_FILE=\"$$CMAKE_TOOLCHAIN_FILE\""
    manual.depends  += $$CMAKE_TOOLCHAIN_FILE
}

manual.dir      = $$OUT_PWD/manual
manual.target   = $$manual.dir/html/index.qhp
manual.commands = @\
  mkdir -p "$$manual.dir" && \
  cd "$$manual.dir" && \
  if [ ! -f "html/index.qhp" ] ; then \
    if [ -d CMakeFiles -o -f CMakeCache.txt ] ; then rm -R CMake*; fi ; \
    cmake "$$PWD" $$CMAKE_ARGS ; \
  fi && \
  $(MAKE) VERBOSE=$(VERBOSE) && \
  $(MAKE) -C "$$manual.dir" install DESTDIR=\"$$clean_path($$OUT_PWD/../..)\"

QMAKE_EXTRA_TARGETS += manual
PRE_TARGETDEPS      += $$manual.target
QMAKE_CLEAN         += $$manual.target \
                       $$manual.dir/html/* \
                       $$manual.dir/*.{qch,qhc} \
                       *.{qch,qhc}
android {
	INSTALLS += manual_data
	manual_data.path = /assets/doc/manual
	manual_data.extra = $(MAKE) -C "$$manual.dir" install DESTDIR=$(INSTALL_ROOT)
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
