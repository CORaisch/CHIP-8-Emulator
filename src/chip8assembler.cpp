#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <streambuf>
#include <fstream>
#include <vector>
#include <map>
#include <deque>

/* function prototypes */
bool parseArgs(int argc, char** argv);
void printUsage();
bool assemble(std::deque<std::string>& command, std::string& cmd);
bool isRegister(std::string& arg);
bool markerExists(std::string& cmd, std::string& marker);
bool checknargs(std::string& mnemonic, std::string& cmd, int n_required, int n_given);
bool checkaddrrange(std::string& cmd, uint16_t addr);
bool checkregrange(std::string& cmd, int reg);
bool checkregrange(std::string& cmd, long reg);
bool checkconstrange(std::string& cmd, std::string& sbyte);
uint8_t getRegister(std::string& cmd, std::string& reg);

/* globals */
std::string strFilename = "../code/TEST.ch8";
enum mnemonics {CLS, RET, SYS, JP, CALL, SE, SNE, LD, ADD, OR, AND, XOR, SUB, SHR, SUBN, SHL, RND, DRW, SKP, SKNP};
std::map<std::string, int> map_mnemonic;
std::map<std::string, uint16_t> markers;
std::vector<uint16_t> machinecode;

int main(int argc, char** argv)
{
    // read in args from command line
    if(!parseArgs(argc, argv))
        return EXIT_FAILURE;

    // load mnemonic mapping to enum, which will be used later at the assembling step
    map_mnemonic["CLS"] = CLS;   map_mnemonic["cls"] = CLS;
    map_mnemonic["RET"] = RET;   map_mnemonic["ret"] = RET;
    map_mnemonic["SYS"] = SYS;   map_mnemonic["sys"] = SYS;
    map_mnemonic["JP"] = JP;     map_mnemonic["jp"] = JP;
    map_mnemonic["CALL"] = CALL; map_mnemonic["call"] = CALL;
    map_mnemonic["SE"] = SE;     map_mnemonic["se"] = SE;
    map_mnemonic["SNE"] = SNE;   map_mnemonic["sne"] = SNE;
    map_mnemonic["LD"] = LD;     map_mnemonic["ls"] = LD;
    map_mnemonic["ADD"] = ADD;   map_mnemonic["add"] = ADD;
    map_mnemonic["OR"] = OR;     map_mnemonic["or"] = OR;
    map_mnemonic["AND"] = AND;   map_mnemonic["and"] = AND;
    map_mnemonic["XOR"] = XOR;   map_mnemonic["xor"] = XOR;
    map_mnemonic["SUB"] = SUB;   map_mnemonic["sub"] = SUB;
    map_mnemonic["SHR"] = SHR;   map_mnemonic["shr"] = SHR;
    map_mnemonic["SUBN"] = SUBN; map_mnemonic["subn"] = SUBN;
    map_mnemonic["SHL"] = SHL;   map_mnemonic["shl"] = SHL;
    map_mnemonic["RND"] = RND;   map_mnemonic["rnd"] = RND;
    map_mnemonic["DRW"] = DRW;   map_mnemonic["drw"] = DRW;
    map_mnemonic["SKP"] = SKP;   map_mnemonic["skp"] = SKP;
    map_mnemonic["SKNP"] = SKNP; map_mnemonic["sknp"] = SKNP;

    // open file stream
    printf("assemble file \"%s\"\n", strFilename.c_str());
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
    std::vector<std::deque<std::string>> tokens;
    std::deque<std::string> tokensLine;
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
    printf("#### PARSED ASSEMBLY ####\n");
    for(size_t i=0; i<tokens.size(); ++i)
    {
        printf("%li: ", i);
        for(auto t : tokens[i])
            printf("%s ", t.c_str());
        printf("\n");
    }
    printf("\n");
    // end DEBUG

    /* assembly loop */
    // reserve memory for machinecode
    machinecode.reserve(tokens.size());
    // 1 iteration: find and add markers
    for(size_t i=0; i<tokens.size(); ++i)
    {
        // check if line starts with JP-marker -> markers are only allowed to be defined at the beginning of a line
        if(tokens[i].front().back() == sMarker)
        {
            // compose complete command string
            std::string strCommand;
            for(std::string s : tokens[i]) strCommand += s + " ";
            strCommand.pop_back(); // pop last space -> cosmetics
            // extract marker
            tokens[i].front().pop_back();
            uint16_t addr = 0x200 + uint16_t(i);
            // only add marker if address is valid
            if(!checkaddrrange(strCommand, addr)) return false;
            // insert marker
            markers.insert(std::pair<std::string, uint16_t>(tokens[i].front(), addr)); // store PC address pitched by 0x200, since this is the start address of each CHIP-8 programme
            // update mnemonic
            tokens[i].pop_front();
        }
    }
    printf("\n");

    // beg DEBUG
    printf("#### MARKERS ####\n");
    std::map<std::string, uint16_t>::iterator it;
    for(it=markers.begin(); it!=markers.end(); ++it)
        printf("marker: %s -> address: 0x%03x\n", (it->first).c_str(), it->second);
    printf("\n");
    // end DEBUG

    // 2nd iteration: assemble code
    printf("#### ASSEMBLY OUTPUT ####\n");
    for(size_t i=0; i<tokens.size(); ++i)
    {
        // compose complete command string
        std::string strCommand;
        for(std::string s : tokens[i]) strCommand += s + " ";
        strCommand.pop_back(); // pop last space -> cosmetics

        // assemble line by line
        if(!assemble(tokens[i], strCommand))
        {
            // error case
            fprintf(stderr, "ERROR: couldn't assemble command \"%s\"\n", strCommand.c_str());
            break;
        }
    }
    printf("\n");

    // beg DEBUG
    printf("#### MACHINE CODE ####\n");
    for(int i=0; i<machinecode.size(); ++i)
        printf("0x%03x: %04x\n", i, machinecode[i]);
    // end DEBUG

    return EXIT_SUCCESS;
}

