cmake_minimum_required (VERSION 3.5)
project (CHIP-8_Emulator)

set (CMAKE_CXX_STANDARD 11)

include_directories (${PROJECT_SOURCE_DIR}/include)
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable (chip8-disassembly ${SRC_FILES})
