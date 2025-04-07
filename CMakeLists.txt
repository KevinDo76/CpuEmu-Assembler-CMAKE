cmake_minimum_required(VERSION 3.5.0)

set(CMAKE_CXX_STANDARD 20)
project(CpuEmu-Assembler VERSION 0.1.0 LANGUAGES C CXX)

set(SOURCE_FILE src/main.cpp src/lexer.cpp src/syntax.cpp)

add_executable(CpuEmu-Assembler ${SOURCE_FILE})

