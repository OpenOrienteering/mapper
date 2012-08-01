#
#    Copyright 2012 Kai Pastor
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
 
message("-- Looking for clipper library")

if(NOT CLIPPER_REVISION)
	set(CLIPPER_REVISION 251 CACHE STRING
	"SVN revision number of the clipper library to be used, or 'HEAD'"
	FORCE)
endif(NOT CLIPPER_REVISION)

set(CLIPPER_REPO "http://polyclipping.svn.sourceforge.net/viewvc/polyclipping/trunk")
if(CLIPPER_REVISION)
	set(CLIPPER_REVISION_QUERY "?revision=${CLIPPER_REVISION}")
else(CLIPPER_REVISION)
	set(CLIPPER_REVISION_QUERY "")
endif(CLIPPER_REVISION)
set(CLIPPER_FILES
  README
  License.txt
  cpp/clipper.cpp
  cpp/clipper.hpp
)
if(NOT CMAKE_CURRENT_LIST_DIR)
	# CMAKE_CURRENT_LIST_DIR is not available before cmake 2.8.3
	get_filename_component(CMAKE_CURRENT_LIST_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
endif()
set(CLIPPER_DIR "${CMAKE_CURRENT_LIST_DIR}/clipper")

foreach(CLIPPER_FILE ${CLIPPER_FILES})
  if(NOT EXISTS "${CLIPPER_DIR}/${CLIPPER_FILE}")
    message("   Downloading ${CLIPPER_FILE}")
    file(DOWNLOAD
      "${CLIPPER_REPO}/${CLIPPER_FILE}${CLIPPER_REVISION_QUERY}"
      "${CLIPPER_DIR}/${CLIPPER_FILE}"
      TIMEOUT 30
      STATUS DOWNLOAD_STATUS
    )
    message("   Downloading ${CLIPPER_FILE} - ${DOWNLOAD_STATUS}")
	list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS)
    if(NOT ${DOWNLOAD_STATUS} EQUAL 0)
      file(REMOVE "${CLIPPER_DIR}/${CLIPPER_FILE}")
      message(FATAL_ERROR "-- Looking for clipper library - not found")
    endif(NOT ${DOWNLOAD_STATUS} EQUAL 0)
  endif(NOT EXISTS "${CLIPPER_DIR}/${CLIPPER_FILE}")
endforeach(CLIPPER_FILE ${CLIPPER_FILES})

message("-- Looking for clipper library - found")

