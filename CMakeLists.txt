cmake_minimum_required(VERSION 3.10)
project(itch_parser C CXX)

set(CMAKE_C_STANDARD 11)

add_executable(itch_parser
    src/main.c
    src/db.c
    src/parser.c
    src/duckdb_stub.cpp
)

target_include_directories(itch_parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(itch_parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libduckdb_static.a
    pthread
    dl
    m
    stdc++
)