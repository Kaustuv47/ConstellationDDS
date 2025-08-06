cmake_minimum_required(VERSION 4.0)
project(ConstellationDDS C)

set(CMAKE_C_STANDARD 23)

add_library(ConstellationDDS STATIC library.c
        library.h)

add_executable(Test Test.c)

target_link_libraries(Test
        PRIVATE
        ${CMAKE_SOURCE_DIR}/cmake-build-debug/libConstellationDDS.a
        ws2_32
)