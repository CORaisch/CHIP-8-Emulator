#include "chip8.h"
#include "utils.h"
#include <string.h>

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
    memset(memory, 0, sizeof(uint8_t)*4096);
    // CHIP-8 has 16 8-Bit registers for general purpose
    V = new uint8_t[16];
    memset(V, 0, sizeof(uint8_t)*16);
    // CHIP-8 allowed for maximal 16 nested subroutine calls
    // the stack is not allowed for general purpose usage
    stack = new uint16_t[16];
    memset(stack, 0, sizeof(uint16_t)*16);
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

    // set emulation state to running
    running = true;

    // seed random generator
    time_t t;
    srand((unsigned int) time(&t));

    // TODO load fonts in memory at location [0x000, 0x200[

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

    // print name of ROM just loaded
    std::set<char> delim{'/'};
    std::vector<std::string> path = splitpath(_filename, delim);
    printf("load ROM \"%s\"\n", path.back().c_str());

    // return size of file in bytes
    return nBytesFile;
}

bool chip8::is_running()
{
    return running; // TODO figure out when to end emulation
}

int chip8::fetch_command()
{
    // check if PC is still in chip8 memory
    if(PC >= 0x0FFE)
    {
        fprintf(stderr, "ERROR: command cannot be fetched since PC is out of scope. PC: 0x%03x\n", PC);
        // set command to fail command
        command = FAIL_COMMAND;
        // stop emulation if emulator is trying to access memory out of scope
        running = false;
        return -1;
    }
    // fetch command, each command is 2 bytes long
    command = memory[PC++];
    command = (command << 8) | memory[PC++];
    // return address of current fetched command
    return PC-2;
}

