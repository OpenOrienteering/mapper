cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# project(CoveTests)

find_package(Qt5Core ${Qt5Core_VERSION} REQUIRED)
find_package(Qt5Gui ${Qt5Core_VERSION} REQUIRED)
find_package(Qt5Test ${Qt5Core_VERSION} REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)

configure_file(test_config.h.in test_config.h)
add_library(cove-test-config INTERFACE)
target_include_directories(cove-test-config INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(cove-ParallelImageProcessingTest
  ParallelImageProcessingTest.cpp
)
add_test(
  NAME cove-ParallelImageProcessingTest
  COMMAND cove-ParallelImageProcessingTest
)
if(${CMAKE_VERSION} VERSION_LESS 3.8)
	target_include_directories(cove-ParallelImageProcessingTest PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endif()

add_executable(cove-PolygonTest
  PolygonTest.cpp
)
target_link_libraries(cove-PolygonTest
  PRIVATE cove-test-config
)
add_test(
  NAME cove-PolygonTest
  COMMAND cove-PolygonTest
)

add_executable(cove-PolygonBenchmark EXCLUDE_FROM_ALL
  PolygonTest.cpp
)
target_compile_definitions(cove-PolygonBenchmark
  PRIVATE
    COVE_BENCHMARK=QBENCHMARK
)

foreach(target
  cove-ParallelImageProcessingTest
  cove-PolygonTest
  cove-PolygonBenchmark
)
	target_link_libraries(${target}
	  PRIVATE
	    cove-vectorizer
	    Qt5::Test
	    Qt5::Gui
	)
endforeach()

