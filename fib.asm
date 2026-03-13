; simple fibonacci seq

.m p0 4088
.m p1 4089
.m p2 4090
.m p3 4091
.m p4 4092

; prints a space
space:
    STR 15    ; tmp reg
    LDI 0     ; clear acc
    LIRP 6 '\s' ; RP7 = R12 and R13, \s = space

    SRC p0    ; port 0 (ascii printing nibble 0)
    XCH 12
    STM
    XCH 12

    SRC p1    ; port 1 (ascii printing nibble 1)
    XCH 13
    STM
    XCH 13

    SRC p2    ; port 2 (ascii toggle)
    LDI 1
    STM

    XCH 15    ; restore acc
    LIR 15 0  ; reset R15

    SRC p3    ; restore port addr

    RET

newline:
    STR 15    ; tmp reg
    LDI 0     ; clear acc
    LIRP 6 '\n' ; RP7 = R12 and R13, \n = newline

    SRC p0    ; port 0 (ascii printing nibble 0)
    XCH 12
    STM
    XCH 12

    SRC p1    ; port 1 (ascii printing nibble 1)
    XCH 13
    STM
    XCH 13

    SRC p2    ; port 2 (ascii toggle)
    LDI 1
    STM

    XCH 15    ; restore acc
    LIR 15 0  ; reset R15

    SRC 4091  ; restore port addr

    RET

getch:
    STR 15    ; tmp reg
    SRC 4092  ; port 4 (integer input)
    LDM       ; input
    STR 14    ; put it in R14
    LDR 15    ; restore acc
    SRC 4091  ; restore port addr
    RET

_start:
    SRC 4091  ; port 3 (integer printing port)

    JSR getch ; get user input

    STM       ; initial 0
    JSR space

    ; print the first 1
    LDI 1
    STM
    JSR space

    LIR 1 0
    STR 0
    
    loop:
    LDR 0
    ADD 1

    JC end    ; check for 4-bit overflow

    STR 15    ; store in tmp reg
    ; check if the current value reached the user-inputted val
    MOV 14 2
    SUB 2
    LIR 2 0
    JZ cont
    JNC end   ; cf will be set if the subtraction result is > 0

    cont:
    LDR 15    ; restore acc
    XCHR 0 1
    STR 0
    STM
    JSR space
    JUC loop

    end:
    JSR newline
    HALT
