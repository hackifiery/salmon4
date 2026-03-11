#include <fstream>
#include <iostream>
#include <cassert>
#include <vector>
#include <sstream>
#include <map>
#include <stdexcept>
#include <array>
#include <ios>
#include <string>

#include "cpu.hpp"
#include "asm.hpp"

using namespace std;

class UnknownInstruction : public runtime_error{using runtime_error::runtime_error;};
class UndefinedReference : public runtime_error{using runtime_error::runtime_error;};

static Opcode getOp(string s) {
    #define ret(c, o) if (s == c) return o
    ret("LIR", LIR);
    ret("LIRP", LIRP);
    ret("SRC", SRC);
    ret("ADD", ADD);
    ret("SUB", SUB);
    ret("AND", AND);
    ret("OR", OR);
    ret("XOR", XOR);
    ret("JZ", JZ);
    ret("JNZ", JNZ);
    ret("JC", JC);
    ret("JNC", JNC);
    ret("JUC", JUC);
    ret("JR", JR);
    ret("JSR", JSR);
    ret("EXT", EXT);
    #undef ret
    string msg = "Unknown instruction " + s;
    throw UnknownInstruction(msg);
}

static ExtOpcode getExtOp(string s) {
    #define ret(c, o) if (s == c) return o
    ret("LDI", LDI);
    ret("LDR", LDR);
    ret("STR", STR);
    ret("STM", STM);
    ret("LDM", LDM);
    ret("XCH", XCH);
    ret("MOV", MOV);
    ret("NOT", NOT);
    ret("XCHR", XCHR);
    ret("RCR", RCR);
    ret("RCL", RCL);
    ret("RET", RET);
    ret("SHL", SHL);
    ret("SHR", SHR);
    ret("NOP", NOP);
    ret("HALT", HALT);
    #undef ret
    string msg = "Unknown instruction " + s;
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

int assembler(const vector<string>& argvvec) {
    int argc = (int)argvvec.size();
    assert(argc <= 4 && "Invalid number of arguments");

    string inputFname = "";
    string outputFname = "a.out";

    for (int i = 1; i < argc; ++i) {
        string arg = argvvec[i];
        if (arg == "-o") {
            if (i + 1 < argc) {
                outputFname = argvvec[++i];
            } else {
                cerr << "Error: -o requires an output filename." << endl;
                return 1;
            }
        } else {
            inputFname = arg;
        }
    }

    map<string, ui16> labels;
    vector<string> lines;
    vector<vector<string>> split;
    vector<array<ui8, 4>> parsed;

    ifstream f(inputFname, ios::binary);
    if (!f) {
        cerr << "Error: could not open input file '" << inputFname << "'" << endl;
        return 2;
    }
    ofstream o(outputFname, ios::binary);
    unsigned int virtPC = 0; // Simulated PC for labels

    string str;
    bool hasHalt = false;
    while (getline(f, str)) {
        size_t semi = str.find(";");
        if (semi != string::npos) str = str.substr(0, semi);

        // Strip stuff
        str.erase(0, str.find_first_not_of(" \t\r\n"));
        str.erase(str.find_last_not_of(" \t\r\n") + 1);

        if (str.empty()) continue;
        if (str.back() == ':') {
            labels[str.substr(0, str.length()-1)] = virtPC;
            continue;
        }

        lines.push_back(str);
        virtPC += 2;
    }

    for (string i : lines) split.push_back(splitStr(i));

    for (vector<string> i : split) {
        string opStr = i[0];
        bool ext = false;
        Opcode op = AND; // dummy
        ExtOpcode eop = NOP; // dummy
        for (size_t j = 1; j < i.size(); j++) {
            if (labels.find(i[j]) != labels.end()) {
                i[j] = to_string(labels[i[j]]);
            }
        }
        try {
            op = getOp(opStr);
        }
        catch(UnknownInstruction const&) {
            ext = true;
            eop = getExtOp(opStr);
        }

        if (!ext) {
            // Regular instruction: [opcode:4, upper:4, mid:4, lower:4]
            ui8 opNum = static_cast<ui8>(op);
            ui8 u = 0, m = 0, l = 0;
            if (i.size() == 2) {
                // Case 1: single 12-bit arg (opcode:4, arg:12)
                ui16 tmp = stoi(i[1]);
                l = tmp & 0x0F;
                m = (tmp >> 4) & 0x0F;
                u = (tmp >> 8) & 0x0F;
            }
            else if (i.size() == 3) {
                // Case 2: 4-bit + 8-bit args (opcode (LIR or LIRP):4, arg1:4, arg2:8)
                ui8 arg1 = stoi(i[1]);
                ui16 arg2 = stoi(i[2]);
                u = arg1;
                m = (arg2 >> 4) & 0x0F;
                l = arg2 & 0x0F;
            }
            else {
                assert(i.size() == 4);
                // Case 3: 3 4-bit args (opcode:4, arg1:4, arg2:4, arg3:4)
                u = stoi(i[1]);
                m = stoi(i[2]);
                l = stoi(i[3]);
            }
            parsed.push_back({opNum, u, m, l});
        } else {
            // Extended instruction: [EXT (0xFF):4, ext_opcode:4, high:4, low:4]
            ui8 eOpNum = static_cast<ui8>(eop);

            if (i.size() == 1) {
                // Case 1: no arg (EXT:4, ext_opcode:4)
                if (eOpNum == static_cast<ui8>(HALT)) hasHalt = true;
                parsed.push_back({static_cast<ui8>(EXT), eOpNum, 0, 0});
            } 
            else if (i.size() == 2) {
                // Case 2: single 8-bit param (EXT:4, ext_opcode:4, arg:8)
                ui16 tmp = stoi(i[1]);
                ui8 h = (tmp >> 4) & 0x0F;
                ui8 l = tmp & 0x0F;
                parsed.push_back({static_cast<ui8>(EXT), eOpNum, h, l});
            } 
            else if (i.size() == 3) {
                // Case 3: 2 4-bit params (EXT:4, ext_opcode:4, arg1:4, arg2:4)
                ui8 arg1 = stoi(i[1]);
                ui8 arg2 = stoi(i[2]);
                parsed.push_back({static_cast<ui8>(EXT), eOpNum, arg1, arg2});
            }
        }
    }

    if (!hasHalt) cerr << "Warning: no HALT instruction found" << endl;
    if (labels.find("_start") == labels.end()) throw UndefinedReference("Undefined reference to _start");

    assert(labels["start"] <= 4095);
    o << static_cast<char>((labels["_start"] >> 8) & 0xFF);
    o << static_cast<char>(labels["_start"] & 0xFF);
    for (array<ui8, 4> i : parsed) {
        ui8 byte1 = static_cast<ui8>(((i[0] & 0x0F) << 4) | (i[1] & 0x0F));
        ui8 byte2 = static_cast<ui8>(((i[2] & 0x0F) << 4) | (i[3] & 0x0F));
        o << static_cast<char>(byte1);
        o << static_cast<char>(byte2);
    }

    o.close();
    f.close();

    return 0;
}
