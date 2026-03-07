#include <cstdint>
#include <fstream>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iterator>

#include "cpu.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    vector<char> raw;
    assert(argc == 2);
    ifstream f(argv[1]);
    char ch;
    while (f.get(ch)) {
        raw.push_back(static_cast<ui8>(ch) & 0x0F);
    }
    Cpu cpu = Cpu();
    for (int i = 0; i < raw.size(); i+=2) {
        if (i+1 < raw.size()) {
            cpu.rom[i/2] = (raw[i] << 4) | raw[i+1];
        }
    }
    for (int i = 0; i < MEM/2; i++) {
        cpu.step();
    }
    return 0;
}