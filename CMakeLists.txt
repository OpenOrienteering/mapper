project(Mapper)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-ansi -pedantic)
if (CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-Wno-long-long -Wno-deprecated -Wall)
endif (CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O2 -s")

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(Mapper_SRCS
 src/main.cpp
 src/util.cpp
 src/matrix.cpp

 src/main_window.cpp
 src/main_window_home_screen.cpp

 src/map.cpp
 src/map_dialog_new.cpp
 src/map_widget.cpp
 src/map_editor.cpp
 src/color_dock_widget.cpp

 src/template.cpp
 src/template_image.cpp
 src/template_gps.cpp
 src/template_dock_widget.cpp
 src/georeferencing.cpp
 src/paint_on_template.cpp

 src/gps_coordinates.cpp
 src/gps_track.cpp
)
qt4_automoc(${Mapper_SRCS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
add_executable(Mapper ${Mapper_SRCS})
target_link_libraries(Mapper
 ${QT_QTCORE_LIBRARY}
 ${QT_QTGUI_LIBRARY}
)
