cxx = g++
cxxflags = -std=c++20 -Wall -Wextra

all: cpu.o runner asm

cpu.o: cpu.cpp
	$(cxx) $(cxxflags) -c $< -o $@

runner: runner.cpp cpu.o
	$(cxx) $(cxxflags) $^ -o $@

asm: asm.cpp cpu.o
	$(cxx) $(cxxflags) $^ -o $@

clean:
	rm -f *.o runner asm

.PHONY: all