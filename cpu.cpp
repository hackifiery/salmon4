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
    assert(opcode < 16 && opcode >= 0);
    ull arg = static_cast<ull>(param);
    assert(arg <= 16 && arg >= 0);

    b4 msb(opcode); // opcode
    b4 lsb(arg); // parameter
    b8 out;

    for (int pos = 0; pos < 8; pos++) {
        if (pos <= 3) out.set(pos, msb[pos]);
        else out.set(pos - 4, lsb[pos - 4]);
    }
    return out;
}

Program::Program(void) {}
void Program::addInstr(const Opcode op, const b4 param) {
    // TODO: add assert here
    progVec.push_back(Instruction(op, param));
}
ui8 Program::size(void) {
    assert(progVec.size() <= PROG_MEM);
    return progVec.size();
}

Cpu::Cpu(void) : ram({0}), rom({0}), pc(0), cf(false), acc(0) {}
void Cpu::loadProg(Program prog) {
    for (int i = 0; i < prog.size(); i++) {
        rom[i] = prog.progVec[i].instrToMem();
    }
}