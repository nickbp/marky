cmake_minimum_required (VERSION 2.6)

project(marky)

set(marky_VERSION_MAJOR 1)
set(marky_VERSION_MINOR 0)
set(marky_VERSION_PATCH 0)

# Enable c++0x and more warnings
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-std=c++0x -Wall")
endif()

set(marky_srcs
  backend.h
  backend.cpp
  config.in.h
  config.cpp
  link.h
  marky.h
  marky.cpp
  scorer.h
  scorer.cpp
  selector.h
  selector.cpp
  )
configure_file (
  "${PROJECT_SOURCE_DIR}/config.in.h"
  "${PROJECT_BINARY_DIR}/config.h"
  )

include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}" ${INCLUDES})
add_library(marky ${marky_srcs})
target_link_libraries(marky ${marky_libs})

include (InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "../LICENCE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${marky_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${marky_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${marky_VERSION_PATCH}")
include(CPack)
