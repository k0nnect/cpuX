; count down from 5 to 1, printing each number.
; r0 holds zero, r1 is the counter, r2 is the constant one.

  set r0, 0
  set r1, 5
  set r2, 1
loop:
  out r1            ; print the counter
  sub r1, r1, r2    ; counter = counter - 1
  blt r0, r1, loop  ; while 0 < counter, go again
  hlt
