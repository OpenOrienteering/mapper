#
#    Copyright 2012-2015 Kai Pastor
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


# Prerequisite
add_subdirectory(data)

find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)


# This macro defines a unit test from a list of source file names 
# (without ending). For each filename, a .cpp file and .h must exist.
# The .h file is processed with Qt's moc.
# The first filename is also used as the name of the executable file and
# as the name of the test.
#
# A unit test executable is linked only to external libraries by default. 
# It only needs to be rebuild and run when one of its components was modified.
# Additional link libraries may be added to the executable target as usual.
#
macro(add_unit_test testname)
	unset(TEST_${testname}_SRCS)
	unset(TEST_${testname}_HDRS)
	foreach(arg ${ARGN})
		list(APPEND TEST_${testname}_SRCS ${arg}.cpp)
		list(APPEND TEST_${testname}_HDRS ${arg}.h)
		if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${arg}_p.h)
			list(APPEND TEST_${testname}_HDRS ${arg}_p.h)
		endif()
	endforeach(arg)
	qt5_wrap_cpp(TEST_${testname}_MOC ${testname}.h)
	qt5_wrap_cpp(TEST_${testname}_XTRA_MOC ${TEST_${testname}_HDRS} OPTIONS -nw)
	add_executable(${testname} ${testname}.cpp ${TEST_${testname}_SRCS} ${TEST_${testname}_MOC} ${TEST_${testname}_XTRA_MOC})
	target_link_libraries(${testname} libocad ${PROJ_LIBRARY} Qt5::Test Qt5::Widgets polyclipping)
	add_test(${testname} ${CMAKE_COMMAND} -P ${testname}-RUN.cmake)
	list(APPEND Mapper_UNIT_TESTS ${testname})
	set(TESTNAME ${testname}) # required for configure_file
	configure_file(TESTNAME-RUN.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${testname}-RUN.cmake @ONLY)
endmacro(add_unit_test)
unset(Mapper_UNIT_TESTS)


# This macro defines a system test from a list of source file names 
# (without ending). For each filename, a .cpp file and .h must exist.
# The .h file is processed with Qt's moc.
# The first filename is also used as the name of the executable file and
# as the name of the test.
#
# A system test executable is linked to the full Mapper runtime. 
# That is why it will be rebuild and run very often.
#
macro(add_system_test testname)
	unset(TEST_${testname}_SRCS)
	unset(TEST_${testname}_HDRS)
	foreach(arg ${ARGN})
		list(APPEND TEST_${testname}_SRCS ${arg}.cpp)
		list(APPEND TEST_${testname}_HDRS ${arg}.h)
		if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${arg}_p.h)
			list(APPEND TEST_${testname}_HDRS ${arg}_p.h)
		endif()
	endforeach(arg)
	qt5_wrap_cpp(TEST_${testname}_MOC ${testname}.h)
	qt5_wrap_cpp(TEST_${testname}_XTRA_MOC ${TEST_${testname}_HDRS} OPTIONS -nw)
	add_executable(${testname} ${testname}.cpp ${TEST_${testname}_SRCS} ${TEST_${testname}_MOC} ${TEST_${testname}_XTRA_MOC})
	target_link_libraries(${testname} Mapper_Common libocad ${PROJ_LIBRARY} Qt5::Test Qt5::Widgets polyclipping)
	add_test(${testname} ${CMAKE_COMMAND} -P ${testname}-RUN.cmake)
	list(APPEND Mapper_SYSTEM_TESTS ${testname})
	set(TESTNAME ${testname}) # required for configure_file
	configure_file(TESTNAME-RUN.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${testname}-RUN.cmake @ONLY)
endmacro(add_system_test)
unset(Mapper_SYSTEM_TESTS)


# Include generated files (moc ouput, build configuration)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")


# Unit tests
add_unit_test(autosave_t ../src/core/autosave ../src/settings ../src/util ../src/mapper_resource)
add_unit_test(georeferencing_t ../src/core/georeferencing
	../src/core/latlon
	../src/core/crs_template
	../src/core/crs_template_implementation
	../src/gui/widgets/crs_param_widgets
	../src/mapper_resource
	../src/file_format
)
add_unit_test(map_color_t ../src/core/map_color)
add_unit_test(qpainter_t)

# Benchmarks
add_system_test(coord_xml_t)

# System tests
add_system_test(map_t)
add_system_test(file_format_t)
add_dependencies(file_format_t Mapper_test_data)
add_system_test(duplicate_equals_t)
add_dependencies(duplicate_equals_t Mapper_test_data)
add_system_test(tools_t)
add_system_test(path_object_t)
add_system_test(undo_manager_t)
add_system_test(symbol_set_t)


# Collect the AUTORUN_TESTS
set(Mapper_AUTORUN_TESTS )
if(Mapper_AUTORUN_UNIT_TESTS)
	list(APPEND Mapper_AUTORUN_TESTS ${Mapper_UNIT_TESTS})
endif()
if(Mapper_AUTORUN_SYSTEM_TESTS)
	list(APPEND Mapper_AUTORUN_TESTS ${Mapper_SYSTEM_TESTS})
elseif(Mapper_AUTORUN_UNIT_TESTS)
	# Don't autorun expensive unit tests unless system tests are autorun, too.
	list(REMOVE_ITEM Mapper_AUTORUN_TESTS autosave_t coord_xml_t)
endif()
configure_file(AUTORUN_TESTS.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/AUTORUN_TESTS.cmake @ONLY)


# This top-level target builds and runs selected test as part the default 
# "all" target. For a particular test to run, 
#   a) autorun must be configured for that type of test
#      (Mapper_AUTORUN_UNIT_TESTS Mapper_AUTORUN_SYSTEM_TESTS), and
#   b) the test or its dependencies must have been modified.
#
add_custom_target(AUTORUN_TESTS
  ALL
  DEPENDS ${Mapper_AUTORUN_TESTS}
  COMMAND ${CMAKE_COMMAND} -P AUTORUN_TESTS.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(AUTORUN_TESTS Mapper_prerequisites)


# This top-level target always runs CTest (i.e. all tests) but
# unlike the default "test" target, it will
#   a) generate the test executables if necessary,
#   b) display the output of failed tests,
#   c) show up in KDevelop's project view.
#
add_custom_target(Mapper_Test
  DEPENDS ${Mapper_UNIT_TESTS} ${Mapper_SYSTEM_TESTS}
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
)
add_dependencies(Mapper_Test Mapper_prerequisites)


if(Mapper_BUILD_QT)
	write_qt_conf()
endif()

