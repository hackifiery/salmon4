cxx = g++
cxxflags = -std=c++20 -Wall -Wextra

target = salmon4
objs = cpu.o runner.o asm.o main.o
all: $(target)

cpu.o: src/cpu.cpp
	$(cxx) $(cxxflags) -c $< -o $@

runner.o: src/runner.cpp
	$(cxx) $(cxxflags) -c $^ -o $@

asm.o: src/asm.cpp
	$(cxx) $(cxxflags) -c $^ -o $@

main.o: src/main.cpp
	$(cxx) $(cxxflags) -c $^ -o $@

$(target): $(objs)
	$(cxx) $(cxxflags) $(objs) -o $(target)

clean:
	rm -f *.o *.out salmon4

test:
	./salmon4 asm fib.asm -o fib.out
	./salmon4 run fib.out
	rm -f fib.out

.PHONY: all