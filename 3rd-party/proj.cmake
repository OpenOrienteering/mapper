set(PROJ_LIBRARY "proj")

if(WIN32)
	add_library(${PROJ_LIBRARY} SHARED IMPORTED)
	set(PROJ_VERSION 4.8.0)
	set(PROJ_DIR "${CMAKE_CURRENT_LIST_DIR}/proj-${PROJ_VERSION}/build")
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
endif(WIN32)
