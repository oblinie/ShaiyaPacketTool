
cmake_minimum_required(VERSION 3.20)

project(ShaiyaPacketTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ShaiyaPacketTool WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
)

target_compile_definitions(ShaiyaPacketTool PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(ShaiyaPacketTool PRIVATE
    comctl32
)
