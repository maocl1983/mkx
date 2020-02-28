cmake_minimum_required(VERSION 2.8)
project(server)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../framework)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../common)

file(GLOB SRC_FILES "*.cc")
file(GLOB SRC_PROTOS "protocc/*.cc")
set(SRC_LIST ${SRC_FILES} ${SRC_PROTOS})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)
link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../framework/lib/
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/lib/
)

add_compile_options(-g -Wall)

add_executable(server ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} framework common ev pthread protobuf)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "server")
set(EXECUTABLE_OUTPUT_PATH ../)
