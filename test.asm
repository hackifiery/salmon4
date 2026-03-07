; simple fibbonacci seq
LDI 1
STR 0
XCH 1

loop:
LDR 0
ADD 1
XCHR 0 1
STR 0

; check for overflow
LDI 0
RCR
SHR 3

JZ loop

HALT