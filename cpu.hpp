#pragma once
#include <cstdint>
#include <bitset>
#include <vector>

#define MEM 256
#define MEM_BYTES MEM/2
#define PROG_MEM MEM
#define WORD_SIZE 4
#define PADDING initBitset(0)

typedef std::bitset<4>     b4;
typedef std::bitset<8>     b8;
typedef uint8_t            ui8;
typedef ull ull;

template <size_t N> std::bitset<N> initBitset(ull val);

enum Opcode {
    NOP, // do nothing
    LDI, // number -> acc
    LDR, // reg -> acc
    LDM, // mem -> acc
    LDC, // carry flag -> acc
    STR, // acc -> reg
    STM, // acc -> mem (addr set by SRC)
    SRC, // reg pair -> mem addr latch
    ADD, // acc += reg
    SUB, // acc -= reg
    AND, // acc AND reg
    OR,  // acc OR reg
    NOT, // acc NOT reg
    JZ,  // jmp if acc=0
    JSR, // jmp to subroutine
    HALT // halt cpu
};

class Instruction {
    private:
    public:
    const Opcode op;
    const b4 param;
    Instruction(const Opcode op, const b4 param);
    b8 instrToMem(void);
};

class Program {
    private:
    public:
    std::vector<Instruction> progVec;
    Program(void);
    Instruction& operator[](unsigned int index);
    ui8 size(void);
    void addInstr(const Opcode op, const b4 param);
};

class Cpu {
    private:
    public:
    b4 ram[MEM]; // note: 4-bit words
    b8 rom[PROG_MEM]; // 8 bit words for rom
    ui8 pc;
    bool cf; // carry flag
    b4 acc;
    Cpu(void);
    void loadProg(Program prog);
    void run(void);
};