int chip8::exec_command()
{
    // don't execute command if it wasn't fetched properly
    if(command == FAIL_COMMAND)
    {
        fprintf(stderr, "ERROR: command will not be executed since it couldn't be fetched properly.\n");
        running = false;
        return -1;
    }

    // read opcode (most significant nibble at chip-8)
    uint8_t opcode = command >> 12;

    // handle each opcode
    switch(opcode)
    {
    case 0x0:
    {
        if(command == 0x00E0)
            // TODO cmd: CLS
            fprintf(stderr, "WARNING opcode not implemented: 0x%03x: CLS\n", PC-2);
        else if(command == 0x00EE)
        {
            // cmd: RET
            if(PC > 0)
                PC = stack[--SP];
            else
            {
                fprintf(stderr, "ERROR at 0x%03x: stack is empty, but it is tried to return from subroutine. Command: RET\n", PC-2);
                running = false;
                return -1;
            }
        }
        else
        {
            // cmd: SYS addr
            // NOTE this opcode was only used on hardware implementations of CHIP8, this emulator will ignore it
            uint16_t addr = command & 0x0FFF;
            fprintf(stderr, "WARNING opcode not implemented: 0x%03x: SYS %03x\n", PC-2, addr);
        }
        break;
    }
    case 0x1:
    {
        // cmd: JP addr
        // NOTE memory is not yet checked -> make it robust for segfaults
        uint16_t addr = command & 0x0FFF;
        PC = addr;
        break;
    }
    case 0x2:
    {
        // cmd: CALL addr
        // NOTE memory is not yet checked -> make it robust for segfaults
        uint16_t addr = command & 0x0FFF;
        stack[SP++] = PC; // NOTE PC already points to next command (see chip8::fetch_command())
        PC = addr;
        break;
    }
    case 0x3:
    {
        // cmd: SE Vx, byte
        // NOTE memory is not yet checked -> make it robust for segfaults
        uint8_t byte = command & 0x00FF;
        uint8_t x  = (command & 0x0F00) >> 8;
        if(V[x] == byte) PC += 2;
        break;
    }
    case 0x4:
    {
        // cmd: SNE Vx, byte
        // NOTE memory is not yet checked -> make it robust for segfaults
        uint8_t byte = command & 0x00FF;
        uint8_t x  = (command & 0x0F00) >> 8;
        if(V[x] != byte) PC += 2;
        break;
    }
    case 0x5:
    {
        // cmd: SE Vx, Vy
        // NOTE memory is not yet checked -> make it robust for segfaults
        uint8_t x = (command & 0x0F00) >> 8;
        uint8_t y  = (command & 0x00F0) >> 4;
        if(V[x] == V[y]) PC += 2;
        break;
    }
    case 0x6:
    {
        // cmd: LD Vx, byte
        uint8_t byte = command & 0x00FF;
        uint8_t x  = (command & 0x0F00) >> 8;
        V[x] = byte;
        break;
    }
    case 0x7:
    {
        // cmd: ADD Vx, byte
        // NOTE according to all specs I could find with this ADD operation the carry flag is not changed
        uint8_t byte = command & 0x00FF;
        uint8_t x  = (command & 0x0F00) >> 8;
        V[x] += byte;
        break;
    }
    case 0x8:
    {
        uint8_t x = (command & 0x0F00) >> 8;
        uint8_t y = (command & 0x00F0) >> 4;
        uint8_t nibble = command & 0x000F;
        switch(nibble)
        {
        case 0x0:
            // cmd: LD Vx, Vy
            V[x] = V[y];
            break;
        case 0x1:
            // cmd: OR Vx, Vy
            V[x] |= V[y];
            break;
        case 0x2:
            // cmd: AND Vx, Vy
            V[x] &= V[y];
            break;
        case 0x3:
            // cmd: XOR Vx, Vy
            V[x] ^= V[y];
            break;
        case 0x4:
        {
            // cmd: ADD Vx, Vy
            uint16_t tmp = V[x] + V[y];
            if(tmp > 255)
                V[0xF] = 1;
            else
                V[0xF] = 0;
            V[x] = tmp;
            break;
        }
        case 0x5:
            // cmd: SUB Vx, Vy
            if(V[x] > V[y])
                V[0xF] = 1;
            else
                V[0xF] = 0;
            V[x] -= V[y];
            break;
        case 0x6:
            // cmd: SHR Vx {, Vy}
            V[0xF] = V[x] & 0x01;
            V[x] >>= 1;
            break;
        case 0x7:
            // cmd: SUBN Vx, Vy
            if(V[y] > V[x])
                V[0xF] = 1;
            else
                V[0xF] = 0;
            V[x] = V[y] - V[x];
            break;
        case 0xE:
            // cmd: SHL Vx {, Vy}
            V[0xF] = (V[x] & 0x80) >> 7;
            V[x] <<= 1;
            break;
        default:
            fprintf(stderr, "WARNING unknown opcode: 0x%03x: %04x\n", PC-2, command);
        }
        break;
    }
    case 0x9:
    {
        // cmd: SNE Vx, Vy
        // NOTE memory is not yet checked -> make it robust for segfaults
        uint8_t x = (command & 0x0F00) >> 8;
        uint8_t y  = (command & 0x00F0) >> 4;
        if(V[x] != V[y]) PC += 2;
        break;
    }
    case 0xA:
    {
        // cmd LD I, addr
        // NOTE memory is not yet checked -> make it robust for segfaults
        uint16_t addr = command & 0x0FFF;
        I = addr;
        break;
    }
    case 0xB:
    {
        // cmd: JP V0, addr
        // NOTE memory is not yet checked -> make it robust for segfaults
        uint16_t addr = command & 0x0FFF;
        PC = V[0] + addr;
        break;
    }
    case 0xC:
    {
        // cmd: RND Vx, byte
        uint8_t byte = command & 0x00FF;
        uint8_t x   = (command & 0x0F00) >> 8;
        uint8_t rnd = rand() % 255;
        V[x] = rnd & byte;
        break;
    }
    case 0xD:
    {
        // TODO cmd: DRW Vx, Vy, nibble
        uint8_t Vx = (command & 0x0F00) >> 8;
        uint8_t Vy = (command & 0x00F0) >> 4;
        uint8_t nibble = command & 0x000F;
        fprintf(stderr, "WARNING opcode not implemented: 0x%03x: DRW V%x, V%x, %x\n", PC-2, Vx, Vy, nibble);
        break;
    }
    case 0xE:
    {
        uint8_t Vx = (command & 0x0F00) >> 8;
        uint8_t byte = command & 0x00FF;
        if(byte == 0x9E)
        {
            // TODO cmd: SKP Vx
            fprintf(stderr, "WARNING: opcode not implemented: 0x%03x: SKP V%x\n", PC-2, Vx);
        }
        else if(byte == 0xA1)
        {
            // TODO cmd: SKNP Vx
            fprintf(stderr, "WARNING: opcode not implemented: 0x%03x: SKNP V%x\n", PC-2, Vx);
        }
        else
            fprintf(stderr, "WARNING: unknown opcode: 0x%03x: %04x\n", PC-2, command);
        break;
    }
    case 0xF:
    {
        uint8_t x = (command & 0x0F00) >> 8;
        uint8_t byte = command & 0x00FF;
        switch(byte)
        {
        case 0x07:
            // cmd: LD Vx, DT
            V[x] = DT;
            break;
        case 0x0A:
            // TODO cmd: LD Vx, K
            fprintf(stderr, "WARNING opcode not implemented: 0x%03x: LD V%x, K\n", PC-2, x); break;
        case 0x15:
            // cmd: LD DT, Vx
            DT = V[x];
            break;
        case 0x18:
            // cmd: LD ST, Vx
            ST = V[x];
            break;
        case 0x1E:
            // cmd: ADD I, Vx
            // NOTE memory is not yet checked -> make it robust for segfaults
            I += V[x];
            break;
        case 0x29:
            // TODO cmd: LD F, Vx
            fprintf(stderr, "WARNING opcode not implemented: 0x%03x: LD F, V%x\n", PC-2, x); break;
        case 0x33:
            // cmd: LD B, Vx
            // NOTE memory is not yet checked -> make it robust for segfaults
            memory[I]   = (V[x]-(V[x]%100))/100;
            memory[I+1] = ((V[x]-(V[x]%10))-((V[x]-(V[x]%100))))/10;
            memory[I+2] = V[x] % 10;
            break;
        case 0x55:
            // cmd: LD [I], Vx
            // NOTE memory is not yet checked -> make it robust for segfaults
            for(int i=0; i<=V[x]; ++i) // TODO check wether if i<=V[x] or i<=x is correct
                memory[I+i] = V[i];
            break;
        case 0x65:
            // cmd: LD Vx, [I]
            // NOTE memory is not yet checked -> make it robust for segfaults
            for(int i=0; i<=V[x]; ++i) // TODO check wether if i<=V[x] or i<=x is correct
                V[i] = memory[I+i];
            break;
        default:
            fprintf(stderr, "WARNING unknown opcode: 0x%03x: %04x\n", PC-2, command);
        }
        break;
    }
    default:
        fprintf(stderr, "WARNING unknown opcode: 0x%03x: %04x\n", PC-2, command);
    }

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

void chip8::print_complete_memory_map(int _cols)
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
    printf("######## REGISTERS ########\n");
    printf("PC: 0x%03x\nSP: 0x%03x\nI: %i\nST: %i\nDT: %i\n", PC, SP, I, ST, DT);
    for(int i=0; i<16; ++i)
        printf("V%x: %i\n", i, V[i]);
    printf("######## STACK ########\n");
    for(int i=0; i<16; ++i)
        printf("0x%03x: 0x%03x\n", i, stack[i]);
}

void chip8::print_memory(int _cols)
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

void chip8::print_registers()
{
    printf("######## REGISTERS ########\n");
    printf("PC: 0x%03x\nSP: 0x%03x\nI: %i\nST: %i\nDT: %i\n", PC, SP, I, ST, DT);
    for(int i=0; i<16; ++i)
        printf("V%x: %i\n", i, V[i]);
    printf("######## STACK ########\n");
    for(int i=0; i<16; ++i)
        printf("0x%03x: 0x%03x\n", i, stack[i]);
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
