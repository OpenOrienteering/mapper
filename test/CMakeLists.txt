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
#
# A unit test executable is linked only to external libraries by default. 
# It only needs to be rebuild and run when one of its components was modified.
# Additional link libraries may be added to the executable target as usual.
#
macro(add_unit_test testname)
	unset(TEST_${testname}_SRCS)
	unset(TEST_${testname}_HDRS)
	foreach(arg ${testname} ${ARGN})
		list(APPEND TEST_${testname}_SRCS ${arg}.cpp)
		list(APPEND TEST_${testname}_HDRS ${arg}.h)
	endforeach(arg)
	qt4_wrap_cpp(TEST_${testname}_MOC ${TEST_${testname}_HDRS})
	add_executable(${testname} ${TEST_${testname}_SRCS} ${TEST_${testname}_MOC})
	target_link_libraries(${testname} ${PROJ_LIBRARY} ${QT_QTTEST_LIBRARY} ${QT_LIBRARIES})
	add_test(${testname} ${testname})
	list(APPEND Mapper_UNIT_TESTS ${testname})
	add_custom_command(
	  OUTPUT ${testname}.stamp
	  DEPENDS ${testname}
	  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "^${testname}$"
	  COMMAND ${CMAKE_COMMAND} -E touch ${testname}.stamp
	  COMMENT "Running unit test ${testname}"
	  VERBATIM
	)
	list(APPEND Mapper_UNIT_TEST_STAMPS ${testname}.stamp)
endmacro(add_unit_test)
unset(Mapper_UNIT_TESTS)
unset(Mapper_UNIT_TEST_STAMPS)


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
	foreach(arg ${testname} ${ARGN})
		list(APPEND TEST_${testname}_SRCS ${arg}.cpp)
		list(APPEND TEST_${testname}_HDRS ${arg}.h)
	endforeach(arg)
	qt4_wrap_cpp(TEST_${testname}_MOC ${TEST_${testname}_HDRS})
	add_executable(${testname} ${TEST_${testname}_SRCS} ${TEST_${testname}_MOC})
	target_link_libraries(${testname} ${QT_QTTEST_LIBRARY} Mapper_Common)
	add_test(${testname} ${testname})
	list(APPEND Mapper_SYSTEM_TESTS ${testname})
	add_custom_command(
	  OUTPUT ${testname}.stamp
	  DEPENDS ${testname}
	  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "^${testname}$"
	  COMMAND ${CMAKE_COMMAND} -E touch ${testname}.stamp
	  COMMENT "Running system test ${testname}"
	  VERBATIM
	)
	list(APPEND Mapper_SYSTEM_TEST_STAMPS ${testname}.stamp)
endmacro(add_system_test)
unset(Mapper_SYSTEM_TESTS)
unset(Mapper_SYSTEM_TEST_STAMPS)


# Include generated files (moc ouput, build configuration)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../src")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")


# Now define the actual unit tests.
add_unit_test(georeferencing_t ../src/georeferencing)

# and the automated system tests.
add_system_test(file_format_t)


# This top-level target builds and runs all unit tests for modified units and 
# all system tests.
# The tests are run by individual targets. This means they may run in parallel.
add_custom_target(AUTORUN_TESTS
  ALL
  DEPENDS ${Mapper_UNIT_TEST_STAMPS} ${Mapper_SYSTEM_TEST_STAMPS}
  COMMENT "All tests finished"
)


# This top-level target will always run all tests but
#   a) generate the unit test executables if necessary
#   b) display the output of failed unit tests.
# (Both features are missing from the default test target.)
add_custom_target(RUN_TESTS
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS ${Mapper_UNIT_TESTS} ${Mapper_SYSTEM_TESTS}
)
