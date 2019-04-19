cmake_minimum_required (VERSION 3.5)
project (CHIP-8_Emulator)

set (CMAKE_CXX_STANDARD 11)

include_directories (${PROJECT_SOURCE_DIR}/include)

add_executable (chip8-disassembly ${PROJECT_SOURCE_DIR}/src/chip8disassembler.cpp)
target_link_libraries (chip8-disassembly)
