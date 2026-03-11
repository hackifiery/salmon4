#include "cpu.hpp"
#include <stdexcept>
#include <cassert>
#include <iostream>

using namespace std;

Cpu::Cpu() : acc(0), addrLatch(0), cf(false), pc(0) {
    for (int i = 0; i < MEM; i++)      ram[i]  = 0;
    for (int i = 0; i < PROG_MEM; i++) rom[i]  = 0;
    for (int i = 0; i < NUM_REGS; i++) regs[i] = 0;
}

bool Cpu::step() {
    #define jmp(target) { if (target > 4095) {throw overflow_error("Invalid jump address");} pc = (target); return true; }
    if (pc > 4095) throw overflow_error("Program counter overflow");
    if (acc > 15) throw overflow_error("Acc. overflow");
    // Fetch the 2-byte instruction
    // We combine two 8-bit ROM entries into one 16-bit word
    ui8 byte1 = rom[pc];
    ui8 byte2 = rom[pc + 1];

    ui8 opCodeVal = (byte1 >> 4) & 0x0F;
    // cout << (int)opCodeVal << " " << (int)byte1 << " " << (int)byte2 << endl;

    if (opCodeVal == EXT) {
        ui8 extSubType = byte1 & 0x0F; 
        ui8 arg8       = byte2;
        // cout << (int)opCodeVal << (int)extSubType << (int)arg8 << endl;

        switch (extSubType) {
            case LDI: acc = arg8 & 0x0F; break; 
            case LDR: acc = regs[arg8 & 0x0F]; break;
            case STR: regs[arg8 & 0x0F] = acc; break;
            case XCH: {
                ui8 reg = arg8 & 0x0F;
                ui8 tmp = regs[reg];
                regs[reg] = acc;
                acc = tmp;
                break;
            }
            case STM:
                if (!writeIO(addrLatch, acc)) {
                    ram[addrLatch] = acc;
                }
                break;
            case LDM: 
                if (!readIO(addrLatch)) {
                     acc = ram[addrLatch];
                }
                break;
            
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
                }
                else throw underflow_error("Call stack underflow");
            
            case SHR: acc = acc >> arg8; break;
            case SHL: acc = acc << arg8; break;
            
            case NOP:  break;
            case HALT: return false;
            default:   throw runtime_error("Unknown Opcode");
        }
    }
    else {
        ui16 arg12 = ((byte1 & 0x0F) << 8) | byte2;

        switch (opCodeVal) {
            case LIR: {
                ui8 reg = (arg12 >> 8) & 0x0F;
                ui8 val = arg12 & 0x0F;
                regs[reg] = val;
                break;
            }

            case LIRP: {
                ui8 pairIdx = (arg12 >> 8) & 0x07; // mask to 3 bits (8 pairs max)
                ui8 val     = arg12 & 0xFF;
                ui8 baseReg = pairIdx * 2;
                regs[baseReg] = (val >> 4) & 0x0F; // high nibble to even register
                regs[baseReg + 1] = val & 0x0F; // low nibble to odd register
                break;
            }


            case SRC:
                if (arg12 > MEM-1) throw overflow_error("Invalid memory address");
                addrLatch = arg12;
                break;

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

            case JSR: callStk.push(pc + 2); jmp(arg12);
            case JZ:  if (acc == 0) jmp(arg12); break;
            case JNZ: if (acc != 0) jmp(arg12); break;
            case JC:  if (cf == 1)  jmp(arg12); break;
            case JNC: if (cf != 1)  jmp(arg12); break;
            case JUC: jmp(arg12);
            case JR: 
            {
                int8_t offset = static_cast<int8_t>(byte2);
                ui16 nextInstructionAddr = pc + 2;

                if (!(nextInstructionAddr <= 4095)) throw overflow_error("Invalid jump address");
                
                // should be 24-bit, padded to 32
                int32_t target = static_cast<int32_t>(nextInstructionAddr) + offset;

                jmp(static_cast<ui16>(target & (PROG_MEM - 1)));
            }
            default: throw runtime_error("Unknown Opcode");
        }
    }

    pc += 2; // 2 bytes
    #undef jmp
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

bool Cpu::writeIO(ui16 addr, ui8 val) {
    if (addr < IO_START) return false;
    // simple display for now
    // cout << (int)val << endl;
    ui8 port = addr - IO_START;
    ram[addr] = val & 0x0F;
    switch (port) {
        case 0:
        case 1: break;

        case 2:
            if (val == 0) break;
            cout << static_cast<char>((ram[IO_START] << 4) | ram[IO_START + 1]) << flush;
            ram[IO_START] = 0;
            ram[IO_START + 1] = 0;
            break;
        case 3:
            cout << static_cast<int>(ram[IO_START + 3]) << flush;
            ram[IO_START + 2] = 0;
            break;
    }
    return true;
}

bool Cpu::readIO(ui16 addr) {
    if (addr < IO_START) return false;

    ui8 port = addr - IO_START;
    switch (port) {
        case 4:
        {
            int input;
            cin >> input;
            ram[addr] = static_cast<ui8>(input) & 0x0F;
            break;
        }

        case 5:
        {
            char input;
            cin >> input;
            ram[addr] = static_cast<ui8>(static_cast<ui8>(input) & 0x0F);
            break;
        }
        
        case 6: // TODO: add keyboard drivers
            ram[addr] = 0; 
            break;
    }
    acc = ram[addr];
    return true;
}