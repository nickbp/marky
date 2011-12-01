cmake_minimum_required (VERSION 2.6)

project(marky)

include_directories(
	${PROJECT_BINARY_DIR} #for generated config.h
)

set(marky_srcs
	backend-map.cpp
	config.cpp
	marky.cpp
	scorer.cpp
	selector.cpp
)
set(marky_libs
)

if(BUILD_BACKEND_SQLITE)
	include_directories(
		${sqlite_INCLUDE_DIR}
	)
	list(APPEND marky_srcs
		backend-cache.cpp
		backend-sqlite.cpp
	)
	list(APPEND marky_libs
		${sqlite_LIBRARY}
	)
endif()

configure_file (
	"${PROJECT_SOURCE_DIR}/config.in.h"
	"${PROJECT_BINARY_DIR}/config.h"
)

add_library(marky ${marky_srcs})
target_link_libraries(marky ${marky_libs})
