cmake_minimum_required(VERSION 3.14)

project(sharelang LANGUAGES C)

add_library(lexer src/lexer.c)
target_include_directories(lexer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/lexer>)

add_executable(sharelang "src/main.c")
target_link_libraries(sharelang PRIVATE lexer)