bool isRegister(std::string& arg)
{
    if((arg.front() == 'V') || (arg.front() == 'v'))
        return true;
    else
        return false;
}

bool markerExists(std::string& cmd, std::string& marker)
{
    // check if marker has an entry in markers map
    if(markers.find(marker) == markers.end())
    {
        fprintf(stderr, "ERROR: marker \"%s\" is not defined (%s).\n", marker.c_str(), cmd.c_str());
        return false;
    }
    return true;
}

bool checknargs(std::string& mnemonic, std::string& cmd, int n_required, int n_given)
{
    if(n_given != n_required)
    {
        fprintf(stderr, "ERROR: invalid number of arguments for \"%s\" (%s). Required: %i, Give: %i\n", mnemonic.c_str(), cmd.c_str(), n_required, n_given);
        return false;
    }
    return true;
}

bool checkregrange(std::string& cmd, int reg)
{
    if(reg < 0 || reg >= 16)
    {
        fprintf(stderr, "ERROR: Register \"%i\" out of range (%s). Register range from V0-VF.\n", reg, cmd.c_str());
        return false;
    }
    return true;
}

bool checkregrange(std::string& cmd, long reg)
{
    if(reg < 0 || reg >= 16)
    {
        fprintf(stderr, "ERROR: Register \"%i\" out of range (%s). Register range from V0-VF.\n", reg, cmd.c_str());
        return false;
    }
    return true;
}

bool checkaddrrange(std::string& cmd, uint16_t addr)
{
    // check if most significant nibble is set
    if(0xF000 & addr)
    {
        // error case, address out of range
        fprintf(stderr, "ERROR: Address out of range (%s). Consider that original CHIP-8 only consits of 4K memory.\n", cmd.c_str());
        return false;
    }
    return true;
}

bool checkconstrange(std::string& cmd, std::string& sbyte)
{
    // CHIP-8 only supports 8 bit constants so check if given number is representable by 8 bits
    int byte = std::atoi(sbyte.c_str());
    if(byte >> 8) // check if only least significant byte is set
    {
        fprintf(stderr, "ERROR: constant \"%i\" is not representable by 1 byte (%s). Remember, CHIP-8 is an 8 bit machine.\n", byte, cmd.c_str());
        return false;
    }
    return true;
}

