cmake_minimum_required (VERSION 2.6)

project(marky)

include_directories(
    ${PROJECT_BINARY_DIR} #for generated build-config.h
)

set(marky_srcs
    backend.cpp
    backend-cache.cpp
    backend-map.cpp
    config.cpp
    marky.cpp
    markyc.cpp
    rand-util.cpp
    scorer.cpp
    selector.cpp
    snippet.cpp
    string-pack.cpp
)
set(marky_libs
)

if(BUILD_BACKEND_SQLITE)
    include_directories(
        ${sqlite_INCLUDE_DIR}
    )
    list(APPEND marky_srcs
        backend-sqlite.cpp
    )
    list(APPEND marky_libs
        ${sqlite_LIBRARY}
    )
endif()

configure_file (
    "${PROJECT_SOURCE_DIR}/build-config.in.h"
    "${PROJECT_BINARY_DIR}/build-config.h"
)

add_library(marky SHARED ${marky_srcs})
target_link_libraries(marky ${marky_libs})
