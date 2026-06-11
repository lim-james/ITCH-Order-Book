cmake_minimum_required(VERSION 3.20)
project(itch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++26")

add_library(itch_flags INTERFACE)

target_compile_options(itch_flags INTERFACE
    # warnings
    -Wall -Wextra -Wpedantic
    -Wuninitialized -Wunused -Wconversion
    -Wno-interference-size          # suppress the atomic padding noise

    # experimental features
    -freflection -fcontracts
    -fcontract-evaluation-semantic=ignore

    # error clarity
    -ftemplate-backtrace-limit=3    # cuts the "required from" cascade
    -fconcepts-diagnostics-depth=2  # enough context without the avalanche
    -fdiagnostics-color=always      # colour in terminal / piped output

    # per-config
    $<$<CONFIG:Release>:-O3 -march=native -g>
    $<$<CONFIG:Debug>:-O0 -g -fsanitize=address,undefined>
)

target_link_options(itch_flags INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)


add_library(itch_includes INTERFACE)
target_include_directories(itch_includes INTERFACE ${CMAKE_SOURCE_DIR}/include)

add_library(itch_core
    src/feed/file_source.cpp
    src/book/order_book.cpp
)
target_link_libraries(itch_core PUBLIC itch_includes itch_flags)

add_executable(bench bench/replay.cpp)
target_link_libraries(bench PRIVATE itch_core itch_flags)
