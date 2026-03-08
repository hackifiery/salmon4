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

using namespace std;

class UnknownInstruction : public runtime_error{using runtime_error::runtime_error;};

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

static int assembler(const vector<string>& argvvec) {
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

static int runner(const vector<string>& argvvec) {
    int argc = (int)argvvec.size();
    if (argc < 2) {
        cerr << "runner: missing program file\n";
        return 1;
    }

    bool verbose = false;
    string fname;
    for (int i = 1; i < argc; ++i) {
        string a = argvvec[i];
        if (a == "-v" || a == "--verbose") verbose = true;
        else if (fname.empty()) fname = a;
        else {
            // ignore extra args for now
        }
    }

    if (fname.empty()) {
        cerr << "runner: no program file specified\n";
        return 1;
    }

    ifstream f(fname, ios::binary);
    if (!f) return 2;

    vector<ui8> bytes;
    char ch;
    while (f.get(ch)) {
        bytes.push_back(static_cast<ui8>(static_cast<char>(ch)));
    }

    Cpu cpu = Cpu();
    for (int i = 0; i < (int)bytes.size(); i++) {
        assert(i < MEM);
        cpu.rom[i] = bytes[i];
    }

    cpu.run(verbose);
    return 0;
}

static vector<string> tokenize(const string& line) {
    vector<string> parts;
    istringstream iss(line);
    string w;
    while (iss >> w) parts.push_back(w);
    return parts;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "SALMON-4 interactive shell. Type 'help' for commands.\n";
        string line;
        while (true) {
            cout << "> ";
            if (!std::getline(cin, line)) break;
            auto parts = tokenize(line);
            if (parts.empty()) continue;
            string cmd = parts[0];
            if (cmd == "exit" || cmd == "quit") break;
            if (cmd == "help") {
                cout << "Commands:\n";
                cout << "  asm <input.asm> [-o output.bin]   Assemble an assembly file\n";
                cout << "  run <program.bin> [-v]            Run a binary in the CPU simulator\n";
                cout << "  quit, exit                        Exit the shell\n";
                continue;
            }

            if (cmd == "asm" || cmd == "assemble") {
                vector<string> subargs;
                subargs.push_back(string("asm"));
                for (size_t i = 1; i < parts.size(); ++i) subargs.push_back(parts[i]);
                int r = assembler(subargs);
                if (r != 0) cerr << "assembler exited with code " << r << "\n";
                continue;
            }

            if (cmd == "run" || cmd == "exec") {
                if (parts.size() < 2) { cerr << "run requires a program file\n"; continue; }
                vector<string> subargs;
                subargs.push_back(string("run"));
                for (size_t i = 1; i < parts.size(); ++i) subargs.push_back(parts[i]);
                int r = runner(subargs);
                if (r != 0) cerr << "runner exited with code " << r << "\n";
                continue;
            }

            cerr << "Unknown command: " << cmd << ". Type 'help' for available commands.\n";
        }

        cout << "Exiting shell.\n";
        return 0;
    }

    string cmd = argv[1];
    if (cmd == "asm" || cmd == "assemble") {
        vector<string> subargs;
        // Build argvvec where index 0 is program name for assembler
        subargs.push_back(string("asm"));
        for (int i = 2; i < argc; ++i) subargs.push_back(string(argv[i]));
        return assembler(subargs);
    }
    else if (cmd == "run" || cmd == "exec") {
        vector<string> subargs;
        // Build argvvec where index 0 is program name for runner
        subargs.push_back(string("run"));
        for (int i = 2; i < argc; ++i) subargs.push_back(string(argv[i]));
        return runner(subargs);
    }
    else {
        cerr << "Unknown command: " << cmd << "\n";
        return 1;
    }
}
