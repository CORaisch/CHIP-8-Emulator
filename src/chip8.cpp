#include "chip8.h"

chip8::~chip8()
{
    delete[] memory;
    delete[] stack;
    delete[] V;
}

void chip8::init()
{
    // regular CHIP-8 machines run 4K of memory
    memory = new uint8_t[4096];
    // CHIP-8 has 16 8-Bit registers for general purpose
    V = new uint8_t[16];
    // CHIP-8 allowed for maximal 16 nested subroutine calls
    // the stack is not allowed for general purpose usage
    stack = new uint16_t[16];
    // user code is located from 0x200 onwards
    PC = 0x200;
    // stack initially is empty
    SP = 0;
    // no command fetched initially
    command = 0x0000;
    // no address is loaded initially in I
    I  = 0x000;
    // sound and delay timers are disabled initially
    ST = 0;
    DT = 0;

    printf("CHIP-8 System initialized successfully\n");
}

int chip8::load_ROM(std::string _filename)
{
    // open file stream
    FILE *pfRom;
    pfRom = fopen(_filename.c_str(), "rb");
    if(!pfRom)
    {
        printf("couldn't open file \"%s\"\n", _filename.c_str());
        return -1;
    }

    // get length of file
    fseek(pfRom, 0L, SEEK_END);
    size_t nBytesFile = ftell(pfRom);
    fseek(pfRom, 0L, SEEK_SET);

    // copy rom bytes into CHIP-8 memory starting from address 0x200
    size_t nRead = fread(memory+0x200, sizeof(uint8_t), nBytesFile, pfRom);
    if(nRead != nBytesFile)
    {
        printf("number bytes read from file (%li) differs from file size (%li)\n", nRead, nBytesFile);
        return -1;
    }

    // close file stream
    fclose(pfRom);
    return nBytesFile;
}

int chip8::fetch_command()
{
    // fetch command, each command is 2 bytes long
    command = memory[PC++];
    command = (command << 8) | memory[PC++];
    // return address of current fetched command
    return PC-2;
}

int chip8::exec_command()
{
    // TODO
    return 0;
}

