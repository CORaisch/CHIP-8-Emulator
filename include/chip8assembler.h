#ifndef CHIP8ASSEMBLER_H
#define CHIP8ASSEMBLER_H

#include <cstdint>
#include <string>
#include <deque>
#include <map>
#include <vector>

class chip8assembler
{
public:
    chip8assembler(const std::string &file, bool verbose);
    ~chip8assembler() {};

    bool compile();
    void writeMachinecode(const std::string &out);
    void swapEndian();

    bool verbose;
    std::vector<uint16_t> machinecode;

private:
    void parse(std::vector<std::deque<std::string>>& tokens);
    bool assemble(std::vector<std::deque<std::string>>& tokens);

    bool assembleCommand(const std::deque<std::string> &command, const std::string &cmd);
    bool isRegister(const std::string& arg);
    bool markerExists(const std::string& cmd, const std::string& marker);
    bool checkNumArgs(const std::string& mnemonic, const std::string& cmd, int n_required, int n_given);
    bool checkAddrRange(const std::string& cmd, const uint16_t addr);
    bool checkRegRange(const std::string& cmd, const int reg);
    bool checkRegRange(const std::string& cmd, const long reg);
    bool checkConstRange(const std::string& cmd, const long lconst);
    bool checkConstRange(const std::string& cmd, const int iconst);
    bool checkNibbleRange(const std::string& cmd, const long lnibble);
    bool checkNibbleRange(const std::string& cmd, const int inibble);
    bool checkI(const std::string& cmd, const std::string& arg);
    bool getRegister(const std::string& cmd, const std::string& reg, uint8_t& ret);
    bool getConst(const std::string& cmd, const std::string& sconst, uint8_t& ret);
    bool getNibble(const std::string& cmd, const std::string& snibble, uint8_t& ret);

    std::map<std::string, int> map_mnemonic;
    std::map<std::string, uint16_t> markers;
    std::string code;

    enum mnemonics {CLS,RET,SYS,JP,CALL,SE,SNE,LD,ADD,OR,AND,XOR,SUB,SHR,SUBN,SHL,RND,DRW,SKP,SKNP};

    /* special symbols */
    char sWhitespace = ' '; char sIndent = '\t'; char sNewline = '\n';
    char sComment = '#'; char sComma = ','; char sMarker = ':';
};

#endif
