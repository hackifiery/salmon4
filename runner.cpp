#include <cstdint>
#include <fstream>
#include <cassert>
#include <vector>
#include <string>
#include <sstream>

#include "cpu.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    assert(argc == 2);
    ifstream f(argv[1], ios::binary);
    if (!f) return 2;

    vector<ui8> bytes;
    char ch;
    while (f.get(ch)) {
        bytes.push_back(static_cast<ui8>(static_cast<char>(ch)));
    }

    Cpu cpu = Cpu();
    for (int i = 0; i < bytes.size(); i++) {
        assert(i < MEM);
        cpu.rom[i] = bytes[i];
    }

    cpu.run(true);
    return 0;
}