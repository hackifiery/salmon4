#include <cstdint>
#include <bitset>
#include <cassert>

#include "cpu.hpp"

using namespace std;

template <size_t N> bitset<N> initBitset(ull val) {
    return bitset<N>(val);
}

Instruction::Instruction(const Opcode op, const b4 param) : op(op), param(param) {}
b8 Instruction::instrToMem(void) {
    ull opcode = static_cast<ull>(op);
    assert(opcode < 16); // must fit in 4 bits
    ull arg = static_cast<ull>(param);
    assert(arg < 16); // must fit in 4 bits

    b4 msb(opcode); // opcode
    b4 lsb(arg); // parameter
    b8 out;

    for (int i = 0; i < 4; ++i) {
        out.set(i + 4, msb[i]); // opcode -> upper nibble (bits 4..7)
        out.set(i, lsb[i]); // param -> lower nibble (bits 0..3)
    }
    return out;
}

Program::Program(void) {}
void Program::addInstr(const Opcode op, const b4 param) {
    assert(progVec.size() < PROG_MEM); // ensure we don't overflow program memory
    progVec.push_back(Instruction(op, param));
}
Instruction& Program::operator[](unsigned int index) {
    return progVec[index];
}
ui8 Program::size(void) {
    assert(progVec.size() <= PROG_MEM);
    return progVec.size();
}

Cpu::Cpu(void) : ram({0}), rom({0}), pc(0), cf(false), acc(0) {}
void Cpu::loadProg(Program prog) {
    assert(prog.size() <= PROG_MEM); // ensure program fits rom
    for (ui8 i = 0; i < prog.size(); ++i) {
        rom[i] = prog[i].instrToMem();
    }
}