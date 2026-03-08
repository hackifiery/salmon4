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
#include "assembler.hpp"
#include "runner.hpp"

using namespace std;

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
