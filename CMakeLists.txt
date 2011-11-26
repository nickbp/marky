cmake_minimum_required (VERSION 2.6)

set(marky_VERSION_MAJOR 1)
set(marky_VERSION_MINOR 0)
set(marky_VERSION_PATCH 0)

# GTest options

find_path(gtest_INCLUDE_DIR NAMES gtest/gtest.h)
find_library(gtest_LIBRARY NAMES gtest)

if(gtest_INCLUDE_DIR AND gtest_LIBRARY)
	message(STATUS "Found gtest")
	set(FOUND_GTEST ON)
else()
	message(STATUS "Didn't find gtest")
endif()

option(BUILD_TESTS "Build unit tests" ${FOUND_GTEST})

# SQLite search/options

find_path(sqlite_INCLUDE_DIR NAMES sqlite3.h)
find_library(sqlite_LIBRARY NAMES sqlite3)

if(sqlite_INCLUDE_DIR AND sqlite_LIBRARY)
	message(STATUS "Found SQLite")
	set(FOUND_SQLITE ON)
else()
	message(STATUS "Didn't find SQLite")
endif()

option(BUILD_BACKEND_SQLITE "Build SQLite backend" ${FOUND_SQLITE})

# Enable c++0x and more warnings
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-std=c++0x -Wall")
endif()

set(MARKY_INCLUDES
	${PROJECT_SOURCE_DIR}
	${PROJECT_BINARY_DIR}
)

add_subdirectory(apps)
add_subdirectory(marky)
if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENCE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${marky_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${marky_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${marky_VERSION_PATCH})
include(CPack)
