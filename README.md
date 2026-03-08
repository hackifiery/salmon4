# SALMON-4
A pretty basic 4-bit CPU simulated in C++, very similar Intel 4004's architecture. It has 31 total instructions (achieved using extended instructions), all of which are used. It has 4096 bytes of program memory via 12-bit indexing, and 4096 words (2048 bytes) of RAM.
## Building and running stuff
To build:
```sh
make
```
To assemble a program:
```sh
./salmon4 asm [filename] [-o [output name]]
```
To run a program in the simulator:
```sh
./salmon4 run [binary] [-v]
```
A test program, `fib.asm`, is provided for testing. To run it, assemble and run it manually, or just run `make test`.