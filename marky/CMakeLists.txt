cmake_minimum_required (VERSION 2.6)

project(marky)

include_directories(
	${PROJECT_BINARY_DIR}
)

set(marky_srcs
	backend.h
	backend-map.h
	backend-map.cpp
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
set(marky_libs
)

if(BUILD_BACKEND_SQLITE)
	list(APPEND marky_srcs backend-sqlite.h backend-sqlite.cpp)
	list(APPEND marky_libs ${sqlite_LIBRARY})
	include_directories(${sqlite_INCLUDE_DIR})
endif()

configure_file (
	"${PROJECT_SOURCE_DIR}/config.in.h"
	"${PROJECT_BINARY_DIR}/config.h"
)

add_library(marky ${marky_srcs})
target_link_libraries(marky ${marky_libs})
