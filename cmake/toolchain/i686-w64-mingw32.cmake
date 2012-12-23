# This toolchain file is known to work with:
# Ubuntu 12.04 (Precise)

# Release build options
set(Mapper_BUILD_QT   TRUE CACHE BOOL "Toolchain default")
set(Mapper_BUILD_PROJ TRUE CACHE BOOL "Toolchain default")

# The name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# The name of the target system name as known to GNU tools
set(GNU_SYSTEM_NAME i686-w64-mingw32)

# The GCC version (Major.Minor)
set(GCC_VERSION 4.6)

# The name of the target system name as Qt platform
set(QT_PLATFORM "unsupported/win32-g++-${GCC_VERSION}-${GNU_SYSTEM_NAME}-cross")

# The name of a target system known to Qt from which to inherit settings
# when QT_PLATFORM is unknown to Qt
set(QT_PARENT_PLATFORM "unsupported/win32-g++-${GCC_VERSION}-cross")

# The name of the target system name as Qt5 platform
set(QT5_PLATFORM "-xplatform win32-g++ -device-option CROSS_COMPILE=${GNU_SYSTEM_NAME}-")

# Find the tools
find_program(CMAKE_C_COMPILER NAMES ${GNU_SYSTEM_NAME}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${GNU_SYSTEM_NAME}-g++)
find_program(CMAKE_RC_COMPILER NAMES ${GNU_SYSTEM_NAME}-windres)

# Set root path and behaviour of FIND_XXX() commands
#set(USER_TARGET_ROOT_PATH "${CMAKE_CURRENT_BINARY_DIR}/usr/${GNU_SYSTEM_NAME}")
#set(CMAKE_FIND_ROOT_PATH /usr/${GNU_SYSTEM_NAME} "${USER_TARGET_ROOT_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

set(BUILD_SHARED_LIBS ON)

# Toolchain properties for gcc 4.6
set(TOOLCHAIN_SHARED_LIBS libgcc_s_sjlj-1 libstdc++-6)
set(TOOLCHAIN_PATH_SUFFIXES gcc/${GNU_SYSTEM_NAME}/${GCC_VERSION})
