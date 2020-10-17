#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <string>

class chip8processor
{
public:
    chip8processor();
    ~chip8processor();

    int load_ROM(std::string _filename);
    bool is_running();
    int fetch_command();
    int exec_command();
    void disassemble_command();
    void print_complete_memory_map(int _cols);
    void print_memory(int _cols);
    void print_registers();
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

    const uint16_t FAIL_COMMAND = 0xFFFF; // NOTE 0xFFFF is an invalid opcode, so it will not interfere with other commands
    bool running;
};

#endif
