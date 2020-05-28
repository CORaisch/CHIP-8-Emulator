#include "chip8.h"
#include <cstring>

/* function prototypes */
bool parseArgs(int argc, char** argv);
void printUsage();

/* globals */
std::string strFilename = "../roms/FISHIE";
int nMemMapCols = 16;
bool bStepMode = false;
bool bVerbose = false;

int main(int argc, char** argv)
{
    // read in args from command line
    if(!parseArgs(argc, argv))
        return EXIT_FAILURE;

    // initialize chip8 emulator
    chip8 CHIP_8;
    CHIP_8.init();

    // load ROM to emulate
    size_t lenROM = CHIP_8.load_ROM(strFilename);
    if(lenROM < 0)
    {
        printf("failed to load ROM %s\n", strFilename.c_str());
        return EXIT_FAILURE;
    }

    if(bVerbose)
    {
        // print memory map
        CHIP_8.print_complete_memory_map(nMemMapCols);

        // print ROM binary
        CHIP_8.print_ROM(lenROM, nMemMapCols);
    }

    // disassemble rom code
    printf("######## RUN EMULATION ########\n");
    while(CHIP_8.is_running())
    {
        // fetch command
        int PC = CHIP_8.fetch_command();

        if(bVerbose)
        {
            printf("\033[1;44m next command \033[0m ");
            CHIP_8.disassemble_command();
        }

        // execute command
        if(CHIP_8.exec_command() < 0)
        {
            fprintf(stderr, "ERROR: some command couldn't be executed. Emulation will be stopped.\n");
            break;
        }

        if(bVerbose)
        {
            // CHIP_8.print_memory(nMemMapCols);
            CHIP_8.print_registers();
            if(bStepMode) getchar();
        }
    }

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
        // check for step-by-step execution
        if(!std::strcmp(argv[i], "-s") || !std::strcmp(argv[i], "--step"))
        {
            bStepMode = true;
        }
        // check for verbose flag
        if(!std::strcmp(argv[i], "-v") || !std::strcmp(argv[i], "--verbose"))
        {
            bVerbose = true;
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
    printf( "-c --cols                               set columns of memory map\n");
}
