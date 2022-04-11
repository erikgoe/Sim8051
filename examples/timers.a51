; Timer (and interrupt) test program

ljmp main

intr0:
mov r0, ab ; Do something.
setb b2 ; Reset port pin.
reti
.data 000000 ; Fill space for proper alignment.

t_intr0:
reti
.data 00000000000000 ; Fill space for proper alignment.

; This is not neccessary as interrupt 1 is not enabled.
intr1:
reti
.data 00000000000000 ; Fill space for proper alignment.

t_intr1:
inc r7
reti

main:
setb a8 ; Enable interrupt 0.
setb a9 ; Enable timer 0.
setb ab ; Enable timer 1.
setb af ; Unlock all interrupts.
mov 89, 1 ; Load TMOD: set timer 0 to mode 1.
mov 88, 50 ; Start both timers.
sjmp fe ; Infinite loop.
