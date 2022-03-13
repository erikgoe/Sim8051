; Simple test program.
; Assembled by hand.

; Load constants to ram.
mov 0x01, #0a ; 75 01 0a
mov 0x02, #0b ; 75 02 0b

; Load RHS value (indirectly).
mov R0, 0x01 ; 78 01
mov A, @R0 ; E6
mov R0, A ; F8

; Load LHS value (directly).
mov A, (0x02) ; E5 02

; Add values (b+a) and write to (03).
add A, R0 ; 28
mov (0x03), A ; F5 03
