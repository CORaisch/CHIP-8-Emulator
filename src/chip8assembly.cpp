#include "chip8assembler.h"
#include <cstdlib>
#include <stdio.h>
#include <cstring>
#include <algorithm>

// forward declarations
bool parseArgs(int argc, char** argv);
void printUsage();

// globals
bool bVerbose = false;
std::string input_file = "../code/TEST.ch8";
std::string output_file;

int main(int argc, char** argv)
{
    /* read in args from command line */
    if (!parseArgs(argc, argv))
        return EXIT_FAILURE;

    // initialize assembler
    chip8assembler assembler(input_file, bVerbose);

    /* compile code */
    if(!assembler.compile())
    {
        printf("ERROR: something went wrong during assembly.\n");
        return EXIT_FAILURE;
    }

    /* write machine code to disk */
    // swap endian before saving to disk on Unix
    assembler.swapEndian();
    assembler.writeMachinecode(output_file);

    return EXIT_SUCCESS;
}

bool parseArgs(int argc, char** argv)
{
    // local helpers
    bool bOutputSet = false;

    // parse commandline arguments
    for (int i = 1; i < argc; ++i)
    {
        // print usage on demand
        if(!std::strcmp(argv[i], "-h") || !std::strcmp(argv[i], "--help"))
        {
            printUsage();
            return false;
        }
        // check for input filename
        if(!std::strcmp(argv[i], "-i") || !std::strcmp(argv[i], "--input"))
        {
            i++;
            if(i < argc)
            {
                input_file = argv[i];
            }
            else
                return false;
        }
        // check for output filename
        if(!std::strcmp(argv[i], "-o") || !std::strcmp(argv[i], "--output"))
        {
            i++;
            if(i < argc)
            {
                bOutputSet = true;
                output_file = argv[i];
            }
            else
                return false;
        }
        // check for verbose flag
        if(!std::strcmp(argv[i], "-v") || !std::strcmp(argv[i], "--verbose"))
        {
            bVerbose = true;
        }
    }

    // if no output filename was given cut ending of input file and use input capitalized filename for output
    if(!bOutputSet)
    {
        // cut path from filename
        output_file = input_file.substr(input_file.find_last_of("/")+1);
        // cut file extension of input filename if exists
        size_t pos_ext;
        if((pos_ext = output_file.find_last_of(".")) != std::string::npos)
            output_file = output_file.substr(0, pos_ext);
        // set capitalized cut input filename as output filename
        std::transform(output_file.begin(), output_file.end(), output_file.begin(), ::toupper);
    }

    return true;
}

void printUsage()
{
    printf( "Usage: chip8disassembly [OPTION]...\n");
    printf( "By default chip8disassembly starts disassembling the MAZE program, which is good for debugging.\n");
    printf( "\nOptions:\n");
    printf( "-h --help                                print usage\n");
    printf( "-i --input PATH/TO/ROM                   set input filename\n");
    printf( "-o --output PATH/TO/ROM                  set output filename\n");
    printf( "-v --verbose                             activate for many outputs\n");
}
