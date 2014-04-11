cmake_minimum_required (VERSION 2.6)

project(tests)

include_directories(
    ${gtest_INCLUDE_DIR}
    ${MARKY_INCLUDES}
)

find_package(Threads)
set(gtest_libs ${gtest_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

# unit tests

add_executable(test-marky test-marky.cpp)
target_link_libraries(test-marky marky ${gtest_libs})
add_test(test-marky test-marky)

add_executable(test-snippet test-snippet.cpp)
target_link_libraries(test-snippet marky ${gtest_libs})
add_test(test-snippet test-snippet)

add_executable(test-scorer test-scorer.cpp)
target_link_libraries(test-scorer marky ${gtest_libs})
add_test(test-scorer test-scorer)

add_executable(test-selector test-selector.cpp)
target_link_libraries(test-selector marky ${gtest_libs})
add_test(test-selector test-selector)

add_executable(test-backend-map test-backend-map.cpp)
target_link_libraries(test-backend-map marky ${gtest_libs})
add_test(test-backend-map test-backend-map)

if(BUILD_BACKEND_SQLITE)
    add_executable(test-string-pack test-string-pack.cpp)
    target_link_libraries(test-string-pack marky ${gtest_libs})
    add_test(test-string-pack test-string-pack)

    add_executable(test-backend-sqlite test-backend-sqlite.cpp)
    target_link_libraries(test-backend-sqlite marky ${gtest_libs})
    add_test(test-backend-sqlite test-backend-sqlite)
endif()

# benchmark tests

set(BUILD_BENCH_TESTS false)

# unzip test data if not already present
set(TEST_DATA_FILENAME "data-my_antonia.txt")
find_file(TEST_DATA_OUT_PATH ${TEST_DATA_FILENAME}
    PATHS ${PROJECT_BINARY_DIR}
    NO_DEFAULT_PATH)
mark_as_advanced(TEST_DATA_OUT_PATH)
if (TEST_DATA_OUT_PATH)
    # unzipped data found, all set
    message(STATUS "Found test data: "${TEST_DATA_OUT_PATH})
    set(BUILD_BENCH_TESTS true)
else()
    set(TEST_DATA_ZIPPED_EXT ".bz2")
    find_file(TEST_DATA_IN_PATH ${TEST_DATA_FILENAME}${TEST_DATA_ZIPPED_EXT}
        PATHS ${PROJECT_SOURCE_DIR}
        NO_DEFAULT_PATH)
    mark_as_advanced(TEST_DATA_IN_PATH)
    if(NOT TEST_DATA_IN_PATH)
        message(ERROR " Couldn't find compressed test data: ${PROJECT_SOURCE_DIR}/${TEST_DATA_FILENAME}${TEST_DATA_ZIPPED_EXT}")
    else()
        find_program(TEST_DATA_BUNZIP2_EXE bunzip2
            DOC "path to bunzip2 executable, used to decompress benchmark test data")
        mark_as_advanced(TEST_DATA_BUNZIP2_EXE)

        if(NOT TEST_DATA_BUNZIP2_EXE)
            message(INFO " 'bzip2' executable not found. Excluding benchmark tests from build.")
        else()
            execute_process(COMMAND ${TEST_DATA_BUNZIP2_EXE}
                INPUT_FILE "${TEST_DATA_IN_PATH}"
                OUTPUT_FILE "${PROJECT_BINARY_DIR}/${TEST_DATA_FILENAME}"
                RESULT_VARIABLE DECOMPRESS_RESULT)
            if(DECOMPRESS_RESULT EQUAL 0)
                # unzip successful, all set
                set(BUILD_BENCH_TESTS true)
            else()
                message(ERROR " Command failed! Benchmark tests disabled: " ${DECOMPRESS_RESULT})
            endif()
        endif()
    endif()
endif()

if(BUILD_BENCH_TESTS)
    include_directories(
        ${PROJECT_BINARY_DIR} #for generated test-bench-config.h
    )

    if(BUILD_BACKEND_SQLITE)
        add_executable(test-bench-sqlite test-bench-sqlite.cpp)
        target_link_libraries(test-bench-sqlite marky ${gtest_libs})
        # don't add to CTest, assume it'll be too slow to run often
    endif()

    configure_file (
        "${PROJECT_SOURCE_DIR}/test-bench-config.in.h"
        "${PROJECT_BINARY_DIR}/test-bench-config.h"
    )
endif()
