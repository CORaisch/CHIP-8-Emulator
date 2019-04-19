#include <stdio.h>
#include <string>
#include <cstring>

/* function prototypes */
bool parseArgs(int argc, char** argv);
void printUsage();
void printMemoryMap(uint8_t *mem, int cols);
void disassemble(uint16_t cmd, int addr);
void printCodeBinary(uint8_t *mem, int nBytes, int cols);

/* globals */
std::string strFilename;
int nMemMapCols = 16;

int main(int argc, char** argv)
{
    /* read in a CHIP8 rom */
    // open file stream
    FILE *pfRom;
    strFilename = "../roms/MAZE"; // set path to default rom
    // parse commandline arguments
    if(!parseArgs(argc, argv))
        return EXIT_FAILURE;
    pfRom = fopen(strFilename.c_str(), "rb");
    if(!pfRom)
    {
        printf("couldn't open file \"%s\"\n", strFilename.c_str());
        return EXIT_FAILURE;
    }
    // get length of file
    fseek(pfRom, 0L, SEEK_END);
    size_t nBytesFile = ftell(pfRom);
    fseek(pfRom, 0L, SEEK_SET);
    // copy rom bytes into array
    uint8_t *chip8Memory = new uint8_t[4096]; // 4K by 8 Bit is regular size of CHIP-8 memory
    size_t nRead = fread(chip8Memory+0x200, sizeof(uint8_t), nBytesFile, pfRom); // load code into memory starting from address 0x200
    if(nRead != nBytesFile)
    {
        printf("number bytes read from file (%li) differs from file size (%li)\n", nRead, nBytesFile);
        return EXIT_FAILURE;
    }
    // close file stream
    fclose(pfRom);
    // print memory map of CHIP-8 to check if code was loaded properly
    printMemoryMap(chip8Memory, nMemMapCols);
    // print binary rom code
    printCodeBinary(chip8Memory, nBytesFile, nMemMapCols);

    /* disassemle rom code */
    printf("######## DISASSEMBLED CODE ########\n");
    for(int i = 0x200; i < 0x200+nBytesFile; ++i)
    {
        uint16_t command = (chip8Memory[i++] << 8) | chip8Memory[i];
        disassemble(command, i-1);
    }

    /* clean up */
    delete[] chip8Memory;

    return EXIT_SUCCESS;
}

bool parseArgs(int argc, char** argv)
{
    // if no arg is passed use default config
    if(argc == 1)
        return true;

    // parse commandline arguments
    for (int i = 1; i < argc; ++i)
    {
        // print usage on demand
        if(!std::strcmp(argv[i], "-h") || !std::strcmp(argv[i], "--help"))
        {
            printUsage();
            return false;
        }
        // check for rom
        if(!std::strcmp(argv[i], "-i") || !std::strcmp(argv[i], "--input"))
        {
            i++;
            if(i < argc)
            {
                strFilename = argv[i];
            }
            else
                return false;
        }
        // check for memory map format
        if(!std::strcmp(argv[i], "-c") || !std::strcmp(argv[i], "--cols"))
        {
            i++;
            if(i < argc)
            {
                nMemMapCols = atoi(argv[i]);
            }
            else
                return false;
        }
    }

    return true;
}

void printUsage()
{
    printf( "Usage: chip8disassembly [OPTION]...\n");
    printf( "By default chip8disassembly starts disassembling the MAZE program, which is good for debugging.\n");
    printf( "\nOptions:\n");
    printf( "-h --help                                print usage\n");
    printf( "-i --input PATH/TO/ROM                   set rom to disassemble\n");
    printf( "-c --colls                               set columns of memory map\n");
}

void printMemoryMap(uint8_t *mem, int cols)
{
    printf("######## MEMORY MAP ########\n");
    int rows = 4096/cols;
    for(int iy = 0; iy < rows; ++iy)
    {
        printf("0x%03x: ", cols*iy);
        for(int ix = 0; ix < cols; ++ix)
            printf("%02x ", mem[cols*iy + ix]);
        printf("\n");
    }
}

void printCodeBinary(uint8_t *mem, int nBytes, int cols)
{
    printf("######## ROM CODE ########\n");
    int rows;
    nBytes % cols == 0 ? rows = nBytes/cols : rows = nBytes/cols + 1;
    for(int iy = 0; iy < rows; ++iy)
    {
        printf("0x%03x: ", 0x200+(cols*iy));
        for(int ix = 0; ix < cols; ++ix)
            printf("%02x ", mem[0x200+(cols*iy + ix)]);
        printf("\n");
    }

}

