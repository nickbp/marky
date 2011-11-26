cmake_minimum_required (VERSION 2.6)

project(tests)

include_directories(
	${gtest_INCLUDE_DIR}
	${MARKY_INCLUDES}
)

find_package(Threads)
set(gtest_libs ${gtest_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

add_executable(test-link test-link.cpp)
add_test(test-link test-link)
target_link_libraries(test-link marky ${gtest_libs})

add_executable(test-scorer test-scorer.cpp)
add_test(test-scorer test-scorer)
target_link_libraries(test-scorer marky ${gtest_libs})

add_executable(test-selector test-selector.cpp)
add_test(test-selector test-selector)
target_link_libraries(test-selector marky ${gtest_libs})

add_executable(test-backend-map test-backend-map.cpp)
add_test(test-backend-map test-backend-map)
target_link_libraries(test-backend-map marky ${gtest_libs})

if(BUILD_BACKEND_SQLITE)
	add_executable(test-backend-sqlite test-backend-sqlite.cpp)
	add_test(test-backend-sqlite test-backend-sqlite)
	target_link_libraries(test-backend-sqlite marky ${gtest_libs})
endif()

add_executable(test-marky test-marky.cpp)
add_test(test-marky test-marky)
target_link_libraries(test-marky marky ${gtest_libs})
