cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# project(CoveTests)

find_package(Qt5 COMPONENTS Core Gui Test REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("../app")
include_directories("..")

add_executable("PolygonTest" "PolygonTest.cpp" "PolygonTest.h")
target_link_libraries("PolygonTest"
          cove
          Qt5::Test
          Qt5::Gui
        )
