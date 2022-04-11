; Simple test program.
; Assembled by hand or with integrated assembler.

; Load constants to ram.
mov 01, 0a ; 75 01 0a
mov 02, 0b ; 75 02 0b

; Load RHS value (indirectly).
mov R0, 01 ; 78 01
mov A, (R0) ; E6
mov R0, A ; F8

; Load LHS value (directly).
mov A, (02) ; E5 02

; Add values (b+a) and write to (03).
add A, R0 ; 28
mov 03, A ; F5 03
