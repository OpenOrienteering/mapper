project(Mapper)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Debug)


if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-Wno-long-long -Wno-deprecated -Wall -pedantic")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
	set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O2 -s")
	set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG")

	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
	set(CMAKE_C_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_RELEASE})
	set(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
else (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG")
endif (CMAKE_COMPILER_IS_GNUCXX)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(Mapper_SRCS
 libocad/types.c
 libocad/array.c
 libocad/geometry.c
 libocad/path.c
 libocad/file.c
 libocad/color.c
 libocad/setup.c
 libocad/symbol.c
 libocad/object.c
 libocad/template.c

 qtsingleapplication/qtlocalpeer.cpp
 qtsingleapplication/qtsingleapplication.cpp

 src/main.cpp
 src/util.cpp
 src/undo.cpp
 src/qbezier.cpp
 src/matrix.cpp

 src/main_window.cpp
 src/main_window_home_screen.cpp
 src/print_dock_widget.cpp

 src/map.cpp
 src/map_widget.cpp
 src/map_editor.cpp
 src/map_undo.cpp
 src/map_dialog_new.cpp
 src/map_dialog_scale.cpp

 src/map_color.cpp
 src/color_dock_widget.cpp

 src/symbol.cpp
 src/symbol_dock_widget.cpp
 src/symbol_setting_dialog.cpp
 src/symbol_point_editor.cpp
 src/symbol_point.cpp
 src/symbol_line.cpp
 src/symbol_area.cpp
 src/symbol_text.cpp
 src/symbol_combined.cpp
 src/renderable.cpp
 src/object.cpp
 src/object_text.cpp
 src/path_coord.cpp

 src/template.cpp
 src/template_image.cpp
 src/template_gps.cpp
 src/template_map.cpp
 src/template_dock_widget.cpp
 src/template_position_dock_widget.cpp
 src/template_move_tool.cpp
 src/georeferencing.cpp
 src/paint_on_template.cpp

 src/tool_edit.cpp
 src/tool_draw_point.cpp
 src/tool_draw_path.cpp
 src/tool_draw_text.cpp
 src/tool_cut.cpp
 src/tool_cut_hole.cpp
 src/tool_rotate.cpp
 src/tool_measure.cpp

 src/gps_coordinates.cpp
 src/gps_track.cpp 

 src/file_format.cpp
 src/file_format_native.cpp
 src/file_format_ocad8.cpp
 src/file_format_xml.cpp
)
qt4_automoc(${Mapper_SRCS})

set(Mapper_TRANS
 translations/OpenOrienteering_de.ts
)
qt4_add_translation(Mapper_QM ${Mapper_TRANS})

SET(Mapper_RESOURCES resources.qrc translations.qrc)
qt4_add_resources(Mapper_RESOURCES_RCC ${Mapper_RESOURCES} OPTIONS -no-compress)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

IF(MINGW)
	# resource compilation for MinGW
	SET(CMAKE_WINDRES windres)
	ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.o
			COMMAND ${CMAKE_WINDRES} -I${CMAKE_SOURCE_DIR} -i${CMAKE_SOURCE_DIR}/resources/resources.rc
			-o ${CMAKE_CURRENT_BINARY_DIR}/resources.o )
	SET(Mapper_SRCS ${Mapper_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/resources.o)
ENDIF(MINGW)

add_executable(Mapper
	WIN32
	MACOSX_BUNDLE
	${Mapper_SRCS}
	${Mapper_QM}
	${Mapper_RESOURCES_RCC}
)

target_link_libraries(Mapper
 ${QT_QTCORE_LIBRARY}
 ${QT_QTGUI_LIBRARY}
 ${QT_QTNETWORK_LIBRARY}
 ${QT_QTXML_LIBRARY}
)

include(CPackaging.cmake)