#pragma once
#include <cstdint>
#include <vector>
#include <stack>
#include <string>
#include <map>
#include <functional>
#include <cassert>

#define MEM 4096
#define IO_MEM 8
#define IO_START MEM-IO_MEM
#define PROG_MEM 4096
#define NUM_REGS 16

typedef uint8_t            ui4;
typedef uint8_t            ui8;
typedef uint16_t           ui16;
typedef unsigned long long ull;

enum Opcode : ui8 {
    LIR,
    LIRP,// wide
    SRC, // wide
    
    ADD,
    SUB,

    AND,
    OR,
    XOR,

    JZ,  // wide
    JNZ, // wide
    JC,  // wide
    JNC, // wide
    JUC, // wide
    JR,  // wide
    JSR, // wide

    EXT = 0xF
};

enum ExtOpcode : ui8 {
    NOP,
    
    LDI,
    LDR,
    STR,
    STM,
    LDM,

    XCH,

    MOV, // wide

    NOT,

    XCHR,

    RCR,
    RCL,

    RET,

    SHR,
    SHL,
    HALT = 0xF
};

struct Instruction {
    ui8 op;    // Base Opcode
    ui8 eOp;   // Extended Opcode (if op == EXT)
    ui16 arg;  // Can hold 4-bit, 8-bit, or 12-bit values
};

class Cpu {
private:
    ui16 getIOAddr(ui8 io_addr);
    void writeIO(ui16 addr, ui8 val);
public:
    Cpu();
    
    // Memory and Registers (using uint8_t for simplicity)
    ui8 ram[MEM];
    ui8 rom[MEM];
    ui8 regs[NUM_REGS];

    ui16 addrLatch;

    ui8  acc;   // Accumulator
    bool cf;    // Carry Flag
    ui16 pc;    // Program Counter (12-bit)
    std::stack<uint16_t> callStk;

    bool step();
    void run(bool verbose = false);
    // debugging helpers
    void printState() const;
};