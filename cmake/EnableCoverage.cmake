set(COVERAGE_FLAGS_FOR_GNU -fprofile-arcs -ftest-coverage)
set(COVERAGE_FLAGS_FOR_Clang ${COVERAGE_FLAGS_GNU})
set(COVERAGE_FLAGS_FOR_AppleClang ${COVERAGE_FLAGS_GNU})

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
	set(is_clang 1)
else()
	set(is_clang 0)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR is_clang)
	set(use_gcov 1)
else()
	set(use_gcov 0)
endif()

# Most of these functions are project specific; add_subdirectory'ing a CMakeLists.txt with a call to project() will
# result in a new "scope" of coverage to be opened

if(use_gcov AND UNIX)
	set(coverage_dir "${CMAKE_BINARY_DIR}/coverage")

	if(is_clang)
		find_package(LLVM)
		if(LLVM_FOUND) # prefer using the LLVM cmake files
			get_target_property(LLVM_COV llvm-cov LOCATION)
		else() # but fallback to simply searching for the program otherwise
			find_program(LLVM_COV llvm-cov)
		endif()
		if(NOT LLVM_COV)
			message(SEND_ERROR "No llvm-cov executable found")
		endif()

		# create gcov wrapper
		file(WRITE ${coverage_dir}/llvm-gcov.sh "#!/bin/bash\nexec ${LLVM_COV} gcov \"$@\"")
		execute_process(COMMAND chmod +x ${coverage_dir}/llvm-gcov.sh)
		set(LCOV_EXTRA_OPTS "--gcov-tool=${coverage_dir}/llvm-gcov.sh")
	endif()
	if(use_gcov)
		find_program(LCOV lcov)
		find_program(GENHTML genhtml)
		if(NOT LCOV OR NOT GENHTML)
			message(SEND_ERROR "Either lcov or genhtml executable not found")
		endif()
		set(LCOV_OPTS --directory ${PROJECT_SOURCE_DIR} --base-directory ${PROJECT_SOURCE_DIR} ${LCOV_EXTRA_OPTS})
	endif()

	add_custom_target(coverage_reset
		COMMAND ${LCOV} ${LCOV_OPTS} --zerocounters
		COMMAND ${LCOV} ${LCOV_OPTS} --capture --initial --no-external --output-file ${coverage_dir}/${PROJECT_NAME}_base.info
		VERBATIM
		COMMENT "Reseting coverage counters"
		ALL
	)

	# Adds the necessary compiler flags to the given target
	function(enable_coverage_for target)
		target_compile_options(${target} PRIVATE -fprofile-arcs -ftest-coverage)
		# apparently the way to set linker flags is through target_link_libraries...
		target_link_libraries(${target} --coverage)
		# rebuilding changes the target, thus the coverage will need to be regenerated to be valid
	endfunction()

	# Sets the given target up for coverage generating, and adds it to the list of targets to call when capturing coverage
	function(register_coverage_generating_target target)
		get_target_property(target_type ${target} TYPE)
		if(target_type STREQUAL EXECUTABLE)
			enable_coverage_for(${target})

			add_custom_target(coverage_${target} COMMAND ${target} ${ARGN} COMMENT "Running ${target}")

			# this ensures that coverage_reset is executed BEFORE running targets in coverage_capture
			add_dependencies(coverage_${target} coverage_reset)

			set_property(DIRECTORY ${PROJECT_SOURCE_DIR} APPEND PROPERTY coverage_targets coverage_${target})
		else()
			message(AUTHOR_WARNING "register_coverage_generating_target only works with executable targets")
		endif()
	endfunction()

	# Creates some common targets required for coverage generation
	function(finalize_coverage_setup)
		get_property(coverage_targets DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY coverage_targets)
		string(REPLACE ";" " " coverage_targets_joined "${coverage_targets}")
		message(STATUS "Finalizing coverage setup with these targets: ${coverage_targets_joined}")

		if(use_gcov)
			set(info_file ${coverage_dir}/${PROJECT_NAME}.info)

			add_custom_target(coverage_capture
				DEPENDS coverage_reset ${coverage_targets}
				COMMAND ${LCOV} ${LCOV_OPTS} --capture --no-external --output-file ${info_file}
				COMMAND ${LCOV} ${LCOV_OPTS} --add-tracefile ${info_file} --add-tracefile ${coverage_dir}/${PROJECT_NAME}_base.info --output-file ${info_file}
				COMMAND ${LCOV} ${LCOV_OPTS} --remove ${info_file} "moc_*" "${PROJECT_BINARY_DIR}/*" ${ARGN} --output-file ${info_file}
				BYPRODUCTS ${info_file}
				VERBATIM
				COMMENT "Capturing coverage data (this may take some time)"
			)

			add_custom_target(coverage_html
				DEPENDS coverage_capture
				COMMAND ${GENHTML} --demangle-cpp --legend --title ${PROJECT_NAME} --output-directory ${coverage_dir}/html ${info_file}
				VERBATIM
				COMMENT "Generating coverage HTML report"
			)

			set(codecov ${coverage_dir}/codecov.sh)
			file(DOWNLOAD https://codecov.io/bash ${codecov})
			add_custom_target(coverage_codecov
				DEPENDS coverage_capture
				COMMAND ${codecov} -f ${info_file} -X search -X gcov
				VERBATIM
				COMMENT "Uploading coverage data to codecov.io"
			)
		endif()
	endfunction()
else()
	message(WARNING "No support for generating code coverage for the current compiler")
endif()
