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

# Custom sanitizer build type: cmake -DCMAKE_BUILD_TYPE=Asan ...
if(CMAKE_BUILD_TYPE STREQUAL "Asan")
    add_compile_options(-O1 -g -fno-omit-frame-pointer
                        -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# ---------- Static library (reused by exe and tests) ----------
add_library(htn_lib STATIC
    src/log.c
    src/net.c
    src/server.c
)
target_include_directories(htn_lib PUBLIC include)

# ---------- Server executable ----------
add_executable(htn src/main.c)
target_link_libraries(htn PRIVATE htn_lib)

# ---------- Tests wired up in section 3 ----------
