cmake_minimum_required(VERSION 3.20)
project(itch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

add_link_options(
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

add_library(itch_includes INTERFACE)
target_include_directories(itch_includes INTERFACE ${CMAKE_SOURCE_DIR}/include)

add_library(itch_core
    src/feed/file_source.cpp
    # src/market_data/MarketDataHandler.cpp
    # src/book/OrderBook.cpp
)
target_link_libraries(itch_core PUBLIC itch_includes)

add_executable(bench bench/replay.cpp)
target_link_libraries(bench PRIVATE itch_core)
