include(AddFileDependencies)

project(Mapper)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include(3rd-party/clipper.cmake)
include(3rd-party/proj.cmake)

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

set(OPT_INCLUDES "/opt/local/include")
set(OPT_LIBRARIES "/opt/local/lib")

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${PROJ_INCLUDES} ${OPT_INCLUDES})
link_directories(${PROJ_LIBRARIES} ${OPT_LIBRARIES})

set(Mapper_SRCS
 libocad/types.c
 libocad/array.c
 libocad/geometry.c
 libocad/path.c
 libocad/file.c
 libocad/color.c
 libocad/setup.c
 libocad/ocad_symbol.c
 libocad/ocad_object.c
 libocad/string.c

 qtsingleapplication/qtlocalpeer.cpp
 qtsingleapplication/qtsingleapplication.cpp
 qtsingleapplication/qtsinglecoreapplication.cpp

 3rd-party/clipper/cpp/clipper.cpp

 src/main.cpp
 src/util.cpp
 src/undo.cpp
 src/qbezier.cpp
 src/matrix.cpp

 src/main_window.cpp
 src/main_window_home_screen.cpp
 src/print_dock_widget.cpp
 src/settings.cpp
 src/settings_dialog.cpp

 src/map.cpp
 src/map_widget.cpp
 src/map_editor.cpp
 src/map_undo.cpp
 src/map_dialog_new.cpp
 src/map_dialog_scale.cpp
 src/georeferencing.cpp
 src/georeferencing_dialog.cpp

 src/map_color.cpp
 src/color_dock_widget.cpp

 src/symbol.cpp
 src/symbol_dock_widget.cpp
 src/symbol_setting_dialog.cpp
 src/symbol_properties_widget.cpp
 src/symbol_point_editor.cpp
 src/symbol_point.cpp
 src/symbol_line.cpp
 src/symbol_area.cpp
 src/symbol_text.cpp
 src/symbol_combined.cpp
 src/renderable.cpp
 src/renderable_implementation.cpp
 src/object.cpp
 src/object_text.cpp
 src/path_coord.cpp

 src/template.cpp
 src/template_image.cpp
 src/template_gps.cpp
 src/template_map.cpp
 src/template_dock_widget.cpp
 src/template_position_dock_widget.cpp
 src/template_adjust.cpp
 src/template_tool_move.cpp
 src/template_tool_paint.cpp

 src/tool_edit.cpp
 src/tool_draw_line_and_area.cpp
 src/tool_draw_point.cpp
 src/tool_draw_path.cpp
 src/tool_draw_circle.cpp
 src/tool_draw_rectangle.cpp
 src/tool_draw_text.cpp
 src/tool_cut.cpp
 src/tool_cut_hole.cpp
 src/tool_rotate.cpp
 src/tool_scale.cpp
 src/tool_measure.cpp
 src/tool_boolean.cpp

 src/gps_coordinates.cpp
 src/gps_track.cpp
 src/dxfparser.cpp

 src/file_format.cpp
 src/file_format_native.cpp
 src/file_format_ocad8.cpp
 src/file_format_xml.cpp
)

set(Mapper_MOC_INPUT
 qtsingleapplication/qtlocalpeer.h
 qtsingleapplication/qtsingleapplication.h
 qtsingleapplication/qtsinglecoreapplication.h

 src/color_dock_widget.h
 src/georeferencing.h
 src/georeferencing_dialog.h
 src/main_window.h
 src/main_window_home_screen.h
 src/map.h
 src/map_color.h
 src/map_dialog_new.h
 src/map_dialog_scale.h
 src/map_editor.h
 src/map_undo.h
 src/map_widget.h
 src/print_dock_widget.h
 src/settings.h
 src/settings_dialog.h
 src/symbol.h
 src/symbol_area.h
 src/symbol_combined.h
 src/symbol_dock_widget.h
 src/symbol_line.h
 src/symbol_point.h
 src/symbol_point_editor.h
 src/symbol_properties_widget.h
 src/symbol_setting_dialog.h
 src/symbol_text.h
 src/template.h
 src/template_adjust.h
 src/template_dock_widget.h
 src/template_gps.h
 src/template_image.h
 src/template_map.h
 src/template_position_dock_widget.h
 src/template_tool_move.h
 src/template_tool_paint.h
 src/tool_cut.h
 src/tool_cut_hole.h
 src/tool_draw_circle.h
 src/tool_draw_line_and_area.h
 src/tool_draw_path.h
 src/tool_draw_point.h
 src/tool_draw_rectangle.h
 src/tool_draw_text.h
 src/tool_edit.h
 src/tool_measure.h
 src/tool_rotate.h
 src/tool_scale.h
 src/undo.h
)

qt4_wrap_cpp(Mapper_MOC ${Mapper_MOC_INPUT})

set(Mapper_TRANS
 translations/OpenOrienteering_de.ts
 translations/OpenOrienteering_ja.ts
 translations/OpenOrienteering_sv.ts
 translations/OpenOrienteering_uk.ts
)
qt4_add_translation(Mapper_QM ${Mapper_TRANS})

set(Mapper_RESOURCES resources.qrc translations.qrc)
qt4_add_resources(Mapper_RESOURCES_RCC ${Mapper_RESOURCES} OPTIONS -no-compress)

set(Mapper_HELP_DIR ${PROJECT_SOURCE_DIR}/bin/help)
set(Mapper_HELP_COLLECTION ${Mapper_HELP_DIR}/oomaphelpcollection.qhc)
file(GLOB Mapper_HELP_EN ${Mapper_HELP_DIR}/html_en)
add_custom_command( 
	OUTPUT    ${Mapper_HELP_COLLECTION}
	COMMAND   qcollectiongenerator
	ARGS      ${PROJECT_SOURCE_DIR}/oomaphelpcollection.qhcp -o ${Mapper_HELP_COLLECTION}
	DEPENDS   ${PROJECT_SOURCE_DIR}/oomaphelpcollection.qhcp ${PROJECT_SOURCE_DIR}/oomaphelp.qhp ${Mapper_HELP_EN}
	COMMENT   "Compile help collection file"
)

if(MINGW)
	# resource compilation for MinGW
	set(CMAKE_WINDRES windres)
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.o
			COMMAND ${CMAKE_WINDRES} -I${CMAKE_SOURCE_DIR} -i${CMAKE_SOURCE_DIR}/resources/resources.rc
			-o ${CMAKE_CURRENT_BINARY_DIR}/resources.o )
	set(Mapper_SRCS ${Mapper_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/resources.o)
endif(MINGW)

add_executable(Mapper
	WIN32
	MACOSX_BUNDLE
	${Mapper_SRCS}
	${Mapper_MOC}
	${Mapper_QM}
	${Mapper_HELP_COLLECTION}
	${Mapper_RESOURCES_RCC}
)

target_link_libraries(Mapper
 ${QT_QTCORE_LIBRARY}
 ${QT_QTGUI_LIBRARY}
 ${QT_QTNETWORK_LIBRARY}
 ${QT_QTXML_LIBRARY}
 ${PROJ_LIBRARY}
)

include(CPackaging.cmake)
