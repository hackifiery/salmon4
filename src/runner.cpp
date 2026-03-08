#include <fstream>
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <ios>

#include "cpu.hpp"

using namespace std;

int runner(const vector<string>& argvvec) {
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