uint8_t getRegister(std::string& cmd, std::string& reg)
{
    // extract substring with register number
    std::string regno = reg.substr(1, reg.size()-1);
    // extract register number from string
    // string can be {V,v}0-{V,v}16 or {V,v}0-{V,v}F
    // check if regno is decimal coded
    if(regno.c_str()[strspn(regno.c_str(), "0123456789")] == 0)
    {
        int ireg = std::atoi(regno.c_str());
        // check if register is in range
        if(!checkregrange(cmd, ireg)) return 0xFF;
        // NOTE INVARIANT: register number is in valid range [0,16] which can be implicitly casted to uint8_t since it is 8 bit representable
        return ireg;
    }
    // check if regno is hexadecimal coded
    else if(regno.c_str()[strspn(regno.c_str(), "AaBbCcDdEeFf0123456789")] == 0)
    {
        long lreg = std::strtol(regno.c_str(), NULL, 16);
        // check if register is in range
        if(!checkregrange(cmd, lreg)) return 0xFF;
        // NOTE INVARIANT: register number is in valid range [0,16] which can be implicitly casted to uint8_t since it is 8 bit representable
        return lreg;
    }
    else
    {
        // error case
        fprintf(stderr, "ERROR: invalid register given \"%s\". Registers numbers need to be defined wether coded decimal or hexadecimal and with a leading 'v' or 'V', like 'V12' or 'VC'.\n", reg.c_str());
        return 0xFF;
    }
}

