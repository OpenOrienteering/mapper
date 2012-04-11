message("-- Looking for clipper library")

set(CLIPPER_REVISION revision=HEAD)
set(CLIPPER_REPO "http://polyclipping.svn.sourceforge.net/viewvc/polyclipping/trunk")
set(CLIPPER_FILES
  README
  License.txt
  LIC
  cpp/clipper.cpp
  cpp/clipper.hpp
)
set(CLIPPER_DIR "${CMAKE_CURRENT_LIST_DIR}/clipper")

foreach(CLIPPER_FILE ${CLIPPER_FILES})
  if(NOT EXISTS "${CLIPPER_DIR}/${CLIPPER_FILE}")
    message("   Downloading ${CLIPPER_FILE}")
    file(DOWNLOAD
      "${CLIPPER_REPO}/${CLIPPER_FILE}?${CLIPPER_REVISION}"
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

