cmake_minimum_required (VERSION 3.5)
project (CHIP-8_Emulator)

set (CMAKE_CXX_STANDARD 11)

include_directories (${PROJECT_SOURCE_DIR}/include)

# make disassembler
add_executable (chip8-disassembly src/chip8disassembler.cpp src/chip8.cpp)

# make assembler
add_executable (chip8-assembly src/chip8assembler.cpp)

# make emulator
add_executable (chip8-emulate src/chip8emulator.cpp src/chip8.cpp)
