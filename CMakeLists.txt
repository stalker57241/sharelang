cmake_minimum_required(VERSION 3.14)

project(sharelang LANGUAGES C)

add_library(lexer src/lexer.c)
target_include_directories(lexer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/lexer>)

add_library(compiler src/compiler.c src/emit.c)
target_include_directories(compiler PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/compiler>)
target_link_libraries(compiler PRIVATE lexer)

add_executable(sharelang src/main.c)
target_link_libraries(sharelang PRIVATE lexer PRIVATE compiler)