void chip8::disassemble_command()
{
    // read opcode (most significant nibble at chip-8)
    uint8_t opcode = command >> 12;

    // handle each opcode
    switch(opcode)
    {
    case 0x0:
    {
        if(command == 0x00E0)
            printf("0x%03x: CLS\n", PC-2);
        else if(command == 0x00EE)
            printf("0x%03x: RET\n", PC-2);
        else
        {
            uint16_t addr = command & 0x0FFF;
            printf("0x%03x: SYS %03x\n", PC-2, addr);
        }
        break;
    }
    case 0x1:
    {
        uint16_t addr = command & 0x0FFF;
        printf("0x%03x: JP %03x\n", PC-2, addr);
        break;
    }
    case 0x2:
    {
        uint16_t addr = command & 0x0FFF;
        printf("0x%03x: CALL %03x\n", PC-2, addr);
        break;
    }
    case 0x3:
    {
        uint8_t byte = command & 0x00FF;
        uint8_t Vx  = (command & 0x0F00) >> 8;
        printf("0x%03x: SE V%x, %02x\n", PC-2, Vx, byte);
        break;
    }
    case 0x4:
    {
        uint8_t byte = command & 0x00FF;
        uint8_t Vx  = (command & 0x0F00) >> 8;
        printf("0x%03x: SNE V%x, %02x\n", PC-2, Vx, byte);
        break;
    }
    case 0x5:
    {
        uint8_t Vx = (command & 0x0F00) >> 8;
        uint8_t Vy  = (command & 0x00F0) >> 4;
        printf("0x%03x: SE V%x, V%x\n", PC-2, Vx, Vy);
        break;
    }
    case 0x6:
    {
        uint8_t byte = command & 0x00FF;
        uint8_t Vx  = (command & 0x0F00) >> 8;
        printf("0x%03x: LD V%x, %02x\n", PC-2, Vx, byte);
        break;
    }
    case 0x7:
    {
        uint8_t byte = command & 0x00FF;
        uint8_t Vx  = (command & 0x0F00) >> 8;
        printf("0x%03x: ADD V%x, %02x\n", PC-2, Vx, byte);
        break;
    }
    case 0x8:
    {
        uint8_t Vx = (command & 0x0F00) >> 8;
        uint8_t Vy = (command & 0x00F0) >> 4;
        uint8_t nibble = command & 0x000F;
        switch(nibble)
        {
        case 0x0:
            printf("0x%03x: LD V%x, V%x\n", PC-2, Vx, Vy); break;
        case 0x1:
            printf("0x%03x: OR V%x, V%x\n", PC-2, Vx, Vy); break;
        case 0x2:
            printf("0x%03x: AND V%x, V%x\n", PC-2, Vx, Vy); break;
        case 0x3:
            printf("0x%03x: XOR V%x, V%x\n", PC-2, Vx, Vy); break;
        case 0x4:
            printf("0x%03x: ADD V%x, V%x\n", PC-2, Vx, Vy); break;
        case 0x5:
            printf("0x%03x: SUB V%x, V%x\n", PC-2, Vx, Vy); break;
        case 0x6:
            printf("0x%03x: SHR V%x\n", PC-2, Vx); break;
        case 0x7:
            printf("0x%03x: SUBN V%x, V%x\n", PC-2, Vx, Vy); break;
        case 0xE:
            printf("0x%03x: SHL V%x\n", PC-2, Vx); break;
        default:
            printf("0x%03x: unknown command %04x\n", PC-2, command);
        }
        break;
    }
    case 0x9:
    {
        uint8_t Vx = (command & 0x0F00) >> 8;
        uint8_t Vy  = (command & 0x00F0) >> 4;
        printf("0x%03x: SNE V%x, V%x\n", PC-2, Vx, Vy);
        break;
    }
    case 0xA:
    {
        uint16_t addr = command & 0x0FFF;
        printf("0x%03x: LD I, %03x\n", PC-2, addr);
        break;
    }
    case 0xB:
    {
        uint16_t addr = command & 0x0FFF;
        printf("0x%03x: JP V0, %03x\n", PC-2, addr);
        break;
    }
    case 0xC:
    {
        uint8_t byte = command & 0x00FF;
        uint8_t Vx   = (command & 0x0F00) >> 8;
        printf("0x%03x: RND V%x, %02x\n", PC-2, Vx, byte);
        break;
    }
    case 0xD:
    {
        uint8_t Vx = (command & 0x0F00) >> 8;
        uint8_t Vy = (command & 0x00F0) >> 4;
        uint8_t nibble = command & 0x000F;
        printf("0x%03x: DRW V%x, V%x, %x\n", PC-2, Vx, Vy, nibble);
        break;
    }
    case 0xE:
    {
        uint8_t Vx = (command & 0x0F00) >> 8;
        uint8_t byte = command & 0x00FF;
        if(byte == 0x9E)
            printf("0x%03x: SKP V%x\n", PC-2, Vx);
        else if(byte == 0xA1)
            printf("0x%03x: SKNP V%x\n", PC-2, Vx);
        else
            printf("0x%03x: unknown command %04x\n", PC-2, command);
        break;
    }
    case 0xF:
    {
        uint8_t Vx = (command & 0x0F00) >> 8;
        uint8_t byte = command & 0x00FF;
        switch(byte)
        {
        case 0x07:
            printf("0x%03x: LD V%x, DT\n", PC-2, Vx); break;
        case 0x0A:
            printf("0x%03x: LD V%x, K\n", PC-2, Vx); break;
        case 0x15:
            printf("0x%03x: LD DT, V%x\n", PC-2, Vx); break;
        case 0x18:
            printf("0x%03x: LD ST, V%x\n", PC-2, Vx); break;
        case 0x1E:
            printf("0x%03x: ADD I, V%x\n", PC-2, Vx); break;
        case 0x29:
            printf("0x%03x: LD F, V%x\n", PC-2, Vx); break;
        case 0x33:
            printf("0x%03x: LD B, V%x\n", PC-2, Vx); break;
        case 0x55:
            printf("0x%03x: LD [I], V%x\n", PC-2, Vx); break;
        case 0x65:
            printf("0x%03x: LD V%x, [I]\n", PC-2, Vx); break;
        default:
            printf("0x%03x: unknown command %04x\n", PC-2, command);
        }
        break;
    }
    default:
        printf("0x%03x: unknown command %04x\n", PC-2, command);
    }
}

void chip8::print_memory_map(int _cols)
{
    printf("######## MEMORY MAP ########\n");
    int rows = 4096/_cols;
    for(int iy = 0; iy < rows; ++iy)
    {
        printf("0x%03x: ", _cols*iy);
        for(int ix = 0; ix < _cols; ++ix)
            printf("%02x ", memory[_cols*iy + ix]);
        printf("\n");
    }
}

void chip8::print_ROM(int _len, int _cols)
{
    printf("######## ROM CODE ########\n");
    int rows;
    _len % _cols == 0 ? rows = _len/_cols : rows = _len/_cols + 1;
    for(int iy = 0; iy < rows; ++iy)
    {
        printf("0x%03x: ", 0x200+(_cols*iy));
        for(int ix = 0; ix < _cols; ++ix)
            printf("%02x ", memory[0x200+(_cols*iy + ix)]);
        printf("\n");
    }
}
