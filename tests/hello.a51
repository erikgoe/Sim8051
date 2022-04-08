; This program writes "Hello World!" to 0x80 in internal RAM.

mov r0, 80        ; Output address.
mov r1, 0         ; Initialize counter.
mov dptr, message ; Store the string address.

loop:
mov a, r1
movc a, (a+dptr)  ; Load character from program memory.
mov (r0), a       ; Store indirectly into internal RAM.
inc r0
inc r1
cjne r1, 0c, loop ; Repeat until all 12 characters were copied.
nop               ; stop here (default breakpoint).

message:
.str Hello World!
