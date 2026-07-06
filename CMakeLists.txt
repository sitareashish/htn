cmake_minimum_required(VERSION 3.25)
project(htn
    VERSION 0.1.0
    DESCRIPTION "HTN: multithreaded epoll TCP server"
    LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wstrict-prototypes
    -Wmissing-prototypes
    -Werror=implicit-function-declaration
    -D_GNU_SOURCE
)

add_executable(htn
    src/main.c
)
target_include_directories(htn PRIVATE include)

enable_testing()
