message(WARNING "LICENSING_PROVIDER=ubuntu is deprecated. Switching to debian.")
set(LICENSING_PROVIDER "debian" CACHE STRING "Provider for 3rd-party licensing information" FORCE)
include("${CMAKE_CURRENT_LIST_DIR}/debian-licensing.cmake")
