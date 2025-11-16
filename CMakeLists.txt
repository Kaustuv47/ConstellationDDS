cmake_minimum_required(VERSION 4.0)
project(ConstellationDDS C)

set(CMAKE_C_STANDARD 23)

add_library(ConstellationDDS STATIC library.c
        library.h
)

add_executable(Test Test.c)

# Link against the static library
target_link_libraries(Test PRIVATE ConstellationDDS)

# Platform-specific libraries
if (WIN32)
    target_link_libraries(Test PRIVATE ws2_32)
elseif (UNIX)
    # On Linux, usually no extra libs needed, but if you use pthreads:
    find_package(Threads REQUIRED)
    target_link_libraries(Test PRIVATE Threads::Threads)
endif()