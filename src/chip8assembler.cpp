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
bool checkNumArgs(std::string& mnemonic, std::string& cmd, int n_required, int n_given);
bool checkAddrRange(std::string& cmd, uint16_t addr);
bool checkRegRange(std::string& cmd, int reg);
bool checkRegRange(std::string& cmd, long reg);
bool checkConstRange(std::string& cmd, long lconst);
bool checkConstRange(std::string& cmd, int iconst);
bool checkNibbleRange(std::string& cmd, long lnibble);
bool checkNibbleRange(std::string& cmd, int inibble);
bool checkI(std::string& cmd, std::string& arg);
uint8_t getRegister(std::string& cmd, std::string& reg);
bool getConst(std::string& cmd, std::string& sconst, uint8_t& ret);
uint8_t getNibble(std::string& cmd, std::string& snibble);

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
        while((p < strCode.size()) && (strCode[p] != sNewline) && (strCode[p] != sComment))
        {
            // skip all symbols till {whitespace, indent, newline, comment, comma} occurs
            std::string::size_type p0 = p;
            for(; (p < strCode.size()) && (strCode[p] != sWhitespace) && (strCode[p] != sIndent) && (strCode[p] != sNewline) && (strCode[p] != sComment) && (strCode[p] != sComma); ++p);
            // save word from p0 to current position p
            size_t len = p - p0;
            if(len > 0) // tokens of size 0 can occur but aren't valid (e.g. V0, 1 (whitespace after comma))
            {
                std::string token = strCode.substr(p0, len);
                tokensLine.emplace_back(token);
            }
            // if {newline, comment, EOF} follows last token then make new line token-vector
            if(p == strCode.size() || p == strCode.size()-1 || strCode[p] == sNewline || strCode[p] == sComment)
            {
                // do not save line tokens if:
                //// last token was a marker (we want to treat markers as if they where defined at the same line where they refer to) OR
                //// current symbol is EOF (if file ends on last written character) OR
                //// next symbol is EOF (if file ends on {newline, whitespace})
                if((strCode[p-1] != sMarker) || (p == strCode.size()) || (p == strCode.size()-1))
                {
                    tokens.emplace_back(tokensLine);
                    tokensLine.clear();
                }
            }
            // if token ended on {whitespace, comma} simply skip it
            if(strCode[p] == sComma || strCode[p] == sWhitespace || strCode[p] == sIndent)
                p++;
        }
        // if last char was a comment, skip all chars till next newline
        if((p < strCode.size()) && (strCode[p] == sComment))
            for(; (p < strCode.size()) && (strCode[p] != sNewline); ++p);
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
            if(!checkAddrRange(strCommand, addr)) return false;
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
        printf("0x%03x: %04x\n", i+0x200, machinecode[i]);
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
        fprintf(stderr, "ERROR: marker \"%s\" is not defined (passed: %s).\n", marker.c_str(), cmd.c_str());
        return false;
    }
    return true;
}

bool checkNumArgs(std::string& mnemonic, std::string& cmd, int n_required, int n_given)
{
    if(n_given != n_required)
    {
        fprintf(stderr, "ERROR: invalid number of arguments for \"%s\" (passed: %s). Required: %i, Give: %i\n", mnemonic.c_str(), cmd.c_str(), n_required, n_given);
        return false;
    }
    return true;
}

bool checkRegRange(std::string& cmd, int reg)
{
    if(reg < 0 || reg >= 16)
    {
        fprintf(stderr, "ERROR: Register \"%i\" out of range (passed: %s). Register range from V0-VF.\n", reg, cmd.c_str());
        return false;
    }
    return true;
}

bool checkRegRange(std::string& cmd, long reg)
{
    if(reg < 0 || reg >= 16)
    {
        fprintf(stderr, "ERROR: Register \"%i\" out of range (passed: %s). Register range from V0-VF.\n", reg, cmd.c_str());
        return false;
    }
    return true;
}

bool checkAddrRange(std::string& cmd, uint16_t addr)
{
    // check if most significant nibble is set
    if(0xF000 & addr)
    {
        // error case, address out of range
        fprintf(stderr, "ERROR: Address out of range (passed: %s). Consider that original CHIP-8 only consits of 4K memory.\n", cmd.c_str());
        return false;
    }
    return true;
}

