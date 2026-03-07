#include "cpu.hpp"
#include <stdexcept>
#include <cassert>
#include <iostream>

using namespace std;

static ui8 toTwosComplement(int8_t input) {
    if (input >= 0) {
        return static_cast<ui8>(input) & 0x0F; // Keep it 4-bit
    }

    ui8 absVal = static_cast<ui8>(-input);
    ui8 result = (~absVal + 1) & 0x0F;
    
    return result;
}

int8_t fromTwosComplement(ui8 raw_bits) {
    // We only care about the lower 4 bits
    ui8 val = raw_bits & 0x0F;
    if (val & 0x08) {
        return static_cast<int8_t>(val) - 16;
    }

    return static_cast<int8_t>(val);
}

Cpu::Cpu() : acc(0), cf(false), pc(0), addrLatch(0) {
    for (int i = 0; i < MEM; i++)      ram[i]  = 0;
    for (int i = 0; i < PROG_MEM; i++) rom[i]  = 0;
    for (int i = 0; i < NUM_REGS; i++) regs[i] = 0;
}

bool Cpu::step() {
    #define JUMP_TO(target) { pc = (target); return true; }
    // Fetch the 2-byte instruction
    // We combine two 8-bit ROM entries into one 16-bit word
    if (acc > 4095) throw overflow_error("Acc. overflow");
    ui8 byte1 = rom[pc];
    ui8 byte2 = rom[pc + 1];

    ui8 opCodeVal = (byte1 >> 4) & 0x0F;

    if (opCodeVal == EXT) {
        ui8 extSubType = byte1 & 0x0F; 
        ui8 arg8       = byte2;

        switch (extSubType) {
            case STM: ram[addrLatch & (MEM-1)] = acc; break;
            case LDM: acc = ram[addrLatch & (MEM-1)]; break;
            
            case MOV:
                {
                    ui8 src = (arg8 >> 4) & 0x0F;
                    ui8 dst = arg8 & 0x0F;
                    regs[dst] = regs[src];
                }
                break;
            
            case XCHR:
                {
                    ui8 src = (arg8 >> 4) & 0x0F;
                    ui8 dst = arg8 & 0x0F;
                    ui8 tmp = regs[dst];
                    regs[dst] = regs[src];
                    regs[src] = tmp;
                }
                break;

            case NOT: acc = (~acc) & 0x0F; break;

            case RCR:
                {
                    bool oldLSB = (acc & 0x01);
                    acc = (acc >> 1) | (cf ? 0x08 : 0x00);
                    cf = oldLSB;
                }
                break;

            case RCL:
                {
                    bool oldMSB = (acc & 0x08);
                    acc = ((acc << 1) & 0x0F) | (cf ? 0x01 : 0x00);
                    cf = oldMSB;
                }
                break;

            case RET: 
                if (!callStk.empty()) {
                    uint16_t retAddr = callStk.top();
                    callStk.pop();
                    pc = (retAddr);
                    return true;
                } else throw underflow_error("Call stack underflow");
            
            case SHR: acc = acc >> arg8; break;
            case SHL: acc = acc << arg8; break;
            
            case NOP:  break;
            case HALT: 
                return false; // Signal that execution should stop
            default:   throw runtime_error("Unknown Opcode");
        }
    }
    else {
        ui16 arg12 = ((byte1 & 0x0F) << 8) | byte2;

        switch (opCodeVal) {
            case LDI: acc = arg12 & 0x0F; break; 
            case LDR: acc = regs[arg12 & 0x0F]; break;
            case STR: regs[arg12 & 0x0F] = acc; break;
            case XCH: {
                ui8 reg = arg12 & 0x0F;
                ui8 tmp = regs[reg];
                regs[reg] = acc;
                acc = tmp;
                break;
            }
            case SRC: addrLatch = arg12; break;

            case ADD: {
                ui16 sum = acc + regs[arg12 & 0x0F];
                acc = sum & 0x0F;
                cf = (sum > 0x0F); 
                break;
            }
            case SUB: {
                ui8 val = regs[arg12 & 0x0F];
                cf = (val > acc);
                acc = (acc - val) & 0x0F;
                break;
            }
            case AND: acc &= regs[arg12 & 0x0F]; break;
            case OR:  acc |= regs[arg12 & 0x0F]; break;
            case XOR: acc ^= regs[arg12 & 0x0F]; break;

            case JSR: callStk.push(pc + 2); JUMP_TO(arg12);
            case JZ:  if (acc == 0) JUMP_TO(arg12); break;
            case JNZ: if (acc != 0) JUMP_TO(arg12); break;
            case JUC: JUMP_TO(arg12);
            case JR: 
            {
                int8_t offset = static_cast<int8_t>(byte2);
                ui16 nextInstructionAddr = pc + 2;

                assert(nextInstructionAddr <= 4095);
                
                // Should be 24-bit, padded to 32
                int32_t target = static_cast<int32_t>(nextInstructionAddr) + offset;

                JUMP_TO(static_cast<ui16>(target & (PROG_MEM - 1)));
            }
            default: throw runtime_error("Unknown Opcode");
        }
    }

    pc += 2; // 2 bytes
    #undef JUMP_TO
    return true;
}

void Cpu::printState() const {
    cout << "PC=" << pc << " ACC=" << (int)acc
         << " CF=" << cf << " ADDRL=" << addrLatch << "\n";
    cout << "REGS:";
    for (int i = 0; i < NUM_REGS; ++i) {
        cout << " R" << i << "=" << (int)regs[i];
    }
    cout << "\n\n";
}

void Cpu::run(bool verbose) {
    bool cont = true;
    while (cont) {
        if (verbose) printState();
        cont = step();
    }
    if (verbose) {
        cout << "-- halted --\n";
        printState();
    }
}
