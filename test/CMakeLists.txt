#
#    Copyright 2012 Kai Pastor
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

find_package(Qt4 4.6 COMPONENTS QtTest)


# This macro defines a unit test from a list of source file names 
# (without ending). For each filename, a .cpp file and .h must exist.
# The .h file is processed with Qt's moc.
# The first filename is also used as the name of the executable file and
# as the name of the test.
macro(add_executable_test testname)
	unset(TEST_${testname}_SRCS)
	unset(TEST_${testname}_HDRS)
	foreach(arg ${testname} ${ARGN})
		list(APPEND TEST_${testname}_SRCS ${arg}.cpp)
		list(APPEND TEST_${testname}_HDRS ${arg}.h)
	endforeach(arg)
	qt4_wrap_cpp(TEST_${testname}_MOC ${TEST_${testname}_HDRS})
	add_executable(${testname} ${TEST_${testname}_SRCS} ${TEST_${testname}_MOC})
	target_link_libraries(${testname} Mapper_Common ${QT_QTTEST_LIBRARY})
	add_test(${testname} ${testname})
	list(APPEND TEST_LIST ${testname})
endmacro(add_executable_test)
unset(TEST_LIST)


# Include generated files (moc ouput, build configuration)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")


# Now define the actual unit tests.
add_executable_test(georeferencing_t)


# This output will be updated and re-run CTest when something has changed.
add_custom_command(
  OUTPUT auto.run
  DEPENDS ${TEST_LIST}
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  COMMAND ${CMAKE_COMMAND} -E touch auto.run
  COMMENT "Unit test auto-run"
  VERBATIM
)

# This top-level target will run CTest when something has changed.
# It is meant to be part of the "all" target.
add_custom_target(AUTORUN_TESTS
  ALL
  DEPENDS auto.run
)

# This top-level target will always run CTest but
#   a) generate the unit test executables if necessary
#   b) display the output of failed unit tests
# (Both features are missing from the default test target.)
add_custom_target(RUN_TESTS
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS ${TEST_LIST}
)