bool checkConstRange(std::string& cmd, long lconst)
{
    // CHIP-8 only supports 8 bit constants so check if given number is representable by 8 bits
    if(lconst >> 8) // check if only least significant byte is set
    {
        fprintf(stderr, "ERROR: constant \"%li\" is not representable by 1 byte (passed: %s). Remember, CHIP-8 is an 8 bit machine.\n", lconst, cmd.c_str());
        return false;
    }
    return true;
}

bool checkConstRange(std::string& cmd, int iconst)
{
    // CHIP-8 only supports 8 bit constants so check if given number is representable by 8 bits
    if(iconst >> 8) // check if only least significant byte is set
    {
        fprintf(stderr, "ERROR: constant \"%i\" is not representable by 1 byte (passed: %s). Remember, CHIP-8 is an 8 bit machine.\n", iconst, cmd.c_str());
        return false;
    }
    return true;
}

bool checkNibbleRange(std::string& cmd, long lnibble)
{
    if(lnibble >> 4) // check if only least significant nibble is set
    {
        fprintf(stderr, "ERROR: nibble \"%li\" is not representable by 4 bit (passed: %s).\n", lnibble, cmd.c_str());
        return false;
    }
    return true;
}

bool checkNibbleRange(std::string& cmd, int inibble)
{
    if(inibble >> 4) // check if only least significant nibble is set
    {
        fprintf(stderr, "ERROR: nibble \"%i\" is not representable by 4 bit (passed: %s).\n", inibble, cmd.c_str());
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
        if(!checkRegRange(cmd, ireg)) return 0xFF;
        // NOTE INVARIANT: register number is in valid range [0,16] which can be implicitly casted to uint8_t since it is 8 bit representable
        return ireg;
    }
    // check if regno is hexadecimal coded
    else if(regno.c_str()[strspn(regno.c_str(), "AaBbCcDdEeFf0123456789")] == 0)
    {
        long lreg = std::strtol(regno.c_str(), NULL, 16);
        // check if register is in range
        if(!checkRegRange(cmd, lreg)) return 0xFF;
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

bool getConst(std::string& cmd, std::string& sconst, uint8_t& ret)
{
    // NOTE constants in CHIP-8 are always coded in 8 bit
    bool result = false;
    // const can be given wether coded decimal or hexadecimal
    // if const is coded hexadecimal it must be given with prefix 0x
    if(!sconst.substr(0, 2).compare("0x"))
    {
        // case: hexadecimal prefix 0x detected
        std::string shex = sconst.substr(2, sconst.size()-1);
        // check if rest of string is valid hexadecimal
        if(shex.c_str()[strspn(shex.c_str(), "AaBbCcDdEeFf0123456789")] == 0)
        {
            // case: const is valid hexadecimal
            long lconst = std::strtol(shex.c_str(), NULL, 16);
            // check if const is 8 bit representable
            if(checkConstRange(cmd, lconst))
            {
                // NOTE INVARIANT: const is 8 bit representable
                ret = lconst;
                result = true;
            }
        }
    }
    // else: const is not hexadecimal coded
    // check if const is decimal coded (no prefix required for decimal encoding)
    else if(sconst.c_str()[strspn(sconst.c_str(), "0123456789")] == 0)
    {
        // case: const is decimal coded
        int iconst = std::strtol(sconst.c_str(), NULL, 10);
        // check if const is 8 bit representable
        if(checkConstRange(cmd, iconst))
        {
            // NOTE INVARIANT: const is 8 bit representable
            ret = iconst;
            result = true;
        }
    }
    else
    {
        // error case: const is wether coded hexadecimal nor decimal
        fprintf(stderr, "ERROR: constants are only allowed to be coded decimal or hexadecimal. hexadecimal coded constants need to be prefixed by \"0x\" (passed: %s).\n", cmd.c_str());
    }
    return result;
}

uint8_t getNibble(std::string& cmd, std::string& snibble)
{
    uint8_t ret = 0xFF;
    // nibble can be given wether coded decimal or hexadecimal
    // if nibble is coded hexadecimal it must be given with prefix 0x
    if(!snibble.substr(0, 2).compare("0x"))
    {
        // case: hexadecimal prefix 0x detected
        std::string shex = snibble.substr(2, snibble.size()-1);
        // check if rest of string is valid hexadecimal
        if(shex.c_str()[strspn(shex.c_str(), "AaBbCcDdEeFf0123456789")] == 0)
        {
            // case: const is valid hexadecimal
            long lnibble = std::strtol(shex.c_str(), NULL, 16);
            // check if const is 4 bit representable
            if(checkConstRange(cmd, lnibble))
            {
                // NOTE INVARIANT: const is 4 bit representable
                ret = lnibble;
            }
        }
    }
    // else: const is not hexadecimal coded
    // check if const is decimal coded (no prefix required for decimal encoding)
    else if(snibble.c_str()[strspn(snibble.c_str(), "0123456789")] == 0)
    {
        // case: const is decimal coded
        int inibble = std::strtol(snibble.c_str(), NULL, 10);
        // check if nibble is 4 bit representable
        if(checkNibbleRange(cmd, inibble))
        {
            // NOTE INVARIANT: const is 4 bit representable
            ret = inibble;
        }
    }
    else
    {
        // error case: nibble is wether coded hexadecimal nor decimal
        fprintf(stderr, "ERROR: nibbles are only allowed to be coded decimal or hexadecimal. hexadecimal coded nibbles need to be prefixed by \"0x\" (passed: %s).\n", cmd.c_str());
    }
    return ret;
}

bool checkI(std::string& cmd, std::string& arg)
{
    // check if arg only consits of letter 'I'
    if(arg.compare("I"))
    {
        fprintf(stderr, "ERROR: required I as an argument but given %s (passed: %s).\n", arg.c_str(), cmd.c_str());
        return false;
    }
    return true;
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
    {
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 0, nargs)) return false;
        // CLS -> 0x00E0
        machinecode.push_back(0x00E0);
        break;
    }
    case RET:
    {
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 0, nargs)) return false;
        // RET -> 0x00EE
        machinecode.push_back(0x00EE);
        break;
    }
    case SYS:
    {
        fprintf(stderr, "ERROR: mnemonic SYS is not support with this version of CHIP-8 (passed: %s).\n", cmd.c_str());
        return false;
    }
    case JP:
    {
        // there exist 2 variants of JP:
        //// JP addr
        //// JP V0, addr
        if(nargs == 1) // check if only one arg is passed
        {
            // JP addr -> 0x1nnn
            // check that arg is no register
            if(isRegister(command[1]))
            {
                fprintf(stderr, "ERROR: JP with only one argument requires address, but register was passed (passed: %s).\n", cmd.c_str());
                return false;
            }
            // check if MS nibbel is unset -> else code is too big to fit in 4k memory of CHIP-8
            if(!markerExists(cmd, command[1])) return false;
            // NOTE INVARIANT: marker is in map and most significant nibble of command[1] is 0
            machinecode.push_back(0x1000 | markers[command[1]]);
        }
        else if(nargs == 2) // check if two args are passed
        {
            // JP V0, addr -> 0xBnnn
            // check if first register is exactly V0
            uint8_t regno = 0;
            if(isRegister(command[1]))
            {
                // check if register is V0
                regno = getRegister(cmd, command[1]);
                if(regno != 0)
                {
                    fprintf(stderr, "ERROR: when JP is passed with 2 arguments, the first one needs to be exactly register V0 (passed: %s).\n", cmd.c_str());
                    return false;
                }
            }
            else
            {
                fprintf(stderr, "ERROR: when JP is passed with 2 arguments, the first one needs to be a register (passed: %s).\n", cmd.c_str());
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
            fprintf(stderr, "ERROR: invalid number of arguments for JP (passsed: %s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case CALL:
    {
        // CALL addr -> 0x2nnn
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 1, nargs)) return false;
        // check if marker is in map
        if(!markerExists(cmd, command[1])) return false;
        // NOTE INVARIANT: marker is valid and its most significant nibble is 0
        machinecode.push_back(0x2000 | markers[command[1]]);
        break;
    }
    case SE:
    {
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // there exist 2 variants of SE:
        //// SE Vx, Vy
        //// SE Vx, byte
        if(isRegister(command[1]) && isRegister(command[2])) // check if both args are registers
        {
            // SE Vx, Vy -> 0x5xy0
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x5000 | (Vx << 8) | (Vy << 4));
        }
        else if(isRegister(command[1]) && !isRegister(command[2])) // check if first arg is register but second is not
        {
            // SE Vx, byte -> 0x3xkk
            // safely retrieve register from first arg
            uint8_t Vx;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            // NOTE INVARIANT: registers is in valid range  [0,16]
            // safely retrieve constant form second arg
            uint8_t byte;
            if(!getConst(cmd, command[2], byte)) return false;
            // NOTE INVARIANT: integer returned by getConst will be in [0,255] -> byte can be represented with 8 bits
            machinecode.push_back(0x3000 | (Vx << 8) | byte);
        }
        else
        {
            // error case, invalid arguments
            fprintf(stderr, "ERROR: invalid arguments passed to SE (passed: %s)\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SNE:
    {
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // there exist 2 variants of SNE:
        //// SNE Vx, Vy
        //// SNE Vx, byte
        if(isRegister(command[1]) && isRegister(command[2])) // check if both args are registers
        {
            // SNE Vx, Vy -> 0x9xy0
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            machinecode.push_back(0x9000 | (Vx << 8) | (Vy << 4));
        }
        else if(isRegister(command[1]) && !isRegister(command[2])) // check if first arg is register and second is not
        {
            // SNE Vx, byte -> 0x4xkk
            // check if register is in range
            uint8_t Vx;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range  [0,16]
            // safely retrieve constant from second arg
            uint8_t byte;
            if(!getConst(cmd, command[2], byte)) return false;
            // NOTE INVARIANT: integer returned by getConst will be in [0,255] -> byte can be represented with 8 bits
            machinecode.push_back(0x4000 | (Vx << 8) | byte);
        }
        else
        {
            // error case, invalid arguments
            fprintf(stderr, "ERROR: invalid arguments passed to SNE (passed: %s)\n", cmd.c_str());
            return false;
        }
        break;
    }
    case LD:
    {
        // TODO go on here ...
        fprintf(stderr, "TODO implement mnemonic \"%s\"\n", mnemonic.c_str());
        return false;
        break;
    }
    case ADD:
    {
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // there exist 3 variants of ADD:
        //// ADD Vx, Vy
        //// ADD Vx, byte
        //// ADD I, Vx
        if(isRegister(command[1]) && isRegister(command[2])) // check if both args are registers
        {
            // ADD Vx, Vy -> 0x8xy4
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            machinecode.push_back(0x8004 | (Vx << 8) | (Vy << 4));
        }
        else if(isRegister(command[1]) && !isRegister(command[2])) // check if first arg is register but second not
        {
            // ADD Vx, byte -> 0x7xkk
            // check if register is in range
            uint8_t Vx;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            // safely retrieve constant
            uint8_t byte;
            if(!getConst(cmd, command[2], byte)) return false;
            // NOTE INVARIANT: integer returned by getConst will be in [0,255] -> byte can be represented with 8 bits
            machinecode.push_back(0x7000 | (Vx << 8) | byte);
        }
        else if(!isRegister(command[1]) && isRegister(command[2])) // check if first arg is not register but second is
        {
            // ADD I, Vx -> 0xFx1E
            // safely retrieve I from first arg
            if(!checkI(cmd, command[1]))
            {
                // error case
                fprintf(stderr, "ERROR: if only second argument of ADD is a register Vx then the first argument must exactly be I (passed: %s).\n", cmd.c_str());
                return false;
            }
            // NOTE INVARIANT: first arg is I
            // safely retrieve register
            uint8_t Vx;
            if((Vx = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            machinecode.push_back(0xF01E | (Vx << 8));
        }
        else
        {
            // error case, invalid arguments
            fprintf(stderr, "ERROR: invalid arguments passed to ADD (passed: %s)\n", cmd.c_str());
            return false;
        }
        break;
    }
    case OR:
    {
        // OR Vx, Vy -> 0x8xy1
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            machinecode.push_back(0x8001 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: OR can only operate on registers, like OR Vx, Vy (passed: %s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case AND:
    {
        // AND Vx, Vy -> 0x8xy2
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            machinecode.push_back(0x8002 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: AND can only operate on registers, like AND Vx, Vy (passed: %s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case XOR:
    {
        // XOR Vx, Vy -> 0x8xy3
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            machinecode.push_back(0x8003 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: XOR can only operate on registers, like XOR Vx, Vy (passed: %s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SUB:
    {
        // SUB Vx, Vy -> 0x8xy5
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            machinecode.push_back(0x8005 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: SUB can only operate on registers, like SUB Vx, Vy (passed: %s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SHR:
    {
        // SHR Vx -> 0x8xy6 // NOTE second register indicated by y in opcode is not used at this operation (strange design?)
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 1, nargs)) return false;
        // safely retrieve register from first argument
        uint8_t Vx;
        if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
        // NOTE INVARIANT: registers will be in valid range [0,16]
        machinecode.push_back(0x8006 | (Vx << 8)); // NOTE assembler will set register y=0 since it is not used at this operation
        break;
    }
    case SUBN:
    {
        // SUBN Vx, Vy -> 0x8xy7
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // check if both arguments are registers
        if(isRegister(command[1]) && isRegister(command[2]))
        {
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            machinecode.push_back(0x8007 | (Vx << 8) | (Vy << 4));
        }
        else
        {
            fprintf(stderr, "ERROR: SUBN can only operate on registers, like SUBN Vx, Vy (passed: %s).\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SHL:
    {
        // SHL Vx -> 0x8xyE // NOTE second register indicated by y in opcode is not used at this operation (strange design?)
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 1, nargs)) return false;
        // safely retrieve register from first argument
        uint8_t Vx;
        if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
        // NOTE INVARIANT: registers will be in valid range [0,16]
        machinecode.push_back(0x800E | (Vx << 8)); // NOTE assembler will set register y=0 since it is not used at this operation
        break;
    }
    case RND:
    {
        // RND Vx, byte -> 0xCxkk
        // check number of arguments
        if(!checkNumArgs(mnemonic, cmd, 2, nargs)) return false;
        // check if first arg is register but second not
        if(isRegister(command[1]) && !isRegister(command[2]))
        {
            // check if register is in range
            uint8_t Vx;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            // safely retrieve constant
            uint8_t byte;
            if(!getConst(cmd, command[2], byte)) return false;
            // NOTE INVARIANT: integer returned by getConst will be in [0,255] -> byte can be represented with 8 bits
            machinecode.push_back(0xC000 | (Vx << 8) | byte);
        }
        else
        {
            fprintf(stderr, "ERROR: invalid call of RND (passed: %s). RND must be called like \"RND Vx, byte\".\n", cmd.c_str());
            return false;
        }
        break;
    }
    case DRW:
    {
        // DRW Vx, Vy, nibble -> 0xDxyn
        // check number of arguments
        if(!checkNumArgs(mnemonic, cmd, 3, nargs)) return false;
        // check if first and second arguments are registers but third not
        if(isRegister(command[1]) && isRegister(command[2]) && !isRegister(command[3]))
        {
            // safely retrieve register from first two arguments
            uint8_t Vx, Vy;
            if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
            if((Vy = getRegister(cmd, command[2])) == 0xFF) return false;
            // NOTE INVARIANT: registers will be in valid range [0,16]
            // safely retrieve nibble from third argument
            uint8_t nibble;
            if((nibble = getNibble(cmd, command[3])) == 0xFF) return false;
            // NOTE INVARIANT: nibble is 4 bit representable -> most significant nible of uint8 will be unset
            machinecode.push_back(0xD000 | (Vx << 8) | (Vy << 4) | nibble);
        }
        else
        {
            fprintf(stderr, "ERROR: invalid call of DRW (passed: %s). DRW must be called like \"DRW Vx, Vy, byte\".\n", cmd.c_str());
            return false;
        }
        break;
    }
    case SKP:
    {
        // SKP Vx -> 0xEx9E
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 1, nargs)) return false;
        // safely retrieve register from first argument
        uint8_t Vx;
        if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
        // NOTE INVARIANT: registers will be in valid range [0,16]
        machinecode.push_back(0xE09E | (Vx << 8));
        break;
    }
    case SKNP:
    {
        // SKNP Vx -> 0xExA1
        // check for number of args
        if(!checkNumArgs(mnemonic, cmd, 1, nargs)) return false;
        // safely retrieve register from first argument
        uint8_t Vx;
        if((Vx = getRegister(cmd, command[1])) == 0xFF) return false;
        // NOTE INVARIANT: registers will be in valid range [0,16]
        machinecode.push_back(0xE0A1 | (Vx << 8));
        break;
    }
    default:
        fprintf(stderr, "ERROR: undefined mnemonic \"%s\" (passed: %s)\n", mnemonic, cmd.c_str());
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
