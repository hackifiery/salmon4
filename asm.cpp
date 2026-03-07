#include <fstream>
#include <iostream>
#include <cassert>
#include <vector>
#include <sstream>
#include <map>
#include <stdexcept>

#include "cpu.hpp"

using namespace std;

class UnknownInstruction : public runtime_error{using runtime_error::runtime_error;};

static Opcode getOp(string s) {
    #define ret(c, o) if (s == c) return o
    ret("LDI", LDI);
    ret("LDR", LDR);
    ret("STR", STR);
    ret("SRC", SRC);
    ret("XCH", XCH);
    ret("ADD", ADD);
    ret("SUB", SUB);
    ret("AND", AND);
    ret("OR", OR);
    ret("XOR", XOR);
    ret("JZ", JZ);
    ret("JNZ", JNZ);
    ret("JUC", JUC);
    ret("JR", JR);
    ret("JSR", JSR);
    ret("EXT", EXT);
    #undef ret
    string msg = "Unknown instruction" + s;
    throw UnknownInstruction(msg);
}

static ExtOpcode getExtOp(string s) {
    #define ret(c, o) if (s == c) return o
    ret("STM", STM);
    ret("LDM", LDM);
    ret("MOV", MOV);
    ret("NOT", NOT);
    ret("XCHR", XCHR);
    ret("RCR", RCR);
    ret("RCL", RCL);
    ret("RET", RET);
    ret("NOP", NOP);
    ret("HALT", HALT);
    #undef ret
    string msg = "Unknown instruction" + s;
    throw UnknownInstruction(msg);
}

static vector<string> splitStr(const string& str) {
    vector<string> words;
    stringstream ss(str);
    string word;

    while (ss >> word) {
        words.push_back(word);
    }

    return words;
}

int main(int argc, char* argv[]) {
    assert(argc == 2);

    map<string, ui16> labels;
    vector<string> lines;
    vector<vector<string>> split;
    vector<vector<ui8>> parsed;
    
    ifstream f(argv[1]);
    unsigned int virtPC = 0; // simulated PC for labels

    string str;
    while (getline(f, str)) {
        if (str.find(";") == 0) continue;
        if (str.back() == ':') {
            labels[str.substr(0, str.length()-1)] = virtPC;
            continue;
        }
        lines.push_back(str);
        virtPC += 2;
    }

    for (string i : lines) split.push_back(splitStr(i));

    // Parse instructions: convert to nibbles
    for (vector<string> i : split) {
        string opStr = i[0];
        bool ext = false;
        Opcode op = LDI; // dummy
        ExtOpcode eop = NOP; // dummy
        for (int j = 1; j < i.size(); j++) {
            if (labels.find(i[j]) != labels.end()) {
                i[j] = to_string(labels[i[j]]);
            }
        }
        try {
            op = getOp(opStr);
        }
        catch(UnknownInstruction) {
            ext = true;
            eop = getExtOp(opStr);
        }
        
        if (!ext) {
            // Regular instruction: [opcode, upper_nibble, mid_nibble, lower_nibble]
            ui8 opNum = static_cast<ui8>(op);
            ui8 u = 0, m = 0, l = 0;
            assert(i.size() == 2 || i.size() == 4);
            if (i.size() == 2) {
                // Single 12-bit argument
                ui16 tmp = stoi(i[1]);
                l = tmp & 0x0F;
                m = (tmp >> 4) & 0x0F;
                u = (tmp >> 8) & 0x0F;
            }
            else {
                // Three separate 4-bit arguments
                u = stoi(i[1]);
                m = stoi(i[2]);
                l = stoi(i[3]);
            }
            parsed.push_back({opNum, u, m, l});
        } else {
            // Extended instruction: [EXT, ext_opcode, param_high, param_low]
            ui8 eOpNum = static_cast<ui8>(eop);
            
            if (i.size() == 1) {
                // Type 1: No argument
                parsed.push_back({static_cast<ui8>(EXT), eOpNum, 0, 0});
            } 
            else if (i.size() == 2) {
                // Type 2: One 8-bit parameter (or no arg treated as 0)
                ui16 tmp = stoi(i[1]);
                ui8 high = (tmp >> 4) & 0x0F;
                ui8 low = tmp & 0x0F;
                parsed.push_back({static_cast<ui8>(EXT), eOpNum, high, low});
            } 
            else if (i.size() == 3) {
                // Type 3: Two 4-bit parameters
                ui8 arg1 = stoi(i[1]);
                ui8 arg2 = stoi(i[2]);
                parsed.push_back({static_cast<ui8>(EXT), eOpNum, arg1, arg2});
            }
        }
    }

    // Output as hex nibbles
    for (auto& instr : parsed) {
        for (ui8 nibble : instr) {
            cout << hex << (int)nibble << " ";
        }
    }
    cout << endl;

    return 0;
}