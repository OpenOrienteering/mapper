#include(CMakeForceCompiler)
## Prefix detection only works with compiler id "GNU"
## CMake will look for prefixed g++, cpp, ld, etc. automatically
##CMAKE_FORCE_C_COMPILER(arm-linux-androideabi-gcc GNU)

# This toolchain file is known to work with:
# TODO: VERIFY Ubuntu 12.04 (Precise)

# Release build options
set(Mapper_BUILD_QT   FALSE CACHE BOOL "Toolchain default")
set(Mapper_BUILD_PROJ TRUE CACHE BOOL "Toolchain default")
set(PROJ_VERSION      4.9.0b2 CACHE STRING "Toolchain default")

# The name of the target operating system
SET(CMAKE_SYSTEM_NAME Linux)

# The name of the target system name as known to GNU tools
set(GNU_SYSTEM_NAME arm-linux-androideabi)

# The GCC version (Major.Minor)
set(GCC_VERSION 4.8)

# Find the tools
find_program(CMAKE_C_COMPILER NAMES ${GNU_SYSTEM_NAME}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${GNU_SYSTEM_NAME}-g++)

# Set root path and behaviour of FIND_XXX() commands
#set(USER_TARGET_ROOT_PATH "${CMAKE_CURRENT_BINARY_DIR}/usr/${GNU_SYSTEM_NAME}")
#set(CMAKE_FIND_ROOT_PATH /usr/${GNU_SYSTEM_NAME} "${USER_TARGET_ROOT_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

#set(BUILD_SHARED_LIBS ON)

# Toolchain properties for gcc 4.6
#set(TOOLCHAIN_SHARED_LIBS libgcc_s_sjlj-1 libstdc++-6)
set(TOOLCHAIN_PATH_SUFFIXES gcc/${GNU_SYSTEM_NAME}-${GCC_VERSION})
