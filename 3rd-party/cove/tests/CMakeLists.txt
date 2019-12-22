cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# project(CoveTests)

find_package(Qt5Core ${Qt5Core_VERSION} REQUIRED)
find_package(Qt5Gui ${Qt5Core_VERSION} REQUIRED)
find_package(Qt5Test ${Qt5Core_VERSION} REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)

add_executable(cove-PolygonTest
    PolygonTest.cpp
)
target_link_libraries(cove-PolygonTest
  PRIVATE
    cove-vectorizer
    Qt5::Test
    Qt5::Gui
)
