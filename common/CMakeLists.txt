cmake_minimum_required(VERSION 2.8)
project(common)

file(GLOB SRC_FILES "*.cc")
set(SRC_LIST ${SRC_FILES})

add_compile_options(-g -Wall)
add_library(${PROJECT_NAME} ${SRC_LIST})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "common")
set(LIBRARY_OUTPUT_PATH ../lib/)
