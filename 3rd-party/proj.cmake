set(PROJ_LIBRARY "proj")

if(WIN32)
	add_library(${PROJ_LIBRARY} SHARED IMPORTED)
	set(PROJ_VERSION 4.8.0)
	set(PROJ_DIR "${CMAKE_CURRENT_LIST_DIR}/proj/build")
	set(PROJ_INCLUDES "${PROJ_DIR}/include")
	set(PROJ_LIBRARIES "${PROJ_DIR}/lib")
	set_target_properties(${PROJ_LIBRARY} PROPERTIES
	  IMPORTED_IMPLIB ${PROJ_DIR}/lib/libproj.dll.a
	  IMPORTED_LOCATION ${PROJ_DIR}/bin
	)
	add_custom_target(COPY_PROJ_DLL
	  COMMENT Copying libproj-0.dll to executable directory
	  COMMAND ${CMAKE_COMMAND} -E copy_if_different
	  "${PROJ_DIR}/bin/libproj-0.dll"
	  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libproj-0.dll"
	)
	add_dependencies(${PROJ_LIBRARY} COPY_PROJ_DLL)

	SET(PROJ_MSYS_BASH "C:/MinGW/msys/1.0/bin/bash.exe")
	if(EXISTS "${PROJ_MSYS_BASH}")
		include(ExternalProject)
		ExternalProject_Add(
		  proj_external
		  URL http://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz
		  SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/proj"
		  BUILD_IN_SOURCE 1
		  UPDATE_COMMAND ""
		  CONFIGURE_COMMAND "${PROJ_MSYS_BASH}" -c "pushd . && . /etc/profile && popd && ./configure --without-mutex --prefix=\"$PWD\"/build"
		  BUILD_COMMAND "${PROJ_MSYS_BASH}" -c "pushd . && . /etc/profile && popd && make"
		  INSTALL_COMMAND "${PROJ_MSYS_BASH}" -c "pushd . && . /etc/profile && popd && make install"
		)
		add_dependencies(COPY_PROJ_DLL proj_external )
	endif(EXISTS "${PROJ_MSYS_BASH}")
endif(WIN32)
