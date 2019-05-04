#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <streambuf>
#include <fstream>
#include <vector>

/* function prototypes */
bool parseArgs(int argc, char** argv);
void printUsage();

/* globals */
std::string strFilename = "../code/TEST.ch8";

int main(int argc, char** argv)
{
    // read in args from command line
    if(!parseArgs(argc, argv))
        return EXIT_FAILURE;

    // open file stream
    std::string strCode;
    std::ifstream stream(strFilename.c_str());
    // reserve number of chars needed
    stream.seekg(0, std::ios::end);
    strCode.reserve(stream.tellg());
    stream.seekg(0, std::ios::beg);
    // load chars of file into string
    strCode.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

    // iterate file char-wise and cut whitespace, newlines and comments - any different symbol is treated as token
    char sWhitespace = ' '; char sIndent = '\t'; char sNewline = '\n'; char sComment = '#'; char sComma = ','; char sMarker = ':';
    std::vector<std::vector<std::string>> tokens;
    std::vector<std::string> tokensLine;
    for(std::string::size_type p = 0; p < strCode.size(); ++p)
    {
        // extract all tokens of current line
        // token := all chars between {whitespace, comma, newline} and {whitespace, comma, comment, newline}
        while((strCode[p] != sNewline) && (strCode[p] != sComment))
        {
            std::string::size_type p0 = p;
            while((strCode[p] != sWhitespace) && (strCode[p] != sIndent) && (strCode[p] != sNewline) && (strCode[p] != sComment) && (strCode[p] != sComma)) { p++; }
            // save word from p0 till p-1
            size_t len = p - p0;
            if(len > 0) // tokens of size 0 can occur but aren't valid (e.g. V0, 1 (whitespace after comma))
            {
                std::string token = strCode.substr(p0, len);
                tokensLine.emplace_back(token);
            }
            // if {newline, comment} follows last token then start new line vector
            if((strCode[p] == sNewline || strCode[p] == sComment) && strCode[p-1] != sMarker)
            {
                tokens.emplace_back(tokensLine);
                tokensLine.clear();
            }
            // if token ended on {whitespace, comma} simply skip it
            if(strCode[p] == sComma || strCode[p] == sWhitespace || strCode[p] == sIndent)
                p++;
        }
        // if last char was a comment, skip all chars till next newline
        if(strCode[p] == sComment)
            while(strCode[++p] != sNewline);
        // NOTE INVARIANT: when ending up here strCode[p] == '\n'
    }

    // beg DEBUG
    for(size_t i=0; i<tokens.size(); ++i)
    {
        printf("%li: ", i);
        for(auto t : tokens[i])
            printf("%s ", t.c_str());
        printf("\n");
    }
    // end DEBUG

    // TODO transform tokens to machine code

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