bool assemble(std::deque<std::string>& command, std::string& cmd)
{
    // get mnemonic
    std::string mnemonic = command.front();
    // get number of arguments
    int nargs = command.size() - 1; // -1 since mnemonic is no argument

    // switch for mnemonic
    switch(map_mnemonic[mnemonic])
    {
    case CLS:
        // check for number of args
        if(!checknargs(mnemonic, cmd, 0, nargs)) return false;
        // CLS -> 0x00E0
        machinecode.push_back(0x00E0);
        break;
    case RET:
        // check for number of args
        if(!checknargs(mnemonic, cmd, 0, nargs)) return false;
        // RET -> 0x00EE
        machinecode.push_back(0x00EE);
        break;
    case SYS:
        fprintf(stderr, "ERROR: The mnemonic SYS is not support with this version of CHIP-8 (%s).\n", cmd.c_str());
        return false;
    case JP:
    {
        // there exist 2 variants of JP, one with 1 arg and one with 2 args
        if(nargs == 1)
        {
            // JP addr -> 0x1nnn
            // check number of arguments
            if(!checknargs(mnemonic, cmd, 1, nargs)) return false;
            // check that arg is no register
            if(isRegister(command[1]))
            {
                fprintf(stderr, "ERROR: JP with only one argument requires address, but register was passed (%s).\n", cmd.c_str());
                return false;
            }
            // check if MS nibbel is unset -> else code is too big to fit in 4k memory of CHIP-8
            if(!markerExists(cmd, command[1])) return false;
            // NOTE INVARIANT: marker is in map and most significant nibble of command[1] is 0
            machinecode.push_back(0x1000 | markers[command[1]]);
        }
        else if(nargs == 2)
        {
            // JP V0, addr -> Bnnn
            // check number of arguments
            if(!checknargs(mnemonic, cmd, 2, nargs)) return false;
            // check if first register is V0
            uint8_t regno = 0;
            if(isRegister(command[1]))
            {
                // check if register is V0
                regno = getRegister(cmd, command[1]);
                if(regno != 0)
                {
                    fprintf(stderr, "ERROR: when JP is passed with 2 arguments, the first one needs to be exactly register V0 (%s).\n", cmd.c_str());
                    return false;
                }
            }
            else
            {
                fprintf(stderr, "ERROR: when JP is passed with 2 arguments, the first one needs to be a register (%s).\n", cmd.c_str());
                return false;
            }
            // check if marker is in map
            if(!markerExists(cmd, command[2])) return false;
            // NOTE INVARIANT: first arg is register V0
            // NOTE INVARIANT: second arg is valid marker and its most significant nibble is 0
            machinecode.push_back(0xB000 | markers[command[2]]);
        }
        else
        {
            // error case, too many args for JP
            fprintf(stderr, "ERROR: invalid number of arguments for JP (%s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case CALL:
    {
        // CALL addr -> 2nnn
        // check for number of args
        if(!checknargs(mnemonic, cmd, 1, nargs)) return false;
        // check if marker is in map
        if(!markerExists(cmd, command[1])) return false;
        // NOTE INVARIANT: marker is valid and its most significant nibble is 0
        machinecode.push_back(0x2000 | markers[command[1]]);
        break;
    }
    case SE:
    {
        // check for number of args
        if(!checknargs(mnemonic, cmd, 2, nargs)) return false;
        // there exist 2 variants of SE, one that compares a register with constant and one that compares two registers
        if(isRegister(command[1]) && isRegister(command[2])) // check if second arg is a register
        {
            // SE Vx, Vy -> 0x5xy0
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x5000 | (Vx << 8) | (Vy << 4));
        }
        else if(isRegister(command[1]) && !isRegister(command[2]))
        {
            // SE Vx, byte -> 0x3xkk
            // check if register is in range
            uint8_t Vx;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            // check if second arg (byte) is 8 bit representable
            if(!checkconstrange(cmd, command[2])) return false;
            // NOTE INVARIANT: integer returned by atoi will be in [0,255] -> byte can be represented with 8 bits
            uint8_t byte = std::atoi(command[2].c_str());
            machinecode.push_back(0x3000 | (Vx << 8) | byte);
        }
        else
        {
            // error case, invalid arguments
            fprintf(stderr, "ERROR: invalid arguments passed to SE (%s)\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SNE:
        // check for number of args
        if(!checknargs(mnemonic, cmd, 2, nargs)) return false;
        // there exist 2 variants of SE, one that compares a register with constant and one that compares two registers
        if(isRegister(command[1]) && isRegister(command[2])) // check if second arg is a register
        {
            // SNE Vx, Vy -> 0x9xy0
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x9000 | (Vx << 8) | (Vy << 4));
        }
        else if(isRegister(command[1]) && !isRegister(command[2]))
        {
            // SNE Vx, byte -> 0x4xkk
            // check if register is in range
            uint8_t Vx;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            // check if second arg (byte) is 8 bit representable
            if(!checkconstrange(cmd, command[2])) return false;
            // NOTE INVARIANT: integer returned by atoi will be in [0,255] -> byte can be represented with 8 bits
            uint8_t byte = std::atoi(command[2].c_str());
            machinecode.push_back(0x4000 | (Vx << 8) | byte);
        }
        else
        {
            // error case, invalid arguments
            fprintf(stderr, "ERROR: invalid arguments passed to SNE (%s)\n", cmd.c_str());
            return false;
        }
        break;
    case LD:
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    case ADD:
        // TODO go on here ...
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    case OR:
    {
        // OR Vx, Vy -> 0x8xy1
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x8001 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: OR can only operate on registers, like OR Vx, Vy (%s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case AND:
    {
        // AND Vx, Vy -> 0x8xy2
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x8002 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: AND can only operate on registers, like AND Vx, Vy (%s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case XOR:
    {
        // XOR Vx, Vy -> 0x8xy3
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x8003 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: XOR can only operate on registers, like XOR Vx, Vy (%s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SUB:
    {
        // SUB Vx, Vy -> 0x8xy5
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x8005 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: SUB can only operate on registers, like SUB Vx, Vy (%s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SHR:
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    case SUBN:
    {
        // SUBN Vx, Vy -> 0x8xy7
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x8007 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: SUBN can only operate on registers, like SUBN Vx, Vy (%s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SHL:
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    case RND:
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    case DRW:
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    case SKP:
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    case SKNP:
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    default:
        fprintf(stderr, "ERROR: undefined mnemonic \"%s\" (%s)\n", mnemonic, cmd.c_str());
        return false;
    }

    // if reached this line everything went fine
    return true;
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
