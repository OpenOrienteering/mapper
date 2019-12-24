cmake_minimum_required(VERSION 3.5)

project(Bezier VERSION 5.14.0 LANGUAGES CXX)

find_package(Qt5Gui REQUIRED)

add_library(bezier STATIC src/gui/painting/bezier.cpp)
target_link_libraries(bezier Qt5::Gui)
