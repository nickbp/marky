cmake_minimum_required (VERSION 2.6)

project(apps)

include_directories(
	${MARKY_INCLUDES}
)

add_executable(marky-file marky-file.cpp)
target_link_libraries(marky-file marky)