void disassemble(uint16_t cmd, int pc)
{
    // read opcode
    uint8_t opcode = cmd >> 12;

    // handle each opcode
    switch(opcode)
    {
    case 0x0:
    {
        if(cmd == 0x00E0)
            printf("0x%03x: CLS\n", pc);
        else if(cmd == 0x00EE)
            printf("0x%03x: RET\n", pc);
        else
        {
            uint16_t addr = cmd & 0x0FFF;
            printf("0x%03x: SYS %03x\n", pc, addr);
        }
        break;
    }
    case 0x1:
    {
        uint16_t addr = cmd & 0x0FFF;
        printf("0x%03x: JP %03x\n", pc, addr);
        break;
    }
    case 0x2:
    {
        uint16_t addr = cmd & 0x0FFF;
        printf("0x%03x: CALL %03x\n", pc, addr);
        break;
    }
    case 0x3:
    {
        uint8_t byte = cmd & 0x00FF;
        uint8_t Vx  = (cmd & 0x0F00) >> 8;
        printf("0x%03x: SE V%x, %02x\n", pc, Vx, byte);
        break;
    }
    case 0x4:
    {
        uint8_t byte = cmd & 0x00FF;
        uint8_t Vx  = (cmd & 0x0F00) >> 8;
        printf("0x%03x: SNE V%x, %02x\n", pc, Vx, byte);
        break;
    }
    case 0x5:
    {
        uint8_t Vx = (cmd & 0x0F00) >> 8;
        uint8_t Vy  = (cmd & 0x00F0) >> 4;
        printf("0x%03x: SE V%x, V%x\n", pc, Vx, Vy);
        break;
    }
    case 0x6:
    {
        uint8_t byte = cmd & 0x00FF;
        uint8_t Vx  = (cmd & 0x0F00) >> 8;
        printf("0x%03x: LD V%x, %02x\n", pc, Vx, byte);
        break;
    }
    case 0x7:
    {
        uint8_t byte = cmd & 0x00FF;
        uint8_t Vx  = (cmd & 0x0F00) >> 8;
        printf("0x%03x: ADD V%x, %02x\n", pc, Vx, byte);
        break;
    }
    case 0x8:
    {
        uint8_t Vx = (cmd & 0x0F00) >> 8;
        uint8_t Vy = (cmd & 0x00F0) >> 4;
        uint8_t nibble = cmd & 0x000F;
        switch(nibble)
        {
        case 0x0:
            printf("0x%03x: LD V%x, V%x\n", pc, Vx, Vy); break;
        case 0x1:
            printf("0x%03x: OR V%x, V%x\n", pc, Vx, Vy); break;
        case 0x2:
            printf("0x%03x: AND V%x, V%x\n", pc, Vx, Vy); break;
        case 0x3:
            printf("0x%03x: XOR V%x, V%x\n", pc, Vx, Vy); break;
        case 0x4:
            printf("0x%03x: ADD V%x, V%x\n", pc, Vx, Vy); break;
        case 0x5:
            printf("0x%03x: SUB V%x, V%x\n", pc, Vx, Vy); break;
        case 0x6:
            printf("0x%03x: SHR V%x\n", pc, Vx); break;
        case 0x7:
            printf("0x%03x: SUBN V%x, V%x\n", pc, Vx, Vy); break;
        case 0xE:
            printf("0x%03x: SHL V%x\n", pc, Vx); break;
        default:
            printf("0x%03x: unknown command %04x\n", pc, cmd);
        }
        break;
    }
    case 0x9:
    {
        uint8_t Vx = (cmd & 0x0F00) >> 8;
        uint8_t Vy  = (cmd & 0x00F0) >> 4;
        printf("0x%03x: SNE V%x, V%x\n", pc, Vx, Vy);
        break;
    }
    case 0xA:
    {
        uint16_t addr = cmd & 0x0FFF;
        printf("0x%03x: LD I, %03x\n", pc, addr);
        break;
    }
    case 0xB:
    {
        uint16_t addr = cmd & 0x0FFF;
        printf("0x%03x: JP V0, %03x\n", pc, addr);
        break;
    }
    case 0xC:
    {
        uint8_t byte = cmd & 0x00FF;
        uint8_t Vx   = (cmd & 0x0F00) >> 8;
        printf("0x%03x: RND V%x, %02x\n", pc, Vx, byte);
        break;
    }
    case 0xD:
    {
        uint8_t Vx = (cmd & 0x0F00) >> 8;
        uint8_t Vy = (cmd & 0x00F0) >> 4;
        uint8_t nibble = cmd & 0x000F;
        printf("0x%03x: DRW V%x, V%x, %x\n", pc, Vx, Vy, nibble);
        break;
    }
    case 0xE:
    {
        uint8_t Vx = (cmd & 0x0F00) >> 8;
        uint8_t byte = cmd & 0x00FF;
        if(byte == 0x9E)
            printf("0x%03x: SKP V%x\n", pc, Vx);
        else if(byte == 0xA1)
            printf("0x%03x: SKNP V%x\n", pc, Vx);
        else
            printf("0x%03x: unknown command %04x\n", pc, cmd);
        break;
    }
    case 0xF:
    {
        uint8_t Vx = (cmd & 0x0F00) >> 8;
        uint8_t byte = cmd & 0x00FF;
        switch(byte)
        {
        case 0x07:
            printf("0x%03x: LD V%x, DT\n", pc, Vx); break;
        case 0x0A:
            printf("0x%03x: LD V%x, K\n", pc, Vx); break;
        case 0x15:
            printf("0x%03x: LD DT, V%x\n", pc, Vx); break;
        case 0x18:
            printf("0x%03x: LD ST, V%x\n", pc, Vx); break;
        case 0x1E:
            printf("0x%03x: ADD I, V%x\n", pc, Vx); break;
        case 0x29:
            printf("0x%03x: LD F, V%x\n", pc, Vx); break;
        case 0x33:
            printf("0x%03x: LD B, V%x\n", pc, Vx); break;
        case 0x55:
            printf("0x%03x: LD [I], V%x\n", pc, Vx); break;
        case 0x65:
            printf("0x%03x: LD V%x, [I]\n", pc, Vx); break;
        default:
            printf("0x%03x: unknown command %04x\n", pc, cmd);
        }
        break;
    }
    default:
        printf("0x%03x: unknown command %04x\n", pc, cmd);
    }
}
