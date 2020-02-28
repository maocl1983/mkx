cmake_minimum_required(VERSION 2.8)
project(framework)

file(GLOB SRC_FILES "*.cc")
set(SRC_LIST ${SRC_FILES})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../common/
)
link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../common/
)

add_compile_options(-g -Wall)
add_library(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} common)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "framework")
set(LIBRARY_OUTPUT_PATH ../lib/)
