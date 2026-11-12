cmake_minimum_required(VERSION 3.20)

project(ShaiyaPacketTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ShaiyaPacketTool SHARED
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/ShaiyaPacketTool.def
)

target_compile_definitions(ShaiyaPacketTool PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    SHAIYAPACKETTOOL_EXPORTS
)

target_link_libraries(ShaiyaPacketTool PRIVATE
    comctl32
    ws2_32
)

set_target_properties(ShaiyaPacketTool PROPERTIES
    OUTPUT_NAME "ShaiyaPacketTool"
    PREFIX ""
)

add_executable(TestLoader
    src/TestLoader.cpp
)

target_compile_definitions(TestLoader PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
)
