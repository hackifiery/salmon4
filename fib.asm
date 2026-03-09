; simple fibonacci seq

SRC 4091 ; port 3 (integer printing port)

STM      ; initial 0
JSR space

; print the first two 1's
LDI 1
STM
JSR space
STM
JSR space

STR 0
STR 1

loop:
    LDR 0
    ADD 1
    XCHR 0 1
    STR 0
    STM
    JSR space

    ; check if the current value reached 13 (highest 4-bit fib number)
    LIR 14 13
    SUB 14
    LIR 14 0
    JNZ loop
    
JSR newline
HALT

; prints a space
space:
    STR 15    ; tmp reg
    LDI 0     ; clear acc
    LIRP 6 32 ; RP7 = R12 and R13, 32 = space in ascii

    SRC 4088  ; port 0 (ascii printing nibble 0)
    XCH 12
    STM
    XCH 12

    SRC 4089  ; port 1 (ascii printing nibble 1)
    XCH 13
    STM
    XCH 13

    SRC 4090  ; port 2 (ascii toggle)
    LDI 1
    STM

    XCH 15    ; restore acc
    LIR 15 0  ; reset R15

    SRC 4091  ; restore port addr

    RET

newline:
    STR 15    ; tmp reg
    LDI 0     ; clear acc
    LIRP 6 10 ; RP7 = R12 and R13, 10 = newline in ascii

    SRC 4088  ; port 0 (ascii printing nibble 0)
    XCH 12
    STM
    XCH 12

    SRC 4089  ; port 1 (ascii printing nibble 1)
    XCH 13
    STM
    XCH 13

    SRC 4090  ; port 2 (ascii toggle)
    LDI 1
    STM

    XCH 15    ; restore acc
    LIR 15 0  ; reset R15

    SRC 4091  ; restore port addr

    RET