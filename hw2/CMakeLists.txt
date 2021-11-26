cmake_minimum_required(VERSION 3.10)
# set the project name and version
project(TTT VERSION 0.0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

find_package(OpenSSL REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/utils)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "lib/*.c")
file(GLOB_RECURSE UTILS RELATIVE ${CMAKE_SOURCE_DIR} "utils/*.c")
add_executable(TTT_server server_main.c ${SOURCES} ${UTILS})
add_executable(TTT_client client_main.c ${SOURCES} ${UTILS})


