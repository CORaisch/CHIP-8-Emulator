#ifndef CHIP8_H
#define CHIP8_H

#include <stdio.h>
#include <cstdlib>
#include <cstdint>
#include <string>

class chip8
{
public:
    chip8() {}
    ~chip8();

    void init();
    int load_ROM(std::string _filename);
    int fetch_command();
    int exec_command();
    void disassemble_command();
    void print_memory_map(int _cols);
    void print_ROM(int _len, int _cols);

private:
    uint8_t *memory;
    uint8_t *V;
    uint8_t SP;
    uint16_t *stack;
    uint16_t command;
    uint16_t PC;
    uint16_t ST;
    uint16_t DT;
    uint16_t I;
};

#endif
