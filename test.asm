; simple fibbonacci seq
LDI 1
STR 0
XCH 1

loop: ; loop starts here
LDR 0
ADD 1
XCHR 0 1
STR 0

JNC loop

HALT