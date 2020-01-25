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

