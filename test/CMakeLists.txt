#
#    Copyright 2012-2017 Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.


find_package(Qt5Network REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)

if(Mapper_DEVELOPMENT_BUILD)
	include(EnableSanitize)
	enable_sanitize(address undefined NO_RECOVER)
endif()

add_definitions(
  -DQT_NO_CAST_FROM_ASCII
  -DQT_NO_CAST_TO_ASCII
  -DQT_USE_QSTRINGBUILDER
)

# This function defines a test from a list of source file names 
# (without ending). For each filename, a .cpp file and .h must exist.
# The .h file is processed with Qt's moc.
# The first filename is also used as the name of the executable file and
# as the name of the test.
#
# The test executable is linked only to external libraries by default. 
# It only needs to be rebuild and run when one of its components was modified.
# Additional link libraries may be added to the executable target as usual.
#
# The autorun parameter controls whether the test will be run automatically
# when executing Mapper-with-test.
#
function(add_test_helper testname autorun)
	unset(TEST_${testname}_SRCS)
	set(manual_test 0)
	foreach(arg ${ARGN})
		if("${arg}" STREQUAL "MANUAL")
			set(manual_test 1)
		else()
			list(APPEND TEST_${testname}_SRCS ${arg}.cpp)
		endif()
	endforeach()
	add_executable(${testname} ${testname}.cpp ${TEST_${testname}_SRCS})
	target_link_libraries(${testname}
	  PROJ4::proj
	  Qt5::Test
	  Qt5::Widgets
	  Polyclipping::Polyclipping
	)
	if(NOT manual_test)
		configure_file(TESTNAME-RUN.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${testname}-RUN.cmake @ONLY)
		add_test(${testname} ${CMAKE_COMMAND} -P ${testname}-RUN.cmake)
		add_custom_command(OUTPUT ${testname}-RUN.stamp
		  COMMAND ${CMAKE_COMMAND} -P ${testname}-RUN.cmake
		  DEPENDS ${testname}
		)
		if(autorun)
			set_property(DIRECTORY APPEND PROPERTY Mapper_AUTORUN_TESTS ${testname})
		endif()
		if(Mapper_COVERAGE_BUILD)
			register_coverage_generating_target(${testname})
		endif()
	endif()
endfunction()


# This function defines a unit test from a list of source file names 
# (without ending). For each filename, a .cpp file and .h must exist.
# The .h file is processed with Qt's moc.
# The first filename is also used as the name of the executable file and
# as the name of the test.
#
# A unit test executable is linked only to external libraries by default. 
# It only needs to be rebuild and run when one of its components was modified.
# Additional link libraries may be added to the executable target as usual.
#
function(add_unit_test testname)
	add_test_helper("${testname}" 1 ${ARGN})
endfunction()


# This function defines a system test from a list of source file names 
# (without ending). For each filename, a .cpp file and .h must exist.
# The .h file is processed with Qt's moc.
# The first filename is also used as the name of the executable file and
# as the name of the test.
#
# A system test executable is linked to the full Mapper runtime. 
# That is why it will be rebuild and run very often.
#
function(add_system_test testname)
	add_test_helper("${testname}" ${Mapper_AUTORUN_SYSTEM_TESTS} ${ARGN})
	target_link_libraries(${testname}
	  Mapper_Common
	  libocad
	)
endfunction()


# Include generated files (moc ouput, build configuration)
include_directories("${PROJECT_BINARY_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")


# Unit tests
add_unit_test(tst_qglobal)
add_unit_test(autosave_t MANUAL ../src/core/autosave ../src/settings ../src/util/util)
add_unit_test(encoding_t ../src/util/encoding)
add_unit_test(georeferencing_t ../src/core/georeferencing
	../src/core/latlon
	../src/core/crs_template
	../src/core/crs_template_implementation
	../src/gui/widgets/crs_param_widgets
	../src/mapper_resource
	../src/fileformats/file_format
)
add_unit_test(locale_t ../src/util/translation_util)
add_unit_test(map_color_t ../src/core/map_color)
add_unit_test(qpainter_t)
add_unit_test(util_t ../src/util/util ../src/settings)

# Benchmarks
add_system_test(coord_xml_t MANUAL)
if(Mapper_COVERAGE_BUILD)
	register_coverage_generating_target(coord_xml_t -iterations 1)
endif()

# System tests
add_system_test(file_format_t)
add_system_test(duplicate_equals_t)
add_system_test(map_t)
add_system_test(object_query_t)
add_system_test(path_object_t)
add_system_test(symbol_set_t)
add_system_test(template_t)
add_system_test(tools_t)
add_system_test(transform_t)
add_system_test(undo_manager_t)


# Collect the AUTORUN_TESTS
get_property(Mapper_AUTORUN_TESTS DIRECTORY PROPERTY Mapper_AUTORUN_TESTS)
configure_file(AUTORUN_TESTS.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/AUTORUN_TESTS.cmake @ONLY)


# This executable first runs all tests which are marked for auto-run. If the
# tests don't fail, it starts the Mapper executable. Unlike the default "test"
# target, it is displayed in Qt Creator's project view, and when run, it prints
# the output of failed tests.
add_executable(Mapper-with-test mapper_t.cpp)
add_dependencies(Mapper-with-test Mapper_prerequisites)
target_link_libraries(Mapper-with-test Qt5::Core)
target_compile_definitions(Mapper-with-test PRIVATE -DCMAKE_COMMAND="${CMAKE_COMMAND}" -DMAPPER_LOCATION="$<TARGET_FILE:Mapper>")


if(Mapper_BUILD_QT)
	write_qt_conf()
elseif(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
	file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
endif()

